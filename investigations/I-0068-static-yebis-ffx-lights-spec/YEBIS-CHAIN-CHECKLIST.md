# I-0068 part AO: the native YEBIS chain (everything but motion blur) as one ordered checklist

Request (coordinator, 2026-10-11): specify the native port of the whole YEBIS chain except motion blur (glow/bloom, glare, tone mapping and exposure, depth of field, anti-aliasing, pass order, formats, the inputs the native build must provide) as an ordered checklist with the gparam defaults and witnesses on the five zones. Motion blur is already specified by the menus thread in [I-0067 YEBIS-MOTIONBLUR-NATIVE](../I-0067-static-menus-subsystems-map/YEBIS-MOTIONBLUR-NATIVE.md) (its steps Y1-Y16); this note reuses it and does not redo it. Step ids here are **P1-P24** to avoid its Y ids.

Labels: **STATIC** (executable, decompilation, regulation and gparam files), **OBSERVED** (the retained I-0053/I-0054/I-0063 captures, as recorded in the notes cited), **INFERRED**, **LIVE-ONLY**. Nothing was launched. This note orders and completes; the sources of every detail are [YEBIS-NATIVE](YEBIS-NATIVE.md), [YEBIS-HAL-READY](YEBIS-HAL-READY.md), [YEBIS-TECHNIQUES](YEBIS-TECHNIQUES.md), [TONEMAP-SKIN](../I-0067-static-menus-subsystems-map/TONEMAP-SKIN.md) §1, §5, §6, §15-§17, [DISPLAY-SWAPCHAIN-NATIVE](DISPLAY-SWAPCHAIN-NATIVE.md) §9, [EMISSIVE-GLOW-NATIVE](EMISSIVE-GLOW-NATIVE.md) and [NATIVE-PASS-ORDER](NATIVE-PASS-ORDER.md).

> **Part BP (YEBIS-SECOND-TRANCHE-READY):** "AutoExposure off in every file" is true for `default`, `mapbase`, `remobase` and the zone files, not for `MapOverride_0001` and `MapOverride_0003` (AutoExposure 1); the zone DoF and MotionBlur records come from `mapbase` (the zone files carry no such group).

## 0. Answer first

1. **Route.** The library does all the arithmetic; the native build gives it a **D3D12 HAL** and the same **inputs** the engine's `FUN_1400E5110`/`FUN_1400E69A0` give it, then runs three GX steps around it (LUT copy, scene copy, DoF table) and the GX after-chain (`PointSampleColor1`, DLAA, glow). Nothing in the chain has to be re-implemented as shader arithmetic. This is YEBIS-NATIVE §1's route; the pass order, formats and techniques are YEBIS-TECHNIQUES and YEBIS-HAL-READY §8.
2. **What the native build must write:** the per-frame **driver** (the part of `E5110`/`E69A0` the native renderer does not run): assemble the YEBIS globals block from the gparam banks, call the ~80 library setters in the original order, bind the scene input, call `BeginPostEffectScene`/`EndPostEffectScene`. §2 lists the inputs, §3 the steps, §4 the gparam defaults and the five zones' values.
3. **Sizes are not constants.** Every YEBIS target size (DoF layers, glare base, bloom levels, 640×540 history, 420×354) is computed by the library from the native scene size and the gparam levels. The HAL only allocates what it is asked. Observed sizes at 1280×720 are witnesses, not parameters (D-0151).
4. **Defaults that matter, all zones:** ToneType 2 (the sensitometric/filmic curve, no technique suffix), Gamma 2.2, AutoExposure **off** (so no adaptation target and no read-back), ToneMappingFactor 32 (does not enter ToneType 2), anti-aliasing = DLAA from the default file, Feedback blur off. Per zone: Exposure 1.52-2.3, FilmicContrast 0-0.3, MiddleGray (unused), glare luminance/threshold/levels, DoF quality, light shafts, colour grading, vignette (§4).
5. **One correction found while compiling the zone values:** TONEMAP-SKIN §16 gives `T.x = 2.4029` for FilmicContrast 0.3; the formula `1 + 2.08914·c^(1/3)` gives **2.3985** (the other two values of §16, 2.1100 and 2.6582, are right).

## 1. The frame position (STATIC order, OBSERVED draws)

`GXBS::GXYebis2Filter` slot 0 `FUN_1400E4CE0` → `FUN_1400E5110`, **only when `[144961F70] != 0` and pipeline flag bit 10 is set** (the world sets it, the menu views do not: they take the GX tone-map filter, step P23). It runs after the forward and effect passes and the velocity group (NATIVE-PASS-ORDER step 11b) and before the GX after-chain, the UI and the gamma pass.

## 2. Inputs the native build must provide

| # | Input | Format / size | Source | Used by |
|---|---|---|---|---|
| I1 | **scene HDR** (the lit frame after forward and effects) | R16G16B16A16_FLOAT, scene size | the native lit target; copied by `PointSampleColorClamp1` into the YEBIS input | all stages |
| I2 | **scene depth, readable** | R24G8 depth read as one channel (the library picks the channel: `Alpha_<ch>`, `…TextureTypeCast…`) | the shared readable-depth copy (NATIVE-PASS-ORDER rank 1) | DoF depth factor, DoF composite, light-shaft mask, motion blur |
| I3 | **previous frame's final glare** | 640×540 (m40, m50) or 420×354 (m31), the target of draw 14 (`MadT1_S4`) | YEBIS persistent target `17158`-class | DoF depth-factor draw of the next frame (t1) |
| I4 | **DoF focus table** | 2048×2 R32F | `CustomDofLutCopy` (GX compute, step P11), written at the end of every frame, read by the first draw of the next | DoF depth factor (t2) |
| I5 | **afterimage history pair** | glare base size, ping-pong | YEBIS persistent targets | glare `_Afterimage` |
| I6 | **colour-grading 3D LUT** | 16×16×16 (a 16×16 R32G32B32A32 source in the m31 capture) | GX compute `LutCopy2DTo3DCs` at the start of `E5110` when the LUT is enabled (`[144962228]`) from the 2D LUT chosen by `LutSourceId` | `MadColorGradingLutT1` (t16) |
| I7 | **dither noise / film grain** | 256×256 RGBA8 | library-owned, created through the HAL | tone map t3, light-shaft mask |
| I8 | **camera** | current and previous view matrix, projection (tan half angle, near, far, aspect), scene size | the native camera pair (the one the velocity workstream builds; MOTIONBLUR-NATIVE §1.2 Y3-Y4) | DoF (aperture, adaptive factor from fov), light shafts, motion blur |
| I9 | **gparam values** | §4 | the native gparam resolution (LIVE-REDUCTION-AM §5 rules): two keys A and B blended by the key weight | all setters |
| I10 | **graphics-option factors** | floats | `[[GXRS+0x5368]+0x68]` glare, `+0x80` glow, `+0x98` anti-aliasing, `+0xE0`/`+0xF8` motion blur; the option bytes at `[GXRS+0x5360]` (`+0x08` AA, `+0x20` motion blur, `+0x76`) | `E4460` (glare luminances × factor, shape 0 when factor ≤ 0), AA gate, glow gate. Defaults at the highest quality: 1.0 (INFERRED; read once per quality level) |
| I11 | **target surfaces** | default surface = native LDR (R8G8B8A8, back-buffer size) | native; YEBIS adopts it as its default surface (HAL slot 5) | final draw |

Not an input: the adaptation target (640×540 R11G11B10F) and the exposure read-back exist only with AutoExposure; it is **off in every file read** (Tone Map `AutoExposure` 0, `Enable AutoExposure` 0, all zones and the default). Keep the HAL path (YEBIS-NATIVE §6.3, a one-frame-late READBACK copy) but do not build it first.

## 3. The ordered checklist

Steps are in execution order inside one frame; P1-P2 happen once per scene setup (and again on a Setting Info change or a resize). "Mode": **CALL** = the engine function runs on the native objects, **NATIVE** = native code, **HAL** = a slot of the native `IGPUDevice` (YEBIS-HAL-READY §2).

| # | Step | Mode | Expected (defaults / witnesses) | Check |
|---|---|---|---|---|
| **P1** | **Initialise the library** (`FUN_1400E2B20` → `1416C87D0`): a native context (never a D3D11 one), trailing arguments `[144961FB8]` GlareQuality and `[144961FBC]` DepthOfFieldQuality (corrected in part BD: they are not the size; the size comes from the adopted surface, D3D12-MASTER-PLAN §6 S5), flags `0x90000000` + the Setting Info bits (MOTIONBLUR-NATIVE §2.1: Use DepthTexture 0/≠0 → `0x20000`/`0x10000`, LensDistortion `0x40000`, UnlimitedRadius `0x4C0000`, MotionBlur-Dof `0x40000`, InternalVelocityTex `0x240000`, FeedBackEffect `0x800000`, Anti-Alias `0x8040000`, Chromatic `0x40000`, Anamorphic `0x20000000`/`0x40000000`) | NATIVE + CALL | `default.gparam` words give **`0xF8EE0000`** (zones inherit them; the only per-zone differences are Setting Info `Enable ColorGrading` and `DepthOfFieldQuality`, §4.4) | `CPostEffect` `[1448E5E38]` exists; no D3D11 object reaches it |
| **P2** | **HAL** (77 slots, 29 native; creators 60-67; PSO per technique from `CTinyEffect`/`TexUtil_D3D11`; barriers) | HAL | YEBIS-HAL-READY §2-§6; pre-build the zone's PSO set at scene setup (lazy creation otherwise hitches, risk 1) | debug layer clean 600 frames |
| **P3** | **Assemble the globals block** `144961F70` (0x370 bytes) from the gparam banks: `FUN_1400E42A0` (Tone Map → exposure, gamma, tone type, factor, filmic contrast, colour H/S/V, contrast, adaptation set, vignette, dispersion), `FUN_1400E4460` (Bloom → glare, bloom, ghost, star, afterimage, anamorphic, custom set; luminances × glare factor I10), `FUN_1400E4070` (Setting Info → `144961F78..FBC`), the Dof, LightShaft, ColorGrading, FeedBackBlur and Anti-Alias groups into their globals (names and addresses: `evidence/I-0067/queries/yebis-vars.tsv`), `FUN_1400E4B50` (MotionBlur; MOTIONBLUR-NATIVE Y2). A and B keys are blended with the per-group weight (the MotionBlur blend `FUN_1401D9910` lerps floats and takes integers and flags from B; the other groups have blend functions of the same kind, not read here) | CALL the converters with native-built bank structs, or NATIVE fill of the same words | the table of §4 per zone; the struct layouts of §8 | unit test: the block bytes for `default.gparam` and for each zone |
| **P4** | **Driver calls** (`FUN_1400E69A0`, about 80 setters in the original order; list `evidence/I-0067/queries/yebis-api-calls.txt`; groups in TONEMAP-SKIN §6): projection and views (`9450`, `9490`, `95B0`, `95F0`), depth (`CB200`), auto exposure (`CA110`, `CA1B0`, `CA1D0`, `CA210`, `CA230`), tone curve (`CA450` arguments exposure, gamma, tone type, factor; `CA6A0` filmic contrast), chromatic aberration (`CA8B0`, `A870`), colour adjustment (`CBB90`..`CBC90`), glare (`C9670` threshold/remap, `C9690` shape, `C96B0` custom set), light shafts (`C97F0`, `C9830`), DoF (`CB4A0`, `C98D0`, `CBFE0`, `BFE0` edge quality), colour grading (`CA5D0`, `CA570`), feedback (`CA3B0`), anti-alias flag (`CA940`), motion blur (MOTIONBLUR-NATIVE §1.2), scene luminance scale (`C9430`) | CALL | every setter sees the value of I9/I10; the library derives its gates (`+0xC59..+0xC7D` for motion blur; DoF `c79`) from them | read `S` fields after the first frame equal the original's (one pass-through read, `S+0x6E8` exposure, `+0x6EC` gamma, `+0x6F0` tone type, `+0x70C` filmic contrast) |
| **P5** | **LUT copy** `LutCopy2DTo3DCs` when `[144962228]` is set | NATIVE (existing programme) | 16³ 3D LUT from the 2D source chosen by `LutSourceId` (§4.4); skipped when the LUT is disabled | I6 resource present only when enabled |
| **P6** | **Scene copy** `PointSampleColorClamp1`: native HDR → the YEBIS input (R16G16B16A16F), bound by `BeginPostEffectScene` (`FUN_1416C9310` → `141745D10`) | NATIVE | one draw; values clamped | input equals the lit HDR where it is finite |
| **P7** | **Library chain** `EndPostEffectScene` (`FUN_1416E8200`): the draws of YEBIS-HAL-READY §8, in the library's own order: depth factor into the scene alpha → DoF (source, aperture filter, composite; 1 + 2 + 6 + 1 draws in every zone) → glare source → bright pass → bloom (levels) → bloom combine `MadLumGammaT<levels>` → light shafts when enabled → afterimage → glare composite `MadLumGammaT<k>` → final glare resample → **tone map** → colour grading (when enabled). Motion blur sits before DoF (MOTIONBLUR-NATIVE §3) | CALL (library) on HAL | draw counts at the witnesses (§5): m31 48, m37 36, m45 36, m50 41, m51 36, m40 41 (42 draws) | H1/H4 of YEBIS-HAL-READY §8 |
| P7a | depth factor draw (`ApplyEffects_DepthFactorAlpha`) | library | t0 scene RGBA16, t1 I3, t2 I4; writes the blur factor into the scene alpha | alpha of the scene after the draw (feeds DoF) |
| P7b | DoF | library | quality = `min(Setting Info DepthOfFieldQuality, quality+0x14)` (zone value 7 default, 4 on m40, 5 on m50; `+0x14` = 0, 0, 2, 3, 5 at DISABLE..MAX; part BD); focus distance from the gparam (`DofFocusDistance` global `144962120` is read back, `FUN_1416CA2E0`); the 10-draw DoF block is present at **every** zone, including m37, m40 and m50 where both the Dof group (`Yebis-DepthOfField:Enable` 0 in every file) and the zone *Lens Simulation* group (Enable 1 only on m30, m31, m45, m51) leave DoF off: the gate is therefore not the gparam enable (INFERRED: the library initialises the stage from Setting Info and always runs it, with a zero blur factor when off) | the 10-draw DoF block at every zone |
| P7c | glare/bloom | library | §4.2; levels = `min(GlareNumLevels, levels the base size allows)`: gparam 6 gives 6 levels at 392×331 (m40), 8 gives 7 at 210×177 (m31) | bloom input count of the combine draw |
| P7d | tone map | library | **`TonemapHDR_[Dither_][Vignette_]Exposure_[Matrix_]Gamma`**, no curve suffix (ToneType 2): constants in §4.1 | t0 scene, t1 glare, t2 exposure, t3 dither |
| P7e | colour grading | library | `MadColorGradingLutT1_S1`, t0 tone-mapped, t16 the LUT; last YEBIS draw (writes the native LDR target) | LDR target written |
| **P8** | **Motion blur stage** | CALL | MOTIONBLUR-NATIVE Y1-Y16 (not repeated); +2 YEBIS draws when enabled | m40: 41 off, 43 on |
| **P9** | **Release the YEBIS targets to the native frame**; the persistent ones (I3, I4, I5) stay resident across frames and across scene changes (reset on the first frame after a load) | NATIVE | no feedback in the first frame after a load (A6 of SSAO has the same rule) | first-frame image has no smeared history |
| **P10** | (reserved: auto exposure read-back, only when `AutoExposure` is on: `FUN_1416CA160` → `144962008`, `FUN_1416CA2E0` → `144962120`) | NATIVE | off in all zones | — |
| **P11** | **DoF table** `CustomDofLutCopy` (GX compute): writes I4 for the next frame | NATIVE (existing programme) | 2048×2 R32F | table readable by next frame's first draw |
| **P12** | **After-chain copy** `PointSampleColor1` of the LDR target | NATIVE | one draw | — |
| **P13** | **Anti-aliasing** `GXAntiAliasingFilter` (`FUN_1401B9790`): runs when `filter+9`, `[[GXRS+0x5360]+8]` and the AA factor `[[GXRS+0x5368]+0x98] > 0`; gparam `Anti-Alias` falls back to the default file in **every** zone (no zone has the group): Enable 1, **AntiAliasType 4** = DLAA (filter mode 3), DlaaThreshold 0.1, DlaaRamda 1.4, DlaaEpsilon 0.12, Fxaa Alpha 1, ReduceMin 1/128, ReduceMul 0.125, SpanMax 8 | NATIVE (existing programme `DLAA.ppo`) | mode 3 constants: target size `(w,h)`, `(1/w,1/h)`, `(0.5/w,0.5/h)`, `(2/w,2/h)`; threshold term `lerp(0.1, 1, 1 − factor)` | the DLAA draw present at all zones |
| **P14** | **Glow** `GXGlowFilter` (`FUN_1401BB810`): `EmissiveToGlowPS`, `Sample13Weight` ×4, `GlowCompositeSinglePS` ×5, the last into the LDR target | NATIVE | EMISSIVE-GLOW-NATIVE §5, §8; values from `mapbase` id 0 (BlurSampleScale 1, Blur RgbScale 1.5, AlphaScale 1, New Glow Enable 1 (mode B), Final ColorScale 0.5; `default` id 10 is Enable 0); glow factor `[[GXRS+0x5368]+0x80]` 0 skips all ten draws | 1 extraction, 4 blurs, 5 composites per frame |
| **P15** | **UI** (Scaleform, menu offscreen views) | NATIVE (exists) | — | — |
| **P16** | **Gamma and present** | NATIVE | DISPLAY-SWAPCHAIN-NATIVE §9: the 1025×3 curve from the brightness option (identity at 5); without it brightness ≠ 5 is ignored | H3 of YEBIS-HAL-READY |
| P23 | **Views without YEBIS** (menu views): pipeline bit 10 clear → no step P3-P11; the GX tone map filter `GXToneMapFilter` (`pipeline+0x3060`, `FUN_1401C48D0`) with the gparam second half of Tone Map as its block `pipeline+0x3080..+0x30E0` (not an exposure at `+0x3E88`: a MotionBlur field; I-0113 RESULT « Tranche 27 », MENUS-MAP section 27): per key, default.gparam id 0 (Exposure 0, AvgLuminance 1.8, mode 0, Contrast 0.5, Gamma 2.2), the menu ids 10, 11, 13, 14, 15 mode 7 (id 11: 4.60001, 9, 0.5, 2.2) and id 12 (the Quit Game portrait) mode 0 (2.8, 3, 0.5, 1.8), UseTable 1 in all | WRITTEN for modes 0 and 7 with UseTable (I-0113 tranche 27), seen live in mode 0 (I-0113 tranche 27 after the merge: the Quit Game portrait's face matched, mode 7 not yet seen); the other modes not ported | bloom block Enable 0 in every file read, so only its tone-map part matters | menu views drawn with tone map |
| P24 | **Resize and area change**: `Initialize` again, surfaces recreated (HAL slots 60-67 re-wrap), persistent targets reset | NATIVE | no leak over 10 area changes; sizes not multiples of 4 work (D-0151) | debug layer clean |

## 4. The gparam values (STATIC; id 0 of each file, default id 0 as fallback)

Values below are the **id 0** entry of the zone's primary file (`<zone>_0000`) with the default file's id 0 as fallback of the lookup (LIVE-REDUCTION-AM §5). Zone files carry further value ids (weather and time variants, for example `m37_00`: ids 0, 10, 20, …); the engine selects by key, so a zone may leave id 0 for another id at run time: the witness reads the live key (G4·AUDIT, Glow key `pipeline+0x4028`) before comparing. Tables are produced by `Get-ZoneYebisChain.ps1` from the gparam dumps: `evidence/I-0068/queries/yebis/zone-yebis-chain.tsv` (187 parameters, seven zones and the default; the repeated names of the Tone Map and Bloom groups are joined with ` ; `, YEBIS half first, GX half second).

### 4.1 Tone map and exposure (all zones: ToneType 2, Gamma 2.2, AutoExposure 0, ToneMappingFactor 32 except m50 30)

| Zone | Exposure (A) | FilmicContrast | `T = (T.x, T.y)` | MiddleGray (unused, auto off) | ColorS | Vignette Power | Others at default |
|---|---|---|---|---|---|---|---|
| default id 0 | 1.52 | 0.3 | (2.3985, 0.21700) | 0.18 | 1 | 0 | ColorH 0, ColorV 1, Contrast 0.5 |
| `m30_00` High Wall | 2.3 | 0.15 | (2.1100, 0.12746) | 0.028 | 0.85 | 0.34 | |
| `m31_00` | 2.2 | **0** | (1.0000, 0.00000) | 0.075 | 0.9 | 0.2 | |
| `m37_00` | 1.52 | 0.3 | (2.3985, 0.21700) | 0.18 | 0.9 | **0** | |
| `m40_00` | 2.3 | 0.15 | (2.1100, 0.12746) | 0.02 | 0.8 | 0.3 | MinExposure 0.1, MaxExposure 5 |
| `m45_00` | 2.0 | 0.3 | (2.3985, 0.21700) | 0.075 | 0.85 | 0.35 | |
| `m50_00` | 2.0 | 0.1 | (1.9697, 0.09053) | 0.02 | 0.9 | 0.35 | **ColorH 1**, ToneMappingFactor 30, MaxExposure 5 |
| `m51_00` | 2.0 | 0.3 | (2.3985, 0.21700) | 0.075 | 0.85 | 0.35 | |

- **Curve (STATIC, TONEMAP-SKIN §16):** `x = max(scene · eScene · M0.x, 6.1e-5)`, `y = saturate((1 − e^(−x·T.x)) · (1 − T.y·e^(−x·T.x))²)`, `y = y + (1 − y)·glare`, `out = pow(saturate(C·float4(y, 1)), g) + dither` with `g = 1/Gamma = 0.4545`, `eScene` = the exposure above (INFERRED: confirm on one capture of the final draw), `C` = identity unless ColorH/S/V/Contrast differ from neutral (ColorS ≠ 1 everywhere, so the **Matrix** variant is expected in every zone; m50 also has ColorH 1).
- **`T.x = 1 + 2.08914·c^(1/3)`, `T.y` = linear interpolation of the 21-entry table `142B60C40` at `c·20`** (c = FilmicContrast): the table and the formula are in TONEMAP-SKIN §16; the values above are computed from them. m31's `c = 0` gives `T = (1, 0)`: `y = 1 − e^(−x)`.
- **Vignette:** the bright-pass vignette variants appear in every zone except m37 (Vignette Power 0 there), as observed (YEBIS-TECHNIQUES §4 row 9).
- Technique expectation per zone: `TonemapHDR_Dither_[Vignette_]Exposure_Matrix_Gamma` (Dither and Gamma flags set; Vignette per the library's flag, INFERRED from Power ≠ 0, to confirm at m40 where the capture has no Vignette in the tone-map name).

### 4.2 Glare, bloom and light shafts (YEBIS half of the Bloom group; LightShaft group)

| Zone | GlareLuminance | GlareThreshold | GlareNumLevels | GlareBloomGaussianRadiusScale | GlareBloomLuminanceGamma | LightShaft `Enable` (id 0) | Shaft ColorScale, Length, GlareRatio (id 0) |
|---|---|---|---|---|---|---|---|
| default id 0 | 3 | 0.5 | 8 | 0.5 | 1 | 0 | (1, 1, 1, 1), 1, 0.75 |
| `m30_00` | 6 | 0.3 | 8 | 1.5 | 1 | 1 | (0.92, 0.88, 0.81, 2), 0.76, 0.4 |
| `m31_00` | 8 | 0.2 | 8 (7 levels observed) | 2 | 1 | **0** | (1, 1, 1, 1), 1.5, 0.75 |
| `m37_00` | 3 | 0.5 | 6 | 0.5 | 1.4 | 1 | (0.50, 0.78, 1, 10.0), 4, 0.95 |
| `m40_00` | 3 | 0.2 | 6 | 2 | 1.4 | 1 | (0.70, 0.80, 1, 4.94), 1.64, 0.87 |
| `m45_00` | 6 | 0.25 | 6 | 2 | 1 | 1 | (0.27, 0.76, 1, 0.65), 3, 0.8 |
| `m50_00` | 5 | 0.25 | 6 | 2 | 1.4 | 1 | (1, 0.79, 0.74, 1.35), 1.23, 0.78 |
| `m51_00` | 6 | 0.25 | 6 | 2 | 1 | 1 | (1, 1, 0.50, 6), 0.8, 0.99 |

Fixed in all zones: GlareShape 1, RemapFactor 0.5, GlareSamplingThreshold 0.01, GlareStarFilterThreshold 0.001, GlareStarSoftness 0.25, GlareStarLengthFovDependence 0.5, GlareGhostConcentricDistortion 1, custom-bloom set all 0, anamorphic flare luminance 1, `Yebis-Enable LightShaft` (Setting Info) 1, GlareQuality 4.

**The light-shaft rows do not predict the observed draws.** The captured frames show light shafts on m31 and m50 only (YEBIS-TECHNIQUES §4 row 12), while id 0 enables them on m30, m37, m40, m45, m50 and m51 and not on m31 (and m31's id-0 values are the defaults). So the captured frames did not use id 0 of these files (the zone files carry other value ids, §4 intro), or the library's gate is not the group's `Enable` alone. This is **LIVE-ONLY**: read the Glow/Bloom key pair (`pipeline+0x4028`, `+0x4090`) and the LightShaft `Enable` word of the frame that shows shafts; the witness for the native build is the draw list (§5), not this column. Same caveat for `GlareNumLevels` and the other per-zone columns: the observed bloom levels (7 on m31, 6 elsewhere) match id 0 except that m31's 8 yields 7 by the base-size limit.

### 4.3 Anti-aliasing, feedback, glow

| Group | Parameters | Value in every zone |
|---|---|---|
| Anti-Alias | Enable, AntiAliasType, DlaaThreshold, DlaaRamda, DlaaEpsilon, FxaaAlpha, ReduceMin, ReduceMul, SpanMax | (zones carry no group, default file) 1, **4 (DLAA)**, 0.1, 1.4, 0.12, 1, 0.0078125, 0.125, 8 |
| FeedBackBlur | Enable, BlurPower, FallOff, Rate, Weight, AspectRatio, UnitTime | 0, 4.5, 0.9, 0.8, 0, 0, 0 (feedback blur is off; the afterimage of the glare is a separate stage) |
| Glow Filter | Enable, BlurSampleScale, Blur RgbScale/AlphaScale, New Glow Enable, Final ColorScale | `mapbase` id 0 (via the key rules): 1, 1, 1.5/1, 1, 0.5 |

### 4.4 Setting Info and colour grading (per zone)

| Zone | Setting Info `Enable ColorGrading` | `DepthOfFieldQuality` | ColorGrading group `Enable` / `LutSourceId` | Lens Simulation group (zone DoF) |
|---|---|---|---|---|
| default | 0 | 7 | 0 / −1 | absent |
| `m30_00` | | | 1 / 2 | FilmSize 0.024, CoCBlurCount 2, Enable 1 |
| `m31_00` | 0 | 7 | 1 / 1 | Enable 1 |
| `m37_00` | 0 | 7 | 0 / −1 | absent |
| `m40_00` | 1 | 4 | 1 / 3 | absent |
| `m45_00` | 0 | 7 | 1 / 0 | Enable 1 |
| `m50_00` | 1 | 5 | 1 / 1 | absent |
| `m51_00` | 0 | 7 | 1 / 0 | Enable 1 |

(The Setting Info columns for `m30_00` are not in `zone-yebis-params.tsv`; the file has no Setting Info group, so the default words apply.) The observed grading draw exists on **m31 and m40**, with the Setting Info flag 0 on m31: the two gates (Setting Info capability bit and the ColorGrading group's `Enable` through `[144962228]`) are not simply ANDed; INFERRED that the group `Enable` decides the draw and the Setting Info word only the capability flag in P1. `LutSourceId` selects the 2D source of P5: the texture name rule (`LutSourcePrefixA`/`IdA` strings at `143D4C9C8..`) is a GX debug-menu pair; its resolution to a file is **LIVE-ONLY** (read the texture bound to `LutCopy2DTo3DCs` t0 at one zone).

## 5. Witnesses on the zones

Observed in the retained `i63-live-10` frames (YEBIS-TECHNIQUES §4) and the I-0053 m40 frame (YEBIS-NATIVE §3.2):

| Zone | YEBIS draws per frame | Glare base | Bloom levels | Light shafts | Colour grading | Final glare size |
|---|---|---|---|---|---|---|
| m31 | 48 | 210×177 | 7 (combine `MadLumGammaT7`) | yes (`LightShaftMask_SM3`, 4 radial `TexCoordMadT1_S4`, 1 `TexCoordMadT1_S2`) | yes | 420×354 |
| m37 | 36 | 210×177 | 6 | no | not recorded | 420×354 |
| m45 | 36 | 210×177 | 6 | no | not recorded (gparam enables it: §4.4) | 420×354 |
| m50 | 41 | 392×331 | 6 | yes | (to read) | 640×540 |
| m51 | 36 | 210×177 | 6 | no | not recorded (gparam enables it: §4.4) | 420×354 |
| m40 | 41 (42 draws) | 392×331 | 6 (`MadLumGammaT6`) | no | yes | 640×540 |

Every zone shares the first ten draws (DoF: 1 + 2 + 6 + 1) and ends with the tone map and, where enabled, the grading. Checks (extend YEBIS-HAL-READY §8 H1-H4):

| ID | Check | Expected |
|---|---|---|
| **C1** | the draw list and the render-target graph per zone | the counts above with the same sizes and formats (R11G11B10 pyramid, RGBA16 DoF and scene, R8G8B8A8 LDR) |
| **C2** | `S` fields after the first frame (P4) | `+0x6E8` exposure = §4.1, `+0x6EC` = 2.2, `+0x6F0` tone type 2 (low word), `+0x70C` filmic contrast = §4.1 |
| **C3** | the tone-map constants of the final draw (one original capture; `c154`, `c186..c189`, `c202`, `c208`) | `c208 = T` of §4.1, `c202.x = 0.4545`, scene scale = exposure |
| **C4** | glare constants: the bright-pass threshold and the bloom levels | threshold = GlareThreshold of §4.2 (INFERRED mapping), levels per P7c |
| **C5** | the persistent targets | I3 640×540 or 420×354 written every frame and read by the next frame's first draw; I4 written by P11 each frame; I5 pair alternates |
| **C6** | first frame after a load | no history read (I3, I5 cleared), same image as the original's first frame |
| **C7** | resize to a size that is not a multiple of 4 or 128 | all sizes derived by the library, no crash, same graph |
| **C8** | brightness 10/0 | the gamma curve only (H3) |
| **C9** | per-zone luminance | native mean luminance at the m40 view matches the original within the non-YEBIS gaps (H2) |
| **C10** | AA | DLAA draw present at all zones with the default constants (P13) |
| **C11** | glow | 1 + 4 + 5 draws per frame; absent when the glow factor is 0 (P14) |

## 6. Dependencies and implementation order

1. P1-P2 first (the HAL alone reproduces nothing visible); then P3-P4 with the **tone-map path only** (set DoF/glare/shaft globals to the file values but accept the library running them): the library cannot be split per stage from outside, so the first visible result already includes the whole chain; the gain is the tone map (NATIVE-PASS-ORDER rank 3).
2. Then P5-P6, P11-P13, P14, P16 (small GX steps with existing programmes).
3. P8 (motion blur) after the chain works with the option off (MOTIONBLUR-NATIVE Y16: the 41-draw sequence stays unchanged when off).
4. P9, P24 (history reset, resize) before any scene-change test.

## 7. Open items

| Item | Status |
|---|---|
| `LutSourceId` → texture | LIVE-ONLY (one read) |
| exact gate of the grading draw (m31 vs m45/m51) and the light-shaft Enable the library reads | one capture per zone (draw list) |
| option factors I10 at each quality level | read once in the original (`[[GXRS+0x5368]+0x68/+0x80/+0x98]`) |
| the vignette flag in the tone-map technique name at m40 | read the technique of the last-but-one draw |
| bank struct layouts accepted by `E42A0`/`E4460`/`E4070` | **closed in §8** (Tone Map: file order, verified; Bloom: engine order table; Setting Info: by name). Still INFERRED: the unnamed Bloom words 4, 13, 16-23 |

## 8. Appendix: the bank structs the converters read, word by word (STATIC)

Closes the last open item of §7 for P3. The native build can **CALL** `FUN_1400E42A0` and `FUN_1400E4460` with a struct it fills itself; these tables give the word of each parameter. Name-to-global addresses are the registration `FUN_1400E8210` (108 of its 131 entries parsed from the assembly: `Get-YebisGlobals.ps1` output, `evidence/I-0068/queries/yebis/yebis-globals.tsv`).

### 8.1 Tone Map block for `FUN_1400E42A0(float[35])`

The 35 words are the **first 35 values of the gparam Tone Map group in file order** (the YEBIS half; the GX half starts at the second `Exposure`, word 35 is not read). Verified for every word (32 of 32 named parameters agree with the registered global; the three others are unnamed in the registration):

| Word | gparam parameter | Global written |
|---|---|---|
| 0 | Exposure | `144962008` |
| 1 | Gamma | `14496200C` |
| 2 | ToneType | `144962010` |
| 3 | ToneMappingFactor | `144962014` |
| 4 | FilmicContrast | `144962018` |
| 5 | ColorH | `144962034` |
| 6 | ColorS | `144962038` |
| 7 | ColorV | `14496203C` |
| 8 | Contrust | `1449621B8` |
| 9 | AutoExposure | `144961FC0` |
| 10 | DelayFrame | `144961FE4` |
| 11 | MiddleGray | `144961FC4` |
| 12 | ToneInfluencedByGlare | `144961FC8` |
| 13 | MinExposure | `144961FCC` |
| 14 | MaxExposure | `144961FD0` |
| 15 | Exposure MeteringWidth | `144961FD4` |
| 16 | Exposure MeteringHeight | `144961FD8` |
| 17 | Exposure MeteringOffsetX | `144961FDC` |
| 18 | Exposure MeteringOffsetY | `144961FE0` |
| 19 | AdaptationSensitivity | `144961FE8` |
| 20 | AdaptationScale | `144961FEC` |
| 21 | AdaptationSpeedLimit | `144961FF0` |
| 22 | DarkAdaptationSensitivity | `144961FF4` |
| 23 | DarkAdaptationScale | `144961FF8` |
| 24 | DarkAdaptationSpeedLimit | `144961FFC` |
| 25 | fAdaptationScale / fDarkAdaptationScale | `144962000` |
| 26 | Vignette Power | `144962040` |
| 27 | VignetteFovDependence | `144962044` |
| 28 | Enable | `14496201C` |
| 29 | WaveSample | `144962020` |
| 30 | ToneLateralDispersion[0] | `144962024` |
| 31 | ToneLateralDispersion[1] | `144962028` |
| 32 | ToneUniformDispersion[0] | `14496202C` |
| 33 | ToneUniformDispersion[1] | `144962030` |
| 34 | Exposure (the second `Exposure`: word 34, distinct global) | `1449621C0` |

Special cases of the function: word 0 (Exposure, `144962008`) is stored only when word 9 (AutoExposure) is 0; word 3 (ToneMappingFactor) feeds `+0x6F4`/`+0x6F8` of the library context.

### 8.2 Bloom block for `FUN_1400E4460(bank, float[42])` (engine order, **not** file order)

The Bloom struct has its own order. The converter multiplies the luminances by the glare factor `[[GXRS+0x5368]+0x68]` (I10 of §2) and sets the glare shape to 0 when that factor is ≤ 0. Words and globals from the function; names from the registration where it has one (blank: not in the 108 parsed entries):

| Word | Global | Name / note |
|---|---|---|
| 0 | `14496204C` | GlareLuminance (x glare factor) x glare factor |
| 1 | `144962050` | GlareThreshold |
| 2 | `144962274` | GlareSamplingThreshold |
| 3 | `1449621B4` | RemapFactor |
| 4 | `144962058` |  |
| 5 | `144962060` | GlareStarLengthFovDependence |
| 6 | `14496205C` | GlareStarFileterThreshold |
| 7 | `144962064` | GlareStarSoftness |
| 8 | `144962048` | GlareShape (0 when factor <= 0) |
| 9 | `144962054` | GlareNumLevels |
| 10 | `144962068` | GlareBloomGaussianRadiusScale |
| 11 | `14496206C` | GlareBloomLuminanceGamma |
| 12 | `144962070` | GlareGhostConcentricDistortion |
| 13 | `144962074` |  |
| 14 | `144962078` | AnamorphicFlareLuminance x glare factor |
| 15 | `14496207C` | AnamorphicFlareGhostLuminanceScale |
| 16 | `144962080` |  |
| 17 | `144962084` |  |
| 18 | `144962088` |  |
| 19 | `14496208C` |  |
| 20 | `144962090` |  |
| 21 | `144962094` |  |
| 22 | `144962098` |  |
| 23 | `14496209C` |  |
| 24 | `144962278` |  x glare factor |
| 25 | `14496227C` | BloomLuminance x glare factor |
| 26 | `144962280` | BloomDispersion |
| 27 | `144962284` | BloomDispersionBaseLevel |
| 28 | `144962288` | GhostLuminance x glare factor |
| 29 | `14496228C` | GhostHaloLuminance x glare factor |
| 30 | `144962290` | GhostDistortion |
| 31 | `144962294` | GhostSharpness |
| 32 | `144962298` | StarLuminance x glare factor |
| 33 | `14496229C` | StarStreaks |
| 34 | `1449622A0` | StarLength |
| 35 | `1449622A4` | StarSecondaryLength |
| 36 | `1449622A8` | StarRotation |
| 37 | `1449622AC` | StarInclinationAngle x constant 143D7ED40 |
| 38 | `1449622B0` | StarDispersion |
| 39 | `1449622B4` | StarForceDispersion |
| 40 | `1449622B8` | AfterimageLuminance |
| 41 | `1449622BC` | AfterimageLength |

**CORRECTED (part BL, YEBIS-WAVE1-READY §1.2): the 42 words are a permutation of file words 0 to 40 plus one record-only word (word 4, default 1.0); word 13 is the first `Enable`, words 16 to 23 the two anamorphic colours; the second `Enable` is record word 42.** Words 4, 13 and 16-23 have no registered name in the parsed entries. INFERRED from position and size: word 13 (`144962074`) is the anamorphic-lens enable (the `Enable` of the first Bloom half), words 16-19 the anamorphic flare colour and 20-23 the anamorphic ghost colour (four floats each: the two colour parameters of the group); word 4 (`144962058`) is not read by the glare setters listed in TONEMAP-SKIN §17. The converter multiplies words 0, 14, 24, 25, 28, 29 and 32 by the glare factor, word 37 by the constant `143D7ED40`, and copies words 16-23 a second time into the library context (`+0x2220..+0x225F`). Because the order is the engine's own, the native struct is built **by name**: for each registered name take the gparam value of that name and write it at the word of the table; for the unregistered words take the value of the file parameter named in the INFERRED line above.

### 8.3 Setting Info for `FUN_1400E4070(int[…])`

**CORRECTED (part BL): the integer words are the 18 type-3 entries in file order (so `BlurShape` is word 15, `GlareQuality` 16, `DepthOfFieldQuality` 17) and word 20 is the float `SceneLuminanceScale`; YEBIS-WAVE1-READY §1.2.** The words are the Setting Info integers **after `BG Color`** in file order, written to `144961F78 + 4·word` with change flags that raise the re-initialisation (MOTIONBLUR-NATIVE §2.1). Verified for words 0-8 (Use DepthTexture … FeedBackEffect = `144961F78..144961F98`); the registration places Anti-Alias at `144961FA0`, LightShaft at `144961FA4`, Chromatic aberration at `144961FA8`, the anamorphic words at `144961FB0/FB4`, GlareQuality at `144961FB8` and DepthOfFieldQuality at `144961FBC` (so the globals are not contiguous after FeedBackEffect: `144961F9C` and `144961FAC` are not written by name). Fill the global by name, not by position, for this group.

## Exports

`Get-YebisGlobals.ps1` (name → global from the registration `E8210`, §8) and `Get-ZoneYebisChain.ps1` (this folder) with the gparam dumps from `Get-GparamValues.ps1`: `evidence/I-0068/queries/yebis/zone-gparam/`, result `evidence/I-0068/queries/yebis/zone-yebis-chain.tsv`.

See also [YEBIS-D3D12-DESIGN-NATIVE](YEBIS-D3D12-DESIGN-NATIVE.md): the execution model (recorder, plan groups, resources, phases A-G, steps and live rows YB1-YB16).
