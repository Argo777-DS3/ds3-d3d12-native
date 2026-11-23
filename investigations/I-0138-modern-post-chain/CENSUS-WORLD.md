# CENSUS-WORLD: post-processing parameters fed by the GPARAM files, retail world areas

Date 2026-11-12. Made by a Sonnet worker (read-only; nothing launched). **Non-canonical until the pilot re-reads.**
Tags: **OBSERVED** = read in a file (named); **INFERRED** = my reading of names/structure, not verified.
Scope: map files `mAA_BB_NNNN` of the 18 retail world areas (100 files: 18 bases `_0000` plus 82 variants) and the global files
default, mapbase, remobase, mapoverride_*, eclipse, empty, m_template, ml_template. Cutscene `sAA_*` files are not covered.

Sources: `evidence/I-0066/queries/gparam-all/param_drawparam_*.tsv` (hex column decoded: float32 for types 9/12/14, int32 for 3/7,
byte for 5), `evidence/I-0066/queries/ae/msb-gparam-slots.tsv` (selectability), `STUDY-YEBIS-REFUSALS.md` (inheritance rule).
Scripts and raw tables: `evidence/I-0138/artifacts/census-world/` (`census.py`, `tables.py`, `sheet.py`; `effective.tsv` = one row per file x group x parameter, id 0, effective value and source layer; `variant-diffs.tsv` = every effective difference variant vs its base; `per-file-sheet.tsv` = key values for all 100 files).

## Answers first

1. All 100 retail map files carry only value id 0 in these groups (OBSERVED). Effective value = own file, else `mAA_BB_0000`, else `mapbase`, else `default` id 0.
2. The chain actually used in the world is small: filmic tone map (ToneType 2, gamma 2.2 everywhere), exposure 1.52..2.4 (fixed), saturation 0.8..0.9, per-area vignette 0..0.4, per-area colour-grading LUT, glare/bloom (shape 1, 6 or 8 levels), light shafts in 12 of 18 areas, DoF (identical mapbase values in all 18 areas), motion blur (identical), anti-alias entry (AntiAliasType 4, identical).
3. Auto exposure is on only in `m30_00_0100`/`m30_00_0110` (not selectable per the MSB census). Never in any other world file.
4. Never non-default in any world file: star, ghost, halo, afterimage, anamorphic flare, bloom dispersion, FeedBackBlur (Enable 0, weight 0), aperture airy disc, Tone Map `Enable`, uniform dispersion, vignette fov dependence, ColorV != 1, contrast != 1, DoF auto-focus; hue is non-zero (1.0) only at the m41/m47/m50 bases.
5. Lateral dispersion (ToneLateralDispersion[0] = 0.006) is non-zero only in m30_00, m30_01, m34_01, m46_00 (and their variants, except m30_00_0100/0110); everywhere else 0.
6. Glare shape is 1 everywhere except `m32_00_0100` (shape 3, 10 levels, selectable slot 1, 5 of 59 collisions). Bloom luminance 1.8..8, threshold 0.2..0.5.
7. DoF is on everywhere with mapbase values (F 2.0, edge quality 2, far custom on, near custom off); variants differ only in m30_00_0200, m32_00_0100, m38_00_0002.
8. Two exposure oddities: `m41_00_0800`/`m47_00_0800` exposure 0.45 and `m41_00_0900`/`m47_00_0900` exposure **0.0** (the tone pass refuses exposure <= 0 per STUDY-YEBIS-REFUSALS refusal 5).
9. Groups not in the brief but present and relevant: `Lens Simulation` (a DoF/shutter-style block, Enable 1, uniform values in 12 areas), `ColorParaffin Filter` (Enable 0 everywhere), `WaterInteraction Filter` (m30_00 off, m33_00 on).
10. Reader check passed against the STUDY (section 6).

## 1. Group names

All groups in the 168 files (OBSERVED): Anti-Alias, AOFields, AOFields Model, Bloom, ColorGrading, ColorParaffin Filter, Dof, Dominant Light, Env Map, FeedBackBlur, Fog, Glow Filter, Lens Simulation, LightScattering, LightSet, LightShaft, Ls, MotionBlur, Other PostEffects (only `s_template`), SeaWave, Setting Info, ShadowParam, Snow, Snow Model, SSAO, Tone Map, WaterInteraction Filter. No group called LensFlare exists; no wind group in these files. Lights, fog, scattering, envmap, shadows, SSAO, SeaWave, Snow are not covered here.

Group ownership (OBSERVED): `default` id 0 holds Tone Map, Bloom, Dof, LightShaft, MotionBlur, ColorGrading, Anti-Alias, FeedBackBlur, Setting Info; `mapbase` holds Bloom, Dof, Setting Info, MotionBlur, Anti-Alias, Glow Filter. No world base file holds its own Dof, MotionBlur or Anti-Alias group, so those come from mapbase. Own Tone Map, Bloom, LightShaft, ColorGrading in every base; FeedBackBlur in 14 of 18 bases (absent in m33_00, m35_00, m38_00); Setting Info in m33, m38, m39, m40, m41, m47, m50 (others inherit mapbase); Glow Filter only in m45_00. `remobase` holds Bloom(39 entries), Dof, MotionBlur, Anti-Alias, Glow Filter, Setting Info(24); `eclipse` only Fog; `empty` nothing; `mapoverride_0001/0003` hold Tone Map, ColorGrading, FeedBackBlur, Lens Simulation, ColorParaffin Filter (0003 also LightShaft) among others.

## 2. Parameter lists, orders and types

Parameter order in section 5 is the file order (default id 0 first). **A group can hold the same parameter name twice**; I label the second occurrence `Name#2`. Tone Map holds 51 entries in world files: the first 34 are `Exposure`..`ToneUniformDispersion[1]`, then a second block `Exposure#2`, `AvgLuminance`, ..., `AutoExposure Center Weight`; this agrees with the STUDY's position numbering (entry 46 `Enable AutoExposure`). Type codes: 9 float, 3 int, 5 byte/bool, 7 small int, 12 float2, 14 float4. In the tables, `None` in a departing-area list means the group is absent in that file's whole chain (no value).

Entry counts per group in world files: Tone Map 51, Bloom 43, Dof 34, LightShaft 15, MotionBlur 21, ColorGrading 2, Anti-Alias 9, FeedBackBlur 7, Glow Filter 6, Setting Info 25, Lens Simulation 20, ColorParaffin Filter 7, WaterInteraction Filter 7. The full ordered lists are the "parameter" column of section 5. A few variant-only parameters (LightShaft `RadX/RadY`, Bloom `AnamorphicFlareColor#2`, Tone Map `AutoExposure Speed`) appear only in non-base files and are in `effective.tsv`.

## 3. Feature verdict table

| Feature | Used in world areas? / where / values | Tag |
|---|---|---|
| Fixed exposure | Yes, always. Exposure 2.0 most common (46 of 100 files), 2.3 (25), 2.2 (11), 1.52 (m37_00 and 9 variants), 2.4 (m32_00_0000, m53_00_0000), 2.46 (m30_00_0100/0110), 0.45 (m41_00_0800, m47_00_0800), 0.0 (m41_00_0900, m47_00_0900) | OBSERVED |
| Auto exposure (`AutoExposure`, `Enable AutoExposure`) | Only m30_00_0100 and m30_00_0110 (AutoExposure 1; not selectable per MSB census). `Enable AutoExposure` is 0 in all 100 files. The tuning entries (MiddleGray 0.02..0.18, Min/MaxExposure, adaptation sensitivity/scale/speed limits) are present and vary per area even though AutoExposure is 0 | OBSERVED |
| Tone type / filmic contrast | ToneType 2 in all; ToneMappingFactor 32 (30 at m41/m47/m50); FilmicContrast 0.0..0.3 (0.15 most common), 0.5 in m41_00_0800/0900 and m47_00_0800/0900 | OBSERVED |
| Gamma | Never non-default: 2.2 in all 100 files (ColorV 1, Contrust 1, ToneInfluencedByGlare 1 too) | OBSERVED |
| Hue / saturation / value / contrast | ColorS 0.75..1.0 (bases 0.8, 0.84, 0.85, 0.9); ColorH 1.0 in m41_00, m47_00, m50_00 bases (0 elsewhere and in the 0800/0900 variants); ColorV always 1; Contrust always 1 | OBSERVED |
| Vignette | Power 0..0.4 per area (0.3 most common); VignetteFovDependence 0 always (never non-default). No colour or radius parameter exists | OBSERVED |
| Colour grading LUT | `ColorGrading` Enable/LutSourceId per area: ids 0,1,2,3; m37_00 base Enable 0 (m37_00_0500/0510 on, id 1); variants m30_00_0100/0110, m32_00_0100, m38_00_0002, m39_00_0100/0200 switch it off (id -1) | OBSERVED |
| Bloom (threshold, luminance, radius scale, levels, luminance gamma) | Yes, all areas: GlareThreshold 0.2..0.5, GlareLuminance 1.8..8, GlareBloomGaussianRadiusScale 0.5..2, GlareNumLevels 6 or 8 (10 only m32_00_0100), GlareBloomLuminanceGamma 1.0 or 1.4 | OBSERVED |
| Bloom dispersion | Never non-default: BloomLuminance 0, BloomDispersion 0 in all files | OBSERVED |
| Glare shape | Shape 1 in all but m32_00_0100 (shape 3). The data holds no name for shapes 0/1/3; shape 0 appears only in `default` ids (STUDY). I do not know what the numbers mean | OBSERVED / meaning unknown |
| Star, ghost, halo, afterimage | Never non-default in any world file (StarLuminance, GhostLuminance, GhostHaloLuminance, AfterimageLuminance, star lengths, streaks all 0) | OBSERVED |
| Anamorphic flare | Never non-default (AnamorphicFlareLuminance 1, colours white everywhere; the Bloom `Enable` entries are 0 everywhere, meaning unknown) | OBSERVED |
| Afterimage / feedback blur | FeedBackBlur Enable 0, weights 0 in every world file; Setting Info `Enable FeedBackEffect` is 1 | OBSERVED |
| Light shafts | On (Enable 1) in bases of m30_00, m32_00, m35_00, m37_00, m39_00, m40_00, m41_00, m45_00, m47_00, m50_00, m51_00, m53_00; off in m30_01, m31_00, m33_00, m34_01, m38_00, m46_00. Per-area ColorScale (RGB + a fourth value up to 10.006 at m37_00), ShaftLength 0.76..4.31, GlareRatio 0.31..1, per-area angles. Variants turn it on (m30_01_0010, m31_00_0010/0100/0110, m33_00_0500, m34_01_0010, m38_00_0002) or off (m32_00_0010/0110, m39_00_0100, m40_00_0010, several m45_00) | OBSERVED |
| Diffraction ring (LightShaft) | Only m38_00 has DiffractionRing 3.36; others 0; ring radius/attenuation/spectrum fields vary in m32, m33, m35, m38, m53 | OBSERVED |
| Depth of field | On in all 18 areas with mapbase values: ApertuerF 2.0, DofEdgeQuarity 2, DofFocusDistance 1.0, near custom OFF, far custom ON (far 50..10000, CoC size 0.4, scale 0.015), near end 3, near CoC 0.2/0.5. Auto focus (`Enable#3`) 0 always (never non-default). Variants: m30_00_0200 (edge -1, far end 100000, far CoC 1.75; selectable), m32_00_0100 (F 0.8, edge 1, near CoC 1.0, far CoC 1.1, focus delay 1000), m38_00_0002 (F 0.8, edge 1, far CoC 1.1) | OBSERVED |
| Motion blur | Enable 1 everywhere with mapbase values: BlurTimeRatio 0.2, NumBaseSamples 4, MaxBlurLength 0.1, MotionBlurScale 1, MaxRecurrences 2, falloff 0..3, Base Fps 30. Variants differ only in m32_00_0100 (time ratio 0.001, scale 0.01, samples 3, recurrences 3) and m38_00_0002 (time ratio 0.1, recurrences 1) | OBSERVED |
| Lens distortion | Setting Info `Enable LensDistortion` = 1 in all 100 files (default also 1); no lens-distortion strength parameter exists in these groups | OBSERVED / strength unknown |
| Lateral dispersion / chromatic aberration | ToneLateralDispersion[0] = 0.006 in m30_00, m30_01, m34_01, m46_00 bases (0 in m30_00_0100/0110); [1] always 0; ToneUniformDispersion always 0; `Enable Choromaticaberration` 1 everywhere; Tone Map `Enable` 0 and WaveSample 3 everywhere | OBSERVED |
| Film grain | No parameter in any listed group names grain: never present | OBSERVED (absence of name) |
| Heat shimmer | No parameter named shimmer or heat in these groups: never present | OBSERVED (absence of name) |
| Aperture airy disc | `Enable ApertureairyDisc` 0 in all files, never non-default | OBSERVED |
| Anti-alias | Enable 1, AntiAliasType 4 in all 100 files; only DLAA parameters change (m32_00_0100 and m38_00_0002: Ramda 3.0, Epsilon 0.45 vs 1.4/0.12). Meaning of type 4 is not in the data | OBSERVED |
| Glow Filter (extra) | mapbase: Enable 1, new glow on, RgbScale 1.5; m45_00 base: BlurSampleScale 0, RgbScale 3, NewGlow ColorScale 0; variants m32_00_0100, m38_00_0002, m41/m47 0800/0900 off or changed | OBSERVED |
| Lens Simulation (extra) | Enable 1 in 61 files of m30_00, m30_01, m31_00, m32_00, m33_00, m34_01, m38_00, m39_00, m45_00, m46_00, m51_00, m53_00; all values identical (Lens F 5.6, FilmSize 0.024, Shutter Speed 60, FocusDist 100, Near 0..10, Far 100..200, CoCMode 1, UseCameraFocus 1). Absent in m35, m37, m40, m41, m47, m50. Whether the executable reads this group is not established | OBSERVED / usage unknown |
| ColorParaffin Filter (extra) | Enable 0 in all 47 files that hold it | OBSERVED |
| WaterInteraction Filter (extra) | m30_00 Enable 0 (create/delete 80/120, 2048), m33_00 Enable 1 (20/40, 1024); screen-space enable 1 in both | OBSERVED |

## 4. Per-area sheet (18 bases, effective values)

Columns: exp = Exposure, flt = FilmicContrast, sat = ColorS, vig = Vignette Power, LUT = ColorGrading Enable/LutSourceId (OBSERVED, `per-file-sheet.tsv`). Gamma 2.2 and ToneType 2 in all. Bloom: threshold / luminance / levels (radius scale, luminance gamma); shape 1 in all. DoF is on with mapbase numbers (F 2.0, edge 2) in all areas.

| area | exp | flt | sat | vig | hue | LUT | bloom | shafts | lat. disp. | notes |
|---|---|---|---|---|---|---|---|---|---|---|
| m30_00 | 2.3 | 0.15 | 0.85 | 0.34 | 0 | on/2 | 0.3 / 6 / 8 (1.5, 1.0) | on, len 0.76 | 0.006 | 0010 exp 2.0; 0100/0110 auto exposure, LUT off (not selectable); 0200 (selectable) sat 0.75, DoF edge -1, far 100000 |
| m30_01 | 2.2 | 0.12 | 0.8 | 0.3 | 0 | on/2 | 0.3 / 6 / 8 (2.0, 1.0) | off (0010 on) | 0.006 | |
| m31_00 | 2.2 | 0.0 | 0.9 | 0.2 | 0 | on/1 | 0.2 / 8 / 8 (2.0, 1.0) | off (0010/0100/0110 on) | 0 | MiddleGray 0.075 |
| m32_00 | 2.4 | 0.1 | 0.85 | 0.0 | 0 | on/2 | 0.3 / 3 / 8 (1.0, 1.0) | on, len 1.5 (0010/0110 off) | 0 | 0100 (selectable, 5 of 59 collisions): exp 2.0, sat 1.0, shape 3, 10 levels, LUT off, DoF F 0.8, glow off, motion blur near-off, AA ramda 3.0, quality 1; 0010: LUT 1, exp 2.0 |
| m33_00 | 2.3 | 0.25 | 0.85 | 0.38 | 0 | on/2 | 0.23 / 8 / 8 (1.0, 1.0) | off (0500 on) | 0 | own Setting Info (BG colour, GlareQuality 1, DoF quality 1, Enable ColorGrading 0); WaterInteraction on; 0200/0210 filmic 0.4/0.35 |
| m34_01 | 2.0 | 0.13 | 0.8 | 0.3 | 0 | on/1 | 0.3 / 4 / 8 (2.0, 1.0) | off (0010 on) | 0.006 | |
| m35_00 | 2.0 | 0.15 | 0.85 | 0.4 | 0 | on/1 | 0.3 / 8 / 8 (2.0, 1.0) | on, colour (0.88,0.60,0.33), 4th value 6.85, len 2.8 | 0 | strongest vignette |
| m37_00 | 1.52 | 0.3 | 0.9 | 0.0 | 0 | off | 0.5 / 3 / 6 (0.5, 1.4) | on, colour (0.50,0.78,1.0), 4th value 10.0, len 4.0 | 0 | lowest exposure, highest filmic; 0500/0510 (selectable): exp 2.3, flt 0.15, sat 0.85, vig 0.3, LUT 1 on |
| m38_00 | 2.0 | 0.15 | 0.85 | 0.35 | 0 | on/3 | 0.25 / 8 / 8 (2.0, 1.0) | off | 0 | own Setting Info (GlareQuality 4, DoF quality 4); diffraction ring 3.36; 0002 (not selectable): LUT off, vig 0, bloom 2.0/0.3, DoF F 0.8, shaft on |
| m39_00 | 2.3 | 0.15 | 0.84 | 0.3 | 0 | on/3 | 0.28 / 6 / 8 (2.0, 1.0) | on, len 1.8 | 0 | MinExposure 0.001; 0100/0200 (not selectable) LUT off; 0100 exp 2.0 |
| m40_00 | 2.3 | 0.15 | 0.8 | 0.3 | 0 | on/3 | 0.2 / 3 / 6 (2.0, 1.4) | on, len 1.64 (0010 off) | 0 | Firelink/Cemetery; own Setting Info |
| m41_00 | 2.0 | 0.1 | 0.8 | 0.3 | 1.0 | on/1 | 0.25 / 5 / 6 (2.0, 1.4) | on, len 1.0 | 0 | MaxExposure 5, factor 30; 0800 (selectable): exp 0.45, flt 0.5, sat 1.0, hue 0, vig 0, glow off; 0900 (selectable): exp 0.0, bloom thr 0.04, lum 2.5, 8 levels, glow filter zeroed |
| m45_00 | 2.0 | 0.3 | 0.85 | 0.35 | 0 | on/0 | 0.25 / 6 / 6 (2.0, 1.0) | on, colour (0.27,0.76,1.0), 4th value 0.65, len 3.0 | 0 | Glow Filter changed (RgbScale 3, final scale 0); 0100/0400..0800 shaft off; 0300 shaft stronger, glow back to mapbase values |
| m46_00 | 2.0 | 0.13 | 0.8 | 0.3 | 0 | on/1 | 0.3 / 4 / 8 (2.0, 1.0) | off | 0.006 | arena; same numbers as m34_01 |
| m47_00 | 2.0 | 0.1 | 0.8 | 0.3 | 1.0 | on/1 | 0.25 / 1.8 / 6 (2.0, 1.4) | on, len 1.0 | 0 | 0800/0900 same changes as m41_00 0800/0900 (MSB selectable list shows only 0200) |
| m50_00 | 2.0 | 0.1 | 0.9 | 0.35 | 1.0 | on/1 | 0.25 / 5 / 6 (2.0, 1.4) | on, len 1.23 | 0 | Dreg Heap |
| m51_00 | 2.0 | 0.3 | 0.85 | 0.35 | 0 | on/0 | 0.25 / 6 / 6 (2.0, 1.0) | on, colour (1,1,0.5), 4th value 6.0, len 0.8 | 0 | Ringed City |
| m53_00 | 2.4 | 0.1 | 0.85 | 0.0 | 0 | on/2 | 0.3 / 3 / 8 (1.0, 1.0) | on, same shaft as m32_00 | 0 | numerically a copy of m32_00_0000 for these groups (MaxExposure 2) |

Selectability (OBSERVED, `msb-gparam-slots.tsv`, slot N -> `mAA_BB_0N00`): m30_00 0200; m30_01 0100/0110; m31_00 0100/0110; m32_00 0100/0110; m33_00 0100..0700; m35_00 0100..0310; m37_00 0100..0510; m38_00 0100/0200/0600; m39_00 0300/0500; m40_00 0510; m41_00 0200/0800/0900; m45_00 0100..0800; m47_00 0200; m50_00 0100..0300; m51_00 0100/0200. The `selectable_in_msb` column of `per-file-sheet.tsv` follows this list; files that are not selectable (`0010/0020/0030` time or ceremony variants, `m38_00_0002`) still exist in the data.

## 5. Tables per group (base files, 18 areas)

"min..max" is over the 18 `_0000` effective values (vectors not computed). "distinct" counts distinct effective values among the 18. Variant behaviour is in sections 3 and 4 and exhaustively in `variant-diffs.tsv` (503 differing parameter rows across 82 variants; the largest sets are LightShaft, Tone Map, and the m32_00_0100 / m38_00_0002 / m41_00_0900 whole-chain differences).

### Tone Map

| parameter | type | default id0 | mapbase | min..max (18 base files) | distinct | most common | departing areas |
|---|---|---|---|---|---|---|---|
| Exposure | 9 | 1.52 | - | 1.52..2.4 | 5 | 2.0 | m30_00=2.3, m30_01=2.2, m31_00=2.2, m32_00=2.4, m33_00=2.3, m37_00=1.52, m39_00=2.3, m40_00=2.3, m53_00=2.4 |
| Gamma | 9 | 2.2 | - | 2.2..2.2 | 1 | 2.2 |  |
| ToneType | 3 | 2 | - | 2..2 | 1 | 2 |  |
| ToneMappingFactor | 9 | 32.0 | - | 30.0..32.0 | 2 | 32.0 | m41_00=30.0, m47_00=30.0, m50_00=30.0 |
| FilmicContrast | 9 | 0.3 | - | 0.0..0.3 | 7 | 0.15 | m30_01=0.12, m31_00=0.0, m32_00=0.1, m33_00=0.25, m34_01=0.13, m37_00=0.3, m41_00=0.1, m45_00=0.3, m46_00=0.13, m47_00=0.1, m50_00=0.1, m51_00=0.3, m53_00=0.1 |
| ColorH | 9 | 0.0 | - | 0.0..1.0 | 2 | 0.0 | m41_00=1.0, m47_00=1.0, m50_00=1.0 |
| ColorS | 9 | 1.0 | - | 0.8..0.9 | 4 | 0.85 | m30_01=0.8, m31_00=0.9, m34_01=0.8, m37_00=0.9, m39_00=0.84, m40_00=0.8, m41_00=0.8, m46_00=0.8, m47_00=0.8, m50_00=0.9 |
| ColorV | 9 | 1.0 | - | 1.0..1.0 | 1 | 1.0 |  |
| Contrust | 9 | 1.0 | - | 1.0..1.0 | 1 | 1.0 |  |
| AutoExposure | 3 | 0 | - | 0..0 | 1 | 0 |  |
| DelayFrame | 9 | 0.2 | - | 0.01..0.2 | 5 | 0.2 | m30_00=0.06, m30_01=0.06, m31_00=0.01, m33_00=0.12, m34_01=0.06, m38_00=0.12, m40_00=0.15, m41_00=0.15, m45_00=0.01, m46_00=0.06, m47_00=0.15, m50_00=0.15, m51_00=0.01 |
| MiddleGray | 9 | 0.18 | - | 0.02..0.18 | 7 | 0.02 | m30_00=0.028, m30_01=0.028, m31_00=0.075, m32_00=0.1, m34_01=0.028, m35_00=0.05, m37_00=0.18, m39_00=0.03, m45_00=0.075, m46_00=0.028, m51_00=0.075, m53_00=0.1 |
| ToneInfluencedByGlare | 9 | 1.0 | - | 1.0..1.0 | 1 | 1.0 |  |
| MinExposure | 9 | 0.1 | - | 0.001..0.1 | 4 | 0.1 | m30_00=0.05, m30_01=0.05, m32_00=0.05, m34_01=0.05, m35_00=0.01, m39_00=0.001, m46_00=0.05, m53_00=0.05 |
| MaxExposure | 9 | 10.0 | - | 2.0..10.0 | 3 | 10.0 | m32_00=2.0, m40_00=5.0, m41_00=5.0, m47_00=5.0, m50_00=5.0, m53_00=2.0 |
| Exposure MeteringWidth | 9 | 1.0 | - | 1.0..1.0 | 1 | 1.0 |  |
| Exposure MeteringHeight | 9 | 1.0 | - | 1.0..1.0 | 1 | 1.0 |  |
| Exposure MeteringOffsetX | 9 | 0.0 | - | 0.0..0.0 | 1 | 0.0 |  |
| Exposure MeteringOffsetY | 9 | 0.0 | - | 0.0..0.0 | 1 | 0.0 |  |
| AdaptationSensitivity | 9 | 0.8 | - | 0.4..0.8 | 3 | 0.8 | m30_00=0.4, m30_01=0.4, m32_00=0.69, m34_01=0.4, m46_00=0.4, m53_00=0.69 |
| AdaptationScale | 9 | 0.5 | - | 0.5..0.72 | 2 | 0.5 | m32_00=0.72, m53_00=0.72 |
| AdaptationSpeedLimit | 9 | 60.0 | - | 25.0..60.0 | 4 | 30.7801 | m30_00=25.0, m30_01=25.0, m31_00=60.0, m32_00=60.0, m34_01=25.0, m37_00=60.0, m39_00=30.0, m45_00=60.0, m46_00=25.0, m51_00=60.0, m53_00=60.0 |
| DarkAdaptationSensitivity | 9 | 0.1 | - | 0.1..0.6 | 3 | 0.1 | m30_00=0.6, m30_01=0.6, m32_00=0.49, m34_01=0.6, m46_00=0.6, m53_00=0.49 |
| DarkAdaptationScale | 9 | 0.1 | - | 0.0..0.49 | 4 | 0.1 | m30_00=0.0, m30_01=0.0, m31_00=0.45, m32_00=0.49, m34_01=0.0, m39_00=0.45, m45_00=0.45, m46_00=0.0, m51_00=0.45, m53_00=0.49 |
| DarkAdaptationSpeedLimit | 9 | 0.1 | - | 0.1..32.42 | 6 | 32.42 | m30_00=25.0, m30_01=25.0, m31_00=17.04, m32_00=27.65, m34_01=25.0, m37_00=0.1, m39_00=32.4, m45_00=17.04, m46_00=25.0, m51_00=17.04, m53_00=27.65 |
| fAdaptationScale / fDarkAdaptationScale | 9 | 1.0 | - | 0.3..1.0 | 4 | 0.5 | m30_00=0.65, m30_01=0.65, m31_00=0.3, m33_00=1.0, m34_01=0.65, m37_00=1.0, m38_00=1.0, m39_00=1.0, m45_00=0.3, m46_00=0.65, m51_00=0.3 |
| Vignette Power | 9 | 0.0 | - | 0.0..0.4 | 7 | 0.3 | m30_00=0.34, m31_00=0.2, m32_00=0.0, m33_00=0.38, m35_00=0.4, m37_00=0.0, m38_00=0.35, m45_00=0.35, m50_00=0.35, m51_00=0.35, m53_00=0.0 |
| VignetteFovDependence | 9 | 0.0 | - | 0.0..0.0 | 1 | 0.0 |  |
| Enable | 3 | 0 | - | 0..0 | 1 | 0 |  |
| WaveSample | 3 | 3 | - | 3..3 | 1 | 3 |  |
| ToneLateralDispersion[0] | 9 | 0.0 | - | 0.0..0.006 | 2 | 0.0 | m30_00=0.006, m30_01=0.006, m34_01=0.006, m46_00=0.006 |
| ToneLateralDispersion[1] | 9 | 0.0 | - | 0.0..0.0 | 1 | 0.0 |  |
| ToneUniformDispersion[0] | 9 | 0.0 | - | 0.0..0.0 | 1 | 0.0 |  |
| ToneUniformDispersion[1] | 9 | 0.0 | - | 0.0..0.0 | 1 | 0.0 |  |
| Exposure#2 | 9 | 0.0 | - | 0.0..0.0 | 1 | 0.0 |  |
| AvgLuminance | 9 | 1.8 | - | 1.8..1.8 | 1 | 1.8 |  |
| LuminanceBalance R | 9 | 1.0 | - | 1.0..1.0 | 1 | 1.0 |  |
| LuminanceBalance G | 9 | 1.0 | - | 1.0..1.0 | 1 | 1.0 |  |
| LuminanceBalance B | 9 | 1.0 | - | 1.0..1.0 | 1 | 1.0 |  |
| Scaler R | 9 | 1.0 | - | 1.0..1.0 | 1 | 1.0 |  |
| Scaler G | 9 | 1.0 | - | 1.0..1.0 | 1 | 1.0 |  |
| Scaler B | 9 | 1.0 | - | 1.0..1.0 | 1 | 1.0 |  |
| ToneMapMode | 3 | 0 | - | 0..0 | 1 | 0 |  |
| Contrast | 9 | 0.5 | - | 0.5..0.5 | 1 | 0.5 |  |
| Gamma#2 | 9 | 2.2 | - | 2.2..2.2 | 1 | 2.2 |  |
| UseTable | 3 | 1 | - | 1..1 | 1 | 1 |  |
| Enable AutoExposure | 3 | 0 | - | 0..0 | 1 | 0 |  |
| AutoExposure LightAdapt Speed | 9 | 1.0 | - | 1.0..1.0 | 1 | 1.0 |  |
| AutoExposure DarkAdapt Speed | 9 | 0.5 | - | 0.5..0.5 | 1 | 0.5 |  |
| AutoExposure Middle Gray | 9 | -2.0 | - | -2.0..-2.0 | 1 | -2.0 |  |
| AutoExposure Center Weight | 9 | 4.0 | - | 4.0..4.0 | 1 | 4.0 |  |

### Bloom

| parameter | type | default id0 | mapbase | min..max (18 base files) | distinct | most common | departing areas |
|---|---|---|---|---|---|---|---|
| GlareLuminance | 9 | 3.0 | 2.0 | 1.8..8.0 | 6 | 6.0 | m31_00=8.0, m32_00=3.0, m33_00=8.0, m34_01=4.0, m35_00=8.0, m37_00=3.0, m38_00=8.0, m40_00=3.0, m41_00=5.0, m46_00=4.0, m47_00=1.8, m50_00=5.0, m53_00=3.0 |
| GlareThreshold | 9 | 0.5 | 0.5 | 0.2..0.5 | 6 | 0.3 | m31_00=0.2, m33_00=0.23, m37_00=0.5, m38_00=0.25, m39_00=0.28, m40_00=0.2, m41_00=0.25, m45_00=0.25, m47_00=0.25, m50_00=0.25, m51_00=0.25 |
| GlareSamplingThreshold | 9 | 0.002 | 0.01 | 0.01..0.01 | 1 | 0.01 |  |
| RemapFactor | 9 | 0.5 | 0.5 | 0.5..0.5 | 1 | 0.5 |  |
| GlareStarLengthFovDependence | 9 | 0.5 | 0.5 | 0.5..0.5 | 1 | 0.5 |  |
| GlareStarFileterThreshold | 9 | 0.001 | 0.001 | 0.001..0.001 | 1 | 0.001 |  |
| GlareStarSoftness | 9 | 0.25 | 0.25 | 0.25..0.25 | 1 | 0.25 |  |
| GlareShape | 3 | 0 | 1 | 1..1 | 1 | 1 |  |
| GlareNumLevels | 3 | 8 | 8 | 6..8 | 2 | 8 | m37_00=6, m40_00=6, m41_00=6, m45_00=6, m47_00=6, m50_00=6, m51_00=6 |
| GlareBloomGaussianRadiusScale | 9 | 0.5 | 0.5 | 0.5..2.0 | 4 | 2.0 | m30_00=1.5, m32_00=1.0, m33_00=1.0, m37_00=0.5, m53_00=1.0 |
| GlareBloomLuminanceGamma | 9 | 1.0 | 1.0 | 1.0..1.4 | 2 | 1.0 | m37_00=1.4, m40_00=1.4, m41_00=1.4, m47_00=1.4, m50_00=1.4 |
| GlareGhostConcentricDistortion | 9 | 1.0 | 1.0 | 1.0..1.0 | 1 | 1.0 |  |
| GlareLuminance#2 | 9 | 0.0 | 0.0 | 0.0..0.0 | 1 | 0.0 |  |
| BloomLuminance | 9 | 0.0 | 0.0 | 0.0..0.0 | 1 | 0.0 |  |
| BloomDispersion | 9 | 0.0 | 0.0 | 0.0..0.0 | 1 | 0.0 |  |
| BloomDispersionBaseLevel | 7 | 0 | 0 | 0..0 | 1 | 0 |  |
| GhostLuminance | 9 | 0.0 | 0.0 | 0.0..0.0 | 1 | 0.0 |  |
| GhostHaloLuminance | 9 | 0.0 | 0.0 | 0.0..0.0 | 1 | 0.0 |  |
| GhostDistortion | 9 | 0.0 | 0.0 | 0.0..0.0 | 1 | 0.0 |  |
| GhostSharpness | 7 | 0 | 0 | 0..0 | 1 | 0 |  |
| StarLuminance | 9 | 0.0 | 0.0 | 0.0..0.0 | 1 | 0.0 |  |
| StarStreaks | 7 | 0 | 0 | 0..0 | 1 | 0 |  |
| StarLength | 9 | 0.0 | 0.0 | 0.0..0.0 | 1 | 0.0 |  |
| StarSecondaryLength | 9 | 0.0 | 0.0 | 0.0..0.0 | 1 | 0.0 |  |
| StarRotation | 7 | 0 | 0 | 0..0 | 1 | 0 |  |
| StarInclinationAngle | 9 | 0.0 | 0.0 | 0.0..0.0 | 1 | 0.0 |  |
| StarDispersion | 9 | 0.0 | 0.0 | 0.0..0.0 | 1 | 0.0 |  |
| StarForceDispersion | 7 | 0 | 0 | 0..0 | 1 | 0 |  |
| AfterimageLuminance | 9 | 0.0 | 0.0 | 0.0..0.0 | 1 | 0.0 |  |
| AfterimageLength | 9 | 0.0 | 0.0 | 0.0..0.0 | 1 | 0.0 |  |
| Enable | 3 | 0 | 0 | 0..0 | 1 | 0 |  |
| AnamorphicFlareLuminance | 9 | 1.0 | 1.0 | 1.0..1.0 | 1 | 1.0 |  |
| AnamorphicFlareGhostLuminanceScale | 9 | 1.0 | 1.0 | 1.0..1.0 | 1 | 1.0 |  |
| AnamorphicFlareColor | 14 | (1.0,1.0,1.0,1.0) | (1.0,1.0,1.0,1.0) | (vector)  | 1 | (1.0,1.0,1.0,1.0) |  |
| AnamorphicGhostColor | 14 | (1.0,1.0,1.0,1.0) | (1.0,1.0,1.0,1.0) | (vector)  | 1 | (1.0,1.0,1.0,1.0) |  |
| Enable#2 | 3 | 0 | 0 | 0..0 | 1 | 0 |  |
| GlareNumLevels#2 | 7 | 7 | 7 | 7..7 | 1 | 7 |  |
| Glare Luminance | 9 | 1.0 | 1.0 | 1.0..1.0 | 1 | 1.0 |  |
| Glare Threshold | 9 | 0.5 | 0.5 | 0.5..0.5 | 1 | 0.5 |  |
| GlareBloom Alpha | 9 | 1.0 | 1.0 | 1.0..1.0 | 1 | 1.0 |  |
| GlareBloom GaussianRadiusScale | 9 | 1.0 | 1.0 | 1.0..1.0 | 1 | 1.0 |  |
| GlareBloom Sturate Ctrl | 9 | 0.0 | 0.0 | 0.0..0.0 | 1 | 0.0 |  |
| GlareBloom Smooth | 9 | 0.8 | 0.8 | 0.8..0.8 | 1 | 0.8 |  |

### Dof

| parameter | type | default id0 | mapbase | min..max (18 base files) | distinct | most common | departing areas |
|---|---|---|---|---|---|---|---|
| Enable | 3 | 0 | 1 | 1..1 | 1 | 1 |  |
| ApertuerF | 9 | 5.6 | 2.0 | 2.0..2.0 | 1 | 2.0 |  |
| DofFocusDistance | 9 | 1.0 | 1.0 | 1.0..1.0 | 1 | 1.0 |  |
| CCD-Size | 3 | 1110249832 | 1110249832 | 1110249832..1110249832 | 1 | 1110249832 |  |
| DofAdaptiveApertureFactor | 9 | 0.5 | 0.5 | 0.5..0.5 | 1 | 0.5 |  |
| DofAdaptiveApertureBaseFov | 9 | 45.0 | 45.0 | 45.0..45.0 | 1 | 45.0 |  |
| DofRotateScale | 9 | 1.0 | 1.0 | 1.0..1.0 | 1 | 1.0 |  |
| DofRotateOffsetInRadians | 9 | 0.0 | 0.0 | 0.0..0.0 | 1 | 0.0 |  |
| ApertureForntNumLevels | 3 | 3 | 3 | 3..3 | 1 | 3 |  |
| ApertureBackNumLevels | 3 | 3 | 3 | 3..3 | 1 | 3 |  |
| DiaphragmRotate | 3 | 1 | 1 | 1..1 | 1 | 1 |  |
| DofEdgeQuarity | 3 | -1 | 2 | 2..2 | 1 | 2 |  |
| BackgroundMaskThreshold | 9 | 0.1 | 0.1 | 0.1..0.1 | 1 | 0.1 |  |
| Enable#2 | 7 | 0 | 1 | 1..1 | 1 | 1 |  |
| Enable-Near-Custom | 7 | 0 | 0 | 0..0 | 1 | 0 |  |
| DofNearDepthStart | 9 | 0.0 | 0.0 | 0.0..0.0 | 1 | 0.0 |  |
| DofNearDepthEnd | 9 | 2.0 | 3.0 | 3.0..3.0 | 1 | 3.0 |  |
| DofNear CocSize | 9 | 1.0 | 0.2 | 0.2..0.2 | 1 | 0.2 |  |
| DofNear CocScale | 9 | 1.0 | 0.5 | 0.5..0.5 | 1 | 0.5 |  |
| Enable-Far-Custom | 7 | 0 | 1 | 1..1 | 1 | 1 |  |
| DofFarDepthStart | 9 | 100.0 | 50.0 | 50.0..50.0 | 1 | 50.0 |  |
| DofFarDepthEnd | 9 | 150.0 | 10000.0 | 10000.0..10000.0 | 1 | 10000.0 |  |
| DofFar CocSize | 9 | 1.0 | 0.4 | 0.4..0.4 | 1 | 0.4 |  |
| DofFar CocScale | 9 | 1.0 | 0.015 | 0.015..0.015 | 1 | 0.015 |  |
| DofFar DistanceThrethold | 9 | 10000.0 | 20000.0 | 20000.0..20000.0 | 1 | 20000.0 |  |
| Enable#3 | 3 | 0 | 0 | 0..0 | 1 | 0 |  |
| DonotPassOverFocus | 3 | 0 | 0 | 0..0 | 1 | 0 |  |
| FocusSensitivity | 9 | 0.5 | 0.5 | 0.5..0.5 | 1 | 0.5 |  |
| FocusSpeedLimit | 9 | 1.0 | 1.0 | 1.0..1.0 | 1 | 1.0 |  |
| FocusDelay | 9 | 1.0 | 1.0 | 1.0..1.0 | 1 | 1.0 |  |
| FocusDistanceRangeMin | 9 | 0.0 | 0.0 | 0.0..0.0 | 1 | 0.0 |  |
| FocusDistanceRangeMax | 9 | 5000.0 | 5000.0 | 5000.0..5000.0 | 1 | 5000.0 |  |
| FocusScreenPositionX | 9 | 0.5 | 0.5 | 0.5..0.5 | 1 | 0.5 |  |
| FocusScreenPositionY | 9 | 0.5 | 0.5 | 0.5..0.5 | 1 | 0.5 |  |

### LightShaft

| parameter | type | default id0 | mapbase | min..max (18 base files) | distinct | most common | departing areas |
|---|---|---|---|---|---|---|---|
| Enable | 3 | 0 | - | 0..1 | 2 | 1 | m30_01=0, m31_00=0, m33_00=0, m34_01=0, m38_00=0, m46_00=0 |
| ColorScale | 14 | (1.0,1.0,1.0,1.0) | - | (vector)  | 13 | (0.915493,0.878873,0.811268,2.0) | m31_00=(1.0,1.0,1.0,1.0), m32_00=(0.515493,0.715493,1.0,0.901408), m33_00=(0.645429,0.709141,0.753463,2.17175), m35_00=(0.882353,0.596078,0.329412,6.8507), m37_00=(0.501961,0.784314,1.0,10.006), m38_00=(0.944598,0.927978,0.927978,2.39335), m39_00=(1.0,0.894118... (14 areas) |
| ShaftLength | 9 | 1.0 | - | 0.76..4.31 | 12 | 3.0 | m30_00=0.76, m31_00=1.5, m32_00=1.5, m33_00=4.31, m35_00=2.8, m37_00=4.0, m38_00=2.76, m39_00=1.8, m40_00=1.64, m41_00=1.0, m47_00=1.0, m50_00=1.23, m51_00=0.8, m53_00=1.5 |
| GlareRatio | 9 | 0.75 | - | 0.31..1.0 | 11 | 0.4 | m31_00=0.75, m32_00=0.85, m33_00=0.31, m35_00=0.62, m37_00=0.95, m38_00=1.0, m39_00=0.75, m40_00=0.87, m41_00=1.0, m45_00=0.8, m47_00=1.0, m50_00=0.78, m51_00=0.99, m53_00=0.85 |
| AngleAttenuation | 9 | 10.0 | - | 0.0..35.42 | 12 | 12.0 | m31_00=5.0, m32_00=0.01, m33_00=15.58, m35_00=35.42, m37_00=20.0, m38_00=0.82, m39_00=20.64, m40_00=2.5, m41_00=1.0, m45_00=0.0, m47_00=1.0, m50_00=8.75, m51_00=0.0, m53_00=0.01 |
| NoiseMask | 9 | 10.0 | - | 0.0..27.05 | 8 | 0.0 | m30_00=1.0, m30_01=1.0, m31_00=10.0, m32_00=0.1, m33_00=27.05, m34_01=1.0, m38_00=3.28, m39_00=0.01, m41_00=0.5, m46_00=1.0, m47_00=0.5, m50_00=0.5, m53_00=0.1 |
| NoiseFrequency | 9 | 10.0 | - | 0.0..10.0 | 7 | 5.0 | m31_00=1.81, m32_00=5.42, m33_00=3.77, m35_00=0.0, m38_00=1.07, m39_00=10.0, m40_00=0.0, m41_00=10.0, m45_00=1.81, m47_00=10.0, m50_00=10.0, m51_00=0.0, m53_00=5.42 |
| AngleX | 9 | 0.0 | - | 1.501..36.0014 | 12 | 24.0014 | m31_00=25.0808, m32_00=15.0007, m33_00=1.501, m35_00=1.50103, m37_00=17.0, m38_00=25.0808, m39_00=17.4754, m40_00=36.0014, m41_00=12.0014, m45_00=9.0, m47_00=12.0014, m50_00=12.6111, m51_00=20.0, m53_00=15.0007 |
| AngleY | 9 | 0.0 | - | -164.0..150.001 | 14 | -47.9973 | m31_00=-8.85237, m32_00=84.0, m33_00=-86.9993, m34_01=150.001, m35_00=-86.9993, m37_00=-164.0, m38_00=-14.7521, m39_00=55.924, m40_00=120.003, m41_00=14.9993, m45_00=-112.0, m46_00=150.001, m47_00=14.9993, m50_00=126.117, m51_00=96.0, m53_00=113.999 |
| MaskDepthThereshold | 9 | 2000.0 | - | 300.0..5000.0 | 9 | 2000.0 | m30_01=5000.0, m32_00=800.0, m33_00=5000.0, m34_01=5000.0, m35_00=2200.0, m38_00=2910.0, m39_00=300.0, m40_00=400.0, m41_00=1500.0, m46_00=5000.0, m47_00=1500.0, m50_00=800.0, m51_00=1600.0, m53_00=800.0 |
| DiffractionRing | 9 | 0.0 | - | 0.0..3.36 | 2 | 0.0 | m38_00=3.36 |
| DiffractionRingRadius | 9 | 0.2 | - | 0.0..0.811 | 5 | 0.2 | m30_00=0.0, m30_01=0.0, m32_00=0.197, m33_00=0.475, m34_01=0.0, m35_00=0.0, m38_00=0.811, m46_00=0.0, m53_00=0.197 |
| DiffractionRingAttenuation | 9 | 0.01 | - | 0.0..0.84 | 5 | 0.01 | m32_00=0.68, m33_00=0.53, m35_00=0.0, m38_00=0.84, m53_00=0.68 |
| DiffractionRingSpectrumOrder | 9 | 1.0 | - | 0.12..1.0 | 4 | 1.0 | m32_00=0.72, m33_00=0.53, m38_00=0.12, m53_00=0.72 |
| RingOuterColor | 14 | (1.0,1.0,1.0,1.0) | - | (vector)  | 8 | (1.0,1.0,1.0,1.0) | m30_00=(1.0,0.876056,0.529577,2.0), m30_01=(1.0,0.876056,0.529577,2.0), m32_00=(0.930748,0.756233,0.817174,6.62604), m33_00=(0.639889,0.753463,1.0,2.28255), m34_01=(1.0,0.876056,0.529577,2.0), m35_00=(1.0,1.0,1.0,0.540845), m38_00=(0.878116,0.864266,0.869806,6... (11 areas) |

### MotionBlur

| parameter | type | default id0 | mapbase | min..max (18 base files) | distinct | most common | departing areas |
|---|---|---|---|---|---|---|---|
| Enable | 3 | 1 | 1 | 1..1 | 1 | 1 |  |
| BlurTimeRatio | 9 | 0.06 | 0.2 | 0.2..0.2 | 1 | 0.2 |  |
| NumBaseSamples | 3 | 4 | 4 | 4..4 | 1 | 4 |  |
| MaxBlurLength | 9 | 0.1 | 0.1 | 0.1..0.1 | 1 | 0.1 |  |
| MotionBlurScale | 9 | 1.0 | 1.0 | 1.0..1.0 | 1 | 1.0 |  |
| MaxRecurrences | 3 | 1 | 2 | 2..2 | 1 | 2 |  |
| SampleInterleaved | 9 | 1.0 | 1.0 | 1.0..1.0 | 1 | 1.0 |  |
| CameraRotationPerspectiveThreshold | 9 | 0.0 | 0.0 | 0.0..0.0 | 1 | 0.0 |  |
| CameraTranslationThreshold | 9 | 0.0 | 0.0 | 0.0..0.0 | 1 | 0.0 |  |
| FalloffDistanceMin | 9 | 2.0 | 0.0 | 0.0..0.0 | 1 | 0.0 |  |
| FalloffDistanceMax | 9 | 0.0 | 3.0 | 3.0..3.0 | 1 | 3.0 |  |
| Bidirectional PermissionEnable | 3 | 1 | 1 | 1..1 | 1 | 1 |  |
| Enable#2 | 5 | 0 | 0 | 0..0 | 1 | 0 |  |
| Motion Blur Intensity | 9 | 0.0 | 0.0 | 0.0..0.0 | 1 | 0.0 |  |
| Object Blur Intensity | 9 | 1.0 | 1.0 | 1.0..1.0 | 1 | 1.0 |  |
| Camera Blur Intensity | 9 | 1.0 | 1.0 | 1.0..1.0 | 1 | 1.0 |  |
| Blur ReccurenceCount | 7 | 1 | 1 | 1..1 | 1 | 1 |  |
| Use Inner Simulation Time | 5 | 1 | 1 | 1..1 | 1 | 1 |  |
| Enable Average Inner Simulation Time | 5 | 1 | 1 | 1..1 | 1 | 1 |  |
| Enable Fixed Inner Simulation Time | 5 | 0 | 0 | 0..0 | 1 | 0 |  |
| Base Fps | 9 | 30.0 | 30.0 | 30.0..30.0 | 1 | 30.0 |  |

### ColorGrading

| parameter | type | default id0 | mapbase | min..max (18 base files) | distinct | most common | departing areas |
|---|---|---|---|---|---|---|---|
| Enable | 3 | 0 | - | 0..1 | 2 | 1 | m37_00=0 |
| LutSourceId | 3 | -1 | - | -1..3 | 5 | 1 | m30_00=2, m30_01=2, m32_00=2, m33_00=2, m37_00=-1, m38_00=3, m39_00=3, m40_00=3, m45_00=0, m51_00=0, m53_00=2 |

### Anti-Alias

| parameter | type | default id0 | mapbase | min..max (18 base files) | distinct | most common | departing areas |
|---|---|---|---|---|---|---|---|
| Enable | 3 | 1 | 1 | 1..1 | 1 | 1 |  |
| AntiAliasType | 3 | 4 | 4 | 4..4 | 1 | 4 |  |
| FxaaAlpha | 9 | 1.0 | 1.0 | 1.0..1.0 | 1 | 1.0 |  |
| FxaaReduceMin | 9 | 0.0078125 | 0.0078125 | 0.0078125..0.0078125 | 1 | 0.0078125 |  |
| FxaaReduceMul | 9 | 0.125 | 0.125 | 0.125..0.125 | 1 | 0.125 |  |
| FxaaSpanMax | 9 | 8.0 | 8.0 | 8.0..8.0 | 1 | 8.0 |  |
| DlaaThreshold | 9 | 0.1 | 0.1 | 0.1..0.1 | 1 | 0.1 |  |
| DlaaRamda | 9 | 1.4 | 1.4 | 1.4..1.4 | 1 | 1.4 |  |
| DlaaEpsilon | 9 | 0.12 | 0.12 | 0.12..0.12 | 1 | 0.12 |  |

### FeedBackBlur

| parameter | type | default id0 | mapbase | min..max (18 base files) | distinct | most common | departing areas |
|---|---|---|---|---|---|---|---|
| Enable | 3 | 0 | - | 0..0 | 1 | 0 |  |
| FeedbackWeight | 9 | 0.0 | - | 0.0..0.0 | 1 | 0.0 |  |
| FeedbackUnitTimeInSeconds | 9 | 0.0 | - | 0.0..0.0 | 1 | 0.0 |  |
| FeedbackAspectRatio | 9 | 0.0 | - | 0.0..0.0 | 1 | 0.0 |  |
| FeedbackRate | 9 | 0.8 | - | 0.8..0.8 | 1 | 0.8 |  |
| FeedbackBlurFallOff | 9 | 0.9 | - | 0.9..0.9 | 1 | 0.9 |  |
| FeedbackBlurPower | 9 | 4.5 | - | 4.5..4.5 | 1 | 4.5 |  |

### Glow Filter

| parameter | type | default id0 | mapbase | min..max (18 base files) | distinct | most common | departing areas |
|---|---|---|---|---|---|---|---|
| Enable | 3 | - | 1 | 1..1 | 1 | 1 |  |
| BlurSampleScale | 9 | - | 1.0 | 0.0..1.0 | 2 | 1.0 | m45_00=0.0 |
| Blur RgbScale | 9 | - | 1.5 | 1.5..3.0 | 2 | 1.5 | m45_00=3.0 |
| Blur AlphaScale | 9 | - | 1.0 | 1.0..1.0 | 1 | 1.0 |  |
| New Glow Enable | 3 | - | 1 | 1..1 | 1 | 1 |  |
| NewGlow Final ColorScale | 9 | - | 0.5 | 0.0..0.5 | 2 | 0.5 | m45_00=0.0 |

### Setting Info

| parameter | type | default id0 | mapbase | min..max (18 base files) | distinct | most common | departing areas |
|---|---|---|---|---|---|---|---|
| BG Color | 14 | (0.5,0.5,0.5,0.5) | (0.078431,0.078431,0.078431,1.0) | (vector)  | 5 | (0.078431,0.078431,0.078431,1.0) | m33_00=(0.230986,0.216901,0.2,0.5), m38_00=(0.0,0.0,0.0,0.0), m39_00=(0.0,0.0,0.0,1.0), m40_00=(0.5,0.5,0.5,0.5), m41_00=(0.5,0.5,0.5,0.5), m47_00=(0.5,0.5,0.5,0.5), m50_00=(0.5,0.5,0.5,0.5) |
| Use DepthTexture | 3 | 0 | 0 | 0..0 | 1 | 0 |  |
| Enable LensDistortion | 3 | 1 | 1 | 1..1 | 1 | 1 |  |
| Enable ApertureairyDisc | 3 | 0 | 0 | 0..0 | 1 | 0 |  |
| Enable ApertureairyUnlimitedRadius | 3 | 1 | 1 | 1..1 | 1 | 1 |  |
| Enable MotionBlur-Dof | 3 | 1 | 1 | 1..1 | 1 | 1 |  |
| Enable MotionBlurUnlimitedrecursion | 3 | 0 | 0 | 0..0 | 1 | 0 |  |
| Create MotionBlurInternalVelocityTex | 3 | 1 | 1 | 1..1 | 1 | 1 |  |
| Enable Gaussian | 3 | 0 | 0 | 0..0 | 1 | 0 |  |
| Enable FeedBackEffect | 3 | 1 | 1 | 1..1 | 1 | 1 |  |
| Enable Anti-Alias | 3 | 1 | 1 | 1..1 | 1 | 1 |  |
| Enable LightShaft | 3 | 1 | 1 | 1..1 | 1 | 1 |  |
| Enable Choromaticaberration | 3 | 1 | 1 | 1..1 | 1 | 1 |  |
| Enable AnamophicLensAspectratio | 3 | 1 | 1 | 1..1 | 1 | 1 |  |
| Enable AnamorphicLensAperture | 3 | 1 | 1 | 1..1 | 1 | 1 |  |
| Enable ColorGrading | 3 | 0 | 1 | 0..1 | 2 | 1 | m33_00=0 |
| SceneLuminanceScale | 9 | 1.0 | 1.0 | 1.0..1.0 | 1 | 1.0 |  |
| BlurShape | 3 | 0 | 0 | 0..0 | 1 | 0 |  |
| GlareQuality | 3 | 4 | 2 | 1..4 | 3 | 2 | m33_00=1, m38_00=4, m39_00=4, m40_00=4, m41_00=4, m47_00=4, m50_00=4 |
| DepthOfFieldQuality | 3 | 7 | 3 | 1..5 | 4 | 3 | m33_00=1, m38_00=4, m39_00=4, m40_00=4, m41_00=5, m47_00=5, m50_00=5 |
| Auto Compute Enable | 5 | 0 | 1 | 0..1 | 2 | 1 | m33_00=0, m38_00=0, m40_00=0, m41_00=0, m47_00=0, m50_00=0 |
| BaseDist | 9 | 10.0 | 10.0 | 10.0..10.0 | 1 | 10.0 |  |
| DistScaler | 9 | 10.0 | 10.0 | 10.0..10.0 | 1 | 10.0 |  |
| DistMax | 9 | 130.0 | 130.0 | 130.0..130.0 | 1 | 130.0 |  |
| ReflectScene Assign DynamicLight | 5 | 0 | 0 | 0..0 | 1 | 0 |  |

### Lens Simulation

| parameter | type | default id0 | mapbase | min..max (18 base files) | distinct | most common | departing areas |
|---|---|---|---|---|---|---|---|
| Enable | 5 | - | - | 1..1 | 2 | 1 | m35_00=None, m37_00=None, m40_00=None, m41_00=None, m47_00=None, m50_00=None |
| NearEnd | 9 | - | - | 0.0..0.0 | 2 | 0.0 | m35_00=None, m37_00=None, m40_00=None, m41_00=None, m47_00=None, m50_00=None |
| NearStart | 9 | - | - | 10.0..10.0 | 2 | 10.0 | m35_00=None, m37_00=None, m40_00=None, m41_00=None, m47_00=None, m50_00=None |
| FarEnd | 9 | - | - | 200.0..200.0 | 2 | 200.0 | m35_00=None, m37_00=None, m40_00=None, m41_00=None, m47_00=None, m50_00=None |
| FarStart | 9 | - | - | 100.0..100.0 | 2 | 100.0 | m35_00=None, m37_00=None, m40_00=None, m41_00=None, m47_00=None, m50_00=None |
| FocusDist | 9 | - | - | 100.0..100.0 | 2 | 100.0 | m35_00=None, m37_00=None, m40_00=None, m41_00=None, m47_00=None, m50_00=None |
| UseCameraFocus | 5 | - | - | 1..1 | 2 | 1 | m35_00=None, m37_00=None, m40_00=None, m41_00=None, m47_00=None, m50_00=None |
| SkyMaskRate | 9 | - | - | 0.0..0.0 | 2 | 0.0 | m35_00=None, m37_00=None, m40_00=None, m41_00=None, m47_00=None, m50_00=None |
| Lens F | 9 | - | - | 5.6..5.6 | 2 | 5.6 | m35_00=None, m37_00=None, m40_00=None, m41_00=None, m47_00=None, m50_00=None |
| Av (絞り値) | 9 | - | - | 1.0..1.0 | 2 | 1.0 | m35_00=None, m37_00=None, m40_00=None, m41_00=None, m47_00=None, m50_00=None |
| FilmSize(mm) | 9 | - | - | 0.024..0.024 | 2 | 0.024 | m35_00=None, m37_00=None, m40_00=None, m41_00=None, m47_00=None, m50_00=None |
| ApertureShape | 3 | - | - | 0..0 | 2 | 0 | m35_00=None, m37_00=None, m40_00=None, m41_00=None, m47_00=None, m50_00=None |
| DiaphragmRotate | 9 | - | - | 0.0..0.0 | 2 | 0.0 | m35_00=None, m37_00=None, m40_00=None, m41_00=None, m47_00=None, m50_00=None |
| CoCBlurCountPreMask | 7 | - | - | 0..0 | 2 | 0 | m35_00=None, m37_00=None, m40_00=None, m41_00=None, m47_00=None, m50_00=None |
| CoCBlurCount(1) | 7 | - | - | 1..1 | 2 | 1 | m35_00=None, m37_00=None, m40_00=None, m41_00=None, m47_00=None, m50_00=None |
| CoCBlurCount(2) | 7 | - | - | 2..2 | 2 | 2 | m35_00=None, m37_00=None, m40_00=None, m41_00=None, m47_00=None, m50_00=None |
| CoCMode | 3 | - | - | 1..1 | 2 | 1 | m35_00=None, m37_00=None, m40_00=None, m41_00=None, m47_00=None, m50_00=None |
| Motion Blur Intensity | 9 | - | - | 1.0..1.0 | 2 | 1.0 | m35_00=None, m37_00=None, m40_00=None, m41_00=None, m47_00=None, m50_00=None |
| Shutter Speed | 9 | - | - | 60.0..60.0 | 2 | 60.0 | m35_00=None, m37_00=None, m40_00=None, m41_00=None, m47_00=None, m50_00=None |
| Solve Single Step | 5 | - | - | 0..0 | 2 | 0 | m35_00=None, m37_00=None, m40_00=None, m41_00=None, m47_00=None, m50_00=None |

### ColorParaffin Filter

| parameter | type | default id0 | mapbase | min..max (18 base files) | distinct | most common | departing areas |
|---|---|---|---|---|---|---|---|
| Enable | 7 | - | - | 0..0 | 2 | 0 | m33_00=None, m35_00=None, m37_00=None, m40_00=None, m41_00=None, m47_00=None, m50_00=None |
| Mode | 3 | - | - | 0..0 | 2 | 0 | m33_00=None, m35_00=None, m37_00=None, m40_00=None, m41_00=None, m47_00=None, m50_00=None |
| CenterPos | 12 | - | - | (vector)  | 2 | (0.0,0.0) | m33_00=None, m35_00=None, m37_00=None, m40_00=None, m41_00=None, m47_00=None, m50_00=None |
| Radius | 9 | - | - | 0.15..0.15 | 2 | 0.15 | m33_00=None, m35_00=None, m37_00=None, m40_00=None, m41_00=None, m47_00=None, m50_00=None |
| StartColor | 14 | - | - | (vector)  | 2 | (0.0,0.0,0.0,0.0) | m33_00=None, m35_00=None, m37_00=None, m40_00=None, m41_00=None, m47_00=None, m50_00=None |
| EndColor | 14 | - | - | (vector)  | 2 | (0.0,0.0,0.0,0.0) | m33_00=None, m35_00=None, m37_00=None, m40_00=None, m41_00=None, m47_00=None, m50_00=None |
| Curvature | 9 | - | - | 0.5625..0.5625 | 2 | 0.5625 | m33_00=None, m35_00=None, m37_00=None, m40_00=None, m41_00=None, m47_00=None, m50_00=None |

### WaterInteraction Filter

| parameter | type | default id0 | mapbase | min..max (18 base files) | distinct | most common | departing areas |
|---|---|---|---|---|---|---|---|
| Enable | 7 | - | - | 0..1 | 3 | None | m30_00=0, m33_00=1 |
| Screen Space Enable | 7 | - | - | 1..1 | 2 | None | m30_00=1, m33_00=1 |
| Create Distance | 9 | - | - | 20.0..80.0 | 3 | None | m30_00=80.0, m33_00=20.0 |
| Delete Distance | 9 | - | - | 40.0..120.0 | 3 | None | m30_00=120.0, m33_00=40.0 |
| MaxResolution | 7 | - | - | 1024..2048 | 3 | None | m30_00=2048, m33_00=1024 |
| ScreenSpace TexelSize | 9 | - | - | 0.02..0.02 | 2 | None | m30_00=0.02, m33_00=0.02 |
| ScreenSpace Max Resolution | 7 | - | - | 1024..2048 | 3 | None | m30_00=2048, m33_00=1024 |

## 6. Reader check and limits

Spot-check (OBSERVED): `m40_00_0000` Exposure 2.3, ColorS 0.8, Vignette Power 0.3 (hex 40133333, 3F4CCCCD, 3E99999A) match STUDY-YEBIS-REFUSALS; `m37_00_0000` Exposure 1.52 (3FC28F5C) matches; STUDY table A grading ids and glare shapes also agree (m37_00 grading off, m32_00_0100 shape 3 with 10 levels). A first reader pass failed these checks because the TSV holds duplicate parameter names inside one group (a second `Exposure` = 0 overwrote the first); fixed by labelling the second occurrence `#2`.

Limits and honesty notes:
- The TSV `floats` column uses a decimal comma and a comma separator; I decoded only the `hex` column.
- Semantics not established (names only): GlareShape values, the Bloom `Enable` entries (0 everywhere while bloom is drawn), Tone Map `Enable` and `WaveSample`, Dof `Enable#2`, AntiAliasType 4, Setting Info `Enable ColorGrading` (0 at m33_00 base yet ColorGrading Enable 1 there), the fourth value of LightShaft ColorScale.
- The `Exposure#2` / `AvgLuminance` block (entries 34..50) is constant across the world; whether the executable reads it is not covered.
- "Effective" applies a per-parameter fallback own file, base, mapbase, default; a variant that holds a group partially is completed from lower layers. STUDY's group-level inheritance gives the same result for groups wholly held or wholly absent; partial groups were not compared separately.
- Whether Lens Simulation, ColorParaffin and WaterInteraction are consumed by the native path was not examined.
- No film grain, heat shimmer, lens flare or lens-distortion strength parameter exists in these files; this is an absence of names, so a feature implemented in code without GPARAM data is outside this census.
