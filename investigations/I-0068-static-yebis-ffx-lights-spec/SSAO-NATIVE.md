# I-0068 part Q: SSAO and the shadow/AO composite, ready to code

Request (coordinator, 2026-10-10), second task:

- SSAO and the composite (`GXLightDeferredAO`, `GXRend_CompositeShadowMask`);
- check on `main` whether native SSAO exists;
- if not, the ready-to-code specification: passes, targets, constants, blur and the reading by
  the lighting, with checks in the five zones.

See also (part BA, 2026-10-11): [SSAO-D3D12-DESIGN-NATIVE](SSAO-D3D12-DESIGN-NATIVE.md): the AO shaders, the GParam binding and gate, the zone parameters, and the D3D12 design for the frame plan (rows SS1-SS12). See also (part BK, 2026-10-11): [SSAO-WAVE1-READY](../I-0066-independent-native-startup/SSAO-WAVE1-READY.md): parameter record and blend, view record writer, corrected quality gate, rotation texture owner, programmes with fingerprints.

Labels: STATIC, OBSERVED, INFERRED. Evidence: `evidence/I-0068/queries/ssao/`:

- `lighting-input-chain-i63-live-10.tsv`: the producers of the lighting draws' t32 and t33,
  followed back through their inputs in the five witness captures (`Get-LightingInputChain.ps1`);
- the programme interfaces from I-0063's `program-resources.tsv`;
- the m40 listing `evidence/I-0053/artifacts/c1/gpu-order-i52-live-03-A-frame-300.txt`
  (programme names).

## 0. Native status (`main`, code read)

| Part | Production native | Reusable |
|---|---|---|
| SSAO (t33) | **absent**: neutral white for the lighting (`scene_pass.cpp`: "neutral white until the native SSAO family exists"), the forward pass (Native1115–1117: "SSAO t33 white") and the point lights | **every constant producer** in `native/world/light_producers.cpp`, exact on live frames (I-0045 K-0421/K-0424): `cbSSAOParam` (1B33D0/1B38D0), the SSAO kernel `cbSampleOffset` (1B35CC), `cbSSAOParam2` (1B26E0), the depth downsample `cbSampleOffset` (1CB9D0) |
| shadow/AO mask (t32) | **neutral white** (`model_frame.h`). A "paired shadow mask" pipeline exists (`scene_pass.cpp`): the composite's two stencil-tested draws, refs 0 and 16, read mask 16. It is fed only by offline reference inputs | `cbCompositeShadowMask` (10CC20), `cbBlurParam` (1D7850), `cbPerScene` (1D6D20) |

**So native SSAO does not exist.** The specification follows.

## 1. The original chain (OBSERVED in the five zones, names from m40)

Same structure and sizes at m31, m37, m45, m50 and m51 (1280×720 frames), per frame:

| # | Programme (m40) | Draws | Target | Inputs | Constants |
|---|---|---|---|---|---|
| 1 | `DepthScale2x2OutputDepth` | 2 | **640×360 depth** (`R32G8X24`, DSV only, no colour) | t1 the scene depth | b1 `cbSampleOffset` (1CB9D0: the four ±0.5-texel taps) |
| 2 | `DepthScale2x2OutputDepth` | 2 | 320×180 depth | t1 the 640×360 depth | same |
| 3 | **`ScreenSpaceAO2_WithNormal`** | 1 | **640×360 RGBA8** (raw AO) | t0 half depth, **t1 the G-buffer normal** (GBuffer0, RGBA16F), **t2 a 64×64 RGBA8 random-rotation texture** | b0 `cbSSAOParam`, b1 `cbSampleOffset` (8-entry kernel), b2 `cbSSAOParam2` |
| 4 | **`ScreenSpaceAO2_SeparatedReProj`** | 1 | 640×360 RGBA8 | t0 half depth, t1 the raw AO, **t3 the previous frame's AO** (640×360 RGBA8) | b0, b2 (the previous view-projection is `cbSSAOParam2` 44..59) |
| 5 | history copy (programme id 191; name not in the m40 listing) | 1 | the 640×360 history texture | t0 the reprojected AO | — |
| 6 | **`ScreenSpaceAO2_BilateralUpScale`** | 1 | **1280×720 RGBA8 = t33 `g_SSAOTexture`** | t0 the **full depth (the readable depth copy, FORWARD-DEPTH-NATIVE)**, t1 the reprojected AO | b0, b2 |
| 7 | `GXScreenSpaceCascadedShadow_Soft` (or `_PCF4`/`_PCF9` by ShadowParam FilterMode) | 1 | 1280×720 RGBA8 (mask B) | t1 depth, t32 the cascade atlas, t38 dither | b0 `cbPerScene`, b8 `cbSceneParam` (SHADOWS-NATIVE) |
| 8 | `GXScreenSpaceShadow_Blur` | 3 | ping-pong mask A/B | t1 depth, t0 the other mask | b1 `cbBlurParam` (12 Gaussian taps) |
| 9 | programme id 189 = **`OutputColor`** (INFERRED: the m40 listing names `OutputColor` at the same position and count, one draw right before the shadow and before each blur, same target; part S) | 4 (2 per mask) | masks A and B | depth | — (INFERRED: fills the target with a constant colour before the stencil-limited draw) |
| 10 | **`GXRend_CompositeShadowMask`** | **2: stencil ref 0 and 16** (depth target bound, stencil test) | **mask A = t32 `g_ShadowTexture`** | **t3 GBuffer1** (RGBA8), **t33 the SSAO** | **b5 `cbCompositeShadowMask`**: one `{FC_Mult, FC_Add}` pair per draw (executable constants at the two call sites `10CDD9`, `10D01E`) |

Notes:

- The masks and the SSAO are 8-bit RGBA, full resolution. Only the AO itself runs at half
  resolution with temporal reprojection (history texture).
- The composite's two draws split the pixels by **stencil bit 0x10** (read mask 16). The
  G-buffer writes the stencil (native: `|8`, SSS `|4`, `|2`; Native1137 comment on 140140850).
  INFERRED: bit 0x10 marks a pixel class written by another pass (characters or the
  SSAO-excluded materials); see risk 2.

**Readers:**

| Reader | Slot | Use |
|---|---|---|
| `GXLightDeferredAO` (`…WithSsr`) | **t32** | irradiance and specular × `t32.x · t32.y` (NATIVE-LIGHTING-MAP) |
| `GXLightAcc_DDirLightShadowed1[_Cloud]` | **t32** | the sun's shadow |
| point, spot and line lights (`GXLightAcc_D*Light*`) | **t33** | AO on local lights |
| forward `_Fwd` programmes (e.g. `GXFlver_ColDifSpcBumpIbl_Fwd`) | **t33** | AO in forward materials |
| `GXDeferred_Debug` | t32, t33 | debug only |

## 2. Ready-to-code order

| # | Step | Mode | Native |
|---|---|---|---|
| 1 | resources | NATIVE | half (640×360) and quarter (320×180) depth (D32S8 / R32 typeless as the main depth); raw AO, reprojected AO, **history AO** (640×360 RGBA8, kept across frames); t33 1280×720 RGBA8; masks A/B 1280×720 RGBA8; the 64×64 random-rotation texture (INFERRED: a GX system texture; take it from the engine's texture registry like the other system textures) |
| 2 | depth downsample ×2 | NATIVE pass with the original `DepthScale2x2OutputDepth` | `cbSampleOffset` from `DownsampleOffsets` (port) |
| 3 | `ScreenSpaceAO2_WithNormal` | NATIVE | `cbSSAOParam` (`SsaoParam`), kernel (`SsaoKernel`), `cbSSAOParam2` (`SsaoParam2`); point-wrap sampler s0 for the rotation, white-border point s4 for depth |
| 4 | `…SeparatedReProj` | NATIVE | the previous frame's history, the previous view-projection in `cbSSAOParam2` |
| 5 | history copy | NATIVE (`CopyResource` is enough, INFERRED from a 1:1 same-size copy) | keep the history across frames; invalidate on resize |
| 6 | `…BilateralUpScale` → t33 | NATIVE | the readable full depth copy |
| 7 | screen-space cascaded shadow, blur ×3 | NATIVE (SHADOWS-NATIVE step 8) | `cbPerScene`, `cbBlurParam` producers |
| 8 | **composite ×2 → t32** | NATIVE: the existing "paired shadow mask" pipeline, now fed by the live mask B and t33 | `cbCompositeShadowMask` per draw (`CompositeMask` with the call-site constants); stencil refs 0 and 16, read mask 16, EQUAL |
| 9 | lighting | switch t32/t33 from white to the live textures for DeferredAO, the sun, local lights and **forward** | — |

- **Without shadows yet:** steps 2–6 and 8 can run before SHADOWS-NATIVE. Feed the composite a
  white mask B, so that t32 = AO composite only. This gives AO before shadows, with the same
  composite.
- **Timing:** after the G-buffer and before `GXLightDeferredAO` (K-0407 order: depth
  downsample, SSAO, screen-space sun shadow, env-map regions, DeferredAO…).

## 3. Checks (the five witness zones)

| # | Check | Pass |
|---|---|---|
| A1 | structure per frame | as §1 at m31, m37, m45, m50, m51: 2+2 downsample, 1 SSAO, 1 reprojection, 1 history copy, 1 upscale, 1 shadow, 3 blurs, 2 composites (refs 0/16); sizes 640×360 / 1280×720, RGBA8 |
| A2 | constants | each produced block bit-identical to the engine's at the same frame (the I-0045 substitution route already proved this for these producers) |
| A3 | t33 image | against the original's t33 at the same frame and camera (still camera, after 30 frames so the history has converged): mean absolute difference ≤ 2/255 |
| A4 | t32 image | the same for the composite. With shadows absent natively, compare against the original with shadows off (ShadowQuality low/off setting), or compare only t33's contribution |
| A5 | lit HDR | the I-0063 lit-HDR comparison (K-0426 count) improves at all five zones once t33/t32 are live; no regression at m40 |
| A6 | temporal | a camera cut or teleport: the history is not smeared (the engine's `cbSSAOParam2` blend factor; check the first frame after a load) |

## 4. Risks

1. **History.** Reprojection needs the previous frame's AO and view-projection. On resize,
   first frame or teleport, the history must be reset as the engine does (`cbSSAOParam2` takes
   a factor from the previous-frame object `+0x64`; read its writer before relying on it).
2. **Stencil bit 0x10.** The composite's two classes rely on the G-buffer stencil. If the native
   G-buffer never sets 0x10 where the original does, the ref-16 draw finds no pixels and those
   pixels get the ref-0 constants. Check which pass writes 0x10 (not read in this pass).
3. **The depth used.** The upscale and the shadow pass read the full-resolution readable depth
   (the copy), and the downsample reads the main depth. Swapping them is harmless before the
   forward pass but not after: keep the order.
4. **Programme 189** (four draws per frame into the masks) is `OutputColor` (INFERRED from the m40 listing:
   `OutputColor` precedes `GXScreenSpaceCascadedShadow_Soft` and each `GXScreenSpaceShadow_Blur`
   on the same target). INFERRED: it fills the mask with a constant colour before the
   stencil-limited draw. Skipping it leaves stale values.

## Exports

No new decompile: the producers are already transcribed in `native/world/light_producers.cpp`
(writers 1B33D0, 1B38D0, 1B26E0, 1CB9D0, 1D7850, 10CC20, 1D6D20).
