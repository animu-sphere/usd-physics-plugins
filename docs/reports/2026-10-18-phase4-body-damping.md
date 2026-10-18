# Phase 4 body damping rates (2026-10-18)

Local Windows follow-up to [contact coefficients](2026-10-18-phase4-contact-material.md).
The neutral `BodyDamping` descriptor carries independent finite non-negative
linear/angular decay rates in inverse seconds. `BodyDescriptor` appends it;
both defaults remain Jolt's previous `0.05`. Core, backend, bridge and consumers
must rebuild together because the body descriptor layout changes. Public
versions and pins are unchanged.

## Semantics and implementation

The rates describe `dv/dt=-linear*v` and `dw/dt=-angular*w`. They are not
dimensionless source retention fractions. Zero disables a channel; rates
above one are valid. Core validation applies independently and to every body
motion type. Jolt passes both rates into native creation settings and rejects
overflow or positive scalar underflow before creating a native body.

[Jolt 5.5.0](https://github.com/jrouwe/JoltPhysics/blob/v5.5.0/Jolt/Physics/Body/MotionProperties.inl#L114-L129)
uses `max(0, 1-rate*dt)` per substep after force/gravity integration. The
continuous exponential is approximated; timestep/force ordering remains a
backend fidelity boundary. No MMD name or source conversion enters this
repository, and `physicsUsd` gains no metadata interpretation.

## Results

Windows x86_64, MSVC 19.51, OpenUSD 26.08, deterministic Jolt 5.5.0.
`physicsJolt.damping` measures a translating body and a torque-free spinning
sphere over one second. Contact seeds the spin, then the floor is removed;
angular speed is measured from consecutive rotations. No public angular
velocity API is introduced. Rates `(0,0)`, `(2,0)`, `(0,2)` and `(2,2)`
verify independence at both step rates.

| Step rate | Linear retention, rate 2 | Angular retention, rate 2 |
| --- | ---: | ---: |
| 60 Hz | 0.130799 | 0.130798 |
| 240 Hz | 0.134206 | 0.134209 |

Continuous retention is `exp(-2) = 0.135335`. Disabled channels retain unity
within `0.000003`. Backend tests also reject negative/non-finite/overflow and
positive-underflow rates for every motion type and verify shape cleanup.

All 20 CTests pass in the USD/six-DOF build, including the clean-prefix
installed consumer. The no-Jolt build passes all 11 CTests, including its
clean-prefix consumer and typed unavailable path. Separately rebuilt installed
packages pass the MMD reader-only and execution probes: three CTests per
mode, twelve generic controls, 22 physics-bearing fixtures (three damped
bodies/two constraints, 120 finite steps each), and six empty fixtures.

## Reproduction

With the existing `build/phase4-usd` deterministic SDK/OpenUSD configuration:

```powershell
cmake --build build/phase4-usd --config Release
$env:PATH = "$env:USD_ROOT/bin;$env:USD_ROOT/lib;$env:PATH"
ctest --test-dir build/phase4-usd -C Release --output-on-failure
cmake --install build/phase4-usd --config Release --prefix build/phase8-damping-prefix
cmake --build build/phase2-no-jolt --config Release
ctest --test-dir build/phase2-no-jolt -C Release --output-on-failure
```

All commands completed locally. Hosted damping acceptance and compatible
artifact publication remain open. Source instantaneous stopping, source-solver
equivalence, material bindings, pair mixing, springs and full MMD acceptance
remain outside this increment.
