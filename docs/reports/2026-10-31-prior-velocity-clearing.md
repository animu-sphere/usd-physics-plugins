# Prior-velocity clearing (2026-10-31)

## Scope and contract

The MMD owner's typed damping resolver distinguishes finite decay from clearing
prior velocity. The latter needs a neutral backend response before the runtime
can select it. This slice implements that response without importing source
coefficients, normalization rules or MMD dependencies into generic physics.

`BodyDamping` retains its existing two rate fields and finite-rate defaults.
Appended independent `linearMode` and `angularMode` fields admit
`BodyDampingMode::clearPriorVelocity` with a zero rate for the selected channel.
Unknown modes and a clearing mode paired with a nonzero rate reject for all
motion types. The [rigid-body contract](../design/RIGID_BODY_CONTRACT.md#5-descriptors)
owns the semantics; the [package contract](../architecture/PACKAGE_CONTRACT.md#8-static-package-compatibility)
owns discovery metadata and closure rebuild requirements.

Jolt retains the descriptor on each body and clears selected dynamic velocities
after step validation, immediately before its one-substep `Update`. Native
body setters preserve queued forces and do not wake sleeping bodies. Gravity,
forces and constraint/contact impulses then generate new motion normally.
Static bodies and explicit kinematic commands are unaffected. No large finite
rate, position lock or post-force velocity override represents this response.

## Automated evidence

`physicsCore.descriptors` covers defaults, independent mode combinations and
invalid declarations across static/dynamic/kinematic bodies.
`physicsJolt.prior_velocity` covers the following at 60, 120 and 240 Hz:

- Analytical velocity and position with zero/directed gravity, with and without
  queued forces, and with force omitted on the last step. Prior speed is removed
  on every step while new acceleration survives; tolerance is `2e-5`.
- Invalid durations, including values below Jolt's positive scalar range,
  reject before velocity changes.
- Contact-seeded spin survives linear clearing; angular clearing removes spin
  after the contact source is destroyed. Fresh contact torque still rotates a
  body while angular clearing is selected.
- All four independent mode combinations, including finite rate 2 in the other
  channel, preserve the selected finite linear/angular response.
- A translated/rotated kinematic anchor drives a fixed dynamic body through
  fresh linear/angular constraint impulses. After removing the joint, prior
  constraint velocity clears on the next step.
- Recreating a body and rebuilding a world reproduce free-body transforms and
  linear velocities exactly for the same ordered inputs in this build.
  Post-step changed-state publication and destruction cleanup are checked.
- Static bodies remain fixed; clearing declarations do not alter commanded
  kinematic translation/rotation.

The existing finite-rate damping, gravity, contact, six-DOF and spring tests
also pass. Clean-prefix consumers compile the appended descriptor fields,
check feature metadata and execute clearing with new force/gravity response.
Existing rejection controls still reject incompatible/missing core identity,
modified headers, incompatible compiler identity and incompatible Jolt targets.
No-Jolt consumers validate descriptors and retain typed backend unavailability.

## Environment and reproduction

Verified from base commit `715764545d840408401ba52aa4fb7289a5728c54` plus this
working-tree change on Windows, MSVC 19.51.36256, CMake 4.4, Release,
OpenUSD 26.08 / Python 3.13.14 and installed deterministic Jolt 5.5.0.

| Verification | Result |
| --- | --- |
| Jolt + USD + experimental six-DOF root CTest | 26/26 passed |
| No-Jolt, no-USD root CTest | 11/11 passed |
| Clean-prefix consumers and compatibility negative controls | Passed in both configurations |
| Documentation checker self-test | 14 cases passed |
| Documentation links and CMake docs check | Passed |

Run in Visual Studio Developer PowerShell with installed dependencies:

```powershell
# Avoid the local MSVC program-database C1902 failure during compiler probes.
# This changes debug-information storage only, including nested consumers.
$env:_CL_ = '/Z7'
cmake -S . -B build/prior-velocity-jolt -G Ninja `
  -DCMAKE_BUILD_TYPE=Release -DUSDPHYSICS_BUILD_TESTS=ON `
  -DUSDPHYSICS_BUILD_USD=ON -DPHYSICSJOLT_REQUIRE_JOLT=ON `
  -DPHYSICSJOLT_ENABLE_SIX_DOF=ON `
  '-DJolt_DIR=<Jolt-prefix>/lib/cmake/Jolt' `
  '-Dpxr_DIR=<OpenUSD-prefix>' '-DCMAKE_PREFIX_PATH=<OpenUSD-prefix>'
cmake --build build/prior-velocity-jolt --parallel 8
ctest --test-dir build/prior-velocity-jolt -C Release --output-on-failure

cmake -S . -B build/prior-velocity-no-jolt-embedded -G Ninja `
  -DCMAKE_BUILD_TYPE=Release -DUSDPHYSICS_BUILD_TESTS=ON `
  -DCMAKE_DISABLE_FIND_PACKAGE_Jolt=ON
cmake --build build/prior-velocity-no-jolt-embedded --parallel 8
ctest --test-dir build/prior-velocity-no-jolt-embedded -C Release --output-on-failure
```

The original compiler probe failed with `C1902` before package compatibility
admission. Embedded debug information allowed fresh builds and all existing
negative controls to execute without changing or bypassing their assertions.
Documentation self-tests used a build-local temporary directory because the
sandbox's default temporary directory refused nested writes.

## Limits

This is local Windows evidence, not hosted/Linux acceptance or a published
package. Rebuild the complete core/backend/bridge/consumer closure after the
descriptor change; old matching product versions do not establish ABI safety.
No MMD runtime policy is enabled here. Source normalization diagnostics, live
host Reset admission and private PMX/VMD intake without the temporary damping
layer remain consumer work. Exact source trajectories and representative avatar
L3 acceptance are not established by these generated backend tests.
