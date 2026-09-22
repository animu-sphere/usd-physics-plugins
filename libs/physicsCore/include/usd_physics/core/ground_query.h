#pragma once

#include "usd_physics/core/handles.h"
#include "usd_physics/core/value_types.h"

#include <optional>

namespace usd_physics::core {

struct GroundContact {
  BodyHandle supportBody;
  Vector3 normal{0.0, 1.0, 0.0};
  double distance{0.0};
};

// Optional backend capability for finding the closest supporting body beneath
// a body within a world-space probe distance.
class GroundQuery {
public:
  virtual ~GroundQuery();

  [[nodiscard]] virtual std::optional<GroundContact>
  groundContact(BodyHandle body, double maxDistance) const = 0;
};

void validateGroundProbeDistance(double maxDistance);
void validateGroundContact(const GroundContact& contact);

} // namespace usd_physics::core
