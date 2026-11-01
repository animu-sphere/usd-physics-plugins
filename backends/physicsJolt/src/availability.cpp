#include "usd_physics/jolt/availability.h"

#include "usd_physics/core/errors.h"

#include <stdexcept>

namespace usd_physics::jolt {

bool backendAvailable() noexcept {
  return false;
}

std::unique_ptr<core::PhysicsWorld> createWorld() {
  throw core::PhysicsError{core::PhysicsErrorCode::backendUnavailable,
                           "physicsJolt was built without Jolt Physics"};
}

std::unique_ptr<core::PhysicsWorld>
createWorld(const core::ContactMixing &mixing) {
  return createWorld(mixing, {0.0, -9.81, 0.0});
}

std::unique_ptr<core::PhysicsWorld>
createWorld(const core::ContactMixing &mixing, core::Vector3 gravity) {
  core::validateContactMixing(mixing);
  core::validatePhysicsVector(gravity, "world gravity");
  return createWorld();
}

std::unique_ptr<core::PhysicsWorld>
createWorld(const core::ContactMixing &mixing, core::Vector3 gravity,
            const WorldCapacity &capacity) {
  if (!capacity.bodies || !capacity.bodyPairs || !capacity.contactConstraints ||
      !capacity.temporaryBytes)
    throw std::invalid_argument("world capacity values must be positive");
  return createWorld(mixing, gravity);
}

} // namespace usd_physics::jolt
