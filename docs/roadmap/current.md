# Current roadmap — Phase 2 `physicsJolt` extraction

> **Status: not started, 2026-09-22.** Phase 1 is complete. The next delivery
> slice moves the proven Stage Runner Jolt implementation behind the installed
> `physicsCore` contract without changing its observable behavior.

## 1. Outcome

Phase 2 produces an installed `physicsJolt` package that constructs a
solver-backed world while keeping every Jolt type, identifier, allocator, job
system, layer, and query implementation private.

```text
installed physicsCore contract
        -> physicsJolt world factory
        -> Jolt resource lifecycle and fixed stepping
        -> changed state, segment queries, and ground queries
```

## 2. Available prerequisites

- Phase 1 provides installed neutral values, typed handles, validated
  descriptors, typed errors, world lifecycle/state, and optional `SegmentQuery`
  and `GroundQuery` interfaces.
- Jolt Physics 5.5.0 at commit
  `23dadd0e603f1b321142d4c74df07fce85064989` is selected in the
  [dependency contract](../architecture/DEPENDENCIES.md).
- The Stage Runner extraction baseline records the proven initialization,
  lifecycle, stepping, changed-state, ignored-body segment, and support-contact
  behavior to preserve.
- The package scaffold, exported target, boundary checks, and clean-prefix
  installed-consumer path already exist.

## 3. Remaining Phase 2 work

- Record Jolt source or binary provenance, license, CMake target, ABI-affecting
  compile options, allocator policy, and job-system policy alongside the
  dependency contract.
- Replace the unavailable scaffold with a factory for a Jolt-backed
  `PhysicsWorld` while keeping backend headers and link details out of
  `physicsCore`.
- Implement neutral shape, body, constraint, force, velocity, state, and
  fixed-step conversion with the Phase 1 validation and error outcomes.
- Implement deterministic changed-state draining and explicit cleanup without
  exposing native IDs through public handles.
- Implement `SegmentQuery` with ignored-body behavior and `GroundQuery` with
  support body, normal, and distance results.
- Add focused backend tests for falling bodies, constraints, cleanup, changed
  state, closest segment hits, ignored bodies, and ground support.
- Verify plain CMake, clean-prefix installed consumption, OpenStrata
  composition, and the claimed Windows and Linux cells.

## 4. Phase 2 completion criteria

- Falling-body, cleanup, changed-state, segment-query, and ground-query tests
  pass through the installed `physicsCore` contract.
- Public `physicsCore` and `physicsJolt` headers expose no Jolt type or native
  resource identifier.
- The backend preserves Phase 1 stale, cross-world, validation, error, and
  changed-state ordering behavior.
- Package metadata and OpenStrata composition describe the same private Jolt
  dependency edge.
- The capability matrix claims only the backend behavior exercised by tests.
