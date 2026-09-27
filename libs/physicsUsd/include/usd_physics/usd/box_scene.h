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
  ImportError(ImportErrorCode code, std::string path, const std::string& detail);
  [[nodiscard]] ImportErrorCode code() const noexcept { return code_; }
  [[nodiscard]] const std::string& path() const noexcept { return path_; }
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

// Validates the complete supported subset before returning any declarations.
// Requires Y-up meters/kg, Cube collision on the body prim, explicit positive
// dynamic mass, translate then scale, and identity ancestors (or reset stack).
// No USD edits or runtime resources are created. Results are sorted by path.
[[nodiscard]] std::vector<BoxBody> readBoxScene(const pxr::UsdStageRefPtr& stage);

} // namespace usd_physics::usd
