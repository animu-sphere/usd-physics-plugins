#include "usd_physics/core/segment_query.h"

#include "usd_physics/core/world.h"

#include <cmath>
#include <stdexcept>

namespace usd_physics::core {

SegmentQuery::~SegmentQuery() = default;

void validateSegmentEndpoints(Vector3 origin, Vector3 target) {
  validatePhysicsVector(origin, "segment origin");
  validatePhysicsVector(target, "segment target");
  const double x = target.x - origin.x;
  const double y = target.y - origin.y;
  const double z = target.z - origin.z;
  const double lengthSquared = x * x + y * y + z * z;
  if (!std::isfinite(lengthSquared) || lengthSquared <= 0.0) {
    throw std::invalid_argument("segment must have non-zero finite length");
  }
}

void validateSegmentHit(const SegmentHit& hit) {
  if (!hit.body) {
    throw std::invalid_argument("segment hit requires a body");
  }
  if (!std::isfinite(hit.fraction) || hit.fraction < 0.0 ||
      hit.fraction > 1.0) {
    throw std::invalid_argument("segment hit fraction must be in [0, 1]");
  }
}

} // namespace usd_physics::core
