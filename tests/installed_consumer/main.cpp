#include "usd_physics/core/version.h"
#include "usd_physics/jolt/availability.h"

int main() {
  const auto version = usd_physics::core::version();
  if (version.major != 0 || version.minor != 1 || version.patch != 0) {
    return 1;
  }
  return usd_physics::jolt::backendAvailable() ? 2 : 0;
}
