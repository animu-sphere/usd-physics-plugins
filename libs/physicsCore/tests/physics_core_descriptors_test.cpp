#include "usd_physics/core/descriptors.h"
#include "usd_physics/core/kinematic_motion.h"

#include <limits>
#include <stdexcept>
#include <utility>

namespace {

using namespace usd_physics::core;

template <typename Function> bool rejectsInvalidArgument(Function &&function) {
  try {
    std::forward<Function>(function)();
  } catch (const std::invalid_argument &) {
    return true;
  }
  return false;
}

} // namespace

int main() {
  const ShapeDescriptor defaultBox;
  if (defaultBox.type != ShapeType::box ||
      defaultBox.halfExtents != Vector3{0.5, 0.5, 0.5} ||
      defaultBox.localTransform != Transform{}) {
    return 1;
  }
  validateShapeDescriptor(defaultBox);
  validateShapeDescriptor(
      {ShapeType::box, {1, 2, 3}, {{1, -2, 3}, {2, 0, 0, 0}}});

  const BodyDescriptor staticBody{ShapeHandle{1},
                                  MotionType::staticBody,
                                  Transform{},
                                  0.0,
                                  {CollisionCategories{1} << 2, 0}};
  const BodyDescriptor dynamicBody{
      ShapeHandle{1},
      MotionType::dynamicBody,
      Transform{{1.0, 2.0, 3.0}, {0.5, 0.5, 0.5, 0.5}},
      2.0,
      {CollisionCategories{1} << 3,
       (CollisionCategories{1} << 2) | (CollisionCategories{1} << 3)}};
  validateBodyDescriptor(staticBody);
  validateBodyDescriptor(dynamicBody);
  validateBodyDescriptor(
      {ShapeHandle{1}, MotionType::kinematicBody, {}, 0, {}});
  validateKinematicTarget({{1, 2, 3}, {2, 0, 0, 0}},
                          PhysicsWorld::Duration{.1});
  if (collisionFiltersAllowPair(staticBody.collisionFilter,
                                dynamicBody.collisionFilter) ||
      !collisionFiltersAllowPair(dynamicBody.collisionFilter,
                                 dynamicBody.collisionFilter) ||
      collisionFiltersAllowPair({}, {1, 0})) {
    return 2;
  }

  const ConstraintDescriptor fixedConstraint{ConstraintType::fixed,
                                             BodyHandle{1}, BodyHandle{2}};
  validateConstraintDescriptor(fixedConstraint);

  const auto infinity = std::numeric_limits<double>::infinity();
  const auto nan = std::numeric_limits<double>::quiet_NaN();
  const ContactMixing defaultMixing;
  if (defaultMixing.friction != ContactCombineRule::geometricMean ||
      defaultMixing.restitution != ContactCombineRule::maximum ||
      defaultMixing.maxFriction)
    return 14;
  for (const auto friction :
       {ContactCombineRule::geometricMean, ContactCombineRule::maximum,
        ContactCombineRule::product}) {
    for (const auto restitution :
         {ContactCombineRule::geometricMean, ContactCombineRule::maximum,
          ContactCombineRule::product}) {
      validateContactMixing({friction, restitution, {}});
      validateContactMixing({friction, restitution, 0});
      validateContactMixing({friction, restitution, 10});
    }
  }
  for (const auto invalid : {-1.0, infinity, nan}) {
    auto mixing = defaultMixing;
    mixing.maxFriction = invalid;
    if (!rejectsInvalidArgument([&] { validateContactMixing(mixing); }))
      return 15;
  }
  auto mixing = defaultMixing;
  mixing.friction = static_cast<ContactCombineRule>(-1);
  if (!rejectsInvalidArgument([&] { validateContactMixing(mixing); }))
    return 16;
  mixing = defaultMixing;
  mixing.restitution = static_cast<ContactCombineRule>(-1);
  if (!rejectsInvalidArgument([&] { validateContactMixing(mixing); }))
    return 16;
  if (BodyDamping{}.linear != .05 || BodyDamping{}.angular != .05 ||
      BodyDamping{}.linearMode != BodyDampingMode::finiteRate ||
      BodyDamping{}.angularMode != BodyDampingMode::finiteRate)
    return 12;
  for (const auto type : {MotionType::staticBody, MotionType::dynamicBody,
                          MotionType::kinematicBody}) {
    auto body = dynamicBody;
    body.motionType = type;
    for (const auto damping : {BodyDamping{0, 0}, BodyDamping{2, 4}}) {
      body.damping = damping;
      validateBodyDamping(damping);
      validateBodyDescriptor(body);
    }
    for (const auto linearMode : {BodyDampingMode::finiteRate,
                                  BodyDampingMode::clearPriorVelocity}) {
      for (const auto angularMode : {BodyDampingMode::finiteRate,
                                     BodyDampingMode::clearPriorVelocity}) {
        body.damping = {0, 0, linearMode, angularMode};
        validateBodyDescriptor(body);
      }
    }
    for (const auto damping : {
             BodyDamping{1, 0, BodyDampingMode::clearPriorVelocity},
             BodyDamping{0, 1, BodyDampingMode::finiteRate,
                         BodyDampingMode::clearPriorVelocity},
             BodyDamping{0, 0, static_cast<BodyDampingMode>(-1)},
             BodyDamping{0, 0, BodyDampingMode::finiteRate,
                         static_cast<BodyDampingMode>(-1)}}) {
      body.damping = damping;
      if (!rejectsInvalidArgument([&] { validateBodyDamping(damping); }) ||
          !rejectsInvalidArgument([&] { validateBodyDescriptor(body); }))
        return 17;
    }
    for (const double invalid : {-1.0, nan, infinity}) {
      for (const auto damping :
           {BodyDamping{invalid, 0}, BodyDamping{0, invalid}}) {
        body.damping = damping;
        if (!rejectsInvalidArgument([&] { validateBodyDamping(damping); }) ||
            !rejectsInvalidArgument([&] { validateBodyDescriptor(body); }))
          return 13;
      }
    }
  }
  if (ContactMaterial{}.friction != 0.2 || ContactMaterial{}.restitution != 0)
    return 9;
  for (const double friction : {0.0, 0.2, 1.0, 4.0}) {
    for (const double restitution : {0.0, 0.5, 1.0}) {
      auto body = dynamicBody;
      body.contactMaterial = {friction, restitution};
      validateBodyDescriptor(body);
    }
  }
  for (const double invalid : {-1.0, nan, infinity}) {
    if (!rejectsInvalidArgument(
            [&] { validateContactMaterial({invalid, 0}); }) ||
        !rejectsInvalidArgument([&] { validateContactMaterial({0, invalid}); }))
      return 10;
  }
  auto invalidMaterialBody = staticBody;
  invalidMaterialBody.contactMaterial.restitution = 1.01;
  if (!rejectsInvalidArgument(
          [&] { validateBodyDescriptor(invalidMaterialBody); }))
    return 11;
  for (const double mass : {-1.0, nan, infinity}) {
    if (!rejectsInvalidArgument([&] {
          validateBodyDescriptor(
              {ShapeHandle{1}, MotionType::kinematicBody, {}, mass, {}});
        }))
      return 7;
  }
  for (const double seconds : {0.0, -1.0, nan, infinity}) {
    if (!rejectsInvalidArgument([&] {
          validateKinematicTarget({}, PhysicsWorld::Duration{seconds});
        }))
      return 8;
  }
  for (const auto type : {ShapeType::sphere, ShapeType::capsule}) {
    ShapeDescriptor shape;
    shape.type = type;
    shape.halfHeight = 0;
    validateShapeDescriptor(shape);
    for (const double radius : {0.0, -1.0, infinity, nan}) {
      shape.radius = radius;
      if (!rejectsInvalidArgument([&] { validateShapeDescriptor(shape); }))
        return 5;
    }
    shape.radius = 1;
    if (type == ShapeType::capsule) {
      for (const double height : {-1.0, infinity, nan}) {
        shape.halfHeight = height;
        if (!rejectsInvalidArgument([&] { validateShapeDescriptor(shape); }))
          return 6;
      }
    }
  }
  for (const auto &transform :
       {Transform{{nan, 0, 0}, {}}, Transform{{}, {1, infinity, 0, 0}},
        Transform{{}, {0, 0, 0, 0}}}) {
    if (!rejectsInvalidArgument([&] {
          validateKinematicTarget(transform, PhysicsWorld::Duration{.1});
        }) ||
        !rejectsInvalidArgument([&] {
          validateShapeDescriptor({ShapeType::box, {1, 1, 1}, transform});
        })) {
      return 4;
    }
  }
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
        validateBodyDescriptor({ShapeHandle{1},
                                MotionType::staticBody,
                                Transform{{nan, 0.0, 0.0}, {}},
                                0.0,
                                {}});
      }) ||
      !rejectsInvalidArgument([&] {
        validateBodyDescriptor({ShapeHandle{1},
                                MotionType::staticBody,
                                Transform{{}, {1.0, 0.0, infinity, 0.0}},
                                0.0,
                                {}});
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
        validateConstraintDescriptor(
            {ConstraintType::fixed, {}, BodyHandle{2}});
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
