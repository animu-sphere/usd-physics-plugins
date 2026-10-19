# Current roadmap

Implementation status and evidence belong in the
[capability matrix](../reference/CAPABILITY_MATRIX.md). This page owns ordered,
unfinished work only; keep Active to three to five outcomes.

## Active

### P0-A — Build and package acceptance

- Outcome: Independently installable core/backend/optional USD packages with
  equivalent plain-CMake and OpenStrata dependency graphs.
- Acceptance: Verify hosted Windows/Linux CI after the CMake cleanup, including
  No-Jolt fallback and clean-prefix consumers without SDKs, with Jolt, and with
  USD. Preserve public targets, headers and compatibility rejection; attach
  hosted evidence separately from local results.
- Tracking: [Package acceptance](../architecture/PACKAGE_CONTRACT.md#7-acceptance);
  Issue/PR to be linked when opened.

### P0-B — Read-only scene gravity

- Outcome: A validated `UsdPhysicsScene` gravity snapshot for the minimal
  standard-USD execution path, without retaining a runtime world.
- Acceptance: Define scene selection, units and typed code/path rejection;
  verify a generated fixture and keep `physicsUsd` independent of Jolt.
- Tracking: [Bridge contract](../design/USD_BRIDGE_CONTRACT.md);
  Issue/PR to be linked when opened. Deliver separately from consumer rebuild.

### P0-B — Consumer rebuild and state presentation

- Outcome: Stage Runner runs a static floor, dynamic Box, gravity and the
  minimum necessary joint from standard USD, using explicit full rebuild.
- Acceptance: Consumer owns path/handle mappings, fixed stepping and display
  transforms. Reload destroys and rebuilds resources; failure cleans up without
  exposing a partial world; stale handles reject. Windows/Linux fixtures show
  falling, contact and rest within documented tolerances. Keep standard import
  opt-in and compatibility fixtures until host/runtime parity passes.
- Tracking: [Consumer ownership](../design/DESIGN_POLICY.md#101-stage-runner);
  Issue/PR to be linked when opened. Deliver rebuild and presentation separately.

### P1-B — Release contract acceptance

- Outcome: Validated public USD artifacts and a versioned release usable without
  a repository checkout.
- Acceptance: Dry-run the complete Windows/Linux release set before tagging;
  validate clean-prefix and downstream consumers, artifact pins, API/ABI/version
  handling and a copyable package example against the release schema.
- Tracking: [Release gates](../architecture/RELEASE_SCHEMA.md#3-promotion-gates);
  Issue/PR to be linked when opened.

## Next

- P1-A: Validate a small MMD consumer slice with generated fixtures and a few
  redistributable real models. Format conversion, filtering, damping and bone
  coupling remain in the source adapter. Require finite state, sound resource
  lifetime and clear failure; report numerical differences without requiring
  exact Bullet trajectories. See [MMD ownership](../design/DESIGN_POLICY.md#102-mmd).

## Deferred

- P2: Admit secondary motion only after one concrete VRM end-to-end slice proves
  the shared descriptor and CPU solver boundary; see the
  [secondary-motion contract](../design/SECONDARY_MOTION_CONTRACT.md).
- P3: Incremental stage synchronization, bidirectional authoring, comprehensive
  compound colliders, additional solvers, character/vehicle abstractions,
  exact Bullet fidelity and advanced queries wait for a demonstrated consumer
  need. See [admission discipline](../design/DESIGN_POLICY.md#17-open-design-discipline).
