# Phase 4 hosted six-DOF acceptance (2026-10-18)

Hosted evidence after the [SDK configuration cause](2026-10-18-phase4-six-dof-sdk-cause.md),
which selected `CROSS_PLATFORM_DETERMINISTIC=ON` for the experimental six-DOF
capability.

## Change under test

A `usd-jolt-six-dof` build intent adds `PHYSICSJOLT_ENABLE_SIX_DOF=ON` to the
`usd-jolt` intent. Two pull-request cells, `workspace-pr-windows-six-dof` and
`workspace-pr-linux-six-dof`, build the pinned Jolt 5.5.0 with the same
deterministic CI step as the other cells, then run `ost build` and `ost test`
with the new intent. They skip the library packaging step, so the
experimental capability never feeds compatible package inputs. The option's
default, the `usd-jolt` cells and their packages are unchanged.

## Results

[Run 39087259949](https://github.com/animu-sphere/usd-physics-plugins/actions/runs/39087259949)
on commit `e52db40`; all five cells passed.

| Cell | Runner | Compiler | Tests | Packaging |
| --- | --- | --- | --- | --- |
| `workspace-pr-windows-six-dof` | `windows-2022` | MSVC 19.44 | 18/18 | skipped |
| `workspace-pr-linux-six-dof` | `ubuntu-24.04` | GCC 13.3 | 18/18 | skipped |

Both cells configured with `PHYSICSJOLT_ENABLE_SIX_DOF:BOOL=ON` and the Jolt
backend, and ran `physicsCore.six_dof`, `physicsJolt.six_dof` (per-axis loads
including the Z-axis gate, rotated frames, offset centers of mass, world
locks, collision suppression and restoration, cleanup) and
`workspace.installed_consumer` with its six-DOF section. The default
`workspace-pr-windows` and `workspace-pr-linux` cells also passed unchanged.
Locally, the same intent passes 18/18 on Windows with MSVC 19.51.

## Scope

This closes the hosted acceptance gate for the selected SDK. It does not
enable the capability by default, publish six-DOF packages or change public
pins. It establishes the existing six-axis tests on hosted runners, not
coefficients, source collision filtering, springs or drives, or MMD
fidelity, which remain open.
