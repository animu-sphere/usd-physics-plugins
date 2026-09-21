#pragma once

#include <cstdint>

namespace usd_physics::core {

struct Version {
  std::uint16_t major;
  std::uint16_t minor;
  std::uint16_t patch;
};

[[nodiscard]] Version version() noexcept;

} // namespace usd_physics::core

