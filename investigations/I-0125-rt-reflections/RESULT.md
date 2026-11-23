# I-0125 result: Les reflets tracés (R6)

## Conclusion

Complete (closed 2026-11-09 with Thomas's yes, « Closed » below); its history: Active since 2026-11-04 05:35 (Paris), the R pilot, from `main` `aca1e3e2`. The plan: [R6-PLAN](R6-PLAN.md). Merged: R6a and R6b
`444bb245` (the traced reflections in the specular ambient), R6c `cb65563e` (the water's t54 traced from its own surface, RR's
specular hit distance) and its follow-up `7f45a49e`, R6d `75eb6c5e` (the sky in the rays), R6e `ea3947ba` (R6d's checks, its
review's Lows), each with its start check passed. R6f (below): the water's mirror view removed (D-0251) and the rays the module's
default (D-0253), with the full control of 2026-11-06. Open: the region cubes' draws on the frames without the lights' pass
(D-0251), the High Wall's and the Undead Settlement's water (R11), the lit cumulus, a size off 16:9, the GPU cost.

## R6a step 1: a hit's ambient (2026-11-04, `16546f47`, its review's fixes `ffb4aaa98`)

- **Written.** `RtMaterialAt` (`rt_material.hlsli`) reads the ambient each G-buffer programme writes into GB4 (R6-PLAN §1), and
  `RtAmbientOf` finishes it with the frame's dominant rows. The lightmapped bodies (t26 and t27 declared) use the instance's
  lightmaps and atlasUvScaleOffset, the vDominantRotateParam quaternion transcribed instruction by instruction, and the frame's
  FC_Dominant* rows. Every other body uses the instance's SH rows. The instance rows (`RtInstanceRow`, now 256 bytes) come from
  the producer that makes the raster's cbInstanceData: `NativeInstanceAnimationRows` and `NativeInstanceLighting` were factored
  out of `ProduceNativeSelectedInstances`, then `ProduceInstanceRecord`. The lightmaps are read per placement from the
  descriptor's lightmap set, as `ApplyNativeLightmapSet` binds them, and kept by descriptor between walks. The records are now 208
  bytes, with the lightmap UV's element. The control view's family rows compare the hit's ambient with GB4, word 14 onward.
- **Step review** (fresh Opus, high). It found two High transcription errors in the lightmap UV: the atlas scale `.xy` was left
  out (H1), and the two-layer bodies sample TEXCOORD1.xy, not TEXCOORD0.zw (H2; the Burn, DirtMask and SandBlend bodies sample
  other interpolants and get no form). It also found three issues it ranked lower: the lightmaps were cached per model but are
  bound per placement (M1), the Ibl bodies were detected by name (L2), and the signed sum was not clamped (L3). The fixes are in
  the second commit. Fine as reviewed: the quaternion, the factors, the raster refactor (no behaviour change), the layouts, the
  view counts, the barriers. The raster samples t26 and t27 with s5, the material bank's wrap sampler, as the rays do (L6).
- **Measured** (control view, `Run-R6aCheck.ps1`, the four zones' Shebang saves, 60 s each, the ASUS; the share of compared
  pixels whose ambient luminance is within 5 % of GB4's). Runs `i125-r6a1-*` (before the fixes) and `i125-r6a2-*` (after):

  | Zone | Pixels compared | Within 5 %, before | Within 5 %, after |
  |---|---|---|---|
  | m40 Firelink Shrine | 11.2 M | 99.6 % (ColDifSpcBumpDOL) | 100.0 % |
  | m30 High Wall of Lothric | 8.5 M | 75.7 % (ColDifSpcBumpDOL; MulMaskDOL 4 %) | 90.3 % (ColDifSpcBumpDOL 97.8 %, MulMaskDOL 97.0 %) |
  | m45 | 1.8 M | n/a | 99.6 % |
  | m31 Undead Settlement | 11.1 M | 89.9 % (ColDifSpcBumpDOL) | 95.0 % (ColDifSpcBumpDOL 99.5 %) |

  These runs left out the lightmapped map blends (MultiBlend*_lightmap, FourBlend: their lightmap UV a float TEXCOORD1, refused by
  the format test, and not counted). The final review found it (H1, below). With its fix (runs `i125-r6b3-on-*`), every checked
  pixel is compared: m40 16.5 M at 99.9 % (MultiBlend2_et1_lightmap 99.7 %), m30 10.9 M at 90.8 % (MultiBlend3_et1_lightmap
  94.8 %, MultiBlend2_et1_lightmap 86.7 %), m45 1.8 M at 99.3 %, m31 15.1 M at 96.2 % (MultiBlend2_et1_lightmap 99.3 %). None is
  missing its inputs. Still off after the fixes: the characters' families at m30 and m31 (SkinSSSIbl_Bloody
  32 % and 58 %, SAT_Equip 30 % and 59 %, the bias about +0.07 log2); they are at 100 % at Firelink. My guess, not checked: their SH
  rows move with their position, and the walk reads them at a different time from the raster's record. Also MultiBlend2_et1 at m31
  (16 %, mean error 1.0 log2; its programme writes the SH form, so the form is right), ColDifSpcBumpEmiIblGlow at m31 (34 %), and at
  m30 ColDifSpcBumpIbl (79 %, its `|128` variant 59 %), ColDifSpcBumpTrIbl `|128` (44 %) and ColDifSpcBumpIbl_Bloody_Cloth (13 %);
  at m31 ColDifSpcBumpTrIbl_Bloody_Cloth (66 %). The bodies whose lightmap element is not read (Burn, DirtMask, SandBlend, the
  other blends) are counted missing; none met at these zones.
- **Not run: R6-PLAN §4's radiance check** (the whole hit radiance against the raster's lit picture). R6b's directional and local
  terms at hits are transcribed and reviewed against the DXBC, but not measured on primary rays: deferred, named here (the final
  review's M4).
- **Conclusion.** H2 of the CONTRACT is SUPPORTED for the map pieces' ambient. Their lightmaps and SH rows give the raster's GB4 from
  the rays' own data. The rows of characters in motion and two families stay open.

## R6b: the specular ambient traced (2026-11-04, `a174604d`, its review's fixes `8d6197e2`)

- **Written.** `rt_reflect.hlsl` (on with the rays; `no-rt-reflect.flag` gives the cubes) replaces the region draws with one
  full-screen pass. It uses the same stencil test (bit 0x08) and writes SpecularAcc. The regions are walked in list order, and
  their blend (source alpha over its inverse) is redone in the shader, `_Pcc`'s correction included.
  - A traced ray's radiance takes every region's cube sample's place, so the result is L × E × h × the regions' coverage
    1 − Π(1 − w).
  - A miss, or a hit on an instance without records, takes the cubes along the ray.
  - The RT level's surfaces with r ≥ the cut take the original's cubes along R. The cut is 0.3 for the mirror ray without RR and
    0.6 for one GGX sample of the lobe on RR frames. The PT level traces every surface.
  - The hit is lit by its material and its ambient (R6a), its light set's directional lights (`GXLightAcc_DDirLightShadowed1`
    transcribed) and the frame's local lights. The sun's mask is lerp(1 − FC_ShadowColor, 1, s), s from one ray along the traced
    shadow's direction. The local lights are unshadowed at the RT level; at the PT level one is drawn by RIS and gets its ray.
  - Left out: AO, the mask's z, translucency, fog, and the specular ambient at the hit (one bounce).
- **Step review** (fresh Opus, high). It found no High finding. It confirmed against the DXBC: the region blend, the `_Pcc` slab
  test, E and the horizon term, the DDirLight terms, the root signature and the layouts, the states and the stencil, and the
  failure fallback. It also showed that dropping the region boxes' depth test and culling changes no covered pixel. Fixed in
  `8d6197e2`:
  - M1, the light list's stale upload (one frame old, its count mismatched).
  - M2, the hits' sun shadow along the set's Vec_0 instead of the frame's traced sun.
  - M3, the rays registering light-set keys the raster never registers (`NativeLightSetFound`, lookup only; an unregistered key
    takes set 0).
  - M4, a grey stand-in at a hit without records (now a miss).
  - L2 to L6.

  Not fixed, recorded:
  - M5, an older gap: the `_Pcc` regions' three parallax blocks are never produced (`07_scene_lighting.inc`), so both the raster
    and this pass sample those regions along a zero vector.
  - M6: RR's specular hit distance is not produced yet (planned with R6b, deferred to R6c).
  - L1: untraced pixels are not bit-exact. The draws round to R11G11B10 after each region; the pass rounds once.
  - L7: with DS3's region programmes absent, the pass still draws the cubes.
- **Seen and measured** (runs `i125-r6b1-on-*` and `-off-*`: Firelink, the High Wall and the Undead Settlement, the traced pass
  then the cubes, RR on). The pictures are the game window's 1280x720, cropped from the desktop captures:
  `evidence/I-0125/artifacts/pictures-r6b1`.
  - Mean luma (ffmpeg signalstats YAVG), traced against cubes: Firelink 58.4 against 58.3 and 57.2 against 57.0; High Wall 60.6
    against 61.7 and 77.7 against 78.2; Undead Settlement 51.7 against 52.6 and 42.5 against 43.7. That is at most 1.2 steps of
    255, slightly darker at the High Wall and the Undead Settlement.
  - FLIP means 0.04 to 0.08. They include the smoke and the sky's own motion between the runs; the amplified difference shows
    mostly those.
  - After the review's fixes (runs `i125-r6b2-rt-m30`, `-pt-m30`, `-norr-m30`: the RT level with RR, the PT level, the RT
    level without RR) the pass ran on every traced frame (975, 967 and 975 frames at the 1,200th collect), no failure, no fault.
  - DS3's map surfaces are mostly rough: E carries g², so their specular ambient is small, and the visible witness of R6 is the
    water (R6c).

## The tranche's final review (2026-11-04, fresh Opus, xhigh, `main...65ff4825`) and its fixes

- **H1**: the lightmapped map blends had no ambient and were not counted. Fixed in `9a47af48`:
  - their element is TEXCOORD1.xy as two floats / 2048, both of ApplyNativeLightmapSet's forms are read;
  - a body that declares the lightmaps but whose element is not read is counted missing;
  - the fix's step review narrowed the element to the census's blends, in `6d0f0cca`.
  - that commit's own step review found no High or Medium finding. MultiBlend3_et1_lightmap is matched by family without its `.vpo`
    read; its measured 94.8 % (m30) and 97.2 % (m31) within 5 % support the element.
- **H2**: a hit on a record without its material was drawn grey. It is now a miss.
- **M1**: the hits' emissive multiplied by an unread colour. It is left out until FC_EmissiveColor is read.
- **M3**: the frames without the local lights' pass drew the cubes uncounted. They are now counted as `missed` in the
  `native_rt_reflect` row; 0 in the runs.
- **L1 to L8**: fixed or recorded.
- **Not fixed, recorded:**
  - M2: a ray-only instance whose light-set key the raster never registered takes set 0, another area's directional lights;
    not counted.
  - M4: the radiance check above.
  - M5: R6b's measures: the post-fix runs `i125-r6b3-*`, three zones RT with RR against the cubes, and the High Wall at the PT level
    and without RR. The plan's four zones × RT and PT × RR on and off are not all run, and the pass's GPU cost is not measured.
  - M6: this RESULT corrected.
- **The re-measure** (runs `i125-r6b3-on-*`, `-off-*`, `-pt-m30`, `-norr-m30`; build `build-r6b3`; the crops in
  `evidence/I-0125/artifacts/pictures-r6b3`, the side-by-side `r6b-cubes-vs-traced.png`):
  - No failure, no fault, no missed frame. The saves were restored by hash.
  - Mean luma (YAVG), traced against cubes: m40 58.4 against 58.3; m30 61.3 against 61.3 and 77.0 against 78.0; m31 51.7 against
    52.6 and 42.5 against 43.7.
  - m30 at the PT level 62.0 and without RR 62.2 (other runs, the view not identical).
  - FLIP 0.04 to 0.09, the motion included. m40 walk-2's 0.47 is a different view between the runs, not compared.

## R6c: the water traced (2026-11-04, the R pilot's fresh thread, branch `claude/zealous-goldstine-ea74fd`)

- **Written** (R6-PLAN §6). `rt_reflect.hlsl`'s `water` programme fills the water's g_ReflectionTexture (t54), one ray a texel, in
  the main frame after R6b's pass:
  - the texel (u, v) takes the main camera's ray through the screen point (u, 1 − v), as the water's programme samples t54
    (OBSERVED in its DXBC);
  - that ray stops at the water's own surface: the water slices now carry a ray bit of their own (`RtMaskWater`). Where no water is
    met, WT-6's plane is used, if the eye is above it;
  - from there the mirrored ray is traced and lit as R6b's hits. A miss takes the region cubes there, at their sharpest level,
    divided by their coverage.
- **The collect.** With the rays and `renderer.rt_water` on, it walks no mirrored world: it keeps the plane and the target only, and
  the frame job draws no mirror view. `no-rt-water.flag` gives the mirror view back. The target is marked written once the pass has
  been recorded into it. A target made again (a new extent) is now marked unwritten until its first picture (before, it kept the
  former target's mark).
- **RR's specular hit distance** (R6b's M6). It is a fifth guide target (R16_FLOAT), written by `trace_hit` on the frames RR
  denoises and tagged `kBufferTypeSpecularHitDistance`:
  - a traced hit: its distance from the surface;
  - a miss and the cubes along R: 10 km;
  - no specular ambient: 0.
  The water's own pixels keep the value of what lies under them.
- **First runs** (`i125-r6c1-*`, 09:37-09:47, build `build-r6c2`, the ASUS): all six stopped at the first RR frame. The debug layer
  refused the hit distance's clear, because its target had been made without a clear value (the same `native_failure` in the six
  journals; the saves restored by hash; the dumps deleted once read). Fixed: the target is made with its clear value. The first
  `native_water_plane` row with the box's span showed that Firelink's water piece is flat (y −59.77 to −59.77), with the eye 4.9 m
  above it. My earlier reading, a plane over the eye, had compared the plane with the mirror view's eye: it was wrong, and I said so
  to Thomas.
- **Step review** (fresh Opus, high, `25e9a492..e7cb2fd2`):
  - H1: under the executed frame graph (the default), the water target stayed in RENDER_TARGET when the water rows sampled it as
    t54. The return to its home state was dropped, and no later pass declared a use. Fixed: the forward passes declare the read.
  - M1: on RR frames without the traced pass (`no-rt-reflect.flag`, R6b latched off, a frame without the local lights' pass), RR
    received a hit distance of 0 everywhere. Fixed: the tag is given only when the pass wrote it that frame.
  - L1: the water bit came from any water or refraction slice, on any instance. Fixed: g_GXFT_PlanarReflection (0x10) alone, on an
    instance the eye sees.
  - L3: the drawn target's address is now cleared when the water latches off.
  - L4: each pipeline is made when first used.
  - L5: the plane is now carried by the frame's own scene.
  - L8: an ineffective write mask was removed.
  - Recorded, not changed:
    - L2: the renderer's silent refusals. A malformed target or plane cannot happen while the collect makes both itself. An unlit
      main frame (after a load) or a refused camera can coincide with a traced collect (the final review's L5): t54 then keeps its
      last picture. It is safe but stale, and not counted.
    - L6: the regions' cache serves the main view only (commented).
    - L7: a water piece with other overlay slices becomes non-opaque: the hits are unchanged, the any-hit cost rises slightly.

- **The fixes' step review** (fresh Opus, high, `e7cb2fd2..f384b9a0`): no High or Medium. L1: the drawn target's address is now
  also cleared when `RtWaterFrame` latches off. L2: the plan's « flat » claim is now limited to the one piece measured.
- **Runs** (`i125-r6c2-*`, 10:02-10:14, build `build-r6c4`, the ASUS, offline, saves restored by hash, no fault):
  - Iudex Gundyr's arena (Eta travelled to its bonfire, four quarter turns):
    - the traced water ran on every frame (`native_rt_water` 8,684 frames at the 9,000th collect, no failure);
    - mean luma (signalstats YAVG, the window's crop, five views), traced against the mirror view: 72.4/73.5, 89.6/87.4, 81.3/79.9,
      109.6/110.0, 84.8/83.6;
    - seen (`pictures-r6c2/gundyr-mirror-top-traced-bottom.png`): the water stays dark, with bright glints near the character
      that the mirror view lacks. **The sky is not in the water: R6c's witness is not reached.**
  - The cause, read in the rows (INFERRED for the original):
    - the sky's dome `m40_00_00_00_009000` is a forward piece (classes 0x82, category 1, no reflection bit 0x10);
    - the rays have no material for it, so a ray toward the sky takes the region cubes;
    - the native mirror view walks category 0x10 (0x01 at MAX) and so never draws it either: gap 18's likely cause;
    - the original draws the sky in its reflection by a route not read.
    - Next: the dome's forward programme transcribed for the rays (the sky in the rays, R6-PLAN §3 R6d's condition).
  - m45 (Epsilon) and the two sizes did not trace the water. The cause was the run script, not the module (found by the final
    review): PowerShell's `$flags` is `$Flags`, so the first `-off` run appended `no-rt-water` to every later run's flags (their
    baselines' `effect_flags`; the same in `i125-r6c1`). These runs are no witness of R6c. The script's loop variable is now
    `$runFlags`; the runs were made again in the follow-up (below).
  - Not done from the plan's « Seen »: FLIP, the High Wall's and the Undead Settlement's water (none in view from their bonfires,
    `native_water_plane` absent from `i125-r6b3`'s journals there), and the GPU cost.

## R6c's final review (fresh Opus, xhigh, `main...917ecde1`)

- No High finding. It found the code safe to merge as a step: lifetimes, barriers, the plan key, masks, the threads and R6b's
  arithmetic unchanged; the executed plan's comparisons at Gundyr had no mismatch.
- M1: the m45 and sizes runs' cause, the run script (above). Corrected in this RESULT and in the script.
- M2, fixed in the follow-up (`a578621c`, below): the water's hits lose FC_ShadowColor on a frame with no drawn region. `env_scene` is filled only when
  a region is drawn, so the sun's mask term is 1 and every reflected hit is lit as in full sun. At m45 no region is drawn (76
  regions, 0 drawn). This is a datum misread (D-0237).
- L1 to L3: a stale shader comment, the plan's mask definition, a misplaced comment. Fixed (comments and plan only).
- L5: the RESULT's L2 above rewritten as observed.
- L7: the plan's unfinished « Seen » items stated above.
- Recorded, not changed:
  - L4: a refused `trace_hit` pipeline would turn R6b off for the session. It should fall back to the plain pipeline.
  - L6: the plane fallback traces a ray for every texel below the horizon, wherever the water is. The GPU cost is not measured;
    limiting it to the water's screen box is to consider.
  - L8: raw pointers compared for the written mark (as the mirror view's): a re-created target at a freed address reads black.

## R6c's follow-up (2026-11-04 afternoon, Thomas: up to 98 % of the weekly limit)

- **FC_ShadowColor in the rays.** The final review's M2 found the colour lost without a drawn region. The fix's step review found
  more: the camera constants never carry FC_ShadowColor (their +464 is zero), so R6b's and R6c's hits were always unmasked. The sun's
  traced visibility at a hit never darkened it.
  - The first fix read the camera constants and did nothing. The second, `a578621c`, takes the colour from the shadow frame's
    cbSceneParam words (+0x1D0, the 16 bytes the sun pass's b8 binds). The fix's step review found it correct.
  - A witness row, `native_rt_shadow_colour`, prints its first values and each change.
- **Runs** (`i125-r6c3*`, 16:11-16:18, build `build-r6c6`, the script fixed): the traced water ran at m45 (Epsilon), at 1600x900, at
  1024x576 (the 1100x700 window snapped by the game: its UI draws and its reflection target [512, 288]), and with DLSS Quality
  (`native_rt_water` 710 frames at the 1,200th collect in each, no fault, restored by hash). The mirror runs ran without it. These
  runs predate the colour's fix. In `i125-r6c3-m45-on` and `i125-r6c3q-m45-on` the pilot's look input was refused (the Claude app
  in front), so their pictures are not the planned views. A size off 16:9 is not exercised yet (D-0151).
- **The colour's control** (`i125-r6c4-gundyr-on` and `-off`, 16:37-16:44, build `build-r6c7`, no fault, restored by hash).
  - `native_rt_shadow_colour` gives (1, 1, 1, 0.005) (OBSERVED): the shadowed term is lerp(0, 1, s). By the code, a hit in the
    traced sun's shadow now loses its sun's diffuse and specular, as the sun pass does; before the fix every hit was lit as in full
    sun. The effect on the pictures is not measured: the traced views' luma is within 0.5 of the runs before the fix. The no-region
    case (m45) has not been run after the fix.
  - The module the runs loaded: `build-r6c7/game_bootstrap.dll`, rebuilt incrementally with the witness change (`build-r6c8.log`).
  - The traced water ran on 8,681 frames.
  - Mean luma (YAVG, the same five views), traced against the mirror view: 72.7/75.2, 89.4/87.5, 81.2/82.4, 109.7/106.7, 84.3/85.8.
- **The follow-up's final review** (fresh Opus, xhigh, `main...646f986c`): no High.
  - M1, M2 and L1 to L3: the documentation corrected (the sizes, the contradicting sentences, the observed against the inferred,
    the evidence, the refused inputs).
  - Recorded, not changed:
    - L4: the renderer could take FC_ShadowColor from the frame's own `shadow_scene` (+0x1D0), beside `rt_reflect_sun_`, instead of
      a second copy in `NativeLightingInput`. Both carry the same bytes today; to simplify in the next tranche.

## R6d step 1: the sky in the rays (2026-11-06, the R pilot's fresh thread, branch `claude/inspiring-cartwright-00e671`)

- **All the water** (Thomas: every water of the game must reflect the scenery). The study thread's census
  ([STUDY-WATER-FAMILIES](STUDY-WATER-FAMILIES.md), OBSERVED in the programmes and the MTD tables): the five water families' 30
  forward programmes all sample t54 with one formula, and the four water MTDs all set g_GXFT_PlanarReflection (0x10). So R6c's water
  bit (`RtMaskWater`) misses no water, and R6c's pass (each ray stopped at its own water surface) already serves every water piece.
  What the water lacked was the sky (R6c's witness); the plane is the original's single plane a frame (study §2).
- **Written** (R6-PLAN §7; `e2e51d5a`, its review's fixes `8c3e0102`). A slice of a forward class whose Fwd programme is
  `GXFlver_Col_Fwd`'s body gets a forward record (diffuse, COLOR1, FC_DynamicDiffuseColor from the admission's own code, factored as
  `NativeDynamicDiffuseColour`; the row's blend as two factors). Its instance row gains mulCol and the model's forward fog
  (`CollectNativeRtForwardFog`, after the light blocks). A reflection ray that meets no surface composes the forward layers along it
  (`SkyRadiance`), each coloured and fogged as `Col_Fwd` does (since step 2 along the reflected leg); no opaque layer: the cubes as before.
- **Step review** (fresh Opus, high, `008356d8..e2e51d5a`): no High. Fixed in `8c3e0102`: M1 a triangle the traversal reports twice
  is one layer; M2 the forward sets made again every walk (the dynamic colour), a colour not finite zero; L1 an instance whose fog was
  not built passed over (the raster refuses its rows then); L2 the witness behind `rt-view.flag`; L3 the unit test's rows view; L4 a
  record only with UV0 and COLOR1 read; L6 the scene's `rt_scene` const again. Recorded: L5 the composition is the nearest opaque
  layer and the blended ones in front of it, not the rows' order (the witness decides); the witness's statics are mutex-guarded in a
  function the thread-affinity check does not scan, as that function's other witnesses.
- **First run** (`i125-r6d1-*`, 09:15-09:25, build `build-r6d1`, the ASUS, offline, restored by hash, no fault): the dome was **not**
  traced. Its witness (`native_rt_forward_slice`): `m40_00_00_00_009000`'s `GXFlver_Col` slices (modes 0 and 2) and its
  `MultiBlend2_et1_nolight_MuitiBlendMask` slice, all `forward: false`. Cause: the dome's group also resolves a G-buffer programme, so
  the slice was read as a G-buffer one (the Col G-buffer programme takes no FC_DifMapMulCol: no material) and its Fwd programme never
  looked at. Fixed in `8c3e0102`: a slice whose class has no G-buffer bit is read by its Fwd programme. The m45 pair: the mirror run's
  look input refused (the Claude app in front), both closed at 60 s as planned.

- **The fixes' step review** (fresh Opus, high, `e2e51d5a..8c3e0102`): no High. Answered in `0ebbf8fc`: M-a the forward sets made again
  only when a forward slice's FC_DynamicDiffuseColor changed (it was every walk); L-a the fog marker tested first (`RtForwardBuilt`);
  L-b the forward COLOR1 in its own record fields. Recorded: L-c the rays zero the colour where the admission's block keeps earlier
  words (a generator over +0x180, none known); L-d other forward bodies and the G-buffer-class skies without a depth write stay out.
- **Second run** (`i125-r6d2-*`, 10:07-10:14, build `build-r6d4`, `rt-view.flag` for the witness, no fault): the dome's two `GXFlver_Col`
  slices now `forward: true` (blends 0x32 and 1, FC_DynamicDiffuseColor 1.5). The water's luma barely moved (the water band of
  `sweep-3-2` 138.3 against 139.0 before R6d), and against the original (`i113-t11-g18-reference`, `evidence/I-0113/t11/g18-water-crop.png`)
  the near water stays dark where the original's reflects a bright sky: the anomaly Thomas named (10:19: a natural, true lighting,
  not a copy of the original; a dark water because the sky does not reach the rays is a bug).
- **The witness** (`rt-sky-witness.flag`, a diagnostic: the water's texels coloured by what the reflection ray met). Run `i125-r6d3-*`
  (10:34-10:41, build `build-r6d6`): the water toward the sky **blue**, the dome's layers met with their fog but no opaque layer. The
  study's table (STUDY-SKY-PROGRAMMES §1): the opaque sky is the scrolling zenith `MultiBlend2_et1_nolight_MuitiBlendMask` (blend 0,
  depth written), the `GXFlver_Col` dome slices blended over it.
- **Step 2, the zenith and the cloud sheets** (`a5321b5e`, `7a9dd13c`). `MultiBlend2_et1_nolight_MuitiBlendMask_Fwd` and
  `Fujir_Nolight_OpacityBlend_Fwd` transcribed from their DXBC (`evidence/I-0125/artifacts/dxbc`): their textures by the rows the
  programmes read, their UVs, TEXCOORD1.x or TEXCOORD3.x, COLOR0.w, the generated words from 0x270 (R3c's route), FC_GlobalTime passed to
  the rays (the simulation clock ProducePerFrame takes). Run `i125-r6d4-*` (11:05-11:12, build `build-r6d7`, the zenith only): the water
  toward the horizon **magenta**, the sky composed. The near water in front of the character still not: its rays meet something before
  the sky (the next witness: a surface without material green, a surface hit red to white by distance).
- **What the near water meets** (runs `i125-r6d5` and `-r6d6`, 11:41 and 12:14, builds `build-r6d9`, `-r6d11`; `rt-view-last.flag` for the
  control view's last pictures, at the arena): the near water's rays **red to pink**, surface hits at about 20 to 50 m; no texel white
  (every eye ray meets a water slice), and the control view's traced pictures at the arena show nothing overhead (the sky black: no
  surface). The water plane `[0, 1, 0, 69.02]` with the eye 1.55 m above it (`native_water_plane`): a reflected ray from the near
  water rises at about 17° and meets the arena's walls, so the near water reflects the walls (INFERRED from the geometry and the hits'
  distances), the far water the sky. The original's mirror view draws only the reflection category and showed the sky there
  (`g18-water-crop.png`): the traced picture differs from it by being physically right (Thomas, 2026-11-06 10:19: a natural, true
  lighting, not a copy). What was wrong: the walls reflected unfogged, darker than the eye sees them.
- **The hits' fog** (`9bd2927b`): GXLightComposite's fog (its listing: the same distance and height fog as Col_Fwd's, the pixel's
  FC_FogParamArray set) applied to every reflection hit along the reflected leg, the reflecting point as the distance's origin (the first
  leg is fogged by the raster: the water's programme, the composite); the sky's layers the same way. Run `i125-r6d7-*` (12:38-12:45, build
  `build-r6d12`, no witness, no fault): the water band's luma (`sweep-3-2`, 900x220 at its foot) 72.4 traced against 69.7 before R6d
  (`i125-r6c4`) and 68.3 in the mirror view; the far water reflects the bright sky (`pictures-r6d/final-sweep-3-2.png`: before R6d,
  R6d, the mirror view). A reflected leg shorter than ExtFogParam.y gets no distance fog (the programmes' own start).
- **Step 2's step review** (fresh Opus, high, `0ebbf8fc..9bd2927b`): no High. Fixed: M1 the height fog at the point's height (H.w is
  the camera's y, written by 14E900 from the view's +0x34: the term is the camera's, not the reflecting point's); M2 a forward record
  of the zenith or the cloud sheets only with every sampled role's view (a full heap left an index ~0); L1 no `RtRecordLayers` on a
  forward kind's layer record, a refused blend's layer dropped; L2 a row the programme reads unwritten refuses the record; L4 one
  generated-words lookup and one weight helper (`079ebcda`). Recorded: L5 the rolling slots carry no serial (the files' times order them).

## R6d's final review (fresh Opus, xhigh, `main...841f1e66`) and its answer

- No High. The transcriptions (Col_Fwd, the zenith, the cloud sheets, the composite's fog), the layouts, the bindings, the lifetimes and
  the raster's unchanged behaviour checked fine.
- M1, recorded: R6b's specular ambient pass changed on every zone (a miss takes the forward sky, every hit is fogged) and no run after
  R6d looked at R6b's surfaces (Firelink, the High Wall, the Undead Settlement): **not seen**. Next tranche's first check.
- M2: the final code's run, `i125-r6d8-gundyr-on` (13:13-13:16, build `build-r6d14`, no fault, restored by hash; the traced water on 8,683 frames, R6b's pass on 8,685, none missed; the water band's luma 73.5 in `sweep-3-2`, 72.4 in r6d7); the plan's « the study's other waters » **not seen** (every water family
  is served by the same pass, STUDY-WATER-FAMILIES §1; a look at the High Wall's and the Undead Settlement's water is open).
- M3: STATE and the investigations index rewritten in the merge checkpoint.
- Recorded for the next tranche (L2 to L12): the witness's bookkeeping run whatever the flag and its start not journaled; the forward
  colour check under the shard's lock (an engine query there); the forward fog's builder making the cbLight entry too; the rows'
  368 bytes for every instance; « counted » refusals that only the witness names; the forward slices in R3's material counters; an
  unreadable mulCol taken white where the raster refuses the instance; the blended sky layers evaluated before the opaque one is known;
  the hard-coded row offsets without `static_assert(offsetof)`; four stale comments (the witness's colours, RT_RECORD_FORWARD's and
  SkyRadiance's « GXFlver_Col », the kinds); the CONTRACT's R6d scope and `status.json`; a refused forward slice's textures kept pending.


## R6e: R6d's checks and its final review's Lows (2026-11-06, the R pilot's fresh thread, branch `claude/elastic-haibt-7e70f9`)

- **R6b's surfaces after R6d** (M1 of R6d's final review). Runs `i125-r6e1-on-*` (traced) and `-off-*` (the cubes, `no-rt-reflect`):
  13:56-14:03, main's module `75eb6c5e` (`build-r6e0`, the launcher's, SHA-256 C2078D65…F38D), Run-R6aCheck at m40, m30, m31, the
  ASUS, no fault, every save restored by hash. Mean luma (signalstats YAVG, the window's 1280x720 crop), traced before R6d (`i125-r6b3-on`)
  then after: m40 49.0 / 49.1 and 50.0 / 50.1; m30 66.6 / 67.9, 53.5 / 53.4, 71.7 / 73.0; m31 41.6 / 41.6 and 42.3 / 42.3. The cubes'
  runs of the two series differ by up to 0.9. The m30 views hold sky whose clouds move between runs, so their lower halves (the ground,
  no sky; the final review's L3) were measured too: traced 64.6 / 66.6, 62.8 / 64.1, 77.8 / 78.8, the cubes' 66.2 / 65.6, 63.2 / 63.9,
  78.8 / 78.1. So at Firelink and the Undead Settlement R6b's surfaces are unchanged by R6d within the runs' spread; at the High Wall
  the traced ground is 1 to 2 steps brighter after R6d (the cubes' within 0.7), now level with the cubes where it was about one step
  under (INFERRED: R6d's misses take the forward sky in place of the cubes, its hits their fog; expected of R6d, not a defect under
  D-0249). Pictures `pictures-r6e1/m30-*-before-after-cubes.png` (before, after, the cubes). Two views lost (m40 and m31 `walk-2`):
  the Claude app took the foreground (below).
- **The other zones' water** (M2): **not seen.** No water is in view from the m30 and m31 zone saves' bonfires (`native_rt_water`
  frames 0; Firelink 469). The six water pieces were located from the MSBs and the FLVER bounds (`evidence/I-0125/artifacts/water-boxes-m30-m31.txt`,
  a Sonnet worker's read with SoulsFormats: m31's `008315` plane at y −240.8 about 31 m from the lowest bonfire object, `007700` and
  `008040` 35 to 41 m from another; m30's `004003` 35 m from one; the bonfires' names not established). Two travel-menu probes
  (`i125-r6e2-menu-*`, `i125-r6e3-menu-*`) lost the foreground to the Claude app, and Beta loads a step off its bonfire. Every water
  family is drawn by the same traced pass (STUDY-WATER-FAMILIES §1): left for R11's zone tour.
- **The Lows** (`03fefece`, the step review's `ce09ba67`): the forward block (mulCol and fog, `RtForwardRow`, 112 bytes) only for the
  forward instances, after the skinned bases, its byte in the row's last word: the rows back to 256 bytes, their offsets and the block's
  asserted with `offsetof`; a forward instance whose mulCol or fog cannot be read gets no block (it was taken white; the rays pass its
  forward slices over, as the raster refuses it); `ProduceNativeForwardLight` makes the fog entry alone for the rays (the light records
  still looked up); the forward colours read outside the shard's lock, the entry taken again only if it still holds that set; the sky's
  blended layers traced after the opaque one is known (two traversals, the second up to it); `forward` and `forward_made` in the
  `native_rt_materials` row, out of R3's role counters; a forward slice refused for a part it cannot read, or a texture that cannot
  arrive, gives its textures and the pending state back; `rt-sky-witness.flag` journaled (`native_rt_sky_witness`) and its bookkeeping
  on its frames only; the stale comments; the CONTRACT's R6d and R6e, `status.json`.
- **Step review** (fresh Opus, high, `b03e58e2..03fefece`): no High, no Medium. L-1 (a refused slice whose texture cannot arrive kept
  its other textures), L-2 (an engine colour call for a refused slice), L-3 (four comments), L-4 (the forward slices without texture
  bindings counted in `no_gbuf`): fixed in `ce09ba67`. L-5, no test or run of the forward path: the witness runs below. Recorded, not
  this step's: `NativeDynamicDiffuseColour`'s provider query (vtable +0x70) runs on the admission pool's threads from the rays' walk,
  as before; whether it is a pure getter is not established (a decompiled read of that slot, or the thread-affinity allow list).
- **Seen with the final code** (`build-r6e1` from `ce09ba67`, the ASUS): `i125-r6e2-gundyr-witness` (14:26-14:30, `rt-sky-witness`):
  the water's classes as R6d's witness (`pictures-r6e1/gundyr-witness-r6e.png` against `pictures-r6d/witness4.png`: the far water
  magenta, the forward sky composed); `i125-r6e3-gundyr` (14:53-14:56): no fault, the traced water on 8,683 frames and R6b's pass on
  8,685, none missed, `forward` 15 and `forward_made` 15; luma against R6d's `i125-r6d8-gundyr-on`: arrive 74.1 / 74.5, the sweeps
  88.4 / 89.7, 80.9 / 82.8, 111.9 / 111.4, 84.6 / 85.2 (the sky's motion between the runs;
  `pictures-r6e1/gundyr-r6d-top-r6e-bottom.png`). One launch (`i125-r6e2-gundyr`, 14:24) was refused at the title before the world:
  `native_ui_movie_viewport_contract` (17_render_step.inc, the title movie advanced before the window was open), its dump in the run's
  `crashes`; unrelated to R6, first seen in the I-0066 journals, passed to the open-gaps pilot.
- **The foreground** (LIVE-RUN-NOTES): the Claude app came to the front during four of the runs (13:57, 14:30, 14:57, 15:00), twice
  at a coordinator's message and twice with none: a pilot's input is then refused (`Pilot-Game.ps1`), its pictures show the app.

## R6e's final review (fresh Opus, xhigh, `main...a15af3d8`) and its answer

- No High, no Medium. Checked fine: every Low of R6d's review answered; the sky's composition the same set of layers; the layouts and
  the HLSL loads; Encode's sizing, offsets and guard; the cache path's two locks; the raster's forward rows unchanged; the mandate.
- Fixed in `e1b10b78` (its own step review: no High, no Medium; its two Lows, a stale header comment and the test's upload count,
  fixed in the merge checkpoint): L4 the rows buffer's comments; L5 the second sky traversal only when a blended forward candidate lies
  in front of the opaque layer (exact: the first traversal meets every candidate nearer than the final layer); L6 one forward block a
  draw group, its mulCol read once, Encode writing each block once; L7 the forward refusal by the slice's own wait; L8 a unit test of
  the shared block (`rt_scene_test.cpp`); L9 RtForwardRow's colour default zero, the foreground count; L1 STATE and the index in the
  merge checkpoint; L2 R6-PLAN's R6d and R6e; L3 the High Wall measured without its sky (« R6e » above: its ground 1 to 2 steps
  brighter after R6d).
- The final code's run, `i125-r6e4-gundyr` (15:39-15:42, `build-r6e1` from `e1b10b78`, the ASUS): no fault, restored by hash; the traced water
  on 8,682 frames, R6b's pass on 8,684, none missed; `forward` 15, `forward_made` 15; luma arrive 75.2, the sweeps 88.4, 81.2, 110.9, 84.4
  (`i125-r6e3`: 74.1, 88.4, 80.9, 111.9, 84.6).

## R6f: the mirror view removed (2026-11-06, the R pilot, branch `claude/relaxed-bose-a8f872`)

- **Thomas's acceptance** (project chat, 2026-11-06 16:57, relayed): « Oui du coup on supprime le raster s'il y a un équivalent
  tracé », D-0251: the traced water accepted, and a raster path with a traced equivalent removed, not kept behind a flag.
- **Removed** (the code, about 260 lines): the mirrored camera, its culler copy, its clip plane, its walk with the gather mask 0x10,
  its admissions (its own instance carry `native_reflection_instances`, its tables' carry, its borrows of the main records, the
  thread flags `native_reflection_preparing` and `native_reflection_clip` with every test of them), its lighting collect, its scene
  (`NativeReflectionState::scene`), the frame job's preparation and draw of the view (`native_reflection_drawn_target`, the
  offscreen job's `prepare` that only it used), `no-rt-water.flag` (and its place in Run-StartupTrace's pattern).
- **Kept**: the plane of the nearest water mesh (WT-6) and the t54 target, now `CollectNativeWaterPlane`, made only when the traced
  water is on and not failed; the ReflectionQuality option's « off » (the black default); `no-water-reflection.flag`; the target's
  « written » mark from the traced pass (RtWaterDrawn). The traced water off or failed for the session gives the water the black
  default, as the original without a reflection object; a collect without the rays' scene keeps the previous state (the step
  review's M1, below)
  (D-0251: no raster mirror view to fall back on). The cubes stay for a reflected ray that meets no sky (R6b, R6d).
- **Left for D-0251 elsewhere** (not this step): the region cubes' draws (`EncodeNativeEnvRegions`, the frames without the local
  lights' pass) and `no-rt-reflect.flag`; the SSAO and the baked ambient (R7, I-0126).
- **Step review** (fresh Opus, high, `ebeb755a..1ac5a9a0`, read-only during the full control: no build): no High. M1, an isolated
  collect without the rays' scene made the water black on the *next* (served) frame and fresh water admissions twice, since a
  collect's epochs read the reflection's record before its walk: fixed, such a collect leaves `traced` and the target as they are
  (the water keeps its last picture for that frame; a long run of skipped walks keeps it too, a known limit). M2, the offscreen
  views' RGBA16F copy path only the mirror view used: removed (the menu views' R8G8B8A8 gate as before WT-7). L2, the stale
  comments; L3, the option's static_assert: fixed. L1, the record check's nullable scene now always given: left (harmless). L4,
  STATE's row: at the checkpoint. Fine as reviewed: the traced path unchanged in steady state, no symbol left, every condition
  that lost its flag keeps its meaning, the lock's use.

## R6f's final review (fresh Opus, xhigh, `main...a24cfa4d`) and its answer

- **H1** (a scope question, Thomas's): the rays run only with `rt-scene.flag`, which no launcher places (the play launcher removes
  every flag): every session without it, Thomas's play sessions included, now has no water reflection while the other raster
  categories still run. Asked to Thomas (2026-11-06 ~20:25): the rays the default (`no-rt-scene.flag` to cut them), merge as it is, or
  hold R6f. His answer (2026-11-07 00:48, « Et je choisis A », D-0253): the rays the module's default, `no-rt-scene.flag` to cut
  them; written in the same merge (`20_startup_phases.inc`, the comments that named `rt-scene.flag`, Run-StartupTrace's flags).
- **M1** (superseded by D-0253: the start check now runs with the rays): the start check ran without the rays, so it could not see the traced water: R6f's check is a run with the rays after the
  merge (D-0227), the traced water's rows and the luma against `i125-r6e3`. **M2**: the stale documentation (STATE's rows, the index,
  the Conclusion, the « Kept » bullet, R6-PLAN's « Seen », the CONTRACT): rewritten. **L1**: the walk's frustum gate only the rays'
  whole box drew after R6f: removed (`NativeBoxInFrustum`, the walk's `frustum` parameter; the visitor's `rays_walk` for the
  diagnostics). **L3**: the kept state bounded, the black default from the 30th collect in a row without the rays' scene. **L4**:
  the comments. **L5**: the endurance's SC_CLOSE close not yet run (said in its comment and in WORKFLOW, the tour not yet at the
  OLED's size). **L7**: the endurance's two sessions said as two. **L2** (the record check's null scene) and **L6** (`no-rt-water`
  kept in Run-StartupTrace's stale-flag cleanup) left. Rebuilt `build-r6f2`.

## R6f's re-review (fresh Opus, xhigh, `a24cfa4d..54253844`, the default flip and the first review's answers)

- **H1**: the start check counted any line holding `_refused"` as a refusal, and the rays' periodic row carries `"pt_refused":false`:
  with the rays the default every start check would have failed (5 such lines in `i126-r7a3-m40-gi`): the refusals by the event's
  name only; and a traced path latched off (`native_rt_*` with `"on":false`) now fails the check when the self-test passed.
  **M1** the Conclusion, **M2** STATE's « What runs natively »: rewritten. **L1** a stale comment (the visitor's two equal bools
  left), **L2** the kept water's comment (a load not covered: a known limit), **L3** a `native_rt_scene` row when
  `no-rt-scene.flag` cuts the rays (the gate keeps no `no-rt-scene` pass-through: its thresholds' fate is Thomas's, D-0253),
  **L4** Run-I117T0's off leg with `no-rt-scene`, **L5** I-0126's status and evidence, **L6** STATE's branch row, the index, the
  evidence. Rebuilt `build-r6f6`.

## The full control of 2026-11-06 (D-0230)

Main `627ec217` (native/ of `ba7ededc`), the launcher's module, 2026-11-06 17:05-19:55 Paris, offline, every save restored by hash
([Run-FullControl-1106.ps1](Run-FullControl-1106.ps1), a copy of I-0124's with the screen as a parameter; logs
`evidence/I-0125/artifacts/full-1106*.log`). Thomas was there and had turned the OLED on at 16:59 (both screens attached).

- **The replayed path gate** `full-1106-gate` with its memory leg, the ASUS base and, the OLED being on, the OLED base:
  **FAIL on the ASUS base, display_late alone**: its four measure runs 192, 337.3, 12.7 and 11 (threshold 22; the two first
  17:10-17:19, the two confirmation runs 17:44-17:52 within it); every other metric within its threshold (period 16.67 ms; over
  2.04 P 1.5; over 1.5 P 19.75; game steps over 2.04 P 1.5; the standstill pictures within theirs). **PASS on the OLED base**
  (display_late 11.9; over 2.04 P 2.5; over 1.5 P 18.5). Memory leg 0.5 MB a visit (threshold 40). The cause of the two high runs
  is not established (INFERRED, a disturbance of the display composition with the two screens just turned on and Thomas at the PC;
  not a code change: main's native tree passed on 2026-11-03 and the two later runs pass): the ASUS base's measure to run again with
  the ASUS alone, at the next slot; `.project/REPLAY-GATE.json`'s record left as the gate wrote it (no pass recorded).
- **The control sizes**, the replayed path each: 1920x1080 in full screen (`full-1106-1080fs`) and a 1500x844 window
  (`full-1106-1500w`): no fault, restoration held, the controller's 0xD3126612 and `closed` 1 as every `-GraphicsLevels` run.
- **The night endurance**, on the ASUS: `full-1106-night` 18:06-19:12, 65.8 minutes, 38 loads, no fault, no crash dump, 60 frames/s,
  private memory +2.4 MB a visit (Get-EnduranceSummary), stopped by its window's close at Thomas's request to move it to the OLED
  (restoration ok); two OLED attempts (`full-1106-night-oled`, 4 minutes; `-oled2` in a 2560x1440 window, 2 minutes: the tour's
  inputs off at that size, Thomas: « le script » is « décalé ») stopped clean, no fault; then the remaining half hour on the ASUS,
  `full-1106-night-asus2` 19:27-19:55, 28.1 minutes, 16 loads, no fault, no crash dump, a normal close. Two sessions (65.8 and 28.1 minutes,
  54 loads) kept by Thomas in place of one 95-minute session (19:14: « comme ça on repart pas de 0 »): the memory's growth within
  one session is measured over 66 minutes, not 95.
- Left: the ASUS gate's measure again; the endurance tour at the OLED's 2560x1440 window (the open-gaps pilot's scaled inputs,
  `3722fcdc`, then the tour's pixel tests) before the next full control (due 2026-11-09).

## Evidence

- R6f: the builds `evidence/I-0125/artifacts/build-r6f1` to `build-r6f5` with their logs; the full control's logs `full-1106*.log`.

- R6e: the builds `evidence/I-0125/artifacts/build-r6e0` (main's launcher module, copied), `build-r6e1` (logs `build-r6e1.log` to
  `build-r6e3.log`); the run logs `runs-r6e1.log` to `runs-r6e3.log`; the journals `evidence/I-0066/artifacts/i125-r6e1-*` to `i125-r6e3-*`;
  the series script `scripts/Run-R6eChecks.ps1` (a copy of Run-R6dSky.ps1); the pictures `pictures-r6e1/`; `water-boxes-m30-m31.txt`.

- R6d: the builds `evidence/I-0125/artifacts/build-r6d1` to `build-r6d14` with their logs; the run logs `runs-r6d1.log` to `runs-r6d8.log`;
  the journals `evidence/I-0066/artifacts/i125-r6d1-*` to `i125-r6d8-*`; the series script `evidence/I-0125/artifacts/scripts/Run-R6dSky.ps1`
  (a copy of Run-R6cWater.ps1 with the checkout and the modes as parameters); the pictures `pictures-r6d/` (`final-sweep-*`: before R6d,
  R6d, the mirror view; `witness2`, `witness4`: the sky witness); the programmes' listings `dxbc/Col_Fwd.*`,
  `dxbc/MultiBlend2_et1_nolight_MuitiBlendMask_Fwd.*`, `dxbc/Fujir_Nolight_OpacityBlend_Fwd.*`.

- R6c: the builds `evidence/I-0125/artifacts/build-r6c2` to `build-r6c7` with their logs (`build-r6c8.log` the last incremental
  one); the run logs `runs-r6c1.log` to `runs-r6c4.log`; the journals `evidence/I-0066/artifacts/i125-r6c1-*` to `i125-r6c4-*`; the
  series script `evidence/I-0125/artifacts/scripts/Run-R6cWater.ps1`; the side-by-side `pictures-r6c2/gundyr-mirror-top-traced-bottom.png`.
  The luma: ffmpeg signalstats YAVG on the game window's crop (1280x720 at 8, 31) of `arrive.png` and `sweep-1-2` to `sweep-4-2`.

- The programmes' disassemblies: `evidence/I-0125/artifacts/dxbc/` (`fxc /dumpbin` of the I-0063 cache: `GXEnvMapRegion`,
  `_Pcc`, its `.vpo`, `GXLightDeferredAO`, `GXLightAcc_DDirLightShadowed1`, `GXLightComposite`, `GXLightCmbAlbedo`,
  `GXFlver_WaterBumpTriInteractionFloatObj_Fwd`, the MulMaskDOL and MultiBlend2_et1 G-buffer `.vpo`).
- R6a step 1: the builds `evidence/I-0125/artifacts/build-r6a1`, `build-r6a2` with their logs; the runs `runs-r6a1.log`,
  `runs-r6a2.log`, the journals `evidence/I-0066/artifacts/i125-r6a1-*`, `i125-r6a2-*` (`native_rt_material_check` rows); the
  runner `evidence/I-0125/artifacts/scripts/Run-R6aCheck.ps1`, a copy of I-0119's `Run-R3ZoneTour.ps1`.

## Closed (2026-11-09)

Closed 2026-11-09 with Thomas's yes (relayed by the coordinator, 01:55 Paris): the open points below are carried to [I-0132 R11-PLAN §6](../I-0132-rt-optimisation-zones/R11-PLAN.md#6-the-open-points-carried-from-the-investigations-closed-on-2026-11-09), where each has its place.

## Uncertainty / unresolved questions

- The TO CHECK items of R6-PLAN §1, §4 and §5; the families still off, above.

## Artifacts produced or modified

- R6-PLAN, this RESULT, the CONTRACT; the code of R6a step 1 (`rt_material.hlsli`, `rt_view.hlsl`, `rt_scene.*`,
  `12_scene_collect.inc`, `05_selected_scene.inc`, `scene_pass_frame.cpp`, `rt_scene_test.cpp`).
