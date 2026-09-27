#include "usd_physics/usd/box_scene.h"
#include <pxr/base/gf/vec3d.h>
#include <pxr/usd/usdGeom/cube.h>
#include <pxr/usd/usdGeom/metrics.h>
#include <pxr/usd/usdPhysics/collisionAPI.h>

int main() {
  const auto stage = pxr::UsdStage::CreateInMemory();
  pxr::UsdGeomSetStageUpAxis(stage, pxr::UsdGeomTokens->y);
  pxr::UsdGeomSetStageMetersPerUnit(stage, 1.0);
  const auto cube = pxr::UsdGeomCube::Define(stage, pxr::SdfPath("/Box"));
  cube.AddTranslateOp().Set(pxr::GfVec3d(0.0));
  pxr::UsdPhysicsCollisionAPI::Apply(cube.GetPrim());
  const auto boxes = usd_physics::usd::readBoxScene(stage);
  return boxes.size() == 1 && boxes[0].shape.halfExtents.x == 1.0 ? 0 : 1;
}
