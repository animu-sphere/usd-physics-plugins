#pragma once

#include "usd_physics/core/descriptors.h"
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

// Cube, Sphere and Capsule variant of the snapshot reader below. Sphere and
// Capsule require uniform composed collider scale (relative tolerance 1e-9).
// Capsule axis X/Y/Z maps to the core's local Y via shape.localTransform;
// height excludes the hemispheres, and zero height is a sphere.
[[nodiscard]] std::vector<PrimitiveBody>
readPrimitiveScene(const pxr::UsdStageRefPtr &stage);

// Validates the complete supported subset before returning any declarations.
// Requires Y-up meters/kg and explicit positive dynamic mass. Supports a
// collision Cube on the body, or one translated/rotated child Cube of a
// rigid body Xform. Composes time-independent affine transforms and reset
// stacks. Cube scales are baked into dimensions; shear and inverse ops fail.
// Separate body frames must be rigid; child reset stacks, compounds and
// kinematic bodies remain unsupported. The path and transform identify the
// body frame, never a displaced collider frame.
// shape.localTransform carries the collider frame relative to that body.
// No USD edits or runtime resources are created. Results are sorted by path.
[[nodiscard]] std::vector<BoxBody>
readBoxScene(const pxr::UsdStageRefPtr &stage);

} // namespace usd_physics::usd
