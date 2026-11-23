# I-0068 part M: E07 GPU particles ready to code (VFX VS/GS/PS/CS, `ccbSceneParam`), the Ariandel snow first

Request (coordinator, 2026-10-10), item 4: E07 GPU particles, with VFX VS/GS/PS/CS,
`ccbSceneParam` and so on; the priority for Ariandel (snow). The format follows
[EFFECTS-PHASE2-E01-E02-NATIVE](EFFECTS-PHASE2-E01-E02-NATIVE.md).

Prerequisites from part H:

- the pass points (§3), including the **pass 0x45 submission with draw-pass flag 0x10**;
- native `GXFfxDrawPass` and the transcribed `FUN_140299E40` (§4);
- the draw context (§5.2).

Labels: STATIC, INFERRED, OBSERVED. Evidence: `evidence/I-0068/queries/ffx/e07/` (decompile
indexes) and `ffx/ffx-program-layouts.tsv` (DXBC reflection).

**Rule, as in part H:**

- **CALL** = no GX object touched;
- **TRANSCRIBE** = keep the function and replace each GX statement by its D3D12 equivalent, in
  the same order with the same objects;
- **NATIVE** = a contract implemented natively.

The GPU-particle code is a fixed sequence of GX statements on buffers the engine owns, so the
right route is a statement-by-statement transcription. The simulation then stays the game's
own: the shaders are the original bytecode and the counts come from the GPU.

## 1. The machinery (STATIC)

**Classes** (RTTI):

- `GXFFX::GXVfxStandardParticleAppearance` (vtable `143D65878`);
- `GXVFX::GXVfxStandardEmitterSeed` (`143D658E0`);
- `GXVfxStandardEmitter` (`143D6A7A0`), on the base `GXVfxEmitter` (`143D6A7D0`);
- force fields and collisions: `GXVfxDynamicForceField*`, `GXVfxDynamicCollision*`,
  `GXVfxEffectorManager`;
- sparks: `GXVfxSparkParticleAppearance` (`143D65D38`), `GXVfxSparkEmitter` (`143D6A808`).

**One GPU state block `S` per particle-system instance** = appearance instance `+0x700`. Its fields
(STATIC, from the functions below):

| `S+` | Content |
|---|---|
| `+0x40..+0x48F` | **`ccbSceneParam`** image (1104 B): elapsed time, camera near/far, parent move/rotation/scale/colour, texture and emit info, depth and collision parameters, camera/view/projection matrices… |
| `+0x40+0x450`, `+0x40+0xD60` | sources of `ccbForceFiledParam` (built by `FUN_14025D0D0`, `FUN_14025D8D0`, `FUN_14025DDF0`) |
| `+0x1230..` | **`ccbEmitionSeed`** image (752 B) |
| `+0x1520..+0x16DF` | **`gcbSceneParam`** image (448 B): base colour, near/far, blend, scale flag, camera fade/position, UV crop, motion blur, billboard and view-projection matrices |
| `+0x2824` | blend type (index into the GX blend bank `+8 + 8·type`; 0 → `+0x10`) |
| `+0x2890` | **capacity** (records) |
| `+0x2898` VS, `+0x28A0` GS, `+0x28B8` PS | draw programmes |
| `+0x28C0` | update CS (by quality: `…Highest` … `…Lowest`) |
| `+0x28C8` / `+0x28D0` | sort CS (`InitSort{1024,512,256,128,}` / `BitonicSort…`) |
| `+0x28D8` | emit CS (`CS_VFX_StandardParticleEmit`) |
| `+0x2900` | input layout (the VS reads only `SV_VertexID`) |
| `+0x24B8`, `+0x24D0`, `+0x24D8` | albedo, normal, depth or extra texture of the draw |
| `+0x2A20..+0x2A4C`, `[+0x2B80]+8 +0x110..+0x118` | SH coefficients × light colour (`cbCubeEnvSH1`) |
| **`+0x3A58`, `+0x3A60`** | **particle buffers A/B**: structured, 176-byte records × capacity, SRV + UAV |
| `+0x3A98`, `+0x3AA0` | their **append/consume UAVs** (D3D11 flag APPEND) |
| `+0x3AA8`, `+0x3AB0` | their SRVs |
| `+0x3A68` / `+0x3A70` | **draw arguments** (16 B, init `{0, 1, 0, 0}`) / its R32_UINT SRV |
| `+0x3A88` / `+0x3A90` | **dispatch arguments** (16 B, init `{0, 1, 1, 0}`) / its R32_UINT SRV |
| `+0x3AF0` | a second 16-byte arguments buffer (same init) |
| **`+0x3AF8` / `+0x3B00`** | the **counter buffer** (16 B, UAV with the D3D11 COUNTER flag), the source of both argument copies |
| `+0x3AB8..+0x3AD8`, `+0x3AC0` | sort buffers (`rwSortingBuffer`, `rwViewZBuffer`) and the sorted-index SRV the VS reads as `IndexedInstance` |
| `+0x3AE8` | `rBufferMax` SRV |
| `+0x3B08` / `+0x3B0C` / `+0x3B10` | previous / current / draw ping-pong index |
| `+0x3968` | pending emit count |
| `+0x3B15` | simulated this frame |
| `+0x3B38` | resources ready |
| `+0x3B39` | active |

**Global constant buffers** (STATIC; `[144962360]+0x18` = one holder shared by all systems):

| Holder | Buffer | Bound |
|---|---|---|
| `+0x08` | `gcbSceneParam` 448 B (`FUN_14025D590`) | GS b9, and PS b9 when a normal texture is set |
| `+0x10` | `ccbSceneParam` 1104 B (`FUN_14025D700`, `FUN_14025E660`) | CS b0 |
| `+0x18` | `ccbForceFiledParam` 3488 B (`FUN_14025D700`) | CS b1 |
| `+0x28` | `ccbEmitionSeed` 752 B (`FUN_14025E720`) | CS b5 |
| `[[GXRS]+0x4DD0]+0x2C0` | `cbCubeEnvSH1` 48 B | PS b11 |

**When the two halves run** (STATIC; appearance slot +0x40 `FUN_1402695E0(app, drawCtx, …)`):

- **Simulation** `FUN_1402A6A30(S, drawCtx+0x100, …)` runs only in the submission whose draw-pass
  word is 0x10, once per frame (guard `S+0x3B15`). INFERRED: this is part H §3's pass-0x45
  submission with flag 0x10.
- **Draw** `FUN_1402A7E70(S, drawCtx+0x100, matrices)` runs in the normal submissions, after the
  draw-context setups (`FUN_1402607E0`, `FUN_140263B50`, `FUN_140263A60`, `FUN_140263C70`,
  `FUN_140263BF0`).
- The FFX-SCENECTRL-NATIVE §3.1 assumption that GPU particles update in the update passes is
  corrected: the update passes run the CPU library only, and the GPU simulation happens at draw
  time in pass 0x45.

## 2. Resources (STATIC `FUN_1402A49F0` create, `FUN_1402AA130` release)

| `S+` | D3D11 (GX descriptor) | D3D12 |
|---|---|---|
| `+0x3A58`, `+0x3A60` | buffer `capacity × 176` B, bind UAV \| SRV (0x88), structured, stride 176 | default-heap buffer, `ALLOW_UNORDERED_ACCESS`; SRV and UAV `StructureByteStride 176`, `NumElements = capacity` |
| `+0x3A98`, `+0x3AA0` | UAV, elements = capacity, flag 2 (APPEND) | UAV **with a counter resource** (`pCounterResource`, 4 B, offset 0), one per buffer |
| `+0x3AA8`, `+0x3AB0` | SRV structured | SRV structured |
| `+0x3A68` (+`+0x3A70` SRV R32_UINT ×4) | 16 B, init `{0,1,0,0}`, indirect-args usage | default buffer, initial state `INDIRECT_ARGUMENT`; initial data through an upload copy |
| `+0x3A88`, `+0x3AF0` (+`+0x3A90` SRV) | 16 B, init `{0,1,1,0}` | same |
| `+0x3AF8` / `+0x3B00` | 16 B, UAV, stride 16, 1 element, flag 4 (COUNTER) | buffer + UAV with a 4-byte counter resource |
| sort buffers | sized by capacity class (1024/512/256/128) | same sizes |

- Release goes through GX's deferred-delete list `[4961AB0]+0x2B8` (`FUN_1400D8780`). Natively,
  retire through the frame fence.
- The engine keeps its pointer fields. They hold the native objects' tokens, as for the other
  transcribed FFX objects (part H §5).

## 3. Per frame (STATIC)

### 3.1 Simulation `FUN_1402A6A30` (once per frame per system)

1. CS b0 `ccbSceneParam` ← `S+0x40` (1104 B); CS b1 `ccbForceFiledParam` ← built image (3488 B)
   (`FUN_14025D700`). Unbind VS/HS/DS/PS/GS.
2. **Emit**, when `S+0x3968 > 0`:
   - CS b5 `ccbEmitionSeed` ← `S+0x1230`;
   - UAVs u0 `rwRandomSeed` (pool `[[144962360]+0x58][i]`), **u1 = particle buffer [current]
     (append)**, u2 = counter `+0x3B00`;
   - initial counts: −1 (keep), or **0 on the system's first emission** (`S+0x3B39` was 0);
   - SRVs t0 = `+0x3A90`, t1 = `+0x3AE8`;
   - CS `+0x28D8` (emit), **`Dispatch(max(⌈n/128⌉, 1), 1, 1)`**;
   - unbind; **`CopyResource(+0x3A88 ← +0x3AF8)`**; `S+0x3968 = 0`.
3. **Update**, when active (`S+0x3B39`):
   - **`ClearUnorderedAccessViewUint(+0x3B00, {0, 1, 1, 1})`**;
   - UAVs **u0 = buffer [current] (consume, keep count)**, **u1 = buffer [previous] (append,
     count 0)**, u2 = counter `+0x3B00` (count 0);
   - SRVs t0..t4 = `[S+0x2A10]+0x48`, depth `[S+0x24D8]` (or default), screen-space normal
     `[S+0x24E0]` (or default), `[S+0x37D0]+0x48`, `+0x3A90` (defaults: GX solid textures
     `[[4961AB0]+0x270]`);
   - CS `+0x28C0` (update by quality), **`DispatchIndirect(+0x3A88, 0)`**;
   - unbind; **`CopyResource(+0x3A68 ← +0x3AF8)`** (draw arguments) and
     **`CopyResource(+0x3A88 ← +0x3AF8)`** (next dispatch).
4. Swap: `S+0x3B08 = S+0x3B0C`.

### 3.2 Draw `FUN_1402A7E70` (each pass that draws the system)

1. GS b9 (and PS b9) `gcbSceneParam` ← `S+0x1520` (`FUN_14025D590`). PS b11 `cbCubeEnvSH1` ← SH
   × light colour.
2. **Sort**, when blend type `S+0x2824` is 1 (alpha) or 0x34:
   - CS b0 `ccbSceneParam` (`FUN_14025E660`);
   - UAVs u0/u1 = `+0x3AC8`/`+0x3AD8`; SRVs t0..t2 = `+0x3A70`, `+0x3AE8`, `+0x3AA8[+0x3B10]`;
   - **`InitSort` `Dispatch(1,1,1)`**, then **`BitonicSort` `Dispatch(1,1,1)`** (`+0x28C8`,
     `+0x28D0`; one group, size by capacity class);
   - unbind; GX state flush `FUN_1400C6990`.
3. **States:**
   - raster bank `+0x208`;
   - PS samplers s7 point clamp (`+0x2E0`), s8 linear wrap (`+0x2D0`);
   - PS SRVs t0 albedo, t1 null, t2 normal, t3 depth/extra (defaults as above; `S+0x2B00`
     forces the default albedo);
   - input layout `+0x2900`, **topology POINTLIST**;
   - **VS SRVs t0 = particle SRV [`+0x3B10`], t1 = `+0x3AC0`** (sorted indices);
   - HS/DS/CS null; VS `+0x2898`, GS `+0x28A0`, PS `+0x28B8`;
   - blend bank by `+0x2824`, depth bank `+0x1E8`;
   - VB slot 0 null, stride 0xB0.
4. **`DrawInstancedIndirect(+0x3A68, 0)`** (`FUN_1400C61B0`), then unbind.

## 4. Programmes (STATIC, DXBC reflection)

| Stage | Programme | Interface |
|---|---|---|
| CS emit | `CS_VFX_StandardParticleEmit` | t0 `rUavCounter`, t1 `rEmitMax`; u0 `rwRandomSeed`, u1 `appendEmitBuf` (append, 176), u2 `wEmitUavCounter` (16); b5 `ccbEmitionSeed` |
| CS update | `CS_VFX_StandardParticle{,Highest,Higher,MiddleHigher,Middle,MiddleLower,Lower,Lowest}` | t0 `csInputTex`, t1 `g_csDepthMap`, t2 `g_csScreenSpaceNormalMap`, t3 `rUpdateUavCounter`, t4 `csTargetTex`; u0 `consumeBuf`, u1 `appendBuf` (176), u2 `wEmitUavCounter`; b0 `ccbSceneParam`, b1 `ccbForceFiledParam` |
| CS sort | `CS_VFX_InitSort{1024,512,256,128,}`, `CS_VFX_BitonicSort…` | t0 `rSortCounter`, t1 `rBufferMax`, t2 `rSortParticleIn`; u0 `rwSortingBuffer`, u1 `rwViewZBuffer`; b0 `ccbSceneParam` (`m_ViewMatrix`) |
| CS counters | `CS_VFX_StandardParticleCounterDownSize…` | t0 `rUavCounter`, u0 `wUavCounter` (counter UAV). Present in the set; its call site was not read in this pass |
| VS | `VS_VFX_StandardParticle` | `SV_VertexID`; t0 `StructuredInstance` (176), t1 `IndexedInstance` (4) |
| GS | `GS_VFX_StandardParticle` | b9 `gcbSceneParam` (billboard and view-projection matrices…) |
| PS | `PS_VFX_{Single,Soft}Texture[Cheap][Bump][GlowLP\|GlowD]` | t0 `g_texAlbedo0`, t2 normal, t3 `g_depthMap` (soft); s7 `g_ClampPointSampler`, s8 `g_samLinearWrap`; b0 `PSCommonBuffer`, b6 `cbFog` (soft), b9, b11 `cbCubeEnvSH1` |
| shadow | `CS_VFX_StandardParticleGenerateSdw`, `GS/PS_VFX_…GenerateSdw`, `PS_VFX_…Sdw` | particle shadow volume (`g_csOut3DTex`) |

- The PS also reads `PSCommonBuffer` b0 and `cbFog` b6. INFERRED: they come from the draw-context
  setups of `FUN_1402695E0` before the draw, i.e. the part H blocks. Confirm on the first witness
  which b0 contents are used (`g_fToneMapReceiprocalExposure`, `g_psGlowScale`,
  `g_ps_ColorBlendType`, `g_ps_softSpriteParam`).

## 5. D3D12 translation (TRANSCRIBE each statement)

| D3D11 statement | D3D12 |
|---|---|
| `CSSetConstantBuffers(0..1 / 5)`, `GSSetConstantBuffers(9)`, `PSSetConstantBuffers(9, 11)` + `UpdateSubresource` of the global holder buffers | root CBVs from the upload ring with the same images (`S+0x40`, built force-field image, `S+0x1230`, `S+0x1520`, SH) |
| `CSSetShaderResources`, `VSSetShaderResources(0, 2)`, `PSSetShaderResources(0, 4)` | descriptor tables |
| `CSSetUnorderedAccessViews(0, n, uavs, initialCounts)` | UAV table; **for each initial count ≠ −1, write that value into the UAV's counter resource first** (`WriteBufferImmediate`, or a copy from a constant buffer), with barriers COPY_DEST → UNORDERED_ACCESS |
| `ClearUnorderedAccessViewUint(+0x3B00, {0,1,1,1})` | `ClearUnorderedAccessViewUint` (GPU handle from a shader-visible heap + CPU handle from a non-visible heap) |
| `Dispatch(x, 1, 1)` | compute PSO + `Dispatch` |
| `DispatchIndirect(+0x3A88, 0)` | `ExecuteIndirect` with a DISPATCH command signature, args `+0x3A88` in INDIRECT_ARGUMENT |
| `DrawInstancedIndirect(+0x3A68, 0)` | `ExecuteIndirect` with a DRAW signature, args `+0x3A68` in INDIRECT_ARGUMENT |
| `CopyResource(args ← +0x3AF8)` | `CopyBufferRegion(16 B)`, with barriers: `+0x3AF8` UNORDERED_ACCESS → COPY_SOURCE; args INDIRECT_ARGUMENT → COPY_DEST; then back |
| between emit, update, sort and draw | **UAV barriers** on the particle buffers and the counter; the particle SRV the VS reads is in NON_PIXEL_SHADER_RESOURCE for the draw |
| draw states | graphics PSO: VS + GS + PS, input layout `+0x2900` (empty or `SV_VertexID` only), **topology type POINT**, blend/depth/raster bank entries, RT forward HDR (+ glow for `…GlowD`) |

**Root signatures:**

- compute: b0, b1, b5 CBV; SRV table t0–t4; UAV table u0–u2;
- graphics: VS SRV t0–t1; GS b9; PS b0, b6, b9, b11, SRV t0–t3, samplers s7 and s8 (static:
  point clamp, linear wrap).

## 6. Ordered list

| # | Function | Mode |
|---|---|---|
| 1 | emitter/appearance construction, the library's CPU update (emit counts, force fields, collisions) | CALL |
| 2 | **`FUN_1402A49F0`** (create GPU resources) | **TRANSCRIBE** (§2) |
| 3 | **`FUN_1402AA130`** (release) | **TRANSCRIBE** (fence retire) |
| 4 | appearance slot +0x40 `FUN_1402695E0` | CALL (it only calls the two below and draw-context slots) |
| 5 | **`FUN_1402A6A30`** (simulation) | **TRANSCRIBE** (§3.1, §5) |
| 6 | **`FUN_1402A7E70`** (sort + draw) | **TRANSCRIBE** (§3.2, §5) |
| 7 | `FUN_14025D590`, `FUN_14025D700`, `FUN_14025E660`, `FUN_14025E720` (constant uploads) | **TRANSCRIBE** (root CBVs; the CPU builders `FUN_14025D0D0/D8D0/DDF0` are CALL) |
| 8 | GX helpers `FUN_1400C5EB0` (Dispatch), `FUN_1400C5F40` (DispatchIndirect), `FUN_1400C61B0` (DrawInstancedIndirect), `FUN_1400BE7D0` (CS SRVs), `FUN_1400C6990` (state flush) | replaced by the native encoder |
| 9 | sparks: `FUN_1402B01E0` / `FUN_1402B1700` (the same calls with the `…Spark…` programmes) | TRANSCRIBE the same way, after the standard path |

**Pass hook-up:** the native pass 0x45 must perform the flag-0x10 submission (part H §3) before
any pass that draws particles. Otherwise no system ever simulates and every draw reads zero
arguments.

## 7. Checks

**Ariandel `m45`, the Snowfield bonfire view.** OBSERVED in I-0063 i63-live-20 s5 frame 40 and
the 2026-10-06 note:

- the snow is **two indirect draws with `PS_VFX_SingleTextureCheap`**;
- they read 176-byte records from buffers of **360,448 B (2,048 records, 1,710 live)** and
  **1,441,792 B (8,192 records, 5,130 live)**;
- the game's dispatch counts are about **1,770–1,780** at those frames (i63-live-32).

| # | Check | Pass |
|---|---|---|
| G1 | resources | two systems with capacity `S+0x2890` = 2,048 and 8,192; buffers 360,448 and 1,441,792 B; stride 176 |
| G2 | simulation runs | per frame, one update dispatch per system in pass 0x45 and one indirect draw per drawing pass; emit dispatches when `S+0x3968 > 0` |
| G3 | live counts | draw arguments `+0x3A68` read back (debug only, one frame late): vertex count of the order of 1,710 and 5,130 at the same view after the same warm-up time; dispatch-argument groups consistent with about 1,770–1,780 |
| G4 | image | the falling flakes are present (the I-0063 flake texel (1211, 100) at frame 40 of the same path); the back-buffer gap of the m45 view falls toward the non-snow residue (I-0063: 36,000–63,000 texels attributed mostly to snow) |
| G5 | first frames | no zeroed start: the native backend owns the buffers from the system's creation, and the first emission uses initial count 0 (I-0063's harness start-up defect must not reappear) |

**m40** (OBSERVED live in I-0063: `CS_VFX_StandardParticle*`, `…Emit`,
`PS_VFX_SingleTextureCheapGlowLP`):

| # | Check | Pass |
|---|---|---|
| G6 | programmes | the same CS/PS family drawn at the bonfire view; GlowLP writes the glow target |
| G7 | debug layer | no UAV hazard, no counter misuse (D3D12 validation clean over 600 frames) |

## 8. Risks

1. **Append/consume initial counts.** D3D11 resets a counter through `CSSetUnorderedAccessViews`;
   D3D12 has no such argument. A missed reset (count 0 on u1 of the update) doubles particles
   every frame. A missed keep (−1) loses them.
2. **The argument copies' order.** The draw reads `+0x3A68` copied after the update. Copying
   before the update, or omitting the barrier, draws last frame's count with this frame's
   buffer. The result is flicker or garbage particles.
3. **Pass 0x45 flag-0x10 submission.** Without it nothing simulates (§6). It must also precede
   the first drawing pass of the frame.
4. **Multi-context.** GX may record different systems on different contexts (I-0063 "the order
   of their dispatches across contexts"). Natively, record all FFX GPU work on one command list
   in submission order.
5. **Sort sizes.** `InitSort`/`BitonicSort` run a single group sized by the capacity class.
   Choosing a smaller class than the original truncates the sort.
6. **`CounterDownSize` not located.** If a system uses it, its call site is outside the two
   functions read here; the native encoder must log any VFX CS not covered by §3.

## Exports

`decompiled/`: `002A6A30`, `002A7E70`, `002A49F0`, `002AA130`, `002A9BC0`, `002695E0`,
`0025D590`, `0025D700`, `0025E660`, `0025E720`, `000C5EB0`, `000C5F40`, `000C61B0`,
`000BE7D0`, `01921320`, `019211E0`, `01920B80`, `01921160`, `01921260`, `019212F0`,
`01920200`, `01920220`, `01920260`, `019205E0`, `019208A0`, `01920A50`, `01920C00`.
Existing: `002B01E0`/`002B1700` were read in part D (not exported).
