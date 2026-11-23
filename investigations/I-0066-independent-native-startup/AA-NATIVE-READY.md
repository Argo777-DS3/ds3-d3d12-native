# Anti-aliasing (GXAntiAliasingFilter, DLAA) native pass: ready to code

**Removed from the native module on 2026-11-14** (Thomas, D-0278; I-0138 RESULT « Tranche 5c »): the pass, its constants
(`native/content/aa_constants.*`) and its stage no longer exist. This note stays as the record of what the original's filter
does and of how the native pass was made (Git history holds its code).

Scope: `GXBS::GXAntiAliasingFilter` (vtable `143D5D408`, slot 0 `FUN_1401B9790`, export `decompiled/DS3-1152-F-001B9790`) as the
original runs it at the default gparam (AntiAliasType 4 = filter mode 3 = `DLAA.ppo`), and the native pass that reproduces it.
Labels: STATIC (decompilation, DXBC, `.rdata`), OBSERVED (a capture), INFERRED. Constants are produced by
`native/content/aa_constants.{h,cpp}` (test `aa_constants_test.cpp`: DLAA RDEF checked, 1280x720 printed). Nothing here was run live.

## 1. What the pass is

One filter run at the end of the world view's post chain, on the **final 8-bit LDR target** after YEBIS (tone map and grading), before
the glow and the UI. In one pass it (a) copies the LDR target into a scratch target with `PointSampleColor1` (a point-sampled 1:1
copy: the pass is **not** in-place, DLAA reads the copy and writes back over the target) and (b) draws a 4-vertex strip with `FXAA.vpo`
and the mode's pixel programme, input = the copy, output = the LDR target, no blend (OBSERVED I-0054: `PointSampleColor1`, `DLAA`,
`EmissiveToGlowPS`..., in that order; STATIC: the copy is a call inside `1B9790`, `FUN_14002C320`, so with the pass off there is no
`PointSampleColor1` either, INFERRED for any other user of that programme).

## 2. When `1B9790` runs

Enqueue (STATIC, `FUN_1400FB890` lines 561-589, before the glow's enqueue): needs the view byte `view+0x3EBA` (per-view AA enable,
kept by `885F0` under view flag `view+0x50` bit 4; INFERRED set for the world view, DLAA is drawn in every captured world frame) and
**not** the library AA (`view+0x50` bit 10 set and record word 1 = 0, i.e. AntiAliasType 0, which would instead set the YEBIS flag
`144962190`; never the case in the shipped files). Then, if `filter+8` (`IgnoreParamFromOut`, debug) is 0, the gparam record is copied
into the filter, `filter+9 = word0 != 0` (Enable), **`filter+0xC = word1 - 1`** (mode 0..3), and the floats below. The command
(`FUN_1401B9670`: `{filter, mode, settings*}` with `settings*` = 16 bytes + the target, allocated per frame) executes later in the
render command list.

The gparam record is the `Anti-Alias` group, bank `root+0x4638`, keys A `view+0x4010` / B `view+0x4078`, weight `view+0x40C4`, lookup
`1723C0`, blend `140220B50` (STATIC; native: `BlendAntiAlias`, `native/world/yebis_blocks.cpp`, already evaluated into `aa[9]` at
`CollectNativeYebisInputs`, `game_bootstrap.cpp` YB-4b). Blend: |w| <= eps (`[143D7ECE8]` = 1.19e-7) takes A; otherwise words 0, 1 come
from B; |w-1| <= eps takes B for the rest; else `(b-a)*w + a` on words 2-8. No zone has the group, so the default file's id 0 is used
everywhere (OBSERVED, YEBIS-CHAIN-CHECKLIST P13). Record words in file order and their destination (STATIC, FB890 lines 570-577):

| word | gparam entry | default | filter field |
|---|---|---|---|
| 0 | Enable | 1 | `+9` |
| 1 | AntiAliasType | 4 | `+0xC` = type - 1 = 3 |
| 2..5 | FxaaAlpha, FxaaReduceMin, FxaaReduceMul, FxaaSpanMax | 1, 1/128, 0.125, 8 | `+0x10, +0x14, +0x18, +0x1C` |
| 6, 7, 8 | **DlaaThreshold, DlaaRamda, DlaaEpsilon** | **0.1, 1.4, 0.12** (`3DCCCCCD`, `3FB33333`, `3DF5C28F`) | `+0x28, +0x2C, +0x30` |

Note: YEBIS-CHAIN-CHECKLIST (the gparam table, line 112) lists the Dlaa entries before the Fxaa ones; the file order (`param_drawparam_default.tsv`) and
FB890 agree with the table above, and `yebis_blocks_test` pins `aa[8] = 3DF5C28F`.

Execution gates, at the top of `1B9790` (STATIC): `filter+9` Enable; then, unless `GXRS` (`[144961B50]`) or the quality object
`[GXRS+0x5360]` is null (then the pass runs with factor 1): the **quality byte `[[GXRS+0x5360]+8]` != 0** (the Antialiasing option:
file `ON` = block level 3, byte 1 at HIGH and MAX, table `0,0,0,1,1`; OFF gives 0, GRAPHICS-OPTIONS-SHEET OP-6) and the **factor
`[[GXRS+0x5368]+0x98]` > 0** (read unguarded). `AaRuns()` in `aa_constants.cpp` is this gate. Same gate forces the YEBIS flag
`144962190` to 0 (FB890 lines 396-410). The factor also fades the filter (section 4).

## 3. Programme choice

`mode = param[0]` (must be < 4), programme at `filter+0x68 + 8*mode` (loaded by `FUN_1401B93C0` from the name table at `14473E388`
in the programme table `[GXRS+0x4DD8]+0xB60`), vertex shader `filter+0x60` = **`FXAA.vpo`** (`143D5D270`) for every mode:

| mode | AntiAliasType | pixel programme | bytes | reads |
|---|---|---|---|---|
| 0 | 1 | `FXAA.ppo` | 2952 | b0 only (`g_vScreenSize.zw`, `g_vFilterParam`) |
| 1 | 2 | `FXAA3.ppo` | 3040 | b0 (`g_vScreenSizeHalf`) |
| 2 | 3 | `FXAA3HQ.ppo` | 5016 | b0 (`g_vScreenSize.zw`) |
| 3 | **4 (default file)** | **`DLAA.ppo`** | **6132, SHA-256 `51F9ED5385294EE57343E15C01A4F0F25E3C757BFDD42594461715A22B5E6326`** (md5 `461779CF...`) | b0 (`g_vFilterParam`), b1 |

`FXAA.vpo`: 1112 bytes, SHA-256 `80F7B4FA89003EC0E7A279B1830DB1D75787645AB1AB0169B76142425ED7F989` (the I-0063 cache copy; `vs_5_0`,
inputs `POSITION float4`, `TEXCOORD float2`; outputs position, `TEXCOORD0` = uv and `TEXCOORD0.zw`, `TEXCOORD1` offsets from `cb0[0].zw`
that DLAA does not read). DLAA's RDEF (extracted from `data1/shader/gxposteffect.shaderbnd.dcx`, checked by the test): sampler
`SS_ClampLinear` s3, texture `SourceTex` t0, `CBFxaaParam` cb0, `CBDlaaParam` cb1.

Programme route, native: the glow stage's (`AcquireNativeGlowPrograms`, `game_bootstrap.cpp` ~5099) `FindLoadedProgramme(0x4792BF8,
L"DLAA", 0u)` (pixel repository) and `FindLoadedProgramme(0x4792D58, L"FXAA", 1u)` (vertex repository), each checked against the pins
above; a miss is retried, a pin mismatch disables the pass softly. The DXBC were disassembled with `fxc -dumpbin` from the I-0063 cache
(`C:/Users/User/.codex/I0063-cache/programs/gxposteffect__DLAA.ppo`, hash equal to the pin).

## 4. Constant buffers (STATIC; the oracle `light_producers.cpp` `FxaaParam`/`CBDlaaParam` reads the same layout)

Both are default-usage buffers created in `FUN_1401B93C0` (`filter+0x40` 0x30 bytes, `filter+0x48` 0xA0 bytes) and updated per draw
(`FUN_1421397C0` = `UpdateSubresource`). `CBFxaaParam` is bound to **VS b0 and PS b0** (`1400201B0`, `140020250`); `CBDlaaParam` to
**PS b1** (mode 3 only). `W, H` are the output target's size (`RT+0x50`, `RT+0x54`, uint converted to float).

`CBFxaaParam` (48 B, three float4), `s = clamp(1 - factor, 0, 1)` (NaN gives 0):

| index | value | mode 3 | modes 0-2 |
|---|---|---|---|
| [0..3] | `W, H, 1/W, 1/H` | same | same |
| [4] | | `(1 - thr) * s + thr` (thr = DlaaThreshold) | ReduceMin |
| [5] | | DlaaRamda | ReduceMul |
| [6] | | DlaaEpsilon | SpanMax |
| [7] | | `1.0` (unread) | `(0 - alpha) * s + alpha` (FxaaAlpha) |
| [8..11] | `2/W, 2/H, 0.5/W, 0.5/H` | same | same |

At the default gparam and factor 1: [4..7] = `0.1, 1.4, 0.12, 1`. The PS uses `cb0[1].x` as the luma floor under which the
pixel is copied unchanged, `cb0[1].y` and `.z` as the edge gain and bias (`gain * edgeLuma - bias`). As the factor falls to 0 the floor
rises to 1 (everything below luma 1 is left alone), then the pass stops at factor <= 0.

`CBDlaaParam` (160 B): ten float4 `{dx, 0, 0, dy}`, the PS adds `.xy` and `.zw` to the uv, so each entry is one horizontal tap
`(dx, 0)` and one vertical tap `(0, dy)`. `dx = k/W`, `dy = k/H` (float products of `1/W` and `k`), `k` per entry (the `.rdata`
constants, addresses in `aa_constants.cpp`):

| entry | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 |
|---|---|---|---|---|---|---|---|---|---|---|
| k | -1.5 `143D7F0CC` | 1.5 `143D7EE38` | -7.5 `143D7F104` | -5.5 `143D7F0FC` | -3.5 `143D7F0F0` | 3.5 `143D7EEC8` | 5.5 `143D7EEE4` | 7.5 `143D7EF28` | -1 `143D7F0C8` | 1 (not multiplied) |

Other `.rdata`: 1.0 `143D7EE10`, 0.5 `143D7EDB4`, 2.0 `143D7EE7C`. The four-tap ring at +-1.5 (entries 0, 1) and the +-1 pair (8, 9)
plus six far taps per axis (+-3.5, 5.5, 7.5) are the DLAA edge search. The 1280x720 buffers are printed by the test.

## 5. Binding and state (STATIC, `1B9790` after the copy)

The roles of the context wrappers (`1400201B0` VS constant buffers, `140020250` PS constant buffers, `140006100` PS SRVs, `14002BA00` PS
samplers, `14002BAA0`/`14002B7E0` PS/VS, `140005F00` render targets, `141920CD0` viewports) are read from the context offsets they write and
checked against DLAA's DXBC bindings (t0, s3, cb0, cb1): INFERRED, consistent.

- Copy first (`FUN_14002C320`, scratch object `[GXRS+0x4DD8]+0x7A0`): VS `ScreenSpaceQuadShader` (`[4DD8]+0xC18`), PS `PointSampleColor1`
  (`[4DD8]+0xB70`, 688 B, SHA-256 `204D5639...AD53`, `SS_ClampPoint` s2, `t0`), sampler `common+0x2E0` in slots 0 and 2, source = the
  target's texture (`RT+0x28`), viewport = the scratch's own size, draw of the strip; the helper saves and restores its state.
- Common state `[[144961AB0]+0x2A0]`: blend `+0x08` (blending off, mask RGBA), depth-stencil `+0x1D8` (depth and stencil off, stencil ref 0xFF),
  rasterizer `+0x208` (solid, cull none, front CCW, scissor off), PS samplers 0 and 3 = `+0x2F0` (the linear clamp of the family
  `+0x2E0..+0x2F0`, FFX-DRAW; DLAA samples with explicit LOD 0 on a one-mip texture, so the max-LOD variant is immaterial).
- Constant buffers as section 4; PS SRV slot 0 = the scratch copy's SRV (`scratch+0x48`); VS `filter+0x60`, PS the mode's programme;
  input layout `filter+0x58` (built from `FXAA.vpo`: `POSITION` R32G32B32A32 at 0, `TEXCOORD` R32G32 at 16, stride 24).
- Output: one render target, `RT+0x30` (the view's final LDR colour, R8G8B8A8 at the back-buffer size), no depth; viewport
  `{0, 0, W, H, 0, 1}` with `W, H` from the target. IA: vertex buffer `[4DD8]+0xB58` stride 0x18, `TRIANGLESTRIP`, `Draw(4, 0)`
  (`FUN_14007F820`). The strip is the glow stage's `quad_vertices` (`-1,1,1,1,0,0, 1,1,1,1,1,0, -1,-1,1,1,0,1, 1,-1,1,1,1,1`).
- Afterwards the SRV and render target are unbound and the state restored. DLAA writes alpha 1 and keeps luma-below-floor pixels bit-exact
  (a copy of the centre).

`RT` is `view+0x1E0` for a normal view, the swap chain buffer when `view+0x7E0` is set (FB890 lines 116, 581-585); the glow's final target
is the same object, so **glow and UI draw on the anti-aliased image** and the gamma pass reads the result.

## 6. Place in the frame

Original, per world frame (OBSERVED I-0054 m40, TONEMAP-SKIN section 5; STATIC FB890 enqueue order): ... YEBIS draws (tone map, then the
grading draw into the LDR target), `CustomDofLutCopy`, **[`PointSampleColor1`, `DLAA`]** (one `1B9790`), `EmissiveToGlowPS`, 4x
`Sample13Weight`, 5x `GlowCompositeSinglePS` (last into the LDR target), the UI, the display/gamma. One DLAA draw per frame
(I-0063 `drawn-families.tsv`; two or three while the Status page shows). Menu views reach it too: their pipelines keep `3EBA` and FB890 enqueues the filter for every
listed view (READ, I-0140 tranche 39), and the original runs it on the Status body's 560x768 every frame and the portrait's
512x256 once per opening (MEASURED, `aa-observe.flag`); the earlier « menu views never reach it » (INFERRED) was wrong. Natively
D-0278 removed the filter; D-0305 decides the menu views take the native's own TAA or DLSS (not yet implemented).

## 7. Where it goes natively

Today (`native/renderer/scene_pass.cpp`, the frame record near the composition): `target = display ? display_.Target() :
backbuffer RTV`, `output = display ? display_.Intermediate() : buffer` (`DisplayStage::Format()` = R8G8B8A8_UNORM, back-buffer size).
`tonemap_.Record(...)` then `grading_.Record(...)` write `target`; `if(glow){ ... glow_.Record(...) }` follows, then the UI, then
`display_.RecordPass`. **The AA pass belongs between the grading record and `if(glow)`**, on `target`/`output`. Nothing reads the quality
byte today (`D3D12-PHASE-G-GRAPHICS-OPTIONS-SHEET` row Antialiasing).

Ready-to-code steps:

1. **AA-1 inputs.** `NativeAaInput { uint32_t mode; float p[4]; float factor; programs; bool run; }` collected in
   `CollectNativeYebisInputs` (`game_bootstrap.cpp` ~9186): `aa[]` is already blended at YB-4b; `mode = aa[1] - 1`; require `aa[0] != 0`
   and `aa[1] in 1..4` (type 0 = library AA: no GX pass); p = `aa[6..8]` (mode 3) or `aa[2..5]`; gate with `ds3content::AaRuns`
   (quality byte `[quality+8]`, factor `[settings+0x98]`, both already read at the `144962190` write ~9342). Log a `native_aa_inputs` row
   like `native_glow_inputs`. Flag `no-aa.flag` to disable.
2. **AA-2 programmes.** `AcquireNativeAaPrograms()` beside `AcquireNativeGlowPrograms`: `DLAA` (pixel, 6132 B) and `FXAA` (vertex, 1112 B)
   with the pins of section 3; modes 0-2 later (their `.ppo` are in the same repository, same CBV b0, no b1).
3. **AA-3 stage.** `native/renderer/aa_stage.{h,cpp}` modelled on `glow_stage` (same quad, `POSITION`/`TEXCOORD` layout, one-descriptor
   sampler table at `s3` = linear clamp, SRV table `t0`): root = CBV b0 (visibility ALL, VS and PS read it), CBV b1 (PS), SRV table,
   sampler table; PSO: R8G8B8A8_UNORM, blend off (write mask 0xF), no depth, solid, cull none, FrontCCW, depth clip on; owns the scratch
   copy texture (R8G8B8A8_UNORM, display extent, `Resize`/`Close`/`QuarantineToProcessExit` like the glow stage). `Record(list, output,
   target_rtv, constants_sink, recorder)`: transition `output` RT -> COPY_SOURCE and the scratch to COPY_DEST, `CopyResource` (exact for
   the point copy), scratch -> PIXEL_SHADER_RESOURCE, `output` -> RENDER_TARGET, set viewport/scissor of the display extent, upload the two
   buffers from `MakeDlaaConstants(display width, height, p[0], p[1], p[2], factor)`, `Draw(4)`; scratch left in PSR.
   (A `Ldr[A]/Ldr[B]` ping-pong, YEBIS-D3D12-DESIGN-NATIVE 4.2, removes the copy later; the copy is the first rung and bit-comparable.)
4. **AA-4 wiring.** `OpenAa` like `OpenGlow` (~line 2030); `const bool aa = tonemap && input.aa && input.aa->run && AaReady(...)`; one new
   bit in the plan key (`graph->Execute` mask) and a `graph->Declare` of the scratch (`"aa_copy"`, PSR, persistent); `graph->Pass("aa",
   PassKind::PostProcess)` with Read(scratch)/Write(output); resize and close next to the glow's (~3110, ~3137, ~3182). The pass requires
   `display` (the intermediate); without it skip (it cannot read the swap chain buffer it writes).
5. **AA-5 option class.** Treat the Antialiasing option as a G-class gate in the options sheet: OFF (quality byte 0) removes the pass and
   changes the plan key; no resource is recreated.
6. **AA-6 witnesses.** (a) test: `aa_constants_test` (done). (b) native `native_aa_constants` row: the two buffers' digests per extent.
   (c) image: native with/without the pass on a still frame, edges only differ, pixels with luma < 0.1 identical (gamma-encoded values);
   (d) against the original: Reference run, the LDR image after `DLAA` (`post_check` spec 9 of I-0054) on m40; expected equal up to
   everything else that still differs (YEBIS parity, shadows).

## 8. Interactions

- **Upscaler** (UPSCALER-DLSS-READY decision 4): the GX filter is proposed off while DLSS/DLAA-NGX runs; `AaRuns` is the single gate to
  extend. Constants take the **output** extent (they follow `RT+0x50/54`, the target, not the render size).
- **Glow** (GLOW-WAVE1-READY): additive composites on the AA result; the glow's input is GB5, unaffected.
- **Gamma pass / UI**: unchanged; they come after.
- **Setting Info `Enable Anti-Alias`** (`144961FA0`, YEBIS-MOTIONBLUR-NATIVE) is a different, library-side flag; whether it feeds `view+0x3EBA`
  is not established.

## 9. Uncertain, to settle by one live capture of the original (m40, default settings, plus Antialiasing OFF)

- The scratch object `[4DD8]+0x7A0`: format and extent (INFERRED equal to the LDR target; native copies same-format same-size).
- The exact sampler descriptor at `common+0x2F0` (INFERRED linear clamp; immaterial for DLAA's LOD 0 sample of one mip).
- `view+0x3EBA` for the world view and its source (INFERRED 1 whenever the quality byte and factor allow it).
- The factor `[[GXRS+0x5368]+0x98]` at HIGH/MAX: 1 is assumed (the table value was not read); the `CBFxaaParam` bytes of one captured DLAA
  draw (`[4]` = `3DCCCCCD` when 1) would confirm it and the operation order bit for bit (the older `FxaaParam` producer of the live host was
  already compared byte for byte, `light_producers.cpp`).
- `filter+0x34` (the fourth copied float of the DLAA settings) is never read by the mode-3 code or the PS.
- Cut-scene views: whether any of them runs the filter. The Status menu views do (I-0140 tranche 39, MEASURED: gate true, the
  quality byte 1 and the factor 1, also for the world's 1920x1080 at the defaults).

## 10. Implemented (2026-10-18)

AA-1 to AA-4 and AA-6(b): `CollectNativeYebisInputs` builds `NativeAaInput` (row `native_aa_inputs`: quality 1, factor 1, words
`3DCCCCCD 3FB33333 3DF5C28F` at m40), `AcquireNativeAaPrograms` (pins held, row `native_aa_programmes_resolved` 1112/6132 bytes),
the frame preparation makes the two buffers (`MakeDlaaConstants`, the camera extent), `renderer/aa_stage.{h,cpp}` (scratch copy, then
FXAA.vpo + DLAA.ppo over the gamma intermediate) between the grading and the glow, plan-key bit 34, row `native_aa_frame`;
`no-aa.flag` opts out. Types 1-3 (the FXAA programmes) are not native: the record never asks them in the shipped files.

Witness (Firelink, Alpha still at the bonfire; Native2135 with, Native2136 without, Reference2137): the stair-stepped silhouette
edges (shoulder, strap) of the native frame without the pass are smoothed with it, as the original's
(`evidence/I-0066/artifacts/i66-native-2135-aa-on/aa-off-on-original.png`); the dark areas (luma under the 0.1 floor) are left
as they are in both. Not compared bit for bit (frames differ by animation). Section 9 stays open.
