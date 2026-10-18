# Phase 4 experimental rotation springs (2026-10-18)

Local Windows follow-up to [translation springs](2026-10-18-phase4-translation-springs.md).

The neutral six-DOF descriptor adds three `RotationSpring` stiffness/damping
pairs in N*m/rad and N*m*s/rad, with identity equilibrium and axes in the second
joint frame. Core validation rejects invalid scalars, damping on a disabled
spring and active springs on locked angular axes. Backend overflow and nonzero
values lost to float conversion reject before resource creation.

The existing default-OFF six-DOF option gates execution. The backend configures
orientation position motors with SI stiffness/damping. This is a small-angle
approximation; no finite coupled Euler-spring law or source solver equivalence
is claimed. Jolt 5.5.0's [motor implementation](https://github.com/jrouwe/JoltPhysics/blob/v5.5.0/Jolt/Physics/Constraints/SixDOFConstraint.cpp#L421-L517)
uses second-frame axes and projected quaternion error. Source-format scaling
and activation stay in consumers; the generic USD reader/converter does not
interpret source springs or USD drives.

## Results

Windows x86_64, MSVC 19.51, OpenUSD 26.08, deterministic Jolt 5.5.0.
All 23 USD/six-DOF CTests pass, including an installed consumer with an initially
rotated dynamic body converging toward aligned frames. All 11 no-Jolt CTests pass.

The response oracle is the continuous damped torsional oscillator with sphere
inertia `I=2*m*r^2/5`, or reduced inertia for two moving endpoints. It compares
every sample over 0.5 seconds, using 0.1 rad initial displacement. Controls
cover X/Y/Z, identity/90-degree frames, world/static endpoints in both orders,
two dynamic endpoints, mass 1/4 kg, damping ratios 0/0.2, free/limited axes and
single/three-axis anisotropic settings. Inactive components stay below 1e-5;
the two-body case checks angular momentum balance.

| Step rate | Maximum angular error / initial displacement | Bound |
| --- | ---: | ---: |
| 60 Hz | 0.231179 | 0.24 |
| 240 Hz | 0.0654808 | 0.07 |

The tighter response at smaller steps measures numerical damping and small-angle
approximation. Separate tests load a hard angular stop, reject unsafe scalars,
and check cleanup after both rejection and execution.

The installed packages also execute the MMD consumer's 28 generated PMX inputs:
22 physics-bearing fixtures create three bodies/two constraints and step 120
times, each with three approximate rotation axes alongside the translation
springs; six physics-free fixtures remain empty. The MMD adapter owns its
`0.08^2` stiffness inference and preserves its source-fidelity gap counts.

## Reproduction

Use the deterministic SDK/OpenUSD configuration from the
[six-DOF report](2026-10-18-phase4-six-dof-execution.md#reproduction):

```powershell
cmake --build build/phase4-usd --config Release
$env:PATH = "$env:USD_ROOT/bin;$env:USD_ROOT/lib;$env:PATH"
ctest --test-dir build/phase4-usd -C Release --output-on-failure
cmake --install build/phase4-usd --config Release --prefix build/phase8-rotation-spring-prefix
cmake --build build/phase2-no-jolt --config Release
ctest --test-dir build/phase2-no-jolt -C Release --output-on-failure
```

Rebuild core/backend/bridge and every consumer together into a separate prefix;
the static descriptor layout changes. No published pin, version or component
dependency changes. Hosted spring acceptance, published compatible packages,
finite-angle/source response, standard drive bindings and runtime synchronization
remain open.
