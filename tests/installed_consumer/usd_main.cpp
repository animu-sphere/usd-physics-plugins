#include "usd_physics/usd/box_scene.h"
#include <pxr/base/gf/vec3d.h>
#include <pxr/usd/usdGeom/cube.h>
#include <pxr/usd/usdGeom/metrics.h>
#include <pxr/usd/usdGeom/xform.h>
#include <pxr/usd/usdPhysics/collisionAPI.h>
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
