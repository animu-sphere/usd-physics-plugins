#include "usd_physics/usd/box_scene.h"
#include <cmath>
#include <functional>
#include <iostream>
#include <limits>
#include <pxr/base/gf/matrix4d.h>
#include <pxr/base/gf/quatf.h>
#include <pxr/base/gf/rotation.h>
#include <pxr/base/gf/vec3d.h>
#include <pxr/base/gf/vec3f.h>
#include <pxr/usd/sdf/layer.h>
#include <pxr/usd/usdGeom/cube.h>
#include <pxr/usd/usdGeom/metrics.h>
#include <pxr/usd/usdGeom/pointInstancer.h>
#include <pxr/usd/usdGeom/xform.h>
#include <pxr/usd/usdPhysics/collisionAPI.h>
#include <pxr/usd/usdPhysics/massAPI.h>
#include <pxr/usd/usdPhysics/metrics.h>
#include <pxr/usd/usdPhysics/rigidBodyAPI.h>
#include <pxr/usd/usdPhysics/scene.h>

using namespace usd_physics;
using Code = usd::ImportErrorCode;

pxr::UsdStageRefPtr fixture() {
  auto stage = pxr::UsdStage::CreateInMemory();
  pxr::UsdGeomSetStageUpAxis(stage, pxr::UsdGeomTokens->y);
  pxr::UsdGeomSetStageMetersPerUnit(stage, 1.0);
  auto cube = pxr::UsdGeomCube::Define(stage, pxr::SdfPath("/World/Box"));
  cube.AddTranslateOp(pxr::UsdGeomXformOp::PrecisionFloat)
      .Set(pxr::GfVec3f(0, 3, 0));
  cube.AddScaleOp().Set(pxr::GfVec3f(-0.5, 1, 2));
  pxr::UsdPhysicsCollisionAPI::Apply(cube.GetPrim());
  pxr::UsdPhysicsRigidBodyAPI::Apply(cube.GetPrim());
  pxr::UsdPhysicsMassAPI::Apply(cube.GetPrim()).CreateMassAttr().Set(70.0f);
  return stage;
}

void check(bool condition, const char *message) {
  if (!condition)
    throw std::runtime_error(message);
}

pxr::UsdStageRefPtr childFixture() {
  auto stage = pxr::UsdStage::CreateInMemory();
  pxr::UsdGeomSetStageUpAxis(stage, pxr::UsdGeomTokens->y);
  pxr::UsdGeomSetStageMetersPerUnit(stage, 1.0);
  auto parent = pxr::UsdGeomXform::Define(stage, pxr::SdfPath("/World"));
  parent.AddTranslateOp().Set(pxr::GfVec3d(10, 0, 0));
  parent.AddRotateZOp().Set(90.0f);
  auto body = pxr::UsdGeomXform::Define(stage, pxr::SdfPath("/World/Body"));
  body.AddTranslateOp().Set(pxr::GfVec3d(2, 0, 0));
  body.AddOrientOp(pxr::UsdGeomXformOp::PrecisionFloat)
      .Set(pxr::GfQuatf(pxr::GfRotation(pxr::GfVec3d(0, 1, 0), 30).GetQuat()));
  pxr::UsdPhysicsRigidBodyAPI::Apply(body.GetPrim());
  pxr::UsdPhysicsMassAPI::Apply(body.GetPrim()).CreateMassAttr().Set(4.0f);
  auto collider =
      pxr::UsdGeomCube::Define(stage, pxr::SdfPath("/World/Body/Shape"));
  collider.CreateSizeAttr().Set(4.0);
  collider.AddScaleOp().Set(pxr::GfVec3f(-0.5f, 1, 2));
  pxr::UsdPhysicsCollisionAPI::Apply(collider.GetPrim());
  return stage;
}

void checkGeometry(const pxr::UsdStageRefPtr &stage,
                   const pxr::SdfPath &colliderPath) {
  std::string before, after;
  stage->GetRootLayer()->ExportToString(&before);
  const auto boxes = usd::readBoxScene(stage);
  stage->GetRootLayer()->ExportToString(&after);
  check(before == after && boxes.size() == 1, "read-only single-body snapshot");
  const auto &box = boxes[0];
  const auto q = box.transform.rotation;
  pxr::GfMatrix4d runtime(1.0);
  runtime.SetRotate(pxr::GfQuatd(q.w, q.x, q.y, q.z));
  runtime.SetTranslateOnly(pxr::GfVec3d(box.transform.translation.x,
                                        box.transform.translation.y,
                                        box.transform.translation.z));
  const auto local = box.shape.localTransform;
  pxr::GfMatrix4d geometry(1.0);
  geometry.SetRotate(pxr::GfQuatd(local.rotation.w, local.rotation.x,
                                  local.rotation.y, local.rotation.z));
  geometry.SetTranslateOnly(pxr::GfVec3d(
      local.translation.x, local.translation.y, local.translation.z));
  runtime = geometry * runtime;
  const pxr::UsdGeomCube collider(stage->GetPrimAtPath(colliderPath));
  double size = 0;
  collider.GetSizeAttr().Get(&size);
  const auto expected =
      collider.ComputeLocalToWorldTransform(pxr::UsdTimeCode::Default());
  // Compare all eight corners as unordered sets, including reflected scales.
  for (int corner = 0; corner != 8; ++corner) {
    const auto actual = runtime.Transform(
        pxr::GfVec3d((corner & 1 ? 1 : -1) * box.shape.halfExtents.x,
                     (corner & 2 ? 1 : -1) * box.shape.halfExtents.y,
                     (corner & 4 ? 1 : -1) * box.shape.halfExtents.z));
    bool matched = false;
    for (int candidate = 0; candidate != 8; ++candidate) {
      const auto point =
          expected.Transform(pxr::GfVec3d((candidate & 1 ? 1 : -1) * size / 2,
                                          (candidate & 2 ? 1 : -1) * size / 2,
                                          (candidate & 4 ? 1 : -1) * size / 2));
      if (pxr::GfIsClose(actual, point, 1e-8))
        matched = true;
    }
    check(matched, "runtime box corners must match composed USD geometry");
  }
}

void checkHierarchy() {
  const pxr::SdfPath bodyPath("/World/Body");
  const pxr::SdfPath shapePath("/World/Body/Shape");
  auto stage = childFixture();
  const auto boxes = usd::readBoxScene(stage);
  check(boxes.size() == 1 && boxes[0].path == bodyPath.GetString() &&
            boxes[0].mass == 4 &&
            boxes[0].motionType == core::MotionType::dynamicBody &&
            boxes[0].shape.halfExtents == core::Vector3{1, 2, 4} &&
            std::abs(boxes[0].transform.translation.x - 10) < 1e-9 &&
            std::abs(boxes[0].transform.translation.y - 2) < 1e-9,
        "body identity, mass, child scale and composed translation");
  checkGeometry(stage, shapePath);
  // Keep the body origin while composing an offset, rotation and reflection.
  auto shape = pxr::UsdGeomXformable(stage->GetPrimAtPath(shapePath));
  shape.ClearXformOpOrder();
  shape.AddTranslateOp().Set(pxr::GfVec3d(1, -2, 3));
  shape.AddRotateXYZOp().Set(pxr::GfVec3f(17, 31, -23));
  shape.AddScaleOp(pxr::UsdGeomXformOp::PrecisionFloat, pxr::TfToken("local"))
      .Set(pxr::GfVec3f(-0.5f, 1, 2));
  checkGeometry(stage, shapePath);
  const auto offset = usd::readBoxScene(stage)[0];
  check(offset.path == boxes[0].path &&
            offset.transform == boxes[0].transform &&
            offset.shape.localTransform.translation == core::Vector3{1, -2, 3},
        "collider pose must not move the body frame");
  // Local transforms also compose across intermediate Xforms.
  stage = childFixture();
  stage->RemovePrim(shapePath);
  auto intermediate =
      pxr::UsdGeomXform::Define(stage, pxr::SdfPath("/World/Body/Offset"));
  intermediate.AddTranslateOp().Set(pxr::GfVec3d(2, 3, 4));
  intermediate.AddRotateXOp().Set(41.0f);
  auto nested =
      pxr::UsdGeomCube::Define(stage, pxr::SdfPath("/World/Body/Offset/Shape"));
  nested.AddTranslateOp().Set(pxr::GfVec3d(-1, 2, 0));
  nested.AddRotateYOp().Set(29.0f);
  nested.AddScaleOp().Set(pxr::GfVec3f(0.5f, 1, 2));
  pxr::UsdPhysicsCollisionAPI::Apply(nested.GetPrim());
  checkGeometry(stage, nested.GetPath());
  stage = childFixture();
  pxr::UsdPhysicsRigidBodyAPI(stage->GetPrimAtPath(bodyPath))
      .CreateRigidBodyEnabledAttr()
      .Set(false);
  check(usd::readBoxScene(stage)[0].motionType == core::MotionType::staticBody,
        "disabled parent body remains static");
  // A reset stops both composition and validation of animated ancestors.
  stage = childFixture();
  auto body = pxr::UsdGeomXformable(stage->GetPrimAtPath(bodyPath));
  body.SetResetXformStack(true);
  stage->GetPrimAtPath(pxr::SdfPath("/World"))
      .GetAttribute(pxr::TfToken("xformOp:rotateZ"))
      .Set(10.0f, 1);
  checkGeometry(stage, shapePath);
  check(std::abs(usd::readBoxScene(stage)[0].transform.translation.x - 2) <
            1e-9,
        "reset retains local body translation");

  using Edit = std::function<void(const pxr::UsdStageRefPtr &,
                                  const pxr::UsdPrim &, const pxr::UsdPrim &)>;
  const std::vector<std::pair<Code, Edit>> invalid{
      // Rotation before an existing nonuniform scale introduces shear.
      {Code::unsupportedTransform,
       [](auto, auto, auto p) {
         pxr::UsdGeomXformable(p).AddRotateYOp().Set(30.0f);
       }},
      {Code::unsupportedTransform,
       [](auto, auto, auto p) {
         pxr::UsdGeomXformable(p).SetResetXformStack(true);
       }},
      {Code::unsupportedTransform,
       [](auto, auto b, auto) {
         pxr::UsdGeomXformable(b).AddScaleOp().Set(pxr::GfVec3f(2));
       }},
      {Code::invalidValue,
       [](auto s, auto, auto) {
         s->GetPrimAtPath(pxr::SdfPath("/World"))
             .GetAttribute(pxr::TfToken("xformOp:rotateZ"))
             .Set(0.0f, 1);
       }},
      {Code::invalidValue,
       [](auto, auto, auto p) {
         p.GetAttribute(pxr::TfToken("xformOp:scale")).Set(pxr::GfVec3f(1), 1);
       }},
      {Code::invalidValue,
       [](auto, auto, auto p) {
         p.GetAttribute(pxr::TfToken("xformOp:scale")).Set(pxr::GfVec3f(0));
       }},
      {Code::unsupportedDeclaration,
       [](auto, auto, auto p) { pxr::UsdPhysicsMassAPI::Apply(p); }},
      {Code::unsupportedDeclaration,
       [](auto, auto, auto p) { p.SetTypeName(pxr::TfToken("Sphere")); }},
      {Code::unsupportedDeclaration,
       [](auto, auto, auto p) {
         pxr::UsdPhysicsCollisionAPI(p).CreateCollisionEnabledAttr().Set(false);
       }},
      {Code::unsupportedDeclaration,
       [](auto s, auto, auto) {
         s->RemovePrim(pxr::SdfPath("/World/Body/Shape"));
       }},
      {Code::unsupportedDeclaration,
       [](auto s, auto, auto) {
         auto second =
             pxr::UsdGeomCube::Define(s, pxr::SdfPath("/World/Body/Other"));
         pxr::UsdPhysicsCollisionAPI::Apply(second.GetPrim());
       }},
      {Code::unsupportedDeclaration,
       [](auto, auto b, auto) { pxr::UsdPhysicsCollisionAPI::Apply(b); }},
      {Code::unsupportedDeclaration,
       [](auto, auto, auto p) { pxr::UsdPhysicsRigidBodyAPI::Apply(p); }},
      {Code::unsupportedDeclaration,
       [](auto s, auto, auto) {
         pxr::UsdPhysicsMassAPI::Apply(
             s->GetPrimAtPath(pxr::SdfPath("/World")));
       }},
      {Code::unsupportedDeclaration,
       [](auto s, auto, auto) {
         pxr::UsdGeomPointInstancer::Define(s, pxr::SdfPath("/Points"))
             .CreatePrototypesRel()
             .SetTargets({pxr::SdfPath("/World/Body/Shape")});
       }},
      {Code::unsupportedTransform,
       [](auto, auto b, auto) {
         pxr::UsdGeomXformable(b).AddTranslateOp(
             pxr::UsdGeomXformOp::PrecisionDouble, pxr::TfToken("inverse"),
             true);
       }},
  };
  for (std::size_t i = 0; i != invalid.size(); ++i) {
    stage = childFixture();
    invalid[i].second(stage, stage->GetPrimAtPath(bodyPath),
                      stage->GetPrimAtPath(shapePath));
    try {
      (void)usd::readBoxScene(stage);
      throw std::runtime_error("invalid child fixture accepted: " +
                               std::to_string(i));
    } catch (const usd::ImportError &error) {
      check(error.code() == invalid[i].first && !error.path().empty(),
            "child diagnostic code/path");
    }
  }
}

int main() {
  try {
    checkHierarchy();
    auto stage = fixture();
    std::string before, after;
    stage->GetRootLayer()->ExportToString(&before);
    const auto boxes = usd::readBoxScene(stage);
    stage->GetRootLayer()->ExportToString(&after);
    check(before == after, "import must not edit USD");
    check(boxes.size() == 1 && boxes[0].path == "/World/Box" &&
              boxes[0].mass == 70 &&
              boxes[0].motionType == core::MotionType::dynamicBody &&
              boxes[0].shape.halfExtents == core::Vector3{0.5, 1, 2} &&
              boxes[0].transform.translation == core::Vector3{0, 3, 0},
          "box conversion");
    auto box = stage->GetPrimAtPath(pxr::SdfPath("/World/Box"));
    pxr::UsdPhysicsRigidBodyAPI(box).CreateRigidBodyEnabledAttr().Set(false);
    check(usd::readBoxScene(stage)[0].motionType ==
              core::MotionType::staticBody,
          "disabled rigid body is static");
    box.RemoveAPI<pxr::UsdPhysicsRigidBodyAPI>();
    box.RemoveProperty(pxr::TfToken("physics:rigidBodyEnabled"));
    check(usd::readBoxScene(stage)[0].motionType ==
              core::MotionType::staticBody,
          "collision-only is static");
    pxr::UsdPhysicsCollisionAPI(box).CreateCollisionEnabledAttr().Set(false);
    check(usd::readBoxScene(stage).empty(),
          "disabled static collision is skipped");
    stage = fixture();
    auto floor = pxr::UsdGeomCube::Define(stage, pxr::SdfPath("/A"));
    floor.AddTranslateOp().Set(pxr::GfVec3d(0, -1, 0));
    pxr::UsdPhysicsCollisionAPI::Apply(floor.GetPrim());
    check(usd::readBoxScene(stage)[0].path == "/A",
          "path order is deterministic");
    auto parent = pxr::UsdGeomXform::Define(stage, pxr::SdfPath("/World"));
    parent.AddTranslateOp().Set(pxr::GfVec3d(4, 0, 0));
    pxr::UsdGeomXformable(stage->GetPrimAtPath(pxr::SdfPath("/World/Box")))
        .SetResetXformStack(true);
    check(usd::readBoxScene(stage)[1].transform.translation.x == 0,
          "reset stack ignores ancestors");
    // Direct Cubes retain scale support under composed, rotated ancestors.
    stage = fixture();
    parent = pxr::UsdGeomXform::Define(stage, pxr::SdfPath("/World"));
    parent.AddTranslateOp().Set(pxr::GfVec3d(4, 0, 1));
    parent.AddRotateXOp().Set(40.0f);
    checkGeometry(stage, pxr::SdfPath("/World/Box"));
    stage = fixture();
    auto direct =
        pxr::UsdGeomXformable(stage->GetPrimAtPath(pxr::SdfPath("/World/Box")));
    direct.ClearXformOpOrder();
    pxr::GfMatrix4d composed(1.0);
    composed.SetRotate(
        pxr::GfRotation(pxr::GfVec3d(1, 2, 3).GetNormalized(), 27));
    composed.SetTranslateOnly(pxr::GfVec3d(3, -2, 7));
    direct.AddTransformOp().Set(composed);
    checkGeometry(stage, pxr::SdfPath("/World/Box"));
    pxr::UsdGeomXformable(stage->GetPrimAtPath(pxr::SdfPath("/World/Box")))
        .ClearXformOpOrder();
    checkGeometry(stage, pxr::SdfPath("/World/Box"));

    using Edit =
        std::function<void(const pxr::UsdStageRefPtr &, const pxr::UsdPrim &)>;
    const std::vector<std::pair<Code, Edit>> invalid{
        {Code::unsupportedUnits,
         [](auto s, auto) { pxr::UsdGeomSetStageMetersPerUnit(s, .01); }},
        {Code::unsupportedUnits,
         [](auto s, auto) {
           pxr::UsdGeomSetStageUpAxis(s, pxr::UsdGeomTokens->z);
         }},
        {Code::unsupportedUnits,
         [](auto s, auto) {
           pxr::UsdPhysicsSetStageKilogramsPerUnit(s, .001);
         }},
        {Code::invalidValue,
         [](auto, auto p) {
           pxr::UsdPhysicsMassAPI(p).CreateMassAttr().Set(0.0f);
         }},
        {Code::invalidValue,
         [](auto, auto p) {
           pxr::UsdPhysicsMassAPI(p).CreateMassAttr().Set(-1.0f);
         }},
        {Code::invalidValue,
         [](auto, auto p) {
           pxr::UsdGeomCube(p).CreateSizeAttr().Set(
               std::numeric_limits<double>::infinity());
         }},
        {Code::invalidValue,
         [](auto, auto p) {
           p.GetAttribute(pxr::TfToken("xformOp:scale")).Set(pxr::GfVec3f(0));
         }},
        {Code::invalidValue,
         [](auto, auto p) {
           p.GetAttribute(pxr::TfToken("xformOp:translate"))
               .Set(pxr::GfVec3f(2), 1);
         }},
        {Code::unsupportedDeclaration,
         [](auto, auto p) {
           pxr::UsdPhysicsRigidBodyAPI(p).CreateKinematicEnabledAttr().Set(
               true);
         }},
        {Code::unsupportedDeclaration,
         [](auto, auto p) {
           pxr::UsdPhysicsRigidBodyAPI(p).CreateVelocityAttr().Set(
               pxr::GfVec3f(1));
         }},
        {Code::unsupportedDeclaration,
         [](auto, auto p) {
           pxr::UsdPhysicsMassAPI(p).CreateDensityAttr().Set(1000.0f);
         }},
        {Code::unsupportedDeclaration,
         [](auto, auto p) { p.template RemoveAPI<pxr::UsdPhysicsMassAPI>(); }},
        {Code::unsupportedDeclaration,
         [](auto, auto p) { p.SetTypeName(pxr::TfToken("Sphere")); }},
        {Code::unsupportedDeclaration,
         [](auto, auto p) {
           pxr::UsdPhysicsCollisionAPI(p).CreateCollisionEnabledAttr().Set(
               false);
         }},
        {Code::unsupportedDeclaration,
         [](auto s, auto) {
           pxr::UsdPhysicsScene::Define(s, pxr::SdfPath("/Scene"));
         }},
        {Code::unsupportedDeclaration,
         [](auto s, auto) {
           pxr::UsdGeomPointInstancer::Define(s, pxr::SdfPath("/Points"))
               .CreatePrototypesRel()
               .SetTargets({pxr::SdfPath("/World")});
         }},
        {Code::unsupportedDeclaration,
         [](auto s, auto) {
           pxr::UsdPhysicsRigidBodyAPI::Apply(
               s->GetPrimAtPath(pxr::SdfPath("/World")));
         }},
        {Code::unsupportedTransform,
         [](auto, auto p) {
           pxr::UsdGeomXformable(p).AddRotateYOp().Set(30.0f);
         }},
        {Code::invalidValue,
         [](auto, auto p) {
           pxr::UsdGeomXformable(p).GetXformOpOrderAttr().Set(
               pxr::VtTokenArray{pxr::TfToken("xformOp:missing")});
         }},
        {Code::unsupportedTransform,
         [](auto, auto p) {
           pxr::GfMatrix4d matrix(1.0);
           matrix[0][3] = 1;
           pxr::UsdGeomXformable(p).AddTransformOp().Set(matrix);
         }},
        {Code::invalidValue,
         [](auto, auto p) {
           pxr::UsdGeomXformable(p).AddRotateXOp().Set(
               std::numeric_limits<float>::quiet_NaN());
         }},
    };
    for (std::size_t i = 0; i < invalid.size(); ++i) {
      stage = fixture();
      invalid[i].second(stage,
                        stage->GetPrimAtPath(pxr::SdfPath("/World/Box")));
      try {
        (void)usd::readBoxScene(stage);
        throw std::runtime_error("invalid fixture accepted: " +
                                 std::to_string(i));
      } catch (const usd::ImportError &error) {
        check(error.code() == invalid[i].first && !error.path().empty(),
              "wrong diagnostic code/path");
      }
    }
    stage = fixture();
    auto instance = stage->DefinePrim(pxr::SdfPath("/Instance"));
    instance.GetReferences().AddInternalReference(pxr::SdfPath("/World"));
    instance.SetInstanceable(true);
    try {
      (void)usd::readBoxScene(stage);
      throw std::runtime_error("instanced physics accepted");
    } catch (const usd::ImportError &error) {
      check(error.code() == Code::unsupportedDeclaration,
            "instance diagnostic");
    }
    try {
      (void)usd::readBoxScene({});
      throw std::runtime_error("null Stage accepted");
    } catch (const usd::ImportError &error) {
      check(error.code() == Code::invalidStage, "null Stage diagnostic");
    }
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
