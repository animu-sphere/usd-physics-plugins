#include "usd_physics/usd/joint_conversion.h"
#include <cmath>
#include <limits>
#include <stdexcept>
#include <iostream>

using namespace usd_physics;
namespace {
void check(bool ok) { if (!ok) throw std::runtime_error("joint conversion mismatch"); }
template<class F> void rejects(F f) {
  try { f(); } catch (const usd::ImportError& e) {
    check(e.code() == usd::ImportErrorCode::unsupportedDeclaration && e.path() == "/Joint");
    return;
  }
  throw std::runtime_error("unsupported joint conversion accepted");
}
}
int main() {
  try {
    usd::Joint j; j.path = "/Joint"; j.body1 = "/Body";
    j.localFrame0.translation = {1,2,3};
    j.localFrame1.rotation = {2,0,0,0};
    j.collisionEnabled = true;
    const double inf = std::numeric_limits<double>::infinity();
    j.limits[0] = {true, -.5, .75};
    j.limits[1] = {true, 0, 0};
    j.limits[2] = {true, 1, -1};
    j.limits[3] = {true, -inf, inf};
    j.limits[4] = {true, -.2, .3};
    const auto d = *usd::toSixDofConstraint(j, {}, core::BodyHandle{1});
    check(!d.firstBody && d.secondBody == core::BodyHandle{1} &&
          d.firstFrame == j.localFrame0 && d.secondFrame == j.localFrame1 && d.collisionEnabled);
    check(d.limits[0].motion == core::AxisMotion::limited && d.limits[0].low == -.5 &&
          d.limits[0].high == .75 && d.limits[1].motion == core::AxisMotion::locked &&
          d.limits[2].motion == core::AxisMotion::locked &&
          d.limits[3].motion == core::AxisMotion::free &&
          d.limits[4].motion == core::AxisMotion::limited &&
          d.limits[5].motion == core::AxisMotion::free);
    for (const auto range : {usd::JointLimit{true, .2, .2}, {true, -inf, .3},
                             {true, -.3, inf}, {true, 0, std::nan("")},
                             {true, -4, 4}, {true, -inf, -inf}}) {
      auto bad = j; bad.limits[4] = range;
      rejects([&] { (void)usd::toSixDofConstraint(bad, {}, core::BodyHandle{1}); });
    }
    rejects([&] { (void)usd::toSixDofConstraint(j, core::BodyHandle{2}, core::BodyHandle{1}); });
    rejects([&] { (void)usd::toSixDofConstraint(j, {}, {}); });
    auto badFrame = j; badFrame.localFrame0.rotation = {0,0,0,0};
    rejects([&] { (void)usd::toSixDofConstraint(badFrame, {}, core::BodyHandle{1}); });
    j.type = usd::JointType::fixed;
    const auto fixed = usd::toSixDofConstraint(j, {}, core::BodyHandle{1});
    for (const auto& axis : fixed->limits)
      check(axis.motion == core::AxisMotion::locked);
    j.enabled = false;
    check(!usd::toSixDofConstraint(j, {}, {}));
  } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
