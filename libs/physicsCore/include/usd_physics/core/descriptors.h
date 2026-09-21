#pragma once

#include "usd_physics/core/handles.h"
#include "usd_physics/core/value_types.h"

#include <cstdint>

namespace usd_physics::core {

enum class ShapeType {
  box,
};

struct ShapeDescriptor {
  ShapeType type{ShapeType::box};
  Vector3 halfExtents{0.5, 0.5, 0.5};
};

enum class MotionType {
  staticBody,
  dynamicBody,
};

using CollisionCategories = std::uint64_t;

struct CollisionFilter {
  CollisionCategories categories{1};
  CollisionCategories collidesWith{~CollisionCategories{0}};
};

[[nodiscard]] constexpr bool
collisionFiltersAllowPair(CollisionFilter first, CollisionFilter second) noexcept {
  return (first.categories & second.collidesWith) != 0 &&
         (second.categories & first.collidesWith) != 0;
}

struct BodyDescriptor {
  ShapeHandle shape;
  MotionType motionType{MotionType::staticBody};
  Transform initialTransform;
  double mass{1.0};
  CollisionFilter collisionFilter;
};

enum class ConstraintType {
  fixed,
};

struct ConstraintDescriptor {
  ConstraintType type{ConstraintType::fixed};
  BodyHandle firstBody;
  BodyHandle secondBody;
};

void validateShapeDescriptor(const ShapeDescriptor& descriptor);
void validateBodyDescriptor(const BodyDescriptor& descriptor);
void validateConstraintDescriptor(const ConstraintDescriptor& descriptor);

} // namespace usd_physics::core
