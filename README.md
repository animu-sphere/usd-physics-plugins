# OpenUSD Physics Plugins

[![CI](https://github.com/animu-sphere/usd-physics-plugins/actions/workflows/ost-source-ci.yml/badge.svg)](https://github.com/animu-sphere/usd-physics-plugins/actions/workflows/ost-source-ci.yml)
[![License: Apache-2.0](https://img.shields.io/github/license/animu-sphere/usd-physics-plugins?label=license)](LICENSE)

Backend-neutral physics runtime components for OpenUSD-based applications.

> OpenUSD describes the physical world; `usd-physics-plugins` makes that world
> executable without exposing a backend SDK to its consumers.

```text
standard UsdPhysics -> physicsUsd (read-only snapshots)
                                |
                                v
                           physicsCore <--- physicsJolt
                                |               |
                                +--- state <----+
                                     |
                       consumer scheduling and display
```

`physicsCore` owns neutral handles, descriptors, world and query contracts.
`physicsJolt` owns solver execution. The optional `physicsUsd` reads standard
USD declarations without owning a runtime world. The host owns rebuilding,
path/handle mappings and presentation; `usd-avatar-runtime` coordinates shared
world stepping across avatars. MMD/VRM repositories own source-format
interpretation and bone coupling. See the
[consumer boundaries](docs/design/DESIGN_POLICY.md#10-consumer-boundaries).

See the [capability matrix](docs/reference/CAPABILITY_MATRIX.md) for exact
implementation limits and evidence, and the
[current roadmap](docs/roadmap/current.md) for unfinished work.

## Build and use

A minimal plain-CMake build uses the repository's platform preset:

```powershell
cmake --preset windows-msvc
cmake --build --preset windows-release
ctest --preset windows-release
```

On Linux, use `linux-ninja` to configure and `linux-release` to build and test.
Make the selected SDK prefixes discoverable through `CMAKE_PREFIX_PATH`.
The optional USD adapter is selected with `-DUSDPHYSICS_BUILD_USD=ON`.
Dependency versions belong in [DEPENDENCIES.md](docs/architecture/DEPENDENCIES.md);
build options and OpenStrata intents belong in
[WORKSPACE.md](docs/architecture/WORKSPACE.md#5-build-modes).

Installed packages can be consumed from a clean prefix:

```cmake
find_package(physicsCore CONFIG REQUIRED)
find_package(physicsJolt CONFIG REQUIRED)
target_link_libraries(my_runtime PRIVATE physicsCore::physicsCore physicsJolt::physicsJolt)
```

See the [package contract](docs/architecture/PACKAGE_CONTRACT.md) for installation,
optional SDK dependencies and compatibility requirements.

## Documentation and community

Start with the [documentation index](docs/README.md) and
[design policy](docs/design/DESIGN_POLICY.md). Verification evidence lives in
[dated reports](docs/reports/README.md); release rules live in the
[release schema](docs/architecture/RELEASE_SCHEMA.md).

Contributions are welcome; read [CONTRIBUTING.md](CONTRIBUTING.md) and the
[Code of Conduct](CODE_OF_CONDUCT.md). Use the
[bug report](.github/ISSUE_TEMPLATE/bug_report.md) or
[feature request](.github/ISSUE_TEMPLATE/feature_request.md) template.
Report vulnerabilities privately following [SECURITY.md](SECURITY.md).

Licensed under the [Apache License 2.0](LICENSE). External dependencies retain
their own licenses.
