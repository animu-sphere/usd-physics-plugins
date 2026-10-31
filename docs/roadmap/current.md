# Current roadmap

Implementation status and evidence belong in the
[capability matrix](../reference/CAPABILITY_MATRIX.md). This page owns ordered,
unfinished work only; keep Active to three to five outcomes.

The avatar integration sequence is P1 USD snapshots/diagnostics -> P2 Jolt
body/constraint MVP -> P3 shared-world lifecycle -> P4 external MMD adapter
smoke -> P5 VRM secondary motion (deferred below). Extend the existing
implementation rather than duplicating it.

## Active

### P1 — USD snapshots and integration diagnostics

- Outcome: Standard `UsdPhysics` inputs produce neutral snapshots with explicit
  validation and diagnostics for the avatar body/joint MVP.
- Acceptance: Cover sphere/box/capsule, dynamic/kinematic bodies, mass, friction,
  restitution, damping, gravity, endpoint/local frames and required linear/angular
  limits under the reader's declared admission rules. Unsupported or unreadable
  values never silently fall back. Structured failures identify code, operation
  and source object; approximation reports remain distinct from rejection.
  Keep `mmd:physics:*`, format coefficients and bone bindings outside the reader.
- Tracking: [USD input contract](../design/USD_BRIDGE_CONTRACT.md#3-initial-planned-slice),
  [diagnostic proposal](../design/RIGID_BODY_CONTRACT.md#91-integration-diagnostics-proposal);
  Issue/PR to be linked when opened.

### P2 — Jolt body and constraint MVP acceptance

- Outcome: A standalone neutral body/joint slice executes reproducibly through
  Jolt without format or renderer dependencies.
- Acceptance: Exercise kinematic/dynamic contact, primitive shapes, body/material
  values, local joint frames and required limits. Replay identical ordered inputs
  with fixed dt, declared substep/catch-up limits and seed policy in the same
  configuration/environment. Return explicit backend-limit and creation failures
  with cleanup. Treat six-DOF spring integration as a subsequent, explicitly
  reported approximation; do not require cross-platform bitwise or Bullet parity.
- Tracking: [Runtime contract](../design/RIGID_BODY_CONTRACT.md#7-stepping-and-state),
  [six-DOF boundary](../design/RIGID_BODY_CONTRACT.md#54-optional-six-dof-constraints);
  Issue/PR to be linked when opened.

### P3 — Shared-world lifecycle and consumer presentation

- Outcome: Host-injected worlds support the common avatar scheduler and Stage
  Runner's standard-USD rebuild/presentation path.
- Acceptance: All avatars finish pre-physics before one step per world/tick;
  every avatar receives post-step state before bone feedback. Define ownership
  and API/ABI evolution against the existing interfaces. Reset/seek/loop rebuild
  without stale handles or residual velocities; teleport has explicit velocity
  handling. Generation-tagged copied/interpolated render state never changes
  canonical physics state. Preserve Stage Runner's floor/dynamic Box/gravity/joint
  acceptance, failure rollback and Windows/Linux falling/contact/rest checks;
  keep standard import opt-in and compatibility fixtures until parity passes.
- Tracking: [Lifecycle proposal](../design/RIGID_BODY_CONTRACT.md#63-avatar-integration-vocabulary-proposal),
  [scheduler barrier](../design/RIGID_BODY_CONTRACT.md#73-shared-world-scheduling),
  [world gravity](https://github.com/animu-sphere/usd-physics-plugins/pull/30) and
  [consumer rebuild](https://github.com/animu-sphere/usd-stage-runner/pull/35).
  Shared-world Issue/PR to be linked when opened; deliver rebuild and presentation
  separately.

### P4 — External MMD adapter integration smoke

- Outcome: A small MMD consumer slice validates the generic world through the
  adapter owned by `usd-mmd-plugins`.
- Acceptance: Use generated fixtures and a few redistributable real models.
  Keep coefficient conversion, 16-group/mask mapping and bone coupling in the
  MMD adapter; the host allocates shared-world category bits across avatars.
  Require finite state, sound lifetime, basic joints, usable bone feedback and
  one world step per tick with multiple avatars. No direct MMD importer or
  `hydra-toon` dependency enters generic physics. Report spring approximations
  and numerical differences; exact Bullet trajectories are not a gate.
- Tracking: [MMD ownership](../design/DESIGN_POLICY.md#102-mmd);
  Issue/PR to be linked when opened.

## Deferred

- P5: Admit VRM secondary motion after a concrete end-to-end slice proves the
  shared descriptor and CPU solver boundary. Keep SpringBone chain/Verlet solving
  independent of Jolt rigid bodies; prioritize evaluation contracts, collider
  descriptors and diagnostics reuse. See the
  [secondary-motion contract](../design/SECONDARY_MOTION_CONTRACT.md).
- Further expansion: Incremental stage synchronization, bidirectional authoring,
  comprehensive compound colliders, additional solvers, character/vehicle
  abstractions, exact Bullet fidelity and advanced queries wait for a demonstrated
  consumer need. See
  [admission discipline](../design/DESIGN_POLICY.md#17-open-design-discipline).
