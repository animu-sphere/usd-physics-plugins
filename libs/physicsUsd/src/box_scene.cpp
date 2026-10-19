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
#include <pxr/usd/usdPhysics/fixedJoint.h>
#include <pxr/usd/usdPhysics/joint.h>
#include <pxr/usd/usdPhysics/limitAPI.h>
#include <pxr/usd/usdPhysics/massAPI.h>
#include <pxr/usd/usdPhysics/metrics.h>
#include <pxr/usd/usdPhysics/rigidBodyAPI.h>
#include <pxr/usd/usdPhysics/scene.h>
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

void validateUnits(const pxr::UsdStageRefPtr &stage, const pxr::UsdPrim &prim) {
  if (pxr::UsdGeomGetStageUpAxis(stage) != pxr::UsdGeomTokens->y ||
      !pxr::UsdGeomLinearUnitsAre(pxr::UsdGeomGetStageMetersPerUnit(stage),
                                  1.0) ||
      !pxr::UsdPhysicsMassUnitsAre(
          pxr::UsdPhysicsGetStageKilogramsPerUnit(stage), 1.0))
    reject(Code::unsupportedUnits, prim, "expected Y-up meters and kilograms");
}

std::optional<SceneSettings>
readSettings(const pxr::UsdStageRefPtr &stage) {
  if (!stage)
    throw ImportError(Code::invalidStage, {}, "expected an open Stage");
  std::vector<pxr::UsdPrim> scenes;
  for (const auto &prim : stage->Traverse(pxr::UsdTraverseInstanceProxies()))
    if (prim.IsA<pxr::UsdPhysicsScene>())
      scenes.push_back(prim);
  std::sort(scenes.begin(), scenes.end(), [](const auto &a, const auto &b) {
    return a.GetPath() < b.GetPath();
  });
  if (scenes.size() > 1)
    reject(Code::unsupportedDeclaration, scenes[1],
           "multiple physics scenes require explicit consumer selection");
  if (scenes.empty())
    return std::nullopt;
  const auto &prim = scenes.front();
  if (prim.IsInstance() || prim.IsInstanceProxy() ||
      prim.GetTypeName() != pxr::TfToken("PhysicsScene"))
    reject(Code::unsupportedDeclaration, prim, "unsupported physics scene");
  for (const auto &schema : prim.GetAppliedSchemas())
    if (schema.GetString().rfind("Physics", 0) == 0)
      reject(Code::unsupportedDeclaration, prim, "unsupported scene physics API");
  for (const auto &property : prim.GetAuthoredProperties()) {
    const auto &name = property.GetName().GetString();
    if ((name.rfind("physics:", 0) == 0 &&
         name != "physics:gravityDirection" &&
         name != "physics:gravityMagnitude") ||
        name.rfind("limit:", 0) == 0 || name.rfind("drive:", 0) == 0)
      reject(Code::unsupportedDeclaration, prim, "unsupported scene property");
    if (name.rfind("material:binding", 0) == 0 &&
        (name.find(":physics:") != std::string::npos ||
         name == "material:binding:physics"))
      reject(Code::unsupportedDeclaration, prim,
             "physics material bindings are unsupported");
  }
  validateUnits(stage, prim);
  const pxr::UsdPhysicsScene api(prim);
  const auto direction = read<pxr::GfVec3f>(api.GetGravityDirectionAttr(), prim);
  double magnitude = read<float>(api.GetGravityMagnitudeAttr(), prim);
  if (!std::isfinite(direction[0]) || !std::isfinite(direction[1]) ||
      !std::isfinite(direction[2]) || std::isnan(magnitude) ||
      magnitude == std::numeric_limits<double>::infinity())
    reject(Code::invalidValue, prim, "gravity must produce finite acceleration");
  if (magnitude < 0)
    magnitude = 9.81;
  const double length = std::hypot(double(direction[0]), double(direction[1]),
                                   double(direction[2]));
  SceneSettings result;
  result.path = prim.GetPath().GetString();
  result.gravity = length == 0
      ? core::Vector3{0.0, -magnitude, 0.0}
      : core::Vector3{direction[0] / length * magnitude,
                      direction[1] / length * magnitude,
                      direction[2] / length * magnitude};
  return result;
}

const std::array<const char *, 6> jointAxes{"transX", "transY", "transZ",
                                            "rotX",   "rotY",   "rotZ"};

void validateJointProperties(const pxr::UsdPrim &prim) {
  const bool fixed = prim.GetTypeName() == pxr::TfToken("PhysicsFixedJoint");
  if (!fixed && prim.GetTypeName() != pxr::TfToken("PhysicsJoint"))
    reject(Code::unsupportedDeclaration, prim, "unsupported joint type");
  if (prim.HasAPI<pxr::UsdPhysicsRigidBodyAPI>() ||
      prim.HasAPI<pxr::UsdPhysicsCollisionAPI>() ||
      prim.HasAPI<pxr::UsdPhysicsMassAPI>())
    reject(Code::unsupportedDeclaration, prim, "joint cannot also be a body");
  std::vector<std::string> admitted{
      "physics:body0",        "physics:body1",           "physics:localPos0",
      "physics:localPos1",    "physics:localRot0",       "physics:localRot1",
      "physics:jointEnabled", "physics:collisionEnabled"};
  for (const auto &schema : prim.GetAppliedSchemas()) {
    const auto &name = schema.GetString();
    if (name.rfind("Physics", 0) != 0)
      continue;
    bool supported = false;
    for (const auto *axis : jointAxes) {
      if (!fixed && name == std::string("PhysicsLimitAPI:") + axis) {
        supported = true;
        admitted.push_back(std::string("limit:") + axis + ":physics:low");
        admitted.push_back(std::string("limit:") + axis + ":physics:high");
      }
    }
    if (!supported)
      reject(Code::unsupportedDeclaration, prim, "unsupported joint API");
  }
  for (const auto &property : prim.GetAuthoredProperties()) {
    const auto &name = property.GetName().GetString();
    if ((name.rfind("physics:", 0) == 0 || name.rfind("limit:", 0) == 0 ||
         name.rfind("drive:", 0) == 0) &&
        std::find(admitted.begin(), admitted.end(), name) == admitted.end())
      reject(Code::unsupportedDeclaration, prim, "unsupported joint property");
  }
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
    if (name.rfind("material:binding", 0) == 0 &&
        (name.find(":physics:") != std::string::npos ||
         name == "material:binding:physics"))
      reject(Code::unsupportedDeclaration, prim,
             "physics material bindings are unsupported");
    if (name.rfind("physics:", 0) != 0)
      continue;
    const bool admitted =
        (collision && name == "physics:collisionEnabled") ||
        (rigid && (name == "physics:rigidBodyEnabled" ||
                   name == "physics:kinematicEnabled")) ||
        (mass && (name == "physics:mass" || name == "physics:density"));
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

// The supported subset has one collider, meters/kg, and no descendant or
// inherited MassAPI overrides. Compute from the already scaled collision
// geometry, independent of its offset, orientation or capsule axis.
double readMass(const pxr::UsdPrim &prim, const core::ShapeDescriptor &shape) {
  double mass = 0.0, density = 0.0;
  if (prim.HasAPI<pxr::UsdPhysicsMassAPI>()) {
    const pxr::UsdPhysicsMassAPI api(prim);
    mass = read<float>(api.GetMassAttr(), prim);
    density = read<float>(api.GetDensityAttr(), prim);
    if (!std::isfinite(mass) || mass < 0 || !std::isfinite(density) ||
        density < 0)
      reject(Code::invalidValue, prim,
             "mass and density must be finite and non-negative");
  }
  if (mass > 0)
    return mass;
  if (density == 0)
    density = 1000.0; // USD's fallback density, in kg/m^3.
  double volume = 0;
  if (shape.type == core::ShapeType::box) {
    const auto h = shape.halfExtents;
    volume = 8.0 * h.x * h.y * h.z;
  } else {
    const double r = shape.radius;
    const double pi = std::acos(-1.0);
    volume = (4.0 / 3.0) * pi * r * r * r;
    if (shape.type == core::ShapeType::capsule)
      volume += pi * r * r * (2.0 * shape.halfHeight);
  }
  mass = density * volume;
  if (!std::isfinite(mass) || mass <= 0)
    reject(Code::invalidValue, prim,
           "inferred mass must be positive and finite");
  return mass;
}
} // namespace

ImportError::ImportError(ImportErrorCode code, std::string path,
                         const std::string &detail)
    : std::runtime_error("physicsUsd: " + path + ": " + detail), code_(code),
      path_(std::move(path)) {}

static std::vector<BoxBody> readScene(const pxr::UsdStageRefPtr &stage,
                                      bool boxesOnly, bool joints = false) {
  if (!stage)
    throw ImportError(Code::invalidStage, {}, "expected an open Stage");
  std::vector<BoxBody> result;
  std::vector<pxr::UsdPrim> candidates;
  bool unitsChecked = false;
  for (const auto &prim : stage->Traverse(pxr::UsdTraverseInstanceProxies())) {
    const bool rigid = prim.HasAPI<pxr::UsdPhysicsRigidBodyAPI>();
    const bool collision = prim.HasAPI<pxr::UsdPhysicsCollisionAPI>();
    const bool mass = prim.HasAPI<pxr::UsdPhysicsMassAPI>();
    // Scene settings were fully validated by readSettings(). Body-only readers
    // keep their existing rejection of PhysicsScene declarations.
    if (joints && prim.IsA<pxr::UsdPhysicsScene>())
      continue;
    const bool joint = joints && prim.IsA<pxr::UsdPhysicsJoint>();
    if (joint)
      validateJointProperties(prim);
    else
      validateProperties(prim, rigid, collision, mass);
    if (!rigid && !collision && !mass && !joint)
      continue;
    if (!unitsChecked) {
      validateUnits(stage, prim);
      unitsChecked = true;
    }
    if (prim.IsInstance() || prim.IsInstanceProxy()) {
      reject(Code::unsupportedDeclaration, prim,
             "instanced physics is not supported");
    }
    if (joint)
      continue;
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
      const bool kinematic = read<bool>(api.GetKinematicEnabledAttr(), prim);
      if (read<bool>(api.GetRigidBodyEnabledAttr(), prim)) {
        body.motionType = kinematic ? core::MotionType::kinematicBody
                                    : core::MotionType::dynamicBody;
      }
    }
    readTransform(prim, collider, body);
    if (body.motionType == core::MotionType::dynamicBody)
      body.mass = readMass(prim, body.shape);
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
            descendant.HasAPI<pxr::UsdPhysicsCollisionAPI>() ||
            (joints && descendant.IsA<pxr::UsdPhysicsJoint>())) {
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

PhysicsScene readPhysicsScene(const pxr::UsdStageRefPtr &stage) {
  PhysicsScene result;
  result.settings = readSettings(stage);
  result.bodies = readScene(stage, false, true);
  for (const auto &prim : stage->Traverse()) {
    const pxr::UsdPhysicsJoint api(prim);
    if (!api)
      continue;
    Joint joint;
    joint.path = prim.GetPath().GetString();
    joint.type = prim.IsA<pxr::UsdPhysicsFixedJoint>() ? JointType::fixed
                                                       : JointType::sixDof;
    joint.enabled = read<bool>(api.GetJointEnabledAttr(), prim);
    joint.collisionEnabled = read<bool>(api.GetCollisionEnabledAttr(), prim);
    for (int endpoint = 0; endpoint != 2; ++endpoint) {
      pxr::SdfPathVector targets;
      const auto rel = endpoint ? api.GetBody1Rel() : api.GetBody0Rel();
      if (!rel ||
          (!rel.GetForwardedTargets(&targets) && rel.HasAuthoredTargets()) ||
          targets.size() > 1)
        reject(Code::invalidValue, prim, "expected zero or one joint target");
      auto &path = endpoint ? joint.body1 : joint.body0;
      if (!targets.empty()) {
        const auto target = stage->GetPrimAtPath(targets[0]);
        if (!targets[0].IsPrimPath() || !target ||
            !target.HasAPI<pxr::UsdPhysicsRigidBodyAPI>())
          reject(Code::unsupportedDeclaration, prim,
                 "joint target must name an exact rigid body");
        path = targets[0].GetString();
        if (std::none_of(result.bodies.begin(), result.bodies.end(),
                         [&](const auto &body) { return body.path == path; }))
          reject(Code::unsupportedDeclaration, prim,
                 "joint target has no supported body snapshot");
        // Body-only readers bake direct geometry scale into dimensions. Joint
        // coordinates must not silently lose that scale; defer scaled frames.
        core::Transform frame;
        (void)decompose(worldTransform(target), target, frame, false);
      }
      auto &frame = endpoint ? joint.localFrame1 : joint.localFrame0;
      const auto p = read<pxr::GfVec3f>(
          endpoint ? api.GetLocalPos1Attr() : api.GetLocalPos0Attr(), prim);
      const auto q = read<pxr::GfQuatf>(
          endpoint ? api.GetLocalRot1Attr() : api.GetLocalRot0Attr(), prim);
      const auto v = q.GetImaginary();
      const double length =
          std::sqrt(double(q.GetReal()) * q.GetReal() + double(v[0]) * v[0] +
                    double(v[1]) * v[1] + double(v[2]) * v[2]);
      if (!std::isfinite(p[0]) || !std::isfinite(p[1]) ||
          !std::isfinite(p[2]) || !std::isfinite(length) || length <= 0)
        reject(Code::invalidValue, prim, "invalid joint frame");
      frame.translation = {p[0], p[1], p[2]};
      frame.rotation = {q.GetReal() / length, v[0] / length, v[1] / length,
                        v[2] / length};
    }
    if (joint.body0 == joint.body1)
      reject(Code::invalidValue, prim,
             "joint requires distinct endpoints and at least one body");
    for (std::size_t axis = 0; axis != jointAxes.size(); ++axis) {
      const pxr::TfToken name(jointAxes[axis]);
      if (!prim.HasAPI<pxr::UsdPhysicsLimitAPI>(name))
        continue;
      const pxr::UsdPhysicsLimitAPI limit(prim, name);
      const double low = read<float>(limit.GetLowAttr(), prim);
      const double high = read<float>(limit.GetHighAttr(), prim);
      if (std::isnan(low) || std::isnan(high))
        reject(Code::invalidValue, prim, "joint limits cannot be NaN");
      constexpr double radiansPerDegree = 0.017453292519943295769;
      const double scale = axis < 3 ? 1.0 : radiansPerDegree;
      joint.limits[axis] = {true, low * scale, high * scale};
    }
    result.joints.push_back(std::move(joint));
  }
  std::sort(result.joints.begin(), result.joints.end(),
            [](const auto &a, const auto &b) { return a.path < b.path; });
  return result;
}
} // namespace usd_physics::usd
