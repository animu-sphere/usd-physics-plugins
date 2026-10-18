#pragma once

#include "usd_physics/core/world.h"
#include <array>

namespace usd_physics::core {

enum class AxisMotion { free, locked, limited };

struct AxisLimit {
  AxisMotion motion{AxisMotion::free};
  double low{0}, high{0};
};

struct SixDofConstraintDescriptor {
  // One invalid handle denotes world; two invalid or identical handles reject.
  BodyHandle firstBody, secondBody;
  // Body-origin local frames, or world frame for the world endpoint.
  Transform firstFrame, secondFrame;
  // Translation X/Y/Z in meters, then twist X and swing Y/Z in radians.
  // Locked axes constrain to zero. Limited axes require finite low < high;
  // angular ranges lie strictly inside (-pi, pi). Springs/drives are absent.
  std::array<AxisLimit, 6> limits;
  bool collisionEnabled{false};
};

void validateSixDofConstraintDescriptor(const SixDofConstraintDescriptor& value);

// Optional capability. Destroy through PhysicsWorld::destroyConstraint().
class SixDofConstraints {
public:
  virtual ~SixDofConstraints() = default;
  virtual ConstraintHandle createSixDofConstraint(
      const SixDofConstraintDescriptor& descriptor) = 0;
};

} // namespace usd_physics::core
