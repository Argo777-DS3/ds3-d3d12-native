# Elden Ring 2.7.1 post-processing chain: static study (I-0138)

Copied from `evidence/I-0138/artifacts/er-post-chain/ER-POST-CHAIN.md` (2026-11-12, a Sonnet `ghidra-research` worker for the workstream 11 pilot; its scripts
and raw outputs stay there). Its statuses are the worker's; the pilot's reading is in [STUDY](STUDY.md) §5.

Binary `eldenring_2.7.1.0_25080141_1A354710.exe` (image base `0x140000000`, Ghidra project `ER_2_7_1_25080141_I0001`, mirror
`er-mirror-i0072-a`, headless 12.1.3 `-process -readOnly -noanalysis`, JVM at Idle priority, one run at a time; 10 headless runs, the
rest is Python on the exe on disk). Nothing launched, nothing written outside this folder. All addresses are ER virtual addresses.
Function names are mine (`FUN_<addr>` in Ghidra, no symbols). Statuses: OBSERVED = read in the decompile / bytes; INFERRED = reasoned
from observed code or names; NOT FOUND.
All raw outputs are in this folder (`exp1`..`exp8` = decompile `.c` + `.asm.txt` per function, `s*.txt` string scans, `rtti_*.txt`,
`ptrtab_s2.txt`, `editor_params2.txt`, `xrefs_a.txt`, `xrefs_b.txt`). Scripts and arguments are listed at the end.

## 0. The one-paragraph answer

The YEBIS 2 middleware (`PPFX`, `CPostEffect`, `CTinyEffect`, `tech_*`, `TonemapHDR`, `BeginPostEffectScene`, `GXYebis2Filter`,
`GPUInterfaceDevice`) is **absent** from ER 2.7.1: none of these strings exist. What remains of "Yebis" is a naming legacy: editor
labels `Tone Map[Yebis]`, `Bloom[Yebis]`, `LightShaft[Yebis]`, `MotionBlur-Editor[Yebis]`, `FeedBackBlur[Yebis]`, `ColorGrading[Yebis]`,
`Setting Info[Yebis]`, `Yebis-Anti-Alias Enable`, and one base class `GXBS::GXYebisFilterBase` (vtable `0x142f17b98`, 41 slots,
instantiated by the `GXPostProcessManager` constructor, a shared full-screen-draw helper). The post chain is FromSoftware's own
code: a set of `GXBS::GX*Filter` classes all deriving from `GXBS::GXPostProcessEffect` (vtable `0x1430504b0`, constructor
`0x141c1ee20`), each with `Submit` (CPU, fills a job and queues it) and `Execute` (vtable slot 1, runs the passes) functions, shader
programmes loaded **by name** from the bank `shader:/GXPostEffect.shaderbnd` (`ToneMap_*.ppo`, `LensSimu2_*.ppo`,
`MotionBlur_*.ppo`, ...), not embedded. The per-view driver is `0x141c7ce60`. It does not execute the filters in call order: every
filter queues a 24-byte command whose sort key is (layer, id, sub-order), and the queue is merge-sorted
(`0x141ad1b30`/`0x141ad1a20`); the order below is the **sort-key order** (INFERRED that the executor walks the sorted list ascending).

## Chain order (HDR lit scene to final output), main view

Key = (id, sub-order) of each job; layer 0x38 jobs sort before layer 0x39 jobs. Ids and sub-orders are the constants in the call
sites of `0x141c7ce60` (disassembly `MOV R9D/R8D`), re-checked in each Submit (`FUN_141ad3740(ctx,id,sub)` just before
`FUN_141ad2b40` queues the job).

| # | Layer | (id, sub) | Stage | Submit | Execute | View field |
|---|---|---|---|---|---|---|
| 1 | 0x38 | (0x28, 7) | Light shaft sky-mask + radial/parallel blur (job kind 1 of the tone map filter) | `0x141b8ea80` (second queue inside) | `0x141b90210` case 1 -> `0x141c80b10` | V+0xbf50 |
| 2 | 0x38 | (0x3a, 7) | Outline (`GXOutlineFilter`, incl. `Outline_TempAAResolvePS`) | `0x141bd6d40` | `0x141bd6e30` (no Ghidra function) | V+0xbf88 |
| 3 | 0x38 | (0x3a, 8) | Fur blur (second `GXMotionBlurFilter` instance, job kind 3) | `0x141b488e0` | `0x141b48cb0` | V+0xc0a0 |
| 4 | 0x38 | (0x3a, 9) **or** (0x3e, 9) | Depth of field (`GXLensSimulationFilter2`): id 0x3a when the param `PreTempAA` is on (before TempAA), else 0x3e (after it) | `0x141bd7460` | `0x141bdbda0` -> `0x141bd7ac0` | V+0xc090 |
| 5 | 0x38 | (0x3a, 10) | TempAA (AA mode 4), HDR colour | `0x141ad6640` | `0x141ad79a0` | V+0xbac0 |
| 6 | 0x38 | (0x3e, 11) | Motion blur (camera + object velocity, tile max, gather) | `0x141b47f00` | `0x141b48cb0` | V+0xc098 |
| 7 | 0x38 | (0x3e, 12) | DoF second job (queued only when the job flag `+0x8f` is set) | `0x141bd7460` | `0x141bdbda0` | V+0xc090 |
| 8 | 0x38 | (0x40, 13) | Glow (`GXGlowFilter`: emissive glow pyramid, "CustomBloom") | `0x141b89640` | `0x141b89c40` | V+0xbcf0 |
| 9 | 0x38 | (0x40, 14) | Tone map filter: auto exposure, bloom/glare, light-shaft composite, colour grading/LUT, vignette, chromatic aberration, feedback blur end passes, OETF | `0x141b8ea80` | `0x141b90210` -> `0x141b92690` | V+0xbf50 |
| 10 | 0x39 | (0x3e, 19) | Scene bokeh (`GXSceneBokehFilter`, simple blur mip chain, runs only when enabled) | `0x141bdc7c0` | `0x141bdcb10` | V+0xbf90 |
| 11 | 0x39 | (0x40, 15) | Spatial AA (FXAA2/FXAA3/FXAA3HQ/DLAA) with optional Sharpen, only when AA mode != 4 | `0x141ad6640` | `0x141ad79a0` | V+0xbac0 |

Then the UI (profile markers `Velocity`, `HDRPostProcess`, `PostProcess`, `UI`, `Debug` in that order at `0x143c26548..0x143c26568`;
the UI is not in `0x141c7ce60`). The earlier TAA study read these stages by call order and mislabelled three classes; corrected here:
V+0xbcf0 is the **Glow** filter (not the tone map), V+0xbf50 the **tone map**, V+0xbf88 the **Outline** (not "DOF final composite").
Its conclusion "TempAA runs on HDR colour before the tone map" holds (key (0x3a,10) < (0x40,14)).

What was not resolved: which render target each stage reads and writes (V+0x2b0, V+0x2c8, V+0x2d8, V+0x248 are passed as texture
handles; the tone map writes `*(V+0x248 obj + 0x30)`, the spatial AA works in place on `FUN_141a1cc10(V+0x248)`), so the HDR/LDR
boundary is INFERRED to sit at the tone map (layer 0x38 -> 0x39), not proven; the shipped value of `PreTempAA` (GX param data).

## 1. Is YEBIS still in 2.7.1? (Q1)

| Item | Evidence | Status |
|---|---|---|
| `YEBIS` as library | `strscan.py` needles `yebis ppfx posteffect tinyeffect tech_ tonemaphdr gxyebis postprocess GPUInterfaceDevice` -> `s1.txt` (63 hits): no `PPFX`, `CPostEffect`, `CTinyEffect`, `tech_*`, `TonemapHDR`, `GXYebis2Filter`, `BeginPostEffectScene`, `GPUInterfaceDevice`; no ASCII "yebis" at all | OBSERVED (absence) |
| "Yebis" in editor labels only | UTF-16: `Setting Info[Yebis]` `0x14303dc90`, `Tone Map[Yebis]` `0x14303e498`, `Bloom[Yebis]` `0x14303f3b0`, `LightShaft[Yebis]` `0x14303faf8`, `MotionBlur-Editor[Yebis]` `0x143040770`, `FeedBackBlur-Editor[Yebis]` `0x143041250`, `ColorGrading[Yebis]` `0x143041878`, `Yebis-Anti-Alias Enable` `0x143052aa0`, `Yebis-FeedbackWeight`, `Yebis-LutSourceId`, `Yebis-LightShaftAngle` | OBSERVED |
| Class `GXBS::GXYebisFilterBase` | TypeDescriptor `0x143d2db90`, vtable `0x142f17b98` (41 slots, slot 0 `0x141a5c330`); built by `GXPostProcessManager` ctor `0x141a5be30` (`alloc 0x380`, `FUN_141c1ee20` then `*p = &UNK_142f17b98`); `GXBS::GXPostProcessManager` vtable `0x142f17ce8` (12 slots); both live | OBSERVED (class, ctor); role as shared full-screen helper INFERRED |
| Shader names | `ToneMap_BrightPassTemporalPS.ppo`, `LensSimu2_ScaledTemporal.ppo` are FromSoft programmes in the bank `shader:/GXPostEffect.shaderbnd` (`0x142b6a208`); loaders resolve names with `FUN_141a5e740/e880/e620` (pixel/vertex/compute) | OBSERVED |
| Conclusion | "Yebis" is a legacy name over FromSoftware's own filters (tone map table variants `Filmic/Sensitometric/Reinhard/ReinhardEx` and `LensSimu2` keep the YEBIS-era vocabulary, lineage INFERRED); no middleware code or API is called | INFERRED from observed absences |

## 2. Inventory of post filter classes (Q2)

RTTI from `rttilist.py` (`rtti_gxbs.txt`). Vtable slot 0 = scalar-deleting dtor, slot 1 = Execute (`FUN_x(filter, ctx, job*)`; job
kind in `job[1]`). All derive from `GXPostProcessEffect` (ctor `0x141c1ee20`, called by every ctor below). Built in the view ctor
`0x141a19c80` (only caller of each ctor).

| Class | TypeDescriptor | Vtable | Dtor | Execute | Constructor | Submit / notes | Status |
|---|---|---|---|---|---|---|---|
| `GXAntiAliasingFilter` | `0x143d318a8` | `0x142f242b8` | `0x141ad64f0` | `0x141ad79a0` | `0x141ad6240` | submit `0x141ad6640`; see ER-TAA-STUDY | OBSERVED |
| `GXMotionBlurFilter` (2 instances: motion blur V+0xc098, fur blur V+0xc0a0) | `0x143d32e08` | `0x143035e88` | `0x141b47ec0` | `0x141b48cb0` (job kinds 0..3) | `0x141b47a50` | submits `0x141b47f00` (kind 0), `0x141b488e0` (kind 3); loader `0x141b48b50` | OBSERVED |
| `GXGlowFilter` | `0x143d34098` | `0x14303b440` | `0x141b89600` | `0x141b89c40` | `0x141b89450` | submit `0x141b89640`, param copy `0x141b8ad70`, texture getter `0x141b89880` | OBSERVED |
| `GXToneMapFilter` (also hosts bloom/glare, light shaft, colour grading, feedback blur, vignette, chromatic aberration) | `0x143d34168` | `0x14303b958` | `0x141b8e5f0` | `0x141b90210` (job 0 -> `0x141b92690`, job 1 -> light shaft `0x141c80b10`) | `0x141b8ddf0` | submit `0x141b8ea80`, loader `0x141b8f420` | OBSERVED |
| `GXLensSimulationFilter2` (depth of field) | `0x143d349b0` | `0x143049380` | `0x141bd7240` | `0x141bdbda0` -> `0x141bd7ac0` | `0x141bd6ea0` | submit `0x141bd7460`, param copy `0x141bdc5d0`, loader `0x141bd9b10` | OBSERVED |
| `GXSceneBokehFilter` | `0x143d349e8` | `0x14304a078` | `0x141bdc740` | `0x141bdcb10` | `0x141bdc690` | submit `0x141bdc7c0`, loader `0x141bdc970` | OBSERVED |
| `GXOutlineFilter` | `0x143d34980` | `0x1430492c8` | `0x141bd6c10` | `0x141bd6e30` (Ghidra has no function there) | `0x141bd6ba0` | submit `0x141bd6d40` | OBSERVED |
| `GXDepthScaleFilter` (depth down-scale utility for AO/DoF) | `0x143d34068` | `0x14303b2a8` | `0x141b88c10` | `0x141b89320` | `0x141b88b10` | `DepthScale2x2*.ppo` | OBSERVED |
| `GXSSAOFilter` (out of scope) | `0x143d34040` | `0x14303ab48` | `0x141b85810` | `0x141b862f0` | `0x141b854a0` | | OBSERVED |
| `GXSettingInfoFilterParam` | `0x143d340c0` | `0x14303b570` | `0x141b8af90` | - | `0x141b8adb0` | | OBSERVED |
| `GXColorGradingFilterParam` | `0x143d340f8` | `0x14303b840` | `0x141b8bf70` | - | `0x141b8b6f0` | | OBSERVED |
| `GXToneFilterParam` | `0x143d34198` | `0x14303cb58` | `0x141b96920` | - | `0x141b967c0` | | OBSERVED |
| `GXBloomFilterParam` | `0x143d341c8` | `0x14303cc00` | `0x141b975d0` | - | `0x141b974d0` | | OBSERVED |
| `GXLightShaftFilterParam` | `0x143d341f8` | `0x14303cc60` | `0x141b97d60` | - | `0x141b97a90` | | OBSERVED |
| `GXMotionBlurFilterParam` | `0x143d34230` | `0x14303cfd0` | `0x141b988c0` | - | `0x141b98780` | | OBSERVED |
| `GXFeedBackBlurFilterParam` | `0x143d34268` | `0x14303d030` | `0x141b98fc0` | - | `0x141b98ee0` | | OBSERVED |
| `GXAntiAliasFilterParam` | `0x143d37ce0` | `0x14305ba68` | `0x141c8f710` | - | | | OBSERVED |
| `GXGlowFilterParam` | `0x143d37d18` | `0x14305bac8` | `0x141c902c0` | - | `0x141c901d0` | | OBSERVED |
| `GXLensSimulationFilterParam` | `0x143d37d48` | `0x14305bb28` | `0x141c90810` | - | `0x141c906d0` | | OBSERVED |
| `GXUpdateJob<GX*ParamManager>` (one per param class: SSAO `0x142f1cd88`, SettingInfo `0x142f1cd98`, Tone `0x142f1cda8`, Bloom `0x142f1cdb8`, LightShaft `0x142f1cdc8`, AA `0x142f1cdd8`, MotionBlur `0x142f1cde8`, FeedBackBlur `0x142f1cdf8`, ColorGrading `0x142f1ce08`, Glow `0x142f1ce18`, LensSimulation `0x142f1ce28`) | | | | | | the `*ParamManager` classes themselves have no RTTI of their own, only inside these templates | OBSERVED |

No class exists for: sharpen (a programme inside the AA filter + generic `SampleColorSharpness` in the post manager), vignette,
chromatic aberration, colour grading (all inside the tone map filter, parameters in `GXToneFilterParam`/`GXColorGradingFilterParam`),
film grain (NOT FOUND, no string), heat haze / distortion and lens flare and radial blur: these are **effect-system appearances**
(`GXFFX::GXFfxAppearanceDistortion` `0x143d387b8`, `GXFFX::GXFfxLensFlareAppearance` `0x143d38f58`, `GXFXCG::GXFXCGAppearanceRadialBlur`
`0x143d39878`, shaders `GXFfxdistortion*`, `GXFfxtessellateLensFlare`, `GXFfxradialBlur`), particles in the `Effect` stage, not
post filters. Feedback blur has no filter class: only the param class, mask programmes (`FeedBackBlurMaskPS/MaskMargePS/FallOffPS.ppo`,
loader inside `0x141b1a7a0`) and the tone-map end programmes `ToneMap_LUTFeedBackSeparateEndPS`, `ToneMap_PostOETFFeedBackPS`.

## 3. Order of the chain (Q3)

| Finding | Address / excerpt | Status |
|---|---|---|
| Driver | `FUN_141c7ce60` called only from `FUN_141c7cb90` (`0x141c7ce1b`) when `param_10 == 0x38`; `FUN_141c7cb90` is called from the view-setup functions `0x141c7a580` (main view), `0x141c7ace0`, `0x141c7b360`, `0x141c7c580` | OBSERVED (`xrefs_b.txt`) |
| Submit pattern | each Submit does `FUN_141ad3740(ctx, id, sub)` (sets `ctx+0x118 = id`, `ctx+0x11c = sub`, `ctx+0x552 = 2`) then `FUN_141ad2b40(ctx, filter, &job)` queues `{key u32, ptr, flag byte}` (24 bytes, `FUN_141ad1f10`) | OBSERVED |
| Sort key | `key = ((ctx+0x110 << 6 \| ctx+0x114 & 0x3f) << 7 \| id & 0x7f) << 16 \| sub`; comparator in `FUN_141ad1a20` orders by the flag byte (`ctx+0x552`) first, then by key, ascending (stable merge sort, `FUN_141ad1b30`). `ctx+0x110` = view index, `ctx+0x114` = layer (the ctx constructor `FUN_141b2eba0(ctx,.., view, 0x38)` / `(..,0x39)` passes it to `FUN_141ad21b0`, stored at `+0x114`) | OBSERVED code |
| Ids and sub-orders | MB `(0x3e,0xb)` (`MOV R9D,0xb; LEA R8D,[R9+0x33]` at `0x141c7d17d`), fur blur `(0x3a,8)` (`0x141c7d1f4`), DoF `(0x3a,9)` (`0x141c7d32a`) with a second id 0x3e and second job `(0x3e,0xc)` chosen inside `0x141bd7460`, tone map `(0x40,0xe)` (`0x141c7d3fe`), TempAA `(0x3a,0xa)` (`0x141c7d4b1`), glow `(0x40,0xd)` (`0x141c7d53e`), outline `(0x3a,7)` (`0x141c7d5c7`), then the second ctx (layer 0x39): scene bokeh `(0x3e,0x13)` (`0x141c7d6bb`), spatial AA `(0x40,0xf)` (`0x141c7d686`); tone map second queue `(0x28,7)` (args of `0x141b8ea80`) | OBSERVED |
| DoF id depends on `PreTempAA` | `0x141bd7460`: `uVar5 = param_5 /*0x3e*/; if (*(char*)(param_1+100) != 0) uVar5 = param_3 /*0x3a*/; FUN_141ad3740(ctx,uVar5,param_4);` ; filter `+100` is copied from the lens param (`0x141bdc5d0`: `param_1[100] = param_2[0x55]`), editor row `PreTempAA` at param offset `0xad` (`0x141c3d820`) | OBSERVED; value shipped in GX params NOT FOUND |
| Executor consumes the sorted list ascending | not read (the dispatcher that walks the queue was not decompiled) | INFERRED |
| Marker list of the frame | `Z PrePass, Geometry, Water Simulation, Lighting (ScreenSpaceShadow, ViewSpaceShadow, SSAO, AOFields, SSR, EnvMapRegion, DirLight, DynLight), Composite, Forward, Effect, ScaleRender, Velocity, HDRPostProcess, PostProcess, UI, Debug, Etc` pointer table `0x143c264c8..0x143c26570` | OBSERVED (names); these are profiler labels, their code was not located |
| Order of calls in `0x141c7ce60` (not the execution order) | AA params (`0x141ad8f90`), glow params (`0x141b8ad70`), motion blur (`0x141b47f00`), fur blur (`0x141b488e0`), DoF (`0x141bdc5d0`, `0x141bda380` x2, `0x141bd7460`), tone map (`0x141b8ea80`), TempAA, glow (`0x141b89640`), outline, ctx 0x39, spatial AA, bokeh | OBSERVED |

## 4. Shader programme names by family (Q4)

Programme tables are arrays of 8-byte pointers to UTF-16 names (`ptrtab_s2.txt`); a loader copies them to filter fields through
`FUN_141a5e740` (pixel), `FUN_141a5e880` (vertex), `FUN_141a5e620` (compute). Full-screen helper programmes without extension live in
the post manager table `0x143c26580..0x143c26680` (below).

| Family | Table | Names | Passes / sizes / formats | Status |
|---|---|---|---|---|
| Anti-aliasing | `0x143c27c90` (11) | `FXAA.ppo FXAA3.ppo FXAA3HQ.ppo DLAA.ppo TempAA.ppo TempAA[StencilMask].ppo SharpenFilter.ppo AACopyStencil.ppo AAStencilPreprocess.ppo DilateMotionVector.ppo StencilAndMotionVectorPreprocess.ppo` + `FXAA.vpo TempAA.vpo` | TempAA history format code 0x1a (see TAA study) | OBSERVED |
| Tone map + bloom/glare + LUT | `0x143c29768` (3 vpo, 33 ppo, 4 cpo) | vpo `ToneMapVS ToneMapAdaptVS ToneMapDynamicResolutionVS`; ppo `ToneMap_DownSample2x2PS WriteFilmicToneMappingTablePS WriteSensitometricToneMappingTablePS WriteReinhardToneMappingTablePS WriteReinhardExToneMappingTablePS ExpToneMapPS DivideToneMapPS ReinhardToneMapPS RGBMEncodePS NonePS GammaPS FilmicToneMapPS TabledToneMapLUTPS TabledToneMapPS ColorToLuminancePS ScaleLuminancePS AdaptedLuminanceCopyPS AdaptedLuminancePS ResetAdaptedLuminancePS ResetSmoothAdaptedLuminancePS BrightPassAndScalePS BloomGaussBlurPS BrightPassPS BrightPassTemporalPS BloomCompositeUpScalePS BloomCompositeSinglePS VisualizeLUTPS TabledToneMapSeparateBeginPS TabledToneMapSeparateEndPS TabledToneMapLUTSeparateEndPS LUTFeedBackSeparateEndPS PostOETFPS PostOETFFeedBackPS`; cpo `ToneMap_Copy2Dto3DLUTCS Blend2Dto3DLUTCS Blend2D3Dto3DLUTCS Blend2D2DandBlend3Dto3DLUTCS` | the bloom chain = BrightPass(+Temporal, +AndScale) -> BloomGaussBlur -> BloomCompositeUpScale/Single, levels `GlareNumLevels` 1..8 (default 7 in `GXBloomFilterParam` ctor `0x141b974d0`: `+0xc = 7`); exec sub-passes in order `0x141b96260` (DownSample2x2), `0x141b964d0`, `0x141b90800`, `0x141b92120`, `0x141b916a0`, `0x141b91540`, `0x141b91120`, light-shaft composite `0x141c807f0`, `0x141b908f0`, final `0x141b93d00`, `0x141b96630` (pass-by-pass shader mapping not resolved: slots are selected by computed index) | names OBSERVED; role of each sub-pass INFERRED |
| Depth of field | `0x143c29a00` (21 ppo) | `LensSimu2_`: `DownSample2x2RGBMF DownSample2x2Detail DownSample2x2DetailUseCoC Copy ScaledTemporal ComputeCoCF ComputeCoC ComputeCoCDetail BlurCoC ScaleCoCPS ScaleCoCDetailPS MaskByCoC MaskCoC MakeBlurBuffer MakeBlurBuffer19 MakeBlurBufferFix16 FinalComposite FinalCompositeFullReso FinalCompositeFar FinalCompositeFarFullReso Debug` | textures made in `0x141bd9b10`: `LensSim FullScreenTexture` full res code 0x1a; `2x2Texture[0..3]` at w/2 code 10; `4x4Texture[0..3]` at w/4 code 10; `CoCTexture[0..1]` at w/D code 0x18 (D = `_DAT_143c298c0` = 2); `2x2CoCTexture[0..1]` at w/(2D) code 0x18; DoF resolution divisor global `0x143c298c0` = 2 (exec has branches for 2 and 4) | OBSERVED; code to DXGI (10 = RGBA16F, 0x1a = R11G11B10F, 0x18 = R10G10B10A2) INFERRED |
| DoF slot use seen in exec | | `Copy` (`0x141bd7280`, slot 0x178), `ScaledTemporal` (`0x141bdbf10`, slot 0x180), `BlurCoC` (`0x141bdaf30`, slot 0x1a0, repeated per `CoCBlurCount`), `MaskByCoC` (`0x141bdb7a0`), `MaskCoC` (`0x141bdbac0`); generic downsample/CoC/blur-buffer/composite passes go through `0x141bda3b0`, `0x141bdb1f0`, `0x141bdb4c0`, `0x141bda870` with a shader index argument | OBSERVED slots; rest INFERRED |
| Motion blur | `0x143c29530` (21 ppo, filter `+0x30..+0xd0`) | `MotionBlur.ppo MotionBlurWriteCMBVelocity.ppo MotionBlurWriteCMBVelocityNoMove.ppo MotionBlurDilateVelocity.ppo MotionBlur_WriteVelocityAndDepthPackPS MotionBlur_WriteVelocityAndDepthPack_DownScalePS MotionBlur_DownSampleVelocityPS MotionBlur_DownSampleVelocity_KeepMaxLengthPS MotionBlur_DilateVelocityPS MotionBlur_DilateVelocityAdaptiveXPS MotionBlur_DilateVelocityAdaptiveYPS MotionBlur_DilateVelocityAdaptiveX_KeepMaxLengthPS MotionBlur_DilateVelocityAdaptiveY_KeepmaxLengthPS MotionBlur_WriteMotionBlurPS[Simple] [Simple][Bidir] [FrontMask] [FrontMask][Bidir] [FrontBackMask] [FrontBackMask][Bidir] [Reconstruction] [Reconstruction][Bidir]` | velocity targets made in `0x141b4a1c0`: two at (w,h) with a caller format, one at `param_2[3..4]` code 0x18, two more halved each; job kind 1 (`0x141b4a800`) builds the camera velocity from current/previous matrices (`FUN_1401490b0` inverse, `FUN_140147990` multiply), kind 2 (`0x141b4ad40`) writes object velocity + depth pack, kind 0/3 (`0x141b4b140`) dilates and blurs | OBSERVED |
| Glow | post manager names | `EmissiveToGlowPS GlowCompositeSinglePS Sample13WeightforGlow Sample13WeightforGlowKernel` | exec `0x141b89c40`: down chain of `NumLevels` targets (filter `+0x38..`, default 6 in ctor `0x141b89450`: `+0x1c = 6`, smoothness default 10.0 at `+0x18`), then up chain from the smallest level back, composite to the colour target | OBSERVED structure; shader per sub-pass INFERRED |
| Light shaft | `0x143c29f70` (2 vpo + 10 ppo, loader `0x141c80fc0`) | `ToneMapVS.vpo ToneMapDynamicResolutionVS.vpo LightShaft_DetectSkyPS DetectSkyDownSamplePS DetectSky_MaskOnlyPS DetectSkyDownSample_MaskOnlyPS DetectSkyUseMaskPS DetectSkyDownSampleUseMaskPS SkyMaskBlurPS SkyMaskBlurParallelPS SkyMaskBlurParallelBlendPS CompositePS` | mask/blur at `(0x28,7)` through `0x141c81a10` (slot `+0x30+8*i`), composite `LightShaft_CompositePS` (slot `+0x78`) inside the tone map exec (`0x141c807f0`) | OBSERVED slots |
| Outline | `0x143c29fd0` | `Outline_StencilToColorPS StencilToColor2x2PS Scale2x2PS CopyPS GaussBlurPS TempAAResolvePS FinalPS ShrinkFinalPS BoxDilationFinalPS BoxShrinkFinalPS` | | OBSERVED names |
| Scene bokeh | `0x143c29ab8` (5) | `SceneBokeh_SimpleBlurPS SimpleBlurFadePS SimpleBlurCBPS SimpleBlurCBFadePS DownSamplePS` | 6-level mip chain (filter `+0x50..+0x78`), each level half the previous, code 0x18 (`0x141bdc970`) | OBSERVED |
| Feedback blur | `0x143c287e0` (3) | `FeedBackBlurMaskMargePS FeedBackBlurMaskPS FeedBackBlurFallOffPS` | | OBSERVED names |
| Depth scale / SSAO (out of scope) | `0x143c296c0` | `DepthScale2x2 _Max _Direct _DirectMS _ConvertLinear DepthConverLinear ScreenSpaceAO2*...` | | OBSERVED names |
| Post manager built-ins (no extension, table `0x143c26580..0x143c26680`) | | `PointSampleColor1 PointSampleColorClamp1 PointSampleColorCollect PointSampleColorInterpolate ColorGrading SampleColor1 SampleTwoColors1 SampleColor4 Sample13Weight Sample13WeightRotation Sample13WeightforGlow Sample13WeightforGlowKernel SampleBilateralBlur SampleBilateralDepthBlur(+Two,+Two1..3) OutputOne OutputColor EmissiveToGlowPS GlowCompositeSinglePS SampleColorSharpness SampleColorOrverlay DepthResolve DepthResolveMS ScreenSpaceQuadShader(+Z,+Scale) LutCopy2DTo3DCs LutBlend2DTo3DCs LutCopy3DTo2DCs CustomDofLutCopy`; compute `DownSampleDepth16CS CopyDepthCS DownSampleDepthCS` (`0x142f17d80`) | | OBSERVED names |
| Colour grading | | `ColorGrading` (post manager), `ColorGradingLut_` (`0x142f10368`), LUT 2D<->3D compute (above), `TabledToneMapLUT*` | | OBSERVED names |
| Not found | | any film grain, DLSS/FSR/XeSS, lens-flare post filter, post-process distortion filter | | NOT FOUND |

HDR output blend programmes (UI): `_BlendUI_SDR2HDR_F`, `_BlendUI_HDR2SDR_F`, `_BlendUI_UPSAMPLE_SDR2HDR_F`, `_BlendUI_UPSAMPLE_HDR2SDR_F`
(`0x142f250d0..0x142f251e0`), `Draw.EnableFakeHdrMode` (`0x142bef8f0`, referenced from `0x1459d1ec9`): noted for the tone map / HDR worker.

## 5. Editor parameter lists (Q5)

Source: editor-definition functions (`exp3/*.c`), parsed by `edparse2.py` into `editor_params2.txt`. The struct offsets are the
offsets inside the editor-side parameter block (`param_1+off`); the runtime param class has other offsets. Range = [min, max, step, x];
integers show `n(f=...)` raw. Defaults are NOT in the editor functions; the shipped values come from GX param files (not in the exe):
NOT FOUND. Code defaults of the runtime param classes are given where read.

| Group | Editor function | Parameters (offset: name [range]) | Status |
|---|---|---|---|
| Lens simulation (DoF) | `0x141c3d820` | `+0x58` Enable; `+0x5c` FarStart [0,10000,1]; `+0x60` FarEnd [0,10000,1]; `+0x64` NearStart [0,1000]; `+0x68` NearEnd [0,1000]; `+0x6c` FocusDist [0.1,2000]; `+0xab` UseCameraFocus; `+0x74` SkyMaskRate [0,1]; `+0x78` Lens F [0.1,100]; `+0x7c` Av [1,32]; `+0x88` FilmSize(mm) [0.01,0.24,0.001]; `+0x8c` LinearCoCFactorNear [0,1]; `+0x90` LinearCoCFactorFar [0,1]; `+0x84` DiaphragmRotate [-pi,pi]; `+0x94` CoCBlurCountPreMask [0,5]; `+0x98` CoCBlurCount(1) [0,5]; `+0x9c` CoCBlurCount(2) [0,5]; `+0xa0` MaxBlurLevel [0,2]; `+0xa8` NearCoCMode Linear; `+0xa9` FarCoCMode Linear; `+0xaa` HiResolutionBlur; `+0xac` NearFarSeparatedBlur; `+0xad` **PreTempAA**; `+0xb0` TemporalFeedbackWeight [0,0.99]; `+0xb4` AffectedByTransparentDepth. Also strings `ApertureShape` | OBSERVED |
| Bloom / glare | `0x141ba6240` | `+0x7c` Enable; `+0x80` GlareNumLevels [1,8]; `+0x84` Glare Luminance [0,10]; `+0x88` Glare Threshold [0,10]; `+0x8c` GlareBloom Alpha [0,1]; `+0x90` GaussianRadiusScale [0.01,2]; `+0x94` Saturate Ctrl [0,1]; `+0x98` Smooth [0,1]; `+0x9c` FeedbackWeight [0,0.99]. Runtime param ctor `0x141b974d0`: levels 7, 1.0, 0.5, 1.0, 1.0, 0, 0.8, 0.8 (field to name map INFERRED) | OBSERVED ranges |
| Glow filter ("CustomBloom") | `0x141c3ae40` | `+0x58` Enable; `+0x5c` Glow NumLevels [1,8]; `+0x60` Blur Smoothness [0.01,2]; `+0x64` Glow FinalColorScale [0,100]; deprecated: `+0x68` BlurSampleScale [0,5]; `+0x6c` Blur RgbScale [0,2]; `+0x70` Blur AlphaScale [0,3] | OBSERVED |
| Motion blur | `0x141bac270` | `+0x90` Enable; `+0x94` Motion Blur Intensity [0,1]; `+0x98` Blur Max Length [0.01,0.1,0.001] (fraction of the screen); `+0x9c` Shutter Speed [10,200]; `+0xa0` Sample Count [2,32]; `+0xa4` Blur RecurrenceCount [1,4]; `+0xa8` Bidirectional Blur; `+0xac` Blur Cutoff threshold in Pixel [0.1,5]; time group `+0xb0..0xb2` Use Inner Simulation Time / Enable Average / Enable Fixed. Runtime param ctor `0x141b98780`: `+0x10 = 0.05`, `+0x14 = 0.1`, `+0x18 = 4`, `+0x1c = 1`, ... (mapping INFERRED) | OBSERVED ranges |
| Fur blur | same function, second block | `+0x90` Enable; `+0x94` Fur Blur Intensity [0,1]; `+0x98` Blur Max Length [0.01,0.1]; `+0xa0` Sample Count [2,32]; `+0xa4` Blur RecurrenceCount [1,4] | OBSERVED |
| Light shaft | `0x141ba8fc0` | `+0xe0` Enable; `+0xf0` Angle; `+0x100` EnableRefLight; `+0x108` LightBankId; `+0x10c` LightId [-1..]; `+0x120` HighQualitySkyMask; (SkyMaskMode); `+0x128` MaskDepthThreshold [1,10000]; `+0x12c` SkyMaskSizeAngle [0.0175,1.5708 rad]; `+0x130` SkyMaskCenterPower [0.1,8]; `+0x140` Sky Multiply Color; `+0x150` Sky Add Color; `+0x160` SkyColorSaturate [0,1]; `+0x164` SkyColorThreshold [0,10]; `+0x168` ExposureInfluenceRate AddColor [0,1]; `+0x16c/+0x170` Parallel mode begin/end ang [0,1.5533 rad]; `+0x174/+0x178` Fade out begin/end ang [0,pi]; `+0x17c` ShaftBlurRange [1.01,10]; `+0x180` ShaftBlurRange for Parallel [0.01,1]; `+0x184` ShaftBlurFallOff [0,10]; `+0x188` ShaftBlurCount [1,4] | OBSERVED |
| Feedback blur | `0x141baf1a0` | `+0x58` Enable; `+0x5c` FeedbackWeight [0,0.999]; `+0x64` Feedback UnitTimeInSeconds [0,1]; object group `+0x68` FeedbackRate [0,0.999]; `+0x6c` FeedbackBlurFallOff [0,1]; `+0x70` FeedbackBlurPower | OBSERVED |
| Colour grading | `0x141bb1af0` | `+0x58` Enable; `+0x5c` LutSourceId [-1,10000]; `+0x60` FadeTime [0,10] | OBSERVED |
| Tone map group (addresses only, another worker's topic) | `0x141ba2af0` | ToneMapMode, `+0x100` Exposure [-8,8]; `+0x108` FilmicContrast [0,1]; `+0x10c` ReinhardContrast [1,10]; `+0x110` ReinhardWhiteBalance [0.1,10]; auto exposure `+0x114..0x130` (LightAdapt/DarkAdapt speed [0.1,10], Middle Gray [-8,8], Center Weight [0,10], Adaption Max [0.01,16], Min [-16,-0.01], FreezeAdaptionFade) | OBSERVED |
| Chromatic aberration | `0x141ba2af0` | `+0x134` Enable; `+0x138` ToneLateralDispersion[0] [-1,1]; `+0x13c` ToneLateralDispersion[1] [-1,1,0.001] | OBSERVED |
| Vignetting | `0x141ba2af0` | `+0x140` Power [0,10]; `+0x144` Range [0.01,2]; `+0x148` Shape [-1,1]; `+0x14c` Fov Dependence [0,1] | OBSERVED |
| Detail info (grade) | `0x141ba2af0` | `+0x150` ColorH [0,360]; `+0x154` ColorS [0,2]; `+0x160` ColorShift; `+0x170` ColorScale; `+0x180` ColorV [0,2]; `+0x184` Contrast [0,2] | OBSERVED |
| Setting info | `0x141b9f9a0` | BG Color `+0x150`; light-collect (not post) | OBSERVED |
| Quality options | `0x140b1c7b0` | config keys `Gconfig_DOFQuality` (`0x142b686a8`, row `param_1+0x58`), `Gconfig_MotionBlurQuality` (`0x142b686d0`, `param_1+0x94`), `Antialiasing` (TAA study); the mapping of these options to filter fields was not read | OBSERVED keys; mapping NOT FOUND |
| Per-area banks | strings `beginToneMapBank/Id endToneMapBank...`, same for LensSimulation, Bloom, ColorGrading, LightShaft, FeedbackBlur, MotionBlur, Glow, FurBlur (`0x142b50eb0..0x142b517a8`) | the param blocks are animated between a begin and an end bank id per area | OBSERVED names; use INFERRED |
| Anti-alias | `0x141c37880` | see ER-TAA-STUDY section 1 | OBSERVED |

## 6. Depth of field and motion blur (Q6, all INFERRED from names, parameters and exec structure)

**Depth of field (`GXLensSimulationFilter2`)**: a CoC-driven, physically parameterised, **half-resolution gather** DoF.
Evidence: CoC from thin-lens inputs (`Lens F`, `Av`, `FilmSize(mm)`, `FocusDist`) or from artistic ranges (`NearStart/NearEnd/FarStart/FarEnd`,
`LinearCoCFactorNear/Far`), `ComputeCoC / ComputeCoCDetail / ComputeCoCF`; CoC pre-blur loops (`BlurCoC` repeated `CoCBlurCountPreMask`,
`CoCBlurCount(1)`, `CoCBlurCount(2)`); `MaskByCoC / MaskCoC` and `NearFarSeparatedBlur` give separate near/far layers
(`FinalCompositeFar*`); blur kernel buffers `MakeBlurBuffer` (default), `MakeBlurBuffer19` (19 taps) and `MakeBlurBufferFix16`
(16 taps), aperture control `ApertureShape`, `DiaphragmRotate` (polygonal bokeh), `MaxBlurLevel` 0..2; scene down-sampled with CoC
(`DownSample2x2RGBMF`, `Detail`, `DetailUseCoC`) into a half-resolution (`_DAT_143c298c0` = 2, also a quarter-resolution branch) buffer set (RGBA16F-class),
CoC in R10G10B10A2; temporal accumulation `ScaledTemporal` (`TemporalFeedbackWeight`, `PreTempAA`); final composite at half or full
resolution (`FinalComposite`, `FinalCompositeFullReso`, far variants). No scatter-sprite programme exists and no tile-max for DoF.
Class: gather with variable-radius fixed kernels over layered near/far buffers, not scatter. DoF is skipped when `Enable` is off
(`0x141bd7460`: `if (*(char*)(param_1+8) != 0)`). Also a `CustomDofLutCopy` programme and a `DofFocus` debug string (`0x1432851b0`).

**Motion blur (`GXMotionBlurFilter`)**: velocity-buffer blur with **both camera and per-object velocity**, McGuire/Jimenez-style
tile-max and neighbour-max, then a gather. Evidence: `WriteCMBVelocity` (camera motion blur velocity from the current and previous
view/projection matrices, job kind 1 `0x141b4a800`) and `...NoMove` (static case), `WriteVelocityAndDepthPack` (+`_DownScale`) for
object velocity packed with depth (job kind 2 `0x141b4ad40`); `DownSampleVelocity` / `_KeepMaxLength` = tile max;
`DilateVelocity`, `DilateVelocityAdaptiveX/Y` (+`_KeepMaxLength`) = separable neighbour max (X then Y); final
`WriteMotionBlurPS` in variants `Simple`, `FrontMask`, `FrontBackMask`, `Reconstruction`, each with `Bidir`
(forward + backward blur); parameters `Sample Count` 2..32 (exec clamps to even 2..32), `Blur RecurrenceCount` 1..4 (the blur repeated),
`Blur Max Length` 0.01..0.1 of the screen, `Shutter Speed`, `Blur Cutoff threshold in Pixel`, inner (simulation) time options. A second
instance with the same class is the **fur blur** (objects with fur only). The velocity buffer for the main view is the `V+0x7b0`
target (INFERRED, also read by TempAA).

## What was not reached (NOT FOUND list)

1. The executor that walks the sorted command list (ascending order is INFERRED from the comparator only).
2. The resource wiring between stages (which texture is HDR colour, tone map output, outline/bloom targets): V+0x2b0/0x2c8/0x2d8/0x248
   are handles, not named.
3. Pass-by-pass shader mapping inside `GXToneMapFilter` (bloom chain order, LUT use, feedback end passes): the slots are indexed by
   computed values; only the programme list and the sub-function order are given. Tone map internals are another worker's topic.
4. The body of `GXOutlineFilter` Execute (`0x141bd6e30`, no Ghidra function; needs a created function) and the Glow filter's per-pass
   shader names (they live in the post manager table; call sites `FUN_141a5dea0/141a5d9e0` take names by index).
5. The shipped values of every parameter (GX param files), including `PreTempAA`, `NearFarSeparatedBlur`, bloom levels, MB samples.
6. Mapping of `Gconfig_DOFQuality`, `Gconfig_MotionBlurQuality`, `Antialiasing` options to filter fields.
7. Film grain, DLSS/FSR/XeSS, a post-process lens flare, a post-process heat-haze filter: no strings, no class (lens flare, radial blur and
   distortion are effect-system appearances).
8. The class at V+0xbf18 (`FUN_141bd4c60`, alloc 0x160) and `FUN_141b99390` (V+0xbda0): not identified.
9. Runtime behaviour: nothing launched.
10. Format-code to DXGI mapping (code 10, 0x18, 0x1a) is INFERRED from DXGI numbering, never confirmed by a descriptor creation call.

## Scripts and outputs (all in this folder)

- `strscan.py` (`py -E -s -X utf8 strscan.py needles.txt out`): raw string scan of the exe; needle files `n1.txt..n4.txt`, outputs `s1.txt..s4.txt`.
- `rttilist.py` (`py -E -s rttilist.py GXBS` ...): RTTI -> vtable lists, `rtti_gxbs.txt`, `rtti_more*.txt`.
- `ptrtab.py s2.txt` -> `ptrtab_s2.txt` (programme name tables); `ptrdump.py 143c26480 143c26700` (marker and post-manager name table); `rngstr.py` (range strings).
- `riprefs2.py <exe> <va>...` (78 s each): `rip_vt.txt` (vtable and table references, constructors/loaders), `rip_str.txt` (editor functions, config keys).
- `run.ps1` + `ER897Export.java` (decompile + asm per function) -> `exp1..exp8` (arguments are the entries listed in each `export.txt`);
  `run.ps1` + `ErXrefs.java to ...` -> `xrefs_a.txt`, `xrefs_b.txt`.
- `edparse2.py <exp3/*.c>` -> `editor_params2.txt` (editor rows); `tmslots.py`, `pev.py`, `qrefs.py`, `vtname.py` helpers.
- Reused from I-0132: `ER-TAA-STUDY.md` and its `exp3/141a19c80.c` (view constructor), `exp4/141c7ce60.c` and `.asm.txt` (driver).
