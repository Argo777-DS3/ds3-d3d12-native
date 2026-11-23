# I-0068 part H: effects phase 2 ready to code: E01 billboards, then E02 trails (one machinery)

Request (coordinator, 2026-10-10): the same format as [EFFECTS-PHASE1-NATIVE](EFFECTS-PHASE1-NATIVE.md).

- An ordered list, each entry marked CALL / TRANSCRIBE / NATIVE.
- Minimal layouts, measurable checks at m40 and on one witness zone, risks.
- It must cover:
  - the GPU ring and shader set skipped in phase 1;
  - the scene-entity attachment and passes 0x45/0x39;
  - native `GXFfxDrawPass` and `GXFfxDrawContext`;
  - the PSOs of E01/E02;
  - the three direct builders;
  - the frame copy.

Prerequisite: phase 1 runs. The scene control is published, the library update runs, and lights
appear (R1–R7). Labels: STATIC, INFERRED, OBSERVED. Evidence:
`evidence/I-0068/queries/ffx/phase2/`.

Same rule as phase 1:

- **CALL** = no GX object touched;
- **TRANSCRIBE** = keep the function, replace only its GX statements;
- **NATIVE** = a contract implemented natively.

## 1. The machinery in one paragraph (STATIC)

Per GX pass:

1. The scene graph reaches the SFX layer and calls the scene control's draw (slot 10,
   `FUN_14024E790`).
2. That calls the library draw (`GXFfxManager` `vt+0x20`) with a `GXFfxDrawPass`.
3. For each visible draw entity, the library calls pass slot 4 (`FUN_1402997E0`). Slot 4 records
   a sorted pass command whose callback is `FUN_140299E40`.
4. At execution, `FUN_140299E40` builds a `GXFfxDrawContext` (`FUN_14025FB80`) and calls the
   entity's renderer.
5. The renderer evaluates the FXR properties through the draw-context slots (CPU), writes its
   vertices into the 2 MB GPU ring, and draws through one of three builders:
   - `FUN_140385F70` tessellated, 1-control-point patches;
   - `FUN_140396F30` triangle list;
   - `FUN_14039E900` `Draw(4)`.
6. Before each draw, the builder calls slot 27 (`FUN_140264230`): programme choice, states,
   constant upload, textures.

## 2. Construction additions, in order

| # | What | Original | Mode | Native replacement |
|---|---|---|---|---|
| 1 | **GPU ring pool** `sc+0xC0` (2 MB, align 16) | `FUN_1421392C0(sc+0xC0, *[GX], 0x200000, 0x10)` → `FUN_14213B340` → `FUN_14213B6B0` | **NATIVE** | ring object (§5.1) |
| 2 | default ring `sc+0xC8 = pool vt+8` | `FUN_14213B500` | CALL on the native pool | — |
| 3 | per-context sub-allocators `sc+0xD0[i]`, `i < sc+0xD8` | `FUN_142139450(pool, dst, 0x40, heap)` → pool `vt+0x10` | NATIVE pool slot | one per native recording thread (1 is enough) |
| 4 | **shader set** `sc+0xE0`, `[sc+0x28]+0x168` | `FUN_14024C300` → `FUN_140291520(*[GX], heap, shaderContainer)` | **TRANSCRIBE** | keep the table construction (the 128-entry VS/PS lists, glow twins, VFX tables); replace each GX programme creation by a native programme record (bytecode from the shader container `SprjSfxImp+0x38`) |
| 5 | resource manager slot 1 (FFX GPU resources) | `FUN_14036FD70` → `FUN_14036F9E0` | **TRANSCRIBE** | native textures/buffers + SRVs (FFX-FAMILIES-PHASE2 §4 row 1); the CPU record stays the engine's |
| 6 | **SFX scene layer** `SprjSfxImp+0x60` | `E6C9D0`: `FUN_1400C2400` (`GXSG::GXSgLayerFlat`, 0x38), `FUN_140D648A0`, then `layer vt+0x40(sc+0x8)` (adds the entity) | CALL | the layer object is CPU |
| 7 | **attach to the view's scene** | `E6EA10`: `FUN_1400C1E30(view vt+0x10(), layer)`, then `FUN_14024F2C0` | **NATIVE** | the native renderer does not walk the GX scene graph. Record that the SFX layer belongs to the view, and call the scene control draw at the pass points of §3 |

- The five constant buffers of the constructor stay null even in phase 2. The commit uploads the
  context blocks through the ring (FFX-FAMILIES-PHASE2 §6.1), so no fixed GPU buffer is needed.

## 3. Pass points and the scene-control draw (STATIC → NATIVE)

**The native pass points:** the native frame calls the scene-control draw once per GX pass that
reaches the SFX layer:

- the effect passes `sc+0x30..+0x40` = **0x21, 0x29 (forward), 0x2D, 0x20, 0x31**;
- plus **0x39** and **0x45**;
- never 0x3A.

**`FUN_14024E790(sc, passCtx)`: TRANSCRIBE.** Keep the logic.

- **Skip** when any of these holds:
  - the GX pause flag (`[GX]+0x3E1` → a native frame flag);
  - `sc+0x5BE`;
  - `!sc+0x20`;
  - pass 0x3A.
- **Default mode** (`sc+0x590 == 0`):
  - pass **0x45** first submits once with draw-pass flag **0x10** (entities whose `+0x4C` has bit
    16);
  - every pass submits with flag **(pass == 0x39)**.
  - Each submission is `mgr vt+0x20(&GXFfxDrawPass{passCtx, sc, ids sc+0x30..+0x40, flag})`, the
    library draw (CALL).
- **Multi mode** (`sc+0x590`) walks `sc+0x598..+0x5A0` directly. Leave `sc+0x590 = 0`.

**Kind → GX pass** (slot 4, unchanged):

- 1 → 0x21;
- 5 → 0x2D;
- 6 → 0x29, or 0x31 (`DAT_14473DAD6` / `DAT_144962375`, or near-camera);
- others → 0x29 (0x31 under the same flags);
- 0x23 when the entity sorts as a special pass.

## 4. `GXFfxDrawPass` (vtable `143D6A1F8`) and the pass context: NATIVE contract

**Slots** (STATIC):

| Slot | Original | Native behaviour |
|---|---|---|
| 0 | dtor | — |
| 1 begin | `FUN_140299360`: `passCtx+0x420 = 0x101`, `+0x423 = 0`, `+0x128 = pass type`, `+0x12C = 0` | same fields of the native pass context |
| 2 end | `FUN_1402993A0`: reset; `FUN_140198680(+0x4B0, 0xFF, 0, 4)` (sort-list flush) when `+0x421 == 0` | flush the native per-pass effect list |
| 3 | returns 0 | same |
| **4 per entity** | **`FUN_1402997E0`**: masks (`entity+0x1F8 & ~sc+0x14C`, `passCtx+0x4B8 & +0x1FC`, `+0x4BC & +0x200`); environment fade (`[GXRS+0x5360]`, `[GXRS+0x5368]+0x188/+0x170/+0x158`); 0x39 culling (`FUN_140299500`, kind 5, factor); `FUN_14029AA10` visibility; near test → kind → pass id; sort key `FUN_1400BF330` (distance) or `FUN_1400BF3F0`; **records** `FUN_1400BF4C0(passCtx, 4, 0x18)` = {`FUN_140299E40`, entity, data} with data = the 0x58-byte block of §4.2 | **TRANSCRIBE**: keep every test; record into a native per-pass list `{key, entity, data}` instead of the GX command stream |
| 5 | `FUN_140299DD0`: `[passCtx+0x410] vt+0x60(bounds)` (frustum) | native frustum test |
| 6 | `FUN_140299DC0`: bit test on `pass+0x2C` | same |
| 7 | returns 0 | same |

**Pass-context fields read** (minimal native layout, offsets of `GXRenderPassContext`):

| Offset | Content |
|---|---|
| `+0x124` | GX pass id |
| `+0x128` | sub-pass |
| `+0x12C` | sort key |
| `+0x410` | frustum object (`vt+0x60`) |
| `+0x418` | camera (matrices, `+0x30..+0x3C`, `+0x54/+0x58`, `+0x70`) |
| `+0x420..+0x426` | flags |
| `+0x425` | LDR flag |
| `+0x450` | GX context (only passed to `FUN_14025FB80`; a native token) |
| `+0x458` | context selector (`FUN_141921AC0`) |
| `+0x4B0` | sort/alloc block (`FUN_14020E570(+0x38, 0x80, 4)`) |
| `+0x4B8/+0x4BC` | pass masks (world `{1, 1}`) |
| **`+0x4C0`** | **frame copy** (FFX-FAMILIES-PHASE2 §8) |
| `+0x4C8` | depth |
| `+0x4D0` | pipeline `+0x3958` |
| `+0x4D8` | pipeline `+0x1110` |
| `+0x4E0` | pipeline `+0x2F80` (optional) |
| `+0x4F0` | pipeline (`FUN_140089140` → a dword) |
| `+0x58C` | dword |

**Command data** (0x58, from `FUN_1402997E0`/`FUN_14024E790`):

| Offset | Content |
|---|---|
| `+0x00` | 0 |
| `+0x08` | pass id |
| `+0x0C` | draw-pass mask word |
| `+0x10` | sc |
| `+0x20..+0x48` | pass-context `+0x4C0, +0x4C8, +0x4D0, +0x4D8, +0x4B8, +0x4E0` |
| `+0x50` | `+0x58C` |
| `+0x54` | `FUN_140089140(+0x4F0)` |
| `+0x78` | near flag |

**Execution, `FUN_140299E40(passCtx, entity, data)`: TRANSCRIBE.** At the native pass, in key
order, for each recorded item:

1. Keep the skips:
   - pass 0x3A;
   - kind 0x2718 when `[GXRS+0x5368]+0x1E8 <= 0`;
   - kind 0x25F under `DAT_14496235E` or `+0x5B9`.
2. Choose the ring:
   - `FUN_141921AC0(+0x458)` gives the context;
   - single context → `sc+0xC8`;
   - else `sc+0xD0[ctx]`.
3. **CALL `FUN_14025FB80`** (draw-context constructor, CPU; `param_3` = native context token).
4. CALL `FUN_1402600A0` (view/projection/world matrices from the camera).
5. Fill:
   - `ctx+0xA70` = frame-copy SRV, `ctx+0x990` = the same (t4);
   - `ctx+0xA68` = depth SRV;
   - the owner block: the create-info `[[entity vt+0x28()]+0x10]+0x40`, its `+0x18..+0x30` and, when `+0x224` is set, its light override `+0x230..+0x260`;
   - the pass type `ctx+0x18` (0x20 → 0, 0x21 → 1, 0x29 → 3, 0x2D → 5, 0x31 → 6);
   - `ctx+0x1C` = (pass 0x39).
6. **CALL `entity vt+8(entity, ctx, data[0])`**: the library renderer, CPU until it reaches a builder.

## 5. Ring and draw context: NATIVE contracts

### 5.1 Ring (original object `FUN_14213B6B0`, vtable `143CCCB10`; buffer desc 0x200000, usage 2 dynamic, bind 3, CPU write, misc 0x800)

- `vt+8` = **map**: `FUN_14213B690(ring, &ptr, ctx, bytes)` returns a CPU pointer for `bytes`
  (vertices, then R16 indices).
- `vt+0x10` = **unmap**: `FUN_14213B6A0(ring, &buffer, &offset, ctx)` returns the GPU buffer and
  the byte offset of the block.
- **Native:** a per-frame upload-heap ring, fence-retired.
  - Map hands out a CPU pointer.
  - Unmap returns `{GPU VA base, offset}` for the VB/IB views.
  - 2 MB per frame in flight.
  - INFERRED: the 0x800 misc flag is a raw-view flag; the native ring needs no views.

### 5.2 `GXFfxDrawContext` (vtable `143D65688`, 29 slots)

- **Keep the original object and slots 0–26 and 28.** They are CPU writes into the context:
  - matrices: slots 1–3, 28;
  - textures: slots 4–10, 12;
  - appearance setups: slots 13–20;
  - near/far, fog, screen size: slots 21–25;
  - slot 26.
- **Replace slot 27 only (`FUN_140264230`): TRANSCRIBE.**
  - Keep the CPU part:
    - the kind switch and flags (soft `+0xA94`, glow `+0xA95`, lit) → **shader-set index**
      (GlowD = +75);
    - the fog block `FUN_14014E900` → `ctx+0x880`;
    - the default-texture rule (`+0xAB9`);
    - t4 = `+0x990` when trace.
  - Replace the GX part:
    - **PSO** from the key of §6;
    - **root CBVs** from the ring: VS b0 `ctx+0x140` 0x1C0, DS b0 `+0x300` 0x1C0 (tessellated),
      PS b0 `+0x4C0` 0x230, PS b1 `+0x6F0` 0x190 (lit), PS b6 `+0x880` 0xE0;
    - **SRV table** t0–t4 = `ctx+0x968..+0x988`;
    - **sampler** s9 (`[[GX]+0x2A0]+0x2C8 + 8i`, or `+0x2E8` when `+0x9BA`), s1 for the
      `…2Sampler`/distortion programmes.
  - Its original GX calls are, in order: input layout, topology cache, VS/HS/DS/PS, blend,
    depth, raster, `UpdateSubresource` ×5, CB binds, SRV binds, samplers.

## 6. PSOs for E01 and E02 (STATIC tables; FFX-FAMILIES-PHASE2 §2)

**PSO key:**

```
{ shader-set index (VS, PS; HS/DS = _quad|_octed|_quincunx for tessellated kinds),
  input layout (from the VS input signature; slot 0),
  blend   = bank entry by FXR blend type ctx+0x960: 0 +0x08 off, 1 +0x10 alpha, 2 +0x20 additive, 3 +0x28 reverse-subtract (INFERRED), 4 +0x18 multiply;
            glow pass (type 6): +0x1A0..+0x1B8 MRT variants,
  depth   = +0x1E0 (pass type 1) or +0x1E8 (others; INFERRED test on, write off),
  raster  = +0x208, or +0x218 when ctx+0x9B8,
  topology type = PATCH (tessellated) | TRIANGLE,
  RTVs = forward HDR (R16G16B16A16_FLOAT); + glow target as RT1 for …GlowD,
  DSV = scene depth }
```

**Root signature** (one for the family):

- CBV b0 (VS / DS), CBV b0 + b1 + b6 (PS);
- SRV table t0–t4;
- samplers s1, s9.

**E01** (shader-set indices, glow +75):

| Indices | Programmes |
|---|---|
| 0–2 | `GXFfxtexture` |
| 3–11 | bump, soft, softBump, multi, softMulti (+2Sampler), colorOnly, pointSprite |
| **12–35** | **`GXFfxtessellate{,Bump,Soft,SoftBump}Texture`** (+`Blend…` PS 24–35) with HS/DS |
| 41–56 | `tessellate{,Soft}MultiTexture` (+2Sampler, Blend…) |
| 64–69 | `_PNT1..3` lit |

- **OBSERVED at m40** (I-0063 i63-live-01, 1,800 frames), the eight E01 programmes drawn:

  | Programme | Frames drawn |
  |---|---|
  | `GXFfxtessellateBlendSoftTexture` | 1800 |
  | `GXFfxtessellateTexture` | 1556 |
  | `GXFfxtessellateSoftTexture` | 1553 |
  | `GXFfxtessellateSoftMultiTexture` | 1561 |
  | `…BlendSoftTextureGlowD` | 88 |
  | `…SoftTextureGlowD` | 79 |
  | `…SoftMultiTextureGlowD` | 77 |
  | `…SoftMultiTexture2Sampler` | 30 |

  - Build these eight PSOs first, with their `_quad/_octed/_quincunx` hull/domain pairs as the
    pattern requires.

### 6.1 OBSERVED pipeline states of the E01 patch-list draws (part AK)

Source: the retained `i63-live-10` frames (`frame-60`) of m31, m37, m45, m50 and m51: every draw
with topology `1_CONTROL_POINT_PATCHLIST`, grouped by state. TSV:
`evidence/I-0068/queries/ffx/step-audit/e01-patch-draw-states.tsv`. State descriptors decoded as
D3D11 values.

**Rasteriser:** the same in every draw: solid fill, **cull none**, front-counter-clockwise on,
depth clip on, no bias, no scissor, no multisample.

**Depth-stencil:**

| Descriptor | Use |
|---|---|
| test on, **write off**, `LESS_EQUAL` (the stencil off) | every blended draw (this replaces the INFERRED of §6) |
| test on, **write on**, `LESS_EQUAL` | the opaque draw (blend type 0), seen at m51 |

**Blend** (RT0; with IndependentBlendEnable 0, RT1 takes the same state):

| Observed state | Colour src / dest / op | Alpha src / dest / op | Seen with |
|---|---|---|---|
| additive, alpha ONE/ZERO | SRC_ALPHA / ONE / ADD | ONE / ZERO / ADD | two targets; the commonest (m31 15, m37 102+3+82, m45 24, m50 15, m51 15) |
| additive, alpha ZERO/ONE | SRC_ALPHA / ONE / ADD | ZERO / ONE / ADD | one target (m37 9, m45 10) |
| alpha | SRC_ALPHA / INV_SRC_ALPHA / ADD | ZERO / INV_SRC_ALPHA / ADD | one target (m37 9, m50 3) |
| alpha, alpha ONE/ZERO | SRC_ALPHA / INV_SRC_ALPHA / ADD | ONE / ZERO / ADD | two targets (m45 3+3) |
| opaque | off (ONE / ZERO) | ONE / ZERO | two targets, with depth write (m51 3) |

- Write mask 15 in every state.
- **Targets:** 187 of the 205 m37 draws bind two targets ([HDR, GB5]); the rest bind one. The
  number follows the programme (glow twin or not) and the pass, as in EMISSIVE-GLOW §8.1.
- **Vertex strides** per input layout:
  - 36 bytes (82 of 205 draws at m37);
  - 112 bytes (the commonest, two layouts);
  - 160 bytes (3 draws at m37).
- **Sampler s9** (OBSERVED, all 296 patch-list draws of the five zones, one state):
  - descriptor `[21, 1, 1, 1, 0, 0, 1, 0,0,0,0, 0, MAX]`: filter `MIN_MAG_MIP_LINEAR`, **address mode 1 (WRAP) on U, V and W**, LOD bias 0, min LOD 0, max LOD maximum;
  - this is the descriptor the original binds for `g_ClampLinearSampler` (the PS register name, `s9`). The name does not match the mode: **bind WRAP**, as observed;
  - slot 1 holds an anisotropic wrap sampler (filter 85, 16), left bound by an earlier pass; the E01 pixel shaders declare no `s1`;
  - the descriptor columns follow D3D11's order (INFERRED by comparison with sampler id 1 of the same capture, address mode 3 = clamp).
- The layout of the programme is not recoverable from the capture, but it is in the executable:
  see FFX-INPUT-LAYOUTS (part AM: 11 layouts, formats, offsets, strides per VS index).

**E02** (same machinery and same builders):

- indices **36–40**: `tessellateTracerTexture`, `tessellateTracerBumpTexture`,
  `tessellateLineColorOnly`, `tessellateQuadLineColorOnly`, `tessellatePointSprite`;
- the non-tessellated `softTracer`;
- action 10012 (trail) uses the tracer programmes (INFERRED).
- E02 was not drawn at m40 in I-0063 stream 1 (first live witness: i63-live-33).

## 7. The three builders: TRANSCRIBE (keep the vertex generation, replace the GX statements)

| Builder | GX statements (STATIC) | Native |
|---|---|---|
| **`FUN_140385F70`** (tessellated E01/E02) | ring map `FUN_14213B690(ring, &ptr, ctx, (n+1)·0xA0 [+ 2·indices])`; unmap `FUN_14213B6A0`; `FUN_141920420(…, 10)` topology (GX 10 → 1-control-point patch list); `FUN_140056F30(ctx, 0, 1, &buffer/stride/offset)` (VB); `Draw(n, 0)` (`FUN_141921370`), or `FUN_141920460(…, buffer, 0x39 = R16_UINT)` + `DrawIndexed` (`FUN_141921390`) | ring map/unmap native; `IASetPrimitiveTopology(1_CONTROL_POINT_PATCHLIST)`; VB view {ring VA + offset, size, stride}; `DrawInstanced(n, 1, 0, 0)` or IB view R16 + `DrawIndexedInstanced` |
| `FUN_140396F30` (triangle list) | topology GX 4; `Draw` | TRIANGLELIST + `DrawInstanced` |
| `FUN_14039E900` (`Draw(4)`) | `Draw(4)` through the interface `param_2` slots `+0x28..+0xD8` | `DrawInstanced(4, 1, 0, 0)`; topology from the PSO (INFERRED strip) |

- All three call draw-context slot 27 (`vt+0xD8`) before drawing.
- The tessellated builder also calls slots 9, 10, 11, 12, 17, 21, 22.
- **Vertex stride:**
  - the builder reserves 0xA0 bytes per control point in its largest case;
  - **OBSERVED patch-list strides: 112 bytes (most), 36, 160** (`i63-live-10` m37), i.e. one input
    layout per programme;
  - take the stride from the PSO's input layout, not from a constant.

## 8. Frame copy (FFX-FAMILIES-PHASE2 §8)

- Pipeline `+0x1108`, the HDR target's format and size, copied at the start of forward pass 0x29.
- It feeds `passCtx+0x4C0` (§4), hence `ctx+0xA70` and t0/t4 for the effects that ask for it
  (E04/E05, and every soft/distortion kind that passes null to slot 8).
- Implement it in phase 2 even if E04/E05 come later: the slot-8 fallback can be reached by E01
  kinds.

## 9. Checks

**At the m40 witness** (bonfire area; compare with I-0063 i63-live-01 and EFFECT-LIGHTS §5):

| Check | Expectation |
|---|---|
| P1 | the eight E01 programmes of §6 are drawn, with the frequency ranking of §6 over a camera turn |
| P2 | all E01 draws use topology `1_CONTROL_POINT_PATCHLIST` with a hull and a domain shader |
| P3 | blend types seen: off, alpha, additive (and DEST_COLOR multiply); GlowD draws write two render targets |
| P4 | no draw in pass 0x3A; 0x39 draws only for glow-capable entities (flag 1) |
| P5 | the ring never exceeds 2 MB per frame (high-water mark logged) |
| P6 | back-buffer comparison: the remaining effect gap of the I-0066 m40 comparison drops (I-0063 measured the effect families as the remaining forward difference) |

**On a witness zone (m37 Irithyll; retained capture `i63-live-10/m37-s3/frame-60`, OBSERVED
counts per frame, 3 frames captured):**

| Check | Expectation |
|---|---|
| Z1 | **≈ 68 patch-list draws per frame** (205 in the 3 captured frames) |
| Z2 | patch-list vertex strides 112 (≈ 40 per frame), 36 (≈ 27), 160 (1) |
| Z3 | m31: 5 per frame; m51: 6 per frame (stride 112 only); same capture series |

## 10. Risks

1. **Input layouts.** GX builds them per programme from static tables (decoded in part AM,
   FFX-INPUT-LAYOUTS: use those, not the signature guess). Mismatched formats
   show as garbled billboards.
   - Mitigation: take formats from the VS input signature and the observed strides (§7), then
     confirm on one capture's VB bytes.
2. **Sort order.** GX sorts recorded commands by `FUN_1400BF330` (camera distance) and reverses
   it for some passes (`+0x426` table). A wrong order shows as alpha artefacts. Keep the key and
   the reversal.
3. **Multi-context.** `sc+0xD8` > 1 hands per-context rings to jobs. The native records on one
   thread, so `sc+0xD8 = 1`.
4. **The pass type `ctx+0x18`** drives the blend-bank choice (types 3/5 vs glow type 6). A wrong
   mapping gives opaque or invisible effects.

## Exports

`decompiled/DS3-1152-F-<RVA>`: `00E6EA10`, `000C2400`, `000C1E30`, `00D648A0`, `0024E790`,
`021392C0`, `02139450`, `0213B500`, `0213B340`, `0213B6B0`, `0213B690`, `0213B6A0`,
`01921370`, `01921390`, `01920420`, `00385F70`/`00396F30`/`0039E900` (existing), `00299360`,
`002993A0`, `00299DD0`, `00299DC0`.
