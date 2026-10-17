#include "usd_physics/core/ground_query.h"
#include "usd_physics/core/segment_query.h"
#include "usd_physics/jolt/availability.h"

#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>

namespace {

int fail(const char* message) {
  std::cerr << message << '\n';
  return 1;
}

bool checkLocalShapeFrame() {
  using namespace usd_physics;
  auto world = jolt::createWorld();
  auto *query = dynamic_cast<core::SegmentQuery *>(world.get());
  const double s = std::sqrt(0.5);
  const auto shape = world->createShape({core::ShapeType::box,
                                         {1, 0.25, 0.5},
                                         {{2, 0, 0}, {2 * s, 0, 0, 2 * s}}});
  const auto body = world->createBody(
      {shape, core::MotionType::staticBody, {{10, 3, 0}, {s, 0, 0, s}}, 0, {}});
  const auto state = world->bodyState(body);
  const auto hit = query->segmentHit({8, 5, 0}, {12, 5, 0});
  if (!hit || hit->body != body || std::abs(hit->fraction - 0.25) > 0.001 ||
      query->segmentHit({8, 3, 0}, {12, 3, 0}) ||
      std::abs(state.transform.translation.x - 10) > 0.001 ||
      std::abs(state.transform.translation.y - 3) > 0.001 ||
      std::abs(state.transform.rotation.z - s) > 0.001) {
    return false;
  }
  // The offset shape's center of mass differs from the reported body origin.
  // Contact and changed-state reporting must preserve that distinction.
  const auto floorShape =
      world->createShape({core::ShapeType::box, {5, 0.5, 5}});
  world->createBody(
      {floorShape, core::MotionType::staticBody, {{20, -0.5, 0}, {}}, 0, {}});
  const auto falling = world->createBody({shape,
                                          core::MotionType::dynamicBody,
                                          {{20, 3, 0}, {s, 0, 0, s}},
                                          2,
                                          {}});
  for (int i = 0; i < 240; ++i)
    world->step(core::PhysicsWorld::Duration{1.0 / 60.0});
  const auto settled = world->bodyState(falling);
  const auto changes = world->takeChangedBodyStates();
  return std::abs(settled.transform.translation.y + 1.75) < 0.03 &&
         std::abs(settled.transform.rotation.z - s) < 0.01 &&
         changes.size() == 1 && changes[0].body == falling &&
         changes[0].transform == settled.transform;
}

} // namespace

int main() {
  using namespace usd_physics;

  if (!jolt::backendAvailable()) {
    return fail("the Jolt backend test requires an available backend");
  }
  if (!checkLocalShapeFrame()) {
    return fail("local collider pose must preserve geometry and body state");
  }

  // Query the distinct axial/radial extents and settle rounded shapes with
  // displaced geometry. State continues to report the body origin.
  for (const auto type : {core::ShapeType::sphere, core::ShapeType::capsule}) {
    for (const double halfHeight : {0.0, 1.0}) {
      auto roundedWorld = jolt::createWorld();
      auto* query = dynamic_cast<core::SegmentQuery*>(roundedWorld.get());
      core::ShapeDescriptor descriptor;
      descriptor.type = type;
      descriptor.radius = 0.5;
      descriptor.halfHeight = halfHeight;
      descriptor.localTransform.translation = {2, 0, 0};
      const auto shape = roundedWorld->createShape(descriptor);
      // Backend precision failures reject before allocating native geometry.
      for (const double invalid : {std::numeric_limits<double>::max(),
                                   std::numeric_limits<double>::min()}) {
        auto bad = descriptor;
        bad.radius = invalid;
        try {
          roundedWorld->createShape(bad);
          return fail("rounded radius outside backend scalar range accepted");
        } catch (const std::invalid_argument&) {
        }
      }
      const auto target = roundedWorld->createBody(
          {shape, core::MotionType::staticBody, {{0, 3, 0}, {}}, 0, {}});
      const double extent =
          0.5 + (type == core::ShapeType::capsule ? halfHeight : 0);
      const auto axial = query->segmentHit({2, 6, 0}, {2, 0, 0});
      const auto radial = query->segmentHit({0, 3, 0}, {4, 3, 0});
      if (!axial || axial->body != target || !radial ||
          radial->body != target ||
          std::abs(axial->fraction - (3 - extent) / 6) > 0.001 ||
          std::abs(radial->fraction - 0.375) > 0.001 ||
          query->segmentHit({0, 6, 0}, {0, 0, 0}))
        return fail("rounded geometry query extents");
      roundedWorld->destroyBody(target);
      if (type == core::ShapeType::capsule && halfHeight > 0) {
        auto rotated = descriptor;
        const double s = std::sqrt(0.5);
        rotated.localTransform.rotation = {s, 0, 0, s};
        const auto rotatedShape = roundedWorld->createShape(rotated);
        const auto rotatedBody = roundedWorld->createBody(
            {rotatedShape, core::MotionType::staticBody, {{0, 3, 0}, {}}, 0, {}});
        const auto hit = query->segmentHit({0, 3, 0}, {4, 3, 0});
        if (!hit || hit->body != rotatedBody ||
            std::abs(hit->fraction - .125) > .001)
          return fail("capsule local rotation must rotate its spine");
        roundedWorld->destroyBody(rotatedBody);
        roundedWorld->destroyShape(rotatedShape);
      }
      const auto floor =
          roundedWorld->createShape({core::ShapeType::box, {5, .5, 5}});
      roundedWorld->createBody(
          {floor, core::MotionType::staticBody, {{0, -.5, 0}, {}}, 0, {}});
      const auto falling = roundedWorld->createBody(
          {shape, core::MotionType::dynamicBody, {{0, 3, 0}, {}}, 2, {}});
      for (int i = 0; i < 240; ++i)
        roundedWorld->step(core::PhysicsWorld::Duration{1.0 / 60});
      if (std::abs(roundedWorld->bodyState(falling).transform.translation.y -
                   extent) > .04 ||
          roundedWorld->takeChangedBodyStates().size() != 1)
        return fail("rounded shapes settle and report body state");
    }
  }

  auto world = jolt::createWorld();
  auto* groundQuery = dynamic_cast<core::GroundQuery*>(world.get());
  auto* segmentQuery = dynamic_cast<core::SegmentQuery*>(world.get());
  if (groundQuery == nullptr || segmentQuery == nullptr) {
    return fail("the Jolt world must expose ground and segment queries");
  }

  const auto floorShape =
      world->createShape({core::ShapeType::box, {5.0, 0.5, 5.0}});
  const auto cubeShape =
      world->createShape({core::ShapeType::box, {0.5, 0.5, 0.5}});
  const auto wallShape =
      world->createShape({core::ShapeType::box, {0.5, 2.0, 0.5}});
  const auto supportShape =
      world->createShape({core::ShapeType::box, {2.0, 0.1, 2.0}});

  const auto floor = world->createBody(
      {floorShape, core::MotionType::staticBody,
       core::Transform{{0.0, -0.5, 0.0}, {}}, 0.0, {1, ~std::uint64_t{0}}});
  const auto wall = world->createBody(
      {wallShape, core::MotionType::staticBody,
       core::Transform{{2.9, 2.0, 0.0}, {}}, 0.0, {1, ~std::uint64_t{0}}});
  const auto probe = world->createBody(
      {cubeShape, core::MotionType::dynamicBody,
       core::Transform{{2.0, 0.65, 0.0}, {}}, 1.0,
       {1, ~std::uint64_t{0}}});

  const auto elevatedContact = groundQuery->groundContact(probe, 0.2);
  if (!elevatedContact || elevatedContact->supportBody != floor ||
      std::abs(elevatedContact->distance - 0.15) > 0.01 ||
      elevatedContact->normal.y < 0.99) {
    return fail("the ground query must select the closest upward support");
  }
  if (groundQuery->groundContact(probe, 0.1)) {
    return fail("the ground query must honor the requested probe distance");
  }
  const auto filteredProbe = world->createBody(
      {cubeShape, core::MotionType::dynamicBody,
       core::Transform{{-2.0, 0.65, 0.0}, {}}, 1.0, {2, 2}});
  if (groundQuery->groundContact(filteredProbe, 0.2)) {
    return fail("the ground query must honor the target collision filter");
  }
  const auto lowerSupport = world->createBody(
      {supportShape, core::MotionType::staticBody,
       core::Transform{{10.0, -0.4, 0.0}, {}}, 0.0, {}});
  const auto slopedSupport = world->createBody(
      {supportShape, core::MotionType::staticBody,
       core::Transform{{10.0, 0.0, 0.0},
                       {0.965925826, 0.0, 0.0, 0.258819045}},
       0.0, {}});
  const auto nearestProbe = world->createBody(
      {cubeShape, core::MotionType::dynamicBody,
       core::Transform{{10.0, 1.0, 0.0}, {}}, 1.0, {}});
  const auto nearestContact = groundQuery->groundContact(nearestProbe, 1.0);
  if (!nearestContact || nearestContact->supportBody != slopedSupport) {
    return fail("the ground query must prefer the nearest upward support");
  }

  try {
    static_cast<void>(world->createBody(
        {cubeShape, core::MotionType::dynamicBody, {},
         std::numeric_limits<double>::denorm_min(), {}}));
    return fail("dynamic body mass must remain representable in Jolt");
  } catch (const std::invalid_argument&) {
  }

  const auto wallHit =
      segmentQuery->segmentHit({2.0, 2.0, 0.0}, {4.0, 2.0, 0.0});
  if (!wallHit || wallHit->body != wall ||
      std::abs(wallHit->fraction - 0.2) > 0.01) {
    return fail("the segment query must return the closest hit");
  }
  if (segmentQuery->segmentHit({2.0, 2.0, 0.0}, {4.0, 2.0, 0.0},
                               wall)) {
    return fail("the segment query must ignore the requested body");
  }

  try {
    static_cast<void>(groundQuery->groundContact(
        probe, std::numeric_limits<double>::quiet_NaN()));
    return fail("the ground query must reject invalid distances");
  } catch (const std::invalid_argument&) {
  }

  auto otherWorld = jolt::createWorld();
  const auto otherShape =
      otherWorld->createShape({core::ShapeType::box, {0.5, 0.5, 0.5}});
  const auto otherBody = otherWorld->createBody(
      {otherShape, core::MotionType::dynamicBody,
       core::Transform{{}, {2.0, 0.0, 0.0, 0.0}}, 1.0, {}});
  const auto normalizedState = otherWorld->bodyState(otherBody);
  if (std::abs(normalizedState.transform.rotation.w - 1.0) > 0.001 ||
      std::abs(normalizedState.transform.rotation.x) > 0.001 ||
      std::abs(normalizedState.transform.rotation.y) > 0.001 ||
      std::abs(normalizedState.transform.rotation.z) > 0.001) {
    return fail("body creation must normalize admitted rotations");
  }
  try {
    static_cast<void>(world->createBody(
        {otherShape, core::MotionType::dynamicBody, {}, 1.0, {}}));
    return fail("body creation must reject a cross-world shape handle");
  } catch (const std::invalid_argument&) {
  }
  if (world->destroyBody(otherBody) || world->setLinearVelocity(otherBody, {})) {
    return fail("commands must reject cross-world handles");
  }
  try {
    static_cast<void>(world->bodyState(otherBody));
    return fail("state lookup must reject cross-world handles");
  } catch (const std::out_of_range&) {
  }
  try {
    static_cast<void>(segmentQuery->segmentHit(
        {2.0, 2.0, 0.0}, {4.0, 2.0, 0.0}, otherBody));
    return fail("queries must reject cross-world handles");
  } catch (const std::out_of_range&) {
  }

  if (!world->destroyBody(probe) || !world->destroyBody(filteredProbe) ||
      !world->destroyBody(nearestProbe) || !world->destroyBody(slopedSupport) ||
      !world->destroyBody(lowerSupport) || !world->destroyBody(wall) ||
      !world->destroyShape(supportShape) || !world->destroyShape(wallShape)) {
    return fail("queried resources must retain explicit lifetime");
  }

  const auto cube = world->createBody(
      {cubeShape, core::MotionType::dynamicBody,
       core::Transform{{0.0, 3.0, 0.0}, {}}, 1.0, {}});
  const auto filteredCube = world->createBody(
      {cubeShape, core::MotionType::dynamicBody,
       core::Transform{{3.0, 3.0, 0.0}, {}}, 1.0, {2, 2}});

  if (!world->setLinearVelocity(cube, {0.0, -1.0, 0.0}) ||
      !world->applyForce(cube, {0.0, -1.0, 0.0}) ||
      world->setLinearVelocity(floor, {}) || world->applyForce(floor, {})) {
    return fail("velocity and force commands must admit only dynamic bodies");
  }
  try {
    static_cast<void>(world->applyForce(
        cube, {std::numeric_limits<double>::quiet_NaN(), 0.0, 0.0}));
    return fail("force commands must validate finite vectors");
  } catch (const std::invalid_argument&) {
  }
  try {
    static_cast<void>(world->createConstraint(
        {core::ConstraintType::fixed, floor, otherBody}));
    return fail("constraint creation must reject cross-world body handles");
  } catch (const std::invalid_argument&) {
  }

  for (int step = 0; step < 240; ++step) {
    world->step(core::PhysicsWorld::Duration{1.0 / 60.0});
  }

  const auto settled = world->bodyState(cube);
  if (std::abs(settled.transform.translation.y - 0.5) > 0.03 ||
      std::abs(settled.linearVelocity.y) > 0.05) {
    return fail("an admitted dynamic body must settle on the static floor");
  }
  const auto fallenThrough = world->bodyState(filteredCube);
  if (fallenThrough.transform.translation.y > -1.0) {
    return fail("semantic collision filters must reject disallowed pairs");
  }

  const auto settledContact = groundQuery->groundContact(cube, 0.1);
  if (!settledContact || settledContact->supportBody != floor ||
      settledContact->distance > 0.02 || settledContact->normal.y < 0.99) {
    return fail("a settled body must report its supporting body");
  }

  const auto changed = world->takeChangedBodyStates();
  if (changed.size() != 2 || changed[0].body.value() >= changed[1].body.value()) {
    return fail("changed state must be unique and ordered by handle");
  }
  if (!world->takeChangedBodyStates().empty()) {
    return fail("taking changed state must drain the queue");
  }

  const auto fixed = world->createConstraint(
      {core::ConstraintType::fixed, floor, cube});
  if (!fixed || !world->destroyBody(cube) || world->destroyConstraint(fixed)) {
    return fail("destroying a body must destroy dependent constraints");
  }
  try {
    static_cast<void>(world->bodyState(cube));
    return fail("state lookup must reject stale body handles");
  } catch (const std::out_of_range&) {
  }
  if (!world->destroyBody(filteredCube) || !world->destroyBody(floor) ||
      !world->destroyShape(cubeShape) || !world->destroyShape(floorShape)) {
    return fail("Jolt resources must have explicit handle-based lifetime");
  }
  if (world->destroyBody(cube) || world->destroyShape(cubeShape)) {
    return fail("repeated destruction must report a safe no-op");
  }

  return 0;
}
