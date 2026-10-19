# Secondary-motion contract

> **Decision: proposed, revised 2026-10-20.** This contract reserves a domain
> boundary. A concrete VRM end-to-end slice must prove the shared descriptor and
> deterministic CPU behavior before component admission or public-header design. Component status
> belongs in the [capability matrix](../reference/CAPABILITY_MATRIX.md); work
> ordering belongs in the [current roadmap](../roadmap/current.md).

## 1. Purpose

`secondaryMotion` provides format-neutral dynamics for spring-like skeletal
chains interacting with colliders. It serves hair, skirts, tails,
accessories, and similar procedural motion without pretending each joint is a
general rigid body.

## 2. Separation from rigid bodies

Rigid bodies and secondary motion may share transform math, timing conventions,
collision geometry descriptions, and debug-output conventions. They do not
share one solver interface.

```text
physicsCore                         secondaryMotion
  bodies, mass, inertia              chains, rest pose, stiffness
  constraints, forces                drag, gravity, joint radius
  rigid-body solver                  chain solver
```

A secondary-motion solver may query rigid-world colliders through a narrow
capability adapter. It does not require every chain joint to own a
`BodyHandle`.

Jolt rigid bodies must not become the sole implementation strategy for VRM
SpringBone. Reuse portable evaluation inputs/outputs, collider descriptors and
structured diagnostics before attempting to unify solver algorithms.

## 3. Format boundary

VRM concepts remain in `usd-vrm-plugins`:

- `VrmSpringBoneAPI` and `VrmColliderAPI`;
- collider groups;
- center and anchor interpretation;
- VRM stiffness, gravity, drag, and hit-radius semantics.

A VRM-aware adapter converts them to generic descriptors:

```text
VRM schemas -> VRM adapter -> SpringChainDescriptor -> secondaryMotion solver
```

No generic header contains `Vrm`, PMX, or avatar-specific terminology.

## 4. Descriptor direction

The minimum candidate model contains:

- a chain with ordered joints;
- rest transforms and segment lengths;
- stiffness and damping/drag;
- gravity vector or acceleration;
- per-joint collision radius;
- collider references;
- explicit root/anchor input;
- solver settings whose semantics are portable.

Exact names and units are not frozen until Phase 6 tests exist.

## 5. Runtime inputs and outputs

The caller supplies an explicit step and current animation/root transforms.
The solver returns local or world-space joint corrections under an explicitly
documented convention. It does not mutate a USD stage or skeleton object.

Reset, teleport, discontinuous animation, scale change, and collider-set
change must have deterministic state-reset semantics.

## 6. Solver implementations

The first implementation is a deterministic CPU solver, likely Verlet or a
closely related position-based method. The algorithm name is not part of the
core contract, and a future XPBD or custom implementation remains possible.

Backend-specific iteration controls are exposed only if they acquire portable
meaning or stay in an implementation-specific construction API.

## 7. Collision boundary

Secondary motion needs generic sphere/capsule-style colliders first. Reuse
with rigid-body queries must not introduce a dependency from `physicsCore` to
`secondaryMotion` or force backend-native collision shapes into descriptors.

The ownership and sharing model for collider geometry is `SM-O1`.

## 8. Scheduling boundary

Applications own ordering such as:

```text
animation -> control evaluation -> secondary motion -> final pose -> rendering
```

This repository defines the step operation and state, not the application
frame graph or avatar runtime.

`usd-avatar-runtime` supplies the common scheduler and defines where secondary
motion runs relative to rigid-body evaluation. The admitted solver contract
must specify its pre-evaluation pose/collider inputs and post-evaluation
corrections. Sharing a scheduler does not give a chain solver permission to
step a borrowed rigid-body world. Rigid-world barriers follow the
[shared-world contract](RIGID_BODY_CONTRACT.md#73-shared-world-scheduling).
Presentation interpolation uses copied results and must not modify solver state.

## 9. Validation

Begin with one consumer-owned VRM end-to-end slice: format interpretation,
collider groups, bone chains and pose updates stay in usd-vrm-plugins or
usd-avatar-runtime. Extract only the demonstrated neutral descriptor and CPU
solver behavior. Use minimal fixtures for contract, reset, collision and stable
execution; add numerical cases only for an identified boundary or regression.
No generic target depends on VRM headers.

## 10. Open questions

- **SM-O1:** shared collider identity and ownership across rigid-body and
  secondary-motion systems.
- **SM-O2:** output convention—absolute transforms, local rotations, or
  explicit pose deltas.
- **SM-O3:** fixed-step and substep guarantees needed for stable playback.
- **SM-O4:** portable parameter normalization across VRM semantics and the
  selected first solver.
