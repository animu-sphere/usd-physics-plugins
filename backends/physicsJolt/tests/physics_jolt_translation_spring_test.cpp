#include "usd_physics/core/six_dof.h"
#include "usd_physics/jolt/availability.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>

using namespace usd_physics;
namespace {
void check(bool ok, const char* text) { if (!ok) throw std::runtime_error(text); }
template<class F> void rejects(F f) {
  try { f(); } catch (const std::invalid_argument&) { return; }
  throw std::runtime_error("invalid translation spring accepted");
}
core::Vector3 direction(int axis, bool rotated) {
  if (axis == 2) return {0,0,1};
  if (axis == 0) return rotated ? core::Vector3{0,1,0} : core::Vector3{1,0,0};
  return rotated ? core::Vector3{-1,0,0} : core::Vector3{0,1,0};
}
double coordinate(core::Vector3 value, core::Vector3 axis) {
  return value.x * axis.x + value.y * axis.y + value.z * axis.z;
}
// Independent continuous mass/spring solution; checks the force law, mass,
// equilibrium, axis frame and damping rather than finite-state stepping.
double response(int axis, bool rotated, bool worldEndpoint, bool limited,
                int hz, double mass, double stiffness, double dampingRatio) {
  auto w = jolt::createWorld();
  auto* capability = dynamic_cast<core::SixDofConstraints*>(w.get());
  check(capability != nullptr, "six-DOF capability missing");
  const auto shape = w->createShape({});
  const auto dir = direction(axis, rotated);
  const double equilibrium = -.1, displacement = .2;
  const double initial = equilibrium + displacement;
  const auto body = w->createBody({shape,core::MotionType::dynamicBody,
      {{initial*dir.x, 5+initial*dir.y, initial*dir.z},{}},mass,{1,0},{0,0},{0,0}});
  core::SixDofConstraintDescriptor d;
  d.secondBody = body;
  if (!worldEndpoint)
    d.firstBody = w->createBody({shape,core::MotionType::staticBody,{},0,{1,0}});
  d.firstFrame.translation = {0,5,0};
  if (rotated) d.firstFrame.rotation = {std::sqrt(.5),0,0,std::sqrt(.5)};
  d.secondFrame.rotation = d.firstFrame.rotation;
  for (auto& limit : d.limits) limit.motion = core::AxisMotion::locked;
  d.limits[axis] = limited ? core::AxisLimit{core::AxisMotion::limited,-.4,.4}
                         : core::AxisLimit{};
  const double omega = std::sqrt(stiffness/mass);
  const double decay = dampingRatio * omega;
  const double damped = omega * std::sqrt(1-dampingRatio*dampingRatio);
  d.translationSprings[axis] = {stiffness,2*mass*decay,equilibrium};
  const auto constraint = capability->createSixDofConstraint(d);
  double maxError = 0;
  for (int i = 1; i <= hz/2; ++i) {
    w->applyForce(body,{0,mass*9.81,0}); // Cancel world gravity independently.
    w->step(core::PhysicsWorld::Duration{1.0/hz});
    const auto state = w->bodyState(body);
    auto p = state.transform.translation; p.y -= 5;
    const double t = double(i)/hz;
    const double expected = equilibrium + displacement * std::exp(-decay*t) *
        (std::cos(damped*t)+decay/damped*std::sin(damped*t));
    maxError = std::max(maxError,std::abs(coordinate(p,dir)-expected)/displacement);
    const double along = coordinate(p,dir);
    check(std::abs(p.x-along*dir.x)<.001 && std::abs(p.y-along*dir.y)<.001 &&
          std::abs(p.z-along*dir.z)<.001, "spring leaks into another axis");
  }
  check(w->destroyConstraint(constraint), "spring resource cleanup");
  return maxError;
}
}
int main() {
  try {
    for (const int hz : {60,240}) {
      double worst = 0;
      for (int axis = 0; axis != 3; ++axis)
        for (const bool rotated : {false,true})
          for (const double mass : {1.,4.})
            for (const double stiffness : {25.,100.})
              for (const double ratio : {0.,.2})
                worst = std::max(worst,response(axis,rotated,axis!=1,axis!=2,
                                               hz,mass,stiffness,ratio));
      std::cout << hz << " Hz max normalized analytic error=" << worst << '\n';
      check(worst < (hz==60 ? .24 : .07), "spring analytic response outside timestep bound");
    }
    // Two moving endpoints use reduced mass, preserving center of mass.
    {
      auto pair = jolt::createWorld();
      auto& cap = *dynamic_cast<core::SixDofConstraints*>(pair.get());
      const auto shape = pair->createShape({});
      const auto first = pair->createBody({shape,core::MotionType::dynamicBody,
                                          {},1,{1,0},{0,0},{0,0}});
      const auto second = pair->createBody({shape,core::MotionType::dynamicBody,
                                           {{.2,0,0},{}},3,{1,0},{0,0},{0,0}});
      core::SixDofConstraintDescriptor d; d.firstBody=first; d.secondBody=second;
      d.translationSprings[0]={25,0,0};
      cap.createSixDofConstraint(d);
      double worst=0;
      for (int i=1; i<=120; ++i) {
        pair->step(core::PhysicsWorld::Duration{1.0/240});
        const auto a=pair->bodyState(first).transform.translation;
        const auto b=pair->bodyState(second).transform.translation;
        const double expected=.2*std::cos(std::sqrt(25*(1+1.0/3))*i/240);
        worst=std::max(worst,std::abs(b.x-a.x-expected)/.2);
        check(std::abs((a.x+3*b.x)/4-.15)<1e-5,"spring violates center of mass");
      }
      check(worst<.04,"two-body spring reduced mass response");
    }
    // A spring target beyond a hard limit must not soften that limit.
    {
      auto limited = jolt::createWorld();
      auto& cap = *dynamic_cast<core::SixDofConstraints*>(limited.get());
      const auto shape=limited->createShape({});
      const auto body=limited->createBody({shape,core::MotionType::dynamicBody,
                                          {},1,{1,0},{0,0},{0,0}});
      core::SixDofConstraintDescriptor d; d.secondBody=body;
      for (auto& axis : d.limits) axis.motion=core::AxisMotion::locked;
      d.limits[0]={core::AxisMotion::limited,-.1,.1};
      d.translationSprings[0]={25,10,.3};
      cap.createSixDofConstraint(d);
      for (int i=0; i<240; ++i) {
        limited->step(core::PhysicsWorld::Duration{1.0/240});
        check(limited->bodyState(body).transform.translation.x<.105,
              "spring overrides hard translation limit");
      }
      check(limited->bodyState(body).transform.translation.x>.095,
            "spring fails to load hard translation limit");
    }
    auto w = jolt::createWorld();
    auto& capability = *dynamic_cast<core::SixDofConstraints*>(w.get());
    const auto s = w->createShape({});
    const auto b = w->createBody({s,core::MotionType::dynamicBody});
    core::SixDofConstraintDescriptor d; d.secondBody=b;
    for (int axis=0; axis!=3; ++axis) {
      for (const double bad : {1e300,1e-300}) {
        auto invalid=d; invalid.translationSprings[axis]={bad,0,0};
        rejects([&] { capability.createSixDofConstraint(invalid); });
        invalid.translationSprings[axis]={1,bad,0};
        rejects([&] { capability.createSixDofConstraint(invalid); });
        invalid.translationSprings[axis]={1,0,bad};
        rejects([&] { capability.createSixDofConstraint(invalid); });
      }
    }
    std::cout << "translation spring controls passed\n";
  } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
