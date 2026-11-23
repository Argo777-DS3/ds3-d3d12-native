# I-0102 - Nettoyage du code

Capability: C3

FINISH-LINE line 2 bis (Thomas, 2026-10-28 04:03: « un nettoyage du code + relecture opus xhigh neuve derrière », placed after
I-0099 and before the effects' census, which will rework the code).

## Question

Which code of the native module (`native/`) and its scripts can go without changing the game's behaviour or any tool still in
use, and is it removed?

## Scope

Included:

- `native/frame/game_bootstrap.cpp` and its `.inc` first, then the rest of `native/frame`, `native/renderer`, `native/world`:
  unreferenced functions and data, diagnostics no kept runner can turn on any more, duplicated paths (one kept), dead branches,
  stale comments; the diagnostic pictures every native run writes without a flag (I-0099 final review L10).
- The scripts those removals orphan.

Excluded: any change of behaviour (the rule of 2026-10-19), renaming and reformatting, the decompiled sources, the effects' work
(FINISH-LINE lines 3 to 5), flags a kept runner or tool still places.

## Inputs

- The census of candidates (a Sonnet subagent, read-only, `evidence/I-0102/CENSUS.md`), each candidate checked by the pilot
  before it is removed.
- The final and step reviews of I-0100, I-0101 and I-0099 (their « left » items).
- Tools: clangd, the MSVC build and its unit tests (`native/frame/Build-GameBootstrap.ps1`), `scripts/Test-Project.ps1`.

## Expected evidence

The build and every unit test passing after each step; `Test-Project` passing; after the merge the one-minute start check
(D-0229); a full review of the branch by a fresh Opus at xhigh (Thomas's request), looking first for a behaviour change.

## Exit conditions

- Success: the removals merged, the review answered, the start check passed.
- Refuted: a removal changes behaviour (a review or the start check shows it): it is put back.
- Stagnation: none expected; the census bounds the work.
- Budget exhausted: close the tranche and replan under `.project/WORKFLOW.md`.
