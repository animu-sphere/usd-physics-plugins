#pragma once

namespace usd_physics::core {

struct Vector3 {
  double x{0.0};
  double y{0.0};
  double z{0.0};

  friend constexpr bool operator==(Vector3 left, Vector3 right) noexcept {
    return left.x == right.x && left.y == right.y && left.z == right.z;
  }

  friend constexpr bool operator!=(Vector3 left, Vector3 right) noexcept {
    return !(left == right);
  }
};

struct Quaternion {
  double w{1.0};
  double x{0.0};
  double y{0.0};
  double z{0.0};

  friend constexpr bool operator==(Quaternion left, Quaternion right) noexcept {
    return left.w == right.w && left.x == right.x && left.y == right.y &&
           left.z == right.z;
  }

  friend constexpr bool operator!=(Quaternion left, Quaternion right) noexcept {
    return !(left == right);
  }
};

struct Transform {
  Vector3 translation;
  Quaternion rotation;

  friend constexpr bool operator==(Transform left, Transform right) noexcept {
    return left.translation == right.translation && left.rotation == right.rotation;
  }

  friend constexpr bool operator!=(Transform left, Transform right) noexcept {
    return !(left == right);
  }
};

} // namespace usd_physics::core
