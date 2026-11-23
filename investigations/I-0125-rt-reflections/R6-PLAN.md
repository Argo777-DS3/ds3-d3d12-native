# R6, « Les reflets tracés » : the plan

2026-11-04, the R pilot (RT-PT-PLAN §8 R6; after I-0122 R4c). Statuses: OBSERVED (read in the DXBC disassembled into
`evidence/I-0125/artifacts/dxbc` from the I-0063 cache, the I-0119 census's disassemblies, or the native's code), INFERRED, TO
CHECK. The rules it follows: D-0237 (a traced picture different from the original is not a defect in itself; a datum misread is),
D-0247 (the fill lights and the baked ambient kept until R7), D-0197 (no stand-in), the RT and PT levels of RT-PT-PLAN §6.

## 0. In short

DS3 reflects through its region cubes: each region box blends its cube's sample along the mirror direction into the specular
accumulation, which the deferred AO pass adds to the lit specular. R6 puts a traced ray's radiance where the cube's sample was,
keeps DS3's own environment term around it, keeps the cube for a ray that meets nothing, and later does the same for the water's
mirror view. The hard part is not the ray but its hit: lighting a surface no G-buffer saw with DS3's own data. That comes first
(R6a), checked on primary rays against the raster's lit picture, the way R3b checked the materials.

## 1. What DS3 does (OBSERVED)

**The cubes.** `GXEnvMapRegion` (pass 4, one draw a selected region box over the G-buffer pixels it holds): P from the depth, its
box coordinates b = |g_envMapInvWorld · P|, the fade f = min over the axes of (1 − b) × g_envMapDrawAreaExtents (outside: nothing),
w = smoothstep(f / g_envMapFadeDistance); N = normalize(GB0 × 2 − 1) and V (toward the eye) in the world, R = 2 (N·V) N − V; g =
GB1.x, F0 = GB3²; the colour

  cube(R, min(6.5 − 6.5 g, 6)) × (F0 + (1 − F0)(1 − sat(N·V))⁵ g²) × g_envMapColorScale × min(N·V + 1, 1)²

blended with alpha w × g_envMapFadeRate (source alpha over its inverse) into `SpecularAcc` (R11G11B10, cleared to black: no region,
no specular ambient). `_Pcc` takes the cube's direction from R's exit of the region's parallax box (g_envMapMatInvPccObb, the
origin g_envMapRegionPccOrigin), and gives nothing where the exit is not on the box. The `IemDiff`, `IvDiff`, `RemDiff` variants
exist (a diffuse from the cube, TO CHECK whether any region draws them).

**The specular ambient.** `GXLightDeferredAO`: diffuse = GB4 × t32.x t32.y, specular = `SpecularAcc` × t32.x t32.y (the AO and
mask); the sun's pass and the local lights add to both; `GXLightCmbAlbedo` multiplies the diffuse by GB2; `GXLightComposite` adds
the specular and GB5 (the emissive), then the fog of the pixel's set (`cbFog`).

**A surface's ambient (GB4), as the G-buffer programmes write it** (GBUF-FAMILIES §0; the reference `ColDifSpcBumpDOL_Gbuf.ppo`):
- the lightmapped families (DOL): L = t26.rgb² × cbTexHdrScaleParam.rgb × FC_DominantRgbScale.rgb (cbPerFrame +0xD0) ×
  FC_DominantColorScale (+0xCC); D the dominant direction t27.rgb × 2 − 1, normalised, rotated by the instance's
  vDominantRotateParam (a quaternion, cbInstanceData +0xC0); s = sat(t27.a + FC_DominantSharpnesOffset (+0xC4));
  GB4 = (1 − F0) × L × (s (1 − F0) sat(D·N) + 1 − s); the lightmap UV TEXCOORD0.zw / 2048 × atlasUvScaleOffset.xy + .zw (the
  `.vpo`; the two-layer Mul, MulMask and EmiMul bodies from TEXCOORD1.xy; the Burn, DirtMask and SandBlend bodies from other
  interpolants, not read: R6a step 1's review, H1 and H2);
- the Ibl families: GB4 = (1 − F0) × max(0, (aSh0·(N,1), aSh1·(N,1), aSh2·(N,1))), the instance's SH rows (+0x80).

**The light set's directional lights** (`GXLightAcc_DDirLightShadowed1`; the set GB3.w × 255, 18 rows a set in `cbLight`): up to four
(Vec_0 to Vec_3 with their Col and Spc), the first with the shadow (t32.w through FC_ShadowColor: lerp(1 − ShadowColor, 1, s)),
the second unshadowed, the third and fourth wrapped ((N·L / 2 + 1 / 2)², min 1) when bLocalLight.x, .y; Schlick on (1 − F0), the
programmes' Blinn-Phong lobe; the SSAO weights FC_SsaoWeight per term. The light sets 1 to 3 are DS3's fill lights (D-0247).

**The water.** Its forward programmes (`GXFlver_Water*_Fwd`) sample `g_ReflectionTexture` (t54) at the pixel's screen position,
x shifted by its bump and y flipped (1 − y): the mirror view's picture. The native draws that view (the reflection walk,
`native_reflection_preparing`), without the sky in the Iudex Gundyr arena (I-0113 gap 18).

## 2. Elden Ring (D-0195, D-0236) and the frame challenge

ER's ray tracing (1.09) traces the sun's shadows and the AO only (ER-PIPELINE-STUDY §8); its reflections stay the region cubes
and its SSR (DS3 has `GXSsr` too, never drawn: COVERAGE-MATRIX row 26). There is no traced reflection to copy; the place in the
engine is DS3's own (the specular accumulation between the regions and the deferred AO pass). What the native does better than ER:
no screen-space holes (SSR's), reflections of what the cube never held (characters, moving objects, shadows), the sky where the
mirror view loses it.

Sought: a ray's radiance at the cube sample's place. Engine invariant: the environment term, the region weights' meaning where the
ray misses, every input of the hit's lighting (the formulas above). Redesigned: where the specular ambient's radiance comes from.
Smaller experiment first: the hit's lighting on primary rays, where the raster gives the expected answer at every pixel (R6a).

## 3. The tranches

| Tranche | What | Seen |
|---|---|---|
| R6a | `rt_hit.hlsli`: a hit's radiance (emissive + albedo × (its ambient + its light set's directional lights, the sun with a traced ray + the local lights of the list) + the lights' specular × F0); the material table's lightmap roles (t26, t27) and the rows' per-instance data (atlasUvScaleOffset, vDominantRotateParam, the SH rows, the light set); the control view's radiance comparison | the control view's per-family rows: the hit radiance against the raster's lit picture without its specular ambient, at the agreeing pixels; luminance and FLIP |
| R6b | the reflection pass (`rt_reflect.hlsl`) writing `SpecularAcc` in place of the region draws on traced frames: the ray's radiance × DS3's environment term × the horizon term; a ray that misses takes the region cubes' sample (their weights at the pixel, `_Pcc` corrected); RT level: the surfaces with r below a cut, the mirror ray (RR off) or one GGX sample (RR on); PT level: every surface; RR's specular hit distance | the four zones, RT and PT, RR on and off, against the cubes' pictures, measured |
| R6c | the water: `g_ReflectionTexture` made by rays (the mirror view's rays from the water's plane, lit as R6a) in place of the mirror view's draws | Iudex Gundyr's arena (the sky in the water), the High Wall's and the Undead Settlement's water |
| R6d | the sky in the rays (§7, merged `75eb6c5e`): a miss composes the sky's unlit forward pieces, every hit fogged along the reflected leg | Iudex Gundyr's arena |
| R6e | R6d's checks and its final review's Lows (RESULT « R6e ») | |
| R6f | Thomas's acceptance (D-0251, 2026-11-06): the mirror view's walk and draws removed (RESULT « R6f »), the cubes kept for the misses that meet no sky; a collect the rays cannot serve gives the water the black default | the start check |

## 4. R6a in detail

**The hit.** `RtMaterialAt` (R3) gives albedo, F0, gloss, normal, emissive. Added per record: the lightmap roles t26 and t27 (their
views, as the raster binds them: 5B110, P[3] and P[4] of the draw's lightmap set) and the lightmap UV element (TEXCOORD0.zw, the
same element as UV0, its second half); per instance (the rows grow to 256 bytes): atlasUvScaleOffset, vDominantRotateParam, the
three SH rows and the light set index (envData), from the same producers as the raster's cbInstanceData (5B3D0's equivalent).
An instance the raster does not draw this frame (behind the camera, a ray-only instance) has no producer run: its rows from the
last frame that drew it, else none (TO CHECK how many hits that leaves without an ambient; the census of R6a's control view counts
them). The per-frame rows: `cbLight` (the sets), cbPerFrame's dominant rows, the sun's direction; the pass binds them.

**The radiance.** L = emissive + albedo × (GB4' + Σ directional diffuse) + Σ directional specular + local lights (diffuse ×
albedo, specular), with DS3's programmes' formulas (`rt_lights.hlsli` for the local lights; the DDirLight terms transcribed into
`rt_hit.hlsli`); the sun's visibility one ray toward it (the cone on RR frames); AO 1 (no SSAO off screen; the SSAO weights then
give 1); the local lights unshadowed at the RT level (one ray a light is too dear at a hit), at the PT level one light chosen by RIS
over the list's lights in reach and its ray (R4c's estimator). No fog (TO CHECK: the composite's fog on the reflected path), no
specular ambient at the hit (one bounce; R8 adds more).

**The local lights' reach.** The list holds every enabled light of the main view's category (R4-PLAN §1: no frustum cull), but its
tiles only the camera's. At a hit the pass walks the whole list with each light's reach test (a sphere); a world-space grid is
written only if the measure shows the walk costs (TO CHECK the lists' sizes at the four zones in R6a's run).

**The check.** The control view (`rt-view.flag`) gains a radiance picture: at each pixel where the primary ray agrees with the
raster depth (R2's classes), the hit radiance against the raster's lit picture before its specular ambient and fog (the composite
target without `SpecularAcc`: a check frame draws the composite twice, as R4a's check drew the batches), by programme family:
mean luminance ratio, the pixels within 5 %, FLIP on the two pictures. The R3b rows' families with a material and a lightmap first.

## 5. Risks

- The lightmap views: a texture still at its stand-in (SYSTEX_DUMMY, white) gives a white ambient: the record keeps no view until
  the texture is real (R3's rule for its textures).
- The light sets: an instance's envData indexes the frame's `cbLight`; a set index past the frame's sets reads set 0 (the raster's
  guard: TO CHECK).
- Cost: one ray a pixel with a full material read at the hit, plus the sun's ray; the RT level's cut and half size if the measure
  asks (RT-PT-PLAN §6: half size at the RT level).
- The reflections will look different from the cubes (D-0237): sharper, darker where the cube was brighter than the scene. The
  regions' colour scale is a cube's exposure and does not apply to a traced radiance; a miss keeps it.

## 6. R6c in detail (2026-11-04, the R pilot's fresh thread)

**What the water reads (OBSERVED, `WaterBumpTriInteractionFloatObj_Fwd_ppo.asm` lines 564-569).** t54 at u = SV_Position.x /
width + the bump's x × cb2[20].z × 10 / (v8.w + 10), v = 1 − SV_Position.y / height (cb8[1].zw the reciprocal extent), rgb only.
So t54's texel (u, v) holds what the water at the screen point (u, 1 − v) reflects, whatever t54's extent.

**The pass** (`rt_reflect.hlsl`'s second programme `water`, the same root and constants, one full-screen triangle into the
reflection target, RGBA16F at half the output extent as the mirror view's): for each texel, the main camera's ray through the screen
point (u, 1 − v) without the jitter (the mirror view never jitters), up to the water's own surface there: its slices are in the rays
(forward-only pieces), now with a ray bit of their own (`RtMaskWater`, 0x20: a slice of a g_GXFT_PlanarReflection material, on an
instance the eye sees). Where no water slice is met, WT-6's plane (the nearest water mesh's box centre, n = +Y) when the eye is
above it. From that point P the mirrored direction R = D − 2 (n·D) n (n = +Y, the mirror view's; the water's bump moves the lookup
itself) is traced with the view's mask (the surfaces, as R6b's rays: a forward-only slice, the sky's dome or a cloud, never stops it);
a hit lit as R6b's (`HitRadiance`); a miss, or a hit without its records or material, the region cubes at P along R at their
sharpest level (g = 1), their blend divided by their coverage (the cubes' radiance there, not a specular term); no region at P:
black, as the mirror view's clear. A texel whose ray goes up (its mirror would go down), or meets neither water nor the plane ahead
of the eye: black. No fog, as the hits of R6b.

**Why the water's own surface first.** WT-6's plane is the nearest water piece's box centre. The one piece measured so far is flat
(the new row's box y span: Firelink's `m40_00_00_00_000011` from −59.77 to −59.77, the eye 4.9 m above it, `i125-r6c1-gundyr-on`),
so the plane is exact for that piece (other pieces not measured yet); the earlier reading of a plane over the eye compared it with the mirrored eye
(`native_reflection_view`'s eye is the mirror view's), a mistake. A frame with two pieces at different heights still gets one plane;
the ray's own water hit holds for each piece.

**Its place.** In the main frame's lighting, after R6b's pass (its regions' buffer and cube slots, the frame's light list, the top
level, the sets reused), before the forward rows that sample t54. The collect, when the rays and the pass are on and the rays'
walk made a scene, finds the plane and makes the target as before but walks no mirrored world and publishes no mirror scene: the
frame job draws no mirror view (R6f removed that code, D-0251: the rays off, the water takes the black default). The target is marked written once the renderer recorded the pass into it, as the mirror view's draw marks
it; until then the water binds the black default, as before.

**RR's specular hit distance** (the R6b step review's M6). On the frames RR denoises, R6b's pass writes a fifth guide target
(R16_FLOAT, the render extent), tagged `kBufferTypeSpecularHitDistance` (the RR guide §4.1.9: the world distance from the primary
surface to the reflection's hit, with the world-to-view rows the options already give): a traced hit its distance from the surface;
a miss and the cubes along R 10 km (the cubes' environment at infinity, INFERRED as RR's reading); cleared to 0 every RR frame (no
specular ambient: the pixels outside the stencil, no region). The water's own pixels keep what lies under them (the forward rows
write no guide): recorded, not done.

**Seen.** Iudex Gundyr's arena (the sky in the water: the witness), the High Wall's and the Undead Settlement's water; traced
against `no-rt-water.flag` while it existed (removed by R6f: now against an earlier traced run), RR on; mean luma by signalstats and FLIP, the captures shown to Thomas.

## 7. R6d step 1: the sky in the rays (2026-11-06, the R pilot's fresh thread)

**What the dome draws (OBSERVED, `evidence/I-0125/artifacts/dxbc/Col_Fwd.ppo.asm` and `.vpo.asm`, `fxc /dumpbin` of the I-0063
cache's `GXFlver_Col_Fwd`).** Iudex Gundyr's dome `m40_00_00_00_009000` is a forward piece of the `GXFlver_Col` family (I-0117
RESULT « The sky out of the rays »; WAVE1-LAST-EFFECTS §2: class 0x80 at the 0x1F row, the clouds class 2 at the 0x29 row). Its
pixel programme is unlit: c = g_DiffuseTexture(UV) × (1, 1, 1, COLOR1.w), c.rgb × FC_DynamicDiffuseColor.rgb (cbMatDynParam
+0x180), c × mulCol (cbInstanceData +0x70; the vertex programme passes instance 0); UV = UV0 / 2048 × VC_TexScroll_0.xy + .zw, ×
FC_TextureTilingScale[0].xy + .zw. Then its fog, with C the camera (FC_MatrixInvView's translation) and P the pixel's world point:
the distance fog f = sat(E.x − E.x exp(−(|C.xz − P.xz| − E.y) E.z)) toward FC_FogCol.rgb × (1 + FC_FogCol.w (ℓ − 1)), then the
height fog h = sat(sat((P.y − C.y + H.w − H.y) H.z) H.x) toward FC_HeightFogCol.rgb × (1 + FC_HeightFogCol.w (ℓ − 1)), ℓ the
unfogged colour's luminance (0.29891, 0.58661, 0.11448), E FC_ExtFogParam, H FC_HeightFogParam (cbFog, the model's own forward
entry); alpha c.a. The blend is the forward row's (NativeOrdinaryForwardBlend: the group's kind and row, else the MTD's mode).

**The rays.** A slice with no G-buffer programme whose forward programme is this body (`GXFlver_Col`, its Fwd pixel programme
sampling g_DiffuseTexture and reading FC_DynamicDiffuseColor, no cbLight) gets a forward record (`RtRecordForward`): its diffuse,
COLOR1, FC_DynamicDiffuseColor (computed by the admission's own code, factored out), and its blend as two factors (source ONE,
SRC_ALPHA or ZERO; destination ZERO, ONE or INV_SRC_ALPHA; ADD; another blend: no record, counted). Its instance gains mulCol and
the model's forward fog entry (R6e: in a forward block of its draw group, apart from the rows) (`ProduceNativeForwardLight`, after the frame's light blocks; a model the raster drew reuses its own).
A reflection ray that meets no surface (R6b's and R6c's `TraceRadiance`) traces the forward instances (`RtMaskForward`, every
candidate non-opaque) and composes what it meets as the rows do: the nearest opaque layer, then the blended layers in front of it
from far to near, each fogged as its programme fogs it along the reflected leg (the distance fog from the reflecting point, the height
fog at the point's height, as the programmes' H.w, the camera's y, makes it). No opaque layer met: the cubes, as before. RR's hit
distance of a sky: 10 km (the cubes' infinity). Step 2 (the witness showed the dome's layers without an opaque one: STUDY-SKY-PROGRAMMES
§1): the scrolling zenith `MultiBlend2_et1_nolight_MuitiBlendMask_Fwd` (opaque) and the cloud sheets `Fujir_Nolight_OpacityBlend_Fwd`
(alpha) transcribed the same way (their textures by the rows they read, their UVs, the generated words from 0x270, FC_GlobalTime), and
every reflection hit fogged along the reflected leg with GXLightComposite's fog (the same formula, the hit's FC_FogParamArray set). Not
in R6d: the lit cumulus (`GXFlver_ColDif_Fwd`, lit by its group's lights: STUDY-SKY-PROGRAMMES §5), a forward piece in front of a
surface (the rays stop at the surface), a skinned or wind-moved forward piece (no fog entry: passed over), the G-buffer-class skies
drawn without a depth write.

**Seen.** Iudex Gundyr's arena (the sky in the water: R6c's witness), traced against `no-rt-water.flag` (R6c; the flag removed by R6f); then the study's other
waters. After Thomas's acceptance (D-0251): the mirror view's walk and draws removed (§3 R6f).
