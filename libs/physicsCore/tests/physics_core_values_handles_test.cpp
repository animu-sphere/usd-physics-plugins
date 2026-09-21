#include "usd_physics/core/handles.h"
#include "usd_physics/core/value_types.h"

#include <type_traits>
#include <unordered_set>

namespace {

using namespace usd_physics::core;

constexpr Vector3 zeroVector;
constexpr Quaternion identityRotation;
constexpr Transform identityTransform;
constexpr BodyHandle invalidBody;
constexpr BodyHandle body{42};
constexpr ShapeHandle shape{42};

static_assert(zeroVector == Vector3{0.0, 0.0, 0.0});
static_assert(identityRotation == Quaternion{1.0, 0.0, 0.0, 0.0});
static_assert(identityTransform == Transform{});
static_assert(!invalidBody.isValid());
static_assert(body.isValid());
static_assert(body.value() == 42);
static_assert(shape.value() == body.value());
static_assert(!std::is_same_v<BodyHandle, ShapeHandle>);
static_assert(!std::is_convertible_v<BodyHandle::ValueType, BodyHandle>);
static_assert(std::is_trivially_copyable_v<Vector3>);
static_assert(std::is_trivially_copyable_v<Quaternion>);
static_assert(std::is_trivially_copyable_v<Transform>);
static_assert(std::is_trivially_copyable_v<BodyHandle>);
static_assert(sizeof(BodyHandle) == sizeof(BodyHandle::ValueType));

} // namespace

int main() {
  std::unordered_set<BodyHandle, PhysicsHandleHash<BodyHandle>> bodies;
  bodies.insert(body);
  bodies.insert(BodyHandle{42});
  bodies.insert(BodyHandle{7});

  if (bodies.size() != 2 || bodies.count(body) != 1 ||
      bodies.count(BodyHandle{7}) != 1) {
    return 1;
  }

  const Transform pose{{1.0, 2.0, 3.0}, {0.5, 0.5, 0.5, 0.5}};
  return pose.translation == Vector3{1.0, 2.0, 3.0} &&
                 pose.rotation == Quaternion{0.5, 0.5, 0.5, 0.5}
             ? 0
             : 2;
}
