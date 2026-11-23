# R7 study: DS3's own ambient, SSAO and baked GI, what traced GI replaces or feeds

2026-11-06, a read-only static study for the R pilot (RT-PT-PLAN §6 and §8 R7, D-0247). Nothing was launched, built or edited; the
only tools run were `fxc /dumpbin` on two single programmes of the I-0063 cache (the forward `ColDifSpcBumpIbl_Fwd` and
`ColDifSpcBumpDOL_Fwd`, output in the session's scratchpad, not kept) and a read of four floats of `DarkSoulsIII.exe`'s `.rdata`
(PE section walk, Python). Statuses: OBSERVED (read in a primary source: a disassembly, a native code line, an evidence file; the
path is given), INFERRED (from what is said), TO CHECK. `evidence/I-0125/artifacts/dxbc/` is written `dxbc/` below; the I-0119
census disassemblies `evidence/I-0119/artifacts/census/asm/` are `census/`. Already established and not redone: R6-PLAN §1,
GBUF-FAMILIES, the SSAO studies.

## 0. In short

1. **One formula carries all of DS3's ambient.** lit = fog( GB2 × [ GB4 × t32.x × t32.y + Σ dir-light diffuse_i (1 + w_i (AO − 1)) +
   local diffuse × t33.x ] + [ SpecularAcc × t32.x × t32.y + Σ dir spec + local spec × t33.x ] + GB5 ). SSAO enters three times
   (ambient, the four directional lights by per-light weights, the local lights unweighted), and the baked ambient GB4 once.
2. **The baked ambient can be switched off at its source without touching any DXBC**: `cbPerFrame +0xCC` (FC_DominantColorScale) at 0
   zeroes every lightmapped GB4 (and the forward lightmapped rows), the instance SH rows at 0 zero every Ibl GB4 (and the forward Ibl
   rows). SSAO off is the existing neutral-white t33. Both are constants the native already produces.
3. **Natively there is already a place that looks like R7's**: R4's local-light pass (`EncodeRtLights`) is a full-screen native pass
   that adds into the lighting's targets after pass 0 and 1. The traced diffuse GI fits after `GXLightCmbAlbedo` (pass 2), adding
   `GB2 × (1 − F0)`-modulated irradiance into `light_targets[2]` (R16G16B16A16F), before the SSS and the composite (§4).
4. **Open and not read here**: the lightmaps' format and resolution (no census in the repository), which G-buffer draws write stencil
   bit 0x10 (it decides whether GB1.z gates the sun or the ambient), what the decals write into GB4, the forward rows' t33 and the
   IEM / `IemDiff` variants' use.

## 1. Per G-buffer family: what GB4 holds, what multiplies it

### 1.1 The final-lighting terms and where SSAO and the masks enter (OBSERVED)

| Term | Formula | Source |
|---|---|---|
| Diffuse ambient | `GB4 × (t32.x × t32.y)` (`GXLightDeferredAO` o0), later × GB2 (`GXLightCmbAlbedo`: `t6 × t4`, alpha from t4) | `dxbc/GXLightDeferredAO_ppo.asm:44-48`, `dxbc/GXLightCmbAlbedo_ppo.asm:38-41` |
| Specular ambient | `SpecularAcc × (t32.x × t32.y)` (o1; added to the lit by `GXLightComposite`: `t6 + t7 + t8` then fog) | `GXLightDeferredAO_ppo.asm:49-50`, `dxbc/GXLightComposite_ppo.asm` (the three samples `t6`, `t7`, `t8` summed, then the fog rows of `cbFog[set]`) |
| Directional light i diffuse and specular | `... × (1 + w_i × (t32.x − 1))`, w_i = `FC_SsaoWeight` row 17 of the light's set (`cbLight`, 18 rows a set, set = GB3.w × 255); light 0 also × `t32.z` and the shadow tint `lerp(1 − FC_ShadowColor, 1, t32.w)` | `dxbc/GXLightAcc_DDirLightShadowed1_ppo.asm:177-187` (`r6 = cb5[set×18+17] × (t32.x − 1) + 1`, `r5 = t32.xzwy`), `:199`, `:221-223`, `:267`, `:274` |
| Local lights (point, spot, line) | diffuse and specular × `t33.x` directly (the SSAO texture, no weight) | SSAO-NATIVE §1 readers table (STATIC); native: `rt_lights.hlsli:34-35`, `rt_lights.hlsl:19` (the `ao` slot) |
| Emissive | `GB5` added in the composite, no AO, no mask | `GXLightComposite_ppo.asm` (`t8`) |
| Skin | the SSS blur reads the CmbAlbedo output (`light_targets[2]`) between pass 2 and the composite | `scene_pass_lighting.cpp:428-435`, `scene_pass_internal.h:122` |

**What t32 holds** (OBSERVED unless said). `GXRend_CompositeShadowMask` writes `o0 = (t33.x, FC_Mult.y × GB1.z + FC_Add.y,
FC_Mult.z × GB1.z + FC_Add.z, 1)` into the mask whose `w` is the sun shadow (`SUN-SHADOWS-C4-STATIC-PREP.md:138`, STATIC from the
programme's reflection; `GXRend_CompositeShadowMask` itself is not disassembled in `dxbc/`). The two draws' constants are
`.rdata` words the native reads at `13b_prepare_model_frame.inc:1501-1502`; I read them in `DarkSoulsIII.exe` (RVA, little-endian
floats): `0x3D7F690 = (0, 0, 1, 0)` and `0x3D7FC00 = (1, 1, 0, 1)` (draw 0, stencil reference 0), `0x3D7F370 = (0, 1, 0, 0)` and
`0x3D7FE70 = (1, 0, 1, 1)` (draw 1, reference 0x10). The order Mult then Add is INFERRED from the native's `{FC_Mult, FC_Add}` comment
(`light_producers.cpp:210`). Result:

| Pixels (stencil bit 0x10 of the G-buffer) | t32.x | t32.y (ambient mask, both ambient terms) | t32.z (sun mask) | t32.w |
|---|---|---|---|---|
| clear (draw 0, ref 0; the common case) | SSAO | **1** | **GB1.z** | sun shadow |
| set (draw 1, ref 0x10) | SSAO | **GB1.z** | **1** | sun shadow |

GB1.z is the lightmapped bodies' `t26.a` and the Ibl bodies' `instance.ShLightMask.x` (`census/…ColDifSpcBumpDOL_Gbuf.ppo.asm`:
`sample … t26` then `mov o1.z, r2.w`; GBUF-FAMILIES §0, §1). So on the common pixel GB1.z is a **mask on the sun** (light 0), and
the forward programmes agree: `ShLightMask.x` multiplies light 0's colour and spec colour (`cb4[…+11].x × cb5[4]` and `× cb5[8]`,
`ColDifSpcBumpIbl_Fwd` disassembly lines 520 and 545). Whether bit 0x10 is ever set natively is TO CHECK: the native writes `|8`
(opaque), `|4` (SSS), `|2` (SSAO-NATIVE §1 notes, from `Native1137`); SSAO-NATIVE risk 2 names it as unread. The composite draws it
with `OMSetStencilRef(draw ? 0x10 : 0)` (`scene_pass_lighting.cpp:731`). OBSERVED that the AO channel is NOT gated by it.

The material's `FC_SSAOParam` (`cbMtdParam +0x30`) is **unused in the G-buffer programmes** (`census/…DOL_Gbuf.ppo.asm:73`
`[unused]`, reference and Ibl): in the deferred path every pixel takes the full `t33.x` on the ambient and the per-set weights on
the directional lights. It is read by the forward programmes only (§2.3).

SSAO weights per zone (OBSERVED from the decoded GParam, SSAO-D3D12-DESIGN §2.3), light set 0, lights 0 to 3: m30 (0.70, 1, 0.76,
1); m31 (0.53, 1, 0.75, 1); m37 (0, 1, 0, 1); m40 (0.75, 1, 0, 1); m45 (0.80, 1, 0.75, 1); m50 (0.73, 1, 0.60, 1); m51 (0.80, 1,
0.75, 1). Light 1 (the unshadowed second light, the one a fill or ambient-like direction in DS3's sets, INFERRED) is fully
AO-weighted everywhere. CONTRADICTED: see I-0126's copy of this study and I-0129 RESULT « R9d » (light 1 exempt from the AO in
Firelink's and Anor Londo's sets; the decoded table's L1 and L2 likely swapped).

### 1.2 The families

| Family group (GBUF-FAMILIES §) | GB4 holds | GB1.z | SSAO / AO place | What R7's GI replaces |
|---|---|---|---|---|
| **A. Lightmapped (DOL)**: `ColDifSpcBumpDOL`, `…EnvDOL`, `…DOL_SnowLt`, `GrassSpcBumpDOL`, `GrassDOL`, `ColDifSpcPomDOL`, `DirtMaskDOL`, `MulDOL`, `MulMaskDOL`, `MultiBlend{2,3,4}_et1_lightmap`, `FourBlend_lightmap`, `Rock/Sand/Blood/Rotten/SSSLightmap_lightmap` (§0, §3-5, §11, §12, §14-16) | the **baked lightmap** (not SH): `(1 − F0) × L × (s (1 − F0) sat(D·N) + 1 − s)`; L = `t26.rgb² × cbTexHdrScaleParam × FC_DominantRgbScale (cbPerFrame +0xD0) × FC_DominantColorScale (+0xCC)`; D = `t27.rgb × 2 − 1` rotated by `vDominantRotateParam`; s = `sat(t27.a + FC_DominantSharpnesOffset (+0xC4))` (OBSERVED, `census/…DOL_Gbuf.ppo.asm`: `t26` line 52, `t27` 63, `cb9[12/13]` 56-94, `o4` 96; R6-PLAN §1) | `t26.a` | ambient × t32.x (full SSAO) × t32.y; no per-material weight | all of GB4 and the SSAO factor |
| **B. Ibl / SH**: `ColDifSpcBumpIbl`, `…BlendIbl`, `…TrIbl`, `…MulIbl`, `…EmiIbl[Glow]`, `…Bloody`, `…Df`, `SkinIbl`, `SSSIbl`, `Eye*`, `Hair*`, `Equip`, `SAT_*`, `GrassSpcBump`, their Cloth and Phantom twins, the map blends without a lightmap (`MultiBlend*_et1`, `FourBlend`, `BloodBlend`, `Rotten`, …) (§1, §2, §6-8, §10, §16-18) | `(1 − F0) × max(0, (aSh0·(N,1), aSh1·(N,1), aSh2·(N,1)))`, the **instance's** SH rows (`cbInstanceData +0x80`), N the final shading normal (OBSERVED, GBUF-FAMILIES §0, §1) | `instance.ShLightMask.x` | same as A | all of GB4 and the SSAO factor; the rows come from the engine's probe pick (§3.2) |
| **C. `ColDif_Gbuf`** (§13) | `max(0, SH(N))` **without** `(1 − F0)` (F0 is 0 there: `o3 = 0`); albedo = `FC_DynamicDiffuseColor × t0` | `ShLightMask.x` | same | same |
| **D. Unlit colour** `Col`, `ColSh` (§13) | **0** (`o2 = o3 = o4 = 0`); the colour goes to GB5 | `ShLightMask.x` | multiplies zero | nothing to replace (stays emissive-like) |
| **E. `*_nolight*` blends** (`MultiBlend2/3/4_et1_nolight*`, `Fujir_Nolight_OpacityBlend`) (§16b, STEP 3 « 52 = constant 0 ») | no SH ambient (`o1.x = 0`, census text; the `o4` of these bodies not re-read) | n.r. | none | nothing, INFERRED |
| **F. Emissive** (`Emi*`, `Glow`, `EmiMul*`, SAT glows) (§6) | the A or B ambient as above, **plus** `o5 = FC_EmissiveColor × t6` into GB5 | as A or B | GB5 gets no AO | GB4 only; the emissive stays and, later, may become a traced emitter (RT-PT-PLAN §8 lead) |
| **G. Snow second pass** `…_SnowLt_Gbuf2` (§11) | as its base family; reads a `t35` screen copy for `o1.x` | as base | as base | as base |
| **H. Decals** (7 targets, `scene_pass.h:321`) | they write GB4 among their targets (SpecularAcc, GB1, GB2, GB3, GB4, GB5, GB0) | TO CHECK | TO CHECK | TO CHECK what a decal writes into GB4 before R7 overwrites or ignores it |
| **I. Water** (`WaterBump*_Fwd`, forward) | not in the G-buffer: forward programme; its reflection is `t54` (R6c) | n.a. | TO CHECK (its ambient is in the forward body, not read) | R9 |

Coverage (OBSERVED, I-0110 RESULT §6, SoulsFormats census): lightmapped bodies are 3,186 + 323 of 3,605 m30 map-piece meshes (97 %
with the `MultiBlend*_lightmap` counted) and 1,071 + 88 of 1,296 at m40 (89 %); the rest (`Ibl`, `Col`, grass, water, no MTD) are 96
and 137 meshes. So R7's baked-ambient replacement is overwhelmingly the lightmap family A; the SH family B is characters, props
and the unlit-lightmap remainder.

## 2. The frame order today, natively, and every place R7 would replace or feed

### 2.1 Main-view order (OBSERVED, `native/renderer/`)

| # | Pass | Where | Ambient and AO terms it touches |
|---|---|---|---|
| 1 | G-buffer, 6 targets: GB0 RGBA16F, GB1 RGBA8, GB2 RGBA8 sRGB, GB3 RGBA8, **GB4 R11G11B10F**, GB5 RGBA16F | `scene_pass_internal.h:31-33`; decals after it write GB4 too (`scene_pass.h:321`) | GB4 written by the programmes of §1.2 |
| 2 | colours to PSR | `scene_pass_lighting.cpp:332-334` | |
| 3 | **SSAO chain** (depth downsample, AO, reprojection, bilateral upscale → t33) then the **mask** t32 (sun shadow, traced when `rt_sun_now_`, else cascades; two composite draws refs 0 and 0x10) | `EncodeNativeShadowMask`, called at `scene_pass_lighting.cpp:355`, defined `:655-732`; the AO recorded through `ao_.Record` (`:676`, `ao_stage.h`, the passes of SSAO-D3D12-DESIGN §1); traced sun `:688-704`; composite draws `:729-731` | **t33 (SSAO)**, t32.x/y/z/w |
| 4 | env regions into `SpecularAcc` (R11G11B10F, RTV slot 7): R6b traced (`EncodeRtReflections`, `:374`, `:465-`), else the region draws (`EncodeNativeEnvRegions`, `:734-`) | | specular ambient |
| 5 | the water's `t54` traced (R6c) | `:377` | |
| 6 | **pass 0 `GXLightDeferredAO`**: t6 = GB4, t7 = SpecularAcc, t32 → targets 0 and 1 (R11G11B10F: `formats[4]` at `scene_pass_lighting.cpp:198`) | `:406` (`pass(0,{0,1})`) | **GB4 × t32.x·t32.y; SpecularAcc × t32.x·t32.y** |
| 7 | pass 1 the sun, `GXLightAcc_DDirLightShadowed1[_Cloud]` additive into the pair | `:406` (`pass(1,{0,1})`; PSO choice in the `pass` lambda just above it) | dir lights × `1 + w_i (AO − 1)`, light 0 × t32.z × shadow |
| 8 | local lights: R4 native traced pass `EncodeRtLights(list,input,t33)` or the raster batches | `:417-424` | local lights × t33.x |
| 9 | pass 2 `GXLightCmbAlbedo` → `light_targets[2]` (R16G16B16A16F) | `:427` | × GB2 |
| 10 | SSS blur X/Y on `light_targets[2]` | `:428-435` | |
| 11 | pass 3 `GXLightComposite` → `light_targets[3]` (lit HDR, R16G16B16A16F): + specular + GB5, then fog | `:439` | |
| 12 | RR guides (GB0..GB3 + depth) and the lit HDR copied as « before transparency » | `EncodeRrGuides`, `:441`, `:599-616` | |
| 13 | forward pass 0x29 into the lit HDR, effects, DLSS or RR (`scene_pass_frame.cpp:877-917`), YEBIS | `scene_pass_frame.cpp:661` | forward ambient (§2.3) |

### 2.2 The places R7's traced diffuse GI has to replace or feed (the list)

1. **Pass 0's diffuse output** (GB4 × AO): replaced (the GI term is added later, §4; GB4 zeroed at its source).
2. **The SSAO chain and t33/t32.x**: removed (RT-PT-PLAN §6: « remplace SSAO »): t33 and t32.x go to the neutral white the lighting
   already binds when `ao` is null (`scene_pass_lighting.cpp:398`, `ao ? ao_.Output() : native_light_neutral_[0]`), which also makes
   the per-light weights and the local lights' `t33.x` neutral. Note `EncodeNativeShadowMask` also builds the mask's `w` (sun
   shadow) and t32.y/z from GB1.z: the mask must stay (only its AO channel goes), so the AO stage is removed and the composite
   draws kept with a white t33: `scene_pass_lighting.cpp:355` currently runs both under `if(ao)`, TO CHECK how the plan key and the
   `ao` input split them (the traced sun already writes the mask without the layer, `:688`).
3. **GB4 itself**: zeroed at the source (§0 point 2).
4. **R6a/R6b's hit ambient**, `RtAmbientOf(h, frame.dominant)` at `rt_reflect.hlsl:132` (HitRadiance): replaced by the radiance cache's value at the hit (one bounce), with the baked value kept as the cache's seed (§3).
5. **The forward rows' ambient** (§2.3): R9's line in the plan (« éclairés par le cache de radiance »); R7 must offer them a feed.
6. **Specular ambient**: not R7's (R6b done); but the AO factor on it (t32.x) goes with SSAO.
7. **The control view** (`rt_view.hlsl:101-107`) compares the hit's ambient with GB4; with GB4 zeroed the check needs a copy of the baked GB4 or a flag.

### 2.3 What the forward rows use for ambient (OBSERVED, `ColDifSpcBumpIbl_Fwd` and `…DOL_Fwd` disassembled with `fxc /dumpbin`)

`ColDifSpcBumpIbl_Fwd` (861 lines): the SH ambient is evaluated per pixel from `cbInstanceData` rows `+8, +9, +10` (`aSh`: `dp4 r3.x,
r2, cb4[r0.w + 8]`, `max 0`, lines 510-514), times `(1 − F0)`, times the AO factor
`ao = lerp(1, t33.x, FC_SSAOParam.x) × (COLOR1.x == 0 ? 1 : COLOR1.x)` (lines 392-397: `t33` sampled at `SV_Position × FC_ScreenSize.zw`;
`cb3[3].x` = `FC_SSAOParam.x`; COLOR1.x is a vertex AO), summed with the directional lights at line 837
(`r1 = ambient × ao + r10`; `r10` the lights' diffuse) and multiplied by the albedo at 839; the specular environment is the
instance's two IBL cubes `t30/t31` blended by `envMapInterpRatio` (`cb4[+13].w`, lines 454-502), also × ao. The directional lights
come from `cbLight` rows 0-11, 16 (`FC_AmbientSH`, `FC_AmbientReflection` and `FC_SsaoWeight` are `[unused]` in this programme); the
local lights from the light grid `t59..t62`. `ColDifSpcBumpDOL_Fwd` (1,038 lines) reads the lightmaps `t26`, `t27` (lines 481-482)
with the same dominant rows `cb9[12/13]` (lines 510-516) and `t33` (line 463). Native state of the forward rows: `t30/t31` bound to
a 1×1 black cube by policy 1114 (STUDY-NOT-YET-NATIVE row 1, OBSERVED), the light grid native since I-0107; their `t33` TO CHECK
(I-0113 RESULT tranche 16 observed `t33` and `t44` as 1×1 textures at the original's crystal draws, and natively SYSTEX_WHITE: so
the original's forward rows may not see the SSAO either; not checked for ordinary rows). Consequence: the forward rows' ambient is
the instance SH rows (or the lightmap) × a vertex AO and, maybe, nothing from SSAO natively.

## 3. What the baked data is worth

### 3.1 Lightmaps (family A)

- **Content** (OBSERVED, the programme): `t26.rgb` is gamma-squared radiance/irradiance scaled by a per-texture HDR constant
  (`cbTexHdrScaleParam`, the TPF entry's float metadata, `native/world/producers.h:141`, `13b_prepare_model_frame.inc:684`) and by the
  zone's `Dominant Light` GParam rows (`FC_DominantRgbScale`, `FC_DominantColorScale`, `FC_DominantSharpnesOffset`; per zone Colour
  scale 0.45 at m37, 0.5 at m30, 0.8 at m31, 1.0 at m45, 1.2 at m40 with RGB 0.6/0.8/1: WAVE1-LAST-EFFECTS-STATIC-PREP §1.3, STATIC
  from the decoded GParam); `t26.a` is GB1.z; `t27.rgb` is a dominant direction (x2 − 1, rotated by the instance's quaternion) and
  `t27.a` its sharpness; names like `40000000_m000063_gi_0000_00_dol_00`, with `_l` low-resolution and `_atlas000` packages
  (`08_model_programmes.inc:953-955`, `producers.h:141`). The UV: the mesh's lightmap element / 2048 × `atlasUvScaleOffset.xy` + `.zw`
  (GBUF-FAMILIES §0, R6-PLAN §1).
- **Format, resolution, per map**: TO CHECK. Nothing in the repository records a lightmap's DXGI format or size (the I-0110 census
  has the BTAB banks, 23 of them, « lightmap atlases », INFERRED as their per-map UV-rect tables; the textures sit in the maps'
  `.tpfbhd`/DCX archives, which I-0110 recovered only as headers). A cheap static follow-up: read the DDS headers of the `_gi_*_dol_*`
  entries of `m30_00` and `m40_00` (and the `_l` packages).
- **What it contains** (INFERRED, I-0110 RESULT §0 and §6, D-0237): the baked bounce, sky occlusion and baked soft shadows of the
  static scene lit by the zone's sun and sky; that is what R7 is meant to regenerate. Whether it includes the sun's direct term or
  only indirect is TO CHECK (it decides whether using it as a bounce source double-counts the traced sun).
- **Value to R7**: (a) a **fallback** for a frame or pixel the GI cannot serve: the raster's own path, free, kept by simply not zeroing
  GB4; (b) the cache's **seed**: R6a already reproduces GB4 from a hit's own data (`RtAmbientOf`, `rt_material.hlsli:336-342`) at
  90-100 % of pixels within 5 % of the raster's (RESULT « R6a step 1 »: m40 99.9 %, m30 90.8 %, m45 99.3 %, m31 96.2 %), so a hit
  can start with its baked ambient and move toward the traced value over frames; (c) the **look reference** for Thomas's judgement
  (I-0110 §0 point 2: R7 carries most of the look). Its direction (t27) is also a free hint for the final gather's importance
  sampling (INFERRED).

### 3.2 The instance SH (family B, and forward Ibl rows)

- **Source** (OBSERVED static, TONEMAP-SKIN §10): `aSh` is built by the engine's CPU code `FUN_1401AC840` from the object's animation
  block: a **baked map probe** (13 int16: L0 RGB and L1 y, z, x per channel, scaled by the SH basis constants 0.282095 and 0.488603,
  rotated by the map's yaw, `FUN_140003FB0`) or, without a record, the **light set's hemisphere**: `row_c = ((U − D)/2 × axis,
  (U + D)/2)` with U, D the zone GParam's upper and lower hemisphere colours (squared, times intensity) and the axis default (0, 1, 0)
  (`FUN_1401495F0`, the rows `+0x290..+0x2BF` of the set record). Blended over time (clock `+0x164/+0x168`); the whole path gated on
  `[GXRS+0x4DD0] != 0`.
- **Probe data**: `.btpb` files (BTPB, 22 banks, 23 MB, I-0110 §5.3; INFERRED link to `[map+0x40]+0x90`, not proved), organised as
  volumes with grids (AABB, cell size, cell counts) in the manager `[G+0x46E8]`; the query takes the **nearest** probe among the 27
  cells around the object, the lowest `+0x1A` short first, with no interpolation between probes (OBSERVED static). They ARE an
  irradiance volume (an L1 SH grid), though a coarse one with a hard pick (INFERRED from the format).
- **Native**: the engine's own producer runs natively and its rows reach the instance record (`05_selected_scene.inc:1065-1090`,
  `12_scene_collect.inc:1663-1678` `NativeInstanceLighting` → `ProduceInstanceRecord`, the rows copied into `RtInstanceRow.sh`).
  R6a measured the characters' families at m30/m31 only 30-60 % within 5 % (SkinSSSIbl_Bloody 32 % and 58 %; RESULT lines 42-48):
  the SH rows of moving objects move between the walk and the raster's record (guess, not checked).
- **Value to R7**: the world-space probe grid is a ready, engine-resident **cache seed** for ambient at any point (nearest-probe
  L1: rough but coarse-right), and the hemisphere rows (`FC_AmbientSH` in the `sets` buffer the reflect pass already binds,
  `rt_reflect.hlsl:89-90`: row 12 to 14) are the cheapest **sky miss fallback**: a GI ray that leaves the scene without meeting the
  forward sky returns `(U + D)/2 + (U − D)/2 × N.y` per channel (INFERRED from the rows' formula). R6d's `SkyRadiance`
  (`rt_reflect.hlsl:150-185`) is the better miss when the dome exists.
- **A bridge without DXBC changes**: R7 can **write the instance SH rows itself** (from the radiance cache projected to L1 at the
  object) and so feed every Ibl row, deferred and forward alike, through the existing constants (the same route the engine's probe
  takes). INFERRED, attractive for R9 (forward rows).

### 3.3 The env cubes (REM, IEM, regions)

- **Region cubes** (REM, radiance) are the specular ambient's source today (R6-PLAN §1) and R6b's miss: `EnvMap_%04d` names from the
  gparam EnvMap group (`GXEnvMapParamManager`), the prefix with a `GILM` suffix in one case (TONEMAP-SKIN §11, OBSERVED static;
  meaning of « GILM » INFERRED: a GI-lightmap-lit variant). Fallbacks: the global REM `[[GXRS+0x4DD0]+0x178]`, then a GX common
  texture. A cube seen natively: 128×128, 8 mips, R16G16B16A16F (RESULT-HISTORY Latest337, which cube not stated).
- **IEM** (irradiance): the global IEM cube `[GXRS+0x4DD0]+0x180`, from which `FUN_1400D9EE0` derives the block `[+0x190..+0x1E8]` that
  `cbInstanceData` reads, else the constant `1439F2500` (TONEMAP-SKIN §11, OBSERVED static). What reads that block, whether it is
  an SH of the IEM, and whether the deferred programmes use it: TO CHECK.
- **Diffuse region variants**: `GXEnvMapRegion_IemDiff[_Pcc]`, `_IvDiff[_Pcc]`, `_RemDiff[_Pcc]` exist as programmes
  (PROGRAM-FINGERPRINTS.md:337-343) but the native loads only `GXEnvMapRegion` and `_Pcc` (`06_pass_programmes.inc:133-134`,
  OBSERVED). Whether any original region draws a diffuse variant stays TO CHECK (R6-PLAN §1 already says so); if one does, it writes
  an irradiance term into `SpecularAcc`'s channel, which the lighting treats as specular: not seen.
- **Value to R7**: a low-mip REM sample is a usable far-field and miss radiance where the forward sky is absent (interiors, caves),
  and the region boxes give a coarse per-region radiance probe set the cache can compare against. The cubes are specular
  radiance, not irradiance: using them for diffuse needs a convolution (or the IEM, if present).

### 3.4 Summary of reuse

| Baked data | Keep as | How |
|---|---|---|
| GB4 as the raster draws it | fallback when GI is off or fails; the look reference; the control view's truth | do not zero GB4 on those frames |
| `RtAmbientOf` at a hit | the radiance cache's seed (first-frame value) | already written (R6a); needs a copy of the unzeroed frame rows if GB4 is zeroed at source |
| lightmap `t26.a` / `ShLightMask` | read as a sun mask only while the raster sun is used; superseded by the traced sun on traced frames | TO CHECK values (does it cover more than static shadows?) |
| light-set hemisphere rows | miss fallback and sky ambient floor | `sets` buffer rows 12-14 |
| `.btpb` probes | optional coarse irradiance volume for seeding | engine-resident, nearest-pick |
| REM cubes | far-field radiance for misses | `Regions()` in `rt_reflect.hlsl:82` |

## 4. What the native already traces and R7's insertion point

### 4.1 What exists (OBSERVED)

- **The rays' scene**: top level with masks `RtMaskView 1`, `Sun 2`, `Local 4`, `Forward 0x10`, `Water 0x20`
  (`rt_reflect.hlsl:12-14`, `rt_scene.h`); the material table (`RtMaterialRecord`, 224 bytes, `rt_material.hlsli:28`) and instance
  rows (`RtInstanceRow`, 368 bytes, `rt_scene.h:189-193`) holding the ambient rows (`atlas`, `rotate`, `sh[16]`, `lit` = SH-read
  flag, t26/t27 slots, light set), `hdr`, the forward mulCol and fog.
- **A hit's shading** (`HitRadiance`, `rt_reflect.hlsl:116-133`): `albedo × (RtAmbientOf + directional diffuse + local diffuse) +
  directional specular + local specular`, the sun by one ray (cone on RR frames), the local lights unshadowed (RT) or one chosen
  by RIS with its ray (PT); **no AO, no t32.z mask, no translucency, no emissive** (FC_EmissiveColor not read yet: final review M1),
  no specular ambient (one bounce), fog applied by the caller along the reflected leg (`TraceRadiance`, `:213-214`). Ambient: the
  baked form per R6a, `RT_AMBIENT_MISSING` where inputs are missing (counted, taken as a miss by the control view, taken as 0 by
  `RtAmbientOf`).
- **Passes already tracing**: the sun's shadow into the mask (`rt_shadow.hlsl`), R4's local lights (`rt_lights.hlsl`: it writes the
  accumulation pair, reads depth, GB0, GB1, GB3 and the AO slot), R6b's reflections (`trace`/`trace_hit`, into `SpecularAcc`,
  R11G11B10F, and RR's specular hit distance), R6c's water (`water`), R6d's sky. Their shared constants: `RtReflectConstants`
  (`rt_scene.h:601`, 240 bytes) with the camera, depth/GB0/GB1/GB3 slots, the sets and fogs buffers, the regions buffer.
- **Views available**: `rt_main_views_` has depth and GB0 to GB4 (`scene_pass.h:681`, `scene_pass_frame.cpp:1401` stores GB4's).
  GB2 and GB4 are not in `RtReflectConstants` today (GB4 only in the control view's constants, `rt_view.hlsl:19`).
- **RR's inputs**: `rr_guides.hlsl` writes the diffuse albedo (GB2 through the sRGB view, linear; sky 0.5), the specular albedo
  (`EnvBRDFApprox2` of F0 = GB3², α = r², N·V), the normal and roughness (r = (2/(2^(13 g) + 2))^¼, INFERRED equivalence), reading
  GB0..GB3 and depth (`rr_guides.hlsl:1-40`); the colour RR denoises is the **lit HDR copied right after the composite**
  (`scene_pass_lighting.cpp:441`, `:611-615`: `rr_targets_[3]`), plus the specular hit distance (`rr_targets_[4]`, R16F). RR is
  evaluated at `scene_pass_frame.cpp:915-917`. The diffuse albedo guide is GB2, so a GI term modulated by GB2 before the copy is
  consistent with RR's demodulation model (INFERRED from the guide, not re-read).

### 4.2 Insertion point (INFERRED design on the OBSERVED order; each choice with its reason)

**Where**: right after `pass(2,{2})` `GXLightCmbAlbedo` (`scene_pass_lighting.cpp:427`), before the SSS stage (`:428`) and the
composite (`:439`): a native full-screen (or half-size) pass, same shape as R4's `EncodeRtLights` and R6b, that

1. reads depth, GB0 (N), GB2 (albedo), GB3 (F0; `(1 − F0)` is what GB4 carried), the TLAS, the material table, the instance rows,
   the lights list and `sets`/`fogs`, and the radiance cache;
2. gathers the diffuse irradiance at the pixel (one cosine-weighted ray at the RT level, the cache at its hit: § below; ReSTIR GI at
   PT), shading hits with `HitRadiance` where `RtAmbientOf` becomes the cache lookup;
3. **adds** `GB2.rgb × (1 − F0) × E` (E in the units GB4 used, i.e. already without the 1/π the programmes never apply; GB4 holds
   an irradiance-like value and `CmbAlbedo` multiplies by albedo, so the scale of the baked values is the target's scale, TO CHECK
   by comparing E with GB4 on the control view) into `light_targets[2]` (R16G16B16A16F) with additive blending.

Why there and not elsewhere:
- Into `light_targets[0]` (the diffuse accumulation, R11G11B10F): unsigned small float, 6-bit mantissa for blue, banding on a noisy
  1 spp term; and `CmbAlbedo` would multiply the noisy irradiance by albedo anyway. `light_targets[2]` is R16F.
- Into GB4's own target: it is in PSR at that point (`:332-334`), R11G11B10F, and pass 0 would still multiply by AO and t32.y.
- After the SSS stage: skin would lose the SSS blur on its GI diffuse (the SSS reads `light_targets[2]`, `scene_pass_internal.h:122`).
- Before `rr_targets_[3]`'s copy (`:441`): the GI must be in the lit HDR RR denoises.

**What the pass makes consistent**: (a) GB4 zeroed at its source (`cbPerFrame +0xCC` = 0 and the SH rows 0) so pass 0's diffuse is 0
and the baked ambient is not counted twice; but `frame.dominant` of R6a's hits and `rt_view.hlsl` then need their own copy of the true
rows (`RtReflectConstants.dominant`, `RtViewConstants.dominant`: fed from the same producer, TO CHECK which). (b) SSAO off: t33 and
t32.x neutral (§2.2). (c) The sun's `t32.z` mask (GB1.z on clear pixels) stays on the raster sun path; on traced frames the traced
sun's mask has `w`, and `t32.z` still gates it (`DDirLightShadowed1`: `r5.y = t32.z`): if `t26.a` is not 1 on lightmapped surfaces
the traced sun is darkened by a baked mask: TO CHECK (read `GB1.z`'s distribution on the control view).

**Buffers written and who reads them**:

| Buffer | Format, extent | Written by | Read by |
|---|---|---|---|
| radiance cache (world-space probes or a hash grid) | to design (RT-PT-PLAN §6: « cache de radiance »; PT: ReSTIR GI + cache) | an update pass before the gather (its own rays; hits shaded as `HitRadiance` with a cache read for the bounce) | the gather pass, then R6b/R6c/R6d's hit ambient (`rt_reflect.hlsl:132`) so a reflection's hit gets traced ambient, then R9's forward rows (§2.3) |
| `light_targets[2]` | R16G16B16A16F, render extent (`formats[2]`) | `CmbAlbedo` (pass 2), then the GI add | SSS, composite (`t6`), RR via the lit HDR |
| GI irradiance (optional, only if a spatial filter or half-size upsample is wanted) | R16G16B16A16F, half or full extent | the gather pass | the add pass |
| RR guides | unchanged (GB2/GB3/GB0 based) | `EncodeRrGuides` | RR |
| t33 / t32.x | neutral | none (SSAO removed) | passes 1, 8, 9 (unchanged) |

**Cheaper alternatives noticed**: write E into GB4's target in the G-buffer phase (before the PSR transition), leaving passes 0 to
3 untouched, at the price of R11G11B10F precision and a pass in the G-buffer phase whose inputs (GB0, GB3) are being written
(a second G-buffer read dependency) — rejected above; and a fallback frame (no GI) simply skips the add pass and leaves GB4 and
SSAO alone, which is what makes the baked data a free fallback (§3.4).

### 4.3 Risks and open items the study met

1. **No RR on the RT level's « Off » option** (D-0247: RR is a player option): a noisy 1 spp diffuse term has no denoiser then;
   the radiance cache's smoothness is the only reducer (a design constraint for the gather, INFERRED).
2. **GB1.z / stencil 0x10** (§1.1): unknown which pixels carry the mask on the ambient; R7's add ignores `t32.y`.
3. **Emissive at hits** is not shaded yet (final review M1): bounce from lava, braziers and glowing meshes is absent until R3's
   emissive reading lands.
4. **Characters in motion**: R6a's hit ambient is 30-60 % within 5 % on SH families at m30/m31 (rows read at another time than the
   raster's), the seed would be off for them; the radiance cache replaces it, so this matters only for the seed.
5. **Lightmap format and the sun in the baked term** (§3.1): TO CHECK before trusting the baked value as a bounce source.
6. **Decals write GB4**: a decal's ambient contribution is lost if GB4 is zeroed and nothing replaces it: TO CHECK what the decal
   programmes write there.
7. **Forward rows** keep their own ambient (aSh, lightmap `t26/t27`, `t33`, `t30/t31`): they are not fed by the pass above; the
   zeroing at source also zeroes their baked ambient, so with R7 on and R9 not yet done the forward objects (fading characters,
   phantoms, FFX models, glass, translucent) would lose their ambient entirely unless the SH rows are rewritten from the cache or
   the forward rows are left on baked data (zero only the deferred path: the forward programmes read the same `cbPerFrame`
   words, so a per-pass flag is needed, TO CHECK how `cbPerFrame` is produced per programme, `13b_prepare_model_frame.inc:200-205`).

## 5. Statuses per question

| Q | Answer | Status |
|---|---|---|
| 1 | table §1.2; SSAO weights, t32 channels and their constants §1.1 | OBSERVED except: the Mult/Add order of the composite constants and its channel semantics (STATIC reflection, pairing INFERRED), the stencil 0x10 population (TO CHECK), the no-lightmap-no-SH `nolight` rows (census text, not re-read), decals' GB4 (TO CHECK) |
| 2 | order §2.1 with native lines; replace/feed list §2.2; forward rows §2.3 | OBSERVED (forward t33 binding TO CHECK) |
| 3 | lightmaps, SH probes, cubes §3 | OBSERVED for formulas and sources; lightmap format and size TO CHECK; IEM's consumers TO CHECK; probe grid as « irradiance volume » INFERRED |
| 4 | what exists §4.1; insertion §4.2 | exists: OBSERVED; the insertion and its ordering: INFERRED design (not built) |
