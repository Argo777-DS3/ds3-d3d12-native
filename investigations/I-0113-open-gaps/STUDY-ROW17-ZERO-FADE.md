# I-0113: STUDY-NOT-YET-NATIVE row 17, the `zero_fade` refusals (study thread, read-only)

Study thread S, 2026-11-06 (Paris), for the open-gaps pilot E; repository reading only, no run.

## Where row 17 stands

- **Its first cases are fixed.** Row 17 names m33's c1370, c1441, c2271 (FABLE-REVIEW lines 114-122: the native walk refused whole
  character families at the Road of Sacrifices' bonfire, the crosses o331204-o331208 too). The cause and its correction are in
  OPTIMISATION-REVIEW §8.2: the original weighs a category-0x01 entity of the main view (type 0x45) by its track weight and only
  the views of engine passes 1..0x38 by the geometry weight (`D04B90`, `D06E10`); the native main walk weighed every entity as pass
  5. Since then the main walk takes the 0x45 rule for category 0x01 (Native2216 against Reference2208: the crosses drawn, and the
  pooled characters c1441, c2271, c1370, c1360, c1446, c2150 emitted at the origin with track weight 0.083 as the original emits
  them), and I-0115 made the more opaque of the two views draw an entity of both categories
  (`native/frame/game_bootstrap/09_model_admission.inc:260-298`, `no-view-weight-fix.flag` the former rule).
- **What is still logged is not a list of missing models.** `native_zero_fade` (`09_model_admission.inc:302-311`) writes the first
  refusal of each model name only, so a model refused for a few frames while it loads, or while its LOD ramp starts, is listed
  the same as one never drawn (RESULT tranche 11, row 17: the player's parts, c1000, c1410, c1430, c2131, c2160, c3190, c3200,
  c5225-c5227, o000250, o391000, o379030, o404854...). The player's parts in that list fit a load-time refusal: the player is
  drawn in every run (INFERRED).

## What would answer it, cheaply

A per-name count of the refusals after the settle (for example, from the 30th second of a run), not the first refusal: a model
refused on every frame of a zone after its load, and present in the original's `reference_draw_entity` rows at the same place,
is a real gap; one refused only at its load is not. The rows exist (`popin_native_refused` with `popin-observe.flag`, I-0100 T0),
so this is a reader of existing journals, or a counter beside the first-refusal row; no new hook. Zones never toured natively
(m34, m35, m38, m39, m41, m51 and the DLC's) are where a new case would show.

## The count, from the journals already taken (added the same day)

A reader of the existing native and original journals (the study thread's Sonnet worker; script and outputs in
`evidence/I-0113/row17/`: `zf.py`, `table.tsv` 326 (run, model) lines, `runs.tsv`; no run, nothing launched). Numbers, then their
reading.

- **The rows** (OBSERVED): `popin_native_refused` (`13_model_frame_inputs.inc:604-611`) is a transition row, written when an
  (entity, model)'s refusal starts or changes, keyed by the presentation serial; it exists in 74 native runs (i100*, i114-st5*,
  i115*), 11 of them with a same-place original run. So a per-frame refused fraction cannot be read; the reader takes the share
  of the post-settle span (settle = 30 s after the run's first serial, at its own frame rate) each key spends refused, a refusal
  with no later row lasting to the run's end (which flags 315 of 326 lines: the flag is weak).
- **The models refused after the settle while the original draws them at that zone** (exact pairs, OBSERVED): c6080 at Ariandel
  (`i100-t6a-ariandel-n1` / `-o1`: 84 rows on 73 serials after the settle; the original drew it, off by the run's end); c2021 at the
  Undead Settlement (`i100-t6a`, `t6k`, `t8a` pairs: 8 to 10 rows); c2131 at Firelink and in the view test (`i115-fl-n60`, `-view-n60`:
  44 and 16 rows; the original on at the end); at Anor Londo's hall `m37_00_00_00_000081`, `002010`, `002020` (`i115-hall-n60`: 15, 2,
  1 rows; the original drew them, off by the end); 14 other m37 pieces refused there are not in the original's journal of that run
  (not reached or not drawn: « no » is not « not drawn »).
- **Their dates matter** (INFERRED): the i100 runs are of 2026-10-27/28, after OPTIMISATION-REVIEW §8.2's main-view rule (written by
  2026-10-19) but before I-0115's view-weight rule (merged 2026-11-02, `f0e8548c`); the i115 runs of 2026-11-02 are I-0115's own
  diagnosis runs, before its fix. c2131's case is I-0115's (the 0x45 view and the pass-5 view handing over at the LodParam's second
  distance, 19.8 m), fixed since. c6080 and c2021 were not run again after I-0115: they may be the same case (both categories,
  at their LOD distance) or another.

**For E:** one native pass with `popin-observe.flag` at the three places (Ariandel's c6080, the Settlement's c2021, Anor Londo's
hall pieces 000081/002010/002020), on today's `main`, settles it: if `popin_native_refused` still names them after the settle, they
are row 17's real gap; else row 17 can close.
