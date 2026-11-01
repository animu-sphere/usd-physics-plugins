#include "usd_physics/jolt/availability.h"

#include "usd_physics/core/errors.h"

#include <cstdint>
#include <iostream>
#include <memory>
#include <stdexcept>

namespace core = usd_physics::core;
namespace jolt = usd_physics::jolt;

namespace {
constexpr int overlappingBodies = 100; // 4,950 pairs, above the default 1,024

void check(bool condition, const char *message) {
  if (!condition)
    throw std::runtime_error(message);
}

// Whether a pile of overlapping dynamic boxes survives a few steps.
bool stepsPile(core::PhysicsWorld &world) {
  const auto shape = world.createShape({});
  for (int i = 0; i != overlappingBodies; ++i)
    (void)world.createBody(
        {shape, core::MotionType::dynamicBody, {{i * .001, 0, 0}, {}}, 1});
  try {
    for (int step = 0; step != 3; ++step)
      world.step(core::PhysicsWorld::Duration(1.0 / 120));
  } catch (const core::PhysicsError &error) {
    check(error.code() == core::PhysicsErrorCode::solverFailure, "exhaustion code");
    return false;
  }
  return true;
}
} // namespace

int main() {
  try {
    check(!stepsPile(*jolt::createWorld()), "the default capacity is exhausted");
    jolt::WorldCapacity capacity;
    capacity.bodyPairs = 16384;
    capacity.contactConstraints = 16384;
    capacity.temporaryBytes = 64 * 1024 * 1024;
    check(stepsPile(*jolt::createWorld({}, {0, -9.81, 0}, capacity)), "a larger capacity steps");
    for (auto invalid : {&jolt::WorldCapacity::bodies, &jolt::WorldCapacity::bodyPairs,
                         &jolt::WorldCapacity::contactConstraints}) {
      jolt::WorldCapacity zero;
      zero.*invalid = 0;
      bool refused = false;
      try {
        (void)jolt::createWorld({}, {}, zero);
      } catch (const std::invalid_argument &) {
        refused = true;
      }
      check(refused, "a zero capacity rejects");
    }
    jolt::WorldCapacity noTemporary;
    noTemporary.temporaryBytes = 0;
    bool refused = false;
    try {
      (void)jolt::createWorld({}, {}, noTemporary);
    } catch (const std::invalid_argument &) {
      refused = true;
    }
    check(refused, "zero temporary memory rejects");
    return 0;
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
