#include "usd_physics/usd/box_scene.h"
#include <cmath>
#include <iostream>
#include <limits>
#include <pxr/usd/sdf/layer.h>
#include <pxr/usd/usdGeom/capsule.h>
#include <pxr/usd/usdGeom/cube.h>
#include <pxr/usd/usdGeom/metrics.h>
#include <pxr/usd/usdGeom/sphere.h>
#include <pxr/usd/usdGeom/xform.h>
#include <pxr/usd/usdPhysics/collisionAPI.h>
#include <pxr/usd/usdPhysics/massAPI.h>
#include <pxr/usd/usdPhysics/rigidBodyAPI.h>

using namespace usd_physics;

void check(bool condition, const char *message) {
  if (!condition)
    throw std::runtime_error(message);
}

int main() {
  try {
    const double pi = std::acos(-1.0);
    for (const bool child : {false, true}) {
      for (const auto *type : {"Cube", "Sphere", "Capsule"}) {
        auto stage = pxr::UsdStage::CreateInMemory();
        pxr::UsdGeomSetStageUpAxis(stage, pxr::UsdGeomTokens->y);
        pxr::UsdGeomSetStageMetersPerUnit(stage, 1.0);
        auto body =
            pxr::UsdGeomXform::Define(stage, pxr::SdfPath("/Body")).GetPrim();
        pxr::UsdPhysicsRigidBodyAPI::Apply(body);
        auto collider =
            child ? stage->DefinePrim(pxr::SdfPath("/Body/Shape")) : body;
        collider.SetTypeName(pxr::TfToken(type));
        pxr::UsdPhysicsCollisionAPI::Apply(collider);
        auto xform = pxr::UsdGeomXformable(collider);
        xform.AddTranslateOp().Set(pxr::GfVec3d(1, -2, 3));
        xform.AddRotateXYZOp().Set(pxr::GfVec3f(20, 30, 40));
        // Reflections and scale affect volume; translations and rotations do
        // not.
        xform.AddScaleOp().Set(pxr::GfVec3f(-2, 2, 2));
        const std::string name(type);
        double volume = 0;
        if (name == "Cube") {
          pxr::UsdGeomCube(collider).CreateSizeAttr().Set(3.0);
          volume = 216.0; // 6 x 6 x 6 meters.
        } else {
          if (name == "Sphere") {
            pxr::UsdGeomSphere(collider).CreateRadiusAttr().Set(0.5);
          } else {
            const pxr::UsdGeomCapsule capsule(collider);
            capsule.CreateRadiusAttr().Set(0.5);
            capsule.CreateHeightAttr().Set(2.0);
            capsule.CreateAxisAttr().Set(pxr::UsdGeomTokens->z);
          }
          volume = name == "Sphere" ? 4 * pi / 3 : 16 * pi / 3;
        }
        auto verify = [&](double expected) {
          std::string before, after;
          stage->GetRootLayer()->ExportToString(&before);
          const auto snapshot = usd::readPhysicsScene(stage);
          stage->GetRootLayer()->ExportToString(&after);
          check(before == after && snapshot.bodies.size() == 1 &&
                    std::abs(snapshot.bodies[0].mass - expected) <
                        expected * 1e-12,
                "read-only inferred primitive mass");
          if (name == "Cube")
            check(std::abs(usd::readBoxScene(stage)[0].mass - expected) <
                      expected * 1e-12,
                  "legacy box reader mass inference");
          check(std::abs(usd::readPrimitiveScene(stage)[0].mass - expected) <
                    expected * 1e-12,
                "primitive reader mass inference");
        };
        verify(1000 * volume); // No MassAPI.
        const auto api = pxr::UsdPhysicsMassAPI::Apply(body);
        verify(1000 * volume); // Schema defaults are zero.
        api.CreateMassAttr().Set(0.0f);
        api.CreateDensityAttr().Set(0.0f);
        verify(1000 * volume); // Authored zero requests inference too.
        api.CreateDensityAttr().Set(12.0f);
        verify(12 * volume);
        api.CreateMassAttr().Set(7.0f);
        verify(7); // Explicit mass wins over density and geometry volume.
        api.CreateMassAttr().Set(0.0f);
        if (name == "Capsule") {
          pxr::UsdGeomCapsule(collider).CreateHeightAttr().Set(0.0);
          verify(12 * 4 * pi / 3); // Zero-height capsule is a sphere.
        }
        auto reject = [&](usd::ImportErrorCode code, const char *path) {
          try {
            (void)usd::readPhysicsScene(stage);
            throw std::runtime_error("invalid mass declaration accepted");
          } catch (const usd::ImportError &error) {
            check(error.code() == code && error.path() == path,
                  "mass error code/path");
          }
        };
        for (const bool density : {false, true}) {
          const auto attr =
              density ? api.CreateDensityAttr() : api.CreateMassAttr();
          for (float value : {-1.0f, std::numeric_limits<float>::infinity(),
                              std::numeric_limits<float>::quiet_NaN()}) {
            attr.Set(value);
            reject(usd::ImportErrorCode::invalidValue, "/Body");
          }
          attr.Set(1.0f);
          attr.Set(2.0f, 1);
          reject(usd::ImportErrorCode::invalidValue, "/Body");
          attr.Clear();
        }
        api.CreateDensityAttr().Set(1.0f);
        for (const auto *binding :
             {"material:binding:physics",
              "material:binding:collection:physics:group"}) {
          body.CreateRelationship(pxr::TfToken(binding))
              .SetTargets({pxr::SdfPath("/ExternalMaterial")});
          reject(usd::ImportErrorCode::unsupportedDeclaration, "/Body");
          body.RemoveProperty(pxr::TfToken(binding));
        }
        // Valid geometry can still produce a mass outside double's range.
        if (name == "Cube")
          pxr::UsdGeomCube(collider).CreateSizeAttr().Set(1e150);
        else if (name == "Sphere")
          pxr::UsdGeomSphere(collider).CreateRadiusAttr().Set(1e150);
        else
          pxr::UsdGeomCapsule(collider).CreateRadiusAttr().Set(1e150);
        reject(usd::ImportErrorCode::invalidValue, "/Body");
        if (name == "Cube")
          pxr::UsdGeomCube(collider).CreateSizeAttr().Set(1e-150);
        else if (name == "Sphere")
          pxr::UsdGeomSphere(collider).CreateRadiusAttr().Set(1e-150);
        else
          pxr::UsdGeomCapsule(collider).CreateRadiusAttr().Set(1e-150);
        reject(usd::ImportErrorCode::invalidValue, "/Body");
      }
    }
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
