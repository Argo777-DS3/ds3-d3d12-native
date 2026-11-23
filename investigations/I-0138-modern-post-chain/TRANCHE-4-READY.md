# I-0138 tranche 4: the depth of field and the motion blur, a first sheet (static)

2026-11-13, the R pilot's thread (Fable 5.2), written after tranche 3's checkpoint while the fresh thread's card waited for
Thomas. Read-only and short: what the tranche is made of, in what order, and what must be read or captured first. Read for
it: `native/world/yebis_dof.cpp` and `yebis_motionblur.cpp` (the techniques they name, their refusals), `yebis_pins.h`, the
library's technique table (`evidence/I-0067/queries/yebis-techniques.tsv`), [STUDY](STUDY.md) §4, the originals'
disassemblies (extracted for this sheet). Not read: the two plans' bodies beyond those lines, their tests' bodies,
[CENSUS-CUTSCENES](CENSUS-CUTSCENES.md) again. Labels: **verified**, **proposed**, **open**.

## 0. Answer first

1. **Proposed: three parts, each merged alone.** 4a, what the world draws today remade exactly with our programmes, as
   tranches 1 and 2 (nothing changes on screen; small). 4b, the cutscenes' depth of field, which the native refuses today
   in almost every record: it needs a capture of the original in a cutscene first, then the library's other branch
   transcribed (the large part). 4c, the Reinhard curve of two cutscene records.
2. **4a alone removes nothing visible; 4b is where the picture changes**, and a first observation exists: the game tour
   notes, by eye only, that the background of Vordt's intro looks sharper natively (I-0139 RESULT « Tranche 3 »,
   `evidence/I-0139/artifacts/sheets-t3/`), which is what a refused depth of field gives.
3. Lesson of tranche 2 to apply from the start: **every technique a plan can name, at every size, must have its
   programme**; a test at other extents finds the ones the 1280x720 captures never showed.

## 1. What is drawn today (verified)

| Pass | Draws | The library's techniques | Pinned originals |
|---|---|---|---|
| Depth of field (`yebis::BuildDof`, a second `GlareStage`) | 10 at the witness zones | `DepthOfField_BlurFactorAlphaFromDepthTextureTypeCastLut[T1_S1]_RVALUE` (6 and 7 instructions), `MadT1_S{n}` (the layers' downsamples, n = nx x ny, each at most 5, and the tap blurs), `DepthOfField_FrontBlurBlendFactor` (7), `DepthOfField_CompositeBlurLevelsFrontBlur_TS{back}_{front}` (back 0 to 4, front 0 to 7: 10 to 55 instructions, up to 8 textures) | all 40 composites pinned (one observed, `TS0_3`), the three others observed |
| Motion blur (`yebis::BuildMotionBlur`, a third `GlareStage`) | the camera velocity, then the blur passes | `MotionBlurMakeCameraVelocityOverwrite_R` (29 instructions), `MotionBlurVelocityDirDepthInnerscreen_SM3_T2_S{taps}` (79 at 5 taps) | the 5-tap blur only; the library's table has 2 to 8 taps |

- `MadT1_S{n}` is the glare family's shape, ours since tranche 2 up to 15 taps. The depth of field also names 16, 20 and
  25 taps (pinned, not observed; 40, 52 and 67 instructions): the taps beyond the fifteenth read their offsets and weights
  from further words of the same block (`168 + 4t`, `616 + 4t`): the shape's loop extends, its block declaration too.
- The plans' refusals today: the depth of field refuses holder flags other than 2 (the front blur), a custom preset, a
  quality, level count or taps index outside the witnessed ones, a base under 16; the motion blur's plan refuses a tap
  count other than 5 (« unpinned technique », twice) and a far depth threshold, and its prepare block a mode other than 2,
  the falloff and the camera threshold flag.
- Tests: `dof_stage_test` (five captures: m37, m31, m50, m40, m30) and `mb_stage_test` (those and a turning camera at
  m37), the plans' word tests `yebis_dof_test` and `yebis_motionblur_test`.

## 2. Part 4a: the same bytes with our programmes (done 2026-11-13)

Done as proposed: [RESULT](RESULT.md) « Tranche 4, part 4a ». What differs from the proposal: the listings are compared
by [Compare-Programmes.py](Compare-Programmes.py) (the strict rule, then the values where the compiler's scheduling
differs); the depth factor's `T1_S1` form is not written (no plan names it); the motion blur's refusals at tap counts
other than 5 are gone (2 to 8 served): a latent refusal, every world record giving 5 taps at every size (the RESULT; the
cutscenes' three MotionBlur records are not read). The refusals left in the two plans are of the plans themselves (§1's
list but for the tap counts), with one exception found by the final review and open for part 4b: the frame builds
seven levels, and `DofLevelCounts` can count more than four back levels (a focus at or under 0, no back limit), for
which neither the library nor we have a composite (`TS5_x` to `TS7_x` do not exist): the library's clamp of that count
is to read in `FUN_14175ED50`, or the plan to refuse it by name. No witness reaches it.

## 3. Part 4b: the cutscenes' depth of field (done 2026-11-14)

Done with part 4c: [RESULT](RESULT.md) « Tranche 4, part 4b » (the cutscenes' draw-parameter files were never read natively:
found and fixed first; the capture settled this section's open points). The text below is the sheet as it was.

- **Verified in the data, inferred in its mapping**: 229 of the 232 cutscene depth-of-field records have an edge quality
  of -1, which STUDY-YEBIS-REFUSALS maps to holder flags other than 2: refused natively. The library holds 832
  depth-of-field techniques (background layer masks at 5, 9, 13 and 17 taps, colour masks, and others): the branch those
  records take draws techniques the native has never drawn.
- **Read since (2026-11-13, part 4a's thread, GhidraMCP; verified on the decompilation, its link to the records
  inferred)**: `FUN_1416CBFE0(…, index)` sets the holder's flags (+0x140) from `FUN_1416FDD80(index)`, the table
  `{0, 0, 2, 6, 6}`; for an index of -1 that function returns `0x10000000` and the flags become **the holder's creation
  flags** (+0xC8: the same table at the DoF quality row's third word, `DofSetup::creation_flags`). The quality rows
  (`14469B510`, 0x1C bytes each) hold 1, 1, 2, 2, 2, 3 there: creation flags 0 at qualities 0 and 1, 2 at qualities 2 to
  4, 6 at quality 5. The native frame refuses an index outside 0 to 4 (« holder flags ») where the original takes the
  creation flags. Our runs draw at DoF quality 0 (the rows `native_yebis_dof`, `"quality":0`): there a record of edge
  quality -1 gives flags 0, which `FUN_14175F850` composites with `FUN_1417263B0`,
  `tech_DepthOfField_CompositeBlurredLevels[_USNORM]_T{n}` (I-0068 YEBIS-TECHNIQUES §8.2: flags under 2), without the
  front blur's blend-factor draws (§8.1: front-blur mode only). So the first branch to transcribe is that one (eight
  techniques by level count, none pinned), not the masked composites (flags 6, 14, 22). **Inferred**: that the records'
  edge quality is the index `FUN_1416CBFE0` takes (STUDY-YEBIS-REFUSALS; YEBIS-TECHNIQUES §8.2 gives `DofEdgeQuarity` to
  +0x158 instead): the capture below settles it (the flags and the composite's technique it draws).
- **The flags-under-2 composite, read in `FUN_14175F850` and `FUN_1417263B0`** (the same reading; verified on the
  decompilation): the textures are the scene and the levels up to the holder's larger count (+0x1D4), n = that count
  + 1; each takes a pair of words from its level record (+0xD0, +0xD4: the ramps `d0`, `d4` `DofLayerRamps` already
  computes; zeros for the scene), put as one float4 a texture; the technique is row n of a handle table, eight rows
  further for its second form. **Not verified here**: that this table is `CompositeBlurredLevels` and the second form
  `_USNORM` under +0x154 (I-0068 YEBIS-TECHNIQUES §8.2 says so of the mode-2 composite, and the function carries that
  name in the Ghidra project); `FUN_1417263B0` also holds a second table, chosen by a byte the caller clears on this
  path, not identified (perhaps the `…BlurredLevelsFrontBlur_T{n}` family); its last argument is a column of the
  row (22 a row), 0 here by the caller's stack, unread. The capture names the technique drawn. The pixel programme of
  `T1` is the one `…FrontBlur_TS0_0` has (the same hash: a plain
  copy); `T2` to `T8` are other programmes than `TS1_0` to `TS4_0` (their hashes differ: read their listings before
  assuming the back levels' formula). All eight are in the library's table
  (`evidence/I-0067/queries/yebis-techniques.tsv`), none pinned.
- **First step, before any code**: a capture of the original's library draws during a cutscene with such a record
  (`yebis-draws-capture.flag` in a Reference run; the game tour's cutscene tool, once merged, replays Vordt's intro), to
  learn which techniques, surfaces and constants that branch really uses. Without it the transcription has no oracle.
- Then: the library's CPU branch for those holder flags transcribed (`ghidra-research` on the functions `BuildDof` names),
  its plan, its programmes of our own, each compared with the capture, as the world's were.
- **Open**: which records each cut of a cutscene selects (the `.mqb` timelines are not extracted on this PC, PLAN §6);
  a first target is one cutscene end to end (Vordt's intro), not all 49 files.
- For Thomas at its end: Vordt's intro filmed beside the original's (PLAN §5).

## 4. Part 4c: the Reinhard curve (done 2026-11-14)

Done: [RESULT](RESULT.md) « Tranche 4, part 4c ». The text below is the sheet as it was.

- Two cutscene records have a tone type other than 2 (refused natively: « tone type »): the Tone Map ids 0 and 1 of
  `s40_00_0040`, tone type 3 (CENSUS-CUTSCENES' spot check; [STUDY](STUDY.md) §2's table: curve 3 is the technique
  `_REINHARD`, after TONEMAP-SKIN §16). The library's table holds the `TonemapHDR_*_REINHARD_*` techniques; none is
  pinned or captured. Which cutscene id plays `s40_00_0040` is to find for the capture (the tour's
  `sheets/cutscenes-events.md` and `tools/Find-RemoEmevd.ps1`).
- First step: which technique the original draws for those records (a capture in that cutscene, or the function that
  selects the tone-map technique from the tone type), then its listing, a curve mode in `PostToneParams` and
  `post_tone.hlsl`, and its test against the captured draw.

## 5. Order and size

4a done. 4b next (L: a capture, a decompilation read, a new plan branch; by an Opus 5.5 thread, Thomas 2026-11-13,
D-0276's entry). 4c with 4b's capture slot if the same cutscene serves, else after it. Steps for 4b, in order:

1. The capture of the original in a cutscene (a run slot from the coordinator): the holder's flags, the techniques
   drawn, their constants and surfaces, at the DoF quality our runs use. The tools: the game tour's cutscene runner,
   merged with I-0139 (`investigations/I-0139-game-tour/Run-I139Cutscenes.ps1`: `Run-AlphaCast -Cutscene`, the module's
   `witness-cutscene.txt`, natively then in the original), and the library draws' capture of the Reference runs
   (`yebis-draws-capture.flag` with `yebis-hal-observe.flag`: I-0066 W10-NATIVE-PASSES « The capture of every draw »;
   it dumps the first detailed frame from a frame count, 1200 by default, which must fall inside the cutscene: to check
   dry before the slot, as must the runner's passing of those flags). **Not known**: which of the game's Depth of Field
   options gives which quality (word 0x4C; our runs' is 0). At qualities 2 to 4 an edge quality of -1 gives flags 2,
   the branch already served: there step 2's mapping alone may serve the cutscene, and the capture says which case
   Thomas's own settings are.
2. The native frame's mapping: an index of -1 takes the setup's creation flags (`13b_prepare_model_frame.inc`, where
   `flags_index<0` is refused today); `BuildDof` then serves flags 0 (no blend-factor draw, the composite
   `CompositeBlurredLevels_T{n}`), each word checked against the capture as `yebis_dof_test` does for the world's.
3. The eight composites' programmes: pinned from the table (as `mb-table.tsv` was made: the table's hashes, then
   `extract_pinned.py` once the pins are in `yebis_pins.h`), a shape in `post_glare.hlsl`, a row in
   Compare-Programmes.py's `units()`, a sweep in `dof_stage_test` (as `CompositePlan`).
4. What else the cutscene's records ask and the plan refuses (a custom preset, taps, the lens values' ranges): from the
   capture's rows, one refusal at a time, each with its programme and its test.
5. For Thomas: Vordt's intro filmed beside the original's.

## 6. Evidence gathered for this sheet

`evidence/I-0138/artifacts/tranche-4-programmes/`: the listings of the depth-factor shapes, the front blur's blend factor,
the forty composites, `MadT1_S16`, `S20` and `S25`, the camera velocity, the blur at 2 to 8 taps (the six unpinned ones
by the library table's hashes, `mb-table.tsv`) and their vertex programmes (`extract_pinned.py`, `fxc /dumpbin /Lx`).

## 7. The programmes read (verified on the listings; notes for part 4a's transcription)

The block is the glare family's (`post_glare.hlsl`'s cbuffer); registers named `cN`.

- **Depth factor** (`..._TypeCastLut_RVALUE`): `d = t0(uv).x x c0.x + c0.y`; `f = t2(d, c50.x).x` (the CoC table, its row at
  `c50.x`); alpha `= c17.x x f + c17.y`; colour 0. The `T1_S1` form: colour `= t1(uv).rgb x c154.rgb`.
- **Front blur's blend factor**: `w = t0(uv).a x c154.w + c155.w`; colour kept; alpha `= min(max(w, 0)^2 x 0.25, 1)`.
- **Composite `TS{B}_{F}`** (`uv[0]` for t0, `uv[1]` for the levels; alpha out `= t0.a`): `c = t0.rgb`; for i = 1 to B:
  `c = c + sat(t0.a x c(18+i).x + c(18+i).y) x (t_i.rgb - c)`; then for k = 1 to F:
  `c = c + sqrt(sat((t_k.a - t_(k+1).a) x 4)) x (t_k.rgb - c)`, the last k without the subtraction. The same textures
  t1.. serve both loops. `TS0_0` is a plain copy of t0. fxc schedules the 40 variants differently (which operand of a
  difference comes first changes between variants): the bytes are the criterion, the listings a guide.
- **`MadT1_S16`, `S20`, `S25`**: the glare's taps shape; taps 8 and on at `uv[0] + c(42+t).xy`, weights `c(154+t)`.
- **Camera velocity** (`vs_2D_Tex2D3`, three coordinate pairs): `d = t0(uv0).x x c0.x + c0.y`; `z = sat(c155.x d +
  c155.y)`; `w = c15.w z - c15.y`; `q = -c15.z / w`; `p = (uv1.x, uv1.y, z) q`, `p.w = q`; `n = (p.c224, p.c225,
  p.c226) / max(p.c227, 0x3a7fffd5)`; `v = n.xy - uv1`; alpha out `= n.z`; object velocity `o = t1(uv2)` (read `.wxyz`):
  `ov = (o.xy c156.z + c156.w) c156.xy - v`; `v = v + sat(o.w) ov`; `v = v c154.xy`; if `|v| > 1`, `v = v / |v|`
  (`rsq`); out.xy `= v c154.z + c154.w`, out.z 0.
- **Blur `..._T2_S{n}`** (n = 2 to 8): `step = t1(uv).xy c187.xy + c187.zw`, `dir = t1(uv).xy c186.xy + c186.zw`, centre
  `= t0(uv)`, `thr = centre.a + c189.y`. A tap at `p = uv + m step` (`SampleLevel` 0): it is taken when it is inside
  (`|p c190.xy + c190.zw| <= 1` on both axes) and (`dot(dir, dir) c189.x < dot(tdir, dir)`, `tdir` the tap's decoded
  velocity, or its alpha `>= thr`); `sum` starts at the centre and adds the taken taps' texels, `count` starts at 1.
  Out `= (|dir.x| + |dir.y| >= c188.z) ? sum x c(154+n) / count : centre`. The taps' multiples m, in the order the
  sums are made: n odd, the negative side first (`S3`: -1, +1; `S5`: -1, -2, +1, +2 read in the listing); n even, one
  side (`S2`: +1; `S6`: +1, +2 and on). **To confirm for each n** on the listing's chain of adds before trusting it;
  the comparison with the library's programme on synthetic velocities will catch a wrong order (a float sum).
- The motion blur's plan refused a tap count other than 5 on the base (`NumBaseSamples`) and on the last pass. This
  sheet first read that as « likely refused at some sizes »: wrong, the world's records take the recurrence cap at
  every real size and draw 5 taps (the RESULT's part 4a, the final review's M1).

## 8. What part 4a leaves for parts 4b and 4c

- **The method's tools**: [Compare-Programmes.py](Compare-Programmes.py) (a new shape is one row of its `units()`; it
  needs the original's listing as `original-<name>.asm` in the evidence folder: `extract_pinned.py`, then
  `fxc /dumpbin /Lx`); the stage tests' profile `-ours` (`dof_stage_test`, `mb_stage_test`: the captures, the library's
  programmes on the same device, other extents with `synthetic_scene.h`'s inputs, plans made for the comparison where
  no builder's plan names a technique); the frame's `ChooseNativePostProgrammes` and its two flags
  (`post-glare-old.flag`, `post-glare-verify.flag`, whose witness pairs [Compare-ToneVerify.py](Compare-ToneVerify.py)
  reads with `--need dof` and `--need mb`).
- **Lessons**: a heredoc in the Bash tool loses backslashes and fails on some quotes: write an edit script to a file,
  or use the edit tool. Today's fxc under `/Gis` emits `mad` then `mov_sat` for a saturated `mad`, writes only the
  components of a sample it reads, and orders a difference's operands its own way: the reader's values rule absorbs the
  first two and lists the third. WARP propagates another NaN than the library's programmes in the motion blur on inputs
  that hold NaNs (the card does not): the test's rule for it is stated in the RESULT.
- **Part 4a's live slot was held** (the RESULT's « The merge and the live comparison » of part 4a: 52 motion blur,
  52 glare and 52 tone-map pairs equal). What it did not cover, for a later slot that has another reason to run: a
  depth of field pair (the witness views draw no level: a cutscene of part 4b will) and a camera that turns in every
  run (the window lost the foreground to the Claude application in 10 of 12: LIVE-RUN-NOTES' pitfall of the small
  windows, here at every size). The motion blur's constants at a tap count other than 5 have no reference from
  the original (the step review's L4): one would need a cutscene's record that asks another count (the three cutscene
  MotionBlur records are to read with part 4b's capture) or a test's values in a Reference run.
- The earlier thread's draft (`draft-4a.patch`, `compare_listings.sh`, `compare-listings.txt` in the evidence folder)
  is superseded by the tree; kept as it was left.
