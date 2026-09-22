#include "usd_physics/core/version.h"
#include "usd_physics/core/errors.h"
#include "usd_physics/jolt/availability.h"

int main() {
  const auto version = usd_physics::core::version();
  if (version.major != 0) {
    return 1;
  }
  if (usd_physics::jolt::backendAvailable()) {
    return usd_physics::jolt::createWorld() ? 0 : 2;
  }
  try {
    static_cast<void>(usd_physics::jolt::createWorld());
  } catch (const usd_physics::core::PhysicsError& error) {
    return error.code() ==
                   usd_physics::core::PhysicsErrorCode::backendUnavailable
               ? 0
               : 3;
  }
  return 4;
}
