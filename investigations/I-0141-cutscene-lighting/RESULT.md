# Investigation result

ROADMAP workstream 7 step 7, by the R pilot: tranches 1 and 2 by Fable 5.2 (branch `claude/quizzical-chebyshev-e5cf9c`, the thread
of I-0138's tranches 6 and 5b continued: no fresh thread could open while Thomas was away), tranche 3 by Opus 5.5 (branch
`claude/nifty-lewin-0959f1`, a fresh thread, D-0281). Written as the work goes.

## Conclusion

Tranche 1: built in two variants, both witnessed without a fault; B merged (`ccde3618a`). **Thomas chose B** (put to him in the R
thread, 2026-11-14 18:50, with the pictures; his answer in the project chat, 19:35, cited to the pilot by the coordinator:
« B oui », then 19:36: « Ce n'est que temporaire le temps d'ajuster les sources lumineuses comme fait DS2LightningEngine »):
B is kept as a provisional rule, until the light sources are set by us (D-0279, on his « Oui » in the R thread, 19:57).

- **What was asked** (option 1, the coordinator's recommendation while Thomas was away): where a cutscene's ShadowParam names
  no light set, the traced sun takes light 0 of the light set of the scene's characters. Built: variant A.
- **What the witness showed**: option 1 applies, and cannot give the original's picture. In 39000020 the light 0 of the
  characters' set is off; the original lights Siegward by the lights 1 to 3 of the cutscene's own light set, which the native
  removes from every set since D-0262 (the sets' « fill lights », replaced by the traced bounce). Variant A turns Siegward
  from black to a flat grey (the bounce alone); the original shows him in a warm key light.
- **Variant B** (A, and the lights 1 to 3 of a cutscene's own light sets kept, unshadowed as the original draws them):
  Siegward lit by the same warm key light as the original's (brighter than it at 15 and 20 s on unpaired shots). An exception to D-0262 for one family of light sets (those of a cutscene's own file).
- **32000010**: at paired instants (the third witness, shots every 2 s) the scene's end differs: the original's last cuts
  are clear and bright (a blue sky, the temple white in the sun), the native's a grey veil then a picture as at dusk. The
  pilot's first claim of a « fog the original does not have » (to Thomas, 18:50) was made on unpaired shots and withdrawn
  (19:00); the paired shots show a difference all the same. Its larger part is the native's automatic exposure, 1.4 to
  1.75 stops under the zone's own exposure in those cuts: a run without it has the original's level (176.9 / 96.3 / 172.2
  of 255 at one paired moment), and Archdragon Peak in play comes from 60 % to 84 % of the original's. Put to Thomas
  (D-0264 is his). A pale veil and a clouded sky remain without it, where the original is crisp: a difference of its own,
  not yet read (« 32000010 at paired instants »).
- **After the step review** (its M1): the cut's light set stands for the missing one only where its light 0 is on. In
  39000020 it is off, so variant A alone changes nothing there, and Siegward is lit by variant B's lights or not at all.
- **Read for the next tranche**: the original cross-fades a zone's draw-parameter files over the time their binding gives
  (3 s at 32000010's start); the native switches at once (« Read for the next tranches »).
- **Tranche 2, first observation** (Thomas's remarks of 22:18 and 22:19: the bell, the zone's first state): 32000010 was
  compared in the wrong state of the zone. With the King alive again and the bell rung the game plays its ceremony scene
  and puts Archdragon Peak in its cloud-sea state; 32000010 then plays under a grey storm sky in both renderers, at the same
  level in its first four moments and brighter natively in its last cut (the run had the automatic exposure's bound of I-0138's tranche 5e; what the bound gives in this state was not
  separated). Three differences remain. Observed in the original: during the ceremony scene the cutscene act overrides
  the map's light and fog records by the cutscene file's. The pilot first wrote, and told Thomas at 23:06, that this
  never happens natively: false, the observers were installed under the original renderer only (withdrawn at 23:17 on the
  merge's integrity review). **Observed since with the observers in both modes: natively the act runs and writes the same
  overrides**, into the engine's banks, which the native renderer does not read: the gap is in `Banks` (« Tranche 2 »).
- **Tranche 3** (Opus 5.5, a fresh thread): those overrides given to `Banks`, so every reader of the light and fog records
  answers them while the act holds them (« Tranche 3 »). Witnessed in the ceremony scene 32000000: the 10 overrides made and
  withdrawn with the scene, the map's pieces of its close shots lit and fogged by the ceremony's records (a cool grey, as the
  original's, where they were sunlit before); the smoke's veil and the darker close shots left (the effects' fog is the E
  pilot's tranche 8, which reads the same channel).

- **Tranches 6 and 7** (Opus 5.5, a fresh thread, `claude/lucid-shtern-59efb5`): the ceremony's « world fog » was a flash's tail
  taken out of phase (« The ceremony's world fog re-read »); the effects now lit by the zone's and the act's records (« Tranche 6 »:
  the original's words, OBSERVED); a zone file's binding fades as the original's (« Tranche 7 »: the fog blocks the original's step for
  step, OBSERVED on tranche 7's record-blend path; the per-value path of tranche 24 ran through 32000010's fade without that observer);
  STUDY-UNREAD-SETTINGS' items 5 to 10 answered. Left: the smoke still brighter, the haze in play (« The witnesses of
  tranches 6 and 7 »).

- **Tranches 8 and 9** (Opus 5.5, `claude/lucid-shtern-59efb5` then `claude/priceless-bun-91a29e`): 45000030's cyan veil was the post
  chain's light shafts with a mask open over the whole frame, because the native's focus distance stayed at the DoF record's (0, then
  1 m) where the original takes the game camera's own (« Tranche 9 »): the camera's request now reaches the native,
  in cutscenes and in play; the veil is gone (`i141-t14a`). The shafts are also gated as the library keeps its records (« After the
  merge »). The later cuts' excess was D-0264's x2 local lights.

- **Tranche 10** (Opus 5.5, `claude/r-i138-t7` with I-0138's tranche 7): the local lights' A/B (`i141-ab-*`): x1 nearer the original in
  every paired place but one (within the noise); D-0293, x1 for good. The gate's brighter Firelink still was the x2 lights, not D-0280;
  D-0294 lowers the automatic exposure's target to 0.12, the original's darkest tenth (« Tranche 10 »).

- **Tranches 11 and 12** (Opus 5.5, `claude/r-i141-t11`, `claude/r-i141-t12`; tools and records, no fix yet): the ceremony's smoke
  sampled on the scene's own clock (D-0299): 102 smoke entities natively against 73 at the same moment. The library creates the same
  instances at the same times in both modes; the original ends its first instances' type-0x898 effect objects (state 8) from 14 s,
  the native's almost never leave state 3 and are deleted at the cut with their particles (« Tranche 12 », OBSERVED); why, not yet read.
  Moonlight Vortex's streak and green ground are in both films at the same moment.

- **Tranche 13** (Opus 5.5, `claude/r-i141-t13`): the ceremony smoke's end fixed, its 14.4 s burst open. The effects' FXR state machines end on « 1 <= external
  value 0 », and the original puts the stop table {0: 1.0} on a stopped SFX entry's handles each frame (E6B3C0, gated on the SFX
  manager's `+0x68`); the native manager had left that table out as a « colour table », so a stopped entry's effects ran until the
  cut. The native manager now makes it as E6C9D0 does (« Tranche 13c and 13d »): 74 smoke entities against 73 at the same moment,
  the same exits (VERIFIED); left, the twenty instances' first walk at 14.4 s, about 120 smoke entities offered once natively,
  none in the original (OPEN).

- **Tranche 14** (Opus 5.5, `claude/r-i141-t14`; tools and records, no fix): the 14.4 s burst explained. The original makes no
  effect draw for about 80 ms at the ceremony's start and at the 14.4 s cut while its updates go on, so the heads born there are
  first walked after a second update; the native draws every step and walks them at birth with the first update's non-finite box,
  which passes the frustum test in both modes (« Tranche 14 », OBSERVED; the reading INFERRED).
  The haze in play at Archdragon Peak's bonfire (« The haze in play at Archdragon Peak's bonfire »; read again by tranche 15,
  below): the veil over the castle is the effects' smoke (it goes without the native effect draw); under it a darker, flatter
  castle and player, which stay without the effects: each set's fog block and the light words the builders write are the same per
  key in both modes (OBSERVED), and where both draw a castle piece its rows take the same key block (« Tranche 17 »; OBSERVED for the records, INFERRED for
  the original's pixels).

- **Tranche 15** (Opus 5.5, `claude/r-i141-t15`; a measuring tool and records, no fix): the haze is not more smoke. In play at the
  bonfire the effects are the same in both modes (the entities each second by kind, their cull answers, the draws' kinds and
  numbers, each constant word's median within 1 % but one minor kind's two words; OBSERVED), and with the new
  `reference-no-ffx-draw.flag` the original without its effects loses what the native loses without its own (castle 19.0 against
  20.5 of mean luma, stairs 23.9 against 26.3, the whole 9.0 against 8.7; OBSERVED; that this is the effects' pictures alone,
  INFERRED). Under them the native's castle is darker (4.6) and of two thirds the original's contrast: the haze is the
  scene under the smoke. Its deferred pixels take their fog in the game's own `GXLightComposite` from the same per-key blocks
  (READ), so what differs is the light under it (D-0262's removal the likeliest, INFERRED, workstream 12's) or the key each pixel
  takes, not read. Firelink's cutscene 40000040 binds its kind-11 file and takes its sun as the original's (OBSERVED, « Tranche 15 »);
  I-0138's two kind-11 rules (the MapOffset turn, the Snow prefix) not exercised there.

- **Tranche 18** (Opus 5.5, `claude/r-i141-t18`; two runs, a Reference-only witness, no fix): of the 143 models the native keeps
  drawing at m32's bonfire, the original with `reference-no-occlusion.flag` (which forces its occlusion nodes and cells visible,
  READ) draws 137 (OBSERVED: 340 models a window instead of 203, every one of its own and none the native does not draw); that its occlusion
  system, never run natively, alone makes the difference is INFERRED. c3140 is D-0222's launch LOD scale (OBSERVED). A cost (the G-buffer's 346 models against 203, not measured); the native occlusion cull is ROADMAP 15's
  (D-0304) (« Tranche 18 »).
- **Tranche 19** (Opus 5.5, `claude/r-i141-t19`; one run, no fix): the five m32 pieces left are the native's 1.5 m cull margin
  (OBSERVED: none drawn with `no-cull-margin.flag`); tranche 18's 150 m lead withdrawn. The full control of 2026-11-21 on this branch:
  the gate passed, no fault (« The full control of 2026-11-21 »).
- **Tranche 22** (Opus 5.5, `claude/r-i141-t22`; a culler row, two runs, records): every map piece the 1.5 m margin keeps at m32's
  bonfire, the five included, lies wholly outside one side plane of the view's own culler, by 0.03 to 1.5 m (OBSERVED, `i141-t22b`:
  one run, the whole-filter path of light-observe.flag, D-0222's opt-out; no pixel INFERRED); the 8 models without a G-buffer row are drawn natively outside it, the original prepares them in its arrival window, after it not
  shown (OBSERVED in the journals); Yorshka's +1 not pursued (« Tranche 22 »).
- **Tranche 23** (Opus 5.5, `claude/r-i141-t23b`): the post, shadow and IBL settings fade value by value during an area fade, as
  the original's value classes (READ: floats and vectors lerped, integers truncated, colours per byte, bools and 84 definitions at
  once, the angle pairs through AngleLerp; unit-tested, no fade compared in a run, the IBL fade without an observed consumer); 32000010's last cut read once on the scene's clock (`i141-t23a`, 8 holds paired): no
  veil at holds 5 and 6, hold 4 brighter frame-wide natively with the blade lighting the King, not settled (« Tranche 23 »).
- **Tranche 24** (Opus 5.5, `claude/r-i141-t24`): Sun and Fog fade value by value (unit-tested); the original still draws seven of
  m32's 8 models in a window like tranche 18's (OBSERVED, a partial window, one run a mode); 32000010 on three pairs, the title bar
  left out of the luma: the native brighter at holds 0, 1 and 4 in every pair, beyond the repeat spread at hold 0 alone (« Tranche 24 »).
- **Tranche 25** (Opus 5.5, `claude/r-i141-t25`): I-0138's D-0299 pairs measured again below the title bar; 32000010's native
  shares (fill lights, glare, the GI's sky) each removed once at the same holds: none attributed, the clear excess at cut 1 on the lit foreground
  (« Tranche 25 »); `no-cutscene-fill-lights.flag`, a test tool.
- **Tranche 26** (Opus 5.5, `claude/r-i141-t26`): the native's shares switched live at one held moment (`witness-variant.txt`,
  `-CutsceneHoldVariants`, `Compare-I141HoldVariants.py`), two runs a state: the GI's and the cache's sky and ambient (not the probes' own
  misses) carry at most 2 at the clear state's steady holds; the fill lights about -20 at every steady hold of cuts 0 to 2 whatever the
  excess, -8.4 at cut 5; the glare -2.4 to -4.1 where the excess is small, -12.9 at cut 1
  and -9.4 at cut 5's frame 140.2 where it is +15.9 and +17.7; no share attributed until the original's own shares are measured; the
  storm's hold 0 moves by 11.2 within one hold, more than its +8.1 excess (« Tranche 26 »).
- **Tranche 27** (Opus 5.5, `claude/r-i141-t27`): the original's own shares at the same holds (`reference-share-variant.flag`) and
  the world's timeline frozen in a hold (`hold-world-freeze.flag`, base ranges 0.3 at most): the fill lights and glare take as much
  in both modes at cut 1 (about 4 more natively at cuts 0 and 2, 4 more in the original at cuts 4 and 5); with them and the diffuse
  and specular ambient out the frame means agree to -4.4 to +6.6; the native's diffuse-ambient share -8.9 to -10.7 where the
  original's is 1 or less, its specular 1.8 to 4.6 times the original's at cuts 1, 2, 4 and 5 (less at cut 0); the storm's cut 3 and
  cut 5's frame 14.56 bright outside the four shares (« Tranche 27 »).
- **Tranche 28** (Opus 5.5, `claude/r-i141-t28`): the native's diffuse-GI and specular excess at 32000010 in the clear state is the
  sun's light at the traced hits at cuts 1 and 5, about half each at cut 1 (without it cut 1 122.0 against the original's 121.8, cut 5's frame 140.2
  151.0 against 147.3); the original's characters get only a small bluish instance SH from their animation block (mean w 0.014 to
  0.062, its writer's source OPEN) and a sun mask of 0 that the native's hits ignore (OPEN); too bright for Thomas (« Oui ils sont trop
  clairs », 18:01; scaling it down a lighting cheat, D-0262); the storm's cut 1 split (the fill lights -16.7 natively, -14.4 in the
  original), single runs settling no storm excess (« Tranche 28 »).

## Tranche 1: the traced sun and the lights of a cutscene whose shadow record names no light set (2026-11-14)

**The request.** The E pilot's tranche 7 (I-0140, merged `d1fbc6f51`) made the native's traced sun the original's shadow sun: the
view's ShadowParam record (its keys A and B, their blend) and its Shadow Light Id, the light set whose light 0 casts. Where
that id is -1 the original's F90F0 takes the record's Angle: (0, 0, 1), a horizontal sun, in every cut of 32000010 (the
King of the Storm's fight start, Archdragon Peak) and of 39000020 (the Profaned Capital's boss intro, with Siegward). Put to
Thomas by the E pilot; he was asleep, and the coordinator applied the recommendation under his rule (his choice B of that
morning, « in a cutscene the traced sun takes the cutscene's light set », prolonged): where a cutscene's ShadowParam names
no light set, the traced sun takes light 0 of the light set of the scene's characters. To be confirmed by Thomas.

**What the journals show** (OBSERVED, read by the pilot in `i140-e16-c32000010-nat`, `i140-e17-c39000020-nat` and
`i140-e16-c39000020-orig`):

- The view's ShadowParam key changes at each cut: `{232000010, 10}`, then 30, 40, 50, 60, 70; `{239000020, 10}`, then 40.
  Each record's own keys read `{the area, -1}`: no light set.
- The cutscene's own light sets carried by the drawn models have **the same ids**: in 32000010 the character models (`c5…`)
  carry `{232000010, 10}`, 30, 40, 50, 60 and 70, a character built of parts (face, hair) `{232000010, 60}`; in 39000020 the
  parts of a character in the Catarina set (`…_M_7400`: Siegward), two objects and `c5260` carry `{239000020, 10}`, `c5260`
  also `{239000020, 40}`. The map's pieces keep the map's sets (`{132000000, …}`, `{139000000, …}`). So « the light set of the
  scene's characters » is the light set at the view's own ShadowParam key. INFERRED for the cutscenes not run.
- **In 39000020 that set's light 0 is off**, in the original as natively: the original's cbLight blocks of the two sets
  (`reference_sun_block`, 72 floats a set) hold light 0 at the default direction (0.707, 0.707, 0) with a zero colour, light
  1 at (0.786, -0.485, 0.383) with the colour (1.399, 0.732, 0.5) (set 10; (0.513, 0.268, 0.183) for set 40), a cool light 2
  (0.66, 0.944, 1.026) and a weak light 3 whose directions follow the camera. The native's records are the same
  (`native_light_set`: the engine's own records, the blocks made by the engine's own 1496D0 from the native view).
- **The native removes the lights 1 to 3 of every set on a traced frame** (D-0262, Thomas, 2026-11-09: the sets' fill lights,
  rows 5 to 7 and 9 to 11 of each 288-byte set zeroed for the pixel's passes and the rays' hits, `scene_pass_lighting.cpp`;
  the forward rows' own sets too). For a map's set those are fills the traced bounce replaces; for a cutscene's own set
  they are the cut's authored lights.

**What is built.**

- Variant A, option 1 (`eb7e417a5`): `ds3light::CutsceneShadowLightSet` (`native/world/light_producers.h`): for a record whose
  Shadow Light Id is -1 under a view key that names a cutscene file's area (2AABBVVVV) and an id, the light set that stands
  for the missing one is the view key's; `ShadowSunDirection` takes the view's keys and applies it to the record's A and B
  before F90F0's choice (a cut's set the bank lacks leaves the record's Angle); the journal's `sun_source` says
  `cut_light_set` or `cut_blend`. In play nothing changes (a map's key is not a cutscene's; the title's `{-1, -1}` keeps
  the Angle).
- Variant B, A and: `ds3light::CutsceneLightSet` (both keys of a draw's light key block name a cutscene file's area); the
  collect marks such sets (`LightBlocks::authored`, `NativeLightingInput::authored_sets`) and the renderer keeps their rows
  5 to 7 and 9 to 11 on a traced frame (the deferred block, which the rays' hits read too; the forward rows' own sets the
  same). They stay unshadowed, as the original draws them (INFERRED, not read again here: the original's cascades are the
  shadow sun's alone, so only what the programme takes for light 0 can be shadowed).
- After the step review (below): the cut's set stands for a record's missing one only where the bank holds it with its
  light 0 on (`ds3light::SunLightOn`: a positive strength and a colour); a record's key is matched to either view key; a
  blend of a map's set with a cut's keeps D-0262's removal.

**Tests.** `glow_bank_test` (data-free part): `CutsceneShadowLightSet`'s cases (a record without a light
set under a cutscene's key, in place, default's record under a cutscene's key, a record that names its set, a map's key, the
title's key, a key without an id, the area range's two ends), `CutsceneArea`, `CutsceneLightSet` (changed with the step
review's L2: a block with one map key no longer counts, the test's expectation changed with the rule) and `SunLightOn`.
`ShadowSunDirection` itself and the renderer's kept rows have no unit test. Builds: `evidence/I-0141/artifacts/build-1.log`
(variant A), `build-2.log` (variant B), `build-3.log` (after the step review): exit 0, 179 PASS lines each (`grep -c PASS`), WARP and the card.

**The witnesses** (the OLED in a 2560x1440 window, offline, no measure, `Run-I139Cutscenes` natively with the E pilot's flags
`test-no-ai,world-pictures,gbuffer-pictures,pictures-from-cutscene,light-observe`, Eta's save restored by hash after each
run; `evidence/I-0141/artifacts/tranche-1/`): `i141-t1a-*` (17:56 to 18:06, variant A), `i141-t1b-*` (18:38 to 18:48, variant
B): four runs, exit 0, no fault. OBSERVED:

- Variant A's rows: `sun_source` `cut_blend` at every cut of both cutscenes; the sun (0.707, 0.707, 0) in 39000020 (its sets'
  light 0, off: the default direction), (-0.824, 0.358, 0.438) then (-0.637, 0.358, -0.683) in 32000010; in play the same
  keys and suns as `i140-e16` (`blend`, the map's).
- The pictures against the original's (`i140-e16-*-orig`) and the native's before (`i140-e17-c39000020-nat`,
  `i140-e16-c32000010-nat`), mean luma of each shot (ffmpeg's signalstats, of 255; the whole picture, and its centre, the
  middle half in both directions; `Make-I141Triptychs.ps1`, `triptychs/` for A and `triptychs-b/` for B, their
  `*-levels.txt`). The shots are taken by the clock from the scene's start and the native's run a little later than the
  original's, so a shot is not the same instant in each run: the numbers go beside the pictures, not in their place.

  Every shot of the two scenes' cuts (5 to 40 s; at 2 s the native's picture is black, the scene not begun, and from 50 s
  the runs are back in play: those rows are in the files), the centre's level (the whole picture's is in the files too; the
  integrity review's Medium: the first table here gave six rows chosen by the pilot, without A's darker shots). A and B are `build-1` and `build-2`, both
  **before the step review's M1**: in 39000020 they cast the sun along the placeholder direction the merged rule no longer
  takes; no levels were taken on `build-3` (its witness is the timeline sheets, read by eye).

  | Cutscene, shot | Original | Before | A | B |
  | --- | --- | --- | --- | --- |
  | 39000020, 5 s | 37.2 | 36.6 | 40.0 | 45.7 |
  | 39000020, 10 s | 56.5 | 42.2 | 45.3 | 43.4 |
  | 39000020, 15 s | 40.9 | 31.2 | 35.0 | 53.1 |
  | 39000020, 20 s | 47.3 | 36.3 | 42.8 | 55.8 |
  | 39000020, 25 s | 51.9 | 84.6 | 81.8 | 85.3 |
  | 39000020, 30 s | 44.8 | 67.7 | 66.9 | 70.7 |
  | 39000020, 40 s | 40.8 | 61.3 | 62.2 | 66.4 |
  | 32000010, 5 s | 100.2 | 54.7 | 58.8 | 71.8 |
  | 32000010, 10 s | 91.8 | 60.2 | 46.1 | 68.7 |
  | 32000010, 15 s | 86.0 | 61.4 | 31.7 | 67.9 |
  | 32000010, 20 s | 102.3 | 61.4 | 61.5 | 73.3 |
  | 32000010, 25 s | 62.0 | 31.4 | 32.5 | 55.9 |
  | 32000010, 30 s | 159.0 | 81.8 | 70.7 | 94.0 |
  | 32000010, 40 s | 178.8 | 88.8 | 109.4 | 112.6 |

  What the rows say and do not: in 39000020 from 25 s on (Yhorm's cuts) the native was already far brighter than the
  original before this tranche, and stays so; in 32000010 A is darker than before at 10, 15 and 30 s, B brighter than before at every shot from
  5 to 40 s and still under the original's; back in play (50 to 90 s) both are at or under the levels before (the whole
  picture at 50 s: A 41.1, B 55.3, against 59.6; B's centre at 75 and 90 s 64.5 and 58.9 against 66.8 and 59.8); a row
  compares shots a few seconds apart in their scenes (the third witness pairs them), so no row alone is a verdict.

- By eye, 39000020 at 10 s (`c39000020-10.png` of both folders): the original's Siegward in a warm, bright key light; before,
  black; A, a flat neutral grey (no light of his set reaches him: what lights him is the bounce of the map's pieces, which
  the horizontal sun left in shadow before); B, the warm key light, close to the original's. In B he is brighter than the
  original at 15 and 20 s (centre 53 and 56 against 41 and 47; not the same instants).
- 32000010 (`c32000010-40.png`): the two runs' shots at 40 s are different moments of the scene (« The first claim of a
  fog veil, withdrawn »); what differs at the same moments is in « 32000010 at paired instants ». In B the player's body is
  brighter than in the native's shot before.
- Against the contract's « Expected evidence » (the mean level of the characters' regions): the levels are the whole
  picture's and its centre half's, not a crop on each character (the final review's L6); the characters are judged on the
  pictures.

**The step review** (a fresh `step-reviewer` on `fdf408335..43ea3bfa2`; no High, one Medium, four Low; the criteria on play
unchanged, the `authored` bytes, the forward and deferred rule, stale state and the enum's table held):

- M1, variant A took the direction of a light 0 that is off (39000020: the default (0.707, 0.707, 0) at a zero strength)
  and shadowed the map's pieces along it, where the original shadows them along the record's Angle: fixed, the cut's set
  stands only with its light 0 on (`SunLightOn`); with it off the record's Angle stays, as before this tranche. Variant A's
  rise of the levels in 39000020 (the table's A column) came, INFERRED, from that placeholder direction, not from a light of
  Siegward's (no run of A alone was made on `build-3`).
- L1, a record's keys matched to the wrong view key on `BlendShadowRecord`'s whole-record branches (a fade between a map's
  key and a cut's): fixed, either view key answers.
- L2, variant B kept the lights 1 to 3 of a block that blends a map's set with a cut's: fixed, both keys must be a cut's.
- L3, a comment stating for the family what 39000020 alone showed: rewritten with its statuses. L4, « shadows light 0 alone »
  without a status: marked INFERRED above.

**The third witness** (`i141-t1c-*`, 19:25 to 19:44, the OLED, `build-3`, after the step review; shots every 2 s from 2 to
70 s, then 75, 80, 90; natively then in the original; `evidence/I-0141/artifacts/tranche-1/runs-t1c.log`, the sheets of
`Make-I141Timeline.ps1` in `timelines/`): four runs, exit 0, no fault, the save restored. A build (I-0138's tranche 5c) ran
on the PC during the two native runs; nothing is measured. OBSERVED:

- `sun_source` in 39000020: `angle` at every cut (its sets' light 0 is off: the record's Angle stays, M1), `blend` in
  play; in 32000010 `cut_blend` at the cuts, `angle` on the frame between two cuts where the view's key is unset.
- 39000020, the same cuts 2 s later natively than in the original: Siegward in his warm light in both; natively the hall
  brighter and warmer (the fires' traced light), and Yhorm on his throne brighter, with more silver in his armour, than the
  original's dark blue.
- 32000010: below.

### 32000010 at paired instants

OBSERVED on the timeline sheets (`timelines/i141-t1c-c32000010-{orig,nat}-{a,b}.png`): the native's scene runs about 2 s
behind the original's and has the same cuts in the same order (the wyvern's foot, the King on its back, the low angle, the
burst and the dark column on the temple, the player before the mountains, the player from behind before the arena, the
King in the mist, the King before the temple, the end). At the same cuts:

| Cut (original's second / native's) | Original | Native |
| --- | --- | --- |
| The player from behind, the arena (38 to 40 / 40 to 42) | clear, the temple white, hard shadows | a grey veil over the arena, thicker at each shot |
| The King before the temple (44 to 46 / 46 to 48) | a bright blue sky, the temple white in the sun | a dark blue sky, the temple dim, as at dusk |
| Back in play at the bonfire (50 on / 52 on) | a clear day | darker (mean luma about 62 against 103 of 255, the earlier runs' levels) |

What the journals say of it (OBSERVED, `i141-t1c-c32000010-nat`): the automatic exposure's rows (`native_auto_exposure`,
D-0264: half strength toward one target, 0.2) give a factor of 2^-1.53 to 2^-1.76 in the cuts of the burst, the player and
the arena (the view's keys 50 and 60), 2^-1.38 to 2^-1.40 in the last cut (the King before the temple, key 70), 2^-0.2 to
2^-1.2 in the cuts before, 2^-0.93 in play at the bonfire, against the zone's own exposure (`area_exposure` 2 in the
scene, 2.4 in play). The zone's file of that state (`m32_00_0100`, bound at the scene's start in both runs) gives the sun a
strength of 30 and a distance fog of colour 1: a picture meant very bright, where a fog of that colour does not show against
the white stone.

**The fourth witness: the same scene without the automatic exposure** (`i141-t1d-c32000010-nat`, 20:26 to 20:31, the OLED,
`build-3` with `no-auto-exposure`: the zone's own exposure; exit 0, no fault, the save restored; `runs-t1d.log`, the
sheet `exposure/exposure-32000010.png`: the original, the native, the native without it, at three paired moments). OBSERVED,
the whole picture's mean luma (ffmpeg's signalstats, of 255), each native shot 2 s after the original's:

| Moment (original's second / native's) | Original | Native (`i141-t1c`) | Native without the automatic exposure |
| --- | --- | --- | --- |
| The player from behind, the arena (40 / 42) | 176.9 | 96.3 | 172.2 |
| The King before the temple (46 / 48) | 151.6 | 95.1 | 154.7 |
| In play at the bonfire (60 / 62) | 103.3 | 62.2 | 86.7 |

- **The dusk is the automatic exposure's**: without it the scene's end has the original's level (within 3 %) and its bright
  sky, and the bonfire in play comes from 60 % of the original's level to 84 %. D-0264's rule (half strength toward one
  target for the whole game) darkens by one stop to a stop and three quarters the zones the game makes bright on purpose.
  Put to Thomas (the R thread, 20:33, with the sheet): the pilot recommends that the automatic exposure keep brightening
  the dark places and no longer darken under the zone's own exposure; his choice.
- **A veil remains without it**: at the same two moments the native still shows a pale haze over the arena and a clouded
  sky behind the King, where the original is crisp under a clear sky. So the first claim, withdrawn for want of paired shots,
  holds at paired instants for these cuts, as a fog or sky difference of its own. Its cause is not read; first suspect
  (INFERRED): the cutscene act that replaces the in-game light and fog records by the scene's own (« Read for the next
  tranches », item 2), which the native does not follow.
- In play the player stays darker than the original's with or without it (the traced lighting without the sets' fills,
  D-0262: known).

### The first claim of a fog veil, withdrawn (unpaired shots)

The pilot told Thomas (18:50) that 32000010 natively « lies under a fog the original does not have », from the shots at 40 s
(`c32000010-40.png`). Read again with every shot of both runs side by side (OBSERVED, `i140-e16-c32000010-orig` and
`i141-t1b-c32000010-nat`, the shots at 5 to 50 s): the two runs' shots are not the same moments of the scene, and not by one
constant delay. The King on the wyvern's back: the original's shots at 15 and 20 s, the native's at 20 s; the low-angle cut:
25 s in both; then the original shows the player before the mountains (30 s), the temple from the arena, clear, the wyvern's
shadow crossing it (40 s), and is back in play at 50 s; the native shows the temple with the storm's dark column (30 s), the
arena under the storm's veil (40 s) and the King at dusk (50 s), still in the scene. So the shot at 40 s compares two
different moments, and the veil is drawn with the storm's own effects at a moment the original's shots do not hold. Whether
the original shows that moment the same way, and why the two runs' scenes differ in length, is NOT OBSERVED: it needs
denser shots of both runs. Corrected to Thomas at 19:00.

The keys and the files are the same in both runs (OBSERVED): every drawn model's light and fog keys during the scene
(`native_model_light_key` against `reference_model_light_key`: the map's pieces `{132000000, 0}` for both), the files bound
(`m32_00_0100` on bank 6 at the scene's start, beside `m32_00_0000` on bank 5; `s32_00_0010` on bank 13), and the map set's
light 0 at the end of the change (the original's `reference_sun_block` of its set-0 record: (-0.146, 0.643, 0.752) at a
strength of 30, the native's `native_light_set` `{132000000, 0}` the same).

### Read for the next tranches (2026-11-14)

1. **A zone's draw-parameter files cross-fade in the original; the native switches at once.** OBSERVED: in the original's
   run the record of the map's set 0 goes from `m32_00_0000`'s light 0 ((0.705, 0.643, 0.299), strength 1.3) to
   `m32_00_0100`'s ((-0.146, 0.643, 0.752), strength 30) through six sampled steps (strengths 8.5, 11.9, 16.8, 20.2, 23.5,
   30; `reference_sun_block` rows of the same `set` pointer); natively the two states only. Read (VERIFIED in the decompiled
   code, GhidraMCP): the area controller `AreaGparamController_Apply_140CE6350` passes its transition time (`+0xC0`, or
   `+0x10C` when not negative; 0 at a first binding) to both the unbinding (`GparamUnapply_140D26DD0`) and the binding
   (`D26A70`, the « weight » of `D275F0` and `49D00`: 3.0 in both runs' rows for `m32_00_0100`, 0 for a first file);
   `FUN_140096560` stores it per group slot, area and bank in the layer table of `[GXRS+0x4788]` (17 entries of 0x20 bytes
   an area: the handle, a blending flag, the elapsed time, the duration) and marks the area's records dirty;
   `FUN_140096B70` (the unbinding) gives the duration to the next live layer beneath. The step that advances the elapsed
   time and the rebuild of the records: read by a `ghidra-research` subagent, filed as
   [STUDY-LAYER-FADE](STUDY-LAYER-FADE.md) (a worker's reading, to re-read before a design relies on it: a weight linear in
   time, the values blended one by one from their state at the binding toward the top layer's). The layers and their slots
   are in I-0067 MENUS-MAP « The layers in use », whose merge rule is INFERRED there; these rows are its first runtime
   evidence of a fade. Natively `RecordNativeGparamBinding` keeps the weight's bits in the binding and
   `Banks::SetAreaLayers` ignores them (the latest layer answers whole). Visible wherever a zone changes its file in play:
   a pop of the sun, the fog and the post settings (a boss's phase, 32000010's storm), where the original fades in 3 s.
2. **The cutscene's own override of the in-game records** (read, VERIFIED in the decompiled code; observed since in the
   original, in Archdragon Peak's ceremony scene 32000000: « Tranche 2 »). The Remo act `IngameDrawParam` (slot 9 `FUN_140CC8180`) writes, at the key {the area of
   `FUN_140CDDE40`, the act's id `+0x30A`} of five banks (the light sets `[GXRS+0x41E0]`, the fog `+0x41E8`,
   `+0x41F0`, `+0x42A8`, `+0x4600`), on the layer of kind 9 (`MapOverride_forRemo`, bank 11), a record made of two source
   keys and a weight (`FUN_140CC8530`: each an area chosen by a selector and an id of the act's track; `FUN_1400380A0` for
   the fog: the blend `14E5D0` of the two records when the weight is positive, else the first), and removes it
   (`FUN_1400387D0` and its siblings) when the track names none. Natively the engine's banks hold no file (the records are
   `Banks`'), so these writes would land in records nothing reads, and a cutscene that uses the act would keep the map's own
   light and fog natively (INFERRED; the native side's observation: « Tranche 2 »).
3. The E pilot's notes (relayed 2026-11-14 19:00): in 51000000 (the Dreg Heap) the angels are admitted and the picture
   nearly black at 30 s is the lighting's or the fog's; with 51000010. Not looked at yet.

**Reviews at the merge** (the tree with `main` merged in, `51f5ac4d3`; this tranche with I-0138's tranche 5c).

- The optimisation pass (read): a cut's set the sun takes is built once and kept as the blend's `ra` or `rb`; a view's
  second key is not tried when it is the first again (the fixes' step review's Low); the `authored` bytes are one pass over
  the frame's keys.
- **The final review** (a fresh `final-reviewer`): no High, no Medium; the exit criteria held (play unchanged, checked in
  `i141-t1c`'s journals; no out-of-range read of `authored_sets`; one rule for the forward and the deferred sets; the log
  not edited before Thomas's word). Eleven Low. Fixed: **L1** the two view keys are each tried, the first whose set the bank
  holds with its light 0 on answers (a fade between two cuts whose first set is off took none); **L3** three comments
  (Thomas's choice, the light 0's condition); **L4** to **L7** this RESULT's sentences (the withdrawn claim no longer under
  OBSERVED, the Conclusion's « as in the original », the levels' regions, the artifacts' list) and ROADMAP, STATE, the index;
  **L11** the timeline script (one column, a picture of another shape). Left, with the reason: **L2** no unit test of
  `ShadowSunDirection`'s new branches (`Banks::Sun` and the blend read the executable's constants through `Rdata`, which
  the unit test has not: the journals' `sun_source` rows of the three witnesses stand for them); **L8** to **L10** are
  tranche 5c's (I-0138 RESULT).
- **The integrity review** (a fresh `integrity-reviewer`): no High; the judge untouched (the test's changed expectation
  and the removed test programme both stated), no scene's special case (the area range is the cutscene files' family),
  each run on the build named, the numbers checked against the evidence. One Medium, fixed: the levels' table was a choice
  of six rows without A's darker shots (now every shot of both scenes, and what the rows do not say). Low, fixed: B's
  column is from before M1 and no levels were taken on `build-3`; the exposure's range (2^-1.38 in the last cut); A's rise
  in 39000020 marked INFERRED.
- **The fixes' step review** (a fresh `step-reviewer` on `51f5ac4d3..a77a5ec90`): L1's four criteria held (no change in
  play nor where both view keys name one set; the records left for the code after; the key written on success alone; the
  order). One Medium, fixed: the levels' table said « every shot » and stopped at 40 s, and its prose left out B's darker
  shots in play (the sentences above now say which shots and give both). Low, fixed: the same set built twice a frame where
  both view keys name it and its light 0 is off (the second try skipped then); D-0279's first point named a « choice B of
  D-0262 » the log does not hold (now I-0140 RESULT « Thomas's choice for the rest »); its second point said plainly to
  be the coordinator's wording. The fixes' build: `build-4` (`a77a5ec90`: exit and counts in « Evidence »); the second
  try's skip came after it and is first built with the launcher's module at the merge.

**The merge** (2026-11-14): `ccde3618a` into `main` at 20:24 (the branch at `b10c755bb`, with I-0138's tranche 5c;
`Test-Project -MergeGate` passed before, `Test-Project` after); the launcher's module rebuilt in the main checkout
(`evidence/I-0141/artifacts/build-launcher-ccde3618a.log`: exit 0, 179 PASS lines); the start check `start-ccde3618a`
passed on the OLED in a 2560x1440 window (20:48: at the world in 3.17 s, closed, the save restored).

## Tranche 2: the zone's true state and the cutscene act's overrides (2026-11-14, an observation)

**The request** (Thomas, the R thread, 22:17 to 22:19: « ça a été testé dans les mêmes conditions avec l'original ? »,
« faut activer le flag de la bell », « Vu qu'il a été déjà tué, il faut peut être revenir à l'état inital de la zone puis
réactiver la bell »). The runs of tranche 1 played 32000010 on Eta's save as it is: the Nameless King killed, so the
zone's ceremony event ends at once (m32 event 13205441, its first instruction: end if 13200850) and Archdragon Peak stays
in its clear state, in both renderers.

**The protocol** (one run a renderer, the OLED in a 2560x1440 window, offline, no measure, `build-t5e`: the branch at
`a2ffd0942`, with tranche 5e's exposure bound; the pilot's wrapper kept as
`evidence/I-0141/artifacts/tranche-2/run-t2b.ps1`, its log `runs-t2b.log`): the boss's defeat flag 13200850 cleared by the test
tools (`test-revive.txt`), the flags 13200446, 13205852, 13205441 and 13200445 cleared (`Run-AlphaCast -SetFlag`), Eta's
travel to the Archdragon Peak bonfire, a shot a second for 80 s, then 32000010 by the cutscene tool with a shot every 2 s.
Both runs with the same test flags: `test-no-ai`, `world-pictures`, `gbuffer-pictures`, `pictures-from-cutscene`,
`light-observe` (the bank observers; natively it also makes the scene collect walk on one thread with its diagnostics) and
`test-tools` (the revive). Runs `i141-t2b-c32000010-nat` (22:49 to 22:56) and `i141-t2b-c32000010-orig`
(22:56 to 23:03): exit 0, no fault, the save restored.

**What happened** (OBSERVED, both renderers alike): the four flags and the revive applied (`witness_flag`,
`test_revive`); at the arrival the game played the ceremony scene 32000000 by itself and the zone came back in its
cloud-sea state (the title « Archdragon Peak », a grey haze at the bonfire). The bell's flag was not set by the pilot: the
bell's own event 13205440 sets 13200445 again at the load where the lever's flag 13200440 is on, as it must be in Eta's
save (INFERRED from the event and from what the run showed; the save's flag not read; the
event dump `evidence/I-0139/artifacts/static/bosses/event_m32_00_00_00.emevd.txt` lines 634 to 654; the ceremony event
lines 656 to 684). The runner's new `-SetFlagAfterTravel "13200445 1"` was written during that scene and not taken (the
module does not take the file while a cutscene plays): without effect, the flag being on already.

**32000010 in that state** (OBSERVED on the pilot's sheets `timeline-t2b-nat.png`, `timeline-t2b-orig.png` and the pairs
`pairs-t2b-orig-left-native-right.png`, shown to Thomas at 23:10; the native about 4 s behind the original in the scene;
mean luma of the whole shot, ffmpeg's signalstats YAVG of 255):

| Moment | Original (shot: YAVG) | Native (shot: YAVG) |
|---|---|---|
| the drake's head | 4 to 6: 86, 92 | 8 to 10: 78, 88 |
| the King on the drake, from the front | 14 to 18: 83, 84, 92 | 20 to 24: 88, 92, 88 |
| the King from behind, his sword raised | 22 to 24: 89, 90 | 26 to 30: 82, 85, 86 |
| the player on guard | 30 to 36: 126, 124, 117, 137 | 36 to 40: 132, 133, 128 |
| the King standing with his spear (the last cut) | 44 to 46: 92, 94 | 50 to 52: 130, 125 |
| in play at the bonfire, after the scene | 50 to 70: 84 to 87 | 56 to 70: 79 to 83 |

- The same grey storm sky over the cloud sea in both, at the same level in the first four moments. The automatic exposure
  stays at the factor 1 through the whole native run (`native_auto_exposure`: 90 rows, `log2_factor` 0.0000 to 0.0028,
  none negative; Firelink included, mean 0.27 against the target 0.20): tranche 5e's bound.
- **The last cut**: a pale veil over the King natively, where the original is crisp (dark clouds, the towers behind him);
  the two shots are not the same instant of the cut.
- **In play after the scene**: the native's haze is denser (the building behind the bonfire nearly gone, plain in the
  original) and the player darker, at close mean levels.
- The orange lightning of the original's shots at 26 and 32 s is on no native shot; the instants differ, so not counted a
  defect until paired.
- So tranche 1's « blue sky and white temple against a grey veil » compared a state of the zone the scene is not made
  for. The claim that the automatic exposure darkened the bright zones stands on its own run (`i141-t1d`).

**The cutscene act's overrides** (OBSERVED, `remo_bank_override` rows of the observers written for this tranche,
`cbc2a4a58`): in the original, 10,812 rows, half in the light bank and half in the fog bank, all during the ceremony
scene and none during 32000010 (the last row before the cutscene tool's): the map's records `{132000000, id}` (ids 0,
60, 300, 350 and -1) overridden on layer 11 by the ceremony file's `{232000000, id}` (ids 35, 350, 200, 20, 1020 and 0),
the same pairs in both banks. **Natively: not observed.** The native run has no row because the four hooks were installed
in the Reference block only (`22_setup.inc`; the observers' comment and their commit `cbc2a4a58` said « both modes »,
wrongly: the merge's integrity review, its High, which the run's missing `reference_sun_block` rows confirm). The pilot's
first reading (« the game does not call them in native mode ») was a zero read as a fact, and is withdrawn. Whether the
act writes its overrides natively was open at that point (answered by the second observation, below: it does); whatever
it writes there lands in the engine's banks, which the native renderer
does not read (its records come from `Banks`): so the map's pieces of a cutscene keeping the map's light and fog natively
is INFERRED from the code, not seen. That the act does run natively and reaches the fog bank's function is already
on record: the E pilot's crash stack of Friede's intro, `FUN_140097600 < 140038430 < 140CC8180+0x22C` (I-0140 RESULT « The
faults from the game tour's cutscenes », fixed there by the registry's group slots); so the likelier gap is that `Banks`
does not read the layer-11 records, not that the act is silent (the merge's final review, H1). Fixed for the next run: the
observers installed in both modes, with a row that says so (`remo_bank_observers`), the act's own update observed too
(`remo_draw_param_act`: its calls and callers), and their rows kept by key (the original's 10,812 rows were every call:
the first form compared each call with the last one only, and the act writes several keys in turn). No removal was seen
in the original's run (0 `remo_bank_override_removed` rows): the removers' observers are unproved.

Read since (STATIC, GhidraMCP, the pilot): the two functions' one caller is the act's update `FUN_140CC8180`, which
writes the same override into five banks in turn (`[GXRS+0x41E0]` LightSet, `+0x41E8` Fog, `+0x41F0` LightScattering,
`+0x42A8` Env Map, `+0x4600` Snow Model), or removes it from the five when its track names no record; it has no static
caller (its one reference is its unwind entry at 144A2F248: a virtual call). The record it writes comes from
`FUN_140CC8530` on the act's track (`+0x170`): a source kind at `+0x32` (-1: none, the override removed; 0: one area
getter, `FUN_140CCDA60`; otherwise the other, `FUN_140CCD9F0`), its id at `+0x7A`, and an optional second source at `+0xC2`
and `+0x10A` blended with the first over the act's time (the `t` of the observers' rows). A candidate cause of the E pilot's cyan veils (the map's fog in full in 45000030,
45000031 and 51000010, I-0140) and of the tour's pale ceremony scene: a hypothesis, to test once a scene's overrides are
observed natively and then given to `Banks`.

**Not answered by this observation**: the last cut's veil and the denser haze in play after the scene (32000010 has no
override in the original, so their cause is elsewhere: the ceremony state's own files, the fade of `m32_00_0100`, the fog's
reading); the ceremony scene itself was not shot through (one shot at the arrival, the rest from its end): to shoot with
the same protocol, the 80 s moved to the travel's end.

**The second observation: the native side** (`i141-t2c-c32000010-nat`, 2026-11-14 23:58 to 2026-11-15 00:05, the OLED
in a 2560x1440 window, `build-t5e3`: the branch at `4329951b5`; the same state and flags as the first, the ceremony scene
shot through, a shot a second for 99 s from 12 s after the travel; exit 0, no fault, the save restored; the wrapper
`evidence/I-0141/artifacts/tranche-2/run-t2c.ps1`, its log `runs-t2c.log`, the sheet `ceremony-t2c-nat.png`). OBSERVED:

- The observers installed (`remo_bank_observers`: installed true; this build's row is one word for the five hooks).
- The act's update runs natively: `remo_draw_param_act`, over 3,000 calls through the ceremony scene, all from one
  caller (return address RVA 0x235B32A), the target ids 0 and 60 on the rows sampled.
- It writes the same overrides as in the original: 62 `remo_bank_override` rows (kept by key now), 18 distinct (bank,
  key, layer, A, B), **the same 18 as the original's run** (compared set against set: the map's `{132000000, id}` on layer
  11 from the ceremony file's `{232000000, id}`, in the light and the fog banks), `t` 0 on 58 rows and between on 4 (a blend
  of two sources at a cut's change). No removal (0 rows), and no override during 32000010, as in the original.
- So the native's difference is not in the game's act: the overrides land in the engine's banks, and the native renderer
  takes its light sets and fog from `Banks` (the files it read itself), which has no layer of kind 9. That the map's pieces
  keep the map's records during such a scene natively follows from the code (INFERRED; no picture compared yet: the
  original's ceremony scene was not shot through).

**Reviews at the merge** (the branch at `6b06c5740`, with I-0138's tranches 5d and 5e; a fresh `final-reviewer` and a
fresh `integrity-reviewer`). Both: **one High, in this record**: « natively the game does not call them » rested on
observers installed under the original renderer only (above). Answered: the claim withdrawn here and with Thomas
(23:17), the observers moved to the setup's common part with a row that proves them, the act's update observed too. The
final review's Low: **L1** the observers' rows were every call (kept by key now); **L2** the removers unproved (said
above) and their sixth argument passed on as the whole stack word; **L4** this record's artifacts list and the
ROADMAP's sentence; **L5** « at the same level » where the last cut differs (said in the Conclusion). The integrity
review's Low: the runs' flags listed. Its checks that held: the rows' counts and their place in time, every mean luma of
the table recomputed, the builds, the event dump's lines.

**The step review of those fixes** (a fresh `step-reviewer` on `6b06c5740..4329951b5`): no High, no Medium; the hooks'
placement on every path, the pass-through and the signatures held (the removers' sixth argument is a 4-byte float in
the stack word, so the whole word passed on is right). Six Low, fixed (`fd697c64a`, read by the pilot only, not reviewed
again): a word a hook in the observers' row; the tables' bounds hardened against a second thread; a row when a bank's
table is full; the original-probe block without the act's observer, said in the code; two stale sentences. The seventh
(the tab's Defaults Button shown before the game adds its row, unconditional) is left: the game always adds that row.

**The merge** (2026-11-15): `c9cc6bcf8` into `main` at 00:12 (the branch at `46f8a678a`: I-0138's tranches 5d and 5e with
I-0141's tranche 2, first observation; `Test-Project -MergeGate` passed before, `Test-Project` after); the launcher's
module rebuilt in the main checkout (`evidence/I-0138/artifacts/build-launcher-c9cc6bcf8.log`: exit 0, 179 PASS lines);
the start check `start-c9cc6bcf8` passed on the OLED in a 2560x1440 window (00:30: at the world in 3.28 s, closed, the save
restored). The disk rotation was refused at the checkpoint (another thread's game was running; C: 250 GB free and more).

## Tranche 3: the cutscene act's overrides given to `Banks` (2026-11-15)

By the R pilot's fresh thread (Opus 5.5, branch `claude/nifty-lewin-0959f1`, D-0281).

**What the original does** (READ, GhidraMCP, 2026-11-15, the pilot):

- `FUN_140031A20(bank, key, layer, A, B, t)` (the light sets' bank `[GXRS+0x41E0]`): finds or makes the record node of `key`
  (`GparamLightSetRecord_Construct_140149460` and `SetDefaults_14014B3A0` for a new one, the key at its `+0x2E0`, `+0x2E4`),
  registers `key` on `layer` in the layer table (`FUN_140097600` on `[GXRS+0x4788]`), looks A and B up
  (`GparamBankLookup_140032770`), makes four lights with `FUN_140148F50` in a 0x2A0-byte buffer, blends A's and B's records
  `+0x40` into it with `FUN_140147ED0(out, A+0x40, B+0x40, t)` and copies it into the node's `+0x40` (`FUN_14014CCE0`).
  `147ED0` copies A whole when t <= 0 (then turns the four lights' vectors by the record's rotation `+0x290`), and when t > 0
  lerps the colours, the angles (`AngleLerpWrap_1400F8FD0`), the ambient rows and the rotation (`QuaternionSlerp_1400FEA40`).
- `FUN_1400380A0`, the same for the fog's bank (`+0x41E8`): the node's `+0x10..+0x6F` A's record, or `FUN_14014E5D0`'s blend of
  A's and B's at t when t > 0 (A and B through `GparamBankLookupFallback_140038E70`); the key at `+0x70`, `+0x74`.
- So the record of an overridden key is the override itself, over the map's, until it is removed: `FUN_1400320B0` and
  `FUN_1400387D0` (bank, id from, id to, area, layer) remove a range of an area's ids on a layer; the cutscene's map holder's
  destructor `FUN_140CDCCD0` withdraws its area's (`+0xD4`) kind-9 layer (`GetGparamKindRegisteredId_140D27BA0(9)`, 11)
  through `GparamFile_Withdraw_14004A060`, gated on the MapOverride_forRemo cap's GX handle (`+0x100`'s `+0x70` not -1).
  The act's key takes its area from `FUN_140CDDE40`, which reads the same `+0xD4` of the object the act reaches through
  `CC1350` and `CCDB30` (that both are the same holder: INFERRED); that `4A060` takes the act's records with the layer is
  INFERRED too (its body not read for them). `320B0` removes `to - from + 1` ids from `from` on, none when `to - from` is
  negative (its loop, read); neither remover was seen in a run.
- The engine functions the module now calls on other threads than the game's are pure on their inputs (READ): `147ED0`
  (lerps, `QuaternionSlerp_1400FEA40`, `AngleLerpWrap_1400F8FD0`, `AngleToDirection_140148E30`, `QuatRotateNormalize_140149040`,
  `.rdata` constants), `148F50` (constants, an asin approximation `141778150`, the CRT's `atan2f` `141FD3C60`, which reports
  a range error only for non-finite inputs, and the read-only global quaternion `144929B20`), `14E5D0` (lerps on its aligned
  operands, two calls of `14014F710` on the positions; it writes `+0x00..+0x57` when 0 < t < 1, B's 0x60 bytes when t >= 1).

**What is built** (one channel for every reader, R's and E's):

- `ds3light::Banks::SetRecordOverrides` and `Overridden`: a table of overrides by (group, area, id), group 0 the light sets
  and 1 the fog; `Banks::Sun` and `Banks::Fog` answer an overridden key's override before the key's own resolution (A's and B's
  records by their usual rule, one level only), blended by the engine's own functions installed by the module
  (`UseRecordBlends`: `148F50` then `147ED0` for a light set, `14E5D0` for a fog record); a change of the table moves the
  binding generation, so the caches keyed by it renew. Every reader of the banks takes it: the deferred and forward light
  and fog sets (`NativeGparamRecords`, which now also answers a key `{area, -1}` that is overridden, as the original's node
  does), the instance rows, the traced sun's lookups (`ShadowSunDirection`) and the E pilot's effect fog (`38E70`'s detour,
  I-0140 tranche 8, through `Banks::Fog`, unmerged when this was written).
- The module (`18_resource_admission.inc`): `31A20`, `380A0`, `320B0`, `387D0` and `CDCCD0` hooked natively, pass-through (the
  engine's own state stays the original's); the overrides kept in `remo_record_overrides` (under `gparam_lock`) and given to
  every published `Banks` (`ApplyNativeGparamAreaBindings`), once after each update of the act that changed them (the act's
  update `CC8180` hooked natively too). Ended where the original ends them as read above: the removers' ranges, and the map
  holder's withdrawal when it made its cap, without the cap's handle gate (natively the cap never has one); both links
  INFERRED, so a backstop: the retirement of an area's own file (kind 7) ends that area's overrides. Rows:
  `native_remo_override` (a key's first override), `native_remo_override_end` (a removal, the withdrawal or the backstop,
  with the counts); `light-observe.flag`'s rows as before, written by the native hooks in a native run (the observers' five
  hooks now in the Reference only). `no-remo-overrides.flag` leaves the banks without them.
- Kept as it was, said here: the overridden map keys are map keys for D-0262 and D-0279 (their lights 1 to 3 removed on a
  traced frame like any map set's), though their records are the cutscene file's: B's rule names the cutscene's own keys, and
  this tranche does not widen it. The act's three other banks (LightScattering, Env Map, Snow Model) are not followed: the
  native does not read them by key yet (STUDY-UNREAD-SETTINGS).

**Tests.** `glow_bank_test` (data part, on the bank folder): an override set, the generation moved and kept for the same
table; no light blend installed, no record; t 0 answers A through the blend, a 0x260-byte read its part, another id of the
area its own record; t > 0 blends A and B with the generation moved; the fog at t 0 A's 0x60 bytes with the key's area at
`+0x64`, at t > 0 the fog blend's bytes with the key's area; `{area, -1}` overridden like any id; a source's own override not
followed; the table cleared, the generation moved, the light and fog keys' own records again. The blends there are stand-ins
that copy A or B and mark the record (the engine's functions need the game); in the bank folder of the test the sources may
hold the same values, so the marks and `+0x64` are what tell an override from the key's own record. The module's hooks, its
table and the engine's blends on real records have no unit test: the witness runs stand for them.
A change to the test's runner, stated (D-0268): `Build-GameBootstrap.ps1` now sets `DS3_EXE` before `glow_bank_test` (it was set
just after it, for `yebis_blocks_test`): the light-set records the new checks read take the executable's constants through
`Rdata`; in a worktree, without the main checkout's `game/` copy, all seven light checks failed for want of them (the second
build's log; 0 failures with the variable set, the test run by hand on the same build).

**The step review** (a fresh `step-reviewer` on `8fc964829..59c7c93ac`): no High; criteria 1 to 4 and 6 held (no change
without an override; the sources read with their rule, the guard, the generation; the hooks once per mode with their
signatures; the overrides kept across a republication; no special case). Two Medium, fixed: **M1** the commit came before the
build and its tests had run (the first build failed on two shadowed names in the test, `C4456`; the build after the fixes
below is the one cited in « Evidence »); **M2** how an override ends rested on two INFERRED links stated as facts (now marked
above) with nothing else to end them: the backstop at an area file's retirement, and the witness's `native_remo_override_end`
row a criterion of the run. Low, fixed: **L1** the thread-local guard replaced by private `OwnSun` and `OwnFog`; **L2** the fog
blend's output 0x80 bytes, zeroed; **L3** the fog node's key words said as they are (`+0x70` the id, `+0x74` the area);
**L4**, **L5** two comments; **L6** the withdrawal only for a holder that made its cap (`+0x100`); **L7** the removers' range
as `320B0` loops it; **L8** the purity of the engine's blends (above); **L9** one publication an act update; **L10** the fog
blend, `{area, -1}`, the clear's generation and the fog after it tested; **L11** the observers' row and this record's
wording. Left: whether the removers' sixth word is a fade time (the native removal is at once; never observed).

**The witness** (`i141-t3a-c32000010-nat` 01:40 to 01:47, `i141-t3a-c32000010-orig` 01:47 to 01:53, 2026-11-15, the OLED in a
2560x1440 window, offline, no measure, `build-t3` at `00a17c62f`; tranche 2's protocol unchanged, `run-t3.ps1`, its log
`runs-t3.log`): both runs exit 0, no fault, the save restored. OBSERVED:

- Natively the act's overrides reached the banks: 10 `native_remo_override` rows (light and fog for `{132000000, 0}`,
  `60`, `-1`, `350` and `300`, from the ceremony file's `{232000000, 1020}`, `20`, `0`, `350` and `35`, at t 0 when first
  written); the observers' rows give the same 18 distinct (bank, key, layer, A, B) in both renderers again (62 rows natively,
  102 in the original). `{132000000, -1}` then took a blend, A `{232000000, 200}` and B `{232000000, 0}` at t 0.997 and 0.3815
  (the observers' rows): the only live run of the engine's blends `147ED0` and `14E5D0` on this bank's records, without a fault
  (their values not compared).
- They ended where the reading says: one `native_remo_override_end` row, `map_holder_withdrawn`, area 132000000, layer 11, the
  10 erased, 0 left, before the cutscene tool's 32000010. No removal and no backstop row. The overrides' area is the holder's
  `+0xD4` in this scene (the first INFERRED link, seen here for one scene).
- The pictures (`ceremony-orig-before-after.png`: the original, the native before this tranche (`i141-t2c`, same protocol)
  and the native after it, a row every 2 s, the original's shots 1 s earlier, its scene being 1 s ahead; `kneel.jpg`, the
  kneeling shot (28 s) and the shot down the bell tower (30 s)): judged by eye, in those close shots the map's pieces take
  the ceremony's records. The kneeling player's background, warm sunlit stone before, is a cool grey after, as the
  original's (whose grey is bluer); down the bell tower the balustrade, black before, is lit grey. Whole-picture mean luma
  (ffmpeg's signalstats, of 255; `levels-*.tsv`), at the kneeling shot: original 81.8, before 88.7, after 75.9; down the bell
  tower: original 55.4, before 67.0, after 61.3 (brighter than the original's). The hue was not measured. The pairing by the
  clock is noisy: from 10 to 20 s (the wide shots over the cloud sea) the two native runs are within 4.6 units, from 21 to
  37 s they differ by up to 39 (34 s: 98.1 before, 137.2 after), partly different camera positions at one second; back in
  play (39 to 99 s) they agree within a few units (the overrides gone).
- What stays different from the original: the ceremony's smoke drawn as a dense veil over the bell shots in both native
  runs (the effects without fog: the E pilot's tranche 8, I-0140, its fog detour reading `Banks::Fog`, so the overrides,
  once both are merged); most close shots brighter natively than the original's at the paired seconds (the original's 32,
  33 and 34 s at 77.6, 100.7 and 122.7, the native's 33, 34 and 35 s at 121.2, 119.9 and 137.2), the smoke among them; the
  last cut's veil of 32000010 and the haze in play (tranche 2's): not in this tranche.

**Reviews at the merge.** The optimisation pass (read): `Banks::Sun` and `Fog` are called for every light and fog key of
every frame, and the first form took the bank's lock at each call to look for an override even with none: an atomic flag
now answers an empty table without the lock (`any_override`, set with the table); the act's update publishes once
(the step review's L9); the overridden record is built at each lookup, as many times as the key is looked up (a handful of
keys during a scene, nothing outside one). The pass came after the witness: its tree was built as `build-t3b` (exit 0, 179
PASS lines, 66 checks), not run live; `build-t3b` was written into the witness's folder `build-t3` (the witness's binary
replaced by the pass's).

- **The final review** (a fresh `final-reviewer` at xhigh on `main...b555cf4a9`): no High; criteria 2, 3 and 5 held (the
  signatures checked against `CC8180`'s calls and `CDCCD0`'s epilogue; the lock order `gparam_lock` then `variant_m`; the
  republication; no stand-in, no rule keyed to an asset). Three Medium. **M1** the merged tree had no build: `build-t3b`
  above, then `build-t3c` after these fixes. **M3** the hooks assumed the act the one writer: GhidraMCP's references give
  `CC8180` as the one caller of `31A20`, `380A0`, `320B0` and `387D0` (their other references unwind data), and of
  `CDCCD0` `CCD550` and `CDD760`; said in the code. **M2, left for the next tranche, with its reason**: natively the act's
  genuine writes also make nodes in the engine's banks, which the original's withdrawal removes and the native's never does
  (`4A060` gated on the cap's handle); a key `{area, -1}` that is no longer overridden goes to the engine's lookup
  (`NativeGparamRecords`, the I-0113 rule) and so finds the act's node after the scene. In `main` that node holds the engine's
  built-in records blended (its sources looked up natively in empty banks: INFERRED, not compared), close to what the lookup
  returned before the scene; it matters once the E pilot's fog detour answers the act's sources from `Banks`. The fix (the
  genuine removers called for the erased keys when the holder withdraws, on the game's thread) changes the engine's state,
  so it goes with the E pilot's combined witness of 32000000, the next tranche. Low, fixed: **L1** the observers' stale
  comments; **L2** the dead second `DS3_EXE`; **L3** the output zeroed once on the plain path; **L9** B looked up for a light
  override only when t > 0 (147ED0 reads it only then); **L10** the removers' range as a tested function
  (`Banks::RemovedId`: from equal to, a range, a negative difference, the wrap at -1); **L6**, **L7**, **L8** this record's
  witness text (above: the t > 0 blends, the shots named, « darker » withdrawn), its uncertainties (below) and STATE's rows.
  Left: **L4** the generation moving at each t change of a blend (the instance rows read the overridden light sets, so their
  cache must renew: a generation of the overrides alone would need every cache to take two; a blend lasts a cut's change);
  **L5** three locks a file retirement (rare).
- **The integrity review** (a fresh `integrity-reviewer` on the same range): no High; the judge untouched (the runner's
  `DS3_EXE` stated, no check removed or loosened, 49 earlier checks and 17 new), no scene's special case, nothing cut during a
  measure, the counts and the luma figures checked. **M1** the tree's build: as the final review's M1. Low, fixed: **L1** the
  second close shot named (the bell tower) and « darker » withdrawn against the levels; **L2** the noisy pairing said with its
  range; **L3** the test's A and B now chosen different and that asserted (the first id of default whose light record
  differs from `{1, 0}`'s); **L4** the seven light failures of the second build and the hand-run test have no kept log (the
  build log was overwritten by the next build, the hand run printed to the console).
- **The step review of those fixes** (a fresh `step-reviewer` on `b555cf4a9..b6936008b`): no High, no Medium; the output bytes
  defined on every path, the t <= 0 path the same as before wherever B had a record (the engine's lookup of B never fails: it
  returns its built-in record, so 31A20 always writes), `RemovedId` the loop, `DS3_EXE` inherited by every later test, the
  adaptive B not vacuous. Two Low left for the next tranche's build (each would cost a full build now, and neither changes the
  code's behaviour): the new t <= 0 rule (a B without a record) has no check of its own; the fog checks do not assert that
  the fog records of A and B differ (the light records' do).

**The merge** (2026-11-15): `a2448ae21` into `main` at 02:33 (the branch at `d1a09a9d7`; `Test-Project -MergeGate` passed before,
`Test-Project` after); the launcher's module rebuilt in the main checkout (`evidence/I-0141/artifacts/build-launcher-a2448ae21.log`:
exit 0, 179 PASS lines, 68 checks of `glow_bank_test`); the start check `start-a2448ae21` passed on the OLED in a 2560x1440 window
(03:01: at the world in 3.36 s, closed, the save restored); the disk rotation run (nothing old enough to delete, 242 GB free).

## Tranche 4: the act's engine nodes removed at the withdrawal (2026-11-15)

**The defect** (tranche 3's final review, M2): natively the act's genuine writers also make its record nodes in the engine's own
banks, and nothing removes them (the original's withdrawal `4A060` is gated on the MapOverride_forRemo cap's GX handle, which
a native cap never has). A key the native renderer leaves to the engine's lookup (`{area, -1}`, `NativeGparamRecords`' I-0113
rule) then finds the act's record after the scene; with the E pilot's fog detour reading the act's sources from `Banks`, that
record would be the ceremony's real fog.

**What the original does** (READ, GhidraMCP, 2026-11-15, the pilot): `CC8180` writes its override into five banks (the light
sets `[GXRS+0x41E0]` through `31A20`, the fog `+0x41E8` `380A0`, LightScattering `+0x41F0` `3C900`, Env Map `+0x42A8` `40DD0`,
Snow Model `+0x4600` `45CA0`) and removes it from the five (`320B0`, `387D0`, `3D080`, `40690`, `45560`). `4A060` calls each
bank's layer withdrawal; the fog's (`37B50`) unbinds the layer in the layer table (`96B70`), then destroys each node of the area
that no live layer still holds (`96DA0`) and marks the others to be rebuilt from the layers left; the light sets' bank goes
through `314D0` (not read: its withdrawal INFERRED alike). Natively those banks hold no file and their layer table no map
layer (`49D00` is `BindNativeGparamScene`'s), so no live layer holds the act's nodes and the original's rule would destroy each.
`320B0` and `387D0` find the area's node and the id's node and do nothing where either is missing (their `node != end` tests;
the other three removers INFERRED alike).

**What is built** (`18_resource_admission.inc`): a ledger of those nodes, `remo_engine_nodes` (group, area, id, layer), for the
five banks (the three others' writers and removers hooked, pass-through, for the ledger alone), filled at each genuine write
whatever `no-remo-overrides.flag` says, dropped where the act's own removers took them, consumed at the map holder's withdrawal:
the banks read first (a row `native_remo_engine_nodes_kept` when they cannot be), then the genuine removers called once a node,
`(bank, id, id, area, layer, 0)` as `CC8180` calls them. The backstop of tranche 3 (an area file retired) leaves the ledger. The
withdrawal is by area and layer, as `4A060`'s: a second act of the same area loses its keys until its next write. The row
`native_remo_engine_nodes_removed`: the nodes, the remover calls a bank, the thread, the light and the fog lookups of
`{area, -1}` before and after with whether each is its bank's built-in record (light `+0x700`, fog `+0x180`).

**The first witness** (`i141-t4a-c32000010-nat`, 03:30 to 03:37, the OLED in a 2560x1440 window, offline, no measure,
`build-t4` at `9e1dc8917`, light and fog only; `tranche-4/run-t4.ps1`, `runs-t4.log`): exit 0, no fault, the save restored.
OBSERVED: the 10 overrides as in tranche 3, withdrawn with the scene, then 10 remover calls without a fault, and the light
bank's lookup of `{132000000, -1}` moved from the act's node (`7FF379F19290`) to another record (`7FF3737B0BF0`; whether the
built-in one was not logged by that build).

**The step review** (a fresh `step-reviewer` on `90c7a6a8b..9e1dc8917`): no High, no Medium. Low, fixed: **L1** the backstop
erased the overrides that were the nodes' only record (now the ledger); **L2** `no-remo-overrides.flag` also kept the nodes
(now recorded whatever the flag); **L3** the removers' handling of a missing node, read and cited; **L4** a silent path and a
row hard to read (the thread and the built-in flags in the row); **L5** the withdrawal by area said; **L6** the vector reserved
outside the lock. **L7** whitespace only. Residual named by the review: the rays' walk pool reads the light bank through `32A80`
(05_selected_scene.inc's instance lighting); the act's inserts already ran on the game thread beside it, the removals are new;
the row's thread shows where they run (46316 in `i141-t5a`, the engine's thread).

## Tranche 5: D-0282, the lights 1 to 3 of an overridden map key's cutscene set kept (2026-11-15)

Thomas (the R thread, 03:14): « Oui », to the pilot's question of 03:08 (D-0282): a map key the act overrides by a cutscene
file's set keeps that set's lights 1 to 3 on a traced frame, as D-0279 keeps a cutscene's own keys', provisional until the
light sources are set by us (ROADMAP workstream 12). Built: `Banks::AuthoredLightKey` (a cutscene file's key, or a map key
whose light override's A is a cutscene file's set, and B too when it blends); `NativeGparamRecords::Authored` (both keys of an
entry, asked of the same banks the records are built from, a key of area -1 asked as `{1, 0}` as `Build` maps it) for the
deferred block's `authored` bytes and the forward rows' fill removal. Test: `glow_bank_test`'s « authored light key » (a
cutscene's key; an override from a cutscene's set at t 0, B a map's; the same blending, not; both a cutscene's, blending; A a
map's, not, blending or not; a fog override, not; no override, not). A test change stated (D-0268): the fog override's B in
`glow_bank_test` is now the first id of default whose fog record differs from A's, and that difference asserted (before, the
light test's B, which could make « fog at t > 0 is B's » vacuous); the t <= 0 rule of a light override without B's record still
has no check of its own (tranche 3's left Low).

**The step review** of tranche 4's fixes with tranche 5 (a fresh `step-reviewer` on `9e1dc8917..78a05124a`): no High, no
Medium. Low, fixed: **L1** the authored decision taken from another banks snapshot than the records' (now the records' own,
which also removes a lock an entry); **L2** the blend's positive case and a map's A blending, tested; **L3** the two diagnostic
lookups only when a row is written; **L4** one lock a write in `RecordRemoOverride`; **L5** `{-1, -1}` asked as `{1, 0}`. Its
note (whether the act rewrites a key at each update): not read.

**The witness of tranches 4 and 5** (`i141-t5a-c32000010-nat` 04:33 to 04:39 and `i141-t5a-c32000010-orig` 04:39 to 04:46,
2026-11-15, the ASUS in a 1920x1080 window: the OLED off, Thomas; offline, no measure; `build-t5`'s folder at `5d20ef4b6`, the
build of `build-t5c.log`, light and fog only; `tranche-5/run-t5.ps1`, `runs-t5.log`): both exit 0, no fault, the save restored.
OBSERVED:

- Tranche 4: the 10 overrides made and withdrawn with the scene, 10 remover calls on the engine's thread (46316), the light
  bank's lookup of `{132000000, -1}` a node before the removal (`before_builtin` false) and the bank's built-in record after
  (`after_builtin` true). That this is what the lookup gave before the scene is INFERRED (no lookup was logged before it). The
  observers' rows: the same 18 distinct (bank, key, layer, A, B) in both renderers again.
- Tranche 5: **no visible effect shown**. The pilot's first reading of `kneel-t5.jpg` (« a haze before, the stone after »,
  told to Thomas at 04:52) compared a « before » shot one second out of phase: in each run the scene's white flash peaks at a
  different second (`levels-*.tsv`: 23 s natively in `i141-t5a`, 24 s in the original and in `i141-t3a`), and `i141-t3a`'s shot
  at 26 s is still in the flash's tail, where at 27 s its background already shows the stone, dark (the integrity review's H1).
  Withdrawn, and said to Thomas (05:00). The levels do not show a gain either: the top-left quarter (the background;
  `tranche-5/quarter-luma.tsv`) 74.9 in the original, 77.8 before, 68.6 after, further from the original; the two native runs
  also differ by screen, size and build. D-0282's effect is to be shown by an A/B on one build and one screen.
- `t5-orig-t3-t5.jpg` (26 to 34 s): the native's haze over the bell shots stays; the E pilot read it as the world's fog in the
  cutscene, not the effects' (its `i140-e27`, I-0140; « Suggested next step »).

**The final review** (a fresh `final-reviewer` at xhigh on `main...c1130e7c6`, briefed under D-0283): no High. Three Medium:
**M1** tranche 4's premise written as fact (`4A060` removing the act's nodes): now READ for the fog bank (`37B50`) and stated with
the light bank's `314D0` not read, above and in the code; **M2** the act writes five banks and the withdrawal removed two: the
five followed and withdrawn; **M3** `review-final`'s description not valid YAML (an unquoted « : »): quoted, `review-step`'s too.
Low, fixed: **L1** `EraseRemoOverrides` returned an unused vector (a count now); **L2** the ledger taken before the banks were
read (read first, a row when they cannot be); **L3** `Authored`'s key mapping now `Build`'s; **L5** a fog probe beside the
light's, the row's « removed » renamed `remover_calls`; **L6** the quarter luma in a file, the background said further from the
original; **L7** this record's stale text (the function's name, the « Uncertainty », the evidence lists, a dangling « below »,
« L4 » given twice); **L8** `review-final`'s order (the integrity review after the final review, D-0268); **L9**
`final-reviewer`'s first item and `CLAUDE.md`'s skill list. Left: **L4** no thread guard on the removal (the row's thread is the
engine's in the witness; a guard would need the engine thread's id at that point); **L10** D-0279's added reference to workstream
12 rested on Thomas's « Oui stp, pour que ça soit clair » in the project chat, relayed: confirmed by his « Oui » in the R
thread (05:11).

**The step review of the five banks** (a fresh `step-reviewer` on `c1130e7c6..f62526fae`, `build-t6`: exit 0, 179 PASS lines,
69 checks): no High, no Medium; the six criteria met. Low: **L1** the ledger takes any caller's writes: GhidraMCP's references give
`CC8180` as the one caller of the five writers and the five removers (their other references unwind data), said in the code;
**L6** two stale comments, fixed. Left, with the reason (each a diagnostic or a case not reachable today, a build not run for
them): **L2** the ledger does not keep the bank pointer (a bank remade between a write and the withdrawal: not seen; the two read
removers do nothing on a missing node); **L3** `remover_calls` counts a call behind a null trampoline (none can be null: a failed
hook stops the start); **L4** the fog probe goes through 38E70 as hooked once the E pilot's detour is merged (its builtin field then
meaningless); **L5** the act's removals in banks 2 to 4 scan the override table for nothing (never observed). Its open question:
whether the GXRS root and its banks can be gone before a holder's destructor at the quit (the two-bank form had the same exposure).

**The integrity review** (a fresh `integrity-reviewer` on the same range): **H1** tranche 5's « haze before, stone after »
compared out-of-phase shots (withdrawn, above, and told to Thomas); **H2** the fog test's B change not stated (stated above,
with the untested t <= 0 rule); **L1** the build evidence mislabelled (the witness's binary is `build-t5c.log`'s, 69 checks;
`build-t5.log` is an earlier build of the same folder, 68); **L2** the quarter figures without a file (now
`quarter-luma.tsv`); **L3** « what it gave before the scene » marked INFERRED; **L4** stale text (as the final review's L7).

**The merge** (2026-11-15): `ad15b002a` into `main` at 05:40 (the branch at `1e00f1aa1`; `Test-Project -MergeGate` passed before);
the launcher's module rebuilt in the main checkout after the E pilot's run (`evidence/I-0141/artifacts/build-launcher-ad15b002a.log`:
exit 0, 179 PASS lines, 69 checks of `glow_bank_test`); the start check `start-ad15b002a` passed on the ASUS (06:25, the OLED off,
the save restored).

## The ceremony's « world fog » re-read (2026-11-15, the fresh R thread, documentation)

The item « the world's fog in the ceremony scene » (the E pilot's `i140-e27`: the original's grey veil at the haze shot, YAVG 142.9
of 255, the native's 75.0) **is not confirmed**. On `i141-t5a`'s shots a second apart (the same ASUS 1920x1080 window in both modes,
the two runs aligned by their darkest frame, 37 s in both: YAVG 1 and 17.8), the kneeling shot is clear in the original too one second after the white flash
(YAVG at 25, 26, 27 s: native 81, 82, 88; original 93, 84, 83; `evidence/I-0141/artifacts/tranche-6/yavg-t5a-18-41.tsv`). The
original's veiled still of `i140-e27` (50 s, shots 5 s apart) is the flash's fading tail, taken out of phase (tranche 5's H1 again).
OBSERVED besides: natively the world's fog blocks carry the act's records (`native_fog_set` in `i140-e27-c32000000-nat`: {132000000,
0} and {132000000, 60} with start 0.1 m and density -0.833 while the act holds 1020 and 20, then the light record of 35); READ
(GhidraMCP, 14E900 and `GXLightComposite_ppo.asm`): the composite's distance fog is ExtParam.x (1 - 2^(-(d - y) z)) with ExtParam.x =
the record's word 3 times word 8 (0.155 for 1020), so that record cannot make a dense veil in either renderer.

**The scene's real difference** (OBSERVED, the same paired shots): the smoke from 28 to 35 s, natively far whiter and denser (YAVG at
29 s 108 against 61, 31 s 146 against 53, 32 s 127 against 64; `ceremony-paired-orig-left-native-right-24-25-26-29-31-33.jpg`), and in
`i140-e27` with the E pilot's tranche 8. Lead (I-0140 RESULT « Next » item 6, READ below): the effects' light.

## Tranche 6: the effects' light from the native banks (2026-11-15)

**What the original does** (READ, GhidraMCP, 2026-11-15): the effect draws ask the engine's light-set bank `[GXRS+0x41E0]`
(`GparamBankLookup_140032770`, the draw's key `+0xA20`): `GXFfxDrawContext_Slot27_CommitDraw_140264230` at 140265919 (the record's
`+0x2C0`, a strength the draw's colours `+0x590..+0x59C` are scaled toward a constant by), 140265AB3 ({-1, -1} when the id is -1),
140265B4E (`BuildCbLightRecord_1401496d0` on the record's `+0x40`, and the ambient words `+0x220..+0x23F`) and 140265CDB (1495F0's
rows); the GPU particles' 2A45A0, 2A4EB0, 2ADC80, 2AE5A0 and the act's writer 31A20 call it too. Natively that bank holds no file, so
every lit effect of the game drew with the bank's built-in record (`+0x700`) whatever its zone or the act's override.

**What is built** (`07_scene_lighting.inc`, `22_setup.inc`): a detour of 32770, natively, beside the E pilot's fog detour (I-0140
tranche 8) and with its rule: on the light bank only, a key the native rule resolves or one the act overrides takes a record built
from the native banks (an overridden key its engine node while 31A20 holds one), any other key the engine's answer, a node of the
engine's bank passed over for the built-in record. The record is a whole engine record (0x350 bytes): the built-in record's bytes
(its header and SetDefaults 14B3A0's words beyond Banks' range, +0x330..+0x340: READ; the strength `+0x2C0` from the set's LightMask Influence since the witness), `Banks::Sun`'s words over
`+0x40..+0x290` and its rotation at `+0x2D0`, the key at `+0x2E0`, then 1495F0's rows at `+0x290`. `no-effect-light-banks.flag` leaves
32770 unhooked; `effect-light-observe.flag` logs what the effects ask and get in both modes (`effect_light_key`: light 0, the ambient
words, the strength). `NativeGparamRecords::Build`'s fallback and tranche 4's withdrawal probe call the engines' own bodies.

**The step review** (a fresh `step-reviewer` on `f314c451e..5920e8765`, briefed under D-0283): no High. **M1** the thread-affinity
check scanned the three-line wrappers, not the functions holding the statics (the fog's too, its entries stale): the scope is now
`NativeEngineFogAnswer`, `LogEffectFogKey`, `NativeEngineLightAnswer`, `LogEffectLightKey`, with reviewed entries. **M2** the strength
`+0x2C0` taken as 1 for every key on an observation of map keys: READ since as 14B3A0's default for every record; that no file field
changes it is not read, and the witness logs it. Low: **L1** the store keyed on the whole record (the header is the built-in record's,
the same for all: said in the code); **L2** the record now 0x350 bytes with its key; **L3** both witnesses test their 600-row bound
before inserting a key; **L4** `Banks::BindingGeneration` an atomic read without the lock; **L5** the comment on 31A20's node narrowed
(Banks' records where its sources are keys the native rule resolves); **L6** the observer comment and a row `native_effect_light_detour`.

## Tranche 7: a zone file's fade (2026-11-15)

**What the original does** (READ, GhidraMCP; STUDY-LAYER-FADE): the area controller `AreaGparamController_Apply_140ce6350` gives one
transition time to a step's unbindings and bindings (`+0xC0`, or `+0x10C` when not negative; 0 when its caps were just found, with
`+0x15` or its other first-binding test), and the area's records fade linearly over it (140092B50). The time reaches the native as
the weight of the step's kind-7 binding (D26A70 to D275F0; 3.0 for `m32_00_0100`).

**What is built**: `Banks::SetAreaFades` keeps, per area, the layers before the change, a start and a duration; while the weight is
below 1, `Sun` and `Fog` of that area answer the engine's record blend (`UseRecordBlends`) of the record the earlier layers give and
the one the current layers give (the earlier layers seen by `Layer`, `File` and `BoundFile` on that thread only, for that area only,
through `FadeView`). `ApplyNativeGparamAreaBindings` starts a fade at a layer change carrying a binding's time; one controller step's
two native changes (the binding, then the unbinding of the cap it replaces) make one fade; a change without a time ends a running one;
the fades survive a bank's republication. The caches of those answers (`NativeInstanceLightRows`, the effect detours' maps and stores)
renew at each fade step (`RecordGeneration`, the step reviews below). Limits: the original blends value by value before deriving its records (INFERRED from STUDY-LAYER-FADE §2, READ in tranche 23), here
the records are blended (the same ends, maybe another path between them); an unbinding alone takes the step's time read from the controller (the final review's M1);
the other readers (`Env`, `Shadow`, `GroupFloat`, `GroupEntries`: the post settings) answer the new layers at once. Test:
`glow_bank_test`'s fade checks on a test clock (the weights 0, 0.25, 0.75, the end, another area, no earlier layers, an override
over a fade, a fade given when already over).

**The step review** (a fresh `step-reviewer` on `5920e8765..9c88948db`, with tranche 6's answers): no High. Four Medium: **M1** the effect
detours tested the fade before reading the generation, so a record built in the window could outlive the fade; **M2** the light
record's key at +0x2E0 in the wrong order (30D80 writes the id then the area); **M3** fading keys served from the eight-slot ring for a
whole fade; **M4** every lookup during a fade rebuilt its record. All answered by one change: the weights move only in SetAreaFades and
`Banks::AdvanceFades` (once a world collect, at `CollectNativeSelectedScene`'s start), and each step renews a counter of the fades'
own (`RecordGeneration` for the caches of Sun's and Fog's answers), so every cache renews each step and at the end; a fading key is
built once a step per thread into a per-thread step store. Low, fixed: **L1** fades given before layers; **L3** one controller step
scoped by `NativeAreaControllerStep`'s span on its thread, not a time window; **L6** the tests (records asserted to differ, a
quarter-turn rotation, the weight-0 fog, an override over a fade); **L7** comments. Left, said in the code: **L2** an expired fade is
dropped at the next step now; **L4** the wall clock, not the game's step delta; **L5** an unbinding alone keeps no time; **L8** the
thread-affinity scan does not see a `thread_local struct` (the detours' per-thread caches, reviewed here).

**The second step review** (a fresh `step-reviewer` on `9c88948db..a73180f01`): no High. Medium: **M1** a fade's step renewed the
whole bank's generation, so caches no fade changes (the snow rows, the Dominant rows, the region IBL scale, kept draws) were rebuilt
every frame: the fades now have their own counter, `RecordGeneration` = the binding generation plus it, for Sun's and Fog's caches
only; **M2** a step store cleared at a generation change could free a record between a caller's two lookups (31A20's and 380A0's A
and B): the store is kept one generation more. Low, fixed: **L1** the record's key as `NativeGparamKeyArea` maps it; **L2**, **L3** the
step's span as atomics with its thread (no lock a step, another thread not in it); **L4** the step stores bounded; **L5** the fog tail
checks' wording (the tail is the same in both records); **L6** comments; **L7** `AdvanceFades` before any model's lighting is asked;
**L8** the resident areas' whole table (2, 3, 5, 6, 7, 100000000 MapBase, 200000000 RemoBase) and the Dominant Light group read by
position where a name is unknown. Not checked by the reviewers: 31A20's own use of its two answers (its export not at hand).

## The witnesses of tranches 6 and 7 (2026-11-15)

The runs (the ASUS in a 1920x1080 window, offline, no measure; `tranche-6/run-t6.ps1`, `run-t6b.ps1`, `runs-t6.log`, `runs-t6b.log`,
`runs-t7b.log`; every run exit 0, no fault, the save restored): on `build-t6` (the tree with `main` merged, `ecae4d347`; `build-t6b.log`:
exit 0, 179 PASS lines, 87 checks of `glow_bank_test`) `i141-t6a-c32000010-orig` (09:42), `i141-t6a-c32000010-nat-no0282` (09:48,
`no-override-fill-lights.flag`), `i141-t6c-c32000010-nat` (10:16), `i141-t6b-c45000030-nat/orig` and `i141-t6b-c51000010-nat/orig`
(09:55 to 10:15); on `build-t7b` (`51db1fa7a`: the witness's two fixes below and the fog observer's; `build-t7b.log`: exit 0, 179 PASS
lines, 88 checks) `i141-t7a-c32000010-nat` (11:02) and `i141-t7b-c45000030-nat/orig` (11:09 to 11:18). The name `i141-t6a-c32000010-nat`
was the earlier R thread's (05:32): this series' first native run was refused by the runner (« Run exists »), hence `i141-t6c`.

- **Tranche 6, OBSERVED** (`effect_light_key`, both modes): the effects' light lookups natively answer the zone's and the act's records
  with the original's words (Archdragon Peak's {132000000, 0}, 100, the cutscene file's {232000000, 20}, 35, 1020; light 0, the
  ambient words), where before every one was the built-in record (READ: that bank holds no file natively; no run before the detour
  logged these rows). The witness showed two differences, fixed in `build-t7b` and seen
  equal in `i141-t7a`: the strength `+0x2C0` is the set's LightMask Influence (0.96 at m32, 0 for default's {1, 0}); a key of area 0
  ({0, 32000}, {0, -1}, {0, -2}) answers default's {1, 0}, not the built-in record.
- **The ceremony's smoke** (paired by the second, the runs aligned by the scene's darkest frame): darker than on `i141-t5a` at 29 to
  33 s (108, 106, 146 against `i141-t6c` and `i141-t7a` 75 to 118), **not attributable to tranche 6**: `i141-t5a`'s build lacks the E
  pilot's effect fog (I-0140 tranche 8) and the framings differ (an A/B with `no-effect-light-banks.flag` on one build is left to do);
  **still brighter than the original's** (54 to 86), the walls beside it too (the region 250x600 at (20, 200): 76 to 99 against 35 to
  59; `tranche-6/yavg-t6-t7.tsv`); natively the shots are framed differently in each run (the
  scene's timing), so the pairs are not the same picture; the automatic exposure stays at its lower bound there (`log2_factor` 0).
  Not explained: to read further (the light shafts, the haze between the walls).
- **Tranche 7, OBSERVED on `build-t7b`**: natively the fades of 3 s at 132000000 (32000010's storm), 145000000 (the Painted World's
  arrival) and 151000000 (51000010) (`native_gparam_area_fade`); the original's fade seen at 132000000 (its `effect_fog_key` rows) and
  145000000, not at 151000000 (its observer's rows spent elsewhere: INFERRED there from CE6350's reading); **the world's fog blocks
  during the Painted World's fade the original's step for step** (`fog_block_build`, the composite's 137B97 against the native's:
  FogParam.x 0.5024, 0.5076, 0.5141 ... natively, 0.5028, 0.5067, 0.5105 ... in the original, from the same record; the original's
  composite rows stop mid-fade at the observer's 200-row bound, its effects' rows show the end; the step size differs by the two runs'
  frame times). The final review's fixes (`build-t8`: an unbinding's fade, Dominant Light, the stores) have not been run live; the start
  check after the merge is their first run.
- **D-0282's A/B** (`i141-t6c` against `i141-t6a-...-no0282`, one build, one screen): the kneeling shots' levels differ by less than the
  runs' own spread (72 to 84 with, 71 to 76 without); no visible effect shown, as tranche 5's run.
- **45000030's cyan veil: not the world's fog (INFERRED for the composite).** The cbFog blocks of the cutscene's first cuts are the same
  in both modes for the effects and forward rows (266213, 263B0E: the original's composite rows were spent during the arrival's fade;
  natively the composite's equal them) (the act's override, FogParam (0.462, 0.0067, ...), HeightParam (1, -20, -0.0033, 5.36); then the dark record 0.9014), and the veil covers
  the player a few metres from the camera, where neither the distance nor the height fog reaches. Natively the light shafts draw there
  (`native_yebis_shafts` 31 draws, the m45_00_0000 record) — a lead, not shown (I-0138's pass, workstream 11). The whole frame is still
  brighter natively in the later cuts (YAVG 63 to 96 against 47 to 57). 51000010 no longer shows a veil; natively brighter, the player
  darker after it (the known « player wrong after a scene »).
- **The haze in play after 32000010** (`i141-t5a`'s shots, natively denser): the fog record in play is m32_00_0010's in both
  (`native_fog_set`); the composite's blocks in play to compare with the fixed observer's rows (not yet read).

## STUDY-UNREAD-SETTINGS, Thomas's « Corrigez les quand même » (2026-11-15)

- Item 5 (eclipse): **left** (the final review's L1: Banks reads a file's groups by position and eclipse.gparam holds a Fog group alone; naming its area 7 would read the wrong group). Read for it: the resident kinds' areas are 27790's table 14294EFB0 (READ: kind 12
  `eclipse` 7; the templates 2, 3, 5, 6; MapBase, MapOverride and MapOverride_forRemo 100000000; RemoBase 200000000). Whether the engine
  ever keys area 7 is not read.
- Item 10: Dominant Light read by name, a name of none of the eleven at its own place (m36_00_0002's fourth, « Dominant RGB Scale »), and a parameter the group lacks keeps the built-in row (m36_00_0002 holds four, m30_00_0100, m30_00_0110 and m36_00_0001 seven); no test file holds such a group.
- Item 6 (Lens Simulation): **never drawn in the original**, so nothing to make native (a `ghidra-research` reading, GhidraMCP and a
  scan of the executable's displacements, 2026-11-15; READ but where marked): FB890 looks the group up and records its two GX lens
  filters only when the pipeline's bytes `+0x3EC1` / `+0x3EC2` are set; the constructor 1400843C0 clears them and their only setters
  (`PipelineSceneContextSetup_140085540` at 1400867F2 and 140086849) need the view policy's bit 6 or bit 36, which no view type's
  policy word carries (the main view `0x1000951EE1F` and its masks, the menus' `0x8422014` and `0x400852201F`, the reflection and cube
  pipelines' `0x2207` / `0x8002207`, the resize paths); the debug menu's « Enable Lens Simulation » boxes are gated alike. Open: a
  camera effect mask (`1403BCD70` sets bit 36 in `cam+0x68`) whose path to the pipeline's policy was not found, and no scan of later
  stores to `+0x50` (INFERRED: none).
- Item 7 (LightScattering): no renderer reader found. Its bank is read by the cutscene act's insert (14003C900 through 14003D710, and
  14003D080), which blends two records (14014F7C0) and registers their fields in the generic parameter registry `[GXRS+0x4788]` that
  every group uses; whether a shader reads those slots is not read, nor the body behind the dispatcher thunk at 14003D600 (an
  obfuscated trampoline, outside the I-0133 overlay). Left as dead data until a reader is shown.
- Items 8 and 9 need nothing (the built-in record at unset keys; data inert in the shipped files). Items 1 and 2 were made native by
  I-0138 tranche 6 (the cutscene file's LUT strip, mapbase for map areas only): their witness in a cutscene is still to run.
- `no-override-fill-lights.flag`: D-0282's A/B on one build and one screen (the overridden map keys' part of `Authored` off).

## Reviews at the merge of tranches 6 and 7

**The final review** (a fresh `final-reviewer` at xhigh on `main...37e3418d9`, briefed under D-0283): no High. Three Medium:
**M1** a step that only unbinds a file jumped where the original fades over the step's time (CE6350 gives D26DD0 the same time): the
unbinding now takes the time read from the controller before its step; **M2** the Dominant Light fix was inert on m36_00_0002 (its
group holds four parameters): the group is read by name, an unknown name at its own place, a missing parameter the built-in row;
**M3** stale and contradictory records (I-0138's « answer awaited », ROADMAP's « pick open », this RESULT's superseded sentences, its
Uncertainty, Evidence and Artifacts, status.json): rewritten. Low, fixed: **L1** item 5's resident-area names withdrawn (eclipse.gparam
holds a Fog group alone and Banks reads groups by position); **L4** the step stores kept at a bank change too; **L5** the instance
strength comment, the gap recorded; **L6** the fog block witness in the thread-affinity scope; **L8** dead code; **L9** this record's
wording. Left, with the reason: **L2** the area-0 rule in the two detours only (the world's sets never ask area 0); **L3** the two
detours' common shape kept as two functions (one bank each, a template would bind E's and R's code together); **L7** no unit test of
the fade's start rules or of a partial Dominant Light group (they live in the native admission, under the game's state);
**L10** the fading set's rotation the current layers' (the witnessed layers share one transform).

**The step review of the fixes** (a fresh `step-reviewer` on `37e3418d9..67c8c65dc`): **H1** a bank publication still freed the step
store (two rotations): one rotation now, at the generation check; **M1** the unbinding's time missed the case where the step itself
sets `+0x13` (`+0x11` and `+0x18` against `+0x2C`, asm 140CE63B1..CD): zero there too; Low, fixed: **L1** DominantAt's comments,
**L2** a positional pick of another kind skipped, **L5** « 0.96 for most of m32's sets »; left: **L3** a named parameter without the
key's id reads 0 (no shipped file has one), **L4** no test of a partial Dominant Light group. Built in `build-t8` (`build-t8b.log`).

**The integrity review** (a fresh `integrity-reviewer` on `main...20b69b492` with this RESULT): no High; the tests not weakened (70
`Expect` on `main`, 88 on the branch; the one removal went with its code), no special case for one scene, no work cut during a measure.
Medium, fixed in this record: **M1** 45000030's « not the world's fog » rested for the original on the effects' blocks (its composite's
rows were spent during the arrival's fade): now INFERRED for the composite; **M2** the original's fade at 151000000 not observed: said;
**M3** the smoke's darkening compared builds differing by the E pilot's tranche 8 and by framing: not credited to tranche 6, an A/B
with `no-effect-light-banks.flag` left to do; **M4** the final review's fixes not run live: said (the start check their first run). Low:
**L1** the figures' file (`tranche-6/yavg-t6-t7.tsv`), the original's range 54 to 86, the walls' region, « black » at 37 s (17.8);
**L2** the area-0 rule in the fog detour INFERRED from 27790's table (no area-0 fog key seen in either mode); **L3** « before, the
built-in record » marked READ; **L4** the stale sentence on an unbinding's time; the investigation's budget renewed from 8 to 12
iterations (CONTRACT « Budget exhausted »).

**The merge** (2026-11-15): `25f3f309b` into `main` at 13:00 (the branch at `a51a099ff`; `Test-Project -MergeGate` passed before);
the launcher's module rebuilt in the main checkout (`evidence/I-0141/artifacts/build-launcher-25f3f309b.log`: exit 0, 179 PASS lines, 87
checks of `glow_bank_test`); the start check `start-25f3f309b` passed on the ASUS (13:16, restored), the first run of the final review's
fixes; the disk rotation run (nothing to delete).

## After the merge: 45000030's veil is the light shafts (2026-11-15)

**OBSERVED** (`i141-t8a-c45000030-nat`, 13:45, the ASUS 1920x1080 window, offline, no measure, `build-t8` = `main`'s code, the same
flags as `i141-t7b` plus `no-yebis-shafts.flag`; `tranche-6/run-t8.ps1`, `runs-t8.log`; exit 0, restored): without our light shafts
the first cuts' level is the original's (YAVG at 5 and 10 s: 46.4 and 49.2 without them, 97.9 and 101.1 with them, the original 45.8
and 47.9); the later cuts do not move (63 to 96 either way, the original 47 to 58). The E pilot's check of the same runs: no FFX
light-shaft row (3D65A88, `native_ffx_light_shaft`), so the veil is the post chain's screen shafts (I-0138, workstream 11), not the
effects' shafts. The native draws them with the map's key {145000000, 0} (the engine context's key index 9 during the scene; its
LightShaft record Enable 1); the cutscene file's own LightShaft is Enable 0. Why the original draws none there is being read
(the shafts' driver and its zero-colour rule, AppendNativeShafts' note on Reference1670).

The ceremony with `no-effect-light-banks.flag` (`i141-t8b-c32000010-nat-nolight`, 13:50, the same build): run, to compare with
`i141-t7a` for what tranche 6 alone changes in the smoke (the integrity review's M3). OBSERVED, the two runs aligned by their black
frame (36 and 37 s): over the smoke shots the levels go both ways by up to 20 or so (with the detour 79, 86, 103, 118, 115, 113, 178,
179; without it 95, 93, 113, 126, 134, 121, 151, 159), within the runs' framing differences: tranche 6 does not measurably change the
smoke's level, and the smoke's excess over the original's is not the effects' light.

**The library's own gates for a shaft** (READ, a `ghidra-research` reading, GhidraMCP 2026-11-15): FB890 blends the LightShaft records
of the context's keys `+0x4020`/`+0x4088` (the native's key index 9) and E4700 writes the globals: Enable `+0x130` (0 when the settings'
scale `[[GXRS]+0x5368]+0x50` is not positive), the strengths, Rad-X/Rad-Y `+0x160`/`+0x164` and MaskDepthThreshold `+0x24C` from which
E69A0 builds the sun's direction (no light set: the record's angles and the camera at `passCtx+0x210`); the slot's colour is
`clamp(pose.row2.xz . d.xz + 0.3, 0, 1)` times the record's colour. `FUN_1416EA000` keeps a slot as a glare record only when its Enable
byte is set (`FUN_1416F37C0`: also gated on `glare+0xFC`, not traced), the glare luminance `S+0x6A4` (the globals' `+0xDC`) is positive,
`S+0x6DE` is 0 (not traced) and its colour is not zero; no Remo branch found. **Built** (`d66d57a82`, `build-t9`): the Enable and the
luminance gates in AppendNativeShafts. **Witness** `i141-t9a-c45000030-nat` (14:23, exit 0, restored): **the veil stays** (YAVG 98.0,
101.1 at 5 and 10 s): Enable is 1 through most of the cut (`native_yebis_shafts` refused « disabled » twice, 29 to 32 draws between). Left to
read: `glare+0xFC`, `S+0x6DE`, the original's slot colour in that cut (an observer of E69A0's slot or 1416EA000's kept count in the
original), and whether the original's glare stage runs there at all (natively the glare plan is refused « glare shape » at moments).

**The library's gates, read further** (a second `ghidra-research` reading, 2026-11-15): `glare+0xFC` is a creation-time constant, 1 for
the whole run (YebisInitLibrary's flags bit 28, hard-wired); `S+0x6DE` is zeroed at init, no other writer found, and if set it skips the
whole post process (bloom too), so it is not this cut's difference; FUN_1416EA000 runs every frame from EndPostEffectScene_Process;
1416C9670 leaves `S+0x6A4` (the glare luminance) at its last value when the globals' `+0x244` is not positive; YebisGlareFrame skips the
anamorphic stage (and the shafts) when an earlier stage (source chain, bloom pyramid, star, ghost) fails. In `i141-t9a`'s cut the
native's globals give luminance 3 or 6 and `+0x244` 0.5 (both positive), and the native glare plan is refused « glare shape » on part
of the rows. Left: the original's slot colour and its glare stages' results in that cut (an observer of E69A0's slot and of
YebisGlareFrame's returns in the original), then the gate that differs.

**The original's keeper observed** (`i141-t10a-c45000030-orig`, 14:57, the ASUS, offline, no measure, `build-t10` with
`shaft-observe.flag`; `runs-t10.log`; exit 0, restored): in the cut the keeper's kept count `[glare+0x1C0]` is 0 on most calls and 1 on
some (several calls a frame, the luminance read 1, 3 or 6 by turns: more than one view); the slot fields as read (`S+0x6B0` colour,
`S+0x700` Enable) stay about 0 before and during the cut, so those offsets are not the slot's (the slot base may be a pointer):
INFERRED, to read again before relying on them. Next: the slot's true layout (FUN_1416EA000's loop), then the original's kept count
for the main view only, against the native's shaft draws in the same cut.

**The keeper's slots read again** (READ, GhidraMCP 2026-11-15, this thread): FUN_1416EA000's loop takes its slots from the pointer
`[S+0x6B0]`, 0x58 bytes each, `[S+0x378]` of them (the index -1 meaning `[S+0x6B8]`); a slot is kept when its byte `+0x50` is set, `S+0x6A4`
is positive and one of its colour words `+0x00..+0x08` is positive; it then copies the slot to the glare's record `[glare+0x1C8] + i x 0x50`
(FUN_141752DB0, E60, EF0), scales the colour by `S+0x6F8` when `S+0x456` is set, and returns kept > 0 (also `S+0x9A0`). The earlier observer
read the pointer as a colour: `i141-t10a`'s slot fields are void. **Observed** (`i141-t11a-c45000030-nat/orig`, 19:36 to 19:46, the ASUS
1920x1080 window, offline, no measure, `build-t11` = `5f20c20f4`; `tranche-6/run-t11.ps1`, `runs-t11.log`; both exit 0, restored): in the
original's cut the shaft **is kept** (kept 1, Enable 1, glare luminance 6) with the native's own colour, cyan (0.18, 0.50, 0.65), rising from 0
over its first 150 frames or so (frames about 7700 to 7850 of the run, the slot's depth 0.995 to 1); natively the same slot (0.18, 0.50, 0.65,
Enable 1, luminance 6, `native_shaft_slot`) from the cut's start. Yet the original's picture stays dark where the native's is a uniform cyan
veil over the whole frame, foreground included (`cutscene-5`, `cutscene-10`): the shaft's inputs agree, its drawing does not. Lead (INFERRED):
the mask, Alpha_R's depth words (the native takes the DoF holder's words as (1, 0, 1, 0), INFERRED in LS-1), which would let every pixel through.
Next: those words and the kept record observed in both modes (`build-t12`, `reference_shaft_depth` and `native_shaft_depth`).

**The focus found** (superseded in part by « Tranche 9 »: the cause is the game camera's request, not the auto focus) (`i141-t12a-c45000030-nat/orig`, 21:37 to 21:46, `i141-t12b-c45000030-nat` to 21:51, the ASUS 1920x1080 window,
offline, no measure, `build-t12` = `8267b1ba4`; `tranche-6/run-t12.ps1`, `runs-t12.log`; the three exit 0, restored). OBSERVED: in the cut the
original's library holds the main view's depth words (near 0.08, far 10000, the DoF branch on, the conversion words (1, 0, 1, 0), as the
native takes them) and **its focus `S+0x6C0` at 3.08 to 3.80 m**, moving with the shot; the native's focus (globals `+0x1B0`, which
Alpha_R and the native depth of field read) is **0, then 1**: Alpha_R's block is then (scale 0, offset 1) and (12.5, -11.5), the mask 1 over
the whole frame at a focus of 0: the full-screen veil. READ (E69A0 at 1400E79C9 to 1400E7A80, `YebisGetFocusDistance_1416ca2e0`): when the
DoF record's auto-focus word is set (globals `+0x188`), the driver writes the focus distance the library measured
(`[S+0x1F8] + slot x 0x48 + 0x34`) into `+0x1B0` each frame; natively the auto focus was refused since YB-4b (YEBIS-SECOND-TRANCHE-READY
§2.5, §4.3: « the focus distance at the record word 2 »); this cut's records ask no auto focus (« Tranche 9 »). The kept record
also shows the original fading the shaft in over the cut's first 150 frames or so (record words 13 to 16 from 0.003 to 1, words 6 and 8
moving), natively at its end values at once (INFERRED: the library's own onset; not read, left).

**The later cuts' excess** (the same runs, YAVG at 20 to 60 s; `i141-t12b` with `rt-local-x1.flag` and `no-yebis-shafts.flag`): native at
x2 63, 72, 96, 53, 46; native at x1 52, 57, 71, 46, 41; the original 48, 55, 57, 30, 32. Most of the excess is D-0264's local lights at twice
DS3's strength (Thomas's choice of 2026-11-09 for the RT), **provisional until workstream 12** (Thomas, 2026-11-16 01:30 Paris, the project chat: « Pour les lumières locales à x2, c'est provisoire en attendant le chantier 12 »): not to correct in I-0141; what is left at x1 (up to 16 at 50 s) is not read.

## Tranche 9: the cutscene camera's focus distance (2026-11-15)

**The library's auto focus, read and withdrawn.** A `ghidra-research` reading (GhidraMCP and the headless mirror, 2026-11-15) read the
library's auto focus (the driver's setters, the update `FUN_1416E1680`, its depth query and ring); a native port was built
(`c82b6504c`, its step review's fixes `6a225e69b`) and **withdrawn** (`7f2211a71`, `f45170b62`): the witness `i141-t13a-c45000030-nat`
(23:03, `build-t13`, exit 0, restored) showed that 45000030's DoF records ask no auto focus (`native_yebis_auto` false; the records
`{245000030, 101}` and `{245000030, 201}` hold DofFocusDistance 0 and 1, `param_drawparam_s45_00_0030.tsv`), and no shipped gparam
record sets the auto-focus Enable (the third Enable of the Dof group is 0 in the 243 records of `evidence/I-0066/queries/gparam-all`):
code no file reaches. The context's scripted override (+0x413D, +0x4140) has no writer in the decoded image (the same reading: the
constructor and the per-frame latch only; INFERRED never set in retail).

**The cause, READ** (GhidraMCP, the pilot): the third writer of the focus distance 144962120 is `FUN_140017ea0(use, distance)` (the
distance in XMM1), called only from the camera update `FUN_14062ddf0` when the camera's byte `+0xC1` is set, with
the camera's own distance `+0xC4`, or `+0xD0` with `use` = `+0xCC == 0`. It writes the library's post object's byte `+0x2271`
(`[[144961B50+0x4DD8]+0xCB8]+0x2271`) and, when `use` is 0, 144962120 and 144961DB0; the DoF converter E4890 writes the record's word 2
there only while `+0x2271` is set. So the original's 3.1 to 3.8 m in the cut is the game camera's own focus; the camera in play asks
one too (OBSERVED in `i141-t14a`, both modes: about 3.08 m, the original's `S+0x6C0` 3.07 to 3.08 in play while its record says 1). Natively the library's post
object is not made: `FUN_140017ea0` returns without a write, and the converters' scratch object held `+0x2271` at 1, so the record's
1 m in play and 0 in 45000030's first cut stood. **Built** (`60760893a`): `FUN_140017ea0` detoured in native runs (`NativeExternalFocus`, then the original), the
request kept and given to the scratch object's `+0x2271` and, when the camera gives its distance, written into the two globals after
the converter, as the original does. INFERRED: the post object's `+0x2271` kept between scenes as the native keeps it (the post
object made again at a load would put it back to 1). Observers for the witness (`shaft-observe.flag`): `native_dof_focus` (the DoF
keys, weight and words natively), `reference_dof_blend` (the original's record blend). **In play too**: the native's focus moves from
the record's 1 m to the camera's 3.08 m (OBSERVED in two maps, m40 and m45; INFERRED elsewhere from FUN_14062ddf0), toward the original's, for the depth of field and the shafts' mask; the I-0140
sheets `sheets-e31/` (the shafts' brightness for Thomas's eye) predate it. The in-play pictures after it: the local lights' A/B
(below). `build-t14` (`build-t14d.log`: exit 0, the auto focus's port removed).

**The witness** (`i141-t14a-c45000030-nat/orig`, 00:26 to 00:35 on 2026-11-16, the ASUS 1920x1080 window, offline, no measure,
`build-t14` = `622af7c38` (`build-t14d.log`: exit 0); `tranche-6/run-t14.ps1`, `runs-t14.log`; both exit 0, restored). OBSERVED (within the witnesses' row caps: `native_dof_focus` its first 373 rows of the cut, `native_shaft_depth` down to 3.70, the
original's `reference_shaft_depth` down to 3.67): the camera asks its own distance (`native_dof_focus` `camera` [0, 3.80 ... 3.64], then
4.90) where the cutscene file's records hold 0 (`{245000030, 101}`), and the native's focus (`native_shaft_depth`) follows it from 3.80
down, as the original's `S+0x6C0`. **The cyan
veil is gone**: YAVG at 2, 5, 10 and 15 s natively 14.7, 47.1, 49.2, 29.7 against the original's 12.7, 44.6, 48.3, 32.1 (before: 86.1, 98.4,
100.2, 87.4; `tranche-6/yavg-t14.tsv`); `tranche-6/c45000030-veil-before-after-original.jpg` (the native before, after, the original; 5 and 10 s). The later cuts
(20 and 30 s: 63 and 72 against 48 and 56) keep the excess of D-0264's x2 lights, provisional until workstream 12 (above).

## Reviews at the merge of tranches 8 and 9

**The step reviews** (fresh `step-reviewer`s): the auto focus's port (`c524d2084..c82b6504c`): H1 the probe pass outside the frame graph's
plan key, H2 the update never started with a record's distance of 0, M1 a lost request froze it, M2 its velocity reset dead, Lows;
fixed (`6a225e69b`), then the port withdrawn (« Tranche 9 »). The camera focus fix (the net `c524d2084..f45170b62`): no High, no Medium;
L1 the detour outside the start's own hooks, L2 the distance written only with a DoF record, L3 the request not in the witness row, L4
the witnesses' dedup on a moving weight, L5 the thread-affinity scope: fixed (`622af7c38`).

**The final review** (a fresh `final-reviewer` at xhigh on `main...47fb8bfb6`, briefed under D-0283): no High. **M1** the focus request
called a cutscene camera's where the camera in play makes it too, every in-play frame changed unrecorded: the code's comments and this
record corrected (« Tranche 9 »: in play too), the in-play pictures given to the local lights' A/B, Thomas told the light shafts'
sheets `sheets-e31/` predate it; **M2** three witnesses holding statics outside the thread-affinity scope: added, reviewed
(`09eeba5a5`); **M3** this record's tail and ROADMAP stale: rewritten (the Conclusion, « The focus found », Uncertainty, Evidence,
Artifacts, Suggested next step; ROADMAP's item 7). Low, fixed: **L1** the witness's figures within the rows' caps; **L2** the shaft gates'
comment; **L4** the shaft witnesses do nothing past their caps; **L5** the request read once a collect; **L6** STUDY-SILENT-CALLS,
GX-DEPENDENCIES, YEBIS-SECOND-TRANCHE-READY and `yebis_shafts.h` corrected; **L7** the x2 lights said one way (the A/B's measures decide
whether x2 stays until workstream 12), main's D-0230 text kept at the merge; **L8** the builds cited (`build-t14d`), the stale test
files removed from `build-t14/`, `run-t14.ps1`'s header, `yavg-t14.tsv`. Left, with the reason: **L3** the luminance gate on the
frame's `+0xDC` (Uncertainty, INFERRED equal where `+0x244` is positive); **L9** no unit test of the gates or the request (Uncertainty:
they live in the module's collect and prepare; the witness is the run); **L10** I-0138 tranche 7's sheet on this branch, the same as on
`claude/r-i138-t7` (no conflict).

**The step review of the final fixes** (a fresh `step-reviewer`): **M1** three new counters invisible to the thread-affinity checker
(it skips a line holding « static const »): each on its own line, reviewed entries added; L1 two comments still said « a cutscene's
camera »; L2 the keeper's witness stopped only when both caps were reached: each section its own cap; fixed. The checker's blind spot
for the about 30 older statics sharing such a line is left to its own reviewed step (not this branch's). The step review of that fix (a fresh `step-reviewer`): no High, no Medium; L3 the keeper mutex's allow-list text stale: fixed; left: L1 four guarded reads a frame still done by the keeper witness while only its depth section is open (flag-only, original runs), L2 witness statics still sharing a line where no `static const` sits (the checker sees them).

**The integrity review** (a fresh `integrity-reviewer` on `main...28b7f4f68` with this record): no High, no Medium; six Low, fixed in
this record: « in every map » OBSERVED in two maps only; the veil's focus « 0, then 1 m »; 373 rows, not 355; t9a's « disabled » twice;
the checker's blind spot (above); the final tree built, not run live.

**The merge** (2026-11-16): `3f4442e26` into `main` (`Test-Project -MergeGate` passed before); the launcher's module rebuilt in the main checkout (`evidence/I-0141/artifacts/build-launcher-3f4442e26.log`: exit 0, 192 PASS lines); the start check `start-3f4442e26` passed on the ASUS (05:00, restored), the first run of the final tree.

## Evidence

- Tranches 8 and 9: `evidence/I-0141/artifacts/build-t8.log`, `build-t9.log`, `build-t11.log`, `build-t12.log`, `build-t13.log`,
  `build-t13b.log` (overwritten on 2026-11-20 09:55 by tranche 13's own incremental build of that name: the tranche-9 log is lost,
  the final review of tranche 13's M3), `build-t14.log`, `build-t14d.log` (the witness's; overwritten on 2026-11-20 13:17 by
  tranche 14's build of that name, lost: tranche 14's integrity review, M2), `build-t15.log` to `build-t15c.log` and `build-t15d.log` (`build-t15a.log` and
  `build-t15b.log` and their folders overwritten on 2026-11-21 02:18 and 02:23 by tranche 15's own builds of those names, lost:
  tranche 15's final review, M4; the final tree, built with its tests, not run live:
  its changes after the witness are the witnesses' caps, the request read once a collect and comments); `tranche-6/run-t8.ps1`,
  `run-t11.ps1` to `run-t14.ps1`, `runs-t8.log` to `runs-t14.log`, `c45000030-veil-before-after-original.jpg`, `yavg-t14.tsv`; the runs
  `i141-t8a-c45000030-nat`, `i141-t8b-c32000010-nat-nolight`, `i141-t9a-c45000030-nat`, `i141-t10a-c45000030-orig`,
  `i141-t11a-c45000030-nat/orig`, `i141-t12a-c45000030-nat/orig`, `i141-t12b-c45000030-nat`, `i141-t13a-c45000030-nat`,
  `i141-t14a-c45000030-nat/orig`.

- Tranches 6 and 7: `evidence/I-0141/artifacts/build-t6b.log`, `build-t7b.log`, `build-t8.log` (the final review's fixes);
  `tranche-6/` (`run-t6.ps1`, `run-t6b.ps1`, `runs-t6.log`, `runs-t6b.log`, `runs-t7b.log`, the paired sheets and `yavg-t5a-18-41.tsv`);
  the runs `i141-t6a-c32000010-orig`, `i141-t6a-c32000010-nat-no0282`, `i141-t6c-c32000010-nat`, `i141-t6b-c45000030-nat/orig`,
  `i141-t6b-c51000010-nat/orig`, `i141-t7a-c32000010-nat`, `i141-t7b-c45000030-nat/orig`.
- `evidence/I-0141/artifacts/build-1.log`, `build-2.log`, `build-3.log`, `build-4.log` (the reviews' fixes with tranche 5c:
  exit 0, 179 PASS lines, 79 executables); `evidence/I-0141/artifacts/tranche-1/runs-t1a.log`,
  `runs-t1b.log`, `runs-t1c.log`, `runs-t1d.log`, `triptychs/`, `triptychs-b/`, `timelines/`, `exposure/`.
- The runs under `evidence/I-0066/artifacts/`: `i141-t1a-c39000020-nat`, `i141-t1a-c32000010-nat`, `i141-t1b-c39000020-nat`,
  `i141-t1b-c32000010-nat`, `i141-t1c-c39000020-nat`, `i141-t1c-c32000010-nat`, `i141-t1c-c39000020-orig`,
  `i141-t1c-c32000010-orig`, `i141-t1d-c32000010-nat`; the E pilot's `i140-e16-*` and `i140-e17-*` (the original's and the native's before).
- Tranche 2: `evidence/I-0141/artifacts/tranche-2/runs-t2b.log`, `timeline-t2b-nat.png`, `timeline-t2b-orig.png`,
  `sweep-t2b-nat.png`, `sweep-t2b-orig.png`, `pairs-t2b-orig-left-native-right.png`; the runs `i141-t2b-c32000010-nat` and
  `i141-t2b-c32000010-orig`; the build `evidence/I-0138/artifacts/build-t5e.log`. The second observation: `run-t2c.ps1`,
  `runs-t2c.log`, `ceremony-t2c-nat.png`, the run `i141-t2c-c32000010-nat`, the builds `build-t5e3.log` (the run's) and
  `build-t5e4.log` (the last fixes: exit 0, 179 PASS lines).
- Tranche 3: `evidence/I-0141/artifacts/build-t3.log` (`00a17c62f`: exit 0, 179 PASS lines, `glow_bank_test: passed (66 checks, 0
  failures)`; the witness's build, its folder since rewritten by `build-t3b`), `build-t3b.log` (`b555cf4a9`, the optimisation pass:
  exit 0, 179 PASS lines, 66 checks), `build-t3c.log` (the final reviews' fixes: exit 0, 179 PASS lines, `glow_bank_test: passed (68
  checks, 0 failures)`); `evidence/I-0141/artifacts/tranche-3/run-t3.ps1`, `runs-t3.log`, `ceremony-orig-before-after.png`, `kneel.jpg`,
  `levels-i141-t2c-c32000010-nat.tsv`, `levels-i141-t3a-c32000010-nat.tsv`, `levels-i141-t3a-c32000010-orig.tsv`; the runs
  `i141-t3a-c32000010-nat` and `i141-t3a-c32000010-orig`.
- Tranches 4 and 5: `evidence/I-0141/artifacts/build-t4.log` (`9e1dc8917`: exit 0, 179 PASS lines, 68 checks; the first witness's),
  `build-t5.log`, `build-t5b.log` (earlier builds of the `build-t5` folder, 68 and 69 checks), `build-t5c.log` (`5d20ef4b6`: exit 0,
  179 PASS lines, `glow_bank_test: passed (69 checks, 0 failures)`; the witness's binary), `build-t6.log` (the final review's fixes);
  `tranche-4/run-t4.ps1`, `runs-t4.log`; `tranche-5/run-t5.ps1`, `runs-t5.log`, `levels-i141-t5a-c32000010-nat.tsv`,
  `levels-i141-t5a-c32000010-orig.tsv`, `quarter-luma.tsv`, `kneel-t5.jpg`, `t5-orig-t3-t5.jpg`; the runs `i141-t4a-c32000010-nat`,
  `i141-t5a-c32000010-nat`, `i141-t5a-c32000010-orig`.

## Tranche 10: the local lights' A/B, and DS3's own strength by default (2026-11-16, D-0293)

Thomas's « D'accord » (2026-11-16 01:31, to the R thread's proposal): an A/B of the local lights at x1 (DS3's own strength), at x2
(D-0264's default since 2026-11-09) and the original, the measures deciding whether x2 stays until workstream 12.

**The runs** (`i141-ab-*`, 05:29 to 06:16 on 2026-11-16, the ASUS, offline, no measure; `build-t15` = `main`'s native tree; the
runner `tranche-6/run-ab.ps1`, its log `runs-ab.log`; ten native runs, every one exit 0, no fault, the saves restored by hash): the
tour's rows z01 (Firelink Shrine), z25 (Cleansing Chapel) and z36 (Church of Yorshka) at x1 and at x2 (Run-I139Tour, its arrival and
look sweep; the originals the tour's `i139-t1-<row>-orig`), the ceremony 32000010 at x2 and x1 (run-t6's protocol; the original
`i141-t6a-c32000010-orig`) and 45000030 at x2 and x1 (the original `i141-t14a-c45000030-orig`). The levels: ffmpeg's YAVG of 255
(`tranche-6/ab/`: `Get-AbLevels.ps1`, `yavg-ab-zones.tsv`, the game window's client only: the tour's shots are the whole desktop;
`c45000030-orig-x2-x1-levels.txt`, `c32000010-orig-x2-x1-levels.txt`, `sweep-*.tsv`; the sheet `ab-lights-orig-x2-x1.jpg`).

OBSERVED:
- **Firelink**: no picture; the travel stayed on its menu in all three runs, the tour's original too. Left out.
- **Cleansing Chapel** (ten pictures): the original 33 to 52; x1 under it by 4.9 on average (0.2 to 7.5), x2 over it by 7.2 (3.6 to 13.8).
- **Church of Yorshka** (ten pictures): x1 over the original by 9.7 on average, x2 by 15.5; the native brighter there even at x1
  (another cause, not read).
- **45000030**, its cuts at 20 to 60 s: the original 30.6 to 57.8; x1 over it by 0.9 to 15.0, x2 by 14.8 to 38.7 (the statue at 40 s:
  57.8, x2 96.5, x1 70.7); at 5 to 15 s the whole picture puts x2 nearer (5 s: 44.6, x2 46.1, x1 40.7; 10 s: 48.3, 48.8, 42.3;
  15 s: 32.1, 31.9, 33.5), the centre x1 (5 s: 49.7, x2 69.3, x1 51.9; 10 s: 54.4, 68.2, 54.5).
- **32000010 in play after the ceremony** (Archdragon Peak, the sweep's 38 to 99 s): the original about 86, x2 about 81, x1 about 79:
  the one place x2 is nearer, by about 2, within the runs' own spread (72 to 84 for one build in « The witnesses of tranches 6 and 7 »).
  32000010's own shots are not paired (the flash at 26 s in the original, earlier natively): not read for this A/B.
- **The ceremony's smoke** (28 to 34 s of the sweep): x1 89 to 154, x2 96 to 141, the original 55 to 119: the local lights do not
  explain it.
- On the sheet both natives stay warmer than the original (its cool ambient: STUDY-ALL-GAPS row 4, workstream 12).

**Decision** (D-0293, the measures deciding as Thomas agreed): x1 is nearer the original in every paired place but one,
where x2 leads within the noise; the local lights take DS3's own strength by default. `rt-local-x2.flag` (added to the runner's
flags, `Run-StartupTrace.ps1`) and `rt-local-x4.flag` stay test tools; `rt-local-x1.flag` is the default's own and changes nothing. Thomas told in the R thread
(06:20) with the sheet; his answer (the project chat, 06:35, relayed), then in the R thread (06:45): « Oui pour les deux » (D-0293, D-0294); « j'avais dit x1 [x2] car le jeu me semblait sombre sans fill
light ... c'était qu'un artifice en attendant. On garde x1 pour toujours ».

**The automatic exposure's rule** (D-0294; Thomas, 06:33: « ma règle n'était pas la bonne visiblement. Un sous agent Fable 5.2 high
peut trancher »). The tour's bisection (I-0139) put the gate's brighter Firelink still on D-0280's merge `c9cc6bcf8` (the factor log2
-0.46 before, 0 after). A Fable 5.2 high subagent, read-only (its scripts and tables in `tranche-6/ab/exposure/`), found, OBSERVED and
checked by the pilot on the pictures (the start still `ingame.png`, the game window's client): the original 45.9 to 47.5 (55 tour runs;
46 in the two read here), x1 48.6, x2 66.5: **the excess is D-0264's x2 lights**, which D-0264's half-strength darkening hid; at x1 the
still is x1.04 of the original's. The original never adapts (K-0448, READ); its 540 tour pictures' mean grey runs from 19 to 166 of
255, its tenth percentile 0.116 and its median 0.19: the target 0.20 pulls toward its middle and brightens interiors already right
(Church of Yorshka at x1: x1.36 of the original with the lift, about x0.97 at the zone's own exposure, INFERRED from the model
« level ∝ 2^(0.5 Δlog2) », OBSERVED at three pairs). Over the tour's 55 zones no rule of this family tracks the original (the factor
that would match it correlates with the solver's at r = 0.24 to 0.28): the residual is the lighting's. ER's per-area middle grey and
soft clamp (READ, ER-VALUES) do not help with one game-wide target. **Kept**: D-0280's lower bound and the half strength; **the target
0.12**: the exposure brightens only views darker than the original's darkest tenth (two measures compared: the 0.12 is the
plain mean luma of the original's JPEG captures, the kernel's target its own trimmed mean of the luma; at x1 the kernel logged 0.215
where the picture read 0.191, at x2 0.274 and 0.261, not the same frames; INFERRED: Yorshka at x1 about x1.05, the bright
zones unchanged at a factor of 0). Rejected: a band of ±0.5 stop on 0.20 (best on the old tour's numbers, but by darkening the x2
interiors, now gone, and darkening the open-sky zones that match at 0). Not measured: the original's level in Firelink's unlit corridor
(the lift there falls from about +1.07 to +0.29).

## The ceremony's smoke: not the shafts, the effects' light or fog banks (2026-11-16, an observation)

`i141-smoke-c32000010-nat`, `-nat-noshafts` (`no-yebis-shafts.flag`), `-nat-nolightbanks` (`no-effect-light-banks.flag`), `-nat-nofogbanks`
(`no-effect-fog-banks.flag`) (09:32 to 09:59 on 2026-11-16, the ASUS 1920x1080 window, offline, no measure; `build-t7d`, main's tree;
`tranche-6/run-smoke.ps1`, `runs-smoke.log`; four runs exit 0, no fault, restored), run-t6's protocol; the original
`i141-t6a-c32000010-orig`. The sweep's shots aligned by the scene's darkest frame (37 s in the original and without the light
banks, 36 s natively and without the shafts, 35 s without the fog banks; `tranche-6/ab/sweep-i141-smoke-*.tsv`). OBSERVED, YAVG over
the original's 29 to 34 s (55 to 119, mean 73.8): natively 78 to 140 (mean 116), without the shafts 81 to 159 (123), without the light
banks 81 to 163 (123), without the fog banks 79 to 136 (109; its darkest sample 29.5 at 35 s, not a black frame: aligned at 36 s instead, 119): none takes the smoke to the original's level. On the paired pictures
(`tranche-6/ab/smoke-orig-nat-nolight-nofog.jpg`: the native, without the light banks, without the fog banks) the native's smoke is
white and dense where the original's is grey and sparse. So, OBSERVED, none of the three switches brings the frame to the original's
level (the whole frame's YAVG, the walls in it; the runs' own spread about ±7, the variants framed differently); INFERRED, the
excess is in the effect's own draw (its colour, its density or its blend), other causes (the scene's lighting, the tone map) not
excluded. Next: one GPU capture of a smoke frame in each renderer (PIX natively,
the original's draw by RenderDoc or the GX observer) to compare the smoke's draws, their constants and their count.

## The ceremony's smoke: a native GPU capture (2026-11-16, an observation)

`i141-smokepix-c32000000-nat` (18:26 to 18:31, the ASUS 1920x1080 window, offline, no measure, restored; the run ended in an access violation inside PIX's capturer (WinPixGpuCapturer.dll) about native frame 11814, after the four captures, two dumps in its `crashes/`, the runner reporting `EXIT 0` (the integrity review at the merge); its flags `world-pictures`, `effect-light-observe` and `effect-fog-observe` besides `pix-capture`; `build-t8a`, main's tree at
I-0138 tranche 8; [Run-I141SmokePix.ps1](Run-I141SmokePix.ps1), `evidence/I-0141/artifacts/runs-smokepix.log`): run-smoke's protocol
up to the ceremony 32000000, `pix-capture.flag`, F8 at the sweep's second 29 (`Run-AlphaCast -SweepKeyAt`), four captures 40
presented frames apart (`evidence/I-0141/artifacts/smokepix/pix-f8-10828` to `10948.wpix`, 2.5 GB each; the sweep's
`sweep-1-30.png` shows the smoke white and dense). OBSERVED in `pix-f8-10868` (pixtool's event list with the D3D counters,
`events-10868-counters.csv`): the module records no PIX markers; the frame's largest draw by pixel-shader invocations is one
`DrawInstanced` (global id 3500) with 26.6 million invocations (12.8 times the 1920x1080 picture), among the instanced effect draws
3464 to 3513, the smoke's by its overdraw (INFERRED). The tone map at that moment takes the cutscene's area record
`Tone Map[232000000,35]` (exposure 2, contrast 0.5), and the effects' colour scale its exposure (`native_effect_exposure`, 21_effects
« 1.5 / value »); the automatic exposure's factor stays 0 (its log2; the views brighter than the target 0.12). Not compared: the
original's draw of the same smoke, which needs a dump of the original's effect draw constants (no tool does it yet: the GX
observer `ffx-draw-observe.flag` counts the original's draws, RenderDoc never used on this game; since then the blocks dumped in both modes and RenderDoc tried, below) or a replay of the native draw's
constants against the effect's own data. A side observation, to check against the original: through the ceremony the native's
post records fall to the default keys (0xFFFFFFFF: tone exposure 1.52, the glare refused for its shape) for about three frames
every 15 to 25 s (`native_yebis_globals` calls 5366, 6155, 7599, 8417, 9407), perhaps the cutscene's cuts; whether the original
shows those frames is not established.

## The ceremony's smoke: the effects' pixel-shader blocks alike in both renderers (2026-11-16, an observation)

`i141-smokedump-c32000000-nat` and `-orig` (18:50 to 18:59, the ASUS, offline, no measure, restored; `build-t8a`;
[Run-I141SmokePix.ps1](Run-I141SmokePix.ps1) `-Dump`, `evidence/I-0141/artifacts/runs-smokedump.log`): the same protocol, F9 at the
sweep's second 30 (the smoke), I-0113 gap 12's dump of the next 300 effect draws' pixel-shader block (rows `ffx_ps_image`, the hook
264230 in both modes: `ffx-record.flag` natively, `ffx-draw-observe.flag` in the original). OBSERVED: the same blocks in both, by
kind and nearly by count: the main kind (blend 1, `g_vColorScale` (0.375, 0.375, 0.375, 0.5), no fog mode, no soft sprite, no
pre-alpha) 219 draws natively and 217 in the original, then the fogged kinds (blend 2, colour scales 0.75 and 2.25, the same alphas,
fog mode 1, pre-alpha 1) 31 and 39, 6 and 7, 6 and 6; the native's 300 also hold 8 draws (0.75, 0.75, 0.75, 10) and 4 (0.9, 0.9, 0.9,
1) the original's 300 do not (an order of draws that differs between the renderers, or other effects; not established). So the
colour scale (with its exposure term), the blend type, the fog mode, the soft-sprite words and the pre-alpha flag the smoke's draws
take are the original's: INFERRED, the excess is outside them: the vertex colours or the particles' lighting set on the vertex side,
the textures, or the blend state and target the native stage binds. Next: the same dump of the vertex side (the light and ambient
words the effects' vertex programmes read) and of the draws' vertex colours, in both modes.
The tool for it (`9b40fa96c`, built as `evidence/I-0141/artifacts/build-vs1`, `-Cache`, `-NoControls`; run since, widened: « … the smoke's effect constant blocks alike »): the same F9 now
also logs the next 300 `VSCommonBuffer` images whole (0x150 bytes, rows `ffx_vs_image`, both modes: `effect_draw`'s UpdateSubresource
hook natively, `GxUpdateObserved` in the original); `Run-I141SmokePix.ps1 -Dump` with `-Build` that folder, natively then `-Original`.
Not written: the vertex colours (the draw's vertex buffer).
Read again before the run (the fresh R thread, 2026-11-16): `VSCommonBuffer` holds no lighting (matrices, fog start and end, the
fades, the soft-sprite words, the camera: I-0068 FFX-FAMILIES-PHASE2 §2.2); the effects' light colours, hemisphere and ambient SH
are `PSCommonBuffer`'s, of which the `ffx_ps_image` rows logged only the words named above. So F9 now also logs, for each draw it
logs, that draw's `PSCommonBuffer` (+0x4C0, 0x230), `PSLightBuffer` (+0x6F0, 0x190; uploaded for the lit programmes only, an earlier
draw's elsewhere) and `cbFog` (+0x880, 0xE0) as raw 32-bit words (rows `ffx_ps_block`, the same `n`; `97fd47065`, `1653fb6c4`);
[Compare-I141FxBlocks.py](Compare-I141FxBlocks.py) compares the two journals word by word by kind. Built as
`evidence/I-0141/artifacts/build-ps3` (`-Cache r2-cache`, `-NoControls`).

**Step reviews** (Opus 5.5 high, briefed per D-0283). Of `f7375a7f2` and `9b40fa96c`: no high. M1, the original's wrapper hook
logs `VSCommonBuffer`-like blocks of any pass and size that pass the near/far test, the native's the effects' buffers alone, so the
two 300-row windows hold different draws: answered in the comparer (word by word per writer both modes logged); left in the hook
(the original's rows may spend part of the 300 on other writers). L1 (no sequence in `ffx_vs_image`): left, the comparison is by
writer and statistics, not by position. L2 (`reference_renderdoc_layer`'s `d3d11_before` always true; RenderDoc's load before the
device a likely race, not a guarantee): left, the runner now lists the `.rdc` files a run made (L3). L3 (the runner silent on
-Original's and -Dump's outcome): fixed (`f1ac2d5ac`: the rows counted, RenderDoc's row and captures listed). L4 (the runner
removed files it had not placed on an early refusal): fixed. L5 (stale header, the default run name « -nat »): the header fixed,
the run names given each time. L6 (`renderdoc-capture.flag` not refused beside the Reference D3D11 vtable hooks): left, the runner
never combines them. L7 (a `bad_alloc` in the row building caught by `Update`'s handler): left. Of `97fd47065` and `0928c3340`:
H1, the blocks logged as floats and the comparer's 1e-4 floor hid every integer word (blend types, the point-light count) and
small floats: fixed (raw words, integer patterns compared exactly). M1, the comparer crashed on a word with no finite value: fixed.
M2, `PSLightBuffer` stale on unlit draws: stated in the hook and the comparer, its section read with that in mind. L1 (a failed
read logged as zeros): fixed (`read`). L2 (orphan and unreadable rows dropped silently): fixed (counted). L3 (VSCommonBuffer pooled
over writers): fixed. L4 (the animated colour scale in the kind key): kept by design. L5 (allocation in the noexcept hook): left,
as its neighbours. Of those answers (`f1ac2d5ac`, `1653fb6c4`): no high. M1, one zero sample made a float word an « integer »
compared by its most frequent pattern (a light colour 0 in most draws hid a 2x difference in the others): fixed (zeros are not an
integer's sign; a float word compared by its median and its median without the zeros; a section whose read failed left out). L1
(the lit-only note on cbFog's words instead of PSLightBuffer's): fixed (a comment; `build-ps3`'s module unchanged by it). L2 (the
header on mixed kinds): fixed. L3 (a malformed block row aborted the comparer): fixed (checked and counted). L4 (the runner would
overwrite another run's files in the build folder): fixed (it refuses).

## The ceremony's smoke: the smoke's effect constant blocks alike in both renderers (2026-11-16, an observation)

`i141-smokeblk-c32000000-nat` and `-orig` (22:32 to 22:41, the ASUS 1920x1080 window, offline, no measure, both exit 0 without
fault, restored; `build-ps3`; [Run-I141SmokePix.ps1](Run-I141SmokePix.ps1) `-Dump -KeySecond 30`,
`evidence/I-0141/artifacts/runs-smokeblk.log`; the runner's closing row count threw on its quoting, after the runs and their
restoration: fixed since): the same protocol, F9 at the sweep's second 30. 300 `ffx_ps_image`, 300 `ffx_ps_block` and 300
`ffx_vs_image` rows in each journal, none orphaned, no failed read. [Compare-I141FxBlocks.py](Compare-I141FxBlocks.py)
(`evidence/I-0141/artifacts/smokeblk-compare.txt`), OBSERVED: the smoke's kind (blend 1, colour scale (0.375, 0.375, 0.375, 0.5),
215 draws natively, 217 in the original): of `PSCommonBuffer`'s 140 words five differ, words 73, 76, 78, 79 and 81 (+0x124 to
+0x144: the camera's position and direction by their values, -39.5 against -36.6 for the largest, the runs' framing); none of its
light colours, hemisphere, ambient SH, `g_color` or blend words; `PSLightBuffer` and `cbFog` alike word for word; `VSCommonBuffer`
(one writer, 266308, 300 rows in each) differs in 11 words, all in its matrices and camera words (+0x4 to +0x64), not its fog,
fades or soft-sprite words. The other kinds are not alike (the integrity review at the merge, per draw): for (2, 2.25, 1.5), (2, 2.25, 2) and (0, (1, 1, 1, 1), 0, 1)
every native draw's differing `cbFog` words fall outside the original's range, `cbFog` +0x8 9.26e11 [7.28e11, 1.19e12] natively
against 4.16e11 [3.67e11, 4.71e11] (about x2.2, not explained), the fog-mode-0 kind's `cbFog` too, and its `g_color` words 3 and 22
outside the original's range in 4 of 8 draws; the fogged kind (2, 0.75, 1, 1) differs in 24 words, among them +0x160..+0x168,
median 0.505 natively against 1; none of this read further; the fogged kinds' `cbFog` and two `g_color` words vary with the camera and the
draw, alike in range). So, OBSERVED: the constants the game hands the smoke's draws are the original's, but for the view;
INFERRED: the excess is in what the native stage does with them: the textures it binds (their format and colour space), the blend
state and target it draws into, or the vertex colours (the particles' vertex buffer, not dumped). Next: the same frame's draw
state in both renderers, the native's in the PIX capture `pix-f8-10868` (draw 3500) and the original's by RenderDoc
(`renderdoc-capture.flag`, the runner's `-Original` without `-Dump`): blend description, target format, the textures' formats.

**RenderDoc's layer, first try** (`i141-smokerdc-c32000000-orig`, 23:09 to 23:11, the ASUS, offline, no measure, restored;
`evidence/I-0141/artifacts/runs-smokerdc.log`), OBSERVED: RenderDoc 1.47 loaded in the game (its log
`%TEMP%/RenderDoc/RenderDoc_app_2026.11.16_23.09.14.log` registers its hooks, then stops), the game's window never came, the
controller exited 11 (« the game window could not be placed »), the module's journal empty (not even the layer's own row). So the
flag's path (the layer loaded by the module's setup thread after the start) does not start the original; why (RenderDoc's hooks
against the game's protection, or a hang in the loader) is not read; the runner reported `EXIT 0` over Run-AlphaCast's failure
(its exit code is not taken from the throw it logs). The flag is not tried again before its cause is read; another route for
the original's draw state: the module's own D3D11 observer (`OMGetBlendState`, the bound targets and views, as I-0113 tranche 17's
`EffectBlendWitness` reads them) on the F9 rows.

**Kinds the native draws that the original's window lacks** (the same two journals, OBSERVED): the native's 300 F9 draws hold three
kinds absent from the original's 300, in both pairs of runs (`i141-smokedump` and `i141-smokeblk`): blend 1 with colour scale
(0.75, 0.75, 0.75, 1) without pre-alpha (4 draws), blend 1 with (0.9, 0.9, 0.9, 1) (8; 4 in the first pair) and blend 2 (the FFX
additive type) with (0.75, 0.75, 0.75, 10) and fog mode 1 (8). The patch-draw rows (`*_ffx_patch_draw`, 120 a press, the draws'
point counts) agree: natively the kind (0.75, 0.75, 0.75, 10) 4 draws of 116 points and (0.9, 0.9, 0.9, 1) 4 of 39, none in the
original's 120; (0.75, 0.75, 0.75, 1) 131 points natively against 27; the smoke's own kind 161 against 225 points, the DS words
logged alike. INFERRED, a lead, not established: an additive effect with an alpha scale of 10 drawn natively over the smoke and not
in the original would whiten it; a 300-draw window starting at a press may also simply hold a different part of the frame in each
renderer. The tool now logs 3000 draws a press with their tick (`f3afadfb3`, `606d433bc`, `build-ps5`) so that each window spans
whole frames.

## The ceremony's smoke: the native draws effects the original does not (2026-11-16, an observation)

`i141-smokewin-c32000000-nat` and `-orig` (23:46 to 23:55, the ASUS, offline, no measure, both exit 0 without fault, restored;
`build-ps5`; `evidence/I-0141/artifacts/runs-smokewin.log`): the same protocol, F9 at the sweep's second 30, 3000 effect draws
through 264230 logged in each mode with a performance-counter tick. The draws come in bursts, one a frame (gaps over 4 ms): natively
26 bursts over 735 ms (the logging slows the frame), 111 to 122 draws each (one burst of 191 holds two frames); in the original 32 bursts over 511 ms, 95 to 98 each.
OBSERVED, by kind over the whole window: natively three kinds the original never draws in its 32 frames: blend 2 (additive) with
colour scale (0.75, 0.75, 0.75, 10) and fog mode 1, four draws in every frame (100); blend 1 with (0.9, 0.9, 0.9, 1) (68); blend 1
with (0.75, 0.75, 0.75, 1) without pre-alpha (50); and the smoke's own kind about 80 draws a frame natively against 68 in the
original (2069 in 26 frames, 2188 in 32); the other kinds about alike a frame. So the excess has a first cause, OBSERVED: the native
draws more effect draws a frame than the original at the same moment, among them an additive kind with an alpha scale of 10 that
the original does not draw at all. INFERRED, not established: the game's own decision which emitters to draw differs between the
renderers: the native's pass context for the effects (`21_effects.inc`, the pass 0x45 context: the frustum 6E780/6E9E0 built from
the native's camera, the passes 0x1F, 0x29, 0x2D, 0x31 and 0x34 enabled at +0x426) against the original's. Next: name those
effects (the draw commands' entity and key, natively from the recorded commands, in the original from its pass's callback) and
read the condition that drops them in the original.

**The entity test's inputs alike** (READ, then OBSERVED). The draw pass's slot 4, 2997E0 (GhidraMCP, 2026-11-17), runs per entity:
it drops the entity when its resource's masks (+0x1F8 against the scene control's +0x14C, +0x1FC and +0x200 against the pass
context's +0x4B8 and +0x4BC) do not match, when 29AA10 culls it by its draw distances from the pass camera (+0x418's +0x30..+0x38:
the resource's near and far distances, or a draw range of 0x2904 kind), and weighs a near test by the options object's
`[GXRS+0x5360]+0x64` and the settings object's `[GXRS+0x5368]+0x158`. `i141-smokecam-c32000000-nat` and `-orig` (00:26 to 00:35,
the ASUS, offline, no measure, both exit 0, restored; `build-ps7`; `runs-smokecam.log`): the pass camera the same cutscene camera
in both (positions about 2.1 apart by the frame the press fell on, near 0.08, far 10000), the options object's +0x48..+0x64 and
+0x74 and the settings object's +0x158, +0x170 and +0x188 equal (1); the objects differ elsewhere (the options object at +0x10, 1000 natively and 0 in the original, and +0x20, +0x28, +0x6C, +0x78; +0x118 and +0x190 of the settings object 6 and 2 against 8 and 4), words the test does not read. So the extra draws
are not the distance test's inputs: either the entities offered differ, or their masks. The tool for it: `a9844d81a`
(`build-ps8`), each entity offered to pass 0x45 after F9 logged in both modes with its position, masks, distance answer and
whether it was recorded (rows `ffx_entity_offered`).

**More effect entities alive natively** (OBSERVED; one node an entity in both modes, so more instances, see « The instances created alike »). `i141-smokeent-c32000000-nat` and `-orig` (01:54 to 02:03, the ASUS, offline, no
measure, both exit 0, restored; `build-ps10`, the hook's return value kept after its step review; `runs-smokeent.log`): 1500
entities offered to pass 0x45 after F9 in each mode, by their world position (rounded to a unit): 118 distinct natively, 102 in the
original, 101 in both; 17 natively that the original never offers in its window, one the other way. The tests are the same in both:
the scene control's mask `FFFFFFF9`, the pass masks (1, 1), the resources' masks (4, 0x11, 1), (8, 0x11, 1) and (-1, 0x11, 1) in the
same proportions; the 17 extra entities pass them (not culled) and are recorded (a draw command each, every time), 15 of kind 3 and two of kind 6, at y 79 to 99 around the bell (x 54 to 80, z 317 to 343). About 160 entities a frame natively against about 140 and
about 120 recorded against 95, as the draw counts. So, OBSERVED: the native's excess is effect instances the original does not
have at that moment (or has not yet, or no longer), drawn by the same rules; INFERRED, the smoke's whiteness is their drawing. Not
established: whether they are spawned natively and not in the original, or outlive their end natively (an effect a cut should
remove, an emitter whose end waits on something the native does not do). Next: the instances' life, in both modes: their creation
and their release (the library's entity list 1AD6440, the effect manager's create and delete), named by their FXR id.

**The frustum alike; the extra entities lie outside it** (READ, then OBSERVED). The library's walk (24E790's multi path shows the
same test) offers an entity to 2997E0 only when the pass context's culler (+0x410, vtable 143D42D58) classifies its bounds as not
outside: slot 0x60, 6F310, an overlap of the culler's box (+0x220..+0x23F) then six planes (+0x160..+0x1BF); natively the culler is
6E780's, updated each frame by 6E9E0 from the pass camera. `i141-smokefr-c32000000-nat` and `-orig` (02:39 to 02:48, the ASUS,
offline, no measure, both exit 0, restored; `build-ps11`; `runs-smokefr.log`): the two cullers' planes and box agree within the
camera's motion between the presses (the planes' normals to the second decimal; the native's tail words +0x2A8..+0x2F4 zero where the
original holds 2, 0.2, 2048, 32, FLT_MAX, 965100 and 1, words 6F310 does not read). The 17 entities the native offers (15 by rounding the positions, 17 by their floor) and the original
does not (by position, this pair) lie outside both frusta by their centres (most behind all six planes): they are offered natively
because their bounds overlap the frustum, or because something other than this test lets them through. Next: their bounds (2997E0's
box argument, logged since `931a67586`, `build-ps12`) in both modes.

**Not their bounds: entities the original does not have** (OBSERVED). `i141-smokebox-c32000000-nat` and `-orig` (02:57 to 03:06, the
ASUS, offline, no measure, both exit 0, restored; `build-ps12`; `runs-smokebox.log`): 2997E0's box argument is the entity's position
(x, y, z, 1, then stack words), not its bounds, equal in both modes for the entities both offer; and both modes offer entities far
outside the view (at (-92, 21, 120), (-28, 79, 250)), so the library's walk is not a tight frustum cull. The 17 entities only the native
offers (216 of its 1500 rows) sit on a regular layout around the bell: kind 3 at (67.0, 78.5, 319.5), (67.0, 78.5, 334.5),
(82.0, 78.5, 319.5), (53.7, 78.5, 327.9), (68.7, 78.5, 327.9), (79.5, 82.1, 343.4)..., kind 6 at y 98.6 ((74.5, 98.6, 327.0),
(61.2, 98.6, 335.4), (86.2, 98.6, 339.0)). INFERRED: placed effects (the map's or the event's) that exist natively and that the
original does not have during the ceremony (removed, hidden or never started there); their position pins them in the map data.
Next: name them from the map (Archdragon Peak's MSB regions or event effects at those positions) and read what removes them in the
original during the ceremony (an event, the cutscene's act, a display group), then why the native keeps them.
Archdragon Peak's MSB (`map_MapStudio_m40_00_00_00.msb.dcx` of the I0063 cache, SoulsFormats; parts, objects and the 18 SFX
regions) has nothing within 158 units of them nor of the ceremony's camera (75, 83, 336), and no single translation maps the offered
entities onto its regions: INFERRED, the ceremony is staged in its own space and these are the cutscene's own effects (its
timeline's), not the map's. Next: whether the original has them earlier in the scene and loses them (a lifetime the native does not
end) or never has them (a spawn only the native does): the same pair of runs with F9 at an earlier second.

**Absent earlier in the original too** (OBSERVED; the spawn INFERRED). `i141-smokeearly-c32000000-nat` and `-orig` (03:22 to 03:31, the ASUS, offline, no
measure, both exit 0, restored; `build-ps12`, F9 at the sweep's second 12; `runs-smokeearly.log`): 25 entities offered natively
against 17 in the original at that moment; of the 17 extra entities of second 30, 6 are already there natively at second 12 and
one in the original; 9 extra natively at second 12 ((58, 92, 325), (62, 96, 322), (68, 94, 320), (69, 92, 317), four at (79-80,
80-82, 341-343), (200, 126, 393)). So the original does not have them earlier either: INFERRED, the native spawns cutscene effects
the original does not (or spawns them sooner and keeps them), not an end the native misses. Open: which code creates them and on
what condition (the cutscene's timeline, the cutscene set's objects' model effects, a renderer-dependent answer the creation reads).
Next: the effect creation in both modes during the ceremony, by its caller and FXR id (a static read of the creation paths first).

**The creation paths read** (a `ghidra-research` subagent, GhidraMCP and the decrypted-code overlay, 2026-11-17; its scripts' outputs
in the session's scratch, not kept; READ unless said): every effect instance comes through the scene control's 24ED90 (a debug
boolean sc+0x5BE, the library manager) into 141ACCA20 (FXR checks), fed by the Remo timeline's Sfx act (140CCB3B0 → 140CCEF00, one
multi instance a dummy-polygon count of the act's model), the Remo factory (FXR 700000000), ModelSfxParam objects (14065A010) and the
event and map effects; none of these tests GX state ([144961B50]), a device flag, a texture's or model's readiness or a visibility
result; the draw groups they apply come from game logic. One renderer-dependent gate sits one level lower: a node's renderer
(14037D260 and its siblings) is destroyed when its texture lookup through 28F7B0 returns null, which the original does while the
GXTexture2D has no view yet, and a node without a renderer is never offered to slot 4; natively `NativeTexture` returns a token as soon
as the texture entry exists (INFERRED: the original leaves those nodes without a renderer for good where its texture was not ready at
creation, the native draws them). The census to check it: `4d0e74643` (`build-ps13`; no step review yet), every lookup by id and kind
with its non-null answers in both modes, written on F9 (rows `ffx_texture_census`).
Refuted (OBSERVED): `i141-smoketex-c32000000-nat` and `-orig` (04:20 to 04:29, the ASUS, offline, no measure, both exit 0, restored;
`build-ps14`, the census's step review answered; `runs-smoketex.log`): every lookup of a non-zero id answered in both modes (the
original 1947 of 1947 over 40 ids and kinds, the native 1651 of 1651 over 37); the earlier null answers were id 0. So the nodes'
renderers are not lost to a missing texture in the original. Next: the creation itself in both modes (`baf3d2ebc`, `build-ps16`:
24ED90 hooked in both, rows `ffx_create` with the FXR id, the translation, the tick and the callers).

**The instances created alike** (OBSERVED). `i141-smokecreate-c32000000-nat` and `-orig` (04:39 to 04:48, the ASUS, offline, no
measure, both exit 0, restored; `build-ps16`, no step review yet of `4e48b9c32`/`baf3d2ebc`; `runs-smokecreate.log`): 434 effect
creations through 24ED90 natively and 440 in the original over the whole run, the same FXR ids by count but four (4 and 840080 once
in the original only: the native's own drop of the Firelink wall effects, `04_updates_and_test_tools.inc`'s detour of E6DCD0, also the « 0 » frame in every native caller list; 320 seven against eight, 201001 70 against 73), the same callers (Sfx instances E6DD65 → E6A497 → E6B290 →
E6E7E2, and 7CB8B2). Corrected by the final review at the merge (H1): 24ED90 is the door of the scene control's creations only; the instances the library spawns itself (its FXR actions, through 141ACCA20 from GXFfxManager's slots 5 to 7, 1421551C0, 142155310, 142155460: I-0068 EFFECTS-ENTRY-DOOR-NATIVE) never pass it, and the journals' `native_ffx_entity_slots` and `reference_ffx_draw_list` show one node an active entity in both modes (natively 560 and 560, in the original 1002 and 1002 in their samples). So, OBSERVED: the door's creations are alike; INFERRED: the extra entities are instances the library spawns natively and not in the original, or keeps longer. 24ED90's six arguments read in its disassembly (GhidraMCP, 2026-11-17): the fifth and sixth as dwords at [RSP+0x4F0] and [RSP+0x4F8] after a 0x4B0 frame and three pushes, the handle (RDI) returned: the hook's signature holds. The +0x30 row of its matrix is not the translation (rotation terms). Every per-frame comparison above was taken under the 3000-row F9 dump, which slows the native to about 28 ms a frame against 16 in the original: a count a frame, or the update step, is to be taken without it. Next (the next R thread): 141ACCA20 hooked in both modes, by caller (24ED90 against the library's slots) and FXR id, the entity rows armed without the block dump, then the cause.

## Reviews at the merge of the smoke's tools (2026-11-17)

The final review (Opus 5.5 xhigh, on `main...a0d38c026`, briefed per D-0283): H1, « the native does not create more instances, the
extra entities are nodes inside them » not supported (24ED90 sees the scene control's creations only; one node an entity in both
modes): corrected in « The instances created alike » and STATE. M1, 24ED90's signature unverified: read in its disassembly (six
arguments, the two on the stack read as dwords, the handle returned), recorded there; no separate step review of `4e48b9c32` and
`baf3d2ebc` nor of `63163931f` beyond this one. M2, stale and contradictory text: the headings corrected, the vertex tool and the
capture paragraphs marked as overtaken, « Suggested next step », « Uncertainty » and « Artifacts » rewritten. Lows: the counts
corrected (15 and 2 of kinds 3 and 6, 17 entities by floor, 111 to 122 draws a burst, 95 to 98, 2.1 apart, the fogged kind's 24
words); the two FXR ids the original alone creates named as the native's Firelink wall drop; left, with their reason: the Reference's
`recorded` reads a wrapping index (22 of 1500 rows -63), the `ffx_create` field `t` (its +0x30 row, said in the RESULT), the runner's
`EXIT 0` over a failed Run-AlphaCast and `renderdoc-capture.flag` kept (both in « Uncertainty »), the per-frame comparisons under the
dump (said with the next step), the nits (a guarded read before the row counter, `HexWords` beside `HexBytes`, two `ffx_vs_image`
builders, the census under `ffx-draw-observe` natively, an empty census row under `ffx-series` alone), the creation-path reading's
scratch outputs not kept (its claims stay READ by the subagent, to reread before a fix rests on them), tranche 8b's runner removing
files it did not place. All are diagnostic tools behind their flags; nothing reaches play.
The integrity review (on `main...a4849be6f`): no weakened test, gate, tolerance or special case, no work cut during a measure, the
figures it sampled reproduced but these: M1, the PIX capture run's crash in PIX's capturer after its captures, unreported: now stated; M2,
« the smoke's effect constant blocks alike » beyond the smoke's own kind: narrowed, the other kinds' differences listed as unexplained; L1, tranche
8b's « sixteen times » from another run: corrected in I-0138's RESULT; L2, the capture run's other flags: stated; L3, the options words
listed in full; the smoke kind's « 68 a frame » counts a partial burst (whole frames hold 70 to 71): left as an approximation.
Merged `50a022a46` (2026-11-17), the launcher rebuilt (`build-launcher-50a022a46.log`: exit 0, 179 PASS lines), start check
`start-50a022a46` passed (restored); the disk rotation (D-0260): 1035 files, 7.6 GB of raw frames older than 7 days, 203.1 GB free on C:.

## Tranche 11: the library's creations, the smoke's extra entities, the wheel, the Rekindled vignette (2026-11-17)

**The library's instance creations alike** (OBSERVED). The witness `d5c8d31e6` (`build-ps19`): 141ACCA20, the library's instance
creation, hooked in both modes (rows `ffx_spawn`, and on the press a tally `ffx_spawn_tally` by caller and FXR id since the start;
F13 a light press without the vertex and block rows). Its six arguments read in its disassembly (four in registers, the matrix at
[RSP+0x28] and the pre-roll at [RSP+0x30] on entry). `i141-smokespawn-c32000000-nat` and `-orig` (08:27 to 08:36, the ASUS,
offline, no measure, both exit 0 without fault, restored; `runs-smokespawn.log`), compared by `Compare-I141Spawns.py`: 428 and 434
calls up to the press, every one from 24EE0D (the scene control's 24ED90) in both modes, the same FXR ids by count but the four
already known (201001 69 against 72, 320 4 against 5, 4 and 840080 in the original only); no call from the library's own slots 5
to 7 in either mode up to the press; in the 20 s before it the same creations (4040 twice, 732035 24 times, 732200 once). So no
extra instance is created natively through 141ACCA20 (the entities inside the instances not counted by it).

**The smoke's extra entities, not offered twice** (OBSERVED). The first reading, from `build-ps19`'s rows without the entity's
address (`i141-smokespawn-*`), counted kind 3 with mask 4 at 179 offers a frame natively against 75 to 77 (by the frames' cut): those counts hold; its reading
of 81 floor entities « offered twice » from equal positions does not. The witness `f8dc92b52` and `aa73b0ca6` (`build-ps23`; each
`ffx_entity_offered` row with the entity, its vtable, its object, the pass context, whether it is the native's and its two callers;
`ffx_pass_context` once a session) in `i141-smokedup-c32000000-nat` and `-orig` (09:08 to 09:18, the ASUS, offline, no measure,
both exit 0 without fault, restored; `runs-smokedup.log`): every offer from the same two callers (1ADE046 under 1ADB142) and the
same vtable (3A9CA60, the entity at its object's +0x148) in both modes; a frame is two walks of slot 4 (natively 170 and 73 to 89
offers, in the original 157 and 49); no entity is offered twice in a walk in either mode, and only 4 (2 in the original) in both
walks; the pairs at one exact position are two entities with two objects in both modes (73 positions in a first walk natively, 68
in the original). The pass context is one natively; the original's journal holds four, not alternating by frame. Over the rows'
window 227 distinct kind-3 entities natively against 179. The two modes' samples are not at the same moment of the scene (the
final review, measured from the first creation of FXR 732035, the scene's own, in each run): the press natively at 18.00 s against
20.55 s in the original in `i141-smokedup`, 18.09 s against 18.92 s in `i141-smokespawn`; the original's count of kind 3 with mask
4 a frame rises from 75 to 77 to 139 between its two samples, the native's stays 171 to 186. So at the closer timing the native walks
about 100 more smoke entities a frame, not the same ones twice; the smokedup pair's « about 35 » is the offset's. Next: the rows
armed from the scene's own clock (a time after the first 732035 creation) so both modes are sampled at the same moment, then the
extra entities' age across frames (a lead, not established: kept alive past the original's end of their life).

**The wheel on the Advanced page** (Thomas, 2026-11-17: the wheel did not scroll the options' side bar). `d5c8d31e6`, `5e04ff07a` and
`a0908d36d`: the notches counted from WM_MOUSEWHEEL in the input hook while no page blocks it, the page's window moved by one row a
notch, clamped to the list, the cursor kept on its row of the list (`ScrollByWheel`, unit-tested). `i141-t11-wheel` (08:36 to
08:40, same build and screen, exit 0, restored): each notch moved the window by one row, 1, 3 down (1 to 4: the end of 18 rows in
14 slots), then up to 0, the side bar at the new place each time (rows `wheel` and `scroll_bar`), the cursor on the same list row
(Sharpening, list row 7) throughout, no setting changed; the still after the twenty notches down shows the list's end (Vignette)
and the bar at the bottom (`options-step-12.png`). The game's own pointer events read the notches too (`ui_event_pointer`
wheel 1) without acting on the page (the first step review's M1: answered by this run).

**The Rekindled vignette** (Thomas, 2026-11-17: « vignettage trop sombre » with the Rekindled look; no slider, the On/Off row kept;
08:28: a Sonnet subagent to re-check its value in ER; 08:32: « Sinon mets celle de DS3 d'origine »). `de48c51db`, `9ae31a2cc`,
`787019a29`: the candidate's factor eased toward none, f + e (1 - f), by one constant `look::kErVignetteEase` (0.5, PROVISIONAL;
the corner's factor 0.096 on the curve's output before the lift and the encoding becomes 0.548), the block's unused third grain
word its place (240 bytes kept, the layout test updated); a test flag `vignette-ease-cycle.flag` (the Rekindled look forced, with
`world-pictures.flag`) shows on successive pictures no vignette, ER's as read, eased 0.35, 0.5 and 0.65, and DS3's own on the
exposed scene (`kPostToneErDs3Vignette`, the area's record's, test only), rows `native_vignette_sheet` with whether the vignette
is wanted; `Make-I138LookSheet -Vignette` makes the sheet; Run-I138HdrMenu's `vignette` run. Step review (Opus 5.5 high, on
`bd3bf1948..de48c51db`): no High; M1 the grain copy of `scene_pass_frame.cpp` still three words (fixed: by its size); M2 the sheet
row said the step asked, not whether the vignette was wanted (fixed: `wanted`, the sheet refuses a run without it); L1 the sheet
step outside the thread-affinity scope (fixed: in it, its atomic reviewed); L2 the factor's place in a comment (fixed); L3 two
writers of the ease (fixed: the sheet's override only). Second step review (on `de48c51db..aa73b0ca6`, the DS3 step and the
smoke's witness): H1 the stage rebuilt the block's flags without bit 7, so the DS3 tile would have shown no vignette (fixed in
`post_tone_stage.cpp`, with a stage test: the ER body with bit 7 against without it, 2.5 million channel values darker and none
brighter on the card and on WARP, `build-ps22`); M1 the DS3 tile labelled where the area has none (fixed); L1 the picture cap of 8
(16 under the sheet's flag); L2 an older run's rows refused by their own message; L3 EntityOffered's counter reviewed; L4 the
shader's comments; L5 the vtable's RVA bounded: fixed. Third (on `aa73b0ca6..fa54fb459`): the build broken by a comment that
swallowed a declaration (fixed in `fa54fb459` before it was reviewed); M1 the DS3 tile's label decided for the whole run (fixed:
rows `native_vignette_sheet_ds3` by frame); M2 the « without bit 0 » check could not fail (fixed: the coordinates kept, bit 0
cleared); L1 the RVA bound a constant, not the image's size: left, the file's convention for every RVA it writes; L2, said here:
the glare's bright pass is vignetted by the area's record in every step of the sheet (GL-4), so its tiles without the vignette
still carry it on the bloom's input.

**DS3's vignette in the Rekindled look** (Thomas, 2026-11-17 09:05: « Sinon on reprend la vignette de DS3, c'est sûrement mieux vu que
ça dépend des zones » and, on the pilot's agreement, 09:06: « Je suis d'accord »; no adapter beside it, Thomas 2026-11-12). `f4e47e95b`:
the candidate after Elden Ring applies the area's own vignette, bit 0 of the block (the record's Vignette Power, none at m32, m37
and m53, and the player's row), on the exposed scene before its curve with the bloom not vignetted, as the original's programme
(`ToneColour`); Elden Ring's vignette is removed with its ease and the sheet's test tools (`er_vignette`, `er_vignette_ease`,
`kPostToneErDs3Vignette`, `look::ErVignetteWords`, `look::ErVignette`, `vignette-ease-cycle.flag`), the block back to 224 bytes
(the automatic exposure's `tone_rest[6]`), `ApplyToneLook` without its aspect and vignette arguments. The stage test: the area's
vignette (power 0.3) in the ER body against the same draw without one, no value brighter, many darker, the centre the same, and
its coordinates without bit 0 the draw without one (`build-ps24`, incremental, and `build-ps25`, the complete build of the merged tree: 179 PASS each; 2.49 to 2.58 million channel values darker and none
brighter, the coordinates without the bit drawing none, on the card and on WARP). Step review (Opus 5.5 high, on
`847157319..fc53450c6`): no High or Medium; L1 the stage test's head still described a vignette in the ER models (fixed, comment
only); L2 the new check covers the 8-bit entry without bloom, so neither the bloom left unvignetted nor the HDR gain's input
with the factor is tested: left, both read in the code (`ErScene`, `ToneColourEr`), a test for a later tranche. The sheet's run `i141-t11-vignette` (`build-ps23`, before this change; 09:18
to 09:20, Firelink, exit 0, restored): Make-I138LookSheet refused it (no `native_output_extent` row under the gbuffer pictures), so
its pictures were read by frame against the `native_vignette_sheet` rows; the luma (ffmpeg signalstats YAVG, of 255) at the
centre 44.3, 43.9, 43.8, 44.0 and 43.9 for ER as read, eased 0.35, 0.5, 0.65 and DS3's own (`native_vignette_sheet_ds3` record
true), the four corners 46.5, 23.5, 42.4, 43.3 with ER as read and 55.4, 27.6, 60.1, 58.8 with DS3's (near ER eased 0.5); the
first picture (no vignette, frame 2400) is darker everywhere (centre 25.5: the exposure not yet settled) and is not compared.
Shown to Thomas: `vignette-er-vs-ds3.png` (evidence/I-0141/artifacts/vignette-sheet). The sheet's test tools are removed with
the ER vignette (the scripts back to `bd3bf1948`).

**ER's vignette re-checked** (a Sonnet 5.5 subagent, read-only, headless Ghidra on its own mirror of the ER project; its scripts and
outputs in evidence/I-0141/artifacts/er-vignette-recheck). OBSERVED in ER 2.7.1's disassembly: FUN_141b8e7d0 writes (A aspect F, B F,
Power / Range, 1/2 - 1 / Range) into cbToneMap's g_vVignettingParam (CB+0x120), F = (tan(fov / 2) - 1) Fov Dependence + 1
(tan INFERRED); its shader takes smoothstep(t)^4 of the Euclidean length on the raw uv, on the table's linear output before the
display gamma: the formula the project had. OBSERVED in the gparams: Power keyed by the hour (m60 0.39 at 4 to 8 h, 0.43 at noon,
0.5 from 15 h; default 0.4, Range 1.2, m60's 1.3), median 0.49 over 227 files: the project's 0.5 and 1.25 were the evening's value
all day; ER's corner at 16:9 on the encoded picture about 0.53 to 0.64 by day, 0.34 at night. Corrected by it: I-0138's « Power
raised to fov / FovDependence at CB+0x110 » is the feedback blur's word (g_vFeedBackBlurParam), not the vignette's; the factor
0.096 is on linear light, not on the encoded picture. INFERRED, not read: the gparam rows' copy into the pass's words, the time
interpolation. Moot for the project since DS3's vignette replaces it (above).

**Moonlight Vortex** (E's audit: the glare streak and the green ground natively). `i141-t11-mv-noglare-thomas-arts-nat` (the bloom
row at 0) and `-nolight-` (`no-effect-light-banks.flag`), 09:19 to 09:27, `build-ps23`, exit 0, restored: made, not yet read beside
`i140-e89-thomas-arts-*` (the casts' full-desktop stills at 900 ms do not show the vortex large enough: crops needed). Thomas, 09:23,
watching the run: « Le weapon art de Moonlight Greatsword s'affiche correctement à première vue ».

**Reviews at the merge** (tranche 11, on `main...4e85dfeb2`, `main` `ecdc3cb5b` merged in, `build-ps25` complete: 179 PASS). The
final review (Opus 5.5 xhigh): no High. M1 the smoke's « about 35 more » compared the modes at different moments of the scene, and
called wrong the first reading's correct counts (fixed in the records: « The smoke's extra entities », STATE, with both pairs'
offsets; the same-moment sampling the next step); M2 this RESULT's Uncertainty, Suggested next step and Artifacts stale (fixed). L1
the pass contexts four, not alternating, the second walk 73 to 89 (fixed); L2 « not created by the native » beyond the evidence
(fixed: no extra instance through 141ACCA20); L3 the wheel's notches counted while the Advanced page is closed but no page blocks:
left, emptied at the page's opening (a dialog over the open page could keep them; a later fix with a build); L4 the wheel's row
rebuild during a slider's drag not run: left, not read; L5 the stage test checks the vignette's direction and bit, not its place
before the curve nor the bloom left alone (read in the code; a model check for a later tranche, as the step review's L2); L6 the
shader's comment on bits 4-5 (fixed, comment only); L7 the wheel's commits (fixed); L8 the merge build not cited (fixed:
`build-ps25`); L9 the spawn tally's map nodes on DS3's threads, bounded by content, behind its flag: left; L10 « checked in play »
(fixed: a scripted run), the I-0138 index row (fixed), `status.json` (fixed), the sheet's YAVG crops not stored (left: the ER path
is gone). The integrity review: no High or Medium. L1 the new check of the area's vignette in the ER body would still pass with
the factor on the red channel alone (more than 5 % of the values darker): left, the shader applies it to the three (read), a
per-channel or model check with L5 above; L2 the thread-affinity scope and allow entry not listed (fixed: in the Artifacts); L3
the ER re-check's 227-file median and corner figures have no stored output (left: m60's keys re-run and reproduced by the
reviewer, the median not; moot, ER's vignette removed); L4 the sheet's crops not stored (the reviewer's own crops give the same
order, DS3's near ER eased 0.5); L5 the first sample's 75 against 77 (fixed: 75 to 77), STATE's « alike » (fixed: but four ids). Merged `0860cbc8c`; the launcher rebuilt (`build-launcher-0860cbc8c.log`: exit
0, 179 PASS), start check `start-0860cbc8c` passed (restored); `Clear-DiskCache -Apply`: nothing deleted, 510 GB free.

## Tranche 12: the smoke sampled on the scene's clock, the entities' lives; Moonlight Vortex paired (2026-11-17)

**The tool** (`58a8aa526`, `69d57594b`; `build-ps27`, incremental from `r2-cache`): the scene's own clock from the first creation of
FXR 732035 through 141ACCA20 in both modes (`ffx_scene_clock`); `ffx-entity-arm.txt` makes the light press once at a moment of
that clock (`ffx_scene_press`); `ffx-entity-life.flag` follows every entity offered on pass 0x45 for 90 s (`ffx_entity_life`: kind,
masks, vtable, first position, first and last offer, offers; a life ends after 12 missed walks and 100 ms, or when its address
comes back with another kind, vtable, object or resource; `ffx_entity_life_count` each second with the longest gap between
offers). Runner `Run-I141SmokePix.ps1 -ArmMs -Life`, series `Run-I141T12.ps1`, reader `Compare-I141Lives.py`. **Step review**
(Opus 5.5 high, `e96ddeb4d..58a8aa526`): no High; M1 an address reused within 250 ms merged two lives into one longer (fixed: the
identity checked on every offer, « reborn »); M2 a wall-clock gap alone ended every life at a hitch (fixed: missed walks); L1 the
count rows' drift (fixed), L2 the final flush (fixed: swept first, re-checked under the lock), L3 the clock read before the lock
(fixed), L4 the row cap silent (fixed: Compare warns), L6 the runner's switches (fixed); left: L5 (the press reads a newborn's
words twice, 1500 rows), L7 (the scene clock is wall time: the 24 creations of 732035 fall 14.40 s and 14.39 s after the first in
the two modes, below).

**The runs** (`i141-t12-life-c32000000-nat` and `-orig`, 10:46 to 10:55, the ASUS, offline, no measure, both exit 0 without fault,
restored; `-ArmMs 19000`; `evidence/I-0141/artifacts/runs-t12.log`). OBSERVED:
- **The same moment**: the press at 19027 ms natively and 19021 ms in the original on the scene's clock; the library's creations
  alike in time (4040, 732200 and four 732035 at 0 s; 4040 at 14.31 and 14.34 s; twenty 732035 at 14.40 and 14.39 s).
- **At the press**: kind 3 with mask 4 (the smoke), 102 distinct entities natively against 73; the other kinds within three.
- **A second at a time** (offered in the last 100 ms): kind 3 mask 4 higher natively through the scene (34 to 39 against 19 to 24
  before 5 s; 147 to 175 against 127 to 158 at 16 to 18 s; 102 and 95 against 74 and 71 at 19 and 20 s; equal from 27 s); the other
  kinds alike (mask 8, and mask FFFFFFFF's 180 a second after 30 s).
- **Not one cause but three** (kind 3 mask 4): (a) at 14.40 s, the twenty instances' first walk offers 119 entities once natively,
  none in the original (191 births against 59; the entities offered more than once 72 against 59); (b) natively 16 entities born
  together at 18.055 s at the layout's positions (lasting 0 to 3.4 s), one at 18.45 s in the original; (c) four entities (at (79.5, 82.1, 343.4) twice,
  (66.2, 85.1, 323.1), (80.7, 83.3, 336.3); three of them of objects older than the clock, the final review's M2) offered until 29.8 s
  natively, where the original's lives at those positions end at 1.4 to 15.1 s. The lives offered more than once last alike (median 3.3 to 4.0 s in both). At 19 s
  the 28 extra are (c) 4, (b) 7 (and 4 born after 18.3 s), and the 14 to 17 s births' survivors 13.
- INFERRED, not established: the instances are created alike, their entities are not: the library's emitters inside them spawn or
  end differently natively (a first update the native offers before the original does, a respawn at 18.055 s, an end the native
  does not reach). Next: the entity's creation and end (the vtable 3A9CA60's constructor and destructor, their callers, the instance
  and the library's time) hooked in both modes on this clock.

**The entity's creation and end, read statically** (a ghidra-research subagent, MCP reads; the pilot re-read 1ADAD80, 1ADAF70,
1ADDD20, 1ADCA20, 1ADC070 and 140267B70 itself). READ: the entity is the `+0x78` member of a 0xC0-byte emitter node; its vtable's
only writer is the constructor 1ADB540 (one RIP reference in the image, none as a stored pointer). The first node is embedded at
`+0xD0` of the type-0x898 effect object (0xD0 + 0x78 = the `+0x148` tranche 11 recorded), born with the instance
(141ACCA20 → 1AD1D30 → 1ACCE70 → 1AD5240 → 1AD4C00); further nodes are allocated at a state's entry (1ADAD80, under 1AD6BB0) when
a data flag is clear, the old head's contents moved into them. Each frame 1ADAF70 frees a non-head node whose particle count
(`+0x70`) is 0 and queues the others' simulation (1ADDD20, only when `+0x80` is set). The effect object ends (state 10) only after
1AD6740 finds the nodes' counts at 0 and the nodes' lifetimes passed, then waits on the `+0xA0` object (state 9). A particle's
life gains the node's extra life only when the renderer (`+0x58`) exists (`ParticleNode_Simulate`, 1ADD510, read by the subagent
only). INFERRED: (a) is nodes born at a state's entry and freed the next frame with no particle; (c) is particle counts that do not
reach 0. The census now carries each node's count and objects (`8a722fbad`) to decide it.

**The nodes' state** (`i141-t12-node-c32000000-nat` and `-orig`, 11:26 to 11:36, build `build-ps29` at `ae14b1dcc`, both exit 0,
restored; the press at 19012 and 19000 ms of the scene clock; step reviews of `8a722fbad` (M1 a node at a reused address merged
into the old life, fixed: a new particle array is a new life; L1 to L3 fixed) and of `ae14b1dcc` (M1 the runner's tag, fixed in
`b1a15696a`; L1, L4 left: the array test assumes the array changes only with the node, the state read before the lock). OBSERVED:
- every emitter node has its renderer and its particle array in both modes (bits 9), none has the `+0x98` object and one a mode the `+0xA0` object: the
  renderer is not what differs;
- the one-offer lives are not empty nodes: their count is 1 or 2 at their offer in both modes (natively 9 have 6, 10 or 24); natively 123 of them at 14.5 s,
  none in the original;
- (c): the four long native lives keep their particles to the end (counts 1, 1, 6 and 132, steady), all four last offered at
  29866 ms with 107 other lives (the cut); in the original (79.5, 82.1, 343.4) ends at 15079 ms (count 1), (66.2, 85.1, 323.1) at
  2.3 s and (80.7, 83.3, 336.3) at 1.5 s with particles left (the final review's M2: the 17262 and 17833 ms ends first written here are
  other positions): their effect is deleted whole, not emptied;
- the groups that end together end alike (18.5/18.4 s 69 lives, 24.5/24.4 s 51/55, 29.9/29.7 s 107/105), the native about 0.1 to
  0.2 s later (why, INFERRED: the scene clock is QPC, and the native runs at about 30 frames a second in that window, the final review's M4).
INFERRED: the original stops the first instances (an instance's hard stop deletes its objects, particles or not); the native
does not. Next: the instances' stops (141AB18C0, 141AB1080) observed in both modes (`1b7f2660e`).

**The instances' stops** (`i141-t12-stop-c32000000-nat` 11:49 to 11:54 and `-orig` 11:55 to 11:59, build `build-ps31` at
`89d4cc2de`, both exit 0, restored; the press at 19018.5 and 19016.7 ms; the original run alone after the runner refused to go on, a
backspace in its word-boundary check (fixed in `c38c9abfd`); step reviews of `1b7f2660e` (M1 every 141AB1080 call named a soft
stop, fixed: rows `bit8` by callers; L1 to L3 fixed, L4 left: no row says the budget ran out) and of `89d4cc2de` (M1 one budget
for both kinds, left: 85 and 50 rows were written of 4000; L1 to L4 left: the hook count a literal, call sites as return addresses,
the comment's coverage, the clock read before the budget)). OBSERVED: no hard stop before the cut in either mode; at the cut
(29.84 s natively, 29.76 s in the original, through 1A87CB0 from E6A5D2 and E6A0B2, or 68AB1C) the native stops 24 instances of
732035, 4 of 732000, 4 of 732010, one each of 732100 and 732200, 3 of 4040; the original 20 of 732035, one of 4040, none of the
others. So in the original the first four 732035 instances and, INFERRED (the clock saw nothing before itself, the final review's M2),
732000, 732010, 732100, 732200 and two 4040 end before the cut (an instance ends when its last object does, 1AD20B0); natively they never end. INFERRED: their type-0x898 objects do not reach
state 10 natively (1AD6740: the nodes' counts, the nodes' end property, the age from `+0x190`). Next: the head objects' state in both
modes (`c38c9abfd`).

**The effect objects' state** (`i141-t12-owner-c32000000-nat` and `-orig`, 12:08 to 12:17, build `build-ps33` at `a4ff74d8e`, both
exit 0, restored; step reviews of `c38c9abfd` (H1 the owner buffer at the largest floats, fixed in `a4ff74d8e` with L2 and L4; L1
and L3 left: the head test is one vtable read, an owner read that fails keeps the last one) and of `a4ff74d8e` (no High or Medium;
L1 the owner's first state named in the header comment)). A Fable read (advice, MCP reads, ranked causes: a distance-level container
1AD4460/1AD4200 swapping its children, the end request from draw code through 141AB1080, the object disabled, state 3 never left,
state 8 stuck) preceded it. OBSERVED, the type-0x898 objects at the last offer of their head node, both modes:
- natively 1242 in state 3 (flags 481), 10 in state 8 (503); in the original 857 in state 3, 220 in state 8 (195 with flags 503);
- the original's objects enter state 8, the end (1AD6740), at 14 to 18 s (27 objects: the first 732035 instances' heads at
  (79.4, 82.1, 343.4), (83.9, 79.2, 350.1) and the four kind 6 heads among them, the (79.4, 82.1, 343.4) head's end counted from
  age 14.48) and at the cut (186 at 29 s), three more at 1.2, 2.3 and 7.3 s; natively one at 7.27 s (the same place as the original's
  7.35 s one, the final review's M1), none else before 29 s, 9 after: the native's objects almost never leave state 3 and are deleted at the cut with their
  particles by the instances' hard stop;
- the objects' age advances alike (the native heads' age 53.5 and 68.6 s at the cut): the objects are updated, not disabled;
- the flags differ: 481 natively and 503 in the original for heads at the same positions; the rows carry the last offer's
  flags only, so whether they change within a life was not observed (the final review's M3; « Reviews at the merge of tranche 12 »).
INFERRED: the state-3 exit of 1AD71A0 is not met natively: its duration against the age, the spawner `+0xC0`'s vt+0x18, or the flag
0x400 with the end request (140267AE0 on the instance's `+0x80` bit 8; no 141AB1080 call in either mode before the cut). Next: the
three exit inputs of 1AD71A0 for the heads in both modes, and the FXR state machine's operands (the instance's `+0x78` chain, its
external values at `+0x48`, the camera distance, the state time; 1ACA640, 1ACA8A0, 1ACA310).

**Moonlight Vortex** (`i141-t12-mv-thomas-arts-nat` and `-orig`, 10:55 to 11:02, `Run-I140Spells -Families thomas-arts`, the same
build and screen, both exit 0, restored, both filmed): the horizontal streak, the rings and the green ground are in **both** films
at the same moment of the cast (`evidence/I-0141/artifacts/mv-read/t12-mv-pair-c1.png`, crops 760x560 at 260,120, 10 a second);
the e89 pair's difference was the phase between its two stills. The native run with the bloom row at 0 (`i141-t11-mv-noglare`,
`bloom` 0 in its journal) keeps the streak: it is the effect's own, not our glare. Without the effect light banks
(`i141-t11-mv-nolight`) the ground is green as well. OBSERVED; no defect left there but the character darker natively (the known
« player darker », `i140-e11`).

## Reviews at the merge of tranche 12 (2026-11-20)

The final review (a fresh Opus 5.5 at xhigh, `main...20d70b554`, read-only, the ten run journals re-read with its own scripts) found
no High and held criteria 1 and 2 (the hooks only behind their flags, the originals called with their arguments, the reads guarded),
and confirmed the press times, 102 against 73 at the press, the per-second series, cause (a)'s 191/59 births and 119 one-offer lives,
the stops at the cut (24/4/4/1/1/3 against 20/1), the owner totals 1242/10 against 857/220 and the end groups. It found the
records claiming more than the journals hold; the branch was not merged alone, its corrections ride with tranche 13:
- **M1** « the native's objects never leave state 3 » is too strong: natively one head, at (64.7, 87.2, 320.8), enters state 8 at
  7.27 s, as the original's at the same place (7.35 s); the original also has three heads in state 8 before 14 s (1.2, 2.3, 7.3 s).
  Restated above as « almost never »; the 7.3 s object is the control case of tranche 13.
- **M2** cause (c) paired the wrong lives: in the original (66.2, 85.1, 323.1) ends at 2.3 s and (80.7, 83.3, 336.3) at 1.4 to 1.5 s;
  the 17.3 and 17.8 s ends are other positions. Three of the four long native lives belong to objects older than the clock (heads'
  ages 68.6, 53.5 and 53.5 s at the cut: 732000/4040 created about 38.5 s and 732100 about 23.8 s before 732035). The scene's effects
  start about 38.5 s before the clock, which saw nothing before itself: that 732000, 732010, 732100 and 732200 « end by themselves »
  in the original was never observed (INFERRED; a stop before the clock would leave no row). Tranche 13's state-3 rows start at the
  ceremony's first FXR (`ffx_scene_pre`).
- **M3** « 481 to 503 is not the creation level changing » was not supported: the rows carried the flags at the last offer only.
  Read since (the pilot, MCP): 141ACA090 is called with level 0 by the root's factory 1AD1D30 and level 1 by 1AD2700, 1AD5240 and
  1AD7FA0 (1AD3260 not read), so 0x100 does not come from those; the heads at the same positions carry 481 natively and 503 in the
  original. Open, INFERRED either way; tranche 13's rows carry the flags at the first and the last call (four original kind-6 heads
  reach state 8 with 481, so 503 is not needed for state 8).
- **M4** the compared window pairs a native at about 30 frames a second (16 to 24 s on the clock: median 25.5 to 31.6 ms, in
  tranche 11's run without the life flag too) with the original at 60: the library's per-frame work runs about half as often
  natively there. The headline at equal windows is the 100 ms count, 102 against 74 at 19 s; the « 102 against 73 at the press »
  came from 1500 rows covering about 9 native and 11 original walks. The 0.1 to 0.2 s « drift » of the end groups is INFERRED (QPC
  against the game's time, or the slow native frames). Open: whether the frame rate is part of the cause.
- **M5** the smoke runner prints `EXIT 0` over a failed Run-AlphaCast (known, « Uncertainty »): to fix before another series relies on
  it.
- **M6** the life tracker never closed: every run's count rows stop at 67 to 69 s of the clock, before its 90 s; about 440 lives a run
  were never written and Compare-I141Lives' « open at the end 0 » was not measured. The smoke claims are not affected (every smoke
  life ends by 31 s). Tranche 13's state-3 rows close at 45 s.
- Lows: L1 numbers that do not trace (« 21 entities born together at 18.055 s » is 16 in every native run; « (b) 10, survivors 14 »
  is 7 and 13; one node a mode has the `+0xA0` object; natively 9 one-offer lives have counts 6, 10 or 24; « equal from 27 s » is
  115/113, 111/108, 106/107; only the (79.4, 82.1, 343.4) head has the age 14.48, the others 0); L2 « two walks a frame natively » is
  wrong (one a frame: 12 missed walks last about 400 ms natively in the slow window); L3 25F9D0 called under `life_lock`; L4 a press
  row without rows armed; L5 a leftover `ffx-entity-arm.txt` not in Run-StartupTrace's pre-clean list; L6 the clock's FXR id
  hard-coded; L7 duplicate getter and position calls after the press; L8 `instance_fxr` never drops a freed instance; L9 the Moonlight
  Vortex pair aligned on the runner's key time (D-0299), its sheet's script not recorded; L10 STATE's « D-0230 due 2026-11-19 » past.
  Left as tool debts of a diagnostic (not on a production path) unless tranche 13 needs them; L1 and L10 corrected in the records.
  M5 fixed in tranche 13b (the runner fails unless Run-AlphaCast reports exit 0 and the save restored).

## Tranche 13: the state-3 exits, the FXR state machine's end index (2026-11-20)

**The observer** (`ed1e92f74`, `8f4c50a21`, `ab6145dcc`, `9cc6c7b43`; `ffx_state3` rows under `ffx-entity-life.flag`, both modes):
every type-0x898 object's state-3 handler 1AD71A0 and its duration 1AD6520 from the ceremony's first FXR (`ffx_scene_pre`, 732000,
38.5 s before the scene clock in both modes) to 45 s of the clock; per object the age, the duration, the flags, the instance's
flags, the FXR state machine's index and count, `+0xB8`, `+0x18` and `+0xC0` at its first and last call, one « exit » row when it
leaves state 3 and one « open » row for each life still in state 3 at the close. Its step reviews: of `ed1e92f74` (H1 a move of the
state machine inside a call, 1AD6BB0 or the recursion 1AD65F0 → 1AD75E0 → 1AD71A0: the innermost call records, the inputs after a
call kept, the moves counted; M1, L1, L2, L4), fixed in `8f4c50a21`; of `ab6145dcc` (M1 the « open » rows' FXR id read from freed
memory at the close; M2 the end row without the open count or the rows the cap refused; L1 the close time, L2 a window opened
without a clock, L4 the reader's docstring, L5 the runner's summary), fixed in `9cc6c7b43` and `99e241966`; L3 (the 732xxx range
taken as the ceremony's) left as the diagnostic's assumption.

**The run** (`i141-t13-s3-c32000000-nat` and `-orig`, 10:08 to 10:17, build `build-t13c` (`ab6145dcc`), the ASUS 1920x1080
window, offline, both exit 0, restored by hash; `Compare-I141State3.py`). OBSERVED, on the scene clock (D-0299):
- the duration is not finite (+inf) for every object of the heads in both modes (one 732200 object has a duration of 5 and exits by
  it), and the spawner `+0xC0` is set with the same vtables: neither
  exit (a) nor (b) applies; the original's ceremony objects leave state 3 by the **FXR state machine's index -1** (`+0x90`→`+4`
  = 0xFFFFFFFF, out of its count 1: an end asked, 1ACA5F0 false, then 1AD6740), with the flags 0x400 set;
- in the original: 732035's objects 36 at 14.4 s (the first instances) and 180 at 29.75 s (the cut), 732000's from -37 to -33 s,
  732010's from -28 to -25 s, 732100's at 0 s, 732200's one at 14 s and 4040's at -25, 14 and 30 s, all by the index -1;
- natively **none** of the 732xxx or 4040 objects moves: their index stays 0 of 1, `+0xB8` 0, the flags 481 (0x400, 0x80, 1)
  throughout; 732035's 404 lives are open at the cut (deleted there by the instances' hard stop, the « open » rows at 30 s: 494
  natively against 28) or at the close; 732000 and 732010 natively never end either;
- the other families end alike in both modes (60, 62, 101001, 800022 by the flag or the duration at the same times; 320 by the
  index -1 in the original, by the flag natively (« flag? »: `+0x18` set, its type not in the row), three objects at 30 to 32 s), and the 7.3 s control object (732200, 5 s on this
  run's clock) ends alike (state 8 by the flag);
- the native's « open » rows carry the FXR id 0 for 35 lives (the step review's M1, read at the close from freed memory; fixed for
  the next run); the rows' cap was not reached (18458 and 18457 rows left).

So the cause is narrowed to the FXR state machine's step 141ACA640 (READ, GhidraMCP): it adds dt to the state time (`+8`) and, while
the index is below the count, evaluates the state's conditions (141ACA8A0: an operator over two operands, each evaluated by
141ACA310 from its kind: a constant, the instance's external value by key (`+0x48`, set at creation to keys 0 and 10000 by
140E6F8C0 for SprjSfxImp's effects), a camera distance, or the state time) and takes the first true condition's next index. In the
original the ceremony's machines reach -1 (some within 1.5 s of their creation), natively never: one of the operands differs.
Which one is not observed yet: tranche 13b's rows (`ffx_machine`, `99e241966`: the conditions with both operands evaluated by the
game's own 141ACA310 at each step, and the instance's external values) are made for the next run.

**Tranche 13b** (`99e241966`, its step reviews: of `99e241966` H1 the state time passed to 141ACA310 in an integer register where
the game passes it in XMM2 (every time operand would have been garbage), M1 the reader hiding moves, M2 a machine at a reused
address, L1 to L3; of `d6af2dbec` L2 141ACA310 called on a constant or external operand whose value pointer is null (it dereferences
it unchecked: a possible fault in the game), M1 a move on a « first » row not counted, M2 a reused instance address, L1, L3, L4, L6;
all fixed in `d6af2dbec` and `f6eaccbcf`, L5 of the second (non-finite values written 0) left: none was met). The run
`i141-t13-m1-c32000000-nat` and `-orig` (10:46 to 10:55, `build-t13g` (`f6eaccbcf`), the ASUS, offline, both exit 0, restored;
`Compare-I141Machine.py`). OBSERVED, the same in both modes for every machine of 732000, 732010, 732035, 732100, 732200 and 4040:
one state, one condition, `1 <= external value 0` (operator 11, the float compare; the constant 1.0 against the instance's key 0)
leading to -1, the end; the instance's external values {0: 0, 10000: 0} from its creation (the creation's table, 140E6F8C0).
- In the original the condition becomes true because the instance's table is **replaced by {0: 1}** (one key): 732000's and
  732010's machines 1.0 s after their creation (state time 1.017), 732100's at 0 s, 732035's first instance and 732200's at 14.40 s,
  4040's at -25.1 and 14.4 s, the 20 other 732035 machines at 29.75 s, just before the hard stops of the cut.
- Natively the table stays {0: 0, 10000: 0} on every machine to the end (state times to 65 s): **nothing puts {0: 1} on the
  ceremony's instances**, so their objects never leave state 3 until the cut deletes them.
So the native's extra smoke is the ceremony's effects never asked to end. The end tables come from 141AB1A60 (instance, table;
READ), called by CreateFfxInstance 141ACCA20 and FfxHandle_ApplyExternalValues 141A87DF0, itself reached through the thunk
140244A50 from twelve places (SprjSfxImp's 140E6A750 applying a table to all its handles, the handle stop 1404D2AC0 applying the
global stop table at 144773698 when the instance is live and the table exists, else the hard stop 141AB18C0, and character SFX
code at 1408xxxxx). Which of them puts {0: 1} on the ceremony's instances, and why natively it does not: tranche 13c's rows
(`ffx_ext_set`, `4847b46e3`: each call of 141AB1A60 on those instances with its table and six callers).

**Tranche 13c and 13d** (`4847b46e3`, its step reviews: of `4847b46e3` M1 the creation calls lost or filed under a stale FXR, L1 the
tail jumps hiding 1404D2AC0, L2 the stack walked before the filter, L3 the silent cap, L4 an unreadable table shown empty, fixed in
`1b44799dc`; of `1b44799dc` M1 the same creation calls (the FXR now from SpawnObserved's thread-local `creating_fxr`), L1 to L4
(the close row's time, the counts a snapshot, an empty table not « unreadable », a truncated table flagged), fixed in `28a7a70c3`,
L5 the runner's summary without `ffx_ext_set_end` left (the reader shows it)). The run `i141-t13-x1-c32000000-nat` and `-orig`
(11:04 to 11:13, `build-t13h` (`4847b46e3`), the ASUS, offline, both exit 0, restored). OBSERVED (`Compare-I141Machine.py`):
- in the original {0: 1.0} (one table, the same address every call) is put on 732000, 732010, 732035, 732100, 732200 and 4040's
  instances about 60 times a second from their stop to their end (1954 rows, the cap reached), always by `1A87E32 < E6A77B <
  E6B2B7`: SprjSfxImp's entry update E6B210 calls E6B3C0, the entry's state-1 step, which, when the entry's stop bit (+0x14 bit 0)
  is set and the SFX manager's `+0x68` is not null, tail-jumps to the entry's vt+0x40 with that `+0x68` table, which applies it
  through 141A87DF0: `SprjFD4MultiSfxIns` (vtable 1429922C8) 140E6A750 to each handle of `+0x70..+0x78` (stride 0x460),
  `SprjFD4SingleSfxIns` (vtable 142992728, found by the fix's second step review) 140E6C1D0 to its handle at `+0x70`. READ
  (GhidraMCP, 2026-11-20);
- natively no such row (39 rows, all the creation tables {0: 0, 10000: 0} and the instances' own creation calls): a stopped SFX
  entry never got the table, stayed in state 1 and its effects ran until the cut's hard stop.
The SFX manager's `+0x68` is the table SprjSfxImp's constructor E6C9D0 makes at 140E6D050..140E6D08F: 141AB0150(&key 0, 1, heap
479C270) then 141AB05E0(table, 0, the bits of [3D7EE10] = 1.0f in R8D), stored at `+0x68` and at the global 4773698 (READ). The native
manager (PublishNativeSfxManager) had left it null as a « colour table ». **The fix** (`814ee89e3`): the native manager makes it by the
same calls and stores it at both places. Its step review (High none; M1 the comment's mechanism, first written as 1404D2AC0's
fallback, rewritten to E6B3C0's gate; M2 the readers of `+0x68`: E6B3C0 read (called by E6B210 alone), the other readers of the manager's
`+0x68` not enumerated (an offset from a global, no cheap cross-reference: left, the run and the start check are the witnesses), the
entry classes' vt+0x40 CPU only, 141A87DF0 null-safe on a handle without instance (so with `no-native-effects` too); L1 two allocations, not
one; L2 GX-DEPENDENCIES and EFFECTS-PHASE1-NATIVE corrected; L3 the guard moved to the function's head; fixed in `87a0e7094`). The step
review of `87a0e7094` (High none; M1 the second entry class, Single; M2 EFFECTS-PHASE1-NATIVE's list; L1 the value check moved to
the head too; L2 the refusal's name for a set global now « not pristine »; L3 01_state's list; L4 « did nothing » worded; all fixed
in the next commit). INFERRED (READ, not witnessed outside this scene): the change is game-wide, every stopped SFX entry now gets the
table as in the original; the readers of 4773698 (the handle stop 1404D2AC0, character SFX code) natively took a soft stop
(141A883D0, 141AB1140, 141AB1080) and now apply the table; no family outside the Archdragon Peak arrival was witnessed.

**The fix verified** (`i141-t13-x2-c32000000-nat` and `-orig`, 11:25 to 11:34, `build-t13j`, built from the tree at `28a7a70c3`
(which holds `814ee89e3`), published 11:22:17 before `87a0e7094`'s edits; a stale `build-t13j.exit` beside it is a first launch that
failed on its path, by the timestamps (the build log names no commit); its DLL's SHA-256 is the run's `baseline.json`'s; the merged
tree's `87a0e7094` and `be58803de` move the two refusals to the function's head, the same conditions, first launched by the start
check after the merge; the ASUS, offline, both exit 0, restored). VERIFIED:
- the native SFX manager publishes the stop table (`native_sfx_manager_published` `"stop_table":true`); natively {0: 1.0} is put on
  the ceremony's instances by the same chain as in the original (`1A87E32 < E6A77B < E6B2B7 < E6E7E2`), from the same moments
  (`Compare-I141Machine.py`: 732000 at -37.62/-37.49 s, 732010 at -27.67/-27.58 s, 4040 at -25.13/-25.07 s, 732100 at 0 s, 732035
  and 732200 at 14.43/14.40 s, native/original);
- every machine of those FXR that moves in the original moves to -1 at its times within 0.15 s (the same 37 moves in both modes;
  4040's fourth machine moves in neither);
- the state-3 exits (`Compare-I141State3.py`): 509 exits and 1032 against 1034 open rows, FXR by FXR the same causes and counts
  (732035: 216 to state 8 and 216 to state 10 in both modes);
- the smoke at the press (`Compare-I141Lives.py`, kind 3 mask 4): **74 distinct entities natively against 73** (102 against 73
  before the fix, tranche 12), each second of the scene within six of the original's (they were 3 to 28 above from 14 to 19 s, about 12 to 28 from 15 s);
  causes (b) (the 16 entities born together at 18.055 s) and (c) (the four long lives) of tranche 12 are gone;
- **not fixed, cause (a)**: at 14.3 to 14.4 s, the twenty 732035 instances' first walk, 178 smoke births natively against 50 (122
  entities offered once against 0; kind 6 mask 4 16 against 0), as before the fix (`i141-t13-x1`: 119 against 0); over the scene
  591 smoke lives against 467, births between 10 and 15 s 210 against 84 (the final review of tranche 13's M1). OPEN.
OBSERVED, not on the scene's clock (so outside D-0299, not a verification): the arrival's sweep (a shot a second on the wall clock,
the framing about one shot apart between the modes), the mean luma (signalstats YAVG) of shots 28 to 33: natively 52, 48, 46, 57,
79, 107 against 65, 59, 56, 54, 66, 87 in the original, at the same shot 18 to 20 % darker at 28 to 30 and 20 to 23 % brighter at 32
and 33 (before the fix, `i141-t13-x1`: 114, 149, 153, 125, 191, 169 against 59, 55, 55, 69, 90, 122: the white veil gone); the sheet
`evidence/I-0141/artifacts/t13-smoke-sheet/smoke-before-after-original.png` (shots 29 to 32: the native before, after, the original).

## Reviews at the merge of tranches 12 and 13 (2026-11-20)

The final review (`final-reviewer`, xhigh, `main...520f95892`, main `f4b6ecb88`): High none. M1 cause (a), the first walk's burst at
14.4 s, left unfixed but dropped from the records (fixed in the records: named OPEN in « The fix verified », « Uncertainty », the next
step, STATE and ROADMAP); M2 the sweep's luma, on the wall clock, listed as VERIFIED and summarised as « within » (moved out of the
verification, its numbers stated as they are); M3 `build-t13b.log` overwritten by tranche 13's build of that name (recorded in
« Evidence »; future builds named apart). Low: L1 the `ffx_ext_set_end` row written only at a call after 45 s (the reader now warns at
the 2000-row cap; the module left, an observer's close); L2 a null table shown « unreadable » (the reader shows « null »); L3
`status.json` (13 of 16); L4 the artifacts (a line for tranches 12 and 13); L5 « game-wide » labelled INFERRED; L6 commit messages
(informational, left: `0dd24e2be`'s « both entry classes » observed only for Multi, Single READ; `814ee89e3`'s mechanism corrected by
`87a0e7094`); L7 three small inaccuracies (fixed); L8 `build-t13j`'s source (recorded); L9 01_state's +0x58 comment (fixed); L10
D-0267's IN-FORCE line « after R12 » (left: a decision record, the coordinator told); L11 the runner's `-Life` header (fixed).
The integrity review (`integrity-reviewer`, `main...5f93a1a84`): High and Medium none; L1 the « before » gap overstated (3 to 28,
fixed); L2 the index's « fixed » (qualified); L3 a « never » left in tranche 12's summary (« almost never »); L4 D-0230's overdue
control dropped from STATE (back, overdue); L5 FXR 320's « flag » without its question mark (fixed); L6 the verifying build predates
the guards' move (recorded; the start check launches the merged code); L7 Run-StartupTrace's flag missing from the artifacts (added).

After the merge (`134d59be4`): the launcher rebuilt from `main` (`evidence/I-0141/artifacts/build-launcher-134d59be4.log`: exit 0, 179
PASS lines), start check `start-134d59be4` passed (restored); the disk rotation deleted nothing (521.5 GB free).

## Tranche 14: the first walk's burst at 14.4 s, the original's pause in the effect draw (2026-11-20)

**The question** (tranche 13's cause (a)): at 14.3 to 14.4 s the twenty 732035 instances' first walk offers about 120 smoke
entities once natively, none in the original; why the native draws a newborn object's particles on its first frame.

**The tools** (`claude/r-i141-t14`, `22427126c` to `585d18ab0`; `ffx-entity-life.flag`, both modes): `ffx_newborn_draw` at the node
draw 1ADDE80 (READ, GhidraMCP: it offers node+0x78 through the pass's vt+0x20 when the count +0x70, the renderer +0x58 and the
resource +0x68 are set, the pass's vt+0x30 accepts the renderer's kind and its vt+0x28 the box node+0x30) and `ffx_newborn_update`
at 1ADD510, for the heads (object+0xD0) of FXR 732035's type-0x898 objects under 0.25 s of age; `ffx_draw_gate` at each
scene-control draw 24E790 of pass 0x45 (its gates, the library's draw list, the nodes drawn and the offers); `ffx_frame_ring`, the
last twelve pass-0x45 draws and update bursts before each window. `Compare-I141Newborn.py` reads them. Both modes reach 24E790's
direct path (`sc_590` 0, `sc_5be` 0, `sc_20` 1; the GX display's `+0x3E1` 0 in the original), so 1ADDE80 is the same walk.

**The runs** (the ASUS 1920x1080 window, offline, both exit 0, restored; the boss revived by `test-revive.txt`, Eta's travel and the
four flags as tranche 13): `i141-t14-n3`, `-n5`, `-n6` (`-c32000000-nat` and `-orig`; builds `build-t14b`, `-t14d`, `-t14e`).
`-n1` (no revive: no ceremony), `-n2` (a row buffer overflowed by a reset box's 3.4e38: 0xC0000409 at the scene clock's start,
fixed `5af739c35`) not used; `-n4`'s gate rows not used (its 400 rows spent before the scene clock, `SceneMs` -1 there, fixed
`1d0fb0448`), its walks used for the gap measure below.

**OBSERVED** (n3, n5, n6 alike, but for n6's native 14.4 s window: its fixed window [14370, 14440] ms closed before that run's
births at 14457 to 14460 ms, so its ring and gate rows there come before them):
- in both modes the first update of a newborn head (count 0 to 1, or 0 to 5) leaves half of them with a non-finite box (36 at 0 s,
  180 at 14.4 s, the same heads' counts in both) and the other half with the empty (reset) box; the box is made before the
  emission (1ADD510 calls 1ADB150 over the particles before it adds the emitted count, tranche 13), and a non-finite box passes
  slot 5's frustum test;
- natively a pass-0x45 draw follows every update, so those heads are drawn at age 0 (36 and 180 offered with their non-finite box)
  and are then culled once their box is real (the lives whose owner's age at the last offer read is 0: 12 and 137 to 141
  natively, none in the original); each head is drawn twice a frame, about 3 ms apart on one thread, the first draw never offering
  in either mode, so a « refused » draw mixes that pass (not identified: the row carries no pass id) with frustum refusals;
- in the original no head is drawn at age 0: between a birth and the next walk there are always two updates. The original makes no
  pass-0x45 draw for about 80 ms at both moments while its updates go on: the last draws at -64.3 and 14335.5 ms, the next at 16.1
  and 14415.4 (n6's rings); the longest gap between two walks in the second of the burst 78 to 91 ms in all four original runs
  measured (n3, n4, n5 and t13-x2), the largest of the run (other seconds about 20 ms, a few 25 to 55 ms: n4 at 18 s, n6 at 19 and
  50 s); the native's 40 to 68 ms gaps are its usual frames, without a pause;
- where a walk follows a birth in the original (29.77 s, one kind-3 entity, n5 and n6), it offers that newborn at age 0, as the
  native does (one at 29.8 s);
- the original's effect dt ramps after each pause (0.0044, 0.0084, 0.0122, 0.0167 at 0 s; 0.0018, 0.0068, 0.0117 at 14.4 s), the
  native's is its frame time from the first step (n6: 0.018 to 0.020 at 0 s, 0.027 to 0.038 at 14.4 s), without a ramp.

**Reading** (INFERRED): cause (a) is not in the native's effect code. Both modes run the same library functions; the original does
not draw the effects during its ~80 ms pause at the ceremony's start and at the 14.4 s cut, so the heads born in it are first walked
after a second update (a finite box, then culled when out of view); the native draws every step and walks them at birth. Whether the
pause holds the original's whole frame or only the effect draw, and whether it is deliberate (a cut's hold) or a render hitch, is
not read (the dt ramp points at the game's step); natively no picture of a held frame was compared. No fix: imitating a hitch is
not a goal, and a rule refusing non-finite boxes would depart from the original's code where it walks a birth (29.77 s).

### The haze in play at Archdragon Peak's bonfire (2026-11-20)

**The question** (« The witnesses of tranches 6 and 7 »): natively, in play after 32000010, a denser haze (the castle behind the
bonfire nearly gone) and a darker player.

**The tools** (`be9fb3a7e`, `94f2c58d4`, `e86698198`; `fog-build-observe.flag`, both modes): `fog_block_latest`, each 14E900 caller's
latest cbFog block every 5 s; `fog_array`, every 5 s one whole deferred array with its keys, its cbFog blocks and its cbLight blocks.
The original's array is 137910's (READ, GhidraMCP: the light entries by 1496D0 at 137A7D or the blend 14A2D0 at 137ABF, uploaded, then
the fog entries by 14E900 returning to 137B97 or 137C3E, into one allocation when the device flag `+0xA8` is set; its entries
`[root]+0x41F8+0x88`, 0x30 bytes, count `+0x90`), snapshot at a run's first fog call before the builder writes; the native's from
`CollectNativeLightBlocks`' own keys and blocks.

**The runs** (the ASUS 1920x1080 window, offline, all exit 0, restored; Eta's travel, the four flags and the boss revived as tranche
13): `i141-t14h-c32000010-{nat,orig}` (`build-t14f`, 13:51 to 14:06, tranche 6's protocol, the cutscene tool's shots to 94 s);
`i141-t14k-c32000000-nat-noffx` and `-nat-nofogbanks` (`no-ffx-draw.flag`, `no-effect-fog-banks.flag`, 14:08 to 14:20);
`i141-t14m-c32000000-{nat,orig}` (`build-t14g`, 14:26 to 14:37); `i141-t14n-c32000000-{nat,orig}` (`build-t14h`, 14:42 to 14:53).
Sheets `evidence/I-0141/artifacts/tranche-14/haze/` (`pair-*.png` original | native, `quad-99.png`).

**OBSERVED**:
- the haze is still there on today's tree, and already in play after the ceremony 32000000, before 32000010 (the sweep's shot at
  99 s); whole-picture luma in play 77 to 81 natively against 84 to 89 (the cutscene tool's shots from 54 s);
- the veil over the stairs and the castle is mostly the native effects' smoke: in the sweep's shot at 99 s (`sweep-1-99.png`) the
  castle's crop (560x300 at 300,120) has a mean luma of 101.8 natively with the effects (`i141-t14h-c32000010-nat`, dense smoke
  clouds), 94.7 in the original (`i141-t14h-c32000010-orig`, a thin mist; luma 69 to 119) and 83 natively without any effect draw
  (`i141-t14k-c32000000-nat-noffx`, luma 70 to 103: the clouds gone, the castle grey and flat); the stairs' crop (300x200 at 820,260)
  52 to 79 without the effects against the original's 74 to 128; the player's (160x300 at 880,380) mean 52 against 84. Whether the
  native smoke there matches the original's in density and brightness was not measured. Without the native fog banks for the
  effects the picture is the native's own;
- the deferred arrays in play (`-t14m` fog blocks, `-t14n` fog and light blocks; builds `build-t14g` and `build-t14h`, the camera
  at the bonfire): the original's 13 entries are all among the native's 31 (its extra keys: sets the native registry keeps for 8
  collects, area 232000000's). For each key the cbFog block's words 0 to 19 are the same in both modes to the printed digits; the
  cbLight block is the same to about 1e-4 in the words 1496D0 and 14A2D0 write (`-t14n` only), not in the others: row 16's z and w
  (FC_bLocalLight) hold (-100, 1), (100, 1) or (0.127, 162.6) in the original and 0 natively for 12 of 13 keys, set 211's light 3
  rows and the default key {..., -1}'s rows 2, 3, 6 and 7 differ and change between two original rows at the same camera (stale
  memory, INFERRED). The fog keys differ in strength: fog 0 max 0.6535 at 1/300 per metre, colour (0.47, 0.54, 0.75); 350 0.4563 and
  0.0026; 300 0.8169 and 0.0005; 310 1.0 and 0.0017 (fog 0's distance fog on a castle 100 to 150 m away, computed from these words
  without the shader's formula checked, is of the order of 0.2: INFERRED). Light {132000000, 0}: the sun (0.379, 0.481, 0.8, 0.64), light 2 weak (colour
  (0.012, 0.013, 0.019), specular (0.057, 0.065, 0.093)), no SH ambient; several other sets there carry real fill lights: 30's light 2
  (0.21, 0.24, 0.29), 100's lights 1 and 2 (0.40, 0.47, 0.60) and (0.27, 0.35, 0.50), 215's light 1 (0.42, 0.51, 0.82), 211's lights
  1 and 2 (0.36, 0.57, 0.89) and (0.22, 0.31, 0.39) without a sun, 370's (3.5, 4.2, 4.8) and (1.2, 1.3, 1.9). Which set the castle's
  and the player's pixels take was not read.
- the cbFog block's last row (words 20 to 23, the row `rt_scene.h` names FogType), read afterwards in `fog_block_latest` (all 24
  words, the same runs): 14E900 writes word 20, the int 1, and never 21 to 23, nor does 137910 after it (READ, GhidraMCP); the
  original's rows hold (0, 0, 0, 0) most often (277 and 273 of 339), else (0, 1, 1, 1) or (0, 0.117, 0.140, 0.020), the native's
  (int 1, 0, 0, 0). Natively the block is its own
  zeroed array; in the original 21 to 23 are what the shared allocation held before (its light entries, 0x120 bytes each, written
  first in the same memory), so they are padding of FogType (INFERRED: a field 14E900 never writes). Word 20 prints 0 in every
  original-mode row (678, `-t14m` and `-t14n`), the game's own callers 263B0E and 266213 included, and 1.4e-45 in 408 of the 480
  native-mode ones (the other 72 hold 0 in words 20 to 23): the same int 1 read
  as a denormal flushed to zero in the original's process (INFERRED, not checked).

**Reading** (INFERRED): the haze is two things. The clouds over the castle are the native effects' smoke, denser and brighter than
the original's mist there (an effects question, open: the ceremony's smoke tranches matched its entities, not its look in play).
Under them the castle and the player are darker and flatter natively, and the sets' records are not built differently: each key's
fog block and the light words the builders write are the same in both modes. Not excluded: that a pixel takes another set natively (the arrays
differ, 13 entries against 31, and the fog keys there differ in strength), which needs the castle's and the player's set index in
both modes. The likeliest difference is the light D-0262 removes from traced frames: the baked ambient (the area's environment
probes) and the sets' fill lights 1 to 3, several of them strong at this bonfire (100, 211, 215, 370); the original lights the
castle and the player with them, the native's traced frame has the sun and the traced bounces alone, so they are darker and flatter
and the same fog covers more of them. Which of the two (ambient or fill lights) does it is not separated. Both are workstream 12's
(ROADMAP: each area's ambient as a real source of the traced lighting; the player's look from its fill lights), not brought forward
(D-0300); no fix here. Not read: the sets the castle's and the player's pixels take, the original's ambient term on these pixels (a
capture of its light accumulation), and whether the native's smoke over the castle is brighter beyond the darker background. The
witness code after `build-t14h` (`consistent`, `light_from`, the read flags, `tail`) has not run.

**Step review of tranche 14's tools** (a fresh `step-reviewer`, Opus 5.5 high, on `main...ea5821bd0`'s native code and the
reader): no High. M1 the node draw hook 1ADDE80 installed with `ffx-record.flag` (Native) or `ffx-draw-observe.flag` (Reference)
alone, its counters counted without `ffx-entity-life.flag`: fixed (`dc63295e3`, with the fixes below; build `build-t14i`), the hook installed only with that flag in both chains
and the offers counted only with it. M2 an original `fog_array` row could pair keys and light entries read at a run's start with fog
blocks of another count, unmarked: fixed, each original row carries `consistent` (keys, fog blocks and light entries alike) and
`light_from` (`separate`, `shared` or `unread`); the builder's object is still read from `[root]+0x41F8`, not from a 137910 hook.
M3 the update bursts in `ffx_frame_ring` were moments 3 ms after the previous burst, not after the previous call, as the comment says:
fixed, the previous call's time kept on every call (any thread) and a moment noted when the gap exceeds 3 ms. M4 the reader's « owner
age at the first offer » reads the owner's age at its last offer: relabelled. L1 a menu view's fixed key wrote a native row: left out.
L2 the native row did not name the sets whose record failed (they carry set 0's blocks): `failed_light` and `failed_fog` added. L3 a
run's last address stopped at the 225-block cap, splitting one run in two: kept past the cap. L5 the original's keys and light read
at every run's start and a non-finite camera or `t` printed as a number: read only for a due run, non-finite values printed `null`.
L4 the draw-list walk in `DrawGateLeave` calls the library's own iterators without `Read`: accepted, it runs on the draw thread inside
24E790 where those lists are walked by the game itself, under an observation flag. L6 the new thread-locals and statics
(`newborn_offers`, `newborn_node_draws`, the burst's `previous`, the native row's `next_ms` and `rows`) are not in the thread-affinity
allow-list: the gate scans only its listed functions (of these, `EntityOffered`, whose new static is a `static const`); the others
are outside its scope, recorded here. L7 the reader's « age0 » is an age printed as 0 (`%.4f`), not an exact zero: said in its docstring.

**Second step review** (a fresh `step-reviewer`, Opus 5.5 high, on `ea5821bd0..dc63295e3`): no High. M1 the `fog_array` rows printed
20 of each cbFog block's 24 words, so the haze's « same fog record per key » could not see words 20 to 23: the rows now print 24
(`3ae27786f`, build `build-t14k`), and the existing `fog_block_latest` rows (24 words) were read for them (the OBSERVED bullet above: words 21 to 23 differ,
padding the shaders' FogType row never written by 14E900; the reading stands, INFERRED for those three words). L1 the reader's
« age0 » class means an exact 0 in the life rows (`%.6g`) and a null age fell in « age>0 »: each format said, null its own class,
the owner's age at the last offer whose owner was read. L2 the update bursts could note a false moment or drop a real one when two
threads raced: the gap test and the latest call's time now under the ring's lock, a late call no moment. L3 « shared » was
inferred, not read (137910's selector `+0xA8` is not read): renamed `fog_output`, the comment says so. L4 the fourth value `none`:
documented. L5 a light failure was listed as a fog failure: the fields are `light_failed` and `fog_replaced`. L6 one world key is
doubled natively (`entries` 2 where the original's row says 1): said in the comment, left. L7 the Reference hook count logged 35
with the 1ADDE80 hook: 36 with `ffx-entity-life.flag`. L8 a native run with `ffx-entity-life.flag` and without `ffx-record.flag`
writes `ffx_draw_gate` rows with 0 node draws and offers although those hooks are absent: left (the runs of this tranche set both;
older than this range). L9 the fog-array functions allocated under `noexcept` and formatted the row under the lock: no longer
`noexcept` (their caller catches), the run moved out of the lock before it is formatted; `FrameRingRow`'s Log under the ring's lock
(three rows a run) left. L10 a failed camera read kept the previous run's camera: cleared to null first; the row's time is its
emission, said in the comment.

**Third step review** (a fresh `step-reviewer`, Opus 5.5 high, on `dc63295e3..3ae27786f`): no High. M1 word 20 is an int that the
rows convert to a float, so it printed 0 in the original's rows where memory holds 1 (the reading of the bullet above rests on it):
the three fog witnesses (`fog_array`, `fog_block_latest`, `fog_block_build`) now also print words 20 to 23 as their bits (`tail`,
`a40db794c`, build `build-t14l`); no run since, so the original's word 20 stays INFERRED. L1 a failed block read printed as zeros: printed null. L2 an update
call held up between its clock read and the lock could be dropped or dated late: the clock is read under the lock. L3 the comment's
« 5 s after the last row » is after the last due run's start: corrected. L4 the reader dropped unparsable rows silently (a non-finite
`dt` prints `nan`): it counts them.

**Fourth step review** (a fresh `step-reviewer`, Opus 5.5 high, on `3ae27786f..a40db794c`): no High. M1 a failed block read still
printed its NaN fill's bits in `tail`: each block's read is kept, and a failed one prints `block` and `tail` null (`ec0afd311`, build `build-t14m`). M2
`fog_block_latest` and `fog_block_build` still printed words 20 to 23 as floats beside their bits: the three witnesses print words 0
to 19 as floats and 20 to 23 as bits alike. L1 words 0 to 19 still go through a float conversion: said in the comment (the
guarantee covers 20 to 23). L2 the reader's comment on unparsed rows named one cause: widened. L3 a journal passed twice summed its
unparsed count: reset per pass. L4 an update moment is the call's return, a draw's its start: said in the comment.

**Reviews at the merge** (on `main...532703dbb`, main `00fb47ad9`; build `build-t14m`, 178 tests passed). The fifth step review (on
`a40db794c..ec0afd311`, Opus 5.5 high): no High, no Medium; L1 the read flags could fall out of step with the blocks after an
allocation failure: room reserved first (`f8ff5e731`, build `build-t14n`); L2 `fog_block_latest` and `fog_block_build` printed a non-finite camera as `nan`:
null (`f8ff5e731`, build `build-t14n`); L3 the comment's precision and DAZ wording: corrected. The final review (`final-reviewer`, Opus 5.5 xhigh): **H1** « no
fill light » was false and the darker player was put on the ambient alone: set {132000000, 0}'s light 2 is weak, not zero, and five
other sets at the bonfire carry real fill lights: the haze section's bullet and reading rewritten (the ambient and the fill lights
D-0262 removes, not separated), with the summary, the Uncertainty, the next step, STATE and ROADMAP. **M1** « the cbLight block the
same » was true only of the words the builders write (row 16's FC_bLocalLight and stale words differ; `-t14m` has no light blocks):
said. **M2** per-key equality was read as per-surface equality: the set each pixel takes is now named as not read and the fog keys'
strengths listed. **M3** `i140-e11` (the Road of Sacrifices) was moved to workstream 12 without a measure: kept in workstream 7's
list and the next step. Low: L1 a non-finite `dt` or a negative age broke the newborn rows' JSON: null, and only ages in [0, 0.25)
read (`f8ff5e731`, build `build-t14n`); L2 n6's native 14.4 s births fell after the fixed window: said under OBSERVED (the window not moved, no run since); L3
the double draw a frame and no pass id in `ffx_newborn_draw`: said under OBSERVED (the pass not identified); L4 n4 both unused and
used: its use said; L5 the lives' label after the reader's change: corrected; L6 the tail's commonest pattern: added; L7 tranche 14's
artifacts: their own bullet; L8 which builds made the evidence: said, the later witness code not run; L9 1ADDE80's and 1ADD510's
three-argument void detours rest on GhidraMCP reads not committed as listings (node draws equal in both modes, 346 and 986): left;
L10 the QPC clock written three times (`FogArrayClockMs`, `native_ffx::QpcMs`, inline): left, observers of two namespaces; L11
STATE's next action lacked I-0141's next step: added.

The step review of those code fixes (on `ec0afd311..f8ff5e731`, Opus 5.5 high): no High. **M1** the new `dt` cell (24 bytes,
`%.4f`) would overflow on a finite `dt` of 1e18 or more and end the game (0xC0000409): `%.6g` (`cf81ba846`, build `build-t14o`). L1 the tail comment's « reads
the int 1 as 0 » holds only under DAZ: said. L2 the draw gate counted negative ages that the newborn rows refuse: one test, ages in
[0, 0.25), in both and their comments. L3 a reservation per block reallocated the flags each call: 225 flags and blocks reserved
once when a run becomes due, a block skipped when no room is left. L4 the newborn rows print a non-finite bound as -1e30 and a
failed read as 0: left (the reader decodes the sentinel). L5 a throw while a run's keys or light are read leaves `light_from` set or
drops a counted row, and the newborn detours build a string under `noexcept`: left, allocation failures in observers.

The integrity review (`integrity-reviewer`, on `main...8493103dd`): no High; no test, gate or allow-list weakened, no scene special
case outside the flags, nothing cut during a measure. **M1** « not the effects » and « no native defect » for the haze: the castle's
crop is brighter natively with the effects (101.8) than in the original (94.7) and darker without them (83): the veil is mostly the
native smoke, the darker castle under it; rewritten in the summary, the section, the Uncertainty, the next step, STATE, ROADMAP and
the index. **M2** tranche 9's `build-t14d.log` overwritten by this tranche: recorded at tranche 9's Artifacts line. Low: L1 the
crops' runs and the player's rectangle named; L2 the native's dt range corrected; L3 « the same to about 1e-4 »; L4 the other
seconds' gaps; L5 STATE's burst reading marked INFERRED; L6 which builds ran their tests; L7 the distance-fog figure moved out of
the observation as INFERRED; L8 the uncommitted GhidraMCP reads, as the final review's L9: left.

The step review of that fix (on `f8ff5e731..cf81ba846`, Opus 5.5 high): no High, no Medium; the four criteria hold. L1 a failed
reservation before the camera's fill would print an unread camera; L2 a throw after the swap drops the previous due run's row
unannounced; L3 the newborn detours build a string under `noexcept`: all three only on an allocation failure in an observer behind
its flag, left.

**The merge** (2026-11-20): `63043d152` in `main`; `Test-Project -MergeGate` passed on the branch; the launcher's module rebuilt in
the main checkout with its tests (178 passed, `evidence/I-0141/artifacts/launcher-63043d152.log`); the start check D-0229 at 16:34
to 16:36, the ASUS, offline, restored: `passed` true, exit 0 (`startcheck-63043d152.log` beside it).

## Tranche 15: the haze in play at Archdragon Peak's bonfire, the effects' share (2026-11-21)

Opus 5.5, `claude/r-i141-t15` (the fresh R thread after the one of tranche 14). **The question** (tranche 14's first next step): the
native effects' smoke over the castle at the bonfire, denser and brighter than the original's mist: more effects, or the same
effects drawn otherwise, or the scene under them.

**Already in the evidence** (OBSERVED, `i141-t14-n1-c32000000-nat`, tranche 14's run without the King's revival, so without the
ceremony and in the zone's sunny state): the native's haze is there too (`sweep-1-60.png`), so the smoke is the map's effects
(present without the ceremony), not a remnant of the ceremony's. No original run of that state.

**The runs** (the ASUS 1920x1080 window, offline, no measure, all exit 0, the save restored; `build-t14o`, main's tree of tranche 14;
wrappers and logs `evidence/I-0141/artifacts/tranche-15/`): `i141-t15a-c32000000-{nat,orig}` (01:52 to 02:01, `Run-I141SmokePix -Dump
-Light -Life -ArmMs 75000`: every entity offered on pass 0x45 followed on the scene clock, D-0299; both games were closed by the
runner before the scene clock's 90 s, so no close rows and, natively, no press at 75 s) and `i141-t15b-c32000000-{nat,orig}` (02:03 to
02:12, `-Dump -KeySecond 58`: F9 in play, the next 3000 effect draws' kinds and blocks and 1500 entities offered with their cull answer). After the tool: `i141-t15c-c32000000-{orig,orig-noffx}` (02:40 to 02:52, `build-t15b`, Reference,
tranche 14's `run-t14k` protocol: the ceremony at the load, a shot a second for 99 s, the second with `reference-no-ffx-draw.flag`,
its rows `hooked` and `first_skip`) and `i141-t15d-c40000040-nat` (02:52 to 02:57, `build-t15b`, i140-e16's flags, Firelink's
cutscene 40000040).

**OBSERVED**:
- the entities in play are the same in both modes: each second from 60 s of the scene clock, the entities offered by kind and first
  mask (`Compare-I141InPlay.py`, `inplay-60s.txt` as `c7252aa86`'s reader writes it, over the 11 seconds 60 to 70 s both journals have, D-0299; the native's 70 s after a
  walk gap of 177.5 ms, perhaps undercounted): kind 3 mask 8 43 and 43, kind 3 mask
  FFFFFFFF 176 to 183 natively, 176 to 182 in the original (medians 179 and 180), kind 5 7 and 1 in both, kind 6 212 to 218 and 212 to 219. The 1500 entities
  offered after F9 at 58 s: culled and recorded alike by kind (kind 3 mask FFFFFFFF recorded 213 natively, 220 + 4 in the original;
  culled 488 and 484; kind 6 recorded 46 and 50, culled 594 and 583; the others equal). The lives table (lives offered after 60 s by
  place) lacks the lives still held at the journals' end (no close): not used;
- the draws and their constants are the same: the 3000 effect draws after F9 hold the same thirteen kinds in nearly the same
  numbers (the main ones blend 2, colour scale (0.75, 0.75, 0.75, 1), fog 1: 1244 natively and 1265; (0.75, 0.75, 0.75, 0.5) with
  the soft-sprite word 0.5: 666 and 680; blend 1 (0.75, 0.75, 0.75, 1): 533 and 519), and for every kind but one the
  `PSCommonBuffer`, `PSLightBuffer` and `cbFog` blocks have each word's median (and non-zero median) within 1 %, the integer-like
  words' most frequent value equal (`Compare-I141FxBlocks.py`, `blocks-compare.txt`);
  the exception, the kind (0, (1, 1, 1, 1), 0, 1) (75 and 61 draws), differs in two `PSCommonBuffer` words, +0xC and +0x58 (medians
  0.48 against 0.68 and 0.144 against 0.200);
- the automatic exposure natively stays at 0 (its log2 factor in every `native_auto_exposure` row of `i141-t15b-…-nat`, the zone's
  exposure 2 to 2.4): the native does not brighten this view;
- tranche 14's crops over the sweep's 85 to 99 s (`crops.sh`, ffmpeg's YAVG): the castle's mean 103.8 natively with the effects
  (`i141-t14h-…-nat`), 106.1 in the original (`i141-t14h-…-orig`), 83.3 natively without them (`i141-t14k-…-nat-noffx`, a still
  camera: the same 83.3 every second); the stairs' 93.9, 99.7 and 67.6; the whole picture 78.9, 85.8 and 70.2. So the castle is not
  brighter natively on average; natively its picture is flat and grey under the clouds where the original's is a lit, textured
  stone through a thin mist (`pair-sweep-60.png`, `crop-castle-58.png`, by eye).

**The tool for the original's share** (`d6887aac4`, `861fc2cf2`, `a81015dfe`; build `build-t15b`, `-Cache`, `-NoControls`):
`reference-no-ffx-draw.flag`, Reference runs only, the original's counterpart of `no-ffx-draw.flag`: the scene control's effect draw
24E790 returns at once, as its own four early exits do (READ, GhidraMCP: the gates, the epilogue 24EC8D setting neither RAX nor XMM0),
keeping its last write `[14496235C] = 0` where the gates pass (GhidraMCP lists that write and 24D000's, no read). That the run then
differs from a plain one by the effects' pictures alone is INFERRED (the skipped body holds the library's walk and its callees, the
GPU particles' 2A6A30 among them, and on the +0x590 branch pass-context writes, none read for side effects). Refused with a row, the
start stopped, beside `ffx-draw-observe.flag`, on a failed hook and with `original-probe.flag`; in a Native run the flag is ignored
with a row (the final review's L4). Natively `no-ffx-draw.flag` removes the GPU particles too (their world is built only with the
native effect draw, `21_effects.inc` `gpu_particles`, and their 2A6A30 hook is set only then, `22_setup.inc`; no
`native_fx_world_gpu` row in `i141-t14k-…-nat-noffx`), as the original's skip passes over 2A6A30.

**Step review** (a fresh `step-reviewer`, Opus 5.5 high, on `0bdbe83d5..d6887aac4`): no High. M1 the skip left the byte
`[14496235C]` at 1 for the run where the full path clears it: kept where the gates pass (`861fc2cf2`). M2 the reader missed the lives
held at the close and a spent row cap silently: it warns of both. M3 the reader's « alive » counted lives over the window, not those
present together, and dropped a second without the key: the lives table relabelled, the seconds counted with 0. L1 `original-probe.flag`
ignored the flag: refused there. L2 the refusal beside `ffx-draw-observe.flag` let the run go on: the start stops. L3 a failed hook
wrote no row: it does. L4 the comment omitted the gate sc+0x20: added. L5 the reader checked no mode and grouped unread positions at
the origin: both done.
**Second step review** (on `d6887aac4..861fc2cf2`): no High. M1 « only the draws are missing » claimed more than was read: the comment
now names the skipped body and marks the claim INFERRED (`a81015dfe`). L1 a failed `MH_EnableHook(MH_ALL_HOOKS)` stops the start with
the generic code and no row of this flag: left (no effect drawn, the start stops). L2 a second without any offer has no count row:
the reader prints the rows against the window's seconds. L3 an empty journal passed the mode check, and the rows' tag is the native
effect draw's, not the run's mode: an empty journal refused, the message says what the tag means. L4 `rows_left` 0 with exactly 30000
rows written warns falsely: « may be incomplete ». No third review of `a81015dfe` (a comment and the reader's labels).

**The original's share** (`crops.sh` over the sweep's 85 to 99 s, tranche 14's crops; `quad-99.png`: top the original | the native
with their effects, bottom both without). The luma differences are OBSERVED; reading them as the effects' share rests on the skip
removing the effects' pictures alone (INFERRED, above). The seconds are the runner's sweep shots on the wall clock after the load
(a still camera in steady play, not the scene clock of D-0299). The native's pair comes from tranche 14's runs on other builds and
observers (`i141-t14h-c32000010-nat`: `build-t14f`, `fog-build-observe.flag` and the cutscene tool, whose 32000010 starts after the
99 s; `i141-t14k-…-nat-noffx`), the original's from this tranche's `build-t15b` with `test-no-ai` alone.

| Mean luma (YAVG) | castle | stairs | whole |
|---|---|---|---|
| original, with its effects (`i141-t15c-…-orig`) | 106.9 | 97.0 | 86.3 |
| original, without (`-orig-noffx`) | 87.9 | 73.1 | 77.3 |
| the effects' share in the original | +19.0 | +23.9 | +9.0 |
| native, with its effects (`i141-t14h-…-nat`) | 103.8 | 93.9 | 78.9 |
| native, without (`i141-t14k-…-nat-noffx`) | 83.3 | 67.6 | 70.2 |
| the effects' share natively | +20.5 | +26.3 | +8.7 |

The effects add light of the same order in both modes (1.5 to 2.4 apart on the crops, 0.3 on the whole picture, from one run a
leg; the castle with effects reads 103.8 to 106.0 over tranche 14's native runs `i141-t14h`, `-t14m`, `-t14n`, `-t14k-…-nofogbanks`
and 106.1 to 111.0 over the original's `i141-t14h`, `-t14m`, `-t14n` and `i141-t15c`, so the shares run 20.5 to 22.7 natively and
18.2 to 23.1 in the original: the runs' spread is as large as the gap; the integrity review, L2). The stairs crop holds the
player's head and shoulders (darker natively, `i140-e11`). Under them the
native's scene is darker (castle 4.6, stairs 5.5, whole 7.1) and flatter: the castle crop's 10 % to 90 % luma (signalstats YLOW to
YHIGH, the 95 s shot) 63 to 113 in the original without its effects, 70 to 103 natively without them, two thirds of the spread; with
the effects 72 to 131 and 85 to 122. So the haze is not more or brighter smoke: it is the scene under the smoke, darker and of less
contrast natively, which the same smoke then greys more. Fog or lighting: the castle's deferred pixels take their fog in the
`light_composite` pass, which runs the game's own `GXLightComposite` programme (READ: `AcquireNativeLightingPrograms`,
`06_pass_programmes.inc`; `scene_pass_lighting.cpp`'s pass 3) with `cbFog` at b6 indexed by the pixel's key (`GBuffer3.w`), the
per-key blocks the same in both modes (tranche 14); so what can differ is its inputs: the lit image under it (the light D-0262
removes, each area's baked ambient and the sets' fill lights, the likeliest, INFERRED, workstream 12's) or the key each pixel
takes (not read). `RtForwardFog` (`rt_material.hlsli`) fogs the reflection rays' hits only (`rt_reflect.hlsl`), not the deferred castle.

**The kind-11 rules at Firelink** (OBSERVED, `i141-t15d-c40000040-nat` against `i140-e16-c40000040-orig`): natively the cutscene's
`s40_00_0040` binds as the original's (`native_gparam_scene_binding_source` area 240000040, kind 11, bank 13, weight 0, caller
CDF131, the identity, as `reference_gparam_file_bind`), the map's `m40_00_0000` with the same MapOffset transform (the 28 degree
yaw, (-203.6, 7.3, 33.5)); the sun in play the original's (the blend's (0.747261, 0.544639, -0.380748), `reference_sun_block`'s
first three words) and during the cutscene, with no light-set key, the same angle sun (0.591677, 0.681999, 0.429879) as the
original's `reference_shadow_source`. I-0140 tranche 7's rule (a cutscene file CE1DB0 does not list keeps its cap's own transform)
was witnessed for 31000010 and 39000020; it now holds for Firelink's 40000040 too. I-0138 part 4b's two INFERRED rules for kind-11
files (an unbound area's sun direction and fog position turned by the map's MapOffset, a cutscene Snow record's textures named with
the map's prefix) are not exercised here: the cutscene's sun is the record's Angle (no light-set key, so no bank direction to turn),
the fog positions and Snow records not compared. The pictures of this run not compared.

## Reviews at the merge of tranche 15 (2026-11-21)

**Final review** (a fresh `final-reviewer`, Opus 5.5 xhigh, on `main...2befe1586`): no High; the four code criteria held (the gates
and epilogue against `DS3-1152-F-0024E790.asm.txt`, the refusals, the flag's harness cleaning, the reader's fields) and every
number of the section recomputed. **M1** « natively `no-ffx-draw.flag` leaves the GPU particles drawn » was false (the GPU world and
its 2A6A30 hook exist only with the native effect draw; no `native_fx_world_gpu` row in the no-effects run): corrected in the
section and the Uncertainty. **M2** the Conclusion kept tranche 14's « denser and brighter » smoke beside « the haze is not the
smoke », and « so it is the map's effects » read as the haze: the tranche 14 bullet defers to tranche 15, the sentence names the
smoke (tranche 14's 101.8 against 94.7 was one shot; over 85 to 99 s the castle is 103.8 against 106.1). **M3** STATE's « no native
defect » for a cause left open: « no fix », the cause open. **M4** `build-t15a.log` and `build-t15b.log` of tranche 9's evidence
overwritten by this tranche's builds: the loss recorded at that Artifacts line (the third such loss; the next builds take names
with the tranche and a letter of their own, `build-i141-t16a`). **M5** I-0138's two kind-11 rules, not exercised, had left the next
steps: back in the Suggested next step and STATE with what would exercise them. **L1** « word for word » for a median-within-1 %
comparison: the comparison named. **L2** the effects' « share » read from an ablation INFERRED to remove the pictures alone: the
attribution marked INFERRED. **L3** the reader took each journal's own seconds (60 to 70 s natively, 60 to 82 s in the original),
said « to the close » without one, ignored `gap_max`: it compares the seconds both journals have, names the end and the seconds
after a gap over 100 ms (the native's 70 s, 177.5 ms); `inplay-60s.txt` written again, the medians unchanged. **L4** the flag was
silent in a Native run: a row says it is ignored (`22_setup.inc`). **L5** ROADMAP and the index say « merged » before the merge:
left, written for the merge commit; « the smoke's end and its burst » now « its burst read ». **L6** the crops' clock and the
native leg's other runs: stated above the table. **L7** ROADMAP's « Left » lacked m32's castle reading: added (its keys, since the
deferred fog is the game's own composite, READ by the pilot after the review's brief).

**Step review of those fixes** (a fresh `step-reviewer`, Opus 5.5 high, on `2befe1586..7d5c24ea1`'s `22_setup.inc` and reader): no
High; the Native-mode row only logs (no return, no hook; `Mode` holds only Reference and NativeBootstrap), the original-probe
refusal is not shadowed, the per-second table uses exactly the common seconds. **M1** the lives table and totals still set the
native's 60 to 70 s beside the original's 60 to 82 s: the lives are now those offered in the compared seconds' span (60 to 71 s:
89 natively, 152 in the original; still unequal, see the second review). **L1** two rows in one
second: the writer makes one a second (`life_counted`); a journal holding two keeps the last, with a warning. **L2** `gap_max` is the
longest gap between two offers since the row before, not the gap just before it, and a row under 100 ms may still miss an entity
offered early in a long walk: the label and docstring say so. **L3** the labels (each journal's seconds counted from --from, the
docstring's span): rewritten. **L4** a row without a mode tag printed as missing: the refusal shows every tag. `inplay-60s.txt`
written again; its per-second medians unchanged.

**Its second step review** (on `7d5c24ea1..07c23587c`, the reader): **H1** the lives table still compared unequal coverage: a
journal without its close lacks the lives held at its end (natively nothing after 69.55 s, while the original's span ran to 71 s:
about 40 % of the 89 against 152 an artefact). **M1** the span was a continuous range, not the common seconds; **M2** the docstring
said « within its last 100 ms » where the row is written at a second's first offer for the 100 ms before it; **L1** a dead
variable; **L2** two rows in one second mean a reset clock or two runs; **L3** the layout. Answered (`Compare-I141InPlay.py`): the
lives table is printed only when both journals hold their close, over the lives whose offers overlap a compared second; a
journal with two rows in one second is refused; the docstring says when a row is written; `inplay-60s.txt` written again (its
lives table now left out, said). Tried on a pair of hand-made closed journals (a life in a second only one journal has left out);
no third review (the reader's last change removes a table and refuses a case).

**Integrity review** (the `integrity-reviewer`, on `main...7d5c24ea1` and the records): no High, no Medium; no test, gate or
threshold touched, no scene-specific code, the flag named wherever its run is used, every number recomputed. **L1** `inplay-60s.txt`
was not the reviewed commit's reader's output: the RESULT names the reader commit that writes it. **L2** « within 1.5 to 2.4 » from
one run a leg while repeat runs differ by 2 to 5: the spread over every same-zone run and the shares' ranges given. **L3** the
stairs crop holds the player: said. **L4** the code that ran (`build-t15b`) is not the final tree's: the Artifacts line says so and
names the final build. **L5** the index and ROADMAP stated the reading without its INFERRED: marked; the Conclusion's « the haze is
not the smoke » now « not more smoke ».

**The merge** (2026-11-21): `3cac46d8c` in `main`; `Test-Project -MergeGate` passed on the branch; the branch's final tree built
complete with its tests (178 passed, `evidence/I-0141/artifacts/build-i141-t15-final.log`); the launcher's module rebuilt in the
main checkout with its tests (178 passed, `launcher-3cac46d8c.log`); the start check D-0229 at 03:40 to 03:41, the ASUS, offline,
restored: `passed` true, exit 0 (`startcheck-3cac46d8c.log`; a first call at 03:39 never started, its shell redirection mistyped).
Disk rotation: nothing older than 48 h, 509.5 GB free.

## Tranche 16: Yorshka at x1, I-0138's two kind-11 rules, the player at the Road of Sacrifices (2026-11-21)

Opus 5.5, `claude/r-i141-t16` (the fresh R thread after tranche 15's). The Suggested next step's first three items.

**Church of Yorshka at x1** (OBSERVED, no new run; `yorshka-levels.ps1`, `yorshka-levels.tsv`). Three runs were compared on ffmpeg's
YAVG of the game window's client (as `Get-AbLevels.ps1`), each one run a leg, with tour stills taken at fixed delays after a key
press (not at the same moment of the game's clock, D-0299):

- tranche 10's x1 run (`i141-ab-x1-z36-…-nat`, target 0.20);
- I-0138 tranche 7's sheet run in the original colorimetry (`i138-t7v-original-z36-…-nat`, `build-t7v`, x1, target 0.12);
- the tour's original (`i139-t1-z36-…-orig`, the only original run of this zone).

| Pictures | Original | Native, target 0.12 | Native, target 0.20 |
|---|---|---|---|
| arrive, stay | 28.9 | 31.0 to 31.2 | 39.4 to 39.7 |
| the eight sweeps | 31.6 to 36.2 | 32.9 to 37.6 | 41.7 to 45.2 |

The native is 9.7 over on average under the target 0.20, and over by 0.7 to 2.3 under 0.12.

- **Arrive and stay.** The exposure held `log2_factor` at about 0.24 (`mean` 0.111, under the target) from frame 4800 to 7680, the
  arrival and the stay. A x2^0.24 linear lift at about gamma 2.2 takes 28.9 to about 31.2, the value measured. So the +2.2 there is
  D-0294's own lift under 0.12 (INFERRED).
- **The sweeps.** The factor fell to 0.00 to 0.01 from frame 8160 to 8640, then rose again (0.096 at 8760, 0.115 at 8880) under
  the last sweep. The sweeps sit 0.7 to 1.8 over; about 1 of sweep 4-2's +1.7 is that rising lift (the integrity review's
  timing).
- **Spread.** The original's tour runs repeat within 0.5 of 255 on the whole client where a zone has two (z01, z16, z61; the final
  review's reading). So the sweeps' residual is above the original's repeat. The native's own repeat was never measured.

Tranche 10's « brighter even at x1, another cause » was mostly D-0280's lift toward 0.20, gone with D-0294. What remains is
D-0294's lift on the darker arrival views and about +1 on the sweeps, open.

**I-0138 part 4b's MapOffset rule** (READ).

- **The rule.** `Banks::OwnSun` and `OwnFog` turn an area's sun directions by the map's MapOffset yaw, and take its fog position
  from the MapOffset, only when the area has no layer and no loaded bound file. The yaw and the position come from `mapoffset.txt`
  in the bank folder (`Banks::Load`).
- **Dead in play.** In the game the bank folder is memory-only (`ds3hostio::MemoryOnly`, `18_resource_admission.inc`). It holds
  only the GPARAM files `PublishNativeGparamBanks` publishes, and no `mapoffset.txt`. So in play `Impl::Offset` is always null, the
  rule never turns anything, and an unbound area's fog position is 0. No unit test or tool in the repository writes that file
  (`Dump-MapOffset.ps1` was archived). The offsets' parse and MapYaw's turning branch run in neither play nor any test, and are
  left for a cleanup.
- **Every area is bound.** Since I-0140 tranche 7 every cutscene file is bound by kind 11 (with the map's transform where CE1DB0
  lists the cutscene, otherwise its cap's own `+0x80`), and the maps are bound by kind 7.
- **The observer.** What is left is whether a lookup ever answers an unbound area's file. The original's bank holds the bound
  files' records only, and a key absent from it reads as {1, 0} (140032770). `Banks::UnboundAnswers` lists the first 32 Sun or Fog
  keys answered so, by (area, id, group, `fade_view`), each with whether a MapOffset turned it. The module writes
  `native_bank_unbound_answer` for each at a world collect, so a key noted after the last collect is not written. The header
  (`light_producers.h`) gives the rule's exact cases.
- **Its test.** `glow_bank_test` covers it with copies of default as `m99_*`:
  - an unbound area's light set and fog are noted once;
  - an area bound before any lookup adds nothing;
  - default's {1, 0} is never noted;
  - the list stops at exactly 32.
- **Not covered.** The other lookups through `Impl::File` (Shadow, FindGroupParam, GroupEntries and the post settings and Snow
  through it, BackgroundColor, EnvColorScale) are not observed.
- **Unreachable here.** SelectVariant runs only behind `g_banks`, which no caller sets (`UseBanks`), so the variant map is empty in
  the module.

**I-0138 part 4b's Snow rule** (READ; data, `snow-records.sh`, `snow-records.txt`).

- **The name.** A `Snow` record's name string sits at record `+0x28`, from which 131EA0 makes the `%sDeposit_*_%04d` texture
  names. The bank's file receiver 12EA70 copies it from the GPARAM manager's entry for that file (94DA0: the entry's `+0x298`, or
  `+0x350` when its `+0x348` is set). The built-in record's name is « m22_00 » (12DCC0).
- **The prefix is already READ.** I-0138's tranche 6 (K-0495): `CSGparamResCap_Parse_140D272C0` calls `FUN_140D28490`, then
  `FUN_140095100`, for every parsed file. That stores `"m%02d_%02d_"` at the entry's `+0x350` for kind 7 and kind 0xB alike, and
  sets `+0x348`. So a cutscene file's Snow name carries its map's « mAA_BB_ », as the native's `NativeSnowAreaPrefix` gives.
- **The records.** Nineteen cutscene files hold a Snow group: s30_00_0010, s30_00_9000, s31_00_0010, s32_00_0000, s32_00_0030,
  s33_00_0000, s33_00_0020 to 0042, s34_01_0000 and 0010, s39_00_0030, s51_00_0000. Each one's Snow group equals that of every
  Snow-bearing file of its map, value for value, and all 54 Snow records have Texture:ID 0.
- **Inert.** With the same name and the same values, a cutscene key reaching the snow draw would change nothing. The rule is inert
  by data.
- **The runs.** The original's `reference_snow_blend` rows over every run that has them hold only « m30_00_ » and « m22_00 ». In
  30000010 (`i139-t3c-c30000010-nat` and this tranche's run) the native's snow keys are the map's `{130000000, 0}`. This scene
  cannot tell the map's record from s30_00_0010's: they have the same name and the same values.

**The runs** (2026-11-21 04:10 to 04:23, on the ASUS; `build-i141-t16a`, f8dc87835's native tree; `run-t16a.ps1`, `runs-t16a.log`).
All three exited 0 with no fault and the save restored:

- the tour's row z16, natively, with `gbuffer-pictures` (`i141-t16a-z16-road-of-sacrifices-nat`);
- the High Wall's cutscene 30000010, natively and in the original (`test-no-ai`, plus `ffx-draw-observe` in the original;
  `i141-t16a-c30000010-{nat,orig}`).

What they show:

- **No unbound Sun or Fog answer** (OBSERVED). Neither native journal holds a `native_bank_unbound_answer` row. Even that build's
  reader, before the review's M1, could only miss a row after writing one, so a journal with no rows is exact. Other lookups are
  not observed (above).
- **Snow during 30000010** (OBSERVED).
  - The original's `reference_snow_blend` rows (callers 10466B and 23F79) hold only « m30_00_#0 » (its textures bound) and
    « m22_00#0 » (the built-in record, its textures zero).
  - The native's `native_snow_rows` hold the map's `{130000000, 0}` (`m30_00_0010`, named `m30_00_Deposit_*_0000`), the built-in
    (« m22_00 »), an empty key, and before the scene m40's `{140000000, 0}` answered by the built-in record.
  - Alike in both modes, as the data says it must be.
- **The player at the Road of Sacrifices' bonfire** (OBSERVED; `player-crops.sh`, `player-crops.txt`, `pair-z16-stay.png`). YAVG of
  255 on the tour's still shots (arrive, stay), not at the same moment of the game's clock:

  | Crop | Original (`i139-t1`, `i140-e15`) | Native x2 (`i139-t1`, an older build) | Native today (x1, target 0.12) |
  |---|---|---|---|
  | Player's back | 86.1 to 88.1 | 58.7 to 61.0 | 46.0 to 46.7 |
  | Trousers | 87.2 to 89.5 | 61.0 to 61.1 | 47.4 |
  | Whole client | 59.6 to 60.2 | 42.5 to 42.7 | 37.5 to 37.6 |

  - **The colour.** The native player is warmer: mean RGB about (64, 40, 33) against (98, 82, 76).
  - **The exposure at the stills** (frames 6960 to 8040). Today it did not lift: `log2_factor` 0, mean 0.169 to 0.170 over the target.
    The x2 tour's column was lifted: `log2_factor` 0.186 to 0.189 toward D-0280's target 0.20, mean 0.187.
  - **The whole view.** It is darker, not the player alone: the player is about 3 % of the frame.
  - **Since the x2 tour.** The view fell 12 %, and the scene's mean 9 % (0.187 to 0.170). That comparison mixes three changes: the
    local lights x2 to x1, the target 0.20 to 0.12, and a lift of about 0.19 log2 (about +6 % of YAVG) present then absent. So it
    is not a witness of the local lights alone. A run of today's build with `rt-local-x2.flag` would separate their share.
  - **The rest of the gap.** That it is the light D-0262 removed (the baked ambient and the sets' fill lights) is INFERRED. It is
    the same reading as the Archdragon Peak haze in tranche 15, and the same lever: workstream 12's first step, each area's
    ambient as a traced source.

**Step review** (`step-reviewer`, 3b89f029d..f8dc87835): no High; two Medium and six Low.

- M1: `UnboundAnswers` read the generation after releasing the lock, so a caller could skip a row. Fixed: with `out`, the copy and its
  generation are read under one lock (since replaced by the lock-free list, the second fix's L2).
- M2: the test's « bound: no unbound answer » could not fail, because its keys had been noted before. Fixed: a second copy
  m99_01_0000 is bound before any lookup and its keys add nothing, while the same id 10 on the unbound area is noted.
- L1: `OwnSun` noted a key before its last refusals. Fixed: it notes once the record is whole.
- L2: the header's comments were wrong in three places, now corrected in the header and in `Load`'s line. Dedup includes `fade_view`.
  `File()` may answer an earlier layer's or a variant's file. `fade_view` means the earlier layers' first file is not in the bank.
  `Load`'s line also named a tool that no longer exists (`Dump-MapOffset.ps1`).
- L3: an override's sources are noted like any lookup. Documented in the header.
- L4: the global lock. Left as it is: it is taken only on an unbound answer, and none appeared in the runs. Documented.
- L5: the new statics are outside the thread-affinity checker. No fix asked.
- L6: the test depends on process-wide state. Its comment now says so.

**The fix's step review** (`step-reviewer`, f8dc87835..bd4c28e15): no High and no Medium; criteria 1, 2, 3 and 5 held, and five Low.

- L1 to L3: three header phrases said more than the code. A noted answer never comes from an earlier layer's file. A `Sun` made of
  two OwnSun records can be refused after its first is noted. The lock is also taken by `UnboundAnswers` with `out`, and a claim
  about runs did not belong in the header. All three were rewritten.
- L4: the lock on every unbound lookup. An atomic flag skipped it once the 32 entries were held (since replaced, the second fix's L2).
- L5: the test counted the new entry without checking it. It now checks the entry {199000000, 10, light set} and that no entry
  names the bound area.
- L6: the runs' « no unbound answer » came from `build-i141-t16a`, before M1's fix, whose reader could miss a single note in a
  window a few instructions wide. The final review showed that reader could only miss a row after writing one, so a journal with
  no rows is exact.

**The second fix's step review** (`step-reviewer`, f57771ae9..f3197bed0): no High, one Medium, five Low.

- M1: the header said a noted answer comes from a variant of the area. SelectVariant can also settle on a global state file
  (eclipse, mapoverride_000N, mapbase, default), and a Fog lookup without a fog variant takes the sun's. The header now says so.
- L1: a Fog built from two records can also be refused after its first is noted. The clause now covers Sun and Fog.
- L2: the flag saved the lock only once the list was full. Entries are now published before their count and never rewritten, so a
  key already listed costs no lock.
- L3: the cap was not tested. Thirteen more copies fill the list to 32, and a key past it changes nothing.
- L4: an area bound to a file missing from the bank counts as unbound. Stated in the header.
- L5: SelectVariant's trial lookups note like any other. Stated in the header: a listed key may never answer so itself.
  Suppressing the trials would also hide real lookups on other threads during a trial.
- The build of that fix also failed on C4456 (the test's counter hid an earlier `ten`, which clangd did not flag). Renamed.

**The third fix's step review** (`step-reviewer`, f57771ae9..abe23e5e5): no High, one Medium, four Low; the list's memory ordering,
the lock-free path and the unchanged records held.

- M1: the cap test filled 39 candidates for 32 places, so it could lose up to seven fill keys unseen. It now fills exactly 32
  (three earlier entries, nine areas of three keys, two light sets of a tenth), checks each lookup answered, and checks the 33rd
  key is not listed.
- L1: the comments on the copies disagreed. Now 02 to 11 fill, and there are no unused copies.
- L2: « the answer unchanged » was not checked. The label now claims only what the test checks.
- L3: a lookup on another thread can answer a candidate SelectVariant is still trying. Stated in the header.
- L4: for an overridden key, the key noted is the source's. Stated in the header.

**The fourth fix's step review** (`step-reviewer`, d7ed00242..0bb8628b4): one High, no Medium, three Low.

- H1: the folder's count was wrong (twelve for thirteen files), so the block was skipped and the build's test failed. The build
  had already shown it. Fixed: thirteen files.
- L1: the exact count of 32 needs the list empty at the block's start. Now checked.
- L2: a blend notes both of its sources. Stated in the header.
- L3: the header's lines were reflowed.

These last changes touch only the test and comments. The final review on the merged tree covers them.

## Reviews at the merge of tranche 16 (2026-11-21)

**Final review** (`final-reviewer`, main 061c619f8...f5d0c462b, the tree built as `build-i141-t16b`): no High, three Medium, seven
Low. The code met criteria 1, 2, 3 and 5; criterion 4 (the records) did not hold.

- M1: Yorshka « within the runs' spread » did not hold. The original's runs repeat within 0.5, not 2. The 0.24 lift fell on the
  arrival and the stay, not the sweeps. D-0299 was not applied, and the numbers were saved nowhere. Restated (« Tranche 16 »,
  `yorshka-levels.tsv`), with STATE and ROADMAP corrected.
- M2: the Snow prefix was already READ (I-0138 tranche 6, K-0495), and 30000010 cannot tell the two records apart. Restated: the
  rule is inert by data (`snow-records.txt`, checked), and the « neither mode » sentence is dropped.
- M3: the observer covers Sun and Fog only. The records now say so, and the other lookups are listed as not observed.
- L1: STATE's rows contradicted each other, and its « Updated » line named E's merge. Rewritten.
- L2: stale lines in the RESULT (`fade_view`, the test's first form, « under one lock »). Rewritten or marked as replaced.
- L3: the header described SelectVariant's path, which the module never runs (no `UseBanks` caller), and the MapOffset branch is
  dead code. One header clause now says so; removing them is left for a cleanup.
- L4: the separate locks in `OwnSun` and `OwnFog`, older than this tranche. Recorded under Uncertainty.
- L5: rows are written at a world collect only. Stated in the record.
- L6: « the drop matches D-0293's local lights » compared no quantity. Restated with the numbers, and the separating run named.
- L7: the module comment's « one atomic read » corrected. The observer stays as long as tranches 17 and later read its rows.

The last two answers change only comments in `light_producers.h` and `12_scene_collect.inc`. The built tree is `build-i141-t16b`,
the same code.

**Integrity review** (`integrity-reviewer`, main 061c619f8...144088239): no High, one Medium, two Low; no weakened test, no
scene-specific case, nothing cut during a measure.

- M1: the Road of Sacrifices' exposure figures were the runs' last rows, not the stills'. At the stills, today's mean is 0.170 with
  no lift, and the x2 tour's is 0.187 with a lift of 0.19 toward 0.20. Restated (checked in both journals). The fall since the x2
  tour is now named as mixing three changes. STATE's row no longer gives it as a lights reading.
- L1: Yorshka's factor rose again under the last sweep. Restated.
- L2: the native's snow keys also held m40's `{140000000, 0}` before the scene. Added.

**The merge** (2026-11-21 05:40): `e7aad66a3` in main; the launcher rebuilt with its tests (`launcher-e7aad66a3.log`), the start
check passed (`startcheck-e7aad66a3.log`, `evidence/I-0141/artifacts/`); the disk rotation found nothing older than 48 h.

## Tranche 17: the key each castle pixel takes at Archdragon Peak's bonfire (2026-11-21)

Opus 5.5, `claude/r-i141-t17`. The Suggested next step's first item, Thomas's « Oui vas y » (2026-11-21 03:47): R6's « per-pixel
set » witness at m32's bonfire, after the ceremony 32000000, where tranche 15 found the castle and the player darker and the castle
flatter natively under alike effects.

**The question.** K-0451: the G-buffer programmes write the instance envData (record +0x60, written by 5B3D0 for pass state types
6 and 7) to GBuffer3.w, and the composite and the sun pass index the light and fog sets by it. 5B3D0 takes envData from one of two
sources chosen by `[root+0x42A0]`: set, the group's `+0xF0` {list, index}; clear, FUN_14014DA10's map keyed by the light key a
(`+0xF8`) alone. Natively the set is the registry's slot of the group's full key block (light a/b, fog a/b and both blend weights,
NativeLightSetIndex). If the original's clear branch gave a castle piece an entry whose fog key or weights differ from its own
block, the native's pixels would take another key there: a per-pixel difference behind the darker castle.

**The witness** (`pixel-keys.flag`, with `light-observe.flag` for the original's hook), in its final form (the reviews below):

- `reference_pixel_key` (original, `04c_scene_witnesses.inc`): for each type 6 or 7 record, the envData, the branch byte, the
  group's own key block and 137910's entry at that envData (`[[root+0x41F8]+0x88]+0x30·env`, null when envData is not below the
  count), read at once. Once per 60-second QPC window and (model, branch byte, the key words of both blocks); envData, the count,
  the blend weights and the group's address (a per-frame copy) are written, not keyed. At most 40,000 rows, then a
  `reference_pixel_key_full` row.
- `native_pixel_key` (`05_selected_scene.inc`): the set, `set_full` and the group's own block, once per window and (model, group,
  key words, `set_full`). `native_pixel_key_gbuffer` (`12_scene_collect.inc`): every 300th main collect, the models with a G-buffer
  row. `native_pixel_key_list` beside each world picture (every 1200th world frame with the flag): that frame's scene's key list
  by set index and the render extent, so a G-buffer picture's GB3.w reads as a key.
- `Compare-I141PixelKeys.py`: both modes aligned on each mode's arrival (its first row under the area's key, `--arrival-key
  132000000`), whole windows from 10 s after it, the same number in both; the native restricted to the models with a G-buffer row;
  per model, the key blocks its rows take; the pictures' GB3.w histograms.

**The runs** (both 840 s caps, both ending after about 5.5 min; tranche 14m's protocol: Eta's save, the ceremony's flags reset, the
travel to m32's bonfire, the look sweep; the ASUS 1920x1080 window, offline, both saves restored by hash, no measure):

- `i141-t17a` (`run-t17a.ps1`, `build-i141-t17c` of `5105b1a75`, 06:08 to 06:20): the first form, without the G-buffer list (its
  original rows keyed the group's address and envData: 31,804 rows).
- `i141-t17b` (`run-t17b.ps1`, `build-i141-t17g` of `7b7bc9430`, 06:55 to 07:06): the final form but for the G-buffer list's
  passes (engine pass 5 only; 5 to 7 since, the last step review's M2): 1,094 original rows, 13,506 native frames.
- Neither run reached a cap; the registry never filled (no `native_light_sets_full`, no `set_full`).

**Results** (`pixel-keys-t17b.txt`, `pixel-keys-t17a-arrival.txt` in `evidence/I-0141/artifacts/tranche-17/`). The original's side
is its records (OBSERVED); which key its pixels take follows from them through K-0451 (INFERRED: no G-buffer of the original read).

- **The original takes each group's own block.** In both runs `[root+0x42A0]` was 1 on every row (the `+0xF0` source, never the
  light-key map), every row had its entry read, the entry at envData equalled the group's own block (all eight key words), and
  every row was type 6. Every blend weight was 0 in both modes (no blend exercised).
- **Where both draw a model, the same keys.** `i141-t17b`, two whole windows (from 41.7 s after the arrival in the original, 18.7 s
  natively, the windows' phases): 200 of the 203 models in both modes take the same key blocks. `i141-t17a`, one window: 191 of
  194. The same three differ in both runs, each holding the original's key and more natively: `m32_00_00_00_004190` and `004210`,
  natively also 12 groups under fog `{132000000, 60}` (set 6, which no native picture's pixel carries), where the original draws
  those models' groups only under `{132000000, 0}`; `o324014`, natively also two groups under `{132000000, 0}`. Per model the native holds the
  original's key block and one more; whether those are groups the original does not draw or another key for the same groups is not
  read (the original's group address is a per-frame copy, so groups are not matched across modes).
- **The native keeps drawing what the original stops drawing.** 143 models (`i141-t17b`; 139 m32 map pieces, 3 objects, `c3140`)
  have prepared native G-buffer rows in the compared windows and no original row there; 8 native models with a set, absent from
  every G-buffer sample, were left out. The original records 137 of them at type 6 in its arrival window, under the same key
  blocks, then no more: its type-6 models fall from 378 in the window holding the arrival (up to 41.7 s after it) to 203 in each
  later window, where the native holds 346. Of the 143, 60 take set 3 (`{132000000, 0}`), 51 set 6 (fog `{132000000, 60}`) and 22
  set 7 (fog `{132000000, 50}`); no pixel of the native pictures after the arrival carries set 6 or 7, so those 73 add nothing to
  the pictured frames. Why the original stops drawing them: tranche 18 (its occlusion for 137, INFERRED; the LOD scale for c3140).
- **The player's face parts** (`FC_M_0000`, `FG_A_0100`…) are type 6 rows in the original, under `{132000000, 100}`, as natively.
- **The pictures.** After the arrival, GB3.w over the default crop (300,120 560x300, the castle's tower and arch) takes set 3,
  `{132000000, 0}`, on 97.6 % of its pixels and set 4 (`{132000000, 31}`, `i141-t17a`'s index; set 5 in `i141-t17b`) on the rest; before it, m40's `{140000000, 0}`. Set 0 also
  holds the pixels no G-buffer row wrote.

So the per-pixel key is not where m32's castle differs: where both renderers draw a castle piece, they index the same light and fog
sets (INFERRED for the original's pixels, from its records). Tranche 15's reading stands: under alike effects and alike keys, the
difference is the light the sets give, D-0262's removed raster light (the baked ambient, the sets' fill lights; INFERRED,
workstream 12's). The sets' cbLight and cbFog values as the composite reads them on that frame were not compared; the native's
extra pieces are tranche 18's.

**Step reviews.**

- First (`7a3e1fd46..e067909ba`): no High; three Medium, four Low. M1 the native list was the live registry, not the picture's
  scene's: fixed (`NativeSceneLightKeys(selected)`, the scene's `light_keys`). M2 the blend weights in the dedup key could fill the
  cap during the ceremony: fixed, both witnesses keyed on the key words with a 60 s window. M3 the script's « last state » and its
  fallback to the own block said more than the data: fixed (final windows, « entry unread » apart). L1 failed reads written as
  values: fixed, every read required. L2 NaN broke the JSON: fixed (`PixelKeyNumber`). L3 the native set assumed equal to the own
  block: answered in the script. L4 the 16:9 guess and unclipped crops: fixed, the logged extent and clipped crops.
- Second (`e067909ba..6ba71034d`): one High, one Medium, five Low. H1 the native dedup still held the set, which follows the blend
  weights (NativeLightSetIndex keys on all ten words): fixed in `a4f32f54d`, the set written, not keyed. The run's build had H1; its
  native rows (4,730) stayed far under the cap, so the run's data hold. M1 the script took the native key from the next list in the
  journal, or the own block without one: replaced, the key is the own block by construction of NativeLightSetIndex, and set-0 rows
  are kept apart when the registry was full. L1 the window could move back: forward only. L2 the cap was silent: one `*_full` row,
  and the script stops its windows before it. L3 envData's read unchecked: required. L4 set 0 mixes unwritten pixels: stated
  above. L5 the new statics outside the thread-affinity check: under their mutex, as the functions' others; left.
- Third (`6ba71034d..a4f32f54d`): no High, two Medium, two Low. M1 the original's dedup still held envData and the count,
  which 137EF0's per-frame dedup on all ten words (t included) moves during a blend: fixed in `6a7252844`, both written, not keyed.
  M2 the registry's full fallback was invisible per row (set 0 is also a real slot): each native row now writes `set_full` (set 0
  not holding its key, `NativeLightSetHolds` under the registry's lock), kept apart in the dedup, and the script reads that flag.
  L1 a row timed in an older window could still insert into the current one: dropped. L2 the cap row's time is the 40,000th
  row's, not the latest: the script stops before that window; noted.
- Fourth (`a4f32f54d..6a7252844`): no High, no Medium, six Low. L1 the « all written » comments and the weights finding: the comments now say the first sample's; the weights finding became « every blend weight
  was 0 » (the final review). L2 the script's header: rewritten. L3 the RESULT's
  missing third review and builds: written. L4 the branch byte not keyed: keyed (`seen_key[0]`'s second byte). L5 whether envData
  is below the count stays keyed and can flip once in a shift: said in the comment, the script keeps « entry unread » apart. L6
  `NativeLightSetHolds` takes the registry's lock for every set-0 draw with the flag: left (flag only, no measure with it).
- Fifth (`6a7252844..7b7bc9430`, the final review's code findings): no High, three Medium, four Low. M1 the witnesses' 60-second
  windows have a phase against the arrival that differs between runs, so a span cut inside windows compares different samples:
  the script now takes whole windows only, the same number in both, and prints each phase. M2 the G-buffer list kept engine pass 5
  where the G-buffer rows are passes 5 to 7: fixed (`i141-t17b` ran with pass 5 only; a model drawn only at 6 or 7 would have read
  « only in the original », and none did). M3 an empty restriction was reported as one: now said, with the samples' count. L1 the
  native's arrival now among the G-buffer models, the area word named. L2 a model drawn only between two 300-collect samples is
  missed: said in the comment. L3 the labels and `only_n` without the registry-full models: fixed. L4 the 04c comment's reads:
  completed.
- Sixth (`7b7bc9430..96adf4151`): no High, one Medium, six Low. M1 « groups the original does not record, not another key » was
  more than the data say (groups are not matched across modes): restated. L1 the script's windows from `qpc_ms` printed at 0.1 ms
  where the code uses the unrounded value: left (a row within 0.05 ms of a boundary). L2 the window's length repeated in the
  script: left, said in its header. L3 the labels: fixed. L4 an empty G-buffer list and the arrival's message: fixed. L5 the
  windows' ends: printed. L6 « draws » means prepared G-buffer rows: said.

## Reviews at the merge of tranche 17 (2026-11-21)

**Final review** (`final-reviewer`, main 7a3e1fd46...285ad0a45): one High, three Medium, six Low. The code held (the decompile's
offsets, the flag, the reads, the windows, the caps); the comparison and the records did not.

- H1: the comparison paired each run's last three wall-clock minutes, which straddled the travel at different offsets (D-0299): the
  « 377 of 378 » held m40's models and the pre-travel time, `o000250`'s difference was before the travel. Fixed: the script aligns
  both modes on each one's arrival and takes whole windows after it; recounted (191 of 194 in `i141-t17a`), then run again with the
  final witness (`i141-t17b`, 200 of 203); the RESULT, STATE and ROADMAP rewritten on those figures.
- M1: the original's group address is a per-frame copy, and keying it multiplied the rows (16,527 in the arrival minute, the cap
  in about 8 minutes): written, not keyed (`i141-t17b`: 1,094 original rows).
- M2: the records' claims: the « 47 native-only player parts », the face parts « never at type 6 or 7 » (they are, under
  `{132000000, 100}`), the 29,774 rows' keys, the weights (all 0, no blend exercised), OBSERVED for the original's pixels
  (INFERRED from its records): each restated.
- M3: the native witness counted every model that takes a set, forward ones included: `native_pixel_key_gbuffer` lists the models
  with a G-buffer row and the script restricts to them; the native-only count stated (143).
- L1 STATE's rows before the merge (the pilot branch, the merge hash, the « Updated » line) and the investigations index's row:
  rewritten at the merge. L2 ROADMAP's line put the result in « Left »: moved. L3 the builds in Artifacts: listed, with
  `build-i141-t17-final`. L4 the script: unparsed rows counted, the registry-full fallback only after its line, those models apart,
  whole windows. L5 the original hook's cost under the flag (the name and a mutex for each type 6 or 7 record): left: the flag runs no measure; its cost in Reference mode not measured. L6 the entries and the
  count read apart while 137EF0 may rebuild them: left, no row differed.

**Integrity review** (`integrity-reviewer`, main 7a3e1fd46...96adf4151): no High, three Medium, two Low; no weakened test, no
scene-specific case in the code, nothing cut during a measure.

- M1: `build-i141-t17-final` predates `7b7bc9430`: the final tree's build is `build-i141-t17-final2`, now listed.
- M2: « most take set 3 » was false (60 of 143; 51 set 6, 22 set 7, and no picture pixel carries 6 or 7): restated.
- M3: the summaries' « never sees » dropped that the original records 137 of the 143 in its arrival window and then stops:
  restated in the Results, Uncertainty, Suggested next step and STATE (checked: 378 type-6 models in that window, 203 after).
- L1: set 4 is `i141-t17a`'s index, set 5 in `i141-t17b`: said. L2: the INFERRED label on the summary's « not the key choice »
  and the index's row: added; the intermediate output listed.

**The merge** (2026-11-21 07:25): `8be57c2ce` in main; the launcher rebuilt with its tests (`build-launcher-8be57c2ce.log`, 179 PASS),
the start check passed at 07:33 (`start-8be57c2ce.log`, `evidence/I-0141/artifacts/`; the world at 3.2 s, no refusal, restored); the disk rotation found nothing older than 48 h (501.7 GB free).

## Tranche 18: the 143 models only the native draws at Archdragon Peak's bonfire (2026-11-21)

R9's idea, Thomas's yes (2026-11-21 11:09, through the coordinator): why the original stops drawing, after the arrival, the 143
models (139 m32 map pieces) the native keeps in its G-buffer (tranche 17, `i141-t17b`). Two runs, a reading and a witness for
Reference runs alone (`reference-no-occlusion.flag`); no fix.

**The counts.** The models with a pixel-key row, per QPC minute window (rows are written once per model and key in each window;
the first whole window after the arrival's starts 19 to 42 s after a run's first m32 row): the original 203 in each whole window
after the arrival's, the native 354. The original's drop to 203 is complete in its first whole window. The 151 native-only models
of those windows are the 143 of tranche 17 and the 8 it left out, absent from every G-buffer sample; they are recounted by
[models.py](../../evidence/I-0141/artifacts/tranche-18/models.py) (`models.txt`). The G-buffer's union in those windows: 346
models (203 + 143).

**The lead read first: D-0222's launch LOD scale** (OBSERVED in `i141-t17b`'s journals). The LOD manager's scale (`+0x150`, the
`lod_step` rows) is 0.504 in the original until the travel's load, then 1 from journal line 10,074 (1,338 rows). Natively it is
0.504 on every row (1,126): the launch pair {720, 0.96} is kept against the camera's {1080, 0.75} (`native_lod_reference_kept`), so
the native's LOD and draw distances are about twice the original's after a load (derived from the scales; the `lod_step` d
values agree: about 75 m at 0.504, 148 to 151 m at 1, for c3140 and the m32 pieces).

**Run a** (`run-t18a.ps1`, `evidence/I-0141/artifacts/tranche-18/`; the launcher's module of main `d3d5104c9` copied to
`build-main-d3d5104c9`; `i141-t18a-c32000000-nat-ds3lod`, 14:04:04 to 14:09:42, tranche 17's native protocol with
`no-lod-launch-scale.flag` and without the G-buffer pictures; the ASUS 1920x1080 window, offline, no measure, exit 0, no fault,
13,930 native frames, restored by hash; `runs-t18a.log`).
- The scale is 0.504 until the load and 1 after it (248 and 911 rows), as the original's.
- The native still holds 353 models in each whole window after the arrival: every one of the original's 203 and 150 of the 151
  (142 of the 143). A « still drawn » model is one with a `native_pixel_key` row in those windows.
- The one missing is c3140, a character: at scale 1 its `lod_step` rows give weight 0 in both modes (the original's at d 148.5 m),
  and run a writes `native_zero_fade` for it (candidate outcome « zero_fade »), where `i141-t17b`'s native at 0.504 admits it
  (`native_main_view_weight_admitted`).
- **The LOD scale explains c3140 alone** (OBSERVED). Of the 143, 95 have an original `lod_step` row at scale 1: 94 weight 1, c3140
  weight 0; 48 have no row (the rows are sampled).

**The original's occlusion system** (READ: [MENUS-MAP](../I-0067-static-menus-subsystems-map/MENUS-MAP.md) « Occlusion system »,
[COLLECT-ORIGINAL-STATIC](../I-0066-independent-native-startup/COLLECT-ORIGINAL-STATIC.md); confirmed here on GhidraMCP,
DarkSoulsIII.exe 1.15.2; the excerpts in [occlusion-reading.md](../../evidence/I-0141/artifacts/tranche-18/occlusion-reading.md)).
- The model render manager's GPU initializer `B1F60` -> `B1E50` -> `202DF0` fills a pool of `GXVisibilityQueryHandle` built by
  `19C790` (D3D11 OCCLUSION queries, K-0209, K-0219).
- `OcclusionQuery_RecordPass45_1400ca6d0` records them in engine pass 0x45; `202F00` keeps the handles in states 1 to 3 and frees
  the others.
- `UpdateOcclusionCellStates_1400ca050` sets each node's state: 4 visible; 0 hidden by the cell hierarchy, from its slot 8 against
  the frustum; 1 hidden by its own query, fewer than 11 samples, written by `140147390`; 3 retest; a node near the camera always
  visible.
- Natively `B1F60` is not run (`20_startup_phases.inc`: « occlusion queries ... explicit initial omissions », 400 queries) and the
  scene walk zeroes the visitor's bytes +0x10..+0x12, so no node is ever hidden by occlusion (`12_scene_collect.inc`: « Occlusion
  is open »). I-0113's census lists occlusion culling as not native by design, « cost only ».

**The witness** (`reference-no-occlusion.flag`, Reference runs alone; `04c_scene_witnesses.inc`, `22_setup.inc`; READ in
occlusion-reading.md).
- A hook on `CA050` clears the view state's `+0x98` for the call and puts it back.
- `CA050`'s own first branch then sets every node to 4, hands each node's query handles back (`140147830`), and runs `14020D240`
  on the cell hierarchy. That sets the root cell's state to 7 and every cell below to 6, so the traversers' gates (`142E80`: node
  state above 1; `208180`: cell state above 2) pass everywhere.
- Skipped with the rest of the call: slot 8's frustum and, with `+0x9A` set, slot 0x60 on each node, which give states 0 and 3;
  the cells' update `14020D0E0`; `140147390`, which writes node state 1 and queues the node for pass 0x45's queries (its only code
  caller `CA050`; one DATA reference not read); `+0x330`/`+0x334` and the camera history.
- INFERRED: no node's query is recorded (the queue's reader not traced; who fills `CA6D0`'s second list not read); slot 8 may
  write the cells' `+0x78` too. The traversers' own frustum tests still apply.
- Other readers see `+0x98` at the game's value and `+0x15C` at 1 (`CA050` leaves 0 only when `+0x98` and `+0x99` are both set;
  its readers not searched).

**Run b** (`run-t18b.ps1`; `build-i141-t18b`, the branch at `47b35edaa`; `i141-t18b-c32000000-orig-noocc`, 14:42 to 14:47, tranche
17's original protocol with `reference-no-occlusion.flag`; the ASUS 1920x1080 window, offline, no measure, exit 0, no fault,
restored by hash; `runs-t18b.log`).
- The witness's row: the first call read `+0x98` = 1, so the hook cleared it (READ, from the code); `nodes` 0 (that build's
  code also gives 0 when a read fails; the map not yet loaded, INFERRED). That build's only row.
- The original then holds 340 models in each whole window after the arrival (320 m32 pieces, against 186 with its occlusion):
  every one of its own 203, none the native does not draw, and 137 of the 143 (recounted by
  [models-b.py](../../evidence/I-0141/artifacts/tranche-18/models-b.py), `models-b.txt`).
- **With `reference-no-occlusion.flag`, which forces every occlusion node and cell visible (READ), the original draws 137 of the
  143** (OBSERVED). That its occlusion system alone makes the difference is INFERRED: the hook also skips the call's camera history
  and counter, and the comparison crosses two runs and two builds (`i141-t17b`'s on `build-i141-t17g`; run a on main's
  `d3d5104c9`, run b on `build-i141-t18b`; the protocols otherwise identical, `alphacast-args.json`).
- Not drawn by the original even so: c3140 (the LOD scale, above) and five m32 pieces, `m32_00_00_00_005504`, `005529`, `005548`,
  `005650` and `005660`; and all 8 native-only models without a G-buffer row (FG_A_2012, FG_A_7002, HR_A_0101,
  `m32_00_00_00_030000` to `030030`, o328120). Tranche 22: for the 8, withdrawn (they take no G-buffer row in either mode;
  whether the original draws them after the arrival's window is open).
- The five pieces' original `lod_step` rows give weight 1 at d 148 to 151 m (three of them have rows). Tranche 19: that is the
  distance to the map's origin, common to the m32 pieces, not a limit; the five are the native's cull margin.

**What follows** (INFERRED).
- A node hidden by its own query had fewer than 11 samples of its box pass the frame before (MENUS-MAP), so natively it adds few or
  no pixels; a node hidden by the cells lies outside them. So the castle's flatness is likely not these models. Of the 143, tranche
  17 found the 73 under the fog sets 6 and 7 without a pixel in the native's pictures; the 60 under set 3, the castle's, were not
  checked (run b's world pictures against `i141-t17b`'s original ones would show it).
- The native's G-buffer draws about 70 % more models there (346 against 203), a cost, not measured (D-0227).
- The native occlusion cull is ROADMAP 15's: a modern one (Hi-Z from the previous frame's depth, perhaps per cluster), not a copy
  of the D3D11 queries, without taking out of the RT/PT scene what a ray can see (D-0304,
  [OPTIMISATION-SURVEY](../../.project/OPTIMISATION-SURVEY.md)).

## Tranche 19: the five m32 pieces are the native's cull margin (2026-11-21)

« Left » 3 of tranche 18: the five m32 pieces (`m32_00_00_00_005504`, `005529`, `005548`, `005650`, `005660`) the native draws in its
G-buffer at Archdragon Peak's bonfire and the original does not, even without its occlusion. No code changed.

- **The lead of tranche 18 was wrong**: `lod_step`'s `d` is `|record+0x20 - manager+0x80|` times the scale (READ,
  `04c_scene_witnesses.inc` near line 733; what +0x20 holds is not read, and it changes over a run for 31 to 39 m32 models a run).
  At the bonfire most m32 rows sit between 140 and 170 m, drawn or not, in both modes (the camera about 150.7 m from the map's
  origin), another camera position giving about 650 m; [lod-d.py](../../evidence/I-0141/artifacts/tranche-19/lod-d.py) lists every
  m32 row outside 140..170 m (`lod-d.txt`; OBSERVED). So « 148 to 151 m » is a distance most pieces share, not a limit.
- **The native's own margin** (READ, `12_scene_collect.inc`, `09_model_admission.inc`): the main walk's culler is the view's with
  its six planes and its world box pushed out by `kNativeCullMargin` (1.5 m; I-0100 tranche 4, D-0220's « margin against flicker »),
  so DS3's own tests keep what lies within 1.5 m outside the view; `no-cull-margin.flag` walks with the view's own culler.
- **Run t19a** ([run-t19a.ps1](../../evidence/I-0141/artifacts/tranche-19/run-t19a.ps1): run a of tranche 18 with
  `no-cull-margin.flag` added; `i141-t19a-c32000000-nat-ds3lod-nomargin`, 19:59 to 20:05, the gate's build of main's tree
  `evidence/I-0083/gates/build-9b95e7a44183`, the ASUS 1920x1080 window, offline, no measure, exit 0, no fault, 13,915 native frames,
  restored by hash; `runs-t19a.log`; run a ran on `build-main-d3d5104c9`, native tree `707bc43c`: the five native files changed
  between them are menu views' tallies, an offscreen re-key hook and the Reference-only occlusion hook, none in the main walk's
  culler). In each whole window after the arrival the native holds 348 models (run a: 353), 340 with a
  G-buffer row, as many as the original without its occlusion (340, tranche 18's run b); **none of the five** has a pixel-key or a
  G-buffer row, where run a had all five in both; the native-only models in the G-buffer: none (run a: the five)
  ([models-c.py](../../evidence/I-0141/artifacts/tranche-19/models-c.py), `models-c.txt`). OBSERVED.
- **Conclusion**: the five are the margin's: map pieces whose box E+0x40 the walk tests against the widened culler
  (`09_model_admission.inc` near line 1231, `classify(culler, entity+0x40)`; the culler `walk_culler`, `12_scene_collect.inc` near
  lines 3240-3250), which the view's own culler rejects (OBSERVED by the flag's A/B). That they are wholly outside the view and add no
  pixel is INFERRED (no picture settles it, tranche 21; the view's world box may cut before its planes near the far corners, 05 near line 716). By
  design for characters (D-0220, a mesh wider than its bones' box); for map pieces, whose box bounds their geometry, a cost without
  a known gain: [OPTIMISATION-SURVEY](../../.project/OPTIMISATION-SURVEY.md) item 12. The 8
  native-only models without a G-buffer row (forward or shadow draws, not read) stay as tranche 18 left them. With tranche 18, the
  143 are accounted for: 137 the original's occlusion (INFERRED its sole cause), c3140 D-0222's LOD scale, five the margin.

## Reviews at the merge of tranche 19 (2026-11-21)

- **Integrity review** (`main...d5b2d729b`): no High; nothing in the judge touched (only `last_pass` in REPLAY-GATE.json, as the gate
  writes it). M1: the endurance's « retried as visit 53 » was false: rewritten from `tour.txt`, the journal and the shots. L1: run a's
  build named. L2: the « every » claim on `d`: now `lod-d.py`. L3: the gate's per-run 5 and 2. All in `e96a43f5e` and after.
- **Final review** (`main...d5b2d729b`, Opus xhigh): no High.
  - M1: the margin's test is the map pieces' (E+0x40 against `walk_culler`), not `margin_kept`; « without a pixel » now INFERRED and
    a still owed (« Suggested next step » 3). M2: the endurance (as the integrity M1). M3: `d`'s formula READ, +0x20 not read, the
    script filed. M4: the summary's tranche 18 and 19 bullets.
  - Lows: the margin's cost on map pieces now OPTIMISATION-SURVEY item 12; the 8 native-only models without a G-buffer row given an
    owner (« Suggested next step » 3, ROADMAP 7); the closed item out of the « Left » lists; ROADMAP 1's heading; STATE under its size
    and its row's ending; the Next action's unsourced step removed. Left, not this branch's (scripts untouched): the tour's « in world »
    test accepts any world, so a recovery's Continue is counted as the intended zone's load (`Pilot-EnduranceTour.ps1`); the
    control's runner logs « reference: kept (the gate's reasons are not picture departures alone) » on a PASS without reasons.
- No native change: the launcher's module is not rebuilt and no start check is run after this merge (the tree is main's, run by the
  full control).

## Reviews at the merge of tranche 18 (2026-11-21)

- **Step review of the witness** (`47b35edaa`; Opus 5.5 high): no High.
  - M1 and M2 asked for the other writers of the node and cell states; read (above) and named in the comment.
  - M3: the flag was silently ignored in original-probe mode; it is now refused there with a row (`6d993b972`).
  - L1: a `hooked` row at setup and rows that show it acting were added.
  - L2: the comment names what the call skips.
- **Step review of the fix** (`6d993b972`): no High.
  - M1: « the queries are still recorded » was not supported; corrected (no node's query recorded, INFERRED).
  - M2: slot 8's frustum states and their skipping are now named.
  - L1: +0x15F is the camera-motion flag. L2: +0x15C is named.
  - L3: the « first clearing with nodes » row could log the second such call; both flags are now evaluated first and a `why`
    field added.
  - L4: the node list is read only when a row may be written, -1 unless both ends read.
  - L5 left: the `hooked` row comes before `MH_EnableHook`, as `reference-no-ffx-draw`'s does; an enable failure stops the start.
  - All in `5eee0fedf`, built with its tests (`build-i141-t18e.log`); the last rows not run live.
- **Third step review** (`5eee0fedf`): no High. M1: « its only caller CA050 » left out `140147390`'s DATA reference; now said.
  L1 to L4: `+0x15C`'s condition, slot 0x60 under `+0x9A`, the INFERRED labels, `+0x330`/`+0x334`. L5: the rows call
  `PixelKeyClockMs`. L6: « the first such call » holds while `CA050` runs on one thread at a time (E00270's single call site),
  said. L7: the reasons are JSON booleans. All in `a7a3c93f2`, built with its tests (`build-i141-t18f.log`).
- **Fourth step review** (`a7a3c93f2`): no High, no Medium; the declaration, the format and the behaviour held. Four Low on the
  comment's labels, left (a comment, no behaviour; this RESULT carries the right labels): the push to `+0x138` feeds pass 0x45
  only by inference; the gates' identities with `CA050`'s cells, nodes and counter are inferred; « E00270's single call site »
  comes from MENUS-MAP, one row in any case; the object `14020E360` builds is not read, and `140147390` writes state 4 too.
- **Final review** (`main...8ea3cfb36`; Opus 5.5 xhigh): no High.
  - M1: c3140 is the LOD scale, not open (journals re-read; above).
  - M2: the occlusion's sole cause promoted to OBSERVED; requalified everywhere.
  - L1: the READ claims without recorded excerpts; added to occlusion-reading.md.
  - L2: « What follows » unlabelled; now INFERRED.
  - L3: tranche 17's two sentences now point to this tranche.
  - L4, L5: D-0255's line and ROADMAP's abandoned row aligned with D-0304.
  - L6: the survey's attribution says the coordinator's description, accepted by Thomas.
  - L7: the windows' wording.
  - L8: `qpc_ms` in the rows; the builds listed.
  - L9: this block, STATE's rows at the merge, blank lines.
  - Its recounts matched every count above.
- **Integrity review** (`main...fb980776e`): no High; every count recomputed from the four journals matched.
  - M1: `build-i141-t18e` was named built before its tests had ended; they ended passed (192 PASS lines, its hashes), now said.
  - L1: « forced visible » is READ from the code, the draws OBSERVED; reworded. L2: tranche 17's pointer labelled. L3: the
    builds differ too; said. L4: the doubled distances derived from the scales; said. L5: run a's times.

## The full control of 2026-11-21 (D-0230), run by the R thread

Overdue since 2026-11-19 (the last, 2026-11-16, I-0139 RESULT). Main `9732c7521` (native tree `9b95e7a44183`, the launcher's module
rebuilt 15:40 from it after tranche 18's merge), from this branch's checkout (`f0f007040`, main and one STATE commit; the gate writes
`.project/REPLAY-GATE.json` in its own checkout and its calibration refuses a dirty one, so not in the main checkout E merges in; the
coordinator agreed). `Run-FullControl-1112.ps1 -Tag full-1121 -NewReference` in a hidden detached process, 2026-11-21 16:35-18:51
Paris (the coordinator's slot, the PC whole), the ASUS alone (the OLED unplugged, Thomas 2026-11-21), offline, every save restored by
hash; log `evidence/I-0141/artifacts/full-1121.log`. A first launch at 16:34 died at once (a path with spaces in `-ArgumentList`; no
game started).
- **The replayed path gate** `full-1121-gate` with its memory leg (`evidence/I-0083/gates/full-1121-gate.json`): **PASS**, the first
  since 2026-11-03, against the reference of 2026-11-16 (`ref-1116b`): pictures 0.51 and 0.48 (threshold 1.5; cells 0.0004 and
  0.0001 for 0.005); period 16.67 ms; frames over 2.04 P 3.5, the mean of m1's 5 and m2's 2, the mean the gate judges (threshold 4); over 1.5 P 17 (27); display_late 5 (12); game steps over
  2.04 P 1 (6); memory -1 MB a visit (40). The gate build `evidence/I-0083/gates/build-9b95e7a44183` made at its start (16:35-16:38).
- **The reference kept**: the runner's reference step did not run (the gate passed; no threshold changed). `REPLAY-GATE.json` now
  names this tree as the last passing one.
- **The control sizes**: 1920x1080 in full screen (the controller's 0xD3126612 and `closed` 1, as every full-screen run since
  I-0100), the 1500x844 window, 1280x720 windowed: each replay complete, no fault, restored.
- **The night endurance** `full-1121-night` (17:17-18:51): 94 minutes of tour, 53 visits and 52 loads, no fault, no crash dump,
  335,380 native frames, restored. One load missed (visit 52, Gamma's m37, Irithyll of the Boreal Valley, at 18:45): the pilot's shot
  `shots/v52-missed.jpg` shows the title screen with Continue selected, the load not taken. In the recovery the tour's title step
  timed out (`tour.txt`: « title » with an empty result at 18:49:47) and the game entered Beta's world (m31, about 18:47:30 by the
  journal's `native_present_pacing` content draws; `v53-zone.jpg` shows the rocks and the HUD of `v51-zone.jpg`, Beta's, not Irithyll's snow), through the title
  helper's Continue (INFERRED). Visit 53 found that world already loaded (« loading » and « in world » 15 ms apart): Gamma m37 was
  not retried, and the 52 loads are 51 the tour drove and that Continue (OBSERVED in `tour.txt`, the journal and the shots). No
  fault, refusal or crash row; a miss of the pilot's inputs, INFERRED.

## Tranche 20: a view of forward rows alone, 41000060's black cut 2 (2026-11-23)

The defect (I-0138 tranche 10, OBSERVED there): in 41000060 (the ending after the Soul of Cinder) cut 2 is black natively, the
original showing the eclipse over the clouds. **The cause, read in that run's journal** (`i138-t10-c41000060-nat`, OBSERVED): from
cut 2's first clock row the main walk admits 1 or 2 models (about 58 in cut 1), 4 or 5 prepared rows, 0 G-buffer draws
(`native_selected_draw_preparation`), and the kept draws are 5 for 2 models whose changes are all `forward` (`native_persistent_draws`);
`PrepareNativeModelFrame` set the lighting only for a frame with G-buffer draws, cleared the forward rows without it, and returned no
frame at all without G-buffer draws, so each cut-2 frame was a clear or the subtitles alone. The original draws every frame
(INFERRED: its frames show the sky those forward rows draw).

**The change** (`5a5085cad`, `5806a3d8b`): a main view with forward draws and no G-buffer draw (its G-buffer rows absent, or all
refused at their preparation: then the sky and effects over an empty world, where nothing was drawn before) gets its lighting and is kept; the
renderer's `RenderModels` accepts an empty G-buffer list only with forward draws and lighting with programmes (its G-buffer pass then
clears and records nothing); the menu views keep the G-buffer rule (`RenderView` requires it); no decal pass on such a frame; a sampled
row `native_forward_only_frames` (forward, water and effect draws).

**The witness** `i141-t20a-c41000060-nat` (build `evidence/I-0141/build-i141-t20a` from `5a5085cad`, its tests passed; the ASUS,
2026-11-22 23:59 to 2026-11-23 00:05 Paris, offline, restored by hash, no fault; log `evidence/I-0141/t20-run-a.log`), on the Remo
clock against `i138-t10-c41000060-orig` (D-0299, the same ten holds): within cut 2, 0 `native_clear` frames (1208 before) and 3311
model frames; `native_forward_only_frames` 4 or 5 forward rows and 1 to 9 effect draws, its first row at cut 2 (none in the run's play before it:
no change in play there, the CONTRACT's condition); no unserved-rays or fatal row. The witness and the tests are of `5a5085cad`;
`5806a3d8b` (the step review's L1 to L3, the decal pass skipped on such a frame) is built and started by the merge's build and start
check only: its picture the same, INFERRED (a decal writes only where the receiver's stencil is 1, and a cleared G-buffer holds 0). The game
window's luma (`crop=1280:720:8:31`, YAVG), native / original: holds 6 to 9 121.9 / 123.2, 201.9 / 205.9, 200.8 / 204.5, 175.8 /
178.4 (MEASURED; before, the native's cut 2 was black); holds 0 and 3 unchanged against tranche 10 (65.8 and 122.5 natively). Pairs:
`evidence/I-0141/t20-pairs/` (`c41000060-hold-6.png`, the native above the original; its table's luma columns are the whole
1920x1080 screen, desktop included, 5 to 6.2 of 255 apart at holds 6 to 9: the numbers above are the game window's).

**Step review** (a fresh `step-reviewer`, `9063b3118..5a5085cad`): no High, no Medium. **L1** the drop's comment still said « no
draws »: rewritten. **L2** the new counter admitted by another static's allow entry of the same name: renamed `forward_only_rows`,
its own entry. **L3** the decal pass ran on a cleared G-buffer: not built for such a frame. **L4** `idle` (no content draw) now false
on these frames, a pending kept-draw reset waiting through them: left, a small change of timing, not a fault. Its residual risk (the
rays' fatal rule now judging these frames) answered by the witness: no unserved row.

**Reviews at the merge** (D-0283, D-0268; `main...36de115ab`). **The final review** (a fresh `final-reviewer`): no High. **M1** the
keep tested G-buffer draws while the comments and the records said rows (a view whose G-buffer rows were all refused is kept too):
the code kept (the original draws its frames), the comments, « The change » and the criterion restated as draws. **L1** a view with
effects or water and no model draw still dropped: named under « Uncertainty ». **L2** the rays' fatal rule now judging these frames,
answered for 41000060 alone: named there. **L3** the witness of `5a5085cad`, not of `5806a3d8b`: stated above. **L4** the keep
written three times and decided after the post chain's plans: left (no behaviour, a cleanup; the late drop older), named. **L5** the
records: STATE, ROADMAP, the index and I-0138's « Suggested next step » rewritten; the pairs' table's columns named above. **L6** no
change in play: cited above. **The integrity review** (`integrity-reviewer`): **L1** the witnessed binary not HEAD: stated above;
no other finding (the allow entry tightens, nothing keyed to the scene, the numbers reproduce).

**The merge** (`a3b6ad64a`, 2026-11-23 00:25 Paris): the launcher's module rebuilt from it (its tests passed), the start check
`start-a3b6ad64a` passed (restored); the disk rotation deleted nothing (457.6 GB free).

**Outcome** (the R pilot): 41000060's cut 2 is drawn natively, within 1.3 to 4 of 255 of the original's window luma at its four holds
(MEASURED). Left in step 7: 34010010 lighter and 41000060's cut 0 darker than the original before the bloom (I-0138 tranche 10;
workstream 12's light, INFERRED, as the other light items).

## Tranche 21: the margin pieces' pixels, the stills of `i141-t18a` and `i141-t19a` (2026-11-23, an observation)

« Suggested next step » 3's second half: whether the five m32 pieces the cull margin keeps (tranche 19) add pixels. No code, no run.
The two runs' world dumps at the same presentation serials (2400 to 10800, `world-<serial>-display.rgba`, 1920x1080) were
differenced: ffmpeg, both converted to `gbrp`, `blend=all_mode=difference`, then grey and times 4 (`evidence/I-0141/t21-margin/`:
`diffs-x4.png` holds 2400, 4800, 6000, 7200, 8400, 9600, 10800 top to bottom, without 3600; `diffs-x4-all8.png` the eight, and
`pairs-2400-6000.png`, `pairs-7200-10800.png` the pictures side by side, the margin run left, both remade by the final review;
`world-2400-` and `world-3600-margin-vs-nomargin-diff.png` each serial's two pictures and their difference). OBSERVED: 2400 a loading
screen in both runs with different texts; 3600 and 4800 the starting interior bonfire, the same view, only the flames, the candles
and the character's edges differing; 6000 the travel menu in both, almost alike; 7200 that bonfire with `i141-t19a`'s character
already in the travel's ash and `i141-t18a`'s not; 8400 to 10800 Archdragon Peak, where the haze differs over the upper two thirds
(29 to 54 % of its pixels over 8 of 255, the lower third 3 to 10 %; the final review's band means). 2400 and 7200 show the two runs
are not at the same moment of the game (D-0299's pairing not met). So these stills do not settle it: the five pieces' pixels, if
any, cannot be told from the haze's motion. Still INFERRED none (tranche 19). `native_pixel_key` names every model that takes a
light set, not the pixels it writes (READ, `05_selected_scene.inc` near line 977), so the earlier rows do not answer either. What
would: a row naming which of the view culler's tests (a plane, or the world box that can cut before the planes near the far
corners, `05_selected_scene.inc` near line 722) rejects each of the five boxes, a plane meaning wholly outside the view; or the
runs' `world-<serial>-mask.rgba` dumps (written before the forward rows and the effects; about 97 % alike at 8400 to 10800 by the
final review's count, its channels not read: a lead only).

**Reviews at the merge** (records only; `main...71ff1267d`). **The final review**: no High. **M1** the serials' scenes read shifted
by one row of the unlabelled sheet (6000 and 7200 called Archdragon Peak, 6000's small difference given to 4800) and **M2** a
maximum range from other serials and « over the whole picture »: the paragraph above rewritten from its remade pairs and band means.
**L1** the method and the files: named above. **L2** tranche 19's « no picture checked »: pointed here. **L3** STATE, ROADMAP and
the index: rewritten. **L4** the cheaper routes: named above. **L5** OPTIMISATION-SURVEY's « five pieces outside the view »: « the
view's culler rejects them » (outside the view INFERRED). **The integrity review**: the same two findings (M, L), answered with
them; nothing else. **The merge** (`26e2af70c`, 2026-11-23 00:47 Paris): records only, no rebuild, no start check.

## Tranche 22: m32's 8 models without a G-buffer row, the margin pieces' culler row, Yorshka (2026-11-23)

Opus 5.5, `claude/r-i141-t22` (the fresh R thread after tranche 21). « Suggested next step » 3, then 2.

**The 8 models without a G-buffer row: drawn natively outside the G-buffer; the original prepares them in its arrival window**
(OBSERVED in the journals of `i141-t17b`, `i141-t18a`, `i141-t18b`; no run). Tranche 18 counted the original's models by its
`reference_pixel_key` rows, written for the G-buffer's pass state types 6 and 7 alone (READ, `04c_scene_witnesses.inc` near line
220), while `native_pixel_key` names every model that takes a light set, forward draws included (tranche 21). Natively (`i141-t18a`):
- `m32_00_00_00_030000`, `030010`, `030020`, `030030`: map pieces with no G-buffer mesh (`native_map_piece_rows`' `without_gbuffer`
  4, these four its examples), drawn in the forward rows (`native_forward_map_draw`, engine pass 41 = 0x29, recipe 6), with a
  scrolling fog row (`native_forward_fog`; fog key 310);
- `o328120`: an object of two bones admitted as a character (`native_character_candidate`), with a forward fog row (key 350);
- `HR_A_0101`, `FG_A_2012`, `FG_A_7002`: hair and face parts at one placement, drawn as parts (`native_part_draw`, passes 0x1F and
  0x29), present before the travel: the player's own parts (INFERRED from the shared placement).

In the original (`i141-t17b`, `i141-t18b`): `reference_pixel_key` (cap 40,000, not reached) names none of the 8, so none takes a
G-buffer row; `reference_model_light_key` (pass 69: the four pieces and `o328120`, light key {132000000, 0} and fog keys 310 and 350,
as the native's `native_model_light_key`), `reference_instance_env_model` (state types 0, 2, the depth rows, `04c` near line 630, and
4; its 600-row cap reached at `i141-t17b`'s line 12,814) and `reference_part_row_state` (the three parts) name them. The pieces' and
`o328120`'s rows lie in the window of the arrival (`i141-t17b` lines 10,565 to 12,364; m32's first row at 10,077, the next window from
about 17,097), their writers keeping one row per key for the run; the parts' rows lie before and during it (lines 996 to 13,041:
`reference_part_row_state`, 53 or 54 rows a part, to 9,774, and `reference_instance_rows`), none after it. The only later rows naming
the 8 are three sampled `lod_step` rows (one call in 20,000; `o328120` at line 26,021, `030020` at 28,198, `030010` at 29,572, in the
windows after the arrival's): the original still steps their LOD then, which is not a draw. **Whether the original still draws the 8
in the whole windows after the arrival's, where tranche 18 counts, is not shown** (the final review's M1). The original drops 137 G-buffer models after that
window (tranche 18): the 8 may do the same. So tranche 18's « not drawn by the original without its occlusion either » rested on
G-buffer rows the 8 never have: withdrawn; whether they are native-only stays open. What would answer: a per-window row of the
original's forward and depth draws (`reference_model_light_key` keyed per window, or `reference_pixel_key` extended to state type 4)
and one original run.

**The margin pieces: which test of the view's culler ends each** (« Suggested next step » 3's second half).
- **The row** (`4f30f6fe9`, `379417351`, `e38353a36`; `05_selected_scene.inc` `NoteNativeMarginPiece`, called in
  `09_model_admission.inc` after a map piece's gates 1 to 4, popin-observe.flag only): for a map piece the main walk admits through
  its widened culler and the view's own culler (6F310 on `native_cull_margin.real`) ends, `native_margin_piece` names 6F310's stage
  that ends it (its world box 14001FFF0 on +0x220, tested first, or a plane of +0x160), per plane the signed distance in metres of
  the box's corner farthest on the plane's inside side (negative: the whole box on its outer side) and the plane's inside normal
  against the camera's forward axis, the gap to the view's world box, the box and the piece's models; once per minute window and key.
  Its callers: the main walk's two paths (gathered on the pool, and light-observe.flag's whole filter) and the record-revalidation
  guard's serial walk (the same culler, deduplicated).
- **Step review** (a fresh `step-reviewer`, `50658d2f2..4f30f6fe9`): no High, no Medium; the stage order, the corner, the sign and
  the facing found to match 6F310 and the margin code. **L1** the 900-byte row could overflow on huge finite floats (a CRT fast
  fail): 2048 bytes. **L2** the row was written before gates 2 to 4, so for pieces never drawn: now after them. **L3** the dedup
  and the cap lived for the session: a minute window, 20,000 rows. **L4** with light-observe.flag the walk takes its whole filter
  without gathering, where the row was not called: called on both paths now (`379417351`).
- **Run a** (`run-t22a.ps1`, `evidence/I-0141/artifacts/tranche-22/`; `build-i141-t22a` of `4f30f6fe9`, its tests passed;
  `i141-t22a-c32000000-nat-ds3lod`, 01:06 to 01:12 Paris, announced for 01:07 and started a minute early; tranche 18's run a protocol
  with popin-observe.flag, the margin on; the ASUS 1920x1080 window, offline, no measure, exit 0, no fault, restored by hash): no
  row, L4's case (the protocol has light-observe.flag). Not read further.
- **Run b** (`run-t22b.ps1`; `build-i141-t22b` of `379417351`, its tests passed; `i141-t22b-c32000000-nat-ds3lod`, 01:21 to 01:26,
  the same protocol and conditions, exit 0, no fault, 13,943 native frames, restored by hash): 220 rows for 103 entities, 85 of them
  at m32 (38 first models, 29 of m32, the others at Firelink's interior bonfire before the travel)
  ([margin.py](../../evidence/I-0141/artifacts/tranche-22/margin.py), `margin-t22b.txt`;
  [margin-stats.py](../../evidence/I-0141/artifacts/tranche-22/margin-stats.py), `margin-stats-t22b.txt`). **Every row's stage is a
  plane, never the world box; each box lies wholly on the outer side of one plane (two for 9 rows), by 0.03 to 1.5 m at m32 (0.01
  to 1.5 m in the run); the rejecting planes are side planes (their inside normals at 0.37 and 0.57 of the camera's forward axis,
  the near and far planes at +1 and -1)** (OBSERVED). The five of tranche 19 have rows in the arrival's window and each of the 3
  after it, with `native_pixel_key` rows as in tranche 18's run a: `005504`, `005529` and `005548` outside plane 3 by 0.22 to 1.49 m,
  `005650` and `005660` (two entities each) outside plane 1 by 0.21 to 0.35 m.
- **Conclusion**: the five, and every piece the margin keeps in this run, are wholly outside the view (OBSERVED, against the view's
  own planes); they add no pixel provided a map piece's box E+0x40 bounds its geometry (INFERRED, the culler's own premise; most at
  stake for the boxes a few centimetres out). Tranche 21's question is answered for « outside the view »; « no pixel » stays INFERRED. Only the whole-filter
  path ran (light-observe.flag): the gathered path's rows are READ only. The margin's cost on map pieces stays
  OPTIMISATION-SURVEY item 12's.

**Yorshka's sweeps** (« Suggested next step » 2): **not pursued**, the pilot's call, put to Thomas's eye (STATE's « Next action »).
The residual is 0.7 to 1.8 of 255 over the original on tour stills taken on the runner's clock (tranche 16), above the original's own
repeat (0.5), the native's repeat never measured. A pairing at the same moment (D-0299) in play would need a clock both modes share for
a camera sweep: the runner's look sweep runs on its wall clock and no tool pairs it (TOOLS names the effects' pairing alone). Under
D-0249 a traced picture is not held to the original's pixels, and about 1 of 255 on the whole window shows no anomaly (no wrong input
or light where none belongs named). Left as an observation; reopened if Thomas's eye finds Yorshka too bright.

## Reviews at the merge of tranche 22 (2026-11-23)

- **Final review** (`final-reviewer`, `main...53a5d2fc2`, the tree built as `build-i141-t22-final`, its tests passed): no High, two
  Medium, eleven Low; criteria 1 and 2 (the code) met, 3 and 4 (the records) not.
  - M1: « the 8 are not native-only (OBSERVED) » was not shown for the windows after the arrival's, where tranche 18 counts: the
    original's rows for them all lie in the arrival's window and are written once per key. Restated as open (« Tranche 22 »,
    Uncertainty, « Suggested next step » 4, STATE, ROADMAP step 7).
  - M2: « 0.2 to 1.5 m » held for the five, not for every piece: 0.03 to 1.5 m at m32, 0.01 in the run. Restated.
  - L1 the counts (103 entities, 85 at m32; 9 of the 38 first models Firelink's): restated, OPTIMISATION-SURVEY item 12 too. L2 the
    figures' script not saved: `margin-stats.py`. L3 tranche 18's withdrawn sentence marked inline; the Uncertainty bullets made
    consistent. L4 `build-i141-t22-final` in Artifacts. L5 the journal named for the line numbers. L6 « never 6 or 7 » rests on
    `reference_pixel_key` (cap not reached): named. L7 the index row's « each in the Conclusion » and the occlusion as cause INFERRED:
    rewritten. L8 Yorshka put in STATE's list for Thomas's eye. L10 the gathered path not run: stated. L11 run a's early start: stated.
  - L9 the model-name walk a third copy of `refuse_disp`'s, and the record-revalidation guard a third caller: the caller named in
    the record; the copy left (a cleanup, no behaviour).
- **Integrity review** (`integrity-reviewer`, `main...7d955cb03`): no High, no Medium, six Low; nothing in the judge touched, no
  weakened test, no special case, nothing cut during a measure.
  - L1 the parts' original rows lie before and during the arrival's window, not in it alone: restated. L2 three later sampled
    `lod_step` rows for three of the 8, not draws: named. L3 `margin-stats.py` printed the farthest distance twice and `margin.py`
    six rows a piece: both fixed and their outputs remade (`margin-t22b.txt` now `margin.py`'s alone). L4 the summary lines without
    the run's configuration: the Conclusion bullet and item 12 name it. L5 the margin's « no pixel » kept as an INFERRED residual
    (« Suggested next step » 4, ROADMAP); the 8 open in the index row. L6 « 4 windows after the arrival » was the arrival's and 3
    after: restated.
- Merged `ab8479c4a`; the launcher rebuilt (`game-bootstrap-build-main-ab8479c4.log`: exit 0, 194 PASS, no FAIL), start check
  `start-ab8479c4a` passed (02:05 to 02:06 Paris, world in 2.8 s, no fault, restored); `Clear-DiskCache -Apply`: 0.3 GB deleted
  (one build folder older than 48 h), 454.5 GB free.

## Tranche 23: the post, shadow and IBL settings fade value by value (2026-11-23)

Opus 5.5, `claude/r-i141-t23b` (the fresh R thread after tranche 22). « Suggested next step » 2.

**What the original does** (READ in the executable, llvm-objdump on `DarkSoulsIII.exe` 1.15.2 and GhidraMCP, 2026-11-23; the
scripts and tables in `evidence/I-0141/artifacts/tranche-23/`). `FUN_140095B00` rebuilds every group's live fields layer by layer
through the value slot's `vt+0x80(slot, key, A, B, w)` (STUDY-LAYER-FADE §2): A the field before the layer, B the field it writes,
w the layer's weight when the slot fades (`vt+0x88`), else 1. The slots are `GXValueAccessorSlot<T>` (their RTTI), one class a
type; each `vt+0x80` first looks for the key (absent: the lower layers' value stays), then:
- float (`1401EEC30`): `w <= 0` A, `w >= 1` V (the layer's value), V when A = V, else `(1 - w) A + V w` in float; vectors (V2
  `1401F0BB0`, V3 `1401F1B70`, V4 `1401F2AD0`) the same on every used lane, V only when all of them are equal (`cmpeqps`,
  `movmskps`: the step review's M1);
- s32 (`1401E8FA0`), u32 (`1401ECCF0`, through 64-bit conversions), s16 (`1401E8070`), u8 (`1401EAE50`), s8 (`1401E7120`): the same
  in float, then truncated (`cvttss2si`);
- colour (`1401DECD0`): each byte so, truncated, no equality test; bool (`1401DECA0`): V at once whatever w;
- angle pair (subtype 0x16, `1401DEDF0`): `w <= 0` A, `w >= 1` V, else component 0 by `1400F8FD0`'s AngleLerp on [-pi/2, pi/2]
  and component 1 on [-pi, pi] (`1401DEAB0`, the same routine at another address; the constants ShadowSunDirection reads).

Whether a slot fades is its definition's byte +0x44 (a 0x50-byte entry: name, short name, default, type +0x38, subtype +0x3C),
the slot constructor's argument (the float slot's `1401F4DC0` writes it at +0x59), given by `AddParam_1401F72A0`'s last argument
when the groups are declared at start. All 410 AddParam calls of the 26 groups were read (`fadeflags.tsv`) and the 14 enumerations,
declared by four other adders (`14015F520`, `140164320`, `14016F930`, `14018CF50`) with the byte 0 (`enums.tsv`): every value of
default.gparam is one of them (its Tone Map « Gamma » twice under a name held in a register, both floats that fade). Almost every
float, vector, colour and angle fades; 70 definitions do not (the Enable switches, the IDs of textures and maps, Setting Info's
BG Color and qualities, Tone Map's AutoExposure and UseTable, ColorGrading's LutSourceId, ShadowParam's Light Id and Cull Flip,
Bloom's GhostSharpness, StarRotation, StarForceDispersion, MotionBlur's ReccurenceCount, WaterInteraction's resolutions), nor do
the enumerations; some integers do (LightScattering's s16, Bloom's GlareNumLevels, Dof's sample counts, ShadowParam's Blur Count,
Cloud Shadow's Texture:ID). Two register-held flags (LightScattering) read as 1, INFERRED.

**What is built** (`light_producers.cpp`): `FadeValue` applies those classes by the file's type (the engine's class is the
definition's; a file whose type differs from it, INFERRED), with the table of the values declared without the fade, by group and
short name (some names repeat across groups). During a fade of the area (tranche 7's `SetAreaFades`, the weight from
`AdvanceFades`), `Shadow`, `GroupWord` and `GroupFloat` (so `GlowAt`), `GroupEntries` (so `DominantAt`, `SnowAt`, `SnowModelAt`
and the post settings' blocks) and `EnvColorScale` answer each value between the earlier layers' (through `FadeView`) and the
current layers'; the current's alone where the earlier layers give none or one of another type. `GroupEntries`' layered lookup
now honours the fade's view. `BackgroundColor` stays at once (BG Color does not fade); `Env` is the MSB boxes, no layer's. The
Dominant, snow and snow-rate caches and the kept draws' snow stamp key on `RecordGeneration`, so they renew at each fade step (and
during any area's fade: their cost a few seconds a fade, which tranche 7's second review's M1 had kept off them before they faded).
Limits: the earlier end is the earlier layers' answer, not the values blended at the change (tranche 7's L7, alike);
`EnvColorScale` of an area whose own file lacks the key answers nothing during a fade as before (its caller's {1, 0} does not fade).
Test: `glow_bank_test`'s value-fade checks (default's id 11 through the layers to {1, 0}: the shadow record at the weights 0 and
0.25, its floats, Blur Count 4 to 0 truncated to 3, FilterMode and Resolution at once, the angle pair, the bank tag; GroupFloat,
a value declared without the fade at weight 0, SSAO's entries with their bools at once; Blur Count truncated to 2 at 0.3; the end).

**Step review** (step-reviewer on `ee17419f5..c6d8b7bf9`): no High; M1 the vectors' equality test taken per lane where the
original tests the whole vector: fixed (`FadeValue`, and `GroupWord` now passes a vector's lanes all). L1 the shadow record's
at-once fields chosen by the file's name where the record is filled by position (m21 and m22 name Light Id with single spaces):
fixed, by record field (Light Id, FilterMode, Resolution, the two bool bytes). L2 a repeated name matched to its first occurrence:
fixed, its same occurrence. L3 an entry whose row lacks the id faded from or to zero: fixed (`BankEntry::present`). L5 a stale
comment: fixed. L7 the tests: truncation now exercised (2.8 to 2); left untested, with no file pair that shows them, the partial
vector equality, colours, s16 and u8, the angle's wrap and `EnvColorScale` during a fade. Left: L4 the caches renew during any
area's fade (the cost accepted above); L6 the IBL memo (`09_model_admission.inc`) keyed without the fade, unreached (the own keys
observed are {0, -1}, which never fade), so `EnvColorScale`'s fade has no consumer yet that can show it.

## Tranche 23, step 3: 32000010's last cut on the scene's clock (2026-11-23, an observation)

« Suggested next step » 3, D-0299. Tranche 2's « pale veil over the King natively » at the last cut came from shots taken on the
wall clock, at two different instants of the cut.

**The runs.** `i141-t23a-c32000010-nat` (03:00 to 03:07, Paris) and `-orig` (03:07 to 03:14). Both exit 0, no fault, the save
restored by hash, the ASUS in a 1920x1080 window, offline, no measure. The build is `build-i141-t23b` (`c6d8b7bf9`, before the step
review's fixes, which do not touch a value without an area fade). Tranche 14's protocol (`i141-t14h`):
- Eta's save; the King revived by the test tools (`test-tools.flag`, `test-revive.txt` 13200850, placed in the build folder by
  hand and removed after); the four flags cleared, so the ceremony 32000000 plays at the arrival.
- Then 32000010 by the cutscene tool, held by `Run-AlphaCast -CutsceneHolds '7000 p15 p35 p55 p75 p86 p90 p95 p99'`.
- The pilot's series wrapper lost its log after the native run (its process ended without its `finally`), so the original run's
  `alphacast-args.json` was rewritten at 03:19 by a second launch that stopped on « Run exists ». That launch had the same
  arguments, and nothing else of the run changed.

**The pairs** (`Make-I138HoldPairs.ps1`; `evidence/I-0141/artifacts/tranche-23/holds/t23a.tsv` and its sheets). All 8 holds are
compared, the same cut and frame in both modes. Whole-capture YAVG, native against original:

| Hold | Cut, frame | Native | Original |
|---|---|---|---|
| 0 | 0, 209.4 | 85.4 | 86.7 |
| 1 | 1, 128.6 | 94.6 | 90.6 |
| 2 | 3, 51.8 | 95.6 | 100.0 |
| 3 | 4, 221 | 134.9 | 126.9 |
| 4 | 5, 14.56 | 117.2 | 102.6 |
| 5 | 5, 70.4 | 101.9 | 108.6 |
| 6 | 5, 140.2 | 93.6 | 102.9 |
| 7 | 5, 196.04 | 21.5 | 26.2 |

**What the last cut shows** (holds 4 to 7, OBSERVED on this pair):
- Both modes show the storm state after the ceremony, with the same grey sky, towers and rain.
- The whole frame is brighter natively at hold 4 (117.2 against 102.6) and darker at holds 5 and 6 (6.7 and 9.3 lower); at hold 7
  both fade to black (21.5 against 26.2). Crops (YAVG, native against original): the upper-left sky (x 0 to 500, y 60 to 300)
  123.8 against 108.9 at hold 4, 124.7 against 121.6 at hold 5, 107.2 against 120.9 at hold 6; the lower right (x 1500 to 1900,
  y 700 to 1000) 111.2 against 99.6, 89.0 against 108.8, 93.7 against 80.0; the left towers (x 560 to 760, y 40 to 500) 118.9
  against 108.8, 139.3 against 129.4, 119.4 against 139.1. The signs change from hold to hold with the moving clouds.
- At hold 4 the native's blade glows gold along its length and the King's body is lit orange, while the original's blade carries
  lightning and its body stays dark. At hold 6 it is the other way round: the original's blade glows and the native's does not.
  The holds freeze the Remo clock, not the effects' own simulation (Make-I138HoldPairs' header, step review M3); the shots sit at
  nearly the same offsets in their holds (2623 against 2601 ms at hold 4).

**Reading.** That the blade's phase differs between the runs is INFERRED (no effect phase was measured); a stronger native effect
light at hold 4 would show the same. No pale veil is seen at holds 5 and 6; hold 4 is brighter frame-wide natively, its cause not
established (the blade's light, the clouds' phase, or a veil). One pair under a moving sky settles neither tranche 2's veil nor the
blade's light: both stay this item's (« Suggested next step » 3).

## Reviews at the merge of tranche 23 (2026-11-23)

**Final review** (final-reviewer on `main...d98c89cab`): no High.
- M1: the region IBL memo (`09_model_admission.inc`) kept an own key's scale through a fade. Fixed: the memo of an own key other
  than {0, -1} holds while the banks' `RecordGeneration` holds.
- M2: `glow_bank_test` left most value classes untested. Fixed through a test seam, `FadeValueForTest`, with checks on synthetic
  words:
  - the float at w <= 0, 0.25, w >= 1 and NaN;
  - a vector with some lanes unequal lerping its equal lane, and one equal on every lane taking V's words;
  - s32, u32 (64-bit), s16 (sign, its 16-bit equality), u8, s8 and colour truncation;
  - a bool and an enumeration at once;
  - the angle periods read from the executable (now required, no longer skipped) and both AngleLerp branches.
  Left untested: the readers' fallbacks other than the ones the area fade above exercises.
- M3: step 3's conclusions went beyond one pair. Rewritten: the blade's phase is INFERRED; hold 4 is brighter frame-wide natively;
  the item stays R's (« Suggested next step » 3), not handed to E. The commit message of `dd9d88bde` (« no veil natively ») says
  more than the evidence.
- L1: stale labels. Fixed: READ in the header, the Uncertainty lines and the view comment.
- L2: Sun and Fog still on the record blend. Recorded as « Suggested next step » 5 and in ROADMAP.
- L4: the snow names renewed at each fade step. Fixed: keyed on `BindingGeneration`.
- L6: the evidence scripts depended on the scratchpad. Fixed: they resolve beside themselves; `enumcalls.txt`, the intermediate
  tables, `join.py` (which reproduces `fadeflags.tsv` exactly) and a `README.txt` are kept.
- L7: `Run-AlphaCast.ps1` rewrote an existing run's `alphacast-args.json`. Fixed: a run already there keeps its file.
- The integrity review (integrity-reviewer on `main...19bf02ffa`): no High or Medium. L-a: the fade READ and unit-tested only, the
  IBL fade without an observed consumer: said in the conclusion and « Suggested next step » 2. L-b: the final code's tests:
  `build-i141-t23d` (module SHA-256 `A42EB95A…`), then after the fixes' step review `build-i141-t23e` (`85AE6E90…`):
  `glow_bank_test` 119 checks, 0 failures, every unit test passed (`evidence/I-0141/artifacts/tranche-23/build-t23d-tests.txt`,
  `build-t23e-tests.txt`). L-c: Bloom's enumeration named « ? » in `enums.tsv` is
  GlareShape: its name string (0x143d4dc50) is the one passed to `140164320` (READ, the final reviewer's check). L-d: the
  `Run-AlphaCast.ps1` test of an existing run moved before the trace job starts (see the step review below).
- The fixes' step review (step-reviewer on `d98c89cab..19bf02ffa`): no High.
  - M1 and M2: the memo was keyed on a generation without the bank's identity, and took `gparam_lock` per model and re-chose its
    regions at any fade step. Fixed: the regions snapshot carries the collect's `RecordGeneration` beside its banks; a memo whose
    regions, position and keys hold keeps its choice and renews only its scales when the banks (compared as owners) or the
    generation moved, with no lock on the hit.
  - M3: AngleLerp's exit with b above a across the period and the move of V below the period's start were untested. Fixed: two
    more cases.
  - M4: `Run-AlphaCast.ps1` went on with an existing run's name and clicked over its pictures. Fixed: it now throws before the
    trace job, as Run-StartupTrace does, which also answers L1, the race.
  - L2: the slot calls 1401DEAB0, which is byte for byte 1400F8FD0. Named.
  - L3: the colour test now has distinct bytes in all four lanes.
- The second fixes' step review (step-reviewer on `19bf02ffa..c9f3dea36`): no High or Medium. Left, all Low:
  - L1: a memo's renewal reads the global banks (NativeEnvMapScale) while it is stamped with the collect's; a publication between
    the two is corrected at the next collect (one frame).
  - L2: every own-key memo renews its scales at each fade step of any area; the own keys observed are {0, -1}, which never do.
  - L3: no colour case with an equal byte (the slot's missing equality test).
  - L4: 1401DEAB0 is 1400F8FD0 up to its RIP-relative displacements (the same mask), not byte for byte as the comment says.
- After the merge (`9354f103a`): the launcher rebuilt with its unit tests (`glow_bank_test` 119 checks, 0 failures), the start
  check passed (04:23 to 04:24, restored); `Clear-DiskCache -Apply`: 1366 files, 1.0 GB (4 build folders older than 48 h), 452.3 GB
  free on C:.
- Left with their reasons:
  - L3: ends and the at-once set are matched by name, not registry index. The native readers hold no definition index; every
    default.gparam pair is a definition (the reviewer's count, 288), and variant names stay a residual.
  - L5: the small allocations per value during a fade, accepted (fades only).
  - L8: a colour with equal ends may lose a unit, as the original's slot does; no consumer in the main view.

## Tranche 24: Sun and Fog fade value by value; the passes by window (2026-11-23)

Opus 5.5, `claude/r-i141-t24` (the fresh R thread after tranche 23). « Suggested next step » 5, 4, then 3.

**Sun and Fog on the per-value path** (« Suggested next step » 5; the original's path READ in tranche 23 and STUDY-LAYER-FADE §2:
FUN_140095B00 rebuilds the live fields value by value, the records are made from them). During an area fade, `Banks::Sun` and
`Banks::Fog` no longer blend the two ends' records with the engine's record blends (`UseRecordBlends`, which stay for the act's
overrides, tranche 3, and still answer first): `OwnSun` and `OwnFog` take the fade, look the earlier end up by the same rule through
`FadeView`, and read each parameter through `FadedCopy` (the current value, then `FadeValue` between the earlier and the current by
the original's classes, by parameter position as the records are filled; a parameter missing from the earlier file or of another type
keeps the current's). The record is then derived as from a file's: LightSet's Angle pair through AngleLerp, its direction turned by the
current layers' rotation; the colours, Sharpness, SsaoWeight, the hemisphere colours, giOcclusionRate and LightMask Influence lerped;
the three bools (Light1 FollowedCamera, Enable LocalLight1 and 2) and Fog's ToUpper at once, even at weight 0; Fog's map position and
area the current's. What differs from tranche 7's path: the direction between the ends (AngleLerp on the angles, not the record
blend's), the bools, ToUpper and the fog position. That the derivation takes the current binding's transform during a fade is INFERRED
(the transform's reader was not read for a fade). Test: `glow_bank_test`'s tranche 7 block rewritten for the per-value path (the judge's
change, D-0268: its expectations were the record blend's stand-ins; now each field by its class from the two ends, the bools at once at
weight 0, the Angle pairs (b - a) t + a inside their periods and each direction a unit vector with its {x, z, y} copy, the fog field by
field; the generation, AdvanceFades-only weight, override first, other area, end, no earlier layers and over-when-given checks kept).

**The passes by window** (« Suggested next step » 4's tool). With pixel-keys.flag, `reference_model_pass_window` (04c's 57A20
observer: once per 60-second window and (model, the pass id at view +0x124), every pass the original makes a model's rows for, the
forward and depth ones included) and `native_model_pass_window` (12's main collect, every 30th: once per window and (model, engine
pass) of the prepared meshes); 20,000 rows per window each at most, a `_full` row naming a window whose budget ran out. Reader:
`Compare-I141PassWindows.py`.

**Step review** (step-reviewer on `2d5a4be58..1147a3cd6`): no High; one Medium, seven Low.
- M1, the row caps were per process (40,000), so a long run's later windows could lose every row after the arrival: fixed in
  `9c9fe0388` (a budget per window on both sides, the window named in the `_full` row, the reader lists them).
- L1, the header still described the record blend for a fade: fixed in `9c9fe0388`.
- L2, the earlier end's unbound key is noted only after the current record is whole, even when its file lacks parameters: left,
  documented in the header (the note is a diagnostic; the missing parameters keep the current value).
- L3, the test did not check the direction at w > 0 exactly, nor ends whose bools and ToUpper differ: the whole record is now
  compared while the weight holds, and an Angle pair must be checked (`9c9fe0388`); the exact direction at w > 0 and ends with
  differing bools are left (the direction is checked as a unit vector with its copy; the test banks' bools are equal at both ends).
- L4, the fog position and area are the current's at once: left, INFERRED (the original's writer of +0x4C during a fade not read).
- L5, a current file lacking LightSet parameter 26 leaves +0x280 at its default 1: left (not checked across the game's files).
- L6, thread-affinity allow entries for the new mutexes: left (optional).
- L7, the native side keeps no `now >= window` drop and prints %.1f: left (negligible against 60-second windows).

**The second step review** (on `1147a3cd6..9c9fe0388`): no High; one Medium, three Low. M1, the « weight moves only at
AdvanceFades » check covered Sun alone: the whole Fog record is now compared there too (`11fc2cad0`). L1, the header's restart
sentence still named records: fixed (`11fc2cad0`). L2, the reader counted the windows that lost rows: they are marked (`11fc2cad0`).
L3, rows in the last 0.05 ms of a minute bucketed by the rounded `qpc_ms`: left (one row in about 1.2 million). **The third**
(on `9c9fe0388..11fc2cad0`): no High; one Medium, six Low. M1, leaving a window out made its models read as the other run's alone:
the windows whose budget was reached count again (a row proves a draw) and a one-sided claim is printed « unproven » when the
other run reached a budget in its later windows (`b43e7a7e9`). L1 to L4, `Compare-I141HoldSpread.py` on one table, holds whose
target, cut or frame differ, an empty luma, a BOM: refused, refused, not compared, read (`b43e7a7e9`). L5, the fade test's guard
did not make a faded fog value differ between the ends: now required (`b43e7a7e9`). L6, a window with exactly 20,000 rows is
marked although nothing was lost: the label now says « rows may be lost » (`b43e7a7e9`).

**The runs** (one slot, the ASUS in a 1920x1080 window, offline, no measure, every run exit 0 with no fault and the save restored
by hash; build `build-i141-t24b`, `9c9fe0388`, whose later commits touch a test and the readers only). Tranche 17's protocol (Eta's
save, the King revived by the test tools, `test-tools.flag` and `test-revive.txt` 13200850 placed beside the build and removed
after, the four flags cleared, the ceremony 32000000 at the arrival, the look sweep at 99 s), light-observe and pixel-keys:
`i141-t24a-c32000000-orig` (05:07 to 05:13, Paris) and `-nat` (05:13 to 05:19). Script `evidence/I-0141/artifacts/tranche-24/run-t24.ps1`,
its log `runs-t24.log`.

**Step 4: m32's 8 models after the arrival** (`Compare-I141PassWindows.py` on the two runs, its output `tranche-24/pass-windows.txt`;
no window reached its budget; both modes ran test-no-ai, world-pictures, fog-build-observe, light-observe and pixel-keys). Windows:
the original's 4062 to 4066 (the arrival 4063; later, whole: 4064, 4065; 4066 partial, the run's last), the native's 4068 to 4072
(the arrival 4069; later 4070, 4071). The original's pass ids are 57A20's view +0x124: 69 its main view (0x45, `04c` near line 558),
5 to 8 the sun's cascades (`04b` near line 292).
- **This run did not reproduce tranche 18's state in its whole later windows** (the final review's H2, OBSERVED from the same run's
  `reference_pixel_key` rows; `tranche-24/gbuffer-windows.py` and its output `gbuffer-windows.txt`): the original's G-buffer held 351
  models in 4064 and 485 in 4065, against tranche 18's steady 203 (`i141-t17b`), and only 21 models were native-only by G-buffer rows
  there (151 in `i141-t17b`; 4065 holds 142 of those 151). Only the partial window 4066 (194 G-buffer models, 9 of the 151) resembles
  tranche 18's state.
- In 4066 the original still has rows for the four pieces `m32_00_00_00_030000` to `030030` (pass 69) and the three parts
  `HR_A_0101`, `FG_A_2012`, `FG_A_7002` (passes 6, 8, 9 and 69); `o328120` has none there (its last rows in 4065), and none natively
  in 4071 or 4072 either. Natively the four pieces are drawn in every window from the arrival (engine pass 41), the parts in every
  window (41 and 52, `HR_A_0101` 31 too). OBSERVED, one run a mode, on a partial window: seven of the 8 are not native-only in the state tranche 18
  measured; `o328120` is drawn by neither then. The four pieces have no `reference_pixel_key` row in any window, so the original
  draws them outside its G-buffer (in its main view; forward INFERRED, as the native's pass 41).
- Over the whole later windows, by any pass: the original draws 602 models, the native 593; native-only 4 (`c3140`, D-0222's LOD
  scale, and `m32_00_00_00_001555`, `006062`, `007018`, the native's pass 5, in 4070 alone), original-only 13 (the player's
  `AM_M_0000`, `BD_M_0000`, `HD_M_0000`, `LG_M_0000`, `FG_A_3000`, `WP_A_0001` and `WP_A_0001_1`, which the native names by the
  equipped parts instead, `AM_M_2650_L` and the like: the same body, other part files, INFERRED; `c0000` and `o328500` in 4064 and
  4065; `m32_00_00_00_005524`, `008007`, `008008`, `008010` in pass 69 alone, which the native never names). These windows are not
  tranche 18's state (above), so these counts say nothing on tranche 18's 137; the parts' names and the four pieces are not pursued.
- Tranche 18's « 137 = the original's occlusion » (INFERRED) is not re-tested by this run: its original kept them in its G-buffer
  through 4065. Why this run's original drew more than `i141-t17b`'s at the same protocol is not established.

**Step 3: 32000010's last cut, three pairs** (tranche 23's protocol: the same travel and revive, then 32000010 by the cutscene tool,
held at '7000 p15 p35 p55 p75 p86 p90 p95 p99'). `i141-t24b-c32000010-nat` (05:19 to 05:26), `-orig` (05:26 to 05:32),
`i141-t24c-c32000010-nat` (05:32 to 05:39), `-orig` (05:39 to 05:46). Every hold of both new pairs compared, the same cut and
frame as `i141-t23a`'s.

**The window's title bar biased every whole-capture luma** (the final review's H1, OBSERVED): rows 0 to 31 of each 1920x1080
capture are the window's title bar, about 35 natively and 235 in the original on every hold of the three pairs (why the two
modes' bars differ, focus or the dark mode, not established). It added about 4 of 255 to the original's whole-capture mean and took
about 1.5 from the native's: the difference between the modes moved by about 5.8.
`Make-I138HoldPairs.ps1` now measures below row 32 (the sheets keep the whole capture); the three pairs were measured again
(`evidence/I-0141/artifacts/tranche-24/holds-body/`; the biased tables stay in `tranche-24/holds/` and `tranche-23/holds/`). The
`t23a` pair ran `build-i141-t23b` (the record-blend fade), `t24b` and `t24c` `build-i141-t24b`; 32000010's 3 s area fade ends before
hold 0 in each.
Tranche 23's « hold 4 brighter » and every I-0138 pair measured with this tool (D-0299's pairs, tranches 8g to 10) carry the same
bias. `Compare-I141HoldSpread.py` on the three corrected tables (YAVG of 255 below the title bar; per pair in the order t23a,
t24b, t24c; the spread is a mode's range over the three):

| Hold | Cut, frame | Native | Original | Spread native, original | Native - original, mean (range) |
|---|---|---|---|---|---|
| 0 | 0, 209.4 | 86.9, 92.0, 88.9 | 82.3, 77.6, 83.5 | 5.1, 5.9 | +8.1 (+4.6 to +14.4) |
| 1 | 1, 128.6 | 96.3, 86.8, 94.0 | 86.3, 79.2, 84.0 | 9.4, 7.1 | +9.2 (+7.7 to +10.0) |
| 2 | 3, 51.8 | 97.4, 96.6, 92.3 | 96.0, 91.9, 95.1 | 5.1, 4.1 | +1.0 (-2.9 to +4.7) |
| 3 | 4, 221 | 137.8, 95.8, 124.8 | 123.7, 130.7, 111.9 | 42.1, 18.8 | -2.6 (-34.9 to +14.1) |
| 4 | 5, 14.56 | 119.6, 109.7, 106.8 | 98.7, 106.2, 104.8 | 12.7, 7.5 | +8.8 (+2.0 to +20.9) |
| 5 | 5, 70.4 | 103.8, 106.1, 110.0 | 104.9, 99.2, 103.2 | 6.2, 5.7 | +4.2 (-1.0 to +6.9) |
| 6 | 5, 140.2 | 95.3, 101.9, 97.0 | 98.9, 101.5, 97.0 | 6.6, 4.5 | -1.1 (-3.6 to +0.3) |
| 7 | 5, 196.04 | 21.1, 21.4, 21.0 | 20.0, 23.8, 20.6 | 0.4, 3.8 | -0.3 (-2.4 to +1.0) |

What it shows (OBSERVED on three pairs, whole frame below the title bar):
- The native is brighter at holds 0, 1 and 4 in every pair (by 4.6 to 14.4, 7.7 to 10.0 and 2.0 to 20.9); hold 0's mean lies
  beyond both modes' spreads, hold 1's at the spread's edge, hold 4's within it (one pair at +20.9). Hold 5 brighter in two of three.
- Holds 2, 6 and 7 differ by less than the spreads, either sign: no veil and no darkening at the last cut's end. The two negative
  pairs left at holds 6 and 7 coincide with the original's lit blade (`t23a` hold 6, `t24b` hold 7, on the sheets).
- Hold 3 (cut 4) moves by 42 natively and 19 in the original from run to run (a flash's phase, INFERRED).
- The blade's light differs by effect phase, not by mode: at hold 6 of `t24c` the native's blade carries the orange lightning and
  the original's does not, the other way round from `t23a` (the holds freeze the Remo clock, not the effects).

## Reviews at the merge of tranche 24 (2026-11-23)

**Final review** (final-reviewer on `main...c317e11dc`, the tree built as `build-i141-t24-final`, its tests passed): two High, two
Medium, five Low.
- H1, step 3's whole-capture luma included the window's title bar (about 35 natively, 235 in the original): fixed, the tool measures
  below it and the three pairs were measured again; step 3's claims rewritten (the native brighter at holds 0, 1 and 4, no
  darkening at holds 6 and 7). The I-0138 pairs carry the same bias: « Suggested next step » 3.
- H2, step 4's whole later windows were not tranche 18's state (the original's G-buffer at 351 and 485 models): recorded; the
  claim now rests on the partial window 4066 (seven of the 8 drawn by the original, `o328120` by neither) and the 137 are not
  re-tested.
- M1, pass 69 called « the forward pass »: it is 57A20's main view; forward stays INFERRED for the four pieces (no G-buffer row).
- M2, three stale Uncertainty bullets (tranches 7, 22 and the ceremony's walls): rewritten.
- L1, FadedCopy pairs parameters by position: left (the reviewer found only m21's, m22's and the templates' LightSet layouts differing
  from default's, and OwnSun refuses m21 and m22 already, a refusal older than this branch; not scanned here); the comment corrected.
- L2, the test's bools equal at both ends: left (the bank files the test loads hold no key whose bools differ; FadeValue's bool class
  is unit-tested in tranche 23's value-class test).
- L3, tranche 7's « fog step for step » OBSERVED on the record-blend path: the summary says so.
- L4, `native_pixel_key_gbuffer` and `reference_model_pass` overlap the new per-window rows: left (older tranches' readers use them).
- L5, a stale comment in `18_resource_admission.inc`: fixed.

**Integrity review** (integrity-reviewer on `main...683310827`): no High; one Medium, four Low. M1, the headlines put holds 0, 1 and 4
together though only hold 0 lies beyond the spread: the headlines say so. L1, the bar's effect stated as 5.8 on the original: about
4 on the original and 1.5 on the native. L2, the step-4 G-buffer counts without a saved reader: `gbuffer-windows.py` and the two
outputs saved (the 7.4 and 27.4 s offsets dropped, not reproduced). L3, « unreachable » and « ran live » stated as fact: INFERRED.
L4, the build of each pair and the runs' flags: stated. Every change measured, test and reader found fair to both modes.

**The tool fix's step review** (step-reviewer on `c317e11dc..683310827`, code only): no High, no Medium, six Low. L1, the bar is rows
0 to 30 (the crop from 32 drops one picture row in both modes): the comment corrected. L2, the 5.8 stated on the original: corrected
with the integrity review's L1. L3, the crop a fixed 32 rows, no size check: left (every capture of these pairs and of I-0138's t8g2,
t8h2 and t10 is 1920x1080 with the bar ending at row 30, as the reviewer measured; a capture at another scale would need it). L4,
columns 0 to 7 (wallpaper and border) still measured: left (about 0.03 of 255 on the difference). L5, FadedCopy's comment on m21
and m22 as the earlier end, L6, the 18_resource_admission comment too broad: corrected.

**After the merge** (`ade978954`): the launcher rebuilt with its unit tests (`glow_bank_test` 121 checks, 0 failures), the start
check passed (06:29, Paris, `start-ade978954`, restored); `Clear-DiskCache -Apply`: 4207 files, 3.2 GB (13 build folders older than
48 h), 450.4 GB free on C:.

## Tranche 25: I-0138's pairs below the title bar; what brightens 32000010 (2026-11-23)

Opus 5.5, `claude/r-i141-t25` (the fresh R thread after tranche 24). « Suggested next step » 3.

**I-0138's D-0299 pairs measured again** (OBSERVED; `Make-I138HoldPairs.ps1` as fixed in tranche 24, below row 32, on the runs'
own captures; tables and sheets in `evidence/I-0138/t25-body/<run>/`). The pairs: `i138-t8g2` (32000020 and 32000010, `build-t8g-7`),
`i138-t8h2` (32000010, `build-t8h-4`) and `i138-t8h3` (32000010, `build-t8h-5`), all held at '6000 p5 ... p95' from Eta's save
without the revive (the zone's clear-sky state, not tranche 24's storm). Tranche 10's pairs (`i138-t10`) were measured on the game's
1280x720 client (`t10-client-luma.txt`), so its conclusions never carried the bar's bias; its `.tsv` (whole 1920x1080 captures with
the desktop around the window) is not a measure.
- **32000020** (one pair): the native stays darker: 85.2/107.3, 83.9/97.8, 85.3/102.1, 148.8/147.3, 108.0/146.0, 103.6/123.3,
  254.1/254.1 (holds 0 to 6, native/original), so by 14 to 38 at holds 0 to 2, 4 and 5, level at 3 and 6 (the biased table said
  « darker in every pair by 4.3 to 42.7 »).
- **32000010** (three pairs, `Compare-I141HoldSpread.py`, `t25-body/c32000010-spread.txt`): the native brighter at every hold by
  the mean, beyond both modes' repeat spreads at six of ten: cut 0 (frames 69.8, 209.4, 349) +1.4, +3.0, +3.4; cut 1 frame 128.6
  +15.5 (+15.3 to +15.8); cut 5 frames 0.6 and 140.2 +16.8 and +16.6. Within the spreads: cut 2 +2.8, cut 3 +6.5, cut 4 +2.1 and
  +13.5. The biased tables had it « either way ».
- In the storm state (tranche 24's three pairs) cut 5 frame 140.2 is level (-1.1); in the clear state it is +16.6: the difference
  depends on the zone's state, not only on the cut.

**Where the difference lies** (`Compare-I141HoldRegions.py`, every output in `evidence/I-0141/artifacts/tranche-25/regions/`: the two captures below the bar at half size, each mode's luma quantiles,
a 6x4 grid of differences, the shares by the original's luma band; OBSERVED):
- Clear state (`i138-t8h3`, holds 3, 8 and 9): the native lifts the 5 % to 75 % quantiles by 8 to 29 and leaves the top equal (both
  clip at 255); the pixels the original shows at 64 to 159 carry 48 to 79 % of the difference, the darks below 64 lifted by +28 to +48 a
  pixel (hold 9's 64 to 159 band the most, +33.2). By eye on the sheets (`t25-body/t8h3/c32000010-hold-3.png`, `-9.png`): the castle's sunlit white wider and
  flatter natively, the King's shadowed side lighter, a pale glow over the bright sky at cut 1.
- Storm state (`i141-t24b`, holds 0, 1 and 4): at hold 0 the darks are level (-3 at 5 % and 25 %) and the upper half lifted (+14 to
  +53 from 50 % to 95 %), the right half of the frame (the clouds) most; hold 1 lifted from +5 at 5 % to +22 at 99 %; hold 4 mixed.
- The automatic exposure is not it: `native_auto_exposure` reads `log2_factor` 0 at every hold of the six native runs (both
  states; the view's mean 0.23 to 0.73 above the target 0.12), the tone map's exposure 2 (the area's) throughout.

**Each native share removed at the same holds** (`no-cutscene-fill-lights.flag`, new, a test tool: `Authored` answers false, so no set
keeps its lights 1 to 3 on a traced frame, D-0262's rule everywhere; with `no-yebis-glare` (the bloom and the shafts) and `rt-gi-no-sky`
(the GI's sky misses)). Script `tranche-25/run-t25.ps1`, logs `runs-t25-sun.log` and `runs-t25-storm.log`; build `build-i141-t25a`
(`e8baca9a5`, 194 PASS lines); one announced slot, the ASUS in a 1920x1080 window, offline, no measure, native only, every run exit 0
with no fault and the save restored by hash: the clear state `i141-t25s-<variant>-nat` (09:09 to 09:29, Paris; I-0138 t8h3's
protocol) and the storm state `i141-t25r-<variant>-nat` (09:29 to 09:57; tranche 24's protocol, `test-tools.flag` and
`test-revive.txt` placed beside the build before and removed after). A first launch at 07:02 never started (Start-Process split the
script's path at its space; no log, no run). Each variant one run, paired with one original run (YAVG below the bar;
`tranche-25/sun/variants.tsv`, `tranche-25/storm/variants.tsv`):

| Clear state hold (cut, frame) | Base | No glare | No GI sky | No fill lights | Original (t8h3) |
|---|---|---|---|---|---|
| 0 (0, 69.8) | 103.3 | 99.5 | 103.1 | 83.9 | 101.6 |
| 1 (0, 209.4) | 98.1 | 93.0 | 96.6 | 71.2 | 94.4 |
| 2 (0, 349) | 97.0 | 96.2 | 98.4 | 75.1 | 94.7 |
| 3 (1, 128.6) | 138.1 | 125.1 | 138.3 | 117.1 | 122.5 |
| 4 (2, 68.2) | 69.7 | 67.9 | 69.6 | 49.5 | 61.5 |
| 5 (3, 51.8) | 149.4 | 151.7 | 155.1 | 161.4 | 151.2 |
| 6 (4, 81.4) | 165.4 | 162.3 | 169.6 | 168.7 | 149.4 |
| 7 (4, 221) | 191.3 | 175.2 | 184.3 | 191.2 | 179.5 |
| 8 (5, 0.6) | 170.7 | 153.8 | 157.1 | 152.8 | 147.6 |
| 9 (5, 140.2) | 172.3 | 161.3 | 167.8 | 161.1 | 151.7 |

The storm state (holds 0 to 7 as tranche 24's, the original `i141-t24b`): base 90.2, 86.4, 94.1, 128.8, 108.9, 108.4, 106.1, 21.4; no
glare 84.9, 92.3, 95.8, 112.7, 115.5, 96.8, 90.5, 20.4; no GI sky 81.9, 90.4, 85.0, 115.1, 121.4, 101.7, 90.4, 20.2; no fill lights
63.0, 69.0, 93.4, 113.9, 106.2, 96.0, 90.5, 19.2; original 77.6, 79.2, 91.9, 130.7, 106.2, 99.2, 101.5, 23.8.

What it shows (OBSERVED, one run a variant on one build; the originals reused from `i138-t8h3` and `i141-t24b`, other builds and
other days; the base runs leave the earlier natives' ranges at clear holds 2, 5, 8 and 9 and storm hold 6, by up to 7.5, and
removing a share sometimes brightens a hold by 4 to 13 (clear hold 5 without the fill lights or the sky, clear hold 6
without the sky, storm holds 1 and 4 without the glare or the sky): single-run deltas around 10 are not separated from the noise; the shares are not additive, the glare being thresholded
on the scene the fill lights light; where every variant agrees and the base alone moves (storm holds 3 and 6, clear hold 8: the
three removals within 4.3 of each other, the base 13.6 to 17.9 above them), the delta is the base run's noise, not a share, and the
lists below give those holds for completeness only):
- **The fill lights (D-0279)**: removing them lowers clear cuts 0 to 2 by 19 to 27 from the base (against the original: -17.7,
  -23.2, -19.6, -5.4 at cut 1, -12.0 at cut 2), the storm's holds 0 and 1 by 27.2 and 17.4 (against the original -14.6 and -10.2),
  clear cut 5 by 17.9 and 11.2, and raises clear hold 5 by 12.0.
- **Our glare family** (the bloom and the shafts): clear cut 1 -13.0 (125.1 against the original's 122.5), cut 5 -16.9 and -11.0;
  storm holds 5 and 6 -11.6 and -15.6.
- **The GI's sky misses**: clear hold 8 -13.6, hold 9 -4.5, hold 7 -7.0, hold 5 +5.7; storm hold 0 -8.3, hold 2 -9.1, hold 6 -15.7.
- **Where the picture differs, not only its mean** (`regions/sun-hold-3-noglare-vs-orig.txt`): without our glare, clear cut 1's
  mean is +2.7 by cancellation: the pixels the original shows below 64 stay 25.6 lighter natively (the King and the dragon's shadowed
  side, cells up to +47), the bright ones 8.5 darker, and the top-right sky cells 13 to 15 darker than the original's, where the base
  is level (`regions/t8h3-hold-3.txt`): so the original's picture carries a lift there too. Without the fill lights the same hold is
  closer region by region (`regions/sun-hold-3-nofill-vs-orig.txt`: mean -5.4, cells -29.5 to +20.0).

**Reading.** No share is attributed: the mean shares above are each of the size of the excess, at several holds of each other's,
with single-run noise of the same order; region by region the clear excess at cut 1 sits on the lit foreground (the King, the
dragon), not as a veil the original lacks. The storm state's excess at hold 0 (+8.1 on three pairs, beyond the spread; +12.6 here)
stays unexplained. Next: each variant repeated on one build with a same-build base spread, a combined run without the fill lights
and the glare, and the foreground's region compared (or its light terms read) before the excess is put on workstream 11's bloom or
workstream 12's light.

## Reviews at the merge of tranche 25 (2026-11-23)

**Step review** (step-reviewer on `ff484b3e6..e8baca9a5`, the flag, the runner's pattern and the reader): no High; one Medium, five Low.
- M1, the reader's size check ran on ffmpeg's output after its own crop and scale, so it could never fire (a 2560x1440 capture would
  be cropped silently): fixed, the full frame is decoded and must be 1920x1080, the crop and the 2x2 mean made in the reader, an
  ffmpeg failure printed with its message.
- L1, `--grid 6x4` as the header wrote it did not parse, `__doc__` was empty, a zero grid divided by zero: fixed (both forms, the
  bounds checked, the header a docstring, an unknown option refused).
- L2, the band shares meaningless when the whole difference is near 0: « n/a » under 0.5 of 255 a pixel.
- L3, 14 sorts of 500,944 values: one 256-bin histogram.
- L4, no `-c:v mjpeg` and no hidden window: both added (D-0199).
- L5, the record cited a file holding only the grid for its quantiles, and « near-uniform » did not fit cells of +1.9 to +30.6:
  every output saved whole under `tranche-25/regions/` (recomputed with the fixed reader: the same figures), the wording « a veil
  over the whole frame, uneven by region ».
The flag's coverage (the deferred `authored_sets` and the forward rows, `CutsceneShadowLightSet` untouched) and the pattern's two
groups held.

**Final review** (final-reviewer on `main...08f1715a3`, the tree built as `build-i141-t25-final`, 194 PASS lines): « not ready to
merge »; two High, three Medium, five Low.
- H1, « our bloom carries the clear state's excess » was not supported: without the glare cut 1's +2.7 is a cancellation (the
  original's darks still 25.6 lighter natively, the bright ones 8.5 darker, the sky's cells 13 to 15 darker than the original's),
  the fill lights' removal closes the gap as well or better region by region, the shares are not additive: the attribution
  withdrawn everywhere (the shares stated as observed, none attributed; the item stays workstream 7's).
- H2, the storm's hold 0 (+8.1 beyond the spread) dropped by « within the spread »: restored as unexplained in the next step and
  ROADMAP; the sky's -8.3 there stated.
- M1, the fill lights' figures mixed the base-relative and the original-relative: both given; « change little » dropped.
- M2, the base runs do not repeat the earlier natives at clear holds 2, 5, 8, 9 and storm hold 6: stated, with the single-run noise.
- M3, the sky « small »: its deltas given against the spreads.
- L1, tranche 25 missing from the Conclusion, Uncertainty and Artifacts: added. L2, I-0138's stale sentences (the 8g note, « darker
  in every pair of 32000020 » twice): corrected. L3, two region descriptions: corrected. L4, the clear spread mixing three builds:
  stated in Uncertainty. L5, the reader's fix without a step review of its own: left, the final reviewer checked it (it reproduces
  two saved outputs byte for byte), a measuring reader changed only as the step review asked (D-0268, stated above).

**Integrity review** (integrity-reviewer on `main...f503abadc` with this section): no High, two Medium, three Low; no test weakened,
the flag a test tool off by default and unused in a merged run, no scene special case, every table figure found in the saved tables.
- M1, the share lists counted deltas where every variant agrees and the base alone moves (storm holds 3 and 6, clear hold 8): stated
  as the base run's noise, not a share, in « What it shows », step 3 and STATE.
- M2, « the reviewer's run of the reader » cited no file: the output saved as `regions/sun-hold-3-nofill-vs-orig.txt` and cited.
- L1, « the clear excess on the lit foreground » held at cut 1 only: « at cut 1 » added in the Conclusion and STATE. L2, the view's
  mean starts at 0.23, not 0.25 (recomputed by the reviewer from the six runs' rows): corrected. L3, the list of holds a removal
  brightens by 4 to 13 was incomplete: clear hold 5 without the sky (+5.7) and hold 6 without the sky (+4.2) added.

**After the merge** (`3f320c468`): the launcher rebuilt with its unit tests (`tranche-25/build-main-3f320c468.log`: exit 0,
195 PASS lines, no failure), the start check passed (10:42 to 10:43, Paris, `start-3f320c468`, restored); `Clear-DiskCache -Apply`:
11259 files, 14.8 GB (23 build folders older than 48 h, 1277 raw frames older than 7 days), 458.1 GB free on C:.

## Tranche 26: 32000010's native shares switched live at one held moment (2026-11-23)

Opus 5.5, `claude/r-i141-t26` (the fresh R thread after tranche 25). « Suggested next step » 3, with the coordinator's idea of
reading the shares in one picture rather than in separate runs, taken in this form: every share switched off and on during one
cutscene hold, so each variant is shot at the same held moment of one run (D-0299), and the noise between runs leaves the shares.

**The tool** (test tooling, a bounded run only; outside one nothing is read or changed). `witness-variant.txt` beside the module
holds the share variant's bits, read at the frame's start every 15th call and removed once read, with a `witness_share_variant` row
(the bits, the scene clock's cut and frame): 1 the cutscene sets' lights 1 to 3 (`Authored` false, as `no-cutscene-fill-lights.flag`),
2 the glare family served empty as the library's own skip (no bloom, no shafts; the depth of field and the blur's Alpha_R kept,
unlike `no-yebis-glare.flag`, which drops the whole plan: 32000010's clear state draws no DoF, `native_yebis_dof` « depth of field
off » in `i141-t25s-base-nat`, so tranche 25's « no glare » lost nothing else there), 4, 8 and 16 the traced rays' sky misses, the
cache's ambient and the hits' local lights (R7a's diagnostic bits, ORed directly into the GI's rays, the specular rays on the
frames NRD denoises and the cache's update only, the final review's H1; every pass reading the cache that frame, the probes' hits,
the water and RR's reflections, reads cells built under them once the cache has turned over, while those passes keep their own
misses' sky and their hits' local lights, the step reviews' M1; the runs: dlss off, every GI frame through NRD; at the PT level bit 16 does nothing, the fourth step review's M1). Run-AlphaCast `-CutsceneHoldVariants "bits:ms ..."` shoots each hold's first picture as before, then for each entry writes the
bits, waits for the module's row, waits ms, shoots `cutscene-hold-<k>-v<bits>-<j>.png`; it returns the module to 0 before the shot
file releases the hold (a return not taken in 10 s stops the variants for the run). The holds' timeout bound is 30 s (was 20 s) in
the module and the runner. The reader `Compare-I141HoldVariants.py`: each shot's luma as Make-I138HoldPairs' (YAVG below row 32),
each variant against the mean of the base shots just before and after it; a hold is refused unless released by its shot with no
drift at its own frame, with no share off before its first shot, every variant row read at the hold's scene frame and a base shot
last.

**The runs** (OBSERVED; build `build-i141-t26a`, 195 PASS lines; script `tranche-26/run-t26.ps1`, logs `runs-t26-sun.log` and
`runs-t26-storm.log`; one slot, the ASUS in a 1920x1080 window, offline, no measure, native only, each run exit 0 with no fault and
the save restored by hash): the clear state twice (`i141-t26sa-nat`, `i141-t26sb-nat`, 11:05 to 11:22, Paris; I-0138 t8h3's
protocol, holds '30000 p5 ... p95'), the storm state twice (`i141-t26ra-nat`, `i141-t26rb-nat`, 11:22 to 11:42; tranche 24's
protocol, `test-tools.flag` and `test-revive.txt` beside the build for those two only). At each hold: the base, 1, 2, 3, 0, 4, 4 again
2 s later, 0, 8, 0. 35 of 36 holds kept (one shot of `i141-t26sa-nat`'s hold 7 took 11 s, the hold timed out: refused). The auto
exposure stayed at `log2_factor` 0 in every row of the four runs (the view's mean 0.19 at its lowest, above the target 0.12), so no
variant moved the exposure. Tables: `tranche-26/sun/variants.txt`, `tranche-26/storm/variants.txt` (`variants-final.txt` beside
each: the same with each hold's base range, the integrity review's M1).

| Clear state hold (cut, frame) | Base, first shot (sa / sb) | Original (t8h3) | Fill lights | Glare | Both | Rays' sky | Cache ambient | Base range in the hold (sa / sb) |
|---|---|---|---|---|---|---|---|---|
| 0 (0, 69.8) | 102.8 / 103.5 | 101.6 | -19.8 (-20.1..-19.5) | -4.1 | -20.9 | -0.4, -1.3 | -0.8 | 1.4 / 1.9 |
| 1 (0, 209.4) | 98.3 / 98.9 | 94.4 | -23.9 (-26.0..-21.8) | -4.0 | -27.8 | +0.2, +1.7 | +0.9 | 2.5 / 3.1 |
| 2 (0, 349) | 98.6 / 97.1 | 94.7 | -20.8 | -4.1 | -22.6 | +1.1, -1.1 | +1.1 | 1.2 / 2.8 |
| 3 (1, 128.6) | 138.4 / 138.4 | 122.5 | -20.6 (-21.0..-20.2) | -12.9 (-12.9..-12.8) | -38.0 | +0.1, +0.1 | -1.2 | 0.3 / 0.5 |
| 4 (2, 68.2) | 70.0 / 70.3 | 61.5 | -19.7 | -2.4 | -20.3 | +0.4, +0.3 | -0.7 | 0.2 / 0.7 |
| 9 (5, 140.2) | 168.2 / 170.6 | 151.7 | -8.4 (-10.0..-6.8) | -9.4 (-11.1..-7.8) | -16.2 (-17.9..-14.5) | 0.0, +0.3 | -0.9 | 6.6 / 2.0 |

Each share is the mean over the two runs of the variant minus its neighbouring bases (the two runs' range in brackets where shown;
the two « rays' sky » figures the shots 1.5 s and about 4 s after the row; the base range, the largest minus the smallest of the
hold's four base shots: a share below it is within the base's own motion). Hold 9's shares carry `i141-t26sa-nat`'s middle base
(173.6 against 168.2, 167.3, 167.0). Holds 5 to 8 (cuts 3 to 5) moved inside the hold by -11.8 to
+11.8 between their own base shots (the world's effects and the dragon go on while the Remo clock is held): their shares are not
separated (hold 8's « both », -17.1 and -10.6, stands above its drift in `i141-t26sa-nat` alone, -7.6; +11.8 in the other).

**The storm state**: the base moves inside a hold by up to 25.5 (base ranges: `i141-t26ra-nat` holds 1 to 6 8.7 to 24.0,
`i141-t26rb-nat` holds 0 to 6 6.7 to 25.5). Hold 0 (cut 0, 209.4), tranche 25's unexplained one, is steady in `i141-t26ra-nat` alone
(base range 0.8): fill lights -18.1, glare -2.6, sky +1.2 and -5.2 (the second shot, beyond the base's range: in one run, the sky's
share there not ruled out), ambient -0.1; in `i141-t26rb-nat` its base reads 89.3 at the first shot and 78.1 at
the last (the original 77.6). Hold 7 (cut 5, 196.04) moves little in both (base ranges 1.5 and 3.6) and is dark (21 to 25): fill
lights -2.2, both -3.3, the rest within 1.3, all within the base's own motion of the second run.

**Where the clear difference lies, region by region** (`Compare-I141HoldRegions.py`,
`tranche-26/regions/sun-hold-<k>-<variant>-vs-orig.txt`, `i141-t26sb-nat`'s shots against `i138-t8h3`'s original; OBSERVED):
- Hold 3 (cut 1): the base +16.0; its bottom row (the King, the dragon) +28 to +58 on its four left cells, +25 and +11 on the right;
  the middle rows +8 to +24 on their four left cells; the sky's right columns level (-0.1 to +1.2). Without the glare (+3.0): the
  bottom row's left cells still +20 to +49, the sky's right cells 13 to 15 darker than the original's, the darks (the original's luma
  0 to 63) still +27.1 and the brights -8.4. Without the fill lights (-4.4): the bottom row +3, +26, +10, -30, -17, -9, the middle
  columns of the middle rows 8 to 18 below the original's. Without both (-22.2): every cell below but two of the bottom row.
- Hold 9 (cut 5, 140.2): the base +19.1, +23 to +62 in the middle columns and +32 to +49 on the bottom row, -43 at the bottom right;
  the variants move cells by up to 40 while the base drifts 1.7, the scene's own effects among them: no region reading.

**Reading.**
- The sky misses and the cache's ambient of the GI, the specular rays and the cache's update carry at most 1.7 in the two runs'
  mean at every steady hold of the clear state (3.2 in one run): at cuts 1 and 2, at most 1.2 against base ranges of 0.2 to 0.7;
  at cut 0 within the base's own motion (ranges 1.2 to 3.1). In those passes neither is 32000010's clear excess (OBSERVED; tranche
  25's single runs gave them up to 13.6, the runs' own noise). The storm's hold 0, in its one steady run: +1.2 and -5.2 (above the
  range 0.8), the sky's share there not ruled out.
  The probes' own sky misses and their hits' local lights, which light the forward rows and the effects (their hits' ambient reads
  the cache), were not removed (the final review's H1): their share is not measured.
- The fill lights' share is about -20 at every steady hold of cuts 0 to 2 (-19.7 to -23.9; -8.4 at cut 5's frame 140.2), where the excess over the original is
  +1.2 to +4.5 (cut 0), +15.9 (cut 1) and +8.6 (cut 2): the share does not follow the excess. The glare's share is largest where the
  excess is: -12.9 at cut 1 and -9.4 at cut 5's frame 140.2 (excess +15.9, and +17.7: +16.5 and +18.9 in the two runs), -2.4 to -4.1 at cuts 0 and 2. Removing both
  leaves cut 5's frame 140.2 at 156.4 and 151.8 against the original's 151.7, cut 1 at 100.3 against 122.5. The shares are not
  additive at cut 1 (both -38.0 against -33.5 summed; the glare is -17.4 without the fill lights): the tone curve's shoulder takes less
  of the bloom on the brighter, fill-lit picture (INFERRED).
- So the excess sits in the native's glare family and the fill-lit foreground at cut 1, and in both alike at cut 5; but the original
  has its own bloom and its own lights 1 to 3 there (its sky at cut 1 carries a lift the native's no-glare picture lacks): a native
  share says how much the native draws, not how much more than the original. No share is attributed (INFERRED, not shown: the
  glare). The next step measures the original's own shares at the same holds.
- The storm's hold 0: the scene moves by 11.2 in one hold of `i141-t26rb-nat` (89.3 to 78.1), the size of the +8.1 « excess » of
  three pairs; its weather runs on the world's clock, which the Remo hold does not stop (INFERRED: the storm's hold 0 is not one
  moment of the world, so its pairs compare moments that differ; not checked against the original's own drift in a hold).

**Step review** (step-reviewer on `28dfcd4bb..3b04632c1`: the module's variant, the runner, the reader): no High; two Medium, seven Low.
- M1, a return to 0 not taken in 3 s deleted the file and left the module at the last variant, so the next hold's base shot could
  carry a share still off, unseen by the reader: fixed (the runner waits 10 s, then stops the variants for the run and leaves the
  file; the reader refuses a hold whose last variant row before it is not 0). Not met in the runs (every return taken).
- M2, bits 4, 8 and 16 named the GI's only while R7a's diagnostics reach the reflections, the cache's update and the probes too:
  the comments, the runner's help and this record relabelled, still wrong about the probes (the final review's H1, below).
- L1, a file filling the buffer or with text after the number was taken truncated: refused whole. L3, the module's rows capped at
  512 under the runner's 544 possible: 2048. L4, no base shot after the last variant unless the list ends with 0: the reader refuses
  such a hold (the runs' list ends with 0). L5, the reader lacked Make-I138HoldPairs' drift and release-frame checks: added.
- Left: L2 (the room check depends on the frame rate: at a low traced rate the hold times out and is refused, as seen once); L6 (the
  GI's cache and histories persist from one variant to the next) answered in the runs' list by a base shot between the GI variants
  and the second sky shot; L7 (a leftover file read before the start's flags): answered at the merge (the final review's L6).
- The module's L1 and L3 fixes, the final review's L6 and L10 and the comments came after `build-i141-t26a`; the runs' files were the runner's well-formed digits
  and 80 rows a clear run, 64 a storm run, none refused and no leftover, so neither path changed what the runs measured.

## Reviews at the merge of tranche 26 (2026-11-23)

**Final review** (final-reviewer, `main...85beae437`, on the tree built as `build-i141-t26-final`, 195 PASS lines): one High, two
Medium, ten Low.
- H1, bits 4, 8 and 16 never reach the probe pass (`EncodeRtProbes` builds its constants without `rt_gi_diagnostic`), while the
  comments, the runner's help and this record said they did, and the rays' sky share was ruled out on that: the comments and the
  records narrowed, no code change (the runs measured what the code did). Its first answer was too narrow in turn (the second step
  review's M1: the cache's cells, built under the bits, are read by the probes' hits, the water and RR's reflections that frame; only
  the probes, the water and RR's reflections keep their own misses' sky and their hits' local lights, the third step review's M1):
  rewritten so. The probes' own sky misses are a share left to measure (« Suggested next
  step » 3).
- M1, headline figures wider than the tables (« about -20 at every steady clear hold » against hold 9's -8.4; « at most 2 » against
  the storm's -5.2): each claim restricted to its holds. M2, the storm's and cuts 3 to 5's shares dropped from « Left » on an
  INFERRED reading: back in « Suggested next step » 3 and ROADMAP 7.
- L1 the hold count (35 of 36), L2 the excesses (one figure, the runs' two named), L3 hold 8's « both » (one run), L4 the second sky
  shot (about 4 s), L5 the rows (64 a storm run), L7 the runner's help (the shot's ticks only), L9 STATE (« 26 at its merge »): fixed
  in the records. L6, a leftover `witness-variant.txt` read before the start's flags: removed unread, its failed removal retried
  (the second step review's L2). L8, an entry the runner skips leaves no tsv line: the reader refuses a hold whose variants differ
  from `alphacast-args.json`'s list. L10, an embedded NUL taken as a line end: the check runs to the bytes read. L10's other half,
  tranche 25's `no-cutscene-fill-lights.flag` read with no bounded-run gate and doubling bit 1: left, a test flag beside the module
  like the other `ModuleFlag` test tools (a player's install has none).
- Second step review (step-reviewer, `85beae437..8512de745`): no High; M1 (the cache carries the bits to its readers) and L1 (the
  specular rays get them on NRD's frames only, RR off; the runs: dlss off, every GI frame through NRD) fixed in the comments and
  records; L2 (a failed removal let a leftover be read) fixed; L3 (EncodeRtNrd traces the GI's rays too on NRD frames) in the
  comments.
- Third step review (step-reviewer, `8512de745..a84ba76a0`: the leftover's retry, the reader's base range): no High; M1 (the water
  and RR's reflections keep their own sky and local lights too) fixed in the comments and records; M2 (the reader failed on the
  module's leftover rows, which carry no tick) fixed; L1 (a runner's file taken for the leftover when the leftover vanished between
  two checks or was overwritten in place) and L2 (an unreadable attribute taken as absent) fixed by telling a leftover by its last
  write before the module's first call, and an unreadable file waiting for the next check; L3 (cells mixing the frames before a
  switch for a few frames) in the comments (the runner's settle time covers it). This last change built as
  `build-i141-t26-final4`.
- Fourth step review (step-reviewer, `a84ba76a0..a0ee7566b`): no High; M1, bit 16 does nothing at the PT level (`HitLight`'s PT
  branch samples one local light whatever the bit; the runs: `"level":"rt"`, no PT frame): said in the comments and the « Tool »;
  L4 (the specular without the bits on every frame NRD does not denoise, RR's or NRD refused) in the comments. Left: L1 (a leftover
  that can never be removed, read-only, logs a row a check and can spend the 2048 rows in about 8 min), L2 (a clock stepped back
  after the first call could take a runner's file for a leftover) and L3 (a runner's file written between the time check and the
  removal of a surviving leftover): each needs a leftover surviving to a hold, which the runner's clean-up and a writable folder
  prevent; the reader refuses such a hold (no row, the entry skipped). The comment-only answer built in the launcher's rebuild.

**Integrity review** (integrity-reviewer, `main...8512de745` with this section): no High; one Medium, four Low. Re-running the reader
on the four runs gives both tables byte for byte; every sampled figure matched.
- M1, the « drift » (last base minus first) hid the middle bases' motion: the reader prints each hold's base range too
  (`variants-final.txt`), the table gives it, and the shares within it are named so (cut 0's sky and ambient, the storm's hold 7,
  hold 9's middle base in `i141-t26sa-nat`).
- L1 the fixes after the runs' build named in full and the step review's L7 marked answered; L2 the region files' inputs written
  beside them (`regions/inputs.txt`; the ten files re-made identical); L3 the storm's hold 0 quoted from its steady run alone; L4 the
  storm's hold 0 against its own +8.1: fixed.

**Merge** (2026-11-23 12:29, `7d0ae0723`): the launcher rebuilt in the main checkout (195 PASS lines, `tranche-26/launcher-rebuild.log`),
the start check of `7d0ae0723` passed (12:37 to 12:38, restored), the disk rotation 3.2 GB (6 build folders, 338 raw frames).

## Tranche 27: the original's own shares at 32000010's holds; the world frozen in a hold; the ambient's shares (2026-11-23)

Opus 5.5, `claude/r-i141-t27` (a fresh R thread). « Suggested next step » 3: the original's own shares at the same holds as
tranche 26's native ones, then the native's against them; the coordinator's idea (the previous thread's) of freezing the world's
clock during a hold, taken once the original's storm holds were seen to move too.

**Step 1, the original's fill lights and glare** (test tooling, `reference-share-variant.flag`, a bounded Reference run only;
commit `4d0f8ce69`). The same `witness-variant.txt` read from the Remo Play_Wait hook (a Reference run has no BeginFrame of ours),
bits 1 and 2: bit 1 zeroes lights 1 to 3's Col and Spc rows (5 to 7, 9 to 11) where D-0262 takes them out natively, in the entries
137910 builds with 1496D0 or the blend 14A2D0 (return addresses 140137A82, 140137AC4; the effects' 264230 and 32650 left whole) and
in the forward draws' sets 5A120 builds with 149CB0 or 14AB20 (handed a copy of the record with the three lights' intensities 0:
+0xCC, +0xDC, +0x13C, +0x14C, +0x1AC, +0x1BC, each row c*c*w with w*w last, READ); bit 2 stores glare shape 0 through 1416C9690, the
library's own skip (no bloom, streaks, ghost, shafts; the DoF and the blur before its gate, I-0138 tranche 8c). The rows count each
path (`paths`). Refused beside `light-observe.flag` (it hooks 1496D0 and 5B3D0 too) and in original-probe mode.

**The first slot** (OBSERVED; build `build-i141-t27a`, 195 PASS lines; `tranche-27/run-t27.ps1`, logs `runs-t27-sun.log`,
`runs-t27-storm.log`; 13:01 to 13:35 Paris, the ASUS in a 1920x1080 window, offline, no measure, each run exit 0 with no fault, the
save restored by hash; `test-tools.flag` and `test-revive.txt` beside the build for the storm runs only, removed after): Reference
runs only, the clear state twice (`i141-t27sa-orig`, `i141-t27sb-orig`, holds '30000 p5 ... p95'), the storm twice
(`i141-t27ra-orig`, `i141-t27rb-orig`, tranche 24's protocol). At each hold: the base, 1 (shot 1.5 s and 4 s after the module's
row), 0, 2 (the same), 0, 3, 0. 36 of 36 holds kept by the reader. 32000010 keeps glare shape 1 in every clock row
(`i141-t27sa-orig`: 570 rows), so bit 2 reached it (the step review's M1: a shape-14 custom glare would not be reached). Tables:
`tranche-27/sun/variants.txt`, `tranche-27/storm/variants.txt`.

| Clear hold (cut, frame) | Original base (sa / sb) | Original: fill | glare | both | Native (t26): base | fill | glare | both | Without both: native - original |
|---|---|---|---|---|---|---|---|---|---|
| 0 (0, 69.8) | 101.0 / 101.4 | -16.9 | -3.5 | -17.2 | 103.2 | -19.8 | -4.1 | -20.9 | -1.7 |
| 1 (0, 209.4) | 95.1 / 94.7 | -20.0 | -2.7 | -21.4 | 98.6 | -23.9 | -4.0 | -27.8 | -2.7 |
| 2 (0, 349) | 96.1 / 96.3 | -19.0 | -1.6 | -17.5 | 97.9 | -20.8 | -4.1 | -22.6 | -3.4 |
| 3 (1, 128.6) | 122.4 / 122.4 | -19.1 | -14.8 | -37.8 | 138.4 | -20.6 | -12.9 | -38.0 | +15.8 |
| 4 (2, 68.2) | 61.9 / 62.2 | -15.5 | -1.2 | -15.7 | 70.2 | -19.7 | -2.4 | -20.3 | +3.5 |
| 9 (5, 140.2) | 149.6 / 150.1 | -10.2 | -12.6 | -18.8 | 169.4 | -8.4 | -9.4 | -16.2 | +22.1 |

The original's shares: the first shot after the row (the second, 2.5 s later: the two runs' means within 2.9 of it; one run's up to 5.4 and 6.8 at cut 5's frame 140.2, which moved
unfrozen); the native's from
tranche 26 (the mean of its two runs; « without both » each mode's mean base minus its « both »). The original's steady holds
repeat between its runs to 0.1 at cuts 1 and 2 (base ranges 0.2 to 0.7), its cut-0 holds move by 2 to 4.7 inside a hold.

**Reading.**
- The original's fill lights take 3 to 4 less than the native's at the steady clear holds (-15.5 to -20.0 against -19.7 to -23.9),
  its glare about as much (-1.2 to -3.5 at cuts 0 and 2 against -2.4 to -4.1, -14.8 at cut 1 against -12.9); « both » -37.8 against
  -38.0 at cut 1 (OBSERVED). Neither share is the excess: with both removed in each mode, cut 1 stays +15.8 brighter natively (its whole
  base excess, +16.0) and cut 5's frame 140.2 +22.1 (+19.6); cuts 0 and 2 drop to -3.4 to +3.5.
- Where, at cut 1 with both removed (`tranche-27/regions/sun-hold-3-both-nat-vs-orig.txt`, `i141-t26sb-nat` v3 against
  `i141-t27sa-orig` v3; `hold3-quad.jpg`, `hold3-both-bottomleft.jpg`): the medians 91 against 39, the brights equal (75 % 153
  against 147, 99 % 255 both); 92 % of the difference on the original's darks (luma below 64, 59.6 % of the pixels, +24.2 there):
  the dragon's wing and the King, lit grey-lavender natively, dark blue in the original. A broad light on the shadowed side, the
  ambient's (INFERRED from the picture: it is what lights a surface the sun and lights 1 to 3 do not).
- The storm: the original moves inside its holds too (base ranges 1.2 to 22.2 in `i141-t27ra-orig`, 1.7 to 15.6 in
  `i141-t27rb-orig`; hold 0 2.8 and 7.0): the storm's shares are measurable in neither mode with the Remo clock alone held.

**Step 2, the world frozen in a hold and the ambient's shares** (test tooling, a bounded run; commits `2848b4136`, `2c4983558`,
`f83f69e55`). `hold-world-freeze.flag` (both modes): from the frame after a hold starts to its release, F0FC90 (the timeline's run,
called once a frame by MainLoopTick at EDAC5A) is handed dt 0 and the limiter's mean of 16 (+0x26C, the cloth's) zeroed with it
(READ, `t27-world-clock-reading.md` and the pilot's GhidraMCP reading); the fixed-step tasks (the FFX wind 1/30, YEBIS's 1/60) go on
(INFERRED), a 31 s safety release, a `witness_hold_world_freeze` row at each change. Bits 32 and 64 of the share variant: natively the
traced diffuse GI's compose (EncodeRtGi skipped, the frame counted served) and SpecularAcc cleared, the main view's deferred surfaces
only; in a Reference run the instance record's SH rows +0x80..+0xAF zeroed after 5B3D0 builds them (pass kinds 0, 3, 4; that these
kinds are the Ibl G-buffer programmes' GB4 INFERRED, the kind-to-pass tie not read), the forward cube scales +0xEC and +0xFC zeroed,
and the deferred specular IBL's region pass 675B0 handed count 0 (READ, `t27-original-ambient-reading.md`); every view the hooks
reach. The original's bit 32 leaves the map pieces' lightmapped ambient, the native's takes the whole traced diffuse GI of the
deferred surfaces: the two removals are not the same. The scopes differ by mode on the forward draws, so a
pair's share is read on the deferred surfaces.

**The second slot** (OBSERVED; build `build-i141-t27d`, 195 PASS lines, commit `f83f69e55`; `tranche-27/run-t27b.ps1`, logs
`runs-t27b-sun.log`, `runs-t27b-storm.log`; 14:17 to 14:51 Paris, as the first slot): one run a mode a state, `hold-world-freeze.flag`
in all four (`i141-t27sc-nat`, `i141-t27sc-orig` the clear state; `i141-t27rc-nat`, `i141-t27rc-orig` the storm, the original's with
`shaft-observe` too). At each hold the base, then 3, 0, 32, 0, 64, 0, 99 (1+2+32+64), 0, each shot once 1.5 s after the module's row.
36 of 36 holds kept; a `witness_hold_world_freeze` on and off row at each hold (1260 to 1297 frames zeroed a hold, 842 to 1275
in `i141-t27rc-nat`, whose holds lasted the same 21.8 to 22.8 s at a lower frame rate), no `reset` row.
**The world held: every base range 0.0 to 0.3 in both modes and both states** (the first slot's original unfrozen: up to
22.2 in the storm, 2 to 4.7 at cut 0; tranche 26's native: up to 25.5 in the storm, 1.3 to 3.2 at cut 0), so cuts 3 to 5 and the storm are measurable now. Tables: `tranche-27/sun/variants-t27b.txt`, `tranche-27/storm/variants-t27b.txt`.

| Clear hold (cut, frame) | Base native / original | 3 (fill+glare) n / o | 32 (diffuse ambient) n / o | 64 (specular ambient) n / o | 99 (all four) n / o | Without all: n - o |
|---|---|---|---|---|---|---|
| 0 (0, 69.8) | 107.0 / 107.6 | -19.6 / -16.9 | -0.8 / -0.8 | -0.8 / -1.3 | -20.8 / -19.9 | -1.5 |
| 1 (0, 209.4) | 96.2 / 95.7 | -26.1 / -22.4 | -1.5 / -1.9 | -1.6 / -2.2 | -29.4 / -28.4 | -0.5 |
| 2 (0, 349) | 98.9 / 94.4 | -23.5 / -19.0 | -1.3 / -1.3 | -1.4 / -2.1 | -26.1 / -24.1 | +2.5 |
| 3 (1, 128.6) | 137.6 / 121.8 | -38.1 / -37.9 | -8.9 / -1.0 | -7.2 / -3.8 | -68.8 / -55.2 | +2.2 |
| 4 (2, 68.2) | 70.1 / 62.0 | -20.1 / -15.6 | -3.1 / -0.3 | -7.6 / -4.3 | -33.7 / -26.2 | +0.6 |
| 5 (3, 51.8) | 162.4 / 153.9 | -9.8 / -10.9 | -3.4 / -0.1 | -0.6 / -0.1 | -15.7 / -11.1 | +3.9 |
| 6 (4, 81.4) | 164.7 / 147.7 | -10.1 / -13.7 | -9.1 / 0.0 | -2.2 / -0.8 | -25.3 / -14.9 | +6.6 |
| 7 (4, 221) | 174.6 / 165.9 | -6.4 / -11.2 | -10.7 / -0.2 | -2.3 / -0.5 | -22.3 / -12.9 | -0.7 |
| 8 (5, 0.6) | 146.5 / 150.5 | -5.1 / -5.8 | -0.8 / -0.1 | -0.8 / -0.4 | -6.7 / -6.3 | -4.4 |
| 9 (5, 140.2) | 166.3 / 148.1 | -16.7 / -21.1 | -9.7 / -0.9 | -7.2 / -2.1 | -45.9 / -31.2 | +3.5 |

| Storm hold (cut, frame) | Base native / original | 3 n / o | 32 n / o | 64 n / o | 99 n / o | Without all: n - o |
|---|---|---|---|---|---|---|
| 0 (0, 209.4) | 82.4 / 80.9 | -18.1 / -19.9 | -1.9 / -1.3 | -1.9 / -2.1 | -21.9 / -23.6 | +3.2 |
| 1 (1, 128.6) | 83.0 / 81.5 | -21.1 / -10.0 | -2.3 / -0.6 | -2.5 / -1.6 | -25.7 / -12.6 | -11.6 |
| 2 (3, 51.8) | 104.7 / 95.8 | -1.6 / -0.3 | -0.1 / 0.0 | 0.0 / 0.0 | -1.7 / -0.3 | +7.5 |
| 3 (4, 221) | 139.8 / 138.3 | -8.2 / -7.2 | -1.0 / -0.1 | -0.3 / -0.3 | -9.3 / -7.6 | -0.2 |
| 4 (5, 14.56) | 138.9 / 117.0 | -6.7 / -4.4 | -0.2 / -0.1 | -0.2 / -0.2 | -7.0 / -4.7 | +19.6 |
| 5 (5, 70.4) | 102.5 / 107.8 | -4.3 / -7.1 | -0.7 / -0.6 | -0.4 / -1.0 | -5.2 / -8.8 | -1.7 |
| 6 (5, 140.2) | 92.3 / 95.6 | -13.8 / -13.0 | -1.8 / -1.2 | -2.4 / -2.7 | -18.6 / -18.3 | -3.6 |
| 7 (5, 196.04) | 21.0 / 20.9 | -3.3 / -2.8 | -0.4 / -0.3 | -0.6 / -0.6 | -4.3 / -4.1 | -0.1 |

**Reading.**
- With the four shares out in each mode (bits 99), the clear state's frame means agree to -4.4 to +6.6 at every hold (to 2.2 and 3.5
  at cut 1 and cut 5's frame 140.2, where the bases differ by +15.8 and +18.2; OBSERVED); at cut 1 most pixels are black in both
  modes then (median 2), and cells of the 6x4 grid still differ by up to about 18 (`sun-hold-3-v99-nat-vs-orig.txt`,
  `sun-hold-9-v99-nat-vs-orig.txt`). So 32000010's clear excess, as a frame mean, is in those shares, not in what is left (the sun, the original's map pieces' lightmapped ambient, the native's forward rows' probes).
- Of them, **the diffuse ambient carries the most of the excess**: natively -8.9 to -10.7 at cuts 1, 4 and 5's frame 140.2 (and -3.1,
  -3.4 at cuts 2 and 3), in the original -1.0 to +0.0 at the same holds (its instance SH, the pass kinds 0, 3 and 4). The specular
  ambient next: -7.2 against -3.8 (cut 1), -7.6 against -4.3 (cut 2), -2.3 against -0.5 (cut 4's frame 221), -7.2 against -2.1
  (cut 5's frame 140.2), less than the original's at cut 0 (-0.8 to -1.6 against -1.3 to -2.2). The fill lights and glare (bit 3)
  take as much in both modes at cut 1 (-38.1 against -37.9), cut 3 and cut 5's frame 0.6; natively about 4 more at cuts 0 and 2
  (-26.1 against -22.4, -23.5 against -19.0, -20.1 against -15.6: at cut 2, base +8.1, the largest of the four gaps), in the original
  3.6 to 4.8 more at cut 4 and cut 5's frame 140.2.
- Where, at cut 1 (`tranche-27/regions/sun-hold-3-nat-base-vs-v32.txt` and the `orig` one): the native's diffuse ambient lifts the
  mid-tones (76 % of its +9.0 on the pixels of luma 64 to 159, the medians 139 against 123), the original's +1.0 nearly nothing; at
  cut 5's frame 140.2 the same (81 % of +9.7 on 64 to 159; the original +0.9). With all four out the two modes' cut 1 pictures differ
  by +2.2, 63 % of it in the darks (`sun-hold-3-v99-nat-vs-orig.txt`). The shares are not additive (cut 1 natively: 99 is -68.8
  against -54.2 summed), the tone curve's shoulder (INFERRED, as in tranche 26).
- So the native's diffuse-ambient share is far larger than the original's (OBSERVED), the two bits not removing the same thing (the
  original's lightmapped ambient stays, path 7 counts records, not the draws they feed); that it lies on the characters and the
  foreground is INFERRED from the region grids (the lower and left cells), the dragon's and the King's draws among the zeroed
  records not shown. Why (the GI's
  level, the scene's own ambient values, which of the three SH sources 5B3D0 hands the dragon and the King) is not read.
- The storm, now held: its bases differ by +1.5 or less at cuts 0, 1 and 4, its shares small; but the native's fill lights and glare
  together (bit 3; neither shot alone in this slot) take twice the original's at cut 1 (-21.1 against -10.0, without all -11.6 darker
  natively; natively bit 1 also takes the fill lights' traced bounce, which the original has none of), and cut 3 (+8.9) and cut 5's frame 14.56 (+21.9)
  are bright natively with every share out (+7.5, +19.6): an excess outside the four shares (the sky, the fog or the sun: not
  separated).
- The original's exposure adapts during a hold (YEBIS's fixed step, not frozen) and the native's on the CPU's time: a share is the
  picture 1.5 s after the row in both modes (the first slot: the runs' means of the second shot within 2.9 of the first).

**Step reviews** (step-reviewer, four on step 2 and its answers, one more on the last; no High).
- `4d0f8ce69..2848b4136`'s first: M1, a shape-14 custom glare is out of bit 2's reach: said, and the stored shape in the Reference
  rows (32000010 shows shape 1 in all 570 clock rows of `i141-t27sa-orig`). L1, bits left on when a scene ends: the world's gate puts
  them back to 0 a second after the last Play_Wait call, with a row. L2, two Ins's Play_Wait at once: the follower under a lock. L3
  comments. L4, a forward set's failed copy: counted (path 6).
- `2848b4136..2c4983558`: M1, the bits' scope differs by mode (natively the main view's deferred surfaces, in the original every
  view the hooks reach): said in the comments, and the shares read region by region. L1 comments, L2 the hold's first frame keeps
  its dt (named), L3 the idle reset restored a scene's fresh bits.
- `2c4983558..f83f69e55`: M1, that restore could put stale bits over a runner's fresh 0, or the previous scene's back: the reset now
  takes the follower's lock and reads the tick again inside it. L1, bit 32's reach by pass kind and 5D5C0's upload INFERRED again;
  L2, the first frame INFERRED; L3 gone with the restore.
- `f83f69e55..d3de53a53` (after the runs, which used `f83f69e55`): M1, the follower's rows were logged under the lock: logged after
  it. L1, the journal's order of a bits row and a reset row: a serial taken under the lock in both. L2, the tick check before the
  lock again (no lock while a scene plays). L3, a file whose removal failed read twice: a file is read once by its write time. L4,
  the release frame INFERRED. L5, the order named as the lock's. L6, a scene following another within a second keeps the bits: said
  (Run-AlphaCast writes 0 at each hold's end and waits for its row). None of these changed what the runs measured: no `reset` row and
  no refused file in the four runs.
- `f83f69e55..14250889e`: no High, no Medium. L1, the read file's time kept even after its removal (a new file with the same time
  lost): kept only while the removal failed (`af95bc602`). L2, the read-once rule reaches the native path too: said, both modes. L4,
  the parameter's name: `out`. Left: L3, `Compare-I141HoldVariants.py` does not read the serial nor the reset rows (a hold whose
  return to 0 came from a reset is refused as « shares still off », a refusal, not a misreading; no reset row in any run); L5,
  information (the new state is under the lock or BeginFrame's alone).

## Reviews at the merge of tranche 27 (2026-11-23)

**Final review** (final-reviewer, `main...72b104b59`, the tree built as `build-i141-t27-final`, 195 PASS lines): no High, two
Medium, eleven Low.
- M1, the storm's bit 3 read as « the fill lights » and the bounce caveat dropped: relabelled (the fill lights and glare, bit 3;
  natively bit 1 takes the fill lights' traced bounce too), the caveat and a bit-1 and a bit-2 shot at the storm's cut 1 back in
  « Suggested next step » 3, STATE qualified (equal in the clear state, twice at the storm's cut 1).
- M2, the freeze's comment called READ what its reading leaves INFERRED or OPEN: the frame's tasks under F0FC90's one call and
  EED810's writes moved to INFERRED (the thunk to EED8B0 OPEN), F0FC90's three callers named, the readers that divide by dt named
  OPEN (a bounded run only).
- L1 tranche 26's ranges, L2 the second shot's agreement (the runs' means within 2.9, one run up to 6.8 at an unfrozen cut 5), L3 the
  frames a hold (842 to 1297, `i141-t27rc-nat` at a lower frame rate over the same 22 s), L8 bit 32's kinds-to-GB4 tie INFERRED and
  the two bits not the same removal, L10 the Conclusion, the index, the Uncertainty and the hold protocol's header: fixed in the
  records. L4, the reset row had no tick: a tick in it, and `Compare-I141HoldVariants.py` takes it as bits 0, sorts by serial and
  refuses a hold with a reset inside (its output on the tranche's eight runs unchanged). L5, the freeze's tick and flag relaxed:
  seq_cst. L6, `hold-world-freeze.flag` silent in original-probe mode: refused there. L7, bits applied past the 2048-row cap: the
  follower stops at the cap. L9, the reading's scratch pointer: the listing's evidence copy named, the folder in « Artifacts »;
  149CB0, 14AB20, F0FC90 and 234CB00 not exported to `decompiled/` (left: read in GhidraMCP, their addresses in the readings).
  L11, the final module never run in a bounded Reference run (the tables are `f83f69e55`'s): left, the next run of the follower
  checks a variant and a reset first.

**Step review of those answers** (step-reviewer, `72b104b59..085a90279`): no High, no Medium, five Low. L1, « 72 frozen holds »: 36
(four runs). L2, the reset row's tick taken after the lock: taken under it with the serial. L3, the 31 s safety release's check and
act not atomic and its subtraction able to wrap: the tick read first and read again before the release (narrower, still not one
atomic step: left, a bounded-run tool). L4, the hold protocol's header implied the flag stops the exposure's adaptation: rewritten.
L5, WmTimerHandler and the ConstDt pump are 234CB90's callers in the reading: said so, the pump's F0FC90 call from I-0112,
WmTimerHandler's INFERRED. The reader's docstring names the reset refusal and the order; the dead `rows<2048` guards left.

**The last step review** (`085a90279..271dfea50`): no High, no Medium, three Low, comments: L1 the safety release's remaining window
named in the code (a lost freeze shows as an off row of index -1); L2 the hold protocol's header (« without the flag », the cloth by
+0x26C, INFERRED); L3 what zeroing [RDX+8] reaches (the float and 47415D8, READ; WmTimerHandler's pass INFERRED). No code changed.

**After the merge** (`8afffe0ef`): the launcher's module rebuilt in the main checkout (195 PASS lines), the start check passed
(15:55, restored), the disk rotation 4.4 GB deleted (13 build folders older than 48 h), 454 GB free on C:.

**Integrity review** (integrity-reviewer, `main...085a90279` with this record): no High, one Medium, five Low. M1, « fill lights and
glare equal / as much in both modes in the clear state » against the tables (natively about 4 more at cuts 0 and 2, the largest of
the four gaps at cut 2; the original 3.6 to 4.8 more at cuts 4 and 5; the first slot's fill 3 to 4 more natively): rewritten in the
readings, the Conclusion, « Suggested next step » 3 and STATE. L1, the -4.4 to +6.6 agreement is of frame means (cut 1 mostly black
then, grid cells up to about 18): said. L2, the 36 frozen holds (as the step review's L1). L3, « specular about twice »: 1.8 to 4.6
times at cuts 1, 2, 4 and 5, less at cut 0. L4, tranche 26's cut-0 ranges 1.3 to 3.2. L5, the builds listed in « Artifacts »; the
final tree built (`build-i141-t27-final2`, then the answers' last build). Its checks found no weakened test, no scene-specific case
and no work cut during a measure; every table cell recomputed from the evidence.

## Tranche 28: the native's excess at 32000010's cuts 1 and 5 is the sun's light at the traced hits; the original's instance SH census; the storm's runs differ (2026-11-23)

Opus 5.5, `claude/r-i141-t28` (a fresh R thread). « Suggested next step » 3: why the traced diffuse GI takes -8.9 to -10.7 at cuts
1, 4 and 5 where the original's instance SH takes 1 or less; the storm's cut 1 split into bits 1 and 2; the storm's excess outside the
four shares.

**The tools** (test tooling, a bounded run only; commits `f1b83de2a`, `2b5afc94b`, `b928c4a78`). Two native share bits ORed into the
GI's diagnostic constants as bits 4 to 16 (so the same passes: the GI's rays, the specular rays on NRD's frames, the cache's update;
the probes, the water and the specular on frames NRD does not denoise keep their hits' light, and read cache cells built under
the bits): 128 takes the sets' lights 1 to 3 (the
cutscene's fill lights, D-0279) out of the traced hits' light, 256 light 0 (the sun) out of it, the pixels' own direct light kept in
both (`DirLights`, `RT_REFLECT_HIT_NO_FILL`, `RT_REFLECT_HIT_NO_SUN`; under 256 the hit's sun ray is not traced). A Reference run's bit
512 changes no picture: while it is set, 04c's `ReferenceShareCensus` logs once per (model, pass kind 0, 3 or 4) the instance record's
+0x80..+0xBF as 5B3D0 wrote them and the source the same inputs choose when read again after the call (0 the global override
[[GXRS+0x4DD0]+0x13E8], 1 the animation block q = [group+0x68]'s rows q+0x130.. when [q+0x170] is set, 2 the light-set records of
[group+0x100]+0xF8 and +0x100 blended; READ in the pilot's decompile of 14005B3D0, GhidraMCP); `Read-I141ShCensus.py` reads it. Run-AlphaCast
checks the bits by mode as the module parses them (native 0 to 511; Reference combinations of 1, 2, 32, 64 and 512).

**The slot** (OBSERVED; build `build-i141-t28b`, commit `2b5afc94b`, 195 PASS lines; `tranche-28/run-t28.ps1`, logs `runs-t28-sun.log`,
`runs-t28-storm.log`; 16:29 to 17:01 Paris, the ASUS in a 1920x1080 window, offline, no measure, each run exit 0 with no fault (but the
storm's original run wrote a crash report at its close, after its last hold: 0xC0000005 at DarkSoulsIII+0xF14C52, DS3's own exit race
of I-0124, INFERRED from its place), the save restored by hash; `test-tools.flag` and `test-revive.txt` beside the build for the storm runs only, removed after;
`hold-world-freeze.flag` in all four): `i141-t28sc-nat` (variants 128, 256, 384, 32), `i141-t28sc-orig` (512, 32), `i141-t28rc-nat`
(1, 2, 4, 256), `i141-t28rc-orig` (1, 2, 512), each variant followed by the base. 36 of 36 holds kept by the reader, every base range
0.0 to 0.9. Tables: `tranche-28/sun-nat-variants.tsv`, `sun-orig-variants.tsv`, `storm-nat-variants.tsv`, `storm-orig-variants.tsv`;
the censuses `sun-orig-census.txt`, `storm-orig-census.txt`.

| Clear hold (cut, frame) | Native base | 128 (fill at hits) | 256 (sun at hits) | 384 (both) | 32 (diffuse GI) | Native without the sun's bounce | Original base | Original 32 (instance SH) |
|---|---|---|---|---|---|---|---|---|
| 0 (0, 69.8) | 107.4 | -0.6 | -0.5 | -1.1 | -0.7 | 106.9 | 106.0 | -1.5 |
| 1 (0, 209.4) | 95.6 | -1.0 | -1.3 | -2.3 | -1.4 | 94.2 | 95.3 | -1.9 |
| 2 (0, 349) | 97.9 | -0.9 | -1.0 | -2.0 | -1.2 | 96.8 | 94.2 | -1.3 |
| 3 (1, 128.6) | 137.8 | -0.7 | -15.3 | -16.0 | -8.4 | 122.0 | 121.8 | -1.0 |
| 4 (2, 68.2) | 70.0 | -3.9 | -0.7 | -5.1 | -3.0 | 69.0 | 62.2 | -0.3 |
| 5 (3, 51.8) | 163.2 | 0.0 | -4.3 | -4.3 | -3.5 | 158.8 | 148.6 | 0.0 |
| 6 (4, 81.4) | 173.2 | 0.0 | -13.4 | -13.4 | -9.6 | 159.7 | 154.7 | 0.0 |
| 7 (4, 221) | 194.4 | 0.0 | -14.9 | -14.7 | -11.4 | 179.2 | 166.7 | -0.2 |
| 8 (5, 0.6) | 133.9 | 0.0 | -2.1 | -2.1 | -0.8 | 131.7 | 150.4 | -0.3 |
| 9 (5, 140.2) | 168.1 | -0.3 | -16.8 | -16.9 | -9.2 | 151.0 | 147.3 | -0.9 |

Each share is the variant minus its neighbouring bases' mean, one run a mode (the world frozen: the bases repeat to 0.9 at most).

**The split** (OBSERVED; the final review's H1: bit 256 takes the sun from every traced hit, so the first slot did not separate the
diffuse GI's part; build `build-i141-t28-final`, the runs' code with comments changed; `tranche-28/runs-t28-split.log`,
`sun-nat-split.tsv`; 17:32 to 17:40 Paris, the same protocol, exit 0 with no fault, the save restored by hash): `i141-t28sd-nat`,
variants 288 (32 and 256), 320 (64 and 256) and 64 (the specular ambient), each followed by the base; 10 of 10 holds, base ranges 0.1
to 0.8; its log holds the runner's output alone (the start from the clock read before it, the first hold shot 17:36:35). Every native
run of the tranche at the RT level without DLSS (so no PT and no Ray Reconstruction, D-0265), one bounce, ReSTIR not run, every GI
frame through NRD (the runs' `native_rt_level` and `native_rt_gi` rows: 21214 of 21214 in `i141-t28sd-nat`).

| Clear hold (cut, frame) | Base | 288 (diffuse GI, sun at every hit) | 320 (specular ambient, sun at every hit) | 64 (specular ambient) |
|---|---|---|---|---|
| 0 (0, 69.8) | 106.3 | -1.1 | -1.2 | -0.8 |
| 1 (0, 209.4) | 98.2 | -2.0 | -2.2 | -1.4 |
| 2 (0, 349) | 97.6 | -1.8 | -2.1 | -1.3 |
| 3 (1, 128.6) | 137.7 | -15.7 | -15.3 | -6.7 |
| 4 (2, 68.2) | 70.0 | -3.6 | -8.2 | -7.7 |
| 5 (3, 51.8) | 159.0 | -4.2 | -4.1 | -0.6 |
| 6 (4, 81.4) | 154.0 | -17.4 | -17.3 | -2.8 |
| 7 (4, 221) | 191.2 | -16.0 | -15.8 | -2.4 |
| 8 (5, 0.6) | 147.6 | -3.8 | -3.8 | -1.9 |
| 9 (5, 140.2) | 165.2 | -17.1 | -17.0 | -6.9 |

**Reading.**
- **At cuts 1 and 5 the native's diffuse and specular excess at 32000010 in the clear state is the sun's light at the traced hits**
  (OBSERVED, two runs; cuts 2 to 4 keep a gap without it, below): at cut 1 the whole diffuse GI takes -8.4 and the whole specular ambient -6.7, the sun at every hit -15.3; with the sun out of
  the hits, the rest of the diffuse GI (288) adds 0.4 and the rest of the specular ambient (320) nothing, so both shares are almost
  wholly the sun's light, about half each (cut 5's frame 140.2: -9.2 and -6.9 against -16.8; cut 4: -9.6 to -11.4 and -2.4 to -2.8,
  the diffuse the most, across two runs whose cut-4 bases differ by up to 19). Without it the native meets the original at cut 1 (122.0 against 121.8) and cut 5's frame 140.2 (151.0
  against 147.3); the diffuse GI alone out leaves cut 1 at 128.9. Tranche 27's « specular 1.8 to 4.6 times the original's » is mostly that
  sun's light reflected at cuts 1, 4 and 5, not at cut 2: there the specular ambient takes -7.7 and -8.2 with the sun at the hits
  out too, so about 7.5 of it is not the sun's (not read). The fill lights' traced light takes 1 or less (3.9 at cut 2). The triptychs `sun-hold-3-orig-nat-nosunbounce.jpg` and `sun-hold-9-orig-nat-nosunbounce.jpg` (original, native,
  native without the sun at the hits): the dragon's wing and the King lit grey-lavender natively, as dark and blue as the original's
  without it, the scales' sheen gone too.
- **The original's instance SH** (the census, OBSERVED): every census row takes source 1, the animation block's own rows; the map
  pieces' rows are 0 (their ambient is the lightmap's, t26) with ShLightMask 1, the characters' a small bluish ambient that varies by
  cut: c5030 mean w 0.014 at cut 0 and 0.037 at cuts 1 and 2 (rgb 0.028, 0.036, 0.046; the six axis normals 0 to 0.084), c5010 0.062 at
  cut 4 and 0.049 at cut 5 (axes 0 to 0.13), the same in the storm's runs. Those rows are written each update from the `.btpb` probe's
  SH or from the light-set rows of key q+0x1A0, blended by the clock (READ, `t27-original-ambient-reading.md` §2); which one feeds these
  characters, and whether it holds a baked bounce, OPEN. The original's characters receive no traced light of the sun, only that
  ambient and their IBL; its map pieces' lightmaps carry their own bounce (the white temple at cut 5 darkens natively without the sun at
  the hits, third picture of `sun-hold-9`, INFERRED from the picture). The native's chain composes its GI as its pixels' light (albedo
  (1 - F0) L, L the cosine-weighted mean of the hits' radiance, the hits lit by the same set rows as the pixels: READ, rt_reflect.hlsl
  `gi`, `DirLights`): the excess is a light the original's characters do not receive, not an error of the chain's arithmetic.
- **The characters' sun mask** (OPEN; the final review's H2): the census reads ShLightMask 0, 0, 0, 0 on c5030, c5010 and the player's
  parts, 1 on the map pieces; the objects' own values vary by instance and by run (the clear run: 32 rows 0.998, 23 at 0, others 0.023
  to 0.69; the storm's: 32 at 0.04, 24 at 0.998, others 0 to 0.70; which input of 5B3D0 sets it not read here). ShLightMask.x is GB1.z, the deferred sun's mask (READ, t27 reading §1; the native
  writes it the same, `12_scene_collect.inc`), so neither renderer lights these characters by the sun directly; the native's hits take
  every instance's mask as 1 (`rt_reflect.hlsl`'s header, `rt_material.hlsli`), so a ray landing on a character finds it sunlit. What
  the mask means for the characters' indirect light, whether the hits must honour it, and the characters' stencil bit 0x10 (the ambient
  mask t32.y's) not read.
- **Its level.** Scaling the GI or the reflections down to the original's would be a lighting cheat (D-0262); D-0249 accepts a true
  lighting where the original differs. The levers that stay: the cutscenes' sun in their sets (workstream 12), and, if the mask's
  reading says so, the hits honouring the instance's sun mask. Put to Thomas with the triptychs (2026-11-23 17:02, recommending to keep
  the native as it is, the attribution then the diffuse GI's alone); Thomas asleep, the coordinator applies the recommendation until his
  answer (17:07); corrected to him after the split (the diffuse GI and the reflections alike, the mask open). Thomas (18:01, relayed
  by the coordinator): « Oui ils sont trop clairs »: the native's level not kept; the next R thread reads the mask at the hits, then the
  cutscenes' sun (workstream 12) if needed, with no lighting cheat; his aim (18:03): the original's mood with the rays, no raster.
- Left at the clear state after the sun at the hits: cut 4 +5.0 and +12.5, cut 3 +10.2, cut 2 +6.8 (+2.5 with the fill lights' traced
  light out too), cut 5's frame 0.6 -18.7 (the first slot's run). The native's bases move between runs at cuts 3 to 5 although each
  hold is frozen: hold 6 164.7, 173.2 and 154.0 (`i141-t27sc-nat`, `-t28sc-nat`, `-t28sd-nat`), hold 7 174.6, 194.4 and 191.2, hold 8
  146.5, 133.9 and 147.6, hold 5 163.2 and 159.0; cut 1's 137.8 and 137.7 and cut 5's frame 140.2 168.1 and 165.2 hold. The original's
  run pair 147.7 and 154.7, 165.9 and 166.7, 150.5 and 150.4. Those cuts' gaps carry a run-to-run spread the frozen hold does not
  remove (its cause not read).
- **The storm** (OBSERVED, one run a mode): at cut 1 the native's fill lights alone take -16.7 (with their traced light) and the
  original's -14.4, the glare -1.3 and -1.2: tranche 27's « twice the original's » for bit 3 (-21.1 against -10.0) is not reproduced
  when the bits are shot apart. The GI's sky misses take -2.2 to -2.3 at cuts 0 and 1, the sun at the hits 0.4 or less everywhere. At
  cut 4's frame 221 the bases differ more between runs of one mode than between the modes (the native 139.8 in `i141-t27rc-nat`
  against 120.6 here, the original 138.3 against 110.9); cut 3's base gap +8.9 in tranche 27 is -0.2 here (102.8 against 103.0); at
  cut 5's frame 14.56 the native stays brighter in both pairs (+21.9 there, +13.4 here; the native 138.9 and 138.6, the original 117.0
  and 125.2), beyond each mode's own spread. The world frozen at the hold does not freeze the same weather in every run (INFERRED: the
  storm runs on the world's clock up to the hold): single runs a mode settle no storm excess.

**Step reviews** (step-reviewer, three; no High).
- `0d249e331..f1b83de2a`: one Medium, six Low. M1, the runner accepted bits the module refuses (0 to 1023 in both modes): checked by
  mode as the module parses them. L1 a census row could carry the next census's epoch: the epoch read under the lock printed. L2 the
  name read on every call after the caps: an atomic full flag first. L3 a failed read reported as a source: source -1; the reader counts
  rows it cannot parse and lists rows without their SH. L4 the rows de-duplicate by model name while the SH is the draw group's: said
  in the comment. L5 the new statics outside the thread-affinity list: guarded by the lock, the hook outside the checked functions
  (left). L6 the sun's shadow ray traced under bit 256: skipped.
- `f1b83de2a..2b5afc94b`: no Medium, four Low. L1 the pass kind still read once full: the full check first. L2 a non-finite SH zeroed
  (data lost): the SH logged as written, the reader marks it. L3 the source is read again after the call, not the original's own read:
  said in the comment and here. L4 the census's per-draw cost while open (no picture changed, the frame time may): said. These came
  after the runs' build (`b928c4a78`); the runs' census rows held no non-finite value, no source -1 and fewer than 800 rows a census
  (126 at most), so neither change alters what the runs logged.
- `2b5afc94b..b928c4a78`: no High, no Medium, three Low. L1, the caps' flags set by the first call that finds the cap reached, so
  each thread in flight still reads one kind and one name after it: said in the comment (a test tool under bit 512, one read a thread).
  L2, « -1 when a pointer 5B3D0 dereferences reads null » claimed more than the code (a null s under source 2 and a failed key, blend
  or scale read leave 0, unmarked): the comment narrowed to what the code does. L3, a census cut at a cap not shown: the reader lists
  every opened census, rows or none, and says when one reached the cap of 800 (none did: 126 at most). Comments and the reader only;
  the final tree built before them (`build-i141-t28-final`, 195 PASS lines) compiles the same code.

## Reviews at the merge of tranche 28 (2026-11-23)

**Final review** (final-reviewer, `main...4e5c5bf40`, the code built as `build-i141-t28-final`, 195 PASS lines): two High, two Medium,
seven Low.
- H1, bit 256's result called the diffuse GI's while it takes the sun from every traced hit (every frame an NRD frame: 9214 of 9214),
  the diffuse GI alone out leaving cut 1 at 128.9: the split run (`i141-t28sd-nat`, variants 288, 320, 64) shot in a short slot
  (17:32 to 17:40), the reading rewritten on it (the sun's light at the hits, about half diffuse, half reflected at cut 1), the
  Conclusion, the Uncertainty, « Suggested next step » 3, STATE, ROADMAP and the index with it; the message to Thomas corrected.
- H2, the characters' ShLightMask 0 left OPEN while the repository reads it (GB1.z, the deferred sun's mask) and the native's hits take
  every mask as 1: the mask's reading and the hits' difference written in, the claim « not a fault of the GI's chain » narrowed to its
  arithmetic, the recommendation made provisional, the mask's meaning for indirect light, the characters' stencil and a variant
  honouring it at the hits in « Suggested next step » 3.
- M1, « no bounce at all, only that constant »: the two writers of the animation block's rows named (the `.btpb` probe or key
  q+0x1A0's set), which one feeds these characters OPEN, « constant » dropped. M2, the storm's excess « within the spread » while cut 5's
  frame 14.56 stays brighter in both pairs: said.
- L1 the cut-3 sign and its comparison (tranche 27's base gap +8.9, -0.2 here), L2 the storm's original run's crash report at its close
  (named, DS3's exit race INFERRED), L3 « every census row », L4 the cache cells built under the bits, L5 the follower's header comment
  (bits 32 to 512 added), L6 the reader's run-cap hint under `--epoch`: fixed. L7, STATE's « Last finished » before the merge: completed
  at the merge.

**Integrity review** (integrity-reviewer, `main...dba46ddcf` with this record): no High, two Medium, three Low. M1, the specular
excess generalised to cut 2 where most of it is not the sun's (64 -7.7, 320 -8.2): the reading, the title, the Conclusion, « Suggested
next step » 3 and STATE narrowed to cuts 1 and 5. M2, « 1 on the map pieces and objects » while the objects' masks range 0 to 0.70
and differ between the runs: their values written in. L1 cut 4's split across two runs with different bases: said. L2 the measured
configuration (the RT level, NRD only): written in the split and the Uncertainty. L3 the split log without the runner's time
markers (Run-AlphaCast launched directly): said. Its checks found no weakened test, no scene-specific case and no work cut during a
measure; every table cell recomputed, five stills' YAVG recomputed with ffmpeg.

**After the merge** (`2b76eec52`): the launcher's module rebuilt in the main checkout (195 PASS lines), the start check passed
(18:00 to 18:02, restored), the disk rotation 0.6 GB deleted (2 build folders older than 48 h, raw frames older than 7 days), 452 GB
free on C:.

**Step review of those answers** (step-reviewer, `a9c2a5c47..dba46ddcf`, the reader and a comment): no High, one Medium, five Low.
M1, the follower's header named bits 32 and 64 for the native run alone and no bit's mode: each bit's native and Reference effect, the
modes and the whole file refused for a bit outside its mode's set written in. L1 a census cut by the run cap during it, L2 « truncated »
at exactly 800 (« possibly »), L5 an unknown `--epoch`: the reader says them. L3, the rows that do not parse not counted toward the
run cap: said in the reader. L4, an unparsed opener row shifting the cut and frame shown: left (none in the runs: every census's
opener row found).

## Uncertainty / unresolved questions

- Tranche 24: the Sun and Fog fade unit-tested only (no area fade compared native against original in a run); the fog position
  and area the current's at once, and the earlier end a fade's earlier layers, not the values on screen at a restart (INFERRED,
  the original's writers during a fade not read); that the per-value path ran in 32000010's 3 s fade is INFERRED (one
  `native_gparam_area_fade` row in each native t24b and t24c run, no fault, nothing logging OwnSun's fade itself; no hold inside it).
  FadedCopy pairs parameters by position (the final review's L1; that no map's LightSet but m21's and m22's differs INFERRED, not
  scanned); the test's bools are equal at both ends (L2). Step 4 rests on one run a mode and
  on the original's partial last window; its whole later windows were not tranche 18's state. The native's pass rows sample every
  30th collect. Step 3: whole frame below the title bar, three pairs, no crop, the effects' phase unmatched.
- Tranche 20: one run, the native's cut 2 only; the forward rows' models not named (INFERRED the sky); other views of forward rows
  alone (other cutscenes) not sought; the effects' share of cut 2 not separated. A main view with effects or water and no model
  draw is still dropped (a clear). Such frames now fall under the rays' fatal rule (2 s and 30 unserved frames), answered for
  41000060 alone (its walk kept 29 map instances in cut 2). The keep is decided after the post chain's plans are built (older).
- Tranche 22: the margin rows from one native run, on the whole-filter path alone (the gathered path's rows READ only); that a map
  piece's box bounds its geometry (so no pixel) INFERRED, most at stake for the boxes a few centimetres out; the 8 models' original
  rows all in the arrival's window and written once per key (tranche 24's per-window rows: seven still drawn in a partial window);
  Yorshka's +1 not pursued (no same-moment pairing in play). The model-name walk is a third copy of `refuse_disp`'s (a cleanup).
- Tranche 18: the 8 models without a G-buffer row take none in either mode, drawn natively outside it (tranche 22); the five m32
  pieces are the native's cull margin (tranche 19), wholly outside the view (tranche 22, OBSERVED). One run a mode, compared over QPC minute windows (D-0299's strict form not met). The
  witness's run showed one row (the first call, before the map's nodes); the later rows (`hooked`, `why`, the first clearing with
  nodes, the periodic count, `qpc_ms`) and the original-probe refusal were built with their tests (`build-i141-t18f`), not run. The hook makes a
  state the game never makes (`CA050` alone sees `+0x98` clear; its camera history and counter `+0x160` not updated; `+0x15C`
  left at 1): that the run differs from a plain one by the occlusion system alone is INFERRED. The set-3 pieces' pixels and the
  cost not measured.

- Tranche 17: two runs a mode, aligned on the arrival's event, not on a game-clock value read in both (D-0299's strict form); the
  original's pixels' keys INFERRED from its records (no G-buffer of the original read); no blend exercised (every weight 0); the
  12 extra groups of `004190` and `004210`, not read (culling or another path in the original; the 143 models: tranche 18's); the sets' values as the composite reads them on the frame not
  compared; the last code (the G-buffer list's passes 5 to 7) built with its tests (`build-i141-t17-final2`), not run live; the
  two runs' windows start 23 s (`i141-t17b`) and 43 s (`i141-t17a`) apart after the arrival.

- Tranche 16: Yorshka's sweeps about +1 over the original with no lift but on the last one, the native's own repeat not measured, every comparison one
  run a leg on tour stills (not D-0299's same moment); the Road of Sacrifices' share of the local lights not separated (a run with
  `rt-local-x2.flag`); the bank's lookups other than Sun and Fog not observed for unbound answers; `OwnSun` and `OwnFog` read
  the layer, the bound file and `File()` under separate locks, so a binding landing between them can answer the new file without its
  rows (older than this tranche, the final review's L4); the MapOffset parse and turn and SelectVariant are dead code in the module,
  left for a cleanup.
- The ceremony's smoke: its effects end as the original's since tranche 13d (the stop table); cause (a), the 14.4 s burst, is the
  original's ~80 ms pause in the effect draw (« Tranche 14 »): whether the pause holds the whole frame and why, not read;
  `renderdoc-capture.flag` does not start the original (cause not
  read; the flag kept, the runner's `-Original` without `-Dump` not to be used). The other readers of the SFX manager's `+0x68`
  not enumerated (tranche 13d's review, M2).
- The ceremony's walls beside the smoke brighter natively before tranche 13d (« The witnesses of tranches 6 and 7 »), not
  measured since on the scene's clock; 32000010's last cut (« Tranche 24 », step 3: three pairs, no darkening at its end). The haze in play at Archdragon Peak: the effects the same in
  both modes and adding the same light (« Tranche 15 »); under them the castle and the player darker and the castle flatter
  natively, each key's fog block and written light words alike in both modes, the castle's keys the same where both draw a piece (« Tranche 17 »),
  the light D-0262 removes (the baked ambient, the sets' fill lights, not separated) the likeliest difference (INFERRED),
  workstream 12's (the castle's fog is the game's own composite, READ).
  `reference-no-ffx-draw.flag`: that the run differs from a plain one by the effects' pictures alone is INFERRED (24E790's skipped
  body not read for side effects).
- Tranche 7: the original blends value by value (READ in tranche 23), the native's Sun and Fog too since tranche 24 (a fade restarted during another starts from the earlier layers, not the blended values); the time on the wall clock,
  not the game's step delta; the unbinding's time read from the controller before its step (INFERRED for a step that fetches its
  caps); the rotation of a fading set is the current layers' (the final review's L10).
- Tranche 23: each value's class taken from the file's type, not the definition's (INFERRED where they differ); LightScattering's two
  register-held fade flags read as 1 (INFERRED); that each bank's record (ShadowParam's 18C590, the YEBIS blocks) is built from those
  live fields as `Banks` builds it from the file's values (INFERRED: the records' builders were read in earlier tranches, not
  their reads of the live fields during a fade).
- Tranche 6: the strength `+0x2C0` of the world's instance lighting (32A80, the engine record's 1 natively where the original holds the
  set's LightMask Influence) matters only where an animation owns the model (the final review's L5); 31A20's own use of its two
  answers not read.
- Tranches 3 to 5: that the map holder's `+0xD4` is the act's key area (seen in one scene); the light sets' bank's withdrawal
  (`314D0`) and three removers not read; the act's removers' sixth word not read; D-0282 without a visible effect on its A/B.
- Tranches 8 and 9: the library post object's `+0x2271` kept for the session natively (the original's is made again at a load,
  back to 1: INFERRED, not observed); the original's shaft fading in over a cut's first 150 frames or so (its kept record's words 13 to
  16), not served natively (not read); the shafts' luminance gate on the frame's `+0xDC` where the library keeps `S+0x6A4` latched while
  `+0x244` is not positive (INFERRED equal where it is positive); the witnesses' row caps (the focus traces partial); no unit test of the
  two shaft gates or of the camera's request (they live in the module's collect and prepare, under the game's state; the witness is
  the run).
- B is provisional by Thomas's word (until the light sources are set by us).
- Why the original's horizontal shadow sun leaves its pictures as they are (the contract's H2): not read.

- Tranche 25: one run a variant on `build-i141-t25a`, the originals reused from `i138-t8h3` and `i141-t24b` (other builds); the
  clear state's repeat spread mixes three builds (`build-t8g-7`, `-t8h-4`, `-t8h-5`); no share of 32000010's excess attributed.
- Tranche 26: two runs a state on one build, native only; the originals still `i138-t8h3` and `i141-t24b`'s; the shares of cuts 3 to 5
  and of the storm not separated from the scene's motion inside a hold; the sky's settling checked at 1.5 and about 4 s only; the
  probes' own sky misses not removed (the bits never reach the probe pass; its hits read the cache built under them); the
  module's parser and leftover fixes not in the runs' build (the original's glare, cuts 3 to 5 and the storm measured in tranche 27); at cut 0 the GI's and the cache's shares within the base's own motion.
- Tranche 27: one run a mode a state in the second slot (two in the first), one build each; each share shot once, 1.5 s after its row,
  both modes' exposure still adapting; the original's bit 32 reaches the instance SH of the pass kinds 0, 3 and 4 only (not the map
  pieces' lightmapped ambient nor the decals' GB4), its bit 64 every view, the native's bits the main view's deferred surfaces only;
  that the native's diffuse-ambient excess lies on the characters INFERRED from the region grids; why the traced GI is so much
  stronger there not read; the storm's excess at cut 3 and cut 5's frame 14.56 outside the four shares, not separated; the fixed-step
  tasks (the FFX wind, YEBIS) not frozen in a hold (INFERRED from I-0112's sites); the probes' own sky misses still not removed.
- Tranche 28: the attribution measured at the RT level with NRD only (no PT, no RR, no ReSTIR); one run a mode a state in the first slot, one native run for the split (two builds of the same code); the census's
  source read again after 5B3D0's call, one row per (model, kind) where the SH is the draw group's; which writer fills the characters'
  rows (the probe or key q+0x1A0's set) and whether it holds a baked bounce not read; the characters' ShLightMask 0 against the hits'
  mask 1, its meaning for indirect light and the characters' stencil not read; that the original bakes the sun's bounce into the
  lightmaps INFERRED from one native picture; the clear gaps left at cuts 2 to 5 against the native's own run-to-run spread there (its
  cause not read); the storm's weather not aligned between runs (INFERRED), so no storm excess settled; the probes' own sky misses
  still not removed.

## Artifacts produced or modified

- Tranche 24: `native/world/light_producers.{h,cpp}` (`FadedCopy`, `OwnSun`, `OwnFog` on a fade), `native/frame/glow_bank_test.cpp`,
  `native/frame/game_bootstrap/04c_scene_witnesses.inc` and `12_scene_collect.inc` (the pass windows), `Compare-I141PassWindows.py`,
  `Compare-I141HoldSpread.py`; runs `i141-t24a-c32000000-orig`, `-nat`, `i141-t24b-c32000010-nat`, `-orig`, `i141-t24c-c32000010-nat`,
  `-orig` (`evidence/I-0066/artifacts/`); `run-t24.ps1`, `runs-t24.log`, `holds/` (`evidence/I-0141/artifacts/tranche-24/`);
  builds `build-i141-t24a`, `build-i141-t24b` (`evidence/I-0141/artifacts/`).
- Tranche 22: `native/frame/game_bootstrap/05_selected_scene.inc` (`NoteNativeMarginPiece`), `09_model_admission.inc`,
  `12_scene_collect.inc` (the margin's sign and forward axis); runs `i141-t22a-c32000000-nat-ds3lod`, `i141-t22b-c32000000-nat-ds3lod`
  (`evidence/I-0066/artifacts/`); `run-t22a.ps1`, `run-t22b.ps1`, `runs-t22a.log`, `runs-t22b.log`, `margin.py`, `margin-t22b.txt`
  `margin-stats.py`, `margin-stats-t22b.txt` (`evidence/I-0141/artifacts/tranche-22/`); builds `build-i141-t22a`, `build-i141-t22b`,
  `build-i141-t22-final` (the final tree `53a5d2fc2`, its tests passed; `evidence/I-0141/`).
- Tranche 21: `evidence/I-0141/t21-margin/` (the differences and pairs); records only.
- Tranche 20: `native/frame/game_bootstrap/13b_prepare_model_frame.inc`, `native/renderer/scene_pass_frame.cpp`,
  `scripts/thread-affinity-allow.json`.
- Tranche 18: `native/frame/game_bootstrap/04c_scene_witnesses.inc` (`ReferenceOcclusionCellsVisible`), `22_setup.inc`
  (`reference-no-occlusion.flag`: hooked, refused in original-probe mode, ignored natively),
  `investigations/I-0066-independent-native-startup/Run-StartupTrace.ps1` (the flag, allowed and pre-cleaned); runs
  `i141-t18a-c32000000-nat-ds3lod` and `i141-t18b-c32000000-orig-noocc` (`evidence/I-0066/artifacts/`); `run-t18a.ps1`,
  `run-t18b.ps1`, `runs-t18a.log`, `runs-t18b.log`, `models.py`, `models.txt`, `models-b.py`, `models-b.txt`, `occlusion-reading.md`
  (`evidence/I-0141/artifacts/tranche-18/`); builds `build-main-d3d5104c9` (main's module copied, 886 MB), `build-i141-t18b`,
  `build-i141-t18c`, `build-i141-t18d` (main merged in), `build-i141-t18e` and `build-i141-t18f` (the reviews' fixes) (`evidence/I-0141/artifacts/`);
  `.project/ROADMAP.md` (workstream 15), `.project/OPTIMISATION-SURVEY.md`, `decisions/LOG.md` and `IN-FORCE.md` (D-0304).
- Tranche 17: `native/frame/game_bootstrap/04c_scene_witnesses.inc` (`reference_pixel_key`), `05_selected_scene.inc`
  (`native_pixel_key`, `NativeSceneLightKeys`, `NativeLightSetHolds`), `12_scene_collect.inc` (`native_pixel_key_gbuffer`),
  `02_frame_lifecycle.inc` (`native_pixel_key_list`, the 1200-frame cadence), `01_state.inc`;
  `investigations/I-0066-independent-native-startup/Run-StartupTrace.ps1` (`pixel-keys`, allowed and pre-cleaned);
  `Compare-I141PixelKeys.py`; runs `i141-t17a-c32000000-{nat,orig}` and `i141-t17b-c32000000-{nat,orig}`
  (`evidence/I-0066/artifacts/`); `run-t17a.ps1`, `run-t17b.ps1`, `runs-t17a.log`, `runs-t17b.log`, `pixel-keys.txt` (the first
  script's), `pixel-keys-t17a-arrival.txt`, `pixel-keys-t17b.txt`, `tcomp.py`, `tcomp.txt` (`evidence/I-0141/artifacts/tranche-17/`);
  builds in `evidence/I-0141/artifacts/`, all `-Cache` but the last: `build-i141-t17a` (`e067909ba`), `-t17b` (failed: RenderWidth
  is private), `-t17c` (`5105b1a75`, `i141-t17a`'s), `-t17d` to `-t17f` (the step reviews' fixes), `-t17g` (`7b7bc9430`,
  `i141-t17b`'s), `build-i141-t17-final` (complete with its unit tests, a tree before `7b7bc9430`), `build-i141-t17-final2` (the
  final tree, complete with its unit tests); `pixel-keys-arrival.txt` (an intermediate output of the arrival script).
- Tranche 16: `native/world/light_producers.{h,cpp}` (`Banks::UnboundAnswers`), `12_scene_collect.inc` (`native_bank_unbound_answer`),
  `native/frame/glow_bank_test.cpp`; runs `i141-t16a-z16-road-of-sacrifices-nat`, `i141-t16a-c30000010-{nat,orig}`
  (`evidence/I-0066/artifacts/`); `run-t16a.ps1`, `runs-t16a.log`, `player-crops.sh`, `player-crops.txt`, `pair-z16-stay.png`
  (`evidence/I-0141/artifacts/tranche-16/`); builds `build-i141-t16a` and `build-i141-t16b` (`-Cache`) in `evidence/I-0141/artifacts/`.
- Tranche 15: `native/frame/game_bootstrap/21_effects.inc` (`ReferenceSceneDrawSkipped`), `22_setup.inc` (its hook and refusals,
  `reference-no-ffx-draw.flag`); `investigations/I-0066-independent-native-startup/Run-StartupTrace.ps1` (the flag allowed and
  pre-cleaned); `Compare-I141InPlay.py`; runs `i141-t15a-c32000000-{nat,orig}`, `i141-t15b-…`, `i141-t15c-c32000000-{orig,orig-noffx}`,
  `i141-t15d-c40000040-nat` (`evidence/I-0066/artifacts/`); wrappers `run-t15a.ps1` to `run-t15d.ps1`, `run-t15cd.ps1`, logs,
  `inplay-60s.txt`, `blocks-compare.txt`, `crops.sh`, `pair-sweep-50/55/60.png`, `crop-castle-58.png`, `quad-99.png`
  (`evidence/I-0141/artifacts/tranche-15/`); builds `build-t15a` and `build-t15b` (`-Cache`, `-NoControls`) in
  `evidence/I-0141/artifacts/` (every `reference-no-ffx-draw.flag` run is `build-t15b`'s, `861fc2cf2`'s native tree: the comments
  of `a81015dfe` and the Native-mode row of the final review's L4 never ran live); `build-i141-t15-final` (`7d5c24ea1`'s native
  tree, the final one, complete with its tests, not run live).
- Tranches 12 and 13: `native/frame/game_bootstrap/21_effects.inc` (the entity lives, the state-3, machine and external-table
  observers under `ffx-entity-life.flag`; the native SFX manager's stop table), `22_setup.inc`, `01_state.inc`;
  `Compare-I141Lives.py`, `Compare-I141State3.py`, `Compare-I141Machine.py`, `Run-I141T12.ps1`, `Run-I141SmokePix.ps1`;
- Tranche 14: in `21_effects.inc` the witnesses `ffx_newborn_draw`, `ffx_newborn_update`, `ffx_draw_gate` and `ffx_frame_ring`, the
  1ADDE80 hook in `22_setup.inc`, the gate's call in `04b_pass_witnesses.inc`; in `07_scene_lighting.inc` `fog_block_latest` and
  `fog_array`; three entries in `scripts/thread-affinity-allow.json`; `Compare-I141Newborn.py`; the burst runs `i141-t14-n1` to
  `-n6`, the haze runs `i141-t14{h,k,m,n}-*` (wrappers `run-t14h.ps1` to `run-t14n.ps1` and sheets `haze/` in
  `evidence/I-0141/artifacts/tranche-14/`); builds `build-t14b` to `build-t14o` in `evidence/I-0141/artifacts/` (the haze runs on
  `build-t14f` to `-t14h`; `-t14i` a cache build without the control suites; `-t14j` to `-t14o` built with their tests, not run);
  `build-t14d.log` and the folder `build-t14d` were reused by tranche 14 on 2026-11-20 13:17: tranche 9's witness build log of that
  name is lost (as tranche 13's `build-t13b.log`);
  `investigations/I-0066-independent-native-startup/Run-StartupTrace.ps1` (the flag `ffx-entity-life`, allowed and pre-cleaned);
  `investigations/I-0066-independent-native-startup/GX-DEPENDENCIES.md`, `I-0068-static-yebis-ffx-lights-spec/EFFECTS-PHASE1-NATIVE.md`;
  evidence: runs `i141-t12-life-*`, `i141-t13-{s3,m1,x1,x2}-c32000000-{nat,orig}` (`evidence/I-0066/artifacts/`), builds
  `build-t13c` to `build-t13j` and `build-t13-final` (the merge tree with its tests), logs `runs-t12.log`, `runs-t13*.log`,
  `t13-smoke-sheet/` (`evidence/I-0141/artifacts/`).

- The ceremony's smoke (2026-11-16/17): `native/frame/game_bootstrap/21_effects.inc` (F9's blocks, visibility inputs, entity rows,
  texture census, the 24ED90 and 2997E0 hooks), `04b_pass_witnesses.inc` (the camera and culler rows), `22_setup.inc` (the hooks,
  `renderdoc-capture.flag`), `native/frame/effect_draw.{h,cpp}` (`ffx_vs_image`); `Run-I141SmokePix.ps1`,
  `Compare-I141FxBlocks.py`; `investigations/I-0066-independent-native-startup/Run-StartupTrace.ps1`, `Run-AlphaCast.ps1`
  (`-SweepKeyAt`). Runs `i141-smoke*-c32000000-{nat,orig}` (`evidence/I-0066/artifacts/`), their logs
  `evidence/I-0141/artifacts/runs-smoke*.log`, builds `build-vs1`, `build-ps3` to `build-ps16`, `build-merge-a0d38c026`.

- Tranches 8 and 9: `native/frame/game_bootstrap/06_pass_programmes.inc` (the shafts' gates, their witnesses), `07_scene_lighting.inc`
  (NativeExternalFocus, the original's keeper and DoF blend witnesses), `12_scene_collect.inc` (the camera's request), `22_setup.inc`;
  `investigations/I-0066-independent-native-startup/Run-StartupTrace.ps1` (`shaft-observe.flag`); `scripts/Test-ThreadAffinity.ps1`,
  `scripts/thread-affinity-allow.json`.

- Tranches 6 and 7 (this thread): `native/frame/game_bootstrap/07_scene_lighting.inc`, `12_scene_collect.inc`, `18_resource_admission.inc`,
  `22_setup.inc`, `05_selected_scene.inc` (a comment), `native/world/light_producers.{h,cpp}`, `native/frame/glow_bank_test.cpp`,
  `investigations/I-0066-independent-native-startup/Run-StartupTrace.ps1` (the flags), `scripts/Test-ThreadAffinity.ps1`,
  `scripts/thread-affinity-allow.json`; `decisions/LOG.md` and `IN-FORCE.md` (D-0285).
- `native/world/light_producers.{h,cpp}`, `native/frame/game_bootstrap/10_options_and_passes.inc`, `05_selected_scene.inc`,
  `07_scene_lighting.inc`, `12_scene_collect.inc`, `native/renderer/model_frame.h`, `scene_pass_lighting.cpp`,
  `native/frame/glow_bank_test.cpp`; `investigations/I-0141-cutscene-lighting/CONTRACT.md`, `status.json`,
  `STUDY-LAYER-FADE.md`, `Make-I141Triptychs.ps1`, `Make-I141Timeline.ps1`.
- Tranche 2: `native/frame/game_bootstrap/04c_scene_witnesses.inc` and `22_setup.inc` (the observers),
  `investigations/I-0066-independent-native-startup/Run-AlphaCast.ps1` (`-SetFlagAfterTravel`).
- Tranches 4 and 5: `native/frame/game_bootstrap/18_resource_admission.inc`, `22_setup.inc`, `07_scene_lighting.inc`,
  `12_scene_collect.inc`, `native/world/light_producers.{h,cpp}`, `native/frame/glow_bank_test.cpp`; `decisions/LOG.md` (D-0282,
  D-0283); `.claude/agents/step-reviewer.md`, `final-reviewer.md`, `.claude/skills/review-step`, `review-final`,
  `merge-workstream`, `.project/WORKFLOW.md`, `CLAUDE.md` (D-0283).
- Tranche 3: `native/world/light_producers.{h,cpp}`, `native/frame/game_bootstrap/01_state.inc`, `07_scene_lighting.inc`,
  `18_resource_admission.inc`, `04c_scene_witnesses.inc`, `22_setup.inc`, `native/frame/glow_bank_test.cpp`,
  `native/frame/Build-GameBootstrap.ps1` (`DS3_EXE` before `glow_bank_test`); `Get-I141SweepLevels.ps1`, `Make-I141SweepRows.ps1`.
- Tranche 11: `native/frame/game_bootstrap/21_effects.inc` (141ACCA20's witness and tally, `ffx_entity_offered`'s fields,
  `ffx_pass_context`), `Compare-I141Spawns.py`, `Run-I141SmokePix.ps1` (`-Light`, F13); the wheel: `native/frame/graphics_rows.h`
  (`ScrollByWheel`), `graphics_rows_test.cpp`, `02b_rekindled_graphics.inc`, `17_render_step.inc`, `22_setup.inc`, Run-AlphaCast's
  `w:` step, Run-I138HdrMenu's wheel run; DS3's vignette in the Rekindled look (D-0297): `native/renderer/post_params.h`,
  `shaders/post_tone.hlsl`, `tone_looks.h`, `auto_exposure_shader.h`, `native/world/post_tone.h`, `post_tone_test.cpp`,
  `post_tone_stage_test.cpp`; evidence `evidence/I-0141/artifacts/` (`runs-smokespawn.log`, `runs-smokedup.log`, `vignette-sheet/`,
  `er-vignette-recheck/`, `build-ps24`, `build-ps25`); `scripts/Test-ThreadAffinity.ps1` (EntityOffered in the checked scope) and
  `scripts/thread-affinity-allow.json` (its once-a-session row counter, an atomic behind its flags).

- Tranche 25: `native/frame/game_bootstrap/07_scene_lighting.inc` (`no-cutscene-fill-lights.flag`), `Run-StartupTrace.ps1`'s flag
  pattern and clean-up list, `Compare-I141HoldRegions.py` (new); runs `i141-t25s-*-nat` and `i141-t25r-*-nat`, `build-i141-t25a`,
  `evidence/I-0141/artifacts/tranche-25/` (script, logs, `sun/`, `storm/`, `regions/`), `evidence/I-0138/t25-body/`.
- Tranche 26: `native/frame/game_bootstrap/01_state.inc` (`witness_share_variant`), `04_updates_and_test_tools.inc`
  (`FollowWitnessShareVariant`, the holds' 30 s bound), `07_scene_lighting.inc`, `13b_prepare_model_frame.inc`,
  `15_cpu_service_and_input.inc`; `Run-AlphaCast.ps1` (`-CutsceneHoldVariants`), `Compare-I141HoldVariants.py` (new); runs
  `i141-t26sa-nat`, `i141-t26sb-nat`, `i141-t26ra-nat`, `i141-t26rb-nat`, `build-i141-t26a`, `evidence/I-0141/artifacts/tranche-26/`
  (script, logs, `sun/`, `storm/`, `regions/`).
- Tranche 27: `native/frame/game_bootstrap/01_state.inc` (bits 32 and 64, `reference_share_counts`), `04_updates_and_test_tools.inc`
  (`FreezeTimelineDuringHold`, `SetHoldWorldFrozen`, `ReferenceShareVariantIdle`, the follower's Reference reads), `04c_scene_witnesses.inc`
  (the `ReferenceShare*` hooks), `22_setup.inc` (`reference-share-variant.flag`, `hold-world-freeze.flag`), `native/renderer/scene_pass.h`
  and `scene_pass_lighting.cpp` (`share_variant_off`); `Run-AlphaCast.ps1` (variants to 127), `Run-StartupTrace.ps1` (the freeze
  flag); `t27-world-clock-reading.md`, `t27-original-ambient-reading.md` (new); runs `i141-t27sa-orig`, `i141-t27sb-orig`,
  `i141-t27ra-orig`, `i141-t27rb-orig`, `i141-t27sc-nat`, `i141-t27sc-orig`, `i141-t27rc-nat`, `i141-t27rc-orig`; builds
  `build-i141-t27a`, `build-i141-t27d` (the runs'), `build-i141-t27b`, `-t27c`, `-t27e`, `-t27-final`, `-t27-final2` (the reviews'
  answers, 195 PASS lines each); `evidence/I-0141/artifacts/tranche-27/` (scripts, logs, `sun/`, `storm/`, `regions/`,
  `world-clock/`).
- Tranche 28: `native/renderer/shaders/rt_reflect.hlsl` (`RT_REFLECT_HIT_NO_FILL`, `RT_REFLECT_HIT_NO_SUN` in `DirLights` and
  `HitLight`), `native/renderer/rt_scene.h`, `native/frame/game_bootstrap/01_state.inc` (bits 128, 256 and 512,
  `reference_census_epoch`), `04_updates_and_test_tools.inc` (the follower's new bits), `04c_scene_witnesses.inc`
  (`ReferenceShareCensus`); `Run-AlphaCast.ps1` (the bits by mode), `Read-I141ShCensus.py` (new); runs `i141-t28sc-nat`,
  `i141-t28sc-orig`, `i141-t28rc-nat`, `i141-t28rc-orig`; builds `build-i141-t28a`, `build-i141-t28b` (the runs'); `evidence/I-0141/artifacts/tranche-28/`
  (`run-t28.ps1`, logs, the four tables, the two censuses, the two triptychs).

## Suggested next step

Tranches 17 to 24 read the m32 counts: at Archdragon Peak's bonfire the castle's rows take the same key block as the original's
(tranche 17); of what the native draws there and the original does not, 137 models are the original's occlusion (INFERRED its sole
cause), c3140 D-0222's LOD scale, and the five margin pieces lie wholly outside the view (tranche 22, OBSERVED); the 8 models without a
G-buffer row: seven still drawn by the original in a partial window like that state (tranche 24). Yorshka's +1 is not pursued (tranche 22).

Left:

1. The darkness of the characters (Father Ariandel, STUDY-ALL-GAPS row 4), of the castle and player under the haze at Archdragon
   Peak, and of the whole Road of Sacrifices view (tranche 16); 34010010 lighter and 41000060's cut 0 darker (I-0138 tranche 10).
   All are the lighting's, workstream 12's: its first steps are each area's ambient and the fill lights as traced sources. Thomas
   keeps the order (« On garde le rythme actuel », 2026-11-21 04:52). Its witnesses are `i141-t14n`'s bonfire, `i141-t16a-z16`'s
   player and forest, and `i141-t17b`'s castle keys (set 3, `{132000000, 0}`, on the castle's pixels).
2. Done in tranche 23 (READ and unit-tested; no fade compared native against original in a run; the IBL fade reaches only own
   keys other than {0, -1}, none observed): the post, shadow and IBL settings fade value by value as the original's value classes.
   Left of it: the
   earlier end is the earlier layers' answer, not the values the screen showed at the change (a fade over a fade), and
   EnvColorScale of an area without its own file answers nothing during a fade, as before.
3. Tranches 25 to 28: 32000010's clear excess (cut 1 +15.8, cut 5's frame 140.2 +18.2): the fill lights and glare take about as
   much in both modes (tranche 27); the native's diffuse-GI and specular excess at cuts 1 and 5 is the sun's light at the traced hits,
   about half each at cut 1 (not cut 2's specular), which the original's characters do not receive (tranche 28): without it the native meets the original at cut 1 (122.0
   against 121.8) and cut 5's frame 140.2 (151.0 against 147.3). Thomas finds the dragon and the King too bright (2026-11-23 18:01:
   « Oui ils sont trop clairs »), and sets the aim at 18:03: « Le mieux serait de retrouver l'ambiance d'origine avec le RT mais sans
   raster »; scaling the light down would be a lighting cheat (D-0262). Left: the characters' ShLightMask 0 (the original's deferred sun mask, GB1.z),
   which the native's hits take as 1: its meaning for their indirect light and the characters' stencil read, then a variant whose hits
   honour the instance's mask; which writer fills the characters' instance SH (the `.btpb` probe or key q+0x1A0's set); the clear gaps
   left at cuts 2 to 5 against the native's run-to-run spread there (several runs a mode, the spread's cause); the storm's weather
   aligned between runs (read or set at the hold) before any storm excess is measured; the probes' own sky misses (bit 4 ORed into
   `EncodeRtProbes`' constants).
4. Tranche 24 (one run a mode): in a partial window like tranche 18's state, the original still draws seven of the 8 (`o328120`
   by neither). Left: a run whose whole later windows reach tranche 18's state (a longer stay), to settle the 8 and re-test the 137;
   why `i141-t24a`'s original kept them in its G-buffer longer; the parts' names (the original's base `*_M_0000` against the
   native's equipped files); the margin pieces' « no pixel » (INFERRED) and the row on the gathered path (not run).
5. Done in tranche 24 (unit-tested, no fade compared in a run): Sun and Fog fade value by value. Left: the fog position and area
   the current's at once (INFERRED), and a fade over a fade starting from the earlier layers' values.
