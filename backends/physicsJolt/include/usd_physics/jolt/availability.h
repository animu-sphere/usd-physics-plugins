#pragma once

namespace usd_physics::jolt {

// Phase 0 installs the package boundary, not a solver. This function remains
// false until the Phase 2 Jolt implementation is linked into this component.
[[nodiscard]] bool backendAvailable() noexcept;

} // namespace usd_physics::jolt
