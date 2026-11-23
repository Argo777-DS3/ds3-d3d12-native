# R3, « Les matériaux pour les rayons » : the plan

2026-11-02, the RT pilot (RT-PT-PLAN §8 R3, after R5; D-0215). Statuses: OBSERVED (read in a programme's disassembly or the
native's code), INFERRED, TO CHECK.

## 0. In short

A ray that hits a surface off the G-buffer (a reflection, a bounce) needs that surface's material. DS3 writes it in the G-buffer
through about 676 G-buffer pixel programmes; the rays cannot run them. R3 gives every LOD-0 slice of the rays' instances a
material record (T3's alpha record is its seed) that reproduces, at the hit, what the slice's programme writes: albedo, F0, gloss,
shading normal, emissive, alpha. The check is numeric, as R2's T4 was: the camera's primary rays shaded by the records against
the G-buffer the raster wrote at the same pixels, by channel and programme family. R3 draws nothing new in play: R6 (reflections)
and R7 (GI) are its readers.

## 1. What DS3 writes (OBSERVED unless marked)

The reference family, `GXFlver_ColDifSpcBumpDOL_Gbuf.ppo` (the lightmapped map pieces, 97 % of m30's map meshes, I-0110 §6):

| Target | Content | From |
|---|---|---|
| GB0 (o0) | shading normal × 0.5 + 0.5 (world) | t3 `g_BumpmapTexture`.rg × 2 − 1 in the vertex frame: n = x B + y T + sqrt(1 − |xy|²) N, B = cross(N, T) × T.w, N flipped on a back face |
| GB1 (o1) | x gloss; y translucency; z the lightmap's alpha | x = t3.b × (FC_SpcParam.x / 256)², FC_SpcParam.x = the MTD's `g_SpecularPower`; y = COLOR1.y × FC_TranslucencyScale × 0.1 |
| GB2 (o2) | albedo (an sRGB target); w alpha | t0 `g_DiffuseTexture`.rgb × FC_DifMapMulCol.rgb (the MTD's `g_DiffuseMapColor`) |
| GB3 (o3) | sqrt(F0); w the light set | t1 `g_SpecularTexture`.rgb (sRGB-decoded, I-0113 WHEEL-STUDY) |
| GB4 (o4) | ambient (the lightmaps t26, t27) × (1 − F0) | baked light: not a material, left out (the traced light replaces it) |

Each texture tk reads the slice's UV0 (the vertex programme's TEXCOORD0 with VC_TexScroll) tiled by FC_TextureTilingScale[k].

**The gloss** (RT-PT-PLAN §2's « canal de brillance pas encore identifié »): GB1.x. The light programmes read it as a
normalised Blinn-Phong exponent (`GXLightAcc_DPointLight1.ppo`): n = 2^(13 × GB1.x), clamped by the light's cb1[1].w (its diffuse colour row's w)
(n' = max(1, n w / (0.01 n + w))), normalised by (n' + 2) / 8, times F0 = GB3²; the diffuse times (1 − F0 / 2). For a GGX reader
(the traced lighting, Ray Reconstruction's roughness guide) the usual equivalence α² = 2 / (n + 2) gives the perceptual
roughness r = α^½ = (2 / (2^(13 g) + 2))^¼ (INFERRED: an equivalence, not DS3's own; the sun's programme to confirm).

The sun's programme (`GXLightAcc_DDirLight1.ppo`, the census) has the same exponent and normalisation, a Schlick Fresnel
F0 + (1 − F0)(1 − V·H)^5 and the diffuse times (1 − F0); the point and spot lights none (F0 alone, the diffuse times
(1 − F0 / 2): RESULT « The Fresnel »): R4's and R6's concern, not the record's.

**The other families** ([GBUF-FAMILIES](GBUF-FAMILIES.md), the census of the 676 G-buffer programmes; 216 write the reference's
gloss):

| Family | Against the reference | Tranche |
|---|---|---|
| `EnvDOL`, `Ibl`, `TrIbl`, `Grass*DOL`, the `_Phantom`, `_Cloth`, `_Cm` twins | the same material (the ambient GB4, GB1.y, GB1.z differ: not material) | R3a |
| `Emi*` | + emissive in GB5: t6 `g_EmissiveTexture` × FC_EmissiveColor (cbMatDynParam, the material's GX03 item, +0x104) | R3a (the colour TO CHECK) |
| `BlendIbl` | the normal blended with a second bump map (t10) by the vertex colours COLOR2, COLOR3 | R3c |
| `Mul`, `MulMask`, `DirtMask` | two full layers (t7, t8, t10 at UV1) blended by COLOR1.w (MulMask: a mask t14) | R3c: `Mul{DOL,EnvDOL,Ibl}`, `MulMask{DOL,EnvDOL,Ibl}` done; the `_Bloody`, `_SnowLt`, `_Cloth`, Ghost, Emi, Pom variants and `DirtMask` left |
| `_Bloody`, `_Df` and the wet block (`Equip*`, `Hair*`, `Eye*`, `SAT_*`) | a blood decal layer (t41, t42 at TEXCOORD5) and a wet layer driven by cbMatDynParam; `_Df` a back-face set | R3c: `SAT_Equip` and `SAT_Equip_SSS` (and `_cloth`) by their rows, without the wet, blood and damage blocks (`SAT_Equip` measured, the others read); `SAT_HairIbl2` (and `_cloth`) by its rows with the hair's curve (fourth tranche); the blocks themselves left |
| `MultiBlend2/3/4`, `FourBlend` and the map blends | up to four layers by per-vertex weights and a mask (row 0: t9, t13 or t17); no FC_DifMapMulCol, gloss the raw bump b | R3c: the plain, `_e` and lightmapped bodies done; `_mt1`, `_p`, `_em`, unlit, BloodBlend, Sand left |
| `SnowLt`, `Pom`, the skin (`FC_SkinMatchColor`), `Col`, `ColDif` | snow by height, parallax, the skin's albedo curve, constant colours | R3c: the skin (`SkinIbl`, `SkinIbl_Bloody`, `SkinSSSIbl_Bloody`; the last measured) done; fourth tranche: the hair (`HairIbl` and its variants) and the face (`FgFaceIbl`, `FgFaceIbl_Bloody`, `FgFaceSSSIbl_Bloody`, the last measured on few pixels) done; `Col`, `ColDif`, `ColSh`, `EmiGlow`, `Veill_Curtain` and the eyes: no material until decoded; the rest left |

## 2. What the native already has (OBSERVED in code)

- Per slice, through the rays' walk (`NativeRtAlphaOf`, `12_scene_collect.inc`): the layout's programme set (interfaces with
  each programme's resources by register, the recipes, the policy, the authored MTD parameters), the texture bindings resolved
  as the raster resolves them (`NativeTextureHolders`, `ResolveNativeTextureHolders`), the tiling rows
  (`ProduceModelTextureTiling`), the vertex layout's elements; T3 reads t0, UV0, COLOR1 and g_AlphaRef from them.
- The table: `RtAlphaTable` encodes, each frame, one 80-byte record a slice of every instance with an alpha set into upload
  memory, with bindless views of the buffers and textures (`rt_alpha.hlsli`).
- The control view (`rt-view.flag`, `rt_view.hlsl`): the camera's primary rays against the raster depth, by class; the
  G-buffer is readable at that point (the pass runs right after the G-buffer). `gbuffer-pictures.flag` reads GB0..GB4 back.

## 3. Tranches

Each tranche behind `rt-scene.flag` (the rays) and, for its witness, `rt-view.flag`; unit tests; a step review; merged once
written and reviewed (D-0227), the start check after the merge (D-0229). Runs announced (WORKFLOW « Live game rule »).

**R3a, the record of every slice (the reference shape).** The table becomes a material table: every LOD-0 slice of every
instance has a record (an alpha-tested slice keeps its test in it). The record: the buffer and index data (T3's), UV0's element,
the vertex NORMAL and TANGENT elements and formats (a skinned instance: its triangle's own frame, the rays' positions having no
skinned normals; counted), the views of the slice's diffuse, specular, bump and emissive textures by their names in the G-buffer
programme's interface, each with its tiling row, FC_DifMapMulCol, the gloss scale (g_SpecularPower / 256)², the emissive colour,
the alpha data, and the programme family's index. The records are kept by set across frames in a GPU table (written when a set is
made or changes, freed when the set retires), the instances naming their first record (InstanceContributionToHitGroupIndex) and
a small per-frame row (the scroll) by instance index; the per-frame upload of T3 goes. The texture's level from a ray cone (the
primary ray's spread, the triangle's UV-to-world ratio), not the top mip (T3 keeps its top-mip test for the alpha). The shader
side: one include (`rt_material.hlsli`) that turns a hit into a material (albedo linear, F0, gloss and roughness, the shading
normal in the world, emissive), the only reader R6 and R7 will call.

**R3b, the material control view.** `rt-view.flag` adds, at the pixels whose traced depth agrees, the record's material against
the G-buffer at that pixel: albedo (GB2, its sRGB values) within a tolerance, F0 (GB3 squared), gloss (GB1.x), the normal (GB0)
by angle; counts per channel and per family (`native_rt_material_check` rows: the family's name, its pixels, each channel's
agreeing share and mean error), and pictures (the traced albedo, the traced normal, the differences). The exit: at Firelink, the
High Wall and m45, on the agreeing-depth pixels of the reference-shape families, a share within tolerance set from the first
measure; each family off it explained. The same runs read the rays' walk's skipped walks (`native_rt_walk` `skips`,
`skip_run_max`; I-0118's open point).

**R3c, the families off the shape** (from the census and R3b's rows, in order of their pixel share): the two-layer blends (their
blend factor), the multi-layer map pieces, the masks, the blood and decal layers of characters, emissive families, then the
rest; each one a record variant or listed as left (its share stated).

## 4. Risks and how each is caught

| Risk | Caught by |
|---|---|
| A family whose formula is not the reference's | the census (§1), R3b's per-family rows |
| Runtime material words differing from the MTD's (the numeric writes of the material object, D-0180's block plans) | R3b: a family off by a constant factor; then the record takes the raster's own b3 words |
| The mip: a ray query has no derivatives | the ray cone; R3b's tolerance read on the mean, not on single texels |
| The vertex frame's sign conventions (T.w, the back face) | R3b's normal angle on two-sided slices |
| The per-frame cost (the CPU is the engine's bottleneck, I-0095) | records kept across frames; the walk's and the table's times in `native_rt_walk` |
| Memory of the views (the bindless heap) | the table's counters (views alive, refused), as T3's |

## 5. Not in R3

The lights (R4), the traced reflections (R6) and GI (R7) that read the records, Ray Reconstruction's guides, the transparent and
forward rows' materials (R9), the lightmaps (baked light).
