# Roadmap

The roadmap contains incomplete work only. Phase definitions and acceptance
criteria are owned by
[DESIGN_POLICY.md §14](../design/DESIGN_POLICY.md#14-phases); this page owns
status and ordering.

## Status at a glance

| Phase | Status | Current outcome |
| --- | --- | --- |
| 1 — extract `physicsCore` | in progress | Neutral values and typed handles are implemented; descriptors and validation are next. |
| 2 — extract `physicsJolt` | not started | Waits for Phase 1 and an accepted Jolt dependency. |
| 3 — Stage Runner migration | not started | Waits for installable Phase 1–2 packages. |
| 4 — `physicsUsd` foundation | not started | Waits for the neutral rigid-body contract. |
| 5 — MMD validation | not started | Waits for the USD bridge and a generated vertical-slice fixture. |
| 6 — secondary-motion core | not started | Deferred until rigid-body extraction is stable and a VRM slice validates the descriptor. |
| 7 — VRM validation | not started | Waits for Phase 6 and a format-owned adapter. |
| 8 — vehicle primitives | not started | Waits for lower-level contracts to stabilize. |

No release assignment is made yet. A release column is added when the first
version plan exists; release numbers do not belong in the phase definitions.

## Current plan

[current.md](current.md) contains the ordered Phase 1 extraction work.
Completed items leave that page and are reflected in the
[capability matrix](../reference/CAPABILITY_MATRIX.md).

## Status rules

- **in progress:** implementation work for the phase exists in the current
  development line;
- **not started:** no implementation work has landed;
- **blocked:** work cannot proceed until a named external dependency or
  decision changes.

A future phase is not marked blocked merely because earlier phases must occur
first.
