#pragma once

#include "usd_physics/usd/box_scene.h"
#include "usd_physics/core/six_dof.h"
#include <optional>

namespace usd_physics::usd {
// Caller resolves nonempty endpoint paths to handles in its world. Empty
// endpoint paths require an invalid (world) handle. Disabled joints return
// nullopt. Unsupported execution ranges throw ImportError with the joint path;
// snapshots retain their wider declaration subset. No world is mutated.
[[nodiscard]] std::optional<core::SixDofConstraintDescriptor>
toSixDofConstraint(const Joint& joint, core::BodyHandle body0,
                   core::BodyHandle body1);
} // namespace usd_physics::usd
