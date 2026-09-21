# Current roadmap — Phase 0 and the extraction hand-off

> **Status: Phase 0 in progress, 2026-09-21.** The plain-CMake workspace and
> OpenStrata package scaffolds are verified locally. Hosted CI evidence remains
> before the phase closes.

## 1. Outcome

Phase 0 leaves an empty-but-real workspace that can build, test, install, and
be consumed through plain CMake and OpenStrata. Both modes and clean-prefix
consumption now pass; the remaining outcome is hosted delivery evidence.

```text
documentation contract
        -> verified workspace skeleton
        -> installable target scaffolds
        -> Phase 1 extraction
```

## 2. Remaining Phase 0 delivery work

- Run the generated hosted Windows and Linux CI cells and record their URLs or
  append-only reports.
- Propose the handle/lifecycle error policy `RB-O1` before accepting Phase 1
  public headers.

The completed namespace, math ownership, dependency selection, package
scaffolds, boundary tests, clean-prefix tests, OpenStrata build/test/package
verification, reproducible CI generation, and extraction audit are recorded
in architecture/reference pages rather than retained as roadmap work.

## 3. Phase 1 ready condition

Phase 1 starts when:

- the hosted and OpenStrata Phase 0 evidence is green;
- the handle/lifecycle error policy `RB-O1` has a proposed resolution.

The namespace, include root, package names, math ownership, extraction-source
revision, and runnable package/boundary tests are already fixed.

Phase 1 then moves behavior in small tested slices: values and handles,
descriptors and validation, world lifecycle and state, then optional queries.

## 4. Work deliberately deferred

Do not add `physicsUsd`, MMD joints, VRM secondary motion, vehicle types,
custom schemas, or broad query APIs during the bootstrap. Their phases exist to
ensure each addition arrives with its consumer fixture and correct dependency
boundary.
