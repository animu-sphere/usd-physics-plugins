#include "usd_physics/core/version.h"

namespace usd_physics::core {

Version version() noexcept {
  return {USDPHYSICS_CORE_VERSION_MAJOR, USDPHYSICS_CORE_VERSION_MINOR,
          USDPHYSICS_CORE_VERSION_PATCH};
}

} // namespace usd_physics::core
