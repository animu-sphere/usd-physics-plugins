# Current roadmap — Phase 1 `physicsCore` extraction

> **Status: Phase 1 in progress, 2026-09-22.** Phase 0 closed with successful
> hosted graph, Linux, and Windows cells. Neutral values, typed handles, and
> validated descriptors are the implemented extraction slices.

## 1. Outcome

Phase 1 produces an installed, solver-neutral `physicsCore` package that
preserves the Stage Runner behavior without depending on Stage Runner,
OpenUSD, or Jolt.

```text
values, handles, and descriptors
        -> world lifecycle and state
        -> optional segment and ground queries
        -> Phase 2 Jolt extraction
```

## 2. Current evidence

- `Vector3`, `Quaternion`, and rigid `Transform` values are available from the
  installed `usd_physics/core/` include root.
- Typed 64-bit `ShapeHandle`, `BodyHandle`, and `ConstraintHandle` values
  preserve invalid zero, comparison, and hashing behavior.
- Box shape, static/dynamic body, and fixed-constraint descriptors preserve
  the Stage Runner extraction baseline against neutral transforms and handles.
- Descriptor validation rejects unsupported enum values, non-finite transforms,
  invalid dimensions, invalid masses, missing categories, and invalid handles
  with `std::invalid_argument`.
- `CollisionFilter` uses 64-bit `categories` and `collidesWith` sets; no
  backend layer identifier enters the public contract.
- `physicsCore.values_handles`, `physicsCore.descriptors`, and the clean-prefix
  installed consumer cover these public slices.
- `RB-O1` has a proposed mixed error policy in the
  [rigid-body contract](../design/RIGID_BODY_CONTRACT.md#9-errors-and-validation).
- `RB-O3` is resolved by the semantic category/mask contract in
  [the body descriptor](../design/RIGID_BODY_CONTRACT.md#52-bodies).

## 3. Remaining Phase 1 work

- Implement and test fixed-step validation; use the world tests to accept or
  revise the remaining `RB-O1` branches.
- Extract the single-owner world lifecycle, force/velocity commands, direct
  state, fixed stepping, and changed-state draining contract.
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
- `RB-O1` and the Phase 1 portion of `RB-O4` are accepted or narrowed to
  explicitly deferred behavior; the accepted `RB-O3` filter remains covered.
- The capability matrix and package contract describe only the tested surface.
