# OpenUSD Physics Plugins

Backend-neutral physics runtime components for OpenUSD-based applications.

> **Status: Phase 1 core extraction.** The repository builds and installs
> `physicsCore` and `physicsJolt`; neutral rigid-transform values and typed
> handles are implemented, but descriptors, rigid-body runtime behavior, an
> OpenUSD bridge, and a solver backend are not. The
> [capability matrix](docs/reference/CAPABILITY_MATRIX.md) is the only page
> that states what exists, and the [current roadmap](docs/roadmap/current.md)
> states what comes next.

The repository is intended to extract the reusable physics boundary proven in
[`usd-stage-runner`](https://github.com/animu-sphere/usd-stage-runner), then
validate it with
[`usd-mmd-plugins`](https://github.com/animu-sphere/usd-mmd-plugins) and
[`usd-vrm-plugins`](https://github.com/animu-sphere/usd-vrm-plugins).

## The central rule

> **OpenUSD describes the physical world; `usd-physics-plugins` makes that
> world executable without exposing a backend SDK to its consumers.**

```text
source-format semantics
        |
        v
standard UsdPhysics or solver-neutral descriptors
        |
        v
usd-physics-plugins
        |
        v
replaceable solver backend
```

The initial rigid-body path is:

```text
UsdPhysics stage -> physicsUsd -> physicsCore -> physicsJolt -> body states
```

Secondary skeletal motion is related but distinct:

```text
format-owned semantics -> generic spring-chain descriptors
                       -> secondaryMotion -> solver implementation
```

MMD, VRM, gameplay, camera, character, and vehicle policy remain in their
owning repositories. Backend-native identifiers and types remain private to
their backend.

## Planned components

| Component | Kind | Role | State |
| --- | --- | --- | --- |
| `physicsCore` | plain C++ library | Handles, descriptors, world lifecycle, state, and optional query contracts; no OpenUSD and no backend SDK | neutral values and handles present; remaining runtime API planned |
| `physicsJolt` | plain C++ library | First rigid-body backend implementing `physicsCore` | package scaffold present; backend unavailable |
| `physicsUsd` | OpenUSD-facing C++ library | Translate standard `UsdPhysics` declarations to runtime descriptors and maintain transient prim/resource mappings | planned |
| `secondaryMotion` | plain C++ library | Solver-neutral spring-chain and collider contracts | deferred until the rigid-body boundary is extracted |
| `secondaryMotionVerlet` | plain C++ library | First deterministic CPU secondary-motion solver | deferred until a real VRM integration slice requires it |
| `physicsSchema` | OpenUSD schema bundle | Semantic extensions that pass the schema admission test | not admitted |

Component identities, directories, and allowed dependency edges are owned by
the [workspace contract](docs/architecture/WORKSPACE.md). Planned names do not
mean empty targets should be created ahead of their phase.

## Scope boundaries

This repository owns reusable simulation mechanics:

- shapes, bodies, constraints, forces, velocities, and changed-body state;
- optional collision, ground, ray, segment, and shape-cast capabilities;
- standard `UsdPhysics` interpretation and transient runtime mappings;
- backend adapters, beginning with Jolt;
- generic secondary-motion contracts when validated by a real consumer.

It does not own source-format parsing, MMD or VRM semantics, application update
loops, player input, character policy, camera behavior, rendering, or editor
UI. See the [design policy](docs/design/DESIGN_POLICY.md) for the full boundary.

## Documentation

| | |
| --- | --- |
| [docs/design/](docs/design/) | Intended behavior and rationale; start with [DESIGN_POLICY.md](docs/design/DESIGN_POLICY.md) |
| [docs/architecture/](docs/architecture/) | Binding component identities, dependency directions, external dependencies, and package surfaces |
| [docs/reference/](docs/reference/) | Facts about the current tree; start with [CAPABILITY_MATRIX.md](docs/reference/CAPABILITY_MATRIX.md) |
| [docs/roadmap/](docs/roadmap/) | Incomplete work and phase status |
| [docs/contributing/](docs/contributing/) | How these documents are maintained |

Build and usage guides will be added only when commands can be run against an
implementation. Release records and dated reports will likewise be created
only when a release or real evidence exists.
