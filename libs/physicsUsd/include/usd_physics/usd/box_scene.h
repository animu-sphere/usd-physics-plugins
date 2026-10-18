#pragma once

#include "usd_physics/core/descriptors.h"
#include <array>
#include <limits>
#include <pxr/usd/usd/stage.h>
#include <stdexcept>
#include <string>
#include <vector>

namespace usd_physics::usd {

enum class ImportErrorCode {
  invalidStage,
  unsupportedUnits,
  unsupportedDeclaration,
  unsupportedTransform,
  invalidValue,
};

class ImportError : public std::runtime_error {
public:
  ImportError(ImportErrorCode code, std::string path,
              const std::string &detail);
  [[nodiscard]] ImportErrorCode code() const noexcept { return code_; }
  [[nodiscard]] const std::string &path() const noexcept { return path_; }

private:
  ImportErrorCode code_;
  std::string path_;
};

// Snapshot-only declarations. Paths belong to the supplied Stage; consumers
// own resource handles, Stage lifetime, scheduling, and writeback.
struct BoxBody {
  std::string path;
  core::ShapeDescriptor shape;
  core::MotionType motionType{core::MotionType::staticBody};
  core::Transform transform;
  double mass{1.0};
};

using PrimitiveBody = BoxBody;

enum class JointType { sixDof, fixed };

struct JointLimit {
  bool applied{false};
  double low{-std::numeric_limits<double>::infinity()};
  double high{std::numeric_limits<double>::infinity()};
};

// Standard USD declarations only; these are not executable core constraints.
// See joint_conversion.h for the separate, bounded descriptor conversion.
struct Joint {
  std::string path;
  JointType type{JointType::sixDof};
  // Empty endpoint means world. Nonempty endpoints name exact rigid bodies.
  std::string body0, body1;
  core::Transform localFrame0, localFrame1;
  bool enabled{true};
  bool collisionEnabled{false};
  // transX/Y/Z in meters, rotX/Y/Z in radians. Absent API means free;
  // low > high means locked, following USD (not a source-format convention).
  // Fixed joints lock all axes and leave this array unused.
  std::array<JointLimit, 6> limits;
};

struct PhysicsScene {
  std::vector<PrimitiveBody> bodies;
  std::vector<Joint> joints;
};

// Adds generic PhysicsJoint frames and six-axis LimitAPI, and FixedJoint.
// Endpoints must be distinct, rigid, unscaled body frames or one world anchor.
// Rejects other joint types, distance limits, drives and breaking thresholds.
// Like the body-only APIs, validates everything and returns sorted snapshots.
// This does not create constraints, interpret format metadata, or simulate.
[[nodiscard]] PhysicsScene readPhysicsScene(const pxr::UsdStageRefPtr &stage);

// Cube, Sphere and Capsule variant of the snapshot reader below. Sphere and
// Capsule require uniform composed collider scale (relative tolerance 1e-9).
// Capsule axis X/Y/Z maps to the core's local Y via shape.localTransform;
// height excludes the hemispheres, and zero height is a sphere.
[[nodiscard]] std::vector<PrimitiveBody>
readPrimitiveScene(const pxr::UsdStageRefPtr &stage);

// Validates the complete supported subset before returning any declarations.
// Requires Y-up meters/kg. Dynamic mass is explicit or computed from scaled
// collision volume and body-local density (fallback 1000 kg/m^3). Zero mass
// and density are ignored per USD; positive mass takes precedence. Inherited
// and collider MassAPI, physics materials and inertia overrides remain
// rejected. Supports a collision Cube on the body, or one translated/rotated
// child Cube of a rigid body Xform. Composes time-independent affine transforms
// and reset stacks. Cube scales are baked into dimensions; shear and inverse
// ops fail. Separate body frames must be rigid; child reset stacks and
// compounds remain unsupported. Enabled kinematic bodies need no mass (it is
// ignored); disabled rigid bodies are static. The path and transform identify
// the body frame, never a displaced collider frame. shape.localTransform
// carries the collider frame relative to that body. No USD edits or runtime
// resources are created. Results are sorted by path.
[[nodiscard]] std::vector<BoxBody>
readBoxScene(const pxr::UsdStageRefPtr &stage);

} // namespace usd_physics::usd
