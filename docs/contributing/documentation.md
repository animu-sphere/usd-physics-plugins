# Documentation guidelines

A public behavior, contract, package or dependency change updates the document
that owns it in the same change. The [documentation index](../README.md) maps
each kind of information to its sole owner.

## 1. Category ownership

| Category | Responsibility |
| --- | --- |
| `architecture/` | Component identities, dependency edges, build modes, dependency pins, installation, compatibility and release rules. |
| `design/` | Normative contracts, rationale, invariants and non-goals; no implementation status or dated proof. |
| `reference/` | Evidence-backed facts; the capability matrix alone owns current support status. |
| `roadmap/` | Ordered unfinished outcomes, acceptance criteria and tracking links. |
| `reports/` | Dated, append-only facts from actual verification, including environment and limitations. |
| `contributing/` | Repository and documentation maintenance. |

Create guides only for verified user workflows and release records only for
actual releases. Keep README focused on purpose, minimal setup and links; do
not add phase completion tables, dated implementation lists or test results.
Keep design documents free of TODO lists and changing support claims.

## 2. Status vocabulary

Design decisions may be `proposed`, `accepted`, `superseded` or `rejected`;
these are decisions, not implementation states. Capability states and their
definitions belong only in
[CAPABILITY_MATRIX.md §1](../reference/CAPABILITY_MATRIX.md#1-status-vocabulary).
Executable support requires an automated test or dated verification evidence.
A documentation-only row may cite its directly inspectable owning page.

Keep each capability's state, minimal boundary and evidence in its row. Do not
repeat the table in an introductory status summary. Local success is not hosted
success, and experimental results do not establish a supported release.

## 3. Stable numbering

Preserve numbered design/architecture section meanings and existing anchors.
Append sections or add subsections rather than renumbering cited sections.
Open questions use never-reused prefixes: `RB-O`, `USD-O`, `SM-O` and `ARCH-O`.

## 4. One source of truth per fact

Link to owners; do not copy their tables, status sentences or progress rates.
There are no exceptions to this rule. The roadmap index is navigation only;
[current.md](../roadmap/current.md) owns all unfinished work, with three to five
active outcomes. Each outcome contains only Outcome, Acceptance and Tracking.
Use a real Issue/PR URL when available; otherwise state that it has not been
opened. Do not invent tracking numbers.

Remove completed outcomes from the roadmap. Put verification evidence in a
new dated report or CI run and link it from the appropriate capability row.
Never rewrite an old report to imply a newer result. Measurements such as run
times belong only in dated reports, not README, design or roadmap pages.

## 5. Language and form

- Repository documents are in English and use relative links within this repo.
- Put paths, commands, schemas, attributes and identifiers in code spans.
- Keep document indexes synchronized when adding or removing pages.
- Do not commit machine-local paths; use `<OpenUSD-prefix>` or similar placeholders.
- Use "must" for binding invariants, "should" for preferred design, and "may"
  for allowed options. Keep intentions separate from implementation facts.

## 6. Change destination checklist

| Change | Update |
| --- | --- |
| Public behavior or support boundary | Only the affected capability rows, with evidence. |
| Semantics or public contract | Owning design or architecture contract. |
| Unfinished work advances or completes | Relevant current-roadmap outcome; remove it when complete. |
| Experiment, timing or CI measurement | New dated report or specific CI run. |
| First-use instructions | Root README. |
| Dependencies, targets or compatibility | Architecture contract and matching build/manifests. |

One behavior change gets one capability-status update. Do not append a
"current situation" paragraph to every document in a PR.

## 7. Lightweight checks

Run `python scripts/check_docs.py --selftest` and `python scripts/check_docs.py`.
Also run `cmake -DPROJECT_ROOT=. -P tests/check_docs.cmake` from the repository
root to cover the existing `workspace.docs` CTest path, including README badges.
CI runs the same checks without SDK setup or external API requests. The checker
validates relative paths with case-sensitive spelling, Markdown heading anchors
and the format of GitHub Issue/PR links in the roadmap. Human review uses the
PR template to check responsibility and evidence; do not build a semantic-drift
inference system.
