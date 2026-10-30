#include "usd_physics/core/descriptors.h"

#include <cmath>
#include <initializer_list>
#include <stdexcept>

namespace usd_physics::core {
namespace {

bool isFinite(Vector3 value) noexcept {
  return std::isfinite(value.x) && std::isfinite(value.y) &&
         std::isfinite(value.z);
}

bool isFinite(Quaternion value) noexcept {
  return std::isfinite(value.w) && std::isfinite(value.x) &&
         std::isfinite(value.y) && std::isfinite(value.z);
}

void validateTransform(Transform value) {
  if (!isFinite(value.translation)) {
    throw std::invalid_argument("transform translation must be finite");
  }
  if (!isFinite(value.rotation)) {
    throw std::invalid_argument("transform rotation must be finite");
  }
}

} // namespace

void validateShapeDescriptor(const ShapeDescriptor &descriptor) {
  validateTransform(descriptor.localTransform);
  const auto q = descriptor.localTransform.rotation;
  if (q.w == 0 && q.x == 0 && q.y == 0 && q.z == 0) {
    throw std::invalid_argument("shape rotation must have non-zero magnitude");
  }
  if (descriptor.type == ShapeType::sphere ||
      descriptor.type == ShapeType::capsule) {
    if (!std::isfinite(descriptor.radius) || descriptor.radius <= 0.0) {
      throw std::invalid_argument("shape radius must be positive and finite");
    }
    if (descriptor.type == ShapeType::capsule &&
        (!std::isfinite(descriptor.halfHeight) ||
         descriptor.halfHeight < 0.0)) {
      throw std::invalid_argument(
          "capsule half height must be non-negative and finite");
    }
    return;
  }
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

void validateContactMaterial(const ContactMaterial &material) {
  if (!std::isfinite(material.friction) || material.friction < 0.0) {
    throw std::invalid_argument(
        "contact friction must be finite and non-negative");
  }
  if (!std::isfinite(material.restitution) || material.restitution < 0.0 ||
      material.restitution > 1.0) {
    throw std::invalid_argument(
        "contact restitution must be finite and in [0,1]");
  }
}

void validateContactMixing(const ContactMixing &mixing) {
  for (const auto rule : {mixing.friction, mixing.restitution}) {
    switch (rule) {
    case ContactCombineRule::geometricMean:
    case ContactCombineRule::maximum:
    case ContactCombineRule::product:
      break;
    default:
      throw std::invalid_argument("unsupported contact combine rule");
    }
  }
  if (mixing.maxFriction &&
      (!std::isfinite(*mixing.maxFriction) || *mixing.maxFriction < 0)) {
    throw std::invalid_argument(
        "contact friction ceiling must be finite and non-negative");
  }
}

void validateBodyDamping(const BodyDamping &damping) {
  if (!std::isfinite(damping.linear) || damping.linear < 0.0 ||
      !std::isfinite(damping.angular) || damping.angular < 0.0) {
    throw std::invalid_argument(
        "body damping rates must be finite and non-negative");
  }
  const auto validateChannel = [](BodyDampingMode mode, double rate) {
    switch (mode) {
    case BodyDampingMode::finiteRate:
      return;
    case BodyDampingMode::clearPriorVelocity:
      if (rate == 0)
        return;
      throw std::invalid_argument("prior-velocity clearing requires a zero rate");
    default:
      throw std::invalid_argument("unsupported body damping mode");
    }
  };
  validateChannel(damping.linearMode, damping.linear);
  validateChannel(damping.angularMode, damping.angular);
}

void validateBodyDescriptor(const BodyDescriptor &descriptor) {
  validateContactMaterial(descriptor.contactMaterial);
  validateBodyDamping(descriptor.damping);
  if (!descriptor.shape) {
    throw std::invalid_argument("a physics body requires a valid shape handle");
  }
  validateTransform(descriptor.initialTransform);

  switch (descriptor.motionType) {
  case MotionType::staticBody:
  case MotionType::dynamicBody:
  case MotionType::kinematicBody:
    break;
  default:
    throw std::invalid_argument("body motion type is unsupported");
  }

  if (!std::isfinite(descriptor.mass) ||
      (descriptor.motionType == MotionType::dynamicBody &&
       descriptor.mass <= 0.0) ||
      (descriptor.motionType != MotionType::dynamicBody &&
       descriptor.mass < 0.0)) {
    throw std::invalid_argument("body mass must be finite, non-negative for "
                                "static/kinematic bodies, and "
                                "positive for dynamic bodies");
  }
  if (descriptor.collisionFilter.categories == 0) {
    throw std::invalid_argument(
        "a physics body requires at least one collision category");
  }
}

void validateConstraintDescriptor(const ConstraintDescriptor &descriptor) {
  if (descriptor.type != ConstraintType::fixed) {
    throw std::invalid_argument("constraint type is unsupported");
  }
  if (!descriptor.firstBody || !descriptor.secondBody) {
    throw std::invalid_argument(
        "a physics constraint requires two valid body handles");
  }
  if (descriptor.firstBody == descriptor.secondBody) {
    throw std::invalid_argument(
        "a physics constraint requires two distinct bodies");
  }
}

} // namespace usd_physics::core
