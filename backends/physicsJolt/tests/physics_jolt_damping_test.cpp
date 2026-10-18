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

double linearResponse(core::BodyDamping damping, int hz) {
  auto world = jolt::createWorld();
  const auto shape = world->createShape({});
  const auto body = world->createBody(
      {shape, core::MotionType::dynamicBody, {}, 1, {}, {}, damping});
  check(world->setLinearVelocity(body, {10, 0, 0}), "start translation");
  for (int i = 0; i != hz; ++i)
    world->step(core::PhysicsWorld::Duration{1.0 / hz});
  return world->bodyState(body).linearVelocity.x / 10;
}

double angularSpeed(core::Quaternion a, core::Quaternion b, double dt) {
  const double x = a.w * b.x - a.x * b.w - a.y * b.z + a.z * b.y;
  const double y = a.w * b.y + a.x * b.z - a.y * b.w - a.z * b.x;
  const double z = a.w * b.z - a.x * b.y + a.y * b.x - a.z * b.w;
  const double w = a.w * b.w + a.x * b.x + a.y * b.y + a.z * b.z;
  return 2 * std::atan2(std::sqrt(x * x + y * y + z * z), std::abs(w)) / dt;
}

double angularResponse(core::BodyDamping damping, int hz) {
  auto world = jolt::createWorld();
  const auto floorShape =
      world->createShape({core::ShapeType::box, {20, .5, 5}});
  const auto floor = world->createBody({floorShape,
                                        core::MotionType::staticBody,
                                        {{0, -.5, 0}, {}},
                                        0,
                                        {},
                                        {1, 0}});
  core::ShapeDescriptor sphere;
  sphere.type = core::ShapeType::sphere;
  const auto shape = world->createShape(sphere);
  const auto body = world->createBody({shape,
                                       core::MotionType::dynamicBody,
                                       {{0, .51, 0}, {}},
                                       1,
                                       {},
                                       {1, 0},
                                       damping});
  check(world->setLinearVelocity(body, {5, 0, 0}), "start rolling");
  // Seed spin by contact, then remove the floor. A sphere in free fall has
  // no applied torque. Measure rotation without expanding the public API.
  for (int i = 0; i != 120; ++i)
    world->step(core::PhysicsWorld::Duration{1.0 / 240});
  check(world->destroyBody(floor), "remove spin source");
  const double dt = 1.0 / hz;
  auto previous = world->bodyState(body).transform.rotation;
  world->step(core::PhysicsWorld::Duration{dt});
  auto current = world->bodyState(body).transform.rotation;
  const double initial = angularSpeed(previous, current, dt);
  check(initial > 1 && std::isfinite(initial),
        "contact must seed measurable spin");
  for (int i = 0; i != hz; ++i) {
    previous = current;
    world->step(core::PhysicsWorld::Duration{dt});
    current = world->bodyState(body).transform.rotation;
  }
  return angularSpeed(previous, current, dt) / initial;
}
} // namespace

int main() {
  try {
    for (const int hz : {60, 240}) {
      for (const auto damping :
           {core::BodyDamping{0, 0}, {2, 0}, {0, 2}, {2, 2}}) {
        const double linear = linearResponse(damping, hz);
        const double angular = angularResponse(damping, hz);
        check(std::abs(linear - std::exp(-damping.linear)) < .015,
              "linear decay must approximate the independent rate");
        check(std::abs(angular - std::exp(-damping.angular)) < .015,
              "angular decay must approximate the independent rate");
        std::cout << hz << " Hz rates=" << damping.linear << ','
                  << damping.angular << " retention=" << linear << ','
                  << angular << '\n';
      }
    }
    auto world = jolt::createWorld();
    const auto shape = world->createShape({});
    for (const auto type :
         {core::MotionType::staticBody, core::MotionType::dynamicBody,
          core::MotionType::kinematicBody}) {
      for (const double invalid :
           {-1.0, std::numeric_limits<double>::quiet_NaN(),
            std::numeric_limits<double>::infinity(),
            std::numeric_limits<double>::max(),
            std::numeric_limits<double>::min()}) {
        for (const auto damping :
             {core::BodyDamping{invalid, 0}, {0, invalid}}) {
          bool rejected = false;
          try {
            world->createBody({shape, type, {}, 1, {}, {}, damping});
          } catch (const std::invalid_argument &) {
            rejected = true;
          }
          check(rejected, "unsafe damping must reject for every motion type");
        }
      }
    }
    check(world->destroyShape(shape), "rejected bodies must not retain shapes");
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
  return 0;
}
