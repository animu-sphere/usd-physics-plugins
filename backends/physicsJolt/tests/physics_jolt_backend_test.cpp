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

} // namespace

int main() {
  using namespace usd_physics;

  if (!jolt::backendAvailable()) {
    return fail("the Jolt backend test requires an available backend");
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
      !world->destroyBody(wall) ||
      !world->destroyShape(wallShape)) {
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
