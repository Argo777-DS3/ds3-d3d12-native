# I-0113 gap 7, the Undead Settlement wheels: static study

Study thread, 2026-10-30 (Paris, evening), for the I-0113 pilot. Read-only: the game's bytes recovered by the pilot
(`Tools/DS3-Decompilation/gamedata-i0113/o001279`), the material bank `allmaterialbnd.mtdbnd.dcx` (I-0042 backup), the
pilot's native and original journals and G-buffer dumps (`i113-m31b-*`, `i113-m31c-*`), GhidraMCP on DarkSoulsIII.exe 1.15.2
(FUN_140141070), and `native/`. No run, no build. The readers are in the scratchpad of the study thread (FLVER, DDS, TPF, MTD and
G-buffer probes, a few dozen lines each); the numbers below are their output.

## Answer in short

Every input the wheel's G-buffer row takes is the same natively as in the original, as far as the data and the journals show:
vertex colour, textures and their formats, MTD parameters, light key, light set and instance SH rows. The native G-buffer at the
wheel is coherent with those inputs. What darkens the wheel in the original is therefore not in the asset data; the candidates
left are in the draw itself (a constant the original's character-path draw sets for an object, or a programme variant), and
they need the original's draw read live (section 4). The five GX items the native does not parse are zero UV scroll speeds (§2).

## 1. Eliminated

- **Vertex colour** (the pilot's first hypothesis): both FLVERs (`o001279`, 1,037 vertices; `o001279_1`, 5,135) carry one
  colour stream, `COLOR1` (type 0x13, four bytes), the same value on every vertex: (1, 0, 1, 1), magenta. No baked darkening.
  The native takes DS3's own input layout (`DecodeModelVertexLayout`, 192F090's conversion) and the original DXBC, so the stream
  reaches the same shader input in both. OBSERVED (the FLVER bytes).
- **The `_1` model**: `o001279_1` is the broken cart (4 bones, its own Havok file `o001279_1.hkx`), same material `M[ARSN]`, same
  four textures, same GX items. The intact `o001279` is the one both renderers draw at the bonfire (`lod_step` rows equal:
  `o001279/disp0`, lod [0, 0, 1], in `i113-m31b-native` and `-reference`). OBSERVED.
- **Texture formats**: `o001279.tpf` holds `_a` (TPF format 0, DDS DX10 header BC1_UNORM_SRGB, 1024²), `_r` (format 0,
  BC1_UNORM_SRGB, 256²), `_s` (format 103, ATI1 = BC4_UNORM, 512²), `_n` (format 109, DXT1 = BC1_UNORM, 1024²). The native's DDS
  reader takes these declared formats (`dds_resource.cpp`). OBSERVED.
- **Texture contents against the native G-buffer** (`i113-m31b-native`, frame 1800, 1280x720):
  - `_a` mean sRGB (135, 118, 99), light grey-brown wood; GB2 (sRGB target) on the spokes and rim (120-156, 110-137, 94-123),
    on the iron hub (60-67, 48-57, 36-38): the texture's values. OBSERVED.
  - `_r` is 85 % one value, sRGB 60 (F0 0.045 once decoded); the hub reaches 90-99. GB3.rgb at the wood 54 = 255 x sqrt(0.045),
    at the hub 90 = 255 x sqrt(0.12): GB3 stores sqrt(F0) of the sRGB-decoded `_r`, coherent. The rocks, posts, planks and beams
    beside it write the same 54. INFERRED (the encoding read from the values; DECALS-PAINT-READY's « g sqrt(reflectance) » agrees).
  - `_s` has real data (mean 9.9 of 255). It is never in a `native_texture_white_fallback` row in any journal (o001280's
    `_s`, the other woodencart, is in all the m31 runs; many m31 map `_s` paths too, the original's loader binding the same
    default for those). Caveat: the witness stops at 96 paths and `i113-m31c-native` reached 96. OBSERVED.
- **Material parameters**: `M[ARSN].mtd` names `GXFlver_ColDifSpcBumpIbl.spx`, `[MAP/OBJ]`, and the usual values
  (g_SSAOFactor 1, g_DiffuseMapColor 1, g_SpecularMapColor 1, g_SpecularPower 256, g_BaseReflectance 0.04,
  g_AmbientReflectionRate 0.5, g_DetailBumpCoordScale (32, 32), g_TessellationMode 0, g_DisplacementEnable 0): a plain shared
  map and object MTD, nothing specific to this object (the neighbours' MTDs not checked). OBSERVED.
- **Light key, light set, SH**: the original's `reference_model_light_key` rows for o001279 name {131000000, 1} (three
  instances) and {131000000, 0} (two); the native's `native_model_light_key` the same keys, set 4 and set 2. The words of
  {131000000, 0} (native entry 2 against `reference_light_set` entry 0) and of {131000000, 1} (native entry 4 against reference
  entry 1) are equal word for word. The `native_instance_rows` of o001279 equal the original's `reference_instance_rows` to the
  digit. Not the chair's case. OBSERVED.

## 2. What the native does not reproduce for this material

FUN_140141070, the GX item parser, does not run natively (`18_resource_admission.inc`, C2): the native writes GX00, 01, 03, 05,
06, 07, 09, 13, 14 and GX04/GX90 itself. The wheel carries GX00 (zero), GX04 and five items the native skips:

| Item | Data (words) | What 141070 writes |
|---|---|---|
| GX04 | (0, 0, 5, 1, 0.5, 0, 5, 0.5, 1, …) | +0x254 = 0 (no displacement), +0x250 = 0 (no tessellation): flat, native and original alike |
| GX20 | (0, 0, 0) | +0x284 \|= 0x1, +0x288 = 0, +0x28C = words 0..1 |
| GX22 | (0, 0, 0) | +0x284 \|= 0x4, +0x2B0 = 1, +0x2B4 = words 0..1 |
| GX24 | (0, 0, 0) | +0x284 \|= 0x10, +0x2D8 = 2, +0x2DC = words 0..1 |
| GX26 | (0, 0, 0) | +0x284 \|= 0x40, +0x300 = 3, +0x304 = words 0..1 |
| GX30 | (0, 0, 0) | +0x284 \|= 0x800, +0x364 = 5, +0x368 = words 0..1 |

**What they are (settled, OBSERVED, decompiled)**: per-texture-slot UV scroll speeds. The material's serializer 1402306C0
walks the 16 bits of +0x284 (its +0x1B4 from the +0xD0 base), names entry i from the table at 0x14473E6F0 and writes the entry
at +0x288 + 20·i through 140243F60, whose one field is « Scroll Speed », two floats. The names in bit order: Albedo1, Albedo2,
Reflectance1, Reflectance2, Shininess1, Shininess2, Normal1, Normal2, Normal3, DetailNormal1, DetailNormal2, Displacement1,
Displacement2, Emissive1, Emissive2, BlendMask. So the wheel's five items are « Albedo1, Reflectance1, Shininess1, Normal1,
Displacement1: scroll speed (0, 0) »: no scroll, nothing to darken. 22E930 (cbMatDynParam) reads neither the mask nor the
entries. The headless scan (every `[reg + D]` and `lea` for D in +0x284 and the entries' offsets, mirror `ghidra-mirror-i0068`,
read-only) finds no other reader in the material code (0x140130000-0x140240000); `native/` has its own texture-scroll path
(`content/texture_tiling.cpp`), not checked against these items (moot here, the speeds being zero). **GX2x eliminated.**
The map pieces beside the wheel were not checked for the same items.

## 3. The native G-buffer at the wheel and its neighbours

`i113-m31b-native` frame 1800 (three samples each; GB3.w the light set):

| Point | GB1.r | GB2 (albedo) | GB3 | GB4 (ambient) | HDR |
|---|---|---|---|---|---|
| wheel rim (925, 560) | 111-112 | 120-136, 111-127, 98-112 | 54, w 2 | 0.0004 | 0.022-0.041, 0.006-0.012, 0.004-0.007 |
| wheel spoke (1060, 462) | 70-147 | 128-156, 110-137, 94-123 | 54, w 2 | 0.006-0.010 | 0.009-0.010, 0.004-0.005, 0.004 |
| wheel hub (1013, 510) | 132 | 60-67, 48-57, 36-38 | 90, w 2 | 0.003-0.005 | 0.009-0.013 |
| post (800, 300) | 87-102 | 92-102, 84-93, 71-79, **a 0** | 54, w 2 | **0** | 0.10, 0.03, 0.02 |
| rock (1000, 250) | 150-191 | 152-202, … | 54, w 2 | 0.002-0.004 | 0.016-0.042 |
| plank (600, 650) | 18-26 (b 40-49) | 69-79, 55-67, 43-53 | 54, w 2 | 0.011-0.019 | 0.015-0.019 |
| player (640, 420) | 49-54 (b 255) | 147-151, …, a 20 | 63-64, w 1 | 0.005-0.010 | 0.085-0.129 |

The wheel's albedo is the lightest wood in view by a wide margin (the posts 92-102), which is its texture: in the original the
same texture must come out darker, so the darkening is in the lighting terms the original applies to this draw, or in a
programme output the native writes differently. GB1.r is the shininess the programme writes (bump.b × FC_ShininessScale in
the decal pass's reading, DECALS-NATIVE; not checked for this programme).

## 4. What would decide it (live, the pilot's)

1. **The original's draw of o001279**: its pixel shader, its constant buffers (the material block and the instance block, in
   particular the words the character path sets for an object: the chair went through the same path) and its SRVs, read at
   the wheel's G-buffer draw: a Reference witness on the draw whose model is `o001279`, or RenderDoc on the D3D11 original
   (not installed: Thomas's call). Natively, `!o001279`'s `native_named_draw_sources` in the same view. A word that differs is
   the cause.
2. (Dropped: §2 settles the five items as zero scroll speeds.)
3. **Other character-path objects** in the same view against the original (o001280, the second cart, if in view; the pots
   o314010): if they are lighter too, the cause is the path, not this material.

The static side is exhausted for this gap: what is left is in the draw (the original's constants and SRVs at the wheel's
G-buffer draw), item 1 or item 3.

## 5. After the live reading `i113-s12-wheel-original` (study thread, 2026-10-30 evening)

The pilot's oracle at the original's G-buffer draw of o001279 (`63CB0` → `DrawIndexedInstanced`, 2 instances, 3,114 indices) saw
five pixel SRVs bound (t0 1024² BC1, t1 256² BC1, t2 512² BC4 the `_s`, t3 1024² BC1 the normal, t5 a 1x1 B8G8R8A8) where the
native reads t0, t1 and t3, and PS b3 equal but for words 14, 16 and 18 (1307.9, 1309.8, 1307.1 against 0). The two G-buffer
pixel programmes of the family, read from the game's `data1/shader/gxflvershader.shaderbnd.dcx` (recovered read-only with
`Recover-GameDataByName.ps1`, their RDEF parsed; the same bindings as I-0063's `program-resources.tsv`), OBSERVED:

| Programme | Textures read | Constant buffers |
|---|---|---|
| `GXFlver_ColDifSpcBumpIbl_Gbuf.ppo` | t0 `g_DiffuseTexture`, t1 `g_SpecularTexture`, t3 `g_BumpmapTexture` (samplers s0-s2) | b1 `cbTextureTilingScale`, b3 `cbMtdParam`, b4 `cbInstanceData` |
| `GXFlver_ColDifSpcBumpIbl_Gbuf_[A].ppo` | the same plus t38 `g_DitherTexture` (s8) | the same plus b2 `cbMatDynParam` (`FC_isDitherAlpha` only used) and b7 `cbModelParam` |

- **t2 and t5 are bound but read by neither programme**: the material binds every texture of its set (the `_s` and the
  displacement slot's default, the 1x1: INFERRED from the FLVER's `dummy128.tga`, which has no data) whatever the programme reads. Not the cause.
- **b3's words 14, 16 and 18 are `FC_SSAOParam.z`, `FC_CDRFParam.x` and `FC_CDRFParam.z`, flagged unused in both programmes'
  RDEF**. The values (about 1,308) look like the low halves of pointers left in the buffer (the oracle's b1 shows such residue
  after its used rows too, INFERRED). Not the cause. The words both programmes use are `FC_DifMapMulCol` (0-3), `FC_SpcParam`
  (8-11) and `FC_AlphaRef` (20-23), equal per the pilot.
- **What is left among the programme's inputs**: b4 `VC_InstanceData` (532 bytes a draw, used: the instance rows), b1's first
  rows (the used tiling scales of t0, t1, t3), the three samplers' states, and **which of the two programmes runs**: the
  original's b2 starts with `FC_isDitherAlpha = 1` (the oracle's `ps_cb2` first word), which only the `_[A]` programme reads (an
  alpha test with a dither pattern); if the original draws the wheel with `_[A]` and a dither in progress, some of its pixels are
  discarded where the native's `Gbuf.ppo` (recipe 0) writes them all. The oracle can tell the two apart by the bound pixel
  shader's bytecode hash (I-0063 `shader.tsv`: `Gbuf.ppo` `43B0C3C8…30F9`, `Gbuf_[A].ppo` `B6B75351…BBCF0`) or by whether t38 is
  bound.

## 6. b4 `VC_InstanceData`: layout, what `Gbuf.ppo` reads, who fills it (study thread, 2026-10-30, 21:30)

From `Gbuf.ppo`'s RDEF and its disassembly (`D3DDisassemble`, `d3dcompiler_47`), in `shader/gxflvershader.shaderbnd.dcx`.

- **Layout**: `cbInstanceData { PerInstanceData VC_InstanceData[2]; }`, 532 bytes: two elements of 68 words (272 bytes, the last
  one unpadded). By word of an element:

| Words | Field | Read by `Gbuf.ppo` |
|---|---|---|
| 0-11 | `mWorld` (float4x3) | no (vertex shader) |
| 12-15 | `atlasUvScaleOffset` | no |
| 16-19 | `userClipPlane` | no |
| 20-23 | `matricesData` (uint4) | no |
| 24 | `envData` (uint) | **yes**: `GB3.w = envData / 255` (unless `cb3[5].z`, `FC_AlphaRef.z`, is not 0: then `mulCol.w × albedo.a`) |
| 25 | `dynamicLightDisable` (bool) | no |
| 26-27 | `envPad` (uint2) | no |
| 28-31 | `mulCol` | **word 31 only**: `GB2.w = mulCol.w × albedo.a` |
| 32-43 | `aSh[3]` (three float4) | **yes**: `GB4.xyz = max(dot4(normal, 1; aSh[i]), 0) × (1 - specular)` |
| 44-47 | `ShLightMask` | **word 44 only**: `GB1.z = ShLightMask.x` |
| 48-51 | `vDominantRotateParam` | no |
| 52-55 | `parallaxCorrectedCubemapOrigin_envMapInterpRatio` | no |
| 56-59 | `parallaxCorrectedCubemapAabbMin_envMapColorScale0` | no |
| 60-63 | `parallaxCorrectedCubemapAabbMax_envMapColorScale1` | no |
| 64 | `ditherFadeAlpha` | no (the `_[A]` programme's) |
| 65-67 | padding of element 0 | - |

- **The instance index is not `SV_InstanceID`**: the pixel shader takes `round(TEXCOORD0.w)` (its input signature has no
  `SV_InstanceID`), times 17 rows, and indexes `cb4` dynamically. The vertex shader passes the index; with two elements a
  constant buffer, a draw holds at most two instances for this programme.
- **Who fills it in the original**: `FUN_14005B3D0` (`decompiled/DS3-1152-F-0005B3D0.c`, about lines 561-581), the per-draw
  instance writer. `envData` (byte 0x60) is written only when the draw state's type (`[state+0x1C]`) is 6 or 7: a lookup
  `14014DA10` by the state's key (`+0xF8`) in `[root+0xA58]` (or the table `[root+0x4200]` when `[root+0x42A0]`); **otherwise
  `envData = 0`**. `ditherFadeAlpha` (0x100) is `[state+0x124]`; `dynamicLightDisable` (0x64) is `[group+0x128] != 0`.
- **The native contrast**: `ProduceInstanceRecord` (`native/world/producers.cpp:121`) writes 0x60 only when `type_value` is set,
  the same rule; if the native gives the wheel a non-zero `envData` (its light set) where the original's draw state is not of
  type 6 or 7, `GB3.w` differs (`envData / 255`) and so does the lighting pass's choice of the environment light set for those
  pixels (INFERRED: what set 0 means in the lighting pass was not read). The pilot's word dump should be read against the table
  above: word 24 is `envData`, word 25 `dynamicLightDisable` (a bool: a 4 there is not a valid value for it, so a "0, 4, 0, 0"
  at words 24-27 is more likely an off-by-one with 4 in `envData`).
