# I-0113: the crystals' remaining hue, the programme's colour inputs not yet compared (static, study thread)

Study thread, 2026-11-06 (Paris), read-only, for the open-gaps pilot. The open point (RESULT « Tranche 16 »): Crystal Magic
Weapon's crystals (`s04020`..`s04022`, E03 model particles drawn with `GXFlver_ColDifSpcBumpIbl_Fwd`) are a little paler
natively, the original's a little pinker in the core; mulCol, the SH rows, ShLightMask, envData, the region IBL rows, the light
and fog keys ({0, 0} / {0, 0}) and the b3 words are equal in both; the IBL is the black cube in both (tranche 10, 14).

Source: `gxflvershader__GXFlver_ColDifSpcBumpIbl_Fwd.ppo` from the I-0063 cache, disassembled with fxc 10.0.28000 `/dumpbin`
(the listing stays in the study's scratchpad; reproduce with `fxc /nologo /dumpbin /Fc ps.asm <ppo>`). Everything below is read
in that listing (VERIFIED) unless marked.

## The inputs that reach the colour, and whether they were compared

| Input | Where in the programme | Compared? |
|---|---|---|
| cbLight b5 rows 0..11 (FC_DirLightVec/Col/Spc 0..3) and b5[16].xy (lights 2 and 3 on) | the four directional terms (`cb5[0..3]` · n, `cb5[4..11]` colours) | keys only, not the 0x120 bytes |
| the local lights: t59..t62, gated by cbInstanceData row 6.y (`dynamicLightDisable`, equal) | tile = (uv × FC_ScreenSize `cb8[1]`) / FC_LightTile_Width `cb8[56].x`, row × NumCellX `cb8[56].y`; slice = clamp((log(z − `cb8[55].x`) − `cb8[55].z`) / `cb8[55].y`, 0, 14), the light's slice mask `t59.y & (1 << slice)`; points from t61 (64 B), spots from t62 (128 B) | the buffers bound in both, contents not compared |
| cbSceneParam b8: FC_ScreenSize, FC_MatrixView, FC_MatrixInvProj (`cb8[12..15]`), FC_MatrixInvView, the grid words +880..+900 | the view-space position for the grid and the reflection vector | not compared at these draws |
| cbFog b6 rows 1..4 (FogCol, ExtFogParam, HeightFogParam, HeightFogCol) | the end: height fog and distance fog toward the fog colours, a luminance-scaled term | keys only |
| cbPerFrame b9[3].x FC_GlobalSpecularMask | the specular colour `cb3[1].xyz × cb9[3].x` | not compared |
| t44 g_RLRTexture (sampled at the screen uv, mip by gloss) | `colour = ibl × (1 − rlr.a) + rlr.rgb × spec` | the original binds a 1x1 B8G8R8A8 texture (OBSERVED, `reference_e03_inputs`), the native SYSTEX_ZERO; **which 1x1 the original's is (black with alpha 0, white, grey) is not known** |
| t33 g_SSAOTexture | ambient occlusion | the original a 1x1 B8G8R8A8, the native SYSTEX_WHITE |
| cbModelParam b7, the parallax-corrected cube words (+128..+420) | the box-projected IBL | not compared (the IBL is black: no effect while it is) |

## Leads, ranked (INFERRED)

1. **The local lights at the crystals' tiles** (the bonfire's light): the « pinker core » fits a warm point light reaching
   the original's crystals and not the native's. A one-row witness at the crystals' draws: the grid words of their b8
   (+880..+900) and, at their screen tile, the light count `t60[tile].y` and the first indices, both sides.
2. **t44**: if the original's 1x1 is not black with alpha 0, the IBL term is replaced by `rlr.rgb × spec` (alpha 1) natively
   absent. One read of the bound texture's texel (as `reference_unit_cube` did for the cubes) answers it.
3. cbLight's bytes for key {0, 0} and FC_GlobalSpecularMask at the draws.

## t44: which 1x1 it can be, and the cheapest witness (study thread, 2026-11-06, later)

The original's binder of t44 for the forward rows was not found statically (no call of the PS resource setter `141920DA0` with
an immediate slot 0x2C in the image; the slot is set some other way, not chased). The 1x1 B8G8R8A8 (DXGI 87) textures the engine
owns are the GX common 2D set `[[144961AB0]+0x270] + 8 × i`, i = 0..13 (entry 14 at `+0x70` is the black cube, tranche 10), with
the colours `native/content/model_common_textures.cpp` transcribes from `06C00`: WHITE FFFFFFFF, BLACK FF000000, GRAY FF808080,
SPEC 800A0A0A, BLACKSPEC 01000000, 18PERCENT_GRAY (DXGI 91, so not t44's), NORMAL FF8080FF, ZERO 00000000, then the six colours
(INFERRED that t44's 1x1 is one of them: the same format and size). With the IBL black in both, `ibl × (1 − rlr.a) + rlr.rgb ×
spec` changes the colour only if the 1x1's rgb is not zero: BLACK and ZERO give the native's result, WHITE, GRAY or SPEC add the
specular colour (`FC_SpcMapMulCol × FC_GlobalSpecularMask` times the specular map) to the crystals. **Witness:** at a crystal
draw, the t44 view's resource pointer compared with the fourteen entries of `[[144961AB0]+0x270]` (an identity, no texel read).
