# 2026-09-22 Phase 2 Linux backend verification

## Scope

This report records local Linux verification of the extracted `physicsJolt`
implementation under WSL. It complements the Windows backend report but does
not replace OpenStrata or hosted CI evidence.

The Jolt package came from `jrouwe/JoltPhysics` tag `v5.5.0`, commit
`23dadd0e603f1b321142d4c74df07fce85064989`, installed with the
`Jolt::Jolt` CMake target. The compiler was GCC 15.2.0 and the generator was
Ninja Multi-Config.

## Result

The Release root build completed with `PHYSICSJOLT_REQUIRE_JOLT=ON`. All 11
tests passed, including:

- the focused Jolt backend lifecycle, filtering, stepping, changed-state,
  segment-query, and ground-query test;
- source and public-header boundary checks;
- documentation and version checks; and
- clean-prefix installation, downstream configure/link, world construction,
  and execution through the imported packages.

## Remaining evidence

Phase 2 still requires the Jolt-enabled OpenStrata dependency edge and the
generated hosted Windows/Linux cells recorded by the current roadmap.
