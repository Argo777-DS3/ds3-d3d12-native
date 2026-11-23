# I-0068 part AT: the tessellation of the veil and the shadows on forward rows: the original mechanism, what native lacks, and the correction by family (STATIC)

Request (coordinator, 2026-10-11): the two other fidelity gaps of NPC-W347 §3: the **tessellation of the veil** (`Veill`, drawn flat natively) and the **shadows on forward
rows**. For each: the original mechanism, what native lacks, a general correction by family. Builds on MENUS-MAP §72–73 (tessellated families, `cbPNTriangles`,
`cbTessScene`), SHADOWS-NATIVE (the shadow system) and WATER-NATIVE (preparation → shader pass). Nothing was launched; native code was read, not changed.

Labels: STATIC (decompiled code, DXBC of the cached programmes, FLVER GX records, native source at `b5b20a63`), OBSERVED (I-0063 captures, cited), INFERRED, OPEN.
Evidence (outside Git): `evidence/I-0068/queries/shadows/` (`fwd-sdw-hash-compare.tsv`), `evidence/I-0068/queries/npc-families/all-chr-gx.tsv` (reused).

## 0. Answer first

1. **Tessellation is a per-material PN-triangle pass, not a different mesh.** A material whose FLVER carries a `GX04` (or `GX90`) record with a mode gets variant programmes
   `<family>_<pass>[_Skin]_PN` (mode 2, `_FT` for mode 1; `_D` with displacement): the same vertex shader up to the skinned position, then a **hull** (`partitioning_fractional_odd`,
   `domain_tri`, 3 control points in and out, factors capped at 64) and a **domain** shader that evaluates the cubic PN patch, then the same pixel shader. The hull does
   frustum culling of the patch, back-face culling, and silhouette- and distance-adaptive factors from `cbPNTriangles` (the material) and `cbTessScene` (the view). The vertex count
   changes, the surface is smoothed along silhouettes; the pixel shader is unchanged (§1).
2. **Native draws the veil flat because it never reads `GX04`.** `mat+0x250/+0x254` (mode, displacement) are written only by the GX item parser, which native does not run for
   `GX04` (it parses GX03/05/09 since C2), so the deformation is always 0 and `SelectModelPrograms` takes the plain programme. Beyond the parser, the native model PSO has **no hull or
   domain stage and only triangle-list topology**, so `_PN` needs renderer work, not only data (§2).
3. **Correction by family** (§3): the tessellated characters of the whole game are five, in two classes: the veil/hair `_PN` (c5270 `Veill_NoLight[_cloth]`, c2230 `Veill[_cloth]`, c6060 hair cloth) and the
   `FlowDisplacement` `_FT_D` (c3140, c3141). Fallback policy: keep the flat variant until the HS/DS PSO exists; it is exact in the interior and differs only on silhouettes.
4. **Forward shadows are chosen by a view state, not by the family.** The forward pass is `Fwd` (0), `FwdSdw` (4) or `FwdSSdw` (5): `shader pass = 0` when `view+0x710 == 0`, else
   `4 + (view+0x710 == 3)`. The state is set by the shadow layers' pass commands: **1 = cascade atlas receive (PCF in the pixel shader), 3 = screen-space mask receive, 0 = none**. The world
   views do use the shadowed variants (OBSERVED: the hair and the water of I-0063 are drawn with `FwdSdw`/`FwdSSdw`); SHADOWS-NATIVE's "world forward has no shadow input" is wrong (§4).
5. **Which forward families are affected** (§4.2, all 664 forward pixel programmes compared): 620 differ between `Fwd` and the shadowed variants (310 families), 44 are byte-identical (the 19 unlit
   families, **including `Veill_NoLight[_cloth]`: the veil of `c5270` receives no shadow in any variant**).
6. **Native has neither the state nor the inputs**: no shadow system at all, the pass selector refuses passes other than 0/1/6/7, and the forward PS `cbSceneParam` holds camera words only; the
   shadowed programmes read the shadow words (rows 28, 29, 54) and **index the cascade matrices dynamically (`cb8[r + 39…41]`)**, so the native read analysis would refuse them even with the words written
   (§4.3). Until the shadow system exists, `Fwd` is the right interim policy (it is the original's own `view+0x710 == 0` result).

## 1. Tessellation: the original mechanism (STATIC)

### 1.1 Selection

- **Material state**: `GX04` (`+0x254 = d[3] & 1` displacement enable, `+0x250 = d[4]` mode) or `GX90` (mode 2, enable 1) of the FLVER material (MENUS-MAP §67); the MTD's `g_TessellationMode` does not decide
  (MENUS-MAP §72: `c2170`'s veil has MTD mode 2 but no GX04 mode and is not tessellated).
- **Variant**: mode 2 → `_PN` (`_PN_D` with displacement), mode 1 → `_FT` (`_FT_D`); forced to 0 for pass kinds 1 and 0xB (velocity), when `ctx+0x18C` bit 4 is set, when `ctx+0x18 ≠ 0`, and for LOD > 0
  (`EffectiveOrdinaryModelDeformation`, `native/content/model_pass.cpp:35`).
- **Programme sets** (cache, `Veill_NoLight`): per pass `Gbuf`, `Fwd`, `Dpt`, `DptA`: `<pass>_PN.vpo`, `<pass>_PN.hpo`, `<pass>_PN.dpo` (+ `_Skin_PN.vpo`, + `_D`), pixel unchanged; **`Velo` has no tessellated variant**.
- **Stages bound** by the material apply (`FUN_140142430` → VS, PS, HS `FUN_1419208A0`, DS `FUN_141920A50`, GS); the topology is a 3-control-point patch list (GX 0xC → D3D `0x23`, `FUN_141929BA0`; a bound hull
  requires a patch topology in D3D11).

### 1.2 The stages (reflection and disassembly of `Veill_NoLight_Fwd_PN`)

| Stage | Inputs | Constants | Behaviour |
|---|---|---|---|
| VS `…_Skin_PN` | the ordinary skinned inputs (no `cbSceneParam`) | `cbInstanceData` b4, `cbObjMatrix` b5, `cbTexScroll` b11 (+ `cbMatDynParam` b2, `cbvModelParam` b7 for cloth) | outputs the skinned position **before projection** |
| HS | 3 control points (POSITION, NORMAL, TANGENT, TEXCOORD0–3) | **`cbTessScene` b5**: `g_f4Eye` @64, `g_f4ViewVector` @80, `g_f4GUIParams2` @96, `g_f4ViewFrustumPlanes[4]` @112; **`cbPNTriangles` b6**: `g_f4TessFactors`, `g_f4GUIParams1`, `g_fDetailTessellationHeightParam` | `hs_5_0`, `domain_tri`, `fractional_odd`, `output_triangle_cw`, max factor 64; culls a patch whose three vertices are outside one frustum plane (margin `cb6[2].x`) or all back-facing (`dot(N, ViewVector) < −cb6[1].x`); factors blend `cb6[0].x`/`.y` by silhouette and fade with distance (`cb6[0].zw` start/range × `cb5[6].x`); emits the PN control data (cubic edge and centre positions, normals) as patch constants |
| DS | patch constants | **`cbTessScene.g_f4x4ViewProjection`** b5, `cbSceneParam` b8 (`VC_ClipPlane`); `_D`: `g_DisplacementTexture` t5, s4 | cubic PN evaluation, projection |

### 1.3 The data: the veil of `c5270` (GX04 words of the FLVER, `all-chr-gx.tsv`)

| Material | mode | displacement | edge / inside factor | distance fade end |
|---|---|---|---|---|
| `veil` | 2 | 0 | 7 / 3 | 20 |
| `veil_cloth` | 2 | 0 | 7 / 3 | 20 |
| `veil2_cloth` | 2 | 0 | 3 / 3 | 20 |
| `Waist_cloth`, `Shoulder_cloth` | 2 | 0 | 5 / 1 | 5 |

(The GX04 words of `Dump-FlverGx.ps1` start 3 words after the section-67 numbering: the mode word is the second value of the printed record, the displacement flag the first, the factors the third and fourth, the fade end the seventh. INFERRED alignment, confirmed by the non-tessellated rows (mode 0, factors 5 / 1). Row 1 of `cbPNTriangles` = `{1 − mat+0x260, min(mat+0x264, 0.99), 1, mat+0x268}`.)
Tessellated characters in the game: `c5270` (5 veil/cloth materials + `Line_cloth`), `c2230` (`Veill`, `Veill_cloth`), `c6060` (`Hair_cloth`) → `_PN`; `c3140`, `c3141` (`FlowDisplacement`) → `_FT_D`; every other
material of the 165 FLVERs has mode 0.

### 1.4 Producers of the constants (MENUS-MAP §73, restated)

- **`cbPNTriangles`**: `FUN_1402302C0(sub = mat+0xD0, ctx, exparams)`, called by the material apply `FUN_140140850` when `mat+0xDF` and `mat+0x250` are non-zero; rows as MENUS-MAP §73 §1; uploaded to
  `[[144961B50]+0x4DD0]+0x2C8`, bound HS b6 / DS b6.
- **`cbTessScene`**: the 0xC0-byte block at the view's `+0x1130`, copied by `FUN_14013AA60` to `[[144961B50]+0x4DD0]+0x2D0`; layout (from the reflection) `{ViewProjection, Eye, ViewVector, GUIParams2,
  FrustumPlanes[4], GlobalTime}` = 0xB4 bytes. Its writer was not found by direct stores (MENUS-MAP §73); the content is the camera (view-projection, eye position, view axis, the four side planes
  of the frustum, time). INFERRED from the field names and the HS use.

## 2. Tessellation: what native lacks

| Item | Original | Native today | Gap |
|---|---|---|---|
| `GX04`/`GX90` parse | `141070` writes `mat+0x250/0x254/0x258…` | not parsed (C2 parses GX03/05/09 only) → `base_deformation` = 0 (`game_bootstrap.cpp:3262`) | add GX04/GX90 to `BuildModelMaterial` (as C2) |
| programme selection | `_PN/_FT[_D]` VS + HS + DS | `SelectModelPrograms` refuses `deformation ≠ 0` (`model_programs.cpp`) and has no hull/domain lookup | suffix, `.hpo`/`.dpo` lookup (the shader bank already types the stages, `shader_bank.cpp:74`) |
| PSO | VS, HS, DS, PS; patch topology | VS + PS only, `PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE`, triangle list (`model_draw.cpp:272,310`) | HS/DS in the PSO, `TYPE_PATCH`, `IASetPrimitiveTopology(3_CONTROL_POINT_PATCHLIST)`, root signature with HS/DS CBVs b5/b6 (+ b8 on DS, t5/s4 for `_D`) |
| `cbPNTriangles` | `1402302C0` | none | transcribe (rows in MENUS-MAP §73) |
| `cbTessScene` | per-view block | none | camera-derived block: content closed in part AW (TESSSCENE-SSMASK-NATIVE §1.3) |
| depth/shadow prepass `Dpt_PN` | tessellated depth | n/a (no depth prepass of models natively) | follows the shadow system |
| LOD and pass rules | deformation 0 for LOD > 0, velocity, `ctx+0x18C` | rule present (`EffectiveOrdinaryModelDeformation`) | none |

The veil's pixel inputs are already produced (NPC-W347 R3, R6); only the geometry path differs.

## 3. Tessellation: correction by family and witnesses

| Family | Materials | Needs | Interim | Witness |
|---|---|---|---|---|
| **Veil `_PN`** | `Veill_NoLight[_cloth]` (c5270), `Veill[_cloth]` (c2230), `Veill_Curtain` | GX04 parse, `_PN` selection, HS/DS PSO, `cbPNTriangles`, `cbTessScene` | flat `Fwd` (silhouette not rounded) | `c5270` at W4: the veil silhouette smooth; draw count of the veil meshes unchanged, patch count 3 control points each |
| Hair/cloth `_PN` | `Hair_cloth` (c6060, default `_Bloody_Cloth` family) | same | flat | c6060 at `m45_00` |
| **`_FT_D`** flow displacement | `FlowDisplacement` (c3140, c3141) | same plus `g_DisplacementTexture` t5/s4, flow texture t17, time | flat, no displacement | c3141 at `m30_01` (MENUS-MAP §69) |

Order: (1) parse GX04/GX90; (2) the renderer HS/DS PSO and patch topology with a unit test on the veil programme set; (3) `cbPNTriangles`; (4) `cbTessScene` (verify against one frame's block, live row TS1); (5) select `_PN/_FT`.

## 4. Shadows on forward rows

### 4.1 The mechanism (STATIC)

- **Selection** (`5A120`, the preparation switch, WATER-NATIVE §2.3): preparation 0 → shader pass `0` (`Fwd`) when `view+0x710 == 0`, otherwise `4 + (view+0x710 == 3)`: **4 `FwdSdw`**, **5 `FwdSSdw`**.
- **The state `view+0x710`** (dword of the view/pass context) has two writers, both pass-command callbacks of the shadow layers (the only code that stores it; searched over the whole database):
  - `FUN_1401DD490` (cascaded shadow scene): command 2 → `1`; command 3 → `0`;
  - `FUN_1401B8F30` (screen-space shadow layer): command 2 → `3` and binds the shadow-mask texture as the view's t32 source (`view+0x12A0`); command 3 → `1` if it was 3.
  So forward rows prepared between the begin and end commands take `FwdSdw` (1) or `FwdSSdw` (3), others `Fwd`.
- **Receivers** (the pixel programmes, `GXFlver_ColDifSpcBumpIbl_Bloody_Fwd*`):
  - `FwdSdw`: `t32 = g_ShadowTexture` (the cascade atlas), comparison sampler `s12`; cascade chosen against `FC_CascadeSelectDist` (`cbSceneParam` row 54), 4-tap PCF through the cascade matrices
    (`cb8[c + 39…41]`, dynamically indexed), fade by distance with `FC_ShadowMapParam` (row 28: far start, scale), tint `FC_ShadowColor` (row 29): `colour *= 1 − tint × fade × (1 − pcf)`.
  - `FwdSSdw`: `t32 = ` the screen-space shadow mask (sampled with `s8` at the screen position), `FC_ShadowColor` only.
  - `Fwd`: none of these is read (the shadow variables are flagged unused by the compiler).
- **Observed in the original**: I-0063 world captures draw `SAT_HairIbl2_*_FwdSdw_[A]` (14 draws), `SAT_HairIbl2_Fwd_[A]` (4), `GXFlver_WaterBumpTri_FwdSdw`, and, in m51, `SAT_HairIbl2_*_FwdSSdw` (LIVE-RUNS lines 275, 391).

### 4.2 Which forward families receive shadows (`fwd-sdw-hash-compare.tsv`, SHA-256 of the cached programmes)

664 forward pixel programmes (plain and `[A]`): 620 differ from `Fwd` in both shadowed variants (310 families), 44 are identical to `Fwd` (19 unlit families: `GXFlver_Col`, `GXFlver_ColSh`,
`GXFlver_Emi[Glow]`, `GXFlver_Sword`, `MultiBlend2_et1_nolight*`, `MultiBlend3/4_et1_nolight*`, `Eclipse_cloth`, `Fujir_Nolight_OpacityBlend`, **`Veill_NoLight`, `Veill_NoLight_cloth`**).
The lit forward families (`Veill`, `Veill_Curtain`, ghosts, cinder, Fur, crystal, hair, water, `Phantom` variants, `SAT_Equip`) receive shadows.

### 4.3 What native lacks

| Item | Original | Native today | Gap |
|---|---|---|---|
| the shadow system (atlas, cascades, mask) | SHADOWS-NATIVE | none | steps 1–8 of SHADOWS-NATIVE §2 |
| the view state | `view+0x710` from the layers' commands | none: forward is always pass 0 (`model_pass.cpp:55`, `SelectModelPrograms` accepts passes 0, 1, 6, 7) | a native per-view shadow-receive mode (0/1/3) set where the native shadow passes begin and end |
| pass naming | `FwdSdw`, `FwdSSdw` | not in `SelectModelPrograms` | add passes 4 and 5 |
| inputs | t32, s12, rows 28/29/54 and the cascade matrices of `cbSceneParam` | forward PS `cbSceneParam` = camera words + four zeroed words (`game_bootstrap.cpp:4859…`); no t32 for forward; s12 absent | produce the rows; bind t32/s12 |
| read analysis | n/a | the dynamic `cb8[r + 39…41]` read makes the native analysis treat **the whole of `cbSceneParam` as read** (as for the Grass `cbvModelParam`, FOLIAGE-NATIVE §7), so any unproduced word refuses the draw | produce the full buffer (all 960 bytes) or extent-bound the read |
| result | shadowed hair, water, lit forward bodies | all forward rows unshadowed | none until the system exists |

### 4.4 Correction by family and witnesses

| Family class | Examples | Native policy now | After the shadow system |
|---|---|---|---|
| unlit forward (19) | `Veill_NoLight[_cloth]` (W4), `GXFlver_Emi`, `MultiBlend*_nolight` | `Fwd` is exact | unchanged |
| lit forward, cascade receive | hair `SAT_HairIbl2` (FwdSdw), water, ghosts, cinder, Fur, crystal, `Veill[_cloth]`, phantoms | `Fwd` (unshadowed, brighter in shade) | mode 1 → `FwdSdw`: t32 atlas + `s12` + rows 28/29/54 + cascade matrices |
| lit forward, screen-space receive | hair in m51 (`FwdSSdw`) | `Fwd` | mode 3 → `FwdSSdw`: t32 mask + `FC_ShadowColor` |
| deferred rows | all G-buffer models | shadows through the lighting composite (t32 mask) | unchanged |

Witnesses: the hair of the player at the High Wall (forward alpha-tested): original `FwdSdw_[A]` against native `Fwd_[A]`; a ghost in shade (`c1480` at `m37_00`); the water at m37 (the m31 water is not in the captured views, WATER-TESS-WAVE1-READY 4). Read the mode: live row TS2.

## 5. Live rows (none taken)

| ID | Read | Expected |
|---|---|---|
| TS1 | the `cbTessScene` block (`[[144961B50]+0x4DD0]+0x2D0`, 0xB4 bytes) and `cbPNTriangles` on a `c5270` veil draw | camera view-projection, eye, view axis, side planes; factors 7 / 3 and fade 20 for `veil` |
| TS2 | `view+0x710` while the world view prepares forward rows (hair, water) | 1 or 3 |
| TS3 | the programme names of the `c5270` veil draws | `Veill_NoLight_Fwd_Skin_PN` / `_cloth_Fwd_PN` with HS/DS |
| TS4 | the topology of a veil draw | 3_CONTROL_POINT_PATCHLIST |

## 6. Corrections to earlier notes

- SHADOWS-NATIVE §1.5 table ("forward `_Fwd` (world): no shadow input"): the world's forward rows take `FwdSdw`/`FwdSSdw` when the view state says so (OBSERVED, I-0063).
- NPC-FAMILIES-STATIC §2.3 / NPC-W347 §3: the veil's `Fwd` shadow variants are byte-identical to `Fwd`, so the "forward shadows" gap does not apply to the veil.

## Exports and evidence

- Evidence (outside Git): `evidence/I-0068/queries/shadows/fwd-sdw-hash-compare.tsv`.
- Decompiled (private database): `FUN_14005A120`, `FUN_1401B8F30`, `FUN_1401DD490`, `FUN_1402302C0`, `FUN_140142430`, `FUN_141929BA0`.
