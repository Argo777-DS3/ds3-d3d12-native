# I-0113 tranche 17: the 0x31 layer's alpha in the original (static, study thread)

Study thread, 2026-11-06 (Paris), read-only, for the open-gaps pilot. Source: Thomas's Ghidra GUI on `DarkSoulsIII.exe` 1.15.2
(sha256 `ef5e07c5…498c`) through GhidraMCP. Non-canonical until the pilot re-reads the lines a change rests on. The question:
with tranche 17's half-size layer, the Firelink bonfire's coiled sword disappears behind the flame
(`evidence/I-0113/t17/bonfire-layer-top-full-bottom.png`, OBSERVED, RESULT « Tranche 17 »). The composite is
`layer.rgb + scene.rgb × layer.a` with the layer cleared to (0, 0, 0, 1) ([STUDY-PASS-31](STUDY-PASS-31.md) §2, §3), so a 0x31
draw that writes its source alpha into the layer, instead of attenuating the layer's alpha, hides the scene behind it.

## 1. Every FFX entity command runs through one callback (VERIFIED)

`1402997E0` (the sub-pass chooser of STUDY-PASS-31 §4) records each entity's command with callback `140299E40`
(`puVar10[0] = FUN_140299e40`, data the entity and a block whose `+8` is the chosen sub-pass) and writes the pass context's
`+0x128` (the engine pass: `0x23` when `1402993F0` holds, else the sub-pass). The sub-pass by the entity's kind `param_3`
(meanings not identified): 6 → 0x31 when `DAT_14473DAD6` (default 1), else 0x29; 3 and every other kind → 0x29, or 0x31 when
« near »; 1 → 0x21 (0x31 only with the debug byte `DAT_144962375`); 2, 4 → 0x29; 5 → 0x2D. « Near »: the entity's view-space
depth at most |size (`1402994B0`) × (2 − env `[+0x158]`) × cam `[+0x64]`|, so a kind-3 effect close to the camera moves to 0x31
(INFERRED: which Firelink effects do is a live read, not static).

## 2. The draw context's mode follows the pass (VERIFIED)

`140299E40` builds a `GXFfxDrawContext` on its stack (`FUN_14025FB80(apuStack_b58, …)`, vtable `143D65688`) and sets its
`+0x18` (the local at `apuStack_b58 + 0x18`) from the pass context's `[+0x128]`: 0x20 → 0, 0x21 → 1, 0x29 → 3, 0x2D → 5,
**0x31 → 6**, any other → 3; then for passes 0x30 and 0x31 it replaces the depth SRV by the half depth
(`[[block+0x30]+0x28]+0x48`), and calls the renderer's `vt+8` with the context.

## 3. The commit takes the blend by that mode (VERIFIED)

`140264230` (the context's commit, slot `+0xD8`), around `140265DF0`, with `t` = the blend type `ctx+0x960` (FFX-DRAW's types:
0 off, 1 alpha, 2 additive, 3 reverse subtract, 4 multiply) and the GX common state bank `B = [[144961AB0]+0x2A0]`:

| mode `ctx+0x18` | blend | depth |
|---|---|---|
| 1 (0x21) | `B+0x08` (off) | `B+0x1E0` (LESS_EQUAL, write) |
| 3 (0x29), 5 (0x2D) | t 0 → `+0x08`, 1 → `+0x10`, 2 → `+0x20`, 3 → `+0x28`, 4 → `+0x18`, other → `+0x10` | `B+0x1E8` |
| **6 (0x31)** | **t 0 → `+0x1A0`, 1 → `+0x1A8`, 2 → `+0x1B8`, 4 → `+0x1B0`, other (3 included) → `+0x1A8`** | `B+0x1E8` |
| 0, 2, 4 | none: the blend argument is 0 | 0 |

then `141920ED0(ctx, ctx+0x10, blend, 0)` (the GX context's set-blend, the native's `Blend` hook at RVA `1920ED0`) and
`141920F00(…, depth, 0xFF)`. Before it, when `ctx+0xA39` is set, `ctx+0x960` is forced to 0 unless `DAT_14496237D`.

The bank's layer entries as their D3D11 objects describe themselves (FFX-DRAW, Reference1956): `+0x1A0` ONE / ZERO, alpha
ZERO / ZERO; `+0x1A8` SRC_ALPHA / INV_SRC_ALPHA, alpha ZERO / INV_SRC_ALPHA; `+0x1B0` DEST_COLOR / ZERO, alpha ZERO /
INV_SRC_ALPHA; `+0x1B8` SRC_ALPHA / ONE, alpha ZERO / ONE. So in the layer an alpha effect multiplies the layer's alpha by
(1 − src.a), an additive one leaves it, an opaque one sets it to 0: the layer's alpha is the scene's transmittance. The ordinary
entries `+0x08..+0x28` all have alpha ONE / ZERO (they write src.a), harmless in the full-size HDR, wrong in the layer.

## 4. The native's side (VERIFIED in `native/`, branch `claude/focused-lovelace-a80c8a`)

- `native/frame/effect_draw.cpp:495` writes `pass_context+0x128 = c.pass` before calling each command's callback, so the
  original's `140299E40` sets mode 6 natively too, and the hooked `Blend` captures the layer entries for the classic draws.
- `native/renderer/effect_stage.cpp` `twin_blends` holds `+0x1A0..+0x1D0` with the descriptors above (D3D11 enumerations).
- A null blend is not drawn natively (`NativeEffectBankBlend` refuses it: « blend entry unknown »).
- So the classic E01 draws at 0x31 should already carry the layer entries natively (INFERRED from the code; the pilot's witness
  `native_effect_pass_blends` reads it). The candidates left are the draws the native issues with states of its own in the
  layer: the 0x29 rows, the trails (606, 10012) and the E03 model particles drawn by `before_pass` / `before_draw` (the pilot's
  lead), or a renderer class that does not take its blend from the commit (being read, §6).

## 5. The composite (VERIFIED)

`140102A80`, the plain path (block byte 0 = 0, `Graphics.IsEnableBilateralUpscale` and `Force HQ UpScale` off): two samplers
`B+0x2E0` and `B+0x2E8` (FFX-DRAW: the clamp versions of point and linear; which one the PS samples is in its bytecode, not
read), blend `B+0x1D0` (ONE / SRC_ALPHA, alpha ZERO / ONE), depth `B+0x1D8` (off), raster `B+0x208`, VS `[lib+0xB60]`, input
layout `[[lib+0xC18]+0x10]`, PS `[[lib+0xB98]+0x10]`, t0 the half colour (`block+0x10`), the other stages' resources null,
`lib = [GXRS+0x4DD8]`. The native's composite matches it but for the sampler (a static linear clamp).

Correction (the hooks' names in `native/frame/effect_draw.cpp:372-387`): `1419203F0` sets the input layout, `1419206E0` the VS,
`141920EA0` the PS; so the composite's input layout is `[lib+0xB60]`, its VS `[[lib+0xC18]+0x10]` (ScreenSpaceQuadShader, as
AA-NATIVE-READY reads that slot).

## 5b. The bilateral composite (the live path: E's `reference_layer_composite` read byte 0 = 1, 2026-11-06)

- **Who sets byte 0** (`140102790`, VERIFIED): `byte0 = [settings+0x415C] || [[GXRS+0x5360]+0x4B]`. `GXRS+0x5360` is the quality
  object; its `+0x48..+0x67` is the Effects row `table[0x168 + level × 0x20]` (QUALITY-OBJECT-NATIVE §2), so `+0x4B` is the row's
  fourth flag byte. In the table (`quality-table.tsv`): level 0 `00 00 00 00`, 1 `00 00 00 00`, 2 `01 01 01 00`, 3 `01 01 01 00`,
  4 `01 01 01 01` (OBSERVED in the dump): only the highest Effects level turns « Force HQ UpScale » on; Thomas's Effects MAX is
  that level (INFERRED: the levels run DISABLE..MAX as 0..4, QUALITY-OBJECT-NATIVE §1). The native's quality object is built from
  the same table, so `+0x4B` is 1 natively too at MAX.
- **The PS** `[[lib+0xBE0]+0x10]` is **`SampleBilateralDepthBlur`** and the plain `[[lib+0xB98]+0x10]` is **`SampleColor1`**
  (INFERRED, strong): the library's programme-name table at `.data 14473CAB0` lists the post-effect pixel programmes
  `PointSampleColor1, PointSampleColorClamp1, PointSampleColorCollect, PointSampleColorInterpolate, ColorGrading, SampleColor1,
  SampleColor4, Sample13Weight, Sample13WeightRotation, Sample13WeightforGlow, Sample13WeightforGlowKernel, Sample13WeightforSnow,
  Sample13WeightforDeferredSnow, SampleBilateralBlur, SampleBilateralDepthBlur, OutputOne, OutputColor, EmissiveToGlowPS,
  GlowCompositeSinglePS, SampleColorSharpness, SampleColorOrverlay` (UTF-16 names read in the executable), then
  `14473CA78` the VS `ScreenSpaceQuadShader, ScreenSpaceQuadShaderZ`; with one slot of 8 bytes each from `lib+0xB70`, the two
  anchors read earlier hold (`+0xB70` PointSampleColor1, `+0xC18` ScreenSpaceQuadShader, AA-NATIVE-READY), index 14 is `+0xBE0`
  and index 5 `+0xB98`. The writer of the slots was not read.
- **Its resources** (`140102790`'s block, `140102A80`'s bilateral branch, VERIFIED): t0 = block+0x10 = `[[settings+0xF88]+0x48]`
  (the half colour); **t1 = block+0x18 = the HDR target group's `+0x38`** (the group `settings+0x660` when `DAT_144961BA0`, else
  `settings+0x240`; the same group whose `+0x30` is the RTV at block+8): the full-size scene depth of that group (INFERRED from its
  place beside the RTV; not the effect copy t3 `[settings+0xFD0]`); t6 = block+0x20 = `[[settings+0x3980]+0x48]` (the half depth);
  samplers s0 = `B+0x2E0`, s1 = `B+0x2E8`; PS constants slot 1 (`141920E20(…, 1)`) from the buffer object `[lib+0xB18]`, filled
  by `141920300` with 0xD0 bytes (the words below); blend `B+0x1D0`, depth `B+0x1D8`, raster `B+0x208`; the target block+8.
- **The constant block** (floats from offset 0; `hw, hh` = block+0x34/+0x38 the half size, `W, H` = block+0x2C/+0x30 the full
  size; the constants `143D7EE10` = 1.0, `143D7F0C8` = −1.0, `143D7FB40` = (0, 0, 0, 1), read in memory):
  `[0] (−1/hw, −1/hh, 1, 1)`, `[1] (1/hw, −1/hh, 1, 1)`, `[2] (−1/hw, 1/hh, 1, 1)`, `[3] (1/hw, 1/hh, 1, 1)` (the four taps),
  `[4] (W, H, 0, 1)`, `[5] (P[0x28], P[0x38] / (far − near), far − near, 1)` with `P` the 4x4 from `14004B070(camera)` (the
  projection, INFERRED) and near/far the camera's `+0x54/+0x58`, `[6..12] (0, 0, 0, 1)`. E's reading of the programme (cb1
  `g_avSampleOffsets[0..5]`: offsets × 0.5, a size for `frac(uv × size)`, `z = −c.y / (d − c.x)`) matches rows 0..5.

## 6. Not reached yet

Which renderer classes draw with a context other than `140299E40`'s (the persistent contexts built by `14024AA60`, `14024D000`,
`140296690`, `140296E20`), how the E03 model particles and the trails choose their blend at 0x31, and who sets the GPU
particles' `ctx+0x9BB` (their layer indices 0x34 to 0x36 in `1402695E0`): a `ghidra-research` read in progress.
