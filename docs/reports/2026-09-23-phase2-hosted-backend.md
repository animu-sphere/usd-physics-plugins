# 2026-09-23 Phase 2 hosted backend verification

## Scope

This report records the hosted Windows and Linux verification of the extracted
`physicsJolt` implementation at commit
`0851d9e3717e11401e1ee1d3ef442a2f29e88781`. GitHub Actions run
[`35754385903`](https://github.com/animu-sphere/usd-physics-plugins/actions/runs/35754385903)
was triggered for the `codex/physics-jolt-backend` pull request and completed
successfully on 2026-09-22 UTC.

The run used the generated OpenStrata source-CI workflow, the `jolt` intent,
OpenStrata 0.23.2, and Jolt Physics 5.5.0 at commit
`23dadd0e603f1b321142d4c74df07fce85064989`.

## Result

All three hosted jobs completed successfully:

- `workspace-graph-pr` validated the CI manifest and workspace dependency
  graph on `ubuntu-24.04`;
- `workspace-pr-linux` built the pinned Jolt dependency, materialized and
  validated the pinned runtime, built the workspace, and passed the workspace
  test suite on `ubuntu-24.04`; and
- `workspace-pr-windows` performed the equivalent Jolt-required build and test
  path on `windows-2022`.

Both platform cells passed the `ost build --intent jolt` and
`ost test --intent jolt` steps. This exercises the focused backend behavior and
clean-prefix installed-consumer path described by the local Windows and Linux
Phase 2 reports through the hosted OpenStrata composition.

## Conclusion

The hosted evidence closes the remaining Phase 2 roadmap item. The extracted
`physicsJolt` package has passing local and hosted Windows/Linux evidence, and
Stage Runner package migration can begin as Phase 3.
