# Changelog

Notable changes to the released physics libraries. The format follows
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/). While the version is
0.x, a minor version may change the public API or ABI.

## [0.1.0] - 2026-10-31

The first versioned release: `physicsCore`, `physicsJolt` and `physicsUsd` for
`cy2026` / `usd` on Windows x86_64 and Linux x86_64. What each capability covers
and its limits are in the
[capability matrix](docs/reference/CAPABILITY_MATRIX.md).

### Added

- `physicsCore`: neutral body, collider, joint, query and world values and
  interfaces, with typed errors and installed package compatibility checks.
- `physicsCore`: body contact coefficients, per-world mixing policies, finite
  damping rates and independent prior-velocity clearing modes.
- `physicsJolt`: Jolt Physics 5.5.0 backend with box, sphere and capsule shapes,
  kinematic bodies, fixed and limited joints, and world gravity at creation.
- `physicsUsd`: read-only `UsdPhysics` scene snapshots of gravity, bodies, mass,
  colliders, composed transforms and joint frames and limits, rejecting
  unsupported input with typed diagnostics.
- A release workflow publishing each library by digest, with a generated consumer
  pin table.

### Known limitations

- `physicsJolt` packages do not contain Jolt. A consumer supplies the SDK whose
  compile definitions the package recorded, and any other build is refused.
- Six-DOF constraints and neutral springs are experimental, need a separate
  deterministic Jolt SDK and are off in the published `physicsJolt`.
- Windows packages are built with MSVC 19.44 and need that version or newer. On
  Linux a consumer needs the exact compiler version of `ubuntu-24.04`.
- macOS, mobile and WebAssembly are not targets.
