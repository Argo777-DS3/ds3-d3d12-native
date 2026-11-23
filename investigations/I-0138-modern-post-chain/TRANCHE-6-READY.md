# I-0138 tranche 6: the Rekindled colorimetry, a first sheet

2026-11-14, written by the thread that built tranche 5 (Fable 5.2), for the thread that builds tranche 6. Nothing here is
built. Sources: [PLAN](PLAN.md) §5 and §7 with Thomas's answers, ROADMAP workstream 11 item 6, [ER-VALUES](ER-VALUES.md) (its
« Ten answer lines » and §6), [ER-TONEMAP-HDR](ER-TONEMAP-HDR.md), the tone-map pass as tranche 5 left it
(`native/renderer/shaders/post_tone.hlsl`, `post_params.h`, `world/post_tone.h`), the licences read on 2026-11-14
(`evidence/I-0138/artifacts/tranche-6-licences/`). Labels: **verified** (read in the code or a primary source), **proposed**,
**open**.

## 0. Answer first

1. **What the tranche delivers** (PLAN §5, ROADMAP item 6): two or three candidate colorimetries beside the original one,
   behind test flags, shown to Thomas on sheets in SDR and in HDR; he picks. The settings row that chooses one is tranche 7's.
2. **The candidates** (Thomas, 2026-11-13: AgX and ACES « on peut les utiliser »; 2026-11-14 06:12 on AgX: « Oui ok, si c'est
   qualitatif et open source »; 2026-11-12 on Elden Ring: its values read from its data, its slight chromatic aberration
   kept): **AgX** after Google Filament's `ToneMapper.cpp` (Apache-2.0), **ACES** by Stephen Hill's fit (MIT), and one **after
   Elden Ring** (its ReinhardEx curve and shipped values, its bloom added before the curve, its vignette, its lateral
   chromatic aberration, without its LUT images).
3. **Where they plug in** (proposed): one more curve choice in `ToneColour` (the body `ps` and `ps_hdr` share since tranche
   5), chosen by a compile-time entry as the Reinhard curve is, so the original's programmes keep their instructions (the
   listings' hash, `tranche-5-step2/sdr-programmes-hash.txt`, is the check to run again).
4. **The sheets** (proposed): one run a zone with a flag that changes the candidate every 300 world frames, in step with
   `world-pictures.flag`'s pictures, so one still view gives every candidate; HDR pictures are judged on the OLED only.
5. **Decisions that are Thomas's**: the pick; whether the area's LUT and colour matrix stay under a new curve (§3.3);
   the glow before the curve (§3.5, his pending answer from tranche 5).

## 1. The pass today (verified)

- `ToneColour` (post_tone.hlsl): the scene texel times the adapted factor and the record's exposure, the vignette, then the
  curve (sensitometric: `(1 - e)(1 - b e)^2` with `e = exp(-a x)`; or the library's Reinhard, tone type 3), the glare
  blended after the curve by a screen blend (`y + (1 - y) gl`), then the area's colour matrix (hue, saturation, value,
  contrast: `colour[3][4]`), saturated. `Tone` (SDR): the record's gamma, the 4x4 dither, an 8-bit write, the LUT in a second
  draw. `ToneHdr`: the gamma, the LUT read in the same draw, the highlights' gain from the curve's largest channel above the
  knee (`hdr::Highlight`, 0.75), written display-encoded in float.
- The parameter block (`PostToneParams`, 160 bytes) has no free word: a candidate that needs constants grows it (the SDR
  programmes read by name, their instructions do not move when words are added at the end; to check by the hashes).
- Auto exposure, the glare, DoF and motion blur run before this pass and do not depend on the curve, except the glare's
  place (§3.5) and the exposure's meaning (§3.4).

## 2. The licences (the web reader's survey, 2026-11-14; not a legal opinion)

- **AgX**: Filament's `filament/src/ToneMapper.cpp`, Apache-2.0, « Copyright (C) 2021 The Android Open Source Project »; its
  matrices credited to Blender's AgX, its contrast curve « Adapted from » iolite's minimal implementation; the excerpt kept
  verbatim (`filament-ToneMapper-agx-excerpt.txt`). What the mod must carry (Apache-2.0 §4): the licence's text, the file's
  copyright notice, a statement that the code was changed (it becomes HLSL). The upstream AgX repositories (sobotka,
  EaryChow) showed no licence file: what is relied on is Filament's grant on its own file. **Open**: where the mod's
  third-party notices live (no such file exists in the repository today; the first candidate that lands creates it).
- **ACES**: Stephen Hill's fit as in BakingLab's `ACES.hlsl` (MIT, « Copyright (c) 2016 MJP »; the credit to Stephen Hill kept).
  Narkowicz's shorter fit is CC0 or MIT: a fallback, lower quality in the saturated colours.
- **Elden Ring**: no code is taken; its curve's formula and its parameter values were read (ER-VALUES §6 and lines 2 to 7),
  its LUT images are not used (D-0276).

## 3. Design (proposed unless said)

1. **The curve's place.** A candidate replaces the curve inside `ToneColour` and nothing else of the frame: same exposed
   input, same output convention (0 to 1, linear, before the record's gamma). AgX and ACES work on Rec. 709 linear input here
   (Filament's AgX converts to Rec. 2020 inside its inset matrix; the excerpt's matrices are the ones to copy).
2. **Exposure matching.** Each curve maps middle grey elsewhere. So the sheets compare looks and not brightness, a candidate
   gets one scale on its input such that a scene value that the original curve of the area sends to display middle grey
   (0.18 linear) lands on 0.18 too; computed on the CPU from the record's curve constants (`curve_a`, `curve_b`), one float
   in the block. **Open**: whether Thomas prefers each curve at its own native exposure (then the exposure rows change
   meaning); recommendation: matched.
3. **The area's colour matrix and LUT under a new curve** (Thomas's call, on the sheets): the areas' looks live in them (the
   green of the Undead Settlement, Irithyll's blue). Three variants a candidate can be shown in: (a) curve only, the matrix
   and the LUT kept (the areas keep their identity; the LUT was authored on the original curve's output, so it reads slightly
   differently); (b) the matrix kept, no LUT; (c) neither (the candidate's own neutral look, as ER's matrix is neutral in
   278 of 279 entries and its look is in LUTs we do not have). Recommendation: show (a) and (c) for AgX and ACES, (c) with
   ER's own values for the ER candidate.
4. **The Elden Ring candidate** (ER-VALUES lines 3 to 7, §6): `t = (k x)^c ; y = t / (t + 1)` with the contrast `c` 1.4 (its
   p10..p90: 1.3..1.55) and `k` from its exposure (median 3.75 EV: the mapping from its EV to our record's exposure is
   **open**, matched as §3.2 until read); its vignette (power 0.5, range 1.2 to 1.3, shape 0.2, a smoothstep to the fourth
   power on the encoded colour); its lateral chromatic aberration (red-green 0.0012, blue 0.0001 of the half picture, a pass
   of its own after the tone map: `ToneMap_PostOETFPS` is only that); its bloom added linearly before the curve
   (`c = scene x exposure + glare x luminance`), where ours blends after it.
5. **The glow and the glare before the curve** (D-0276 asked the emissive glow before the tone map at tranche 5; kept at the
   original's place there, Thomas told, answer pending). For the ER candidate the glare composite adds before the curve
   (one line in `ToneColour`: the screen blend after becomes an add before); the emissive glow (`GlowStage`, a pass after the
   tone map today) would need its composite drawn into the lit HDR before the pass, in scene units: a real change of that
   stage, to cost before promising it. Recommendation: the glare's order with the ER candidate now; the emissive glow's
   move only if Thomas still wants it after seeing the float picture on the OLED.
6. **HDR.** `ToneHdr`'s gain is written for a curve whose shoulder closes exponentially on 1 (the sensitometric one). AgX
   and ACES have their own shoulders; the same knee rule on their output is continuous and monotonic for any curve (it only
   reads the curve's largest channel), so the first HDR pictures can use it as it is; whether each candidate deserves its
   own continuation (for Reinhard-type shoulders `y = t / (1 + t)`: continue in `t`) is judged on the OLED. ER's own HDR
   branch (ER-VALUES §6 item 7) undoes its curve above 1 and applies another shoulder: readable as a model, not decoded
   to its constants.
7. **Flags and the sheet's run.** `tone-look-agx.flag`, `tone-look-aces.flag`, `tone-look-er.flag`, and their variants of
   §3.3 as a suffix; `tone-look-cycle.flag` changing the look every 300 world frames from the original, in the order of a
   fixed list, each change on the frame after a `world-pictures` picture. The journal names the look of each picture.
   Reading HDR-state captures on the ASUS: only through [Compare-HdrStills.py](Compare-HdrStills.py) or the back-buffer
   files ([Decode-Hdr10.py](Decode-Hdr10.py)); a raw capture of a forced HDR10 chain is scaled by the compositor (RESULT
   « Tranche 5 », « The UI in place »).
8. **Tests.** Each curve against a CPU model of its formula on a ramp (the pass's own unit test, `post_tone_stage_test`'s
   harness draws from captured scenes); AgX against Filament's published arithmetic evaluated in double (not against
   Filament's binary); the original's programmes' hashes unchanged; the sheets are Thomas's judgement, not a test.

## 4. Steps in order (proposed)

1. The look's plumbing: the flags, the block's new words, the compile-time curve entries, the cycle flag; the original's
   programmes' hashes checked. No picture changes without a flag.
2. AgX and ACES with their unit tests and their notices; a first sheet at Firelink (variants (a) and (c)).
3. The ER candidate: the curve and its values, the glare before the curve, its vignette, the chromatic aberration pass.
4. The sheets at the witness zones of tranche 2's stills (`evidence/I-0138/artifacts/tranche-2-stills/`, the two extremes of
   the bloom among them: Irithyll and the Undead Settlement), SDR on sheets for Thomas, HDR on the OLED with him; his pick
   recorded.
5. The tranche's reviews and merge. The pick becomes the default only if Thomas says so; otherwise it waits for tranche 7's
   row.

## 5. Also filed for this workstream (the study thread, 2026-11-14)

`investigations/I-0140-effect-families/STUDY-UNREAD-SETTINGS.md` items 1 and 2, to treat before or with this tranche since
they change what a cutscene's sheet shows: a cutscene's ColorGrading LUT refused « non-map record (prefix unread) » (26 of 49
s-files enable it; the original's LUT name for an s-file is unread: `FUN_140095220`'s other branch), and `mapbase` answering
a cutscene key where the original's lookup goes to `default` (Setting Info, MotionBlur, Anti-Alias: INFERRED from the
registration rules).

## 6. Open before code

- The mapping from ER's Exposure EV and ReinhardParam to constants (INFERRED in ER-VALUES §7).
- Whether the cycle flag's change of programme costs a pipeline compile at each change (the three entries built at the
  stage's opening avoid it).
- The third-party notices' file and where the distributed mod shows it.
- A capture of the lit HDR before the tone map (to draw sheets offline from one run) does not exist natively; the cycle
  flag replaces it.
