# Native decals by family (wave 1)

Thomas (2026-10-18, before the resolutions): native decals by family (blood, hit marks, burns, ground messages); to make blood
flow, strike the Fire Keeper (allowed, no consequence). STATE listed "decals" among what runs natively: wrong, only the CPU
requests were native (FABLE-REVIEW-PREP R-27; the specification of the world pass is
[DECALS-NATIVE](../I-0068-static-yebis-ffx-lights-spec/DECALS-NATIVE.md)).

Labels: OBSERVED (a run), STATIC (code read), INFERRED.

## 1. The families in the original (OBSERVED, Reference2106-2117)

Probe: `decal-observe.flag` (both modes). Reference: the manager draw 51AA0 (its list `+0x68..+0x70` on each change), the
per-decal draw 198910 (first draw of each kind), the character patch 51F80 and the paint-target clear 4CD70. Native: the
object of each completed kind-0 request (`decal_observe_request`).

| Family | What the original does | Witness |
|---|---|---|
| World decals (gxdecal, kind 0, vtable 3D5B8D8) | a physical hit on a character leaves a blood decal on the ground: blend mode 1, width scale 13.33, textures +0x190 diffuse, +0x198 specular, +0x1A8 bump, +0x1C0 mask (mask 0x4B), blood flag +0x258 = 0: programme `Decal_DifSpcBumpAm` (§2.1 of DECALS-NATIVE: bump and specular, mask). They persist: 4 after 6 punches (Reference2113), drawn every frame by 198910 | Alpha punching the Fire Keeper (left fist, Reference2113; by hand, Thomas, Reference2117) |
| Character paint (kind 1, vtable 3D5B978, 0x140 bytes, every scene slot a bare RET) | the hit is painted onto the struck character: a `CSDecalPaintRenderTarget` (NS_SPRJ) is created lazily on the first hit ("Draw.Decal.PaintRenderTargetLazyInitialization") and cleared (4CD70, 8 clears for one struck NPC); its `GXPaintRenderTarget` (GXSR, 1024x1024 R16 masks "PaintRT Diffuse/Specular/Height") feeds the character's `GXBloodyMaterialPatch` (t41/t42 of the `_Bloody` materials). Fire (Fire Orb on the Shrine Handmaid, Reference2109) paints too (burns), with no world decal | Reference2109, 2113, 2117 |
| Fire hits on walls and floors | nothing: the Fire Orb impacts leave no decal (Reference2106-2108) | — |
| Ground messages, bloodstains | FFX/SFX objects (MENUS-MAP "Bloodstain and message signs"), the native effect family; to witness only | to do |

At Firelink with the zone saves the manager list is empty before any hit (no static map decal there).

Native (Native2114-2118): the same kind-0 objects are created by the native CPU requests (same fields, 56 completions in
Native2118), but nothing draws them (the scene consumer refuses, the GPU slots of the manager are zeroed), and no paint
target is ever created or cleared: the struck characters keep the neutral decal (1184).

## 2. Plan

D1. World decals natively: a D3D12 decal stage after the G-buffer, on the native data path (DECALS-NATIVE §3): the receiver
stencil bit, the G-buffer copies, the decal objects from the native registry, constants computed natively from the objects
(§2.2), the original's DXBC programmes, seven targets; checked against a Reference oracle of 198910 (its constant bytes,
textures and states) and side by side at the Fire Keeper.

D2. Character paint: the paint pass into each struck character's paint target and the `_Bloody` materials reading it.

D3. Ground messages and bloodstains: witness the FFX family.

## 3. Frames in flight of the original (Thomas's question, open)

Not yet read: a Reference probe of the swap chain's buffer count and the device's maximum frame latency (Reference2116
crashed on a wrong pointer, Reference2117 found no swap chain at the expected place). D3D11's default maximum frame
latency is 3 (DXGI documentation); DS3's own value is INFERRED until read.

## 4. D1, the world decals natively (implementation)

- **Collect** (`SnapshotNativeDecals`, engine thread, main collect): the decal option `[[GXRS+0x5360]+0x68]`; every registered
  manager's kind-0 objects with a material (`+0x270`), in list order: the object's 0x420 bytes, the material's 0x190 bytes, the
  manager settings `+0x400..+0x42F`, the programme tables, the texture records of `+0x190..+0x200` (an SprjGxTexture forwards
  to its GXTexture2D `+0x10`), and the cbCubeEnvSH1 rows 59470 blends from the material's light banks (`+0xF8`/`+0x100`,
  t `+0x108`: A at t <= FLT_EPSILON, B at t >= 1, else `(b-a)t+a`; STATIC, Sonnet read of 59470 L175-248).
- **Constants**: `ds3content::ComputeDecalConstants` (`native/content/decal_constants.*`), the transcription of 198910
  (programme selection, VS b10, PS b10, PS b2; checked geometrically on the 49 decals of Reference2124: the centre maps through
  VC_DecalTransform to the same clip point as through the view-projection, 7e-6; FC_ProjToDecalSpace inverts the widened cube,
  4e-5). The camera-inside test is an exact convex clip in place of the original's SIMD chain (INFERRED equivalent).
  PS b7 is `ProduceModelParamPs` over the 1A8580 state (material at `+0x240`; FC_DifColMul = material `+0xD0`, STATIC 1A86C0).
  The view inputs are the native camera's (pose inverse, the camera block's FC_MatrixProj, lens, render size).
- **Draw** (`BuildNativeDecalDraws`, main lit view): the original DXBC pair, the 50720 cube as a 12-triangle list, PS SRVs by
  198910's 18 slots (t4 depth copy, t13 GB0 copy, t14 GB1 copy), samplers by the 10AB80 bank map, the observed states
  (DECALS-NATIVE 1). RT2's blend by `+0x208`: mode 1 premultiplied, others multiply (mode 1 OBSERVED, Reference2134; modes 0 and 2 INFERRED). POM programmes (PS b2) are refused (none observed).
- **Receivers**: the G-buffer stencil reference keeps bit 0 only for non-character models whose material does not set
  `g_GX_bDisableDecal` (140850; the character test is the model name's `c`).

## 5. D1 results (2026-10-18)

- **Oracle** (Reference2131, 2134; `decal-observe.flag`): the GX wrappers receive GX objects whose D3D11 object sits in their
  first bytes; the oracle now finds it by QueryInterface (`D3d11As`). OBSERVED for programme 16 (`Decal_DifSpcAm`, the punch
  blood): RT0 SRC_ALPHA/INV_SRC_ALPHA alpha ZERO/ONE mask RGB, RT1 the same mask RG, **RT2 ONE/INV_SRC_ALPHA for blend mode 1**
  (the premultiplied mode B: the native choice holds), RT3-RT7 as RT0; depth on, no write, LESS_EQUAL, stencil read 1 EQUAL,
  ref 1; cull front, front counter-clockwise; t3 the 8x8 BC7 default, t4 the R32G8X24 depth copy, t13 RGBA16F, t14 RGBA8,
  t17 a 512x512 BC4 mask; **PS b11 all zero**, PS b7 FC_DifColMul (1,1,1,1).
- **Constants**: `ComputeDecalConstants` against the 27 captured draws: VS b10, PS b10 and PS b2 floats 16-19 bit-identical
  (two summation orders of the matrix helpers 1CE40 and 4BCA0 taken from the disassembly). Not covered by the oracle: box,
  cubic, blood-set and POM programmes, the emissive curve, the camera inside a volume.
- **Native** (Native2132-2133, Alpha punching the Fire Keeper by the pilot): 1 to 4 decals drawn per frame, no refusal; the
  punch blood is barely visible on that floor in both modes. Thomas (play with Argo, 13:24-13:30 Paris): the blood on the
  ground looks as the original's. A frame-graph declaration order fault of the copies pass (logged every frame) fixed.
- **Open**: D2, the paint (the struck character's blood and burns: `CSDecalPaintRenderTarget` and the `_Bloody` materials'
  t41/t42), and D3 (ground messages, bloodstains); the camera-inside states (INFERRED); `decal-debug.flag` (the albedo takes
  the decal colour outright) for coverage checks.
