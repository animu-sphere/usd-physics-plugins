# Current roadmap — Phase 0 and the extraction hand-off

> **Status: Phase 0 in progress, 2026-09-21.** The documentation contract is
> the first deliverable. No code component exists yet.

## 1. Outcome

Phase 0 leaves an empty-but-real workspace that can build, test, install, and
be consumed through plain CMake and OpenStrata. It makes dependency and package
decisions needed to extract `physicsCore` without copying Stage Runner's
application coupling.

```text
documentation contract
        -> verified workspace skeleton
        -> installable target scaffolds
        -> Phase 1 extraction
```

## 2. Documentation contract

- Establish the same category ownership used by `usd-mmd-plugins`.
- Adopt the central design policy and focused contracts.
- Record the workspace graph, candidate dependencies, and planned packages.
- Publish an honest all-empty capability matrix.
- Keep guides, reports, and releases absent until real evidence exists.

Completion: all repository-relative Markdown links resolve, every page has one
owner, and no planned runtime behavior is presented as implemented.

## 3. Workspace skeleton

- Add the root CMake project, `VERSION`, presets, OpenStrata workspace and CI
  matrix.
- Fix `ARCH-O1`: public namespace and include root.
- Resolve `ARCH-O2` and `RB-O2`: neutral math types inside `physicsCore` or a
  justified separate package.
- Scaffold `physicsCore` and `physicsJolt` with manifests and package configs.
- Select and record exact OpenUSD, Jolt, OpenStrata, compiler, and platform
  versions in [DEPENDENCIES.md](../architecture/DEPENDENCIES.md).
- Add boundary checks for forbidden includes and link edges.
- Add installed-consumer tests from a clean prefix.

Completion: the root builds and tests in both build modes, install exports are
clean, and the capability matrix cites the evidence without claiming runtime
physics support.

## 4. Extraction audit

Inventory the current `usd-stage-runner` sources before moving them:

```text
libs/physicsCore/
  handles.h
  physics_shape.h
  physics_body.h
  physics_constraint.h
  physics_world.h
  collision_query.h
  ground_query.h
  physics_runtime.h

backends/physicsJolt/
  jolt_physics_world.h
  jolt_physics_world.cpp
  backend tests and package metadata
```

Classify each item as:

- neutral physics behavior to preserve;
- Stage Runner runtime coupling to replace;
- backend implementation detail to keep private;
- application orchestration that stays in Stage Runner.

The audit must call out the current `physicsCore -> runtimeCore` dependency and
the `PrimId <-> BodyHandle` mapping explicitly. The intended split is recorded
in [WORKSPACE.md §7](../architecture/WORKSPACE.md#7-extraction-rule).

## 5. Phase 1 ready condition

Phase 1 starts when:

- the public namespace, include root, and package names are fixed;
- neutral math types are owned outside Stage Runner;
- exact extraction-source revision is recorded;
- package and boundary tests are runnable;
- the handle/lifecycle error policy `RB-O1` has a proposed resolution.

Phase 1 then moves behavior in small tested slices: values and handles,
descriptors and validation, world lifecycle and state, then optional queries.

## 6. Work deliberately deferred

Do not add `physicsUsd`, MMD joints, VRM secondary motion, vehicle types,
custom schemas, or broad query APIs during the bootstrap. Their phases exist to
ensure each addition arrives with its consumer fixture and correct dependency
boundary.
