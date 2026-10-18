# Phase 4 experimental translation springs (2026-10-18)

Local Windows evidence after [contact mixing](2026-10-18-phase4-contact-mixing.md).

## Change under test

The neutral six-DOF descriptor appends three `TranslationSpring` values:
stiffness (N/m), viscous damping (N*s/m), equilibrium (meters), in the first
joint frame. Defaults disable springs. Finite non-negative coefficients and
finite equilibria are required. Zero stiffness with nonzero damping and active
springs on locked axes reject. Limited axes keep hard limits even when the
spring equilibrium lies outside the range.

Jolt 5.5.0 uses position motors with `StiffnessAndDamping` settings. Invalid
active scalar conversions reject before resource creation. Execution remains gated
by `PHYSICSJOLT_ENABLE_SIX_DOF=ON`. The generic USD reader and converter still
read no source springs or USD drives. Core/backend/bridge and consumers are
rebuilt together into a separate prefix because descriptor layout changes.
No source-format types, versions, pins or component dependencies change.

## Results

Windows x86_64, MSVC 19.51, OpenUSD 26.08, deterministic Jolt 5.5.0.
All 22 USD/six-DOF CTests pass, including the installed consumer's nonzero
spring target. The no-Jolt build passes all 11 CTests.

The response test cancels gravity and compares every sample over 0.5 seconds
with the independent continuous damped-oscillator solution. It covers X/Y/Z,
identity/90-degree joint frames, world/static endpoints, free/limited axes,
mass 1/4 kg, stiffness 25/100 N/m, damping ratio 0/0.2 and equilibrium -0.1 m.
All inactive axes stay within 1 mm. Separate controls check two dynamic
endpoints using reduced mass, center-of-mass conservation, hard-limit loading,
unsafe scalars and resource cleanup.

| Step rate | Maximum position error / initial displacement |
| --- | ---: |
| 60 Hz | 0.231214 |
| 240 Hz | 0.0655849 |

The test bounds are 0.24 and 0.07 respectively. The worst case is deliberately
retained: an implicit solver dissipates energy even with zero viscous damping.
This is bounded numerical approximation, not exact continuous dynamics or
Bullet/MMD trajectory evidence.

The installed packages also pass the isolated MMD consumer's seven execution
and five reader CTests. All 22 physics-bearing generated PMX fixtures step
120 times with two experimental translation axes each (44 total); six empty
fixtures still step without bodies. That consumer selects and reports its
own source approximation outside shared code.

## Reproduction

Use the deterministic SDK/OpenUSD configuration from the
[six-DOF report](2026-10-18-phase4-six-dof-execution.md#reproduction):

```powershell
cmake --build build/phase4-usd --config Release
$env:PATH = "$env:USD_ROOT/bin;$env:USD_ROOT/lib;$env:PATH"
ctest --test-dir build/phase4-usd -C Release --output-on-failure
cmake --install build/phase4-usd --config Release --prefix build/phase8-translation-spring-prefix
cmake --build build/phase2-no-jolt --config Release
ctest --test-dir build/phase2-no-jolt -C Release --output-on-failure
```

All commands completed locally. Hosted spring acceptance, compatible published
packages, angular springs, source solver trajectories, standard material/drive
bindings and runtime synchronization remain open.
