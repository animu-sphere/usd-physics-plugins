#include "usd_physics/usd/box_scene.h"
#include <cmath>
#include <iostream>
#include <limits>
#include <pxr/usd/sdf/layer.h>
#include <pxr/usd/usdGeom/cube.h>
#include <pxr/usd/usdGeom/metrics.h>
#include <pxr/usd/usdGeom/xform.h>
#include <pxr/usd/usdPhysics/collisionAPI.h>
#include <pxr/usd/usdPhysics/metrics.h>
#include <pxr/usd/usdPhysics/rigidBodyAPI.h>
#include <pxr/usd/usdPhysics/scene.h>

using namespace usd_physics;

void check(bool condition, const char *message) {
  if (!condition)
    throw std::runtime_error(message);
}

template <class Read>
void rejects(Read read, usd::ImportErrorCode code, const char *path) {
  try {
    read();
  } catch (const usd::ImportError &error) {
    check(error.code() == code && error.path() == path, "typed scene error");
    return;
  }
  throw std::runtime_error("invalid scene accepted");
}

int main() {
  try {
    using Code = usd::ImportErrorCode;
    rejects([] { (void)usd::readPhysicsScene({}); }, Code::invalidStage, "");
    const auto stage = pxr::UsdStage::CreateInMemory();
    pxr::UsdGeomSetStageUpAxis(stage, pxr::UsdGeomTokens->y);
    pxr::UsdGeomSetStageMetersPerUnit(stage, 1.0);
    check(!usd::readPhysicsScene(stage).settings, "no implicit scene");
    // The generated fixture also exercises coexistence with real body data.
    const auto cube = pxr::UsdGeomCube::Define(stage, pxr::SdfPath("/Box"));
    pxr::UsdPhysicsCollisionAPI::Apply(cube.GetPrim());
    pxr::UsdPhysicsRigidBodyAPI::Apply(cube.GetPrim());
    const auto parent = pxr::UsdGeomXform::Define(stage, pxr::SdfPath("/World"));
    parent.AddRotateZOp().Set(90.0f);
    parent.AddScaleOp().Set(pxr::GfVec3f(3));
    const auto scene = pxr::UsdPhysicsScene::Define(stage,
        pxr::SdfPath("/World/Scene"));
    auto read = [&] { return usd::readPhysicsScene(stage); };
    auto verify = [&](core::Vector3 expected) {
      std::string before, after;
      stage->GetRootLayer()->ExportToString(&before);
      const auto snapshot = read();
      stage->GetRootLayer()->ExportToString(&after);
      check(before == after, "reader must not author defaults or gravity");
      check(snapshot.bodies.size() == 1 && snapshot.joints.empty() &&
            snapshot.settings && snapshot.settings->path == "/World/Scene",
            "scene path and body snapshot");
      const auto gravity = snapshot.settings->gravity;
      check(std::abs(gravity.x - expected.x) < 1e-10 &&
            std::abs(gravity.y - expected.y) < 1e-10 &&
            std::abs(gravity.z - expected.z) < 1e-10,
            "world-space gravity conversion");
    };
    verify({0, -9.81, 0}); // Schema fallback: zero direction, -infinity magnitude.
    scene.CreateGravityMagnitudeAttr().Set(10.0f);
    verify({0, -10, 0});
    scene.CreateGravityDirectionAttr().Set(pxr::GfVec3f(0, 3, 4));
    verify({0, 6, 8}); // Normalize; ignore scene-prim ancestor transforms.
    scene.CreateGravityMagnitudeAttr().Set(-2.0f);
    verify({0, 9.81 * .6, 9.81 * .8});
    scene.CreateGravityMagnitudeAttr().Set(0.0f);
    verify({0, 0, 0});
    scene.CreateGravityMagnitudeAttr().Set(10.0f);
    const auto saved = read();
    scene.CreateGravityMagnitudeAttr().Set(20.0f);
    check(saved.settings->gravity.y == 6, "snapshot retains plain values");
    scene.CreateGravityDirectionAttr().Set(pxr::GfVec3f(
        std::numeric_limits<float>::max(), 0, 0));
    verify({20, 0, 0}); // Normalization must not overflow float length.
    scene.CreateGravityDirectionAttr().Set(pxr::GfVec3f(
        std::numeric_limits<float>::min(), 0, 0));
    verify({20, 0, 0});
    const auto direction = scene.CreateGravityDirectionAttr();
    const auto magnitude = scene.CreateGravityMagnitudeAttr();
    for (float invalid : {std::numeric_limits<float>::infinity(),
                          std::numeric_limits<float>::quiet_NaN()}) {
      magnitude.Set(invalid);
      rejects(read, Code::invalidValue, "/World/Scene");
      magnitude.Set(0.0f);
      direction.Set(pxr::GfVec3f(invalid, 0, 0));
      rejects(read, Code::invalidValue, "/World/Scene");
      direction.Set(pxr::GfVec3f(0));
    }
    direction.Set(pxr::GfVec3f(-std::numeric_limits<float>::infinity(), 0, 0));
    rejects(read, Code::invalidValue, "/World/Scene");
    direction.Clear();
    for (const auto &attribute : {direction, magnitude}) {
      if (attribute == direction)
        attribute.Set(pxr::GfVec3f(0), 1);
      else
        attribute.Set(1.0f, 1);
      rejects(read, Code::invalidValue, "/World/Scene");
      attribute.Clear();
      attribute.Block();
      rejects(read, Code::invalidValue, "/World/Scene");
      attribute.Clear();
    }
    // Selection rejects the second lexical path, regardless of authored order.
    pxr::UsdPhysicsScene::Define(stage, pxr::SdfPath("/Earlier"));
    rejects(read, Code::unsupportedDeclaration, "/World/Scene");
    stage->RemovePrim(pxr::SdfPath("/Earlier"));
    pxr::UsdGeomSetStageUpAxis(stage, pxr::UsdGeomTokens->z);
    rejects(read, Code::unsupportedUnits, "/World/Scene");
    pxr::UsdGeomSetStageUpAxis(stage, pxr::UsdGeomTokens->y);
    pxr::UsdGeomSetStageMetersPerUnit(stage, .01);
    rejects(read, Code::unsupportedUnits, "/World/Scene");
    pxr::UsdGeomSetStageMetersPerUnit(stage, 1.0);
    pxr::UsdPhysicsSetStageKilogramsPerUnit(stage, .001);
    rejects(read, Code::unsupportedUnits, "/World/Scene");
    pxr::UsdPhysicsSetStageKilogramsPerUnit(stage, 1.0);
    scene.GetPrim().CreateAttribute(pxr::TfToken("physics:unsupported"),
        pxr::SdfValueTypeNames->Float).Set(1.0f);
    rejects(read, Code::unsupportedDeclaration, "/World/Scene");
    scene.GetPrim().RemoveProperty(pxr::TfToken("physics:unsupported"));
    scene.GetPrim().CreateRelationship(pxr::TfToken("material:binding:physics"))
        .SetTargets({pxr::SdfPath("/Material")});
    rejects(read, Code::unsupportedDeclaration, "/World/Scene");
    scene.GetPrim().RemoveProperty(pxr::TfToken("material:binding:physics"));
    pxr::UsdPhysicsRigidBodyAPI::Apply(scene.GetPrim());
    rejects(read, Code::unsupportedDeclaration, "/World/Scene");
    scene.GetPrim().RemoveAPI<pxr::UsdPhysicsRigidBodyAPI>();
    rejects([&] { (void)usd::readBoxScene(stage); },
        Code::unsupportedDeclaration, "/World/Scene");
    rejects([&] { (void)usd::readPrimitiveScene(stage); },
        Code::unsupportedDeclaration, "/World/Scene");
    // Inactive declarations do not select a scene.
    scene.GetPrim().SetActive(false);
    check(!read().settings, "inactive scene ignored");
    scene.GetPrim().SetActive(true);
    stage->RemovePrim(pxr::SdfPath("/Box"));
    const auto sceneOnly = read();
    check(sceneOnly.settings && sceneOnly.bodies.empty(), "scene-only snapshot");
    pxr::UsdGeomSetStageUpAxis(stage, pxr::UsdGeomTokens->z);
    rejects(read, Code::unsupportedUnits, "/World/Scene");
    pxr::UsdGeomSetStageUpAxis(stage, pxr::UsdGeomTokens->y);
    // A referenced scene under an instance must fail rather than disappear.
    stage->RemovePrim(pxr::SdfPath("/World"));
    const auto asset = pxr::UsdStage::CreateInMemory();
    pxr::UsdGeomXform::Define(asset, pxr::SdfPath("/Asset"));
    pxr::UsdPhysicsScene::Define(asset, pxr::SdfPath("/Asset/Scene"));
    const auto instance = stage->DefinePrim(pxr::SdfPath("/Instance"));
    instance.GetReferences().AddReference(asset->GetRootLayer()->GetIdentifier(),
        pxr::SdfPath("/Asset"));
    instance.SetInstanceable(true);
    rejects(read, Code::unsupportedDeclaration, "/Instance/Scene");
    return 0;
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
