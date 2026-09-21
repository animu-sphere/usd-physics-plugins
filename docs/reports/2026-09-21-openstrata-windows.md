# 2026-09-21 Windows OpenStrata verification

## Environment

- OpenStrata: 0.23.2
- Runtime artifact: `sha256:ebb0c7da509ee14ada19ee5b461de6996aad0024b5c9640f12dde76912e849b5`
- OCI manifest: `sha256:d3ff79a6f330558c3b9a427a927d340fe3dcab1fe89107faa0ea9f66a104b7bf`
- Runtime: `cy2026-windows-x86_64-py313-usd`, OpenUSD 26.08
- Compiler: MSVC 19.51.36256 from Visual Studio Community 2026
- Generator: Ninja bundled with Visual Studio

The runtime pull verified the artifact digest, 4,171 file digests, SPDX SBOM,
SLSA/in-toto provenance, archive safety, and attested trust before
materialization. Local runtime validation passed. Its configure/link probes
were skipped because the validator did not discover Visual Studio 2026 through
its legacy default-generator probe; the subsequent OpenStrata build loaded
`vcvars64.bat` and compiled successfully.

## Root workspace

`ost build` configured and built the root workspace in Release. The first
`ost test` exposed two test-harness defects: documentation scanning entered the
materialized runtime, and the nested installed-consumer configure did not
inherit the selected Ninja/compiler/toolchain. After fixing both boundaries,
the second run passed all five tests:

- `physicsCore.scaffold`;
- `physicsJolt.scaffold`;
- `workspace.boundaries`;
- `workspace.docs`; and
- `workspace.installed_consumer`.

## Isolated libraries and packages

`ost library verify-consumer` passed for both packages after their explicit
package modes and package contracts were added. The `physicsCore` consumer
resolved one package; the `physicsJolt` consumer resolved the two-package
closure containing `physicsCore`.

Both isolated library CTest suites passed 1/1. Packaging produced:

- `physicsCore` 0.1.0: six files, no dependencies,
  `sha256:0d738d7490b1d25a6d657f5bf64a316a8ec425568bc006808f34926c46e70fc4`;
- `physicsJolt` 0.1.0: six files, one dependency,
  `sha256:43434a1085d98f688c554b53b6015c2e731e21dbfa288c4a458badc821af2364`.

The archives and OpenStrata evidence records are generated outputs and are not
committed.

## Limits

This is local Windows evidence, not a hosted CI result. It does not claim a
Jolt solver or rigid-body behavior; `physicsJolt` remains an unavailable
backend scaffold.
