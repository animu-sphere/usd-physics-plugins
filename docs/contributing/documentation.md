# Documentation guidelines

Documentation is part of the implementation contract. A change is incomplete
when it changes a public boundary, implemented architecture, dependency,
package surface, or delivery status without updating the document that owns
that fact.

## 1. Category ownership

| Category | Put this here | Not this |
| --- | --- | --- |
| `architecture/` | Component identities, dependency edges, layout, build modes, external dependencies, and package surfaces. | Design rationale or unimplemented features described as present. |
| `design/` | Intended contracts, rationale, invariants, non-goals, and open questions. | Claims that something is implemented. |
| `reference/` | Facts about the current tree and evidence-backed capabilities. | Plans or aspirational support. |
| `roadmap/` | Incomplete ordered work, blockers, and completion criteria. | Completed work or long-form rationale. |
| `guides/` | How to complete a task, using commands that have been run. | Speculative commands. |
| `releases/` | One immutable record per released version. | Work in progress. |
| `reports/` | Dated, append-only evidence from real runs. | Current-state claims without evidence. |
| `contributing/` | How to maintain the repository and its documents. | End-user workflows. |

`guides/`, `releases/`, and `reports/` are created only when they have real
content.

## 2. Status vocabulary

- A design document is `proposed`, `accepted`, `superseded`, or `rejected`.
- A roadmap item is `in progress`, `not started`, or `blocked`.
- The capability matrix uses `supported`, `partial`, `experimental`,
  `unsupported`, `planned`, and `not present`.
- `supported` executable behavior requires an automated test or a dated report
  linked from the capability row. A documentation-only row may cite the
  owning, directly inspectable page and a link check.
- A release record and dated report are immutable. Later findings go in a new
  record with a forward note from the old one when needed.

## 3. Stable numbering

Design and architecture documents number their sections. A section number does
not change meaning after another page cites it; revisions append sections or
add subsections instead of renumbering.

Open questions use a document-specific prefix and a never-reused number:

- `RB-O<n>` for the rigid-body contract;
- `USD-O<n>` for the USD bridge contract;
- `SM-O<n>` for the secondary-motion contract;
- `ARCH-O<n>` for workspace decisions.

## 4. One source of truth per fact

- What a phase means: [DESIGN_POLICY.md §14](../design/DESIGN_POLICY.md#14-phases).
- Which phase is active: [roadmap/README.md](../roadmap/README.md).
- Structure and dependency edges:
  [WORKSPACE.md](../architecture/WORKSPACE.md).
- Exact third-party versions:
  [DEPENDENCIES.md](../architecture/DEPENDENCIES.md).
- Installed package promises:
  [PACKAGE_CONTRACT.md](../architecture/PACKAGE_CONTRACT.md).
- Release identity and promotion gates:
  [RELEASE_SCHEMA.md](../architecture/RELEASE_SCHEMA.md).
- What is implemented:
  [CAPABILITY_MATRIX.md](../reference/CAPABILITY_MATRIX.md).

Summaries link to the owning page rather than silently creating a second
contract.

## 5. Language and form

- Repository documents are in English.
- Use relative links for files in this repository.
- Put commands, paths, targets, schemas, attributes, and identifiers in code
  spans.
- Keep `docs/README.md` and `roadmap/README.md` synchronized with their
  directories.
- Do not commit machine-local paths. Use placeholders such as
  `%USERPROFILE%`, `$HOME`, or `<OpenUSD-prefix>`.
- Keep intended and implemented behavior visibly separate.
- Use "must" only for a binding invariant, "should" for the preferred design,
  and "may" for an allowed option.

## 6. Change checklist

1. Planned behavior is not presented as implemented.
2. Every new page appears in its category index.
3. Relative links and heading anchors resolve.
4. Structural changes update `architecture/` before or with implementation.
5. Capability changes update `reference/` and cite their evidence.
6. Completed work leaves the active roadmap.
7. A departure from a design contract is recorded in that contract, not only
   in code or a pull request.
