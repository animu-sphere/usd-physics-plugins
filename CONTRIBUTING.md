# Contributing

Thanks for taking an interest in `usd-physics-plugins`. Documentation, generated
fixtures, build fixes and clear bug reports are useful contributions. Small
pull requests do not require advance permission.

## Before you start

- Search existing issues and pull requests.
- For a substantial API or behavior change, open an issue with the consumer,
  neutral semantics, units, errors, minimal acceptance scenario and package impact.
- Use the [bug report](.github/ISSUE_TEMPLATE/bug_report.md) or
  [feature request](.github/ISSUE_TEMPLATE/feature_request.md) template.
- Prefer generated fixtures. Share models, motions, textures or screenshots only
  when their licenses permit redistribution.
- Report vulnerabilities privately through [SECURITY.md](SECURITY.md).

## Ownership and scope

Read the [design policy](docs/design/DESIGN_POLICY.md) and
[workspace contract](docs/architecture/WORKSPACE.md). Neutral physics belongs in
`physicsCore`, Jolt execution in `physicsJolt`, and read-only standard USD
interpretation in `physicsUsd`. Scheduling, mappings and display belong in
consumers; MMD/VRM interpretation and bone coupling stay with their adapters.

Prioritize the [current roadmap](docs/roadmap/current.md). New shared APIs need
a real consumer and a minimal executable acceptance scenario. Exact Bullet
trajectories are not a general merge requirement.

## Working locally

The [README](README.md#build-and-use) gives platform preset commands;
[WORKSPACE.md §5](docs/architecture/WORKSPACE.md#5-build-modes) defines options,
test labels and OpenStrata responsibilities, and
[DEPENDENCIES.md](docs/architecture/DEPENDENCIES.md) owns SDK pins.

For documentation changes, run:

```powershell
python scripts/check_docs.py --selftest
python scripts/check_docs.py
```

For code changes, run the focused checks that protect the affected contract
and consumer scenario. Add tests for a public contract, a real regression, an
uncovered boundary or a consumer acceptance requirement. Preserve existing
representative tests; avoid multiplying fidelity cases or CI cells just for
coverage. State unavailable checks and remaining uncertainty in the PR.

## Pull requests

Keep changes focused. Explain the problem, resulting behavior, ownership and
validation using the [PR template](.github/PULL_REQUEST_TEMPLATE.md).
Update only the owning documents under the
[documentation guidelines](docs/contributing/documentation.md): capability rows
for behavior, contracts for semantics, current roadmap for unfinished work and
new dated evidence for measurements. Do not rewrite historical reports.

Contributions are made under the repository's [Apache-2.0 license](LICENSE).
Respect third-party licenses and the [Code of Conduct](CODE_OF_CONDUCT.md).
