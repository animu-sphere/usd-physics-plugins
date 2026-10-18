# Phase 4 body contact coefficients (2026-10-18)

Local Windows evidence following the six-DOF execution work. The neutral core
now accepts `ContactMaterial` on `BodyDescriptor`, with non-negative finite
friction and restitution in `[0,1]`. Defaults preserve Jolt's previous `0.2` / `0`
behavior. Every motion type is validated. The backend passes these values to
Jolt body creation and rejects positive scalar underflow and unsafe friction
pair products before creating a native body.

## Results

Windows x86_64, MSVC 19.51, OpenUSD 26.08, deterministic Jolt 5.5.0.
The existing `build/phase4-usd` configuration has USD and experimental six-DOF
enabled. All 19 CTests pass, including the clean-prefix installed consumer,
core validation, six-axis execution, dependency boundaries and documentation.
The no-Jolt configuration also rebuilds and passes all 11 CTests, including
its clean-prefix consumer and typed backend-unavailable path.

`physicsJolt.material` observes contact behavior without a joint. A box starts
sliding at 5 m/s on a horizontal floor; after 0.5 s its X velocity is:

| Floor friction | Box friction | Observed speed (m/s) |
| ---: | ---: | ---: |
| 0 | 1 | 4.87655 |
| 1 | 0 | 4.87655 |
| 1 | 1 | 0.0333689 |
| 0.25 | 1 | 2.45468 |

A sphere falls from a center height of 3 m onto the same floor. Maximum upward
speed is zero for restitution `(0,0)`, and 5.52308 m/s for `(0,0.8)`, `(0.8,0)`
and `(0.25,0.8)`. Controls step at 1/240 s and distinguish either body's
coefficient and the backend's geometric-mean / maximum mixing rules.
Invalid input and scalar-range controls also confirm rejected creations do
not retain a shape. Core tests include friction greater than one and endpoints
of the restitution interval.

## Reproduction

With the existing deterministic SDK and OpenUSD configuration:

```powershell
cmake --build build/phase4-usd --config Release
$env:PATH = "$env:USD_ROOT/bin;$env:USD_ROOT/lib;$env:PATH"
ctest --test-dir build/phase4-usd -C Release --output-on-failure
cmake --install build/phase4-usd --config Release --prefix build/phase8-contact-prefix
```

All commands above completed locally. The new prefix was consumed separately
by the MMD reader-only and execution probes, including all 22 physics-bearing
fixtures. Core, backend, bridge and consumers were rebuilt together because
`BodyDescriptor` changes binary layout. Public versions and pins are unchanged.

## Remaining boundary

Pair mixing remains backend policy; copying PMX coefficients is not a claim
of MMD/Bullet trajectory equivalence. No MMD name enters this repository.
The generic reader does not yet read standard USD physics material bindings.
Damping, source-solver mixing equivalence, springs, hosted contact acceptance
and compatible artifact publication remain open.
