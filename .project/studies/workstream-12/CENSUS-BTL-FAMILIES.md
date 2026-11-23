# Study ws12: the game's placed bank lights by family (BTL census)

**Non-canonical until the study thread re-reads it.**

Date: 2026-11-22. Author: a read-only worker for the study thread of workstream 12 (« physical light sources »). No game, no build, no Ghidra was run.

Inputs (all read-only):

- `evidence/I-0110/btl-census.tsv` (2,431 lights of 29 light banks, SoulsFormats 1.0.1 names; I-0110 RESULT §5).
- `evidence/I-0110/msb-m30.tsv`, `msb-m40.tsv`: **these have no position column** (columns: kind, name, model, scale, draw/display groups, part flags). Positions were therefore re-exported with `Get-MsbPositions.ps1` (a variant of I-0110's `Get-MsbCensus.ps1`, copied, the original untouched; run with `pwsh`, hidden) from the recovered MSBs of `gamedata-i0110` (m30_00, m40_00) and `gamedata-i0113` (m37_00). Output `msbpos-m30_00.tsv`, `msbpos-m40_00.tsv`, `msbpos-m37_00.tsv`: map pieces, objects, enemies and every region type (SFX regions with their FFX id).
- `evidence/I-0068/queries/objparam/zone-effect-sources.tsv`, `ModelSfxParam.tsv`, `objectparam-sfx.tsv`: which object models carry an effect (ObjectParam or ModelSfxParam) and which FFX ids the SFX regions use, per zone.
- `evidence/I-0068/queries/ffx/effect-light-families.tsv` and `investigations/I-0068-static-yebis-ffx-lights-spec/EFFECT-LIGHTS.md` (effect lights, m40 witness); `evidence/I-0103/census/actions.tsv`, `actions-by-bundle.tsv` and `investigations/I-0103-effects-census/CENSUS.md` (FXR action counts).
- `evidence/I-0139/artifacts/static/npcs-merchants/msb-bonfires.tsv` (model `o000100` is the bonfire).

Scripts and per-light tables, in `C:/Users/User/Tools/DS3-Decompilation/study-ws12/btl-families/` (called S below): `Get-MsbPositions.ps1`, `common.py`, `glossary.py`, `core.py` (classification, tiers, frame transforms), `analyze.py` (writes the TSVs), `build_report.py` (this report's tables), `hough*.py`, `refine*.py`, `fit37*.py`, `fit40.py` (frame searches). Tables: `lights-classified.tsv` (one row per light of the game, 2,431 rows), `name-tokens.tsv` (every distinct name token, count, zones, gloss), `zone-summary.tsv`, `bank-diff.tsv`, `emitter-side.tsv`.

Tags: **OBSERVED** = read in a named file or computed from one; **INFERRED** = the author's reading, with the basis said. Every Japanese translation below is INFERRED (no name table exists in the repository).

Zone names are the community names of the map ids; only those that the repository itself states (RESULT I-0110 §5.1) are not marked INFERRED. Lights are counted per bank file: `m30_00` bank 0000 is « m30_00_0000 ».

## 0. Summary (answer first)

- **OBSERVED** 2,431 lights in 29 banks: 2,174 point, 234 spot, 23 directional (the « KeyLight » rows). 44 carry the `Unk27` flag (inferred shadow flag).
- **OBSERVED** colour (diffuse R minus B; warm at +40 or more, cold at -40 or less): 1,201 cold (50%), 581 neutral (24%), 626 warm (26%) of the 2,408 point and spot lights. The bank lights are mostly cold fill, not flame colour.
- **OBSERVED** by name prefix: dynamic(動的) 921, other(place-name) 856, Omni 244, Spot 202, Fill/Fake* 88, ADD: 49, ポイントライト(point light) 48, Direct/KeyLight 23.
- **OBSERVED, m30_00 (High Wall, bank 0000, 184 point/spot lights, frame validated)**: 17 within 3 m of a known emitter (2 within 1.5 m), 148 (80%) free-floating at 3 m. The 133 torch objects (`o304863`, `o004660`) carry their flame as an effect; almost no bank light sits on them.
- **OBSERVED, m37_00 (Irithyll, bank 0000, 523 point/spot lights, frame fitted)**: 215 (41%) within 3 m of a named emitter (146 within 1.5 m, 28%): candle, torch and lantern effect regions. The opposite of m30.
- **NOT DONE**: m40_00 (Firelink). Its MSB parts and its BTL positions are in different frames and the fit stayed ambiguous (section 2.1). Name-based classes are given for m40 (section 2.6).
- **OBSERVED** m30_00, m40_00, m37_00 are the only MSBs available as recovered files (m30_00 and m40_00 in `gamedata-i0110`, m37_00 in `gamedata-i0113`); every other zone has only name-based classes.
- Effect lights: 609 (point light action) occurs 2,457 times in 1,269 FXR files in 17 bundles; 11000 (spot) 7 times in 4 files; 11002 does not occur in the census (section 5).

## 1. Bank lights by family, whole game

### 1.1 Method and thresholds

- **Type**: record word `Unk18` (0 point, 1 spot, 2 directional), as I-0110 §5.1 (OBSERVED loader reading).
- **Colour class** (my choice, from the diffuse colour): `d = R - B` of the 0..255 bytes. Warm: d >= +40. Cold: d <= -40. Neutral: -40 < d < +40. Directional lights excluded from the colour statistics. The threshold sits on a local minimum of the histogram below (neutral is one flat bin of 80, not a trough, but warm and cold are the two clear groups). Histogram of d (bin width 40, lights of type 0 and 1):

| d = R-B bin | lights |
|---|---|
| -160..-121 | 125 |
| -120..-81 | 552 |
| -80..-41 | 515 |
| -40..-1 | 291 |
| 0..39 | 299 |
| 40..79 | 213 |
| 80..119 | 331 |
| 120..159 | 67 |
| 160..199 | 15 |

- **Radius bucket** (m, `Radius` word): <2, 2-4, 4-6, 6-8, 8-12, 12-20, >=20 (n/a for directional). **Brightness bucket** (`DiffuseBrightness`): <0.5, 0.5-1, 1-2, 2-4, 4-10, >=10.
- **Name tokens**: the name is split on `_`; trailing digits and a single trailing Latin letter after a Japanese word (`A000`) are removed; the empty remainder (a bare number) is dropped. **Name prefix class** is the first token: `dynamic(動的)`, `Omni`, `Spot`, `ADD:`, `Direct/KeyLight`, `Fill/Fake*` (a first token containing Fill, Fake or ウソ), `ポイントライト`, or `other(place-name)` (a Japanese place or function phrase).
- **Name tags**: a keyword inside the name that states what the designer meant (window 窓/ステンド, candle ろうそく/蝋燭/蜀台, chandelier, bonfire 篝火, hearth かまど/だんろ, lantern, torch, moonlight, fake ウソ/Fake, fill 補助/Fill, bounce バウンス/ブリン, ambient 環境, cutscene 演出, boss, keylight). These are names, not geometry: a light named « candle » may sit anywhere.

### 1.2 Whole-game table

| type | lights | % |
|---|---|---|
| point | 2174 | 89% |
| spot | 234 | 10% |
| directional | 23 | 1% |

| colour class (type 0,1) | lights | % |
|---|---|---|
| warm | 626 | 26% |
| neutral | 581 | 24% |
| cold | 1201 | 50% |

| radius bucket | lights | % |
|---|---|---|
| <2 | 14 | 1% |
| 2-4 | 110 | 5% |
| 4-6 | 587 | 24% |
| 6-8 | 651 | 27% |
| 8-12 | 588 | 24% |
| 12-20 | 196 | 8% |
| >=20 | 262 | 11% |
| n/a | 23 | 1% |

| diffuse brightness bucket | lights | % |
|---|---|---|
| <0.5 | 381 | 16% |
| 0.5-1 | 456 | 19% |
| 1-2 | 795 | 33% |
| 2-4 | 479 | 20% |
| 4-10 | 250 | 10% |
| >=10 | 70 | 3% |

| shadow flag (`Unk27`) | lights |
|---|---|
| true | 44 |
| false | 2387 |

| name prefix class | lights | % |
|---|---|---|
| dynamic(動的) | 921 | 38% |
| other(place-name) | 856 | 35% |
| Omni | 244 | 10% |
| Spot | 202 | 8% |
| Fill/Fake* | 88 | 4% |
| ADD: | 49 | 2% |
| ポイントライト(point light) | 48 | 2% |
| Direct/KeyLight | 23 | 1% |

| name tag (keyword in the name) | lights | warm | neutral | cold |
|---|---|---|---|---|
| boss | 360 | 105 | 149 | 106 |
| fill | 243 | 36 | 74 | 133 |
| window | 206 | 32 | 100 | 74 |
| ambient | 138 | 6 | 84 | 48 |
| bounce | 126 | 53 | 42 | 31 |
| fake | 90 | 11 | 34 | 45 |
| candle | 58 | 58 | 0 | 0 |
| bonfire | 56 | 11 | 34 | 11 |
| cutscene | 44 | 0 | 0 | 44 |
| keylight | 23 | 0 | 0 | 0 |
| chandelier | 16 | 10 | 0 | 6 |
| moonlight | 14 | 0 | 1 | 13 |
| torch | 10 | 10 | 0 | 0 |
| lantern | 6 | 6 | 0 | 0 |
| hearth | 2 | 2 | 0 | 0 |

**Reading (INFERRED).** The `動的` (« dynamic ») group is 921 lights (38%); Omni and Spot are the High Wall / Lothric Castle naming, the Japanese place-or-function phrases (m37, m45, m51) the rest. Only 148 lights (6%) carry a keyword for a light-source object (candle, chandelier, lantern, torch, bonfire, hearth) in their name; `bonfire` is partly a place name (篝火教会 « bonfire church », 16 lights at m37), so the true count is lower. All 58 `candle`-tagged and all 10 `torch`-tagged lights are warm; the 206 `window`-tagged lights are mostly neutral or cold (a window light is a sky-coloured fill, not a flame).

### 1.3 Per bank (every zone and bank)

| bank | zone | lights | point | spot | dir | warm | neutral | cold | shadow | median radius m | median brightness |
|---|---|---|---|---|---|---|---|---|---|---|---|
| dlc1_m47_00_0000 | DLC1 arena m47_00 | 5 | 5 | 0 | 0 | 0 | 0 | 5 | 0 | 42.10 | 0.10 |
| m30_00_0000 | High Wall of Lothric | 185 | 154 | 30 | 1 | 64 | 43 | 77 | 5 | 8.00 | 1.00 |
| m30_00_0001 | High Wall of Lothric | 133 | 126 | 6 | 1 | 39 | 25 | 68 | 2 | 8.00 | 1.00 |
| m30_01_0000 | Lothric Castle | 116 | 94 | 21 | 1 | 36 | 41 | 38 | 2 | 7.00 | 1.00 |
| m30_01_0001 | Lothric Castle | 92 | 75 | 16 | 1 | 20 | 14 | 57 | 2 | 8.00 | 1.00 |
| m31_00_0000 | Undead Settlement | 161 | 154 | 6 | 1 | 13 | 10 | 137 | 2 | 5.00 | 0.50 |
| m32_00_0000 | Archdragon Peak | 43 | 36 | 6 | 1 | 5 | 7 | 30 | 3 | 6.50 | 0.90 |
| m33_00_0000 | Road of Sacrifices / Farron Keep | 64 | 57 | 6 | 1 | 16 | 0 | 47 | 1 | 8.00 | 2.00 |
| m33_00_0001 | Road of Sacrifices / Farron Keep | 60 | 56 | 3 | 1 | 16 | 0 | 43 | 1 | 8.00 | 2.00 |
| m34_01_0000 | Grand Archives | 82 | 67 | 14 | 1 | 18 | 29 | 34 | 1 | 9.00 | 1.00 |
| m34_01_0001 | Grand Archives | 82 | 67 | 14 | 1 | 18 | 29 | 34 | 1 | 9.00 | 1.00 |
| m35_00_0000 | Cathedral of the Deep | 81 | 72 | 8 | 1 | 17 | 1 | 62 | 1 | 6.00 | 2.00 |
| m35_00_0001 | Cathedral of the Deep | 79 | 71 | 8 | 0 | 17 | 1 | 61 | 0 | 6.50 | 2.00 |
| m37_00_0000 | Irithyll of the Boreal Valley / Anor Londo | 524 | 478 | 45 | 1 | 204 | 159 | 160 | 3 | 7.00 | 1.20 |
| m38_00_0000 | m38_00 | 57 | 56 | 1 | 0 | 22 | 3 | 32 | 1 | 10.00 | 2.00 |
| m39_00_0000 | Irithyll Dungeon | 264 | 250 | 13 | 1 | 50 | 31 | 182 | 5 | 6.00 | 1.00 |
| m39_00_0001 | Irithyll Dungeon | 39 | 38 | 0 | 1 | 0 | 37 | 1 | 2 | 8.00 | 0.25 |
| m40_00_0000 | Firelink Shrine | 45 | 38 | 6 | 1 | 11 | 8 | 25 | 3 | 6.00 | 1.10 |
| m40_00_0001 | Firelink Shrine | 41 | 37 | 4 | 0 | 10 | 8 | 23 | 0 | 6.00 | 1.00 |
| m41_00_0000 | Untended Graves | 16 | 13 | 2 | 1 | 10 | 4 | 1 | 2 | 12.00 | 1.50 |
| m45_00_0000 | Ashes of Ariandel | 122 | 111 | 10 | 1 | 23 | 56 | 42 | 1 | 7.00 | 0.70 |
| m46_00_0000 | m46_00 | 1 | 0 | 0 | 1 | 0 | 0 | 0 | 1 | - | - |
| m47_00_0000 | base-game arena m47_00 | 5 | 5 | 0 | 0 | 0 | 0 | 5 | 0 | 42.10 | 0.10 |
| m50_00_0000 | Ringed City, m50 | 41 | 33 | 7 | 1 | 1 | 10 | 29 | 1 | 6.00 | 1.45 |
| m51_00_0000 | The Ringed City, m51 | 87 | 78 | 8 | 1 | 16 | 65 | 5 | 1 | 4.50 | 2.00 |
| m51_01_0000 | DLC2 m51_01 | 4 | 3 | 0 | 1 | 0 | 0 | 3 | 1 | 9.00 | 1.00 |
| m53_00_0000 | m53_00 | 1 | 0 | 0 | 1 | 0 | 0 | 0 | 1 | - | - |
| m54_00_0000 | m54_00 | 1 | 0 | 0 | 1 | 0 | 0 | 0 | 1 | - | - |

| bank | r<2 | r2-4 | r4-6 | r6-8 | r8-12 | r12-20 | r>=20 | b<0.5 | b0.5-1 | b1-2 | b2-4 | b4-10 | b>=10 |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| dlc1_m47_00_0000 | 0 | 0 | 0 | 0 | 0 | 0 | 5 | 5 | 0 | 0 | 0 | 0 | 0 |
| m30_00_0000 | 0 | 2 | 24 | 56 | 50 | 23 | 29 | 18 | 44 | 68 | 30 | 17 | 7 |
| m30_00_0001 | 0 | 2 | 23 | 40 | 41 | 20 | 6 | 18 | 33 | 56 | 22 | 3 | 0 |
| m30_01_0000 | 3 | 1 | 18 | 37 | 29 | 7 | 20 | 26 | 26 | 31 | 18 | 14 | 0 |
| m30_01_0001 | 3 | 1 | 17 | 23 | 27 | 4 | 16 | 12 | 24 | 30 | 14 | 11 | 0 |
| m31_00_0000 | 0 | 8 | 84 | 34 | 9 | 17 | 8 | 65 | 55 | 27 | 6 | 3 | 4 |
| m32_00_0000 | 0 | 4 | 12 | 8 | 10 | 2 | 6 | 8 | 13 | 10 | 3 | 3 | 5 |
| m33_00_0000 | 0 | 0 | 3 | 13 | 39 | 2 | 6 | 0 | 0 | 22 | 25 | 16 | 0 |
| m33_00_0001 | 0 | 0 | 3 | 13 | 38 | 2 | 3 | 0 | 0 | 21 | 26 | 12 | 0 |
| m34_01_0000 | 0 | 5 | 14 | 15 | 21 | 11 | 15 | 4 | 29 | 28 | 12 | 4 | 4 |
| m34_01_0001 | 0 | 5 | 14 | 15 | 21 | 11 | 15 | 4 | 29 | 28 | 12 | 4 | 4 |
| m35_00_0000 | 0 | 1 | 19 | 30 | 14 | 6 | 10 | 0 | 0 | 29 | 33 | 17 | 1 |
| m35_00_0001 | 0 | 1 | 17 | 30 | 15 | 6 | 10 | 0 | 0 | 27 | 34 | 17 | 1 |
| m37_00_0000 | 8 | 36 | 137 | 149 | 127 | 29 | 37 | 88 | 114 | 133 | 119 | 37 | 32 |
| m38_00_0000 | 0 | 1 | 4 | 17 | 9 | 10 | 16 | 0 | 2 | 26 | 5 | 23 | 1 |
| m39_00_0000 | 0 | 16 | 71 | 96 | 38 | 19 | 23 | 45 | 19 | 120 | 60 | 15 | 4 |
| m39_00_0001 | 0 | 3 | 4 | 9 | 22 | 0 | 0 | 36 | 1 | 0 | 0 | 1 | 0 |
| m40_00_0000 | 0 | 1 | 16 | 11 | 7 | 3 | 6 | 2 | 7 | 21 | 6 | 7 | 1 |
| m40_00_0001 | 0 | 2 | 14 | 12 | 6 | 3 | 4 | 3 | 8 | 19 | 6 | 4 | 1 |
| m41_00_0000 | 0 | 0 | 0 | 4 | 1 | 4 | 6 | 1 | 2 | 8 | 3 | 1 | 0 |
| m45_00_0000 | 0 | 9 | 33 | 25 | 40 | 8 | 6 | 34 | 37 | 26 | 11 | 9 | 4 |
| m46_00_0000 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 |
| m47_00_0000 | 0 | 0 | 0 | 0 | 0 | 0 | 5 | 5 | 0 | 0 | 0 | 0 | 0 |
| m50_00_0000 | 0 | 7 | 11 | 8 | 5 | 2 | 7 | 0 | 6 | 22 | 9 | 3 | 0 |
| m51_00_0000 | 0 | 5 | 49 | 6 | 17 | 6 | 3 | 2 | 6 | 26 | 23 | 29 | 0 |
| m51_01_0000 | 0 | 0 | 0 | 0 | 2 | 1 | 0 | 0 | 1 | 1 | 1 | 0 | 0 |
| m53_00_0000 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 |
| m54_00_0000 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 |

| bank | dynamic(動的) | Omni | Spot | ADD: | Direct/KeyLight | Fill/Fake* | ポイントライト(point light) | other(place-name) |
|---|---|---|---|---|---|---|---|---|
| dlc1_m47_00_0000 | 0 | 5 | 0 | 0 | 0 | 0 | 0 | 0 |
| m30_00_0000 | 68 | 81 | 23 | 0 | 1 | 0 | 0 | 12 |
| m30_00_0001 | 60 | 61 | 4 | 0 | 1 | 0 | 0 | 7 |
| m30_01_0000 | 14 | 56 | 21 | 0 | 1 | 0 | 24 | 0 |
| m30_01_0001 | 15 | 36 | 16 | 0 | 1 | 0 | 24 | 0 |
| m31_00_0000 | 28 | 0 | 4 | 0 | 1 | 65 | 0 | 63 |
| m32_00_0000 | 36 | 0 | 6 | 0 | 1 | 0 | 0 | 0 |
| m33_00_0000 | 57 | 0 | 6 | 0 | 1 | 0 | 0 | 0 |
| m33_00_0001 | 56 | 0 | 3 | 0 | 1 | 0 | 0 | 0 |
| m34_01_0000 | 67 | 0 | 14 | 0 | 1 | 0 | 0 | 0 |
| m34_01_0001 | 67 | 0 | 14 | 0 | 1 | 0 | 0 | 0 |
| m35_00_0000 | 67 | 0 | 8 | 0 | 1 | 0 | 0 | 5 |
| m35_00_0001 | 66 | 0 | 8 | 0 | 0 | 0 | 0 | 5 |
| m37_00_0000 | 0 | 0 | 32 | 0 | 1 | 1 | 0 | 490 |
| m38_00_0000 | 50 | 0 | 1 | 0 | 0 | 0 | 0 | 6 |
| m39_00_0000 | 147 | 0 | 5 | 18 | 1 | 21 | 0 | 72 |
| m39_00_0001 | 0 | 0 | 0 | 31 | 1 | 1 | 0 | 6 |
| m40_00_0000 | 38 | 0 | 6 | 0 | 1 | 0 | 0 | 0 |
| m40_00_0001 | 37 | 0 | 4 | 0 | 0 | 0 | 0 | 0 |
| m41_00_0000 | 13 | 0 | 2 | 0 | 1 | 0 | 0 | 0 |
| m45_00_0000 | 0 | 0 | 10 | 0 | 1 | 0 | 0 | 111 |
| m46_00_0000 | 0 | 0 | 0 | 0 | 1 | 0 | 0 | 0 |
| m47_00_0000 | 0 | 5 | 0 | 0 | 0 | 0 | 0 | 0 |
| m50_00_0000 | 32 | 0 | 7 | 0 | 1 | 0 | 0 | 1 |
| m51_00_0000 | 0 | 0 | 8 | 0 | 1 | 0 | 0 | 78 |
| m51_01_0000 | 3 | 0 | 0 | 0 | 1 | 0 | 0 | 0 |
| m53_00_0000 | 0 | 0 | 0 | 0 | 1 | 0 | 0 | 0 |
| m54_00_0000 | 0 | 0 | 0 | 0 | 1 | 0 | 0 | 0 |

**OBSERVED.** The per-bank counts of I-0110 §5.1 (m30_00 185/133, m37 524, m39 264/39, m40 45/41, m45 122, m51_00 87) are reproduced. Note that `m47_00` appears twice (a base-game bank and the DLC1 arena bank, 5 lights each) and that the 0001 banks of m30_00 (133) and m30_01 (92) are a different lighting state, not additions to 0000 (section 3).

### 1.4 Name tokens (every distinct token is in `S/name-tokens.tsv`, 501 tokens)

The gloss is the author's word-by-word reading in source word order (INFERRED; particles such as の « of » are kept). `*` = a part of the token is left untranslated in the TSV. The first 90 tokens by count:

| token | lights (distinct per token) | translation (INFERRED) | top zones |
|---|---|---|---|
| 動的 | 973 | dynamic (the largest group: 973 lights; INFERRED the lights the engine may switch or fade with the area state) | m39_00:147, m34_01:134, m35_00:133 |
| Omni | 237 | omni = point light | m30_00:142, m30_01:82, dlc1_m47_00:5 |
| Spot | 142 | spot light | m34_01:28, m30_00:27, m30_01:27 |
| 冒頭 | 72 | opening (the start of the zone) | m33_00:28, m34_01:26, m40_00:10 |
| GreenFillLight | 61 | green fill light | m31_00:51, m39_00:10 |
| 拠点内部 | 54 | base interior (the Firelink Shrine interior at m40) | m40_00:54 |
| 窓ライト | 53 | window light | m51_00:53 |
| ADD: | 50 | prefix « ADD: » (added; INFERRED a later addition to the bank, m39) | m39_00:50 |
| ポイントライト | 48 | point light | m30_01:48 |
| 冒頭通路 | 42 | opening passage | m32_00:36, m34_01:4, m39_00:2 |
| BossArea | 42 | boss area | m39_00:42 |
| 廃村 | 37 | ruined village | m45_00:37 |
| FakeLight | 35 | fake light | m39_00:35 |
| ガーデン地下の照明ブリン | 34 | garden-underground lighting « brin » | m37_00:34 |
| ステンド | 33 | stained glass | m34_01:32, m45_00:1 |
| 深淵ボス前 | 32 | in front of the abyss boss | m33_00:32 |
| 深淵ボス部屋 | 29 | abyss boss room | m33_00:29 |
| 補助 | 26 | auxiliary | m45_00:14, m30_00:12 |
| ランドマーク聖堂のろうそく | 26 | landmark-cathedral candle | m37_00:26 |
| 王城2繋ぎ広間窓補助 | 24 | royal castle 2 link hall window auxiliary | m30_01:24 |
| 補助ライト | 22 | auxiliary light | m51_00:18, m45_00:4 |
| ウソライト | 22 | fake light (ウソ = lie) | m31_00:22 |
| 聖堂前広場の小さい照明 | 22 | cathedral front plaza of small lighting | m37_00:22 |
| 霊廟 | 21 | mausoleum | m37_00:21 |
| ボス部屋 | 19 | boss room | m30_00:14, m39_00:3, m31_00:2 |
| 建物内バウンス | 19 | building inside bounce | m31_00:19 |
| 照明A環境 | 19 | lighting A ambient/environment | m37_00:19 |
| 教会ボス後部屋 | 18 | church boss rear room | m51_00:18 |
| 拠点接続 | 18 | base/hub connection | m30_00:18 |
| サリヴァーン部屋2階 | 18 | Sulyvahn room 2 floor | m37_00:18 |
| spot | 17 | spot | m51_00:8, m45_00:5, m35_00:2 |
| 竜ボス部屋 | 17 | dragon boss room | m30_00:17 |
| 地下牢接続の照明ブリン | 17 | dungeon connection of lighting brin(unclear) | m37_00:17 |
| FakeFillLight | 17 | fake fill light | m39_00:17 |
| 教会ボス部屋 | 16 | church boss room | m51_00:16 |
| 王城2大広間奥通路 | 16 | royal castle 2 large hall back/deep passage | m30_00:16 |
| 王城2広間光源用 | 16 | royal castle 2 hall light source for | m30_01:16 |
| 大シャンデリア | 16 | large chandelier | m35_00:16 |
| 塔の環境 | 16 | tower of ambient/environment | m37_00:16 |
| 篝火教会 | 16 | bonfire church | m37_00:16 |
| 照明 | 16 | lighting | m37_00:16 |
| 地底湖 | 16 | underground lake | m38_00:16 |
| 裏回廊 | 16 | back corridor | m39_00:16 |
| 最奥の建物 | 15 | innermost of building | m45_00:15 |
| 竜ボス前広場 | 15 | dragon boss front plaza | m30_00:15 |
| 地下牢接続の照明 | 15 | dungeon connection of lighting | m37_00:15 |
| ゴーレム部屋 | 15 | golem room | m39_00:15 |
| Direct | 14 | directional | m30_00:2, m30_01:2, m34_01:2 |
| 教会ボス前部屋 | 14 | church boss front room | m51_00:14 |
| 王城2繋ぎ篝火部屋出口 | 14 | royal castle 2 link bonfire room exit | m30_01:14 |
| 裏道門 | 14 | back road gate | m31_00:14 |
| スタート地点 | 14 | start point | m40_00:13, m31_00:1 |
| ２階通路 | 14 | ２ floor passage * | m35_00:14 |
| 冒頭階段 | 14 | opening stairs | m35_00:14 |
| 屋外ドア前 | 14 | outdoors door front | m35_00:14 |
| 地下牢接続の環境 | 14 | dungeon connection of ambient/environment | m37_00:14 |
| 地下墓地ライト | 13 | catacombs light | m45_00:13 |
| 教会妻部屋 | 13 | church wife room | m51_00:13 |
| 廊下 | 13 | corridor | m30_00:13 |
| 王城2繋ぎ窓用スポット | 13 | royal castle 2 link window for spot | m30_01:13 |
| 森後半接続 | 13 | forest latter half connection | m33_00:13 |
| 拠点 | 13 | base/hub | m41_00:13 |
| 王城1正面大階段 | 12 | royal castle 1 front face large stairs | m30_00:12 |
| 霊廟森 | 12 | mausoleum forest | m30_00:12 |
| 王城3ボス部屋回廊壁 | 12 | royal castle 3 boss room corridor wall | m34_01:12 |
| 王城3ボス部屋窓際暖色 | 12 | royal castle 3 boss room by the window warm colour | m34_01:12 |
| 隠し部屋 | 12 | hidden room | m34_01:12 |
| １階外壁 | 12 | １ floor outer wall * | m35_00:12 |
| ランドマーク聖堂２階のろうそく | 12 | landmark cathedral ２ floor of candle * | m37_00:12 |
| 厨房の照明ブリン | 12 | kitchen of lighting brin(unclear) | m37_00:12 |
| ランドマーク聖堂 | 11 | landmark cathedral | m37_00:11 |
| ランドマーク聖堂内部 | 11 | landmark cathedral interior | m37_00:11 |
| ガーデン地下の環境 | 11 | garden underground of ambient/environment | m37_00:11 |
| 冒頭空洞 | 11 | opening hollow | m38_00:11 |
| 後半冒頭 | 10 | latter half opening | m30_00:10 |
| 暗い青 | 10 | dark blue | m30_01:10 |
| omni | 10 | omni (point light) | m30_01:10 |
| 扉入り口用 | 10 | door entrance for | m30_01:10 |
| 王城2繋ぎ回廊 | 10 | royal castle 2 link corridor | m30_01:10 |
| 左部屋光源用 | 10 | left room light source for | m30_01:10 |
| Spot蝕 | 10 | S p o t eclipse | m30_01:10 |
| 演出用 | 10 | staging/cutscene for | m31_00:10 |
| 王城3ボス部屋サイド窓用スポット | 10 | royal castle 3 boss room side window for spot | m34_01:10 |
| 王城3ボス部屋ベット前 | 10 | royal castle 3 boss room bed front | m34_01:10 |
| NPC部屋 | 10 | NPC room | m35_00:10 |
| 祭壇外壁 | 10 | altar outer wall | m35_00:10 |
| 前半ルート | 10 | first half route | m38_00:10 |
| BigGreenFillLight | 10 | big green fill light | m39_00:10 |
| 塔裏道 | 10 | tower back road | m39_00:10 |
| TorchLight | 10 | torch light | m39_00:10 |

Other prefixes that are not Japanese words: `Omni`/`Omn`/`omni` (point), `Spot`/`spot`, `Direct`/`Direc` (directional), `KeyLight`, `GreenFillLight`/`BigGreenFillLight`/`JailPathFillLight`/`FakeFillLight`/`FillLight` (fill lights, mostly m39 and m38 INFERRED), `BossArea`, `TorchLight` (10 lights; their zones are in the TSV), `FakeLight`, `Tower`, `Titan*`, `PathToTitanCage`, `Underlabyrinth`, `katota` (unclear), `DY`, `RO`, `breakpoint` (unclear).

The complete table (all 501 tokens, zones and gloss) is `S/name-tokens.tsv`; the tail of 1- and 2-light tokens is mostly one-off place or function phrases (« corner of the manor first floor », « light that makes the lever conspicuous »).

## 2. Visible emitter or free-floating

### 2.1 The two frames: which zones could be placed

**The MSB tables of I-0110 have no positions** (see Inputs); they were re-exported. Then the lights and the MSB parts had to be put in one frame.

- **m30_00: identity** (OBSERVED, no fit). Nearest map-piece origin: median 4.9 m, 163 of 184 point/spot lights within 10 m; nearest object: median 5.2 m, 143 of 184 within 10 m. With the same test and no transform m37_00 gives a median of 419 m (objects) and m40_00 100 m: those frames differ.
- **m37_00: a fitted rigid transform** (INFERRED by a search I wrote, `hough4.py`, `fit37b.py`): MSB = rotation about Y by 179.25 degrees of the BTL position, plus (423.0, -242.0, -925.5), with `x' = cos(a) x + sin(a) z + tx`, `z' = -sin(a) x + cos(a) z + tz`. The search first found a sharp peak at 180 degrees (278 of 523 distinct lights fell in one 10 m cell against a median of 138 over all angles; 176 and 184 degrees gave 237 and 218, none other above 190), then a hill climb on the count of lights within 2.5 m of an object, enemy or SFX region: 209 of 523 (40 %) within 2.5 m, 149 (28 %) within 1.5 m. A random frame gives about 2 % (m30 identity: 3 of 184 within 1.5 m). The scoring included the SFX regions, so the next fact is not an independent test but is the strongest evidence: 18 of the 19 « torch flame A » regions (FFX 800023, named in the region label) have a bank light within 1.5 m. The accuracy of the frame was not measured separately; 18 of 19 torch regions within 1.5 m suggests errors of the order of 1 m (INFERRED). The half turn agrees with NATIVE-LIGHTING-MAP (the m37 sun is « 1.6 degrees past the original, twice the rows' 0.8-degree offset from a half turn »).
- **m40_00: not placed.** Hough searches over translation and rotation (hough*.py, refine*.py, `fit40.py`) give a ridge, not a peak: yaw 21 to 32 degrees with a coupled translation (for example 23.1 degrees, t = (-152, 5, -5), 19 of 44 lights within 2.5 m of an object, enemy or SFX region), no candidate above 21 of 44 and no way to tell the ridge points apart (the point cloud is a corridor). The lights' z (445 to 777) exceeds the objects' z (175 to 652), so the identity is excluded (nearest object 10 to 159 m). **I did not classify the m40 lights against the MSB.** The effect lights of the original (I-0068 table, section 6) do not help: they are in the MSB frame, not the BTL frame.
- The BTL positions are map-local and « placed by the area's world transform » (RESULT I-0110 §5.1); the area transform of m30_00 is the identity, that of m37_00 a half turn plus a translation (INFERRED from the fit above). This is why identity works for m30 only.

### 2.2 What counts as an emitter and how each was judged

No object-name table exists in the repository. An object model is called an emitter only through the effect that the game binds to it (OBSERVED tables), never through its model name alone:

- **Tier A, a named light-emitting thing.** (a) Object models whose ModelSfxParam row names FFX 800023, which the m37 SFX region label calls « 松明の炎A » (torch flame A): `o304863` (35 instances) and `o004660` (98) at m30 (`EFFECTS-ENTRY-DOOR-NATIVE.md` §E-D8 documents `o304863` as the torch), `o372010` (12) at m37; FFX 800061 labelled « 薪の炎 » (firewood flame) on `o302526` (4). (b) SFX regions whose label names a flame or lamp: m37 `800023` torch ×19, `800061` ×1, `837010` bridge lanterns ×23, `837011` plaza lanterns ×30, `837012` candles ×167, `302022` Anri's seven-colour stone ×2; m30 `833050` « forest flame » ×3. (c) `o000100`: the bonfire (named in `msb-bonfires.tsv`, I-0139). All OBSERVED labels, INFERRED translation of the Japanese.
- **Tier B, an effect is bound to the object, kind unknown.** ModelSfxParam or ObjectParam rows name an FFX id that no table in the repository labels: 800050, 800052, 800060, 800062, 800030/800043, 800044, 830060-830076 and the two commonest ids `839010`. For `839010`: INFERRED that it carries small warm point lights (the m40 witness has 85 effect point lights of radius 2, colour (0.47, 0.31, 0.20), x 120-286, z 564-622, the range of the 409 `o391000` instances that carry 839010 at m40; I-0068 EFFECT-LIGHTS §3 reads them as candle-like). Tier B means « could emit »; it is not confirmed.
- **Tier C, generic** (not counted as an emitter): FFX 130000 (`o001010`, `o001020`, `o001050` ...) is a generic object effect.
- **Window / opening (W).** I **cannot establish windows from the MSB tables**: they hold no window flag and no model class. The only opening-like object is the effect region: m30_00 has 10 « light shaft » regions (王城1ライトシャフト, FFX 830001-830008) and 1 « boss-room stained glass » region (FFX 830000); a light within the distance of one of them is classed « near light-shaft/stained-glass effect ». m37 and m40 have no such region class in the placed tables (m40 has 3 « 拠点ライトシャフト » regions but m40 is not placed). The name tag `window` (窓, ステンド) is the other, weaker, source.
- **Other object.** Any other placed object (including the Tier C ones): « at an object that carries no known light effect ».
- Distances are from the light position to the **MSB part origin**. The flame sits a little above a torch origin (the dummy point 200), so a true emitter can read as 0.5 to 1.5 m away; a bank light placed beside the model by hand can read further. Hence the two thresholds 1.5 m and 3 m.

### 2.3 What the placed objects are (counts)

m30_00: 2496 objects of 116 models; the 12 commonest: o001090 ×299, o001080 ×297, o304005 ×173, o001050 ×164, o004660 ×98, o001110 ×88, o001240 ×74, o000250 ×56, o302512 ×55, o304852 ×51, o301897 ×46, o302552 ×43.

m37_00: 469 objects of 77 models; the 12 commonest: o000250 ×74, o001090 ×74, o374000 ×29, o373017 ×28, o373016 ×25, o373010 ×15, o372001 ×14, o373023 ×14, o373004 ×13, o372010 ×12, o000401 ×11, o371001 ×10.

### 2.4 m30_00, High Wall of Lothric (frame: identity)

**OBSERVED.** 184 point and spot lights of bank 0000 (the directional light excluded). Placed objects: 2496, SFX regions: 238; Tier A instances 146, Tier B 342.

| class | within 1.5 m | % | within 3 m | % |
|---|---|---|---|---|
| at-emitter A (named flame/candle/lantern/torch/bonfire) | 0 | 0% | 7 | 4% |
| at-emitter B (effect-bearing object, kind unnamed) | 2 | 1% | 10 | 5% |
| near light-shaft/stained-glass effect | 1 | 1% | 3 | 2% |
| at other object (no known light effect) | 1 | 1% | 16 | 9% |
| free-floating | 180 | 98% | 148 | 80% |

Class crossed with colour class (3 m):

| class | lights | warm | neutral | cold |
|---|---|---|---|---|
| at-emitter A (named flame/candle/lantern/torch/bonfire) | 7 | 1 | 0 | 6 |
| at-emitter B (effect-bearing object, kind unnamed) | 10 | 6 | 2 | 2 |
| near light-shaft/stained-glass effect | 3 | 2 | 0 | 1 |
| at other object (no known light effect) | 16 | 1 | 2 | 13 |
| free-floating | 148 | 54 | 39 | 55 |

Class crossed with colour class (1.5 m):

| class | lights | warm | neutral | cold |
|---|---|---|---|---|
| at-emitter A (named flame/candle/lantern/torch/bonfire) | 0 | 0 | 0 | 0 |
| at-emitter B (effect-bearing object, kind unnamed) | 2 | 2 | 0 | 0 |
| near light-shaft/stained-glass effect | 1 | 0 | 1 | 0 |
| at other object (no known light effect) | 1 | 0 | 0 | 1 |
| free-floating | 180 | 62 | 42 | 76 |

Class (3 m) crossed with name prefix class:

| class | lights | name prefix classes |
|---|---|---|
| at-emitter A (named flame/candle/lantern/torch/bonfire) | 7 | Omni 5, other(place-name) 1, dynamic(動的) 1 |
| at-emitter B (effect-bearing object, kind unnamed) | 10 | dynamic(動的) 6, Spot 2, other(place-name) 1, Omni 1 |
| near light-shaft/stained-glass effect | 3 | Spot 2, dynamic(動的) 1 |
| at other object (no known light effect) | 16 | Omni 10, dynamic(動的) 5, other(place-name) 1 |
| free-floating | 148 | Omni 65, dynamic(動的) 55, Spot 19, other(place-name) 9 |

Class (3 m) crossed with the name tag (a light can carry several tags; `none` = no keyword):

| class | lights | name tags |
|---|---|---|
| at-emitter A (named flame/candle/lantern/torch/bonfire) | 7 | none 7 |
| at-emitter B (effect-bearing object, kind unnamed) | 10 | none 7, boss 2, window 1, fill 1 |
| near light-shaft/stained-glass effect | 3 | none 2, window 1, boss 1 |
| at other object (no known light effect) | 16 | none 14, fill 1, window 1, boss 1 |
| free-floating | 148 | none 105, boss 28, fill 15 |

Free-floating lights (3 m): the nearest placed object or SFX region of any kind is at median 5.7 m (min 0.7, max 104.9); the nearest map-piece origin at median 4.9 m. (INFERRED: map-piece origins are poor proxies; a piece origin is not its geometry.)

15 examples of each class (3 m threshold; all of them when fewer than 15). Position is the BTL position; « nearest » is the nearest of the class's own tier, or the nearest object of any kind for « other » and « free-floating »:

*at-emitter A (named flame/candle/lantern/torch/bonfire)* (7 lights):

| name | type | position | diffuse RGB | radius m | brightness | nearest (kind:model:name) | distance m |
|---|---|---|---|---|---|---|---|
| Omni254 | point | (-41.1, -3.7, 40.4) | (150,212,255) | 6 | 1 | o004660:o004660_8022 | 1.74 |
| 広場中央の像スポット001 | spot | (-33.8, 8.2, 104.2) | (255,232,189) | 23 | 20 | o004660:o004660_8059 | 2.03 |
| Omni266 | point | (-121.5, 30.1, 70.8) | (164,191,225) | 4.3 | 1.5 | o004660:o004660_8076 | 2.19 |
| Omni253 | point | (2.9, 1.7, 59.8) | (147,204,255) | 6 | 1.5 | o004660:o004660_8044 | 2.30 |
| 動的_塔入り口_001 | point | (-120.0, 30.1, 49.1) | (168,211,255) | 8 | 0.5 | o004660:o004660_8053 | 2.33 |
| Omni263 | point | (-32.6, 1.2, 87.1) | (197,227,255) | 6 | 0.8 | o304863:o304863_8030 | 2.37 |
| Omni227 | point | (2.4, 27.0, -5.2) | (147,211,255) | 4 | 1.3 | o004660:o004660_8056 | 2.44 |

*at-emitter B (effect-bearing object, kind unnamed)* (10 lights):

| name | type | position | diffuse RGB | radius m | brightness | nearest (kind:model:name) | distance m |
|---|---|---|---|---|---|---|---|
| Spot_冒頭部屋祭壇用_001 | spot | (-24.6, 53.1, -64.0) | (238,220,210) | 8 | 5 | o301240:o301240_1109 | 2.93 |
| 動的_牢屋_002 | point | (-81.6, 13.0, 33.3) | (255,175,144) | 4 | 0.7 | o301240:o301240_1000 | 1.40 |
| 動的_拠点接続_007 | point | (-64.3, -53.0, 499.9) | (255,189,144) | 5 | 1.2 | o001240:o001240_5032 | 1.29 |
| Omni260 | point | (-119.2, 24.6, 71.7) | (137,191,255) | 4.6 | 2.5 | o304517:o304517_1007 | 1.82 |
| 動的_拠点接続_003 | point | (-100.8, -51.2, 478.4) | (255,218,169) | 7 | 1.5 | o001240:o001240_5043 | 2.23 |
| 動的_竜ボス奥扉_001 | point | (-91.8, -45.6, 439.1) | (255,189,144) | 5 | 1.2 | o001240:o001240_5065 | 2.25 |
| 動的スタート地点壁補助_005 | point | (-23.1, 51.7, -59.4) | (255,229,213) | 2.8 | 0.7 | o301240:o301240_1124 | 2.35 |
| 動的_ショートカット塔_004 | point | (-21.2, 56.3, -55.4) | (229,239,255) | 12 | 0.4 | o302525:o302525_8013 | 2.52 |
| ボス部屋_正面ステンドグラス001 | spot | (27.6, -1.8, 169.2) | (125,194,255) | 14 | 2 | o304857:o304857_6013 | 2.67 |
| Spot_拠点接続通路_002 | spot | (-33.6, -49.2, 499.0) | (255,193,150) | 44.7 | 2 | o001240:o001240_5070 | 2.86 |

*near light-shaft/stained-glass effect* (3 lights):

| name | type | position | diffuse RGB | radius m | brightness | nearest (kind:model:name) | distance m |
|---|---|---|---|---|---|---|---|
| Spot_塔内部_002 | spot | (-125.1, 25.2, 73.1) | (251,240,210) | 28.7 | 10 | Region:SFX:ffx=830005:【GD_SFX:王城1ライトシャフト】_ID830005_1 | 1.95 |
| 動的_ボス部屋丸窓 | point | (27.6, 8.7, 132.2) | (147,195,238) | 7 | 4 | Region:SFX:ffx=830000:【GDボス部屋ステンドグラス】_ID830000_1000 | 2.74 |
| Spot_廊下_001 | spot | (-35.5, 13.6, 71.3) | (255,225,191) | 60 | 7 | Region:SFX:ffx=830001:【GD_SFX:王城1ライトシャフト】_ID830001_1 | 2.96 |

*at other object (no known light effect)* (16 lights):

| name | type | position | diffuse RGB | radius m | brightness | nearest (kind:model:name) | distance m |
|---|---|---|---|---|---|---|---|
| Omni252 | point | (-28.6, -3.8, 59.6) | (124,214,255) | 8 | 2 | o302555:o302555_3019 | 1.45 |
| 動的_ショートカット塔_002 | point | (0.4, 27.1, 1.6) | (170,217,255) | 2 | 0.2 | o300510:o300510_1001 | 1.69 |
| 動的_ショートカット塔_001 | point | (4.7, 4.4, 4.4) | (175,199,255) | 4 | 1 | o300510:o300510_1000 | 1.82 |
| Omni243 | point | (-12.5, -5.8, 55.6) | (140,202,255) | 8 | 0.6 | o302553:o302553_3008 | 1.86 |
| 動的_塔エレベータ_001 | point | (-132.2, 19.5, 59.0) | (162,191,255) | 5 | 1.5 | o300510:o300510_1002 | 1.89 |
| Omni228 | point | (20.4, 35.8, -12.7) | (158,199,255) | 4 | 1.5 | o302051:o302051_1008 | 1.92 |
| Omni216 | point | (-16.8, -3.9, 79.4) | (124,209,255) | 6 | 1.2 | o304852:o304852_3003 | 1.96 |
| Omni_最初の城壁内 | point | (-73.4, 30.0, -25.0) | (255,215,176) | 7 | 0.1 | o001050:o001050_1077 | 2.03 |
| ボス部屋_正面ステンドグラス | spot | (27.8, 7.1, 128.6) | (157,183,220) | 60 | 3 | o309400:o309400_6000 | 2.28 |
| Omni268 | point | (-46.6, 6.9, 87.0) | (185,215,229) | 4.5 | 0.5 | o000200:o000200_0002 | 2.41 |
| Omni246 | point | (-21.4, -2.5, 65.5) | (140,212,255) | 8 | 1.5 | o302556:o302556_3015 | 2.51 |
| 動的_渡り廊下_001 | point | (-51.7, 11.9, 63.3) | (155,215,255) | 5 | 0.8 | o001100:o001100_0009 | 2.63 |
| Omni258 | point | (-105.1, 20.0, 64.5) | (137,191,255) | 4 | 1.5 | o001091:o001091_0001 | 2.70 |
| Omni_王城1正面大階段_補助_004 | point | (22.9, -15.3, 74.0) | (255,235,220) | 9 | 0.4 | o302590:o302590_1020 | 2.78 |
| Omni261 | point | (-106.1, 25.2, 64.4) | (137,191,255) | 5 | 2 | o001050:o001050_1170 | 2.92 |

*free-floating* (148 lights):

| name | type | position | diffuse RGB | radius m | brightness | nearest (kind:model:name) | distance m |
|---|---|---|---|---|---|---|---|
| Omni255 | point | (48.2, -3.9, 84.8) | (207,235,255) | 7 | 1.5 | Region:SFX:ffx=830047:【GD_SFX:王城1舞い上がる落ち葉】_ID830047_ | 0.70 |
| 動的_倉庫_001 | point | (-85.1, 19.1, 65.3) | (130,214,255) | 6 | 1.5 | o001110:o001110_1027 | 3.12 |
| Spot_廊下_002 | spot | (-35.2, 13.5, 74.2) | (255,225,191) | 60 | 7 | o000250:o000250_0031 | 3.42 |
| Omni247 | point | (47.7, -1.0, 117.4) | (141,205,255) | 8 | 2 | Region:SFX:ffx=830047:【GD_SFX:王城1舞い上がる落ち葉】_ID830047_ | 3.67 |
| 動的_竜ボス前広場_009 | point | (-74.7, -48.1, 363.6) | (214,238,255) | 7 | 1 | o000401:o000401_5000 | 3.93 |
| Spot_廊下_006 | spot | (-34.5, 13.2, 68.5) | (255,225,191) | 60 | 6 | Region:SFX:ffx=830001:【GD_SFX:王城1ライトシャフト】_ID830001_1 | 4.30 |
| Omni_王城1霊廟エレベータ内部_006 | point | (-17.8, -1.3, 269.7) | (192,225,255) | 6 | 2 | o300510:o300510_5002 | 4.70 |
| Omni267 | point | (-47.3, 11.2, 103.2) | (185,215,229) | 4.5 | 0.7 | o001080:o001080_3091 | 5.00 |
| Omni_王城1霊廟補助オムニ_004 | point | (-73.9, -39.9, 247.4) | (255,217,183) | 15 | 0.6 | o304815:o304815_5025 | 5.57 |
| ボス部屋_祭壇奥 | point | (27.4, 2.3, 185.4) | (218,240,247) | 9 | 0.4 | o304863:o304863_6010 | 6.13 |
| Omni_王城2大広間奥通路_008 | point | (32.4, 2.3, 196.5) | (255,220,144) | 6 | 0.8 | o304863:o304863_6011 | 6.84 |
| 動的_噴水広場_001 | point | (-31.7, 3.0, 56.9) | (113,208,255) | 8 | 1.2 | o000250:o000250_0032 | 7.46 |
| 広場中央の像スポット | spot | (-32.3, 5.8, 67.2) | (218,229,189) | 23 | 40 | o001050:o001050_3074 | 8.58 |
| Omni_霊廟森_ルート補助_004 | point | (-53.5, -39.4, 265.6) | (255,225,191) | 6 | 1 | Region:SFX:ffx=830047:【GD_SFX:王城1舞い上がる落ち葉】_ID830047_ | 9.71 |
| Omni_王城1ショトカガーデン2_002 | point | (-7.7, 8.9, 201.4) | (255,253,233) | 7 | 0.5 | o004660:o004660_5003 | 12.77 |

Emitter side: for each emitter set, how many instances have a bank light within 1.5 m and 3 m (bank 0000, point and spot lights). This answers « does the bank already light this emitter? »:

| tier | emitter set | instances | with a light <=1.5 m | with a light <=3 m |
|---|---|---|---|---|
| A | object o004660 | 98 | 0 | 8 |
| A | object o304863 | 35 | 0 | 2 |
| A | object o000100 | 6 | 0 | 0 |
| A | object o302526 | 4 | 0 | 0 |
| A | sfx ffx=833050 【GD_SFX:王城1森_炎】 | 3 | 0 | 0 |
| B | object o001240 | 74 | 1 | 8 |
| B | object o304854 | 36 | 0 | 0 |
| B | object o302565 | 33 | 0 | 0 |
| B | object o301240 | 31 | 1 | 9 |
| B | object o302520 | 27 | 0 | 0 |
| B | object o301896 | 26 | 0 | 0 |
| B | object o304857 | 20 | 0 | 2 |
| B | object o304783 | 16 | 0 | 0 |
| B | object o301890 | 15 | 0 | 0 |
| B | object o302525 | 14 | 0 | 1 |
| B | object o301885 | 12 | 0 | 0 |
| B | object o001510 | 11 | 0 | 0 |
| B | object o304517 | 10 | 0 | 1 |
| B | object o301895 | 6 | 0 | 0 |
| B | object o302564 | 6 | 0 | 0 |
| B | object o301880 | 1 | 0 | 0 |
| B | object o301881 | 1 | 0 | 0 |
| B | object o301883 | 1 | 0 | 0 |
| B | object o301898 | 1 | 0 | 0 |
| B | object o304853 | 1 | 0 | 0 |
| C | object o001050 | 164 | 0 | 5 |
| C | object o001020 | 21 | 0 | 1 |
| C | object o001010 | 2 | 0 | 1 |
| C | object o001030 | 1 | 0 | 0 |
| C | object o001031 | 1 | 0 | 0 |
| C | object o001032 | 1 | 0 | 0 |
| C | object o001033 | 1 | 0 | 0 |
| C | object o001034 | 1 | 0 | 0 |
| W | sfx ffx=830005 【GD_SFX:王城1ライトシャフト】 | 5 | 1 | 2 |
| W | sfx ffx=830001 【GD_SFX:王城1ライトシャフト】 | 1 | 0 | 1 |
| W | sfx ffx=830003 【GD_SFX:王城1ライトシャフト】 | 1 | 0 | 0 |
| W | sfx ffx=830006 【GD_SFX:王城1ライトシャフト】 | 1 | 0 | 0 |
| W | sfx ffx=830007 【GD_SFX:王城1ライトシャフト】 | 1 | 0 | 0 |
| W | sfx ffx=830008 【GD_SFX:王城1ライトシャフト】 | 1 | 0 | 0 |
| W | sfx ffx=830000 【GDボス部屋ステンドグラス】 | 1 | 0 | 1 |
| N | all other SFX regions (fog, snow, ash, leaves, wind: not light emitters) | 224 | 3 | 14 |

Statistical candidates (INFERRED, weak): placed object models with no known effect (tier O), at least 3 instances, of which a quarter or more have a bank light within 3 m: o001100 (3 of 6), o300510 (3 of 8), o000200 (1 of 4). Their names are not known; a model that attracts lights may be a lamp, a brazier, a window frame or a doorway. They are listed only so a later reader can look at them.

Bank 0001 of m30_00 (132 point/spot lights), same classes at 3 m: at-emitter A 6, at-emitter B 9, near light-shaft/stained-glass effect 0, at other object 16, free-floating 101. Per-light rows are in `lights-classified.tsv`.

### 2.5 m37_00, Irithyll / Anor Londo (frame: fitted, INFERRED)

**OBSERVED.** 523 point and spot lights of bank 0000 (the directional light excluded). Placed objects: 469, SFX regions: 304; Tier A instances 263, Tier B 102.

| class | within 1.5 m | % | within 3 m | % |
|---|---|---|---|---|
| at-emitter A (named flame/candle/lantern/torch/bonfire) | 146 | 28% | 213 | 41% |
| at-emitter B (effect-bearing object, kind unnamed) | 0 | 0% | 2 | 0% |
| near light-shaft/stained-glass effect | 0 | 0% | 0 | 0% |
| at other object (no known light effect) | 2 | 0% | 9 | 2% |
| free-floating | 375 | 72% | 299 | 57% |

Class crossed with colour class (3 m):

| class | lights | warm | neutral | cold |
|---|---|---|---|---|
| at-emitter A (named flame/candle/lantern/torch/bonfire) | 213 | 174 | 39 | 0 |
| at-emitter B (effect-bearing object, kind unnamed) | 2 | 1 | 0 | 1 |
| near light-shaft/stained-glass effect | 0 | 0 | 0 | 0 |
| at other object (no known light effect) | 9 | 2 | 5 | 2 |
| free-floating | 299 | 27 | 115 | 157 |

Class crossed with colour class (1.5 m):

| class | lights | warm | neutral | cold |
|---|---|---|---|---|
| at-emitter A (named flame/candle/lantern/torch/bonfire) | 146 | 120 | 26 | 0 |
| at-emitter B (effect-bearing object, kind unnamed) | 0 | 0 | 0 | 0 |
| near light-shaft/stained-glass effect | 0 | 0 | 0 | 0 |
| at other object (no known light effect) | 2 | 1 | 0 | 1 |
| free-floating | 375 | 83 | 133 | 159 |

Class (3 m) crossed with name prefix class:

| class | lights | name prefix classes |
|---|---|---|
| at-emitter A (named flame/candle/lantern/torch/bonfire) | 213 | other(place-name) 212, Spot 1 |
| at-emitter B (effect-bearing object, kind unnamed) | 2 | other(place-name) 2 |
| near light-shaft/stained-glass effect | 0 |  |
| at other object (no known light effect) | 9 | other(place-name) 9 |
| free-floating | 299 | other(place-name) 267, Spot 31, Fill/Fake* 1 |

Class (3 m) crossed with the name tag (a light can carry several tags; `none` = no keyword):

| class | lights | name tags |
|---|---|---|
| at-emitter A (named flame/candle/lantern/torch/bonfire) | 213 | none 85, bounce 75, candle 45, lantern 5, fill 2, hearth 2, ambient 2, bonfire 1 |
| at-emitter B (effect-bearing object, kind unnamed) | 2 | ambient 1, none 1 |
| near light-shaft/stained-glass effect | 0 |  |
| at other object (no known light effect) | 9 | ambient 6, bounce 2, bonfire 1, none 1 |
| free-floating | 299 | ambient 129, none 107, bonfire 30, window 23, bounce 21, moonlight 14, fill 3, fake 2 |

Free-floating lights (3 m): the nearest placed object or SFX region of any kind is at median 8.0 m (min 3.0, max 57.2); the nearest map-piece origin at median 5.1 m. (INFERRED: map-piece origins are poor proxies; a piece origin is not its geometry.)

15 examples of each class (3 m threshold; all of them when fewer than 15). Position is the BTL position; « nearest » is the nearest of the class's own tier, or the nearest object of any kind for « other » and « free-floating »:

*at-emitter A (named flame/candle/lantern/torch/bonfire)* (213 lights):

| name | type | position | diffuse RGB | radius m | brightness | nearest (kind:model:name) | distance m |
|---|---|---|---|---|---|---|---|
| 厨房の照明ブリンA011 | point | (119.0, -26.4, 47.2) | (255,200,160) | 8 | 0.8 | Region:SFX:ffx=800023:【GD_SFX:松明の炎A】_ID800023_1011 | 0.18 |
| 厨房の照明ブリンA005 | point | (98.7, -27.9, 36.1) | (255,200,160) | 8 | 0.8 | Region:SFX:ffx=800023:【GD_SFX:松明の炎A】_ID800023_1005 | 0.41 |
| キッチンエリア_かまどの光A000 | spot | (117.2, -31.0, 32.2) | (255,175,120) | 24 | 2 | Region:SFX:ffx=800061:【GD_SFX:薪の炎】_ID800061_1000 | 0.54 |
| ランドマーク聖堂２階のろうそくA004 | point | (17.5, 35.5, 221.5) | (255,200,160) | 5 | 10 | Region:SFX:ffx=837012:【GD_SFX:ろうそくA】_ID837012_1007 | 0.57 |
| エレベーター建物内部_ランタンA000 | point | (50.6, -5.5, 131.6) | (246,194,159) | 2 | 4 | Region:SFX:ffx=837012:【GD_SFX:ろうそくA】_ID837012_1044 | 0.61 |
| 聖堂前広場の小さい照明A017 | point | (18.3, 13.0, 169.5) | (246,194,159) | 2 | 1.2 | Region:SFX:ffx=837012:【GD_SFX:ろうそくA】_ID837012_1115 | 0.70 |
| 聖堂前広場の小さい照明A010 | point | (35.2, 13.0, 175.5) | (246,194,159) | 2 | 1.2 | Region:SFX:ffx=837012:【GD_SFX:ろうそくA】_ID837012_1095 | 0.76 |
| 地下牢接続の照明ブリンB002 | point | (151.4, -39.0, 16.8) | (255,210,140) | 3 | 0 | Region:SFX:ffx=800023:【GD_SFX:松明の炎A】_ID800023_1015 | 0.85 |
| 厨房の照明A000 | point | (107.5, -28.4, 36.6) | (255,200,160) | 5 | 0.4 | Region:SFX:ffx=800023:【GD_SFX:松明の炎A】_ID800023_1004 | 1.01 |
| 大橋の照明範囲A001 | point | (24.4, -18.0, -29.1) | (220,191,172) | 9 | 1 | o372010:o372010_1004 | 1.12 |
| 地下牢接続の照明A008 | point | (194.9, -57.3, 12.8) | (255,210,140) | 7 | 1.5 | Region:SFX:ffx=837012:【GD_SFX:ろうそくA】_ID837012_1166 | 1.37 |
| ランドマーク聖堂のろうそくB038 | point | (18.7, 22.1, 231.5) | (255,200,160) | 8.5 | 3 | Region:SFX:ffx=837012:【GD_SFX:ろうそくA】_ID837012_1038 | 1.69 |
| 屋敷路地の照明A004 | point | (145.0, -25.9, 109.3) | (246,194,159) | 6.5 | 5.5 | o372010:o372010_1021 | 1.90 |
| 照明A029 | point | (157.8, -16.2, 69.0) | (255,200,160) | 5 | 2 | Region:SFX:ffx=837012:【GD_SFX:ろうそくA】_ID837012_1063 | 2.20 |
| 地下牢接続の照明ブリンA013 | point | (182.8, -54.2, 43.0) | (255,210,140) | 5 | 0 | Region:SFX:ffx=837012:【GD_SFX:ろうそくA】_ID837012_1174 | 2.33 |

*at-emitter B (effect-bearing object, kind unnamed)* (2 lights):

| name | type | position | diffuse RGB | radius m | brightness | nearest (kind:model:name) | distance m |
|---|---|---|---|---|---|---|---|
| 聖堂繋ぎの建物の環境光G000 | point | (88.2, 4.4, 163.3) | (162,215,230) | 4 | 0.8 | o373023:o373023_1011 | 2.16 |
| 配膳室の照明A000 | point | (127.7, -27.2, 42.5) | (255,200,160) | 10 | 1.2 | o373019:o373019_1000 | 2.94 |

*near light-shaft/stained-glass effect* (0 lights):

| name | type | position | diffuse RGB | radius m | brightness | nearest (kind:model:name) | distance m |
|---|---|---|---|---|---|---|---|

*at other object (no known light effect)* (9 lights):

| name | type | position | diffuse RGB | radius m | brightness | nearest (kind:model:name) | distance m |
|---|---|---|---|---|---|---|---|
| 聖堂前広場の照明範囲A002 | point | (27.6, 8.6, 169.3) | (246,194,159) | 8 | 2.5 | o000250:o000250_0014 | 1.24 |
| 地下牢接続の環境A000 | point | (130.2, -33.7, -6.0) | (150,204,255) | 3 | 0.2 | o000401:o000401_0004 | 1.32 |
| ガーデン地下の環境_マクダネル | point | (-82.8, -4.4, 283.4) | (93,109,128) | 10 | 0.8 | o371004:o371004_3000 | 2.28 |
| ガーデン地下の環境_マクダネルB | point | (-82.1, -3.5, 285.8) | (93,109,128) | 4 | 1 | o371004:o371004_3000 | 2.47 |
| カーテン外からの光バウンスA002 | spot | (-348.9, 142.3, 157.9) | (140,247,255) | 12 | 0.4 | o000499:o000499_0007 | 2.69 |
| 篝火教会_塔の環境A001 | point | (-79.3, 44.6, 100.8) | (194,219,230) | 5.5 | 0.8 | o000250:o000250_0082 | 2.70 |
| 屋敷出口照り返しB000 | point | (142.2, -25.2, 94.6) | (255,227,206) | 5 | 0.3 | o374000:o374000_4028 | 2.72 |
| 照明A環境016 | point | (147.0, -16.2, 90.2) | (209,199,179) | 5.5 | 0.28 | o000200:o000200_0002 | 2.85 |
| 屋敷の環境ライトB011 | point | (146.8, -13.7, 81.2) | (150,150,150) | 10 | 1.5 | o372002:o372002_1005 | 2.88 |

*free-floating* (299 lights):

| name | type | position | diffuse RGB | radius m | brightness | nearest (kind:model:name) | distance m |
|---|---|---|---|---|---|---|---|
| Spot汎用_窓からの光C004 | spot | (51.8, -6.8, 114.9) | (168,227,234) | 16 | 3.6 | o372010:o372010_1011 | 3.02 |
| 照明A環境018 | point | (141.3, -16.2, 37.9) | (209,199,179) | 5.5 | 0.28 | Region:SFX:ffx=837012:【GD_SFX:ろうそくA】_ID837012_1068 | 3.68 |
| 聖堂前広場の環境光B001 | point | (32.7, 11.5, 170.5) | (96,188,215) | 8 | 0.5 | Region:SFX:ffx=837012:【GD_SFX:ろうそくA】_ID837012_1114 | 4.27 |
| 厨房の照明拡散D000 | point | (110.4, -28.5, 32.2) | (170,190,180) | 7.8 | 6.4 | o373003:o373003_1000 | 4.58 |
| 照明A環境007 | point | (135.8, -16.2, 56.4) | (209,199,179) | 5.5 | 0.28 | Region:SFX:ffx=837012:【GD_SFX:ろうそくA】_ID837012_1055 | 5.08 |
| Spot月明かり_屋敷の出口A001 | spot | (147.3, -21.8, 109.5) | (195,243,255) | 30 | 8 | o372010:o372010_1021 | 5.74 |
| Spot月明かり_接続建物の入り口A | spot | (127.2, -29.3, -8.1) | (150,204,255) | 42 | 0.35 | o000401:o000401_0004 | 6.17 |
| 地下牢接続の照明バウンスD001 | point | (179.2, -49.8, 26.1) | (224,210,180) | 8 | 0.18 | Region:SFX:ffx=837012:【GD_SFX:ろうそくA】_ID837012_1162 | 7.43 |
| 第２広場の環境B000 | point | (117.8, -1.1, 56.3) | (216,210,204) | 5 | 3 | o000250:o000250_0010 | 8.11 |
| ガーデン地下の環境A001 | point | (-102.1, -2.0, 230.4) | (93,109,128) | 11.5 | 1.2 | Region:SFX:ffx=837012:【GD_SFX:ろうそくA】_ID837012_1129 | 9.16 |
| 窓付近B005 | point | (-280.9, 124.4, 140.8) | (181,228,255) | 6 | 1.5 | o379030:o379030_3000 | 10.16 |
| アノール本棟屋内A002 | point | (-211.0, 119.1, 157.4) | (255,255,255) | 7.5 | 2 | o000250:o000250_0069 | 10.94 |
| グウィネヴィア側面A000 | point | (-339.8, 145.3, 162.2) | (140,247,255) | 4 | 0.6 | o000499:o000499_0007 | 12.85 |
| 暗がりの環境光A002 | point | (-146.5, 34.5, 235.1) | (180,230,255) | 10 | 3 | Region:SFX:ffx=837010:【GD_SFX:橋の灯篭】_ID837010_1022 | 14.41 |
| 地下水路の照明環境ライト用_商人B001 | point | (56.1, -36.2, 7.1) | (186,253,255) | 7 | 0.9 | o000250:o000250_0038 | 18.43 |

Emitter side: for each emitter set, how many instances have a bank light within 1.5 m and 3 m (bank 0000, point and spot lights). This answers « does the bank already light this emitter? »:

| tier | emitter set | instances | with a light <=1.5 m | with a light <=3 m |
|---|---|---|---|---|
| A | sfx ffx=837012 【GD_SFX:ろうそくA】 | 167 | 109 | 146 |
| A | sfx ffx=837011 【GD_SFX:広場の灯篭】 | 30 | 2 | 5 |
| A | sfx ffx=837010 【GD_SFX:橋の灯篭】 | 23 | 8 | 10 |
| A | sfx ffx=800023 【GD_SFX:松明の炎A】 | 19 | 18 | 19 |
| A | object o372010 | 12 | 4 | 7 |
| A | object o000100 | 9 | 0 | 0 |
| A | sfx ffx=302022 【GD_SFX:アンリの七色石】 | 2 | 0 | 0 |
| A | sfx ffx=800061 【GD_SFX:薪の炎】 | 1 | 1 | 1 |
| B | object o373017 | 28 | 0 | 0 |
| B | object o373016 | 25 | 0 | 0 |
| B | object o373010 | 15 | 0 | 0 |
| B | object o373023 | 14 | 0 | 1 |
| B | object o373025 | 4 | 0 | 0 |
| B | object o373019 | 3 | 0 | 1 |
| B | object o373020 | 3 | 0 | 1 |
| B | object o373021 | 3 | 0 | 1 |
| B | object o373022 | 3 | 0 | 0 |
| B | object o379030 | 3 | 0 | 0 |
| B | object o373024 | 1 | 0 | 1 |
| C | object o001020 | 3 | 0 | 0 |
| C | object o001050 | 3 | 0 | 0 |
| C | object o001010 | 2 | 0 | 0 |
| N | all other SFX regions (fog, snow, ash, leaves, wind: not light emitters) | 62 | 0 | 0 |

Statistical candidates (INFERRED, weak): placed object models with no known effect (tier O), at least 3 instances, of which a quarter or more have a bank light within 3 m: o001110 (3 of 7). Their names are not known; a model that attracts lights may be a lamp, a brazier, a window frame or a doorway. They are listed only so a later reader can look at them.

### 2.6 m40_00, Firelink Shrine: name-based classes only

**OBSERVED.** 44 point and spot lights in bank 0000, no emitter class (frame not resolved, 2.1). Names: dynamic(動的) 38, Spot 6. Colour: cold 25, warm 11, neutral 8. The 6 `Spot_冒頭_*` spots are « opening » spots (cold, diffuse (172-195, 220, 255), radius 5 to 12 m, cone 25 to 50 degrees); the 38 « dynamic » (動的) points are named by place: 拠点内部 ×28, スタート地点 ×6, サブルート入口 ×2, 拠点外観 ×1, ボス広場扉 ×1 (start point, sub-route entrance, base exterior, base interior, boss-plaza door). The placed emitters of m40 for later use: 5 « 蝋燭光源 » (candle light source) SFX regions at (273-285, -50, 596-615), 3 light-shaft regions, 4 bonfire objects (`o000100`), 409 `o391000` carrying FFX 839010 (section 2.2).

The two warm groups of m40 by name: 動的_スタート地点_004 (255, 225, 170), 動的_スタート地点_005 (255, 235, 210), 動的_拠点内部_030 (255, 218, 179), 動的_拠点内部_032 (255, 218, 179), 動的_拠点内部_029 (255, 218, 179), 動的_拠点内部_028 (255, 218, 179), 動的_拠点内部_022 (255, 225, 205), 動的_拠点内部_021 (255, 225, 201), 動的_拠点内部_026 (255, 218, 200), 動的_拠点内部_015 (255, 229, 192), 動的_拠点内部_011 (255, 231, 209).

The five m40 candle regions coincide, to the digit, with five of the 104 effect lights the original adds at run time (I-0068 table, radius 3, colour (1, 0.67, 0.55), strength 3.5: x 273.0896 to 285.4533, z 596.5459 to 615.1899; the regions are at (273.0896, 615.1899), (276.3011, 613.5255), (279.869, 601.1634), (276.1159, 596.5459), (285.4533, 606.5197)). OBSERVED (two files compared): these Firelink candle lights are effect lights, not bank lights.

## 3. Bank 0000 against 0001

Zones with both banks: m30_00, m30_01, m33_00, m34_01, m35_00, m39_00, m40_00 (m32_00 has no 0001 rows in the census). Lights matched by exact name; an identical name whose position or any parameter differs is « changed ». OBSERVED from the census. (m34_01's one removed and one added light are its directional light renamed `Direct001` to `Direct001_m30_00`, with the direction of the High Wall bank 0001 (rotation 0.504204;2.32143;0); m30_01 bank 0001 does the same with `Direct001_王城1と同じライト`, « same light as royal castle 1 ».)

| zone | lights 0000 | lights 0001 | identical | changed | removed in 0001 | added in 0001 |
|---|---|---|---|---|---|---|
| m30_00 | 185 | 133 | 130 | 2 | 53 | 1 |
| m30_01 | 116 | 92 | 71 | 0 | 45 | 21 |
| m33_00 | 64 | 60 | 59 | 1 | 4 | 0 |
| m34_01 | 82 | 82 | 81 | 0 | 1 | 1 |
| m35_00 | 81 | 79 | 72 | 0 | 9 | 7 |
| m39_00 | 264 | 39 | 0 | 0 | 264 | 39 |
| m40_00 | 45 | 41 | 38 | 1 | 6 | 2 |

- **m30_00**: removed in 0001: Spot_最初の城壁内_001, 動的_廊下_003, 広場中央の像スポット, Omni_王城1ショトカガーデン入り口_004, Omni_王城1ショトカガーデン_001, Omni_王城1ショトカガーデン補助_003, 動的_竜ボス部屋_004, Omni_王城1ショトカガーデン2_001, Omni_王城1ショトカガーデン_005, Spot_冒頭部屋祭壇用_001, 動的_王城繋ぎ_002, Spot_廊下_001, 動的_フルアーマー_002, Spot_霊廟森の円形土台_002 ... (+39 more names). Added in 0001: Direct001. Changed: Omni227: pos 0.1 m, colour (147, 211, 255)->(170, 217, 255), brightness 1.3->1, Omni228: pos 0.5 m, colour (158, 199, 255)->(145, 190, 255), brightness 1.5->1.
- **m30_01**: removed in 0001: Spot_王城2繋ぎ広間下層窓用スポット_001, Spot_太陽の祭壇_001, Omni_王城2繋ぎ広間窓補助_002, Spot_王城2繋ぎ広間下層窓用スポット_002, Omni_王城2繋ぎ王城2繋ぎ付近_001, Omni_王城2繋ぎ広間窓補助_017, Omni_王城2繋ぎ広間窓補助_008, 動的_太陽の祭壇_002, Spot_王城2繋ぎ窓用スポット_006, Omni_王城2繋ぎ広間窓補助_006, Direct001, Omni_王城2繋ぎ広間窓補助_013, Spot_王城2繋ぎ窓用スポット_002, Spot_右兵舎窓廊下_005 ... (+31 more names). Added in 0001: Spot蝕_王城2繋ぎ窓用スポット_010, Spot蝕_王城2繋ぎ窓用スポット_008, Spot蝕_王城2繋ぎ窓用2スポット_004, Spot_王城2繋ぎ広間下層窓用スポット_005, Omni_王城2繋ぎ広間窓補助_023, Spot蝕_太陽の祭壇_002, Spot蝕_王城2繋ぎ窓用スポット_012, Spot蝕_王城2繋ぎ窓用スポット_011, Spot蝕_王城2繋ぎ窓用2スポット_006, Spot_王城2繋ぎ広間下層窓用スポット_006, Direct001_王城1と同じライト, Omni_王城2繋ぎ広間窓補助_024, 動的蝕_太陽の祭壇_003, 動的蝕_王城2繋ぎ広間全体照らす_003 ... (+7 more names). Changed: none.
- **m33_00**: removed in 0001: Spot_深淵ボス部屋_004, Spot_深淵ボス部屋_006, 動的_森後半接続_007, Spot_深淵ボス部屋_005. Added in 0001: none. Changed: 動的_森後半接続_001: colour (154, 207, 255)->(184, 222, 255), brightness 1.4->2.
- **m34_01**: removed in 0001: Direct001. Added in 0001: Direct001_m30_00. Changed: none.
- **m35_00**: removed in 0001: Spot_冒頭階段_003, Spot_冒頭階段_006, Spot_冒頭階段_004, Spot_冒頭階段_005, 動的_祭壇外壁_003, 動的_祭壇外壁_002, Spot_冒頭階段_001, Spot_冒頭階段_002, KeyLight. Added in 0001: Spot_冒頭階段_011, Spot_冒頭階段_008, Spot_冒頭階段_013, Spot_冒頭階段_009, Spot_冒頭階段_012, Spot_冒頭階段_010, 動的_冒頭道_004. Changed: none.
- **m39_00**: removed in 0001: 動的_塔裏道_003 ×2, 動的_中央通路_001 ×2, 動的_通路水面_001, ボス部屋前_床ウソライト_1000, TitanJailSpot_1003, 動的_塔下部_001, JailPathFillLight_1016, 動的_通路壁_009, 動的_裏建物_003, ボス部屋前_天井ウソライト_1002, JailPathFillLight_1026, 動的_ゴーレム部屋_011, 動的_裏回廊_004, 動的_1-2接続通路_002 ... (+248 more names). Added in 0001: ADD:1225_FakeLight_BottomRight_1000 ×2, ADD:1225_FakeLight_Left_1000, ADD:0204_FakeLight_Elevator_1003, ADD:1225_FakeLight_TopLeft_1003, ***            FakeLight_BonfireRoom_1000, ADD:1225_FakeLight_MiddleLeft_1002, ADD:1225_FakeLight_BottomRight_1002, ADD:1225_FakeLight_PathToTitanCage_1002, DY_ZoneD_GreenLight_1000, DY_ZoneD_FakeLight_1008, ADD:1225_FakeLight_PathToTitanCage_1000, ADD:1225_FakeLight_TopLeft_1000, ADD:1225_FakeLight_MiddleRight_1001, KeyLight_ID1 ... (+24 more names). Changed: none.
- **m40_00**: removed in 0001: Spot_冒頭_004, 動的_スタート地点_004, 動的_拠点内部_034, Spot_冒頭_001, 動的_拠点内部_033, Direct001. Added in 0001: 動的闇_スタート地点_009, 動的闇_スタート地点_008. Changed: 動的_拠点内部_016: pos 2.5 m, radius 5.2->3.7.

**Reading (INFERRED).** m30_00 (185 to 133) and m30_01 (116 to 92) drop most of their lights: a second lighting state with fewer lights (the removed lights are in the TSV). m33, m34_01 (82 and 82), m35 and m40 keep nearly all; m40 swaps two « opening » spots for two « 動的闇 » (dynamic dark) points. m39 0001 (39 lights) is a small second bank. Which bank the engine loads when remains open (RESULT I-0110 §9).

## 4. The directional « KeyLight » rows (type 2)

**OBSERVED**, all 23 (the engine never builds a type-2 record, RESULT I-0110 §5.1). `Rotation` is the stored three-value rotation word (SoulsFormats `Rotation`, radians as stored, no conversion); `Width`, `Radius` and `ConeAngle` are 0 for all. Brightness is the diffuse brightness.

| bank | name | direction as stored (rotation) | diffuse RGB | brightness | specular brightness | shadow flag | position |
|---|---|---|---|---|---|---|---|
| m30_00_0000 | Direct002 | 0.490432;-1.04881;-0 | (255,240,221) | 1 | 5 | 1 | (363.2, 223.7, -208.9) |
| m30_00_0001 | Direct001 | 0.504204;2.32143;0 | (255,240,221) | 1 | 5 | 1 | (-296.5, 223.7, 276.6) |
| m30_01_0000 | Direct001 | 0.457639;0.953356;0 | (255,240,221) | 1 | 5 | 1 | (-296.5, 158.0, -8.4) |
| m30_01_0001 | Direct001_王城1と同じライト | 0.504204;2.32143;0 | (255,240,221) | 1 | 5 | 1 | (-296.5, 223.7, 276.6) |
| m31_00_0000 | KeyLightNEW | 0.456403;-2.45324;0 | (190,211,236) | 0.25 | 0.25 | 1 | (214.5, 165.8, 260.8) |
| m32_00_0000 | Direct001 | 0.599255;-1.80841;0 | (255,254,247) | 0.25 | 5 | 1 | (521.5, 422.8, 401.7) |
| m33_00_0000 | KeyLight | 0.473212;-2.62712;0 | (177,186,203) | 1 | 2 | 1 | (-50.0, 107.2, 15.5) |
| m33_00_0001 | KeyLight | 0.473212;-2.62712;0 | (177,186,203) | 1 | 2 | 1 | (-50.0, 107.2, 15.5) |
| m34_01_0000 | Direct001 | 0.457639;0.953355;0 | (255,240,221) | 1 | 5 | 1 | (-296.5, 158.0, -8.4) |
| m34_01_0001 | Direct001_m30_00 | 0.504204;2.32143;0 | (255,240,221) | 1 | 5 | 1 | (-296.5, 223.7, 276.6) |
| m35_00_0000 | KeyLight | 0.872545;0.0529048;0 | (240,220,210) | 2 | 2 | 1 | (-0.0, 0.8, -0.6) |
| m37_00_0000 | 平行光源_0000 | 0.524537;-2.68818;0 | (160,210,230) | 1 | 5 | 1 | (268.0, 354.0, 550.0) |
| m39_00_0000 | KeyLight_ID0 | 0.360398;-0.716459;-0 | (210,221,243) | 0.25 | 0.25 | 1 | (321.6, 39.7, -189.5) |
| m39_00_0001 | KeyLight_ID1 | 0.680647;-1.92402;0 | (255,201,107) | 50 | 5 | 1 | (333.3, 287.6, 122.9) |
| m40_00_0000 | Direct001 | 0.373398;-1.70354;0 | (255,255,255) | 1 | 1 | 1 | (326.7, 62.6, 581.3) |
| m41_00_0000 | Direct001 | 0.478922;-2.67326;0 | (255,255,255) | 1 | 1 | 1 | (-22.0, 257.9, 289.5) |
| m45_00_0000 | KeyLight_0000_new | 0.785257;1.91974;0 | (176,216,255) | 1 | 2.2 | 1 | (-47.0, 50.0, 17.1) |
| m46_00_0000 | Direct001_00 | 0.646262;1.02947;0 | (255,240,221) | 1 | 2 | 1 | (-48.4, 42.6, -29.1) |
| m50_00_0000 | Direct001_00 | 0.729616;-0.766664;-0 | (218,228,253) | 0.3 | 2 | 1 | (277.4, 663.4, -141.1) |
| m51_00_0000 | Direct001_00 | 0.836856;-1.04667;-0 | (240,232,175) | 1 | 2 | 1 | (410.0, 525.0, -237.0) |
| m51_01_0000 | KeyLight_0000 | 0.384157;0.959945;0 | (255,247,221) | 1 | 1.5 | 1 | (-537.0, 265.0, -376.0) |
| m53_00_0000 | Direct001 | 0.599255;-1.80841;0 | (255,254,247) | 0.25 | 5 | 1 | (521.5, 422.8, 401.7) |
| m54_00_0000 | Direct001_00 | 0.524537;-2.68818;0 | (160,210,230) | 1 | 2 | 1 | (268.0, 354.0, 550.0) |

Brightness over the 23: min 0.25, max 50; all 23 have the shadow flag `Unk27` true. (This is why RESULT I-0110 counts 23 of the 44 shadowed lights as the directional ones.)

## 5. Effect lights (FFX)

**OBSERVED** from `EFFECT-LIGHTS.md` and the I-0103 census: torch, bonfire and candle flames are not bank lights. A light is made by an FXR **action**: type 609 (0x261, « E06 point lights », host `GXFfxDrawEntityHostFrpgPointLight`), 11000 (0x2AF8, spot, host `...SpotLight`, the map-light path) and 11002 (0x2AFA, line light). I did not parse any FXR; everything below is read from tables that already exist.

Counts (`evidence/I-0103/census/actions.tsv`, whole game): action **609: 2,457 occurrences in 1,269 distinct FXR files, in 17 bundles**; action **11000: 7 occurrences in 4 FXR files, in 3 bundles** (commoneffects 1, m33 2, m45 4); **11002: no row**, so it does not occur in the census. (A single FXR can carry several light actions; the census does not tabulate per FXR which colour or radius.)

| bundle | 609 occurrences | 609 FXRs | 11000 occurrences | 11000 FXRs |
|---|---|---|---|---|
| commoneffects | 490 | 462 | 1 | 1 |
| dlc1 | 50 | 45 |  |  |
| dlc2 | 77 | 68 |  |  |
| m30 | 160 | 79 |  |  |
| m31 | 64 | 29 |  |  |
| m32 | 96 | 44 |  |  |
| m33 | 140 | 68 | 2 | 1 |
| m34 | 164 | 81 |  |  |
| m35 | 96 | 46 |  |  |
| m37 | 170 | 82 |  |  |
| m38 | 162 | 78 |  |  |
| m39 | 56 | 24 |  |  |
| m40 | 40 | 19 |  |  |
| m41 | 104 | 49 |  |  |
| m45 | 128 | 62 | 4 | 2 |
| m50 | 170 | 77 |  |  |
| m51 | 290 | 135 |  |  |

**Placed effects per zone** (`zone-effect-sources.tsv`; ObjectParam/ModelSfxParam, OBSERVED): m30_00: torches 133 (800023), firewood flame 4, 800030/800043 ×36 (on `o304783`, `o304857`), 800050 ×21, 800052 ×33, 800060 ×14, 800062 ×6, 839010 ×105, 319 ObjectParam-bound object effects (830060-830076, 130000) and 238 SFX regions, which at m30 are fog (830050-830056, 830200-830202, 610020/21: 200 regions), leaves, ash, 10 light shafts, 1 boss-room stained glass and 3 forest flames; m37_00: torches 12 (`o372010`), 800044 ×44 (`o373023`, `o373024`), 304 SFX regions among them 167 candles, 53 lanterns, 19 torch flames, 1 firewood flame; m40_00: 839010 ×409 (`o391000`), 800022 ×12, 18 SFX regions (7 interior fog, 5 candle light sources, 3 light shafts, 3 far fog).

**The one table of typical effect-light values** is the m40 witness of I-0068 (`effect-light-families.tsv`, Reference1152; 104 effect point lights present in the original and absent natively at the time). Reproduced:

| count | kind | radius m | colour (0..1) | strength / specular | fade | scale | position range |
|---|---|---|---|---|---|---|---|
| 85 | point | 2 | 0.4706/0.3137/0.1961 | 1 / 6 | 80 | 1..1 | x 120.5263..286.4593, z 564.4231..621.9603 |
| 8 | point | 5 | 1/0.6275/0.3529 | 1 / 3 | 0 | 0.8794..0.992 | x 240.24..291.5776, z 595.6727..614.6392 |
| 5 | point | 3 | 1/0.6667/0.549 | 3.5 / 3.5 | 0 | 1..1 | x 273.0896..285.4533, z 596.5459..615.1899 |
| 2 | point | 6 | 0.9961/0.5098/0.3922 | 0.6 / 2 | 0 | 0.8769..0.9939 | x 104.8553..142.6328, z 507.0784..594.666 |
| 1 | point | 0.5 | 1/0.6314/0.3686 | 3.2 / 3.2 | 0 | 1..1 | x 271.5759..271.5759, z 607.4909..607.4909 |
| 1 | point | 2.5 | 0.8824/1/0.9608 | 0.3 / 0.1 | 0 | 1..1 | x 271.7524..271.7524, z 606.9902..606.9902 |
| 1 | point | 3.2 | 1/0.6275/0.3529 | 0.6 / 3 | 0 | 0.9262..0.9262 | x 302.9663..302.9663, z 600.7562..600.7562 |
| 1 | point | 6 | 0.9961/0.5098/0.3922 | 1.2 / 5 | 0 | 0.5865..0.5865 | x 271.3148..271.3148, z 605.6465..605.6465 |

All 104 are point lights (type 609); no spot or line effect light appears in the scene. INFERRED sources (I-0068): 85 small warm candle-like lights, 8+5+3 fire lights of radius 3 to 6, one held-flame pair at the player. The warm family above is therefore an effect, not a bank, family: in the m40 bank 11 of the 44 point and spot lights are warm (section 2.6, colours around (255,218,179), mostly named 拠点内部 « base interior »).

**Not tabulated** (so not stated): the colours and radii of the effect lights outside the m40 witness; the FXR files that hold a light action; the link from an FFX id to its light. A study of those needs the FXR files read (not done, by the brief).

## 6. Checks

### 6.1 Five lights re-read by hand

Each light was found by `grep` on the raw `btl-census.tsv` line and its fields read by column name with a plain tab split (no `core.py` code), then compared with its row in `lights-classified.tsv`.

| bank #index | name (raw) | position (raw) | diffuse RGBA (raw) | brightness | radius | Unk18 | Unk27 | colour class (mine) | radius bucket | brightness bucket | raw = table |
|---|---|---|---|---|---|---|---|---|---|---|---|
| m30_00_0000 #98 | Omni254 | -41.063, -3.7336, 40.355 | 150,212,255,255 | 1 | 6 | 0 | False | cold | 6-8 | 1-2 | OK |
| m37_00_0000 #221 | 厨房の照明ブリンA011 | 119.011, -26.3552, 47.2296 | 255,200,160,255 | 0.8 | 8 | 0 | False | warm | 8-12 | 0.5-1 | OK |
| m40_00_0001 #2 | 動的闇_スタート地点_008 | 7.09, -65.669, 541.429 | 202,215,255,255 | 0.6 | 7 | 0 | False | cold | 6-8 | 0.5-1 | OK |
| m39_00_0001 #38 | KeyLight_ID1 | 333.288, 287.632, 122.88 | 255,201,107,255 | 50 | 0 | 2 | True | warm | n/a | >=10 | OK |
| m51_00_0000 #0 | spot_断罪の塔出口A | 84.2545, -91.028, 210.402 | 165,205,240,255 | 0.1 | 28 | 1 | False | cold | >=20 | <0.5 | OK |

Colour class by hand, d = R - B (warm >= +40, cold <= -40, else neutral): Omni254: -105 -> cold; 厨房の照明ブリンA011: 95 -> warm; 動的闇_スタート地点_008: -53 -> cold; KeyLight_ID1: 148 -> warm; spot_断罪の塔出口A: -75 -> cold. The fourth, `KeyLight_ID1` of m39 bank 0001, has diffuse brightness 50, type 2 (directional) and no radius, as listed in section 4; the fifth is the first row of the m51_00 bank.

### 6.2 Two nearest-object distances recomputed by hand

**Light 1**, m30_00 bank 0000 `Omni254` (an « at-emitter A at 3 m » example). m30_00 uses the identity frame. Raw BTL position (-41.063, -3.7336, 40.355); raw MSB object `o004660_8022` (model o004660, the torch) at (-40.81233, -3.97905, 42.06161), read from `msbpos-m30_00.tsv`. Differences: dx = 0.2507, dy = -0.2454, dz = 1.7066; squares 0.0628 + 0.0602 + 2.9125 = 3.0356; square root = 1.742 m. The table says 1.74 m. Matches.

**Light 2**, m37_00 bank 0000 `厨房の照明ブリンA011` (« kitchen lighting », at an emitter at 1.5 m). Raw BTL position (119.011, -26.3552, 47.2296). Frame: cos(179.25 deg) = -0.99991, sin(179.25 deg) = 0.01309. x' = -0.99991 x 119.011 + 0.01309 x 47.2296 + 423.0 = 304.617; y' = -26.3552 - 242.0 = -268.355; z' = -0.01309 x 119.011 + -0.99991 x 47.2296 - 925.5 = -974.283. Raw MSB SFX region `【GD_SFX:松明の炎A】_ID800023_1011` at (304.7972, -268.352, -974.283). Differences: dx = 0.180, dy = 0.003, dz = 0.000; sum of squares 0.0323; square root = 0.180 m. The table says 0.18 m. Matches.

### 6.3 Consistency checks of the pipeline

- Rows in `lights-classified.tsv`: 2431 (2,431 expected). Per-bank counts of the census reproduced in table 1.3 (m30_00 185/133, m37_00 524, m40_00 45/41, m39_00 264/39).
- Class counts add up: m30_00 bank 0000, 3 m classes 184 = 184 point/spot lights; m37_00 523 = 523.
- Type counts equal I-0110 §5.1 (2,174 point, 234 spot, 23 directional; 44 shadow-flagged, 23 of them directional).
- Not independent: the m37 frame was fitted with the same MSB parts that the class uses; the m30 frame was not fitted.

## 7. What could not be done

- **m40_00 against the MSB**: the BTL-to-MSB frame was not recovered (section 2.1), so the Firelink lights have no emitter class. A fix needs the area transform of m40_00 (the 3x4 at `[[+0xE8]+0x80]+0x5C0` of the map object, NATIVE-LIGHTING-MAP) or one native witness of a bank light's world position; neither is in the files read.
- **Other zones against the MSB**: only three recovered MSBs exist locally (m30_00, m40_00, m37_00). Every other zone has name-based families only. The reader and `Get-MsbPositions.ps1` work on any recovered `map_mapstudio_mXX_XX_00_00.msb.dcx`, but those files must first be recovered with the I-0068 recovery tool (game files; not done, by the brief).
- **Window openings** cannot be established from the MSB tables (no window flag, no model class): only the m30 light-shaft and stained-glass SFX regions were used, plus the name tag.
- **What an effect id emits**: FFX ids 800030/43/44/50/52/60/62, 830060-830076 and 839010 are bound to objects but no table labels them; whether they carry light actions was not established (no per-FXR table of light actions exists). FFX contents were not parsed, by the brief.
- **Effect-light colours and radii** outside the m40 witness (section 5) are not tabulated anywhere I could read.

