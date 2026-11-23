# The glow of wave 1: forward second target, inputs, resources, gate and the ten passes (ready to implement, static)

Request (coordinator, 2026-10-11): an implementation-ready note for the glow of wave 1 (points 1 to 3 and 6 of [WAVE1-PREP](WAVE1-PREP.md) §2.2: G1 forward rows bind `[HDR, GB5]`, G2 `Banks::Glow` and the factor, G3 the glow resources, G6 the gate), built on the model-draw envelope ([MODEL-DRAW-ENVELOPE-READY](MODEL-DRAW-ENVELOPE-READY.md), commit W0a) and on the colour path ([GAMMA-COLOUR-PATH-READY](GAMMA-COLOUR-PATH-READY.md), `DisplayStage`). It gives the targets, the original shaders with their fingerprints, the constants, the pass order, the commits under flags, the tests and the witnesses by family. Independent Opus 5.5 review applied (log in §11). Static; no native code was changed, nothing was launched. The design of the filter itself is [GLOW-CHAIN-NATIVE](../I-0068-static-yebis-ffx-lights-spec/GLOW-CHAIN-NATIVE.md) and [EMISSIVE-GLOW-NATIVE](../I-0068-static-yebis-ffx-lights-spec/EMISSIVE-GLOW-NATIVE.md); this note re-reads `1BB810`, `7F950`, `7F820`, `7FED0`, `FB890`, `2227C0`, `1400C6220`, `1400C63D0` and the post-object initialiser `7ECB0` for the points that were open, and **corrects two statements of those notes** (§1.3).

Labels: **verified** = read in the executable (`DarkSoulsIII.exe` 1.15.2.0, SHA-256 `EF5E07C5…498C`) or in the decompiled functions for this note; **observed** = read in the native tree at `7864ed34` (line numbers are that tree's and will move: the anchors are function names); **note** = a statement of a linked note; **proposed** = this note's design; **open** = needs a read or a run. Abbreviations: `SP` = `native/renderer/scene_pass.cpp`, `GB` = `native/frame/game_bootstrap.cpp`, `LP` = `native/world/light_producers.cpp`, `MD` = `native/renderer/model_draw.cpp`, `MF` = `native/renderer/model_frame.h`.

## 0. Answer first

1. **The glow is ten fixed draws and a handful of numbers, all verified.** Extraction `EmissiveToGlowPS` (GB5 → source, `rgb × a`), four `Sample13Weight` blurs that halve down (source → L0 ¼ → L1 ⅛ → L2 1/16 → L3 1/32), three additive `GlowCompositeSinglePS` up (L3→L2, L2→L1, L1→L0), one onto the source (L0→S) and the last onto the final colour (S→final). Targets R11G11B10 except the last (the 8-bit colour target). The four programmes (three pixel shaders and the `ScreenSpaceQuadShader` vertex shader) are original DXBC, pinned by SHA-256 (§2); their constants are 13 taps `exp(−r²/2)` normalised, four scales and one gate (§3). The quad is **four vertices verified in the executable** (§1.2).
2. **Two statements of the earlier notes were wrong and are corrected here (§1.3)**: the blur taps are spaced in texels of the **target** level, not of the source (`cbFilterParam` reads the viewport, which `1400C6220` sets to the render target's size), and `BlurSampleScale` is a **float** (`record+0x10`, loaded with `MOVSS`), not an int. The first changes the image (taps one target texel apart, i.e. 4 source texels for the first blur and 2 for the others, a proper downsample footprint); the second means the scale needs no integer conversion (only the two enables are integers).
3. **Native has the sources and no filter** (§4): GB5 is written and sampled (`SP:1018`), and left in `RENDER_TARGET` by the lighting (`SP:1135`); forward rows bind one target (`GB:4913`); there is no glow resource, programme, input or gate.
4. **The design is one new class and four small producers.** `GlowStage` (`renderer/glow_stage.h/.cpp`, the shape of `DisplayStage`) owns the five R11G11B10 targets, the views, the root signature, four PSOs, the quad buffer and a per-frame constants buffer, and records the ten draws from a `NativeGlowInput` into any 8-bit target handed by the caller (`DisplayStage::Target()` once the colour path exists, the back buffer before). The producers are `Banks::GroupWord`/`Banks::Glow` (the engine's key rule and the `2227C0` blend), `CollectNativeGlowInputs` (beside `CollectNativeVelocityInputs`), `AcquireNativeGlowPrograms` (beside `AcquireNativeLightingPrograms`) and, for G1, the `Forward2` envelope of W0a (§5.6).
5. **Eight reversible commits under flags** (§6). Six are inert (no pixel of the final colour changes: math, bank reader, collect row, programmes, the stage not wired, forward draws writing GB5), one wires the ten draws (the first visible change, `no-glow.flag` reverts it), one adds the development aids. The idle High Wall frame digest must stay equal through GL-6; the glow appears with GL-7.
6. **Witnesses by family** (§8): `c2150`/`c5270` (`EmiIblGlow`) bloom, `c1090`/`c5110` (`EmiIbl`) do not, forward Glow draws are counted and imaged, FFX `…GlowD` waits for the effects phase, the veil `Veill_NoLight_Fwd` and every one-output forward programme must leave GB5 untouched.

## 1. What the original does (verified)

### 1.1 The gate and the ten passes (`FUN_1401BB810`, mode B)

Gate (decompile lines 66–68, asm `1401BB88C`): the filter record exists, `record+0x24` (Enable) is non-zero and, when the GX root `[144961B50]` exists, `factor = [[GXRS+0x5368]+0x80] > 0` (when the root is absent the factor is 1 and the gate passes). Then both post-object members must exist (`[GXRS+0x4DD8]`, `record→target`). `f = clamp(factor, 0, 1)`: the code computes `s = clamp(1 − factor, 0, 1)` and every scale is `(0 − g)·s + g`, i.e. `g·(1 − s)`, **in that form** (bit-exact for `factor ≥ 1`: `s = 0` and the value is `g` exactly). The upstream step `FB890` (lines 656–683) runs only when `frame+0x3ECA` is set; the **only direct byte write found in the image is in the pipeline constructor `85540`** (`MOV byte [RSI+0x3ECA],1`, evidence `g4`; wider stores or block copies of the frame object are not excluded, so this is **inferred**), so the byte is taken as constant 1 and native ignores it.

Mode B is `record+0x1C ≠ 0` (New Glow Enable, the value 1 in `mapbase.gparam` id 0). Its code is lines 219–423 of the decompile; line 423 jumps to the function exit, so **lines 424–447 are dead for both modes** (they repeat the mode test after the jump) and mode A reaches only the tail 448–453: a last `7F950` call with ps variant 1, source `post+0x928` and scale `[143D7EF40]` = 10.0, not `record+0x10`. Passes, in order (all share the VS `[post+0xC18]` and the quad of `7F820`):

| # | Programme (post member) | `t0` | Target (size) | Blend | `cb2.x` (`cbGlowParam`) | Call |
|---|---|---|---|---|---|---|
| 1 | `EmissiveToGlowPS` (`+0xBF8`) | GB5 (`record+8`) | S (`[filter+0x20]` object, **W × H**) | opaque | `Rgb·f` bound, not read | lines 110–146 |
| 2 | `Sample13Weight` (variant index 0 → entry 7) | S | L0 (`filter+0x28`, **⌈W/4⌉ × ⌈H/4⌉**) | opaque | bound, not read | loop `uVar11 = 0` (lines 228–252), `7F950(post, ctx, S, BlurSampleScale)` with ps variant 0 and mode 0 |
| 3 | same | L0 | L1 (`+0x30`, ⌈W/8⌉ × ⌈H/8⌉) | opaque | | loop 1 |
| 4 | same | L1 | L2 (`+0x38`, ⌈W/16⌉ × ⌈H/16⌉) | opaque | | loop 2 |
| 5 | same | L2 | L3 (`+0x40`, ⌈W/32⌉ × ⌈H/32⌉) | opaque | | loop 3 |
| 6 | `GlowCompositeSinglePS` (`+0xC00`) | L3 | L2 | `SRC_ALPHA, ONE, ADD`; alpha `ONE, ZERO, ADD`; mask RGBA | `Rgb·f` | do-loop lines 267–309, count 3, targets `+0x38, +0x30, +0x28` from sources `+0x40, +0x38, +0x30` |
| 7 | same | L2 | L1 | same | `Rgb·f` | |
| 8 | same | L1 | L0 | same | `Rgb·f` | |
| 9 | same | L0 | S | same | `Rgb·f` | lines 310–349 (source `*[filter+0x28]+0x48`, target the S object) |
| 10 | same | S | the final colour target (`*[record]+0x30`) | same | **`Final·f`** (`record+0x20`) | lines 350–406 |

Sizes at 1280 × 720 are 1280×720, 320×180, 160×90, 80×45, 40×23: the **observed** chain of EMISSIVE-GLOW §1 (all five witness zones and m40) and the formula `⌈W/(4·2^i)⌉ × ⌈H/(4·2^i)⌉` reproduces it (720/32 = 22.5 → 23). Whether the original follows the presenter size or is fixed at 1280×720 is **open** (row GL6). Every pass sets the target through `FUN_1400C5B00`/`1400C6220`/`1400C63D0`, which also **sets the viewport to the render target's width and height** (`1400C6220` stores the target texture's size, shifted by the mip level, as the context's viewport entry, and `1400C63D0` as the scissor; evidence `g6`), and every pass is a `7F820` draw: a **triangle strip of 4 vertices** from the buffer at `post+0xB58`, stride `0x18`, topology 5 (`7F820`: `Draw` with 4 vertices after `FUN_141920420(…, 5)`).

The state of the context between passes is set but not read by any shader of the chain: `cb2` is bound for passes 1–5 and used only by the composites; `cb0`/`cb1` are bound only by the blurs. A native implementation may bind exactly what each shader declares (§2).

### 1.2 The quad (verified; closes the open point of the earlier notes)

`FUN_14007ECB0` (post-object resource creation) creates the vertex buffer: `size 0x60`, `stride` 0x18 (= one `float4` position + one `float2` coordinate), data copied from six `.rdata` vectors (`[143D7FF00]`, `[143D7FE40]`, `[143D7F6D0]`, `[143D7FF50]`, `[143D80280]`, `[143D7FEF0]`) onto the stack and uploaded by `1419254F0` into `post+0xB58` (`7ECB0` asm `14007F18C…14007F21B`). The 24 floats, read from the executable:

| Vertex | Position `(x, y, z, w)` | Coordinate `(u, v)` |
|---|---|---|
| 0 | (−1, +1, 1, 1) | (0, 0) |
| 1 | (+1, +1, 1, 1) | (1, 0) |
| 2 | (−1, −1, 1, 1) | (0, 1) |
| 3 | (+1, −1, 1, 1) | (1, 1) |

The vertex shader `ScreenSpaceQuadShader` (§2) copies position and coordinate unchanged. So the native quad is the standard clip-space strip, **identical for every pass and every target size**; nothing is scaled by the target.

### 1.3 Corrections to earlier notes (verified)

| Earlier statement | Where | What the code says | Effect |
|---|---|---|---|
| "the shader samples `uv + off_i·(1/w, 1/h)`, `(1/w, 1/h)` = the source's texel size" | EMISSIVE-GLOW-NATIVE §8.2 | `7F950` copies the context's viewport array into locals right after `FUN_1400C5B00` bound the **target** (decompile lines 54–57; the locals `fStack_1a0/19c` sit at the width and height of a 24-byte viewport entry); `cbFilterParam[0] = (w/64, h/64, 0, 0)`, `cbFilterParam[1] = (1/w, 1/h, 0, 0)` with **w, h = the target's size** (`1400C6220` writes the viewport entry at state+0x258 with the width and height as floats and the count at +0x4D8; `1400C63D0` writes the scissor; `7F950` copies that entry and truncates the size with `CVTTSS2SI`/`CVTSI2SS`, so the constants are `1/trunc(w)`, `1/trunc(h)`) | taps one **target** texel apart: 4 source texels for pass 2, 2 for passes 3–5 (with `BlurSampleScale = 1`) |
| "`BlurSampleScale` is `(int)param_3[2]`" (Ghidra's rendering, which led to treating the scale as an integer) | the 1BB810 decompile, line 175 | every call site loads `MOVSS XMM3, [R12+0x10]` (asm `1401BBC90` in mode A, `1401BBEB4` and `1401BBEFE` in mode B): **a float** argument, which `7F950` keeps in `XMM3` (`MOVAPS XMM6,XMM3` at `14007F994`) and hands to the generator as its float `param_3` (`MOVAPS XMM2,XMM6` at `14007FB37`) | `Banks::Glow` returns floats for the scale; the two enables are the only integers (§5.1) |

GLOW-CHAIN-NATIVE §1 and EMISSIVE-GLOW-NATIVE §8.2 carry the corrections as a pointer to this note.

## 2. The programmes: original DXBC, pinned (verified)

From the I-0063 programme cache (`gxposteffect` bundle; `evidence/I-0068/queries/fingerprints/programs.tsv`, disassembly in `evidence/I-0068/queries/glow-ready/dxbc/`). SHA-256 over the whole container; "DXBC" is the 16-byte shader hash a capture tool shows.

| Programme | Stage | Bytes | SHA-256 | DXBC hash |
|---|---|---|---|---|
| `ScreenSpaceQuadShader` | vs_5_0 | 616 | `94EFD8245EE14DBA8B5018F3E762C487DB55788E3E6D90800737E45149DA168A` | `AE3D609D7F49B711E410913F5BAF1463` |
| `EmissiveToGlowPS` | ps_5_0 | 744 | `BC02ECF1710A075266122E4E28D6DF6AE0BCAB7B2B349FFB1C13D1C22D4E933B` | `C7E4B6D0F24D7ECCEDC15020652B3139` |
| `Sample13Weight` | ps_5_0 | 1384 | `CD4CE825F5B81AE66E9624C125E863E68B7AD8BE953A7DA6CB95532328932AA2` | `8A6612487E87FDB9DF2BDD9168826E6D` |
| `GlowCompositeSinglePS` | ps_5_0 | 936 | `F85C2B7C55774BE4418369DFC7CBFE4ABDDCA4AB48EC9E8A82495255F65D75B3` | `692AF0ED41C68455F9535210A86C2CF3` |

Semantics (from the disassembly):

| Programme | Inputs and bindings | Computation |
|---|---|---|
| VS | input `POSITION0` float4 (register 0), `TEXCOORD0` float2 (register 1); output `SV_Position` float4, `TEXCOORD0` float2 | `o0 = v0; o1.xy = v1.xy` |
| `EmissiveToGlowPS` | `t0 g_SourceTex`, `s3 SS_ClampLinear`; input `TEXCOORD0` | `r = t0.SampleLevel(s3, uv, 0); o0.rgb = r.rgb·r.a; o0.a = saturate(r.a)` |
| `Sample13Weight` | `t0`, `s3`; `cb0 cbFilterParam` (2 float4), `cb1 cbSampleOffset` (`g_avSampleOffsets[16]`, 14 vectors declared dynamically indexed) | `Σ_{i<13} cb1[i].z · t0.SampleLevel(s3, uv + cb1[i].xy · cb0[1].xy, 0)` (all four channels) |
| `GlowCompositeSinglePS` | `t0`, `s3`; `cb2 cbGlowParam` (1 float4) | `o0 = (t0.SampleLevel(s3, uv, 0).rgb · cb2[0].x, 1)` |

The unused `Sample13WeightforGlow` (1540 bytes, `7C1D499D3EA5118F…`) and `…Kernel` (1560 bytes, `401450D16F2BF397…`) belong to the legacy mode A and to other callers; mode B uses `Sample13Weight` (the **observed** m40 name of the four blurs, EMISSIVE-GLOW §1; the variant-index-to-name map `{7,9,10,11,12,13}` of `7F950` is a table of member slots whose fill was not decoded, so the name rests on the observation; row GL7 reads the DXBC hash of a bound blur).

**Which vertex shader**: the post member `+0xC18` is the one every glow pass (and `7F950`) binds; the plain `ScreenSpaceQuadShader` is **inferred** (the `…Z` twin, 820 bytes, reads `cb8` `g_vtxParam` for a depth value; none of `1BB810`, `7F950` or `7F820` binds a vertex-stage constant buffer). The row GL7 confirms the VS hash too. Either way the native result is the same quad at any depth.

## 3. The constants (verified)

### 3.1 The 13 taps and their weights

Generator `FUN_14007FED0(out, scale)` (mode 0): 13 pairs `(x, y)` from `.rdata` (`143D7FB10, 143D7FB18, 143D7F7D0, 143D7F7D8, 143D80250, 143D80258, 143D7FA10, 143D7FA18, 143D7FA20, 143D7FA28, 143D80210, 143D80218`, then `{−0.5, [143D7F0BC]}`), weights `w_i = expf((x_i² + y_i²)·k) / sqrtf(2π)` with `k = [143D7F0C0] = −0.5` and `2π = [143D7EEFC] = 6.283185`, summed in index order and **normalised** `w_i/Σw`; the written entry is `(x_i·scale, y_i·scale, w_i, 0)` and entries 13–15 are zero. The constants were read from the executable (bit images):

| i | `x`, `y` (bits) | `x`, `y` | normalised `w` |
|---|---|---|---|
| 0 | `00000000`, `00000000` | (0, 0) | 0.098182 |
| 1 | `BF000000`, `3F3504E6` | (−0.5, 0.707107) | 0.067479 |
| 2 | `3F000000`, `3F3504E6` | (0.5, 0.707107) | 0.067479 |
| 3 | `BF800000`, `00000000` | (−1, 0) | 0.059550 |
| 4 | `3F800000`, `00000000` | (1, 0) | 0.059550 |
| 5 | `BF000000`, `BF35048B` | (−0.5, −0.707101) | 0.067480 |
| 6 | `3F000000`, `BF35048B` | (0.5, −0.707101) | 0.067480 |
| 7 | `3F000000`, `3E715BEF` | (0.5, 0.235702) | 0.084271 |
| 8 | `00000000`, `3EF15BEF` | (0, 0.471405) | 0.087857 |
| 9 | `BF000000`, `3E715BEF` | (−0.5, 0.235702) | 0.084271 |
| 10 | `3F000000`, `BE715BEF` | (0.5, −0.235702) | 0.084271 |
| 11 | `00000000`, `BEF15BEF` | (0, −0.471405) | 0.087857 |
| 12 | `BF000000`, `BE715BEF` | (−0.5, −0.235702) | 0.084271 |

The lower pair has `y = −0.707101` (bits `BF35048B`) against `+0.707107` (`3F3504E6`): an asymmetry of the original's constants that the native table must **keep** (tests compare bit images, §7). The weights sum to 1 within 2e-6 (decimal rounding). **The bit-exact oracle already exists**: `GlowTaps` (`LP:970`) computes the same block from a snapshot (`w = std::exp((y·y + x·x)·k)/sqrt(n)`, accumulation in index order, then `w/sum`) and was qualified in I-0064 against captures; the native producer calls the same core (commit GL-1) rather than a second copy.

### 3.2 The three constant buffers

| Buffer | Bytes | Content | Bound by |
|---|---|---|---|
| `cb0 cbFilterParam` | 32 | `[0] = (tw/64, th/64, 0, 0)` (unused by the shader), `[1] = (1/tw, 1/th, 0, 0)` with `(tw, th)` = **the target's** width and height as floats | blurs (passes 2–5) |
| `cb1 cbSampleOffset` | 256 | 16 `float4`: entries 0–12 as §3.1 with `scale = BlurSampleScale`, entries 13–15 zero (a native choice: the original uploads 48 bytes of stack residue that the loop `i < 13` never reads) | blurs |
| `cb2 cbGlowParam` | 16 | `(g·(1−s), y_bits, 0, 0)` with `g` the record's scale for the pass (`Rgb` or `Final`), `y_bits` = the raw dword of `record+0x18` (AlphaScale, written to `.y`, read by no shader) | composites |

`1/64 = [143D7ED38] = 0.015625`, `1.0 = [143D7EE10]`. The existing `GlowParam` (`LP:919`) is the oracle of `cb2`: `(0−g)·s + g`, `s = clamp(1 − fade)`, `.y` = the dword at `+0x18`.

### 3.3 The values for the world (from `mapbase.gparam` id 0; the factor is 1 natively)

| Field | Value | Where it lands |
|---|---|---|
| Enable | 1 | gate |
| BlurSampleScale | 1 | `cb1` scale |
| Blur RgbScale | 1.5 | `cb2.x` of passes 6–9 |
| Blur AlphaScale | 1 | `cb2.y` bits `3F800000` |
| New Glow Enable | 1 | mode B |
| NewGlow Final ColorScale | 0.5 | `cb2.x` of pass 10 |

`remobase.gparam` id 0 (cut scenes): 1, 2, 2, 1, 1, 0.5. The factor object is built natively by `FUN_127010` (`GB:9253-9262`, value 1.0 at `+0x80`): **observed** and read every frame.

The resulting image, for orientation: `S = E` (extraction), `L0 = B(S)`, `L1 = B(L0)`, `L2 = B(L1)`, `L3 = B(L2)` with `B` the 13-tap blur at one target texel, then `L2 += 1.5·up(L3)`, `L1 += 1.5·up(L2)`, `L0 += 1.5·up(L1)`, `S += 1.5·up(L0)` and `final += 0.5·up(S)` (`up` = bilinear clamp upsample by the sampler).

## 4. What native does today (observed, tree `7864ed34`)

| Item | Native | Anchor |
|---|---|---|
| GB5 | `colors_[5]` RGBA16F, six G-buffer targets cleared to 0; sampled by the light composite as t8; every lighting target and `colors_[0..5]` go back to `RENDER_TARGET` at the end of `EncodeNativeLighting` | `SP:29-31`, `SP:1018`, `SP:1135` |
| forward draws | one target (`render_target_count=1`, `formats[0]=ForwardTargetFormat()`), blend on `RenderTarget[0]` only (alpha blend for blend mode 2 or fading), executed after lighting into the lit HDR (`native_light_rtvs_` slot 3) | `GB:4903-4925`, `SP:1593-1609` |
| validators | `MD:272` accepts 6 or 1 targets; the scene checks require 1 for the forward list | `MD:272`, `SP:1364-1366`, `SP:1555-1557` |
| composition | draws the lit HDR (or material colour) into the back buffer with a 3-vertex triangle, then the UI | `SP:1616-1629` |
| velocity inputs | `CollectNativeVelocityInputs` reads the view context keys at `+0x4008/+0x4070`, weight `+0x40C0`, epsilon `image+0x3D7ECE8`, factor `[[image+0x4961B50]+0x5368]+0xE0` and `Banks::GroupFloat`; result in `NativeSelectedScene::velocity`, copied to `NativeModelFrameInput::velocity` | `GB:4576-4600`, call `GB:4685`, `GB:2413`, `GB:5418`, `MF:75` |
| bank reader | `Banks::GroupFloat` returns floats only (`p.Float(i)` is a bit copy, so an integer parameter reads as a denormal) and **fails** for a key the group lacks in both the area file and `mapbase` and `default` | `LP:1320-1331` |
| programmes | `AcquireNativeLightingPrograms` resolves loaded programmes by bare name from the engine's repositories (`FindLoadedProgramme`) | `GB:2679-2735` |
| precedent for a stage | `NativeVelocityResources` builds its own root, PSO, constants buffer and views lazily, targets per scene generation | `SP:1218-1260` |
| glow | nothing: no resource, no programme, no input, no gate, no `Banks::Glow` | — |
| the `Glow` policy flag `0x10000` | has no consumer | `content/model_parameters.cpp:471` |

## 5. Design (proposed)

### 5.1 Inputs: `Banks::GroupWord`, `Banks::Glow`, `NativeGlowInput` (G2, G6)

**`Banks::GroupWord(area, id, group, param, uint32_t& out, source)`**: the body of `GroupFloat` returning the raw 32-bit word (`values[i][0]`); `GroupFloat` becomes `GroupWord` plus a bit cast (same results, existing callers untouched).

**`Banks::Glow(a_area, a_id, b_area, b_id, weight, epsilon, GlowRecord& out, std::string* sources)`**, new, beside `GroupFloat` (`light_producers.h:111`): for each key the six values of the group `Glow Filter` (group and parameter short names as in the dump `evidence/I-0068/queries/gparam/*.gparam.tsv`: `Enable`, `BlurSampleScale`, `Blur RgbScale`, `Blur AlphaScale`, `New Glow Enable`, `NewGlow Final ColorScale`; `Enable` and `New Glow Enable` as words), with the key rule of `GroupFloat` (`{−1, −1}` → `{1, 0}`; area file, else `mapbase`) and, **where the lookup fails, the built-in record** of `17DBE0` (Enable 0, scale 20, Rgb 3, Alpha 1, New Glow 0, Final 1; `PRODUCERS-UNION` §3.2: the bank returns its fallback record, not an error; `default.gparam` holds the Glow group only at id 10). Then the **`2227C0` blend** (verified, `FB890` lines 657–667):

| Weight `t` | `Enable` | scales `1,2,3,5` | `New Glow` (4) |
|---|---|---|---|
| `|t| ≤ ε` | A | A | A |
| `|t − 1| ≤ ε` | B | B | B |
| otherwise | B | `(b − a)·t + a` | B |

with `ε = image+0x3D7ECE8` (the epsilon the motion-blur producer reads), the record order `[Enable, BlurSampleScale, Rgb, Alpha, NewGlow, Final]` and `Enable`, `New Glow` taken as words. The keys are the **index-10** entries of the view context: A at `context+0x4028`, B at `+0x4090`, weight at `+0x40D0` (the same arrays as the motion-blur keys at indices 6: `+0x4008`, `+0x4070`, `+0x40C0`; `FB890` lines 657–659 read `+0x4028`, `+0x4090`, `+0x40D0`).

**`CollectNativeGlowInputs(NativeSelectedScene&, uintptr_t view)`**, new, called beside `CollectNativeVelocityInputs` (`GB:4685`), same context lookup (`scene_view_cpu_owners`, `gparam_banks`), reading keys, weight, `ε` and the factor `[[image+0x4961B50]+0x5368]+0x80` (factor 1 when the root is absent, as the original): fills `std::optional<NativeSelectedScene::Glow> glow` only when **all** hold: the blend gives `Enable ≠ 0` **and** `New Glow ≠ 0` **and** `factor > 0` **and** `no-glow.flag` is absent **and** `AcquireNativeGlowPrograms()` returned the programmes (stored in `NativeGlowInput::programs`; an unresolved set leaves `glow` empty and logs the row of §5.2). Else the member stays empty (G6a) and a row says why. A record with `Enable ≠ 0` and `New Glow = 0` is mode A (the 20-texel two-direction chain of the built-in record) and is **not implemented in wave 1**: the row `native_glow_inputs` reports `mode_a=true` and no draws run (the world files never select it; `default.gparam` id 10 has Enable 0).

`NativeGlowInput` in `MF` after `NativeVelocityInput`:

```
struct NativeGlowInput {
    float blur_scale{1};      // BlurSampleScale: the 13-tap scale (a float)
    float rgb_scale{};        // (0 - Rgb)*s + Rgb with s = clamp(1 - factor): cb2.x of passes 6-9
    uint32_t alpha_bits{};    // the raw dword of AlphaScale: cb2.y (read by no shader)
    float final_scale{};      // (0 - Final)*s + Final: cb2.x of pass 10
    std::shared_ptr<const NativeGlowPrograms> programs;   // the four pinned blobs of §5.2; resolved by CollectNativeGlowInputs, empty = no glow this frame
};
```

`NativeModelFrameInput` gets `std::shared_ptr<const NativeGlowInput> glow;` after `velocity` (`MF:75`), copied from `scene->glow` at the same place as the velocity (`GB:5418`), **main view only** (`!offscreen`): the menu views keep `Forward1` and no glow.

The row `native_glow_inputs`: `{read, keys[4], weight, enable, new_glow, blur_scale, rgb, alpha, final, factor, sources, mode_a}` (the first three frames, on change, every 600th).

### 5.2 Programmes (G3a)

`NativeGlowPrograms { vertex, extract, blur, composite }` (byte vectors) in `MF`, filled by `AcquireNativeGlowPrograms()` beside `AcquireNativeLightingPrograms` (`GB:2700`): `FindLoadedProgramme(0x4792D58, L"ScreenSpaceQuadShader", 1u)` and `FindLoadedProgramme(0x4792BF8, L"EmissiveToGlowPS" / L"Sample13Weight" / L"GlowCompositeSinglePS", 0u)`; **all four or none**; the result travels in `NativeGlowInput::programs` (a `shared_ptr`, so the renderer needs no `SetGlowSource` and no image access; the stage copies the bytes at `Open`). Each blob is compared with the SHA-256 of §2 (a pin like the display blobs of GAMMA-COLOUR-PATH-READY §3.2); a mismatch or a missing programme **disables the glow for the session softly** (row `native_glow_programmes_unresolved` with the missing names, 600th-repeat throttle like `GB:2673`, no `Refuse`: `Refuse()` is `ExitProcess`). Whether the `gxposteffect` bundle is resident in the repositories at the first frame of the native path is **open** (row GL8); if not, the producer retries each frame until it is, as the lighting does.

### 5.3 `GlowStage` (G3): resources, root, PSOs, constants

New `renderer/glow_stage.h/.cpp` (the shape of `DisplayStage`; no dependency on the image):

```
struct GlowBlobs { std::vector<uint8_t> vertex, extract, blur, composite; };       // validated copies
class GlowStage {
public:
    bool Open(ID3D12Device*, const GlowBlobs&, DXGI_FORMAT final_format, const D3D12_SAMPLER_DESC& clamp_linear_row4);        // root, 4 PSOs, quad buffer; false = disabled (reason logged)
    // Called at the start of every frame that has a NativeGlowInput: Open the first time (blobs from input.programs, the final target's format), Resize when the extent changed, false = disabled for the session (reason logged once).
    bool Ready(ID3D12Device*, const NativeGlowPrograms& programs, DXGI_FORMAT final_format, uint32_t width, uint32_t height);
    void Resize(ID3D12Device*, uint32_t width, uint32_t height);                  // after the caller's Drain: the five targets and their views
    // Records the ten passes. gb5 is in RENDER_TARGET on entry and on exit; final_rtv is an 8-bit target the caller keeps in RENDER_TARGET.
    void Record(ID3D12GraphicsCommandList*, ID3D12Resource* gb5, DXGI_FORMAT gb5_format, D3D12_CPU_DESCRIPTOR_HANDLE final_rtv,
                uint32_t final_width, uint32_t final_height, const NativeGlowInput&);
    void AfterSubmit();                                                          // hands the frame's constants buffer to RetireWhenSubmittedWorkCompletes
    void Close(); void Detach();
    uint32_t PassCount() const;                                                  // test and witness aid: 10 per Record
};
```

| Resource | Format, size | Notes |
|---|---|---|
| S (glow source) | `R11G11B10_FLOAT`, W × H of the presenter | RTV + SRV; created in `RENDER_TARGET`; no clear value needed (never cleared: pass 1 writes every texel) |
| L0..L3 | `R11G11B10_FLOAT`, `⌈W/4⌉×⌈H/4⌉`, `⌈W/8⌉…`, `⌈W/16⌉…`, `⌈W/32⌉…` | the same |
| RTV heap | 5 slots (private; `native_light_rtvs_` is full, `SP:975`) | |
| SRV heap | shader-visible, 6 contiguous slots: GB5, S, L0, L1, L2, L3 | GB5's view is rewritten when the resource or its format changes |
| quad | upload buffer 96 bytes, the 24 floats of §1.2, `D3D12_VERTEX_BUFFER_VIEW{stride 24}` | created once |
| constants | one upload buffer per frame, 10 slots × 3 × 256 bytes, each CBV a 256-byte-aligned root CBV; retired after submit | the staging pattern of `DisplayStage` (`RetireWhenSubmittedWorkCompletes` keyed on the submission at call time, so called after `gpu_.Execute`) |
| root signature | root CBVs `b0`, `b1`, `b2` (pixel), one SRV table (`t0`, pixel), a **sampler table of one descriptor at `s3`** holding a copy of engine sampler-bank row 4 (`EngineSamplerBankOwner` lease `descriptors[4]`, trilinear clamp, `MaxLOD` FLT_MAX: the original's `SS_ClampLinear`; the post family uses a sampler table fed from the bank rows its binding code names, master plan §6 S1, and row 4 does not change with the texture-quality level; tests pass the default descriptor) | `ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT`, deny the vertex-stage, hull, domain and geometry root access (the vertex stage binds nothing; the tree serialises root signatures at version 1.0, `backend.cpp:307`, `model_draw.cpp:202`, so no amplification/mesh flags) |
| input layout | `POSITION` R32G32B32A32_FLOAT at 0, `TEXCOORD` R32G32_FLOAT at 16, slot 0 | matches the VS signature of §2 |
| PSOs (4) | VS + PS, rasterizer solid / cull none (depth clip as the default: z/w = 1 passes), `SampleMask = UINT_MAX`, depth and stencil disabled, `DSVFormat = UNKNOWN`: **P-extract** and **P-blur** (R11G11B10, blend off), **P-level** (R11G11B10, blend `SRC_ALPHA, ONE, ADD`, alpha `ONE, ZERO, ADD`, mask all), **P-final** (the 8-bit format, same blend) | the blend is the **observed** state 17 of EMISSIVE-GLOW §1 |

The unused constant buffers are still bound for every draw (the root has the three CBV parameters; binding a harmless 256-byte zero slot for the buffers a shader does not declare avoids an unset-root-descriptor error); the blurs bind `b0`/`b1` and the composites `b2`.

Memory at 1280×720: S 3.5 MB, L0 0.9, L1 0.2, L2 0.06, L3 0.02 = about 4.7 MB (the "about 5 MB" of WAVE1-PREP); at 1920×1080 10.6 MB, at 3840×2160 42 MB.

### 5.4 Recording order and barriers (the ten passes)

Entry: GB5 `RENDER_TARGET` (the lighting leaves it there), the final target `RENDER_TARGET`, S and L0..L3 `RENDER_TARGET`. For each pass: `OMSetRenderTargets(1, rtv)`, viewport and scissor = the target's size, `SetPipelineState`, root signature, descriptor heap (the stage's SRV heap), SRV table at the source slot, the root CBVs, vertex buffer, `IASetPrimitiveTopology(TRIANGLESTRIP)`, `DrawInstanced(4, 1, 0, 0)`.

| Pass | Barriers before the draw | Target |
|---|---|---|
| 1 | GB5 `RT→SRV` | S (then GB5 `SRV→RT` at once: nothing else reads it this frame) |
| 2 | S `RT→SRV` | L0 |
| 3 | L0 `RT→SRV` | L1 |
| 4 | L1 `RT→SRV` | L2 |
| 5 | L2 `RT→SRV` | L3 |
| 6 | L3 `RT→SRV`; **L2 `SRV→RT`** (it was read by pass 5) | L2 (blend) |
| 7 | L2 `RT→SRV`; **L1 `SRV→RT`** | L1 (blend) |
| 8 | L1 `RT→SRV`; **L0 `SRV→RT`** | L0 (blend) |
| 9 | L0 `RT→SRV`; **S `SRV→RT`** (it was read by pass 2) | S (blend) |
| 10 | S `RT→SRV` | the final target (blend), kept in `RENDER_TARGET` by the caller |
| end | S, L0, L1, L2, L3 `SRV→RT` | |

That is **20 transitions per frame** (2 + 1 + 1 + 1 + 1 + 2 + 2 + 2 + 2 + 1 + 5; each pass's pair goes into one `ResourceBarrier` call). At the end every glow target and GB5 are in `RENDER_TARGET`, the between-frames state (as the velocity targets). The stage keeps a per-resource state so every barrier is explicit and no resource is read and written in one pass.

### 5.5 Placement in the frame (G4, G5)

In `RenderNativeModels` (`SP:1545-1670`), **between the composition draw (`SP:1624`) and `if(primitive)` (`SP:1626`)**: `if(input.glow && input.glow->programs && glow_stage_.Ready(device, *input.glow->programs, target_format, width, height)) glow_stage_.Record(list, colors_[5].Get(), color_formats[5], target, width, height, *input.glow)` (`target_format` = the back buffer's `R8G8B8A8_UNORM` today, the intermediate's later), with `target` the variable the composition and the UI already use. Today `target` is the back buffer (already in `RENDER_TARGET`, `SP:1619`); once `DisplayStage` lands (GAMMA-COLOUR-PATH-READY §3.5) `target = display_stage_.Target()` and nothing here changes: the glow is added to the 8-bit colour before the UI and before the gamma pass, as the original (EMISSIVE-GLOW §1, MASTER-PLAN §6 S4). When the YEBIS chain lands (YEBIS-D3D12-DESIGN-NATIVE §4.2) the call moves after the anti-aliasing output with the same arguments. `RenderNativeOffscreen` (`SP:1354-1431`), `RenderNativeMovie` and the offline `RenderFrame` are untouched. `SceneRenderer::Resize` (`SP:2119`) calls `glow_stage_.Resize` when the stage is open (after the existing drain); `Close` (`SP:2125`) and the quarantine in `~SceneRenderer` (`SP:2159`) call `Close` and `Detach`; `AfterSubmit()` follows the `gpu_.Execute(list)` of `RenderNativeModels` (and is a no-op when no glow was recorded).

### 5.6 G1: forward draws that write GB5 (on the W0a envelope)

W0a-4 makes `Forward2` valid (`[ForwardTargetFormat(), ModelTargetFormats()[5]]`, RT1 = GB5; the 2-handle `Encode` in both paths). This note adds the **producer rule**: at `GB:4913` a forward draw takes `shape.envelope = Forward2` (`render_target_count = 2`, `formats[1] = SceneRenderer::ModelTargetFormats()[5]`) **iff** its pixel shader declares a second output, i.e. `mesh.programs->interfaces[prepared.recipe][1]->outputs` holds an `SV_Target` with semantic index 1; otherwise (including a null `interfaces[recipe][1]`, reset on a reflection failure at `GB:3359`) it stays `Forward1`. This is the same image as the original's "bind both, write mask 0 for one-output programmes" (a draw that writes no GB5 leaves it untouched either way) and avoids a write-mask path: **no write-mask logic is needed**, and `Forward1` stays exactly today's code. After the blend section (`GB:4916-4925`) copy `RenderTarget[0]` to `RenderTarget[1]` (the original has `IndependentBlendEnable = 0`, so RT1 takes RT0's blend, 95/105 draws, EMISSIVE-GLOW §8.1; MODEL-DRAW-ENVELOPE-READY writes the same thing as an independent blend with RT1 = RT0, which is the same image: the producer copies the descriptor) for `Forward2`. Flag `no-forward-gb5.flag`: the producer never selects `Forward2` (today's behaviour).

Consequences, all **observed**: the 276 forward programmes with `o1 = r × cb3[9].xyz` (GLOW-CHAIN §2) become `Forward2`; draws with alpha blending (blend mode 2, `GB:4919-4922`) blend into GB5 with RT0's factors, so `GB5.w` becomes `o1.w + GB5.w·(1 − o1.w)` (the original does the same; `o1.w` is a mask × alpha); the PSO key already serialises all eight blend entries and the formats (`MD:236-240`, `MD:274-282`), so only W0a's `ModelPsoKeyFields` carries the new count; `Forward2` is meaningful only with lighting (`input.forward_draws` requires `lit`, `SP:1593`) and only on the main view. GB5 must be in `RENDER_TARGET` for the forward pass, which is the state the lighting leaves (`SP:1135`; a witness row asserts it).

## 6. Commits (each builds, passes the unit tests, is reversible alone)

| # | Commit | Behaviour change | Flag | Test | Revert |
|---|---|---|---|---|---|
| GL-1 | **pure math** `renderer/glow_math.h/.cpp`: the 26 tap bit images, `GlowTapImage(scale, uint8_t[256])`, `GlowFilterParamImage(tw, th, uint8_t[32])`, `GlowParamImage(g, alpha_bits, factor, uint8_t[16])`, `GlowLevelSize(W, H, level)`; `LP`'s `GlowTaps`/`GlowParam` call the cores (the Snapshot wrappers stay) | none | — | T1 | `git revert` |
| GL-2 | **bank reader**: `Banks::GroupWord`, `Banks::Glow` (built-in fallback, `2227C0` blend) | none (no caller) | — | T2 (`glow_bank_test`) | `git revert` |
| GL-3 | **inputs**: `NativeGlowInput`, `NativeSelectedScene::Glow`, `CollectNativeGlowInputs`, `frame->glow` copy, row `native_glow_inputs` | none (no consumer) | `no-glow.flag` | T3 (log row structure), live GL1/GL2 | `git revert` |
| GL-4 | **programmes**: `NativeGlowPrograms`, `AcquireNativeGlowPrograms` with the four pins, row `native_glow_programmes_resolved`/`_unresolved` | none | `no-glow.flag` | T3 (row check); the pins are tested with the stage (T4, GL-5) | `git revert` |
| GL-5 | **`GlowStage`** (resources, root, PSOs, quad, constants, `Record`, `AfterSubmit`, `Resize`, `Close`/`Detach`), not wired | none | — | T4, T5 on WARP | `git revert` |
| GL-6 | **`Forward2` producer rule** (§5.6) on W0a-4: envelope, formats, RT1 blend copy, row `native_forward_gb5` (draws with a second output, draws total); requires W0a-4 | none to colour; GB5 changes where only a forward row draws | `no-forward-gb5.flag` | T6 (envelope test T4 already covers the encoder) | flag / `git revert` |
| GL-7 | **wiring**: `glow_stage_` member, `Ready`, the call of §5.5 in `RenderNativeModels`, Resize/Close/Detach/AfterSubmit; row `native_glow_frame` (passes, sizes, scales, GPU ms when `measure.flag`) | **the glow appears** (G-buffer Glow families, and forward Glow after GL-6) | `no-glow.flag` | T7 live structure | flag / `git revert` |
| GL-8 | **development aids**: `glow-verify.flag` (every 120th frame: reads back S and L0..L3 means and the maximum of GB5.w into a row), `glow-factor.flag` (a float replaces `[[…]+0x5368]+0x80` at the read, for G6) | none unless the flags exist | the two flags | T8 | `git revert` |

Order: **GL-1 → GL-2 → GL-3** (CPU, any order after GL-1 for GL-2/GL-3), **GL-4**, **GL-5** (needs GL-1), then **W0a-4** (the model-draw plan) → **GL-6**, then **GL-7**, **GL-8**. GL-1..GL-6 change no pixel: the idle High Wall frame digest (master plan row C0) and `content_draws` must stay equal after each (GL-6 may change GB5 content, which nothing reads until GL-7: the digest of the final colour stays equal). Conflicts with the other waves: `GB` and `SP` are edited by phase C (R6/R9), the K workstream and the colour path; every anchor above is a function or a block name, and each edit is a few lines at a block boundary.

## 7. Tests

| # | Test | Where | Pass |
|---|---|---|---|
| T1 | `glow_math_test` (CPU, built by `Build-GameBootstrap.ps1`): the 26 tap words equal the bit images of §3.1; `GlowTapImage(s)` for `s ∈ {1, 2, 20}` equals **frozen golden byte arrays** (produced once from §3.1's table by the commit's throwaway cross-check against the pre-GL-1 `GlowTaps`, then committed as data: `GlowTaps` and `GlowParam` live in the unnamed namespace of `light_producers.cpp` and call the new cores after GL-1, so a test cannot call them nor use them as an independent oracle); weights sum to 1 ± 2e-6; entries 13–15 zero; `GlowParamImage` equals golden words for factors {−1, 0, 0.5, 1, 2} and both `g`; `GlowFilterParamImage` words for (320, 180); `GlowLevelSize(1280, 720, i)` = (320,180), (160,90), (80,45), (40,23), and (1920,1080) → (480,270)…(60,34), odd sizes (1281,721) round up | `Build-GameBootstrap.ps1` | all equal |
| T2 | `glow_bank_test` (CPU), a new executable of `Build-GameBootstrap.ps1` (which already compiles `light_producers` and `gparam`; **not** `light_check`: it is built only by `native/world/Build.ps1`, whose output is the retired `evidence/I-0039` tree that must not be recreated), taking the admitted gparam directory as a path argument: key {1,0} → mapbase id 0 (1, 1, 1.5, 1, 1, 0.5); an area file without the group falls back to mapbase; a key absent everywhere → the built-in record (0, 20, 3, 1, 0, 1); weight 0 → A, weight 1 → B, weight 0.5 → floats `(b − a)·0.5 + a`, `Enable`/`New Glow` from B; epsilon snap; `GroupFloat` unchanged on its existing cases | `Build-GameBootstrap.ps1` | exact |
| T3 | `CollectNativeGlowInputs` row (in the existing `Test-GameModelPreparation.ps1` witness): present and well-formed, `glow` empty with the flag, with `factor` forced 0 and with `Enable = 0` | script | rows as specified |
| T4 | pins (in `glow_stage_test`, so it ships with GL-5; GL-4 is covered by the row check of T3): `GlowBlobs` accepted with the four blobs of §2, refused with a flipped byte (stage disabled, no throw). The blobs are **not in Git or in the evidence store** (disassembly only): the test takes a directory argument holding `ScreenSpaceQuadShader.vpo`, `EmissiveToGlowPS.ppo`, `Sample13Weight.ppo`, `GlowCompositeSinglePS.ppo` (the I-0063 programme cache `C:\Users\User\.codex\I0063-cache\programs`, as `Get-ProgramLayouts.ps1`, or the files extracted from the game's `gxposteffect` bundle); without the argument the test is skipped and says so | `glow_stage_test` | refuse is soft |
| T5 | `glow_stage_test` (WARP, then hardware; blobs as T4): a 64×36 RGBA16F GB5 with a 4×4 patch of `(1, 0.5, 0.25, 1)` and elsewhere `(0, 0, 0, 0)`, an RGBA8 final target cleared to zero: after `Record` with `(blur 1, rgb 1.5, final 0.5)` the final target equals a float CPU reference (extraction, 13-tap bilinear blur at one target texel, composites in the order of §1.1) **that quantises to R11G11B10 after each of the nine glow-target writes** (a wider tolerance would hide a real error: nine round-trips of 5–6-bit mantissas exceed 3%), within 1/255 absolute on the 8-bit result; `PassCount() == 10`; no debug-layer message of severity WARNING or worse (the renderer counts them as errors); every glow target and GB5 in `RENDER_TARGET` afterwards (transition probe); a `(0, …)` scale leaves the final target bit-equal; GB5 alpha 0 everywhere leaves it bit-equal | new test executable | equal |
| T6 | forward second target: the envelope tests T2c/T4 of MODEL-DRAW-ENVELOPE-READY §8.1 plus the rule: a PS interface with `SV_Target1` selects `Forward2`, one without selects `Forward1` | `model_draw_envelope_test` | equal |
| T7 | live structure (row `native_glow_frame`): 10 passes per main-view frame, 5 target sizes as §1.1, `rgb`/`final` scales, `factor` | pilot | as expected |
| T8 | gate: `glow-factor.flag = 0` → no `native_glow_frame` rows, the final colour equals the `no-glow.flag` frame at the same camera; a negative factor and a factor above 1 give the clamped scales | pilot | equal |

## 8. Live witnesses by family

Pilots: `Pilot-LoadGameThomas.ps1 -Teleport "x,y,z" -Look`, `Run-PilotPair.ps1`, `Pilot-NativeWitness.ps1`; the rows `native_glow_inputs`, `native_glow_frame`, `native_forward_gb5`. Image checks compare **the same frame** (same-frame tap with `glow-verify.flag` and the pixel file), not two runs: the animation and the sky differ between runs.

| Family | Programmes (examples) | Witness | Expected |
|---|---|---|---|
| **G-buffer Glow** (56 families, `o5.w = FC_GlowScale.w`) | `EmiIblGlow_Bloody` | `c2150` (W3) at Firelink `m40_00` (309.3, −35.4, 617), the TestAnim save; `c5270` (W4) | `glow-verify`: GB5.w max ≈ 1 over the emissive texels; S non-zero there; the final image shows a halo of 1.5·0.5 weight at three scales; with `no-glow.flag` the same frame has none |
| **G-buffer plain Emi** (`o5.w` from a register, 0) | `EmiIbl_Bloody` | `c1090` (W3/W4/W7) at `m30_01` next to it; `c5110` | bright (lit by the composite), **GB5.w = 0, no halo**: S zero over the model |
| **Forward Glow** (276 programmes with `o1`) | `…EmiIblGlow_*_Fwd`, `SAT_Equip*_Fwd`, `Veill_Curtain`, cinder `_Fwd` | `native_forward_gb5` counts (draws with a second output > 0 where such materials draw); the m40 listing's forward draws all bind `…/12447` (G3 of EMISSIVE-GLOW §6); `glow-verify` GB5 changed only under forward geometry | RT1 follows RT0's blend; GB5 untouched where no forward row draws |
| **Plain forward, one output** | `Veill_NoLight_Fwd`, every `_Fwd` without `o1` | `native_forward_gb5`: they stay `Forward1`; GB5 unchanged under them | no bloom |
| **FFX `…GlowD`** (44) | flames, auras, spells | **waits for the effects phase**: the FFX draw path will need its own two-target envelope with the same RT1 = RT0 rule (EFFECTS-D3D12-DESIGN-NATIVE §2.4) | — |
| **Map `M[ARSN]_em` bowls** | High Wall torches (MENUS-MAP §54) | after the map families are admitted (EMISSIVE-GLOW G5) | halo on the bowls |
| **Gate** | any | `glow-factor.flag = 0`; `Enable = 0` in a test record | no glow rows, image equal to `no-glow` |
| **Non-regression** | all | idle High Wall frame digest equal after GL-1..GL-6; `content_draws` equal; `prepare_ms` unchanged | equal |

Measures: structural (`glow_factor`, `glow_enable`, forward draws that wrote GB5, passes per frame, glow bytes about 4.7 MB at 720p), cost (GPU ms of the ten passes with `measure.flag`: the estimate is under 0.3 ms at 1280×720, passes 1, 9, 10 being the full-size ones), and `prepare_ms` unchanged.

## 9. Risks

1. **Tap spacing is the target's texel (§1.3).** An implementation that follows the earlier note would blur half as wide at the first level. The test T5 pins the reference; the live row GL5 compares one blurred level with the original's when raw reads are possible again.
2. **R11G11B10 has no alpha and clamps at 65 504.** Extraction keeps `a` only in the colour; GB5 rgb values (`FC_EmissiveColor` 0.07 to 5, times `a`) stay far below the clamp; the composites add in float.
3. **GB5.w pollution.** Forward draws with alpha blending write source alpha into RT1 as the original does; the lit composite reads only GB5.rgb and runs before the forward pass.
4. **Debug-layer warnings are fatal** (`SceneRenderer::Diagnostics`, `SP:782`): the PSO output formats must match the views, descriptors set before the draw, every root CBV set, no clear without a matching optimised clear value (the stage never clears).
5. **`Refuse()` exits the process**: every glow failure (programme missing, pin mismatch, device failure in `Open`) is soft, logs a row and leaves the frame unglowed.
6. **Frame cost of per-frame upload buffers**: one 7.5 KB committed buffer per frame retired through the backend; a pool is an optimisation, not required.
7. **Shared files**: `GB`/`SP`/`MD` are edited by phase C, K and the colour path; anchors are block names, edits are small.

## 10. Open points and live rows (none taken)

| ID | Read | Expected |
|---|---|---|
| GL1 | the six glow values, the keys at `context+0x4028/+0x4090/+0x40D0` and the weight at the High Wall and m40, from the native row | A and B keys, weight 0 or 1, values 1, 1, 1.5, 1, 1, 0.5 |
| GL2 | `[[GXRS+0x5368]+0x80]` | 1.0 |
| GL3 | GB5 alpha over `c2150` and `c1090` in the original | ≈ 1 and 0 |
| GL4 | RT1's blend in a two-target forward draw | RT0's (OBSERVED, repeat once) |
| GL5 | one blurred level (`L0`) of the original over a bright patch against the native `glow-verify` readback | the target-texel spacing of §1.3 |
| GL6 | the glow target sizes at a presenter size other than 1280×720 | `⌈W/(4·2^i)⌉` (or the original's fixed size) |
| GL7 | the DXBC hashes of the VS and of the pixel shader bound for passes 1, 2, 6 of a capture | `AE3D609D…`, `C7E4B6D0…`, `8A661248…`, `692AF0ED…` |
| GL8 | whether the four programmes are resident in `0x4792BF8` / `0x4792D58` at the first native frame | resolved on the first frame |

Open design points: the remo (cut-scene) view key is not covered (`Banks` has no `remobase` selection; the world uses `mapbase` id 0); mode A is not implemented; the FFX twins; the exact engine sampler-bank row of `SS_ClampLinear`.

## 11. Review log

Independent review by an Opus 5.5 reader (read-only; given the note and the evidence, not conclusions; it re-read the decompiles, the evidence, the executable and the native tree at `7864ed34`). No blocker; **13 findings, all applied**. The reader found no problem in: the quad (§1.2, six vectors and strip order), the record blend and key indices (§5.1), the shader fingerprints and semantics (§2), the native anchors (every line and function checked was accurate), the ten-pass table (§1.1), the 26 tap bit images and weights (§3.1).

| # | Severity | Finding | Applied |
|---|---|---|---|
| 1 | important | the programme blobs never reached the renderer; `Ready` was called but not declared | `NativeGlowInput::programs` (shared_ptr), `GlowStage::Ready(device, programs, format, w, h)` declared, §5.1, §5.2, §5.3, §5.5 |
| 2 | important | T2 relied on `light_check`, built only by `native/world/Build.ps1` into the retired `evidence/I-0039` tree | T2 is `glow_bank_test`, a new executable of `Build-GameBootstrap.ps1` with the gparam directory as argument |
| 3 | important | T1 oracle impossible (unnamed namespace) and circular after GL-1 | frozen golden byte arrays committed as data; the pre-GL-1 cross-check is a throwaway step of the commit |
| 4 | important | T4/T5 had no source for the DXBC blobs; T4 sat on GL-4 before the test executable existed | the test takes a directory argument (I0063 cache or extracted bundle), skipped with a message otherwise; T4 moved to GL-5 |
| 5 | important | wrong citation: the viewport is written by `1400C6220`, the scissor by `1400C63D0`; sizes truncated | §0, §1.1, §1.3 corrected; constants are `1/trunc(w)`; the conclusion (target size) stands |
| 6 | minor | lines 424–447 are dead for both modes; mode A's last call uses `post+0x928` and 10.0 | §1.1 rewritten |
| 7 | minor | entries 13–15 are stack residue in the original | relabelled a native choice (§3.2 table) |
| 8 | minor | `filter+0x10` should be `record+0x10` | §0 item 2 |
| 9 | minor | the `3ECA` grep only covers direct byte writes | relabelled inferred (§1.1) |
| 10 | minor | root signature version 1.0 (no amplification/mesh flags); `SampleMask`, `DSVFormat`; depth clip unnecessary | §5.3 rows corrected |
| 11 | minor | T5 tolerance too loose for nine R11G11B10 round-trips | the CPU reference quantises after each write; 1/255 absolute |
| 12 | minor | wording against MODEL-DRAW-ENVELOPE-READY; GB5.w blend formula; null interface | §5.6: same image stated, `o1.w + GB5.w·(1 − o1.w)`, null `interfaces[recipe][1]` → `Forward1` |
| 13 | minor | GAMMA-COLOUR-PATH-READY §0 item 6 read as glow after the UI | reworded: YEBIS chain, anti-aliasing and glow, then the UI |

Post-merge alignment (2026-10-11, with SSAO-WAVE1-READY): the static sampler `s3` of §5.3 became a one-descriptor sampler table copied from bank row 4, as the master plan §6 S1 settles for the post family; `GlowStage::Open` takes the row-4 descriptor as an argument.

## Exports and evidence

- Evidence (outside Git): `evidence/I-0068/queries/glow-ready/` (`dxbc/` disassembly of the four programmes, `f2`, `f3`, `f4` exports of `7F5F0`, `7E130`, `7F4D0`, `g1`–`g4` greps, `g3` the post-object initialiser `7ECB0`, `g5`/`g6` the render-target and viewport functions).
- Tools: `Invoke-Map.ps1` with `I0067Map.java` and `I0068Grep.java` (unchanged); PE reads of the executable by a throwaway script (the 26 tap floats, the six quad vectors).
- No native code changed.

## Implementation log (thread « Vague 1 producteurs W5 », branch `claude/friendly-feynman-70213e`, 2026-10-13: W5d)

**GL-1.** `native/renderer/glow_math.*`: the 26 tap words, `TapImage` (cbSampleOffset), `FilterParamImage` (cbFilterParam),
`ParamImage` and `FadeSelect` (cbGlowParam), `LevelSize`; the snapshot producers `GlowTaps` and `GlowParam` of
`native/world/light_producers` now call the cores on the words they read. T1 (`glow_math_test`, run by the build with the
executable): the tap words and the two constants equal the executable's .rdata, the weights equal §3.1 within 5e-6 and sum
to 1 within 2e-6, entries 13-15 zero, cbGlowParam for the factors -1/0/0.5/1/2 (g exactly at factor 1), cbFilterParam at
320 x 180, the level sizes at 1280 x 720, 1920 x 1080 and 1281 x 721. The golden byte arrays of the sheet were not frozen:
the old `GlowTaps` was not callable as an independent oracle, so the test checks the independent sources instead (the
executable's words and the table of §3.1). **GL-2.** `Banks::GroupWord` (the raw first word) and `GroupFloat` now share one
lookup that compares the group and parameter short names without their padding spaces (the files pad `Glow Filter`; the
former exact match could not find it); `Banks::GlowAt`/`Glow`/`BlendGlow` (the built-in record of 17DBE0 where the lookup
fails, the 2227C0 blend). T2 (`glow_bank_test` on the I-0067 `default.gparam.dcx`, the only binary gparam kept in the
evidence store; mapbase exists only as a TSV): default id 10 (0, 20, 3, 1, 0, 1), key {1, 0} and {-1, -1} give the built-in
record, the raw word, `GroupFloat` unchanged in value, the lerp with the enables from B and the epsilon snap on two records.
**GL-3.** `CollectNativeGlowInputs` (keys +0x4028/+0x4090, weight +0x40D0, epsilon [143D7ECE8], factor
[[GXRS+0x5368]+0x80]): `NativeSelectedScene::glow` and `NativeModelFrameInput::glow` (main view) when mode B and the factor
are on and `no-glow.flag` is absent; row `native_glow_inputs` (on change and every 600th); the programmes are GL-4 (W6).
Rows GL1/GL2 (Reference1517, observer of `FUN_1401BB810`'s record with `shadow-observe.flag`): in game Enable 1, New Glow 1,
BlurSampleScale 1, Rgb 1.5, Alpha 1, Final 0.5, factor 1. Native1518 (same save, key 140000000/0 from mapbase): the same
six values, factor 1, glow on. Next: W6 (GL-4 to GL-8: programmes, stage, forward second target, wiring).

## Implementation log (thread « Source du soleil puis W6 », branch `claude/zen-blackwell-8eae69`, 2026-10-13: W6)

**GL-5.** `native/renderer/glow_stage.*`: `GlowStage` records the ten passes of §1.1 with the four original programmes (pins
`glow_pins`, §2): S and L0..L3 in R11G11B10_FLOAT (never cleared), root CBVs b0-b2 and a one-descriptor t0 table, SS_ClampLinear
as a **static sampler** at s3 (trilinear, clamp, full LOD range; the shaders sample level 0, so the bank row 4 table of §5.3 is
not needed for the image; noted as a simplification), the quad of §1.2, viewport and scissor at each target's size, the
constants of §3.2 in eight 256-byte blocks of the frame's upload arena, and every barrier and pass declared through a
`GlowRecorder` (the renderer routes them through its frame-graph shadow). T4/T5 (`glow_stage_test`, WARP and hardware, the
I-0063 programme cache): the pins hold and a flipped byte fails; the final colour equals a CPU model of the ten passes
(bilinear clamp sampling, the 13 taps at one target texel, R11G11B10 storage after every glow write) **within 2/255** (the
sheet asked 1/255; the residue is the hardware's filtering precision against the float model, on both adapters); zero scales
and a zero GB5 alpha leave the colour unchanged; a resize keeps the chain; no debug-layer message. **GL-4.**
`AcquireNativeGlowPrograms` reads the four programmes from the engine's repositories, all or none, checks each pin (a mismatch
disables the glow for the session, row `native_glow_programmes_refused`; a missing one is retried, row `_unresolved`);
`CollectNativeGlowInputs` gives the frame a glow input only with them. **GL-6.** Forward rows of the main view whose pixel
programme declares `SV_Target1` take `Forward2` (lit HDR and GB5, RT1 with RT0's blend); the renderer accepts both forward
envelopes and binds GB5 for the second; `no-forward-gb5.flag` keeps Forward1; row `native_forward_gb5`. **GL-7.** The
renderer opens the stage from the first frame's programmes at the presentation extent and records it after the composition
and before the UI, into the 8-bit colour target (the gamma intermediate); Resize, Close and the quarantine follow the stage;
the frame graph declares the five targets and the ten passes (structure bits 11 glow, 12 forward GB5); row
`native_glow_frame`; `no-glow.flag` removes it. **GL-8** (development aids `glow-verify`, `glow-factor`) deferred: the
witnesses below did not need them.

Live (Native1550-1557, Thomas's save at Firelink and Shebang Gamma at m37): programmes resolved and pinned; ten passes per
frame (Native1556: 3,868 glow frames, 38,680 passes); about 9 % of the forward rows take Forward2 at Firelink (Native1550:
7,908 of 80,000 prepared rows); 0 debug errors in every run; under the graph shadow with its executor (Native1550/1551) every
glow pass is recorded and no mismatch row appears. The m37 bonfire frame with and without the glow (Native1556/1557,
`game.jpg`) differs only where the scene animates (no systematic brightening): its emissive sources are effects (FFX, not
drawn natively yet) rather than G-buffer Glow materials. The visible witness of §8 (`c2150` at Firelink) waits for the
Firelink decal stop (ROADMAP, next item). Firelink runs end about 40 s in on that known refusal, not on the glow.

**W6 review (fresh Opus, no blocking finding) and status.** W6 is **implemented and structurally witnessed; its image witness
is open**. The runs prove the ten passes execute (sizes 1280x720, 320x180, 160x90, 80x45, 40x23 and the scales blur 1, Rgb
1.5, Final 0.5 in `native_glow_frame`, Native1561), with 0 debug errors and no graph mismatch; they do not yet show the glow's
contribution in an image: the m37 pair (Native1556/1557) differs only where the scene animates, the earlier m40 captures were
title or loading screens (Native1550/1551, 1561), and GB5's alpha was not measured. The sentence above about m37's emissive
sources is an inference, not a measurement. The Forward2 share is of prepared rows (kept draws skip the counter, menu-view rows
excluded since they never take Forward2), not of drawn rows. Review fixes: the GB5 view is keyed on the target generation as
well as the resource; an extent mismatch disables the glow instead of faulting; the failure reason is logged; the test
requires the debug-layer info queue; T6 added (`WritesSecondTarget`: the composite stays Forward1; 74 of the cache's 333 `_Fwd`
pixel programmes declare SV_Target1); T7's row carries the sizes and scales. Open: the image witness (`c2150` at Firelink,
or GB5.w and S readbacks through GL-8's `glow-verify`), GL-8.
