#include "usd_physics/core/world.h"

#include <cmath>
#include <stdexcept>
#include <string>

namespace usd_physics::core {

PhysicsWorld::~PhysicsWorld() = default;

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
