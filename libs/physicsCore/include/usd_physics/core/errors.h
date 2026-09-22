#pragma once

#include <stdexcept>
#include <string>

namespace usd_physics::core {

enum class PhysicsErrorCode {
  backendUnavailable,
  resourceExhausted,
  solverFailure,
};

class PhysicsError : public std::runtime_error {
public:
  PhysicsError(PhysicsErrorCode code, const std::string& message);

  [[nodiscard]] PhysicsErrorCode code() const noexcept { return code_; }

private:
  PhysicsErrorCode code_;
};

} // namespace usd_physics::core
