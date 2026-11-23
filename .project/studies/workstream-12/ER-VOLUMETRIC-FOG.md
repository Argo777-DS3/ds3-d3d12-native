# Elden Ring 2.7.1: the volumetric fog, its lights, the fog volumes (static study for workstream 12)

**Non-canonical until the study thread re-reads it.** Date 2026-11-22. Author: Sonnet 5.5 `ghidra-research` subagent for the study thread.
Read-only reference (D-0195): ER's defects are named, not copied.

Binary `eldenring_2.7.1.0_25080141_1A354710.exe` (image base `0x140000000`, no symbols; every `FUN_<addr>` is Ghidra's auto name).
Headless Ghidra 12.1.3 `-readOnly -noanalysis`, hidden, Idle priority, mirror `er-mirror-vignette`, project `ER_2_7_1_25080141_I0001`
(no `*.lock` under the mirror before or after; one run at a time; no java process of mine left). Plus Python raw scans of the exe on
disk (`C:\Users\User\Tools\ER-Decompilation\inputs\eldenring_2.7.1.0_25080141_1A354710.exe`, helpers copied from I-0138). 47 functions
exported/decompiled (the cap of about 40 was overrun by small helpers; the 25 that matter are in "Function index"). Raw outputs and
scripts: `C:/Users/User/Tools/DS3-Decompilation/study-ws12/er-fog/` (`exp1..exp13` = `.c` + `.asm.txt`; `out_*.txt`; `scripts/`).

Status tags: **OBSERVED** = read in code/strings/bytes (address given). **INFERRED** = reasoned from observed code or names.
Shader bytecode is not in the exe: shader behaviour is never observed, only programme names, binding order and constants.

## Answers first

1. The volumetric fog is a froxel pipeline of **14 compute programmes + 1 vertex + 4 pixel programmes** named `GXVolumetricFog_*`, owned
   by the sky-simulation object (`GXSR::GXSkySimulation`, vtable `0x142f29478`, member at view+`0xbf20`); per frame: clear, scene-max-depth
   (optional), fog-volume density injection, local-light injection (one dispatch per light), dir-light shadow sampling (jittered, reprojected,
   post-filtered), in-scattering write, a per-column solve/integration, then a full-screen composite onto the scene (OBSERVED).
2. The grid is 3D `w x h x d` with `w,h` multiples of 8 (cap 640, tile at least 4 px) and `d` 4..128; `w` and `d` come from the draw parameters
   `VolumeWidth` (editor 80..320 step 16) and `VolumeDepth` (8..64 step 4) scaled by a quality-option ratio read from `Gconfig_VolumetricEffectQuality`
   data; `h` follows the aspect (OBSERVED `0x141c614c0`). The shipped ratios are in data, not reached.
3. Depth slices are **logarithmic**: `t(d) = (log2(d+NearOffset) - log2(Near+NearOffset)) / (log2(Far+NearOffset) - log2(Near+NearOffset))`
   with parameters `NearDepth`, `FarDepth` (editor up to 2000), `NearOffset` (OBSERVED `0x141c5ca00`, `0x141c60dd0`, constant `0x142f10b90` = 1/ln 2).
4. Temporal: only the **dir-light shadow** (ping-pong `LightShadow` x2, `EnableReproj`, `FeedbackWeight` 0.1..0.9) and the **fog-volume density**
   (`FogVolumeDensityAcc` x2, `InjectingReprojEnabled`, `InjectingFeedbackWeight`) are reprojected; the jitter is Halton 2/3/5 over `JitterCount`
   (1..16) frames, xy amplitude `JitterXYSize` (OBSERVED). The scatter/extinction volume itself is rebuilt each frame (single `VolumeFogTex`).
5. Lights: the directional light's **colours are parameters of the fog** (`DirLight DirectColor`/`OppositeColor`, gamma 2.2 converted); its direction
   comes from the LightSet by (`LightBankId`,`LightId`); its shadow is sampled per froxel (`ShadowSampleCount` 1..32) (names + code lookup OBSERVED, cascade use INFERRED).
6. Local lights: **all point and spot lights** (light kinds 1 and 2) inside a camera sphere (radius scaled by `LocalLightDistScale`) with
   **`ScatteringScale` > 0** (a per-light editor field, with `ScatteringBlinkInfluence`, `EnableOverrideScatteringG`/`OverrideScatteringG`);
   unshadowed (no per-light shadow in the injection), colour x `LocalLightScale` x `ScatteringScale` x blink factor (OBSERVED `0x141c60aa0`, `0x141a452d0`, `0x141befb70`, `0x141c628f0`).
7. Ambient = a constant colour `SkyLight SkyColor` with `HeightAtten`/`StartDist`; **no** irradiance-volume, SH or env-map sampling and no emissive term
   was found in code or strings (absence over names: INFERRED).
8. `FogVolume` = a placed oriented box (transform from a game entity + half-extents, one FogVolume parameter row id, a density byte) handled by
   `GXSR::GXFogVolume`: **two paths**: a compute `GXFogVolume_WriteInjectingDensity` that injects density into the froxels (queued just before the fog), and a raster ray-march
   pass (box draw, `RayMarching` rows, bilateral/linear up-scale with stencil marking) queued after water; both only exist when the volumetric fog is enabled for the view (OBSERVED, single call site).
9. `VOXELIZATION` (stage index 74 in the stage-name table based at `0x143c28860`) is **not** the fog's grid: the only voxel code in the exe is an
   irradiance-volume baking pipeline (`Voxelize/Injection/Bounce/ComputeSh`, `.ivInfo`, `Voxelization*` model-shader techniques); its executor was not reached (NOT FOUND).
10. No plain `Fog` parameter group exists among ER's 21 draw-parameter groups; fog = the `VolumetricFog` group (which also carries an analytical height fog:
    `EnableAnalyticalFog`, `FogEndDepth`, `HeightFog*`). Forward/transparent/water shaders get the fog by sampling `VolumeFogTex` bound at scene-texture slot `0x2f`
    (a default texture when the fog is off); deferred surfaces get it from the composite pass (binding OBSERVED, shader use INFERRED).

## Q1. Pipeline

### 1.1 Parameter block and classes (OBSERVED)

- Editor definition `FUN_141bcb140` (`0x141bcb140..0x141bcc8b0`), rows recovered with `edparse2.py` into `out_vf_editor_rows.txt`; struct offsets are those of
  `GXSR::GXVolumetricFogParam` (RTTI `0x143d34688`, vtable `0x143045e10`, slot 0 `0x141bc8ee0`, object 0x1a0 bytes). Update job `GXUpdateJob<GXVolumetricFogParamManager>`
  (`0x143d2eab0`, vtable `0x142f1ce68`). Rows (offset, editor range): `Enable` +0x60; `VolumeWidth` +0x64 (80..320 step 16); `VolumeDepth` +0x68 (8..64 step 4);
  `NearDepth` +0x6c (0.01..10); `FarDepth` +0x70 (1..2000); `NearOffset` +0x74 (0..100); `FogEndDepth` +0x78 (0..10000); `GameScale` +0x7c (0.01..100); `DrawTiming` enum
  (`AfterDeferred`,`AfterWater`, strings `0x143045ec0/ee0`); `EnableAnalyticalFog` +0x84; `VolumeSliceSampleOffset` +0x88 (-1..1); `CullingBySceneDepth` +0x8c;
  `CancelInScatteringRateFromSSR` +0x90; `Global`: `FixedDensity` +0xa0, `ScatteringG` +0xa4 (0..0.99), `LocalLightScale` +0xa8 (0..20), `LocalLightDistScale` +0xac (0.01..1),
  `HeightFogDistribution/Offset/ReferenceOffset/ReferenceModeRate/Density Scale` +0xb0..+0xc0, `AbsorptionScale` +0xd0 (colour); `SkyLight`: `SkyColor` +0xe0, `HeightAtten` +0xf0, `StartDist` +0xf4;
  `DirLight`: `LightBankId` +0x100, `LightId` +0x104, `DirectColor` +0x120, `OppositeColor` +0x130, `HeightAtten` +0x140, `StartDist` +0x144, `ShadowSampleCount` +0x148 (1..32),
  `ShadowDither` +0x14c, `AngleClip` +0x150, `EnableReproj` +0x154, `JitterCount` +0x158 (1..16), `JitterXYSize` +0x15c (0..2), `FeedbackWeight` +0x160 (0.1..0.9);
  `FogVolume`: `InjectingSampleCount` +0x170 (1..32), `InjectingDither` +0x174, `InjectingReprojEnabled` +0x178, `InjectingFeedbackWeight` +0x17c (0.1..0.9); `Config`: `SampleCount` +0x180 (1..20), `Dither` +0x184.
  The shipped values are in the GX param files (the other worker tabulates them); `ER-LIGHT-FOG-VALUES.md` in this folder.
- Runtime copy of the parameters: `FUN_141c5db30` (SetParam: copies the param record into the executor object at `+0x1d1..+0x2ac`; calls the (re)allocation `FUN_141c614c0`).
- Classes whose name contains Volumetric/Fog/Froxel/Voxel/Scatter (RTTI scan `out_strings_fog.txt`): `GXSR::GXVolumetricFogParam` (`0x143d34688`), `GXUpdateJob<GXVolumetricFogParamManager>`,
  `GXSR::GXFogVolume` (`0x143d2d580`, vtable `0x142f129d0`), `GXSR::GXFogVolume::_QualitySettingsMessageListener`, `GXSR::GXFogVolumeParam` (`0x143d333e0`, vtable `0x143037a58`),
  `GXUpdateJob<GXFogVolume>`, `GXUpdateJob<GXFogVolumeParamManager>`, `CS::CSRemoVolumeFogAct`, `CS::CSRemoVolumeFogDisableAct`, `CS::CSRemoVolumeFogDisableAllAct` (cutscene actions),
  `GXFLV::GXFlverModelRendererDrawImplVoxel` (vtable `0x142f20da8`). **There is no class named `*VolumetricFog*` for the executor**: the executor is a member object of
  `GXSR::GXSkySimulation` (RTTI `0x143d32150`, vtable `0x142f29478`: slot 0 `0x141b0d610`, slot 1 = Execute `0x141b0db50`) at view+`0xbf20`; its payload object is allocated by `0x141b0da50`
  (init, descriptor default 160 x 90 -> 96 rows, depth 32; `0x141c5cac0`) (INFERRED that this object is "the volumetric fog": it owns every `GXVolumetricFog_*` programme and texture).

### 1.2 Programmes (names in `shader:/` banks; OBSERVED strings at `0x143058570..0x143058b40`, loaded by `FUN_141c60780`, Idx = object offset)

| Off | Name | Loader slot |
|---|---|---|
| +0x08 | `GXVolumetricFog_WriteInScattering` | `0x80` (compute) |
| +0x10 | `..._WriteInScatteringShadowed` (falls back to +0x08 if missing) | `0x80` |
| +0x18 | `..._WriteInScatteringPreShadowed` | `0x80` |
| +0x20 | `..._SolveVolumetricFog` | `0x80` |
| +0x28 | `..._WriteLocalDensityBox` | `0x80` |
| +0x30 | `..._WriteLocalLightPoint` | `0x80` |
| +0x38 | `..._WriteLocalLightSpot` | `0x80` |
| +0x40 | `..._ClearDensityAndLightBuffer` | `0x80` |
| +0x48 / +0x50 / +0x58 | `..._GenerateSceneMaxDepth` / `...H` / `...V` | `0x80` |
| +0x60 / +0x68 / +0x78 | `..._DirLightShadow` / `..._DirLightShadowReproj` / `..._DirLightShadowPostFilter` | `0x80` |
| +0x80 | `..._FullScreenQuad` | `0x40` (vertex) |
| +0x88 / +0x90 / +0x98 / +0xa0 | `..._SceneComposite` / `..._SceneComposite_Transmittance` / `..._SceneComposite_InScattering` / `..._CancelInScattering` | `0x30` (pixel) |

Slot-to-type (`0x80` compute, `0x40` vertex, `0x30` pixel) is INFERRED from usage: the `0x80` handles are invoked with a Dispatch (`FUN_141acad20(ctx, x, y, z)`), `+0x80` is the only one bound as a full-screen vertex stage.
Constant-buffer fields are not named in the exe (no `FC_`-style strings; one 0x4c0-byte buffer filled by `FUN_141c60dd0` from the param copy, the job and the camera: froxel dims and their reciprocals, near/far/offset
and the log2 mapping, view and previous-frame matrices, colours, jitter): NOT FOUND by name.

### 1.3 Volume textures (OBSERVED creation `FUN_141c614c0` `0x141c614c0..0x141c626d6`; names `0x143058b88..0x143058cc8`)

Create-3D = `FUN_141a047b0(tex, w, h, d, fmt, 1,0,1,...)`; create-2D = vtable slot `0xb8`. Format code `fmt` follows DXGI numbering (INFERRED: the same code convention gives `0x1a` = R11G11B10F and `0x18` = R10G10B10A2 elsewhere, ER-POST-CHAIN; never confirmed from a descriptor).

| Object off | Name | Kind | Size | fmt code (DXGI by INFERENCE) |
|---|---|---|---|---|
| +0xc8 | `VolumeFogTex` | 3D | w x h x **d+2** | 10 (RGBA16F) |
| +0xd0 | `AccScatterTex` | 3D | w x h x d | 10 |
| +0x118 | `DensityAccTex` | 3D | w x h x d | 0x2a (R32_UINT) |
| +0x120 | `LocalLightRGAccTex` | 3D | w x h x d | 0x2a |
| +0xd8 | `SceneMaxDepth` | 2D | w x h | 0x36 (R16_FLOAT) |
| +0xe0 | `SceneMaxDepthH` | 2D | w x (screen height / 2) | 0x36 |
| +0x128, +0x130 | `LightShadow` x2 (ping-pong) | 3D | w x h x d | 0x3d (R8_UNORM) |
| +0x138, +0x140 | `LightShadowResult`, `LightShadowResultFiltered` | 3D | w x h x d+2 | 0x3d |
| +0x150, +0x158 | `FogVolumeDensityAcc0/1` (ping-pong) | 3D | w x h x d | 0x36 |

Dimensions (OBSERVED `0x141c614c0`; `cfg` = `*(_DAT_1447f37a8+0x13a60)`, the volumetric-effect quality struct; its `+0xb4/+0xb8/+0xbc/+0xc0` ratios are config data, NOT reached):
`w = round_up8(min(max(16, VolumeWidth*cfg[0xb4]/cfg[0xb8]), screenW*stereo/4, 640))`; `h` = the same width scaled by the screen aspect (`screenH*width/(screenW*stereo*...)`), min 16, `<= screenH/4`, cap 640, round_up8;
`d = clamp(VolumeDepth*cfg[0xbc]/cfg[0xc0], 4, 128)` (not rounded). So tile size is at least 4 px; the cap is 640 x 640 x 128. Constants: the 160 x 96 x 32 default (`0x141b0da50`) only seeds the object. Textures are rebuilt when any of `w,h,d` change.
Depth distribution: logarithmic as in answer 3; the shader samples with `z' = (z + 0.5)/d * d/(d+2)` style correction (`FUN_141c60dd0` writes `d/(d+2)` and `(VolumeSliceSampleOffset-0.5)/d`) (INFERRED from the packing).

### 1.4 Steps in order (the Execute `FUN_141c5cc30` `0x141c5cc30..0x141c5db2f`, reached from vtable slot 1 `0x141b0db50` job kind 0)

1. **Constant buffer** `FUN_141c60dd0` (0x4c0 bytes). **Jitter**: Halton bases 2, 3, 5 on index `job+0xfc`; `xy = (h2-0.5, h3-0.5)*JitterXYSize + 0.5`, `z = h5` (loops with divisors `0.5`, `3.0`, `5.0` at `0x141c5cc30`); index = a counter cycling `JitterCount` frames (4 if a job flag), `-1` (jitter off, constant 0.5) when `JitterCount < 2` (`0x141c5ddf0`). OBSERVED.
2. If nothing to draw (no local lights, no dir light colour, no fog volume) the chain is skipped and the result texture is cleared to a constant (OBSERVED `cStack_698`).
3. `FUN_141c5e6b0`: clear `DensityAccTex` and `LocalLightRGAccTex` (UAV clears). Reprojection history textures are cleared with a constant fill.
4. `FUN_141c605b0` (if `CullingBySceneDepth`): `GenerateSceneMaxDepthH` over `(w>>3) x ((screenH/2+7)>>3)`, then `GenerateSceneMaxDepthV` over `(w>>3) x (h>>3)`: a per-column max scene depth, for froxel culling (INFERRED purpose).
5. **Local lights** `FUN_141c628f0`: for each light record, `FUN_141c5ea20` computes its froxel bounding box by binary searches (`FUN_141c5ecb0` z, `FUN_141c5f270` x, `FUN_141c5fad0` y, testing the light's volume against sub-frusta via a collector virtual call, at most 10 iterations), then
   **one dispatch per light**: `WriteLocalLightPoint` (record type `0x65`) or `WriteLocalLightSpot` (`0x67`), 0x80-byte per-light constants (position/range, colour x `LocalLightScale`, spot axis and cone), UAVs `DensityAccTex` + `LocalLightRGAccTex`, groups `((x1-x0)+7)>>3, ((y1-y0)+7)>>3, (z1-z0)`. A running counter `obj+0x2f8` sums the groups (a stat). `WriteLocalDensityBox` (+0x28) has no call site in the functions read (NOT FOUND where it is dispatched).
6. **Dir-light shadow** `FUN_141c60330` (only with the light object present, `param_5[0xcc]`/debug flag): `DirLightShadow` (or `Reproj` when `EnableReproj`, post-filtered by `DirLightShadowPostFilter` when the filter flag is on) over `(w>>3, h>>3, d)`, writing `LightShadow[ping]` -> `LightShadowResult(+Filtered)`; with `ShadowSampleCount` samples per froxel and `ShadowDither`. The shadow source is an object exposing cascade-like data (4 matrices at job-light+`0x23d0`, a depth/shadow texture at +`0x24d0`) (INFERRED to be the sun's cascaded shadow map).
7. **In-scattering write** `FUN_141c626e0`: `WriteInScattering` (no shadow) / `Shadowed` / `PreShadowed` (chosen by "has shadow" and "pre-shadowed" flags), over `(w>>3, h>>3, d)`, reading the previous `FogVolumeDensityAcc` (SRV slot 8) and writing `DensityAcc`, `LocalLightRGAcc`, the current `FogVolumeDensityAcc` (3 UAVs) (UAV/SRV roles from the binder calls; what each texture holds is INFERRED).
8. **Solve** `FUN_141c613c0`: `SolveVolumetricFog` over `(w>>3, h>>3, 1)`: one thread per column, so an integration along z (INFERRED); UAVs `VolumeFogTex`, `DensityAcc`, `LocalLightRGAcc`.
9. **Application** `FUN_141c5e760`: full-screen draw (`FullScreenQuad` + `SceneComposite`) reading `VolumeFogTex` and the scene depth, onto the scene colour. When three job floats (job+`0x74/0x78/0x7c`) equal three global defaults (`_DAT_1448062e0/e4/e8`) it draws the single `SceneComposite`; otherwise it draws `SceneComposite_Transmittance` then `SceneComposite_InScattering` with two different render-state slots (`g+0x5a8` table `+0x10`, `+0x18`) (INFERRED multiplicative then additive; the condition's meaning NOT resolved).
10. **Job kind 1** `FUN_141c5bc50` (queued by `FUN_141b0d740` at sort id `0x3f` or `0x3a`, sub-order 0 or `0xb`): full-screen `FullScreenQuad` + `CancelInScattering` with three SRVs (`VolumeFogTex` + two textures of the caller): removes the in-scattering that SSR reflected, consistent with `CancelInScatteringRateFromSSR` (INFERRED from names and binding).

### 1.5 Place in the frame

- Submit `FUN_141b0d650` (CPU side `FUN_141c5ddf0`: builds the job, 0x160 bytes) is called from the per-view driver `FUN_141b16f40` at `0x141b17bbc` (disassembly `0x141b17b45..0x141b17bbc`): sort id **`0x1e` when `DrawTiming == 0` (AfterDeferred), `0x2a` otherwise (AfterWater)**, sub-order 9; `DrawTiming` = executor obj +`0x1f0` (`FUN_141b0da20`). OBSERVED.
  The queue key is (view, layer, id, sub-order) as in ER-POST-CHAIN; **these ids are not the stage-name table's indices** (the table at `0x143c28860` has `WATER` at 101, `DEFERRED_COMPOSITE` at 89...; the ids `0x1e/0x2a` live in the per-view queue of `FUN_141b16f40`, whose other submits use `0x1a`, `0x22`, `0x3a`, `0x3e`, `0x3f`, `0x40`). The label "AfterDeferred" / "AfterWater" and the other entries of that driver are the only anchors; whether the queue ids 30/42 map to `DEFERRED_COMPOSITE`..`BACKGROUND` and `WATER`..`TRANSPARENT` is INFERRED from the enum names only.
- The fog-volume density injection job is queued with the same id and sub-order **8** (before the fog's 9); the fog-volume ray-march pass with id `0x2a`, sub-order 9 (see Q4). OBSERVED disassembly `0x141b17bd1..0x141b17c75`.
- Consumers of the fog texture outside the composite: `FUN_141ad33b0(ctx, 0x2f, tex)` binds `VolumeFogTex` (`FUN_141b0d9c0(vf, idx)+0x48`, valid when `DrawTiming <= idx`) at scene-texture slot `0x2f` for layers `0x38` and `0x2f..0x34` (`0x141b18597`, `0x141b18c50`, `0x141b18f95`), with the device default texture `0x13` as fallback when the fog is disabled, and `FUN_141ad37b0(ctx, 0x2b, 1)` sets a flag. So forward-shaded geometry (forward opaque, water, transparents) can apply the fog itself; deferred opaque gets it from the composite (INFERRED roles). The sky is not named in the fog code: no sky-specific handling was read (NOT FOUND). The stage list around it: `DEFERRED_LIGHT_ACC(_POST)`, `DEFERRED_SSS`, `DEFERRED_COMPOSITE`, `BACKGROUND`, `DEFERRED_END`, `FORWARD_PRE`, ..., `FORWARD_OPAQUE`, `SECOND_SPECULAR`, `SKY_CLOUD`, `WATER_INTERACTION`, `PREPARE_REFRACTION`, `WATER`, ..., `TRANSPARENT` (OBSERVED names, `qdump.py 143c28860`).

## Q2. Which lights feed it

- **Directional**: colours are fog parameters (`DirectColor`, `OppositeColor`: forward/opposite phase colours; `FUN_141c60dd0` converts them with gamma 2.2 (`_DAT_142ba2380` = 2.2) and a scale into the CB: OBSERVED). Direction: `FUN_141a42340(*(g+0xc528), &out, obj+0x248)` looks up the (`LightBankId`,`LightId`) pair (`GXLightManager`, vtable `0x142f116c8`; the LightSet group has Directional Light 0..2 angle/colour/hemisphere rows) and stores a vec3 at job+`0xc` (OBSERVED lookup; that it is the sun direction INFERRED). Shadow: the `DirLightShadow*` programmes, `ShadowSampleCount` samples, `AngleClip`, reprojection + post-filter (names + binding OBSERVED; use of the cascades INFERRED). `HeightAtten`/`StartDist` per light group.
- **Local point/spot**: `FUN_141c60aa0` iterates the view's light vector (`view+0x28..0x30`), keeps lights whose kind field (`light+0xa8`) is 1 or 2 and that return a record from vtable slot 6 (`GXPointLight` slot 6 `0x141a452d0`, `GXSpotLight` `0x141a45d90`) called with a camera sphere (radius 10.0 x `LocalLightDistScale`, `0x143032ea0`) and flags `{1,1}`; with flag byte 1 the record is created only if **`ScatteringScale` (light+`0xf8`) > 0** (`0x141a452d0`: `if (0 < fade && (*param_4 == 0 || 0 < *(float*)(light+0xf8)))`). The draw record (`FUN_141befb70`, type `0x65`) in this mode multiplies the colour by `ScatteringScale`, mixes the blink factor by `ScatteringBlinkInfluence` (`1-(1-blink)*influence`), applies an exponential distance factor, and the record is dispatched per light (Q1 step 5). No per-light shadow in the injection. All OBSERVED. Per-light editor rows (`FUN_141aed4d0`, `GXLightBase` vtable `0x142f261c0` slot 8): `ScatteringScale` +0xf8, `ScatteringBlinkInfluence` +0xfc, `EnableOverrideScatteringG` +0x100, `OverrideScatteringG` +0x104, `AntiExposureRate`, `Translucency Intensity/Power`, `Irradiance Volume Direct/Indirect Light/Scale`, `Bake Shadow` (editor names; that `OverrideScatteringG` replaces the fog's phase factor for that light is INFERRED).
- **Ambient**: `SkyLight SkyColor` + `HeightAtten` + `StartDist` (OBSERVED names/rows; the code packs the colours with the same 2.2 conversion). `FogVolume Light AmbientColor` overrides it inside a fog volume (Q4). No SH / irradiance-volume / env-map read near the fog code: the editor strings `Irradiance Volume ...` belong to the light class's bake rows (INFERRED unrelated).
- **Emissive**: nothing found (NOT FOUND, absence of strings only).
- Global: `FixedDensity` + height-fog density (`HeightFog*`) give the base extinction; `AbsorptionScale` an absorption colour; `ScatteringG` the HG phase g (editor names; their use inside the shaders INFERRED).

## Q3. `VOXELIZATION`

- Stage table: names at `0x143c28860` (BEGIN, GBUFFER, ABOVESHADOW_*, shadows..., `VISIBILITY_TEST`(53)); `VOXELIZATION` appears twice: index 55 (inside a second enum `VOXELIZATION, NORMAL, FILTER, PASS_END`, `0x143c28a18`) and **index 74** (`0x143c28ab0`, between `DEPTHSCALE`(73) and `RAYTRACING_POSTGEOM`(75)) Table read by `0x1457e2d64` only (a name lookup, not read). OBSERVED.
  Indices are `(entry address - 0x143c28860)/8` (e.g. `(0x143c28ab0-0x143c28860)/8 = 74`).
- No code references the name or an executor for it that I could find (RIP scan of the table bases `143c28860/68`, `143c28a10`, `143c28a18`, `143c28ab0`: only the lookup at `0x1457e2d64` in the protected image). **Executor NOT FOUND.**
- What voxel code exists (OBSERVED strings): model-material technique names `Dbg`, `Rsm`, `Voxelization`, `VoxelizationGBuf`, `VoxelizationGBufSvo` (table `0x143c27bc8`, used at `0x141ac7f71`, `0x1416ef6b8`); `GXFLV::GXFlverModelRendererDrawImplVoxel` (vtable `0x142f20da8`); a state list `Waiting, Setup, Voxelize, Injection, InjectionLocalLight, Bounce, ComputeSh, BakingLocalLightSh, Denoise, Encode, State` (table `0x143c295f0`); outputs `%s_X.dds .. %s_W.dds`, `%s.ivInfo`; `GXIrradianceVolumeAvgSh(Lt)` programmes (`0x143033b98`, code `0x141b351c8`); `Draw.CaptureEnvMap.IV.Legacy/Open.safeVoxelNum`; `capture:/texturebake`. This is an **irradiance-volume (IV) bake** (voxelise the scene, inject lights, bounce, project to SH, denoise, encode); `.ivinfobnd` per map is the loaded result (`IvInfoRepository`). **INFERRED:** the `VOXELIZATION` stage serves that bake (the model renderer's voxel variant), not the fog.
- Evidence against "fog = voxelised FogVolume": the fog's FogVolume path is a compute injection (`GXFogVolume_WriteInjectingDensity`, Q4) and no `GXVolumetricFog_*` programme has a voxel name.

## Q4. `FogVolume`

- Class `GXSR::GXFogVolume` (manager at `*(_DAT_1447f37a8 + 0xccc0)`, vtable `0x142f129d0`); parameters `GXFogVolumeParam` (editor `FUN_141a7ed40`, rows in `out_fv_editor_rows.txt`): `DrawArea` (`NearFadeMin/MaxDistance`, `EdgeFadeDistance`, `EdgeFadeExponent`, `HeightFallOff`, `FadeDistanceForHeightMap`, **`MaxInjectingDepth`** 0..20000), `RayMarching` (`MaxSampleCount` 8..64, `SteppingDistance`, `SuspendedTransmittance`, `LightProcessingInterval` 1..8, `LightProcessingIntervalThreshold`), `Light` (`Overwrite`, `DirectionalLightAngle/Color/OppositeColor`, `AmbientColor`, `ShadowingIntensity`, `AttenuationIntensity/Offset`, `OctaveNumPerlinNoise` 1..3), `Density Intensity`, `PerlinNoise` (`Frequency`, `Lacunarity`, `Gain`, `Offset`), `Scroll` (`Speed-xyz`, `DetailScale`). OBSERVED.
- **Run-time object = an oriented box.** Creation `FUN_141a2f980(mgr, &idOut, &record, flag)` allocates a 0x120 volume object + a 0x70 transform/extents record in the manager arrays (capacity set at `0x141a30d50`). The game-side caller `FUN_140a9c360` (`0x140a9c360..0x140a9c5ce`; second caller `0x14072eff6` not read) builds the record from its owner entity: a 4x4 transform (`vtable +0x28` of the entity helper), **half-extents `size * 0.5`** (floats at object `+0xa5c/+0xa60/+0xa64`), the FogVolume parameter row id (object `+0x9c4`), a handle and a density byte (`+0xae9`/255, passed to `FUN_141a277a0`). So volumes are placed boxes from a map/game entity (not meshes) plus the cutscene actions `CSRemoVolumeFogAct/DisableAct/DisableAllAct`. Where the entity's data is authored (MSB region/part type) was not read (NOT FOUND).
- **Two paths** (OBSERVED loaders `FUN_141a30d50`: 1 vertex programme `GXFogVolume` (`0x142f129d8`), 12 pixel programmes (`GXFogVolume`, `_noShadowing`, `_noLiteAttenuateLight`, `_noShadowingAndLiteAttenuateLight`, `_bilateralUpScaleWithStencilMarking`, `_linearUpScaleWithStencilMarking`, `_8SampleBilateralDepthBlur`, `_SampleBilateralColorBlur`, four `GXFogVolumeDbg_*` up-scales), 1 compute programme `GXFogVolume_WriteInjectingDensity` (`0x142f12de0`, slot `0x80`), a tessellation name `GXFogVolume_Tess`):
  1. **Injection into the froxels**: `FUN_141a2ff30` (queued at the fog's id, sub-order 8, needs the volumetric fog enabled: `view+0xbf30`) copies the fog's constants (`FUN_141b0d900`) into a job whose executor `FUN_141a38400` -> `FUN_141a340f0` records, per volume, a **compute dispatch of `WriteInjectingDensity`** over the volume's froxel footprint (group counts from the froxel tile size `param_5+0x90/0x94`), writing the volumetric fog's UAV (`param_5+0x50`), then the fog's `Solve` integrates it. Controlled by `FogVolume InjectingSampleCount/Dither/ReprojEnabled/FeedbackWeight` in the VolumetricFog group and by `MaxInjectingDepth` per volume. OBSERVED code; the exact split by `MaxInjectingDepth` INFERRED.
  2. **Ray-march raster**: `FUN_141a2fd30` (queued at id `0x2a`, sub-order 9, i.e. after water, only inside the same "fog enabled" block; single call site `0x141b17c75`) -> executor `FUN_141a38390` -> `FUN_141a31b20` (7004 bytes, not read) draws each volume's box with the `GXFogVolume` vertex/pixel programmes (per-volume constants 0x160 bytes, light collector data `+0x24d0`), a ray-march with the `RayMarching` rows, and bilateral/linear **up-sampling with stencil marking** (so the march is done at reduced resolution; INFERRED). Strings `Draw.FogVolume.IsDisablePolygonClippingNoiseSuppression` (`0x142bec3a0`) too.
- Enabling conditions: manager `+0x10`, flags `+0x159`, `+0x15a` (quality) in both submits. Quality listener `_QualitySettingsMessageListener`.

## Q5. The old fog

- No plain `Fog`, `Atmosphere` or `Haze` group among the 21 draw-parameter editor groups (list: `evidence/I-0138/artifacts/er-values/er-groups-params.txt`, headers: LightSet, SSAO, Setting Info, Tone Map, Bloom, LightShaft, Anti-Alias, MotionBlur, FurBlur, FeedBackBlur, ColorGrading, ShadowParam, Glow, Lens Simulation, RayTracing, VolumetricFog, FogVolume, EnvironmentConfig, WaterInteraction, Wet, SSR). The exe has no `FC_FogParam`-like string (`FC_` occurs once, as noise). Strings `_50_Fog`, `_51_HeavyFog`, `_52_HeavyFogRain`, `HeavyFog`, `HeavyFogRain` are weather names (`0x142bc8ce0..`, `0x142b4ce00`). OBSERVED.
- The VolumetricFog group carries its own **analytical (non-volumetric) height fog**: `EnableAnalyticalFog`, `FogEndDepth` (0..10000, beyond the froxel range), `FixedDensity`, `HeightFogDistribution/Offset/ReferenceOffset/ReferenceModeRate/Density Scale` (`0x143045f10`, `0x143046060..0x143046120`). OBSERVED names; that the fog is continued analytically from `FarDepth` to `FogEndDepth` in the solve/composite shaders is INFERRED.
- How applied (Q1.5): deferred surfaces by `SceneComposite*`; forward/water/transparent by sampling `VolumeFogTex` through the scene slot `0x2f` (fallback default texture when off). So ER has one fog (the volumetric one + its analytical tail), not two combined fogs; whether any forward shader still evaluates a private distance fog beside it cannot be seen (shaders unreadable): NOT REACHED.

## Q6. `EnvironmentConfig` (names only)

Editor `FUN_141bcde50` (vtable `0x143047770`, RTTI `GXSR::GXEnvironmentConfigParam` `0x143d34708`): `EnvMapColorConvert Hue`, `Saturate`, `Shift` (colour), `Scale` (colour) (`0x143047c60..0x143047dd0`); present in 44 of the draw-param files (I-0138 census). The consumer is the env-map side (`GXSR::GXEnvMapFilter` `0x143d321e0`, `GXEnvironmentManager` `0x143d2e340`), not read: INFERRED from the name only.

## Function index (decompiled; `exp*/<entry>.c` in the output folder)

`0x141c60780` programme loader; `0x141c614c0` texture (re)allocation; `0x141c5cac0` init; `0x141c5db30` SetParam; `0x141c5ddf0` Submit; `0x141c5cc30` Execute (main); `0x141c5bc50` Execute kind 1;
`0x141c5e6b0` clear; `0x141c605b0` scene max depth; `0x141c628f0` local lights; `0x141c5ea20/ecb0/f270/fad0` light froxel bounds; `0x141c60330` dir-light shadow; `0x141c626e0` write in-scattering;
`0x141c613c0` solve; `0x141c5e760` composite; `0x141c60dd0` constant buffer; `0x141c60aa0` light collection; `0x141c5ca00` depth-to-slice; `0x141b0d650/0x141b0da50/0x141b0da20/0x141b0d740/0x141b0d9c0/0x141b0d900/0x141b0db50`
GXSkySimulation members; `0x141b16f40` per-view driver (call sites); `0x141bcb140` VolumetricFog editor; `0x141aed4d0` GXLightBase editor; `0x141a452d0/0x141a45d90/0x141befb70` point/spot light records;
`0x141a30d50/0x141a2dbc0/0x141a2f980/0x141a2fd30/0x141a2ff30/0x141a38390/0x141a38400/0x141a340f0/0x141a7ed40` GXFogVolume; `0x140a9c360` game-side creator; `0x140b1c7b0` graphics-config reader (ratios are file data).

## Not reached

- The values of the quality ratios `cfg[0xb4/0xb8/0xbc/0xc0]` (data in the graphics config), hence the actual froxel counts per quality level; the shipped parameter values (other worker).
- Shader bytecode and constant-buffer field names (not in the exe); therefore the exact integration (energy-conserving?), the phase function, how `AngleClip`, `AbsorptionScale`, `VolumeSliceSampleOffset`, `Config SampleCount/Dither` are used, and what each accumulation texture holds.
- The executor of the `VOXELIZATION` stage (only names/tables found), `WriteLocalDensityBox`'s call site, the body of `FUN_141a31b20` (ray-march raster, 7004 bytes), `FUN_141c5ecb0/f270/fad0` beyond their role, `FUN_141ac7f71` (technique table user), the second FogVolume creator `0x14072eff6`, the authoring of fog volumes in map data.
- Which draw stage(s) the sort ids `0x1e/0x2a` correspond to exactly; the cutscene-action code of `CSRemoVolumeFog*`.
- The sun-shadow object passed to the fog (its cascade layout) and whether `DirLightShadow` uses a dedicated shadow or the main cascades.

## Scripts and arguments (all under `C:/Users/User/Tools/DS3-Decompilation/study-ws12/er-fog/scripts/`)

- `Run-ERfog.ps1 -Script <java> -Out <dir> <args...>`: hidden, Idle priority, `-readOnly -noanalysis -process` on `er-mirror-vignette`, refuses on a `*.lock*` file. All Ghidra results below used it.
- `ERExport.java <outdir> <addr>...` (copy of the I-0109 script): `exp1` = `141c60796 141c62261 141c622cf 141c62659 141bcb140 141bc8ee0 141a30f68 141a2e23d 141a7ed8f 141bcc510 141aeda0b`; `exp2` = `141c5cba1 141c5ddd5 141c5de6b 141a047b0`; `exp3` = `141b0d6ec 141b0dad0`;
  `exp4` = `141c5ea20 141c5ecb0 141c5f270 141c5fad0 141c60330 141c605b0 141c60aa0 141c60dd0 141c5e760 141c5cc30 141c626e0 141c613c0`; `exp5` = `141c628f0 141c5e6b0 141c5ca00`; `exp6` = `141a452d0 141a45d90 141aedda0 141b0db50 141b0d610 141b17bbc`; `exp7` = `141a2ff30 141a2fd30 141b0da20 141befb70`;
  `exp8` = `141a38390 141a38400`; `exp9` = `141a340f0`; `exp10` = `141a2f980 140a9c52d`; `exp11` = `140b1c938`; `exp12` = `141c5bc50 141b0d740`; `exp13` = `141b0d9c0 141b0d900`.
- `ERListFns.java <out> <start> <end>`: `fns1.txt` (`141c5b000 141c62800`), `fns2.txt` (`141a2b000 141a3a000`).
- Python on the exe on disk (`py -I` / `py -E -s`): `strscan.py <out> <needles...>` (`out_strings_fog.txt`: `volumetric froxel voxel scatter fogvolume fog lightshaft "light shaft" inscatter volume`; `out_strings_sky.txt`; `out_strings_gi.txt`; `out_rtti_*.txt`);
  `riprefs2.py <exe> <va...>` (`out_riprefs1.txt`, `out_qrefs_gi.txt`); `qrefs.py <va...>` (`out_qrefs1.txt`, `out_qrefs2.txt`); `callrefs.py <exe> <va...>` (direct CALL/JMP sites); `rtti.py <typedescriptor...>`; `vtdump.py <va>` (RTTI of a vtable slot);
  `wrange.py <start> <end>` (strings in a range); `qdump.py <va> <n>` (qword table with strings); `pev.py f|x|w|a` (floats/bytes/strings); `edparse2.py <decompile.c>` (editor rows -> `out_vf_editor_rows.txt`, `out_fv_editor_rows.txt`).
- Disassembly evidence for the sort ids: `exp6/141b16f40.asm.txt` lines `141b17b45..141b17c75`.
