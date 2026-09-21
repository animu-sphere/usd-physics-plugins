# Installed package contract

> **Status: accepted, revised 2026-09-22.** `physicsCore` and `physicsJolt`
> install and pass a clean-prefix consumer test. `physicsCore` now includes
> neutral values, typed handles, and validated descriptors; world behavior
> remains unimplemented.

## 1. Purpose

Each reusable component is consumable from a clean install prefix through
plain CMake and through an equivalent OpenStrata library identity. Consumers do
not need this repository's source tree.

## 2. Planned packages

| Package | Imported target | Provides | Planned phase |
| --- | --- | --- | --- |
| `physicsCore` | `physicsCore::physicsCore` | Version surface plus neutral vector, quaternion, transform, typed handles, validated box/body/fixed-constraint descriptors, and semantic collision filters; world and query contracts follow in Phase 1 | values, handles, and descriptors present; runtime Phase 1 |
| `physicsJolt` | `physicsJolt::physicsJolt` | Phase 0 explicit unavailable status; Jolt-backed world construction follows in Phase 2 | scaffold present; backend Phase 2 |
| `physicsUsd` | `physicsUsd::physicsUsd` | USD translation, mappings, and synchronization records | 4 |
| `secondaryMotion` | `secondaryMotion::secondaryMotion` | generic secondary-motion contracts | 6, if admitted |
| `secondaryMotionVerlet` | `secondaryMotionVerlet::secondaryMotionVerlet` | first CPU solver | 6, if admitted |
| `physicsSchema` | bundle contract to be defined at admission | generated schema library and resources | only if admitted |

The public include root and namespace are fixed by
[WORKSPACE.md §4](WORKSPACE.md#4-namespace-and-include-policy).

## 3. Install-interface rules

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

These examples are illustrative until installed-consumer tests exist and must
not be copied into a build guide as verified commands before then.

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
