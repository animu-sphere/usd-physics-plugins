#include "usd_physics/core/descriptors.h"

#include <limits>
#include <stdexcept>
#include <utility>

namespace {

using namespace usd_physics::core;

template <typename Function> bool rejectsInvalidArgument(Function&& function) {
  try {
    std::forward<Function>(function)();
  } catch (const std::invalid_argument&) {
    return true;
  }
  return false;
}

} // namespace

int main() {
  const ShapeDescriptor defaultBox;
  if (defaultBox.type != ShapeType::box ||
      defaultBox.halfExtents != Vector3{0.5, 0.5, 0.5}) {
    return 1;
  }
  validateShapeDescriptor(defaultBox);

  const BodyDescriptor staticBody{ShapeHandle{1}, MotionType::staticBody,
                                  Transform{}, 0.0,
                                  {CollisionCategories{1} << 2, 0}};
  const BodyDescriptor dynamicBody{
      ShapeHandle{1}, MotionType::dynamicBody,
      Transform{{1.0, 2.0, 3.0}, {0.5, 0.5, 0.5, 0.5}}, 2.0,
      {CollisionCategories{1} << 3,
       (CollisionCategories{1} << 2) | (CollisionCategories{1} << 3)}};
  validateBodyDescriptor(staticBody);
  validateBodyDescriptor(dynamicBody);
  if (collisionFiltersAllowPair(staticBody.collisionFilter,
                                dynamicBody.collisionFilter) ||
      !collisionFiltersAllowPair(dynamicBody.collisionFilter,
                                 dynamicBody.collisionFilter) ||
      collisionFiltersAllowPair({}, {1, 0})) {
    return 2;
  }

  const ConstraintDescriptor fixedConstraint{ConstraintType::fixed, BodyHandle{1},
                                             BodyHandle{2}};
  validateConstraintDescriptor(fixedConstraint);

  const auto infinity = std::numeric_limits<double>::infinity();
  const auto nan = std::numeric_limits<double>::quiet_NaN();
  if (!rejectsInvalidArgument([] {
        validateShapeDescriptor(
            {static_cast<ShapeType>(-1), Vector3{0.5, 0.5, 0.5}});
      }) ||
      !rejectsInvalidArgument([] {
        validateShapeDescriptor({ShapeType::box, Vector3{0.0, 0.5, 0.5}});
      }) ||
      !rejectsInvalidArgument([&] {
        validateShapeDescriptor({ShapeType::box, Vector3{infinity, 0.5, 0.5}});
      }) ||
      !rejectsInvalidArgument([] { validateBodyDescriptor({}); }) ||
      !rejectsInvalidArgument([] {
        validateBodyDescriptor(
            {ShapeHandle{1}, static_cast<MotionType>(-1), {}, 1.0, {}});
      }) ||
      !rejectsInvalidArgument([&] {
        validateBodyDescriptor({ShapeHandle{1}, MotionType::staticBody,
                                Transform{{nan, 0.0, 0.0}, {}}, 0.0, {}});
      }) ||
      !rejectsInvalidArgument([&] {
        validateBodyDescriptor({ShapeHandle{1}, MotionType::staticBody,
                                Transform{{}, {1.0, 0.0, infinity, 0.0}}, 0.0, {}});
      }) ||
      !rejectsInvalidArgument([] {
        validateBodyDescriptor(
            {ShapeHandle{1}, MotionType::dynamicBody, {}, 0.0, {}});
      }) ||
      !rejectsInvalidArgument([] {
        validateBodyDescriptor(
            {ShapeHandle{1}, MotionType::staticBody, {}, -1.0, {}});
      }) ||
      !rejectsInvalidArgument([&] {
        validateBodyDescriptor(
            {ShapeHandle{1}, MotionType::dynamicBody, {}, nan, {}});
      }) ||
      !rejectsInvalidArgument([] {
        validateBodyDescriptor(
            {ShapeHandle{1}, MotionType::staticBody, {}, 0.0, {0, 0}});
      }) ||
      !rejectsInvalidArgument([] {
        validateConstraintDescriptor({ConstraintType::fixed, {}, BodyHandle{2}});
      }) ||
      !rejectsInvalidArgument([] {
        validateConstraintDescriptor(
            {ConstraintType::fixed, BodyHandle{1}, BodyHandle{1}});
      }) ||
      !rejectsInvalidArgument([] {
        validateConstraintDescriptor(
            {static_cast<ConstraintType>(-1), BodyHandle{1}, BodyHandle{2}});
      })) {
    return 3;
  }

  return 0;
}
