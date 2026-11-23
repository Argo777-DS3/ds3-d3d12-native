# I-0138 STUDY: what YEBIS does in DS3, what Elden Ring does instead, and where the native chain stands

2026-11-12, the workstream 11 pilot (Fable 5.2, high effort). Read-only: no build, no live run, no game launched. Sources are
named at each claim. Tags: **OBSERVED** (read by the pilot in a file, a capture or the code), **WORKER** (read by a Sonnet
worker, its report linked, spot-checked by the pilot where said), **INFERRED** (reasoned, basis named), **TO CHECK** (needs a
run or a capture). The plan drawn from this study is [PLAN](PLAN.md) (French, for Thomas).

Annexes made for this study: [CENSUS-WORLD](CENSUS-WORLD.md) (every post-processing GPARAM value over the 100 retail map
files), [CENSUS-CUTSCENES](CENSUS-CUTSCENES.md) (the 49 cutscene files), [ER-POST-CHAIN](ER-POST-CHAIN.md) and
[ER-TONEMAP-HDR](ER-TONEMAP-HDR.md) (Elden Ring 2.7.1, static), and [ER-VALUES](ER-VALUES.md) (Elden Ring's shipped values and
tone-map programmes, read from its data files after the plan, on Thomas's agreement).

## 0. Answers first

1. **DS3 asks little of YEBIS in the world.** One tone curve everywhere (YEBIS's « sensitometric » filmic curve, gamma 2.2),
   a fixed exposure by area (1.52 to 2.4), a colour matrix reduced to a saturation (0.8 to 0.9), a vignette (0 to 0.4), a
   16³ grading LUT by area, a bloom (threshold 0.2 to 0.5, luminance 1.8 to 8, 6 or 8 levels), screen-space light shafts in
   12 of the 18 areas, and one depth of field and one motion blur, the same everywhere (`mapbase`). Star, ghost, halo,
   afterimage, anamorphic flare, feedback blur, airy disc, auto focus and YEBIS's own auto exposure are never asked by a
   reachable world record (§2).
2. **The cutscenes are where YEBIS is really used.** A physical-lens depth of field (aperture, focus distance, sensor size)
   in 208 of 232 records, animated colour (value 0 to 4, contrast, gamma 0 to 4.62, exposure 0 to 5.09), a vignette that
   follows the field of view, and in a handful of records a second tone curve, a custom glare with a star and the lateral
   chromatic aberration (§3).
3. **DS3's chromatic aberration is never drawn in the world.** The data carry a lateral dispersion of 0.006 at four areas,
   but its enable is 0 in every world file and the original draws no such pass at the High Wall (capture `Reference1706`).
   It is enabled in four records of three cutscene files only. This answers ROADMAP's open check (workstream 2, step 5) (§2.3).
4. **The native renderer does not host YEBIS**: since W10 it draws six passes of its own with YEBIS's shader programmes read
   from the executable and YEBIS's constant formulas (tone map, glare, shafts, depth of field, motion blur, grading). They
   reproduce the witnessed world configurations bit for bit and refuse the others; by the data, most cutscene records fall
   in a refused configuration (§4).
5. **Elden Ring no longer has YEBIS.** FromSoftware replaced it by GX filters of its own, in one fixed order: depth of
   field, TempAA, motion blur, the emissive glow, then one tone-map filter that holds the exposure, the bloom, the light
   shafts' composite, the curve read from a baked table, the colour matrix, the 3D LUT, the vignette and the chromatic
   aberration, then a last pass; the UI is drawn to a texture and blended after. The parameter model is DS3's, extended.
   The shipped areas use an extended Reinhard curve, not YEBIS's sensitometric one, with a bounded auto exposure, a neutral
   colour matrix and the look carried by a LUT by time of day (§5, §5.2).
6. **DS3 already carries the ancestors of ER's filters**: `GXToneMapFilter` (the menu views' tone map), `GXGlowFilter`, and in
   its GPARAM files the parameter blocks ER uses (the Tone Map group's second run, the Bloom group's second run, the `Lens
   Simulation` group), at constant default values (§5.4). The new chain is therefore a continuation of DS3's own engine line,
   not a foreign graft.
7. **ER's HDR output is not a model to copy**: no DXGI colour-space call exists in its executable; what exists is a vendor
   path (NVAPI, AMD AGS) tied to exclusive full screen. The native chain takes the modern DXGI route (§5.3).

## 1. The YEBIS chain of a DS3 world frame

From [TONEMAP-SKIN](../I-0067-static-menus-subsystems-map/TONEMAP-SKIN.md) §5, §15 to §17,
[YEBIS-NATIVE](../I-0068-static-yebis-ffx-lights-spec/YEBIS-NATIVE.md) §3,
[YEBIS-TECHNIQUES](../I-0068-static-yebis-ffx-lights-spec/YEBIS-TECHNIQUES.md) §4 and
[YEBIS-SECOND-TRANCHE-READY](../I-0066-independent-native-startup/YEBIS-SECOND-TRANCHE-READY.md) §2 (all earlier work of
this repository, re-read here).

| # | Stage | What it does | Draws (m40, m30) |
|---|---|---|---|
| 1 | Depth factor, depth of field | CoC from depth through a 2048x2 table, three half and quarter size layers blurred (3, 5 or 25 taps), front-blur composite | 10 |
| 2 | `Alpha_R` | the depth mask of the light shafts into the scene's alpha (areas with shafts) | 1 |
| 3 | Motion blur | camera velocity from depth, object velocity from the velocity target, 5-tap directional blur, recurrences by quality | 2 and more, only with the option on |
| 4 | Glare | bright pass (threshold, luminance, remap) at 0.375 of the width, 6 to 8 Gaussian levels (base, base, /2, /4, /8, /16...), per-channel radii by wavelength, weighted combine | 20 to 29 |
| 5 | Light shafts | mask with a 256² noise, four radial blur passes, added to the glare | 6 |
| 6 | Tone map | exposure, the sensitometric curve, the glare screen-blended **after** the curve, the colour matrix, gamma, a 4x4 dither, the vignette, into 8 bits | 1 |
| 7 | Colour grading | a 16³ LUT (made from a 16x256 strip by a compute pass) applied on the 8-bit picture | 1 |

After YEBIS, on the 8-bit picture, the engine's own GX filters: the spatial anti-aliasing (`DLAA.ppo`, an edge filter), the
emissive glow (`GXGlowFilter`, four levels, composited onto the 8-bit colour), the UI, then the display curve of the
brightness setting ([GAMMA-COLOUR-PATH-READY](../I-0066-independent-native-startup/GAMMA-COLOUR-PATH-READY.md)).

The tone map, exactly (TONEMAP-SKIN §15, §16; W10-NATIVE-PASSES §2 corrected `T.y`): with `x = scene x exposure`,
`T.x = 1 + 2.0891 c^(1/3)` and `T.y` a 21-entry table read at `20 c^(1/3)` (`c` = FilmicContrast),

```
y   = saturate((1 - exp(-x T.x)) (1 - T.y exp(-x T.x))^2)      per channel
y   = y + (1 - y) glare                                        the glare, screen-blended in display space
c   = saturate(M (y, 1))                                       M: hue, saturation, value, contrast
out = c^(1/gamma) + dither                                     then the vignette's factor, then the LUT
```

OBSERVED consequences for a replacement: the curve works per channel (it bends hues towards yellow and white in the
highlights: part of the original look); the bloom is added after the curve, so it never changes the mid-tones' exposure; the
LUT is display-referred (it reads the 8-bit, gamma-encoded picture).

## 2. By area (the world)

Source: [CENSUS-WORLD](CENSUS-WORLD.md) (WORKER; the pilot re-read `m30_00_0000` and `m37_00_0000` in the raw dumps: exposure
2.3 and 1.52, FilmicContrast 0.3 at m37, lateral dispersion 0.006 at m30, bit patterns equal). The effective value of a file
is its own, else its base `mAA_BB_0000`, else `mapbase`, else `default` id 0. All 100 retail map files hold one record (id 0).

### 2.1 The per-area sheet (base files)

| Area | Exposure | Filmic contrast | Saturation | Vignette | LUT | Bloom thr / lum / levels | Shafts |
|---|---|---|---|---|---|---|---|
| m30_00 High Wall of Lothric | 2.3 | 0.15 | 0.85 | 0.34 | 2 | 0.3 / 6 / 8 | on |
| m30_01 Lothric Castle | 2.2 | 0.12 | 0.8 | 0.3 | 2 | 0.3 / 6 / 8 | off (0010 on) |
| m31_00 Undead Settlement | 2.2 | 0 | 0.9 | 0.2 | 1 | 0.2 / 8 / 8 | off (variants on) |
| m32_00 Archdragon Peak | 2.4 | 0.1 | 0.85 | 0 | 2 | 0.3 / 3 / 8 | on |
| m33_00 Road of Sacrifices, Farron Keep | 2.3 | 0.25 | 0.85 | 0.38 | 2 | 0.23 / 8 / 8 | off (0500 on) |
| m34_01 Grand Archives | 2.0 | 0.13 | 0.8 | 0.3 | 1 | 0.3 / 4 / 8 | off (0010 on) |
| m35_00 Cathedral of the Deep | 2.0 | 0.15 | 0.85 | 0.4 | 1 | 0.3 / 8 / 8 | on |
| m37_00 Irithyll, Anor Londo | 1.52 | 0.3 | 0.9 | 0 | off | 0.5 / 3 / 6 | on |
| m38_00 Catacombs, Smouldering Lake | 2.0 | 0.15 | 0.85 | 0.35 | 3 (no strip shipped) | 0.25 / 8 / 8 | off |
| m39_00 Irithyll Dungeon, Profaned Capital | 2.3 | 0.15 | 0.84 | 0.3 | 3 | 0.28 / 6 / 8 | on |
| m40_00 Cemetery of Ash, Firelink Shrine | 2.3 | 0.15 | 0.8 | 0.3 | 3 | 0.2 / 3 / 6 | on |
| m41_00 Kiln of the First Flame | 2.0 | 0.1 | 0.8 | 0.3 | 1 | 0.25 / 5 / 6 | on |
| m45_00 Painted World of Ariandel | 2.0 | 0.3 | 0.85 | 0.35 | 0 (no strip) | 0.25 / 6 / 6 | on |
| m46_00 (arena) | 2.0 | 0.13 | 0.8 | 0.3 | 1 (no strip) | 0.3 / 4 / 8 | off |
| m47_00 | 2.0 | 0.1 | 0.8 | 0.3 | 1 (no strip) | 0.25 / 1.8 / 6 | on |
| m50_00 Dreg Heap | 2.0 | 0.1 | 0.9 | 0.35 | 1 | 0.25 / 5 / 6 | on |
| m51_00 Ringed City | 2.0 | 0.3 | 0.85 | 0.35 | 0 (no strip) | 0.25 / 6 / 6 | on |
| m53_00 | 2.4 | 0.1 | 0.85 | 0 | 2 (no strip) | 0.3 / 3 / 8 | on |

Gamma 2.2, tone type 2, value 1 and contrast 1 in every world file. « No strip »: the map ships no LUT texture for that id,
so the original draws no grading either (STUDY-YEBIS-REFUSALS §1.3; m50's strip exists in the DLC2 archive and the native
logged it absent: TO CHECK, carried from that study).

### 2.2 What is the same everywhere, and the outliers

- **Depth of field** (from `mapbase`, all 18 areas): on, aperture F 2.0, the far range « custom » (from 50 to 10,000 units,
  CoC 0.4 x 0.015: a slight far blur), the near range off, edge quality 2, no auto focus.
- **Motion blur** (from `mapbase`): on, time ratio 0.2, four samples, a maximum length of 0.1 of the screen, two recurrences,
  a base of 30 frames a second. The game's option decides whether it draws (Thomas's setting: off).
- **Anti-alias**: type 4 (the `DLAA.ppo` edge filter) everywhere; replaced natively by the TAA or DLSS (D-0265).
- **Outliers** (selectable by a collision of the map): `m32_00_0100` (5 of Archdragon Peak's 59 collisions: glare shape 3
  with 10 levels, no LUT, aperture 0.8, motion blur nearly off, the glow off), `m41_00_0800` (exposure 0.45) and
  `m41_00_0900` (exposure 0, a black scene by the original's own rule, served natively since I-0113 tranche 20),
  `m37_00_0500/0510` (exposure 2.3 and the LUT on), `m30_00_0200` (edge quality -1, far CoC 1.75).
- **Never non-default in a retail world file**: star, ghost, halo, afterimage, anamorphic flare, bloom dispersion, feedback
  blur, the airy disc, the vignette's field-of-view dependence, the value scale, the contrast, auto focus. No file names a
  film grain, a heat shimmer or a lens flare; `Enable LensDistortion` is 1 everywhere with no strength parameter.
- **YEBIS's auto exposure**: on only in `m30_00_0100/0110`, which no collision selects (K-0448).

### 2.3 The chromatic aberration (ROADMAP workstream 2 step 5's check)

- The Tone Map group holds, in this order, `Enable`, `WaveSample`, `ToneLateralDispersion[0..1]`,
  `ToneUniformDispersion[0..1]` (CENSUS-WORLD §5). OBSERVED (the pilot's own reading of the 168 dumps): the dispersion is
  0.006 at m30_00, m30_01, m34_01 and m46_00 (and 0 elsewhere), and **`Enable` is 0 in all 100 retail world files**.
- OBSERVED in the original's capture at the High Wall (`evidence/I-0066/artifacts/i66-reference-1706-m30-yebis/startup.jsonl`,
  the library's draws by technique): 19 distinct techniques, the tone map `TonemapHDR_Dither_Vignette_Exposure_Matrix_Gamma`,
  and none of the `LateralChromaticAberration`, `LensDistortion`, ghost or star families. So with a dispersion of 0.006 and
  the enable at 0, the original draws no chromatic aberration.
- `Enable` is 1 in four cutscene records: `s32_00_0030` id 0, `s37_00_0030` id 0, `s41_00_0000` ids 0 and 1 (dispersion
  0.005 or 0.006, `WaveSample` 3, 5 or 7), and in the non-retail `m21`, `m22` and `mapoverride_0003`.
- INFERRED: that `Enable` is the chromatic aberration's switch (it sits at the head of its block, and the library has a
  `SetChromaticAberrationEnable`); TO CHECK on one of the three cutscenes in the original (the library's techniques there).
- Consequence for D-0266's row (« DS3's chromatic aberration, none added that YEBIS does not have »): the world has none
  to switch; a row would act on three cutscenes at most.

## 3. In the cutscenes

Source: [CENSUS-CUTSCENES](CENSUS-CUTSCENES.md) (WORKER; its reader reproduces STUDY-YEBIS-REFUSALS' totals) and
[REMO-CUTSCENES-NATIVE](../I-0069-static-menu-audio-map/REMO-CUTSCENES-NATIVE.md). A cutscene is a timeline (`.mqb`) whose
`SceneDrawParam` tracks switch each GPARAM group to an (bank, id) pair, with a blend from a begin pair to an end pair; the
49 cutscene files `sAA_BB_VVVV` hold several records by group (146 Tone Map, 129 Bloom, 232 Dof, 72 LightShaft, 51 grading).

| Feature | Cutscene use | Range | World |
|---|---|---|---|
| Depth of field | 208 of 232 records on | aperture F 0.01 to 80, focus distance 0.1 to 200, sensor size 28.77, 43.27 or 84.75, custom near or far ranges in 15 records, 1 to 7 aperture levels | one fixed far blur |
| Exposure | every record | 0 to 5.09 | 1.52 to 2.4 |
| Gamma | 12 records off 2.2 | 0 (3 records), 1.8 to 4.62 | 2.2 |
| Value, contrast | 39 and 28 records | value 0 (8 records, a fade to black) to 4 (a flash); contrast 0.92 to 1.16 | 1 and 1 |
| Hue, saturation | 19 and 108 records | hue up to 3, saturation 0 to 1.09 | hue 0 or 1, saturation 0.75 to 1 |
| Vignette | 105 records | power up to 1.35; field-of-view dependence in 58 records | power up to 0.4, no dependence |
| Tone type 3 | 2 records (`s40_00_0040`) | a Reinhard curve (TONEMAP-SKIN §16: curve 3 is technique `_REINHARD`) | never |
| Glare | shape 0 in 26 records, shape 1 in 103 | luminance up to 86.7, threshold 0 to 1.5 | shape 1 |
| Custom glare (Bloom `Enable` 1) | 13 records, 8 files | one record with a star and a bloom dispersion (`s41_00_0000` id 0); anamorphic luminance off 1 in 10 records | never |
| Chromatic aberration | 4 records, 3 files (§2.3) | dispersion 0.005 to 0.006 | never |
| Light shafts | 37 records on | colour scale up to 80 | 12 areas |
| Motion blur | 3 records | | the option |
| Feedback blur, auto focus, YEBIS auto exposure | never | | never |
| Grading LUT | 29 records on | LUT ids 0, 1, 2, 3, 10 | ids 0 to 3 |

Beyond the GPARAM records (names read in the timelines, 72 bundles): fade tracks in 32 bundles (the fade plates, drawn by
the fade system after the post chain), `DOF` objects in 13 bundles and a `DoFocus` track in 2 (INFERRED: a focus distance
that follows an object; the engine has a scripted focus-distance override, `context+0x413D/+0x4140`,
YEBIS-SECOND-TRANCHE-READY §2.2). TO CHECK: which records each cut selects and the blend curves (they are in the `.mqb`
files, not extracted on this PC), and what the `DoFocus` track feeds.

What a replacement chain owes the cutscenes: every parameter blended between two records each frame, over wide ranges; a
depth of field computed from lens values, not from fixed ranges; a colour matrix able to reach black and over-bright; a
gamma that is a parameter of the record; a vignette that reads the camera's field of view.

## 4. The native chain today

OBSERVED in `native/renderer/scene_pass_frame.cpp` (the main view's frame, lines 960 to 1295 at `8f415270b`), W10-NATIVE-PASSES
and the stage headers.

| Order | Pass | Whose code | Space |
|---|---|---|---|
| 1 | DLSS (with Ray Reconstruction) or the native TAA, to the output size; Thomas's sharpening | ours, NVIDIA's | linear HDR, RGBA16F |
| 2 | Depth of field (factor, levels, composite, copy back) | YEBIS's programmes and formulas | HDR |
| 3 | `Alpha_R`, then the motion blur | YEBIS's programmes | HDR |
| 4 | The automatic exposure (a histogram compute pass, D-0263, D-0264); it solves its factor through YEBIS's tone-map block | ours | HDR |
| 5 | The glare family (bloom levels, light shafts) | YEBIS's programmes | HDR, R11G11B10 |
| 6 | The tone map, into 8 bits | YEBIS's programme `TonemapHDR_Dither_[Vignette_]Exposure_Matrix_Gamma` | HDR to 8-bit |
| 7 | The colour grading | YEBIS's programme | 8-bit |
| 8 | The original's spatial AA when it still runs, NIS | the game's programmes, NVIDIA's | 8-bit |
| 9 | The emissive glow | the game's GX programmes | 8-bit |
| 10 | The HUD-less colour (frame generation), the UI, our overlay | | 8-bit |
| 11 | The display curve (the brightness setting), into the back buffer `R8G8B8A8_UNORM` | the game's programmes | 8-bit |

- The six YEBIS-derived passes draw **YEBIS's DXBC, read from the game's executable at run time** and pinned by hash
  (`native/world/yebis_pins.h`), with constant blocks (`$Globals`, 3,584 bytes) built by our transcription of the library's
  CPU formulas. No library code runs. « Removing YEBIS » natively means replacing these programmes, formulas and blocks by
  shaders and a parameter model of our own.
- They **refuse** what the witness zones never showed: a tone type other than 2, a glare shape other than 1, a tinted
  glare, a depth of field whose holder flags are not 2, 17-tap depth-of-field levels, motion-blur modes other than 2, some
  shaft configurations (STUDY-NOT-YET-NATIVE rows 12 and 13). INFERRED from the data (the mapping of the holder flags to
  `DofEdgeQuarity` is STUDY-YEBIS-REFUSALS' inference): 229 of the 232 cutscene depth-of-field records (edge quality -1)
  and the 26 glare records of shape 0 fall in a refused configuration, so the cutscenes largely play without their depth
  of field natively. No cutscene has been judged natively yet (ROADMAP workstream 6).
- Everything after the tone map is 8-bit: the glow, the UI, the display curve and the swap chain. An HDR10 output moves
  that boundary.
- The menu views (portraits) do not use YEBIS: they take the game's `GXToneMapFilter`, native since I-0113 tranche 27. Out
  of this workstream's scope.

## 5. Elden Ring 2.7.1

Sources: [ER-POST-CHAIN](ER-POST-CHAIN.md) and [ER-TONEMAP-HDR](ER-TONEMAP-HDR.md) (WORKER, static, 30 headless runs in
all; every row there carries its address and status). The pilot read both reports in full; no ER address was re-read by the
pilot, so what a later design decision rests on is re-read then (WORKFLOW « Roles and delegation »).

### 5.1 The chain

- No YEBIS: none of the library's strings, classes or techniques is in the executable; « Yebis » survives in editor labels
  (`Tone Map[Yebis]`, `Bloom[Yebis]`...) and in one helper class. H1 of the CONTRACT holds.
- The post filters are `GXBS::GX*Filter` classes with a Submit (CPU) and an Execute; their programmes are loaded by name
  from `shader:/GXPostEffect.shaderbnd`. The order is fixed by sort keys, not by call order (the walk of the sorted queue
  was not read: the order below is INFERRED from the keys):

| # | ER stage | Note |
|---|---|---|
| 1 | Light shafts: the sky mask and its blur | queued by the tone-map filter, composited inside it later |
| 2 | Outline, fur blur | no DS3 equivalent |
| 3 | Depth of field (`GXLensSimulationFilter2`) | before the TempAA when its `PreTempAA` parameter is on, after it otherwise |
| 4 | TempAA | on the HDR colour |
| 5 | Motion blur | camera and object velocity |
| 6 | Glow (`GXGlowFilter`, « CustomBloom ») | the emissive glow, **before** the tone map |
| 7 | Tone-map filter | auto exposure, bloom, shafts' composite, table curve, colour matrix, 3D LUT, vignette, chromatic aberration, feedback blur, then `PostOETF` |
| 8 | Scene bokeh, then the spatial AA with a sharpen when the AA is not the TempAA | |
| 9 | UI | drawn to a texture, blended by `_BlendUI_*` programmes (SDR to SDR, SDR to HDR) |

- **Tone map** (ER-TONEMAP-HDR §1, §2): the parameter list is DS3's, extended: exposure in EV, ToneMapMode (Filmic,
  Sensitometric the code's default, Reinhard, ReinhardEx...), FilmicContrast, hue, saturation, value, contrast, colour
  shift and scale, the vignette (power, range, shape, field-of-view dependence), the chromatic aberration (an enable and
  two lateral dispersions). The curve is **baked by a GPU pass into a 1D table** and sampled by the tone-map programme;
  the colour matrix is built on the CPU as in DS3.
- **Bloom** inside the tone-map filter: bright pass (a temporal variant exists), Gaussian levels (1 to 8, the code's default
  7), an up-scaling composite; parameters luminance, threshold, alpha, radius scale, saturate control, smooth.
- **Colour grading**: a 16³ LUT built each frame by compute passes from one or two 16x256 strips, **blended over a fade
  time** between the old and the new, bound to the tone-map draw.
- **Auto exposure**: live code, off by the code's default; a luminance chain down to a 1x1 target, **read back on the CPU
  through four staging buffers** (about four frames late, INFERRED), then folded into the exposure scale.
- **Depth of field**: parameters of a lens (Lens F, Av, film size, focus distance, « use camera focus ») or linear near and
  far ranges; a half-resolution gather with a CoC buffer, near and far layers, a temporal accumulation (INFERRED from the
  programme names and the targets).
- **Motion blur**: camera and object velocity, a tile maximum, a separable dilation, a gather with a reconstruction variant
  (INFERRED from the programme names).
- No film grain, no lens-flare filter and no post-process distortion: those are effect-system appearances in ER.

### 5.2 What the shipped values and programmes are

Read after the plan, from Thomas's own Elden Ring, on his agreement (2026-11-12 12:19): [ER-VALUES](ER-VALUES.md) (WORKER:
319 draw-parameter files parsed, the post-effect programmes disassembled; the pilot read the report in full and re-read none
of its sources). What matters for the design:

- **The curve ER ships is `ReinhardEx`** (mode 8, in 912 of 928 records; the sensitometric mode only in a few cutscene
  records): `t = (k x)^p`, `y = t / (t + 1)`, then the display gamma, with the contrast exponent `p` 1.3 to 1.55 by area.
  ER's sensitometric table is not YEBIS's formula. The table is addressed by `c / (c + 0.2)`.
- **Exposure** is in EV (3.2 to 4.2 in play, following the time of day in the open world). **Auto exposure is on in 133 of
  173 gameplay records, bounded**: it may darken by 1.5 EV and brighten by 0.4 to 0.5 EV, slowly (speeds 0.1 to 0.25),
  centre-weighted.
- **The colour matrix is neutral** (saturation, hue, value and contrast at 1 or 0 nearly everywhere): ER's look is in its
  3D LUT (by time of day in the open world) and its curve. DS3 uses both a per-area saturation and a per-area LUT.
- **The bloom is added linearly before the curve** (DS3's YEBIS screen-blends it after); its bright pass is a soft
  luminance subtraction on the exposed scene; three levels composed at 1, 0.5, 0.25.
- **The vignette is on everywhere** (power about 0.5) and applied after the curve; **a slight chromatic aberration is on
  in 169 of 173 gameplay records** (dispersion about 0.0012), drawn by the pass named `PostOETF`, which is only that.
- **The SDR path ends at the LUT**: no sRGB or PQ step and no dither. **An HDR branch follows the LUT in the same
  programme**: it undoes the curve, extends the highlights above 1 by a shoulder, and rescales the colour: ER's HDR is its
  SDR grade with expanded highlights (constants not decoded).
- Depth of field off in play by default, motion blur off nearly everywhere, light shafts on in most records.

Not established: the mapping from the parameters to the programme's constants (INFERRED), the layering of the files, the
LUT images themselves.

### 5.3 HDR output

- OBSERVED absence (WORKER): no `IDXGISwapChain3` or `4` interface identifier, no `SetColorSpace1`, no `SetHDRMetaData`, no
  HDR key in the graphics configuration. The swap chain is made by the legacy `CreateSwapChain`.
- What exists: a vendor path (NVAPI's HDR colour control, AMD's `agsSetDisplayMode`) called only in exclusive full screen,
  with an `R10G10B10A2` back buffer when the display reports HDR; menu classes `HDRSetting` and `SDRSetting`; UI blend
  programmes `_BlendUI_SDR2HDR`. Whether the PC game ever turns it on was not established.
- ER's defects named for D-0195 (from this decompilation, static): the exposure read back on the CPU (a latency), the HDR
  tied to vendor APIs and exclusive full screen, the tone table and the LUT rebuilt every frame, no output dither found.

### 5.4 DS3 already holds this lineage

OBSERVED in the censuses and TONEMAP-SKIN §17:

- DS3 has `GXBS::GXToneMapFilter` (its programmes `ToneMap_*PS.ppo`) for the views without YEBIS (the menu views), and
  `GXBS::GXGlowFilter`; ER's filters carry the same class names.
- DS3's Tone Map group has a **second run** of parameters (`Exposure`, `AvgLuminance`, `ToneMapMode`, `Contrast`, `Gamma`,
  `UseTable`, `Enable AutoExposure`, `AutoExposure LightAdapt Speed`, `Middle Gray` -2, `Center Weight` 4): ER's tone-map
  list and ER's code defaults. Its Bloom group has a second run (`GlareNumLevels` 7, `Glare Luminance` 1, `Glare Threshold`
  0.5, `GlareBloom Alpha`, `GaussianRadiusScale`, `Sturate Ctrl`, `Smooth` 0.8): ER's bloom list and defaults. Its `Lens
  Simulation` group (Lens F 5.6, film size 0.024, `CoCBlurCount`, `UseCameraFocus`) is ER's depth-of-field list.
- These second runs are constant over the whole world (CENSUS-WORLD §5): DS3's artists tuned the YEBIS runs, not these. So
  the art direction to carry over is in the YEBIS parameters; ER's model says how the chain is organised.

## 6. What the plan must settle (the design questions this study opens)

1. **The original colorimetry in a chain of our own.** Its formulas are known exactly (§1), so a tone-map pass of our own
   can reproduce today's picture; the oracle is today's native chain, itself equal to the original's captures at the
   witness zones (K-0450). Not bit for bit once the chain is in float with a dither: the tolerance is stated in the plan.
2. **The 8-bit boundary.** The glow, the UI and the display curve are drawn on 8 bits today; ER draws its glow before the
   tone map and blends its UI from a texture. HDR10 needs one of the two for the UI: a texture blended at the end (ER's
   way; it would also give frame generation its UI layer), or the UI's pipelines retargeted to a wider format.
3. **The bloom after the curve.** YEBIS screen-blends its glare after the curve (§1); a modern chain adds the bloom before
   it. Matching the original look means keeping YEBIS's order for the original colorimetry, or re-tuning.
4. **The light shafts.** Screen-space, in 12 areas; workstream 12 brings a traced volumetric fog that makes real ones.
5. **The cutscenes' depth of field.** Lens values in, a gather out: ER's model fits DS3's data directly (same names).
6. **Refusals disappear by construction**: a parametric chain has no « configuration never witnessed ».
7. **What only a run can tell** (none blocks the plan): what glare shape 3 and the custom glare draw in the original
   (`m32_00_0100`, the 13 cutscene records); the cutscenes' selected records and blends; the three chromatic-aberration
   cutscenes; m50's LUT strip; whether a cutscene's depth of field is really refused natively today.

## 7. Limits of this study

- The censuses and the two ER reports are workers' readings; the pilot checked the world census on two files' raw values
  and on one capture, the cutscene census through its reproduction of an earlier study's totals, and read the ER reports
  without re-reading ER's code.
- The cutscene census reads every record a file holds, not the records a cut plays.
- ER's values and tone-map programmes are a worker's reading of its data files (§5.2), its parameter-to-constant mapping
  inferred. Whether ER's HDR runs on PC is unknown.
- Nothing was run: every statement on what the native draws in a cutscene is from the code and the data.
