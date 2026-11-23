# I-0119 - DS3 DX12 Vague 2: R3 of the RT/PT plan, the materials for the rays (Les matériaux pour les rayons): result

## Conclusion

Complete (closed 2026-11-09 with Thomas's yes, « Closed » below); its history: Active (opened 2026-11-02 18:10 by the RT pilot on branch `claude/brave-hofstadter-df7500`, after I-0118's R5; plan
[R3-PLAN](R3-PLAN.md)). The gloss is identified (§ « The gloss »); the G-buffer programmes' families are read ([GBUF-FAMILIES](GBUF-FAMILIES.md));
R3a (a material table kept on the GPU, a record for every slice of the reference shape) and R3b (its control view against the
G-buffer: the reference families within a few steps, F0 exact) are written, run at the High Wall, Firelink and m45, reviewed (step,
final and fix reviews) and merged as one tranche, `493e58aa` (the launcher rebuilt, start check `start-493e58aa` passed). R3c, the
families off the shape: the map blends merged `ae6179cd`; the second tranche (the blends' parameters P0 to P2, never found before;
the two-layer Mul and MulMask pieces) merged `88578803` (the launcher rebuilt, start check `start-88578803` passed; § « R3c second tranche »); the third tranche (the skin's curve, the equipment by its rows,
the raster's texture fallback in the rays, the reference shape only where the programme writes it) merged `a4d9ec6a` (§ « R3c third
tranche »). Next: § « Suggested next step ».

## The gloss (2026-11-02)

- OBSERVED (`GXFlver_ColDifSpcBumpDOL_Gbuf.ppo`, the I-0068 disassembly): the G-buffer programme writes GB1.x = the bump map's
  (t3 `g_BumpmapTexture`) b × (FC_SpcParam.x / 256)², FC_SpcParam.x being the MTD's `g_SpecularPower` (`ProduceMtdParams`).
- OBSERVED (`GXLightAcc_DPointLight1.ppo`, disassembled from the I-0063 programme cache): the light programme reads GB1.x as a
  Blinn-Phong exponent n = 2^(13 × GB1.x), clamped by the light's cb1[1].w, the w of its diffuse colour row (n' = max(1, n w /
  (0.01 n + w)); I-0117 T7's final review: not SpecColor.w), normalised by
  (n' + 2) / 8, times F0 = GB3² (GB3 holds sqrt F0); its diffuse times (1 − F0 / 2).
- INFERRED: a GGX roughness for the traced lighting and Ray Reconstruction, r = (2 / (2^(13 g) + 2))^¼ (the usual Blinn-Phong to
  GGX equivalence, not DS3's own).

## The Fresnel (2026-11-03, the R pilot after I-0117 T7)

- OBSERVED (`fxc /dumpbin` of the I-0063 programme cache's `GXLightAcc_DPointLight1.ppo` and `GXLightAcc_DSpotLight1.ppo`, the
  pilot): the local lights have **no Schlick term**: the specular is Spc × NdotH^n' × (n' + 2) / 8 × F0 (F0 = GB3², a constant
  reflectance), times saturate(N·L)^0.1 (`log`, × 0.1, `exp`) instead of a visibility; their diffuse is times (1 − F0 / 2), a
  Lambert where GB1.y is 0, else the wrapped max(N·L + 10 GB1.y, 0) / (1 + 20 GB1.y), plus a translucency term (cb1[3].x ×
  the diffuse colour × 10 GB1.y × saturate(L·V)^(32 cb1[3].y) × ((1 + N·V) / 2)², V the camera-to-point direction), as the
  sun's (GBUF-FAMILIES step 4). The point light's falloffs: max(1 / d² − cb1[0].w, 0) for the diffuse, 1 − saturate(d² ×
  cb1[0].w) for the specular (cb1[0].w, `PositionAndRadius.w`, used as 1 / R²: a reading of its use). The spot (cb1[0].w its
  range R) selects by its cb1[3].w (`ShadingMode`): above 0 the point light's lobe and falloffs (1 / R² made from R) times its
  cone max(1 − |xy|², 0); otherwise the reflection lobe (R·L)^(n'/4) × (n' + 2) / 8 replaces the half-vector lobe, without the
  N·L^0.1 factor, and both terms fall off as saturate(1 − w / R)² × cone², w the light-space w (cb1[7]); in both modes both terms
  are zero unless the light-space z (cb1[6]) is above 0 (the pilot's reading, corrected by I-0117 T7's final and step reviews).
- OBSERVED (the census's `GXLightAcc_DDirLight1.ppo`, GBUF-FAMILIES step 4): the sun's specular has Schlick's
  F0 + (1 − F0)(1 − V·H)^5 and the NdotL / max(NdotV, NdotL) visibility; its diffuse times (1 − F0).
- So DS3 has two Fresnels, one for the sun and one for the local lights: the traced lighting (R4, R6) reads F0 from the material
  table (GB3², exact since R3b) and applies the one of the light it samples. No code for it before R4: nothing traced shades a
  specular yet (the sun's rays carry visibility, the control view albedo), and a function nobody calls is dead code (WORKFLOW
  « Optimise, do not stack »). The local lights' falloffs and the spot's two modes go with R4's light list.

## The census of the G-buffer programmes (2026-11-02)

- [GBUF-FAMILIES](GBUF-FAMILIES.md) (a Sonnet subagent, read-only; the pilot re-read the reference, the point light and `EmiIbl`'s
  emissive): of the 676 G-buffer programmes, 216 write the reference's gloss (the bump's b × (FC_SpcParam.x / 256)², 94 of them
  exactly t3.b), the others a wet or blood formula (260), raw multi-layer blends (90), a constant 0 (52) or another shape. The
  families and their tranches: R3-PLAN §1's table. The sun's programme (`GXLightAcc_DDirLight1.ppo`): the same exponent, a
  Schlick Fresnel, the diffuse times (1 − F0) (OBSERVED by the census; the point light's (1 − F0 / 2), read by the pilot).

## R3a, the material table (2026-11-02)

- **What it does** (`bba25213`): T3's per-frame alpha records give way to a material table kept across frames on the GPU
  (`RtMaterialTable`): every LOD-0 slice of a rays' instance with a G-buffer programme read gets a record (`RtMaterialRecord`, 192
  bytes; `NativeRtSliceMaterial`): its triangle (indices, positions), UV0 (UV16, float2 or half2), NORMAL and TANGENT
  (R8G8B8A8_UINT), the G-buffer programme's g_DiffuseTexture, g_SpecularTexture, g_BumpmapTexture and g_EmissiveTexture resolved
  as the raster resolves them with their tiling rows, the MTD's g_DiffuseMapColor and gloss scale, the family's index, and T3's alpha
  test unchanged. A set's records are copied into a default buffer when it is first named (the table grown by half again when
  full), its range freed when the set is gone or unnamed for 120 frames; a frame buffer holds each instance's row (scroll, first
  record, count, a skinned instance's positions and slice bases). `rt_material.hlsli` turns a hit into a material (R3-PLAN §1's
  reference; the vertex frame rotated by the placement as the vertex programme does; a skinned instance's shading normal its
  triangle's; the level from the ray cone). The control view's traced picture shows the hit's albedo. Emissive colour white
  (FC_EmissiveColor not read, TO CHECK).
- **Unit test** (`rt_scene_test.cpp`, `Materials`, build-3): T3's cases kept; the quad's albedo the texel times FC_DifMapMulCol,
  its gloss the bump's blue times the scale, its F0 the specular texel, its normals toward the ray; a second frame writes no
  record; the views and the range released after 120 frames unnamed.
- **Step review** (fresh Opus, high, `9069cbac..bba25213`): nothing high. Answered (build-5): M1 a colour-only alpha test wrote a
  zero UV0 over the material's (now only a sampled test writes it); M2 a role with no bound path (none, empty, FaceGen) made the set
  pending for good, remade every 30 walks (now left out; pending only when bound and not resident or a stand-in); L1 a skinned
  instance whose positions view the heap refused read its bind pose (now no record read at its hits); L2 the probe read another
  set's record (guarded); L3 `Retire` could throw through `noexcept`. Kept: L4 t0 resolved twice for an alpha slice (the
  material's and T3's), L5 the families' registry lock at set builds, L6 the scroll applied to float UVs (TO CHECK in R3b),
  L7 the test's gaps (the table's growth with a former table, a range reused, a full heap, a skinned hit).

## R3b, the material control view (2026-11-02)

- **What it does** (`28c898ec`): with `rt-view.flag`, at the pixels whose traced depth agrees with the raster's and whose hit has a
  material, the record's albedo, F0, gloss and shading normal against the G-buffer the raster wrote there (GB2 through its sRGB view,
  GB3 as sqrt F0, GB1.x, GB0; after the decals and the lighting, which write none of them but the decals), within 8 steps of 255 as
  stored (albedo sRGB, F0 square root, gloss) and 10 degrees; counted by programme family (`native_rt_material_check` rows, summed
  over the samples) and drawn (`rt-view-N-materials.rgba`: red albedo, green gloss, blue normal).
- **Runs** `i119-r3b-m30-on` (Theta at the High Wall) and `i119-r3b-m40-on` (Eta at Firelink), build-5, the ASUS `\\.\DISPLAY1`,
  19:31-19:34, offline, restored by hash, `rt-scene`, `rt-view`, `world-pictures`, `gbuffer-pictures`; 4,205 and 4,209 frames, no
  fault; depth agreement 95.7 % and 99.8 % (as T4); the traced sun shadow on. OBSERVED (the last rows, 36 and 38 samples):

  | Family (the largest by pixels) | Zone | Pixels | Albedo ok | F0 ok | Gloss ok | Normal ok | Mean errors (albedo, gloss steps; normal °) |
  |---|---|---|---|---|---|---|---|
  | `GXFlver_ColDifSpcBumpDOL` | m30 | 12.4 M | 0.78 | 1.00 | 0.76 | 0.80 | 5.5, 5.2, 6.8 |
  | `GXFlver_ColDifSpcBumpDOL` | m40 | 22.4 M | 0.71 | 1.00 | 0.85 | 0.78 | 6.0, 3.7, 6.6 |
  | `GXFlver_ColDifSpcBumpIbl` | m30 | 1.2 M | 0.71 | 0.99 | 0.74 | 0.73 | 6.6, 5.9, 7.7 |
  | `GXFlver_ColDifSpcBumpEmiDOLGlow` | m40 | 0.4 M | 0.60 | 1.00 | 0.73 | 0.79 | 7.3, 5.3, 6.7 |
  | `MultiBlend3_et1_lightmap`, `MultiBlend2_et1_lightmap` | m30 | 4.5 M, 1.7 M | 0 | 0 | 0 | 0.62, 0.41 | 179, 116, 11; 176, 100, 17 |
  | `MultiBlend2_et1_lightmap` | m40 | 10.3 M | 0 | 0 | 0 | 0.53 | 153, 83, 13 |
  | `GXFlver_GrassSpcBump` (alpha-tested) | m30 | 1.6 M | 0.31 | 0.99 | 0.71 | 0.87 | 20.7, 6.5, 15.0 |
  | `GXFlver_SkinSSSIbl_Bloody` (the character's skin) | m30 | 0.6 M | 0 | 1.00 | 0.99 | 0.64 | 55, 1.0, 9.2 |
  | `SAT_Equip` (the character's equipment) | m30 | 0.4 M | 0 | 0 | 0 | 0.54 | 143, 65, 12 |

- **Reading**: the reference shape holds where the census says it applies (DOL, Ibl, Emi: F0 exact, albedo, gloss and normal within
  a few steps on average; the sheets `r3b-{m30,m40}-sheet-2400.png`: the traced albedo matches GB2's on the walls, towers, statues
  and stones). The residual on those families is texel-level speckle on the differences picture (INFERRED: the level, the ray
  cone's isotropic footprint against the raster's anisotropic sampling, and the linear filter; not a data error, the mean error is a
  few steps). The families off the shape are those the census named: the map blends (white in the traced albedo: no
  g_DiffuseTexture, the generic slots of R3-PLAN §1's table), the skin's albedo curve, the equipment's wet block, `ColDif`'s
  dynamic colour; the grass's albedo (to read). They are R3c.
- **The table's writes**: at the High Wall 23 records rewritten a walk at a stable 1,912 alive (sets with a texture at its stand-in
  remade every 30 walks); now a set made again equal to its former keeps it (`RtSameMaterials`, build-6).
- **Run `i119-r3b-m45-on`** (Epsilon at m45, build-6, 19:53-19:54, the ASUS, offline, restored by hash; 3,579 frames, no fault;
  depth agreement 99.97 %): `GXFlver_ColDifSpcBumpIbl` 12.9 M pixels, albedo 0.70, F0 0.99, gloss 0.77, normal 0.77 (mean 6.3,
  5.0, 7.4°); `GXFlver_ColDifSpcBumpMulMaskIbl` 6.3 M, albedo 0.43, gloss 0.48 (its second layer, R3c); the skin and `SAT_Equip` as
  at the High Wall. The table's writes stable (1,407 records written by call 1,200 and still at 1,800: build-6's reuse). Of 5,422
  slices, 2,310 have no vertex frame the record reads (NORMAL or TANGENT not R8G8B8A8_UINT: m45's formats, to read in R3c; their
  shading normal is the triangle's).
- **I-0118's open point, the skipped rays' walks**: none in the three runs (`native_rt_walk` `skips` 0, `skip_run_max` 0 at calls
  2,400, 2,400 and 1,800; a character standing still at each place, no travel).

## Final review (2026-11-02)

- Fresh Opus, xhigh, `main...cd9d7b4a` (main merged in, `8f3ed640`): nothing high. Answered (build-8): M1 the control view broke the
  frame graph's comparison on its sampled frames (194, 202 and 123 `graph_barrier_mismatch` rows in the three runs: the third
  picture undeclared, GB0 to GB3 moved by raw barriers): the picture declared, GB0 to GB3 through the renderer's Transition before
  the pass's uses; M2 these records (the conclusion, STATE, the index, `status.json`); M3 the cost: the walk's static chunks rose
  (`static_ms_mean` 0.08 to 0.12 ms at the High Wall, 0.29 to 0.37 at m45; INFERRED the scroll row now read for every instance and
  the sets' shared counts), now recorded, and the table's own time is logged (`native_rt_materials` `encode_ms`, `encode_ms_mean`),
  its lookups made once an instance, its sweep every 16th frame (a gone set's address reused told apart at its lookup); L1 a slice
  without the reference's albedo (the map blends' generic slots, an unresolved diffuse) no longer has a material (its triangle and
  alpha test only); L2 the families' registry only with `rt-view.flag`; L3 the unreachable expired-texture branch removed; L4 an entry
  filled only once its range is held; L6 a unit test of `RtSameMaterials`. Kept: L5 the comparison's 32-bit sums (an overflow past
  about 52 degrees a family's mean at 4K: a test tool's), the skinned slice bases written each frame (as many as the skinned
  instances' slices), the step review's L7 test gaps, R3-PLAN's traced-normal picture (the differences picture's blue instead).
- Its fixes' step review (fresh Opus, high, `cd9d7b4a..f965c424`): nothing high or medium; the GB0 to GB3 transitions correct under
  E3 (the plan's own barriers, home RENDER_TARGET), the 16-frame sweep and the entry pointers safe. Answered (build-9): L-a, L-b the
  comments' reasons (a view's and an entry's weak references keep a make_shared record's address from reuse; the expired-entry branch
  defensive), L-c the sweep's period named (`kSweepFrames`), L-d no view made for an unread texture of a slice without a material, L-e
  `material_slices` counts the slices with a resolved diffuse since build-8 (the build-5 and build-6 rows counted every slice read).
  Kept: L-f the table's alive counters lag up to 15 frames after an unload (the sweep's period).
- Run `i119-r3b-fix-m40-on` (Eta at Firelink, build-8, 20:37-20:38, the ASUS, offline, restored by hash; 4,212 frames, no fault):
  no `graph_barrier_mismatch` row (M1's fix), depth agreement 99.8 % as before, the table's time 0.039 ms mean a call (`encode_ms_mean`)
  with 86 sets and 461 records alive.

## R3c, the map blends (2026-11-02)

- **What it does** (`c99cdd19`, then the step review's fixes): a map blend (MultiBlend 2, 3, 4, FourBlend; GBUF-FAMILIES §16) gets a
  material: its layers' textures found by their tiling rows (the diffuse 1 to 4, the specular 5 to 8, the bump 13 to 16, the mask 0;
  a binding's row its remap, else its register, as `ProduceModelTextureTiling` writes it; the reference roles' rows now from it too),
  layer 0 in the slice's record (no FC_DifMapMulCol, the raw gloss, UV0 not scrolled), layers 1 to 3 as records after the set's
  slices (`RtMaterialSlice::layers`, `RtRecordLayers`, `extension`), the first also the mask, the vertex weights (the last TEXCOORD
  besides UV0 the vertex programme reads), COLOR0 and P0, P1, P2 (cbMatDynParam's `*_snp_0_float_{0,1,2}_0`, the material's SPX
  numeric parameters' base words, INFERRED: their animation not read); `rt_material.hlsli` lerps the layers by
  w = saturate((W - (mask + 0.01) P0 (1 - W)) (1 + P1 COLOR0.w)). Only the decoded shape (the step and final reviews' H1): the rows
  sampled exactly those (the lightmaps apart), each layer with its specular and bump rows, exactly the three blend parameters used
  (by name and RDEF use); the other blends (`_mt1`, `_p`, `_em`, BloodBlend, Sand, the equipment's, the unlit) keep no material
  (`_e`'s body is the plain one's: caught). A blend's NORMAL and TANGENT in R16G16B16A16_FLOAT are read as is, its float UV0
  scaled by 1/2048 as its vertex programme does.
- **Runs** `i119-r3c-m30-on`, `i119-r3c-m40-on` (build-10, before the step review's fixes; 21:27-21:30, the ASUS, offline, restored
  by hash; no fault, no graph mismatch row): `MultiBlend3_et1_lightmap` (m30, 4.5 M pixels) albedo error 179 to 10.9 steps (within 8:
  0.40), F0 0.75, gloss 33 steps, normal 11.3°; `MultiBlend2_et1_lightmap` (m30 1.7 M, m40 10.3 M) albedo 9.5 and 12.2 steps, gloss 17
  and 23, normal 17 and 13°. `SAT_Equip` was caught by the loose gate (albedo error 143 to 47, wrong; H1's fix leaves it without a
  material). The reference families unchanged.
- **Step review** (fresh Opus, high, `dc919e9b..c99cdd19`): H1 the gate caught undecoded blends (the `_mt1` mask as an albedo,
  BloodBlend, the equipment): now the decoded shape only; M1 the mask's + 0.01 (every blend programme read adds it); M2 the test now
  checks the layer record and a layer's comparison; L1 no scroll for the blends; L2 a role any layer gives kept; L3 no layers without
  a material; L4 the blends' diffuse counted as named; L5 the comments. Kept: L6 P0 to P2's animation, L7 `row.count` now holding the
  layer records (harmless while the BLAS's geometries are the slices).
- **The tightened gate checked** (build-12's refusal witness `native_rt_blend_refused`, run `i119-r3c-why-m40-on`, 21:58-22:01): it
  also refused the real blends (`MultiBlend2_et1_lightmap`, `MultiBlend3_et1_e_lightmap`: their vertex programmes read TEXCOORD1 and
  TEXCOORD2); the weights now the last TEXCOORD besides UV0 (build-13). Refused rightly: `SAT_Equip`, `SAT_HairIbl2` and
  `MultiBlend2_et1_nolight_MuitiBlendMask` (no blend parameters by name).
- **Run `i119-r3c-fix2-m40-on`** (build-13, 22:20-22:21, the ASUS, offline, restored by hash; 4,215 frames, no fault, no graph mismatch
  row): 27 blend slices; `MultiBlend2_et1_lightmap` (10.4 M pixels) albedo error 12.2 steps (within 8: 0.36), F0 0.84, gloss 22.7
  steps (0.32), normal 12.9° (0.53); the reference families unchanged. Open: the blends' gloss and normal (the raw bump's blue and the
  layers' frame against GB1 and GB0), the High Wall's 3-layer blend with build-13 not run.
- **The full code review I-0120** (merged in main `c83e115e`) names three findings on this work, left for Thomas's decision: S1 (a
  slice the collect never reaches keeps zero triangle data), S6 (a set without a material whose non-diffuse role waits for its
  texture is made again every 30 walks; R3c's blends are materials now, its other families still), F1 (a BLAS postbuild slot reused
  before its build list is submitted, R2's).

## R3c's final review (2026-11-02)

- Fresh Opus, xhigh, `main...0208e85f`: H1 the gate still caught undecoded bodies (the unlit `MultiBlend*_nolight`, `_em`, the
  parallax `_p`, the equipment's `SAT_MultiBlend2_BlendOpacity_*`): now each layer's specular and bump rows and exactly three used
  blend parameters; M1 the blends' bump normal never applied (their NORMAL in R16G16B16A16_FLOAT, refused, TANGENT then not read):
  both read, each in R8G8B8A8_UINT or R16G16B16A16_FLOAT; M2 these records; L1 a role a pending base layer lacks taken from the
  first layer that has it; L3 a blend's float UV0 / 2048; L4 COLOR0 in BGRA. Kept: L2 no probe ray hits a blend slice in the unit test.
- **Runs** `i119-r3c-final-m45-on`, `i119-r3c-final-m30-on` (build-14, 23:00-23:03, the ASUS, offline, restored by hash; no fault, no
  graph mismatch row): the shape witness `native_rt_blend_shape` catches `MultiBlend{2,3,4}_et1` with and without lightmap and
  `MultiBlend3_et1_e_lightmap`, refuses `_em`, the unlit and the equipment's (`why` 40). At the High Wall `MultiBlend3_et1_lightmap`
  (4.5 M pixels) albedo 7.5 steps (within 8: 0.62), F0 0.74, gloss 16.5 steps, normal 8.3° (from 10.9, 33, 11.3 before M1);
  `MultiBlend2_et1_lightmap` 5.8 steps (0.73), gloss 9.7, normal 14.9°; `MultiBlend4_et1_lightmap` 5.9 steps; at m45
  `MultiBlend3_et1` 6.1 steps (0.70), normal 2.5°. The reference families unchanged.
- Its fixes' step review (fresh Opus, high, `0208e85f..be282423`): nothing high; `flags & 2` is D3D_SVF_USED and the gate holds against
  the census (caught: the plain, `_e`, lightmapped MultiBlend and FourBlend bodies; refused: `_em`, `_p`, the unlit, BloodBlend, Sand,
  Rotten, RockBlend, the equipment's, `_mt1`/`_mp1` by name, Fujir by row). Answered (build-15): M1 TANGENT in R16G16B16A16_FLOAT no
  longer read (every census vertex programme decodes TANGENT as R8G8B8A8_UINT; the blends' NORMAL alone is a float); L1 a TANGENT
  only beside a NORMAL read; L2 a blend's half UV0 / 2048 too; L3 the witness's bit 8 only where the parameters were looked at; L4
  the comments. Kept: L5 the new path's unit coverage (half NORMAL, scaled UVs) and the rays' walk outside the thread-affinity
  checker's scope (as before this branch).

## R3c second tranche: the blends' parameters (2026-11-03)

- **What was wrong** (OBSERVED, run `i119-r3c-final-m30-on`'s differences picture at call 2400): the blends' gloss error was not
  texel speckle but large smooth patches on the High Wall's ground (the wet stones around the bonfire, GB1.x near 0.9 in
  `i119-r3b-m30-on`'s `world-2400-gb1`), with little albedo error: a blend weight off where the layers differ in gloss. The cause
  (OBSERVED, build-16's witness): the collect looked P0, P1, P2 up among the MTD's numeric parameters by name, where none of them
  is (`by_name` -1 for every blend family), so the rays blended with P0 0, P1 0, P2 1; the raster's cbMatDynParam takes them from
  the material's generated words (22E930's emission, `generated_parameters`, from byte 0x270 = 624, the parameters' own byte).
  Their values: `MultiBlend2_et1_lightmap`, `MultiBlend4_et1_lightmap`, `MultiBlend3_et1_e_lightmap` 1, 3.15, 1;
  `MultiBlend3_et1_lightmap` 0, 8.41, 2.13 (P1 multiplies the weight by 1 + P1 COLOR0.w: the painted wet patches).
- **The fix** (`80eb8f19`, build-16): P0 to P2 from the generated words at the parameters' byte (a blend whose generated words do not
  cover them keeps no material, the shape witness's bit 64); the witness `native_rt_blend_shape` logs the parameters' byte, the
  generated words' first byte and count, P0 to P2 and the by-name lookup. Their animation (the entity's rows) is not applied
  (INFERRED: none on a map piece).
- **Runs** `i119-r3c-gloss-m30-on` (Theta at the High Wall) and `i119-r3c-gloss-m40-on` (Eta at Firelink), build-16, 00:50-00:53,
  the ASUS `\\.\DISPLAY1`, offline, restored by hash, `rt-scene`, `rt-view`, `world-pictures`, `gbuffer-pictures`; 4,200 and 4,218
  frames, no fault, no graph mismatch row. At call 2400 (before → after, the same places as `i119-r3c-final-m30-on` and
  `i119-r3c-fix2-m40-on`):

  | Family | Zone | Pixels | Albedo error, within 8 | F0 within 8 | Gloss error, within 8 | Normal |
  |---|---|---|---|---|---|---|
  | `MultiBlend3_et1_lightmap` | m30 | 4.6 M | 7.5 → 5.4, 0.62 → 0.78 | 0.74 → 0.98 | 16.5 → 4.7, 0.45 → 0.82 | 8.3° → 6.5° |
  | `MultiBlend2_et1_lightmap` | m30 | 1.7 M | 5.8 → 4.9 | 1.00 | 9.7 → 7.6 | 14.9° → 13.1° |
  | `MultiBlend4_et1_lightmap` | m30 | 0.06 M | 5.9 → 5.0 | 0.95 → 1.00 | 12.0 → 11.2 | 7.2° → 7.1° |
  | `MultiBlend2_et1_lightmap` | m40 | 10.3 M | 12.2 → 5.7, 0.36 → 0.75 | 0.84 → 0.99 | 22.7 → 6.4, 0.32 → 0.70 | 12.9° → 6.8° |
  | `MultiBlend3_et1_e_lightmap` | m40 | 0.07 M | 20.0 → 10.5 | 1.00 | 5.8 → 1.4 | 4.9° → 0.6° |

  The blends now sit with the reference families (`ColDifSpcBumpDOL` 5.5 and 6.0 steps of albedo, 5.1 and 3.7 of gloss); the
  ground's patches are gone from the gloss differences. Left: `MultiBlend2_et1_lightmap`'s normal at the High Wall (13°, 0.58 within
  10°) and `MultiBlend4`'s gloss (11 steps, a small share), not read.

## R3c second tranche: the two-layer pieces, Mul and MulMask (2026-11-03)

- **Read** (OBSERVED, the pixel programmes in the census's disassembly, the vertex programmes disassembled from the I-0063 cache
  with fxc): `ColDifSpcBumpMulDOL` and `MulEnvDOL` are one body, `MulMaskDOL` and `MulMaskEnvDOL` one body; with `MulIbl` and
  `MulMaskIbl` they blend a second layer (t7 `g_DiffuseTexture2` row 7, t8 `g_SpecularTexture2` row 8, t10 `g_BumpmapTexture2`
  row 10) at UV1 = TEXCOORD0.zw × VC_TexScroll_1 / 2048, its normal in TANGENT1's frame, by w = COLOR1.w (Mul) or, MulMask, k =
  t14 `g_BlendMaskTexture`.r × FC_BlendMaskParam.x + .z at the mask's UV, u = k > 0 ? saturate(c (1 + k)) : saturate(c + k (1 - c)),
  w = u² (3 - 2u). The mask's UV is TEXCOORD1 × VC_TexScroll_3 / 2048: its .zw in the lightmapped DOL bodies (their .xy is the
  lightmap's, with atlasUvScaleOffset), its .xy in `MulMaskIbl`. Each layer's normal is built in its own frame, the two lerped and
  then normalised; the gloss is the lerped bump blue × (g_SpecularPower / 256)². FC_BlendMaskParam (cbMatDynParam +0x30) is the
  raster's 22E930 block, {a, b, -0.5 a, 0.5 - 0.5 b} from the material's +0xD0+0x28 when its +0xD0+0x0C or +0x19 flag is set.
- **What it does** (`b49378a0`, build-17): those six bodies by name get the second layer as a record after the slice's
  (`RtMaterialSlice::mul`, `mul_mask`; `RtRecordMul`, `RtRecordMulMask`), its UV, TANGENT1, COLOR1, the mask's UV element and
  FC_BlendMaskParam's .x and .z; `rt_material.hlsli` lerps as the programmes do. The rays' instance rows carry the draw group's
  VC_TexScroll_1 and _3 (64 bytes a row). A piece missing a part keeps the single-layer reference shape. Witness
  `native_rt_mul_shape` (rt-view.flag). The `_Bloody`, `_SnowLt`, `_Cloth`, Ghost, Emi and Pom variants are left (other layers or
  programmes not read).
- **Runs** `i119-r3c-mul-m45-on` (Epsilon, 3,633 frames), `i119-r3c-mul-m30-on`, `i119-r3c-mul-m40-on`, build-17, 01:30-01:35, the
  ASUS, offline, restored by hash; no fault, no graph mismatch row. The witness: `MulMaskDOL` and `MulMaskIbl` caught at m45
  (FC_BlendMaskParam .x 2, .z -1) and at the High Wall (1, -0.5 and 16, -8). Before (`i119-r3b-m45-on`, `i119-r3c-gloss-m30-on`) →
  after, m45 at call 1800 (its run shorter), the High Wall at 2400:

  | Family | Zone | Pixels | Albedo error, within 8 | Gloss error, within 8 | Normal, within 10° |
  |---|---|---|---|---|---|
  | `GXFlver_ColDifSpcBumpMulMaskIbl` | m45 | 6.2 M | 11.1 → 8.4, 0.43 → 0.56 | 13.3 → 7.4, 0.48 → 0.63 | 8.0° → 7.1°, 0.73 → 0.77 |
  | `GXFlver_ColDifSpcBumpMulMaskDOL` | m30 | 0.35 M | 17.5 → 6.0, 0.15 → 0.72 | 23.3 → 10.4, 0.22 → 0.48 | 26.9° → 14.4°, 0.17 → 0.49 |
  | `GXFlver_ColDifSpcBumpMulMaskDOL` | m45 | 0.07 M | 7.8 → 6.8 | 7.2 → 6.4 | 11.3° → 11.2° |

  The reference families and the map blends unchanged. Left: `MulMaskIbl`'s albedo (8.4 steps against 6.3 for `ColDifSpcBumpIbl`
  at m45) and the High Wall `MulMaskDOL`'s gloss and normal, not read; no `Mul` body without the mask met in the three zones.
- **Step review** (fresh Opus, high, `7cd9282c..b49378a0`): nothing high; the programmes reproduced for the six bodies (the reviewer
  disassembled the MulMask vertex programmes again). Answered (build-18): M1 FC_BlendMaskParam was decoded a second time without the
  raster's fallbacks: now `NativeBlendMaskParam` (09_model_admission.inc) is the one producer, the admission's block 0x30 and the
  rays both call it; a value not finite, or a block unwritten while the generator's register or the generated words may reach +0x30,
  refuses the layer (`why` 32) instead of guessing 0; L1 a second texture or the mask with no bound path refuses the layer (`why` 64)
  instead of blending with the plain Mul weight; L2 the second UV's four components bounds-checked; L3 `mul` and `mul_mask` cleared
  with the layers of a slice left without a material; L4 the witness logged after the textures' resolution, by body and outcome.
  L5 (these records) done here.
- **Final review** (fresh Opus, xhigh, `main...5aaa6128`): nothing high; the programmes re-read, the vertex programmes disassembled
  again, every table number matched against the runs, I-0118's walk-skip paragraph checked. Answered (build-19, `819ce9a0`): M1 the
  rays tested only FC_BlendMaskParam's .x and .z for finiteness, the raster's block writer all four: now all four (a b not finite
  refuses the layer, the raster leaving +0x30 unwritten); L1 a refused layer's resolutions (pending state, kept textures) taken
  back; L2 every role of both layers and the mask viewable (else refused), and a second bump without a heap view leaves layer 0's
  normal and gloss; L4 the bodies matched without regard to case, as the raster finds its groups; L5 the blend witness's by-name
  lookup dropped (its answer recorded: none); L6 the comments, GBUF-FAMILIES §3 and §4's vertex side, R3-PLAN's row, the index.
  Kept: L3 P0 to P2 taken from the generated words in every case (the admission writes them only with the entity's animation rows
  available, may animate them, then runs the dynamic object's generator at register +3B8, which starts at the same byte; the rays'
  blends match the G-buffer at the reference families' level with these words, so a difference needs those cases, not seen on a map
  piece); L5's duplicated colour-format decode and the shader's repeated three-vertex reads; L7 no unit test of the Mul shading or
  of the rows' scroll1 and scroll3 (the runs' rows cover them).
- Its fixes' step review (fresh Opus, high, `5aaa6128..819ce9a0`): nothing high or medium; every point does what it says. Answered
  (build-20): 1 layer 0's specular and bump checked before the second layer resolves, and a second layer never viewable (a role
  without a path, a cube or an array) has its resolutions taken back too; 2 a MulMask whose mask has no heap view keeps layer 0
  (never the plain Mul weight). The intended change it notes: a Mul piece whose layer-0 specular or bump has no view now shows layer
  0 alone.

## R3c third tranche: the skin and the equipment (2026-11-03)

- **Read** (OBSERVED, the census's disassembly; the equipment's depth and vertex programmes disassembled from the I-0063 cache with
  fxc; GBUF-FAMILIES §17, §18): the skin bodies (`SkinIbl`, `SkinIbl_Bloody`, `SkinSSSIbl_Bloody`, their Phantom and [A] twins)
  write albedo = FC_DifMapMulCol × saturate(sqrt(t0) + FC_SkinMatchColor)², the colour PS cbModelParam +0x60 (the draw state's
  +0x280: the draw group's +0x40, or the part's appearance colour CFDE70 copies into its stack copy of the group). The equipment
  (`SAT_Equip`, `SAT_Equip_cloth`, `SAT_Equip_SSS`, `SAT_Equip_SSS_cloth`) names its textures `*_snp_0_Texture2D_N`, sampled with
  tiling row N: the diffuse row 5 (no FC_DifMapMulCol), the specular row 3, the bump row 4 whose raw blue is the gloss (no
  FC_SpcParam), a damage detail normal row 2 (by FC_DamagedNormalParam.x), the wet and blood blocks, a rim glow.
- **What it does** (`f2886de6`, build-22): a skin slice's record has `RtRecordSkin`; the rays' instance rows grow to 80 bytes with
  the instance's FC_SkinMatchColor (`NativeRtSkinColour`: the candidate's draw colour, else its group's +0x40; 0, the curve's
  identity, unread), read only for a set with a skin slice; `rt_material.hlsli` applies the curve before FC_DifMapMulCol. The
  equipment bodies by name get the reference roles from their rows 5, 3 and 4 (each bound, sampled and viewable, else no material;
  a role waiting keeps the set pending), albedo and gloss scales 1. Not applied: the _Bloody and equipment wet and blood blocks
  (neutral where the control view met them), the equipment's damage normal and rim glow; the face, eyes and hair (they read the
  colour otherwise). Unit test: the probe's quad a skin slice (its albedo the curve of the white texel with a colour of -0.5),
  its record flagged, `RtSameMaterials` told apart by the skin.
- **Runs** `i119-r3c-skin-m40-on` (Eta at Firelink, 4,189 frames) and `i119-r3c-skin2-m30-on` (Theta at the High Wall; the first
  try `i119-r3c-skin-m30-on` refused at start, `frame_begin_without_native_owner` while the shaders loaded, before any world), build-22,
  03:12-03:28, the ASUS, offline, restored by hash; no fault in play, no graph mismatch row (`skin2-m30`'s controller records an
  access violation at the exit, after `native_exit_network_stop`, `faults` 0: the exit pattern of about 30 earlier runs). At call 2400 (before: `i119-r3c-mul-{m30,m40}-on`):

  | Family | Zone | Pixels | Albedo error, within 8 | F0 within 8 | Gloss error, within 8 | Normal, within 10° |
  |---|---|---|---|---|---|---|
  | `GXFlver_SkinSSSIbl_Bloody` | m30 | 0.56 M | 55.5 → 1.1, 0 → 1.00 | 1.00 | 0.96 → 0.95 | 9.2°, 0.64 |
  | `GXFlver_SkinSSSIbl_Bloody` | m40 | 0.61 M | 55.5 → 1.1, 0 → 1.00 | 1.00 | 0.9 | 9.2°, 0.64 |
  | `SAT_Equip` | m30 | 0.45 M | none → 3.3, 0.91 | 1.00 | 3.2, 0.90 | 12.6°, 0.54 |
  | `SAT_Equip` | m40 | 0.49 M | none → 3.3, 0.91 | 1.00 | 3.2, 0.90 | 12.3°, 0.55 |

  The normal's residual on both: INFERRED the skinned instances' (their triangle's normal, no skinned vertex frame: R3a). The
  walk's and the table's times not higher (m40 `ms_mean` 0.71 → 0.61, `encode_ms_mean` 0.051 → 0.036; no measure). Measured: only
  `SkinSSSIbl_Bloody` and `SAT_Equip` met in these zones; `SkinIbl`, `SkinIbl_Bloody`, `SAT_Equip_cloth` and the `SAT_Equip_SSS`
  bodies caught by reading, not measured. Every other family unchanged but one:
  `ColDifSpcBumpMulMaskDOL` at the High Wall back to its single-layer error (17.4 steps of albedo, 26.7° of normal; 6.0 and 14.4
  with build-17): its main piece refused (`native_rt_mul_shape` why 64) since R3c's second tranche's review fixes (build-18 to 20,
  merged; no High Wall run after them).
- **The MulMaskDOL refusal's cause** (OBSERVED, build-23's witness `roles`, run `i119-r3c-roles-m30-on`, 03:45-03:46): the High
  Wall piece's mask (roles 512) and, on another slice, layer 0's specular and bump (roles 3) never had a resident texture (pending
  for good); the raster gives such a binding its fallback (`NativeFallbackTexture`: the system texture of the path's stem, else
  SYSTEX_WHITE for a texture bound with no data), the rays waited. Build-17's blend matched because it then ignored a missing mask.
- **The fix** (`bacb84a8`, build-24): the rays' texture resolution takes the same fallback (`NativeRtFallbackTexture`), not pending;
  every family's roles go through it. The witness keeps its `roles` and `pending` fields.
- **Runs** `i119-r3c-fallback-{m30,m40,m45}-on` (build-24, 04:04-04:08, the ASUS, offline, restored by hash; 4,213 frames at m40,
  3,624 at m45; no fault, no graph mismatch row): `ColDifSpcBumpMulMaskDOL` at the High Wall 17.4 → 5.9 steps of albedo (0.72 within
  8), gloss 10.3, normal 14.3° (build-17's values); at m45 the skin 1.1 steps, `SAT_Equip` 3.2 steps (gloss 3.1, normal 12.4°);
  `MulMaskIbl` at m45 unchanged (8.3 steps: not this cause, still open). Not as before (the final review's H1): `GXFlver_ColDif`
  (12 slices, 49,707 pixels at the High Wall) took a material, its t0 now resolved by the fallback: 254 steps of albedo, its
  programme writing t0 × FC_DynamicDiffuseColor, not the reference's FC_DifMapMulCol (answered below).
- **Step review** (fresh Opus, high, `c45332dc..069abd8e`): nothing high; the row layout agrees on every side (C++, `RtRowOf`, the
  three shaders, the test's 40-word view), the skin curve and colour source match the programmes and the raster, the twins are
  recipe variants of the same groups, the equipment's take-back right. Answered (build-25): M1 the white fallback (a texture bound
  with no data) keeps the set pending, tried again every 30 walks, as the raster rebinds a texture that arrives later (the stem's
  system textures stay final); L1 the alpha test takes the same fallback; L3 the comments; L4 the witness's `pending` is the slice's
  own. Kept: L2 UV0 scaled by VC_TexScroll_0.xy where the vertex programmes add only its .zw (every family's, since R3a; the rows'
  .xy read 1, not verified), L5 the fallback's rule written twice (the raster's and the rays', a set's build only). Run
  `i119-r3c-step-m30-on` (build-25, 04:32-04:33, the ASUS, offline, restored by hash): the skin 1.1, `SAT_Equip` 3.3,
  `MulMaskDOL` 5.9 steps, the table's writes as build-24's (10,236 against 10,142 at call 2400); a final review ran meanwhile,
  so its times are not read.
- **Final review** (fresh Opus, xhigh, `main...7179e2de`): H1 above: now the reference shape only where the G-buffer programme
  uses FC_DifMapMulCol (cbMtdParam's D3D_SVF_USED); ColDif, ColSh, Col (FC_DynamicDiffuseColor), EmiGlow (FC_EmissiveColor ×
  FC_GlowScale) and Veill_Curtain (cbModelParam's FC_DifColMul), 12 census programmes counting their twins, keep no material until
  decoded (build-26; run `i119-r3c-final-m30b-on`, 04:53-04:54: no ColDif row, the other families as before, 4,649 material
  slices against 4,669). Answered in these records: L1 the exit fault named, the gloss's
  rounding, the times, the normal's residual INFERRED; L2 the step review's range; L3 the evidence; L5 the bodies measured or read;
  L6 the white fallback drawn as the raster draws it (the original's own binding until the texture arrives, not a stand-in of
  ours); L10 STATE. Kept: L4 build-25's alpha-test fallback measured by no shadow run (it follows the raster's DptA); L7 the
  fallback's rule and the equipment's row map written twice; L8 no GPU check of a non-skin slice with a colour (the flag's test
  only); L9 the equipment's alpha-tested twins opaque in the traced shadow (their DptA samples t1, row 5, which the material now
  resolves: a next step).
- Its fixes' step review (fresh Opus, high, `7179e2de..efca2840`): nothing high; the gate keeps 396 of the 408 census programmes
  naming g_DiffuseTexture and refuses exactly the 12. Answered (build-27): M1 the hair, eyes and face (FC_SkinMatchColor used, not
  a skin body; 38 census programmes) had the reference shape since R3a, as wrong: now no material until decoded too, the gate
  decided before the roles are resolved (L2, L3: no tally or resolution for a refused slice); L1 the families' colours named; L5
  the count. Runs `i119-r3c-gate-{m30,m45}-on` (build-27, 05:08-05:11, the ASUS, offline, restored by hash; 3,623 frames at m45; no fault, no graph mismatch row): every family
  as with build-24 and build-26 (the skin 1.1, `SAT_Equip` 3.2 and 3.3, `MulMaskDOL` 6.0, `MulMaskIbl` 8.3 steps); 4,644 material
  slices at the High Wall (5 fewer). Build-27 also refused every programme with a g_DiffuseTexture2 but the Mul bodies; its step
  review (fresh Opus, high, `efca2840..981625594`, nothing high) counted 172 such programmes, the `_Df` back-face sets among them
  (exact on their front faces): that condition taken back (build-28), the others (the undecoded Mul variants, EmiMul, Pom, Burn,
  DirtMask, water, Eclipse) keeping their layer 0 as the reference's, a known approximation. Build-28 gives them build-26's
  behaviour back; not run.

## R3c fourth tranche: MulMaskIbl's bias, the hair, the equipment's alpha test (2026-11-03)

- **MulMaskIbl read again** (OBSERVED, the census's `GXFlver_ColDifSpcBumpMulMaskIbl_Gbuf.ppo` and the I-0063 cache's `_GBuf` and
  `_GBuf_Ins` vertex programmes disassembled with fxc): UV0 = TEXCOORD0.xy × VC_TexScroll_0.xy / 2048 + .zw, UV1 = TEXCOORD0.zw with
  VC_TexScroll_1, the mask's UV TEXCOORD1.xy with VC_TexScroll_3; the albedo lerp(t0 row 0, t7 row 7, w) × FC_DifMapMulCol, w the
  smoothstep of the mask's curve on COLOR1.w; the gloss lerp(t3.b, t10.b, w) × (FC_SpcParam.x / 256)². The rays' path is that
  programme; at m45 (`i119-r3c-gate-m45-on`) its normal is within 10° as often as `ColDifSpcBumpIbl`'s (0.77) while its albedo and
  gloss are not (0.56 and 0.63 within 8): a weight off would move the normal too.
- **The control view's bias** (build-29): each family's mean signed albedo error by channel and signed gloss error (traced less
  raster, steps; texel speckle cancels there, a scale or a weight off does not), and the share of pixels whose second-layer weight
  is at least one half with their mean albedo error (`native_rt_material_check`'s `albedo_bias`, `gloss_bias`, `layered`,
  `layered_albedo_error`); 16 words a family in the counts.
- **Runs** `i119-r3c-bias-m45-on` and `i119-r3c-bias-m30-on` (build-29, 06:00-06:02, the ASUS `\\.\DISPLAY1`, offline, restored by hash;
  2,326 and 2,841 frames, no fault; the m30 run's looks refused, the Claude app in the foreground: the control view unaffected).
  OBSERVED: `MulMaskIbl` at m45 a mean albedo bias of -0.4 to -0.7 steps by channel (its error 7.8, `ColDifSpcBumpIbl`'s 6.2 with
  -0.4), the pixels led by the second layer (19 %) at 8.8: no scale or weight off; its residual is spread (INFERRED: two layers and a
  mask filtered, the mask's curve steepening the differences). The bias showed what the means hid: the alpha-tested families darker
  in the rays, `GrassSpcBump` by 13 steps, `ColDifSpcBumpDOL|o|128` by 4, `TrIbl` by 2.4; the reference families within 0.3.
- **The texture level** (`rt_material.hlsli`, build-31): the ray cone's footprint was its major axis (width / cos, floored at 0.05),
  the raster's anisotropic sampler takes the minor axis unless the major is more than its anisotropy times it; now the latter with 16
  (DS3's Texture Quality MAX: ×16, INFERRED from the options sheet). Grazing surfaces were sampled blurrier than the raster, and
  where transparent texels are dark (the alpha-tested cards) blurrier meant darker.
- **Runs** `i119-r3c-aniso-{m30,m45,m40}-on` (build-31, 06:25-06:28, the ASUS, offline, restored by hash; 2,923, 2,403 and 3,029
  frames, no fault). Every family closer (before: the bias runs, at Firelink `i119-r3c-fallback-m40-on`; the same calls):

  | Family | Zone | Albedo error, within 8 | Gloss error | Normal |
  |---|---|---|---|---|
  | `ColDifSpcBumpDOL` | m30 | 5.08 → 4.39, 0.81 → 0.86 | 5.51 → 4.36 | 7.1° → 5.5° |
  | `ColDifSpcBumpDOL` | m40 | 5.97 → 4.26, 0.71 → 0.83 | 3.67 → 2.33 | 6.6° → 4.1° |
  | `MultiBlend3_et1_lightmap` | m30 | 5.63 → 4.25 | 4.07 → 2.39 | 5.6° → 4.4° |
  | `MultiBlend2_et1_lightmap` | m40 | 5.70 → 4.38 | 6.44 → 5.13 | 6.8° → 5.4° |
  | `ColDifSpcBumpIbl` | m45 | 6.16 → 5.25 | 4.66 → 3.09 | 7.2° → 5.5° |
  | `ColDifSpcBumpMulMaskIbl` | m45 | 7.82 → 7.10, bias -0.5 | 7.35 → 6.14 | 6.2° → 5.3° |
  | `ColDifSpcBumpMulMaskDOL` | m30 | 7.44 → 5.36 | 10.2 → 8.4 | 12.0° → 10.7° |
  | `GrassSpcBump` | m30 | 21.5 → 18.9, bias -13.1 → -9.7 | 6.9 → 6.7 | 23.1° → 22.8° |
  | `SAT_Equip` | m30 | 3.47 → 2.88 | 3.41 → 2.86 | 12.8° |
  | `SkinSSSIbl_Bloody` | m30 | 1.08 → 0.87 | 0.88 → 0.62 | 9.5° |

  The grass still 9.7 steps darker (INFERRED: the wind the rays do not apply, R2's open point, or the alpha test at the top mip
  where the colour takes the cone's level; not told apart). The equipment's alpha test: at Firelink the skinned instances'
  alpha-tested slices not reproduced 114 → 91 (`native_rt_skin` `alpha`), 23 equipment slices now cut in the traced shadow.
- **The hair** (the pixel programmes read 2026-11-03: `GXFlver_HairIbl`, `HairIbl2`, `HairIbl_Bloody`, `HairIbl_Bloody_Cloth`, the
  `HairSSSIbl_Bloody` bodies; the vertex programme `HairIbl_GBuf_Skin`): the reference shape with saturate(sqrt(t0) ×
  FC_SkinMatchColor)² as the albedo (the skin's curve multiplied, not added), the _Bloody and HairIbl2 wet blocks not applied (as
  the skin's); `RtRecordHair`, the instance's colour row as the skin's. The witness characters' hair is `SAT_HairIbl2` (the family
  names of `i119-r3c-aniso-m45-on`; its pixel programme read, `SAT_HairIbl2_cloth` the same body): its textures by rows as the
  equipment's, the diffuse row 0 (t3) with the hair's curve and no FC_DifMapMulCol, the specular row 2 (t1), the bump row 1 (t2) whose
  raw blue is the gloss, the wet and blood decal blocks (t41 by FC_BloodAmount) not applied; the equipment's row table now holds both.
- **The equipment's alpha test** (the final review's L9; SAT_Equip's DptA programmes read: t1 `SAT_Equip_snp_0_Texture2D_5`'s alpha at
  UV0 tiled by row 5, against FC_AlphaRef as T3's shape): a DptA sampling one texture at a register other than t0 is now T3's shape
  when that texture is the material's diffuse (its tiling row the material's); before, opaque in the traced shadow.
- **The face** (`GXFlver_FgFaceSSSIbl_Bloody` the witness characters'; its pixel programme and `_GBuf_Skin` vertex programme read,
  `FgFaceIbl` and `FgFaceIbl_Bloody` too): a Mul piece whose second diffuse t7 is the composed FaceGen image (the binding FG_face_gen,
  bound by the raster as NativeComposedFace of the candidate's face owner), at UV1 = TEXCOORD0.zw (VC_TexScroll_1), lerped by COLOR0.w
  (the vertex programme passes COLOR0.w as the pixel programme's COLOR1.w), the skin's curve on t0 before the lerp (and on t7 in the
  _Bloody bodies), the lerped albedo times COLOR0.rgb, then FC_DifMapMulCol; the F0 and gloss lerped as Mul's; the wet block and the
  blood decal (t41, t42 by FC_BloodAmount) not applied. `RtRecordFace`, `RtRecordFaceBoth`; a face not composed yet keeps its set pending
  (tried again every 30 walks), a composed one is final (a recomposition withdraws the record the set holds, which remakes it); a
  face refused keeps no material.
- **Left**: the eyes (the colour by t0.a at a parallax UV from the bump's alpha), no material (86 pixels at the High Wall).

- **Step review** (fresh Opus, high, `main...6a030f516`): nothing high; the layout agreed on every side, the hair's curve and the
  equipment's DptA matched their programmes. Answered (build-33): M1 the anisotropy held at 16 against D-0151: now the raster's own,
  the material bank's settings rows (12, 13: Texture Quality's filter and anisotropy, 1 when not anisotropic) with their mip bias
  (DLSS's, NativeMaterialBank: -1 with DLAA at 1280x720, `native_material_mip_bias`), carried by NativeRtSceneInput into the view's
  constants; L1 a DptA at another register needs a bound path there before any resolution (else unsupported, not pending); L2 the
  hair's colour unread its identity 1 (the row's w -1), not black; L3 the comments. Kept: L5 no GPU probe of the hair's curve nor of
  the count words 8 to 13 (the runs read them).
- **Runs** `i119-r3c-tour-{m40,m30,m45,m31,m37,m50}` (build-33, 07:00-07:07, the ASUS, offline, restored by hash; 2,294 to 3,026
  frames, no fault; `Run-R3ZoneTour.ps1` in `evidence/I-0119/artifacts/scripts`; looks refused at m45, the Claude app in the
  foreground). Between build-31 and build-33 the rays' anisotropy stayed 16 (the options rows: the Texture selector 4, its profile
  {3, 16}) and the bias -1 came in, so the change below the aniso runs' is the raster's mip bias alone (OBSERVED, the two runs' rows;
  build-31's 16 was the right value by chance). At the last call, every compared pixel together:

  | Zone | Pixels | Albedo error, within 8 | The largest families | The worst (>1 %) and its bias |
  |---|---|---|---|---|
  | m40 Firelink | 16.5 M | 2.94, 0.89 | `ColDifSpcBumpDOL` 2.9, `MultiBlend2` 3.2, `EmiDOLGlow` 2.6 (7.2 with build-31) | `MultiBlend2` 3.2, -0.2 |
  | m30 High Wall | 9.9 M | 3.02, 0.91 | `ColDifSpcBumpDOL` 1.6 (4.4), `MultiBlend3` 3.7 | `GrassSpcBump` 11.7 (18.9), -1.7 (-9.7) |
  | m45 Ariandel | 0.9 M | 3.42, 0.87 | `ColDifSpcBumpIbl` 3.9, `MulMaskIbl` 2.4 (7.1) | `ColDifSpcBumpIbl` 3.9, -0.4 |
  | m31 Undead Settlement | 15.1 M | 1.82, 0.95 | `ColDifSpcBumpDOL` 1.7, `MultiBlend2` 1.4 | `GrassSpcBump` 5.5, -0.7 |
  | m37 Anor Londo | 10.1 M | 2.75, 0.91 | `MulMaskDOL` 3.0, `ColDifSpcBumpDOL` 1.5 | `MulMaskDOL` 3.0, 0.0 |
  | m50 Dreg Heap | 3.3 M | 4.13, 0.84 | `MultiBlend2` 6.8, `ColDifSpcBumpDOL` 2.6 | `MultiBlend2` 6.8, -0.9 |

  The skin 0.2 steps, `SAT_Equip` 1.1; the face (`FgFaceSSSIbl_Bloody`) caught (`native_rt_mul_shape` why 0) and compared on its
  few pixels (125 at Firelink: 3.9 steps). Not compared: the hair, `SAT_HairIbl2` a forward transparent pass (engine pass 5), which
  the view's rays pass over by T5's rule; the sky; at the Dreg Heap a large surface drawn pink (no material: a family to identify).
  Sheets `evidence/I-0119/artifacts/r3c4-tour-sheet-{1,2}.png` (the presented frame and the traced albedo), shown to Thomas.
  The face's residual (Firelink, 125 pixels at 3.9 steps): a hue bias (R -3.4, G -1.5, B +1.5) where layer 0 leads (70 % of its
  pixels, about 5.1 steps; 1.3 where the face image leads), not read (OBSERVED, about 190 face pixels over the six zones).
- **Final review** (fresh Opus, xhigh, `main...2461c3048`): nothing high; every programme re-read with fxc (the hair bodies,
  SAT_HairIbl2, the three face bodies, their vertex programmes, the SAT DptA programmes; of 318 DptA programmes the new rule
  accepts SAT_Equip t1, SAT_Equip_SSS t2 and SAT_HairIbl2 t3, each testing t.w at TEXCOORD0 with the material's row), the layouts
  and the tour's numbers matched. Answered (build-34, `c8ca742e`, and its step review's points after it): M1 a face whose image is
  not composed yet now keeps its set pending as an albedo waiting (it was dropped at the slice's end: a face first met before its
  composition waited for the 600-walk revalidation), only when no shape bit refused it; L1 a composed face no longer pending; M2 a
  witness `native_rt_rows_shape` (rt-view.flag: the body, the rows' registers, the material taken, the slice's own waiting) for the
  by-rows path, and the hair's record corrected: `SAT_HairIbl2|o|128` is also an opaque alpha-tested G-buffer slice, which the rays
  take with build-33 (at Firelink the skinned instances' unsupported alpha slices 91 → 1 at call 1200), yet no hair pixel was
  compared in the nine runs (the run below); M3 « Suggested next step » no longer says R3c complete; L2 the comments and records; L3 a face
  whose image has no heap view keeps layer 0's skin curve; L4 the face's residual above; L5 the bias's attribution; L6
  `RtSkinCurve`. Its fixes' step review (fresh Opus, high, `2461c304..c8ca742e`): nothing high; answered: the records (this),
  the face's pending only for a role waiting, the witness's `pending` the slice's own, two comments.
- **Run** `i119-r3c-final-m40` (build-34, 07:50-07:51, the ASUS, offline, restored by hash; 2,975 frames, no fault): the witness
  shows `SAT_HairIbl2`, `SAT_HairIbl2_cloth`, `SAT_Equip` and `SAT_Equip_cloth` taking their material by rows (registers 3, 1, 2 and
  1, 3, 2), the face caught (20 pixels, 2.4 steps), the skinned unsupported alpha slices at 1; still no hair pixel compared, and the
  traced picture shows the head without hair (the hair's slices not met by the view's rays: INFERRED its alpha test at the top mip
  or T5's pass-over of a mixed set; not read). Build-34's last two points (the face's pending, the witness's field) not run.
- **Merged** `6415598a` (build-35's tree; the launcher rebuilt from `main`, 114 tests passed; start check `start-6415598a` passed,
  08:20-08:21, 343 world frames, the RT self-test passed, restored).

## Evidence

- The programmes: `C:/Users/User/.codex/I0063-cache/programs` (outside Git, read-only); the reference disassembly
  `evidence/I-0068/queries/variants/gxflvershader__GXFlver_ColDifSpcBumpDOL_Gbuf.ppo.asm`; the census's disassemblies, evaluator
  and classes `evidence/I-0119/artifacts/census/`.
- Builds `evidence/I-0119/artifacts/build-3` (R3a), `build-5` (R3b and the step review's fixes; the m30 and m40 runs' module),
  `build-6` (the equal sets kept; the m45 run's), `build-7` (main merged in), `build-8` (the final review's fixes); R3c's second tranche `build-16` (the blends'
  parameters), `build-17` (Mul and MulMask), `build-18` (its step review's answers), `build-19` (the final review's), `build-20` (its fixes' step review's; its log names
  the source commit); their logs beside. Runs `i119-r3c-gloss-{m30,m40}-on`, `i119-r3c-mul-{m45,m30,m40}-on`, `i119-walk-skips-m40`.
- R3c's third tranche: builds `build-21` (the skin), `build-22` (the equipment), `build-23` (the Mul witness's roles), `build-24`
  (the fallback), `build-25` (the step review's answers), `build-26` (the final review's), `build-27` (its fixes' step review's), `build-28` (the last step review's); runs `i119-r3c-skin-{m30,m40}-on`,
  `i119-r3c-skin2-m30-on`, `i119-r3c-roles-m30-on`, `i119-r3c-fallback-{m30,m40,m45}-on`, `i119-r3c-step-m30-on`, `i119-r3c-final-m30b-on`,
  `i119-r3c-gate-{m30,m45}-on`; their log
  `evidence/I-0119/artifacts/runs-r3c3.log`.
- R3c's fourth tranche: builds `build-29` (the bias), `build-30` (the hair, the equipment's alpha test), `build-31` (the anisotropic
  level), `build-32` (the face, SAT_HairIbl2), `build-33` (the step review's answers), `build-34` (the final review's), `build-35` (its step review's); runs `i119-r3c-bias-{m45,m30}-on`,
  `i119-r3c-aniso-{m30,m45,m40}-on`, `i119-r3c-tour-{m40,m30,m45,m31,m37,m50}`; their log `evidence/I-0119/artifacts/runs-r3c4.log`.
- Runs `evidence/I-0066/artifacts/i119-r3b-{m30,m40,m45}-on`; sheets `evidence/I-0119/artifacts/r3b-{m30,m40}-sheet-2400.png` (made
  by `evidence/I-0119/artifacts/scripts/Make-MaterialSheet.ps1`).

## Closed (2026-11-09)

Closed 2026-11-09 with Thomas's yes (relayed by the coordinator, 01:55 Paris): the open points below are carried to [I-0132 R11-PLAN §6](../I-0132-rt-optimisation-zones/R11-PLAN.md#6-the-open-points-carried-from-the-investigations-closed-on-2026-11-09), where each has its place.

## Uncertainty / unresolved questions

- The reference families' residual (a few steps on average, texel speckle): INFERRED the level and filter, not measured apart.
- FC_EmissiveColor not read (white); the scroll on float UVs (step review L6); m45's vertex frames in other formats (2,310 slices
  shaded with their triangle's normal); the skinned instances' shading normal (their triangle's, no skinned normals).
- The rays' skipped walks seen at rest only (none in three runs); in play with travel not yet.

## Suggested next step

R3c's fourth tranche is complete. Left in R3c with no material (not compared by the control view, so their pixel shares are not
measured): `Col`, `ColDif`, `ColSh`, `EmiGlow`, `Veill_Curtain` (other colour sources), the eyes (the colour by t0.a at a parallax
UV), the `_mt1`, `_p`, `_em`, unlit, BloodBlend and Sand blends, the Dreg Heap's pink surface's family (to identify). With layer 0
alone as the reference's (compared under their own families): the Mul `_Bloody`, `_SnowLt`, `_Cloth`, Ghost, Emi and Pom
variants, `DirtMask`, Pom, SnowLt. The wet and blood blocks are not applied (neutral where the control view met them). The hair
(`SAT_HairIbl2`) takes its material but is not met by the view's rays: read 2026-11-03 (the `native_rt_forward_only` rows of
`i119-r3c-final-m40`), the hair models `HR_A_*` have classes 0x82 (transparent and forward: the rows 0x1F and 0x29, none in the
G-buffer), so the view's rays pass them over by T5's rule, as designed; what the rays miss is their shadow (class 2 is in the
original's cascade row, class mask 0x107; the rays' casters take G-buffer classes only, R5's open « translucent casters »). Next for the RT line: the grass, darker in the rays at the High Wall (11.7 steps,
bias -1.7), moved by its wind since I-0117 T6 (7.0 steps, bias -0.8; I-0117 RESULT « T6 »), then I-0117's fault under PIX, then R3's Fresnel and R4
(RT-PT-PLAN §12 risk 9), the remaining families as R4's views need them.
