# Current roadmap — release follow-up and Phase 4

Status: in progress, 2026-09-27. The installed core/backend consumer migration
has passed hosted Windows/Linux verification. An optional standard Box reader
is implemented; the [capability matrix](../reference/CAPABILITY_MATRIX.md)
records the supported subset and tests.

The `usd-jolt` intent and source-CI cells now build the Box reader together
with Jolt and package all three libraries on both OSes. The
[local artifact report](../reports/2026-10-04-phase4-usd-artifacts.md) records
Windows validation; hosted results and public `physicsUsd` pins remain pending.

## 1. Release follow-up

Implement and dry-run the versioned release workflow against the
[release schema](../architecture/RELEASE_SCHEMA.md) before creating a tag.
Existing Windows/Linux core/backend artifacts remain migration inputs rather
than a full release.

## 2. Next Box delivery slice

- Validate and publish Windows/Linux `physicsUsd` artifacts and pin them in
  Stage Runner's OpenStrata requirements before enabling standard import by default.
- Run hosted standard/compatibility parity including standalone and usdview.
- Extend through working fixtures for scene gravity, additional shapes,
  mass inference, fixed joints, and composed transforms.
- Resolve stage-generation identity and synchronization contracts before
  introducing retained resource mappings in the package.

## 3. Completion criteria

Standard declarations become the primary authored representation in Stage
Runner while core/backend packages remain reusable and contain no gameplay
policy. The same package graph must work in plain CMake and OpenStrata, on
Windows and Linux. Keep Runner compatibility fixtures until all host and
runtime-layer parity gates pass. The wider intended bridge is defined in
[USD_BRIDGE_CONTRACT.md](../design/USD_BRIDGE_CONTRACT.md).
