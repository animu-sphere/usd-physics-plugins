#include "usd_physics/usd/box_scene.h"
#include <cmath>
#include <pxr/base/gf/vec3d.h>
#include <pxr/usd/usdGeom/capsule.h>
#include <pxr/usd/usdGeom/cube.h>
#include <pxr/usd/usdGeom/metrics.h>
#include <pxr/usd/usdGeom/xform.h>
#include <pxr/usd/usdPhysics/collisionAPI.h>
#include <pxr/usd/usdPhysics/joint.h>
#include <pxr/usd/usdPhysics/limitAPI.h>
#include <pxr/usd/usdPhysics/massAPI.h>
#include <pxr/usd/usdPhysics/rigidBodyAPI.h>

int main() {
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
  pxr::UsdPhysicsMassAPI(body.GetPrim()).CreateMassAttr().Set(0.0f);
  if (std::abs(usd_physics::usd::readPhysicsScene(stage).bodies[2].mass -
               8000) > 1e-8)
    return 6;
  pxr::UsdPhysicsMassAPI(body.GetPrim()).CreateDensityAttr().Set(5.0f);
  if (std::abs(usd_physics::usd::readPhysicsScene(stage).bodies[2].mass - 40) >
      1e-10)
    return 7;
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
