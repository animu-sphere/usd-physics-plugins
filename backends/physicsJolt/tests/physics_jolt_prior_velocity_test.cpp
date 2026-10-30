#include "usd_physics/core/kinematic_motion.h"
#include "usd_physics/jolt/availability.h"

#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <vector>

namespace {
using namespace usd_physics;
using Mode = core::BodyDampingMode;

void check(bool condition, const char* message) {
  if (!condition)
    throw std::runtime_error(message);
}

void near(double actual, double expected, const char* message) {
  if (!std::isfinite(actual) || std::abs(actual - expected) > 2e-5) {
    std::cerr << message << ": actual=" << actual << " expected=" << expected << '\n';
    throw std::runtime_error(message);
  }
}

double angularSpeed(core::Quaternion a, core::Quaternion b, double dt) {
  const double x = a.w * b.x - a.x * b.w - a.y * b.z + a.z * b.y;
  const double y = a.w * b.y + a.x * b.z - a.y * b.w - a.z * b.x;
  const double z = a.w * b.z - a.x * b.y + a.y * b.x - a.z * b.w;
  const double w = a.w * b.w + a.x * b.x + a.y * b.y + a.z * b.z;
  return 2 * std::atan2(std::sqrt(x*x + y*y + z*z), std::abs(w)) / dt;
}

// Analytic free-body response distinguishes clearing prior velocity from
// damping away the newly applied force, and from clearing just once at creation.
std::vector<core::BodyState> forceResponse(core::PhysicsWorld& world,
                                          core::ShapeHandle shape, int hz,
                                          Mode mode, bool gravity, bool force) {
  const double dt = 1.0 / hz;
  const auto body = world.createBody(
      {shape, core::MotionType::dynamicBody, {{0, 10, 0}, {}}, 2, {}, {},
       {0, 0, mode, Mode::clearPriorVelocity}});
  const auto initialState = world.bodyState(body);
  check(world.setLinearVelocity(body, {10, 0, 0}), "seed prior velocity");
  for (const double invalid : {0.0, -1.0,
                               std::numeric_limits<double>::quiet_NaN(),
                               std::numeric_limits<double>::min()}) {
    try {
      world.step(core::PhysicsWorld::Duration{invalid});
      throw std::runtime_error("invalid step admitted");
    } catch (const std::invalid_argument&) {}
    near(world.bodyState(body).linearVelocity.x, 10,
         "invalid step must not clear velocity");
  }
  std::vector<core::BodyState> result;
  core::Vector3 expectedVelocity{10, 0, 0};
  core::Vector3 expectedPosition{0, 10, 0};
  for (int i = 0; i != 4; ++i) {
    // Omit force on the last step to verify that the force accumulator resets.
    const bool apply = force && i != 3;
    if (apply)
      check(world.applyForce(body, {4, 6, 0}), "queue force before clearing");
    if (mode == Mode::clearPriorVelocity)
      expectedVelocity = {};
    expectedVelocity.x += (apply ? 2 : 0) * dt;
    expectedVelocity.y += ((apply ? 3 : 0) - (gravity ? 9.81 : 0)) * dt;
    expectedPosition.x += expectedVelocity.x * dt;
    expectedPosition.y += expectedVelocity.y * dt;
    world.step(core::PhysicsWorld::Duration{dt});
    const auto state = world.bodyState(body);
    near(state.linearVelocity.x, expectedVelocity.x, "new force survives clearing");
    near(state.linearVelocity.y, expectedVelocity.y, "new gravity survives clearing");
    near(state.transform.translation.x, expectedPosition.x, "force position response");
    near(state.transform.translation.y, expectedPosition.y, "gravity position response");
    result.push_back(state);
    const auto changed = world.takeChangedBodyStates();
    const auto& previous = i == 0 ? initialState : result[i-1];
    const bool moved = state.transform != previous.transform ||
                       state.linearVelocity != previous.linearVelocity;
    check(changed.size() == (moved ? 1u : 0u), "changed-state publication");
    if (!changed.empty())
      check(changed[0].body == body && changed[0].transform == state.transform &&
                changed[0].linearVelocity == state.linearVelocity,
            "changed state contains post-force result");
  }
  check(world.destroyBody(body), "destroy clearing body");
  check(world.takeChangedBodyStates().empty(), "destroy drains changes");
  return result;
}

void sameReplay(const std::vector<core::BodyState>& first,
                const std::vector<core::BodyState>& second) {
  check(first.size() == second.size(), "replay size");
  for (std::size_t i = 0; i != first.size(); ++i)
    check(first[i].transform == second[i].transform &&
              first[i].linearVelocity == second[i].linearVelocity,
          "body/world rebuild replays identically");
}

void independentChannels(int hz, Mode linearMode, Mode angularMode, double rate) {
  auto world = jolt::createWorld();
  const auto floorShape = world->createShape({core::ShapeType::box, {20, .5, 5}});
  const auto floor = world->createBody(
      {floorShape, core::MotionType::staticBody, {{0, -.5, 0}, {}}, 0, {}, {1, 0}});
  core::ShapeDescriptor sphere;
  sphere.type = core::ShapeType::sphere;
  const auto shape = world->createShape(sphere);
  const auto body = world->createBody(
      {shape, core::MotionType::dynamicBody, {{0, .51, 0}, {}}, 1, {}, {1, 0},
       {linearMode == Mode::finiteRate ? rate : 0,
        angularMode == Mode::finiteRate ? rate : 0, linearMode, angularMode}});
  const double dt = 1.0 / hz;
  double seededSpeed = 0;
  for (int i = 0; i != hz / 2; ++i) {
    check(world->setLinearVelocity(body, {5, -1, 0}), "seed contact spin");
    // Linear clearing also clears the explicitly commanded speed. A horizontal
    // force generates fresh contact motion in that mode, including fresh torque.
    check(world->applyForce(body, {5.0 / dt, -1.0 / dt, 0}), "seed contact force");
    const auto before = world->bodyState(body).transform.rotation;
    world->step(core::PhysicsWorld::Duration{dt});
    seededSpeed = angularSpeed(before, world->bodyState(body).transform.rotation, dt);
  }
  check(seededSpeed > .1, "fresh contact torque survives angular clearing");
  check(world->destroyBody(floor), "remove contact torque source");
  const auto before = world->bodyState(body);
  world->step(core::PhysicsWorld::Duration{dt});
  const auto after = world->bodyState(body);
  const double speed = angularSpeed(before.transform.rotation,
                                     after.transform.rotation, dt);
  if (angularMode == Mode::clearPriorVelocity)
    check(speed < 1e-4, "prior angular velocity must clear without new torque");
  else
    check(speed > .1, "linear clearing must preserve prior angular velocity");
  if (linearMode == Mode::clearPriorVelocity)
    near(after.linearVelocity.x, 0, "angular clearing must not select linear mode");
  else
    near(after.linearVelocity.x, before.linearVelocity.x * (1 - rate * dt),
         "angular clearing must preserve prior linear velocity");
  world->step(core::PhysicsWorld::Duration{dt});
  const double nextSpeed = angularSpeed(after.transform.rotation,
                                         world->bodyState(body).transform.rotation, dt);
  if (angularMode == Mode::finiteRate)
    check(std::abs(nextSpeed / speed - (1 - rate * dt)) < .003,
          "linear clearing must preserve finite angular decay");
}

void constraintResponse(int hz) {
  auto world = jolt::createWorld({}, {0, 0, 0});
  auto* motion = dynamic_cast<core::KinematicMotion*>(world.get());
  check(motion != nullptr, "kinematic capability");
  const auto shape = world->createShape({});
  const core::BodyDamping clear{0, 0, Mode::clearPriorVelocity, Mode::clearPriorVelocity};
  const auto anchor = world->createBody(
      {shape, core::MotionType::kinematicBody, {}, 0, {1, 0}, {}, clear});
  const auto body = world->createBody(
      {shape, core::MotionType::dynamicBody, {{3, 0, 0}, {}}, 1, {1, 0}, {}, clear});
  const auto joint = world->createConstraint({core::ConstraintType::fixed, anchor, body});
  const double dt = 1.0 / hz;
  for (int i = 1; i <= 3; ++i) {
    const double angle = i * dt;
    const core::Transform target{{i * dt, 0, 0},
                                 {std::cos(angle/2), 0, 0, std::sin(angle/2)}};
    check(motion->moveKinematic(anchor, target, core::PhysicsWorld::Duration{dt}),
          "command fixed anchor");
    const auto before = world->bodyState(body).transform.rotation;
    world->step(core::PhysicsWorld::Duration{dt});
    const auto state = world->bodyState(body);
    near(world->bodyState(anchor).transform.translation.x, target.translation.x,
         "clearing must not change kinematic motion");
    check(state.linearVelocity.x > .1 &&
              angularSpeed(before, state.transform.rotation, dt) > .1,
          "fresh linear/angular constraint impulses survive clearing");
  }
  check(world->destroyConstraint(joint), "remove constraint");
  const auto before = world->bodyState(body).transform;
  world->step(core::PhysicsWorld::Duration{dt});
  const auto state = world->bodyState(body);
  check(state.linearVelocity == core::Vector3{} &&
            angularSpeed(before.rotation, state.transform.rotation, dt) < 1e-4,
        "constraint velocity clears on following step");
}

void motionTypes() {
  auto world = jolt::createWorld();
  const auto shape = world->createShape({});
  const core::BodyDamping clear{0, 0, Mode::clearPriorVelocity, Mode::clearPriorVelocity};
  const auto body = world->createBody(
      {shape, core::MotionType::staticBody, {{0, 10, 0}, {}}, 0, {}, {}, clear});
  const auto initial = world->bodyState(body);
  for (int i = 0; i != 120; ++i)
    world->step(core::PhysicsWorld::Duration{1.0 / 60});
  check(world->bodyState(body).transform == initial.transform &&
            world->bodyState(body).linearVelocity == core::Vector3{},
        "clearing leaves static bodies unchanged");
}
} // namespace

int main() {
  try {
    for (const int hz : {60, 120, 240}) {
      for (const auto mode : {Mode::finiteRate, Mode::clearPriorVelocity}) {
        for (const bool gravity : {false, true}) {
          for (const bool force : {false, true}) {
            auto world = jolt::createWorld({}, {0, gravity ? -9.81 : 0, 0});
            const auto shape = world->createShape({});
            const auto first = forceResponse(*world, shape, hz, mode, gravity, force);
            sameReplay(first, forceResponse(*world, shape, hz, mode, gravity, force));
            auto rebuilt = jolt::createWorld({}, {0, gravity ? -9.81 : 0, 0});
            sameReplay(first, forceResponse(*rebuilt, rebuilt->createShape({}),
                                             hz, mode, gravity, force));
          }
        }
      }
      for (const auto linear : {Mode::finiteRate, Mode::clearPriorVelocity})
        for (const auto angular : {Mode::finiteRate, Mode::clearPriorVelocity})
          for (const double rate : {0.0, 2.0})
            independentChannels(hz, linear, angular, rate);
      constraintResponse(hz);
      std::cout << hz << " Hz: prior velocity, fresh force/gravity/contact/constraint, "
                   "independent channels and body/world rebuild passed\n";
    }
    motionTypes();
  } catch (const std::exception& error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
