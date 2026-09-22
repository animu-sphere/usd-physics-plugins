#include "usd_physics/jolt/availability.h"

#include "usd_physics/core/errors.h"
#include "usd_physics/core/ground_query.h"
#include "usd_physics/core/segment_query.h"

#include <Jolt/Jolt.h>

#include <Jolt/Core/Factory.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyFilter.h>
#include <Jolt/Physics/Collision/BroadPhase/BroadPhaseLayer.h>
#include <Jolt/Physics/Collision/CastResult.h>
#include <Jolt/Physics/Collision/CollisionCollectorImpl.h>
#include <Jolt/Physics/Collision/NarrowPhaseQuery.h>
#include <Jolt/Physics/Collision/ObjectLayer.h>
#include <Jolt/Physics/Collision/RayCast.h>
#include <Jolt/Physics/Collision/ShapeCast.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Constraints/FixedConstraint.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/RegisterTypes.h>

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <limits>
#include <memory>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <string>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

#if defined(JPH_DOUBLE_PRECISION)
#error "physicsJolt requires a single-precision Jolt Physics build"
#endif

namespace usd_physics::jolt {
namespace {

static_assert(JPH_VERSION_MAJOR == 5 && JPH_VERSION_MINOR == 5 &&
                  JPH_VERSION_PATCH == 0,
              "physicsJolt requires Jolt Physics 5.5.0");

constexpr std::uint32_t maxBodies = 1024;
constexpr std::uint32_t maxBodyPairs = 1024;
constexpr std::uint32_t maxContactConstraints = 1024;
constexpr std::size_t tempAllocatorBytes = 10 * 1024 * 1024;

namespace BroadPhaseLayers {
const JPH::BroadPhaseLayer nonMoving{0};
const JPH::BroadPhaseLayer moving{1};
constexpr JPH::uint count = 2;
} // namespace BroadPhaseLayers

std::mutex joltRuntimeMutex;
std::size_t joltRuntimeUsers = 0;
std::atomic<std::uint64_t> nextWorldId{1};

class JoltRuntime final {
public:
  JoltRuntime() {
    std::lock_guard<std::mutex> lock{joltRuntimeMutex};
    if (joltRuntimeUsers == 0) {
      if (JPH::Factory::sInstance != nullptr) {
        throw core::PhysicsError{
            core::PhysicsErrorCode::solverFailure,
            "Jolt Physics was initialized outside physicsJolt"};
      }
      JPH::RegisterDefaultAllocator();
      JPH::Factory::sInstance = new JPH::Factory();
      JPH::RegisterTypes();
    }
    ++joltRuntimeUsers;
  }

  ~JoltRuntime() {
    std::lock_guard<std::mutex> lock{joltRuntimeMutex};
    --joltRuntimeUsers;
    if (joltRuntimeUsers == 0) {
      JPH::UnregisterTypes();
      delete JPH::Factory::sInstance;
      JPH::Factory::sInstance = nullptr;
    }
  }

  JoltRuntime(const JoltRuntime&) = delete;
  JoltRuntime& operator=(const JoltRuntime&) = delete;
};

struct ObjectLayerRecord {
  core::MotionType motionType;
  core::CollisionFilter filter;
};

class ObjectLayerRegistry final {
public:
  JPH::ObjectLayer layerFor(core::MotionType motionType,
                            core::CollisionFilter filter) {
    for (std::size_t index = 0; index < records_.size(); ++index) {
      const auto& record = records_[index];
      if (record.motionType == motionType &&
          record.filter.categories == filter.categories &&
          record.filter.collidesWith == filter.collidesWith) {
        return static_cast<JPH::ObjectLayer>(index);
      }
    }
    if (records_.size() >
        static_cast<std::size_t>(std::numeric_limits<JPH::ObjectLayer>::max())) {
      throw core::PhysicsError{core::PhysicsErrorCode::resourceExhausted,
                               "Jolt object-layer capacity was exhausted"};
    }
    records_.push_back({motionType, filter});
    return static_cast<JPH::ObjectLayer>(records_.size() - 1);
  }

  [[nodiscard]] const ObjectLayerRecord& record(JPH::ObjectLayer layer) const {
    return records_[static_cast<std::size_t>(layer)];
  }

private:
  std::vector<ObjectLayerRecord> records_;
};

class ObjectLayerPairFilter final : public JPH::ObjectLayerPairFilter {
public:
  explicit ObjectLayerPairFilter(const ObjectLayerRegistry& layers)
      : layers_(layers) {}

  bool ShouldCollide(JPH::ObjectLayer first,
                     JPH::ObjectLayer second) const override {
    return core::collisionFiltersAllowPair(layers_.record(first).filter,
                                            layers_.record(second).filter);
  }

private:
  const ObjectLayerRegistry& layers_;
};

class BroadPhaseLayerInterface final : public JPH::BroadPhaseLayerInterface {
public:
  explicit BroadPhaseLayerInterface(const ObjectLayerRegistry& layers)
      : layers_(layers) {}

  JPH::uint GetNumBroadPhaseLayers() const override {
    return BroadPhaseLayers::count;
  }

  JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer layer) const override {
    return layers_.record(layer).motionType == core::MotionType::staticBody
               ? BroadPhaseLayers::nonMoving
               : BroadPhaseLayers::moving;
  }

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
  const char* GetBroadPhaseLayerName(JPH::BroadPhaseLayer layer) const override {
    return layer == BroadPhaseLayers::nonMoving ? "non-moving" : "moving";
  }
#endif

private:
  const ObjectLayerRegistry& layers_;
};

class ObjectVsBroadPhaseLayerFilter final
    : public JPH::ObjectVsBroadPhaseLayerFilter {
public:
  explicit ObjectVsBroadPhaseLayerFilter(const ObjectLayerRegistry& layers)
      : layers_(layers) {}

  bool ShouldCollide(JPH::ObjectLayer objectLayer,
                     JPH::BroadPhaseLayer broadPhaseLayer) const override {
    if (layers_.record(objectLayer).motionType == core::MotionType::staticBody) {
      return broadPhaseLayer == BroadPhaseLayers::moving;
    }
    return true;
  }

private:
  const ObjectLayerRegistry& layers_;
};

template <typename Handle, typename Value>
using HandleMap =
    std::unordered_map<Handle, Value, core::PhysicsHandleHash<Handle>>;

float toJoltFloat(double value, const char* name) {
  constexpr double max = static_cast<double>(std::numeric_limits<float>::max());
  if (!std::isfinite(value) || value < -max || value > max) {
    throw std::invalid_argument(std::string{name} +
                                " must fit in the Jolt scalar range");
  }
  return static_cast<float>(value);
}

JPH::Vec3 toJoltVector(core::Vector3 value, const char* name) {
  return {toJoltFloat(value.x, name), toJoltFloat(value.y, name),
          toJoltFloat(value.z, name)};
}

JPH::RVec3 toJoltPosition(core::Vector3 value, const char* name) {
  return {static_cast<JPH::Real>(toJoltFloat(value.x, name)),
          static_cast<JPH::Real>(toJoltFloat(value.y, name)),
          static_cast<JPH::Real>(toJoltFloat(value.z, name))};
}

JPH::Quat toJoltRotation(core::Quaternion value) {
  const double magnitude = std::sqrt(value.w * value.w + value.x * value.x +
                                     value.y * value.y + value.z * value.z);
  if (!std::isfinite(magnitude) || magnitude == 0.0) {
    throw std::invalid_argument("body rotation must have non-zero magnitude");
  }
  return {toJoltFloat(value.x / magnitude, "body rotation"),
          toJoltFloat(value.y / magnitude, "body rotation"),
          toJoltFloat(value.z / magnitude, "body rotation"),
          toJoltFloat(value.w / magnitude, "body rotation")};
}

core::Vector3 toCoreVector(JPH::Vec3Arg value) {
  return {static_cast<double>(value.GetX()), static_cast<double>(value.GetY()),
          static_cast<double>(value.GetZ())};
}

core::Vector3 toCorePosition(JPH::RVec3Arg value) {
  return {static_cast<double>(value.GetX()), static_cast<double>(value.GetY()),
          static_cast<double>(value.GetZ())};
}

core::Quaternion toCoreRotation(JPH::QuatArg value) {
  return {static_cast<double>(value.GetW()), static_cast<double>(value.GetX()),
          static_cast<double>(value.GetY()), static_cast<double>(value.GetZ())};
}

bool stateChanged(const core::BodyState& previous,
                  const core::BodyState& current) {
  return previous.transform != current.transform ||
         previous.linearVelocity != current.linearVelocity;
}

class JoltWorld final : public core::PhysicsWorld,
                        public core::GroundQuery,
                        public core::SegmentQuery {
public:
  JoltWorld()
      : worldId_(nextWorldId.fetch_add(1, std::memory_order_relaxed)),
        pairFilter_(layers_), broadPhaseInterface_(layers_),
        objectVsBroadPhaseFilter_(layers_),
        jobSystem_(JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers,
                   workerThreadCount()) {
    if (worldId_ == 0 || worldId_ > std::numeric_limits<std::uint32_t>::max()) {
      throw core::PhysicsError{core::PhysicsErrorCode::resourceExhausted,
                               "physics world handle capacity was exhausted"};
    }
    physicsSystem_.Init(maxBodies, 0, maxBodyPairs, maxContactConstraints,
                        broadPhaseInterface_, objectVsBroadPhaseFilter_,
                        pairFilter_);
  }

  ~JoltWorld() override {
    auto& bodyInterface = physicsSystem_.GetBodyInterface();
    for (auto& entry : constraints_) {
      physicsSystem_.RemoveConstraint(entry.second.constraint.GetPtr());
    }
    constraints_.clear();
    for (auto& entry : bodies_) {
      bodyInterface.RemoveBody(entry.second.id);
      bodyInterface.DestroyBody(entry.second.id);
    }
  }

  core::ShapeHandle
  createShape(const core::ShapeDescriptor& descriptor) override {
    core::validateShapeDescriptor(descriptor);
    const JPH::Vec3 halfExtents =
        toJoltVector(descriptor.halfExtents, "box half extents");
    const float minimumExtent =
        std::min({halfExtents.GetX(), halfExtents.GetY(), halfExtents.GetZ()});
    if (minimumExtent <= 0.0f) {
      throw std::invalid_argument(
          "box half extents must remain positive in the Jolt scalar range");
    }
    const float convexRadius =
        std::min(JPH::cDefaultConvexRadius, minimumExtent * 0.5f);
    JPH::ShapeRefC shape = new JPH::BoxShape(halfExtents, convexRadius);
    const core::ShapeHandle handle = nextHandle<core::ShapeHandle>(nextShape_);
    shapes_.emplace(handle, ShapeRecord{std::move(shape)});
    return handle;
  }

  bool destroyShape(core::ShapeHandle shape) noexcept override {
    for (const auto& entry : bodies_) {
      if (entry.second.shape == shape) {
        return false;
      }
    }
    return shapes_.erase(shape) != 0;
  }

  core::BodyHandle
  createBody(const core::BodyDescriptor& descriptor) override {
    core::validateBodyDescriptor(descriptor);
    const auto shape = shapes_.find(descriptor.shape);
    if (shape == shapes_.end()) {
      throw std::invalid_argument("body references an unknown physicsJolt shape");
    }

    const bool dynamic = descriptor.motionType == core::MotionType::dynamicBody;
    const JPH::ObjectLayer objectLayer =
        layers_.layerFor(descriptor.motionType, descriptor.collisionFilter);
    JPH::BodyCreationSettings settings(
        shape->second.shape,
        toJoltPosition(descriptor.initialTransform.translation, "body position"),
        toJoltRotation(descriptor.initialTransform.rotation),
        dynamic ? JPH::EMotionType::Dynamic : JPH::EMotionType::Static,
        objectLayer);
    if (dynamic) {
      settings.mOverrideMassProperties =
          JPH::EOverrideMassProperties::CalculateInertia;
      settings.mMassPropertiesOverride.mMass =
          toJoltFloat(descriptor.mass, "body mass");
    }

    JPH::Body* body = physicsSystem_.GetBodyInterface().CreateBody(settings);
    if (body == nullptr) {
      throw core::PhysicsError{core::PhysicsErrorCode::resourceExhausted,
                               "Jolt body capacity was exhausted"};
    }
    physicsSystem_.GetBodyInterface().AddBody(
        body->GetID(), dynamic ? JPH::EActivation::Activate
                               : JPH::EActivation::DontActivate);

    const core::BodyHandle handle = nextHandle<core::BodyHandle>(nextBody_);
    const core::BodyState initial{handle, descriptor.initialTransform, {}};
    auto inserted = bodies_.emplace(
        handle, BodyRecord{body->GetID(), body, descriptor.shape,
                           descriptor.motionType, initial});
    inserted.first->second.lastState =
        readBodyState(handle, inserted.first->second);
    return handle;
  }

  bool destroyBody(core::BodyHandle body) noexcept override {
    const auto found = bodies_.find(body);
    if (found == bodies_.end()) {
      return false;
    }
    for (auto iterator = constraints_.begin(); iterator != constraints_.end();) {
      if (iterator->second.firstBody == body ||
          iterator->second.secondBody == body) {
        physicsSystem_.RemoveConstraint(iterator->second.constraint.GetPtr());
        iterator = constraints_.erase(iterator);
      } else {
        ++iterator;
      }
    }
    auto& bodyInterface = physicsSystem_.GetBodyInterface();
    bodyInterface.RemoveBody(found->second.id);
    bodyInterface.DestroyBody(found->second.id);
    bodies_.erase(found);
    changed_.erase(body);
    return true;
  }

  core::ConstraintHandle createConstraint(
      const core::ConstraintDescriptor& descriptor) override {
    core::validateConstraintDescriptor(descriptor);
    const auto first = bodies_.find(descriptor.firstBody);
    const auto second = bodies_.find(descriptor.secondBody);
    if (first == bodies_.end() || second == bodies_.end()) {
      throw std::invalid_argument(
          "constraint references an unknown physicsJolt body");
    }

    JPH::FixedConstraintSettings settings;
    settings.mAutoDetectPoint = true;
    JPH::Ref<JPH::Constraint> constraint =
        settings.Create(*first->second.body, *second->second.body);
    physicsSystem_.AddConstraint(constraint.GetPtr());
    const core::ConstraintHandle handle =
        nextHandle<core::ConstraintHandle>(nextConstraint_);
    constraints_.emplace(
        handle, ConstraintRecord{descriptor.firstBody, descriptor.secondBody,
                                 std::move(constraint)});
    return handle;
  }

  bool destroyConstraint(core::ConstraintHandle constraint) noexcept override {
    const auto found = constraints_.find(constraint);
    if (found == constraints_.end()) {
      return false;
    }
    physicsSystem_.RemoveConstraint(found->second.constraint.GetPtr());
    constraints_.erase(found);
    return true;
  }

  bool applyForce(core::BodyHandle body, core::Vector3 force) override {
    core::validatePhysicsVector(force, "force");
    const auto found = bodies_.find(body);
    if (found == bodies_.end() ||
        found->second.motionType != core::MotionType::dynamicBody) {
      return false;
    }
    physicsSystem_.GetBodyInterface().AddForce(
        found->second.id, toJoltVector(force, "force"),
        JPH::EActivation::Activate);
    return true;
  }

  bool setLinearVelocity(core::BodyHandle body,
                         core::Vector3 velocity) override {
    core::validatePhysicsVector(velocity, "linear velocity");
    const auto found = bodies_.find(body);
    if (found == bodies_.end() ||
        found->second.motionType != core::MotionType::dynamicBody) {
      return false;
    }
    physicsSystem_.GetBodyInterface().SetLinearVelocity(
        found->second.id, toJoltVector(velocity, "linear velocity"));
    return true;
  }

  core::BodyState bodyState(core::BodyHandle body) const override {
    const auto found = bodies_.find(body);
    if (found == bodies_.end()) {
      throw std::out_of_range("unknown physicsJolt body handle");
    }
    return readBodyState(body, found->second);
  }

  std::optional<core::GroundContact>
  groundContact(core::BodyHandle body, double maxDistance) const override {
    core::validateGroundProbeDistance(maxDistance);
    const auto found = bodies_.find(body);
    if (found == bodies_.end()) {
      throw std::out_of_range("unknown physicsJolt body handle");
    }
    const float joltDistance = toJoltFloat(maxDistance, "ground probe distance");

    const auto shape = shapes_.find(found->second.shape);
    if (shape == shapes_.end()) {
      throw core::PhysicsError{core::PhysicsErrorCode::solverFailure,
                               "physicsJolt body references a missing shape"};
    }

    const auto& bodyInterface = physicsSystem_.GetBodyInterface();
    const JPH::RShapeCast cast{
        shape->second.shape, JPH::Vec3::sOne(),
        bodyInterface.GetCenterOfMassTransform(found->second.id),
        JPH::Vec3{0.0f, -joltDistance, 0.0f}};
    JPH::ShapeCastSettings settings;
    settings.mReturnDeepestPoint = true;
    JPH::AllHitCollisionCollector<JPH::CastShapeCollector> collector;
    const JPH::IgnoreSingleBodyFilter ignoreTarget{found->second.id};
    const JPH::DefaultObjectLayerFilter objectFilter{
        pairFilter_, bodyInterface.GetObjectLayer(found->second.id)};
    physicsSystem_.GetNarrowPhaseQuery().CastShape(
        cast, settings, cast.mCenterOfMassStart.GetTranslation(), collector, {},
        objectFilter, ignoreTarget);
    if (!collector.HadHit()) {
      return std::nullopt;
    }

    const JPH::ShapeCastResult* groundHit = nullptr;
    JPH::Vec3 groundNormal = JPH::Vec3::sAxisY();
    double groundDistance = 0.0;
    for (const auto& hit : collector.mHits) {
      const JPH::Vec3 normal =
          -hit.mPenetrationAxis.NormalizedOr(-JPH::Vec3::sAxisY());
      if (normal.GetY() <= 0.0f) {
        continue;
      }
      const double distance =
          std::max(0.0, static_cast<double>(hit.mFraction) * maxDistance);
      if (groundHit == nullptr || normal.GetY() > groundNormal.GetY() ||
          (normal.GetY() == groundNormal.GetY() && distance < groundDistance)) {
        groundHit = &hit;
        groundNormal = normal;
        groundDistance = distance;
      }
    }
    if (groundHit == nullptr) {
      return std::nullopt;
    }

    const core::BodyHandle support = bodyHandle(groundHit->mBodyID2);
    if (!support) {
      throw core::PhysicsError{
          core::PhysicsErrorCode::solverFailure,
          "physicsJolt ground query returned an untracked body"};
    }
    core::GroundContact contact{support, toCoreVector(groundNormal),
                                groundDistance};
    core::validateGroundContact(contact);
    return contact;
  }

  std::optional<core::SegmentHit>
  segmentHit(core::Vector3 origin, core::Vector3 target,
             core::BodyHandle ignoredBody) const override {
    core::validateSegmentEndpoints(origin, target);
    const core::Vector3 displacement{target.x - origin.x, target.y - origin.y,
                                     target.z - origin.z};
    const JPH::RRayCast ray{toJoltPosition(origin, "segment origin"),
                            toJoltVector(displacement, "segment displacement")};
    JPH::RayCastResult hit;
    bool hadHit = false;
    if (ignoredBody) {
      const auto ignored = bodies_.find(ignoredBody);
      if (ignored == bodies_.end()) {
        throw std::out_of_range("unknown ignored physicsJolt body handle");
      }
      const JPH::IgnoreSingleBodyFilter filter{ignored->second.id};
      hadHit = physicsSystem_.GetNarrowPhaseQuery().CastRay(ray, hit, {}, {},
                                                            filter);
    } else {
      hadHit = physicsSystem_.GetNarrowPhaseQuery().CastRay(ray, hit);
    }
    if (!hadHit) {
      return std::nullopt;
    }
    const core::BodyHandle body = bodyHandle(hit.mBodyID);
    if (!body) {
      throw core::PhysicsError{
          core::PhysicsErrorCode::solverFailure,
          "physicsJolt segment query returned an untracked body"};
    }
    core::SegmentHit result{body, static_cast<double>(hit.mFraction)};
    core::validateSegmentHit(result);
    return result;
  }

  void step(Duration fixedStep) override {
    core::validatePhysicsStep(fixedStep);
    const float seconds = toJoltFloat(fixedStep.count(), "physics step");
    if (seconds <= 0.0f) {
      throw std::invalid_argument(
          "physics step must remain positive in the Jolt scalar range");
    }
    const JPH::EPhysicsUpdateError updateError =
        physicsSystem_.Update(seconds, 1, &tempAllocator_, &jobSystem_);
    if (updateError != JPH::EPhysicsUpdateError::None) {
      throw core::PhysicsError{
          core::PhysicsErrorCode::solverFailure,
          "Jolt physics update exhausted a configured simulation capacity"};
    }
    for (auto& entry : bodies_) {
      if (entry.second.motionType != core::MotionType::dynamicBody) {
        continue;
      }
      core::BodyState current = readBodyState(entry.first, entry.second);
      if (stateChanged(entry.second.lastState, current)) {
        entry.second.lastState = current;
        changed_[entry.first] = std::move(current);
      }
    }
  }

  std::vector<core::BodyState> takeChangedBodyStates() override {
    std::vector<core::BodyState> result;
    result.reserve(changed_.size());
    for (auto& entry : changed_) {
      result.push_back(std::move(entry.second));
    }
    std::sort(result.begin(), result.end(), [](const auto& left, const auto& right) {
      return left.body.value() < right.body.value();
    });
    changed_.clear();
    return result;
  }

private:
  struct ShapeRecord {
    JPH::ShapeRefC shape;
  };

  struct BodyRecord {
    JPH::BodyID id;
    JPH::Body* body;
    core::ShapeHandle shape;
    core::MotionType motionType;
    core::BodyState lastState;
  };

  struct ConstraintRecord {
    core::BodyHandle firstBody;
    core::BodyHandle secondBody;
    JPH::Ref<JPH::Constraint> constraint;
  };

  static int workerThreadCount() {
    const unsigned int concurrency = std::thread::hardware_concurrency();
    return concurrency > 1 ? static_cast<int>(concurrency - 1) : 0;
  }

  template <typename Handle>
  Handle nextHandle(std::uint64_t& nextLocalValue) const {
    if (nextLocalValue == 0 ||
        nextLocalValue > std::numeric_limits<std::uint32_t>::max()) {
      throw core::PhysicsError{core::PhysicsErrorCode::resourceExhausted,
                               "physics handle capacity was exhausted"};
    }
    const std::uint64_t value = (worldId_ << 32U) | nextLocalValue++;
    return Handle{value};
  }

  core::BodyState readBodyState(core::BodyHandle handle,
                                const BodyRecord& record) const {
    const auto& bodyInterface = physicsSystem_.GetBodyInterface();
    return {handle,
            core::Transform{toCorePosition(bodyInterface.GetPosition(record.id)),
                            toCoreRotation(bodyInterface.GetRotation(record.id))},
            toCoreVector(bodyInterface.GetLinearVelocity(record.id))};
  }

  core::BodyHandle bodyHandle(JPH::BodyID id) const noexcept {
    for (const auto& entry : bodies_) {
      if (entry.second.id == id) {
        return entry.first;
      }
    }
    return {};
  }

  JoltRuntime runtime_;
  const std::uint64_t worldId_;
  ObjectLayerRegistry layers_;
  ObjectLayerPairFilter pairFilter_;
  BroadPhaseLayerInterface broadPhaseInterface_;
  ObjectVsBroadPhaseLayerFilter objectVsBroadPhaseFilter_;
  JPH::TempAllocatorImpl tempAllocator_{tempAllocatorBytes};
  JPH::JobSystemThreadPool jobSystem_;
  JPH::PhysicsSystem physicsSystem_;
  std::uint64_t nextShape_{1};
  std::uint64_t nextBody_{1};
  std::uint64_t nextConstraint_{1};
  HandleMap<core::ShapeHandle, ShapeRecord> shapes_;
  HandleMap<core::BodyHandle, BodyRecord> bodies_;
  HandleMap<core::ConstraintHandle, ConstraintRecord> constraints_;
  HandleMap<core::BodyHandle, core::BodyState> changed_;
};

} // namespace

bool backendAvailable() noexcept {
  return true;
}

std::unique_ptr<core::PhysicsWorld> createWorld() {
  return std::make_unique<JoltWorld>();
}

} // namespace usd_physics::jolt
