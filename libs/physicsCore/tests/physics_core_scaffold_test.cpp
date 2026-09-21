#include "usd_physics/core/version.h"

int main() {
  const auto value = usd_physics::core::version();
  return value.major == 0 && value.minor == 1 && value.patch == 0 ? 0 : 1;
}

