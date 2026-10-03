# Phase 4 local USD artifact preparation

Date: 2026-10-04 (Asia/Tokyo). Scope: local Windows validation of the
`usd-jolt` intent and isolated three-library packaging.

## Environment

Windows x86_64, MSVC 19.51 / Visual Studio 18 2026, CMake 4.4, Python 3.13.14,
OpenUSD 26.08, a compatible installed Jolt 5.5.0 SDK, and local OpenStrata
0.23.14. Source CI still pins OpenStrata 0.23.2; hosted reproduction remains
necessary.

## Results

`ost build --intent usd-jolt` succeeded. `ost test --intent usd-jolt` passed
12 tests, including `physicsUsd.box_scene`, the Jolt backend test, boundaries,
documentation, versions, and the clean-prefix installed consumer. That consumer
links and runs both the neutral/Jolt path and the installed Box reader.

Isolated `ost library build`, `ost library test`, and `ost library package`
succeeded for `libs/physicsCore`, `backends/physicsJolt`, and `libs/physicsUsd`.
The local archive content digests were:

| Library | Content digest |
| --- | --- |
| `physicsCore` | `sha256:1138cca3171d2eaef205043740c6fce6d7c0689ffffad399abfea82ed0bc79f9` |
| `physicsJolt` | `sha256:d44df53e8385d11b2f9861a1b5293c46f12da396c5519e63a16490dfe6800f3b` |
| `physicsUsd` | `sha256:f183b7588747f5e32e09f2503a44382fb44287df08d100730062607d674ed4a7` |

These archives are local evidence, not published consumer pins; they have no
OCI source digest in this report.

## Consumer-verification limit

`ost library verify-consumer libs/physicsUsd` rebuilt the library successfully
but failed consumer configuration because its plain-library closure excluded
the runtime SDK and could not resolve `pxr`. The generated consumer clears
`CMAKE_PREFIX_PATH`; adding the SDK to the parent environment does not repair
that closure. Source CI therefore uses this command only for `physicsCore`.
The root clean-prefix consumer explicitly supplies both USD and Jolt SDKs
and verifies the other installed targets. Generic SDK-aware consumer closure
remains an OpenStrata follow-up.

## Delivery boundary

Both source-CI test cells now select `usd-jolt`, and isolated packaging covers
all three libraries on both OSes. Linux/hosted execution, public `physicsUsd`
artifacts, Stage Runner digest pins, and a formal release remain pending.
