# Phase 4 contact pair mixing (2026-10-18)

Local Windows evidence after [body damping](2026-10-18-phase4-body-damping.md).
This increment selects contact coefficient rules, without establishing source
solver trajectories or introducing source-format knowledge into shared code.

## Change under test

`physicsCore` adds `ContactMixing`: independent geometric-mean, maximum or
product rules for friction/restitution, plus an optional non-negative finite
friction ceiling. The default is geometric-mean friction, maximum restitution
and no ceiling. The existing no-argument factory retains that behavior.
The new `physicsJolt::createWorld(const ContactMixing&)` validates and copies
the policy. A private immutable listener applies it to added and persistent
contacts. No global callback or mutable policy is used; differently configured
worlds can coexist. The no-Jolt overload validates then throws the existing
typed backend-unavailable error.

[Jolt 5.5.0](https://github.com/jrouwe/JoltPhysics/blob/v5.5.0/Jolt/Physics/PhysicsSystem.h#L62-L69)
documents the existing defaults;
[contact settings](https://github.com/jrouwe/JoltPhysics/blob/v5.5.0/Jolt/Physics/Collision/ContactListener.h)
allow per-contact replacement. No backend type enters public headers. The
generic USD reader gains no metadata or material interpretation. Descriptor
layout, normal dependency graph, versions and public artifact pins are unchanged.
All packages and consumers use a separate rebuilt local prefix.

The existing unsafe-friction rejection also checks the square of the rounded
float in double precision. In particular `2^64`, which a float-rounded
`sqrt(FLT_MAX)` threshold previously admitted, now rejects before body creation.
Ceilings outside Jolt's scalar range or rounded from positive to zero reject.

## Results

Windows x86_64, MSVC 19.51, OpenUSD 26.08, deterministic Jolt 5.5.0.
All 21 USD/six-DOF CTests pass, including the clean-prefix installed consumer.
The no-Jolt build passes all 11 CTests, including its clean-prefix consumer.
Core controls cover every rule combination, absent/zero/positive ceilings,
unknown rules and negative/non-finite ceilings. Backend controls cover the
three rules, swapped and equal pairs, either zero restitution, independent
world settings, scalar rejection and resource cleanup.

Response tests disable both damping channels. A box starts sliding at 5 m/s;
the measurement follows 0.5 seconds of contact. A sphere drops from a center
height of 3 m; the measurement is maximum upward speed.

| Step rate | Product friction `(0.25,1)` speed | Default friction speed | Product restitution `(0.25,0.8)` bounce | Default restitution bounce |
| --- | ---: | ---: | ---: | ---: |
| 60 Hz | 3.77375 | 2.54751 | 1.40610 | 5.62440 |
| 240 Hz | 3.77378 | 2.54748 | 1.39792 | 5.59170 |

A ceiling of `0.25` with friction `(4,4)` matches product `(0.25,1)`.
A ceiling of `10` with `(4,4)` matches `(5,2)` from a 100 m/s start:
36.9609 / 36.9241 m/s at 60/240 Hz. This compares equivalent contact
coefficients instead of assuming high-friction box motion stays purely
translational. Product restitution is 0.2 of the unit-pair bounce, and a zero
on either endpoint absorbs. Defaults are measured while product/ceiling
worlds remain alive, proving isolation from their settings.

## Reproduction

With the existing deterministic SDK/OpenUSD configuration:

```powershell
cmake --build build/phase4-usd --config Release
$env:PATH = "$env:USD_ROOT/bin;$env:USD_ROOT/lib;$env:PATH"
ctest --test-dir build/phase4-usd -C Release --output-on-failure
cmake --install build/phase4-usd --config Release --prefix build/phase8-mixing-prefix
cmake --build build/phase2-no-jolt --config Release
ctest --test-dir build/phase2-no-jolt -C Release --output-on-failure
```

All commands completed locally. The installed prefix also passes both MMD
probe modes: three CTests each, twelve reader controls, 22 physics-bearing
fixtures with three bodies/two constraints and 120 finite execution steps,
and six empty fixtures. Hosted mixing acceptance, compatible publication,
standard material bindings, source trajectory fidelity, springs and runtime
synchronization remain open. A world-wide policy does not resolve how
different source policies should interact inside a mixed-format runtime.
