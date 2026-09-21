#include "usd_physics/core/descriptors.h"

#include <cmath>
#include <stdexcept>

namespace usd_physics::core {
namespace {

bool isFinite(Vector3 value) noexcept {
  return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
}

bool isFinite(Quaternion value) noexcept {
  return std::isfinite(value.w) && std::isfinite(value.x) &&
         std::isfinite(value.y) && std::isfinite(value.z);
}

void validateTransform(Transform value) {
  if (!isFinite(value.translation)) {
    throw std::invalid_argument("body translation must be finite");
  }
  if (!isFinite(value.rotation)) {
    throw std::invalid_argument("body rotation must be finite");
  }
}

} // namespace

void validateShapeDescriptor(const ShapeDescriptor& descriptor) {
  if (descriptor.type != ShapeType::box) {
    throw std::invalid_argument("shape type is unsupported");
  }
  if (!isFinite(descriptor.halfExtents)) {
    throw std::invalid_argument("box half extents must be finite");
  }
  if (descriptor.halfExtents.x <= 0.0 || descriptor.halfExtents.y <= 0.0 ||
      descriptor.halfExtents.z <= 0.0) {
    throw std::invalid_argument("box half extents must be positive");
  }
}

void validateBodyDescriptor(const BodyDescriptor& descriptor) {
  if (!descriptor.shape) {
    throw std::invalid_argument("a physics body requires a valid shape handle");
  }
  validateTransform(descriptor.initialTransform);

  switch (descriptor.motionType) {
  case MotionType::staticBody:
  case MotionType::dynamicBody:
    break;
  default:
    throw std::invalid_argument("body motion type is unsupported");
  }

  if (!std::isfinite(descriptor.mass) ||
      (descriptor.motionType == MotionType::dynamicBody && descriptor.mass <= 0.0) ||
      (descriptor.motionType == MotionType::staticBody && descriptor.mass < 0.0)) {
    throw std::invalid_argument(
        "body mass must be finite, non-negative for static bodies, and positive for dynamic bodies");
  }
  if (descriptor.collisionFilter.categories == 0) {
    throw std::invalid_argument("a physics body requires at least one collision category");
  }
}

void validateConstraintDescriptor(const ConstraintDescriptor& descriptor) {
  if (descriptor.type != ConstraintType::fixed) {
    throw std::invalid_argument("constraint type is unsupported");
  }
  if (!descriptor.firstBody || !descriptor.secondBody) {
    throw std::invalid_argument("a physics constraint requires two valid body handles");
  }
  if (descriptor.firstBody == descriptor.secondBody) {
    throw std::invalid_argument("a physics constraint requires two distinct bodies");
  }
}

} // namespace usd_physics::core
