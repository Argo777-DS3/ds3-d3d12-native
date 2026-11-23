# YEBIS and the tone map of wave 1: bank blocks, converters, recorder HAL, library host, scene copy and output, with the global validation ladder (ready to implement, static)

Request (coordinator, 2026-10-11): the implementation-ready note of the last trunk of wave 1, YEBIS and the tone map (item 6 of [WAVE1-PREP](WAVE1-PREP.md) §2.6: Y1 recorder skeleton, Y2 `CPostEffect` init and driver, Y3 scene copy and tone map; the gamma pass Y0 is [GAMMA-COLOUR-PATH-READY](GAMMA-COLOUR-PATH-READY.md)), in the format of [GLOW-WAVE1-READY](GLOW-WAVE1-READY.md) and [SSAO-WAVE1-READY](SSAO-WAVE1-READY.md): shaders and fingerprints, constants read in the executable, the missing integer accessor of `Banks`, the order with the gamma pass, the commits under flags, the tests and the `Banks` goldens, and, because YEBIS cannot be validated stage by stage, the global validation strategy (§7). Independent Opus 5.5 review applied (log in §11). Static; no native code was changed, nothing was launched. The execution model, the 77 slots of the HAL and the draw lists are the earlier notes ([YEBIS-D3D12-DESIGN-NATIVE](../I-0068-static-yebis-ffx-lights-spec/YEBIS-D3D12-DESIGN-NATIVE.md), [YEBIS-HAL-READY](../I-0068-static-yebis-ffx-lights-spec/YEBIS-HAL-READY.md), [YEBIS-CHAIN-CHECKLIST](../I-0068-static-yebis-ffx-lights-spec/YEBIS-CHAIN-CHECKLIST.md), [YEBIS-TECHNIQUES](../I-0068-static-yebis-ffx-lights-spec/YEBIS-TECHNIQUES.md)); this note re-reads `FB890` (the engine's own per-frame feeding of the library), the three bank blends and the converters, and turns the whole into commits.

Labels: **verified** = read in the executable (`DarkSoulsIII.exe` 1.15.2.0, SHA-256 `EF5E07C5…498C`) or in the decompiled functions for this note; **note** = a statement of a linked note; **observed** = read in the native tree at `4f2324d5` (line numbers are that tree's; anchors are function names); **proposed** = this note's design; **inferred**; **open**. Abbreviations: `SP` = `native/renderer/scene_pass.cpp`, `GB` = `native/frame/game_bootstrap.cpp`, `LP` = `native/world/light_producers.cpp`, `MF` = `native/renderer/model_frame.h`.

> **Part BP (YEBIS-SECOND-TRANCHE-READY):** the commit YB-4b is specified there (depth of field, motion blur, feedback, auto exposure, history). Three statements below are corrected: the DoF driver override targets record word **2**, not word 3 (section 1.1); auto exposure is **on** in `MapOverride_0001` and `MapOverride_0003` (sections 1.2 and 9); and `E4890` can run on a scratch object seeded with `+0x2270 = 0, +0x2271 = 1, +0x2272 = 1` (section 4.4). Where this note differs, the new sheet wins.

## 0. Answer first

1. **The engine already says exactly how the library is fed, and it is nine groups, not three.** `FUN_1400FB890` lines 304 to 460 (verified) look up two records per group in the pipeline's bank table (`root+0x4608…0x4658`, 8 bytes apart), blend them with a per-group function and call a converter that writes the library's globals block `144961F70`: Setting Info → `E4070`, Tone Map → `E42A0`, Bloom → `E4460`, Light shafts → `E4700`, Depth of field → `E4890`, Motion blur → `E4B50`, plus the Anti-Alias flag, the Feedback-blur words and a colour-grading bank. The key index map `i` (A at `context+0x3FD8+8i`, B at `+0x4040+8i`, weight at `+0x40A8+4i`) is the same one SSAO (i = 8), the glow (i = 10) and the motion blur (i = 6) use (§1.1). Native never runs `FB890`, so a native **bank-block builder** must reproduce the lookups, the blends and the record layouts.
2. **The three first records are determined, with three words left explicit.** Tone Map: 35 words in file order (word 34 inferred, §1.2), blend = lerp except words {2, 9, 28, 29, 34} copied from B. Bloom: 42 words in the engine's order, which is a permutation of file words 0 to 40 plus **one record-only word (word 4, default 1.0, not a file value)**; the second `Enable` is record word 42, outside the 42-word block; word 13 is the first `Enable`, words 16 to 23 the two anamorphic colours; blend = lerp except words {8, 9, 13, 27, 31, 33, 36, 39} copied from B. Setting Info: a 0x80-byte record whose 18 integers are the type-3 entries in file order (so `BlurShape` is word 15 and `SceneLuminanceScale`, word 20, is the only lerped float the converter reads) (§1.2 to §1.4). Goldens for the default file and the seven zone files are generated into the evidence store (673 words, `bank-blocks.tsv`).
3. **The integer accessor is a typed entry accessor** (`Banks::GroupEntries`, §4.1), the same word accessor the glow and SSAO notes need (`GroupWord`, defined once in GL-2); no cast of floats is involved.
4. **The three first converters can be called before any library exists, with a scratch object**: `E4070`, `E42A0` and `E4460` write the globals block and a few flags and fields of the object (`E4070`: `+0x20F8`, `+0x2278`, assigned; `E4460`: `+0x2220…0x225F`; `E42A0`: none) and read quality-object fields `[GXRS+0x5360]+0x10..0x1C` and the factor object `[GXRS+0x5368]` (guarded, §4.4), so a zero-filled 0x2400-byte object lets the **globals themselves** be checked at the first rung (§7 V1); `E4700` and `E4890` read more of the object and are fed on the real post object (§4.4).
5. **Validation is a ladder of observables, not of stages** (§7): V0 blocks and goldens, V1 converter round trip into the globals, V2 the library's CPU run on a recording HAL that draws nothing (draw list against the witnesses H1/H4), V3 GPU encode into scratch targets, V4 output replaces the sRGB composition behind a flag, V5 default. Each rung ships behind its own flag and changes no pixel until V4.
6. **Order with the gamma pass**: lit HDR → scene copy (`PointSampleColorClamp1`) → library → LDR (`DisplayStage::Target()`) → glow → UI → gamma curve (§4.7). The tone map already applies `pow(·, 1/2.2)`, so the gamma pass stays a pure brightness curve (identity at 5). Skipping the composition when the output is valid supersedes GAMMA-COLOUR-PATH-READY §3.7 and WAVE1-PREP Y3 on that point; the anti-aliasing (DLAA) is absent in wave 1 and is an expected difference of the image witness H2.
7. **Eleven commits YB-1..YB-10, YB-4b, YB-4c** (§5): the first six need no library; every rung is **opt-in** (`yebis.flag` plus a rung flag) until its exit holds (§7); the HAL, the host and the encode follow the existing HAL note; tests T1 to T12 (§6). Colour grading and light shafts are **mandatory feeds before the library runs** (YB-4b, YB-4c), because the library otherwise skips the LUT copy and the shaft draws and the draw list cannot match.

## 1. What the original does (verified)

### 1.1 The per-frame feeding of the library (`FUN_1400FB890`, lines 304 to 460)

For each group the code reads two keys (A, B) and one weight of the pipeline context, looks the two records up in a bank of the root object, blends them into a local buffer and, when `post+0x2270 == 0` (no "ignore external settings"), calls the converter on the library's configuration object `post = [[pipeline+0x4DD8]+0xCB8]`.

| `i` | Group (inferred from the converter's globals) | Bank (`root+`) | Keys A / B (`context+`) | Weight | Lookup | Blend | Converter / effect |
|---|---|---|---|---|---|---|---|
| 0 | Setting Info | `0x4610` | `0x3FD8` / `0x4040` | `0x40A8` | `15E920` | `1401D8220` (record `+0x10`) | `E4070(post, out)` |
| 1 | Tone Map | `0x4618` | `0x3FE0` / `0x4048` | `0x40AC` | `1638C0` | `140218BA0` (`+8`), defaults `F8D70`, `rec+0xDC ← _DAT_144962008` first | `E42A0(out)` |
| 2 | Depth of field | `0x4630` | `0x3FE8` / `0x4050` | `0x40B0` | `16EED0` | `14021E720` (`+8`), `out+0x30 = 0` when `context+0x413D`; word 3 ← `context+0x4140` and word 24 = 0 (lines 383 to 387) | `E4890(post, out)` |
| 3 | Bloom | `0x4620` | `0x3FF0` / `0x4058` | `0x40B4` | `167D30` | `1402148D0` (`+0x10`), defaults `1402147D0` | `E4460(post, out)` |
| 4 | Colour grading (inferred) | `0x4650` | `0x3FF8` / `0x4060` | `0x40B8` | `17BEE0` | `14021C150(out, rec+8 or rec+0x250 by byte rec+0x4EC, …)` | `_DAT_144962228` (the **LUT-copy trigger**), `22C…238`, `1449622C8/D0` and three string objects at `post+0x2100/0x2130/0x2160` (`1400ECA10`: the LUT source names) |
| 5 | Feedback blur | `0x4648` | `0x4000` / `0x4068` | `0x40BC` | `178CF0` | `140221640` when weight > 0, else record A | globals `144962178…188` |
| 6 | Motion blur | `0x4640` | `0x4008` / `0x4070` | `0x40C0` | `175C60` | `1401D9910` | `E4B50(out)` |
| 7 | Anti-Alias | `0x4638` | `0x4010` / `0x4078` | `0x40C4` | `1723C0` | `140220B50` | `_DAT_144962190` = 1 when record word 0 ≠ 0 and word 1 = 0, forced 0 when `quality+8 = 0` or the AA factor `+0x98 ≤ 0` |
| 8 | SSAO (called from `103510`, not `FB890`) | `0x4608` | `0x4018` / `0x4080` | `0x40C8` | `15AF60` | `140223780` | SSAO layer (SSAO-WAVE1-READY) |
| 9 | Light shafts | `0x4628` | `0x4020` / `0x4088` | `0x40CC` | `16B030` | `14021D690` (`+0x10`) | `E4700(post, out)` |
| 10 | Glow filter (`FB890` lines 657 to 665) | `0x4658` | `0x4028` / `0x4090` | `0x40D0` | `17EF20` | `2227C0` | filter record (GLOW-WAVE1-READY) |

All blends share the first two lines of every snap rule: `|w| ≤ ε` → A, `|w − 1| ≤ ε` → B with `ε = [143D7ECE8] = 1.1920929e-7`; only the in-between branch differs (§1.3). `E5110` itself runs only when `[144961F70] ≠ 0` and the pipeline bit 10 is set (note, checklist §1); which views run the `FB890` part is open (row YB22).

### 1.2 The three records

**Tone Map** (`E42A0`, input `float[35]` = record words 0 to 34): record words 0 to 33 are the file values 0 to 33 in file order (word names in the checklist §8.1; verified by the registered global names). The record has 55 words and its tail is **not** the file order: `F8D70` defaults at `0x2C…0x34` (0, 0.5, 2.2, 1, 0, 1.0, 0.5, −2.0, 4.0) equal file entries 42 to 50, `[0x24]` = 1.8 is `AvgLuminance`, so the file values from 35 on sit one word later and the record has one non-file word (`[0x28]` = 10.0); word 34 (copied from B) is therefore **inferred** to be the file's second `Exposure` and the checklist §8.1 contradicts itself there; the values coincide (0) in every shipped file. Words `0x35` (the exposure global written back by `FB890` before the blend) and `0x36` are the stored-exposure words; they matter only with auto exposure, which is gated by the record words of the file entries 9 and 46 (`AutoExposure`, `Enable AutoExposure`).

**Bloom** (`E4460`, input words 0 to 41 of a 50-word record): the engine order. Words 0 to 3 and 5 to 41 are the first 36 entries of the `Bloom` group (two of them `float4`) **permuted**, `f[k]` = the k-th file word (entries expanded in file order, 42 words `f[0..41]`); the 50-word record continues with the GX half, whose first word (record word 42) is the second `Enable` (`f[41]`) and word 43 `GlareNumLevels` (the blend copies 42 and 43 like integers, the defaults set them to 0 and 7):

| Engine word | File word `f[k]` | Name |
|---|---|---|
| 0 to 3 | `f[0..3]` | GlareLuminance, GlareThreshold, GlareSamplingThreshold, RemapFactor |
| **4** | — | a **record-only word**, default 1.0 (`1402147D0`), not a file value; unread by `E4460`'s globals map as far as the checklist knows (`144962058`) |
| 5 to 12 | `f[4..11]` | StarLengthFovDependence, StarFileterThreshold, StarSoftness, GlareShape, GlareNumLevels, RadiusScale, LuminanceGamma, GhostConcentricDistortion |
| 13 | `f[30]` | the first `Enable` |
| 14, 15 | `f[31], f[32]` | AnamorphicFlareLuminance, AnamorphicFlareGhostLuminanceScale |
| 16 to 19 | `f[33..36]` | AnamorphicFlareColor (4 words) |
| 20 to 23 | `f[37..40]` | AnamorphicGhostColor (4 words) |
| 24 to 41 | `f[12..29]` | the second GlareLuminance, BloomLuminance … AfterimageLength (18 words) |

It is a bijection from engine words {0 to 3, 5 to 41} onto `f[0..40]`, and every one of the 30 registered names of the checklist §8.2 agrees with it (verified against `yebis-globals.tsv`; `converter-words.tsv` lists the 42 words with their globals). The unnamed words of that note are closed except word 4: word 13 = `f[30]`, words 16 to 23 = the two colours; word 4 stays **open** (default 1.0; row YB19 compares it with the engine record).

**Setting Info** (`E4070`, `int[…]`, record `+0x10`, 0x80 bytes): words 0 to 17 are the 18 type-3 entries after `BG Color` in file order (15 enables from `Use DepthTexture` to `Enable ColorGrading`, then `BlurShape`, `GlareQuality`, `DepthOfFieldQuality`) written to `144961F78…F98` (words 0 to 8), `FA0` (9), `FA4` (10), `FA8` (11), `FB0` (12), `FB4` (13), `144962224` (14), `144962270` (15), `FB8` (16), `FBC` (17, capped, §1.5); word 20 is the float at `+0x50` stored to `144962004` (the scene luminance scale, **inferred** from the position of the first lerped float); words 18 and 19 and the floats at `+0x54…0x60` are not read by the converter. The record puts the integers first: in the file `SceneLuminanceScale` precedes `BlurShape`, `GlareQuality` and `DepthOfFieldQuality`, yet those three integers are words 15 to 17 of the record.

### 1.3 The three blend rules (snap on `ε`, then)

| Record | Copy from B (integers, flags) | Lerp `(b − a)·w + a` |
|---|---|---|
| Tone Map words 0 to 34 (`140218BA0`) | words **2, 9, 28, 29, 34** (ToneType, AutoExposure, Enable, WaveSample and the inferred word 34; not all are "integers") | all others (when the record words of `AutoExposure` (9) or `Enable AutoExposure` (48) of A or B are non-zero, word 0 and word 35 follow the stored-exposure rule, `140218BA0` lines ~205 to 253; **no shipped file sets it**; the native refuses it with a row when either is non-zero) |
| Bloom words 0 to 41 (`1402148D0`) | words **8, 9, 13, 27, 31, 33, 36, 39** | all others, including word 4 (record-only, default 1.0 in A and B, so 1.0) and the two colours (words 16 to 23) |
| Setting Info (`1401D8220`) | everything (the buffer is B) | the nine floats at record bytes `0x50, 0x54, 0x58, 0x5C, 0x60, 0x70, 0x74, 0x78, 0x7C`; the byte `0x64` from B |

At the snaps the output is A or B wholesale. In practice the weight is 0 or 1 except during a key transition, so the lerp rows matter in transitions and in the tests.

### 1.4 The other converters (second tranche)

`E4700` (light shafts, 28 input words) and `E4890` (depth of field, 34 input words) are the same kind of function; the word-to-global pairs (with the registered names) are in `evidence/I-0068/queries/yebis-ready/converter-words.tsv` (23 and 32 rows). `E4700` multiplies two words (7 and 0x1B) by the factor `[[GXRS+0x5368]+0x50]`, zeroes word 0 when it is ≤ 0 and writes `post+0x21E0…0x221F`; the DoF words 1 to 17 and 33 are named (`ApertuerF`, `DofFocusDistance`, `CCD-Size`, `DofAdaptiveApertureFactor/BaseFov`, `DofRotateScale/Offset`, `DiaphragmRotate`, `DofEdgeQuarity`, `Enable-Near-Custom`, `DofNearDepthStart/End`, `DofNear CocSize/CocScale`, `BackgroundMaskThreshold`). Their blend rules are exported (`yebis-ready/b2/14021d690.c`, `14021e720.c`); `E4890` also reads the factor `[GXRS+0x5368]+0xC8` (scaling and capping words 8 and 9 into `144962140/144`), `post+0x2271/+0x2272` (they gate `DofFocusDistance` and word 24), quality-object fields `+0x10/+0x18/+0x1C` and `DAT_144961BF4`, so it must run on the real post object. The builders for these two groups follow the Bloom method and are **YB-4b**, **mandatory before the library runs**: m31 (48 draws) and m50 (41) include light-shaft draws, and the DoF block is configured by `E4890` (the 10-draw block runs at every zone whatever the enable, YEBIS-CHAIN-CHECKLIST P7b, but its sizes and passes follow these globals). `converter-words.tsv` lacks `E4890` words 8 and 9 (they pass through arithmetic and are listed by hand in YB-4b). The **colour-grading bank** (row 4) is **YB-4c**, also mandatory: the grading draw is observed at m31 and m40 (the group `Enable` of id 0 is 1 at six zones, the Setting Info flag only at m40 and m50; [FFX-WAVE1-READY](FFX-WAVE1-READY.md) §4.2) and the library skips the LUT copy and the grading draw without `_DAT_144962228`. Until YB-4b/4c exist the feed is not complete and the ladder stops at V1 (§7). The Anti-Alias flag `144962190`, the Feedback globals and `E4B50` (motion blur, from the velocity thread) are fed in YB-4b with their file values: feedback blur is off and the library AA flag is 0 in every shipped file (`AntiAliasType` 4 selects the GX DLAA filter, not a library stage), so the expected draw-list deltas are none for feedback and AA, and the motion-blur draws appear only when the MotionBlur option is on (+2 draws, checklist P8).

### 1.5 Constants read in the executable

| Item | Value |
|---|---|
| tone-map curve `T.x` | `1 + 2.0891273 · c^(1/3)` with the data words `[142B60CD0]` = 2.0891273 (`4005B443`) and `[143D7EDA4]` = 1/3 (`3EAAAAAB`) read by `FUN_141718390` case 10 (`pow(c, [143D7EDA4])·[142B60CD0] + 1`), `c = FilmicContrast`; the earlier notes' 2.08914 was a rounding |
| tone-map curve `T.y` | linear interpolation at `c·20` of the 21 floats at `142B60C40`: 0, 0.048482, 0.090533, 0.127458, 0.16023, 0.1896, 0.217, 0.246641, 0.279107, 0.314625, 0.35344, 0.395867, 0.4422, 0.492722, 0.547864, 0.608, 0.67358, 0.745071, 0.823, 0.90795, 1 |
| curve and glare | `x = max(scene·eScene·M0.x, 6.1e-5)`, `y = saturate((1 − e^{−x·T.x})·(1 − T.y·e^{−x·T.x})²)`, `y += (1 − y)·glare`, `out = pow(saturate(C·(y,1)), 1/Gamma) + dither` (TONEMAP-SKIN §15/§16, the disassembly of `ps_TonemapHDR_Dither_Exposure_Matrix_Gamma`) |
| ε / gamma / 1/64 | `[143D7ECE8]` = 1.1920929e-7; `[143D7EE90]` = 2.2; `[143D7ED38]` = 0.015625 |
| glare wavelengths | `142B63170/68/6C` = 6.15e-7, 4.65e-7, 5.40e-7 |
| bloom constant | `[143D7EEB8]` = 3.0; `[143D7ED40]` = 0.0174532924 (π/180, applied to `StarInclinationAngle`, word 37) |
| quality caps | `E4070` caps `DepthOfFieldQuality` at `quality+0x14` (0, 0, 2, 3, 5 for DISABLE..MAX); `GlareQuality` is uncapped |

## 2. Programmes and fingerprints (verified)

The library's shaders are **not acquired natively**: the technique table (`TexUtil_D3D11`, 2,555 techniques at `142BDB730`) is parsed by the engine's own `CTinyEffect` (CALL), which hands the HAL the DXBC blobs. The pins below are the test oracle and the optional runtime check, not an acquisition path.

**World technique set** (`evidence/I-0068/queries/yebis-ready/world-techniques.tsv`, 26 observed and 56 candidate rows, SHA-256 of the vertex and pixel blobs from `evidence/I-0067/queries/yebis-techniques.tsv`; regenerate with `Get-YebisWorldTechniques.ps1`). Observed draws and their blobs (first 12 hex digits):

| Stage | Technique | VS (`vs_2D_…`) | PS |
|---|---|---|---|
| DoF depth factor | `DepthOfField_BlurFactorAlphaFromDepthTextureTypeCastLutT1_S1_RVALUE` (channel inferred) | `888904BA6446` (`Tex2D1`) | `63E3F424AA40` |
| downsample 2:1, S1 / S2 / bloom up S4 | `MadT1_S1`, `MadT1_S2`, `MadT1_S4` | `BDFD79CCB664`, `D2EEEECA1DFC`, `28BA5A18CA1D` | `8779D207D73A`, `1EBD329AB797`, `CBC404544D10` |
| DoF aperture, composite | `DepthOfField_FrontBlurBlendFactor`, `…CompositeBlurLevelsFrontBlur_TS0_3` | `888904BA6446`, `27BF4CE17702` | `41274D0A9FC3`, `6FA40F398593` |
| bright pass | `BrightPassT1_[Vignette\|VignetteSimulate_]THRESHOLD_LUMINANCE_REMAP_S1` | `888904BA6446`, `27BF4CE17702` | `54110559836F`, `76008493FF29`, `79FE89743A4D` |
| bloom combine, glare composite | `MadLumGammaT1`, `T2`, `T6`, `T7` | `888904BA6446` | `5A0A02CEA568`, `37D5F74D8897`, `970350A1ED4B`, `E40EF8201A55` |
| light shafts (m31, m50) | `LightShaftMask_SM3`, `TexCoordMadT1_S4`, `TexCoordMadT1_S2` | `7F09C8F7D24E`, `5715444AA733`, `27BF4CE17702` | `D275F69939F1`, `CBC404544D10`, `1EBD329AB797` |
| depth to scene alpha | `Alpha_R` | `BDFD79CCB664` | `825861A7BE1A` |
| **tone map** | `TonemapHDR_[Dither_][Vignette_]Exposure_[Matrix_]Gamma` (8 variants, all `vs_2D_Tex2D5`) | `6C48209F2312` | Dither+Exposure+Gamma `153D4DA615B3`; Dither+Exposure+Matrix+Gamma `358673A09BE2`; Dither+Vignette+Exposure+Gamma `AD68AC7710C9`; Dither+Vignette+Exposure+Matrix+Gamma `D225AEBACAA9`; without Dither: `4D99F39F3118`, `108F48AABEF3`, `BC084DF2B3DD`, `DE226881AC7A` |
| colour grading | `MadColorGradingLutT1_S1` | `BDFD79CCB664` | `7C873084B455` |

`TexCoordMadT1_S4` and `MadT1_S4` share one PS blob (`CBC404544D10`), as `TexCoordMadT1_S2` and `MadT1_S2` (`1EBD329AB797`), which the technique note had inferred and is now verified. The selection rule of the tone-map variant is the library's (TONEMAP-SKIN §16: `Gamma` when `+0x6EC ≠ 1`, `Matrix` from the HSV/contrast flags, `Dither`, `Vignette`); the tone map reads `t0` scene, `t1` glare, `t2` exposure, `t3` dither.

**GX programmes around the library** (pixel and compute, `programs.tsv`, from the loose cache; resolved by name like the lighting set):

| Programme | Stage | Bytes | SHA-256 | DXBC hash | Semantics |
|---|---|---|---|---|---|
| `PointSampleColorClamp1` | ps | 736 | `91462FDC7E630E371557714D81F27C6C460160D26FFE8E778536C6DD943E37EB` | `715453357C943A399FE70D5F4F871648` | `max(t0.SampleLevel(s2 ClampPoint, uv, 0), 0)` (the scene copy: removes negative values) |
| `PointSampleColor1` | ps | 688 | `204D5639DC127F39C92CA069014FA216607B83CECEA83D1829F1445B8DD0AD53` | `1B9057CC2AA18BC7E6FF9D125F5C1805` | the after-chain copy (not needed with the `Ldr` ping-pong, later) |
| `LutCopy2DTo3DCs` | cs | 1040 | `BDDD16E09DAFA473B5604D355679C41BDD9F6FEBE7DFFFF50F080D4E2085534E` | `08FA50DC0743E4FE0B00B040A40BAB2D` | `[4,16,1]` threads, group `z` = the slice: reads a 16×256 2D strip (slice z at rows 16z to 16z+15) and stores the 16³ UAV |
| `CustomDofLutCopy` | cs | 1856 | `3D3D289A9697BBA54D3A89735AC1FD9C36DE91D0C0D22E0CA282CB71387F35B0` | `75A8D995E881CF7505A8BD4922694378` | `[128,1,1]` threads, `cbComputeCustomDof` (513 vectors) → the 2048×2 R32F table |
| `DLAA` | ps | 6132 | `51F9ED5385294EE57343E15C01A4F0F25E3C757BFDD42594461715A22B5E6326` | `461779CFF261A8CA8D94206BCB2F0815` | anti-aliasing (not in this note's scope) |

## 3. What native has and lacks (observed, tree `4f2324d5`)

| Item | Native |
|---|---|
| frame | `RenderNativeModels`: G-buffer, lighting, forward, velocity (`SP:1601`), composition of the lit HDR into the back buffer (`source` `SP:1606`, `composition_pso_` `SP:1611`), UI (`SP:1616`), readback, `Present` |
| colour intent | `NativeRenderStep` (`GB:7331`, hook `{0xD55190}` `GB:10036`) forms `native_colour_settings` (`GB:325`, `GB:7405`); nothing reads it |
| banks | `Banks::GroupFloat` (floats only); `gparam_banks` (`GB:511`); the Tone Map keys/ratio witness `native_tonemap_inputs` (`GB:4692`) |
| engine objects | none of `CPostEffect`, the IGPUDevice, the context; `E2B20`, `E5110`, `E69A0`, `E4CE0` not hooked |
| inputs | the quality object and the option factors `[GXRS+0x5368]` are readable; the readable depth copy for the library does not exist (the main depth has an `R32_FLOAT_X8X24` SRV in the lighting views) |
| programmes | `AcquireNativeLightingPrograms` (`GB:2714`) and `FindLoadedProgramme` (`GB:2693`) as the pattern; no YEBIS or GX post programme is looked up |
| GPU precedents | `NativeVelocityResources` (`SP:1218`), `GlowStage`/`AoStage` (notes), `DisplayStage` (note) |

## 4. Design (proposed)

### 4.1 `Banks::GroupEntries`, the typed accessor (YB-1)

```
struct BankEntry { std::string name; uint8_t type; uint8_t words; std::array<uint32_t,4> raw; };   // file order; words = 1 (scalars, bytes widened) or 4 (float4)
bool Banks::GroupEntries(uint32_t area,uint32_t id,const char* group,std::vector<BankEntry>& out,std::string* source=nullptr) const;
bool Banks::GroupWord(uint32_t area,uint32_t id,const char* group,const char* param,uint32_t& out,std::string* source=nullptr) const;   // defined once (GL-2); built on GroupEntries
```
The file, group and id rules are `GroupFloat`'s (`{−1, −1}` → `{1, 0}`; the area file's group, else `mapbase`'s, then the id in that file, else `default` id 0 — `LP:1320-1331`), so a lookup of an absent id **succeeds from `default`** and only fails when `default` lacks the id too; the engine then returns the bank's own fallback record (`140167D30` falls back to key `{1, 0}` and finally `bank+0x2C0`; the fallback content is **open**, row YB19). The group name is matched like `Gparam::Group` (exact `name2`); `Setting Info` needs the trim that `Banks::BackgroundColor` applies (`LP:1340`). Duplicate names (`Exposure`, `Gamma`, `GlareLuminance`, `Enable`) are separate entries in file order. **`GroupWord` is defined once, by GL-2, as `GroupFloat`'s body returning the raw word; `GroupEntries` is implemented on the same lookup lambda and `GroupWord` and `GroupFloat` stay thin wrappers**, so the three never diverge (reconciled with GLOW-WAVE1-READY §5.1). The goldens cover the files that carry the group: `default`, the seven zones (Tone Map and Bloom) and Setting Info only for `default`, `m40_00_0000`, `m50_00_0000` (the other zones fall back to `default`, so their Setting golden equals `default`'s); there is no `mapbase` dump of these groups.

### 4.2 The block builders and the blends (YB-1)

`world/yebis_blocks.h/.cpp`, pure functions:

```
struct YebisBlocks { std::array<uint32_t,21> setting; std::array<uint32_t,35> tone; std::array<uint32_t,42> bloom; };
bool BuildYebisBlock(group, entriesA, entriesB, weight, epsilon, ...);     // per group: file entries → record layout (§1.2), then the blend (§1.3)
YebisBlocks BuildYebisBlocks(const Banks&, const YebisKeys& a, const YebisKeys& b, const YebisWeights& w, float epsilon);
```
`Setting` words 0 to 17 are the type-3 entries in file order, 18 and 19 zero, 20 the `SceneLuminanceScale` bits, the rest zero; `Tone` is the first 35 entries in file order; `Bloom` is §1.2's permutation of the expanded 36 entries (the bank-blocks script `Get-YebisBankBlocks.ps1` is the executable specification). The blends are §1.3 exactly (ε snap, B-copy sets, lerp in `float`). The builders refuse (log row, no block) a record whose `AutoExposure` word is non-zero or whose entry counts differ from 51/43/25 (the shipped layouts).

### 4.3 `CollectNativeYebisInputs` (YB-3)

Beside `CollectNativeVelocityInputs` (`GB:4600`, called `GB:4709`): the view context from `scene_view_cpu_owners`; for `i ∈ {0, 1, 3}` (and 2, 4, 5, 7, 9 with YB-4b/YB-4c) the keys A and B at `context+0x3FD8+8i`, `+0x4040+8i` and the weight at `+0x40A8+4i`; `ε = image+0x3D7ECE8`; the factors `[[image+0x4961B50]+0x5368]+{0x50, 0x68, 0x98, 0xC8}` (light shafts, glare, anti-aliasing, DoF; 1.0 when the object is absent) and the quality bytes (`+0x08` AA, `+0x10…0x1C`, `+0x14` DoF cap); calls `BuildYebisBlocks`; fills `std::optional<NativeSelectedScene::Yebis> yebis` (blocks, factors, quality, `world_view`) and the frame copy `NativeModelFrameInput::yebis` after the velocity (`MF:75`, `GB:5443`). **Main view only**, gated by the opt-in flag `yebis.flag` (§5) and by the engine's own gate: `context+0x50` bit 10 and `[144961F70] ≠ 0` (`FB890` lines 108 and 294; the world sets the bit, the menu views do not). Row `native_yebis_inputs`: `{read, keys[3][4], weights[3], block digests, factors, quality, auto_exposure_refused}` on the first three frames, on change and every 600th.

### 4.4 The converters as engine calls (YB-4)

`YebisConverters::Apply(void* post, const YebisBlocks&, const YebisFactors&)` calls `image+0xE4070`, `+0xE42A0`, `+0xE4460` (and `+0xE4700`, `+0xE4890`, the colour-grading and remaining converters with YB-4b/YB-4c) with the blocks, in the order and under the condition of `FB890` (`post+0x2270 == 0`); the tone-map exposure write-back of §1.1 matters only with auto exposure (never). **Scratch object (rungs V0 and V1)**: for the three first converters a zero-filled 0x2400-byte object stands in for `post`, which is safe because they touch, outside the globals block, only `E4070`: `post+0x20F8` and `+0x2278` (**assigned**, so a change flag is cleared as well as raised), `E4460`: `post+0x2220…0x225F`, `E42A0`: nothing of the object; they also read the quality object (`[GXRS+0x5360]+0x10..0x1C`, for the DoF cap) and the factor object (`[GXRS+0x5368]+0x68`, `+0x50`): `E4460` dereferences `[GXRS+0x5368]` after checking only `GXRS`, so **call it only when `+0x5368 ≠ 0`** (otherwise leave its globals at zero and log). `E4700` writes `post+0x21E0…0x221F` and `E4890` reads `post+0x2271/+0x2272` and more (§1.4): `E4890` runs on the **real post object** (`E4700` only writes `post+0x21E0..0x221F` and reads nothing of `post`, so it may run on the scratch object too, [FFX-WAVE1-READY](FFX-WAVE1-READY.md) §4.1; the library still needs the real one once the host exists) `[[pipeline+0x4DD8]+0xCB8]` (the engine wrapper, not "the library's configuration object"), i.e. after the host exists. The row `native_yebis_globals` digests the 0x370-byte globals block (V1: it is the observable, §7) and, once the library is up, the fields `S+0x6E8` (exposure), `+0x6EC` (gamma), `+0x6F0` (tone type), `+0x70C` (filmic contrast).

### 4.5 The recorder HAL and the library host (YB-6, YB-7)

As YEBIS-HAL-READY: the native device keeps the 0xA50-byte original layout with a vtable copy whose **29 native slots** are replaced (§2 of that note); in the recorder model (YEBIS-D3D12-DESIGN-NATIVE §2.1) each `Draw` becomes a `YebisDrawRecord`, creators 60 to 67 return virtual resource requests with stable ids, and nothing touches D3D12 during the library's CPU pass.

`YebisHost::Open` is **the least specified step of the trunk and starts with an observation commit (YB-6b)**, because `FUN_1400E2B20` is not the thin wrapper the earlier notes implied (verified, asm `1400E2B20…E2B6B` and the decompile): it takes one argument (the post object in `RCX`), latches `post+8` into `144963858` once (guard `144963860`) and calls `1416CE8B0`; it calls slot `+0x50` of `post+8` as an allocator (`alloc(0x10000, 0x10)`, line 269), so `post+8` is the engine's GX context wrapper and **not** the five-slot `IPfxContext`; it creates the library through `1416CC930` into `post+0x21D0`; it builds the library flags and the quality arguments from `144961F78…FB4`, `post+0x2270`, `FB8` and `FBC` (so **`E4070` must run on the same post before `Open`**); it passes to `1416C87D0` the context `**(_DAT_144961ab0[2]+8)` (the engine's GX context, lines 166 to 177), writes that context's `+0x4F8`, `+0x538`, `+0x59D`, and creates GX resources at `post+0x28…0x20E0` through `1419254F0`, `141925830`, `141925B10` on `*_DAT_144961ab0` (lines 181 to 386). Two routes, decided by the observation: (a) **call `E2B20` unchanged** on the real post with the engine's GX context pointer replaced by a native token whose vtable slots the engine functions above reach (the GX creators `1419254F0…` would then need native equivalents, the same set the lighting and AO work already stubs), or (b) **re-implement the tail of `E2B20`** natively and call `1416C87D0` directly with a native `IPfxContext` (vtable `142B4C4A0`, five slots, note HAL-READY §1). YB-6b records, with the real engine and no substitution, the GX context pointer, `post+8`, the arguments of `1416C87D0` and the list of GX resources `E2B20` creates (row `native_yebis_open`); YB-7 implements the chosen route. Per frame (after `Open`): the converters (§4.4), then the driver `FUN_1400E69A0` and `BeginPostEffectScene`/`EndPostEffectScene` around the native scene copy, then the GX compute steps (`LutCopy2DTo3DCs` when `[144962228]` is set, before; `CustomDofLutCopy` after); a quality or surface change is a library re-initialisation (note, design §9). **Record-only mode** (`yebis-record-only.flag`, the first state of YB-7): the host stops after `EndPostEffectScene`; the recorder holds the frame's draws and nothing is encoded. The record is `{technique name, VS/PS SHA-256, input layout stride, render-state index, up to 8 target ids with size and format, SRV ids with sizes and formats, sampler modes, the used `$Globals` bytes, vertex range}`.

### 4.6 Encode and the PSO set (YB-8)

As the design note §2.3 and the HAL note §4/§5: PSO key `{VS blob, PS blob, input layout, render-state index, target formats, no depth, TRIANGLE}`, built on first use and **prewarmed from the world set of §2** (about 30 PSOs); root `b0` (the `$Globals` block, `round_up(highest used offset, 256)` bytes from the frame arena), SRV table `t0…t16`, sampler table `s0…s15` filled from the 16 YEBIS states (YEBIS-NATIVE §5.2; ten have no bank row), a vertex-stage table for `t0…t3` (rare). Barriers per draw from the draw graph (bound targets `RENDER_TARGET`, sampled `PIXEL_SHADER_RESOURCE`, persistent targets never aliased). In V3 every target is a scratch resource and the final draw writes a private LDR scratch.

### 4.7 Frame order, output and the gamma pass (YB-9)

`RenderNativeModels` after the velocity (`SP:1601`) and before the composition (`SP:1604-1615`): (a) `PointSampleColorClamp1` into `YScene` (RGBA16F, scene size, `SS_ClampPoint` = bank row 3), (b) the library draws (record then encode), (c) the DoF table compute for the next frame. **When the output is valid** (V4, `yebis-output.flag`) the composition draw is skipped and the library's last draw writes the 8-bit LDR target, which is `DisplayStage::Target()` once the colour path (GAMMA-COLOUR-PATH-READY) exists and the back buffer before; then `GlowStage::Record` (GLOW-WAVE1-READY §5.5), the UI, the gamma pass. When the host is faulted, disabled or the draw record is empty, the composition is drawn as today (graceful fallback, one row). The tone map's own `pow(·, 1/Gamma)` makes the LDR display-referred, which is what the glow adds to and the gamma curve (identity at brightness 5) expects; with `no-yebis.flag` the sRGB encode of the composition stays.

## 5. Commits (each builds, passes the unit tests, is reversible alone)

| # | Commit | Behaviour change | Flag | Test |
|---|---|---|---|---|
| YB-1 | **typed accessor and the three builders** (§4.1, §4.2), `yebis_bank_golden.h` (default, m31, m40, m50 id 0) | none | — | T1, T2 |
| YB-2 | **technique pins and blob source**: the world set of §2 as a generated header, `YebisPins::Check(name, vs, ps)`; the standalone tests (T9, T11) read the DXBC blobs from the executable at the `ps_offset` / VS offsets of `yebis-techniques.tsv` (file offsets into `DarkSoulsIII.exe`, path as an argument), since the library is not running there | none | — | T3 |
| YB-3 | **inputs**: `NativeYebisInput`, `CollectNativeYebisInputs`, the copy, row `native_yebis_inputs` | none (no consumer) | opt-in `yebis.flag` | T4, live YB4 |
| YB-4 | **converters on a scratch object** (`E4070`, `E42A0`, `E4460` guarded), row `native_yebis_globals` | none (the globals block is read by nothing yet) | opt-in `yebis.flag` | T5, live YB2a/YB3a |
| YB-4b | the light-shaft (28-word record, blend, converter globals, goldens: [FFX-WAVE1-READY](FFX-WAVE1-READY.md) §4.1) and DoF builders and converters (`E4700`, `E4890`), the AA flag, feedback words and motion-blur feed, from `converter-words.tsv` and the exported blends | none | opt-in `yebis.flag` | T2b |
| YB-4c | the **colour-grading bank** (block, blend, globals and the LUT chain decoded in [FFX-WAVE1-READY](FFX-WAVE1-READY.md) §4.2; the LUT texture stays a live row) (index 4: lookup `17BEE0`, blend `14021C150`, the LUT-trigger word `144962228`, the three source-name strings): decode and builder, or, until done, force `144962224 = 0` and `144962228 = 0` with a log row and drop m31/m40/m50 grading from the V2/V4 exits (counts then lose the grading draw and the LUT copy) | none | opt-in `yebis.flag` | T2c |
| YB-5 | **GX programmes**: `PointSampleColorClamp1`, `LutCopy2DTo3DCs`, `CustomDofLutCopy` resolved by name with the pins of §2; the `CustomDof` producer re-expressed on native DoF settings (the existing `CustomDof` in `LP` is snapshot-driven) | none | opt-in `yebis.flag` | T6 |
| YB-6 | **recorder HAL** with a fake-library driver: object layout, vtable, 29 slots, creators, companions, record list; no engine library | none | — | T7 |
| YB-6b | **observation of `E2B20`** on the real engine (GX context, `post+8`, `1416C87D0` arguments, GX resources), no substitution | none | opt-in `yebis-observe.flag` | live row `native_yebis_open` |
| YB-7 | **library host, record-only**: `E4070` then `E2B20` by the route the observation chooses, driver, scene bind, record of the frame; requires YB-4b and YB-4c | none (nothing is encoded) | opt-in `yebis-record-only.flag` | T8, live YB1/YB2/YB6/YB17/YB18 |
| YB-8 | **GPU encode to scratch targets**: PSO set, root, arena blocks, descriptors, barriers, timestamps | none (scratch output discarded) | opt-in `yebis-gpu.flag` | T9, T10 |
| YB-9 | **output**: scene copy, last draw into the LDR target, composition skipped, fallback, the order with glow, UI and gamma; after G0-4 (the colour path), because before it the target would be one of the swap chain's rotating back buffers held in `PRESENT`, or else a private LDR target plus a copy | **the tone-mapped image appears** | opt-in `yebis-output.flag` | T11, T12, live YB7/YB9, H1 to H4 |
| YB-10 | **development aids**: `yebis-test-scene.flag` (a synthetic gradient scene replaces the lit HDR), `yebis-draw-list.flag` (dumps the record), `yebis-factor.flag` | none unless present | the three flags | T12 |

Order: YB-1 → YB-2 → YB-3 → YB-4 → YB-4b → YB-4c → YB-5 → YB-6 → YB-6b → YB-7 → YB-8 → YB-9 → YB-10. **Every flag is opt-in** (a file next to the module enables the rung) until the rung's exit holds at the five zones (§7 rule 1); `no-yebis.flag` keeps the meaning "never create anything". YB-1..YB-6b need no library; the shared `GroupWord` comes from GL-2 (one definition); the shared files (`GB`, `SP`, `MF`) are edited by other waves, so each edit is at a block named in §3/§4.

## 6. Tests and goldens

| # | Test | Where | Pass |
|---|---|---|---|
| T1 | `yebis_blocks_test` (CPU): `BuildYebisBlocks` for the default file and the zone files at id 0 equals `yebis_bank_golden.h` (generated from `bank-blocks.tsv`: Tone 35, Bloom 42, Setting 19 words per file; default, m31, m40, m50 committed, the other zones checked from the evidence file by argument) | `Build-GameBootstrap.ps1` | every word equal |
| T2 | the blends: weight 0 → A, 1 → B (ε snap), 0.5 between m31 and m40: the B-copy sets {2, 9, 28, 29, 34} / {8, 9, 13, 27, 31, 33, 36, 39} / Setting, the lerp of the others in `float`, word 4 of the Bloom block lerped as a float; `AutoExposure` ≠ 0 refused | `yebis_blocks_test` | exact |
| T2b | the same for light shafts, DoF, AA flag and feedback after YB-4b, against the exported blends | same | exact |
| T2c | the colour-grading bank after YB-4c, against `14021C150` | same | exact |
| T3 | pins: the 82 rows of `world-techniques.tsv` against the header; a flipped byte refused; the shared PS of `TexCoordMadT1_S4`/`MadT1_S4` and `S2` | `yebis_pins_test` | equal |
| T4 | `native_yebis_inputs` row: well-formed; `no-yebis.flag` gives no block; world and menu views | pilot row check | as specified |
| T5 | **converter round trip**: the three first converters on a scratch object write the globals as the converter-words table says (`E4070` words → `F78…`, `E42A0` word 0 → `144962008`, `E4460` word 24 × the glare factor …), flags `+0x2278/+0x20F8` assigned (set on a change, cleared otherwise), `DepthOfFieldQuality` capped at `quality+0x14`, `E4460` skipped when `[GXRS+0x5368] = 0` | `Test-YebisConverters.ps1` in a pilot (needs the image; skipped without) | equal |
| T6 | the GX programmes resolve by name and match the pins of §2; `CustomDof` cb bytes against a golden for two DoF settings | `yebis_stage_test` | equal |
| T7 | the recorder with a **fake library** replaying the H1 list (m40: 42 draws) and the H4 list (m31: 48): record order, target ids and sizes, creators returning stable ids, wrapped surfaces, `Clear`, `Draw*` without D3D12 | `yebis_hal_test` | equal |
| T8 | record-only on the real library at the five zones: draw counts 48 / 36 / 36 / 41 / 36 (m40 41), technique SHA-256 against the pins, sizes and formats of H1/H4 and YEBIS-TECHNIQUES §4, 600 frames stable, no allocation growth, a resize and an area change | live (YB1/YB2/YB6) | equal to the witnesses |
| T9 | PSO set: every observed technique builds with the debug layer clean (no message of severity WARNING or worse; the renderer treats them as errors); `yebis_psos_created` 0 after warm-up | `yebis_stage_test` (WARP) and live | clean |
| T10 | scratch encode: GPU timestamps per group, barriers batched, no aliasing of the persistent targets, resize | live | recorded |
| T11 | **analytic tone map**: a synthetic gradient scene with the zone constants (exposure, contrast, ColorS, gamma 2.2) through the real `TonemapHDR` technique equals the CPU formula of §1.5 within 1/255 (also with dither off); brightness 10 and 0 change only the gamma pass | `yebis_stage_test` with `yebis-test-scene.flag` (hardware or WARP) | within 1/255 |
| T12 | non-regression: with `no-yebis.flag` or any rung's flag off, the idle High Wall frame digest equals the pre-YEBIS digest; menu views unchanged | pilot | equal |

## 7. The global validation strategy

YEBIS is one library call: from the first draw the whole chain runs (the earlier notes' design §7 risk 1), so no stage can be switched on alone. The strategy therefore climbs by **observable**, each rung proving one more property with the previous ones fixed, and no rung changes a pixel until V4:

| Rung | What runs | What it proves | Observable and exit | Rollback |
|---|---|---|---|---|
| **V0** | pure code | the bank blocks and blends are the original's: records, permutation, B-copy sets | T1/T2/T3 equal; `bank-blocks.tsv` for the seven zones + default | none needed |
| **V1** | the three first converters on a scratch object, with the real quality and factor objects | the blocks reach the globals as intended | T5; the **globals themselves**: `144962008` exposure (2.3 m30/m40, 2.2 m31, 1.52 m37, 2.0 m45/m50/m51), `14496200C` gamma 2.2, `144962010` tone type 2, `144962018` filmic contrast (0.15, 0, 0.3, 0.3, 0.1, 0.3), `F78…FBC` the Setting-Info words with the DoF cap, `144962224`; rows YB2a/YB3a. (The library state `S+0x6E8…` and the draw constants only exist from V2: rows YB2/YB3.) | `yebis.flag` off |
| **V2** | the real library's **CPU path** on the recording HAL (no GPU) | `CPostEffect` init and the driver are right: draw count, order, techniques (SHA-256), sizes, formats, persistent targets, first-frame and resize behaviour | T8 against H1/H4/YB1/YB5/YB6/YB17/YB18 at the five zones; the draw list of a frame is hash-stable on a still scene (YB13) | `yebis-record-only.flag` stays on; `no-yebis.flag` |
| **V3** | the recorded draws encoded to **scratch targets**, output discarded | every technique has a PSO, the root and descriptor model is valid, the cost is known | T9, T10: debug layer clean 600 frames, 0 PSO creations after warm-up, GPU time per group, memory | `yebis-gpu.flag` off |
| **V4** | the last draw writes the LDR target; the composition is skipped | the image: tone map, glare, DoF, grading | T11 (analytic) in a synthetic scene; live H2 (mean luminance against the original at the m40 view), H3 (brightness curve), the five-zone draw counts again with output on, YB7/YB9/YB11 | `yebis-output.flag` off restores the sRGB composition |
| **V5** | defaults on | stability across zone loads, resize, teleport, menus, the glow and UI order | 30 minutes of the pilot loop, no debug-layer message, `prepare_ms`/`render_ms` within the budget | flags |

Rules: (1) a rung's flag is **opt-in** (a file next to the module) until its exit holds at the five zones plus m30/m40 for the grading and tone variants; only then does it become the default; (2) V2's comparison list is **static truth** (the witnesses are in the repository notes), so it needs no original run; (3) V4 is the first rung that needs an original-side image (H2) and a human look; (4) the library never runs unless V1 holds **and the feed is complete** (YB-4b light shafts, DoF, AA flag, feedback; YB-4c grading): an incomplete feed would show as a wrong draw list at V2 and be misattributed; (5) a fault anywhere (exception, device removal, programme refusal) disables the host for the session and the composition draws as today.

## 8. Live witnesses (rows; none taken)

| ID | Rung | Read | Expected |
|---|---|---|---|
| YB1 | V2 | the draw list per zone between the scene copy and `CustomDofLutCopy` | m31 48, m37 36, m45 36, m50 41, m51 36, m40 41 (42 draws), same order, sizes, formats |
| YB2a | V1 | the globals `144962008/0C/10/18`, `F78…FBC`, `144962224` after the converters | the zone values of §7 V1 |
| YB2 | V2 | `S+0x6E8/+0x6EC/+0x6F0/+0x70C` after the first frame | exposure per zone, 2.2, 2, filmic contrast per zone |
| YB3a | V1 | `T = (T.x, T.y)` from the filmic contrast with the formula of §1.5 for the eight zone values | 2.3985/0.217, 2.1100/0.12746, 1.0/0, 1.9697/0.09053 (checklist §4.1) |
| YB3 | V2 | the recorded `$Globals` of the last tone-map draw | `c208 = T`, `c202.x = 0.4545` |
| YB4 | V0/V1 | pipeline bit 10, `[144961F70]`, the factors `[[GXRS+0x5368]+0x50/+0x68/+0x98]` and `quality+0x08/+0x14` per level | bit 10 set in the world; factors 1.0 at the top level |
| YB6 | V2 | the first frame after a load or teleport | no history read |
| YB7/YB9/YB11 | V4 | the DLAA input, brightness 0/5/10, `slots=2` against `slots=1` | as the design note |
| YB17/YB18 | V2 | the DoF draw list at each level; `[144961FB8]`, `[144961FBC]` | corrected values of design §9 |
| **YB19** | V0/V1 | the engine's own record for a key, when the bank is populated natively (`[root+0x4618]` through lookup `1638C0`), against the native block | equal (also decides the fallback record when the id is absent) |
| **YB20** | V2/V3 | the depth the library reads (`R24G8` in the original, `D32_FLOAT_S8X24` natively): the `…_RVALUE` technique and the LUT conversion | same factor; else a native depth copy to `R24` (open) |
| **YB21** | V1 | `E4070`'s word 20 target `144962004` and the Setting-Info bytes `+0x60/+0x64` | `SceneLuminanceScale` = word 20 |
| **YB22** | V2 | which view takes the Setting-Info/Tone-Map/Bloom lookups with the world keys (menu views) | the main view only runs `E5110` |

## 9. Risks

1. **The library cannot be split**: handled by the ladder (§7); the first image already has DoF, glare and grading.
2. **Auto exposure** is off in every shipped file; the builder refuses it (`AutoExposure` ≠ 0 would need the stored-exposure words 0x35/0x36 and the read-back).
3. **Depth format and readable copy** (YB20): a missing readable depth in the library's expected channel changes the DoF factor; the main depth's `R32_FLOAT_X8X24` SRV is the candidate.
4. **Lazy programme creation** hitches the first frames: the PSO set is prewarmed from §2 (YB-8).
5. **Debug-layer warnings are fatal** (`SceneRenderer::Diagnostics`, `SP:782`) and **`Refuse()` exits the process**: every YEBIS failure is soft (row, composition fallback).
6. **Key blend of other groups** (colour grading bank `0x4650`, feedback, AA) is not decoded; the first rungs use their file values or defaults and the witness is the draw list.
7. **Shared files** (`GB`, `SP`, `MF`) are edited by other waves; anchors are block names.

## 10. Open points

The colour-grading bank (`0x4650`, index 4) and its group; the exact fallback record of each bank when an id is absent (YB19); the `SceneLuminanceScale` word (YB21); the depth channel (YB20); the exact `MadT1_S<n>` blur variants per zone (chosen by the library at run time, YEBIS-TECHNIQUES §5); the menu-view tone-map filter (P23, later); the AutoExposure read-back path; the second-tranche record layouts of light shafts and DoF beyond the named words.

## 11. Review log

Independent review by an Opus 5.5 reader (read-only; given the note and the evidence, not conclusions; it re-derived the group table, the three records and blends, the goldens for two files, the constants and the fingerprints, and checked the converters, the anchors and the ladder). No blocker for YB-1..YB-3; **5 important and 4 minor findings, all applied**. No problem found in: the group table (every bank, key, weight, lookup, blend and converter of `FB890` lines 302-470), the B-copy sets and the nine lerped floats, the `E4070` word-to-global map, word 20 = `SceneLuminanceScale` (corroborated by the `FB890` stack defaults), the goldens for `default` and `m40`, the constants and the 26 observed technique hashes and 5 GX programmes, and every native anchor.

| # | Severity | Finding | Applied |
|---|---|---|---|
| 1 | important | light shafts, DoF, colour grading, AA flag, feedback and motion blur were not fed; the grading trigger `144962228` and the LUT copy depend on bank index 4 (`FB890` 452-480, blend `14021C150`) | §1.1 row 4 decoded, §1.4 rewritten, YB-4b mandatory, new YB-4c, V2/V4 rule 4 |
| 2 | important | `E2B20` is not a thin wrapper (post+8 is the GX context wrapper; creates GX resources; `E4070` must run before) | §4.5 rewritten; new YB-6b observation commit; the two routes (a) call unchanged, (b) re-implement the tail |
| 3 | important | the V1 exit (library state, draw constants) cannot be observed without the library | V1 exit = the globals themselves (§7); YB2/YB3 moved to V2, YB2a/YB3a added |
| 4 | important | Bloom word 4 is a record-only default (1.0), the second `Enable` is record word 42 | §0.2, §1.2, §1.3, `Get-YebisBankBlocks.ps1` fixed, goldens regenerated |
| 5 | important | rungs were on by default before their exits held; `E4460` dereferences `[GXRS+0x5368]` unchecked | all rung flags opt-in; `E4460` guarded (§4.4, §5, §7 rule 1) |
| 6 | minor | converter facts: `E42A0` touches no object field, ranges, assigned flags, reads of quality/factor objects, `E4890` reads | §0.4, §1.4, §4.3, §4.4, T5 |
| 7 | minor | Tone Map tail layout shifted; word 34 inferred; AutoExposure gate words 9 and 48 | §1.2, §1.3 |
| 8 | minor | accessor/fallback wording, name trim, golden scope | §4.1 |
| 9 | minor | table scope, gate bit, `T.x` constants are data words, name count, zone coverage, superseded notes, YB-9 before G0-4, blob source, DLAA absence | §1.1, §1.5, §0.6, §5 (YB-2, YB-9), §7 |

## Exports and evidence

- Evidence (outside Git): `evidence/I-0068/queries/yebis-ready/` (`bank-blocks.tsv` the goldens, `converter-words.tsv`, `world-techniques.tsv`, `b1/` the lookups, defaults and blends of Setting Info / Tone Map / Bloom, `b2/` the light-shaft/DoF converters and blends, `dxbc/` the GX programme disassembly).
- Tools (committed): `Get-YebisBankBlocks.ps1`, `Get-YebisConverterWords.ps1`, `Get-YebisWorldTechniques.ps1`; `Invoke-Map.ps1` and `Read-Dxbc.ps1` unchanged; PE reads for the constants.
- No native code changed.

## Implementation log (thread « Vague 1 producteurs W5 », branch `claude/friendly-feynman-70213e`, 2026-10-13: W5f = YB-1)

`Banks::GroupEntries` (every entry of a group at a key, in file order, short names without padding, the key rule shared
with `GroupWord`/`GroupFloat`), `native/world/yebis_blocks.*`: `ToneBlock` (35 words; AutoExposure refused), `BloomBlock`
(the engine permutation with the record-only word 4 = 1.0), `SettingBlock` (the 18 type-3 entries, word 20
SceneLuminanceScale) and the three blends (Tone Map B-copy {2, 9, 28, 29, 34}, Bloom {8, 9, 13, 27, 31, 33, 36, 39}, Setting
Info B with word 20 lerped). T1/T2 (`yebis_blocks_test`, run by the build): the blocks built from the eight decoded dumps
(default and m30, m31, m37, m40, m45, m50, m51, `evidence/I-0068/queries/yebis/zone-gparam`) equal `bank-blocks.tsv` word for
word (673 words); the blends between m31 and m40 (snaps, the B-copy sets, the float lerps, word 4); AutoExposure refused;
`GroupEntries` on the binary `default.gparam.dcx` equals its dump entry by entry for the three groups. The golden header
`yebis_bank_golden.h` of the sheet was not generated: the test reads the evidence file directly (it is in the store, like
the shadow goldens). Next: YB-2 onward with W10.

## Implementation log (thread « Source du soleil puis W6 », 2026-10-13: W10 YB-2, YB-3, YB-4 = rung V1)

**YB-2.** `native/world/yebis_pins.h`, generated from `world-techniques.tsv` (82 rows: technique, observed or candidate, the
vertex and pixel blob names and SHA-256), `FindTechniquePin`, `CheckTechnique`. T3 (`yebis_pins_test`, run by the build with
the executable): the executable's 3,534 DXBC containers (found by magic and size word) hold every pinned vertex and pixel
blob of the 82 techniques; the shared `MadT1` pixel blobs; a flipped byte refused. **YB-3/YB-4** (opt-in `yebis.flag`,
`no-yebis.flag` wins). `CollectNativeYebisInputs` (main view, beside the AO inputs): keys and weights of i = 0, 1, 3 from the
view context, `Banks::GroupEntries` for `Setting Info`, `Tone Map`, `Bloom`, the YB-1 builders and blends, then the genuine
converters `E4070`, `E42A0`, `E4460` (pinned) on a zero-filled 0x2400-byte scratch object in FB890's order, `E4460` only with
`[GXRS+0x5368]`; row `native_yebis_globals` (the keys, the three block digests, the 0x370-byte globals block digest, exposure,
gamma, tone type, filmic contrast, scene luminance, `144962224`, `F78..FBC`). No frame copy yet (no consumer before the host,
YB-7). Reference observer (`yebis-observe.flag`): rows `reference_yebis_converter` (the converters' input words and digests
and the same globals after the call). **Fix on the way (`Banks::GroupEntries`)**: at m50 the area's latest kind-7 layer is
`m50_00_0200`, which has none of the three groups; the original's records are `m50_00_0000`'s, so a group missing from the
bound file is now taken from the area's own file before `mapbase` and `default` (INFERRED rule for several layers,
witnessed at m50 only). **V1 exit, three zones** (Shebang saves; Reference1590/1592/1594 against Native1591/1593/1596): the
Setting Info, Tone Map and Bloom blocks are **bit-identical** to the original's converter inputs at m37 (`2AF8C7FA…`,
`17685E68…`, `5F83CF0E…`), m31 (`2AF8C7FA…`, `884F7914…`, `D48ACA23…`) and m50 (`A1766C96…`, `A6D724F8…`, `FFC7A803…`), and
the named globals agree (the 0x370-byte block itself was not compared: the scaled Bloom globals are inferred equal from equal inputs): exposure 1.52 / 2.2 / 2.0, gamma 2.2, tone type 2, filmic contrast 0.3 / 0.15 / 0.1, colour-grading word
1, the Setting words `F78..FBC` with the DoF cap; 0 debug errors, no refusal. Also the title view (keys {-1, -1}: B8D95C18…, 7CA93708…, AAC03E46… in both). Not covered: m45, m51, m30/m40 (the tone and
grading variants). Next: YB-4b (light shafts, DoF, AA flag, feedback, motion blur feeds),
YB-4c (colour-grading bank), YB-5, YB-6 (recorder HAL), YB-6b (observation of `E2B20`), then the host (YB-7).
**Review (fresh Opus 5.5, merge after fixes; applied).** The feed now runs under FB890's gate (context +0x50 bit 10 and
`[144961F70]`; Native1597: open in game, same blocks); with `yebis-observe.flag` the native converters do not run (their
calls would be logged as the original's); the JSON rows quote their hex words; a builder refusal is named. Recorded: (1)
**for YB-7**, `E4070` raises `post+0x2278/+0x20F8` only when its words differ from the current globals, so the scratch calls
of every frame consume that change signal: once the host exists the converters must run on the real post object (and not
on the scratch object as well), or a Setting Info change would not re-initialise the library; (2) the "area's own file"
rule of `GroupEntries` is inferred from m50 alone, and `FindGroupParam` (`GroupWord`, `GroupFloat`, SSAO, Glow) does not
have it: if a variant layer lacks the SSAO or Glow Filter group the original might use the base file where native uses
mapbase (no dump of `m50_00_0200` to check; the W5 zone witnesses found the AO and glow inputs equal at m50); (3)
`yebis_pins_test` checks that every pinned hash is a container of the executable, not which technique owns it, and the
header was generated from the TSV rather than checked against it; (4) `E4460` is skipped without `[GXRS+0x5368]` where
the original would run it with the default factor.

**YB-4b, first part: the light shafts (i = 9).** `yebis_blocks`: `LightShaftBlock` (the 28-word `GXLightShaftFilterParam`
block of FFX-WAVE1-READY 4.1, with the record-only words 9 = 0 and 10 = `0x3F7FFF58`) and `BlendLightShaft` (`14021D690`);
T2b in `yebis_blocks_test` (argument 4, `ffx-ready/bank-blocks.tsv`): the block equals the golden for default and the seven
zones (224 words) and the blend between m31 and m50 (B's words 0 and 9, the padding unwritten). The feed adds the key at
`context+0x4020`/`+0x4088`/`+0x40CC` and calls the genuine `E4700` (pinned) on the scratch object after `E4460` (it writes
only `post+0x21E0..0x221F`); the reference observer covers `E4700`. **Live** (Reference1598/1600, Native1599/1601): the
light-shaft block is **bit-identical** to the original's at m50 (`806AA655…`) and m31 (`EDE86597…`), with the three earlier
blocks still identical, and the globals `1449620A0` (enable 1 at both), `B0`, `B4`, `1449621BC` follow; at m31 the enable is
1 although `m31_00_0000` id 0 says 0: the record comes from the area's resident file, which the native lookup already takes,
so the "which record" question of FFX-WAVE1-READY 4.1 is answered at these two zones by the equality. Still to feed before
the library runs (rule 4): the depth of field (`E4890` needs the real post object: with the host), the AA flag, the
feedback words, the motion blur (`E4B50`), and the colour-grading bank (YB-4c).
**Review of YB-4b (fresh Opus 5.5; applied).** A comment had swallowed the row's `last=` statement (a `native_yebis_globals`
row every frame in Native1599/1601), restored (Native1602: 10 rows, same four blocks, 0 debug errors); the scratch object
and the buffer are `alignas(16)` (E4700 uses aligned 16-byte moves on both); `LightShaftBlock` refuses any other entry
width; the blend test adds distinct synthetic records. Limits of the live claims: the weight was 0 at both zones, so the
lerp path ran only in the tests; the reference observer does not log `1449620A0..F4`, so the globals were compared with the
golden natively, and the factor `[[GXRS+0x5368]+0x50]` is 1 natively (`B0` = w7), not confirmed on the original side; the
blend writes zero padding where the original leaves stale bytes (the digests matched live).

**YB-4b, second part: the anti-alias flag and the feedback words (FB890 lines 389-449).** `yebis_blocks`: `AntiAliasBlock`
(the 9 `Anti-Alias` entries in file order) and `BlendAntiAlias` (`140220B50`: words 0 and 1 from B off the 0-snap, words 2-8
lerped or B's at the 1-snap); `FeedbackBlock` and `BlendFeedback` (`140221640` as FB890 calls it: A when the weight is <= 0).
The feed reads the keys i = 7 and 5 and, under FB890's gate, writes `144962190` = Enable && `AntiAliasType` == 0, forced 0
by the quality object's `+0x08` or the factor `[[GXRS+0x5368]+0x98]` <= 0, then `14496218C` = the previous `144962178` and
`144962178..188` = feedback words 0-4. **The feedback record is not in file order:** Reference1603 (m37) gives `144962178..188`
= 0, 0, 0.5, 0, 0 where the file order put `FeedbackRate` 0.8 in word 4 (Native1604); the record has a record-only word 2 =
0.5 (FB890's stack default `0x3F000000`) and entries 2-6 move to words 3-7 (INFERRED from that one observation and the
8-word record against 7 entries); Native1605: `144962178..190` equal the original's, and the light-shaft block at m37
(`381BCD12…`) too. The AA flag is 0 in both (`AntiAliasType` 4 selects the GX DLAA, not a library stage). The
reference row reads `144962178..190` at the converters, i.e. FB890's values of the previous frame (steady here). Still to
feed: the depth of field and the motion blur (`E4890`, `E4B50`, with the real post object or after checking their reads),
and the colour-grading bank (YB-4c).
**Review of the AA and feedback feed (fresh Opus 5.5; applied).** Without the factor object FB890's factor is 1.0 (XMM7 from
`1400FB953`), not 0: fixed; the two writes no longer depend on the three other blocks being complete (FB890 writes them
whatever those are). Limits: at m37 the AA flag is 0 through `AntiAliasType` 4 (the quality and factor branches did not
run), `144962178` is constantly 0 (the order of the `14496218C` copy is not observable there), the weights were 0 (no lerp
ran live), and the feedback mapping pins only word 2 = 0.5 and `FeedbackRate` outside words 0-4 (Enable, Weight, UnitTime,
AspectRatio are 0 in every dump; words 5-7 never observed): INFERRED until the FeedBackBlur record's constructor is read.

**YB-4c, the integer part of the colour grading (FFX-WAVE1-READY 4.2).** `yebis_blocks`: `GradingBlock` (Enable,
LutSourceId, the two fade ids -1, progress 0) and `BlendGrading` (the integer table of `14021C150` for records without a fade
in progress: `w <= 0` {A0, A1, A1, B1, 0}, `w >= 1` {B0, B1, B1, B1, 1.0}, otherwise {B0, A1, A1, B1, w}); T2c: the five
globals at weight 0 equal the `Grading-globals` goldens of `ffx-ready/bank-blocks.tsv` for default and the seven zones, and
the three branches. The feed (key i = 4) writes `144962228..238` under FB890's gate. **Live** (Reference1610/1612,
Native1611/1613): equal to the original's at m37 (0, -1, -1, -1, 0) and m31 (1, 1, 1, 1, 0), weight 0 at both; 0 debug
errors. Not built: the names (`post+0x2100/2130/2160`), the texture fields (`1449622C8/D0`), the record's own timed fade
(`14017BE20`, not driven natively: a zone change cross-fades by the key weight) and the LUT itself, whose copy the library
host needs (row FF14): until then the host must keep the grading draw off (YB-4c fallback of 4.2).
**Review of YB-4c (fresh Opus 5.5; applied).** The table is a port, not the genuine `14021C150` call FW-8 recommended: it is
exact for records without a fade in progress, which is every native record (`14017BE20` never runs natively); the genuine
call would need constructed 0x248-byte blocks with strings. **YB-7 precondition** (written at the write site): these are the
original's values, `144962228` included, but the names and the texture fields `1449622C8/D0` are not built, so a library host
must force `144962228` and `144962224` to 0 (with a row) until the LUT source is fed (FF14). The test now also covers
`w <= 0` with distinct records.

## Implementation log (thread « Suite W10 YEBIS », 2026-10-13: YB-5)

**YB-5 GX programmes and the DoF table core.** `renderer/yebis_stage.h`: the three pins of section 2
(`PointSampleColorClamp1` ps 736, `LutCopy2DTo3DCs` cs 1040, `CustomDofLutCopy` cs 1856, SHA-256 of the whole container);
`AcquireNativeYebisGxPrograms` resolves them by name from the engine's pixel repository (`4792BF8`) and compute repository
(`4792748`), checks each pin, retries a missing one (row every 600th call) and disables them softly on a mismatch (rows
`native_yebis_gx_programmes_{resolved,unresolved,refused}`); the YEBIS feed calls it under FB890's gate (`yebis.flag`); nothing
records them before YB-8/YB-9. The build checks the three pins against the I-0063 programme cache (equal). `cbComputeCustomDof`:
the snapshot producer's body (I-0064, exact against the capture) is now `ds3light::CustomDofTable` on named inputs (LUT size,
`arg0+0xCC`, the library's near/far and lens words, the exposure, the DoF globals `144962244..26C` that the native feed now
writes through `E4890`, the four executable constants); the snapshot producer calls it. T6 (`yebis_blocks_test`): the core
equals a frozen copy of the pre-extraction body on three DoF settings (near/far custom ranges off and on, a zero setting
taking the `.rdata` fallback) with literal constants in that build (the worktree had no `game/` executable; the build now passes `DS3_EXE` and requires the executable's constants). The library-side inputs (Y+0x564.., the exposure) exist only
with the host (YB-7): the native producer is fed there. Live Native1614 (m37): `native_yebis_gx_programmes_resolved
{programmes: 3, pinned: true}`, the feed's blocks unchanged, 0 faults.

**YB-6b, first observation: the original's library traffic (Reference1615, m37, `yebis-hal-observe.flag`).** Pass-through
hooks on `E2B20`, the HAL device factory `170ACC0`, `E5110` (one frame of the chain), the effect commit `1775280` and the
device's `Draw`/`DrawIndexed` (`1709600`/`1709650`); rows `reference_yebis_{open,device,frame,draw}`. Observed: (1) the device
is created once, vtable `142B598C8`, its `+0x10` the D3D11 context handed to the factory; (2) `E2B20` runs three times on the
same post object; `post+8` has vtable `142990838` (the engine's GX context wrapper, as section 4.5 inferred) and **`post+0x21D0`
holds the `IPfxContext` (vtable `142B4C4A0`), created at the first call and kept**, not the `CPostEffect`; (3) every library draw
runs inside `E5110` (no draw outside it) and goes through the commit (draws = commits); (4) **m37 in the world: 46 draws per
frame, the same technique list at frames 1200, 2400 and 3600** (DoF 10 draws: the depth factor `…TypeCastLut_RVALUE`, `MadT1_S2`,
`MadT1_S1`, three `FrontBlurBlendFactor` + `MadT1_S5/S9` blurs, `CompositeBlurLevelsFrontBlur_TS1_3`; `Alpha_R`; **motion blur
3 draws** (`MotionBlurMakeCameraVelocityOverwrite_R` at 640x360, two `MotionBlurVelocityDirDepthInnerscreen_SM3_T2_S5`);
glare source `MadT1_S1` x2; bright pass at 210x177; the bloom pyramid in R11G11B10 (210..14 levels with `MadT1_S3/S5` pairs and
`MadT1_S4` ups); `MadLumGammaT6`; light shafts (`LightShaftMask_SM3`, `TexCoordMadT1_S4` x4, `TexCoordMadT1_S3`); `MadT2`;
`MadT1_S4` to 420x354; the tone map `TonemapHDR_Dither_Exposure_Matrix_Gamma` into an R8G8B8A8 1280x720 target). The
witness row YB1's "m37 36" is corrected to 46: the motion blur (option on in this configuration) and the light shafts are part
of m37's list. (5) The title view (frame 300) runs 4 library draws: `Alpha_R`, the two motion-blur steps and
`TonemapHDR_Dither_Exposure_Gamma`. Every draw is `DrawIndexed` of 3 indices. Render-target descriptions `+0xA8` give the DXGI
format (10 R16G16B16A16_FLOAT, 26 R11G11B10_FLOAT, 28 R8G8B8A8_UNORM) and the size. This list is the V2 oracle for m37; the
other zones are rows to take with the same flag. Route of section 4.5: the library object the engine keeps is the
`IPfxContext`, so route (b) (a native `IPfxContext` handed to `1416C87D0`, `E2B20`'s tail re-implemented) is the one the next
commits follow, the observation of `1416C87D0`'s arguments being taken when the native context is written.
