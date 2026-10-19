#include "usd_physics/usd/box_scene.h"
#include "usd_physics/jolt/availability.h"
#include <cmath>
#include <pxr/base/gf/vec3d.h>
#include <pxr/usd/usdGeom/capsule.h>
#include <pxr/usd/usdGeom/cube.h>
#include <pxr/usd/usdGeom/metrics.h>
#include <pxr/usd/usdGeom/xform.h>
#include "usd_physics/usd/joint_conversion.h"
#include <pxr/usd/usdPhysics/collisionAPI.h>
#include <pxr/usd/usdPhysics/joint.h>
#include <pxr/usd/usdPhysics/limitAPI.h>
#include <pxr/usd/usdPhysics/massAPI.h>
#include <pxr/usd/usdPhysics/rigidBodyAPI.h>
#include <pxr/usd/usdPhysics/scene.h>
#include <pxr/usd/sdf/layer.h>

bool executesSceneGravity() {
  using namespace usd_physics;
  if (!jolt::backendAvailable())
    return true; // The same installed USD package works without a solver.
  const auto stage = pxr::UsdStage::CreateInMemory();
  pxr::UsdGeomSetStageUpAxis(stage, pxr::UsdGeomTokens->y);
  pxr::UsdGeomSetStageMetersPerUnit(stage, 1.0);
  const auto cube = pxr::UsdGeomCube::Define(stage, pxr::SdfPath("/Box"));
  pxr::UsdPhysicsCollisionAPI::Apply(cube.GetPrim());
  pxr::UsdPhysicsRigidBodyAPI::Apply(cube.GetPrim());
  pxr::UsdPhysicsMassAPI::Apply(cube.GetPrim()).CreateMassAttr().Set(1.0f);
  for (int mode = 0; mode != 3; ++mode) {
    if (mode != 0) {
      const auto settings = pxr::UsdPhysicsScene::Define(stage, pxr::SdfPath("/Scene"));
      settings.CreateGravityDirectionAttr().Set(pxr::GfVec3f(2, 3, 6));
      settings.CreateGravityMagnitudeAttr().Set(mode == 1 ? 7.0f : 0.0f);
    }
    std::string before, after;
    stage->GetRootLayer()->ExportToString(&before);
    const auto snapshot = usd::readPhysicsScene(stage);
    // Consumer selects world defaults when no scene settings were authored.
    auto world = snapshot.settings
                     ? jolt::createWorld({}, snapshot.settings->gravity)
                     : jolt::createWorld();
    const auto &declaration = snapshot.bodies.at(0);
    const auto shape = world->createShape(declaration.shape);
    const auto handle = world->createBody(
        {shape, declaration.motionType, declaration.transform, declaration.mass,
         {}, {}, {0, 0}});
    for (int i = 0; i != 30; ++i)
      world->step(core::PhysicsWorld::Duration{1.0 / 60});
    const auto velocity = world->bodyState(handle).linearVelocity;
    const core::Vector3 expected = mode == 0 ? core::Vector3{0, -4.905, 0}
                                  : mode == 1 ? core::Vector3{1, 1.5, 3}
                                              : core::Vector3{};
    stage->GetRootLayer()->ExportToString(&after);
    if (std::abs(velocity.x - expected.x) > 1e-4 ||
        std::abs(velocity.y - expected.y) > 1e-4 ||
        std::abs(velocity.z - expected.z) > 1e-4 || before != after ||
        !world->destroyBody(handle) || !world->destroyShape(shape))
      return false;
  }
  return true;
}

int main() {
  if (!executesSceneGravity())
    return 10;
  const auto stage = pxr::UsdStage::CreateInMemory();
  pxr::UsdGeomSetStageUpAxis(stage, pxr::UsdGeomTokens->y);
  pxr::UsdGeomSetStageMetersPerUnit(stage, 1.0);
  const auto cube = pxr::UsdGeomCube::Define(stage, pxr::SdfPath("/Box"));
  cube.AddTranslateOp().Set(pxr::GfVec3d(0.0));
  pxr::UsdPhysicsCollisionAPI::Apply(cube.GetPrim());
  const auto boxes = usd_physics::usd::readBoxScene(stage);
  if (boxes.size() != 1 || boxes[0].shape.halfExtents.x != 1.0)
    return 1;
  const auto body = pxr::UsdGeomXform::Define(stage, pxr::SdfPath("/Rig"));
  body.AddTranslateOp().Set(pxr::GfVec3d(1, 2, 3));
  body.AddRotateYOp().Set(30.0f);
  pxr::UsdPhysicsRigidBodyAPI::Apply(body.GetPrim());
  pxr::UsdPhysicsMassAPI::Apply(body.GetPrim()).CreateMassAttr().Set(2.0f);
  const auto child =
      pxr::UsdGeomCube::Define(stage, pxr::SdfPath("/Rig/Shape"));
  child.AddTranslateOp().Set(pxr::GfVec3d(3, 4, 5));
  child.AddRotateZOp().Set(90.0f);
  pxr::UsdPhysicsCollisionAPI::Apply(child.GetPrim());
  const auto combined = usd_physics::usd::readBoxScene(stage);
  const auto capsule =
      pxr::UsdGeomCapsule::Define(stage, pxr::SdfPath("/Capsule"));
  capsule.CreateRadiusAttr().Set(.25);
  capsule.CreateHeightAttr().Set(2.0);
  capsule.CreateAxisAttr().Set(pxr::UsdGeomTokens->z);
  pxr::UsdPhysicsCollisionAPI::Apply(capsule.GetPrim());
  const auto primitives = usd_physics::usd::readPrimitiveScene(stage);
  if (primitives.size() != 3 || primitives[1].path != "/Capsule" ||
      primitives[1].shape.type != usd_physics::core::ShapeType::capsule ||
      primitives[1].shape.radius != .25 || primitives[1].shape.halfHeight != 1)
    return 3;
  pxr::UsdPhysicsRigidBodyAPI::Apply(capsule.GetPrim())
      .CreateKinematicEnabledAttr()
      .Set(true);
  if (usd_physics::usd::readPrimitiveScene(stage)[1].motionType !=
      usd_physics::core::MotionType::kinematicBody)
    return 4;
  const auto joint =
      pxr::UsdPhysicsJoint::Define(stage, pxr::SdfPath("/Joint"));
  joint.CreateBody0Rel().SetTargets({body.GetPath()});
  joint.CreateBody1Rel().SetTargets({capsule.GetPath()});
  pxr::UsdPhysicsLimitAPI::Apply(joint.GetPrim(), pxr::TfToken("transX"))
      .CreateHighAttr()
      .Set(0.5f);
  const auto scene = usd_physics::usd::readPhysicsScene(stage);
  if (scene.bodies.size() != 3 || scene.joints.size() != 1 ||
      scene.joints[0].body0 != "/Rig" || scene.joints[0].body1 != "/Capsule" ||
      !scene.joints[0].limits[0].applied ||
      scene.joints[0].limits[0].high != .5)
    return 5;
  auto executable = scene.joints[0];
  executable.limits[0].low = -.25;
  const auto descriptor = usd_physics::usd::toSixDofConstraint(executable,
      usd_physics::core::BodyHandle{1}, usd_physics::core::BodyHandle{2});
  if (!descriptor || descriptor->limits[0].motion != usd_physics::core::AxisMotion::limited ||
      descriptor->limits[0].low != -.25 || descriptor->limits[0].high != .5) return 8;
  pxr::UsdPhysicsMassAPI(body.GetPrim()).CreateMassAttr().Set(0.0f);
  if (std::abs(usd_physics::usd::readPhysicsScene(stage).bodies[2].mass -
               8000) > 1e-8)
    return 6;
  pxr::UsdPhysicsMassAPI(body.GetPrim()).CreateDensityAttr().Set(5.0f);
  if (std::abs(usd_physics::usd::readPhysicsScene(stage).bodies[2].mass - 40) >
      1e-10)
    return 7;
  const auto settings = pxr::UsdPhysicsScene::Define(stage, pxr::SdfPath("/Scene"));
  settings.CreateGravityDirectionAttr().Set(pxr::GfVec3f(0, -2, 0));
  settings.CreateGravityMagnitudeAttr().Set(10.0f);
  const auto withGravity = usd_physics::usd::readPhysicsScene(stage);
  if (!withGravity.settings || withGravity.settings->path != "/Scene" ||
      withGravity.settings->gravity != usd_physics::core::Vector3{0, -10, 0} ||
      withGravity.bodies.size() != 3 || withGravity.joints.size() != 1)
    return 9;
  return combined.size() == 2 && combined[1].path == "/Rig" &&
                 combined[1].mass == 2 &&
                 combined[1].transform.translation.y == 2 &&
                 combined[1].shape.localTransform.translation ==
                     usd_physics::core::Vector3{3, 4, 5} &&
                 combined[1].motionType ==
                     usd_physics::core::MotionType::dynamicBody
             ? 0
             : 2;
}
