# Current roadmap — Phase 1 `physicsCore` extraction

> **Status: Phase 1 in progress, 2026-09-22.** Phase 0 closed with successful
> hosted graph, Linux, and Windows cells. Neutral values, typed handles, and
> validated descriptors plus the world lifecycle/state contract are the
> implemented extraction slices.

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
- `physicsCore.world` covers fixed-step/vector validation, explicit resource
  lifecycle, commands, direct state, deterministic changed-state draining,
  stale and cross-world handle behavior, and typed backend errors.
- `RB-O1` is resolved by the tested mixed error policy in the
  [rigid-body contract](../design/RIGID_BODY_CONTRACT.md#9-errors-and-validation),
  and the Phase 1 portion of `RB-O4` fixes changed-state drain ordering.
- `RB-O3` is resolved by the semantic category/mask contract in
  [the body descriptor](../design/RIGID_BODY_CONTRACT.md#52-bodies).

## 3. Remaining Phase 1 work

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
- The accepted `RB-O1`, `RB-O3`, and Phase 1 `RB-O4` behavior remains covered.
- The capability matrix and package contract describe only the tested surface.
