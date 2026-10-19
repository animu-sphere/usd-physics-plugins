# OpenUSD Physics Plugins

Backend-neutral physics runtime components for OpenUSD-based applications.

See the [capability matrix](docs/reference/CAPABILITY_MATRIX.md) for implemented
features and verification evidence, and the [current roadmap](docs/roadmap/current.md)
for remaining work.

The repository is intended to extract the reusable physics boundary proven in
[`usd-stage-runner`](https://github.com/animu-sphere/usd-stage-runner), then
validate it with
[`usd-mmd-plugins`](https://github.com/animu-sphere/usd-mmd-plugins) and
[`usd-vrm-plugins`](https://github.com/animu-sphere/usd-vrm-plugins).

## The central rule

The optional USD adapter builds with `-DUSDPHYSICS_BUILD_USD=ON` and an OpenUSD
26.08 install on `CMAKE_PREFIX_PATH`. It installs `physicsUsd::physicsUsd` and
`usd_physics/usd/box_scene.h`. The core/backend-only root build remains the
default. See the [bounded Box contract](docs/design/USD_BRIDGE_CONTRACT.md)
for supported declarations and explicit rejection behavior.

`ost build --intent usd-jolt` and `ost test --intent usd-jolt` validate the
reader and Jolt together, including the clean-prefix USD consumer. See the
[build and test responsibilities](docs/architecture/WORKSPACE.md#5-build-modes)
for CMake, OpenStrata and CI ownership.

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

## Components

| Component | Kind | Role |
| --- | --- | --- |
| `physicsCore` | plain C++ library | Handles, descriptors, world lifecycle, state, and optional query contracts; no OpenUSD and no backend SDK |
| `physicsJolt` | plain C++ library | Rigid-body backend implementing `physicsCore` |
| `physicsUsd` | OpenUSD-facing C++ library | Translate standard `UsdPhysics` declarations to neutral snapshots |

Component identities, directories, and allowed dependency edges are owned by
the [workspace contract](docs/architecture/WORKSPACE.md), which also defines
admission conditions for future components. Implementation status belongs in
the [capability matrix](docs/reference/CAPABILITY_MATRIX.md).

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

Package usage is documented in the
[installed package contract](docs/architecture/PACKAGE_CONTRACT.md).
Dated verification evidence lives in [docs/reports/](docs/reports/).
