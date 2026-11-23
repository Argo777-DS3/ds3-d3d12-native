# The native colour path and the gamma pass, first and independent of YEBIS (ready to implement, static)

Request (coordinator, 2026-10-11): an implementation-ready note for the native colour path and the gamma pass shipped first, independent of YEBIS: the intermediate target before the back buffer, the reading of the brightness, the order with the UI, the shaders from the original, the commits with their flag, the tests and the witnesses. To be merged after an independent Opus 5.5 review (log in §11). Static; no native code was changed, nothing was launched. It is slice **Y0** of [WAVE1-PREP](WAVE1-PREP.md) §2.6 and commit **W3** of its §4.2; the design is [DISPLAY-SWAPCHAIN-NATIVE](../I-0068-static-yebis-ffx-lights-spec/DISPLAY-SWAPCHAIN-NATIVE.md) §9 turned into code-level steps, with the facts of §1 re-verified against the executable for this note.

Labels: **verified** = read in the executable (`DarkSoulsIII.exe` 1.15.2.0, SHA-256 `EF5E07C5…498C`) or in the decompiled functions for this note; **observed** = read in the native tree at main `0ec5e28d` (line numbers are that tree's and will move); **note** = a statement of a linked note; **proposed** = this note's design; **open** = needs a read or a run. Abbreviations: `SP` = `native/renderer/scene_pass.cpp`, `GB` = `native/frame/game_bootstrap.cpp`.

## 0. Answer first

1. **The original renders the whole frame, game and UI, into an intermediate R8G8B8A8_UNORM target and then draws one full-screen pass into the back buffer** that maps each channel through a 1025-entry curve (`src/dst` exponent, `dst = 2.2 + 0.5·b`, `b = (brightness−5)/5`). The native frame draws straight into the back buffer: the brightness intent is computed every render step (`GB:7345-7358`) and read by nothing. Brightness therefore has no effect natively today.
2. **The pass is a pure 8-bit identity at brightness 5, bit for bit.** With exponent 1 the curve is `i/1024` and the shader samples it at `c·1025 − 0.5` (a half-texel offset the original has too). For every 8-bit input `n` the sampled value lies within 0.1236 LSB of `n` (checked for all 256 values, with the 8-bit filter-weight quantisation of the hardware emulated: 0 mismatches), so the rounded output equals the input. **Shipping the pass changes no pixel at the default brightness**, which makes the pixel hash of Native1254 an exact non-regression witness (§6).
3. **Everything needed is in the process and verified**: the two shaders are DXBC blobs in the loaded image (`.data`): vertex shader RVA `0x46BD950`, 696 bytes, size dword at RVA `0x46BDC08`; pixel shader RVA `0x46BDC10`, 1056 bytes, size dword at RVA `0x46BE030`; the DXBC header sizes equal the size dwords; SHA-256 `ECAC101E…923D` and `429D7400…68F8`. The engine's own curve generator `FUN_1400C83D0` writes the 1025×RGB floats (`0x300C` bytes) and can be **called from the native module**, so the curve is bit-identical to the original's without reimplementing `powf`.
4. **The design is one new component and a small wiring**: `DisplayStage` (`renderer/display_stage.h/.cpp`) owns the intermediate target (swap-chain size and format), its RTV and a two-descriptor SRV heap, the curve texture, the staging ring and the root/PSO; `SceneRenderer` routes the composition and the UI into `DisplayStage::Target()` instead of the back buffer and records the pass after the UI. A legacy `FinalDisplay` pass already builds the same root and PSO for the offline path (`SP:464-483`); its builder is extracted and shared. **The legacy resources of the offline path cannot be reused for the native path**: the native `bindings_` heap has `composition_binding+2 = 90` entries while `display_binding` is 95 (`SP:24-27`, `SP:813`), and the native RTV heap has 6 targets.
5. **Five reversible commits** (§4): G0-1 the pure curve helpers and their tests; G0-2 the blob pins and loader; G0-3 `DisplayStage` and the shared pipeline builder, not wired; G0-4 the wiring behind `no-gamma.flag`; G0-5 development aids (`brightness-override.flag`, `gamma-verify.flag`, a `-Brightness` pilot switch). Only G0-4 changes behaviour, and only for brightness ≠ 5.
6. **Order with the UI is the original's**: composition, then (later) the YEBIS chain, the anti-aliasing and the glow, then the UI into the intermediate, then the gamma pass, then the readback witness and `Present`. The menu offscreen views and the movie targets never see the pass (they are textures the UI samples).

## 1. What the original does (verified)

| Fact | Value | Source |
|---|---|---|
| swap chain | R8G8B8A8_UNORM, "gamma pass on" bytes `+0x38/+0x39` = `00/01` | `FUN_140006C00` (note §9.1) |
| creation of the pass | `FUN_141933200`: `FUN_141930420(0x144930570, dev, 0x1446BD950, size@0x1446BDC08)` (vertex) and `FUN_1419308F0(&0x144930590, dev, 0x1446BDC10, size@0x1446BE030)` (pixel) | decompiled `141933200` lines 31-33 |
| vertex shader blob | VA `0x1446BD950` = RVA `0x46BD950` (file `0x46BB950`, `.data`), 696 bytes, DXBC, header total size 696, size dword `0x2B8` at RVA `0x46BDC08`, SHA-256 `ECAC101EB24BB344101632D54B7756AEF943CD7C2A9ED435B56A7E788E1B923D` | measured on the executable |
| pixel shader blob | RVA `0x46BDC10` (file `0x46BBC10`), 1056 bytes, DXBC, header total size 1056, size dword `0x420` at RVA `0x46BE030`, SHA-256 `429D7400C0B35DAB096D18FED9ADFFA5D7D752FC4FB76526A4EA3C2AC42F68F8` | measured on the executable |
| shader semantics | VS `vs_5_0`, `SV_VertexID` only, a 4-vertex strip; PS `ps_5_0`: `t0 g_src` (Texture2D), `t1 g_rgbTable` (Texture1DArray), `s0`; `c = g_src.Load(floor(SV_Position.xy))`, `out = (table.SampleLevel(s0, (c.r,0)), (c.g,1), (c.b,2), c.a)`; compiler 9.30.9200.20546 | disassembly `evidence/I-0068/queries/display/gamma/gamma-{vs,ps}.asm`, note §9.2 |
| draw state | triangle strip, `Draw(4,0)`, no input layout, blend off (write mask all), depth and stencil off, rasterizer solid / cull none / depth clip off / scissor off, one RT = back buffer, viewport and scissor full; sampler `s0` linear, CLAMP×3, MinLOD −FLT_MAX, MaxLOD FLT_MAX | `FUN_141933480` (note §9.3) |
| curve generator | `FUN_1400C83D0(out, srcR, srcG, srcB, dstR, dstG, dstB)`: exponents `src/dst` per channel; for `i` in 0..1024: `x = i · [143D7ED0C]` (**verified = 0.0009765625 = 1/1024**), `v = powf(x, exponent)` through the CRT call `141778090` (body `141FD3430`), compared with `[143D7EE10]` (**verified = 1.0**) and replaced by the literal 1.0 above it, by 0 below zero; interleaved RGB floats from `out` offset 0, 12 bytes per entry, `0x300C` bytes total (the callback `FUN_1400C8590` forwards its six float fields `+8..+0x1C`) | decompiled `1400C83D0`, `1400C8590` |
| curve inputs | GX `+0x3F0..+0x404`: `src` R,G,B = 2.2 (verified constant `[143D7EE90]` = 2.2), `dst` R,G,B; per frame `dst = 2.2 + k·b`, `k = [obj+0x98] = 0.5`, `b = [obj+0x94]` then `[obj+0x94] = 0`; `b = clamp((v−5)/5, −1, 1)` from the option byte `v` (0..10, default 5) in-game, 0 elsewhere | note §9.4 (`FUN_140DFDD20`, `FUN_14062C010`) |
| exponents | brightness 5: 1.0; 10: 2.2/2.7 = 0.8148 (brighter); 0: 2.2/1.7 = 1.2941 (darker) | arithmetic |
| upload | `FUN_14192CBD0` de-interleaves into three slices with `UpdateSubresource` (4100 bytes each) into a `Texture1DArray` 1025 × 3 `R32_FLOAT` | note §9.4 |
| order | the game and the UI render into the intermediate (`+0x28`), the pass writes the back buffer (`+0x30`), then `Present` | note §9.1, §9.5 |

## 2. What native does today (observed)

| Region | Lines | What |
|---|---|---|
| main frame | `SP:1545-1670` | G-buffer, lighting, forward (HDR), velocity, then the composition (`:1613-1625`): lit HDR → sRGB encode → **back buffer** (`:1614` index, `:1618` `PRESENT→RENDER_TARGET`, `:1619` target = `presenter_.rtv`), UI via `primitives_.Encode(list,target,stencil,true)` (`:1629`), readback every 120th frame (`:1633-1645`, buffer `RENDER_TARGET→COPY_SOURCE→PRESENT`), `Present(0,0)` (`:1664`) |
| startup frames | `SP:1672-1696` | `RenderStartupFrame`: back buffer `PRESENT→RENDER_TARGET` (`:1681`), `ClearRenderTargetView` black `(0,0,0,1)`, UI, `RENDER_TARGET→PRESENT`, `Present` (`:1694`): logos, title, loading |
| offscreen views | `SP:1354-1431` | menu views: own target, own composition twin (`OffscreenCompositionPipeline`, `:383-405`), never presented |
| movie | `SP:1432` | `RenderNativeMovie` writes a target the UI samples |
| offline path | `SP:1858-2114` | `RenderFrame` already does composition → `display_source_` → UI → `PassKind::FinalDisplay` → back buffer (`:2024-2050`) with `FinalDisplay(assets)` (`:464-483`: root = one SRV table t0..t1, one static sampler, `ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT`; PSO from the supplied DXBC, `DepthClipEnable=FALSE`, `NumRenderTargets=1`, RTV R8G8B8A8_UNORM), fed by captured bytes (`SceneDisplayAssets`, `scene_pass.h:74`); its LUT upload is `SP:615-636`; its intermediate is `Targets()` `:676-681` (RTV slot 9, SRV `display_binding`) |
| presenter | `backend.cpp:402-431` | swap chain R8G8B8A8_UNORM, `ResizeBuffers`; `SceneRenderer::Resize` (`SP:2119-2123`) drains and rebuilds the targets; `Close` (`SP:2125-2158`, legacy display members reset at `:2145`) resets them |
| brightness | `GB:7345-7358` (in `NativeRenderStep`, hook `{0xD55190}` `:9973`) | reads `[image+0x4796298]+0x94` (b), `+0x98` (k) and `[image+0x3D7EE90]` (2.2); `brightness = b·k + 2.2`; `native_colour_settings = {2.2,2.2,2.2,brightness×3}` under `colour_settings_lock`, `++native_colour_generation`; zeroes `+0x94` as `DFDD20` does; **no reader** (the row at `GB:7359` says `colour_pass_implemented:false`; the globals are declared at `GB:323-325`) |
| render driver | `GB:644-715` (`Clear()`, `++serial` at `:649`) | `++serial`, `PrepareNativeModelFrame`, `RenderNativeMovieFrame`, `RenderNativeOffscreenViews`, then `renderer.RenderNativeModels` / `RenderStartupPrimitives` / `RenderStartup`; same owner thread as `NativeRenderStep` (R8B-ENGINE-READS-STATIC §1) |
| tests | none | no unit test of a display pass; `renderer/gbuffer_encoder_test.cpp` is not built by any script (MODEL-DRAW-ENVELOPE-READY §1) |

The UI pipeline's target format is R8G8B8A8_UNORM (`primitive_pass.cpp:227`), equal to the intermediate's.

## 3. Design

### 3.1 `DisplayStage` (new `renderer/display_stage.h/.cpp`)

```
struct DisplayBlobs { std::vector<uint8_t> vertex, pixel; };            // validated DXBC copies (G0-2)
// Produces the interleaved curve (0x300C bytes) for {src RGB, dst RGB}. In the game it calls the engine generator
// (image+0xC83D0); in tests it is the pure reimplementation of display_curve.h. DisplayStage never touches the image.
using CurveGenerator = std::function<bool(const std::array<float,6>&, std::span<uint8_t,0x300C>)>;
class DisplayStage {
public:
    bool Open(ID3D12Device*, const DisplayBlobs&, CurveGenerator, uint32_t width, uint32_t height);  // root, PSO, heaps, curve texture, intermediate; marks the identity curve {2.2 x6} dirty; false = disabled (reason logged)
    void Resize(uint32_t width, uint32_t height);                       // after the caller's Drain; recreates the intermediate and its views
    void SetColour(const std::array<float,6>& values);                  // src RGB, dst RGB; calls the generator and marks the curve dirty only when the values differ from the last generated
    D3D12_CPU_DESCRIPTOR_HANDLE Target() const;                         // RTV of the intermediate (R8G8B8A8_UNORM, optimized clear {0,0,0,1})
    ID3D12Resource* Intermediate() const;
    void RecordUploadIfDirty(ID3D12GraphicsCommandList*);               // CopyTextureRegion per slice from a fresh staging buffer (kept by the stage until AfterSubmit)
    void RecordPass(ID3D12GraphicsCommandList*, ID3D12Resource* back_buffer, D3D12_CPU_DESCRIPTOR_HANDLE back_rtv, D3D12_VIEWPORT, D3D12_RECT);
    void AfterSubmit();                                                 // called right after gpu_.Execute(list): hands the staging to RetireWhenSubmittedWorkCompletes
    void Close();
};
```

**Interface from the game module to the renderer** (the review's finding 1): `DisplayStage` lives in `renderer/`, which is also linked into `startup_primitive_test` and `game_texture_ingress_test` without a game image, so it takes the blobs and a **generator callback** instead of reading the image. `SceneRenderer` gains `SetDisplaySource(DisplayBlobs, CurveGenerator)` (called once by `GB` after the blobs are admitted and before the first frame; it stores them) and `SetDisplayColour(const std::array<float,6>&)`; the stage is opened lazily by `display_stage_.Ready(width, height)` (calls `Open` the first time, `Resize` when the extent changed), which **both** `RenderStartupFrame` and `RenderNativeModels` call at their start. Without `SetDisplaySource` (tests, flag `no-gamma.flag`, failed admission) the stage stays closed and every path draws directly into the back buffer as today.

Resources and states:

| Resource | Format, size | Heap | Created | Steady state |
|---|---|---|---|---|
| intermediate | `R8G8B8A8_UNORM`, swap-chain size, `ALLOW_RENDER_TARGET`, optimized clear `{0,0,0,1}` (matches `RenderStartupFrame`'s clear; **a debug-layer warning is fatal here**: `SceneRenderer::Diagnostics` counts every message of severity WARNING or worse as an error, `SP:782-786`, and both frame paths then throw and set `faulted_`, so a clear-value mismatch would fault the renderer) | default | `Open`, `Resize` | `RENDER_TARGET` between frames |
| intermediate RTV | one slot in a private RTV heap (the shared `rtvs_` and `native_light_rtvs_` are full or sized for other users) | | | |
| SRV heap | shader-visible, **2 contiguous descriptors**: `t0` intermediate (Texture2D, R8G8B8A8_UNORM), `t1` curve (Texture1DArray, R32_FLOAT, 3 slices) | | `Open`; `t0` rewritten on `Resize` | |
| curve texture | `Texture1DArray` 1025 × 3, `R32_FLOAT` (`D3D12_FORMAT_SUPPORT1_SHADER_SAMPLE` asserted at `Open`: linear filtering of R32_FLOAT is required by the shader's sampler) | default | `Open` | `PIXEL_SHADER_RESOURCE` |
| staging | upload buffer sized by `GetCopyableFootprints` for the 1025×3 `R32_FLOAT` array (row pitch 4100 aligned to 4352; placed footprints are 512-aligned, so the three slices sit at offsets 0 / 4608 / 9216 and the total is about 13,316 bytes: **derive every number from the call, never hard-code them**) | upload | per upload (rare: on a brightness change) | held by the stage from `RecordUploadIfDirty` until `AfterSubmit()`, which then calls `RetireWhenSubmittedWorkCompletes` (phase B2b, `backend.h:38`). That function keys the retirement on `LastSubmitted()` **at the moment of the call** (`backend.cpp:671-673`): called while still recording it would tie the buffer to the previous submission and could free it before the copy executes, so it is called after `gpu_.Execute(list)` |

Memory: the intermediate is `w·h·4` bytes (3.5 MB at 1280×720, 8.3 MB at 1920×1080, 33 MB at 3840×2160); the rest is under 40 KB.

### 3.2 The shaders: runtime read from the image, pinned by hash

`LoadDisplayBlobs()` in `GB` (G0-2) reads `size = *(uint32_t*)(image+0x46BDC08)` and `image+0x46BD950` (vertex), `*(uint32_t*)(image+0x46BE030)` and `image+0x46BDC10` (pixel) with the guarded `Read`; it requires the `DXBC` magic, the size equal to the header total size (`+24`), the sizes equal to 696 and 1056 and the SHA-256 equal to the **two constants hard-coded in the source** (`display_blobs.h`, as `primitive_programs.cpp` hard-codes its `ProgramPin`s: the executable identity is already pinned at `Build-GameBootstrap.ps1:9`, and a pin generated from the same executable would make the check circular). The build step of G0-2 does the other direction: it extracts the bytes at the mapped file offsets from the executable (the section table is read at `Build-GameBootstrap.ps1:12-14`) and fails if they differ from the constants. **A mismatch never calls `Refuse`** (`GB:563-568`: `[[noreturn]]`, ends in `ExitProcess`): it logs `native_display_blob_identity` and leaves the stage disabled, so the frame draws directly into the back buffer, i.e. today's behaviour. The blobs live in `.data` (writable); the hash is computed once at admission, so a later write by the engine would not be seen: acceptable because the engine only reads them (`FUN_141930420/1908F0` copy them into shader objects).

### 3.3 The curve: the engine's generator, a pure function of six floats

Per render step the six values are `{src RGB = 2.2, dst RGB = 2.2 + 0.5·b}` (`GB:7352-7356`; only `dst` varies today). `DisplayStage::SetColour` compares them to the last generated six; **on change only** it calls the `CurveGenerator` given at `SetDisplaySource`. In the game module that callback is:

```
alignas(16) uint8_t scratch[0x300C];
reinterpret_cast<void(__fastcall*)(void*,float,float,float,float,float,float)>(image+0xC83D0)(scratch, sr,sg,sb, dr,dg,db);
```

(the x64 convention puts the pointer in RCX, the first three floats in XMM1..XMM3 and the last three on the stack, exactly as the decompiled signature `(longlong, float×6)`), and the engine entry points it uses are **pinned** like the other engine entry points (16-byte `Pin` entries at `Build-GameBootstrap.ps1:15` for RVA `0xC83D0`, for the function the generator calls (`0x1778090`, which jumps to the CRT `powf` body at `0x1FD3430`; first bytes of the latter measured: `48 81 EC 18 01 00 00 66 0F 7F B4 24 B0 00 00 00`)). The stage then de-interleaves into three 1025-float slices and `RecordUploadIfDirty` records the three `CopyTextureRegion` calls. A pure C++ reimplementation lives in `renderer/display_curve.h/.cpp` for the unit tests and for `gamma-verify.flag` (compare it with the engine's output every N frames and log the maximum absolute difference; any difference above 1e-6 is a defect). Why call the engine: the CRT `powf` of the game and of the native module can differ in the last ulp; calling the original removes the question. The original uploads every in-game frame; uploading on change only is a no-op in effect (the curve is a function of the six inputs, §9.5 of the note) and removes 3×1025 `powf` and an upload per frame.

### 3.4 Reading the brightness and handing it over

`NativeRenderStep` already computes and stores the six values (`GB:7352-7356`). The renderer needs them at render time on the same thread, so `Clear()` (`GB:644`) gains, right after `++serial` (`GB:649`):

```
{std::array<float,6> values; { std::lock_guard g(colour_settings_lock); values = native_colour_settings; }
 renderer.SetDisplayColour(values); }
```

`SceneRenderer::SetDisplayColour` forwards to `DisplayStage::SetColour` (and is a no-op when the stage is disabled). The initial `native_colour_settings` is `{}` (all zero) until the first render step: `SetColour` ignores any value with a non-positive component and keeps the identity `{2.2×6}` (a divide by zero would produce infinities). `native_colour_generation` is not used for gating: the stage compares values, which also covers a render step that did not run.

The brightness sources stay the engine's (`FUN_14062C010` sets `b` in-game from the option; outside the world `b = 0`, so title, menus and loading are identity, as in the original).

### 3.5 Frame order and the exact edits

**Main frame, `SP:1545-1670`**: after `NativeModelTargets()` and the other resource set-up, `display_stage_.Ready(width,height)` (creates or resizes). In the composition block:

| Today | After (stage enabled) |
|---|---|
| `SP:1618` `Transition(buffer, PRESENT, RENDER_TARGET)` | removed here; moved into the gamma pass |
| `SP:1619` `target = CPU(presenter_.rtv, stride, index)` | `target = display_stage_.Target()` |
| composition draw `SP:1620-1625` | unchanged (draws into the intermediate; the viewport and scissor are the same extent) |
| UI `SP:1626-1629`: `primitives_.Encode(list, target, stencil, true)` | unchanged (same `target` variable) |
| (nothing) | **new, after the UI and before the readback test `SP:1633`**: `display_stage_.RecordUploadIfDirty(list)`; then `RecordPass(list, buffer, CPU(presenter_.rtv, stride, index), viewport, scissor)` which does: `Transition(intermediate, RENDER_TARGET→PIXEL_SHADER_RESOURCE)`, `Transition(buffer, PRESENT→RENDER_TARGET)`, `OMSetRenderTargets(1, &back_rtv, FALSE, nullptr)`, heaps, root, table, PSO, viewport, scissor, strip topology, `DrawInstanced(4,1,0,0)`, `Transition(intermediate, PIXEL_SHADER_RESOURCE→RENDER_TARGET)` |

After the pass the back buffer is in `RENDER_TARGET`, which is the state the existing readback and `Present` code at `SP:1633-1640` already expects. The readback witness therefore samples the **final** (post-gamma) image. Right after `gpu_.Execute(list)` the frame calls `display_stage_.AfterSubmit()` (the staging hand-over of §3.1).

**Startup frames, `SP:1672-1699`** (`RenderStartupFrame`): call `display_stage_.Ready(width, height)` at the start; replace the back-buffer transition and clear (`:1681-1684`) by the intermediate's clear (`display_stage_.Target()`, black `(0,0,0,1)`), keep the UI encode with the same `target`, then **`RecordUploadIfDirty(list)`** and `RecordPass` (which brings the buffer from `PRESENT` to `RENDER_TARGET`), keep the existing `RENDER_TARGET→PRESENT` (`:1689`), and call `AfterSubmit()` after `gpu_.Execute(list)` (`:1692`). The upload belongs on this path too: returning to the title or a loading screen sets `b` back to 0 after an in-game brightness ≠ 5 and the curve must follow, and the very first frame uploads the identity curve that `Open` marked dirty. Logos, title and loading thus go through the pass, as in the original (every frame does).

**Not changed**: `RenderNativeOffscreen` (`SP:1354-1431`), `RenderNativeMovie` (`:1432`), the offline `RenderFrame` (`:1858-2114`, which keeps its own display members), `backend.cpp`'s fixture path.

### 3.6 States, barriers, resize, close, faults, flags

- **Barriers per frame**: intermediate `RT→SRV→RT` (two), back buffer `PRESENT→RT` (one, moved), curve texture `SRV→COPY_DEST→SRV` (two, only on change). One extra full-screen draw and no extra queue submission or wait.
- **Single queue, frames in flight**: the intermediate is reused every frame; on one queue frame N+1's composition is ordered after frame N's pass, so there is no hazard. Phase D keeps the same property; the staging ring is already ticket-retired.
- **Resize**: `SceneRenderer::Resize` (`SP:2119-2123`) already drains and rebuilds the model targets only `if(scene_ready_ || native_model_targets_)` (`:2122`); `display_stage_.Resize(width, height)` is **unconditional when the stage is open**, placed right after `presenter_.Resize(gpu_,width,height)` (recreate the intermediate, its RTV and the `t0` SRV; the root, PSO, curve and heaps survive).
- **Close**: `Close()` (`SP:2125-2158`) calls `display_stage_.Close()` after `gpu_.Close()`'s retirement withdrawal, so staging held by the retirement queue is released through its own path. The quarantine path in `~SceneRenderer` (`SP:2159-2189`), which detaches every GPU object it cannot release safely, gets the stage's resources too (a `DisplayStage::Detach()` mirroring the other members).
- **Faults**: any failure in `Open` (blob identity, PSO creation, format support) disables the stage for the session, logs `native_display_stage_disabled` with the reason and leaves today's direct-to-back-buffer behaviour; a failure after `Open` (device removed) follows the renderer's existing `faulted_` handling.
- **Flags** (files next to the module, read once at renderer start like `measure.flag`, `GB:8975-8986`): `no-gamma.flag` (stage never created: today's path), `gamma-verify.flag` (curve cross-check), `brightness-override.flag` (development: a file whose content is an integer 0..10 replaces the option byte: `b = (v−5)/5`, applied in `NativeRenderStep` before `GB:7352`).

### 3.7 Alignment with the YEBIS plan and the other waves

The intermediate is the `Ldr[B]` of YEBIS-D3D12-DESIGN-NATIVE §4.2: the UI, the glow composites and (later) the anti-aliasing output land in `DisplayStage::Target()`; the DLAA input `Ldr[A]` is a second texture owned by the YEBIS stage. The tone-map output of Y3 replaces only the composition's source. The gamma pass and its curve are unchanged by YEBIS. Menu views keep the GX tone-map filter and the composition twin (P23).

## 4. Commits (each builds, passes the unit tests, is reversible alone)

| # | Commit | Behaviour change | Flag |
|---|---|---|---|
| G0-1 | `renderer/display_curve.h/.cpp`: the pure curve (`pow` clamp), the exponent from `{src,dst}`, the de-interleave into slices, the CPU model of the pass (§5 T2); tests T1, T2 (footprint arithmetic is exercised on a device in T4, never hard-coded) | none | |
| G0-2 | `display_blobs.h` (RVAs, sizes, the two SHA-256 constants); `LoadDisplayBlobs()` in `GB` + row `native_display_blobs`; the build step extracts the bytes from the executable and compares them with the constants (T3); engine-entry `Pin`s for `0xC83D0`, `0x1778090`, `0x1FD3430` | none (blobs loaded and logged, unused) | |
| G0-3 | `DisplayStage` (with `CurveGenerator`, `AfterSubmit`, `Detach`) + `BuildDisplayPipeline(device, vs, ps, rtv_format)` extracted from `FinalDisplay` (`SP:464-483`) and used by both (it needs `Blend()`, today in the anonymous namespace of `scene_pass.cpp`, to be exposed; the shared root keeps `ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT` as the legacy builder does, where DISPLAY-SWAPCHAIN §9.5 lists `DENY_*` flags and no IA flag: both are valid and neither changes the result); unit tests T4-T7 on WARP; not wired | none (the offline path builds the same root and PSO) | |
| G0-4 | wiring (§3.5; interface in §3.1): `SetDisplaySource` from `GB`, `SetDisplayColour` in `Clear()`, `Ready`/`RecordUploadIfDirty`/`RecordPass`/`AfterSubmit` in both frame paths, Resize/Close/Detach, counters `display_pass_gpu`/`display_uploads` in the row `native_display_stage` | brightness works; identity otherwise | `no-gamma.flag` |
| G0-5 | `brightness-override.flag`, `gamma-verify.flag` (curve cross-check **and** the same-frame tap of §5.2), `-Brightness N` for `Run-PilotPair.ps1`, `Compare-GammaDump.ps1`; a sampled readback in `RenderStartupFrame` (it has none today) | none by default | the two dev flags |

Ordering with the rest: independent of YEBIS, effects, glow and shadows; **independent of phase C** except that it adds a PSO and a root signature (outside the model-draw caches: no interaction with R7/R9) and uses the B2b retirement hook (merged). It touches `SP:1545-1696`, `SP:2119-2147`, `GB:644-660`, `GB:7345-7358` and the build script; no overlap with `model_draw.*` (W0a) or with workstream K's regions.

## 5. Tests

### 5.1 Unit (built from `Build-GameBootstrap.ps1`, WARP then hardware)

| Case | What | Pass |
|---|---|---|
| T1 curve | the reimplementation against a double-precision `pow` for exponents `{1, 2.2/2.7, 2.2/1.7}` over all 1025 entries; exponent 1 gives exactly `i/1024`; clamp to `[0,1]` | within 1 ulp of float; exact at exponent 1 |
| T2 identity and model | the CPU model of the pass (`Load` of an 8-bit value, linear sample of the curve at `c·1025−0.5` with 8-bit filter-weight quantisation, round to 8 bits): with the identity curve the output equals the input for all 256 values and the maximum deviation before rounding is under 0.13 LSB; for the two non-identity exponents the output equals the double-precision expectation within 1 LSB | exact / ≤ 1 |
| T3 pin | the build extracts the blob bytes at the PE-mapped file offsets from the executable and fails if their length, DXBC magic, header total size or SHA-256 differ from the **hard-coded** constants of `display_blobs.h` (the check is not circular: the constants are not derived from the executable); `InspectShaderInterface` on the pixel blob reports `t0`, `t1`, `s0` and no constant buffer, on the vertex blob no resources | exact |
| T4 mechanics | `DisplayStage` on WARP with the **original blobs** (path passed to the test like `game_primitive_programs_test` gets `$exe`): a 16×16 ramp written into the intermediate, one pass into a stand-in back buffer, readback equals the T2 model for identity and the two exponents (±1 LSB for the GPU filter) | per pixel |
| T5 resize | `Resize` to another extent, the same pass again; the root, PSO and curve are reused (object identity) | equal |
| T6 upload gating | `SetColour` twice with equal values records one upload, a changed value one more; a non-positive component is ignored | counters exact |
| T7 states | the sequence clear → pass → present-state under the debug layer on WARP | 0 errors, 0 warnings |
| T8 fallback | with the stage disabled the renderer paths do not create it (counter) and the composition targets the back buffer | exact |

### 5.2 Live witnesses

| Witness | Action | Expected |
|---|---|---|
| **L1 identity (same frame)** | Native1254 (High Wall idle, `Pilot-NativeWitness.ps1`) with `gamma-verify.flag`, brightness 5 (the witness row logs the option byte and `native_colour_settings`): the **tap** copies the intermediate and the back buffer of the same frame (two readbacks every N frames, `intermediate.rgba` / `backbuffer.rgba`) | **identical pixel for pixel in every sampled frame** (bit-exact by §0 item 2; no cross-run dynamic content involved); debug layer 0 errors |
| L1b identity (cross-run) | `changed_pixels`/`pixel_hash` of the same view before and after G0-4 | soft check only: frames differ across runs by animation and particles, so equal hashes are not required; the L1 tap is the proof |
| L2 brightness (same frame) | the tap at `-Brightness 0` and `10` (the override flag): `Compare-GammaDump.ps1` applies the §3.3/T2 model to `intermediate.rgba` and compares with `backbuffer.rgba` of the **same frame** | ≤ 1 LSB per channel |
| L3 title and loading | the title screen and a load-in frame at brightness 5, through the sampled readback G0-5 adds to `RenderStartupFrame` (it has none today, `SP:1672-1699`) with the same tap | intermediate equals back buffer pixel for pixel |
| L4 curve | `gamma-verify.flag` over 600 frames, brightness 0, 5, 10 | maximum difference between the engine generator and the reimplementation ≤ 1e-6 |
| L5 reference | the Options brightness page (UI only, no YEBIS) in the Reference (D3D11) and in Native at brightness 0, 5, 10 | the same panel luminance trend and ratios within the UI differences already known; live row YB9 / H3 of YEBIS-HAL-READY (back-buffer change by the curve, identity at 5) |
| L6 menus | Status screen and portraits, character creation | unchanged at brightness 5; offscreen views never receive the pass |
| L7 non-regression | Native1191 velocity, Native1262 quit, the NPC/cloth witnesses | as before |

### 5.3 Measures

Before and after G0-4 on the phase A settings (S1, `-Measure -NoDebugLayer`): `render_ms` and `gpu_wait_ms` (expected + under 0.1 ms: one full-screen pass of two samples per pixel at 1280×720, an estimate), `prepare_ms` unchanged, local video memory +3.5 MB at 1280×720, PSOs +1, descriptors +2, `display_uploads` per window (0 at constant brightness after the first, 1 per change). GPU time of the pass from a timestamp pair in the stage (`display_pass_gpu`). A regression above 0.3 ms GPU or any `prepare_ms` change holds G0-4 at its flag.

## 6. Risks

1. **The blobs live in `.data`, read from the loaded image**: the pin protects against a different executable, not against a runtime overwrite (none is expected). If the image copy proved unreliable, the build can embed the 1752 bytes in the module instead (same pin).
2. **Calling `FUN_1400C83D0` from native code** relies on its signature `(void*, float×6)` and the CRT thunk `141FD3430` being usable outside the game thread context; it is pure arithmetic over its arguments. `gamma-verify.flag` (L4) is the guard, and the reimplementation is the fallback if a call ever misbehaves.
3. **UI stencil and the intermediate**: the UI encode uses `primitive_dsv_` / `primitive_stencil_initialized_` independent of the colour target (`SP:1627-1629`, `:1685-1688`); only the colour RTV changes, so the stencil handling moves with no edit.
4. **Optimized clear value**: the intermediate's `{0,0,0,1}` matches the startup clear; the composition overwrites every pixel in the world frame. A different clear value would raise a debug-layer warning, which this renderer treats as an error (§3.1).
5. **Present timing and the readback**: unchanged order; the readback samples the post-gamma buffer. If a witness needs the pre-gamma image, `pixel_file` could read the intermediate instead (not planned).
6. **8-bit intermediate**: values the YEBIS/AA stages produce later are still 8-bit LDR (the original's format); no precision is lost relative to the original.
7. **A second intermediate for `Ldr[A]` later** (YEBIS) is a separate texture; the stage exposes `Intermediate()` so the glow and the AA can read it.

## 7. Open points

Whether the original's `[obj+0x98]` is exactly 0.5 in the loaded constructor (the note says so; L2/L5 confirm by effect); the exact `b` mapping when `v` is outside 0..10 (clamped to ±1 by `FUN_14062C010` per the note); whether any engine path re-reads the intermediate (none known).

## 8. Review log

Independent review by an Opus 5.5 reader (given the note, the code and the executable; not my conclusions): **no blocker, verdict "ready after fixes"**. It re-measured the blobs (extents, sizes, header sizes, SHA-256), the three constants, the creation call's arguments (blob address in R8, the size read from the dword in R9), the generator's argument order and output layout (`0x300C` bytes), the de-interleave upload, the channel-to-slice swizzle, the identity mathematics (maximum deviation 0.12355 LSB with weight quantisation, 0 mismatches out of 256), the exponents, the heap-size claim (`bindings_` 90 entries against `display_binding` 95), the RTV heap and the UI target format: all hold. Disposition of the findings:

| # | Finding | Disposition |
|---|---|---|
| 1 | no interface from the game module to a renderer that also links into tests without the image; `Ready` only in the main frame | `CurveGenerator` callback and `SetDisplaySource`/`SetDisplayColour`; `Ready` in both frame paths (§3.1) |
| 2 | no curve upload on the startup path; initial identity upload unspecified | `RecordUploadIfDirty` in `RenderStartupFrame`; `Open` marks the identity curve dirty (§3.1, §3.5) |
| 3 | "refuses" would call `Refuse`, which exits the process | log and disable, never `Refuse` (§3.2) |
| 4 | `RetireWhenSubmittedWorkCompletes` keys on the last submission at call time | `AfterSubmit()` after `Execute` (§3.1) |
| 5 | debug warnings are fatal in this renderer | stated (§3.1, §6) |
| 6 | staging size wrong (placement alignment 512) | derived from `GetCopyableFootprints`, offsets 0/4608/9216, about 13,316 bytes (§3.1) |
| 7 | cross-run comparison cannot be exact | same-frame tap of the intermediate and the back buffer (§5.2 L1, L2) |
| 8 | startup frames have no readback | a sampled readback added in G0-5 (§4, §5.2 L3) |
| 9 | the quarantine path in `~SceneRenderer` | `DisplayStage::Detach()` (§3.6) |
| 10 | resize placement | unconditional when open, after `presenter_.Resize` (§3.6) |
| 11 | the pin test was circular | hard-coded constants, the build compares (§3.2, T3) |
| 12 | engine code not pinned | `Pin`s for `0xC83D0`, `0x1778090`, `0x1FD3430` (§3.3, G0-2) |
| 13 | root-flag divergence and `Blend()` in an anonymous namespace | noted (G0-3) |
| 14 | stale line numbers | corrected (`Clear()` `GB:644`, `++serial` `:649`, bindings heap `SP:813`, presenter `backend.cpp:402-431`, startup `RT→PRESENT` `SP:1689`, row `GB:7359`, brightness `GB:7345-7358`, `Close` `SP:2125-2158`) |
| 15 | clamp wording | corrected (§1) |
| 16 | the witness assumed the option byte was 5 | the witness row logs the option byte and `native_colour_settings` (L1) |

## Exports

None. Evidence (outside Git, unchanged): `evidence/I-0068/queries/display/gamma/gamma-{vs,ps}.asm`; new for this note: `evidence/I-0068/queries/gamma-ready/f1/` (decompiled `141933200`, `141933480`, `14192CBD0`, `1400C83D0`, `1400C8590`). The blob extents, sizes, hashes and the three constants were measured on the executable with the build script's own PE section mapping.

## Implementation log (pilot, 2026-10-12)

**G0-1 to G0-4 done** (`renderer/display_curve.*`, `renderer/display_stage.*`, `frame/display_blobs.h`, the wiring in `scene_pass.cpp` and `game_bootstrap.cpp`). Deviations from section 3, on purpose: the curve is made on the engine thread (the owner calls `FUN_1400C83D0`; the pure curve checks it and replaces it when an entry is not finite, outside [0, 1] or more than 1e-6 away) and travels by value in the frame job (`NativeFrameJob::display_curve`), so the renderer, which records on the render worker, never calls engine code; the staging buffer of an upload is held by the frame slot (`slot.kept`) instead of the retirement hook; the world frame declares the intermediate (persistent, home RENDER_TARGET) and the curve (persistent, three slices, shader-readable: its raw upload barriers bring it back before the pass) and a `display` pass (PostProcess) in the frame graph, shadowed and then driven by the executor like every other pass (plan key bit 10); the startup frames take the pass with hand-written barriers (they were not shadowed before); `no-gamma.flag` and `brightness-override.flag` are placed by the runners (`-DisplayFlags no-gamma | brightness=N`); `gamma-verify.flag` and the same-frame tap of 5.2 L1 are not built (the engine curve is checked against the pure one at each change, row `native_display_curve`, and T4 proves the identity pass on the original shaders); rows `native_display_blob`, `native_display_source`, `native_display_stage` (open, passes, uploads). Tests: T1, T2 (`display_curve_test`), T3 in the build script, T4-T7 (`display_stage_test`, WARP and hardware, the original shaders read from the executable).

**Witnesses.** Native1461 (High Wall protocol, debug layer, both shadows): 0 debug errors, 0 graph mismatches, 5,157 of 5,162 recordings driven, the curve the identity (brightness term 0 in this protocol), engine and pure curves equal; Native1462 (charmake; input refused after a focus change by Thomas, the run timed out on the movie): 0 debug errors, 0 mismatches over 59,602 recordings; Native1463/1464 (`brightness=0` and `=10`): the picture darker and brighter than Native1461, 0 debug errors, 0 mismatches. After the review: Native1465 (S1 measure, no debug layer): 60.0 frames/s, record 2.55 ms (2.41 before), slowest frame per 2-s window 20.2 ms median as before (21.2 in Native1451); Native1466 (GPU-based validation): 0 errors, 1,382 of 1,387 recordings driven with the pass; Native1467 (Bink and character creation): 0 debug errors, 0 mismatches, the known close fault `0xD12759`.

**Opus review: no blocker.** Applied: the engine curve validated with a pure fallback, no curve work without the source, the stage rows, the curve's three slices, the extent check, the runners' validation of `-DisplayFlags`. Noted: a brightness that alternates frame to frame would regenerate and upload each frame (as the original uploads every frame; a two-entry cache if it is ever seen); the override also applies outside the world (a development flag).
