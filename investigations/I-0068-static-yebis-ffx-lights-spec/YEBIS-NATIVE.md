# I-0068 part A: native replacement of the YEBIS post chain (world post-processing)

Request (coordinator, 2026-10-10): the native replacement of the world post-processing family
(YEBIS 2):

- the `PPFX::IGPUDevice` methods and resources;
- the passes in order, with the library's own pyramid sizes, formats and weights;
- for each pass: DXBC, constants and targets;
- the D3D12 equivalent: PSOs, root signature and barriers.

Inputs: [TONEMAP-SKIN](../I-0067-static-menus-subsystems-map/TONEMAP-SKIN.md) sections 1, 5, 6,
15, 16 and 17 (engine side, YEBIS globals, technique table, tone-map formulas, glare
parameters); the Scaleform HAL pattern of MENUS-MAP §36; K-0410, K-0412 and K-0433; the I-0053
GPU order of one m40 frame. Static reading of DarkSoulsIII.exe 1.15.2.0 on a private read-only
database copy. Labels: STATIC (decompiled), OBSERVED (capture data already in the project),
INFERRED.

## 1. Answer first: replace the HAL, keep the library

- **The YEBIS library talks to D3D11 only through its own HAL** (STATIC), which is H1 of the
  contract:
  - `PPFX::IGPUDevice` (vtable `142B598C8`, 77 slots, source `..\src\Gpu\GPUInterfaceDevice.cpp`,
    built "Oct 14 2014");
  - its resource classes `IGPUTexture`/`IGPUSurface`/`IGPUVolumeTexture`/`IGPUCubeTexture`/
    `IGPUVertexBuffer`/`IGPUIndexBuffer`/`IGPUInputAttribute`/`IGPUQueryOcclusion`;
  - the effect runtime `IGPUEffect` → `CTinyEffect` (vtable `142BDB618`), which binds shaders,
    the constant buffer, textures and samplers;
  - the D3D11 context wrapper `IPfxContext_Dx11` (vtable `142B4C4A0`).
- Every pass of the chain is library CPU code built on four HAL actions:
  1. `CTextureUtil::DrawRectGPU_*` on `IGPUDevice`;
  2. `SetRenderTarget`;
  3. `SetTexture` / `ApplySamplerState` through the effect;
  4. `DrawIndexedPrimitive`.
- The library computes everything else itself: pass order, target sizes, blur weights, tone
  curve and colour matrices (§3, §4).
- **Recommended native route, same as the Scaleform HAL (MENUS-MAP §36):**
  - implement the HAL on D3D12 (native objects with the same vtable contracts);
  - let the game's YEBIS code keep running unchanged on the CPU;
  - YEBIS then never receives a D3D11 object.
- This is a native backend for a middleware interface, not a translation of D3D11 calls:
  - the HAL is the library's portability layer (INFERRED: it was written for other back ends
    too, judging by the `_Gcm` setters and the D3D9-style render-state slots that this build
    stubs as "not supported (DX10/11)");
  - the native object implements its contract directly with D3D12 resources, PSOs and
    descriptors.
- **Without the HAL route,** a native reimplementation of the passes needs the library's CPU
  constant builders, which are not decompiled (TONEMAP-SKIN §15 "open"). It is possible for the
  final tone map (§4.6, formulas known) but long for glare, DoF and afterimage.

## 2. Where YEBIS sits in a DS3 frame (STATIC)

**Path:** `GXBS::GXYebis2Filter` slot 0 `FUN_1400E4CE0` → `FUN_1400E5110` (GX side), only when
the pipeline enables YEBIS (flag bit 10, TONEMAP-SKIN §17).

| Step | Function | Work |
|---|---|---|
| 1 | `FUN_1400E5110` start | when the colour-grading LUT is enabled (`[144962228]`): GX compute `LutCopy2DTo3DCs` writes the 16³ LUT |
| 2 | `FUN_1400E69A0` | configures YEBIS from the YEBIS globals (TONEMAP-SKIN §6), ends with **`BeginPostEffectScene`** `FUN_1416C9310` (binds YEBIS's scene input target through `FUN_141745D10` → `FUN_141706C80`, the HAL's multi-target bind) |
| 3 | `FUN_14007F820` | GX draws `PointSampleColorClamp1`: scene HDR → YEBIS input target (R16G16B16A16F) |
| 4 | `FUN_1416C9330` | **`EndPostEffectScene`** → `FUN_1416E8200` (runs the whole chain, §3) |
| 5 | `FUN_1400E5110` end | GX compute `CustomDofLutCopy` (2048×2 R32F table read by the next frame) |

**Initialisation** (`FUN_1400E2B20`, from `GXSceneContext`):

1. GX binds the target YEBIS adopts (`OMSetRenderTargets(1, …)`).
2. It calls `FUN_1416C87D0` (`PPFX` initialise):
   - context `IPfxContext_Dx11` holding **GX's immediate `ID3D11DeviceContext`**
     (`[[[4961AB0]+0x10]+8]`);
   - GlareQuality `[144961FB8]` and DepthOfFieldQuality `[144961FBC]` (part BD correction: they are not the size; the size comes from the adopted surface);
   - flags `0x90000000 | feature bits`.
3. That creates `CPostEffect` (0xC90, ctor `FUN_1416EBC30`, global `[1448E5E38]`) and runs
   `CPostEffect::Initialize` `FUN_1416EBF80`.
4. `Initialize` builds the device: factory `FUN_14170ACC0` → ctor `FUN_1417080E0` → init
   `FUN_1417074C0(device, initParams, context)`. The init:
   - takes the `ID3D11Device` from the context;
   - creates the device states (`FUN_141707E70`, §5.2);
   - adopts the bound render target and depth-stencil (`OMGetRenderTargets`) as default
     surfaces (`FUN_1417071E0`).

**Native consequence:**

- `[4961AB0]` (GX display object) is absent natively.
- The native backend must create YEBIS with a native context object, never with a D3D11 one.
- It must give the default surface (the native LDR target) and the scene input (the native
  scene HDR) as native HAL surfaces.

## 3. Pass order (STATIC library structure + OBSERVED m40 draws)

### 3.1 Library structure (STATIC; method names from the library's own diagnostics: `FUN_1416E8200` logs "EndPostEffectScene_PreProcess/InternalProcess/PostProcess() failed" after its calls to `FUN_1416E5310`, `FUN_1416E5BC0` and `FUN_1416E8170`; stage placement inside each step INFERRED from the method names)

`CPostEffect::EndPostEffectScene` → `EndPostEffectScene_Process` (`FUN_1416E8200`):

1. **`EndPostEffectScene_PreProcess`** (`FUN_1416E5310`):
   - scene target housekeeping (resolve, tiling);
   - static-scene effects;
   - temporal targets.
2. **`EndPostEffectScene_InternalProcess`** (`FUN_1416E5BC0`):
   - `ApplyEffects_DepthFactorAlpha`;
   - `ApplyEffects_DepthOfField` (`CDepthOfField::ApplyDepthOfField`, `FUN_141767D80`: `_Source`,
     `ApplyApertureFilter`, `_ApertureFilterAllLevels`, `_Composite[_Scatter]`);
   - heat shimmer (`HeatShimmerParticles`);
   - **`CRenderGlare::GenerateGlare`**: `_Source` (`FUN_14174B2D0`), `_Bloom`, `_Star`, `_Ghost`,
     `_AnamorphicFlare`, `_Afterimage` (`FUN_141750000`; corrected in part F, it was given as `_LightShafts`), `_LightShafts`, `_Composite`
     (`DrawRectGPU_MadLumGammaTn`, n = number of levels);
   - **`CRenderGlare::TonemapToSurface`** (`CTextureUtil::DrawRectGPU_TonemapHDR`).
3. **`EndPostEffectScene_PostProcess`** (`FUN_1416E8170`):
   - colour grading (`MadColorGradingLutT1`, enabled by `SetColorGradingLutEnable`);
   - anti-aliasing and feedback;
   - end of frame.

Only the stages enabled by the YEBIS globals run. TONEMAP-SKIN §6 and §16 give which globals
switch them on; for m40: glare shape 1, Bloom on, DoF on, AutoExposure off, colour-grading LUT
on.

### 3.2 The m40 frame: 41 YEBIS draw calls (42 draws: 309 is doubled) mapped to stages (OBSERVED + INFERRED; "35" corrected in I-0068 part I by counting lines 300–340)

Source: `evidence/I-0053/artifacts/c1/gpu-order-i52-live-03-A-frame-300.txt`, lines 297–341: one
original frame. Each YEBIS draw is `DrawIndexed(3)`, a full-screen triangle (§5.4). Resource
numbers are capture identities.

| Draws (lines) | Target ← inputs | INFERRED stage | Why |
|---|---|---|---|
| 297 (GX CS) | 3D LUT 17496 | `LutCopy2DTo3DCs` | GX step 1 |
| 299 (GX) | 17150 (+DS 17153) ← scene HDR 12439 | `PointSampleColorClamp1` | GX step 3 |
| 300 | 17150 ← 12438, 17158, 10476 | `ApplyEffects_DepthFactorAlpha`: blur factor into the scene alpha from depth through the 2048×2 DoF table 10476 (`DepthOfField_BlurFactorAlphaFromDepth…Lut` family) | reads the DoF table of the previous frame |
| 301–308 | 17206, 17207, 17241, 17212, 17213, 17242, 17214 ← 17150 / each other | **DoF**: `_Source` (half-resolution layers), `ApplyApertureFilter` (ping-pong blurs) | three blurred layers produced |
| 309 (×2) | 17151 ← 17150, 17212, 17213, 17214 | **DoF `_Composite`** | scene + 3 layers |
| 310–311 | 17229 ← 17151; 17155 ← 17229 | **glare `_Source`**: downsample, then bright pass at the glare base size (392×331, §4.1) | single chain from the DoF output |
| 312–329 | levels: (17161→17160), (17163→17162), (17166→17165→17164), (17169→17168→17167), (17172→17171→17170→17173), (17176→17175→17174→17177) | **glare `_Bloom`**: per level, a downsample then Gaussian blurs on the level's surfaces | **6 levels** = glare quality 4 (§4.1) |
| 330 | 17186 ← 17160, 17162, 17164, 17167, 17173, 17177 | **bloom level combine** inside `_Bloom` (`FUN_14174C9E0`): `MadLumGammaT6` (corrected in YEBIS-TECHNIQUES §4; it was given as `_Composite`) | exactly 6 inputs |
| 331–336 | 17203 ← 17155, 15601; 17195 ↔ 17196 (ping-pong); 17204 | **`_Afterimage`** (feedback with the previous frame 15601), blurred | the only stage with an input from the previous frame |
| 337 | 17157 ← 17186, 17204 | **glare `_Composite`**: `MadLumGammaT2` (bloom + afterimage; YEBIS-TECHNIQUES §3–§4) | 2 inputs |
| 338 | 17158 ← 17157 | final glare texture, resampled to 640×540 with `MadT1_S4` (also read by draw 300 of the next frame) | same programme as the bloom upsamples in the witness zones |
| 339 | 17221 (+DS 17222) ← 17151, 17158, 15605, 15600 | **`TonemapToSurface`**: `TonemapHDR_Dither_Exposure_Matrix_Gamma` (t0 scene, t1 glare, t2 exposure, t3 dither noise) | exactly the four inputs of the technique (TONEMAP-SKIN §15) |
| 340 | LDR target 4 (R8G8B8A8, 1280×720) ← 17221, 3D LUT 17490 | **`MadColorGradingLutT1`** (t0 + t16 3D LUT) | the only draw reading the 3D LUT |
| 341 (GX CS) | DoF table 10476 | `CustomDofLutCopy` | GX step 5 |

**Correction to TONEMAP-SKIN §16 (INFERRED from the inputs of draws 339 and 340)** ("tone map = final YEBIS draw"):

- On this frame the tone map writes an intermediate (17221).
- The **last** YEBIS draw is the colour-grading LUT into the LDR target.
- The tone-map formulas of §15/§16 are unchanged; only the order is refined.

**Sizes and formats observed in K-0410:**

- glare pyramid R11G11B10, 392×331 … 25×21;
- 784×294 and 392×147 RGBA16 (INFERRED: the DoF layers);
- a 640×540 target re-read next frame;
- 1×1 / 4×4 RGBA8;
- final 1280×720.

## 4. Per stage: techniques, constants, targets

### 4.1 Glare pyramid: sizes, levels, formats (STATIC, checked against the observation)

**Base size,** `CPostEffect::Initialize` `FUN_1416EBF80` (lines ~1660–1730):

```
q          = glare quality (Setting Info GlareQuality: 4 on m40 and default, TONEMAP-SKIN §17)
table[q]   = {scale, levels} at 14469B4D0:
             q0 {0.125, 2}  q1 {0.15, 3}  q2 {0.2, 4}  q3 {0.25, 5}
             q4 {0.375, 6}  q5 {0.5, 7}   q6 {0.75, 8}  q7 {1.0, 9}
A          = render aspect  (+0x328, 1280/720 = 1.7778)
r          = glare aspect divisor (+0x350; 1.5 when init flag bit 29, else 1.0)
baseW      = round(W · scale · sqrt((A/r) / A))      ; capped by W
baseH      = round(baseW / (A/r))
if baseW < 16 or baseH < 16: the smaller side becomes 16, the other scaled by the aspect
levels     = table[q].levels unless the caller overrides them (≤ 16)
```

- **Check:** W = 1280, q = 4, r = 1.5 → baseW = 1280 · 0.375 · √(1/1.5) = 391.9 → **392**;
  baseH = 392 / 1.1852 = 330.8 → **331**. This matches the observed 392×331. **Levels = 6**
  matches the six inputs of the bloom level combine.
- **Level surfaces,** `CRenderGlare::CreateSystemObjects` `FUN_141749790`:
  - level i, from 0: size = `(uint)(base · 2^-(i+1-d) + 0.99)` per axis, so rounded up; d =
    init field `+0xD4`; minimum 3 when < 4;
  - **correction (YEBIS-TECHNIQUES §4, OBSERVED in the witness zones):** the levels are
    **base, base, base/2, base/4, base/8, base/16**, i.e. 392×331 (×2), 196×166, 98×83, 49×42,
    25×21 at this base. The earlier list (196×166 … 7×6, d = 0) was wrong; d = 2 fits;
  - each level is a `CRenderTextureArray` of **3 surfaces**, 4 for the coarsest levels when the
    level's blur needs a double-width surface: ping-pong blur targets;
  - format: init field `+0xC4`, R11G11B10_FLOAT observed. When the caller leaves it 0, the
    library picks the first supported of R16G16B16A16_FLOAT, R16G16B16A16_UNORM,
    R10G10B10A2_UNORM, R8G8B8A8_UNORM, B8G8R8A8, B8G8R8X8 (`FUN_14170AC20`, usage RT|SRV).
- **Fixed glare surfaces** (allocated only when their stage is enabled):
  - source `+0x210`;
  - work targets `+0x238`, `+0x280`, `+0x288/+0x290/+0x298` (star/ghost/anamorphic);
  - afterimage history `+0x270[]` (count `+0xF0`, each initialised with weight 1.0);
  - light shafts `+0x248..+0x260`, `+0x2A0..+0x2B0`.
  - Validation is `FUN_141748EF0`: base ≥ 16, levels ≤ 16, second level set ≤ 16.
- **Per-level record** `+0x240[i]` (0x38 bytes), filled at creation:
  - `{k, scale = 2^-k, 2·scale (blur radius factor), surfaces 0..3}`;
  - the blur weights used per draw are computed from these and from `GlareBloomGaussianRadiusScale`
    (2 on m40) and `GlareBloomLuminanceGamma` (1.4 on m40) inside `GenerateGlare_Bloom`. That
    function's arithmetic is not decoded.
  - **Under the HAL route the library keeps computing them.** Under a reimplementation, one
    capture of the 18 level draws (§6, C3) gives the `afParam_TexCoordScaler`/`afRGBA_Modulate`
    values.
- **Composite weights:** `MadLumGammaT6` is `Σ_k level_k · Modulate[k]`, then the luminance
  gamma (TONEMAP-SKIN §15 "auto exposure (programmes)" gives the formula). `Modulate[k]` comes from
  the Bloom luminance terms `GlareLuminance` × the global glare factor `[[GXRS+0x5368]+0x68]`
  (TONEMAP-SKIN §17).

### 4.2 Techniques by stage (STATIC, technique table `evidence/I-0067/queries/yebis-techniques.tsv`)

| Stage | Technique families (name → VS/PS indices and SHA-256 in the table) | Notes |
|---|---|---|
| depth factor | `DepthOfField_BlurFactorAlphaFromDepthTextureTypeCastLut*` | selected by depth format and HDR encoding |
| DoF | `DepthOfField_*` (832 entries), `MadCocT2_Sn`, `DepthOfField_BackgroundLayerMaskT1_Sn` | `CTextureUtil::DrawRectGPU_MadCocT2_Sn`, `_DepthOfFieldLayerMask` |
| glare source / downsample | `BrightPass*` (54), `Mad*T1_Sn`, `TexCoordMadT1_Sn` | `DrawRectGPU_MadT1_Sn`, `_TexCoordMadT1_Sn` |
| bloom blur | `TexCoordOffsetMadTn` (n taps) | `DrawRectGPU_TexCoordOffsetMadTn` |
| composite | `MadLumGammaT1..T16` (n = levels) | `GenerateGlare_Composite` |
| afterimage | `MadOffsetTn` | `GenerateGlare_Afterimage` |
| tone map | `TonemapHDR_[Dither_][Vignette_]Exposure_[Matrix_]Gamma` (no curve suffix = sensitometric curve, TONEMAP-SKIN §16) | `DrawRectGPU_TonemapHDR` logs `HDRTMAP_IsPremap`, `bFullColorDither`, `bVignette` |
| colour grading | `MadColorGradingLutT1_S1`/`_S4` | PostProcess |
| luminance (AutoExposure only; off on m40) | `MadLumGammaTn`, `MaxColorLumGammaTn`, `AnalyzeIntensity` | `CTextureUtil::AnalyzeLuminance`, `GetResult_UpdateInternalLuminance` (read-back) |

- **Technique lookup** is by name at run time: `GetHandle_tech_<Name>(taps, samples)` builds
  the name, e.g. `tech_MadLumGammaTn` with n, then `CTinyEffect` finds it.
- **DXBC:** each technique row names its VS and PS blob. The 2551 blobs from file offset
  `0x2CC3B48` are pinned by SHA-256 in the table; `Get-DxbcDisassembly.ps1` disassembles them.
- **Correction (YEBIS-TECHNIQUES §4):** the vertex layout follows the technique's VS: 16 bytes
  for `vs_2D_Tex2D1`/`OffsetTex2Dn`, 24 `Tex2D2`, 32 `Tex2D3`, 40 `Tex2D4`, 48 `Tex2D5` (tone map).
  The vertices are written by `FUN_141718C00` into a dynamic ring buffer
  (§5.4).

### 4.3 Constants (STATIC)

- **Every** YEBIS shader reads one `$Globals` buffer at b0, 4096 bytes, shared by VS and PS
  (TONEMAP-SKIN §15 has the field offsets).
- `CTinyEffect` (`FUN_141775280`, "commit/BeginPass") does the following per draw:
  1. `Map(WRITE_DISCARD)`;
  2. writes only the variables the technique uses (offset `+0x48` of each variable binding):
     - type 1 scalar or array with stride;
     - type 2 vectors, copied as is;
     - type 3 `float4x4`, transposed;
  3. `Unmap`;
  4. `VSSetConstantBuffers(0, 1, cb)` and `PSSetConstantBuffers(0, 1, cb)`.
- The used fields per technique are the `ps_used_globals` column of the technique table.
- **Native:** one root CBV per draw, carved from an upload ring and sized to the highest used
  offset, rounded up to 256 bytes. The whole 4096 bytes can be used as a simple first step.

### 4.4 Textures and samplers per draw (STATIC)

- **Textures:** variable type 4. The commit collects the texture's SRV (`IGPUTexture+0xD8`) for
  `PSSetShaderResources(start, count)`, or `VSSetShaderResources` for vertex shaders. Up to 16
  PS slots and 4 VS slots are unbound at each commit.
- **Samplers:** each texture carries its own **sampler mode** (`IGPUTexture+0xE0`, 4 bits:
  filter in bits 0–1, address in bits 2–3; setters at texture slots 13/15/16).
  - At commit the texture applies it to the matching stage through `IGPUDevice` slot 50:
    `PSSetSamplers(stage)` for stages 0–15, `VSSetSamplers(stage − 16)` for 16–19.
  - The device keeps a per-stage cache `+0x9FC[20]`.

### 4.5 Targets (STATIC)

- `IGPUDevice` slot 6 `SetRenderTarget(i, surface)` does three things:
  - `OMSetRenderTargets(surface+0x120 count, surface+0x128 RTVs, surface+0x168 DSV)`;
  - stores the surface at `+0x8D0[i]`;
  - resets the viewport to the surface size (`+0xB0/+0xB4`).
- Slot 7 sets or drops the depth-stencil surface.
- Targets come from `CRenderTexturePool` / `CRenderTextureArray`.
  - Creation is `IGPUDevice::CreateTexture` (slot 61, `FUN_14176DB70`): `CreateTexture2D`
    (bind RT `0x20` / DS `0x40` / SRV `8`; staging + CPU read for read-back textures) then a
    `TEXTURE2D` SRV, with typeless formats resolved for depth.
  - Per-mip surfaces (RTV/DSV) are made by the texture's slot 7.

### 4.6 Tone map, LUT, dither (STATIC, already decoded)

- Formulas, curve constants (`T.x = 1 + 2.08914·FilmicContrast^(1/3)`, table `142B60C40`),
  exposure path and the 16³ LUT are in TONEMAP-SKIN §15–§16.
- m40 values: FilmicContrast 0.15 → `T = (2.1100, 0.12746, 0, 0)`; exposure 2.3; gamma 1/2.2.
- The dither input (t3) is a library noise texture (15600 in the capture).
- Vignette is off unless the `Vignette*` globals are set (0 on m40).

## 5. The HAL contract: `PPFX::IGPUDevice` and companions (STATIC)

### 5.1 Device object (0xA50 bytes; ctor `FUN_1417080E0`, init `FUN_1417074C0`, states `FUN_141707E70`)

| Offset | Content |
|---|---|
| `+0x10` / `+0x18` | `ID3D11DeviceContext` (GX's immediate context) |
| `+0x20` | `ID3D11Device` (from `context->GetDevice`) |
| `+0x28` / `+0x30` / `+0x38` | default rasterizer / depth-stencil / blend state |
| `+0x40 + 16·i`, i < 64 | `{ID3D11BlendState, ID3D11RasterizerState}` for render-state index i |
| `+0x440 + 8·j`, j < 16 | `ID3D11SamplerState` for sampler mode j |
| `+0x8D0[8]`, `+0x910` | bound render-target surfaces, depth-stencil surface |
| `+0x918[16]` | textures set by slot 13 (bookkeeping; binding happens in the effect commit) |
| `+0x998`, `+0x9A0` | index buffer and its offset |
| `+0x9A4..+0x9B8` | viewport x, y, w, h, minZ, maxZ (1.0) |
| `+0x9C0`, `+0x9C8` | default colour and depth surfaces adopted at Initialize |
| `+0x9D0` | GPU memory counter (bytes) |
| `+0x9E4` / `+0x9E8` | cached / current render-state index |
| `+0x9EC[4]` | blend factor |
| `+0x9FC[20]` | sampler-mode cache per stage |

### 5.2 Device states (STATIC, `FUN_141707780`, `FUN_1417079E0`, `FUN_141707CB0`)

**Render-state index (6 bits) = blend mode | write mask << 3 | scissor << 5:**

| Field | Values |
|---|---|
| blend mode (bits 0–2) | 0 opaque (blend off); 1 alpha `SRC_ALPHA / INV_SRC_ALPHA` (colour and alpha); 2 additive `ONE / ONE`; 3 screen `ONE / INV_SRC_COLOR` (alpha `ONE / INV_SRC_ALPHA`); 4 feedback `ONE / BLEND_FACTOR` (colour and alpha); op ADD everywhere |
| write mask (bits 3–4) | 0 RGBA, 1 RGB, 2 A |
| scissor (bit 5) | rasterizer `ScissorEnable` |

- Every rasterizer: solid, cull none, `FrontCounterClockwise = 1`, no depth bias, **depth clip
  off**, multisample on.
- Depth-stencil: one state, **depth and stencil disabled** (func ALWAYS).
- Defaults `+0x28/+0x30/+0x38`: the same rasterizer without scissor, the disabled depth-stencil,
  opaque blend with mask 0xF.

**Sampler mode (4 bits) = filter | address << 2:**

| Field | Values |
|---|---|
| filter (bits 0–1) | 0 `MIN_MAG_LINEAR_MIP_POINT`, MaxLOD 0; 1 `MIN_MAG_MIP_POINT`, MaxLOD 0; 2 `MIN_MAG_LINEAR_MIP_POINT`, MaxLOD FLT_MAX; 3 `MIN_MAG_MIP_LINEAR`, MaxLOD FLT_MAX |
| address U/V (bits 2–3) | 0 BORDER (border colour 0,0,0,0); 1 CLAMP; 2 MIRROR; 3 WRAP (W always CLAMP) |

Common to all: MipLODBias 0, MaxAnisotropy 1, no comparison, MinLOD 0.

### 5.3 Method table (STATIC; names from the library's diagnostics where present, otherwise INFERRED from behaviour)

| Slot | Method | D3D11 work | D3D12 equivalent |
|---|---|---|---|
| 0–2 | AddRef, Release, destructor | refcount; releases states and objects | fence-retire owned objects |
| 3 | IsSameDevice | compares contexts | identity |
| 5 | `Initialize` | adopts `OMGetRenderTargets` RTV/DSV as default surfaces, sets the viewport | native default surfaces = native LDR target (+ depth) |
| 6 | `SetRenderTarget(i, surf)` | `OMSetRenderTargets` + viewport | barriers to RENDER_TARGET; record RTV handles; viewport/scissor |
| 7 | `SetDepthStencilSurface(surf)` | rebinds RTVs with or without DSV | same, DSV handle or none |
| 8 / 12 | `SetIndices` / get | stores the IB (bound at draw) | IB view at draw |
| 9–11 | getters (RT i, DS, default surface) | — | — |
| 13 / 14 | `SetTexture(stage)` / get | bookkeeping only | — |
| 15 / 16 | `SetViewport` / get | `RSSetViewports` / `RSGetViewports` | `RSSetViewports` |
| 17 / 18 | `SetScissorRect` (null = viewport) | `RSSetScissorRects` | `RSSetScissorRects` |
| 19–37 | D3D9-era render-state and fixed-function calls | `E_NOTIMPL` (some log "not supported (DX10/11)"), or constant 0/1 | same constants |
| 38 / 39 | invalidate state / sampler caches | — | — |
| **40** | `ApplyRenderState(index, cached)` | `OMSetBlendState(blend[i], blendFactor, ~0)` + `RSSetState(raster[i])` | PSO key fields + `OMSetBlendFactor` |
| 41–48 | set/get the index, blend mode, write mask, scissor | — | — |
| 49 | `SetBlendFactor(float4)` | stores, invalidates | `OMSetBlendFactor` |
| **50** | `ApplySamplerState(stage, mode, cached)` | `PSSetSamplers(stage)` / `VSSetSamplers(stage−16)` | sampler table entry (§6.2) |
| 51 / 52 | `Clear(flags, colour, depth, stencil)` | `ClearRenderTargetView` on each RTV of RT 0 (flag bit 28), `ClearDepthStencilView` (flags & 3) | same on the native views (RENDER_TARGET / DEPTH_WRITE state) |
| 53 | unbind vertex streams | `IASetInputLayout(null)`, `IASetVertexBuffers(0, 32, null)` | — |
| 56 | `BeginScene` | — (returns 0) | — |
| **57** | `DrawPrimitive(topology, count, start)` | `IASetPrimitiveTopology` + `Draw` | `IASetPrimitiveTopology` + `DrawInstanced` |
| **58** | `DrawIndexedPrimitive(topology, count, start)` | `IASetIndexBuffer` + topology + `DrawIndexed` | IB view + `DrawIndexedInstanced` |
| 59 | `SetInputAttribute` | `IASetInputLayout` | PSO input layout key |
| 60 / 62 / 66 | `CreateTexture` / `CreateVolumeTexture` / `CreateSurface` from an existing resource (names STATIC from the slots' diagnostics, YEBIS-TECHNIQUES §1) | wrap an external D3D11 resource | wrap a native resource (scene HDR, depth, LDR target) |
| **61** | `CreateTexture(w, h, mips, format, msaa, flags, cpu)` | `CreateTexture2D` + SRV (`FUN_14176DB70`) | committed or placed resource + SRV; RT/DS flags → RTV/DSV heaps |
| 63 | `CreateVolumeTexture` (new) | **stub: returns null** (`FUN_14176E2C0`) | return null |
| 64 / 65 / 67 | `CreateRenderTarget` / `CreateDepthStencilSurface` / `CreateSurface` (corrected: 64 and 65 were swapped) | 2D texture + RTV / DSV | same |
| 68 / 69 | `CreateVertexBuffer` / `CreateIndexBuffer` (index R16 or R32) | dynamic buffers | upload-heap ring buffers |
| 70 | `CreateInputAttribute` | `CreateInputLayout` | input-layout description for the PSO |
| 71 | `CreateQueryOcclusion` | `CreateQuery` | query heap (unused by DS3's settings: INFERRED) |
| 72–76 | format and MSAA support queries | `CheckFormatSupport`, `CheckMultisampleQualityLevels` | `CheckFeatureSupport` |

**`IGPUTexture`** (vtable `142B67338`, 22 slots), **`IGPUSurface`** (`142B66AE8`, 14 slots):

- `+0xA8..+0xBC` desc (type, format, w, h, …);
- `+0xD8` SRV;
- `+0xE0` sampler mode;
- `+0xE8[]` per-mip surfaces;
- surface `+0x120/+0x128/+0x168` RTV count, RTVs, DSV;
- texture slot 19 applies its sampler mode to a stage;
- surface slot 6 `UpdateSubresource`;
- `CRenderTextureArray::CopyToSurface` and `CRenderTexture::Resolve*` use copy and resolve
  (`CopySubresourceRegion` / `ResolveSubresource`, INFERRED from the method names; DS3 runs
  without MSAA).

### 5.4 The draw path (STATIC, `FUN_14171BE50`)

For each pass, `DrawRectGPU_*` does the following:

1. `ApplyRenderState(-1, cached)` (slot 40).
2. `effect->Begin` + `BeginPass(0)`: CTinyEffect +0xA8/+0xB8, through IGPUEffect slots 29/31,
   which commit shaders, constants, textures and samplers (§4.3–§4.4).
3. `FUN_141718C00` builds the vertices (count = `aiStack_118[0]`, 3 for the full-screen
   triangle) into a 16-byte-per-vertex dynamic VB:
   - a ring at `CPostEffect+0x21778`, offset `+0x21768`, size `+0x2176C`;
   - lock with NO_OVERWRITE, or DISCARD on wrap.
4. `SetStreamSource` (input layout `+0x21658`, VB, offset).
5. `SetIndices(+0x21780)`.
6. **`DrawIndexedPrimitive(5 = TRIANGLESTRIP, count)`**. OBSERVED as `DrawIndexed(3)`.
7. `EndPass`/`End`: PS and VS SRVs unbound.

## 6. D3D12 equivalent (INFERRED design from §4–§5; layout is the pilot's choice)

### 6.1 PSO key

```
PSO = { VS blob, PS blob            (from the technique row: vs_index, ps_index; 2551 pinned blobs)
        input layout                 (the YEBIS 2D layout: POSITION float2, TEXCOORD float2, 16 B)
        blend   = blend mode (5) × write mask (3)       (index bits 0–4, §5.2)
        raster  = cull none, FrontCCW, depth clip OFF, scissor bit (bit 5 → always enable scissor
                  in D3D12 and set a full-target rect when the bit is clear)
        depth-stencil = disabled (DSV may be bound without depth test: 339 binds 17222)
        RTV format (+ DSV format when bound), sample count 1
        topology type TRIANGLE }
```

**Expected PSO count:** the techniques actually used in a world frame (§4.2; INFERRED about 15 distinct
pairs on m40) × the blend and mask combinations they use (mostly opaque; additive or feedback for
afterimage and composite). Create PSOs lazily and cache them by key.

### 6.2 Root signature (one for all YEBIS draws)

| Parameter | Content | Visibility |
|---|---|---|
| 0 | root CBV b0: the `$Globals` block of the draw (upload ring, ≤ 4096 B) | ALL (VS and PS share it) |
| 1 | SRV table t0..t15 | PIXEL |
| 2 | SRV table t0..t3 | VERTEX (DoF scatter/bokeh vertex fetch; INFERRED rare) |
| 3 | sampler table s0..s15 | PIXEL |
| 4 | sampler table s0..s3 | VERTEX |

- Samplers: create the **16 modes of §5.2 once** in a non-visible heap.
- Per draw, copy the modes the bound textures request into a shader-visible sampler range.
  Static samplers cannot follow the per-texture mode.
- t16 (3D LUT of `MadColorGradingLutT1`): either widen the PS table to t0..t16 or remap the
  register at load. The reflection names the register.

### 6.3 Resources and barriers

- **Scene input** (17150 equivalent, R16G16B16A16F, full resolution):
  1. RENDER_TARGET for `PointSampleColorClamp1` and draw 300;
  2. then PIXEL_SHADER_RESOURCE for the DoF and glare reads.
- **Every library target:** RENDER_TARGET while it is the bound target of `SetRenderTarget`,
  PIXEL_SHADER_RESOURCE when a later draw samples it.
  - The HAL knows both events (slot 6 binds, the commit lists the SRVs), so the native HAL can
    issue the transitions itself, batched before each draw.
  - Ping-pong pairs (17195↔17196, the per-level surfaces) flip every draw.
- **Temporal resources** (keep across frames, never alias):
  - the final glare 17158 (read by the next frame's draw 300);
  - the afterimage history (15601, `+0x270[]`);
  - the DoF table 10476 (GX compute, UAV → SRV barrier before the next frame's draw 300);
  - the eye-adaptation target when AutoExposure is on (off on m40).
- **Read-backs (AutoExposure only):** `GetResult_UpdateInternalLuminance` reads a small staging
  texture. Natively: `CopyTextureRegion` into a READBACK buffer, read one or more frames later
  after the fence. The library already tolerates the delay (the D3D11 path used staging
  `Map`).
- **3D LUT:** GX compute `LutCopy2DTo3DCs` writes it (UAV). Barrier to PIXEL_SHADER_RESOURCE
  before draw 340.
- **Final output:** the native LDR target, RENDER_TARGET for draw 340. Then the GX after-chain
  (`PointSampleColor1`, `DLAA`, glow: TONEMAP-SKIN §17) and the interface.

### 6.4 Lifecycle

| When | Original | Native duty |
|---|---|---|
| scene setup | `FUN_1400E2B20`: `FUN_1416C87D0(IPfxContext_Dx11(GX context), size, flags)` → `CPostEffect::Initialize` → device states, pools, glare, DoF | create the native HAL objects first; pass a native context object; same init parameters (size, flags) |
| per frame | `FUN_1400E5110`: LUT CS → configure + `BeginPostEffectScene` → `PointSampleColorClamp1` → `EndPostEffectScene` → `CustomDofLutCopy` CS | same order on the native command list; the HAL records into it |
| resize | `CPostEffect` re-initialised with the new size (pool and glare sizes follow §4.1) | retire after the fence, recreate |
| shutdown | `Uninitialize`, device destructor (`FUN_141708210`, `FUN_141707EE0` releases the 64 + 16 + 3 states) | fence-retire |

## 7. What stays open, and how to close it

1. **Constant values per draw** for a reimplementation: the HAL route does not need them.
   - Otherwise one original capture of a world frame gives the `$Globals` of the 41 draw calls.
   - TONEMAP-SKIN §16 already predicts the tone-map ones.
2. ~~Exact technique per draw~~: closed in YEBIS-TECHNIQUES §2–§4 (handle tables, selection rules, five witness zones), except the DoF aperture pre-pass, the DoF composite variant and the exact blur tap counts.
   - The pilot can confirm by hashing the PS bound at each YEBIS draw and looking it up in the
     technique table (SHA-256 column).
   - With the native HAL this is automatic: the HAL sees the technique name at `GetHandle`.
3. ~~`GenerateGlare_Bloom` weight arithmetic~~: decoded in YEBIS-TECHNIQUES §5.
4. ~~Slot names 60, 62, 64–67~~: read from the slots' own diagnostics (YEBIS-TECHNIQUES §1).

## Exports

`decompiled/DS3-1152-F-017080E0` (device ctor), `017074C0` (init), `01707E70`/`01707780`/
`017079E0`/`01707CB0` (states), `017071E0` (Initialize), `017082E0`/`017083C0`
(SetRenderTarget/DS), `01708F90` (ApplyRenderState), `01709240` (ApplySamplerState), `017093C0`
(Clear), `01709600`/`01709650` (draws), `0176DB70` (CreateTexture), `01775280` (effect commit),
`0171BE50` (DrawRectGPU core), `016E8200` (EndPostEffectScene_Process), `016E5310`,
`016E5BC0`, `016C9310`, `016C9330`, `01749790` (glare objects), `017524B0` (glare init),
`01748EF0` (validation), `016C87D0` (YEBIS init), `000E5110`, `000E2B20` (GX side). Query
outputs: `evidence/I-0068/queries/yebis/`.
