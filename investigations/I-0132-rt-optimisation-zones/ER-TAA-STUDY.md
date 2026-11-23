# Elden Ring 2.7.1 temporal anti-aliasing (TempAA): static study

Copied from `evidence/I-0132/artifacts/er-taa-study/ER-TAA-STUDY.md` (2026-11-10, a Sonnet `ghidra-research` worker for the R pilot; its scripts
and raw outputs stay there). Guides R11j (R11-PLAN §11); its statuses are the worker's, the pilot's reading in R11-PLAN.

Binary: `eldenring_2.7.1.0_25080141_1A354710.exe`, image base `0x140000000`, Ghidra project `ER_2_7_1_25080141_I0001`
(headless 12.1.3, `-process -readOnly -noanalysis`, mirror `er-mirror-i0072-a`, JVM at Idle priority, one run at a time).
Nothing launched, nothing written outside this folder. All addresses are ER virtual addresses (VA).
Statuses: OBSERVED = read in the decompile / bytes; INFERRED = reasoned from observed code; NOT FOUND.

Method and outputs (all in this folder):
- `ErStrSearch.java` + `needles1.txt` -> `strings1.txt` (raw scan of non-code memory for ASCII/UTF-16 strings, with data refs).
- `riprefs2.py` (RIP-relative reference scan of the exe on disk, derived from `riprefs.py`) -> `rip1.txt`..`rip6.txt`.
- `qrefs.py` (8-byte absolute pointers), `rtti.py` (RTTI -> vtable), `vtname.py` (function -> vtable class), `pev.py` (VA reader).
- `ER897Export.java` (the existing exporter, unchanged) -> `exp1`..`exp10` (decompile `.c` + `.asm.txt` per function).
- `ErXrefs.java` -> `xrefs1..3.txt`, `range1.txt` (functions in 141ad5000..141ae2000).
Function names below are mine; Ghidra shows `FUN_<addr>`. The binary has no function symbols.

## 0. The one-paragraph answer

ER does not have one "TAA". It has an anti-aliasing stage `GXBS::GXAntiAliasingFilter` with five selectable modes
(index 0..4): FXAA2, FXAA3, FXAA3_HQ, DLAA (a spatial "directional localised AA", not NVIDIA DLAA), and **TempAA = mode 4**,
the temporal one. TempAA: Halton(2,3) jitter on the main camera, history in two ping-pong targets, a full-screen resolve
that reads current colour + previous history + two other view targets (depth and velocity, order to confirm) + the dilated
motion vector and an optional stencil mask, a current-to-previous matrix, a depth-banded feedback weight, an optional
SharpenFilter pass, run **before** tone map / DOF composite (YEBIS), whereas the non-temporal modes run at the very end.
The history is seeded (copied from current colour) on the first frame / after any reset instead of being blended.

## 1. Entry points, option, class names

| Finding | Address | Evidence | Status |
|---|---|---|---|
| RTTI `GXBS::GXAntiAliasingFilter`, vtable `0x142f242b8` (slot 0 `0x141ad64f0` scalar-deleting dtor; slot 1 `0x141ad79a0` = the per-pass Execute) | TypeDescriptor str `0x143d318a8`, COL `0x1433bb068` | `rtti.py` -> `0x143d318a8 COL 0x1433bb068 vtable 0x142f242b8 [0x141ad64f0, 0x141ad79a0]`; `vtname.py 141ad79a0` -> slot 1 of that class | OBSERVED |
| Param classes: `GXBS::GXAntiAliasFilterParam` (vtable `0x14305ba68`), `GXUpdateJob<GXAntiAliasFilterParamManager>` (vtable `0x142f1cdd8`) | `0x143d37ce0`, `0x143d2e780` | `rtti.py` | OBSERVED |
| Param editor definition (debug param UI = the full parameter list with ranges) | `0x141c37880` | registers "Anti-Alias-Editor"/"Anti-Alias Param", "Yebis-Anti-Alias Enable", enum `AntiAliasType`, then FXAA, DLAA, TempAA parameter groups | OBSERVED |
| Mode enum names (editor): `FXAA2_GS`, `FXAA3_GS`, `FXAA3_HQ_GS`, `DLAA_GS`, `TempAA_GS` at `0x143052ad0/ae8/b00/b18/b28` | `0x141c37880` | enum rows built with values 1,2,3,4,5 (the decompile shows the values as 1..5) | OBSERVED (editor values); runtime filter mode is 0..4 (it indexes the shader table), mapping 1..5 -> 0..4 INFERRED |
| Runtime mode field: filter object `+0xc` (default 3 = DLAA in the ctor `0x141ad6240`); enable byte `+9`; TempAA "armed" byte `+0x21` | `0x141ad6240`, `0x141ad78c0` | `*(uint*)(p+0xc)=3`; TempAA active iff `+9 && +0x21 && +0xc==4` and the global gate below | OBSERVED |
| Global gate: `*(_DAT_1447f37a8+0x13a60)` byte `+9` set turns AA mode to 0 (`FUN_141ad7580`); `+8` and float at `*(_DAT_1447f37a8+0x13a68)+0xa8 > 0` required to run | `0x141ad7580`, `0x141ad78c0` | e.g. `FUN_141ad7580: if (*(char*)(g+0x13a60)+9) return 0; return *(int*)(p+0xc)` | OBSERVED code; meaning of that global (looks like a debug/photo/overlay override) UNKNOWN |
| Graphics option: config key `"Antialiasing"` (wide string `0x142c01688`, key pointer table `0x143b4d060`, entry `0x143b4d0b8`), stored in the graphics-config struct at `+0x2c`, read/written by `0x140e954e0` (save) and `0x140e96480` (load), as an enum of **6 values: DISABLE=0, LOW=1, MEDIUM=2, HIGH=3, MAX=4, CUSTOM=5** (table `0x142c012a0`, 16-byte {name, value} entries) | `0x140e954e0` line `MOV R9,[0x143b4d0b8]` after `LEA RDI,[0x142c012a0]` | `exp9/140e954e0.asm.txt`; table dump in this study | OBSERVED (key, struct offset, value set) |
| Mapping from that option (0..5) to AntiAliasType / enable | not reached | | **NOT FOUND** (the setter of filter `+9`, `+0xc`, `+0x21` was not located; the stores go through a pointer to the filter embedded in the view at `+0xbac0`, so no direct-displacement scan finds them) |
| Strings: `TempAA Auto Jitter` (shadow param bool, editor row built at `0x141bb4980`, `0x141bb54dc`), `VolumetricFog DirLight JitterCount/JitterXYSize` (`0x141bcb140`), `ToneMap_BrightPassTemporalPS.ppo`, `LensSimu2_ScaledTemporal.ppo`, `TemporalFeedbackWeight` (DOF/tone map own temporal filters, not this TAA) | see `strings1.txt` | | OBSERVED names; their use not studied (shadow "TempAA Auto Jitter" suggests shadow sampling jitter linked to TempAA: INFERRED from the name only) |

Per-mode editor parameters (`0x141c37880`, offsets inside `GXAntiAliasFilterParam`; ranges are the 4-float rows [min, max, step, 1] read from the data blocks named in the call, e.g. `0x143053710`):
FXAA: FxaaAlpha `+0x60`, FxaaReduceMin `+0x64`, FxaaReduceMul `+0x68`, FxaaSpanMax `+0x6c`; DLAA: DlaaThreshold `+0x70`,
DlaaRamda `+0x74`, DlaaEpsilon `+0x78`; **TempAA**: FeedbackWeight `+0x7c` (0.01..0.99), JitterCount `+0x80` (int 1..64),
EnableDitherSwitch `+0x84`, EnableMotionVectorDilation `+0x85` (bytes), SharpenIntensity `+0x88` (0..1), SharpenRangeMin `+0x8c` (-10..0),
SharpenRangeMax `+0x90` (0..10), SubpixelFeedbackFactor `+0x94` (0..1), CBFeedbackWeightSampled `+0x98` (0.01..0.99),
CBFeedbackWeightFiltered `+0x9c` (0.01..1), EnableStencilMask `+0xa0`, RejectedPixelBlurIntensity `+0xa4`, FBWeightScale(None,0,1,2)
`+0xa8..+0xb4`, FBWeightCancelByDepth(0..3) `+0xb8..+0xc4`, FBWeightBorderDepth(0..2) `+0xc8..+0xd0` (0.1..10000). OBSERVED.
The values actually used per area come from GX param files (data), not the exe: NOT FOUND statically.

## 2. Camera jitter

| Finding | Address | Excerpt | Status |
|---|---|---|---|
| Sequence: **Halton base 2 and 3**, sample `h(i+1,2) - 0.5`, `h(i+1,3) - 0.5` (so each coordinate in (-0.5, 0.5), units = pixels, see below). `FUN_141ad7470` = GetJitter(filter, float2*) | `0x141ad7470`, radical inverse `0x141ad75f0` | `uVar3 = *(uint*)(p+0x124); if (*(char*)(p+0x10)) uVar3 >>= 1; fVar4 = h(uVar3+1,2); fVar5 = h(uVar3+1,3); out = {fVar4-0.5, fVar5-0.5}` with `_DAT_142f24750 = _UNK_142f24754 = 0.5`; `h`: `fVar5 /= base; fVar4 += (n % base) * fVar5; n /= base` | OBSERVED |
| Number of samples = `JitterCount` (filter `+0x64`; editor range 1..64; **default 8**, `FUN_141ad6530`: `*(int*)(p+0xc)=8` in the block at `+0x58`). Index `+0x124` incremented once per TempAA setup call and wrapped: `idx = (idx+1); n = +0x64 (x2 if flag +0x10); if (n <= idx) idx = 0` | `0x141ad6640` | `*(int*)(p+0x124) += 1; uVar18 = *(uint*)(p+100); if (*(char*)(p+0x10)) uVar18 *= 2; ... if (uVar18 <= uVar13) uVar13 = 0;` | OBSERVED. Flag `+0x10` (copied from the init descriptor byte, `0x141ad7640`): doubles the cycle and halves the index (each Halton sample used for 2 consecutive frames); meaning of the flag UNKNOWN (checkerboard / two-eye style?) |
| First samples with default index start 0: (0.5,1/3)-0.5 = (0.000, -0.167), then (-0.25, 0.167), (0.25, -0.389), (-0.375, -0.056)... | computed from the radical inverse | arithmetic on the observed function | INFERRED (arithmetic) |
| Where applied: `0x141c7a580` (main view setup) calls GetJitter when the view's AA flag `+0xc0cd` is set, multiplies `x` by `0.5` when `*(view+0x30)==1`, and **writes the pair into the camera record copy at `+0x74` (x) and `+0x78` (y)** | `0x141c7a6e9` | `FUN_141ad7470(view+0xbac0,&j); if (*(int*)(view+0x30)==1) j.x *= _DAT_1432a1920 /*0.5*/; rec[+0x74]=j.x; rec[+0x78]=j.y;` | OBSERVED |
| How it enters the projection: `FUN_141a545b0(cam, outMatrix, applyJitter)`: when `applyJitter != 0` and `cam+0x74 / +0x78 != 0`, the clip matrix is post-multiplied by a shift of **`(2*jx/width, jy * -2 / height)`** NDC (`_DAT_1432a19e4 = -2.0`, width = `cam+0x5c`, height = `cam+0x60`), i.e. jitter is in pixels, +x right, +y down, a constant NDC translation | `0x141a545b0` | `fVar12 = (fVar5+fVar5) * (1/width); fVar10 = jy * -2.0 * (1/height);` then folded into the 4x4 | OBSERVED code; sign convention INFERRED from the -2.0 |
| **Removed for the reprojection**: when TempAA builds its matrices (`0x141ad6640`) it copies the camera record and overwrites record words `+0x74/+0x78` (`uStack_b4/uStack_b0`) with `_DAT_1448062e0` (8 bytes, a very widely read global, 3,962 RIP references, so a shared zero/identity vector: INFERRED) before calling `FUN_141a545b0(..., 1)` and `FUN_141a549b0` | `0x141ad6640` | `uStack_b4 = (undefined4)_DAT_1448062e0; uStack_b0 = (undefined4)(_DAT_1448062e0 >> 32);` then `FUN_141a549b0(&uStack_128,&uStack_268); FUN_141a545b0(&uStack_128,&uStack_368,1);` | OBSERVED code; "zero" value INFERRED. The saved previous matrices (`filter+0x140` view-proj, `filter+0x180` view) are therefore unjittered |
| Velocity vs jitter: whether the engine's velocity buffer is jitter-free was not read (the velocity writers are the MotionBlur passes `MotionBlurWriteCMBVelocity*`, `MotionBlur_WriteVelocityAndDepthPackPS`, strings at `0x143035eb8..`) | | | NOT FOUND |

## 3. The passes

Per-view post chain driver: `FUN_141c7ce60` (called from `0x141c7cb90`). It reads the view object `V` (the AA filter lives at `V+0xbac0`).

### 3.1 Where TempAA sits (OBSERVED order of the calls in `0x141c7ce60`, INFERRED class identities)

1. `FUN_141ad8f90(filter, ...)` (when `V+0xc0cd`), then optional passes under `V+0xc0dd`.
2. Motion-blur class (`V+0xc098`, `V+0xc0a0`; its loader `0x141b48ba7` references the `MotionBlur*.ppo` names): `FUN_141b47f00`, `FUN_141b488e0`, guarded by `V+0xc0cb`, `V+0xc0cc`.
3. DOF class (`V+0xc090`; loader `0x141bda30e` references `LensSimu2_*.ppo`): `FUN_141bdc5d0`, `FUN_141bd7460`.
4. A pass tagged `0x28`/`0x40` on `V+0xbf50` (`FUN_141b8ea80(...)`) taking `V+0x2d8`, `*(V+0xa70)+0x48`, `V+0x7b0` (the lit scene compose / HDR scene colour).
5. **If `FUN_141ad7580(filter) == 4` (TempAA): `FUN_141ad6640(filter, ctx, 0x3a, 10, colour=V+0x2b0, p6=*(V+0xa70), p7=*(V+0x7b0), p8=(*(V+0x30)==1), p9=*(*(V+0x338)+0x80))`.** This is the TempAA pass.
6. Tone map class (`V+0xbcf0`; loader `0x141b8f480` references `ToneMap_*.ppo`): `FUN_141b89640` under `V+0xc0dd`.
7. DOF final composite `FUN_141bd6d40(V+0xbf88, ..., TempAAactive = FUN_141ad78c0(filter))`: the DOF composite is told whether TempAA is active.
8. **If mode != 4 (FXAA2/FXAA3/FXAA3_HQ/DLAA): `FUN_141ad6640(filter, ctx, 0x40, 0xf)` at the very end**, on the final colour (`FUN_141a1cc10(V+0x248)`), after tone map, bloom, DOF, motion blur.

So **TempAA runs on the HDR scene colour, before tone map and before the DOF composite; the spatial AA modes run last.** Status: OBSERVED for the call order; the identification of 6/7/3/2 as tone map / DOF / motion blur comes from the loader functions that reference those `.ppo` name tables (`rip6.txt`, `rip5.txt`), INFERRED. UI is not in this function. Bloom not placed (the tone-map class owns `ToneMap_Bright*`/`BloomComposite*` programmes, so bloom is at or after step 6: INFERRED).

### 3.2 Inputs of the TempAA resolve (record built by `FUN_141ad6640`, 0x100 bytes from `FUN_141ad61e0`)

- `rec+0x00..0x5b` = copy of the TempAA parameter block `filter+0x58..0xb3`; `rec+0x60 = p5` (scene colour), `rec+0x68 = p6 = *(V+0xa70)`, `rec+0x70 = p7 = *(V+0x7b0)` (**both are required: if either is null the pass is skipped**, `if ((param_6 == 0) || (param_7 == 0)) goto end`).
- Of `p6/p7` one is depth, one is velocity: the depth-threshold maths (4 view-space distances turned into NDC depth with `m22, m32, m23, m33` of the camera, `0x141ad79a0`) proves a hardware-depth input, and `V+0x7b0` is the one the motion-blur step passes only when its feature flag `V+0xc0cb` is on (INFERRED: `V+0x7b0` = velocity, `V+0xa70` = depth). Not proven.
- `rec+0x78 / 0x80`: dilated-motion-vector history/target when `EnableMotionVectorDilation` (`filter+0x69` path): targets `filter+0x100` and `+0x108`, format code `0x22`; `rec+0x98/0xa0/0xa8` stencil-mask related textures (`param_9` = `*(*(V+0x338)+0x80)` is the stencil/mask source); `rec+0xb0` flag "dilated MV available", `rec+0xb1` "reprojection matrix valid", `rec+0xb2` "stencil mask valid", `rec+0xb4` float = `*(ctx+0x4dc) / filter[+0x134]` (ratio of a per-frame value to its previous value; the quantity at `ctx+0x4dc` was not identified: could be frame time or exposure) .
- Reprojection matrix: `rec+0xc0..0xff` (`puVar14[0x18..0x1f]`), a 4x4 built with `FUN_140147990` (4x4 multiply) and `FUN_1401490b0` (4x4 inverse) from the current unjittered view / view-proj and the saved previous ones (`filter+0x140..0x17f` previous view-proj, `+0x180..` previous view), **identity (`0x1430b38e0`, flag `rec+0xb1=0`) when `filter+0x130 == 0`** (no valid previous frame). The exact algebra was not resolved: INFERRED "current clip -> previous clip".

### 3.3 History targets and formats

- Two history colour targets `filter+0xf0` and `+0xf8` (ping-pong; current index `filter+0x120`, swapped at the end of the pass `*(p+0x120) = 1 - *(p+0x120)`), created at the size of `p5` (`*(p5+0x50)`, `*(p5+0x54)`) with format code `0x1a`; `filter+0x100` and `+0x108` format `0x22`; `filter+0x110/0x118` format `0x3e` (stencil-mask history). `FUN_1419f2ed0(target, w, h, formatCode)`. If the engine's format codes are DXGI numbers (not verified), `0x1a` = R11G11B10_FLOAT, `0x22` = R16G16_FLOAT, `0x3e` = R8_UINT. Status: format codes OBSERVED, DXGI reading INFERRED.
- Pipeline objects: `GXBS` loader `0x141ad7640` fetches `FXAA.vpo` (`+0x1c0`), `TempAA.vpo` (`+0x1c8`) and 11 pixel programmes into `filter+0x1d0 + 8*i` from the name table at `0x143c27c90` (loop count 0xb at `0x141ad7852`).

### 3.4 Steps of mode 4 in `0x141ad79a0` (Execute) (OBSERVED unless stated)

1. Optional pre-passes: `StencilAndMotionVectorPreprocess` (`+0x220`) or `AAStencilPreprocess` (`+0x210`) and/or `DilateMotionVector` (`+0x218`) through `FUN_141ad90a0` / `FUN_141ad92b0`, depending on which of `rec+0x98/0xa0/0xa8` exist and `rec+0xb0`.
2. Constant buffer (`filter+0xb8`, 0x110 bytes, `FUN_141acf190`): size, 1/size, 2/size, 0.5/size, the transposed reprojection matrix, `FeedbackWeight`, the `+0x60` field, `rec+0xb4`, and for **four depth bands** a pair `(FBWeightScale_i, FBWeightCancelByDepth_i)` plus the border depth converted to NDC depth (`uStack_158..`, loop `uVar25 < 4`, border clamped with `max(border, near)`; for band 3 the border is 0).
3. Draw a full-screen pass with `TempAA.vpo` + `TempAA.ppo` (`filter+0x1f0`) or `TempAA[StencilMask].ppo` (`+0x1f8`, when `rec+0xb2`) **into `filter+0xf0[idx]`** (the new history; RTV from `history[idx]+0x70`), reading the other history `filter+0xf0[1-idx]`, the dilated MV and the viewed targets above.
4. Result to the scene colour: if `SharpenIntensity (filter+0x6c) <= 0` or no sharpen shader, copy history[idx] -> scene colour (`FUN_1419f3320`); otherwise run `SharpenFilter.ppo` (`filter+0x200`) from history[idx] into the scene colour, so **the history stays unsharpened and a separate sharpen pass exists** (INFERRED from argument roles of `FUN_141ad92b0` / `FUN_1419f3320`: destination object first, source resource last; direction not proven by names).
5. First frame / invalid reprojection (`rec+0xb1 == 0`, `cStack_688 != 0`): **the pass is skipped and the current scene colour is copied into `history[idx]`** (seeding), then the index is swapped. OBSERVED.
6. Optional `AACopyStencil` (`+0x208`) path keeps the stencil mask history in `filter+0x110/0x118`.

## 4. CPU constants and history reset

- Filter default block (`FUN_141ad6530`, at filter `+0x58`, 0x5c bytes): EnableReproj byte `+0x58`=1; `+0x5c` FeedbackWeight = **0.75** (`0x3f400000`); `+0x60` = 0.001 (`0x3a83126f`, not in the editor list: name UNKNOWN); `+0x64` JitterCount = **8**; bytes `+0x68`=1 (EnableDitherSwitch), `+0x69`=1 (EnableMotionVectorDilation), `+0x6a`=1 (EnableStencilMask); `+0x6c` SharpenIntensity = 0; `+0x70` Min = -2.0; `+0x74` Max = 2.0; `+0x78`,`+0x7c`,`+0x80` = 0.9, 0.8, 0.9 (Subpixel, CBSampled, CBFiltered); `+0x84` = 0.5 (RejectedPixelBlurIntensity); `+0x88..0x94` = 1.0 x4 (FBWeightScale); `+0x98..0xa4` = 0 x4 (CancelByDepth); `+0xa8,0xac,0xb0` = 10, 20, 40 (BorderDepth). The field names follow the editor order (floats match; the byte flags are grouped differently): INFERRED. These are code defaults; the shipped values come from the GX param files: NOT FOUND.
- Dither switch: `filter+0x128` toggles `1 - x` each TempAA frame when `+0x68` (EnableDitherSwitch) and `param_8 == 0`, else 0; read by `FUN_141ad75b0` and by `FUN_141a0b840` (a per-frame consumer; not studied). OBSERVED toggle, use INFERRED.
- Previous matrices and `ctx+0x4dc` saved in the filter each frame (`+0x130` valid flag set to 1, `+0x134` value, `+0x140..+0x1bf` matrices) at the end of the setup. OBSERVED.
- **Reset of the history** (`filter+0x130 = 0` => identity reprojection, no resolve, seed copy): in `FUN_141ad8e90` (release of `+0x108/+0x110/+0x118`), in `FUN_141ad7210` (full release, also clears `+0xa` = "targets created"; called from init `0x141ad7640`, i.e. whenever the AA stage is (re)initialised, e.g. on resize/view recreation, from `0x141a1f190`), when the mode is not 4 or AA is globally gated off (`0x141ad6640` ends with `*(char*)(p+0x130) = 0`), and when the view is disabled (`p+9 == 0` calls `FUN_141ad8e90`). When `+0xa == 0` the history targets are created (size from `p5`). Camera cut: **NOT FOUND** (no camera-cut flag is read in the TempAA code; the previous matrices are simply replaced every frame, so a teleport only avoids ghosting through the depth/velocity based rejection, INFERRED). Resolution change: handled by re-init (above); an explicit size comparison was not seen: INFERRED.
- Special view type `*(V+0x30) == 1` (`param_8`): the jitter x is halved and, in `0x141ad6640`, `param_8 != 0` makes the history unused (targets released, `+0x128 = 0`, pass not scheduled). Meaning of view type 1 UNKNOWN.

## 5. Shader identity

Pixel/vertex programme names (wide strings, table at `0x143c27c90`, 11 x {name ptr, 1}); ER names programmes by string, the programmes themselves are in the shader archives (not in the exe):

| Slot (`filter+0x1d0+8*i`) | Name | String address |
|---|---|---|
| 0 | `FXAA.ppo` | `0x142f242c8` |
| 1 | `FXAA3.ppo` | `0x142f242e0` |
| 2 | `FXAA3HQ.ppo` | `0x142f242f8` |
| 3 | `DLAA.ppo` | `0x142f24310` |
| **4** | **`TempAA.ppo`** | `0x142f24328` |
| **5** | **`TempAA[StencilMask].ppo`** | `0x142f24340` |
| **6** | **`SharpenFilter.ppo`** | `0x142f24370` |
| 7 | `AACopyStencil.ppo` | `0x142f24398` |
| 8 | `AAStencilPreprocess.ppo` | `0x142f243c0` |
| 9 | `DilateMotionVector.ppo` | `0x142f243f0` |
| 10 | `StencilAndMotionVectorPreprocess.ppo` | `0x142f24420` |
| vertex | `FXAA.vpo` (`+0x1c0`, modes 0..3 and sharpen/copy), `TempAA.vpo` (`+0x1c8`, mode 4) | `0x142f24470`, `0x142f24488` |

The same strings also appear in the debug-parameter list (`SharpenIntensity`, `SharpenRangeMin/Max`, `JitterCount`, `EnableReproj`, `FeedbackWeight`, ...). The slot-to-role mapping (slots 5..10) is confirmed by the Execute code using `filter+0x1f0/0x1f8/0x200/0x210/0x218/0x220`. OBSERVED.
Shader-side constants/semantics (the `.ppo` bodies) were not examined: NOT FOUND.

## 6. What could not be found

- The code mapping the "Antialiasing" graphics option (DISABLE/LOW/MEDIUM/HIGH/MAX/CUSTOM) and the editor `AntiAliasType` to the filter's `+9/+0xc/+0x21` and the quality differences between LOW..MAX (JitterCount, sharpen?).
- The data-driven per-area parameter values (FeedbackWeight, JitterCount, sharpen, depth bands): they live in GX param files.
- The identity of the camera-record fields other than `+0x74/+0x78` (jitter) and `+0x5c/+0x60` (width/height) used by the projection helper; the exact reprojection matrix algebra; which of `*(V+0xa70)`/`*(V+0x7b0)` is depth and which velocity (INFERRED velocity = `0x7b0`); whether the velocity target is jitter-free.
- The semantic of filter flag `+0x10` (cycle x2), view type `*(V+0x30)==1`, the `ctx+0x4dc` ratio, the `0.001` field at `+0x60`.
- The DXGI meaning of the format codes (`0x1a`, `0x22`, `0x3e`).
- Runtime behaviour (no game launched).
