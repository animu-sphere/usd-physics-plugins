#pragma once

#include "usd_physics/core/world.h"

#include <cstddef>
#include <cstdint>
#include <memory>

namespace usd_physics::jolt {

// How much one world may simulate per step. Exceeding bodies rejects creation;
// exceeding the others fails the step with PhysicsErrorCode::solverFailure. The
// defaults suit small scenes. Larger values cost memory for the world's lifetime.
struct WorldCapacity {
  std::uint32_t bodies = 1024;
  std::uint32_t bodyPairs = 1024;
  std::uint32_t contactConstraints = 1024;
  std::size_t temporaryBytes = 10 * 1024 * 1024;
};

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

// Select the capacity as well. A zero value rejects before backend availability
// is checked.
[[nodiscard]] std::unique_ptr<core::PhysicsWorld>
createWorld(const core::ContactMixing &mixing, core::Vector3 gravity,
            const WorldCapacity &capacity);

} // namespace usd_physics::jolt
