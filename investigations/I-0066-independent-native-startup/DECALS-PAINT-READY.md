# D2, the character paint (blood and burns on a struck character): ready to code

Request (coordinator, 2026-10-18): specify family D2 of [DECALS-WAVE1](DECALS-WAVE1.md) (the `CSDecalPaintRenderTarget` / `GXPaintRenderTarget`
paint and the `_Bloody` materials' t41/t42) so that a struck NPC shows blood natively. D1 (world decals) is done and is the template.

Labels: **STATIC** (code or shader read, address given), **OBSERVED** (a retained run), **INFERRED**. Addresses are RVAs of
`DarkSoulsIII.exe` 1.15.2.0 (`decompiled/DS3-1152-F-<RVA>.c` or a read-only Ghidra query); `file:line` are in this worktree at the time of
reading (`game_bootstrap.cpp` is edited by other sessions: search the function name, not the line). Shader facts come from the DXBC of the
I-0063 programme cache (`C:\Users\User\.codex\I0063-cache\programs`, disassembled with `D3DDisassemble`; nothing licensed is copied here).

## 0. Verdict

- The paint pass is **fully in plain, analysable code** (no Arxan region): the receiver `GXSR::GXDecalReceiver` (5E990, 5F430, 5FCD0),
  the render target `GXSR::GXPaintRenderTarget` (4C3B0, 4C440, 4CFB0, 4C900, 4CB50, 4CD70) and the paint draw 1AAA70 / 1AB370 / 1AC390.
- **Settles an open question of the MTD-flag sheet**: shader passes 8 `Pnt` and 9 `PntCube` (table `14473CE60`) are the **paint**
  passes, not point-light shadows (MTD-FLAG-BITS-INVENTORY-SHEET row 2 said "names do not decide"). STATIC: the pixel programme
  `GXFlver_Common_Pnt` has the constant buffer `cbPaintParam`, and its vertex programmes rasterise the mesh in **UV space**.
- The struck character's materials already read the target: the `_Bloody` G-buffer pixel programmes sample `g_DecalDiffuseTexture` (t41)
  and `g_DecalSpecularTexture` (t42) on the mesh's **second UV set**. A cleared paint target is bit-identical to the neutral decal
  natively bound today (`absent_decal`, (0,0,0,1)): the plumbing rung can be proved a no-op before any paint is drawn.
- Not what the request text assumed: the paint colour targets are **RGBA8** (diffuse `0x1D` = R8G8B8A8_UNORM_SRGB, specular and height
  `0x1C` = R8G8B8A8_UNORM; `1419294A0` maps GX format ids to themselves, so they equal the DXGI values), the 16-bit surfaces are the
  **D16** depth maps (`0x37`). 1024 is only the default of the eager constructor; the NPC's size is a per-entity short (section 1.2).

## 1. The original flow, end to end

### 1.1 Objects (STATIC unless marked)

| Object | Class (RTTI) | Layout / role |
|---|---|---|
| paint request | `GXPaintDecal` vt `3D5B978`, 0x140 bytes (kind 1 of `51BF0`) | `+0x18..+0x12F` = the 0x118-byte descriptor (copied by `19BA40`, finite checks only); `+0x138` handle; `+0x13C` pending-paint count; `+0x10` byte "painted"; every scene slot is a bare RET |
| manager | `GXDecalManager` vt `3D20460` | `+0x20..+0x28` vector of 0x30-byte **paint queue entries** {receiver, handle, +0x18 type, +0x1C 5 floats}; `+0x40..+0x48` all receivers; `+0x60` "clear all" flag; `+0x408` per-frame paint budget; `+0x4A0`, `+0x500..+0x508`, `+0x50C/+0x50D` debug-menu requests (`522B0`) |
| receiver | `GXDecalReceiver` vt `3D41DD8` (+ sub-object `3D41DA8` at `+0x10`), 0x80 bytes, built by `51F80` (`E000E0`) | `+0x20..+0x38` four `GXPaintRenderTarget*` slots (`14005EC00` sets slot i); `+0x40` the entity's draw object (`entity+0xD0`); `+0x50` a pointer to its `GXBloodyMaterialPatch` (vt `3D41DF0`, 0x10 bytes allocated by `5E990`, back pointer `+8` = receiver); `+0x58` index of the model texture named `*_bloodmask` (`5EC10`); `+0x5C` paint flag; `+0x60` eager RT; `+0x68` `GXPaintVertexBuffer` (vt `3D5C588`); `+0x70` `GXDecalVertexStreamPatch` (vt `3D41DB8`); `+0x78` = model flags `+0x224` bit 2, `+0x79` = bit 3 |
| render target | `GXPaintRenderTarget` vt `3D1F6F8`, 0xB8 bytes (`4C3B0`) | `+0x08/+0x10/+0x28` SRV objects diffuse / specular / height; `+0x18/+0x20/+0x30` the three textures; `+0x38`, `+0x40`, `+0x48` depth surfaces; `+0x50` initialised byte; `+0x5C` pool flag; `+0x60..+0x78` creation descriptor (size, projector depth size, formats); vt `+0x20` = **clear** (`4CD70`) |
| holder | `CSDecalPaintRenderTarget` (NS_SPRJ) vt `142945328`, 0x20 bytes | refcount `+8`, `+0x10` the `GXPaintRenderTarget*`; made by `140CEDE20` -> `140CEDD20` / `140CEE040` |

### 1.2 Lifetime (STATIC, OBSERVED where marked)

1. **Entity construction** (`D05C80`, lines 137-153 of its export; the same shape in `CFDBA0` and `D3C4D0` for part items): if the init
   parameters carry a paint size (`short` at `params+0x1E` > 0, gate `*(short*)(row+6) > 0` through `D01D20`) **and** `1400600A0(modelData)`
   is true (model flag `+0x224` bit 2 or 3, or a material name containing `_Bloody` / `_DeepSnow`), it creates the receiver
   (`E000E0` -> `51F80`, stored at `entity+0xA60`) and the holder (`E000F0` -> `CF9610` -> `CEDF30` -> `CEE040`, size = `short entity+0xCE`
   twice, stored at `entity+0xA68`), then `14005EC00(receiver, 0, holder->+0x10)`: **only slot 0** is filled.
2. `51F80`'s fourth argument (the eager paint flag) is **0 for every caller** (OBSERVED: `decal_observe_patch` rows of Reference2109 and
   2113, 31 patches, all `paint:0`), so `5E990` does not build its eager 1024x1024 target; the holder's target is built with
   `4C440(..., lazy)`; lazy when `StageRend+0xCD` (the "PaintRenderTargetLazyInitialization" option, INFERRED to be the default) is set.
3. **Lazy GPU creation**: every consumer starts with `if(rt+0x50 == 0) 4CFB0(rt, ctx, mask)` (`4C900`, `4CB50`, `1AAA70`, `1AC390`). `4CFB0` makes
   (bit 0) the diffuse texture format `0x1D`, (bit 1) the specular `0x1C`, (bit 2) the height `0x1C`, each `w x h`, bind render target + shader
   resource (`14002BD50`, flags `0x28`), named "PaintRT Diffuse" / "PaintRT Spec" / "PaintRT Height" (`143D1F7C0/808/848`); `+0x38` is a **pooled**
   depth-stencil `w x h` format `0x14` (D32_FLOAT_S8X24, `140077640`), `+0x40` a pooled **D16 `0x200 x 0x200`** projector depth map (`140077880`,
   descriptor `+0x68/+0x6C = 0x200` from `CEDD20`), `+0x48` an optional pooled cube depth (`140077AE0`, size `+0x74`).
   Only the colour textures are per entity; the depth surfaces come from a shared pool keyed by size and format.
4. **Clear** (`4CD70`, STATIC): diffuse and specular to **(0,0,0,1)**, height to **(1,1,1,1)**, depth-stencil `+0x38` to depth 1.0 (flags 3).
   Issued as a type-0 command with handle 0 (`5F430`), de-duplicated per distinct target; also by type 2 with a mask.
   OBSERVED: `decal_observe_paint_clear` 8 rows in Reference2113 (punches on the Fire Keeper), 1 row in Reference2109 (Fire Orb, Shrine Handmaid).
5. **Free**: with the receiver (`51F80`'s object, `5E940` -> `5EB20`) and the holder's refcount when the entity is released (`140E00260`).
   Nothing frees a target between hits.

### 1.3 A hit (STATIC)

```
punch / spell -> kind-1 request (51BF0 -> 19BA40, descriptor kept) -> synchronous world query -> receiver vt+8 (5F114/5F120)
   if object.kind (+0x50) == 1:  queue {receiver, handle, type 0} on manager+0x20, ++object+0x13C        (also 52220)
manager per-frame consumer 1400522B0 (a render-pass slot; never reached natively):
   +0x60 flag  : type-0 handle-0 command for every receiver  (clear)
   +0x50C flag : type 1 queue entries (5F1D0), +0x50D : type 2 (5F170)       (debug-menu driven)
   for the first +0x408 queue entries: 5F250(receiver, cmdlist, handle, type, floats)  -> render command {receiver+0x10, handle, {handle, draw object, type, floats}}
receiver sub-object 5F430(cmd):
   type 0, handle 0   : clear the four slots (4CD70)
   type 0, handle != 0: 5FCD0 -> paint 1AAA70            <-- the paint draw
   type 1             : flag +0x24 set: erase pass 1AAF90 (PntCol); else 5F620 = blit of the model's "*_bloodmask" texture into the target
   type 2             : clear by mask;   type 10: debug view (4C8A0)
5FCD0 then: --object+0x13C, object+0x10 = 1
```

`5FCD0` only draws when the receiver flag is set: `+0x78` (bit 2) for a **material** paint (descriptor `+0x78` source == 0), `+0x79` (bit 3) for a
**source** paint (descriptor `+0x78` != 0, the height variant). Both gates test the request's own object, not the world: a request is painted onto
every receiver that the world query reported for it.

### 1.4 The material read (STATIC)

The `_Bloody` family reads t41 / t42 through the patch: `GXBloodyMaterialPatch` vt `+0x10` (`5E8A0`) is `mov rdx,[rdx+8]; mov r8,[r8]; mov rcx,[rcx+8]; mov rdx,[rdx]; jmp 5EDA0`,
i.e. `5EDA0(receiver, ctx, item)` with `slot = receiver+0x20 + 8*[item+0xD4]` (`item` = the draw descriptor `D079C0` builds, `+0xD4` INFERRED 0):

```
rt = slot or return;                       // slot 1..3 are null for entities: such items bind nothing
t41 = rt+0x50 ? (rt+0x08 ? (rt+0x08)+0x48 : default0) : default0            // diffuse SRV
t42 = rt+0x50 ? (rt+0x10 ? (rt+0x10)+0x48 : default0) : default0            // specular SRV
t43 = [[GX+0x270]+0x30] default;  PSSetShaderResources(41, 3, {t41,t42,t43})
t53 = rt+0x28 ? (rt+0x28)+0x48 : default;  VS and PS SetShaderResources(53, 1, t53)    // height
```
`default0` = `[[144961AB0]+0x270]+8` vt+0x10, the image natively called `absent_decal`; the common provider `6FFA0` (model flag bit 2 without a provider, option `14473C7E5`)
binds the same default to t41 and t42. **No compiled programme reads t43 or t53** (reflection of all 11,641 programmes of the cache: `g_Decal*` appear only at t41 and t42), so only t41 / t42 matter.

## 2. The paint pass exactly

### 2.1 Programmes (STATIC, names from the I-0063 cache)

| Role | Vertex | Pixel | Notes |
|---|---|---|---|
| depth from the projector | `<family>_Dpt[_Skin]` (existing native recipes 20 / 21) | `<family>_Dpt` / `_DptA` | shader pass 2 (the engine pass id the code sets: `ctx+0x1C = 2`) |
| **colour paint, box** (the rung-1 draw) | `<family>_Pnt` and `<family>_Pnt_Skin` | **`GXFlver_Common_Pnt`**, alpha variant `GXFlver_Common_Pnt_[A]` | shader pass 8; 83 families have a `_Pnt` VS (47 in `gxflvershader`: the `_Bloody` families, `Equip`; plus `SAT_*`, `Dragon_FDP`, `Fur_FDP`, `C_ARSN*`, `MultiBlend4_et1_p`, `RecursiveCrystal`); the pixel programme is shared like `GXFlver_Common_Velo` |
| colour paint, cube | the same `_Pnt` | `GXFlver_Common_PntCube` / `_[A]` (texture cubes at t0, t1, t32) | pass 9, descriptor projection != 0 |
| erase | `<family>_PntCol[_Skin]` (828 B, COLOR1 -> COLOR) | `GXFlver_Common_PntCol` | pass 10, only materials with flag bit 7 (MTD sheet) |
| `bloodmask` blit | `14007F820` fullscreen draw | (5F620) | type 1 without the erase flag |
| `Decal_Sub` | gxdecal bundle | 1004-byte pixel programme: `o0 = o1 = (0,0,0, t0.r * g_scaleFactor)` | not found in the paint path read here; not needed |

`<family>_Pnt` VS (5040 B for `ColDifSpcBumpIbl_Bloody`), disassembled:

```
uv2  = TEXCOORD0.zw (int, shorts)            ndc = ( uv2.x/1024 - 1, 1 - uv2.y/1024 )      // second UV set, 2048 = 1.0
m    = NORMAL.w + cb4[5].x ; world = float4(POSITION,1) * VC_aObjMatrix[m]    (cbObjMatrix b5, 3 rows each)
c    = world * VC_MatrixWVP  (dp4 with cb0[0..3])                              // projector clip
SV_Position = ( ndc.x*c.w, ndc.y*c.w, c.z, c.w ) ;   TEXCOORD0 = c ;           // the mesh is drawn flat in its UV layout, depth = projector depth
SV_ClipDistance0 = dot(SV_Position, cb8[4] = VC_ClipPlane) ;  ClipDistance1 = dot(SV_Position, cb4[4] = userClipPlane)
```
Resources: cbMatrixWVP b0 (64 B), cbInstanceData b4 (96 B), cbObjMatrix b5, cbSceneParam b8 (80 B); inputs POSITION, NORMAL, TANGENT x2, COLOR1, TEXCOORD0 (the `_Skin` variant adds the palette).

`GXFlver_Common_Pnt` PS (11.6 KB asm), two render targets:

```
ndc = v1.xyz / v1.w ;  uv = ndc.xy * (0.5,-0.5) + 0.5
dmax = max of t32.SampleLevel(s7 ClampPoint, uv, 0) at texel offsets (+1,+1) (-1,+1) (+1,-1) (-1,-1)  + 0.001
vis  = dmax < ndc.z ? 0 : 1                                           // the character shadows itself from the projector
a    = t4.Sample(s3, uv).r * t0.Sample(s0, uv).a * vis ;  if (a < 1/255) discard ;  a *= cb4[7].w            // cb4[7] = VC_InstanceData[0].mulCol
sp   = t1.Sample(s1, uv).rgb ;  bump = t3.Sample(s2, uv).b
wet  = saturate( (dot(sp, (0.29891,0.58661,0.11448)) * (cb2[36].y - cb2[36].x) + cb2[36].x) * 12 )          // FC_WetParam1
o0   = ( lerp(t0.rgb, cb2[35].rgb, wet*cb2[35].w) * cb3[0].rgb * cb4[7].rgb , a )                           // FC_WetDiffuseColor, FC_DifMapMulCol, mulCol
o1.y = sqrt( lerp(sp.r, cb2[37].x, wet*cb2[37].w) ) * g_PaintMaterialParamScale.x                           // cb10[10].x
o1.x = lerp( bump * cb3[2].x/256, cb2[38].x, wet*cb2[38].z ) * g_PaintMaterialParamScale.y                  // cb10[10].y ; FC_SpcParam.x
o1.z = 0 ; o1.w = a
```
`cbPaintParam` (b10, 13 vec4 = 208 bytes declared, Native2331 RDEF; the other fields are unused by this programme): only `g_PaintMaterialParamScale` (offset 160) is read.
`GXFlver_Common_PntCol` PS: `discard if FC_PaintFadeParam.x < v1.z; a = (1 - sat((v1.z - FC_PaintFadeParam.y) * FC_PaintFadeParam.z)) * FC_PaintEraseColor.a;
o0 = o1 = (FC_PaintEraseColor.rgb, a)` (cb10 offsets 176 / 192).

### 2.2 Constants (STATIC; `1AAA70`, `1AB370`, `1AA540`, `1AC640`, `5FCD0`)

Row-vector convention (as the executable stores `Z[4c+r]` = element r,c: each constant-buffer register `j` holds column `j` of the matrix, the
layout native already uses for the sun-shadow VS b0, `game_bootstrap.cpp` "shadow_views", `block+64`). Descriptor offsets `d+x` are from the 0x118-byte copy
(`RequestNativeDecalCpu` already keeps it in `NativeDecalCpuIntent::descriptor`); object offset = `d+0x18`.

| Name | Value |
|---|---|
| decal frame `W` (decal space -> world) | rows: `D` = `d+0x18` (unit direction), `P1 x D` with `P1` = `d+0x0C`, `-P1`, then `(P0, 1)` with `P0` = `d+0x00` (the code builds `cross(D, K-P1)` with `K` = 0 at `1439F2490`) |
| inverse | `V = W^-1`: transpose the 3x3, translation `-t.R` (`1AC640`, constant `(-1,-1,-1,1)` at `143D80010`) |
| `s`, `z`, `range` | `s = d+0x24` (square width), `z = d+0x2C` (depth), `range = d+0x30`; `d+0x28` is not used by the box |
| range > 1 (diverging, `1AA540`) | `a = (s*z*0.5)/(s*range - s*0.5) = z/(2*range-1)`, `B` = identity with row 3 `(0,0,a,1)`, `t = 2a/s`, `P = [[t,0,0,0],[0,t,0,0],[0,0,(z+a)/z,1],[0,0,-(z+a)*a/z,0]]` (near `a`, far `a+z`) |
| range <= 1 (orthographic) | `B` row 3 `(0,0,0.1,1)`, `P = [[2/s,0,0,0],[0,2/s,0,0],[0,0,1/z,0],[0,0,-0.1/z,1]]` (`143D7ED70` = 0.1) |
| `VC_MatrixWVP` (VS b0) | `F = V * B * P`; depth 0 at the hit surface, 1 at depth `z` into the body (decal axis Z = `-P1`) |
| projection != 0 (cube) | six faces, `FC_PaintCubeProjMatrix`, t32 a cube: out of rung 1 |
| `g_PaintMaterialParamScale` (PS b10 +160) | `(float at d+0xD4, float at d+0xE0)`; `FC_PaintMatrixView` (+0) is `W` times constant matrices (not transcribed), `FC_PaintParam` (+64) `(0, 0.5*d+0x2C, 0, 0)`, `FC_OccTexRcpSize` (+80) 1/size of the cube depth (zero otherwise): all unused by `Common_Pnt` |
| `cbInstanceData` (b4) | `mulCol` (+112) = `(d+0xC8 rgb * ctx+0x310.., ctx+0x31C)` (ctx scale INFERRED 1); the draw's own `cb4[5].x` matrix index and `cb4[4]` user plane stay the model's |
| `cbMatDynParam` (b2), `cbMtdParam` (b3) | the mesh's own material blocks (the shader reads `FC_Wet*`, `FC_DifMapMulCol`, `FC_SpcParam` from them; INFERRED, no paint-specific upload found) |
| `VC_ClipPlane` (b8 +64) | not resolved statically (decompiler reorders the upload); INFERRED `(0,0,1,0)` = keep clip z >= 0, else zero (no clip) |

Clears per paint: projector depth map to 1.0; the colour pass clears depth-stencil `+0x38` to 1.0 first (`1419201A0`, flags 3).

### 2.3 States (STATIC; bank offsets of `[[144961AB0]+0x2A0]`, documented in FFX-DRAW N6 / Reference1956)

| Pass | Targets | Viewport | Blend | Depth | Raster | Samplers |
|---|---|---|---|---|---|---|
| projector depth (pass 2) | none; D16 `0x200 x 0x200` map | the map | `+0x198` (off, write mask 0) | `+0x1E0` LESS_EQUAL, write | `+0x208` solid, cull none | the model's |
| colour paint (pass 8) | RT0 diffuse (SRGB view), RT1 specular; DSV D32S8 `w x h` | the target | **`+0x1A8`: SRC_ALPHA / INV_SRC_ALPHA, alpha ZERO / INV_SRC_ALPHA** (height variant `+0x1B0` DEST_COLOR / ZERO) | `+0x1E0` | `+0x208` | s0..s3 = bank row 10 (`+0x318`; s3 row 4 `+0x2E8` when the decal has no mask), s7 row 3 (point clamp, `14010AB80`), rest as the 10AB80 map |

The blend is exactly what the sampling side expects: the target stores (accumulated colour, remaining base fraction): `dst.a *= 1 - a`, so
the neutral clear (0,0,0,1) means "keep the base albedo" and repeated hits accumulate. Texture slots of the pass (its own provider, vt `143D5C5A0` +0x10 = `1AA470` -> `1AC390`, the analogue of the patch's `5E8A0`): t0 `[d+0x48]`, t1 `[d+0x50]`, t3 `[d+0x60]`
(default flat bump when `d+0xE8` is 0 or the pointer null), t4 `[d+0x70]` (default `[[GX+0x270]+0]`), t32 the projector depth map; each is a texture object whose vt+0x10 returns the view
(the same `SprjGxTexture` -> `GXTexture2D +0x10` forwarding the D1 collect already resolves). `d+0x48 == 0 && d+0x78 == 0` is rejected by `RequestNativeDecalCpu`.

## 3. The material side

- Programmes: 1,128 pixel programmes declare `g_DecalDiffuseTexture` at t41 (1,096 also `g_DecalSpecularTexture` at t42): the `_Bloody` G-buffer, forward and related programmes (MENUS-MAP lists the families, hair and eyes included).
- `GXFlver_ColDifSpcBumpIbl_Bloody_Gbuf.ppo` (DXBC, 86 slots), UV = `v4.zw` (the VS TEXCOORD output `.zw`, the second UV set), samplers s0 for t41, s1 for t42:
  ```
  dec  = t41.Sample(s0, uv2)                      // rgba: rgb added colour, a = fraction of the base kept
  dsp  = t42.Sample(s1, uv2)                      // r shininess, g sqrt(reflectance)
  spc  = lerp(spec, spec*dec.a + float4(dsp.g*dsp.g.xxx, dsp.r), FC_BloodAmount)           // FC_BloodAmount = cbMtdParam +96 (cb3[6].x)
  alb  = lerp(wetAlbedo, wetAlbedo*dec.a + dec.rgb, FC_BloodAmount)
  ```
- The only scaling constant is `FC_BloodAmount` (`g_BloodAmount` of the MTD; native already produces it, `native/world/producers.cpp:237`; the same name sets policy flag bit 2, `model_parameters.cpp:478`).
  With a materially zero amount no blood would show: check it for the NPC's `_Bloody` MTDs (section 6).
- A mesh is painted only when its material has policy flag bit 2 (`0x4`: `g_BloodAmount` undeclared, or `GXFT_PaintDecal_Mat`): "a material without bit 2 is skipped in shader passes 8 and 9" (MTD sheet, `1401407D0`).
- Not applicable to t41 / t42: no per-character level, no tint; the colour comes from the decal textures and `mulCol`.

## 4. Native anchors (this worktree, names first)

| What | Where |
|---|---|
| kind-1 requests, descriptor bytes, handle, `kind` | `RequestNativeDecalCpu` (`game_bootstrap.cpp` ~1780); `NativeDecalCpuIntent` ~181 (`descriptor` 0x118, `kind`); registry `decal_cpu_registries` (key = manager); the `observe` dump at ~1846 skips kind 1 (`!kind`) |
| the manager CPU object and its queue | `ConstructNativeDecalCpuRegistry` ~1663 keeps the original `GXDecalManager`; `51AA0`, `522B0`, `19B690`, `198910` are **not** run (consumers hooked or never scheduled), so the queue at `manager+0x20..+0x28` and `object+0x13C` are never drained natively (INFERRED, verify) |
| receivers and patches | the original `E000E0` / `51F80` / `5E990` / `D05C80` run natively; `selected.patch` = `[entity+0xA60]+0x50` is read in `NativeSelectedModel` (~4485, `patch`, `decal_choice`, `neutral_instance_decal`); receiver = `Read(patch+8)` |
| **where t41 / t42 are bound today** | `LegacyNativeMeshTextures` (~6765, block ~6825-6832) and `PlannedNativeMeshTextures` (~6862, ~6924-6930): `bloody_provider` (patch vt == `image+0x3D41DF0`) / `Common` / `None` / `neutral_instance_decal` -> `admit_scene(41/42, result.common_images->absent_decal)`; `absent_decal` is built at ~1464 |
| dynamic texture published to the selection | `PublishNativeSceneCopy` (~1043: record from a render-thread resource, `render_target=true`, own CPU descriptor, lock-protected shared_ptr, slot 45) is the template for a paint record |
| programme recipes | `native_recipe_count = 22`, `NativeRecipe()` (~643-648); `ds3content::SelectModelPrograms` `native/content/model_programs.cpp:41` (accepts passes 0, 1, 2, 6, 7) and `:56` (pass names, `Dpt`/`DptA`); the recipe loop ~6249-6290 and `velocity_common_pixel` (~6267) for a shared loose pixel programme; `ComposeNativeShaderGroup` ~5967 collects loose programmes by the prefixes `_Gbuf/_Velo/_Dpt/_Fwd` |
| collect hook | `SnapshotNativeDecals` (~9621, called from the scene collect) |
| draw template | `BuildNativeDecalDraws` (~10201): `NativeModelDrawInput`, `PrepareNativeModelDraw`, sampler bank rows, blend/depth/raster, `ModelPassEnvelope` (`native/renderer/model_draw.h:66`, `EnvelopeSpec`) |
| depth-only draws from another viewpoint | the sun-shadow casters: `NativeModelFrameInput::shadow_draws/shadow_views` (`model_frame.h`), `DepthOnly` envelope, VS b0 block 128 B with the view-projection at `+64`; encoded in `scene_pass.cpp` ("shadow_casters", between the G-buffer draws and `EncodeNativeDecals`) |
| encode order | `SceneRenderer` main view: G-buffer draws -> shadow casters -> `EncodeNativeDecals` -> lighting (`scene_pass.cpp` ~2290-2311); the decal stage declares its frame-graph pass and copies (~1740-1785) |
| `FC_BloodAmount`, flag bit | `native/world/producers.cpp:237`, `native/content/model_parameters.cpp:478` |

## 5. Ready-to-code steps (D3D12), smallest first rung first

**Rung 0, observation (no render change, one native run).** In `RequestNativeDecalCpu` drop `!kind` from the `observe` dump and log, for kind 1, the descriptor
(`d+0x00..0x30` floats, `d+0x38` kind, `d+0x3C` projection, the texture pointers `d+0x48..0x78` with `textures.Acquire` residency and sizes, `d+0xC8..0xE8`), and once per struck entity
`[entity+0xA60]`, `[entity+0xA68]`, the holder's `+0x10` target, `short entity+0xCE`, the target's `+0x50` and `+0x60..+0x78`, and the length of `manager+0x20..+0x28`.
This settles the shape of the descriptors (punch vs Fire Orb), whether the queue fills natively, and the paint size, without any Reference run.

**Rung 1, plumbing with a proved no-op.** (a) `NativePaintTargets`: per receiver (key `Read(patch+8)`), on first sight of a `bloody_provider` mesh with a non-null slot 0,
create `RGBA8_TYPELESS` `size x size` x2 (diffuse SRV/RTV as `_SRGB`, specular `UNORM`), clear (0,0,0,1) on the render thread, publish `NativeTextureRecord`s
(`resident`, `render_target`, own CPU descriptors, `description`) under a lock, like `PublishNativeSceneCopy`. (b) In `LegacyNativeMeshTextures` / `PlannedNativeMeshTextures` admit t41 / t42 from that
record instead of `absent_decal` when it exists. (c) Frame graph: declare the two textures read in the G-buffer pass. **Gate:** the pictures at the Fire Keeper are unchanged (a cleared target equals `absent_decal`); retire records when the receiver vtable no longer reads `image+0x3D41DD8`; cap the pool (a 1024 target pair is 8 MB).

**Rung 2, the first visible blood (one struck NPC).**
1. Drain: at the collect (`SnapshotNativePaint`, next to `SnapshotNativeDecals`) read `manager+0x20..+0x28` (0x30-byte entries), keep at most `manager+0x408` per frame, for each type-0 entry with `handle != 0` take
   the kind-1 intent from `decal_cpu_registries[manager].requests[handle]`; then do what `5FCD0` does on the CPU: `--object+0x13C`, `object+0x10 = 1`, erase the processed entries (the vector is the original's, use `memmove` and the end pointer as `FUN_140052A80` does). Skip entries whose receiver flag `+0x78` (material) is clear.
2. Matrices from section 2.2 (`W`, `V`, `B`, `P`, `F`), written as the native sun-shadow does (register `j` = column `j`).
3. Draws: for the receiver's entity (the draw object `receiver+0x40`; natively the entity's prepared meshes whose material has policy bit 2): **depth pass** with the existing Dpt / DptA recipes into one shared D16 `512 x 512` map cleared to 1.0, VS b0 = 128-byte block with `F` at `+64`, viewport the map, blend off, depth LESS_EQUAL write, cull none;
   **colour pass** with a new recipe (shader pass 8, pixel variants 0 / 1): extend `SelectModelPrograms` (pass 8: VS `<family>_Pnt[_Skin]`, PS shared loose `GXFlver_Common_Pnt` / `_[A]`, like `velocity_common_pixel`), `native_recipe_count` 22 -> 24, `ComposeNativeShaderGroup` prefix `_Pnt`
   (the `PntCol` / `PntCube` names would also match the prefix: filter them out), a new `ModelPassEnvelope` (two colour targets, `DXGI_FORMAT_R8G8B8A8_UNORM_SRGB` + `R8G8B8A8_UNORM`, depth D32S8 `size x size` shared and cleared to 1.0 per paint), blend `+0x1A8` as above, depth LESS_EQUAL, cull none, samplers per 2.3, SRVs t0 t1 t3 t4 from the kind-1 textures (`d+0x48/0x50/0x60/0x70`, defaults as in `1AC390`) and t32 the map, VS b0 = `F`, b4 / b5 / b8 as the mesh's other passes (b8 `VC_ClipPlane` per the table), PS b10 with `g_PaintMaterialParamScale` at `+160`.
4. Encode: a new `EncodeNativePaint` in `scene_pass.cpp` just before `EncodeNativeDecals` (the original runs these commands at order 7, before the decal order 8 and after the G-buffer; INFERRED same list, so the paint shows **one frame later** on the character, as originally). Transition: target RTV <-> PIXEL_SHADER_RESOURCE; frame-graph `Pass("paint", Material)`.
5. **Degraded first picture (allowed for rung 2a):** skip the depth pass and bind a map cleared to 1.0 (no self-occlusion, blood also on the far side of the body); add the depth pass for parity in 2b.

**Rung 3.** Equip / player parts (`item+0xE8/+0xE9`, `140C3CA10` holders per equipment slot, size from the equipment); the `bloodmask` blit (type 1, `5F620`) and `PntCol` erase (type 1 with flag, `1AAF90`: blend `+8` off, `FC_PaintEraseColor = (0,0,0,1-f2)`, `FC_PaintFadeParam = (f0+f1, f0, 1/f1)`; vertex COLOR1 comes from the `GXPaintVertexBuffer` stream
bound by `5F030`); the height variant (descriptor `+0x78`, target `+0x30`, blend `+0x1B0`); the cube variant (`PntCube`, projection != 0); burns (Fire Orb) as the same pass with its textures.

## 6. What only a live capture can settle

1. **States and constants of the original draw** (a Reference run with the existing oracle pattern of D1, hooking the `Pnt` and `Dpt` draws of `1AAA70`): blend / depth / raster bank offsets (section 2.3 is read from the bank table, not from a paint draw), VS b0 / b4 / b5 / b8 and PS b2 / b3 / b4 / b10 bytes of the first `Pnt` draw, compared with `F` computed from the same descriptor (bit-identity as for `ComputeDecalConstants`); `VC_ClipPlane`; `mulCol`; whether b2 / b3 are the mesh's or the decal's.
2. The real kind-1 descriptors (punch vs Fire Orb): projection, range, sizes, which of `d+0x48..0x78` are set, texture sizes and formats, whether `d+0x78` (source) is non-null for burns; the real paint size (`short entity+0xCE`), the lazy flag, whether more than slot 0 is ever used and `[item+0xD4]`.
3. Which meshes the draw object paints in pass 8 / 2 (all meshes of the entity or only the intersecting ones; frustum culling by the main camera, which would skip off-screen characters in the original) and the exact vertex layout the `Pnt` VS needs (COLOR1 from the `GXPaintVertexBuffer` stream, TEXCOORD0 second UV).
4. That the world query really queues the receiver natively (length of `manager+0x20..+0x28` after a punch; `5F114`'s kind test) and what the `+0x408` budget is (read at run time).
5. `FC_BloodAmount` of the NPC's materials (is it > 0 for the Fire Keeper's `_Bloody` meshes?) and that the second UV set is intact in the native vertex buffers (the picture of the 2D atlas, `ndc = uv*2-1`).
6. Where `5F1D0` / `5F170` / the `+0x60` clear flag are raised in play (counts of types 1 and 2); `Decal_Sub` is not on this path as far as read.

## Evidence (all STATIC unless marked)

Exports and queries: `51F80`, `5E990`, `5EC10`, `5EDA0`, `5E8A0` (bytes), `4C3B0`, `4C440`, `4CFB0`, `4C900`, `4CB50`, `4CD20`, `4CD70`, `5F114`, `5F250`, `5F430`, `5F620`, `5FB70`, `5FCD0`,
`522B0`, `51860`, `52220`, `1AAA70`, `1AAF90`, `1AB370`, `1AC390`, `1AC640`, `1AA540`, `1AA470` (bytes), `14010AB80`, `140CEDD20`, `140CEDE20`, `140CEE040`, `140CF9610`, `140E000E0/0F0`, `140D05C80`, `1400600A0`, `140077640/880/AE0`, `1419294A0`,
`19BA40`, `140C3CA10`; vtables `3D1F6F8`, `3D41DD8/DA8/DB8/DF0`, `3D5B978`, `3D5C588/5A0/5B0`, `142945328`; strings `143D1F7C0/808/848`, `14283D3F0` (referenced only from an unanalysed function).
OBSERVED: `evidence/I-0066/artifacts/i66-reference-2109-keeper-decals` and `i66-reference-2113-keeper-punch` (`startup.jsonl`: `decal_observe_patch`, `decal_observe_paint_clear`).
Shader data: `evidence/I-0068/queries/fingerprints/programs.tsv`, `evidence/I-0063/artifacts/program-resources.tsv`; DXBC disassembly of `GXFlver_Common_Pnt`, `_PntCube`, `_PntCol`, `GXFlver_ColDifSpcBumpIbl_Bloody_Pnt`, `_PntCol`, `_Gbuf`, `Decal_Sub`.
Cross-checks: FFX-DRAW N6 (common state bank), MTD-FLAG-BITS-INVENTORY-SHEET (passes 8 / 9 / 10 table, bit 2), TONEMAP-SKIN (bank layout), MENUS-MAP "Decals" (descriptor `+0x98`), DECALS-NATIVE (descriptor offsets, sampler map).

## 7. Implementation log (thread « Sang sur persos, eau, siège », branch `claude/musing-bhabha-c546cc`, 2026-10-18)

- **Rung 0** (`decal-observe.flag`): `ObserveNativePaintRequest` logs each completed kind-1 request (`decal_observe_paint_request`: the
  box floats, kind, projection, the texture objects d+0x48..+0x78 with vtable, residency and size, the 0x118 bytes, the manager's queue
  length, its budget +0x408, the first entries, and once per receiver its slots, gates +0x78/+0x79, mask index and the slot-0 target's
  +0x50 / +0x60..+0x7F). The Reference oracle of 1AAA70 (`paint_oracle` rows: the pass id, VS b0/b4/b5/b8, PS b2/b3/b4/b10, the 48 PS
  SRVs, samplers, states, the kind-1 object and the frame W) rides the D1 oracle wrappers. **Not yet witnessed**: four native runs
  (Native2138-2141, Alpha punching from the pilot, `-KeeperLook`, `-LockOn`, `-AimAt`) never hit the Fire Keeper, who stands somewhere
  else at every load (Thomas); the in-game check waits for Thomas striking her himself.
- **Static reads that settle §2.2** (exports of 5FCD0, 1AAA70, 1AB370, 1AA540, 1AC640, 1AC390, 522B0; constants read from the
  executable): W rows D, D x (0 - P1) (mask 143D1C0E0 clears w), 0 - P1, (P0, 1); V by 1AC640's order; B, P as §2.2 (minimum range
  [143D7EE10] = 1, K = 0.5, ortho constants 0.1 / 2 / -0.1); F = V B P with 1AB370's summation orders (row 2 its own), uploaded
  transposed; **VC_ClipPlane = [1439F2500] = (0,0,0,1)** (cbSceneParam +64, VS b8); cbInstanceData (VS and PS b4, the same 0xB90 buffer)
  userClipPlane 0, matricesData.x 0, mulCol = d+0xC8 x ctx+0x310; cbPaintParam (PS b10) +160 = (d+0xD4, d+0xE0); the `_Pnt` VS reads
  only POSITION, BLENDINDICES/WEIGHT, TEXCOORD0.zw and declares the same inputs as the family's `_Gbuf` VS (COLOR1 unused);
  GXFlver_Common_Pnt's cbMatDynParam / cbMtdParam fields sit at the same offsets as the `_Bloody_Gbuf` programme's.
- **Rung 1-2 code** (`no-paint.flag` opts out): `SnapshotNativePaint` (main collect) drains the managers' queues as 522B0 (budget
  +0x408, the taken entries erased), takes each type-0 command's kind-1 object by bucket and generation (5FCD0), checks the receiver gate,
  applies 5FCD0's side effects (+0x13C, +0x10), computes F (`ds3content::ComputePaintProjector`, `native/content/paint_constants.*`,
  /fp:strict), resolves t0..t4 (defaults SYSTEX_NORMAL / SYSTEX_WHITE, INFERRED for [[GX+0x270]+0x10] / +0) and makes the receiver's
  target at its first paint (`SceneRenderer::MakePaintTarget`: R8G8B8A8_UNORM_SRGB + R8G8B8A8_UNORM, the size from the slot-0 target
  +0x60 when a power of two, else 1024); `native_paint_epoch` re-admits the model records so t41/t42 (`NativePaintViewsOf`) take the
  target. The prepare derives, for each G-buffer row of a painted model whose pixel programme samples t41, a `Paint2` draw from the
  row's draw (`NativePaintDraw`: geometry, VS b5 palette, b8, PS b2/b3; b4 with userClipPlane 0 and the paint's mulCol; VS b0 frame; PS
  b10; t32 white = rung 2a, no self-occlusion; samplers bank rows 10/4/3; states of §2.3). `SceneRenderer::EncodeNativePaint` clears a
  target at its first job (0,0,0,1), clears the D32 depth, binds VS b0 = F and VS b8 = a block with VC_ClipPlane (0,0,0,1), and paints
  before the G-buffer (the original paints after it: one frame earlier natively). Rows `native_paint_target`, `native_paint_drain`,
  `native_paint_frame`, `native_paint_event` (decal-observe.flag). Builds; unit tests pass; not run live with a hit.
- Open for the live check: the queue really filling natively (rung 0 rows), the target size word, the decal texture residency, the
  paint draws' validation, the picture against the original (`paint_oracle` for F bit-identity).

## 8. Live log and rung 2b (D-0193, thread « Textures et sang D2 », 2026-10-19)

Runs on a copy of Thomas's save with Alpha placed before the Fire Keeper (`Run-AlphaCast.ps1 -SaveCopy`, bare fists, two punches), as
the random spot of the Fire Keeper allows (a miss is relaunched).
- **The receiver gate** (8859b213, 160ef335): 51F80 copies the gates +0x78/+0x79 from the model's +0x224, never written natively; the
  drain takes them from the native union of the model's material policy flags, found through the patch pointer at receiver +0x50.
  Native2328: 6 requests, 6 accepted.
- **PS b10 is 208 bytes** (Native2331: the RDEF of GXFlver_Common_Pnt declares 13 vec4; +176 / +192 unread here, zero).
- **The `_Pnt` VS reads more of the G-buffer VS's registers** (Native2334-2335): b2 cbMatDynParam (`VC_isEnableCloth`) and b7
  cbvModelParam (`VC_World`) in the `_Bloody_Cloth_Pnt_Skin` family (the cloth path takes `VC_World` instead of the palette). They are
  the G-buffer row's own bindings, checked by name against the G-buffer programme (b2 falls back to the row's PS b2).
- **The mask t4 is d+0x70** (1AC390, §2.3): the code read d+0x48 + 8 x 4 = d+0x68, zero in every request, so every paint covered its
  projector square wherever t0 (`dp000100000_a`, BC1 sRGB, 256 x 256) has alpha: whole garments dark red (Native2339-2365).
- **Rung 2b, the projector depth map** (Reference2362 `paint_oracle`: pass 2 then pass 8, 40 and 25 of the first 64 rows): each paint
  target owns a D16 512 x 512 map (R16 typeless, SRV R16_UNORM, t32 of the paint draws); each painted G-buffer row also makes
  `NativePaintDepthDraw` (its Dpt / DptA recipe 20 / 21, every declared register the row's own by name, VS b4 userClipPlane 0, DepthOnly,
  cull none) and `EncodeNativePaint` draws them before the paint draws with VS b0 = the identity at +0 and F at +64 (the original's
  block). Without a depth draw (a refused row) the row still paints, unoccluded (`native_paint_depth_refused`).
- **Result** (Native2369, Native2375 against Reference2362): Alpha keeps thin streaks on his back and the Fire Keeper is unchanged at a
  distance, as in the original; 80 paint draws and 80 depth draws per paint frame, no refusal. The original also paints the attacker.
- **Not covered**: an ordinary enemy (red blood) was not struck in a live run; the requests of types 1 and 2 (the fading erase PntCol,
  the bloodmask blit, the masked clear, §1) stay undrawn natively, so blood never fades or washes; the cube projection (projection != 0).
