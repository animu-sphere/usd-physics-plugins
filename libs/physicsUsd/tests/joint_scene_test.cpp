#include "usd_physics/usd/box_scene.h"
#include <cmath>
#include <functional>
#include <iostream>
#include <limits>
#include <pxr/base/gf/quatf.h>
#include <pxr/usd/usdGeom/cube.h>
#include <pxr/usd/usdGeom/metrics.h>
#include <pxr/usd/usdGeom/xform.h>
#include <pxr/usd/usdPhysics/collisionAPI.h>
#include <pxr/usd/usdPhysics/driveAPI.h>
#include <pxr/usd/usdPhysics/fixedJoint.h>
#include <pxr/usd/usdPhysics/joint.h>
#include <pxr/usd/usdPhysics/limitAPI.h>
#include <pxr/usd/usdPhysics/massAPI.h>
#include <pxr/usd/usdPhysics/revoluteJoint.h>
#include <pxr/usd/usdPhysics/rigidBodyAPI.h>

using namespace usd_physics;
using Code = usd::ImportErrorCode;

void check(bool value, const char *message) {
  if (!value)
    throw std::runtime_error(message);
}

pxr::UsdStageRefPtr fixture() {
  auto stage = pxr::UsdStage::CreateInMemory();
  pxr::UsdGeomSetStageUpAxis(stage, pxr::UsdGeomTokens->y);
  pxr::UsdGeomSetStageMetersPerUnit(stage, 1.0);
  const auto parent = pxr::UsdGeomXform::Define(stage, pxr::SdfPath("/World"));
  parent.AddTranslateOp().Set(pxr::GfVec3d(10, 20, 30));
  parent.AddRotateZOp().Set(90.f);
  for (const char *name : {"/World/B", "/World/A"}) {
    auto body = pxr::UsdGeomXform::Define(stage, pxr::SdfPath(name));
    pxr::UsdPhysicsRigidBodyAPI::Apply(body.GetPrim());
    pxr::UsdPhysicsMassAPI::Apply(body.GetPrim()).CreateMassAttr().Set(1.f);
    auto shape = pxr::UsdGeomCube::Define(
        stage, body.GetPath().AppendChild(pxr::TfToken("Shape")));
    shape.AddTranslateOp().Set(pxr::GfVec3d(5, 6, 7));
    pxr::UsdPhysicsCollisionAPI::Apply(shape.GetPrim());
  }
  auto joint = pxr::UsdPhysicsJoint::Define(stage, pxr::SdfPath("/Joint"));
  joint.CreateBody0Rel().SetTargets({pxr::SdfPath("/World/A")});
  joint.CreateBody1Rel().SetTargets({pxr::SdfPath("/World/B")});
  joint.CreateLocalPos0Attr().Set(pxr::GfVec3f(1, 2, 3));
  joint.CreateLocalPos1Attr().Set(pxr::GfVec3f(-1, -2, -3));
  joint.CreateLocalRot0Attr().Set(pxr::GfQuatf(2, 0, 0, 2));
  auto limit =
      pxr::UsdPhysicsLimitAPI::Apply(joint.GetPrim(), pxr::TfToken("rotZ"));
  limit.CreateLowAttr().Set(-90.f);
  limit.CreateHighAttr().Set(180.f);
  return stage;
}

void rejects(const std::function<void(const pxr::UsdStageRefPtr &)> &change,
             Code code, const char *path = "/Joint") {
  auto stage = fixture();
  change(stage);
  try {
    (void)usd::readPhysicsScene(stage);
  } catch (const usd::ImportError &error) {
    check(error.code() == code && error.path() == path,
          "typed rejection code/path");
    return;
  }
  throw std::runtime_error("expected joint rejection");
}

int main() {
  try {
    auto stage = fixture();
    std::string before, after;
    stage->GetRootLayer()->ExportToString(&before);
    auto scene = usd::readPhysicsScene(stage);
    stage->GetRootLayer()->ExportToString(&after);
    check(before == after, "snapshot must not edit USD");
    check(scene.bodies.size() == 2 && scene.joints.size() == 1 &&
              scene.bodies[0].path == "/World/A",
          "body identity and sorting");
    const auto &joint = scene.joints[0];
    check(joint.type == usd::JointType::sixDof && joint.body0 == "/World/A" &&
              joint.body1 == "/World/B" && joint.enabled &&
              !joint.collisionEnabled,
          "joint endpoints and defaults");
    check(joint.localFrame0.translation == core::Vector3{1, 2, 3} &&
              joint.localFrame1.translation == core::Vector3{-1, -2, -3} &&
              std::abs(joint.localFrame0.rotation.z - std::sqrt(.5)) < 1e-12 &&
              joint.localFrame1.rotation == core::Quaternion{},
          "local frames stay relative to bodies, not colliders or world");
    check(!joint.limits[0].applied && joint.limits[5].applied &&
              std::abs(joint.limits[5].low + std::acos(-1.) / 2) < 1e-12 &&
              std::abs(joint.limits[5].high - std::acos(-1.)) < 1e-12,
          "free axis and angular conversion");
    const auto api =
        pxr::UsdPhysicsJoint(stage->GetPrimAtPath(pxr::SdfPath("/Joint")));
    api.CreateBody0Rel().SetTargets({});
    api.CreateJointEnabledAttr().Set(false);
    api.CreateCollisionEnabledAttr().Set(true);
    for (const char *axis : {"transX", "transY", "transZ", "rotX", "rotY"}) {
      const auto limit =
          pxr::UsdPhysicsLimitAPI::Apply(api.GetPrim(), pxr::TfToken(axis));
      if (std::string(axis) == "transX") {
        limit.CreateLowAttr().Set(-.25f);
        limit.CreateHighAttr().Set(.5f);
      } else if (std::string(axis) == "transY") {
        limit.CreateLowAttr().Set(1.f);
        limit.CreateHighAttr().Set(-1.f);
      } else if (std::string(axis) == "transZ") {
        limit.CreateLowAttr().Set(.25f);
        limit.CreateHighAttr().Set(.25f);
      }
    }
    const auto fixed =
        pxr::UsdPhysicsFixedJoint::Define(stage, pxr::SdfPath("/AFixed"));
    fixed.CreateBody0Rel().SetTargets({pxr::SdfPath("/World/B")});
    scene = usd::readPhysicsScene(stage);
    check(scene.joints[0].path == "/AFixed" &&
              scene.joints[0].type == usd::JointType::fixed,
          "fixed world joint and deterministic joint order");
    const auto &changed = scene.joints[1];
    check(changed.body0.empty() && !changed.enabled &&
              changed.collisionEnabled && changed.limits[0].low == -.25 &&
              changed.limits[0].high == .5 &&
              changed.limits[1].low > changed.limits[1].high &&
              changed.limits[2].low == changed.limits[2].high &&
              std::isinf(changed.limits[3].low) && changed.limits[3].low < 0 &&
              std::isinf(changed.limits[3].high) && changed.limits[3].high > 0,
          "world anchor, disabled flag, linear, locked, equal and default "
          "infinite limits");
    for (bool boxes : {false, true}) {
      try {
        if (boxes)
          (void)usd::readBoxScene(stage);
        else
          (void)usd::readPrimitiveScene(stage);
        throw std::runtime_error("body-only readers must still reject joints");
      } catch (const usd::ImportError &error) {
        check(error.code() == Code::unsupportedDeclaration, "legacy rejection");
      }
    }
    auto forwardedStage = fixture();
    const auto forwarding = forwardedStage->DefinePrim(pxr::SdfPath("/Link"))
                                .CreateRelationship(pxr::TfToken("endpoint"));
    forwarding.SetTargets({pxr::SdfPath("/World/A")});
    pxr::UsdPhysicsJoint(forwardedStage->GetPrimAtPath(pxr::SdfPath("/Joint")))
        .CreateBody0Rel()
        .SetTargets({forwarding.GetPath()});
    check(usd::readPhysicsScene(forwardedStage).joints[0].body0 == "/World/A",
          "forwarded endpoint relationship");
    rejects(
        [](auto s) {
          s->GetPrimAtPath(pxr::SdfPath("/Joint"))
              .SetTypeName(pxr::TfToken("PhysicsFixedJoint"));
        },
        Code::unsupportedDeclaration);
    rejects(
        [](auto s) {
          s->GetPrimAtPath(pxr::SdfPath("/Joint"))
              .CreateAttribute(pxr::TfToken("limit:transX:physics:low"),
                               pxr::SdfValueTypeNames->Float)
              .Set(0.f);
        },
        Code::unsupportedDeclaration);
    rejects(
        [](auto s) {
          pxr::UsdPhysicsJoint(s->GetPrimAtPath(pxr::SdfPath("/Joint")))
              .CreateBody0Rel()
              .SetTargets({pxr::SdfPath("/Missing")});
        },
        Code::unsupportedDeclaration);
    rejects(
        [](auto s) {
          pxr::UsdPhysicsJoint(s->GetPrimAtPath(pxr::SdfPath("/Joint")))
              .CreateBody0Rel()
              .SetTargets({pxr::SdfPath("/World/A/Shape")});
        },
        Code::unsupportedDeclaration);
    rejects(
        [](auto s) {
          pxr::UsdPhysicsJoint(s->GetPrimAtPath(pxr::SdfPath("/Joint")))
              .CreateBody0Rel()
              .SetTargets({pxr::SdfPath("/World/A"), pxr::SdfPath("/World/B")});
        },
        Code::invalidValue);
    rejects(
        [](auto s) {
          pxr::UsdPhysicsJoint(s->GetPrimAtPath(pxr::SdfPath("/Joint")))
              .CreateBody0Rel()
              .SetTargets({pxr::SdfPath("/World/B")});
        },
        Code::invalidValue);
    rejects(
        [](auto s) {
          const pxr::UsdPhysicsJoint j(
              s->GetPrimAtPath(pxr::SdfPath("/Joint")));
          j.CreateBody0Rel().SetTargets({});
          j.CreateBody1Rel().SetTargets({});
        },
        Code::invalidValue);
    rejects(
        [](auto s) {
          pxr::UsdPhysicsJoint(s->GetPrimAtPath(pxr::SdfPath("/Joint")))
              .CreateLocalRot1Attr()
              .Set(pxr::GfQuatf(0, 0, 0, 0));
        },
        Code::invalidValue);
    rejects(
        [](auto s) {
          pxr::UsdPhysicsJoint(s->GetPrimAtPath(pxr::SdfPath("/Joint")))
              .CreateLocalPos0Attr()
              .Set(pxr::GfVec3f(std::numeric_limits<float>::infinity()));
        },
        Code::invalidValue);
    rejects(
        [](auto s) {
          pxr::UsdPhysicsJoint(s->GetPrimAtPath(pxr::SdfPath("/Joint")))
              .CreateLocalPos0Attr()
              .Set(pxr::GfVec3f(0), 1.0);
        },
        Code::invalidValue);
    rejects(
        [](auto s) {
          pxr::UsdPhysicsJoint(s->GetPrimAtPath(pxr::SdfPath("/Joint")))
              .CreateJointEnabledAttr()
              .Set(true, 1.0);
        },
        Code::invalidValue);
    rejects(
        [](auto s) {
          pxr::UsdPhysicsLimitAPI(s->GetPrimAtPath(pxr::SdfPath("/Joint")),
                                  pxr::TfToken("rotZ"))
              .CreateHighAttr()
              .Set(1.f, 1.0);
        },
        Code::invalidValue);
    rejects(
        [](auto s) {
          pxr::UsdPhysicsLimitAPI(s->GetPrimAtPath(pxr::SdfPath("/Joint")),
                                  pxr::TfToken("rotZ"))
              .CreateHighAttr()
              .Set(std::numeric_limits<float>::quiet_NaN());
        },
        Code::invalidValue);
    rejects(
        [](auto s) {
          pxr::UsdPhysicsLimitAPI::Apply(
              s->GetPrimAtPath(pxr::SdfPath("/Joint")),
              pxr::TfToken("distance"));
        },
        Code::unsupportedDeclaration);
    rejects(
        [](auto s) {
          pxr::UsdPhysicsDriveAPI::Apply(
              s->GetPrimAtPath(pxr::SdfPath("/Joint")), pxr::TfToken("rotX"));
        },
        Code::unsupportedDeclaration);
    rejects(
        [](auto s) {
          pxr::UsdPhysicsJoint(s->GetPrimAtPath(pxr::SdfPath("/Joint")))
              .CreateBreakForceAttr()
              .Set(1.f);
        },
        Code::unsupportedDeclaration);
    rejects(
        [](auto s) {
          s->GetPrimAtPath(pxr::SdfPath("/Joint"))
              .SetTypeName(pxr::TfToken("PhysicsRevoluteJoint"));
        },
        Code::unsupportedDeclaration);
    rejects(
        [](auto s) {
          pxr::UsdGeomXformable(s->GetPrimAtPath(pxr::SdfPath("/World/A")))
              .AddScaleOp()
              .Set(pxr::GfVec3f(2));
        },
        Code::unsupportedTransform, "/World/A");
    rejects([](auto s) { pxr::UsdGeomSetStageMetersPerUnit(s, .01); },
            Code::unsupportedUnits, "/World/B");
    auto empty = pxr::UsdStage::CreateInMemory();
    check(usd::readPhysicsScene(empty).bodies.empty() &&
              usd::readPhysicsScene(empty).joints.empty(),
          "empty scene");
    return 0;
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
