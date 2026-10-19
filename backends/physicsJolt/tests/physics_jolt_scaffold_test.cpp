#include "usd_physics/core/version.h"
#include "usd_physics/core/errors.h"
#include "usd_physics/jolt/availability.h"

#include <limits>
#include <stdexcept>

int main() {
  const auto version = usd_physics::core::version();
  if (version.major != 0) {
    return 1;
  }
  for (double value : {std::numeric_limits<double>::infinity(),
                       -std::numeric_limits<double>::infinity(),
                       std::numeric_limits<double>::quiet_NaN()}) {
    for (const auto gravity : {usd_physics::core::Vector3{value, 0, 0},
                               usd_physics::core::Vector3{0, value, 0},
                               usd_physics::core::Vector3{0, 0, value}}) {
      try {
        (void)usd_physics::jolt::createWorld({}, gravity);
        return 5;
      } catch (const std::invalid_argument &) {
      }
    }
  }
  if (usd_physics::jolt::backendAvailable()) {
    return usd_physics::jolt::createWorld() ? 0 : 2;
  }
  try {
    static_cast<void>(usd_physics::jolt::createWorld({}, {}));
  } catch (const usd_physics::core::PhysicsError& error) {
    return error.code() ==
                   usd_physics::core::PhysicsErrorCode::backendUnavailable
               ? 0
               : 3;
  }
  return 4;
}
