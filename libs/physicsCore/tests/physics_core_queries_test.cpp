#include "usd_physics/core/ground_query.h"
#include "usd_physics/core/segment_query.h"
#include "usd_physics/core/world.h"

#include <limits>
#include <optional>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

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

class WorldWithoutQueries : public PhysicsWorld {
public:
  ShapeHandle createShape(const ShapeDescriptor&) override { return {}; }
  bool destroyShape(ShapeHandle) noexcept override { return false; }
  BodyHandle createBody(const BodyDescriptor&) override { return {}; }
  bool destroyBody(BodyHandle) noexcept override { return false; }
  ConstraintHandle createConstraint(const ConstraintDescriptor&) override {
    return {};
  }
  bool destroyConstraint(ConstraintHandle) noexcept override { return false; }
  bool applyForce(BodyHandle, Vector3) override { return false; }
  bool setLinearVelocity(BodyHandle, Vector3) override { return false; }
  BodyState bodyState(BodyHandle) const override { return {}; }
  void step(Duration) override {}
  std::vector<BodyState> takeChangedBodyStates() override { return {}; }
};

class QueryWorld final : public WorldWithoutQueries,
                         public SegmentQuery,
                         public GroundQuery {
public:
  std::optional<SegmentHit>
  segmentHit(Vector3 origin, Vector3 target,
             BodyHandle ignoredBody) const override {
    validateSegmentEndpoints(origin, target);
    lastIgnoredBody = ignoredBody;
    return segmentResult;
  }

  std::optional<GroundContact>
  groundContact(BodyHandle body, double maxDistance) const override {
    validateGroundProbeDistance(maxDistance);
    lastGroundBody = body;
    lastGroundDistance = maxDistance;
    return groundResult;
  }

  std::optional<SegmentHit> segmentResult;
  std::optional<GroundContact> groundResult;
  mutable BodyHandle lastIgnoredBody;
  mutable BodyHandle lastGroundBody;
  mutable double lastGroundDistance{-1.0};
};

} // namespace

int main() {
  static_assert(std::has_virtual_destructor_v<SegmentQuery>);
  static_assert(std::has_virtual_destructor_v<GroundQuery>);

  const BodyHandle body{11};
  const BodyHandle support{12};
  const BodyHandle ignored{13};
  validateSegmentEndpoints({}, {0.0, 0.0, 1.0});
  validateSegmentHit({body, 0.0});
  validateSegmentHit({body, 1.0});
  validateGroundProbeDistance(0.0);
  validateGroundContact({support, {0.0, 2.0, 0.0}, 0.25});

  const double infinity = std::numeric_limits<double>::infinity();
  const double nan = std::numeric_limits<double>::quiet_NaN();
  if (!rejectsInvalidArgument([] { validateSegmentEndpoints({}, {}); }) ||
      !rejectsInvalidArgument(
          [infinity] { validateSegmentEndpoints({}, {infinity, 0.0, 0.0}); }) ||
      !rejectsInvalidArgument([nan] {
        validateSegmentEndpoints({nan, 0.0, 0.0}, {1.0, 0.0, 0.0});
      }) ||
      !rejectsInvalidArgument([infinity] {
        validateSegmentEndpoints({-infinity, 0.0, 0.0},
                                 {infinity, 0.0, 0.0});
      }) ||
      !rejectsInvalidArgument([] { validateSegmentHit({{}, 0.5}); }) ||
      !rejectsInvalidArgument([body] { validateSegmentHit({body, -0.1}); }) ||
      !rejectsInvalidArgument([body] { validateSegmentHit({body, 1.1}); }) ||
      !rejectsInvalidArgument([body, nan] { validateSegmentHit({body, nan}); })) {
    return 1;
  }

  if (!rejectsInvalidArgument(
          [] { validateGroundProbeDistance(-0.1); }) ||
      !rejectsInvalidArgument(
          [infinity] { validateGroundProbeDistance(infinity); }) ||
      !rejectsInvalidArgument([nan] { validateGroundProbeDistance(nan); }) ||
      !rejectsInvalidArgument(
          [] { validateGroundContact({{}, {0.0, 1.0, 0.0}, 0.0}); }) ||
      !rejectsInvalidArgument(
          [support] { validateGroundContact({support, {}, 0.0}); }) ||
      !rejectsInvalidArgument([support, infinity] {
        validateGroundContact({support, {infinity, 1.0, 0.0}, 0.0});
      }) ||
      !rejectsInvalidArgument([support] {
        validateGroundContact({support, {0.0, 1.0, 0.0}, -0.1});
      }) ||
      !rejectsInvalidArgument([support, nan] {
        validateGroundContact({support, {0.0, 1.0, 0.0}, nan});
      })) {
    return 2;
  }

  WorldWithoutQueries plainWorld;
  PhysicsWorld* plain = &plainWorld;
  if (dynamic_cast<SegmentQuery*>(plain) != nullptr ||
      dynamic_cast<GroundQuery*>(plain) != nullptr) {
    return 3;
  }

  QueryWorld queryWorld;
  PhysicsWorld* world = &queryWorld;
  auto* segmentQuery = dynamic_cast<SegmentQuery*>(world);
  auto* groundQuery = dynamic_cast<GroundQuery*>(world);
  if (segmentQuery == nullptr || groundQuery == nullptr) {
    return 4;
  }

  queryWorld.segmentResult = SegmentHit{body, 0.4};
  queryWorld.groundResult = GroundContact{support, {0.0, 1.0, 0.0}, 0.2};
  const auto hit = segmentQuery->segmentHit({}, {0.0, 0.0, 2.0}, ignored);
  const auto contact = groundQuery->groundContact(body, 0.5);
  if (!hit || hit->body != body || hit->fraction != 0.4 ||
      queryWorld.lastIgnoredBody != ignored || !contact ||
      contact->supportBody != support || contact->distance != 0.2 ||
      queryWorld.lastGroundBody != body || queryWorld.lastGroundDistance != 0.5) {
    return 5;
  }

  return 0;
}
