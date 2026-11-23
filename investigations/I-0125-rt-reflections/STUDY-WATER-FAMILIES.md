# I-0125 study: the game's water programme families, the reflection plane, the sky pieces

Static study, read-only, made for the R pilot by the study thread's Sonnet subagent (2026-11-06, Paris); non-canonical until the
pilot re-reads what a change rests on. Re-checked by the study thread: the t54 and t55 declarations of all 30 water `_Fwd*`
programmes in `program-resources.tsv`, `GXOceanWave` the only other t54 reader, and `WaterBumpTri_Fwd`'s reflection UV and
Fresnel (cb2[20].z = FC_WaterFogParam0.z, cb2[22].xy = FC_WaterFresnel, `v = 1 − SV_Position.y × FC_ScreenSize.w`). Labels: **OBSERVED** (read in a primary source: bytecode disassembly, data file, decompile; source cited),
**INFERRED** (reasoned from observations, the basis said), **TO CHECK** (not established). Nothing was launched, built or edited.

Sources used:

- Disassembly of the pixel programmes by `fxc /dumpbin /Fc` (Windows SDK 10.0.28000.0) from `C:/Users/User/.codex/I0063-cache/programs`
  (`gxflvershader__GXFlver_Water*_{Fwd,FwdSdw,FwdSSdw}[_[A]].ppo`). Disassembly was written to the session scratchpad, not the repo. Line
  numbers below are in the pixel body (after the `ps_5_0` line) of the named programme; they were checked on `Water_Fwd`, and the other
  families were compared against it (diff and the grep of the sample lines).
- `evidence/I-0063/artifacts/program-resources.tsv` (programme to resources), `evidence/I-0063/artifacts/scan/{flver,mtd,msb}.tsv`
  (per-FLVER-material MTD, MTD to shader family), `evidence/I-0066/queries/mtd-bits/{mtd-policy-bits,bit-usage}.tsv` (policy word per MTD,
  bits per map), `evidence/I-0068/queries/npc-families/all-mtd.tsv`, `evidence/I-0068/queries/water-ready/zone-msb-reflect.tsv` (MSB part positions),
  decompile `evidence/I-0066/artifacts/direct-model-contract-731/DS3-1152-F-000575C0.c`.
- Prior notes cited, not redone: WATER-NATIVE (I-0068), WATER-REFLECTION-INTERACTION-E08, WATER-TESS-WAVE1-READY (I-0066, sections 1.1, 1.3, 1.5),
  MTD-FLAG-BITS-INVENTORY-SHEET (bits 4, 5, 6), R6-PLAN and RESULT (I-0125).

---

## 1. Water programme families and the planar reflection

### 1.0 Answer in short

1. **Five FLVER families**, all with the same pixel structure; every one of their `Fwd`, `FwdSdw`, `FwdSSdw` programmes (and the `_[A]` alpha
   variants) samples `g_ReflectionTexture` (t54) once. The census lists exactly 30 `.ppo` (5 families x 6 programmes) plus `gxrenderershader__GXOceanWave.ppo`
   as the only programmes that declare `g_ReflectionTexture` or `g_RefractionTexture` (OBSERVED: `program-resources.tsv`, grep on both names).
2. **The reflection UV is identical in all five**: `u = SV_Position.x * FC_ScreenSize.z + N.x * FC_WaterFogParam0.z * 10/(10 + depth)`, `v = 1 - SV_Position.y * FC_ScreenSize.w`.
   Only the u coordinate is shifted by the bump (world normal x), the v coordinate is the plain flip. No other reflection source exists in these programmes
   (no IBL cube, no SSR texture): the reflection is **only** t54 (OBSERVED, resource tables: no t30/t31/t44).
3. **No water would be missed by a mask on bit 0x10 (`g_GXFT_PlanarReflection`).** All four water MTDs of the game set 0x10, 0x20 (Refraction) and 0x40 (WaterDepth) together
   (OBSERVED: word `0x70` in `mtd-policy-bits.tsv`; bit 4 and bit 5 counts equal on every map in `bit-usage.tsv`). The three tessellated Interaction families have **no MTD at all**.

### 1.1 The families (one row each)

Common to all rows: samplers s9 `SS_ClampLinear` for t54 and t55, s7 `SS_ClampPoint` for t56; t55 (refraction) sampled twice; t56 (depth), t57 (interaction map) sampled; t59..t62 light lists.
"Disasm ref" is the instruction line (in the pixel body of the `_Fwd` programme) of the t54 sample. The FwdSdw and FwdSSdw programmes differ only by the shadow lookups (t32, 4 samples in `FwdSdw`).

| Programme family (`gxflvershader__GXFlver_...`) | t54 | UV formula (reflection) | Other reflection source | Refraction (t55) | Notes |
|---|---|---|---|---|---|
| `Water` (`_Fwd`, `_FwdSdw`, `_FwdSSdw`, + `_[A]`) | yes (`Water_Fwd` line 175) | `(u + N.x*FogParam0.z*K, 1 - v)`, K = 10/(10+v6.w) | none | yes, `(u + dot(N,B)*FogParam0.w*K, v)` and `(u, v)` | two diffuse/bump layers (t0/t7, t3/t10), no third normal layer; no t16, no t58. MTD `m33_00_Water.mtd` only |
| `WaterBumpTri` | yes (`WaterBumpTri_Fwd` line 188) | same | none | yes, same | adds `t16` (third bump layer, the "3normal"). The three map-water MTDs |
| `WaterBumpTriInteraction` | yes (line 199) | same | none | yes, same | `WaterBumpTri` + `t58 g_WaterNormalTexture` (interaction normal map). No MTD; hull/domain variants exist (`_FT`) |
| `WaterBumpTriInteractionFloatObj` | yes (line 231) | same, **but the reflection weight is reduced** by a mask `r3.w` (`F' = F*(1 - m)`, line 225) | none | yes, same | adds t18, t19, t20 (floating-object foam layers). No MTD |
| `WaterInteraction` | yes (line 186) | same | none | yes, same | `Water` + t58 (no t16). No MTD |
| `gxrenderershader__GXOceanWave.ppo` (not a FLVER family) | yes (`resinfo` on t54, then a sample, `oc.asm` body lines 73, 93) | different (wave/Gerstner programme: `cbWave`, `cbGarstnerWaveParam`, reflection UV from a displaced position) | none seen | yes, `sample_l` on t55 | **TO CHECK whether the game ever draws it**: no `OceanWave` string in any retained scan or note except the resource table; not in any MTD/FLVER |

How the formula was read (OBSERVED, `Water_Fwd` pixel body, symbols from the disassembly header):

- `u = v0.x * cb8[1].z`, `v = v0.y * cb8[1].w` (`mul r6.yz, v0.xxyx, cb8[1].zzwz`). `cb8` is `cbSceneParam`, `cb8[1]` = `FC_ScreenSize`; `.xy` is the size (the light-tile code multiplies uv by `.xy` and truncates to a pixel), so `.zw` = 1/size (INFERRED from that use).
- `r12.y = 1 - v0.y*cb8[1].w` (line 174: `mad r12.y, -v0.y, cb8[1].w, l(1)`): **v is flipped, never perturbed by the bump.**
- `r12.x = u + (N.x * cb2[20].z) * (10 / (v6.w + 10))` (lines 172-176). `cb2[20]` is `cbMatDynParam` offset 320 = `FC_WaterFogParam0`; `.z` is the reflection distortion, `.w` (used by the refraction UV, line 468 `mul r1.w, r1.w, cb2[20].w`) is the refraction distortion. The value comes from the material (FLVER parameter), not from the MTD file: **TO CHECK** the values per material.
- `N` is the world-space normal after the bump (`r7.xyz = normalize(r1)` just before the view-matrix dot products `cb8[5..7]`): the two bump layers (t3, t10) are mixed by the vertex weight, the t57 interaction height taps (3 samples) add a gradient term. `N.x` is the world x component (INFERRED from the dots by `cb8[5..7]` coming after it: `FC_MatrixView` rows turn it to view space).
- `v6` (TEXCOORD7) is a clip-space position: it is fed to `dp4` with `cb8[8..11]` (`FC_MatrixInvViewProj`) and divided by the result's w; `v6.w` is then compared with the linear depth of t56 at the end (`r0.w = -cb8[2].z/(d - cb8[2].x) - v6.w`). So `v6.w` is the water pixel's linear view depth (INFERRED). `K = 10/(10 + depth)` shrinks the distortion with distance.
- Refraction UV: `u' = u + dot(N, B) * FogParam0.w * K` with `B` the normalised bitangent `cross(v1, v2) * v2.w` (lines 467-470), `v` unchanged. Two t55 samples, the distorted one kept only where its alpha is 0 (already in WATER-TESS 1.2).

Composition (OBSERVED, `Water_Fwd`, end of the body), for the reflection:

- `F = (1 - |N.V|)^5 * FC_WaterFresnel.x + FC_WaterFresnel.y` (lines 168-171; `cb2[22]`).
- `spec_accum += t54(uv) * F` (line 474 `mad r4.xyz, r12, r6.w, r4`), i.e. the reflection is **added to the specular term**, then the sum is multiplied by `FC_SpcMapMulCol * FC_GlobalSpecularMask` (`cb3[1].xyz * cb9[3].x`) and added to `lerp(refraction, diffuse-lit, FC_WaterFresnel.z)`.
- Final: `o0.rgb = lerp(t55(u,v) , fogged colour, mask)`, `mask = saturate((dot(V,N_flipped) * (FadeParam.x - Fresnel.w) + Fresnel.w) * (sceneDepth - waterDepth))` (end lines): the shoreline fade; `o0.a = mulCol.w * layer alpha`.
- **No IBL, no cube map** is read by any of the five (resource tables show no t30, t31, t44; `cbInstanceData.aSh` SH is read but it is the diffuse term). WATER-NATIVE's "also bound IBL" is therefore harmless.

What this means for the rays: the original's water reflection colour is exactly "the mirror view's pixel at the water pixel's screen position flipped in v, shifted in u by `N.x * k`". The RT pass replaces t54 (R6-PLAN, RESULT R6c) by ray results at `(u, 1-v)`. If the pilot wants to match the original's bump response, the only perturbation to apply is the u shift above (v is never perturbed); the mirrored direction from the true bumped normal is a different (better) physical model, a choice for the pilot, not a parity item.

### 1.2 Which MTDs and materials use each family; what a bit-0x10 mask misses

MTDs of `AllMaterialBnd` that map to a water family (OBSERVED: `evidence/I-0063/artifacts/scan/mtd.tsv`, 512 MTDs; policy words `mtd-policy-bits.tsv`):

| MTD | SPX / programme family | Blend | Policy word (bits) | Texture-type bits 0x10 (PlanarReflection) / 0x20 (Refraction) |
|---|---|---|---|---|
| `M[ARSN]_Water.mtd` | `GXFlver_WaterBumpTri` | 0 (opaque) | 0x70 (4,5,6) | both set |
| `M[ARSN]_Water_3normaltet.mtd` | `GXFlver_WaterBumpTri` | 2 | 0x70 (4,5,6) | both set |
| `M[ARSN]_Water_DL.mtd` | `GXFlver_WaterBumpTri` | 2 | 0x40070 (4,5,6,18; bit 18 = `GXFT_WaterApplyDynamicLight`) | both set |
| `m33_00_Water.mtd` | `GXFlver_Water` | 2 | 0x70 (4,5,6) | both set |

No MTD maps to `GXFlver_WaterInteraction`, `GXFlver_WaterBumpTriInteraction` or `...FloatObj` (OBSERVED: `mtd-policy-bits.tsv` has no such spx; MTD-FLAG-BITS sheet §0 item 5: no MTD sets bit 17, `GXFT_WaterInteraction`). WATER-TESS 1.1 says these variants are selected at run time by `GXWaterInteractionMaterialPatch` (static reading, not observed live): such a material is a water material by its MTD and so carries 0x10 already (INFERRED).

Where they are used (OBSERVED, `scan/flver.tsv`, material to MTD; every water piece has exactly one material and one mesh; counts are FLVER materials; matches bit-4 counts of `bit-usage.tsv`):

| Map | `Water_DL` | `Water_3normaltet` | `Water` (blend 0) | `m33_00_Water` / other families | Total (bit 4) |
|---|---|---|---|---|---|
| m30 (High Wall) | 3 (pieces 004002, 004003, 009500) | | | | 3 |
| m31 (Undead Settlement) | 3 (007700, 008040, 008315) | | | | 3 |
| m33 (Road of Sacrifices) | 12 (000070..000081) | 1 (000012) | | | 13 |
| m35 | 5 (000008..000012) | 1 (000013) | 1 (000007, `Material #361`) | | 7 |
| m37 (Irithyll) | | 6 (000020, 22, 23, 24, 25, 542) | | | 6 |
| m38 | 6 (001097, 001102, 002001, 002721, 002805, 003200) | | | | 6 |
| m39 (Irithyll Dungeon) | 8 (005000..005060) | | | | 8 |
| m40 (Firelink/Iudex) | | 3 (000010, 11, 12) | | | 3 |
| m45 (Ariandel) | | 4 (003021, 006100, 006101, 010440) | | | 4 |
| m50 (Dreg Heap) | 3 (001363, 020060, 030201) | | | | 3 |
| m51 (Ringed City) | 1 (020000) | | 1 (040002 `Swamp`, 84,048 indices, WATER-TESS 1.1) | | 2 |
| objects | | 2 (`o389050`, `o389051`) | | | 2 |
| m32, m34, m36, m41, m46, m47, m53, m54 | none in the scan | | | | 0 |

- **`GXFlver_Water` (`m33_00_Water.mtd`) is not used by any scanned FLVER material** (zero hits in `flver.tsv`; WATER-TESS 1.1 also says it is "in the game and not in the five zones"). So it is data-only today: the pilot still loses nothing by treating it like the others.
- The map labels in the left column are from the project's notes where they exist (m30, m31, m33, m37, m39, m40, m45, m50 appear with these names in the repo); m35, m38, m51 names are not asserted here (**TO CHECK**: m35 and m38 hold underground or lake water; do not rely on a name).
- `scan/flver.tsv` does not list every map (no m36 rows with water; maps absent from the scan are not claimed): the claim "no water elsewhere" is limited to the scanned FLVERs (m30..m34, m35, m37..m41, m45..m47, m50, m51, m53, m54, objects).

**Water that a mask on bit 0x10 alone would miss: none among MTD water.** Bits 0x10 and 0x20 are always set together (all four MTDs; per-map counts equal).

What a bit-0x10 mask correctly does *not* catch (so no change needed), and could be mistaken for water:

- Materials with water-like names but **ordinary MTDs**: `AL_Swamp` (m37 000027, `M[ARSN]_al`, `GXFlver_ColDifSpcBumpIbl`), `SwampTest` (m37 000027, 004507, `MulMask` families), `沼地ブレンド...` (m51 020005 swamp *ground*, `GXFlver_ColDifSpcBumpDOL` / `MultiBlend3`), `地底湖地面` (m38 underground lake *ground*, `MultiBlend2_et1`), `凍った湖B` (m45 frozen lake, `MultiBlend2_et1_em_lightmap`), mud/ground materials. They are land textures; the original gives them **no planar reflection** (no t54 read). OBSERVED: names + MTD from `flver.tsv`; that they have no t54 read: census (only the 30 water programmes + OceanWave read `g_ReflectionTexture`).
- `GXFlver_Slime` (`c3000[slime].mtd`, a character family): reads IBL t30/t31 and `g_RLRTexture` t44 (screen-space reflection), no t54/t55 (OBSERVED: `program-resources.tsv`, `GXFlver_Slime_Fwd.ppo`).
- `ColDifSpcBumpTrIbl` / `Slime` variants "water-like meshes" at m31/m45/m50/m51 (WATER-NATIVE §1 correction): drawn in the G-buffer, no t54.

Model-level note (OBSERVED, `DS3-1152-F-000575C0.c` lines 56 and 80): in the original, the water decision is read from the **model** aggregate `model+0x224` (bit 4 at line 56, bit 5 at line 80), not per material, so a model with any bit-4 material draws all of its draws as kind 0x27. In the data above every water piece is a one-material FLVER, so slice-level and model-level coincide today (OBSERVED for the scanned FLVERs). This matches the pilot's current rule (R6c: `RtMaskWater` on 0x10 slices of an eye-visible instance).

---

## 2. Non-flat / sloped water, and the reflection plane

### 2.1 Answer in short

- **One plane per frame, for the whole view.** The planar reflection is a single mirror view built from **one** plane `(n, d)` (frame object `frame+0x3F90`), chosen among all water draws of the main view as the one with the smallest scalar `t`; it is not per water piece and not a global fixed height (OBSERVED: `FUN_1400575C0` decompile, lines 56-80; prior note WATER-TESS 1.5, VERIFIED code). Every water pixel then reads the same reflection texture, whatever its surface normal; the only per-pixel adaptation is the u shift of 1.1.
- **The plane's normal is not the water mesh's surface normal.** It is the draw placement's second axis (`p5+0x10..0x18`, with `p5+0x1C` also read): the model's local Y axis (INFERRED: row 1 of the placement matrix). Its offset is `d = 0 - n . c` with `c` the bounding-box centre of the draw (lines 53-71). So:
  - a sloped river or a stepped pool that is baked in world space (m45, m50, m51: placement identity, WATER-TESS 1.1 "geometry baked in world") gets **n = +Y** and the plane height of the *box centre of the nearest draw*; the surface slope is never seen by the mirror camera (INFERRED: from n = placement axis);
  - a piece with a rotated placement (m31, m37 pieces have a part position; rotation not extracted here) gets the rotated Y axis (**TO CHECK**: any water placement with a rotated Y axis; I did not extract part rotations).
- The mirror camera is `p' = p - 2 (n.p + d) n`, composed with the view and with a constant Y-flip matrix (WATER-TESS 1.5, VERIFIED constants); that composition is what the shader's `1 - v` undoes.
- **The plane is reset every frame** to the constant `(0, 1, 0, 0)` with `t = FLT_MAX` (`FUN_140023790:261-271`), then replaced by the nearest water candidate, or by the height default (collision `ReflectPlaneHeight` under the camera target, INFERRED) when `frame+0x3ECD` and a flag hold (WATER-TESS 1.5; not re-read here). Whether the frame N mirror uses the plane found in frame N or N-1 is still **OPEN** in the earlier notes (WATER-REFLECTION-INTERACTION-E08 §1.2 says N-1 from lines 2277-2287; WATER-TESS 1.5 says OPEN WL3). I did not resolve it.
- Candidate gating (OBSERVED, `575C0` lines 49-82): only for the main view (`param_1+0x124 == 0x45`); `t` is a dot of the box centre (with the constants at `1439F2500..`) with `view+0x250..0x25C`; the candidate is kept when `t < view+0x894` and also sets `view+0x891 = 1`. The meaning of `t` (distance along the view axis, INFERRED) is not resolved.

### 2.2 Per-map data (OBSERVED, `zone-msb-reflect.tsv` and WATER-TESS 1.1)

- Collision `ReflectPlaneHeight` equals the water height for the main level at m31 (-155) and m37 (-241.76), with a few other values (-198.49, -208.5, -240.82 at m31; -247.56, -277.16 at m37); at m45 it is non-zero on only 8 of 187 collisions (-63.6, -66.7, +4.1); m50 -72.767 on 4 of 126; m51 none (WATER-TESS 1.1, same TSV). So for the zones with several water levels the original has **one plane**, and pieces at another level get a reflection mirrored about the wrong height (INFERRED from "one plane per frame"; the visual effect in the original was not looked at).
- MSB flag `DrawByReflectCam` (what the mirror view draws) is set on 3,765/3,782 pieces at m31, 2,312/6,635 at m37, 0 at m45 (one `DrawOnlyReflectCam`), 2,100/3,653 at m50, 122/6,292 at m51 (WATER-TESS 1.5). The mirror view of the original therefore shows a **subset** of the world; the RT version traces everything. (Cited, not redone.)

### 2.3 Waterfalls, rivers, swamps, lakes

- **No water-family programme/MTD is attached to anything named as a waterfall in the scanned FLVER materials.** A name search over `scan/flver.tsv` (waterfall/滝/fall/flow/river/川/swamp/沼/lava/溶岩/mud/泥/pond/池...) returns land and ground materials (mud, swamp ground, underground-lake ground, frozen lake) plus water pieces; none is a waterfall with a water MTD (OBSERVED, search results above in 1.2). A waterfall is therefore most probably an effect (FFX, particles) or an ordinary material: **TO CHECK** (no waterfall asset was identified; I did not browse FFX or object materials).
- Because only the 30 water programmes (and OceanWave) read t54 (census), **any non-water-programme surface has no planar reflection in the original**; a waterfall drawn by an FFX or an ordinary material would show no mirror, and for the ordinary materials the cube/IBL or screen-space `g_RLRTexture` (t44) applies if their programme reads it (INFERRED from the census; the individual waterfall materials not identified).
- Rivers and stepped water at map scale are several water FLVER pieces (m33: 13, m39: 8, m35: 7, m38: 6): each is drawn kind 0x27 and reads the same t54. There is **no vertical or sloped special case** in the pixel programme: the shader's normal `N` is the bump-perturbed normal around the FLVER's own vertex normal (tangent frame `v1`,`v2`), but the reflection lookup only uses `N.x`, so a vertical sheet would sample the same horizontal-plane mirror image shifted by its `N.x` (INFERRED from the formula; no vertical water piece was identified in the data).
- The m51 swamp (`040002`, `Water` blend 0, 84,048 indices, WATER-TESS 1.1) is a single huge opaque water piece with a world-baked placement: plane normal +Y, plane through the box centre of that draw (INFERRED); its height varies across a 14,265-vertex mesh that I did not measure.

### 2.4 What the RT pilot can take from this

- Using +Y and the nearest water box centre (WT-6 as in R6-PLAN) reproduces the original's plane choice for baked or unrotated pieces; using each water slice's own ray hit (R6c's "water's own surface first") is **more accurate than the original** on stepped or multi-level water and intentionally departs from it.
- A mask on `g_GXFT_PlanarReflection` is sufficient for all water that has a `t54` read. Exclude from `RtMaskWater` the land "swamp/lake" materials listed in 1.2 (the current rule already does since it keys on the bit).
- If the pilot also wants waterfalls to reflect, no data here says what they are; this needs an asset search (TO CHECK).

---

## 3. Sky pieces per map (partial)

Source: `evidence/I-0063/artifacts/scan/flver.tsv` (material name, MTD) joined to `mtd.tsv` (MTD to SPX), filtering on sky-like material names (天球 "celestial sphere", 雲 clouds, 星空, `sky`, `cloud`, `Sky`) and the MSB part table `zone-msb-reflect.tsv` (only zones m31, m37, m45, m50, m51 are in that table; no MSB position for the others). Piece names are the model file numbers (`mNN_00_00_00_NNNNNN.flver`); the MSB part model is `mNNNNNN`.

| Map | Piece (flver) | Materials (name: MTD: SPX) | MSB position (OBSERVED where present) |
|---|---|---|---|
| m30_00 | 009000 | 天球 `M[A]_NoLight` `GXFlver_Col`; 光 `M[A]_al_NoLight` `GXFlver_Col`; 光2 `M[A]_add` `GXFlver_ColDif`; 雲, 雲_上層 `M[A]_al_NoLight` `GXFlver_Col` | not in the table |
| m30_00 | 901030 | 積雲1/2 `M[A]_al` `GXFlver_ColDif` (cumulus) | not in the table |
| m30_01 | 009000, 902200 | same set as m30_00 (dome, glow, clouds); cumulus | not in the table |
| m31_00 | 009000 | `sky` `M[A]_2m_NoLight_scroll` `MultiBlend2_et1_nolight_MuitiBlendMask` | (50, -155, -580), draw_by_reflect 0 |
| m31_00 | 008501 | `cloud1`, `cloud1_scroll違い` `M[A]_al_NoLight` `GXFlver_Col` | not in the table |
| m33_00 | none found (no sky-named material in the scan) | | |
| m35_00 | 601000 | 積雲1/2 `M[A]_al` `GXFlver_ColDif` (clouds only, no dome found) | not in the table |
| m37_00 | 009000 | 天球半透明 `M[A]_al_NoLight` `GXFlver_Col`; 天球 `M[A]_NoLight` `GXFlver_Col`; 真上ブレンドテストスロー2 `M[A]_2m_NoLight_scroll` `MultiBlend2_et1_nolight_MuitiBlendMask` | (423.198, -241.76, -925.096), draw_by_reflect 0 |
| m38_00 | 002995 | 天球 `M[A]_NoLight` `GXFlver_Col` | not in the table |
| m39_00 | 009010 | `Sky` `M[ARSN]_em` `GXFlver_ColDifSpcBumpEmiIbl` (lit, emissive; underground ceiling) | not in the table |
| m39_00 | 009020 | TransCloud2/3 `M[A]_al` `GXFlver_ColDif` | not in the table |
| m40_00 | 009000 | same three materials as m37 (半透明, 天球, 真上ブレンドテストスロー2) | not in the table (WATER-TESS: m40 is Firelink/Iudex; the pilot's m40 dome) |
| m40_00 | 008052, 008053, 008091, 908820 | 雲視差テスト2 (cloud parallax test) `M[A]_al_NoLight` `GXFlver_Col` | not in the table |
| m41_00 | none sky-named | | |
| m45_00 | 090000 | ベース空 (base sky), 星空 (starry sky): `M[A]_2m_NoLight_scroll` `MultiBlend2_et1_nolight_MuitiBlendMask` | model `m090000`, (0, 0, 0), draw_by_reflect 0 |
| m45_00 | 091020, 091021, 091022 | 雲エフェクトA/B/C `M[A]_al_m_NoLight_Opacity` `Fujir_Nolight_OpacityBlend` | not in the table |
| m46_00 | 009000, 902200 | same set as m30_00 (dome, glow, clouds); cumulus | not in the table |
| m47_00 | 009005 | `Material #271` x2 `M[A]_NoLight` `GXFlver_Col` | not in the table |
| m50_00 | 040001 | 天球 `M[A]_NoLight` `GXFlver_Col` (673 indices per WAVE1-LAST-EFFECTS-STATIC-PREP 3.2) plus cloud layers | model `m040001`, (0, 0, 0), draw_by_reflect 1 |
| m50_00 | 090002 | `Test_sun` `M[A]_al_NoLight` `GXFlver_Col` (the eclipse disc and ring, INFERRED by that note) | model `m090002`, (-493.10, 0, 153.96) |
| m50_00 | 090020 | 遠景雲A `M[A]_al_NoLight` `GXFlver_Col`; 雲エフェクトA/B `M[A]_al_m_NoLight_Opacity` `Fujir_Nolight_OpacityBlend` | model `m090020`, (0, 0, 0) |
| m50_00 | (m090001 in the MSB) | `そら1` per WAVE1-LAST-EFFECTS-STATIC-PREP 3.2, not matched by my filter | model `m090001`, (0, 0, 0), draw_by_reflect 1 |
| m51_00 | 900001, 900002 | 天球 `M[A]_NoLight` `GXFlver_Col` | models `m900001`/`m900002`, (-500, 0, -500) (the zone origin used for the whole map) |
| m51_00 | 900011 | 雲海1 (cloud sea) `M[A]_2m_NoLight_scroll` `MultiBlend2_et1_nolight_MuitiBlendMask`; 近景雲1 `M[A]_al_m_NoLight_Opacity` `Fujir_Nolight_OpacityBlend` | model `m900011`, (-500, 0, -500) |
| m51_00 / m51_01 | 000014, 900100, 900101, 090015 | 雲エフェクト `M[A]_al_m_NoLight_Opacity` `Fujir_Nolight_OpacityBlend` | not in the table |
| m54_00 | 009000 | identical to m37/m40 (半透明, 天球, 真上ブレンド) | not in the table |
| m32, m34, m36, m53 | no sky-named material in the scan | | |

Families seen (OBSERVED, `mtd.tsv`): `GXFlver_Col` (MTDs `M[A]_NoLight`, `M[A]_al_NoLight`; forward-only `NoLight`), `GXFlver_ColDif` (`M[A]_add`, `M[A]_al`), `MultiBlend2_et1_nolight_MuitiBlendMask` (scrolling zenith layer), `Fujir_Nolight_OpacityBlend` (cloud sheets), one lit exception `GXFlver_ColDifSpcBumpEmiIbl` (m39 `Sky`). The pilot's m40 dome (`GXFlver_Col_Fwd`) is the same material as m37's 天球 `M[A]_NoLight` and is shared by m30, m30_01, m38, m46, m50 and m51 (same MTD, same SPX).

Not reached or unreliable in this table:

- The `scan/flver.tsv` pass is a name filter; a sky piece whose materials carry neutral names (`Material #271`, `default`) would be missed unless its piece number begins `009` or `09`: **TO CHECK** for m32, m33, m34, m36, m41, m53 (no hit does not prove no sky; m33's nine-thousand pieces are lit map pieces).
- MSB positions exist only for m31, m37, m45, m50, m51 (the table I could read).

### 3.1 Does the sky follow the camera?

- **The vertex stage does not re-centre the dome.** `GXFlver_Col_Fwd.vpo`: `world = M * pos` (the `cbObjMatrix` rows indexed from `cbInstanceData`), then `world - SC_CameraPos` (`cb8[3]`), then `VC_MatrixViewProj` (`cb8[24..27]`) (OBSERVED, `v.asm` instructions 5-12). The same camera-relative subtraction is in an ordinary map programme (`GXFlver_ColDifSpcBumpDOL_Fwd.vpo`, lines 153-155) and in `WaterBumpTri_Fwd.vpo` (line 165): it is the engine-wide convention, **not** a sky-specific follow.
- **The MSB places each dome at a fixed map position** (m37 at (423.2, -241.8, -925.1), m31 at (50, -155, -580), m50/m45 at (0, 0, 0), m51 at (-500, 0, -500)): not at the camera (OBSERVED, MSB table).
- Therefore whether the dome moves with the camera is decided by the engine placement (a per-frame matrix update for these parts) or by the dome being large enough that translation is imperceptible. **Not determined: TO CHECK.** I did not find a note or code establishing either (searches for follow/dome/sky in `investigations/` and `native/` returned nothing on it). A cheap decider: the FLVER bounding box (radius) of `m37_00_00_00_009000` against the map size, or one live capture reading the dome's `cbObjMatrix` translation at two camera positions.

---

## What I could not reach / limits

- Values of `FC_WaterFogParam0` (.z reflection distortion, .w refraction distortion), `FC_WaterFresnel` per water material: they live in the FLVER material parameter blocks, not in the MTD file; not extracted.
- Whether the mirror view of frame N uses the plane of frame N or N-1 (OPEN WL3 in the earlier notes); part rotations of water placements (to know if any water plane is non-horizontal); the identity of the game's waterfalls; whether `GXOceanWave` is ever drawn; sky follow-camera behaviour; sky pieces of maps m32/m34/m36/m41/m53 and the MSB positions of the maps other than m31/m37/m45/m50/m51.
- `scan/flver.tsv` includes the maps listed in 1.2 but is a scan of the unpacked archives I did not re-run; the DLC maps m45/m50/m51 appear there under bare file names.
