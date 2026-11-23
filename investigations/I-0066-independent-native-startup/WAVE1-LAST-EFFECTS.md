# Wave 1, the last missing effects: implementation log

Workstream started 2026-10-17 from `main` 0228b403 (branch `claude/wave1-last-effects`), after the static preparations
WAVE1-LAST-EFFECTS-PREP (phantoms, summoned NPC face, W6 glow; branch `claude/relaxed-almeida-957f40`, 10d2cffe) and
WAVE1-LAST-EFFECTS-STATIC-PREP (lightmaps, bonfire flames, m50 sky; `claude/focused-wright-0cbac5`, 9f87a5f0). Order asked by
Thomas: lightmaps, bonfire flames, eclipse ring, phantoms, the summoned Sword Master without a face, W6 glow. Zone naming: m50 is
the Dreg Heap (DLC2), not Anor Londo; m37 is Irithyll / Anor Londo.

## 1. Lightmapped floors (done)

Two separate causes, both by mechanism.

1. **Dominant Light rows of cbPerFrame (b9 +0xC0..+0xDF).** Reference1844 (m37) and Reference1850 (m30): the view context's
   Dominant pair (+0x4110/+0x4118) holds the area key ({137000000, 0}, {130000000, 0}) and the engine bank 134A20 returns the
   zone file's `Dominant Light` row in file order (m37 0.5, 0, 1.25, **0.45**, 1, 1, 1, SH 1 x4; m30 0.5, **0.55**, 1.25,
   **0.5**, 1, 1, 1, SH **0.5**, 1, 1, 1). Native1845: the same keys, but the native bank never receives a file and returned
   its built-in row (colour scale 1). Fix: `Banks::DominantAt` (light_producers) reads the row per key from the game's gparam
   files with GroupEntries' layer rule; a key without the group keeps the built-in row; looked up only when the keys change.
   Rows `reference_dominant_rows`, `native_dominant_rows`, `native_dominant_source`.
2. **The lightmaps themselves were never bound.** Native1848/1849 (m30): every DOL lookup of the material's lightmap names
   (`<area>_m<id>_gi_0000_00_dol_0N`) found a registry holder without a texture, so every lightmapped piece took the white
   default. Reference1851: in the original those names resolve to `SYSTEX_DUMMY` too (SprjDummyBlinkGxTexture over
   SYSTEX_WHITE). The real lightmaps reach the draw through the ForLightMap path (K-0151/K-0153): 1A9C30 calls slot 1 of the
   draw's GXFlverModelDrawInstanceParam (19CA80), which with the draw parameter bit 13 clear calls 5B110 on the param's
   lightmap set +0x30: P[3], P[4] at t26/t27 (DOL) or P[0..2] at t26..t28 (RNM), each object's slot +0x10, and the PS
   cbTexHdrScaleParam (b0) from P[3]'s authored constant (slot +0x70), else the default [root+0x46E0]. Reference1855/1856: the
   set is the map piece item's +0x100, i.e. its draw descriptor (+0x70) plus 0x90, and the item's +8 is the param (the emit
   copies the address; the native draw does not run it); 5B3D0 reads the same descriptor's +0xB8 (atlasUvScaleOffset), which
   the native instance record already took. Fix (`ReadNativeLightmapSet`, `ApplyNativeLightmapSet`): the candidate's
   descriptor + 0x90 gives the set; the slot 26/27(/28) bindings take the set's textures and cbTexHdrScaleParam the lightmap's
   authored constant (`native_texture_constants`, now erased on retirement); the set's words, and a holder's texture, are
   kept with the model record. Native1857: lightmaps resident (256 and 128 wide), constants (0.70, 0.75, 1.02, 1) etc.

Results (lit HDR against the zone's reference capture, frame 2400; window pictures 140 s after launch in
`evidence/I-0066/artifacts/lightmap-shots/`): m30 whole 1.25 -> **0.98**, floor-left 2.41 (3.4 before D-0180) -> **1.03**,
wall 1.00, sky 1.01; m37 whole 1.10 -> 0.99; m31 floors at the original's level. Open: the m31 wheels stay lighter natively;
they are no lightmapped model draw entity (Native1860) and no DOL set was seen on them (cause not identified). The character
lighting ratio (0.84-0.86) is the known instance SH gap (IMAGE-REGRESSION-STATIC-LEADS L2 row 2).

Step review (Opus, light): no blocking finding; applied: constants erased on retirement, holder textures in the record words,
the constant only when t26 is the set's lightmap, compared at revalidation, the Dominant lookup cached by keys. Not applied
(OPEN): the RNM sets' default constant and default textures (19CA80 +0x38), the draw parameter bit 13 producer, a null set
entry binding null.

## 2. m50 (Dreg Heap) sky and eclipse ring (done)

- Native1862: the eclipse piece `m090002` (`Test_sun`, GXFlver_Col, forward-only material) was refused at the 0x29 row,
  recipe unresolved: `ComposeNativeShaderGroup` composed a loose group's G-buffer, velocity and depth programmes but not its
  forward ones. Fix (kept): `<group>_Fwd...` too. The sky pieces are now all prepared (Native1892: the dome and the zenith
  layer, class 0x80, at the 0x1F row; the clouds and the ring, class 2, at the 0x29 row).
- Still flat: every map piece's forward draw has FC_DynamicDiffuseColor (cbMatDynParam +0x180) zero (Native1871-1874), so the
  forward Col programmes output black and the fog shows. The material flag (+0x20 of its dynamic object) is clear for every map
  piece and their materials have no per-slot dynamic images (+0x90 count 0): 140850 binds no b2 for them and the original's
  draw reads whatever b2 an earlier draw left bound (Reference1873: images whose flag is clear hold 0 there). A default of
  (1,1,1,1) brought the m50 sky and ring back (Native1875/1888) but turned the m30 eclipse disc white (Native1889, lit HDR sky
  4.4x), so it was withdrawn.
- Cause (Fable 5.1 read-only review at Thomas's request, 2026-10-17; the two helpers re-read here): the flag is set by two
  helpers of the GX item parser 141070, which does not run natively. **22BD90** (item GX14, 141070 line 233): +0x20 = 1,
  +0x1F = 1; item version 0x65: +0x1E = 1 and +0x350..+0x35C = the item's colour and scale, else colour (1, 1, 1, 0) and scale
  its first word; provider +0x360 = the MTD's `g_DiffuseColor` parameter or none. **22BEC0** (141070 lines 451-466, after the
  items, when the MTD has the PoweredColor parameter `g_DiffuseColor`, its vt+0x38 holds and +0x1F is clear): +0x20 = 1,
  +0x1F = +0x1E = 0, +0x350 = (1, 1, 1, 1), provider = that parameter. 22E930 then writes {s*r, s*g, s*b, [492994C]} with the colour
  +0x350 (provider's UseCAParam and +0x1E) or the MTD `g_DiffuseColor`, the scale +0x35C (UseCAParam and +0x1F) or the MTD
  `g_DiffuseColorPower`. In the MTD bundle every `GXFlver_Col` MTD carries it (power 1, 3 for M[A]_Sky4, 50 for
  m30_00_sky); the Fujir and MultiBlend2 NoLight MTDs do not (their +0x180 stays 0). Fable also found that the original
  binds every material's own b2 (141F80 allocates the per-frame images for every material): the "count 0" was native.
- Fix: both helpers transcribed at the material's creation (`NativeModelMaterial::diffuse_provider`, the MTD values in place
  of the parameter object) and 22E930's provider branch reads them; no default. Native1901 (m50): the cloud piece 0.8 (the
  0.8 of Reference1873), the dome 1; the sky, clouds and the eclipse ring drawn (`effects-shots/m50-diffuse.jpg`).
  Native1902 (m30): the eclipse disc's meshes 0 (black, as the original), the sky unchanged (`m30-diffuse.jpg`).
  Early-game sky (Thomas: the sky changes between the start and the end of the game): TestAnim slot 1, camera raised,
  Reference1905 against Native1905: the clouded sky equal (`testanim-1905-ingame-look-3.jpg`).

## 2b. The embered armour's cinder glow (done)

TestAnim (Reference1903 against Native1903): the red glowing cracks of the embered character's armour were missing natively.
The SAT_Equip programmes multiply their cinder term by the generated parameter `SAT_Equip_snp_0_float_4_0` (b2 +0x270), which
22D620 animates through **22E5C0** when the parameter-animation table holds a universal row or one for the material's index:
each SPX descriptor with component 0 (+0x30), a nonzero key (+0x3C) and type 1..4 (11: four values, packed again into +0x398)
moves towards the row 146920 selects by that key and the material name, v += (t - v) * s; 22E930 then emits. Fix:
`AnimateModelGeneratedParameters` (model_parameters) with the descriptors kept by the parser, the emission shared
(`EmitGeneratedWords`). Native1904: the words become 1 (+0x270) and the cracks glow (`testanim-1904-ingame-look-2.jpg`).
## 2c. Equipment hiding the body (done)

TestAnim: white bands on the shins natively (Native1903/1904). `native_part_draw` (light-observe.flag, Native1909): the naked
body FC_M_0000's underwear meshes #18# and #19# (P_LG MTD, `FC_M_0000_Underwear_a`) drawn with the capability words
(FFFF.., FFFF..). `reference_part_mesh_draw` (cascade-observe.flag, Reference1908): the original's 64610 instances of the body
carry (0B13E3D6B551EC10, 80000000FFFF2880) in the main view, so a material whose #NN# bit (material +8/+0x10) is clear there is
not drawn. **CFDE70** draws each ChrAsm part with a stack copy of the group whose +0xC0/+0xC8 are the part descriptor's first
16 bytes OR bit 127 (5278C0(0x7F), 5B0800; CFDE70 lines 362-371), not the shared group's (the native read entity+0x240+0xC0, all
ones). Fix: `NativeSelectionCandidate::capability_source` = the descriptor for ChrAsm parts. Native1910: the shins as the
original (`effects-shots/testanim-1910-equiphide.jpg`). The same words choose a helmet's head meshes.

The summoned NPC (Reference1911 against Native1911, Argo teleported to the High Wall sign, `summon-1911-crop.jpg`): the face
and helmet are drawn natively (Native1878 before: dark body). Open: the phantom's axe and shield stay dark natively where the
original whitens them (the weapon parts, P_WP MTDs), and the sign's ghost before the summon is more opaque natively.

The reported case itself, the Sword Master (Argo teleported to the MSB sign c0000_0008 at (25.0, -25.5, 52.3), 6 m from the
start; Reference1919/1921 against Native1919/1920, `effects-shots/swordmaster-1920-looks.jpg`, `swordmaster-1921-looks.jpg`):
natively summoned with a face (an old man, white hair, eyes). Open: up close the original draws his skin and face entirely in
the white phantom shading, natively they keep their brown skin with white patches (the FgFace/Skin SSS families are drawn
without their `_Phantom_Fwd` programme; no `native_phantom_forward_fallback` row, so these meshes are not on a forward phantom
row). From a distance both are white silhouettes.
## 3. Bonfire flames (decided: own workstream)

The native renderer draws no FFX effect: wave 1 runs the effect system (instances, lights, wind) but not the draw families
E01-E08 (FFX-WAVE1-READY F7). The flame is effect 1020 (E01 billboards, E04 heat haze). A mechanism fix is the effects draw
side (EFFECTS-D3D12-DESIGN N5-N9), which also brings every fire, ember, smoke and spell. Asked to Thomas (2026-10-17 05:15):
its own workstream right after this one (recommended), here, or later. Thomas (2026-10-17 06:48 Paris): own workstream « Dessin des effets FFX » right after this one, E01 first.

## 4. Phantoms (colours done; translucent NPC phantom open)

- Run-PilotPair `-PhantomList` (new: each id in turn, one screenshot each). Reference1876 against Native1876 on Argo: gold 85,
  gold-red 86, purple 91, magenta 88, blue 82 and pale blue 83 equal the original (`effects-shots/phantoms-1876.jpg`).
- The skin's Phantom_Fwd and Fwd programmes were refused for cbPerFrame +0x30 (Native1876, 145,200 incomplete draws):
  23790 fills +0x00, +0x30, +0x40 from 1439F24B0 and +0x20, +0x50 from 1439F2490 (`ProduceNativePerFrameGlobals`).
- Kind 3 (181, fade 0.5): natively the inner surfaces showed and the cape vanished; the fading 0x29 row and the 0x1F row now
  write depth (Native1878 against Reference1878: the player's translucent phantom matches).
- **Open, the summoned NPC without a face**: at the summon sign near the High Wall teleport point (6.7, -6.3, 87.1) the sign's
  white NPC phantom (Lion Knight Albert there) shows its inner surfaces natively (`effects-shots/1878-ingame-phantom-181.jpg`),
  which hides a face seen from the front. Reference1879: no 64610 draw selects the cull rows 1 or 2 (14473C9F8 {0,2,1},
  {0,0,0}), so the original keeps row 0 there too; the cause is still open (next reads: the NPC phantom's rows and states,
  its model class and the draw path, single model or ChrAsm, PREP section 2.4 R1-R3).

## 5. W6 glow (measured natively; the original's output not yet isolated)

- Natively the glow runs every frame with the original's values (blur 1, rgb 1.5, final 0.5; `native_glow_inputs`).
  Native1893 (no-glow.flag) against Native1876: no visible difference on the gold and purple phantoms. Native1894
  (world-pictures.flag now also writes GB5 and S): GB5 alpha 1 over 56,000 phantom texels but its colour 0, so S is 0. The
  SAT_Equip phantom programme writes o1.rgb = r8 * FC_GlowScale, r8 being the cinder term times `SAT_Equip_snp_0_float_4_0`:
  zero before 2b's fix.
- The original's glow output: hooking 1BB810 copies nothing useful (Reference1898-1900: the state after the call is the
  previous presented picture, and running the glow twice changes nothing on it): the GX layer records its draws and runs them
  later, so a capture needs the D3D11 draw itself (the quad of the ten passes). The probe (`glow-capture.flag`, Reference1895-1900) was removed after the full review (D3D11 calls on a context it does
  not own); a capture of the original's glow needs a hook on the D3D11 draw of its ten quads.

## 6. Final comparison and review (2026-10-17)

Measure runs with the final build (no debug layer, no instrumentation), window pictures 140 s after launch against Reference1796-1806
(`effects-shots/<zone>-final.jpg`): m40 60 frames/s (Native1882), m30 60 (1883), m31 60 (1884), m37 60 (1885), m45 59.6 (1886),
m50 60 (1887). The scenes match except the effects (bonfire, torches, lanterns: the FFX draw side), the m50 sky, and at m31
two wooden crosses and a cloth the native draws where the original shows none (already in Native1824, before this
workstream) and the lighter wheels.

Full review (Opus): nothing blocking. Applied: the fading 0x29 and 0x1F depth writes honour the material's disable-depth-write;
the wave-1 witnesses behind `light-observe.flag` (the Reference probe behind `cascade-observe.flag`); 19CA80's fourth argument
forwarded; the Dominant cache holds the bank; lightmap record words only for a model with a set; distinct static names;
comments. Not applied (noted): a census of the draws the `<group>_Fwd` composition and the cbPerFrame words unlock (the six
pictures show no regression); a positive DominantAt fixture; the Reference state of the fading 0x29 depth (INFERRED from the
picture).

## 7. Second final comparison and full review (2026-10-17, after sections 2-2c)

Measure runs (no debug layer, no instrumentation), window pictures 140 s after launch against Reference1796-1806
(`effects-shots/<zone>-wave1.jpg`): m40 60 frames/s (Native1912), m30 60 (1913), m31 60 (1914), m37 60 (1915), m45 59.5
(1916), m50 60 (1917). Equal to the original except: the effects (FFX: own workstream), m31's two crosses and cloth and its
lighter wheels (before this workstream), the m37 chapel brighter natively (already in Native1885), the summoned phantom's
weapons (2c).

Full review (Opus, high): nothing blocking. Applied: the glow-capture probe removed (D3D11 calls on a context it does not own,
a kept render target, a stored HAL pointer); `native_generated_animated` behind light-observe.flag; the 22BEC0 gate and the
provider bytes labelled INFERRED; `no-glow.flag` and the other effect flags cleared at a run's start; the new witnesses stop
locking once full; a failed capability read refuses; non-finite animation results leave the words; the packed-colour slot
divergence documented; WP parts in `reference_part_mesh_draw`; a unit test of `AnimateModelGeneratedParameters`
(`game_model_preparation_test`, PASS). Not applied: the menu portrait path (D3DA20 reads the item's group: it does not call
5278C0/5B0800, so no per-part copy there).

## 8. Summoned phantoms' skin, face, eyes and weapons (done, one residual)

`native_phantom_part_row` (light-observe.flag, Native1922, the Sword Master): the NPC's parts are category 0x29 with their
phantom row (kind 3), the SAT families take `<family>_Phantom_Fwd`, but the gxflvershader `.spx` families (FgFaceSSSIbl,
SkinSSSIbl, EyeIbl, GXFlver_Equip) had no alternate: their effect lists no phantom programme. A9EF0 loads
`<shader>_Phantom_Fwd[_[A]]` from the family's bundle, or from the loaded repository [GXRS+0x4DD0] without one (MENUS-MAP 874).
Fix: the phantom recipes 12-13 take the loaded PS repository's programme by name with the Fwd VS (`phantom_loose_pixel`,
`shared_pixel`). Native1923: every part on its alternate, the white rim on the face and body and the weapons as the original
(`effects-shots/swordmaster-1923.jpg` against Reference1921); the player's phantoms unchanged (Native1924,
`phantoms-1924.jpg`). Residual: up close, the face's interior stays darker natively than the original's white translucency.
