#include "usd_physics/jolt/availability.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>

namespace {
using namespace usd_physics;

double slide(double floorFriction, double bodyFriction) {
  auto world = jolt::createWorld();
  const auto floor = world->createShape({core::ShapeType::box, {20, .5, 5}});
  const auto box = world->createShape({});
  world->createBody({floor, core::MotionType::staticBody, {{0, -.5, 0}, {}},
                     0, {}, {floorFriction, 0}});
  const auto body = world->createBody(
      {box, core::MotionType::dynamicBody, {{0, .51, 0}, {}},
       1, {}, {bodyFriction, 0}});
  // Start sliding before the initially resting body can sleep.
  for (int i = 0; i != 24; ++i) world->step(core::PhysicsWorld::Duration{1.0/240});
  world->setLinearVelocity(body, {5, 0, 0});
  for (int i = 0; i != 120; ++i) world->step(core::PhysicsWorld::Duration{1.0/240});
  return world->bodyState(body).linearVelocity.x;
}

double bounce(double floorRestitution, double bodyRestitution) {
  auto world = jolt::createWorld();
  const auto floor = world->createShape({core::ShapeType::box, {5, .5, 5}});
  core::ShapeDescriptor sphere;
  sphere.type = core::ShapeType::sphere;
  const auto shape = world->createShape(sphere);
  world->createBody({floor, core::MotionType::staticBody, {{0, -.5, 0}, {}},
                     0, {}, {0, floorRestitution}});
  const auto body = world->createBody(
      {shape, core::MotionType::dynamicBody, {{0, 3, 0}, {}},
       1, {}, {0, bodyRestitution}});
  double upward = 0;
  for (int i = 0; i != 360; ++i) {
    world->step(core::PhysicsWorld::Duration{1.0/240});
    const auto velocity = world->bodyState(body).linearVelocity.y;
    if (!std::isfinite(velocity)) throw std::runtime_error("non-finite bounce");
    upward = std::max(upward, velocity);
  }
  return upward;
}
}

int main() {
  const auto free = slide(0, 1), otherFree = slide(1, 0);
  const auto rough = slide(1, 1), mixed = slide(.25, 1);
  std::cout << "slide speeds " << free << ' ' << otherFree << ' ' << rough << ' ' << mixed << '\n';
  if (!(free > 4.7 && otherFree > 4.7 && std::abs(rough) < .5 &&
        mixed > 1.8 && mixed < 3)) return 1;
  const auto dead = bounce(0, 0), live = bounce(0, .8), otherLive = bounce(.8, 0);
  const auto unequal = bounce(.25, .8);
  std::cout << "bounce speeds " << dead << ' ' << live << ' ' << otherLive << ' ' << unequal << '\n';
  if (!(dead < .5 && live > 4 && otherLive > 4 && std::abs(unequal-live) < .1)) return 2;

  auto world = jolt::createWorld();
  const auto shape = world->createShape({});
  for (const auto material :
       {core::ContactMaterial{-1, 0}, core::ContactMaterial{0, 1.1},
        core::ContactMaterial{std::numeric_limits<double>::max(), 0},
        core::ContactMaterial{std::numeric_limits<double>::min(), 0},
        core::ContactMaterial{0, std::numeric_limits<double>::min()},
        core::ContactMaterial{1e20, 0},
        core::ContactMaterial{std::ldexp(1.0, 64), 0}}) {
    try {
      world->createBody({shape, core::MotionType::staticBody, {}, 0, {}, material});
      return 3;
    } catch (const std::invalid_argument&) {
    }
  }
  // Rejection must leave the world usable, with no body retaining the shape.
  if (!world->destroyShape(shape)) return 4;
  return 0;
}
