# usd-physics-plugins {tag}

Neutral rigid-body physics for OpenUSD runtimes: `physicsCore` values and world
interfaces, the `physicsJolt` backend and `physicsUsd`'s read-only standard
`UsdPhysics` snapshots. Consumers reach this repository only through installed
packages.

- **Release record:** [docs/releases/{tag}.md](https://github.com/animu-sphere/usd-physics-plugins/blob/{tag}/docs/releases/{tag}.md)
- **Capability matrix:** [CAPABILITY_MATRIX.md](https://github.com/animu-sphere/usd-physics-plugins/blob/{tag}/docs/reference/CAPABILITY_MATRIX.md)
- **Release schema:** [RELEASE_SCHEMA.md](https://github.com/animu-sphere/usd-physics-plugins/blob/{tag}/docs/architecture/RELEASE_SCHEMA.md)
- **Dependencies:** [DEPENDENCIES.md](https://github.com/animu-sphere/usd-physics-plugins/blob/{tag}/docs/architecture/DEPENDENCIES.md)

{changelog}

## Consuming a library from another repository

Each library is published as a `.tar.zst` asset below and as a digest-pinned
OpenStrata artifact in `ghcr.io/animu-sphere/usd-physics-plugins`, tagged
`<library>-{version}-<target>`. Paste the entries you need under
`requires.libraries`; selecting `physicsJolt` or `physicsUsd` also needs
`physicsCore`.

{pins}

**`physicsJolt` needs a matching Jolt SDK.** The package does not contain Jolt.
It rechecks Jolt 5.5.0 and the exact compile definitions it was built with when
a consumer configures, and refuses any other build
([DEPENDENCIES.md](https://github.com/animu-sphere/usd-physics-plugins/blob/{tag}/docs/architecture/DEPENDENCIES.md#3-jolt-physics)).
`physicsUsd` needs the same OpenUSD 26.08 runtime as its consumer.

## Artifacts

| artifact | contents |
| --- | --- |
| `<library>-{version}-<target>.tar.zst` | one installed library for `<target>`: `physicsCore`, `physicsJolt`, `physicsUsd` |
| `<library>-{version}-<target>.manifest.json` | its OpenStrata manifest |
| `external-library-pins.md` / `.json` | the pin table above, as a file |
| `usd-physics-plugins-{version}-src.tar.gz` | source archive at this tag |
| `SHA256SUMS` | SHA-256 checksums of every file above |

## SHA-256 checksums

```text
{checksums}
```
