# I-0125 study: the sky layers' forward programmes (static, study thread)

Study thread, 2026-11-06 (Paris), read-only, for the R pilot's sky in the rays (the step after the `GXFlver_Col_Fwd` dome, which
the pilot read itself). Sources: the pixel and vertex programmes of the I-0063 cache (`C:/Users/User/.codex/I0063-cache/programs`),
disassembled with fxc 10.0.28000 `/dumpbin` (listings in the study's scratchpad; reproduce with `fxc /nologo /dumpbin /Fc`); the
MTD parameters `evidence/I-0068/queries/npc-families/all-mtd.tsv`; the forward-only census of the native journals
(`native_rt_forward_only`, `evidence/I-0066/artifacts/*/startup.jsonl`). Which map uses which layer:
[STUDY-WATER-FAMILIES](STUDY-WATER-FAMILIES.md) §3. Read in the listings or the tables unless marked INFERRED or TO CHECK.

## 1. The MTDs of the sky layers

| MTD | family | g_BlendMode | depth write | other flags |
|---|---|---|---|---|
| `M[A]_NoLight` (the domes) | `GXFlver_Col` | 0 | on (`g_GX_bDisableDepthWrite` False) | `GXFT_InvalidLighting` |
| `M[A]_al_NoLight` (glow, far clouds, the m37 translucent dome) | `GXFlver_Col` | 2 | off | `GXFT_InvalidLighting` |
| `M[A]_2m_NoLight_scroll` (the scrolling zenith, m45's base and starry skies, m51's cloud sea) | `MultiBlend2_et1_nolight_MuitiBlendMask` | 0 | on | `GXFT_ForceForward` |
| `M[A]_al_m_NoLight_Opacity` (cloud sheets) | `Fujir_Nolight_OpacityBlend` | 2 | off | `GXFT_ForceForward`, `g_GX_bDisableDecal` |
| `M[A]_al` (cumulus) | `GXFlver_ColDif` | 2 | off | `GXFT_InvalidLighting`, alpha ref (1, 0) |
| `M[A]_add` (glow 2) | `GXFlver_ColDif` | 4 | off | `GXFT_InvalidLighting` |
| `Eclipsesun_M[A]_al_NoLight` | `GXFlver_Col` | 5 | off | (not placed in the sky table; TO CHECK where) |
| `M[ARSN]_em` (m39's `Sky`) | `GXFlver_ColDifSpcBumpEmiIbl` | 0 | **on** | none: an opaque G-buffer piece (INFERRED from blend 0 and no ForceForward) |

`g_BlendMode` 0 is opaque and 2 alpha blending (the dome and cloud pieces' use); 4 and 5 are read through the table `14473E5C0`
(STUDY-TRANSLUCENT-CHAMELEON §5), not resolved here (TO CHECK: additive is the likely 4 for a « glow », INFERRED from the name).

## 2. `MultiBlend2_et1_nolight_MuitiBlendMask_Fwd` (the scrolling zenith)

Vertex (`.vpo`): world = `VC_aObjMatrix[instance]` (cb5) × position; clip = `VC_MatrixViewProj` × (world − `SC_CameraPos`); o3 = the
clip position; o4 = the vertex colour; o5.xy = TEXCOORD0 × `VC_TexScroll_0.xy` / 2048 + `VC_TexScroll_0.zw`, o5.zw = TEXCOORD1 ×
`VC_TexScroll_1.xy` / 2048 + `VC_TexScroll_1.zw` (cb11 `cbTexScroll`); o6 = TEXCOORD (float4, input 5); o1.w = 0 (the instance
index the pixel reads). Nothing follows the camera beyond the camera-relative transform every map programme has.

Pixel (`.ppo`), `T` = `FC_GlobalTime` (cb9[11].x), `K[n]` = `FC_TextureTilingScale[n]` (cb1, xy scale, zw offset), the material
words cb2 at 624.. (`snp_0_color_0` = cb2[39].xyz, `float_0` = cb2[40].x, `float_1` = cb2[40].y, `float2_1` = cb2[40].zw,
`float2_2` = cb2[41].xy, `float2_3` = cb2[41].zw, `float2_4` = cb2[42].xy):

```
m  = t4(Texture2D_0, (uv0 + T·float2_1)·K[0]).x + t1(Texture2D_3, (uv0 + T·float2_2)·K[3]).x + 0.01
m  = saturate((v6.x − m·float_0·(1 − v6.x)) · (1 + vertexColour.a · float_1))
A  = t2(Texture2D_2, (uv1 + T·float2_3)·K[2]).rgb
B  = t3(Texture2D_1, (uv0 + T·float2_4)·K[1]).rgb
rgb = lerp(B, A, m) · snp_0_color_0 · FC_DifMapMulCol (cb3[0]) · mulCol.rgb (cbInstanceData row 7)
a   = mulCol.a
then the fog (below)
```

One sampler (s0 `SS_DiffuseMap`) for the four textures. No light, no IBL, no depth read.

## 3. `Fujir_Nolight_OpacityBlend_Fwd` (the cloud sheets)

Vertex: as §2 for the position (no `cbTexScroll`); o4.xy = TEXCOORD0 / 2048, o4.zw = TEXCOORD1 / 2048, o5.xy = TEXCOORD2 / 2048,
o6 = TEXCOORD3 (float4).

Pixel, the material words `float2_0` = cb2[40].xy, `float2_1` = cb2[40].zw, `float2_2` = cb2[41].xy, colour cb2[39].xyz:

```
a   = t1(Texture2D_5, (uv2 + T·float2_0)·K[5]).x · t2(Texture2D_4, (uv1 + T·float2_1)·K[4]).x · TEXCOORD3.x · mulCol.a
rgb = t3(Texture2D_1, (uv0 + T·float2_2)·K[1]).rgb · snp_0_color_0 · mulCol.rgb
then the fog (below); output (rgb, a), blended alpha by the MTD (mode 2), depth not written
```

## 4. The fog, the same in the three programmes and in `GXFlver_Col_Fwd`

The world position from the clip position (o3) × `FC_MatrixInvViewProj` (cb8[8..11]), the camera from `FC_MatrixInvView`'s
translation (cb8[20..22].w); then with cbFog (cb6: FogCol row 1, ExtFogParam `E` row 2, HeightFogParam `H` row 3, HeightFogCol
row 4): the height fog `h = saturate(saturate((H.w − (y_cam − y) − H.y) · H.z) · H.x)`, the distance fog over the horizontal
distance `d = saturate(E.x · (1 − exp(−(|xz_cam − xz| − E.y) · E.z)))`, a luminance term on the unfogged colour
`L = dot(rgb, (0.29891, 0.58661, 0.11448)) − 1`; `rgb = lerp(rgb, FogCol.rgb · (1 + FogCol.w · L), d)`, then
`rgb = lerp(rgb, HeightFogCol.rgb · (1 + HeightFogCol.w · L), h)`; alpha untouched. The instruction sequence is identical in the four listings
(compared line by line on the tails).

## 5. `GXFlver_ColDif_Fwd` (the cumulus): forward, a lit programme under an unlit MTD

The family has a `Gbuf` and `Fwd` programme; the cumulus MTDs are blended (modes 2 and 4) with depth write off, and the journals
list the cumulus pieces as forward only (m30 `901020`, `901030`, OBSERVED): forward. Its `_Fwd` pixel programme reads `cbLight`
(the four directional lights, cb5[16].xy enabling lights 2 and 3), the SH rows of cbInstanceData (8..10), the local-light grid
(t59..t62, gated by `dynamicLightDisable`, row 6.y) and t33 (SSAO through `FC_SSAOParam.x`), diffuse only (no specular, no IBL),
times the albedo t0 × cb2[24] and mulCol, then the fog. **But both cumulus MTDs carry `GXFT_InvalidLighting` True**, as the domes
do. **The flag does not reach a map draw** (a `ghidra-research` read, headless on the mirrors and GhidraMCP; its scans and
outputs in `evidence/I-0125/invalid-lighting/`): the flag is bit 20 of material+0x24 (A7E40 → gxMtd+0x88 → 140F90), and its only
reader found is `140061A20` (re-read by the study thread: the E03 model set's materials, `(+0x24 >> 0x14) & 1`, for 27EA70's colour
scale); gxMtd+0x88's bit 20 has no reader; the material apply `140140680` reads +0x24's bits 2, 7 and 11 only; the draw-state
word's own bit 20 (+0x200, set by 5A120 from the pass byte +0x1A9: FC_DitherEnable) is another flag (VERIFIED). The lighting
inputs come from the draw group and state, not the material: `dynamicLightDisable` = (group+0x128 != 0) (`14005B3D0`, re-read in
`decompiled/DS3-1152-F-0005B3D0.c:581`), the SH rows from the group's light data, cbLight from the state's light keys +0x338/+0x340
(149CB0, 14AB20) for pass type 0 (VERIFIED). So a cumulus pixel is lit like any `ColDif_Fwd` pixel, by whatever lights its
group and state carry (INFERRED: the scan's residue, about 80 shift hits outside the draw core and ten 0x100000 operands, was not
opened one by one; the sky group's light keys and +0x128 were not read). A live read at one cumulus draw (b5, cbInstanceData
rows 6 and 8..10, against a map draw of the same frame) would settle what light it receives.

## 6. m39's `Sky`

`M[ARSN]_em`, `GXFlver_ColDifSpcBumpEmiIbl`, blend 0, depth written, no ForceForward: an ordinary opaque lit piece with an emissive
term, drawn in the G-buffer like the map (INFERRED); a ray hitting it should already find its material through the G-buffer
families (I-0119) if `ColDifSpcBumpEmiIbl` is covered there (TO CHECK in GBUF-FAMILIES).
