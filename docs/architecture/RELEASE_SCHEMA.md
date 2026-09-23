# Release schema

> **Status: proposed, 2026-09-23.** This defines the intended versioned
> release contract; it does not claim that a tag, release workflow, or GitHub
> release exists. The public Windows artifacts published during Phase 3 are
> consumer-validation inputs, not a complete `v0.1.0` release.

## 1. Release unit and scope

One repository version in `VERSION` applies to every released library. The
initial release set is `physicsCore` and the Jolt-enabled `physicsJolt`, for
`cy2026` / `usd` on Windows x86_64 and Linux x86_64. A new library or target
joins the set only after its clean-prefix and downstream-consumer checks pass.
macOS and a no-Jolt stub are not release targets for this first set.

The distributable unit is an individual OpenStrata library artifact, not a
workspace source checkout or an umbrella package. Its installed CMake target
and dependency contract remain governed by [PACKAGE_CONTRACT.md](PACKAGE_CONTRACT.md).
`physicsJolt` must record and enforce the Jolt version and ABI-affecting build
definitions used for that target; a matching library artifact alone does not
replace its external Jolt SDK dependency.

## 2. Registry identity and consumer pins

All released libraries use the existing public OCI repository
`ghcr.io/animu-sphere/usd-physics-plugins`. A discoverable tag has the shape
`<library>-<version>-<target>`, but a consumer must pin two digests obtained
from the actual published artifact:

```yaml
requires:
  libraries:
    - id: physicsCore
      version: ">=0.1,<0.2"
      artifact:
        targets:
          cy2026-windows-x86_64-py313-usd:
            digest: sha256:<archive-content-digest>
            source: oci://ghcr.io/animu-sphere/usd-physics-plugins@sha256:<oci-manifest-digest>
    - id: physicsJolt
      version: ">=0.1,<0.2"
      artifact:
        targets:
          cy2026-windows-x86_64-py313-usd:
            digest: sha256:<jolt-archive-content-digest>
            source: oci://ghcr.io/animu-sphere/usd-physics-plugins@sha256:<jolt-oci-manifest-digest>
```

The `digest` is the OpenStrata archive content digest; the digest in `source`
identifies its OCI manifest. They are not interchangeable. Tags are discovery
labels only and must never replace digest pins. Publishing a target again may
change its OCI digest, so the consumer pin table is generated from successful
push results, not from a planned tag or locally packaged archive alone.
Because `physicsJolt` declares `physicsCore`, a consumer selecting Jolt must
include both exact pins in the selected OpenStrata library closure.

The pin table must contain exactly one entry per released library and target,
with version, target, content digest, and OCI source. It ships as both a
machine-readable release asset and a copyable consumer example in the release
notes. A missing library or target fails the release rather than producing a
partial table.

## 3. Promotion gates

The eventual release workflow has two modes:

1. A manual dry run builds, tests, packages, and stages the complete release
   set, but neither pushes to GHCR nor creates a GitHub release. Its pin table
   includes local content digests and explicitly omits `source`.
2. A `vX.Y.Z` tag is accepted only when it matches `VERSION` and has finalized
   release notes. It runs the same verification, then publishes each library
   and target, generates the digest pin table, checksums the release files, and
   creates a draft GitHub release for human review.

The build matrix must mirror the corresponding source-CI runtime digests,
OpenStrata version, OS/architecture, Python ABI, and external Jolt revision.
Each target must pass root tests, isolated library tests, package creation,
clean-prefix installed-consumer tests, and a downstream Stage Runner consumer
check before promotion. Windows validation must include a compiler-compatible
link test against the published binaries; source-only tests cannot establish
that ABI compatibility.

Publishing credentials belong only to the tag-only publish job. PR and dry-run
jobs stay read-only. Release artifacts are retained with checksums and the
exact pins; publishing does not by itself mark a version released until the
draft is reviewed and published.

## 4. Current boundary

The existing public Windows `physicsCore` and `physicsJolt` OCI artifacts have
been pulled and verified from a fresh cache for the Phase 3 migration. They
remain valid exact-digest inputs for that consumer, but do not imply Linux
availability or a formal versioned release. The first release workflow should
be implemented and dry-run before any `v0.1.0` tag is pushed. Linux artifacts,
cross-repository hosted consumer evidence, and a finalized release note are
still required.
