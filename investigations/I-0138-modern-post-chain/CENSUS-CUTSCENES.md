# I-0138 CENSUS-CUTSCENES: what the Remo cutscenes ask of the post-processing (YEBIS)

Date 2026-11-12. Made by a Sonnet worker (read-only, nothing launched). **Non-canonical until the pilot re-reads it.**
Tags: **OBSERVED** = read in a named file; **INFERRED** = deduced, not read. Parameter meanings are not guessed beyond their names.
Sources (all read-only): `evidence/I-0066/queries/gparam-all/param_drawparam_s*.tsv` (49 cutscene GPARAM files), the same folder's `m*`, `default`, `mapbase`, `remobase` (comparators), `evidence/I-0069/queries/remo/{remobnd-census,remobnd-contents-witnesses,mqb-strings-s30_00_0030,mqb-strings-s45_00_0000,emevd-cutscene-calls}.txt`, `investigations/I-0069-static-menu-audio-map/REMO-CUTSCENES-NATIVE.md`, `investigations/I-0113-open-gaps/STUDY-YEBIS-REFUSALS.md` (section 3 B). Scripts and raw tables: `evidence/I-0138/artifacts/census-cutscenes/`.

## 0. Answers first

1. **Every one of the 49 cutscene GPARAM files carries Tone Map (146 value ids), Bloom (129 ids) and Dof (232 ids)**; LightShaft 48 files (72 ids), ColorGrading 48 files (51 ids), Setting Info 32 files, FeedBackBlur 26 files (26 ids), MotionBlur 3 files (5 ids), Anti-Alias 1 file (`s30_00_0010`), plus Lens Simulation 19, ColorParaffin 9, Glow Filter 3 files (OBSERVED, `groups-per-file.json`, `cutscene-vs-map-ranges.tsv`).
2. **Depth of field is the cutscene feature**: Dof Enable (first, type 3) is 1 in 208 of 232 ids in all 49 files, against 6 world-map records plus `mapbase`/`remobase`; the focus distance is set per id (139 of 232 ids differ from 1; range 0.1 to 200), aperture F 0.01 to 80, CCD size 28.77 / 43.27 / 84.75 (maps: 43.27 only). Dof Enable (type 7) is on in 9 ids (7 files); near-custom in 7 ids, far-custom in 8 ids. The auto-focus terms (Focus*, DonotPassOverFocus) stay at their defaults in every cutscene id.
3. **Tone Map values used only by cutscenes (zero in every world-map file and in default/mapbase/remobase)**: ColorV 0 (8 ids, 8 files: fade to black, INFERRED from the name), Gamma 0 (3 ids), ToneType 3 (2 ids, `s40_00_0040` ids 0 and 1), field-of-view vignette (VignetteFovDependence != 0: 58 ids, 13 files), Vignette Power above 0.5 (13 ids; max 1.35), Exposure above 2.5 (13 ids; max 5.09), Gamma other than 2.2 and 0 (9 ids; 1.8 to 4.62), Contrust != 1 (28 ids; 0.92 to 1.1625), ToneMappingFactor 0.01 (13 ids, 3 files).
4. **Exposure 0 and ColorV above 1 are not strictly cutscene-only**: exposure 0 also appears in 2 world-map records, ColorV above 1 in 2 map records. ColorH != 0 is in 19 cutscene ids (5 files; up to 3.0) and 3 map files.
5. **Glare**: GlareShape 0 in 26 ids (15 files), shape 1 in 103 ids; no cutscene uses shape 2 or 3. Ghost, afterimage and star-length terms are 0 everywhere; only `s41_00_0000` id 0 uses BloomLuminance 35.05, StarLuminance 1.97, BloomDispersion 1.88 and Bloom Enable 1. `s34_01_0010` has GlareLuminance up to 86.68.
6. **Never used by cutscenes**: FeedBackBlur (26 ids, Enable 0 and Weight 0 in all), ColorParaffin (Enable 0 in all 9), light-shaft colour scale above 8, ghost/afterimage glare. Used little: MotionBlur on in 3 ids (`s30_00_0010` id 0, `s45_00_0000` and `s45_00_0010` id 100), Anti-Alias 1 id, Glow Filter 3 files.
7. **Timelines** (OBSERVED, `remobnd-census.txt`): 50 of 72 bundles carry the SceneDrawParam group-switch properties (ToneMap, Dof, Bloom, ColorGrading, LightShaft, FeedbackBlur, MotionBlur, Antialias, Ssao, DofGlareQuality...); 32 carry `beginFadeCol`/`endFadeCol`; 13 carry `DOF` objects; 2 carry `DoFocus` (+1 `DofFoucs`). Which GPARAM ids each cut selects is in the MQB data and was **not recoverable** (no raw bundles available).
8. **Spot check of the reader passed** (section 1): the three values of STUDY table B reproduce, and the STUDY's refusal totals (58 ids / 13 files fov vignette, 39 / 12 ColorV != 1, 4 / 4 exposure or gamma 0, 2 / 1 tone type 3) reproduce exactly.
9. **Cutscene id to place**: 35 cutscene ids are called by EMEVD bank 2002 in `emevd-cutscene-calls.txt` (section 5); only three have a role read in the inputs; 19 of the 49 GPARAM s-files have no call in that extract.
10. **Reader caveat found while building this**: the Tone Map group lists some names twice in one id (second `Exposure`, `Gamma`, plus `Dof` `Enable`, `Bloom` `GlareLuminance`, `Enable`): two parameter runs concatenated. Reading the TSV by name alone overwrites the first run with the second (the second `Exposure` is always 0). This census keeps both (`#2` suffix). Any earlier reader should be checked for the same overwrite.

## 1. Method, reader and spot check

- **Columns** (OBSERVED, header of every TSV): `group, param, type, id, floats, hex`. The `floats` column uses a decimal comma and a comma between components ("1,2" is 1.2; "-0,575959,1,55334" is two values), so the reader decodes the `hex` column (big-endian words as printed): type 9 = float32, 3 and 7 = int32, 5 and 11 = byte, 2 = uint16, 12 = 2 floats, 14 = 4 floats, 15 = 4 bytes. `Dof CCD-Size` is typed 3 but holds a float32 bit pattern (decoded as float: 28.77 / 43.27 / 84.75; INFERRED reading, the map files show 43.267 throughout).
- **Value ids** (OBSERVED): inside an s-file each group lists its ids (0, 10, 100, 101, ...); a "record" below is one (file, group, id).
- **Two parameter runs** (OBSERVED, `s40_00_0040.tsv` lines 347 and 483): Tone Map, Bloom and Dof repeat some names inside one id (`Exposure`, `Gamma`, `Enable`, `GlareLuminance`): the second occurrence is stored as `name#2`. Tone Map columns B are the first run (the one STUDY-YEBIS-REFUSALS discusses); B2 shows the second run. What the second run is, was not established (INFERRED: a second tone-map parameter set; its Exposure is 0 in all 146 ids, Gamma 2.2, ToneMapMode 0, UseTable 1).
- **Comparators**: "world map files" = all `param_drawparam_m<digits>*.tsv` files (107 files, including the test files `m21_00_0000` and `m22_00_0000`; the templates are excluded), "global" = `default`, `mapbase`, `remobase`.
- **Spot check against STUDY-YEBIS-REFUSALS table B** (script `spotcheck.py`, `crosscheck.py`): `s40_00_0040` ToneType ids {0: 3, 1: 3, 10: 2, 100: 2} and ColorV {0: 0.6, 1: 0.73, 10: 0, 100: 1} match "tone type 3, ColorV 0.6 / 0.73 / 0"; `s45_00_0000` id 113 ColorV = 4 matches; `s41_00_0070` id 100 Vignette Power 1.35, VignetteFovDependence 0.99 matches. Totals over the 49 files: fov-dependent vignette 58 ids / 13 files, ColorV != 1 39 / 12, exposure or gamma 0 4 / 4, tone type 3 2 / 1, 146 Tone Map records: all equal to STUDY section 0 point 2. **Check passed.** The check exposed the duplicate-name overwrite described in answers-first point 10 (it did not affect the STUDY's totals, since those columns read the first run).

## 2. Per cutscene file (49 s-files)

All 49 files carry Tone Map, Bloom and Dof (49 / 41 / 33 parameters per id; `s30_00_0010` and `s30_00_9000` have 48 Tone Map parameters (no `AutoExposure LightAdapt Speed`) and 24 or 25 Setting Info parameters; the counts are in `groups-per-file.json`). Ranges are min..max over the file's ids. "-" = group or parameter absent from the file. Raw per-parameter table: `ranges-per-file.tsv` (12 118 rows); cutscene vs map ranges: `cutscene-vs-map-ranges.tsv`.
Contrast: the Tone Map "Contrust" column is spelled as in the data. In Table C the "non-zero" cell lists ghost, halo, star, afterimage and dispersion terms that are above 0 in the file; "-" means all are 0.


### A. Value ids per post group (count of distinct ids)

| file | Setting Info | Tone Map | Bloom | Dof | LightShaft | MotionBlur | FeedBackBlur | ColorGrading | Anti-Alias |
|---|---|---|---|---|---|---|---|---|---|
| s30_00_0000 | 1 | 2 | 2 | 10 | 2 | - | - | 2 | - |
| s30_00_0010 | 1 | 1 | 1 | 8 | 6 | 1 | 1 | 1 | 1 |
| s30_00_0020 | 1 | 1 | 6 | 6 | 1 | - | - | 1 | - |
| s30_00_0030 | 1 | 1 | 1 | 4 | 1 | - | - | 1 | - |
| s30_00_0040 | 1 | 1 | 5 | 5 | 5 | - | - | 1 | - |
| s30_00_9000 | 1 | 1 | 1 | 2 | 1 | - | 1 | 1 | - |
| s31_00_0000 | - | 3 | 3 | 8 | - | - | - | - | - |
| s31_00_0010 | - | 3 | 2 | 5 | 1 | - | 1 | 2 | - |
| s32_00_0000 | - | 2 | 4 | 4 | 1 | - | 1 | 1 | - |
| s32_00_0010 | 1 | 1 | 8 | 9 | 1 | - | - | 1 | - |
| s32_00_0020 | 1 | 1 | 1 | 6 | 1 | - | - | 1 | - |
| s32_00_0030 | - | 1 | 1 | 1 | 1 | - | 1 | 1 | - |
| s33_00_0000 | - | 1 | 1 | 5 | 1 | - | 1 | 1 | - |
| s33_00_0010 | 1 | 1 | 1 | 4 | 1 | - | - | 1 | - |
| s33_00_0020 | - | 1 | 1 | 4 | 1 | - | 1 | 1 | - |
| s33_00_0021 | - | 1 | 1 | 4 | 1 | - | 1 | 1 | - |
| s33_00_0022 | - | 1 | 1 | 4 | 1 | - | 1 | 1 | - |
| s33_00_0030 | - | 1 | 1 | 2 | 1 | - | 1 | 1 | - |
| s33_00_0031 | - | 1 | 1 | 2 | 1 | - | 1 | 1 | - |
| s33_00_0032 | - | 1 | 1 | 2 | 1 | - | 1 | 1 | - |
| s33_00_0040 | - | 1 | 1 | 4 | 1 | - | 1 | 1 | - |
| s33_00_0041 | - | 1 | 1 | 4 | 1 | - | 1 | 1 | - |
| s33_00_0042 | - | 1 | 1 | 4 | 1 | - | 1 | 1 | - |
| s34_01_0000 | - | 1 | 7 | 9 | 1 | - | 1 | 1 | - |
| s34_01_0010 | 1 | 6 | 7 | 8 | 1 | - | 1 | 1 | - |
| s35_00_0000 | 1 | 1 | 3 | 3 | 1 | - | - | 1 | - |
| s35_00_0001 | 1 | 1 | 3 | 3 | 1 | - | - | 1 | - |
| s37_00_0020 | 1 | 1 | 5 | 5 | 5 | - | - | 1 | - |
| s37_00_0021 | 1 | 1 | 5 | 5 | 5 | - | - | 1 | - |
| s37_00_0030 | - | 1 | 1 | 1 | 1 | - | 1 | 1 | - |
| s38_00_0000 | 1 | 1 | 1 | 3 | 1 | - | - | 1 | - |
| s38_00_0010 | 1 | 1 | 1 | 3 | 1 | - | - | 1 | - |
| s38_00_0020 | 1 | 1 | 1 | 4 | 1 | - | - | 1 | - |
| s39_00_0010 | 1 | 1 | 1 | 2 | 1 | - | - | 1 | - |
| s39_00_0020 | 1 | 1 | 1 | 3 | 1 | - | - | 1 | - |
| s39_00_0030 | 1 | 1 | 1 | 1 | 1 | - | 1 | 1 | - |
| s40_00_0020 | 2 | 7 | 2 | 7 | 1 | - | 1 | 1 | - |
| s40_00_0021 | 2 | 7 | 2 | 7 | 1 | - | 1 | 1 | - |
| s40_00_0040 | 1 | 4 | 1 | 6 | 1 | - | 1 | 1 | - |
| s41_00_0000 | 1 | 2 | 1 | 1 | 1 | - | - | 1 | - |
| s41_00_0050 | 1 | 11 | 4 | 4 | 1 | - | 1 | 1 | - |
| s41_00_0051 | 1 | 11 | 4 | 4 | 1 | - | 1 | 1 | - |
| s41_00_0060 | 1 | 5 | 3 | 3 | 1 | - | - | 1 | - |
| s41_00_0070 | 1 | 4 | 3 | 3 | 1 | - | 1 | 1 | - |
| s45_00_0000 | 3 | 14 | 10 | 12 | 1 | 2 | - | 1 | - |
| s45_00_0010 | 1 | 14 | 10 | 12 | 1 | 2 | - | 1 | - |
| s45_00_0030 | 4 | 5 | 1 | 5 | 1 | - | - | 1 | - |
| s45_00_0031 | 4 | 5 | 1 | 5 | 1 | - | - | 1 | - |
| s51_00_0000 | - | 10 | 4 | 6 | 7 | - | 1 | 2 | - |

### B. Tone Map (first parameter run: Exposure ... Enable); ranges across the file's ids

| file | ids | Exposure | Gamma | ToneType | ToneMappingFactor | FilmicContrast | ColorH | ColorS | ColorV | Contrust | Vignette Power | VignetteFov | AutoExposure | TM Enable |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| s30_00_0000 | 2 | 2..2.75 | 2.2 | 2 | 32 | 0 | 0 | 0.9 | 1 | 1 | 0.2 | 0 | 0 | 0 |
| s30_00_0010 | 1 | 1 | 2.2 | 2 | 32 | 0.2 | 0 | 0.75 | 0.9 | 1 | 0.4 | 0.5 | 0 | 0 |
| s30_00_0020 | 1 | 2 | 2.2 | 2 | 32 | 0.5 | 0 | 1 | 1 | 1 | 0 | 0 | 0 | 0 |
| s30_00_0030 | 1 | 2 | 2.2 | 2 | 32 | 0.5 | 0 | 1 | 1 | 1 | 0 | 0 | 0 | 0 |
| s30_00_0040 | 1 | 2 | 2.2 | 2 | 32 | 0.5 | 0 | 1 | 1 | 1 | 0 | 0 | 0 | 0 |
| s30_00_9000 | 1 | 1 | 2.2 | 2 | 32 | 0.1 | 0 | 0.75 | 1 | 1 | 0.3 | 0 | 0 | 0 |
| s31_00_0000 | 3 | 2.1..2.5 | 2.2 | 2 | 32 | 0.28..0.32 | 0 | 0.9 | 1 | 0.985..1 | 0.139..0.35 | 0.05..0.11 | 0 | 0 |
| s31_00_0010 | 3 | 2.2..2.3 | 2.2 | 2 | 32 | 0..0.15 | 0 | 0.75..0.9 | 1 | 1 | 0.2..0.34 | 0 | 0 | 0 |
| s32_00_0000 | 2 | 2 | 2.2 | 2 | 32 | 0..0.5 | 0 | 0.85..1 | 1 | 1 | 0 | 0 | 0 | 0 |
| s32_00_0010 | 1 | 2 | 2.2 | 2 | 32 | 0.5 | 0 | 1 | 1 | 1 | 0 | 0 | 0 | 0 |
| s32_00_0020 | 1 | 2 | 2.2 | 2 | 32 | 0.5 | 0 | 1 | 1 | 1 | 0 | 0 | 0 | 0 |
| s32_00_0030 | 1 | 2.2 | 2.2 | 2 | 32 | 0 | 0 | 0.9 | 1 | 1 | 0.4 | 0 | 0 | 1 |
| s33_00_0000 | 1 | 2.3 | 2.2 | 2 | 32 | 0.2 | 0 | 0.85 | 1 | 1 | 0.3 | 0 | 0 | 0 |
| s33_00_0010 | 1 | 1.6 | 2.2 | 2 | 32 | 0.5 | 0 | 1 | 1 | 1.005 | 0.559 | 0.52 | 0 | 0 |
| s33_00_0020 | 1 | 2.3 | 2.2 | 2 | 32 | 0.2 | 0 | 0.85 | 1 | 1 | 0.3 | 0 | 0 | 0 |
| s33_00_0021 | 1 | 2.3 | 2.2 | 2 | 32 | 0.2 | 0 | 0.85 | 1 | 1 | 0.3 | 0 | 0 | 0 |
| s33_00_0022 | 1 | 2.3 | 2.2 | 2 | 32 | 0.2 | 0 | 0.85 | 1 | 1 | 0.3 | 0 | 0 | 0 |
| s33_00_0030 | 1 | 2.3 | 2.2 | 2 | 32 | 0.2 | 0 | 0.85 | 1 | 1 | 0.3 | 0 | 0 | 0 |
| s33_00_0031 | 1 | 2.3 | 2.2 | 2 | 32 | 0.2 | 0 | 0.85 | 1 | 1 | 0.3 | 0 | 0 | 0 |
| s33_00_0032 | 1 | 2.3 | 2.2 | 2 | 32 | 0.2 | 0 | 0.85 | 1 | 1 | 0.3 | 0 | 0 | 0 |
| s33_00_0040 | 1 | 2.3 | 2.2 | 2 | 32 | 0.2 | 0 | 0.85 | 1 | 1 | 0.3 | 0 | 0 | 0 |
| s33_00_0041 | 1 | 2.3 | 2.2 | 2 | 32 | 0.2 | 0 | 0.85 | 1 | 1 | 0.3 | 0 | 0 | 0 |
| s33_00_0042 | 1 | 2.3 | 2.2 | 2 | 32 | 0.2 | 0 | 0.85 | 1 | 1 | 0.3 | 0 | 0 | 0 |
| s34_01_0000 | 1 | 2 | 2.2 | 2 | 32 | 0.13 | 0 | 0.8 | 1 | 1 | 0.3 | 0 | 0 | 0 |
| s34_01_0010 | 6 | 2..2.34 | 2.2 | 2 | 0.01..32 | 0.13..0.52 | 0..3 | 0.8..0.85 | 1 | 0.985..1 | 0.279..0.45 | 0..1 | 0 | 0 |
| s35_00_0000 | 1 | 2 | 2.2 | 2 | 32 | 0.5 | 0 | 1 | 1 | 1 | 0 | 0 | 0 | 0 |
| s35_00_0001 | 1 | 2 | 2.2 | 2 | 32 | 0.5 | 0 | 1 | 1 | 1 | 0 | 0 | 0 | 0 |
| s37_00_0020 | 1 | 2 | 2.2 | 2 | 32 | 0.5 | 0 | 1 | 1 | 1 | 0 | 0 | 0 | 0 |
| s37_00_0021 | 1 | 2 | 2.2 | 2 | 32 | 0.5 | 0 | 1 | 1 | 1 | 0 | 0 | 0 | 0 |
| s37_00_0030 | 1 | 2 | 2.2 | 2 | 32 | 0.3 | 0 | 0.85 | 1 | 1 | 0.5 | 0 | 0 | 1 |
| s38_00_0000 | 1 | 2 | 2.2 | 2 | 32 | 0.5 | 0 | 1 | 1 | 1 | 0 | 0 | 0 | 0 |
| s38_00_0010 | 1 | 2 | 2.2 | 2 | 32 | 0.5 | 0 | 1 | 1 | 1 | 0 | 0 | 0 | 0 |
| s38_00_0020 | 1 | 2 | 2.2 | 2 | 32 | 0.5 | 0 | 1 | 1 | 1 | 0 | 0 | 0 | 0 |
| s39_00_0010 | 1 | 2 | 2.2 | 2 | 32 | 0.5 | 0 | 1 | 1 | 1 | 0 | 0 | 0 | 0 |
| s39_00_0020 | 1 | 2 | 2.2 | 2 | 32 | 0.5 | 0 | 1 | 1 | 1 | 0 | 0 | 0 | 0 |
| s39_00_0030 | 1 | 2.3 | 2.2 | 2 | 32 | 0 | 0 | 0.84 | 1 | 1 | 0.3 | 0 | 0 | 0 |
| s40_00_0020 | 7 | 0..2.8 | 0..2.2 | 2 | 32 | 0.15..0.65 | 0 | 0.6..0.8 | 0..1 | 1 | 0.3 | 0 | 0 | 0 |
| s40_00_0021 | 7 | 0..2.8 | 0..2.2 | 2 | 32 | 0.15..0.65 | 0 | 0.6..0.8 | 0..1 | 1 | 0.3 | 0 | 0 | 0 |
| s40_00_0040 | 4 | 0..2.2 | 0..2.2 | 2,3 | 0.36..32 | 0..1 | 0 | 0..0.8528 | 0..1 | 1..1.015 | 0..0.3 | 0 | 0 | 0 |
| s41_00_0000 | 2 | 0.5..0.7 | 2 | 2 | 32 | 0.3..0.5 | 0 | 0.8 | 1.2 | 1 | 0.2 | 0 | 0 | 1 |
| s41_00_0050 | 11 | 1.25..2.2 | 1.8..2.2 | 2 | 0.01..30 | 0.1..1 | 0..1 | 0.5..0.8 | 0..1 | 0.98..1 | 0.3..0.5 | 0..0.99 | 0 | 0 |
| s41_00_0051 | 11 | 1.25..2.2 | 1.8..2.2 | 2 | 0.01..30 | 0.1..1 | 0..1 | 0.5..0.8 | 0..1 | 0.98..1 | 0.3..0.5 | 0..0.99 | 0 | 0 |
| s41_00_0060 | 5 | 0..2 | 2.2 | 2 | 30..32 | 0.3..0.4 | 1 | 0.8 | 1 | 1 | 0..0.3 | 0..0.4 | 0 | 0 |
| s41_00_0070 | 4 | 1.2..2 | 2.2 | 2 | 30 | 0.1..0.52 | 0..1 | 0.8 | 0..1 | 0.99..1 | 0.3..1.35 | 0..0.99 | 0 | 0 |
| s45_00_0000 | 14 | 1.8..5.09 | 2.2..4.62 | 2 | 32 | 0.5..1 | 0 | 0.95..1.094 | 0.95..4 | 0.995..1.163 | 0..0.6 | 0..0.7 | 0 | 0 |
| s45_00_0010 | 14 | 1.8..5.09 | 2.2..4.62 | 2 | 32 | 0.5..1 | 0 | 0.95..1.094 | 0.95..4 | 0.995..1.163 | 0..0.6 | 0..0.7 | 0 | 0 |
| s45_00_0030 | 5 | 2..3.6 | 2.2 | 2 | 32 | 0.46..0.55 | 0 | 0.93..1 | 0..1 | 1 | 0..0.65 | 0..0.85 | 0 | 0 |
| s45_00_0031 | 5 | 2..3.6 | 2.2 | 2 | 32 | 0.46..0.55 | 0 | 0.93..1 | 0..1 | 1 | 0..0.65 | 0..0.85 | 0 | 0 |
| s51_00_0000 | 10 | 0.5..2.2 | 2.2..3.17 | 2 | 32 | 0.3..0.5 | 0 | 0.85..1 | 1 | 0.92..1 | 0.2..0.579 | 0..0.91 | 0 | 0 |

### B2. Tone Map second parameter run (second "Exposure", "Gamma", ...)

| file | Exposure#2 | Gamma#2 | Contrast | AvgLuminance | ToneMapMode | UseTable | Enable AutoExposure | AutoExp LightAdapt |
|---|---|---|---|---|---|---|---|---|
| s30_00_0000 | 0 | 2.2 | 0.5 | 1.8 | 0 | 1 | 0 | 1 |
| s30_00_0010 | 0 | 2.2 | 0.5 | 1.8 | 0 | 1 | 0 | - |
| s30_00_0020 | 0 | 2.2 | 0.5 | 1.8 | 0 | 1 | 0 | 1 |
| s30_00_0030 | 0 | 2.2 | 0.5 | 1.8 | 0 | 1 | 0 | 1 |
| s30_00_0040 | 0 | 2.2 | 0.5 | 1.8 | 0 | 1 | 0 | 1 |
| s30_00_9000 | 0 | 2.2 | 0.5 | 1.8 | 0 | 1 | 0 | - |
| s31_00_0000 | 0 | 2.2 | 0.5 | 1.8 | 0 | 1 | 0 | 1 |
| s31_00_0010 | 0 | 2.2 | 0.5 | 1.8 | 0 | 1 | 0 | 1 |
| s32_00_0000 | 0 | 2.2 | 0.5 | 1.8 | 0 | 1 | 0 | 1 |
| s32_00_0010 | 0 | 2.2 | 0.5 | 1.8 | 0 | 1 | 0 | 1 |
| s32_00_0020 | 0 | 2.2 | 0.5 | 1.8 | 0 | 1 | 0 | 1 |
| s32_00_0030 | 0 | 2.2 | 0.5 | 1.8 | 0 | 1 | 0 | 1 |
| s33_00_0000 | 0 | 2.2 | 0.5 | 1.8 | 0 | 1 | 0 | 1 |
| s33_00_0010 | 0 | 2.2 | 0.5 | 1.8 | 0 | 1 | 0 | 1 |
| s33_00_0020 | 0 | 2.2 | 0.5 | 1.8 | 0 | 1 | 0 | 1 |
| s33_00_0021 | 0 | 2.2 | 0.5 | 1.8 | 0 | 1 | 0 | 1 |
| s33_00_0022 | 0 | 2.2 | 0.5 | 1.8 | 0 | 1 | 0 | 1 |
| s33_00_0030 | 0 | 2.2 | 0.5 | 1.8 | 0 | 1 | 0 | 1 |
| s33_00_0031 | 0 | 2.2 | 0.5 | 1.8 | 0 | 1 | 0 | 1 |
| s33_00_0032 | 0 | 2.2 | 0.5 | 1.8 | 0 | 1 | 0 | 1 |
| s33_00_0040 | 0 | 2.2 | 0.5 | 1.8 | 0 | 1 | 0 | 1 |
| s33_00_0041 | 0 | 2.2 | 0.5 | 1.8 | 0 | 1 | 0 | 1 |
| s33_00_0042 | 0 | 2.2 | 0.5 | 1.8 | 0 | 1 | 0 | 1 |
| s34_01_0000 | 0 | 2.2 | 0.5 | 1.8 | 0 | 1 | 0 | 1 |
| s34_01_0010 | 0 | 2.2 | 0.5 | 1.8 | 0 | 1 | 0 | 1 |
| s35_00_0000 | 0 | 2.2 | 0.5 | 1.8 | 0 | 1 | 0 | 1 |
| s35_00_0001 | 0 | 2.2 | 0.5 | 1.8 | 0 | 1 | 0 | 1 |
| s37_00_0020 | 0 | 2.2 | 0.5 | 1.8 | 0 | 1 | 0 | 1 |
| s37_00_0021 | 0 | 2.2 | 0.5 | 1.8 | 0 | 1 | 0 | 1 |
| s37_00_0030 | 0 | 2.2 | 0.5 | 1.8 | 0 | 1 | 0 | 1 |
| s38_00_0000 | 0 | 2.2 | 0.5 | 1.8 | 0 | 1 | 0 | 1 |
| s38_00_0010 | 0 | 2.2 | 0.5 | 1.8 | 0 | 1 | 0 | 1 |
| s38_00_0020 | 0 | 2.2 | 0.5 | 1.8 | 0 | 1 | 0 | 1 |
| s39_00_0010 | 0 | 2.2 | 0.5 | 1.8 | 0 | 1 | 0 | 1 |
| s39_00_0020 | 0 | 2.2 | 0.5 | 1.8 | 0 | 1 | 0 | 1 |
| s39_00_0030 | 0 | 2.2 | 0.5 | 1.8 | 0 | 1 | 0 | 1 |
| s40_00_0020 | 0 | 2.2 | 0.5 | 1.8 | 0 | 1 | 0 | 1 |
| s40_00_0021 | 0 | 2.2 | 0.5 | 1.8 | 0 | 1 | 0 | 1 |
| s40_00_0040 | 0 | 2.2 | 0.5 | 1.8 | 0 | 1 | 0 | 1 |
| s41_00_0000 | 0 | 2.2 | 0.5 | 1.8 | 0 | 1 | 0 | 1 |
| s41_00_0050 | 0 | 2.2 | 0.5 | 1.8 | 0 | 1 | 0 | 0.5..1 |
| s41_00_0051 | 0 | 2.2 | 0.5 | 1.8 | 0 | 1 | 0 | 0.5..1 |
| s41_00_0060 | 0 | 2.2 | 0.5 | 1.8 | 0 | 1 | 0 | 1 |
| s41_00_0070 | 0 | 2.2 | 0.5 | 1.8 | 0 | 1 | 0 | 1 |
| s45_00_0000 | 0 | 2.2 | 0.5 | 1.8 | 0 | 1 | 0 | 1 |
| s45_00_0010 | 0 | 2.2 | 0.5 | 1.8 | 0 | 1 | 0 | 1 |
| s45_00_0030 | 0 | 2.2 | 0.5 | 1.8 | 0 | 1 | 0 | 1 |
| s45_00_0031 | 0 | 2.2 | 0.5 | 1.8 | 0 | 1 | 0 | 1 |
| s51_00_0000 | 0..1 | 2.2 | 0.5 | 1.8 | 0 | 1 | 0 | 1 |

### C. Bloom / glare

| file | ids | Enable | GlareShape | GlareNumLevels | GlareLuminance | GlareThreshold | BloomLuminance | GaussRadiusScale | non-zero ghost/star/afterimage/dispersion | AnamFlareLum | AnamGhostLumScale | GlareLuminance#2 |
|---|---|---|---|---|---|---|---|---|---|---|---|---|
| s30_00_0000 | 2 | 0 | 0,1 | 8 | 3..8 | 0.2..0.5 | 0 | 0.5..2 | - | 1 | 1 | 0 |
| s30_00_0010 | 1 | 0 | 1 | 8 | 10 | 1.5 | 0 | 0.5 | - | 1 | 1 | 0 |
| s30_00_0020 | 6 | 0 | 1 | 8 | 2.5 | 0.25 | 0 | 0.5 | - | 1 | 1 | 0 |
| s30_00_0030 | 1 | 0 | 0 | 8 | 3 | 0.5 | 0 | 0.5 | - | 1 | 1 | 0 |
| s30_00_0040 | 5 | 0 | 0,1 | 8 | 3..15 | 0.12..0.5 | 0 | 0.5..1.73 | - | 1 | 1 | 0 |
| s30_00_9000 | 1 | 0 | 1 | 8 | 3 | 0.5 | 0 | 2 | - | 1 | 1 | 0 |
| s31_00_0000 | 3 | 0 | 1 | 8 | 6.5..15 | 0.12..0.22 | 0 | 2 | - | 1 | 1 | 0 |
| s31_00_0010 | 2 | 0 | 1 | 8 | 6..8 | 0.2..0.3 | 0 | 0.5..2 | - | 1 | 1 | 0 |
| s32_00_0000 | 4 | 0 | 1 | 8 | 5.83 | 0.2 | 0 | 1 | - | 1 | 1 | 0 |
| s32_00_0010 | 8 | 0 | 0,1 | 8 | 3..9.17 | 0.2..0.5 | 0 | 0.5 | - | 1 | 1 | 0 |
| s32_00_0020 | 1 | 0 | 0 | 8 | 3 | 0.5 | 0 | 0.5 | - | 1 | 1 | 0 |
| s32_00_0030 | 1 | 0 | 1 | 8 | 3 | 0.3 | 0 | 1 | - | 1 | 1 | 0 |
| s33_00_0000 | 1 | 0 | 1 | 8 | 8 | 0.23 | 0 | 1 | - | 1 | 1 | 0 |
| s33_00_0010 | 1 | 0 | 0 | 8 | 3 | 0.5 | 0 | 0.5 | - | 1 | 1 | 0 |
| s33_00_0020 | 1 | 0 | 1 | 8 | 8 | 0.23 | 0 | 1 | - | 1 | 1 | 0 |
| s33_00_0021 | 1 | 0 | 1 | 8 | 8 | 0.23 | 0 | 1 | - | 1 | 1 | 0 |
| s33_00_0022 | 1 | 0 | 1 | 8 | 8 | 0.23 | 0 | 1 | - | 1 | 1 | 0 |
| s33_00_0030 | 1 | 0 | 1 | 8 | 8 | 0.23 | 0 | 1 | - | 1 | 1 | 0 |
| s33_00_0031 | 1 | 0 | 1 | 8 | 8 | 0.23 | 0 | 1 | - | 1 | 1 | 0 |
| s33_00_0032 | 1 | 0 | 1 | 8 | 8 | 0.23 | 0 | 1 | - | 1 | 1 | 0 |
| s33_00_0040 | 1 | 0 | 1 | 8 | 8 | 0.23 | 0 | 1.46 | - | 1 | 1 | 0 |
| s33_00_0041 | 1 | 0 | 1 | 8 | 8 | 0.23 | 0 | 1.46 | - | 1 | 1 | 0 |
| s33_00_0042 | 1 | 0 | 1 | 8 | 8 | 0.23 | 0 | 1.46 | - | 1 | 1 | 0 |
| s34_01_0000 | 7 | 0 | 1 | 8 | 4..5.83 | 0.09..0.3 | 0 | 0.5..2 | - | 1 | 1 | 0 |
| s34_01_0010 | 7 | 0..1 | 1 | 8 | 4..86.68 | 0.13..0.3 | 0 | 2..5.43 | - | 0.8..1 | 1 | 0 |
| s35_00_0000 | 3 | 0 | 1 | 8 | 3..7 | 0.2..0.5 | 0 | 0.5 | - | 1 | 1 | 0 |
| s35_00_0001 | 3 | 0 | 1 | 8 | 3..7 | 0.2..0.5 | 0 | 0.5 | - | 1 | 1 | 0 |
| s37_00_0020 | 5 | 0 | 1 | 8 | 10.83..15 | 0.17..0.43 | 0 | 0.84..1.25 | - | 1 | 1 | 0 |
| s37_00_0021 | 5 | 0 | 1 | 8 | 10.83..15 | 0.17..0.43 | 0 | 0.84..1.25 | - | 1 | 1 | 0 |
| s37_00_0030 | 1 | 0 | 1 | 6 | 3 | 0.5 | 0 | 0.5 | - | 1 | 1 | 0 |
| s38_00_0000 | 1 | 0 | 0 | 8 | 3 | 0.5 | 0 | 0.5 | - | 1 | 1 | 0 |
| s38_00_0010 | 1 | 0 | 0 | 8 | 3 | 0.5 | 0 | 0.5 | - | 1 | 1 | 0 |
| s38_00_0020 | 1 | 0 | 0 | 8 | 3 | 0.5 | 0 | 0.5 | - | 1 | 1 | 0 |
| s39_00_0010 | 1 | 0 | 0 | 8 | 3 | 0.5 | 0 | 0.5 | - | 1 | 1 | 0 |
| s39_00_0020 | 1 | 0 | 0 | 8 | 3 | 0.5 | 0 | 0.5 | - | 1 | 1 | 0 |
| s39_00_0030 | 1 | 0 | 1 | 8 | 10 | 0.28 | 0 | 2 | - | 1 | 1 | 0 |
| s40_00_0020 | 2 | 0 | 1 | 6 | 3..4 | 0.2..0.3 | 0 | 2 | - | 1 | 1 | 0 |
| s40_00_0021 | 2 | 0 | 1 | 6 | 3..4 | 0.2..0.3 | 0 | 2 | - | 1 | 1 | 0 |
| s40_00_0040 | 1 | 0 | 1 | 6 | 1 | 0 | 0 | 2 | - | 1 | 1 | 0 |
| s41_00_0000 | 1 | 1 | 1 | 6 | 4 | 0 | 35.05 | 4.2 | StarLuminance=1.97;BloomDispersion=1.88 | 1.54 | 10 | 40.17 |
| s41_00_0050 | 4 | 0..1 | 1 | 5..6 | 1.5..7 | 0.25 | 0 | 0.57..2 | - | 0.04..1 | 1 | 0 |
| s41_00_0051 | 4 | 0..1 | 1 | 5..6 | 1.5..7 | 0.25 | 0 | 0.57..2 | - | 0.04..1 | 1 | 0 |
| s41_00_0060 | 3 | 0 | 1 | 8 | 1.67 | 0.25 | 0 | 0.5 | - | 1 | 1 | 0 |
| s41_00_0070 | 3 | 0..1 | 1 | 5..6 | 5..7 | 0.25 | 0 | 1.18..2 | - | 0.04..1 | 1 | 0 |
| s45_00_0000 | 10 | 0..1 | 0,1 | 8 | 3..16.67 | 0.17..0.5 | 0 | 0.5 | - | 0.5..1 | 0.5..1 | 0 |
| s45_00_0010 | 10 | 0..1 | 0,1 | 8 | 3..16.67 | 0.17..0.5 | 0 | 0.5 | - | 0.5..1 | 0.5..1 | 0 |
| s45_00_0030 | 1 | 0 | 0 | 8 | 3 | 0.5 | 0 | 0.5 | - | 1 | 1 | 0 |
| s45_00_0031 | 1 | 0 | 0 | 8 | 3 | 0.5 | 0 | 0.5 | - | 1 | 1 | 0 |
| s51_00_0000 | 4 | 0..1 | 1 | 6 | 3..6 | 0.25 | 0 | 2 | - | 1 | 1 | 0 |

### D. Depth of field (Enable3 = first Enable, type 3; Enable7 = Enable, type 7; counts are ids on / ids)

| file | ids | Enable3 on | Enable7 on | ApertuerF | DofFocusDistance | CCD-Size | EdgeQuality | NearDepthEnd | NearCocSize | FarDepthStart | FarDepthEnd | FarCocSize | NearCustom on | FarCustom on | Focus*/other non-default |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| s30_00_0000 | 10 | 10/10 | 1/10 | 2..6 | 1..8 | 43.27 | -1 | 2..3 | 0.2..1 | 50..100 | 150..1e+04 | 0.4..1 | 0/10 | 1/10 | defaults |
| s30_00_0010 | 8 | 8/8 | 1/8 | 1..6.2 | 1..20 | 43.27 | -1 | 2 | 0..1 | 15..100 | 150 | 1 | 1/8 | 1/8 | defaults |
| s30_00_0020 | 6 | 6/6 | 0/6 | 1..5.2 | 0.61..11 | 43.27 | -1 | 2 | 1 | 100 | 150 | 1 | 0/6 | 0/6 | defaults |
| s30_00_0030 | 4 | 3/4 | 0/4 | 2..5.6 | 1..50 | 43.27 | -1 | 2 | 1 | 100 | 150 | 1 | 0/4 | 0/4 | defaults |
| s30_00_0040 | 5 | 4/5 | 0/5 | 1..5.6 | 1..10 | 43.27 | -1 | 2 | 1 | 100 | 150 | 1 | 0/5 | 0/5 | defaults |
| s30_00_9000 | 2 | 2/2 | 0/2 | 2.2 | 1..1.5 | 43.27 | 4 | 2 | 1 | 100 | 150 | 1 | 0/2 | 0/2 | defaults |
| s31_00_0000 | 8 | 8/8 | 0/8 | 0.8..4 | 0.27..25 | 84.75 | -1 | 2 | 1 | 100 | 150 | 1 | 0/8 | 0/8 | defaults |
| s31_00_0010 | 5 | 5/5 | 1/5 | 5.6 | 1..8 | 43.27 | -1..2 | 0..2 | 1..20.51 | 100..1966 | 150..2.308e+04 | 1..22.22 | 0/5 | 0/5 | defaults |
| s32_00_0000 | 4 | 4/4 | 0/4 | 0.5..5.6 | 1..3.67 | 43.27 | -1 | 2 | 1 | 100 | 150 | 1 | 0/4 | 0/4 | defaults |
| s32_00_0010 | 9 | 9/9 | 0/9 | 2 | 1..20 | 43.27 | -1 | 2 | 1 | 100 | 150 | 1 | 0/9 | 0/9 | defaults |
| s32_00_0020 | 6 | 5/6 | 0/6 | 2..5.6 | 1..2 | 43.27 | -1 | 2 | 1 | 100 | 150 | 1 | 0/6 | 0/6 | defaults |
| s32_00_0030 | 1 | 1/1 | 1/1 | 5.6 | 1 | 43.27 | -1 | 2 | 1 | 10 | 30 | 0.29 | 1/1 | 1/1 | defaults |
| s33_00_0000 | 5 | 4/5 | 0/5 | 0.45..5.6 | 1..33.34 | 43.27 | -1 | 2 | 1 | 100 | 150 | 1 | 0/5 | 0/5 | defaults |
| s33_00_0010 | 4 | 4/4 | 0/4 | 5.6 | 1 | 43.27 | -1 | 2 | 1 | 100 | 150 | 1 | 0/4 | 0/4 | defaults |
| s33_00_0020 | 4 | 4/4 | 0/4 | 0.09..4 | 2.9..200 | 43.27 | -1 | 2 | 1 | 100 | 150 | 1 | 0/4 | 0/4 | defaults |
| s33_00_0021 | 4 | 4/4 | 0/4 | 0.08..3 | 2.9..200 | 43.27 | -1 | 2 | 1 | 100 | 150 | 1 | 0/4 | 0/4 | defaults |
| s33_00_0022 | 4 | 4/4 | 0/4 | 0.08..1.77 | 3.5..200 | 43.27 | -1 | 2 | 1 | 100 | 150 | 1 | 0/4 | 0/4 | defaults |
| s33_00_0030 | 2 | 2/2 | 0/2 | 1.5..4 | 2.9..23 | 43.27 | -1 | 2 | 1 | 100 | 150 | 1 | 0/2 | 0/2 | defaults |
| s33_00_0031 | 2 | 2/2 | 0/2 | 1.5..4 | 2.9..23 | 43.27 | -1 | 2 | 1 | 100 | 150 | 1 | 0/2 | 0/2 | defaults |
| s33_00_0032 | 2 | 2/2 | 0/2 | 1.5..4 | 2.9..23 | 43.27 | -1 | 2 | 1 | 100 | 150 | 1 | 0/2 | 0/2 | defaults |
| s33_00_0040 | 4 | 4/4 | 0/4 | 0.45..3 | 4.2..23 | 43.27 | -1 | 2 | 1 | 100 | 150 | 1 | 0/4 | 0/4 | defaults |
| s33_00_0041 | 4 | 4/4 | 0/4 | 0.45..3 | 4.2..23 | 43.27 | -1 | 2 | 1 | 100 | 150 | 1 | 0/4 | 0/4 | defaults |
| s33_00_0042 | 4 | 4/4 | 0/4 | 0.45..3 | 4.2..23 | 43.27 | -1 | 2 | 1 | 100 | 150 | 1 | 0/4 | 0/4 | defaults |
| s34_01_0000 | 9 | 7/9 | 0/9 | 1..5.6 | 0.59..20 | 43.27 | -1 | 2 | 1 | 100 | 150 | 1 | 0/9 | 0/9 | defaults |
| s34_01_0010 | 8 | 5/8 | 0/8 | 0.15..5.6 | 1 | 43.27..84.75 | -1 | 2 | 1 | 100 | 150 | 1 | 0/8 | 0/8 | defaults |
| s35_00_0000 | 3 | 2/3 | 0/3 | 2.4..5.6 | 1..2 | 43.27 | -1 | 2 | 1 | 100 | 150 | 1 | 0/3 | 0/3 | defaults |
| s35_00_0001 | 3 | 2/3 | 0/3 | 2.4..5.6 | 1..2 | 43.27 | -1 | 2 | 1 | 100 | 150 | 1 | 0/3 | 0/3 | defaults |
| s37_00_0020 | 5 | 5/5 | 0/5 | 1..6 | 0.9..7.9 | 43.27 | -1 | 2 | 1 | 100 | 150 | 1 | 0/5 | 0/5 | defaults |
| s37_00_0021 | 5 | 5/5 | 0/5 | 1..6 | 0.9..7.9 | 43.27 | -1 | 2 | 1 | 100 | 150 | 1 | 0/5 | 0/5 | defaults |
| s37_00_0030 | 1 | 1/1 | 1/1 | 5.6 | 1 | 43.27 | -1 | 2 | 0 | 10 | 20 | 0.07 | 1/1 | 1/1 | defaults |
| s38_00_0000 | 3 | 3/3 | 0/3 | 3.2..4 | 0.64..3.5 | 43.27 | -1 | 2 | 1 | 100 | 150 | 1 | 0/3 | 0/3 | defaults |
| s38_00_0010 | 3 | 3/3 | 0/3 | 3.2..4 | 0.64..3.5 | 43.27 | -1 | 2 | 1 | 100 | 150 | 1 | 0/3 | 0/3 | defaults |
| s38_00_0020 | 4 | 4/4 | 0/4 | 3..5.6 | 0.82..4 | 43.27 | -1 | 2 | 1 | 100 | 150 | 1 | 0/4 | 0/4 | defaults |
| s39_00_0010 | 2 | 2/2 | 0/2 | 0.1..3 | 1 | 43.27 | -1 | 2 | 1 | 100 | 150 | 1 | 0/2 | 0/2 | defaults |
| s39_00_0020 | 3 | 2/3 | 0/3 | 1.8..5.6 | 1..7 | 43.27 | -1 | 2 | 1 | 100 | 150 | 1 | 0/3 | 0/3 | defaults |
| s39_00_0030 | 1 | 1/1 | 1/1 | 2 | 1 | 43.27 | -1 | 0 | 0 | 10 | 20 | 0.53 | 1/1 | 1/1 | defaults |
| s40_00_0020 | 7 | 5/7 | 0/7 | 0.45..10.68 | 1..3 | 28.77..43.27 | -1 | 2 | 1 | 100 | 150 | 1 | 0/7 | 0/7 | defaults |
| s40_00_0021 | 7 | 5/7 | 0/7 | 0.45..10.68 | 1..3 | 28.77..43.27 | -1 | 2 | 1 | 100 | 150 | 1 | 0/7 | 0/7 | defaults |
| s40_00_0040 | 6 | 6/6 | 3/6 | 0.01..80 | 0.1..33.34 | 43.27 | -1 | 0..5 | 0.02..1 | 6.2..100 | 100..150 | 1..10 | 3/6 | 3/6 | defaults |
| s41_00_0000 | 1 | 1/1 | 0/1 | 3 | 1 | 28.77 | -1 | 2 | 1 | 100 | 150 | 1 | 0/1 | 0/1 | defaults |
| s41_00_0050 | 4 | 4/4 | 0/4 | 4..8.5 | 1 | 43.27..84.75 | -1 | 2 | 1 | 100 | 150 | 1 | 0/4 | 0/4 | defaults |
| s41_00_0051 | 4 | 4/4 | 0/4 | 4..8.5 | 1 | 43.27..84.75 | -1 | 2 | 1 | 100 | 150 | 1 | 0/4 | 0/4 | defaults |
| s41_00_0060 | 3 | 3/3 | 0/3 | 4..14 | 1.2..8 | 43.27 | -1 | 2 | 1 | 100 | 150 | 1 | 0/3 | 0/3 | defaults |
| s41_00_0070 | 3 | 3/3 | 0/3 | 2.5..4.5 | 1 | 84.75 | -1 | 2 | 1 | 100 | 150 | 1 | 0/3 | 0/3 | defaults |
| s45_00_0000 | 12 | 10/12 | 0/12 | 0.05..5.6 | 1 | 28.77..84.75 | -1 | 2 | 1 | 100 | 150 | 1 | 0/12 | 0/12 | defaults |
| s45_00_0010 | 12 | 10/12 | 0/12 | 0.05..5.6 | 1 | 28.77..84.75 | -1 | 2 | 1 | 100 | 150 | 1 | 0/12 | 0/12 | defaults |
| s45_00_0030 | 5 | 3/5 | 0/5 | 0.8..5.6 | 0..1 | 28.77..43.27 | -1 | 2 | 1 | 100 | 150 | 1 | 0/5 | 0/5 | defaults |
| s45_00_0031 | 5 | 3/5 | 0/5 | 0.8..5.6 | 0..1 | 28.77..43.27 | -1 | 2 | 1 | 100 | 150 | 1 | 0/5 | 0/5 | defaults |
| s51_00_0000 | 6 | 6/6 | 0/6 | 0.5..5.6 | 1 | 43.27 | -1 | 2 | 1 | 100 | 150 | 1 | 0/6 | 0/6 | defaults |

### E. Motion blur, feedback blur, light shaft, colour grading, anti-alias

| file | MB Enable | MB BlurTimeRatio | MB Scale | MB samples | MB FalloffMax | FB Enable | FB Weight | LS Enable | LS ColorScale | LS ShaftLength | LS DiffRing | CG Enable | CG LutSourceId | AA Enable |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| s30_00_0000 | - | - | - | - | - | - | - | 0 | 1 | 1 | 0 | 1 | 10 | - |
| s30_00_0010 | 1 | 0.05 | 1 | 5 | 3 | 0 | 0 | 0..1 | 0.3831..1.442 | 0..1.46 | 0 | 1 | 1 | 1 |
| s30_00_0020 | - | - | - | - | - | - | - | 0 | 1 | 1 | 0 | 0 | -1 | - |
| s30_00_0030 | - | - | - | - | - | - | - | 0 | 1 | 1 | 0 | 0 | -1 | - |
| s30_00_0040 | - | - | - | - | - | - | - | 1 | 0.6845..1.668 | 1..1.23 | 0 | 0 | -1 | - |
| s30_00_9000 | - | - | - | - | - | 0 | 0 | 1 | 0.8113..2 | 0.76 | 0 | 1 | 2 | - |
| s31_00_0000 | - | - | - | - | - | - | - | - | - | - | - | - | - | - |
| s31_00_0010 | - | - | - | - | - | 0 | 0 | 0 | 1 | 1.5 | 0 | 1 | 1..10 | - |
| s32_00_0000 | - | - | - | - | - | 0 | 0 | 1 | 0.25..1 | 2.5 | 0 | 1 | 2 | - |
| s32_00_0010 | - | - | - | - | - | - | - | 0 | 1 | 1 | 0 | 0 | -1 | - |
| s32_00_0020 | - | - | - | - | - | - | - | 0 | 1 | 1 | 0 | 0 | -1 | - |
| s32_00_0030 | - | - | - | - | - | 0 | 0 | 1 | 0.25..1 | 2.5 | 0 | 1 | 1 | - |
| s33_00_0000 | - | - | - | - | - | 0 | 0 | 0 | 0.6454..2.172 | 4.31 | 0 | 1 | 1 | - |
| s33_00_0010 | - | - | - | - | - | - | - | 0 | 1 | 1 | 0 | 0 | -1 | - |
| s33_00_0020 | - | - | - | - | - | 0 | 0 | 0 | 0.6454..2.172 | 4.31 | 0 | 1 | 1 | - |
| s33_00_0021 | - | - | - | - | - | 0 | 0 | 0 | 0.6454..2.172 | 4.31 | 0 | 1 | 1 | - |
| s33_00_0022 | - | - | - | - | - | 0 | 0 | 0 | 0.6454..2.172 | 4.31 | 0 | 1 | 1 | - |
| s33_00_0030 | - | - | - | - | - | 0 | 0 | 0 | 0.6454..2.172 | 4.31 | 0 | 1 | 1 | - |
| s33_00_0031 | - | - | - | - | - | 0 | 0 | 0 | 0.6454..2.172 | 4.31 | 0 | 1 | 1 | - |
| s33_00_0032 | - | - | - | - | - | 0 | 0 | 0 | 0.6454..2.172 | 4.31 | 0 | 1 | 1 | - |
| s33_00_0040 | - | - | - | - | - | 0 | 0 | 0 | 0.6454..2.172 | 4.31 | 0 | 1 | 1 | - |
| s33_00_0041 | - | - | - | - | - | 0 | 0 | 0 | 0.6454..2.172 | 4.31 | 0 | 1 | 1 | - |
| s33_00_0042 | - | - | - | - | - | 0 | 0 | 0 | 0.6454..2.172 | 4.31 | 0 | 1 | 1 | - |
| s34_01_0000 | - | - | - | - | - | 0 | 0 | 1 | 0.8113..2 | 3 | 0 | 1 | 1 | - |
| s34_01_0010 | - | - | - | - | - | 0 | 0 | 1 | 0.8113..2 | 3 | 0 | 1 | 1 | - |
| s35_00_0000 | - | - | - | - | - | - | - | 0 | 1 | 1 | 0 | 0 | -1 | - |
| s35_00_0001 | - | - | - | - | - | - | - | 0 | 1 | 1 | 0 | 0 | -1 | - |
| s37_00_0020 | - | - | - | - | - | - | - | 1 | 0.3972..7.527 | 2.92..7 | 0 | 0 | -1 | - |
| s37_00_0021 | - | - | - | - | - | - | - | 1 | 0.3972..8 | 2.92..7 | 0 | 0 | -1 | - |
| s37_00_0030 | - | - | - | - | - | 0 | 0 | 1 | 0.502..10.01 | 4 | 0 | 0 | -1 | - |
| s38_00_0000 | - | - | - | - | - | - | - | 0 | 1 | 1 | 0 | 0 | -1 | - |
| s38_00_0010 | - | - | - | - | - | - | - | 0 | 1 | 1 | 0 | 0 | -1 | - |
| s38_00_0020 | - | - | - | - | - | - | - | 0 | 1 | 1 | 0 | 0 | -1 | - |
| s39_00_0010 | - | - | - | - | - | - | - | 0 | 1 | 1 | 0 | 0 | -1 | - |
| s39_00_0020 | - | - | - | - | - | - | - | 0 | 1 | 1 | 0 | 0 | -1 | - |
| s39_00_0030 | - | - | - | - | - | 0 | 0 | 1 | 0.6113..2.524 | 2.75 | 0 | 1 | 1 | - |
| s40_00_0020 | - | - | - | - | - | 0 | 0 | 1 | 0.7014..4.935 | 1.64 | 0 | 1 | 3 | - |
| s40_00_0021 | - | - | - | - | - | 0 | 0 | 1 | 0.7014..4.935 | 1.64 | 0 | 1 | 3 | - |
| s40_00_0040 | - | - | - | - | - | 0 | 0 | 0 | 0.7451..80 | 0.3 | 0 | 1 | 3 | - |
| s41_00_0000 | - | - | - | - | - | - | - | 0 | 1 | 1 | 0 | 0 | -1 | - |
| s41_00_0050 | - | - | - | - | - | 0 | 0 | 1 | 0.5831..7.008 | 1 | 0 | 1 | 1 | - |
| s41_00_0051 | - | - | - | - | - | 0 | 0 | 1 | 0.5831..7.008 | 1 | 0 | 1 | 1 | - |
| s41_00_0060 | - | - | - | - | - | - | - | 0 | 1 | 1 | 0 | 0 | -1 | - |
| s41_00_0070 | - | - | - | - | - | 0 | 0 | 1 | 0.5831..7.008 | 1 | 0 | 1 | 1 | - |
| s45_00_0000 | 0..1 | 0.05..0.2 | 1 | 4 | 0..3 | - | - | 0 | 1 | 1 | 0 | 0 | -1 | - |
| s45_00_0010 | 0..1 | 0.05..0.2 | 1 | 4 | 0..3 | - | - | 0 | 1 | 1 | 0 | 0 | -1 | - |
| s45_00_0030 | - | - | - | - | - | - | - | 0 | 1 | 1 | 0 | 0 | -1 | - |
| s45_00_0031 | - | - | - | - | - | - | - | 0 | 1 | 1 | 0 | 0 | -1 | - |
| s51_00_0000 | - | - | - | - | - | 0 | 0 | 1 | 0.2704..2.411 | 5 | 0 | 1 | 0..1 | - |

### F. Setting Info (32 files carry it)

| file | ids | BlurShape | GlareQuality | DepthOfFieldQuality | Enable ColorGrading | Create MB velocity tex | Auto Compute Enable | BG Color |
|---|---|---|---|---|---|---|---|---|
| s30_00_0000 | 1 | 0 | 1 | 1 | 0 | 1 | 0 | 0.5 |
| s30_00_0010 | 1 | 3 | 2 | 4 | 1 | 1 | 1 | 0.5 |
| s30_00_0020 | 1 | 0 | 1 | 1 | 0 | 1 | 0 | 0.5 |
| s30_00_0030 | 1 | 0 | 1 | 1 | 0 | 1 | 0 | 0.5 |
| s30_00_0040 | 1 | 0 | 1 | 1 | 0 | 1 | 0 | 0.5 |
| s30_00_9000 | 1 | 3 | 5 | 9 | 0 | 1 | 0 | - |
| s32_00_0010 | 1 | 0 | 1 | 1 | 0 | 1 | 0 | 0.5 |
| s32_00_0020 | 1 | 0 | 1 | 1 | 0 | 1 | 0 | 0.5 |
| s33_00_0010 | 1 | 0 | 1 | 1 | 0 | 1 | 0 | 0.5 |
| s34_01_0010 | 1 | 0 | 2 | 3 | 1 | 1 | 0 | 0.5 |
| s35_00_0000 | 1 | 0 | 1 | 1 | 0 | 1 | 0 | 0.5 |
| s35_00_0001 | 1 | 0 | 1 | 1 | 0 | 1 | 0 | 0.5 |
| s37_00_0020 | 1 | 0 | 4 | 6 | 0 | 1 | 0 | 0.5 |
| s37_00_0021 | 1 | 0 | 4 | 6 | 0 | 1 | 0 | 0.5 |
| s38_00_0000 | 1 | 0 | 1 | 1 | 0 | 1 | 0 | 0.5 |
| s38_00_0010 | 1 | 0 | 1 | 1 | 0 | 1 | 0 | 0.5 |
| s38_00_0020 | 1 | 0 | 1 | 1 | 0 | 1 | 0 | 0.5 |
| s39_00_0010 | 1 | 0 | 1 | 1 | 0 | 1 | 0 | 0.5 |
| s39_00_0020 | 1 | 0 | 1 | 1 | 0 | 1 | 0 | 0.5 |
| s39_00_0030 | 1 | 0 | 4 | 4 | 1 | 1 | 1 | 0..1 |
| s40_00_0020 | 2 | 0 | 4 | 4..5 | 1 | 1 | 0 | 0.5 |
| s40_00_0021 | 2 | 0 | 4 | 4..5 | 1 | 1 | 0 | 0.5 |
| s40_00_0040 | 1 | 0 | 3 | 7 | 0 | 1 | 0 | 0.5 |
| s41_00_0000 | 1 | 0 | 4 | 6 | 0 | 1 | 0 | 0.5 |
| s41_00_0050 | 1 | 0 | 4 | 5 | 1 | 1 | 0 | 0.5 |
| s41_00_0051 | 1 | 0 | 4 | 5 | 1 | 1 | 0 | 0.5 |
| s41_00_0060 | 1 | 0 | 4 | 5 | 1 | 1 | 0 | 0.5 |
| s41_00_0070 | 1 | 0 | 4 | 5 | 1 | 1 | 0 | 0.5 |
| s45_00_0000 | 3 | 0 | 1..3 | 1..5 | 0 | 0..1 | 0 | 0.5 |
| s45_00_0010 | 1 | 0 | 1 | 1 | 0 | 1 | 0 | 0.5 |
| s45_00_0030 | 4 | 0 | 1 | 1 | 0 | 1 | 0 | 0.5 |
| s45_00_0031 | 4 | 0 | 1 | 1 | 0 | 1 | 0 | 0.5 |


Other findings read from the tables (OBSERVED unless marked):

- **Fade-to-black candidates** (ColorV 0): `s40_00_0020` / `s40_00_0021` / `s40_00_0040` id 10 (all three of Exposure, Gamma, ColorV are 0 there), `s41_00_0050` / `s41_00_0051` / `s41_00_0070` id 101, `s45_00_0030` / `s45_00_0031` id 103. Exposure 0 alone: `s41_00_0060` id 21. That these ids are fades is INFERRED from the names (`outliers.txt`).
- **High exposure / gamma**: Exposure 4.34, 3.2, 5.09 at `s45_00_0000/0010` ids 111, 112, 113 (id 113 also ColorV 4 and Gamma 4.62); 3.6 at `s45_00_0030/0031` ids 101, 102; 2.8 at `s40_00_0020/21` id 100; 2.75 at `s30_00_0000` id 0. Gamma 3.17 / 2.48 / 2.3 at `s51_00_0000` ids 1041 / 2051 / 2052; 1.8 at `s41_00_0050/51` id 135; 2.0 at `s41_00_0000` ids 0, 1.
- **Hue rotation**: ColorH 3 at `s34_01_0010` ids 100, 101, 104 (a value up to 3 does not occur in map files: 0 to 1); ColorH 1 at `s41_00_0050/0051/0060/0070`.
- **Tone type 3**: only `s40_00_0040` ids 0 and 1 (also ToneMappingFactor 0.36 at id 1).
- **Vignette**: Power 0 to 1.35, fov dependence 0 to 1 (1.0 at `s34_01_0010` and values 0.99 at `s41_00_0050/51/70`).
- **Colour grading**: LUT ids used (`LutSourceId`, Enable 1): 0, 1, 2, 3, 10; 22 of 48 files have LutSourceId -1 and Enable 0. `s30_00_0000` id 10 and `s31_00_0010` uses LUT ids 1 to 10 (10 is the highest id in the data).
- **Setting Info**: the 32 files that carry it set `BlurShape` 0 except `s30_00_0010` and `s30_00_9000` (3), `GlareQuality` 1 to 5 (5 at `s30_00_9000`), `DepthOfFieldQuality` 1 to 9 (9 at `s30_00_9000`), `Auto Compute Enable` 1 at `s30_00_0010` and `s39_00_0030`; the Setting Info switches for chromatic aberration, anamorphic lens aperture, light shaft, feedback effect, anti-alias and lens distortion are 1 in all of them (identical to the map files).
- **Lens Simulation** (19 files) and **Glow Filter** (3 files) are carried with the same values as in the map files that carry them (cutscene range = map range in `cutscene-vs-map-ranges.tsv`).

## 3. Feature verdict for cutscenes

Counts are records (file x id) / number of files where the condition holds (`feature-counts.tsv`, script `verdict.py`). "world map files" = all `m*` GPARAM files. A feature is called cutscene-only here when no world-map record and no default/mapbase/remobase record meets the condition (OBSERVED within these files; the other worker's census of the maps may refine the map columns).

| feature (condition on a value id) | cutscene records / files | world map files (m*) records / files | default+mapbase+remobase records / files | only cutscenes? |
|---|---|---|---|---|
| Tone ColorV != 1 | 39 / 12 | 2 / 2 | 0 / 0 | no (also elsewhere) |
| Tone ColorV == 0 (black) | 8 / 8 | 0 / 0 | 0 / 0 | YES (cutscenes only) |
| Tone ColorV > 1 | 6 / 3 | 2 / 2 | 0 / 0 | no (also elsewhere) |
| Tone ColorH != 0 | 19 / 5 | 3 / 3 | 0 / 0 | no (also elsewhere) |
| Tone ColorS != 1 | 108 / 34 | 42 / 42 | 0 / 0 | no (also elsewhere) |
| Tone Exposure == 0 | 4 / 4 | 2 / 2 | 0 / 0 | no (also elsewhere) |
| Tone Gamma == 0 | 3 / 3 | 0 / 0 | 0 / 0 | YES (cutscenes only) |
| Tone Gamma != 2.2 and != 0 | 9 / 6 | 0 / 0 | 0 / 0 | YES (cutscenes only) |
| Tone Exposure > 2.5 | 13 / 7 | 0 / 0 | 0 / 0 | YES (cutscenes only) |
| Tone ToneType == 3 | 2 / 1 | 0 / 0 | 0 / 0 | YES (cutscenes only) |
| Tone ToneType == 2 | 144 / 49 | 51 / 51 | 7 / 1 | no (also elsewhere) |
| Tone Contrust != 1 | 28 / 10 | 0 / 0 | 0 / 0 | YES (cutscenes only) |
| Tone FilmicContrast != 0.5 | 96 / 34 | 44 / 44 | 1 / 1 | no (also elsewhere) |
| Tone ToneMappingFactor != 32 | 34 / 6 | 4 / 4 | 0 / 0 | no (also elsewhere) |
| Tone Vignette Power > 0 | 105 / 34 | 30 / 30 | 0 / 0 | no (also elsewhere) |
| Tone VignetteFovDependence != 0 | 58 / 13 | 0 / 0 | 0 / 0 | YES (cutscenes only) |
| Tone Vignette Power > 0.5 | 13 / 7 | 0 / 0 | 0 / 0 | YES (cutscenes only) |
| Tone AutoExposure (3) != 0 | 0 / 0 | 7 / 7 | 0 / 0 | no (maps/global only) |
| Tone Enable (3) == 1 | 4 / 3 | 2 / 2 | 0 / 0 | no (also elsewhere) |
| Tone ToneLateralDispersion[0] > 0 | 12 / 6 | 10 / 10 | 0 / 0 | no (also elsewhere) |
| Tone WaveSample != 3 | 3 / 2 | 1 / 1 | 0 / 0 | no (also elsewhere) |
| Bloom GlareShape == 0 | 26 / 15 | 0 / 0 | 2 / 1 | no (also elsewhere) |
| Bloom GlareShape == 1 | 103 / 39 | 38 / 38 | 2 / 2 | no (also elsewhere) |
| Bloom GlareShape >= 2 | 0 / 0 | 1 / 1 | 0 / 0 | no (maps/global only) |
| Bloom Enable (3) == 1 | 13 / 8 | 0 / 0 | 0 / 0 | YES (cutscenes only) |
| Bloom GlareNumLevels (3) != 8 | 22 / 9 | 10 / 10 | 0 / 0 | no (also elsewhere) |
| Bloom GlareLuminance > 20 | 3 / 1 | 0 / 0 | 1 / 1 | no (also elsewhere) |
| Bloom BloomLuminance > 0 | 1 / 1 | 0 / 0 | 0 / 0 | YES (cutscenes only) |
| Bloom StarLuminance > 0 | 1 / 1 | 0 / 0 | 0 / 0 | YES (cutscenes only) |
| Bloom GhostLuminance > 0 | 0 / 0 | 0 / 0 | 0 / 0 | used nowhere |
| Bloom AfterimageLuminance > 0 | 0 / 0 | 0 / 0 | 0 / 0 | used nowhere |
| Bloom AnamorphicFlareLuminance != 1 | 10 / 7 | 0 / 0 | 0 / 0 | YES (cutscenes only) |
| Bloom GlareThreshold == 0 | 2 / 2 | 0 / 0 | 0 / 0 | YES (cutscenes only) |
| Dof Enable (3) == 1 | 208 / 49 | 6 / 6 | 2 / 2 | no (also elsewhere) |
| Dof Enable (7) == 1 | 9 / 7 | 6 / 6 | 2 / 2 | no (also elsewhere) |
| Dof DofFocusDistance != 1 | 139 / 36 | 0 / 0 | 0 / 0 | YES (cutscenes only) |
| Dof DofFocusDistance > 20 | 16 / 13 | 0 / 0 | 0 / 0 | YES (cutscenes only) |
| Dof ApertuerF != 5.6 | 193 / 45 | 4 / 4 | 2 / 2 | no (also elsewhere) |
| Dof ApertuerF < 1 | 38 / 19 | 2 / 2 | 0 / 0 | no (also elsewhere) |
| Dof ApertuerF > 10 | 5 / 4 | 0 / 0 | 0 / 0 | YES (cutscenes only) |
| Dof CCD-Size != 43.267 | 35 / 12 | 0 / 0 | 0 / 0 | YES (cutscenes only) |
| Dof DofEdgeQuarity == -1 | 229 / 48 | 3 / 3 | 1 / 1 | no (also elsewhere) |
| Dof DofEdgeQuarity >= 0 | 3 / 2 | 3 / 3 | 2 / 2 | no (also elsewhere) |
| Dof Enable-Near-Custom == 1 | 7 / 5 | 3 / 3 | 0 / 0 | no (also elsewhere) |
| Dof Enable-Far-Custom == 1 | 8 / 6 | 6 / 6 | 2 / 2 | no (also elsewhere) |
| Dof Enable#2 (3) == 1 | 0 / 0 | 0 / 0 | 0 / 0 | used nowhere |
| Dof DofNear CocSize != 1 | 6 / 6 | 2 / 2 | 2 / 2 | no (also elsewhere) |
| Dof DofFar CocSize != 1 | 6 / 6 | 5 / 5 | 2 / 2 | no (also elsewhere) |
| Dof ApertureFront/BackNumLevels != 3 | 41 / 16 | 0 / 0 | 0 / 0 | YES (cutscenes only) |
| Dof DofRotateScale != 1 | 11 / 3 | 0 / 0 | 0 / 0 | YES (cutscenes only) |
| Dof FocusScreenPosition != 0.5 | 0 / 0 | 1 / 1 | 0 / 0 | no (maps/global only) |
| MotionBlur Enable (3) == 1 | 3 / 3 | 5 / 5 | 3 / 3 | no (also elsewhere) |
| FeedBackBlur Enable == 1 | 0 / 0 | 0 / 0 | 0 / 0 | used nowhere |
| FeedBackBlur FeedbackWeight > 0 | 0 / 0 | 0 / 0 | 0 / 0 | used nowhere |
| LightShaft Enable == 1 | 37 / 17 | 43 / 43 | 0 / 0 | no (also elsewhere) |
| LightShaft ColorScale max > 8 | 0 / 0 | 0 / 0 | 0 / 0 | used nowhere |
| LightShaft DiffractionRing > 0 | 0 / 0 | 4 / 4 | 0 / 0 | no (maps/global only) |
| ColorGrading Enable == 1 | 29 / 26 | 37 / 37 | 0 / 0 | no (also elsewhere) |
| ColorGrading LutSourceId >= 0 | 29 / 26 | 37 / 37 | 0 / 0 | no (also elsewhere) |
| Anti-Alias Enable == 1 | 1 / 1 | 4 / 4 | 9 / 3 | no (also elsewhere) |
| Glow Filter Enable == 1 | 3 / 3 | 7 / 7 | 2 / 2 | no (also elsewhere) |
| Lens Simulation Enable == 1 | 19 / 19 | 36 / 36 | 0 / 0 | no (also elsewhere) |
| ColorParaffin Enable == 1 | 0 / 0 | 0 / 0 | 0 / 0 | used nowhere |
| SettingInfo Enable ColorGrading == 1 | 11 / 9 | 9 / 9 | 2 / 2 | no (also elsewhere) |
| SettingInfo BlurShape != 0 | 2 / 2 | 2 / 2 | 0 / 0 | no (also elsewhere) |
| SettingInfo GlareQuality >= 5 | 1 / 1 | 0 / 0 | 0 / 0 | YES (cutscenes only) |
| SettingInfo DepthOfFieldQuality >= 8 | 1 / 1 | 2 / 2 | 0 / 0 | no (also elsewhere) |
| SettingInfo Auto Compute Enable == 1 | 2 / 2 | 1 / 1 | 1 / 1 | no (also elsewhere) |

How to read the main rows (INFERRED where it goes beyond the names):

| YEBIS feature | used by cutscenes? | main cutscene range | consequence for a replacement chain |
|---|---|---|---|
| Exposure / Gamma (tone map, first run) | yes, 146 ids, all 49 files | Exposure 0..5.09 (typical 2.0 to 2.3); Gamma 2.2 except 0 (3 ids), 1.8, 2.0, 2.3, 2.48, 3.17, 4.62 | exposure is a per-id scalar to honour including 0 (black) and values above 2.5; a gamma that varies per id needs a per-id output transfer, not a fixed 2.2 |
| ToneType | 144 ids type 2, 2 ids type 3 | 2 or 3 | one extra tone curve in two ids of `s40_00_0040`; keep or approximate |
| FilmicContrast / ToneMappingFactor | yes | 0 to 1; factor 0.01 to 32 | per-id curve controls; factor 0.01 occurs only in cutscenes (13 ids) |
| ColorH / ColorS / ColorV / Contrust (colour matrix) | yes | H 0..3, S 0..1.094, V 0..4, Contrust 0.92..1.1625 | the matrix is animated by the timeline blend; V reaches 0 (fade) and 4 (flash); H 3 is outside the maps' 0..1 |
| Vignette (power, fov dependence) | yes, power in 105 of 146 ids | power 0..1.35; fov dependence 0..1 in 58 ids | a vignette that scales with camera field of view (cutscene-only) |
| Auto exposure | no (AutoExposure 0 in all 146 ids) | - | the cutscenes use fixed exposure; map files use auto exposure in 7 records |
| Bloom / glare (GlareLuminance, threshold, radius scale, levels) | yes, 129 ids | luminance 1..86.7, threshold 0..1.5, levels 5..8 (and 6 to 10 in maps) | shape 0 in 26 ids (the "no star" shape; STUDY refuses it) |
| Ghost / halo / afterimage / star length / streaks | no | all 0 | not needed |
| BloomLuminance, StarLuminance, BloomDispersion | one id (`s41_00_0000` id 0) | 35.05 / 1.97 / 1.88, Bloom Enable 1 | a single special-effect frame |
| Anamorphic flare | partly | AnamorphicFlareLuminance 0.04..1.54 in 10 ids (cutscene-only), ghost scale up to 10 | few ids; Bloom Enable (3) is on in 13 ids (8 files), never in map files |
| Depth of field | yes, main use | Enable 208 / 232 ids; F 0.01..80; focus distance 0.1..200; CCD 28.77..84.75; edge quality -1 in 229 ids (6 map records and mapbase/remobase differ: 2 or 4) | see section 4 |
| Light shaft | yes | Enable 1 in 37 ids (17 files); ColorScale 0.25..80; ShaftLength 0..7; no diffraction ring | map files use it more (43 files) and with rings; cutscene use is a subset |
| Motion blur | 3 ids | BlurTimeRatio 0.05 to 0.2, samples 4 to 5, falloff max 3 | small |
| Feedback blur | no | all disabled | not needed for cutscenes (the timeline still lists the group) |
| Colour grading (LUT) | yes, 29 ids | LUT ids 0, 1, 2, 3, 10 | a per-id LUT choice, blended |
| Anti-Alias | 1 id | AntiAliasType 4, FXAA-style terms | not cutscene-specific |

## 4. What the timelines add beyond the GPARAM ids

OBSERVED (`remobnd-census.txt`, property names found in the MQB schema strings, number of bundles containing each, out of 72; a name present means the bundle contains that track type, not that every cut uses it):

| property family | bundles |
|---|---|
| SceneDrawParam: `begin/end` x {Light 49, Fog 48, Scattering 48, Envmap 48, Shadow 50, DofGlareQuality 50, ToneMap 50, Dof 50, Bloom 50, ColorGrading 50, LightShaft 50, FeedbackBlur 50, MotionBlur 50, Antialias 50, Ssao 50, DominantLightMap 47} x {Bank, Id} | 47 to 50 each |
| `beginFadeCol` / `endFadeCol` (fade track, `FadeInOut` act) | 32 |
| `fadeCol` (model draw params, `forChr`/`forObj`/`forMap`) | 48 |
| `beginAlpha` / `endAlpha` (`DrawAlpha`) | 18 |
| `DOF` (Dmyobj properties) | 13 |
| `DoFocus` | 2 (+1 `DofFoucs`, a misspelling in the data) |
| `SSAO` | 3 |
| `Near`, `Far`, `Height`, `Width` | 16 |
| light-like: `DiffuseColor`, `SepcularColor`, `Radius`, `ShadingModel`, `Spot` | 26 / 21 / 26 / 2 |
| `talkParamId` (subtitle) | 21 |
| `materialName`, `paramName`, `blendRate` (material parameter edit) | 8 |
| `rumbleId`, `decalId`, `fadeType`, `lerpToIngameCameraTime`, `BlendMaskFadeRenge` | 9, 2, 4, 1, 1 |

- **Witness strings** (OBSERVED): `mqb-strings-s30_00_0030.txt` shows the full SceneDrawParam block 3 times (3 cuts, 0010 / 0020 / 0060), no `DoFocus`, one `DOF` string and a `DOF-T` string, `fadeCol` twice, no `beginFadeCol`; `mqb-strings-s45_00_0000.txt` shows the full block 5 times and a reduced block (only ToneMap, DofGlareQuality, Bloom, Dof ids) 5 more times (so `beginToneMapBank` occurs 10 times), `DoFocus` once right after `cam0010`, `DOF` once after `VFX`, `fadeCol` twice, 7 `talkParamId`. INFERRED: one SceneDrawParam track per cut, the reduced block being a second track class that only drives ToneMap / DofGlareQuality / Bloom / Dof (what drives it is not read).
- **How the values reach the screen** (OBSERVED in REMO-CUTSCENES-NATIVE sec. 3): the `SceneDrawParam` act (`140CCA850`) holds 12 entries per update and switches GPARAM groups by (bank, id) with a begin and an end blend. So the cutscene values of section 2 are blended over time between two ids (blend curve and duration are in the MQB values, not read). INFERRED consequence: a replacement chain must interpolate every parameter it takes from a group between two ids (exposure, gamma, ColorH/S/V, vignette, glare terms, DoF terms, LUT choice), and the ids of one group can differ by orders of magnitude (e.g. Exposure 0 to 2.2, ColorV 0 to 1, aperture F 0.01 to 80).
- **Fade tracks** (OBSERVED): the `FadeInOut` act sets fade plate 4 each update with a colour; the manager drives plate 3 for the enter and exit fade (REMO-CUTSCENES-NATIVE sec. 2 and 3). 32 bundles have a `beginFadeCol`/`endFadeCol` track; separately, 18 bundles fade a model draw colour (`beginAlpha`). These fades are a full-screen overlay applied by the fade system and are **not** part of the GPARAM Tone Map; a post chain that replaces YEBIS must still leave them drawn after (or inside) its output. The ColorV 0 ids of section 2 are an additional, post-chain way to reach black. INFERRED: the two do not conflict.
- **DOF objects and the DoFocus track** (OBSERVED names only): 13 bundles carry `DOF` properties (the `Dmyobj` act creates a 0xA0-byte object that copies a track matrix into it per update; REMO-CUTSCENES-NATIVE sec. 3, INFERRED to carry the DOF/SSAO/light objects), 2 bundles a `DoFocus` track, 16 bundles `Near/Far/Height/Width` properties. The meaning of these fields was not read.
- **What a per-cut DoF needs from a new chain** (INFERRED, not verified): (a) the focus distance as a per-id number (`DofFocusDistance`, 139 ids differ from 1), blended by the SceneDrawParam blend; (b) possibly a focus that follows an object: the `DoFocus` track and `DOF` objects have a transform (they are on dummy objects, INFERRED from the `Dmyobj` act), so a focal distance computed each frame as the camera-to-object distance would be a second source for the same value; this exists in at most 13 bundles and only the two `DoFocus` bundles are candidates for an animated target; (c) the camera lens (field of view and, since CCD size and aperture are in the GPARAM, the physical-lens inputs: F number 0.01 to 80, CCD 28.77 / 43.27 / 84.75) so that circle-of-confusion size can be derived the way YEBIS does it; (d) the near/far custom ranges (9 ids) as an override of the physical model; (e) edge quality, front/back aperture levels (1 to 7) and rotate scale as quality knobs (not cutscene-specific except the values 5, 7 of aperture levels at `s30_00_9000` and `s40_00_0020`).

## 5. Cutscene id to where it plays (from `emevd-cutscene-calls.txt`)

Id `AABBVVVV` -> file `sAA_BB_VVVV` (REMO-CUTSCENES-NATIVE sec. 1). The "2002 instructions" column keeps the arguments after the id exactly as extracted ([3] / [4] = instruction index); the x value (for example 3002810) looks like an entity or region id (INFERRED in that note). Only three ids have a role read in the inputs; the rest are left blank. Id 0 in `m33_00` is passed through an event parameter (filled by `2000[0]` with 33000020..33000043). 19 of the 49 GPARAM s-files (`s30_00_0000`, `s30_00_9000`, `s31_00_0010`, `s32_00_0000`, `s32_00_0020`, `s33_00_0020..0022`, `s33_00_0030..0032`, `s33_00_0040..0042`, `s37_00_0030`, `s38_00_0010`, `s39_00_0030`, `s40_00_0040`, `s41_00_0060`) have no direct call in this extract (INFERRED: called through event parameters or other instructions). 24 opened bundles have no GPARAM s-file (list in `idmap.py` output); for those the cutscene presumably uses the generic banks (INFERRED, not checked).

| cutscene id | s-file | bundle opened (census) | GPARAM s-file | map event file | event id | 2002 instructions (args after the id) | where it plays |
|---|---|---|---|---|---|---|---|
| 30000030 | s30_00_0030 | yes | yes | m30_00 | 13005810 | [4] 0,3002810,30,10000; [4] 10,3002810,30,10000; [3] 2,10000 | m30_00 (High Wall) boss introduction, entity 3000800 (INFERRED in REMO-CUTSCENES-NATIVE sec. 1) |
| 30000040 | s30_00_0040 | yes | yes | m30_00 | 13005830 | [3] 0,10000; [3] 10,10000; [3] 2,10000 | m30_00, cutscene with subtitles (talkParamId; event 13005830) |
| 30000020 | s30_00_0020 | yes | yes | m30_00 | 13005880 | [4] 0,3002880,30,10000; [4] 10,3002880,30,10000; [3] 2,10000 |  |
| 30000010 | s30_00_0010 | yes | yes | m30_00 | 13005899 | [4] 8,3002897,30,10000; [4] 10,3002897,30,10000; [3] 2,10000 |  |
| 31000000 | s31_00_0000 | yes | yes | m31_00 | 13105320 | [4] 0,3102270,31,10000 |  |
| 32000030 | s32_00_0030 | yes | yes | m32_00 | 13200811 | [4] 0,3202809,32,10000 |  |
| 32000010 | s32_00_0010 | yes | yes | m32_00 | 13205861 | [4] 8,3202858,32,10000; [4] 10,3202858,32,10000; [3] 2,10000 |  |
| 0 | s00_00_0000 | no | no | m33_00 | 13300412 | [3] 0,10000; [3] 10,10000; [3] 2,10000; [3] 0,10000 |  |
| 33000010 | s33_00_0010 | yes | yes | m33_00 | 13305810 | [3] 0,10000; [3] 10,10000; [3] 2,10000 |  |
| 33000000 | s33_00_0000 | yes | yes | m33_00 | 13305814 | [3] 0,10000; [3] 10,10000; [3] 2,10000 |  |
| 34010010 | s34_01_0010 | yes | yes | m34_01 | 13415830 | [4] 0,3412840,290,10000; [4] 10,3412840,290,10000; [3] 2,10000 |  |
| 34010000 | s34_01_0000 | yes | yes | m34_01 | 13415831 | [3] 0,10000; [3] 8,10000; [3] 2,10000 |  |
| 35000050 | s35_00_0050 | yes | no | m35_00 | 13505100 | [4] 0,4502100,45,10000 |  |
| 35000051 | s35_00_0051 | no | no | m35_00 | 13505100 | [4] 0,4502100,45,10000 |  |
| 35000000 | s35_00_0000 | yes | yes | m35_00 | 13500783 | [3] 0,10000; [4] 0,3502725,35,10000 |  |
| 35000001 | s35_00_0001 | yes | yes | m35_00 | 13500783 | [3] 0,10000; [4] 0,3502725,35,10000 |  |
| 37000020 | s37_00_0020 | yes | yes | m37_00 | 13705644 | [4] 0,0,37,10000; [3] 0,10000 |  |
| 37000021 | s37_00_0021 | yes | yes | m37_00 | 13705644 | [4] 0,0,37,10000; [3] 0,10000 |  |
| 38000020 | s38_00_0020 | yes | yes | m38_00 | 13800800 | [4] 8,3802804,38,10000; [4] 10,3802804,38,10000; [4] 2,3802807,38,10000 |  |
| 38000000 | s38_00_0000 | yes | yes | m38_00 | 13805809 | [4] 8,3802802,38,10000; [4] 10,3802802,38,10000; [4] 2,3802803,38,10000 |  |
| 39000020 | s39_00_0020 | yes | yes | m39_00 | 13905810 | [3] 0,10000 |  |
| 39000010 | s39_00_0010 | yes | yes | m39_00 | 13905900 | [4] 0,3202900,32,10000 |  |
| 40000020 | s40_00_0020 | yes | yes | m40_00 | 14005110 | [4] 0,4102100,41,10000 |  |
| 40000021 | s40_00_0021 | yes | yes | m40_00 | 14005110 | [4] 0,4102100,41,10000 |  |
| 41000070 | s41_00_0070 | yes | yes | m41_00 | 14100100 | [3] 2,10000 |  |
| 41000000 | s41_00_0000 | yes | yes | m41_00 | 14100101 | [3] 2,10000 |  |
| 41000050 | s41_00_0050 | yes | yes | m41_00 | 14100101 | [3] 2,10000 |  |
| 41000051 | s41_00_0051 | yes | yes | m41_00 | 14100101 | [3] 2,10000 |  |
| 45000030 | s45_00_0030 | yes | yes | m45_00 | 14505620 | [3] 0,10000; [3] 10,10000; [3] 2,10000 |  |
| 45000031 | s45_00_0031 | yes | yes | m45_00 | 14505620 | [3] 0,10000; [3] 10,10000; [3] 2,10000 |  |
| 45000000 | s45_00_0000 | yes | yes | m45_00 | 14505810 | [4] 0,4502810,45,10000; [4] 10,4502810,45,10000; [3] 2,10000 | m45_00 (DLC1) boss introduction, entities 4500800..4500802 (INFERRED, REMO-CUTSCENES-NATIVE sec. 1) |
| 45000010 | s45_00_0010 | yes | yes | m45_00 | 14505811 | [4] 0,4502812,45,10000; [4] 10,4502812,45,10000; [3] 2,10000 |  |
| 51000000 | s51_00_0000 | no | yes | m50_00 | 15005100 | [4] 0,5102110,51,10000 |  |
| 51010000 | s51_01_0000 | no | no | m51_00 | 15105500 | [4] 0,5112110,307,10000 |  |
| 51000010 | s51_00_0010 | no | no | m51_00 | 15105713 | [4] 0,0,51,10000; [4] 10,0,51,10000; [3] 2,10000 |  |


## 6. What could not be established

- **Which GPARAM ids each cut selects, and the begin/end blend curves**: they live in the `.mqb` SceneDrawParam tracks. No raw `remobnd`/`.mqb` file exists under `C:/Users/User/.codex/I0063-cache/files/` (only `param_drawparam_remobase.gparam.dcx` matched "remo") or under `gamedata-*`; no parser was written (the rule was to report from strings). So the timeline section is property-name presence only, and the per-file tables describe every id in a file, not the ids that a cut actually plays. The census lists property names in aggregate (counts of bundles), not per bundle; per-bundle contents are known only for the two witnesses.
- **Whether a bundle's cuts really switch to the id set of the same-named s-file**: assumed by name (INFERRED); also what bank the (bank, id) pairs of a bundle without an s-file use (24 bundles) was not checked.
- **The meaning of the second parameter run** in Tone Map / Bloom / Dof (section 1) and of `Dof Enable` type 3 versus type 7 (which one is the master switch); both are reported.
- **The DLC2 bundles** (`s50`, `s51`) were not opened (REMO-CUTSCENES-NATIVE), only the GPARAM `s51_00_0000` exists; `s51_00_0010`, `s51_01_0000` and `s35_00_0051` are called by EMEVD but have no bundle in the census.
- **Boss or ending identity** of most ids: no input names them; the area names for m30 (High Wall) and m45 (DLC1) come from the inputs, nothing else is stated.
- **Units and semantics** of DoF (focus distance in metres?, F-number, CCD size) and of `Bloom Enable (3)` are not stated by the data; only names and values are reported.
- Nothing was launched; no Git commands were used; nothing outside `investigations/I-0138-modern-post-chain/CENSUS-CUTSCENES.md` and `evidence/I-0138/artifacts/census-cutscenes/` was written.
