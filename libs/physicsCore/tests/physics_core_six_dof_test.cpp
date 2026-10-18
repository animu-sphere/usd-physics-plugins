#include "usd_physics/core/six_dof.h"
#include <limits>
#include <stdexcept>

using namespace usd_physics::core;
template<class F> bool rejects(F f) {
  try { f(); } catch (const std::invalid_argument&) { return true; }
  return false;
}
int main() {
  SixDofConstraintDescriptor d;
  if (!rejects([&] { validateSixDofConstraintDescriptor(d); })) return 1;
  d.firstBody = BodyHandle{1};
  validateSixDofConstraintDescriptor(d);
  d.secondBody = d.firstBody;
  if (!rejects([&] { validateSixDofConstraintDescriptor(d); })) return 2;
  d.secondBody = BodyHandle{2};
  const double inf = std::numeric_limits<double>::infinity();
  const double nan = std::numeric_limits<double>::quiet_NaN();
  for (std::size_t index = 0; index != 6; ++index) {
    for (const auto range : {AxisLimit{AxisMotion::limited, nan, 1},
                             {AxisMotion::limited, -inf, 1},
                             {AxisMotion::limited, 0, inf},
                             {AxisMotion::limited, 1, 1},
                             {AxisMotion::limited, 2, 1},
                             {static_cast<AxisMotion>(99), 0, 0}}) {
      auto bad = d; bad.limits[index] = range;
      if (!rejects([&] { validateSixDofConstraintDescriptor(bad); })) return 3;
    }
    d.limits[index] = {AxisMotion::limited, -.2, .5};
    validateSixDofConstraintDescriptor(d);
  }
  for (const auto pose : {Transform{{inf,0,0},{}}, Transform{{},{0,0,0,0}},
                          Transform{{},{nan,0,0,0}}}) {
    auto bad = d; bad.firstFrame = pose;
    if (!rejects([&] { validateSixDofConstraintDescriptor(bad); })) return 4;
    bad = d; bad.secondFrame = pose;
    if (!rejects([&] { validateSixDofConstraintDescriptor(bad); })) return 5;
  }
  d.limits[3] = {AxisMotion::limited, -4, 4};
  if (!rejects([&] { validateSixDofConstraintDescriptor(d); })) return 6;
  d.limits[3] = {AxisMotion::locked, nan, inf};
  validateSixDofConstraintDescriptor(d);
  for (std::size_t axis = 0; axis != 3; ++axis) {
    for (const auto spring : {TranslationSpring{-1,0,0}, {inf,0,0}, {nan,0,0},
                              {1,-1,0}, {1,inf,0}, {1,nan,0},
                              {1,0,inf}, {1,0,nan}, {0,1,0}}) {
      auto bad = d; bad.translationSprings[axis] = spring;
      if (!rejects([&] { validateSixDofConstraintDescriptor(bad); })) return 7;
    }
    auto value = d; value.translationSprings[axis] = {25,2,-.1};
    validateSixDofConstraintDescriptor(value);
    value.limits[axis].motion = AxisMotion::free;
    validateSixDofConstraintDescriptor(value);
    value.limits[axis].motion = AxisMotion::locked;
    if (!rejects([&] { validateSixDofConstraintDescriptor(value); })) return 8;
  }
}
