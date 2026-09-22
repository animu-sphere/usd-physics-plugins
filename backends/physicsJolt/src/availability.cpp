#include "usd_physics/jolt/availability.h"

#include "usd_physics/core/errors.h"

namespace usd_physics::jolt {

bool backendAvailable() noexcept {
  return false;
}

std::unique_ptr<core::PhysicsWorld> createWorld() {
  throw core::PhysicsError{core::PhysicsErrorCode::backendUnavailable,
                           "physicsJolt was built without Jolt Physics"};
}

} // namespace usd_physics::jolt
