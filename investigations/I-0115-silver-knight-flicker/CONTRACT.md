# I-0115 - Le Silver Knight qui clignote (near characters vanishing)

Capability: C3

## Question

Why does a Silver Knight (c1410) 2-4 m from the camera on the Anor Londo stairs vanish and come back from one frame to the next
natively, at 60 as at 240 (Thomas, 2026-11-01, series 29 of I-0114, the ASUS), when he never saw it in the original; and fix it.

## Scope

Included: the main view's decision for characters near the camera (DS3's frustum tests on the walk's culler with T2b's margin,
D-0220; the native's admission and its ordinary category from the fade group; the rows drawn), both renderers on the same path.

Excluded: distant characters' fades and LODs (I-0100, DS3's own), shadows.

Amended 2026-11-02 (the open-gaps pilot): Thomas's description that morning (the flesh masses on the left well into the
cathedral, the knights, gone and back within half a second, also with the enemies frozen) replaces series 29's « 2-4 m with AI »
as the question; H3 added: a drawn character's fade changing view (the pass-5 geometry weight against the 0x45 track weight) at
its LodParam second distance, which brings the fades and the shadows' fade into scope (RESULT « Step 3 »).

## Inputs

- I-0100 RESULT tranche 4 (the margin, the original's draws following DS3's frustum test, INFERRED for the knight at the view's edge).
- Series 30 of I-0114: `i114-st5-o60` (the original), `i114-st5-n60b`, `i114-st5-f240` (native), without AI, popin-observe.
- Tools: `investigations/I-0100-popin/Get-PopinJournal.ps1`, this folder's `Get-NearRows.py`, GhidraMCP.

## Hypotheses

- H1: a cull of a knight whose mesh is on screen (its bones' box outside the walk's culler while the mesh is in the drawn view).
- H2: not a cull: a near knight drawn natively with another state than the original's (its fade group at the camera's approach,
  the ordinary category 0x21/0x29 flipping, a row refused).

## Expected evidence

`popin_native_near` and `popin_reference_near` on the same path with AI: a native gap (not drawn while near) or a category or
fade change at the moment Thomas sees the knight vanish, and the original's rows at the same place.

## Exit conditions

- Success: the cause observed in both renderers and a fix that removes the native gaps the original does not have.
- Refuted: no native gap or state change at Thomas's moment: the instrument is extended (the rows drawn, the picture).
- Stagnation: no new discriminating evidence after two runs.
