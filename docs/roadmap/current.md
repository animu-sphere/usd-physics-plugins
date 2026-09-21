# Current roadmap — Phase 1 `physicsCore` extraction

> **Status: Phase 1 in progress, 2026-09-22.** Phase 0 closed with successful
> hosted graph, Linux, and Windows cells. Neutral values and typed handles are
> the first implemented extraction slice.

## 1. Outcome

Phase 1 produces an installed, solver-neutral `physicsCore` package that
preserves the Stage Runner behavior without depending on Stage Runner,
OpenUSD, or Jolt.

```text
values and handles
        -> descriptors and validation
        -> world lifecycle and state
        -> optional segment and ground queries
        -> Phase 2 Jolt extraction
```

## 2. Current evidence

- `Vector3`, `Quaternion`, and rigid `Transform` values are available from the
  installed `usd_physics/core/` include root.
- Typed 64-bit `ShapeHandle`, `BodyHandle`, and `ConstraintHandle` values
  preserve invalid zero, comparison, and hashing behavior.
- `physicsCore.values_handles` and the clean-prefix installed consumer cover
  this public slice.
- `RB-O1` has a proposed mixed error policy in the
  [rigid-body contract](../design/RIGID_BODY_CONTRACT.md#9-errors-and-validation).

## 3. Remaining Phase 1 work

- Extract box shape, static/dynamic body, and fixed-constraint descriptors
  against the neutral values and handles.
- Implement and test descriptor, vector, transform, and fixed-step validation;
  use those tests to accept or revise `RB-O1`.
- Extract the single-owner world lifecycle, force/velocity commands, direct
  state, fixed stepping, and changed-state draining contract.
- Settle `RB-O3` collision category/mask vocabulary before collision data is
  frozen in the public body descriptor.
- Settle `RB-O4` changed-state ordering across create, sleep, wake, teleport,
  and destroy before the world contract is accepted.
- Extract optional segment and ground query capabilities with ignored-body and
  support-contact behavior.
- Keep `PhysicsRuntime`, prim identity, dirty synchronization, and Stage
  traversal in Stage Runner.

## 4. Phase 1 completion criteria

- The full neutral extraction baseline is implemented under
  `usd_physics::core` and covered by deterministic tests.
- `physicsCore` public headers and link interfaces contain no OpenUSD, Jolt,
  Stage Runner, MMD, or VRM dependency.
- A clean-prefix consumer includes and exercises the runtime contract.
- `RB-O1`, `RB-O3`, and the Phase 1 portion of `RB-O4` are accepted or narrowed
  to explicitly deferred behavior.
- The capability matrix and package contract describe only the tested surface.
