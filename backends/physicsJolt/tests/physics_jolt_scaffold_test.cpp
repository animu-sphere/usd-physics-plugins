#include "usd_physics/core/version.h"
#include "usd_physics/jolt/availability.h"

int main() {
  const auto version = usd_physics::core::version();
  return version.major == 0 && !usd_physics::jolt::backendAvailable() ? 0 : 1;
}

