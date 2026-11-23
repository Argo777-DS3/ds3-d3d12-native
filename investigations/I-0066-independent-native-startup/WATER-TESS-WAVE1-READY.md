# I-0066 / I-0068 part BO: water, refraction/reflection and tessellation of wave 1, ready to implement

Request (coordinator, 2026-10-11): after the shadows ([SHADOWS-WAVE1-READY](SHADOWS-WAVE1-READY.md)), a sheet "water and tessellation wave 1" on the same model: the water-surface families, refraction and reflection (with the link to `FrameCopy`), then tessellation and displacement, with the witnesses of the five zones (m31, m37, m45, m50, m51). Static: nothing was launched, native code was read and not changed. It builds on [WATER-NATIVE](../I-0068-static-yebis-ffx-lights-spec/WATER-NATIVE.md), [WATER-REFLECTION-INTERACTION-E08](../I-0068-static-yebis-ffx-lights-spec/WATER-REFLECTION-INTERACTION-E08.md), [TESS-SHADOW-FORWARD-NATIVE](../I-0068-static-yebis-ffx-lights-spec/TESS-SHADOW-FORWARD-NATIVE.md), [TESSSCENE-SSMASK-NATIVE](../I-0068-static-yebis-ffx-lights-spec/TESSSCENE-SSMASK-NATIVE.md), MENUS-MAP sections 67, 72, 73 ([MENUS-MAP](../I-0067-static-menus-subsystems-map/MENUS-MAP.md)), [WAVE1-PREP](WAVE1-PREP.md) 2.4, [D3D12-MASTER-PLAN](../I-0068-static-yebis-ffx-lights-spec/D3D12-MASTER-PLAN.md) S3 and the [phase E sheet](D3D12-PHASE-E-FRAME-GRAPH-SHEET.md).

Labels: **VERIFIED** (read in the decompilation, the DXBC or the game data, with the anchor), **OBSERVED** (retained captures, cited), **PROPOSED** (this sheet's design), **INFERRED**, **OPEN** (a live row or a data read is named). Anchors: `GB` = `native/frame/game_bootstrap.cpp`, `SP` = `native/renderer/scene_pass.cpp`, `MD` = `native/renderer/model_draw.cpp`, `MP` = `native/content/model_programs.cpp`, `MPS` = `native/content/model_pass.cpp`; `FUN_` numbers are original addresses, the decompiled files are in `decompiled/`.

Evidence (outside Git, `evidence/I-0068/queries/water-ready/`): `zone-water-tess.tsv` (every FLVER material of the five zones, with the shader family, the water flag and the GX04 record; `Get-ZoneWaterTess.ps1`), `zone-msb-reflect.tsv` (map pieces and collisions of the five MSBs with the reflection flags; `Get-ZoneReflectParts.ps1`), `water-reads.tsv`, `pom-reads.tsv`, `dol-pn-reads.tsv` (constant-buffer read sets of the programmes; `Scan-ProgramReads.ps1`), `capture-water-patch-scan.tsv` (water programmes and patch topologies per retained capture), `zone-tess-enemies.tsv`, `water-tess-blocks.tsv` (reference words; `Get-WaterTessBlocks.ps1`), and the recovery reports (`Recover-NamedGameData.ps1` recovers archive entries the pinned dictionary does not name: the DLC map pieces).

## 0. Answer first

1. **All five zones contain water, of one family, and none contains a water variant that needs tessellation or the ripple simulation.** `GXFlver_WaterBumpTri` (MTDs `M[ARSN]_Water_DL`, `Water_3normaltet`, `Water`): m31 3 pieces, m37 6, m45 4, m50 3, m51 2 (one of them the 84,048-index swamp). No material carries `GXFT_WaterInteraction` (0 MTDs) and no map material of the five zones has a tessellated water programme. Section 1.1.
2. **Only m37 draws water in the retained views.** The water programmes (`WatDpt` then `FwdSdw`) appear in 9 of the 11 retained m37 captures (one mesh pair per frame, the 9,261-index piece `m000023`) and in none of the 52 retained m31, m45, m50 and m51 captures (15, 17, 5 and 15); the identification is by index count against the game data (m37 `000023` = 9,261, Firelink `000012` = 30). This corrects the sheets that cite "the water at m31". The other four zones are witnessed by the data only until a camera is put near their water (live row WL12). Section 4.
3. **The engine binds t54..t57 itself, at the start of the view, with defaults.** `FUN_140103510` records four bind commands (slots 0x36..0x39 = t54..t57): t54 = the reflection target, or a **black 1x1** default; t55 = the frame copy; t56 = the readable depth; t57 = the interaction map, or a **grey 0x808080 1x1** default (the neutral 0.502). So "WATER-NATIVE risk 1" (provenance through `FUN_1401BF180`) is closed, and the neutral interaction map is exactly the default texture: no 640x360 target is needed until a simulation exists. The water pixel programmes read no IBL (t30/t31). Section 1.3.
4. **FrameCopy: one resource, copied twice when water exists.** The same SRV is bound as t45 (effects, forward rows) and t55 (water). In the retained m37 frames the HDR is copied into it **twice**: once right after `WatDpt` (what the surface refracts: it holds the underwater tint and, in its alpha channel, the water mask the surface programme tests) and once after the surface, the readable-depth copy and the depth downscale, before the post-water forward rows and the effects (so t45 **does** contain the water surface). A frame without water has the second copy only (m31). The native copy today sits before all forward rows (`SP:1587`): the sheet makes it "C2 at the start of the 0x29 group, after the 0x1F rows" and adds "C1 after `WatDpt`" when water rows exist. This corrects the master-plan S3 ("one copy ... does not contain the water") and its check C7. Section 1.4, commit WT-2.
5. **The reflection view is a half-resolution forward view of the parts flagged `DrawByReflectCam`, through a mirrored camera whose determinant is +1.** Camera: the mirror `I - 2 n n^T` with the translation row `-2 d n`, composed with a Y flip that restores the winding, so the draws keep the main view's raster state (cull back, front counter-clockwise: OBSERVED on the m37 reflection draws). Gather: mask low byte 0x10 (INFERRED to select the MSB flag); group 0x39 drops the water rows and the decals. The MSB flag is set on 3,765 of 3,782 pieces at m31 and on 2,312 of 6,635 at m37, so the three draws of the m37 capture are most likely a visibility result, not a flag result (INFERRED). Plane: reset every frame to a default `(0,1,0,0)`, lowered by the nearest water mesh of the main view (box centre, up axis) and by a height-based default, then copied into the reflection object when closer than its own (`FUN_140023790`, `FUN_1400575C0`, `FUN_140024740`, `FUN_1400885F0`); the collision `ReflectPlaneHeight` equals the water height for most collisions (m31 -155 on 89 of 93, m37 -241.76 on 59 of 63). The "hold 100 frames" counter of earlier notes belongs to another feature (1.5). Section 1.5, commits WT-6, WT-7.
6. **Tessellation in the five zones is small and exact.** Map side: one piece in m50 (`m001523`, 5 placements, trees `tree_002` and `tree_branches_002`: GX04 mode 2, factors 5/1, fade 10..20), drawn in the **G-buffer** through `GXFlver_ColDifSpcBumpDOL_GBuf_PN` (vertex, hull, domain). The MTD never decides (`M[ARSN]_l.mtd` has mode 0). Character side: `c6060` (hair cloth, mode 2) x27 at m45 is the only tessellated character of the five zones; `c6232` (m50) and `c2170` (m37) have mode 0. Patch topologies are drawn in every zone, but only by the FFX effects (1-control-point patch lists with hull and domain, EFFECTS-PHASE2-E01-E02); no retained capture (all 80 searched) draws a 3-control-point model patch. Section 1.7.
7. **Displacement splits in three, only one of them tessellation.** (a) Parallax relief (POM) in the pixel shader: m31 33 materials, m37 32, already admitted natively and reading no model-parameter word that is missing. (b) The `_D` domain displacement and the flow vertex offset (`FlowDisplacement`, `Slime`, `Eclipse`, water interaction): characters and the m30 arena only, none in the five zones. (c) Nothing else. Section 1.8.
8. **The two constant blocks of tessellation are now fully specified**: `cbPNTriangles` (transcribed from `FUN_1402302C0`, constants read: 64, 0.001, 1.0, 0.99; goldens for the 8 distinct GX04 records of the five zones and the characters) and `cbTessScene` (TESSSCENE section 1.3, unchanged). The D3D12 model path needs the hull/domain family, the patch topology and the flat fallback that [MODEL-DRAW-ENVELOPE-READY](MODEL-DRAW-ENVELOPE-READY.md) specifies as W0a-5 and W0a-6; this sheet consumes them (TS-1, TS-3) and adds the constants, the selection and the witnesses. Section 3.4.
9. **Design**: eight water commits WT-1..WT-8 and six tessellation commits TS-1..TS-6, every rung behind its own flag file; tests T-Q1..T-Q12; fourteen live rows WL1..WL14 (LIVE-CHECKS-CONSOLIDATED 1.15). The surface is drawn with `Fwd` until the shadow state exists, then `FwdSdw` (needs SW-4..SW-6).

## 1. The original, verified

### 1.1 Water families and where they are (data of the five zones, VERIFIED)

`mtd.tsv` (I-0063) maps the three water MTDs to `GXFlver_WaterBumpTri` (`Water_DL` and `Water_3normaltet` blend 2, `Water` blend 0; `Water_DL` adds `GXFT_WaterApplyDynamicLight`; all three set `g_GXFT_PlanarReflection`, `g_GXFT_Refraction`, `g_GXFT_WaterDepth`). Every water material carries the GX items `GX00 GX09 GX07` (m31 piece 008040 also `GX04`, mode 0): the native parser reads GX09 only, GX07 fills the phantom-parameter block `mat+0x1A0..0x1FC` (MENUS-MAP 67), which the water programmes do not read.

| Zone | Pieces (material: vertices / indices) | MTD | Placement | `DrawByReflectCam` | Collision `ReflectPlaneHeight` |
|---|---|---|---|---|---|
| m31 | 007700 (117 / 414), 008040 (98 / 360), 008315 (4 / 6) | `Water_DL` | (50, -155, -580) | 1 | -155 on 89 of 93 (others -198.49 x2, -208.5, -240.82) |
| m37 | 000020 (1776 / 9786), 000022 (408 / 2106), **000023 (1660 / 9261)**, 000024 (10 / 24), 000025 (20 / 72), 000542 (911 / 3210) | `Water_3normaltet` | (423.198, -241.76, -925.096) | 1 except 000025 (0) | -241.76 on 59 of 63 (-247.56 x1, -277.16 x3) |
| m45 | 003021 (2880 / 15855), 006100 (904 / 4260), 006101 (732 / 3888), 010440 (586 / 3198) | `Water_3normaltet` | (0, 0, 0), geometry baked in world | 0 | 8 of 187 non-zero (-63.6, -66.7, +4.1 ...) |
| m50 | 001363 (24 / 93), 020060 (63 / 252), 030201 (748 / 3936) | `Water_DL` | (0,0,0), (7.36, 0, 3.66), (22.4, 0, 6.66) | 1 | -72.767 on 4 of 126 |
| m51 | 020000 (194 / 621) `Water_DL`, **040002 `Swamp` (14265 / 84048) `Water`** | both | (-500, 0, -500) | 0 | none |

Reading: the collision `ReflectPlaneHeight` equals the water surface height for the main water level where the water is placed with a part position (m31, m37) and is a candidate default plane there; zones with several levels (the few other values at m31 and m37) and the zones where the water is baked (m45, m50, m51) need a per-area rule or the water candidate alone (WL11). The largest water is the m51 swamp (blend 0, opaque), which the native must be able to draw in one call (84,048 indices) and which cannot be a witness of the transparent path.

Other water-class families exist in the game and are not in the five zones: `GXFlver_Water` (`m33_00_Water.mtd`), the tessellated `GXFlver_WaterBumpTriInteraction[FloatObj]`/`WaterInteraction` (hull/domain programmes only, selected at run time by `GXWaterInteractionMaterialPatch`; the MTD flag bit 17 is set by 0 MTDs), `GXFlver_Slime`. They are listed so that the pass whitelist (WT-1) names them as refused with a reason.

### 1.2 Passes, rows, programmes and states (VERIFIED code, OBSERVED states)

Unchanged from WATER-NATIVE 1 and 2, restated with the new proof of the draw kind:

- **How a draw becomes a water draw.** `FUN_1400575C0`: for the main view (type 0x45) a material with the `g_GXFT_PlanarReflection` bit (4, mask 0x10) sets the draw kind byte to **0x27**, tracks the reflection plane candidate (1.5) and keeps the draw; the `g_GXFT_Refraction` bit (5, mask 0x20) sets `view+0x892` and the kind 0x27 too; views 1..0x38 and 0x39 set kind 0x27 for either bit.
- **Rows** (`FUN_140057A20`): a kind-0x27 descriptor yields `{engine 0x24, preparation 6, class mask 0xFFFF, secondary 0xFF}` and `{engine 0x27, preparation 0, 0xFFFF, 0xFF}`; a kind-0x27 descriptor gathered by a view of group 0x39 yields none (`57A20:245`).
- **Shader pass** (`FUN_14005A120`): preparation 6 -> pass 12 `WatDpt`; preparation 0 -> pass 0 `Fwd`, or `4 + (view+0x710 == 3)` when `view+0x710` is non-zero: **4 `FwdSdw`** (state 1, cascade atlas) or 5 `FwdSSdw` (state 3, screen-space mask). The state is the forward window of the shadow layers (SHADOWS-WAVE1-READY 2, `ShadowFrame`).
- **Programmes** (cache, `GXFlver_WaterBumpTri_*`): `WatDpt.vpo/.ppo`, `Fwd.vpo/.ppo`, `Fwd_[A].ppo`, `FwdSdw.ppo`, `FwdSdw_[A].ppo`, `FwdSSdw.ppo`, `FwdSSdw_[A].ppo`; the vertex stage of `FwdSdw`/`FwdSSdw` is the `Fwd` one; skinned and `@[cl]` variants exist and are not used by map water.
- **States** (OBSERVED, WATER-NATIVE 1; I-0055 and the I-0063 streams agree): `WatDpt`: blend colour SRC_ALPHA / INV_SRC_ALPHA / ADD, alpha ZERO / ZERO, mask RGBA; depth on, no write, LESS_EQUAL; cull back, front counter-clockwise. `FwdSdw`: the same blend with alpha ONE / ZERO; depth write on. Both bind two render targets (the HDR and GB5) with the programme writing only `o0`.

**Read sets of the programmes** (DXBC, `water-reads.tsv`, the native read analysis replayed): this is what the native producers must cover.

| Programme | Constant buffers read (fixed rows; D = dynamic index, i.e. the whole buffer) | Textures | Samplers |
|---|---|---|---|
| `WatDpt.ppo` | b1 rows 0, 3, 7, 10, 16; b2 rows 17, 19, 21 (of 352 B); b8 rows 1, 2 (of 48 B); b9 row 11 | t0, t3, t7, t10, t16, **t56** | s0, s2, s7 |
| `Fwd.ppo` | b1 rows 0, 1, 3, 7, 8, 10, 16; b2 rows 6, 17, 18, 19, 20, 22, 23; b3 rows 0..2; **b4 D**; b5 rows 0, 1, 4, 5, 8, 9, 12..14; b6 rows 1..4; b8 rows 1, 2, 5..15, 20..22, 55, 56 (of 912 B); b9 rows 3, 11 | t0, t1, t3, t7, t8, t10, t16, **t54..t57**, t59..t62 | s0, s1, s2, s7, s9 |
| `FwdSdw.ppo` | as `Fwd` plus b8 rows 28, 29, 54 and **b8 D** (cascade matrices indexed by register) | plus **t32** | plus **s12** (comparison) |
| `FwdSSdw.ppo` | as `Fwd` plus b8 row 29 | plus t32 (the mask) | plus s8 |
| `WatDpt.vpo` / `Fwd.vpo` | b4 rows 4, 5; **b5 D** (`cbObjMatrix`, 96 B, indexed); b8 rows 3, 4, 24..27; b11 rows 0..2 (`WatDpt`/`Fwd` identical) | - | - |

Notes: the pixel stage of the surface reads `cbInstanceData` by dynamic index (SH coefficients), so the 544-byte block must be complete; `FwdSdw` also needs the whole `cbSceneParam` (its dynamic read; the programme declares 912 bytes, the SW-5 image is 960), which is the SW-5 image. The vertex stage writes two `SV_ClipDistance` from `cbInstanceData.userClipPlane` (row 4/5): zero in the main view; the reflection view feeds the plane there (1.5). **Refraction alpha mask** (`FwdSdw.ppo` disassembly lines 816-819, `Fwd` and `FwdSSdw` likewise): the surface samples t55 twice (at the distorted and at the plain coordinate, sampler s9) and keeps the distorted sample only where the distorted sample's alpha is exactly 0, otherwise the plain one. With `WatDpt` writing alpha through ZERO / ZERO, the frame copy's alpha is therefore the water mask: the native `WatDpt` must write alpha 0 and every non-water pixel of the HDR must keep a non-zero alpha, or the refraction leaks foreground (WL7 reads the alpha).

### 1.3 Who binds t54..t57 (VERIFIED, `FUN_140103510`, lines 1440-1560 of the export)

At the start of the pass recording of a view the frame renderer queues "bind slot / SRV" commands (`FUN_1400BEDB0(slot, srv)`):

| Slot | HLSL | Condition | Value |
|---|---|---|---|
| 0x36 | **t54** `g_ReflectionTexture` | view group > 0x3A (the main view 0x45 and 0x3B..0x40) | the reflection object `R = frame+0x1D0` exists **and** the option byte `[[GX2+0x5360]+0x6C]` is set: the colour SRV at `R+0x240+0x28` (`R+0x660` when `DAT_144961BA0` is set); otherwise the default texture index 1 (**black**, `0xFF000000`) |
| 0x37 | **t55** `g_RefractionTexture` | same | `[frame+0x1108]+0x48`: the frame copy |
| 0x38 | **t56** `g_DepthTexture` | same | `[frame+0xFD0]+0x48`: the readable depth |
| 0x39 | **t57** `g_WaterInteractTexture` | every view | `[frame+0x1008]` (the interaction map) when `frame+0x3EC7` and `[[GX2+0x5360]+0x74]` are set; otherwise default texture index 2 (**grey** `0xFF808080`) |
| 0x2D | t45 (effects) | queued with the copy, 1.4 | `[frame+0x1108]+0x48` again |

The defaults are the 17-colour table built by `FUN_140006C00` at `GX1+0x270` (index 0 white `0xFFFFFFFF`, 1 black `0xFF000000`, 2 grey `0xFF808080`, then `0x800A0A0A` ...; format B8G8R8A8); the colours sit in consecutive 4-byte stack slots walked by one pointer (`6C00:794-888`), so the index order is that of the list. The native already builds the same words as `SYSTEX_BLACK` and `SYSTEX_GRAY` (`native/content/model_common_textures.cpp:89-105`): reuse them, no new 1x1 resources. Grey 128/255 = 0.50196 is the clear value of the 640x360 map seen at Firelink (half-float `3804`): **the neutral interaction map and the default texture are the same colour**, and the programme reads t57 `.x` only, so binding the default is exact for every witness. The m40 capture clears a `R16G16_FLOAT` target (the real map exists because the option is on) and nothing writes it (no capture has a character in water). Two root objects are meant by the abbreviations in this sheet: `GX1` = the object at `144961AB0` (default textures at `+0x270`, the reflection kill flag at `+0x3E1`) and `GX2` = the object at `144961B50` (options at `+0x5360`, the light/object list at `+0x41D0`); earlier notes call both "GXRS".

### 1.4 The frame copy and its relation to water and effects (VERIFIED + OBSERVED)

- **Command**: `FUN_1400BFA70(recorder, dst = [frame+0x1108], src = the HDR of the view)` records a copy (callback `1400BEA50`), or the other callback `1400BEAD0` when a format/sample test on the source passes (INFERRED: a multisample resolve; the native target is single-sampled); it is recorded under the pass header 0x29 (`103510:2040`), followed by the bind of slot 0x2D. This is the copy **C2** below. The recorder of **C1** (the water copy after `WatDpt`) was not found. A third copy in the same function (`103510:957`, `:967`) is the velocity copy (`+0x1118`).
- **Order** (OBSERVED; the I-0053 Firelink listing `gpu-order-i52-live-03-A-frame-300.txt` and the copy events of `i63-live-13/m37-s4/frame-60`, three frames): `GXLightComposite` -> forward `Col_FwdSSdw` rows (INFERRED engine 0x1F: the ForceForward `GXFlver_Col` family; no copy bound) -> clear of the interaction target -> `WatDpt` -> **C1: copy of the HDR (resource 19030) into the frame copy (19031)** -> `FwdSdw` -> copy of the depth into the readable depth -> two `DepthScale2x2` draws -> **C2: the same copy again** -> the post-water forward rows (three `Col_FwdSSdw`, now with the frame copy bound at t45) and the effects. In the m37 frames there are 6 copy events 19030 -> 19031 for 3 frames, i.e. 2 per frame; in the m31 frames (no water) 1 per frame. So t55 (surface) reads C1 (the `WatDpt` tint, the 0x1F rows, the water mask in alpha) and t45 (forward rows, effects) reads C2, which **contains the water surface**.
- **Demand**: the original copies C2 every frame and C1 only with water; the master plan S3 allows the native to skip C2 when no consumer exists (no 0x21/0x22/0x29 row and no effect that needs it).
- **Consequence for native** (PROPOSED): C2 at the start of the 0x21/0x22/0x29 group, after the 0x1F rows, after the surface and the readable-depth re-copy; C1 right after `WatDpt` when water rows exist. Today the single copy is at the start of the whole forward block (`SP:1587-1597`), which excludes the 0x1F rows (and, with water, the surface) from what the effects see: the rule fixes that too. Check C7 of the master plan becomes "1 `CopyResource` of the HDR without water, 2 with".

### 1.5 The planar reflection view (VERIFIED code; the plane semantics partly OPEN)

Driver `FUN_1400FB0E0` (called through `FUN_1402058C0`), gates: `[GX1+0x3E1] == 0`, `R = frame+0x1D0` exists and `R+0x3F7C` (armed), option `[[GX2+0x5360]+0x6C]`. The view is recorded as group **0x39** and run through `FUN_1400FCE30` -> `FUN_140103510`. `R` is itself an object of the frame-renderer class: it carries its own copies of the plane fields (`+0x3F7C..+0x3FA4`).

- **Camera** (VERIFIED, constants dumped): plane `P = R+0x3F90` = (n, d), normalised by |n| (a zero length keeps the zero vector). `M` (row-vector convention) has rows `(1 - 2nx², -2nx ny, -2nx nz, 0)`, `(-2nx ny, 1 - 2ny², -2ny nz, 0)`, `(-2nx nz, -2ny nz, 1 - 2nz², 0)`, `(-2d nx, -2d ny, -2d nz, 1)`, i.e. `p' = p - 2 (n·p + d) n`. It is multiplied with the camera view and with the constant matrix `(1,0,0,0) (0,-1,0,0) (0,0,1,0) (0,0,0,1)` (a Y flip, `143D7F1A0/143D7F4A0/143D7F690/143D1C080`). The product of two reflections has determinant +1: **the winding is preserved**, which is why the m37 reflection draws use the same raster state as the main view (OBSERVED: `[3,3,1,...]`, cull back) except `ps 2793` (cull none, a two-sided material). Goldens: the matrix for the m37 plane (n = (0,1,0), d = 241.76) is `row1 = (0,-1,0,0)`, `row3 = (0,-483.52,0,1)`; `water-tess-blocks.tsv`.
- **Size**: the target is `R+0x20/+0x24` (640x360 in a 1280x720 session, RGBA16F colour, `R32G8X24` depth with stencil); half of the output extent (INFERRED from the one session). The `+0x70 == 1` branch of the view parameters selects it.
- **Gather**: the mask passed to `FUN_1400FCE30` is the caller's mask with the low byte replaced by **0x10** (`FB0E0:326`); `FCE30` ORs bit 0 into the mask when `[[GX2+0x5360]+0x6E]` is set (group 0x39 only, `FCE30:144-146`) and walks the objects of `[GX2+0x41D0]` that match the mask only when `[...+0x6D]` is set (`FCE30:168-181`, with the extra gate `frame+0x3EBE`; INFERRED: the per-view light objects). Group 0x39 skips the deferred decals (`103510`, before `FUN_140051AA0`), skips water rows (`57A20:245`) and, for a kind-2 descriptor, rewrites the engine pass of the first of three rows from 2 to 0x1E (`57A20:694`; the rows of the half-resolution view: INFERRED reading, row table OPEN, WL9). The draws are **forward** draws into one colour target (the capture shows the `ps 2793`/`ps 342`/sky programmes with `cbLight`/`cbFog` and IBL, no G-buffer).
- **Which parts**: the candidate flag is the MSB part field `DrawByReflectCam` (and `DrawOnlyReflectCam`, set on one m45 piece): 3,765 of 3,782 map pieces at m31, 2,312 of 6,635 at m37, 0 at m45 (one `DrawOnlyReflectCam`), 2,100 of 3,653 at m50, 122 of 6,292 at m51 (`zone-msb-reflect.tsv`). The m37 capture draws three meshes (two map parts and the sky); INFERRED to be a visibility result (the part `draw_groups` could also cull: not checked); PROPOSED: gather the flagged parts through the ordinary visibility test. The mapping "mask 0x10 = DrawByReflectCam" is INFERRED (the gather service `_DAT_144961B78` is not decompiled); live row WL10 reads it.
- **Plane** (VERIFIED code; the scalar and the order of use OPEN): the plane lives in the frame object (`frame+0x3F90..0x3F9C` = (n, d), `+0x3FA0` = the scalar `t`, `+0x3F7C`/`3F7D`/`3F7E` flags) and is **reset every frame** by `FUN_140023790:261-271` (`+0x3F7D` takes `+0x3F7C`, then `+0x3F7C = +0x3F7E = 0`, the plane becomes the constant `1439F24E0` = (0, 1, 0, 0), `t = FLT_MAX`). Candidates: for each kind-0x27 draw of the main view `FUN_1400575C0` computes the surface normal (the draw placement's second axis, `p5+0x10..0x18`), the bounding-box centre `c = (min + max) / 2` and `t` (a dot of `(c, 1)` with `view+0x250..0x25C`), sets `view+0x891` and, when `t < view+0x894`, stores `t`, `n` and `d = 0 - n·c` (the constant at `1439F249C` is 0.0) at `view+0x894/0x8A0..0x8AC`: the plane through the box centre. A height default is built at `103510:1837-1928` when `frame+0x3ECD` is set and the flag `[frame+0x2F18]+0xC540` holds, from the value at `+0xC538` (INFERRED: the collision `ReflectPlaneHeight` under the camera target), setting `+0x3F7C`/`3F7E` and `t = 0.5` if the frame's `t` is above the constant `143D7EDB4`. At the end of a main frame (`103510:2277-2292`) the candidate is stored in the frame fields when its `t` is lower. The plane reaches `R` only through `FUN_140024740:302-317` and `FUN_1400885F0:69-75`, which copy it when the frame's `t` is below the reflection object's (and reset `R` first). So **the plane is the nearest water mesh of the frame, falling back on the height default and the constant**; whether the reflection of frame N uses the plane of frame N or N-1 depends on where `FB0E0` runs relative to that copy: OPEN (WL3). The byte `+0x3F7F` and the counter `+0x3F80 = 100` come from `view+0x893`, set when the gathered object has byte `0x86` set (`57A20:238-243`; group 0x3A skips objects without it); the counter gates `FUN_140205920` (which calls `FCE30`) and the `+0x3EC9` branch of `103510` (pass 0x2B, otherwise an HDR copy into `+0x1100`, lines 2049-2088): INFERRED to be the interaction-simulation hold, not a plane hold. `t` is not interpreted: OPEN.

### 1.6 The interaction (ripple) map (VERIFIED + OBSERVED)

No draw writes an `R16G16_FLOAT` target in the retained captures (WATER-REFLECTION-INTERACTION-E08 2 counted 123); the map is cleared to the neutral value each frame when the option is on, otherwise t57 is the grey default (1.3). The simulation (`GXWaterInteractionManager`, programmes `GXWaterInteractionHeightMap/NormalMap`, tessellated `WaterBumpTriInteraction*_FT`) has no material in the data of the five zones (flag bit 17: 0 MTDs; the run-time patch is the only entry) and no witness: **not planned**; the sheet binds the default.

### 1.7 Tessellation: selection, stages, data (VERIFIED)

Mechanism as TESS-SHADOW-FORWARD-NATIVE 1 (variant `<family>_<pass>[_Skin][_Ins]_PN[_D]` or `_FT`; hull `partitioning_fractional_odd`, `domain_tri`, 3 control points, factor cap 64; stages bound by `FUN_140142430`; patch topology 0x23), with these additions from the five zones:

- **The decider is the material's GX04/GX90 item** (MENUS-MAP 67: `d[3] & 1` displacement flag, `d[4]` mode, `d[5..6]` edge/inside factors, `d[7]`, `d[8..9]` fade start/end, `d[10]`, `d[11]`; the item is `{id, unk, size, data...}`). Of the 8,551 materials of the five zones that carry a GX04 (of 9,174 materials in all, `zone-water-tess.tsv`), **two have a non-zero mode**: m50 piece `001523`, `tree_002` and `tree_branches_002`, mode 2, factors 5 / 1, `d7` 0.5, fade start 10, end 20 and 19, `d10` 0.5, `d11` 1, displacement 0. No material has GX90. The MTD's `g_TessellationMode`/`g_DisplacementEnable` are 0 for these two (`M[ARSN]_l.mtd`).
- **Programmes of the m50 tree** (cache): `GXFlver_ColDifSpcBumpDOL_GBuf_PN` `.vpo/.hpo/.dpo`, `_GBuf_Skin_PN.vpo`, `_GBuf_Ins_PN.vpo` (and `@[cl]`), `_Dpt_PN`, `_DptA_PN`, `_Fwd_PN`; the pixel stage is the ordinary `GBuf` one. Note the name case `GBuf_PN` against `Gbuf` for the pixel programme. Hull and domain are shared by the skinned and instanced vertex variants.
- **Read sets** (`dol-pn-reads.tsv` for the vertex and pixel stages; the hull and domain from their DXBC): `GBuf_PN.vpo`: b4 rows 3, 5; b5 D (`cbObjMatrix`); b11 row 0; **no `cbSceneParam`** (the projection moves to the domain). Hull: **b5 `cbTessScene` rows 4..10** (eye, view vector, GUI parameters, the four planes; the declared buffer is 11 rows) and **b6 `cbPNTriangles` rows 0, 1 and 2.x**. Domain: **b8 `cbSceneParam` (row 4 of 5, the clip plane)** and **b5 `cbTessScene` rows 0..3 (the view-projection)**; it writes position, four varyings and two clip distances. The hull is `output_triangle_cw`: the raster winding of patch draws must be checked (WL14). Pixel `GBuf.ppo`: b0 row 0, b1 rows 0, 1, 3, b2 row 1, b3 rows 0, 2, 5, b4 D, b9 rows 12, 13 (unchanged). `GXFlver_ColDifSpcBumpDOL_DptA_PN` has a domain shader but no hull shader in the cache (depth pass, wave 3: the hull rule must tolerate it).
- **Selection rules** (original: TESS-SHADOW-FORWARD-NATIVE 1.1, STATIC): the mode is forced to 0 for the velocity pass kinds, when `ctx+0x18C` bit 4 is set, when `ctx+0x18 != 0` and for LOD > 0. The native `EffectiveOrdinaryModelDeformation` (`MPS:16`) implements only `disabled || requested_lod ? 0 : base`; nothing sets `DisableDeformation` and nothing forces the velocity rows, so **TS-2 must add those rules** (row 0x34 would otherwise ask for a tessellated `Velo` programme, which does not exist). In the native, `bound.base_deformation = mode == 1 ? 1 + (secondary != 0) : mode == 2 ? 3 + (secondary != 0) : 0` already exists (`GB:4367`) and reads the instance fields `+0x250/+0x254`, which nothing writes natively (no GX04 parse): the veil/tree is therefore drawn flat today, not refused; once the fields are written without the programmes, a non-zero deformation is refused at gate 27 (`GB:5052`).
- **Characters in the five zones** (MSB enemies, `zone-tess-enemies.tsv`): `c6060` x27 at m45 (`Hair_cloth`, mode 2, family `GXFlver_ColDifSpcBumpIbl_Bloody_Cloth_*_PN`; its GX04 factors are in the DLC chr archive, **OPEN**: a data read, WL13); `c6232` x14 at m50 (`KAGE`, `Veill_NoLight`, mode 0); `c2170` x1 at m37 (the veil: MTD mode 2, GX04 mode 0, not tessellated).
- **Observed**: the FFX effects use hull and domain stages (1-control-point patch lists, topology 33, e.g. hull 10836 / domain 10861; 75 of the 80 retained captures, 195 to 1,869 draws per zone: `capture-water-patch-scan.tsv`; EFFECTS-PHASE2-E01-E02 6.1), and the native replay host already builds such PSOs (`native/world/live_host.cpp:2260-2280`, `PrimitiveTopologyType = PATCH`, reusable for TS-3 and T-Q7). **No capture draws a 3-control-point patch (topology 35)** in any zone: model tessellation has never been in a retained view. The 35 test is not vacuous (the field uses the D3D11 enumeration: triangle list 4, strip 5, 1-control-point patch 33).

### 1.8 Displacement (VERIFIED)

| Kind | Where | Mechanism | Status |
|---|---|---|---|
| Parallax relief (POM) | m31: 33 materials (`ColDifSpcPomDOL` 16, `PomMulMaskDOL` 13, `PomMulMaskIbl` 4); m37: 32 (`PomIbl` 20, `PomDOL` 4, `PomMulMaskDOL` 6, `PomMulMaskIbl` 2); MTD `g_DisplacementEnable` 1 | pixel shader only; reads `cbMatDynParam` rows `FC_PomHeightScale/MinSamples/MaxSamples` (+64/+72/+76), `FC_Pom2HeightScale` (+112), `FC_BlendMaskParam` (+48) and `FC_TranslucencyScale`; **no** `cbvModelParam`, so `VFC_PomLightDir` is not read (`pom-reads.tsv`) | admitted (NATIVE-GATES-AUDIT 2.2); the image is row V1, not this sheet |
| Tessellation displacement `_D` | `FlowDisplacement` (`c3140`, `c3141`, m30_01), `Slime`, `Eclipse`, `WaterBumpTriInteraction*` | domain shader samples `g_DisplacementTexture` t5 / s4 (and `cbTessScene.g_fGlobalTime` in 24 programmes); `cbPNTriangles` row 2 scales it | none in the five zones; commit TS-6 deferred |
| Flow vertex offset | `FlowmapVertexOffset` (`c1090`, `c1220`, `c2060`, `c5110`) | vertex-texture fetch, no tessellation | characters, not this sheet |

## 2. Native status today (anchors on `main`)

| Part | Native today | Anchor |
|---|---|---|
| Shader passes | `SelectModelPrograms` accepts 0, 1, 6, 7 only; the name table is `Fwd/Velo/Gbuf/Gbuf2` | `MP:40-54` |
| Engine rows | the row loop covers `{5, 6, 7, 0x1F, 0x21, 0x22, 0x29, 0x34}`; forward rows are one family with preparation 0, shader pass 0 | `GB:4951`, `GB:4998-5000`, `MPS:45-46` |
| Forward encode | all forward draws (0x1F, 0x21, 0x22, 0x29) in one block after the lighting; the scene-colour copy at its start | `SP:1584-1600`, `GB:5455` |
| Frame copy | `scene_colour_copy_` (HDR size), created on demand, copied once per forward block | `SP:1326-1332`, `SP:1587-1597`, `scene_pass.h:190` |
| t54..t57 | no binding | - |
| Readable depth | no copy resource: `depth_` is moved to `DEPTH_READ \| PIXEL_SHADER_RESOURCE` in places (`SP:1081`, `:1266`, `:1962`), which serves a read-only draw; the shared `DepthRead` of the master plan does not exist yet | `SP` |
| Reflection, interaction | absent | COVERAGE-MATRIX rows 31, 42 |
| GX04/GX90 | not parsed (the C2 loop reads GX03, GX05, GX09); `base_deformation` therefore 0 | `GB:8409-8421`, `GB:4367` |
| Model PSO | VS + PS only, `PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE`, triangle list, 1 or 6 render targets, root signature with two stage tables and `DENY` hull/domain/geometry | `MD:20-23`, `MD:244`, `MD:327`, `MD:346`, `MD:418` |
| `cbPNTriangles`, `cbTessScene` | no producer | WAVE1-PREP 2.4 T4, T5 |
| Water material words | `cbMatDynParam`/`cbMtdParam`/`cbTextureTilingScale` come from the generic SPX declaration + MTD producers; GX09 is parsed | `model_parameters.cpp:17`, `GB:3363-3384`, `GB:8420` |

## 3. Design

### 3.1 Water rows and programmes (WT-1)

1. **Classification**: a material with MTD policy bit 4 or bit 5 (`MaterialPolicyWord`, MTD-FLAG-BITS-INVENTORY-SHEET rows 4 and 5, golden word `0x70`/`0x40070` already specified by its test F4) is a kind-0x27 draw in the main view. Both rows 0x24 and 0x27 are emitted for every kind-0x27 descriptor (`57A20:626-685`); bit 6 (`WaterDepth`) has no reader (MTD-FLAG-BITS-INVENTORY-SHEET row 6).
2. **Rows**: add `0x24` (preparation 6, shader pass 12, class mask 0xFFFF, secondary 0xFF) and `0x27` (preparation 0, shader pass 0, 4 or 5) to the row loop `GB:4951`, to the selector check `MPS:45-46` and to `forward_row` (`GB:4998`) as a separate `water_row`; `required_mesh_mask` 1 as the other rows. A kind-0x27 draw gathered by a group-0x39 view gets no row.
3. **Pass names**: `MP:40-54` gains `4 FwdSdw`, `5 FwdSSdw`, `12 WatDpt` with the `[A]` pixel variants for 0, 4 and 5 (`Fwd_[A]`, `FwdSdw_[A]`, `FwdSSdw_[A]`, 1.2); skinned and `@[cl]` variants stay refused for water.
4. **Count row** `native_water_rows` per zone: materials by MTD, prepared meshes by engine pass, refusals by reason (the gate histogram `GB:4940`).

### 3.2 Order, resources and states (WT-2..WT-5)

PROPOSED native order inside the forward block (replacing `SP:1584-1600`), every step skipped when it has no rows:

| # | Step | Target / reads | Source of the order |
|---|---|---|---|
| 1 | forward rows engine 0x1F | HDR (`native_light_targets_[3]`), depth test | 1.4 (GPU order) |
| 2 | (no clear of the interaction target: t57 = the grey default) | - | 1.3 |
| 3 | `WatDpt` rows (engine 0x24) | HDR only (GB5 untouched, WL6), depth read-only, blend as 1.2, t56 = readable depth; **alpha written 0 through ZERO / ZERO, the rest of the HDR keeps a non-zero alpha** (1.2, refraction mask) | 1.2 |
| 4 | **C1**: FrameCopy <- HDR (only when water rows exist) | what t55 will read | 1.4 |
| 5 | surface rows (engine 0x27) | HDR, depth write on, blend as 1.2; t54 black until WT-7, t55 = FrameCopy (C1), t56 = readable depth, t57 grey | 1.2, 1.3 |
| 6 | readable-depth re-copy and the depth downscale (the surface wrote depth) | `DepthRead` | 1.4 |
| 7 | **C2**: FrameCopy <- HDR (always, or when a consumer exists) | what t45 will read; contains the surface | 1.4 |
| 8 | forward rows 0x21, 0x22, 0x29 and the effects | HDR, t45 = FrameCopy (C2) | 1.4 |

The existing barriers (`SP:1589-1599`) are kept for the copies; the split of `forward_draws` by `prepared.engine_pass` is the only structural change (the vector order is preserved inside each group). Without water rows the output equals today's, except that the copy now follows the 0x1F rows (the original's behaviour) and is taken once, as C2.

Resources: t54 and t57 reuse the native `SYSTEX_BLACK` and `SYSTEX_GRAY` images (`model_common_textures.cpp:89-105`, the exact `FUN_140006C00` words, admitted at `GB:832`): no new 1x1 resources. The frame copy is `scene_colour_copy_`. **The readable depth does not exist natively** (section 2): WT-2 creates it, or declares its dependency on the shared-copies step (NATIVE-PASS-ORDER rank 1, DECALS-NATIVE 1; the original binds `[frame+0xFD0]`, copied after the G-buffer and re-copied after the surface); the surface writes depth while sampling t56, so the sampled copy cannot alias the depth buffer. The phase E planner declares the water passes as members of slot 11 (phase E sheet, slot table) with `HDR` read/write and `DepthRead` read, so no barrier is hand-written once E3 lands.

Shadow selection (WT-5): while no `ShadowState` exists the surface is drawn with `Fwd` (the original's `view+0x710 == 0` result, documented in TESS-SHADOW-FORWARD-NATIVE 4.4); once SW-4..SW-6 exist the shader pass follows the `ShadowFrame` forward window at the row's position (1 -> `FwdSdw`, 3 -> `FwdSSdw`), which needs the 960-byte `cbSceneParam` image of SW-5, t32, s12.

### 3.3 Reflection (WT-6, WT-7)

- **WT-6, plane state (CPU)**: `WaterReflectionState { plane (n, d); t; flags }` **reset every frame** to the default plane (0, 1, 0, 0) with `t = FLT_MAX`, lowered in the frame by the height default (when the zone supplies one: the MSB collision `ReflectPlaneHeight`, a single value for the main water level at m31/m37, per-area for m45/m50: OPEN, WL11) and by the nearest water draw of the main view (the draw placement's up axis, the box centre, `t` = the view-axis distance of the centre: PROPOSED until WL3 reads the original's `t`), then copied to the reflection object when closer than its own (1.5). There is **no 100-frame hold** in this state. Row `native_water_plane` per frame. Opt-in `water-plane.flag`.
- **WT-7, the view (GPU)**: a second visibility pass over the parts flagged `DrawByReflectCam` with the mirrored camera of 1.5 (the `cbSceneParam` camera words rebuilt from the product; the plane in `cbInstanceData.userClipPlane`), forward rows only (shader pass 0, no water rows, no decals), into a `RGBA16F` colour + `R32G8X24` depth target of half the output extent, cleared to (0,0,0,1) (`1439F2500`), then bound as t54. The target pair is a transient of the frame plan (phase E sheet, item 18 of its complement list); the view machinery is the one that already draws the menu views (`SwapTargetSet`). Opt-in `water-reflection.flag`; depends on WT-4, WT-6 and on the second-gather support of the collect side (workstream K).

### 3.4 Tessellation (TS-1..TS-6)

1. **TS-1, parse and flat fallback (CPU)**: transcribe the GX04/GX90 rows of MENUS-MAP 67 into the instance fields (`+0xDF`, `+0x250`, `+0x254`, `+0x258`, `+0x25C`, `+0x260`, `+0x264`/`+0x268` kept 0, `+0x26C`, `+0x270`, `+0x274`, `+0x278`, GX90: `+0xE4`), beside the GX09 branch (`GB:8420`). The flat fallback is the one MODEL-DRAW-ENVELOPE-READY §7 (W0a-6) specifies: `NativeModelPassFlag::DisableDeformation` is set for every mesh unless `native-tess.flag` is on **and** the tessellation recipes resolved, so `base_deformation` may be non-zero in the data while the draw stays flat and gate 27 stays empty. Row `native_tess_materials` (per zone: materials with mode, factors, fade).
2. **TS-2, selection**: `ModelProgramSelection` gains hull and domain indices; suffix rules `_PN`/`_FT` (+ `_D`) after `_Skin`/`_Ins` for the vertex name, `<base>_PN[_D].hpo/.dpo` without `_Skin`/`_Ins` (a depth pass with a domain but no hull, `DptA_PN`, is tolerated or refused with a reason); the pass name case is taken from the cache (`GBuf_PN` vertex/hull/domain, `Gbuf` pixel). **New rules in `EffectiveOrdinaryModelDeformation` or its caller** (1.7): mode 0 for the velocity rows (`Velo` has no tessellated variant), for `ctx+0x18C` bit 4, for `ctx+0x18 != 0`, for LOD > 0 (already there).
3. **TS-3, PSO and root signature**: consumes W0a-5 of [MODEL-DRAW-ENVELOPE-READY](MODEL-DRAW-ENVELOPE-READY.md) (the `ModelTess` family: `Stage()` for hull and domain, a second root signature with four stage tables of the same 14 CBV + 128 SRV + 16 sampler layout and no hull/domain `DENY`, `PRIMITIVE_TOPOLOGY_TYPE_PATCH`, the 3-control-point patch list for the draw, `HS`/`DS` in the PSO key); this sheet adds the stage contents: HS b5/b6, DS b5/b6/b8 (and t5/s4 for `_D`). Programmes are DXBC SM5 (`domain_tri`): D3D12 accepts them unchanged; the replay host already builds such PSOs (`native/world/live_host.cpp:2260-2280`). Render targets as W0a: 6 (G-buffer) or 1 (forward).
4. **TS-4, constants (CPU, golden)**:
   - **`cbPNTriangles`** (0x30 bytes, `FUN_1402302C0`; runs only when `mat+0xDF != 0` and the mode `!= 0`): row 0 = `(min(edge, 64), min(inside, 64), fadeStart, max(fadeEnd - fadeStart, 0.001))` with `0.001` substituted when the difference is `<= 0.001`; row 1 = `(1 - d7, min(mat+0x264, 0.99), 1.0, float(mat+0x268))` where words 1 and 3 stay 0 natively (the constructor values) and word 2 is the constant 1.0; row 2 = `(displacement ? (mat+0xE4 ? exparams+0x24 : 1.0) * d11 : 0, d10, -, -)` (words 10 and 11 are the stack cookie in the original; the hull reads `cb6[2].x` only: write 0). Uploaded to `[[144961B50]+0x4DD0]+0x2C8`, bound HS b6 and DS b6.
   - **`cbTessScene`** (0xB4 bytes per view): TESSSCENE-SSMASK-NATIVE 1.3 unchanged (absolute-world view-projection at 0, eye at 64, `-forward` at 80, `(1,0,0,0)` at 96 until WL8 reads it, four unit inward planes `row3 + row0`, `row3 - row0`, `row3 + row1`, `row3 - row1` at 112, the clock at 176). New here: the hull reads `cb5[0..10]`, the domain `cb5[0..3]` only; the buffer is bound to HS b5 and DS b5 (not VS, whose b5 is `cbObjMatrix`: the stage tables keep them apart).
5. **TS-5, enable**: `native-tess.flag` lets the W0a-6 fallback release the mesh; the G-buffer `GBuf_PN` for the m50 trees and the forward `Fwd_PN` for the veil/hair; the depth prepass `Dpt_PN` follows the shadow casters (wave 3, not here).
6. **TS-6, `_D`**: DS t5 / s4 (`g_DisplacementTexture`), `cbTessScene` time, `cbPNTriangles` row 2: deferred, no witness in the five zones (characters and m30).

## 4. Witnesses of the five zones

| Zone | Water in the data | Water in the captured views | Reflection data | Tessellation |
|---|---|---|---|---|
| m31 Undead Settlement | 3 pieces, `Water_DL`, 414 / 360 / 6 indices, y = -155 | none (15 captures searched) | flag on 3,765 pieces; plane -155 | none (GX04 mode 0 on all 1,995) |
| m37 Irithyll | 6 pieces, `Water_3normaltet`; `000023` has 9,261 indices | **yes, 1 pair per frame, 9,261 indices** (9 of the 11 captures; the two without are frame 250 views) | flag on 2,312; reflection view of 3 meshes OBSERVED (`ps 2793` 5,520, `ps 342` 6,108, `ps 295` 789 indices) | none; `c2170` x1 mode 0 |
| m45 Ariandel | 4 pieces, `Water_3normaltet`, up to 15,855 indices, world-baked | none (17 captures) | `DrawByReflectCam` 0; 8 collision planes | **`c6060` x27, mode 2, hair cloth** (factors OPEN, WL13); no map material |
| m50 Dreg Heap | 3 pieces, `Water_DL` (mud) | none (5 captures) | flag on 2,100; plane -72.767 | **map `m001523` x5 (trees): mode 2, GBuf_PN**; `c6232` mode 0 |
| m51 Ringed City | 2 pieces, `Water_DL` 621 and **`Water` 84,048 indices** (opaque swamp) | none (15 captures) | flag on 122; no plane | none |
| m40 Firelink (reference) | `000010` (2,112), `000011` (144), **`000012` (30)** | yes, 30 indices (I-0055, I-0053 listing) | - | - |

Use: m37 is the only zone that can validate the structure and the image (W1..W5 of WATER-NATIVE, R1/R2 of WATER-REFLECTION-INTERACTION-E08) today; m40 validates the small mesh; m31, m45, m50, m51 need one captured camera placed near their water (the part positions above are the starting points; WL12). The m51 swamp is the large-index and opaque-blend witness; m50 `001523` is the only tessellated map witness (its trees are at (-75, 66, 178) and four neighbours); `c6060` at m45 the only tessellated character; the veil `c5270` (m30) stays the forward witness of WAVE1-PREP T-item 4.

## 5. Commits (each builds, passes the unit tests, is reversible alone; every rung is opt-in by a flag file until its exit holds)

| # | Commit | Touches (anchors by function) | Flag | Exit |
|---|---|---|---|---|
| **WT-1** | water rows and pass names (3.1): classification, rows 0x24/0x27, `MP` whitelist 4/5/12, row `native_water_rows`; nothing drawn | `GB:4951-5000`, `MPS:45`, `MP:40-54` | `water-rows.flag` | T-Q1; per-zone counts 3 / 6 / 4 / 3 / 2 prepared meshes, refusals empty |
| **WT-2** | defaults, forward split, FrameCopy positions and the readable depth (3.2 steps 1, 4, 6, 7, 8): `SYSTEX_BLACK`/`GRAY` as t54/t57, the split of `forward_draws` by engine pass, C2 after the 0x1F rows, C1 hook, the `DepthRead` copy if absent | `SP:1584-1600`, `model_draw` caches | `framecopy-order.flag` | T-Q3, T-Q10: frames without water rows identical except the copy content; 1 HDR copy per frame; live row WL2 |
| **WT-3** | `WatDpt` draw (3.2 step 3): programme set, states, t56, cbuffers | `GB:5455` area, `MD` | `water-depth.flag` | T-Q2; m37 structure W1/W2 of WATER-NATIVE; image of the fog tint |
| **WT-4** | surface draw with `Fwd` (steps 4-6): C1, t54 black, t55, t56, t57 grey, depth write, depth re-copy | as WT-3 | `water-surface.flag` | m37 W4 (mean absolute difference <= 2/255 over the water pixels), W5; the refraction alpha mask (WL7); m40 30-index mesh |
| **WT-5** | shader pass selection by the forward window (`FwdSdw`/`FwdSSdw`), the full `cbSceneParam` image | `ShadowFrame` consumer | `water-shadow.flag` | after SW-4..SW-6: row WL1 (the window at the surface row) equals the original's |
| **WT-6** | `WaterReflectionState`, row `native_water_plane` (3.3) | new `native/frame/water_reflection.*` | `water-plane.flag` | T-Q11; plane equals WL3 at m37 and m31 within the tolerance of the live row |
| **WT-7** | reflection view (3.3), t54 bound | `scene_pass`, collect K | `water-reflection.flag` | T-Q4; R1/R2: one 640x360 view per frame at m37, three draws, same index counts |
| **WT-8** | cleanup after the exits hold at m37, m40 and one placed capture per other zone | - | - | flags become defaults |
| **TS-1** | GX04/GX90 parse with the W0a-6 flat fallback, row `native_tess_materials` | `GB:8409-8421` | `native-tess.flag` (release of the fallback) | T-Q5 against `zone-water-tess.tsv` and `all-chr-gx.tsv`; gate 27 empty with the flag off |
| **TS-2** | `_PN/_FT[_D]` selection, hull/domain lookup | `MP`, `ModelProgramSelection` | `native-tess.flag` | T-Q6 on the m50 tree, the veil, `c6060` |
| **TS-3** | the hull/domain stage contents on the W0a-5 family | `MD:20`, `:244`, `:343-347`, `:418` | `native-tess.flag` | T-Q7: PSO creation for all 172 `_PN` and 110 `_FT` hull/domain pairs on a WARP device |
| **TS-4** | `cbPNTriangles` and `cbTessScene` producers | beside `ProduceCameraConstants`, material producers | `native-tess.flag` | T-Q8, T-Q9 |
| **TS-5** | enable: G-buffer `GBuf_PN` and forward `Fwd_PN` draws | `GB:4998`, `MD` | `native-tess.flag` | WL8/WL9 at m50 and m45 (or m30 for the veil): patch count per draw, silhouette |
| **TS-6** | `_D` displacement | - | - | deferred, no witness |

Order: WT-1 -> WT-2 -> WT-3 -> WT-4 -> {WT-5, WT-6} -> WT-7 -> WT-8; TS-1 -> TS-2 -> TS-3 -> TS-4 -> TS-5. The two chains are independent; both use the model-draw envelope of [MODEL-DRAW-ENVELOPE-READY](MODEL-DRAW-ENVELOPE-READY.md) (WT-3/WT-4 one render target, TS-3 the patch envelope that sheet reserves) and the shared files `GB`, `SP`, `MD` at the blocks named above.

## 6. Tests and goldens

- **Goldens** (`water-tess-blocks.tsv`, `Get-WaterTessBlocks.ps1`): `cbPNTriangles` words for the 8 distinct GX04 records of the five zones and the characters (the m50 tree: `40A00000 3F800000 41200000 41200000 | 3F000000 00000000 3F800000 00000000 | 00000000 3F000000`; the veil: `40E00000 40400000 00000000 41A00000 | ...`), the default colours, the half-float neutral value `3804`, the mirror matrices of the m37 and m31 planes and one tilted plane.
- **Data goldens**: `zone-water-tess.tsv` (water pieces, indices, GX04 rows), `water-reads.tsv`, `dol-pn-reads.tsv`, `pom-reads.tsv`, `zone-msb-reflect.tsv`.

| ID | What | Pass |
|---|---|---|
| T-Q1 | prepared water meshes per zone from the game data, with the pass whitelist | 3 / 6 / 4 / 3 / 2; indices equal the TSV; no refusal |
| T-Q2 | the native read analysis over the `WatDpt`/`Fwd`/`FwdSdw`/`FwdSSdw` programmes against `water-reads.tsv`; every fixed row is covered by a producer, `b4`/`b5`/`b8` dynamic reads force the complete buffers | equal; the only uncovered words are the shadow words until SW-5 |
| T-Q3 | the default images are the `SYSTEX_BLACK`/`SYSTEX_GRAY` words; the half-float neutral value | equal to the TSV |
| T-Q4 | mirror matrix for the three sample planes; properties: determinant of `M` is -1, `M·M = I`, and with the Y flip the camera product has determinant +1 | word for word; properties within 1 ulp |
| T-Q5 | GX04/GX90 parse: instance fields for the 2 m50 materials, the 6 character veil/cloth materials, and a mode-0 material | fields equal; with the flag off the effective deformation is 0 for every mesh (W0a-6) |
| T-Q6 | programme selection: m50 tree -> `GXFlver_ColDifSpcBumpDOL_GBuf_PN` vertex/hull/domain + `Gbuf` pixel; veil -> `Veill_NoLight_Fwd_Skin_PN` + `_PN` hull/domain; `Velo` -> flat | equal names, `Missing` never |
| T-Q7 | PSO creation with HS/DS for all hull/domain pairs of the cache (WARP), patterned on `live_host.cpp:2260-2280` | all succeed; the patch topology type is set |
| T-Q8 | `cbPNTriangles` producer against the goldens | word for word |
| T-Q9 | `cbTessScene` for a synthetic camera: matrix equals `transpose(view·proj)` with translation, planes unit and inward for the frustum centre, view vector `-forward` | within 1 ulp |
| T-Q10 | forward split: order inside each group preserved; barrier and copy counts for frames with and without water | 1 HDR copy without water, 2 with (C1 after `WatDpt`, C2 after the depth downscale) |
| T-Q11 | `WaterReflectionState`: reset each frame, nearest candidate wins, the default plane, the guarded copy | the table of 1.5 |
| T-Q12 | the water draws never enter a group-0x39 view; a kind-0x27 draw gets no row there | empty |

## 7. Live rows (none taken; LIVE-CHECKS-CONSOLIDATED 1.15)

| ID | Kind | Read | Expected |
|---|---|---|---|
| WL1 | order | the forward window (`view+0x710`) at the `0x27` rows and at the 0x1F rows (SHADOWS SL rows) | 1 at the surface (OBSERVED `FwdSdw`), 3 at the `Col` rows of m40 |
| WL2 | order | the GPU order of a frame with water: which forward rows precede `WatDpt`; the HDR `CopyResource` events (expected 2 per frame, 1 without water) and the depth copy | 0x1F rows first; C1 between `WatDpt` and the surface, C2 after the depth downscale (FC1, corrected) |
| WL3 | state | `view+0x891/0x894/0x8A0..0x8AC` after the gather, the frame fields `+0x3F7C..+0x3FA4` and `R+0x3F7C..0x3FA4` before and after the copy of `FUN_140024740`/`FUN_1400885F0`, and where `FB0E0` runs relative to it, at m37 | `n` = (0,1,0), the plane through y = -241.76; the meaning of `t`; whether frame N or N-1 feeds the view |
| WL4 | camera | the reflection view's `cbSceneParam` camera words at m37 against 1.5 | equal to `M · view · flipY` |
| WL5 | options | `[[GX2+0x5360]+0x6C/0x6D/0x6E/0x74]`, `[GX1+0x3E1]`, `frame+0x3EC7/0x3EC9/0x3ECD` at the five zones | `0x6C` set for HIGH, `0x74` set at Firelink |
| WL6 | targets | GB5 around the water draws (two targets bound, one written) | unchanged by the water (the native binds one) |
| WL7 | provenance | the SRVs at t54..t57 of the surface draw (slot range extended to 64): t54 content at m37 and Firelink, t55 = frame copy (C1), t56 = depth, t57 grey or the map; **the alpha channel of the frame copy at water pixels and elsewhere** | as 1.3; alpha 0 on water pixels, non-zero elsewhere |
| WL8 | block | `cbTessScene` and `cbPNTriangles` on a tessellated draw (m50 tree, `c6060`, or the m30 veil: TS1) | the golden words; `g_f4GUIParams2.x` = 1.0 |
| WL9 | rows | the engine rows of a group-0x39 view (the 0x1E rewrite) and the programme names of the three m37 reflection draws | forward `Fwd` programmes, rows 0x1E/0x29 |
| WL10 | gather | the part mask test of the reflection view: which MSB flag the mask 0x10 selects (a part with the flag cleared inside the frustum must be missing) | `DrawByReflectCam` |
| WL11 | plane | the plane against the collision `ReflectPlaneHeight` at m31, m45, m50 | equal at m31/m37; the per-area rule for m45/m50 |
| WL12 | witness | one capture near the water of m31 (50, -155, -580), m45, m50 (22.4, 0, 6.7), m51 swamp: the water programmes drawn, indices | the indices of 1.1 |
| WL13 | data | the GX04 record of `c6060` `Hair_cloth` (DLC chr archive) | mode 2 and its factors |
| WL14 | draw | patch topology draws at m50 near `m001523` | 3-control-point patch list, GBuf_PN programmes |

## 8. Risks and open points

1. **The meaning of the plane scalar `t`** (`FUN_1400575C0`) and the order of use (frame N or N-1): the code is exact, the quantity is not named. The native plane (WT-6) can run on the box-centre height (m37, m31) before WL3.
2. **Reflection rows and gather** are read to the extent of 1.5; the 0x1E rewrite and the mask-to-flag mapping are INFERRED (WL9, WL10). WT-7 is the least certain rung and is last.
3. **Water on a single render target**: the original binds HDR and GB5 with a programme that writes `o0` only (WL6). If GB5 changes around the water, WT-3/WT-4 need the two-target envelope of WAVE1-PREP W0a.
4. **`FwdSdw` needs the whole shadow state** (SW-4..SW-6); until then the surface is `Fwd`, brighter in shade.
5. **FrameCopy positions** change what effects see (the 0x1F rows and, with water, the surface now included in C2); the image check is FC1/WL2 at m37. The recorder of C1 was not found: its position is taken from the captures only.
6. **Tessellation without a captured witness**: the m50 trees and the m45 `c6060` have never been in a retained view; the first draws are verified by WL14 and the patch-count row, not by an image comparison. The flat fallback is exact in the interior (TESS-SHADOW-FORWARD-NATIVE 0.3).
7. **`c6060` factors** are a data read (WL13).
8. **Interaction simulation, E08 light shafts, water `Interaction` programmes**: not planned (no witness, no material).
9. **Refraction alpha mask**: the frame copy's alpha is the water mask (1.2); a native HDR whose alpha is 0 outside water (or whose `WatDpt` alpha is not 0) breaks the refraction (WL7).
10. **Readable depth** does not exist natively (section 2); WT-2/WT-4 depend on it.

## 9. Corrections to earlier notes

| Note | Statement | Correction |
|---|---|---|
| SHADOWS-WAVE1-READY 5 (m31 row) and TESS-SHADOW-FORWARD-NATIVE 4.4 | "water `FwdSdw`" at m31 | no water programme is drawn in the m31 captures; the m31 water is in the data only (1.1, 4) |
| WATER-NATIVE 2.3, risk 1 | t54..t57 come from the binder `FUN_1401BF180`; "also bound: IBL t30/t31" | the frame renderer binds them (`103510:1440-1560`); t30/t31 are bound on the draw (the capture shows them) but no water programme declares them (1.3) |
| WATER-NATIVE 3, step 2 and risk 3 | "start with a 1x1 or cleared target"; the interaction target 640x360 cleared | defaults: t54 black 1x1, t57 grey 1x1 = the neutral value (1.3) |
| D3D12-MASTER-PLAN S3 and check C7 | one copy, "after the opaque forward rows and before the water surface", not containing the water; C7 "1 with water or distortion" | two copies with water (C1 after `WatDpt`, C2 after the surface), one without; t45 contains the surface (1.4) |
| WATER-REFLECTION-INTERACTION-E08 1.1/1.2/1.3 | the m37 view draws "only 3 meshes" as if filtered; "the reflection of frame N uses the plane of frame N-1" and a 100-frame plane hold | the flag selects thousands of parts, three draws are INFERRED to be the visibility result; the plane is reset every frame and the 100-frame counter belongs to another gate (1.5) |
| TESS-SHADOW-FORWARD-NATIVE 3, WAVE1-PREP 2.4 | tessellated witnesses are the veil and hair characters | the five zones add a map witness: m50 `m001523`, G-buffer `GBuf_PN` (1.7); and "no patch topology is drawn" is true only for 3-control-point model patches: the FFX effects draw 1-control-point patches with hull/domain in every zone (1.7) |
| SHADOWS-WAVE1-READY SL14, LIVE-CHECKS-CONSOLIDATED 1.14 SL14 | "m31 (water)" | m37 (the m31 water is not drawn in the captured views); the sheets are corrected |

## 10. Review log

Independent reader: an Opus 5.5 subagent that re-derived the claims from the exports, the DXBC, the evidence TSVs and the retained captures (it edited nothing). 14 findings; the author re-checked the load-bearing ones against the sources before applying them.

| # | Severity | Finding | Disposition |
|---|---|---|---|
| 1 | blocking | With water the HDR is copied into the frame copy twice per frame (C1 after `WatDpt`, C2 after the surface and the depth downscale); the sheet had one. Re-checked on `i63-live-13/m37-s4/frame-60` (6 copy events 19030 -> 19031 for 3 frames; m31 has 1 per frame) and in the I-0053 listing | applied: 0.4, 1.4, 3.2 (steps 4-8), WT-2/WT-4, T-Q10, WL2, risk 5, correction row for the master plan and its check C7 |
| 2 | blocking | The "plane held 100 frames / frame N-1" reading is unsupported: the plane is reset every frame (`FUN_140023790:261-271`), reaches the reflection object only through a guarded copy (`FUN_140024740`, `FUN_1400885F0`), and the counter belongs to `view+0x893` (objects with byte 0x86), gating other code. Re-checked in the exports | applied: 0.5, 1.5 plane bullet, 3.3 WT-6, T-Q11, WL3, risk 1, correction row |
| 3 | important | The topology-33 events are 1-control-point patch draws with hull/domain bound (the FFX effects), not compute dispatches; only 3-control-point model patches (35) are absent. Re-checked (`hs`/`ds` ids in the events) | applied: 0.6, 1.7 observed, `capture-water-patch-scan.tsv` (new column), prior art `live_host.cpp:2260-2280` for TS-3/T-Q7 |
| 4 | important | The refraction alpha mask: `FwdSdw` samples t55 twice and keeps the distorted sample only where its alpha is 0 (disassembly 816-819), so `WatDpt` must write alpha 0 and the rest of the HDR must keep a non-zero alpha. Re-checked in the disassembly | applied: 1.2 notes, 3.2 step 3, WL7, risk 9 |
| 5 | important | The readable depth (`DepthRead`) does not exist natively | applied: section 2 row, 3.2 resources, WT-2, risk 10 |
| 6 | important | `EffectiveOrdinaryModelDeformation` only handles `disabled || LOD`; the velocity, `ctx+0x18C`, `ctx+0x18` rules are not in the native | applied: 1.7 selection rules, TS-2 |
| 7 | important | The collision `ReflectPlaneHeight` is not uniform per zone (m31 89 of 93, m37 59 of 63) | applied: 0.5, 1.1 table and reading, WT-6, WL11 |
| 8 | minor | Bit 6 does not select `WatDpt`; both rows are emitted for every kind-0x27 descriptor (`57A20:626-685`) | applied: 3.1 item 1 |
| 9 | minor | `K` at `1439F249C` is 0.0; "nearest water mesh" and the mask-to-flag mapping are inferred; the 0x1E rewrite concerns kind-2 descriptors | applied: 0.5, 1.5 |
| 10 | minor | The native already builds `SYSTEX_BLACK/GRAY`; the table root `144961AB0` and the options root `144961B50` are different objects | applied: 1.3 (GX1/GX2), 3.2 resources, T-Q3, WL5 |
| 11 | minor | Hull/domain read sets were uncited and too broad; `output_triangle_cw`; `DptA_PN` has no hull | applied: 1.7 read sets, WL14, TS-2 |
| 12 | minor | `cbPNTriangles` row 1 word 2 is the constant 1.0; words 10-11 are the stack cookie | applied: TS-4 |
| 13 | minor | Counts and anchors (m45 17 captures; gate histogram `GB:4940`; `cbObjMatrix` not a palette; `Fwd_[A]` missing; 960 against 912 bytes) | applied |
| 14 | minor | WATER-NATIVE's "IBL bound" was a correct binding observation; SL14 still said "m31 (water)" | applied: correction row; SL14 corrected in SHADOWS-WAVE1-READY and LIVE-CHECKS-CONSOLIDATED |

Also found by the author while applying the review: MODEL-DRAW-ENVELOPE-READY (W0a) already owns the hull/domain family and the flat fallback; TS-1/TS-3 now consume it instead of restating it.

## Exports and tools

`Get-ZoneWaterTess.ps1`, `Get-ZoneReflectParts.ps1`, `Get-WaterTessBlocks.ps1`, `Recover-NamedGameData.ps1` (this part); `Scan-ProgramReads.ps1`, `Read-Dxbc.ps1`, `Invoke-Map.ps1` (reused). Decompiled (read, in `decompiled/`): `FUN_140103510` (frame renderer: the t54..t57 and t45 binds, the copy, the plane update), `FUN_1400BFA70` (copy command), `FUN_1400FB0E0` (reflection driver), `FUN_1400FCE30` (view gather), `FUN_1400575C0` (kind 0x27 and plane candidate), `FUN_140057A20`, `FUN_14005A120` (rows, passes), `FUN_1402302C0` (`cbPNTriangles`), `FUN_140006C00` (default textures), `FUN_14022C180` (GX07). Constants read in the executable: `143D1BF90..` identity, `143D4EDC0` (-2,-2,-2,0), `143D7F4A0` (0,-1,0,0), `143D7EFAC` 64.0, `143D7ED10` 0.001, `143D7EE10` 1.0, `143D7EE00` 0.99.

## Implementation log (thread « Vague 1 producteurs W5 », branch `claude/friendly-feynman-70213e`, 2026-10-13: W5e)

**TS-1 (the parse and the flat fallback).** The C2 GX loop of the native material admission now parses GX04 and GX90
(`native/renderer/tess_constants`: `FromGx04`, `FromGx90`, the field map of MENUS-MAP 67) into the instance fields +0x250,
+0x254, +0x258, +0x25C, +0x260, +0x26C, +0x270, +0x274, +0x278, +0xDF and (GX90) +0xE4; row `native_tess_material` for each
material with a non-zero mode (bounded). The deformation the collect already derives from +0x250/+0x254 therefore becomes
3 or 4 for those materials; every ordinary pass now carries `DisableDeformation` unless `native-tess.flag` exists, so the
draw stays flat and gate 27 stays empty (W0a-6), and the flag is the release for W7. **TS-4 (CPU halves).**
`PnTriangles` (1402302C0) and `TessScene` (TESSSCENE-SSMASK 1.3). T-Q5/T-Q8/T-Q9 (`tess_constants_test`, run by the
build): the eight GX04 records of `water-tess-blocks.tsv` (m50 trees, the c5270 veil and cloths, c2230, c3140) parse and
give the golden cbPNTriangles words exactly (80 words), GX90 fields and its exparams scale, no block without the item,
cbTessScene on a synthetic camera (transpose, eye, -forward, (1, 0, 0, 0), four unit inward planes, clock). Not here:
`_PN` selection, the hull/domain PSOs and draws, the bindings (W7). Live: the zone witnesses (D-0174) include m50 for the
parse rows.

## Implementation log (thread « Source du soleil puis W6 », branch `claude/zen-blackwell-8eae69`, 2026-10-13: W7 TS-3)

**TS-3 (the ModelTess draw family).** `NativeModelProgramSet` carries the hull and domain programmes; `Stage()` knows hull (2)
and domain (3); a second root signature (`tess_root`) keeps the model root's parameters and adds the hull and domain
registers as root CBVs: **HS b0, b5, b6 and DS b0, b5, b6, b8** (64 of 64 root DWORDs). The fresh Opus review read the RDEF
of every `_PN` hull and domain of the cache: 100 hulls declare b5, b6 and 56 (`Dpt_PN`, the depth passes) b0, b5, b6; 100
domains b5, b8 and 100 (`Dpt_PN`, `DptA_PN`) b0, b8; so b0 was added. **Deviation from §3.4 and W0a-5:** no hull or domain
descriptor table (no SRV or sampler on those stages), so the `_D` domains (t5/s4, t12) are not reachable until TS-6. The
pipeline takes HS/DS and the PATCH topology type, the draw a 3-control-point patch list; the pipeline key carries the hull
and domain entries; triangle draws are unchanged (the review checked the model root byte for byte, the root indices, frame
roots and bounded CBVs). Tests: `model_draw_test` draws a tessellated control (pass-through hull with HS b6 factors, domain
with DS b5/b8) equal to the triangle draw, on the global and the legacy tables and under the shadow, on WARP and hardware
with D3D12 validation, and refuses a hull register outside the root, a hull SRV, a triangle family with hull programmes, a
domain register on a triangle draw and a missing DS b8. T-Q7 (`tess_pso_test`): **all 156 `_PN` hull/domain pairs of the
cache** (the sheet's 172 counted other files; `_FT` not covered) make a patch pipeline with their vertex and pixel
programmes on WARP and hardware (a permissive root for the creation) and declare no register outside the tess root (checked
on their RDEF). No producer selects the family yet: TS-2 (selection and recipes), TS-4b (the HS/DS bindings, DS b8 sharing
the view's `cbSceneParam` payload) and TS-5 (the draw behind `native-tess.flag`) follow; WL14 (`output_triangle_cw`
winding) is not witnessed.

## Implementation log (thread « Source du soleil puis W6 », 2026-10-13: W7 TS-2, TS-4b, TS-5 behind `native-tess.flag`)

**TS-2 (selection and recipes).** `SelectModelPrograms` resolves deformation 3 (`<family>_<pass>[_Skin][_Ins]_PN` vertex,
`<family>_<pass>_PN` hull and domain, the ordinary pixel; T-Q6). The backend has six more recipes (14-16 `GBuf_PN`, 17-19
`Fwd_PN`), built only with `native-tess.flag` for a material whose GX04/GX90 item asks for mode 2 without displacement, with
their hull and domain interfaces and the material's cbPNTriangles block (`PnTriangles` on the same parse as the C2
admission). The composed groups (map pieces whose group is not a loaded effect) now also take their `_GBuf_PN` hull and
domain from the engine's hull (4792B08) and domain (4792838) repositories, with the flag. The velocity pass kinds take
deformation 0 (`EvaluateOrdinaryModelPass`, 1.1). Gate 27 is gone: a deformed row takes its `_PN` recipe when it resolved,
the block exists, both stages' texture selections are ready and **the mesh's vertex elements feed every input of the `_PN`
vertex programme** (they read TEXCOORD1, an int4 the ordinary programmes do not: Native1584 failed a pipeline on a map piece
without it under a forced exercise, now a "layout" outcome); any other row is drawn flat by its pass's recipe. Row
`native_tess_rows` (outcomes pn, pass, recipe, block, textures, layout), `native_tess_candidate`, `native_tess_draw`.
**TS-4b.** The view's cbTessScene (hull/domain b5): `AbsoluteViewProjection` turns the camera-relative `VC_MatrixViewProj`
(cbSceneParam 384, applied by the ordinary VS to position - SC_CameraPos, read in `GXFlver_ColDifSpcBumpDOL_Gbuf.vpo`) into
the absolute-world one the domain applies to world positions (each stored row's w less its xyz dot the eye; T-Q9b in
`tess_constants_test`), eye SC_CameraPos, view vector -forward; domain b8 the view's cbSceneParam image (the domains read
VC_ClipPlane only, 960 bytes declared); hull b6 the material's cbPNTriangles. **TS-5.** A tessellated row is the ModelTess
family with the 3-control-point patch list; it is never a kept draw (its hull/domain blocks are per view) and never rebuilt
without its programme set. **Live** (Shebang saves, `native-tess.flag` via `Run-StartupTrace -EffectFlags native-tess`; the runs used successive builds
of this work): **no `_PN` draw has completed in game.** Native1580 (m50 Earthen Peak Ruins) and Native1581/1582 (m45), earlier
builds, ran with 0 debug errors and no refusal and drew nothing tessellated: the m50 trees (`m001523`, MSB (-77..-66, 50..70,
120..197)) are about 100 m above the bonfire camera (-23, -41, 88) and their material is not admitted there; at m45 one GX04
mode-2 material is admitted (factors 7/3, fade 0-20) and Native1582's `native_tess_candidate` witness saw no row of it
(INFERRED: its model is not drawn at that bonfire). A forced exercise (every material treated as the tree record, a temporary
test flag since removed): Native1584 drew one `GBuf_PN` row (recipe 14, `ColDifSpcBumpDOL`) whose pipeline creation failed
and ended the native renderer (refusal `native_render_worker_faulted`); the cause is INFERRED to be the missing TEXCOORD1
(T-Q7 builds the same pairs with a layout taken from the vertex programme), hence the "layout" fallback; Native1585 with it:
0 debug errors, 635,531 rows "layout", 777,589 "recipe" (character families without `_PN` programmes), 0 "pn". The final
binary: Native1587 (flag on, m50) 0 debug errors, no refusal, no tessellated material admitted; Native1586 (flag off, m37) 0
debug errors and the same pass-gate histogram as Native1579. **Risks before the flag becomes a default:** any `_PN` pipeline
failure still ends the native renderer (`NativeLayoutFeeds` checks semantics, not formats); a `_PN` row whose hull/domain
register cannot be bound (camera words unwritten) is dropped rather than drawn flat; a phantom forward row replaces a `_PN`
recipe by the flat `Phantom_Fwd` one after `native_tess_rows` counted it "pn". **Open:**
WL8/WL14 (a drawn `_PN` row: the m50 trees need a save near them, or `c6060` at m45 in view), the original's `ctx+0x18C` bit 4
and `ctx+0x18` rules (unidentified natively), `_FT`, `_D` (TS-6), and the depth pass `Dpt_PN` with the shadows (W9).

## Implementation log (thread « Sang sur persos, eau, siège », branch `claude/musing-bhabha-c546cc`, 2026-10-18: WT-1..WT-4)

- **WT-1** (on by default, `no-water.flag` keeps the former rows): a material with policy bit 4 or 5 (g_GXFT_PlanarReflection,
  g_GXFT_Refraction) gets the rows 0x24 (preparation 6, shader pass 12 `WatDpt`, recipe 22) and 0x27 (preparation 0, `Fwd`) after the
  0x1F rows, and no other row (shadow, velocity and 0x29 included); `SelectModelPrograms` and `EvaluateOrdinaryModelPass` take pass 12
  and the two engine passes (class mask 0xFFFF); the composed gxflvershader group collects `<group>_WatDpt` programmes.
- **The water block of cbMatDynParam** (Native2145: `FC_Water*` unwritten, the rows refused): the native GX parser now reads **GX07**
  (14022C180: data words 0..3 to sub+0xD0, 4..11 to +0xE8, 12..15 to +0x110, 16..17 to +0x120, version > 0x66 18..19 to +0x128, > 0x67
  the next two to +0xE0; sub+0x12 = 1) and the collect writes 22E930's water block (+0x110..+0x17F: scroll speeds, FogParam0/1 with the
  squared colour times its intensity, Fresnel = (d9 - d8, d8, d17, 1 / a) zeroed in its first two words while the reflection option `[[GX2+0x5360]+0x6C]` is off, FadeParam = (1 / b, 0, 0, 0) with b = d19 or a). This
  corrects 1.1's note: GX07 is the water block, not a phantom one. The forward lights are built for the water rows too.
- **WT-2..WT-4**: t54 SYSTEX_BLACK, t55 the frame copy, t56 the readable depth (`SceneRenderer::WaterDepthCopy`, R32G8X24, copied from the
  depth before WatDpt), t57 SYSTEX_GRAY; states of 1.2 (WatDpt alpha ZERO / ZERO without depth writes, the surface alpha ONE / ZERO with
  depth writes); with water rows the forward block runs the 0x1F rows, the depth copy, WatDpt, C1, the surface, C2, the other forward rows
  (plan-key bit 35); rows `native_water_draws`, `native_water_draw_incomplete`.
- **Witnesses** (zone saves made with Iota, Alpha untouched: `shebang-zones-iota-m33` at the Crucifixion Woods bonfire, SHA-256
  1908C700..., and `shebang-zones-iota-m33-farron` at the Farron Keep bonfire, D64FF8ED..., Reference2147): Firelink Native2146 (1 + 1
  water draws), Farron Keep Native2149 (12 + 12, the twelve Water_DL swamp pieces at y = -279), Crucifixion Woods lake (piece m000012,
  Water_3normaltet, teleport "-56 -261 -478"): **with ReflectionQuality = DISABLE in both modes the lake matches the original**
  (Native2161 / Reference2160); with the normal settings the original shows the planar reflection of the trees and sky and the native the
  lake floor (Native2159 / Reference2158). No debug error.
- **Open**: WT-6/WT-7 (the reflection view, the dominant difference), WT-5 (FwdSdw / FwdSSdw: every forward row natively still takes
  `Fwd`, a forward-shadow item of its own), the m33 Crucifixion Woods cross beside the bonfire missing natively (Native2157 / Reference2156,
  cause open), a small object in the lake missing natively (Native2161).
- **Departures from 3.2 / 1.4 recorded at the review** (2026-10-18): t56 is copied after the 0x1F rows (the original copies `[frame+0xFD0]`
  after the G-buffer); the readable depth is not re-copied after the surface (step 6); in a water frame the 0x1F rows see the previous
  frame's t45 (the original binds no copy for them); without water the frame copy still precedes the 0x1F rows (3.2 has it follow them).
  Pre-merge review (Opus): one blocker on the paint path fixed (the first-clear announcements were cleared again), a re-presented scene
  no longer paints twice, a stale receiver no longer blocks the queue, the paint-target cap is checked before the GPU objects, both b4
  stages are size-checked, the plan-key bit 12 covers water-only frames, the t56 record is in the record epochs. Firelink Native2164
  after the fixes: water drawn, no debug error.
