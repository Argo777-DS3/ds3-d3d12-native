# I-0120 - La revue du code entier

Capability: C3

## Question

What does a fresh full read of the whole native code find (correctness, the mandate, optimise-do-not-stack) before the R
pilot continues alone, at 75 % of the weekly limit (Thomas's rule, relayed by the coordinator 2026-11-02 21:20)?

## Scope

Included:

- Every production source of `native/` except `third_party/`: 244 files, 5.0 MB (59 000 lines), at `main` `831023b8` (the
  open-gaps checkpoint; tranche 17 stays off `main` on `claude/compassionate-maxwell-d20ef9` and is not reviewed).
- The session's `code-review` skill at xhigh (it reviewed the latest native merge, `493e58aa`, since the branch had no diff).
- Eight fresh Opus 5.5 reviewers at xhigh, one lot each ([REVIEW-BRIEF](REVIEW-BRIEF.md): the brief and the lots).

Excluded:

- The unit tests (`*_test.cpp`, 8 500 lines), the PowerShell tools, the HLSL of the game's own programmes, the documentation.
- Any correction: the review is read-only; findings are recorded here and corrected only with Thomas's agreement.
- Builds and live runs.

## Inputs

- Code: the checkout at `831023b8`.
- Rules the reviewers apply: the mandate (no D3D11 bridge, D-0084, D-0155), no stand-in (D-0197), skip the object (FINISH-LINE
  line 1), optimise do not stack (WORKFLOW « Method rules »), two frames in flight (D-0187), the render worker (D-0168), the
  frame graph's barriers (D-0171).
- Tools: Read, Grep, clangd; the `code-review` skill; the Agent tool (Opus 5.5, xhigh).

## Hypotheses

- None: a review, not an experiment. Each finding is CONFIRMED (the failure follows from the code as written) or PLAUSIBLE (it
  depends on a runtime condition the code does not settle); the thread re-reads the high and medium ones itself before
  recording them.

## Expected evidence

File and line, severity, status, a concrete failure scenario and a fix for each finding ([REVIEW](REVIEW.md)).

## Exit conditions

- Success: every lot reviewed, the high and medium findings re-read by the thread, REVIEW written, Thomas told.
- Insufficient information: a lot whose reviewer could not read all its files is named as such.
- Budget: one pass; no second round without Thomas.

## Safety and human interaction

No live run, no build, no code change. Thomas decides which findings are corrected and by whom (the R pilot or a later thread).
