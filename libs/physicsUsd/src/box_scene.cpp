#include "usd_physics/usd/box_scene.h"

#include <algorithm>
#include <cmath>
#include <utility>
#include <pxr/base/gf/matrix4d.h>
#include <pxr/base/gf/vec3d.h>
#include <pxr/usd/usd/primRange.h>
#include <pxr/usd/usdGeom/cube.h>
#include <pxr/usd/usdGeom/metrics.h>
#include <pxr/usd/usdGeom/pointInstancer.h>
#include <pxr/usd/usdGeom/xformable.h>
#include <pxr/usd/usdPhysics/collisionAPI.h>
#include <pxr/usd/usdPhysics/massAPI.h>
#include <pxr/usd/usdPhysics/metrics.h>
#include <pxr/usd/usdPhysics/rigidBodyAPI.h>

namespace usd_physics::usd {
namespace {
using Code = ImportErrorCode;

[[noreturn]] void reject(Code code, const pxr::UsdPrim& prim, const char* detail) {
  throw ImportError(code, prim.GetPath().GetString(), detail);
}

template<class T> T read(const pxr::UsdAttribute& attribute, const pxr::UsdPrim& prim) {
  T value{};
  if (!attribute || attribute.GetNumTimeSamples() != 0 || !attribute.Get(&value)) {
    reject(Code::invalidValue, prim, "expected a readable, time-independent value");
  }
  return value;
}

void validateProperties(const pxr::UsdPrim& prim, bool rigid, bool collision, bool mass) {
  for (const auto& schema : prim.GetAppliedSchemas()) {
    const auto& name = schema.GetString();
    if (name.rfind("Physics", 0) == 0 && name != "PhysicsRigidBodyAPI" &&
        name != "PhysicsCollisionAPI" && name != "PhysicsMassAPI") {
      reject(Code::unsupportedDeclaration, prim, "unsupported physics API");
    }
  }
  if (prim.GetTypeName().GetString().rfind("Physics", 0) == 0) {
    reject(Code::unsupportedDeclaration, prim, "scenes and joints are not supported yet");
  }
  for (const auto& property : prim.GetAuthoredProperties()) {
    const auto& name = property.GetName().GetString();
    if (name.rfind("physics:", 0) != 0) continue;
    const bool admitted =
        (collision && name == "physics:collisionEnabled") ||
        (rigid && (name == "physics:rigidBodyEnabled" || name == "physics:kinematicEnabled")) ||
        (mass && name == "physics:mass");
    if (!admitted) {
      reject(Code::unsupportedDeclaration, prim, "unsupported physics property or missing API");
    }
  }
}

void readTransform(const pxr::UsdPrim& prim, BoxBody& result) {
  const pxr::UsdGeomXformable xform(prim);
  bool reset = false;
  const auto ops = xform.GetOrderedXformOps(&reset);
  if (ops.empty() || ops.front().IsInverseOp() ||
      ops.front().GetOpType() != pxr::UsdGeomXformOp::TypeTranslate) {
    reject(Code::unsupportedTransform, prim, "expected translate followed by scale ops");
  }
  // GetAs supports the float and half translate ops used by host fixtures.
  pxr::GfVec3d position;
  if (ops.front().GetAttr().GetNumTimeSamples() != 0 ||
      !ops.front().GetAs(&position, pxr::UsdTimeCode::Default())) {
    reject(Code::invalidValue, prim, "unreadable translation");
  }
  result.transform.translation = {position[0], position[1], position[2]};
  pxr::GfVec3d scale(1.0);
  for (std::size_t i = 1; i < ops.size(); ++i) {
    if (ops[i].IsInverseOp() || ops[i].GetOpType() != pxr::UsdGeomXformOp::TypeScale) {
      reject(Code::unsupportedTransform, prim, "expected translate followed by scale ops");
    }
    pxr::GfVec3d value;
    if (ops[i].GetAttr().GetNumTimeSamples() != 0 ||
        !ops[i].GetAs(&value, pxr::UsdTimeCode::Default())) {
      reject(Code::invalidValue, prim, "unreadable or animated scale");
    }
    for (int axis = 0; axis != 3; ++axis) scale[axis] *= value[axis];
  }
  if (!reset) {
    for (auto parent = prim.GetParent(); parent && !parent.IsPseudoRoot(); parent = parent.GetParent()) {
      const pxr::UsdGeomXformable parentXform(parent);
      if (parentXform && (parentXform.TransformMightBeTimeVarying() ||
          !pxr::GfIsClose(parentXform.ComputeLocalToWorldTransform(pxr::UsdTimeCode::Default()),
                          pxr::GfMatrix4d(1.0), 1e-9))) {
        reject(Code::unsupportedTransform, prim, "expected identity ancestors or resetXformStack");
      }
    }
  }
  const auto size = read<double>(pxr::UsdGeomCube(prim).GetSizeAttr(), prim);
  if (!std::isfinite(size) || size <= 0.0) {
    reject(Code::invalidValue, prim, "cube size must be positive and finite");
  }
  result.shape.halfExtents = {std::abs(scale[0]) * size / 2.0,
                             std::abs(scale[1]) * size / 2.0,
                             std::abs(scale[2]) * size / 2.0};
  try {
    core::validateShapeDescriptor(result.shape);
  } catch (const std::exception&) {
    reject(Code::invalidValue, prim, "invalid scaled cube dimensions");
  }
  if (!std::isfinite(position[0]) || !std::isfinite(position[1]) || !std::isfinite(position[2])) {
    reject(Code::invalidValue, prim, "translation must be finite");
  }
}
} // namespace

ImportError::ImportError(ImportErrorCode code, std::string path, const std::string& detail)
    : std::runtime_error("physicsUsd: " + path + ": " + detail), code_(code), path_(std::move(path)) {}

std::vector<BoxBody> readBoxScene(const pxr::UsdStageRefPtr& stage) {
  if (!stage) throw ImportError(Code::invalidStage, {}, "expected an open Stage");
  std::vector<BoxBody> result;
  bool unitsChecked = false;
  for (const auto& prim : stage->Traverse(pxr::UsdTraverseInstanceProxies())) {
    const bool rigid = prim.HasAPI<pxr::UsdPhysicsRigidBodyAPI>();
    const bool collision = prim.HasAPI<pxr::UsdPhysicsCollisionAPI>();
    const bool mass = prim.HasAPI<pxr::UsdPhysicsMassAPI>();
    validateProperties(prim, rigid, collision, mass);
    if (!rigid && !collision && !mass) continue;
    if (!unitsChecked) {
      if (pxr::UsdGeomGetStageUpAxis(stage) != pxr::UsdGeomTokens->y ||
          !pxr::UsdGeomLinearUnitsAre(pxr::UsdGeomGetStageMetersPerUnit(stage), 1.0) ||
          !pxr::UsdPhysicsMassUnitsAre(pxr::UsdPhysicsGetStageKilogramsPerUnit(stage), 1.0)) {
        reject(Code::unsupportedUnits, prim, "expected Y-up meters and kilograms");
      }
      unitsChecked = true;
    }
    if (prim.IsInstance() || prim.IsInstanceProxy()) {
      reject(Code::unsupportedDeclaration, prim, "instanced physics is not supported");
    }
    for (auto parent = prim.GetParent(); parent && !parent.IsPseudoRoot(); parent = parent.GetParent()) {
      if (parent.HasAPI<pxr::UsdPhysicsRigidBodyAPI>() || parent.HasAPI<pxr::UsdPhysicsMassAPI>()) {
        reject(Code::unsupportedDeclaration, prim, "nested bodies and inherited mass are not supported");
      }
    }
    if (!collision || !pxr::UsdGeomCube(prim)) {
      reject(Code::unsupportedDeclaration, prim, "expected CollisionAPI on a Cube at the body prim");
    }
    const bool enabled = read<bool>(pxr::UsdPhysicsCollisionAPI(prim).GetCollisionEnabledAttr(), prim);
    if (!enabled) {
      if (rigid) reject(Code::unsupportedDeclaration, prim, "a body without an enabled collider is unsupported");
      continue;
    }
    BoxBody body;
    body.path = prim.GetPath().GetString();
    if (rigid) {
      const pxr::UsdPhysicsRigidBodyAPI api(prim);
      if (read<bool>(api.GetKinematicEnabledAttr(), prim)) {
        reject(Code::unsupportedDeclaration, prim, "kinematic bodies are not supported");
      }
      if (read<bool>(api.GetRigidBodyEnabledAttr(), prim)) body.motionType = core::MotionType::dynamicBody;
    }
    if (body.motionType == core::MotionType::dynamicBody) {
      if (!mass) reject(Code::unsupportedDeclaration, prim, "dynamic boxes require explicit MassAPI mass");
      body.mass = read<float>(pxr::UsdPhysicsMassAPI(prim).GetMassAttr(), prim);
      if (!std::isfinite(body.mass) || body.mass <= 0.0) {
        reject(Code::invalidValue, prim, "mass must be explicit, positive and finite; density inference is unsupported");
      }
    }
    readTransform(prim, body);
    result.push_back(std::move(body));
  }
  for (const auto& prim : stage->Traverse()) {
    const pxr::UsdGeomPointInstancer instancer(prim);
    if (!instancer) continue;
    pxr::SdfPathVector prototypes;
    if (!instancer.GetPrototypesRel().GetTargets(&prototypes)) {
      reject(Code::unsupportedDeclaration, prim, "unreadable point-instancer prototypes");
    }
    for (const auto& prototype : prototypes) {
      for (const auto& body : result) {
        if (pxr::SdfPath(body.path).HasPrefix(prototype)) {
          reject(Code::unsupportedDeclaration, prim, "point-instanced physics is not supported");
        }
      }
    }
  }
  std::sort(result.begin(), result.end(), [](const auto& a, const auto& b) { return a.path < b.path; });
  return result;
}
} // namespace usd_physics::usd
