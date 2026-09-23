# Current roadmap — Phase 3 Stage Runner migration

> **Status: in progress, 2026-09-23.** Phase 2 is complete: the installed
> `physicsCore` and `physicsJolt` packages pass local and hosted Windows/Linux
> verification. The next slice migrates Stage Runner to those packages while
> preserving its orchestration and authored compatibility behavior.

## 1. Outcome

Phase 3 makes `usd-stage-runner` a consumer of the installed packages instead
of an owner of the neutral physics and Jolt backend implementations.

```text
installed physicsCore + physicsJolt
        -> Stage Runner composition boundary
        -> fixed-step orchestration and gameplay capabilities
        -> incremental runtime-layer synchronization
```

## 2. Available prerequisites

- Phase 1 provides the installed neutral values, handles, descriptors, errors,
  world contract, and optional segment and ground query capabilities.
- Phase 2 provides the installed Jolt-backed world factory with passing local
  and hosted Windows/Linux evidence.
- The Stage Runner extraction inventory fixes the current public surface,
  consumers, compatibility importer, fixed-step order, and deterministic parity
  evidence at its recorded source revision.
- `StageSession::PhysicsWorldFactory` already gives the standalone and usdview
  hosts one backend-selection seam.

## 3. Completed locally

- Replaced Stage Runner's repository-local `physicsCore` and `physicsJolt`
  source edges with installed-package discovery and equivalent OpenStrata
  requirements.
- Adapted Stage Runner includes, namespace usage, and math values explicitly at
  the composition boundary.
- Moved prim/body mapping, dirty synchronization, and fixed-step delegation from
  the local `PhysicsRuntime` into `stageRuntime` without moving `PrimId` or
  `RuntimeWorld` into the external package.
- Kept the Runner physics schema importer in Stage Runner as a temporary
  compatibility path that produces neutral external descriptors.
- Preserved `StageSession::PhysicsWorldFactory` and used the same external Jolt
  factory in standalone and usdview hosts.
- Removed the repository-local physics libraries after the installed package
  path passed all 48 local Windows parity tests.
- Published the pinned Windows packages as public, immutable OCI artifacts and
  verified both OCI and content digests from a fresh cache.
- Published equivalent Linux packages and verified their content and OCI
  digests from a fresh cache; see the
  [Linux artifact report](../reports/2026-09-23-phase3-linux-artifacts.md).

## 4. Remaining Phase 3 work

- Run the Stage Runner hosted Windows and Linux suites from fresh artifact
  caches and record the evidence.
- Implement and dry-run the versioned release workflow against the
  [release schema](../architecture/RELEASE_SCHEMA.md) before creating a tag;
  the currently published Windows and Linux artifacts are Phase 3 inputs, not a full
  release.

## 5. Phase 3 completion criteria

- Existing falling-body, character grounding, jump-support, and camera
  collision scenarios pass against installed `physicsCore` and `physicsJolt`.
- Stage Runner no longer owns the reusable neutral physics implementation or
  the Jolt backend, and no Jolt type crosses a public Stage Runner boundary.
- Prim/body mapping, fixed-step ordering, changed-state synchronization, and
  discardable runtime-layer behavior remain owned and tested by Stage Runner.
- Runner physics schema fixtures continue to work through the local
  compatibility importer; no new `runner:physics:*` property is added.
- Plain CMake, OpenStrata, standalone, and usdview resolve the same package and
  world-factory graph.
