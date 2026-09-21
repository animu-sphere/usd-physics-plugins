#include "usd_physics/core/handles.h"
#include "usd_physics/core/value_types.h"
#include "usd_physics/core/version.h"
#include "usd_physics/jolt/availability.h"

int main() {
  const auto version = usd_physics::core::version();
  if (version.major != 0 || version.minor != 1 || version.patch != 0) {
    return 1;
  }
  const usd_physics::core::BodyHandle body{1};
  const usd_physics::core::Transform transform{
      {1.0, 2.0, 3.0}, {1.0, 0.0, 0.0, 0.0}};
  if (!body || transform.translation.y != 2.0) {
    return 2;
  }
  return usd_physics::jolt::backendAvailable() ? 3 : 0;
}
