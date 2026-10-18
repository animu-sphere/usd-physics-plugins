#include "usd_physics/core/kinematic_motion.h"
#include "usd_physics/core/segment_query.h"
#include "usd_physics/jolt/availability.h"

#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>

using namespace usd_physics;

void check(bool condition, const char* detail) {
  if (!condition) throw std::runtime_error(detail);
}

int main() {
  try {
    auto world = jolt::createWorld();
    auto* motion = dynamic_cast<core::KinematicMotion*>(world.get());
    auto* query = dynamic_cast<core::SegmentQuery*>(world.get());
    check(motion && query, "kinematic and query capabilities");
    const core::PhysicsWorld::Duration dt{1.0 / 60};
    const double s = std::sqrt(.5);
    const auto shape = world->createShape(
        {core::ShapeType::box, {.5, .5, .5}, {{2, 0, 0}, {}}});
    const auto body = world->createBody(
        {shape, core::MotionType::kinematicBody, {{0, 3, 0}, {}}, 0, {}});
    // Idle bodies do not fall; force/velocity commands remain dynamic-only.
    world->step(dt);
    check(world->bodyState(body).transform.translation.y == 3 &&
              world->takeChangedBodyStates().empty() &&
              !world->applyForce(body, {0, -100, 0}) &&
              !world->setLinearVelocity(body, {1, 0, 0}), "kinematic idle/commands");
    const core::Transform target{{1, 4, 0}, {2 * s, 0, 0, 2 * s}};
    check(motion->moveKinematic(body, target, dt), "target admitted");
    check(world->bodyState(body).transform.translation == core::Vector3{0, 3, 0} &&
              world->takeChangedBodyStates().empty(), "command must not teleport or queue");
    world->step(dt);
    const auto state = world->bodyState(body);
    check(std::abs(state.transform.translation.x - 1) < .001 &&
              std::abs(state.transform.translation.y - 4) < .001 &&
              std::abs(state.transform.rotation.z - s) < .001,
          "offset collider target uses body origin and normalized orientation");
    const auto changes = world->takeChangedBodyStates();
    check(changes.size() == 1 && changes[0].body == body &&
              changes[0].transform == state.transform &&
              world->takeChangedBodyStates().empty(), "step queues and drains changes");
    const auto hit = query->segmentHit({-1, 6, 0}, {3, 6, 0});
    check(hit && hit->body == body && std::abs(hit->fraction - .375) < .001,
          "queries follow transformed offset geometry");
    // Holding requires another command; no implicit playback state or clock.
    check(motion->moveKinematic(body, state.transform, dt), "hold target");
    world->step(dt);
    check(std::abs(world->bodyState(body).transform.translation.y - 4) < .001,
          "resubmitted pose holds against gravity");
    world->takeChangedBodyStates();
    const auto unchanged = world->bodyState(body);
    const double nan = std::numeric_limits<double>::quiet_NaN();
    for (const auto pose : {core::Transform{{nan, 0, 0}, {}},
                            core::Transform{{1e300, 0, 0}, {}},
                            core::Transform{{}, {0, 0, 0, 0}},
                            core::Transform{{}, {1, nan, 0, 0}}}) {
      try {
        motion->moveKinematic(body, pose, dt);
        throw std::runtime_error("invalid pose accepted");
      } catch (const std::invalid_argument&) {}
    }
    for (const double seconds : {0.0, -1.0, nan,
                                  std::numeric_limits<double>::infinity(),
                                  std::numeric_limits<double>::min()}) {
      try {
        motion->moveKinematic(body, {}, core::PhysicsWorld::Duration{seconds});
        throw std::runtime_error("invalid duration accepted");
      } catch (const std::invalid_argument&) {}
    }
    check(world->bodyState(body).transform == unchanged.transform &&
              world->bodyState(body).linearVelocity == unchanged.linearVelocity &&
              world->takeChangedBodyStates().empty(), "invalid commands do not mutate state");
    auto other = jolt::createWorld();
    const auto staticBody = world->createBody({shape});
    const auto dynamicBody = world->createBody(
        {shape, core::MotionType::dynamicBody, {{10, 10, 0}, {}}, 1, {}});
    check(!motion->moveKinematic({}, {}, dt) &&
              !motion->moveKinematic(staticBody, {}, dt) &&
              !motion->moveKinematic(dynamicBody, {}, dt) &&
              !dynamic_cast<core::KinematicMotion*>(other.get())->moveKinematic(body, {}, dt),
          "incompatible/unknown/cross-world handles");
    motion->moveKinematic(body, {{2, 4, 0}, state.transform.rotation}, dt);
    world->step(dt);
    check(world->destroyBody(body) && !motion->moveKinematic(body, {}, dt), "stale target");
    for (const auto& changed : world->takeChangedBodyStates())
      check(changed.body != body, "destroy removes pending changes");

    // A moving kinematic platform pushes a dynamic cube but is not displaced.
    auto contactWorld = jolt::createWorld();
    auto* platformMotion = dynamic_cast<core::KinematicMotion*>(contactWorld.get());
    const auto platformShape = contactWorld->createShape(
        {core::ShapeType::box, {2, .25, 2}});
    const auto cubeShape = contactWorld->createShape({});
    const auto platform = contactWorld->createBody(
        {platformShape, core::MotionType::kinematicBody, {}, 0, {}});
    const auto cube = contactWorld->createBody(
        {cubeShape, core::MotionType::dynamicBody, {{0, .75, 0}, {}}, 1, {}});
    for (int i = 1; i <= 120; ++i) {
      platformMotion->moveKinematic(platform, {{0, i / 120.0, 0}, {}}, dt);
      contactWorld->step(dt);
    }
    check(std::abs(contactWorld->bodyState(platform).transform.translation.y - 1) < .001 &&
              std::abs(contactWorld->bodyState(cube).transform.translation.y - 1.75) < .04,
          "kinematic platform transfers motion to dynamic body");
    contactWorld->step(dt);
    const auto continued = contactWorld->bodyState(platform).transform;
    check(std::abs(continued.translation.y - (1 + 1.0 / 120)) < .001,
          "kinematic velocity persists until replaced");
    platformMotion->moveKinematic(platform, continued, dt);
    contactWorld->step(dt);
    check(std::abs(contactWorld->bodyState(platform).transform.translation.y -
                   continued.translation.y) < .001, "hold replaces persistent velocity");
  } catch (const std::exception& error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
