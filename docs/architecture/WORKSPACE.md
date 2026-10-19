# Workspace contract

This binding contract owns component identities, directories, dependency
edges, build modes and structural invariants. Structural changes update it
before or with implementation. Component creation and capability status belong
only in the [capability matrix](../reference/CAPABILITY_MATRIX.md).

## 1. Identities

### 1.1 Initial rigid-body path

| Identity | Kind | Directory | Manifest | Role |
| --- | --- | --- | --- | --- |
| `physicsCore` | plain static CMake library | `libs/physicsCore/` | `openstrata.library.yaml` | Neutral units, values, handles, descriptors, world, state, queries and errors; no OpenUSD or SDK types. |
| `physicsJolt` | plain static CMake library | `backends/physicsJolt/` | `openstrata.library.yaml` | Private Jolt conversion, lifecycle, stepping, collision and constraints. |
| `physicsUsd` | plain static CMake library | `libs/physicsUsd/` | `openstrata.library.yaml` | Read-only standard USD validation and neutral snapshots; no Jolt or retained world/mappings. |

Consumers own world construction, path/handle mappings, reload/full rebuild,
frame updates and presentation. Source adapters own format defaults, filters,
damping conversion and bone coupling. Optional neutral capabilities do not
change these component boundaries.

### 1.2 Later, only when the responsibility is real

| Identity | Kind | Directory | Role | Created when |
| --- | --- | --- | --- | --- |
| `secondaryMotion` | plain static CMake library | `libs/secondaryMotion/` | Format-neutral spring-chain, collider, solver-state, and step contracts. | A real VRM end-to-end slice proves a shared neutral descriptor. |
| `secondaryMotionVerlet` | plain static CMake library | `backends/secondaryMotionVerlet/` | First deterministic CPU implementation of `secondaryMotion`. | The consumer slice proves the minimal deterministic CPU behavior. |
| `physicsSchema` | OpenUSD schema plugin bundle | `plugins/physicsSchema/` | Narrow semantic extensions to standard USD physics. | A proposal passes [the schema admission rule](../design/DESIGN_POLICY.md#9-schema-admission-rule). |

Empty architectural placeholders are not created before their admission
condition. Libraries and bundles use lower-camel identities matching their
directory and exported package identity.

## 2. Dependency directions

### 2.1 Allowed edges

```text
physicsCore ------------------> no repository component; no OpenUSD; no SDK

physicsJolt ------------------> physicsCore, Jolt

physicsUsd -------------------> physicsCore, OpenUSD
                                 physicsSchema only if admitted

secondaryMotion -------------> no format repository; neutral shared math only
secondaryMotionVerlet --------> secondaryMotion

physicsSchema ----------------> OpenUSD only
```

Consumers point inward:

```text
usd-stage-runner -----> installed physicsCore and physicsJolt packages
usd-mmd-plugins ------> standard UsdPhysics authored data
usd-vrm-plugins ------> a generic adapter contract at composition time
usd-avatar-runtime ---> composes format adapters, runtimes, and this repository
```

`physicsCore` may use repository-owned math value types. If Phase 1 proves that
those types deserve their own identity, `WORKSPACE.md` changes before the new
target is created; `runtimeCore` is not reused because it would invert the
repository boundary.

### 2.2 Forbidden edges

| Edge | Why |
| --- | --- |
| `physicsCore -> OpenUSD` | the solver contract must work without a stage |
| `physicsCore -> Jolt` or another backend SDK | consumers must not compile against a backend to use neutral types |
| `physicsCore -> usd-stage-runner` | the extracted library cannot depend on its former host |
| any component -> `usd-mmd-plugins` or `usd-vrm-plugins` | source-format semantics stay in their owning repositories |
| `physicsJolt -> OpenUSD` | the backend consumes core descriptors, not scene objects |
| `physicsUsd -> Jolt` | the USD bridge and solver backend are independently replaceable |
| `physicsUsd -> Stage Runner runtime types` | the reader returns neutral snapshots; orchestration stays with consumers |
| `secondaryMotion -> VRM schemas` | VRM interpretation belongs in a VRM-aware adapter |
| a public header -> backend-native identifier or settings type | backend replacement must not rewrite consumers |
| a component -> a sibling source checkout | repositories are consumed as installed packages or OpenStrata members, never private source trees |

### 2.3 Enforcement

Each component declares edges in CMake and its OpenStrata manifest. Phase 0
adds boundary checks with each scaffold; Phase 1 proves `physicsCore` has no
OpenUSD, Stage Runner, MMD, VRM, or backend includes and no corresponding link
dependency. Phase 2 proves Jolt remains private to `physicsJolt`.

The checks must inspect source includes and resolved link interfaces. A prose
rule alone is not sufficient.

## 3. Planned repository layout

```text
usd-physics-plugins/
|- CMakeLists.txt
|- CMakePresets.json
|- VERSION
|- openstrata.toml
|- openstrata.ci.yaml
|- docs/
|  |- architecture/
|  |- contributing/
|  |- design/
|  |- reference/
|  `- roadmap/
|- libs/
|  |- physicsCore/
|  `- physicsUsd/                 # Phase 4
|- backends/
|  |- physicsJolt/
|  `- secondaryMotionVerlet/      # Phase 6, if admitted
|- plugins/
|  `- physicsSchema/              # only after schema admission
|- tests/
|  |- integration/
|  `- fixtures/
`- examples/                      # only when a maintained example exists
```

Unit tests live beside the component they test. Cross-component and installed
consumer tests live under root `tests/`.

## 4. Namespace and include policy

The public include root is `usd_physics/`. Public C++ names use the
`usd_physics` namespace with component namespaces such as
`usd_physics::core` and `usd_physics::jolt`. This resolves `ARCH-O1`; neither
surface retains `usd_stage_runner`.

Neutral vector, rotation, and transform values will live in `physicsCore` for
the Phase 1 extraction. A separate math package is not admitted without a
second repository-level consumer that needs it independently. This resolves
`ARCH-O2` and `RB-O2` for the initial extraction.

Public headers contain only types from their declared dependencies. Private
backend headers live below the backend target and are not installed as part of
the neutral package.

## 5. Build modes

The same component graph must work in two modes:

1. a root plain-CMake build for development and packaging;
2. OpenStrata workspace composition for ecosystem builds.

Installed-consumer tests configure against a clean prefix and never rely on a
source checkout, build-tree include, undeclared sibling, or workspace-only
target alias.

The root CMake file owns project settings, option defaults, component selection,
testing and the final dependency checks. Components own their targets, sources,
public/private dependencies, headers and installation. The small
`cmake/UsdPhysicsInstallPackage.cmake` helper shares only config/version/export
boilerplate; each component retains its package config and identity checks.
Source builds use the helper from this checkout, including when configured with
`cmake -S libs/physicsCore` (or another component). Installed consumers use only
their packages and declared SDKs.

`USDPHYSICS_BUILD_TESTS` defaults to ON only for a top-level root build;
component test defaults follow that option or their own top-level status.
`USDPHYSICS_BUILD_USD` and `PHYSICSJOLT_ENABLE_SIX_DOF` default to OFF. USD OFF
does not discover OpenUSD. Without Jolt, the backend provides typed
unavailability unless `PHYSICSJOLT_REQUIRE_JOLT` or six-DOF execution is requested.
Six-DOF requires the deterministic SDK definition. Component test overrides
remain available for standalone builds.

OpenStrata owns runtime SDK acquisition, pins, intents and the CI graph.
`CMakePresets.json` provides local Windows/MSVC and Linux/Ninja shortcuts, not
another dependency lock or CI matrix. Exact supported tools and platforms are
owned by [DEPENDENCIES.md](DEPENDENCIES.md).

CTest labels separate `unit`, `backend`, `usd`, `consumer`, `experimental` and
`workspace` checks. The existing `installed-consumer` label is retained as an
alias on the clean-prefix test. For focused local runs, use
`ctest --test-dir <build> -C Release -L consumer --output-on-failure`, or replace
`consumer` with another label. Labels overlap: opt-in six-DOF and spring tests
are both `backend` and `experimental`. Existing analytic spring checks keep
running in that gate; no fidelity gate is added.

PR CI keeps the graph cell and the Windows/Linux `usd-jolt` and
`usd-jolt-six-dof` cells. The graph cell additionally runs plain CMake without
Jolt or USD; the default intent cells test and package the three independent
libraries, while six-DOF cells test the experimental execution without
packaging. Full workspace tests remain unfiltered. Isolated library tests are
retained because they also exercise standalone builds. Changes to generated CI
must preserve the repository additions documented in the workflow header and
`openstrata.ci.yaml`.

## 6. Package boundaries

Each public library that exists installs its own CMake package. Package names,
targets, headers, transitive dependencies, and version compatibility are owned
by [PACKAGE_CONTRACT.md](PACKAGE_CONTRACT.md).

No umbrella package is created until a consumer demonstrates that it improves
composition without hiding optional dependencies.

## 7. Extraction rule

The Stage Runner sources are migration input, not a subtree to preserve.
History and behavior should be retained where practical, while includes,
namespaces, install paths, CMake package files, and manifests are rewritten to
this repository's ownership.

In particular, the current `physicsCore -> runtimeCore` dependency is removed:
math values move behind a neutral contract, and `PhysicsRuntime`'s
`PrimId <-> BodyHandle` mapping becomes a consumer responsibility
instead of dragging Stage Runner runtime identity into the core.

## 8. Open questions

- **ARCH-O3:** whether integration fixtures stay at root or under
  `libs/physicsUsd/tests/` until a second cross-component suite exists.
