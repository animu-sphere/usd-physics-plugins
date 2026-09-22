#pragma once

#include "usd_physics/core/handles.h"
#include "usd_physics/core/value_types.h"

#include <optional>

namespace usd_physics::core {

struct SegmentHit {
  BodyHandle body;
  // Normalized distance from origin (0) to target (1).
  double fraction{0.0};
};

// Optional backend capability for finding the first body along a world-space
// segment. The ignored body is useful for probes that start at a controlled
// character or another followed physics object.
class SegmentQuery {
public:
  virtual ~SegmentQuery();

  [[nodiscard]] virtual std::optional<SegmentHit>
  segmentHit(Vector3 origin, Vector3 target,
             BodyHandle ignoredBody = {}) const = 0;
};

void validateSegmentEndpoints(Vector3 origin, Vector3 target);
void validateSegmentHit(const SegmentHit& hit);

} // namespace usd_physics::core
