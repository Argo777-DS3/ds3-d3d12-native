# Census of the baked light probes (.btpb) of Dark Souls III

**Non-canonical until the study thread re-reads it.** Date 2026-11-22. Worker output for the study of workstream 12 (physical light sources). Read-only data work: no game, no build, no Ghidra.

Inputs: the 22 recovered banks `C:/Users/User/Tools/DS3-Decompilation/gamedata-i0110/*btpb.dcx`; `investigations/I-0067-static-menus-subsystems-map/TONEMAP-SKIN.md` section 10; `investigations/I-0126-rt-gi/STUDY-R7-DS3-AMBIENT.md` section 3.2; `evidence/I-0110/btl-census.tsv`; the recovered MSBs `map_mapstudio_m30_00_00_00.msb.dcx` and `map_mapstudio_m40_00_00_00.msb.dcx` (read with SoulsFormats 1.0.1 `MSB3`, positions and regions); `evidence/I-0066/queries/gparam-all/param_drawparam_m30_00_0000.tsv` and `..._m40_00_0000.tsv`.

Scripts and tables (all in `C:/Users/User/Tools/DS3-Decompilation/study-ws12/btpb/`): `Expand-Btpb.ps1` (DCX to raw), `btpb.py` (parser), `analyze.py` (per-bank statistics), `place.py` (m30/m40 places), `hemi.py`, `checks.py`, `Get-MsbPositions.ps1`, `build_report.py`. Tables in `C:/Users/User/Tools/DS3-Decompilation/study-ws12/btpb/out/`: `banks-summary.tsv` (one row per bank), `groups-all-banks.tsv` (every volume of every bank), `probes-m30_00_0000.tsv`, `probes-m30_00_0001.tsv`, `probes-m40_00_0000.tsv` (every probe: raw int16, decoded values), `clusters-m30_00_0000.tsv` (16 m x 16 m x 10 m bins), `bank-summary.json`, `place.json`, `hemi.json`, `msb-pos-m30.tsv` and `msb-pos-m40.tsv` (one level up).

Tags: OBSERVED = read in a file or computed from one; INFERRED = my reading, with its basis.

## 0. Headlines

1. OBSERVED: SoulsFormats 1.0.1 has no `BTPB` class (`[SoulsFormats.BTPB]` not found); only `DCX.Decompress` is used, and the bytes are parsed here. The format decodes with full internal consistency in all 22 banks (section 1).
2. OBSERVED: **the m40_00 bank holds no probe near the Firelink Shrine.** The bonfire (273.2, -56.2, 606.3), the Fire Keeper and the interior region (271.1, -56.2, 605.9) are 123 to 124 m from the nearest valid probe; the bank spans x -74..185, z 426..792. The question "interior of the shrine against open air" cannot be answered from this bank; section 3 answers it for the places the bank does cover (the Cemetery of Ash side) and for the High Wall.
3. OBSERVED: the 13th short of every record (`s12`, the engine's `q+0x16C` target, `s12/1024`) is 0 or about 1.0 (1022) and separates open air from covered places: probes with s12/1024 >= 0.5 ("open") against s12 == 0 ("closed"). INFERRED that it is an authored sky-exposure weight, because its two classes differ in brightness and in L1 direction (below). All inside/outside numbers use it.
4. OBSERVED, m30_00 bank 0000 (High Wall): open probes (22.6 % of the valid ones) have a median L0 luma 0.104 against 0.031 for closed probes, so covered places are 3.3 times darker; open L1 points up in 96 % of the probes (down in 0.3 %), closed ones point down in 51 % and up in 35 %.
5. OBSERVED, m40_00 (the part it covers): open (41.1 %) median luma 0.164, closed 0.030 (5.4 times darker); open L1 up 72 %, side 28 %, down 0.1 %; closed L1 down 54 %, up 18 %.
6. OBSERVED: colour is nearly one constant per bank. The median R/B of L0 is 0.686 in the High Wall banks whatever the brightness (open p10..p90 0.682..0.688), 0.65 in m40; 83 to 100 % of the probes of 19 of the 22 banks classify "cold" (R/B < 0.87): the authored probe ambient is a blue-grey everywhere except a few pockets (m30_00 group 24, mean RGB 0.259, 0.223, 0.199, a warm local light; m51_00 and m51_01 neutral).
7. OBSERVED: the authored hemisphere of the zones (LightSet "Hemi Color Up/Down") is a uniform 0.25 grey in m30_00 for 34 of its 43 ids and mostly black (intensity 0) in m40_00. The open-air probes' up-facing ambient is 0.285 luma (m30) and 0.265 (m40), bluer (R/B 0.69, 0.65), and their down-facing is 0 (m30) and 0.077 (m40): section 4.

## 1. Format as read

### 1.1 Container and header (OBSERVED)

The `.btpb.dcx` files are DCX; `SoulsFormats.DCX.Decompress` gives the raw bank (little endian). The sizes check exactly in all 22 banks: `0x3C + probe_bytes + groups x group_stride = file size`.

| offset | type | meaning | evidence |
|---|---|---|---|
| +0x00 | u32 | version: 3 in 21 banks, 2 in `m39_00_0001` | stride 0x98 (v3) / 0x48 (v2) matches TONEMAP-SKIN s10 "group stride 0x48 before version 3, 0x98 from version 3" |
| +0x04 | u32 | 0 in v3 banks, 1 in the v2 bank (unknown) | |
| +0x08 | u32 | number of groups (volumes) | |
| +0x0C | u32 | probe data size in bytes = 72 x total probes | exact in 22/22 |
| +0x10 | u32 | 0 | |
| +0x14 | u32 | group stride (152 or 72) | |
| +0x18 | u32 | probe stride = 72 | |
| +0x1C | 6 x f32 | bank AABB: (min x, min y, **-max z**, max x, max y, **-min z**): it equals the union of the group AABBs with z negated and swapped, exactly in 22/22 banks (m30_00: header z -505.2..64.2, group z -64.2..505.2; the probe positions fill it to within 2 to 10 m of padding) | computed |
| +0x34, +0x38 | 2 x u32 | 0 | |
| +0x3C | | probes of group 0 start here; group `g` starts at `+0x3C + off_g`; the group table sits at `+0x3C + probe_bytes` | TONEMAP-SKIN s10: offsets relative to header +0x3C |

### 1.2 Group (volume) record, v3 stride 0x98 (OBSERVED; v2 stride 0x48 is the same up to +0x44)

| offset | type | meaning |
|---|---|---|
| +0x00..+0x0B | 3 x u32 | 0 in the file (runtime pointers: INFERRED, the engine relocates offsets) |
| +0x0C | u32 | probe count = nx*ny*nz (exact in every group of 22 banks) |
| +0x10, +0x14, +0x18 | 3 x u32 | cell counts nx, ny, nz (TONEMAP-SKIN s10: "cell counts ushort +0x90..+0x94" of the runtime volume) |
| +0x1C | 3 x f32 | cell size (m): 2 m in most groups, also 0.5, 1, 1.5, 3, 5, 10 |
| +0x28 | 3 x f32 | AABB min of the grid |
| +0x34 | 3 x f32 | AABB max (about min + n*cell, not exactly) |
| +0x40 | u32 | byte offset of the group's first probe in the probe data; groups are consecutive (checked) |
| +0x44 | u32 | 0 |
| +0x48 | 3 x f32 | Euler rotation in radians, only the Y component used in practice (e.g. -0.2094 = -12 degrees). INFERRED a volume yaw; **the stored probe positions are always `min + (ix, iy, iz) * cell` in world axes, rotated groups included** (91,768 of 91,768 probes of m30_00_0000, checked) |
| +0x94 | u32 | 93 in one or two groups per bank, else 0 (unknown flag) |

Probe order inside a group: x fastest, then y, then z (`k = ix + nx*(iy + ny*iz)`, OBSERVED on the hand-decoded probe below and on all unrotated and rotated groups of m30_00_0000).

### 1.3 Probe record, 72 bytes (OBSERVED layout; mapping to the engine)

| offset | type | field | engine reading (TONEMAP-SKIN s10, `FUN_140061950`) |
|---|---|---|---|
| +0x00 | 3 x i16 `s0..s2` | L0, R G B | `row R.w = s0*k0`, `row G.w = s1*k0`, `row B.w = s2*k0` |
| +0x06 | 3 x i16 `s3..s5` | L1 **y** component, R G B | `row R.y = s3*k1` ... |
| +0x0C | 3 x i16 `s6..s8` | L1 **z** component, R G B | `row R.z = s6*k1` ... |
| +0x12 | 3 x i16 `s9..s11` | L1 **x** component, R G B | `row R.x = s9*k1` ... |
| +0x18 | i16 `s12` | the weight whose target is `q+0x16C = s12/1024` | 0 or about 1022 in practice |
| +0x1A | i16 | the priority short ("lowest +0x1A first" in the engine pick): **0 in every probe of the 22 banks** (OBSERVED) | the tie-break is moot in these banks |
| +0x1C | 3 x f32 | probe position (x, y, z), world axes of the map | `float3 +0x1C` |
| +0x28..+0x47 | 32 bytes | zero in every probe of m30_00_0000 (checked) | |

Scale (OBSERVED from the documents, verified here): `k0 = 0.282095/1024 = 2.7548e-4` (float bits 0x39906EC1) and `k1 = 0.488603/1024 = 4.7715e-4` (0x39FA2A2C) fold the SH basis constants Y00 and Y1m and the 1/1024 fixed-point step into the shorts; I checked both hex constants against the decimal values. So **the float values are `s*k0` and `s*k1`; the shorts are fixed point with 10 fractional bits before the basis constants**. Not uncertain by the documents; what stays uncertain is the unit (see 1.5).

Evaluation (the engine's, OBSERVED in TONEMAP-SKIN s10 and STUDY-R7 s1/s3.2): for a normal N, ambient_c(N) = max(0, k0*s_c + k1*(s_(9+c)*Nx + s_(3+c)*Ny + s_(6+c)*Nz)), c = 0,1,2 for R,G,B.

### 1.4 Quantities computed per probe (my definitions)

- **L0_c = k0 * s_c** (c = R,G,B): the N-independent term of the engine's evaluation, i.e. the level the probe gives a surface whatever its normal if L1 were zero. "Radiance-like level" in the brief = this; luma(L0) = 0.2126 R + 0.7152 G + 0.0722 B (applied to the linear values).
- **Up-facing** = ambient(N = (0,1,0)) = max(0, L0_c + k1*s_(3+c)); **down-facing** = max(0, L0_c - k1*s_(3+c)); luma of each computed on the clamped values. (This is the engine's `(1-F0) * max(0, aSh . (N,1))` without the F0 factor.)
- **L1 vector** V = sum over channels of luma weight_c * (k1*s_(9+c), k1*s_(3+c), k1*s_(6+c)) = (x, y, z): the luma-weighted direction of the gradient, which is where the light comes from. Strength = |V| / luma(L0). Classes: **up** if V.y/|V| > 0.5, **down** if < -0.5, else **side**; **flat** if |V|/luma(L0) < 0.05.
- **Temperature class** from R/B of L0 (K0 cancels, so it equals s0/s2): **warm** R/B > 1.15, **cold** R/B < 1/1.15 = 0.870, else **neutral** (thresholds mine, symmetric in log). A probe with L0 = 0 in all three channels is "dark".
- **Valid probe** = at least one of s0..s11 non-zero. All-zero probes (INFERRED placeholders for cells inside solid geometry, from their large share and a probe with no data being useless to the engine; their positions were not checked against geometry) are counted separately and left out of every statistic.
- **Open / partial / closed** by `s12/1024`: >= 0.5, in (0, 0.5), == 0.

### 1.5 Limits of the reading

- The unit of the floats is "the engine's `aSh` rows", i.e. what multiplies the albedo in the Ibl programmes before the `(1 - F0)` factor and the tone mapping; a radiance in cd/m2 is not claimed. (INFERRED from TONEMAP-SKIN s10 and STUDY-R7 s1.)
- The engine rotates every probe's L1 shorts by the map's rotation at load (`FUN_140003FB0`); the maps of this census are read **unrotated**. Up/down statements are unaffected by a yaw. Horizontal directions are in file axes (the m40 open probes lean toward +x: mean direction (0.78, 0.62, -0.09)); whether the map transform is identity here was not checked.
- The L1/L0 ratio is above 1 for 97 to 99 % of the probes of m30_00 (median 1.9 to 2.1): a clamped-cosine irradiance cannot exceed 1, a radiance projection of an ideal sky hemisphere gives 1.5 (my calculation: L0 amplitude 0.5, L1 amplitude 0.75 in units of the radiance). INFERRED: the stored SH is closer to a radiance projection than to a Lambert-convolved irradiance, and the `max(0, .)` in the engine clamps the side facing away to black; that is why "down-facing" medians are 0 for open probes.
- The meaning of `s12` and of the group flags +0x04 and +0x94 are not documented (INFERRED for `s12`, see headline 3).
- SoulsFormats lacks BTPB: my parser reproduces the engine's documented layout (strides, relocation base 0x3C, group count/size relations), so I regard the layout as OBSERVED; the field meanings follow the engine documents.

### 1.6 One probe decoded by hand (m30_00 bank 0000, nearest probe to the High Wall start bonfire at (-16.7, 41.2, -13.1))

The probe is index k = 2400 of group 35 (26 x 6 x 24 cells, 1 m, min (-24.5184, 39.4227, -28.2490), group offset 3,811,680), at byte 0x3C + 3,811,680 + 2400 x 72 = 3,984,540 of the expanded file. Raw 40 bytes:

`76 01 93 01 21 02 | 91 01 b1 01 48 02 | e7 ff e5 ff db ff | ff ff ff ff ff ff | 0c 02 | 00 00 | af 25 84 c1 d8 b0 25 42 e7 fb 53 c1`

- s0..s2 (L0): 0x0176 = 374, 0x0193 = 403, 0x0221 = 545. L0 = 374 x 2.7548e-4 = **0.10303**, 403 x ... = **0.11102**, 545 x ... = **0.15014**. luma = 0.2126 x 0.10303 + 0.7152 x 0.11102 + 0.0722 x 0.15014 = 0.02190 + 0.07940 + 0.01084 = **0.11214**.
- s3..s5 (L1 y): 0x0191 = 401, 0x01B1 = 433, 0x0248 = 584; times 4.7715e-4 = 0.19134, 0.20661, 0.27866.
- s6..s8 (L1 z): 0xFFE7 = -25, 0xFFE5 = -27, 0xFFDB = -37; times k1 = -0.01193, -0.01288, -0.01765.
- s9..s11 (L1 x): all 0xFFFF = -1: -0.00048 each.
- s12 = 0x020C = 524, so s12/1024 = 0.512 (an "open" probe). The priority short at +0x1A is 0.
- Position: f32 words 0xC18425AF, 0x4225B0D8, 0xC153FBE7 (bytes `af 25 84 c1 | d8 b0 25 42 | e7 fb 53 c1`) = (-16.5184, 41.4227, -13.2490); grid check: ix = 2400 mod 26 = 8, iy = (2400/26 = 92) mod 6 = 2, iz = 15: min + (8, 2, 15) x 1 = (-16.5184, 41.4227, -13.2490). Matches.
- Up-facing = L0 + k1 s3 = 0.10303 + 0.19134 = **0.29437** (G 0.31763, B 0.42879); down-facing = 0.10303 - 0.19134 < 0 so clamped to **0**. R/B = 374/545 = 0.686 (cold). |V|/L0 = 1.83 (V is almost purely +y: (-0.0005, 0.2055, -0.0127) in luma weighting, direction y = 1.00). The script's output for the same probe (nearest to the bonfire, 0.32 m): L0 0.1030, 0.1110, 0.1501; luma 0.1121, up 0.2944, 0.3176, 0.4288: same numbers.

## 2. Per bank

### 2.1 Structure

| bank | version | groups | probes | all-zero probes | probe bounds x | y | z | cell sizes (groups) |
|---|---:|---:|---:|---:|---|---|---|---|
| dlc1_m45_00_0000 | 3 | 967 | 13888 | 1105 (8 %) | -253..241 | -97..37 | -450..51 | 2x1x2 (801), 2x1.5x2 (111), 2 (22), 4x4x2 (22), 1.5 (10), 2x3x2 (1) |
| dlc2_m50_00_0000 | 3 | 569 | 16147 | 2802 (17 %) | -101..260 | -342..216 | -76..268 | 2x1x2 (521), 2 (32), 5x1.5x5 (9), 4 (3), 2x4x2 (3), 4x5x4 (1) |
| dlc2_m51_00_0000 | 3 | 727 | 101220 | 9238 (9 %) | -232..137 | -272..13 | -47..357 | 2x1x2 (658), 1x1x2 (30), 2 (17), 2x1x1 (12), 1 (4), 1.5x1x1.5 (3), 2x5x2 (1), 2x4x2 (1), 5 (1) |
| dlc2_m51_01_0000 | 3 | 768 | 70343 | 531 (1 %) | 17..402 | -26..30 | -168..233 | 2x1x2 (757), 4x2x4 (10), 5x2x10 (1) |
| m30_00_0000 | 3 | 72 | 91768 | 26575 (29 %) | -148..82 | -55..55 | -64..503 | 2 (51), 2x3x2 (8), 1 (8), 5 (1), 1.5 (1), 0.5 (1), 1x1x2 (1), 2x2x2.1 (1) |
| m30_00_0001 | 3 | 72 | 91768 | 26906 (29 %) | -148..82 | -55..55 | -64..503 | 2 (51), 2x3x2 (8), 1 (8), 5 (1), 1.5 (1), 0.5 (1), 1x1x2 (1), 2x2x2.1 (1) |
| m30_01_0000 | 3 | 27 | 66417 | 17832 (27 %) | -92..102 | -23..148 | 90..296 | 2 (26), 1 (1) |
| m30_01_0001 | 3 | 22 | 54579 | 12201 (22 %) | -92..94 | 9..128 | 90..296 | 2 (22) |
| m31_00_0000 | 3 | 37 | 434291 | 86061 (20 %) | -279..39 | -87..48 | -105..328 | 2 (19), 1.5 (10), 1 (7), 5 (1) |
| m32_00_0000 | 3 | 33 | 329635 | 75777 (23 %) | -134..171 | -24..116 | 51..506 | 2 (32), 1x1x2 (1) |
| m33_00_0000 | 3 | 27 | 73700 | 3956 (5 %) | -329..133 | -42..31 | -656..-47 | 3 (20), 2 (3), 3x4x3 (2), 2x4x2 (1), 4 (1) |
| m34_01_0000 | 3 | 29 | 51229 | 13556 (26 %) | -246..84 | 40..164 | 126..368 | 2 (25), 2x2x4 (4) |
| m34_01_0001 | 3 | 23 | 40968 | 12884 (31 %) | -246..-101 | 40..164 | 126..274 | 2 (19), 2x2x4 (4) |
| m35_00_0000 | 3 | 36 | 182999 | 117187 (64 %) | -110..166 | -46..52 | -206..182 | 2 (35), 2x3x3 (1) |
| m37_00_0000 | 3 | 806 | 91027 | 13924 (15 %) | -365..210 | -60..150 | -187..335 | 1x4x1 (410), 1x2x1 (167), 2 (86), 1x3x1 (55), 1 (32), 5 (21), 1x1.5x1 (11), 2x1x2 (10), 4x2x4 (4), 1x4x2 (3), 2x4x2 (2), 2x3x2 (1), 5x4x5 (1), 5x5x2 (1), 4 (1), 3x2x3 (1) |
| m39_00_0000 | 3 | 62 | 87345 | 64557 (74 %) | -74..334 | -112..48 | -170..110 | 2 (62) |
| m39_00_0001 | 2 | 20 | 2329 | 718 (31 %) | -71..79 | -30..57 | -145..66 | 10x2x10 (6), 2x1.5x3 (6), 1x1.5x3 (5), 1.5x3.3x4.8 (2), 3.5x2x3 (1) |
| m40_00_0000 | 3 | 9 | 144974 | 18168 (13 %) | -74..185 | -80..24 | 426..792 | 2 (9) |
| m41_00_0000 | 3 | 3 | 72730 | 6437 (9 %) | -218..-74 | 53..104 | -325..136 | 2 (3) |
| m46_00_0000 | 3 | 108 | 10395 | 203 (2 %) | -238..-88 | 138..165 | 158..313 | 2x1x2 (100), 2 (8) |
| m53_00_0000 | 3 | 7 | 3864 | 884 (23 %) | -42..25 | -6..17 | -22..27 | 2 (7) |
| m54_00_0000 | 3 | 31 | 418 | 1 (0 %) | -16..15 | 0..2 | -16..14 | 2x1x2 (31) |

The bank AABB in the header is the union of the group AABBs with z negated and swapped (checked on every bank; the probe bounds in this table are inside it by up to 10 m); the per-group volumes are listed in `C:/Users/User/Tools/DS3-Decompilation/study-ws12/btpb/out/groups-all-banks.tsv`.

### 2.2 Ambient distribution (valid probes only; L0 luma in the engine's `aSh` units)

| bank | valid | luma min | p10 | median | p90 | max | warm / neutral / cold | mean L0 R, G, B | R/B p10 / median / p90 | L1 up / down / side | median abs(L1)/L0 | share with abs(L1) > L0 |
|---|---:|---:|---:|---:|---:|---:|---|---|---|---|---:|---:|
| dlc1_m45_00_0000 | 12783 | 0.0000 | 0.0068 | 0.0281 | 0.0511 | 1.0980 | 0 / 0 / 100 % | 0.0085, 0.0327, 0.0538 | 0.15 / 0.15 / 0.16 | 90 / 1 / 9 % | 2.20 | 99 % |
| dlc2_m50_00_0000 | 13345 | 0.0000 | 0.0014 | 0.0234 | 0.0446 | 0.0671 | 0 / 0 / 100 % | 0.0211, 0.0228, 0.0314 | 0.66 / 0.67 / 0.68 | 88 / 1 / 11 % | 2.11 | 100 % |
| dlc2_m51_00_0000 | 91982 | 0.0000 | 0.0195 | 0.0708 | 0.0819 | 0.0915 | 0 / 100 / 0 % | 0.0631, 0.0620, 0.0588 | 1.07 / 1.07 / 1.08 | 22 / 69 / 8 % | 1.82 | 100 % |
| dlc2_m51_01_0000 | 69812 | 0.0000 | 0.0270 | 0.0355 | 0.0410 | 0.0435 | 0 / 100 / 0 % | 0.0343, 0.0343, 0.0343 | 1.00 / 1.00 / 1.00 | 83 / 16 / 0 % | 1.70 | 100 % |
| m30_00_0000 | 65193 | 0.0000 | 0.0011 | 0.0462 | 0.1316 | 2.5909 | 4 / 2 / 93 % | 0.0636, 0.0651, 0.0809 | 0.66 / 0.69 / 0.75 | 53 / 35 / 11 % | 1.91 | 98 % |
| m30_00_0001 | 64862 | 0.0000 | 0.0011 | 0.0442 | 0.1260 | 0.4900 | 2 / 1 / 96 % | 0.0495, 0.0534, 0.0719 | 0.65 / 0.69 / 0.69 | 55 / 35 / 10 % | 1.17 | 98 % |
| m30_01_0000 | 48585 | 0.0000 | 0.0008 | 0.0364 | 0.1698 | 0.2323 | 0 / 1 / 97 % | 0.0607, 0.0670, 0.0886 | 0.67 / 0.68 / 0.69 | 51 / 30 / 19 % | 0.22 | 95 % |
| m30_01_0001 | 42378 | 0.0000 | 0.0008 | 0.0942 | 0.2128 | 0.2780 | 0 / 1 / 97 % | 0.0891, 0.0979, 0.1300 | 0.67 / 0.69 / 0.69 | 52 / 19 / 29 % | 1.65 | 88 % |
| m31_00_0000 | 348230 | 0.0000 | 0.0071 | 0.0626 | 0.1147 | 0.1911 | 0 / 1 / 99 % | 0.0504, 0.0630, 0.0863 | 0.58 / 0.58 / 0.59 | 63 / 28 / 9 % | 1.79 | 98 % |
| m32_00_0000 | 253858 | 0.0000 | 0.0048 | 0.1401 | 0.2116 | 0.2474 | 0 / 0 / 100 % | 0.1017, 0.1288, 0.1655 | 0.61 / 0.61 / 0.62 | 82 / 6 / 12 % | 0.84 | 86 % |
| m33_00_0000 | 69744 | 0.0000 | 0.0103 | 0.0676 | 0.1249 | 0.2328 | 0 / 1 / 99 % | 0.0623, 0.0688, 0.1008 | 0.59 / 0.60 / 0.67 | 41 / 46 / 12 % | 1.34 | 89 % |
| m34_01_0000 | 37673 | 0.0000 | 0.0003 | 0.0254 | 0.1961 | 0.2374 | 0 / 5 / 93 % | 0.0622, 0.0670, 0.0929 | 0.60 / 0.67 / 0.69 | 47 / 35 / 18 % | 2.91 | 87 % |
| m34_01_0001 | 28084 | 0.0000 | 0.0008 | 0.0504 | 0.1658 | 0.2226 | 0 / 0 / 100 % | 0.0614, 0.0663, 0.0895 | 0.64 / 0.69 / 0.69 | 50 / 30 / 21 % | 2.26 | 99 % |
| m35_00_0000 | 65812 | 0.0000 | 0.0006 | 0.0276 | 0.0798 | 0.1058 | 0 / 8 / 92 % | 0.0307, 0.0340, 0.0402 | 0.75 / 0.76 / 0.80 | 82 / 0 / 18 % | 1.80 | 100 % |
| m37_00_0000 | 77103 | 0.0000 | 0.0003 | 0.0483 | 0.1102 | 0.2931 | 0 / 0 / 100 % | 0.0308, 0.0561, 0.0763 | 0.00 / 0.39 / 0.45 | 12 / 51 / 37 % | 2.45 | 57 % |
| m39_00_0000 | 22788 | 0.0000 | 0.0000 | 0.0011 | 0.0206 | 0.1317 | 0 / 12 / 83 % | 0.0061, 0.0067, 0.0082 | 0.00 / 0.75 / 1.00 | 22 / 20 / 58 % | 2.17 | 92 % |
| m39_00_0001 | 1611 | 0.0000 | 0.0003 | 0.0048 | 0.0836 | 6.7132 | 2 / 5 / 91 % | 0.0381, 0.0323, 0.0452 | 0.58 / 0.71 / 0.80 | 11 / 32 / 56 % | 2.82 | 91 % |
| m40_00_0000 | 126806 | 0.0000 | 0.0075 | 0.0789 | 0.1781 | 0.4365 | 0 / 1 / 99 % | 0.0791, 0.0886, 0.1210 | 0.61 / 0.65 / 0.69 | 42 / 30 / 28 % | 0.92 | 73 % |
| m41_00_0000 | 66293 | 0.0000 | 0.0060 | 0.1105 | 0.1775 | 0.2353 | 0 / 1 / 99 % | 0.0871, 0.0936, 0.1091 | 0.76 / 0.80 / 0.85 | 49 / 24 / 27 % | 1.87 | 58 % |
| m46_00_0000 | 10192 | 0.0003 | 0.1340 | 0.2343 | 0.2394 | 0.2424 | 0 / 0 / 100 % | 0.1972, 0.2128, 0.2929 | 0.67 / 0.67 / 0.67 | 16 / 0 / 84 % | 0.17 | 13 % |
| m53_00_0000 | 2980 | 0.0003 | 0.0613 | 0.1529 | 0.2000 | 0.2076 | 0 / 0 / 100 % | 0.1144, 0.1449, 0.1861 | 0.61 / 0.61 / 0.62 | 99 / 0 / 1 % | 1.47 | 100 % |
| m54_00_0000 | 417 | 0.0146 | 0.0201 | 0.0220 | 0.0246 | 0.0348 | 0 / 0 / 100 % | 0.0068, 0.0233, 0.0558 | 0.12 / 0.12 / 0.12 | 100 / 0 / 0 % | 1.50 | 99 % |

Notes (OBSERVED unless marked): the minimum luma is 0 almost everywhere (probes with a black L0 but a non-zero L1 exist: 248 in m30_00_0000). The m39_00_0001 maximum (6.7, luma) comes from a handful of probes with very large s values; its open statistics are not meaningful (3 probes with s12 >= 0.5). R/B p10/p90 hides outliers where p10 is 0 (probes with B = 0). The full per-bank table including open/closed means is `C:/Users/User/Tools/DS3-Decompilation/study-ws12/btpb/out/banks-summary.tsv`.

## 3. By place

### m40_00 by authored openness (s12) and by column position

| class | valid probes | median luma(L0) | median luma(up-facing) | mean L0 colour (R/G/B over luma) | mean R/B | warm / neutral / cold | L1 up share | mean L1 dir y |
|---|---:|---:|---:|---|---:|---|---:|---:|
| open sky (s12/1024 >= 0.5) | 52121 | 0.1636 | 0.2652 | 0.893 / 0.997 / 1.347 | 0.66 | 0% / 0% / 100% | 72% | 0.62 |
| partial (0 < s12/1024 < 0.5) | 4009 | 0.0911 | 0.1792 | 0.889 / 0.996 / 1.366 | 0.65 | 0% / 0% / 100% | 81% | 0.99 |
| closed (s12 == 0) | 70676 | 0.0300 | 0.0000 | 0.881 / 0.995 / 1.403 | 0.63 | 0% / 1% / 98% | 18% | -0.82 |
| all valid probes | 126806 | 0.0789 | 0.0983 | 0.890 / 0.996 / 1.361 | 0.65 | 0% / 1% / 99% | 42% | 0.57 |
| column-top probes (secondary proxy) | 23715 | 0.0976 | 0.1843 | 0.890 / 0.996 / 1.358 | 0.66 | 0% / 0% / 100% | 63% | 0.72 |
| non-top probes | 103091 | 0.0773 | 0.0880 | 0.890 / 0.996 / 1.362 | 0.65 | 0% / 1% / 98% | 37% | 0.52 |

#### m40_00: distance from each bonfire return point to the nearest valid probe (region names read from the MSB; English glosses are mine, INFERRED)

| bonfire (region name gloss) | position | nearest valid probe m | its luma L0 | its up-facing RGB | its s12/1024 |
|---|---|---:|---:|---|---:|
| Iudex Gundyr defeat bonfire (two regions, same position) | 140.9, -68.9, 593.6 | 62.1 | 0.1355 | 0.1862, 0.2094, 0.2896 | 0.52 |
| tutorial bonfire (Cemetery of Ash) | 103.5, -49.6, 507.0 | 39.5 | 0.1827 | 0.2328, 0.2603, 0.3534 | 1.00 |
| tutorial bonfire, back | -11.3, -62.6, 519.3 | 0.9 | 0.0296 | 0.0000, 0.0000, 0.0000 | 0.00 |
| Firelink Shrine bonfire (base bonfire) | 273.2, -56.2, 606.3 | 124.3 | 0.1769 | 0.1955, 0.2173, 0.2874 | 1.00 |

The probe positions of the bank span x -74..185, y -80..24, z 426..792; the Firelink bonfire (273.2, -56.2, 606.3), the Fire Keeper (261.0, -55.2, 600.6) and the shrine region cylinder (271.1, -56.2, 605.9) lie 123 m from the nearest probe.

### m40_00 volumes (the 9 groups)

| group | grid cells | cell m | probes (valid) | AABB min | AABB max | yaw rad | median luma L0 | mean L0 RGB | L1 up share | mean L1 dir y |
|---:|---|---:|---:|---|---|---:|---:|---|---:|---:|
| 0 | 55x7x74 | 2 | 28490 (26957) | -74,-69,426 | 38,-56,575 | -0.09 | 0.0491 | 0.0443, 0.0500, 0.0702 | 24% | -0.89 |
| 1 | 19x8x27 | 2 | 4104 (3664) | 33,-65,526 | 71,-48,582 | -0.09 | 0.1572 | 0.0987, 0.1100, 0.1475 | 23% | 0.43 |
| 2 | 20x13x32 | 2 | 8320 (7780) | 15,-76,568 | 55,-50,632 | -1.48 | 0.0456 | 0.0791, 0.0882, 0.1189 | 41% | 0.52 |
| 3 | 43x12x29 | 2 | 14964 (14045) | 3,-80,624 | 90,-56,683 | -1.09 | 0.1460 | 0.1031, 0.1150, 0.1550 | 68% | 0.65 |
| 4 | 47x16x22 | 2 | 16544 (15291) | 44,-80,677 | 138,-49,720 | -0.00 | 0.0992 | 0.0808, 0.0907, 0.1248 | 55% | 0.91 |
| 5 | 38x24x34 | 2 | 31008 (25748) | 106,-76,688 | 183,-27,757 | -0.70 | 0.0881 | 0.0780, 0.0873, 0.1192 | 44% | 0.78 |
| 6 | 16x51x39 | 2 | 31824 (27494) | 155,-76,716 | 187,26,794 | -0.70 | 0.1304 | 0.1092, 0.1222, 0.1666 | 46% | 0.48 |
| 7 | 29x4x30 | 2 | 3480 (2953) | 103,-68,684 | 162,-59,744 | -0.70 | 0.0117 | 0.0219, 0.0245, 0.0338 | 10% | -0.44 |
| 8 | 26x12x20 | 2 | 6240 (2874) | 128,-59,711 | 182,-35,751 | -0.70 | 0.0216 | 0.0365, 0.0409, 0.0556 | 28% | 0.68 |

### m30_00 bank 0000 by height (y bands of 10 m)

| y band | valid probes | median luma(L0) | median luma(up-facing) | mean L0 colour (R/G/B over luma) | mean R/B | warm / neutral / cold | L1 up share | mean L1 dir y |
|---|---:|---:|---:|---|---:|---|---:|---:|
| -60..-50 | 826 | 0.1617 | 0.0000 | 0.918 / 0.990 / 1.338 | 0.69 | 0% / 0% / 100% | 0% | -1.00 |
| -50..-40 | 7180 | 0.0679 | 0.0000 | 0.918 / 0.990 / 1.338 | 0.69 | 0% / 2% / 98% | 31% | -1.00 |
| -40..-30 | 5486 | 0.0122 | 0.0126 | 0.918 / 0.990 / 1.340 | 0.69 | 0% / 3% / 97% | 73% | 0.34 |
| -30..-20 | 3117 | 0.0364 | 0.0000 | 0.918 / 0.990 / 1.338 | 0.69 | 0% / 0% / 100% | 47% | -0.95 |
| -20..-10 | 4495 | 0.0518 | 0.0000 | 0.974 / 0.986 / 1.212 | 0.80 | 5% / 4% / 91% | 34% | -0.85 |
| -10..0 | 8867 | 0.0369 | 0.0033 | 1.102 / 0.978 / 0.919 | 1.20 | 14% / 7% / 77% | 40% | 0.25 |
| 0..10 | 10221 | 0.0323 | 0.0215 | 0.911 / 0.992 / 1.344 | 0.68 | 1% / 1% / 98% | 48% | 0.86 |
| 10..20 | 4433 | 0.0203 | 0.0151 | 0.908 / 0.999 / 1.286 | 0.71 | 1% / 3% / 96% | 42% | 0.92 |
| 20..30 | 7076 | 0.0526 | 0.1511 | 0.929 / 0.990 / 1.307 | 0.71 | 11% / 1% / 87% | 67% | 0.99 |
| 30..40 | 6006 | 0.0794 | 0.2189 | 0.923 / 0.990 / 1.326 | 0.70 | 9% / 0% / 91% | 66% | 1.00 |
| 40..50 | 5373 | 0.1155 | 0.3122 | 0.918 / 0.990 / 1.338 | 0.69 | 0% / 0% / 100% | 85% | 1.00 |
| 50..60 | 2113 | 0.1372 | 0.3459 | 0.918 / 0.990 / 1.338 | 0.69 | 0% / 0% / 100% | 96% | 1.00 |
| open sky (s12/1024 >= 0.5) | 14741 | 0.1044 | 0.2854 | 0.918 / 0.991 / 1.335 | 0.69 | 0% / 0% / 100% | 96% | 0.99 |
| partial (0 < s12 < 0.5) | 5206 | 0.0186 | 0.0566 | 0.922 / 0.991 / 1.323 | 0.70 | 0% / 3% / 97% | 95% | 0.99 |
| closed (s12 == 0) | 45246 | 0.0314 | 0.0000 | 0.994 / 0.986 / 1.162 | 0.86 | 6% / 3% / 90% | 35% | -0.81 |
| column-top probes (secondary proxy) | 28697 | 0.0601 | 0.1462 | 0.979 / 0.986 / 1.197 | 0.82 | 3% / 2% / 94% | 75% | 0.98 |
| non-top probes | 36496 | 0.0381 | 0.0001 | 0.948 / 0.989 / 1.262 | 0.75 | 5% / 2% / 92% | 37% | -0.83 |
| all valid | 65193 | 0.0462 | 0.0226 | 0.965 / 0.987 / 1.227 | 0.79 | 4% / 2% / 93% | 53% | 0.90 |

#### m30_00 bank 0000: nearest valid probe to each bonfire return point (names: my English reading of the region name, INFERRED)

| bonfire (region name, my reading) | position | nearest probe m | L0 RGB | up-facing RGB | down-facing RGB | luma L0 | L1 dir | probes within 8 m: median luma L0 |
|---|---|---:|---|---|---|---:|---|---:|
| default (start) bonfire of the zone | -16.7, 41.2, -13.1 | 0.30 | 0.1030, 0.1110, 0.1501 | 0.2944, 0.3176, 0.4288 | 0.0000, 0.0000, 0.0000 | 0.1121 | -0.00, 1.00, -0.06 | 0.1119 (1087) |
| full-armour beast (boss) defeat bonfire | 26.7, -25.0, 6.0 | 22.91 | 0.0129, 0.0140, 0.0190 | 0.0497, 0.0537, 0.0729 | 0.0000, 0.0000, 0.0000 | 0.0142 | -0.01, 1.00, 0.01 | nan (0) |
| wall-beast tower bonfire (my reading: Tower on the Wall) | -113.5, 33.8, 66.7 | 0.73 | 0.0766, 0.0826, 0.1116 | 0.2226, 0.2406, 0.3244 | 0.0000, 0.0000, 0.0000 | 0.0834 | 0.18, 0.91, 0.37 | 0.0712 (265) |
| Pursuer defeat bonfire | 27.6, -9.8, 158.0 | 0.80 | 0.0887, 0.0956, 0.1292 | 0.0000, 0.0000, 0.0000 | 0.2662, 0.2869, 0.3878 | 0.0966 | 0.21, -0.97, -0.09 | 0.0005 (158) |
| Dragonslayer Armour defeat bonfire | -89.8, -48.6, 425.4 | 1.64 | 0.1375, 0.1482, 0.2003 | 0.0000, 0.0000, 0.0000 | 0.3589, 0.3868, 0.5224 | 0.1497 | 0.02, -1.00, 0.01 | 0.1485 (58) |
| Darksign-without-bonfire respawn | -21.4, 49.4, -54.8 | 9.15 | 0.0003, 0.0003, 0.0006 | 0.0000, 0.0000, 0.0000 | 0.0012, 0.0012, 0.0020 | 0.0003 | 0.00, -1.00, 0.00 | nan (0) |

#### m30_00 bank 0000: the 72 volumes sorted by probe count (top 20; full table in `out/groups-all-banks.tsv`)

| group | cells | cell m | valid probes | AABB min | AABB max | median luma L0 | mean L0 RGB | L1 up share | mean L1 dir y |
|---:|---|---:|---:|---|---|---:|---|---:|---:|
| 8 | 35x4x54 | 2 | 5497 | -83,-43,159 | -11,-34,267 | 0.0084 | 0.0269, 0.0290, 0.0393 | 60% | -0.98 |
| 24 | 23x7x28 | 2 | 4143 | 6,-15,76 | 52,-1,133 | 0.0690 | 0.2587, 0.2233, 0.1986 | 43% | 0.37 |
| 5 | 20x9x45 | 2 | 3578 | -105,-50,362 | -64,-32,452 | 0.0694 | 0.0698, 0.0753, 0.1018 | 33% | -0.99 |
| 29 | 23x6x28 | 2 | 3436 | 6,-27,29 | 52,-14,84 | 0.0417 | 0.0439, 0.0474, 0.0640 | 39% | -0.96 |
| 35 | 26x6x24 | 1 | 3333 | -25,39,-28 | 1,46,-4 | 0.1040 | 0.0731, 0.0788, 0.1065 | 73% | 1.00 |
| 50 | 21x10x17 | 0.5 | 3135 | -12,1,53 | -1,6,61 | 0.0675 | 0.0500, 0.0545, 0.0742 | 80% | 0.99 |
| 6 | 19x11x18 | 1.5 | 2299 | -147,24,2 | -119,39,29 | 0.0384 | 0.0506, 0.0522, 0.0675 | 46% | 0.98 |
| 53 | 16x7x19 | 2 | 2126 | 12,2,202 | 45,16,240 | 0.0619 | 0.0627, 0.0688, 0.0945 | 0% | -0.85 |
| 21 | 19x7x16 | 2 | 1991 | -41,-14,50 | -2,0,83 | 0.0587 | 0.0493, 0.0535, 0.0727 | 27% | -0.90 |
| 37 | 26x8x13 | 1 | 1979 | -4,24,-16 | 22,33,-3 | 0.0528 | 0.0434, 0.0468, 0.0632 | 77% | 0.96 |
| 19 | 31x14x10 | 2 | 1946 | -61,-3,85 | 0,26,106 | 0.0368 | 0.0393, 0.0421, 0.0563 | 33% | 0.11 |
| 51 | 23x7x21 | 2 | 1935 | 6,-11,130 | 52,3,173 | 0.0010 | 0.0229, 0.0250, 0.0329 | 5% | -0.97 |
| 10 | 15x12x14 | 2 | 1644 | -130,16,49 | -100,40,78 | 0.0137 | 0.0416, 0.0440, 0.0575 | 55% | 0.97 |
| 67 | 29x5x13 | 2 | 1572 | -92,-44,265 | -35,-35,292 | 0.0125 | 0.0349, 0.0376, 0.0508 | 66% | -0.98 |
| 40 | 20x8x12 | 1 | 1457 | -91,45,-43 | -71,53,-31 | 0.1350 | 0.1171, 0.1263, 0.1706 | 98% | 0.99 |
| 61 | 11x5x28 | 2 | 1455 | 51,5,147 | 74,16,203 | 0.0145 | 0.0132, 0.0144, 0.0197 | 30% | 0.12 |
| 17 | 19x10x9 | 2 | 1430 | -72,8,54 | -34,28,73 | 0.0909 | 0.0743, 0.0804, 0.1012 | 69% | 1.00 |
| 39 | 17x8x13 | 2 | 1214 | -92,28,-34 | -58,43,-7 | 0.0946 | 0.0708, 0.0764, 0.1032 | 62% | 0.98 |
| 34 | 19x5x12 | 1 | 1134 | -26,50,-37 | -7,55,-25 | 0.1355 | 0.1200, 0.1295, 0.1749 | 99% | 0.99 |
| 41 | 22x8x7 | 1 | 1080 | -115,21,9 | -93,29,16 | 0.0653 | 0.0597, 0.0645, 0.0871 | 100% | 0.96 |



The 10 m height bands above show the open-air probes at the top of the map (y 20..60: luma(L0) 0.05..0.14, up-facing 0.15..0.35, L1 up in 66 to 96 %) and the low probes (y -60..-10, mean L1 direction y -1.0 to -0.85) mostly lit from below. Bank 0001 of m30_00 gives the same picture (open median luma 0.1035 against 0.1044; closed 0.0308 against 0.0314; open up share 95.9 %); the two banks differ mainly in the high tail (max luma 2.59 in bank 0000 against 0.49) and in the warm pockets (the bands -20..-10 and -10..0 have 0 % and 1 % warm probes in bank 0001 against 5 % and 14 % in bank 0000). The 16 m clusters are in `C:/Users/User/Tools/DS3-Decompilation/study-ws12/btpb/out/clusters-m30_00_0000.tsv`.

### 3.1 The questions, with numbers

| | m30_00 (High Wall), bank 0000 | m40_00 (area covered by the bank) |
|---|---|---|
| open probes (s12/1024 >= 0.5), share of valid | 22.6 % (14741) | 41.1 % (52121) |
| median luma(L0): open / closed / ratio closed:open | 0.1044 / 0.0314 / 0.30 | 0.1636 / 0.0300 / 0.18 |
| median up-facing luma: open / closed | 0.2854 / 0.0000 | 0.2652 / 0.0000 |
| median down-facing luma: open / closed | 0.0000 / 0.0084 | 0.0766 / 0.0446 |
| mean L0 colour (R, G, B): open | 0.0901, 0.0973, 0.1312 | 0.1408, 0.1572, 0.2124 |
| mean L0 colour: closed | 0.0587, 0.0582, 0.0687 | 0.0337, 0.0381, 0.0537 |
| mean R/B of the means: open / closed | 0.687 / 0.855 | 0.663 / 0.628 |
| median R/B: open / closed | 0.686 / 0.686 | 0.662 / 0.623 |
| L1 up / side / down share: open | 96 / 4 / 0.3 % | 72 / 28 / 0.1 % |
| L1 up / side / down share: closed | 35 / 14 / 51 % | 18 / 28 / 54 % |
| mean L1 direction (x, y, z; mean of V, normalised): open | -0.04, 0.99, -0.11 | 0.78, 0.62, -0.09 |
| mean L1 direction: closed | -0.32, -0.81, -0.48 | -0.57, -0.82, -0.07 |

- **How much darker inside than outside (OBSERVED, by the s12 class; INFERRED that closed = covered):** covered probes have a median L0 luma 0.30 times the open ones in m30_00 (0.0314 against 0.1044) and 0.18 times in the covered part of m40_00 (0.0300 against 0.1636); in the up-facing term, which is what a standing character's top and a roof-facing surface receive, the closed median is 0 in both (the up-facing ambient of a covered probe is black in the median).
- **What colour:** the same blue-grey. Median R/B 0.686 closed and open in m30, 0.623 against 0.662 in m40; means R/B 0.85 closed against 0.69 open in m30 (closed probes include warm pockets), 0.63 against 0.66 in m40. Covered places are not warmer on average than open ones in these two banks; their chroma is as cold, only darker. The warm exceptions: m30_00 group 24 (y -15..-1, z 76..133; mean RGB 0.259, 0.223, 0.199), 4.5 % of the valid probes classify warm.
- **Does the open-air ambient come mainly from above?** Yes in m30: 96 % of open probes have L1 up (V.y/|V| > 0.5), 0.3 % down, mean direction (-0.04, 0.99, -0.11). In the covered part of m40 72 % up and 28 % side (mean direction (0.78, 0.62, -0.09), a lean toward +x), 0.1 % down. The ambient of open probes is also a half-space light: abs(L1)/L0 median 2.1 in m30 (1.6 in bank 0001) and 0.96 in m40, so the down-facing term is 0 in m30 and 0.077 in m40.
- **Firelink Shrine interior:** not answerable. No probe of the bank lies within 123 m of the shrine's bonfire, region 0 ("shrine interior" cylinder, radius 22.6 m) or the Fire Keeper. How I found the bonfire: the MSB region `復帰ポイント：拠点篝火` (my reading: "respawn point: base bonfire") is a SpawnPoint at (273.2, -56.2, 606.3), the player start `c0000_0001` is at the same place, the BGM region `拠点BGM再生領域0 拠点内部` ("base BGM region 0: base interior", a cylinder radius 22.6 m, height 19, position (271.1, -56.2, 605.9)) and the object `o400100_1000/1001` with `o000499_0022` sit at (271.1, -56.3, 605.7). The four objects `o000100_1000..1003` of `msb-m40.tsv` are NOT the shrine bonfire: they sit at the Cemetery of Ash (104.8, -49.7, 507.2), the back tutorial bonfire (-10.5, -62.2, 520.2) and the Gundyr bonfire (142.7, -68.9, 594.8, twice), next to player starts at the same places. Whether the original has a second bank (`m40_00_0001`, which exists for the lights) holding the shrine is not known: only `...0000.btpb` was recovered. INFERRED consequence: with no volume near it, a character at the shrine gets the light-set hemisphere (the "no record" branch of `FUN_140061950`); the hemisphere of m40_00 is mostly intensity 0 (section 4).

## 4. Against the hemisphere colours

Engine formula (OBSERVED, TONEMAP-SKIN s10, `FUN_1401495F0`): `U = rgb(+0x220)^2 * (+0x22C)` for Hemi Color Up and `D = rgb(+0x230)^2 * (+0x23C)` for Hemi Color Down: the stored colour is the first three floats of the type-14 value (four float32), squared, times the **fourth float** (the intensity, which sits inside the same value). The SH rows are `row_c = ((U_c - D_c)/2 * axis, (U_c + D_c)/2)`, with axis (0, 1, 0) by default; so ambient(up) = U, ambient(down) = D, exactly comparable with the probes' up-facing and down-facing terms. Stored values below are decoded from the hex of the gparam tsv (3F000000 = 0.5, 3F800000 = 1.0).

**m30_00** LightSet Hemi Color rows (43 ids, 6 distinct up/down pairs); stored (r, g, b, intensity) then U and D after the formula:

| ids | Hemi Up stored | Hemi Down stored | U (up-facing) | D (down-facing) | luma U / D |
|---|---|---|---|---|---|
| 0, 2, 3, 4, 10, 11, 12, 13, 23, 24, 25, 26, 27, 28, 29, 30, 120, 121, 122, 130, 131, 133, 134, 135, 136, 137, 138, 150, 151, 152, 200, 300, 310, 320 | 0.5, 0.5, 0.5, 1 | 0.5, 0.5, 0.5, 1 | 0.2500, 0.2500, 0.2500 | 0.2500, 0.2500, 0.2500 | 0.250 / 0.250 |
| 1 | 0.5, 0.5, 0.5, 1 | 0.5, 0.5, 0.5, 1.3296 | 0.2500, 0.2500, 0.2500 | 0.3324, 0.3324, 0.3324 | 0.250 / 0.332 |
| 20, 21, 22 | 0.5, 0.5, 0.5, 0 | 0.5, 0.5, 0.5, 1 | 0.0000, 0.0000, 0.0000 | 0.2500, 0.2500, 0.2500 | 0.000 / 0.250 |
| 50, 51, 123 | 0, 0, 0, 1 | 0, 0, 0, 1 | 0.0000, 0.0000, 0.0000 | 0.0000, 0.0000, 0.0000 | 0.000 / 0.000 |
| 100 | 0, 0, 0, 0 | 0, 0, 0, 0 | 0.0000, 0.0000, 0.0000 | 0.0000, 0.0000, 0.0000 | 0.000 / 0.000 |
| 132 | 0.5, 0.5, 0.5, 0 | 0.5, 0.5, 0.5, 0 | 0.0000, 0.0000, 0.0000 | 0.0000, 0.0000, 0.0000 | 0.000 / 0.000 |

**m40_00** LightSet Hemi Color rows (15 ids, 5 distinct up/down pairs); stored (r, g, b, intensity) then U and D after the formula:

| ids | Hemi Up stored | Hemi Down stored | U (up-facing) | D (down-facing) | luma U / D |
|---|---|---|---|---|---|
| 0, 20, 22, 30, 31, 32, 40, 100, 110, 111, 310 | 1, 1, 1, 0 | 1, 1, 1, 0 | 0.0000, 0.0000, 0.0000 | 0.0000, 0.0000, 0.0000 | 0.000 / 0.000 |
| 120 | 1, 0.8958, 0.7775, 0 | 1, 1, 1, 0 | 0.0000, 0.0000, 0.0000 | 0.0000, 0.0000, 0.0000 | 0.000 / 0.000 |
| 121 | 1, 0.9042, 0.8282, 0.5 | 1, 0.9099, 0.8423, 0.5 | 0.5000, 0.4088, 0.3430 | 0.5000, 0.4140, 0.3547 | 0.423 / 0.428 |
| 122 | 1, 1, 1, 0.3 | 1, 1, 1, 0 | 0.3000, 0.3000, 0.3000 | 0.0000, 0.0000, 0.0000 | 0.300 / 0.000 |
| 300 | 0.5, 0.5, 0.5, 1 | 0.5, 0.5, 0.5, 1 | 0.2500, 0.2500, 0.2500 | 0.2500, 0.2500, 0.2500 | 0.250 / 0.250 |

Probe medians (open = s12/1024 >= 0.5; per channel median of the clamped values):

| | up-facing R, G, B | up luma | down-facing R, G, B | down luma | n |
|---|---|---:|---|---:|---:|
| m30_00 bank 0000 open | 0.2620, 0.2826, 0.3816 | 0.2854 | 0.0000, 0.0000, 0.0000 | 0.0000 | 14741 |
| m40_00 bank 0000 open | 0.2354, 0.2642, 0.3633 | 0.2652 | 0.0692, 0.0764, 0.0998 | 0.0766 | 52121 |
| m30_00 bank 0000 closed | 0.0000, 0.0000, 0.0000 | 0.0000 | 0.0077, 0.0083, 0.0094 | 0.0084 | 45246 |
| m40_00 bank 0000 closed | 0.0000, 0.0000, 0.0000 | 0.0000 | 0.0394, 0.0444, 0.0621 | 0.0446 | 70676 |

Open-air up-facing p10..p90 per channel in m30_00: R 0.109..0.326, G 0.118..0.352, B 0.160..0.475.

Reading (OBSERVED numbers, INFERRED comparison):

- m30_00: the authored hemisphere is the same for 34 of 43 ids, a neutral grey U = D = 0.25 (stored 0.5 grey, intensity 1). The open probes give up-facing 0.262, 0.283, 0.382 (luma 0.285, 1.14 times the hemisphere luma 0.25) and a down-facing of 0: the baked ambient is bluer (R/B 0.69 against 1.0) and one-sided, where the hemisphere is isotropic. The level is of the same order (0.25 against 0.285 up); the baked probes keep the sky above and no light from below.
- m40_00: 12 of 15 ids give a black hemisphere (intensity 0: 11 ids incl. id 0, plus id 120 whose warm colour has intensity 0), so a character without a probe record would get a black ambient from the hemisphere; id 121 is a warm 0.5 (U 0.500, 0.409, 0.343; D 0.500, 0.414, 0.355), id 122 a white 0.3 up and 0 down, id 300 grey 0.25 both. The open probes give up-facing 0.235, 0.264, 0.363 (luma 0.265) and down-facing 0.069, 0.076, 0.100 (luma 0.077): the same grey-0.25 level as id 300 in up-facing, with a blue tint and a dark floor. Which LightSet id applies where is not read here (the id is picked by the zone's runtime state).
- The hemisphere's fourth float is also the product factor in the engine; "stored values" and "product" coincide here because the factor is part of the stored value (nothing else sits beside it in these rows).

## 5. Whole game: one row per bank, darkest to brightest authored ambient (median luma of L0 over valid probes)

Zone names: only those the repository states (High Wall m30_00, Undead Settlement m31_00, DLC names); the other map ids are left as ids. "mean colour" = mean L0 (R, G, B) normalised to luma 1.

| rank | bank | zone | valid probes | median luma | open share | open median luma | closed median luma | mean colour (R, G, B over luma) | warm / cold | L1 up |
|---:|---|---|---:|---:|---:|---:|---:|---|---|---:|
| 1 | m39_00_0000 |  | 22788 | 0.0011 | 7 % | 0.0339 | 0.0008 | 0.91, 1.00, 1.22 | 0 % / 83 % | 22 % |
| 2 | m39_00_0001 |  | 1611 | 0.0048 | 0 % | 2.3629 | 0.0048 | 1.11, 0.94, 1.31 | 2 % / 91 % | 11 % |
| 3 | m54_00_0000 |  | 417 | 0.0220 | 99 % | 0.0220 | 0.0172 | 0.31, 1.05, 2.52 | 0 % / 100 % | 100 % |
| 4 | dlc2_m50_00_0000 | Dreg Heap (DLC2) | 13345 | 0.0234 | 50 % | 0.0324 | 0.0059 | 0.92, 0.99, 1.36 | 0 % / 100 % | 88 % |
| 5 | m34_01_0000 |  | 37673 | 0.0254 | 28 % | 0.1839 | 0.0062 | 0.92, 0.99, 1.37 | 0 % / 93 % | 47 % |
| 6 | m35_00_0000 |  | 65812 | 0.0276 | 41 % | 0.0629 | 0.0046 | 0.91, 1.01, 1.19 | 0 % / 92 % | 82 % |
| 7 | dlc1_m45_00_0000 | Ariandel (DLC1) | 12783 | 0.0281 | 57 % | 0.0345 | 0.0135 | 0.29, 1.12, 1.85 | 0 % / 100 % | 90 % |
| 8 | dlc2_m51_01_0000 | Ringed City (DLC2) | 69812 | 0.0355 | 70 % | 0.0369 | 0.0317 | 1.00, 1.00, 1.00 | 0 % / 0 % | 83 % |
| 9 | m30_01_0000 |  | 48585 | 0.0364 | 28 % | 0.1597 | 0.0158 | 0.90, 1.00, 1.32 | 0 % / 97 % | 51 % |
| 10 | m30_00_0001 | High Wall of Lothric | 64862 | 0.0442 | 23 % | 0.1035 | 0.0308 | 0.92, 0.99, 1.33 | 2 % / 96 % | 55 % |
| 11 | m30_00_0000 | High Wall of Lothric | 65193 | 0.0462 | 23 % | 0.1044 | 0.0314 | 0.97, 0.99, 1.23 | 4 % / 93 % | 53 % |
| 12 | m37_00_0000 |  | 77103 | 0.0483 | 43 % | 0.0910 | 0.0051 | 0.59, 1.08, 1.46 | 0 % / 100 % | 12 % |
| 13 | m34_01_0001 |  | 28084 | 0.0504 | 17 % | 0.1554 | 0.0220 | 0.92, 0.99, 1.34 | 0 % / 100 % | 50 % |
| 14 | m31_00_0000 | Undead Settlement | 348230 | 0.0626 | 38 % | 0.0922 | 0.0374 | 0.81, 1.02, 1.39 | 0 % / 99 % | 63 % |
| 15 | m33_00_0000 |  | 69744 | 0.0676 | 21 % | 0.1147 | 0.0529 | 0.89, 0.99, 1.45 | 0 % / 99 % | 41 % |
| 16 | dlc2_m51_00_0000 | Ringed City (DLC2) | 91982 | 0.0708 | 14 % | 0.0242 | 0.0730 | 1.02, 1.00, 0.95 | 0 % / 0 % | 22 % |
| 17 | m40_00_0000 | Firelink map, west part (not the shrine) | 126806 | 0.0789 | 41 % | 0.1636 | 0.0300 | 0.89, 1.00, 1.36 | 0 % / 99 % | 42 % |
| 18 | m30_01_0001 |  | 42378 | 0.0942 | 39 % | 0.1891 | 0.0197 | 0.91, 1.00, 1.32 | 0 % / 97 % | 52 % |
| 19 | m41_00_0000 |  | 66293 | 0.1105 | 51 % | 0.1516 | 0.0149 | 0.93, 1.00, 1.17 | 0 % / 99 % | 49 % |
| 20 | m32_00_0000 |  | 253858 | 0.1401 | 63 % | 0.1772 | 0.0433 | 0.81, 1.02, 1.32 | 0 % / 100 % | 82 % |
| 21 | m53_00_0000 |  | 2980 | 0.1529 | 77 % | 0.1764 | 0.0755 | 0.81, 1.02, 1.32 | 0 % / 100 % | 99 % |
| 22 | m46_00_0000 |  | 10192 | 0.2343 | 94 % | 0.2352 | 0.1123 | 0.92, 0.99, 1.36 | 0 % / 100 % | 16 % |

Observations (OBSERVED): the median luma spans 0.0011 (m39_00_0000, 74 % all-zero probes, 6.6 % open) to 0.2343 (m46_00_0000, 94.5 % open). By the open-probe median (banks with at least 50 open probes) the three darkest are m54_00_0000 0.0220, dlc2_m51_00_0000 0.0242, dlc2_m50_00_0000 0.0324 and the three brightest m34_01_0000 0.1839, m30_01_0001 0.1891, m46_00_0000 0.2352. The ranking by overall median mixes brightness with the open share; compare zones by the open or closed column when the question is the sky level. Colour: dlc2_m51_00 (median R/B 1.07) and dlc2_m51_01 (R = G = B exactly: one grey tint for the whole bank) are not blue; dlc1_m45_00 is a deep teal (median R/B 0.154, mean RGB 0.0085, 0.0327, 0.0538).

Rows 2 and 3 of the table (m39_00_0001, 3 open probes; m54_00_0000, 417 probes in 31 small volumes of a few metres) rest on very few probes.

## 6. Checks

1. **Hand-decoded probe**: section 1.6 (raw bytes to L0, L1, s12, position and grid index, equal to the script's numbers).
2. **File-size and relation checks, 22/22 banks**: `0x3C + 72 x probes + groups x stride = file size`; per group `count = nx*ny*nz`; group offsets consecutive; header probe bytes = 72 x total probes; header AABB = union of the group AABBs with z negated and swapped (deviation 0.0 in 22/22).
3. **Statistics recomputed a second way** (`checks.py`, from the raw int16 columns of the per-probe TSV, with `statistics.median` and integer ratios instead of the float records):

| statistic | bank | first way (`analyze.py`) | second way (raw shorts) |
|---|---|---:|---:|
| valid probes | m30_00_0000 | 65193 | 65193 |
| median luma(L0) | m30_00_0000 | 0.046181 | 0.046181 |
| cold share (R/B < 0.87; s0*1.15 < s2) | m30_00_0000 | 0.9288 | 0.9288 |
| warm share | m30_00_0000 | 0.0445 | 0.0445 |
| open share (s12 >= 512) | m30_00_0000 | 0.2261 | 0.2261 |
| open median luma | m30_00_0000 | 0.104350 | 0.104350 |
| L1 up share | m30_00_0000 | 0.5347 | 0.5343 |
| valid probes | m40_00_0000 | 126806 | 126806 |
| median luma(L0) | m40_00_0000 | 0.078940 | 0.078940 |
| cold share (R/B < 0.87; s0*1.15 < s2) | m40_00_0000 | 0.9866 | 0.9866 |
| warm share | m40_00_0000 | 0.0000 | 0.0000 |
| open share (s12 >= 512) | m40_00_0000 | 0.4110 | 0.4110 |
| open median luma | m40_00_0000 | 0.163582 | 0.163582 |
| L1 up share | m40_00_0000 | 0.4205 | 0.4202 |

The only difference (L1 up share 0.5347 against 0.5343 in m30_00, 0.4205 against 0.4202 in m40_00) comes from probes with a black L0 but a non-zero L1 (248 in m30_00_0000, 26 of them "up"): the first way classifies their direction, the second treats them as flat (L0 = 0) by its definition.
4. **Sign check of the L1 convention**: if the sign were reversed the open probes would point down; instead 96 % (m30) and 72 % (m40) of the probes with a strong sky weight point up. The y axis is the map's up axis (probe y equals the objects' y: bonfire (-16.7, 41.2, -13.1) against the nearest probe 0.32 m away).
5. **Frame check**: the nearest valid probe to the m30 bonfire return points is 0.3 m (start), 0.73 m (tower), 0.80 m (Pursuer), 1.64 m (Dragonslayer Armour); in m40 0.9 m for the back tutorial bonfire and 1.1 to 1.3 m for the player starts at (0.8, -58.4, 560.2) and (-0.5, -57.5, 566.6): the probe frame is the MSB frame. The Cemetery of Ash tutorial bonfire is 39.5 m, the Gundyr bonfire 62 m and the Firelink bonfire 124 m from the nearest probe.

## 7. What I could not do

- Answer the Firelink Shrine interior-against-open-air question from the bank (no probe within 123 m); the recovered set has no second m40_00 bank.
- Confirm the meaning of `s12`, the +0x04 and +0x94 flags, the group yaw (the stored positions ignore it) and whether the map transform rotates the L1 vectors in these maps.
- Name the zones beyond the repository's own names; map ids only.
- Compare against the original's lit frames (not in scope: no live run).
