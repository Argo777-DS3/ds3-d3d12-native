# Sun shadows and SSAO in wave 1 (D-0180): implementation route

Thread « Tests des menus » (pilot, 2026-10-16). Thomas moved the screen-space sun shadow and the SSAO into wave 1
(D-0180) after the image regression measurement ([IMAGE-REGRESSION-STATIC-LEADS](IMAGE-REGRESSION-STATIC-LEADS.md) §0b:
the native lit HDR at m40 is 4.6 times the original's before every YEBIS pass, 0.6-0.9 without the sun). This note is
the route and the log; the specifications stay in [SHADOWS-WAVE1-READY](SHADOWS-WAVE1-READY.md),
[SHADOWS-NATIVE](../I-0068-static-yebis-ffx-lights-spec/SHADOWS-NATIVE.md), [SSAO-WAVE1-READY](SSAO-WAVE1-READY.md) and
[SSAO-NATIVE](../I-0068-static-yebis-ffx-lights-spec/SSAO-NATIVE.md).

## 1. Done

**Step 1, SSAO and the mask (commit 796e96f7).** The AO stage records the original chain (SSAO-NATIVE §1 rows 1-6):
`DepthScale2x2OutputDepth` into the half depth, the `ScreenSpaceAO2` variant of `PlanAo` (WithNormal at HIGH), `SeparatedReProj`
with the ping-pong history, `BilateralUpScale` into the full-resolution output (t33). Its blocks are the AO-3 cores on native
inputs (the scene camera's view and projection rows, the layer record, the half-resolution AO size, the full height,
`fDepthClipRange`), the rotation image is `NativeCommonImages::dither[counter & 15]`. The mask t32 (RGBA8, the AO stage's) is
cleared to white and written by `GXRend_CompositeShadowMask` twice (10CC20, read: reference 0 with the pair of 10CDD9
{(0,0,1,0), (1,1,0,1)}, then reference 0x10 with the pair of 10D01E {(0,1,0,0), (1,0,1,1)}; stencil read mask 0x10 EQUAL on
the read-only depth view; RGB only, because the cascaded shadow writes its term in w and the composite keeps it). The
lighting views name the mask (every pass block's t32) and the AO output (t33) once per view heap, view generation and stage
generation, after a drain. Shader facts read for this (I-0063 cache, disassembled): `GXLightDeferredAO` multiplies by
`t32.x * t32.y`; `GXLightAcc_DDirLight1` reads `t32.x` and `t32.z`; `DDirLightShadowed1` adds `t32.w` blended toward
`FC_ShadowColor`; `GXScreenSpaceCascadedShadow_Soft` writes the shadow in `w` (packed depth in xyz); the blur keeps xyz and
blurs w. Native1720 (m40): no fault; lit HDR 3.44 times the original (4.58 before), floor 4.6 (6.3).

## 2. Next: the cascaded sun shadow

What the original does (read for this step): `FUN_1401B6020` (per cascade) builds the cascade pass's culling volume and sets
it as the pass's culler (`pass+0x410`): the light-space volume of `FUN_14023C1E0`, replaced for the cascade passes
(`pass+0x124` in 1..4) by `FUN_1401B4830(obj+0x570+0xE0 i, fit outputs)` built from the cascade's fit (the orthographic box).
The main view's culler (vtable `143D42D58`) is six planes at `+0x160` and a world AABB at `+0x220`
(`FUN_14006F310`: AABB overlap by `FUN_14001FFF0`, then the six planes; 0 outside, 1 crossing, 2 inside). The cascade passes'
casters are drawn by the model draw `FUN_140057A20` with the default row of `FUN_14005C060` (class mask 0xFFFF) and the
material's `Dpt` (shader pass 2) or `DptA` (3, alpha-tested) programmes: `Dpt` VS reads `cbMatrixWVP` (+64 the view-projection),
`cbInstanceData`, `cbObjMatrix`, `cbSceneParam` (`VC_ClipPlane` only), its PS is empty; `DptA` PS is the alpha test with the
diffuse, dither, tiling, `cbMtdParam.FC_AlphaRef`, `cbModelParam.FC_DitherEnable`.

Native route, each step built, tested and witnessed live before the next:

| # | Step | Witness |
|---|---|---|
| C1 | Caster selection: a second walk of the world query with a culler copied from the main view's and given a volume that holds the four cascade boxes; each admitted model kept for the cascades whose box its bounds cross | counts per cascade (row), cost |
| C2 | Rows and recipes: the shadow row of 57A20/5C060 for the casters, recipes `Dpt` / `DptA` (VS only for Dpt) | recipes resolved, gates |
| C3 | Draws into the atlas: per cascade a view block (`cbMatrixWVP` from the cascade's view-projection), the depth-only envelope on D16 with the cascade rasterizer (slope bias R+0x6C), the quadrant viewport | atlas readback |
| C4 | The mask: `GXScreenSpaceCascadedShadow_Soft` into the mask, the blur passes, the composite (step 1) on top; the sun `DDirLightShadowed1` (done, §3) | lit HDR against the original, side by side |

## 3. Done: C1 to C4, and the light sets (2026-10-16)

- **C1-C3** (commits d38ea07a, ab2e65bd): the caster walk, the `Dpt`/`DptA` rows (57A20 preparation 2, class mask 0x107) and
  their draws into the D16 atlas per cascade. Native1726 m40: 751 caster rows, 0 graph mismatches.
- **C4** (`native/renderer/sun_shadow_stage.*`): `GXScreenSpaceCascadedShadow_Soft` on the stencil bit 0x08 (`OutputColor`
  fill elsewhere), the blur passes alternating between the second target and the mask so the last lands in t32, then the
  composite; the sun takes `DDirLightShadowed1` with cbSceneParam at b8 (`FC_ShadowColor`). cbPerScene (336 bytes) is
  transpose(inverse projection), transpose(camera pose x atlas matrix n) and the select distances; cbBlurParam from the
  executable's twelve taps; samplers from banks 0, 3, 15. Meshes whose material has +0xD1 set are not drawn into the atlas
  (FUN_140140680's refusal; 0 such meshes at m40). Witnesses (`world-pictures.flag`): the mask, the depth plane and the atlas
  are written beside the picture once.
- **Shadow checked** (Native1729/1730, scratch CPU check of the receivers against the atlas): the atlas holds the scene seen
  from the sun, and the floor around the m40 bonfire is genuinely in the shadow of the shrine walls (the receivers' cascade
  depth is 0.1-0.2 beyond the stored occluders in cascades 1-2): the shadow is correct, not a depth defect.
- **The remaining excess was the light set index.** `DDirLightShadowed1` lights a pixel with up to four directional lights
  of the set `cbLight.FC_DirLight[GBuffer3.w x 255]`; the shadow only scales light 0. The Gbuf programmes write
  `cbInstanceData.envData / 255` into GBuffer3.w. Reference1736 (`light-observe.flag`, `ObserveInstanceRecord` on 5B3D0):
  5B3D0 writes envData for pass state type 6 with values 0-3 (m40: 43,200 / 270,842 / 228,009 / 14,250 records), the
  index of the group's key block in 137EF0's entries (decompiled: deduplication of the eight per-object source lists by the
  ten key words, at most 225 entries, then each source entry's +4 set to its index); 137910 builds cbLight/cbFog for every
  entry. Reference1734: the original's set 0 record equals the native one byte for byte; Reference1731: entries
  [100], [40], [0], [31]. The native wrote envData 0 everywhere, so the m40 floor took set 0, whose second light
  (0.71, 0.85, 1) x 0.79 is the fill the original's floor (sets 1, 2: intensity 0 and 0.1) does not receive.
- **Fix**: `NativeLightSetIndex` gives each model's envData its key block's index in the collect's list (`light_keys` of the
  scene, admission order, at most 225 as 137EF0, index 0 past it, logged); cbLight and cbFog hold a set for every listed key
  (root descriptors, the 80 KB and 32 KB ranges). Native1737 m40 against Reference1705 (`Compare-SceneHdr.ps1`, frame 2400):
  whole 1.03 (3.43 before), floor 1.01 and 1.00, wall 1.23, arch 1.07, character 0.98. Still different: the bonfire's fire
  (effects) and the wall (1.23). (A first version kept a registry across frames; the light review found that a restart
  inside a collect, likely in gparam blend zones where the key's t changes, would mismatch envData and the sets.)
- **Fog map position.** With every set in use, m31 and m45 went under an opaque fog (Native1738/1740). Reference1743
  (`reference_fog_set`, the engine lookup 38E70 per entry) against Native1744 (`native_fog_set`): the records are equal except
  +0x4C..+0x57, the map position the height fog is measured from, which the original holds as (50, -155, -580) at m31 and the
  native left zero (no MapOffset listing is published natively). It is the translation of the area's kind-7 binding
  (m31_00_0010: rows {r0, 50}, {r1, -155}, {r2, -580}); `Banks::Fog` now takes it from the bound area, else the listing (INFERRED rule, one zone observed).
- **Area layers.** m45 binds two kind-7 layers, m45_00_0000 then the partial overlay m45_00_0200; the native read only the
  latest, so the sets 1, 8, 10, 100 and 101 fell back to the default record {1, 0} (Reference1751 `reference_light_set`
  against Native1752: every word different). `Banks::SetAreaLayers` keeps every live layer, latest first, and a lookup takes
  the latest layer that holds the value ID (INFERRED rule, the same as W10 YB-3's fallback to the area's own file).
  Native1753: the six m45 sets equal the original's word for word.

### Zone witnesses (eclipse sky, Shebang zone saves, `Compare-SceneHdr.ps1` frame 2400 against the zone's reference capture)

| Zone | Native run | Whole | Regions (min-max) | Note |
|---|---|---|---|---|
| m40 Firelink | 1750 | 1.03 | 0.96-1.23 | the bonfire's fire (effects) |
| m37 Cathedral | 1749 | 1.10 | 0.89-1.69 | |
| m31 Village | 1745 | 0.73 | 0.51-0.77 (quarter bands) | |
| m50 Anor Londo | 1748 | 0.67 | 0.39-1.37 | |
| m45 | 1753 | 0.74 | 0.88-1.29 | |
| m30 High Wall | 1758 | 1.25 | 0.95-3.41 | was 2.03 (Native1754): the lightmapped map pieces were not drawn (below); the floor and walls still lack part of the lightmap tint |

**m30 map pieces (Native1757/1758).** 2,585 of m30's 2,745 prepared rows were dropped at encoding (`missing_constant_mask`
bit 14: PS b0 `cbTexHdrScaleParam` of the `DOL` lightmapped families), the block being left unproduced when the lightmap
`g_DOLTexture1` carries no float metadata or is not delivered. The game uploads (1, 1, 1, 1) then (old host rule, I-0063):
the native now does the same (a lightmap whose content is not there yet is produced per call, never kept). Native1758:
1,148 draws (160 before), no missing constant; lit HDR 1.25 of the original (2.03), castle, walls and floor drawn. Open: the
floor and walls are flatter than the original (floor-left 3.4): whether their lightmaps are delivered with their metadata.

## 4. On by default, cost (2026-10-16)

The sun shadow and its cascade fit are on by default (`no-sun-shadow.flag` opts out; `shadow-fit.flag`/`sun-shadow.flag`
still force them). Measure runs (no instrumentation, no debug layer): m40 60 frames/s (Native1761); m30 56.3 (Native1762;
59.9 without, Native1763; the collect 12.3 ms against 7.6: shadow walk about 2.2 ms, caster rows in the passes +1.4 ms);
m31 55.3 (Native1768); m45 22.4 (Native1767; 30.5 without, Native1769: m45 is slow without shadows too, collect 26 ms).
The native redraws every cascade every frame with all casters (the recommended native contract of SHADOWS-NATIVE 1.4); the
original keeps a cascade while the camera slice stays inside its previous volume and redraws rarely (Reference1771 at m40:
D04B90, the map piece emit, called for passes 6-8, 0x39 and 0x45 only during 150 s). The cascade passes traverse with the
category bit 0x40 (C2EC0), which no native entity carries (Native1769: the walk with 0x40 visits nothing; the original's map
pieces carried 0x03/0x13 in Reference1771): the native walk keeps the main view's bits, an OPEN difference. Next: the cascade
cache by mechanism (§1.4) and the m45 collect cost.

## 5. Full review and final pictures (2026-10-16)

Full review (D-0181) applied: a light or fog set whose record fails takes set 0's block (logged `native_light_sets_failed`)
instead of a zero set; shadow-only admissions add no set; `cbTexHdrScaleParam` without a lightmap binding is shared per
programme recipe and stage; the blur pass count is clamped to the plan key's three bits at the frame input; the census
diagnostics run only with `light-observe.flag`; `SetAreaBindings`/`BindArea` drop stale layers. envData in records shared by
other passes: no Fwd, Dpt or Velo programme reads it (scan of the 7,779 cached programmes). Generality (D-0150/D-0161): no
change targets a zone, map or asset. Native1785 (m30, after the fixes): 1,148 draws, lit HDR 1.26, no fault.

Final pictures side by side (window captures 140 s after launch, Reference1773-1783 against Native1774-1784, all six zones,
eclipse sky; TestAnim early sky in Reference1759/Native1760): the scenes match in light and shadow. Visible gaps: the
bonfire's flames (m40, m50: only the embers natively), the eclipse ring and some far branches at m50, the m30 floor's
lightmap tint. They are effect and sky gaps, outside this workstream's lighting.

## 6. Hand-off to the cascade cache workstream (2026-10-16, read in the ASM, not yet built)

- `FUN_1401B5B40(obj, i, camera)` returns 1 (keep) only when all 8 corners of the current camera slice (FUN_14006EF80 on the
  camera record at caller `RBP+0xA40`) fall inside the previous cascade's clip volume (FUN_14001CE40 of the cascade record
  `[obj+0x140+8i]` +0x60 and +0xA0): per corner, `|x|, |y|, |z|` against 1 (MOVMSKPS on (x, y, z, x) compared with the
  constant at `1439F24B0`), a corner with any coordinate out counts as out (EBX), keep iff none is out. It returns 0 (redo)
  when `obj+0xC19+i` (content) is 0, and once when the forced byte `[14496231C+i]` is set (it clears it).
- Caller `FUN_1400F9B50` at `1400FA426`: AL != 0 jumps to `1400FA565` (next cascade), skipping the record
  (`FUN_1401B6020` at `1400FA544`, `FUN_140138540`, `FUN_1400BF4C0` twice, `FUN_140198680`, `FUN_1401389D0`): a kept cascade
  records nothing that frame. Whether the cascade fit and the receivers' matrices (state +0xAF0) stay the previous ones for a
  kept cascade is the first point to read (the depth must match the matrices the mask projects with).

## 7. The cascade cache workstream: no cache, the casters (2026-10-16)

Thread « Cache de cascades d'ombre » (D-0183's next workstream). Question: does the original keep its cascades (SHADOWS-NATIVE
1.4), and is that what makes it cheaper than the native at m30, m31 and m45?

**Read in the original (STATIC).** For a kept cascade F9B50 records nothing: neither the fit (1B6B80 command 0, 238680) nor the
atlas matrices (command 3 writes state +0xAF0 + 0x40 n), so its receivers would keep projecting with the matrices its depth was
drawn with (1D6D20 reads +0xAF0 every frame). The direction test compares state +0xA60, written by command 0 only, with a
tolerance [143D7F960] = 0.001; [1439F24B0] = 1.0 is 1B5B40's bound; [14473DAD4] = 1 and [14473DAF8] = 2.0 in .data. With
+0xC09 clear, 1B5390 makes one atlas whose quadrants are the four cascades (the 8192 atlas the captures show) and no reprojection.

**Observed in the original (`cascade-observe.flag`, Reference runs, Shebang zone saves, still 60 s then walking and turning).**
Reference1786 (m40), 1787 and 1788 (m30), 1789 (m31): 1B6020 records each cascade 240 times in every window of 240 frames, the
keep test 1B5B40 is never called (F9B50 finds +0xC1D cleared every frame), +0xC09 = 0, the forced bytes 0. **The original
redraws its four cascades every frame**: the cascade cache of SHADOWS-NATIVE 1.4 is inactive as shipped, and the native's
"redraw every cascade every frame" is the original's behaviour. The cache built in this thread (keep test, content tokens) was
removed before merging.

**What the original draws into them (OPEN).** The I-0063 atlas census (i63-live-10, three frames per zone, SHADOWS-NATIVE 1.3)
counts 22 to 33 depth-only and 3 to 8 alpha-tested draws per sun cascade, often none in the far ones; Reference1791 (m31) counts
93 `Dpt` material preparations a frame (type 2 in 140680). The native draws far more (Native1808 m31: 145, 223, 423 and 1,281
per cascade). No pass scene admission reaches the cascade passes in the original (142E80 for passes 1..4: 0, Reference1791;
57A20, D079C0 and D04B90 likewise 0), although D04B90 handles pass ids 1..0x38: its casters reach the cascades by a route not
identified (persistent records are a candidate). **Map pieces do cast**: a first change that refused them (commit f6c6c64a) lost
shadows the original has, the roof's on the wheel at m31 and the balustrade's on the m37 square (side-by-side captures
`cascade-casters-shots/`, Reference1800/Native1801 against Native1808 with map pieces: the wheel dark again as in the original).
That change was reverted. Which map pieces the original casts, and why its far cascades stay nearly empty, is the open question.

**Kept from this thread.** The character's 2-metre box took its centre from placement [9], [10], [11] while the 3x4 placement
holds the translation in [3], [7], [11] (Native1794: (0, 0.96, -593.9) for a player at (61, -156, -593)), so the box missed the
cascades and the player cast no sun shadow at m31 (Native1793); fixed (Native1795: the player in the cascades). The observers
(`cascade-observe.flag`) stay for the next step. Cost with every caster, m31 (Native1808, measure): 54 frames/s, collect 12.0 ms
(the shadow walk +1.9 ms, the caster rows +1.0 ms), prepare 8.0 ms, record 5.3 ms; without map pieces 59.5 (Native1801).

**The original's atlas, drawn (OBSERVED, `cascade-observe.flag`: d3d11.dll's draw entries hooked, draws whose depth view is the
8192 atlas counted per quadrant, vertex programmes named by their DXBC checksum against PROGRAM-FINGERPRINTS `programs.tsv`).**
Draws a frame per cascade 0..3: m31 3, 30, 30, 4 (Reference1809); m37 32, 37, 11, 17 (Reference1812); m30 2, 37, 13, 58
(Reference1813); the shadowed light's 512 tiles 27 to 54. The native drew 140, 218, 418, 1,264 at m31 (Native1815), 1,334 in
cascade 3 at m30 (Native1817). Programmes (detail rows, Reference1811-1813): at m31 only skinned character programmes (`C_ARSN_cinder_Dpt_Skin`,
`..._Dpt_Skin@[cl]`) and grass (`GXFlver_Grass_Dpt`, `GXFlver_GrassSpcBump_Dpt`, one draw of 150,705 indices); at m37 also rigid
lightmapped map families (`GXFlver_ColDifSpcBumpEmiMulMaskDOL_Dpt`, `GXFlver_ColDifSpcBumpDOL_Dpt`); at m30 rigid `EmiIbl` families
(`_Dpt`, `_DptA`, `_Ins`). So the original casts characters and a small subset of map pieces and objects, chosen per piece.

**Which pieces (OPEN).** Matching the original's draws to the native's caster meshes by index count (noisy: shared counts) at m37
points to the part flags the map piece copies at entity +0x80 (D03EB0 -> C4BCD0; byte 0 bit 0x02 gives the G-buffer category, 0x04
forward, 0x10 no reflection, 0x40 pass 0x39): the matched pieces carry byte 0 = 0x0A or 0x4A, the pieces with 0x08 alone or 0x00 do
not match. Bit 0x08 alone is not the rule: at m31 425 pieces carry it while the original casts none of them (Native1815). Not
implemented. Next step: the exact identity of each atlas draw in the original (the row executor 63CB0 at render time, or the
index buffers), then the rule read in the code that applies it.

**Two more facts met on the way.** Map pieces whose part flags byte 0 lacks 0x02 carry the category 0x01 only (D03EB0): the
original gathers them in pass 0x45 (D04B90: about 1.99 million emits against about 0.11 million for passes 5-8, Reference1812).
Correction (same evening): 0x45 is the main view's own type (WATER-TESS-WAVE1-READY 1.2, FUN_1400575C0), so these pieces are drawn
in the main view, as natively; what the category 0x02 adds (passes 5..8 and, K-0453, the sun casters) is not read.

**The rule (2026-10-16 22:20).** Reference1818 (m37) and 1819 (m31) name the models of the depth rows the original executes (63CB0,
state type 2): m37 the player's parts, map pieces `m37_00_00_00_000911`, `000912`, `001335`, objects `o000250`, `o372010`; m31 the
player's parts, `m31_00_00_00_002100`, `o000250`. Every one carries part flags byte 0 bit 0x02, which D03EB0 makes the category bit
0x02 C2EC0 gives passes 5..8; the category-0x01 pieces (most of the map) never cast. **The original's sun casters are the
G-buffer-category entities in its cascades.** The native shadow walk now visits the category 0x02 instead of the main view's union
0x07: Native1820 m31 casts the player, `c1240`, `o000250`, `002100`, `002091`, `002095`, `o310520` (65 draws a frame, against 1,264),
59.5 frames/s, collect 9.3 ms; Native1821 m37 the same five models as the original plus five pieces of the same category
(`000940`, `000950`, `001329`, `001331`, `001332`), 77 draws (288), 60 frames/s. The extras are within the native's cascade boxes
(the character's 2-metre box, the boxes' culling, OPEN, small). The shadows the side-by-side captures attributed to map casters
(the m31 wheel) are not sun-cascade shadows in the original: INFERRED baked (the open m30 lightmap tint, D-0180 note), to check.

**Closing measures and pictures (2026-10-16, Native1822-1827, measure runs, defaults).** Frames per second (collect ms): m40 60 (3.0),
m30 59.9 (8.4; 56.3 before), m31 59.5 (9.4; 55.3), m37 60 (6.5), m45 30.8 (26.1; 22.4 with the shadow, 30.5 without: the m45
collect cost is not the shadow), m50 59.9 (7.5). Side-by-side window captures 140 s after launch against Reference1796-1806
(`evidence/I-0066/artifacts/cascade-casters-shots/<zone>-final.jpg`): the scenes match; the differences left are those of the
lightmaps (the m37 floor strip by the balustrade, the m31 wheel, the m30 ground: lighter natively), the bonfire flames and the m50
eclipse ring, all outside the sun cascades (K-0453: the original casts none of those pieces). m31 stays a few tenths under 60
without any shadow (Native1801), and m45's collect (26 ms) is the next workstream.

**Full review (2026-10-16, D-0181).** Nothing serious: the observers are Reference-only and behind `cascade-observe.flag` (the
d3d11 draw hooks now log a failure and let the run go on), the 0x02 walk keeps the merge logic, no leftover of the abandoned
experiments. Applied: the census labels by data name and assembled part, the census map built only when sampled, the atlas detail
budget stops at zero, the observer comments no longer say "cache". Noted: with `cascade-observe.flag` alone the D04B90 observer
also writes its `reference_disp_emit` rows. Open by design: K-0453 is SUPPORTED on two named zones; a forward-only (0x04) entity
does not cast.
