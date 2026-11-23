# I-0137 - La robustesse

Capability: C3 · ROADMAP workstream 4 (« skip the object and go on » everywhere), its steps 3 and 4 (the coordinator's order,
2026-11-12, after D-0273 and D-0274): the full code review [I-0120](../I-0120-full-code-review/REVIEW.md)'s A1, then its
priority 4, then its priority 5; from tranche 4 (the coordinator's order of 12:46, a fresh thread on `claude/sharp-tereshkova-5cbcb6`)
also its steps 5, then 1 and 2. The E pilot, first on `claude/jovial-elbakyan-42aa70`, after [I-0113](../I-0113-open-gaps/RESULT.md)
closed.

## Question

For each finding of I-0120 left open by [I-0121](../I-0121-review-fixes/RESULT.md) (A1; priority 4: A3, B6, G5, G6, C2, C4, E3,
A2; priority 5: the low ones; H2), is it still true in today's code, and what structural correction makes the module skip, wait
or recover where it closed the game, kept a stale value or drew a stand-in?

## Scope

Included, in this order, each a tranche merged once written and reviewed (D-0227):

1. **A1** (blocks any release, ROADMAP workstream 4 step 4): the controller accepts only the default Steam path.
2. **Priority 4**: A3 (a texture the decoder refuses closes the game), B6 (an instance read that fails closes the game), G5 and
   G6 (the UI's own caps close the game), C2 (the water's Fresnel stale after a Reflection option change), C4 (the lighting
   programme set frozen at its first complete form), E3 (a recoverable resize failure refused), A2 (the black stand-in for a
   movie frame or a menu proxy with no native target yet).
3. **Priority 5**: the low findings (A7 to A9, B5, B8 to B11, C6, C7, C9, C10, D6, D7, E4 to E8, F2 to F8, G7 to G10, H2 to H6,
   S2 to S5, S7 to S13, and A4, A6, B4, C5, D3, D4, G2, G3 left by priorities 1 to 4), each re-read first: the review is of
   `831023b8` (2026-11-02) and ten days of merges have moved or removed code.

Excluded:

- Until tranche 4, ROADMAP workstream 4's steps 1, 2 and 5 (the UI selectors without a programme, the `Unsupported*` stubs and
  I-0101's stops, the exit faults); from tranche 4 they are in scope (step 5 first, then 1 and 2), each a tranche of its own.
- The installer and the production build (workstream 10); a new launcher.
- Any change of a test, a tolerance or a reader that is not stated in the RESULT (D-0268).

## Inputs

- [REVIEW](../I-0120-full-code-review/REVIEW.md) (the findings, their status and the thread's re-reading), I-0121's RESULT (what
  priorities 1 to 3 changed), WORKFLOW « Skip the object and go on » and « No pop-in » (D-0197), I-0101's STUDY.
- The original's functions the module replaces, read with GhidraMCP or the mirrors when a skip must leave the engine's object in
  the state the original would (what the original does when a texture creation fails, for A3).
- Tools: clangd, `native/frame/Build-GameBootstrap.ps1 -Out` (the unit tests), the `step-reviewer`, `final-reviewer` and
  `integrity-reviewer` subagents.

## Hypotheses

- H1: every finding of the scope is still present as the review states it.
- H2: some were removed or changed by the merges since (I-0131's raster removal, R11, R12): recorded as such, not corrected.

## Expected evidence

For each finding: the present code read (file and line), the correction and its unit test where the code is testable without the
game (the controller's identity check, a parser, a cache key), the build's unit tests passing, the reviews. No live run before a
merge (D-0227); the start check after each. A correction that only a run can show (a resize failure, a refused texture) is said
unproved by a run.

## Exit conditions

- Success: each finding corrected, or recorded as no longer true, or left with its reason.
- Refuted: a finding the present code contradicts is recorded.
- Insufficient information: a correction that needs the original's behaviour not yet read is deferred with what must be read.
- Stagnation: no new discriminating evidence after 2 cycles on one finding: recorded and the next one taken.
- Budget exhausted: close the tranche and replan under `.project/WORKFLOW.md`; renew a bounded budget automatically within unchanged authorized scope and existing protected boundaries.

## Safety and human interaction

Start checks only (after each merge, announced, through the coordinator). No live run is planned; a witness run of a corrected
path would be asked of the coordinator and announced under the Live game rule. Nothing irreversible; no Windows, NVIDIA or
security setting touched.
