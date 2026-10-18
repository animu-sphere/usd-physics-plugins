#include "usd_physics/core/six_dof.h"
#include <cmath>
#include <stdexcept>

namespace usd_physics::core {
void validateSixDofConstraintDescriptor(const SixDofConstraintDescriptor& d) {
  if (d.firstBody == d.secondBody)
    throw std::invalid_argument("six-DOF requires distinct endpoints and at most one world anchor");
  for (const auto& frame : {d.firstFrame, d.secondFrame}) {
    validatePhysicsVector(frame.translation, "joint frame");
    const auto q = frame.rotation;
    if (!std::isfinite(q.w) || !std::isfinite(q.x) || !std::isfinite(q.y) ||
        !std::isfinite(q.z) || (q.w == 0 && q.x == 0 && q.y == 0 && q.z == 0))
      throw std::invalid_argument("joint rotation must be finite and nonzero");
  }
  constexpr double pi = 3.14159265358979323846;
  for (std::size_t axis = 0; axis < d.translationSprings.size(); ++axis) {
    const auto& spring = d.translationSprings[axis];
    if (!std::isfinite(spring.stiffness) || spring.stiffness < 0 ||
        !std::isfinite(spring.damping) || spring.damping < 0 ||
        !std::isfinite(spring.equilibrium) ||
        (spring.stiffness == 0 && spring.damping != 0) ||
        (spring.stiffness > 0 && d.limits[axis].motion == AxisMotion::locked))
      throw std::invalid_argument("invalid six-DOF translation spring");
  }
  for (std::size_t axis = 0; axis < d.limits.size(); ++axis) {
    const auto& limit = d.limits[axis];
    switch (limit.motion) {
    case AxisMotion::free:
    case AxisMotion::locked:
      break; // Inactive bounds are ignored.
    case AxisMotion::limited:
      if (!std::isfinite(limit.low) || !std::isfinite(limit.high) ||
          limit.low >= limit.high ||
          (axis >= 3 && (limit.low <= -pi || limit.high >= pi)))
        throw std::invalid_argument("invalid six-DOF limited axis range");
      break;
    default:
      throw std::invalid_argument("unsupported six-DOF axis motion");
    }
  }
}
} // namespace usd_physics::core
