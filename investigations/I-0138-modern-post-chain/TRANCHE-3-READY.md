# I-0138 tranche 3: the exposure, a first sheet (static)

**Corrected on 2026-11-13: §0's point 2 and §2 below are wrong.** The tone map multiplies the glare by the adapted factor too
(`post_tone.hlsl`: `(glare x k) x glare_scale`), so the bloom already follows the exposure in strength; only its threshold sees
the scene before the adaptation. The question for Thomas is withdrawn ([RESULT](RESULT.md) « Tranche 3 », step 2). The text
below is the sheet as written.

2026-11-13, the R pilot's thread (Fable 5.2), written while tranche 2 waited for its merge slot. Read-only and short: what the
next thread should read first and decide; it is not a finished design. Read for it: `native/renderer/auto_exposure.h`, the
tone map's and the glare's prepare blocks (`13b_prepare_model_frame.inc`), the frame's exposure pass
(`scene_pass_frame.cpp`), [PLAN](PLAN.md) §5, [ER-VALUES](ER-VALUES.md) (its answer lines and §3), `auto_exposure_shader.h`. Not read:
`auto_exposure.cpp`'s body, `auto_exposure_math.h`, `auto_exposure_test.cpp`. Labels: **verified**
(read in the code or a record of this repository), **proposed**, **open**.

## 0. Answer first

1. **Proposed: the automatic exposure reads our tone-map block instead of YEBIS's `$Globals`.** It is the last reader of
   that block on the default path: after it, `yebis::TonemapGlobals` is built only for the two test flags of tranche 1.
   Nothing changes on screen; the oracle is the stage as it is (the same factor, bit for bit, from the same scene).
2. **A question the tranche must put to Thomas with sheets: does the bloom follow the adapted exposure?** Today it does
   not (§2). It is the one visible choice of this tranche.
3. **Elden Ring's bounded adaptation as a candidate beside our half strength** (PLAN §5's note), on R11h's views.

## 1. What exists (verified)

- `AutoExposureStage` (I-0132 R11h): a compute pass makes a histogram of the log2 luminance of the HDR the tone map reads;
  a second solves, on the GPU, the factor `a` whose predicted displayed grey over the pixels between two percentiles has
  the game's one target (`kNativeAutoExposureTarget` 0.20, `13b_prepare_model_frame.inc`), adapts towards it and writes a
  1x1 `(a, a, a, 1)`: the tone map's adapted factor (`PostToneStage::Record`'s `adapted`, the block's flag
  `kPostToneAdapted`), and the exposure DLSS reads at the next frame (`CopyDlssExposure`, `RecordDlssExposure`; Ray
  Reconstruction reads none).
- To predict the displayed grey it runs « the tone map's own curve, matrix and gamma » from **the tone map's `$Globals`**
  (3,584 bytes bound as its b0; its header names the words 616, 808, 832 and 840 to 851): the frame passes
  `input.tonemap->globals` (`scene_pass_frame.cpp`, the pass « auto_exposure »).
- The default: on, at half strength (`strength` 0.5: the goal is half the solved log2 factor), bounds `[-8, 10]` in log2,
  speeds 3 and 1 stops a second; `no-auto-exposure.flag` and `rt-auto-exposure-full.flag` are the test tools (D-0263,
  D-0264).
- Tranche 1's RESULT: the factor is taken as grey; the original's desaturating remap of a coloured factor is not
  transcribed.

## 2. The bloom and the adapted factor (verified, then open)

- The glare's bright pass takes the area's fixed exposure (`in.exposure=word(0x98)`, the glare's prepare block; the
  programme's `weights[0]`); the adapted factor multiplies the scene in the tone map only. So in a view the adaptation
  brightens, the scene is multiplied by `a` and its bloom is not: the bloom's share of the picture falls by `a` (and rises
  where the adaptation darkens).
- `yebis_glare.h` says of that word « +0x144 = exposure / 1, the manual case »: the library has another case, with its own
  automatic exposure, on in two world files only, `m30_00_0100` and `m30_00_0110`, not selectable ([CENSUS-WORLD](CENSUS-WORLD.md));
  the native refuses a record with it (`auto exposure`). **Open**: what the
  library's bright pass takes in that case (a targeted read of the function that writes +0x144), as a model for ours.
- Elden Ring applies its exposure to the bloom's source (`ToneMap_BrightPassPS`: `Source x g_ToneMapSceneLumScale`,
  ER-VALUES §6).
- **Proposed**: sheets of R11h's views with the bright pass's factor `exposure` and `exposure x a`, for Thomas; the
  default stays today's until he chooses. Cost: the bright pass's block takes the factor from the 1x1 texture (one more
  input and one product in `post_glare.hlsl`'s bright shape, which then no longer has the original's instructions on the
  frames the adaptation is on: its exactness tests keep the form without it).

## 3. Elden Ring's bounded adaptation (verified in ER-VALUES, its mapping inferred)

- Shipped values: auto exposure on in 133 of 173 gameplay entries; Middle Gray -7 (-8 to -6); Adaption Min -1.5,
  Adaption Max 0.4 to 0.5; light and dark adapt speeds 0.25 (the overworld 0.1). How these parameters become the
  constants of its shaders is inferred, not observed (ER-VALUES §7).
- **Proposed candidate**: our solver at full strength with the log2 factor bounded to `[-1.5, 0.5]` and Elden Ring's
  slower speeds, shown beside our half strength on R11h's views (and one of the tour's views judged too dark outdoors,
  I-0139). It is « ours with Elden Ring's bounds », not Elden Ring's pass: say so on the sheets.

## 4. Design (proposed)

- `AutoExposureStage::Record` takes a `PostToneParams` (160 bytes, `post_params.h`) in place of the `$Globals` pointer.
  **Verified** in `auto_exposure_shader.h` (a source string the stage compiles at `Open`, the test too): the solver reads
  five things of the `$Globals`, all of them fields of our block: `g[154].x` the exposure, `g[208].xy` the curve's two
  constants, `g[210]` to `g[212]` the colour matrix's rows, `g[202].x` the inverse gamma. Its `DisplayGrey` is the tone
  map's curve, matrix and gamma on a grey, without the vignette, the glare's add, the dither or the LUT, and with
  `exp2(-x a 1.442695)` where the tone map has `exp(-x a)`: it is the solver's prediction, not the picture, and it must
  stay as it is for the factor to stay the same (do not « share » `post_tone.hlsl`'s expression: the bits would move).
  So the change is the cbuffer's declaration and five names; `auto_exposure_math.h` and the test follow.
- `NativeTonemapInput::globals` and `yebis::TonemapGlobals` leave the default path (they stay for `post-tone-old.flag`
  and `post-tone-verify.flag` until tranche 8).
- The exposure DLSS reads: unchanged (the area's exposure x `a`).

## 5. Tests (proposed)

| Test | Oracle |
|---|---|
| `auto_exposure_test` on the block | the same factor as from the `$Globals`, bit for bit, on the captures' scenes and synthetic histograms (both forms drawn in the test while the old one exists) |
| the CPU mirror (`auto_exposure_math.h`) | unchanged values |
| live: R11h's sheets again | the same pictures (before and after), then the two candidates of §2 and §3 for Thomas |

## 6. Open points to lift first

- Read `auto_exposure.cpp`'s body and its test (the shader is read: §4).
- The library's automatic case for the bright pass (§2).
- Whether the adaptation should see the picture after the bloom (today: the scene before it).
