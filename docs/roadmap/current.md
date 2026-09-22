# Current roadmap — Phase 2 `physicsJolt` extraction

> **Status: in progress, 2026-09-23.** The Jolt-backed runtime and Windows/Linux
> plain-CMake/installed-consumer slices are implemented, and the Jolt-required
> OpenStrata intent passes locally on Windows. Hosted evidence remains before
> Phase 2 is complete.

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

- Run the configured Jolt-enabled hosted Windows/Linux Phase 2 cells and
  record dated evidence.
- Reconcile any platform-specific Jolt compile options discovered by those
  cells with the dependency contract before changing the capability claim to
  fully supported.

## 4. Phase 2 completion criteria

- Falling-body, cleanup, changed-state, segment-query, and ground-query tests
  pass through the installed `physicsCore` contract.
- Public `physicsCore` and `physicsJolt` headers expose no Jolt type or native
  resource identifier.
- The backend preserves Phase 1 stale, cross-world, validation, error, and
  changed-state ordering behavior.
- Package metadata and the `jolt` OpenStrata intent describe the same private
  Jolt dependency edge.
- The capability matrix claims only the backend behavior exercised by tests.
