#include "usd_physics/core/descriptors.h"
#include "usd_physics/core/errors.h"
#include "usd_physics/core/ground_query.h"
#include "usd_physics/core/handles.h"
#include "usd_physics/core/kinematic_motion.h"
#include "usd_physics/core/segment_query.h"
#include "usd_physics/core/six_dof.h"
#include "usd_physics/core/value_types.h"
#include "usd_physics/core/version.h"
#include "usd_physics/core/world.h"
#include "usd_physics/jolt/availability.h"

int main() {
  const auto version = usd_physics::core::version();
  if (version.major != 0 || version.minor != 1 || version.patch != 0) {
    return 1;
  }
  const usd_physics::core::BodyHandle body{1};
  const usd_physics::core::Transform transform{{1.0, 2.0, 3.0},
                                               {1.0, 0.0, 0.0, 0.0}};
  if (!body || transform.translation.y != 2.0) {
    return 2;
  }
  const usd_physics::core::ShapeDescriptor box;
  const usd_physics::core::ContactMaterial material{0.75, 0.5};
  usd_physics::core::validateContactMaterial(material);
  const usd_physics::core::BodyDamping damping{.7, .3};
  usd_physics::core::validateBodyDamping(damping);
  usd_physics::core::validateShapeDescriptor(box);
  usd_physics::core::validateBodyDescriptor(
      {usd_physics::core::ShapeHandle{1},
       usd_physics::core::MotionType::dynamicBody,
       transform,
       1.0,
       {},
       material,
       damping});
  usd_physics::core::validatePhysicsStep(
      usd_physics::core::PhysicsWorld::Duration{1.0 / 60.0});
  usd_physics::core::validateSegmentEndpoints({}, {0.0, 0.0, 1.0});
  usd_physics::core::validateSegmentHit({body, 0.5});
  usd_physics::core::validateGroundProbeDistance(0.1);
  usd_physics::core::validateGroundContact({body, {0.0, 1.0, 0.0}, 0.05});
  const usd_physics::core::BodyState state{body, transform, {}};
  const usd_physics::core::PhysicsError error{
      usd_physics::core::PhysicsErrorCode::solverFailure, "solver failed"};
  if (state.body != body ||
      error.code() != usd_physics::core::PhysicsErrorCode::solverFailure) {
    return 3;
  }
  if (usd_physics::jolt::backendAvailable()) {
    auto world = usd_physics::jolt::createWorld();
    auto *motion =
        dynamic_cast<usd_physics::core::KinematicMotion *>(world.get());
    if (!motion)
      return 4;
    auto *constraints =
        dynamic_cast<usd_physics::core::SixDofConstraints *>(world.get());
#ifdef TEST_SIX_DOF
    if (!constraints)
      return 9;
#else
    if (constraints)
      return 9;
#endif
    const auto shape = world->createShape({});
    const auto moving =
        world->createBody({shape,
                           usd_physics::core::MotionType::kinematicBody,
                           {},
                           0,
                           {},
                           material,
                           damping});
    const usd_physics::core::PhysicsWorld::Duration dt{.1};
    if (!motion->moveKinematic(moving, {{1, 2, 3}, {}}, dt))
      return 7;
    world->step(dt);
    const auto position = world->bodyState(moving).transform.translation;
#ifdef TEST_SIX_DOF
    const auto held = world->createBody(
        {shape, usd_physics::core::MotionType::dynamicBody, {{10, 5, 0}, {}}});
    usd_physics::core::SixDofConstraintDescriptor descriptor;
    descriptor.secondBody = held;
    descriptor.firstFrame.translation = {10, 5, 0};
    for (auto &axis : descriptor.limits)
      axis.motion = usd_physics::core::AxisMotion::locked;
    const auto constraint = constraints->createSixDofConstraint(descriptor);
    for (int i = 0; i != 60; ++i)
      world->step(usd_physics::core::PhysicsWorld::Duration{1.0 / 60});
    const auto heldY = world->bodyState(held).transform.translation.y;
    if (heldY < 4.999 || heldY > 5.001 || !world->destroyConstraint(constraint))
      return 10;
#endif
    return position.x > .999 && position.x < 1.001 && position.y > 1.999 &&
                   position.y < 2.001
               ? 0
               : 8;
  }
  try {
    static_cast<void>(usd_physics::jolt::createWorld());
  } catch (const usd_physics::core::PhysicsError &unavailable) {
    return unavailable.code() ==
                   usd_physics::core::PhysicsErrorCode::backendUnavailable
               ? 0
               : 5;
  }
  return 6;
}
