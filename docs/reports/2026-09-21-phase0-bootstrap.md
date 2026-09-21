# 2026-09-21 Phase 0 bootstrap verification

## Environment

- Host: Windows with Ubuntu WSL execution for the Linux build
- CMake/CTest: 4.2.3
- Generator: Ninja Multi-Config
- Compiler: GCC 15.2.0
- OpenStrata CLI: 0.23.2

## Executed evidence

The root configured with the `linux-ninja` preset, built in Release, and ran
five CTest cases:

- `physicsCore.scaffold`;
- `physicsJolt.scaffold`;
- `workspace.boundaries`;
- `workspace.docs`; and
- `workspace.installed_consumer`.

The installed-consumer test installed both libraries into a clean prefix,
configured a separate CMake project using only that prefix, built it, and ran
it. Both components were also configured, built, and installed independently;
`physicsJolt` resolved `physicsCore` from the standalone install prefix.

`ost ci validate` accepted all three cells in `openstrata.ci.yaml`, and
`ost ci plan` resolved one graph cell plus hosted Windows and Linux workspace
cells. OpenStrata library build dry-runs resolved `physicsJolt -> physicsCore`
and produced standalone configure/build/install plans. Regenerating
`.github/workflows/ost-source-ci.yml` from the matrix produced the same SHA-256
digest.

## Limits

This report does not claim a Windows build, an executed OpenStrata source
build, a hosted CI result, a Jolt solver, or any rigid-body behavior. The
Windows Visual Studio preset could not run locally because this host has no
Visual Studio 2022 instance. Visual Studio 2026 was later discovered by
OpenStrata.

The later [Windows OpenStrata report](2026-09-21-openstrata-windows.md)
supersedes the first two limits after OpenStrata discovered Visual Studio 2026
through its developer-environment integration.
