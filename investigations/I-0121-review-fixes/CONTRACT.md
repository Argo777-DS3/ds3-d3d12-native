# I-0121 - Les corrections de la revue

Capability: C3

## Question

Correct the findings of the full code review [I-0120](../I-0120-full-code-review/REVIEW.md) in the order Thomas chose
(2026-11-02, relayed by the coordinator: its « Priorities » 1 to 3): which of them does the code fix, and how, without a new
stand-in, refusal or flagless diagnostic?

## Scope

Included:

- Priority 1: G1 (a menu model freed while the offscreen collect reads it), C1 (D24490 called from the admission workers beside
  the registry's mutators).
- Priority 2: B1 (the paint epoch), D1 (dropped effect frames), D2 (effects drawn with a null view), E1 (a view context per
  output size), F1 (the BLAS compaction slot freed on expiry).
- Priority 3: the flagless diagnostics (H1, A5 = G4, B2, B3, B7, C3, C8, D5, T1; H2 with them).
- I-0119's S1 and S6 (the skill pass on R3a/R3b), placed where the pilot finds them useful.

Excluded:

- Priorities 4 and 5 and A1 (the release blocker), unless a fix above touches the same code.
- Any live measurement (D-0227); the routine one-minute start check after the merge stays.

## Inputs

- [REVIEW](../I-0120-full-code-review/REVIEW.md): each finding with file, scenario and proposed fix.
- The code at `main` after I-0119 R3c's merge.
- Tools: clangd, `native/frame/Build-GameBootstrap.ps1` (unit tests), the `step-reviewer` and `final-reviewer` subagents.

## Hypotheses

- H1: every finding of priorities 1 and 2 has a local structural fix (a wait, a lock, a narrower epoch, a deferred give-back, a
  bounded cache, a slot freed on completion) that keeps the original's behaviour.
- H2: the flagless diagnostics can go behind their existing flags without changing what play draws.

## Expected evidence

The code read and changed, clangd 0 errors, the unit tests passing in the build, a step review and the final review (xhigh) on
the branch, the start check after the merge. Neither high finding has been seen in a run: their fixes are structural, not proved
by a reproduction.

## Exit conditions

- Success: priorities 1 to 3 fixed or recorded as refuted with the reason, reviewed, merged, start check passed.
- Refuted: a finding shown wrong by the code is recorded as such in the RESULT, not fixed.
- Insufficient information: a fix needing a runtime fact the code cannot give is recorded with the witness it needs.
- Stagnation: two failed local fixes of one finding open an RCA.
- Budget exhausted: close the tranche and replan under `.project/WORKFLOW.md`; renew a bounded budget automatically within unchanged authorized scope and existing protected boundaries.

## Safety and human interaction

No live run before the merge (D-0227); the start check after it is announced (D-0229). The weekly limit is at 75 %: one pilot,
no optional work.
