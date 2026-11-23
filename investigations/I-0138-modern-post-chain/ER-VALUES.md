# ER-VALUES: Elden Ring 2.7.1 post-processing values and tone-map maths

Copied from `evidence/I-0138/artifacts/er-values/ER-VALUES.md` (2026-11-12, a Sonnet worker for the workstream 11 pilot, on Thomas's
agreement of 12:19; its scripts and value tables stay there, the extracted files and the shader disassembly outside Git and
outside the evidence store). Its statuses are the worker's; the pilot's reading is in [STUDY](STUDY.md) §5.2.

Date 2026-11-12. Written by a Sonnet worker; NON-CANONICAL until the pilot re-reads it. Elden Ring's own files were read, read-only, on Thomas's PC with his agreement (12:19 Paris); nothing launched (no game, no build, no Ghidra), nothing published, no raw game file or shader bytecode in this folder (extracted files and disassembly are in `C:/Users/User/Tools/DS3-Decompilation/er-post/`). Tags: OBSERVED = read in a file (named), INFERRED = my reading.

## Ten answer lines

1. OBSERVED: 319 draw-parameter files exist as `/param/drawparam/<name>.gparam.dcx` in Data0 (`default`, `mapbase`, 70 `mAA_BB_0000`/`sAA_BB_0000` bases, plus `_0100`..`_0900` variants, and m60/m61 `_00XX` regional/DLC variants such as `m60_00_0010`, `m61_00_0050`). No `remobase`. Parsed with my own Python parser (header `filt`, game = 5 like Sekiro, header 0x54; ids carry a time-of-day float).
2. OBSERVED: ER ships ToneMapMode 8 (ReinhardEx) in 912 of 928 gameplay and cutscene values; mode 7 (Sensitometric) only 16 values, all in the cutscene files (s12_02, s19_00_0015, s60_38/49/51 variants) and `m00_00_0000`. Filmic (0) is never used.
3. OBSERVED: gameplay (m*) Exposure median 3.75, p10..p90 3.2..4.2, absolute 4.7 (one dungeon outlier -3); overworld m60_00_0000 follows a time-of-day curve (keys at 0, 4, 8, 11.5, 12, 15, 21, 22.5 h): 3.9, 3.5, 3.35, 3.48, 3.57, 3.2, 3.2, 3.3. ReinhardContrast median 1.4 (p10..p90 1.3..1.55); ReinhardWhiteBalance 1.0 in 171 of 174; the "FilmicContrast" row (`ToneMap-Contrast`) is 0.5 in 163 of 174 (irrelevant to mode 8).
4. OBSERVED: auto exposure is enabled in 133 of 173 gameplay entries (0 in 40 of 173 entries, among them default/mapbase and several areas such as m18, m19, m25, m35). Typical: Middle Gray -7 (range -8..-6; -2 where off), Adaption Min -1.5, Adaption Max 0.4..0.5, Center Weight 7, Light/Dark adapt speed 0.25 (overworld 0.1, interiors 0.2).
5. OBSERVED: colour saturation, hue, value and contrast are neutral: ColorS 1 (278 of 279), ColorH 0, ColorV 1 (a few 1.2), Contrast 1; ColorShift/ColorScale (1,1,1,0) in 291 of 299 entries. The look is not in these scalars: it is in the 3D LUT (ColorGrading Enable 1 in 223 of 252 gameplay entries, LutSourceId 1/2/4/5 for the overworld by time of day, 5000..5003, 7800/7804, 1000..2102... elsewhere) and in the Reinhard curve.
6. OBSERVED: bloom group appears as an override in only about 18 gameplay files (mostly the overworld variants): threshold 0.2..1 (median 0.3), luminance 1..1.5 (median 1.45), alpha 0.25..1, levels 5..6, Smooth 1, GaussianRadiusScale 1, Saturate Ctrl 0, FeedbackWeight 0.8. Everywhere else the base values come from default/mapbase (INFERRED inheritance): default id 0 threshold 0.5, luminance 1, alpha 1, 7 levels; mapbase 0.5..1 / 5 levels. Glow filter: 1 gameplay file only.
7. OBSERVED: vignette is on everywhere: Power median 0.5 (p10..p90 0.41..0.55, 0 in a few), Range 1.2..1.3, Shape 0.2, FovDependence 0. Chromatic aberration: Enable 1 in 169 of 173 gameplay entries, lateral dispersion 0 (red-green) median 0.0012 (0.0004..0.0018), 1 (blue) median 0.0001.
8. OBSERVED: depth of field is rarely overridden in gameplay (12 values, m61 DLC files): off by default (`default` id 10/11 Enable 0), when on near 0, far 3000..4000, F 5.6, Av 1, film 0.024. Motion blur: Enable 0 almost everywhere (3 gameplay entries, 1 enabled: intensity 0.8, max length 0.02, shutter 100, 8 samples). Light shafts: enabled in 598 of 689 gameplay entries.
9. OBSERVED (shader `ToneMap_TabledToneMapLUTPS`, DXIL, SM 6.0): order is: colour = scene * g_ToneMapSceneLumScale (exposure) + bloom * g_GlareLuminance.x (bloom ADDED, linear, BEFORE the curve) -> 3x4 affine colour matrix (g_mtxColorMultiplyer) -> max(0) -> u = c/(c+0.2) -> 1D table lookup per channel (the baked curve, which also applies the 1/gamma) -> x vignette (smoothstep^4 on the display-encoded colour) -> ^g_ToneMapParam.z -> 3D LUT (16^3, input encoded as v*15/16+1/32) -> output. No dither, no sRGB/PQ in the SDR branch; an HDR-only branch (flag g_bEnableFlags.z) follows the LUT.
10. OBSERVED (`WriteReinhardExToneMappingTablePS`): table(u) with x = 0.2u/(1-u): t = (g_ReinhardParam.y * x)^g_ReinhardParam.x; y = t/(t+1); out = y^(1/g_ToneMapParam.y). The sensitometric table is NOT DS3's formula: y = A + c(B-A), A = 1-2^(-x), B = (1-exp(-4.4567x))^3, c = g_ToneMapParam.x, out = y^(1/g_ToneMapParam.y). Filmic is the Hejl/Burgess-Dawson curve x'(6.2x'+0.5)/(x'(6.2x'+1.7)+0.06) with x' = max(x-0.004,0), out = y^(2.2/g_ToneMapParam.y).

## 1. Where the values are (OBSERVED)

- Name hash search against the five BHD5 indexes (Data0..3, DLC) with my own script `Find-Names.ps1` (indexes decrypted once, cached in er-post/index). All 319 hits are in Data0. `/param/drawparam/default.gparam.dcx`, `mapbase.gparam.dcx`; `m00_00`, `m10_00/01`, `m11_00/05/10/70/71`, `m12_01..09`, `m13`, `m14`, `m15`, `m16`, `m18`, `m19_00/70`, `m20_00/01`, `m21`, `m22`, `m25`, `m28`, `m30`, `m34_10..15`, `m35`, `m39_20`, `m40..43`, `m45_00..02`, `m60_00`, `m61_00`; cutscenes `s12_01/02/06`, `s14`, `s15`, `s16`, `s19`, `s20_00/01`, `s21_00/01`, `s28`, `s34_10/11`, `s35`, `s60_37/38/42/43/49/51/54`, `s61_44`. Sweep: mAA/sAA 00..99 x BB 00..99 x `0000`, then every `_0000..9999` suffix for the 70 bases found. Full list: `r4.txt`.
- Variant suffixes: `_0100`..`_0900` (area variants), `m60_00_00XX` (10, 20, 21, 30, 31, 40..42, 100, 110, 142, 200, 300, 400..440, 510, 600, 800..842, 900..942: overworld variants), `m61_00_00XX` (DLC: 50, 55, 60, 65, 70, 75, 78, 150..978). Which suffix maps to which place or condition was not read (needs the MSB/map tables).
- Format: decompressed GPARAM, UTF-16 magic `filt`, game = 5 (Sekiro layout), header size 0x54, same group/param/value/id blocks as DS3, plus a float "time of day" beside each value id. Value ids are lighting-set ids (0, 10, 20 ... 150, 900...): `default` has ids 0/10/11, most bases only id 0. Parser validated on every file (319 of 319 parse; names read in clear UTF-16 are consistent): `parse_gparam.py`.
- Time keys seen: 0, 4, 8, 11.5, 12, 15, 16, 21, 22.5 (hours). 166 files have a single key; 74 have 7 keys (the standard day curve).
- Files are overrides: groups absent from a file (e.g. Bloom in `m60_00_0000`) are INFERRED to be inherited from `default`/`mapbase`; not verified in the executable.
- Raw tables: `er-gparam-values.tsv` (246,563 value rows: file, group, param, type, id, time, value), `er-groups-params.txt`, `tab-per-file.md`, `tab-per-area.md`, `tab-global.md`, `tab-gameplay.md` (all in this folder).

## 2. Groups present (OBSERVED, `er-groups-params.txt`)

LightSet (26 params), SSAO, Setting Info[Yebis], Tone Map[Yebis] (Exposure, ToneMapMode, Contrast, ReinhardContrast, ReinhardWhiteBalance, AutoExposure x8, Chromatic Aberration Enable + ToneLateralDispersion[0/1], Vignetting Power/Range/Shape/FovDependence (+AspectTrackingMax and Color in one file), ColorH/S/V, ColorShift/ColorScale, Contrast, LocalExposure x7 in one DLC file), Bloom[Yebis] (Enable, NumLevels, Luminance, Threshold, Alpha, GaussianRadiusScale, Saturate Ctrl, Smooth, FeedbackWeight), LightShaft[Yebis] (22 params), Anti-Alias (5 files), MotionBlur (11 files), FurBlur, FeedBackBlur, ColorGrading[Yebis] (Enable, LutSourceId, FadeTime), ShadowParam, Glow Filter (19 files), Lens Simulation (DoF, 98 files), RayTracing (47 files), VolumetricFog, FogVolume, EnvironmentConfig, WaterInteraction, Wet, SSR.

## 3. Distributions

### 3a. Gameplay files only (m*, every id and time key; n counts value rows, not areas)

| parameter | n | min | p10 | median | p90 | max | most common |
|---|---|---|---|---|---|---|---|
| Exposure | 636 | -3 | 3.2 | 3.75 | 4.2 | 4.7 | 4.2 x71; 4 x65; 3.9 x63; 3.6 x56 |
| ToneMapMode | 177 | 7 | 8 | 8 | 8 | 8 | 8 x176; 7 x1 |
| FilmicContrast(ToneMap-Contrast) | 174 | 0.4 | 0.5 | 0.5 | 0.5 | 0.5 | 0.5 x163; 0.4 x11 |
| ReinhardContrast | 635 | 1.1 | 1.3 | 1.4 | 1.55 | 2 | 1.3 x178; 1.35 x108; 1.4 x100; 1.5 x99 |
| ReinhardWhiteBalance | 174 | 1 | 1 | 1 | 1 | 2 | 1 x171; 2 x2; 1.7 x1 |
| AE Enable | 173 | 0 | 0 | 1 | 1 | 1 | 1 x133; 0 x40 |
| AE LightAdaptSpeed | 173 | 0.1 | 0.1 | 0.25 | 1 | 2 | 0.25 x79; 0.2 x35; 1 x20; 0.1 x19 |
| AE DarkAdaptSpeed | 173 | 0.1 | 0.1 | 0.25 | 0.5 | 2 | 0.25 x79; 0.2 x35; 0.5 x20; 0.1 x19 |
| AE MiddleGray | 473 | -8 | -7.8 | -7 | -6 | -2 | -7 x133; -7.8 x74; -8 x44; -6.2 x42 |
| AE CenterWeight | 173 | 1 | 3 | 7 | 7 | 7 | 7 x112; 4 x38; 1 x13; 3 x9 |
| AE AdaptMax | 175 | 0.01 | 0.4 | 0.5 | 8 | 8 | 0.4 x79; 0.5 x33; 1 x25; 8 x20 |
| AE AdaptMin | 438 | -8 | -3 | -1.5 | -0.8 | -0.01 | -1.5 x236; -1 x31; -2 x29; -3 x29 |
| CA Enable | 173 | 0 | 1 | 1 | 1 | 1 | 1 x169; 0 x4 |
| CA Disp0 | 173 | 0 | 0.0004 | 0.0012 | 0.0018 | 0.004 | 0.0015 x46; 0.001 x39; 0.0006 x19; 0.0014 x12 |
| CA Disp1 | 173 | 0 | 0 | 0.0001 | 0.0003 | 0.002 | 0 x53; 0.0001 x43; 0.0002 x32; 0.0003 x30 |
| Vig Power | 568 | 0 | 0.41 | 0.5 | 0.55 | 0.65 | 0.5 x97; 0.52 x80; 0.48 x57; 0.43 x37 |
| Vig Range | 224 | 0.01 | 1.2 | 1.25 | 1.3 | 1.5 | 1.2 x83; 1.3 x82; 1.25 x37; 1.5 x6 |
| Vig Shape | 224 | -0.19 | 0.17 | 0.2 | 0.2 | 0.4 | 0.2 x189; 0 x12; 0.4 x10; 0.17 x5 |
| Vig FovDep | 173 | 0 | 0 | 0 | 0 | 1 | 0 x171; 0.05 x1; 1 x1 |
| ColorH | 279 | 0 | 0 | 0 | 0 | 1 | 0 x262; 1 x17 |
| ColorS | 279 | 0.75 | 1 | 1 | 1 | 1 | 1 x278; 0.75 x1 |
| ColorShift | 299 | - | - | - | - | - | 1,1,1,0 x291; 1,1,1,1 x5; 0.613185,0.635741,0.677553,0 x2; 0.984314,1,0.996078,1 x1 |
| ColorScale | 334 | - | - | - | - | - | 1,1,1,0 x327; 1,1,1,1 x5; 0.984314,1,0.996078,1 x1; 5 x1 |
| ColorV | 279 | 0.6 | 1 | 1 | 1 | 1.5 | 1 x262; 1.2 x15; 0.6 x1; 1.5 x1 |
| ColorContrast | 279 | 1 | 1 | 1 | 1 | 1 | 1 x279 |
| Bloom Enable | 18 | 1 | 1 | 1 | 1 | 1 | 1 x18 |
| Bloom NumLevels | 16 | 5 | 5 | 6 | 6 | 6 | 6 x9; 5 x7 |
| Bloom Luminance | 16 | 1 | 1 | 1.45 | 1.5 | 1.5 | 1.5 x8; 1.4 x6; 1 x2 |
| Bloom Threshold | 53 | 0.2 | 0.25 | 0.3 | 0.5 | 1 | 0.3 x15; 0.25 x12; 0.5 x7; 0.4 x6 |
| Bloom Alpha | 53 | 0.25 | 0.5 | 0.7 | 1 | 1 | 0.7 x11; 0.8 x11; 1 x10; 0.5 x8 |
| Bloom GaussRadiusScale | 16 | 1 | 1 | 1 | 1 | 1 | 1 x16 |
| Bloom SaturateCtrl | 16 | 0 | 0 | 0 | 0 | 0 | 0 x16 |
| Bloom Smooth | 16 | 1 | 1 | 1 | 1 | 1 | 1 x16 |
| Bloom FeedbackW | 16 | 0.8 | 0.8 | 0.8 | 0.8 | 0.8 | 0.8 x16 |
| LS Enable | 689 | 0 | 0 | 1 | 1 | 1 | 1 x598; 0 x91 |
| LS SkyMul | 501 | - | - | - | - | - | 0,0,0,0 x126; 1,0.627451,0.627451,0 x48; 1,1,1,0 x39; 1,0.996078,0.980392,0 x33 |
| LS SkyAdd | 430 | - | - | - | - | - | 0,0,0,0 x126; 1,1,1,0 x69; 0.784314,0.901961,1,0 x33; 1,0.784314,0.666667,0 x32 |
| LS ExposureInfluence | 210 | 0 | 0 | 1 | 1 | 1 | 1 x133; 0 x77 |
| LS BlurCount | 165 | 2 | 2 | 2 | 2 | 3 | 2 x162; 3 x3 |
| MB Enable | 3 | 0 | 0 | 1 | 1 | 1 | 1 x2; 0 x1 |
| MB Intensity | 1 | 0.8 | 0.8 | 0.8 | 0.8 | 0.8 | 0.8 x1 |
| MB MaxLength | 1 | 0.02 | 0.02 | 0.02 | 0.02 | 0.02 | 0.02 x1 |
| MB Shutter | 1 | 100 | 100 | 100 | 100 | 100 | 100 x1 |
| MB SampleCount | 1 | 8 | 8 | 8 | 8 | 8 | 8 x1 |
| CG Enable | 252 | 0 | 0 | 1 | 1 | 1 | 1 x223; 0 x29 |
| CG LutSourceId | 301 | -1 | 0 | 1000 | 7800 | 9900 | 0 x29; 5 x24; 5000 x24; 7804 x24 |
| CG FadeTime | 186 | 1 | 1 | 1 | 1 | 2 | 1 x185; 2 x1 |
| Glow Enable | 1 | 1 | 1 | 1 | 1 | 1 | 1 x1 |
| Glow NumLevels | 1 | 7 | 7 | 7 | 7 | 7 | 7 x1 |
| Glow BlurSmooth | 1 | 1.2 | 1.2 | 1.2 | 1.2 | 1.2 | 1.2 x1 |
| Glow BlurSampleScale | 1 | 1 | 1 | 1 | 1 | 1 | 1 x1 |
| Glow RgbScale | 1 | 1.5 | 1.5 | 1.5 | 1.5 | 1.5 | 1.5 x1 |
| Glow AlphaScale | 1 | 1 | 1 | 1 | 1 | 1 | 1 x1 |
| DoF Enable | 12 | 0 | 1 | 1 | 1 | 1 | 1 x11; 0 x1 |
| DoF NearEnd | 12 | 0 | 0 | 0 | 0 | 0 | 0 x12 |
| DoF NearStart | 12 | 0 | 0 | 0 | 0 | 0 | 0 x12 |
| DoF FarStart | 12 | 3000 | 3000 | 3000 | 3000 | 3000 | 3000 x12 |
| DoF FarEnd | 12 | 4000 | 4000 | 4000 | 4000 | 4000 | 4000 x12 |
| DoF FocusDist | 12 | 100 | 100 | 100 | 100 | 100 | 100 x12 |
| DoF UseCameraFocus | 12 | 1 | 1 | 1 | 1 | 1 | 1 x12 |
| DoF LensF | 12 | 5.6 | 5.6 | 5.6 | 5.6 | 5.6 | 5.6 x12 |
| DoF Av | 12 | 1 | 1 | 1 | 1 | 1 | 1 x12 |
| DoF FilmSize | 12 | 0.024 | 0.024 | 0.024 | 0.024 | 0.024 | 0.024 x12 |
| DoF MaxBlurLevel | 12 | 2 | 2 | 2 | 2 | 2 | 2 x12 |
| DoF CoCMode | 36 | 0 | 0 | 1 | 1 | 1 | 1 x24; 0 x12 |
| LocalExp Enable | 8 | 0 | 0 | 0 | 0 | 0 | 0 x8 |
| Vig AspectMax | 8 | 4 | 4 | 4 | 4 | 4 | 4 x8 |
| Vig Color | 8 | - | - | - | - | - | 0,0,0,0.9 x8 |

### 3b. All files including cutscenes (s*)

| parameter | n values | files | min | p10 | median | p90 | max | distinct (top 6 with counts) |
|---|---|---|---|---|---|---|---|---|
| Exposure | 1504 | 265 | -3 | 2 | 3.6 | 4.42 | 8 | 4 x121; 3.5 x111; 4.2 x97; 3.2 x93; 3.9 x77; 3.6 x77 |
| ToneMapMode | 928 | 260 | 7 | 8 | 8 | 8 | 8 | 8 x912; 7 x16 |
| FilmicContrast(ToneMap-Contrast) | 918 | 260 | 0.4 | 0.5 | 0.5 | 0.5 | 1 | 0.5 x849; 0.4 x67; 1 x2 |
| ReinhardContrast | 1485 | 264 | 1.1 | 1.3 | 1.5 | 1.7 | 10 | 1.7 x348; 1.5 x239; 1.3 x218; 1.6 x138; 1.35 x135; 1.4 x130 |
| ReinhardWhiteBalance | 936 | 260 | 1 | 1 | 1.3 | 1.3 | 10 | 1.3 x456; 1 x402; 1.7 x40; 2 x13; 1.5 x8; 1.75 x7 |
| AE Enable | 905 | 259 | 0 | 0 | 0 | 1 | 1 | 0 x741; 1 x164 |
| AE LightAdaptSpeed | 903 | 259 | 0.1 | 0.1 | 1 | 1 | 2 | 1 x597; 0.25 x105; 0.1 x99; 0.2 x78; 0.15 x23; 2 x1 |
| AE DarkAdaptSpeed | 903 | 259 | 0.1 | 0.1 | 0.5 | 0.5 | 2 | 0.5 x597; 0.25 x105; 0.1 x99; 0.2 x78; 0.15 x23; 2 x1 |
| AE MiddleGray | 1216 | 259 | -8 | -7.8 | -6 | -2 | 0 | -2 x548; -6.4 x164; -7 x160; -7.8 x78; -6.2 x63; -6 x51 |
| AE CenterWeight | 903 | 259 | 1 | 4 | 4 | 7 | 7 | 4 x694; 7 x185; 1 x14; 3 x9; 2 x1 |
| AE AdaptMax | 906 | 259 | 0.01 | 0.4 | 8 | 8 | 8 | 8 x590; 0.4 x131; 1 x104; 0.5 x40; 0.8 x17; 0.01 x8 |
| AE AdaptMin | 1189 | 259 | -8 | -8 | -2.5 | -1 | -0.01 | -8 x554; -1.5 x368; -0.5 x75; -1 x44; -2 x31; -3 x29 |
| CA Enable | 926 | 259 | 0 | 0 | 1 | 1 | 1 | 1 x523; 0 x403 |
| CA Disp0 | 926 | 259 | 0 | 0 | 0.001 | 0.002 | 0.04 | 0 x380; 0.002 x109; 0.0015 x104; 0.001 x87; 0.0013 x61; 0.0018 x25 |
| CA Disp1 | 926 | 259 | 0 | 0 | 0.0001 | 0.002 | 0.02 | 0 x431; 0.0001 x127; 0.0003 x87; 0.0002 x76; 0.002 x61; 0.0008 x44 |
| Vig Power | 1367 | 259 | 0 | 0 | 0.43 | 0.55 | 8.13 | 0 x291; 0.5 x137; 0.35 x128; 0.52 x93; 0.48 x82; 0.4 x67 |
| Vig Range | 1006 | 259 | 0.01 | 0.4 | 1.2 | 1.5 | 2 | 1.2 x342; 0.4 x259; 1.3 x142; 1.5 x93; 1.25 x44; 0.6 x36 |
| Vig Shape | 998 | 259 | -0.19 | 0 | 0.2 | 0.3 | 1 | 0.2 x400; 0 x328; 0.3 x214; 0.7 x14; 0.4 x11; 0.1 x10 |
| Vig FovDep | 903 | 259 | 0 | 0 | 0 | 0 | 1 | 0 x848; 0.05 x24; 1 x23; 0.55 x6; 0.2 x1; 0.09 x1 |
| ColorH | 1026 | 259 | 0 | 0 | 0 | 0 | 1 | 0 x994; 1 x32 |
| ColorS | 1049 | 259 | 0.75 | 1 | 1 | 1 | 1.2 | 1 x1030; 0.9 x14; 1.2 x4; 0.75 x1 |
| ColorShift | 1225 | 256 | - | - | - | - | - | 1,1,1,0 x950; 1,1,1,1 x272; 0.613185,0.635741,0.677553,0 x2; 0.984314,1,0.996078,1 x1 |
| ColorScale | 1293 | 256 | - | - | - | - | - | 1,1,1,0 x988; 1,1,1,1 x272; 8 x8; 3 x7; 5 x5; 10 x5 |
| ColorV | 1026 | 259 | 0.6 | 1 | 1 | 1 | 2 | 1 x1003; 1.2 x17; 2 x4; 0.6 x1; 1.5 x1 |
| ColorContrast | 1026 | 259 | 1 | 1 | 1 | 1 | 1.03 | 1 x1004; 1.03 x22 |
| Bloom Enable | 701 | 86 | 0 | 1 | 1 | 1 | 1 | 1 x674; 0 x27 |
| Bloom NumLevels | 702 | 84 | 1 | 5 | 7 | 7 | 8 | 7 x497; 6 x98; 5 x84; 8 x11; 4 x8; 1 x2 |
| Bloom Luminance | 704 | 84 | 0.2 | 0.6 | 0.9 | 1.2 | 10 | 1 x229; 0.9 x194; 0.6 x110; 1.2 x37; 0.5 x25; 0.8 x20 |
| Bloom Threshold | 742 | 96 | 0 | 0.03 | 0.1 | 0.8 | 4 | 0.1 x177; 0.08 x88; 0.5 x66; 0.03 x63; 0.8 x59; 1 x49 |
| Bloom Alpha | 735 | 96 | 0.1 | 0.5 | 1 | 1 | 1 | 1 x573; 0.5 x86; 0.25 x44; 0.7 x11; 0.8 x11; 0.6 x6 |
| Bloom GaussRadiusScale | 698 | 84 | 1 | 1 | 1 | 1 | 1.8 | 1 x696; 1.8 x2 |
| Bloom SaturateCtrl | 698 | 84 | 0 | 0 | 0 | 0 | 0 | 0 x698 |
| Bloom Smooth | 698 | 84 | 0.8 | 0.8 | 0.8 | 1 | 1 | 0.8 x558; 1 x139; 0.9 x1 |
| Bloom FeedbackW | 698 | 84 | 0.8 | 0.8 | 0.8 | 0.8 | 0.8 | 0.8 x698 |
| LS Enable | 1047 | 158 | 0 | 0 | 1 | 1 | 1 | 1 x836; 0 x211 |
| LS SkyMul | 765 | 162 | - | - | - | - | - | 0,0,0,0 x152; 1,1,1,0 x64; 1,0.627451,0.627451,0 x52; 1,0.996078,0.980392,0 x36; 1,0.941176,0.862745,0 x35; 1,0.85,0.85,3 x31 |
| LS SkyAdd | 604 | 162 | - | - | - | - | - | 0,0,0,0 x152; 1,1,1,0 x134; 0.784314,0.901961,1,0 x38; 1,0.784314,0.666667,0 x35; 1,0.862745,0.666667,0 x27; 0.784314,0.901961,1,0.2 x16 |
| LS ExposureInfluence | 342 | 156 | 0 | 0 | 1 | 1 | 1 | 1 x253; 0 x88; 0.2 x1 |
| LS BlurCount | 291 | 156 | 2 | 2 | 2 | 2 | 4 | 2 x271; 3 x12; 4 x8 |
| MB Enable | 81 | 11 | 0 | 0 | 0 | 1 | 1 | 0 x51; 1 x30 |
| MB Intensity | 27 | 11 | 0 | 0 | 0 | 1 | 1 | 0 x21; 1 x4; 0.8 x1; 0.001 x1 |
| MB MaxLength | 27 | 11 | 0.01 | 0.02 | 0.02 | 0.02 | 0.02 | 0.02 x26; 0.01 x1 |
| MB Shutter | 27 | 11 | 10 | 100 | 100 | 100 | 100 | 100 x26; 10 x1 |
| MB SampleCount | 27 | 11 | 8 | 8 | 8 | 8 | 8 | 8 x27 |
| CG Enable | 286 | 125 | 0 | 0 | 1 | 1 | 1 | 1 x256; 0 x30 |
| CG LutSourceId | 334 | 126 | -1 | 0 | 101 | 7800 | 9900 | 0 x47; 5 x27; 5000 x24; 7804 x24; 2 x22; 3 x18 |
| CG FadeTime | 220 | 125 | 1 | 1 | 1 | 1 | 2 | 1 x219; 2 x1 |
| Glow Enable | 32 | 19 | 0 | 0 | 1 | 1 | 1 | 1 x26; 0 x6 |
| Glow NumLevels | 32 | 19 | 4 | 4 | 7 | 7 | 8 | 7 x20; 4 x5; 6 x4; 8 x2; 5 x1 |
| Glow BlurSmooth | 32 | 19 | 1 | 1 | 1.2 | 1.5 | 2 | 1 x12; 1.2 x11; 1.5 x7; 2 x2 |
| Glow BlurSampleScale | 32 | 19 | 1 | 1 | 1 | 1 | 2.11 | 1 x31; 2.11 x1 |
| Glow RgbScale | 32 | 19 | 1 | 1 | 1 | 1 | 1.5 | 1 x29; 1.5 x3 |
| Glow AlphaScale | 32 | 19 | 1 | 1 | 1 | 1 | 1 | 1 x32 |
| DoF Enable | 523 | 98 | 0 | 0 | 1 | 1 | 1 | 1 x468; 0 x55 |
| DoF NearEnd | 523 | 98 | 0 | 0 | 0 | 0 | 60 | 0 x514; 0.5 x6; 3 x2; 60 x1 |
| DoF NearStart | 523 | 98 | 0 | 10 | 10 | 10 | 200 | 10 x491; 0 x13; 1 x10; 0.1 x3; 0.5 x2; 30 x1 |
| DoF FarStart | 523 | 98 | 6 | 100 | 100 | 100 | 3000 | 100 x494; 3000 x13; 20 x11; 300 x2; 50 x1; 6 x1 |
| DoF FarEnd | 523 | 98 | 10 | 200 | 200 | 200 | 1e+04 | 200 x494; 4000 x13; 50 x7; 30 x3; 300 x2; 10000 x2 |
| DoF FocusDist | 523 | 98 | 100 | 100 | 100 | 100 | 100 | 100 x523 |
| DoF UseCameraFocus | 523 | 98 | 0 | 1 | 1 | 1 | 1 | 1 x512; 0 x11 |
| DoF LensF | 523 | 98 | 0.1 | 5.6 | 11 | 22 | 100 | 5.6 x123; 16 x102; 11 x95; 8 x52; 22 x34; 20 x16 |
| DoF Av | 523 | 98 | 1 | 1 | 1 | 4 | 13 | 1 x336; 2 x65; 4 x46; 3 x35; 1.5 x13; 2.5 x7 |
| DoF FilmSize | 523 | 98 | 0.01 | 0.024 | 0.024 | 0.024 | 0.04 | 0.024 x509; 0.035 x6; 0.01 x2; 0.02 x2; 0.015 x2; 0.011 x1 |
| DoF MaxBlurLevel | 485 | 89 | 2 | 2 | 2 | 2 | 2 | 2 x485 |
| DoF CoCMode | 1569 | 98 | 0 | 0 | 0 | 1 | 1 | 0 x1030; 1 x539 |
| LocalExp Enable | 8 | 1 | 0 | 0 | 0 | 0 | 0 | 0 x8 |
| Vig AspectMax | 8 | 1 | 4 | 4 | 4 | 4 | 4 | 4 x8 |
| Vig Color | 8 | 1 | - | - | - | - | - | 0,0,0,0.9 x8 |

Notes: type-3 and type-7 variants of `Enable AutoExposure` and `Chromatic Aberration Enable` exist in a few files; the stats group by name. `DoF CoCMode` also matches the Near/Far linear flags, so its count is inflated. ER's Exposure is in EV by the editor's range (-8..8, ER-TONEMAP-HDR section 1); its mapping to `g_ToneMapSceneLumScale` was not read here.

## 4. Per-area table (all files of one map merged: min..max over variants, ids and times; `.` = group not overridden)

Columns: RC = ReinhardContrast, RWB = ReinhardWhiteBalance, AE = auto exposure flags found, MG = Middle Gray, AEmin/AEmax = Adaption Min/Max, Lv = bloom levels, VigPow = vignette power, CA = chromatic aberration enable, S/V = ColorS/ColorV. Area names are from my memory of ER's map numbering (INFERRED, not read): m10 Stormveil, m11_00/05 Leyndell, m14 Raya Lucaria, m15 Haligtree, m16 Volcano Manor, m60 overworld, m61 DLC overworld.

| area (files) | Exposure EV | mode | RC | RWB | AE | MG | AEmin | AEmax | BloomThr | BloomLum | BloomA | Lv | LUT ids | VigPow | CA | DoF | S | V |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| default (1) | 3.5..3.8 | 8 | 1.3..1.35 | 1 | 0 | -7 | -0.5 | 0.4 | 0..0.5 | 1 | 0.1..1 | 1..7 | -1 | 0..0.4 | 1 | 0 | 1 | 1 |
| m00_00 (1) | 0 | 7 | 1.6 | 1.7 | 0 | -2 | -8 | 8 | 0.25..0.5 | 1.4..1.5 | 0.5..0.7 | 5..6 | . | 0 | 1 | . | 1 | 1 |
| m10_00 (9) | 3.2..4.2 | 8 | 1.25..1.6 | 1 | 1 | -8..-6 | -1.5..-0.5 | 0.4..1 | . | . | . | . | -1/0/1/2/4/5/1000/1002 | 0.39..0.55 | 1 | . | 1 | 1 |
| m10_01 (2) | 3.3..3.9 | 8 | 1.5 | 1 | 1 | -7 | -0.8..-0.2 | 0.4 | . | . | . | . | 0 | 0.42..0.53 | 1 | . | 1 | 1 |
| m11_00 (5) | 3..4.2 | 8 | 1.3..1.65 | 1 | 0/1 | -8..-2 | -8..-0.5 | 0.4..8 | . | . | . | . | 0/101/2002 | 0.32..0.6 | 1 | . | 1 | 1..1.2 |
| m11_05 (3) | 3.2..4.2 | 8 | 1.3..1.65 | 1 | 0/1 | -8..-6.3 | -1.5..-0.5 | 0.4..1 | . | . | . | . | 0/2002 | 0.45..0.6 | 1 | . | 1 | 1..1.2 |
| m11_10 (1) | 4.2 | 8 | 1.4 | 1 | 1 | -7 | -2 | 0.4 | . | . | . | . | 101 | 0.54 | 1 | . | 1 | 1 |
| m11_70 (1) | 4 | 8 | 1.55 | 1 | 0 | -2 | -8 | 8 | . | . | . | . | . | 0.45 | 1 | . | 1 | 1 |
| m11_71 (1) | 4.1 | 8 | 1.5 | 1 | 1 | -7 | -1 | 0.5 | . | . | . | . | . | 0.42 | 1 | . | 1 | 1 |
| m12_01 (6) | 3.8..4.2 | 8 | 1.4..1.45 | 1 | 1 | -7 | -2..-1.5 | 0.01..0.75 | . | . | . | . | 0/9000 | 0.46..0.54 | 1 | . | 1 | 1 |
| m12_02 (1) | 4.3 | 8 | 1.55 | 1 | 1 | -7 | -1.3 | 0.5 | . | . | . | . | . | 0.51 | 1 | . | 1 | 1 |
| m12_03 (1) | 4.5 | 8 | 1.5 | 1 | 1 | -6.4 | -0.1 | 0.4 | . | . | . | . | 0 | 0.5 | 1 | . | 1 | 1 |
| m12_04 (1) | 3.5 | 8 | 1.4 | 1 | 1 | -7 | -0.1 | 0.4 | 1 | . | 0.25 | . | 0 | 0.53 | 1 | . | 1 | 1 |
| m12_05 (1) | 4.3 | 8 | 1.55 | 1 | 1 | -7 | -1.5 | 0.4 | . | . | . | . | . | 0.55 | 1 | . | 1 | 1 |
| m12_07 (2) | 3.2..4.3 | 8 | 1.3..1.55 | 1 | 1 | -8..-6 | -1.5..-1 | 0.4..0.5 | . | . | . | . | 1/2/4/5 | 0.39..0.51 | 1 | . | 1 | 1 |
| m12_08 (1) | 3.5 | 8 | 1.4 | 1 | 0 | -7 | -0.5 | 0.4 | 0.2 | . | 1 | . | 0 | 0.435 | 1 | . | 1 | 1 |
| m12_09 (1) | 3.5 | 8 | 1.4 | 1 | 0 | -7 | -0.5 | 0.4 | 0.2 | . | 1 | . | 0 | 0.435 | 1 | . | 1 | 1 |
| m13_00 (3) | 4..4.4 | 8 | 1.48..1.55 | 1 | 1 | -7 | -1.5 | 0.4 | . | . | . | . | . | 0.43..0.54 | 1 | . | 1 | 1 |
| m14_00 (6) | 3.4..4.3 | 8 | 1.4..1.55 | 1 | 1 | -7 | -1.5..-0.5 | 0.4..0.6 | . | . | . | . | 0/99 | 0.435..0.55 | 1 | . | 0.75..1 | 0.6..1 |
| m15_00 (8) | 3.1..3.9 | 8 | 1.1..1.6 | 1 | 1 | -7 | -1 | 0.4 | 0.25..0.5 | 1 | 0.6..1 | 6 | 0/1/2/3 | 0.45..0.55 | 1 | . | 1 | 1 |
| m16_00 (8) | 3.2..4.5 | 8 | 1.3..1.4 | 1 | 1 | -8..-6 | -2..-0.1 | 0.4..0.5 | . | . | . | . | 101/105/2002/2102/9800/9900 | 0.35..0.56 | 1 | . | 1 | 1..1.2 |
| m18_00 (4) | 2.9..3.9 | 8 | 1.3..1.7 | 1 | 0/1 | -8..-2 | -8..-0.5 | 0.4..8 | . | . | . | . | . | 0.39..0.55 | 1 | . | 1 | 1 |
| m19_00 (6) | 2.2..4.2 | 8 | 1.5..1.65 | 1 | 0/1 | -7..-2 | -8..-0.01 | 0.6..8 | . | . | . | . | . | 0..0.53 | 1 | . | 1 | 1 |
| m19_70 (1) | 2.2 | 8 | 1.5 | 1 | 0 | -2 | -8 | 8 | . | . | . | . | . | 0 | 1 | . | 1 | 1 |
| m20_00 (2) | 3.6..4.2 | 8 | 1.3..1.55 | 1 | 1 | -8..-6 | -3..-1.5 | 1 | . | . | . | . | 2010/5000/5001/5002/5003 | 0.45..0.58 | 1 | 0/1 | 1 | 1 |
| m20_01 (6) | 3.4..4.1 | 8 | 1.25..1.55 | 1 | 1 | -6.4 | -1.5..-0.1 | 0.5..1 | . | . | . | . | 2010/2110/2190 | 0.42..0.58 | 1 | 1 | 1 | 1 |
| m21_00 (5) | 3.35..4.45 | 8 | 1.2..1.6 | 1 | 1 | -8..-6 | -3..-0.5 | 0.4..1 | . | . | . | . | 0/5500/5501/5502/7510/7800/7804 | 0.35..0.6 | 1 | 1 | 1 | 1 |
| m22_00 (5) | 1.75..4.1 | 8 | 1.32..1.5 | 1 | 1 | -8..-2.108 | -6..-1.5 | 0.4..1 | . | . | . | . | 100/300/6000 | 0.45..0.58 | 1 | . | 1 | 1 |
| m25_00 (1) | 4.2 | 8 | 1.35 | 1 | 0 | -6.4 | -8 | 8 | . | . | . | . | 0 | 0.54 | 1 | . | 1 | 1.5 |
| m28_00 (3) | 3.9..4.3 | 8 | 1.3..1.6 | 1 | 1 | -7..-6.4 | -1.5..-0.5 | 0.4 | 1 | . | 0.25 | . | 0/1000/6500 | 0.35..0.56 | 1 | . | 1 | 1 |
| m30_00 (7) | 3.2..4 | 8 | 1.3..1.55 | 1 | 0/1 | -8..-6 | -1.5..-0.5 | 0.4..0.5 | . | . | . | . | -1/1/2/4/5 | 0.39..0.5 | 1 | . | 1 | 1 |
| m34_10 (1) | 3.35..4.1 | 8 | 1.45..1.6 | 1 | 1 | -6.4 | -1.4..-0.5 | 0.4 | . | . | . | . | . | 0.42..0.48 | 1 | . | 1 | 1 |
| m34_11 (2) | 3.2..3.75 | 8 | 1.3..1.6 | 1 | 1 | -7..-6.4 | -2..-1.5 | 0.4..1 | . | . | . | . | . | 0.42..0.55 | 1 | . | 1 | 1 |
| m34_12 (3) | 3.2..4 | 8 | 1.3..1.55 | 1 | 1 | -8..-6.3 | -1.5 | 0.4..0.5 | . | . | . | . | -1/2002 | 0.48..0.5 | 1 | . | 1 | 1..1.2 |
| m34_13 (1) | 3.15..3.8 | 8 | 1.3..1.35 | 1 | 1 | -7.8..-6.4 | -0.8 | 0.8 | . | . | . | . | 3002 | 0.4..0.46 | 1 | . | 1 | 1..1.2 |
| m34_14 (3) | -3..3.65 | 8 | 1.45..1.6 | 1 | 0/1 | -7..-2 | -8..-1 | 0.4..8 | . | . | . | . | . | 0..0.32 | 0/1 | . | 1 | 1 |
| m34_15 (1) | 3.2..3.6 | 8 | 1.3..1.35 | 1 | 1 | -7 | -1 | 0.4 | . | . | . | . | . | 0.48..0.52 | 1 | . | 1 | 1 |
| m35_00 (5) | 3.5..4.1 | 8 | 1.5..1.55 | 1 | 0/1 | -7..-6.4 | -1..-0.2 | 0.2..0.4 | . | . | . | . | 0 | 0.435..0.6 | 1 | . | 1 | 1 |
| m39_20 (5) | 3.2..4.2 | 8 | 1.3..1.5 | 1 | 1 | -8..-6.3 | -1.5 | 0.4..1 | . | . | . | . | 1000/1002/2002 | 0.42..0.55 | 1 | . | 1 | 1..1.2 |
| m40_00 (5) | 3.2..4.3 | 8 | 1.3..1.55 | 1 | 1 | -8..-6 | -3..-1 | 0.4..1 | 1 | . | 0.25 | . | 1/2/4/5/5000/5001/5002/5003/6500/7800/7804 | 0.35..0.6 | 1 | 1 | 1 | 1 |
| m41_00 (5) | 3.2..4.2 | 8 | 1.3..1.55 | 1 | 0/1 | -8..-6 | -3..-1 | 0.4..1 | . | . | . | . | 1000/5000/5001/5002/5003/7800/7804 | 0.35..0.6 | 1 | 1 | 1 | 1 |
| m42_00 (4) | 3.6..4.2 | 8 | 1.3..1.55 | 1 | 0/1 | -8..-6 | -3..-1 | 0.4..1 | . | . | . | . | -1/5000/5001/5002/5003/7800/7804 | 0.35..0.6 | 1 | 1 | 1 | 1 |
| m43_00 (4) | 3.35..4.2 | 8 | 1.3..1.55 | 1 | 0/1 | -8..-6 | -3..-1 | 0.4..1 | . | . | . | . | -1/1/2/4/5/5000/5001/5002/5003 | 0.39..0.53 | 1 | . | 1 | 1 |
| m45_00 (1) | 3.9 | 8 | 1.5 | 1 | 1 | -6.2 | -0.6 | 0.5 | . | . | . | . | 101 | 0.43 | 1 | . | 1 | 1 |
| m45_01 (1) | 3.15 | 8 | 1.32 | 1 | 0 | -6.4 | -8 | 8 | . | . | . | . | 3002 | 0.4 | 1 | . | 1 | 1.2 |
| m45_02 (1) | 3.5 | 8 | 1.6 | 1 | 0 | -6.4 | -8 | 8 | . | . | . | . | . | 0.42 | 1 | . | 1 | 1 |
| m60_00 (20) | 0..4.7 | 8 | 1.3..2 | 1..2 | 0/1 | -8..-2 | -8..-0.01 | 0.01..8 | 0.25..0.5 | 1.4..1.5 | 0.5..0.7 | 5..6 | 1/2/4/5/1002/1009/2002/2102/3002/4002/4202 | 0..0.56 | 0/1 | . | 1 | 1..1.2 |
| m61_00 (18) | 3.1..4.7 | 8 | 1.2..1.5 | 1 | 0/1 | -8..-6 | -8..-0.5 | 0.4..8 | 1 | . | 0.25 | . | 0/1/2/4/5/5000/5001/5002/5003/5009/5500/5501/5502/6000/6500/7500/7510/7800/7804/7808/7809 | 0.35..0.65 | 1 | 1 | 1 | 1 |
| mapbas (1) | 3.5 | 8 | 1.35 | 1 | 0 | -7 | -0.5 | 0.4 | 1 | 1 | 0.25 | 5 | -1 | 0.4 | 1 | 1 | 1 | 1 |
| s12_01 (2) | 1..4.1 | 8 | 1.4..2 | 1..2 | 0/1 | -7..0 | -8..-0.5 | 0.01..8 | 0.03..0.2 | 0.9..1 | 1 | 5..8 | . | 0..0.51 | 0/1 | 1 | 1 | 1 |
| s12_02 (2) | 1..6 | 7/8 | 1.5..2 | 1..2 | 0/1 | -7..-2 | -8..-0.1 | 0.3..8 | 0.03..0.4 | 0.35..1.5 | 1 | 4..7 | . | 0..0.51 | 0/1 | 0/1 | 1 | 1 |
| s12_06 (1) | 1.3..4 | 8 | 1.3..2.7 | 1..1.7 | 0 | -7..-2 | -8..-0.5 | 0.4..8 | 0.01 | 0.2..0.3 | 1 | 7 | . | 0..0.5 | 0 | 1 | 1 | 1 |
| s14_00 (3) | 0.5..3.9 | 8 | 1.5..1.7 | 1..1.3 | 0 | -2 | -8 | 8 | 0.04..0.1 | 0.8..0.9 | 1 | 7..8 | . | 0..0.43 | 0/1 | 0/1 | 0.9..1 | 1 |
| s15_00 (3) | -0.7..3.2 | 8 | 1.5..1.8 | 1.3..1.7 | 0 | -2 | -8 | 8 | 0.05..0.25 | 0.5..1 | 1 | 7 | . | 0..0.45 | 0/1 | 0/1 | 1 | 1 |
| s16_00 (4) | 0.5..5 | 8 | 1.5..1.7 | 1..1.7 | 0 | -6.2..-2 | -8..-1.5 | 8 | 0.02..0.09 | 0.3..1 | 1 | 3..7 | . | 0..8.13 | 0/1 | 0/1 | 1 | 1 |
| s19_00 (25) | -3..8 | 7/8 | 1.5..2 | 1..2 | 0/1 | -7..-2 | -8..-1 | 0.8..8 | 0.01..2 | 0.3..2 | 1 | 7..8 | . | 0..0.53 | 0/1 | 0/1 | 1 | 1 |
| s20_00 (2) | 3.5..5.5 | 8 | 1.3..1.55 | 1 | 0/1 | -6.4 | -1.5 | 1 | 0.1 | 1.3 | 0.25 | 5 | . | 0.45..0.56 | 1 | 0/1 | 1 | 1 |
| s20_01 (2) | 2..4.1 | 8 | 1.45..1.6 | 1 | 0/1 | -6.4 | -8..-1.5 | 1..8 | 0.08..1 | 0.3..2 | 0.25..1 | 5..7 | . | 0.45..0.56 | 1 | 1 | 1 | 1 |
| s21_00 (3) | 3.2..5.4 | 8 | 1.3..1.6 | 1 | 0/1 | -8..-6 | -1.5..-0.5 | 0.4 | 1 | 1 | 0.25 | 5 | 0 | 0.4..0.6 | 1 | 1 | 1 | 1 |
| s21_01 (2) | 3.8..4.5 | 8 | 1.6 | 1 | 0/1 | -6.4 | -0.5 | 0.4 | 0.08..1 | 0.5..1.5 | 0.25 | 5 | . | 0.45..0.65 | 1 | 1 | 1 | 1 |
| s28_00 (1) | 3.9..4.3 | 8 | 1.6 | 1 | 0/1 | -6.4 | -0.5 | 0.4 | 0.04..0.08 | 1.2..3 | 0.25 | 5 | . | 0.35 | 1 | 1 | 1 | 1 |
| s34_10 (2) | 1..2.4 | 8 | 1.7..2 | 1.3..2 | 0 | -2 | -8 | 8 | 0.03 | 0.9 | 1 | 7 | . | 0..0.4 | 0 | 0 | 1 | 1 |
| s34_11 (3) | 0..4 | 8 | 1.25..1.7 | 1..1.75 | 0/1 | -7..-2 | -8..-1 | 0.4..8 | 0.02..0.8 | 0.9..1 | 1 | 5..7 | . | 0..0.52 | 0/1 | 1 | 0.9..1 | 1 |
| s35_00 (1) | 2..4.1 | 8 | 1.55..1.7 | 1..1.3 | 0 | -2 | -8 | 8 | 0.08..0.4 | 0.9..2 | 1 | 7 | . | 0.45..0.6 | 1 | 1 | 1 | 1 |
| s60_37 (1) | 3.1..4.2 | 8 | 1.25..1.7 | 1..1.3 | 0 | -2 | -8 | 8 | 0.07..0.08 | 0.6..0.8 | 1 | 7 | . | 0.42 | 1 | 1 | 1 | 1..1.2 |
| s60_38 (4) | 0..4.7 | 7/8 | 1.25..1.7 | 1..1.7 | 0 | -2 | -8 | 8 | 0.01..4 | 0.9..1.25 | 0.5..1 | 5..7 | 0/5 | 0..0.35 | 0/1 | 0/1 | 0.9..1 | 1 |
| s60_42 (8) | -0.4..3.85 | 8 | 1.5..1.7 | 1.3 | 0 | -2 | -8 | 8 | 0.02..0.5 | 1..10 | 0.5..1 | 7..8 | 0 | 0..0.9 | 0/1 | 1 | 1 | 1 |
| s60_43 (1) | 1.1..3.5 | 8 | 1.6..1.7 | 1.3..1.7 | 0 | -2 | -8 | 8 | 0.08..4 | 0.9..1.25 | 0.5..1 | 6..7 | 0 | 0..0.4 | 0 | 0/1 | 1 | 1 |
| s60_49 (8) | 0..4.7 | 7/8 | 1.25..1.7 | 1..1.7 | 0 | -2 | -8 | 8 | 0.01..1 | 0.9..1.25 | 0.5..1 | 5..7 | -1/0 | 0..0.4 | 0/1 | 1 | 0.9..1.2 | 1 |
| s60_51 (3) | 1.4..8 | 7/8 | 1.3..10 | 1..10 | 0/1 | -6.8..-2 | -8..-1.5 | 0.8..8 | 0.02..4 | 0.9..5 | 0.5..1 | 6..7 | 0 | 0..0.5 | 0/1 | 0/1 | 1 | 1..2 |
| s60_54 (4) | 0.75..4.1 | 8 | 1.5..1.7 | 1..1.3 | 0 | -2 | -8 | 8 | 0.1..0.8 | 0.6..1 | 0.5..1 | 6..7 | -1/4002 | 0..0.4 | 0/1 | 0/1 | 1..1.2 | 1 |
| s61_44 (1) | 3..5.5 | 8 | 1.2..1.48 | 1 | 0/1 | -8..-6 | -2.5..-1.5 | 0.5..8 | 0.25..0.5 | 0.5..8 | 1 | 7 | 5500/5502 | 0.38..0.6 | 1 | 1 | 1 | 1 |

(Per individual file, every variant: `tab-per-file.md`.)

### 4b. Overworld day curve (`m60_00_0000`, OBSERVED, keys in hours)

| key | 0 | 4 | 8 | 11.5 | 12 | 15 | 21 | 22.5 |
|---|---|---|---|---|---|---|---|---|
| Exposure | 3.9 | 3.5 | 3.35 | 3.48 | 3.57 | 3.2 | 3.2 | 3.3 |
| ReinhardContrast | 1.3 | 1.35 | 1.35 | 1.4 | 1.4 | 1.4 | 1.3 | 1.3 |
| AE Middle Gray | -7 | -6.2 | -6 | -6.75 | -6.75 | -8 | -7.8 | -7.8 |
| AE Adaption Min | -1 | -1.5 | -1.5 | -1.5 | -1.5 | -1.5 | -1.5 | -1.5 |
| LUT id | 1 | 2 | 2 | 4 | 4 | 5 | 5 | 5 |
| Vignette Power | 0.41 | 0.39 | 0.39 | 0.43 | 0.43 | 0.5 | 0.5 | 0.5 |

Same file: AE enabled, adapt speeds 0.1/0.1, Adaption Max 0.4, Center Weight 7, colour grading enabled, vignette range 1.3 shape 0.2, CA enabled with dispersion 0.0006 / 0, light shafts on (ExposureInfluence 0 until 12 h, 1 from 15 h). `default`: mode 8, exposure 3.5 (ids 10/11: 3.8), RC 1.35 (1.3), AE off, MG -7, Adaption -0.5..0.4, bloom id 0 thr 0.5 / lum 1 / alpha 1 / 7 levels, vignette 0.4 / 1.2 / 0.2, CA on (dispersion 0), DoF off, colour grading off (LUT -1).

## 5. What these files do not give

DoF in gameplay: only 12 gameplay value rows override it (DLC), so the base comes from `default` (Enable 0, near 1, far 100..200, F 5.6, Av 1); the effective gameplay state was not resolved. LUT ids are references to LUT textures (not read here). The merge order (default, mapbase, map, variant, weather/time) was not read from the executable. Cutscene sets (s*) differ strongly (exposure -3..8, mode 7, RC up to 10, bloom thresholds 0.01..4, DoF on): not a gameplay look.

## 6. Shader maths (Part 2; OBSERVED in the disassembly; constant names from the shaders' reflection blocks)

Container: `/shader/gxposteffect.shaderbnd.dcx` (Data0) is a BND4 of 193 entries (`.vpo`, `.cpo`, `.ppo`), each a DXBC container holding DXIL, shader model 6.0, entries named as in the executable (`ToneMap_*PS`). Disassembled with `dxc.exe -dumpbin` (Windows Kits 10.0.28000.0); outputs kept in er-post/shader/dis. `cbToneMap` layout (byte offsets): g_ToneMapInvSceneLumScale 0, g_ReinhardParam 16, g_ToneMapParam 32, g_ToneMapSceneLumScale 48, g_AdaptParam 64, g_AdaptCenterWeight 80, g_BrightPassThreshold 96, g_GlareLuminance 112, g_BloomBoostColor 128, g_vBloomFinalColor 144, g_vBloomScaleParam 160, g_mtxColorMultiplyer 176 (float4x3), g_vChromaticAberrationRG 224, ...B 240, g_bEnableFlags 256, g_vFeedBackBlurParam 272, g_vVignettingParam 288, g_vHDRDisplayParam 304, g_vChromaticAberrationShapeParam 320, g_vScreenSize 336, g_vScenePreExposure 384, g_vCameraParam 400.

**Tables** (1D; input u = TEXCOORD.x in 0..1; x = 0.2 u/(1-u); output copied to RGB):
- ReinhardEx (mode 8, ER's shipping mode): `t = (ReinhardParam.y * x)^ReinhardParam.x ; y = t/(t+1) ; out = y^(1/ToneMapParam.y)`. ReinhardParam.x is the contrast exponent (the ReinhardContrast row; INFERRED), .y a scale (INFERRED); ToneMapParam.y is the display gamma (INFERRED 2.2). No white-point term appears despite the name.
- Sensitometric (mode 7): `A = 1 - exp(-1.442695 x) = 1 - 2^-x ; B = (1 - exp(-4.456668 x))^3 ; y = A + ToneMapParam.x (B - A) ; out = y^(1/ToneMapParam.y)`. Answer to the question: no, it is not DS3's `y = (1-exp(-xa))(1-b exp(-xa))^2`; it is a blend of a 1-exp and a cubed 1-exp, weighted by ToneMapParam.x.
- Filmic (mode 0): `x' = max(x - 0.004, 0) ; y = x'(6.2 x' + 0.5) / (x'(6.2 x' + 1.7) + 0.06) ; out = y^(2.2/ToneMapParam.y)` (the Hejl / Burgess-Dawson fit, which embeds gamma 2.2).

**ToneMap_TabledToneMapLUTPS** (the final pass), in order:
1. uv = input * g_dynamicScreenPercentage; scene = Source(uv); glare = GlareAcc(uv).
2. c = scene * g_ToneMapSceneLumScale.xyz + glare * g_GlareLuminance.x. Bloom is added linearly, before the curve; additive, not a screen blend.
3. c' = max(0, M c + m) with the 3x4 affine matrix g_mtxColorMultiplyer (ColorH/S/V/Contrast/ColorShift/Scale are INFERRED to be folded into it on the CPU).
4. v = Table(c'/(c'+0.2)) per channel (bilinear, clamp): a rational input map with knee 0.2, not log.
5. Vignette: p = (2 uv - 1) * VignettingParam.xy; r = |p|; t = saturate(1 - saturate(r*VignettingParam.z + VignettingParam.w)); factor = (t^2 (3-2t))^4 (smoothstep to the fourth power); v *= factor, on the display-encoded colour, after the curve.
6. 3D LUT: coordinate = max(v,0)^ToneMapParam.z * 0.9375 + 0.03125 (16-cube LUT, half-texel mapped), trilinear. In the SDR path (g_bEnableFlags.z == 0) the LUT output is written as is: no sRGB or PQ step, no dither.
7. HDR path (flag set): undo the LUT power, undo Reinhard (x = (v/max(1-v, 0.0157))^(1/ReinhardParam.x)), BT.601 luma (0.299, 0.587, 0.114), a highlight shoulder for luma > 1 (`0.5 + 19 (R(1 + (L-1) 0.0521) - 0.5)` with R(x) = x^c/(x^c+1)), colour rescaled by the new luma, then `* HDRDisplayParam.y`, a power near 0.30 and a scale near 0.50 (constants not decoded further). Not studied in depth.
Chromatic aberration is not in this pass.

**ToneMap_PostOETFPS** (misleading name): only the lateral dispersion pass. Per channel it samples the source at `uv + (CA_RG.xy for R, CA_RG.zw for G, CA_B.xy for B) * (2 uv - 1)`, clamped half a texel inside the dynamic region. No gamma, OETF, dither, sRGB or PQ. The `PostOETFFeedBack` and `SeparateEnd` variants were not read.

**ToneMap_BrightPassPS**: scene = Source * g_ToneMapSceneLumScale.xyz (exposure applies to the bloom source); L = 0.299 R + 0.587 G + 0.114 B; k = max(L - g_BrightPassThreshold.x, 0)/(L + 1e-4); out = k * scene (NaN to 0); saturation control `out += (luma(out) - out) * g_BloomBoostColor.w`; `out *= g_BloomBoostColor.xyz`. A soft luminance subtraction, in post-exposure linear units.

**ToneMap_BloomCompositeUpScalePS**: out = Source (bilinear, clamped to the region) * g_vBloomScaleParam.x, a per-level weight; the accumulation across levels is done by render state (additive, INFERRED). **ToneMap_BloomCompositePS**: out = (S3 + 0.5 S2 + 0.25 S1) * g_vBloomFinalColor.xyz from three source textures (which texture is which level not resolved). **ToneMap_BloomGaussBlurPS**: taps from `cbToneMapSampleOffset[16]` (offsets and weights are CPU-side, not read). The mapping of Luminance, Alpha, GaussianRadiusScale and Smooth to these constants was not established.

## 7. Not done / not established

- Which suffix (`_0010`, `_0100`...) is selected where and when; the layering order of the files.
- Run-time constant-buffer values (ToneMapParam, ReinhardParam, SceneLumScale): the mapping from the parameters (Exposure EV, ReinhardContrast, White Balance, gamma) to the table constants is INFERRED, not observed.
- The HDR branch constants, the Separate and Feedback variants, the glare Gaussian weights, the construction of the colour matrix from H/S/V/Contrast.
- The 3D LUT images (ids 1, 2, 4, 5, 5000...) were not read: the look of the grade is not captured, only that it is applied.
- Area names are from memory. The name sweep used `mAA_BB_CCCC` / `sAA_BB_CCCC` patterns built from the bases found; a file with an unusual name would be missed.
