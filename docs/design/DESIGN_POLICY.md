# usd-physics-plugins design policy

> **Decision: accepted, revised 2026-10-20.** This policy defines intended
> behavior and responsibility boundaries. Implementation claims belong only in
> the [capability matrix](../reference/CAPABILITY_MATRIX.md); ordered unfinished
> work belongs only in the [current roadmap](../roadmap/current.md).
>
> Focused contracts own their areas and win if wording differs:
>
> | Area | Owning document |
> | --- | --- |
> | Rigid-body runtime API | [RIGID_BODY_CONTRACT.md](RIGID_BODY_CONTRACT.md) |
> | OpenUSD interpretation and synchronization | [USD_BRIDGE_CONTRACT.md](USD_BRIDGE_CONTRACT.md) |
> | Secondary skeletal motion | [SECONDARY_MOTION_CONTRACT.md](SECONDARY_MOTION_CONTRACT.md) |
> | Component identities and dependency edges | [architecture/WORKSPACE.md](../architecture/WORKSPACE.md) |

## 1. Purpose

`usd-physics-plugins` provides reusable simulation mechanics for OpenUSD-based
applications. It connects standard USD physics declarations and
solver-neutral runtime descriptions to replaceable solver implementations.
Jolt is the first backend. A reproducible fixed-step world must be callable
from a common avatar scheduler for MMD rigid bodies and future VRM secondary
motion, without importing either format's semantics into generic physics.

The repository sits below application orchestration and source-format
semantics, and above backend SDKs:

```text
OpenUSD and format adapters
           |
           v
solver-neutral contracts
           |
           v
usd-physics-plugins
           |
           v
Jolt first; other solvers remain possible
```

It begins by extracting the `physicsCore` and `physicsJolt` responsibilities
already exercised by `usd-stage-runner`. MMD and VRM are validation consumers,
not definitions of the generic API.

## 2. Central rule

> OpenUSD describes the physical world; `usd-physics-plugins` makes that world
> executable without leaking backend-specific or application-specific types
> into consumers.

This implies three hard boundaries:

1. Standard `UsdPhysics` is the preferred persistent representation.
2. Public runtime contracts use stable handles and plain value types.
3. Scheduling, gameplay, source-format interpretation, and rendering stay
   outside this repository.

## 3. Design principles

### 3.1 Standard schemas first

Use `UsdPhysicsScene`, `UsdPhysicsRigidBodyAPI`,
`UsdPhysicsCollisionAPI`, `UsdPhysicsMassAPI`, standard joint schemas,
`UsdPhysicsDriveAPI`, and `UsdPhysicsLimitAPI` where their semantics match.

A custom schema is not justified by convenience, a source-format field, or a
backend option. The admission rule is in §9.

### 3.2 Backend-neutral public APIs

Public reusable headers must not expose Jolt, PhysX, Bullet, Stage Runner,
MMD, or VRM types. Backend-native identifiers are transient implementation
details.

The initial vocabulary is deliberately small: handles, descriptors, body
state, a world lifecycle, and optional query capability interfaces.

### 3.3 Extract before redesigning

The first vertical slice preserves behavior proven by `usd-stage-runner`:

- 64-bit opaque shape, body, and constraint handles;
- box shapes, static and dynamic bodies, fixed constraints;
- force and velocity operations;
- fixed-step simulation and changed-body extraction;
- segment and ground query capabilities;
- a Jolt implementation.

Extraction must remove the dependency on Stage Runner's `runtimeCore`, but it
must not use that removal as an excuse to invent a universal engine API.
Compatibility adapters may temporarily exist in Stage Runner while consumers
migrate.

### 3.4 Real consumers validate abstractions

A capability becomes public only when an implemented vertical slice needs it.
Prefer validation by more than one consumer before freezing a broad contract.
Validate the minimal standard-USD execution path and package consumption
before expanding rigid bodies, constraints or springs. Secondary motion and
advanced queries enter only when a concrete consumer proves the need.

### 3.5 Capability-oriented extensibility

Backends need not pretend to implement features they do not support. Optional
features such as segment casts, shape casts, or ground queries are separate
capability interfaces that a consumer can discover explicitly.

### 3.6 Deterministic and rebuildable runtime state

Mappings from scene identities to runtime resources are transient,
deterministic, and safe to rebuild after stage reload, prim deletion, or
backend reset. Persistent USD data never stores a backend pointer or native
body ID.

## 4. Repository responsibilities

This repository owns:

- backend-neutral shape, body, constraint, stepping, and state contracts;
- validation of solver-neutral descriptors;
- optional collision and support-query contracts;
- Jolt initialization and implementation behind the neutral boundary;
- interpretation of standard `UsdPhysics` into runtime descriptors;
- generic secondary-motion contracts once a consumer validates them.

It may later own additional solver implementations, debugging data contracts,
or tools whose responsibilities are generic to physics, after consumer-led
admission rather than speculative API design.

## 5. Responsibilities kept outside

The repository does not own:

- PMX, VMD, VRM, or other source-format parsing;
- MMD rigid-body modes or VRM schema interpretation;
- stage-runner scheduling, input, character state, camera, or gameplay policy;
- animation-system ordering or application-specific transform writeback;
- rendering, authoring UI, or editor integration;
- a single high-level four-wheel vehicle abstraction;
- backend tuning expressed as persistent authored semantics.

Format repositories translate their semantics to standard USD or to a narrow
generic descriptor. Applications decide when systems run and where resulting
transforms are consumed.

## 6. Rigid-body domain

`physicsCore` is independent of OpenUSD and backend SDKs. Its initial contract
is extracted from Stage Runner and then evolved only through tested consumer
requirements.

The full intended contract is in
[RIGID_BODY_CONTRACT.md](RIGID_BODY_CONTRACT.md). Its most important invariant
is dependency direction:

```text
physicsJolt ----> physicsCore <---- physicsUsd
     |                                  |
     v                                  v
   Jolt SDK                           OpenUSD
```

`physicsCore` does not depend on either side.

## 7. OpenUSD domain

`physicsUsd` reads conventional USD physics representation and produces neutral
snapshots containing `physicsCore` values and descriptors. It owns read-only interpretation and
transform conversion. The consumer owns runtime construction, path/handle
mappings, rebuild and presentation; source-format semantics stay in adapters.

The full intended contract is in
[USD_BRIDGE_CONTRACT.md](USD_BRIDGE_CONTRACT.md).

The bridge starts with scene gravity, box/sphere/capsule collision, static and
dynamic bodies, mass, and fixed joints. Additional geometry and joints are
added from tested vertical slices.

## 8. Secondary-motion domain

Rigid bodies and secondary skeletal motion are related but distinct domains.
Hair, skirts, tails, accessories, and VRM spring bones should not be forced
into a rigid-body world abstraction merely because they use colliders.

A secondary-motion component is admitted only after a minimal VRM end-to-end
slice demonstrates which neutral descriptors and deterministic CPU behavior
should be shared. Do not freeze an abstract API before that slice. A
format-specific adapter translates VRM semantics into the neutral contract
outside the core library. The full boundary is in
[SECONDARY_MOTION_CONTRACT.md](SECONDARY_MOTION_CONTRACT.md).

## 9. Schema admission rule

`physicsSchema` is created only when a proposed persistent concept satisfies
all of these conditions:

1. no standard OpenUSD schema represents the semantics adequately;
2. the concept must survive as authored scene data, rather than runtime state;
3. at least two plausible consumers benefit, or one real consumer proves a
   stable general-purpose need;
4. the property describes physical meaning, not backend configuration;
5. a generic consumer can ignore it without corrupting standard USD meaning.

`JoltBodyID`, broad-phase layer indices, native shape subtypes, allocator
settings, job-system details, and backend pointer identity fail this test.

## 10. Consumer boundaries

### 10.1 Stage Runner

Stage Runner owns fixed-timestep scheduling, stage lifecycle, input, character
and camera policy, animation/physics ordering, and application writeback. This
repository owns physical resources, stepping, queries, and solver state.
Stage Runner owns the temporary prim-path/handle table and explicit full
rebuild on reload. It applies runtime state to display transforms without
requiring automatic edits of authored USD. Construction failures must destroy
partial resources before exposing a world. Standard import begins opt-in;
compatibility fixtures remain until host and runtime parity pass.

The rule is: Stage Runner decides **when** physics runs; this repository
defines **how** generic simulation runs.

### 10.2 MMD

`usd-mmd-plugins` owns PMX parsing, MMD collision groups, body modes, bone
association, and joint semantics. It authors standard `UsdPhysics` wherever
possible and preserves unmatched MMD semantics in its own contract.

The generic path is:

```text
PMX -> MMD canonical model -> UsdPhysics -> physicsUsd -> physicsCore -> Jolt
```

MMD-specific defaults, filtering, damping conversion and bone coupling remain
in its adapter. Acceptance requires stable execution, clear failure and usable
bone integration; exact Bullet trajectories are a separate fidelity study.

No PMX header, MMD mode enum, or `mmd:*` attribute is interpreted by
`physicsCore`, `physicsJolt`, or the standard `physicsUsd` reader. In particular,
`mmd:physics:*` is not part of the generic reader contract. MMD's 16-group
conversion stays in `usd-mmd-plugins`; allocation of shared-world category bits
across avatars is host policy.

### 10.3 VRM

`usd-vrm-plugins` owns `VrmSpringBoneAPI`, `VrmColliderAPI`, collider groups,
and VRM parameter meaning. Bone chains, pose updates and format coupling stay
in the VRM adapter / avatar runtime. A VRM-aware adapter produces generic
secondary-motion descriptors. The generic solver does not include VRM schema
headers or terminology.

### 10.4 Character and camera systems

Physics may expose ground contacts, support-body identity, velocities, ray or
segment casts, shape casts, ignored-body filters, and collision masks. It does
not define walk speed, jump input, locomotion states, camera damping, orbit
behavior, or framing.

### 10.5 Vehicles

Vehicle work begins with reusable primitives: a rigid chassis, forces,
torques, angular velocity, wheel contact or shape casts, contact normals,
surface friction, and suspension force. A higher-level vehicle contract is
admitted only after real consumers demonstrate it. Jolt's vehicle types do not
become public API.

### 10.6 Avatar runtime and shared worlds

`usd-avatar-runtime` owns the common scheduler: all avatars submit their
pre-physics kinematic targets, each shared world steps once per fixed tick,
then all avatars read body state and apply bone feedback. The application/host
injects world ownership and lifetime; no generic world owns an avatar's clock.
Format adapters own bone associations and feedback semantics. Render snapshots
and interpolation must not modify canonical solver state. The detailed runtime
contract is in [RIGID_BODY_CONTRACT.md §7.3](RIGID_BODY_CONTRACT.md#73-shared-world-scheduling).

## 11. Transform policy

Initial implementation may support a documented subset, but the design does
not freeze Stage Runner's current transform restrictions into the generic API.
The bridge must grow toward composed USD transforms: translation, orientation,
scale, parent transforms, `resetXformStack`, collider-local transforms, and
world/local conversion.

Unsupported or lossy cases fail explicitly or produce a stable diagnostic.
They never silently produce a different physical scene.

## 12. Testing policy

Tests follow the dependency layers:

- **core tests:** handle validity, descriptor validation, lifecycle, stepping
  preconditions, state and query contracts, with no OpenUSD or Jolt;
- **backend tests:** falling body, static floor, collision, constraints,
  queries, changed-body extraction, and deterministic cleanup against Jolt;
- **USD tests:** small synthetic USDA fixtures covering
  `USD -> descriptors -> solver -> state`;
- **consumer scenarios:** MMD bodies and joints, VRM secondary motion, Stage
  Runner grounding, and later vehicle primitives.

Core tests do not use format-specific assets. Any external asset included in
an integration suite must have redistribution terms recorded; generated
fixtures are preferred.

PR CI emphasizes builds and small fixed-step, unit and smoke checks. Heavy
trajectory and fidelity benchmarks are manual evidence, recorded in dated
reports; do not add nightly jobs. Preserve the existing representative analytic
and experimental gates under §18 and the
[workspace CI policy](../architecture/WORKSPACE.md#5-build-modes).

## 13. Build and distribution policy

The workspace is intended to support plain CMake, installed CMake packages,
and OpenStrata composition. It must not require a Stage Runner checkout.

Dependency versions and SDK configuration belong in
[architecture/DEPENDENCIES.md](../architecture/DEPENDENCIES.md); tested support
and evidence belong only in the capability matrix.

Python bindings are deferred until a real consumer requires them. The physics
kernel remains C++-first.

## 14. Phases

These historical phase definitions describe scope, not current progress,
release numbers or execution order. The current roadmap owns the avatar
integration sequence and package acceptance gates.

| Phase | Goal | Acceptance |
| --- | --- | --- |
| 0 — documentation and workspace skeleton | Adopt contracts, root build, manifests, CI, dependency pins, and empty-but-buildable initial targets. | Both plain CMake and OpenStrata validate the workspace; docs distinguish planned from implemented behavior. |
| 1 — extract `physicsCore` | Move proven handles, descriptors, world lifecycle, state, validation, and query interfaces out of Stage Runner; replace `runtimeCore` value types with repository-owned neutral types. | Core builds and tests with no Stage Runner, OpenUSD, or backend SDK dependency. |
| 2 — extract `physicsJolt` | Move the Jolt implementation without changing observable Phase 1 behavior. | Falling-body, cleanup, changed-state, segment-query, and ground-query tests pass through `physicsCore`. |
| 3 — Stage Runner migration | Consume installed `physicsCore` and `physicsJolt`; keep orchestration in Stage Runner. | Existing falling-body, grounding, jump-support, and camera-collision scenarios pass without Stage Runner owning Jolt. |
| 4 — `physicsUsd` foundation | Translate standard scene gravity, primitive collision, static/dynamic bodies, mass, and fixed joints; validate consumer-owned mappings and reset/rebuild. | Synthetic USDA tests cover import, simulation, deletion/rebuild, and state synchronization. |
| 5 — MMD validation | Add the joint, limit, spring, damping, friction, restitution, and collision-filter capabilities required by a PMX vertical slice. | A generated PMX-derived USD fixture runs through the standard bridge with no MMD dependency in this repository. |
| 6 — secondary-motion core | Define and implement the smallest generic spring-chain and collider contract needed by VRM. | Deterministic solver tests and a format-neutral descriptor fixture pass. |
| 7 — VRM validation | Adapt VRM spring-bone semantics outside the core and run them through the generic secondary-motion API. | A generated VRM-derived scenario runs without VRM types in the generic libraries. |
| 8 — vehicle primitives | Add only the reusable query and force primitives demonstrated by a Stage Runner vehicle scenario. | The scenario uses no backend-native public type and does not require a universal vehicle abstraction. |

Ordered unfinished work is tracked only in
[roadmap/current.md](../roadmap/current.md).

## 15. Early non-goals

The first releases do not attempt cloth, fluids, FEM, full soft bodies, GPU
physics, networking or rollback, robotics simulation, a complete game-engine
API, every feature of any backend, every `UsdPhysics` schema, or a universal
vehicle system.

## 16. Success criteria

The repository succeeds when:

- Stage Runner consumes it without directly owning Jolt;
- MMD rigid bodies and joints use the same runtime through standard USD;
- VRM reuses generic collider and secondary-motion infrastructure without
  leaking VRM types into the core;
- persistent scene data is OpenUSD-first and backend identifiers stay
  transient;
- public contracts are theoretically implementable by another backend;
- runtime mappings are deterministic, destroyable, and rebuildable;
- tests cover neutral contracts, the real backend, USD translation, and real
  consumer scenarios.

## 17. Open design discipline

Do not resolve an unknown by widening the abstraction. Record the consumer,
the scenario, the missing capability, and the smallest contract change that
serves it. If a second backend could not implement a public operation without
pretending to be Jolt, the operation belongs behind a capability or inside the
Jolt adapter.

## 18. Acceptance and expansion policy

Contract tests (invalid values, units, handles, errors and dependency boundaries)
and behavior smoke tests (falling/rest, contact, basic joints and clean-prefix
consumers) are merge gates. Numerical fidelity studies are normally optional;
retain existing representative analytic tests and experimental opt-in six-DOF
coverage without adding every parameter combination to every PR.

Add a test only to protect a public contract, prevent a real regression, prove
consumer acceptance or cover an untested boundary. Preserve existing tests
unless a justified consolidation keeps their guarantees. Keep unit, backend,
USD, consumer and workspace responsibilities distinct. Record unstable SDK
conditions and measurements in new dated evidence.

A shared API proposal must identify a real consumer, solver-neutral meaning,
units, failure semantics, minimal execution test and build/distribution impact.
A second real use case should drive broader abstraction. Promote an expansion
into immediate work only if the minimal end-to-end acceptance needs it.

Incremental USD synchronization, bidirectional authoring, comprehensive compound
colliders, additional solvers, character/vehicle systems, exact Bullet fidelity
and finite-angle spring equivalence are deferred expansion areas. Reconsider
them when a consumer demonstrates a concrete requirement. Do not add a custom
schema for a concept standard UsdPhysics can already represent.
