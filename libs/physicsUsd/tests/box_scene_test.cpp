#include "usd_physics/usd/box_scene.h"
#include <functional>
#include <iostream>
#include <limits>
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
  cube.AddTranslateOp(pxr::UsdGeomXformOp::PrecisionFloat).Set(pxr::GfVec3f(0, 3, 0));
  cube.AddScaleOp().Set(pxr::GfVec3f(-0.5, 1, 2));
  pxr::UsdPhysicsCollisionAPI::Apply(cube.GetPrim());
  pxr::UsdPhysicsRigidBodyAPI::Apply(cube.GetPrim());
  pxr::UsdPhysicsMassAPI::Apply(cube.GetPrim()).CreateMassAttr().Set(70.0f);
  return stage;
}

void check(bool condition, const char* message) {
  if (!condition) throw std::runtime_error(message);
}

int main() {
  try {
    auto stage = fixture();
    std::string before, after;
    stage->GetRootLayer()->ExportToString(&before);
    const auto boxes = usd::readBoxScene(stage);
    stage->GetRootLayer()->ExportToString(&after);
    check(before == after, "import must not edit USD");
    check(boxes.size() == 1 && boxes[0].path == "/World/Box" && boxes[0].mass == 70 &&
          boxes[0].motionType == core::MotionType::dynamicBody &&
          boxes[0].shape.halfExtents == core::Vector3{0.5, 1, 2} &&
          boxes[0].transform.translation == core::Vector3{0, 3, 0}, "box conversion");
    auto box = stage->GetPrimAtPath(pxr::SdfPath("/World/Box"));
    pxr::UsdPhysicsRigidBodyAPI(box).CreateRigidBodyEnabledAttr().Set(false);
    check(usd::readBoxScene(stage)[0].motionType == core::MotionType::staticBody, "disabled rigid body is static");
    box.RemoveAPI<pxr::UsdPhysicsRigidBodyAPI>();
    box.RemoveProperty(pxr::TfToken("physics:rigidBodyEnabled"));
    check(usd::readBoxScene(stage)[0].motionType == core::MotionType::staticBody, "collision-only is static");
    pxr::UsdPhysicsCollisionAPI(box).CreateCollisionEnabledAttr().Set(false);
    check(usd::readBoxScene(stage).empty(), "disabled static collision is skipped");
    stage = fixture();
    auto floor = pxr::UsdGeomCube::Define(stage, pxr::SdfPath("/A"));
    floor.AddTranslateOp().Set(pxr::GfVec3d(0, -1, 0));
    pxr::UsdPhysicsCollisionAPI::Apply(floor.GetPrim());
    check(usd::readBoxScene(stage)[0].path == "/A", "path order is deterministic");
    auto parent = pxr::UsdGeomXform::Define(stage, pxr::SdfPath("/World"));
    parent.AddTranslateOp().Set(pxr::GfVec3d(4, 0, 0));
    pxr::UsdGeomXformable(stage->GetPrimAtPath(pxr::SdfPath("/World/Box"))).SetResetXformStack(true);
    check(usd::readBoxScene(stage)[1].transform.translation.x == 0, "reset stack ignores ancestors");

    using Edit = std::function<void(const pxr::UsdStageRefPtr&, const pxr::UsdPrim&)>;
    const std::vector<std::pair<Code, Edit>> invalid{
      {Code::unsupportedUnits, [](auto s, auto) { pxr::UsdGeomSetStageMetersPerUnit(s, .01); }},
      {Code::unsupportedUnits, [](auto s, auto) { pxr::UsdGeomSetStageUpAxis(s, pxr::UsdGeomTokens->z); }},
      {Code::unsupportedUnits, [](auto s, auto) { pxr::UsdPhysicsSetStageKilogramsPerUnit(s, .001); }},
      {Code::invalidValue, [](auto, auto p) { pxr::UsdPhysicsMassAPI(p).CreateMassAttr().Set(0.0f); }},
      {Code::invalidValue, [](auto, auto p) { pxr::UsdPhysicsMassAPI(p).CreateMassAttr().Set(-1.0f); }},
      {Code::invalidValue, [](auto, auto p) { pxr::UsdGeomCube(p).CreateSizeAttr().Set(std::numeric_limits<double>::infinity()); }},
      {Code::invalidValue, [](auto, auto p) { p.GetAttribute(pxr::TfToken("xformOp:scale")).Set(pxr::GfVec3f(0)); }},
      {Code::invalidValue, [](auto, auto p) { p.GetAttribute(pxr::TfToken("xformOp:translate")).Set(pxr::GfVec3f(2), 1); }},
      {Code::unsupportedDeclaration, [](auto, auto p) { pxr::UsdPhysicsRigidBodyAPI(p).CreateKinematicEnabledAttr().Set(true); }},
      {Code::unsupportedDeclaration, [](auto, auto p) { pxr::UsdPhysicsRigidBodyAPI(p).CreateVelocityAttr().Set(pxr::GfVec3f(1)); }},
      {Code::unsupportedDeclaration, [](auto, auto p) { pxr::UsdPhysicsMassAPI(p).CreateDensityAttr().Set(1000.0f); }},
      {Code::unsupportedDeclaration, [](auto, auto p) { p.RemoveAPI<pxr::UsdPhysicsMassAPI>(); }},
      {Code::unsupportedDeclaration, [](auto, auto p) { p.SetTypeName(pxr::TfToken("Sphere")); }},
      {Code::unsupportedDeclaration, [](auto, auto p) { pxr::UsdPhysicsCollisionAPI(p).CreateCollisionEnabledAttr().Set(false); }},
      {Code::unsupportedDeclaration, [](auto s, auto) { pxr::UsdPhysicsScene::Define(s, pxr::SdfPath("/Scene")); }},
      {Code::unsupportedDeclaration, [](auto s, auto) {
        pxr::UsdGeomPointInstancer::Define(s, pxr::SdfPath("/Points"))
          .CreatePrototypesRel().SetTargets({pxr::SdfPath("/World")});
      }},
      {Code::unsupportedDeclaration, [](auto s, auto) { pxr::UsdPhysicsRigidBodyAPI::Apply(s->GetPrimAtPath(pxr::SdfPath("/World"))); }},
      {Code::unsupportedTransform, [](auto, auto p) { pxr::UsdGeomXformable(p).AddRotateYOp().Set(30.0f); }},
      {Code::unsupportedTransform, [](auto, auto p) { pxr::UsdGeomXformable(p).ClearXformOpOrder(); }},
      {Code::unsupportedTransform, [](auto s, auto) { pxr::UsdGeomXform::Define(s, pxr::SdfPath("/World")).AddTranslateOp().Set(pxr::GfVec3d(1)); }},
    };
    for (std::size_t i = 0; i < invalid.size(); ++i) {
      stage = fixture();
      invalid[i].second(stage, stage->GetPrimAtPath(pxr::SdfPath("/World/Box")));
      try {
        (void)usd::readBoxScene(stage);
        throw std::runtime_error("invalid fixture accepted: " + std::to_string(i));
      } catch (const usd::ImportError& error) {
        check(error.code() == invalid[i].first && !error.path().empty(), "wrong diagnostic code/path");
      }
    }
    stage = fixture();
    auto instance = stage->DefinePrim(pxr::SdfPath("/Instance"));
    instance.GetReferences().AddInternalReference(pxr::SdfPath("/World"));
    instance.SetInstanceable(true);
    try {
      (void)usd::readBoxScene(stage);
      throw std::runtime_error("instanced physics accepted");
    } catch (const usd::ImportError& error) {
      check(error.code() == Code::unsupportedDeclaration, "instance diagnostic");
    }
    try {
      (void)usd::readBoxScene({});
      throw std::runtime_error("null Stage accepted");
    } catch (const usd::ImportError& error) {
      check(error.code() == Code::invalidStage, "null Stage diagnostic");
    }
  } catch (const std::exception& error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
