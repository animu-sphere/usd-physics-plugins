#pragma once

#include "usd_physics/core/descriptors.h"

#include <chrono>
#include <vector>

namespace usd_physics::core {

struct BodyState {
  BodyHandle body;
  Transform transform;
  Vector3 linearVelocity;
};

class PhysicsWorld {
public:
  using Duration = std::chrono::duration<double>;

  PhysicsWorld() = default;
  virtual ~PhysicsWorld();
  PhysicsWorld(const PhysicsWorld&) = delete;
  PhysicsWorld& operator=(const PhysicsWorld&) = delete;
  PhysicsWorld(PhysicsWorld&&) = delete;
  PhysicsWorld& operator=(PhysicsWorld&&) = delete;

  virtual ShapeHandle createShape(const ShapeDescriptor& descriptor) = 0;
  virtual bool destroyShape(ShapeHandle shape) noexcept = 0;

  virtual BodyHandle createBody(const BodyDescriptor& descriptor) = 0;
  virtual bool destroyBody(BodyHandle body) noexcept = 0;

  virtual ConstraintHandle createConstraint(const ConstraintDescriptor& descriptor) = 0;
  virtual bool destroyConstraint(ConstraintHandle constraint) noexcept = 0;

  virtual bool applyForce(BodyHandle body, Vector3 force) = 0;
  virtual bool setLinearVelocity(BodyHandle body, Vector3 velocity) = 0;
  [[nodiscard]] virtual BodyState bodyState(BodyHandle body) const = 0;

  virtual void step(Duration fixedStep) = 0;
  virtual std::vector<BodyState> takeChangedBodyStates() = 0;
};

void validatePhysicsStep(PhysicsWorld::Duration fixedStep);
void validatePhysicsVector(Vector3 vector, const char* name);

} // namespace usd_physics::core
