# World gravity at construction (2026-10-19)

The consumer need is Stage Runner's minimal standard-USD full-rebuild path in
the [current roadmap](../roadmap/current.md). The read-only bridge already
provides scene acceleration, but the backend factory previously could not
accept it. This change adds `createWorld(mixing, gravity)` with neutral values
and no new mandatory world method, descriptor layout or backend SDK type.
The [rigid-body contract](../design/RIGID_BODY_CONTRACT.md#61-world-gravity-at-construction)
owns units, defaults and rejection semantics.

`physicsJolt.gravity` compares velocity and displacement with constant
acceleration for directed, zero and default gravity at 60/240 Hz. Differently
configured worlds coexist, and both existing factory overloads retain their
default. Damping is disabled for these measurements. Velocity tolerance is
`1e-4` m/s; displacement tolerance is `0.05` m over half a second, allowing
semi-implicit integration error. Static and uncommanded kinematic bodies stay
fixed. Resource destruction clears changed-state entries. Each component
rejects NaN, infinity, double-to-float overflow and nonzero values rounded to
zero; a subsequent valid world still runs. The contact-mixing test uses the
new overload for its product-policy world and retains the existing contact
response checks.

The clean-prefix consumer links the new backend symbol and checks horizontal
acceleration. Its USD executable creates generated Box fixtures and passes
the optional snapshot gravity into a newly constructed world. Absent scenes
select the factory default, directed scenes produce the expected three-axis
velocity, and zero magnitude disables gravity. Root-layer serialization stays
unchanged after snapshot reading and stepping. The same executable retains
reader-only checks when the backend is unavailable. The scaffold checks that
nonfinite gravity rejects even without Jolt; valid settings produce typed
backend unavailability in that configuration.

Verification uses Windows, Visual Studio 18 2026 / MSVC 19.51, Release,
OpenUSD 26.08 and deterministic Jolt 5.5.0. Both configurations are fresh
plain-CMake builds, with USD enabled and experimental six-DOF disabled.

| Verification | Result |
| --- | --- |
| Jolt + USD configure/build and full root CTest | 22/22 passed |
| No-Jolt + USD configure/build and full root CTest | 16/16 passed with `CMAKE_DISABLE_FIND_PACKAGE_Jolt=ON` |
| Clean-prefix installed consumers | Passed in both configurations, including existing compatibility negative controls |
| Documentation checker self-test | 14 cases passed |
| Documentation links and CMake docs check | Passed |
| Whitespace/error check | `git diff --check` passed |

MSBuild and CTest ran outside the sandbox because the existing
[scene-gravity report](2026-10-19-scene-gravity.md) records sandboxed MSBuild
stalls. This run adds no hosted, Linux or OpenStrata evidence. It publishes
no package or release pin. Existing factory signatures remain available;
consumers of the new overload must link a rebuilt backend library. All
verification installations use build-local prefixes.

Stage Runner was inspected to identify the missing factory input but was not
modified. Consumer factory wiring, joint execution, reload cleanup and display
presentation remain unfinished in the roadmap. This prerequisite does not
establish consumer or host parity for the wider scene path.

Reproduction with installed SDKs:

```sh
cmake -S . -B build/world-gravity-usd \
  -DUSDPHYSICS_BUILD_TESTS=ON -DUSDPHYSICS_BUILD_USD=ON \
  -DPHYSICSJOLT_REQUIRE_JOLT=ON -DJolt_DIR=<Jolt-prefix>/lib/cmake/Jolt \
  -Dpxr_DIR=<OpenUSD-prefix> -DCMAKE_PREFIX_PATH=<OpenUSD-prefix>
cmake --build build/world-gravity-usd --config Release
ctest --test-dir build/world-gravity-usd -C Release --output-on-failure
```

For the No-Jolt check, select a separate build directory, omit
`PHYSICSJOLT_REQUIRE_JOLT` and `Jolt_DIR`, and set
`CMAKE_DISABLE_FIND_PACKAGE_Jolt=ON`.
