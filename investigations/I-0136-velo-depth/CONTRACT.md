# I-0136 - La précision de profondeur des lignes Velo

Capability: C3

Origin: I-0134's final review (H1) and its Fable 5.2 read (I-0134 RESULT « Reviews », « Uncertainty »): the motion-vector rows
(engine pass 0x34, « Velo ») leave sharp-edged holes on flat triangles (up to 20 % of a character's outline at Firelink, frames
of `i132-r11j5-m40-taa-1920x1080` and `i132-r11n2-launcher-m40-nrd-1920x1080`), where the pixel falls back to the camera's
motion; opened by the coordinator on 2026-11-12 08:00 (Paris).

## Question

Can every Velo row's pixel that the scene shows keep its object motion, without letting a row hidden behind a nearer surface write
its motion there?

## Scope

Included:

- The Velo rows' depth test (13b's row state, the renderer's velocity family).
- The test at Firelink and one zone farther from the origin if a save reaches one.

Excluded:

- The game's Velo programmes themselves (their absolute projection kept).
- A camera-relative re-basing of the palettes and world rows for the Velo rows (a larger change across every producer; named as
  the alternative in RESULT).

## Inputs

- READ: the Velo programmes project absolute world positions through an absolute view-projection (12_scene_collect.inc's
  `VelocityMatrixBlock`); the G-buffer programmes subtract `SC_CameraPos` first; the rows were tested against the scene depth with
  a constant bias of -64 and a slope bias of -2 (13b), the depth 0 at near and 1 at far (velocity.h).
- INFERRED (the Fable read, a float32 emulation): clip z and w differ by 1e-5 to 1e-4 at Firelink's coordinates, a few hundred
  depth ulps against the bias of 64.

## Hypotheses

- H1: the holes are the depth test's failures (the rows' rounding beyond the bias).
- H2: a test with a tolerance scaled to the coordinates' rounding and to the view distance removes them without visible leaks.

## Expected evidence

- The velocity pictures (`world-pictures.flag`) at Firelink: the enclosed holes (`evidence/I-0134/holes.py`) near zero with the
  change, against `velo-depth-legacy.flag` (the former test) in the same build and save.

## Exit conditions

- Success: holes near zero, no fault, reviews (step, final xhigh, integrity), merged.
- Refuted: the holes stay: another cause, an RCA.
- Stagnation: two failed fixes, then an RCA.

## Safety and human interaction

Live runs only in the coordinator's slots, on a test save restored by hash.
