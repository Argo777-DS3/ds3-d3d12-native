# Sun shadows C4: the screen-space mask layer, static preparation

Thread « Statique régression d'image » (2026-10-16), for the pilot's step C4 of `SUN-SHADOWS-WAVE1.md`. Read-only: no game, no save, no
`main`. Labels: **STATIC** (decompile, listing or DXBC read), **INFERRED**, **OPEN**. Programmes were disassembled with `fxc /dumpbin` from the
I-0063 programme cache (`C:/Users/User/.codex/I0063-cache/programs`, `gxrenderershader__GXScreenSpaceCascadedShadow_Soft.ppo` and others); only
names, registers and constants are quoted here, no bytes. New exports in `decompiled/` (names `DS3-1152-F-<RVA>`): `001D76B0`, `001D6550`, `001D7C80`, `001D60C0`, `001D7660`, `0019E090`, `000800C0`,
`0007F820`, `000C5B00`, `00005EA0`, `0192F760`; already there: `001B8F30`, `001B8EA0`, `001D6D20`, `001D7850`, `001D7F70`, `001D8050`.

## 0. Answer first

| # | Question | Answer |
|---|---|---|
| 1 | Order, targets, parity, states | Per frame: phase 2 (`pass+0x710 = 3`), phase 0 (draw), phase 3 (`3 -> 1`). Draw phase, cascaded mode: **OutputColor on the pixels the G-buffer did not write, then the filter programme on the pixels it wrote, then `n` blur passes, each starting with the same OutputColor draw**. Stencil tests read bit **0x08**, ref 0xFF. The first target is **B when `blur radius x factor > 0` and the pass count is odd, else A**; the result is **always A** (layer `+8`). |
| 2 | OutputColor | `cbColor.g_fillColor` (`cb2[0]`) = **(0, 0, 1, 1)**: the « no shadow » value (w = 1, packed depth = (0, 0, 1)). Drawn with stencil NOT_EQUAL, read mask 0x08, ref 0xFF, no depth. Drawn before the cascaded draw and before each blur pass. |
| 3 | cbPerScene, cbBlurParam | cbPerScene is 0x150 bytes; the formulas are re-read in §3 and match the native `PerScene` producer. cbBlurParam is 0xD0 bytes: `{g_vDepthComputeParam = (P[2][2], P[3][2] / far, 0, 0), 12 taps {x / W * s, y / H * s, gauss, 0}}`, the 12 taps listed in §3.2. |
| 4 | Soft inputs | t1 depth, t32 atlas (R16_UNORM SRV on the R16_TYPELESS D16 resource, from SHADOWS-NATIVE §1.1), **t38 = the holder's texture `(root+0x4DD8)+0x78`**, a 32 x 32 texture (the producer constant is 1/32): **not** the 64 x 64 model dither the native already makes; content OPEN. Samplers: **s6 = bank 0 (SS_WrapPoint), s7 = bank 3 (SS_ClampPoint), s12 = bank 15 (SS_Shadow, comparison)**. |
| 5 | Sun | `DDirLightShadowed1` selected when **`*(int*)(pass+0x710) == 3`** (else `DDirLight1`); `_Cloud` when the byte `root+0x48FC` is set. It reads only **`FC_ShadowColor` (`cbSceneParam +464`)** besides what `DDirLight1` reads, and **t32.x, t32.z, t32.w** (DDirLight1: `.x`, `.z`). |

## 1. The layer (1B8F30) and its phases

`FUN_1401B8F30(layer, pass, args)`; `args = {phase, record}`; `record = {mode (int), state ptr, ..., depth target, executor}`:

- **phase 2** (begin): `pass+0x710 = 3`; `FUN_1401B8EA0` resolves the layer's mask texture (`{A, B, 0, 0, ..., record+0x0C, +0x4C}` through the layer's `+0x20` object `vt+8`); the result goes to `pass+0x12A0` and to `(pass+0xC98) vt+0x18` (the forward `FwdSSdw` reader). **When `record == null` it returns the engine's default white** (`[[4961AB0]+0x270]`).
- **phase 3** (end): `pass+0x710 = 1` when it was 3.
- **phase 0** (draw):
  1. `T0 = (state+0x60 float * factor > 0 and state+0x5C is odd) ? B (layer+0x10) : A (layer+8)`; `factor = [[GXRS+0x5368]+0x128]` (default 1.0). `FUN_1400C5B00(ctx, RTV slot 0 of T0, DSV = the scene depth-stencil)` pushes it on the pass's render-target stack. The DSV comes from the record's depth object (`FUN_1400CD1E0(record+0x20, 0)`), and carries the **G-buffer stencil**.
  2. `mode == 0` (shadows off): `ClearRenderTargetView` of **A and B** to (1, 1, 1, 1) (`FUN_141920200` = vtable `+0x190`), no draw. This is the case when no cascaded shadow runs.
  3. `mode == 1` (single map): `vt+0x18` of the executor (`FUN_1401D6550`, the menu views; not read here).
  4. `mode == 2` (cascaded): `vt+0x10` of the executor = **`FUN_1401D6D20(executor, pass, cascade state, depth SRV provider, args)`**, `args = {+0 A, +8 B, +0x18 depth DSV, +0x20 filter mode (state+0x1C), +0x24 blur radius (state+0x60), +0x28 pass count (state+0x5C), +0x2C state+0x24+state+0x28}`.
  5. `FUN_1400C5C60(ctx)` pops the render-target stack.

**Parity (STATIC, traced).** `FUN_1401D7850` takes `X = A, Y = B` (swapped when the count is odd) and each blur iteration writes `dst`, reads `src` with
iteration 1 `dst = Y, src = X`, then they alternate. Hence for count `n` the last `dst` is **A** (odd: X = B, Y = A, the last `dst` is Y = A; even: X = A, the last `dst`
is X = A). The cascaded draw writes **X** (T0 above): that is why T0 depends on the parity. Without blur the draw writes A. **The layer's visible mask is A.**
The composite (`GXRend_CompositeShadowMask`, RGB only) then writes into the **same target A**, keeping w: the lighting's t32 is A; so the order in the
frame is shadow layer first, composite second (it overwrites xyz, which only the blur needed as packed depth).

## 2. The draws, in order (mode 2, `FUN_1401D6D20`)

Resources of the executor (`FUN_1401D60C0`, built once): `+0x10` cbuffer 0x150 bytes (**cbPerScene**), `+0x18` cbuffer 0x100 (**cbBlurParam**, 0xD0 used),
programme set from `FUN_1401D76B0`: **`+0x20` VS `GXScreenSpaceShadow`**, `+0x28` PS `GXScreenSpaceShadow_PCF4`, `+0x30` `_PCF9`, **`+0x38`
`GXScreenSpaceCascadedShadow_PCF4`, `+0x40` `_PCF9`, `+0x48` `_Soft`**, **`+0x50` `GXScreenSpaceShadow_Blur`**, `+0x58` the input layout (2 elements, `0x14473CA40`,
built from the VS: POSITION float4 + TEXCOORD0 float2, stride 0x18); five GX depth-stencil states `+0x60, +0x68, +0x70, +0x78, +0x80`.

The VS is a pass-through (`o0 = v0` position, `o1.xy = v1.xy` uv, `o2 = v0` the clip position): the quad is the holder's (`root+0x4DD8`, VB `+0xB58`) 4-vertex
**triangle strip** drawn by `FUN_14007F820` (`Draw(4, 0)`, topology 5, stride 0x18).

Sequence (STATIC; the wrapper names are the D3D11 context's vtable slots read in `141920xxx`: `+0x40 PSSetShaderResources`, `+0x48 PSSetShader`,
`+0x58 VSSetShader`, `+0x80 PSSetConstantBuffers`, `+0x88 IASetInputLayout`, `+0x118 OMSetBlendState`, `+0x120 OMSetDepthStencilState`, `+0x158 RSSetState`, `+0x180 UpdateSubresource`, `+0x190 ClearRenderTargetView`):

1. `[state+0x110] == 0`: nothing (the setup is skipped).
2. cbPerScene built (§3.1) and uploaded to `+0x10`; `PSSetConstantBuffers(b0 = cbPerScene)`; `PSSetConstantBuffers(b8 = (root+0x4DD0)+0x2A8)` the engine's cbSceneParam.
3. **Push** the render target set (`+0x480..+0x498`), the depth-stencil state and ref (`+0x210/+0x218`), the rasterizer state (`+0x258`).
4. **`FUN_1400800C0` defaults**: rasterizer = bank `+0x208`, **blend = bank `+8` with sample mask 0xFFFFFFFF**, depth-stencil = bank `+0x1D8` (depth off, ALWAYS, stencil off) ref 0xFF (`bank = [[4961AB0]+0x2A0]`; blend and raster entries are the GX defaults, not read here: **OPEN**, the pilot's Reference capture of one draw gives them).
5. **`FUN_1401D7F70(layer, pass, ., 1, ., 1)`** (both flags 1 at this call; the same call in every blur pass): `OMSetDepthStencilState(layer+0x78 = S_E, 0xFF)`, then **`FUN_1401D8050` = the OutputColor draw**: a cbuffer `(root+0x4DD8)+0xB20` updated with `(0, 0, 1, 1)` (`.rdata 143D7FE40`), bound as PS **b2**, VS = layer `+0x20`, PS = the holder's `+0xBF0` programme (`gxposteffect OutputColor`, reflection: `cbColor { g_fillColor }` at cb2), quad draw; then **`OMSetDepthStencilState(layer+0x68 = S_B, 0xFF)`**.
6. Bind VS `+0x20`, the filter PS (`*(int*)(args+0x20)`: 1 -> `+0x40` PCF9, 2 -> `+0x48` Soft, else `+0x38` PCF4), input layout `+0x58`; geometry, hull, domain shaders null (`GS/HS/DS` setters `141920C00/141920A50/1419208A0`).
7. **t1** = the depth SRV (`param_4 vt+0x10`), **t32** = the cascade atlas SRV (`cascade state +0xBF0`, helper `FUN_140006100(ctx, 32, 1, ptr)`), **t38** = the holder's `+0x78` (`FUN_141920DA0` slot 0x26).
8. **Draw** (`FUN_14007F820(holder, ctx+0x460, 0)`).
9. Unbind t32 and t1 (slots 0x20 and 1 set to null); t38 stays.
10. `FUN_1401D7850(layer, pass, args)` (blur, §3.2), only if `factor * blur radius > 0` and the pass count is non-zero.
11. **Pop** the three stacks (raster, depth-stencil + ref, render target set).

**States S_B (`+0x68`) and S_E (`+0x78`) (STATIC, from `FUN_1401D60C0`'s descriptors).** The GX depth-stencil descriptor, layout read in `FUN_14192F760`
(D3D11 `GetDesc` into GX): `+0` DepthEnable, `+4` write mask, `+8` depth func (GX n = D3D n+1), `+0xC` StencilEnable, `+0xD` read mask, `+0xE` write mask,
front `{+0x10 fail, +0x14 depth fail, +0x18 pass, +0x1C func}`, back `{+0x20, +0x24, +0x28, +0x2C}`, `+0x30` a byte flag. All five start from bank `+0x1D8` (depth off) and
set `+8 = 3`, **StencilEnable = 1, ReadMask = 0x08, WriteMask = 0x08**, all ops 0 (KEEP), front and back func **2 (EQUAL)**:

| member | StencilEnable | func (GX -> D3D) | `+0x30` | role |
|---|---|---|---|---|
| `+0x60` | 1 | EQUAL | 1 | other modes |
| **`+0x68` S_B** | 1 | **EQUAL** | 0 | filter and blur draws (pixels with stencil bit 0x08) |
| `+0x70` | **0** | - | 1 | other modes |
| `+0x80` | 1 | **NOT_EQUAL** (5 -> 6) | 1 | other modes |
| **`+0x78` S_E** | 1 | **NOT_EQUAL** | 0 | OutputColor draws (pixels without bit 0x08) |

With ref 0xFF and read mask 0x08 the test is `(0xFF & 8) op (stencil & 8)`. **Bit 0x08 of the G-buffer stencil is set by every G-buffer draw** (the native writes `stencil = 9 [| 4 | 2]` for all
model draws, `game_bootstrap.cpp:8926`), so S_B covers all geometry and S_E the sky and any untouched pixel. The `+0x30` byte is unknown (a D3D11 field the GX struct carries
after the back face; it separates the pairs); the draws use the `+0x30 == 0` pair. D3D12: `DepthEnable = FALSE`, stencil read mask 0x08, write mask 0 (the ops keep), ref 0xFF,
DSV = the scene depth-stencil (stencil read), blend = the GX default `bank+8`, raster = `bank+0x208` (OPEN).

## 3. Constants

### 3.1 cbPerScene (`FUN_1401D6D20`, 0x150 bytes; DXBC reflection of `_Soft`)

`cbuffer cbPerScene { float4x4 g_InvProjMatrix @0; float4x4 g_ProjSpaceToShadowMatrix[4] @64; float4 g_CascadeSelectDist @320 }`, size 336 (the
reflection is the same for PCF4 and PCF9). Producers (the native `PerScene` in `light_producers.cpp` is qualified against the engine; this re-reads it):

- `@0` = **transpose(inverse(pass+0x50))** (`FUN_14004BCA0` = the engine's matrix inverse): `pass+0x50` is the **projection matrix**, 16 floats `+0x50..+0x8F` (`+0x78` = P[2][2], `+0x88` = P[3][2], used by the blur);
- `@64 + 64 k` = **transpose(Mul(pass+0xD0, cascade matrix k))**, `k = 0..3`, matrices `shadow state +0xAF0 + 0x40 k` (`FUN_14001CE40` = `EngineMultiply`); `pass+0xD0` is the camera world matrix (SHADOWS-WAVE1-READY §2.x);
- `@320` = `state +0xAA0` (`FC_CascadeSelectDist`, xyz used by the `lt` cascade selector).

The transposes are real: the stack copies in the decompile write element `[4i+j]` to `[4j+i]`.

### 3.2 cbBlurParam (`FUN_1401D7850`, 0xD0 bytes)

Run only if `[[GXRS+0x5368]+0x128] x args+0x24 > 0` (and the pass count `args+0x28` is non-zero). `s = factor x radius` with `radius = state+0x60` (the record's Blur Radius). Block:

- `+0x00`: `g_vDepthComputeParam = (pass+0x78 as float = P[2][2], pass+0x88 / far, 0, 0)`, with `far = *(view+0x58)` (the camera output record's far plane, `+0x50` fov, `+0x54` near);
- `+0x10 + 16 k`, `k = 0..11`: `(x_k / W * s, y_k / H * s, G_k, 0)`, `W, H = *(view+0x5C), *(view+0x60)` (as floats), `G_k = exp(-0.5 (x_k^2 + y_k^2)) / sqrt(2 pi)` (INFERRED from the constants `-0.5` at `143D7F0C0` and `2 pi = 0x40C90FDB` at `143D7EEFC` and the two CRT helpers `141FDC9A0` and `141FD1F60`).

Taps `(x_k, y_k)` from `.rdata`, in order: (0,-2) `143D7F4D0`, (-1,-1) `F4C0`, (0,-1) `F4A0`, (1,-1) `F4B0`, (-2,0) `F260`, (-1,0) `F240`, (1,0) `F1A0`, (2,0) `F200`, (-1,1) `F390`, (0,1) `F370`, (1,1) `F380`, (0,2) `F3A0`.
`GXScreenSpaceShadow_Blur` (`cb1`, `t1`, `s7`): it reads **t1 = the source mask** (named `g_DepthTexture` in the reflection, but it is the layer's own target): `center + 12 taps`; each sample's depth is decoded from its **xyz** as `dot(xyz, (1/65025, 1/255, 1))` (the raw NDC depth the cascaded pass packed), linearised as
`-(cb1[0].y) / (cb1[0].x - d)`, weight = `exp(-100 (dz)^2 / z0^2) x G_k`, and the result is `o0.xyz = centre.xyz`, **`o0.w = sum(w_k s_k) / sum(w_k)`** over `.w`. The blur therefore needs xyz intact: **the composite must run after it.**

### 3.3 cbSceneParam words (b8, the engine's) read

- `_Soft`: `FC_ScreenSize` (16), `FC_ShadowMapParam` (448: `x` far fade end, `y` 1/fade range), `FC_ShadowDepthRate` (560), `FC_ShadowDepthScale` (564), `FC_ShadowKernelScale` (568), `FC_ShadowRandTexInvScale` (572 = 1/32), `FC_ShadowTexInvScaleX/Y` (576/580), `FC_ShadowDepthOffsetScale` (584), `FC_ShadowViewKernelScale` (592, dotted with the cascade index column).
- `DDirLightShadowed1`: **only `FC_ShadowColor` (464)** beyond `DDirLight1`.
- PCF4/PCF9 differ only in the tap count (`_Soft` has 4 point taps for the blocker search, then 12 comparison taps, 16 samples in all, result `o0.w = 1 - sum(1 - cmp) x fade / 16`).

## 4. The Soft programme (`GXScreenSpaceCascadedShadow_Soft`, ps_5_0, 165 slots)

Registers: **s6 `SS_WrapPoint`, s7 `SS_ClampPoint`, s12 `SS_Shadow` (comparison)**, t1 depth, t32 atlas, t38 `g_DitherTexture`, cb0 `cbPerScene`, cb8 `cbSceneParam`. Flow:

1. **Dither** `t38` sampled at `uv x FC_ScreenSize.xy x FC_ShadowRandTexInvScale` with s6 (wrap, point), then `2x - 1`: a random vector in [-1, 1]^2 per screen texel. Content OPEN (INFERRED only: a 2-channel random table, the shader reads `.xy`; the `RandomRatation` string at `143D43818` is a reflected property of the holder `+0x730`, not the texture generator).
2. Depth `d` = `t1.x` (s7); position = `g_InvProjMatrix x (clip.xy, d, 1)`, view-space `z`; cascade index = number of `g_CascadeSelectDist.xyz < z` (0..3); shadow-space position = row `4 + 4 n .. 7 + 4 n` of `g_ProjSpaceToShadowMatrix`; distance fade `(FC_ShadowMapParam.x - |p|) x FC_ShadowMapParam.y` saturated.
3. A kernel scale from `FC_ShadowViewKernelScale` dotted with an identity-matrix column picked by the cascade index and `FC_ShadowTexInvScale`; four point taps (s7) at fixed offsets give the nearest blocker depth, then the 12 `sample_c_lz` taps (s12) at fixed offsets rotated by the dither vector.
4. **Output**: `o0.w = 1 - fade x (sum of (1 - cmp)) / 16`; **`o0.xyz` = the raw depth `d` packed** (`r2 = d x (65025, 255, 1)`, `o0.x = frac(r2.x)`, `o0.yz = r2.yz - frac(r2.xy) / 255`): a 3-channel 8-bit encoding (the blur's decode above).

**Samplers (STATIC).** The engine's common sampler table (native `common_banks[13] = {12,12,12,12,14,12,0,3,1,4,12,13,15}`, `game_bootstrap.cpp:9432`): `s6 -> bank 0`, `s7 -> bank 3`, `s12 -> bank 15`. The bank rows
(`sampler_bank.h`): row 0 `{0, 0,0,0, 0, 1, 7, border 0, minLOD 0, maxLOD max}` (point, wrap), row 3 `{0, 2,2,2, ...}` (point, clamp), **row 15 `{filter 15, 3,3,3, aniso 0, compare 1, border (0,0,0,0), maxLOD 0}`**: the comparison sampler, GX address 3 = border, comparison GX 1. The names and the registers agree with the reflection
(`SS_WrapPoint` s6, `SS_ClampPoint` s7, `SS_Shadow` s12). The native decode `scene->sampler_bank->descriptors[15]` already holds the D3D12 form (log it once next to the atlas format).

## 5. The sun programmes

`FUN_14019E090` builds the sun layer's programme table: **`+0x1A8` `GXLightAcc_DDirLight1`, `+0x1B0` `DDirLightShadowed1`, `+0x1B8` `DDirLightShadowed1_Cloud`, `+0x300` `DDirLight1MS`, `+0x308` `DDirLightShadowed1MS`**.
The choice is in `FUN_14019FB20(layer, pass, ., ms)` (`ms` = multisampled variant):

```
if (pass+0x710 == 3):  root+0x48FC ? Cloud (+0x1B8) : (ms ? +0x308 : +0x1B0)
else:                  ms ? +0x300 : +0x1A8
```
and, when `pass+0x710 == 3` and `pass+0xC98` is set, the cloud shadow textures (t22, t23) when `root+0x48FC != 0`. So **the shadowed sun is the sun after the mask layer's begin phase and until its end phase**; native can mirror it as "mask valid for this view".

DXBC (`DDirLightShadowed1` against `DDirLight1`): the additions are `cbSceneParam` (b8) and the read of **t32.w** (sampler s8, swizzle `.xzwy`: x = t32.x, y = t32.z, z = t32.w):
`tint = (1 - SC) + t32.w x SC` per channel, `SC = FC_ShadowColor.xyz`; the light's main diffuse factor becomes `t32.z x tint` (DDirLight1: `t32.z`); the second term of the light (`cbLight +5`) is not tinted. `t32.x` scales through `cbLight +17` (`1 + SsaoWeight x (t32.x - 1)`) in both. Resources otherwise equal (t0, t1, t3, t5, t32, b0, b5; s7, s8).

`GXRend_CompositeShadowMask` (reflection): `o0.x = t33.x` (the AO), `o0.yz = FC_Mult.yz x t3.z + FC_Add.yz` (t3 = GBuffer1, `.z`), `o0.w = 1`; b5 `{FC_Mult, FC_Add}`, s7, s8. With the pilot's colour write mask RGB the layer's w survives.

## 6. What to build and what to capture

- Native order per frame: shadow layer (OutputColor sky, filter, `n` x [OutputColor, blur]) writing A/B with the parity rule; **then** the composite RGB into A; the lighting reads A as t32; the sun is `DDirLightShadowed1` while the layer ran. Blur draws read the *previous* iteration's target as t1.
- Stencil: bit 0x08 (read-only DSV with stencil); D3D12 ref 0xFF, read mask 0x08, EQUAL for the filter and blur, NOT_EQUAL for OutputColor; no depth; blend off and raster none/solid are the GX defaults (capture to confirm).
- **To capture once in a Reference run (the pilot has the infrastructure)**: (a) the holder's `+0x78` texture bytes (the shadow dither: size, format, content), (b) the D3D11 blend and rasterizer states of one filter draw and one blur draw, (c) the SRV format of t32 and the RTV format of the mask, (d) `t32` mean on the floor at m40 with and without the sun shadow, to size the expected effect.
- **OPEN**: the `+0x30` flag byte of the depth-stencil descriptor; the single-map slot `FUN_1401D6550` (menus); the cloud shadow (`root+0x48FC`); the multisampled programmes; the mask target's creation (`FUN_1401B8EA0`'s object, format INFERRED R8G8B8A8_UNORM, TESSSCENE-SSMASK §2).
