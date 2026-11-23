# I-0138 tranche 1: the base and the original colorimetry, ready to implement (static)

2026-11-12, the workstream 11 pilot. Written read-only from the native tree at `8f415270b`, [STUDY](STUDY.md) and the
original's tone-map programme's disassembly; nothing built or run. It waits for Thomas's go on [PLAN](PLAN.md) and for ROADMAP
workstream 2 to close. Labels: **verified** (read in the code, the disassembly or a record of this repository), **proposed**,
**open**.

**Built on 2026-11-13** ([RESULT](RESULT.md) « Tranche 1 »), with these departures from the sheet below, which stays as the design
it was: two draws as the original's and not one (§2.3's open point answered: the card's 8-bit write is not `floor(x 255 + 0.5)`),
so the 8-bit image between the tone map and the LUT stays; the programmes compiled by fxc with IEEE strictness (§6's fallback, taken
from the start); §1's `c = sat(C (y, 1))` is `sat(C (y, 1) + 2^-22)`, and the vignette multiplies the exposed scene before the
curve's floor (§1's open point); the vignette's coordinates given at the three vertices, not derived from a rectangle (§2.1); the
new stage owns its LUT volume, the old stages untouched behind the flag (§2.2); the journal row keeps its name with a
`programmes` field (§2.4).

## 0. Answer first

1. **One pass of our own replaces YEBIS's tone map and grading programmes**: `PostToneStage`, an HLSL pixel programme compiled
   by the build, fed by a parameter block of our own (no `$Globals`), drawing the lit HDR, the glare, the automatic
   exposure's factor and the 16³ LUT into the 8-bit target the frame already uses.
2. **Its oracle is the original itself, bit for bit**, not a tolerance: the unit tests T12 and T17 already hold the original's
   captured tone-map and grading draws at the witness zones (inputs and output bytes); the new pass runs on the same inputs
   and must give the same bytes on the capture's adapter. So the tranche keeps the original's own 4x4 dither and its 8-bit
   step between the curve and the LUT, reproduced inside the one pass (§2.3). **Nothing changes on screen, by construction.**
3. **The blue-noise dither and the float output do not come here**: they change bytes (within the dither's amplitude) and
   belong with the output transform of the HDR10 tranche, where the format changes anyway. This corrects PLAN §5's first
   wording for tranche 1 (« seuil annoncé »): there is no threshold, the comparison is exact.
4. **The curve is computed in the programme, not read from a table.** Elden Ring bakes its curve into a table each frame;
   DS3's sensitometric curve is one exponential, and computing it keeps the result exact and costs less than a fetch. The
   parameter model keeps ER's shape (a curve mode, its contrast, the colour matrix, the vignette, the LUT).
5. **What goes**: the pins and run-time reads of the two YEBIS programme pairs (tone map with and without vignette,
   grading), their two `$Globals` blocks, `TonemapStage` and `GradingStage`. **What stays for now**: the glare family, the
   depth of field, the motion blur and the shafts (later tranches), the game's `LutCopy2DTo3DCs` compute programme (GX's,
   not YEBIS's), the automatic exposure reading its tone values (rebased in tranche 3).
6. The old path stays behind a test flag until tranche 8, so both chains can be drawn from one build and compared.

## 1. What the original computes (verified)

The pixel programme `ps_TonemapHDR_Dither_Exposure_Matrix_Gamma`
(`evidence/I-0067/queries/yebis-world/ps_TonemapHDR_Dither_Exposure_Matrix_Gamma.asm`, 40 instructions; DXBC's `exp` and `log`
are base 2):

```
e    = t2(v3)                                              the adapted exposure (1x1)
g_a  = sat(mean(e O0.x + O0.y) + O1.x (e O0.x + O0.y - mean)) O0.z + O0.w        additive glare term
k    = (mean(e O2.x + O2.y) + O3.x (e O2.x + O2.y - mean)) O2.z + O2.w          the exposure's multiplier
gl   = sat((t1(v1) k + g_a) M0.y)                          the glare
x    = max(t0(v0) k M0.x, 0.000061)                        the scene, exposed
E    = exp2(-x T.x 1.442695)
y    = sat((1 - E) (1 - T.y E)^2)                          the sensitometric curve, per channel
y    = y + (1 - y) gl                                      the glare, screen-blended after the curve
c    = sat(C (y, 1))                                       the colour matrix (rows c210..c212)
out  = exp2(log2(c) g) + t3(v4).x D.x + D.y                gamma, then the 4x4 dither;  out.a = sat(dot(t0, M1))
```

With the words the native builds today (`native/world/yebis_tonemap.cpp`, `TonemapGlobals`): `O0 = (2, -1, 0, 0)`,
`O1.x = 0.5`, so `g_a = 0`; `O2 = (2, -1, 0, 1)`, `O3.x = 0.5`, so `k = 1` in the manual case; with our automatic exposure
(I-0132 R11h) `O2 = (1, 0, 1, 0)` and `O3.x = 1`, so `k` = its factor; `M0 = (exposure, 1, 1, 1)`, `M1 = (0, 0, 0, 1)`;
`T` from `TonemapCurve` (`T.x = 1 + 2.0891273 c^(1/3)`, `T.y` the 21-entry table at `c^(1/3)`); `C` from `BuildTonemap` (hue,
saturation, value, contrast, with the library's guards); `g = 1 / gamma`; `D = (1/255, -0.5/255)`.

The vignette variant (`..._Dither_Vignette_Exposure_...`) multiplies by `sat(1 - |uv2|^2)^4`, `uv2` the vertices' TEXCOORD2
from `VignetteRect` (`yebis_tonemap.h`): **open**, where in the sequence it multiplies; read from that programme's
disassembly (`Get-DxbcDisassembly.ps1` on its pin) before writing the HLSL.

The grading (`ps` of `MadColorGradingLutT1_S1`, TONEMAP-SKIN §15, `GradingStage::Globals`): the tone map's 8-bit output is
read back with a linear clamp sampler at the same size, then `out.rgb = LUT3D(rgb 15/16 + 1/32)`, the volume a 16³ built from
the map's 16x256 strip by `LutCopy2DTo3DCs` (`3D[x][y][z] = 2D[x][16z + y]`), sampled trilinearly.

## 2. Design (proposed)

### 2.1 The parameter block: `native/renderer/post_params.h`

```
struct PostToneParams {            // one constant buffer, 128 bytes, ours
    float exposure;                // the record's Exposure after the library's guard (FLT_MIN floor)
    float curve_a, curve_b;        // T.x, T.y (mode 0); the Reinhard constants later (mode 1)
    uint32_t mode;                 // 0 sensitometric (DS3's ToneType 2); 1 Reinhard (ToneType 3, tranche 4); 2.. the Rekindled curves (tranche 6)
    float inv_gamma;               // 1 / gamma after the guards
    float glare_scale;             // M0.y, 1 today
    uint32_t flags;                // bit 0 vignette, bit 1 LUT, bit 2 the automatic exposure's factor bound, bit 3 exact 8-bit output
    float pad;
    float colour[12];              // C's three rows
    float vignette_rect[4];        // VignetteRect's (x0, y0, x1, y1); the programme derives uv2 from the pixel's uv
    float lut_scale, lut_offset;   // 15/16 and 1/32
    float dither_scale, dither_offset;
};
```

- Built by `BuildPostTone(record words, camera, executable data)` in a new `native/world/post_tone.{h,cpp}`, which calls
  the existing `TonemapCurve`, the matrix builder and `VignetteRect` unchanged (they are DS3's data semantics, already ours
  and already tested bit for bit on captured words; they move out of the `yebis` namespace in tranche 8).
- `NativeTonemapInput` carries `PostToneParams` instead of the 3,584-byte block; its `globals` member stays until tranche 3
  because the automatic exposure reads it (`AutoExposureStage::Record`, words 616, 808, 832, 840-851).

### 2.2 The stage: `native/renderer/post_tone_stage.{h,cpp}`, `native/renderer/shaders/post_tone.hlsl`

- One full-screen triangle; inputs t0 the lit HDR (or DLSS's or the TAA's output), t1 the glare composite (a null view
  without the family), t2 the automatic exposure's 1x1 factor (flag bit 2), t3 the 4x4 dither texture (the same bytes,
  `yebis::TonemapDither`), t4 the LUT volume (flag bit 1); samplers as the two original draws bind them (point clamp for
  t0, linear for t1, point wrap for the dither, linear clamp for the volume: to confirm on the captures' sampler rows).
- The programme compiled by the build with dxc into a header (as `rr_guides.hlsl`, `Build-GameBootstrap.ps1`), no run-time
  compile; its pipeline goes in the pipeline cache and the precompile list like the others (I-0132 R12d).
- The HLSL transcribes §1's instruction order literally (`exp2` and `log2`, the same association of the multiplies), so
  that the driver compiles the same arithmetic.
- The stage owns nothing but its root, pipeline and dither texture; the caller owns the transitions (the frame graph sees
  them), as `TonemapStage` today. The LUT volume stays `GradingStage`'s builder, reduced to that role (its draw goes).

### 2.3 The 8-bit step inside one pass (the exact mode, flag bit 3)

The original writes the tone map into an 8-bit target, then the grading samples it. Sampled at the same size, the linear
filter returns the texel itself, so the two draws equal one pass that quantises between them:

```
q   = floor(out_tonemap 255 + 0.5) / 255          the UNORM write of the first draw
rgb = LUT3D(q 15/16 + 1/32)                       the second draw
```

then the target's own UNORM write. With the LUT off the pass writes `out_tonemap` directly, as the original's single draw.
**Open**: that the rounding of a UNORM render-target write is `floor(x 255 + 0.5)` on the capture's adapter (D3D's rule);
T17's goldens decide.

### 2.4 The frame (`scene_pass_frame.cpp`, the `tonemap` branch, lines 1164 to 1189 at `8f415270b`)

- `tonemap_.Record` and `grading_.Record` become one `post_tone_.Record(list, source, glare_t1, adapted, lut_volume, target,
  params)`; the grading's 8-bit source target and its pass (`colour_grading` in the frame graph) go; the plan key keeps one
  form for both paths (the pass declared whatever happens, as the others).
- `13b_prepare_model_frame.inc`: the tone block builds `PostToneParams`; the row `native_yebis_tonemap` becomes
  `native_post_tone` with the same fields; the refusals « programmes » and « vignette programme » disappear (the programme
  is ours); « tone type » stays refused until tranche 4 brings the Reinhard mode with its constants read.
- The old path behind `post-tone-old.flag` (a test tool beside the module, WORKFLOW's flag habit), removed in tranche 8.
- `no-yebis-tonemap.flag` and `no-yebis-grading.flag` keep their meaning for the runners.
- The witnesses of `world-pictures.flag` (`-tonemap.rgba`, `-lut.rgba32f`) : the first has no target any more in the new
  path; it is taken from the old path's flag run when needed.

## 3. Tests (proposed)

| Test | What it proves | Oracle |
|---|---|---|
| `post_tone_stage_test` on T12's captures (m37 `Reference1638/1643`; the vignette variant where the capture has it; m40 and m30 `Reference1705/1706`) | the curve, the matrix, the gamma, the dither, the vignette | the original's captured output, 0 differing bytes on the capture's adapter, the tolerance of T12 elsewhere (unchanged: 1/255 on fewer than 5 % of the bytes) |
| the same on T17's captures (m31 `Reference1696`, m40, m30), LUT on, the tone map's input taken from the frame's tone-map capture | the 8-bit step and the LUT | the original's captured grading output, same rule |
| `post_tone_test` (CPU) | `BuildPostTone` equals `BuildTonemap`'s words on the captured records (the existing goldens of `yebis_tonemap_test`) | the captured `$Globals` words |
| the old and the new stage on the same synthetic inputs (a ramp to 64, saturated colours, negative and NaN inputs) | no divergence outside the captures' range | byte equality between the two paths |

D-0268: no tolerance is added or changed; the goldens are the existing ones. If the hardware comparison is not 0 bytes, the
count and the largest difference are reported as they are and the cause is read before any merge.

## 4. Live (after the merge, D-0229, and one grouped witness slot)

- The start check.
- One slot, announced: the old path's flag against the default at Firelink Shrine, the High Wall and Irithyll, at 1280x720,
  1920x1080 and one arbitrary window, the frames taken at the same replayed instant; expected equal bytes outside what
  moves between two runs, so the comparison is on a paused or replayed frame, measured (`ffmpeg` signalstats and the byte
  count), not judged by eye. Shown to Thomas as a number and one pair of pictures.

## 5. Commits (each builds and passes the unit tests)

1. `post_tone.{h,cpp}` with its CPU test (the parameter block from the record).
2. `post_tone.hlsl`, `PostToneStage`, the build's dxc line, `post_tone_stage_test` on the captures.
3. The frame's edits and the prepare block, the old path behind its flag; the pipeline cache and precompile list.
4. The records (RESULT, STATE), after the reviews.

## 6. Risks and open points

- A DXIL programme compiled by the driver may round one operation differently from the original's DXBC on the same GPU: the
  test tells; the fallback is the same HLSL compiled as shader model 5 by `fxc` (the original's own compiler family).
- The vignette's place in the sequence and the samplers of the two draws: read on the pins and the captures before the HLSL
  (§1, §2.2).
- The automatic exposure keeps reading the old block until tranche 3: `NativeTonemapInput` carries both for two tranches.
- Not in this tranche: the Reinhard mode, the chromatic aberration (three cutscenes), a LUT fade between two areas (the
  original snaps), HDR10, the blue-noise dither, the Rekindled curve.
