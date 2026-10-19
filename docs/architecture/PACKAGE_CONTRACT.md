# Installed package contract

> **Decision: accepted.** This document owns installation, public package
> surfaces and compatibility requirements. Implementation and delivery status
> belong only in the [capability matrix](../reference/CAPABILITY_MATRIX.md).

## 1. Purpose

Each reusable component is consumable from a clean install prefix through
plain CMake and through an equivalent OpenStrata library identity. Consumers do
not need this repository's source tree.

## 2. Planned packages

Component admission is governed by [WORKSPACE.md](WORKSPACE.md#1-identities),
not by this table's presence.

| Package | Imported target | Contract |
| --- | --- | --- |
| `physicsCore` | `physicsCore::physicsCore` | Neutral values, handles, descriptors, validation, world/state and optional query/capability headers. |
| `physicsJolt` | `physicsJolt::physicsJolt` | Backend availability metadata and factories; private compatible SDK dependency or typed unavailable fallback. |
| `physicsUsd` | `physicsUsd::physicsUsd` | Read-only primitive and joint snapshots plus stateless neutral conversion; no retained world or mapping. |
| `secondaryMotion` | `secondaryMotion::secondaryMotion` | Generic secondary-motion contract, only if admitted. |
| `secondaryMotionVerlet` | `secondaryMotionVerlet::secondaryMotionVerlet` | Deterministic CPU solver, only if admitted. |
| `physicsSchema` | bundle contract defined at admission | Generated schema library/resources, only if admitted. |

The public include root and namespace are fixed by
[WORKSPACE.md §4](WORKSPACE.md#4-namespace-and-include-policy).

## 3. Install-interface rules

The optional `physicsUsd` target is selected by `USDPHYSICS_BUILD_USD=ON`
in the root or built as standalone `libs/physicsUsd`. It exports headers under
`usd_physics/usd/`, depends on `physicsCore` and OpenUSD `usd`, `usdGeom` and
`usdPhysics` targets (accepting `pxr::` names), and resolves dependencies from
its installed config. The USD package must work without a Jolt SDK. Reading
and rejection semantics belong in the
[bridge contract](../design/USD_BRIDGE_CONTRACT.md).

Public core headers live under `usd_physics/core/`; the optional kinematic and
six-DOF capabilities do not expand the mandatory world interface. USD snapshot
APIs live in `box_scene.h`; stateless joint conversion lives in
`joint_conversion.h`. `physicsJolt_BACKEND_AVAILABLE` and
`physicsJolt_SIX_DOF_AVAILABLE` metadata must agree with runtime capability
discovery. Six-DOF execution requires explicit opt-in and the selected SDK
configuration; see [DEPENDENCIES.md](DEPENDENCIES.md#3-jolt-physics).

Descriptor layout changes require rebuilding core, backend, bridge and every
consumer together into a separate prefix. Defaults can preserve source
initializers without preserving static binary compatibility. Do not mix older
published artifacts with rebuilt descriptor headers. Adding API or changing
layout does not establish a newly published artifact; only a verified release
pin table does. Source-format coefficient conversion and policy selection stay
in consumers, including contact-mixing factory settings.

Every library package must:

- install public headers under one repository-owned include root;
- export a namespaced CMake target;
- publish accurate transitive dependencies through its target interface;
- install a config and compatible-version file;
- avoid absolute source/build paths in exported metadata;
- work from a clean prefix in an installed-consumer test;
- match the dependency edges in `openstrata.library.yaml`.

Backend SDKs are private unless their symbols are unavoidably part of the
backend package link interface. They never become dependencies of
`physicsCore`.

## 4. Consumer examples

The intended neutral consumer shape is:

```cmake
find_package(physicsCore CONFIG REQUIRED)
target_link_libraries(my_runtime PRIVATE physicsCore::physicsCore)
```

A consumer selecting Jolt adds it explicitly:

```cmake
find_package(physicsJolt CONFIG REQUIRED)
target_link_libraries(my_runtime PRIVATE physicsJolt::physicsJolt)
```

Installed-consumer acceptance must verify both package imports. A Jolt-enabled
consumer must also make the selected Jolt installation discoverable to CMake;
the installed `physicsJolt` config resolves that declared dependency.

## 5. Versioning

Packages begin pre-1.0. Minor versions may evolve source APIs, but changes are
documented and kept coherent across packages released together. Breaking
descriptor semantics or handle/lifecycle guarantees require an explicit
version change and migration note.

Compatibility mode for Stage Runner's old include paths or namespaces, if
needed, belongs in Stage Runner or a time-limited adapter package. It does not
become the permanent public surface here.

## 6. Bundle and runtime discovery

An admitted OpenUSD plugin installs its resources so `PlugRegistry` can
discover them from the installed bundle without a source-tree path. The exact
bundle contract is added only when `physicsSchema` passes admission.

`physicsCore` and solver libraries are ordinary CMake packages; loading them
must not require OpenUSD plugin discovery.

## 7. Acceptance

A package changes from `not present` to `supported` only when a clean consumer:

1. configures using only the install prefix and declared external dependencies;
2. includes its public headers;
3. links its imported target;
4. runs a minimal behavior test;
5. proves no source-tree or build-tree path leaked into the export.

## 8. Static package compatibility

Rebuilt packages expose `<package>_PACKAGE_CONTRACT_VERSION=1`. The installed
core config checks a conservative identity of its public headers (normalized
line endings), compiler ID/version, system/processor, pointer size and MSVC
runtime selection. It exports `USDPHYSICS_ABI_ID` on its CMake target. Backend
and USD bridge configs compare the core identity they were built against;
missing or different metadata rejects with a rebuild diagnostic. Source
version compatibility alone does not establish descriptor compatibility.

The bridge additionally checks its installed public headers and the selected
OpenUSD version/header set against its build inputs. Header edits trigger
CMake regeneration. These checks support root and standalone package builds
and use no source/build paths in the installation.

Six-DOF builds require the SDK's exported
`JPH_CROSS_PLATFORM_DETERMINISTIC` definition. The existing installed backend
check still requires the exact Jolt version and matching interface definitions.
Execution remains opt-in and no new published artifact pin is implied.

The identity is deliberately conservative: a comment edit may require rebuilding
dependent packages. It is not a binary checksum or a full compiler-flag,
standard-library, SDK-binary or custom-macro ABI audit. Keep each package's
headers, config and static library together, use the validated SDK, and rebuild
the complete closure after descriptor changes. Installed-consumer tests cover
matched packages and rejection of mismatched/missing core identity, compiler
identity and installed core headers, including the bridge without a backend.
