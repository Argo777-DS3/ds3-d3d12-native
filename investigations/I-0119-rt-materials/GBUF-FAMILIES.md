# DS3 G-buffer pixel programmes: families compared with the reference

> I-0119 R3, the census of the G-buffer programmes (2026-11-02, a Sonnet subagent, read-only; the pilot re-read the reference
> `ColDifSpcBumpDOL`, the point light and the `EmiIbl` emissive line in the disassembly). The evaluator and its classes:
> `evidence/I-0119/artifacts/census/` (`symx.py`, `step3_cls.json`).

Scope: static reading of the DXBC (SM5) disassembly of the `*Gbuf*.ppo` programmes in `C:\Users\User\.codex\I0063-cache\programs\`.
All .asm files: `evidence/I-0119/artifacts/census/asm/` (outside Git) (676 `*Gbuf*.ppo`, 6 `.vpo`, 1 light-accumulation `.ppo`, plus 4 more `.vpo` named `..._GBuf.vpo`).
Tags: OBSERVED = read in the disassembly (or computed by a symbolic evaluator `symx.py` that follows register data flow; the evaluator output was
cross-checked by hand on the reference, Ibl, Bloody, MulDOL, MulMaskDOL, MultiBlend2/4). INFERRED = interpretation of a name or role.

Reference = `GXFlver_ColDifSpcBumpDOL_Gbuf.ppo` (taken as given in the brief). Notation: `tN` = texture register, `TTS[k]` = `FC_TextureTilingScale[k]`
(cb1, uv*row.xy+row.zw), `inst.X` = `cbInstanceData` field of instance `round(v1.w)` (17 rows each), `cb3:` = cbMtdParam, `cb2:` = cbMatDynParam,
`s = FC_SpcParam.x/256`.

## 0. Things common to every programme read (OBSERVED)

* Alpha channel logic is the same everywhere. A = (alpha source) * inst.mulCol.w, where the alpha source is `t0.a * COLOR1.w` in the plain
  variants. Flag F = (`cb3:FC_AlphaRef.z` != 0). Then o2.w = A; o0.w = o1.w = F ? A : 0; o3.w = F ? A : envData/255; o4.w = F ? A : 1
  (`envData` = `inst.envData.x`, the light-set index). Emissive programmes: o5.w = F ? A : 0 (`Glow` variants: o5.w = F ? A : `cb3:FC_GlowScale.w`).
* `[A]` programmes (alpha-tested) wrap every output in a dither/alpha-test `movc(test, 0, value)` (test uses t38 g_DitherTexture,
  `inst.ditherFadeAlpha`, `cb3:FC_AlphaRef.x/.y`, `cb2:FC_isDitherAlpha`, `cb7:FC_DitherEnable`); the colour arithmetic inside is the same as the
  non-[A] twin. 340 of the 676 programmes are [A] twins (336 are not). Phantom and Cloth twins of a name are byte-identical in code to their base in the cases checked
  (Ibl, Bloody, Df, BlendIbl, EmiIbl: see "same body" lists below).
* o0.xyz normal = normalize(...)*0.5+0.5 with the 0.500489 offset, back faces flipped (SV_IsFrontFace), tangent basis T=normalize(tangent), B=normalize(cross(N,T))*tangent.w,
  in every programme that has a bump map.
* Vertex side (OBSERVED, .vpo): UV0 = int16 texcoord * (1/2048) + a per-draw offset (`cb11[0].zw`); lightmap UV (DOL families) = int texcoord * (1/2048) + `inst.atlasUvScaleOffset.zw`
  (cb4[3].zw); COLOR1, COLOR2, COLOR3, TEXCOORD1/2 vertex-stream values are passed straight through (`mov oN, vK`) in the families
  read here, not in the Mul ones (§3, §4: their TEXCOORD0.zw and TEXCOORD1 are scrolled and scaled, R3c second tranche, 2026-11-03).
* Ibl (no lightmap) programmes: o1.z = `inst.ShLightMask.x`; o4.xyz = (1 - t1.rgb) * max(0, [dot(N,1) with inst.aSh0/aSh1/aSh2 rows]) (SH L1 ambient evaluated on the final shading normal).
  DOL (lightmap) programmes: o1.z = t26.a at lightmap UV, o4.xyz = (1 - t1.rgb) * [the t26/t27 dominant-direction lightmap term as in the reference].

---------------------------------------------------------------------------------------------------------------------------------

## 1. ColDifSpcBumpIbl  (`gxflvershader__GXFlver_ColDifSpcBumpIbl_Gbuf.ppo`)
Same body (identical md5 of disassembly body): ColDifSpcBumpIbl_Gbuf, _Phantom, _Cloth, _Cloth_Phantom, and `GrassSpcBump_Gbuf`.
Inputs: v1 normal(w=instance), v2 tangent, v3 = COLOR1 (only .w), v4.xy = UV0 (TEXCOORD6), v5.x = front face. cb1/cb3/cb4 only (no cb2, no lightmap).
* o0.xyz: same as reference (bump t3.rg, TTS3, UV0). OBSERVED
* o1.x: same as reference (t3.b * s^2). o1.y = 0 (reference: COLOR1.y*FC_TranslucencyScale*0.1). o1.z = `inst.ShLightMask.x` (reference: lightmap alpha t26.a). OBSERVED
* o2.xyz: same as reference (t0.rgb * cb3:FC_DifMapMulCol.rgb, TTS0). o2.w: t0.a*COLOR1.w*inst.mulCol.w, same. OBSERVED
* o3.xyz: same as reference (sqrt(t1.rgb), TTS1). o3.w: common alpha logic. OBSERVED
* o4.xyz: (1 - t1.rgb) * max(0, SH(N)) using inst.aSh0..2 (reference: lightmap term). OBSERVED
* o5: not written.

## 2. ColDifSpcBumpBlendIbl  (`GXFlver_ColDifSpcBumpBlendIbl_Gbuf.ppo`)  - normal-map blend only
Same body: ColDifSpcBumpBlendIbl_Gbuf, _Cloth_Gbuf. Inputs: v1,v2 as Ibl; v3.xyz = COLOR2, v4.xyz = COLOR3 (new), v5.w = COLOR1.w, v6.xy = UV0 (TEXCOORD6), v7.x front face.
* Blend factor: w = saturate( dot(cb3:FC_BmpBlendParam0.xyz, COLOR2.xyz) + dot(cb3:FC_BmpBlendParam1.xyz, COLOR3.xyz) ). Source: two vertex colour streams (COLOR2, COLOR3, passed straight through by the .vpo, `mov o3, v4 / mov o4, v5`)
  weighted by two per-material vectors in cbMtdParam (offsets 192 and 208). OBSERVED
* o0.xyz: n1 = normal from t3 (TTS3), n2 = normal from t10 `.rg` (same UV, same TTS3 row, same sampler s2, same tangent frame); n = n1 + w*(n2 - n1) with n2 normalised, n1 normalised; the lerp result is NOT re-normalised
  before *0.5+0.5. OBSERVED. (The second bump texture's .b is unused.)
* o1.x: same as reference but from t3.b only (t3, not t10). o1.y = 0. o1.z = ShLightMask.x. OBSERVED
* o2, o3, o4, alpha: identical to ColDifSpcBumpIbl (single layer t0/t1, no blend). OBSERVED  => "Blend" here blends two normal maps (wrinkles/damage), not albedo or F0.

## 3. ColDifSpcBumpMulDOL  (`GXFlver_ColDifSpcBumpMulDOL_Gbuf.ppo`)  - two full layers
Same body: ColDifSpcBumpMulDOL_Gbuf, ColDifSpcBumpMulEnvDOL_Gbuf. Inputs: v1 normal, v2 = tangent set 0 (TEXCOORD2), v3 = tangent set 1 (TEXCOORD4), v4.w = COLOR1.w, v5.xy = UV layer 0, v5.zw = UV layer 1 (TEXCOORD6), v6.xy = lightmap UV (TEXCOORD7), v7.x front.
* Blend factor: w = COLOR1.w (vertex colour alpha, straight through the .vpo `mov o4, v4`). OBSERVED
* Layer 0 = t0 diffuse (TTS0), t1 specular (TTS1), t3 bump (TTS3) at UV v5.xy with tangent set 0. Layer 1 = t7 diffuse (TTS7), t8 specular (TTS8), t10 bump (TTS10) at UV v5.zw with tangent set 1. OBSERVED
* o0.xyz: n = normalize( lerp(n_layer0, n_layer1, w) ) with each n built as in the reference from its own bump/tangent frame. OBSERVED
* o1.x: s^2 * lerp(t3.b, t10.b, w). o1.y: same as reference (COLOR1.y*FC_TranslucencyScale*0.1). o1.z: lightmap alpha, same as reference. OBSERVED
* o2.xyz: lerp(t0.rgb, t7.rgb, w) * cb3:FC_DifMapMulCol.rgb. o2.w: lerp(t0.a, t7.a, w) * inst.mulCol.w (no vertex alpha factor: COLOR1.w was used as blend). OBSERVED
* o3.xyz: sqrt( lerp(t1.rgb, t8.rgb, w) ). OBSERVED. o4.xyz: same lightmap form as reference with (1 - lerp(t1, t8, w)). OBSERVED
* Vertex side (the .vpo, 2026-11-03): UV layer 0 = TEXCOORD0.xy x VC_TexScroll_0 / 2048, layer 1 = TEXCOORD0.zw x VC_TexScroll_1 / 2048
  (TEXCOORD0 R16G16B16A16_SINT); tangent set 1 = TANGENT1; the lightmap UV TEXCOORD1.xy x atlasUvScaleOffset. OBSERVED

## 4. ColDifSpcBumpMulMaskDOL  (`GXFlver_ColDifSpcBumpMulMaskDOL_Gbuf.ppo`)
Same body: ColDifSpcBumpMulMaskDOL_Gbuf, ColDifSpcBumpMulMaskEnvDOL_Gbuf. Identical to MulDOL except the factor:
* Blend factor: k = t14.r * cb2:FC_BlendMaskParam.x + cb2:FC_BlendMaskParam.z (t14 g_BlendMaskTexture at UV TEXCOORD7.zw, TTS14, sampler s10 SS_WrapAniso; FC_BlendMaskParam is cbMatDynParam offset 48);
  c = COLOR1.w; if k > 0 then u = saturate(c*(1+k)) else u = saturate(c + k*(1-c)); w = u^2*(3-2u) (smoothstep). OBSERVED
  Then every layer lerp uses w. Normal is lerped then normalised. All other targets as MulDOL. OBSERVED
* The mask's UV (the .vpo, 2026-11-03): TEXCOORD1.zw x VC_TexScroll_3 / 2048 in MulMaskDOL (TEXCOORD1 four SINT16, its .xy the
  lightmap's), TEXCOORD1.xy x VC_TexScroll_3 / 2048 in MulMaskIbl. FC_BlendMaskParam: the raster's 22E930 block {a, b, -0.5 a,
  0.5 - 0.5 b} from the material's +0xD0+0x28. OBSERVED

## 5. ColDifSpcBumpEnvDOL  (`GXFlver_ColDifSpcBumpEnvDOL_Gbuf.ppo`)
OBSERVED: byte-identical file (same md5) to `ColDifSpcBumpDOL_Gbuf.ppo`, so "same as reference" for o0..o4. The Env part is not in the G-buffer pass. Same for `ColDifSpcBumpMulEnvDOL`, `MulMaskEnvDOL` (same body as their non-Env twins).

## 6. ColDifSpcBumpEmiIbl  (`GXFlver_ColDifSpcBumpEmiIbl_Gbuf.ppo`)  - emissive in o5
Same body: ColDifSpcBumpEmiIbl_Gbuf, _Cloth_Gbuf. o0..o4 identical to ColDifSpcBumpIbl (section 1) (o1.y = 0).
* o5.xyz = cb2:FC_EmissiveColor.rgb * t6.rgb (t6 g_EmissiveTexture, TTS6, UV0, sampler s8). FC_EmissiveColor = cbMatDynParam offset 32 (a per-material dynamic param). o5.w = F ? A : 0. OBSERVED
* `EmiIblGlow` differs only in o5.w = F ? A : cb3:FC_GlowScale.w (and the order of the alpha moves). OBSERVED
* Of the 676 programmes, 248 write o5. Besides `Emi*` (EmissiveColor * t6), `EmiMul*` blend t6 and t13 with the same layer weight as the albedo blend then * EmissiveColor; the SAT_Equip/cinder/Veill families
  write a view-angle (Fresnel-like exp/log term, `cb2/cb3` material floats, GlobalTime) glow; `Col`, `ColSh`, `Sword`, `Veill_NoLight` write colour*DynamicDiffuseColor/DifMapMulCol (unlit colour, see section 13). OBSERVED

## 7. ColDifSpcBumpIbl_Bloody  (`GXFlver_ColDifSpcBumpIbl_Bloody_Gbuf.ppo`)  - decal (blood) layer + wet layer
Same body (8 programmes): Ibl_Bloody_{,Phantom,Cloth,Cloth_Phantom}_Gbuf and the same with `_Cm` (so `ColDifSpcBumpIbl_Cm` does not exist as such; only `Bloody_Cm`, whose code equals Bloody). Inputs: v3.z = COLOR1.z (wetness threshold), v3.w = COLOR1.w, v4.xy = UV0, v4.zw = UV1/decal UV (TEXCOORD5), v5.x front.
Extra textures: t41 g_DecalDiffuseTexture (rgba, sampled at raw v4.zw, sampler s0, no tiling row), t42 g_DecalSpecularTexture (sampler s1, raw v4.zw: .r = decal gloss, .g = decal F0 root). Extra constants: cb3:FC_BloodAmount (B), cbMatDynParam FC_WetDiffuseColor (offset 560), FC_WetParam1 (576), FC_WetRefrectanceColor (592), FC_WetParam2 (608).
Let a = t41.a (decal alpha), B = FC_BloodAmount.
* o0.xyz: same as reference (bump t3 only; the decal does not alter the normal). OBSERVED
* Spec base S = t1.rgb*(1 - B*(1-a)) + B*(t42.g)^2 (per channel); gloss base G = t3.b*(1 - B*(1-a)) + B*t42.r. OBSERVED
* Wetness w = saturate( ( lerp(FC_WetParam1.x, FC_WetParam1.y, luma(S)) * 1.2 - COLOR1.z ) * 10 ), luma weights (0.29891, 0.58661, 0.11448). OBSERVED
* o2.xyz = ( wetD*(1 - B*(1-a)) + B*t41.rgb ) * FC_DifMapMulCol.rgb, where wetD = lerp(t0.rgb, FC_WetDiffuseColor.rgb, w*FC_WetDiffuseColor.w). o2.w = t0.a*COLOR1.w*inst.mulCol.w. OBSERVED
  (t41.rgb is added as a pre-multiplied colour: result = base*(1-B*(1-a)) + B*decal.)
* o3.xyz = sqrt( lerp(S, FC_WetRefrectanceColor.rgb, w*FC_WetRefrectanceColor.w) ). OBSERVED
* o1.x = s * lerp( G*s , FC_WetParam2.x , w*FC_WetParam2.z ). With B=0 and w=0 this is G*s^2 = the reference gloss. OBSERVED
* o1.y = 0, o1.z = ShLightMask.x, o4.xyz = (1 - S') * max(0,SH(N)) where S' is the wet-blended spec (before sqrt). OBSERVED
* The wet code (no decal) alone is in `ColDifSpcBumpIbl_Df` (next section), `Equip*`, `Eye*`, `Hair*`, `SAT_*` (the 260 "blood/wet" programmes). INFERRED role: rain/blood on characters; FC_Wet*, FC_BloodAmount are per-material dynamic (cbMatDynParam / cbMtdParam), i.e. CPU-driven.

## 8. ColDifSpcBumpIbl_Df  (`GXFlver_ColDifSpcBumpIbl_Df_Gbuf.ppo`)  - two-sided textures + wet
Same body: Df_{,Phantom,Cloth,Cloth_Phantom}_Gbuf. Inputs: v4.xy = UV0, v4.zw = UV1 (TEXCOORD5), v3.z = COLOR1.z, v5.x = SV_IsFrontFace.
* "Df" = a separate texture set for the back face: every material texture is `movc(front, tFront@UV0, tBack@UV1)`: diffuse t0 (TTS0, UV v4.xy) / t7 (TTS7, UV v4.zw), specular t1 / t8 (TTS8), bump t3 / t10 (TTS10). OBSERVED. (INFERRED: Df = double-face.)
* Then the same wet blend as Bloody (without the decal): w = saturate((lerp(WetParam1.x,WetParam1.y,luma(spec))*1.2 - COLOR1.z)*10); albedo = lerp(diff, FC_WetDiffuseColor.rgb, w*.w) * FC_DifMapMulCol; F0 = sqrt(lerp(spec, FC_WetRefrectanceColor.rgb, w*.w));
  gloss = s * lerp(g*s, FC_WetParam2.x, w*FC_WetParam2.z) with g = selected bump .b. o2.w = t0.a*mulCol.w (front-set alpha only, and no vertex alpha). OBSERVED
* o1.y = 0; o1.z = ShLightMask.x; o4 SH form with (1 - wet-blended spec). OBSERVED

## 9. ColDifSpcBumpIbl_Cm  - does not exist as a name. `ColDifSpcBumpIbl_Bloody_Cm_Gbuf.ppo` exists and is code-identical to `ColDifSpcBumpIbl_Bloody_Gbuf.ppo` (section 7) (OBSERVED, same body hash). Also `Equip_Cm`, `SAT_MultiBlend2_BlendOpacity_Equip_Cm`: not read individually.

## 10. ColDifSpcBumpTrIbl  (`GXFlver_ColDifSpcBumpTrIbl_Gbuf.ppo`)
Same body: `Slime_Gbuf`. Equal to ColDifSpcBumpIbl except o1.y = 0.1 * cb2:FC_TranslucencyScale * COLOR1.y (the reference's translucency term, so "Tr" = translucency). OBSERVED. o0, o1.x, o2, o3, o4 as Ibl.

## 11. ColDifSpcBumpDOL_SnowLt  (`GXFlver_ColDifSpcBumpDOL_SnowLt_Gbuf.ppo`) and `_Gbuf2`
Same body: ColDifSpcBumpDOL_SnowLt_Gbuf, ColDifSpcBumpEnvDOL_SnowLt_Gbuf. Extra: t15 g_SnowMaskTexture (TTS15), t47 g_SnowDiffuseTexture (UV0 * cb9:FC_SnowTilingScale), cbMatDynParam FC_SnowMaskParam (offset 160), cbModelParam FC_SnowRate/MinDot/MaxDot, cbPerFrame FC_SnowDir, FC_SnowReflectance, FC_GroundPlaneHeight, FC_RcpSnowHeightRange.
* Snow weight (OBSERVED): noise m = (0.251*t15@(uv*3.99) + 0.5*t15 + 0.124747*t15@(uv*16.08))*1.141882 * FC_SnowMaskParam.x + FC_SnowMaskParam.z; up factor
  u = saturate((dot(N_geom, FC_SnowDir) - FC_SnowMinDot)/(FC_SnowMaxDot - FC_SnowMinDot)); cover = smoothstep-style combination of m, u and COLOR1.z; W = saturate((cover*FC_SnowRate - FC_GroundPlaneHeight) * FC_RcpSnowHeightRange).
* o2.xyz = lerp(t0.rgb, t47.rgb, W) * FC_DifMapMulCol. o3.xyz = sqrt( lerp(t1.rgb, FC_SnowReflectance, W) ). OBSERVED
* o0.xyz: n = lerp(bump-mapped normal, geometric normal (flipped for back faces), cover*FC_SnowRate), normalised: snow flattens the normal. OBSERVED
* o1.x: unchanged reference (t3.b*s^2). o1.y reference form. o1.z lightmap alpha. o1.w = F ? A : cover*FC_SnowRate (a snow amount in G-buffer1.w). OBSERVED
* `_Gbuf2` programmes (18, e.g. ColDifSpcBumpDOL_SnowLt_Gbuf2): a second pass that reads a screen-space texture t35 at the pixel position (uv = SV_Position.xy * FC_ScreenSize.zw): o1.x = s/256-style `0.003906*FC_SpcParam.x*t35.x`,
  o1.y = 0.1*FC_TranslucencyScale*COLOR1.y, o1.w = t35.w when not alpha-flagged. OBSERVED (INFERRED: t35 is the first pass's G-buffer1 copy).

## 12. ColDifSpcBumpDirtMaskDOL  (`GXFlver_ColDifSpcBumpDirtMaskDOL_Gbuf.ppo`)
Same body: DirtMaskDOL_Gbuf, DirtMaskEnvDOL_Gbuf. Inputs: v3.w = COLOR1.w, v4.xy = UV0, v4.zw = mask UV (TEXCOORD6), v5.xy = lightmap UV (TEXCOORD7).
* Blend factor: the same smoothstep weight as MulMask (COLOR1.w modulated by t14 g_BlendMaskTexture at v4.zw, TTS14, `cb2:FC_BlendMaskParam.x/.z`). OBSERVED
* Only the diffuse (and alpha) is blended: o2.xyz = lerp(t0.rgb, t7.rgb, w) * FC_DifMapMulCol (t7 = dirt diffuse at UV0 with TTS7); o2.w = lerp(t0.a, t7.a, w) * inst.mulCol.w. OBSERVED
* o0, o1.x, o1.y, o1.z, o3, o4: same as the reference (single bump t3, single spec t1: dirt does not change F0, gloss or normal). OBSERVED

## 13. GXFlver_Col_Gbuf and GXFlver_ColDif_Gbuf  (no bump, no spec)
`Col_Gbuf` (same body: ColSh_Gbuf): unlit colour. Inputs: v1 normal, v2.w = COLOR1.w, v3.xy = UV (TEXCOORD4), v4.x front.
* o0.xyz = normalize(vertex normal)*0.5+0.5 (back-face flip), no bump. o1.x = 0, o1.y = 0, o1.z = ShLightMask.x. o2.xyz = 0. o3.xyz = 0. o4.xyz = 0.
* o5.xyz = cb2:FC_DynamicDiffuseColor.rgb * t0.rgb (TTS0); o5.w = F ? A : 0. OBSERVED. (colour goes to the emissive target.)
`ColDif_Gbuf`: o0 as Col; o1.x = 0, o1.y = 0, o1.z = ShLightMask.x; o2.xyz = cb2:FC_DynamicDiffuseColor.rgb * t0.rgb (not FC_DifMapMulCol); o3.xyz = 0 (F0 zero); o4.xyz = max(0, SH(N)) (no 1-F0 factor); no o5. OBSERVED

## 14. Grass: GrassSpcBumpDOL  (`GXFlver_GrassSpcBumpDOL_Gbuf.ppo`; `_[A]` read too)
Same body: GrassSpcBumpDOL_Gbuf, GrassSpcBumpEnvDOL_Gbuf. Compared with the reference: o1.y = 0 (COLOR1.y not used); everything else same as reference (o0 bump t3, o1.x t3.b*s^2, o2, o3, o4 lightmap, o1.z lightmap alpha). OBSERVED. `GrassSpcBump_Gbuf` is the Ibl body (section 1). `Grass_Gbuf`/`GrassDOL_Gbuf`: o1.x = 0 (no spec). The `[A]` twin adds the dither alpha test (section 0); the wind is in the .vpo (not read).

## 15. Parallax: ColDifSpcPomDOL  (`GXFlver_ColDifSpcPomDOL_Gbuf.ppo`)
Same body: ColDifSpcPomDOL_Gbuf, ColDifSpcPomEnvDOL_Gbuf. Extra inputs: v5 = TEXCOORD9 (clip/world position), cb8 (FC_MatrixInvViewProj, FC_MatrixInvView), cbMatDynParam FC_PomHeightScale (offset 64), FC_PomMinSamples (72), FC_PomMaxSamples (76).
* A view-ray march through the height stored in t3 .a (alpha of the bump map; `sample_d ... t3.xwyz`, loop with samples lerp(Min,Max, dot(V,N))), then an interpolated hit; the resulting UV offset is divided by TTS3.xy and added to UV0. OBSERVED
* The shifted UV is used for ALL material lookups: t3 bump (n and gloss), t1 spec, t0 diffuse. The lightmap UVs (t26/t27) are NOT shifted. Arithmetic of o0..o4 otherwise same as the reference (o1.y translucency present, o1.x = t3.b*s^2). OBSERVED
* `ColDifSpcPomMulMaskDOL` etc. combine POM with the Mul/MulMask layer blend (not read).

## 16. Map multi-layer families (no FC_DifMapMulCol, no FC_SpcParam!)
Observed structure shared by `MultiBlend{2,3,4}_et1*`, `FourBlend*`, `BloodBlend*`, `Rotten*`, `RockBlend*`, `SandBlend*`, `Fujir_MultiBlend2_*`:

### 16a. 2-layer: `MultiBlend2_et1__MultiBlend2_et1_Gbuf.ppo` (+ `_lightmap`)
Reflection has NO texture names (generic t2..t9) and only cbMtdParam FC_AlphaRef used. Material params in cbMatDynParam: `MultiBlend2_et1_snp_0_float_0_0` (offset 624 = P0), `..._float_1_0` (628 = P1), `..._float_2_0` (632 = P2).
Inputs: v1 normal, v2 tangent, v3.w = COLOR0.w (v1.w of the VS), v4.xy = UV0 (TEXCOORD0), v5.x = TEXCOORD1.x (non-lightmap) or TEXCOORD2.x (lightmap variant; the _lightmap UV is TEXCOORD0.zw), v6.x front face.
* Blend factor: w = saturate( ( W - (t9.r + 0.01)*P0*(1 - W) ) * (1 + P1*COLOR0.w) ), W = TEXCOORD1.x (a per-vertex blend weight straight from the vertex stream: .vpo `mov o5, v5`), t9 = a height/mask texture sampled at UV0*P2 with TTS0. OBSERVED. (Height-contrast blend.)
* Layer 0: diffuse t8 (TTS1), specular t3 (TTS5), bump t6 (TTS13). Layer 1: diffuse t4 (TTS2), specular t2 (TTS6), bump t5 (TTS14). OBSERVED
* o0.xyz: n from lerp(bump0.rgb, bump1.rgb, w) (one normal, built with the single vertex tangent frame). OBSERVED
* o1.x = lerp(t6.b, t5.b, w) (RAW bump blue, no FC_SpcParam, no squaring). o1.y = 0. o1.z = ShLightMask.x (lightmap version: t26.a). OBSERVED
* o2.xyz = lerp(t8.rgb, t4.rgb, w), NO FC_DifMapMulCol; o2.w = inst.mulCol.w (no texture alpha, no vertex alpha); o0.w/o1.w = F ? mulCol.w : 0. OBSERVED
* o3.xyz = sqrt(lerp(t3.rgb, t2.rgb, w)). o4.xyz = (1 - lerp spec) * max(0,SH(N)) (lightmap version: DOL form). OBSERVED
* 3-layer (MultiBlend3_et1): same with two weights; chain lerp(lerp(L0,L1,wx),L2,wy) (not read in detail beyond o1.x/o2.x text). INFERRED from the o1.x/o2.x chains.

### 16b. 4-layer: `MultiBlend4_et1__MultiBlend4_et1_Gbuf.ppo` (same body as `FourBlend__FourBlend_Gbuf.ppo`, whose reflection names the textures)
Weights from TEXCOORD1.xyz (v5.xyz, vertex stream straight through), 3 weights chained: wi = saturate( (Wi - (t17.r + 0.01)*P0*(1 - Wi)) * (1 + P1*COLOR0.w) ), t17 sampled at UV0*P2 (TTS0). OBSERVED
Chain: value = lerp(lerp(lerp(L0, L1, w0), L2, w1), L3, w2). Textures (slot, tiling row), FourBlend name in brackets:
* diffuse: L0 t16 (row 1) [Texture2D_1], L1 t8 (row 2) [_2], L2 t7 (row 3) [_3], L3 t6 (row 4) [_4]
* specular: L0 t5 (row 5) [_5], L1 t4 (row 6) [_6], L2 t3 (row 7) [_7], L3 t2 (row 8) [_8]
* bump: L0 t12 (row 13) [_13], L1 t11 (row 14) [_14], L2 t10 (row 15) [_15], L3 t9 (row 16) [_16]; mask/height t17 [_0].
* o0.xyz from the chained bump (rg), o1.x = chained bump .b (raw), o2.xyz = chained diffuse (no mulCol), o3.xyz = sqrt(chained spec), o4 = (1 - spec)*SH; alpha = inst.mulCol.w. OBSERVED
* Vertex programme (MultiBlend4_et1 .vpo): `mov o3,v1` (COLOR0), `mov o5,v5` (TEXCOORD1), `mul o4.xy, v4.xy, 1/2048`. So the blend weights are baked vertex data (texture-splat weights painted per vertex). OBSERVED
* `_lightmap` versions use TEXCOORD2 (`mov o5, v6`) for the weights and TEXCOORD0.zw for the lightmap UV. OBSERVED (2-layer lightmap .vpo and .ps read)
* Variants `_mp1`, `_mt1`, `_p`, `_e`, `_em`, `_nolight`: other mask/parameter sources for the weight (e.g. `t13`/`t14` texture masks, `cb2[39]`...), not decoded. `*_nolight*` write o1.x = 0 and no SH ambient. `_em` write o5. (names only)

## 17. Character skin: `GXFlver_SkinIbl_Gbuf.ppo`
Same as the Ibl body (section 1) EXCEPT o2.xyz:
* o2.xyz = FC_DifMapMulCol.rgb * ( saturate( cb7:FC_SkinMatchColor.rgb + sqrt(t0.rgb) ) )^2 (cb7 = cbModelParam offset 96). OBSERVED. o0, o1, o3, o4 as ColDifSpcBumpIbl (o1.y = 0).
Other skin names read: `SSSIbl_Gbuf` = Ibl body with o1.y = 0.1*FC_TranslucencyScale*COLOR1.y, o1.w = F ? A : cb2:FC_SSS_ID/255, o2.w = F ? A : t21.x (g_SSSMaskTexture, UV0 TTS0) instead of alpha. OBSERVED. `SkinIbl_Bloody`, `SkinSSSIbl_Bloody`, `FgFace*`, `Eye*`, `Hair*` carry the wet/blood blocks (sections 7, 8).
The pilot's reading (2026-11-03, R3c): `SkinIbl`, `SkinIbl_Bloody`, `SkinSSSIbl_Bloody` and their Phantom and [A] twins all apply `saturate(sqrt(t0.rgb) + cb7[6].rgb)²` (the _Bloody ones then the wet lerp toward FC_WetDiffuseColor and the blood decal, as section 7). Of the 676 programmes, 56 read cb7[6] (FC_SkinMatchColor): those ten, the `FgFace*` (the same curve on t0 and on the face image t7), the `Eye*` (`mad_sat cb7[6] × t0.a + sqrt(t0)`), the `Hair*` (read 2026-11-03: `saturate(sqrt(t0.rgb) × cb7[6].rgb)²` × FC_DifMapMulCol, the reference's F0 and gloss, the _Bloody and HairIbl2 bodies then the wet block) and `SAT_HairIbl2` (read 2026-11-03: its textures by rows, `saturate(sqrt(row 0 t3) × cb7[6])²` with no FC_DifMapMulCol, F0 row 2 t1, the bump row 1 t2 whose raw blue is the gloss, then the wet block and a blood decal t41 by FC_BloodAmount; `SAT_HairIbl2_cloth` the same body). The `FgFace*` read 2026-11-03: the face image t7 at UV1 (TEXCOORD0.zw, VC_TexScroll_1) lerped by the vertex COLOR0.w (the vertex programme passes it in the pixel programme's COLOR1.w), times COLOR0.rgb; `FgFaceIbl` curves t0 only, the _Bloody ones t0 and t7, then the wet block. OBSERVED. The colour is PS cbModelParam +0x60 = the draw state's +0x280: the draw group's +0x40, or the part's appearance colour CFDE70 copies into its stack copy of the group (09_model_admission.inc).

## 18. Equipment: `SAT_Equip`  (`SAT_Equip__SAT_Equip_Gbuf.ppo`; read by the pilot 2026-11-03)
Same body: `SAT_Equip`, `SAT_Equip_cloth` and their Phantom twins (code identical); `SAT_Equip_SSS` and `_SSS_cloth` the same with every texture one register later (t1 its SSS row 6) and o1.w the SSS id. Textures by name `SAT_Equip_snp_0_Texture2D_N`, sampled with tiling row N: 5 the diffuse (t1), 4 the bump (t2), 3 the specular (t3), 2 the damage detail normal (t4), 1 and 0 the rim glow (t5, t6). OBSERVED
* o2.xyz = t1.rgb (no FC_DifMapMulCol), then the wet lerp toward FC_WetDiffuseColor and the blood decal (t41) as section 7; o2.w = t1.a × mulCol.w.
* o3.xyz = sqrt(t3.rgb), wet and blood as section 7. o1.x = the bump's raw blue t2.b (no FC_SpcParam), wet-lerped toward FC_WetParam2.x.
* o0: the bump's rg (row 4) plus FC_DamagedNormalParam.x × the damage normal's rg (row 2 at UV0 × FC_DamagedNormalUvTiling.xy), z from the bump alone, normalised, then in the vertex frame as the reference.
* o5: a rim glow (t6 at a scrolled UV, t5, FC_Falloff_*), not a material.
* The depth programme `SAT_Equip_DptA` samples t1 (row 5) alone for its alpha; the vertex programme's UV0 = TEXCOORD0.xy / 2048 + VC_TexScroll_0.zw, the decal UV TEXCOORD0.zw / 2048 + VC_TexScroll_2.zw (disassembled from the I-0063 cache with fxc).

---------------------------------------------------------------------------------------------------------------------------------

## STEP 3. Programmes whose o1.x follows the reference gloss pattern

Method (OBSERVED, script `census\step3b.py`/`step3c.py`): symbolic o1.x of each of the 676 `*Gbuf*.ppo` files, with the alpha-test `movc(test,0,x)` wrapper of the `[A]` twins removed. Pattern = `(0.003906*cb3[2].x)^2 * X`.

| class | programmes (of 676) | meaning |
|---|---|---|
| A1: o1.x = s^2 * t3.z exactly (t3 = g_BumpmapTexture, the bump `.b`) | 92 (+2 where the bump is bound to t4: `ColDifSpc...` unnamed `Gbuf` pair) = 94 | reference pattern |
| A2: o1.x = s^2 * (blend of bump `.z` of several layers or another derived scalar) | 122 | same squared FC_SpcParam term, but the sampled value is lerp(t3.z, t10.z, w) (Mul/MulMask/EmiMul/Burn/Pom MulMask/FlowDisplacement) or a water fresnel mix |
| total with the squared FC_SpcParam term | 216 | 32 % |
| B/C: other | 460 | below |

Of the 460 "other": 260 wet/blood formula (s * lerp(G*s, WetParam2.x, w*WetParam2.z), section 7/8), 90 multi-layer raw bump `.z` blends without FC_SpcParam (section 16 families), 52 = constant 0 (unlit / nolight / Col*), 28 = raw single `tN.z` or `.x` without FC_SpcParam, 18 = `SnowLt_Gbuf2` (section 11), 10 = time-animated (Eclipse, Flowmap), 2 = water float object.
In per-family counts, the `[A]`, `Phantom`, `Cloth` twins are folded in (a family below = 1..8 programmes). Families that write o1.x another way (OBSERVED; 133 family-category entries, grouped by the nature of the formula instead of the 60-name cut, since a family stands for 1 to 8 programmes):

* constant 0: ColDif, ColSh, ColShZFeather, ColZFeather, Col, Emi, EmiGlow, Eclipse, Sword, Grass, GrassDOL, GrassEnvDOL, Veill, Veill_NoLight, Veill_Curtain, Fujir_Nolight_OpacityBlend, MultiBlend2_et1_nolight, MultiBlend2_et1_nolight_MuitiBlendMask, MultiBlend2_et1_nolight_MuitiBlendMask_OpacityMask, MultiBlend2_et1_nolight_MuitiBlendMask_Vtxalpha, MultiBlend3_et1_nolight, MultiBlend4_et1_nolight, MultiBlend4_et1_nolight_scroll (23 families, 52 programmes)
* raw tN.z with no FC_SpcParam: RockBlend, RockBlend_lightmap, SandBlend, SandBlend_lightmap, SandBlend_2, SandBlend_2_lightmap, SSSLightmap, SSSLightmap_lightmap, SATTest_ARN, SATTest_ARSN, SATTest_Burn, Shininess_B_Height_A_pom, pom, Fujir_[ARSN]_OpacityBlend (14)
* multi-layer raw bump `.z` lerp, no FC_SpcParam: MultiBlend2_et1, MultiBlend2_et1_lightmap, MultiBlend2_et1_e, MultiBlend2_et1_em, MultiBlend2_et1_em_Mask2, MultiBlend2_et1_mp1, MultiBlend2_et1_mt1, MultiBlend2_et1_p, MultiBlend3_et1 (and its _e, _mp1, _mt1, _p, _lightmap forms), MultiBlend4_et1 (and its _e, _mp1, _mt1, _p, _lightmap forms), FourBlend, FourBlend_lightmap, BloodBlend, BloodBlend_lightmap, Rotten, Rotten_lightmap, Crystal, BurnCloth, Fujir_MultiBlend2_BlendOpacity, Fujir_MultiBlend2_BlendOpacity_lightmap (44 families, 90 programmes)
* wet/blood blend (s*lerp(g*s, WetParam2.x, w*WetParam2.z)): ColDifSpcBumpIbl_Bloody, ColDifSpcBumpIbl_Bloody_Cm, ColDifSpcBumpIbl_Bloody_Df, ColDifSpcBumpIbl_Bloody_Df_Cm, ColDifSpcBumpIbl_Df, ColDifSpcBumpBlendIbl_Bloody, ColDifSpcBumpEmiIbl_Bloody, ColDifSpcBumpEmiIblGlow_Bloody, ColDifSpcBumpMulIbl_Bloody, ColDifSpcBumpMulMaskIbl_Bloody, ColDifSpcBumpTrIbl_Bloody, Equip, Equip_Cm, Equip_Df, Equip_Df_Cm, EyeIbl, EyeIbl_Bloody, FgFaceIbl_Bloody, FgFaceSSSIbl_Bloody, HairIbl2, HairIbl_Bloody, SkinIbl_Bloody, SkinSSSIbl_Bloody, SSSIbl_Bloody, SSSEmiIblGlow_Bloody, Dragon_FDP, Fur_FDP, RecursiveCrystal, c5280LastBoss_em, SAT_Equip, SAT_Equip_Df, SAT_Equip_SSS, SAT_Equip_Veill, SAT_Equip_em, SAT_HairIbl2, SAT_MultiBlend2_BlendOpacity_Bloody, SAT_MultiBlend2_BlendOpacity_Equip, SAT_MultiBlend2_BlendOpacity_Equip_Cm (38 families, 260 programmes)
* SnowLt second pass (0.003906*FC_SpcParam.x*t35.x): ColDifSpcBumpDOL_SnowLt_Gbuf2, ColDifSpcBumpEnvDOL_SnowLt_Gbuf2, ColDifSpcBumpIbl_SnowLt_Gbuf2, ColDifSpcBumpMulDOL_SnowLt_Gbuf2, ColDifSpcBumpMulEnvDOL_SnowLt_Gbuf2, ColDifSpcBumpMulIbl_SnowLt_Gbuf2, ColDifSpcBumpMulMaskDOL_SnowLt_Gbuf2, ColDifSpcBumpMulMaskEnvDOL_SnowLt_Gbuf2, ColDifSpcBumpMulMaskIbl_SnowLt_Gbuf2 (9)
* time-animated: Eclipse_cloth, Eclipse_grass, FlowmapVertexOffset, FlowmapVertexOffset_em (4); water float object: WaterBumpTriInteractionFloatObj (1)

Full per-programme classification: `census\step3_cls.json` (name -> class, formula, has-flow-control) and `census\step3_cats.json` (category -> families). The unique pixel-body count over the 676 files is 344.

---------------------------------------------------------------------------------------------------------------------------------

## STEP 4. `gxrenderershader__GXLightAcc_DDirLight1.ppo`  (file `census\asm\gxrenderershader__GXLightAcc_DDirLight1.ppo.asm`)

Targets: o0 = diffuse light accumulation (rgb), o1 = specular light accumulation (rgb); both .w = 1. OBSERVED
Resources: t0 g_LightPrepassAttrTexture (normal), t1 g_DepthTexture, t3 g_GBuffer1Texture, t5 g_GBuffer3Texture, t32 g_ShadowTexture; cb0 cbLightAccParam (inverse view-projection, camera), cb5 cbLight (indexed per light set, 18 rows per set, 36 rows total = 2 sets). Constants per set: Vec_i (rows 0..3, direction .xyz + a scalar .w), Col_i (4..7, diffuse colour), Spc_i (8..11, specular colour), flags (row 16 .x/.y enable lights 2/3), shadow-weight vector (row 17).

* Normal: t0.rgb*2-1, normalised (this is the "attribute" texture; its alpha is not read). Position: depth t1.x + screen xy through cb0[0..3] (div w); view vector V = normalize(camera - position). OBSERVED
* GBuffer1: `.x` -> specular power n = exp2(13*GB1.x) (the `exp` instruction is base 2). `.y` = translucency: if GB1.y > 0 an extra back-scatter term is added to the DIFFUSE output. `.z`, `.w` not read. OBSERVED
* GBuffer3: `.rgb` -> F0 = GB3.rgb^2 (r3 = GB3*GB3); `.w*255` (ftoi) = light-set index, row base = index*18 in cb5. OBSERVED  (GB3.rgb = sqrt(spec texture) as written by o3 in the G-buffer, so F0 = specular texture value.)
* Shadow: t32 sampled at the pixel (s8): r5.x = shadow, r5.y = second channel (.z of the texture, an extra factor). Per-light factor f_i = 1 + weight_i*(shadow-1) (weight_i = cb5[row17].i), i.e. lerp(1, shadow, weight_i). Light 0's diffuse and specular are additionally multiplied by t32 .z. OBSERVED
* Diffuse (lights 0 and 1, always; 2 and 3 when their flag != 0): out0 += Col_i * f_i * saturate(N.L_i) * (1 - F0).   **Correction: the factor is (1 - F0), not (1 - 0.5*F0)** (r1 = 1 - GB3^2; no 0.5). OBSERVED
  Lights 2 and 3 use a wrapped, squared Lambert instead: min(1, (N.L*0.5+0.5)^2) * (1 - F0) * Col_i * f_i. OBSERVED
* Translucency (GB1.y > 0, lights 0 and 1): out0 += GB1.y*10 * Col_i*f_i * [ saturate(L_i.(-V))^32 * ((max(V.N + 1, 0)*0.5)^2) ]. OBSERVED
* Specular (each light i): H = normalize(V + L_i); NdotH = saturate(N.H); n' = max(1, n*Vec_i.w / (n*0.01 + Vec_i.w))  (the same form as the reference point light, but with the scalar in the LIGHT DIRECTION row's `.w`, `cb5[row i].w`, not an Spc colour `.w`);
  spec_i = Spc_i * f_i * [(n' + 2)/8] * NdotH^n' * Fresnel * ( saturate(N.L) / max(N.V, saturate(N.L), 1e-4) ).   OBSERVED
  (n'*0.125 + 0.25 = (n'+2)/8: confirmed. NdotH^n' as exp2(log2(NdotH)*n'). Light 0 is also multiplied by t32 .z.)
* Fresnel: **Yes, Schlick**: F = F0 + (1 - F0) * (1 - saturate(V.H))^5 with F0 = GB3.rgb^2 (r10 = (1-VdotH)^5 via x^2, x^4, x^5; `mad ... r1(1-F0), r3(F0)`). OBSERVED. The visibility is the NdotL / max(NdotV, NdotL) form above, not a Smith term. INFERRED (not read from the point-light shader here): whether the reference point light has the same Fresnel is not confirmed by this file.
* Which light is "the sun": the shader does not say. INFERRED: light 0 of the set (the only one with the t32 .z factor and shadow factor f_0 first in order); the set is chosen by GB3.w*255 (the `envData/255` byte every G-buffer programme writes to o3.w).
* The albedo is NOT applied here (it is applied later; this pass outputs diffuse irradiance*(1-F0) and specular radiance). GB1.z (ShLightMask / lightmap alpha) is not used by this pass. OBSERVED

## Open points
* `ColDifSpcBumpIbl_Cm` does not exist; the `_Bloody_Cm` programmes are code-identical to `_Bloody`.
* Not decoded: the `_mp1/_mt1/_p/_e/_em` mask sources of the MultiBlend families, the 3-layer chain details, `ColDifSpcPomMulMask*`, `Water*`, `FlowDisplacement`, `Burn`, `Hair*` specular params (`FC_HairSpecularParam*`), `SAT_*` glow, vertex programmes other than the 6 read.
* The evaluator `symx.py` ignores `if`/`loop` control flow (linear last-write), so programmes with flow control (POM, [A]) were read by hand where it mattered.
