# W10 under D-0175: our own D3D12 post passes, checked against the original's captures

Thread « Suite W10 YEBIS », 2026-10-13. D-0175 (Thomas): YEBIS is not a dependency; our passes reproduce its passes with its
values, then become replaceable by modern techniques (remaster options). The original library hosted natively
([YEBIS-HOST-IMPL](YEBIS-HOST-IMPL.md)) stays an opt-in development aid; **the oracle is the original game**: the Reference
observer (`yebis-hal-observe.flag`) captures, per draw, the pass, programmes, target, textures and the `$Globals` block
(Reference1615, Reference1632 at m37), and `Compare-YebisReference.ps1` compares a native frame with it word for word.

## 1. Method

1. For each pass family: the draws of the original's frame (pass, inputs, target, the constants the shader reads: the
   `ps_used_globals` column of `evidence/I-0067/queries/yebis-techniques.tsv`), per zone.
2. The constants are computed natively from the native inputs (the YEBIS globals the native feed already writes bit-identical to
   the original at V1, the camera, the extent, the quality object), with the original formulas read in the executable;
   a unit test fixes the formulas on the captured words (golden: the Reference rows).
3. The pass is a native D3D12 pass with the original's programmes (DXBC pinned, YB-2 `yebis_pins.h`), behind a flag, drawn
   into native targets; its output is checked against the original's image (H2) by Thomas's eye and by measurements.
4. Each pass keeps a seam (its inputs and outputs as plan resources) where a modern replacement can be dropped in later.

## 2. First family: the tone map (m37, Reference1632 frame 1200 draw 45)

`tech_TonemapHDR_Dither_Exposure_Matrix_Gamma` (VS `vs_2D_Tex2D5` `6C48209F2312`, PS `358673A09BE2`), into R8G8B8A8_UNORM
1280x720; inputs t0 the scene HDR (R16G16B16A16_FLOAT 1280x720), t1 the glare composite (R11G11B10 420x354), t2 the adapted
exposure (R8G8B8A8 1x1), t3 the dither pattern (R8G8B8A8 4x4). Words the PS reads (reflection) and their captured values:

| Variable (byte offset) | Words | m37 value | Native source |
|---|---|---|---|
| `afRGBA_Modulate` (2464, 32 x float4) | 616-743 | [0] = (1.52, 1, 1, 1): the exposure on t0; [2] (0.25 x4), [3]/[4] (6.72 x4) and [6]/[7] (0.124406 x4): the glare inputs' weights | exposure: global `144962008` (V1); the glare weights from the Bloom globals (to read: the glare composite's own weighting) |
| `afRGBA_Offset` (2976, 16 x float4) | 744-807 | (2, -1, 0, 0), (0.5, 0, 0, 0), (2, -1, 0, 1), (0.5, 0, 0, 0), (2, 2, -1, -1) | to read (texture-coordinate remaps of the five inputs) |
| `fParam_GammaCorrection` (3232) | 808 | 0.45454544 = 1 / 2.2 | 1 / global gamma `14496200C` |
| `fParam_DitherOffsetScale` (3236) | 809-810 | 1/255, -0.5/255 | constants |
| `fParam_TonemapMaxMappingLuminance` (3328) | 832-835 | (2.3985305, 0.51415336, 0, 0) | `T.x = 1 + 2.0891273 c^(1/3)` (`FUN_141718390` case 10, `c` = filmic contrast 0.3); **`T.y` = the 21-entry table at `142B60C40` read at `20 c^(1/3)`** (0.4927 + 0.389 x 0.0551 = 0.5142): YEBIS-WAVE1-READY 1.5's "at `c·20`" is corrected by the capture |
| `m44_ColorTransformMatrix` (3360) | 840-855 | the saturation matrix `s I + (1 - s) L` with `L` = (0.2126, 0.7152, 0.0722) rows and `s` = 0.9, last row (0, 0, 0, 1) | the Tone Map record's ColorS (and ColorH/ColorV, identity here) |

Words 20-21 (1, -1) and the other non-zero words of the block belong to variables this PS does not read.

## 3. Next steps

1. `yebis_tonemap` (CPU), **done (TM-1)**: the constant block from the native globals and the extent; test against the captured words above
   (T.y table, saturation matrix, gamma, dither, exposure); read the glare weights and the offsets' producers.
2. The native tone-map pass (D3D12, pinned programmes), behind `yebis-tonemap.flag`, replacing the sRGB composition when on;
   its glare input black until the glare family exists; the adapted-exposure texture as a native 1x1 constant (auto exposure off).
   **Done (TM-2, §4):** equal to the original's output bit for bit on its captured inputs.
3. Then the glare family (bright pass, pyramid, composite), the light shafts, the depth of field, the motion blur, each with its
   capture comparison; captures at m31, m45, m50, m51 and the title view.

## 4. Implementation log

**TM-1, the tone-map constants (CPU).** `native/world/yebis_tonemap.{h,cpp}`: `CurveTable` (`FUN_141743210`: clamp to [0, 1], index
`x x 20`, floor, lerp to the next entry capped at 20), `TonemapCurve` (`FUN_141718390` case 10: `T.x = c^(1/3) x 2.0891273 + 1`,
`T.y = CurveTable(c^(1/3))`), `BuildTonemap` (exposure, `1 / gamma`, the dither pair, the saturation matrix). `yebis_tonemap_test`
(run by the build with the executable for its data words) against Reference1632's draw 45 at m37: the exposure, the gamma
correction, both dither words and **both curve words are bit-identical**; the colour matrix's diagonal and last row are
bit-identical and its six off-diagonal words are 2 to 4 ulps away (the original's formula order for the matrix is OPEN: it is
built elsewhere than the curve and was not read yet); the hue rotation and the value scale are not handled (`matrix_supported`
false when `ColorH != 0` or `ColorV != 1`; served since GL-4 and I-0113 tranche 20). Next: the D3D12 pass (the programmes' DXBC from the image by their pins, the
captured vertex layout, the dither pattern and the 1x1 exposure input), behind a flag.

**TM-2, the native tone-map pass (D3D12).** `native/renderer/tonemap_stage.{h,cpp}`: one draw of the original programmes
(`vs_2D_Tex2D5`, `ps_TonemapHDR_Dither_Exposure_Matrix_Gamma`, found once in the image's .rdata section by their DXBC magic,
size and the `yebis_pins.h` SHA-256: `AcquireNativeTonemapPrograms`) with the captured vertex layout (stride 48: position and five
texture coordinates; the VS scales the position by `fParam_ScreenSpaceScale` (1, -1) and passes the coordinates through, read in its
disassembly), render state 0 (opaque, no depth) and **the samplers the original binds, observed** (`i66-reference-1643`, `PSGetSamplers`):
s0 point clamp, s1 trilinear clamp, s2 point clamp, s3 point wrap, MaxLOD 0. The host's recorded modes (0, 15, 5, 13) read through
YEBIS-NATIVE 5.2's table give border for s0 and wrap for s1; a first build with s1 wrap differed from the original on the image
edges (1/255 to 7/255 on 0.14% of the bytes), so that table or the record is wrong for these stages (OPEN, for the glare family).
Inputs: t0 the lit HDR (R16G16B16A16_FLOAT), t1 the glare composite (a null view, black, in the game until the glare family exists;
any R11G11B10 resource through `Record`'s seam), t2 a null view (with the captured Offset words t2's multiplier of t0 and t1 is 1 and
its additive term 0 whatever t2 holds; the original's t2 is 0x7F7F7F7F at m37), t3 the dither texture. `yebis_tonemap`: `TonemapGlobals`
(the 3584-byte block: the TM-1 words plus `ScreenSpaceScale`, `Modulate[0].yzw`, `Modulate[1]` and `Offset[0..3]` at their captured
values, whose producer is not read) and `TonemapDither` (`141719770`: the Bayer rows at `142B60D60/D40/D50/D30`, `v/16 + 1/32`
times 255.9999 truncated = `16 v + 7`; texel (r, c) = (v[r][c], v[r][0], v[r][0], v[r][c]), the decompiled interleave, confirmed
by the original's texture). Game side (`yebis-tonemap.flag` with `yebis.flag`): the main view's frame carries `NativeTonemapInput`
(block, dither, programmes) built from the blended Tone Map record (words 0, 1, 2, 4, 5-7, `FUN_1400E42A0`'s order); refused with
the row `native_yebis_tonemap` (the sRGB composition stays) under auto exposure, a ToneType other than 2, a hue rotation or a value
scale; the renderer draws it in place of the sRGB composition when the lit HDR has the presentation extent (rows
`native_tonemap_frame`). Witness pictures: `world-pictures.flag` writes the presented image of every 300th world frame (at most 8).

The oracle: `yebis-tonemap-capture.flag` (Reference runs, with `yebis-hal-observe.flag`) copies, after the original's tone-map draw
of the first detailed frame from 1200, its four inputs and its target through staging textures of the library's own D3D11 context
(`[HAL device+0x10]`) into `reference-tonemap-*.raw` next to the trace, with a row giving the technique, the four samplers and the
adapter (also `reference-tonemap-adapter.txt`: vendor, device, user-mode driver) and the library's tone-map globals at the draw
(`reference-tonemap-tone.raw`). **T12** (`tonemap_stage_test`, run by the build on WARP and hardware): the m37 capture
`i66-reference-1643-m37-tonemapcapture` (t0, t1, its tone globals) through our pass with the natively built block and dither equals
the original's output **bit for bit on the capture's adapter and driver (0 of 3,686,400 bytes)**, which the test then requires; on
another adapter, driver or WARP it requires 1/255 on fewer than 5% of the bytes (WARP: 1/255). The first capture (1638) gave the
same result. `yebis_tonemap_test` also checks the block's words against Reference1632 and the dither against the captured t3 (136
checks).

Live (m37, Gamma): `i66-native-1634` and `i66-native-1639` (the pass on; programmes resolved, 3,881 tone-mapped frames, no fault,
restoration ok; their builds logged the tone-map row only at the first state, before the zone's record), `i66-native-1635` (the sRGB
composition, for comparison) and **`i66-native-1644`** (current build): its rows show the title's record (`hsv [0,1,1]`, matrix word
840 `3F800000`), then the zone's (`hsv [0,0.9,1]`, **word 840 `3F6BD7B2`, the original's captured word**): the game-side mapping of
the blended Tone Map record feeds the pass the original's block (exposure, gamma, curve, matrix). **Observation:** the native
presented picture and the original's tone-map output (before its display pass and interface) differ visibly: the original is
darker, the snow and the cobbles are brighter natively, the character's trousers darker; the native frame has no glare.
**Interpretation:** since T12 shows the pass equal to the original's on the same inputs and 1644 shows the same block, the difference
comes from the inputs (the native scene HDR against the captured t0, and the glare): the scene and glare families' work, not TM-2's.

Captures at other zones (`i66-reference-1640` m31, `1641` m45, `1642` m50): the original draws
**`TonemapHDR_Dither_Vignette_Exposure_Matrix_Gamma`** there (m50's glare t1 640x540).

**TM-3, the vignette variant and the matrix order.** The vignette PS (`D225AEBACAA9`, read in its disassembly) is the tone map with
one more factor on the scene term, `saturate(1 - |uv2|^2)^4`, uv2 the vertex TEXCOORD2: no new `$Globals` word. The rectangle of
uv2 at the screen corners is `FUN_141751A60` (power path): `v = (tan(pi/8) (1 - d) + d fov) power`, `h = v (A / k) / (m / k)` with
`tan(pi/8)` from `[142A66FD0]` = pi/8, `d` = VignetteFovDependence, `A` the render aspect, `k` = 4/3 and `m` = 2/3 the values
`CPostEffect::Initialize` (`FUN_1416EBF80`) stores at `+0x32C`, `+0x388` under its init-flag bits 29 and 30 and hands to the glare
object (`FUN_1417524B0`, `+0x108`, `+0x10C`); `rect = (-h, -v, h, v)` (lens offsets 0), uv2 = rect.xy + uv0 (rect.zw - rect.xy).
The capture now dumps the draw's vertices and indices: at m31 (Vignette Power 0.2, dependence 0) the computed uv2 of the three
vertices equal the captured ones **bit for bit** (`yebis::VignetteRect`, `VignetteCoordinates`). The variant is chosen natively
when the record's Vignette Power (word 26) is positive (inferred: m31 0.2 vignette, m37 0 none); a fov-dependent vignette was
refused (row) until I-0113 tranche 20, which serves it. The stage takes the vignette programme as an optional second pipeline and writes that frame's three vertices with
their uv2 in the frame's upload memory; the vignette is one switch (the vertex coordinates and the pipeline), ready for the
on/off option Thomas asked for (ROADMAP). The colour matrix: `(L - s L) + s` on the diagonal, `L - s L` elsewhere, gives the
captured words bit for bit at m37 and m31 (the earlier `s I + (1 - s) L` was 2-4 ulps off and flipped 3 bytes at m31).

T12 on both captures (`i66-reference-1643` m37, **`1646` m31 with the vignette**): **0 differing bytes** on the capture's adapter
and driver, 1/255 on WARP. Live m31 (`i66-native-1647`): the zone's record selects the vignette (`rect [0.2209, 0.0828]`, the
capture's values), no fault. **Observation:** the native m31 picture is flat and washed out against the original's tone-map output
(contrasted light and shadow, darker stones), the character's hair is missing; the scene HDR, not the tone map (T12), as at m37.

Next: the glare family (bright pass, the pyramid, the composite into t1) with its capture (the same capture flag extended to the
glare draws), then the light shafts, the depth of field and the motion blur; the auto-exposure case (t2 and the Offset producers);
captures at m51 and the title view; and the scene HDR difference (above), measured against the captured t0.

## 5. The glare family: preparation (read while TM-2/TM-3 were reviewed; nothing implemented)

**The draws at m37** (Reference1632 frame 1200; the `$Globals` words the PS read, see the rows): 14 `MadT1_S1` 1280x720 to 640x360
and 15 to 320x180 (offset 0, weight 1); 16 `BrightPassT1_THRESHOLD_LUMINANCE_REMAP_S1` 320x180 to 210x177 RGBA16F (Modulate[0] the
exposure 1.52, [1] 0.015625, [2] 0.5, `fParam_BrightPassRemapFactor` (14, , , 0.125)); 17/18 `MadT1_S3` horizontal/vertical at
210x177 (+-1 texel); 19/20 `MadT1_S5` (+-1, +-2 texels); 21 `MadT1_S1` to 105x89, 22/23 S5; 24 to 53x45, 25/26 S5, 27 `MadT1_S4` up to
106x90 (four taps at +-0.75 target texel, 0.25 each); 28 to 27x23, 29/30, 31 up to 54x46; 32 to 14x12, 33/34, 35 up to 28x24;
36 `MadLumGammaT6` into 210x177 from 210x177 (x2), 105x89, 106x90, 54x46, 28x24 (Modulate 6.72 each at m37, gamma word 808);
37-43 the light shafts; 44 `MadT1_S4` 210x177 to 420x354, the tone map's t1. Programmes (disassembled from the image by their pins):
`vs_2D_OffsetTex2Dn` gives `uv_i = uv + cb0[34+i].xy` and the position scaled by `cb0[5].xy`; `ps_MadT1_Sn` sums
`sample(t0, v_i) x cb0[154+i]`; `ps_BrightPass` thresholds the average luminance against `cb0[156].w` with the `cb0[209]` remap;
`ps_MadLumGammaT6` sums six textures with their Modulate words.

**Blur weights** (YEBIS-TECHNIQUES §5 formulas, m37: radius scale 0.5, D = 2, C = 2.2, so R = 0.55): per channel sigma = r x (1,
540/615, 465/615) (the wavelengths); level 0 (draws 17/18) r = R, normalised over its three drawn taps: the captured words
(0.723071, 0.810116, 0.900081) match; levels 1 and up r = R sqrt 3 (= 2 R sqrt 0.75), `exp(-j^2 / (2 sigma^2))` normalised over 15
taps (`FUN_1416DC220`: integer offsets 1..7, the draw uses the first n): within 0-2 ulps of the capture (green exact). The
normalisation support of the S3 path and the exact float order are to read in `FUN_14174C9E0` / `FUN_14173FDF0`; `FUN_1416DAA20`
is the bilinear-merged variant (the `TexCoordOffsetMad` blurs). The tap-count rule of §5 (n = 2 floor(k/2) + 1) predicts 3 taps at
level 1 where 5 are drawn: to reread.

**Base size**: `FUN_1416EBF80` lines ~1661-1700 (YEBIS-NATIVE 4.1, corrected): `baseW = (int)(W x scale[q] x sqrt((A / r) / a) + [143D7EE00])`
with `[143D7EE00]` = **0.99** (a ceiling, not a rounding), `baseH = (int)(baseW / (A / r) + [142B55250])` with 0.499999: at m37
(GlareQuality 2, scale 0.2, r 1.5, a = A inferred from the decompiled assignment) 1280 x 0.2 x sqrt(1 / 1.5) = 209.02 gives **210**, and 210 / 1.1852 = 177.19 gives **177**, the
drawn sizes (m40: 391.9 gives 392, 330.75 gives 331). Sampler states: to capture per draw (the host's recorded modes were wrong for
the tone map's s0 and s1, §4).

**Plan**: extend the capture flag to every glare draw (inputs, target, samplers, block), a generic stage for the `Mad*` family
(pinned programmes, per-draw blocks computed natively, targets owned by the stage), the bright pass and the composite, each draw
compared byte for byte with the capture, then t1 in our tone map.

**The capture of every draw** (`yebis-draws-capture.flag`, with `yebis-hal-observe.flag`): each library draw of the first detailed
frame from 1200 is dumped as the tone map's (inputs t0..t7, target, samplers, geometry; prefix `reference-draw-NN-`, NN the draw
index of the `reference_yebis_draw` rows). `i66-reference-1648-m37-drawscapture`: the 46 draws, 291 files (128 MB); the depth input of
the depth-of-field draws (R32G8X24_TYPELESS) is refused by the texel-size table (row `"reason":"format"`). This is the glare family's
oracle, draw by draw.

**The bright pass block** (draw 16; `FUN_14174B830` -> `FUN_141736FE0` -> `FUN_141722480`, decompiled exports): Modulate[0] =
(e, e, e, 1) with e the exposure argument (1.52 at m37), Modulate[1] = (1/F, 1/F, 1/F, 1) with F the glare object's `+0x114` (64 at
m37: 0.015625; its producer to read, possibly 2 x ToneMappingFactor 32), Modulate[2] = (t, t, t, t) the threshold `+0x11C`
(GlareThreshold 0.5), and in remap mode 3 `fParam_BrightPassRemapFactor` = (4/p^2 - 2, 1 (captured; its producer not read), 1/p^2, p^2/2) with p = RemapFactor 0.5
(`[143D7EED8]` = 4, `[143D7EE7C]` = 2): (14, 1, 4, 0.125), the captured words. The downsamples before it: `FUN_14172A660`, S1 at
2:1 (offset 0, weight 1).

## 6. The glare family: implementation log (thread « W10 famille glare », 2026-10-13)

**GL-1, the generic 2D post draw** (`native/renderer/post_draws.{h,cpp}`): the library's 2D programmes (pinned, found in the image)
with the original's three vertices (stride 16: (-1, 1) (0, 1), (-1, -3) (0, -1), (3, 1) (2, 1); stride 24 with TEXCOORD1 for the
vignette bright pass), the `$Globals` block in b0, tables of eight SRVs and eight samplers with the original's descriptions per draw
(D3D11 and D3D12 number filters and address modes alike; border transparent black, MaxLOD 0), opaque output at the target's extent.
**T13** (`post_draws_test`) replays the 24 glare draws of Reference1648 (m37) with their captured blocks, inputs and samplers: **0
differing texels** on the capture's adapter (WARP reported).

**GL-2, the plan** (`native/world/yebis_glare.{h,cpp}`): from the globals block 144961F70 (GlareQuality 0x48, exposure 0x98,
GlareLuminance 0xDC, GlareThreshold 0xE0, GlareNumLevels 0xE4, radius scale 0xF8, luminance gamma 0xFC, RemapFactor 0x244, sampling
threshold 0x304) and the extent, with the library's formulas read in the decompilation: the base size (CPostEffect::Initialize, the
0.99 ceiling), the source chain (2:1 while the half stays at least the base; INFERRED rule, m37 two, m50 one), the bright pass
(FUN_14174B830/FUN_141736FE0/FUN_141722480: Modulate (e, 1/F, t), remap (4/p^2 - 2, 1, 1/p^2, p^2/2) in that float order), the
pyramid (FUN_14174C160: R = 2^(1 - min(D, N)) C s, level radius rec[+8] R x sqrt 0.75 after the first, per channel through
(r lambda) / lambda_red; the tap index k of FUN_14173FDF0 against GlareSamplingThreshold; k = 2, 4, 6 take FUN_14173DB50/E6B0/F250
with 3, 7, 11 taps normalised over themselves, otherwise 2k - 1 taps (at most 15) of FUN_1416DC220's 15; weights
exp(-(j s)^2 / (2 sigma^2)) summed g1 x 2 + centre + g2 x 2 ..., offsets j / size, the eight first in the VS's cb0[34 + t], the
others in the PS's cb0[42 + t]), the levels capped at D - 2 + (int)(log2 baseW) (FUN_141748D50), the 2x surfaces of the levels whose
width is at most W >> 4 and of the last (FUN_141749790; offsets (1 / size) x 0.375, FUN_1416D6570), the composite (FUN_14174C9E0:
w_i = (1 / q^i) L, q = 1 for the R11G11B10 pyramid, L = (((F 0.035) shape8) luminance) shapeC; MadLumGammaT{N} with 1 / gamma or
MadT{N} when the gamma is 1) and the tone map's t1 (2x the base width capped at W/2, the height from the glare aspect; INFERRED from
m37 420x354 and m50 640x540). F = 64 (CRenderGlare +0x2BC x +0x2CC) and the GlareShape 1 record (1, 1, 0) are taken as observed at
every witness zone (the library does not run natively). **`yebis_glare_test`**: the plans equal the original's draw lists and every
block word their programmes read, **bit for bit at m37, m31, m50, m45** (Reference1649-1652, 4,812 checks), the vignette bright
pass's TEXCOORD1 equal to the captured vertices.

**GL-3, the stage** (`native/renderer/glare_stage.{h,cpp}`): the plan through PostDraws into surfaces the stage owns (RGBA16F source
and bright pass, R11G11B10 pyramid), recreated with a drain on a layout change; its output bound as the tone map's t1. **T14**
(`glare_stage_test`): from each zone's captured scene HDR, **94 of 94 surfaces** up to the composite equal the original's draws, 0
differing texels on the capture's adapter. Game side `yebis-glare.flag` (with `yebis-tonemap.flag`): row `native_yebis_glare`.
**GL-4, the tone map's colour matrix** (needed at m50): E69A0 composes, from the identity, the hue rotation (FUN_1416DDB90,
Haeberli's luminance-axis rotation, angle (h pi) / 180, ColorH != 0), the saturation (FUN_1416D1D90, ColorS != 1) and diag(v, v, v,
1) (ColorV != 1), and since I-0113 tranche 20 last the contrast of Contrust (FUN_1416CBC90, FUN_1416D20C0), through FUN_1416DD9B0; the block holds the transpose; `yebis_tonemap_test` checks the words at m37, m31, m50 (hue 1),
m45 bit for bit.

Live (`yebis;yebis-tonemap;yebis-glare;world-pictures`): **Native1653** m37 (24 draws), **1654** m31 (28), **1656** m50 (22, the hue
matrix word 840 equal to the capture), **1657** m45 (24): plan on, no fault, restoration ok; witness pictures `world-2400` (the bonfires
and lanterns glow, the m50 vignette). The shape table (Reference1658): GlareShape 0, the default file's (zones without their own
Bloom group), has a zero bloom luminance (+8 = +0xC = 0): the native path refuses it and leaves t1 black (INFERRED equal: the original's
composite weights would be 0); shapes 2-7 (star and streak records) are used by no zone file in the bank goldens and stay refused.

Open: the light shafts (draws 37-43: the mask, the radial blurs, MadT2 into the t1 source; at m45 a MadT1 copy) are not native, so t1
lacks them where the original draws them; the bloom gamma (+0x1B0) other than 1 and the tinted shapes are refused; F and the shape
records are observed constants.

## 7. The light shafts: preparation (2026-10-13, nothing implemented)

The m37 draw list of `i66-reference-1648` frame 1200 (rows `reference_yebis_draw`): 0-9 depth of field, **10 `tech_Alpha_R`**
(VS `vs_2D_OffsetTex2D1`, PS `825861A7BE1A`, t0 the depth R24G8 1280x720, into the RGBA16F scene target: its alpha becomes the
shaft mask the glare source chain carries down, YEBIS-TECHNIQUES §8.4 step 1), 11-13 motion blur, 14-36 glare (the native plan),
then the shafts, all at the base size 210x177 into R11G11B10:

| Draw | Technique | VS | PS sha | Inputs |
|---|---|---|---|---|
| 37 | `LightShaftMask_SM3` | `vs_2D_Tex2D3` | `D275F69939F1` | t0 the bright-pass output (RGBA16F 210x177, alpha = mask), t1 the film-grain noise (R8G8B8A8 256x256, `CTextureUtil+0x216E8`, created at `141719C3C`) |
| 38-41 | `TexCoordMadT1_S4` | `vs_2D_Tex2D4` | `CBC404544D10` | t0 the previous shaft surface (radial blur: the four tap coordinates come per vertex, so the vertices carry the sun's screen position) |
| 42 | `TexCoordMadT1_S3` | `vs_2D_Tex2D3` | `2B14DBEB53E9` | t0 the previous shaft surface |
| 43 | `MadT2` | `vs_2D_Tex2D1` | `19A65C0B1D48` | t0 the glare composite (draw 36), t1 the shafts |
| 44 | `MadT1_S4` up to 420x354 | `vs_2D_OffsetTex2D4` | `CBC404544D10` | the draw 43 result (natively today: the draw 36 result) |

At m31 and m50 the static notes (YEBIS-TECHNIQUES §8.4) give `TexCoordMadT1_S2` last and the shafts folded into the composite
(`MadLumGammaT2`) instead of `MadT2`: two composite variants to read in `FUN_1417538E0`/`FUN_1417511C0`. Builders to read:
`FUN_1417372A0` -> `FUN_1417290B0` (mask, constants at `+0xDC`), the radial blur's vertex coordinates (sun position from the
light-shaft globals, the 28-word block already bit-identical natively at YB-4b), the film-grain generator, and the Alpha_R block.
**Alpha_R's block** (draw 10, decoded; builder `FUN_141727DA0`, export `evidence/I-0068/queries/yebis/hal-impl/b/141727da0.c`):
the variable at `+0x108` gets two float4, `(s, s, s, s)` then `param_21` (captured (1, 1, 1, 1), words 616-623), the one at `+0x10C`
gets `(o, o, o, o)` (words 744-747), with `s = p13[0] x (param_12 ? [param_12+8] : [143D7EE10])` and `o = p13[1] + (param_12 ?
[param_12+0xC] : 0) x p13[0]`, `p13` defaulting to (1, 0). m37: s = 38.499557, o = -37.499866, so alpha = saturate(38.5 d - 37.5):
0 below depth 0.974, 1 at the far plane (the sky carries the shafts). The other non-zero words of that block are earlier draws'
leftovers the PS does not read. To read: the caller `FUN_1416DF6E0` (where p13 and param_12 come from, presumably the light-shaft
globals of E4700).
Plan: LS-1 Alpha_R natively (depth -> scene alpha) checked on draw 10; LS-2 the plan (mask, radials, last blur, composite variant)
against the captured blocks and vertices at m37, m31, m50 (captures 1649-1651 if they hold the shaft draws, else new ones); LS-3 the
stage through PostDraws (vertex layouts with 3 and 4 texture coordinates) with the film-grain texture; T15 byte for byte; live.

**The glare family's pre-merge review (§6; Opus 5.5, D-0176, 2026-10-13):** fixed before the merge: the tone map's t1 view is keyed by the glare surface and
the stage's layout generation, and the renderer drains when that pair changes (a recreated surface at a reused address, or a view
rewritten while an earlier frame reads it); the stage allocates a draw's vertices before its barrier and reports exceptions through
its reason; NeedsLayout also compares targets, samplers and vertex layouts; refused plans are cached; the plan carries its extent and
is drawn only on a source of that extent; ColorV != 1 stayed refused (no witness; served by I-0113 tranche 20 from its setter read statically); the zone tests fail on a missing named capture;
the build script back to LF. Not done: the glare passes are not declared to the frame-graph shadow. Live check **Native1659** (m37):
plan on, no fault.

## 8. The light shafts: implementation log (2026-10-14)

**LS-1 (Alpha_R) done.** Read: FUN_1416DF6E0 (the caller), FUN_1416F4AF0 (the library's depth conversion between its spaces: 0
the device depth, 1 through near, far and +0xC48, 2 through +0xC38/+0xC3C, 3 through +0xC40/+0xC44 and FUN_1416F52A0; the inverse
when the lower space is the target), FUN_1416E9050 (the conversion words), FUN_1416C9450 (near and far, set by E69A0 from its camera
record +0x54/+0x58). Witness **Reference1660** (m37; `CaptureYebisShaftState` at the Alpha_R and LightShaftMask draws: the library
object, its depth source [+0x430], the DoF holder [+0x110]; the draws capture now logs each draw's blend and keeps depth inputs):
near 0.08, far 10000, the depth-of-field branch on (S+0xC33), so +0xC48 = S+0x6C0 = the focus distance (globals 144962120, the
native feed's word), +0xC38..+0xC44 = the DoF holder's +0x1B4..+0x1C0 (1, 0, 1, -0), the source space 0, the source scale and bias
(1, -0). Then s = ((far - near) F) / (far near) and o = 1 - F / near (F the focus distance): at m37 38.499252 and -37.499557, the
1648 values differing only by F. The blend: write mask 8 (alpha only), no blend. `world/yebis_shafts` computes the block (s at word
616, the colour (r, r, r, 1) at 620, o at 744); `yebis_shafts_test` equals the captured words bit for bit at **m37, m31, m50, m45**
(Reference1660-1663; 85 checks; m50's focus 1.6671 gives s 20.838591). `PostDraws` takes a write mask; T13 replays draw 10 at the
four zones from the captured depth (its R32_FLOAT word): **0 differing texels** on the capture's adapter. Not served: the branch
without depth of field (+0xC48 the adjusted near of FUN_1416E9050) and a second alpha source (`tech_MadAlphaColorT2_*`), no witness.
m45 draws Alpha_R but not the shafts (MadT6 then a MadT1 copy): the shaft condition is LS-2's.
Next: LS-2, the plan (LightShaftMask's block at +0xDC, the four radial blurs with their per-vertex coordinates, the last blur, MadT2
or MadLumGammaT2) against the captured draws 37-44 (m37), 41-48 (m31), 35-42 (m50); then LS-3, the stage through PostDraws (vertex
layouts with 3 and 4 texture coordinates, the film-grain texture) and the native Alpha_R draw into the scene target before the glare.

**LS-2 reading (in progress).** Witnesses **Reference1664** (m37), **1665** (m31), **1666** (m50): `CaptureYebisShaftState` also keeps,
at the mask draw, CRenderGlare ([library+0x108], 0x400), its shaft records ([+0x1C8], 0x50 each, [+0x1C0] = 1 at the three zones)
and the shaft surfaces (the mask source [+0x2A0], the output [+0x2B0], the ping-pong pair [[+0x248]+0x48][+0xE4], +1; all 210x177
at m37 and m31, 392x331 at m50). The chain (FUN_1417538E0 -> FUN_141750F70, per record FUN_141750620, then FUN_141735980 when
there are several records):
- the record (0x50, copied from the library's slot S+0x6B0 + i x 0x58): +0x00..+0x0C the colour (ColorScale x the scale word, then
  its maximum), +0x10 the 0.95 word, +0x14, +0x18, +0x1C, +0x20, +0x24 (non-zero: the FUN_141737480 composite), +0x28, +0x2C, +0x30,
  +0x34..+0x40 a second colour, +0x44/+0x48 the sun's screen position, +0x4C its depth. The m37 words equal the light-shaft globals
  1449620A0.. (w4 10.006 the scale, w5 4 -> +0x1C, w6 0.95 -> +0x10, w9 20 -> +0x20, w11 5 -> +0x18, w15 0.2 -> +0x30, w16 0.01
  -> +0x28; elevation 17 and azimuth -164 at 1449620D0/D4 give the sun); the game side is E69A0 (sun direction from the two angles
  with FUN_141FD1A40/FUN_141FD1740, projected with the camera's matrices; colours from its CPostEffect +0x21E0..+0x220C; setters
  FUN_1416C97F0 -> FUN_1416F3870, FUN_1416C9830 (+0x24..+0x40), FUN_1416C9870 (+0x44/+0x48)).
- the mask (FUN_1417372A0 -> FUN_1417290B0): `tech_LightShaftMask_SM3` (handle +0x6F68, else +0x6F64), t0 the source, t1 the film
  grain ([+0x216E8]); block +0xDC = (rec+0x20, rec+0x18, rec+0x14, rec+0x10) as written from the call, +0x11C = ([143D7EFF8],
  rec+0x4C, sun x + sun y, 0); the quad from FUN_14171C540 with texture coordinates from the screen aspect, FUN_141FD59E0(p17 x
  0.5) and p18 x [143D7ED7C], centred on the sun.
- the radial blurs: the pass count n = ceil(log(L x eed8) / log(eed8)) capped by [142B63164] (L = log(base) / log(0.5) / (C5C /
  FUN_141751CB0 / rec+0x1C), base [143D7ED10] or [143D7ED1C] by the source's format), the last pass's taps clamp(ceil(...), 1..4),
  the tap weight pow(+0x138 x ed9c, 1/n); per pass FUN_14171E8E0 with four (or the last pass's) taps: per-vertex coordinates
  lerped from the sun towards the corners by 1 / ((i x step + h) / h), weights x the record's colour and the glare factor on the
  last pass (+0x114 x [142899374] x shape+8 x +0x118 x ed9c x eed8), ping-pong between the pair, the last into [+0x2B0].
  FUN_141FDD060 log, FUN_141FD2700 ceil, FUN_141FD3430 pow (CRT).
Next: transcribe FUN_14171D490/FUN_14171C540 (vertices) and the plan, test against the captured draws 37-44 (blocks and vertices),
then the record from E69A0 (camera capture needed).

**LS-2 plan done (record given).** `world/yebis_shafts` `BuildShaft`: from the record, the glare object's +0xB0, +0x114, +0x118,
+0x134, +0x138 (= tan(+0x134 / 2), the library's S+0x570 from E69A0's field of view; +0x134 = S+0x56C), +0x148 (the exposure,
144962008), the shape's +8 and the surfaces' extents (texel scale 1/extent, bias 0, checked on the capture), the mask (blocks c15 =
(rec+0x14, rec+0x10, exposure / factor, rec+0x4C) and c10 = (256, rec+0x18, sun y + sun x, 0); texture rectangles (0,0)-(1,1),
(0,0)-(4 W/H, 4) for the film grain, and the sun-centred one from tan(fov / 2) x rec+0x20 / 8) and the radial blurs (n =
ceil(log4(L)) <= 8 with L = log(0.001) / log(0.5) / s, s = (0.0625 / (1 / (2 tan) x 0.003125 base)) / rec+0x1C; the last pass's
taps clamp(ceil(4 L / 4^n), 1..4); weights pow(0.5, k s 4^p) x pow(tan / 4, 1 / n) (x 1/4 on the first pass), the last pass x the
colour (rec 0..3, rgb x rec+0x34.. when not all 1) x (factor x 0.035 x shape+8 x luminance x 1/4 x 4); tap k's rectangle from the
sun towards the corners at 1 / ((k 4^p + H) / H); FUN_141718C00's covering triangle (x' = (x1 - x0) 2 + x0, y' = (y0 - y1) 2 +
y1)). `yebis_shafts_test` equals the captured draws from the mask on at **m37, m31, m50** (Reference1664-1666): techniques
(5 passes; the last S3 at m37, S2 at m31 and m50), every word the programmes read and every vertex float, **860 checks, 0
failures**. Not served: rec+0x24 != 0 (FUN_141737480), an 8-bit shaft surface (base 1/255), several records (FUN_141735980).
- The film grain (t1 of the mask) is the executable's 256x256 RGBA8 block at RVA 0x2B68A40 (FUN_141719770 uploads it with
  FUN_141711350): SHA-256 `11C7FAD64F41EB04E249BEE63F37C2F7CBCBD340AED9D0EAC8EC3713C031354C`, equal to the captured t1 at the four
  runs; natively it is read from the executable like the display blobs.
- The composite (FUN_1417511C0): MadT{n} of [glare+0x238] (the glare composite), the optional lens terms, and [glare+0x2B0] (the
  shafts) when +0x1C0 > 0, each weight (1, 1, 1, 1), into [glare+0x280]; the 2x draw then reads it (MadT2 at the three zones; m45's
  MadT1 is the same composite with one input, which the native glare plan already folds).
- LS-2b, to do: the record natively from E69A0 (1400E7416..1400E79A8: the sun direction from 1449620D0/D4 x [143D7ED40] (+
  [143D7EEC0]) through sin/cos FUN_141FD1A40/FUN_141FD1740, the SSE matrix products with the camera (FUN_14004BCA0,
  FUN_14004B070), the projection to the screen, the visibility factor, the colours from CPostEffect +0x21E0..+0x220C); oracle:
  a Reference capture of E69A0's inputs (camera records, post object) and the library slot S+0x6B0.

**LS-2b (in progress).** Witnesses **Reference1667** (m37), **1668** (m31), **1669** (m50): the draws capture now also hooks E69A0
in the captured frame (`ObserveYebisDriver`: the post object, the camera records [engine+0x210] and the previous one, the globals
before the call; the library's slots [S+0x6B0] after it). The record CRenderGlare draws with is the slot's words 0-18 and the sun's
depth converted with Alpha_R's DepthConversion(6, 2) (slot 0.99996614 -> record 0.9983864 at m37): `ShaftRecordFromSlot`, bit for
bit at the three zones (`yebis_shafts_test`, 1758 checks with the plan on the three new runs). Remaining: E69A0 -> slot (the sun
direction from 1449620D0/D4, the camera matrices through FUN_14004BCA0 (a 4x4 inverse with its determinant) and FUN_14004B070, the
projection 1400E7632..1400E76D0 into [RBP+0xB0], the visibility term clamped to [0, 1] at 1400E77A7, the setters' arguments
from CPostEffect +0x21E0..+0x220C and the globals words 0x52..0x5D); to transcribe with the original's SSE lane order.

**LS-2b done; LS-3 done (thread « W10 rayons de lumière », 2026-10-14).**
- E69A0's shaft section natively (`DriverSlot`, the SSE lanes emulated instruction by instruction from 1400E741B..1400E79A8;
  FUN_14004BCA0 through `ds3prod::InverseMatrix`, FUN_14004B070 through `ds3light::CameraProjection4B070`; sin, cos, sqrt as the
  CRT's): the library's slot bit for bit at m37, m31 (sun off screen, visibility 0.16) and m50 (Reference1667-1669). The slot ->
  record copy (words 0-18, the depth through DepthConversion(6, 2)) bit for bit. Reference1670 (m45): the slot's colour is 0 (no
  shaft): the library keeps no record (INFERRED rule: a zero colour scale gives no record), Alpha_R alone.
- The film grain's 256x256 block is the executable's (RVA 0x2B68A40) and its sampler (trilinear, wrap, MaxLOD FLT_MAX) reads the
  CPU mip chain FUN_141711350 builds (2x2 box, the 4x4 dither table from the 4-texel level up, both channels sets): with
  `FilmGrainLevels` the m50 mask, which differed by 31064 texels with level 0 alone, is exact.
- `PostDraws`: 1-4 texture-coordinate pairs, a sampler's MaxLOD, every mip viewed. T13 replays the seven shaft draws at m37, m31,
  m50 exactly; the pins gain `tech_TexCoordMadT1_S3` and `tech_MadT2` (full SHA-256 found in the executable).
- The glare plan carries the shafts (`AlphaRDraw`, `AppendShafts`: four shaft surfaces at the base extent, MadT2 at the
  composite's, the 2x draw then reads MadT2); the merged plan equals the captured sequence from the first source downsample to the
  2x draw at six runs. `GlareStage` draws Alpha_R (`RecordPre`, into the scene's alpha from the depth's plane 0, write mask 8), the
  film grain (uploaded once with its mips), the shafts' own vertices; T14 now draws the merged chain from each run's captured scene
  HDR: **180 of 180 surfaces equal** (the four glare zones and the three shaft runs; the ping-pong surfaces compared at their last
  writer).
- Native: `yebis-shafts.flag` (with `yebis-glare.flag`): `AppendNativeShafts` builds E69A0's camera record from the scene camera
  (the engine record's tiles 1, INFERRED: Native1671 refused 'camera lens' on the scene output's own tile count), the YEBIS snapshot
  carries E4700's colours (CPostEffect +0x21E0..+0x220F); the scene pass declares `alpha_r` (the lit HDR and the depth's plane 0) to
  the frame graph. Live **Native1672** (m37): `native_yebis_shafts` served (Alpha_R 1, one shaft, 31 draws, 11 programmes
  resolved), 6351 native frames, no fault, files restored. Not done: the shaft passes are not declared to the frame-graph shadow
  (as the glare); several shaft records (FUN_141735980), rec+0x24 (FUN_141737480) and the non-DoF branch of Alpha_R stay refused.
- **Pre-merge review (fresh Opus 5.5, D-0176, 2026-10-14)**, applied: `alpha_r` enters the frame graph's plan key and is decided
  before it (with the tone map's readiness), its transitions now precede its pass declaration (Native1673: 1343
  `graph_barrier_mismatch` rows); degenerate radial plans (no pass, a one-tap last pass with `tech_TexCoordMadT1_S1` unpinned)
  refused before the integer conversions; own vertices chosen by their presence; zero-colour shafts kept with a zero last weight (a
  stable layout as the sun crosses the visibility edge); the native +0x134/+0x138 checked from the camera's field of view. Found
  live after the review: the stage's stored shape lost the own-vertices marker, a relayout and a drain every frame (Native1673-1674,
  23 frames/s); fixed, and T14 now requires no relayout after drawing a plan. Live **Native1675** (m37): shafts served, 6347 native
  frames, 43.8 frames/s (the glare alone, Native1659: 44.6), no graph mismatch, no fault. Not done: a T14 case for `RecordPre`
  with a planar depth (exercised live only).

## 9. The depth of field: preparation (2026-10-14, nothing implemented)

The m37 draw list of `i66-reference-1667` frame 1200 (rows `reference_yebis_draw`, the blocks' non-zero words; the same ten draws
open every zone, YEBIS-TECHNIQUES §4):

| Draw | Technique | Target | Inputs | Words (m37) |
|---|---|---|---|---|
| 0 | `DepthOfField_BlurFactorAlphaFromDepthTextureTypeCastLut_RVALUE` | 1280x720 RGBA16F | t0 depth (21), t1 the previous frame's final glare (420x354 R11G11B10), t2 the DoF table 2048x2 R32F (41) | 0-2 (0.9995117, 0.00024414, 1), 68 238.6266, 70 0.004190648, 71 -0, 200 0.25 |
| 1 | `MadT1_S2` | 784x294 | the draw-0 target | taps 137-165, weights 616-623 (0.5) |
| 2 | `MadT1_S1` | 392x147 | draw 1 | taps, 616 1 |
| 3, 5, 7 | `DepthOfField_FrontBlurBlendFactor` | 784x294, 784x294, 392x147 | the layer | 616-619 (1, 1, 1, a), 623 b: (-0.399, -0.255), (-0.809, -2.897), (-0.290, -1.605) |
| 4, 6, 8 | `MadT1_S5`, `MadT1_S9`, `MadT1_S9` | the layer | the layer | taps 140-201, weights 616-635 |
| 9 | `DepthOfField_CompositeBlurLevelsFrontBlur_TS1_3` (a = 1, not TS0_3) | 1280x720 RGBA16F | the scene and the three layers | 76-77, 80-81, 84-85; stride 24 (TEXCOORD1) |

Readings already in the repository: YEBIS-TECHNIQUES §8.1-§8.3 (the pre-pass, the composite's mode 2, the blur tap rule); the
native DoF record (`yebis_blocks` `DofBlock`, 34 words, bit for bit since YB-4b.1); `CaptureYebisShaftState` keeps the DoF holder
([library+0x110], 0x400 bytes: +0x164 the enable, +0x180, +0x188 the focus distance, +0x1B4..+0x1C0 the conversion words) and the
library at the Alpha_R draw. Next: decompile draw 0's builder (the table `0x147F4`, its 2048x2 lookup and the words 68-71), the
layer records of `FUN_14175EA40`/`FUN_14175C7E0` (a and b per layer), the composite `FUN_1417266B0` (words 76-85), then capture
the DoF holder and the table at draw 0 per zone (m37, m31, m50, m45), as for the shafts.
Call map (FUN_1416E5BC0, the library's frame): the DoF stage is `FUN_1416E2910`, called when S+0xC33 (the depth-of-field branch
of FUN_1416E9050, the same flag Alpha_R's constants depend on), right before Alpha_R (FUN_1416DF6E0); draw 0 reads the previous
frame's final glare (YEBIS-NATIVE §3.2 line 300, `ApplyEffects_DepthFactorAlpha`): a history input. Start the reading there.
`FUN_1416E2910` (decompiled in this preparation): returns when S+0xC33 is clear; `FUN_1416E1680(S)` (INFERRED draw 0, the depth
factor), `FUN_1417629F0([S+0x110], S+0x53C or 0xB when S+0x6D8 != 1, -1)`, the viewport scale (S+0x6F4, S+0x6F8 / S+0x6D8),
the DoF holder's +0x384 = S+0x6E8 / (S+0x454 ? S+0x6E8 x S+0x69C : S+0x698), then `FUN_141767D80([S+0x110], surface [S+0x4B4],
[S+0x4B8], [S+0x4BC], 1)` (the layers and the composite, INFERRED) and S+0x51A = 1.
`FUN_141767D80` (the DoF holder's frame): `FUN_141726280(HAL, [+0x1B4])`, `FUN_14175EB30` when [+0x210] (INFERRED the history
of draw 0's previous-frame glare), `FUN_141756580`, `FUN_14175ED50(source)` (INFERRED draws 0-2), `FUN_14175EA40` (the aperture
filter of every level: draws 3-8, YEBIS-TECHNIQUES §8.1), `FUN_14175F850(source, …)` (the composite: draw 9, §8.2).
`FUN_1417629F0` stores the level count words +0x1F0/+0x1F8/+0x1F4. `FUN_1416E1680` reads the auto-focus result back
(FUN_14176A060/A070/A840: the read-back the HAL owes, YB-8), not a draw.

**DF-0 (the mechanics), 2026-10-14.** T13 replays the ten DoF draws (0-9) with their captured inputs, blocks, samplers, blends and
vertices at m37, m31, m50 (Reference1667-1669): **0 differing texels** on the capture's adapter (the depth factor's 2048x2 R32F
table read as format 41, its depth as the R32_FLOAT word; the composite's TEXCOORD1 vertices given). The pins gain
`tech_DepthOfField_BlurFactorAlphaFromDepthTextureTypeCastLut_RVALUE` (the full SHA-256 found in the executable). Next DF-1: the
constants natively (draw 0's words 0-2, 68-71, 200 and its table; the source taps; the layers' (a, b) and blurs; the composite's
words 76-85), from FUN_14175ED50, FUN_14175EA40 and FUN_14175F850.

**DF-1 reading (in progress).** The DoF holder H = [S+0x110] (CDepthOfField):
- `FUN_141726280(HAL, H+0x1B4)`: a block at handle +0xE8 = (p, 1 - p, 1 / p, ((1 - p) x [143D7F0C8]) / p), p = H+0x1B4 (1 at m37).
- `FUN_14175EB30` (when H+0x210, the depth source): draw 0 through `FUN_141727980(HAL, rect 0, 0, W, H of [H+0x200], tex rect
  from [H+0x200]'s scale and bias, near = H+0x1E8 clamped to [H+0x168, H+0x16C], far = H+0x1EC (H+0x16C when larger or <= 0),
  focus H+0x188, [H+0x210], H+0x154, 0, 0, H+0x218, &H+0x1B4, 0, 0, 0, 0, 0.5, colour (1, 1, 1, 1), 0)` with the colour write
  mask 8 around it (FUN_141708DA0 8 then 0xF): the blur factor into the target's alpha.
- `FUN_141756580`: per layer (H+0xCC of them, records [H+0x470] + i x 0x108): the near/far ramps (+0x30 the layer's size word,
  the previous layer's -0xD8; flags H+0x140 bits 1 and 2 with [142B658B0], [1429D9B10], [142B65898], [142944890], [143D7ED00],
  0.25), then +0xC0..+0xDC (1 / span, -(start / span), x H+0x1B0), +0xE8/+0xEC (H+0x1B0 x 1 x (float)rec+4 / rec+0x20) and
  +0xF0/+0xF4 (H+0x1BC/H+0x1C0 when H+0x154 == 1): the aperture filter's (a, b) words.
Next: FUN_141727980 (draw 0's words), FUN_14175ED50 (draws 1-2), then capture H (0x600), its layer records and surfaces at draw 0.
Witnesses **Reference1676** (m37), **1677** (m31), **1678** (m50): `CaptureYebisDofState` at draw 0 (the holder 0x800, seven layer
records each, the surfaces [H+0x200], [H+0x210]); the draws capture of the same frame.

**DF-1 state per zone (handoff, 2026-10-14).** Captures in the Evidence Store, `I-0066/artifacts/i66-reference-1676-m37-dofstate`,
`-1677-m31-dofstate`, `-1678-m50-dofstate` (`reference-dof-holder.raw` 0x800, `reference-dof-layers.raw` 7 x 0x108, the surfaces,
the draws of the same frame). At m37: H+0xCC 7 layers, +0x140 = 2 (bit 2 only at all three zones; the bit 4 path is transcribed
but unwitnessed), +0x154 0, +0x168 0.08, +0x16C 10000, +0x188 3.0800052, +0x1B0 0.004190526, +0x1B4 1, +0x1BC 1, +0x1D8 3,
+0x1DC 1; layers 784x294, 784x294, 392x147, 392x147, 196x74, 98x37, 49x19 (+0x20 3.15, 5.25, 5.25, 9.45, ...; +0x30 0.0107143,
0.0178571, 0.0357143, 0.0642857, 0.1277027, 0.2689189, 0.5368421). The composite words 76-85 and the aperture words 619/623
match the layers' D0/C4 and D8/CC (sign).
**`native/world/yebis_dof`** (`DofLayerRamps`, FUN_141756580 in the original single-precision order, constants 1, 0.25, 0.0001,
1.15, 0.61, 0.51, 0.98): +0xC0..+0xDC, +0xE8..+0xF4 of the 7 layers equal bit for bit at m37, m31 and m50 (`native/frame/
yebis_dof_test.cpp`, args = the three dofstate directories; built by hand with cl, not yet wired into Build-GameBootstrap).
Next: wire the test, FUN_141727980 (draw 0 words 0-2, 68-71, 200), FUN_14175ED50 (draws 1-2), the aperture (616-619, 623) and
composite (76-85) words from the layer records against draws 3, 5, 7, 9; then the native DoF plan, stage, live, review.

**DF-1 done; DF-2, DF-3 done; DF-4 wired (thread « Profondeur de champ native », 2026-10-14).** Decompiled exports in
`evidence/I-0068/queries/yebis/dof*` (dof .. dof27, the DXBC disassemblies in `dof-dxbc`).
- Draw 0 is not FUN_14175EB30 (the holder's +0x210 is 0): it is FUN_1416E0040 -> FUN_1416DF6E0 with the DoF table (S+0x68) ->
  FUN_141727DA0 -> FUN_1417274F0 (table +0x147F4, the R value): words 0-3 = (S+0x448 a', a' S+0x44C + b', 1, 0) with (a, b) =
  DepthConversion(6, S+0x74) scaled to the table's texel centres ((W - 1) / W, + 0.5 / W); words 68-71 = (f, f', 1/f, -(f'/f)) from
  DepthConversion(S+0x78 = 3, 1) through S+0xC38/C3C; word 200 = (S+0x80 + 0.5) / H. Its pixel programme reads t0 (depth) and t2
  (the table) only: alpha = word 68 x table(depth x word 0 + word 1, word 200) + word 69 (the t1 bound is stale, unread). The table
  is GX's CustomDofLutCopy (cbComputeCustomDof, `ds3light::CustomDofTable`), written at a frame's end for the next.
- FUN_14175ED50: the counts (front +0x1D8, back +0x1DC from the ramps against the focus and the near/far bounds, capped by
  +0x1C4/+0x1C8 = globals +0x1D0/+0x1D4 through FUN_1416CB4A0) and the source chain (FUN_14172A660, taps by FUN_1416D6570's
  bilinear rule n = ceil((src x 0.5) / dst) per axis, offsets FUN_1416D63B0, weights 1/n); FUN_14175EA40 -> FUN_14175C7E0: the front
  blend factor (FUN_141727350: words 619 = -D8, 623 = DC, point mirror) then the level blur FUN_14175BB40 kind 0 (taps 3:
  FUN_14173C3F0 -> FUN_1416D7300, five bilinear taps of the 3x3 kernel (0.4, 1, 0.4 / 1, 1, 1 / 0.4, 1, 0.4); taps 5: FUN_1416D7540,
  nine taps at 1.4444 and 1.2982 texels, weights 1, 1.8, 2.0306249 over 1 + 4 x 1.8 + 3 x 2.0306249 + [142B4D118]; bilinear mirror);
  FUN_14175F850 mode 2 -> FUN_1417266B0: TS{back}_{front}, words 72 + 4k = (0, 0) then each level's (D0, D4), TEXCOORD1 = TEXCOORD0.
  The composite writes a second scene-sized surface; Alpha_R, the glare's source and the tone map read that surface.
- DF-1 `yebis::BuildDof` (plan from the holder state): technique, targets, inputs, samplers and every word read equal the captured
  draws 0-9 at m37, m31, m50 (Reference1676-1678).
- DF-2 the holder natively: the creation (FUN_1416EBF80's DoF block: base = (int)(W x row scale x sqrt((A / k) / A) + 0.99),
  height (int)(base / (A / k) + 0.499999), k = CPostEffect +0x388; the quality table 14469B510 by the DoF quality, globals word 0x4C;
  FUN_141755ED0's levels from the preset table (preset 2 at the three zones), FUN_141754B60/FUN_141754D30's taps 3, 5, 9 and radius
  x 1.05), the lens (E69A0's FUN_1416C98D0: S+0x6C4 = diagonal x 0.001 / sqrt(aspect^2 + 1), S+0x6CC = E5110's factor (1 / (WH /
  2073600)) q/12 + (1 - q/12) x f-number; FUN_141763320 -> FUN_141762CF0, aperture type 0: focal, aperture, CoC, +0x1B0 = CoC /
  film), the table (CustomDofTable, both rows): `yebis_dof_test` 2316 checks, 0 failures, from the captured camera record and
  globals only (holder fields, level records, plan, table).
- DF-3 T15 `dof_stage_test`: the glare stage draws the native plan (the depth factor through RecordPre with the depth's plane 0 and
  the native table, input -5; per-slot address modes) from the captured scene: **30 of 30 surfaces equal the original's** (the
  scene's alpha and draws 1-9) at the three zones on the capture's adapter.
- DF-4: `yebis-dof.flag` (with the glare): the frame builds the native DoF (setup cached by extent and quality; the previous frame's
  table, as the original), row `native_yebis_dof`; the renderer draws it before Alpha_R (passes `dof_factor`, `dof`, `dof_copy`;
  plan key bit 14) and copies the DoF scene back into the lit HDR. Refused, with a row: no DoF branch, holder flags other than 2,
  USNORM composite, a level smaller than its surface, blurs other than 3 or 5 taps, an unpinned composite variant.
- Live: **Native1679** (m37) and **Native1682** (m50): `native_yebis_dof` served (10 draws, base 784x294, quality 3 and 5), no fault,
  no graph mismatch; **Native1681** (m37, `-Measure -NoDebugLayer`: normal play) **60 frames/s** (59.9-60.1, median 16.7 ms, 920 draws)
  with the tone map, the glare, the shafts and the DoF native (the 44 frames/s of the witness runs are their instrumentation).
- **Pre-merge review (fresh Opus 5.5, D-0176, 2026-10-14)**, applied: an unpinned technique logged once (the DoF plan is built every
  frame); the previous frame's table dropped after a refusal; the table extent guarded (g_Coc[2048]); a relayout row (the first eight,
  then every 300th) and an extent mismatch told once. Open (no witness of a cost yet): the level counts follow the lens, so a change
  of the front or back count relays out the DoF stage (a drain); allocating the setup's seven levels once would remove it. The CRT
  tan/atan stand in for the game's FUN_141FD59E0/FUN_141FDC374: bit for bit on this machine's CPU only.

## 10. Motion blur (MB-0 thread « Profondeur de champ native », MB-1 to MB-4 thread « Flou de mouvement natif », 2026-10-14)

Witnesses Reference1676-1678 (m37, m31, m50) hold the motion blur's three draws after Alpha_R: 11
`tech_MotionBlurMakeCameraVelocityOverwrite_R` (vs_2D_Tex2D3, 640x360 RGBA16F, t0 the depth, t1 the engine's object velocity 640x360
RGBA8; words 0-2, 20-21, 60-63, 616-627, 896-911 the transform), 12 and 13 `tech_MotionBlurVelocityDirDepthInnerscreen_SM3_T2_S5`
(1280x720, t0 the scene, t1 the internal velocity; two passes: R = 2 at these zones). Done: the two techniques pinned
(`yebis_pins.h`, SHA-256 of yebis-techniques.tsv); `PostDraws::GlobalsBytes` and the glare draw block raised to 4096 (the velocity
programme reads `$Globals` up to float4 227); T13 (`post_draws_test`) replays the depth (format 21) as the renderer holds it, a planar
R32G8X24 depth-stencil resource viewed R32_FLOAT_X8X24. Result: **the two blur draws equal the original's, 0 differing texels, at the
three zones**; the velocity draw differs by 1 half-float unit in its red channel only, at 30 (m37), 5149 (m31), 13356 (m50) of 230400
texels (green, blue, alpha exact). Not explained yet: the depth view does not change it (R32_FLOAT and the planar view give the same
count), the vertices, samplers and block are the captured ones; candidates: the D3D11 and D3D12 paths compiling the programme's
dp4/div chain differently, or an input the capture does not hold as the draw read it. `POST_DRAWS_WHERE=1` prints the first texels.
Next: settle MB-0's red channel, then MB-1 the constants natively (YEBIS-MOTIONBLUR-NATIVE §1-§4: E69A0's setters, the planner, the
transform from the two cameras), the engine's object velocity (the native velocity target, D-0162) and the stage.

**MB-0's red channel (thread « Flou de mouvement natif », 2026-10-14).** INFERRED: the render target write converts float32 to half by
truncation (a CPU emulation of the velocity programme on the captured inputs matches 96% of m37's texels with round-toward-zero, 48%
with round-to-nearest). The differing texels lie on a half boundary, and an emulation with unfused `mad` (the dp4 as a fused chain,
the divisions as reciprocal products) gives the original's values at the sampled texels, the same emulation with fused `mad` gives
ours: INFERRED, the D3D11 and D3D12 driver compilations of the same DXBC fuse the `mad` chain differently. Not corrected (the
programme is the original's); T16 below shows it does not reach the blur's output at the three zones.

**MB-1 done: the plan natively** (`world/yebis_motionblur`, decompiled exports in `evidence/I-0068/queries/yebis/mb1`). The stage
FUN_1416E21E0 (mode 2): the camera velocity FUN_1416E0160 (once a frame, S+0xC7D) -> FUN_141730F30 -> FUN_141724410; the blur
FUN_1417301D0 -> FUN_14172FBD0 (planner) -> FUN_141724840 per pass.
- Projections: FUN_1416D0F00(out, 0, 1, x0, x1, y0, y1, near, far) from S+0x564/+0x568 (both projections), tan S+0x570 (previous
  +0x588), aspect S+0x328, lens offsets +0x574/+0x578 (+0x58C/+0x590). FUN_1416D12C0 gives the PerspectiveFactor (near', far',
  far' near', far' - near') from P10/P14 (far' 10016.33 at near 0.08, far 10000: the float projection's cancellation).
- The previous-clip transform: the two poses (S+0x594, S+0x614 = the current and previous camera records' +0x00, checked against
  `reference-driver-camera.raw`) with their first three rows normalised (FUN_1416DD810; the decompile drops the stores, the
  assembly normalises all six rows), T = inv(inv(Pc) Ac inv(Ap) Pp) through FUN_1416FE370 (Gauss-Jordan with partial pivoting on a
  row's columns, transcribed operation for operation) and FUN_1416DD9B0; am44_TransformMatrix[0] holds its transpose (words 896-911).
- The velocity block: words 0-3 (1, 0, 1, 0), 60-63 the PerspectiveFactor, 616-627 p16 = ((0.5 / s.x) A, -0.5 / s.y) with
  A = (1 / P00) / (1 / P11), p17 = (0.5, 0.5) (S+0xA10 = 1), the object depth map FUN_1416F4AF0(source, 0) = (1, 0), and their
  inverses.
- The planner FUN_1416D7080: L = H x MaxBlurLength, N from NumBaseSamples (S+0x21694 = S+0x9D8, FUN_1416E42B0) and the parity
  (S+0x21698 = 2 - Bidirectional), R capped by FUN_1416EADA0 (S+0x340 2: MaxRecurrences); witnessed (N, R, n, step / L) = (5, 2,
  5, 1/24) at the three zones (mapbase-like words: BlurTimeRatio 0.2, MaxRecurrences 2).
- The blur block per pass: weights 1/n and their sequential sum, O0 = (s k e.x, s k e.x, s k e.y, ...) with e = (2, -1), O1 = p12 x
  O0, (p12, 2/H, 0) with p12 = (step/L) x (1/A, 1) times N per pass, (S+0xA00, (1 / d.x) S+0xA04) with d = FUN_1416F4AF0(2, 0) (the
  scene alpha's depth map, Alpha_R's inverse: -0.385 at m37), the rectangle (2, 2, -1, -1).
`yebis_motionblur_test`: from the library at the Alpha_R draw (Reference1676-1678) every word the programmes read, the techniques,
targets, inputs, samplers and the velocity draw's vertices equal the captured draws 11-13: **521 checks, 0 failures**. The three
captures hold a still camera (current = previous pose): a moving camera's transform is not witnessed.

**MB-2 done: the stage.** The glare stage draws the plan (inputs -4 the depth's plane 0, -6 the object velocity; the passes ping-pong
between two scene-sized surfaces, the last one's copied back into the scene by the renderer). **T16** (`mb_stage_test`): from the
captured depth, object velocity and scene, our velocity surface keeps MB-0's half-unit red texels (30, 5149, 13356) and **both blur
passes equal the original's, 0 differing texels, at m37, m31, m50** (the blur passes pixels below its 2/H threshold unchanged; the
captures' still camera keeps most of the image below it).

**MB-3/MB-4: the object velocity and the wiring.** The renderer reduces its full-size R16G16B16A16_UNORM velocity target (D-0162) to
the engine's half-size R8G8B8A8_UNORM object velocity (each half texel the 2x2 block's texel of largest alpha, ties by the smallest
blue; the output merger quantises as the original's rows are). The GX dilation is not drawn: its shipped length (MotionBlurScale x
MaxBlurLength x BlurTimeRatio texels) keeps each texel and its zero shift (+0.5/255 - 3e-7) leaves every 8-bit value (STATIC
arithmetic; its sampler not witnessed). Game side `yebis-motionblur.flag` (with the glare and the velocity input): E69A0's setters
from the globals block's MotionBlur words (+0x1DC..+0x204, +0x2AC/+0x2B0, +0x350), the camera and its previous pose by the velocity's
cut rule, S+0x340 2, S+0xA00 0.15, S+0xA04 -0.01, S+0xA08 0 (INFERRED constants, observed at the three zones), the internal velocity
(int)(W x 0.5 + 0.99) x (int)(w / A + 0.499999); row `native_yebis_motionblur`; refused: Enable 0, a mode other than 2, the
camera-threshold flag, the falloff, no depth-of-field branch, taps other than 5. Renderer: `velocity_half`, `motion_blur`,
`motion_blur_copy` after Alpha_R (plan key bit 15).
- Live (`yebis;yebis-tonemap;yebis-glare;yebis-shafts;yebis-dof;yebis-motionblur`, the Shebang zone saves, MotionBlur HIGH in the
  configuration): **Native1684** (m37, Gamma), **Native1686** (m31, Beta), **Native1687** (m50, Delta): `native_yebis_motionblur`
  served with the original's words (BlurTimeRatio 0.2, MaxBlurLength 0.1, NumBaseSamples 4, MaxRecurrences 2, scale 1: R 2, n 5,
  step/L 1/24), `native_motionblur_frame` counts the stage's frames (9276 at m37 normal play), no fault, files restored by hash;
  **Native1685** (m37, `-Measure -NoDebugLayer`, normal play): **60 frames/s** (59.5-60.1 over 77 windows), GPU wait 5.15 ms
  against 4.0 ms with the DoF alone (Native1681).
- Open: a moving camera (previous pose != current) is witnessed neither by a capture nor by a picture: the transform's other
  path and the blur's visible result wait for a Reference/Native pair turning the camera; the object velocity is our reduction's,
  not the original's half-size raster.

**Pre-merge review (fresh Opus 5.5, D-0176, 2026-10-14) and the turning-camera witness.**
- Applied: the blur needs Alpha_R (its depth test reads the scene alpha Alpha_R writes): the renderer's gate takes `alpha_r`, the game
  side refuses a glare plan without its pre draw, a failed Alpha_R skips that frame's blur; planner unit tests on the paths the
  captures do not take (R 1, uncapped R 3, interleave 2, even taps, no blur) and an inverse test with pivot swaps; T16 bounds the
  velocity surface and counts the texels the original's passes change.
- **Found by the review:** at Reference1676-1678 the original's blur passes change **0 texels** (a still camera: every velocity under
  the 2/H rule): T16's equality there showed only the identity. **Witness Reference1693** (m37, Gamma, `yebis-capture-late.flag`: the
  draws capture at frame 2400, the camera turned by continuous relative mouse motion): the first detail frame from 1200 holds the
  current camera as the previous one even while it turns (Reference1688-1692, OPEN why), frame 2400 holds a moving camera. There the
  plan equals the captured draws word for word with previous != current (183 checks: the transform, the projections, the blocks);
  T13 replays both blur passes on their captured inputs, **0 differing texels**, while the original's passes change 887952 and 889057
  of 921600 texels; chained after our velocity surface (188 texels one half unit off) T16's passes differ on 47 and 228 texels.
  A precise HLSL transcription of the velocity programme (own VS and PS, no fused operations) differs more (1348 texels at 1693):
  the original programme is kept; the residual is INFERRED to be the driver's D3D11 against D3D12 compilation of the same DXBC.
- Labels: the object velocity reduction's equivalence to the original's half-size raster, the previous field of view taken equal to
  the current, the previous pose by the velocity's cut rule, and S+0x340, S+0xA00..+0xA08 are INFERRED; the background of the native
  target (0.5 -> 128 in 8 bits against the original's 127) does not reach the image (alpha 0: the velocity draw keeps the camera
  velocity exactly). Not done: a byte comparison of the reduction against a captured object velocity.

## 11. Auto exposure: preparation (2026-10-14, thread « Flou de mouvement natif », handed over)

- At the witness zones auto exposure is off: no adaptation draw in the captured frames (Reference1676-1678, 1693: no
  `tech_AnalyzeIntensity`, the library's measurement technique), the tone map's t2 constant; the native tone map with a null t2 is
  the original's output there (§4). The native refuses the tone map (row `native_yebis_tonemap`) when the blended record turns it on.
- Where it turns on (YP8 of YEBIS-SECOND-TRANCHE-READY): `MapOverride_0001` and `_0003` set `AutoExposure`. Read
  (`evidence/I-0068/queries/yebis/ae1`..`ae7`): FUN_140619BD0 loads `gparam:/MapOverride_%04u.gparam` (format at 14294F560,
  FUN_140D27E20, FUN_140A36700) with the id FUN_140CE70A0([1447914C8], area): word +0x1C of the area's 0x110-byte record
  ([table+0x20] + area x 0x110, [table+0x10] records; +0x18 is the id FUN_140A36420 takes). Native row added (not yet run):
  `native_map_override_table` (with `yebis.flag`, once per session) lists the records whose override id is not 0, so one run names
  the areas that load MapOverride_0001/_0003; then a Reference capture there (`tech_AnalyzeIntensity`, the exposure read-back
  FUN_1416CA160 into 144962008) gives the oracle.
- Also found: at m31 the original draws a colour grading LUT pass after the tone map (`tech_MadColorGradingLutT1_S1`, a 16x16 LUT
  t16, Reference1650/1665/1668/1677), not native yet (YB-4c).
- **Answer (thread « Exposition auto et étalonnage LUT natifs », 2026-10-14; K-0448 SUPPORTED): the original never turns auto
  exposure on in reachable play, so there is nothing to draw natively.** Live Native1695 (m37, `yebis.flag`): the map-info table
  holds 26 records, none with a MapOverride id (`native_map_override_table`). Static: the id is written only by FUN_140CE7100 from
  the Ceremony row's u16 at +0x0A (`q4`), 0 in every row of the regulation's Ceremony param except 30001020 (600: no
  `MapOverride_0600` exists; the dump is `evidence/I-0067/queries/gparam/ceremonyparam.tsv`). The same record's slot-0 file is
  `i x 100 + base % 100` with i = 0 (FUN_14061B590 loop, base = the row's u16 at +0x08: 0 or 10 in the m30 rows, 0 to 30 in all),
  so a base never makes a `x1xx` file the slot-0 file. A dump of the 168 drawparam GPARAM files (`evidence/I-0066/queries/gparam-all`:
  the 161 of the I-0063 cache plus the 7 the dictionary does not name, `m47_00_0800`/`0900`, `m50_00_0100`-`0300`, `m51_00_0100`/
  `0200`, recovered by hashing every `mAA_BB_0000`-`0999` name of the 22 world maps: `Recover-GameDataByName.ps1`,
  `queries/ae/gparam-recovery.json`) finds `AutoExposure` set only in `MapOverride_0001`/`_0003`, `m30_00_0100`/`_0110` and the five
  `m36_00` files (`Enable AutoExposure` 0 everywhere; no cutscene `s*` file, `remobase` or `mapbase` sets it). A map's slot files 1..4 are chosen by the player's collision
  part (or the override part `[144791248]`, FUN_1408FA7F0): FUN_140CE6040 reads FUN_140E17640 = `(signed char)[[part+0x10]+0xC8]
  [+0x3C + slot - 1]` (0 none) and the fade FUN_140E17690 = float `+0x40` (`q14`). `Get-MsbGparamSlots.ps1` reads those bytes in
  the 31 world MSBs (`evidence/I-0066/queries/ae/msb-gparam-slots.tsv`): in the maps whose values name files, every value names an
  existing file (m37 1..5, m45 1..8, m50 1..3, m51 1..2, m38 1, 2, 6, m39 3, 5 ...), which validates the offsets; values naming no
  file remain at m30_00 (3, 4, 5), m34_01 (2) and the unused sub-maps m31_01-m31_09 (1). No m30_00 collision names file 1 (values
  2..5): the High Wall's auto-exposure files are never applied. m36_00 is not a retail world map (no Ceremony row; INFERRED unreachable).
  Residual: the map-info record's override targets +0xE4..+0xF4 have no writer found. The review found the census had counted
  m47_00 twice and left the DLC2 variant files out (the dictionary does not name them): corrected above. The native guard stays: a positive flag in
  either Tone Map record refuses our tone map (`native_yebis_tonemap {refused: "auto exposure"}`) and logs `native_yebis_auto`.

## 12. Colour grading (thread « Exposition auto et étalonnage LUT natifs », 2026-10-14; K-0449)

- **The original (Reference1696, m31, `yebis-draws-capture.flag` extended to PS slot 16 and 3D textures, and
  `reference_yebis_grading_state`).** Draw 50 of the library chain, right after the tone map (draw 49, whose output is draw 50's t0
  byte for byte): `tech_MadColorGradingLutT1_S1` (`vs_2D_OffsetTex2D1`, `ps_MadColorGradingLutT1_S1`), t0 the tone-mapped image
  (R8G8B8A8, 1280x720), t16 a 16x16x16 R32G32B32A32_FLOAT volume, s0 and s1 filter 20 (linear, point mips), U and V clamp (W not captured: clamp assumed, inert at the 1/32 to 31/32 coordinates the PS samples), MaxLOD 0, the
  three vertices of the glare family's 2D draws, opaque. The PS (disassembly `evidence/I-0067/queries/yebis-asm`):
  `rgb = t16.Sample(t0.rgb x Modulate[0].rgb x Offset[0].rgb + Offset[1].rgb)`, `a = t0.a x Modulate[0].a`, with Modulate[0] = 1,
  Offset[0] = 15/16 and Offset[1] = 1/32 (the (N - 1) / N and 1 / 2N of the 16-texel LUT; Offset[1].w 0.5 is the captured word, its
  producer not read and unused by the PS). The post object's three names (`post+0x2100/2130/2160`) are `m31_00_CGrading`: the
  record's prefix (FUN_14017B270: the gparam name and the wide string "CGrading" at 143D46B88 for a map file), to which 14021CC20
  appends LutSourceId with `%s%04d` (143D42C18) and asks the texture repository. The textures are the strips of each map's
  `map:/mAA/mAA_cgrading.tpf` (16x256 RGBA8, names `mAA_BB_CGrading<id>`; m30, m31-m35, m37, m39-m41; none for the DLC maps). The
  volume equals `m31_00_CGrading0001` bit for bit through GX's `LutCopy2DTo3DCs` (3D[x][y][z] = 2D[x][16z + y], read in its
  disassembly), which FUN_1400E5110 dispatches each frame the trigger 144962228 is set; FUN_1400E69A0 enables the library's grading
  only with the volume, the record's flag and a texture at +0x358 or +0x360 (no texture: no draw, which is why m50, word 1 = 1 but no
  `m50_00_CGrading0001`, draws none).
- **Native (`yebis-grading.flag`, with the tone-map input).** `renderer/grading_stage`: the LUT copy with the pinned compute (YB-5's
  `LutCopy2DTo3DCs`) into the stage's volume, then the grading draw with the original programmes (the image's .rdata, pinned) from
  the stage's own source target, into which the tone map now draws; the block is `GradingStage::Globals` (the words the two
  programmes read). Game side: the TPF strips are registered by entry name at admission (`native_grading_names`, beside the env-map
  cubes); the feed keeps the blended ColorGrading integers and the area of the record whose LUT id is word 1 (A below weight 1, B
  from 1, 14021C150's table), names the texture `m%02u_%02u_CGrading%04d`, and refuses (no draw) when word 0 is 0, word 1 negative,
  the record is not from a map file (mapbase, default, a cutscene file: the prefix branch of FUN_140095220 is unread, refused with that reason; a record the bank takes from the area's base file mAA_BB_NNNN uses that file's name, §13), the texture is absent or not resident, or not a 16x256 RGBA8 strip (the original tests the size only and falls back to the library's default LUT; no shipped strip differs) (`native_yebis_grading` at each
  change). Rows `native_grading_frame` count the stage's frames.
- **Tests.** T17 (`grading_stage_test`, the build runs it on WARP and hardware): the words the programmes read equal the captured
  block's; the strip rebuilt from the captured volume (every word k/255) copied by the stage equals the captured volume bit for bit;
  the grading draw from the captured t0 equals the original's output, **0 of 3,686,400 bytes on the capture's adapter** (WARP within
  1/255); no debug-layer message.
- **Live.** Native1697 (m31, all YEBIS flags, `world-pictures.flag`): `m31_00_CGrading0001` found (area 131000000), 4022 graded
  frames, 0 debug errors, no fault. Native1698 (m50): `texture absent`, no draw, as the original. Native1699 (m31, `-Measure
  -NoDebugLayer`, normal play): **60 frames/s** (59.93 median over 79 windows), GPU wait 2.42 ms, 9502 graded frames. Native1700 (m45) and 1701 (m51): word 1 = 0, `m45_00_CGrading0000`
  and `m51_00_CGrading0000` absent, no draw (Reference1641/1652/1663/1670 at m45: no grading draw); Native1702 (m37): word 0 = 0, off. No fault in any.
- Not done: a key transition uses word 1's texture of A below weight 1 (the record's own timed fade, 14017BE20, is not driven
  natively); the zones with a LUT other than m31 (m40 id 3, m30 id 2 ...) are not witnessed; `tech_MadColorGradingLutT1_S4` (four
  samples) was not seen drawn.
- **Review (fresh Opus 5.5, before the merge; applied).** (1, blocking) The frame graph's plan key had no grading bit while the
  `colour_grading` pass is declared only with the grading: a key cached without it would throw at a frame with it (the grading toggles
  at loads and where an m31 collision selects `m31_00_0100`, which has no ColorGrading group). The tone map and the grading are now
  decided before the key (`tonemap`, `grading`) and the key carries `grading << 16`. (2) The LUT prefix came from the area number with
  an invented gate: the record's bank source is now kept (`Banks::GroupEntries`, `"area"` for the area's own map file) and any other
  record (mapbase, default, cutscene files, whose prefix branch is unread) is refused as `non-map record (prefix unread)`. (3) The
  volume-against-TPF comparison is preserved: `Compare-GradingLut.ps1`, `i66-reference-1696-m31-grading/grading-lut-vs-tpf.txt`
  (0 of 4096 voxels). (4) K-0448's census: m47_00 was counted twice and the DLC2 variant files were missing from the dump (corrected
  in §11; none sets auto exposure); the base file id (+0x08) cannot make a `x1xx` file the slot-0 file. (5) The size-or-format refusal
  and (6) the key transition are listed as departures/limits; K-0449 says "at a settled key". (7) The name lookup and the texture
  acquisition now hold the name map's lock together. (8) The samplers' W mode is an assumption (stated); m37, m45 and m51 are now
  witnessed live (Native1700-1702).

## 13. W10 closure: our YEBIS passes by default (thread « Exposition auto et étalonnage LUT natifs », 2026-10-14; K-0450)

- **Rule 1 of YEBIS-WAVE1-READY 7** (the five zones plus m30 and m40 for the grading and tone variants). Two Shebang characters were
  placed for the D-0174 witnesses (Reference1704: Eta loaded at Firelink Shrine, Theta travelled to the High Wall of Lothric under the
  eclipse; `.ds3-i0039-backups/shebang-zones-eta-m40-*`, `-theta-m30-*`, SHA-256 in `record.json`; Alpha untouched). Reference1705
  (m40) and 1706 (m30) captured the library chain (`yebis-draws-capture.flag`, `yebis-tonemap-capture.flag`, `yebis-observe.flag`):
  both draw the vignette tone map and the grading (`m40_00_CGrading0003`, `m30_00_CGrading0002`, each volume equal to its TPF strip:
  `grading-lut-vs-tpf.txt`, 0 of 4096).
- **Tests on them (hardware, the capture's adapter):** T12 (tone map, vignette variant) and T17 (grading; the test now finds a
  capture's grading draw by its row) 0 of 3,686,400 bytes at both zones; the DoF plan bit for bit (`yebis_dof_test`, 5 zones) and its ten
  draws replayed (T13) and drawn by the glare stage (T15: 50 of 50 surfaces at 5 zones).
- **The DoF's nine-tap levels (m40).** At m40 the DoF levels 1 and 2 draw `tech_MadT1_S25`, which the native refused ("a level blur
  other than 3 or 5 taps"). FUN_14173C2B0 dispatches by the level's tap count (the preset table 142B659C0: 3, 5, 9, 17): 3 to
  FUN_14173B2C0, 5 to FUN_14173B3D0, 9 to FUN_14173B850 (any other: E_NOTIMPL, nothing drawn); FUN_14173B850 builds a 5x5 kernel with
  FUN_1416D77E0 (25 taps at ten distances read from the executable, weights 1, 2, 4, 1.75, 0.9216, 2.688 normalised by their sum row by
  row), now `Blur25` in `yebis_dof` (`LoadDofDrawData` reads the ten words with the others); the pin `tech_MadT1_S25` is added
  (`yebis-techniques.tsv`). 17 taps stay refused.
- **Defaults.** `NativeYebisPass`: the feed and the six passes (tone map, glare, shafts, DoF, motion blur, grading) are on unless
  `no-yebis.flag` (all) or `no-yebis-<pass>.flag` (that one, with its dependants: see the review below) lies next to the module; the old opt-in files no longer matter;
  `Run-StartupTrace.ps1` accepts the opt-out flags.
- **Live, defaults (no effect flag):** Native1707 (m40, before `Blur25`: the DoF refused "a level blur other than 3 or 5 taps", which motivated it), Native1710 (m40: tone map 2.3 / 0.15 / ColorS 0.8 / vignette 0.3 as Reference1705's tone,
  grading `m40_00_CGrading0003`, glare 29 draws, shafts, DoF 10 draws), Native1711 (m30: exposure 2, contrast 0.13, ColorS 0.85, vignette
  0.34 as Reference1706's, grading `m30_00_CGrading0002`, DoF 10 draws), Native1714 (m31: grading, DoF), Native1713 (37,790 frames over
  successive loads of m50 and m45 and returns to the title through Load Game and Quit Game: no fault, 0 debug errors, no stage failure;
  the tour's list positions loaded m50 and m45 only, as on 2026-10-13), Native1712 (m40 normal play, `-Measure -NoDebugLayer`: **60
  frames/s**, GPU wait 1.07 ms). No fault in any.
- **Fixed on the way:** the LUT prefix when the bank takes the ColorGrading group from the area's base file (a variant layer without the
  group: source `mAA_BB_NNNN`, Native1713 at m50/m45) is that file's name, no longer a refusal. Native1708/1709 never reached the world
  (the title pilot missed the network notice, the game ran without fault); 1710 with the same build did.
- **Observation, outside W10:** at Firelink Shrine the native floor looks snow-covered where the original shows grey stone (Native1710
  against the Reference1704 screenshot): a scene-family difference (the snow family), not a YEBIS pass.
- **Not done:** V5's 30-minute pilot loop with resize and teleport (the tour covers zone loads and menus); 17-tap DoF levels; the
  grading of cutscene records (refused, prefix unread).
- **Review (fresh Opus 5.5, before the merge; nothing blocking; applied).** (1) T17 now takes the grading row of the captured frame
  (the frame before the first capture row), and fails when a trace has none instead of skipping. (2) Native1707 is the DoF refusal
  that motivated `Blur25`; the DoF is witnessed by 1710. (3) The glare (`yebis_glare_test`), shafts (`yebis_shafts_test`) and motion-blur
  (`yebis_motionblur_test`) tests now run on 1705/1706 too (the build's zone lists; the motion-blur list no longer drops them). (4) The
  named-map prefix branch has no live witness after the fix: at m50/m45 it changes "non-map record" into "texture absent", the same
  no-draw outcome as the original; the digit test no longer uses `::isdigit` on a char. (5) The tests write one output file per zone.
  (6) The opt-outs depend on each other: `no-yebis-tonemap` also turns off the grading, the glare, the DoF and the motion blur (they
  hang on the tone-map input), `no-yebis-glare` the DoF and the motion blur; and `yebis-observe.flag` in a native run stops the
  converters, so the glare (which reads their globals) is refused there: it belongs to Reference runs. The old opt-in flags are
  accepted by `Run-StartupTrace.ps1` and do nothing. (7) Stale comments and the pin file's header updated. (8) `1.75f` written plainly.
