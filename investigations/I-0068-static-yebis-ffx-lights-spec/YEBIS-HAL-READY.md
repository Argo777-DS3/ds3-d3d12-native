# I-0068 part I: native YEBIS HAL (`PPFX::IGPUDevice` on D3D12) with the gamma pass, ready to code

Request (coordinator, 2026-10-10): the same "ready to code" format, because the native world is
brighter than the original for lack of the tone map. The note covers:

- the ordered CALL / TRANSCRIBE / NATIVE list of the 77 slots;
- resource creation (creators 60–67);
- lifecycle and resize;
- PSOs from `CTinyEffect` / `TexUtil_D3D11`;
- the `$Globals` CB;
- barriers;
- the hook-up to the native HDR target and to the back buffer through the gamma pass;
- checks at m40 (the draws in order) and on m31 (light shafts), and the risks.

It builds on:

- YEBIS-NATIVE (HAL decoded, D3D12 design §6);
- YEBIS-TECHNIQUES (technique names per draw);
- DISPLAY-SWAPCHAIN-NATIVE §9 (gamma pass).

Labels: STATIC, INFERRED, OBSERVED. Evidence: `evidence/I-0068/queries/yebis/hal/` (decompile
index of every slot of the five interfaces, `hal-slots.tsv`).

**Rule.** The native device keeps the original object layout (0xA50 bytes, YEBIS-NATIVE §5.1),
so every slot that only reads or writes that layout is **CALL**: the original function runs on
the native object. A slot that calls `ID3D11Device` (`+0x20`) or `ID3D11DeviceContext`
(`+0x10/+0x18`), directly or through a creator, is **NATIVE**.

## 1. Where the HAL is created (STATIC)

| # | Step | Original | Mode |
|---|---|---|---|
| 1 | scene setup | `FUN_1400E2B20` → `FUN_1416C87D0(IPfxContext_Dx11(GX context), size, flags)` (`CPostEffect`, 0xC90 at `[1448E5E38]`, `FUN_1416EBC30`) | CALL, with a **native `IPfxContext`** (vtable `142B4C4A0`, 5 slots: `1416CD950`, `1416CD960`, `1416CD9A0`, `1416CD860` dtor, `1416C8CC0`) handing out the native device token instead of the GX context |
| 2 | `CPostEffect::Initialize` (`FUN_1416EBF80`) | `FUN_14170ACC0(?, d3dContext)`, the device factory: alloc 0xA50 from the YEBIS allocator `PTR_PTR_14469AB80` → ctor `FUN_1417080E0` → init `FUN_1417074C0` (stores the context, `GetDevice` into `+0x20`, then the 64 + 16 + 3 states of §5.2) | **NATIVE factory**: same allocation and ctor (CALL); native init creates no D3D11 states, only the native PSO/sampler caches; **vtable = a copy of `142B598C8` with the NATIVE slots of §2 replaced** |
| 3 | library objects (`CTextureUtil`, `CRenderGlare`, `CDepthOfField`, pools) | `CPostEffect::Initialize` | CALL: they create their GPU objects through the HAL creators (§3) |
| 4 | per frame | `FUN_1400E5110`: LUT CS → configure → `BeginPostEffectScene` → `PointSampleColorClamp1` → `EndPostEffectScene` → `CustomDofLutCopy` CS | CALL (YEBIS); the two GX compute passes and `PointSampleColorClamp1` are native GX steps |
| 5 | resize | `CPostEffect` re-initialised with the new size | CALL; the native HAL retires its resources after the fence |
| 6 | shutdown | `Uninitialize`, device dtor (slot 2) | CALL, then native slot 2 |

## 2. `IGPUDevice`: the 77 slots (vtable `142B598C8`)
| Slot | Address | Method | Mode |
|---|---|---|---|
| 0 | `14176D2E0` | AddRef | CALL |
| 1 | `14176D2F0` | Release | CALL |
| 2 | `14170AD80` | destructor (releases states and objects) | **NATIVE** |
| 3 | `141708580` | IsSameDevice | CALL |
| 4 | `1417085B0` | (returns 0) | CALL |
| 5 | `1417085C0` | Initialize (adopts OMGetRenderTargets, viewport) | **NATIVE** |
| 6 | `141708610` | SetRenderTarget(i, surf) | **NATIVE** |
| 7 | `141708620` | SetDepthStencilSurface | **NATIVE** |
| 8 | `141708630` | SetIndices | CALL |
| 9 | `141708690` | GetRenderTarget(i) | CALL |
| 10 | `1417086E0` | GetDepthStencilSurface | CALL |
| 11 | `141708720` | GetDefaultSurface | CALL |
| 12 | `141708750` | GetIndices | CALL |
| 13 | `141708790` | SetTexture(stage) (bookkeeping) | CALL |
| 14 | `1417087F0` | GetTexture | CALL |
| 15 | `141708840` | SetViewport (RSSetViewports) | **NATIVE** |
| 16 | `141708920` | GetViewport (RSGetViewports) | **NATIVE** |
| 17 | `141708A90` | SetScissorRect (RSSetScissorRects, null = viewport) | **NATIVE** |
| 18 | `141708A00` | GetScissorRect / reapply | **NATIVE** |
| 19 | `141708AF0` | D3D9-era call: E_NOTIMPL or constant | CALL |
| 20 | `141708B00` | D3D9-era call: E_NOTIMPL or constant | CALL |
| 21 | `141708B10` | D3D9-era call: E_NOTIMPL or constant | CALL |
| 22 | `141708BA0` | D3D9-era call: E_NOTIMPL or constant | CALL |
| 23 | `141708C30` | D3D9-era call: E_NOTIMPL or constant | CALL |
| 24 | `141708C40` | D3D9-era call: E_NOTIMPL or constant | CALL |
| 25 | `141708CD0` | D3D9-era call: E_NOTIMPL or constant | CALL |
| 26 | `141708CE0` | D3D9-era call: E_NOTIMPL or constant | CALL |
| 27 | `141708CF0` | D3D9-era call: E_NOTIMPL or constant | CALL |
| 28 | `141708D00` | D3D9-era call: E_NOTIMPL or constant | CALL |
| 29 | `141708D10` | D3D9-era call: E_NOTIMPL or constant | CALL |
| 30 | `141708D20` | D3D9-era call: E_NOTIMPL or constant | CALL |
| 31 | `141708D30` | D3D9-era call: E_NOTIMPL or constant | CALL |
| 32 | `141708D40` | D3D9-era call: E_NOTIMPL or constant | CALL |
| 33 | `141708D50` | D3D9-era call: E_NOTIMPL or constant | CALL |
| 34 | `141708D60` | D3D9-era call: E_NOTIMPL or constant | CALL |
| 35 | `141708D70` | D3D9-era call: E_NOTIMPL or constant | CALL |
| 36 | `141708D80` | D3D9-era call: E_NOTIMPL or constant | CALL |
| 37 | `141708D90` | D3D9-era call: E_NOTIMPL or constant | CALL |
| 38 | `141708ED0` | invalidate render-state cache | CALL |
| 39 | `141708EF0` | invalidate sampler cache | CALL |
| 40 | `141708F90` | ApplyRenderState(index) | **NATIVE** |
| 41 | `1417090F0` | set render-state index | CALL |
| 42 | `141709110` | get render-state index | CALL |
| 43 | `141709120` | set blend mode | CALL |
| 44 | `141709150` | set write mask | CALL |
| 45 | `141709180` | set scissor bit | CALL |
| 46 | `1417091B0` | get blend mode | CALL |
| 47 | `1417091C0` | get write mask | CALL |
| 48 | `1417091E0` | get scissor bit | CALL |
| 49 | `141709200` | SetBlendFactor (→ slot 38) | CALL |
| 50 | `141709240` | ApplySamplerState(stage, mode) | **NATIVE** |
| 51 | `1417094B0` | Clear(ARGB) → slot 52 | CALL |
| 52 | `1417093C0` | Clear (ClearRenderTargetView / ClearDepthStencilView) | **NATIVE** |
| 53 | `141709540` | unbind vertex streams | **NATIVE** |
| 54 | `14170ADD0` | store +0x9D4/+0x9D8 | CALL |
| 55 | `14170ADE0` | store +0x9DC | CALL |
| 56 | `1417095F0` | BeginScene (0) | CALL |
| 57 | `141709600` | DrawPrimitive | **NATIVE** |
| 58 | `141709650` | DrawIndexedPrimitive | **NATIVE** |
| 59 | `14170ADB0` | SetInputAttribute (IASetInputLayout) | **NATIVE** |
| 60 | `141709AC0` | CreateTexture (wrap existing) | **NATIVE** |
| 61 | `141709720` | CreateTexture | **NATIVE** |
| 62 | `141709C70` | CreateVolumeTexture (wrap) | **NATIVE** |
| 63 | `141709900` | CreateVolumeTexture (stub, null) | **NATIVE** |
| 64 | `141709E30` | CreateRenderTarget | **NATIVE** |
| 65 | `141709FE0` | CreateDepthStencilSurface | **NATIVE** |
| 66 | `14170A380` | CreateSurface (wrap) | **NATIVE** |
| 67 | `14170A1D0` | CreateSurface | **NATIVE** |
| 68 | `14170A540` | CreateVertexBuffer | **NATIVE** |
| 69 | `14170A6F0` | CreateIndexBuffer | **NATIVE** |
| 70 | `14170A8B0` | CreateInputAttribute (CreateInputLayout) | **NATIVE** |
| 71 | `14170A980` | CreateQueryOcclusion | **NATIVE** |
| 72 | `14170AA30` | CheckMultisampleQualityLevels | **NATIVE** |
| 73 | `14170AA50` | format query → slot 72 | CALL |
| 74 | `14170AAB0` | CheckFormatSupport | **NATIVE** |
| 75 | `14170AB70` | format query → slot 74 | CALL |
| 76 | `14170ABD0` | format query → slot 74 | CALL |

- **CALL slots** only touch the 0xA50 layout: refcount, getters, the render-state index and
  blend factor bookkeeping (41–49), the D3D9-era stubs 19–37, cache invalidation 38/39, 54–56,
  and the three format-query wrappers that call 72/74.
- **NATIVE slots and their D3D12 behaviour:**

| Slot | D3D12 behaviour |
|---|---|
| 2 | release the native objects through the fence |
| 5 Initialize | adopt the native default surfaces: the native scene HDR (input) and the native LDR target (`+0x9C0/+0x9C8`) |
| 6 / 7 | record RTV/DSV handles in `+0x8D0[8]` / `+0x910`; on the next draw: barriers (§6) + `OMSetRenderTargets` + viewport |
| 15 / 16 | viewport into `+0x9A4..+0x9B8`; `RSSetViewports` at draw |
| 17 / 18 | scissor (`RSSetScissorRects`; null = full viewport); D3D12 always has the scissor enabled |
| 40 | PSO key fields: blend mode, write mask, scissor bit (§4) |
| 50 | sampler mode per stage → shader-visible sampler copy (§4.2) |
| 52 | `ClearRenderTargetView` / `ClearDepthStencilView` (target in RENDER_TARGET / DEPTH_WRITE) |
| 53 | nothing (D3D12 has no persistent IA state) |
| 57 / 58 | `IASetPrimitiveTopology` + `DrawInstanced` / IB view + `DrawIndexedInstanced`, after the effect commit (§5) |
| 59 / 70 | the input layout is part of the PSO key: one YEBIS 2D layout (POSITION float2 + TEXCOORD float2, 16 B), plus the strides 24/32/40/48 of `vs_2D_Tex2Dn` (YEBIS-TECHNIQUES §4) |
| 60–67 | §3 |
| 68 / 69 | vertex/index buffers on the upload ring (dynamic; NO_OVERWRITE/DISCARD semantics become ring offsets) |
| 71 | occlusion query → query heap (unused by DS3's settings: INFERRED) |
| 72 / 74 | `CheckFeatureSupport` (format support, MSAA levels) |

## 3. Resource creation: creators 60–67 (STATIC names, part F)

| Slot | Name | Back end (STATIC) | Native |
|---|---|---|---|
| 60 | `CreateTexture` from an existing resource | `FUN_14176DF60`: `IGPUTexture` (0xF8, vtable `142B67338`), `vt+0x48(resource, sRGB)` | wrap a native resource: the scene HDR, the depth, the DoF table, the 3D LUT (one SRV each) |
| 61 | `CreateTexture(w, h, mips, format, msaa, flags, cpu)` | `FUN_14176DB70`: `CreateTexture2D` + SRV | committed resource + SRV; RT/DS flags → RTV/DSV; initial state PIXEL_SHADER_RESOURCE |
| 62 | `CreateVolumeTexture` wrap | `FUN_14176E3C0` → `FUN_14176E2D0` | wrap the native 3D LUT (`LutCopy2DTo3DCs` output) |
| 63 | `CreateVolumeTexture` | `FUN_14176E2C0` returns **null** | return null (same behaviour) |
| 64 | `CreateRenderTarget` | `FUN_14176CD00` | resource + RTV + SRV |
| 65 | `CreateDepthStencilSurface` | `FUN_14176CD00` | resource + DSV (+ SRV when typeless) |
| 66 | `CreateSurface` wrap | `FUN_14176C260` (0x170 `IGPUSurface`, vtable `142B66AE8`) | wrap the native LDR target and the scene targets the library draws into |
| 67 | `CreateSurface` | `FUN_14176CD00` | resource + view |

- The **format map** is the library's (`FUN_141702090` name, `FUN_141702AD0`, `FUN_141703440`
  sRGB); keep it (CALL), and map its D3D format to the same DXGI format.
- **Companion objects** keep their original layouts:
  - `IGPUTexture` (22 slots, `142B67338`);
  - `IGPUSurface` (14 slots, `142B66AE8`): `+0xA8..+0xBC` desc, `+0xD8` SRV, `+0xE0` sampler mode,
    `+0xE8[]` mip surfaces, `+0x120/+0x128/+0x168` RTVs/DSV.
- **Their D3D-touching slots are native** (the ones that call the helpers `FUN_14176B540`,
  `14176BA90`, `14176BDA0`, `1417711A0`, `1417713C0`, `14176BF60`, `14176D0F0`, `14176BE80`,
  `14176B970`, `14176C0F0`, `141701F50`, `14176BB70`, `14176D830`, `14176C380`):
  - texture slots 2, 7, 8, 9;
  - surface slots 2, 6 (`UpdateSubresource`), 7, 8, 12, 13 (copy / resolve).
- All other companion slots are getters (CALL).
- **Native view handles:** store the native descriptor handles in the same fields the CPU getters
  read (`+0xD8` SRV, `+0x128[]` RTVs, `+0x168` DSV). The CALL slots then hand them out unchanged.

## 4. PSOs from `CTinyEffect` / `TexUtil_D3D11`

- **Technique table:** the `TexUtil_D3D11` text at `142BDB730` (2555 lines
  `tech, vs, vs_index, ps, ps_index`, then the 35 global parameters).
  - `CTinyEffect` parses it on the CPU (CALL).
  - The blobs are pinned by SHA-256 in `evidence/I-0067/queries/yebis-techniques.tsv`; their
    traits are in `yebis-technique-traits.tsv` (VS stride, PS texture count).
- **`CTinyEffect` vtable `142BDB618` (26 slots):**
  - slots 8–15 copy variable values into the staging block (`memcpy`; CALL);
  - **slot 23 (thunk `141776880`) → `FUN_141775280`** (commit = BeginPass) and slot 24 (`1417768C0`) → `FUN_141775B50` (end) are
    **NATIVE**;
  - shader creation from the blobs (the D3D11 `CreateVertexShader`/`PixelShader` on first use;
    YEBIS creates programmes lazily, OBSERVED in the retained captures) becomes **PSO creation on
    first use**.
- **PSO key:**
  `{technique (VS blob, PS blob), input layout (by VS: 16/24/32/40/48 B), render-state index (blend 0–4 × write mask 0–2 × scissor), RTV format(s), DSV format or none, sample count 1, TRIANGLE}`.
  - Raster: solid, cull none, FrontCCW, depth clip off.
  - Depth-stencil: disabled.
  - Cache by key; build on first use. INFERRED: about 30 distinct PSOs in a world frame (the
    techniques of YEBIS-TECHNIQUES §4 × their blend modes).
- **Root signature** (YEBIS-NATIVE §6.2):
  - root CBV b0 (ALL);
  - SRV table t0..t16 (PIXEL; t16 = the 3D LUT of `MadColorGradingLutT1`);
  - SRV t0..t3 (VERTEX, rare);
  - sampler tables s0..s15 (PIXEL) and s0..s3 (VERTEX).
  - The 16 sampler modes of YEBIS-NATIVE §5.2 are created once (non-visible heap) and copied per
    draw.

### 4.1 The `$Globals` CB (STATIC, YEBIS-NATIVE §4.3)

- One 4096-byte block shared by VS and PS.
- At commit, only the variables the technique uses are written (`ps_used_globals` column):
  - type 1 scalar/array;
  - type 2 vector;
  - type 3 `float4x4` transposed.
- **Native:** allocate `round_up(highest used offset, 256)` bytes from the upload ring per draw,
  write the same values the commit writes (the staging block the CALL slots 8–15 filled), and
  bind them as root CBV b0.
- The 35 global parameters and their array sizes are listed at the end of the technique table.

## 5. Draw path (STATIC `FUN_14171BE50` → device slots)

1. Slot 40 `ApplyRenderState(-1)`.
2. Effect slot 23: the native commit:
   - PSO from the key;
   - root signature;
   - `$Globals` CBV;
   - SRV and sampler tables from the textures and sampler modes of the bound stages.
3. `FUN_141718C00` writes the vertices into the library's dynamic VB (device slot 68 buffer: the
   native ring).
4. `SetStreamSource` / `SetIndices`.
5. **Slot 58 `DrawIndexedPrimitive(TRIANGLESTRIP, n)`** (OBSERVED `DrawIndexed(3)`).
6. Effect slot 24 (end): unbind.

## 6. Barriers (NATIVE HAL issues them itself; YEBIS-NATIVE §6.3)

- **Before each draw:**
  - the bound RT(s) → RENDER_TARGET;
  - every SRV of the commit → PIXEL_SHADER_RESOURCE;
  - batch them.
  - Ping-pong pairs (bloom levels, afterimage 17195↔17196, light-shaft radial pair) flip every
    draw.
- **Temporal resources**, never aliased:
  - the final glare (read by the next frame's depth-factor draw);
  - the afterimage history;
  - the DoF table 2048×2 R32F (GX compute UAV → SRV before the next draw 300);
  - the 3D LUT (UAV → SRV before the grading draw).
- **Scene input** (GX `PointSampleColorClamp1` output): RENDER_TARGET → PIXEL_SHADER_RESOURCE
  after draw 300. Draw 300 also writes it: the depth factor goes into its alpha, then the
  depth-to-alpha pass of the light shafts.
- **Final output:**
  - the native LDR target (R8G8B8A8) is RENDER_TARGET for the last YEBIS draw (grading on m40/m31;
    tone map on m37/m45/m50/m51);
  - then the GX after-chain (`PointSampleColor1`, DLAA, glow; TONEMAP-SKIN §17), then the
    interface.

## 7. Hook-up: native HDR → YEBIS → LDR → gamma pass → back buffer

1. **Input:** the native lit HDR target (R16G16B16A16_FLOAT, the frame copy's source,
   FFX-FAMILIES-PHASE2 §8), copied by the GX step `PointSampleColorClamp1` into the YEBIS scene
   input. That keeps YEBIS's own input.
2. **YEBIS** draws as above. Its default surface (slot 5) is the native LDR target.
3. **After-chain and interface** on the LDR target (existing native steps).
4. **Gamma pass** (DISPLAY-SWAPCHAIN-NATIVE §9):
   - the LDR target is the gamma pass's intermediate (t0);
   - the 1025×3 curve (t1) comes from the brightness option;
   - the pass draws into the swap-chain back buffer;
   - then `Present`.
   - Without it, brightness ≠ 5 is ignored. With brightness 5 the curve is identity.

## 8. Checks

**m40 witness** (`evidence/I-0053/artifacts/c1/gpu-order-i52-live-03-A-frame-300.txt`, lines
300–340; YEBIS-NATIVE §3.2 corrected by YEBIS-TECHNIQUES §4). The frame has **41 YEBIS draw
calls, i.e. 42 draws** (line 309 is `x2`), not 35. The count in YEBIS-NATIVE §3.2 is corrected
(OBSERVED: `raw12` lines between `PointSampleColorClamp1` and `CustomDofLutCopy`).

| Order | Draw(s) | Technique (expected) | Target |
|---|---|---|---|
| 1 | 300 | `DepthOfField_BlurFactorAlphaFromDepthTextureTypeCastLutT1_S1_<ch>` | scene input |
| 2 | 301 | `MadT1_S2` | 784×294 |
| 3 | 302 | `MadT1_S1` | 392×147 |
| 4 | 303–308 | 3 × (`DepthOfField_FrontBlurBlendFactor`, blur `MadT1_Sn`) | layers |
| 5 | 309 (2 events) | `DepthOfField_CompositeBlurLevelsFrontBlur[_USNORM]_TS0_3` | full res |
| 6 | 310 | `MadT1_S1` | 640×360 |
| 7 | 311 | `BrightPassT1_[Vignette_]THRESHOLD_LUMINANCE_REMAP_S1` | 392×331 |
| 8 | 312–329 (18) | bloom: L0 and L1 blur pairs at 392×331; then per level `MadT1_S1` + pair; `MadT1_S4` up for 49×42 and 25×21 | pyramid |
| 9 | 330 | `MadLumGammaT6` | 392×331 |
| 10 | 331–336 | afterimage (`MadOffsetTn` family + blurs) | history pair |
| 11 | 337 | `MadLumGammaT2` | 392×331 |
| 12 | 338 | `MadT1_S4` | 640×540 |
| 13 | 339 | `TonemapHDR_…` (VS `vs_2D_Tex2D5`, 4 textures) | intermediate |
| 14 | 340 | `MadColorGradingLutT1_S1` (t16 3D LUT) | LDR 1280×720 |

- **H1:** 41 draw calls (42 draws) between `PointSampleColorClamp1` and `CustomDofLutCopy`, with
  the same order, the same render-target/input graph as lines 300–340, the same sizes, the same formats (R11G11B10 pyramid, RGBA16 DoF and
  scene).
- **H2:** the native world luminance matches the original at the m40 view. The tone map is
  present; the remaining difference is within the earlier native comparison's non-YEBIS gaps.
- **H3:** brightness 10 and 0 change the back buffer by the curve of DISPLAY-SWAPCHAIN-NATIVE
  §9.4 (exponent 0.815 / 1.294).

**m31 witness** (retained capture `i63-live-10/m31-s3/frame-60`, 48 YEBIS draws per frame, in
order):

| Order | Draws | Content |
|---|---|---|
| 1 | 1–10 | DoF (as m40, 1 + 2 + 6 + 1) |
| 2 | 11 | **depth → scene alpha** (`Alpha_<ch>`) |
| 3 | 12–13 | glare source `MadT1_S1` ×2 (→ 640, → 320) |
| 4 | 14 | bright pass (Vignette, `vs_2D_Tex2D2`) → 210×177 |
| 5 | 15–37 | bloom (23): 7 levels 210, 210, 105, 53, 27, 14, 7 (L0, L1 a blur pair each; then a `MadT1_S1` down + pair); `MadT1_S4` ×2 ups for 53, 27, 14, 7 |
| 6 | 38 | `MadLumGammaT7` (7 inputs) |
| 7 | 39 | `LightShaftMask_SM3` (t1 = film grain 256×256) |
| 8 | 40–43 | `TexCoordMadT1_S4` ×4 radial |
| 9 | 44 | `TexCoordMadT1_S2` |
| 10 | 45 | `MadLumGammaT2` |
| 11 | 46 | `MadT1_S4` → 420×354 |
| 12 | 47 | tone map → 1280×720 (R8G8B8A8_TYPELESS) |
| 13 | 48 | `MadColorGradingLutT1_S1` (+ 16×16 R32G32B32A32 LUT) → R8G8B8A8 |

- **H4:** 48 draws per frame in this order with these sizes (OBSERVED in the retained capture).

## 9. Risks

1. **Lazy programme creation.** YEBIS creates shaders on first technique use. The first frames
   build PSOs: pre-build the m40 set at scene setup to avoid hitches.
2. **The scissor bit** (render-state bit 5). D3D12 always scissors; a missing full-target rect
   when the bit is clear clips draws.
3. **Surfaces wrapping native resources** (slots 60/62/66). Their lifetime follows the native
   resource. Resize must re-wrap them, as `CPostEffect` re-initialisation does.
4. **The DoF composite USNORM suffix** and the blur tap counts are chosen by the library at run
   time. The native HAL only builds what is asked, so no static choice is needed.
5. **AutoExposure read-back** (off on m40 and the witness zones): if enabled elsewhere, a
   one-frame-late `READBACK` copy is required (YEBIS-NATIVE §6.3).

## Exports

The 139 slot functions of the five interfaces are indexed in `evidence/I-0068/queries/yebis/hal/`
(`hal-slots.tsv`: interface, slot, address, size, D3D call or log, callees; `slot-table.md`: the
§2 table; `vt-*.txt`: the five vtables). Decompiled exports in `decompiled/`: the factory
`0170ACC0`, `016C87D0`, `017074C0`, the commit `01775280`, the effect thunks `01776880` and
`017768C0`, the five `IPfxContext_Dx11` slots, and every NATIVE device slot of §2.
