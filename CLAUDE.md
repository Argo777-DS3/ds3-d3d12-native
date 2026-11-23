# Working contract

This repository is the persistent memory of the Dark Souls III native D3D12 project (« Dark Souls III: Rekindled ») and the
only reference for the rules; chats are not project memory (D-0214).

## Start here

Read [`.project/INDEX.md`](.project/INDEX.md), STATE's resume rows ([`.project/STATE.md`](.project/STATE.md)), the current
workstream in [`.project/ROADMAP.md`](.project/ROADMAP.md), the decisions in force
([`decisions/IN-FORCE.md`](decisions/IN-FORCE.md)), then only the records the question needs. Rules:
[`.project/WORKFLOW.md`](.project/WORKFLOW.md), tools in [`.project/TOOLS.md`](.project/TOOLS.md). After a context compression
or in a new session, skill `resume-thread` comes before any technical decision, implementation or live test. Archives
(`.project/archive/`, `timing/`, the Git tags) are read only when a question needs them.

## Rules for every turn

- Thomas directs goals, scope and trade-offs. Opus 5.5 high pilots by default; mechanical work always goes to Sonnet 5.5
  subagents; delegation, reviews and Fable follow WORKFLOW « Roles and delegation » (D-0245).
- A pilot never yields its turn except for a necessary human intervention, a decision that is Thomas's, a destructive or
  protected action, a real technical inability or an explicit `PAUSE`; a message from Thomas is not a stop request unless it
  says so (D-0245). The study thread is not bound by this.
- Use the project's and the session's skills (`resume-thread`, `live-run`, `merge-workstream`, `review-step`, `review-final`)
  for any procedure they cover; reviewers are the subagents `step-reviewer` and `final-reviewer` (WORKFLOW, end).
- Keep observation, interpretation, decision and evidence distinct; never promote plausible to true
  ([`knowledge/LEDGER.md`](knowledge/LEDGER.md)); validate an uncertain result before building on it; report a bad result as it is.
- Evidence lives in `C:/Users/User/Desktop/DS3_evidences` through the ignored `evidence/` junction (new checkout:
  `pwsh -NoProfile -File scripts/Connect-EvidenceStore.ps1`); never replace the junction, force-add evidence, or restore,
  recreate or reacquire removed evidence.
- Live tests only as WORKFLOW « Live game rule » says (D-0116, D-0117): announced, offline, hash-checked restore; never an
  unannounced run.
- Native C++: clangd (WORKFLOW « Native code navigation (clangd) »). Decompile only what the systems in scope need. After two
  failed local fixes, open an RCA.
- Background processes run hidden (D-0199); pushes and mass Git deletions are Thomas's.
- At 50 % of the weekly usage limit every pilot tells the coordinator, which stops the open-gaps pilot after its next clean
  checkpoint; the RT pilot continues (D-0248).

## Before ending meaningful work, and before every merge

Clean the documentation you touched (WORKFLOW « Threads and continuity », D-0291): STATE's rows rewritten, not appended to;
stale, contradictory or duplicated sentences removed; the size warnings of `Test-Project` answered. Update the ledger only on
evidence; record only consequential decisions in `decisions/LOG.md` with their line in `decisions/IN-FORCE.md`; run
`pwsh -NoProfile -File scripts/Test-Project.ps1` (PowerShell 7); make a coherent Git checkpoint. Recovery never authorizes a
merge into `main`.
