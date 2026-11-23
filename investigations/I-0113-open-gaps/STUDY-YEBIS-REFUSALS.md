# STUDY-YEBIS-REFUSALS: where the native renderer refuses its tone map, grading LUT, glare or DoF (whole game, by GPARAM)

2026-11-06, Sonnet 5.5, read-only census for the open-gaps pilot (STUDY-NOT-YET-NATIVE rows 12 and 13). Nothing was launched, no
Ghidra, nothing in the code was changed. Tree `2ef560d9a`. Tags: **OBSERVED** (read in code or game data, location given),
**INFERRED** (basis named), **TO CHECK**. Made by the study thread's Sonnet worker; non-canonical until the pilot re-reads
what a change rests on. Re-checked by the study thread: the tone map's refusal order (13b:1337-1339) and « hue or value » being
`ColorV != 1` when the colour data is read (`yebis_tonemap.cpp:55`; hue 0 is also required only when it is not, `:59`).
For E (D-0250: the post-processing stays raster, so these stay visible with the rays): the cutscenes are where it shows
(74 of 146 s-file records refused: the vignette's field-of-view dependence 58, ColorV 39), then m41's slot 9 and m50's grading.

Sources. Code: `native/frame/game_bootstrap/13b_prepare_model_frame.inc` ("13b"), `12_scene_collect.inc` ("12"),
`native/world/yebis_blocks.cpp`, `yebis_tonemap.cpp`, `yebis_glare.cpp`, `yebis_dof.cpp`, `light_producers.cpp` (`Banks::GroupEntries`).
Data: the 161 GPARAMs of `C:/Users/User/.codex/I0063-cache/files/param_drawparam_*.gparam.dcx` read with a small Python reader
(DCX zlib at 0x4C, layout of `investigations/I-0067-static-menus-subsystems-map/Get-GparamValues.ps1`; checked against
`evidence/I-0113/water-interaction/gparam-m30_00_0000.tsv`: Tone Map rows identical) plus the 7 DLC2 files the dictionary does not
name (`m47_00_0800/0900`, `m50_00_0100-0300`, `m51_00_0100/0200`) read from `evidence/I-0066/queries/gparam-all/*.tsv`: 168 files
(107 `mAA_BB_CCCC`, 49 `sAA_BB_CCCC`, 12 global: default, eclipse, empty, mapbase, remobase, mapoverride_0000..0003, m_template,
ml_template, s_template). Variant selection: `evidence/I-0066/queries/ae/msb-gparam-slots.tsv` (W10-NATIVE-PASSES section 11, K-0448).
Journal checks: `evidence/I-0066/artifacts/i66-native-{1697,1698,1700,1701,1707,1711,1738,1740,1741}-*/startup.jsonl`, rows
`native_yebis_tonemap`, `native_yebis_grading`, `native_yebis_glare`, `native_yebis_dof` (the only journals read; the later runs under
`evidence/I-0125` (16 GB) were not scanned). The reader and tables were produced by throwaway scripts in the session scratchpad
(not in the repository).

Note (I-0113 tranche 20, the pilot): the tone map's refusals « hue or value » (ColorV), « exposure or gamma » and « vignette fov »
are lifted, served from the executable's setters read statically (RESULT « Tranche 20 »); the counts below are the census's
state before it.

## 0. Answers first

1. **The tone map is refused on almost no retail world record.** Of the 51 world-class (`mAA_BB_CCCC`) Tone Map records, 40 are
   accepted. The refusals: `m30_00_0100/0110` (auto exposure; no collision names file 1 at m30_00, K-0448), the five `m36_00` files
   (auto exposure, one also ColorV 1.2; m36_00 has no Ceremony row, not a retail world map per K-0448, INFERRED), `m41_00_0900`
   (Exposure 0, "exposure or gamma"; selectable through slot 9, **1 of the 16 m41_00 collisions**), `m47_00_0900` (same; not selectable
   per the census) and the test files `m21_00_0000`, `m22_00_0000` (34-param Tone Map layout: whole feed refused). Reachable in the
   retail game: **`m41_00_0900` only** (INFERRED reachable: its collision names slot 9).
2. **Most tone refusals are in the cutscene/event files `sAA_BB_CCCC`**: of 146 Tone Map records in 49 s-files, **74 records in 17
   files** are refused: field-of-view vignette (58 records, 13 files), ColorV != 1 (39, 12), Exposure or Gamma 0 (4, 4), ToneType 3 (2,
   `s40_00_0040`). Which of those files the engine applies, and when, is **TO CHECK** (not established here); the native shows
   the cutscene frames through the sRGB composition only if it does apply them.
3. **"hue or value" is ColorV only**, not hue: `matrix_supported` tests `color_v==1.0f` when the executable colour data is read
   (`yebis_tonemap.cpp:55`) and `color_h==0 && color_v==1` only in the no-colour-data branch (`:59`). A hue rotation (ColorH != 0) is
   served: `m41_00_0000`, `m47_00_0000`, `m50_00_0000` have ColorH 1 and the native tone map is on at m50 (journals 1698/1741
   `hsv:[1,0.9,1]`, `on:true`). Row 12's "hue rotation" should read "value scale".
4. **The grading's "non-map record" refusal cannot fire on the shipped data** (INFERRED from the code): a record reaches
   `source!="area"` only from `mapbase` or `default`; `mapbase` has no ColorGrading values and `default` has Enable 0, tested first
   (`refused="off"`). A cutscene s-file record taken through the bank's layers gets source `"area"` (`light_producers.cpp`, `layered()`:
   `map_named?name:"area"`) and is **not** refused. So row 12's "cutscenes" clause is not what the code does. What stops the grading in
   the retail world is **"texture absent"** (the map ships no strip for the record's LUT id; the original draws nothing either;
   journals 1698, 1700, 1701, 1740, 1741) and `Enable 0` (no draw, as the original).
5. **Glare shape != 1 refuses the glare on 29 Bloom records**: `default` ids 0 and 12 (shape 0: the frames before an area loads; the
   journals log `glare shape` at start-up in 1711/1738/1741), `m32_00_0100` (shape 3, levels 10, selectable slot 1 at m32_00, 5 of 59
   collisions) and 27 s-file records. **DoF**: all zone DoF comes from `mapbase` (edge quality 2, on at the witnesses); the few
   zone-class files with their own Dof group differ in DofEdgeQuarity (section 3 notes): refused "holder flags" if the INFERRED mapping
   holds. **Auto focus** (Dof entry 25 = block word 24) is 0 in all 239 shipped Dof records: never refuses.

## 1. The exact refusal conditions

### 1.1 Gate (OBSERVED)

- `CollectNativeYebisInputs` (12:13-124) reads for each group the key pair A/B (keys at context+0x3FD8+8i and +0x4040+8i, weight
  +0x40A8+4i) and builds the record with `Banks::GroupEntries(area,id,group,...)` (`light_producers.cpp:1496`) in this order: the area's
  live kind-7 layers, latest first (the first layer whose group holds the id); the area's bound file; the area's own `mAA_BB_<area%10000>`
  file (an area key ends in 0000, so `mAA_BB_0000`); `mapbase`; `default` id 0. **A variant file without the group therefore inherits
  the base `mAA_BB_0000` record** (rows "absent: inherits base" below).
- `yebis.complete=setting_ok && tone_ok && bloom_ok` (12:105). `tone_ok` needs both records and `ToneBlock` (35 entries at least,
  `yebis_blocks.cpp:20`); `bloom_ok` 36 entries and 42 words (`BloomBlock`); `setting_ok` 18 type-3 entries plus `SceneLuminanceScale`
  (`SettingBlock`). Failing: `refused="Setting Info block" / "Tone Map block (layout)" / "Bloom block"` (12:124) and the tone pass is
  skipped (13b:1331 needs `scene->yebis->complete`). In the data only the four 34-param Tone Map files `m21_00_0000`, `m22_00_0000`,
  `m_template`, `s_template` fail it (all non-retail; OBSERVED, my census).

### 1.2 Tone map (13b:1318-1350, row `native_yebis_tonemap`)

Tone words are the blended Tone Map entries by position: 0 Exposure, 1 Gamma, 2 ToneType, 4 FilmicContrast, 5 ColorH, 6 ColorS, 7 ColorV,
9 AutoExposure, 26 Vignette Power, 27 VignetteFovDependence (13b:1318-1325 comment; positions checked against the file order in
`evidence/I-0113/water-interaction/gparam-m30_00_0000.tsv` lines 1358-1385). Refusal chain (13b:1337-1339, first match wins):

| # | refusal text | condition | GPARAM group "Tone Map", field (entry index) |
|---|---|---|---|
| 1 | `executable data` | `!TonemapImageData().ok` | none (image .rdata) |
| 2 | `auto exposure` | `scene->yebis->auto_exposure`: flag in **either** record A or B, `ToneAutoFlags` (`yebis_blocks.cpp:167`): `Positive(entry 9)` or the first entry named `Enable AutoExposure` positive (12:106-113) | `AutoExposure` (9), `Enable AutoExposure` (46) |
| 3 | `tone type` | `tone[2]!=2` | `ToneType` (2) |
| 4 | `hue or value` | `!t.matrix_supported`: ColorV != 1 (colour data read, `yebis_tonemap.cpp:55`); also ColorH != 0 only without colour data (`:59`) | `ColorV` (7), `ColorH` (5) |
| 5 | `exposure or gamma` | `!(0<exposure) \|\| !(0<gamma)` | `Exposure` (0), `Gamma` (1) |
| 6 | `vignette fov` | `vignette_power>0 && vignette_fov!=0` | `Vignette Power` (26), `VignetteFovDependence` (27) |
| 7 | `vignette data` | vignette on and `!(tan_eighth_pi>0 && width && height)` | none (image data, output extent) |
| 8 | `programmes`, `vignette programme` | programme acquisition failed; vignette variant blob empty | none |

Not refusals: Vignette Power > 0 alone (selects the vignette variant), ColorS != 1 (saturation matrix), ColorH != 0 with colour data.
Only the **blended** record is tested; this census tests each file/value id on its own. A cross-fade between two records can add a
refusal (ColorV lerped off 1, tone[2] taken from B, auto exposure from either side) or hide one: **TO CHECK** per transition.

### 1.3 Grading LUT (13b:1258-1288 comment; code 13b:1358-1383, row `native_yebis_grading`)

Needs `frame->tonemap` (any tone refusal above removes the grading too), `grading_ready` (both ColorGrading records carry the two
integer entries, `GradingBlock`, `yebis_blocks.cpp:145`). The record is A below weight 1, B from 1 (12:91-96); `grading_source` is the
bank layer's name (`GroupEntries` `source`). Chain (13b:1369-1380):

| # | refusal | condition | field |
|---|---|---|---|
| 1 | `off` | `!g[0]` | ColorGrading `Enable` |
| 2 | `no LUT id` | `int32(g[1])<0` | ColorGrading `LutSourceId` |
| 3 | (name) | source map-named (`mAA_BB_NNNN`, 11 chars): name `%.7sCGrading%04d` from the source file name | |
| 4 | `non-map record (prefix unread)` | not map-named and (`area<10000000` or `source!="area"`): source `mapbase`, `default`, area <= 1 | |
| 5 | (name) | else `m%02u_%02u_CGrading%04d` from the **key's area** digits (source `"area"`, which includes non-map-named layers such as s-files and MapOverride) | |
| 6 | `texture absent`, `texture not resident` | the TPF strip registered by entry name at admission (`native_grading_names`, `14_scene_step_and_storage.inc:549`) | availability |
| 7 | `texture not a 16x256 RGBA8 strip` | | format |
| 8 | `programmes` | | |

Strips shipped (scan `evidence/I-0063/artifacts/scan/texture-formats.tsv`, 36 `CGrading` rows, all 16x256): m30_00 {1,2,4,10}, m30_01
{1,2}, m31_00 {1,10}, m32_00 {1,2}, m33_00 {1,2}, m34_01 {1,2,4}, m35_00 {1,2}, m37_00 {1}, m39_00 {1,2,3}, m40_00 {1,2,3}, m41_00 {1,2,3};
**and, in the DLC2 archive, m50_00 {1,2,3} and m51_00 {1,2,3}** (hash-named containers `#0000000020D7A5CE` and `#000000003EB28648`).
None for m21, m22, m36, m38, m45, m46, m47, m53. K-0449 and W10-NATIVE-PASSES section 12 say "none for the DLC maps", yet the scan
lists the m50 and m51 strips, and the native logged `texture absent` for `m50_00_CGrading0001` (journals 1698, 1741). Either the
original never loads them either (no m50 reference for the grading is cited) or the native admission does not register the DLC2
hash-named TPFs: **TO CHECK** (the m51 record asks id 0, which has no strip either way).

### 1.4 Row 13: glare, DoF, auto focus, motion blur

- **Glare** (13b:1395-1421): `plan.refused="glare shape"` if `integer(0xD8)!=1`; `BuildGlare` (`yebis_glare.cpp:35-43`) refuses quality
  outside 0..7, levels outside 3..16, `tinted shape` (`shape_tint>0`; 13b:1399 sets `shape_tint=0`, so it never fires), `bloom gamma`
  (word 0x100 != 1), base size < 16. Word 0xD8 is Bloom entry 7 `GlareShape` (INFERRED; consistent with the journals: the start-up
  `glare shape` row logs levels 8, radius 0.5, luminance 3, threshold 0.5, which is `default.gparam` Bloom id 0 with shape 0).
  `GlareBloomLuminanceGamma` (Bloom entry 10: 1.4 at m37, m40, m41, m47, m50; 1.68 at `m32_00_0100`) is **not** the refusing
  `bloom gamma`: journal 1741 logs `gamma:[1.4,1]` at m50 with the glare on, so word 0x100 is another field (not mapped; **TO CHECK**;
  every witness passes it).
- **DoF** (13b:1430-1470, `yebis_dof.cpp`): refused when `!lens.branch` (f-number, near/far planes, the Enable words), when
  `flags_index=integer(0x240)` is outside 0..4 (`holder flags`), and `BuildDof` refuses `f.flags!=2` (`yebis_dof.cpp:131`) after
  `holder_flags[5]={0,0,2,6,6}`; so only index 2 is served. INFERRED: word 0x240 is Dof entry 11 `DofEdgeQuarity` (mapbase = 2, the one
  value giving flags 2; shipped values -1, 1, 2, 4). The converter writing 0x240 lives in the image and was not read: **TO CHECK**.
- **Auto focus** (12:113): `DofAutoFocus(dof)` = `Positive(dof[24])` = Dof entry 25 (the `Enable` after `DofFar DistanceThrethold`);
  logged (`native_yebis_auto`), the feed continues with the record's focus distance. It is 0 in all 239 shipped Dof records.
- **Motion blur** (13b:1520-1550, `yebis_motionblur.cpp:66-117`): refused for mode != 2 (`integer(0x1E0)`), camera-threshold flag,
  falloff, taps != 5, far depth threshold, `depth of field off`. Only 12 files carry a MotionBlur group (default, mapbase, remobase, the
  templates, `m32_00_0100`, `m36_00_0003`, `m38_00_0002`, `s30_00_0010`, `s45_00_0000/0010`); every other zone uses mapbase's. Values not
  read: **TO CHECK**.
- Light shafts: `AppendShafts` refusals (record +0x24, 8-bit surface, one-tap last pass ...) depend on LightShaft (119 files carry it)
  and the render surface; **not censused** (TO CHECK).

## 2. Witness zones (check of the reading)

| zone | file(s) | tone | grading | journal check |
|---|---|---|---|---|
| Firelink / Cemetery of Ash m40 | `m40_00_0000/0010` | accepted (exp 2.3, S 0.8, vp 0.3) | drawn `m40_00_CGrading0003` | 1707: exposure 2.3, hsv [0,0.8,1], `m40_00_CGrading0003` on, source `m40_00_0000`: match |
| High Wall m30 | `m30_00_0000/0010/0200` | accepted | drawn `m30_00_CGrading0002` | 1711: exposure 2 (= `m30_00_0010`), vp 0.34, `m30_00_CGrading0002` on, source `area`: match |
| Undead Settlement m31 | `m31_00_0000..0030` | accepted | drawn `m31_00_CGrading0001` | 1697/1738: exposure 2.2, vp 0.2, `m31_00_CGrading0001` on: match |
| Archdragon Peak m32 | `m32_00_0000/0010` | accepted (exp 2.4 / 2, vp 0) | drawn `m32_00_CGrading0002` / `0001` | no journal read; consistent with no refusal logged |
| Anor Londo m37 | `m37_00_0000/0010` | accepted (exp 1.52) | `off` (Enable 0; the original draws none) | W10-NATIVE-PASSES section 12: Native1702 word 0 = 0 |
| Painted World m45 | `m45_00_0000` | accepted (exp 2, S 0.85, vp 0.35) | `texture absent` (`m45_00_CGrading0000`, id 0, no m45 strip) | 1700/1740: `refused:"texture absent"`: match |
| m50 / m51 | `m50_00_0000` / `m51_00_0000` | accepted (m50 ColorH 1 served) | `texture absent` (m50 id 1: TO CHECK above; m51 id 0) | 1698/1741/1701: match |

No witness zone has a tone-map refusal. The grading rows at m45, m50, m51 are logged as "refused" (texture absent), which is the
original's behaviour too (no draw), not a departure. The start-up frames before an area loads use key (-1,-1) = `default`: tone
accepted (exp 1.52), grading `off`, glare refused `glare shape`.

## 3. Census

Reading the tables. Records are "value id: verdict". Map files hold one id (0); the s-files hold several. "absent: inherits base" =
the file has no values for the group, the record then comes from `mAA_BB_0000`, `mapbase` (which holds Dof, MotionBlur, Anti-Alias but
no Tone Map, Bloom or ColorGrading) or `default`. "slot role" is from `msb-gparam-slots.tsv`: `selectable (slot N)` = some collision part
of the map names the file; `NOT selectable` = no collision names it (the file can still apply through a Ceremony base id 0..30 for
`0000/0010/0020/0030`). Tone verdicts are per record, unblended. In the grading column `a/b` = `Enable`/`LutSourceId`; `drawn` means
the strip named by the file's `mAA_BB` prefix exists. For s-files the LUT name is built from the key's area, so the strip is not
decidable here (LUT ids 1, 2, 3, 10 occur; strips with those ids exist for m30, m31, m40 etc.). Dof/Bloom notes: DoF edge quality
(entry 11) per class: map files with their own Dof group: `m30_00_0200` -1, `m32_00_0100` 1, `m38_00_0002` 1, `m36_00_0003` 4
(mapbase 2); s-files: -1 in 229 of 232 records (2 in 1, 4 in 2); `default` -1. If the INFERRED mapping holds, these are the records whose
DoF is refused ("holder flags"), including `m30_00_0200` (High Wall slot 2, 2 collisions of 91) and `m32_00_0100` (5 of 59).

### A. Map files (mAA_BB_CCCC): world areas

| file | slot role (msb census) | Tone Map: id: verdict (exp, S, vignette) | ColorGrading (Enable/LutSourceId) verdict | Bloom shape/levels |
|---|---|---|---|---|
| m21_00_0000 |  | REFUSED (whole feed, "Tone Map block (layout)"): layout (34 params <35) | 0: 1/0 -> texture absent (m21_00_CGrading0000 ships no strip) | layout |
| m22_00_0000 |  | REFUSED (whole feed, "Tone Map block (layout)"): layout (34 params <35) | 0: 1/0 -> texture absent (m22_00_CGrading0000 ships no strip) | layout |
| m30_00_0000 | base slot0 | 0: accepted (exp 2.3 S 0.85 vp 0.34) | 0: 1/2 -> drawn: m30_00_CGrading0002 present | 0: shape 1 lv 8 |
| m30_00_0010 | base slot0 | 0: accepted (exp 2 S 0.85 vp 0.34) | 0: 1/2 -> drawn: m30_00_CGrading0002 present | 0: shape 1 lv 8 |
| m30_00_0100 | NOT selectable per census | 0: auto exposure (exp 2.46 S 0.9 vp 0) | 0: 0/-1 -> off (Enable 0: no draw, as the original) | absent: inherits base |
| m30_00_0110 | NOT selectable per census | 0: auto exposure (exp 2.46 S 0.9 vp 0) | 0: 0/-1 -> off (Enable 0: no draw, as the original) | absent: inherits base |
| m30_00_0200 | selectable (slot 2) | 0: accepted (exp 2.3 S 0.75 vp 0.34) | 0: 1/2 -> drawn: m30_00_CGrading0002 present | 0: shape 1 lv 8 |
| m30_01_0000 | base slot0 | 0: accepted (exp 2.2 S 0.8 vp 0.3) | 0: 1/2 -> drawn: m30_01_CGrading0002 present | 0: shape 1 lv 8 |
| m30_01_0010 | base slot0 | 0: accepted (exp 2.2 S 0.85 vp 0.3) | 0: 1/2 -> drawn: m30_01_CGrading0002 present | 0: shape 1 lv 8 |
| m30_01_0100 | selectable (slot 1) | absent: inherits base | absent: inherits base file record | absent: inherits base |
| m30_01_0110 | selectable (slot 1) | absent: inherits base | absent: inherits base file record | absent: inherits base |
| m30_01_0210 | NOT selectable per census | absent: inherits base | absent: inherits base file record | absent: inherits base |
| m31_00_0000 | base slot0 | 0: accepted (exp 2.2 S 0.9 vp 0.2) | 0: 1/1 -> drawn: m31_00_CGrading0001 present | 0: shape 1 lv 8 |
| m31_00_0010 | base slot0 | 0: accepted (exp 2.2 S 0.85 vp 0.2) | 0: 1/1 -> drawn: m31_00_CGrading0001 present | 0: shape 1 lv 8 |
| m31_00_0020 | base slot0 | 0: accepted (exp 2.2 S 0.9 vp 0.2) | 0: 1/1 -> drawn: m31_00_CGrading0001 present | 0: shape 1 lv 8 |
| m31_00_0030 | base slot0 | 0: accepted (exp 2.2 S 0.9 vp 0.2) | 0: 1/1 -> drawn: m31_00_CGrading0001 present | 0: shape 1 lv 8 |
| m31_00_0100 | selectable (slot 1) | absent: inherits base | absent: inherits base file record | absent: inherits base |
| m31_00_0110 | selectable (slot 1) | absent: inherits base | absent: inherits base file record | absent: inherits base |
| m32_00_0000 | base slot0 | 0: accepted (exp 2.4 S 0.85 vp 0) | 0: 1/2 -> drawn: m32_00_CGrading0002 present | 0: shape 1 lv 8 |
| m32_00_0010 | base slot0 | 0: accepted (exp 2 S 0.75 vp 0) | 0: 1/1 -> drawn: m32_00_CGrading0001 present | absent: inherits base |
| m32_00_0100 | selectable (slot 1) | 0: accepted (exp 2 S 1 vp 0) | 0: 0/-1 -> off (Enable 0: no draw, as the original) | 0: shape 3 lv 10  REFUSED glare shape |
| m32_00_0110 | selectable (slot 1) | 0: accepted (exp 2 S 0.85 vp 0) | 0: 1/1 -> drawn: m32_00_CGrading0001 present | 0: shape 1 lv 8 |
| m33_00_0000 | base slot0 | 0: accepted (exp 2.3 S 0.85 vp 0.38) | 0: 1/2 -> drawn: m33_00_CGrading0002 present | 0: shape 1 lv 8 |
| m33_00_0010 | base slot0 | 0: accepted (exp 2.3 S 0.85 vp 0.38) | 0: 1/2 -> drawn: m33_00_CGrading0002 present | 0: shape 1 lv 8 |
| m33_00_0100 | selectable (slot 1) | absent: inherits base | absent: inherits base file record | absent: inherits base |
| m33_00_0110 | selectable (slot 1) | absent: inherits base | absent: inherits base file record | absent: inherits base |
| m33_00_0200 | selectable (slot 2) | 0: accepted (exp 2.3 S 0.85 vp 0.38) | absent: inherits base file record | absent: inherits base |
| m33_00_0210 | selectable (slot 2) | 0: accepted (exp 2.3 S 0.85 vp 0.38) | absent: inherits base file record | absent: inherits base |
| m33_00_0300 | selectable (slot 3) | absent: inherits base | absent: inherits base file record | absent: inherits base |
| m33_00_0310 | selectable (slot 3) | absent: inherits base | absent: inherits base file record | absent: inherits base |
| m33_00_0400 | selectable (slot 4) | absent: inherits base | absent: inherits base file record | absent: inherits base |
| m33_00_0410 | selectable (slot 4) | absent: inherits base | absent: inherits base file record | absent: inherits base |
| m33_00_0500 | selectable (slot 5) | absent: inherits base | absent: inherits base file record | absent: inherits base |
| m33_00_0510 | selectable (slot 5) | absent: inherits base | absent: inherits base file record | absent: inherits base |
| m33_00_0600 | selectable (slot 6) | absent: inherits base | absent: inherits base file record | absent: inherits base |
| m33_00_0700 | selectable (slot 7) | absent: inherits base | absent: inherits base file record | absent: inherits base |
| m34_01_0000 | base slot0 | 0: accepted (exp 2 S 0.8 vp 0.3) | 0: 1/1 -> drawn: m34_01_CGrading0001 present | 0: shape 1 lv 8 |
| m34_01_0010 | base slot0 | 0: accepted (exp 2 S 0.8 vp 0.3) | 0: 1/1 -> drawn: m34_01_CGrading0001 present | 0: shape 1 lv 8 |
| m34_01_0110 | NOT selectable per census | absent: inherits base | absent: inherits base file record | absent: inherits base |
| m35_00_0000 | base slot0 | 0: accepted (exp 2 S 0.85 vp 0.4) | 0: 1/1 -> drawn: m35_00_CGrading0001 present | 0: shape 1 lv 8 |
| m35_00_0010 | base slot0 | 0: accepted (exp 2 S 0.85 vp 0.4) | 0: 1/1 -> drawn: m35_00_CGrading0001 present | 0: shape 1 lv 8 |
| m35_00_0100 | selectable (slot 1) | absent: inherits base | absent: inherits base file record | absent: inherits base |
| m35_00_0110 | selectable (slot 1) | absent: inherits base | absent: inherits base file record | absent: inherits base |
| m35_00_0200 | selectable (slot 2) | absent: inherits base | absent: inherits base file record | absent: inherits base |
| m35_00_0210 | selectable (slot 2) | absent: inherits base | absent: inherits base file record | absent: inherits base |
| m35_00_0300 | selectable (slot 3) | absent: inherits base | absent: inherits base file record | absent: inherits base |
| m35_00_0310 | selectable (slot 3) | absent: inherits base | absent: inherits base file record | absent: inherits base |
| m35_00_0500 | NOT selectable per census | absent: inherits base | absent: inherits base file record | absent: inherits base |
| m35_00_0510 | NOT selectable per census | absent: inherits base | absent: inherits base file record | absent: inherits base |
| m36_00_0000 | base slot0 | 0: auto exposure (exp 2.46 S 1 vp 0) | 0: 0/-1 -> off (Enable 0: no draw, as the original) | absent: inherits base |
| m36_00_0001 | NOT selectable per census | 0: auto exposure; hue or value (ColorV=1.2) (exp 1.56 S 0.8 vp 0) | 0: 1/0 -> texture absent (m36_00_CGrading0000 ships no strip) | 0: shape 1 lv 8 |
| m36_00_0002 | NOT selectable per census | 0: auto exposure (exp 1.56 S 0.9 vp 0) | 0: 1/0 -> texture absent (m36_00_CGrading0000 ships no strip) | 0: shape 1 lv 8 |
| m36_00_0003 | NOT selectable per census | 0: auto exposure (exp 1.56 S 0.3 vp 0) | 0: 1/0 -> texture absent (m36_00_CGrading0000 ships no strip) | 0: shape 1 lv 8 |
| m36_00_0100 | selectable (slot 1) | 0: auto exposure (exp 2.46 S 1 vp 0) | 0: 0/-1 -> off (Enable 0: no draw, as the original) | absent: inherits base |
| m37_00_0000 | base slot0 | 0: accepted (exp 1.52 S 0.9 vp 0) | 0: 0/-1 -> off (Enable 0: no draw, as the original) | 0: shape 1 lv 6 |
| m37_00_0010 | base slot0 | 0: accepted (exp 1.52 S 0.9 vp 0) | 0: 0/-1 -> off (Enable 0: no draw, as the original) | 0: shape 1 lv 6 |
| m37_00_0100 | selectable (slot 1) | absent: inherits base | 0: 0/-1 -> off (Enable 0: no draw, as the original) | absent: inherits base |
| m37_00_0110 | selectable (slot 1) | absent: inherits base | 0: 0/-1 -> off (Enable 0: no draw, as the original) | absent: inherits base |
| m37_00_0200 | selectable (slot 2) | absent: inherits base | 0: 0/-1 -> off (Enable 0: no draw, as the original) | absent: inherits base |
| m37_00_0210 | selectable (slot 2) | absent: inherits base | 0: 0/-1 -> off (Enable 0: no draw, as the original) | absent: inherits base |
| m37_00_0300 | selectable (slot 3) | absent: inherits base | 0: 0/-1 -> off (Enable 0: no draw, as the original) | absent: inherits base |
| m37_00_0310 | selectable (slot 3) | absent: inherits base | 0: 0/-1 -> off (Enable 0: no draw, as the original) | absent: inherits base |
| m37_00_0400 | selectable (slot 4) | absent: inherits base | 0: 0/-1 -> off (Enable 0: no draw, as the original) | absent: inherits base |
| m37_00_0410 | selectable (slot 4) | absent: inherits base | 0: 0/-1 -> off (Enable 0: no draw, as the original) | absent: inherits base |
| m37_00_0500 | selectable (slot 5) | 0: accepted (exp 2.3 S 0.85 vp 0.3) | 0: 1/1 -> drawn: m37_00_CGrading0001 present | absent: inherits base |
| m37_00_0510 | selectable (slot 5) | 0: accepted (exp 2.3 S 0.85 vp 0.3) | 0: 1/1 -> drawn: m37_00_CGrading0001 present | absent: inherits base |
| m38_00_0000 | base slot0 | 0: accepted (exp 2 S 0.85 vp 0.35) | 0: 1/3 -> texture absent (m38_00_CGrading0003 ships no strip) | 0: shape 1 lv 8 |
| m38_00_0002 | NOT selectable per census | 0: accepted (exp 2 S 0.8 vp 0) | 0: 0/-1 -> off (Enable 0: no draw, as the original) | 0: shape 1 lv 8 |
| m38_00_0100 | selectable (slot 1) | absent: inherits base | absent: inherits base file record | absent: inherits base |
| m38_00_0200 | selectable (slot 2) | absent: inherits base | absent: inherits base file record | absent: inherits base |
| m38_00_0600 | selectable (slot 6) | absent: inherits base | 0: 1/3 -> texture absent (m38_00_CGrading0003 ships no strip) | absent: inherits base |
| m39_00_0000 | base slot0 | 0: accepted (exp 2.3 S 0.84 vp 0.3) | 0: 1/3 -> drawn: m39_00_CGrading0003 present | 0: shape 1 lv 8 |
| m39_00_0100 | NOT selectable per census | 0: accepted (exp 2 S 0.8 vp 0.3) | 0: 0/-1 -> off (Enable 0: no draw, as the original) | absent: inherits base |
| m39_00_0200 | NOT selectable per census | absent: inherits base | 0: 0/-1 -> off (Enable 0: no draw, as the original) | absent: inherits base |
| m39_00_0300 | selectable (slot 3) | absent: inherits base | absent: inherits base file record | absent: inherits base |
| m39_00_0500 | selectable (slot 5) | absent: inherits base | absent: inherits base file record | absent: inherits base |
| m40_00_0000 | base slot0 | 0: accepted (exp 2.3 S 0.8 vp 0.3) | 0: 1/3 -> drawn: m40_00_CGrading0003 present | 0: shape 1 lv 6 |
| m40_00_0010 | base slot0 | 0: accepted (exp 2.3 S 0.8 vp 0.3) | 0: 1/3 -> drawn: m40_00_CGrading0003 present | 0: shape 1 lv 6 |
| m40_00_0510 | selectable (slot 5) | absent: inherits base | absent: inherits base file record | absent: inherits base |
| m41_00_0000 | base slot0 | 0: accepted (exp 2 S 0.8 vp 0.3) | 0: 1/1 -> drawn: m41_00_CGrading0001 present | 0: shape 1 lv 6 |
| m41_00_0100 | NOT selectable per census | absent: inherits base | absent: inherits base file record | absent: inherits base |
| m41_00_0200 | selectable (slot 2) | absent: inherits base | absent: inherits base file record | absent: inherits base |
| m41_00_0800 | selectable (slot 8) | 0: accepted (exp 0.45 S 1 vp 0) | absent: inherits base file record | absent: inherits base |
| m41_00_0900 | selectable (slot 9) | 0: exposure or gamma (0/2.2) (exp 0 S 1 vp 0) | absent: inherits base file record | 0: shape 1 lv 8 |
| m45_00_0000 | base slot0 | 0: accepted (exp 2 S 0.85 vp 0.35) | 0: 1/0 -> texture absent (m45_00_CGrading0000 ships no strip) | 0: shape 1 lv 6 |
| m45_00_0100 | selectable (slot 1) | absent: inherits base | absent: inherits base file record | absent: inherits base |
| m45_00_0200 | selectable (slot 2) | absent: inherits base | absent: inherits base file record | absent: inherits base |
| m45_00_0300 | selectable (slot 3) | absent: inherits base | absent: inherits base file record | absent: inherits base |
| m45_00_0400 | selectable (slot 4) | absent: inherits base | absent: inherits base file record | absent: inherits base |
| m45_00_0500 | selectable (slot 5) | absent: inherits base | absent: inherits base file record | absent: inherits base |
| m45_00_0600 | selectable (slot 6) | absent: inherits base | absent: inherits base file record | absent: inherits base |
| m45_00_0700 | selectable (slot 7) | absent: inherits base | absent: inherits base file record | absent: inherits base |
| m45_00_0800 | selectable (slot 8) | absent: inherits base | absent: inherits base file record | absent: inherits base |
| m46_00_0000 | base slot0 | 0: accepted (exp 2 S 0.8 vp 0.3) | 0: 1/1 -> texture absent (m46_00_CGrading0001 ships no strip) | 0: shape 1 lv 8 |
| m47_00_0000 | base slot0 | 0: accepted (exp 2 S 0.8 vp 0.3) | 0: 1/1 -> texture absent (m47_00_CGrading0001 ships no strip) | 0: shape 1 lv 6 |
| m47_00_0100 | NOT selectable per census | absent: inherits base | absent: inherits base file record | absent: inherits base |
| m47_00_0200 | selectable (slot 2) | absent: inherits base | absent: inherits base file record | absent: inherits base |
| m47_00_0800 | NOT selectable per census | 0: accepted (exp 0.45 S 1 vp 0) | absent: inherits base file record | absent: inherits base |
| m47_00_0900 | NOT selectable per census | 0: exposure or gamma (0/2.2) (exp 0 S 1 vp 0) | absent: inherits base file record | 0: shape 1 lv 8 |
| m50_00_0000 | base slot0 | 0: accepted (exp 2 S 0.9 vp 0.35) | 0: 1/1 -> TO CHECK: m50_00_CGrading0001 is in the DLC2 archive but the native logged texture absent | 0: shape 1 lv 6 |
| m50_00_0100 | selectable (slot 1) | absent: inherits base | absent: inherits base file record | absent: inherits base |
| m50_00_0200 | selectable (slot 2) | absent: inherits base | absent: inherits base file record | absent: inherits base |
| m50_00_0300 | selectable (slot 3) | absent: inherits base | absent: inherits base file record | absent: inherits base |
| m51_00_0000 | base slot0 | 0: accepted (exp 2 S 0.85 vp 0.35) | 0: 1/0 -> texture absent (m51_00_CGrading0000 ships no strip) | 0: shape 1 lv 6 |
| m51_00_0100 | selectable (slot 1) | absent: inherits base | absent: inherits base file record | absent: inherits base |
| m51_00_0200 | selectable (slot 2) | absent: inherits base | absent: inherits base file record | absent: inherits base |
| m53_00_0000 | base slot0 | 0: accepted (exp 2.4 S 0.85 vp 0) | 0: 1/2 -> texture absent (m53_00_CGrading0002 ships no strip) | 0: shape 1 lv 8 |

### B. Cutscene / event files (sAA_BB_CCCC)

| file | Tone Map ids: refused reasons | ColorGrading | Bloom shape != 1 ids |
|---|---|---|---|
| s30_00_0000 | accepted: 0,90 | 0:1/10 90:1/10 | 0=0 |
| s30_00_0010 | accepted: -; REFUSED: 0 (hue or value (ColorV=0.9); vignette fov (0.5, power 0.4)) | 0:1/1 | - |
| s30_00_0020 | accepted: 0 | 0:0/-1 | - |
| s30_00_0030 | accepted: 0 | 0:0/-1 | 0=0 |
| s30_00_0040 | accepted: 0 | 0:0/-1 | 0=0 |
| s30_00_9000 | accepted: 0 | 0:1/2 | - |
| s31_00_0000 | accepted: -; REFUSED: 100 (vignette fov (0.11, power 0.139)) / 103 (vignette fov (0.05, power 0.35)) / 104 (vignette fov (0.09, power 0.329)) | absent | - |
| s31_00_0010 | accepted: 0,11,20 | 0:1/1 10:1/10 | - |
| s32_00_0000 | accepted: 0,35 | 0:1/2 | - |
| s32_00_0010 | accepted: 0 | 0:0/-1 | 0=0,20=0 |
| s32_00_0020 | accepted: 0 | 0:0/-1 | 0=0 |
| s32_00_0030 | accepted: 0 | 0:1/1 | - |
| s33_00_0000 | accepted: 0 | 0:1/1 | - |
| s33_00_0010 | accepted: -; REFUSED: 0 (vignette fov (0.52, power 0.559)) | 0:0/-1 | 0=0 |
| s33_00_0020 | accepted: 0 | 0:1/1 | - |
| s33_00_0021 | accepted: 0 | 0:1/1 | - |
| s33_00_0022 | accepted: 0 | 0:1/1 | - |
| s33_00_0030 | accepted: 0 | 0:1/1 | - |
| s33_00_0031 | accepted: 0 | 0:1/1 | - |
| s33_00_0032 | accepted: 0 | 0:1/1 | - |
| s33_00_0040 | accepted: 0 | 0:1/1 | - |
| s33_00_0041 | accepted: 0 | 0:1/1 | - |
| s33_00_0042 | accepted: 0 | 0:1/1 | - |
| s34_01_0000 | accepted: 0 | 0:1/1 | - |
| s34_01_0010 | accepted: 0,102,103; REFUSED: 100 (vignette fov (1, power 0.279)) / 101 (vignette fov (1, power 0.279)) / 104 (vignette fov (1, power 0.45)) | 0:1/1 | - |
| s35_00_0000 | accepted: 0 | 0:0/-1 | - |
| s35_00_0001 | accepted: 0 | 0:0/-1 | - |
| s37_00_0020 | accepted: 0 | 0:0/-1 | - |
| s37_00_0021 | accepted: 0 | 0:0/-1 | - |
| s37_00_0030 | accepted: 0 | 0:0/-1 | - |
| s38_00_0000 | accepted: 0 | 0:0/-1 | 0=0 |
| s38_00_0010 | accepted: 0 | 0:0/-1 | 0=0 |
| s38_00_0020 | accepted: 0 | 0:0/-1 | 0=0 |
| s39_00_0010 | accepted: 0 | 0:0/-1 | 0=0 |
| s39_00_0020 | accepted: 0 | 0:0/-1 | 0=0 |
| s39_00_0030 | accepted: 0 | 0:1/1 | - |
| s40_00_0020 | accepted: 0,100,101,102; REFUSED: 10 (hue or value (ColorV=0); exposure or gamma (0/0)) / 200 (hue or value (ColorV=0.5)) / 300 (hue or value (ColorV=0.7)) | 0:1/3 | - |
| s40_00_0021 | accepted: 0,100,101,102; REFUSED: 10 (hue or value (ColorV=0); exposure or gamma (0/0)) / 200 (hue or value (ColorV=0.5)) / 300 (hue or value (ColorV=0.7)) | 0:1/3 | - |
| s40_00_0040 | accepted: 100; REFUSED: 0 (tone type 3; hue or value (ColorV=0.6)) / 1 (tone type 3; hue or value (ColorV=0.73)) / 10 (hue or value (ColorV=0); exposure or gamma (0/0)) | 0:1/3 | - |
| s41_00_0000 | accepted: -; REFUSED: 0 (hue or value (ColorV=1.2)) / 1 (hue or value (ColorV=1.2)) | 0:0/-1 | - |
| s41_00_0050 | accepted: 0; REFUSED: 100 (hue or value (ColorV=0.75); vignette fov (0.99, power 0.4)) / 101 (hue or value (ColorV=0)) / 102 (vignette fov (0.77, power 0.35)) / 103 (vignette fov (0.77, power 0.5)) / 133 (hue or value (ColorV=0.9231); vignette fov (0.99, power 0.4)) / 134 (hue or value (ColorV=0.9231); vignette fov (0.99, power 0.4)) / 135 (vignette fov (0.99, power 0.4)) / 150 (hue or value (ColorV=0.9231); vignette fov (0.99, power 0.4)) / 151 (hue or value (ColorV=0.6); vignette fov (0.99, power 0.4)) / 200 (vignette fov (0.99, power 0.4)) | 0:1/1 | - |
| s41_00_0051 | accepted: 0; REFUSED: 100 (hue or value (ColorV=0.75); vignette fov (0.99, power 0.4)) / 101 (hue or value (ColorV=0)) / 102 (vignette fov (0.77, power 0.35)) / 103 (vignette fov (0.77, power 0.5)) / 133 (hue or value (ColorV=0.9231); vignette fov (0.99, power 0.4)) / 134 (hue or value (ColorV=0.9231); vignette fov (0.99, power 0.4)) / 135 (vignette fov (0.99, power 0.4)) / 150 (hue or value (ColorV=0.9231); vignette fov (0.99, power 0.4)) / 151 (hue or value (ColorV=0.6); vignette fov (0.99, power 0.4)) / 200 (vignette fov (0.99, power 0.4)) | 0:1/1 | - |
| s41_00_0060 | accepted: 0,1,10,20; REFUSED: 21 (exposure or gamma (0/2.2); vignette fov (0.4, power 0.3)) | 0:0/-1 | - |
| s41_00_0070 | accepted: 0; REFUSED: 100 (vignette fov (0.99, power 1.35)) / 101 (hue or value (ColorV=0)) / 102 (vignette fov (0.77, power 0.5)) | 0:1/1 | - |
| s45_00_0000 | accepted: 0,102,103,104,108,109; REFUSED: 101 (hue or value (ColorV=0.95); vignette fov (0.7, power 0.6)) / 105 (hue or value (ColorV=0.95); vignette fov (0.7, power 0.6)) / 106 (hue or value (ColorV=0.95); vignette fov (0.7, power 0.45)) / 107 (vignette fov (0.7, power 0.6)) / 110 (hue or value (ColorV=0.95); vignette fov (0.7, power 0.6)) / 111 (hue or value (ColorV=1.197); vignette fov (0.7, power 0.45)) / 112 (vignette fov (0.7, power 0.45)) / 113 (hue or value (ColorV=4); vignette fov (0.7, power 0.45)) | 0:0/-1 | 0=0,101=0,103=0,104=0,107=0,109=0 |
| s45_00_0010 | accepted: 0,102,103,104,108,109; REFUSED: 101 (hue or value (ColorV=0.95); vignette fov (0.7, power 0.6)) / 105 (hue or value (ColorV=0.95); vignette fov (0.7, power 0.6)) / 106 (hue or value (ColorV=0.95); vignette fov (0.7, power 0.45)) / 107 (vignette fov (0.7, power 0.6)) / 110 (hue or value (ColorV=0.95); vignette fov (0.7, power 0.6)) / 111 (hue or value (ColorV=1.197); vignette fov (0.7, power 0.45)) / 112 (vignette fov (0.7, power 0.45)) / 113 (hue or value (ColorV=4); vignette fov (0.7, power 0.45)) | 0:0/-1 | 0=0,101=0,103=0,104=0,107=0,109=0 |
| s45_00_0030 | accepted: 0,100; REFUSED: 101 (vignette fov (0.85, power 0.65)) / 102 (vignette fov (0.8, power 0.5)) / 103 (hue or value (ColorV=0)) | 0:0/-1 | 0=0 |
| s45_00_0031 | accepted: 0,100; REFUSED: 101 (vignette fov (0.85, power 0.65)) / 102 (vignette fov (0.8, power 0.5)) / 103 (hue or value (ColorV=0)) | 0:0/-1 | 0=0 |
| s51_00_0000 | accepted: 0; REFUSED: 1010 (vignette fov (0.5, power 0.2)) / 1020 (vignette fov (0.5, power 0.4)) / 1030 (vignette fov (0.5, power 0.5)) / 1040 (vignette fov (0.29, power 0.3)) / 1041 (vignette fov (0.91, power 0.579)) / 2050 (vignette fov (0.5, power 0.2)) / 2051 (vignette fov (0.5, power 0.2)) / 2052 (vignette fov (0.5, power 0.2)) / 2060 (vignette fov (0.5, power 0.2)) | 0:1/0 1:1/1 | - |

### C. Global files

| file | Tone Map | ColorGrading | Bloom |
|---|---|---|---|
| default | 0: accepted ; 10: accepted ; 11: accepted ; 12: accepted ; 13: accepted ; 14: accepted ; 15: accepted | 0:0/-1 | 0=0,12=0 |
| eclipse | absent | absent | absent |
| empty | absent | absent | absent |
| m_template | REFUSED (whole feed, "Tone Map block (layout)"): layout (34 params <35) | 0:0/-1 | layout |
| mapbase | absent | absent | 0=1 |
| mapoverride_0000 | absent | absent | absent |
| mapoverride_0001 | 0: auto exposure | 0:0/-1 | absent |
| mapoverride_0002 | absent | absent | absent |
| mapoverride_0003 | 0: auto exposure; hue or value (ColorV=1.23); vignette fov (0.5, power 0.5) | 0:0/-1 | absent |
| ml_template | absent | absent | absent |
| remobase | absent | absent | 0=1 |
| s_template | REFUSED (whole feed, "Tone Map block (layout)"): layout (34 params <35) | 0:0/0 | layout |

### D. Counts (records = file x value id; files = files with at least one)

| class | reason | records | files |
|---|---|---|---|
| global | ANY refused | 2 | 2 |
| global | accepted | 7 | 1 |
| global | auto exposure | 2 | 2 |
| global | hue or value | 1 | 1 |
| global | layout | 2 | 2 |
| global | vignette fov | 1 | 1 |
| map | ANY refused | 9 | 9 |
| map | accepted | 40 | 40 |
| map | auto exposure | 7 | 7 |
| map | exposure or gamma | 2 | 2 |
| map | hue or value | 1 | 1 |
| map | layout | 2 | 2 |
| s | ANY refused | 74 | 17 |
| s | accepted | 72 | 45 |
| s | exposure or gamma | 4 | 4 |
| s | hue or value | 39 | 12 |
| s | tone type 3 | 2 | 1 |
| s | vignette fov | 58 | 13 |

Tone Map records by class: {'global': 11, 'map': 51, 's': 146}

## 4. Counts per reason, ranked by how much of the game they touch

Unit "areas" = the retail world map areas `m30_00 m30_01 m31_00 m32_00 m33_00 m34_01 m35_00 m37_00 m38_00 m39_00 m40_00 m41_00 m45_00
m46_00 m47_00 m50_00 m51_00 m53_00` (18; the msb census also lists m33_01, m34_00, m30_02, m31_01..09 with no gparam of their own).

Tone map, world class (records / files; D above lists all classes):

| rank | reason | world-class records / files | retail-reachable | touches |
|---|---|---|---|---|
| 1 | `exposure or gamma` (Exposure 0) | 2 / 2 (`m41_00_0900`, `m47_00_0900`) | `m41_00_0900` (slot 9, 1 collision of 16) | a small area or fade state of one map; INFERRED reachable |
| 2 | `auto exposure` | 7 / 7 (`m30_00_0100/0110`, `m36_00_0000..0003, 0100`) | none (K-0448: no m30_00 collision names file 1; m36 not retail) | nothing in retail (INFERRED) |
| 3 | `hue or value` (ColorV 1.2) | 1 / 1 (`m36_00_0001`) | none | nothing |
| 4 | `layout` (34-param Tone Map) | 2 / 2 (`m21`, `m22`) + 2 global templates | none | test files |

Tone map, s-files (cutscene/event class, 49 files, 146 records; 74 refused records, 17 files; reachability and use TO CHECK):

| rank | reason | records / files |
|---|---|---|
| 1 | `vignette fov` (VignetteFovDependence != 0) | 58 / 13 (`s30_00_0010`, `s31_00_0000`, `s33_00_0010`, `s34_01_0010`, `s41_00_0050/0051/0060/0070`, `s45_00_*`, `s51_00_0000`...) |
| 2 | `hue or value` (ColorV != 1) | 39 / 12 (`s30_00_0010`, `s40_00_0020/0021/0040`, `s41_00_*`, `s45_00_*`...) |
| 3 | `exposure or gamma` | 4 / 4 (`s40_00_0020/0021/0040` id 10, `s41_00_0060` id 21) |
| 4 | `tone type 3` | 2 / 1 (`s40_00_0040` ids 0 and 1) |
| - | `auto exposure` | 0 |

Global files: `mapoverride_0001` (auto exposure, vignette fov 1), `mapoverride_0003` (auto exposure, ColorV 1.23, fov 0.5): not selected
in retail (K-0448: the Ceremony row supplies no override id but `30001020` = 600, no such file).

Grading LUT, retail world areas (base slot-0 files; per-area, from section A):

| outcome | areas | count |
|---|---|---|
| drawn (strip exists) | m30_00, m30_01, m31_00, m32_00 (0000: id 2, 0010: id 1), m33_00, m34_01, m35_00, m39_00, m40_00, m41_00 | 10 |
| off (Enable 0; also off in the original) | m37_00 base (m37_00_0500/0510 draw `m37_00_CGrading0001`, 1 collision) | 1 |
| `texture absent` (no strip: no draw, as the original) | m38_00 (id 3), m45_00 (id 0), m46_00 (id 1), m47_00 (id 1), m51_00 (id 0), m53_00 (id 2) | 6 |
| `texture absent` with a strip in the scan | m50_00 (id 1, `m50_00_CGrading0001` in DLC2): **TO CHECK** | 1 |
| `non-map record` | never on the shipped data (INFERRED, section 0.4) | 0 |

The grading rule's departure from the original is therefore limited to m50 (if the original loads the DLC2 strip) and to the s-file
records if the original names the LUT differently from "the key's area" (the prefix branch of FUN_140095220 is unread, W10 section 12).
Cutscene LUT records: 48 of 49 s-files carry ColorGrading (51 records: 22 with Enable 0, 29 with Enable 1 and LUT ids 0, 1, 2, 3, 10); `s31_00_0000` has none.

Glare shape != 1: `default` ids 0 and 12 (frames before an area key is known), `m32_00_0100` (retail, slot 1), 27 s-file records (list in the B
table column "Bloom shape != 1"). Auto focus: 0 records. DoF holder flags (INFERRED mapping): `m30_00_0200`, `m32_00_0100`, `m38_00_0002`
(base id 2 only), `m36_00_0003`, and about 229 s-file records.

## 5. Open points (TO CHECK)

1. Which s-files and ids the engine applies, in which scenes, and the transitions (the blended tone[2], ColorV, auto flag): the 74 refused
   s-file records only matter if applied; cutscenes and boss intros in the layers the area binds are the candidates.
2. m50/m51 DLC2 `CGrading` strips: is the original's grading drawn at m50 (id 1), and does native admission register the hash-named
   containers? The m51 record asks id 0 (no strip in any case).
3. Word 0x240 (DoF holder flags) source and word 0x100 (bloom gamma): the exe converters, not read.
4. `m41_00_0900` (and `m47_00_0900`): Exposure 0 is accepted by the original as black; what the native shows there under the sRGB
   composition fallback is a different picture (INFERRED): confirm the slot 9 collision's location.
5. LightShaft, MotionBlur and Anti-Alias records were not censused for row 13's shaft/motion-blur limits.
6. Later journals (I-0113 onward) were not read: a run in a zone of section A that shows a `refused` other than the listed ones would
   contradict this reading.
