# Investigation result

ROADMAP workstream 11 (the modern post-processing chain in YEBIS's place). The workstream stays `active` until its last tranche
merges; this file holds one section by tranche.

## Step 1: the study and the plan (2026-11-12, read-only)

### Conclusion

- **What YEBIS does in DS3** ([STUDY](STUDY.md) §1 to §3): in the world, one filmic curve, a fixed exposure by area, a
  saturation, a vignette, a 16³ LUT by area, a bloom, screen-space light shafts in 12 of 18 areas, one depth of field and one
  motion blur the same everywhere; in the cutscenes, a physical-lens depth of field in 208 of 232 records, animated exposure,
  gamma, value and contrast, a vignette that follows the field of view and, in a few records, a Reinhard curve, a custom glare
  and the chromatic aberration. H2 of the CONTRACT holds.
- **ROADMAP's open check is answered** (STUDY §2.3): DS3's chromatic aberration is never drawn in the world. Its enable is 0
  in all 100 retail world files, the original's capture at the High Wall (`Reference1706`) draws no such pass with a
  dispersion of 0.006 in the data, and the enable is 1 in four records of three cutscene files only. That the entry named
  `Enable` is the aberration's switch is inferred from its place in the group.
- **What Elden Ring does instead** (STUDY §5): no YEBIS; GX filters of FromSoftware's own in a fixed order (depth of field,
  TempAA, motion blur, glow, one tone-map filter holding exposure, bloom, shafts, a table curve, the colour matrix, a blended
  3D LUT, vignette and chromatic aberration, then an output pass; the UI blended from a texture). The parameter model is
  DS3's, extended, and DS3 already holds the ancestors of these filters and their parameter blocks. H1 holds. ER's HDR is a
  vendor-API path, not a model.
- **The native chain today** (STUDY §4): six passes of our own that draw YEBIS's programmes read from the executable, and
  refuse the configurations the witness zones never showed; everything after the tone map is 8-bit.
- **The plan put to Thomas** ([PLAN](PLAN.md)): the chain in ER's order with DS3's values, two colorimetries, eight tranches
  (the original colorimetry remade first with nothing changing on screen, the bloom and shafts, the exposure, the depth of
  field and motion blur, HDR10, the Rekindled colorimetry, the settings rows, YEBIS removed), four decisions for him.

### Evidence

- [CENSUS-WORLD](CENSUS-WORLD.md) and [CENSUS-CUTSCENES](CENSUS-CUTSCENES.md) (Sonnet workers, from the 168 GPARAM dumps of
  `evidence/I-0066/queries/gparam-all/`; scripts and raw tables in `evidence/I-0138/artifacts/census-world/` and
  `census-cutscenes/`). The pilot's checks: `m30_00_0000` and `m37_00_0000` re-read in the raw dumps; the dispersion block of
  all 168 files re-read by a script of the pilot's own (not kept: its output is STUDY §2.3's list); the cutscene reader
  reproduces STUDY-YEBIS-REFUSALS' totals.
- `evidence/I-0066/artifacts/i66-reference-1706-m30-yebis/startup.jsonl`: the original's library draws at the High Wall, 19
  techniques, none of the chromatic, distortion, ghost or star families.
- [ER-POST-CHAIN](ER-POST-CHAIN.md) and [ER-TONEMAP-HDR](ER-TONEMAP-HDR.md) (`ghidra-research` workers, Elden Ring 2.7.1,
  headless Ghidra 12.1.3 on the mirrors `er-mirror-i0072-a` and `-b`, 30 runs; raw outputs in
  `evidence/I-0138/artifacts/er-post-chain/` and `er-tonemap-hdr/`).
- The native tree at `8f415270b`: `native/renderer/scene_pass_frame.cpp` (the frame's post passes), the stage headers,
  `native/world/yebis_*`.

### Uncertainty / unresolved questions

- The ER reports are workers' readings, read in full by the pilot, with no ER address re-read by it: a design decision
  that rests on one re-reads it first. ER's execution order comes from its sort keys (the walk of the queue was not read);
  its per-area values and shader maths are in data files not opened; whether its HDR runs on PC is unknown.
- The cutscene census reads every record a file holds, not the records a cut plays (the `.mqb` timelines are not extracted
  on this PC).
- That most cutscene depth-of-field records are refused natively is inferred from the code and the data (the holder flags'
  mapping to `DofEdgeQuarity` is STUDY-YEBIS-REFUSALS' inference); no cutscene was run.
- What glare shape 3 and the custom glare draw in the original is not known (no capture).
- Nothing was built, launched or measured.

### Artifacts produced or modified

- This investigation's CONTRACT, STUDY, PLAN, the two censuses and the two ER reports; `evidence/I-0138/artifacts/` (four
  folders, scripts and raw outputs).
- ROADMAP (workstream 11 step 1 done; the chromatic aberration check answered), STATE (the Fable lane), the investigations
  index.

### After the plan (2026-11-12, 11:55 to 12:45)

- Thomas's answers in the thread: he can turn Windows' HDR on for the HDR10 tranche (11:55); « Je suis ta recommandation
  pour les points 2 à 4 » (12:19): no chromatic aberration row, reproduced in the three cutscenes; Elden Ring's shipped
  values read from its data files, read-only, as a candidate of the Rekindled colorimetry; the emissive glow before the
  tone map at the HDR10 tranche. Point 1 (the light shafts) is open: he asked for a way without wasted work, and the
  pilot proposed keeping the current pass until workstream 12 (PLAN §7, with what it leaves of YEBIS). The decisions
  go to `decisions/LOG.md` at the merge, once point 1 is settled.
- [TRANCHE-1-READY](TRANCHE-1-READY.md): the design sheet of tranche 1, written read-only: one pass of our own in place
  of YEBIS's tone map and grading programmes, compared bit for bit with the original's captured draws (the existing
  goldens of T12 and T17), so no tolerance; the blue-noise dither and the float output moved to the HDR10 tranche.
- [ER-VALUES](ER-VALUES.md): Elden Ring's shipped values and tone-map programmes, read by a Sonnet worker from Thomas's
  own Elden Ring (his agreement above; value tables in `evidence/I-0138/artifacts/er-values/`, the extracted files and the
  disassembly in `Tools/DS3-Decompilation/er-post`, outside Git). ER ships an extended Reinhard curve, a bounded auto
  exposure, a neutral colour matrix with the look in a LUT by time of day, a bloom added before the curve, and an HDR
  branch that expands the SDR grade's highlights (STUDY §5.2). **A correction**: the plan first sent to Thomas said ER's
  default curve was DS3's sensitometric one; that is the code's default, not what the game ships. PLAN §3 and STUDY §0
  are corrected.
- The light shafts (Thomas, 12:21: « Je suis pas fan des adaptateurs franchement »): no bridge from the old pass to the
  new bloom; they are rewritten once inside the new bloom in tranche 2, as Elden Ring keeps screen-space shafts beside
  its volumetric fog (PLAN §7; written unless Thomas says otherwise).

### The merge (2026-11-12 12:40, documentation only, D-0239's frame)

- `main` (`cfe1fa457`) merged into the branch first (conflicts in STATE and the investigations index, resolved to main's
  rows with this workstream's lines added); Thomas's answers recorded as D-0276; `Test-Project -MergeGate` passed.
- No code: nothing rebuilt, no start check. **No fresh reviewer read this documentation before the merge** (the
  coordinator's slot was short, while R compiled); the workers' reports carry their own statuses and the pilot's checks
  are listed under « Evidence ». The disk rotation was not run (a build was running).

### After the merge (2026-11-12 12:34 to 12:50)

Thomas (12:34): « S'il a une légère aberration chromatique, on garde. Sinon je suis ok, ça me va. N'oublie pas de
joindre ton étude à la roadmap pour pas qu'elle soit oubliée ». So: the light shafts' rewrite inside the new bloom is
accepted; the Rekindled candidate made after Elden Ring keeps its slight chromatic aberration, with a settings row at
tranche 7 (the pilot's reading, said to him); ROADMAP workstream 11 now carries the plan's eight tranches and links the
study and its annexes. D-0276 completed.

## Tranche 1: the base and the original colorimetry (2026-11-13)

ROADMAP workstream 11, tranche 1 ([TRANCHE-1-READY](TRANCHE-1-READY.md), [PLAN](PLAN.md) §5), by the R pilot's thread (Fable 5.2,
branch `claude/silly-lovelace-0d9bdf`).

### Conclusion

- **Our tone-map pass draws in place of YEBIS's tone map and grading programmes**: `PostToneStage`
  (`native/renderer/post_tone_stage.{h,cpp}`), its programmes `native/renderer/shaders/post_tone.hlsl` (`vs`, `ps`, `ps_lut`,
  compiled by the build into headers) and a parameter block of our own, `PostToneParams` (160 bytes,
  `native/renderer/post_params.h`), laid out from the Tone Map record by `BuildPostTone` (`native/world/post_tone.h`). The
  record's semantics (the guards, the curve's two constants, the colour matrix, the vignette's rectangle) stay where they were
  computed and tested (`yebis_tonemap.{h,cpp}`). The two passes no longer draw a programme read from the game's image, by pin
  or otherwise, and no longer bind a `$Globals` block. What still comes from the game: GX's `LutCopy2DTo3DCs` (the game's
  compute programme, not YEBIS's, pinned) fills the LUT volume; the executable's data words read once at start
  (`TonemapImageData`: the curve's table, the colour matrix's constants, the dither's Bayer rows with their scale and bias,
  pi/8 for the vignette) feed the block and the dither texture; and the tone map's `$Globals` block
  (`yebis::TonemapGlobals`) is still built every frame, as the automatic exposure's input, until tranche 3 rebases it.
- **The same bytes as the original**, measured by `post_tone_stage_test` on the adapter and driver that drew the captures
  (`10DE 2689`, driver `00200000001006B2`, still this PC's): 0 differing bytes of 3,686,400 against the original's tone-map
  output at m37 (`i66-reference-1643`), m31 with the vignette (`1646`), and at m31 (`1696`), m40 (`1705`) and m30 (`1706`) for the
  tone map, for the LUT's draw alone from the original's 8-bit input, and for the two draws from the HDR to the graded output;
  the stage's LUT volume equal to the captured one bit for bit. Against the two stages it replaces (the original's programmes
  through `TonemapStage` and `GradingStage`), 18 draws a capture (the capture's scene and a synthetic one of ramps, saturated
  colours, values at the curve's floor, negatives, NaN, infinities, alphas out of range and noise; each plain, with the glare,
  the vignette, the automatic exposure's factor, a hue, value and contrast matrix, the LUT): equal byte for byte, on the card and
  on WARP.
- **The old path stays behind `post-tone-old.flag`** (a test tool: the original's programmes through the two old stages,
  untouched), and `post-tone-verify.flag` keeps our pass and, on the witness frames of `world-pictures.flag`, also draws the
  original's programmes from the same frame's inputs into a scratch target, both pictures written
  (`<stem>-tone-ours.rgba`, `<stem>-tone-yebis.rgba`; [Compare-ToneVerify.py](Compare-ToneVerify.py)). Both go with YEBIS in the
  workstream's last tranche.
- **Departures from the design sheet**, each for a reason found while building:
  1. **Two draws, as the original's, not one.** The sheet's one pass computed the 8-bit step between the curve and the LUT in
     the programme (`floor(x 255 + 0.5) / 255`) and named that rule as its open point. Measured on the m31 capture: that pass
     differs from the original on 73,764 bytes of 3,686,400 (2 %, 2/255 at most: 69,066 colour bytes one level higher, 1,863
     two higher, 512 one lower, and 2,323 alpha bytes one higher, the alpha going through no LUT). The original's graded output
     is a function of the pixel's own 8-bit input alone (87,245 distinct inputs, none with two outputs), ours was not (21,713
     with more than one), and 63,692 of our 66,378 differing pixels equal the original's output for an 8-bit input shifted by a
     level: 65,465 colour values a level up and 217 a level down, 2.38 % of the 2,764,800 (a lower bound: a shift that leaves
     the LUT's output equal is not seen). The programme's rounding so gave, mostly, a level above the card's write. That build's
     tone map alone was exact (its output kept, equal to the original's tone-map draw). So the pass keeps an 8-bit intermediate
     and a second draw wherever a LUT is on: the card's own write is part of the original's picture. One draw comes back with
     the HDR10 tranche, whose output is not 8-bit.
  2. **fxc with IEEE strictness, not dxc.** The original's programmes are shader model 4 without the refactoring flag;
     `fxc /T ps_5_0 /Gis` with `mad()` where the original has a `mad` gives the same operations in the same order, every one
     marked precise (compared instruction by instruction with the pinned programmes' disassembly). The sheet named fxc as its
     fallback; it was taken from the start, since the instruction stream can be read against the original's.
  3. **Two things the sheet's formula had not**: the matrix's output is saturated after adding 2^-22 (the literal prints as
     0.000000; `fxc /dumpbin /Lx` shows `0x34800000`), and the vignette multiplies the exposed scene before the curve's floor
     (the glare is not vignetted), as `saturate(-d 2 + (d + 1))^4`.
  4. The vignette's coordinates go to the programme at the three vertices (the original's TEXCOORD2, interpolated by the
     rasteriser as the original's), not as a rectangle the pixel programme would expand. The block has no curve mode and no
     « exact output » flag yet: they come with the tranches that give them a second value.
  5. The new stage owns its LUT volume and the copy; `GradingStage` is not reduced to a volume builder, so that the old path
     behind its flag is the code that ran before, unchanged.
  6. The journal row keeps its name `native_yebis_tonemap`, with a field `"programmes":"ours"` or `"yebis"`; it is renamed
     when YEBIS goes (documents of I-0113 name it).
- **What the frame draws now** (`scene_pass_frame.cpp`): the same passes in the same places (the plan key and the frame graph's
  passes unchanged: « composition », then « colour_grading » with a LUT); `TonemapFrames` and `GradingFrames` count either
  path. The refusals « programmes » and « vignette programme » exist only where the original's programmes are read (the two
  flags); « tone type » (the Reinhard curve of two cutscene records) stays refused until tranche 4.

### Evidence

- The build `evidence/I-0138/artifacts/build-t1` (this branch's final tree `7092e90bf`, `main` merged in, built 2026-11-13 09:19 to 09:32, every unit test passing): `post-tone.txt` (`post_tone_test`: 78 checks, the
  block against the original's captured words and the cbuffer's layout in the three programmes), `post-tone-stage-<capture>-hardware.txt`
  and `-warp.txt` (`post_tone_stage_test`, five captures), with the unchanged T12 and T17 (`tonemap-stage-*`, `grading-stage-*`).
- The one-draw form and its reading: `evidence/I-0138/artifacts/tranche-1-one-pass/` (`post_tone_one_pass.hlsl`, its outputs
  `one-pass-m31-lut.raw`, SHA-256 `b2b143e6…e2b9f4`, and `one-pass-m31-tone.raw`, equal to the capture's
  `reference-draw-49-output.raw`; the readers `lut_counts.py`, `lut_diff.py`, `lut_func.py`, `lut_q.py`, `lut_model.py` and their
  outputs).
- The programmes compared: `evidence/I-0138/artifacts/tranche-1-programmes/` holds the five pinned originals' listings
  (extracted from the executable by their SHA-256, `yebis_pins.h`; `fxc /dumpbin /Lx`, the literals as bits; the vignette
  variant's is what departure 3 rests on) and ours (`ours-post_tone_*.asm`, the same compile line as the build's).

### Tests (D-0268)

- Added: `post_tone_stage_test` and `post_tone_test` (the build runs both; a failure stops it). No existing test, assertion,
  tolerance, reference or reader is changed; T12 and T17 still run on the old stages.
- The new test's rule, stated: byte for byte on the capture's adapter and driver; on another adapter, driver or WARP each draw
  alone within 1/255 on fewer than 5 % of the bytes (T12's and T17's rule), and the two draws chained from the HDR are reported
  without a bound, because the tone map's 1/255 then goes through the LUT (WARP: 2/255 on 4.8 % of the bytes, where the old
  tests never chained the two); against the stages it replaces, byte for byte on any adapter.

### Uncertainty / unresolved questions

- The rule the card applies when it writes a float into an 8-bit target, measured after the reviews (K-0489, `OBSERVED`;
  `evidence/I-0138/artifacts/tranche-1-unorm-rule/`: a pass-through programme over 1,044,480 floats around every level's
  midpoint, read back): `floor(256 x - floor(16 x) / 16 + 7/16)`, with no mismatch, where D3D's rule is `floor(255 x + 0.5)`
  (D3D allows 0.6 unit of the last place): a level lower than D3D's rule where the fraction lies between 0.5 and a threshold
  that runs from 0.5003 to 0.5623 with the level, never higher. That is the direction and the size the one-draw form showed.
  The rule is this card's and driver's; another card's is not measured, which is one more reason to let the card write: the
  two draws are exact on any card, a programme that copied this rule would be exact on this one.
- The byte-for-byte proof holds while this PC's adapter and driver are the captures' (`10DE 2689`, `00200000001006B2`). After a
  driver update the hardware runs fall to the « another adapter » rule, as T12 and T17 do, and say so in their line; the
  comparison with the stages it replaces stays byte for byte on any adapter until those stages go.
- The automatic exposure's factor is taken as grey (its stage writes `(a, a, a, 1)`); a coloured factor would differ from the
  old path by the original's desaturating remap, which is not transcribed.
- The unit tests draw 1280x720 only (the captures' size). Nothing in the pass depends on the size but the viewport and the
  dither's repeat; the witness slot covers three sizes.
- The pipelines of the new pass are not in a player's existing pipeline list: they compile at the first world frame of the
  first run after the update (three small ones), then join the list.
- Not run before the merge (D-0229): any live frame. The unit harness does not cover another presentation size, the TAA's or
  DLSS's output as the source, the live factor and vignette, nor the frame graph's plan with the new resources: the witness
  slot below does, before tranche 2 builds on this one.

### Artifacts produced or modified

- New: `native/renderer/post_params.h`, `post_tone_stage.{h,cpp}`, `shaders/post_tone.hlsl`, `native/world/post_tone.h`,
  `native/frame/post_tone_stage_test.cpp`, `post_tone_test.cpp`, [Compare-ToneVerify.py](Compare-ToneVerify.py).
- Changed: `Build-GameBootstrap.ps1` (the fxc lines, the unit, the two tests), `model_frame.h`, `scene_pass.{h,cpp}`,
  `scene_pass_frame.cpp`, `13b_prepare_model_frame.inc`, `game_bootstrap_includes.h`, `Run-StartupTrace.ps1` (the two flags).

### Reviews

- Step review (a fresh Opus, high effort, on `7a895f032`): no High, no Medium. It checked the programmes' arithmetic against the
  original's operation for operation (the dropped remap of the adapted exposure included), the stage's D3D12 usage, the frame's
  integration, the tests and the build. Two Lows, both answered: the RESULT section the code cites was not yet committed
  (committed, `1c828fde3`); `post_tone_stage_test` passed with a « skipped » line without the evidence store or the programme
  cache, as its siblings do, which here would drop this pass's proof (the build now fails on a skip of this test; a grading
  draw numbered 0 no longer falls back to the tone-map capture).
- Step review of the witness tool (a fresh Opus, high effort, on `15e2ab977`): no High. One Medium, fixed: on a frame the frame
  graph drives, the check's draw read the lit HDR after the composition pass, undeclared, so a transient lit HDR (a frame
  without the TAA or DLSS) had already been sent home (the check now declares its own pass, `tone_verify`, with that read).
  Two Lows, fixed: the check opened the original's stages through the frame's own latches, so its failure would have switched
  our pass off for the session (it has its own latch now, and drops only itself); its scratch target is never replaced with
  frames in flight.
- Integrity review (`integrity-reviewer`, on `a11f93c10`): no test, tolerance, reference, gate or reader weakened; every number
  of the Conclusion found in a file. Lows, answered: the test's target is now cleared to a sentinel before each draw (a path
  that drew nothing could have been compared with the other's bytes); `post_tone_test` without the executable no longer passes
  in the build; the test's summary line names the chained draws' figure; the one-draw form's numbers are given by sign and
  with their derivation (`lut_counts.txt`), its tone-map output kept; ROADMAP says what is unit-tested and what waits for the
  live comparison.

- Final review (`final-reviewer`, a fresh Opus at xhigh, on `a11f93c10`): no High, no correctness defect on the default path;
  it compared our tone-map programme's listing with the original's operation by operation and re-read the tests' outputs.
  Two Mediums, both in the records, answered: this RESULT said that nothing of these passes came from the game's image any
  more, without naming the data words read at start and the `$Globals` block the automatic exposure still reads (the first
  bullet above now lists what stays); no live validation was scheduled before tranche 2 (the next step, STATE and ROADMAP now
  put the witness slot first). Lows: the witness tool's shared latch (fixed, above); K-0449 and K-0450 scoped in the ledger
  (their live witnesses were on the original's programmes); the one-draw numbers made exact, the alpha's 2,323 bytes cited;
  the driver precondition written under « Uncertainty »; the programmes' listings kept. Recorded, left: `TonemapFrames` and
  `GradingFrames` count a witness frame of the check twice (journal rows only, under the flag).
- Step review of those fixes (a fresh Opus, high effort, on `9d66f6def`): no High, no Medium; the declared pass, the check's own
  latch, the test's sentinel and the build's skip checks found right, no regression of the default path. One Low, recorded and
  left: a check that cannot open the original's stages or make its target drops itself without a journal row naming the cause
  (the reader then fails on the missing pictures; `post-tone-old.flag` opens the same stages through the frame's latches, whose
  failure rows name it).

- Added after the reviews, documentation and evidence only (no code): the measure of the card's 8-bit write above, its ledger
  claim K-0489, and this section's first « Uncertainty » bullet rewritten from it.

### The decision log

The coordinator relayed on 2026-11-13 that Thomas had answered D-0276's reading of the chromatic aberration (in the candidate
made after Elden Ring only) with a 👍 on 2026-11-12 at 12:37. A relayed answer is not written into `decisions/LOG.md` by this
thread; asked for a line, Thomas wrote here on 2026-11-13 at 09:55: « Je confirme ce qui a été dit ». D-0276 carries it.

### The merge and the live comparison (2026-11-13, 11:53 to 12:35)

- `main` had been merged into the branch (the final tree built and reviewed, above); merged into `main` at 11:53 as `cf0af0f3b`
  on the coordinator's go, `Test-Project` passing from the main checkout; the launcher's module rebuilt from `main` (12:05, every
  unit test passing); the start check `start-cf0af0f3b` passed (12:08 to 12:09, restored): the journal's row says
  `"programmes":"ours"`, 374 tone-map frames and 372 with the LUT.
- **The live comparison** (one announced slot, 12:09 to 12:33, the ASUS, offline, no measure, 13 runs, 13 restorations by
  hash; `Run-R7aGi.ps1` with `world-pictures` and `post-tone-verify`, the build `evidence/I-0138/artifacts/build-t1`, whose
  native sources are the merged ones): on each witness frame our pass's picture and the original's programmes' picture drawn
  from the same frame's inputs. **96 pairs, 96 equal, 0 differing bytes**: Firelink Shrine (the LUT `m40_00_CGrading0003`, the
  vignette), the High Wall (`m30_00_CGrading0002`, the vignette), the Undead Settlement (`m31_00_CGrading0001`, the vignette)
  and Anor Londo (neither), each at 1280x720, 1920x1080 and a 1500x840 window (a 1440x810 picture), with the TAA and the
  automatic exposure (88 pairs), and Firelink Shrine with DLSS and Ray Reconstruction (8 pairs). So the proof no longer
  rests on 1280x720 and the unit harness alone: other sizes, the TAA's and DLSS's output as the source, the live factor,
  vignette and LUT, and the frame's plan with the new resources are covered at these four places.
- Evidence: `evidence/I-0138/artifacts/tranche-1-live-verify/` (`compare-taa.txt`, `compare-dlaa.txt`, `series.log`, one pair
  kept, `m31-world-2400-tone-ours.rgba` and `-tone-yebis.rgba`, with a PNG of it); the runs `evidence/I-0066/artifacts/i138-t1v-*`
  (their journals; their raw frames deleted once compared, D-0142); `evidence/I-0066/artifacts/start-cf0af0f3b`.
- Not covered: the other areas (the pass has no area-specific code; their records differ only by values), a cutscene, a
  resize in play, the vignette row switched in the settings page while playing.
- The disk rotation (`Clear-DiskCache -Apply`): not run at this checkpoint, refused by its own guard: another thread's game was running by then (12:36), after
  this slot's « PC libre »; left to the next merge.
- Tranche 2's form: the coordinator relayed on 2026-11-13 (not Thomas's words in this thread) that the sheet's recommendation
  is applied, the bloom and the shafts remade exactly, Thomas informed and free to come back on it.

## Tranche 2: the bloom and the light shafts with our programmes (2026-11-13)

ROADMAP workstream 11, tranche 2 ([TRANCHE-2-READY](TRANCHE-2-READY.md), [PLAN](PLAN.md) §5), by the R pilot's thread (Fable 5.2,
branch `claude/silly-lovelace-0d9bdf`, continued in tranche 1's thread on the coordinator's order of 12:35).

### Conclusion

- **The glare family draws our own programmes** in place of YEBIS's read from the game's image: the bloom (the source chain,
  the bright pass with and without the vignette, the levels' blurs, the 2x draws, the composite with and without the
  luminance gamma, the output), the light shafts (the mask, the radial blurs, the add) and `Alpha_R`.
  `native/renderer/shaders/post_glare.hlsl` holds them as one shape a define with a count (48 programmes: 12 vertex, 36
  pixel), compiled by the build with fxc as tranche 1's (`/T ?s_5_0 /O3 /Gis`), listed by the technique a plan's draw names
  in a generated table (`post_glare_table.h`, 36 techniques) and returned by `post_glare_programs.{h,cpp}`. The draws, their
  targets, samplers and constants stay the plan's (`yebis::BuildGlare`, `BuildShaft`, `AlphaRDraw`: our transcription of the
  library's CPU formulas, unchanged), drawn by `GlareStage` through `PostDraws`, both unchanged.
- **The same instructions as the originals'**: `compare_listings.sh` compiles each shape as the build does and compares its
  listing with the original's (`fxc /dumpbin /Lx`), every instruction with every operand in order, its swizzle or write
  mask, its negation and its absolute value (only the register numbers are set aside; kept, the count is the same): 45 of
  48 are identical. The three
  others differ by forms today's fxc emits otherwise, none of which can change a result: the vignette bright pass has `mad`
  then `mov_sat` where the original has `mad_sat`; the shaft mask has that once, lacks three copies between registers
  (the original samples four components into a scratch register and copies the one it reads; ours writes that one
  directly), and has the two
  operands of one bitwise `and` in the other order; `Alpha_R` writes one component of its sample where the original writes
  four and reads one. The operands of the products, sums, minima and maxima are in the original's order (the source is
  written for it: fxc emits them reversed), so a NaN or a signed zero meets the same operand first. All three draw the
  original's bytes (below).
- **The same bytes as the original**, on the adapter and driver that drew the captures (`10DE 2689`, driver
  `00200000001006B2`): with our programmes, T13 (`post_draws_test`, each captured draw replayed alone) gives 0 differing
  texels on the 24 glare draws of m37 (`i66-reference-1648`), `Alpha_R` at the twelve shaft captures (m37, m31, m50, m45, m40,
  m30) and the shafts' seven draws at m37, m31 and m50; T14 (`glare_stage_test`, the whole plan from the original's scene HDR) gives 228 surfaces of 228 equal to the
  original's at 9 zones (m37, m31, m50, m45, m40, m30 and the three shaft runs), as with the library's programmes.
- **Against the library's programmes on the same device, byte for byte, on the card and on WARP** (the tests require it
  on whatever adapter they run on): each captured draw T13 replays
  (the 24, `Alpha_R`, the shafts' draws), every surface of the 9 zones' plans, the output included (237 of 237, on the card
  and on WARP), and away from the captures: each zone's state at 800x450,
  1501x843, 1680x1050 and 3840x2160 (the shafts appended at the plan's sizes, as the frame does), from two synthetic scenes
  (ramps, noise around the thresholds, negatives, the half-float range's top above the bright pass's 64,000 clamp; then the
  same with infinities, NaN, a denormal and a negative zero on about one texel in 97): 1,944 surfaces of 1,944 in 36 plans, on the card and on WARP; no plan refused by the builder.
- **A defect of `main` found by that test, fixed here**: the composite of an eight-level area without a luminance gamma is
  `tech_MadT8` once the glare's base is 256 wide (the levels are capped by the base width: `yebis_glare.cpp`; the base is
  0.1633 of the width at glare quality 2, the High Wall's and the Undead Settlement's, so from about 1,562 wide, 1600x900
  included; the captures are 1280x720, base 210, where it is `MadT7`). `MadT8` had no pin, so the plan was refused « programmes »: **no bloom and no light shafts natively at the High
  Wall and the Undead Settlement at 1920x1080** (tranche 1's own live runs say so: `i138-t1v-m30-taa-1920x1080` and
  `i138-t1v-m31-taa-1920x1080`, the row `native_yebis_glare` with `"refused":"programmes"`; the 1280x720 and 1440x810 runs
  drew it), and, by the same rule, at every area of eight levels and gamma 1 from that width up, Thomas's 2560x1440 included.
  The depth of field and the motion blur need the glare's plan, so they were not prepared there either (those two runs have
  no `native_yebis_dof` or `native_yebis_motionblur` row).
  The refusal had been there since the glare was written (W10); nothing compared a native picture with the original's
  above 1280x720 in those areas closely enough to see it. With our programmes the plan is drawn. The five techniques of the
  family the pins lacked (`MadT8`; and, for completeness, `MadT3`, `MadT4`, `MadT5` and `MadT1_S14`, which no known record
  names: the world's records have 6 or 8 levels, the cutscenes' are not established) are in the library's technique table
  (`evidence/I-0067/queries/yebis-techniques.tsv`): their programmes were extracted from the executable by its hashes,
  compared with ours (the same instructions, above) and pinned (`yebis_pins.h`, 98 to 103 rows, `observed` false), so the
  comparison with the library's covers them, and the old path, the depth of field's and the motion blur's plans draw them
  too where they name one.
- **So the picture changes where the bloom was missing** (towards the original's), and nowhere else: at the sizes and areas
  where the plan was drawn before, the bytes are the same. Where it was missing, the depth of field and the motion blur
  are prepared again too, as at the other areas and sizes (they hang on the glare's plan). This rests on the offline tests
  until the merge's live slot: before and after stills at the High Wall at 1920x1080, with the original's, are owed to
  Thomas there (D-0268). `post-glare-old.flag` does not bring `main`'s picture back at those sizes: with the five pins the
  library's path draws `MadT8` too; `no-yebis-glare` does (no glare at all, which is what `main` showed there).
- **The frame** (`13b_prepare_model_frame.inc`, `scene_pass_frame.cpp`): our programmes by default (a plan whose technique
  is not one of ours is refused, the row `native_yebis_glare` naming it; none of the plans the builder makes is). Two test tools until YEBIS's removal, as tranche 1's: `post-glare-old.flag` draws the library's
  programmes read from the image (the row `native_yebis_glare` says `"programmes":"ours"` or `"yebis"`);
  `post-glare-verify.flag` keeps ours and, on the witness frames of `world-pictures.flag`, a second stage draws the same plan
  from the same lit HDR with the library's programmes, both outputs written (`<stem>-glare-ours.r11g11b10`,
  `<stem>-glare-yebis.r11g11b10`; [Compare-ToneVerify.py](Compare-ToneVerify.py) reads both kinds of pairs, `--need glare`
  failing when none is found).
- **Departures from the design sheet**:
  1. **The plan's block and technique names are kept this tranche.** The sheet replaced the 4,096-byte block by a compact
     one with named fields and the technique string by a kind and a count. `GlareDraw` is shared with the depth of field's
     and the motion blur's plans, which still draw the library's programmes (tranche 4) and need that block; so our
     programmes read the same block at the same registers (`packoffset`), and the plans' word-for-word tests stay as they
     are. The compact block comes when those two plans move, or with YEBIS's removal.
  2. **The witness check is inside the composition pass**, not a graph pass of its own: the second stage reads the lit HDR
     where the glare reads it, and its surfaces are the stage's own, outside the frame graph, so the copies are recorded
     without a readback pass (`RecordPixels`'s new `tracked` argument). The frame's plan key and passes are unchanged.
     `Alpha_R` is not doubled by the check (it writes the lit HDR itself): the offline tests alone compare it.
  3. The sheet's « synthetic extents » test compares our programmes with the library's (it cannot compare with an
     original's picture, none being captured at those sizes).
  4. The sheet gave `Alpha_R` as `saturate(s d + o)`: the original has no saturation (a plain `mad` into the alpha), and
     ours follows the original.
- **What still comes from the game for these passes**: the depth of field's and the motion blur's programmes (tranche 4);
  the film grain (the executable's 256x256 block, the shafts' mask input) and the glare's data words read at start
  (`GlareImageData`); the block's layout, which is the library's. The pins and `AcquireNativeGlarePrograms` serve the two
  flags, the depth of field, the motion blur and the tests.

### Evidence

- The build of this tree in `evidence/I-0138/artifacts/build-t1`: `post-draws-ours-<n>-<profile>.txt` (T13 with our programmes),
  `glare-stage-ours-<profile>.txt` (T14 with our programmes, the comparison with the library's, the other extents), beside the
  unchanged runs on the library's programmes (`post-draws-*`, `glare-stage-<profile>.txt`) and `yebis-pins.txt`.
- The programmes compared: `evidence/I-0138/artifacts/tranche-2-programmes/` (`extract_pinned.py`, the 55 originals'
  listings, `compare_listings.sh` and its output `compare-listings.txt`: « 45 of 48 the same », the three others' differences
  printed).
- The refusal in `main`: `evidence/I-0066/artifacts/i138-t1v-m30-taa-1920x1080` and `i138-t1v-m31-taa-1920x1080`
  (`startup.jsonl`, the rows `native_yebis_glare`), against `i138-t1v-m30-taa`, `-1500x840` and the m37 and m40 runs.

### Tests (D-0268)

- Changed, stated: `post_draws_test` and `glare_stage_test` take a profile `hardware-ours` or `warp-ours` that draws our
  programmes; the runs on the library's programmes stay, with their assertions and rule unchanged (exactness on the
  capture's adapter and driver, reported elsewhere). No tolerance or reference is changed. A reader is:
  [Compare-ToneVerify.py](Compare-ToneVerify.py) now also reads the glare pairs, and takes `--need tone` and `--need glare`;
  without `--need` its rule is tranche 1's (it fails when no tone pair is found), with it the kinds named must each have
  a pair (the live slot reads its runs with both). A runner too: `Run-StartupTrace.ps1` accepts and cleans the two new
  flags, `post-glare-old` and `post-glare-verify`; nothing else of it changes. In `glare_stage_test` the
  recording became a function of the plan, the scene and the extent (it also serves the synthetic extents), the scene's
  upload takes an aligned row pitch (a width that is not a multiple of 32 texels), the output's surface is read back too (for
  the comparison with the library's only: the capture comparison still stops at the composite, as before), and the library's
  programmes are kept by technique between plans (one scan of the image for new techniques).
- Added: the comparison of our programmes with the library's, byte for byte on the adapter the test runs on, at the zones and at the four
  other extents with the two synthetic scenes, and for each draw T13 replays; the build runs T13 and T14 with our programmes
  on the card and on WARP and fails on a « skipped » line. `yebis_pins_test` now checks 103 techniques (the five added pins found in the executable).
- A plan with a technique that has no pinned original fails `glare_stage_test` (it is drawn by ours alone and named first);
  with the five pins added no plan of the test is in that case.

### Uncertainty / unresolved questions

- No live frame was drawn before the merge (D-0229). The live slot after it covers: the witness pairs of
  `post-glare-verify.flag` at the four areas and three sizes of tranche 1's slot, and the before and after stills at
  1920x1080 where the bloom was refused.
- An eight-level composite has never been compared with an original's picture (no capture above 1280x720): its plan is the
  library's formulas at another count (the same code that gives six and seven levels, exact at the captures) and its
  programme the original's instructions; a capture of the original at 1920x1080 would close it.
- The byte-for-byte proof against the captures holds while this PC's adapter and driver are the captures' (tranche 1's
  note). The equality with the library's programmes is shown on this card and on WARP; on another vendor's driver it is
  not known (ours are shader model 5 with every instruction precise, the originals shader model 4 without: a driver may
  fuse a product and a sum in one and not in the other): the build's tests would say, they run the comparison on the
  adapter they find.
- Where the glare was refused, the depth of field and the motion blur are prepared for the first time at those sizes;
  their own tests draw 1280x720 only. The live slot looks at them at the High Wall and the Undead Settlement at 1920x1080.
- The glare's GPU time is still unmeasured (TRANCHE-2-READY §1); nothing here changes the number of draws.
- The pipelines of our programmes are new to a player's pipeline list: those an area's plan uses compile at the first world
  frame there after the update.
- Glare shape 3, the custom glare, a tinted shape, a bloom gamma other than 1 and the shafts' second composite stay
  refused (PLAN §6), as before.

### Artifacts produced or modified

- New: `native/renderer/shaders/post_glare.hlsl`, `native/renderer/post_glare_programs.{h,cpp}`.
- Changed: `Build-GameBootstrap.ps1` (the 48 fxc lines, the table, the unit, the runs with our programmes),
  `post_draws_test.cpp`, `glare_stage_test.cpp`, `yebis_pins.h` (five rows), `model_frame.h`, `scene_pass.{h,cpp}`,
  `scene_pass_frame.cpp`, `scene_pass_views.cpp` (`RecordPixels`), `13b_prepare_model_frame.inc`, `02_frame_lifecycle.inc`
  (the row `native_glare_verify`), `game_bootstrap_includes.h`, `Run-StartupTrace.ps1` (the two flags), [Compare-ToneVerify.py](Compare-ToneVerify.py).

### Reviews

- Step review (a fresh Opus, high effort, on `564bd337f`): no High, no Medium; the five criteria met. It compiled the 48
  shapes itself and compared them with the originals under a stricter rule than the first `compare_listings.sh` (swizzles,
  masks, negations and absolute values kept, the signatures too), checked the table's vertex programmes against the pins,
  the block's registers against the plans' words, the frame's states and lifetimes, and found the refusal in the two
  1920x1080 journals. Five Lows, all answered:
  1. The comparison script dropped swizzles, masks, signs and bars, so « the same » was weaker than it read: the script
     now keeps them, and the source's products, sums, minima and maxima were reordered to the original's operand order
     (38 of 48 identical under the strict rule before, 45 after; the three left are the Conclusion's).
  2. `Alpha_R` was compared with the capture only, which is exact on this card and driver alone: T13 with our programmes
     now also replays each draw with the pinned original's on the same device and requires equal bytes, on any adapter.
  3. A plan naming a technique without a pinned original passed the test with a printed line: it now fails it.
  4. This section said « `Alpha_R` at three zones » where the build replays it at twelve captures: corrected.
  5. The witness check's second stage dropped its failure reason: the journal now has a row `native_glare_verify` with
     it, once.
- Integrity review (`integrity-reviewer`, on `e55c8d4d3`, with the final tree's build outputs): no weakened test, no special
  case, no number without evidence (every number of the Conclusion found in a file; the five pins' hashes checked against
  the library's table). One Medium, answered: the Tests section said no reader was changed while `Compare-ToneVerify.py`
  was, and its first form passed a run without tone pairs as long as it had glare pairs (the reader's default is tranche
  1's rule again, the section names it and the runner's two flags). Lows, answered: the stored `compare-listings.txt` had
  been made by a form of the script that kept the register numbers, a control character having replaced the word
  boundary of its pattern (repaired and rerun: 45 of 48 either way); this section claimed the comparison of T13 with the
  pinned originals a few minutes before the build that holds it had run (`post-draws-ours-*`: 57 draws of 57 on the card
  and on WARP); the Conclusion now says that the depth of field and the motion blur come back with the glare where it was
  refused, and that `post-glare-old.flag` no longer shows `main`'s picture there.
- Final review (`final-reviewer`, a fresh Opus at xhigh, on `e55c8d4d3`, with the final tree's build outputs): no High, no
  Medium; criteria 1 to 4 met, the records after its Lows. It reran the listings' comparison, checked the five pins against
  the library's table, the frame's states, lifetimes and latches, and the refusal in the two journals. Six Lows, answered:
  1. « Byte for byte on any adapter » was shown on two (this card, WARP): the wording now says so, and other vendors'
     drivers are under « Uncertainty » (our programmes are shader model 5 with every instruction precise, the originals
     shader model 4 without it: a driver may lower the two differently).
  2. The comparison script's register pattern (the integrity review's Low too) and two sentences built on it: the three
     moves the shaft mask lacks are copies between registers, not of a register onto itself.
  3. The refusal's extent was understated (« about 1,680 wide », the test's size): it starts at a base width of 256.
  4. The depth of field and the motion blur come back with the glare where it was refused, untested at those sizes; no
     known record names `MadT3`, `MadT4` or `MadT5`: both said now.
  5. ROADMAP and the investigations index said tranche 2 was a sheet only (updated in the branch); the sheet's formula for
     `Alpha_R` had a saturation the original has not (said under the departures).
  6. Some programmes are compiled and listed that no plan names today (`MadT1_S2`, `S6`, `S8`, `S10`, `S12`, `S14`,
     `MadLumGammaT2`, the vertex programmes for two and six offsets): compared by listing only, never drawn by a test;
     `post_glare_programs.h` says so (kept: the family by count, and tranche 4's depth of field names some). Recorded,
     left: the tests' `PostGlareProgram` repeats the lookup `PostGlarePrograms()` gives.
- After the reviews, no code changed but comments (`yebis_pins.h`, `post_glare_programs.h`); the final tree's build
  (`e55c8d4d3`, 13:38 to 13:51, every unit test passing) stands for it.

### The merge and the live comparison (2026-11-13, 15:03 to 15:45)

- `main` had moved (`983a56d5`, another thread's documentation): merged into the branch (no native change; `Test-Project
  -MergeGate` passing), then the branch merged into `main` at 15:03 as `a651528c6` on the coordinator's go, `Test-Project`
  passing from the main checkout; the launcher's module rebuilt from `main` (15:07 to 15:16, every unit test passing, T13
  and T14 with our programmes among them); the start check `start-a651528c6` passed (15:16 to 15:18, restored): the
  journal's row `native_yebis_glare` says `"programmes":"ours"`, on.
- **The live comparison** (one announced slot, 15:18 to 15:45, the ASUS, offline, no measure, 15 runs, 15 restorations by
  hash; `Run-R7aGi.ps1` with `world-pictures`, `post-tone-verify` and `post-glare-verify`, the build
  `evidence/I-0138/artifacts/build-t1`: the reviewed tree `e55c8d4d3`, which differs from `main`'s native sources by
  comments in two headers): on each witness frame our glare's output and the library's programmes' output drawn by a
  second stage from the same lit HDR, and tranche 1's pair of tone-map pictures. **96 glare pairs, 96 equal, 0 differing
  bytes; 96 tone-map pairs, 96 equal**: the High Wall, the Undead Settlement, Firelink Shrine and Anor Londo, each at
  1280x720, 1920x1080 and a 1500x840 window (a 1440x810 picture), with the TAA (88 and 88 pairs), and Firelink Shrine with
  DLSS and Ray Reconstruction (8 and 8). No `native_glare_verify` failure row, no frame-graph mismatch row in any run.
- **Where the bloom was refused**: at the High Wall and the Undead Settlement at 1920x1080 the row now says on, 39 draws
  at a base 314 wide (the eight-level composite), `"verify":true`, and its pairs are equal: there the library's `MadT8`,
  pinned in this tranche, and ours draw the same bytes live. The depth of field and the motion blur have their rows
  again there (« no level drawn », « motion blur off »: prepared, with nothing to draw at those views, as at the other
  areas).
- **The stills for Thomas** (D-0268): `evidence/I-0138/artifacts/tranche-2-stills/`: at the High Wall and the Undead
  Settlement at 1920x1080, three looks each, the window before (tranche 1's runs of `main` without the fix), after
  (this slot's) and the original game's (`Run-R7aGi`'s `orig` mode, the same save and looks);
  `sheet-before-after-original.jpg` holds one look of each area. Measured, not judged by eye (ffmpeg `signalstats`, the
  mean luma of a 1900x600 crop under the title bar): after is 0.7 to 4.3 levels of 255 above before at the six looks (the
  bloom's haze, strongest by the Settlement's fire and windows: 40.0 to 44.3). The original's picture differs from both
  by more than that, and in both directions (the High Wall 68.0 against 53.9 at one look, the Settlement 35.7 against
  44.3 at another): that is the traced lighting's difference the game tour filed for workstream 7 (I-0139), not the
  bloom's, so these stills show the bloom back, not a match with the original.
- One run of the 15 (`i138-t2v-m30-taa-1500x840`) lost the foreground to the desktop before its looks (the known pitfall of
  a small window, LIVE-RUN-NOTES): its camera did not turn, its 8 pairs were still written and are equal.
- Evidence: `evidence/I-0138/artifacts/tranche-2-live-verify/` (`compare-taa.txt`, `compare-dlaa.txt`, `series.log`, one
  pair kept, `m30-1920x1080-world-3000-glare-ours.r11g11b10` and `-glare-yebis.r11g11b10`); the runs
  `evidence/I-0066/artifacts/i138-t2v-*` (their journals and window pictures; their raw frames deleted once compared,
  D-0142); `evidence/I-0066/artifacts/start-a651528c6`.
- Not covered: the other areas; 2560x1440 (Thomas's size: the same plan rule, a base 419 wide, untried live); a cutscene;
  an original's capture of an eight-level composite (the « Uncertainty » bullet stands).
- The disk rotation (`Clear-DiskCache -Apply`, owed since tranche 1's checkpoint): run at this one (2,233 files, 1.7 GB of
  old builds; 330 GB free on C:).

## Tranche 3: the exposure (2026-11-13)

ROADMAP workstream 11, tranche 3 ([TRANCHE-3-READY](TRANCHE-3-READY.md), [PLAN](PLAN.md) §5), by the R pilot's thread (Fable 5.2,
branch `claude/silly-lovelace-0d9bdf`, continued in the same thread on the coordinator's order of 15:49: the fresh thread's
card waits for Thomas).

### Step 1: the automatic exposure on our tone-map block

- **The automatic exposure reads our tone-map pass's parameter block** (`PostToneParams`, the 160 bytes the pass itself
  binds) in place of the YEBIS tone map's `$Globals` (3,584 bytes): `AutoExposureStage::Record` takes the block, its
  kernels' b0 is declared as it (`auto_exposure_shader.h`), and the five values the solver reads (the exposure, the
  curve's two constants, the colour matrix's rows, 1 / gamma) keep their expressions. The CPU mirror
  (`auto_exposure_math.h`) reads the block too. `yebis::TonemapGlobals` is now built only for `post-tone-old.flag` and
  `post-tone-verify.flag`: on the default path nothing of the tone map or of the exposure binds a `$Globals` block any more.
- **The same factor, bit for bit**: a tool, `exposure_bits`, built against the stage before and after the change, runs it
  on four captured scene HDRs (m37, m31, m40, m30; 1280x720), two sets of tone-map values (Firelink's with the identity
  matrix; another exposure, gamma and curve with a full matrix), two strengths, a reset then nine frames, then the target
  moved and nine adapting frames, and prints the reading's log2 factor, goal and mean, the pixels counted, the factor
  texel and the exposure DLSS reads, as bits: the 32 lines are the same text before and after, on the card and on WARP
  (the card's and WARP's own lines differ from each other, as before).
- The values in the block are the ones the `$Globals` held: both are laid out from the same `yebis::TonemapConstants`
  (`BuildPostTone`, `TonemapGlobals`: the exposure after the guard, the curve, the matrix, the gamma correction), and
  tranche 1's `post_tone_test` checks `BuildPostTone`'s fields against the original's captured words 616, 808, 832, 833
  and 840 to 851, bit for bit (`exposure_bits` fills both forms by hand: it proves the kernels, not the builder).
- Nothing changes on screen, and nothing is new for Thomas's eye in this step.

### Evidence

- `evidence/I-0138/artifacts/tranche-3-exposure-bits/`: `exposure_bits.cpp` (one source, `/D OLD_GLOBALS` for the stage
  before), its two build scripts, the two binaries (`exposure_bits_old.exe`, SHA-256 `ca9b7721…f5775e`, and
  `exposure_bits_new.exe`), `old-hardware.txt` and `new-hardware.txt` (equal), `old-warp.txt` and `new-warp.txt` (equal).
  The « before » side was built at `278fc66f0^` (the tool's old form no longer compiles against the tree's header) and
  linked with the launcher folder's `auto_exposure.obj` of that moment, an object of `main`'s unchanged stage (it holds
  the old kernel's `float4 g[224]`); that object has been rewritten since by a later rebuild of the same source, which is
  why the binary is kept. The two binaries in the folder are the ones that wrote the four outputs (copied there after
  the runs).
- `auto_exposure_test` and `auto_exposure_stage_test` (the kernels on WARP against the CPU mirror) pass on the block: the
  build's `auto-exposure.txt` and `auto-exposure-stage.txt` (`evidence/I-0138/artifacts/build-t1`: the reviewed tree
  `520a6af4c` built 16:28 to 16:41, then the final tree `31a117d34`, 16:43 to 16:55, every unit test passing both times).

### Tests (D-0268)

- Changed, stated: `auto_exposure_test` and `auto_exposure_stage_test` build their tone-map values as a `PostToneParams`
  where they wrote words of a `$Globals` block (the stage's interface changed); the same values, the same assertions and
  tolerances. The first test's sentence « the tone map's words read from the $Globals block » became « read from our
  parameter block ».
- Changed, stated: `Run-StartupTrace.ps1` accepts and cleans one more flag, `auto-exposure-er-bounds`;
  `Build-GameBootstrap.ps1` links `auto_exposure_test` with `dxguid.lib` (the reflection check's interface identifier).
- Removed, stated: the frame's `static_assert` that the exposure stage's block size was the tone map's `$Globals` size
  (I-0132 R11h's final review L3). The stage no longer reads that block; the reflection check below replaces it and is
  stronger (the size and five offsets against `PostToneParams`).
- Not added to the build: `exposure_bits` compares two builds of the stage, the earlier of which no longer exists in the
  tree; it is kept with its outputs as evidence.
- Added (the step review's L1): `auto_exposure_test` reflects the kernels' `Tone` block and requires `PostToneParams`'s
  size and the offsets of the five values the solver reads, so a field moved in `post_params.h` fails the build here as
  it does in `post_tone_test`.

### Step 2: Elden Ring's bounds behind a test flag; the bloom's question withdrawn

- **`auto-exposure-er-bounds.flag`: the full adaptation within bounds read from Elden Ring**, in place of our half
  strength: the log2 factor held in `[-0.5, 1.5]` (brightened by 1.5 stops at most, darkened by 0.5 at most). The reading:
  Elden Ring ships Adaption Min -1.5 and Adaption Max 0.4 to 0.5 (ER-VALUES); its code soft-clamps the adapted luminance's
  offset between two limits and multiplies the exposure by `2^(-offset)` ([ER-TONEMAP-HDR](ER-TONEMAP-HDR.md) §3: the code
  observed, the two limits' roles inferred). The flag's first form had the interval the other way round, `[-1.5, 0.5]`
  (the final review's Medium: corrected). It is « ours with bounds after Elden Ring's », not Elden Ring's pass: our solver
  and target, a hard clamp where Elden Ring's is soft, around the area's exposure where Elden Ring's is its Middle Gray,
  and our speeds (the unit of Elden Ring's adapt speeds is not read). Off by default: nothing changes on screen; it is
  there for a sheet beside our half strength, for Thomas, labelled as this reading.
- **The question « does the bloom follow the adapted exposure? » is withdrawn: it rested on an error of this thread.**
  [TRANCHE-3-READY](TRANCHE-3-READY.md) said, labelled verified, that the adapted factor multiplies the scene in the tone
  map only, so that the bloom did not follow it. It does: the tone map multiplies the glare by the same factor as the scene
  (`post_tone.hlsl`: `gl = saturate((glare x k) x glare_scale)`, written in tranche 1 from the original's programme, whose
  Offset words make the factor « the scene's and the glare's »). A candidate had been written on that error
  (`bloom-adapted.flag`: the bright pass multiplied by the factor, commit `95098eb21`, never merged); its step review
  found that it applied the factor twice to the bloom (High). It is removed, with its programmes, its stage input and its
  test; the recommendation sent to the coordinator was corrected the same hour (2026-11-13). What is left of the question
  is small: the bloom's threshold sees the scene before the adaptation, not after; that belongs to the candidate made after
  Elden Ring (tranche 6), which applies the exposure at the bloom's source and not at its add.

### Reviews

- Step review of step 1 (a fresh Opus, high effort, on `278fc66f0`): the five criteria met, no High, no Medium. It checked
  the kernels' block against `PostToneParams` offset by offset, traced the five values from `yebis::TonemapConstants`
  through both builders, found no reader of the `$Globals` on a path where it is no longer filled, and judged the bits
  tool's second set of values able to show a swapped or shifted field. Three Lows, answered: nothing checked the kernels'
  block against `PostToneParams` (the reflection check in `auto_exposure_test`); the evidence's « before » side could not be
  rebuilt (the binary kept, the Evidence bullet says how it was made); the two tests' passing runs had no kept output (the
  build's `auto-exposure.txt` and `auto-exposure-stage.txt`).
- Step review of step 2 (a fresh Opus, high effort, on `95098eb21`): one High, the one above (the factor applied twice
  with `bloom-adapted.flag`, and this section's premise false): answered by the candidate's removal. It found the frame
  unchanged without a flag, the bounds flag as stated, and the reflection check correct. Its Lows concerned the removed
  candidate (the latch's cascade, the test's coverage) and the evidence of the new checks, which the final tree's build
  holds.
- Integrity review (`integrity-reviewer`, on `520a6af4c`): no test, bound or tolerance loosened, the candidate's addition
  and removal left no trace in the tests, the bits tool's two binaries hold the old and the new kernel as said. Findings,
  all in the record and answered: the runner's new flag, the test's link line and the removed `static_assert` were not
  stated under « Tests » (stated now); the build outputs cited were, at its reading, an earlier tree's (the final build
  rewrote them: 16:39, both passing, the reflection check among them); the block's values equal to the `$Globals`' rested
  on reading where `post_tone_test` proves it (cited); a leftover sentence still named the withdrawn question (removed).
- Final review (`final-reviewer`, a fresh Opus at xhigh, on `520a6af4c`): no High; criteria 1 to 4 met (the five values
  traced end to end, the kernel's diff limited to the block and five names, no reader of an unfilled block on any path, no
  leftover of the removed candidate in the tree or in the built module). Two Mediums, answered: the bounds flag's interval
  was probably the reverse of Elden Ring's (corrected to `[-0.5, 1.5]` from ER-TONEMAP-HDR §3, and labelled an inferred
  reading); the leftover sentence above. Lows, answered: the block's size is now asserted against the 256 bytes the stage
  asks; the removed assertion stated; an unused include removed. Recorded, left: `NativeTonemapInput::globals` (3,584
  bytes, zeroed in every frame's input, read by the two test flags only) goes with YEBIS in tranche 8; the CPU mirror's
  `ToneWords` is now a copy of the block's fields.
- After the reviews: the bounds' two constants and their comment, a `static_assert`, an include; the tree rebuilt
  (`31a117d34`, every unit test passing).

### The merge and the sheet (2026-11-13, 16:56 to 17:30)

- Merged into `main` at 16:56 as `020685cb9` on the coordinator's go, `Test-Project` passing from the main checkout; the
  launcher's module rebuilt from `main` (16:58 to 17:08, every unit test passing); the start check `start-020685cb9` passed
  (17:08 to 17:10, restored): the automatic exposure's rows valid on our block (Firelink: log2 factor -0.46 at a mean
  displayed grey of 0.23, the area's exposure 2.3, 921,600 pixels).
- **The sheet for Thomas** (one announced slot, 17:10 to 17:27, the ASUS, offline, no measure, 10 runs, 10 restorations
  by hash; `Run-R7aGi.ps1`, the build `evidence/I-0138/artifacts/build-t1`, the final tree): five views at 1920x1080,
  each with our half strength (the default) and with `auto-exposure-er-bounds.flag`. The settled log2 factor, from each
  run's last `native_auto_exposure` row:

  | View | Half strength (default) | Bounds read from Elden Ring |
  |---|---|---|
  | Firelink Shrine, outdoors | -0.44 | -0.50 (at the bound) |
  | The High Wall | -0.40 | -0.50 (at the bound) |
  | The Undead Settlement | -0.22 | -0.43 |
  | Anor Londo's staircase hall | -0.20 | -0.38 |
  | Firelink's unlit lower corridor | +1.07 | +1.50 (at the bound) |

  So with the bounds the bright views are darkened a little more (up to 0.2 stop) and the dark corridor is brightened
  0.4 stop more; with the half strength, a dark place stays darker. In the stills the corridor's mean luma goes from 37 to
  47 of 255 (a 1900x600 crop, ffmpeg `signalstats`); at the outdoor looks the two stills differ by less than 3 levels,
  which a still taken a moment after a camera turn does not resolve: the table is the measure there, not the stills.
- Evidence: `evidence/I-0138/artifacts/tranche-3-sheets/` (`sheet-half-vs-erbounds.jpg`: the High Wall, Anor Londo's
  hall and the corridor, half strength left, bounds right; the 30 stills; `series.log`); the runs
  `evidence/I-0066/artifacts/i138-t3h-*` and `i138-t3e-*` (their journals and window pictures; raw frames deleted,
  D-0142); `evidence/I-0066/artifacts/start-020685cb9`.
- Thomas's choice (2026-11-13 18:57, tranche 4's thread, on the pilot's recommendation; D-0276's entry): **the half
  strength stays**; the bounds read from Elden Ring are not retained now, and an exposure after Elden Ring's is judged
  again at tranche 6 with the Rekindled colorimetry's curve. The test flag `auto-exposure-er-bounds.flag` is removed with
  tranche 4's part 4a (the frame's three lines, the runner's pattern and cleaning list).

## Tranche 4, part 4a: the depth of field and the motion blur with our programmes (2026-11-13)

ROADMAP workstream 11, tranche 4 ([TRANCHE-4-READY](TRANCHE-4-READY.md): three parts, each merged alone), part 4a, by a fresh R
thread (Fable 5.2, branch `claude/objective-curie-3a46f9`), from the earlier thread's unfinished draft
(`evidence/I-0138/artifacts/tranche-4-programmes/draft-4a.patch`: the depth of field's shapes, 10 of 46 compared).

### Conclusion

- **The depth of field and the motion blur draw our own programmes** in place of YEBIS's read from the game's image, as the
  glare family since tranche 2: `post_glare.hlsl` gains the depth factor, the front blur's blend factor, the composite
  as one shape with its two counts (40 programmes), the taps at 16, 20 and 25, the camera velocity and the blur along the
  velocity at 2 to 8 taps (53 pixel programmes more; the vertex programmes were tranche 2's). The plans, their targets,
  samplers and constants are unchanged (`yebis::BuildDof`, `BuildMotionBlur`), drawn by the same `GlareStage`.
- **Compared with the originals' listings by a new reader, [Compare-Programmes.py](Compare-Programmes.py)**, which does
  two things. First tranche 2's strict rule on the listings (every instruction with every operand in order, the
  register numbers set aside: alone it cannot tell which register is which, so it counts only with the second). Then,
  always, the values: the inputs' and outputs' declarations must be the same, and each listing is evaluated symbolically,
  component by component, into the expression each output component holds (the operations, their operands in the
  instruction's order, the constants' registers and components, the samples with their coordinates, the literals' bits),
  so that two programmes are « the same values » whatever the order the compiler scheduled the instructions in, the
  temporaries it chose and the components of a sample it wrote without reading. Reason for the second rule: the
  original's compiler scheduled the composite's 40 counts and the blur's 7 each its own way (the samples' order, the
  registers), so no single source gives 47 identical listings, while the values are what the draw computes. Of the 57
  programmes compared (the 53 new ones and the four vertex programmes they are drawn with): **16 the same listing, 30 the
  same values with every operand in the same order, 11 the same values with commuted operands, none different**
  (`evidence/I-0138/artifacts/tranche-4-programmes/compare-programmes.txt`).
  - The 11 are composites (the four counts without a front level, `TS1_0` to `TS4_0`, and the seven others with four back
    levels): in each, one or two differences `level - colour` have their two operands in the other order (52 component
    additions in all, listed by the reader). The original's compiler ordered them by its own order of the samples, which
    today's does not follow from the source: two source forms were tried and neither changed it (the difference written
    `-c + level`: `reader-checks/mut6`, the same verdicts; the samples declared in the original's order: not kept). An addition's result does not
    depend on its operands' order for any pair of numbers, signed zeros and infinities included; only which NaN comes out
    of two NaNs could. The stage test below compares all forty counts' bytes with the original programmes' on inputs that
    hold NaNs.
  - The reader's own checks: it reported a literal typed wrongly in the camera velocity (`0x3a7fffc4` for `0x3a7fffd5`)
    as different, and the blur's first forms, whose operands were in another order, as commuted (both met while
    writing, then made again for the record: `reader-checks/mut4` and `mut5`); a `mad` then `mov_sat`
    is read as the original's `mad_sat`, and the operands of a bitwise `and` or `or` (the comparisons' masks) are put in
    one order always (theirs cannot change a bit). It evaluates a closed list of instructions (those of these
    programmes); another one, or a temporary read before it is written, counts as different. The step review's two
    mutated sources (the composite's scene sampled at the levels' coordinate; the taps' first two coordinates
    exchanged), which the listings' rule alone passed, are reported different (`reader-checks/` in the evidence
    folder: 40 and 3 programmes).
- **The motion blur's plan served a tap count of 5 only; it now serves 2 to 8** (the library's table). It refused a base
  count (`NumBaseSamples` with the tap parity) or a last pass's count other than 5 as « unpinned technique ». **A latent
  refusal, not a defect seen in the world** (this record's first form said the blur was refused at some sizes when the
  option is on: wrong, the final review's M1): with the values every world record holds (`NumBaseSamples` 4 both ways,
  so 5 base taps; `MaxBlurLength` 0.1; `MaxRecurrences` 2, CENSUS-WORLD) the recurrence cap is taken at every picture
  height above 240, and a capped plan draws the base count in every pass: 5 (`yebis::PlanMotionBlur`;
  `yebis_motionblur_test` now checks it from 360 to 4320 high, for a cap of 1 and of 2); the 2,394 served
  `native_yebis_motionblur` rows of the evidence store's journals all say 5 taps (the final review's count). The only
  world record with another base count (m32_00_0100, 3) is refused earlier for its glare shape. Another count is
  reachable only by values no world record holds: the cutscenes' three MotionBlur records, not read, or a test's. The
  six other techniques (`tech_MotionBlurVelocityDirDepthInnerscreen_SM3_T2_S2` to `S8`) are pinned from the library's
  table (`yebis_pins.h`, 103 to 109 rows, `observed` false; `mb-table.tsv`) and the two refusals now bound the count to
  the table's 2 to 8. The taps' places, read on the seven listings (the reader's walk of each sum): an odd count takes
  -1 to -(n - 1) / 2 then +1 to +(n - 1) / 2 steps, an even count +1 to n - 1, summed in that order. A count other than
  5 rests on the plan's formulas at another count and on the programme's equality with the library's, not on a picture
  of the original (none exists).
- **The frame** (`13b_prepare_model_frame.inc`, `06_pass_programmes.inc`, `scene_pass_frame.cpp`): one helper,
  `ChooseNativePostProgrammes`, gives the glare's, the depth of field's and the motion blur's plans our programmes (a plan
  naming a technique that is not one of ours is refused, its row naming it) and holds tranche 2's two test flags, which
  now cover the three: `post-glare-old.flag` draws the library's programmes (the rows `native_yebis_dof` and
  `native_yebis_motionblur` say `"programmes":"ours"` or `"yebis"`); `post-glare-verify.flag` keeps ours and, on the
  witness frames, a second stage draws each plan with the library's from the same inputs, both outputs written
  (`<stem>-dof-ours.rgba16f` and `-dof-yebis.rgba16f`, `<stem>-mb-ours.rgba16f` and `-mb-yebis.rgba16f`, on the frames that
  draw them; [Compare-ToneVerify.py](Compare-ToneVerify.py) reads them, `--need dof`, `--need mb`). The depth factor's draw
  is not doubled (it writes the scene's alpha itself, as `Alpha_R`). A failure of a second stage is the row
  `native_glare_verify`, its reason now starting with the stage's name.
- **Not written**: the depth factor's second form (`..._TypeCastLutT1_S1_RVALUE`, which also copies a colour; pinned and
  observed in the captures). No plan names it (the builder names the plain form only) and the stage has no draw for
  it: a first shape of it gave the original's values under the reader and was removed, a programme no test can draw
  having no place in the build.
- **Tranche 3's test flag removed**: `auto-exposure-er-bounds.flag` (the frame's three lines, the runner's pattern and
  cleaning list), Thomas having kept the half strength (D-0276's entry). The solver's bounds it set are the stage's own
  parameters, older than it, and stay.
- **What still comes from the game for these passes**: the block's layout, which is the library's; the executable's words
  the plans' builders read (`DofImageData`, the film grain); the pins and `AcquireNativeGlarePrograms`, which serve the two
  flags and the tests. Nothing of the default path reads a YEBIS programme from the image any more for the glare, the
  shafts, the depth of field, the motion blur or the tone map.

### Tests (D-0268)

- **The same bytes as the original's captured draws**, on the adapter and driver that drew the captures: `dof_stage_test`
  with our programmes gives 50 surfaces of 50 equal to the original's at the five zones (m37, m31, m50, m40, m30: the
  depth factor in the scene's alpha and the nine other draws each), as with the library's programmes. `mb_stage_test`
  with our programmes passes the test's unchanged rule at the six runs (the velocity surface within one half-float unit
  of the original's in red and green, the blur passes chained after it differing on at most 0.1 % of the texels: the
  rule the library's programmes are held to, the D3D11 against the D3D12 compilation of the velocity programme, MB-0).
- **Against the library's programmes on the same device, byte for byte** (a second stage draws each plan with the pinned
  originals): at the captures' inputs, every surface of the depth of field's five plans (50 of 50) and of the motion
  blur's six (18 of 18), on the card and on WARP.
- **Away from the captures** (the comparison with the library's alone: no original picture exists there):
  - the depth of field: each zone's camera and globals at 800x450, 1501x843, 1680x1050 and 3840x2160, from two synthetic
    scenes (tranche 2's: ramps, noise, negatives, the half-float range's top; then the same with infinities, NaNs, a
    denormal and a negative zero on about one texel in 97) and a synthetic depth: 360 surfaces of 360 in 18 plans, on
    the card and on WARP; two plans are refused by the builder at 800x450 (m50 and m40: « a level smaller than its
    surface: no witness », a refusal of the plan itself, older than this tranche);
  - **the composite's forty counts and the taps at 16, 20 and 25**, which no captured plan and no plan of the extents
    above names: plans made for the comparison (`CompositePlan` in the test: each level a taps draw of the scene, the
    composite over them with arbitrary remaps, its second coordinate a little off the first; not plans the builder
    makes), two synthetic scenes each: 400 surfaces of
    400 in 40 plans, 47 techniques, on the card and on WARP. The eleven composites whose listings have commuted operands
    are among them, on the scene that holds NaNs: the same bytes;
  - the motion blur: the turning camera's run at 800x450, 1501x843 and 1680x1050, every base count 2 to 8, both tap
    parities, three blur lengths (38 distinct plans; 9 combinations refused by the builder: a base count of 9, outside
    the library's table), two synthetic scenes, a synthetic depth and object velocity; every count 2 to 8 drawn. **On the
    card, 228 surfaces of 228 byte for byte. On WARP, 164 byte for byte and 64 that differ only by which NaN a texel
    holds**, all on the scene of special values, all in the blur passes, none in the velocity surface (the run of the
    test before its rule below, which printed each differing surface with its texels of each kind:
    `nan-classification/` in the programmes' evidence folder; the test now prints their count alone).
- **A rule of the new test, stated**: on the special values' scene alone, `mb_stage_test` counts apart, and does not
  fail on, a surface whose every differing half-float is a NaN in both (any sign and payload); a difference of any other
  kind fails it, on the plain scene and at the captures the bytes must be equal, and on the adapter and driver that
  drew the captures no such surface is accepted at all (the card gives equal bytes). Reason: which NaN comes out of an
  operation on two is the adapter's choice for each compilation, and WARP gives another NaN than with the library's
  programmes at some texels (0 texels differ otherwise; the card gives the same bytes). The likely cause, not isolated
  by a run: our programmes are compiled IEEE-strict (every instruction `precise`), the originals not, and their
  registers are allocated otherwise. The depth of field's and the glare's tests have no such rule and need
  none.
- Changed, stated: `dof_stage_test` and `mb_stage_test` take a profile `hardware-ours` or `warp-ours`; their runs on the
  library's programmes stay, with the same assertions and messages (the recording became a function of the plan and its
  inputs, used by both profiles; the library's programmes are kept by technique between plans). `glare_stage_test`'s
  synthetic scene moved, unchanged, to `synthetic_scene.h` (shared with the two tests, with a synthetic depth and
  object velocity), and the three tests' search of the library's programmes by their pins, with the lookup of ours, to
  one copy, `post_test_programmes.h` (the glare test's now also resolves a plan's pre draw, `Alpha_R`; its assertions
  unchanged). `yebis_motionblur_test` gains the planner at the world's values (20 checks: 5 taps at ten heights, two
  caps). `dof_capture.h`'s `Native` takes an optional extent. `yebis_pins_test` now checks 109 techniques
  (the six pins added). The build runs the two tests with our programmes on the card and on WARP and fails on a
  « skipped » line. [Compare-ToneVerify.py](Compare-ToneVerify.py) also reads the depth of field's and the motion blur's
  pairs (`--need dof`, `--need mb`; its rule for the tone and glare pairs unchanged).
- A plan naming a technique without a pinned original fails both tests; none does.

### Evidence

- The build of this tree in `evidence/I-0138/artifacts/build-t4`: `dof-stage-ours-<profile>.txt`,
  `mb-stage-ours-<profile>.txt` beside the unchanged runs (`dof-stage-<profile>.txt`, `mb-stage-<profile>.txt`),
  `yebis-pins.txt`, the glare's and the tone map's as before.
- The programmes compared: `evidence/I-0138/artifacts/tranche-4-programmes/` (the 57 originals' listings,
  `extract_pinned.py`, `mb-table.tsv`, `compare-programmes.txt`, `compare-programmes-glare.txt`; `reader-checks/`, five
  mutated sources with the reader's output on each; `nan-classification/`, the motion blur test's run on WARP before
  its NaN rule; the earlier thread's `draft-4a.patch`, `compare_listings.sh` and `compare-listings.txt` as it left
  them).

### Uncertainty / unresolved questions

- No live frame was drawn before the merge (D-0229). The live slot after it: the witness pairs of
  `post-glare-verify.flag` with the depth of field's and, the motion blur option on and the camera turning, the motion
  blur's (five taps: the world's only count).
- A motion blur at a tap count other than 5 has never been compared with an original's picture, and no world record
  asks for one (above): nothing changes on screen for it. A reference would need a cutscene's record that asks another
  count (its three MotionBlur records are not read) or a test's values in a Reference run.
- The byte-for-byte equality with the library's programmes is shown on this card and on WARP (the motion blur on WARP
  but for which NaN); another vendor's driver is not known (tranche 2's note).
- The depth of field's plan is refused at 800x450 at two of the five zones' states (a level smaller than its surface):
  a small window draws no depth of field there. Not looked at further here; it belongs with part 4b's work on the
  builder.
- The pipelines of the new programmes are new to a player's pipeline list (tranche 2's note).

### Artifacts produced or modified

- New: [Compare-Programmes.py](Compare-Programmes.py), `native/frame/synthetic_scene.h`,
  `native/frame/post_test_programmes.h`.
- Changed: `native/renderer/shaders/post_glare.hlsl`, `Build-GameBootstrap.ps1` (the 53 fxc lines and rows, the two tests'
  link lines and runs), `yebis_pins.h` (six rows), `yebis_motionblur.cpp` (the two refusals), `06_pass_programmes.inc`,
  `13b_prepare_model_frame.inc`, `model_frame.h`, `scene_pass.{h,cpp}`, `scene_pass_frame.cpp`, `dof_stage_test.cpp`,
  `mb_stage_test.cpp`, `glare_stage_test.cpp`, `dof_capture.h`, [Compare-ToneVerify.py](Compare-ToneVerify.py),
  `Run-StartupTrace.ps1` (tranche 3's flag removed).

### Reviews

- Step review (a fresh Opus, high effort, on `33c0c6d4c`): no High; criteria 1, 3, 4 and 5 met, 2 partly, 6 but for two
  wordings. It read the camera velocity, the blur at 3 and 4 taps, two composites and the two factors against the
  listings, the block's registers against the plans' words, the frame's states and lifetimes, ran the reader on three
  mutated sources and its evaluation on all 57 programmes, and found the build's figures in the logs. One Medium,
  answered: **the reader's « listing » verdict ignored which register is which** (two of its mutations passed it: the
  composite's scene sampled at the other coordinate, two taps' coordinates exchanged) and skipped the evaluation; the
  16 programmes concerned all pass the evaluation, so nothing shipped was wrong, but the verdict was weaker than it
  read. The reader now always evaluates, compares the inputs' and outputs' declarations, and reports both mutations
  different. The same gap was in tranche 2's `compare_listings.sh` (its 45 « identical » listings): the reader was run
  over the glare family too (`--glare`: 45 the same listing and the same values, the 3 others the same values with
  every operand in the same order, none different; `compare-programmes-glare.txt`), which closes it there. Lows,
  answered: the evaluation raised on nothing it did not know (a closed list of
  instructions now, a plain `ret` alone dropped, an unwritten temporary an error); the composite's two coordinates
  were equal in every tested plan, so the stage tests could not see them exchanged (`CompositePlan`'s second coordinate
  is now a little off the first); ROADMAP said « the same bytes as the original's captured draws » of the motion blur
  too (corrected: within its test's unchanged rule); `dof_capture.h` took the height's override on the width's test.
  Recorded, left: with `post-glare-verify.flag` the depth of field's second stage lays out the plan's pre draw it never
  draws (a table, eight upload buffers, a pipeline: a test tool's waste); the motion blur's constants at a tap count
  other than 5 have no reference from the original (« Not established » above: a capture of the original whose last
  pass draws 3 taps would give one).
- Integrity review (`integrity-reviewer`, on `8b6b7ddeb`, with the final tree's build outputs): no High, no Medium. The
  six stated changes (the NaN rule, the two tests' restructuring, the two refusals' bounds, the reader, the flag's
  removal, the synthetic scene's move) are as stated and no wider: every `Require` of `main`'s two tests is kept, the
  runs on the library's programmes print the same figures as before, the NaN rule is reached from the special values'
  scene alone and only when every differing half is a NaN on both sides; no special case in production; every number
  it sampled found in the logs. Three Lows, all on this record, answered: the 64 surfaces' kind was stated as observed
  where the final logs hold their count alone (the run before the rule is kept: `nan-classification/`); the NaN
  difference's cause was stated as a fact (now « likely, not isolated »); three of the reader's checks had no kept
  output (two made again, `mut4` and `mut5`; of the two source forms tried one is kept, `mut6`, the other said not
  kept).
- Final review (`final-reviewer`, a fresh Opus at xhigh, on `8b6b7ddeb` and the commit after it, with the final tree's
  build outputs): no High; criteria 1 to 4 met (it reran the reader on both families, checked every register the
  programmes read against the builders' words, the vertex programmes' rows and the six pins against the library's
  table, the second stages' states, lifetimes and latch, and found the build's binaries to hold the final sources),
  the records after its findings. **One Medium, answered: this record, ROADMAP and the sheet said the motion blur
  « was refused at some sizes » when the option is on, a defect like tranche 2's missing bloom, and promised a visible
  change. The game's data contradict it**: every world record takes the recurrence cap at every real height and
  draws 5 taps, and no journal row was ever refused for its tap count. The pilot had read the planner without its cap
  and said the same to Thomas in the thread; corrected there the same hour, in the three records, and held by a unit
  test (`yebis_motionblur_test`: the planner at the world's values). Lows, answered: two stale comments
  (`post_glare_programs.h` still said the two passes took the library's programmes; `post_glare.hlsl` named the old
  comparison script); the tests' programme search copied in three files (one header now); the NaN rule also applied
  on the capture's card (no such surface accepted there now); the records' hygiene (the index row, the lanes' table,
  the ledger, a link to a paragraph not yet written: done with the merge's checkpoint). Recorded, left: a depth of
  field plan with more than four back levels would name a composite that does not exist (`TS5_x` to `TS7_x`: the frame
  builds seven levels and the count's clamp is not transcribed; older than this branch, no witness reaches it; open
  for part 4b in the sheet); the depth of field refused at 800x450 at two zones (« Uncertainty »); with the verify
  flag, each witness frame reads back four more full-size RGBA16F pictures (about 66 MB each at 3840x2160: a test
  tool's cost).
- Step review of the reviews' fixes (a fresh Opus, high effort, on `51f868eb6`'s native changes): the four criteria met
  (the shared header a line-for-line move, every call site and assertion kept; the capture's-adapter rule a tightening
  only; the planner's checks able to fail; the comments true), no High, no Medium. Three Lows, answered:
  `yebis_motionblur_test` returned « skipped » before its checks that need no capture when none was named (the early
  return removed: the matrix and planner checks always run); a comment's threshold (under a height of 241 the cap is
  not taken, and the odd parity still gives 5 taps down to a spread of 15); a `using` line under another's comment.
- The build that stands for the tree: `51f868eb6`'s (20:05 to 20:22, every unit test passing: the figures above). After
  it, the three answers just above only (two comments and the removed early return of one test): not rebuilt on the
  branch; the launcher's rebuild from `main` after the merge compiles and runs them (« The merge » below).

### The merge and the live comparison (2026-11-13, 20:29 to 21:06)

- The optimisation pass, read: the three post plans take their programmes through one helper where the glare's choice
  was inline and the two others took the library's; the tests' programme search is one header; nothing is read or
  made twice. `main` (I-0139, the game tour: one native file of test tools) merged into the branch before the reviews;
  `Test-Project -MergeGate` passing; the branch merged into `main` at 20:29 as `080479eab` on the coordinator's go,
  `Test-Project` passing from the main checkout; the launcher's module rebuilt from `main` (20:30 to 20:44, every unit
  test passing, the planner's checks without their early return among them: `yebis motion blur: 1068 checks`); the start
  check `start-080479eab` passed (20:50 to 20:51, 350 world frames, restored; the automatic exposure's row valid:
  log2 factor -0.46).
- **The live comparison** (one announced slot, 20:51 to 21:06, the ASUS, offline, no measure, 12 runs, 12 restorations
  by hash; `Run-R7aGi.ps1` with `world-pictures`, `post-tone-verify` and `post-glare-verify`, the game's motion blur
  option on (`MotionBlur=HIGH`), the build `evidence/I-0138/artifacts/build-t4`: the tree `51f868eb6`, whose module
  sources are `main`'s): the High Wall, the Undead Settlement, Firelink Shrine and Anor Londo, each at 1280x720,
  1920x1080 and a 1500x840 window, with the TAA. On each witness frame our output and the library's programmes' drawn
  by a second stage from the same inputs: **52 motion blur pairs, 52 equal, 0 differing bytes; 52 glare pairs, 52
  equal; 52 tone-map pairs, 52 equal**. The motion blur's row in every run: on, `"programmes":"ours"`, `"verify":true`,
  2 passes of 5 taps, 3 draws (the world's count, as the final review computed). No `native_glare_verify` failure row.
- **Its limits**: the depth of field drew nothing at these views (« no level drawn », as in tranche 2's slot), so no
  depth-of-field pair was written: live, its programmes are not exercised here, and rest on the offline tests. In 10
  of the 12 runs the game's window had lost the foreground to the Claude application before the looks, which were
  refused: the camera did not turn there (the pairs were still written), so the blur's taps were probably seldom taken
  on those frames (not measured); the two last runs (Anor Londo at 1920x1080 and 1500x840, 5 pairs) had their looks.
  A run of 60 s gives 2 to 5 witness frames, fewer than tranche 2's 8.
- Evidence: `evidence/I-0138/artifacts/tranche-4-live-verify/` (`compare.txt`, `series.log`, `slot.log`, one pair kept,
  `m37-1920x1080-world-2400-mb-ours.rgba16f` and `-mb-yebis.rgba16f`); the runs `evidence/I-0066/artifacts/i138-t4v-*`
  (their journals and window pictures; their 817 raw frames deleted once compared, D-0142);
  `evidence/I-0066/artifacts/start-080479eab`; the launcher's rebuild log
  `evidence/I-0138/artifacts/launcher-rebuild-080479eab.log`.
- Nothing is owed to Thomas's eye for this part: the picture is the same bytes as before it.
- The checkpoint merged into `main` as `47ec9d438` (documentation only). The disk rotation (`Clear-DiskCache -Apply`)
  was refused at it, a game and a compiler running (the tour's run, another thread's build; 331 GB free on C:): left to
  the next checkpoint.

## Tranche 4, part 4b: the cutscenes' draw parameters and depth of field (2026-11-13 and 14)

An Opus 5.5 thread, branch `claude/fervent-sammet-b8a4b7` from `main` `e03ae881` (Thomas, D-0276's entry).

### First findings (before any capture)

- **The capture tool is not in the module any more** (OBSERVED): `yebis-draws-capture.flag`, `yebis-hal-observe.flag` and
  the observers they drive were removed by I-0102 tranche 2 lot 2b (`2e8885b2`; I-0102 RESULT: « a new capture starts from
  the recovery commit `6a3b127a` »). TRANCHE-4-READY §5 step 1 assumed them in the tree. The cutscene tool
  (`witness-cutscene.txt`, I-0139, `b0f1882d`) is newer than `6a3b127a`.
- **Natively no cutscene draw-parameter record is ever read** (OBSERVED in `i139-t3b-c01-vordt-nat`, the tour's Vordt
  intro): during the scene the depth of field's keys become `{230000030, 10}`, `{…, 20}`, `{…, 60}` and the bank answers
  them from `default` (rows `native_yebis_sources`: `Dof[230000030,10]=default`), so the native row says « depth of field
  off » (default's Enable 0) where `s30_00_0030`'s ids 10, 20 and 60 have Enable 1 (its dump,
  `evidence/I-0066/queries/gparam-all/param_drawparam_s30_00_0030.tsv`). The cause: the file is admitted (kind 11, area
  230000030), but `Banks::File` names an area's file `mAA_BB_CCCC` only, so `m30_00_0030` is looked for and absent.
  VERIFIED on the decompilation (GhidraMCP): `GetGparamArea_140d27790` gives a kind-11 file `sAA_BB_VVVV` the area
  `((AA + 200) x 100 + BB) x 10000 + VVVV`, a kind-7 file `((AA + 100) x 100 + BB) x 10000`; the engine's lookup
  `GparamBankLookup_140032770` finds a key by that area, then its id, else `{1, 0}`. So until now every cutscene record
  (Tone Map, Bloom, Dof, LightShaft, ColorGrading, and the lighting groups the 50 cutscene files also carry: LightSet, Fog,
  Dominant Light, Env Map, ShadowParam, LightScattering) fell to `default`'s `{1, 0}` natively. The records' edge quality
  -1 (§3) was never reached: the « holder flags » refusal comes after this.
- **The flags-0 branch, read** (GhidraMCP, VERIFIED on the decompilation): `FUN_1416cbfe0` writes the holder's +0x140 from
  `YebisDofCreationFlagsTable_1416fdd80(index)`, whose index -1 returns `0x10000000`, replaced by the holder's creation
  flags (+0xC8); an index over 4 reads past the table (an assertion). `CDepthOfField_ApplyDepthOfField_141767d80` draws
  the depth factor, the levels (`FUN_14175ed50`: the counts +0x1D8 front, +0x1DC back, +0x1D4 their larger; the source
  chain over +0x1D4 levels when +0x140 has no bit 4), the aperture filters (`YebisDofApertureFilterLayer_14175c7e0`: the
  front blur's blend factor only when +0x140 has bit 2, the level blur for every level under +0x1D4), then the composite
  (`YebisDofComposite_14175f850`): under flags 0 or 1 `CompositeBlurredLevels_Tn_1417263b0` with n = +0x1D4 + 1
  textures (the scene, then each level's result, record +0xA0) and their (+0xD0, +0xD4) pairs as one float4 each at the
  same register as the FrontBlur composite's (handle +0xEC; `afXY_DepthOfFieldLevelBlendFactor16` at byte 288, c18), the
  technique row n + 8 x (+0x154) of the table at +0x71E4. So under flags 0 the frame is the FrontBlur frame without the
  blend-factor draws, the composite `CompositeBlurredLevels_T{levels + 1}`.
- **The eight composites' programmes** (`evidence/I-0138/artifacts/tranche-4-programmes/original-ps_DepthOfField_CompositeBlurredLevels_T*.asm`, extracted by their table
  hashes, `fxc /dumpbin /Lx`): `T{n}` is `CompositeBlurLevelsFrontBlur_TS{n-1}_0` with the scene's factor taken absolute
  (`mad_sat r, |t0.a|, c(18+i).x, c(18+i).y`; the two listings differ only there, T3 against TS2_0); T1 is the same copy
  (one hash).

### The code

- `light_producers.{h,cpp}`: `AreaFileName(area)` (default, `sAA_BB_VVVV` for a 2AABBVVVV area, `mAA_BB_CCCC` otherwise)
  names the area's file in `Banks::Impl::File` and `GroupEntries`' own-file fallback; a cutscene file's records are
  labelled by its name, so the grading resolver keeps refusing them (« non-map record »: a cutscene LUT's prefix is not
  read). Test: `glow_bank_test` (a folder holding default.gparam twice, the second named `s30_00_0030`: the area
  230000030 reads it; another cutscene area and a map area without their file fall back as before; `AreaFileName`'s
  cases).
- `13b_prepare_model_frame.inc`: a holder flags index of -1 takes the setup's creation flags (`setup.creation_flags`), the
  index over 4 or under -1 still refused.
- `yebis_dof.{h,cpp}` `BuildDof`: holder flags 0 served (no blend factor, the composite
  `CompositeBlurredLevels_T{levels + 1}`); 6 and over still refused.
- `yebis_pins.h`: the eight `CompositeBlurredLevels_T{n}` pinned from the library's table (117 rows).
- `post_glare.hlsl`: the composite's back levels up to seven, `DOF_ABS` (the factor absolute); the build's eight units and
  rows `pg_ps_dof_levels_{n}` (T1 TS0_0's unit since the final review); Compare-Programmes.py's eight rows (`evidence/I-0138/artifacts/tranche-4-programmes/compare-programmes-4b.txt`): T1 the same listing, T2 to T8 the same values with
  commuted operands (the original's `add r, -c, l` against ours `l - c`, as eleven of the FrontBlur counts in part 4a).
- Tests: `yebis_dof_test` (each captured zone's frame under flags 0: no blend factor, the plan's draws less the front
  levels', the composite T{levels + 1} with the FrontBlur composite's inputs, block and coordinates); `dof_stage_test -ours`
  (T1 to T8 on plans made for the comparison, two synthetic scenes, against the library's programmes; the sweep now 48
  plans). No test, tolerance or threshold was weakened.

### The capture of the original in a cutscene (2026-11-14 00:17-00:31, the ASUS, offline, restored)

[Run-I138Cutscenes.ps1](Run-I138Cutscenes.ps1), log `evidence/I-0138/artifacts/runs-4b.log`; five runs, no fault, every save
restored by hash. The original: the scratch build of `6a3b127a` (I-0102's recovery commit, the oracles whole) with I-0139's
`witness-cutscene.txt` and a capture frame set N library frames after the scene is handed (`yebis-capture-cutscene.txt`;
the tree in the session's scratchpad, never merged; its build `evidence/I-0138/artifacts/build-4b-oracle`, its unit tests
passed). Natively, this branch's build `build-4b`.

- **Vordt's intro, 30000030** (theta m30), the original at +180 and +600 frames (`i138-4b-vordt-orig-180`, `-600`):
  VERIFIED in the captured bytes: the globals' holder flags index (+0x240) **-1**, the DoF quality (+0x4C) 0, the holder's
  +0x140 **0** = its creation flags +0xC8 0 (`reference-shaft-alpha-dof.raw`), the counts +0x1D8, +0x1DC, +0x1D4 all 0, and
  the draws `DepthOfField_BlurFactorAlphaFromDepthTextureTypeCastLut_RVALUE`, `MadT1_S1`,
  **`DepthOfField_CompositeBlurredLevels_T1`**. So the record's edge quality is the index FUN_1416cbfe0 takes, the
  flags-0 table is `CompositeBlurredLevels` (the step review's M2 settled), and at DoF quality 0 the scene draws no level:
  its composite is a copy of the scene. Natively (`i138-4b-vordt-nat`) the records now come from `s30_00_0030` (rows
  `native_yebis_sources`: `Dof[230000030,10]=s30_00_0030`, 20 and 60 too) and the depth of field is refused « no level
  drawn »: the same picture (the original's T1 copies the colour; its alpha the factor, which Alpha_R then overwrites, as
  in the world's « no level drawn » since part 4a). The game tour's « background a little sharper natively » in this
  scene is therefore not the depth of field at this quality.
- **The opening scene, 40000040** (eta m40), the original at +120 frames (`i138-4b-opening-orig-120`): its tone map was
  still the map's (tone type 2, exposure 2.3: `reference-glare-globals.raw`), no Reinhard draw. Natively
  (`i138-4b-opening-nat`) the scene's Tone Map ids 0 and 1 of `s40_00_0040` are read from the scene's start (tone type 3)
  and, without part 4c, refused (« tone type »: no tone-map pass on those frames), where before this branch the native
  drew `default`'s record there. So part 4b alone regresses those seconds: **4b and 4c merge together** (a departure from
  TRANCHE-4-READY's « each merged alone », for that reason). A later capture in that scene is needed for 4c's oracle.
- Our runs' DoF quality is 0; which game option gives which quality is still not known (TRANCHE-4-READY §5).

### The step review (a fresh Opus, `199b75db`)

No High. Answered:
- **M1** (the flags-0 test reused the flags-2 frame's ramps, so its checks passed by construction): the test now builds the
  frame as the native frame does (`NativeDofFrame(…, 0)`: bit 2 clear, its own near ramps and counts), checks its far ramps
  equal flags 2's, no blend factor, one blur a drawn level, the composite `T{levels + 1}` over the scene and the drawn
  levels, its words 72..75 zero and 76 + 4k the levels' (d0, d4), TEXCOORD1 as the FrontBlur composite's.
- **M2** (the flags-0 branch served before its oracle; TRANCHE-4-READY §3's open points: that the edge quality is the index
  FUN_1416CBFE0 takes, that the +0x71E4 table is `CompositeBlurredLevels`, the row's column): agreed, the merge waits for
  the cutscene capture, which names the holder's +0x140 and the technique drawn (below). From the decompilation (the
  caller's stack slots, INFERRED from Ghidra's argument order, not the disassembly): the column 0 and the table byte 0 on
  this path (the table at +0x71E4); the technique's name stays the capture's.
- **L1** (flags 0 is not cutscene-only: edge quality -1 at m30_00_0200, m21_00_0000, m22_00_0000, 1 at m32_00_0100 and
  m38_00_0002, id 0, Enable 1): the comments say so; no native journal of 5,968 has a « holder flags » refusal, so none was
  met in play yet.
- **L2** (the cutscene label differed between `GroupEntries` and `FindGroupParam`, and named the area's file whatever file
  served): one rule, `Banks::Impl::Label(area, file)`, keyed on the file `File()` returned; `glow_bank_test` checks both
  paths give `s30_00_0030`.
- **L3** (stale comments in post_glare.hlsl, yebis_dof.cpp, light_producers.cpp): rewritten.

## Tranche 4, part 4c: the Reinhard curve (2026-11-14)

- **The programme** (`evidence/I-0138/artifacts/tranche-4-programmes/reinhard/`, the eight
  `TonemapHDR_[Dither_][Vignette_]Exposure_[Matrix_]Gamma_REINHARD` extracted by their table hashes): the sensitometric
  programme with its curve's seven instructions replaced: `s = x c208.y`, `y = sat(s (1 + s c208.z) / (1 + x c208.y))`
  (VERIFIED, the listings' difference with `Dither_Exposure_Matrix_Gamma`); the rest, the glare, the matrix, the gamma and
  the dither, identical.
- **Its constants** (a `ghidra-research` worker's static reading, its report
  `evidence/I-0138/artifacts/tranche-4c-reinhard-static.md`; the lane order checked on the disassembly; confirmed by the capture below): tone type 3 is technique index 2 (`_REINHARD`); `YebisTonemapCurveConstants_141718390` case 2 with a = the
  record's ToneMappingFactor F (global 144962014, the bank's field 3, unscaled) and b = 1 (the library's +0x456 clear, its
  creation flags' low nibble 1: INFERRED from the code, not read at run time) writes c208 = (4/F² − 2, 1, 1/F², F²/2), scalar
  SSE, no FMA ([143D7EED8] 4.0, [143D7EE7C] 2.0, [143D7EE10] 1.0). Exposure, gamma and filmic contrast do not enter it.
  TONEMAP-SKIN §16's line on this case has z and w swapped (the code: z = 1/F², w = F²/2).
- **The code**: `yebis::ReinhardCurve(F)` (the four words, the library's operations in its order) and `BuildTonemap` with the
  record's tone type and ToneMappingFactor (entries 2 and 3); `PostToneParams.curve_c` (the former padding) and the flag
  `kPostToneReinhard`; `post_tone.hlsl`: one body, two entry points, `ps` (unchanged arithmetic) and `ps_reinhard` (the
  curve's five operations as the listing has them), the stage's second pipeline chosen by the flag; the frame serves tone
  type 3 by our pass (refused still under the two test flags that draw the library's programmes); the automatic
  exposure's model of the displayed grey takes the curve the block names (`auto_exposure_shader.h`,
  `auto_exposure_math.h`). Pins: the two `TonemapHDR_Dither[_Vignette]_Exposure_Matrix_Gamma_REINHARD` (119 rows).
- **Tests**: `yebis_tonemap_test` (c208 for F 32 and 0.36 bit for bit, the sensitometric block's .zw 0);
  `post_tone_stage_test` (every case of its comparison with the stages it replaces, both scenes, also with the Reinhard
  curve at F 32, 0.36 and 4 against the library's REINHARD programmes, byte for byte on any adapter: 54 more draws);
  `auto_exposure_stage_test` (a Reinhard block: the kernels' goal and factor the CPU mirror's, and not the sensitometric
  goal).
  `post_tone_test` (the layout): its field list follows the block (`curve_c` and a 4-byte `pad` where an 8-byte `pad` was;
  the three programmes it reflects still checked; `ps_reinhard` is not in its list, its layout covered by the stage test's
  bytes). No test, tolerance or threshold was weakened.
- **The capture** (2026-11-14 00:54-01:02, the ASUS, offline, restored; log `evidence/I-0138/artifacts/runs-4c.log`): the
  original in the opening scene at +360 and +1,500 library frames (`i138-4b-opening2-orig-360`, `-1500`) draws
  `TonemapHDR_Dither_Vignette_Exposure_Matrix_Gamma_REINHARD` (id 0, Vignette Power 0.3) and
  `TonemapHDR_Dither_Exposure_Matrix_Gamma_REINHARD` (id 1), its globals tone type 3 and ToneMappingFactor 32 and 0.36; its
  c208 words, read in the captured draws' `$Globals`: `BFFF8000 3F800000 3A800000 44000000` (F 32) and
  `41E6E9E0 3F800000 40F6E9E0 3D84B5DD` (F 0.36), **bit for bit `ReinhardCurve(F)`** (the static reading confirmed, the
  library's +0x456 clear at run time). Natively with 4c (`i138-4b-opening2-nat`) the two records are drawn by our pass
  (rows `native_yebis_tonemap`: tone type 3, on, the vignette at id 0 only, as the original).
- **The capture in the unit tests**: `post_tone_stage_test` takes `i138-4b-opening2-orig-1500` (kept: `keep.txt`) as a sixth
  capture: its tone type, its ToneMappingFactor and Contrust from the capture's globals block, its glare input's extent from
  its draw row (524x441, the first capture outside the two sizes the test knew): **0/255 on the capture's adapter**, the
  Reinhard tone map and the LUT chained (`build-4c-6.log`).

### Parts 4b and 4c: what they leave, the reviews, the merge

- **The cutscenes' lighting groups follow** (the final review's M2): the bank change serves every group of a cutscene key
  from its s-file, the lighting groups too (LightSet, Fog, ShadowParam, Dominant Light, Env Map, LightScattering, Snow),
  where `default`'s `{1, 0}` served them before: in `i138-4b-vordt-nat` the light set's entry for `{230000030, 60}` now holds
  `s30_00_0030`'s direction (0.707, 0.707, 0) where it held default's (0.497, 0.743, -0.448). That is the original's data
  (its lookup finds the s-file by area, VERIFIED). Two native rules then apply to them, unread for kind-11 files and
  **INFERRED**: an unbound area's sun direction and fog position rotated by the map's MapOffset `mAA_BB_00_00`
  (`Banks::Impl::Offset`), and a cutscene Snow record's textures named with the map's prefix (`NativeSnowAreaPrefix`). To
  check against the original in a cutscene whose map has a MapOffset yaw (the RT's sun and fill rows, the snow), filed for
  ROADMAP workstream 5 or 7.
- **Only T1 of the flags-0 composites has a capture oracle** (the final review's L8): every witnessed cutscene draws no
  level at our DoF quality 0; T2 to T8's words, samplers and the flags-0 level counts rest on the decompilation, their
  programmes on the byte-for-byte comparison with the library's. A cutscene at a higher DoF quality (which game option
  gives which quality is still unknown) would witness them.
- **Left from TRANCHE-4-READY**: more than four back levels under flags 2 (no `TS5_x` to `TS7_x` in the library) still ends
  in the refusal « no programme of ours », met by no witness (§2); the other cutscene refusals (§5 step 4): none met in the
  three scenes captured (the rows `native_yebis_*` of the native runs); Vordt's intro filmed for Thomas (§5 step 5): stills
  instead, `evidence/I-0138/artifacts/tranche-4b-sheets/` (`vordt-*.png`: main before this branch, this branch, the
  original; `opening-*.png`: part 4b alone (the Reinhard records refused), 4b and 4c, the original).
- **The second step review** (`199b75db..76741f11`, part 4c): no High. M1 (the Reinhard pipeline not detached at the
  quarantine, a freed pipeline under a hung list) fixed; L1 (the glare extent's row tied to the captured frame) fixed;
  L2 (the « sensitometric » label of a Reinhard capture's own record) fixed: the label follows the record's tone type;
  L3 (the F 0.36 words recomputed by the formula under test) fixed: the captured words pinned, .y checked; L4 (the flags-0
  checks of `yebis_dof_test` may run on no zone) fixed: « no level drawn » only with no level counted, and the zones that
  ran the composite checks counted and printed; L5 (comments) rewritten.
- **The final review** (a fresh Opus at xhigh on `main...HEAD` after `main` merged in): no High. M1 (this RESULT and the
  sheet not final) done here and in TRANCHE-4-READY; M2 recorded above; L1 the step review's M1; L2 its L3; L3 the comments
  (the tone-map row now logs the third constant); L4 the vignette power (0.3) and the sheets referenced; L5 (T1 compiled
  twice, the same programme as TS0_0) fixed: T1's row uses TS0_0's unit; L6 the step review's L2; L7 (the live verify tool
  cannot witness the Reinhard pass: the two test flags refuse tone type 3) kept, recorded; L8 above.
- **The integrity review** (`integrity-reviewer` on `main...HEAD` and the reviews' fixes): no High, nothing weakened. M1 (the
  fixes not yet built): built and tested, `evidence/I-0138/artifacts/build-final-2.log`: `yebis_tonemap_test` 251 checks (the
  captured words, .y), `yebis_dof_test` 4,275 checks and « flags 0: the composite checks ran at 5 of 5 zones », the T{n}
  sweep 472 of 472 in 48 plans, the opening scene's capture 0/255 on its adapter. L1 (the reader's T rows unsaved): saved,
  `tranche-4-programmes/compare-programmes-4b.txt`. L2 (the layout test's change unstated in 4c): stated. L3 (K-0492 worded
  T{levels + 1} as witnessed): only T1 witnessed, the rest INFERRED, reworded.

### The merge (2026-11-14)

Merged into `main` as `d365e26f` (the PC lent by the coordinator, no game running); the launcher's module rebuilt from
`main` with its unit tests (`evidence/I-0138/artifacts/build-launcher-d365e26f.log`); the start check `start-d365e26f4`
passed (03:29-03:30, the ASUS, restored). The disk rotation (`Clear-DiskCache -Apply`) refused at the checkpoint, a
compiler running (the E pilot's build): left to the next checkpoint. For Thomas's eye: the opening scene's stills
(`evidence/I-0138/artifacts/tranche-4b-sheets/opening-*.png`: 4b alone, 4b and 4c, the original); the rest of the two
parts draws the picture the native drew before or the original's.

### A Fable 5.2 read of tranche 4 (2026-11-14, read-only, after the merge)

The idle-thread rule (Thomas, 2026-11-12: a Fable 5.2 reviewer, read-only, on what the thread added; the coordinator,
03:38): a fresh Fable 5.2 read both merges (`080479eab`, `d365e26f4`) against this RESULT. Nothing fixed here; recorded for
the next thread that touches these files. No High.

- **M1** `auto_exposure_stage_test.cpp:109-115`: its one Reinhard case (F 32: `curve_c` 1/1024, `curve_b` 1) cannot tell
  whether the kernel reads `curve_c` or `curve_b` at all (the term moves y by about 3e-5 against tolerances of 1e-3 and
  2e-3); `auto_exposure_test` has no Reinhard case. The code is right by reading (offsets 140 and 152, the formula). A
  case at F 0.36 (`curve_c` about 7.7, the opening scene's id 1) would discriminate. Open.
- **L1** `yebis_dof_test.cpp:140-158`: the flags-0 checks take their expected technique and words from the same
  `DofLevelCounts` and `NativeDofFrame` the builder reads: a wrong rule for T2 to T8's levels passes; « 5 of 5 zones » reads
  stronger than the one oracle (T1) this RESULT states. Open (needs a capture at a higher DoF quality).
- **L2**: flags 0 now draws for the five map records with an edge quality of 0, 1 or -1 (listed above) where the plan was
  refused; met by no witness yet. Same oracle gap as L1.
- **L3**: at DoF quality 3 or 4 a cutscene record's -1 takes creation flags 6, still refused (« holder flags other than
  0 and 2 »): no cutscene depth of field there. A remaining refusal, not listed above until now.
- **L4** `yebis_pins.h:76, 84, 85`: T1 and the two `_REINHARD` pins say `observed=false` though this RESULT's captures
  observed them (`yebis_pins_test` counts them). Documentation only.
- **L5** `yebis_tonemap.cpp:86`: the Reinhard words 834-835 of the library's block are reached only from the test (the two
  flags that draw the library's programmes refuse tone type 3); c208.x and .w are computed and unread by our programmes.
- **L6** `yebis_motionblur.cpp:102`: `base<2` can never hold (part 4a).
- **L7** `post_glare.hlsl:25`: « DOF_BACK n - 1 (0 to 7) » with `DOF_ABS`: compiled for 1 to 7 (T1 is TS0_0's unit).
- **L8** `post_tone_stage_test.cpp:88-96, 332`: `ReinhardProgrammes` repeats `Programmes`' scan with another size cap; on
  the opening scene's capture the « mapping 0 » case is a Reinhard draw counted under `compared`.
- Nothing found in the programmes' block offsets, the plan builders' ranges, the tone stage's pipelines and quarantine,
  the banks' naming and labels, the verify stages' resource states, the stage tests' gating, threading and lifetime.

## Tranche 5: HDR10 (2026-11-14)

ROADMAP workstream 11, tranche 5 ([TRANCHE-5-READY](TRANCHE-5-READY.md), [PLAN](PLAN.md) §5), by the R pilot's thread (Fable 5.2,
branch `claude/frosty-newton-8ec678`). Not seen on an HDR screen yet: everything below was built and witnessed on the ASUS
(no Windows HDR), the chain forced; the session on the OLED with Thomas is the tranche's last step, still to hold.

### Conclusion

- **Delivered**: an HDR10 output beside the SDR one, chosen by a row of the Advanced Settings page (« HDR output », with
  « Peak brightness » and « Paper white » under it), live. On: an `R10G10B10A2_UNORM` chain in ST 2084 over Rec. 2020 where
  Windows' HDR is on for the window's screen; off, or on an SDR screen: the SDR chain. Its tone programmes are the ones
  before this tranche (their instructions hash equal, their tests still exact against the original's draws); the other
  files its frames go through and this tranche touched (the glow and display stages, the UI's and the overlay's passes, the
  frame's tail) were read by five reviews, their SDR frames run live (`i138-t5c-menu`, the cycles' SDR halves), not compared
  byte for byte with a build of `main`.
- **The original colorimetry in HDR**: the SDR picture under a knee at 0.75 of the curve's output, its highlights continued
  above it (up to 4.4 times the paper white), the area's LUT read in the same draw, the emissive glow added in float; the
  game's UI and our overlay drawn in place on that float picture, with the game's own blend states; one output programme
  (decode, roll-off to the peak, Rec. 2020, ST 2084, a blue-noise dither of one 10-bit code).
- **OBSERVED** on the ASUS (seven series, no fault, no debug-layer error): every back buffer read within 1.06 codes of the
  output's model; the rows work in the game's menu; 39 live switches between the two chains, every one by the test flag
  `hdr-cycle.flag` (the last request of each cycle run, made while the game was closing, was refused: « Evidence »); a
  menu's stills on both chains differ by 1.05 to 1.47 levels of 255 on average once the compositor's conversion is taken
  back (their means by 0.35 at most). **Not observed**: the picture on an HDR screen (the knee, the glow's weight, the
  paper white are Thomas's to judge there); an HDR10 chain brought up by the settings' row (on the ASUS the row leaves the
  chain SDR, as it must: every HDR10 chain here was forced by a flag); DLSS-G, DLSS super resolution and the DLAA on an
  HDR10 chain (every witness ran the TAA; the HUD-less colour is written, no frame generation run was made); a
  full-screen toggle, an exclusive display mode and the frame generation's chain reopen on an HDR10 chain (a resize to
  another size was run: `i138-t5f-resize`); a chain that
  refuses the colour space (its path ran in no test and no witness); the brightness option at another setting than the
  runs' identity.
- **Departures from the plan** (each in its section): the emissive glow is not moved before the tone map (D-0276 asked it at
  this tranche; Thomas told, his answer pending); no HDR metadata; the UI is not Elden Ring's texture blended last (D-0195's
  model, the sheet's §3.7) but drawn in place, so DLSS-G gets no UI colour-and-alpha buffer (the sheet listed it as a gain;
  it gets the HUD-less colour); the brightness option keeps its meaning instead of being left to the paper white; the row
  « HDR output » is offered on every screen (the sheet's §3.12 showed it only where Windows' HDR is on: the HDR can be
  switched on while the game runs and the row's request is followed live; on an SDR screen it changes nothing); the
  session on the OLED is not held yet.
- **A limit to put to Thomas**: on an HDR10 chain with the TAA and DLSS both off there is no anti-aliasing at all (the
  game's own filter is an 8-bit pass, not run on a float frame; such frames are counted, `aa_filter_skipped_frames`).

### The study at the tranche's head (what the sheet left open)

- **The output format is HDR10, not scRGB** (the sheet's §3.1, VERIFIED in NVIDIA's guide): Streamline 2.14.16's
  `docs/ProgrammingGuideDLSS_G.md` §11.0: « If your game supports HDR, use UINT10/RGB10 pixel format and HDR10/BT.2100 color
  space », « DLSS-G currently does NOT support FP16 pixel format and scRGB color space »; the HUD-less colour in « the same color
  space » as the back buffer (§5); a UI colour-and-alpha buffer must not be `R10G10B10A2` (its alpha).
- **No HDR metadata is set** (a departure from the sheet's §3.3): Microsoft's page for `IDXGISwapChain4::SetHDRMetaData` says
  « It is no longer recommended for apps to explicitly set HDR metadata on their swap chain » and to « tone-map content into
  the gamut and luminance range supported by the monitor » (read 2026-11-14 by a web reader). The output clamps to the peak
  itself.
- **The screen's state** (Microsoft's « Use DirectX with Advanced Color on high/standard dynamic range displays », the same
  reader): `IDXGIOutput6::GetDesc1`, a `ColorSpace` of `DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020` means HDR is on for that
  screen; a factory made earlier keeps a stale answer (`IsCurrent`), so the probe makes its own; Windows' SDR reference white
  is 80 nits by default and « typically » set around 200 on desktop HDR monitors (`DISPLAYCONFIG_SDR_WHITE_LEVEL`).
- **The original colorimetry's highlights in HDR** (the sheet's §5 decision 1, the recommendation (a) applied, Thomas away): our
  tone map's SDR value v (the largest channel of the curve's output with its glare, before the colour matrix) is kept up to a
  knee of 0.75 and continued above it as `knee + (1 - knee) ln((1 - knee) / (1 - v))`. That is the continuation, at the
  knee's slope, of a curve whose shoulder closes on 1 as an exponential of the scene's luminance, which the sensitometric
  curve's does: for `y = 1 - exp(-a x)` the straight continuation at the knee is exactly that expression, whatever `a`. The
  colour takes the gain on its three channels (its hue and everything under the knee are the SDR picture's), the LUT is read
  at the SDR value and the gain applied after it (the sheet's §3.5, first candidate). 0.99 gives 1.55 times the paper white,
  0.999 gives 2.1, the float limit 4.4; the output rolls the largest channel off to the peak above max(paper white, half the
  peak). The knee is one number (`kPostToneHdrKnee`), to judge with Thomas on the OLED (0.5 would give 2.5 at 0.99).
- **The emissive glow stays at the original's place in HDR** (a departure from D-0276's « before the tone map », said to Thomas
  in the thread on 2026-11-14 at 06:24, his answer pending): the reason given to him then (« nécessaire pour le HDR ») no longer
  holds with a float picture, where the glow adds without a clamp; at its place and in the encoded values it is the SDR
  picture's glow. Elden Ring's order (the glow before the curve) goes to tranche 6's candidate made after Elden Ring, where it
  is judged on sheets; the variant promised to Thomas for the OLED session comes with it, not before.

### Step 1: the screen's probe, the HDR10 chain, an SDR picture inside it

- **The probe** (`ProbeHdrDisplay`, `backend.cpp`): `IDXGIOutput6::GetDesc1` of the window's monitor through a factory made for
  the call; at Start, at every resize of the presentation, and when the window changes monitor or that factory is no longer
  current (step 3). A probe that does not find the window's screen (a window going away) keeps the last answer.
- **The chain** (`FramePresenter`): asked (`hdr_wanted`), its buffers are `R10G10B10A2_UNORM` and its colour space
  `DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020` (`CheckColorSpaceSupport`, then `SetColorSpace1`), said again after every accepted
  resize; a chain that refuses the colour space is made 8-bit again with its reason and asked again only when the request or
  the screen's answer changes; the stand-in buffers, the exclusive full-screen modes and the witness readback take the
  chain's format; an SDR chain that stays one is told nothing new.
- **Test flags** (before the settings): `hdr-output.flag` (HDR10 where Windows' HDR is on for the window's screen),
  `hdr-output-force.flag` (on any screen: the compositor converts), `hdr-paper-80.flag` (the paper white at 80 nits),
  `hdr-cycle.flag` (the request alternating every 600 engine frames between off and forced). The journal's row
  `native_hdr_output`.
- **The witness picture** of an HDR10 back buffer is written as `<stem>.r10g10b10a2`, never under the 8-bit picture's name;
  [Decode-Hdr10.py](Decode-Hdr10.py) reads it against the picture the output pass read.

### Step 2: the picture in float

- **Every frame of an HDR10 chain ends in one float picture** (`OutputStage`, `R16G16B16A16_FLOAT`, display-encoded: the
  power 1/2.2 of linear light, 1 the paper white). A world frame with our tone map: its HDR entry (`ps_hdr`,
  `ps_hdr_reinhard`: `ToneColour`, the body shared with `ps`, then the record's gamma, the LUT read in the same draw, the
  highlights' gain) draws once into it and the emissive glow adds onto it (the glow stage's float final pipeline, the same
  programme and values); no AA filter, no NIS and no 8-bit image on such a frame (the TAA or DLSS and Thomas's sharpening
  run before the tone map as ever; the frames whose 8-bit AA filter was asked are counted). Any other world frame (no tone
  map of ours, the two test flags that draw the library's programmes, the HDR entry given up): its 8-bit picture copied into
  the float one (`ps_copy`). A startup frame (logos, title, menus, loading): from black.
- **The output pass** (`ps_scene`, `shaders/post_output.hlsl`), the frame's last draw in place of the game's brightness pass:
  decoded, the largest channel rolled off to the peak, taken to Rec. 2020 (BT.2087's matrix), scaled to the paper white,
  clamped to the peak, encoded by ST 2084, dithered. DLSS-G's HUD-less colour is the same pass before the UI, 10-bit.
- **The dither** (`blue_noise.h`, written by [Make-BlueNoise.py](Make-BlueNoise.py)): a 64x64 blue noise of our own (void and
  cluster, Ulichney 1993; its power under 0.1 % of its variance up to radius 8 of 32, a white noise's being 100 %), one 10-bit
  step, its place moved every frame, the same for the HUD-less colour and the final picture of a frame.
- **The SDR programmes are unchanged**: `ps`, `ps_reinhard` and `ps_lut` compiled from the new file hold the same instructions
  as before (the listings without their comment lines hash equal, checked again after each change of the file; the block's
  last word, unused there, is named `hdr_knee`), and `post_tone_stage_test` still requires their bytes against the original's
  captured draws.

### The UI in place (a design changed inside the tranche, and a wrong reading)

- **First design** (steps 2 and 3 as committed in `8c67103ee` and `b9f5c3819`): the game's UI and our overlay drawn alone on a
  transparent 8-bit layer, blended over the float scene by the output pass, after Elden Ring's UI texture (D-0195). The
  additive rows had their own alpha rule there; the multiply and screen rows, which read the target, could only see the
  layer: counted as approximations (2,156 draws in `i138-t5c-cycle`, a menu open).
- **A wrong reading, said to Thomas and withdrawn** (the thread, 08:40 then 09:10): in `i138-t5c-cycle`'s stills the menu on
  the HDR10 chain looked a third brighter (mean luma 44.5 against 32.8, `tranche-5-step3/stills-raw.txt`) and I named the
  layer's multiply rows as the cause.
  The cause was the compositor: on an SDR screen it shows an HDR10 picture counting the screen's white as 80 nits, and those
  runs sent a paper white of 200. Taken back through that conversion ([Compare-HdrStills.py](Compare-HdrStills.py)), the
  stills of the HDR10 chain equal the SDR chain's within 1.1 to 1.5 levels of 255 in the three regions compared (the title
  band, the panel, the scene beside it), **with the layer and without it** (`tranche-5-step3b/stills.txt`). OBSERVED: no
  visible effect of the layer's approximations was found in those regions; where those 2,156 draws land was not located.
  The reader leaves out every value the compositor clipped (a capture byte of 255: at 200 nits every level above about
  168 of 255), so it does not see the UI's brightest values; the menu-closed pair of the same file differs by 9.4 levels
  on average (means 56.2 and 55.2, 29,218 values clipped): the game's scene, its fire and its highlights, INFERRED, not
  separated.
- **Final design** (`3e493c20d`), kept for what it is, not for a defect seen: the UI and the overlay draw in place on the
  float picture, as the SDR chain draws them on its 8-bit picture, with the game's own blend states (the pipelines of the
  draws into the frame's own target made for the float format, `PrimitivePass::Build`'s `float_target`; our overlay's three
  passes a second pipeline). Exact for every row of the game's blend table by construction; one target, one clear, one
  programme and the alpha rule less than the layer. A UI build is made with its frame, so its format is the chain's; a build
  that would meet a frame of the other format is not drawn that frame and counted (`ui_format_skips`, 0 in the witness).
  Known limit: the multiply and screen rows over a scene value above the paper white see `1 - target` negative or a target
  above 1 (the float picture is not clamped): the screen row then takes a little light away instead of adding it. Not met
  where the UI draws in the witnesses; INFERRED small, to look at on the OLED over a bright scene.

### Step 3: the settings' rows, the live switch, the game's brightness

- **The settings** (`rekindled_settings.h`): `hdr` (off by default), `hdr_peak` (`auto`, or 400 to 10,000 nits) and
  `hdr_paper_white` (80 to 500 nits, 200 by default). The Advanced Settings page (`graphics_rows.h`): « HDR output » (« Sortie
  HDR »), and under it while it is on « └ Peak brightness » (Automatic: the screen's own maximum, `IDXGIOutput6`'s
  `MaxLuminance`, or 1,000 nits when it gives none above the paper white; 400 to 4,000 nits) and « └ Paper white » (80 to 400
  nits), before the vignette (kept last). The test flags pass before the settings.
- **Live** (`FollowNativeLiveSettings`, `SceneRenderer::SetHdrOutputLive`): a changed request is applied between frames, the
  worker quiesced (`WithPresentationIdle`); the chain is resized only when its format changes (about half a second), a paper
  white or a peak alone changes the output pass's constants. The same block follows the screen (`HdrOutputStale`).
- **The game's brightness option in HDR** (a departure from the sheet's §3.6, which left it to the paper white): the option
  is a power applied to the encoded picture before the screen's decode (the display stage's curve, `values[0] / values[3]`),
  so on an HDR10 chain the output pass decodes by 2.2 times that power up to the paper white, and by 2.2 above it (the option
  does not reach the highlights, which keep their ratio to the paper white). The paper white is left to its own row.
  INFERRED equal to the SDR chain's picture under the paper white (the unit test holds the arithmetic; not compared live at
  another brightness than the runs' identity).

### Tests (D-0268)

New, or changed, in this tranche; each change of a requirement is stated.

- **`hdr_output_test`** (new; WARP and the card, 95 checks): the ST 2084 curve against the standard's published values (100,
  203, 1,000, 4,000 nits) and its inverse, the Rec. 2020 matrix's rows summing to 1, the peak's rule, the highlights'
  continuation and the roll-off (their argument below their start, continuous, slope 1 there, rising); the C++ block's layout
  against the compiled programme's (`D3DReflect`); then the stage's draws into a 10-bit target against the CPU model
  (`hdr::EncodedToHdr10`) within one code without the dither (measured 0.62 on WARP, 0.70 on the card): the 256 values of an
  8-bit picture copied and transformed at 80 and 200 nits and at a peak under the paper white; a float picture up to 11 times
  the paper white at peaks of 1,000 and 300 nits and under the brightness option's power 0.8 and 1.25; with the dither each
  code within one and a half and the mean error under a fifth of a code; no debug-layer message (the layer's message queue
  required since the integrity review: the check passed without it before, had the queue been missing). **Changed during the
  tranche**: its first form (99 checks) tested the 8-bit picture's own output programme and the UI layer's blend; both were
  removed with the code they tested, when the UI went in place.
- **`frame_presenter_test`** T5 and T6 (new): an HDR10 chain asked takes the colour space, its buffers and stand-ins are
  10-bit: run on WARP and on the card, which both take it. The other branch of T5 (a chain that refuses the colour space
  is 8-bit again, with its reason) is written but ran on neither adapter, and nothing tests that a refused chain is not
  asked again until the request or the screen's answer changes: the refusal's path is held by reading only. T6 prints the
  probe's answer, it requires nothing.
- **`post_tone_stage_test`** section 3 (new): the HDR entry against the 8-bit draws of the same inputs (16 draws: the capture
  and a synthetic scene, plain, glare, vignette and factor, LUT, each with both curves). Where the curve's largest channel is
  under 0.97 of the knee (read from the entry's own output through the colour matrix's inverse, no channel of that output at
  a clamp), every value within 1.1/255 of the 8-bit draw (measured 1.06 on the card, 1.09 on WARP: the 8-bit draw's dither
  and rounding leave little under the bound there); any other texel never under it by more
  than that; with a LUT, under the knee (read from the entry's draw of the same inputs without the LUT) within, and
  elsewhere never under, the two-draw form by more than the LUT's bound: the strip's slope for one level of input (the
  one-draw read of the unrounded value against the read of a dithered, rounded byte), the two reads' filter weights and
  the roundings: 5.02/255 for the captured strips; measured two-sided 4.18 to 4.59 on the card, 4.04 to 4.28 on WARP; the
  alpha within 0.65/255; the plain sensitometric case against a CPU model of the whole entry within 0.2 % (0.098 % measured;
  skipped for every draw of the Reinhard curve, the capture whose own record is that curve's included, so the Reinhard
  entry has no model and its highlights are only held from below, as every graded draw's; and within 1e-4 of the curve's
  end, where single precision decides the logarithm). The texels set aside from the two-sided check are printed by reason (of 14.7 million:
  4.6 to 4.9 with a channel at a clamp, 0.8 to 1.6 at the knee or above; 1.4 of them graded). **Changed during the tranche**: the LUT bound was
  first written 3/255, measured 4.07, and replaced by the bound derived from the strip's slope (a derivation, `StripSlope`,
  not a fit to the measure); the « under the knee » rule was first the entry's largest channel under the knee, then (the
  second step review's L5, the knee moved to the curve's output) read through the matrix's inverse with a floor of 1e-3 on
  every channel, which set aside 46 % of the opening scene's texels without saying so (the third step review's M1): now the
  clamps alone; a graded texel was only held from below (the final review's L2): held on both sides under the knee since,
  and the LUT's bound, first the slope plus 0.7, failed that two-sided check on the card by 0.005 (4.587 against 4.582):
  restated as the slope times (1 + 34/256) plus 0.62, the 34/256 being the two reads' linear-filter weights (8 fractional
  bits: 1/256 of a node step of 17 levels each) that the first bound had left out (INFERRED from the card reading 0.3
  level away from WARP; a derivation, 0.43 above the measure). Sections 1 and 2 (our SDR draws against the original's captured bytes) are unchanged and still exact on the
  card.
- **`post_tone_test`**: the block's last word named (`hdr_knee`, 0.75) and the two HDR programmes' layout; no requirement removed.
- **`rekindled_settings_test`**: the three keys' parse, bounds and write; the round trip's count of accepted keys 12 to 15
  (three keys more in the file).
- **`graphics_rows_test`**: the three rows (shown, greyed, values, the nearest choice of a file's value outside the lists, the
  help lines' bound), the vignette still last; no requirement removed.
- **No unit test draws the game's UI on a float target**: the UI's float pipelines and the overlay's are witnessed live only
  (the debug layer on, which refuses a pipeline whose format is not its target's: no error in `i138-t5d-cycle`).
- **The readers** (the live witnesses' judges). [Decode-Hdr10.py](Decode-Hdr10.py) was written first to decode the HDR10
  picture back to 8 bits and require the SDR picture's bytes; that criterion failed on 187 of 2.76 million values of the
  first witness (the weakest channel of saturated colours: 10-bit codes over a wider gamut do not hold it to an 8-bit level)
  and was replaced, before any result was reported, by the forward model: every code within one of the pass's own arithmetic
  on the picture it read (the float picture's form: within one and a half, the dither's code included, and a mean difference
  under a tenth). [Compare-HdrStills.py](Compare-HdrStills.py) takes the compositor's conversion back before two screen
  captures are compared (above); it gives means and a mean absolute difference, no verdict.

### Evidence

- **Builds**: `evidence/I-0138/artifacts/build-t5s2` (`8c67103ee`, `build-t5s2.log`) and `build-t5` (`b9f5c3819`,
  `build-t5s3.log`; `3e493c20d`, `build-t5s4.log`; the tree with `main` merged, `798ab188d`, `build-t5s5.log`; the fourth
  step review's fixes, `b94c75ec6`, `build-t5s6.log`; the final review's and the fifth step review's, `3542b8cdf`'s tree,
  `build-t5s8.log`): every unit test passed on WARP and on the card each time, 177 lines (`build-t5s7.log` is the one
  build that stopped: the graded two-sided check against its first bound, « Tests »); the SDR tone draws 0/255 against the original's captured draws on the card. The SDR tone programmes' listings
  before the tranche and at its head: `tranche-5-step2/sdr-programmes-hash.txt` (equal).
- **`i138-t5a-*`** (06:50 to 07:01, the ASUS, no measure, Firelink Shrine, restored by hash; `tranche-5-step1/`): step 1's
  chain forced at 1280x720, 1920x1080 and a 1500x840 window, at 80 and 200 nits; 32 of 32 back buffers within 0.563 code of
  the model on the 8-bit picture read; the auto mode stays the SDR chain on this screen. A screen capture at 80 nits is 8 %
  darker than the SDR run's (ratio 0.918; the same picture re-encoded by sRGB on the CPU: 0.929; `screen-captures.txt`).
- **`i138-t5b-*`** (07:27 to 07:35, the same protocol; `tranche-5-step2/`): about 2,200 float frames a run, 32 of 32 back
  buffers within 1.059 codes of the model (the dither's code included), mean -0.03; at 200 nits under 1 % of Firelink's
  picture is above the paper white, up to 784 nits before the roll-off (763 in the back buffer); the HUD right on the
  decoded pictures. Two runs' pilots reported a lost
  focus and a late network notice; the runs wrote their pictures and restored.
- **`i138-t5c-menu`** (08:16 to 08:21; `tranche-5-step3/`): the Advanced Settings page by keys: fourteen rows, sixteen with
  « HDR output » on (the window scrolls), the peak Automatic, 400, 600 nits and back, the paper white 250, 200, 150, 200,
  then off; each change saved and followed (`native_rekindled_graphics`, ten `native_hdr_output` rows); the chain stays SDR
  on this screen (auto). Stills `options-step-7.png`, `-12.png`.
- **`i138-t5c-cycle`** (08:21 to 08:25, `hdr-cycle.flag`): 17 live switches, no fault; four HDR10 back buffers within 1.058
  codes. At the run's end the window's screen was not found and a last resize was refused (`0x80070005`) while the game was
  closing: the chain kept its format and the journal said so. The probe now keeps the last answer and does not probe a
  window that is gone; a resize asked of a closing window is still refused (below), its journal row naming no reason.
- **`i138-t5d-cycle`** (08:59 to 09:03, `3e493c20d`; `tranche-5-step3b/`): 22 live switches through the options' tabs and
  the game; a 23rd, asked as the game closed, was refused (`0x80070005` three times, stand-in buffers, the chain left SDR;
  its `native_hdr_output` row says « the chain resized » and names no reason). No fault, no debug-layer error,
  `ui_format_skips` 0, no frame copied (every world frame took the tone map's HDR entry); four back buffers within 1.057
  codes of the model on the float picture with its UI; the menu's stills on both chains (`stills.txt`: the same page and
  cursor, mean absolute difference 1.05 to 1.45 levels in the three regions, 1.9 over the whole window); the float
  picture's values under the HUD (`float-picture-scan.txt`).
- **`i138-t5e-copy`** (10:20 to 10:22, `718b41ed7`'s module; `tranche-5-step4/`): a forced HDR10 chain with
  `post-tone-old.flag` (the library's tone programmes, so no HDR entry): 2,637 world frames' 8-bit picture copied into the
  float one (`copied_frames`), the frame graph's `hdr_copy` pass on each with no state mismatch reported, no fault, no
  debug-layer error; seven back buffers within 1.058 codes of the model on the float picture (nothing above the paper
  white there: an 8-bit picture has no highlight).
- **`i138-t5f-resize`** (10:46 to 10:49, the final module, `build-t5s8`): a forced HDR10 chain resized from the menu's
  Resolution row to 1440x810 (596 world frames) and back to 1280x720, the float picture made again each time and the
  former one retired by the queue; no fault, no debug-layer error, no display loss. (`i138-t5e-resize`, 10:22 to 10:25,
  resized nothing: its keys did not open the row's list.)

### Uncertainty / unresolved questions

- **Nothing was seen on an HDR screen.** The knee (0.75), the glow's weight in float, the default paper white (200 nits), the
  roll-off's start and the dither are numbers of mine; Thomas judges them on the OLED, Windows' HDR on. A paper white read
  from Windows' own SDR white level (`DISPLAYCONFIG_SDR_WHITE_LEVEL`) as the default is an idea, not done.
- **DLSS-G on an HDR10 chain**: the HUD-less colour is written in the back buffer's colour space as the guide asks; no run
  with frame generation on an HDR10 chain was made.
- **The frames that take the copy in play** (an unlit frame, a scaled frame the DLSS does not resolve, a tone map's HDR
  entry given up) were not met; the copy itself was run by a test flag (`i138-t5e-copy`).
- **The compositor's conversion** on an SDR screen (80 nits, sRGB) is INFERRED from the captures' ratios, not read in a
  document.
- **The UI on an unclamped target** (the fourth step review's L2): on the 8-bit picture a blend's result stops at 1; on the
  float picture an additive stack of UI rows may pass the paper white, a screen row over a highlight pulls it toward the
  paper white, and a UI programme writing above 1 could drive a screen row negative (shown black). In `i138-t5d-cycle`'s
  float pictures no value is negative and the HUD's boxes hold nothing above 1 but the scene's own highlights behind them
  (12 and 33 texels in the boxes over the candles and the bonfire, `tranche-5-step3b/float-picture-scan.txt`; the UI's part
  and the scene's not separated). Unseen on a bright scene under a menu: to look at on the OLED.
- The 2.2 decode of the SDR picture (against sRGB's piecewise curve) is the choice that keeps the HDR picture's shadows as
  the SDR screen shows them; to explain to Thomas on the OLED if the shadows read differently from Windows' own SDR
  conversion.

### Artifacts produced or modified

`native/renderer/hdr_output.h`, `output_stage.{h,cpp}`, `blue_noise.h`, `shaders/post_output.hlsl` (new); `backend.{h,cpp}`,
`display_stage.{h,cpp}`, `scene_pass*.{h,cpp}`, `post_params.h`, `post_tone_stage.{h,cpp}`, `shaders/post_tone.hlsl`,
`glow_stage.{h,cpp}`, `primitive_pass.{h,cpp}`, `overlay_pass.{h,cpp}`, `rekindled_settings.h`; `native/frame/graphics_rows.h`,
`game_bootstrap/01_state.inc`, `02_frame_lifecycle.inc`, `02b_rekindled_graphics.inc`, `10_options_and_passes.inc`,
`15_cpu_service_and_input.inc`, `20_startup_phases.inc`, `Build-GameBootstrap.ps1`, the tests named above;
`Run-StartupTrace.ps1` (the four flags); this folder's `Decode-Hdr10.py`, `Compare-HdrStills.py`, `Make-BlueNoise.py`,
`Run-I138Hdr.ps1`, `Run-I138HdrMenu.ps1`. Evidence: `evidence/I-0138/artifacts/tranche-5-step1`, `-step2`, `-step3`,
`-step3b`, `tranche-6-licences` (the licences read for tranche 6: AgX from Filament's `ToneMapper.cpp`, Apache-2.0; ACES by
Stephen Hill's fit, MIT).

### Reviews

- **Step 1's step review** (fresh, read-only, `89f51cb12`): no High. M1 the display and output stages opened before the chain
  is asked 10-bit; L1 a refused colour space latched and a buffer of another format never acquired; L2 the brightness curve
  not uploaded, declared or read on an HDR10 chain; L3 the refusal named; L4 the 2.2's comment; L6 the probe printed, not
  required; L5 (follow the screen without a resize) done at step 3. Applied in `8c67103ee`.
- **Step 2's step review** (`8c67103ee`): **H1** the output stage made a view again from a pointer kept across a resize of
  the display stage (a freed picture): two heaps, each made from the resources its own Record is given. M1 a timing
  statement swallowed by a comment I had put mid-line: restored. L1, L6 a chain changing format against the UI build and the
  glow's float pipeline (now: the build carries its format, the pipeline is made when first needed); L2 the gain's gamma
  tied to `kSdrGamma`; L3 the frames whose AA filter does not run counted; L4 the alpha's tolerance stated; L5 the
  highlights' gain taken from the curve's output, not from the colour matrix's (a matrix that saturates a channel no longer
  jumps to the float limit). Applied in `b9f5c3819`.
- **Step 3's step review** (`b9f5c3819`): no High. M1 the test's floor (above): the clamps alone, counts printed. L1 a format
  change outside `Resize` (the frame generation's reopen) missing the hooks: no hook any more; L2 a refused colour space
  never asked again: asked again when the screen's answer changes; L3 two probes a switch: one; L4 the follow's first
  request the renderer's; L5 the brightness option brightening the highlights when set darker: its power stops at the paper
  white. Applied in `3e493c20d`.
- **The fourth step review** (the UI in place, `b9f5c3819..c7e3b42c7`): no High; formats, states, lifetimes, the probe and
  the tests read and found right. M1 the copy's pass declared to the frame graph before its hand transition (every such
  frame would have been reported and its plan never cached): the transition first, as every pass there. L1 a float picture
  that cannot be made faulted a frame: made before the chain is asked 10-bit, a failure refuses HDR10 with its reason. L2
  the UI's blends on an unclamped target (« Uncertainty »); L3 a failed float pipeline of the glow turns the glow off for
  the session (left: it is the 8-bit pipeline's programme on another format); L4 no offline test of the UI on a float
  target (« Tests »). Applied in `b94c75ec6`.
- **The integrity review** (fresh, read-only, `main...559decb08`): no High; every test change found stated; no special
  case. Its points answered in this section: the live switches counted 39, the last request of each cycle run refused
  while the game closed (M1); `frame_presenter_test`'s refusal branch never ran and the latch is untested (M2); the builds
  of the merged tree and of the head cited (M3: the fourth step review's two fixes are reached by no unit test; the
  witness `i138-t5e` runs them); the
  stills' menu-closed pair, « within 1 level » put as measured, the reader blind to clipped values (L1); files for the
  float picture's scan, the raw stills and the SDR programmes' hashes (L2); WARP's thin margin, the Reinhard entry
  without a model (L3); the debug layer's queue required in `hdr_output_test` (L4, 95 checks; that test alone built
  and run again on both adapters after `build-t5s6`); the departures gathered in the conclusion, the witnesses' TAA only,
  no HDR10 chain from the row on this screen (L5); the SDR claim cut to what was checked (L6).
- **The final review** (fresh, read-only, `main...559decb08`): no High; formats, states, lifetimes, threads, settings,
  the maths and the merge read and found right. **M1** the float picture was kept for the process's life at every resize
  of an HDR10 chain (introduced by the fourth step review's L1 fix: the stage asked from the engine thread, which submits
  no list): retired by the queue there. **M2** the paths no witness ran: named in the conclusion; the copy and a resize
  witnessed since (`i138-t5e`, below). **M3** the emissive glow against D-0276: ROADMAP item 5 says what is built, D-0276
  stands until Thomas answers (asked again at the tranche's close). L1 a live switch marked done though its resize was
  refused: asked again, five times, the journal row saying so; L2 the graded texels (« Tests »); L3 two claims of this
  section beyond the evidence: corrected; L4 our overlay's float pipeline made at its first float frame, its failure its
  own; L5 comments, two unused accessors; L6 the row offered on every screen and L7 no anti-aliasing without the TAA or
  DLSS: in the conclusion. Not done: a test hook forcing the colour space's refusal (the refusal's path stays unrun); a
  unit check of the picture's retirement. Applied in `718b41ed7`, its own step review below.
- **The fifth step review** (the final review's fixes, `559decb08..718b41ed7`): no High, no Medium; the picture's
  retirement, the retries, the overlay's pipeline and the restated LUT bound read and found right (the bound « derived,
  not just loosened »). Its lows: a new request inheriting the former one's tries, the give-up row without its own text
  (both done); the overlay's float failure silent (named in the overlay's failure text); the graded two-sided check able
  to pass without a graded texel (required); the 8-bit Reinhard programme's layout unchecked since part 4c (checked);
  `StripSlope` not a strict upper bound for every strip (a strip whose steepest edges do not meet at one node: a false
  failure then, never a false pass; left, said in the test). Applied in `3542b8cdf`.
- **The optimisation pass** (read, no measure: D-0227): the glow stage's four pipelines made by one function; the UI's
  layer, its clear, its second pipelines and the 8-bit picture's output programme gone with the first design; one probe a
  switch; our overlay's float pipelines made only on an HDR10 chain. No side-by-side sheet: the SDR picture's bytes are unchanged (above) and an HDR10
  picture cannot be judged on an SDR sheet; the OLED session is that judgement.

### The merge (2026-11-14)

`claude/frosty-newton-8ec678` merged into `main` as `2991edf9a` (after `main` `0af9e316f` merged into the branch, the five step
reviews, the final and the integrity reviews, `Test-Project -MergeGate`). The launcher's module rebuilt in the main checkout
(its unit tests passed); start check `start-2991edf9a` passed (11:22, restored). The disk rotation run (D-0260: 1,727 files, 1.3 GB, 5 build folders older than 48 h; it stopped when another thread's game or compiler started; 314.7 GB free on C:).

### Tranche 5 on the OLED (2026-11-14, the fresh R thread; Thomas asleep, the OLED left on by him for this)

Thomas switched the OLED on at 11:37 and Windows' HDR at about 11:50 (Win+Alt+B; he had authorized the thread to switch it,
11:31, which this session's own rules did not let it do: WORKFLOW « Thomas's PC and tools »), then left the screen on for the
technical checks: « c'est mieux de travailler le HDR avec une sortie HDR ». The judgement by eye (the knee, the glow, the
paper white, the UI over a bright scene) is still his, at his return.

- **The screen** (a Gigabyte MO27Q28G by its EDID name): the probe reads HDR on, ST 2084 over Rec. 2020 (colour space 12),
  10 bits, **1,532.3 nits at most, 295.4 over a full frame, 0.0002 at least** (`i138-t5g-auto`'s `native_hdr_output` row at
  start; the unit test's T6 at 11:58). One reading to remember: at 11:51, within about a minute of the switch, the same
  probe (the unit test's T6) read 240 nits at most; seven minutes later 1,532. Cause not established (INFERRED: Windows had
  not yet read the screen's HDR description); a game started that soon after the switch would take 240 as its automatic
  peak until its next probe (a resize, another screen, a stale factory).
- **The series** `i138-t5g-*` (11:57 to 12:16, the OLED, a 2560x1440 window, offline, no measure, the tranche-5 module
  `build-t5`, Eta's m40 save restored by hash after each of the seven runs; `evidence/I-0138/artifacts/tranche-5-oled/`,
  `Run-I138HdrMenu.ps1`'s new runs; Thomas watched part of it, 12:17: « ça fonctionne bien », « le feu ressort bien »).
  OBSERVED, every run without a fault, every frame's receipt at 0 debug-layer error:
  - `-auto` (`hdr-output.flag`): the HDR10 chain up by itself from the first frame (`native_hdr_output` at start: `hdr10`
    true, peak 1,532.3); three back buffers within 1.059 codes of the output's model on the float picture, mean -0.031; 0.62
    to 0.64 % of Firelink's picture above the paper white, the brightest value 4.44 to 4.78 times it (888 to 955 nits, `decode-auto.txt`; the integrity review's finding 2: the
    range first written here was another file's).
  - `-menu` (no flag): **the settings' row brings the real chain up** (« live, the chain resized », `hdr10` true), the peak
    row 400, 600, 400 and automatic, the paper white 250, 200, 150, 200, then off (the chain resized back to SDR), each
    followed live; 3,465 tone-map HDR frames.
  - `-autoresize`: the Resolution row to 800x450 and back to 2560x1440 on the HDR10 chain (15 and 23 ms), the pictures
    after it within 1.059 codes.
  - `-dlaa`, `-dlss` (Quality: 1707x960 rendered to 2560x1440): DLSS with Ray Reconstruction on the HDR10 chain, a back
    buffer of each within 1.058 codes of the model.
  - `-fg` (`fg-on.flag`): frame generation on from the world's first frame (597 of the next 600 presents generated a
    frame, status 0), a back buffer within 1.059 codes.
  - `-fullscreen`: exclusive full screen at 2560x1440 from the start, the chain HDR10 through the display-mode change
    (`native_display_mode` `fullscreen` true, three `native_hdr_output` rows, `hdr10` true).
  The three runs that the controller ended at their time limit (`-menu`, `-autoresize`, `-fullscreen`) show its own exit
  code 0xD3126612 after the module's clean close (`native_closed`, `debug_errors` 0): the limit, not a fault.
- **Still unseen**: a full-screen toggle made while the game runs (the run started in full screen); a chain that refuses
  the colour space; the brightness option away from its identity; Thomas's judgement by eye of the knee, the glow, the
  paper white and the UI over a bright scene.
- **Thomas's requests on the rows** (2026-11-14 12:05, the project chat): the HDR settings in the Graphics menu, not in
  Advanced Settings; sliders, not choice lists (steps of 10 or 50 at the thread's choice: 10 nits for the paper white, 50
  for the peak); the game's own brightness setting off while an HDR output is on. To do (a constraint read since: the
  Graphics tab's film has six slots, all taken, R12C-STATIC §5).

## Tranche 6: the Rekindled colorimetry's candidates (2026-11-14)

ROADMAP workstream 11, tranche 6 ([TRANCHE-6-READY](TRANCHE-6-READY.md), [PLAN](PLAN.md) §5), by the R pilot's fresh thread
(Fable 5.2, branch `claude/quizzical-chebyshev-e5cf9c`, from `main` `51df978b8`).

### Conclusion

- **Delivered**: three candidate looks beside the original, behind test flags only (no settings row, no default changed,
  no frame's picture changed without a flag): AgX after Filament's `AgxToneMapper`, the ACES fit of Stephen Hill, and a
  candidate after Elden Ring (its curve of contrast 1.4, the bloom added before the curve, its vignette, its lateral
  dispersion, no area grade), each in the SDR and the HDR10 entries of our tone-map pass; the keys F2 and F3 for a session
  with Thomas at the keyboard; the third-party notices. With it, the two items the study thread filed for the cutscenes,
  read in the executable and served: the ColorGrading LUT's name of a cutscene's file (K-0495), and `mapbase` answering map
  areas only (K-0496).
- **OBSERVED**: the original's five programmes hold the same instructions as on `main` (`programmes-hash.txt`); the build's
  tests at each step (177 lines at builds 2, 3 and 7, WARP and the card; the last build's count under « Reviews at the
  merge »); the six looks on a real HDR10 chain (the OLED, Windows' HDR on, `i138-t6b-*`: no fault, 0 debug-layer error),
  at step 3's build; the six looks at five zones on an SDR chain with the fixed build (`i138-t6e-*`, `i138-t6f-*`: no
  fault), their sheets sent to Thomas.
- **Not observed, left open**: **Thomas's pick** (the sheets are with him, his A/B by the keys on the OLED is to hold;
  nothing becomes a default without his word, and the pick's settings row is tranche 7's); the looks on the HDR10 chain
  after the second step review's fixes (the candidate after Elden Ring's dispersion and its gain's read changed since
  `i138-t6b-*`); the cutscenes' LUT name and the mapbase rule in a cutscene run against the original's captures; the cases
  listed under « Not tested » below.
- **What Elden Ring's look is in its post chain, measured** (Thomas, 2026-11-14 12:25: he means its art direction, vivid
  colours against DS3's grey): its LUTs are close to the identity; what differs from DS3 is the absence of desaturation
  and a contrast curve applied to each channel, which the candidate draws; the rest is its textures and lighting (« The
  candidate after Elden Ring » below).

### What is built (steps 1 and 2 of the sheet: the plumbing, AgX and ACES)

- **Two candidate curves inside our tone-map pass's one body** (`shaders/post_tone.hlsl`: `ps_agx`, `ps_aces` and their HDR entries
  `ps_hdr_agx`, `ps_hdr_aces`): the curve is a compile-time choice of the pass's one body (`ToneColour`), as the Reinhard
  curve is. A candidate replaces the record's curve and nothing else: the same exposed scene in, the glare blended after it,
  the area's colour matrix, the record's gamma, the dither and the LUT as before. **The original's five programmes hold the
  same instructions as before** (`ps`, `ps_reinhard`, `ps_lut`, `ps_hdr`, `ps_hdr_reinhard`: the listings without their
  comment lines, md5 equal before and after; `ps`, `ps_reinhard` and `ps_lut` at the values of
  `tranche-5-step2/sdr-programmes-hash.txt`).
- **AgX** after Google Filament (`ToneMapper.cpp`'s `AgxToneMapper`, the look NONE; Apache-2.0) **with the conversions
  Filament makes around it**: the sheet's §3.1 said the matrices convert to Rec. 2020 themselves; the source read says
  otherwise (`ColorGrading.cpp`: `sRGB_to_Rec2020` before the tone mapper, `Rec2020_to_sRGB` after it; the excerpt
  `tranche-6-licences/filament-ColorGrading-excerpt.txt`), so the pass applies both. **ACES** by Stephen Hill's fit as in
  Baking Lab's `ACES.hlsl` (MIT; `bakinglab-ACES-hlsl.txt`). One change to both: the curve's input is clamped to the largest
  half float (an infinite scene texel would make the fit's quotient a NaN, drawn black).
- **The grey match** (the sheet's §3.2, the recommendation applied): a candidate reads the exposed scene times one scale,
  such that the scene value the area's own curve shows as middle grey (0.18 linear) is shown as middle grey by the candidate
  (`tone_looks.h` `GreyScale`, `world/post_tone.h` `ApplyToneLook`: two bisections in double, kept by their inputs). A sheet
  then compares looks, not brightnesses. AgX's own grey input is 0.1452, ACES's 0.2631.
- **The highlights of a candidate in HDR** (a design of this tranche; the sheet's §3.6 proposed to keep tranche 5's rule on
  the curve's output): tranche 5's rule is, for the sensitometric curve, the curve's tangent at the knee followed in the
  scene's value (for `y = 1 - exp(-a x)` the logarithm of `hdr::Highlight` is exactly that line). Applied to the curve's
  output, the same logarithm would give AgX, whose curve ends at 0.961, at most 1.21 times the paper white. So a candidate's
  HDR entry follows its own tangent in the scene's value: above the grey scene value it shows at the knee, the gain is that
  line over the candidate's value of a grey of the pixel's largest channel (`look::KneeTangent`, `HighlightGain`; the block's
  words `look_a`, `look_b`). A knee of 1 gives no gain (the SDR picture inside the HDR10 chain).
- **The block** grows from 160 to 208 bytes (`look_scale`, `look_a`, `look_b`, `look_c`, the candidate after Elden Ring's `er_vignette` and `er_dispersion`); the look's number rides in two bits
  of the block's flags that no programme reads (the stage picks the candidate's pipeline, made at its first frame).
- **Test tools** (no settings row: tranche 7's): `tone-look-agx.flag`, `tone-look-aces.flag` (the area's colour matrix and
  LUT kept: the sheet's variant (a)), with `tone-look-neutral.flag` neither (variant (c)), `tone-look-er.flag` (step 3);
  `tone-look-cycle.flag` (original, AgX, AgX neutral, ACES, ACES neutral, the candidate after Elden Ring: the next at each
  picture `world-pictures.flag` takes, the first picture the original's); the runner `Run-StartupTrace.ps1` accepts and
  cleans the six flags; `tone-look-keys.flag` for a session with Thomas at the keyboard: F2 the next look, F3 the next
  HDR knee (0.75, 0.5, 0.9, 1), a label on the screen for 2.5 s (the overlay, the titles' font) and a row
  `native_tone_look` at each change. Without a flag nothing changes and nothing is read but the files' presence.
- **The notices**: `native/THIRD-PARTY-NOTICES.md` (new: what the module's own source took from published code) and
  `native/licenses/Apache-2.0.txt` (the licence's text, the copy shipped with Ghidra 12.1.4: 11,357 bytes, the plain text).

### Tests, builds, the step reviews and the runs (D-0268)

- **Tests added** (no existing requirement changed but two sizes that follow the block: `post_tone_test`'s layout table,
  twelve programmes and sixteen fields, and the automatic exposure's kernel block, `tone_rest[5]`, required equal to the
  block's size as before). `post_tone_test` part 3 (CPU): AgX and ACES at five colours each within 1e-8 of values
  computed apart (a Python script of the published arithmetic in double; the step reviewer recomputed them with its own
  script, to 1e-10), their grey inputs, a rising grey ramp, `ApplyToneLook`'s bits, scale (both curves at 0.18 for one
  scene value, within 1e-6), neutral matrix and tangent (the candidate's grey value at the tangent's input is the knee
  within 1e-9, the gain 1 there and rising), a knee of 1; the candidate after Elden Ring's curve, grey input, vignette
  words and factor at four places, dispersion words and block. `post_tone_stage_test` section 4 (WARP and the card): each
  candidate's 8-bit and HDR entries drawn from the capture's scene and the synthetic one against a CPU model of the pass as
  the test draws it (no glare surface, no area vignette, no exposure factor, no LUT), where the scene is finite: the HDR entry's floats within 0.5 % of the model's value plus 5e-4 (measured: the excess
  over 0.5 % at most 0.00029, once; 0 elsewhere), the 8-bit bytes within 1.1 level plus that bound (measured 0.97 to
  1.04); the candidate after Elden Ring modelled without its dispersion, and with its dispersion (since the second step
  review) its green and its blue bytes each against a CPU model of the channel's read (four texels toned and encoded, then
  blended: at most 1.037/255 over the six captures, WARP and the card), its red and alpha bytes equal to the draw without
  it (0 differ), its green and its blue bytes each differing (0.58 to 0.96 million a capture). The tolerances were set
  before the first run and not changed after it. Not tested: the 65504 clamp (the models skip the texels that are not finite), a knee other than
  0.75 on the card, the neutral variant on the card (the step review's L9); the candidate after Elden Ring's bloom before
  the curve with a glare surface, and its HDR entry with the dispersion on (its 8-bit entry alone is drawn with it): to add
  if Thomas keeps that candidate (the final review's L3). `glow_bank_test` (the final review's L2): a folder with a
  `mapbase` file answers a map area without its own file and does not answer a cutscene's area; `post_tone_test`: the
  LUT's name for an area's file, a base file, a cutscene's file, a file of another kind, an area without a map file
  (`GradingLutName`, taken out of the frame's preparation to be tested).
- **Builds**: `build-t6s2.log` (steps 1 and 2, `d18b6afa1`: 177 lines passed, WARP and the card), `build-t6s3.log` (step 3:
  177 passed), `build-t6s7.log` (the second step review's fixes: 177 passed). On the way `build-t6s6.log` failed one line,
  as it should have: `post_tone_stage_test`'s HDR check of the candidate looks (an error of 0.0886 beyond 0.5 % of the value,
  5e-4 allowed). The cause was in the fix of M1 itself: the HDR entries' gain was taken from a blend of the four
  neighbours' scene values, so a neighbour clamped at 65504 with a weight of a ten-thousandth set a pixel's gain. The
  gain is now read from the pixel's own texel (the largest of its three scene values in place); the test and its
  tolerance were not touched (measured after: 0.00004 to 0.00029). The original's five programmes' listings without their comment lines: md5 equal before the tranche and at
  each step (`ps` 4c7ff13f…, `ps_reinhard` 1176dbbc…, `ps_lut` 00eeb0e0…, the three of `sdr-programmes-hash.txt`; `ps_hdr`
  982af556…, `ps_hdr_reinhard` 9429e7a5…; stored at the merge, the final review's M2:
  `evidence/I-0138/artifacts/tranche-6-looks/programmes-hash.txt`, the five listings compiled from `main` `8a64a89a7` and from
  the branch, equal).
- **Steps 1 and 2's step review** (fresh, read-only, `51df978b8..d18b6afa1`): no High, no Medium; the matrices and
  constants found equal to the excerpts (Filament's columns, the Rec. 2020 wrap, the fit), the original's programmes and
  frames untouched without a flag, the cache, the pipelines' lifetime, the label's lock and the cycle's rule read and found
  right. Its Lows, each answered: **L1** a failed look's frame was neither the original nor the candidate (the look's bits
  cleared, the neutral matrix kept): the frame now takes the record's own block (`post_record`), the sheet's script refuses
  a run whose journal has a failed row; **L2** in HDR a candidate's gain reads the scene alone (the original's includes the
  glare blended after the curve) and has no ceiling but the output's roll-off (the original's logarithm ends near 4.4
  times the paper white): said here and in the programme's head, kept as built, for Thomas's eye on the OLED; **L3** the
  programme's comment claimed the logarithm equal to the tangent for the sensitometric curve (true to the first order at
  the knee only; exact for `1 - exp(-a x)`): reworded; **L4** the new statics and the per-thread cache outside the thread-
  affinity scanner: `NativeToneLookRequested` and `ApplyToneLook` added to its scope, five reviewed entries (four since the second review's
  fixes: `span` went with the cycle's rewrite); **L5** a
  neutral look moved the automatic exposure (its kernels read the block's matrix): the exposure now reads the record's own
  block, so every look of a view is drawn at one exposure; **L6** this section ran ahead of the commit (the block's size,
  the mapbase paragraph): true since the next commit; **L7** the shift taken out of the flags' enumeration (`look_c` and
  the third look serve since step 3); **L8** the sheet's script read the extent from a list of 16:9 widths: from the
  journal now; **L9** above (« Not tested »).

- **Step 3's step review** (fresh, read-only, `d18b6afa1..ef2bc98b2`): no High. **M1** the lateral dispersion did not give
  « the same picture as a pass after the tone map »: it filtered the linear scene before the curve, where ER filters the
  toned, encoded picture (its `ToneMap_PostOETFPS`, read by the reviewer): a saturated green and blue fringe of one to three
  pixels on every bright edge toward the sides. Fixed: `ErChannel` (four loaded texels, each toned and encoded, then
  blended; a texel that is not finite is made finite before the blend), the claim reworded to its limits. **L1** the
  dispersion's test held little (red equal by construction, alpha counted with the others): the green and the blue bytes
  are now each compared with a CPU model of the channel's read, the alpha required equal, green and blue each required to
  differ. **L2** the candidate after Elden Ring's model could have run on no texel: each candidate's own counts required
  and printed; a draw with a glare surface is still missing (said in the test's head). **L3** `mapbase` was gated on the
  caller's area, not on the key looked up, so the {1, 0} fallback could still answer from mapbase: the key's area now, {1, 0}
  reads `default` alone (this also ends mapbase's answer for a map key whose own files and mapbase lack the id while
  `default` lacks the group, and for area 1 and the {-1, -1} key: the engine's rule); no unit test builds a bank with a
  mapbase file (not done). **L4** a default aspect of 16:9: no default argument, no vignette for an unknown extent. **L5**
  the header's « nothing else of the frame » reworded; `ApplyToneLook` returns whether the look is neutral and the caller
  takes it from there.
- **The looks on a real HDR10 chain** (`i138-t6b-lookcycle`, `-lookkeys`, 12:35 to 12:40, the OLED, Windows' HDR on,
  `build-t6` at step 3; `evidence/I-0138/artifacts/tranche-6-looks/`): the cycle through the six looks, each its HDR
  entry, 0 debug-layer error over 4,490 frames, six back buffers within 1.060 codes of the output's model; the brightest
  value of the same Firelink view: the original 3.2 times the paper white (637 nits), AgX 3.1 with and without the area's
  grade, ACES 6.5 and 7.1, the candidate after Elden Ring 6.6 (1,318 nits; that candidate's figure is of step 3's build,
  before the second step review's fix of its dispersion and of its gain's read, and was not run again). The keys: F2 six times and F3 four times, each
  taken (eleven `native_tone_look` rows in their order), the label drawn (`step-1.png`; its first words used a plus sign, a
  bar and digits the titles' font lacks: letters only since, at the top right).
- **The first sheets** (`i138-t6c-*`, `i138-t6d-*`, 12:45 to 13:02, the OLED, an SDR chain, 2560x1440, `build-t6b`, the five
  zone saves restored by hash; `tranche-6-looks/sheets/`): Firelink Shrine's complete (the original and the five other
  looks from one still view), sent to Thomas with the High Wall's (five looks, its original missing). **The other zones'
  are not usable**: the cycle counted presentations from the first prepared world frame while the pictures wait for the
  zone (`PicturesWait`), so a slow zone's pictures started two or three looks late and the run ended before the last
  look; a picture taken as a run closed came out black. The cycle now follows the pictures taken (the first picture the
  original, the k-th the k-th look). **These sheets are superseded** (the integrity review's finding 1): their build
  predates the second step review's fix, so their candidate after Elden Ring shows the fringed dispersion.
- **The sheets** (`i138-t6e-*` at the five zones, 13:57 to 14:07, then `i138-t6f-m37` and `i138-t6f-m45` with a longer
  stay, to 14:14; the OLED, an SDR chain, 2560x1440, `build-t6` at build 7, seven runs without a fault, the zone saves
  restored by hash; `tranche-6-looks/series-sdr3.log` to `series-sdr5.log`): the six looks of one still view at Firelink
  Shrine, the High Wall, the Undead Settlement, Anor Londo and Ariandel (`tranche-6-looks/sheets-3/`, five sheets and their
  thirty pictures at full size), sent to Thomas at 14:17. Two things the runs showed about the tool: at three zones the
  first picture was taken on the loading screen (the pictures' wait lets it through), and at Anor Londo and Ariandel the
  first runs ended one look short. The cycle goes round while a run lasts, so the sheet's script now keeps, for each look,
  the first picture that shows the world (a mean level under 12 of 255 is a loading screen or a closing frame: 5.6 and 1.2
  measured on the two left out) and lays the looks in the cycle's order; `tranche-6-looks/sheets-2/` (the first-six rule on
  the same runs) is superseded too. The cycle itself still starts on the loading screen: a known limit of a test tool.

### The candidate after Elden Ring (step 3 of the sheet)

- **What was read first** (the sheet's §6 left the passage from ER's rows to its shader's constants INFERRED): a
  `ghidra-research` subagent read ER 2.7.1's tone-map setup `0x141b92690` and its helpers in the asm
  (`evidence/I-0138/artifacts/er-tonemap-constants/REPORT.md`, its scripts and outputs beside it); the pilot read again
  the vignette helper's asm (`FUN_141b8e7d0`: the four words as the report gives them) and the shader's listing
  (`ToneMap_TabledToneMapLUTPS`: the bloom added before the matrix and the table, the vignette's arithmetic, the gamma after
  it). Three corrections to what the study held: **the two ToneLateralDispersion rows are axes, not channels** (0.0012 across,
  0.0001 down; the red channel in place, the green read at half the shift, the blue at the whole); **the vignette multiplies
  the curve's output before the display gamma** with ER's default ToneMapParam (1, 1 / 2.2), where ER-VALUES said « on the
  display-encoded colour » (that default INFERRED: no writer of the flag found); **`g_GlareLuminance` is a constant 1**, the
  bloom's Luminance and Alpha being in the composite itself. The earlier note « Power raised to fov / FovDependence » was the
  feedback blur's word, not the vignette's.
- **What is drawn** (`ps_er`, `ps_hdr_er`, `ToneColourEr`; no code of Elden Ring's): our bloom added to the exposed scene
  before the curve; ER's curve `t = c^1.4, y = t / (t + 1)` (its ReinhardContrast's median; its scale is ours, the grey match:
  ER's exposure is in its own scene's units); its vignette with its typical rows (Power 0.5, Range 1.25, Shape 0.2, Fov
  Dependence 0: at a 16:9 picture's corner the factor is 0.096 in linear light, 0.40 at the middle of a side), off with the
  player's vignette setting; its lateral dispersion inside the pass, read as ER's pass after its tone map reads it: for the
  green and the blue channel the four texels around the channel's place are each toned and encoded, then blended by the
  linear filter's weights (the second step review's M1: the first form filtered the scene before the curve and fringed
  every bright edge; what still differs from a separate pass: the bloom and the vignette read at the place, the dither
  added in place, the HDR entries' gain one factor a pixel); no area vignette, colour matrix or LUT (ER's own matrix is neutral in 278 of 279
  entries and its look is in LUT images we do not have, D-0276). `tone-look-er.flag`; the last step of the cycle and of F2.
- **Not done**: ER's bounded automatic exposure (its soft clamp of the adaptation between its Adaption Min and Max rows,
  `FUN_141b906a0`: read; tranche 3's test flag for it was removed when Thomas kept our half strength); to propose again to
  Thomas once he has seen this candidate. ER's own bloom values (threshold, luminance) are not taken: the bloom is DS3's.
- **What Thomas means by « Elden Ring » here** (2026-11-14 12:25 to 12:28, the project chat, his words): « quand je
  parlais de colorimétrie d'ER je faisais notamment référence à sa DA et à ses couleurs vives comparées au gris de DS3. Et
  oui ça implique un changement de DA inhérent, je le sais, c'est pour ça qu'on garde l'original en option » ; « A lire
  dans le code de ER je suppose » ; and on ER's data: « Si ça peut aider en local, on peut copier. Mais oui il faudra dans
  tous les cas faire autrement pour la distribution ». So this candidate's target is ER's vivid look, not its curve alone:
  the curve, vignette and dispersion above are its frame; the look itself is where ER keeps it, in its colour-grading LUTs
  (ER-VALUES line 5: ColorGrading on in 223 of 252 gameplay entries) and in DS3's own grey grade, which the neutral
  variants already remove (the areas' desaturating matrix and LUT).
- **ER's LUTs, read** (`evidence/I-0138/artifacts/er-luts/REPORT.md`: a worker's extraction of 76 unique LUTs from 20 of
  ER's `mAA_cgrading.tpf` files and their measure against the identity; one LUT read again by the pilot): **they are close
  to the identity**. Median chroma gain 1.03 (the overworld's day cycle 1.00 to 1.03), HSV saturation 1.00, a lift of the
  midtones and shadows (grey node 8 from 136 to 149..173), slightly cool blacks, white kept at 255. DS3's nine LUTs read
  beside them: chroma 0.98, a warm white point. So ER's LUTs do not make it vivid. In the post chain the difference is: no
  desaturation (ER's ColorS is 1 in 278 of 279 entries; DS3's areas are at 0.8 to 0.9, CENSUS-WORLD), a curve of contrast 1.4
  applied to each channel (which raises saturation), LUTs that do not dull; the rest is ER's textures and lighting, which
  are not post-processing. The candidate after Elden Ring already draws the first two. Not done: a trial with an ER LUT
  itself (its midtone lift; allowed locally by Thomas, D-0276's « without its LUT images » standing for anything
  distributed), a saturation above 1 (a « more vivid » variant, if Thomas wants it after the sheets). Thomas's word on local copies: D-0279.

### The cutscenes' LUT (STUDY-UNREAD-SETTINGS item 1, read and served)

- **The original's rule** (VERIFIED: GhidraMCP on DarkSoulsIII.exe 1.15.2, a `ghidra-research` subagent's read, the two
  deciding functions decompiled again by the pilot): the LUT texture's name is `"%s%04d"` (14021CC20) of a prefix the
  record keeps at +0x20, built by 14017B270 as `"%s%s"` of the gparam file's derived name and « CGrading ». The derived name
  is made once a parsed file (140D272C0 → FUN_140095100, the file entry's +0x350) by **FUN_140D28490**: `"m%02d_%02d_"`
  from the AA and BB of the file's own name when its kind is 7 (`mAA_BB_VVVV`) **or 0xB (`sAA_BB_VVVV`, a cutscene file)**,
  the empty string for every other kind. So a cutscene record reads **the map's own strip** `mAA_BB_CGrading<id>` (the
  cutscene's VVVV does not enter the name), and a record of `mapbase`, `default` or `remobase` asks `CGrading<id>`, a name
  no shipped texture is known to carry (INFERRED: no name search over the TPFs was made). `FUN_140095220` is only the lookup
  of the file entry's flag byte (+0x348, set for every parsed file); the two branches the earlier comment named are
  14017B270's.
- **The native** refused every such record « non-map record (prefix unread) » (26 of 49 cutscene files enable the
  grading). It now names a cutscene file's strip as the original does, and asks `CGrading<id>` for the other files (refused
  « texture absent » when no texture has that name, as the original draws none). Not witnessed yet in a cutscene.

### A cutscene key whose own file lacks a group (STUDY-UNREAD-SETTINGS item 2, read and served)

- **The original's rule** (VERIFIED: the same kind of read, `evidence/I-0138/artifacts/tranche-6-ds3-reads/REPORT.md`; the lookup
  `GparamBankLookup_140032770` and the binder `GparamAddExtraId_140D26AB0` decompiled again by the pilot): a bank lookup knows the
  exact key {area, id}, then {1, 0} (`default`), then the bank's built-in record, nothing else; `mapbase` answers a map key
  only because its records are bound under the area of each loaded map (`MapIdToAreaCode` of the map id: 100,000,000 to
  199,990,000), never under a cutscene file's area (200,000,000 and above). So a cutscene key whose s-file lacks the group
  reads `default`'s {1, 0}: the study's inference holds. `remobase` (bound under area 200,000,000 with weight 0) answers no
  key found, and `MapOverride_forRemo` is a copy of `empty` (no row).
- **The native** (`Banks::GroupEntries`, `FindGroupParam`) fell back to `mapbase` for any area; it now does so for map areas
  only. What changes on screen, for the cutscenes whose s-file lacks the group (17 of 49 lack Setting Info, 46 MotionBlur, 48
  Anti-Alias; the study's list): `default`'s values in place of `mapbase`'s (GlareQuality 4 for 2, DepthOfFieldQuality 7 for
  3, Enable ColorGrading 0 for 1, MotionBlur's BlurTimeRatio 0.06 for 0.2). `Banks::Shadow`'s own mapbase fallback is left as
  it is (the cutscenes' shadow parameters are being read by the E pilot the same day). Not witnessed yet in a cutscene.

### Reviews at the merge (the tree with `main` merged in, `cd2712bdb`)

- **The final review** (fresh, read-only, `main...cd2712bdb`): no High; the native code found right (the candidates' entries,
  pipelines made once, the look's bits stripped from the block the GPU gets, the block's size and packing, the models
  against Filament's and Baking Lab's sources, the thread safety of the test tools, `Banks::Shadow` untouched, nothing
  changed without a flag but its L1). Two Mediums on the repository's records: **M1** the merge of `main` had kept this
  branch's older words for the E pilot in two rows of STATE (a merged tranche written « on its branch », a line of its next
  action cut): `main`'s words restored. **M2** the first exit criterion (the original's five programmes unchanged) rested
  on this text alone: `programmes-hash.txt` stored (the five listings compiled from `main` and from the branch, equal).
  Its Lows: **L1** the keys' label took its lock at every frame in play without a flag: the flag is read first. **L2** the
  mapbase rule's change also reaches map frames, which was not said: a map key whose own files and mapbase lack its id
  reads `default`'s id 0 alone, where the fallback read the area's own file at id 0 before (the engine's rule, K-0496;
  `default` and `mapbase` both lack Dominant Light, Snow and Snow Model, so those keys get the built-in record as before);
  and no test built a bank with a mapbase file or named a cutscene's LUT: both added (`glow_bank_test`, `post_tone_test`
  with `GradingLutName` taken out of the frame's preparation). **L3** the candidate after Elden Ring's bloom before the
  curve is never drawn with a glare surface, nor are the clamp, the neutral variant and another knee on the card: not
  added, listed under « Not tested » (a candidate behind a test flag; to add if Thomas keeps it). **L4** this section's
  stale lines and its missing parts: written. **L5** the WORKFLOW's exception on Windows' HDR switch (Thomas's words in the
  R thread, 11:31 and 11:37) had no echo: the CONTRACT's sentence amended; no decision-log entry (a working rule, in the
  WORKFLOW with his words).
- **The integrity review** (fresh, read-only, the same diff and this section): no High; no test loosened, no tolerance
  changed after a run, no case written for one scene, nothing cut during a comparison, the evidence it sampled matching.
  **1 (Medium)** the stills Thomas had received came from builds before the second step review's fix: the sheets run again
  on the fixed build and sent, the earlier ones and the HDR figure marked. **2** a range of nits copied from another file:
  corrected. **3** the programmes' hashes without a stored file: stored (M2). **4** « the CPU model of the whole pass » said
  more than the test draws: reworded, the gaps listed. **5** « five reviewed entries » where four remain, and the runner's
  six new flags unnamed: both written.
- **The last build** (the fixes of both reviews: the label's flag read first, `GradingLutName`, the two tests): `build-t6s8.log`, 177 lines passed, WARP and the card (`glow_bank_test` 38 checks, `post_tone_test` 357).
- **The fixes' step review** (fresh, read-only, `cd2712bdb..b127e4bc0`): no High, no Medium; the LUT's name found equal to the
  removed lines case by case, the label's flag read before its lock, the new tests failing if their rule changed. Two Lows,
  answered in words only (no instruction of the module changes; the launcher's build runs the tests again): the mapbase
  test's comment claimed the key {1, 0} too, which its files cannot tell (reworded: not checked there); the sheet's script
  sorted looks outside the cycle's six in no promised order (last, by name).
- **The optimisation pass** (read, D-0227): nothing removed. The candidates' pipelines are made at their first frame and
  kept; a frame without a flag takes one comparison in `NativeToneLookRequested` and none in the pass; the candidate after
  Elden Ring reads the glare three times and the scene nine times a pixel, which only its own frames pay.

### Evidence

- Builds: `evidence/I-0138/artifacts/build-t6s2.log`, `build-t6s3.log`, `build-t6s6.log` (the failed line), `build-t6s7.log`,
  `build-t6s8.log`; the module of the last one in `build-t6` (Thomas's A/B uses it).
- `evidence/I-0138/artifacts/tranche-6-looks/`: `programmes-hash.txt`; the HDR runs' log and decoded back buffers
  (`series-hdr.log`, `decode-lookcycle.txt`, six `hdr-lookcycle-*-sdrview.png`); the sheets' series (`series-sdr.log` to
  `series-sdr5.log`); `sheets-3/` (the sheets sent to Thomas); `sheets/` and `sheets-2/` (superseded, above).
- The runs' folders under `evidence/I-0066/artifacts/`: `i138-t6b-lookcycle`, `i138-t6b-lookkeys`, `i138-t6c-*`, `i138-t6d-*`
  (superseded), `i138-t6e-*`, `i138-t6f-*`.
- The reads: `tranche-6-licences/` (the published code the two curves come from), `er-tonemap-constants/` and `er-luts/`
  (Elden Ring, read-only, its LUT files kept on this PC only), `tranche-6-ds3-reads/` (the LUT's name and the mapbase
  rule in the executable).

### Uncertainty / unresolved questions

- Which look Thomas keeps, with or without the area's grade, and whether « Elden Ring » as he means it needs more than
  this candidate (a saturation above 1, an ER LUT's midtone lift tried locally).
- The candidates in HDR: a candidate's gain reads the scene alone and has no ceiling but the output's roll-off (the first
  step review's L2), kept for Thomas's eye on the OLED; not seen by him yet.
- The candidate after Elden Ring is not Elden Ring's pass: its bloom, vignette and dither are read in place, its HDR gain
  is one factor a pixel, its automatic exposure is DS3's (ER's bounded one is not built).
- The cutscenes' LUT name and the mapbase rule are read in the executable (K-0495, K-0496, SUPPORTED) and unit-tested,
  not witnessed in a cutscene. The E pilot found the same day that the native skipped the binding of a cutscene's file
  to the map's rotation (the Remo step at CDEE50; its fix is E's): the cutscenes' per-draw lights of this workstream were
  without that rotation too, to look at again once E's fix is in `main`.
- The cycle of the sheets starts on the loading screen (the sheet's script leaves that picture out).

### Artifacts produced or modified

- New: `native/renderer/tone_looks.h`, `native/THIRD-PARTY-NOTICES.md`, `native/licenses/Apache-2.0.txt`,
  `investigations/I-0138-modern-post-chain/Make-I138LookSheet.ps1`.
- Changed: `native/renderer/shaders/post_tone.hlsl`, `post_params.h`, `post_tone_stage.{h,cpp}`, `scene_pass.h`,
  `scene_pass_frame.cpp`, `model_frame.h`, `auto_exposure.cpp`, `auto_exposure_shader.h`; `native/world/post_tone.h`,
  `light_producers.cpp`; `native/frame/game_bootstrap/02_frame_lifecycle.inc`, `04_updates_and_test_tools.inc`,
  `13b_prepare_model_frame.inc`, `16b_rekindled_overlay.inc`; the tests `post_tone_test.cpp`, `post_tone_stage_test.cpp`,
  `glow_bank_test.cpp`; `native/frame/Build-GameBootstrap.ps1`; `scripts/Test-ThreadAffinity.ps1`,
  `scripts/thread-affinity-allow.json`; the runners `Run-I138HdrMenu.ps1` and `Run-StartupTrace.ps1`; `knowledge/LEDGER.md`
  (K-0495, K-0496); `.project/WORKFLOW.md` (the OLED rule's exception on Windows' HDR switch, Thomas's words).

### The merge (2026-11-14)

Merged into `main` as `1def62857` (14:37; the branch's tip `5c5a7cacd`). The launcher's module rebuilt in the main checkout
(`evidence/I-0138/artifacts/build-launcher-1def62857.log`: 177 lines passed, WARP and the card). Start check
`start-1def62857` passed on the OLED in a 2560x1440 window (14:52 to 14:53: at the world in 3.75 s, closed by its close
box, no refusal, no failure, the save restored). The disk rotation at the checkpoint stopped by itself (a compiler ran: the
next tranche's build): 0 files deleted, 280.3 GB free on C:.

## Tranche 5b: the HDR rows on the Graphics tab, as sliders; the game's brightness off in HDR (2026-11-14)

ROADMAP workstream 11, a follow-up of tranche 5 asked by Thomas in the R thread (2026-11-14 12:05: « Je préfère que les
réglages du HDR soient dans le menu graphics et pas advanced. Aussi, je préfère que ça soit des curseurs plutôt que des
sélections. Ça peut aller par tranche de 10 ou de 50 (au choix de R) », « Il faudrait également que le réglage de luminosité
du jeu de base se désactive si une sortie HDR est détectée, pour pas faire doublon ni perturber »). By the R thread (Fable
5.2, branch `claude/quizzical-chebyshev-e5cf9c`, after tranche 6's merge).

### Conclusion

- **Delivered**: on the options' Graphics tab, after the frame rate: « HDR output », then « Peak brightness (nits) » and
  « Paper white (nits) » as the game's own slider rows (400 to 4000 nits by 50 with « Auto » one step under, 80 to 500 by
  10), greyed while the output is off, followed live; they left the Advanced Settings page. On an HDR10 chain the game's
  brightness option no longer changes the picture and the options' Brightness tab shows a locked row saying so. The tab
  gets its three more slots from the options film as the game loads it (nothing on disk).
- **OBSERVED** on the OLED, Windows' HDR on (two witnesses, the second in French, no fault): the real HDR10 chain brought
  up from the row, every slider step followed by the chain, the Brightness tab locked on the HDR10 chain and free again on
  the SDR one, the settings back as they began; the builds' tests (178 lines, WARP and the card).
- **Not observed** (listed under « Uncertainty »): the Defaults row's press, the page keys, a mouse drag, a film that is not
  the shipped one, the new game's brightness screen on an HDR10 chain, other languages than English and French.
- **For Thomas**: the step sizes are the pilot's (50 and 10 nits, his « au choix de R »); the On and Off words of the HDR
  row are the game's own (« Oui », « Non » in French, as V-sync's); the rule keyed to one film is an exception to D-0161
  (D-0279 since).

### What was read before the code (the executable through GhidraMCP, the options film from the game's data)

- **The game's slider row keeps an int and draws it** (a worker's read, then the pilot's own of each function used):
  `FUN_140AA43C0(page, label, byte*, range, default*)` makes a row of 0xCA8 bytes (`FUN_140AA5940`) whose slider widget is at
  +0x98; the widget's value is an int at +0xBC8 between the range's min and max (+0xBD0, +0xBD4), moved by the step (+0xBD8)
  and, when the range's fourth byte is set, by ten steps on the page keys (`FUN_140A53AF0`). The game draws the bare int
  beside the bar (`FUN_140A53C20`: the child « Text_0 » of the slider's clip at +0x9D8, `FUN_140A51590` with « %d ») at each
  change and when the baseline is set (`FUN_140A537B0`). The row's pointer gets only the low byte of the value (the lambda
  at `140AB4E40`), and the row's default is one byte (`FUN_140AAC470`, the row's vfunc +0x18, which the Defaults button calls
  for every row through `FUN_140A99DC0`). So a slider can show nits (80 to 500, 400 to 4000) if its int is set and read in
  the widget itself, which is what the code does.
- **The Graphics tab's six slots are a property of the film, and three more fit** (`menu/win/02_040_optionsetting.gfx`, 32,917
  bytes, a plain GFX movie; read with a small parser kept outside the repository): the sprite « PCGraphic » (character 140)
  places `Item_0_0` to `Item_5_0` 50 px apart; no ActionScript of the film names a slot; the game's list counts the slots it
  finds by name (`FUN_140A40500`: up to 64 rows) and gives a row the slot of its index (`FUN_140A90E20`). Three PlaceObject2
  tags (74 bytes: `Item_6_0` of the generic kind 92, `Item_7_0` of the kind 131, `Item_8_0` of the kind 92, at 100, 150 and
  200 px) added before the sprite's ShowFrame, the sprite's and the file's lengths grown by them, give a film that parses
  with nine slots and nothing else changed (the offline patch of the shipped film, SHA-256 `c7d10658…`; the module's own
  function gives the same bytes). The game hands each film's bytes to its Scaleform repository through `FUN_140E0EA10`, which
  copies them (`FUN_140E0F3C0`); the module already observes that call (the overlay's fonts).
- **The tab's Defaults row** is `FUN_140AAEC10(page)`: the options' tab dispatcher (not a function in Ghidra; its cases
  decoded from the bytes at `140AB37F0` to `140AB3873`) passes it as the defaults function for « PCGraphic » alone, and
  `FUN_140AAC6A0` as the row builder for « BrightnessSetting ». `CreateOptionSubPageByName_140AB34B0` (read by the pilot):
  after the row builder it adds the page's last row: the caller's defaults function when it gave one, else `FUN_140AAD1B0`
  (the other tabs' Defaults row), or, when its seventh argument is 1, `FUN_140AAFA50` (the new game's brightness screen's
  own button); it keeps its sixth and seventh arguments at page + 0x1160 and + 0x1161, set before the builder runs.
- **The game's Brightness row** is the one slider of `FUN_140AAC6A0(page, state)` (state + 2, 0 to 10; R12C-STATIC §8.5 had
  taken it for the camera speed's: corrected there); the brightness screen of a new game goes through the same builder
  (`FUN_140BE2DC0`). The option reaches the picture through the display curve (`FUN_1405A0BE0`, the write side of the render
  step's « amount x scale + offset »).

### What is built

- **The film's three slots** (`native/frame/options_film.h`, header-only; `16b`'s observer): when the bytes handed to the
  repository are the shipped film (their SHA-256), the slots are added and the result's SHA-256 checked against the offline
  patch's before it is handed on; anything else leaves the film as it is and the tab with its five rows: **the HDR output
  then has no row at all** and follows the settings file alone. A row `native_options_film` says which for a film of the
  shipped size (and with `no-options-film-slots.flag`); a film of another size, or `no-rekindled-overlay.flag` (the
  observer is the overlay's), leaves no row. Nothing is written to disk, no file copied. The rule is keyed to one film by
  its name, size and SHA-256: an exception to D-0161 (rules per family), as D-0275's was for one movie; it follows from
  Thomas's request (the tab's film has six slots) (D-0279).
- **The Graphics tab** (`02b`): Screen Mode, Resolution, V-sync, the frame rate, **HDR output** (the game's On/Off row),
  **Peak brightness** and **Paper white** (the game's slider rows while the HDR output is on; greyed locked rows showing the
  value while it is off), Advanced Settings, then the tab's own Defaults row. The peak: 400 to 4000 nits by 50, and one step
  under 400 the automatic value (the screen's own maximum); the paper white: 80 to 500 nits by 10; the page keys move ten
  steps. A slider shows its number, as the game writes it, and « Auto » for the automatic value (our word written over the
  game's number at each update of the tab); the unit is in the two labels, « (nits) » (the first witness: the slider's text
  field holds five letters, « 450 ni », « Autom »). A value of the settings file off the steps is shown as the nearest step
  and kept until the slider moves. A slider's value is handed to the settings file and the chain once the slider has been
  still for 250 ms (the step reviews: each taking writes the file to the disk and quiesces the renderer); the HDR output's
  switch at once. Switching the HDR output builds the tab's rows again at its next update (as the Advanced
  page does). The three rows left the Advanced page (`graphics_rows.h`: the `RtRow` list and its choice lists lose them; one
  implementation).
- **The game's brightness on an HDR10 chain**: the option no longer reaches the output pass (tranche 5 applied it as a
  power under the paper white: `display_power`, the frame job's field, the output block's second power and the shader's
  second `pow` are removed), and while the chain is HDR10 the options' Brightness tab shows a greyed locked row with the
  game's own label and « No effect in HDR » in place of its slider (`RekindledBrightnessRows`, a detour of
  `FUN_140AAC6A0`). The options keep each tab's page while they are open (the first witness), so the page's row is built
  again at its next update when the chain's kind changed since (`RebuildBrightnessPage`, with the page's Defaults row
  `FUN_140AAD1B0`). The brightness screen of a new game keeps its slider (its film's slot is not read: the step review's
  M1); there the slider changes nothing on an HDR10 chain. On an SDR chain nothing changes.

### Tests, the step review, the witnesses (D-0268)

- **Tests**: `options_film_test` (new): a slot's tag written as the shipped film writes its own (three of its tags, 21 to
  24 bytes each, read from the game's film), a synthetic movie that keeps every byte outside the insertion and grows its
  two lengths, six malformed inputs refused, the three slots 74 bytes; the shipped film itself is not in the repository:
  the module checks its SHA-256 and the result's. `graphics_rows_test`: tranche 5's checks of the three choice rows went
  with the rows (their lists, their words, their writes); in their place the sliders' model: every step of both sliders
  through the settings file and back, the automatic value, a file's value off the steps kept until a move, values outside
  a slider brought inside, the words shown (five letters at most), the labels' unit. `hdr_output_test`: the two cases of the
  brightness option's power and the check that it stopped at the paper white went with the parameter (95 checks before,
  88 now, WARP and the card: seven less, the two power cases' code checks, the two checks of the highlight kept and the
  dark made darker, and the `highlight_gamma` field in the layout of each of the three programmes that hold the block; no
  tolerance changed: the float picture's codes still within one of the model's at both peaks).
- **Builds**: `evidence/I-0138/artifacts/build-t5b-1.log` (step 1: 178 lines passed, WARP and the card), `build-t5b-2.log` (the
  first witness's and the step review's fixes: 178 passed), `build-t5b-3.log` (step 2's review: 178 passed), `build-t5b-4.log`
  (the tree with `main` merged in, the E pilot's tranche 7: 178 passed), `build-t5b-5.log` (the final review's fixes: 178 lines passed, WARP and the card).
- **Step 1's step review** (fresh, read-only, `787a01b1a..c4b4a55eb`): no High; the threads, the guarded pointers, the rows'
  logic, the film's pins and the renderer's change found right. **M1** the Brightness lock's slot was not read and the new
  game's screen not witnessed: the options' tab's slot is the generic kind 92 (the film's sprite, read); the new game's
  screen is left with its slider. **M2** unverified whether a slider's step calls the tab's apply into a mode set or a
  config save: the first witness's journal has no resize, mode or save row during the sliders' moves (the HDR output's
  two switches each resize the chain's buffers, as they must). **L1** a disk flush and a quiesce at every step: settled
  first (above; by a time, 250 ms, since step 2's review). **L2** the words written at every update: kept, the reason in the code (the game writes its number again
  when it sets a slider's baseline, with no change to see). **L3** a greyed row's default not among its choices: its one
  choice now. **L4** a stale tab pointer after an early close: not changed; the review's fix (forget the tab when another
  page is built) is wrong since the pages are kept, and the tab is forgotten when its address shows another window.
  **L5** the runner's default run: `tab`. **L6** a comment in `options_film.h`: corrected.
- **Step 2's step review** (fresh, read-only, `c4b4a55eb..a494aa48a`): **High**: the Brightness page's address could be
  kept after the options closed, and a new game's brightness screen made at that address (the same class, the same two
  rows) would then have had its rows rebuilt and its own button replaced by a Defaults row. Fixed: the kept page is
  forgotten whenever the builder runs for any page, and a page is ours only as an options' tab (page + 0x1161 zero) with
  the list its build left. Lows: the settle as a count of frames depended on the frame rate (a time now); a snapshot can
  be taken twice (the second writes nothing); the two reads of `CreateOptionSubPageByName` the code relies on were not in
  this section (written above).
- **The first witness** (`i138-t5b1-tab`, 15:24 to 15:29, the OLED, Windows' HDR on, a 2560x1440 window, offline, no
  measure, `build-t5b` at build 1, Eta's save restored by hash; `evidence/I-0138/artifacts/tranche-5b/series-1.log`,
  thirty-five stills `options-step-<i>.png`): no fault. OBSERVED: the film taken with nine slots (`native_options_film`:
  « patched », 32,991 bytes); the tab built with eight rows and Defaults in the ninth slot, drawn as the game's own rows
  are; the HDR output's row brings the real HDR10 chain up (« live, the chain resized », `hdr10` true, peak 1,532.3) and the
  two rows become sliders; the peak 400, 450, 500, 450 and the paper white 210, 220, 210, 200, 190, each followed live;
  back to automatic and 200; off: the chain SDR again, the two rows greyed with their values. Shown wrong, fixed since:
  the sliders' words cut at five letters; the Brightness page, built during the tabs' tour before the switch, kept its
  slider on the HDR10 chain.
- **The second witness** (`i138-t5b2-tab`, 16:15 to 16:21, the same screen and steps, the game in French
  (`-TestLanguage french`), `build-t5b` at build 3, restored by hash; `tranche-5b/series-2.log`): no fault. OBSERVED: the
  same rows as the first witness (the chain HDR10 from the row, each slider's step followed live, fourteen writes of the
  settings file, back to automatic, 200 and SDR); the sliders show « 450 », « 200 » and the labels « └ Luminance maximale
  (nits) », « └ Blanc papier (nits) » whole (`options-step-11.png`); the Brightness tab, built during the tour on the SDR
  chain (`"built":"open","locked":false`), is built again locked when visited on the HDR10 chain (« Régler la luminosité :
  Sans effet en HDR », greyed, the cursor on « Par défaut »: `options-step-18.png`) and built again unlocked once the
  chain is SDR (the journal's row). **Corrected by tranche 5d's final review**: this sentence said « again with its slider »,
  which the run's own still contradicts (`options-step-32.png`: no row drawn; the slider was there and hidden): a claim written
  from the journal, not from the picture. Fixed in tranche 5d. The two stills sent to Thomas. **Not observed**: the Defaults row's press (it resets the game's graphics
  settings too); the page keys' ten steps; a mouse drag of a slider; the tab with a film that is not the shipped one; the
  new game's brightness screen on an HDR10 chain; another language than English and French.

### Reviews at the merge (the tree with `main` merged in, `dca6dd8ad`)

- **The final review** (fresh, read-only, `main...dca6dd8ad`): no High, no Medium; the plan found met (nine slots, eight
  rows and Defaults, the sliders' bounds tied to the settings', the Advanced page's rows and writes gone, the film changed
  only on its pins, the brightness power removed end to end, the threads, the identity checks). Its Lows: **L1** the greyed
  rows of the tab and of the Brightness page carried the Advanced page's change callback, which writes the preset's state
  and runs the page's apply: probably never called for a locked row, but a Defaults press there was not witnessed: the
  callback now returns for any page but the Advanced one. **L2** the Brightness lock followed a flag set where the HDR
  output's row is logged, while a resize the system refuses can change the chain's kind without it: the chain's kind is
  read at every engine frame. **L3** a kept page is known by its address, its class, its rows' count and its list: a
  pending rebuild could reach another page built at the same address with the same list and count after the options were
  closed and opened again: not changed; the tab's rebuild exists only with its nine rows, which no other options page
  has, and the pages are kept while the options are open, so forgetting the tab whenever another page is built would
  break it. **L4** the film's pin against D-0161: said above (D-0279 since). **L5** a wrong comment and a
  wrong claim on the fallback: both corrected above. **L6** the unwitnessed cases: listed. **L7** the runner's old `menu`
  run removed; STATE, this section's head and the ROADMAP's items written at the merge.
- **The integrity review** (fresh, read-only, the same diff): no High, no Medium, nothing hidden; the removed tests match
  the removed features, no tolerance changed, the evidence it sampled matching. Four Lows of wording, each corrected: the
  settle said « ten engine frames » where the code waits 250 ms; the film header's fallback comment; the `hdr_output_test`
  count given without its drop from 95; the merged tree's build not recorded yet (recorded).
- **The fixes' build**: `build-t5b-5.log`, 178 lines passed, WARP and the card. **Their step review** (fresh, read-only,
  `dca6dd8ad..6d35e0050`): no High, no Medium; the callback's guard, the chain's kind read on the engine thread and the
  runner found right; two Lows of words (the runner's head, this line), corrected.

### The merge (2026-11-14)

Merged into `main` as `76f77782d` (17:11; the branch's tip `f649aa395`). The launcher's module rebuilt in the main checkout
(`evidence/I-0138/artifacts/build-launcher-76f77782d.log`: 178 lines passed, WARP and the card). Start check `start-76f77782d` passed on
the OLED in a 2560x1440 window (17:25: at the world in 3.16 s, closed by its close box, no refusal, no failure, the save
restored). The disk rotation at the checkpoint (`Clear-DiskCache.ps1 -Apply`): 26,943 files deleted, 20.0 GB (79 build
folders older than 48 h), 283.5 GB free on C:.

### Evidence

- Builds: `evidence/I-0138/artifacts/build-t5b-1.log` to `build-t5b-5.log`; the module of the last one in `build-t5b`.
- `evidence/I-0138/artifacts/tranche-5b/`: `series-1.log`, `series-2.log` (the two witnesses).
- The runs under `evidence/I-0066/artifacts/`: `i138-t5b1-tab` (English, build 1), `i138-t5b2-tab` (French, build 3): the
  journals' rows `native_options_film`, `native_rekindled_graphics` and `native_hdr_output`, thirty-five stills each.
- The film's study (outside the repository and the store, on this PC): `C:/Users/User/Tools/DS3-Decompilation/menu-film-study/`
  (the film read from the game's data, a small parser, the offline patch); the patched film's SHA-256 is pinned in
  `options_film.h`.

### Uncertainty / unresolved questions

- Not witnessed: the tab's and the Brightness page's Defaults press with the new rows (the code: each row resets to its
  default, off, automatic, 200; the callback of a locked row now returns); the page keys' ten steps and a mouse drag on a
  slider; the tab with a film that is not the shipped one (five rows, no HDR row); the new game's brightness screen on an
  HDR10 chain (left with its slider, which then changes nothing); languages other than English and French (their words
  are our English, as the other rows of ours).
- With HDR off, Defaults does not bring the peak and the paper white back to their defaults (their rows were locked, and
  are absent since tranche 5d): they come back when Defaults is pressed with the output on.
- The second witness ran on build 3; the fixes of the final review (builds 4 and 5) changed no row's building: checked by
  the build's tests and the reviews, not by a third run.
- This tranche's two points that are Thomas's (the HDR rows' place and form, the exception to D-0161) are D-0279 (his
  « Oui » in the R thread, 2026-11-14 19:57).

### Artifacts produced or modified

- New: `native/frame/options_film.h`, `options_film_test.cpp`.
- Changed: `native/frame/graphics_rows.h` and its test; `native/frame/game_bootstrap/02b_rekindled_graphics.inc`, `16b`
  (the observer), `22_setup.inc` (the sixth hook), `15_cpu_service_and_input.inc`, `01_state.inc`, `02_frame_lifecycle.inc`,
  `10_options_and_passes.inc`; `native/renderer/hdr_output.h`, `shaders/post_output.hlsl`, `scene_pass.{h,cpp}`,
  `scene_pass_frame.cpp`; `native/frame/hdr_output_test.cpp`; `native/frame/Build-GameBootstrap.ps1`;
  `investigations/I-0138-modern-post-chain/Run-I138HdrMenu.ps1`; `investigations/I-0132-rt-optimisation-zones/R12C-STATIC.md`
  (§8.5's page corrected); `.project/ROADMAP.md` (workstream 11's items 5 to 7).

## Tranche 5c: the game's own anti-aliasing filter removed (2026-11-14)

Thomas's request (the R thread, 11:42: « Ok, supprime ce fameux vieux filtre du jeu. L'option off doit être vraiment off »;
D-0278), read by the pilot at 19:10 only (its turn had not ended since the morning; said to him). Fable 5.2, the R thread,
between two steps of I-0141.

### Conclusion

Written and built; to review; not merged. Not witnessed live (no run shows a removed fallback but a TAA made to fail).

- **What the code did before** (read again before answering him; the pilot's morning sentence, « en SDR le vieux filtre
  prend le relais » when the player turns the TAA and DLSS off, was wrong): the page's Anti-aliasing on Off already ran no
  filter at all (I-0132 R12c, `12_scene_collect.inc`: neither the TAA nor a DLSS mode in the settings left the filter's input
  unset); the filter never ran on a frame the TAA or DLSS resolved (`scene_pass_frame.cpp`'s `aa_pass`), nor on an HDR10
  chain's float frame (an 8-bit pass, tranche 5; on an HDR10 chain it could run only where the tone map's HDR entry had been
  given up, the final review's L10). It ran in one case: the TAA or a DLSS mode chosen and not resolving the frame (the TAA's
  latch, DLSS not ready or latched off, a frame the TAA's own gate refuses: no velocity, a render extent that is not the
  presentation's), on an SDR chain, as their fallback (D-0265).
- **What is removed**: that fallback, and with it the whole native transcription of `GXAntiAliasingFilter`
  (AA-NATIVE-READY): the stage (`renderer/aa_stage.{h,cpp}`), its constants and their test (`content/aa_constants*`), its
  pinned programmes' lookup (FXAA.vpo, DLAA.ppo), its input in the collect and the prepare, its journal rows
  (`native_aa_frame`, `native_aa_inputs`, `native_aa_programmes_*`, `aa_filter_skipped_frames` of `native_hdr_frame`), the
  frame graph's `aa_copy` and `aa` passes. A frame neither DLSS nor the TAA resolves has no anti-aliasing. No row of the
  options is renamed (the TAA never ran with the filter).
- Kept: the blended Anti-Alias record's read for the YEBIS library's own flag (`144962190`, YB-4b), the game's Antialiasing
  option held On (the gate of the TAA and DLSS, D-0265), `no-aa` in `Run-StartupTrace`'s accepted flags (the earlier
  runners name it; the module no longer reads it). I-0126's `Run-R7aGi.ps1` keeps its modes as the record of its runs: its
  `dsaa` (`no-taa`, « DS3's own spatial filter » then) now gives no anti-aliasing, as its `noaa` (a line says so there).

### The step review

A fresh `step-reviewer` on the commit `29ff4e88f`: no High, no Medium; its five criteria held (nothing else changed: the
viewport and the target's state between the tone map and NIS or the glow, the plan key's other bits; no dangling reference;
the YEBIS flag's read intact; no neutralised code; the documents). Three Low, all wording, fixed: two comments of the build
script still naming the AA constants' test, the plan key's comment (bit 34 now unused), and the « one case » sentence
widened above and in D-0278.

### Reviews at the merge (the tree with `main` merged in, `51f5ac4d3`; with I-0141's tranche 1)

The final review: no High, no Medium; this tranche's criteria held (nothing else changes: the viewport, the heap and
root-signature state after the tone map are those every TAA or DLSS frame already used; no dangling reference; the YEBIS
flag's read intact; the removed test stated). Its Low for this tranche: **L8** an older transcription of the same filter's
constants stays in `world/light_producers` (`ProduceSized`'s CBDlaaParam, without a caller, and CBFxaaParam under
`ds3light::Produce`, which only the `ao_constants_test` oracle calls): left, they are the snapshot oracle's table (I-0102
kept them as named), not the removed pass; a cleanup for workstream 16. **L9** `no-aa` still accepted by the runners: said
above. **L10** « on an HDR10 chain, where the filter never ran »: corrected above and in D-0278's paraphrase. The
integrity review: the removed test's statement sound; its Low fixed (the counts' wording; `Run-R7aGi`'s `dsaa` mode).

### The merge (2026-11-14)

`ccde3618a` into `main` (with I-0141's tranche 1); the launcher's module rebuilt (exit 0, 179 PASS lines, its executables 79);
the start check `start-ccde3618a` passed on the OLED (20:48, the save restored): the merged tree's first live run.

### Tests (D-0268)

One test programme is removed with the code it tested: `aa_constants_test` (the filter's two constant buffers against the
original's values): the build's executables go from 80 to 79, its `*_test` programmes from 78 to 77, for that reason alone
(its PASS lines, 179, unchanged: that programme printed none). No other test, tolerance or reference changes. Build: `evidence/I-0138/artifacts/build-t5c.log`
(exit 0, WARP and the card; built while a capture run of I-0141 used the PC, no measure).

## Tranche 5d: the HDR output's two sub-rows hidden while it is off, their separator a quarter shorter (2026-11-14)

Thomas's request (the R thread, 20:00): « Comme pour l'option DLSS, il faudrait cacher les sous menus du HDR quand celui ci est
eteint, également réduire le séparateur blanc de 1/4, toujours comme le sous menu DLSS mode ». Fable 5.2, the R thread.

### Conclusion

Built and witnessed three times; two faults found by the reviews and seen in the first witness are fixed (the tab's two
buttons after a switch off; the Brightness page's slider, tranche 5b's, in `main` since that tranche) and seen fixed in
the second (« The second witness »); the tab opened with the output already on seen in the third. Not observed: the
Defaults press, another film, the DLSS mode's sub-row after this tranche's refactor (« Reviews at the merge (the branch at
`6b06c5740`) »). Merged `c9cc6bcf8` (« The merge »).

- **Hidden while the output is off.** The Graphics tab builds the peak's and the paper white's slider rows only while the HDR
  output is on (`BuildGraphicsTabRows`, `02b_rekindled_graphics.inc`); tranche 5b's greyed locked rows that showed their value
  are removed, with their two text ids and the text reader's branch. A switch of the output builds the tab's rows again at
  the next update, as before: seven rows with it off (Screen Mode, Resolution, V-sync, the frame rate, HDR output, Advanced
  Settings, Defaults), nine with it on.
- **The sixth slot's kind.** With the output off the Advanced Settings button takes the tab's sixth slot (`Item_5_0`), which
  the film makes of the generic kind 92: its Button widget is the centred one of a Defaults row, 15 px lower and without the
  sub-page's mark (read in the film: the sprite 54 at y 15 against the kind 131's sprite 129 with its `Mark`). So the film
  patch of tranche 5b also gives that slot the kind 131 (`options_film::RetypeSlot`, one tag of the sprite `PCGraphic`, two
  bytes): the button is drawn as in its own slot at either place, a slider has the same widget (sprite 90) in both kinds,
  and the Defaults row after it falls in `Item_6_0` (generic). The patched film's pinned SHA-256 changes (EB14204A…AB40; the
  shipped film's pin is unchanged): the new value was derived offline by the pilot, from the patched film of tranche 5b
  (the shipped film's bytes with its three added tags) with the character of the one `Item_5_0` tag inside the sprite
  `PCGraphic` set to 131, then seen in the game (`native_options_film`: « patched »; the integrity review recomputed both
  pins from the shipped film). Another film still leaves the tab its five rows. The retyped slot is the same film under the
  same pins as D-0279's exception to D-0161 (whose words name the three added slots only).
- **The runner.** `Run-I138HdrMenu.ps1`'s `tab` scenario gains four key steps after the switch off (Down twice, Up twice: the
  cursor on Advanced Settings and on Defaults with the output off) and its header says what the tab shows now; steps are
  only added.
- **The separator.** A slot's Slider widget holds the same Caption clip as its ComboBox (read in the film: the sprite 63 at
  x -456 px, its line the shape 60, 206 px each side, its `Text_0` at x -298 px where the Advanced page's film has -198):
  `PlaceRowCaption` (R12f's code for the DLSS mode's row, taken out of `AddAdvancedRow`) gives the two slider rows the
  sub-row's layout, the line three quarters long from its right end and the label where it was; each film keeps its own
  pair of unscaled places, read in a slot before it is changed. Their labels already carried the « └ » of a sub-row.

### Tests (D-0268)

`options_film_test` T4 (new): `RetypeSlot` on the synthetic movie changes one byte, the slot's character, and refuses a
slot the sprite lacks, a slot it holds twice, and what is not a sprite; the Graphics tab's sixth slot as the film writes it.
`graphics_rows_test`: the sub-row's layout on the options film's places (new). **One assertion changed, with its reason**: the
check that the two ids of the greyed sliders' values were ours and had no fixed text is removed with those ids (the rows no
longer exist); the Brightness row's word keeps its check. Builds: `evidence/I-0138/artifacts/build-t5d.log` (exit 0, 179
PASS lines), `build-t5d2.log` (the step review's fix: built from the working tree that was then committed as `fe5e816cb`,
exit 0, 179 PASS lines), `build-t5d3.log` (the final review's fixes), `build-t5d4.log` (the fixes' step review's Low: the
build the second witness ran on).

### The step review and the first witness

A fresh `step-reviewer` on the code's commit (`4e01dbeaf`): the row bookkeeping across a switch in either direction,
`PlaceRowCaption`, `RetypeSlot` and its refusal, the tests and the runner's steps held. **One High**: after the output is
switched from on to off, the Advanced Settings button and the Defaults row are made in the two slots that held the sliders,
and a row maker hides a slot's four other widgets without showing its own (R12C-STATIC §8.4): the two buttons stay hidden.
Two Low: comments still speaking of the greyed rows; `AddAdvancedRow`'s one pair of places serving two films (harmless:
read again at each page's first row; the comment corrected).

The first witness (`i138-t5d1-tab`, 21:13 to 21:20, the OLED in a 2560x1440 window, `build-t5d`, exit 0, no fault, the save
restored; `evidence/I-0138/artifacts/tranche-5d/runs-t5d1.log`, `t5d1-steps-5-7-32-33.png`). OBSERVED:

- The film patched (`native_options_film`: 9 slots, 32,991 bytes); the tab opened with 6 rows and Defaults (the output
  off), rebuilt with 8 at the switch on, with 6 at the switch off (`native_rekindled_graphics` rows).
- The output off at the tab's opening: the Advanced Settings button under the HDR output's row, with its mark, drawn as in
  its own slot; Defaults under it.
- The output on: the peak's and the paper white's sliders, their labels' « └ », their separators shorter than the rows'
  above (by eye on the picture; the clip's values read once: Caption at -456, its Text_0 at -300, both at 100 % before the
  change: `slider_caption`).
- **The High confirmed**: after the switch off, the cursor goes to the two rows (the help line says « Set advanced graphics
  quality », then « Restore default settings ») and neither button is drawn.

The fix: `ShowSlotWidget` shows the Button widget of the slot after the Advanced Settings row and after the tab's Defaults
row are made (the slot taken before the row, as the sliders' and the choice rows' own widgets are shown).

### Reviews at the merge (the branch at `fe5e816cb`)

**The final review** (a fresh `final-reviewer`): not ready as it stood. **One High, tranche 5b's and in `main`**: the
Brightness page's slider is not drawn once the HDR output has been on and is off again (the locked row's maker hid the
slot's Slider, and the game's slider maker does not show its own): no row on the page, the slider still answering unseen
(`i138-t5b2-tab/options-step-32.png`, `i138-t5d1-tab/options-step-36.png`; tranche 5b's RESULT said « again with its
slider », corrected there). Fixed: the slot's Slider shown after the game's builder. And the fix of the tab's two buttons
had no witness: the second witness below. Low: **L1** the tab's Defaults row on a first build is the game's own, without the
show (safe only on new clips): its Button is shown beforehand; **L2** the fallback place of the options film's label, the
film's -298 where the game holds -300: now -300, both named; **L3**, **L5** this section's stale sentences; **L4** the
retyped slot against D-0279's words: said above. Its criteria on the row bookkeeping, `RetypeSlot`, the removed rows and
the tests held.

**The fixes' step review** (a fresh `step-reviewer` on `fe5e816cb..79b297aac`, with I-0141's observers): no High, no
Medium; the two shows and the constant held. One Low for this tranche, fixed: the Brightness slider is shown only when the
game's builder made its one row. Built again after it (`build-t5d4.log`).

**The integrity review** (a fresh `integrity-reviewer`): no High, no Medium; the dropped assertion and the changed pin
stated with their reasons (both pins recomputed from the shipped film), the witness's numbers and stills as the RESULT
says. Two Low, fixed above: how the new pin was derived; the runner's change named.

### The second witness

`i138-t5d2-tab` (22:40 to 22:47, the OLED in a 2560x1440 window, `build-t5d4`, the branch at `514f5c70c`; exit 0, no
fault, the save restored; `evidence/I-0138/artifacts/tranche-5d/runs-t5d2.log`, the six steps on one sheet
`t5d2-steps-5-7-31-32-33-36.png`). The same scenario as the first, read on the stills by the pilot. OBSERVED:

- The journal as the first witness's: the film patched (9 slots), the tab opened with 6 rows, rebuilt with 8 at the
  switch on, the peak and the paper white moved and brought back, the chain HDR10 then SDR again
  (`native_rekindled_graphics`, `native_hdr_output`).
- The output off at the opening (step 5) and on (step 7): as the first witness (seven rows drawn with Defaults; then the
  two slider rows with their « └ » and their shorter separators).
- **The switch off (step 31)**: the two slider rows gone, the Advanced Settings button with its mark and the Defaults row
  drawn under the HDR output's row; the cursor on Advanced Settings (step 32) and on Defaults (step 33) shows each one
  highlighted. The first High is fixed.
- **The Brightness page after the switch off (step 36)**: its picture, « Adjust Brightness » with its slider at 5 and
  Defaults drawn. The final review's High is fixed. The cursor is on Defaults there, where the locked page left it (step
  18); the slider's own answer to the keys was not tried in this run.

### Reviews at the merge (the branch at `6b06c5740`)

A second pass, on the final tree with tranche 5e and I-0141's tranche 2 (a fresh `final-reviewer` and a fresh
`integrity-reviewer`). Their one High is I-0141's record (its RESULT « Tranche 2 »). For this tranche, the final review:
the code equivalent by reading and safe to land; **M1** four cases its exit criteria name had no witness and were not
listed as such: the tab first built with the output already on (the normal path of an HDR player: both witnesses opened
it off), the Advanced page and its DLSS sub-row after `PlaceRowCaption` left `AddAdvancedRow`, the Defaults press, another
(unpatched) film; **L3** the Advanced Settings button and the rebuild's Defaults row shown without the check that their
row was made: the same check as the Brightness row's now. Answered: the first by a third witness, the second in part
(« The third witness », below); the two others stay NOT OBSERVED, by reading only: the Defaults press (it also resets the screen mode and the resolution, which a
run on Thomas's screens does not try), and another film (a film whose hash is not the shipped one's is left as it is, the
tab's HDR rows are then not built and it keeps its five rows: the code's path, `16b_rekindled_overlay.inc` and
`tab_hdr_rows`, never run). The integrity review: this tranche's tests, pins and
second witness as stated.

**The step review of the fixes** (a fresh `step-reviewer` on `6b06c5740..4329951b5`): the two Button shows' check as the
Brightness row's; the runner's new scenario touching no other. Its Lows are I-0141's (its RESULT « Tranche 2 ») but one,
this tranche's 5e: the exposure rows' range (-0.4575), corrected.

### The third witness

`i138-t5d3-tabon` (2026-11-15 00:05 to 00:10, the OLED in a 2560x1440 window, `build-t5e3`: the branch at `4329951b5`;
exit 0, no fault, the save restored; `evidence/I-0138/artifacts/tranche-5d/runs-t5d3.log`, the sheet
`t5d3-steps-14-21-22-23-24-28.png`; the runner's `tabon` scenario). OBSERVED:

- **The tab first built with the output on** (step 14, after the menus were closed whole and opened again; the journal:
  `"built":"open"`, 8 rows, 2 sliders, `hdr` true, where both earlier witnesses opened it off): the nine rows drawn on the
  new page, the two sliders with their « └ » and their shorter separators, Advanced Settings with its mark, Defaults.
- **The Advanced page from it** (steps 21 to 23): the cursor on Advanced Settings, the page's thirteen rows drawn with
  their captions and the Sharpening slider (`"page":"advanced"`, 13 rows; the caption's places read once: -456 and -200),
  the cursor moving in it. **Not observed**: the DLSS mode's sub-row (Anti-Aliasing was TAA in this run, so the row is not
  built), and the way back to the tab: Esc closes the menus whole from a page (as it does from the tab, steps 9 and 24), so
  the scenario's last steps met no menu and the output stayed on in the run's own settings (set aside and restored by
  the runner; the player's file is not the run's).
- No fault in the journal (`native_render_job_failure`: none; the controller's exit 0).

**The merge** (2026-11-15): `c9cc6bcf8` into `main` at 00:12 (the branch at `46f8a678a`: I-0138's tranches 5d and 5e with
I-0141's tranche 2, first observation; `Test-Project -MergeGate` passed before, `Test-Project` after); the launcher's
module rebuilt in the main checkout (`evidence/I-0138/artifacts/build-launcher-c9cc6bcf8.log`: exit 0, 179 PASS lines);
the start check `start-c9cc6bcf8` passed on the OLED in a 2560x1440 window (00:30: at the world in 3.28 s, closed, the save
restored). The disk rotation was refused at the checkpoint (another thread's game was running; C: 250 GB free and more).

## Tranche 5e: the automatic exposure never darkens under the zone's own exposure (2026-11-14)

Thomas's decision (the R thread, 22:16: « Oui pour l'auto expo corrigée »; D-0280, amending D-0264), on what I-0141's runs
showed (its RESULT « 32000010 at paired instants »): the automatic exposure, at half strength toward one target for the
whole game, darkened the zones the game makes bright on purpose by one stop to a stop and three quarters (the King of the
Storm's arena as at dusk, Archdragon Peak in play at 60 % of the original's level).

### Conclusion

Built, reviewed, witnessed and merged (`c9cc6bcf8`, with tranche 5d: its « The merge »).

- The factor's lower bound is 1 (`kAutoExposureMinLog2` 0, `post_params.h`), given to the stage by the prepare
  (`13b_prepare_model_frame.inc`): the kernel's bisection then starts at the zone's own exposure, so a view brighter than the
  target solves to the factor 1 and a darker one is brightened as before (the half strength unchanged); the adapted factor is
  clamped to the same bound. The stage's own default bounds (-8 to 10) are its unit tests' and unchanged.
- No change of the target nor of the strength. The test tools' code is unchanged: `no-auto-exposure` as before;
  `rt-auto-exposure-full` is the full strength under the same bound now (it no longer darkens either).

### Tests (D-0268)

`auto_exposure_test` (new checks, none changed): with the bound a bright view's solved factor is 1 where it was under a half
without it (a bin that solves inside the free range, about -3.9 stops), and a dark view's is the same with and without it.
`auto_exposure_stage_test` (a new pass at its end, none changed): the kernels on WARP with the game's bound keep the factor
1 on its scene, whose free factor is -3.7 stops (the goal and the factor 0 in log2, the factor texel 1, DLSS's exposure the
area's). The kernels are run with the bound on that bright scene only; a dark view under the bound is the CPU mirror's
check alone.

Builds: `evidence/I-0138/artifacts/build-t5e.log` (the tranche's commit `a2ffd0942`: exit 0, 179 PASS lines; the build the
witness ran on), `build-t5e2.log` (the step review's fixes).

### The witness

I-0141's run `i141-t2b-c32000010-nat` (22:49 to 22:56, the OLED in a 2560x1440 window, `build-t5e`; its RESULT
« Tranche 2 »): Firelink, the travel to Archdragon Peak, the zone's ceremony scene, the King of the Storm's scene and the
play after it. OBSERVED: the factor stays 1 through the whole run (`native_auto_exposure`: 90 rows, `log2_factor` 0.0000
to 0.0028, none negative). Like for like at Firelink, the same save and place: the factor 2^-0.45 before (`i141-t1c-
c32000010-nat`: `log2_factor` -0.4495 to -0.4575, mean 0.233) and 1 now (0.0000, mean 0.271, over the target 0.20). The
King's scene is at the original's level in this run (mean luma 78 to 92 of 255 against the original's 83 to 92 in its
first cuts, 128 to 133 against 117 to 137 on the player), but in another state of the zone than tranche 1's runs (the
cloud sea, I-0141 « Tranche 2 »): no run without the bound exists in that state, and the journal's goal is written after
the bound, so this agreement is not shown to come from the bound (the merge's integrity review, its Medium). D-0280's
ground stays tranche 1's same-state shots (176.9 / 96.3 / 172.2). A dark view brightened as before: not witnessed in
this run (the unit tests' case).

**Reviews at the merge** (with tranche 5d's second pass, above): the final review found the tranche's criteria met
(one constant on the one prepare path; the kernel's clamp and its consumers); the integrity review's Medium and the
final review's L5 are this section's wording (the King's scene and the zone's state; the kernels run with the bound on
a bright scene only), corrected above.

### The step review

A fresh `step-reviewer` on `514f5c70c..26e36c247` (this tranche and `Run-AlphaCast -SetFlagAfterTravel`): no High, no
Medium; the bound reaches the kernel on the game's one path, the consumers of a factor pinned at 1 behave, no assertion
weakened. Four Low, fixed: **L1** the full-strength tool is bounded too, which a comment and this section did not say;
**L2** the bright check's bin solved at the old bound (-8): a bin inside the range now; **L3** no test ran the kernels with
the bound: the stage test's new pass; **L4** the runner's new parameter ignored without `-TravelPress`: it says so.

## Tranche 7: the settings rows (2026-11-15 and 2026-11-16, Opus, branch `claude/r-i138-t7`)

The sheet is [TRANCHE-7-READY](TRANCHE-7-READY.md); D-0276, D-0285; Thomas's sliders for the numeric rows (2026-11-14 12:05).

### What is written

- **Step 1, the settings and the rows** (`a824b4c66`): five keys in `rekindled-settings.ini` (`look=original|rekindled`, the original
  the default; `aberration=on|off`, on; `bloom=0..100`, 100; `shafts=on|off`, on; `grain=0..100`, 0), parsed and written with the
  others (`rekindled_settings.h`); five rows on the Advanced page between Reflex and the vignette, which stays last
  (`graphics_rows.h`): Colorimetry (Dark Souls III, Rekindled), its sub-row Chromatic aberration shown while Rekindled is chosen,
  Bloom (%) and Film grain (%) as the game's sliders by tens, Light shafts On or Off; their labels and help lines in English and
  French; the Colorimetry row rebuilds the page (the sub-row appears or goes).
- **Step 2, the readers** (`bd76f2b3a`, `733cdf4cd`, `73d5ef939`): the look (`NativeToneLookRequested`): without a test flag the
  settings' colorimetry, Rekindled being the candidate after Elden Ring (tranche 6's `ps_er`) with its midtone lift of our own
  after ER's LUTs (`look::kErLift`, a power 0.8 on the curve's output after its vignette: ER's grey node 136 in, about 154 out,
  TRANCHE-7-READY §0.4) and its aberration the sub-row's; the test flags keep their candidates. The bloom: the glare's weight in the
  tone pass scaled by the row (the light shafts with it: they ride the glare's picture). The light shafts: the row off draws none
  (as `no-yebis-shafts.flag`). The tone block grew to 224 bytes (`er_lift`); the automatic exposure's kernels read it at its size.
- **Step 3, the film grain** (the R thread of 2026-11-16): a new effect, none by default (PLAN §5). Added to the display-encoded picture by the
  tone pass's last draw, before the UI: the LUT's draw where one follows the tone map (flag bit 6, `kPostToneGrainLater`, set by
  the stage), else the tone map's own draw (before the 4x4 dither in the 8-bit entries, after the highlights' gain in the HDR
  entries). Monochrome: one value a pixel on the three channels; a triangular noise in (-1, 1) at the corners of a grid of
  max(1, height / 1080) pixels (the grain the same share of the picture at every resolution, D-0151), smoothly interpolated
  between them, its corners a hash (pcg3d) of their place and the frame's number (it moves every frame); times the strength (the
  row's percent of `kFilmGrainMost`, 0.08 of the display-encoded value) and a weight on the luma, 1 at mid-grey and 0.25 at black
  and white. The row at 0 leaves every entry's bytes as before (the shader's selection is exact). The block grew to 240 bytes
  (`grain`, `grain_seed`; `world/post_tone.h` `SetFilmGrain`), the automatic exposure's kernels again. A look whose pipeline
  failed falls back to the record's block with the grain kept.
- **Step 4, the field of view** (the same thread; TRANCHE-7-READY §4, read by a `ghidra-research` subagent, GhidraMCP and the mirror,
  `evidence/I-0138/artifacts/tranche-7/fov/`): READ, the follow camera's update 140515790 lerps its vertical field of view +0x50
  toward its param row's (LockCamParam, the row from the camera's id fields; +0x14 in degrees, clamped to [0, 180], times pi/180
  at 140515B7A) by the weight +0x3D0 (140515B57..140515B90); 43 degrees in 110 of LockCamParam's 141 rows (`lockcam.tsv`). The aim
  and zoom cameras keep their own +0x50 (14050E9C0, 14050D0C0) and reach the active camera only through ChrCam's copy (1405103E0).
  The row: `fov=80..130` (percent, 100 the game's), a slider by tens on the Advanced page after the film grain. The reader scales the
  target alone (the first form, +0x50 divided before the call and multiplied after it, let the call's own reads of +0x50, the
  distance pull-back and the lock-on's pitch bound, see DS3's angle: the step review's M2): `camera_laws::Install` (at every native
  start, before the hooks are enabled) moves the two instructions at 140515B76 (movss xmm1,[rax]: the clamped degrees; mulss
  xmm1,xmm4: times pi/180) into a stub near the module that adds mulss xmm1,[scale] and minss xmm1,[150 degrees] and jumps back to
  140515B7E; the site becomes a jump and three nops (its bytes checked first, nothing patched and `camera_fov_refused` journalled
  when they differ). `camera_laws::Update` writes the scale before each call. Every reader of +0x50 then sees the camera's true angle,
  easing toward the scaled target (ChrCam's copy, the render's, the culler's: Step 4F and StageRend_PerFrameUpdate both take the
  holder's camera). Left alone: the aim and zoom cameras, a cutscene with its own lens. READ, not witnessed: a cutscene's blend from
  the game camera (14062DDF0: (game - cut) w + cut) and a cutscene with no lens of its own (140CE25C0 returns the game camera's) take
  the scaled value; `camera-fixed-step.flag` (a test reference) runs without `camera_laws`, so without the patch. INFERRED: the HUD
  markers' and the lock-on's projections (14062D4F0's callers) read a camera with the same field of view. The stub has no unwind
  data (a stack walk stopped in its four instructions sees a leaf), as MinHook's trampolines.

### Tests

- `post_tone_test`: the block's layout in every programme (the grain's two words at their offsets), `SetFilmGrain`'s words (none
  by default, 0 adds none, 100 % its most, 50 % at 2160 lines half of it on a grid of two pixels, over 100 % kept at 100 %, an
  unknown height a grid of one pixel). `post_tone_stage_test` (WARP and the card), section 5: the grain at 100 % on the capture's
  scene against the same draw without it, in the 8-bit entry, with the synthetic LUT and in the HDR entry: monochrome where no
  channel is within the grain's reach of a clamp (within one level on WARP, two on the card: 2.00 is the card's observed maximum
  in its twelve 8-bit cases, the bound set to it after the first card run read 2; the reason, the card's float to UNORM conversion,
  INFERRED; on the card a LUT bend of one level would pass, WARP keeps one; 1.5 in HDR, 0.19 read), its mean move under half a level, at most its strength's bound, more
  than half the pixels moved, the alpha untouched, a second seed another grain. With the LUT, the monochrome check is the proof
  that the grain comes after the LUT (the synthetic LUT bends each channel by the two others). Section 4's model of the candidate
  after Elden Ring now takes the lift (`look::ErLift`; the model's power `kErLift`, the shader's the block's word, its tolerance
  unchanged); `post_tone_test` checks the lift's words (1 for the other looks, kept with the aberration off, ER's grey node 8 at
  154.2 in (150, 158)); `graphics_rows_test` the order (the vignette last, the post rows after Reflex) and every new row;
  `rekindled_settings_test` the six keys, their refusals and the 21 keys written. The runner `Run-I138HdrMenu.ps1` gains the runs
  `advanced` (the Advanced page's rows) and `hdrshot` (the Graphics tab with the HDR output on, for Thomas).
- `rekindled_settings_test`: the field of view's key (100 by default, 80 and 130 accepted, 79, 131 and an empty value refused,
  twenty-one keys written and read back); `graphics_rows_test`: its row (the second to last, before the vignette; 100 % by default, 70
  and 140 refused, 80 and 130 written; a file's 125 shown as 120; every slider step a value the row offers).
- Builds: `evidence/I-0138/artifacts/build-t7b.log` to `build-t7b4.log` (a literal newline in a test string; the grain's monochrome
  check too near the clamps; the settings' key count; the card's float to UNORM conversion: two levels on the card, one on WARP);
  `build-t7b5.log`, `build-t7b6.log` (the first step review's fixes), `build-t7c.log` (D-0293, D-0294) and `build-t7d.log` (the final
  tree with `main` merged in): exit 0, 178 PASS lines each, WARP and the card. The unit tests do not run the camera's patch.

### Not witnessed

Under D-0227 nothing of this tranche ran in the game before its merge: the Advanced page's new rows and their scroll (18 to 21 rows),
the four sliders (the field of view's least value 80, read safe in the game's slider: it keeps the row's byte in an int), the Rekindled look with its lift, the light shafts' row, the grain on screen and the camera's patch are READ and unit-tested only;
the start check after the merge is their first run, the sheets (`tranche-7/run-t7.ps1`) their witness. Of TRANCHE-7-READY §0.4's
proposal only the power 0.8 lift was made; the black's blue raised by about 0.04 (ER's cool black) was not (D-0285's « midtone and
shadow lift » met by the curve). The Rekindled look without its lift is now drawn by no flag: the sheets compare it with tranche 6's
`tranche-6-looks/sheets-3/` (the candidate after Elden Ring without it).

### Step reviews

- **The tranche's code** (a fresh `step-reviewer` on `1e39624be..84d00d428`): no High. **M1** the bloom row lost when a look's pipeline
  fails (the fallback to the record's block did not keep `glare_scale`): fixed (`88117392f`). **M2** the field of view's first form
  divided +0x50 before the camera's call and multiplied it after, so the call's own reads (the distance pull-back, the lock-on's
  pitch bound) saw DS3's angle: the target alone is now scaled, in a stub at 140515B76 (Step 4 above; `88117392f`). Low: **L1** the
  passes after the tone map (NIS, the glow, frame generation) touch the grain: said in the shader's head; **L2** the grain's grid not
  under one pixel below 1080 lines: left (a grain cannot be finer than a pixel), said there; **L3** the new keys missing from the
  journal's settings row: added; **L4** the settings read four or five times a prepared frame: left (a lock and a copy, the values
  are each read where used); **L5** a function-local static slider range: at namespace scope; **L6** the light shafts ride the
  glare, so the bloom at 0 removes them: said on the shafts' help line, the glare's cost at 0 left; **L7** the field of view does
  nothing where `camera_laws` is not installed (test configurations): the patch's refusal is journalled, the configurations left;
  **L8** a new camera object eases from its own start angle to the scaled target: as the original eases to its target (left);
  **L9** the grain checked in three paths of the stage test (the candidate looks, the HDR entry with the LUT and the fallback
  share `Grain()`): left; **L10** stale comments: corrected.
- **The fixes and D-0293, D-0294** (a fresh `step-reviewer` on `84d00d428..c1d9a0f5f`, the site's bytes read in the executable): no
  High, no Medium. Low: **L1** the stub has no unwind data (a stack walk stopped in its four instructions): said in the comment, as
  MinHook's trampolines; **L2** pages kept on a refusal: released; **L3** comments still saying x2: corrected, `rt-local-x1.flag` a
  no-op kept in the runner's pattern; **L4** IN-FORCE's D-0262 and D-0263 lines: annotated; **L5** (before this tranche) a failed
  Rekindled pipeline falls back to the record's block without the area's LUT (the grading's choice is made at prepare): left, a
  fallback that has never been met.

### Reviews at the merge

- **The final review** (a fresh `final-reviewer` at xhigh on `main...8613df55a`, briefed under D-0283; it read the site's bytes in the
  executable and recomputed the A/B's and the exposure's numbers): no High. **M1** the tranche never ran in the game while the records
  called it done: « Not witnessed » above, ROADMAP and STATE say « not yet seen in play ». **M2** the sheets' runner pointed at a stale
  build and this worktree: `run-t7.ps1` now runs `build-t7d` (the final tree) from the main checkout; the look without its lift is
  compared with tranche 6's sheets. **M3** Step 4 still described the first form: rewritten. Low: **L1** stale records (the next steps,
  the builds): corrected; **L2** stale comments (`auto_exposure.cpp`, the look's reader, the settings' bounds): corrected; **L3** a
  slider value off the tens would be refused silently: left (the game's slider steps by the range's step, as the HDR tab's; not seen
  off it); **L4** the field of view's row offered where the patch is refused or `camera_laws` absent (test configurations): left, the
  refusal journalled; **L5** the settings file written at every slider step: left (the Advanced page's rows were so before); **L6**
  the patch's page search beside `rip_float`'s: left (two more 64 KB regions); **L7** a choice list built and thrown away a slider
  row: left (as before the tranche); **L8** the HDR highlights' rule with the Rekindled lift: said in the shader's head; **L9** the
  black's blue lift dropped: said in « Not witnessed »; **L10** the stub's unwind data: recorded; **L11** test gaps (the settings to
  the look, the grain in the looks and the fallback): left.
- **The integrity review** (a fresh `integrity-reviewer` on `main...38eed9e8c` with this record and I-0141's « Tranche 10 »): nothing
  cut from a measured run, no scene-specific code, every number recomputed matches. **H1** (by its rule) test and runner changes
  not listed: listed under « Tests ». **M1** the grain's monochrome bound on the card set to its observed maximum: said there. Low:
  45000030's early cuts by the whole picture as well as the centre, the smoke's windows, the two measures behind the 0.12 and the
  tour's 55 runs: corrected in I-0141's « Tranche 10 ».

**The merge** (2026-11-16): `30bf1a6c3` into `main` with I-0141's tranche 10 (`Test-Project -MergeGate` passed before); the launcher's
module rebuilt in the main checkout (`evidence/I-0141/artifacts/build-launcher-30bf1a6c3.log`: exit 0, 178 PASS lines); the start check
`start-30bf1a6c3` passed on the ASUS (08:06, restored), the tranche's first run in the game.

### The witness (the sheets, after the merge)

`i138-t7-<variant>-z34-irithyll-of-the-boreal-v-nat` (08:10 to 09:01 on 2026-11-16, the ASUS, the tour's 1280x720 window, offline,
no measure; `build-t7d`, the merged tree; `tranche-7/run-t7.ps1`, `runs-t7.log`; twelve runs, every one exit 0, no fault, restored):
the default, Rekindled with and without its aberration, the bloom at 0, the light shafts off, the grain at 50 and 100, Rekindled with
the grain at 50, the field of view at 80 and 130, then the Advanced page in English and French (`i138-t7-en-advanced`,
`i138-t7-fr-advanced`). OBSERVED: each run's `native_settings` row holds its file's values; no `camera_fov_refused` row (the patch
applied); the field of view wider at 130 and narrower at 80 on the same view; the grain visible at 100 % pixel for pixel; the new
rows at the page's end in both languages, the bloom, grain and field-of-view sliders drawn, their help lines shown. Not shown: the
aberration's sub-row appearing (the menu run's Up steps landed on the bloom: the list wraps); the look without its lift. The sheets
(`tranche-7/sheets/`: `t7-irithyll-variants.jpg`, `t7-grain-1to1-default-vs-100.jpg`, `menu-en-34-37.jpg`, `menu-fr37-en45.jpg`) are
with Thomas for his eye (the look, the grain's strength, the bloom at 0).
The disk rotation after the merge (`Clear-DiskCache -Apply`, 09:10): 26.9 GB deleted (24 build folders older than 48 h, 2,515 raw
frames older than 7 days), 219 GB free on C:.

## Tranche 7b: the Rekindled look's vivid variant (2026-11-16)

D-0296. Thomas on tranche 7's sheets (the project chat, 09:55): « Je vois pas beaucoup de différence entre default et rekindled »; the
coordinator relayed his wish to go on with a marked variant behind a flag, judged on sheets side by side. A Fable 5.2 subagent
(read-only; Thomas allows Fable for the colorimetry) measured ER's 76 LUTs (`C:/Users/User/Tools/DS3-Decompilation/er-post/luts/er`;
its report and scripts `tranche-7/vivid-er-luts/`, rerun by the integrity review with the same figures): the subagent's measures, ER has no saturation operator (ColorS 1 in 278 of 279 entries) and its LUTs' chroma off their grey curves is x1.00
(p10 to p90 0.96 to 1.03): they are per-channel tone curves (the grey median 68 -> 68, 136 -> 146, 204 -> 213) with cool shadows (the
red lowered as much as the blue raised, gone by node 7 to 8). Tranche 7's lift (y^0.8) takes 68 to 89: it flattens the shadows and
cuts the channels' ratios by about 20 %, the likely reason the look reads close to the original (INFERRED).

- **The variant** (first behind `tone-look-er-vivid.flag`; since Thomas's choice, below, the Rekindled look itself; `look::ErVivid`, `ErVivid` in `post_tone.hlsl`, the words `er_lift.yzw`, zero
  by default, set by `SetErVivid` whenever the candidate after Elden Ring is drawn): on the encoded value after
  the curve, the lift and the vignette, before the HDR gain, the grain and the dither, a per-channel contrast e (1 - 0.8 (1 - e)^3)
  (the lifted grey curve back to ER's median: 0, 13, 30, 49, 69, 89, 109 ... against 0, 15, 32, 49, 68, 87, 105 ...); a vibrance of 0.25
  keeping the largest channel, halved for skin hues (of taste, not ER's); a cool black, black (0, 0, 10) of 255, after ER's LUTs.
- **Tests**: `post_tone_test` the model against the subagent's table of 17 values (within 2e-4), each channel monotonic and never
  negative over 20,000 values, the channels' order kept where the tint does not act, the words; `post_tone_stage_test` the 8-bit and
  HDR10 entries against the model on both scenes, the HDR values finite and some above the knee, more than half the texels moved.
  `build-t7v3.log`: exit 0, 178 PASS lines (the 8-bit entry within 0.98/255 of the model on WARP, 1.043/255 on the card); after the
  step review `build-t7v5.log`: exit 0, 178 PASS lines (the HDR entry within 0.5 % of the model's value plus at most 2.0e-5, inside
  its 5e-4, on 1.84 million texels, 0.70 million above the knee). The order check first read « where the largest is at least 0.5 »
  and failed once (`build-t7v2.log`: an input of largest 0.5196, the cool black still acting up to about 0.542): moved to 0.6, past
  its reach. The runner's stale-flag list gains `tone-look-er-vivid.flag`. The always-on form (`9778bd73e`, no flag) built and tested
  in `build-t7v6.log` (the final tree with `main` merged in: exit 0, 178 PASS lines); not yet run live (the sheets ran the flag's build).
- **The step review** (a fresh `step-reviewer` on `6b512216b..26d17c322`): no High. **M1** the HDR10 entry not drawn by the test: added.
  Low: **L1** nothing journalled the flag: a row; **L2** the skin weight's hues: said as the code has them; **L3** the table's
  tolerance: 2e-4; **L4** an HDR comment: corrected; **L5** the 8-bit bound of 1.6: its derivation said.
- **The sheets** (`i138-t7v-<original|rekindled|vivid>-<row>-nat`, 11:16 to 11:51 on 2026-11-16, the ASUS, the tour's window, offline,
  no measure; `build-t7v`, the flag's build; `tranche-7/run-t7v.ps1`, `runs-t7v.log`; nine runs exit 0, no fault, restored) at
  Irithyll, Cleansing Chapel and Church of Yorshka beside the tour's originals (`tranche-7/sheets/t7v-orig-original-rekindled-vivid.jpg`):
  OBSERVED by eye, the vivid variant's shadows deeper, its blacks cool, its dull colours a little more saturated, the skins kept;
  the current Rekindled near the original look in saturation (0.19 against 0.20 on the runs' presented frames, measured by the
  integrity review) but brighter (the median channel sum 164 against 118). **Thomas** (the R thread, 12:06): « Oui j'aime bien »: the vivid variant is the
  Rekindled look, its flag gone (`9778bd73e`); D-0296. Judged on SDR sheets: its HDR10 look (the cool black's 0.17 nit of blue at a
  200-nit paper white on the OLED's black) joins the OLED's open A/B.
- **The final review** (a fresh `final-reviewer` at xhigh on `main...e585af67c`): no High. **M1** STATE contradictory (a merge
  claimed before it, tranche 7's sheets still awaited): rewritten at the merge. **M2** D-0285 no longer the look drawn: D-0296. Low:
  **L1** the flag's journal row left, always true: removed; **L2** the vivid words set outside `ApplyToneLook`: set there, tested for
  the ER candidate and none for the others; **L3** no test of the vivid look with the dispersion (the default), non-finite scene
  texels skipped, the bound's derivation: the derivation said, the two gaps left (the shared `ErVivid` after the channels' reads);
  **L4** the figures by adapter and the final build: recorded; **L5** the ER measures without an artifact: the report saved;
  **L6** stale sentences (the next steps, ROADMAP, I-0141's, comments, the area LUTs' bullet): corrected; **L7** the smoke's
  conclusion and the fog-banks run's alignment: INFERRED, both alignments given; **L8** the HDR10 side not judged: said.
- **The integrity review** (a fresh `integrity-reviewer` on the same range): no High; **M1** the smoke's conclusion stated as a
  finding: INFERRED with the runs' spread; Low: the adapters' figures, the order check's move, the runner's list, the always-on
  build, the sheets' claims: recorded (`3ab96ce2c`).
- **The step review of the code's answers** (a fresh `step-reviewer` on `e585af67c..74257b41e`): no High, no Medium. **L1** with
  `vivid` false the words were left as the block held them: written to none (`44f04a750`), tested over a block that had them; **L2**
  the original's part of the check proves little: kept; **L3** a comment's figure by adapter: said; **L4** the runner's stale-flag
  list keeps the former flag's name (a removal only): kept. Builds: `build-t7v7.log`, `build-t7v8.log` (a comment that swallowed a test's line: failed) and
  `build-t7v9.log` (the final tree): exit 0, 178 PASS lines. The decision first numbered D-0295, renumbered D-0296 (the number taken by the build thread).
- **The merge** (2026-11-16): `bb7535fc1` into `main` (`Test-Project -MergeGate` passed before); the launcher's module rebuilt
  (`evidence/I-0138/artifacts/build-launcher-bb7535fc1.log`: exit 0, 178 PASS lines); the start check `start-bb7535fc1` passed on
  the ASUS (restored), the always-on form's first run.

## Tranche 8: YEBIS removed from the module (2026-11-16, Opus, branch `claude/r-i138-t8`)

PLAN §5's last tranche: « the YEBIS shaders, their constant blocks, their refusals and their tests removed ». Split in two by
what the journals show (below): this tranche takes every YEBIS programme, block and test path out of the module and no longer refuses the
bloom at the Twin Princes (a misread word); **8b**, the next, was to write the light shafts' corona that m38's records asked (found unreached in play: « Tranche 8b, 2026-11-21 »).

### What the module no longer holds (8a)

- **The library's programmes read from the executable**: `AcquireNativeTonemapPrograms`, `AcquireNativeGradingPrograms`,
  `AcquireNativeGlarePrograms` (the DXBC containers of `.rdata` found by their pins) are removed, with `yebis_pins.h`'s include
  in the module. Nothing the module draws comes from the library any more: the tone map, the grading, the glare family, the
  light shafts, `Alpha_R`, the depth of field and the motion blur all draw our programmes (tranches 1, 2 and 4).
- **The four test paths that drew them** (`post-tone-old.flag`, `post-tone-verify.flag`, `post-glare-old.flag`,
  `post-glare-verify.flag`): the frame's `old_path` and `verify`, the inputs' `programs` and `verify_programs`, the renderer's
  `TonemapStage tonemap_`, `GradingStage grading_`, the scratch target `tone_verify_*`, the second stages `glare_verify_`,
  `dof_verify_`, `motionblur_verify_`, the row `native_glare_verify` and `GlareVerifyFailed`. The flags are gone from
  `Run-StartupTrace.ps1`'s pattern (kept in its stale list, so a leftover file is removed from a build folder);
  `Run-I138HdrMenu.ps1`'s `copy` run, which used `post-tone-old.flag`, now takes `no-yebis-tonemap.flag` (no tone map, glare,
  grading or automatic exposure of ours, so the same 8-bit-to-float copy on an HDR10 chain; the final review's L5). [Compare-ToneVerify.py](Compare-ToneVerify.py)
  stays for the pairs already in the evidence store; no new pair can be made.
- **The `$Globals` block**: `NativeTonemapInput::globals` (3,584 bytes zeroed in every frame's input, read by the two test flags
  only since tranche 3, its final review's « recorded, left ») and the `static_assert` that tied it to `TonemapStage`.
- **GX's `LutCopy2DTo3DCs`**, the gxposteffect bundle's compute programme the grading's strip copy ran (one of the « three GX
  programmes around the YEBIS library », `yebis_stage.h`): replaced by our own, `cs_lut_copy` in
  [post_tone.hlsl](../../native/renderer/shaders/post_tone.hlsl), transcribed from the original's disassembly (`fxc /dumpbin` of
  the I-0063 programme cache's `gxposteffect__LutCopy2DTo3DCs.cpo`: `[4,16,1]` threads, the group's z the slice, four columns
  x, x + 4, x + 8, x + 12 a thread, `ld` then `store_uav_typed`, no arithmetic; ours compiles to the same loads and stores).
  `AcquireNativeYebisGxPrograms`, its call in the collect and `NativeYebisGxPrograms` are removed; `PostToneStage::OpenLut`
  takes no programme. The module now reads no YEBIS programme and none of the GX programmes around the library; the emissive
  glow's four GX programmes (`AcquireNativeGlowPrograms`, GX's, not YEBIS's) still come from the engine's repositories.
- **Rows kept as they were**: `native_yebis_tonemap`, `_glare`, `_dof`, `_motionblur` keep their fields, `"programmes"` now
  always `"ours"` and `"verify"` always `false`, so the readers of the evidence store read the new journals unchanged. Two journal
  changes: `native_yebis_glare`'s `"gamma"[1]` is now always 1 (it printed the globals word +0x100 before, below); the default
  path's `native_yebis_gx_programmes_resolved` and `_unresolved` rows are gone with the GX copy.

### What stays, and why

- **The offline tests keep the library as their oracle.** `TonemapStage` and `GradingStage` (the original's programmes from the
  player's executable, found by their pins at test time) leave the module's link but stay compiled into the tests:
  `post_tone_stage_test`'s section 2 compares our pass with them byte for byte on any adapter, `glare_stage_test`,
  `dof_stage_test` and `mb_stage_test` compare our programmes with the library's, and `tonemap_stage_test`,
  `grading_stage_test` and `yebis_pins_test` tie the oracle itself to the original's captured draws. PLAN §5 said « their tests
  removed »; I did not remove them (D-0268: removing them would weaken the only byte-for-byte proof that the original colorimetry
  is the original's, and the mandate keeps the original as a reference: « Separate reference runs, captures and comparisons are
  development tools »). They never ship: nothing of them is in `game_bootstrap.dll`.
- **One test added**: `post_tone_stage_test` now compares our copy's volume with GX's (`GradingStage`'s) on the synthetic strip,
  bit for bit, and with the strip itself word for word (3D[x][y][z] = 2D[x][16z + y], each byte k as k / 255); its existing check
  of the volume against the captured `t16` now runs through our copy.
- **Names kept**: the `ds3native::yebis` namespace and the `world/yebis_*` files (they model DS3's YEBIS parameter blocks, the
  game's data that our chain reads every frame through the feed of FB890), the flags `no-yebis*.flag` (test tools of our passes)
  and the rows `native_yebis_*`. TRANCHE-1-READY §2.1 planned to move the tone-map helpers out of the namespace here: not done, a
  rename of about 70 files and of the links of the investigations for no change of behaviour; left to workstream 16's cleanup if
  wanted. The executable's own data words the passes read (the dither texels, the curve's constants, the film grain) are DS3's
  data, not YEBIS's code, and stay read from the image.
- **The glare family's constant block keeps the library's layout** (the final review's M1): the plans of the glare, the shafts,
  the depth of field and the motion blur still write a 4,096-byte block at the registers the library's programmes read
  (`yebis_glare.h` `glare_globals_bytes`, the builders' word offsets, `post_glare.hlsl`'s `packoffset`s), uploaded once a draw
  (about 50 draws a frame, about 200 KB, mostly zeros). Our programmes read it as it is so that the plans' word-for-word tests
  against the original's captured blocks (T13, T14, `yebis_glare_test`, `yebis_shafts_test`, `yebis_dof_test`,
  `yebis_motionblur_test`) stay exact. Not repacked here: PLAN §5 named the blocks, so this is a part of the tranche left, carried
  to 8b (a compact block of our own per draw, the tests kept word for word through a mapping).

### The refusals: which are met in play (a Sonnet count, `measure-tools`)

Every `native_yebis_*` row of the 3,647 native journals of the evidence store (500 since 2026-11-13), its `refused` field counted
by reason, with the nearest preceding grading row's area (`evidence/I-0138/artifacts/t8-refusals/c.py` and `out.txt`; numbers checked by
hand on `full-1116-night`). Since 2026-11-13:

- **Mostly benign**: the glare's « glare shape » (2,178 rows, every run) comes mostly with the grading's « off » at the source
  `default`: the loading screens' default parameters (`full-1116-night`: each one right before a zone's served row);
  the depth of field's « no level drawn » (535) and « depth of field off » (33), the motion blur's « motion blur off » (527),
  the shafts' « off » and « disabled » (the record's own switch, I-0141), the grading's « off » and « texture absent » (the
  DLC maps and m45 ship no strip: the original draws none either).
- **Met in play, a pass missing natively**: the glare's **« bloom gamma »** (433 rows, 4 runs: 431 in the two runs of the Kiln's
  cutscene 41000060 (the ending played after the Soul of Cinder, map m41, event 14100100; first misnamed the Twin Princes'), 2 in the Twin Princes' arena's, the row's `gamma` [1.1, 0]): no bloom there. The glare's **« glare shape »** at
  Archdragon Peak's `m32_00_0100` (GlareShape 3, STUDY §2.1; 11 runs since 2026-11-13, the cutscenes c32000010 and c32000020 of
  `i139-t3c-*`, `i139-t3m-*`, `i140-e12`, `e16`, `e22`, `e24`, `i141-t1a` to `t1d`, the final review's count): no bloom there; what
  shape 3 draws needs a capture of the original (PLAN §6), as tranche 2 left it. The shafts' **« depth of field off »** (60 rows,
  23 runs: the cutscenes c01 Vordt, c30000000, c30000010, c30000030, c30009000, c31, c33, c35, c38, c39, and the Abyss Watchers' and
  Gael's arenas of `i139-v2`): the record's Enable set and the luminance positive, so the original draws shafts there and the
  native, which serves them only in the depth-of-field branch (K-0445's limit), does not. The shafts' **« record +0x24 (the FUN_141737480 composite):
  no witness »** (43 rows, 13 runs, m38: Wolnir, the Old Demon King, the Catacombs; 17 more before): no light shafts there.
- **The bloom gamma was a misread word** (a `ghidra-research` reading, GhidraMCP; its three load-bearing sites re-read by the
  pilot): the frame read the globals word +0x100 as the bloom gamma, but E69A0 passes that word to FUN_1416CB820 (call at
  1400E732E after `movss xmm1, [r14+0x100]`), which stores it at CRenderGlare +0x1A8, a field the ghost reads (FUN_14174E390). The
  bloom gamma is CRenderGlare +0x1B0, copied every frame from CPostEffect +0xC2C (1416E7887..1416E7896), whose only store found is
  the library's init (1416F0F67, 1.0; a scan of every `[..+0xc2c]` store in the mirror; a block copy of the object would escape it,
  not excluded). So the library always draws with a bloom gamma of 1: the frame now takes 1, the builder's refusal stays as a guard
  no input reaches, and the Twin Princes' bloom is no longer refused (`glare_capture.h`, the tests' path, already read +0x1B0 from
  the library object). Corroborated by the word's name: 144962070 (the globals 144961F70 + 0x100) is
  `GlareGhostConcentricDistortion` in [YEBIS-CHAIN-CHECKLIST](../I-0068-static-yebis-ffx-lights-spec/YEBIS-CHAIN-CHECKLIST.md)
  (the bloom's luminance gamma, 14496206C, is the composite's +0xFC). It is an authored, blended parameter (the integrity
  review's M1, the journals): 0 in the arena's two rows, 0.79 to 0.99 through the ending's cutscene 41000060 (431 rows, a
  different plan: quality 4, luminance 1.67, threshold 0.25, remap 0.63), 0.5 in Archdragon Peak's cutscenes c32000010 and
  c32000020 (15 rows, refused first for glare shape 3); 1 elsewhere. Both plans (the Twin Princes' arena, 41000060) are now drawn; seen natively beside the
  original in tranche 10 (« Tranche 10 »); at m32 the bloom stays refused where shape 3 is
  selected (8b keeps the refusal, « Tranche 8b, 2026-11-21 »). The sites read: `evidence/I-0138/artifacts/t8-bloom-gamma/sites.txt`.
- **The shafts' corona** (record +0x24 non-zero): the last radial blur draws into a ping-pong surface and one more draw,
  `tech_Corona`, takes it into the shaft target, the record's tint going to that draw (FUN_141750620, FUN_141737480,
  FUN_141729470; a second `ghidra-research` reading). Its programmes are in the library's table (`yebis-techniques.tsv`:
  `vs_2D_Tex2D2`, already pinned, and `ps_Corona`, RVA 0x2EE8048, 4,316 bytes, SHA-256 894BEC6D…7CBD0), disassembled in
  `evidence/I-0138/artifacts/t8-corona/`: a spectral ring around the shaft's source (the angle and the radius read a 256x1 ramp
  the library builds on the CPU from wavelengths 350 to 730 nm, and the film grain), multiplied into the blurred shafts. Not
  established statically: the blend (the pass sets no render state and inherits the device's), the ramp's colour matrices, two
  record fields' mapping. Tranche 8b: our programme written; the corona found unreached in play (« Tranche 8b, 2026-11-21 »).
- **Met in cutscenes only, once**: the depth of field's holder flags (c30009000) and the shafts' « no radial pass » (c30000010,
  three runs); the grading's « non-map record » (s31_00_0010, s39_00_0030) and « no LUT id » (one row, Ariandel's entry, as the
  original: no LUT); the tone map's « tone type » (2 rows, `i138-4b-opening-nat`, before part 4c served type 3). Left as refusals
  (the pass skipped for that frame, the game going on), named here.
- **« programmes »** (10 rows, 7 runs): tranche 1's verify runs and I-0139 tour runs on builds before tranche 2 (the default
  glare path's library programmes then); none can occur now. « vignette programme »: gone with 8a.
- So tranche 8 does not take every refusal met in play: three are left for 8b (the shafts' corona at m38, the shafts without the
  depth of field, glare shape 3 at Archdragon Peak) with the constant block above; the Twin Princes' bloom is no longer refused, its
  plan's words far from any witnessed (luminance 86.68, radius scale 5.43, remap 0.1, threshold 0.18) and never drawn natively
  yet: owed, a native run there beside the original's `i139-t2g-b16-princes-orig`.

### The step review of 8a (a fresh `step-reviewer`, `2b15cb17a..4401feb59`)

No High, no Medium; criteria 1 to 5 held as written (the original copy's disassembly read again by the reviewer). **L1** a guard in
`ChooseNativePostProgrammes` no input reaches (the table is never null): removed. **L2** the grading lost a third refusal on the
default path, not only the flags' (« programmes » while GX's copy was not yet in the engine's compute repository, or on a pin
mismatch; `GradingReady` without it): intended, the copy being ours; said in the frame's comment. **L3** comments still naming
« pinned programmes » in `model_frame.h`, the oracle's source in `grading_stage.h` (the copy from the I-0063 cache, not the
executable), `yebis_stage.h`'s « only the LUT copy is resolved »: rewritten. **L4** the new copy check runs only where the
executable, the programme cache and the capture are present (the whole test is skipped otherwise): left, said (on this PC they
are; a checkout without them tests none of the post stages either). **L5** `Compare-ToneVerify.py`'s header named the removed
flags as its source: it now says it reads the pairs already in the store.

### Tests and the build (D-0268)

- Changed: `post_tone_stage_test` calls `OpenLut` without a programme (our copy) and gains the copy's check above; no test,
  assertion or tolerance weakened, none removed. Runner changes: the four flags out of `Run-StartupTrace.ps1`'s pattern, the
  `copy` run of `Run-I138HdrMenu.ps1` on `no-yebis-tonemap.flag` (both stated above).
- `build-t8a` on `32a71e995` (the Twin Princes' fix and the step review's answers in): exit 0, 179 PASS lines
  (`evidence/I-0138/artifacts/build-t8a.log`); `post_tone_stage_test` on its six captures, WARP and the card, each printing « the
  LUT copy (ours) against GX's LutCopy2DTo3DCs and the strip: equal », the captured volumes `t16` equal through our copy, the
  pass against the library's stages byte for byte. Not witnessed in play before the merge (D-0227, D-0229): the start check
  after it; the Twin Princes' bloom not yet seen natively (no run there).

### Reviews at the merge

- **The final review** (a fresh `final-reviewer`, Opus xhigh, `main...b346f0310`; GhidraMCP unavailable to it, it read the
  executable with `dumpbin /DISASM` and scanned the built DLL): no High. **M1** the glare family's block keeps the library's
  layout, which PLAN §5 names among the blocks to remove and the records did not say: said under « What stays », carried to 8b.
  **M2** « the module reads no programme of the engine for its post chain » was false (the emissive glow's GX programmes):
  reworded. **M3** the census called glare shape benign and omitted the shafts without the depth of field, both met in play
  (Archdragon Peak's shape 3, 11 runs; the shafts in cutscenes and two arenas, 23 runs), so « refusals removed » overstated: both
  listed and carried to 8b with ROADMAP's line. Low: **L1** the bloom gamma's count (433, not 431), « programmes » misattributed,
  the scan's place, two refusals not named: corrected; **L2** stale comments (the glare's `[144962070]` sentence, `yebis_pins.h`'s
  head, « only store » without its limit): rewritten, the copy's address range corrected to 1416E7887; **L3** two journal changes
  not stated: stated; **L4** the index, STATE's next action, ROADMAP and the ledger's K-0444 and K-0449 out of step: corrected;
  **L5** the `copy` run of `Run-I138HdrMenu.ps1` (the only live tool of the HDR10 chain's 8-bit copy) removed with its flag: kept
  with `no-yebis-tonemap.flag`; **L6** the Twin Princes' plan never drawn natively: owed, a run beside the original's. What held, by
  the reviewer: criteria 1, 2, 4, 5 (the three sites read in the executable, no other store at CPostEffect +0xC2C in `.text`), 6
  and 7; no YEBIS or GX pin's name, hash or container in the built `game_bootstrap.dll` (167 DXBC containers, none of the 130
  pins). Answered in `7191b3b97` (comments, the runner and records only: no rebuild).
- **The integrity review** (a fresh `integrity-reviewer`, `main...d99383fef`): no High; the judge not touched, no test weakened, no
  case for one scene (the bloom gamma's 1 is a universal constant), the counts and the build's figures recounted and equal. **M1**
  the bloom gamma's evidence misstated: word +0x100 is an authored, blended parameter (0.79 to 0.99 in 41000060's 431 rows, 0.5
  at Archdragon Peak's cutscenes), not « 1 elsewhere but the arena »: the record rewritten with the values, the word's name
  (`GlareGhostConcentricDistortion`) and both Twin Princes' plans owed a witness. Low: **L1** the `copy` run said removed in two
  places: corrected; **L2** the Ghidra sites had no stored evidence: `t8-bloom-gamma/sites.txt`; **L3** « served » for a bloom
  not yet seen: « no longer refused ».
- **The merge** (2026-11-16): `af6963426` into `main` (`Test-Project -MergeGate` passed before); the launcher's module rebuilt
  (`evidence/I-0138/artifacts/build-launcher-af6963426.log`: exit 0, 179 PASS lines); the start check `start-af6963426` passed on
  the ASUS (the world at 3.4 s, no refusal, restored; started 17 s before the announced 16:19). The disk rotation: 1.1 GB deleted, then stopped by itself (another thread's game or compiler
  started; no folder left half deleted), 202 GB free.

## Tranche 8b: the original's captures (2026-11-16, branch `claude/r-i138-t8b`)

- **The shafts without depth of field, read** (a `ghidra-research` reading, GhidraMCP, its outputs in
  `evidence/I-0138/artifacts/t8-shafts-nodof/`): the library does not gate the shafts on the Dof record. Without depth of field the
  depth words are (c38, c3c, c40, c44) = (1, 0, 1, -0) with c48 the near plane when a shaft record exists (the far plane without
  one) (FUN_1416E9050, 1416E9836..1416E996A), the first scene-alpha draw is `tech_MadAlphaColorT2_<channel>_Sn` (the scene's colour
  and the depth's alpha into the library's scene copy, EndPostEffectScene's first DepthToSceneAlpha call at 1416E60AB) in place of
  `tech_Alpha_R`, and the record's depth is converted with the previous evaluation's words (FUN_1416EA000 at 1416E91F2). Which
  `MadAlphaColorT2` form DS3 draws (the table holds LOGRGB, REINHARD and other variants) is not read: a capture of a frame
  without depth of field is needed.
- **The captures** (`Run-I138T8bCapture.ps1`, the oracle `build-4b-oracle` of 6a3b127a, 17:42 to 18:08, the ASUS, offline, no
  measure, six runs restored; `evidence/I-0138/artifacts/runs-t8b-capture.log`): **invalid, the cutscenes played outside their
  maps** (the step review's H1 of the runner): the oracle tree's Run-AlphaCast plays `-Cutscene` before `-BonfirePages` and
  `-TravelPress` (the current runner after them), so each of the six played its scene at Eta's loaded spot (m40, the run
  `c32000010-orig-300`: the cutscene's shots 17:53:04 to 17:54:02, then the bonfire, the travel and the arrival), and its captured
  frame says nothing of the scene in its own map. Nothing is drawn from them: not the corona's absence, not shape 3's. The runner's
  header now says so; a capture in the scene's map needs the oracle runner's order fixed first.
- **The native's 32000010 at `i141-smoke`** (its own map, the current runner): the glare served with luminance 9.17, threshold 0.25,
  remap 0.23 (L 20.5408), alternating with the default records' rows (13 refused glare rows in `i141-smoke`, 7 after the cutscene's start; the sixteen once written here were `i139-t3c-c32000010-nat`'s) (« glare shape », the keys at 0xFFFFFFFF) through
  the scene: frames without bloom or with the default exposure, perhaps at the cuts (I-0141 RESULT « The ceremony's smoke: a native
  GPU capture » notes the same in 32000000); not compared with the original.
- **The captures again, in the scenes' maps** (the oracle runner patched; `i138-8b2-*`, 18:59 to 19:25, the ASUS, offline, no measure,
  six runs restored; `evidence/I-0138/artifacts/runs-t8b2-capture.log`), each the frame N library frames after the scene is handed:
  `c38000000-orig-150` and `-600`: a bloom (base 392x331, eight levels, `MadT8`), no light shaft and no `tech_Corona` drawn (no shaft
  record in the original at those two frames, where the native refuses the record's +0x24: the same picture, no shafts, by
  different paths); `c32000010-orig-300` and `-1500`: a bloom at base 157x132 (quality 1 at 1280x720, seven levels, `MadT7`, exposure
  2, threshold 0.25, remap 0.23, L 20.5408: the luminance 9.17 with both shape factors 1) **and the light shafts** (the mask, five
  radial passes, `MadT2`); `c30000010-orig-300` and `-900`: the depth of field on (its composite), so not the no-DoF case. So no
  corona, no frame whose shape is known to be 3 and no frame without depth of field was caught. What it shows: in 32000010 the
  original draws a plain bloom pyramid at glare quality 1 with L 20.54 where the native's journal of the same scene
  (`i139-t3c-c32000010-nat`, a build of 2026-11-13) alternates a quality-2 bloom (L 6.72, then 20.54 in `i141-smoke`) with rows at
  quality 1 refused for the shape: the two renderers blend the scene's glare records differently (INFERRED from two runs that are
  not frame-paired), a lead for the cutscenes' record selection (PLAN §6's « which records each cut selects »).
- **8b on hold**: the corona needs a frame of the Wolnir or Old Demon King arena in play with its shaft record (the capture at a
  chosen frame there), shape 3 the seconds of `m32_00_0100` (one native row in 32000010), the shafts without depth of field one of
  the 23 runs' frames whose Dof is off. Each is a capture in the scene's own map (the oracle runner's order fixed first), timed on
  the native journal's row; left for a later slot, the R thread going on with the ceremony's smoke. The step review of the runners
  (`f21d38c94..6d619b815`): **H1** above; **M1** the smoke runner could report an earlier run's captures: it refuses an existing run
  and lists only its own; **M2** `witness-cutscene.txt` left in the oracle build after a throw: removed in both finally blocks; **L1**,
  **L2** patterns tightened (`-SweepKeyAt` from second 1, `-KeySecond` 1 to 60, `-Burst` within the module's clamps); **L3** the key's tap
  shifts the later sweep shots by about a second: noted.

## Tranche 8b, 2026-11-21: the block of our own and the three refusals re-read (branch `claude/r-i138-t8b`)

- **(a) The glare family's constant block in a layout of our own** (`29106312d`, then the step reviews' answers): the library's
  block was 228 float4 (4,096 bytes a draw); our programmes read 73 of its registers. `yebis_glare.h` `glare_block_registers`
  lists them in the library's order (slot i is register `glare_block_registers[i]`; 1,168 bytes), `post_glare.hlsl`'s packoffsets
  are the slots (each field's comment names its library register), and `PutGlareWord` places a library word (register w/4,
  component w%4) in its slot: the builders keep the library's word numbers, their transcription of its writes. A word without a
  slot marks the draw (`unplaced`) and the stage refuses the whole plan before recording anything. The DoF composite's c18 zeros
  are no longer written (no programme of ours reads c18).
- **Tests changed** (D-0268, said plainly): the word-for-word tests (`yebis_glare_test`, `yebis_dof_test`, `yebis_motionblur_test`,
  `yebis_shafts_test`) read a library word through `post_test::LibraryWord` (0 for a register without a slot) and each checks
  `!unplaced`; their captured words and tolerances are unchanged. The stage tests (`glare_stage_test`, `dof_stage_test`,
  `mb_stage_test`) draw the library's programmes with our block spread into the library's registers (`TestConstants`,
  `LibraryBlock`) and ours with our block; `post_draws_test` draws ours with the captured block gathered into our slots
  (`OurBlock`). The flags-0 check of c18 in `yebis_dof_test` now reads `!unplaced` (c18 has no slot; the captured zeros stay
  checked by `CheckPlan`). New: `post_draws_test` reflects every programme of the family (D3DReflect): `PostGlareBlock` within
  1,168 bytes and every field, each array element included, at its library register's slot; `glare_stage_test` checks that a plan
  with an unplaced word is refused whole by `RecordPre` and `Record` before any layout; compile-time checks of the table's order
  and of the test sink's block-versus-vertices size. No tolerance or threshold was weakened.
- `Compare-Programmes.py` maps our listings' `cb0[n]` back to library registers through the table: the same verdicts as before,
  glare family 45 listing and 3 values, DoF and motion blur 17 listing, 30 values, 18 commuted, 0 different
  (`evidence/I-0138/artifacts/t8b-block/`).
- **Step reviews** (`step-reviewer`): `407670f32..29106312d` no High, no Medium; L1 a refusal per draw left a half-recorded plan
  (now the plan is refused before any transition), L2 no check of the table nor of the cbuffer's size (the static_assert and the
  reflection check), L3 a dead c18 check, L4 a stale comment, L5 the test sink told blocks by size alone (a shared constant and a
  static_assert): fixed in `27e083696`. `29106312d..27e083696` no High, no Medium; L1 an array field checked at its first
  register only, L2 `RecordPre` returned before the check on an empty pre list, L3 a comment, L4 the vertex size a second literal,
  nit the c18 form: fixed in `9653d0af9`; **L5 left**: an unplaced word now refuses the whole plan in `RecordPre`, so the frame's
  motion blur (which needs Alpha_R) is skipped with it; no builder reaches it (each builder test asserts `!unplaced`).
  `27e083696..9653d0af9` no High, no Medium; L1 the stage's refusal untested: the `glare_stage_test` check above (`7f8c8f2d5`);
  **L2 left, before this branch**: the renderer keeps no reason when the glare family fails (`scene_pass_frame.cpp`, the family
  latched off for the session; the DoF's `dof_failure_` is kept but read nowhere).
- **Builds** (lines starting « PASS » counted, the earlier records' rule): `build-t8b` (the tree of `5fcc87744`, 08:16 to 08:23)
  exit 0, 178; `build-t8b-final` (`e05781373`, main's `407d17330` merged in, -Jobs 4) exit 0, 178; `build-t8b-final2` (`932eca309`,
  the final review's answers) exit 0, 178; `build-t8b-final3` (`7e769c04d`, the step review's comments, the merged tree) exit 0, 178
  (`evidence/I-0138/artifacts/build-t8b-final*.log`; the reflection check's « 187 PostGlareBlock cbuffers within 1168 bytes » in
  the build folders' `post-draws-*.txt`).
- **(b) The shafts « without depth of field »: no witness since part 4b.** OBSERVED: of the 1,460 journals written since
  2026-11-10, 23 hold a shafts row refused « depth of field off », the last 2026-11-14 06:20, all before part 4b (`199b75db1`,
  2026-11-13 23:58) took effect in a run (`t8b-readings/shaft-refusals-all-native-since-2026-11-10.txt`, the final review's L1;
  the cutscenes re-run as `i139-t3m-*` that evening serve or refuse « off »); part 4b gave the cutscenes their s-file records
  (natively `Dof[230000010,10]=default` there). Not re-run since: c33000000 to c33000040, c35000051 and Gael's arena; the Abyss
  Watchers' arena ran twice after part 4b (`i139-t4p`, `i139-t4r`, 2026-11-15: shafts served or « off », no « depth of field
  off »), perhaps not at the fight's moments `i139-v2-b05` reached, so the absence in the current code is INFERRED, not observed (the final review's M1). The original at those moments
  (`t8b-readings/original-dof-shaft-words.txt`): in 30000010 at +300 and +900 its DoF is enabled (+0x1AC 1, focus 5.5 and 19.46)
  and its shafts disabled (+0x130 0), Alpha_R's offset c186.x = 1 - focus/near exactly (-67.75, -242.27), our `AlphaR`'s formula;
  38000000 the same (focus 3.08 and 0.64, shafts disabled). So no frame without depth of field and with shafts was ever met
  against the original; the refusal stays as a guard, a re-run of those scenes owed, and the library's `MadAlphaColorT2` form stays unread (INFERRED: the
  library's DoF off altogether, perhaps the game's own option, not a record).
- **(b) Glare shapes other than 1: met in the current code** (the final review's H1). The refusal (`13b_prepare_model_frame.inc`,
  `integer(0xD8)!=1`) takes the whole glare family for that frame: the bloom, and with it the depth of field, the motion blur and
  the shafts, which hang on the glare's plan.
  - **Shape 3** (`m32_00_0100`): OBSERVED (`t8b-readings/shape3-native-rows.txt`), in the cutscene it was the Bloom record for one
    collect at the end of 32000010 played with the Nameless King dead (e.g. `i141-t1a`, about 52 s after the scene is handed),
    never in the 17 runs of 32000010 since I-0141 tranche 2 restored the zone's true state; 32000020 (5 rows, `i139-t3c`,
    2026-11-13) not played since. **In play** the file is a slot file: 5 of m32_00's 59 collisions name slot 1
    (`evidence/I-0066/queries/ae/msb-gparam-slots.tsv`, `m32_00_0100 m32_00_0110`; K-0448: a slot file is the collision's bytes x
    100), so standing on one selects it (INFERRED); which collisions and where is not read, and the Nameless King's arena is not
    one (its runs `i139-t2b`, `t2e`, `t2h`, `t2i` hold no 10-level row).
  - **Shape 0** (the `default` file's Bloom, ids 0 and 12, luminance 3, levels 8, threshold 0.5): tranche 8 called it benign, the
    loading screens' default parameters; but it also comes mid-cutscene, a frame or two at a time between served rows (OBSERVED:
    `i141-t14h-c32000010-nat` 13 rows, `i139-t2i-b18-namelessking-nat` 10 through 32000010's served rows; « glare shape » rows in
    47 journals since 2026-11-19, the loading screens' included), perhaps at the cuts (the records' keys 0xFFFFFFFF). The game selects the record in both modes, so the original's
    library receives shape 0 on those frames too (INFERRED); what it draws then is not read (I-0068's converter writes shape 0 when
    the player's glare factor is not positive, so 0 likely means no bloom, INFERRED), and whether it keeps the depth of field and
    the motion blur, which the native drops with the glare, is not read.
  - The refusal stays; owed: the original's answer for shape 0 (a reading of the library's shape switch, and a capture on such a
    frame) and for shape 3 (a native journal and an original capture on one of m32_00's slot-1 collisions).
- **(b) The corona at m38: not reached in play.** The capture (`Run-I138T8bCapture.ps1` with `-CutsceneShots` on the oracle
  `build-4b-oracle`, its runner rebuilt from `6a3b127a` and `t8-oracle-runner.diff`; 08:29 to 08:39:47, the ASUS, offline, no
  measure, both runs restored by hash; `evidence/I-0138/artifacts/runs-t8b3-capture.log`): the Wolnir run's arrival (tab 563, row
  346), scene 38000000 handed, the frame 4,200 and 6,600 library frames later, in play at the bonfire. OBSERVED in both
  (`i138-8b3-m38-wolnir-orig-4200`, `-6600`; `t8b-readings/corona-capture-m38.txt`): the globals' LightShaft Enable (+0x130) 0,
  the glare luminance 8, DiffractionRing (+0x168) 3.36; 35 draws, no shaft mask, no `tech_Corona`: the ring is authored, the shafts
  are off, so the library keeps no shaft record. Every native « record +0x24 » refusal (29 journals since 2026-11-10, the last
  2026-11-15 12:05, `t8b-readings/shaft-refusals-all-native-since-2026-11-10.txt`) predates the Enable gate (`d66d57a82`, 2026-11-15 14:05, « disabled » first); none has been written at m38 since (so the absence in the current
  code is INFERRED: a native run at m38 after the gate is owed). In the census (`census-world/effective.tsv`: the 118 files of the 168-file dump that
  are not cutscene files, 107 of them map files; m51_01's files absent from the dump, the Gael runs serving shafts) the ring is
  non-zero in m38_00's files (3.36) and in m36_00's `0000` and `0100` (0.16, their shafts on too; m36_00 not a retail world map,
  INFERRED unreachable, K-0448), and among m38_00's the shafts are on only in `m38_00_0002` (the census'
  `per-file-sheet.tsv`), a file no collision can name (K-0448: slot files are the collision bytes x 100; m38_00's name 0100, 0200,
  0600); cutscene files carry no ring. INFERRED: the corona is not drawn in the retail game's play; a cut selecting `m38_00_0002`
  through its SceneDrawParam tracks is not excluded (unparsed), but the two c38000000 captures (`i138-8b2`) hold no shaft record.
  **Decision of the pilot**: the refusal stays as a guard (as for shape 3 and the shafts without depth of field, D-0290's order),
  the corona's native code is set aside unbuilt in `t8b-corona-set-aside.diff` (BuildShaft's `tech_Corona` draw, the ramp's
  generation from the executable's words, the stage's upload as input -7, the frame's wiring), with the ramp's float32 emulation
  `t8b-corona-ramp.py`; `ps_Corona` and its pin stay (the static half, compared). The diff applies on `e26c3597f`.
- **(b) The corona, read statically** (GhidraMCP, the pilot; `t8b-readings/corona-static.txt`): with record +0x24 non-zero
  FUN_141750620 sends the last radial blur (still weighted by the factor and the colour, but no longer tinted by +0x34..+0x3C)
  into a ping-pong surface, then FUN_141737480 and FUN_141729470 draw `tech_Corona` from it into the shaft target: t0 that
  surface, t1 the spectral ramp, t2 the film grain; c154 the tint (+0x34..+0x40), c155 (1.5, 1.5, 1.5, 0), c156 (1, 1, 1, 0);
  c15 (+0x24, +0x28, +0x2C, 0), c10 (2, 4, 0, 1); TEXCOORD1 the mask's third rectangle with tan(fov/2) x pi / (+0x30). The ramp
  (FUN_141719770, built once): 256x1 RGBA8, texel i the colour of wavelength 350 + 380 i / 255 nm through the executable's CIE
  table (14469AE70, 97 rows), the sRGB matrix, a normalisation by the table's largest component, a soft clip (0.2 x the
  luminance), the alpha from a second table (14469ABD0) and a 1/2.2 power, texels 0 and 255 black; its mips a 2x2 box with a
  4x4 table. ps_Corona (`post_glare.hlsl` `PS_CORONA`) is ours: `Compare-Programmes.py` values, one add commuted. Its builder,
  the ramp's native generation are set aside (above); the blend stays unread.
- **Step review** `9653d0af9..5fcc87744` (the refusal test and the corona's programme): no High, no Medium; six Lows (the
  refusal test's « still needs a layout » could not fail with no programmes, an unplaced pre draw untested, the check only with
  the captures, the commuted add avoidable, `tech_Corona` not among the pins' hand rows, a stale count in `Compare-Programmes.py`).
  Answered in `e26c3597f`: the refusal test uses the plan's own programmes and adds an unplaced pre draw (`RecordPre` refused,
  `NeedsLayout` checked on both plans); `tech_Corona` among the pins' hand rows; the count corrected. **Left**: the check needs
  the captures (as 8a's L4: present on this PC); the commuted add, fxc writing `add_sat 1, -x` whichever order the source gives
  (both tried), said in the shader's comment.

### Tranche 8b: reviews at the merge

- **The final review** (a fresh `final-reviewer`, Opus xhigh, `main...af27d5b69`, main `407d17330` merged in): criteria 1 to 5
  held (the 73 slots and the packoffsets, the reflection check printed by the build, `PutGlareWord` and the refusal before any
  recording, the builders' words and the tests' tolerances unchanged, `ps_Corona` values-commuted and never drawn, the three
  refusals in the code, the set-aside diff applying and uncompiled); criterion 6 not met. **H1** « glare shape 3 met only in an
  artificial state » and « met in no current frame » were false: shape 0 (the default record) is met mid-cutscene and shape 3 is a
  slot file in play: the records rewritten (« Glare shapes other than 1 » above), ROADMAP's item 8 kept open as 8c. **M1** the
  corona's and the shafts without depth of field's absence is INFERRED (no run at m38 after the gate, the scenes not re-run since
  part 4b): said, the re-runs named. Lows: **L1** the counts (29 journals for +0x24, 23 for the depth of field, over all 1,460
  journals since 2026-11-10: `shaft-refusals-all-native-since-2026-11-10.txt`); **L2** `yebis_pins.h`'s « met in play » and **L3**
  `PS_CORONA`'s weights described the wrong way round (weights[0] is the tint c154, [1] the ramp's scale c155, [2] the output's
  factor c156): rewritten; **L6** « merged » and « done » written before the merge, the final build unrecorded: corrected;
  **L7** a stale sentence on shape 3 at m32; **L8** the census' coverage stated; **L9** a `static_assert` that the slots fit an
  `int8_t`. **Left**: **L4** `ps_Corona` and its row compiled and pinned but never drawn: kept as the static half the set-aside
  draw needs (the listing compared, no GPU test runs it); **L5** an unplaced word latches the glare family off for the session
  without a reason in the journal: no builder reaches it (each builder test asserts `!unplaced`), as the step reviews' L5 and L2.
- **The step review of those answers** (`af27d5b69..932eca309`, native/ only): no High, no Medium; L1 the `static_assert` one below
  what an `int8_t` indexes (kept as a margin, said), L2 « the ramp's scale » naming two fields, L3 the pins header's « not reached »
  stated as fact and the diff without its folder: the comments rewritten (no bytecode or pin changes: comments only).
- **The integrity review** (a fresh `integrity-reviewer`, `main...932eca309`): no High, no Medium; nothing weakened (the word-for-word
  tests' captured words and tolerances unchanged, their check counts up, no gate file touched, no scene singled out). Lows, the
  records corrected: **L1** 178 and 179 PASS counted by two rules (now lines starting « PASS », 178 for every build); **L2** the
  merged tree's build unrecorded (`build-t8b-final2` and `-final3` above); **L3** m36_00's ring and shafts left out; **L4** « 119 »
  files (118); **L5** the Abyss Watchers' two later runs not cited; the nit, the reflection line's file.
- **The merge** (2026-11-21): `6f60aed41` into `main` (`Test-Project -MergeGate` passed before); the launcher's module rebuilt
  (`evidence/I-0138/artifacts/build-launcher-6f60aed41.log`: exit 0, 178 lines starting « PASS »); the start check `start-6f60aed41`
  passed on the ASUS (09:55 to 09:56, 355 world frames, no fault, restored). The disk rotation deferred: another thread's merge
  was building.

## Tranche 8c: the glare shapes other than 1 (2026-11-21, the R pilot, branch `claude/r-i138-t8c`)

ROADMAP workstream 11, item 8's 8c (tranche 8b's final review H1: shape 0 met mid-cutscene, shape 3 on m32_00's slot-1
collisions, the native then dropping the bloom, the depth of field, the motion blur and the shafts); D-0290 (the original's answer
read before a refusal becomes a skip).

- **The original's answer** (READ, GhidraMCP on DarkSoulsIII.exe 1.15.2, a ghidra-research reading,
  `evidence/I-0138/t8c-readings/glare-shape-reading.md` with its scans; the gate's bytes at 1416E755B and the depth of field's call
  at 1416E631E re-read by the pilot, the two setters too):
  - The converter (1400E4460) copies the Bloom record's word 8 into the globals' +0xD8, 0 when the player's glare factor is not
    positive; E69A0 passes it each frame to 1416C9690 (14 is the custom glare), which stores it as it is at CPostEffect +0xBC4;
    1416C9670 stores the glare luminance (+0x6A4) only while RemapFactor is positive.
  - EndPostEffectScene_InternalProcess (1416E5BC0) skips the whole glare family, from the bright pass to the composite with the
    light shafts, when the shape is 0 (1416E7583 `TEST ECX,ECX; JZ 1416E7B87`) or the luminance is not positive (1416E758B); the tone
    map still runs, its glare factor 0 (GatherToneMapFlags 1416EB110; a default texture in place of the glare's, black INFERRED).
  - The depth of field (1416E2910, called at 1416E631E, gated by +0x51C and +0xC33), Alpha_R (1416DF6E0, 1416E63AB) and the motion
    blur (1416E21E0, 1416E63BE) run before that gate and never read the shape.
  - The shapes (the library's table at 14469BD70): 0 Disable, 1 Bloom, 2 Lens Flare, 3 Standard, 4 Cheap Lens Camera, 5
    Afterimage, 6 to 11 the cross filters, 12 and 13 the streaks. Shapes 1 to 13 draw the bright pass, the pyramid, the shafts and the
    composite; shape 3 adds four-arm streaks (FUN_14174D3A0), a ghost (14174E390) and an afterimage (141750000).
- **The native answer** (`13b_prepare_model_frame.inc`, `scene_pass_frame.cpp`): a luminance not positive or a shape other than 1
  serves an empty glare plan at the frame's extent: no glare draw, no shaft, the tone map's t1 black, and the depth of field kept
  (it hung on the glare's input). Shape 0 and the luminance are the library's own skip; shape 3 and the others are not served
  natively (the row's `refused` names the shape), their depth of field kept the same way. The `native_yebis_glare` row now carries
  `shape` and `skipped` (« shape 0 », « luminance »); the shafts' row names the glare's refusal, the refused shape or « glare skipped ».
- **Alpha_R and the motion blur on those frames** (after the final review's M1, `d4bb13937`): the library draws Alpha_R by its
  depth-factor stage (1416E0040, called at 1416E6218 before the depth of field and the motion blur) when its flag +0xC7A is set, an
  OR of the motion blur's early flag (+0xC64: the blur on at a mode above 1, or its falloff test), the light shafts' (+0x9A0) and other stages (1416E42B0, the store at
  1416E4CE2), never the shape; the motion blur takes its depth from the scene's alpha (1416E2642, channel 3); READ, GhidraMCP, a
  ghidra-research reading (`evidence/I-0138/t8c-readings/alpha-r-gate-reading.md`; that the blur's texture is the one Alpha_R wrote,
  INFERRED). Natively Alpha_R was made only by the shafts, so the blur was absent on those frames and, at shape 1, whenever the
  shafts' setting was off or the shafts refused before their Alpha_R (« disabled », the map's LightShaft Enable 0, met in 46
  journals, among them 45000030 and 32000010's m32_00_0010 moments; « post colours », « depth of field off »). Now `AppendNativeAlphaR` adds it when the globals ask for the blur (Enable +0x1DC, mode 2) and the shafts
  did not draw it, the plan then built every frame (its words follow the camera); the blur's row names « no Alpha_R » if it is still
  missing. The gate itself is `yebis::GlareShapeGate` with a unit test (the final review's L9).
- **Open**: shape 3's streaks, ghost, afterimage and light shafts, and the other shapes the library draws (the passes to read and
  write; until then m32_00's slot-1 collisions and 32000010's shape-3 frames show no bloom and no shaft where the original shows
  the « Standard » glare); at shape 1 a BuildGlare or programmes refusal still takes the depth of field with it (none met). That the
  original also receives shape 0 on the cutscenes' default-record frames is INFERRED (tranche 8b), its capture on such a frame not
  taken; whether the 15 cutscene files that author `GlareShape` 0 (`census-cutscenes/feature-counts.tsv`) are ever selected is not
  read. The native's Alpha_R and motion blur hang on the Dof record's Enable (`dof[0]`, « depth of field off »), the library's do not
  (+0xC7A and +0xC64 never read the DoF's enable c33, READ; the depth spaces without the DoF, INFERRED): on the cutscenes' shape-0
  frames, whose default record holds Dof Enable 0 and MotionBlur Enable 1 (`census-world/effective.tsv`, file `default`), the native
  draws neither the depth of field (as the original, by its record) nor the motion blur (the original does, INFERRED from its
  record). Commit `863fe1123`'s subject (« no longer takes ... the motion blur with it », « shape 0 the original's picture ») is
  superseded by this section.
- **The step review** (fresh, high, `edd2e9c66..863fe1123`): no High. **M1** the motion blur claimed kept while Alpha_R, and so the
  blur, is drawn only with the shafts: the comment and this section say it goes, open. Lows: L1 the row silent on shape 0 and the
  luminance (`shape`, `skipped`); L2 the shafts' « off » for a skipped glare; L3 « the library skips » for shapes it draws; L4 the
  frame's words against the library's stored copies (both setters read: the shape stored as it is, the luminance held while
  RemapFactor is not positive, INFERRED alike here); L5 a shape 3 frame with a luminance not positive named as a refusal (the
  luminance tested first). L6 (a shape 1 refusal still takes the DoF) recorded in « Open ».
- **The fix's step review** (fresh, high, `863fe1123..556a0ff1c`): no High or Medium. Lows answered (`1a5f34843`): L-a the motion
  blur's sentence unambiguous (not drawn natively on those frames); L-b the luminance's held copy: no glare row with RemapFactor not
  positive in the 833 journals holding glare rows of the last ten days (`evidence/I-0066/artifacts`; the frame's word kept, INFERRED alike); L-c the shafts' own luminance test, now
  unreachable from its caller, a guard; L-d the shafts' row names the glare's refusal or the shape before « glare skipped ».
- **The second fix's step review** (fresh, high, `556a0ff1c..1a5f34843`): no High or Medium; its comment Lows answered (L1 the
  guard named, L2 the motion blur absent for the other shapes too, L5 the journals named); kept (older than this tranche): L3 the
  shafts' row written before `ChooseNativePostProgrammes` can refuse the plan (no programme missing in any plan the builders make),
  L4 the shafts' row silent while the setting is off.
- **The witnesses** (D-0290; `Run-I138T8cWitness.ps1`: i141-t14h-c32000010-nat's arguments, 32000010 from Eta's save, the ASUS
  1920x1080 window, offline, each save and GraphicsConfig restored by hash, no measure; logs `evidence/I-0138/t8c-readings/witness-*.log`):
  - `i138-t8c-c32000010-nat` (`build-t8c-3`, 12:25-12:32, the PC's own levels: DoF and motion blur DISABLE): no fault, 18,160 native
    frames; glare rows 9 « skipped shape 0 », 1 « glare shape 3 », 9 served at shape 1; the shafts' rows say « glare skipped » or the
    shape. Its DoF and blur rows read « no level drawn » and « motion blur off » (the PC's settings; « depth of field off » on the
    shape-0 frames), so they witness nothing of the
    tranche (the final review's M2).
  - `i138-t8c-c32000010-nat-b` (`build-t8c-4`, 12:52-12:59, `DepthOfField=HIGH;MotionBlur=HIGH`): no fault, 17,894 native frames;
    the same 9, 1 and 9 glare rows. At shape 1 the DoF (7 to 10 draws) and the blur (2 or 3) on; on the one shape-3 frame (serial
    16163, journal line 63,410) neither row changed (rows are written on a change), so both stayed on with no glare draw (INFERRED
    from that silence); on each shape-0 frame both rows « depth of field off » (the record's
    Dof Enable 0, Open above).
  - `i138-t8c-c45000030-nat` (`build-t8c-5`, 13:28-13:33, the same levels, `-Source i141-t14a-c45000030-nat`'s arguments): no fault,
    15,861 native frames; glare rows 7 « skipped shape 0 », 6 served at shape 1; the shafts « disabled » twice at shape 1 (journal
    lines 40,173 and 56,984) and no blur row changed after them (rows written on a change), so the blur stayed on, its Alpha_R from
    `AppendNativeAlphaR` by the code (the shafts' « disabled » returns before their Alpha_R; no Alpha_R row or picture shows it); at shape 1 the DoF 10 to 12 draws, the blur on; each shape-0 frame « depth of field off » for both.
    `build-t8c-6`'s `game_bootstrap.dll` is byte-identical to `build-t8c-5`'s (SHA-256 `2cc7147e…`), so this witness ran the final tree.
- **The final review** (fresh, xhigh, `main...38cf76381`): no High. **M1** the motion blur still dropped, silently, on those frames:
  answered by Alpha_R for the blur (above, `d4bb13937`), with the blur's « no Alpha_R » and « no glare plan » reasons; the DoF-enable
  part left Open. **M2** the witness could not show the tranche (wrong default build, the PC's DoF and blur off, no shape 3): the
  runner takes the build, run name and log as mandatory and adds the levels; the second witness above; shape 3 met in 32000010.
  **M3** Open completed (the witness, the original's capture, the INFERRED premise). Lows: L1 « glare skipped: <why> »; L2 shape -1
  without the globals; L3 the runner's checkout from its own folder, its log mandatory; L4 STATE's rows; L5 the next step and
  ROADMAP rewritten; L6 the commit subject superseded (Open); L7 shape 3's shafts named; L8 word 14 stored as -1 (the shape here is the
  globals' word, refused as « glare shape 14 »; the luminance test a guard); L9 `GlareShapeGate` and its unit test.
- **The fix's step review** (fresh, high, `38cf76381..d4bb13937`): no High or Medium. Answered (`153af97c0`): L3 the blur's row
  without a glare plan; L4 the shafts' row before the blur's Alpha_R; L5 the comments (+0xC64 at a mode above 1 or the falloff, the
  gate's naming order ours). Left: L1 a shape 0/1 toggle with the blur on relays out the glare stage (its surfaces retired and made
  again: in 32000010 twice, around its one shape-3 frame, since on its shape-0 frames the native's Alpha_R refuses with the
  Dof record's Enable 0 (the final review's L1); allocation, not compilation; the blur is off in Thomas's settings); L2 the plan built every frame
  with the blur on and the shafts off (already so with the shafts, the default).
- **The Lows' step review** (fresh, high, `d4bb13937..153af97c0`): no High or Medium; four comment Lows answered (`69d0d5fd1`: the
  blur's « no glare plan » causes, the shafts' refusals before their Alpha_R, +0xC64's enable, the blur block's header). Recorded: with
  the blur's threshold flag or falloff the glare plan still carries Alpha_R and is built every frame though the blur refuses (the
  library draws it there too, by its mode; no native consumer).
- **Builds** (`-Jobs 4`, the cache `evidence/I-0138/cache-t8c`): `build-t8c-1` (`863fe1123`), `build-t8c-2` (`556a0ff1c`),
  `build-t8c-3` (`1a5f34843`; the later commits comments only), `build-t8c-4` (`d4bb13937`, the glare gate's test added to the
  glare test's output: « PASS yebis glare gate », 9 cases, in `yebis-glare.txt`), `build-t8c-5` (`0b720789c`, the final tree but
  one comment) and `build-t8c-6` (`fec54f3ff`, that comment): 178 PASS lines each and the gate's line, no C++ warning. The trees
  witnessed: `build-t8c-3` and `build-t8c-4` (32000010), `build-t8c-5` (45000030); `153af97c0` after the second witness moved the
  blur's Alpha_R after the shafts' row and named « no glare plan » (a row's count and a refusal's name, no draw changed), so that
  witness's shape-3 shafts row reads « alpha »:1 where the final tree writes 0.
- **The integrity review** (fresh, `main...9041b0ada` with this section): no High or Medium; nothing in the judge touched, no test
  weakened, no scene key, no work cut in a measure; the stated numbers matched their evidence. Lows answered: L1 and L2 the blur's
  « stayed on » qualified as read from the rows' silence (above), L3 the 45000030 witness's source run named.

## Tranche 8d: the motion blur and Alpha_R without the Dof record's Enable (2026-11-21, the R pilot, branch `claude/r-i138-t8d`)

ROADMAP workstream 11, item 8's 8d (tranche 8c's « Open »: the native's Alpha_R and motion blur hung on the Dof record's Enable,
the library's do not).

- **The original's answer** (READ, GhidraMCP on DarkSoulsIII.exe 1.15.2 by the pilot, with disp32 and rel32 byte scans of the file;
  `evidence/I-0138/t8d-readings/alpha-r-spaces-reading.md`): FUN_1416E9050 writes each frame the depth words c38..c48 that Alpha_R's
  block (FUN_141727DA0, DepthConversion(6, 2)) reads. In the depth-of-field branch (S+0xC33: S+0x6CC, the quality factor x the f-number, positive, near
  above 0 and below far, the Dof holder's +0x164 and S+0x924) they are the native's `DofDepthSpaces`. Outside it c48 is the range's
  start and c38 = 1, c3c = 0, c40 = 1, c44 = -0; the range is [near, far] clamped to [S+0x718, S+0x71C] when a shaft record exists or
  the blur is on at mode 2, else c48 = far. Three optional ranges and a 64000 rescale (S+0x720) would change it: their setters are
  reached only from Initialize (with 0) or from wrappers nothing references in the file (S+0x710/+0x714 by 1416CAF10, S+0x720 by
  1416CAF30, S+0xA40 by 1416CAD80), and the AntiAlias stage's S+0xBA4 from the frame block's +0x220, 0 in every shipped file;
  INFERRED: none runs in DS3 (D-0272's encrypted-at-rest code is not covered by a scan of the file). S+0x718 is then
  clamp(0, near, 0.99 far) = near, so c48 = near.
- **Observed before the change**: `i141-t14a-c45000030-orig`'s `reference_shaft_depth` rows with `dof` 0 (23: 22 hold conv
  [1, 0, 1, -0, c48], one is all zeros) with c48 the previous frame's far plane, the no-blur, no-shaft case, as read.
- **The native answer**: `yebis::NearDepthSpaces(near, far)` (c48 = near, c38 = 1, c3c = 0, c40 = 1, c44 = -0);
  `NativeAlphaRInputs` (06_pass_programmes.inc) takes the lens from `DofLensState` and the spaces from `DofDepthSpaces` in the
  branch, `NearDepthSpaces` outside it; `AppendNativeAlphaR` and the motion blur (13b_prepare_model_frame.inc) no longer refuse
  « depth of field off »; the blur's row carries `"depth":"focus"` or `"near"`. The shafts keep their « depth of field off » refusal
  (their own words read only in the branch), but their record and Alpha_R now take the same spaces: where the Dof record's Enable is
  set and the branch is not (a focus of 0, met in `i141-t6b` to `t13a-c45000030-nat` and `i139-t3c-c45000030-nat` by the final review's L5), NearDepthSpaces
  in place of the focus spaces, near and far the lens's sanitised values, and a new guard « executable data »; with a slot colour
  of 0 the native draws zero-weight shafts where the library keeps no record (its c48 then far without the blur): no visible
  change INFERRED (a zero last weight adds nothing); this path outside the branch has no witness yet (the 8d witness's shafts are
  served only at shape 1, in the branch). **The test changed** (D-0268): `yebis_shafts_test`'s zone check no longer expects the
  branch byte set; it prints it and checks the focus in the branch, c48 = near outside it; two hand checks added (NearDepthSpaces'
  words with c44's sign bit, Alpha_R's scale and offset outside the branch for 0.08/10000: the implementation's expression restated,
  not an oracle; no capture outside the branch exists). The shafts test's count went from 2801 (`build-t8c-6`) to 2797: the six
  zone captures' branch asserts removed, two hand checks added.
- **The witnesses** (`Run-I138T8dWitness.ps1`, `build-t8d-1` (`8775c796d`): each source run's arguments in its own mode, 45000030
  from Shebang's save, the ASUS 1920x1080 window, `DepthOfField=HIGH;MotionBlur=HIGH`, 240 s, offline, each save and GraphicsConfig
  restored by hash, no measure; logs `evidence/I-0138/t8d-readings/witness-*.log`):
  - `i138-t8d-c45000030-orig` (Reference, `shaft-observe`, 20:50-20:54): no fault; 600 `reference_shaft_depth` rows, 23 of them
    with `dof` 0 and 577 with `dof` 1, as in `i141-t14a-c45000030-orig` (the PC's own levels, the blur off). At the same positions
    of the dof-0 windows (not matched on the game clock, D-0299; both the depth-of-field and the blur options changed between the
    two runs, so the blur's part rests on the reading): the 17 rows that held conv [1, 0, 1, -0, 10000] (the far plane) with the blur off hold [1, 0, 1, -0, 0.08] (the near
    plane) with it on, OBSERVED as read; the 5 rows holding 1000 and the one all-zero row are the same in both (the previous frame's
    far plane after a 1/1000 frame, the rows read at FUN_1416EA000's call). The branch's 577 rows are there with the depth-of-field
    option off too: the option does not clear S+0xC33.
  - `i138-t8d-c45000030-nat` (Native, the same flags, 20:54-20:58): no fault, 13,192 native frames; glare rows 6 « skipped shape 0 »
    and served at shape 1; the motion blur's rows alternate 6 times « refused "" depth near » (the shape-0 frames: the glare
    skipped, Alpha_R from `AppendNativeAlphaR`) and 6 times « depth focus » (shape 1, in the branch), no other refusal; before this
    tranche the shape-0 frames read « depth of field off » (tranche 8c's `i138-t8c-c45000030-nat`). The shafts' rows: served (1
    shaft) at shape 1, « disabled » once, « glare skipped: shape 0 » 6 times.
- **The step review** (fresh, high, `2a7cc646e..8775c796d`): no High or Medium. Lows answered (`a915fa8e0`, comments): L1 the blur's
  « executable data » unreachable (DofImageData's ok fixed for the process; the row then reads « no Alpha_R »), now named a guard; L2
  the stale « through the depth-of-field branch » sentence; L3 the ranges that would narrow c48 (the blur's falloff at mode 1 too:
  every journal row is mode 2) named, unchecked natively; L4 READ and INFERRED split; L5 the record's depth converted with the
  previous frame's words in the library (one frame apart when the branch toggles) named; L6 the shafts' refusals listed. Kept: L7
  the lens computed three times per frame (negligible); L8 the native branch rule against S+0xC33 is already `yebis_dof_test`'s
  check (`lens.branch` against S+0xC33 on every DoF capture), corrected by the final review's L1: those captures are all in the
  branch, so the false side has no oracle (below).
- **Shape 3's « Standard » glare, read** (a ghidra-research reading, GhidraMCP and the file's bytes, the DS3 values from the captured
  CRenderGlare objects; `evidence/I-0138/t8d-readings/glare-shape3-passes.md` with its scripts and the two programmes' `fxc
  /dumpbin`): the record of id 3 (d0 1, d2 1, d3 0, d4 0.5, d7 0.5, preset 1 « Cross »: 4 arms, a 1.0, length 1.25) and, on top of
  shape 1's bright pass and pyramid:
  - the pyramid's combine takes a tint 0.4 (|d7| x 0.8) that colour-cycles the levels (`yebis::BuildGlare` refuses a tint, so shape
    3's pyramid is not shape 1's);
  - the streaks (FUN_14174D3A0, layer +0x250): a source mix (0.75, 0.1875, 0.0625 of the bright pass and levels 0 and 1), then per
    arm (angle d6 - glare+0x15C - i x 90 degrees: the cross turns with the exposure value) 3 passes of `tech_MadT1_S4` at 4^p texels
    with weights 0.5^(...) and a colour row per tap, ping-ponging over two surfaces, and a `tech_MaxColorT4` composite (the arms'
    maximum, not their sum);
  - the afterimage (141750000, layer +0x270): one `tech_MadOffsetT2` draw mixing level 0 with a history surface of a two-surface
    group flipped each frame (weights about g118/8 and 0.8656 at 1/60 s), so a state kept across frames;
  - the ghost (14174E390, layer +0x260): two draws (`tech_GlareGhost_PointSymmetryDistScaleT4_S8_TrimT1_S8` INFERRED, then
    `tech_GlareGhost_PointSymmetryT3_S8`) over four pyramid levels and a trim texture, eight ghosts whose lanes come from an xorshift
    generator and cosines; NOT REACHED: those final lanes, the `powf` exponents, the ghost programmes and their vertex layout;
  - the composite (1417511C0) sums the layers with weight 1 (`MadT4` or `MadT5` with the shafts). The fixed star (+0x17C) is off in
    every DS3 capture.
- **Shape 3 not written in this tranche** (the pilot's call): about twenty new draws, three new programmes, a state kept across
  frames, the ghost's lanes unread, and no original capture of a shape-3 frame to check them word for word as every other glare
  draw is (shape 3 is met on five m32_00 slot-1 collisions whose places are not recovered, no in-play journal holding
  `m32_00_0100`, on one fallback frame of 32000010, and in 32000020: `i139-t3c-c32000020-nat`, before I-0141 tranche 2, 6
  `native_yebis_sources` rows naming m32_00_0100 and 5 « glare shape » windows, not played since). It goes to tranche 8e, opened by that capture (a reference capture
  triggered by the globals' shape word 3) and the ghost's remaining reading. Until then a shape-3 frame keeps tranche 8c's answer:
  no glare draw, no shaft, the depth of field and, now, the motion blur kept.
- **Kept**: the glare stage relays out its surfaces when a plan of Alpha_R alone (shape 0 with the blur on) follows a full plan
  (tranche 8c's L1, more frequent now that the blur is served there: 6 toggles in 45000030); the stage already relays out whenever
  the shafts' draw count changes (29, 31 and 32 draws in the same run), allocation and not compilation (`PostDraws::Pipeline`
  caches its pipelines), its cost not measured; carried to tranche 8e (the final review's M1: keeping the surfaces when a plan
  holds only pre draws needs the pre draws' tables, numbered after the main draws, renumbered).
- **Builds** (`-Jobs 4`, the cache `evidence/I-0138/cache-t8d`): `build-t8d-1` (`8775c796d`) and `build-t8d-2` (`80eea3466`, main's
  `1f87a0f02` merged in, the final tree's code; the later commits records and comments only), 178 PASS lines each, « PASS yebis
  shafts: ... 2797 checks, 0 failures », no C++ warning (`evidence/I-0138/t8d-readings/build-t8d-2.log`).
- **The final review** (fresh, xhigh, `main...7b773995b`, main `1f87a0f02` merged in): no High; criteria 1 to 4 and 7 met, 5 and 6
  with the records below. **M1** the plan's « glare stage keeping its surfaces on a plan of Alpha_R alone » dropped, not done or
  carried: carried to 8e (« Kept » above, ROADMAP). Lows answered in the records: L1 yebis_dof_test's branch check never sees the
  false side (above); L2 32000020 named among shape 3's frames; L3 `build-t8d-2` and the test's count; L4 the A/B's positions not
  clock-matched, two options changed; L5 the shafts' change stated (above) and « a kept record's case » in `yebis_shafts.h`; L6 the
  hand checks not an oracle; L9 S+0x6CC named; L10 the investigations' README and the reading's last sentence. Left with their
  reason: L7 `NativeAlphaRInputs` gated on all of `DofImageData`'s words where Alpha_R needs only the constants (the words come
  from the pinned executable, read once, all or none in every build so far); L8 the blur's decodes computed from default spaces
  on the guard path (unused: the plan is not built when refused) and the falloff test on the raw camera planes against the lens's
  sanitised ones (different only for a near or far plane not positive, which the lens's branch excludes).
- **The integrity review** (fresh, `main...147f68cd3` with this section): no High or Medium; no test weakened (the change stated,
  the count 2801 to 2797 consistent), no scene key, nothing cut in a run, the numbers matched their evidence. Lows answered: L1 the
  t14a dof-0 rows 23 (22 of the c48 form and one all zeros; the reading likewise), L2 the shafts' « no visible change » marked
  INFERRED, their path outside the branch unwitnessed.

## Tranche 8e: the glare stage on a plan of Alpha_R alone, and the original captured on shape 3 (2026-11-21, the R pilot, branch `claude/r-i138-t8e`)

ROADMAP workstream 11, item 8's 8e (tranche 8d's « Kept » and its final review's M1; shape 3's capture). Shape 3's writing moved
to tranche 8f (below).

- **The glare stage keeps its surfaces on a plan of pre draws alone** (`native/renderer/glare_stage.{h,cpp}`): a plan of Alpha_R
  alone (a shape-0 frame with the motion blur on) no longer relays out the last full plan's surfaces. `NeedsLayout` compares only
  its pre draws then (a pre draw reading the DoF table keeps the former rule); the pre draws' post tables sit at the heap's top
  (`PreTable(j) = draws_max - 1 - j`), the main draws' at its bottom; `WriteDrawInputs` writes the main draws' inputs from the
  layout's own record (`inputs_`, `layout_`), not from the plan, so a pre-only frame's heap renewal keeps them. **The test**
  (`glare_stage_test`): the first zone's plan with an Alpha_R pre draw through a stage of its own, then frames in the renderer's
  order (scene_pass_frame: RecordPre, then Record): a pre-only frame at a new generation, a full frame, a pre-only frame with a new
  depth at the same generation (the heap renewed, the main tables from the layout's record), and a full frame with nothing changed
  (Record writes no table and draws from those); `Generation()` and the surfaces unchanged at each, a different pre draw still
  asks for a layout, and (`-ours`) the full plan's surfaces equal to the zone's stage drawing the plan alone. « glare stage: 1
  plans' pre draws alone keep the layout (tranche 8e) » in every profile.
- **The original's stored shape** (`reference_glare_shape`, `07_scene_lighting.inc` ObserveShaftKeeper, `shaft-observe.flag`,
  original runs only): S+0xBC4 (1416C9690 stores the globals' +0xD8 there) and the glare luminance S+0x6A4, one row per change
  per library context S (one context in every journal so far: the keeper rows, which log any new S, hold a single S per run, up to their 400-row cap,
  reached at journal line 4907 in 32000020's run and 4450 in 32000010's),
  `"read":false` on a failed read, a non-finite luminance written null, at most 400 rows. This form has not run in the game yet:
  the rows below come from the earlier single-state form of `build-t8e-3`.
- **Where shape 3 is met** (OBSERVED, `Run-I139Cutscenes.ps1 -NoNative`, `build-t8e-3`, Shebang's eta save, the ASUS 1920x1080,
  offline, restored by hash, 22:15-22:25; the row then keyed by the shape and the luminance's sign): `i138-t8e-c32000010-orig`
  holds shape 3 on one keeper call (10934); `i138-t8e-c32000020-orig` from call 8170 to 10930, just after the scene is handed:
  32000020's own glare, its luminance falling from 2.95 (2.943, 2.93, 2.918, ... in the keeper rows). The shape-1 luminance-1 rows
  are single calls at the transitions (calls 0-3, 4909-4911, 8171, 13588-13590), the library's default state between scenes.
  INFERRED: about 2760 library frames of shape 3 (one keeper call per library frame: call 8170 at the handing, where the oracle
  runs were handed at library frames 8047 and 8322), the screenshots `cutscene-30` and `-40` still in the scene and `-50` back in
  play.
- **The original captured on shape 3** ([Run-I138T8eOracle.ps1](Run-I138T8eOracle.ps1): part 4b's oracle, `build-4b-oracle`
  with its tree `oracle-6a3b127a`, flags `yebis-hal-observe,yebis-draws-capture`, 22:39-22:47, no fault, restored by hash; the
  oracle's Run-AlphaCast has no `-GraphicsLevels`, so the PC's 1280x720): `i138-t8e-oracle-240` (handed at library frame 8322,
  captured at 8562) and `i138-t8e-oracle-900` (8047, 8947), the globals' shape word 3 in both (`reference-glare-globals.raw`
  +0xD8), 47 library draws each: Alpha_R, the depth of field's composite (T1), the source chain and the bright pass
  (`BrightPassT1_THRESHOLD_LUMINANCE_REMAP_S1` at +240, `BrightPassT1_THRESHOLD_LUMINANCE_S1` at +900), the pyramid (a `MadT1_S3`
  pair at the bright pass's size, then 6 levels: `MadT1_S7` pairs at +240, `MadT1_S5` pairs at +900, `MadT1_S4` upsamples) and its combine `MadLumGammaT7`; the streaks: `MadT3`
  (the source mix), 8 `MadT1_S4` and `MaxColorT4`; the ghost: `GlareGhost_PointSymmetryDistScaleT4_S8_TrimT1_S8` and
  `GlareGhost_PointSymmetryT3_S8`; the afterimage `MadOffsetT2`; the composite `MadT4`; the 2x `MadT1_S4` into t1; the tone map.
  Each draw's inputs, output, vertices and register globals are in the folders (`reference-draw-NN-*`, the `reference_yebis_draw`
  rows) **except the ghost's trim texture** (draw 41's slot 4, 64x64 A8_UNORM, not dumped: the oracle dumps no format-65 slot).
  Tranche 8d's reading counted 3 passes per arm; the capture holds 8 streak draws (2 per arm at this size): 8f settles it.
- **The ghost read to the end** (a ghidra-research reading, GhidraMCP and the shaders' DXBC;
  `evidence/I-0138/t8e-readings/glare-shape3-ghost.md` with its scripts): its xorshift128 is seeded from immediates at every call
  and stores nothing back (READ), so the ghosts' random scales are the same every frame; only the exposure value (glare+0x15C),
  G114, G118 and the level factors move them. The per-ghost lanes of both draws' scale and modulate tables, the `powf`
  exponents, the registers each draw writes, the vertex layout (`vs_2D_PointSymmetryTex2D8`: float2 position and uv, a 4-vertex
  strip) and the pixel programmes' sampling are written down; corrections to tranche 8d's file listed there (the pyramid's
  fourth input b+2, not b+4; row 7's 0.3333). Not yet checked against the capture: a first look at draw 41's globals shows words
  200-228 (1.48, 0.80, 0.82, 1.40, ...) where the reading computes R1 = 1.88, 0.70, 0.73, 1.72, ... with G1A8 = 1: the
  reading's tables are INFERRED until 8f matches them word for word with the captured registers.
- **Not done here** (the pilot's call, the thread's context and the night's slots): shape 3's draws written natively. Until 8f a
  shape-3 frame keeps tranche 8c's answer (no glare draw, no shaft; the depth of field and the motion blur kept), INFERRED to
  cover 32000020's whole cutscene. 8b's re-runs (the shafts without depth of field, m38) and the original's capture on a
  cutscene's shape-0 frame (tranche 8d's next step) also move to 8f.
- **Step reviews** (fresh, high): part 1 (`72c9c6a67..9f141c622`): no High; M1 the witness's new statics outside the
  thread-affinity gate (fixed: a mutex, four reviewed entries), M2 the test not drawing the pre-only plan (fixed); L1 the DoF table
  case commented, L2 the read's result and a key no read gives (fixed), L3 more coverage (the frames above). The fixes
  (`9f141c622..ba27556e3`): no High; M1 the test's last Record at a new generation rewrote every table, so a fault in
  `WriteDrawInputs` stayed invisible (fixed in `88d10fe18`: the renderer's order, the last full frame with nothing changed); L1 the
  comment, L2 the readback without `-ours` (fixed); L3 a context reborn at the same address inherits its last key (kept: a
  diagnostic).
- **Reviews at the merge** (`main...fe4f16218`): the final review (xhigh): no High; M1 « two CPostEffect contexts » refuted by
  the keeper rows (one S per run) and the shape-3 window halved (fixed: one context, about 2760 frames, INFERRED); M2 the ghost's
  trim texture not captured though the records said every input was (fixed: said, carried to 8f; the reading's line corrected);
  M3 the shape-0 capture owed by 8d dropped (fixed: carried to 8f); L1 the two captures described as one, L2 the oracle script
  leaving `witness-cutscene.txt` on an error path, L3 the witness block unguarded in a noexcept detour and a NaN written as
  `nan`, L4 the test's comment, L5 two stale entries, L6 two statements written as known (all fixed in `8bbeb288c`). The
  integrity review: no High or Medium; L1 the frame count and the falling luminance read from the counts (marked INFERRED), L2
  the per-context witness not yet run in the game (said), L3 the merged head unbuilt when it read (`build-t8e-6` since); a note:
  `build-t8e-5` ran during the oracle's runs, which measured nothing. The fixes' step review (`fe4f16218..8bbeb288c`): no High or
  Medium; L1 « one context » holds only up to the keeper rows' cap (said), L2 the pyramid's `MadT1_S3` pair (named), L3 the
  oracle script's trigger files not cleared before the first run (fixed), L4 the README and STATE on 8e's status (aligned).
- **Builds** (`-Jobs 4`, the cache `evidence/I-0138/cache-t8d`): `build-t8e-3` (`9f141c622`), `build-t8e-4` (`ba27556e3`),
  `build-t8e-5` (`88d10fe18`), `build-t8e-6` (`fe4f16218`, main merged in) and `build-t8e-7` (`8bbeb288c`, the reviews' fixes;
  a comment changed since), 178 PASS lines each, no C++ warning.

## Tranche 8f: GlareShape 3 written natively (2026-11-22, the R pilot, branch `claude/r-i138-t8f`)

ROADMAP workstream 11, item 8's 8f: shape 3's « Standard » glare (32000020's cutscene, m32_00_0100) written against tranche 8e's
captures of the original (`i138-t8e-oracle-240`, `-900`).

- **The plan** (`native/world/yebis_glare.{h,cpp}`, `BuildGlare` with `in.shape == 3`; READ from tranche 8d's and 8e's readings,
  matched to the captured registers): the bloom's combine tinted by the colour cycle over `tint_levels` levels from the base
  level b (`kw = (3 / (|r| + |g| + |b|)) x w`, then `kw x lane`, 14174cc4c..ccf7); the streaks' source mix (`MadT3`), the arms
  (`MadT1_S4` passes, 2 per arm at 32000020's size) and `MaxColorT{arms}`; the ghost's two draws
  (`GlareGhost_PointSymmetryDistScaleT4_S8_TrimT1_S8`, or its plain form when GlareGhostConcentricDistortion is 0, then
  `GlareGhost_PointSymmetryT3_S8`), 4-vertex strips, its tables from the xorshift seeded at every call, the offset signs OBSERVED
  from the captures (x negative for ghosts 0, 2, 3, 5, 6, 7, y for 1, 2, 4, 6; the value sign x param9, 0 here: a non-zero param9 is
  refused); the afterimage (`MadOffsetT2`, level 0 over the history, its decay from the frame time and the record's d8/d9, the
  offset 0xB8000100, floor 0, ceiling 64000) and the layers' composite `MadT{n}` (point sampled, weights 1). Constants that are not
  the nearest float written by their bits (0x3CA3D70B 0.02, ...). The 2x output's height `(int)(base_h x (out_w / base_w) +
  0.499999)`, INFERRED from 32000020 (314x264), m37 and m50 (both shape-1 captures that tell give the same height under the former
  formula too).
- **The bright pass's mode** (READ, `evidence/I-0138/t8f-readings/bright-pass-modes.md`, YebisGlareBrightPass_14174b830): mode 3
  becomes 2 at RemapFactor exactly 1 (the technique without REMAP, no remap word: frame 900's), and 2 becomes 0 at a threshold of 0;
  mode 3 at another RemapFactor stays 3 whatever the threshold. Mode 0 is not written: `BuildGlare` refuses it, and the frame (13b)
  serves the family empty before building, as a shape not served (its depth of field and blur kept), named « bright pass mode 0 ».
- **The trim** (`yebis::TrimCircle`, READ, `evidence/I-0138/t8f-readings/trim-circle-reading.md`): 64x64 A8_UNORM made on the
  CPU by CRenderGlare_GenerateGlare_Source_14174b2d0 (1 inside half the radius, a linear ramp to 0 at the radius); no recapture.
- **Word for word** (`yebis_glare_test`, the 8 zones with `i138-t8e-oracle-240` and `-900`): every word of the 43 glare draws'
  blocks the programmes read and the draws' techniques and targets equal the original's, bit for bit, the inputs by their sizes (10567
  checks; the samplers and the inputs' surfaces are checked on the GPU replays below).
  The object's native sources checked against the captured objects: +0x2F8 (`GlareBaseLevel`), +0x15C (`GlareRotation`, from the
  globals' DiaphragmRotate words, bit-equal at 8 of 8 zones, one ulp allowed by the test, INFERRED to hold at other fovs), +0x158
  the globals' word +4; the shape record from the executable's tables equal to the captured one.
- **The programmes** (`post_glare.hlsl`: PS_MAX, PS_MAD_OFFSET, VS_POINT_SYMMETRY, PS_GHOST with DIST, PS_GHOST_T3, PS_BRIGHT
  without REMAP): `Compare-Programmes.py --glare` on the originals' listings (`evidence/I-0138/t8f-readings/dxbc`, extracted by
  their pins, `fxc /dumpbin /Lx`): 63 programmes, 51 the same listing, 11 the same values in the same operand order, 1 commuted (the
  corona, as before), 0 different (`compare-programmes-glare.txt`). The two ghost pixel programmes are
  compiled as ps_4_0, the library's model: their listing then equals the original's, and they carry no « precise » marks; compiled as
  ps_5_0 (`/Gis`), the NVIDIA driver gave surfaces different from the library's at extents other than the captures' (log, exp, div,
  sqrt), never WARP (OBSERVED in a local run of the session, not saved: below, L4). The cbuffer re-slotted for the ghosts' 18 registers (91 slots, 1,456 bytes). 12 pins added (`yebis_pins.h`, 132
  rows; tech_MadOffsetT2 was pinned, now observed).
- **The stage** (`glare_stage`, `post_draws`): a draw's strip (PostDraws' own four vertices, the oracle's), input -7 the trim
  uploaded once, a draw's `copy_to` (the afterimage's history copied after its draw), 96 draws at most. The history is created with
  its clear value (black), cleared when a layout creates it (recorded once the layout holds) and kept across a relayout at the same
  size and format (INFERRED: the library's pair is not recreated when its techniques change); it is cleared at a camera cut, the
  velocity's reset kept until a frame records the glare (the library: E69A0 1400e6a78..a93 calls YebisClearAfterImageHistory_1416c92b0
  on its display record's cut, READ; the two cuts' equivalence INFERRED).
- **Wiring** (`13b_prepare_model_frame.inc`): shape 3 from the globals' +0xD8, its record, the streaks' and ghost's globals, tan(fov/2)
  of the frame's camera, the rotation, the base level and the frame time; the plan's cache keyed by the fov too, and no longer
  rebuilt every frame at shape 3 for the shafts. No shaft on shape 3 (its composite with a shaft layer not read; 32000020's frames
  draw none, but m32_00_0100 holds shape 3 in play with its shafts on, CENSUS-WORLD: the original's five-layer composite is not drawn
  there).
- **The replays on the GPU** (the capture's adapter and driver): `post_draws_test`, both shape-3 captures' draws 0 to 45 through
  PostDraws with the library's programmes and with ours: **46 of 46** equal the original's outputs at both frames, the largest
  difference 0 (the strip checked against the captured vertices and indices, the missing trim slot from `TrimCircle`, every captured
  sampler: the combine `MadLumGammaT7` reads eight). `glare_stage_test`, both captures through the stage: two frames in one list,
  the first laying out, the history then overwritten with the original's captured one (the afterimage draw's input), the second
  compared: **78 of 78 surfaces** equal the original's, and the history after it equal to the afterimage's output; our programmes
  against the library's on the same plans and away from the captures (four extents, two synthetic scenes) byte for byte.
- **The cutscene's own clock** (for D-0299, READ, `evidence/I-0138/t8f-readings/remo-cutscene-clock.md`): the current Remo cut's
  frame counter `float [Cut+0x68]` from SprjRemoImp `[0x144791248]`, advanced by rate x dt x speed each game frame. Not used yet.
- **The first native witness** (`Run-I138T8fWitness.ps1`, `build-t8f-3`, `i138-t8f-c32000020-nat`, 00:52-00:56, offline, restored
  by hash): the native served shape 3 (47 draws) and stopped at its first shape-3 frame: the renderer's diagnostic took the debug
  layer's warning on the history's clear (a surface created without a clear value) as a failure (0xD3126607). Fixed above;
  `glare_stage_test` on shape 3 would have caught it.
- **The second native witness** (`build-t8f-5`, `i138-t8f-c32000020-nat2`, 01:23-01:28, offline, restored by hash, exit 0, no
  fault): shape 3 served through the whole scene (« glare shape 3 » in the shafts row, 47 draws, no render failure), the 12 shots
  taken. OBSERVED, on the wall clock from the scene's request (not the same scene frames: outside D-0299, no pair compared), the
  whole capture's mean luma (`evidence/I-0138/t8f-readings/witness-luma.txt`, its command there) at 5 to 40 s: this run 79, 83, 85,
  109, 107, 130, 88; the original 107, 105, 124, 136, 138, 233, 83; the native before 8f (`i139-t3c-c32000020-nat`) 68, 72, 73, 80,
  81, 93, 89. The sheet (the native before 8f, this run, the original; 5, 15, 30, 40 s): `evidence/I-0138/t8f-readings/sheet-32000020.png`.
  The comparison waits for 8g's shots on the scene's clock.
- **Step review** (fresh, high, `e8781d71c..4035cac02`): no High. **M1** the history's clear recorded before the layout's failure
  point, which releases the new surfaces with the list open (fixed); **M2** the mode-0 refusal at every shape and RemapFactor, taking
  the depth of field and the blur with the glare (fixed, the reading written down); **L1** the history wiped at every relayout
  (fixed); **L2** the camera cut's equivalence (said INFERRED); **L3** shape 3's plan rebuilt every frame with the shafts flag
  (fixed); **L4** no shaft on shape 3 reaches play (said); **L5** two comments (fixed); **L6** the 2x height formula changes shape 1
  too (kept: both shape-1 captures that tell agree, said above); **L7** loose ends (`afterimage_skip` not read natively, said;
  `streak_arms`/`streak_passes` diagnostics, said; `GlareBaseLevel` of a zero width guarded). The fixes (`1e5a3f38a`).
  **The fixes' step review** (`4035cac02..1e5a3f38a`): no High or Medium; L1 this section stale (rewritten), L2 a header comment
  (fixed), L3 the kept-history path run by no test (kept: the relayout at the same history size needs a plan change no capture
  gives; no record shows a run reaching it), L4 the first frame's layout and every shape-3 zone's seeding not required (fixed), L5 the plan
  rebuilt every frame with the blur's Alpha_R (kept: before 8f for shape 1 too, an optimisation of its own), L6 a cut lost on a frame
  without glare draws (fixed: kept until the glare records), L7 the mode-0 comment (fixed). In `dc57087b7`, with the ghosts' shader
  model and the test's own fixes (its history readback, the pre-alone check on a plan without an afterimage).
- **Reviews at the merge** (`main...315259e9d`): the final review (xhigh): no High. **M1** a shape-3 plan the builder refuses
  (a composite wider than 8 inputs above about 4180 px, a streak count out of range) took the DoF and the blur with it (fixed in
  `5e9de3c81`: served empty, named; the streaks' count guarded against a non-finite value); **M2** the second witness's luma
  figures (measured on a 1280x720 corner of a 1920x1080 capture) and a conclusion drawn from unequal pairs (fixed: re-measured on
  the whole capture, saved with its command, no conclusion); L1 the 13b comment on +0x138 (fixed; the plan built from the native
  derivations word for word, and the rotation required exact, left to 8g); L2 the kept history and the cut's clear run by no test
  (the claim removed; the test left to 8g); L3 the test's sentence (fixed); L4 the ps_4_0 reason without a saved log (kept: the
  session's local run, `glare_stage_test hardware-ours` on build-t8f-4's objects, 809 of 844 away from the captures, not saved; the
  other ps_4_0 originals equal the library's on every run as ps_5_0), the script's docstring fixed; L5 the comparison's folder (the
  merged listings not in evidence: kept, 8g); L6 STATE ahead of the merge and `build-t8f-6` (fixed); L7 dead or duplicated code
  (kept: diagnostics and small repeats); L8 the history copy's check after the draw (kept: the plan's history is checked at the
  layout's surfaces, a failure there fails the family for the session). **The fix's step review** (`dc14bf723..5e9de3c81`): no
  High or Medium; L1 the refused plan's row lost its base, levels and luminance (fixed, `e337a30d8`), L2 and L3 two comments
  (fixed), L4 the +0x138 attribution (fixed). **The integrity review** (`main...dc14bf723`, with this section): no High or
  Medium, no test weakened, no scene-specific case, no effect cut during a measure, the figures equal their logs; L1 the last code
  commit without a passing build (answered: `build-t8f-7`, below); L2 the ps_4_0 reason without a saved log (said above); L3
  `yebis_glare_test`'s expectation for shape 3 changed from refused to drawn, shapes 2 and 4 added as refused (said here); its note
  that a few captured constants are not refused when they differ (+0xE4, +0xE8, +0xEE, +0x17C) left to 8g.
- **Builds** (`-Jobs 4`, the cache `evidence/I-0138/cache-t8d`): `build-t8f-1` failed at the link (`TrimCircle` unresolved in
  `startup_primitive_test`: made inline); `build-t8f-2` (draw 30 replayed with six samplers of its eight: the test fixed);
  `build-t8f-3` (`4035cac02`), 186 PASS lines; `build-t8f-4` (`1e5a3f38a`) failed in `glare_stage_test` (its history readback not
  copied out); `build-t8f-5` (`dc57087b7`) and `build-t8f-6` (main merged in), 186 PASS lines each, `glare_stage_test` 306 of 306 surfaces at 11 zones on the
  capture's adapter with the library's programmes and with ours, no C++ warning; `build-t8f-7` (`5e9de3c81`, the final review's M1)
  186 PASS lines; `build-t8f-8` (`67a1eb46d`, the fix's step review answered and `main` `d8de5c91c` merged in) 186 PASS lines, exit 0.
- **The merge** (2026-11-22): `1e391017a`; the launcher rebuilt from `main` (186 PASS), the start check passed (`-Commit 1e391017a`,
  restored); the disk rotation deleted nothing (no build folder older than 48 h), 475 GB free.
- **Moved to tranche 8g** (the pilot's call: the night's slots and the clock not yet wired): the comparison of the original's and
  the native's cutscene pictures at the same scene frame (a row of the Remo clock in both modes and shots scheduled on it, then both
  runs again: the shots so far are on the wall clock from the scene's request, so outside D-0299); the original's capture on a
  cutscene's shape-0 frame; 8b's re-runs (the shafts without depth of field in c33000000 to c33000040, c35000051, Gael's arena, the
  Abyss Watchers at `i139-v2-b05`'s moments, m38 after the shafts' Enable gate); the library's second reset (1416c9270 on
  `[RBX+0x4E0]`, the adaptation and the history) not modelled; the shafts' layer on shape 3.

## Suggested next step

In this order, by the R thread:

1. **Workstream 7 step 7** (I-0141), with tranche 10's leftovers (« Tranche 10 »): the cutscenes' light before the bloom in
   34010010 and 41000060 (41000060's black cut 2, the ending played after the Soul of Cinder, drawn since I-0141 tranche 20). Still open from 8g to 8i: the
   original's lit shape-3 frame in play (m32_00); if a frame ever holds a shaft record with +0x24, `t8b-corona-set-aside.diff`
   and a capture of it. **Thomas's A/B on the OLED** of the HDR highlights and the emissive glow's
   place stays open (tranche 5).
2. **The cutscenes' and the player's lighting** (ROADMAP workstream 7 step 7): its own investigation,
   [I-0141](../I-0141-cutscene-lighting/RESULT.md) (tranche 1: the cutscenes' sun when the original's shadow source is -1,
   and a cutscene's own lights kept, D-0279). From the E pilot, the same day: the Road of Sacrifices' green moss is in the native G-buffer and lost in the lighting's warm tint (I-0140 RESULT
   « The Road of Sacrifices' green moss »); in 45000030 (the Painted World's cutscene) the native drowns the first views in a
   cyan veil, the map's fog {145000000, 0} applied as a full veil from 12 m where the original's picture stays dark
   (`i139-t3c-c45000030-nat`, `evidence/I-0139/artifacts/sheets-t3d/45000030.png`), the same family as 31000010 and 40000040
   (ROADMAP workstream 7 step 7 or 12).
3. **« Elden Ring » as a vivid look** (Thomas, 2026-11-14 and 2026-11-16): on an HDR10 chain a gentle widening toward BT.2020 inside
   the Rekindled look, after workstream 13 (ROADMAP workstream 11).
4. Still open from tranche 5: a full-screen toggle made in play on an HDR10 chain; the probe's low peak read within a
   minute of Windows' HDR being switched on. From part 4b: the cutscenes' lighting rules for kind-11 files (above), with
   E's finding on the map's rotation. The cutscenes' LUT and the mapbase rule in a cutscene (a run of 31000010 and of a
   cutscene whose s-file lacks Setting Info, 33000030 or 37000020, against the original's captures).

## Tranche 8g: the cutscene on the Remo clock (2026-11-22, the R pilot, branches `claude/r-i138-t8g` then `claude/r-i138-t8g2`)

- **The clock, read again** (GhidraMCP, the pilot, the Play step FUN_1423b1370): the cuts' pointers are an array AT `[Impl+0x270]`
  (param_1[0x4e]), not inline as tranche 8f's reading wrote; the last cut `+0x378` (param_1[0x6f]); `FUN_1423cd9e0` evaluates the
  cut at the frame it stores (camera words +0x70..+0xAC), so a frame set while the speed is 0 is drawn exactly; `143D7EE10` = 1.0f.
- **Written** (`04_updates_and_test_tools.inc` ApplyWitnessCutsceneClock, test tooling, both modes, only after a
  handed `witness-cutscene.txt`): rows `witness_cutscene_cuts`, `witness_cutscene_clock` (cut, frame, our scene frame, speed, the
  globals' shape +0xD8 and luminance; read at Play_Wait since `397cb8227`, below) and `witness_cutscene_hold`; `witness-cutscene-holds.txt`
  (« ms t1 ... », a scene frame or `p<percent>`) holds the scene at each target (speed 0, the cut's frame set to the target's) for
  ms. The scene frame is tagged on `reference_glare_shape` and `native_yebis_glare` rows (`scene_cut`, `scene_frame`).
  Run-AlphaCast `-CutsceneHolds`/`-CutsceneHoldShotMs` shoots each hold from the journal's rows (`cutscene-hold-<k>.png`);
  Run-I139Cutscenes `-Holds`; `Make-I138HoldPairs.ps1` keeps only pairs held at the same cut and frame with no drift (D-0299).
  clangd 0 errors; `build-t8g-1` (`e7b0cd8e6`'s code, `-Jobs 4`, `-Cache evidence/I-0138/cache-t8d`) exit 0, 186 PASS lines, no failure (`evidence/I-0138/build-t8g-1.log`).
- **Step review of `9ea0d7bca..e7b0cd8e6`** (step-reviewer, Opus 5.5 high, 2026-11-22; as the reviewer gave it): **H1** nothing tied a
  shot to its hold (a shot after the release, the scene playing again, still paired « compared »); **M1** `CutsceneClockCell`'s 64-byte
  cell could overflow on a finite huge frame (sprintf_s ends the process) and the hold wrote without checking the frame inside the
  cut's range (a wrong cut-array reading would write into an unrelated object); **M2** a release whose later reads failed, or whose
  Ins changed while its Impl still played, left the speed at 0; **M3** the comment claimed the same moment in both modes where only the
  Remo clock is held; **L1** the holds parser took « 1000x », a bare « p », hex and a cut file; **L2** the runner's bounds differed from
  the module's, and targets left when the scene ended got no « missed » row (the runner waited 15 minutes); **L3** the cuts row cut at 64
  without saying so; **L4** NaN or inf in rows (the pair script's ConvertFrom-Json would throw); **L5** the tag on the glare rows lags
  differently by thread; **L6** the evidence note still said the cut array was inline, and the Play step's thread is not read; **L7** the
  pair script checked no scene id. **Answers** (code after the review, build below): H1 the runner records each shot's start and end on
  the module's clock (TickCount64 = GetTickCount64) in `cutscene-holds.tsv` and then drops `witness-cutscene-shot.txt`, on which the
  module releases the hold (ms is a timeout); the pair script keeps a pair only when both releases say « shot » and both shots fall
  between the hold's and the release's ticks, and gives each shot's offset from its hold (M3: the world's simulation is not held,
  stated in the comment); M1 the clock is valid only with the frame within its cut's start - 2 to end + 2, the cell 96 bytes; M2 the
  speed restored whenever the chain reaches the held Impl, else on the next call that does (a « restored » row), no new hold meanwhile;
  L1 decimal digits only, a digit after p, ms by strtol with its end checked, a full buffer refused; L2 Run-AlphaCast refuses ms outside
  500..20000, a shot delay + 3 s above ms and a percentage above 100, and targets still waiting two seconds after the scene ended get
  « missed »; L3 `truncated`; L4 non-finite values written -1; L5 recorded in the comment (only rows inside a hold name one moment);
  L6 the correction appended to `t8f-readings/remo-cutscene-clock.md`, the thread NOT READ (the drift check catches a lasting race
  only); L7 the hold rows carry the scene id and the pair script refuses differing ids.
- **The light shafts on shape 3** (`0d53c84c9`, ROADMAP workstream 11, D-0276): `AppendShafts` on a plan with a layers' composite
  (GlareKind::Layers) inserts the shaft draws before it and widens it to MadT{n+1}, the shafts' output its last layer, point sampled,
  weight 1, as YebisGlareComposite_1417511c0 does (READ, t8d-readings/glare-shape3-passes.md section 5); no MadT2 then; shape 1 unchanged.
  13b no longer refuses the shafts on shape 3 (« glare shape 3 »), rebuilds the plan every frame with the shafts on, and gives the
  shafts the record's bloom (+8, 1 for shapes 1 and 3). `yebis_shafts_test` checks the structure on every shaft capture's state made
  shape 3 (the same shaft draws as shape 1's, MadT{n+1}, its weights, the 2x draw's input): no capture of the original holds shape 3
  with shafts, so its texels are not compared (m32_00_0100 in play is the witness to take). `build-t8g-4` exit 0, 186 PASS lines
  (`yebis shafts` 2869 checks, 0 failures).
- **The shafts' step review** (`c59637c52..0d53c84c9`, step-reviewer, Opus 5.5 high): no High, no Medium; **L1** the shape-3 plan is
  rebuilt whole every frame with the shafts on (only the shafts follow the camera; heavier than shape 1's); **L2** a shafts refusal left
  Alpha_R pushed, so the blur's own Alpha_R was skipped; **L3** comments said MadT5 or MadT2 unconditionally; **L4** the test checks the
  structure only partly (the rest of the Layers block, the shaft draws' inputs and targets, no refusal case); **L5** shape 1's bloom is a
  constant, shape 3's from the table. Answers: L2 Alpha_R pushed only once the shafts are appended; L3 the comments corrected (build
  `build-t8g-6`); L1, L4, L5 left (8h's « Left » below; not fixed here).
- **The run** (`i138-t8g-c32000020-nat/-orig`, `i138-t8g-c32000010-nat/-orig`, `build-t8g-5`, the ASUS 1920x1080 window, flags
  `test-no-ai,shaft-observe`, holds `6000 p5 ... p95`, 02:52 to 03:33 Paris, every restoration ok, no fault): **no hold in any run**
  (OBSERVED: `witness_cutscene_holds` parsed, 10 targets; `witness_cutscene_cuts` never written; three `witness_cutscene_clock` rows
  in each run, every one `valid` false, `playing` false, id -1, cut -1, for the whole scene in both modes). So the Remo chain as read
  (SprjRemoImp [144791248], Man [+8], playing Man+0x154 bit 0, Ins count +0xC0, the Ins [[[Man+0xB8]]+0x10]) fails before the Ins id
  in both modes: which link is not known (the row does not name it). Nothing was compared; the shape-0 question stays open.
- **Why no clock** (the fresh R thread, branch `claude/r-i138-t8g2` from `8e91789e7`, 2026-11-22): not the chain. The journal of
  `i138-t8g-c32000020-nat` has calls 6150 and 6151 of `ObserveChrManagerSchedule` 49 s apart (ticks 149384703 and 149434093): the
  WorldChrMan update, the clock's only caller, is not run while the cutscene plays (OBSERVED; READ: 8FA3E0's gate clears its running
  flag on `[[144791248]+8]+0x154` bit 0, the comment of `01_state.inc`; GhidraMCP: 140CE2790 -> 140CD8A20 reads exactly that bit, so
  the chain's first links are as written). **Change** (`397cb8227`): the clock runs in SprjRemoIns's Play_Wait step (140CD5D80, step
  table 144A3001C), given its Ins, before the FD4 player's step (`14235D8E0(W = Ins+0xD0)` jumps to `[[W+8]]`'s slot 0x10), so a hold
  is the frame that step evaluates, on its own thread (answers L6's race); the WorldChrMan caller only closes a scene gone and stands
  aside within a second of a Play_Wait call; one lock serialises them; a one-shot `witness_cutscene_chain` row gives every link (the
  handed Ins and the Man chain's), and the clock rows say `src` and `thread`. `build-t8g-7` (`-Jobs 4`, `-Cache evidence/I-0138/cache-t8d`)
  exit 0, 186 PASS lines.
- **Its step review** (`8e91789e7..397cb8227`, step-reviewer, Opus 5.5 high; as the reviewer gave it): no High; **M1** the WorldChrMan
  caller ran the whole body when Play_Wait had not run for a second (the table, the tag, a hold start on its own thread: a `p100`
  target after the last Play step could be held on a stopped scene); **L1** the comment credited 8FA3E0 with clearing the update and
  dropped the bit-5 exception; **L2** the handed Ins trusted as is (two Ins in one frame would rebuild the table each call, its rows
  uncapped); **L3** a hold could start on a Play_Wait call that skips its Play step ([Ins+0xCC] bit 1, W null); **L4** the new statics
  not in `scripts/thread-affinity-allow.json`; **L5** a read repeated and a field unused; noted, outside the diff: restores write the
  speed without the range check (M2's choice), and `witness_cutscene_handed` is never cleared (a later scene drives the clock).
  Answers: M1 the table, the tag and a hold start only on the Play_Wait path; L1 the comments corrected; L2 Play_Wait keeps to the
  first Ins while it is seen within 0.5 s; L3 a hold starts only with bit 1 clear and W set; L4 five entries; L5 the field removed (the
  one-shot row's second read kept). `build-t8g-8` (`2757750e1`) 186 PASS lines.
- **The fixes' step review** (`397cb8227..2757750e1`, step-reviewer, Opus 5.5 high; as the reviewer gave it): no High; **M1** the five
  allow-list entries are never checked (`Test-ThreadAffinity.ps1` scans only its listed functions; it reports them STALE) and
  `latched_tick` has none; **L1** the call counter counted the ignored calls (the 15- and 30-call cadences); **L2** the first Ins met was
  kept whatever it was (a non-playing Ins could shut the scene's out, or a next scene's refresh the WorldChrMan caller's gate and leave a
  restore pending); **L3** the comments left out the holds' file on the WorldChrMan path and the `[step+1F6]` force; not verified:
  14235D810's branch on [Ins+0xCC] bit 0 before the Play step. Answers: M1 the entries removed, the two functions being outside the
  checker's scope (the render worker's and the prepare's functions; both run on the game's Remo and WorldChrMan steps, every static
  under the SRW lock or atomic); L1 counted after the filter; L2 Play_Wait keeps only an Ins of the id handed (+0xC8) and only a kept
  call refreshes the gate; L3 the comments; bit 0 also refuses a hold start. `build-t8g-9` 186 PASS lines.
- **The holds' run** (`i138-t8g2-c32000020-nat/-orig`, `i138-t8g2-c32000010-nat/-orig`, `build-t8g-7`, the ASUS 1920x1080 window,
  flags `test-no-ai,shaft-observe`, holds `6000 p5 ... p95`, shot 2.5 s into each hold, 04:05 to 04:24 Paris, every restoration ok;
  `evidence/I-0138/run-t8g2-holds.log`). OBSERVED: the clock read in both modes on Play_Wait's thread; the chain row's Ins is the Man
  chain's (`chain_ins` = `ins`), so the chain as read was right; the cuts' tables equal in both modes (32000020: 5 cuts, 1379 frames;
  32000010: 6 cuts, 1396 frames); 40 holds of 40, every release at drift 0 on the held frame. `Make-I138HoldPairs.ps1`
  (`evidence/I-0138/t8g2-pairs/`, sheets `c320000x0-hold-<k>.png`, tables `.tsv`): **17 pairs compared** at the same cut and frame
  (32000020: holds 0 to 6; 32000010: 0 to 9), each shot 2.52 to 2.70 s after its hold in both modes. Not compared: 32000020's original
  hold 7 shot in 11 s (the screenshot stalled; released by its timeout), whose late « shot » file then released hold 8 2.8 s into it (before its shot) and
  hold 9 by shot 8's file (8 and 9 refused as shots outside their holds). 32000010's hold 0 was first refused: its original shot ended
  8 ms after the release's tick, the runner's TickCount64 and the module's GetTickCount64 reading apart (the release follows the file
  written after the shot); the pair script now allows 32 ms. Mean luma of 255 (ffmpeg YAVG, whole capture; biased by the window's title bar, measured again below it in I-0141 tranche 25: 32000020 darker natively by 14 to 38 at holds 0 to 2, 4 and 5 and level at 3 and 6, 32000010 brighter at most holds (this pair's hold 6 darker by 11.2, hold 4 level), the numbers here superseded), native / original: 32000020 83.7/111.1, 82.5/101.9, 83.9/106.0,
  145.5/149.8, 105.9/148.6, 101.6/126.5, 247.8/253.6 (the native darker in every pair, by 4.3 to 42.7); 32000010 100.3/105.0, 94.5/97.7, 98.1/99.2,
  134.9/125.7, 61.0/66.5, 153.6/148.4, 142.6/159.2, 188.5/166.6, 159.0/154.4, 164.1/152.6 (either way). Which pass makes the
  difference is not read (cutscene lighting, workstream 7 step 7; I-0141).
- **The shape-0 question, answered by the clock** (OBSERVED, both scenes): natively, at each cut's start (from the cut's frame -2 to about 0, about two scene frames: 32000020 cut 1 shape 0 at
  -1.991, shape 3 at 0.052; 32000010 cut 4 still shape 0 at -0.813; the integrity review's 5), the globals read shape 0, luminance 3 (the `default` record: `native_yebis_glare` « skipped shape 0 »,
  the shafts « glare skipped », tagged on each cut's first frames), because the view context's keys are all 0xFFFFFFFF there
  (`native_yebis_sources` and `native_dof_focus` rows: every group `[4294967295,4294967295]=default`). In the original, read at the same
  place, the globals keep the scene's record across cuts 1 and on (32000020: shape 3, luminance 1; 32000010: shape 1, luminance 9.17,
  then 3 from cut 4's frame 6.6 on in both modes; no clock row changes shape there). At the scene's start they do not take it at once (the final review's M1, OBSERVED, the clock rows
  of `i138-t8g2-c32000020-orig`): cut 0 opens on the play record (shape 1, luminance 3), `reference_glare_shape` flips 3, 1, 3 at
  frame 0.25, and the luminance ramps 2.95, 2.748, 2.415, 2.082, 1.748, 1.415 (frame 69.3), 1.081, 1 at the pace of world time (it
  goes on while the Remo frame is held); 32000010's original reads 3 at frame -0.2 and 9.17 by frame 14 (ramp or late switch, the
  rows every 30 calls cannot tell). The native takes the scene's record at once (32000020: luminance 1 from frame 0.53, after its
  shape-0 frames). So 32000020's pair 0 is equal only because the original's blend ended during the hold (at frame 68.95 in play
  the original's luminance is about 1.4, the native's 1): the post records' blend is world time a hold does not freeze. READ (GhidraMCP
  14015E920): the bank's lookup maps the key (-1, -1) to the record (1, 0) as ours does, so the gap is in which keys or which gate the
  collect sees on those frames, not in the lookup; and the original's FB890 never meets the default there (OBSERVED, 32000010's
  `reference_dof_blend` rows, written at each change of FB890's DoF blend: 8 → 20 → 4.5 → 18 → 2 across the cut starts, never the
  default's 1/1 mid-scene, where the native's `native_dof_focus` passes through keys -1 and 1/1 at every cut) (FB890's gate: `[144961F70]` and the context's +0x50 bit 10; not yet compared at
  those frames). So the mid-cutscene shape-0 frames of tranches 8b to 8f are the native's, not the game's: four frames a cut without
  bloom, shafts, depth of field or motion blur. The fix is owed (the original's keys and gate on those frames read first, D-0290).
- **Shape 3's shafts in play** (OBSERVED, `i138-t8g2-c32000020-nat`): from cut 1 on, the native draws the shafts on shape 3
  (`native_yebis_shafts` shafts 1, 53 draws, with `native_yebis_glare` shape 3, 47 draws without them); in cut 0 they are « disabled »
  (the record). The held pairs of 32000020 (holds 4 to 6, sheets `evidence/I-0138/t8g2-pairs/c32000020-hold-4.png` to `-6.png`: the
  two modes with the new code only, no shaft told apart in them by eye; the shafts' gain on shape 3 has no picture yet, no still of
  the native without them at those frames, the integrity review's 1) show them beside the original at the same frame; no texel comparison (no capture of the original
  there). Cuts 1 to 4 of 32000020 take m32_00_0100's Bloom and LightShaft records (`native_yebis_sources`), the original keeps an enabled
  shaft record there (`reference_shaft_keeper` kept 1, enable 1), and inside hold 4 the native's slot colour (6.2, 6.7, 8.1) is the
  original's (6.23, 6.7, 8.12) (after its release, 6.8, 7.3, 8.9 and 6.82, 7.34, 8.89) (the final review's L5, OBSERVED): so m32_00_0100's shafts on shape 3 have the original's answer in
  hand there; a GPU capture of the original at hold 4 (cut 1, frame 120.55) gives the texels (8h).
- **The shot file keyed to its hold** (`9c89c0914`, after the holds' run): `witness-cutscene-shot.txt` holds the index of the hold its shot was taken in (Run-AlphaCast writes `$k`); only that hold's index releases it, any other is removed with a `witness_cutscene_shot_stale` row. `build-t8g-10` 186 PASS lines. **Its step review** (`2757750e1..9c89c0914`, step-reviewer, Opus 5.5 high; as the reviewer gave it): no High, no Medium; **L1** the allow-list keeps a stale entry (`NativeRtSliceWind`, outside the checker's scope); **L2** the pair script's header said the TSV holds the module's clock (it is .NET's TickCount64, another time source than GetTickCount64, whose offset is not explained); **L3** a shot file read while still empty was deleted as stale; **L4** the `[step+1F6]` force stated too strongly (a one-shot, one frame at most, 4780769 and [step+A9] still clear it after); **L5** a valid scene tag could stay set through another scene (the calls of an Ins not kept no longer clear it). Answers: L2 the header corrected and the tick test called a sanity check (the order is guaranteed by the index); L3 a file not opened or empty is left for the next call; L4 the comments in `01_state.inc` and `04`; L5 a call of an Ins not kept clears the tag once the kept Ins has not been seen for half a second; L1 left: an entry from I-0117, outside this tranche, beside other STALE lines the checker reports and passes (the checker's own clean-up). `build-t8g-11` exit 0, 186 PASS lines.
- **The fixes' step review** (`9c89c0914..07da0fa6f`, step-reviewer, Opus 5.5 high; as the reviewer gave it): no High, no Medium; **L1** the pair script's header stated two time sources and an 8 ms offset without evidence (.NET's TickCount64 thought a call of GetTickCount64); **L2** « one frame at most » for [step+1F6] holds per write only (no setter of the value 1 read); **L3** (older than the diff) a hold whose Ins is gone while an Ins of another id plays is closed only when that scene ends (only kept calls release). Answers: L1 measured (OBSERVED, pwsh 7.7 / .NET 11 on this PC: TickCount64 equals QueryUnbiasedInterruptTime to the ms, GetTickCount64 4 to 14 ms behind it, its 15.6 ms step), written in the header, with the offset's growth after a sleep (every pair refused, loudly); L2 « the frame it is set in, its setters not read »; L3 left: such a hold is released « scene changed » and the pair script does not compare it (test tooling, no two scenes at once met in the runs).
- **Where the keys come from** (READ, GhidraMCP, the pilot, 2026-11-22; a lead for the cut starts, not a cause): the context's keys (+0x3FD8/+0x4040/+0x40A8) are written only by 1400884F0 (one key, weight 0) and 140088530 (A, B, weight), both through `SetPerGroupSlot_140d25270`, which `AreaControllerSet_PerFrameApply_140d52b70` calls for groups 0 to 9 when the set's +0x194/+0x190 equal [1446402E4]/[1446402E0], else one key for every group (140D253F0 -> 140088AF0 or 140088C30, the set's +0x188 entry); its caller `StageRend_PerFrameUpdate_140e006a0` (from 140DFDC50) takes the set at +0x508, else +0x500, else its own +0x348, then clears +0x508 and +0x500, and resets group 2 to (0, -1) when +0x510 is set. FB890 reads the same words of its param_5 (lines 306 to 421). So a context whose keys are all -1 at a cut's start would give the original the default too: either the original's FB890 sees other keys there (another context, or the same one at another moment of the frame than our collect), or it is not called on those frames. Which one is the next tranche's first reading: a row of FB890's param_5 keys and gate in the original and of our collect's in the native, both tagged on the Remo clock, at 32000010's cut starts.
- **8b's re-runs** (native only, `build-t8g-10`, the ASUS, 04:56 to 05:48 Paris (the cutscenes in a 1920x1080 window, the three
  arenas in the runner's default 1296x759 window, the draw counts depending on the extent), `evidence/I-0138/run-t8g2-8b.log`;
  `i138-t8g2-8b-c33000000-nat` to `-c33000040-nat` and `-c35000051-nat` with `test-no-ai,shaft-observe`, `-b05-watchers-nat`,
  `-b26-gael-nat` and `-b08-wolnir-nat` (m38) with `world-pictures,rt-scene,rt-fail-magenta,shaft-observe`): 9 runs of 9, exit 0, no
  fault, every restoration ok. OBSERVED: in every run the depth of field never draws (`native_yebis_dof` quality 0, « no level
  drawn » or « depth of field off », no row « on »: the game's DoF quality 0). The shafts (the final review's M2): c33000000 to
  c33000040 hold them « disabled » or « glare skipped: shape 0 » for the whole scene (the record); they are served (refused « »,
  alpha 1, shafts 1, 29 to 39 draws) in c35000051 and Gael's arena, and elsewhere only before the travel (the same place in every run);
  the Abyss Watchers' fight and m38 « disabled ». No shafts row is refused « depth of field off »: that refusal stays in the code
  (`06_pass_programmes.inc`, the Dof record's Enable at 0, tranche 8d's choice) and its condition was not met in these runs, the DoF
  quality 0 being another condition (the commit message of `354181652` saying « no refusal left » is wrong). « depth of field off »
  and « glare skipped: shape 0 » come together on the frames with the default record (in the arenas too: before an area key is
  known). So the shafts are drawn natively where the DoF draws nothing (quality 0, c35000051 and Gael); the record's Enable at 0 with
  the shafts on is still not met; against the original at those moments they stay uncompared (part 4b's reading above).
- **Left for the next tranche** (8h, in order): the default record at each cut's start, the native's own (above): FB890's param_5
  keys and gate in the original and our collect's in the native, both tagged on the Remo clock at 32000010's cut starts, then the
  fix (D-0290); shape 3 with its shafts against the original's capture (m32_00_0100 in play: its five slot-1 collisions' places
  first, not recovered; or a cutscene of shape 3 with the shafts on); the native darker than the original in every pair of
  32000020 (whole-capture lumas; below the title bar, I-0141 t25: at holds 0 to 2, 4 and 5, level at 3 and 6) (the cutscenes' lighting, workstream 7 step 7, I-0141); the scene-start record blend (the original's luminance ramping
  over about 4.5 s where the native takes the record at once, above), read with the same FB890 keys and weight rows; the merged
  hold code (`9c89c0914`, `07da0fa6f`: the release keyed to the hold's index, the empty shot file left) not yet run live with holds
  (the t8g2 pairs come from `build-t8g-7`, released on any shot file, so they rest on the tick test): the first holds run of 8h
  witnesses it, the release row then naming the file's index for the pair script; from the reviews, left: a hold whose Ins is gone
  while another scene plays closes only when that scene ends; the shape-3 plan rebuilt every frame with the shafts on (cache the
  plan without shafts and append them to a copy); the shafts test's refusal case and the shaft draws' inputs and targets (the
  shafts review's L4), shape 1's bloom a constant (L5); a hold sets the cut's frame up to 2 frames behind the one evaluated (events
  in between fire again) and its saved speed overwrites a change the game makes during the hold; the one-shot chain row and
  `ReadRemoChain`, settled, to drop.

### Tranche 8g: the optimisation pass and the reviews at the merge

- **The optimisation pass** (the branch's additions read, `main...HEAD`): nothing removed. The Remo clock runs only after a handed
  `witness-cutscene.txt` (test tooling, both modes; one early return otherwise); the shafts on shape 3 reuse `AppendShafts` and the
  plan's per-frame rebuild that shape 1 already takes with the shafts on, repeated work left to 8h (the shafts' review's L1, the
  final review's L4; not measured, D-0227); the pair script reads each journal once.
- **The final review** (`main...47c0a3d59`, main `474b05e7f`, final-reviewer, Opus 5.5 xhigh; as the reviewer gave it): no High;
  **M1** the bullet « the globals keep the scene's record across every cut start » is wrong at cut 0 (the original blends from the play
  record over about 4.5 s of world time, the native takes the scene's record at once; pair 0 equal only because the blend ended
  during the hold); **M2** the 8b claims overstated (c33 holds the shafts « disabled »; only c35000051 and Gael serve them; the
  refusal on the Dof record's Enable never reached, DoF quality 0 being another condition; commit `354181652`'s « no refusal left »
  false); **M3** the merged hold code never run live with holds (the pairs from `build-t8g-7`), and the pair script's header claims a
  check (the release on the file naming its hold) the release row cannot show; **L1** « by 2 to 43 » (4.3 to 42.7) and « released hold
  8 at once » (2.8 s in); **L2** stale: the clock rows « read at the schedule call », the scene frame « INFERRED as the game's own »
  (each cut starts at -2), Run-AlphaCast's « GetTickCount64's counter », the reading note's « not yet witnessed », « thread NOT READ »;
  **L3** the shafts review's L4 and L5 without an owner; **L4** the shape-3 plan rebuilt every frame, the settled chain row, the
  optimisation pass overstated; **L5** m32_00_0100's shafts already witnessed in 32000020's cuts 1 to 4 (its records, the original's
  enabled shaft record, the slot colour equal at hold 4); **L6** ROADMAP's open line stale, the final build not recorded, no
  before/after sheet named for shape 3's shafts; **L7** a hold sets the frame up to 2 frames back and its saved speed overwrites a change.
  Answers: M1 the bullet corrected with the ramp, the scene-start blend and the post records' blend as world time added to 8h; M2 the
  8b bullet rewritten as observed, the commit message's error recorded here; M3 recorded (the pairs rest on the tick test, the merged
  hold path witnessed by 8h's first holds run with the file's index in the release row), the header corrected; L1, L2 corrected
  (the RESULT, the comments of `04` and Run-AlphaCast, the reading note); L3, L4, L7 in 8h's « Left »; L5 written in the shafts' bullet
  and 8h; L6 ROADMAP's line, the sheets `c32000020-hold-4.png` to `-6.png` named (no before/after, the integrity review's 1);
  the final tree's build below.
- **The final tree's build**: `build-t8g-13` (`57503f020`'s code, main `474b05e7f`'s and E's tranche 39 merged in; main's later
  commit is STATE alone) exit 0, 186 lines starting « PASS » (`yebis shafts` 2869 checks, 0 failures; `evidence/I-0138/build-t8g-13.log`).
- **The integrity review** (`main...57503f020`, integrity-reviewer; as the reviewer gave it): no High; **Medium 1** the sheets of holds 4
  to 6 called a before/after of shape 3's shafts where both rows carry them (no still of the native without them); **Low 2** the 32 ms
  tolerance admits exactly one pair (32000010's original hold 0, +8 ms; the other 33 between -28 and 0 ms), its clock measurement has
  no file, and « refuses every pair after a sleep » holds only when the offset exceeds the shot delay (a smaller one loosens the end
  test silently); **Low 3** « the final tree's build below » pointed at nothing; **Low 4** 32000010's luminance 9.17 turns 3 in cut 4 in
  both modes; **Low 5** the cut starts' shape 0 lasts to about frame 0, not -1.6; **Low 6** the slot colours cited after hold 4's
  release; **Low 7** the arenas of 8b ran at 1296x759, not 1920x1080. Answers: all seven in the records (the sheets' bullet, the
  shape-0 and 32000010 bullets, the slot colours inside the hold, 8b's windows, this build line); the clock measurement written to
  `evidence/I-0138/t8g2-clock-sources.txt` and the header's condition corrected.
- **After the merge** (`6f174a26d`): the launcher rebuilt from main (exit 0, 186 lines starting « PASS »), the start check passed
  (06:39 to 06:40 Paris, ready in 2750 ms, restored); `Clear-DiskCache -Apply`: 0 GB deleted, 470 GB free on C:.

## Tranche 8h: the cut starts' default record, the original's draw gate (2026-11-22, the R pilot, branch `claude/r-i138-t8h`)

- **The rows** (`efdacbe0f`, test tooling behind `shaft-observe.flag`, both modes; `07_scene_lighting.inc` LogPostKeys and its
  reader `Read-PostKeys.py`): the view context's post keys (groups 0 to 9: A +0x3FD8, B +0x4040, weight +0x40A8), FB890's gate
  (the context's +0x50 bit 10, [144961F70]) and the Remo clock's tag, at three sites: « apply » after
  `AreaControllerSet_PerFrameApply_140d52b70` (hooked in both modes; the set's tokens +0x190/+0x194 against [1446402E0]/[1446402E4]
  and its single key +0x188), « fb890 » at the original's FB890 entry (param_5 the context, the signature's eight arguments read in
  the disassembly) and « collect » at our CollectNativeYebisInputs. A first run (07:00 to 07:06 Paris) ended both modes with
  0xC0000409: the set's cell (about 153 characters) overflowed `char cell[96]` and `sprintf_s` ends the process; the cell made 256
  (`build-t8h-4`), the window set to 1920x1080 as in 8g.
- **The run** (`build-t8h-4`, the ASUS, 07:09 to 07:18 Paris, `test-no-ai,shaft-observe`, holds `6000 p5 ... p95`,
  `evidence/I-0138/run-t8h2-keys.log`; journals `i138-t8h2-c32000010-nat` and `-orig`): both runs exit 0, restored; 20 holds of 20
  released by their shot, so the hold code merged in 8g (`9c89c0914`, `07da0fa6f`) is now witnessed live.
- **What the rows show** (OBSERVED, every cut start of 32000010 in both modes; six cuts, keys 232000010:10 to :70, group 0 at
  132000000:0): on a cut's first steps D52B70 writes keys -1 in both modes (the same data: the set's tokens equal the expected ones,
  the per-group path is taken and the set holds -1). The native collects and draws each of them (cut 1: scene frames 358.08, 358.58,
  359.08, about 16 ms apart, then the cut's keys from 359.57). The original never calls FB890 on them: its « apply » steps come in a
  burst (cut 1: frames 358.00 to 358.79 within about one 15.6 ms tick of GetTickCount64, two values; the whole run about 62 ms), its first FB890 of the cut at frame 360.13 with
  the cut's keys; the count of « apply » calls without an FB890 grows by 15 to 22 at each cut start (8189 against 8188 at cut 0,
  then 9503/9487, 10111/10077, 10639/10583, 11090/11018, 12238/12148). One context in the original (gate [1, 1] throughout). So the
  keys are -1 in the original too; the original simply does not draw those steps.
- **The gate** (READ, GhidraMCP, the pilot's reading checked against a `ghidra-research` sweep): D55190 (the render step the native
  replaces with NativeRenderStep) runs RendManImp_FrameUpdate DFDD20 and DFDD00 every step but draws (D6C760, D6C750,
  RendManImp_Draw DFD270, D5C1F0, 142357040, DFD7F0) only when `FUN_140d56da0([144793348])` is true: the root's +0xCA clear and
  `FUN_140eed5b0(timing)` (the timing object [14489DD10]'s +0x27B) clear. +0x27B is set by `FUN_140eee230`, whose only caller is
  `SprjRemoIns_PlayWait_140cd5d80` (call at 140CD60A0), on the branch where the current cut's frame is past its end
  (`FUN_14235d760`, as read by the sweep); the sweep's byte scan found its only clear at the limiter's exit (140EEE098). +0xCA is
  written once a step by `FUN_140d56d20` (reached by a jump from the undefined code at 140D55170): set when `FUN_140eed7f0(timing)`
  (+0x27B, else the limiter's catch-up +0x278) is or another writer set +0xC8, unless +0xC9 forces the draw; +0xC8 and +0xC9 cleared
  after. A step without a draw records no present: the original's present is the GX render thread's callback after the slot's replay
  (D3D12-R13-R14-RENDER-THREAD-PACING-SHEET), and its post waits for the previous job's present and pacing, so the burst above
  (several steps a tick) is a run of steps without a present (INFERRED from the timing and the sheet). With +0x27B the limiter also
  skips its wait (CADENCE-STUDY A0).
- **The fix** (D-0290: the original's answer for the same input read above, and witnessed below; `f63eb4e80`, then the step review's
  answers): natively the same gate, read where the original reads it (`14_scene_step_and_storage.inc` ReadNativeDrawGate: +0xCA,
  then +0x27B; without the root or the timing object, at start-up, it is not read and the frame is drawn as before; the original's
  D55190 draws nothing without the root and asserts without the timing object). From `e95ee2cfa` to `ae3f729bc` the module also computed +0xCA, calling
  the original's FUN_140d56e00 once a step, because natively its caller never runs (SprjDrawStep's slot 0x80, 140D55170, the vtable
  at 142957780, calls FUN_140d56d20 only while the root's +0x98 is set, and +0x98 is set at the end of the root's GX set-up D573B0,
  `decompiled/DS3-1152-F-00D573B0.c`, not run natively). The final review showed that +0xCA also closes every step of the
  limiter's catch-up (+0x278), which natively nothing could cancel (+0xC9's only writer FUN_140d56d90 is called from the +0x60
  list's code alone, not run natively): the final form (`b90579b08`) reads the original's gate without computing +0xCA, which stays
  0 natively (the root's constructor FUN_140d55db0 writes +0xC8 to +0xCA as 0, 140D55E6F), so the native gate closes on +0x27B
  alone (« Reviews at the merge » below). The other readers of FUN_140d56da0, the cloth vertex update (`ClothVertexUpdate_*`,
  140DD8C40 to 140DD8F70), read the same values as before the tranche. A closed step: UpdateNativeSceneCpu
  posts no UI display and runs no collect (no selection, no menu views; D52B70 and the rest of the scene update still run);
  NativeRenderStep follows its collect's reading when a collect ran (the frame drawn is the one collected, so the published scene
  and the camera's history move only with drawn frames; the models' poses and the cloth frames still step), counts its own reading when it differs, prepares no UI (the options,
  colour reads and our overlay's step stay); EndFrame runs no Clear (no render, no present, no pacing of the worker), no Reflex
  sleep, no display-clock wait and no PCL marker; the screen keeps the last drawn frame. That the original presents nothing on
  such a step is INFERRED (its frame end D56930 still posts a slot to its render thread every step, FUN_14000a7b0, whose post waits
  for the previous job; its steps then come several to a 15.6 ms tick): natively a closed step waits for nothing. No case for the
  keys -1: the steps are the original's own undrawn steps. Rows: `native_draw_gate_skipped` at a closed run's start, its end and
  every 600th step of a run still closed (the first 64 runs, then every 100th), its steps by cause (+0x27B, +0xCA, +0x278) and the
  render step's readings that differed from the collect's (`native_draw_gate_disagreed` outside a run); the post_keys rows carry
  the gate's three bytes (`draw_gate`: +0xCA, +0x27B, +0x278) in both modes.
- **The witness of `f63eb4e80`** (`build-t8h-5`, the ASUS, 07:39 to 07:48 Paris, the same command as above with prefix
  `i138-t8h3`, `evidence/I-0138/run-t8h3-gate.log`): both runs exit 0, restored, 10 holds of 10 each. OBSERVED, native: six closed
  runs, one at each cut start (6, 12, 15, 13, 15 and 21 steps, against 15, 18, 22, 15, 18 and 21 in `i138-t8h3-c32000010-orig`:
  the +0x27B steps a cut differ, the game's catch-up between runs, open; all on +0x27B, +0x278 set on all but the first step of each), none
  elsewhere in the run (load, play before the scene, the scene's end), no disagreement between the collect's and the render step's
  readings; the collect rows with the keys at -1 on the scene's clock went from 23 (the cut starts, two at the scene's start and one
  after its end, `i138-t8h2-c32000010-nat`) to one, after the scene's end at frame 1383.41, where the original's FB890 meets the
  default too (frame 1383.20 of `i138-t8h2-c32000010-orig`); 11 more before the scene's clock (12 before the fix); the glare
  rows on the scene clock went from shape 0 at each of the six cut starts to shape 1 throughout, shape 0 only after the scene's end. OBSERVED, both modes' `draw_gate` cells: the original's
  +0xCA is 1 on every undrawn step but the first of each run ([0, 1, 0], 6 rows; 48 rows [1, 1, 1], the logger's window of 8
  calls after each cut change, not every step) and on one more step after each run ([1, 0, 1]: +0x27B cleared, the catch-up
  +0x278 still set, its FB890 first called on the step after, cut 1 at frame 360.01); the native's +0xCA was 0 in every row (the
  cause above), so `build-t8h-5` drew that trailing step (cut 1 at frame 359.88), one step before the original, with the cut's own
  keys. The form that computes +0xCA is witnessed below; the final form's decision is this run's (+0x27B alone).
- **The witness of `e95ee2cfa`** (`build-t8h-6`, the ASUS, 07:58 to 08:02 Paris, native only, prefix `i138-t8h4`,
  `evidence/I-0138/run-t8h4-gate.log`): exit 0, restored, 10 holds of 10. OBSERVED: six closed runs, one at each cut start (11, 15,
  21, 14, 17 and 23 steps, against 15, 18, 22, 15, 18 and 21 in `i138-t8h3-c32000010-orig` by its « apply » counters: the
  sequence of values and the resume within a step match, not the lengths), each ending with the step where +0x27B is cleared and +0x278 still set (+0xCA 1 on every step of the
  run, +0x27B on all but that last one), none elsewhere, no disagreement; the `draw_gate` cells at D52B70 now take the original's
  values (48 rows [1, 1, 1], the logger's 8 window calls after each of the 6 cut changes, and 6 rows [1, 0, 1] in both `i138-t8h3-c32000010-orig` and this run; the runs' first step reads
  [1, 1, 0] natively and [0, 1, 0] in the original: the module computes +0xCA after Play_Wait, the original before it, and +0x27B
  closes that step in both). Cut 1's first collect is at scene frame 360.13, after the trailing step (frame 359.96), as the
  original's first FB890 follows its own (frames 359.88 and 360.01 in its run: the steps' frames differ between runs, their
  sequence does not). No still or film before and after: the steps concerned last about 50 ms a cut natively before the fix,
  under one 15.6 ms tick in the original, and the runner's holds stop the scene elsewhere; the rows above are the comparison.
- **The step review of `f63eb4e80`** (`efdacbe0f..f63eb4e80`, step-reviewer, Opus 5.5 high; as the reviewer gave it): no High;
  **M1** a collect that reads the gate open on a step the render step reads closed has moved the velocity history, published the
  scene, drained the paint queue and taken the UI movies' flags for an undrawn frame (a cut's history without a reset, DLSS or the
  TAA ghosting the old shot); **M2** the collect may read a +0xCA one step old, so the two readings could disagree at every run's
  ends; **M3** the comments say the original presents only on drawn steps, which its frame end (a slot posted every step) does not
  show; **M4** a gate that stays closed freezes the screen with no row (rows only at a run's end), +0xC8's and +0x278's writers not
  named, the fix not run; **L1** the collect's reading can leak into a later step (the render step's early return); **L2** the
  gate not read in the original's order (+0xCA before the timing object) and +0x278 required to read it; **L3** our overlay's step
  (the title music and sounds) skipped on closed steps; **L4** « +0xC9 forces the draw » (it only cancels +0xCA) and « the original
  would draw nothing » (without the timing object it asserts); **L5** the camera still published on closed steps. Answers: M1 and M2
  the render step follows its collect's reading when a collect ran (one decision a step; its own reading counted when it differs,
  none in both runs); and the run showed +0xCA never written natively (the cause above), now computed once a step by the module
  before the collect; M3 the claim INFERRED in the code and above, with the wait the original keeps; M4 rows at a run's start and
  every 600th closed step, the two runs above; L1 the readings cleared by EndFrame; L2 +0xCA then +0x27B, +0x278 for the row only;
  L3 the overlay's step outside the UI block; L4 the comments corrected; L5 left: with one decision a step, a closed step draws
  nothing that the camera could disagree with.
- **The step review of `e95ee2cfa`** (`f63eb4e80..e95ee2cfa`, step-reviewer, Opus 5.5 high; as the reviewer gave it): no High;
  **M1** with the render step disabled (the service's +0xC8 clear) the collect's closed reading is lost and EndFrame draws and
  presents the last published scene on an undrawn step, and a step with neither a collect nor a render step computes no +0xCA;
  **M2** the +0x60 list attributed to D573B0 while it is FUN_140d56d20's own first half (the list is constructed natively), so
  +0xC9 never comes from it natively, a draw divergence; **M3** the RESULT claimed a witness of the computed +0xCA before any run of
  it, and the code's « OBSERVED » was an inference from the original's rows; **L1** the code identity checked on 16 bytes only
  (not the writes, not FUN_140eed7f0); **L2** the start row written before its step's counts, and the causes counted from the
  render step's reading while the closure follows the collect's; **L3** the stop path's early return skips the step's reset (+0xCA
  then frozen if steps go on after Close, no end row); **L4** the cloth readers' « same steps » claim unwitnessed; **L5** the pacing
  claim (the limiter skips its wait on +0x27B) does not cover steps closed by +0xCA alone. Answers (`57ffaf90b`): M1 the collect
  records its decision for EndFrame, and EndFrame computes +0xCA on a step that read the gate nowhere; M2 the comments and the fix
  above corrected, the list left open under D-0290; M3 the witness of `e95ee2cfa` above (i138-t8h4) and the comment citing it; L1
  FUN_140d56e00 and FUN_140eed7f0 checked byte for byte to their returns (the arrays matched against the installed
  DarkSoulsIII.exe); L2 the start row after its step's counts; the causes stay the render step's reading (a difference is counted,
  none in both runs); L3 the reset moved before the early return; L4 and L5 marked INFERRED.
- **The step review of `57ffaf90b`** (`e95ee2cfa..57ffaf90b`, step-reviewer, Opus 5.5 high; as the reviewer gave it): no High, no
  Medium; **L1** the byte check stops short of the code the call runs (FUN_140d56e00's epilogue 140D56E6E..E73, FUN_140eed7f0's
  `C3` after its `F3`; the assert path 141777DE0 unchecked, unreachable in practice) and the comment's « writes nothing but those
  three bytes » is not what the check shows; **L2** a step closed by its collect alone (the render step returning before its
  reading) appears in no row; **L3** EndFrame runs the game's code before its stop and owner checks; **L4** comments: « undrawn
  (NativeRenderStep) » where the collect can decide, « freezes the screen as the original's would » unmarked, « once, before the
  collect » false on title, menu and loading steps, the post's wait and the limiter's skip unsourced; **L5** a step whose root
  goes null between the collect and the frame end carries its gate state into the next step; **L6** `57ffaf90b` has no run, and
  natively +0xCA is computed from the first step with a root and a timing object where the original waits for D573B0's +0x98 (the
  root's constructor sets +0xC8). Answers (`6d2ac9604`): L1 the arrays to the returns (116 and 20 bytes, matched against the
  installed DarkSoulsIII.exe), the assert path's unreachability stated (the timing pointer read non-null just before, same
  thread); L2 EndFrame counts such a step from the collect's reading; L3 the computation after both checks, the resets before the
  return; L4 corrected or marked (CADENCE-STUDY A0, the pacing sheet, INFERRED); L5 left (the root's teardown, not seen during a
  session); L6 the start check after the merge runs the identity check; the start-up order left open (below).
- **The step review of `6d2ac9604`** (`57ffaf90b..6d2ac9604`, step-reviewer, Opus 5.5 high; as the reviewer gave it): no High, no
  Medium; **L1** a step that read the gate nowhere (the render step disabled, no collect) is drawn by EndFrame whatever its
  computed gate, and does not end a closed run, so two closed spans around it come out as one run; **L2** comments: « on title,
  menu and loading steps » not tied to the code, the root constructor's +0xC8 unsourced, « as the original's step does every step »
  against +0x98, « waits for nothing » while DS3's limiter still runs; **L3** the assert path's unreachability rests on the caller.
  Answers (`f3f0f6cbe`): L1 that step ends the run (its own reading noted open); drawing it stays as before the tranche (with the
  render step disabled the original's D55190 would draw nothing: left open, whether such steps occur once the root exists not checked); L2 corrected (the
  constructor's write cited from the bytes at 140DEEEFC, wrongly: corrected at the final review); L3 the timing object re-read in
  ComputeNativeDrawSkip before the call.
- **The step review of `f3f0f6cbe`** (`6d2ac9604..f3f0f6cbe`, step-reviewer, Opus 5.5 high; as the reviewer gave it): no High, no
  Medium; **L1** « on the engine thread » not enforced (the collect has no owner check; FUN_140d56e00 re-reads the timing global
  itself, so the guard holds only if no other thread writes it); **L2** « that first step is undrawn natively » inferred (the
  constructor clears only bit 0 of +0xC9, the other writers of +0xC9 not shown unrun); **L3** a step that read the gate nowhere
  ends a closed run as open whatever its own reading, so one span can come out as two runs. Answers (comments only, built with the
  tests at `f3f0f6cbe`, `build-t8h-9`): L1 and L2 marked INFERRED in the code; L3 left (the rows then over-count runs, never steps).
- **Left for the next tranche** (8i, in order): shape 3 with its shafts against the original's capture (as 8g left it:
  m32_00_0100 in play, its five slot-1 collisions' places first, or a cutscene of shape 3 with the shafts on); +0xCA's closures not
  honoured natively (open under D-0290): the step after each +0x27B run drawn one step before the original's, and the limiter's
  catch-up closures, which need the +0x60 list (FUN_140d6e5b0's state +0x18, +0x5C) read first and a witness in play out of focus
  and GPU-bound; the native darker than the
  original in every pair of 32000020 (whole-capture lumas; below the title bar, I-0141 t25: at holds 0 to 2, 4 and 5, level at 3 and 6) (workstream 7 step 7, I-0141); the scene-start record blend (the original's luminance ramping
  over about 4.5 s): in the 126 FB890 rows of `i138-t8h2-c32000010-orig`, groups 0 and 1 have A equal to B and a weight of 0, so in
  those rows the ramp is not a blend between two keys (OBSERVED), its source open; from this tranche's reviews: the camera still
  published on closed steps, the causes counted from the render step's reading; from 8g's reviews, unchanged: a hold whose Ins is
  gone while another scene plays, the shape-3 plan rebuilt every frame with the shafts on, the shafts test's refusal case and
  inputs (L4), shape 1's bloom a constant (L5), a hold's frame up to 2 frames behind and its saved speed, the one-shot chain row
  and `ReadRemoChain` to drop. The hold code merged in 8g is witnessed live (the run above). Also left: a step with the
  render step disabled drawn by EndFrame (the fourth review's L1), a root nulled between the collect and the frame end (the third
  review's L5), the final form's rows in a cutscene (its decision is `f63eb4e80`'s, witnessed by `i138-t8h3`), the models' poses
  and cloth frames stepping on closed steps (the final review's L2: harmless at cut starts, where the camera cut resets the history).

### Tranche 8h: the reviews at the merge

- **The final review** (`main...ae3f729bc`, final-reviewer, Opus 5.5 xhigh; as the reviewer gave it): no High; **M1** the gate
  closes steps on DS3's catch-up flag everywhere in play (FUN_140eed7f0: +0x27B, else +0x278; +0x278 set when frames run over
  budget, the window +0x270 in focus and +0x274 out of focus), and natively nothing can cancel it (FUN_140d56d90, +0xC9's only
  writer, has three callers, all in the +0x60 list's code): out of focus or after a hitch the native's following steps go undrawn;
  one 60 Hz cutscene run is the only evidence; **M2** « the root's constructor sets +0xC8 to 1 (140DEEEFC) » is wrong (that write
  is FUN_140deedd0's, the object of 1447957B8; the root's FUN_140d55db0 writes +0xC8 to +0xCA as 0 at 140D55E6F); **L1** 07's
  header: D52B70 not the keys' only writer (FUN_1400884f0 writes group 2 after it), « a cut's first two frames » where the native
  had three or four; **L2** « the velocity history moves only with drawn frames » holds for the camera only (poses and cloth frames
  step); **L3** the cloth readers could read a stale +0xCA; **L4** the 48 [1, 1, 1] rows are the logger's window, the run lengths
  differ (15, 18, 22, 15, 18, 21 against 11, 15, 21, 14, 17, 23), STATE's « its closed steps the native's » overstates, « 23
  (every cut start) » includes the scene's ends; **L5** the live witness is of `e95ee2cfa`, not the final tree; **L6** STATE
  contradicts itself before the merge. Answers (`b90579b08`): M1 the native gate narrowed to +0x27B, the module no longer computes
  +0xCA (natively 0, as before the tranche), the catch-up closures left open (Left above); M2 the constructor cited right in the
  code and above; L1 07's header corrected; L2 « the camera's history », the poses and cloth frames named (left); L3 gone with +0xCA
  no longer computed; L4 the lengths reported and what matches stated, STATE rewritten; L5 the final decision is `f63eb4e80`'s
  (+0x27B alone), witnessed by `i138-t8h3`, the collect-only paths not run live (left); L6 STATE written for the merge.
- **The step review of `b90579b08`** (`ae3f729bc..b90579b08`, step-reviewer, Opus 5.5 high; as the reviewer gave it): no High, no
  Medium; **L1** « +0xC9's only writer » (D56D90 is its only setter; D56E00 and the constructor clear it); **L2** the constructor's
  +0x98 write starts at 140D55E24, not E23; **L3** the catch-up window's focus claim unsourced (140EEDA86 picks +0x270 or +0x274;
  +0x278 forced to 1 while +0x27B is set, 140EEDD62); **L4** « the camera's history » still generic: the game camera's previous
  sample steps on closed steps, `native_velocity_history` does not; **L5** FUN_1400884f0 has a second caller (140D25295); **L6** the
  cloth frames step on closed steps, so a +0x27B run without a camera cut could ghost the cloth for one frame; **L7** the exact gate
  code of `b90579b08` has never run (the collect-only and read-nowhere paths unwitnessed). Answers (comments only, the next commit):
  L1 to L5 corrected (the focus mapping INFERRED); L6 and L7 left (Left above).
- **The integrity review** (`main...876d53da1`, integrity-reviewer, with this section; as the reviewer gave it): no High, no
  Medium; nothing makes the result look better than it is; **L1** the merged tree has no build or run cited (STATE's « OBSERVED,
  `i138-t8h3` » does not say that witness predates the merged code); **L2** « +0xCA is 1 on its undrawn steps (48 rows) »
  overstates (the first undrawn step of each run reads [0, 1, 0]; the 48 rows are the logger's window); **L3** « within one 15.6 ms
  tick » spans two tick values (the run about 62 ms); **L4** the default record's removal is shown by rows inside the path the fix
  skips: the decision ran, the screen is not witnessed; **L5** the run lengths not set side by side, their gap unexplained.
  Answers: L1 the final tree built with its tests in `build-t8h-10` (`876d53da1`'s code, exit 0, 186 suites PASS, none
  failed) and not run live, STATE saying so; L2, L3 and L5
  corrected above (the gap open: the game's catch-up between runs, not the gate); L4 STATE says « undrawn per the gate rows, the
  screen not witnessed ».
- **The merge** (`de681ce97`, 2026-11-22 09:14 Paris): the launcher's module rebuilt from `main` (491 commands, 186 suites PASS, none
  failed), the start check passed (09:22, restored, no refusal); the disk rotation (`Clear-DiskCache.ps1 -Apply`) deleted nothing
  (465.7 GB free).

## Tranche 8i: shape 3's shafts against the original, the draw gate's +0xC9 (2026-11-22, the R pilot, branch `claude/r-i138-t8i`)

- **The capture** (`Run-I138T8eOracle.ps1 -Delays 1250,1950 -Prefix i138-t8i-oracle-shafts`, the oracle `build-4b-oracle`, the ASUS,
  09:49 to 09:57 Paris, offline, both saves restored by hash, no measure; `evidence/I-0138/t8i-oracle.log`; a first launch at 09:48
  refused its delays, `pwsh -File` joining `1250,1950` into one number): `i138-t8i-oracle-shafts-1250` (handed at library frame
  8015, captured at 9265, 32000020's cut 1 by the cuts' table of `i138-t8g2-c32000020-orig`, INFERRED from about two library frames
  a Remo frame): 57 draws, the globals' shape 3: Alpha_R, the DoF's, the sources, the bright pass, the pyramid and
  `MadLumGammaT7`, `MadT3`, four arms of three passes (`MadT1_S4`, `S4`, `S2`), `MaxColorT4`, the two ghost draws, `MadOffsetT2`,
  then `LightShaftMask_SM3`, four `TexCoordMadT1_S4`, `TexCoordMadT1_S3`, the layers' composite `MadT5`, the 2x draw, the tone map.
  So the original draws the shafts on shape 3 after the afterimage, as the composite's fifth input (the draw order and the fifth input OBSERVED;
  that slot carrying the shafts READ in 8g; the wiring and samplers checked by no texel test on shape 3), as 8g wrote them.
  `-1950` captured nothing (0 draws at frame 9977; not read why).
- **The tests** (no native renderer code changed): the capture is a zone of `yebis_glare_test` (the shafts appended from the mask
  draw's state on a `-shafts-` capture, every glare and composite word, the shaft draws' techniques, targets and input sizes, the
  mask's film grain sized 256), `yebis_shafts_test` (Alpha_R's block, the shaft plan LS-2 and the merged chain LS-3 on shape 3: the appended draws from the layers' composite), `post_draws_test` (draws 0 to 55, the library's programmes
  and ours) and `glare_stage_test` (the merged chain from the original's scene HDR). **Alpha_R's focus reference changed** (D-0268):
  the check now reads the frame's globals as E69A0 read them (`reference-driver-globals.raw`, before its call) when the run has
  them; in this cutscene the bright pass's block already holds the next frame's focus (3.8820 against the library's S+0x6C0 and the
  driver block's 3.8919, OBSERVED); in the other captures that have both (1667, 1668, 1669, 1705, 1706; Zone reads 1705 and 1706) the two blocks are byte-identical, so their checks are unchanged. `build-t8i-4` (`-Jobs 4`, `-Cache evidence/I-0138/cache-t8d`, main `91450e28d` merged in) exit 0, 190 lines « PASS »:
  `yebis glare` 12039 checks at 9 zones, `yebis shafts` 3361 checks, 0 failures; **post draws hardware: 56 of 56 draws of the
  capture equal the original's output, largest difference 0** (ours too); **glare stage hardware: 350 of 350 surfaces at 12 zones
  equal the original's** (ours too). **But these texel equalities prove nothing on the shafts** (the step review's H1, OBSERVED in
  the capture's files): the bright pass's output has a black RGB (only its alpha non-zero, threshold 0.55), and every input and
  output of draws 6 to 55 is zero (the glare levels, the ghosts, the afterimage, the mask, the radial blurs, MadT5, the 2x draw);
  the glare adds nothing to that frame. What this capture proves is the words, techniques, targets, input sizes and vertices
  (`yebis_glare_test`, `yebis_shafts_test`) and the replays of draws 0 to 5. Builds `-1` to `-3` failed on the tests' own wiring (a one-zone PowerShell array splatted
  into characters, the film grain's size, the focus reference above).
- **+0xC9, read** (a `ghidra-research` reading, `evidence/I-0138/t8i-readings/draw-gate-c9-list.md`, not yet re-read by the pilot
  in the code): the root's +0x60 is the screenshot manager (SprjScreenCapture, made in D573B0 at 140D5757C); +0xC9's setter
  FUN_140d56d90 runs only while a screenshot request is pending (normal or tiled); the in-game key path is a stub in the overlay. So
  in play +0xC9 stays 0 and the original's +0xCA is FUN_140eed7f0's answer (+0x27B, else +0x278) on every step (INFERRED from
  the reading). The reading also gives +0x278 at 140EEDEE1: set while +0x27B is, else when the frame window is over budget by more
  than 1 % of the period; with the retail defaults (mode 5, IsBackgroundFrameSkip false) its hand check gives catch-up on the +0x27B
  steps and one more (the [1, 0, 1] step of 8h): INFERRED, the limiter's head (140EED805..87B) encrypted and outside the overlay.
- **The step review** (`91450e28d..HEAD -- native/`, step-reviewer, Opus 5.5 high; as the reviewer gave it): **H1** the texel
  checks on the new zone compare all-zero surfaces from draw 6 to 55 (above), so a wrong sun position, tap, film grain sampling,
  MadT5's slot 4 or the 2x draw's input would pass; no test detects a uniformly zero compared surface; a shape-3 shafts frame with a
  lit bright pass (the sun on screen; here the slot's sun at (-5.79, 2.43)) is to capture, and no record may claim the texels
  checked until then; **M1** `Driver()` not run on the new zone though its driver files were captured (the only cutscene camera:
  E69A0's slot, +0x134/+0x138 from the fov, `ShaftRecordFromSlot`); **L1** `yebis_glare_test`'s skipped word range wrong on a
  shape 3 without a Layers draw (AppendShafts' MadT2 path: it would skip the sources' words; `yebis_shafts_test` fails loudly
  there); **L2** comments: `yebis_shafts_test`'s header (the focus reference), its 1667-1669 citation (Zone never reads them; 1705
  and 1706 are the runs whose reference changes, their two blocks byte-identical), « the next frame's focus » an interpretation
  (only word 0x1B0 differs), the 8g block's « no capture holds shape 3 with shafts », `yebis_glare_test`'s header and PASS line
  (« every word » while the shaft draws' words are skipped; the shafts « after the composite »), `glare_stage_test`'s « MadT2 »;
  **L3** the 2x draw's shape-3 check tautological (AppendShafts already refuses otherwise). Held, as the reviewer gave it: shape 1's
  paths unchanged, the zone array and splatting, every failure throwing, the film grain's size, the focus reference sound (tied to
  the library's own S+0x6C0; the native's NativeAlphaRInputs reads the same block as DriverSlot). Not yet answered.
- **The step review's answers** (`c7e0ac515`, then `71ca3fc92`): H1 the texel tests count each compared reference surface as with
  content and with RGB content and print it (« a surface of zeros equal to zeros proves nothing »); a capture named `-sun-` (listed by
  hand in the build, `$sunShaftZones`) fails when its mask and radial blurs are all zero in RGB (not the composites, which carry the
  glare whatever the shafts hold: c7e0ac515's review, H1); `i138-t8i-oracle-shafts-1250` is not so named (all zero: 6 shaft draws,
  0 with RGB content, `build-t8i-7`); M1 `Driver()` runs on it: every slot word and record words 0 to 18 equal, record word 19
  (the depth) 0.998149872 against 0.998146057 (OBSERVED), reported and not required when the two globals blocks differ at +0x1B0 (replaced by the third thread's check, below); L1 the skipped word range from the draws present; L2 the comments; L3 the tautological check removed.
- **+0xCA natively** (`c7e0ac515`, D-0290): the module calls the original's FUN_140d56e00 once a step again (its code checked byte for
  byte, as in 8h's `e95ee2cfa` form), at the step's first gate reading (the collect's), so the gate closes on +0x278 as the
  original's; +0xC9 stays 0 (no screenshot manager natively; none in play in the original, INFERRED from the reading). Rows: the
  closed runs with `without_remo` (closed steps with +0x27B clear) and, in the keys logger (both modes), the limiter's mode and
  windows (+8, +0xC, +0x270, +0x274) and DS3's focus.
- **The witness** (`build-t8i-6`, the ASUS, 10:37 to about 11:05 Paris, offline, restored by hash, no measure;
  `evidence/I-0138/run-t8i-series.log`): `i138-t8i2-c32000010-nat` (holds `6000 p5 ... p95`, 10 of 10 released): six closed runs, one
  at each cut start (7, 12, 15, 15, 17 and 20 steps), each ending with exactly one step closed without +0x27B (the +0x278 step the
  original also leaves undrawn, `[1, 0, 1]` in `i138-t8h3-orig`), none elsewhere, no disagreement (OBSERVED). The limiter: mode 5, both
  windows 0 in every row, with the focus on (257 rows) and off (6 rows) (OBSERVED): so +0x278 can only follow a +0x27B step (the
  reading's formula, INFERRED), in focus or not, and a GPU-bound frame cannot close a step with these settings. The out-of-focus
  runs (`Run-I138T8iFocus.ps1`, `i138-t8i3-focus-*`): SetForegroundWindow did not take the foreground from DS3 (« giving the
  foreground to the taskbar: False »), so they ran in focus but for 4 logger rows each (focus 0); both exit 0, restored; in play no
  closed run natively, `draw_gate` [0, 0, 0] and the limiter mode 5 with both windows 0 in both modes, the original's included (OBSERVED).
- **The lit recapture failed**: `i138-t8i-oracle-sun-2120`, `-2240`, `-2450` (32000020, the bright city at about 35 s, OBSERVED in the
  shots) all captured 0 draws, as `-1950` did; why is given below (« Why the lit captures got 0 draws »).
- **c7e0ac515's step review** (`8df41e7fd..c7e0ac515`, step-reviewer, Opus 5.5 high; as the reviewer gave it): **H1** the declared-lit
  requirement met by non-shaft composites (MadT5, the 2x draw; Layers in the glare stage); **M1** the word-19 exemption keyed on a
  correlate, hiding a native-vs-original mismatch whose cause is unread (D-0290); **M2** the cloth readers' claim unsupported (their
  order against the collect not read); **M3** the runner's fixed `-Seconds 300` against a hold that scales; **L1** `without_remo` also
  counts the forced +0x278 step ending each cut-start run (not catch-up alone); **L2** `Get-ShaftCaptureDraws` assumes the last draw is
  the tone map; **L3** A8 and RGBA8 alpha counted as RGB, R32F -0.0 as content; **L4** a dead `keybd_event` declaration in the runner;
  **L5** the catch-up closure ships before its witness. Answers (`71ca3fc92`): H1 fixed; M2 marked open; L1 by the witness above (the
  one step a run is that forced step; with both windows 0 nothing else can be). Left: M1, M3, L2 to L5 (L5: the witness above shows
  the windows at 0, so no catch-up closure outside the +0x27B runs with the retail settings).
- **The third thread's answers** (`0cbb83ac9`, then its step review's fixes; `build-t8i-10`: `-Jobs 8`, `-Cache evidence/I-0138/cache-t8d`,
  exit 0, 191 lines « PASS »): **M1** the record's c48 re-derived in float32 (OBSERVED, the test's output): the conversion's
  cancellation (1 - c48 / near, near 0.08) gives the captured 0.9981461 for every focus in [3.89206, 3.91163] (the band scanned, not clipped at its end); this frame's 3.89189 gives
  0.9981499 and the bright pass's 3.8820 0.9981575, so the record was converted with neither, as FUN_1416EA000 before FUN_1416E9050's
  stores predicts (the previous frame's words, READ in tranche 8d); the previous focus extrapolated at the same pace, 3.90176, is inside
  (an earlier hand computation gave 3.89108, wrong). Natively the record now takes `RecordDepthSpaces`: this frame's near, far and source
  with the previous presentation serial's conversion words (AppendNativeShafts, its one caller under cache_lock; a frame whose previous
  serial served no shafts takes its own; near and far stored by E69A0 before E9050 is INFERRED from the earlier readings, not re-read in
  Ghidra); Alpha_R keeps the current frame's, as the library. **D-0268**: the 8i exemption of word 19 is replaced by a check: when the
  branch is on and the focus moved, the word must be the conversion of a focus from this frame's to two of its steps back (the band
  printed, `[3.89206, 3.91163]`; whether this frame's own focus gives it reported: not here), else equal; `RecordDepthSpaces` has its own
  unit check. **M3** the focus runner's `-Seconds` is its hold plus 180 (the hold capped at 720); **L2** `Get-ShaftCaptureDraws` fails
  unless the capture's last draw is `tech_Tonemap*`; **L3** `post_draws_test` counts content by the view format (RGBA16F and R32F a zero's
  sign ignored, RGBA16F and RGBA8 alpha apart, A8 never RGB, an unknown format fails); **L4** the dead `keybd_event` removed; the shaft
  depth witness row logs the record's spaces. L5 not answered (recorded above: no catch-up closure with the retail windows at 0).
- **The step review of `0cbb83ac9`** (`71ca3fc92..0cbb83ac9`, step-reviewer, Opus 5.5 high; as the reviewer gave it): **M1** the previous
  frame's whole spaces taken, near and far included, where the library takes only its conversion words (a cut from far 1000 to 10000:
  about 3.5e-3 on word 19 for a frame); **M2** the two new statics not in `scripts/thread-affinity-allow.json` (Test-Project failed), the
  new mutex hidden behind the witnesses' `lock` key; **L1** « previous » counted glare plans, not frames (state from many frames back after
  the shafts were off or a load); **L2** the test assumed a constant focus pace, its comment's band wrong, `nextafter` without a finite
  guard; **L3** the witness row logged this frame's spaces; **L4** the focus runner's refusal before its `try` (no END line), its 800 cap
  dead; **L5** the oracle runner's fixed `Seconds=300` against shots that now extend. All fixed as above (M2: the inner mutex dropped,
  cache_lock serialising the caller, the statics listed; L1: the presentation serial; L5: `Seconds` the last shot plus 200, 300 to 900).
  Held as sound: no offscreen caller, the ordering under cache_lock, the content counts by format, the tone map's pairing, the shot list.
- **The step review of `050f3c888`** (`0cbb83ac9..050f3c888`, step-reviewer, Opus 5.5 high; as the reviewer gave it): **M1** the word-19 band holds this frame's focus, so the check no longer told the previous frame's words from this frame's; **L1** the previous spaces stored before three later refusals, against the comment; **L2** the serial test inline and untested; **L3** the focus runner's 720 cap silent; **L4** the oracle's 900 cap silent; **L5** the band's scan unbounded on a large focus jump. Fixed (`build-t8i-11`, 191 PASS): M1 this frame's own conversion must differ where the grid separates it from the band's end (it does here); L1 the comment says what counts; L3 and L4 refuse instead of capping. Left: L2 and L5 (a test of the serial rule; a bounded scan).
- **Why the lit captures got 0 draws** (OBSERVED in their journals): the oracle's Run-AlphaCast leaves the cutscene after its last
  `-CutsceneShots` entry (30 s), so the journals end near library frame 9900, before the capture frames 10178 to 10497.
  `Run-I138T8eOracle.ps1` now takes shots past the capture frame. **The recapture** (11:13 to 11:27 Paris, the ASUS, offline, three
  saves restored by hash, no measure; `evidence/I-0138/t8i2-oracle-sun.log`): `i138-t8i2-oracle-sun-2120` and `-2240` 57 draws each,
  the slot's sun on screen ((0.94, -0.59), (0.96, -0.59)) with a colour of about 11 to 14, but the original's bright pass is black in
  RGB again (0 texels; alpha only), so the mask, the radial blurs and MadT5 are all zero; `-2450` 47 draws, the slot's colour 0 (the sun
  behind). **H1 stays open in 32000020**: no capture of this cutscene has a lit bright pass with shafts; the three are named `-sun-` by
  the runner but left out of `$sunShaftZones` (they would fail its content requirement, rightly).
- **The fourth thread** (2026-11-22 from 12:00 Paris): 050f3c888's L2 and L5 (`bae65fe59`, `build-t8i-12`, 191 PASS): the serial
  rule is `PreviousSpacesApply` with its unit check; the word-19 band's scan refuses a band of more than 2^22 positive floats.
  **Why 32000020's bright pass is black** (OBSERVED in `i138-t8i2-oracle-sun-2120`'s files): its input (the scene at 320x180) peaks
  at R 0.62, G 0.17, B 0.11 (p99.9 about 0.13) and the pass's threshold is 0.55 after the block's factor 2.0, so no texel passes:
  the cutscene's scene is under the threshold, no word is wrong. That frame's mask is closed by depth as well: its gate
  (`shaft_mask.w`, the record's word 19, 0.99929) is above every alpha of the bright pass (at most 0.857, no sky behind the slot's
  on-screen sun), so its shafts are zero whatever the light. **H1 answered by a lit replay** (`b53c9fa0a`, `build-t8i-14`, 193 PASS):
  `post_draws_test`'s `-ours-lit` gives each input uniformly zero in RGB a synthetic light (a floor, a noise across the thresholds, a
  spot; alpha kept; RGBA16F and R11G11B10), then our programmes and the pinned originals draw from it with the captured block and
  must agree byte for byte, the radial blurs, MadT5 and the draw after it (the 2x draw) required lit (at least 1 % of their texels a
  normal value); the captured outputs are not compared. On `i138-t8i-oracle-shafts-1250`, draws 0 to 55 (`build-t8i-15`, WARP and
  hardware): 70 inputs given light, 56 of 56 byte for byte, 54 of 56 outputs lit, the required draws 7 of 7. **The mask is not
  lit there** (0 texels lit; its 14 non-zero texels are denormal steps): that frame's own words put the sun at (-5.79, 2.43), far off
  screen, and its falloff is about 3e-6 at most whatever the light. The mask's programme is shape 1's (the same pins, `7F09C8F7D24E`
  and `D275F69939F1`, OBSERVED in the three journals), exact at 1667 and 1669 and lit by the 1 % rule at 1669 (25 % of its texels;
  1667's 0.5 %); on shape 3 its words are checked, not its texels. `build-t8i-16` (195 PASS) runs the lit replay on
  `i138-t8i2-oracle-sun-2120` too: 56 of 56 byte for byte, its 7 required draws lit, its mask unlit (its closed gate).
  So shape 3's radial blurs, MadT5 and 2x draw are checked on light with that frame's words; the original's lit shape-3 frame is
  still not captured.
  `i138-t8i2-oracle-sun-2120` (a second shape-3 shafts frame) is added to `yebis_glare_test` and `yebis_shafts_test` (its words and
  driver slot equal, record word 19 equal), not to the texel replays (its zeros).
- **The step review of `02fe4fa52..b53c9fa0a`** (step-reviewer, Opus 5.5 high; as the reviewer gave it): **H1** the mask's lit check
  almost empty (one RGB bit anywhere counted as lit: 14 denormal texels on 1250), the commit message and PASS line overclaiming;
  **M1** MadT5 and the 2x draw, named in the goal, not required lit; **L1** the band's size in a comment wrong (about 82 800, not
  330 000); **L2** the true case just before the serial's sentinel untested; **L3** the per-draw lines still compared with the
  captured outputs in lit mode while the PASS line says not; **L4** the usage text without `-lit`; **L5** the `-sun-` name of a zone
  whose shafts carry no light. Held, as the reviewer gave it: `PreviousSpacesApply` and its caller, the bounded scan's refusals, the
  lit profile's reach, encodings and comparison, the build's loop, 2120's claims against its files. All answered (`build-t8i-15`, 193
  PASS): H1 a draw is lit when 1 % of its texels hold a normal value; the mask out of the requirement, its count reported, the PASS
  line and this record saying so; M1 MadT5 and the draw after it required; L1 to L5 fixed (the per-draw lines labelled a diagnostic
  in lit mode; a note at `$shaftWordZones`).
- **The step review of `6f58b30b7..3fc9a7df8`** (step-reviewer, Opus 5.5 high; as the reviewer gave it): no High; **M1** the comment
  « lit and exact at 1667 and 1669 » false by the new rule at 1667 (its mask 0.5 % of texels normal); **M2** the lit PASS line stating
  what the code does not check (no blur, MadT5 or post-MadT5 draw required by kind, no mask required, the mask's wording fixed);
  **L1** the counts line not echoed in the build log; **L2** the format refusal of `LitTexels` unreachable; **L3** an infinity or a
  NaN counted as lit; **L4** 2120's exclusion from the lit replay no longer justified. Held: the decodings, the 1 % rule in integers,
  the draw after MadT5, the failing paths, the pins, the unit checks, the non-lit behaviour. All fixed (`build-t8i-16`, 195 PASS):
  each kind required at least once and a mask required, the PASS line's mask wording from its count, `-Last 3`, `LitTexels` called
  for every format, exponent 31 not lit, 2120 in the lit replay. These last fixes are reviewed by the final review on the final tree.
- **Left**: an in-play capture of shape 3 with lit shafts. m32_00's five slot-1 collisions are not located (no MSB dump holds their
  names or positions; `Get-MsbGparamSlots.ps1` prints counts only), so it needs an MSB listing first, then a scene bright enough
  to pass the 0.55 threshold with sky behind the sun; carried as a limit. Then the reviews at the merge.

### Tranche 8i: the reviews at the merge

- **The optimisation pass** (read): the production change is two calls (the gate's FUN_140d56e00 once a step, the record's spaces);
  the rest is tests and runners; nothing duplicated to remove.
- **The final review** (`main...330d89bba`, final-reviewer, Opus 5.5 xhigh; as the reviewer gave it): no High. **M1** the records
  contradicting the code (2120 said out of the lit replay; STATE at `build-t8i-14`); **M2** the +0x278 closures unwitnessed under the
  native's own frame-rate option (the module writes the limiter's +0x354/+0x358 when a rate other than 60 is chosen; every witness
  at the default rate); **L1** RESULT sentences unsupported or stale (MadT5's fifth layer marked OBSERVED, « MadT5 read by the 2x
  draw », the 1667-1669 citation, « every focus from 3.89206 up », the oracle's source); **L2** the `-sun-` content requirement keyed on
  a name and fed by an empty list; **L3** two build checks not refusing « skipped »; **L4** the cloth readers' order against the
  collect unread; **L5** overclaiming commit messages (9c6726a1e « texel for texel », b00630984 « the next frame's focus », b53c9fa0a
  « the shaft draws 6 of 6 »: corrected here and in the RESULT above). Held, as the reviewer gave it: the pinned gate code byte for
  byte, the call once a step, +0xC9 at 0 with its reading, the witness rows; the plan's words and the word-19 band; the lit replay;
  RecordDepthSpaces and its state; the failure paths; no D3D11 path. Answers: M1 and L1 the records corrected; L3 fixed; **M2
  witnessed** (13:19 to 13:28 Paris, the ASUS, offline, the save restored by hash, no measure; `Run-I138T8iFocus.ps1 -NoReference
  -EffectFlags test-no-ai,shaft-observe,unlock-120` then `unlock-unlimited`, `build-t8i-16`; `evidence/I-0138/t8i4-rate.log`):
  `i138-t8i4-r120-nat` and `i138-t8i4-runl-nat`, the game asking 120 and 1000 (their `rate` rows) while the 60 Hz ASUS holds about
  16.5 ms a frame (p50; over the requested period), exit 0 both: the limiter mode 5 with both windows 0 in all 101 rows of each, and
  no step closed (0 `native_draw_gate_skipped` rows) (OBSERVED); so the option's rates make no catch-up closure there (a faster screen
  not tried). L2 left: the list stays the mechanism (a capture read as lit goes into `$sunShaftZones` and its name is then checked);
  H1 rests on the lit replay. L4 left open (no fault seen in the runs; the order is not read).
- **The integrity review** (`main...0fb84a3a2`, integrity-reviewer; as the reviewer gave it): **1 High** the last commit's « skipped »
  check on `yebis_glare_test` matched the test's gate line (« the family skipped at ... »), so the final tree's build would fail, and
  that tree was unbuilt; **2 Medium** the focus runner's `-EffectFlags` unstated; **3 Medium** the `-sun-` requirement guarding no
  zone in the build (H1 rests on the lit replay alone); **4 Low** the word-19 check discriminates only where the grid separates this
  frame's focus from the band's end, no test compares production's RecordDepthSpaces with a capture, and the production change since
  0cbb83ac9 ran in no live game before this merge; **5 Low** the shaft draws' words on shape 3 checked as our non-zero words and the
  plan's listed words only (LS-2, LS-3), not every word the programme reads; **6 Low** the 1667-1669 citation; **7 Low** the mask's
  25 % and 0.5 % cited to no file (recomputed by the reviewer from the captures' draws 35 and 37), STATE stale. Held: no special case
  for a scene, no work cut during a measure, the captures', witnesses' and builds' numbers traced to their files. Answers: 1 fixed
  (the skip matched on « PASS yebis glare: skipped », `build-t8i-17`); 2 the runner's flag is the M2 witness above; 3, 4 and 5 recorded
  here as limits (4: the modules of `build-t8i-16` ran in the two M2 runs, shafts on at Firelink, exit 0; the start check after the
  merge runs the merged tree); 6 corrected; 7 the figures are the reviewer's count of `reference-draw-35-output.raw` (32496 of 129752)
  and `reference-draw-37-output.raw` (186 of 37170), STATE rewritten.
- **After the merge** (`079d8b3cf`): the launcher rebuilt from `main` (195 PASS), the start check `start-079d8b3cf` passed (13:42 Paris,
  restored); the disk rotation deleted four build folders older than 48 h (0.9 GB).

## Tranche 9: the main view TAA's static detail (2026-11-22, the R pilot, branches `claude/r-i138-t9` then `claude/r-i138-t9b`)

I-0140 tranche 25's handover: the stone and foliage softer under our TAA since 2026-11-11 (the stone's TAA / no-TAA 0.85 to 0.79).

- **The tools** (`769c5db43`, then the no-jitter flag): `taa-kernel.txt` (the main view's reconstruction coefficient, (0, 30]) and
  `taa-mip-bias.txt` (its material mip bias, [-2, 0]) beside the module, test tools (without them 2.29 and -0.5 as before), one
  `native_taa_calibration` row; `taa-no-jitter.flag` (the TAA on an unjittered camera, accepted by Run-StartupTrace); `taa-reset-always.flag` (`1ec27f1ad`: every
  main-view TAA frame a reset, its own sample when unjittered, so used with `taa-no-jitter.flag`; alone it gives a history-less
  reconstruction of jittered frames; a sharpening still runs after it); the runner
  `Run-I138T9Detail.ps1` (I-0140 tranche 25's view: Eta's m40 save outside Firelink's west entrance, the camera fixed, five stills
  and the hold filmed) and `Measure-I138T9Detail.py` (tranche 39's crops' mean Sobel; the film's mean frame-to-frame change by crop).
- **The runs** (`build-t9-1`, then `build-t9-2` for the no-jitter runs; the ASUS, offline, restored by hash, no measure; 14:17-14:51
  and 15:11-15:23 Paris; `evidence/I-0138/t9-run-a.log`, `-b`, `-d`; all exit 0, no fault). Stone / hair / foliage mean Sobel of the
  five stills, then the film's change on stone / foliage (MEASURED, one run each):

  | Run | Stone | Hair | Foliage | Change stone / foliage |
  |---|---|---|---|---|
  | TAA 2.29 (default; I-0140 t25's 49.6 reproduced) | 49.7 | 31.0 | 55.3 | 0.07 / 0.25 |
  | TAA 4 | 51.5 | 32.0 | 57.6 | 0.13 / 0.41 |
  | TAA 8 | 52.1 | 32.6 | 58.9 | 0.18 / 0.49 |
  | TAA 30 (the nearest sample) | 52.3 | 32.8 | 59.0 | 0.16 / 0.42 |
  | TAA 2.29 without the reactive weighting | 49.6 | 30.8 | 55.4 | 0.07 / 0.24 |
  | TAA 8, mip bias -1 | 51.6 | 32.0 | 56.7 | 0.17 / 0.51 |
  | TAA 2.29, mip bias -1 | 49.1 | 30.3 | 53.5 | 0.10 / 0.35 |
  | TAA 30 without jitter | 57.2 | 35.9 | 67.5 | 0.02 / 0.22 |
  | TAA 2.29 without jitter | 53.4 | 33.1 | 62.8 | 0.02 / 0.14 |
  | No TAA | 62.6 | 41.2 | 72.6 | 0.04 / 0.35 |
  | DLAA (Ray Reconstruction on, 32 phases) | 59.1 | 39.1 | 68.2 | 0.06 / 0.17 |
  | TAA 2.29 with sharpen-50 (Thomas's slider at 0.5) | 67.3 | 40.9 | 75.1 | 0.46 / 0.99 |
  | DLAA over NRD (no-rr.flag) | 54.1 | 34.0 | 60.6 | 0.16 / 0.31 |
  | The original | 57.7 | 37.2 | 55.3 | 0.20 / 0.44 |

  The crops' mean luma is the same in every native run (105 stone, 76 foliage), so the Sobel differences are detail, not exposure;
  the original's is lower (about 91 and 61), so its row is not at the natives' exposure. The films' capture rates differ (622 to 1134
  distinct frames over the measured 19 s, about 33 to 60 fps, `t9-measure.txt`): the change of a crop that moves (foliage, hair)
  grows with the time between frames, so it compares only between runs captured at close rates (the kernels 2.29 and 30, 949 and
  1051 frames; sharpen-50's 0.46, 680 frames, a factor of 6 beyond it); the stone, still, is less affected.
- **Reading** (INTERPRETED, before the diagnostic below, which locates the rest): the reactive weighting does not explain the loss
  (49.6 against 49.7), nor does a bias of -1 under the jitter (it lowers the detail again); the reconstruction's coefficient
  explains about 5 % (2.29 to 30); DLAA keeps far more detail with less crawl. An offline simulation of an ideal jittered accumulation
  from the no-TAA still (Lanczos-shifted by the 8 Halton phases, scratch scripts, not kept) gives the stone 55.0 at 2.29 and 60.5 at
  30, so our resolve loses more than its reconstruction (13 % at 30: 52.3 against 60.5; 10 % at 2.29: 49.7 against 55.0); and
  **without any jitter**, at the nearest sample, our TAA still takes the stone from 62.6 to 57.2. **DLAA over NRD**: DLAA falls
  from 59.1 with Ray Reconstruction to 54.1 over NRD (the same jittered camera; one run each). That run changes both the denoiser and
  DLSS's model (RR's network in place of the SR model) and never varies the jitter: a loss in NRD's lighting under the jitter or in
  the SR model, not separated (INFERRED; NRD under an unjittered camera with DLSS not run). The sharpening at 0.5 overshoots (67.3, above the unfiltered frame) and
  crawls (0.46 against 0.07).
- **Thomas's frame** (2026-11-22 15:33 and 15:34 Paris, relayed by the coordinator): DLAA (with Ray Reconstruction) is not to be the
  default however much better its picture, it costs too much; our TAA stays the default without DLSS; « ne pas trop s'attarder » on the
  TAA, most players will use DLSS; NRD's loss first if it also touches DLSS without RR. The tranche is bounded to one diagnostic slot.
- **Offline, from the 14 runs' records** (the fresh thread, scratch scripts in the session's scratchpad, standard library and ffmpeg):
  the camera is still within each run (a 3x3 kernel fitted between hold-1 and hold-5 of the stone's crop: shift 0.01 px; between
  consecutive film frames 0.005 px at most), the runs differ from each other by a sub-pixel shift (0.08 to 0.46 px from `notaa`, the
  original's run too: the camera's final yaw, not the TAA); the stone's detail in `notaa` is not lighting noise (the mean of the five
  stills keeps 62.6 against 62.9 for one); the loss under the unjittered TAA at 30 is isotropic (x 38.4 to 34.7, y 41.8 to 38.3), not a
  smear along one motion. The motion vectors on that view (the module's `native_velocity_frame` rows, every 30th frame): camera pixels
  mean 2.4e-5 px, max 6.6e-5 px (MEASURED); the history fetch is exact at that motion (Catmull-Rom at f = 0), so the fetch does not
  blur a still view (the diagnostic below splits the rest). The scratch scripts of these readings are kept in
  `evidence/I-0138/t9-scratch/`, re-runnable on the run folders, their outputs not kept; the simulation's (55.0 and 60.5, from the
  thread before) was not kept, so those two figures cannot be checked.
- **The diagnostic** (`build-t9-3`, `1ec27f1ad`'s code, `-Cache evidence/I-0138/cache-t9`, `-NoControls`; four runs on the ASUS, offline,
  restored by hash, no measure, 15:43-15:54 Paris, `evidence/I-0138/t9-run-e.log`, all exit 0, no fault; the kernel 30 and the camera
  unjittered in the three TAA runs (the unjittered camera rests on the flag passed, each run's `alphacast-args.json`, and its effect,
  the stone's change 0.07 to 0.02: the builds before `d26d5cd40` name no `no_jitter` in their row); MEASURED, one run each, stone / hair / foliage, then the film's change on stone / foliage):

  | Run | Stone | Hair | Foliage | Change stone / foliage |
  |---|---|---|---|---|
  | No TAA (again, `i138-t9d-notaa`) | 62.6 | 40.9 | 72.5 | 0.04 / 0.35 |
  | No history (`taa-reset-always`), mip bias 0 | 62.6 | 41.2 | 72.4 | 0.03 / 0.25 |
  | No history, mip bias -0.5 (the TAA's) | 60.0 | 40.6 | 72.2 | 0.04 / 0.31 |
  | History, mip bias 0 | 60.2 | 36.6 | 67.8 | 0.02 / 0.20 |
  | History, mip bias -0.5 (`i138-t9c-k30-taa-no-jitter`) | 57.2 | 35.9 | 67.5 | 0.02 / 0.22 |

  So nothing upstream of the resolve differs once the TAA's mip bias is 0 (the frame through the TAA without its history matches the
  frame without the TAA on the five stills' Sobel means, 62.6 both; the film's foliage change differs, 0.25 against 0.35, one run
  each, the runs up to 0.46 px apart); the unjittered loss of 62.6 to 57.2 is two parts of about 4 % each: **the material mip bias of
  -0.5 lowers the measure** (finer mips do not raise the stone's gradient on this view; tranche 9's -1 lowered it again, 49.1 against
  49.7 under the jitter), and **the history** on a still stone (60.2 against 62.6 at bias 0; the hair and the foliage, which move,
  lose 11 % and 6 %). **The history's 4 %** (INFERRED from `taa.hlsl`'s clip, the final review's M1; not tested): the clip box is the
  3x3's [lo, hi] narrowed to mean +/- 1.25 sigma, so a pixel that stands out from its neighbours (a speck on the stone, a hair strand)
  lies outside it, and its history, equal to it on a still unjittered view, is pulled back to the box's edge every frame; the blend
  at 0.9 then settles near 0.9 x edge + 0.1 x current (an isolated 1 among zeros keeps about 0.55), the perceptual weights favouring
  the darker clipped value, with no motion needed. Its test, carried: the centre sample inside the box (or a clip-gamma test file)
  on the same still view at kernel 30, bias 0, unjittered; about 62.6 if the clip is the cause. The Sobel tables of the runs:
  `evidence/I-0138/t9-measure.txt` (`Measure-I138T9Detail.py` over the 18 run folders, again after the runs).
- **NRD under the jitter** (INFERRED from NRD 4.17.3's sources, `REBLUR`; the run above does not isolate it): NRD takes `cameraJitter` only for its validation view and
  a disocclusion bonus; its temporal accumulation reprojects from the pixel's centre, so on a still view it averages the jittered
  samples of the GI and specular radiance at the pixel's centre (a box over the jitter's footprint), and DLSS or our TAA then treats
  that unjittered lighting as jittered and filters it again. A re-jitter at the compose (the denoised radiance fetched at the
  pixel's jittered position) was computed (the mean complex response over the jitter of the fetch): Catmull-Rom 0.94 at half the
  Nyquist frequency and 0.49 at it, against the box's 0.90 and 0.64; bilinear 0.81 and 0.41: no simple gain, not written. Ray
  Reconstruction, on by default with DLSS (D-0247), denoises and resolves together and does not have the loss (DLAA 59.1 with it).
- **Outcome** (the R pilot, under Thomas's frame): no default changed. The kernel stays 2.29 (30 gains 5 % of detail for twice the
  crawl, 0.07 to 0.16); the material mip bias stays -0.5: 0 is a candidate (4 % on the still stone, unjittered, at kernel 30; why finer mips lower its gradient is not read), but its run under the
  jitter (the detail and the crawl of the default TAA) was refused by Claude Code's guard when the coordinator had given the slot, and
  was not run; DLAA over NRD loses detail against DLAA with RR, NRD's design under the jitter the reading (INFERRED, the SR model's
  share not separated; no simple fix found; RR, on by default with DLSS, does not show it); the history's 4 % on a still view is the
  variance clip by the code (INFERRED, not tested). The test tools stay (`taa-kernel.txt`, `taa-mip-bias.txt`, `taa-no-jitter.flag`,
  `taa-reset-always.flag`). Carried to workstream 15 (the optimisation review): the mip bias at 0 under the jitter; the clip's test;
  and the handover's fall of the stone's TAA / no-TAA ratio from 0.85 to 0.79 since 2026-11-11 (reproduced, 49.7 / 62.6), unexplained:
  the bias -0.5 and the clip both date from R11j (2026-11-10), so neither explains it alone; the change between `build-t38e` and
  `build-t24` (I-0140 tranche 25) is not read. No sheet for Thomas: nothing visible changed.
- **Reviews at the merge** (D-0283, D-0268; `main` at `27e59a137`, the study branch `claude/confident-hofstadter-e7438c` at
  `3f563a6f3` merged in). **The final review** (a fresh `final-reviewer`, `main...12a625c0f`): no High; **M1** the history's 4 % is
  visible in `taa.hlsl`'s variance clip: the record rewritten (INFERRED, its test carried to workstream 15); **M2** NRD's loss under the
  jitter stated as fact on a run that also changes DLSS's model: INFERRED, the confound named, in the RESULT and STATE; **M3** the
  handover's 0.85 to 0.79 neither answered nor carried: carried to workstream 15 as unexplained; **M4** `taa-no-jitter.flag` absent from
  the module's row: `NativeTaaTest` reads it and names it (`d26d5cd40`); **M5** two earlier readings contradicted the diagnostic:
  rewritten. Low: **L1** the row's bias printed where it is not applied: the comment says what applies where (`ae158ca3f`); **L2** the
  runner's ranges: checked before any run; `-MipBias` and `-AddFlags`'s reach said in its comment; **L3** the test files survive a
  killed runner: left, since Run-StartupTrace's stale list would delete the runner's own files before each run; the runner removes
  them at its start and end, and the module's row names any file found; **L4** the measure's output: `t9-measure.txt` (its 18 rows
  match the tables), the scratch scripts kept; **L5** « equals » softened to the stills' means, the film's foliage named; **L6** the
  13 % at 30 and 10 % at 2.29; **L7** the two comments; **L8** STATE's row named workstream 11 twice; **L9** « Suggested next step »
  rewritten to tranche 10; **L10** the study's synthesis and ROADMAP 12 brought in line with D-0307. **The step review** of
  `12a625c0f..d26d5cd40` (a fresh `step-reviewer`): no High; **M1** the runner's comment said `-MipBias` applied to the Extra runs: it
  applies only under the TAA (corrected); **L1** a NaN kernel passed the guard (refused now, checked); **L2** the row's `no_jitter` names
  the flag found, not the camera's state under DLSS; **L3** « about 30 » for the reconstruction's reach, about 8 (both comments
  corrected, `ae158ca3f`, not re-reviewed: comments and the guard the reviewer gave). **The integrity review** (a fresh
  `integrity-reviewer`, `main...45ad9e959`): no High; the stone's Sobel of 11 runs, the crawl of 5, the camera's motion and RR's frames
  recomputed and equal; no test, gate or reference touched, no special case, nothing cut during a run. **M-a** `t9-measure.txt` was
  being written: complete, 18 rows; **M-b** the films' capture rates differ: named above with what compares; **L-a** the unjittered
  state rests on the flag passed and its effect: said; **L-b** the scratch outputs and the simulation not kept: said; **L-c** STATE and
  the outcome now say kernel 30 for the bias's 4 %; **L-d** the original's exposure noted; **L-e** as the final review's L3.
  The complete build of the final tree: `build-t9-final2` (`ae158ca3f`).
- **The merge** (`6d72c6c8c`, 2026-11-22 16:32 Paris): the launcher rebuilt in the main checkout (exit 0, 195 PASS lines,
  `evidence/I-0138/launcher-6d72c6c8c.log`); the start check `start-6d72c6c8c` passed (16:40 to 16:41, restored, no refusal); the disk
  rotation deleted four build folders older than 48 h (1.1 GB).

## Tranche 10: the bloom plans seen natively (2026-11-22, the R pilot, branch `claude/r-i138-t10`)

Tranche 8a's owed witness: since the bloom gamma's misread word was corrected (8a), the two plans the frame refused before, the Twin
Princes' arena's cutscene 34010010 (`s34_01_0010`, luminance up to 86.68) and the ending's 41000060 (`s41_00_0060`, played after the Soul of
Cinder's death, I-0139 `sheets/endings-dlc.md`; first misnamed the Twin Princes'), are drawn natively and were never seen beside the original. The plan: both cutscenes natively and in
the original, held at the same scene frames on the Remo clock (D-0299: Run-I139Cutscenes `-Holds`, Make-I138HoldPairs), the
glare's rows read (the plan drawn, `on`), the bloom compared on the pairs; sheets for Thomas if they differ.

- **The runs** (`build-t9-final2`, main's code; the ASUS, the game's 1280x720 window, offline, restored by hash, no measure;
  Thomas's « J'autorise » at 22:33 after Claude Code's guard refused the launch): `i138-t10-c34010010-nat/-orig` and
  `i138-t10-c41000060-nat/-orig` (`evidence/I-0138/t10-run-b.log`, 22:34 to 22:55 Paris); `i138-t10g-c34010010-nat` with
  `no-yebis-glare` (`t10-run-c.log`, 23:00 to 23:09; the process closed by the controller, exit 0xD3126612, after the module's
  `native_closed`); `i138-t10g-c41000060-nat` with `no-yebis-glare` and `i138-t10s-c34010010-nat` with `no-yebis-shafts`
  (`t10-run-d.log`, 23:26 to 23:37, after the final review's H2 and M1). Holds `6000 p5 ... p95`; every run 10 holds of 10, drift 0,
  no fault, restored. `Make-I138HoldPairs.ps1`: 20 pairs of 20 compared (`evidence/I-0138/t10-pairs/`).
- **Both plans drawn natively** (OBSERVED, the `native_yebis_glare` rows, written at each change: each cut's start and 41000060's
  ramp in cut 2): 34010010's five plans on, none refused (luminance 86.68, 48, 20, 9.17, 12 by cut; L 194 to 20.5); 41000060's on
  (1.67, then a ramp to 5 in cut 2). The original's `witness_cutscene_clock` rows read the same shape and luminance per cut.
- **Against the original** (MEASURED, the client's mean luma of 255, 1280x720 at 8,31, `evidence/I-0138/t10-client-luma.txt`; one
  run each). `no-yebis-glare` removes the glare family (the bloom and the light shafts, `13b_prepare_model_frame.inc`'s glare
  block; the depth of field too, which draws no level in these cutscenes in either run); `no-yebis-shafts` alone changes nothing here (within 0.6), so the family's share is the bloom's. 34010010, native /
  without the glare / original:

  | Hold (cut, luminance) | Native | Without the glare | Original |
  |---|---|---|---|
  | 0 (0, 86.68) | 50.3 | 44.7 | 40.7 |
  | 1 (0, 86.68) | 56.7 | 52.7 | 45.7 |
  | 2 (1, 48) | 65.8 | 55.8 | 48.9 |
  | 3 (1, 48) | 65.5 | 55.7 | 48.4 |
  | 4 (2, 20) | 60.0 | 49.2 | 45.7 |
  | 5 (3, 9.17) | 42.6 | 39.0 | 40.0 |
  | 6 (3, 9.17) | 65.3 | 52.3 | 66.9 |
  | 7 (4, 12) | 43.1 | 42.9 | 38.5 |
  | 8 (4, 12) | 63.6 | 55.9 | 53.7 |
  | 9 (4, 12) | 47.4 | 39.1 | 40.2 |

  The native's bloom adds 0.2 to 13 (MEASURED, its own share). What the numbers bound: at holds 0 to 4 and 7 the native
  **without** its bloom is already 3.5 to 7.3 above the original's final picture, so at least that much is the scene before the
  bloom; the rest of the excess (4 to 11) is not split. At holds 5, 8 and 9 the native without its bloom is level with the
  original's final picture (-1.0, +2.2, -1.1) and the excess (2.6, 9.9, 7.2) comes with our bloom; at hold 6 the native with its
  bloom is level with it (65.3 against 66.9). None of these splits the original's bloom from its scene: there is no original
  without its bloom (and the luma after the tone map does not add), so equal blooms on a darker original scene fit holds 5, 8 and
  9 as well as a stronger bloom of ours, and a weaker bloom on a brighter scene fits hold 6. Only hold 9 has more: by eye on
  `t10-sheets/c34010010-hold-9-3way.png`, our bloom spreads wider around the burning sword (INTERPRETED). Since our programmes
  reproduce the original's from the same HDR (K-0490), any difference in the bloom is in the HDR above the threshold (the scene, an
  effect's emission) or in a word not compared. The bloom's strength against the original is not settled.

  41000060, native / without the glare / original: cut 0 66.3 / 55.1 / 87.3, 28.0 / 27.5 / 51.5, 55.9 / 47.0 / 78.4; cut 1 121.8 /
  93.8 / 136.8, 81.6 / 56.6 / 83.1, 36.2 / 21.8 / 30.9; cut 2 black natively (below). The native's bloom adds 0.5 to 28. Cut 0 is
  21 to 23.5 under the original with the bloom (at hold 1 the ground and the dead tree far darker, `t10-pairs/c41000060-hold-1.png`);
  cut 1 mixed. Whether the original's whitish veil over cut 0 (`t10-sheets/c41000060-hold-0-3way.png`) is a stronger bloom or more
  light in the scene is not split (the same limit).
- **41000060's cut 2 is black natively** (OBSERVED, a defect outside the bloom): the original shows the eclipse over the clouds
  (« The First Flame quickly fades », 123 to 206 of 255 at holds 6 to 9); within cut 2 (from its first clock row to the
  `02_903_NowLoading2` movie) the native presents 1208 `native_clear` frames, 2119 frames of the subtitles alone and 1 content
  frame; the view's sample: 113 objects, 2 model intents, both unsupported, `materials_ready` false. The tour saw it too
  (`i139-t3c-c41000060-nat` and `i139-t3n` at 90 s, the same black frame, 0 in `yavg-t3d.tsv`) without naming it. Not located.
- **Outcome** (the R pilot): no code changed. Tranche 8a's owed witness is given for both plans: drawn natively, the bloom in its
  place; its strength against the original's is not settled (its own share measured, the original's not separable; a wider
  spread at 34010010's hold 9 by eye). Left in workstream 11, item 8: the bloom's strength against the original (a capture of the HDR before
  the bloom in both modes, or the words not compared). Carried to workstream 7 step 7 (I-0141): the scene before the bloom brighter
  in 34010010 (3.5 to 7.3 at holds 0 to 4 and 7) and darker in 41000060's cut 0; 41000060's black cut 2 (its models not drawn; the cutscene
  workstream or E). Sheets for Thomas: `t10-sheets/` (34010010's holds 0, 2, 5, 6 and 9, native, without the glare, original;
  41000060's holds 0 and 4 the same, and hold 6, native above the original).
- **Reviews at the merge** (D-0283, D-0268; no code, `main` at `f399d1a92`). **The final review** (a fresh `final-reviewer`,
  `main...0164067bd`): **H1** the excess put on the scene's light beyond what the three-way numbers allow, holds 5, 7, 8 and 9 left
  out: all ten holds now, with what they bound; **H2** 41000060's bloom never compared: its run without the glare (`i138-t10g-
  c41000060-nat`); **M1** the column without the glare also removes the shafts: a run without the shafts alone (`i138-t10s-*`, no
  change); **M2** 41000060's cut labels; **M3** cut 2's counts taken past the cut; **M4** `s41_00_0060`; **M5** the stale « Suggested
  next step »; lows: the 7.3, the column added to `t10-client-luma.txt`, 8a's « owed » sentence, the glare rows written on change,
  STATE's rows and Thomas's list, the tour's measure named. **The integrity review** (a fresh `integrity-reviewer`,
  `main...ce737a956`): no High; the 70 luma values, the derived figures, the cut-2 counts and the glare rows recomputed and equal;
  **M1** the bloom's strength at holds 5, 8, 9 and 6 stated beyond the evidence: rewritten as not settled; **L1** hold 7 with holds
  0 to 4; **L2** the flag also removes the depth of field (not drawn here): said; `t10-run-a.log` holds a refused first launch (the
  rows passed as one string), nothing run.
- **The merge** (`bf49a54f1`, 2026-11-22 23:45 Paris): records only, no native or script change since `6d72c6c8c`, so the launcher
  built then and its start check `start-6d72c6c8c` stand (the coordinator's agreement); no rebuild, no start check.
