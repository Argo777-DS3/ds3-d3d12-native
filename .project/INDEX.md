# Project control plane

Each information type has one canonical home. Start with the current checkpoint;
load historical detail only when it answers the current question.

## Resume route

1. [`STATE.md`](STATE.md): working branch, current position, next action, open gaps and limits.
2. [`ROADMAP.md`](ROADMAP.md): the general plan of everything left, its phases and numbered workstreams (D-0273); C3 is the current capability.
3. [`NATIVE-D3D12-RESET-MANDATE.md`](NATIVE-D3D12-RESET-MANDATE.md): Thomas's objective and its governing decisions.
4. [`../decisions/IN-FORCE.md`](../decisions/IN-FORCE.md): one line per decision in force (D-0291); the full texts in the LOG.
5. [`WORKFLOW.md`](WORKFLOW.md): operating method and recovery; [`../CLAUDE.md`](../CLAUDE.md) is the entry point that
   routes here (only Claude works on the project, D-0261).
6. Relevant claims in [`../knowledge/LEDGER.md`](../knowledge/LEDGER.md), then their cited sources.

Everything else is read when a question needs it. The control files have size targets (WORKFLOW « Canonical sources »,
D-0291); text a cleanup takes out of them goes to [`archive/<date>/`](archive/2026-11-15/), never deleted without Thomas.

The programme: wave 1, [I-0066 « DS3 DX12 Vague 1 »](../investigations/I-0066-independent-native-startup/RESULT.md)
(closed, D-0194) with its static preparations I-0067, I-0068 and I-0069; wave 2, the remaster, from I-0070 on
([investigations index](../investigations/README.md)). STATE owns progress and resume conditions; an investigation's earlier plan
never overrides it.

## Ownership

| Information | Canonical location |
|---|---|
| Current position and resume checkpoint | `.project/STATE.md` |
| Programme contract | `.project/NATIVE-D3D12-RESET-MANDATE.md` |
| ER architectural method reference (read-only) | [Deep study716](../investigations/I-0066-independent-native-startup/ER-ENGINE-D3D12-DEEP-STUDY-716.md), with original provenance and external primary-source routes; the [D3D12 design review](../investigations/I-0066-independent-native-startup/D3D12-DESIGN-REVIEW.md) and the [pilot plan](../investigations/I-0066-independent-native-startup/PILOT-WORK-PLAN.md) carry the implementation order |
| The plan of everything left: phases, numbered workstreams, order, lanes, open decisions (D-0273) | `.project/ROADMAP.md`; [FINISH-LINE.md](FINISH-LINE.md) is the history of D-0228's list; [RT-PT-PLAN.md](RT-PT-PLAN.md) the RT/PT design of 2026-10-30 (D-0235), historical (D-0291); [OPTIMISATION-SURVEY.md](OPTIMISATION-SURVEY.md) workstream 15's items (D-0304) |
| A study ahead of a workstream not yet opened (the study thread's report and its annexes; the pilot copies it into its investigation in its first commit) | `.project/studies/<workstream>/` (now: [workstream 12](studies/workstream-12/STUDY.md), its summary for Thomas in [SYNTHESE](studies/workstream-12/SYNTHESE.md); the [security audit](studies/security-audit/REPORT.md) of 2026-11-23, its fixes made (not yet run in play), its left points in its « Left »; its reviewer `security-reviewer`, skill `review-security`) |
| Thomas's wishes (ideas, not decisions, not on the ROADMAP) | `.project/wishes/` (e.g. [the NPC « Claude »](wishes/NPC-CLAUDE.md), [the new pyromancy](wishes/NEW-PYROMANCY.md)) |
| Operating method (agents, delegation, live tests, checkpoints, clangd) | `.project/WORKFLOW.md`; the investigation and design method in [METHOD.md](METHOD.md) |
| Live-run know-how (pilots, menus and coordinates, capture, PowerShell pitfalls) | [LIVE-RUN-NOTES.md](LIVE-RUN-NOTES.md) |
| Repeated procedures and reviewer briefs (Claude Code skills and subagents) | `.claude/skills/`, `.claude/agents/` (listed at the end of WORKFLOW) |
| Tools: which one for which need, and their state on this PC | WORKFLOW « Tools » (the rules), [TOOLS.md](TOOLS.md) (each tool's row); the external tools study [TOOLS-STUDY.md](TOOLS-STUDY.md); watching films (motion curves, slit-scans, their limits) [VIDEO-STUDY.md](VIDEO-STUDY.md) |
| Technical claims and status | `knowledge/LEDGER.md`, `knowledge/claims/` |
| Consequential decisions | [decisions/LOG.md](../decisions/LOG.md) (full texts); [decisions/IN-FORCE.md](../decisions/IN-FORCE.md) (one line per decision in force) |
| Work contracts, results and history | [investigations/README.md](../investigations/README.md), `investigations/<id>/` |
| Evidence and provenance | External `C:/Users/User/Desktop/DS3_evidences`, through the ignored `evidence/` junction |
| Evidence retention | [EVIDENCE-RETENTION.json](EVIDENCE-RETENTION.json) |
| Replayed path gate: thresholds, their calibration, the last passing native tree (I-0083) | [REPLAY-GATE.json](REPLAY-GATE.json), written by `scripts/Invoke-ReplayGate.ps1` |
| Archived investigations (I-0001 to I-0065) | [ARCHIVE.json](ARCHIVE.json), Git tag `archive/pre-cleanup-2026-10-13` |
| Decompiled game code | [decompiled/README.md](../decompiled/README.md); local inputs in ignored `game/` |
| Native backend source | [native/README.md](../native/README.md) |
| High-framerate timing project (LTF/TRTF), historical, D-0196 | [timing/IMPORT.md](../timing/IMPORT.md): its own investigations I-0005 to I-0014, claims K-0010 to K-0064 and decisions D-0003 to D-0028, numbers that do not overlap the main registries, kept as they are (D-0196) |

Archived states are Git tags, never active instructions: `archive/pre-cleanup-2026-10-02`,
`archive/pre-cleanup-2026-10-13` (D-0177), `archive/pre-cleanup-2026-10-19` (the long STATE before
the documentation cleanup), `archive/pre-cleanup-2026-10-21` (STATE's workstream chain and ROADMAP's wave-2 narrative
before the second cleanup), `archive/pre-roadmap-2026-11-12` (the capability roadmap and its wave-2 narrative before the
general plan, D-0273) and `archive/pre-cleanup-2026-11-15` (before the documentation diet, D-0291, whose earlier texts are also
kept in main under [archive/2026-11-15/](archive/2026-11-15/)); see [ARCHIVE.json](ARCHIVE.json).
Historical links do not imply that removed raw evidence is still available;
retention rules prohibit automatic restoration. Do not create another registry
without a demonstrated recurring need.
