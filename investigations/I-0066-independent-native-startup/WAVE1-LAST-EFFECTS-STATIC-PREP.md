# Wave 1, the last visible gaps: lightmap floors, bonfire flames, the m50 eclipse ring (static preparation)

Static and read-only (no game launch, no run, no save, nothing changed in `main`). Written for the pilot of the « Lenteur du Bourg »
thread. The other static thread prepares the phantoms and the glow. Every correction below is a **mechanism** (D-0150/D-0161), never a
zone, map or asset case. Statuses: OBSERVED (a log or a picture I read), STATIC (code or data I read), INFERRED, OPEN.

## 0. Answer first

| Gap | Cause (best, with its status) | What to build | Size |
|---|---|---|---|
| **Lighter lightmap floors** (m37 balustrade strip, m31 wheel, m30 floor) | **The Dominant Light rows of `cbPerFrame` are the engine bank's built-in defaults, not the zone file's values** (OBSERVED: keys `{-1,-1}` and rows `0.5, 0, 1.25, 1, 1, 1, 1` in every native run; STATIC: the DOL pixel shader multiplies the baked light by the colour scale and the RGB scales). The three zones are exactly those whose file colour scale is far under 1 (m37 0.45, m30 0.5, m31 0.8; m40 1.2 and m45 1.0 are not lighter). | One native Dominant record per frame from `ds3light::Banks` (area key, layering of L3, A/B blend). This is lead **L2 row 1** of [IMAGE-REGRESSION-STATIC-LEADS](IMAGE-REGRESSION-STATIC-LEADS.md) turned into a recipe (§1.4). | small (one producer) |
| **Bonfire flames** (m40, m50) | **Not a bug of the bonfire: native has no draw side for FFX effects** (`effect_system.h`: « the GPU ring stays null (draw-side only) »). The flame is effect `f000001020` (its content is dumped in §2.1: two camera-facing billboard families and a heat-haze distortion). | EFFECTS-D3D12-DESIGN steps N5 (record layer, dry), N6 (E01 tessellated billboards), N8 (two-target glow twins), N9 (distortion + frame copy). Flames need N5, N6 and N8; the heat haze needs N9. | large (the effects phase) |
| **m50 eclipse ring** | **The sky is map geometry that native does not show.** In the original the m50 sky is four map pieces of forward-only unlit families (`m040001`, `m090001`, `m090002` « Test_sun », `m090020`); the native picture has no disc, no ring, no cloud layers and no orange horizon (§3.1, OBSERVED). Why those pieces do not appear is OPEN: five candidates and one log row that decides (§3.4). | The log first; the fix follows the branch the log shows. | log: small |

## 1. Lightmaps: floors lighter than the original

### 1.1 What the pictures say (OBSERVED)

[SUN-SHADOWS-WAVE1](SUN-SHADOWS-WAVE1.md) §7 closing pictures (`evidence/I-0066/artifacts/cascade-casters-shots/<zone>-final.jpg`, original above, native
below, same save, 140 s after launch): the differences left in the lit scene are the lightmapped floors, **lighter natively**: the strip by the m37
balustrade, the m31 wheel, the m30 ground. The sun shadow (D-0180/D-0184) is not the cause: the original casts none of those pieces (K-0453).

### 1.2 The mechanism (STATIC, from the DXBC of the DOL programme, names and registers only)

`GXFlver_ColDifSpcBumpDOL_Gbuf` (the lightmapped map family) reads the lightmap `t26` (`g_DOLTexture`) and the dominant-direction texture `t27`
(`g_DOLTexture2`), `cbTexHdrScaleParam` (b0, `FC_TexHdrScaleParam`) and two rows of `cbPerFrame` (b9). In order:

1. `L = t26.rgb * t26.rgb * FC_TexHdrScaleParam.rgb * cb9[13].xyz` (lightmap radiance, then the three **Dominant R/G/B Scale**);
2. `s = saturate(t27.w + cb9[12].y)` (**Dominant Sharpness Offset**);
3. `out = albedoTerm * ( L * cb9[12].w * (1 - s) + L * cb9[12].w * s * directionalTerm )` (**Dominant Color Scale** is `cb9[12].w`).

So the baked light on a lightmapped surface scales **linearly** with the file's `Dominant Color Scale`. `cb9[12].x` (min) and `.z` (exponent) are not read by this programme.

`cbPerFrame +0xC0..+0xDF` is produced by `ProducePerFrame` (`native/world/producers.cpp`): seven floats at `+0xC0` (min, offset, exponent, colour scale,
R, G, B) and 1.0 at `+0xDC`, interpolated between a row A and a row B by `t`. The rows come from the engine's own bank lookup `134A20`
(`game_bootstrap.cpp`, near the `native_dominant_rows` witness) with the keys `ctx+0x4110` and `ctx+0x4118`.

### 1.3 What native feeds against what the original reads

OBSERVED, every native run that logs `native_dominant_rows` (e.g. `evidence/I-0066/artifacts/i66-native-1718-m40-nosun/startup.jsonl`):
`keys [4294967295 x4]`, `blend 0`, `a = b = [0.5, 0, 1.25, 1, 1, 1, 1, 1, 1, 1, 1]`: the keys never received an area, and the bank has no file (49D00 is replaced by
bookkeeping), so the lookup returns its built-in record. STATIC, the original's values (`evidence/I-0066/queries/gparam-all/param_drawparam_<file>.tsv`,
group `Dominant Light`):

| Zone file (the witness) | Offset | Exponent | **Colour scale** | R / G / B | native default | lightmap contribution native / original |
|---|---|---|---|---|---|---|
| `m37_00_0000` | 0 | 1.25 | **0.45** | 1 / 1 / 1 | 0 / 1.25 / 1 | **2.2 x** (offset equal) |
| `m30_00_0010` | 0.55 | 1.25 | **0.5** | 1 / 1 / 1 | 0 / 1.25 / 1 | **2.0 x**, and the dominant share differs (offset 0.55 against 0) |
| `m31_00_0010` | 0.2 | 1.25 | **0.8** | 1 / 1 / 1 | 0 / 1.25 / 1 | **1.25 x** |
| `m45_00_0000` | 0.2 | 1.25 | 1.0 | 1 / 1 / 1 | | 1 x (not lighter, as seen) |
| `m50_00_0000` | 1 | 0.86 | 0.5 | 1 / 1 / 1 | | 2 x where DOL pieces exist |
| `m40_00_0000` | 1 | 0.86 | 1.2 | **0.6 / 0.8 / 1** | | 0.83 x in colour scale, and the bluish tint missing (the Firelink lead of L2) |
| `m31_00_0020/0030` (later state) | 0.85 | 1.25 | 1.0, R 0.9 | | | later progression variants of the same bank |

This is the **only static cause I found that explains the three named zones and not the others**, with sizes in the right range. It is INFERRED (not yet
measured as the cause) until the rows are fed and the same frame is compared. The file's `SH Color Scale` (m30: 0.5) is a different consumer (instance ambient SH,
L2 row 2: zeros today) and is not part of this producer.

### 1.4 How to build it (mechanism, not a patch of rows)

1. **The key.** In the original, `FUN_140D52B70` (the area controller set's per-frame apply, reached from `FUN_140E006A0`) calls `FUN_140088580(ctx, key)` (pair A = B = key,
   weight 0, writes `ctx+0x4110/+0x4118/+0x4120`) or `FUN_1400885C0(ctx, keyA, keyB, weight)`, with the **scene's main key**; its reset branch `FUN_140088AF0` writes
   every pair to the default key. So the Dominant pair follows the same main `{area, id}` as the light set (OBSERVED natively: `native_light_set` keys `[150000000, 0]` and
   `[150000000, 100]` at m50, area 150000000). Natively the pair stays `{-1,-1}` (OBSERVED). Take the key from the same source the native light-set code already uses for
   the frame's main entry, not from the context pair.
2. **The values.** `Banks::GroupFloat(area, id, "Dominant Light", "<param name>", value, &source)` exists (`native/world/light_producers.h:156`); the group's eleven float
   parameters, in file order, are: Sharpness Min Value, Sharpness Offset, Sharpness Exponant, Color Scale, R, G, B Scale, then the four SH scales. For files without the
   group (102 of 168) use the layering of L3 (`Banks::GroupEntries`, « the area's own file then mapbase », already used for YEBIS), never `default.gparam` id 0.
3. **The blend.** Feed `ProducePerFrame(time, rowA, rowB, t, ...)` with A and B resolved from the pair's two keys and `t` the pair's weight, exactly as the original
   interpolates (`ProducePerFrame` already does; only the rows change). `rows.a/rows.b` take the first seven floats; the SH four are for the instance path.
4. **Fall back to the present behaviour** (the bank's record) only when no file has the group, with a counter row « Dominant rows from bank default » so the residue is visible.
5. **Check** (same-frame, no second run): `native_dominant_rows` against the file row at m37, m30, m31 (the three above), then `Compare-SceneHdr.ps1` on the strip, the wheel and
   the floor. Expected: the m37 strip falls by about 2.2 x, the m30 floor by about 2 x, the m31 wheel by about 1.25 x in their lightmap share.

### 1.5 Secondary levers (log, do not fix before the rows are in)

If a floor stays lighter after the rows are right, the next suspects are in the texture identity, not the shading (all STATIC, `game_bootstrap.cpp`):

- **Candidate resolution**: a lightmap name is looked up as `path`, `path + "_l"` (low-resolution package) and, for names containing `_dol_0`, `path + "_atlas000"` (line ~5529). An absent
  lightmap binds `SYSTEX_WHITE` (default bound texture, `NativeDefaultBoundTexture`): a white lightmap is full-bright, the same symptom as too high a scale.
  One row per lightmapped draw: material, the candidate that hit, `white default` or not.
- **`cbTexHdrScaleParam`**: read from the lightmap's TPF float metadata when present, `(1, 1, 1, 1)` otherwise (the game's own upload rule, I-0063). A lightmap whose metadata
  was dropped by native would be lighter by its scale. Row: the four floats and `float_metadata` present or not, per lightmap record.
- **The atlas page** (`_atlas000` against a page index): the wrong page gives a wrong lightmap tile, i.e. a tint rather than a global lightening.

## 2. Bonfire flames

### 2.1 What the original draws (STATIC: the two effect files of the common effects, read with `Read-Fxr.ps1`, outputs outside the repository)

The bonfire condition (`SprjLuaEventConditionBonfire`, `45DD90` -> `7CC4B0` -> `7CB8F0`) asks the effect id of the bonfire: the two small getters `FUN_14045DD80` and
`FUN_14045DD70` write **1020 (0x3FC)** and **1021 (0x3FD)** (ASM read). The files are `frpg_sfxbnd_commoneffects_effect/f000001020.fxr` (17,648 bytes) and `f000001021.fxr`
(1,008 bytes). Which of the two the lit bonfire takes is INFERRED to be 1020 (1021 holds no action at all: a root container with its three configuration blocks only, no
particle, light or sound). The content of 1020:

| Node | Action (FXR type) | What it is | Key values |
|---|---|---|---|
| #1 | `NodeSound` (75) | the fire sound | outside the render scope |
| #3 | `MultiTextureBillboardEx` (604), **blend Normal**, orientation CameraPlane, soft (`depthBlend` 1) | the flame body | periodic emitter, one alive at a time (interval 0.0667 s), size 10 x 7 (m), rotation 180, three texture layers (ids 11409, 10000, 11409) scrolling by bezier loops, colour ramp orange `(1, 0.50, 0.31)` -> `(1, 0.83, 0.42)` -> back, `color3` 2,2,2,1 (HDR gain) |
| #4 | `MultiTextureBillboardEx` (604), **blend Add**, orientation Camera, soft | sparks and embers | 16 alive, 2 per 0.1 s, box emitter 8 x 5 x 0.3, width 5 -> 8, layers 2000/2001/2000, gravity 0.2, colour ramp white-orange, `color3` 2,2,2,4, speed 0.2 |
| #5 | `Distortion` (607), mode NormalMap (texture id 6000), orientation CameraPlane | heat haze over the flame | size 10 -> 13 x 7, radius 3, intensity bezier peaking at about 4.96, stir speed 2 |

(All three nodes use `NodeAttributes.attachment = Parent` and a negative `depthBias` of -50/-55: the effect is parented to the bonfire dummy, drawn with a small bias toward the camera.)

### 2.2 Why native shows nothing (STATIC)

`native/frame/effect_system.h` states the draw side is not implemented (the effect system, lights and wind are: W4). FFX-FAMILIES-PHASE2 maps 604 to the tessellated billboard
family E01 (`GXFfxtessellate{,Soft}MultiTexture*`, indices 41–56, the `GlowD` variants write a second target) and 607 to E04 (`GXFfxdistortionBump`, a copy of the HDR frame bound
at t0). The earlier pictures say « only the embers natively » at m40 and m50: the embers seen are not this effect's node #4 (nothing of E01 draws natively); I did not identify
what draws them (OPEN, harmless).

### 2.3 What to build, in the plan's own order (EFFECTS-D3D12-DESIGN-NATIVE §3)

| Step | Gives the bonfire | Needs |
|---|---|---|
| N5 record layer, dry | the effect's draw records exist and match the original's counts (no pixels) | the door of ENTRY-DOOR (instance created at the bonfire dummy by `7CB8F0`) |
| **N6 E01 encode** | the flame body (#3) and the sparks (#4): patch-list 1 control point, read-only depth, `Soft` variants read scene depth, `Add` and alpha-blend states of #4/#3 | forward depth and targets (N0, present), the 604 layer textures (FXR texture ids 11409, 10000, 2000, 2001: resolve through the effect texture table, INFERRED) |
| N8 glow twins | the bloom of the flame (the `GlowD` variants: RT1 blend = RT0) | the glow filter of GLOW-CHAIN step 8 (the extraction is already native for forward Glow) |
| N9 frame copy + E04 | the heat haze (#5) | one HDR frame copy per frame at the start of the effect pass |

Witness: the m40 Firelink bonfire (the picture pair already exists: `m40-final.jpg`), second witness the m50 bonfire. Acceptance: draw count of the effect passes per frame equal to
the original's at the same save, then the picture.

### 2.4 One log to take first (cheap, Reference)

At a lit bonfire log the effect id handed to `7CB8F0` (and to the dummy it attaches to): that settles 1020 against 1021 (INFERRED above) and the number of instances (one per bonfire).

## 3. The m50 eclipse ring

### 3.1 What the pictures say (OBSERVED)

`evidence/I-0066/artifacts/cascade-casters-shots/m50-final.jpg` (original above, native below; the single-frame originals `i66-reference-1806-m50-casters.jpg` and
`i66-native-1827-m50-gbcasters.jpg`): the original shows a black disc with an orange ring in the upper right, an orange band along the horizon behind the branches, and
cloud shading; the native shows a **flat grey sky**: no disc, no ring, no horizon band, no clouds. So the ring is not a post effect missing on its own: the whole sky
layer is absent. (The post glow around the ring is the YEBIS bloom/glare, bit for bit with the original since W10: it will follow once the disc draws, INFERRED.)

Zone naming: the label « m50 Anor Londo » in [SUN-SHADOWS-WAVE1](SUN-SHADOWS-WAVE1.md) §3 conflicts with the DLC2 archive (`dlc2_map_MapStudio_m50_00_00_00`) and FFX-WAVE1-READY (« m50 Dreg Heap »); nothing below depends on it.

### 3.2 What draws it in the original (STATIC: the m50 MSB and the MTD parameters of the zone's materials)

There is **no `Sky` group in any gparam file** (groups: Bloom, ColorGrading, Dominant Light, Env Map, FeedBackBlur, Fog, LightScattering, LightSet, LightShaft, Setting Info, Tone Map)
and COVERAGE-MATRIX row 48 already says the sky is not a separate family. In the m50 MSB four map pieces are the sky; all are `MapPiece` parts with no draw group
(always drawn), their materials are unlit (`NoLight`) forward-only families with blend 0 or 2 and an alpha reference of 0:

| Piece (part) | Material (name in the FLVER) | MTD | Shader family | Blend | Depth write | Notes |
|---|---|---|---|---|---|---|
| `m040001_3000` (at 0,0,0) | `天球` (celestial sphere), 673 indices | `M[A]_NoLight` | `GXFlver_Col` | 0 | on | the sky dome |
| | `くもA`, `くもB`, `くも最前面` (clouds A, B, front), `Material #281` | `M[A]_al_m_NoLight_Opacity` | `Fujir_Nolight_OpacityBlend` | 2 | off | `GXFT_ForceForward` |
| `m090001_9000` (at 0,0,0) | `そら1` (sky 1), 1,298 indices | `M[A]_NoLight` | `GXFlver_Col` | 0 | on | |
| | `Material #9`, 904 indices | `M[A]_al_NoLight` | `GXFlver_Col` | 2 | off | |
| | `くもB` | `M[A]_al_m_NoLight_Opacity` | `Fujir_Nolight_OpacityBlend` | 2 | off | |
| | `真上` (directly above) | `M[A]_2m_NoLight_scroll` | `MultiBlend2_et1_nolight_MuitiBlendMask` | 0 | on | scrolling zenith layer (`cbTexScroll`) |
| **`m090002_9000`** (at (-493, 0, 154)) | **`Test_sun`**, 2,176 indices | `M[A]_al_NoLight` | `GXFlver_Col` | 2 | off | **the eclipse disc and ring** (INFERRED from the name, the 2,176-index ring-like mesh and the position matching the disc's side) |
| `m090020_9000` (at 0,0,0) | `遠景雲A` (far clouds A), `雲エフェクトA/B` | `M[A]_al_NoLight`, `M[A]_al_m_NoLight_Opacity` | `GXFlver_Col`, `Fujir_Nolight_OpacityBlend` | 2 | off | |

(Source rows: `evidence/I-0068/queries/water-ready/zone-water-tess.tsv`, MTD parameters `evidence/I-0068/queries/npc-families/all-mtd.tsv`, MSB through `Get-ZoneReflectParts.ps1`.)
Every programme these need exists in the programme list (`evidence/I-0068/queries/fingerprints/programs.tsv`: `GXFlver_Col_Fwd[_[A]].ppo` with the plain and `@[cl]` vertex stages,
`Fujir_Nolight_OpacityBlend_Fwd[_[A]]`, `MultiBlend2_et1_nolight_MuitiBlendMask_Fwd[_[A]]`). The `GXFlver_Col_Fwd` vertex stage projects with the ordinary view-projection
(`VC_MatrixViewProj`, camera-relative: no depth-from-w trick), reads `cbObjMatrix` (b5), `cbInstanceData` (b4), `cbSceneParam` (b8, `SC_CameraPos`, `VC_MatrixViewProj`, `VC_ClipPlane`) and `cbTexScroll` (b11, row 0).

### 3.3 What native admits (STATIC + OBSERVED)

- Native's forward state admits only MTD blend 0 and 2 (event `native_forward_blend_unsupported`; 0 such rows in the m50 runs I read), one HDR target, depth tested and not written, for the engine
  rows 0x29 (class 0x12), 0x1F (class 0x80) and the phantom rows. The sky materials above are blend 0/2: **not excluded by that gate**.
- The m50 frames carry forward rows: `native_pass_rows` shows `[31, 0, 12]` (engine pass 0x1F, 12 models) and `[41, 0, 15]` (0x29, 15 models) at category 0 (OBSERVED, Native1827).
- The refusal counters at m50 are `gates {18: 2422, 21: 530, 30: 16}` (OBSERVED): 18 is the model class mask and 21 the material class (ordinary filters), **30 is « recipe unresolved »**.
  The event keeps only the first six samples (`recipe_samples <= 6`) and all six are the character family `GXFlver_ColDifSpcBumpIbl_Bloody` (recipe 6), so ten of the 16 refusals are unnamed.
  At m45 the first sample is `GXFlver_Col.spx` recipe 7, i.e. the family of the sky `NoLight` pieces (`FOLIAGE-NATIVE` §8 already names it: « Sky `GXFlver_Col`, gate 30, 5 meshes, the known native sky flat »).
  Why a family that has `Fwd` and `Fwd_[A]` programmes in the programme list is « Missing » for its recipe 7 (Fwd, alpha-tested variant) is OPEN: the name list in the log is cut at 1,400 characters before the `Fwd` entries.

### 3.4 Five candidates and the log that decides (OPEN)

One log row settles most of it: for every model whose data name is `m040001`, `m090001`, `m090002`, `m090020` (`NativeModelDataName`, already used by the caster census near
`game_bootstrap.cpp:2884`), log per frame **(a)** admitted by the main walk or not, **(b)** per mesh the engine rows it was prepared for, the `gate(model_index, code)` it ended at (the `gate` lambda, `game_bootstrap.cpp:7632`) and for gate 30 the recipe and its `ModelProgramStatus`, **(c)** whether a draw was submitted. Raise the cap of the unresolved-recipe sample, or key it by data name, so the names of all 16 gate-30 refusals show. Then:

| # | Candidate | Shows in the log as | Fix, as a mechanism |
|---|---|---|---|
| S1 | the sky pieces are not admitted by the native main walk (their entity category, view bit or draw group differs from ordinary pieces; the 0x45/0x39 pass types of `57A20`) | (a) false | the walk's admission rule, by the original's per-pass mask (SUN-SHADOWS-STATIC-PREP §3.1: the `V` and `E+0x20` masks per pass) |
| S2 | refused at gate 30 for the alpha-tested forward recipe 7 | (b) code 30, status Missing | `SelectModelPrograms` for a forward-only family: the programme the original takes when `Fwd_[A]` or the `@[cl]` stage is not the chosen one (read `FUN_140140680`/`AA630`'s forward branch; the cache holds `Fwd_[A].ppo` for `GXFlver_Col`) |
| S3 | prepared but the forward blend or the light gate drops the draw | (b) `native_forward_light_unavailable` or blend rows | none seen at m50 (0 rows): rule out |
| S4 | drawn but invisible: the scroll block (`cbTexScroll`, `VC_TexScroll_0` scales the UV) or `cbInstanceData` is zero for pieces with no instance data, sampling one texel | (c) true, picture flat | produce the tex-scroll rows by `ProduceTexScroll` for these meshes |
| S5 | drawn at the wrong depth or order (the sky must be behind the lit scene; all sky layers are depth-tested, the clouds and the sun do not write depth and need their order) | the layers drawn in the wrong order | the original's row order (forward rows are prepared in row order, the comment at the forward state of `game_bootstrap.cpp` near line 9530) |

Whatever the branch, the ring (`Test_sun`, blend 2, depth write off, drawn after the dome and the clouds) needs: its texture delivered with alpha (DXT/BC format and the `native_encoded_texture_resident` row),
the alpha-blend state of the forward row (`SRC_ALPHA / INV_SRC_ALPHA`, already built for blend 2), and its place after the clouds in the row order.

## 4. Order of work I would take (pilot)

1. **Lightmaps** (§1.4): the Dominant record from `Banks`, one commit, check on the three witnesses by `Compare-SceneHdr.ps1`. It also moves the m40 tint (R/G/B 0.6/0.8/1) of the Firelink lead.
2. **Sky log** (§3.4): the by-name row and the uncapped gate-30 names, one run at m50, then the branch it shows. The sky is most of the picture at m50 (and the « native sky flat » of other zones), well worth more than the ring alone.
3. **Flames** belong to the effects phase (N5–N9). Do not start them inside wave 1 unless Thomas moves them; this note only fixes what the flame consists of and the first Reference log (§2.4).

Do not touch the sun shadow, the cascade casters or the glow for any of these.

## 5. Not found / limits

- The key source for the Dominant pair (§1.4 point 1): the original's setters and their caller chain are read (`FUN_140D25210`, `FUN_140D52B70`, `FUN_140088580`, `FUN_1400885C0`, `FUN_140088AF0`), but which function hands
  `FUN_140D52B70` its main key and whether that function runs natively were not read: log `ctx+0x3FA8..+0x4138` once natively and once in the Reference.
- Which texture and which file the lightmap of each named floor resolves to was not measured (§1.5 is the plan).
- The identity of the disc/ring piece is INFERRED from its name, size and side; no frame listing of the sky draws survives in the retained evidence (I-0053 captures were removed by the retention rule).
- The embers visible natively at the bonfire are not identified.

## Sources and method

Code: `native/world/producers.cpp` (`ProducePerFrame`, `ProduceTexHdrScale`), `native/world/light_producers.h` (`Banks`), `native/frame/game_bootstrap.cpp` (Dominant rows near 8806, `gate` lambda 7632, forward state ~9530, texture candidates ~5529), `native/frame/effect_system.h`, `native/content/model_programs.cpp`, `native/content/model_pass.cpp`.
Decompiled (existing exports): `decompiled/DS3-1152-F-00088AF0.c`, `...000888B0.c`; new exports for this note: `00088580`, `000885C0`, `00088C30`, `00D25210` (the Dominant pair setters and the slot apply), with `00D52B70` and `00E006A0` already exported. Data: gparam TSVs, `all-mtd.tsv`, `zone-water-tess.tsv`, `programs.tsv`, the m50 MSB (SoulsFormats), the FXRs 1020/1021 (`Read-Fxr.ps1`), the cascade side-by-side pictures. Nothing was run; no game, no save, no `main` change.
