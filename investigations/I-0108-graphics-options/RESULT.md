# I-0108 « Les réglages graphiques » - result

## Conclusion

Complete (2026-10-30: stopped after tranche 2 step 1 by Thomas's change of course toward full RT/PT, D-0235). The inventory is done ([INVENTORY](INVENTORY.md)): of the 11 keyed settings, 7 follow the original at
every level natively; the depth of field, refused at the qualities 0 and 1 (« programmes ») until then, so that Crucifixion Woods
(quality 1) never had it natively, is drawn at every quality since tranche 1 (below; its look against the original not compared
bit for bit); the water reflection follows ReflectionQuality's `+0x6C`, `+0x6D` and `+0x6E` since tranche 2 step 1 (the target
divisor `+0x70` not yet: its reader is not found), reads closer to the original's than before (its remaining brightness
difference: the mirrored view's missing sun shadow, D-0189, now uncertain as a cause) and shows no character (INFERRED from one
crab; the player not yet checked);
WaterSurfaceQuality changes nothing natively (an effect family, the effects pilot's); the FXAA filters are not native.

## Evidence

- Static: MENUS-MAP §77.3 against the native readers (INVENTORY « Méthode »); `FUN_14024E790` (GhidraMCP) for `+0x74`.
- Live: `opt-lv-{ref,nat}-{max,high,medium,low,off}` (2026-10-29 23:33-23:51, ASUS, no measure, module `a2a627d8`, restorations
  ok), sheets in `opt-lv-compare/`; the DoF refusal also in 48 earlier native m33 runs.

## Uncertainty / unresolved questions

- One zone only; the menu's reachable levels and presets not read; the effects gates not seen; characters in the reflection seen
  on one crab.

## Next step (superseded by the open points below)

Tranche 2 step 2: why the mirrored walk admits no character (its row now counts the player's assembled parts apart); the `+0x70`
divisor once its reader is found; the reflection walk's cost at MAX (below); the mirrored sun shadow if Thomas takes it back.

## Tranche 1: the depth of field at every quality (2026-10-30)

- **Cause.** The DoF plan's plain-column downsample names `tech_MadT1_S<nx x ny>` (1..5 taps per axis); at the zone quality 1
  (Crucifixion Woods) it names `tech_MadT1_S6`, which had no pin, so `AcquireNativeGlarePrograms` refused the plan («
  programmes », row `native_yebis_glare_programmes` unpinned S6) and no DoF was drawn at m33 at any setting (48 earlier native m33
  runs, the `opt-lv-nat-*` runs).
- **Fix** (`f5c6fbd7`, `d75d4a80`): the six missing tap counts pinned (S6, S8, S10, S12, S16, S20, from
  `evidence/I-0067/queries/yebis-techniques.tsv`); the pins test checks every tap count the downsample can name.
- **Reviews.** Step review (fresh Opus): all four criteria hold (hashes equal the tsv's; the constant layout the downsample writes
  is the one these shaders read, checked on their RDEF: offsets at bytes 544 and 800, weights at 2464; no other refusal); its two
  low findings applied (the header's hand-added rows, the test of the whole family).
- **Witness** (`opt-dof-nat-max`, `opt-dof-nat-low`, 2026-10-30 00:44-00:48, ASUS, no measure, module `59B5E8A0`): `native_yebis_dof`
  on, quality 1 with 10 draws at Thomas's settings, quality 0 with 5 draws at LOW (refused before). The far forest is a little
  softer than before the fix (`opt-lv-compare/dof-fix.png`, `dof-zoom.png`); the difference at this view is small, as the
  original's blur is. No bit-for-bit check of the quality-1 draws: the module no longer has the draws capture the DoF test was
  built from.
- **Final review** (fresh Opus, xhigh, on `main...HEAD` after main merged in): the code meets every criterion (hashes, the whole
  family, the runtime's scan rules re-run on the executable, the constant layout, no barrier, lifetime or D3D11 change); not merged
  until the documentation agreed. Applied: the Conclusion, the index row and STATE no longer say the DoF is refused; INVENTORY says
  « drawn », not « follows » (no bit-for-bit check); the family check runs before the pins test's no-executable return; its header
  count; status. Not applied: one shared constant for the downsample's 5-per-axis cap between `yebis_dof.cpp` and the test (the cap
  is the original's, FUN_1416D6570; the test fails if a count goes missing either way).
- **Merged** `053b55d3` (2026-10-30 01:0x); the launcher rebuilt from main (tests passed, 01:14); start check `start-053b55d3`
  passed (01:20, 346 world frames, restored).

## Tranche 2, step 1: the reflection by ReflectionQuality (2026-10-30)

- **Static** (GhidraMCP, `FUN_1400FCE30`): for the view group 0x39 the gather mask takes bit 0x01 when `[[GXRS+0x5360]+0x6E]` is
  set (MAX), and the view's light container is walked only when `+0x6D` is set (MEDIUM and above). `FUN_1400FB0E0` sizes the view
  from the reflection object's `+0x20/+0x24`; the reader of the table's `+0x70` (4 at DISABLE and LOW, 2 above, INFERRED the target
  divisor) is still not found, so the native target stays half the output at every level.
- **Fix** (`2a2db36f`): `CollectNativeReflectionScene` reads `+0x6C..+0x6E` from the collect's option snapshot; `+0x6D` clear, no
  point lights in the reflection; `+0x6E` set, the walk's low byte 0x11. Row `native_reflection_view` names `lights` and the
  character models admitted.
- **Witness** (`opt-refl2-nat-max`, `opt-refl2-nat-low`, 2026-10-30 01:30-01:34, ASUS, no measure): at MAX the walk takes mask 17,
  500 objects and 212 models (156 and 69 before); at LOW `lights:false`. The lake's reflection loses most of the bright streaks at
  both levels and reads closer to the original's (`opt-lv-compare/refl2-lake.png`, original, native before, native after). **No
  character in the reflection at either level** (`characters: []`: the crab; the player's assembled parts were not listed by that row), while the original reflects
  the crab at LOW and MAX: a native gate drops the characters from the mirrored walk (open, step 2). Cost, read from the journals
  (no measurement run): at MAX the reflection prepares 842 draws against 136 and its collect section takes 2.24-2.30 ms against
  0.83-0.92 ms (`native_collect_sections` index 5, `opt-refl2-nat-max` against `opt-lv-nat-max`), more than the main walk (about
  2.0 ms): the original's own choice at MAX; an optimisation axis (the 146 records reused from the main view could reuse its
  prepared passes too). Not checked: FCE30 also needs `context+0x3EBE` before the light walk, which the native never tests (main
  view or reflection, before this step).
- **Step review** (fresh Opus): the five criteria hold (the old path kept without a snapshot and at MEDIUM/HIGH; the skipped point
  lights leave a fresh scene's lists empty, which the lighting already handles; the snapshot in bounds and equal to the live
  object; the row bounded and JSON-safe; mask 0x11 as FCE30's). Applied: the characters list takes `c`/`C` and a digit, at most 12.
- **Final review** (fresh Opus, xhigh): the code can merge (bounds, FCE30's mask and light gate, no resource, barrier or D3D11
  change; MAX now exercises the reflection's forward branch without fault). Applied: the row counts the player's assembled parts
  apart (`asm_parts`), as its name filter could not see them; the Conclusion, INVENTORY, the index and STATE rewritten; the cost
  figures above; a `static_assert` on the snapshot bound; the walk's comment and a dead initial value. Not applied: the water
  material's Fresnel words (`09_model_admission.inc`, FC_WaterFresnel) still read `+0x6C` live, not the snapshot (one frame of
  difference at an option change; predates this step).
- **Merged** `ebc7c70e` (2026-10-30 02:0x); the launcher rebuilt (tests passed, 02:30); start check `start-ebc7c70e` passed (02:36,
  353 world frames, restored).

## Open points left (Thomas's change of course, 2026-10-30 02:21; D-0235, 03:02)

- **Abandoned by D-0235** (the traced reflections, R6, replace the planar view): no character in the mirrored walk at any level (the crab; the player's assembled parts now counted by the row, not yet seen), and the `+0x70` target divisor.
- The reflection walk's cost at MAX (2.3 ms of collect against 0.9 ms) until R6 removes the planar view.
- WaterSurfaceQuality's effect family (the effects pilot's), the FXAA filters, the small unread bytes (`+0x45`, `+0x4B`, `+0x64`,
  `+0x76`, `+0x77`), the menu's reachable levels: INVENTORY « Ce qui n'est pas vérifié ».
