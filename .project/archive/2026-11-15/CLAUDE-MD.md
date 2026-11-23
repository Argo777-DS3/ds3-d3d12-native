<!-- Archive copy of `CLAUDE.md` as it stood on main before the documentation diet of 2026-11-15 (D-0291, Thomas: « Je valide d'avance »). Text verbatim; only relative links are rewritten to resolve from this folder. Never an active instruction: the live file is `CLAUDE.md`. -->

# Working contract

This repository is the persistent memory for the Dark Souls III native D3D12 project (« Dark Souls III: Rekindled »). Chats
are not project memory; the repository is the only reference for the rules (D-0214).

## Start here

Read, in order: [`.project/INDEX.md`](../../INDEX.md), the resume rows of [`.project/STATE.md`](../../STATE.md), the
current capability in [`.project/ROADMAP.md`](../../ROADMAP.md), then only the records relevant to the question. The rules
are in [`.project/WORKFLOW.md`](../../WORKFLOW.md); repeated procedures are the project's skills (`resume-thread`,
`live-run`, `merge-workstream`, `review-step`, `review-final`), its reviewers the subagents `step-reviewer` and `final-reviewer`, and the
tool for each need is in WORKFLOW « Tools ». After
a context compression or in a new session, use skill `resume-thread` before any technical decision, implementation or live
test. The timing project (LTF/TRTF) is historical, under [`timing/`](../../../timing/IMPORT.md): read it only when a question needs it.

## Rules that apply to every turn

- Thomas directs goals, scope and trade-offs. Opus 5.5 at high effort is the default pilot; mechanical work always goes to
  Sonnet 5.5 subagents; delegation, reviews and Fable follow WORKFLOW « Roles and delegation » (D-0245).
- A pilot never yields its turn, except for a necessary human intervention, a decision that is Thomas's, a destructive or
  protected action, a real technical inability, or an explicit `PAUSE`; a message from Thomas is not a stop request unless it
  says so (Thomas, 2026-11-01, D-0245). The study thread is not bound by this rule.
- Use the existing skills (the project's and the session's) for any procedure they cover instead of redoing it by hand
  (Thomas, 2026-11-01); WORKFLOW « Skills and subagents of the project ».
- Keep observation, interpretation, decision and evidence distinct; never promote plausible to true (statuses in
  [`knowledge/LEDGER.md`](../../../knowledge/LEDGER.md)); validate an uncertain result before dependent work builds on it; report a bad
  result as it is.
- Evidence lives in `C:/Users/User/Desktop/DS3_evidences`, outside Git, through the ignored `evidence/` junction (in a new
  checkout run `pwsh -NoProfile -File scripts/Connect-EvidenceStore.ps1`); never replace the junction, force-add evidence, or
  restore, recreate or reacquire removed evidence ([EVIDENCE-RETENTION.json](../../EVIDENCE-RETENTION.json), STATE « Storage
  and recovery »).
- Live tests only as WORKFLOW « Live game rule » says (D-0116, D-0117): announced, offline, hash-checked restore. Never an
  unannounced run.
- Native C++: navigate and check `native/` with clangd (WORKFLOW « Native code navigation (clangd) »).
- Decompile only what the systems in scope need. After two failed local fixes, open an RCA.
- Background processes run hidden on Thomas's PC (D-0199); pushes and mass Git deletions are Thomas's to run.
- At 50 % of the weekly usage limit, every pilot sends the coordinator a note; the coordinator then stops the open-gaps pilot
  after its next clean checkpoint, and the RT pilot continues (Thomas, 2026-11-05, D-0248; WORKFLOW « Threads and continuity »).

## Before ending meaningful work

Before every checkpoint, clean the documentation you touched (WORKFLOW « Threads and continuity »): STATE's resume rows rewritten,
not appended to; stale, contradictory or duplicated sentences removed. Update the ledger only when evidence justifies it; record only consequential decisions in `decisions/LOG.md`; run `pwsh -NoProfile -File scripts/Test-Project.ps1`
(PowerShell 7); make a coherent Git checkpoint. Recovery never authorizes a merge into `main`.
