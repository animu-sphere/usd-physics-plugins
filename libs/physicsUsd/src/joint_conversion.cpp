#include "usd_physics/usd/joint_conversion.h"
#include <cmath>
#include <limits>

namespace usd_physics::usd {
std::optional<core::SixDofConstraintDescriptor>
toSixDofConstraint(const Joint& joint, core::BodyHandle body0,
                   core::BodyHandle body1) {
  if (!joint.enabled) return std::nullopt;
  const auto fail = [&](const char* message) {
    throw ImportError(ImportErrorCode::unsupportedDeclaration, joint.path, message);
  };
  if (joint.body0.empty() != !body0 || joint.body1.empty() != !body1)
    fail("joint endpoint path/handle mismatch");
  core::SixDofConstraintDescriptor result;
  result.firstBody = body0;
  result.secondBody = body1;
  result.firstFrame = joint.localFrame0;
  result.secondFrame = joint.localFrame1;
  result.collisionEnabled = joint.collisionEnabled;
  if (joint.type != JointType::fixed && joint.type != JointType::sixDof)
    fail("unsupported joint type");
  for (std::size_t axis = 0; axis < result.limits.size(); ++axis) {
    auto& target = result.limits[axis];
    const auto& source = joint.limits[axis];
    if (joint.type == JointType::fixed) target.motion = core::AxisMotion::locked;
    else if (source.applied) {
      if (std::isnan(source.low) || std::isnan(source.high))
        fail("NaN joint limit");
      if (source.low > source.high || (source.low == 0 && source.high == 0))
        target.motion = core::AxisMotion::locked;
      else if (source.low == -std::numeric_limits<double>::infinity() &&
               source.high == std::numeric_limits<double>::infinity()) {}
      else {
        target = {core::AxisMotion::limited, source.low, source.high};
      }
    }
  }
  try { core::validateSixDofConstraintDescriptor(result); }
  catch (const std::invalid_argument&) {
    fail("joint execution requires valid frames and endpoints, finite ordered ranges, "
         "angular bounds inside (-pi, pi), and zero-valued locks");
  }
  return result;
}
} // namespace usd_physics::usd
