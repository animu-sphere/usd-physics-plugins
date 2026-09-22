#include "usd_physics/core/errors.h"
#include "usd_physics/core/world.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
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

template <typename Function> bool rejectsOutOfRange(Function&& function) {
  try {
    std::forward<Function>(function)();
  } catch (const std::out_of_range&) {
    return true;
  }
  return false;
}

class DeterministicWorld final : public PhysicsWorld {
public:
  DeterministicWorld() : handleBase_(nextWorld_++ << 32) {}

  ShapeHandle createShape(const ShapeDescriptor& descriptor) override {
    validateShapeDescriptor(descriptor);
    const ShapeHandle handle{handleBase_ + nextShape_++};
    shapes_.emplace(handle, descriptor);
    return handle;
  }

  bool destroyShape(ShapeHandle shape) noexcept override {
    for (const auto& entry : bodies_) {
      if (entry.second.descriptor.shape == shape) {
        return false;
      }
    }
    return shapes_.erase(shape) != 0;
  }

  BodyHandle createBody(const BodyDescriptor& descriptor) override {
    validateBodyDescriptor(descriptor);
    if (shapes_.find(descriptor.shape) == shapes_.end()) {
      throw std::invalid_argument("body references an unknown shape");
    }
    const BodyHandle handle{handleBase_ + nextBody_++};
    bodies_.emplace(handle,
                    BodyRecord{descriptor, {handle, descriptor.initialTransform, {}}, {}});
    return handle;
  }

  bool destroyBody(BodyHandle body) noexcept override {
    const auto found = bodies_.find(body);
    if (found == bodies_.end()) {
      return false;
    }
    for (auto iterator = constraints_.begin(); iterator != constraints_.end();) {
      if (iterator->second.firstBody == body ||
          iterator->second.secondBody == body) {
        iterator = constraints_.erase(iterator);
      } else {
        ++iterator;
      }
    }
    changed_.erase(body);
    bodies_.erase(found);
    return true;
  }

  ConstraintHandle
  createConstraint(const ConstraintDescriptor& descriptor) override {
    validateConstraintDescriptor(descriptor);
    if (bodies_.find(descriptor.firstBody) == bodies_.end() ||
        bodies_.find(descriptor.secondBody) == bodies_.end()) {
      throw std::invalid_argument("constraint references an unknown body");
    }
    const ConstraintHandle handle{handleBase_ + nextConstraint_++};
    constraints_.emplace(handle, descriptor);
    return handle;
  }

  bool destroyConstraint(ConstraintHandle constraint) noexcept override {
    return constraints_.erase(constraint) != 0;
  }

  bool applyForce(BodyHandle body, Vector3 force) override {
    validatePhysicsVector(force, "force");
    const auto found = bodies_.find(body);
    if (found == bodies_.end() ||
        found->second.descriptor.motionType != MotionType::dynamicBody) {
      return false;
    }
    found->second.accumulatedForce.x += force.x;
    found->second.accumulatedForce.y += force.y;
    found->second.accumulatedForce.z += force.z;
    return true;
  }

  bool setLinearVelocity(BodyHandle body, Vector3 velocity) override {
    validatePhysicsVector(velocity, "linear velocity");
    const auto found = bodies_.find(body);
    if (found == bodies_.end() ||
        found->second.descriptor.motionType != MotionType::dynamicBody) {
      return false;
    }
    found->second.state.linearVelocity = velocity;
    return true;
  }

  BodyState bodyState(BodyHandle body) const override {
    const auto found = bodies_.find(body);
    if (found == bodies_.end()) {
      throw std::out_of_range("unknown body handle");
    }
    return found->second.state;
  }

  void step(Duration fixedStep) override {
    validatePhysicsStep(fixedStep);
    const double seconds = fixedStep.count();
    for (auto& entry : bodies_) {
      auto& record = entry.second;
      if (record.descriptor.motionType != MotionType::dynamicBody) {
        continue;
      }
      const BodyState previous = record.state;
      record.state.linearVelocity.x +=
          record.accumulatedForce.x / record.descriptor.mass * seconds;
      record.state.linearVelocity.y +=
          record.accumulatedForce.y / record.descriptor.mass * seconds;
      record.state.linearVelocity.z +=
          record.accumulatedForce.z / record.descriptor.mass * seconds;
      record.state.transform.translation.x +=
          record.state.linearVelocity.x * seconds;
      record.state.transform.translation.y +=
          record.state.linearVelocity.y * seconds;
      record.state.transform.translation.z +=
          record.state.linearVelocity.z * seconds;
      record.accumulatedForce = {};
      if (record.state.transform != previous.transform ||
          record.state.linearVelocity != previous.linearVelocity) {
        changed_.insert(entry.first);
      }
    }
  }

  std::vector<BodyState> takeChangedBodyStates() override {
    std::vector<BodyState> result;
    result.reserve(changed_.size());
    for (const auto body : changed_) {
      result.push_back(bodies_.at(body).state);
    }
    std::sort(result.begin(), result.end(), [](const BodyState& left,
                                               const BodyState& right) {
      return left.body.value() < right.body.value();
    });
    changed_.clear();
    return result;
  }

private:
  struct BodyRecord {
    BodyDescriptor descriptor;
    BodyState state;
    Vector3 accumulatedForce;
  };

  template <typename Handle, typename Value>
  using HandleMap = std::unordered_map<Handle, Value, PhysicsHandleHash<Handle>>;
  template <typename Handle>
  using HandleSet = std::unordered_set<Handle, PhysicsHandleHash<Handle>>;

  inline static std::uint64_t nextWorld_{1};
  const std::uint64_t handleBase_;
  std::uint64_t nextShape_{1};
  std::uint64_t nextBody_{1};
  std::uint64_t nextConstraint_{1};
  HandleMap<ShapeHandle, ShapeDescriptor> shapes_;
  HandleMap<BodyHandle, BodyRecord> bodies_;
  HandleMap<ConstraintHandle, ConstraintDescriptor> constraints_;
  HandleSet<BodyHandle> changed_;
};

bool near(double left, double right) {
  return std::abs(left - right) < 1.0e-9;
}

} // namespace

int main() {
  static_assert(!std::is_copy_constructible_v<DeterministicWorld>);
  static_assert(!std::is_copy_assignable_v<DeterministicWorld>);
  static_assert(!std::is_move_constructible_v<DeterministicWorld>);
  static_assert(!std::is_move_assignable_v<DeterministicWorld>);

  DeterministicWorld world;
  DeterministicWorld otherWorld;

  const auto box = world.createShape({ShapeType::box, {0.5, 0.5, 0.5}});
  const auto floor = world.createBody(
      {box, MotionType::staticBody, Transform{{0.0, -0.5, 0.0}, {}}, 0.0, {}});
  const auto first = world.createBody(
      {box, MotionType::dynamicBody, Transform{{0.0, 2.0, 0.0}, {}}, 2.0, {}});
  const auto second = world.createBody(
      {box, MotionType::dynamicBody, Transform{{0.0, 3.0, 0.0}, {}}, 1.0, {}});
  const auto idle = world.createBody(
      {box, MotionType::dynamicBody, Transform{{0.0, 4.0, 0.0}, {}}, 1.0, {}});
  if (!box || !floor || !first || !second || !idle || first == second ||
      !world.takeChangedBodyStates().empty()) {
    return 1;
  }

  if (!world.applyForce(first, {4.0, 0.0, 0.0}) ||
      !world.setLinearVelocity(second, {0.0, -2.0, 0.0}) ||
      world.applyForce(floor, {1.0, 0.0, 0.0}) ||
      world.applyForce(BodyHandle{}, {1.0, 0.0, 0.0}) ||
      world.applyForce(BodyHandle{1}, {1.0, 0.0, 0.0}) ||
      otherWorld.applyForce(first, {1.0, 0.0, 0.0})) {
    return 2;
  }

  world.step(PhysicsWorld::Duration{0.5});
  const auto changed = world.takeChangedBodyStates();
  if (changed.size() != 2 || changed[0].body != first ||
      changed[1].body != second ||
      !near(changed[0].linearVelocity.x, 1.0) ||
      !near(changed[0].transform.translation.x, 0.5) ||
      !near(changed[1].transform.translation.y, 2.0) ||
      !world.takeChangedBodyStates().empty() ||
      world.bodyState(floor).transform.translation.y != -0.5) {
    return 3;
  }

  if (!rejectsInvalidArgument([&] {
        world.createBody({ShapeHandle{box.value() + 1000},
                          MotionType::dynamicBody, {}, 1.0, {}});
      }) ||
      !rejectsInvalidArgument([&] {
        otherWorld.createBody({box, MotionType::dynamicBody, {}, 1.0, {}});
      }) ||
      !rejectsOutOfRange([&] { world.bodyState(BodyHandle{}); }) ||
      !rejectsOutOfRange([&] { otherWorld.bodyState(first); })) {
    return 4;
  }

  const auto infinity = std::numeric_limits<double>::infinity();
  const auto nan = std::numeric_limits<double>::quiet_NaN();
  if (!rejectsInvalidArgument(
          [&] { world.step(PhysicsWorld::Duration::zero()); }) ||
      !rejectsInvalidArgument(
          [&] { world.step(PhysicsWorld::Duration{-0.1}); }) ||
      !rejectsInvalidArgument(
          [&] { world.step(PhysicsWorld::Duration{infinity}); }) ||
      !rejectsInvalidArgument(
          [&] { world.step(PhysicsWorld::Duration{nan}); }) ||
      !rejectsInvalidArgument(
          [&] { world.applyForce(first, {infinity, 0.0, 0.0}); }) ||
      !rejectsInvalidArgument(
          [&] { world.setLinearVelocity(first, {0.0, nan, 0.0}); })) {
    return 5;
  }

  const auto constraint =
      world.createConstraint({ConstraintType::fixed, floor, first});
  world.step(PhysicsWorld::Duration{0.5});
  if (!constraint || world.destroyShape(box) ||
      !world.destroyBody(first) || world.destroyBody(first) ||
      world.destroyConstraint(constraint) ||
      !rejectsOutOfRange([&] { world.bodyState(first); }) ||
      !world.destroyBody(second) || !world.destroyBody(idle) ||
      !world.destroyBody(floor) ||
      !world.takeChangedBodyStates().empty() || !world.destroyShape(box) ||
      world.destroyShape(box)) {
    return 6;
  }

  const PhysicsError error{PhysicsErrorCode::resourceExhausted,
                           "physics capacity exhausted"};
  if (error.code() != PhysicsErrorCode::resourceExhausted ||
      std::string{error.what()} != "physics capacity exhausted") {
    return 7;
  }

  return 0;
}
