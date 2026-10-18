#include "usd_physics/core/six_dof.h"
#include "usd_physics/jolt/availability.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>

using namespace usd_physics;
namespace {
void check(bool ok, const char *text) {
  if (!ok)
    throw std::runtime_error(text);
}
template <class F> void rejects(F f) {
  try {
    f();
  } catch (const std::invalid_argument &) {
    return;
  }
  throw std::runtime_error("invalid rotation spring accepted");
}
core::Vector3 direction(int axis, bool rotated) {
  if (axis == 2)
    return {0, 0, 1};
  if (axis == 0)
    return rotated ? core::Vector3{0, 1, 0} : core::Vector3{1, 0, 0};
  return rotated ? core::Vector3{-1, 0, 0} : core::Vector3{0, 1, 0};
}
double angle(core::Quaternion q, core::Vector3 axis) {
  return 2 * std::atan2(q.x * axis.x + q.y * axis.y + q.z * axis.z, q.w);
}
// Uniform sphere: I=2*m*r*r/5, independent of the backend's motor settings.
double response(int axis, bool rotated, int endpoint, bool limited,
                bool allAxes, int hz, double mass, double stiffness,
                double ratio) {
  auto world = jolt::createWorld();
  auto &cap = *dynamic_cast<core::SixDofConstraints *>(world.get());
  core::ShapeDescriptor sphere;
  sphere.type = core::ShapeType::sphere;
  sphere.radius = .5;
  const auto shape = world->createShape(sphere);
  const auto dir = direction(axis, rotated);
  const double initial = .1;
  const auto body = world->createBody(
      {shape,
       core::MotionType::dynamicBody,
       {{},
        {std::cos(initial / 2), dir.x * std::sin(initial / 2),
         dir.y * std::sin(initial / 2), dir.z * std::sin(initial / 2)}},
       mass,
       {1, 0},
       {0, 0},
       {0, 0}});
  core::SixDofConstraintDescriptor d;
  d.secondBody = body;
  if (endpoint == 1 || endpoint == 2 || endpoint == 4)
    d.firstBody =
        world->createBody({shape,
                           endpoint == 2 ? core::MotionType::dynamicBody
                                         : core::MotionType::staticBody,
                           {},
                           mass,
                           {1, 0},
                           {0, 0},
                           {0, 0}});
  if (endpoint >= 3)
    std::swap(d.firstBody, d.secondBody);
  if (rotated)
    d.firstFrame.rotation =
        d.secondFrame.rotation = {std::sqrt(.5), 0, 0, std::sqrt(.5)};
  for (auto &limit : d.limits)
    limit.motion = core::AxisMotion::locked;
  d.limits[axis + 3] = limited
                           ? core::AxisLimit{core::AxisMotion::limited, -.2, .2}
                           : core::AxisLimit{};
  const double inertia = .1 * mass;
  const double effective = endpoint == 2 ? inertia / 2 : inertia;
  const double omega = std::sqrt(stiffness / effective), decay = ratio * omega;
  const double damped = omega * std::sqrt(1 - ratio * ratio);
  d.rotationSprings[axis] = {stiffness, 2 * effective * decay};
  if (allAxes) {
    for (int other = 0; other != 3; ++other) {
      if (other == axis)
        continue;
      d.limits[other + 3].motion = core::AxisMotion::free;
      d.rotationSprings[other] = {stiffness * (other + 1),
                                  2 * effective * decay};
    }
  }
  const auto constraint = cap.createSixDofConstraint(d);
  double worst = 0;
  for (int i = 1; i <= hz / 2; ++i) {
    world->step(core::PhysicsWorld::Duration{1.0 / hz});
    const auto q = world->bodyState(body).transform.rotation;
    const double second = angle(q, dir);
    const double first =
        endpoint == 2
            ? angle(world->bodyState(d.firstBody).transform.rotation, dir)
            : 0;
    const double t = double(i) / hz;
    const double expected =
        initial * std::exp(-decay * t) *
        (std::cos(damped * t) + decay / damped * std::sin(damped * t));
    worst = std::max(worst, std::abs(second - first - expected) / initial);
    check(std::abs(q.x - dir.x * std::sin(second / 2)) < 1e-5 &&
              std::abs(q.y - dir.y * std::sin(second / 2)) < 1e-5 &&
              std::abs(q.z - dir.z * std::sin(second / 2)) < 1e-5,
          "rotation spring axis isolation");
    if (endpoint == 2)
      check(std::abs(first + second - initial) < 1e-4,
            "angular momentum balance");
  }
  check(world->destroyConstraint(constraint), "rotation spring cleanup");
  return worst;
}
} // namespace
int main() {
  try {
    for (const int hz : {60, 240}) {
      double worst = 0;
      for (int axis = 0; axis != 3; ++axis)
        for (bool rotated : {false, true})
          for (int endpoint : {0, 1, 2, 3, 4})
            for (double mass : {1., 4.})
              for (double ratio : {0., .2})
                for (bool allAxes : {false, true})
                  worst = std::max(worst,
                                   response(axis, rotated, endpoint, axis != 1,
                                            allAxes, hz, mass, 5., ratio));
      std::cout << hz << " Hz rotation max normalized analytic error=" << worst
                << '\n';
      check(worst < (hz == 60 ? .24 : .07),
            "rotation spring analytic response");
    }
    auto world = jolt::createWorld();
    auto &cap = *dynamic_cast<core::SixDofConstraints *>(world.get());
    const auto shape = world->createShape({});
    const auto body = world->createBody({shape, core::MotionType::dynamicBody});
    for (int axis = 0; axis != 3; ++axis) {
      for (const double bad :
           {-1., std::numeric_limits<double>::infinity(),
            std::numeric_limits<double>::quiet_NaN(), 1e100, 1e-100}) {
        core::SixDofConstraintDescriptor d;
        d.secondBody = body;
        d.rotationSprings[axis] = {bad, 0};
        rejects([&] { cap.createSixDofConstraint(d); });
        d.rotationSprings[axis] = {1, bad};
        rejects([&] { cap.createSixDofConstraint(d); });
      }
    }
    // Motors cannot pull through a hard angular stop.
    core::SixDofConstraintDescriptor d;
    d.secondBody = body;
    for (auto &limit : d.limits)
      limit.motion = core::AxisMotion::locked;
    d.limits[3] = {core::AxisMotion::limited, .2, .4};
    d.rotationSprings[0] = {10, 1};
    const auto constraint = cap.createSixDofConstraint(d);
    for (int i = 0; i != 240; ++i)
      world->step(core::PhysicsWorld::Duration{1. / 240});
    const double final =
        angle(world->bodyState(body).transform.rotation, {1, 0, 0});
    check(std::abs(final - .2) < .01,
          "rotation spring must respect hard limit");
    check(world->destroyConstraint(constraint),
          "cleanup after rejected descriptors");
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
