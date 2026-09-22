#include "usd_physics/core/ground_query.h"

#include "usd_physics/core/world.h"

#include <cmath>
#include <stdexcept>

namespace usd_physics::core {

GroundQuery::~GroundQuery() = default;

void validateGroundProbeDistance(double maxDistance) {
  if (!std::isfinite(maxDistance) || maxDistance < 0.0) {
    throw std::invalid_argument(
        "ground probe distance must be finite and non-negative");
  }
}

void validateGroundContact(const GroundContact& contact) {
  if (!contact.supportBody) {
    throw std::invalid_argument("ground contact requires a support body");
  }
  validatePhysicsVector(contact.normal, "ground normal");
  const double lengthSquared = contact.normal.x * contact.normal.x +
                               contact.normal.y * contact.normal.y +
                               contact.normal.z * contact.normal.z;
  if (!std::isfinite(lengthSquared) || lengthSquared <= 0.0) {
    throw std::invalid_argument("ground normal must have non-zero finite length");
  }
  if (!std::isfinite(contact.distance) || contact.distance < 0.0) {
    throw std::invalid_argument(
        "ground distance must be finite and non-negative");
  }
}

} // namespace usd_physics::core
