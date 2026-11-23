# I-0068 part AW: the content of `cbTessScene` (TS1) and the format of the screen-space shadow mask (SH5), closed statically

Request (coordinator, 2026-10-11): study in static the content of `cbTessScene` (live row TS1) and the format of the screen-space shadow mask (live row SH5), to close these two open points left by [TESS-SHADOW-FORWARD-NATIVE](TESS-SHADOW-FORWARD-NATIVE.md) and [SHADOWS-NATIVE](SHADOWS-NATIVE.md) §10, §11. Nothing was launched; native code was read, not changed.

Labels: **STATIC** (decompilation, DXBC of the cached programmes, read by the system `d3dcompiler_47.dll`), **INFERRED**, **OPEN**. Evidence (outside Git): `evidence/I-0068/queries/tess/` (`tess-scene-reads.tsv` from `Scan-TessScene.ps1`; `dxbc/*.asm` disassembly of the programmes quoted below; decompiled functions `q1`..`q9`, `s1`..`s7`).

## 0. Answer first

1. **`cbTessScene` is fully specified by what the 282 hull and domain programmes read**, and its convention is settled in two points that earlier notes left open: the view-projection is the **absolute-world** one (not the camera-relative `VC_MatrixViewProj` of `cbSceneParam`), and the eye is the absolute camera position (§1.2). Only `g_f4Eye.xyz`, `g_f4ViewVector.xyz`, `g_f4GUIParams2.x` and the four planes are read by the hulls; only the view-projection (and, in 24 programmes, `g_fGlobalTime`) by the domains.
2. **The engine path of the block is now known up to the producer**: a virtual setter, slot `+0x70` of `GXRenderExecContext` (`FUN_14013B100`, an undefined function that earlier searches missed because Ghidra never defined it), copies `0xB4` bytes from a source block into the context at `+0x1130`; `FUN_14013AA60` uploads them (§1.1). The function that builds the *source* block was not found (the call is virtual and the argument is a plain pointer): OPEN, and not needed by native, which builds the block from the camera.
3. **The screen-space shadow mask is an 8-bit four-channel target** (INFERRED `R8G8B8A8_UNORM` from the 1/255 and 1/255² packing constants), **channel `w` is the shadow term** and channels `x`, `y`, `z` carry the packed scene depth for the blur. The forward programmes `FwdSSdw` read `.w` of `t32` with sampler `s8` = `SS_WrapLinear` (§2).
4. **Corrections** (§3): `cbTessScene.g_f4x4ViewProjection` is not the camera-relative matrix; the mask is not a single-channel target; `FUN_14013B100` is the missing slot of the exec-context block setters.
5. **Remaining live reads**: the three values statically undeterminable (§4): `g_f4GUIParams2.x`, the plane normalisation and `g_fGlobalTime`'s clock, and which of the two mask targets is bound (SH5, reduced).

## 1. `cbTessScene`

### 1.1 The engine path (STATIC)

| Step | Function | What it does |
|---|---|---|
| setter | `FUN_14013B100` = slot `+0x70` of `GXRenderExecContext` (vtable `143D52328`; the sibling `+0x68` `FUN_14013B0C0(this, type, sceneParam, wvp)` copies the `cbSceneParam` and `cbMatrixWVP` images when `type == 8`) | copies `0xB4` bytes (`0x00..0xB3`) from a source block into `this+0x1130..0x11E3` and returns `this+0x1130` |
| clone | `FUN_140139F40` (slot `+0x78`) | `FUN_14013A930(dst+0x1130, src+0x1130)`: the same `0xB4`-byte copy when a context is duplicated |
| upload | `FUN_14013AA60` (slot `+0x08`) | `FUN_14013A930(this+0x1130, src+0x1130)`, then uploads the block to `[[144961B50]+0x4DD0]+0x2D0` (the buffer the hull and domain stages bind as b5, PNTriangles at `+0x2C8` as b6) |
| source producer | not found: the callers are virtual (`CALL [reg+0x70]`), the six GX candidates with a `+0x70` call (`14019E090`, `14011D2D0`, `1401DDF70`, `1401DF9F0`, `140205220`, `14022E930`) call the resource interface, not this slot | OPEN (§4) |

The bytes `0xB4..0xBF` of the 0xC0-byte block are never copied, hence never read (the shader declares the buffer as 176 + 4 bytes).

### 1.2 What the shaders read (STATIC, `tess-scene-reads.tsv`, 626 programmes)

All hull and domain programmes bind `cbTessScene` at **b5** (same register in every stage).

| Variable (offset) | Hulls (282: 172 `_PN`, 110 `_FT`) | Domains (192 with the block: 168 read only the matrix, 16 + 8 also read the time) |
|---|---|---|
| `g_f4x4ViewProjection` (0) | unused | **read** by every domain: `clip.x = dp4(P, cb5[0])`, `y = dp4(P, cb5[1])`, `z = dp4(P, cb5[2])`, `w = dp4(P, cb5[3])` for the evaluated patch position `P` (w = 1) |
| `g_f4Eye` (64) | **`.xyz`** (all 282) | unused |
| `g_f4ViewVector` (80) | **`.xyz`**, `_PN` only (172) | unused |
| `g_f4GUIParams2` (96) | **`.x`** only (all 282): multiplies the distance fade of the tessellation factors (`cb5[6].x`) | unused |
| `g_f4ViewFrustumPlanes[4]` (112) | `_PN` only (172): `cb5[7..10]` | unused |
| `g_fGlobalTime` (176) | unused | 24 domains (`FlowDisplacement`, `Eclipse`, `WaterBumpTriInteractionFloatObj`, `_D` variants): `x += time * k` (a scroll offset added to a displacement coordinate) |

Conventions fixed by the instructions (veil `Veill_NoLight_Fwd_PN`, `dxbc/`):

- **Positions are absolute world.** The tessellated vertex stage (`…_PN.vpo`) outputs `P = pos × VC_aObjMatrix` and nothing else; the ordinary vertex stage (`Veill_NoLight_Fwd.vpo`) then does `P − cb8[3].xyz` (camera position) and `VC_MatrixViewProj` (the camera-relative matrix, `cbSceneParam` +384). The domain applies `cb5[0..3]` directly to the world-space patch position, with no subtraction, so **`g_f4x4ViewProjection` is the absolute-world view-projection** (`view × proj` with the translation, stored as rows for column vectors like the other matrices of `cbSceneParam`).
- **Eye**: the hull takes `|(vertex pair midpoint) − cb5[4].xyz|` as the camera distance for the tessellation fade: the absolute camera position (`w` unused).
- **Frustum planes**: the hull tests `dp4((P, 1), plane) > −cb6[2].x` for every vertex against each of the four planes and culls the patch when all three vertices lie outside one plane: planes in **absolute world space, positive inside**. The margin `cb6[2].x` (`g_fDetailTessellationHeightParam.x`, a height in world units) is only meaningful if the planes are normalised (INFERRED); the four are the side planes (left, right, bottom, top; no near/far), order irrelevant to the test.
- **View vector**: the hull computes `n · cb5[5].xyz` for the normalised mean normal of each edge and keeps the patch when any `n · v > −cb6[1].x`. Front faces must satisfy the test, so `v` points **from the scene toward the camera** (the negated camera forward axis, unit; INFERRED from the sign test, normals outward).
- **GUI parameters 2**: `.x` scales the distance-fade factors (`fade × cb5[6].x`); `.y`, `.z`, `.w` are never read. Its value is a GUI/debug scale: OPEN, expected 1.0 (TS1).
- **Time**: the scroll animation clock of the three displaced families, in seconds (INFERRED); not read by the veil.

### 1.3 What native must write (INFERRED conventions, STATIC dependencies)

Block of 0xB4 bytes for each view that draws a `_PN`/`_FT` material, from the same camera record as `ProduceSceneCamera` (`native/world/producers.cpp`):

| Offset | Value | Source in `ProduceSceneCamera` terms |
|---|---|---|
| 0 | `transpose(view × proj)` as four rows, **no camera-relative rotation trick** | `XMMatrixMultiply(view, proj)` stored with `PutT` (the camera-relative `PutT(out, 384, rotation × proj)` is **not** this) |
| 64 | camera position `xyz` (`w` = 1, unread) | `k.position` |
| 80 | `−forward` normalised, `w` = 0 | `k.forward` negated |
| 96 | `(1, 0, 0, 0)` until TS1 says otherwise | constant |
| 112 | the four planes `row3 + row0`, `row3 − row0`, `row3 + row1`, `row3 − row1` of the matrix at 0 (column-vector form), each divided by `|xyz|` | derived from the matrix |
| 176 | the effect/animation clock in seconds | the frame-time source native already owns |

PN-triangle culling and fades then follow the hull; a wrong plane sign shows as veils culled on screen, a wrong view-vector sign as veils vanishing from the front.

## 2. The screen-space shadow mask

### 2.1 What the programmes write and read (STATIC, `dxbc/`)

| Programme | Output / input | Reading |
|---|---|---|
| `GXScreenSpaceCascadedShadow_PCF4` and `GXScreenSpaceShadow_PCF4` (the PCF9 and Soft siblings were not disassembled) | `dcl_output o0.xyzw` | `o0.w = 1 − fade × (1 − shadow)`: the **shadow term in `w`**; `o0.x = frc(v)` and `o0.yz = frc(…) − previous × 0.003922` (1/255): the standard multi-channel packing of one float in x, y, z (a scaled pixel-space value, base 255) |
| `GXScreenSpaceShadow_Blur` | reads `t1` (the mask), `s7` = `SS_ClampPoint`; decodes `dp3(sample.xyz, (0.000015, 0.003922, 1))` (= 1/255², 1/255, 1) | a depth-aware blur: `o0.xyz` = the centre sample's packed value passed through, `o0.w = r0.y / r0.x` (the weighted average of the shadow terms over the weight sum) |
| `FwdSSdw` forward programmes (e.g. `BloodBlend_FwdSSdw`) | `t32` `g_ShadowTexture` (`float4`, 2D), `sample_indexable(...) r0.w, uv, t32.xyzw, s8` | takes channel **`.w`**, with **`s8` = `SS_WrapLinear`**, then `lerp(1 − FC_ShadowColor, 1, w)` (`cb8[29]` = `FC_ShadowColor`) |

The packing step `1/255` is the quantum of an 8-bit UNORM channel and `0.000015` is `1/255²`: a float or 16-bit target would not need the packing. Hence **`R8G8B8A8_UNORM`** (INFERRED; `B8G8R8A8` would only swap the contents of `x` and `z`, which both programmes treat consistently, so the channel semantics above hold either way).

### 2.2 Which target

`FUN_1401B8F30` code 0 draws into `layer+0x10` when the blur is on (`[[GXRS+0x5368]+0x128] × record+0x24 > 0` and the filter flag bit 0), else into `layer+0x08`, both render-target objects created outside this layer (the layer destructor `FUN_1401B85F0` only releases them). Code 2 binds the mask through `FUN_1401B8EA0`, which asks a transient-target pool (`layer+0x20`, vtable slot `+8`) with the key pair (`layer+8`, `layer+0x10`) plus size and format fields of the command record. Which of the two the pool returns cannot be settled statically: the natural reading is the final one of the chain (the blurred target when the blur ran). With the gate closed, `FUN_1401B8B60` clears the target to white (1.0 in all channels) and `FUN_1401B8EA0` returns the global default texture `**([GXRS+0x270])` for a null record: unshadowed.

## 3. Corrections to earlier notes

| Note | Statement | Correction |
|---|---|---|
| SHADOWS-NATIVE §11, TESS-SHADOW-FORWARD-NATIVE §2 | `cbTessScene` camera-derived, view-projection of the camera (INFERRED) | the matrix is the absolute-world view-projection, not `VC_MatrixViewProj`; eye absolute; planes inward world planes; view vector toward the camera (§1.2) |
| SHADOWS-NATIVE §11 | "no direct writer of `+0x1130`" | there is a setter: slot `+0x70` `FUN_14013B100`, a function Ghidra never defined |
| SHADOWS-NATIVE §10, row SH5 | mask format OPEN, "R8-class, channel `w`" | four 8-bit channels; `w` shadow term; `x y z` packed depth; `FwdSSdw` samples `s8` linear wrap |

## 4. Live rows after this note

| ID | Read | Expected | Status |
|---|---|---|---|
| TS1a | `cbTessScene` on a `c5270` veil draw: matrix rows 0..3, `xyz` at 64 and 80, `x` at 96, planes at 112..175 | rows = `transpose(view × proj)` with translation; eye = camera position; `v` = −forward; `x` = 1.0; planes unit and positive for a point inside the view | the values to compare (§1.3); only the numbers remain unknown |
| TS1b | the same block on a `FlowDisplacement` / `Eclipse` draw: `g_fGlobalTime` against a second read one second later | +1.0 per second (or the clock of the frame timer) | clock OPEN |
| SH5 | the format of the texture bound at `t32` for a `FwdSSdw` row, and whether it equals the `layer+8` or `layer+0x10` object of the pipeline | `R8G8B8A8_UNORM`; the blurred target when the blur is on | reduced to the choice of target |

## Exports

`decompiled/DS3-1152-F-` + `0013B100` (new; an undefined function made decompilable in the session, the copy loop quoted in §1.1), `0013A930`, `0013B0C0`, `001B8F30`, `001B8EA0` (new exports), `00139F40`, `0013AA60` (existing); read for this note and not exported: `1401B8A60`, `1401B8B60`, `1401B85F0`, `140101B40`, `1400843C0`. Tools: `Scan-TessScene.ps1` (hull and domain reads of the whole programme cache); `I0068Grep.java` (instruction-text search, used for the `0x1130` and `+0x70` sites). Programmes disassembled: `Veill_NoLight_Fwd_{PN,FT}.{hpo,dpo,vpo}`, `Veill_NoLight_Fwd.vpo`, `GXFlver_ColDifSpcBumpFlowDisplacement_Fwd_PN_D.dpo`, `GXScreenSpace{CascadedShadow_PCF4,Shadow_PCF4,Shadow_Blur}.ppo`, `BloodBlend_FwdSSdw.ppo`.
