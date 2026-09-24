# 2026-09-23 Phase 3 Linux package artifacts

## Scope

The Linux cell of [GitHub Actions run 35852274531](https://github.com/animu-sphere/usd-physics-plugins/actions/runs/35852274531)
passed the workspace suite, isolated `physicsCore` and `physicsJolt` builds and
tests, `physicsCore` installed-consumer verification, and packaging on
`ubuntu-24.04`. The workspace suite includes the Jolt installed-consumer test
with the external Jolt SDK prefix. The Windows and graph cells also passed.

The Linux packages use the pinned `cy2026` / `usd` runtime and Jolt Physics
5.5.0 at `23dadd0e603f1b321142d4c74df07fce85064989`.

## Published inputs

The two package outputs were imported and checked with `ost artifact verify
--require-sbom --require-provenance`, then published to the existing public
`ghcr.io/animu-sphere/usd-physics-plugins` repository. A separate empty
OpenStrata store pulled both by OCI digest with their expected content digest,
kind, target, SBOM, and provenance requirements.

| Package | Archive content digest | OCI manifest digest |
| --- | --- | --- |
| `physicsCore` | `sha256:81a3433b759e6ec817188a2137f8da24ff8dbebed09a176f48a7ec2596d6f5ad` | `sha256:f050253cea8f6db163b67c9860e88bdf213c8ef400956c29fec130bc4b6726d7` |
| `physicsJolt` | `sha256:2be1f995f3e59d29fcc5a3db66321d4022fd83965d68f09e9b82b5b73bf2703b` | `sha256:e51df5e3815cbbe57034a390c6218f7467be70f92500b437eb7c77f2feda9020` |

The published tags are discovery labels. Consumers must pin both digests for
each package.

## Verification boundary

The first packaging run exposed that `ost library verify-consumer` clears
`CMAKE_PREFIX_PATH` for its generated consumer, so it cannot find the external
Jolt SDK required by `physicsJolt`. The passing run uses the root
installed-consumer test for Jolt and retains the isolated generated consumer
check for `physicsCore`.

These artifacts are Phase 3 consumer inputs, not a versioned `v0.1.0` release.
Stage Runner's hosted Windows and Linux consumer results and the release dry
run remain separate gates.

## Follow-up

Later on 2026-09-23 UTC, Stage Runner's
[hosted Windows and Linux run](https://github.com/animu-sphere/usd-stage-runner/actions/runs/35853380748)
passed 46 tests per platform using the published packages with artifact caches
disabled. Its separate plain-CMake run also passed. The versioned release dry
run remains open.
