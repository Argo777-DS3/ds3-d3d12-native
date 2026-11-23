# I-0068 part BA: the screen-space ambient occlusion of the original, what native lacks, and a D3D12 design for the frame plan (static)

Request (coordinator, 2026-10-11): study the original's SSAO statically (chain, shaders, targets, per-zone parameters, what native lacks); propose a D3D12 design inspired by ER (D-0157) that fits the frame plan of phases A to G of the D3D12 review; witnesses in the five zones and live rows. Complements [SSAO-NATIVE](SSAO-NATIVE.md) (part Q: the observed chain and the ready-to-code order), which this note does not repeat; read its §1 first. Nothing was launched, native code was not changed.

Labels: **STATIC** (decompilation, DXBC of the cached programmes, the decoded GParam files of the retained game data), **OBSERVED** (the I-0063 captures cited by SSAO-NATIVE), **INFERRED**, **OPEN**. Evidence (outside Git): `evidence/I-0068/queries/ssao/` (`dxbc/` disassembly of the four AO programmes, `s1/` and `s2/` decompiled driver and parameter-binding functions, `f1/` displacement searches, `x1/` the parameter names); GParam tables `evidence/I-0068/queries/yebis/zone-gparam/*.tsv` and `.../gparam/*.tsv`.

## 0. Answer first

1. **The AO is a four-programme pixel-shader chain at half resolution with temporal reprojection**: a depth downsample, a normal-oriented 8-pair horizon pass with a 64×64 random-rotation texture, a reprojection with neighbourhood clamp and depth-aware history rejection, and a depth-guided bilateral upscale to the full-resolution `t33` (RGBA8, `x` = AO, `y z w` = the linear depth packed in base 255). All of it is the engine's own DXBC; native keeps it unchanged (§1).
2. **The parameters are the same in all five zones.** The zone GParam files carry no `SSAO` group; the effective set is the world-wide one (`mapbase`, id 0: Sample Offset 1.5, AO Scale 1.2, kernel scale 10 / 0.2 / 2, previous-frame strength 0.7, depth fade 100 + 200, precision 4000, re-projection, prevent-ghost and up-scale on), selected by the quality object at HIGH. The per-zone difference is not in the AO pass but in the lighting: `LightSet.SsaoWeight` per directional light of each light set (§2.3).
3. **Whether the chain runs at all is decided by the quality object and the layer's flags** (`FUN_1401B26E0`): the SSAO quality flag `+0x0A` (set at HIGH and MAX only), the layer's *UseReProj* byte (`+0x161`, or the MAX flag `+0x0D`), and the per-view record (`+0x60` enable, `+0x64` strength > 0). At LOW and MEDIUM the chain does not run and `t33` stays white: native must reproduce that rule, not only the passes (§2.4). **CORRECTED (part BK, SSAO-WAVE1-READY §1.4): the AO pass runs from LOW upward; `+0x0A` / `+0x0B` / `+0x0C` and the MAX forcers only choose the temporal pass, the upscale and the normals; only DISABLE (`+0x09` = 0) skips the AO.**
4. **Native has none of it** beyond the constant producers, and those are *snapshot-driven* (they read the arguments of the engine's own calls captured by the I-0045 substitution route). The GX objects they read (the SSAO layer, the pass record, the previous-frame object) are not built natively, so the production port needs two more pieces than SSAO-NATIVE listed: a native **parameter binder** (GParam → layer fields, §2.2) and a native **view record** for the AO (§3).
5. **Design**: one `AoPass` module recorded as `PassKind::Effect`-style passes of the frame plan, history as a **ping-pong pair** (the reprojection pass writes the slot the next frame reads, so the original's history copy disappears), transient half-resolution targets aliased by life, depth read through a read-only DSV + SRV, constants from the frame arena, one shared post-pass root signature and prewarmed PSOs. ER contributes only its architecture (typed pass records, barriers finalised before encoding); its folder has no AO study (§4).
6. **Order and witnesses**: eleven steps, each with its check at the five zones (m31, m37, m45, m50, m51), and live rows SS1 to SS12 (§5).

## 1. The chain and the shaders (STATIC; structure OBSERVED, SSAO-NATIVE §1)

| Pass | Programme (cache name) | Inputs | Output | What it computes |
|---|---|---|---|---|
| 1, 2 | `DepthScale2x2OutputDepth` (twice) | `t1` depth | depth-only 640×360 then 320×180 | 2×2 depth reduction written through the depth output (`cbSampleOffset`: the four ±0.5-texel taps) |
| 3 | `ScreenSpaceAO2_WithNormal` | `t0` half depth, `t1` G-buffer normal (GBuffer0), `t2` 64×64 random rotation; `s0` wrap point, `s2` clamp point, `s4` white-border point; `b0 cbSSAOParam`, `b1 cbSampleOffset[8]`, `b2 cbSSAOParam2` | raw AO 640×360 RGBA8 | rebuild the view-space position from depth and `g_mInvProj`; if the depth is beyond `g_vDepthRangeParam.x` (the clip range) the output is just the offset term; else read the normal (`g_mView` rows), a random 2-D rotation from `t2` (indexed by `uv × screenSize / 64`), a kernel radius `clamp(baseDist / depth, min, max)`, and for each of **8 sample pairs** (±offset rotated by the random vector) sample two depths, rebuild both positions, and accumulate `w_i · (maxDepthTerm · (1 − a²) + a²)` with `a` the mean of the two normal-angle terms `sat(1 − dot(dir, n))`; the sum is faded by distance (`sat(depth · fadeScale + fadeBias)`), scaled `AO · AOScale + AOOffset`; the output packs the linear depth (`depth / PrecisionDepth`) as `frc(d · 65025)`, `frc(d · 255)`, `d` in three channels |
| 4 | `ScreenSpaceAO2_SeparatedReProj` | `t0` depth, `t1` raw AO, `t3` previous AO; `s2`, `s3` clamp linear, `s4`; `b0`, `b2` | reprojected AO 640×360 RGBA8 | reproject the pixel with `g_mCurToPrevScreen` (`cbSSAOParam2` +176), reject the history when the previous packed depth differs from the current linear depth by more than `depth × rejectRate` (`PrevFrame Depth Aware`, `Depth Reject Rate`), clamp the history to the min/max of the raw AO's 3×3 neighbourhood (the *PreventGhost* clamp), blend with `PrevFrame Strengh` |
| 5 | history copy (programme id 191) | the reprojected AO | the history texture | `CopyResource` of a same-size target (INFERRED) |
| 6 | `ScreenSpaceAO2_BilateralUpScale` | `t0` full depth, `t1` reprojected AO; `s2`, `s4` | **`t33` 1280×720 RGBA8** | four half-resolution taps weighted by the bilinear phase and by `1 / (|packedDepth · linearDepth − 1| + 0.01)` (depth similarity), `x` = the weighted AO |

The AO shaders read `cbSSAOParam` (48 B: `g_vDepthComputeParam = {p22, p32/f, f, 0}` and two blend vectors, the latter unused here), `cbSampleOffset` (256 B: the 8-entry kernel `{x, y, weight, w}`) and `cbSSAOParam2` (288 B: `g_mInvProj`, `g_vScreenSize`, `g_mView`, `g_vAOScaleOffset2`, `g_vSampleRangeParam`, `g_mCurToPrevScreen`, `g_vPrevFrameStrengh`, `g_vMiscParam`, `g_vDepthRangeParam`); the native producers `SsaoParam`, `SsaoKernel`, `SsaoParam2` and `DownsampleOffsets` (`native/world/light_producers.cpp:327-410`) already reproduce them bit-for-bit from the engine's arguments.

The readers are as SSAO-NATIVE §1: `t32` (shadow/AO composite: `GXRend_CompositeShadowMask`, which multiplies in `t33`) for `GXLightDeferredAO` and the sun; `t33` directly for the point, spot and line lights and for the forward programmes.

## 2. Parameters

### 2.1 What the original reads (STATIC)

The SSAO layer (`GXSSAO`, constructed by `FUN_1401B1050`; its vtable member `FUN_1401B22E0` is the pass callback) binds the GParam group `SSAO` to its fields in `FUN_1401B3FB0` (names read from the binding strings, `x1/`):

| Layer field | GParam parameter | Shader constant (`cbSSAOParam2` unless noted) |
|---|---|---|
| `+0x160` | `UseNormal` | selects `WithNormal` against the normal-less programme |
| `+0x161` | `UseReProj` | enables the chain with the quality flag `+0x0D` (§2.4) |
| `+0x162` | `PreventGhost` | selects the history clamp variant |
| `+0x190` | `EnableUpScale` | selects the bilateral upscale |
| `+0x164` | `Ver2 Sample Offset` | scales the 8-entry kernel radius (`FUN_1401B26E0`: `k_i · (i·c + 1) · offset`) |
| `+0x168`, `+0x16C` | `Ver2 AO Scale`, `Ver2 AO Offset` | `g_vAOScaleOffset2 = {scale · g, (offset − 1) · g + 1}`, `g` = the previous-frame object's `+0x64` |
| `+0x170`, `+0x174`, `+0x178` | `Kernel Scale Base Dist`, `Min`, `Max` | `g_vSampleRangeParam = {range · baseDist, min, max}`, `range = uint(layer+0xC) / [0x143D7F044]` |
| `+0x17C`, `+0x180`, `+0x184` | `PrevFrame Strengh`, `PrevFrame Depth Aware`, `PrevFrame Depth Reject Rate` | `g_vPrevFrameStrengh` (`cb2[15]`) |
| `+0x188` | `Geom Bent Offset` | `g_vMiscParam` (unused by the four programmes) |
| `+0x194`, `+0x198` | `Ver2 DepthFadeStart`, `Ver2 DepthFadeRange` | `g_vDepthRangeParam = {(p22·d + p32)/d, 1/range, −sign(start/range)}`, `d = start + range` |
| `+0x19C` | `Ver2 PrecisionDepth` | the float argument of `cbSSAOParam` (`f`) |
| `+0x158` | `Ignore setup from outside` | when set, the debug GUI values win |

(Offsets and names are STATIC from the binding function; the shader-constant column is STATIC from the producers' formulas and the DXBC use, with the field-to-name pairing INFERRED where two parameters share a register.)

### 2.2 Which values apply (STATIC, decoded GParam tables)

| Source | Values |
|---|---|
| `default` id 0 | UseNormal 1, UseReProj 0, PreventGhost 0, UpScale 0, Sample Offset 4, AO Scale 1, AO Offset 0, kernel 10 / 0.2 / 2, PrevFrame 0.7 / 1 / 0.02, DepthFade 3800 + 200, Precision 4000 |
| `default` ids 10-15 (the quality and view variants) | id 10: UseNormal 0, no reprojection, Sample Offset 2, Scale 1.2, kernel max 1.5; ids 11-15: UseNormal, UseReProj, PreventGhost, UpScale all 1, Sample Offset 2, Scale 1.2, kernel max 1.5, DepthFade 3800 + 200 |
| `mapbase` id 0 | UseNormal, UseReProj, PreventGhost, UpScale 1, **Sample Offset 1.5, Scale 1.2**, Offset 0, kernel **10 / 0.2 / 2**, PrevFrame 0.7 / 1 / 0.02, **DepthFade 100 + 200**, Precision 4000 |
| the five zones' own files | **no `SSAO` group** |

The captures show all four re-projection and upscale passes in the five zones (SSAO-NATIVE §1), which only the `mapbase` set enables (`UseReProj` and `UpScale` 1; `default` id 0 has both off). So the effective world set is the `mapbase` one for m31, m37, m45, m50 and m51 (the zones inherit it because their files leave the group empty; the resolution rule is the pilot's `Banks::Resolve`, with the area of the `MapBase` registration still the G4 audit item, row SS3). The menu views use ids 11 and 12-15 of `default`.

### 2.3 What differs per zone: the lighting side

`LightSet.SsaoWeight` is a per-light parameter of every light set (four rows per set, one per directional light). It does not change the AO pass; INFERRED from its name and its place in the light rows: the weight with which the lighting applies the AO for that light. Light set 0 (the zone's default set), lights 0 to 3:

| Zone | Light sets | Light set 0 `SsaoWeight` (L0, L1, L2, L3) | Distinct values over all sets |
|---|---|---|---|
| m30 High Wall | 43 | 0.70, 1, 0.76, 1 | 7 |
| m31 | 19 | 0.53, 1, 0.75, 1 | 7 |
| m37 | 16 | 0, 1, 0, 1 | 5 |
| m40 | 15 | 0.75, 1, 0, 1 | 4 |
| m45 | 17 | 0.80, 1, 0.75, 1 | 5 |
| m50 | 21 | 0.73, 1, 0.60, 1 | 9 |
| m51 | 15 | 0.80, 1, 0.75, 1 | 6 |

(`default.gparam` carries 7 sets with values from 0 to 1.) The runtime rows (I-0129 RESULT « R9d », `native_light_fill`, DS3's own
blocks) match these values with L1 and L2 swapped at m30, m37 and m40: the table's L1 and L2 labels are likely swapped (INFERRED);
the deferred programme weights Col_1 by the row's second component (`GXLightAcc_DDirLightShadowed1`). These are consumed by the native lighting producers already (they read the light bank); the AO design only has to deliver `t33`/`t32`.

### 2.4 The gate (`FUN_1401B26E0`, STATIC)

```
quality = [[GXRS+0x5360]]                       // the quality object (QUALITY-OBJECT-NATIVE)
enabled = quality+0x0A != 0
          && (layer+0x161 != 0 || quality+0x0D != 0)    // UseReProj, or the MAX flag
          && record+0x60 != 0                            // the view record's enable byte
reproj  = layer+0x162 != 0 || quality+0x0D != 0          // PreventGhost, or MAX
if (!enabled) layer+0x224 = 0                            // the AO is off: t33 stays white
```

The SSAO quality levels set the flags (the quality table of MENUS-MAP, "Slot order" section: LOW sets `+9`; MEDIUM `+9,+0xB`; HIGH `+9,+0xA,+0xB,+0xC`; MAX all seven): so the chain runs from **HIGH** (Thomas's configuration) and at MAX, never at DISABLE, LOW and MEDIUM. The pass callback `FUN_1401B22E0` additionally needs the layer's depth object (`+0x240`) and the record's strength `+0x64 > 0`.

## 3. What native has and lacks (observed in the code, `5c12c7a6`)

| Item | State |
|---|---|
| `t33` | neutral white in the lighting (`scene_pass.cpp` comment "neutral white until the native SSAO family exists"), the forward pass and the point lights |
| Constant producers | `SsaoParam`, `SsaoKernel`, `SsaoParam2`, `DownsampleOffsets` exact, but fed from **snapshots of the engine's call arguments** (`Region` 5, 6, 8, 9 of the substitution route), not from live objects |
| Layer fields | the GX pipeline objects (the SSAO layer and its parameter binding `FUN_1401B3FB0`, the per-frame previous-frame object) are **not built natively**: the fields `+0x158..+0x19C` and the previous-frame object (`+0x20` matrix, `+0x64` blend) do not exist |
| Quality object | built natively (Native1255), levels from the live options: the flags `+0x0A`/`+0x0D` are readable |
| GParam banks | native `Banks` read the files; the group `SSAO` is not requested by any consumer |
| Resources and passes | none of §1; the paired shadow-mask composite pipeline exists and is fed with white |
| Random-rotation texture (64×64 RGBA8) | not located: INFERRED an engine system texture (SSAO-NATIVE step 1) |
| Previous-frame object | needs the previous frame's view-projection and the blend factor: a native record updated each frame **Writer found (part BK): `FUN_1401B1960`; `+0x64` is the strength factor `[[GXRS+0x5368]+0xB0]`, `+0x60` the validity byte (SSAO-WAVE1-READY §1.3).** |

## 4. D3D12 design (D-0157; ER for the architecture only)

### 4.1 Principles

- **Keep the engine's shaders and constants; change the execution model.** The four programmes are original SM5.0 DXBC (D-0161, review M3); the native producers keep their formulas; what changes is resource ownership, ordering and barriers.
- **Typed pass records, barriers by the plan.** ER's documented structure (typed operation records above the encoder, barriers finalised before encoding, queue `ExecuteCommandLists` + `Signal`) is the architecture taken over: the AO declares its resources and uses to the frame plan (phase E, R10) and encodes only draws. ER has no AO study and no AO number is borrowed from it.
- **Only what a consumer needs.** The 320×180 depth (pass 2) has no observed consumer in the AO chain (the AO reads the half depth); it is produced only if a later reader exists (row SS6), otherwise skipped.

### 4.2 Resources

| Resource | Size / format | Lifetime | Plan state |
|---|---|---|---|
| `AoDepthHalf` | 640×360, `R32G8X24_TYPELESS` (DSV `D32_FLOAT_S8X24_UINT`, SRV `R32_FLOAT_X8X24_TYPELESS`) | transient (this frame) | `DEPTH_WRITE` in pass 1, `PIXEL_SHADER_RESOURCE` after |
| `AoRaw` | 640×360 RGBA8 | transient | `RENDER_TARGET` → `PIXEL_SHADER_RESOURCE` |
| `AoHistory[2]` | 640×360 RGBA8 | **persistent**, ping-pong by frame parity | `RENDER_TARGET` in pass 4 (slot `cur`), `PIXEL_SHADER_RESOURCE` in passes 4 and 6 and at the next frame's pass 4 (slot `prev`) |
| `t33` `AoFull` | 1280×720 RGBA8 | transient, read by the composite, the lighting and the forward pass | `RENDER_TARGET` → `PIXEL_SHADER_RESOURCE` |
| `AoRotation` | 64×64 RGBA8 | persistent (ingress) | `PIXEL_SHADER_RESOURCE` |
| Depth sources | the main depth (`R32G8X24_TYPELESS`) and the readable copy | existing | read-only DSV + SRV of the same resource while the AO samples it |

**History as a ping-pong pair (D-0157).** The original's chain is "reproject into a target, copy the target into the history" (pass 5). On one queue, writing the reprojection straight into the slot the next frame will read gives the same data without the copy draw: pass 4 reads `AoHistory[prev]` (previous frame's output) and writes `AoHistory[cur]`; pass 6 reads `AoHistory[cur]`; the roles swap each frame. A reset (first frame, resize, teleport) needs no GPU clear: the engine's own blend factor `g` (the previous-frame object's `+0x64`, produced by `SsaoParam2` exactly as the original) is 0, and the shader multiplies the history by it. Resize recreates both slots.

### 4.3 Passes in the frame plan

Order (K-0407 / NATIVE-PASS-ORDER step 6): after the G-buffer and the readable-depth copy, before the shadow composite and `GXLightDeferredAO`.

| # | Plan pass | Reads | Writes | PSO (prewarmed, R9) |
|---|---|---|---|---|
| A1 | depth downsample | main depth (SRV) | `AoDepthHalf` (DSV) | `DepthScale2x2OutputDepth`, depth-only, `DepthFunc ALWAYS`, no render target |
| A2 | (optional) depth quarter | `AoDepthHalf` | quarter depth | same, only with a consumer (SS6) |
| A3 | AO | `AoDepthHalf`, GBuffer0 normals, `AoRotation` | `AoRaw` | `ScreenSpaceAO2_WithNormal` |
| A4 | reprojection | `AoDepthHalf`, `AoRaw`, `AoHistory[prev]` | `AoHistory[cur]` | `ScreenSpaceAO2_SeparatedReProj` |
| A5 | upscale | full depth, `AoHistory[cur]` | `t33` | `ScreenSpaceAO2_BilateralUpScale` |

When the gate of §2.4 is closed, none of A1 to A5 is recorded and `t33` binds the engine's white texture (the same default the lighting uses today), so the saved GPU time is the DISABLE/LOW/MEDIUM case's real behaviour.

### 4.4 Binding model and constants

One **post-pass root signature** shared by the AO, the shadow-mask passes, the composite and (later) the YEBIS passes: root CBVs b0, b1, b2, b5, b8 (addresses into the frame arena), one SRV table for `t0..t3`, `t32`, `t33`, a sampler table `s0..s15` (settled in D3D12-MASTER-PLAN §6 S1: no static samplers; the AO binds the engine bank rows 0, 1, 3, 4, 6 as `s0..s4`, exactly as the original prologue `FUN_1401B22E0` does with `PSSetSamplers(0, 5, ...)`; the YEBIS passes copy their own 16 states into the same table). The three AO constant blocks (48 + 256 + 288 bytes) are written into the arena once per frame (R2) from the native producers; descriptors for the AO come from the frame-linear heap (R6).

### 4.4a The native inputs the design adds

- `AoParams` (new, replaces the layer binding `FUN_1401B3FB0`): resolves the GParam group `SSAO` at id 0 of the world area (fallback rule `Banks::Resolve`), fills the fields of §2.1 once per area change or when the quality level changes, with the same defaults as the layer constructor `FUN_1401B1050` (`+0x194` = 3800, ...).
- `AoView` (new, replaces the pass record): the per-frame record read by the producers: view and projection matrices (`pass+0x50`, `+0x10`, `+0x78/+0x88`), the render size (`+0xA0/+0xA4`), the enable and strength bytes (`record+0x60/+0x64`), and the **previous-frame object** (previous view-projection and blend factor `g`), updated at the end of each AO frame and reset on a camera cut, a resize and a teleport.
- The producers' `Region` reads become reads of these two structures (a mechanical change; their formulas stay).

### 4.5 Fit with the D3D12 review phases

| Phase | What the AO needs from it | What it gives |
|---|---|---|
| A (R15, measure) | GPU time of A1-A5 (two timestamps), counters `ao_draws`, `ao_skipped` | the cost baseline; proof that LOW/MEDIUM skip the chain |
| B (R2, R3, R5, R4) | arena for the 3 constant blocks; the rotation texture through ingress; history textures retired by token on resize | none |
| C (R6, R7, R9, R8a) | frame-linear descriptors; five prewarmed PSOs; persistent draw records (the AO passes are static: one prepared draw per pass) | the post-pass root signature is a first use of R7 shapes |
| D (R1, R12, R14) | the `AoHistory` ping-pong is valid with frames in flight on one queue (same-queue order); the `AoView` previous-frame object and the arena constants are per-slot data; resize waits for all slots | no extra waits |
| E (R10, R11) | plan states and lifetimes; **aliasing**: `AoDepthHalf`, `AoRaw`, the quarter depth and the mask targets of the shadow chain share transient storage (disjoint lifetimes) | the model for the other post effects |
| F (R13, R8b) | the producers read `AoView`/`AoParams`, which the owner thread fills in prepare; the worker only records | `AoView` is an immutable snapshot per frame |
| G (R16) | none (the programmes are pixel shaders; a compute port would change the shaders) | — |

### 4.6 Risks specific to the design

1. **The history ping-pong equals the copy only if pass 6 reads the reprojection output, not the raw AO.** Verified from the DXBC: `BilateralUpScale` reads `g_SrcAOTex` (the reprojected target in the capture); the plan edge A4 → A5 encodes it.
2. **The gate fields.** `quality+0x0A`/`+0x0D` and the layer bytes decide the whole chain; a native mismatch silently disables the AO or enables it at LOW. SS1/SS2 read them in the original and natively.
3. **Stencil bit 0x10 of the composite** (SSAO-NATIVE risk 2) stays a G-buffer receiver question outside this design.
4. **The random-rotation texture** has no located owner: until it is, a deterministic native substitute would change the noise pattern (and the history convergence); take the engine's texture, never a substitute (row SS5).

## 5. Ordered native steps and witnesses

| # | Step | Witness (five zones m31, m37, m45, m50, m51 unless noted) |
|---|---|---|
| S1 | `AoParams` and `AoView` with the original defaults; quality gate (§2.4) | SS1, SS2: the flags and the effective parameter set equal the original's at each zone |
| S2 | resources of §4.2, the post-pass root signature, five prewarmed PSOs | debug layer clean; SS6 sizes |
| S3 | A1 downsample + A3 AO with the three blocks from the producers | A2 of SSAO-NATIVE (constants bit-identical); the raw AO image against the original's |
| S4 | A4 reprojection with the ping-pong history | SS7: no smearing after a camera cut; SS8: history converges in about 30 frames |
| S5 | A5 upscale into `t33` | A3 of SSAO-NATIVE: mean absolute difference of `t33` ≤ 2/255 on a still camera |
| S6 | `t33` bound in the lighting, the point lights and the forward pass (replacing white) | the lit HDR comparison (K-0426) improves at all five zones, no regression at m40 |
| S7 | composite with white mask B (SSAO-NATIVE step 8) | A4 of SSAO-NATIVE |
| S8 | gate off at LOW/MEDIUM/DISABLE: chain not recorded, `t33` white | SS9 **CORRECTED: the AO pass runs from LOW, only DISABLE skips it (SSAO-WAVE1-READY §1.4).** |
| S9 | reset rules (cut, resize, teleport) | SS7 **CORRECTED: no explicit reset; validity byte (SSAO-WAVE1-READY §1.3).** |
| S10 | plan states and aliasing when phase E exists | SS10 |
| S11 | cost and frame-in-flight check | SS11, SS12 |

### Live rows (none taken)

| ID | Orig/Nat | Read | Expected |
|---|---|---|---|
| SS1 | Orig | `[[GXRS+0x5360]]+0x0A`, `+0x0D`, the layer bytes `+0x160..+0x162` and `+0x190`, the record `+0x60/+0x64`, at each of the five zones with HIGH | `+0x0A` 1, `+0x0D` 0, `+0x161` 1, record enable 1, strength > 0 |
| SS2 | Orig | the layer fields `+0x164..+0x19C` (17 floats) | the `mapbase` set of §2.2 (1.5, 1.2, 0, 10, 0.2, 2, 0.7, 1, 0.02, 0, 100, 200, 4000) |
| SS3 | Orig | which GParam file and id the world view's `SSAO` group resolved to (the MapBase registration area, G4 audit) | `mapbase` id 0 (or the area override), identical in the five zones |
| SS4 | Orig, Nat | the three constant blocks of passes 3, 4 and 6 at one still frame per zone | bit-identical (A2) |
| SS5 | Orig | the identity and address of `t2` (64×64 RGBA8) and its first bytes | an engine texture; the native binds the same asset |
| SS6 | Orig | sizes and formats of the AO targets; whether anything samples the 320×180 depth | 640×360 / 1280×720 RGBA8; consumers of the quarter depth (decides A2) |
| SS7 | Orig, Nat | the first frame after a teleport or a camera cut: the blend factor `g` and the reprojected AO | `g` = 0, no smearing **CORRECTED: `g` is the strength factor (1.0); the guard is the validity byte `record+0x60`.** |
| SS8 | Orig, Nat | `t33` over 60 frames of a still camera | converges (history) within about 30 frames |
| SS9 | Orig, Nat | the options SSAO level LOW, MEDIUM, HIGH, MAX | chain absent at LOW/MEDIUM (white `t33`), present at HIGH/MAX; MAX enables the `+0x0D` path **CORRECTED: the AO pass is present from LOW; what `t33` holds without upscale is open (SSAO-WAVE1-READY §1.4).** |
| SS10 | Nat | debug layer with the plan driving the AO passes | 0 errors; aliased targets give the same image |
| SS11 | Nat | GPU time of A1-A5 (timestamps) and draws per frame | 6 draws (7 with the quarter depth); cost recorded for phase A's table |
| SS12 | Nat | `slots=2` against `slots=1` | identical `t33` on a still scene |

## 6. Open points

1. The exact meaning of the flags `+0x09` to `+0x0F` beyond the two the AO driver tests (`+0x0A`, `+0x0D`); SS1 reads them at the four levels.
2. The pairing of the binding strings with the layer offsets rests on the order of the binding calls; SS2 confirms it value by value.
3. The consumer, if any, of the 320×180 depth (SS6).
4. The owner of the 64×64 rotation texture (SS5).
5. Which GParam file the world view uses for a missing `SSAO` group (SS3); the captures only show that the `mapbase` set is the one in effect.

## Exports

No new export: read through Ghidra (`FUN_1401B22E0`, `FUN_1401B26E0`, `FUN_1401B33D0`, `FUN_1401B3FB0`, `FUN_1401B1050`); the DXBC of `ScreenSpaceAO2_WithNormal`, `_SeparatedReProj`, `_BilateralUpScale` disassembled by the system compiler (`evidence/I-0068/queries/ssao/dxbc/`). The producers are in `native/world/light_producers.cpp` (writers `1B33D0`, `1B38D0`, `1B26E0`, `1CB9D0`).
