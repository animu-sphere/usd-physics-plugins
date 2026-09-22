# 2026-09-22 Phase 2 Windows backend verification

## Scope

This report records the first local Windows verification of the extracted
`physicsJolt` implementation. It is evidence for the current tree, not a claim
that Linux, OpenStrata, or hosted Phase 2 verification has completed.

The Jolt package came from `jrouwe/JoltPhysics` tag `v5.5.0`, commit
`23dadd0e603f1b321142d4c74df07fce85064989`, installed with the
`Jolt::Jolt` CMake target. The compiler was MSVC 19.51.36256.0 through Visual
Studio 18 2026, targeting Windows SDK 10.0.26100.0.

## Configurations

Two Release configurations were exercised:

1. no Jolt package discoverable, retaining the typed unavailable fallback;
2. `PHYSICSJOLT_REQUIRE_JOLT=ON` with `Jolt_DIR` pointing at the pinned Jolt
   install.

The no-Jolt configuration passed all 10 tests, including boundary, docs,
version, and clean-prefix installed-consumer checks. The Jolt-enabled
configuration passed all 11 tests. Its additional `physicsJolt.backend` test
covered:

- world construction and optional capability discovery;
- static and dynamic box bodies, falling, and settling;
- semantic collision-filter rejection;
- fixed constraints and dependent cleanup;
- cross-world handle rejection;
- deterministic changed-state ordering and drain behavior;
- closest segment hits and ignored bodies; and
- support body, normal, and distance ground results.

The Jolt-enabled installed-consumer test installed both packages into a clean
prefix, resolved Jolt as a declared external dependency, linked the imported
targets, constructed a backend world, and ran successfully.

## Remaining evidence

This run does not close Phase 2. OpenStrata composition, Linux execution, and
the generated hosted Windows/Linux cells remain required by the current
roadmap.

## Follow-up (2026-09-23): OpenStrata `jolt` intent

After the initial plain-CMake run, the workspace added a `jolt` build intent
that sets `PHYSICSJOLT_REQUIRE_JOLT=ON`. With the same pinned Jolt installation
on `CMAKE_PREFIX_PATH`, `ost build --intent jolt` selected `Jolt::Jolt` and
completed the Release workspace build. `ost test --intent jolt` then passed
all 11 tests, including the focused backend and clean-prefix installed
consumer. The later Linux report also closes the local Linux item above;
hosted Windows/Linux cells remain the final Phase 2 evidence gate.
