#include "usd_physics/jolt/availability.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>

namespace {
using namespace usd_physics;
using Rule = core::ContactCombineRule;

void check(bool condition, const char *message) {
  if (!condition)
    throw std::runtime_error(message);
}

// Retain the supplied world so tests can keep differently configured worlds
// alive together. Remove resources after each observation before reusing it.
double response(core::PhysicsWorld &world, double first, double second,
                bool sliding, int hz, double speed = 5) {
  const auto floor = world.createShape({core::ShapeType::box, {100, .5, 5}});
  core::ShapeDescriptor geometry;
  if (!sliding)
    geometry.type = core::ShapeType::sphere;
  const auto shape = world.createShape(geometry);
  const auto material = [sliding](double value) {
    return sliding ? core::ContactMaterial{value, 0}
                   : core::ContactMaterial{0, value};
  };
  const auto support = world.createBody({floor,
                                         core::MotionType::staticBody,
                                         {{0, -.5, 0}, {}},
                                         0,
                                         {},
                                         material(first),
                                         {0, 0}});
  const auto body = world.createBody({shape,
                                      core::MotionType::dynamicBody,
                                      {{0, sliding ? .51 : 3, 0}, {}},
                                      1,
                                      {},
                                      material(second),
                                      {0, 0}});
  const core::PhysicsWorld::Duration dt{1.0 / hz};
  if (sliding) {
    for (int i = 0; i != hz / 10; ++i)
      world.step(dt);
    check(world.setLinearVelocity(body, {speed, 0, 0}), "start sliding");
  }
  double upward = 0;
  for (int i = 0; i != (sliding ? hz / 2 : hz * 3 / 2); ++i) {
    world.step(dt);
    const auto velocity = world.bodyState(body).linearVelocity;
    check(std::isfinite(velocity.x) && std::isfinite(velocity.y),
          "finite contact response");
    upward = std::max(upward, velocity.y);
    world.takeChangedBodyStates();
  }
  const double result =
      sliding ? world.bodyState(body).linearVelocity.x : upward;
  check(world.destroyBody(body) && world.destroyBody(support) &&
            world.destroyShape(shape) && world.destroyShape(floor),
        "contact cleanup");
  return result;
}
} // namespace

int main() {
  try {
    // Simultaneously alive: changing one world's policy must not affect
    // another.
    auto defaults = jolt::createWorld();
    auto products = jolt::createWorld({Rule::product, Rule::product, 10});
    auto capped = jolt::createWorld({Rule::product, Rule::product, .25});
    auto alternatives =
        jolt::createWorld({Rule::maximum, Rule::geometricMean, {}});
    for (const int hz : {60, 240}) {
      const auto productSlide = response(*products, .25, 1, true, hz);
      const auto swappedSlide = response(*products, 1, .25, true, hz);
      const auto equalSlide = response(*products, .5, .5, true, hz);
      // Coulomb deceleration for a resting box, with damping disabled.
      check(std::abs(productSlide - (5 - .25 * 9.81 * .5)) < .08 &&
                std::abs(productSlide - swappedSlide) < .02 &&
                std::abs(productSlide - equalSlide) < .02,
            "product friction response");
      const auto limitedSlide = response(*capped, 4, 4, true, hz);
      check(std::abs(limitedSlide - productSlide) < .02,
            "friction ceiling reaches contacts");
      const auto highSlide = response(*products, 4, 4, true, hz, 100);
      const auto atCeiling = response(*products, 5, 2, true, hz, 100);
      check(std::abs(highSlide - atCeiling) < .02 && highSlide > 1,
            "friction product above 10 is capped at 10");
      const auto defaultSlide = response(*defaults, .25, 1, true, hz);
      check(defaultSlide > 2.4 && defaultSlide < 2.7 &&
                productSlide > defaultSlide + 1,
            "default friction isolated");
      check(std::abs(response(*alternatives, .25, 1, true, hz)) < .2,
            "maximum friction rule");
      const auto referenceBounce = response(*products, 1, 1, false, hz);
      const auto mixedBounce = response(*products, .25, .8, false, hz);
      const auto swappedBounce = response(*products, .8, .25, false, hz);
      check(std::abs(mixedBounce / referenceBounce - .2) < .025 &&
                std::abs(swappedBounce - mixedBounce) < .02,
            "product restitution response");
      check(response(*products, 0, .8, false, hz) < .1 &&
                response(*products, .8, 0, false, hz) < .1,
            "zero absorbs on either body");
      const auto defaultBounce = response(*defaults, .25, .8, false, hz);
      check(std::abs(defaultBounce / referenceBounce - .8) < .025,
            "default restitution isolated");
      check(std::abs(response(*alternatives, .25, .64, false, hz) /
                         referenceBounce -
                     .4) < .025,
            "geometric restitution rule");
      std::cout << hz << " Hz: product slide=" << productSlide
                << " capped=" << limitedSlide
                << " ceiling-10 slide=" << highSlide
                << " default slide=" << defaultSlide
                << " product bounce=" << mixedBounce
                << " default bounce=" << defaultBounce << '\n';
    }
    auto noFriction = jolt::createWorld({Rule::product, Rule::product, 0});
    check(response(*noFriction, 4, 4, true, 240) > 4.99,
          "zero friction ceiling");
    for (const double invalid : {std::numeric_limits<double>::max(),
                                 std::numeric_limits<double>::min()}) {
      bool rejected = false;
      try {
        static_cast<void>(
            jolt::createWorld({Rule::product, Rule::product, invalid}));
      } catch (const std::invalid_argument &) {
        rejected = true;
      }
      check(rejected, "unsafe backend ceiling rejected");
    }
    bool rejected = false;
    try {
      static_cast<void>(
          jolt::createWorld({static_cast<Rule>(-1), Rule::product, 10}));
    } catch (const std::invalid_argument &) {
      rejected = true;
    }
    check(rejected, "invalid rule rejected");
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
