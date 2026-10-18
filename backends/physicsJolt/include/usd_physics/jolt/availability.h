#pragma once

#include "usd_physics/core/world.h"

#include <memory>

namespace usd_physics::jolt {

// Reports whether this package was built with its private Jolt dependency.
[[nodiscard]] bool backendAvailable() noexcept;

// Constructs a Jolt-backed world when the backend is available. A package
// built without Jolt throws PhysicsErrorCode::backendUnavailable here.
[[nodiscard]] std::unique_ptr<core::PhysicsWorld> createWorld();

// Select an immutable, solver-neutral pair policy for this world only.
[[nodiscard]] std::unique_ptr<core::PhysicsWorld>
createWorld(const core::ContactMixing &mixing);

} // namespace usd_physics::jolt
