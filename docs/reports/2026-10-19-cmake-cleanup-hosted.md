# CMake cleanup hosted acceptance (2026-10-19)

This records hosted evidence after the local
[CMake cleanup verification](2026-10-19-cmake-cleanup.md), without changing that
historical report. [PR #27](https://github.com/animu-sphere/usd-physics-plugins/pull/27)
ran commit `48d3ab5330619494f43fc6d42a8109a4d113a749` in
[run 39187596694](https://github.com/animu-sphere/usd-physics-plugins/actions/runs/39187596694).
All five jobs completed successfully.

| Hosted cell | Workspace CTest result | Additional verification |
| --- | --- | --- |
| Windows `usd-jolt` | 20/20 | Isolated core/backend/USD build, test, consumer and package step passed |
| Linux `usd-jolt` | 20/20 | Isolated core/backend/USD build, test, consumer and package step passed |
| Windows `usd-jolt-six-dof` | 23/23 | Experimental execution; isolated packaging intentionally skipped |
| Linux `usd-jolt-six-dof` | 23/23 | Experimental execution; isolated packaging intentionally skipped |
| Linux graph / no SDK | 11/11 | Plain CMake with USD OFF and Jolt discovery disabled; graph validation and evidence upload passed |

Each root suite includes `workspace.installed_consumer`. Its unchanged script
checks clean-prefix installation, exports without checkout paths and
compatibility rejection for mismatched/missing core identity, toolchain and
edited installed headers. SDK-enabled variants add incompatible Jolt definition
rejection; USD variants add bridge/core mismatch rejection without a backend
import. The No-Jolt consumer exercises unavailable-backend behavior.

The subsequent documentation change at
`7c6838ab6c50e33e45056bde809803aa75fc4a86` also passed
[source run 39189653797](https://github.com/animu-sphere/usd-physics-plugins/actions/runs/39189653797).
These runs precede the scene-gravity implementation and are not hosted evidence
for that new API. The graph fallback cell runs on Linux; Windows fallback
evidence remains the local run in the earlier report. No published release or
new public SDK artifact is established by these source CI results.
