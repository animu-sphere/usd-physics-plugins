# Read-only scene gravity (2026-10-19)

The consumer is Stage Runner's minimal standard-USD full-rebuild path. The
change adds optional plain scene settings to `readPhysicsScene()`; resource
creation, world configuration, handles, stepping and presentation stay with
the consumer. The [bridge contract](../design/USD_BRIDGE_CONTRACT.md#bounded-snapshot-contract)
owns selection, units and rejection semantics.

The generated `physicsUsd.scene_settings` fixture checks absent/default/custom
gravity, normalized world-space direction, ignored scene ancestor transforms,
zero and negative magnitude, extreme finite directions, snapshot independence
and no authored USD edits. Negative cases assert typed code/path for null
stages, ambiguous scenes, units, nonfinite/animated/blocked values, unsupported
physics declarations/APIs and instanced scenes. Body-only reader rejection is
retained. The clean-prefix USD consumer exercises the installed settings API
alongside bodies and a joint.

Verification uses Windows, Visual Studio 18 2026 / MSVC 19.51, Release and
OpenUSD 26.08. The fresh No-Jolt + USD configuration disables Jolt discovery,
proving that scene snapshots require no backend SDK. No hosted or Linux
scene-gravity result is claimed here. Earlier hosted package acceptance is
recorded separately in the [cleanup hosted report](2026-10-19-cmake-cleanup-hosted.md).

| Verification | Result |
| --- | --- |
| Fresh plain-CMake No-Jolt + USD configure/build | Passed with `CMAKE_DISABLE_FIND_PACKAGE_Jolt=ON` |
| Full root CTest suite | 16/16 passed, including all five USD tests |
| Clean-prefix installed consumer | Passed, including the gravity snapshot and existing package compatibility negative controls |
| Documentation checker self-test | 14 cases passed |
| Documentation links and CMake docs check | Passed |
| Whitespace/error check | `git diff --check` passed |

The first sandboxed MSBuild attempts stalled before compilation and were
interrupted. Building and running CTest outside that sandbox completed. The
Python documentation self-test used a repository build directory for temporary
files after the sandbox denied its default temporary-directory access. These
environment restrictions required no source or test workarounds.

Appending optional settings changes the bridge snapshot layout. Rebuild the
bridge and consumers together; the existing installed bridge header identity
checks remain in force. No core descriptor, package target, dependency pin or
backend dependency changes. Consumer rebuild/presentation and multi-scene
routing remain outside this slice.

Reproduction with an installed OpenUSD SDK:

```sh
cmake -S . -B build/scene-gravity-no-jolt-usd \
  -DUSDPHYSICS_BUILD_TESTS=ON -DUSDPHYSICS_BUILD_USD=ON \
  -DCMAKE_DISABLE_FIND_PACKAGE_Jolt=ON \
  -DCMAKE_PREFIX_PATH=<OpenUSD-prefix> -Dpxr_DIR=<OpenUSD-prefix>
cmake --build build/scene-gravity-no-jolt-usd --config Release
ctest --test-dir build/scene-gravity-no-jolt-usd -C Release --output-on-failure
```
