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
  throw std::runtime_error("invalid six-DOF accepted");
}
void step(core::PhysicsWorld& w, int count = 240) {
  for (int i = 0; i < count; ++i) w.step(core::PhysicsWorld::Duration{1.0/120});
}
core::SixDofConstraints& capability(core::PhysicsWorld& w) {
  auto* result = dynamic_cast<core::SixDofConstraints*>(&w);
  check(result != nullptr, "six-DOF capability");
  return *result;
}
}
int main() {
  try {
    // All six locked axes hold a displaced collider at its body origin.
    auto w = jolt::createWorld();
    core::ShapeDescriptor shape;
    shape.localTransform.translation = {0.3, 0.2, 0};
    const auto s = w->createShape(shape);
    const auto b = w->createBody({s, core::MotionType::dynamicBody, {{1, 4, 0}, {}}, 2, {1, 0}});
    core::SixDofConstraintDescriptor d;
    d.secondBody = b;
    d.firstFrame.translation = {1, 4, 0};
    for (auto& axis : d.limits) axis.motion = core::AxisMotion::locked;
    const auto c = capability(*w).createSixDofConstraint(d);
    step(*w);
    auto p = w->bodyState(b).transform.translation;
    check(std::abs(p.x - 1) < .003 && std::abs(p.y - 4) < .003,
          "world lock holds body-origin frame with offset COM");
    check(w->destroyConstraint(c) && !w->destroyConstraint(c), "constraint destruction");
    step(*w, 30);
    check(w->bodyState(b).transform.translation.y < 3.8, "released world lock falls");
    rejects([&] { auto bad = d; bad.firstBody = b; capability(*w).createSixDofConstraint(bad); });
    auto other = jolt::createWorld();
    const auto os = other->createShape({});
    const auto ob = other->createBody({os, core::MotionType::dynamicBody});
    rejects([&] { auto bad = d; bad.secondBody = ob; capability(*w).createSixDofConstraint(bad); });
    for (const double value : {std::numeric_limits<double>::quiet_NaN(),
                               std::numeric_limits<double>::infinity(), 1e300}) {
      rejects([&] { auto bad = d; bad.firstFrame.translation.x = value;
                     capability(*w).createSixDofConstraint(bad); });
    }
    rejects([&] { auto bad = d; bad.secondFrame.rotation = {0,0,0,0};
                   capability(*w).createSixDofConstraint(bad); });
    // Rotating both joint frames by 90 degrees makes transX follow world Y.
    auto slider = jolt::createWorld();
    const auto ss = slider->createShape({});
    const auto sb = slider->createBody({ss, core::MotionType::dynamicBody, {{0, 5, 0}, {}}, 1, {1, 0}});
    core::SixDofConstraintDescriptor sd;
    sd.secondBody = sb;
    sd.firstFrame = {{0, 5, 0}, {std::sqrt(.5), 0, 0, std::sqrt(.5)}};
    sd.secondFrame.rotation = sd.firstFrame.rotation;
    for (auto& axis : sd.limits) axis.motion = core::AxisMotion::locked;
    sd.limits[0] = {core::AxisMotion::limited, -.25, .5};
    capability(*slider).createSixDofConstraint(sd);
    step(*slider);
    const auto sp = slider->bodyState(sb).transform.translation;
    check(std::abs(sp.x) < .003 && std::abs(sp.y - 4.75) < .008,
          "rotated asymmetric translation limit");
    for (int axis = 0; axis != 3; ++axis) {
      auto tw = jolt::createWorld();
      const auto ts = tw->createShape({});
      const auto tb = tw->createBody({ts, core::MotionType::dynamicBody, {{0,5,0},{}},1,{1,0}});
      core::SixDofConstraintDescriptor td; td.secondBody = tb;
      td.firstBody = tw->createBody({ts, core::MotionType::staticBody, {}, 0, {1,0}});
      td.firstFrame.translation = {0,5,0};
      for (auto& a : td.limits) a.motion = core::AxisMotion::locked;
      td.limits[axis] = {core::AxisMotion::limited, -.2, .4};
      capability(*tw).createSixDofConstraint(td);
      double peak = 0;
      for (int i = 0; i < 120; ++i) {
        tw->applyForce(tb, {20,20,20});
        step(*tw, 1);
        const auto sample = tw->bodyState(tb).transform.translation;
        const double components[] = {sample.x, sample.y - 5, sample.z};
        peak = std::max(peak, components[axis]);
        // Position correction is iterative; allow transient limit overshoot
        // during acceleration, independently of the resting slider tolerance.
        check(components[axis] >= -.23 && components[axis] <= .43,
              "translation stays within limit under load");
      }
      const auto pos = tw->bodyState(tb).transform.translation;
      const double values[] = {pos.x, pos.y - 5, pos.z};
      check(peak > .35, "load reaches upper translation bound");
      for (int a = 0; a < 3; ++a)
        check(a == axis || std::abs(values[a]) < .008,
              "each translation axis limits independently");
    }
    // Each angular axis is independently limited; gravity on an offset COM
    // generates torque without introducing a backend-specific torque API.
    for (int axis = 0; axis != 3; ++axis) {
      auto aw = jolt::createWorld();
      core::ShapeDescriptor as;
      as.localTransform.translation = axis == 0 ? core::Vector3{0,0,1} : core::Vector3{1,0,0};
      const auto ash = aw->createShape(as);
      core::Transform initial{{0,5,0}, {}};
      // For Y, rotate the joint frame so its Y points along world Z.
      const auto ab = aw->createBody({ash, core::MotionType::dynamicBody, initial, 1, {1,0}});
      core::SixDofConstraintDescriptor ad;
      ad.secondBody = ab; ad.firstFrame.translation = {0,5,0};
      if (axis == 1) {
        ad.firstFrame.rotation = {std::sqrt(.5), std::sqrt(.5),0,0};
        ad.secondFrame.rotation = ad.firstFrame.rotation;
      }
      for (auto& a : ad.limits) a.motion = core::AxisMotion::locked;
      ad.limits[3 + axis] = {core::AxisMotion::limited, -.2, .35};
      capability(*aw).createSixDofConstraint(ad);
      step(*aw);
      const auto q = aw->bodyState(ab).transform.rotation;
      const double angle = 2 * std::acos(std::min(1.0, std::abs(q.w)));
      check(angle > .15 && angle < .38, "angular limit resists offset-COM torque");
    }
    // Free axes really move, and dependent constraints disappear with a body.
    auto fw = jolt::createWorld();
    const auto fs = fw->createShape({});
    const auto fb = fw->createBody({fs, core::MotionType::dynamicBody, {{0,5,0},{}},1,{1,0}});
    core::SixDofConstraintDescriptor fd; fd.firstBody = fb;
    const auto fc = capability(*fw).createSixDofConstraint(fd);
    step(*fw, 60);
    check(fw->bodyState(fb).transform.translation.y < 4, "free axes fall");
    check(fw->destroyBody(fb) && !fw->destroyConstraint(fc), "body destroys dependent six-DOF");
    rejects([&] { capability(*fw).createSixDofConstraint(fd); });
    // Collision exclusions are reference-counted across parallel constraints.
    auto cw = jolt::createWorld();
    const auto cs = cw->createShape({});
    const auto ground = cw->createBody({cs, core::MotionType::staticBody, {{0,0,0},{}}});
    const auto ball = cw->createBody({cs, core::MotionType::dynamicBody, {{0,2,0},{}}});
    core::SixDofConstraintDescriptor cd; cd.firstBody = ground; cd.secondBody = ball;
    const auto c1 = capability(*cw).createSixDofConstraint(cd);
    const auto c2 = capability(*cw).createSixDofConstraint(cd);
    check(cw->destroyConstraint(c1), "remove first exclusion");
    step(*cw, 80);
    check(cw->bodyState(ball).transform.translation.y < -.1,
          "remaining exclusion disables pair contacts");
    check(cw->destroyConstraint(c2), "remove final exclusion");
    cw->setLinearVelocity(ball, {0,8,0});
    step(*cw, 30);
    check(cw->bodyState(ball).transform.translation.y < -.9,
          "last exclusion removal restores pair contacts");
    cw->destroyBody(ball);
    const auto replacement = cw->createBody({cs, core::MotionType::dynamicBody, {{0,2,0},{}}});
    cd.secondBody = replacement; cd.collisionEnabled = true;
    const auto allowed = capability(*cw).createSixDofConstraint(cd);
    step(*cw);
    check(std::abs(cw->bodyState(replacement).transform.translation.y - 1) < .02,
          "collision enabled preserves ordinary contacts");
    cw->destroyConstraint(allowed);
    cd.collisionEnabled = false;
    const auto removed = capability(*cw).createSixDofConstraint(cd);
    cw->destroyBody(ground);
    check(!cw->destroyConstraint(removed), "body removal cleans up excluded pair");
    cw->createBody({cs, core::MotionType::staticBody});
    step(*cw);
    check(std::abs(cw->bodyState(replacement).transform.translation.y - 1) < .02,
          "reused backend body slot inherits no exclusions");
    std::cout << "six-DOF controls passed\n";
  } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
