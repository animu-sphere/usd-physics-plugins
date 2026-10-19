#include "usd_physics/jolt/availability.h"

#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>

namespace {
using namespace usd_physics;

void check(bool condition, const char *message) {
  if (!condition)
    throw std::runtime_error(message);
}

void near(core::Vector3 actual, core::Vector3 expected, double tolerance,
          const char *message) {
  check(std::abs(actual.x - expected.x) < tolerance &&
            std::abs(actual.y - expected.y) < tolerance &&
            std::abs(actual.z - expected.z) < tolerance,
        message);
}

void response(core::PhysicsWorld &world, core::Vector3 gravity, int hz) {
  const auto shape = world.createShape({});
  const auto moving = world.createBody(
      {shape, core::MotionType::dynamicBody, {}, 1, {}, {}, {0, 0}});
  const core::Transform unmoving{{100, 100, 100}, {}};
  const auto fixed = world.createBody(
      {shape, core::MotionType::staticBody, unmoving});
  const auto kinematic = world.createBody(
      {shape, core::MotionType::kinematicBody, {{200, 100, 100}, {}}});
  check(world.setLinearVelocity(moving, {1, 2, 3}), "initial velocity");
  const core::PhysicsWorld::Duration dt{1.0 / hz};
  for (int i = 0; i != hz / 2; ++i)
    world.step(dt);
  const auto state = world.bodyState(moving);
  near(state.linearVelocity,
       {1 + gravity.x * .5, 2 + gravity.y * .5, 3 + gravity.z * .5},
       1e-4, "world acceleration reaches velocity");
  // Semi-implicit integration has a displacement error of g*t*dt/2.
  near(state.transform.translation,
       {.5 + gravity.x * .125, 1 + gravity.y * .125,
        1.5 + gravity.z * .125},
       .05, "world acceleration reaches position");
  near(world.bodyState(fixed).transform.translation, unmoving.translation,
       1e-6, "gravity does not move static bodies");
  near(world.bodyState(kinematic).transform.translation, {200, 100, 100},
       1e-6, "gravity does not move kinematic bodies");
  check(world.destroyBody(moving) && world.destroyBody(fixed) &&
            world.destroyBody(kinematic) && world.destroyShape(shape),
        "gravity world resource cleanup");
  check(world.takeChangedBodyStates().empty(), "no destroyed changed states");
}
} // namespace

int main() {
  try {
    const core::ContactMixing mixing{core::ContactCombineRule::product,
                                   core::ContactCombineRule::product, 10};
    // Keep different worlds alive together; the setting is per world.
    auto defaults = jolt::createWorld();
    auto mixingOnly = jolt::createWorld(mixing);
    auto directed = jolt::createWorld(mixing, {2, 3, -4});
    auto zero = jolt::createWorld({}, {});
    for (const int hz : {60, 240}) {
      response(*directed, {2, 3, -4}, hz);
      response(*zero, {}, hz);
      response(*defaults, {0, -9.81, 0}, hz);
      response(*mixingOnly, {0, -9.81, 0}, hz);
    }
    for (const double value : {std::numeric_limits<double>::infinity(),
                               -std::numeric_limits<double>::infinity(),
                               std::numeric_limits<double>::quiet_NaN(),
                               std::numeric_limits<double>::max(),
                               -std::numeric_limits<double>::max(),
                               std::numeric_limits<double>::min(),
                               -std::numeric_limits<double>::min()}) {
      for (const auto gravity : {core::Vector3{value, 0, 0},
                                 core::Vector3{0, value, 0},
                                 core::Vector3{0, 0, value}}) {
        bool rejected = false;
        try {
          (void)jolt::createWorld({}, gravity);
        } catch (const std::invalid_argument &) {
          rejected = true;
        }
        check(rejected, "invalid backend acceleration rejected");
      }
    }
    auto rebuilt = jolt::createWorld({}, {0, 1, 0});
    response(*rebuilt, {0, 1, 0}, 60);
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
