#pragma once

#include "usd_physics/core/handles.h"
#include "usd_physics/core/value_types.h"

#include <cstdint>
#include <optional>

namespace usd_physics::core {

enum class ShapeType {
  box,
  sphere,
  capsule,
};

struct ShapeDescriptor {
  ShapeType type{ShapeType::box};
  Vector3 halfExtents{0.5, 0.5, 0.5};
  // Geometry frame relative to the body origin. Scale is baked into dimensions.
  Transform localTransform;
  double radius{0.5};
  // Capsule cylinder half-height, excluding hemispheres; axis is local Y.
  // Zero represents a sphere with the same radius.
  double halfHeight{0.5};
};

enum class MotionType {
  staticBody,
  dynamicBody,
  kinematicBody,
};

using CollisionCategories = std::uint64_t;

struct CollisionFilter {
  CollisionCategories categories{1};
  CollisionCategories collidesWith{~CollisionCategories{0}};
};

[[nodiscard]] constexpr bool
collisionFiltersAllowPair(CollisionFilter first,
                          CollisionFilter second) noexcept {
  return (first.categories & second.collidesWith) != 0 &&
         (second.categories & first.collidesWith) != 0;
}

// Dimensionless body contact coefficients.
struct ContactMaterial {
  double friction{0.2};
  double restitution{0.0};
};

enum class ContactCombineRule {
  geometricMean,
  maximum,
  product,
};

// Immutable world-wide pair policy. Defaults preserve the original backend
// behavior; a source adapter may request products and a friction ceiling.
struct ContactMixing {
  ContactCombineRule friction{ContactCombineRule::geometricMean};
  ContactCombineRule restitution{ContactCombineRule::maximum};
  std::optional<double> maxFriction;
};

// Non-negative decay rates in inverse seconds: dv/dt = -linear * v,
// dw/dt = -angular * w. Backends may approximate the exponential per step.
struct BodyDamping {
  double linear{0.05};
  double angular{0.05};
};

struct BodyDescriptor {
  ShapeHandle shape;
  MotionType motionType{MotionType::staticBody};
  Transform initialTransform;
  double mass{1.0};
  CollisionFilter collisionFilter;
  ContactMaterial contactMaterial;
  BodyDamping damping;
};

enum class ConstraintType {
  fixed,
};

struct ConstraintDescriptor {
  ConstraintType type{ConstraintType::fixed};
  BodyHandle firstBody;
  BodyHandle secondBody;
};

void validateShapeDescriptor(const ShapeDescriptor &descriptor);
void validateContactMaterial(const ContactMaterial &material);
void validateContactMixing(const ContactMixing &mixing);
void validateBodyDamping(const BodyDamping &damping);
void validateBodyDescriptor(const BodyDescriptor &descriptor);
void validateConstraintDescriptor(const ConstraintDescriptor &descriptor);

} // namespace usd_physics::core
