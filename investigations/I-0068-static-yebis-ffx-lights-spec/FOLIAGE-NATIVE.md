# I-0068 part AP: the High Wall foliage native does not draw (W2), the families behind it, the original path, and the native correction by family (STATIC; OBSERVED where marked)

Request (coordinator, 2026-10-11): at the witness point W2 of the High Wall (NPC-FAMILIES-NATIVE "W2": (24.3, −10.3, 96.1),
Native1210-1211 against Reference1212) the original draws orange tree foliage that native does not. Find statically why:
the material and shader families of the foliage (alpha test, wind, two-sided, instancing, LOD), the original submission
path, what native refuses or does not produce; propose the general native correction by family, with witnesses. Nothing was
launched; native code was read, not changed. Status words: **STATIC** (code, data, shader), **OBSERVED** (an existing native
or Reference log or image), **INFERRED**, **OPEN**.

## 0. Answer in eight lines

1. **What is missing at W2 is two populations.** The dense orange **litter and leaf masses** are the **Grass** family
   (`M[ARSN]_e_grass`, `M[ARSN]_l_e_grass`, `M[A]_e_grass`, `M[A]_l_e_grass`; the I-0063 inventory's "M04 Vegetation"):
   42 materials, 705,000 vertices in `m30_00`. The **tree canopies** are `m009002`, a trunk, branches and a `foliage` card
   mesh (§3, §9). The ground leaf cards that native does draw are the Edge family (`M[ARSN]_l_e`), which works (§6).
2. **The Grass family is refused by a bookkeeping rule, not by a missing input** (STATIC, §7). Its vertex shader indexes
   `cbvModelParam` with a register index (wind arrays), the native read analysis therefore treats **every** word of the
   1264-byte buffer as read, and native produces only part of it: `+0x70..+0x7F` (`VFC_PomLightDir`) and `+0x490..+0x4AF`
   (`VC_InstanceId`, its pad, an unused clip plane) have no producer at all, and `+0x80..+0x48F` (`VC_ForceParam`) is written
   only when the model's animation block flag `[anim+0x620]` is set. The Microsoft disassembler marks `VFC_PomLightDir`,
   `VC_InstanceId` and the clip plane **unused** in the shader (§7.2), so no live value is missing for them.
3. **Only the Grass families are affected:** of 6,685 vertex shaders in the retained shader banks, exactly **149** index
   `cb7` dynamically, all of them `GXFlver_Grass*` (9 families, every pass) and 5 `Eclipse_grass` (§7.3).
4. **First correction (one rule, STATIC, §8 F1):** produce the missing words as zero. A zero `VC_ForceParam` has both wind
   counts at 0, the two sway loops do not run, and the vertices stay in rest pose: the foliage appears static. This is the
   engine's own initial state of the block (the constructor clears it, §5 O3).
5. **Wind is a second, later step** (§8 F2): in the original the block is written every frame by the item update
   `FUN_140D3E950` from a node registered in the FX node manager `[1449623A0]` (`FUN_140245870`, flags 4), evaluated by
   `FUN_140245F90`. Native already publishes that manager (`PublishNativeFxWorld`) but runs no FFX scene; the area winds
   come from the zone's `WindSFX` regions (m30: 3 regions, FXR 800081). Until the effects phase, "no wind" is the right policy.
6. **The Edge family is not the problem** (§6): `M[ARSN]_l_e` is the G-buffer alpha-test class (4, pixel variant `_[A]`),
   two-sided, no wind, and native draws it (the leaf cards on the stairs).
7. **The tree canopy `m009002` is not explained by any refusal in the logs (OPEN, §9).** Native joined its geometry (storage
   generation 408: 3 meshes, 299,328 bytes), no logged gate names it, and its `foliage` mesh uses the common layout. §9
   lists the five candidates and the three reads that separate them.
8. **Earlier notes attributed "grass" to the E03 effects path** (COVERAGE-MATRIX row 5, NATIVE-GATES-AUDIT §3 row 5). That is
   true only for the instanced grass of the FFX grass system (`FUN_14029ADF0`, `_Ins@[cl]` shaders, m51). The map-piece grass
   of W2 is an ordinary `SprjFD4ModelItem` draw (§5) and needs only F1 (§11).

## 1. What the two modes show at W2 (OBSERVED)

Images: `evidence/I-0066/artifacts/i66-native-npcfam-1211/ingame-teleport.jpg` against
`i66-reference-npcfam-1212/ingame-teleport.jpg` (same stairs, door and statues). Reference: orange litter all over the
steps and the plaza, dense leaf clusters hanging on both sides of the door. Native: stone steps with a few orange leaf cards
near the camera, no clusters.

Camera poses of the native run (`native_pass_rows` of Native1211; `eye`, `axis_z`) and what the foliage population projects
to at each (`Project-Foliage.ps1`, a pinhole at 60° vertical, 16:9; **approximate**: no display groups, no occlusion, no
engine frustum; evidence `evidence/I-0068/queries/foliage/w2-proj-frame*.tsv`):

| Native frame | eye | axis | prepared world meshes (engine pass 5) | Grass meshes on screen (vertices) | Edge `_l_e` meshes (vertices) |
|---|---|---|---|---|---|
| 241 | (24.75, −11.18, 90.14) | (−0.083, 0.321, 0.943) | 1111 | 3 (24,863) | 125 (686,133) |
| 301 | (27.63, −9.50, 94.49) | (−0.900, −0.022, 0.436) | 816 | 9 (102,497) | 151 (884,363) |
| 361 | (27.36, −9.02, 98.18) | (−0.826, −0.050, −0.561) | 1467 | 22 + 3 `_l_e_grass` (≈ 271,500) | 116 (561,828) |
| 421 | (24.03, −9.35, 99.78) | (0.073, −0.082, −0.994) | 1166 | 14 + 3 (≈ 207,700) | 54 (217,166) |

At frame 241 the tree `m009002_2006` foliage (1,250 vertices) projects to x 14..329, y 36..300: the upper-left canopy
region of the reference image (the right-hand canopy is not identified: `m009002_2010` is outside that frame). The grass
pieces `m001706` and `m001761` project to the two bands y 378..539 across the stairs: the litter. The ground leaf cards that
native does draw are INFERRED to be the small `MARSN_e_02` Edge pieces `m001786` (96 vertices, 6 m) and `m001779` (10 m).

## 2. The data behind the numbers (STATIC, retained game data)

The map pieces of each zone were recovered read-only with `I-0045/Recover-GameData.ps1` (pattern
`^data\d/map[/\\]m30_00_00_00[/\\].*\.mapbnd\.dcx$`, files kept outside the repository) and scanned by `Scan-MapFlver.ps1`
(FLVER materials, MTD, vertex and index counts, cull flag, face-set count, vertex layout). The DLC zones (`m45`, `m50`,
`m51`) have **no entry in the pinned file dictionary**, so their pieces cannot be recovered by name; their M04 weights come
from the I-0063 inventory (`evidence/I-0063/inventory/zone-families.tsv`: 46, 7, 112).

| Zone | pieces | grass materials (pieces) | grass vertices | Edge materials | `_al` materials | grass part instances: `LodParamID` |
|---|---|---|---|---|---|---|
| `m30_00` | 605 | 42 (39) | 704,957 | 279 | 8 | 39: −1 ×33, 61 ×4, 23 ×2 |
| `m31_00` | 464 | 187 (96) | 3,086,567 | 62 | 38 | 351: −1 ×281, 25 ×58, 26 ×5, 61 ×4, 19 ×3 |
| `m37_00` | 583 | 29 (28) | 183,796 | 111 | 21 | 28: 50 ×28 |
| `m40_00` | 333 | 66 (47) | 1,025,999 | 88 | 3 | 216: 0 ×209, −1 ×3, 3 ×3, 30 ×1 |
| `m45_00`, `m50_00`, `m51_00` | not recoverable by name | I-0063 weights 46, 7, 112 | | | | |

The grass material counts equal the I-0063 M04 weights of the four zones (43, 188, 29, 66, one object more each), so
"M04 Vegetation" and the Grass family are the same set. Evidence: `evidence/I-0068/queries/foliage/` (`zone-foliage-summary.tsv`,
`m30/m31/m37/m40-materials.tsv`, `m30-meshes.tsv`).

## 3. The families (STATIC: MTD parameters, SPX, FLVER data, shader banks)

| Family | MTD (examples) | SPX family | Raster / test | Layout and faces | Where at W2 |
|---|---|---|---|---|---|
| **V1 Grass, normal-mapped** | `M[ARSN]_e_grass`, `M[ARSN]_l_e_grass` (DOL: lightmap) | `GXFlver_GrassSpcBump[DOL]` (Env variants exist) | `g_BlendMode` 0, `g_AlphaRef` 128 → class 4, pixel `_[A]`; `g_GXFT_WindEffect` True; `g_GX_bDisableDepthWrite` False | `Position0 Normal0 Tangent0 VertexColor1 UV0(UV) UV1(Byte4C)`; all 38 meshes two-sided (cull flag clear); 30 have 3 face sets (LOD), 8 one | the litter and masses: `m30_00_grass_003`, 37 meshes |
| **V2 Grass, albedo only** | `M[A]_e_grass`, `M[A]_l_e_grass` | `GXFlver_Grass[DOL]` | same | `Position0 Normal0 VertexColor1 UV0 UV1`; `M[A]_e_grass` is two-sided with 32-bit indices (`m004700`, 83,514 vertices, the only 32-bit grass mesh); the 4 `M[A]_l_e_grass` meshes (`m9xx` far pieces) are cull-back, one face set | 5 meshes |
| **V3 Edge** | `M[ARSN]_l_e`, `M[ARSN]_e` (objects) | `GXFlver_ColDifSpcBumpDOL`, `…Ibl` | blend 0, `g_AlphaRef` 128 → class 4, `_[A]`; no wind | common layout (`UV0` UVPair, no UV1); two-sided; 82 of 275 `_l_e` meshes have 3 face sets | ground leaf piles `MARSN_e_xx`, ivy, fences, the tree `foliage` |
| **V4 Alpha-blended** | `M[ARSN]_l_al`, `M[ARSN]_al`, `M[A]_al` | `GXFlver_ColDifSpcBumpDOL`, `ColDif` | blend 2, alpha ref 1, forward row `0x29` | common | 8 materials in the zone |
| **V5 Thin translucency** | `M[ARSN]_tr`, `_e_tr` | `GXFlver_ColDifSpcBumpTrIbl` | blend 0, alpha ref 128 | common | drawn natively (58 of 58 at Firelink, NPC-FAMILIES-NATIVE) |
| **V6 Instanced FFX grass** | none (spawned by the effects system) | `GXFlver_Grass*_GBuf_Ins@[cl]` | as V1/V2 | instance data + `tbForce` (2080 bytes) + `cbInstanceIdxParam` | m51 (34 draws a frame, I-0064) |

All Grass programmes exist for every pass (`Gbuf`, `Gbuf_[A]`, `Dpt`, `DptA`, `Fwd`, `Velo`, plain and `@[cl]`); `@[cl]` is the
variant without the clip-distance output (the plain one writes `SV_ClipDistance` from `VC_ClipPlane`); the original draws the
`@[cl]` form for ordinary scenery (I-0063 `drawn-families.tsv`).

## 4. The tree model at W2 (STATIC)

`m009002` (`m30_00_00_00_009002.mapbnd.dcx`): one FLVER, one bone, three meshes: `tree_002` (2,684 vertices) and
`tree_branches_002` (3,577) in `M[ARSN]_l` with the rare layout `… UV0(UVPair) UV1(Short2toFloat2)` (stride 36, 8 meshes in the
map, all trees), cull-back; `foliage` (1,250 vertices, 2,499 indices) in `M[ARSN]_l_e`, common layout (stride 32),
two-sided. Bounding box (−4.3, −0.5, −5.2)…(3.8, 16.8, 4.6) m. MSB: 6 instances within 33 m of W2 (`_2004`, `_2005`, `_2006`,
`_2013`, `_2014` with DispGroup1 `0x800` and DispGroup2 `0x2`; `_2010` with `0x400`), 14 more at 140-160 m; `LodParamID` −1,
`DrawGroups` all bits. The three meshes share one model, so a model-level cause would hide all three.

## 5. The original path of a Grass map piece (STATIC)

| # | Step | Where |
|---|---|---|
| O1 | MSB MapPiece → `SprjModelDispEntity` (vtable `29489A0`) with `SprjFD4ModelItem` children (vtable `2951FF8`, constructor `FUN_140D3C4D0`). Per frame: frustum on the entity box (`classify`), DrawGroups ∧ DispGroups (`64F000`, `619800`), category (`D043C0`, `D04350`), occlusion nodes, the LOD controller (`LodParamID`), then each child. Native does the same (`game_bootstrap.cpp:4036-4082`) except occlusion | MENUS-MAP §62; `AdmitNativeSelectedObject` |
| O2 | The constructor asks `FUN_1400619B0(model)`: does any material have policy bit 1? That bit is MTD kind 26 `g_GXFT_WindEffect` (the kind-to-bit table of `Classify-Mtds.ps1`: kind 22 → bit 17, 26 → bit 1, 37 → bit 19, matching `FUN_140061B00`/`1400619B0`/`140061A90`). If so: `FUN_140245870(…, 0x10, _DAT_142945250, 4)` → `FUN_14025B610([1449623A0], …)`: a 0x180-byte node is created in the FX node manager and its handle stored at `item+0x760` (−1 = none). Eclipse (bit 19) registers the same way with `0x2_0000_0000`. The constructor also clears the animation object at `item+0x70`, including its 0x410-byte block at `anim+0x210` and the flag `anim+0x620` (`FUN_1400613B0`) | `FUN_140D3C4D0` lines "FUN_1400619b0", "FUN_140245870" |
| O3 | Every frame the item update `FUN_140D3E950` (a): updates the node's transform (`FUN_1402458B0(item+0x760, matrix)`), (b) builds a local `GXSR` force-parameter object (vtable `142948EE0`, two arrays of 8 records of 0xD0 bytes, initialised by `FUN_140245700`), (c) evaluates the node into it (`FUN_140245F90(handle, local)`, 7.9 KB, driven by the node manager `[1449623A0]` and the node's record), (d) calls its slot 0 (`140D07E20`), which writes `{count A, count B, arrays}` to `item+0x280` (= `anim+0x210`), and (e) sets `item+0x690` (= `anim+0x620`) to 1. Without a handle the block stays zero and the flag 0 | `FUN_140D3E950`, `140D07E20` (machine code read) |
| O4 | The draw state builders `FUN_14005B3D0` (G-buffer) and `FUN_140059470` copy `anim+0x210..+0x61F` into the stack block that becomes `cbvModelParam +0x80` **when `anim+0x620` is set**; `+0x60` ← `group+0x50`; `+0x4B0` ← `group+0x60` (`VC_ModelSizeParam`); `+0x4C0..+0x4EF` ← the wind-parameter interface object `[anim+0x628]` when `anim+0x630` is set; `+0x70` (`VFC_PomLightDir`) ← the draw context `+0x1290`, copied by `FUN_14013B380` from the pass context `+0xA60`, which the pass driver `FUN_14013D810` (read; its siblings `14013CCE0` and `14013DD50` write the same field) fills from `[[144961AB0]+0x2A0]+0x2E0` (a 16-byte field of the GX device's parameter record; **`4961AB0` is absent natively**, LEGACY-ROOT-INVENTORY-903) | the two builders; INFERRED for the device record's meaning |
| O5 | Rows: engine pass 5 (mask 5: class 1 and 4; split into two rows 1 then 4 when `dword 14473CBAC == 2` and `model+0x220` bit 2), shader pass 6 `GBuf`/`Gbuf_[A]`; native `EvaluateOrdinaryModelPass` mirrors it (`model_pass.cpp`) | `FUN_140057A20` |
| O6 | The vertex shader (§7.2) displaces the vertex by the sum of the area winds and point winds listed in the block, weighted by the vertex's second colour (`UV1`, `v5`) | disassembly |
| O7 | The instanced variant `_Ins@[cl]` does not read `VC_ForceParam`: it reads `tbForce` and `cbInstanceIdxParam`, and is submitted by the FFX grass system (`FUN_14029ADF0`), outside the FD4 item path (I-0064) | RDEF; I-0064 RESULT |

Original-side values that live reads should confirm (INFERRED until read, §10): for a wind-registered item `item+0x760` ≠ −1
and `item+0x690` = 1 after the first update; the counts are integers 0..8; `VC_ModelSizeParam.y` > 0.

## 6. What native does with each family (STATIC code; OBSERVED logs)

| Family | Native today | Evidence |
|---|---|---|
| V3 Edge `_l_e` | **drawn** (class 4 → `_[A]`, rows 5, two-sided cull, 16 and 32-bit indices) | the leaf cards on the stairs (INFERRED to be `MARSN_e_02` pieces: `m001786` 96 vertices at 6 m, `m001779` at 10 m); no refusal event names the family |
| V1/V2 Grass | **refused** at `cbvModelParam` (§7) | Native1210/1211/1217 `native_selected_constant_unavailable` (`GXFlver_GrassSpcBump.spx`, `required: true`), `native_vmodel_param_unproduced`; Native1217 (Firelink) `native_selected_draw_preparation`: 593 prepared rows, 470 draws, 128 missing-constant refusals (the grass pieces, INFERRED) |
| V4 `_al` | forward blend 0 and 2 only; no depth write | NATIVE-GATES-AUDIT row 9 |
| V5 `_tr` | drawn | NPC-FAMILIES-NATIVE W6 |
| V6 instanced grass | absent (no FFX) | EFFECTS-E03 |
| Sky `GXFlver_Col` | `native_recipe_unresolved` (no `Gbuf` programme exists: the family is forward-only), gate 30, 5 meshes; not foliage, the known "native sky flat" | `native_pass_refusals` `"30":5` |

The other native refusal counts at W2 are not foliage: gate 18 (3,052: model class mask, the normal per-row filter), gate 21
(332: material class), gate 23 (61: capability masks, the character body-part masks of `native_capability_gate`).

## 7. Why the Grass family is refused (STATIC; the chain)

**7.1 The native producer.** `game_bootstrap.cpp:4839-4880` builds the vertex `cbvModelParam` (1264 bytes, `ds3prod::model_param_bytes`)
from: `+0x00` and `+0x30` the placement, `+0x60` ← `group+0x50`, `+0x4B0` ← `group+0x60`, `+0x80..+0x48F` ← `anim+0x210` **only
if** `[anim+0x620]`, `+0x4C0..+0x4EF` zero **only if** `[anim+0x630]` is clear. Its own comment lists `+0x70` (context `+0x1290`)
and `id_mod` (`+0x490`) as needing inputs.

**7.2 The shader.** `GXFlver_GrassSpcBump_GBuf[@[cl]].vpo` (disassembled with the system `d3dcompiler_47`, `Read-Dxbc.ps1`):

- `cbvModelParam` is declared `CB7[76], dynamicIndexed`. Variables marked **[unused]** by the compiler: `VC_World`,
  `VC_PrevWorld`, `VC_VtxCullingParam`, **`VFC_PomLightDir`** (`+0x70`), `VC_InstanceId`, its pad, `unused_VC_UserClipPlane`
  (`+0x490..+0x4AF`), the three world-position corners. **Used:** `VC_ForceParam` (`+0x80`, 1040 bytes) and
  `VC_ModelSizeParam` (`+0x4B0`, read as `cb7[75].y`: `1/sqrt(y)`).
- The layout of `VC_ForceParam` (RDEF `ForceParamST`), absolute offsets in the cbuffer:

| Offset | Member | Rows |
|---|---|---|
| 128 | `int4 WindForceLoopParam` (`x` = area-wind count, `y` = point-wind count, each clamped to 8) | 8 |
| 144, 272, 400, 528, 656, 784, 912, 1040 | `float4 WindForceAndTime[8]`, `PointWindPositionAndTime[8]`, `PointWindForce[8]`, `WindPositionAndRange[8]`, `WindWaveNoiseScale[8]`, `WindWaveNoiseScalePoint[8]`, `PointWindDirAndEnable[8]`, `WindAttractionParam[8]` | 9-16, 17-24, 25-32, 33-40, 41-48, 49-56, 57-64, 65-72 |

- Loop 1 runs `min(8, WindForceLoopParam.x)` times over rows `9+i`, `33+i`, `41+i`; loop 2 runs `min(8, y)` times over rows
  `17+i`, `25+i`, `49+i`, `57+i`, `65+i`. With both counts 0 nothing is added and the vertex keeps its rest position.

**7.3 The read analysis.** `AnalyzeConservativeListing` (`native/world/cb_reads.cpp`) marks any register-indexed read
(`cb7[r1.z + 9]`) as `dynamic[7]` with all four components, and `FloatMask` then expands it to **every word from row 0 to the
end of the declaration**; `PrepareReadShaderRegister` refuses the draw if a masked word has no producer. So for these shaders
native requires all 316 words, and `+0x70..+0x7F` and `+0x490..+0x4AF` are never produced. The logged "unproduced" list
starts at word 112 (`VFC_PomLightDir`) because words 0..111 are produced; it is cut at 700 characters (`vmodel_samples ≤ 6`,
`constant_samples ≤ 12`, the whole run), so the later words are not shown. In the first misses (High Wall start) the whole
`+0x80` block is listed too, i.e. `anim+0x620` was 0 at that time; whether it stays 0 is §10 FOL2.

**7.4 Scope.** A scan of all 6,685 retained `.vpo` found 149 with a dynamically indexed `CB7`: every non-instanced `GXFlver_Grass`,
`GrassSpc`, `GrassSpcBump`, `GrassDOL`, `GrassSpcDOL`, `GrassSpcBumpDOL`, `GrassEnvDOL`, `GrassSpcEnvDOL`, `GrassSpcBumpEnvDOL`
programme (`Dpt`, `DptA`, `Fwd`, `GBuf`, `Velo`; plain and `@[cl]`) and 5 `Eclipse_grass` programmes. No other family is
affected. The instanced `_Ins` forms index `CB4` and `CB5`, not `CB7`.

## 8. The native correction by family

**F1 (Grass V1/V2, G-buffer and every other pass: the whole refusal).** In the vertex `cbvModelParam` producer
(`game_bootstrap.cpp:4839-4880`) write, in this order, with `written` set:

1. `+0x70..+0x7F` zero (the shader never reads it);
2. `+0x80..+0x48F`: the engine block when `[anim+0x620]` is set (existing), **otherwise zero** (also when `model.animation` is
   null or a read fails): counts 0, rest pose;
3. `+0x490..+0x4AF` zero (`VC_InstanceId` = 0 for a non-instanced draw, the old host's `id_mod`; unused);
4. `+0x4C0..+0x4EF` as today (zero when `[anim+0x630]` is clear), and zero as well when `model.animation` is null or unreadable
   (today those words are left unproduced in that case too); when `[anim+0x630]` is set, the interface object `[anim+0x628]`
   must supply them: **read `[anim+0x630]` for a grass item**, expected 0.

An equivalent, more general rule: for slot 7 take the required words from the RDEF "used" flags (`D3D_SVF_USED`) instead of the
conservative dynamic mask. Expected after F1: no `native_vmodel_param_unproduced`; the Grass meshes appear in `native_pass_rows`
and `content_draws`; the image shows the litter and masses, static. Witness (FOL3): W2 poses of §1, +3 / +9 / +25 / +17 grass
meshes (± display groups) over the prepared counts 1111 / 816 / 1467 / 1166.

**F2 (real wind, effects phase; registry chain and commit order in [FFX-WAVE1-READY](../I-0066-independent-native-startup/FFX-WAVE1-READY.md) §1.2, §2.6, FW-3 and FW-6).** Make the original item chain run: `item+0x760` ≠ −1 (registration works natively, the node
manager is published), `FUN_140D3E950` runs for the item (read its outcome), `FUN_140245F90` finds a wind. The area winds come
from the zone's WindSFX regions (`Get-ZoneEffectSources.ps1`: m30 3 regions FXR 800081; m31 19 (800080-800082); m37 1 (800081);
m40 6 (800081); m45 8 (845010); m50 6 (850801, 850802); m51 3 (850800-850802)), i.e. from the effects entry door
(EFFECTS-ENTRY-DOOR-NATIVE), so F2 follows steps 1 and 2 of EFFECTS-IMPLEMENTATION-CHECKLIST. Check: the counts
`[anim+0x210]`/`+0x214` read 1..8 in a windy zone and the grass sways at the original's frequency (the shader's sine uses
`cb7[r+41]` and the vertex colour).

**F3 (alpha test, V1-V3).** Nothing to change: class 4, `_[A]`, `g_AlphaRef` 128; verify the `FC_AlphaRef` word of `cbMtdParam`
(`ProduceMtdParams`) reads 128/255 for a grass mesh (the Edge leaf cards already pass).

**F4 (two-sided).** Nothing: the face-set class mapping of Native1184 keeps `CullBackfaces` False as cull none; the `Gbuf_[A]`
pixel shader takes `SV_IsFrontFace`.

**F5 (index width).** Nothing: R32 slices are supported (`m004700`, 83,514 vertices, the only 32-bit grass mesh).

**F6 (LOD and fade).** Face-set LODs: native joins LOD0 only (`native_lod0_r16_r32`), 29 of 37 grass meshes have 3 face sets;
the lower LODs are a performance choice, not a visibility one. The distance fade is the CPU controller of §5 O1; 33 of 39 grass
part instances at W2 have no `LodParamID`.

**F7 (other passes).** Shadow (`Dpt`, `DptA`), forward (`Fwd`) and velocity (`Velo`) programmes of the same families read the
same block: F1 serves them when those passes exist (SHADOWS-NATIVE, VELOCITY-NATIVE).

**F8 (instanced FFX grass, V6).** Effects phase E03 (EFFECTS-E03-MODEL-PARTICLES-NATIVE): `FUN_14029ADF0`, `tbForce`,
`cbInstanceIdxParam`; not needed for W2.

## 9. The tree canopy `m009002` (OPEN)

**Known.** (a) The model's geometry is joined natively: `native_model_cpu_geometry_joined`, storage generation 408, `meshes` 3,
`source_bytes` 299,328 (the FLVER's vertex and index bytes are 299,278, plus 50 of alignment). (b) No logged gate names it; the
per-frame gate counts (§6) are class and capability filters, the 5 unavailable preparations are the sky. (c) Its `foliage`
mesh has the common layout and the Edge family, which native draws. (d) Both sides of the door are missing: the two trees at
the left (`_2004`..`_2014`) and `_2010` at the right. (e) Map-piece admission outcomes are **not logged**: the witness
`native_character_candidate` prints only names starting with `c` or `o` (`game_bootstrap.cpp:3339-3350`), and the refusal
samples are capped (§7.3), so a map-piece failure leaves no trace.

**Candidates**, from the code of `AdmitNativeSelectedObject` (`game_bootstrap.cpp:4020-4090`) and the original item functions:

| # | Candidate | Why it could hide a whole model | Separating read |
|---|---|---|---|
| T1 | `item+0x71A` (geometry suppressed): cleared in the constructor, and passed as an out-parameter by `FUN_140D3E240` to `FUN_14001D650`, which tests the item's bounds against the volume list at `[144961B50]+0x46E8` (what can set it is not read) | native reads it as `geometry_suppressed` and `SelectModelFade` then drops engine passes 5-8 | `[item+0x71A]` of the six near instances, both modes |
| T2 | the entity gates: `[entity+0x388]` bit 0, the DrawGroups ∧ DispGroups live test, the category test | silent `return` before any witness | the three results for `m009002_2006` |
| T3 | `zero_fade`: the embedded LOD controller (`E+0xF0`, weights `+0x16C/+0x170`) | `count == 0` returns silently; 592 of 853 logged `c`/`o` candidates are `zero_fade` (an unknown share is correct) | `E+0x170` and the controller row (`LodParamID` −1 expected: weight 1) |
| T4 | model not resident yet when the picture was taken (Firelink map pieces appeared "a few seconds after the teleport") | missing storage or `missing_inputs` for a while | the admission outcome per frame after the teleport |
| T5 | the trunk and branch layout `UV1: Short2toFloat2` rejected by `NativeVertexElementBytes` / `supported_layout` | drops two of the three meshes only (`unsupported_layouts`); does **not** explain the missing `foliage` | `unsupported_layouts` of the prepared frame |

T1-T3 are model-level or item-level, T5 is mesh-level and cannot be the whole story, T4 is a timing effect that the 1211 images
(taken several seconds apart) do not support. **Ranking: T2 and T1 first.**

## 10. The minimal live reads (added to LIVE-CHECKS-CONSOLIDATED §1.5)

All at W2 with the witness teleport on Argo's save, both modes unless stated:

| ID | Kind | Read | Expected |
|---|---|---|---|
| FOL1 | NOW (native) | replace the 12-sample caps by a per-frame histogram keyed by (family, resource, first unproduced word) for `native_selected_constant_unavailable` and `native_vmodel_param_unproduced` | grass families only; words 112..127 and 1168..1199 always; 128..1167 only while `anim+0x620` is 0 |
| FOL2 | NOW | for the first three grass items in view: `[item+0x760]`, `[item+0x690]`, the two ints at `[item+0x280]`, `[item+0x70+0x630]` | Original: handle ≠ −1, flag 1, counts 0..8, `anim+0x630` 0 (INFERRED); native: record |
| FOL3 | ACC (F1) | `native_pass_rows` world rows and `content_draws` at the four poses of §1 | +3, +9, +25, +17 grass meshes (± display groups), no grass refusal; image: litter and masses present, static |
| FOL4 | NOW | for `m009002_2004/_2006/_2010`: `[entity+0x388]`, the live-test and `classify` results, `[item+0x71A]`, `E+0x170`; and the admission outcome of **map-piece** items (extend the witness of `game_bootstrap.cpp:3339` to names starting with `m`) | Original: drawn; native: the first failing gate of §9 |
| FOL5 | NOW (Reference) | the 57A20 rows for `m009002`, `m001761`, `m001706` | rows present (classes 1 and 4) |

Poses: frame 241 (24.75, −11.18, 90.14) axis (−0.083, 0.321, 0.943); 301 (27.63, −9.50, 94.49) (−0.900, −0.022, 0.436);
361 (27.36, −9.02, 98.18) (−0.826, −0.050, −0.561); 421 (24.03, −9.35, 99.78) (0.073, −0.082, −0.994).

## 11. Corrections to earlier notes

- COVERAGE-MATRIX row 5 and NATIVE-GATES-AUDIT §3 row 5: the map-piece grass is **not** an E03 path; it is refused by the
  conservative read analysis of `cbvModelParam` and is fixed by F1. Only the instanced grass (V6, m51) is E03.
- NATIVE-GATES-AUDIT row 5 says the native "fills the wind block only when the model animation has `+0x620`, otherwise the draw
  stops": the block is not the only gap; `+0x70` and `+0x490..+0x4AF` are never produced (§7.3), so the draw stops even with
  the flag set.
- NPC-FAMILIES-NATIVE W2 ("tree foliage … a map family"): it is two families (§0); the Edge family is drawn.

## Artifacts

`Read-Dxbc.ps1` (shader interface, disassembly, constant-buffer reads), `Scan-MapFlver.ps1` (zone material and layout scan),
`Project-Foliage.ps1` (foliage on screen for a camera pose); external evidence `evidence/I-0068/queries/foliage/`
(zone summaries, material and mesh tables, the four W2 projections, the grass vertex-shader interfaces and `cb7` reads).
Recovery command (read-only on the game): `pwsh -NoProfile -File investigations/I-0045-attribution-upstream-producer-inputs/Recover-GameData.ps1
-Pattern '^data\d/map[/\\]m30_00_00_00[/\\].*\.mapbnd\.dcx$' -OutDir <outside the repository> -ReportPath <outside>`.
