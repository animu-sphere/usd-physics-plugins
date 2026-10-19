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

// Select immutable world-space acceleration in meters/second^2 as well as the
// pair policy. Zero disables gravity; existing factories use {0, -9.81, 0}.
// Nonfinite values reject before backend availability is checked. An available
// backend also rejects scalar overflow and nonzero components rounded to zero.
[[nodiscard]] std::unique_ptr<core::PhysicsWorld>
createWorld(const core::ContactMixing &mixing, core::Vector3 gravity);

} // namespace usd_physics::jolt
