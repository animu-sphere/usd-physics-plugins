#include "usd_physics/core/world.h"
#include "usd_physics/core/kinematic_motion.h"

#include <cmath>
#include <stdexcept>
#include <string>

namespace usd_physics::core {

PhysicsWorld::~PhysicsWorld() = default;

void validateKinematicTarget(Transform target, PhysicsWorld::Duration duration) {
  validatePhysicsStep(duration);
  validatePhysicsVector(target.translation, "kinematic target translation");
  const auto q = target.rotation;
  if (!std::isfinite(q.w) || !std::isfinite(q.x) || !std::isfinite(q.y) ||
      !std::isfinite(q.z) || (q.w == 0 && q.x == 0 && q.y == 0 && q.z == 0)) {
    throw std::invalid_argument("kinematic target rotation must be finite and nonzero");
  }
}

void validatePhysicsStep(PhysicsWorld::Duration fixedStep) {
  if (!std::isfinite(fixedStep.count()) ||
      fixedStep <= PhysicsWorld::Duration::zero()) {
    throw std::invalid_argument("physics step must be finite and positive");
  }
}

void validatePhysicsVector(Vector3 vector, const char* name) {
  if (!std::isfinite(vector.x) || !std::isfinite(vector.y) ||
      !std::isfinite(vector.z)) {
    throw std::invalid_argument(std::string{name == nullptr ? "physics vector" : name} +
                                " must be finite");
  }
}

} // namespace usd_physics::core
