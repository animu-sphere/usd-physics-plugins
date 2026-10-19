# CMake package and test organization (2026-10-19)

## Baseline and audit

The starting tree was `90b57a0`, which merges
[PR #26](https://github.com/animu-sphere/usd-physics-plugins/pull/26).
Its [hosted run 39136917114](https://github.com/animu-sphere/usd-physics-plugins/actions/runs/39136917114)
passed all five cells. The package contract revision, header/toolchain identities,
Jolt definition checks and deterministic six-DOF guard were present before this
cleanup and are retained.

| Area | Finding | Change |
| --- | --- | --- |
| Root CMake | Already small; two workspace options and final target-boundary checks | Group options and put boundary checks last; no extra options module |
| Components | Three copies of config/version/export installation | One 25-line helper in `cmake/`; targets, header installation, dependency discovery and identity calculation stay in components |
| Installed packages | Three independent static packages, SameMinorVersion compatibility | Same package names, imported targets, include roots, configs and dependencies |
| SDK discovery | Optional Jolt fallback; OpenUSD only inside the optional USD component | Retained; no downloads, SDK pins or new build options |
| Presets and OpenStrata | Local presets plus separate intents and pinned CI SDKs | No preset, intent or dependency-pin changes |
| Tests | Root and standalone library builds exercise overlapping component tests | Add labels; retain all tests because standalone builds validate a different composition |
| README | Dated progress text and a component-state table duplicate the reference docs | Link to Capability Matrix and Roadmap; keep component roles |

## Package and configuration verification

Windows used CMake 4.4.3, Visual Studio 18 2026 / MSVC 19.51, Release,
OpenUSD 26.08 and the existing deterministic Jolt 5.5.0 installation.
Linux used WSL Ubuntu, CMake 4.2.3, GCC 15.2 and Ninja Multi-Config.
Linux's existing Jolt installation was used with six-DOF OFF.

| Verification | Result |
| --- | --- |
| Windows root, forced no-Jolt, USD OFF | 11/11 before and after |
| Windows root, Jolt + USD + six-DOF | 23/23 before and after |
| Linux root, forced no-Jolt, USD OFF | 11/11 after |
| Linux root, Jolt required, USD/six-DOF OFF | 16/16 after |
| Windows OpenStrata `usd-jolt` | Build passed; 20/20 tests after a clean rebuild |
| Standalone `physicsCore` | Build/install and 6/6 tests |
| Standalone `physicsJolt`, six-DOF OFF | Build/install against installed core and 6/6 tests |
| Standalone `physicsUsd` | Build/install against installed core and 4/4 tests |
| Consumer of the three separately installed packages | Configure/link and 2/2 runtime tests |
| Root embedded through `add_subdirectory()` | Workspace/component tests, USD and six-DOF default OFF; no test targets or OpenUSD discovery |
| Six-DOF requested without Jolt | Configure rejected |
| Six-DOF requested with the existing non-deterministic Jolt SDK | Configure rejected |
| Required Jolt unavailable | Configure rejected |
| USD ON with OpenUSD discovery disabled | Configure rejected |
| OpenStrata manifest and workspace graph | `ost ci validate` and graph-only check passed |

The paired Windows root runs installed into fresh consumer prefixes. Comparing
the before/after file contents found no differences in installed headers,
configs, exports, version files or libraries in either configuration. Public
targets remain `physicsCore::physicsCore`, `physicsJolt::physicsJolt` and
`physicsUsd::physicsUsd`. The helper is a source-build implementation detail;
installed packages do not load it or require this checkout.

Every root suite includes the existing clean-prefix installed consumer. Its
negative controls still reject mismatched/missing core identities, compiler
identity and edited installed core headers. SDK-enabled runs also reject
incompatible Jolt definitions; USD-enabled runs check bridge/core mismatch
without importing the backend. Identity remains conservative, with the limits
documented in [PACKAGE_CONTRACT.md](../architecture/PACKAGE_CONTRACT.md#8-static-package-compatibility).

The first incremental OpenStrata run failed `physicsCore.world` with
`0xc0000409`; a focused rerun reproduced it. Cleaning that existing build's
generated outputs and rebuilding through `ost build --intent usd-jolt` resolved
the failure, after which the full intent passed. No runtime or test source was
changed. This is not evidence of a diagnosed compiler/runtime defect.

## Test roles and measurements

CTest's `experimental` label overlaps `backend`; `installed-consumer` remains
an alias for `consumer`. Counts below describe the existing Windows six-DOF
suite, with labels added after the baseline measurement.

| Role | Tests | SDK | Before (s) | After (s) |
| --- | --- | --- | --- | --- |
| `unit` | 6 | None | 0.05 | 0.06 |
| `backend` | 9 | Jolt; 3 tests also `experimental` | 4.84 | 7.08 |
| `usd` | 4 | OpenUSD | 0.35 | 0.38 |
| `workspace` | 3 | None | 0.08 | 0.09 |
| `consumer` | 1 | SDKs selected by the build | 14.38 | 16.09 |

Total CTest wall time was 19.71 s before and 23.71 s after for that suite;
the no-Jolt suite was 7.76 s before and 8.32 s after. These are single samples,
with other validation builds running concurrently, not a performance benchmark
or a speedup claim. Linux no-Jolt and Jolt suites took 13.29 s and 19.65 s on
the mounted Windows filesystem. Standalone component test times were 0.05 s
(core), 1.05 s (backend) and 0.18 s (USD).

No test was deleted, filtered out of hosted acceptance, or moved to a nightly
job. Existing 60/240 Hz analytic spring checks remain in the opt-in experimental
suite; Bullet/MMD fidelity is not added as a merge condition.

## CI comparison

Baseline job durations below come from the linked hosted run's job timestamps.
Workspace-test and library-step timings are separate from SDK setup/build time.

| Cell | Job (s) | Workspace tests (s) | Isolated library build/test/consumer/package step (s) |
| --- | --- | --- | --- |
| `workspace-graph-pr` | 7 | Not run | Not run |
| `workspace-pr-windows` | 183 | 15 | 40 |
| `workspace-pr-linux` | 175 | 9 | 25 |
| `workspace-pr-windows-six-dof` | 173 | 26 | Skipped |
| `workspace-pr-linux-six-dof` | 153 | 10 | Skipped |

The matrix remains **5 cells before / 5 after**. The graph cell gains a
plain-CMake no-SDK configure/build/full test run and uploads its JUnit and CTest
logs. The same configuration passed locally on Linux. It adds the missing
fallback coverage without another matrix combination or another SDK build.
The existing Windows/Linux SDK and six-DOF cells, packaging conditions and
unfiltered workspace tests are unchanged. Standalone library tests retain their
composition checks; no measured work is removed merely to lower CI time.

Hosted execution of this cleanup, including the added graph-cell steps, remains
pending. The after-change hosted duration therefore cannot yet be compared to
the baseline. Local Linux USD/six-DOF was not rerun; the retained hosted cells
must establish that parity for this revision.

## Reproduction

For the no-SDK CI path (no SDK prefix required):

```sh
cmake -S . -B build/ci-no-jolt -G "Ninja Multi-Config" \
  -DUSDPHYSICS_BUILD_TESTS=ON -DUSDPHYSICS_BUILD_USD=OFF \
  -DCMAKE_DISABLE_FIND_PACKAGE_Jolt=ON
cmake --build build/ci-no-jolt --config Release --parallel 2
ctest --test-dir build/ci-no-jolt -C Release --output-on-failure --output-junit ctest.xml
```

For SDK-enabled builds, provide the installed Jolt/OpenUSD prefixes and use the
existing OpenStrata `usd-jolt` or `usd-jolt-six-dof` intent. For isolated source
builds, configure each component directory and install core before configuring
backend or bridge against that prefix. See
[build-mode ownership](../architecture/WORKSPACE.md#5-build-modes) for labels and
the separation between local presets, CMake packages and OpenStrata.
