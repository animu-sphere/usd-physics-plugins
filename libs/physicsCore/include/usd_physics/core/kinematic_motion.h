#pragma once

#include "usd_physics/core/world.h"

namespace usd_physics::core {

// Optional backend capability. Discover with dynamic_cast from PhysicsWorld.
class KinematicMotion {
public:
  virtual ~KinematicMotion() = default;

  // Sets velocities to reach the world-space BODY origin/rotation after the
  // supplied duration, without teleporting. Call before each matching step;
  // velocities persist until replaced (resubmit the current pose to hold).
  // Invalid values throw invalid_argument; unknown/stale/cross-world handles
  // and bodies of another motion type return false. Changes queue on step.
  virtual bool moveKinematic(BodyHandle body, Transform target,
                             PhysicsWorld::Duration duration) = 0;
};

void validateKinematicTarget(Transform target, PhysicsWorld::Duration duration);

} // namespace usd_physics::core
