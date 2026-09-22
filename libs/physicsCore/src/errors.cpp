#include "usd_physics/core/errors.h"

namespace usd_physics::core {

PhysicsError::PhysicsError(PhysicsErrorCode code, const std::string& message)
    : std::runtime_error(message), code_(code) {}

} // namespace usd_physics::core
