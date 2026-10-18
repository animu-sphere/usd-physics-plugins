# Phase 4 six-DOF SDK configuration cause (2026-10-18)

Local evidence after [experimental six-DOF execution](2026-10-18-phase4-six-dof-execution.md),
which left the baseline SDK's failed Z-axis gate without an isolated cause.

## Finding

The failing setting is MSVC `/fp:fast` in Jolt's Release flags. Jolt 5.5.0's
`Build/CMakeLists.txt` adds `/fp:fast` for MSVC whenever
`CROSS_PLATFORM_DETERMINISTIC` is OFF and `/fp:precise` when it is ON. x86 SIMD
options and the exported `JPH_CROSS_PLATFORM_DETERMINISTIC` definition do not
change the outcome. Compiling only
`Jolt/Physics/Constraints/SixDOFConstraint.cpp` with `/fp:precise`, while every
other Jolt translation unit keeps `/fp:fast`, is enough to pass. The line-level
cause inside that translation unit is not isolated.

The earlier "scalar" baseline was built with the NMake Makefiles generator.
Jolt applies `/arch` only when `CMAKE_VS_PLATFORM_NAME` is `x86` or `x64`, which
that generator leaves empty, so that artifact had no optional x86 instruction
sets. That explains its exported definitions, but not its failure.

## Matrix

Windows x86_64, MSVC 19.51, Jolt 5.5.0 at the pinned commit, Release,
`INTERPROCEDURAL_OPTIMIZATION=OFF`, dynamic MSVC runtime. Each row rebuilds
`physicsJolt` with `PHYSICSJOLT_ENABLE_SIX_DOF=ON` against that SDK and runs
`physicsJolt.six_dof` five times.

| SDK | Jolt `/fp` | Deterministic | x86 SIMD | `six_dof` ×5 |
| --- | --- | --- | --- | --- |
| Previous baseline (NMake) | fast | OFF | none | fail ×5 |
| A | fast | OFF | AVX2 set | fail ×5 |
| D (Release flags overridden) | fast | ON | AVX2 set | fail ×5 |
| B | precise | ON | none | pass ×5 |
| C (Release flags overridden) | precise | OFF | AVX2 set | pass ×5 |
| Previous candidate | precise | ON | AVX2 set | pass ×5 |
| E (one translation unit) | fast, `SixDOFConstraint.cpp` precise | OFF | AVX2 set | pass ×5 |
| F (Release flags overridden) | precise | OFF | none | pass ×5 |

Every failure is the same check, `load reaches upper translation bound`. F
exports exactly the previous baseline's compile definitions and passes all 14
non-USD root tests, so `/fp:precise` alone repairs that ABI as well.

"AVX2 set" is Jolt's default x86 set: AVX2, AVX, SSE4.1, SSE4.2, LZCNT, TZCNT
and F16C, plus FMADD when deterministic mode is OFF. "Release flags
overridden" means `OVERRIDE_CXX_FLAGS=OFF` with
`CMAKE_CXX_FLAGS_RELEASE=/GS- /Gy /O2 /Oi /Ot /fp:<mode>`, which reproduces
Jolt's own Release flags with the other `/fp` mode. The generated Release
`FloatingPointModel` was checked in each project. E needed
`SKIP_PRECOMPILE_HEADERS` on that file, because the precompiled header was
built with `/fp:fast`.

## Linux

Ubuntu under WSL2 on the same host, GCC 15.2.0, CMake 4.2.3, Ninja, using the
source CI's Jolt configure options with `CROSS_PLATFORM_DETERMINISTIC` OFF and
ON. Jolt has no fast-math mode on GCC; both builds compile with
`-ffp-contract=off`. Both pass `physicsJolt.six_dof` five times and all 14
non-USD root tests. Linux evidence is local only.

| Deterministic | Installed `libJolt.a` SHA-256 |
| --- | --- |
| OFF | `2ac01a13cf0400a7e1691b324997dff04ed49b073798489dfef6ac450b01ec3e` |
| ON | `da03c56db242889a852f756a59ff6f3440343304d6dbf595259277a3f9d4e824` |

## Selected configuration

The six-DOF SDK acceptance gate selects `CROSS_PLATFORM_DETERMINISTIC=ON`.
It is Jolt's own switch for `/fp:precise`, needs no flag override, and gives
reproducible stepping for later trajectory comparisons. Its exported
definitions add `JPH_CROSS_PLATFORM_DETERMINISTIC` and drop `JPH_USE_FMADD`,
so artifacts built from it are a new ABI set. The installed `physicsJolt`
package already records and rechecks the exact set it was built with.
The source CI's pinned Jolt build now uses this setting on both OSes.
`PHYSICSJOLT_ENABLE_SIX_DOF` stays OFF by default, and hosted CI does not yet
build the experimental capability.

## Reproduction

With `JOLT_SOURCE` at the pinned source, build one SDK per row, for example F:

```powershell
cmake -S "$env:JOLT_SOURCE/Build" -B build/jolt-f -G 'Visual Studio 18 2026' -A x64 `
  -DCROSS_PLATFORM_DETERMINISTIC=OFF -DOVERRIDE_CXX_FLAGS=OFF `
  '-DCMAKE_CXX_FLAGS_RELEASE=/GS- /Gy /O2 /Oi /Ot /fp:precise' `
  -DUSE_SSE4_1=OFF -DUSE_SSE4_2=OFF -DUSE_AVX=OFF -DUSE_AVX2=OFF -DUSE_AVX512=OFF `
  -DUSE_LZCNT=OFF -DUSE_TZCNT=OFF -DUSE_F16C=OFF -DUSE_FMADD=OFF `
  -DINTERPROCEDURAL_OPTIMIZATION=OFF -DGENERATE_DEBUG_SYMBOLS=OFF `
  -DUSE_STATIC_MSVC_RUNTIME_LIBRARY=OFF -DTARGET_SAMPLES=OFF -DTARGET_UNIT_TESTS=OFF `
  -DTARGET_HELLO_WORLD=OFF -DTARGET_PERFORMANCE_TEST=OFF -DTARGET_VIEWER=OFF `
  "-DCMAKE_INSTALL_PREFIX=$PWD/build/jolt-f-install"
cmake --build build/jolt-f --config Release --target Jolt
cmake --install build/jolt-f --config Release
cmake -S . -B build/six-dof-f -G 'Visual Studio 18 2026' -A x64 `
  -DUSDPHYSICS_BUILD_TESTS=ON -DPHYSICSJOLT_REQUIRE_JOLT=ON `
  -DPHYSICSJOLT_ENABLE_SIX_DOF=ON "-DJolt_DIR=$PWD/build/jolt-f-install/lib/cmake/Jolt"
cmake --build build/six-dof-f --config Release
ctest --test-dir build/six-dof-f -C Release -R physicsJolt.six_dof --repeat until-fail:5
```

Installed `Jolt.lib` SHA-256 for F:
`bd0352a1830d6af7e7104b5095e86cb756aecb65ba0b593529f410ccc03f4750`.
No library source, public pin or default option changes. Next gates are hosted
six-DOF acceptance on the selected SDK, then coefficients, filtering, springs
and runtime synchronization.
