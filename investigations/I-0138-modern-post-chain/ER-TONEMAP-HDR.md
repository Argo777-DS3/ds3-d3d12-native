# Elden Ring 2.7.1 tone mapping, exposure, colour grading and HDR output: static study

Copied from `evidence/I-0138/artifacts/er-tonemap-hdr/ER-TONEMAP-HDR.md` (2026-11-12, a Sonnet `ghidra-research` worker for the workstream 11 pilot; its scripts
and raw outputs stay there). Its statuses are the worker's; the pilot's reading is in [STUDY](STUDY.md) §5.

Binary `eldenring_2.7.1.0_25080141_1A354710.exe`, base `0x140000000`, Ghidra project `ER_2_7_1_25080141_I0001`
(headless 12.1.3, `-process -readOnly -noanalysis`, mirror `er-mirror-i0072-b`, JVM Idle priority, one run at a time).
Nothing launched, nothing written outside this folder. All addresses are ER virtual addresses. Function names are mine (`FUN_<addr>`).
Statuses: OBSERVED = read in the decompile / bytes; INFERRED = reasoned from observed code or names; NOT FOUND.
ER's shader bytecode is not in the exe, so every statement about what a `.ppo` computes is INFERRED from its name at best.

## 0. The one-paragraph answer

ER's tone mapping is the **same YEBIS model as DS3**, extended, not a new model. The parameter set (editor `0x141ba2af0`, 26 rows, runtime struct defaults
`0x141b967c0`) is Exposure (EV, -8..8), ToneMapMode (Filmic 0, Sensitometric 7 = default, ReinhardEx 8, Reinhard 3, RGBM 4, Copy 5, Gamma 6),
FilmicContrast, ReinhardContrast, ReinhardWhiteBalance, a full AutoExposure group, Chromatic Aberration, Vignetting (Power/Range/Shape/FovDependence),
ColorH/S/V + ColorShift/ColorScale/Contrast (feeds the same 4x4 colour matrix, `0x141b8e630`). The curve is **not computed on the CPU**: the CPU only
sends raw contrast values and a 2^EV exposure scale; for Filmic / Sensitometric / Reinhard / ReinhardEx a GPU pass bakes the curve into a 1D table
texture each frame (`ToneMap_Write*ToneMappingTablePS`), and `ToneMap_TabledToneMap[LUT]PS` samples it; a 3D colour-grading LUT (16x16x16
R8G8B8A8, built each frame by compute shaders from one or two 256x16 2D strips `ColorGradingLut_<id>`, with a fade/blend) is applied in the same pass, then a
`ToneMap_PostOETFPS` pass encodes for output. Auto exposure is live: GPU luminance chain -> 1x1 R32G32F adapted-luminance target -> **read back to the CPU
(ring of 4 staging buffers, ~4 frames latency)** -> CPU exposure feed. **ER has no modern HDR10 path on PC**: no `IDXGISwapChain1/2/3/4`, `IDXGIOutput6` IID anywhere in the
exe, no `SetColorSpace1`/`SetHDRMetaData`, no HDR key in the graphics config; the only HDR code is a legacy vendor-API path (NVAPI HDR colour control and
AMD AGS `agsSetDisplayMode`) that switches the display to HDR in exclusive fullscreen and makes the back buffer R10G10B10A2 (DXGI 24). Where PQ/Rec.2020
would be applied (PostOETF, `_BlendUI_*_SDR2HDR` UI shaders) lives in shaders I cannot read.

## 1. Tone map parameters (editor `0x141ba2af0`, defaults `0x141b967c0`)

Editor function registers group "Tone Map" (`0x14303cb60`, editor "Tone Map[Yebis]" `0x14303e498`) and one row per parameter. Offsets are in the editor's
param object (base +0x100); the same 26 values are loaded from the GX param files by `FUN_141b97040` (26 entries, source stride 0x30, destination base +0x10, so editor offset - 0xF0).
Range rows are [min, max, step, 1] read from the data blocks named in the call. Defaults are from the runtime struct constructor `0x141b967c0` and reset `0x141b96980`
(code defaults only; the shipped per-area values are in param files: NOT FOUND statically).

| Param (string addr) | Editor off | Struct off (ctor) | Range (data block) | Code default | Status |
|---|---|---|---|---|---|
| Exposure `0x14303c460` | +0x100 | +0x10 | -8..8 step .01 (`0x14303eef0`) | 0.0 (EV) | OBSERVED range; default OBSERVED, mapping INFERRED |
| ToneMapMode `0x14303c508` | +0x104 | +0x14 | enum, below | 7 (Sensitometric) | OBSERVED |
| FilmicContrast `0x14303c520` | +0x108 | +0x18 | 0..1 step .01 (`0x142f15590`) | 0.5 | OBSERVED |
| ReinhardContrast `0x14303c540` | +0x10c | +0x1c | 1..10 (`0x14303ef20`) | 1.6 | OBSERVED |
| ReinhardWhiteBalance `0x14303c568` | +0x110 | +0x20 | 0.1..10 (`0x14303ef10`) | 1.7 | OBSERVED |
| Enable AutoExposure `0x14303c5b8` | +0x114 | +0x24 | bool | 0 (off) | OBSERVED |
| AutoExposure LightAdapt Speed `0x14303c5e0` | +0x118 | +0x28 | 0.1..10 step .1 (`0x14303ef60`) | 1.0 | OBSERVED |
| AutoExposure DarkAdapt Speed `0x14303c620` | +0x11c | +0x2c | 0.1..10 | 0.5 | OBSERVED |
| AutoExposure Middle Gray `0x14303c660` | +0x120 | +0x30 | -8..8 (EV, `0x14303eef0`) | -2.0 | OBSERVED |
| AutoExposure Center Weight `0x14303c698` | +0x124 | +0x34 | 0..10 step .1 (`0x14303ef50`) | 4.0 | OBSERVED |
| AutoExposure Adaption Max `0x14303c6d0` | +0x128 | +0x38 | 0.01..16 (`0x14303ef30`) | 8.0 | OBSERVED |
| AutoExposure Adaption Min `0x14303c708` | +0x12c | +0x3c | -16..-0.01 (`0x14303ef40`) | -8.0 | OBSERVED |
| FreezeAdaptionFade `0x14303c740` | +0x130 | +0x40 | byte bool | 0 | OBSERVED |
| Enable Chromatic Aberration `0x14303c798` | +0x134 | +0x44 | bool | 0 | OBSERVED |
| ToneLateralDispersion[0], [1] `0x14303c7d0/c808` | +0x138, +0x13c | +0x48, +0x4c | -1..1 step .001 (`0x142f1b1f0`) | 0, 0 | OBSERVED |
| Vignetting Power `0x14303c858` | +0x140 | +0x50 | 0..10 (`0x14303ef00`) | 0 | OBSERVED |
| Vignetting Range `0x14303c868` | +0x144 | +0x54 | 0.01..2 (`0x14303eee0`) | 0.4 | OBSERVED |
| Vignetting Shape `0x14303c878` | +0x148 | +0x58 | -1..1 (`0x14303eec0`) | 0 | OBSERVED |
| Vignetting Fov Dependence `0x14303c888` | +0x14c | +0x5c | 0..1 (`0x142f15590`) | 0 | OBSERVED |
| ColorH `0x14303c8c0` | +0x150 | +0x60 | 0..360 step .1 (`0x14303ef70`) | 0 | OBSERVED |
| ColorS `0x14303c8d0` | +0x154 | +0x64 | 0..2 (`0x14303eed0`) | 1.0 | OBSERVED |
| ColorShift `0x14303c8e0` (vec4) | +0x160 | +0x70 | vec4 (default block `0x1432a1c70`) | (1,1,1,0) | OBSERVED |
| ColorScale `0x14303c8f8` (vec4) | +0x170 | +0x80 | vec4 | (1,1,1,0) | OBSERVED |
| ColorV `0x14303c910` | +0x180 | +0x90 | 0..2 | 1.0 | OBSERVED |
| Contrast `0x142f10ac0` | +0x184 | +0x94 | 0..2 | 1.0 | OBSERVED |
| (unnamed in the editor) | - | +0xa0 | - | 10.0 | OBSERVED value, name UNKNOWN |

ToneMapMode enum (editor rows, `0x141ba2af0`): `Filmic`=0 (`0x14303c478`), `Sensitometric`=7 (`0x14303c488`), `ReinhardEx`=8, `Reinhard`=3, `RGBM`=4, `Copy`=5, `Gamma`=6.
Modes 1 and 2 are not in the editor enum but are handled by the draw code (`0x141b93d00`): they select `ToneMap_ExpToneMapPS` and `ToneMap_DivideToneMapPS` (OBSERVED in the switch, names INFERRED from the slot order). Excerpt of the enum build: `puStack_a8 = &UNK_14303c478; ... uStack_90 = 7; ... &UNK_14303c4a8; uStack_80 = 8; ... uStack_70 = 3; ... uStack_60 = 4; ... uStack_50 = 5; ... uStack_40 = 6`.

Other related param groups found (names only, not detailed): `Glare Luminance`, `Glare Threshold`, `GlareBloom Alpha/GaussianRadiusScale/Saturate Ctrl/Smooth/FeedbackWeight` (`0x14303c940..ca48`, editor `0x141ba6424`), `FeedBackBlur` (`0x14303c920`), `Yebis ColorGrading` group (Q5), and `ExposureInfluenceRate AddColor` (`0x14303ce18`, effects' exposure response).

Comparison with DS3 (list in the task): same names for Exposure, ToneType -> ToneMapMode (adds Reinhard, ReinhardEx, RGBM, Copy, Gamma as modes; Sensitometric = DS3's "sensitometric"), FilmicContrast, ColorH/S/V, AutoExposure, Vignette Power / Fov Dependence. New in ER: ReinhardContrast / ReinhardWhiteBalance, Middle Gray / Center Weight / Adaption Min-Max / FreezeAdaptionFade, Chromatic Aberration + ToneLateralDispersion, Vignette Range/Shape, ColorShift/ColorScale/Contrast. `Gamma` is a tone mode, not a separate scalar (no scalar "Gamma" row). Verdict: same model, richer.

## 2. Tone curve and constant setup

Per-frame setup: `FUN_141b92690` (tone-map Execute worker, called by Execute `0x141b90210` = slot 1 of `GXBS::GXToneMapFilter`, vtable `0x14303b958`, RTTI string `0x143d34168`). Draw/pass selection `FUN_141b93d00`, table-bake pass `FUN_141b92120`, bloom `FUN_141b91120`, LUT build `FUN_141b916a0`. Constant buffer sizes created by the loader `0x141b8f420`: 0x1a0 (main), 0x10, 0x100.

| Finding | Address | Evidence | Status |
|---|---|---|---|
| No filmic/ACES/Hable arithmetic on the CPU. The only `pow` calls are `pow(2.0, x)` (`_DAT_1432a1944` = 2.0): `fVar25 = 2^(-p[0x4d]) * (2^(p[0]) / p[1]) * fVar24`; the resulting scale is stored splat in CB +0x30 and its inverse in CB +0x00. FilmicContrast is copied raw (`uStack_168 = CONCAT44(fVar26,param_4[9])` into CB +0x20); contrast terms are copied raw. There is no `1 + 2.08914*c^(1/3)` precomputation as in DS3 | `0x141b92690` | `fVar25 = ... thunk_FUN_14252ac50(uVar34,param_4[0x4d] ^ uVar33) * (FUN_14252ac50(uVar34,*param_4)/param_4[1]) * fVar24; fStack_308 = 1.0/fVar25; fStack_2d8 = fVar25` | OBSERVED code. Which `param_4[i]` is which editor row INFERRED (the exec struct is a resolved copy filled by an update job I did not read) |
| Curve baked on the GPU into a table texture per frame. Mode -> table pass: Filmic(0) -> `ToneMap_WriteFilmicToneMappingTablePS`, Sensitometric(7) -> `...WriteSensitometric...`, Reinhard(3) -> `...WriteReinhard...`, ReinhardEx(8) -> `...WriteReinhardEx...`; other modes get no table | `0x141b92120` (slots `+0xcf8/+0xd00/+0xd08/+0xd10` = PS indices 1,2,3,4 of the table at `0x143c29780`, base offset 0xcf0) | `if (iVar1==0) lVar4=0xcf8; else if (iVar1==7) 0xd00; else if (iVar1==8) 0xd10; else if (iVar1==3) 0xd08; else return;` | OBSERVED code; slot->name mapping OBSERVED from the loader loop (`param_1 + (0x143c29780 - 0x143c28a90) + 8*i`) |
| Non-tabled direct shaders per mode: 1 -> `ExpToneMapPS` (slot 0xd18), 2 -> `DivideToneMapPS` (0xd20), 3 and 8 -> `ReinhardToneMapPS` (0xd28), 4 -> `RGBMEncodePS` (0xd30), 5 -> `NonePS` (0xd38), 6 -> `GammaPS` (0xd40), 0 and 7 -> `FilmicToneMapPS` (0xd48). Tabled path (flag `param_4+0xe8`): `TabledToneMapPS` (0xd58) or `TabledToneMapLUTPS` (0xd50, when a 3D LUT is bound), or `TabledToneMapLUTSeparateEndPS` (0xdc8, "separate" path) | `0x141b93d00` | `if (iVar1==0) ... lVar5 = 0xd48 ... if (iVar1==1) lVar5 = 0xd18 ... else if (iVar1==2) 0xd20 ... 6 -> 0xd40 ... if (bVar3) lVar5 = 0xdc8; else { lVar5 = 0xd58; if (param_5 != 0) lVar5 = 0xd50; }` | OBSERVED |
| Tone-map shader list (33 pixel + 3 vertex + 4 compute programmes, names at `0x14303b968..0x14303c3e0`, pointer tables `0x143c29768` (VS), `0x143c29780` (PS), `0x143c29888` (CS)): ToneMapVS/AdaptVS/DynamicResolutionVS; DownSample2x2, Write{Filmic,Sensitometric,Reinhard,ReinhardEx}ToneMappingTable, Exp/Divide/Reinhard/RGBMEncode/None/Gamma/FilmicToneMap, TabledToneMap{,LUT}, ColorToLuminance, ScaleLuminance, AdaptedLuminance{,Copy}, Reset{,Smooth}AdaptedLuminance, BrightPass{,AndScale,Temporal}, BloomGaussBlur, BloomComposite{UpScale,Single}, VisualizeLUT, TabledToneMap{SeparateBegin,SeparateEnd,LUTSeparateEnd}, LUTFeedBackSeparateEnd, **PostOETF**, **PostOETFFeedBack**; CS: Copy2Dto3DLUT, Blend2Dto3DLUT, Blend2D3Dto3DLUT, Blend2D2DandBlend3Dto3DLUT | `0x14303b968..` | strings + loader loop counts 3 / 0x21 / 4 in `0x141b8f420` | OBSERVED |
| Tone types the code switches on: 0,1,2,3,4,5,6,7,8 (9 values). The DS3-style "Sensitometric" is the default (7) | `0x141b92120`, `0x141b93d00`, ctor `0x141b967c0` | `*(undefined4*)((longlong)param_1+0x14) = 7` | OBSERVED |
| Colour matrix: hue (degrees*pi/180 via `_DAT_1432a18ec` = 0.0174533), saturation, value/colour shift & scale and contrast folded into a 4x4 by `FUN_141c411d0` then post-multiplied by two contrast/gain matrices (`param+0xa0`, `+0xa4`); when the LUT path is baked (`param_1+0xcc5`) the matrix is replaced by identity (`_DAT_1430b38e0`) and the matrix is baked into the 3D LUT instead | `0x141b8e630`, `0x141b92690` | `if (*(char*)(param_1+0xcc5)==0) pfVar13 = FUN_141b8e630(param_4,&fStack_148); else identity` | OBSERVED (same structure as DS3's 4x4 hue/sat/value/contrast matrix) |
| What the table texture contains, exact filmic/sensitometric formula, the white point of Reinhard | shader | - | NOT FOUND (needs the `ToneMap_Write*TablePS.ppo` bytecode) |

## 3. Auto exposure / eye adaptation

| Finding | Address | Evidence | Status |
|---|---|---|---|
| Live code. Parameters in section 1 (Enable, LightAdapt 0.1..10 def 1.0, DarkAdapt 0.1..10 def 0.5, Middle Gray -8..8 EV def -2.0, Center Weight 0..10 def 4.0, Adaption Max def 8, Min def -8, FreezeAdaptionFade). Default is **off** (`Enable AutoExposure` = 0 in the ctor) | `0x141ba2af0`, `0x141b967c0` | see section 1 | OBSERVED |
| GPU measurement: shader chain `ToneMap_ColorToLuminancePS` -> `ScaleLuminancePS` -> `DownSample2x2PS` ... -> `AdaptedLuminancePS` (temporal, uses LightAdapt/DarkAdapt), plus `AdaptedLuminanceCopyPS`, `ResetAdaptedLuminancePS`, `ResetSmoothAdaptedLuminancePS`. No histogram/compute shader exists (the four CS are LUT-only). Adapted-luminance targets: 2 ping-pong 1x1 textures of format 0x10 (= DXGI 16 R32G32_FLOAT) at `param_1+0x410 + i*0xa0`; the pass `FUN_141b94750` writes it using slot 0xd78 (`AdaptedLuminancePS`) or 0xd70 (`...Copy`) | `0x141b8f420` (creation loop `FUN_1419f2df0(lVar15,1,1,0x10)`), `0x141b94750` | `lVar3 = 0xd78; if (*(char*)(param_1+0xca1) != 0) lVar3 = 0xd70;` | OBSERVED structure; shader maths NOT FOUND (centre weighting by Center Weight is INFERRED from the name) |
| **CPU readback exists**: each frame the 1x1 adapted-luminance is copied (`FUN_1419e30d0`) into a ring of 4 staging buffers named `AdaptionReadback` (`0x14303c438`, objects at `param_1+0xad0 + i*0x70`, index `param_1+0xac0`, `& 3`); when the ring has wrapped, a deferred task (`0x141b901f0` -> `FUN_141b96590`) maps the buffer and stores **`*(float*)(param_1+0xc90) = mappedFloat[0] - 20.0`** (`_DAT_142a05fa0` = 20.0) | `0x141b96630`, `0x141b96590`, `0x141b8f420` | `uVar2 = *(int*)(param_1+0xac0)+1U & 3; ... *(float*)(param_1+0xc90) = *apfStack_18[0] - _DAT_142a05fa0;` | OBSERVED (latency ~4 frames INFERRED from the ring size) |
| CPU use of that value: tone-map setup reads `param_1+0xc90` (0 when `param_4+0x54` is set), soft-clamps it with `FUN_141b906a0` (positive: `(1 - exp(-v/lim))*lim`, negative: `(exp(v/lim2) - 1)*lim2`; limits `param_4[0x13]`, `[0x14]`, probably Adaption Max/Min: INFERRED) and folds `2^(-clamped)` into the exposure scale; gate `DAT_143c29760` (initial value 1). The exposure scale and the exposure `param_4[0x4d]` are also passed to `FUN_141c807f0` and `FUN_141b96630` (effects/other consumers; role INFERRED) | `0x141b92690`, `0x141b906a0` | `if (DAT_143c29760 != '\0') { uVar23 = FUN_141b906a0(uVar22,param_4[0x13],param_4[0x14]); fVar24 = pow(2, uVar23 ^ 0x80000000); }` | OBSERVED code, roles INFERRED |
| Debug switch "Enable - TonemapExposureAdjustment" is a global byte `DAT_143c2a024` (initial 1) read by ~16 effect/sprite code sites: effects are pre-compensated for exposure (see also param "ExposureInfluenceRate AddColor", string `AntiExposureRate`) | `0x141ca0dc0` (debug-menu entry `0x14305e410`), reads at `0x140d8f46c`, `0x141ca1048`... | rip scan of `0x143c2a024` | OBSERVED reads; meaning INFERRED |
| Dynamic resolution has its own tone-map vertex shader `ToneMapDynamicResolutionVS` | `0x14303b9b0` | string | OBSERVED |

## 4. HDR output

| Finding | Address | Evidence | Status |
|---|---|---|---|
| **No DXGI colour-space HDR path.** None of the IIDs `IDXGISwapChain1/2/3/4`, `IDXGIOutput5/6`, `IDXGIFactory4/5/6` exists in the exe; only `IDXGIFactory`, `IDXGIFactory1`, `IDXGIFactory2` (`0x1430a4aa8`), `IDXGIAdapter3`. Scan for indirect calls `[reg+0x130]` (SetColorSpace1), `[reg+0x128]` (CheckColorSpaceSupport), `[reg+0x140]` (SetHDRMetaData) with a colour-space immediate (12) within 48 bytes before: none (one false positive at `0x1402729d4`, a store, not a call). The string `DXGI_COLOR_SPACE...`/`SetColorSpace1` is absent | scripts `guid.py`, `vcall.py`, `vcall2.py` | `IDXGISwapChain3 []` ... `IDXGIFactory2 ['0x1430a4aa8']` | OBSERVED (absence of the IIDs); "never called" INFERRED from it |
| Swap chain creation is the legacy `IDXGIFactory::CreateSwapChain` (vtable +0x50) with a `DXGI_SWAP_CHAIN_DESC`: SwapEffect = 4 (FLIP_DISCARD), Windowed = 1, BufferCount from the descriptor, Flags = `(byte & 1) * 2` (ALLOW_MODE_SWITCH); then `MakeWindowAssociation(hwnd, 3)`; Present is `(+0x40)(chain, sync, 0)` | `0x141e9eac0` (create), `0x141e90f20` (flags), `0x141e9f630` (present) | `uStack_70 = 1; uStack_6c = 4; ... (**(code**)(*plStack_b8+0x50))(plStack_b8,uVar8,&uStack_a8,param_1)` | OBSERVED |
| Back-buffer format: the engine's format code equals the DXGI number (identity switch `0x141e90a20`, cases 1..0x63 return themselves). Setup chooses **0x1c (R8G8B8A8_UNORM) normally and 0x18 (R10G10B10A2_UNORM = DXGI 24) when the display reports HDR-capable**; on a resize with AMD (`DAT_1447f33f2`) or NVIDIA (`DAT_1447f33f3`) vendor layers up and the descriptor's HDR bit (flag & 4) set, the format is forced to 0x18 (`DAT_143c1b3d8/da` data bytes = 1 initially, never written: only compares) | `0x1419e9b60` (setup, `uVar7 = 0x1c; if (cVar5 != '\0') uVar7 = 0x18;`), `0x1419ef6e0` (resize) | excerpts | OBSERVED |
| HDR is enabled by **vendor APIs, only on exclusive fullscreen, not by a colour space**: function table `_DAT_144805388` (is HDR available for the format), `_DAT_144805390` (query: supported flag + two floats, AMD reads two doubles at `+0x1b0/+0x1b8` of AGS display info, i.e. min/max luminance: INFERRED), `_DAT_144805398` (set HDR on/off for output). Vendor selection `FUN_141eac340` / `FUN_141e88e40(device, 0, 0|1|2)`: 1 = NVIDIA (`FUN_141eafe50`, vendor id 0x10de, NVAPI), 2 = AMD (`FUN_141eb0400`, vendor id 0x1002, `amd_ags_x64.dll`: `agsInit`, `agsSetDisplayMode`, `agsDeInit` imported). NVIDIA set (`FUN_141eb0170`): NVAPI HDR colour-data struct, version word `0x10028`, command 1 (set), mode field **5** when enabling (NV_HDR_MODE_UHDA_PASSTHROUGH = 5 per NVAPI headers: INFERRED from memory, not read here), 0 when disabling. AMD set (`FUN_141eb06b0`): `agsSetDisplayMode(ctx, deviceIdx, displayIdx, &settings)` with `settings.mode = enable ? 2 : 0` (meaning of 2 depends on the AGS header: not verified; FreeSync2 PQ/scRGB INFERRED) | `0x141e88e40`, `0x141eac340`, `0x141eafe50`, `0x141eb0170`, `0x141eb06b0`, `0x141eb0400` | `auStack_b8[0] = -(uint)(param_3 != '\0') & 2; agsSetDisplayMode(_DAT_144805490,iVar6,iVar10,auStack_b8);` and `pppuStack_d8 = ... & 0xffffffff00000005` | OBSERVED code; NVAPI/AGS enum meanings INFERRED |
| Trigger: in Present and fullscreen-state update the vendor setter is called only when the swap chain's HDR flag (`+0x32 & 4`) is set and `GetFullscreenState` says exclusive fullscreen (`bVar9 = aiStackX_8[0] != 0`); on leaving it is switched off (`FUN_141e9ed90`) | `0x141e9f630`, `0x141e9fb50`, `0x141e9ed90` | `bVar9 = bVar9 & (char)param_1[6] == '\0'; ... cVar2 = (*_DAT_144805398)(uStack_68, lVar8, bVar9)` | OBSERVED. Who sets the descriptor's HDR bit (`param_2+0x78` byte copied to `param_1[0x3a]`) was not found |
| Option keys: the graphics-config key table (`0x142c01438..0x142c01820`: Resolution-*, ScreenMode, Auto-detectBestRenderingSettings, FrameRateLimit, QualitySetting, TextureQuality, Antialiasing, SSAO, DepthOfField, ShadowQuality, LightingQuality, EffectsQuality, ReflectionQuality, WaterSurfaceQuality, ShadeQuality, VolumetricEffectQuality, RaytracingQuality, GIDataQuality, GrassQuality) has **no HDR / MaxLuminance / PaperWhite / Saturation key**. A game option "Game.Option.Disp.Brightness" (`0x142bedc08`) is written by `FUN_140e4ef30` (a byte option clamped to 0..10, normaliser `0x14025c120`), plus menu names `HDRSetting` (`0x142b14b90`), `SDRSetting` (`0x142b14ba0`), `BrightnessSetting` (`0x142b14c28`), windows `02_046_BrightnessSetting`, `05_031_NewGame_BrightnessSetting` (`0x142ac1310/ac1250`), and debug var `Draw.EnableFakeHdrMode` (`0x142bef8f0`, registered in the second `.text`) | listed addresses | | OBSERVED names/keys; how Brightness reaches rendering: NOT FOUND. The `HDRSetting`/`SDRSetting` menu constructors (`0x1409542d0`, `0x1409540f0`) exist; whether the PC menu ever opens them: NOT FOUND |
| Tone map in HDR: no code difference found on the CPU. The tone-map CB has a pair `(X/10000, X/200)` (`_DAT_143c298a8` = 10000.0, `_DAT_143c298ac` = 200.0, only user `0x141b92e92`) from `param_4[0xd]`; I first read it as PQ normalisation but the parameter near it is the glare/bloom luminance group, so I do **not** claim it is HDR | `0x141b92690` | `fStack_148 = (float)param_4[0xd] / _DAT_143c298a8; fStack_144 = (float)param_4[0xd] / _DAT_143c298ac;` | OBSERVED arithmetic; role UNKNOWN |
| Final output pass: `ToneMap_PostOETFPS` (slot 0xde8) / `ToneMap_PostOETFFeedBackPS` (0xdf0) always drawn after the tone-map pass (selected by `param_7`), in the "separate" flow the tone-map result goes to an intermediate target then PostOETF writes the output. "OETF" suggests the display encoding (gamma/sRGB, and PQ in HDR) | `0x141b93d00` | `lVar6 = 0xde8; if (param_7 != '\0') lVar6 = 0xdf0;` | OBSERVED slots; what it encodes: INFERRED from the name, PQ/Rec.2020 maths NOT FOUND (shader) |
| UI vs HDR: UI is drawn to a texture ("GXGUIRenderToTexture" `0x142f10048`) in the render group `UI` (`0x143c26560`, after `HDRPostProcess` `0x143c26550` and `PostProcess`) and blended with built-in 2D shaders `_BlendUI_F`, `_BlendUI_SDR2HDR_F`, `_BlendUI_SDR2SDR_F`, `_BlendUI_HDR2SDR_F` and four `_BlendUI_UPSAMPLE_*` variants (`0x142f250b8..0x142f251e0`, table `0x143c27d80`, loader `0x141ade7d0`). So the engine can composite an SDR UI onto an HDR target; the brightness constant and the variant selection code were not found | `0x141ade7d0` | strings | OBSERVED names; selection logic and UI brightness NOT FOUND |
| Is any of this live in the PC build? The vendor layer is initialised at startup (`0x141eac3a7` calls `FUN_141eb0400`), the format selection runs, but no user-facing HDR option exists and I could not find the creator of the HDR request bit | | | UNKNOWN (static); do not assume PC ER outputs HDR |

## 5. Colour grading

Editor group "Yebis ColorGrading" (`0x141bb1af0`, strings `0x143041878`, `Yebis-ColorGrading-Enable`, `LutSourceId`, `ColorGrading FadeTime`) with three rows: **Enable** (bool, struct `+0x58`), **LutSourceId** (int, `+0x5c`, range -1..10000, `FUN_141a4c6c0` row `0xffffffff, 10000, 1`), **FadeTime** (float, `+0x60`, 0..10 step .01, `0x14303ef00`). Defaults not read.

| Finding | Address | Evidence | Status |
|---|---|---|---|
| 3D LUT target: 16x16x16 texture, format 0x1c (R8G8B8A8_UNORM), created in the tone-map loader at `param_1+0x5c0` (and a UAV copy at `+0x6e0`), pre-filled with an identity cube (`r = x/0xf, g = y/0xf, b = z/0xf`, alpha 0xff); loop bounds `0xff0` in steps of `0xff` | `0x141b8f420` | `*puVar22 = uVar13 / 0xf | (uVar23 / 0xf | (uVar12 / 0xf | 0xffffff00) << 8) << 8; ... FUN_141a047b0(param_1 + 0x5c0,0x10,0x10,0x10)` | OBSERVED |
| LUT source: 2D strips named `ColorGradingLut_...` (prefix `0x142f10368`, wide, 16 chars) validated as 16 x 256 (`FUN_141b94710`: one dimension == 0x10, the other == 0x100), i.e. a 256x16 strip of 16 slices (INFERRED orientation). Compute passes `ToneMap_Copy2Dto3DLUTCS` (one source), `Blend2Dto3DLUTCS` (two 2D sources, weight = a CB float from `param_3+0x15c`), `Blend2D3Dto3DLUTCS`, `Blend2D2DandBlend3Dto3DLUTCS` (two 2D + previous 3D) dispatch 1x1x16 groups; the blend weight is the fade between old and new grading (FadeTime) | `0x141b916a0` (sources at `param_3+0x378/0x380` and `+0x3a8/0x3b0`), `0x141b91970` (CS slot `+0xdf8`), `0x141b91b00` (`+0xe00`), `0x141b91ca0` (`+0xe10`), `0x141b94710` | `FUN_141acad20(lVar1,1,1,0x10)` | OBSERVED structure; fade-time semantics INFERRED |
| Order: the LUT texture is bound to slot 2 of the tone-map draw (`+0x880`; a default LUT from the context when none), together with slot 0 scene colour, slot 1 adapted luminance (`*(param_1+0x3d0)+0x48`), slot 3 bloom, slot 5 the baked tone table. A single pixel shader `TabledToneMapLUTPS` does exposure + table + LUT; so the LUT is part of the tone-map pass, applied after the curve (16^3 over [0,1] suggests display-referred input) and before `PostOETF`. The 3D LUT bake itself (`thunk_FUN_144dc31ad`) is in the protected second `.text` (`UNK_144e4fd3a`) | `0x141b93d00` | `*(undefined8*)(lVar5 + 0x880) = uStack_140;` (slot 2), `0x898` (slot 5) | OBSERVED slots; "after the curve" INFERRED (shader) |
| It runs in the same tone-map pass regardless of HDR; no code path skipping the LUT for HDR was found | | | INFERRED (absence), and HDR output is not enabled by a switch I found |

## 6. Dither / grain / vignette / chromatic aberration

| Effect | Where | Status |
|---|---|---|
| Chromatic aberration: tone-map params `Enable Chromatic Aberration`, `ToneLateralDispersion[0]/[1]` (-1..1, 0 default); the setup splits each value into negative and positive lateral offsets into CB +0xe0/+0xf0 (`uStack_228`, `uStack_218`) (`fVar26 = |x|` split by sign, times 0.5), enabled only if the view flag `param_1+0x1b` and the flag `param_4+0xa8` are set and the values are non-zero (`bStack_368`) | `0x141b92690`, editor `0x141ba2af0` | OBSERVED |
| Vignette: Power, Range (0.01..2, def 0.4), Shape (-1..1), Fov Dependence (0..1). Power is raised to `fov / FovDependence` (`FUN_14252ac50(power, param_4[0x48]/param_4[0x35])`), squared again when `param_1+0x1f` (stereo/split flag) is set, written to CB +0x110; enabled flag `uStack_367` when `param_1+0x1c` and `param_4+0xcc` set and power non-zero | `0x141b92690` | OBSERVED |
| Dither: no dither parameter or shader in the tone map (`Dither`, `InjectingDither`, `ShadowDither` are fog/shadow params; `EnableDitherSwitch` belongs to TempAA; `Camera Dither Fade Min` is object fade). Whether `PostOETFPS` dithers: shader | OBSERVED absence of a param; shader NOT FOUND |
| Film grain: no string or parameter containing "grain" | NOT FOUND (absent) |
| Bloom/glare in the same class (not asked): brightpass/Gauss pyramid/composite, `Glare *`, `GlareBloom *`, `FeedBackBlur`; pyramid levels from `param_4[0x16]` with a geometric weight ratio `param_4[0x1c]` summed in a loop and normalised (`fVar24 = 1/sum`) | `0x141b92690`, `0x141b91120` | OBSERVED |

## 7. Frame-order summary (from the render-step name table at `0x143c28b60`, plus the TAA study)

... `RESOLVE`, `SFX_POST_EFFECT`, `VELOMAP_*`, `RESOLVE_TEMPAA`, `AFTER_TEMPAA_BEGIN/TRANSPARENT/END`, **`HDR_FILTER`**, `FORWARD_POST`, **`FILTER`**, `DEBUG`, `BEFORE_END`, `RENDER_END`;
render groups `ScaleRender`, `Velocity`, `HDRPostProcess`, `PostProcess`, `UI`, `Debug` (`0x143c26540..`). Consistent with the TAA study: TempAA -> tone map class (HDR_FILTER) -> DOF composite -> spatial AA (FILTER), UI afterwards. OBSERVED names; the exact mapping of steps to calls INFERRED.

## 8. Limits, NOT FOUND list

1. Shader bodies (`ToneMap_*.ppo/.cpo`): the table-bake maths, tone formulas, PostOETF (PQ / Rec.2020 / gamma), LUT-application order, UI blend maths: not in the exe.
2. Shipped values of every tone-map / colour-grading parameter per area (GX param files, data).
3. Mapping of the tone-map exec struct fields (`param_4[...]`) to editor rows beyond what is stated; the update job that fills it.
4. The LUT bake `thunk_FUN_144dc31ad` and other functions in the second `.text` (`0x144c13000`, protected: `LOCK/UNLOCK` + jump into `0x144e4fd3a`).
5. Who sets the swap chain's HDR request bit, whether PC ER ever enables vendor HDR; no user HDR option found; AGS mode value 2 and NVAPI mode 5 meanings are from memory of the SDK headers, not read in the binary.
6. How "Game.Option.Disp.Brightness" (0..10) is applied in rendering; selection and brightness of the `_BlendUI_*` variants; use of the `HDRSetting` menu on PC.
7. Film grain: absent. Output-stage dither: not found.
8. The meaning of `(param_4[0xd] / 10000, / 200)` and of the +0xa0 field (10.0) in the param ctor.
9. Runtime behaviour (nothing launched).

## Method and outputs (all in this folder)

Python scans on the exe on disk (`py -E -s`, run from this folder so `pev.py` imports): `strscan.py` (ASCII + UTF-16 string search; outputs `str1..str5.txt`), `wdump.py` (strings in a VA range), `ptrtab.py` (pointer tables), `rip3.py` (RIP-relative references, outputs `rip_ags.txt`, `rip_tm1.txt`, console), `qrefs.py` (8-byte pointers, `q_tm1.txt`), `callers.py` (E8/E9 callers), `dispscan.py` (disp32 users), `vcall.py` / `vcall2.py` (vtable call-slot scan), `imp.py` (import table: AGS and DXGI imports), `guid.py` (DXGI IID search), `rtti.py`, `vtname.py` (from the TAA study).
Headless Ghidra (20 runs, `run.ps1` + `ER897Export.java` / `ErXrefs.java`, mirror `er-mirror-i0072-b`): `exp1` (editor `141ba2af0`, colour grading editor `141bb1af0`, loader `141b8f420`, exposure adjustment `141ca0dc0`, brightness setter `140e4ef30`), `exp2` (tone map Execute worker `141b89640`), `exp3` (Execute `141b90210`), `exp4` (swap chain, vendor HDR functions, `1419e9b60`), `exp5` (`141b92690` constants), `exp6` (`141b93d00`, bloom, matrix), `exp7`-`exp8` (param manager, ctor `141b967c0`, loader `141b97040`), `exp9`-`exp10` (adaptation, readback), `exp11` (format conversion, resize), `exp12` (UI shader loader), `exp13`-`exp14` (GUI registry, brightness option), `exp15` (LUT passes), `exp16`-`exp17` (HDR/SDR menus). Each has `.c` and `.asm.txt`.
