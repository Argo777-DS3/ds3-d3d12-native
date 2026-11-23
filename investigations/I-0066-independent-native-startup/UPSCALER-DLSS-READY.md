# I-0066 / I-0068 part BQ: the temporal upscaler (DLSS) ready to implement: render and output extents, jitter, motion vectors, reactive mask, history reset

Request (coordinator, 2026-10-11): write the DLSS sheet: compatible motion vectors, jitter, render resolution, history reset and reactive mask, from what the engine does and from the YEBIS passes that depend on them; deliver it merged into `main`. Static: nothing was launched, native code was read and not changed. Same form as [YEBIS-SECOND-TRANCHE-READY](YEBIS-SECOND-TRANCHE-READY.md): answer first, facts, design, commits under opt-in flags, tests with goldens, live rows, corrections, review log.

Labels: **verified** = read in the decompilation of `DarkSoulsIII.exe` 1.15.2.0, in the programme cache scans of this part, or in the native tree at the line quoted; **observed** = retained captures; **note** = a linked note; **derived** = follows by algebra from verified facts (a test checks it); **external** = about the NVIDIA SDK, read from the public headers (`NVIDIA/DLSS` on GitHub: `nvsdk_ngx_helpers_d3d.h`, `nvsdk_ngx_defs.h`, fetched and summarised in this session) or recalled from the programming guide, which could not be fetched: every external line must be re-read against the SDK actually downloaded; **proposed**; **inferred**; **open**. Abbreviations: `GB` = `native/frame/game_bootstrap.cpp`, `SP` = `native/renderer/scene_pass.cpp`, `PR` = `native/world/producers.cpp`, `SV` = `native/renderer/scene_view.cpp`, `VH` = `native/renderer/velocity.h`.

Evidence (outside Git, `evidence/I-0068/queries/upscaler-ready/`): `scene-param-readers-ps.tsv` (`Scan-SceneParamReaders.ps1`: the rows of `cbSceneParam` read by each of the 2,206 pixel programmes that use it, among 4,196 scanned), `projection-vars.tsv` (`Scan-ProjectionVars.ps1`: every matrix, screen-size and depth variable of the cbuffers of the post-effect, decal, FFX and renderer bundles, with the compiler's used flag), `sampler-usage.tsv` (sampler slot to name to bundle), `upscaler-goldens.tsv` (`Get-UpscalerGoldens.ps1`: jitter sequence, extents, bias).

## 0. Answer first

1. **The engine has nothing that blocks a temporal upscaler, and one thing it already gives: unjittered motion vectors.** `R16G16_FLOAT`, previous minus current in pixels, render size, from the velocity resolve (VELOCITY-NATIVE, D-0162, `SP:1226-1285`); object pixels decode the `Velo` rows (the model families of grass, foliage, veils, cloth and flow displacement have `_Velo` vertex programmes, checked by name in the programme cache; the count is not saved), every other pixel takes the camera reprojection of its depth. What is missing is on the other side: **a render extent distinct from the output extent** (section 1.1), **a projection that carries the sub-pixel jitter** (1.2), **a mask of the blended draws** (1.4) and **a history-reset service** (1.5).
2. **Jitter has one entry point in the producer; its consumers split into three groups.** The engine's projection record is already the off-centre form (`m[2][0]`, `m[2][1]`, stored as `-0.0`, `PR:34-35`), and `FC_MatrixInvProj`, `FC_MatrixInvViewProj`, `FC_MatrixProj` and `VC_MatrixViewProj` are all derived from that one matrix (`PR:35-46`), so a jitter set there reaches every reader of `cbSceneParam`. By fixed row, **no pixel programme reads the two rows that carry the off-centre terms of `FC_MatrixProj`** (rows 16 and 17 in the transposed storage; rows 18 and 19, read by 12 and 24 programmes, hold only `(0, 0, a, b)` and `(0, 0, 1, 0)`); they read `FC_MatrixInvProj` (1,871 programmes), `FC_MatrixInvViewProj` (2,091 for any of rows 8 to 11), `FC_MatrixInvView` (2,086), `FC_ScreenSize` (1,952), `FC_FrustumNear` (1,848) and `FC_DepthComputeParam` (118); 628 of the 2,206 programmes also index the buffer dynamically (the scan counts that as the whole buffer), so every "never read" is a statement about fixed rows. The inverse is what a jittered depth needs, so those programmes need **no change**. Three groups: (a) what rasterises with the jitter: the main view's model matrices (`view_constants` from `ProduceCameraViewConstants`, which reads byte 256 and so becomes jittered by the same edit), the native `cbSceneParam` block (`SP:744`) and the rasteriser matrices outside `cbSceneParam` listed in 1.2; (b) what must stay unjittered: the shadow and light view matrices (`ViewWvp`, their own projections: untouched) and every view that is not the main world view; (c) what carries the same constant shift on both of its matrices so that the difference stays unjittered: the `Velo` draws (3.3). Section 3.2.
3. **A correction to an earlier rule: temporal reprojection through depth must use the matrices of the projection the depth was rasterised with.** VELOCITY-MOTIONBLUR-NATIVE section 6 says to keep SSAO's and the shadow cache's reprojection unjittered. For a lookup through a jittered depth that is wrong by up to one pixel (a constant offset that changes every frame, so a shimmer): the reconstructed world point is exact only with the jittered inverse, and the previous buffer was rasterised with the previous frame's jitter. Only the **motion vector handed to the upscaler** and the **motion-blur velocity** are unjittered by contract. Because jitter in this projection form is a constant NDC shift (`ndc = x sx / z + m20`, w = z), the unjittered position of a pixel is its own position minus the jitter, at every depth: three small matrices cover SSAO, motion blur and the vector resolve (3.3).
4. **Where the upscaler sits: between slot 12 (velocity) and slot 13 (YEBIS) of the frame graph, on the lit HDR, so YEBIS, glow, UI and gamma run at the output size.** The lit HDR is linear and not pre-exposed (YEBIS applies the exposure at the tone map), which is what DLSS wants; YEBIS would otherwise upscale an LDR image. The cost is that depth-reading YEBIS stages (depth factor to alpha, depth of field, light shafts) need a **depth at the output size**, and DLSS gives none: section 3.7 specifies a depth-aware upsample and marks the artefacts as a risk. The GX anti-alias filter (FXAA 3) is switched off when the upscaler runs.
5. **The reactive mask is a stencil bit and one small native draw** (built by I-0111 tranche 4 from the colours, then removed by
   I-0116 tranche 2: DLSS 310.9's current models do not read it). Blended draws (forward rows 0x1F and 0x29, effects E01 to E07, water, veils) have no velocity. Stencil bits 0x01, 0x02, 0x04, 0x08, 0x10 are in use (decal receiver, SSS, the composite shadow-mask classes) and the optional decal pre-pass uses 0x20 (off by default); **0x40 is proposed for the reactive mask and 0x80 for a second, FSR-only mask**. The blended draws write it with stencil `REPLACE` (their depth is read-only: a DSV with read-only depth and writable stencil), a native full-screen draw turns it into `R8_UNORM`. DLSS takes only the first (`pInBiasCurrentColorMask`; the transparency mask is "reserved for future use" in the SDK header). Section 3.4.
6. **History reset is a counter, not a flag.** Seven owners of cross-frame state exist or are planned (1.5); one `HistoryEpoch` increments on: first world frame after an absence, the native cut rule (more than 10 m or a forward dot below 0.5, `GB:5207-5212`), an extent or mode change, a content incarnation change, a gap of more than `K` frames without a dispatch, and (once the YEBIS driver exists) the two driver routes `FUN_1416C92B0` / `FUN_1416C9270` (YEBIS-SECOND-TRANCHE-READY 2.6). The upscaler's `reset` input is `epoch != last_epoch`. Section 3.5.
7. **The first deliverable needs no NVIDIA code.** A neutral `UpscalerInputs` record and a **bilinear stub** upscaler validate the extents, the jitter, the mask and the reset on any machine (UP-1 to UP-8); the DLSS adapter (UP-9) is a thin function over NGX behind `upscaler-dlss.flag`. Direct NGX is proposed, not Streamline: Streamline's default interposer wraps the device and the swap chain, which the "no wrapper" mandate excludes (Streamline also has a manual-hooking mode without the proxy; a decision for Thomas, section 7).
8. **Design**: ten commits UP-1..UP-10 (section 4), tests T-U1..T-U13 with goldens (section 5), fourteen live rows UL1..UL14 (LIVE-CHECKS-CONSOLIDATED 1.17), and the corrections of section 8.

## 1. The engine (verified unless a label says otherwise)

### 1.1 Extents

| Fact | Where |
|---|---|
| One extent for everything: `presenter_` (61 lines of `scene_pass.cpp` read its members); the G-buffer, lighting, forward, velocity, scene copy, UI depth and the back buffer share it. The menu views swap whole target sets per extent (`SwapTargetSet`, 18 members) and keep their own extents | `SP`, D3D12-PHASE-E-FRAME-GRAPH-SHEET 1 and 3 |
| `FC_ScreenSize` is the **render** size, not the camera's viewport (`UseRenderSize`, `PR:15-18`); the cbSceneParam producer sets rows 1 `{w, h, 1/w, 1/h}` | `PR:17-21`, `scene_ingress.cpp:24`, `live_host.cpp:2572` |
| `SceneViewInput` has `width`, `height`, `explicit_aspect`, and refuses any non-zero `projection_flags` or `tile_count` ("unsupported lens branches are refused") | `scene_view.h:10-16`, `SV:50-57` |
| Resolution and scaling are runtime inputs: every size-dependent resource and constant is listed and recomputed | D-0151 (decisions/LOG.md) |
| Fractional targets are sized from the main extent: SSAO 640x360 (half), the water reflection view 640x360 (fixed), the glow chain `ceil(W / (4 * 2^i))` (follows the presenter size, GLOW-WAVE1-READY GL6), the YEBIS pyramids (derived by the library from the scene extent) | frame graph sheet 3.1, GLOW-WAVE1-READY |

### 1.2 Who reads the projection, the screen size and the depth parameters

**Material and lighting programmes (`cbSceneParam`, register b8).** Fixed-row reads of the 2,206 pixel programmes that use it (`scene-param-readers-ps.tsv`; one programme counts once per row):

| Row | Variable | Pixel programmes | Reading |
|---|---|---|---|
| 1 | `FC_ScreenSize` | 1,952 | render size, pixel to UV |
| 2 | `FC_DepthComputeParam` | 118 | linear depth from `{a, -near/(far-near), b, 0}`; depends on near and far only: **jitter-free** |
| 8 to 11 | `FC_MatrixInvViewProj` | 2,091 for any of the four rows (2,079 row 8, 2,087 each rows 9 to 11) | position from depth: **needs the jittered inverse** |
| 12 to 15 | `FC_MatrixInvProj` | 1,871 | view-space position from depth: **needs the jittered inverse** |
| 16, 17 | `FC_MatrixProj` (transposed storage: these two rows carry `m20`, `m21`) | **0** | never read by a pixel programme |
| 18, 19 | `FC_MatrixProj` | 12, 24 (`MultiBlend4_et1_p` forward, Lpp1 and SSdw variants) | `(0, 0, a, b)` and `(0, 0, 1, 0)`: jitter-free |
| 20 to 23 | `FC_MatrixInvView` (the transposed pose) | 2,086 | pose with translation: jitter-free |
| 24 to 27 | `VC_MatrixViewProj` (camera-relative) | **0** pixel (the vertex stage was not scanned; that vertex programmes read it is INFERRED from the native block at `SP:744`) | the matrix that rasterises: **jittered** |
| 55 | `FC_FrustumNear` | 1,848 | jitter-free |

**Post-effect, decal, FFX and renderer bundles (own cbuffers, `projection-vars.tsv`, used flag only).**

| Variable | Used by | Needs |
|---|---|---|
| `g_mCurToPrevScreen` | `MotionBlurWriteCMBVelocity`, `ScreenSpaceAO2_ReProj`, `ScreenSpaceAO2_SeparatedReProj`, `ScreenSpaceAO2_WithNormalReProj` | three different matrices, section 3.3 |
| `g_mInvProj` | `ScreenSpaceAO2`, `_ReProj`, `_WithNormal`, `_WithNormalReProj` | jittered inverse |
| `g_vPrevFrameStrengh` | the three AO reprojection variants | none |
| `g_vDepthComputeParam` | 22 post-effect programmes (depth conversion, CoC, AO, velocity writers) | none (near/far only) |
| `g_vDepthRangeParam` | 5 AO programmes | none |
| `g_vScreenSize` | 21 post-effect programmes (FXAA, FXAA3HQ, AO, lens/motion blur) | the size of **that pass's target**: render for AO, output for FXAA/DoF/blur |
| `g_InvProjMatrix` | 26 decal programmes, 60 renderer-bundle programmes | jittered inverse |
| `g_ProjMatrix` | 4 renderer-bundle programmes; `m_ProjMatrix` 16 and `g_projectionMatrix` 12 FFX programmes | the projection the pass rasterised with (jittered for forward-drawn effects) |

No programme of the four bundles names a jitter. Previous-frame matrices: `g_mCurToPrevScreen` (four programmes) and `g_ReprojMatrix` (`GXSsr`, a renderer-bundle pixel programme, with its `g_ProjMatrix`, `g_InvProjMatrix` and `g_vScreenSize`: the screen-space reflection pass, whose family sheet does not exist yet; it takes the same pair as SSAO, 3.3). No programme has to be taught about jitter; **the producers of the constants have to pick the right matrix**.

**Rasterising and tile matrices outside `cbSceneParam`** (used flag, `projection-vars.tsv`); all take the jittered projection when their pass draws into the jittered scene: `g_projectionMatrix` (12 `GXFfx*` vertex programmes: texture, soft, bump, point sprite, tracer), `m_gViewProjMatrix` (`VS_VFX_OptStandardParticle`), `m_fWorldViewProjection` (`VS_VFX_RayDataFront`, `VS_VFX_RayDataBack`), `g_viewProjection` and `g_terrainInvProjection` (`GXOceanWave`), `g_matProjection` (`GXAOFields`), `FC_MatrixProj` (the `GXEnvMapRegion` vertex programme), `m_ProjMatrix` (16 FFX particle compute programmes) and `g_ProjMatrix` (`GXLightCompute`, `GXLightOrganize`, `GXLightOrganize16`: tile culling, a pixel of jitter is immaterial there, but one rule is simpler).

### 1.3 Depth and motion vectors that exist

- Depth: standard Z, `LESS_EQUAL`, near = 0 (`GB:5567`), `R32G8X24_TYPELESS` (DSV `D32S8`, SRV `R32_FLOAT_X8X24`); not reversed (VELOCITY-MOTIONBLUR-NATIVE 6). The readable copy `DepthRead` exists in the plan (frame graph slot 2).
- Motion vectors: `R16G16_FLOAT` (`MotionVectorFormat`, `scene_pass.h:181`), overwritten each frame in slot 12, previous minus current in pixels at render size, unjittered because no jitter exists (VELOCITY-NATIVE, "Motion vectors"). Velocity target `R16G16B16A16_UNORM` for the YEBIS blur contract, a half-size `RGBA8` reduction planned for it.
- The previous view-projection is kept per view in `native_velocity_history` (`GB:5206`), keyed on the view pointer; the cut rule `dx² + dy² + dz² > 100` or `forward · forward' < 0.5` (`GB:5207-5212`) restarts it. **This is the native cut policy; the original's own flag (`13BFC0 obj+0x10`) covers warps and loads.**
- The vector is built from `VelocityViewProjection(camera, ...)`, which recomputes the lens from `SceneViewInput` (`VH:34-43`) and so is independent of whatever `BuildSceneView` returns: it stays unjittered when jitter is added to `BuildSceneView`'s output. **The `Velo` draws, however, rasterise with `VC_MatrixWVP` (b0, `VH:55-62`) into a target tested against the jittered scene depth** (`SP:1261`): their position must carry the same jitter or object coverage is off by up to half a pixel against that depth (3.3).

### 1.4 Draws that write colour without motion

From VELOCITY-MOTIONBLUR-NATIVE 4 and 6 and the passes' sheets: forward rows 0x1F and 0x29 (blended), effects E01 to E07 (FFX particles, distortion, soft particles), water (refraction, foam), veils (`Veill*`, which also have `_Velo` programmes but blend), decals (they modify G-buffer attributes: covered by depth and the surface's vectors). The FFX programmes read depth for soft fading (`m_vDepthComputeParam`, 16 programmes). The HDR alpha channel is already used (water refraction mask, written in the forward/water passes; the YEBIS depth factor is written by the library into its own scene copy), so alpha is not a mask carrier. Stencil: bits 0x01 receiver, 0x02, 0x04 SSS, 0x08, 0x10 composite classes are in use (DECALS-NATIVE 86, SSAO-NATIVE 50); 0x20 only with the decal pre-pass (`manager+0x511`, off). Bit 0x10 is read by the composite but its writer is not identified (SSAO-NATIVE 101-103); the native G-buffer writes 9, 4 and 2 with write mask 255 (`GB:5554-5569`). Bits 0x40 and 0x80 are free in everything read (OPEN: a family that sets a high bit is not excluded; row UL10).

### 1.5 Owners of cross-frame state and their resets

| Owner | State | Reset in the original | Native |
|---|---|---|---|
| Velocity | previous view-projection | `13BFC0 obj+0x10` first-frame flag | cut rule above (`GB:5207`) |
| SSAO | `AoHistory[2]` (640x360 RGBA8) | counter / first frame (SSAO-WAVE1-READY) | planned |
| Shadow cache | kept cascades with reprojection | SHADOWS-NATIVE S6 | planned |
| After-image | `AfterHist[2]` | `FUN_1416C92B0` clears the list `[S+0x108]+0x270[]` | planned |
| Glare final | `GlareFinal[2]` (640x540 or 420x354) | with the history | planned |
| DoF table | `DofTable[2]` 2048x2 R32F | with the history | planned |
| Exposure adaptation | adapted exposure, focus | `FUN_1416C9270` resets adaptation | planned |

The driver `E69A0` has two reset routes (`FUN_1416C92B0(S, 0)` when the object at `[GX1+0xB80]` has `+0x3C` set or `+0x30 > 1`; `FUN_1416C9270(S, 0)` when the byte at `post parameter block + 0x4E0` is set) and the word `post+0x227C` that nothing writes (YEBIS-SECOND-TRANCHE-READY 2.6); what triggers the two is OPEN (YP11).

### 1.6 YEBIS and the chain after it

Lit HDR (`light3`, `R16G16B16A16_FLOAT`) is copied by `PointSampleColorClamp1` into the library's scene texture; the library writes the depth factor into alpha, runs DoF, glare/bloom, light shafts, grading, the tone map (Exposure word) and leaves `Ldr[A]` (back-buffer size `R8G8B8A8`); the glow composite samples GB5 by UV and draws at the output size; the AA GX filter (`FXAA`, `FXAA3HQ`; the DLAA variant is a GX filter) follows; then UI, then gamma. All these steps take their sizes from the scene copy or the output, so they follow the output extent without change when the upscaler hands them an output-sized HDR image. The depth-reading steps take a depth SRV whose size must match: section 3.7.

### 1.7 Samplers

`sampler-usage.tsv`: the material set is `s0 SS_DiffuseMap`, `s1 SS_SpecularMap`, `s2 SS_BumpMap`, `s3 SS_MaskMap` (the last only in 2 programmes; `s3` is `SS_ClampLinear` in post-effect), `s10 SS_WrapAniso`, `s11 SS_ClampAniso`. `s5 SS_GIMap` (348 + 212 programmes), `s7 SS_ClampPoint`, `s12 SS_Shadow` are screen-space or probe lookups. `s8 SS_WrapLinear` (1,623 programmes of the main bundle) and `s9 SS_ClampLinear` (1,096) are shared by material and non-material lookups (OPEN what they sample; row UL9). A mip LOD bias belongs on the material set only.

## 2. The external contract (read from the public headers; re-verify against the SDK in hand)

| Input | Contract | Native source |
|---|---|---|
| colour | `pInColor`, render size, linear HDR, `IsHDR` feature flag, **jittered** | `light3` after slot 12, `R16G16B16A16_FLOAT` |
| depth | `pInDepth`, render size, standard Z unless `DepthInverted` | `DepthRead` (a sampled copy: plane 0 as `R32_FLOAT`) |
| motion vectors | `pInMotionVectors`, `InMVScaleX/Y` (0 = 1) convert to pixels, `MVLowRes` / `MVJittered` flags; **direction and sign are not documented in the headers (OPEN, row UL3)**; the native vectors are previous minus current in pixels, unjittered, at the **render** size (so `MVLowRes` is set in every scaled mode, UP-9) | `motion_vectors_` |
| jitter | `InJitterOffsetX/Y`, **render pixels**; the sign convention is not in the headers (OPEN, UL4) | section 3.2 |
| exposure | `pInExposureTexture` (1x1) or `InPreExposure` / `InExposureScale` (0 = 1); `AutoExposure` flag when absent | 3.6 |
| reset | `InReset` int, "set to 1 when the scene changes completely" | 3.5 |
| reactive mask | `pInBiasCurrentColorMask`; `pInTransparencyMask` reserved | 3.4 |
| output | `pInOutput`, output size, needs unordered access | UP-9 |
| sizes | `InRenderSubrectDimensions`; create: `InWidth/InHeight` (render), `InTargetWidth/InTargetHeight`, `InPerfQualityValue` (`MaxPerf`, `Balanced`, `MaxQuality`, `UltraPerformance`, `UltraQuality`, `DLAA`), `InFeatureCreateFlags` | 3.1; render sizes come from `NGX_DLSS_GET_OPTIMAL_SETTINGS`, not from a formula |
| recalled, not re-read | jitter phase count `8 * (output / render)²` with a Halton (2, 3) sequence; texture LOD bias `log2(render / output) - 1`; the application's own matrices and vectors carry no jitter (the SDK takes no matrices); HDR colour before tone mapping | 3.2, 3.8 |

Hardware and licence: an RTX adapter (the test adapter is an RTX 4070 Ti SUPER, binding tier 3, frame graph sheet 1); the SDK carries its own licence and a redistributable DLL. Both are decisions for Thomas (section 7).

## 3. Design

### 3.1 Render and output extents

`UpscalerSettings { enabled, mode (Off, Dlaa, Quality, Balanced, Performance, UltraPerformance), render_scale (stub only) }`; `FrameExtents { render, output }`, equal when the upscaler is off. Every pass of the table takes its size from one of the two; nothing reads `presenter_` directly any more (UP-1).

| Slot (frame graph 2) | Pass | Extent |
|---|---|---|
| Mv, V | movie, menu views | their own (no upscaler, no jitter) |
| S | shadow maps | atlas, own |
| 1 to 3 | G-buffer, copies, decals | **render** |
| 4 | AO A1..A5 | half of render (`AoHistory` follows) |
| 5 to 9 | shadow mask, env, lighting 0..3 | **render** |
| 10, 11 | forward, water, effects | **render** (the reflection view stays 640x360) |
| 12 | velocity, vectors, reactive mask | **render** (the YEBIS velocity reduction is half of **output**: its contract is a half-size target of the scene copy) |
| **U** | **upscaler** | render in, **output** out |
| 13 | YEBIS | **output** (pyramids derived by the library) |
| 14 | AA (off), glow | **output** |
| 15, 16 | UI, gamma, present | **output** (the swap chain) |

### 3.2 Jitter

- **Sequence (proposed, external):** Halton (2, 3), index from 1, minus 0.5, in render pixels, `phases = ceil(8 * (output / render)²)` (8 DLAA, 18 Quality, 24 Balanced, 32 Performance, 72 UltraPerformance: `upscaler-goldens.tsv`); frame index modulo phases; no jitter when the upscaler is off or for a non-main view. First eight samples (x, y in pixels): (0, -0.1667), (-0.25, 0.1667), (0.25, -0.3889), (-0.375, -0.0556), (0.125, 0.2778), (-0.125, -0.2778), (0.375, 0.0556), (-0.4375, 0.3889).
- **Injection (derived):** the projection is `[sx 0 0 0; 0 sy 0 0; m20 m21 a 1; 0 0 b 0]` with `ndc = (x sx, y sy) / z + (m20, m21)` (w = z). A jitter `(jx, jy)` in render pixels (+x right, +y down) is `m20 = 2 jx / W`, `m21 = -2 jy / H` (image y is down, NDC y up). At zero jitter the two words stay **`-0.0` (`0x80000000`)**, bit for bit as today (`PR:34`); `+0.0` would break the produced-image goldens. NDC words for 1280x720 in `upscaler-goldens.tsv`.
- **One source:** `ds3prod::Camera` gets `jitter[2]` (default 0 and the `-0.0` words); `ProduceSceneCamera` writes the jittered words at 128 (`InvViewProj`), 192 (`InvProj`), 256 (`Proj`), 384 (`VC_MatrixViewProj`). `SceneViewInput` gets `jitter_x/jitter_y` (zero default, existing callers unchanged); `BuildSceneView` returns `projection`, `view_projection`, `camera_relative_view_projection` **jittered** (they feed the native cbSceneParam block at `SP:744` and the per-draw matrices) and adds `unjittered_projection` and `unjittered_view_projection`.
- **Consumers of the jittered matrices (all follow from the one edit):** the main view's `view_constants` (`ProduceCameraViewConstants` reads the projection from byte 256 of the camera block, `scene_ingress.cpp:35-43`, and `ProduceViewMatrices` multiplies it into the VS b0 `cbMatrixWVP` of every model draw: `GB:5283`, `:7203`); the native `cbSceneParam` block (`SP:744`); the rasteriser matrices outside `cbSceneParam` (1.2). `ProduceCameraConstants` (`scene_ingress.cpp:15-25`) must forward the jitter from `SceneViewInput` into `ds3prod::Camera`.
- **Consumers of the unjittered pair:** `VelocityViewProjection` (already independent, `VH:34-43`); the cascade fit and the shadow cache of SHADOWS-WAVE1-READY; the shadow and light `cbMatrixWVP`, which come from `ViewWvp` with the view object's own projection (`light_producers.cpp:297-300`) and are untouched. The shadow cache reprojects in light space, so it never reads the camera jitter at all. The forward `FC_ProjSpaceToShadowMatrix` is `transpose(pose × cascade matrix)` (`light_producers.cpp:264-279`): it contains no projection and needs no change; the position it is applied to comes from the jittered inverse (INFERRED from the reader table).
- **Views without jitter:** menu views, the water reflection view, shadow cascades, light views, the movie, startup frames.

### 3.3 The three reprojection matrices (derived)

Notation: `J` = jitter in UV units `(jx / W, jy / H)` (image y down); `Pc`, `Pp` = unjittered view-projection of the current and previous frame; `Pc'`, `Pp'` the jittered ones; `U2N`, `N2U` the engine's uv/NDC maps (`VH:48-49`). Because the jitter is a constant NDC shift, the unjittered UV of a world point is its jittered UV minus `J`, whatever its depth.

| Consumer | Needs | Matrix |
|---|---|---|
| SSAO history lookup (`g_mCurToPrevScreen` of the four AO programmes) | previous buffer rasterised with `Pp'`; world point from `Pc'⁻¹` | `U2N · Pc'⁻¹ · Pp' · N2U` (= `VelocityCurrentToPrevious(Pc', Pp')`) |
| YEBIS motion-blur camera velocity (`g_mCurToPrevScreen` of `MotionBlurWriteCMBVelocity`) | velocity `= prevUnj(W) - (uv - J)` and the shader computes `prev_uv - uv` | `U2N · Pc'⁻¹ · Pp · N2U`, then add `J` to the output UV: column `i` of the 4x4 becomes `M(:,i) + J_i · M(:,3)` for `i = 0, 1` |
| Native vector resolve (the upscaler's input) | `prevUnj(W) - curUnj(W)` in pixels | same matrix as the motion-blur line, and the resolve multiplies by `(W, H)` |
| `Velo` draws (object vectors) | clip positions rasterised against the jittered depth; velocity is the difference of two clip positions | the current-frame NDC shift is added to **both** `VC_MatrixWVP` and `VC_PrevMatrixWVP` of the b0 block (`VelocityMatrixBlock`, `VH:55-62`): `clip.xy += w · (m20, m21)`, i.e. column `i` of the shader-side matrices gets `+ m2i · col3`; the shift is constant in NDC, so it cancels in the difference and the vectors stay unjittered (test T-U13) |

When the upscaler is off `J = 0` and `Pc' = Pc`: the rows collapse to the one matrix of today (`GB:5214`), so UP-4 changes nothing without the flag. **The history record must grow:** `native_velocity_history` keeps only the unjittered view-projection and pose (`GB:5215`); `Pp'` needs the previous frame's jitter as well (a `jitter` pair in the record, reset with it).

### 3.4 Reactive mask

- **Write:** every blended class (forward 0x1F and 0x29, effects, water, veils) gets a PSO variant with stencil `REPLACE`, ref and write mask `0x40`, depth read-only (a DSV with `D3D12_DSV_FLAG_READ_ONLY_DEPTH`, which the native tree already creates, `SP:722-723`). Resource state is per plane: during the blended draws plane 0 is `DEPTH_READ | PIXEL_SHADER_RESOURCE` (soft particles and water sample it) and plane 1 `DEPTH_WRITE`; the "plane 0 `DEPTH_WRITE`" of the frame graph table is the state between passes. The stencil test and the `REPLACE` share one `OMSetStencilRef`: the draw's engine reference is OR-ed with `0x40` and `0x40` is left out of the read mask. The second bit `0x80` marks the FSR-only composition class (particles with no depth contribution) and is written only when that adapter exists.
- **Resolve:** one native full-screen draw (HLSL like the composition draw, `SP:372`) reads the stencil plane through an `X32_TYPELESS_G8X24_UINT` SRV (`PlaneSlice = 1`, the value in `.g` of a `Texture2D<uint2>`) and writes `R8_UNORM` (0 or the reactive strength, a constant 1.0 first); the strength per class is a later tuning input (row UL11).
- **Alternatives rejected:** an extra render target on every blended PSO (changes the engine's output count and the programmes' signatures); HDR alpha (taken, 1.4).

### 3.5 Reset service

`HistoryEpoch` (a `uint64_t` owned by the frame state) increments on: (1) the first world-view frame after any frame without one (title, loads, menus that hide the scene); (2) the native cut rule (the `reset` of `CollectNativeVelocityInputs`, `GB:5212`); (3) an extent, mode or quality change; (4) a `content_incarnation` change (`scene_pass.cpp:304`); (5) the original's two driver routes once the YEBIS driver exists (YEBIS-SECOND-TRANCHE-READY 4.4); (6) a gap of more than `K` frames without a dispatch (`K = 2` proposed); UP-7 implements (1), (2), (3), (4) and (6), the source (5) lands with the YEBIS driver. Each history owner stores the epoch it was written in and refuses to read an older one; the upscaler passes `reset = (epoch != last_epoch)` and stores the new value after a dispatch. The reset is also set on the frame after the upscaler is toggled on.

### 3.6 Exposure and colour

The lit HDR is not pre-exposed (INFERRED: YEBIS applies the Tone Map exposure, YEBIS-WAVE1-READY 1.2). The DLSS adapter passes `AutoExposure` off and the exposure of the previous YEBIS frame (the CPU value read back from the library, YEBIS-SECOND-TRANCHE-READY 2.5, one frame old) as a 1x1 `R32_FLOAT`, or `InPreExposure = 1` while YEBIS is absent. If the sign of the tone map's exposure meaning is not clear in the SDK in hand, `AutoExposure` on is the fallback (row UL6).

### 3.7 Depth at the output size

DLSS returns no depth. For the YEBIS stages that read one (depth factor to alpha, DoF, light shafts) the upscaler slot ends with a native draw producing `DepthOut` at the output size: for each output pixel, the **nearest** of the 2x2 render-depth texels around its jittered position (the one closest to the output pixel centre; max-depth would thicken foreground edges, min-depth would erode them). Format `R32_FLOAT`. Risk R-U3: the DoF circle of confusion and the light-shaft occlusion see a one-render-pixel stair at silhouettes; the live row UL7 compares them with the render-size result.

### 3.8 Mip bias

`MipLODBias = log2(render_width / output_width) - 1` (external; −1.585 Quality, −1.788 Balanced, −2.0 Performance, −2.585 UltraPerformance, −1.0 DLAA) on the **material sampler set** only: `s0, s1, s2, s3, s10, s11` (`s6 SS_WrapPoint`, 18 material and 46 decal programmes, stays at 0: a point sampler with a bias would alias); `s8` and `s9` are PROPOSED to follow until row UL9 says what they sample; `s5, s7, s12` and every post-effect sampler stay at 0. The bias is in the sampler descriptors, not the shaders, so it costs one sampler-heap variant per mode.

### 3.9 Interface, stub and adapter

```cpp
struct UpscalerInputs {
  ID3D12Resource *color, *depth, *motion, *reactive, *exposure, *output;   // output: unordered access
  uint32_t render_w, render_h, output_w, output_h;
  float jitter_x, jitter_y;      // render pixels, +x right, +y down
  float mv_scale_x, mv_scale_y;  // vectors are pixels already: 1, 1
  bool reset, hdr;
};
struct Upscaler { virtual bool Dispatch(ID3D12GraphicsCommandList*, const UpscalerInputs&) = 0; virtual ~Upscaler() = default; };
```

The **stub** is a native bilinear (Catmull-Rom as a second option) draw: it ignores depth, vectors and the mask but checks their presence and sizes, so UP-1..UP-8 are validated without NVIDIA code. The **adapter** (UP-9): `NVSDK_NGX_D3D12_Init`, `GetCapabilityParameters`, `NGX_DLSS_GET_OPTIMAL_SETTINGS` for the render size, `NGX_D3D12_CREATE_DLSS_EXT` at creation and on every extent or mode change, `NGX_D3D12_EVALUATE_DLSS_EXT` per frame; D3D12 specifics to honour: the evaluate call may change the command list's root signature, descriptor heaps and pipeline state, so the encoder re-binds after it (INFERRED from the NGX samples, row UL12); the output needs `ALLOW_UNORDERED_ACCESS`; inputs are in `NON_PIXEL_SHADER_RESOURCE | PIXEL_SHADER_RESOURCE`, the output in `UNORDERED_ACCESS`, with the transitions in the graph; the feature is released before the device.

## 4. Commits (opt-in flags; each leaves the default frame bit-identical)

| ID | Content | Flag |
|---|---|---|
| UP-1 | Split extents: `FrameExtents`, every `presenter_` read in the table of 3.1 takes render or output; equal when off | none (refactor), T-U1 |
| UP-2 | Jitter plumbing: `Camera.jitter`, `SceneViewInput.jitter_*`, jittered and unjittered pair in `SceneView`, `ProduceCameraConstants` forwards the jitter, the Velo b0 block takes the current-frame shift on both matrices; shadow and light views untouched | `upscaler.flag` |
| UP-3 | Sequence and frame counter: Halton table, phase count by mode, zero for non-main views | `upscaler.flag` |
| UP-4 | The reprojection matrices of 3.3 (SSAO, motion blur, vector resolve, `GXSsr`); the vector resolve takes `J` and the jittered inverse; the velocity history keeps the previous jitter | `upscaler.flag` |
| UP-5 | Material sampler set with the mode's mip bias; sampler-heap variants | `upscaler.flag` |
| UP-6 | Reactive mask: stencil variants of the blended PSOs, the resolve draw, `R8` target | `upscaler.flag` |
| UP-7 | `HistoryEpoch` and its sources (1), (2), (3), (4), (6); owners compare epochs | none (service), T-U9 |
| UP-8 | `UpscalerInputs`, `Upscaler`, the bilinear stub, `DepthOut`, exposure feed, slot U in the frame graph | `upscaler-stub.flag` |
| UP-9 | NGX adapter over the interface | `upscaler-dlss.flag` |
| UP-10 | Options: mode list, AA exclusion (the GX AA filter off), resolution scale in the menu, persistence | `upscaler.flag` |

UP-4 and UP-8's depth upsample wait for the SSAO and YEBIS native passes where they touch those; the commit lands the matrix functions and their tests first.

## 5. Tests

| ID | Test | Golden |
|---|---|---|
| T-U1 | extents at render = output: every produced image identical to the pre-UP-1 hashes | existing witnesses |
| T-U2 | producer at zero jitter: words `m[2][0]`, `m[2][1]` equal `0x80000000`; blocks byte-identical | produced-image goldens |
| T-U3 | producer at jitter `(jx, jy)`: `m20 = 2 jx / W`, `m21 = -2 jy / H`; rows 128, 192, 256, 384 recomputed from them; bytes 16 to 79 (screen size, depth parameter, position, clip plane), 80 (view), 320 (inverse view) and 880 (near) unchanged | `upscaler-goldens.tsv` NDC words |
| T-U4 | round trip: for random poses and jitters, project a world point with the jittered projection, rebuild it with the jittered inverse and the depth: error under 1e-5 relative; with the unjittered inverse the error is the expected sub-pixel | derived |
| T-U5 | the unjittered UV of a pixel equals its UV minus `J` at depths from near to far (the constant-shift claim of 3.3) | derived |
| T-U6 | the three matrices of 3.3 against a brute-force reprojection of a point cloud | derived |
| T-U7 | Halton table and phase counts of the five modes (the extents of the file are the stub's: with NGX they come from the SDK) | `upscaler-goldens.tsv` |
| T-U8 | mip bias per mode and the sampler set that receives it | `upscaler-goldens.tsv` |
| T-U9 | `HistoryEpoch`: each of the five implemented sources (absence, cut rule, extent or mode, content incarnation, frame gap) increments it exactly once; owners refuse an older epoch | unit |
| T-U10 | reactive mask: a blended draw over a known pattern gives the exact `R8` result; an opaque draw over it clears nothing it should keep | witness pictures |
| T-U11 | stub dispatch: output size, all inputs present, `reset` seen once per epoch | recording |
| T-U12 | debug layer clean over 600 frames per mode (stub), then with the adapter on the test adapter | live |
| T-U13 | `Velo` b0 block with the current-frame shift on both matrices: the decoded vector equals the unjittered one for random poses and jitters, and the object coverage test against jittered depth has no holes on a slope | derived, witness |

## 6. Live rows (LIVE-CHECKS-CONSOLIDATED 1.17)

| ID | Kind | Read | Expected |
|---|---|---|---|
| UL1 | native | the stub at Quality, Firelink, still camera | sharp, no shimmer; the 18 phases visible in the debug overlay |
| UL2 | native | camera pan with the stub, vectors dumped | vectors non-zero only with motion, unjittered (still camera: all within 0.01 px) |
| UL3 | native | DLSS adapter, pure camera pan | no ghost trail; if there is one, flip the vector sign (`InMVScaleX/Y = -1`) and record which is right |
| UL4 | native | DLAA, still camera | no pixel crawl; if there is, flip the jitter sign |
| UL5 | native | SSAO with jitter on a still camera | no visible shimmer of the AO (the matrices of 3.3) |
| UL6 | native | exposure feed: bonfire to dark corridor | no pumping at the tone map; fallback `AutoExposure` |
| UL7 | native | DoF and light shafts with `DepthOut` against render-size depth | silhouette stair visible or not; decision on 3.7 |
| UL8 | native | motion blur camera velocity with jitter | none on a still camera |
| UL9 | native | what `s8` and `s9` sample (a frame capture by slot) | the sampler set of 3.8 is final |
| UL10 | native | stencil bits 0x40 and 0x80 across a frame at Firelink, High Wall, the DLC maps | never set by any pass except the mask |
| UL11 | native | reactive mask: fog, fire, water, veils | no ghost trail behind a fire or a veil; no softening of the solid next to it |
| UL12 | native | the command list state after the evaluate call | the re-bind list of 3.9 is complete |
| UL13 | native | warp, load, death, cut-scene key change | one reset per event, no smear in the first frames |
| UL14 | native | resolution change and mode change at runtime | the feature recreated, no leak, no stale history |

## 7. Risks and decisions for Thomas

- **Decisions.** (1) SDK route: direct NGX (proposed) or Streamline (an interposer that wraps the device and swap chain, against "no wrapper"). (2) Whether a DLSS dependency (licence, redistributable DLL, RTX-only) is acceptable in the product at all, or only as an optional plug-in; the neutral interface keeps FSR possible. (3) Whether DLAA is the first target (no extent split visible, no depth problem: render = output) before the scaled modes. (4) The GX anti-alias filter: off when the upscaler runs (proposed).
- **Risks.** R-U1 vector sign or jitter sign wrong: two rows. R-U2 the lit HDR may carry a pre-exposure the sheet did not find (UL6). R-U3 the output-size depth (3.7). R-U4 any family that writes the stencil high bits (UL10). R-U5 `s8`/`s9` bias on lightmaps or masks (UL9). R-U6 the YEBIS pyramid sizes derived from the output extent cost more at 4K; the library's own settings are untouched. R-U7 history owners that do not exist yet (SSAO, shadow cache, YEBIS history) must read the epoch when they land; the commit adds the check to their sheets.
- **Open.** The SDK sign conventions; the strength of the reactive mask per class; whether any YEBIS pass reads the HDR alpha written before it (an upscaled colour drops alpha unless `AlphaUpscaling`); the interplay of the half-output velocity reduction with the YEBIS blur contract.

## 8. Corrections to earlier notes

| Note | Statement | Correction |
|---|---|---|
| VELOCITY-MOTIONBLUR-NATIVE 6 | "keep unjittered: SSAO temporal, the shadow cache reprojection" | only the vectors handed to the upscaler and the motion-blur velocity are unjittered; reprojection through a jittered depth uses the jittered pair (3.3) |
| VELOCITY-MOTIONBLUR-NATIVE 6 | "list each reader of `FC_MatrixInvViewProj` / `FC_MatrixInvProj` before switching" | listed (1.2): no pixel programme reads `FC_MatrixProj` or `VC_MatrixViewProj`; the inverses are read by 1,871 / 2,079 programmes and need no change |
| VELOCITY-MOTIONBLUR-NATIVE 6 | "keep unjittered: the velocity matrices (`VC_PrevMatrixWVP` and the current one)" | the `Velo` draws rasterise with that b0 block against the jittered depth: both matrices carry the current-frame shift, which cancels in the vector (3.3, T-U13) |
| VELOCITY-MOTIONBLUR-NATIVE 6 | "decide then whether DLSS takes HDR before YEBIS" | decided: HDR, before YEBIS (0.4) |
| VELOCITY-IMPLEMENTATION-CHECKLIST 10.3 | "keep unjittered: step 3's matrices, shadows and the temporal passes" | the temporal passes that read a jittered depth (SSAO) take the jittered pair; the shadow side is unjittered (3.2, 3.3) |
| VELOCITY-IMPLEMENTATION-CHECKLIST 10.6 | "an R8 coverage target written by those draws" | a stencil bit written by the blended PSOs and one resolve draw to `R8` (3.4) |
| (a point to avoid) | `FC_ProjSpaceToShadowMatrix` as a jitter dependency | it is pose × cascade and contains no projection (`light_producers.cpp:264-279`) |
| D3D12-PHASE-E-FRAME-GRAPH-SHEET 3.1 | velocity targets transient | still true for the GPU targets; the epoch (3.5) is CPU state |

## 9. Review log

Independent review by an Opus 5.5 reader (read-only, 2026-10-11): two blocking, six important, a dozen minor findings; all applied above. The algebra of 3.2 and 3.3 (constant NDC shift, signs, the three matrices, the SSAO correction), the cbSceneParam constants, the cut rule, the counts of 1.2 (apart from those below) and the goldens were reproduced.

| # | Severity | Finding | Fixed in |
|---|---|---|---|
| 1 | blocking | `ProduceCameraViewConstants` produces the main view's VS b0, not shadow or light matrices (`GB:5283`, `:7203`); the sheet put it on the unjittered list | 0.2, 3.2 |
| 2 | blocking | the `Velo` draws rasterise with `VC_MatrixWVP` against the jittered depth; b0 was kept unjittered, leaving holes on slopes | 0.2, 1.3, 3.3, T-U13, UP-2, section 8 |
| 3 | important | "`FC_MatrixProj` never read" overstated: rows 18 and 19 are read by 12 and 24 programmes (they hold no jitter); 628 programmes index dynamically; rows 8 to 11 count is 2,091 | 0.2, 1.2 |
| 4 | important | `GXSsr` (`g_ReprojMatrix`) and the rasteriser matrices outside `cbSceneParam` (FFX vertex, ocean, AO fields, env map region, particle compute, light tiles) were missing | 1.2, 3.3 |
| 5 | important | the vectors are render-size (not "full size"): `MVLowRes` in scaled modes | 2, UP-9 |
| 6 | important | DLAA extent at 1681x945 was floored to even; extents are the stub's, not NGX's | goldens script, 3.1, T-U7 |
| 7 | important | reset sources inconsistent between 0.6 and 3.5; five implemented, one deferred; the velocity history must keep the previous jitter | 0.6, 3.3, 3.5, T-U9 |
| 8 | important | unsupported claims: the 263 / 761 Velo count (not saved), "vertex programmes read `VC_MatrixViewProj`" (vertex stage not scanned) | 0.1, 1.2 |
| 9 | minor | `GB:4219` stale (now `GB:5567`); stencil SRV `PlaneSlice = 1`, `.g`; plane states during blended draws; shared stencil ref; T-U3 byte offsets vs rows; `FC_MatrixInvView` includes the translation; shadow-cache wording; "matrices handed in" external wording; Streamline manual hooking; `s6` missing from the sampler list; forwarding in `ProduceCameraConstants` | 1.3, 2, 3.2, 3.4, 3.8, T-U3 |
| 10 | open | stencil bit `0x10`: read by the composite, its writer unknown (SSAO-NATIVE 101-103); the native G-buffer writes 9, 4, 2 with write mask 255 (`GB:5554-5569`): row UL10 stays | 1.4 |
