#pragma once

#include "usd_physics/core/world.h"
#include <array>

namespace usd_physics::core {

enum class AxisMotion { free, locked, limited };

struct AxisLimit {
  AxisMotion motion{AxisMotion::free};
  double low{0}, high{0};
};

struct TranslationSpring {
  // Force = -stiffness * (position - equilibrium) - damping * velocity,
  // in the first joint frame. Units: N/m, N*s/m, meters.
  // Zero stiffness disables the spring and requires zero damping.
  double stiffness{0}, damping{0}, equilibrium{0};
};

struct RotationSpring {
  // Small-angle rotational restoring stiffness (N*m/rad) and viscous damping
  // (N*m*s/rad), about the second joint frame's axes. Identity equilibrium.
  // Finite-angle response is a quaternion motor approximation, not Euler
  // springs. Zero stiffness disables an axis and requires zero damping.
  double stiffness{0}, damping{0};
};

struct SixDofConstraintDescriptor {
  // One invalid handle denotes world; two invalid or identical handles reject.
  BodyHandle firstBody, secondBody;
  // Body-origin local frames, or world frame for the world endpoint.
  Transform firstFrame, secondFrame;
  // Translation X/Y/Z in meters, then twist X and swing Y/Z in radians.
  // Locked axes constrain to zero. Limited axes require finite low < high;
  // angular ranges lie strictly inside (-pi, pi).
  std::array<AxisLimit, 6> limits;
  bool collisionEnabled{false};
  // X/Y/Z springs may coexist with free or limited translation axes.
  // Active springs on locked axes reject.
  std::array<TranslationSpring, 3> translationSprings;
  std::array<RotationSpring, 3> rotationSprings;
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
