#pragma once

#include "usd_physics/core/world.h"

#include <memory>

namespace usd_physics::jolt {

// Reports whether this package was built with its private Jolt dependency.
[[nodiscard]] bool backendAvailable() noexcept;

// Constructs a Jolt-backed world when the backend is available. A package
// built without Jolt throws PhysicsErrorCode::backendUnavailable here.
[[nodiscard]] std::unique_ptr<core::PhysicsWorld> createWorld();

} // namespace usd_physics::jolt
