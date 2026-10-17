#include "usd_physics/usd/box_scene.h"

#include <algorithm>
#include <cmath>
#include <pxr/base/gf/matrix4d.h>
#include <pxr/base/gf/rotation.h>
#include <pxr/base/gf/vec3d.h>
#include <pxr/usd/usd/primRange.h>
#include <pxr/usd/usdGeom/capsule.h>
#include <pxr/usd/usdGeom/cube.h>
#include <pxr/usd/usdGeom/metrics.h>
#include <pxr/usd/usdGeom/pointInstancer.h>
#include <pxr/usd/usdGeom/sphere.h>
#include <pxr/usd/usdGeom/xform.h>
#include <pxr/usd/usdGeom/xformable.h>
#include <pxr/usd/usdPhysics/collisionAPI.h>
#include <pxr/usd/usdPhysics/massAPI.h>
#include <pxr/usd/usdPhysics/metrics.h>
#include <pxr/usd/usdPhysics/rigidBodyAPI.h>
#include <utility>

namespace usd_physics::usd {
namespace {
using Code = ImportErrorCode;

[[noreturn]] void reject(Code code, const pxr::UsdPrim &prim,
                         const char *detail) {
  throw ImportError(code, prim.GetPath().GetString(), detail);
}

template <class T>
T read(const pxr::UsdAttribute &attribute, const pxr::UsdPrim &prim) {
  T value{};
  if (!attribute || attribute.GetNumTimeSamples() != 0 ||
      !attribute.Get(&value)) {
    reject(Code::invalidValue, prim,
           "expected a readable, time-independent value");
  }
  return value;
}

void validateProperties(const pxr::UsdPrim &prim, bool rigid, bool collision,
                        bool mass) {
  for (const auto &schema : prim.GetAppliedSchemas()) {
    const auto &name = schema.GetString();
    if (name.rfind("Physics", 0) == 0 && name != "PhysicsRigidBodyAPI" &&
        name != "PhysicsCollisionAPI" && name != "PhysicsMassAPI") {
      reject(Code::unsupportedDeclaration, prim, "unsupported physics API");
    }
  }
  if (prim.GetTypeName().GetString().rfind("Physics", 0) == 0) {
    reject(Code::unsupportedDeclaration, prim,
           "scenes and joints are not supported yet");
  }
  for (const auto &property : prim.GetAuthoredProperties()) {
    const auto &name = property.GetName().GetString();
    if (name.rfind("physics:", 0) != 0)
      continue;
    const bool admitted = (collision && name == "physics:collisionEnabled") ||
                          (rigid && (name == "physics:rigidBodyEnabled" ||
                                     name == "physics:kinematicEnabled")) ||
                          (mass && name == "physics:mass");
    if (!admitted) {
      reject(Code::unsupportedDeclaration, prim,
             "unsupported physics property or missing API");
    }
  }
}

void validateMatrix(const pxr::GfMatrix4d &matrix, const pxr::UsdPrim &prim) {
  for (int row = 0; row != 4; ++row) {
    for (int column = 0; column != 4; ++column) {
      if (!std::isfinite(matrix[row][column])) {
        reject(Code::invalidValue, prim, "transform must be finite");
      }
    }
  }
  if (matrix[0][3] != 0 || matrix[1][3] != 0 || matrix[2][3] != 0 ||
      matrix[3][3] != 1) {
    reject(Code::unsupportedTransform, prim,
           "perspective transforms are unsupported");
  }
}

pxr::GfMatrix4d localTransform(const pxr::UsdPrim &prim, bool &reset) {
  const pxr::UsdGeomXformable xform(prim);
  reset = false;
  if (!xform)
    return pxr::GfMatrix4d(1.0);
  const auto orderAttr = xform.GetXformOpOrderAttr();
  if (orderAttr.HasAuthoredValueOpinion()) {
    const auto order = read<pxr::VtTokenArray>(orderAttr, prim);
    for (const auto &token : order) {
      const auto &name = token.GetString();
      if (name == "!resetXformStack!")
        continue;
      if (name.rfind("!invert!", 0) == 0) {
        reject(Code::unsupportedTransform, prim,
               "inverse transform ops are unsupported");
      }
      const auto attr = prim.GetAttribute(token);
      if (!pxr::UsdGeomXformOp::IsXformOp(attr)) {
        reject(Code::invalidValue, prim, "unreadable ordered transform op");
      }
      (void)read<pxr::VtValue>(attr, prim);
      validateMatrix(
          pxr::UsdGeomXformOp(attr).GetOpTransform(pxr::UsdTimeCode::Default()),
          prim);
    }
  }
  pxr::GfMatrix4d matrix(1.0);
  if (!xform.GetLocalTransformation(&matrix, &reset,
                                    pxr::UsdTimeCode::Default())) {
    reject(Code::invalidValue, prim, "unreadable transform");
  }
  validateMatrix(matrix, prim);
  return matrix;
}

pxr::GfMatrix4d worldTransform(const pxr::UsdPrim &prim) {
  pxr::GfMatrix4d matrix(1.0);
  for (auto current = prim; current && !current.IsPseudoRoot();
       current = current.GetParent()) {
    bool reset = false;
    matrix *= localTransform(current, reset);
    if (reset)
      break;
  }
  validateMatrix(matrix, prim);
  return matrix;
}

// USD uses row vectors: row lengths are box scales; normalized rows are
// the rigid orientation. A reflected box is equivalent after flipping a row.
pxr::GfVec3d decompose(const pxr::GfMatrix4d &matrix, const pxr::UsdPrim &prim,
                       core::Transform &transform, bool allowScale) {
  pxr::GfMatrix4d rotation(1.0);
  pxr::GfVec3d scale;
  for (int row = 0; row != 3; ++row) {
    const pxr::GfVec3d axis(matrix[row][0], matrix[row][1], matrix[row][2]);
    scale[row] = axis.GetLength();
    if (!std::isfinite(scale[row]) || scale[row] <= 0) {
      reject(Code::invalidValue, prim, "box scale must be nonzero and finite");
    }
    if (!allowScale && std::abs(scale[row] - 1.0) > 1e-9) {
      reject(Code::unsupportedTransform, prim,
             "separate body frames must be rigid");
    }
    rotation.SetRow3(row, axis / scale[row]);
  }
  for (int row = 0; row != 3; ++row) {
    for (int other = row + 1; other != 3; ++other) {
      if (std::abs(pxr::GfDot(rotation.GetRow3(row), rotation.GetRow3(other))) >
          1e-9) {
        reject(Code::unsupportedTransform, prim,
               "sheared boxes are unsupported");
      }
    }
  }
  if (rotation.GetDeterminant3() < 0) {
    if (!allowScale)
      reject(Code::unsupportedTransform, prim,
             "reflected body frames are unsupported");
    rotation.SetRow3(0, -rotation.GetRow3(0));
  }
  const auto quaternion = rotation.ExtractRotationQuat().GetNormalized();
  const auto imaginary = quaternion.GetImaginary();
  transform.translation = {matrix[3][0], matrix[3][1], matrix[3][2]};
  transform.rotation = {quaternion.GetReal(), imaginary[0], imaginary[1],
                        imaginary[2]};
  return scale;
}

void readTransform(const pxr::UsdPrim &body, const pxr::UsdPrim &collider,
                   BoxBody &result) {
  const auto matrix = worldTransform(body);
  auto scale = decompose(matrix, body, result.transform, body == collider);
  if (body != collider) {
    pxr::GfMatrix4d relative(1.0);
    for (auto current = collider; current != body;
         current = current.GetParent()) {
      bool reset = false;
      relative *= localTransform(current, reset);
      if (reset)
        reject(Code::unsupportedTransform, current,
               "collider cannot reset its body frame");
    }
    validateMatrix(relative, collider);
    scale = decompose(relative, collider, result.shape.localTransform, true);
  }
  if (pxr::UsdGeomCube(collider)) {
    const auto size =
        read<double>(pxr::UsdGeomCube(collider).GetSizeAttr(), collider);
    if (!std::isfinite(size) || size <= 0.0) {
      reject(Code::invalidValue, collider,
             "cube size must be positive and finite");
    }
    result.shape.halfExtents = {std::abs(scale[0]) * size / 2.0,
                                std::abs(scale[1]) * size / 2.0,
                                std::abs(scale[2]) * size / 2.0};
  } else {
    const double largest = std::max({scale[0], scale[1], scale[2]});
    if (std::abs(scale[0] - scale[1]) > 1e-9 * largest ||
        std::abs(scale[0] - scale[2]) > 1e-9 * largest) {
      reject(Code::unsupportedTransform, collider,
             "sphere and capsule require uniform scale");
    }
    const pxr::UsdGeomCapsule capsule(collider);
    result.shape.type =
        capsule ? core::ShapeType::capsule : core::ShapeType::sphere;
    const auto radius =
        read<double>(capsule ? capsule.GetRadiusAttr()
                             : pxr::UsdGeomSphere(collider).GetRadiusAttr(),
                     collider);
    if (!std::isfinite(radius) || radius <= 0)
      reject(Code::invalidValue, collider,
             "radius must be positive and finite");
    result.shape.radius = radius * scale[0];
    if (capsule) {
      const auto height = read<double>(capsule.GetHeightAttr(), collider);
      if (!std::isfinite(height) || height < 0)
        reject(Code::invalidValue, collider,
               "height must be non-negative and finite");
      result.shape.halfHeight = height * scale[0] / 2.0;
      const auto axis = read<pxr::TfToken>(capsule.GetAxisAttr(), collider);
      pxr::GfVec3d direction;
      if (axis == pxr::UsdGeomTokens->x)
        direction = pxr::GfVec3d(1, 0, 0);
      else if (axis == pxr::UsdGeomTokens->y)
        direction = pxr::GfVec3d(0, 1, 0);
      else if (axis == pxr::UsdGeomTokens->z)
        direction = pxr::GfVec3d(0, 0, 1);
      else
        reject(Code::invalidValue, collider, "invalid capsule axis");
      const auto q = result.shape.localTransform.rotation;
      pxr::GfMatrix4d rotation(1.0), alignment(1.0);
      rotation.SetRotate(pxr::GfQuatd(q.w, q.x, q.y, q.z));
      alignment.SetRotate(pxr::GfRotation(pxr::GfVec3d(0, 1, 0), direction));
      const auto composed =
          (alignment * rotation).ExtractRotationQuat().GetNormalized();
      const auto imaginary = composed.GetImaginary();
      result.shape.localTransform.rotation = {composed.GetReal(), imaginary[0],
                                              imaginary[1], imaginary[2]};
    }
  }
  try {
    core::validateShapeDescriptor(result.shape);
  } catch (const std::exception &) {
    reject(Code::invalidValue, collider, "invalid scaled shape dimensions");
  }
}

pxr::UsdPrim bodyAncestor(const pxr::UsdPrim &prim) {
  for (auto parent = prim.GetParent(); parent && !parent.IsPseudoRoot();
       parent = parent.GetParent()) {
    if (parent.HasAPI<pxr::UsdPhysicsRigidBodyAPI>())
      return parent;
  }
  return {};
}
} // namespace

ImportError::ImportError(ImportErrorCode code, std::string path,
                         const std::string &detail)
    : std::runtime_error("physicsUsd: " + path + ": " + detail), code_(code),
      path_(std::move(path)) {}

static std::vector<BoxBody> readScene(const pxr::UsdStageRefPtr &stage,
                                      bool boxesOnly) {
  if (!stage)
    throw ImportError(Code::invalidStage, {}, "expected an open Stage");
  std::vector<BoxBody> result;
  std::vector<pxr::UsdPrim> candidates;
  bool unitsChecked = false;
  for (const auto &prim : stage->Traverse(pxr::UsdTraverseInstanceProxies())) {
    const bool rigid = prim.HasAPI<pxr::UsdPhysicsRigidBodyAPI>();
    const bool collision = prim.HasAPI<pxr::UsdPhysicsCollisionAPI>();
    const bool mass = prim.HasAPI<pxr::UsdPhysicsMassAPI>();
    validateProperties(prim, rigid, collision, mass);
    if (!rigid && !collision && !mass)
      continue;
    if (!unitsChecked) {
      if (pxr::UsdGeomGetStageUpAxis(stage) != pxr::UsdGeomTokens->y ||
          !pxr::UsdGeomLinearUnitsAre(pxr::UsdGeomGetStageMetersPerUnit(stage),
                                      1.0) ||
          !pxr::UsdPhysicsMassUnitsAre(
              pxr::UsdPhysicsGetStageKilogramsPerUnit(stage), 1.0)) {
        reject(Code::unsupportedUnits, prim,
               "expected Y-up meters and kilograms");
      }
      unitsChecked = true;
    }
    if (prim.IsInstance() || prim.IsInstanceProxy()) {
      reject(Code::unsupportedDeclaration, prim,
             "instanced physics is not supported");
    }
    const auto owner = bodyAncestor(prim);
    if (rigid && owner) {
      reject(Code::unsupportedDeclaration, prim,
             "nested bodies are unsupported");
    }
    for (auto parent = prim.GetParent(); parent && !parent.IsPseudoRoot();
         parent = parent.GetParent()) {
      if (parent != owner && parent.HasAPI<pxr::UsdPhysicsMassAPI>()) {
        reject(Code::unsupportedDeclaration, prim,
               "nested bodies and inherited mass are not supported");
      }
    }
    if (owner) {
      if (mass)
        reject(Code::unsupportedDeclaration, prim,
               "collider mass overrides are unsupported");
      continue;
    }
    if (!rigid && !collision) {
      reject(Code::unsupportedDeclaration, prim,
             "mass requires a supported body or collider");
    }
    candidates.push_back(prim);
  }
  for (const auto &prim : candidates) {
    const bool rigid = prim.HasAPI<pxr::UsdPhysicsRigidBodyAPI>();
    const bool mass = prim.HasAPI<pxr::UsdPhysicsMassAPI>();
    pxr::UsdPrim collider;
    for (const auto &child : pxr::UsdPrimRange(prim)) {
      if (!child.HasAPI<pxr::UsdPhysicsCollisionAPI>())
        continue;
      if (child != prim && (!rigid || bodyAncestor(child) != prim))
        continue;
      if (collider)
        reject(Code::unsupportedDeclaration, prim,
               "compound colliders are unsupported");
      collider = child;
    }
    if (!collider ||
        !(pxr::UsdGeomCube(collider) ||
          (!boxesOnly &&
           (pxr::UsdGeomSphere(collider) || pxr::UsdGeomCapsule(collider)))) ||
        (prim != collider && !pxr::UsdGeomXform(prim))) {
      reject(Code::unsupportedDeclaration, collider ? collider : prim,
             "expected supported collision geometry on the body or beneath a "
             "body Xform");
    }
    const bool enabled = read<bool>(
        pxr::UsdPhysicsCollisionAPI(collider).GetCollisionEnabledAttr(),
        collider);
    if (!enabled) {
      if (rigid)
        reject(Code::unsupportedDeclaration, prim,
               "a body without an enabled collider is unsupported");
      continue;
    }
    BoxBody body;
    body.path = prim.GetPath().GetString();
    if (rigid) {
      const pxr::UsdPhysicsRigidBodyAPI api(prim);
      if (read<bool>(api.GetKinematicEnabledAttr(), prim)) {
        reject(Code::unsupportedDeclaration, prim,
               "kinematic bodies are not supported");
      }
      if (read<bool>(api.GetRigidBodyEnabledAttr(), prim))
        body.motionType = core::MotionType::dynamicBody;
    }
    if (body.motionType == core::MotionType::dynamicBody) {
      if (!mass)
        reject(Code::unsupportedDeclaration, prim,
               "dynamic boxes require explicit MassAPI mass");
      body.mass = read<float>(pxr::UsdPhysicsMassAPI(prim).GetMassAttr(), prim);
      if (!std::isfinite(body.mass) || body.mass <= 0.0) {
        reject(Code::invalidValue, prim,
               "mass must be explicit, positive and finite; density inference "
               "is unsupported");
      }
    }
    readTransform(prim, collider, body);
    result.push_back(std::move(body));
  }
  for (const auto &prim : stage->Traverse()) {
    const pxr::UsdGeomPointInstancer instancer(prim);
    if (!instancer)
      continue;
    pxr::SdfPathVector prototypes;
    if (!instancer.GetPrototypesRel().GetTargets(&prototypes)) {
      reject(Code::unsupportedDeclaration, prim,
             "unreadable point-instancer prototypes");
    }
    for (const auto &prototype : prototypes) {
      const auto root = stage->GetPrimAtPath(prototype);
      if (!root)
        continue;
      for (const auto &descendant : pxr::UsdPrimRange(root)) {
        if (descendant.HasAPI<pxr::UsdPhysicsRigidBodyAPI>() ||
            descendant.HasAPI<pxr::UsdPhysicsCollisionAPI>()) {
          reject(Code::unsupportedDeclaration, prim,
                 "point-instanced physics is not supported");
        }
      }
    }
  }
  std::sort(result.begin(), result.end(),
            [](const auto &a, const auto &b) { return a.path < b.path; });
  return result;
}

std::vector<BoxBody> readBoxScene(const pxr::UsdStageRefPtr &stage) {
  return readScene(stage, true);
}

std::vector<PrimitiveBody>
readPrimitiveScene(const pxr::UsdStageRefPtr &stage) {
  return readScene(stage, false);
}
} // namespace usd_physics::usd
