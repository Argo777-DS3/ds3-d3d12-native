# The SSAO parameters of wave 1: layer parameters, view record, quality gate, programmes and resources (ready to implement, static)

Request (coordinator, 2026-10-11): an implementation-ready note, in the format of [GLOW-WAVE1-READY](GLOW-WAVE1-READY.md), for the SSAO parameters of wave 1 (item 5 of [WAVE1-PREP](WAVE1-PREP.md) §2.5: S1 `AoParams`, `AoView` and the quality gate; S2 resources, post root signature and prewarmed pipelines). It gives the original shaders with their fingerprints, the constants read in the executable, the targets, the pass order, the commits under flags, the tests and the witnesses by family. Independent Opus 5.5 review applied (log in §11). Static; no native code was changed, nothing was launched. The AO passes themselves (S3 to S8) stay wave 2; this note fixes everything they need so that wave 2 only records draws.

Labels: **verified** = read in the executable (`DarkSoulsIII.exe` 1.15.2.0, SHA-256 `EF5E07C5…498C`) or in the decompiled functions for this note; **observed** = read in the native tree at `3c7bc82a` (line numbers are that tree's; the anchors are function names); **note** = a statement of a linked note; **proposed** = this note's design; **open** = needs a read or a run. Abbreviations: `SP` = `native/renderer/scene_pass.cpp`, `GB` = `native/frame/game_bootstrap.cpp`, `LP` = `native/world/light_producers.cpp`, `MF` = `native/renderer/model_frame.h`.

## 0. Answer first

1. **The layer parameters come from a 17-dword record of the `SSAO` gparam group, blended between two keys, then copied into the layer** (`103510` → `14015AF60` twice → `140223780` → `1400FEFA0`, all verified). The record layout is the layer's own: dword *i* lands at `layer+0x15C+4i`, and the first dword is `fDepthClipRange` (4000), which no earlier note listed. Keys: **index 8** of the view context (A at `+0x4018`, B at `+0x4080`, weight at `+0x40C8`); the render step has **two call sites** and the second one, which passes the normal and so is the world path of the captures (**inferred**), looks up **A twice** (B is never read; §1.1). The blend has a quirk that matters at the key borders: between the two keys the last three fields (`DepthFadeStart`, `DepthFadeRange`, `PrecisionDepth`) are **not blended** and keep the built-in 3800 / 200 / 4000 (§1.2).
2. **The "previous-frame object" of the earlier notes has a writer: `FUN_1401B1960`** (the layer's per-frame setup, called from `103510`). It allocates a 0x70-byte record in the pass arena and fills it with three object references, a 4×4 matrix at `+0x20` (the current-to-previous screen matrix), a **validity byte at `+0x60`** (the previous camera is close enough in rotation: three diagonal terms ≥ 0.7071), and the **strength at `+0x64` = `[[GXRS+0x5368]+0xB0]`** (1.0 natively). It is *not* a reset factor, and there is no explicit reset on a cut: the validity test is the only guard (§1.3).
3. **The quality gate of the earlier notes was overstated** (`SSAO-D3D12-DESIGN-NATIVE` §0 item 3, §2.4, row SS9): the AO **pass** runs from **LOW** upward. The flags `+0x0A`, `+0x0B`, `+0x0C` (and the MAX forcers `+0x0D`, `+0x0E`, `+0x0F`) only choose the temporal pass, the bilateral upscale and the use of normals; the variant is a pure function of them (§1.4). DISABLE (`+0x09` = 0) is the only level without the pass. Whether `t33` is written at LOW (no upscale runs there) is **open** (row SS9).
4. **The random-rotation texture has an owner, and native already has it**: `FUN_14007ECB0` creates 16 textures of 64×64 RGBA8 with the xorshift generator seeded `0x075BCD15`; native `ProduceModelDitherTextures` is a bit-faithful port of that very loop (same seed, recurrence, trigonometry and quantisation) and `NativeCommonImages::dither[16]` already holds them. `t2` is image `layer_counter & 15` (§1.5): no new asset.
5. **Programmes and constants are pinned and read**: seven original programmes with SHA-256 and DXBC hash (§2), the 8-vector kernel base table with bit images, the `(i·0.14285 + 1)` radius ramp, the defaults and the quality table rows (§3).
6. **Design**: `AoParams`, `AoView` and a pure `PlanAo` in `native/world` or `native/frame` (CPU, testable), the four constant producers re-expressed on them, an `AoStage` that owns targets, root signature and pipelines (inert in wave 1), and an `AoPrograms` set resolved like the lighting programmes. Eight commits AO-1..8 under flags (§6); no pixel changes in wave 1.

## 1. What the original does (verified)

### 1.1 The flow of one frame

1. `FUN_140103510` (the render step) checks the pipeline byte **`+0x3EB8`** (SSAO capability: `1` from the constructor `1400843C0`, reset to 0 by `85540` and restored by `885F0` when `pipeline+0x50 & 8` holds; natively treated as 1) and reads the weight (`+0x40C8`). It has **two call sites** of the whole sequence that follows. **Site 1** (`103510.c:315-350`, asm `140103A08…140103B12`, the `iVar28 == 0` path) looks up key A (`+0x4018`) and key B (`+0x4080`) and passes no normal (`param_5` = 0, so variants 9 and 10 cannot occur there). **Site 2** (`103510.c:1024-1059`, asm `140104F13…140105005`, the other path) looks up **`+0x4018` for both** (B is never read, so the blend is A with A) and passes `[lVar27+0x138]` as the normal; it matches the observed WithNormal chain and is taken as the world path (**inferred**; which views take which path is **open**, row SS17). Each site looks both records up in the SSAO bank at `root+0x4608` (`root` = `*param_2` of the render step, the object whose `+0x4658` holds the Glow bank in `FB890`) with `14015AF60` (the same key rule as every bank: `{−1, −1}` → `{1, 0}`; the engine's fallback record when absent), blends them with `140223780` into a 17-dword block initialised with the built-in values, and copies the block into the layer at **pipeline `+0x1240`** with `1400FEFA0` (skipped when `layer+0x158 ≠ 0`, "ignore setup from outside").
2. If `[pipeline+0xFD0]` is non-null it calls **`1401B1960(layer, pass, group, …)`**: the setup (§1.3). It runs only if the quality object's `+0x09` is set and the strength factor `[[GXRS+0x5368]+0xB0]` is > 0 (the factor is 1.0 when the object is absent); otherwise it clears `layer+0x220` and stops. The setup pushes a command `{layer, record, 0}` into the pass buffer (`FUN_1400BF4C0(pass, 3, 0x18)`).
3. The command later runs the layer callback **`1401B22E0`**: requires the programme object in slot 3 (`layer+0x240` = `0x228 + 8·3`, virtual `+0x10` non-null) **and `record+0x64 > 0`**; saves the viewport and render target state, binds the five bank samplers (§1.4), writes `layer+0x110 = {p22, p32/fDepthClipRange, fDepthClipRange, 0}` (`p22 = pass+0x78`, `p32 = pass+0x88`) and its constant buffer to slot b0, runs **`1401B30E0`** (the depth downsample into `layer+0x48`) unless `record[2]` supplies a half-size depth (note: `1B30E0` binds the full depth at `t0`, writes a **colour** target and uses the programme in slot `layer+0x238`, which is **not** `DepthScale2x2OutputDepth` and is unidentified; the half and quarter depths of the captures come from **`FUN_1401CB9D0`**, run twice by `1CB920` on `pipeline+0x3958` into `pipeline+0x3980` (640×360) and `+0x39E0` (320×180); in the world `record[2] = pipeline+0x3980` and `1B30E0` is presumably skipped), then **`1401B26E0`** (§1.4), then restores the state.

### 1.2 The parameter record (`1400FEFA0`, `140223780`; defaults from `103510`)

| Dword | Layer offset | gparam name | Built-in default | Blended |
|---|---|---|---|---|
| 0 | `+0x15C` | `fDepthClipRange` | 4000 | lerp |
| 1 | `+0x160` (4 bytes) | bytes: `UseNormal`, `UseReProj`, `PreventGhost`, (byte 3 unused) | 0, 0, 0 | bytes 0 and 1 from B; **byte 2 (`PreventGhost`) is never copied in the lerp case** |
| 2 | `+0x164` | `Ver2 Sample Offset` | 4 | lerp |
| 3 | `+0x168` | `Ver2 AO Scale` | 1 | lerp |
| 4 | `+0x16C` | `Ver2 AO Offset` | 0 | lerp |
| 5, 6, 7 | `+0x170/74/78` | `Ver2 Kernel Scale Base Dist / Min / Max` | 10, 0.2, 2 | lerp |
| 8 | `+0x17C` | `PrevFrame Strengh` | 0.7 | lerp |
| 9 | `+0x180` | `PrevFrame Depth Aware` | 1 | lerp |
| 10 | `+0x184` | `PrevFrame Depth Reject Rate` | 0.02 | lerp |
| 11 | `+0x188` | `Geom Bent Offset` | 0 | lerp |
| 12 | `+0x18C` | (not named by any gparam) | 0 | lerp |
| 13 | `+0x190` (byte) | `EnableUpScale` | 0 | byte from B |
| 14 | `+0x194` | `DepthFadeStart` | 3800 | **not blended** |
| 15 | `+0x198` | `DepthFadeRange` | 200 | **not blended** |
| 16 | `+0x19C` | `PrecisionDepth` | 4000 | **not blended** |

The names of dword 0 (`fDepthClipRange`) and of dwords 11 and 12 are **inferred**: `1B3FB0` binds neither `+0x15C` nor `+0x18C`, `fDepthClipRange` equals `PrecisionDepth` (4000) in every file and `Geom Bent Offset` and dword 12 are both 0; the position of the other fifteen is verified by the layer offsets of §2.1 of the earlier note. `default` id 10 also has Sample Offset 2, Scale 1.2 and kernel max 1.5.

The blend (`140223780`, `ε = [143D7ECE8] = 1.1920929e-7`): `|t| ≤ ε` → all 17 dwords from A; `|t − 1| ≤ ε` → all 17 from B; otherwise the "lerp" column. So in the two normal situations (a key reached or an area change that has completed) the whole record is A or B; only during a key transition do dwords 14 to 16 and `PreventGhost` fall back to the built-ins (3800 / 200 / 4000 / 0). The default dwords were read from the executable: `143D80070` = (0, 10, 0.2, 2) for dwords 4 to 7, `143D7F5C0` = (0.7, 1, 0.02, 0) for dwords 8 to 11, immediates 4.0 / 1.0 for dwords 2 and 3, `0x456D8000` / `0x43480000` / `0x457A0000` for 14 to 16.

Values for the world, **verified** from the decoded files (`evidence/I-0068/queries/gparam/*.tsv`): `mapbase.gparam` id 0 is `fDepthClipRange 4000`, the four bytes all 1, Sample Offset **1.5**, Scale **1.2**, Offset 0, kernel **10 / 0.2 / 2**, PrevFrame **0.7 / 1 / 0.02**, Geom 0, DepthFade **100 + 200**, Precision 4000. `default.gparam` id 0 is the same with UseReProj / PreventGhost / UpScale 0, Sample Offset 4, Scale 1, DepthFade 3800 + 200; ids 10 to 15 are the quality and view variants (id 10: UseNormal 0, no reprojection; ids 11 to 15: all four bytes 1, Sample Offset 2, Scale 1.2, kernel max 1.5). The zone files carry no `SSAO` group.

### 1.3 The view record (`1401B1960`)

| Record offset | Content |
|---|---|
| `+0x00`, `+0x08`, `+0x10` | object references: `param_4` (the full-resolution depth), `param_5` (the G-buffer normal, GBuffer0), `param_6` (an optional half-size depth: `pipeline+0x3980` when `pipeline+0x3EC0` is set, else 0; non-zero skips the layer's own downsample) |
| `+0x20…+0x5F` | the **current-to-previous screen matrix**: 16 floats written as four rows of four; (u, v, depth, 1) of the current frame → the previous frame's homogeneous coordinate (the convention of `MotionBlurWriteCMBVelocity`'s `g_mCurToPrevScreen` and of `SeparatedReProj`'s `dp4` with `cb2[11..14]`, verified in the DXBC) |
| `+0x60` | **validity** (byte): 1 only if the layer holds a previous frame (`layer+0x220`) **and** the diagonal terms `M[0][0]`, `M[1][1]`, `M[2][2]` of `M = inverse(V_cur) · V_prev` (row-vector form, view matrices only; `fStack_2d8`, `fStack_274`, `fStack_260`, `1B1960.c:347-387` and `:533`) are all ≥ `[143D7EDE4]` = **0.70709997** (about cos 45°); else 0, and the matrix is the **identity** (rows `143D1BF90`, `143D1BFA0`, `143D1BFB0`, `143D1C080`, read from the executable) |
| `+0x64` | **strength** (float) = `[[GXRS+0x5368]+0xB0]`, 1.0 if the object is absent. `1B22E0` requires it > 0; `1B26E0` multiplies `AO Scale` and the offset blend by it |

State saved in the layer for the next frame: `layer+0x224` (the frame counter) is incremented and `layer+0x220` set to 1 **unconditionally** by the setup; only the saving of the current view data into `layer+0x1A0…0x21C` (the view matrix at `+0x1A0…0x1DC`, the projection at `+0x1E0…0x21C`) is skipped when the debug byte `DAT_144962318` is set. The counter is **reset to 0 by `1B26E0` whenever the temporal path is off** (§1.4). The previous camera is used as is: there is no reset on a cut or a teleport; a large rotation makes the record invalid for that frame, and the depth-aware rejection of `SeparatedReProj` handles translation.

### 1.4 The gate, the variants and the order of the AO passes (`1401B26E0`)

`q` = the quality object `[[GXRS+0x5360]]`. Rows read from the quality table (`quality-table.tsv`, bytes `+0x2D + 7·level`): DISABLE `00…`; LOW `+09`; MEDIUM `+09 +0B`; HIGH `+09 +0A +0B +0C`; MAX all seven (`+09…+0F`).

```
E = q[0x0A] ≠ 0 && (UseReProj ≠ 0 || q[0x0D] ≠ 0) && record.valid      // temporal history in use
R = E && (PreventGhost ≠ 0 || q[0x0D] ≠ 0)                              // separated reprojection pass
N = q[0x0C] ≠ 0 && (UseNormal ≠ 0 || q[0x0F] ≠ 0) && record[1] ≠ 0     // normals
U = q[0x0B] ≠ 0 && (EnableUpScale ≠ 0 || q[0x0E] ≠ 0)                   // bilateral upscale
if (!E) layer.counter = 0                                               // before the rotation image is chosen
rotation image = layer.counter & 15  (the 16 images of §1.5)
```

| Case | AO call (`1B33D0`) | Then |
|---|---|---|
| `E && R` (HIGH, MAX in the world) | variant **9** (`N`) or **7** into the raw target `post+0x8E0` | `1B38D0` `SeparatedReProj` with the history `post+0x840` into the half-resolution AO `layer+0x80`; the history copy runs **before** the AO draw, at the start of the temporal branch (`1B26E0.c:191-224`), from `*(layer+0x78)` |
| `E && !R` | variant **10** (`N`) or **8**, the programme reprojects itself, history passed | — |
| `!E` (LOW, MEDIUM, first frame, invalid record) | variant **9** (`N`) or **7**, no history | — (at LOW no upscale runs: whether `t33` is written is open, SS9) |

Then, if `U`: `1B3CD0` upscales `*(layer+0x68)` (`1B26E0.c:408`) into the full-resolution output `layer+0xC8` with the full depth `record[0]` (this is `t33` at HIGH, **observed** 1280×720 RGBA8 in the five zones). What `t33` is when `U` is false (LOW, and MEDIUM without the byte) is **open** (the lighting binding was not read; row SS9). The variants index the programme slots `layer+0x228+8·variant`; the names `7` `ScreenSpaceAO2`, `8` `…_ReProj`, `9` `…_WithNormal`, `10` `…_WithNormalReProj` follow the name order of the cache and are **inferred** (the observed HIGH chain uses `WithNormal`, `SeparatedReProj`, `BilateralUpScale`; row SS14 reads the hashes).

Samplers of the layer prologue (`1B22E0`, five `PSSetSamplers`): bank rows 0, 1, 3, 4, 6 at `s0…s4` (**verified**, master plan §6 S1): `s0` point wrap, `s1` trilinear wrap, `s2` point clamp, `s3` trilinear clamp, `s4` point **white** border.

### 1.5 Targets and the rotation texture

| Target | Size (1280×720) | Format | Producer |
|---|---|---|---|
| half depth `layer+0x48` | 640×360 | `R32G8X24` (DSV `D32_FLOAT_S8X24_UINT`, SRV `R32_FLOAT_X8X24_TYPELESS`) | `FUN_1401CB9D0` (`DepthScale2x2OutputDepth`: min of four taps, writes `oDepth`, DSV-only target; run twice by `1CB920` on `pipeline+0x3958`: half at `pipeline+0x3980`, quarter at `+0x39E0`); the layer's own `1B30E0` is the unidentified colour-target alternative of §1.1 |
| raw AO `post+0x8E0` | 640×360 | RGBA8 | AO pass (separated path only) |
| AO half `layer+0x80` | 640×360 | RGBA8 | reprojection (or the single AO pass); `x` = AO, `yzw` = the linear depth packed in base 255 |
| history `post+0x840` | 640×360 | RGBA8 | copy of `layer+0x80` (programme id 191) |
| output `layer+0xC8` (`t33`) | 1280×720 | RGBA8 | `1B3CD0` |

`g_vScreenSize` (`cbSSAOParam2` +64) = `(W, H, 1/W, 1/H)` with `W, H` = the uints `layer+0xA0/+0xA4`: **inferred** the half-resolution size (the shader indexes the rotation texture with `uv × screenSize / 64`); `range = float(uint(layer+0xC)) / [143D7F044]` with `[143D7F044]` = **1080.0**, `layer+0xC` = the full-resolution height: **inferred** (row SS13 reads both).

The 16 rotation textures: `FUN_14007ECB0` fills them in post-object storage `post+0x30 + 0x70·i` (64×64, `DXGI_FORMAT 0x1C` = RGBA8_UNORM; the texture-view pointer is the element's member `+0x48`, the same member as the glow levels', hence the reader's `post+0x78 + 0x70·n` in `1B26E0`), generator seeded `0x075BCD15` (`FUN_1417AFFD0`), 4096 texels each, consumed in sequence; each texel: `angle = (rand%0x800001 + 0x3F800000 − 1.0)·2π`, byte 0 = `(cos·0.5 + 0.5)·255`, byte 1 = `(sin·0.5 + 0.5)·255`, bytes 2 and 3 zero; native `ProduceModelDitherTextures` (`native/content/model_common_textures.cpp:68`) is the same loop and `GB:740-743` admits the 16 images. The same function also builds a 32×32 hemisphere-vector texture (`post+0x730`) that the AO passes of wave 1/2 do not read.

### 1.6 Corrections to earlier notes (verified)

| Earlier statement | What the code says |
|---|---|
| "At LOW and MEDIUM the chain does not run and `t33` stays white" (SSAO-D3D12 §0.3, §2.4; row SS9) | the AO pass runs from LOW; the flags choose temporal, upscale and normals (§1.4). Only DISABLE (`+0x09 = 0`) skips the pass; whether `t33` is written at LOW (no upscale) is open |
| "the previous-frame object: writer not located" (SSAO-NATIVE risk 1; row SS7 "g = 0 after a cut") | written by `1B1960`; `+0x64` is the user strength factor (1.0), not a reset; validity is `+0x60` (§1.3) |
| "owner of the 64×64 rotation texture not located" (SSAO-NATIVE step 1; SS5) | the 16 images of `7ECB0`, already native (§1.5) |
| the parameter table of SSAO-D3D12 §2.1 | missing `+0x15C` `fDepthClipRange`, `+0x18C`, and the blend quirks of §1.2 |

## 2. The programmes: original DXBC, pinned (verified)

From the I-0063 cache (`gxposteffect` bundle; `evidence/I-0068/queries/fingerprints/programs.tsv`; disassembly in `evidence/I-0068/queries/ssao/dxbc/` and `…/ssao-ready/dxbc/`). All pixel shaders are `ps_5_0` and use the quad vertex shader `ScreenSpaceQuadShader` of [GLOW-WAVE1-READY](GLOW-WAVE1-READY.md) §1.2 and §2 (post member `+0xC18`, shared with the glow, strip of four clip-space vertices).

| Programme | Bytes | SHA-256 | DXBC hash | Bindings |
|---|---|---|---|---|
| `DepthScale2x2OutputDepth` | 1244 | `8ED440FB52EBF50C8DAB08A5E7CFAF3E0CDE2CEC0B10ABC5A322CE04FBDA6543` | `7F197E9E9DD98FC8530A515157C57005` | `t1 g_DepthTex`, `s2`, `cb1` (4 vectors: four tap offsets); input `TEXCOORD0`; output `oDepth` = `min` of the four taps; no colour target |
| `ScreenSpaceAO2` (variant 7) | 4384 | `5B304D81A9559B79FEEEBA6D274362B74CE9A6E7EC451756C3E0C1ACED31692C` | `6CC3AA9B1F0D381FAC738F6A8999CAD7` | `t0` depth, `t2` rotation; `s0 s4` (verified in the disassembly; no normal, no history) |
| `ScreenSpaceAO2_ReProj` (8) | 5304 | `E06454AE90ECEA1E5BCF12538558E27C53A81DA95EEB9EAF3B16D185901D31F4` | `509443E3E26A586592B927019B5BDB08` | `t0`, `t2`, `t3` history; `s0 s2 s3 s4` (verified) |
| `ScreenSpaceAO2_WithNormal` (9) | 4932 | `38299D8728C5FD99EBC5D06A61B8AD1FF6AB1E4A572740F95F55D330F4C9EB29` | `82EAC6F6F234FD687358AAEF4F6A696D` | `t0` half depth, `t1` normal, `t2` rotation; `s0 s2 s4`; `cb0` (1 vector), `cb1` (kernel, 9 vectors dynamically indexed), `cb2` (18 vectors) |
| `ScreenSpaceAO2_WithNormalReProj` (10) | 5792 | `0156472914F7340080FD679F2CB936CC584E91FC10DE9E9EFCC0CA5BB0203EAE` | `6397A524372B2285AD67A08CE50EDF1D` | `t0 t1 t2 t3`; `s0 s2 s3 s4` (verified) |
| `ScreenSpaceAO2_SeparatedReProj` | 3876 | `64D47110B81ED4C620C33F19190DC9BFFAD8029C38AFAF8FFC7248CD614FC197` | `FF2C481E42431D0D7E72BE277252401A` | `t0` depth, `t1` raw AO, `t3` previous AO; `s2 s3 s4`; `cb0`, `cb2` (18) |
| `ScreenSpaceAO2_BilateralUpScale` | 3764 | `78710C47DEB74DD6830FF8B9AB857D1EA7E1946C145D672A5C348D42604A2D67` | `BA1F6CCFBC59EAEC5A6A571577BA5F16` | `t0` full depth, `t1` AO; `s2 s4`; `cb0`, `cb2` (5 vectors used) |

Constant buffers (reflection): `cbSSAOParam` (48 B: `g_vDepthComputeParam`, two unused blend vectors), `cbSampleOffset` (256 B: `g_avSampleOffsets[16]`), `cbSSAOParam2` (288 B: `g_mInvProj` 0, `g_vScreenSize` 64, `g_mView` 80, `g_vAOScaleOffset2` 144, `g_vSampleRangeParam` 160, `g_mCurToPrevScreen` 176, `g_vPrevFrameStrengh` 240, `g_vMiscParam` 256, `g_vDepthRangeParam` 272). The producers `SsaoParam`, `SsaoKernel`, `SsaoParam2`, `DownsampleOffsets` (`LP`, anonymous namespace, dispatch at the `W1B33D0`, `W1B38D0`, `W1B26E0`, `W1CB9D0` writers) reproduce them bit-for-bit from the engine's arguments; §5.5 re-expresses their inputs.

## 3. Constants read in the executable (verified)

| Item | Value |
|---|---|
| kernel base `(x, y, z, w)` of the eight entries (`1B26E0`, stack table built from `143D7FF30, 143D7FEC0, 143D7FE80, 143D7FF20, 143D7FF30, 143D7FF10, 143D7FE70, 143D7FEB0`) | v0 `(0, −1, 1, 1)`; v1 `(−0.7071067, 0.7071067, 1, 1)` bits `BF3504F2 3F3504F2`; v2 `(−1, 0, 1, 1)`; v3 `(−0.7071067, −0.7071067, 1, 1)`; **v4 `(0, −1, 1, 1)` again (the eighth read repeats `143D7FF30`; no `(0, 1)` entry)**; v5 `(0.7071067, −0.7071067, 1, 1)`; v6 `(1, 0, 1, 1)`; v7 `(0.7071067, 0.7071067, 1, 1)` |
| radius ramp | entry *i*: `x, y ×= (i·0.14285 + 1)·SampleOffset` with `[143D7ED80]` = 0.14284999 (`3E124745`); `z, w` multiplied by `1·1` |
| `SsaoKernel` post-step | `C` = `[143D7FEF0]` = (1, 1, 1, 1); `S` init `[1439F2490]` = 0; `out = {d.x, d.y, d.z/S.z, d.w}` with `S.z` = 8 → weights 0.125 |
| scalars | `1.0 = [143D7EE10]`; `0.5 = [143D7EDB4]`; `2.0 = [143D7EE7C]`; `−1 = [143D7F0C8]`; `ε = [143D7ECE8]` = 1.1920929e-7; relative-rotation threshold `[143D7EDE4]` = 0.70709997 (`3F350481`); height reference `[143D7F044]` = 1080; sign mask `[143D801C0]` = `80000000` |
| `g_vDepthRangeParam` | `{(p22·d + p32)/d, 1/Range, −(Start/Range) (a sign-bit XOR with `[143D801C0]`)}` with `d = Start + Range` (`1B26E0`, `auStack_148`) |

## 4. What native has and lacks (observed, tree `3c7bc82a`)

| Item | Native |
|---|---|
| `t33` and `t32` | neutral white in the lighting and the point lights (`SP:993-994`, `native_light_neutral_`), the forward pass (`GB:3768`) |
| producers | `SsaoParam`, `SsaoKernel`, `SsaoParam2`, `DownsampleOffsets` exact but fed from call-argument snapshots (`LP:327-410`, dispatch `LP:835-851`) |
| quality object | built natively; bytes `+0x09…+0x0F` written from the table (`game_quality_object.cpp:11-13`), tested, **never read as a gate** |
| bank reader | `Banks::GroupFloat` (floats only; fails when the key is absent from the area file, `mapbase` and `default`) |
| keys | `CollectNativeVelocityInputs` shows the pattern: view context from `scene_view_cpu_owners`, keys `+0x4008/+0x4070/+0x40C0` (index 6), epsilon `image+0x3D7ECE8`, factor `[[image+0x4961B50]+0x5368]+0xE0`; the function is at `GB:4600` and called at `GB:4709`, its result copied to the frame at `GB:5443` |
| previous-frame matrix | `VelocityViewProjection` and `VelocityCurrentToPrevious` (`renderer/velocity.h`, double precision, row-vector convention): `from_uv · inverse(V_cur·P_cur) · (V_prev·P_prev) · to_uv`, the same convention as §1.3 |
| rotation images | `NativeCommonImages::dither[16]` (§1.5) |
| programmes | `AcquireNativeLightingPrograms` (`GB:2714`; `FindLoadedProgramme` `GB:2693`) resolves lighting programmes by name (`FindLoadedProgramme`); no AO programme is looked up (the witness `WitnessLightingProgrammes` lists `DepthScale`/`SSAO` names only for logging) |
| samplers | `EngineSamplerBankOwner` (`content/sampler_bank.h`) gives `descriptors[16]`; rows 0, 1, 3, 4, 6 are constant across the texture-quality levels (master plan §6 S1) |
| resources | none; the closest precedent is `NativeVelocityResources` (`SP:1218`) |

## 5. Design (proposed)

### 5.1 `AoParams` and `Banks::Ssao` (S1a)

```
struct AoParams {                               // the 17-dword record of §1.2, named
    float depth_clip_range{4000};
    uint8_t use_normal{}, use_reproj{}, prevent_ghost{}, reserved{};
    float sample_offset{4}, ao_scale{1}, ao_offset{};
    float kernel_base{10}, kernel_min{0.2f}, kernel_max{2};
    float prev_strength{0.7f}, prev_depth_aware{1}, prev_reject{0.02f}, geom_bent{}, word12{};
    uint8_t enable_upscale{}; uint8_t pad[3]{};
    float depth_fade_start{3800}, depth_fade_range{200}, precision_depth{4000};
};
```

`Banks::Ssao(a_area, a_id, b_area, b_id, weight, epsilon, AoParams& out, std::string* sources)` beside `GroupFloat`/`Glow`: for each key the 18 parameters of the group `SSAO` (short names as in the dump: `fDepthClipRange`, `UseNormal`, `UseReProj`, `PreventGhost`, `EnableUpScale`, `Ver2 Sample Offset`, `Ver2 AO Scale`, `Ver2 AO Offset`, `Ver2 Kernel Scale Base Dist/Min/Max`, `PrevFrame Strengh`, `DepthFadeStart`, `DepthFadeRange`, `PrecisionDepth`, `PrevFrame Depth Aware`, `PrevFrame Depth Reject Rate`, `Geom Bent Offset`; booleans through `Banks::GroupWord` of [GLOW-WAVE1-READY](GLOW-WAVE1-READY.md) §5.1), the key rule of `GroupFloat` (`{−1, −1}` → `{1, 0}`; area file, else `mapbase`, else `default` id 0), then the **`140223780` blend of §1.2** (A, B or the lerp column with the built-in tail). A key absent everywhere yields the built-in record (the bank's fallback; its exact content is **open**, row SS3).

### 5.2 `AoView` and the pure view update (S1b)

```
struct AoView {                                  // the 0x70-byte record of §1.3 without the object references
    std::array<float,16> cur_to_prev;            // +0x20..+0x5F, row-major as written
    bool valid;                                  // +0x60
    float strength;                              // +0x64
    bool have_normal, have_half_depth;           // record[1] ≠ 0, record[2] ≠ 0
};
struct AoLayerState { bool have_previous{}; VelocityMatrix previous_view{}, previous_projection{}; uint32_t counter{}; };
AoView UpdateAoView(const VelocityMatrix& current_view, const VelocityMatrix& current_projection, AoLayerState& state, float strength, bool freeze);
```

`UpdateAoView` (pure, CPU): with `M = inverse(current_view) · state.previous_view` (the 4×4 row-vector product of the **view matrices only**), `valid = state.have_previous && M[0][0] ≥ 0.70709997 && M[1][1] ≥ 0.70709997 && M[2][2] ≥ 0.70709997` (verified algebra, `1B1960.c:347-387` and `:533-571`); `cur_to_prev = VelocityCurrentToPrevious(current_vp, previous_vp)` with `previous_vp = state.previous_view · state.previous_projection` when valid, else the **identity**; then, unless `freeze` (`DAT_144962318`), `state.previous_view = current_view; state.previous_projection = current_projection; state.have_previous = true`, and **unconditionally** `++state.counter`. The layer keeps the view and the projection separately (`+0x1A0…0x1DC`, `+0x1E0…0x21C`) because the validity test needs the view alone. `Resize` of the stage resets `AoLayerState` (`have_previous = false`, `counter = 0`) so that the first frame after a resize runs the non-temporal variant and never reads an uninitialised `AoHalf[prev]`. Precision: the original multiplies in float and the native velocity helper in double, so `cur_to_prev` is compared with a relative tolerance and frozen golden matrices (§7 T3), not bit-exact.

### 5.3 `PlanAo`: the gate and the variants as one pure function (S1b)

```
struct AoQuality { bool b09,b0A,b0B,b0C,b0D,b0E,b0F; };              // quality object bytes +0x09..+0x0F
struct AoPlan { bool run, temporal, separated, normals, upscale; int variant; uint32_t rotation_image; bool reset_counter; };
AoPlan PlanAo(const AoQuality&, const AoParams&, const AoView&, uint32_t counter);
```
with exactly the equations of §1.4 (`run = b09 && strength > 0`; `E`, `R`, `N`, `U`; `variant = (E && !R) ? (N?10:8) : (N?9:7)` (the separated pass keeps the plain/normal AO variant 7 or 9 and adds `SeparatedReProj`); `reset_counter = !E`; `rotation_image = (reset_counter ? 0 : counter) & 15`). The unit test T4 enumerates the 5 levels × the layer byte combinations.

### 5.4 `CollectNativeAoInputs` (S1b)

Beside `CollectNativeVelocityInputs` (defined `GB:4600`, called `GB:4709`), same context lookup: reads the key at `context+0x4018` (A; **B is not read for the world view**, §1.1 site 2: both lookups use A, and the weight `context+0x40C8` is still applied, so the lerp column with the built-in tail applies at 0 < t < 1 although A equals B), the epsilon `image+0x3D7ECE8`, the strength `[[image+0x4961B50]+0x5368]+0xB0` (1.0 if the root is absent), the quality bytes `[[image+0x4961B50]+0x5360]+0x09…+0x0F` through the existing quality-object reader (`GB` quality section), and the camera of the scene; calls `Banks::Ssao`, `UpdateAoView` (state held per main view across frames; the offscreen menu views get no AO) and `PlanAo`; stores `std::optional<NativeSelectedScene::Ao> ao` (`params`, `view`, `plan`, `layer_height`) and copies it to `NativeModelFrameInput::ao` (a `shared_ptr<const NativeAoInput>` after `velocity`, `MF:75`) next to the velocity copy (`GB:5443`). The row `native_ao_inputs`: `{read, keys[4], weight, quality[7], plan{run,E,R,N,U,variant,image}, params digest, view{valid,strength}, sources}` on the first three frames, on change and every 600th. 

### 5.5 The four producers on native structures (S1c)

`SsaoParam`, `SsaoKernel`, `SsaoParam2`, `DownsampleOffsets` keep their formulas; their `Region` reads become reads of `AoParams`, `AoView` and a small `AoPassInput { projection (pass+0x50), view (pass+0x10), p22 (pass+0x78), p32 (pass+0x88), layer_width, layer_height (+0xA0/+0xA4), layer_full_height (+0xC) }` built from `scene->camera`, `camera_constants` (projection bytes 256…, column order of `ProduceCameraViewConstants`) and the half-resolution target size. The snapshot wrappers stay as the oracle (they are in the unnamed namespace: the new cores are free functions in `world/ao_constants.h/.cpp`, called by the wrappers, tested against **frozen golden byte arrays** derived once from the wrappers' output, as in GLOW-WAVE1-READY T1). The kernel is built by `AoKernelBase(sample_offset)` from the 8 base vectors and the ramp of §3.

### 5.6 Programmes (S2a)

`NativeAoPrograms { vertex, depth_downsample, ao[4] (variants 7..10), separated_reproj, upscale }` in `MF`, filled by `AcquireNativeAoPrograms()` beside `AcquireNativeLightingPrograms` (`GB:2700`): `FindLoadedProgramme(0x4792D58, L"ScreenSpaceQuadShader", 1u)` and `FindLoadedProgramme(0x4792BF8, name, 0u)` for the seven names of §2; each blob compared with the SHA-256 of §2 (a pin); an incomplete set disables the AO for the session softly (row `native_ao_programmes_unresolved`, no `Refuse`: it exits the process). The set travels in `NativeAoInput::programs`.

### 5.7 `AoStage`: resources, root signature, pipelines (S2a, S2b; inert in wave 1)

New `renderer/ao_stage.h/.cpp` (the shape of `GlowStage`):

| Resource | Format, size | Lifetime |
|---|---|---|
| `AoDepthHalf` | `R32G8X24_TYPELESS`, ⌈W/2⌉×⌈H/2⌉ (640×360), DSV + SRV | transient, per frame; produced in the original by `1CB9D0` (§1.5), not by the layer's `1B30E0` |
| `AoRaw` | RGBA8, half size | transient |
| `AoHalf[2]` | RGBA8, half size | **persistent ping-pong**: the reprojection writes `AoHalf[cur]` and reads `AoHalf[prev]` (replaces the history copy `post+0x840`; the original's `layer+0x80` and `post+0x840` are the same data a frame apart) |
| `AoFull` (`t33`) | RGBA8, W×H | per frame, read by the lighting and the forward pass in wave 2 |

Root signature (a private root like `GlowStage`'s, not the master plan's b0, b1, b2, b5, b8 / t0…t33 / s0…s15 family, which later work may merge): root CBVs `b0`, `b1`, `b2` (pixel), one SRV table `t0…t3` (4 descriptors, pixel), a **sampler table of 5 descriptors `s0…s4`** copied from the bank rows 0, 1, 3, 4, 6 (`EngineSamplerBankOwner::Lease::descriptors`; the stage takes them as an `Open` argument so it needs no image access; tests pass the default descriptors); input layout `POSITION` float4 + `TEXCOORD` float2 (stride 24) as the glow quad; flags `ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT`, version 1.0. Pipelines (prewarmed in S2b, **seven**, keyed by the hashed key of phase C when it lands): depth-only `DepthScale2x2OutputDepth` (`DepthFunc ALWAYS`, depth write on, no render target, `DSVFormat D32_FLOAT_S8X24_UINT`, `NumRenderTargets 0`), `WithNormal`, `SeparatedReProj`, `BilateralUpScale` (RGBA8, blend off) and the variant set 7/8/10 created lazily (they are used at MAX/LOW and in transitions). Sizes follow the presenter (`Resize` after the caller's drain); total about 8.3 MB at 720p (`AoDepthHalf` 1.84 MB, three RGBA8 half targets 0.92 MB each, `AoFull` 3.69 MB; the "about 4 MB" of WAVE1-PREP counts the half-resolution set only, 4.6 MB).
The stage **never clears** a target (every target is fully written by its pass), so no optimised clear value is needed and a clear-value mismatch cannot raise a debug-layer warning. At frame start `AoHalf[cur]` goes `SRV→RT` (it was read as `prev`'s partner the frame before) and `AoHalf[prev]` stays `SRV`. Unlike WAVE1-PREP S2, which puts the programmes in `NativeLightingPrograms`, this note keeps a separate `NativeAoPrograms` (the AO may be absent without disabling the lighting).

### 5.8 The pass order for wave 2 (contract)

Per main-view frame after the G-buffer and the readable-depth copy, before the shadow composite and `GXLightDeferredAO`: if `plan.run` (a) depth downsample into `AoDepthHalf` (the `1CB9D0` min-of-four programme, as the original; the layer's own `1B30E0` is skipped when the record supplies a half depth, which it does in the world), (b) AO draw (variant, rotation image `plan.rotation_image` as `t2` from `dither[]`), (c) `SeparatedReProj` when `separated` (reads `AoRaw`, `AoHalf[prev]`, writes `AoHalf[cur]`), (d) bilateral upscale into `AoFull` when `upscale`. Barriers: `AoDepthHalf` `DEPTH_WRITE→SRV`, `AoRaw` `RT→SRV`, `AoHalf[cur]` `RT→SRV` (next frame it is `prev`), `AoFull` `RT→SRV`, the main depth read-only DSV + SRV while sampled. `plan.run == false` or an unresolved programme set binds the neutral white as today.

## 6. Commits (each builds, passes the unit tests, is reversible alone)

| # | Commit | Behaviour change | Flag | Test |
|---|---|---|---|---|
| AO-1 | **pure data and functions**: `AoParams`, `AoView`, `AoLayerState`, `AoQuality`, `AoPlan`, `PlanAo`, `UpdateAoView`, the kernel base and ramp, `AoPassInput` (`world/ao_core.h/.cpp`) | none | — | T1, T3, T4 |
| AO-2 | **bank reader**: `Banks::Ssao` (depends on GL-2 for `Banks::GroupWord`, so there is a single definition) | none | — | T2 |
| AO-3 | **producers on native structures**: the four cores in `world/ao_constants`, wrappers call them | none | — | T5 |
| AO-4 | **inputs**: `NativeAoInput`, `NativeSelectedScene::Ao`, `CollectNativeAoInputs`, the copy, rows `native_ao_inputs`, quality gate read | none (no consumer) | `no-ao.flag` | T6, live SS1-SS3 |
| AO-5 | **programmes**: `NativeAoPrograms`, `AcquireNativeAoPrograms` with the seven pins | none | `no-ao.flag` | T7 |
| AO-6 | **`AoStage`**: targets, root, sampler table, quad, prewarmable pipelines, not wired | none | `no-ao-resources.flag` | T7, T8 (WARP) |
| AO-7 | **wiring of the resources only**: `ao_stage_` member, lazy `Open`, `Resize`/`Close`/`Detach`, the memory row | none to colour (targets allocated, nothing drawn) | `no-ao-resources.flag` | T9 (resize, debug layer clean) |
| AO-8 | **development aids**: `ao-quality.flag` (a level 0 to 4 replaces the option rows `+0x09…+0x0F` at the read), `ao-verify.flag` (rows of the first frames) | none unless present | the two flags | T10 |

Order: AO-1 → AO-2 → AO-3 (CPU, after AO-1), AO-4, AO-5, AO-6 (needs AO-1), AO-7, AO-8. No pixel of the final colour changes through wave 1: the idle High Wall frame digest and `content_draws` stay equal after each commit (the AO draws are wave 2, S3 to S8). `GB`, `SP` and `MF` are edited by other waves; every edit is at a block named above.

## 7. Tests

| # | Test | Where | Pass |
|---|---|---|---|
| T1 | `ao_core_test` (CPU): defaults of `AoParams` equal §1.2 (bit images of the executable's default vectors); kernel base: eight vectors with the repeated `(0, −1)`, ramp factors `i·0.14284999 + 1` for `SampleOffset ∈ {1.5, 2, 4}`; `SsaoKernel` entries (`d = k·C`, `z` = 0.125) | `Build-GameBootstrap.ps1` | exact |
| T2 | `ao_bank_test` (CPU, gparam directory as argument; a new executable of `Build-GameBootstrap.ps1`, **not** `light_check`, which is built only by `native/world/Build.ps1` into the retired I-0039 tree): key {1,0} → the `mapbase` set of §1.2; an area file without the group falls back to `mapbase`; the booleans through the word reader; weight 0 → A, 1 → B, 0.5 → the lerp column **with the built-in tail (3800 / 200 / 4000, `PreventGhost` 0)**; ε snap; a key absent everywhere → the same fallback chain as `GroupFloat` (`{1, 0}` then the engine bank fallback, `bank+0x198` at the end of `14015AF60`; the stack defaults of `103510` seed only the blend); the gparam files come from a directory argument (not in Git) and the executable links `light_producers` for `Banks` | `Build-GameBootstrap.ps1` | exact |
| T3 | `UpdateAoView`: first frame invalid; identical cameras valid with `cur_to_prev` = the identity mapping of the screen; a 60° yaw invalid; `freeze` leaves the state; against frozen golden matrices for three camera pairs (relative tolerance 1e-6) and the validity algebra `M = inverse(V_cur)·V_prev` on a 44°, a 46° and a 60° yaw (valid, invalid, invalid) | `ao_core_test` | as specified |
| T4 | `PlanAo`: all five quality rows × (`UseNormal`, `UseReProj`, `PreventGhost`, `EnableUpScale`) × (`valid`, `have_normal`) against the table of §1.4, including the counter reset and `rotation_image` sequence 1, 2, …, 15, 0 while temporal and 0 otherwise | `ao_core_test` | every cell equal |
| T5 | the four cores against frozen golden byte arrays (48 / 256 / 288 / 64 bytes) derived once from the snapshot wrappers on a synthetic region set | `ao_core_test` | bit-identical |
| T6 | `native_ao_inputs` row: present, well-formed, `plan.run` false with `no-ao.flag` and with `ao-quality.flag` level 0, strength 1.0 | pilot row check | as specified |
| T7 | pins: the seven blobs accepted, a flipped byte refused softly; blobs from a directory argument (the I0063 programme cache or the extracted bundle; not in Git), skipped with a message otherwise | `ao_stage_test` | refuse is soft |
| T8 | `ao_stage_test` (WARP, then hardware): `Open` with default sampler descriptors creates the root, the sampler table and the five pipelines with no debug-layer message of severity WARNING or worse (the renderer counts them as errors); the targets have the sizes and formats of §5.7 at 1280×720, 1920×1080 and an odd size | new test executable | equal |
| T9 | resize and close: `Resize` recreates the ping-pong pair and the transient targets; `Close`/`Detach` release everything | `ao_stage_test` | no leak, no message |
| T10 | the aids: `ao-quality.flag` level 3 gives the HIGH plan, level 1 the LOW plan | pilot | as specified |

## 8. Live witnesses by family

Nothing is drawn in wave 1, so the witnesses are the inputs and the structure; the image witnesses belong to wave 2 (SSAO-D3D12 §5 S3 to S8). Pilots: `Pilot-LoadGameThomas.ps1 -Teleport "x,y,z" -Look`, `Run-PilotPair.ps1`, `Pilot-NativeWitness.ps1`; rows `native_ao_inputs`, `native_ao_programmes_*`, the AO memory row.

| Family / consumer | Reader | Wave-1 witness | Expected |
|---|---|---|---|
| **Parameters, world** | all | `native_ao_inputs` at the High Wall and the five zones (m31, m37, m45, m50, m51) at HIGH | `mapbase` id 0 set (1.5, 1.2, 0, 10, 0.2, 2, 0.7, 1, 0.02, 0, 100, 200, 4000), identical in the five zones; keys index 8; strength 1.0 |
| **Gate by quality** | all | the plan at DISABLE, LOW, MEDIUM, HIGH, MAX through the options (or `ao-quality.flag`) | DISABLE no run; LOW plain; MEDIUM + upscale; HIGH temporal + normals + upscale; MAX the same here (§1.4) |
| **Sun and `GXLightDeferredAO`** | `t32` (composite of mask and `t33`) | none in wave 1 (white); wave 2 A3/A4 of SSAO-NATIVE | image unchanged |
| **Local lights** (point, spot, line) | `t33` | none in wave 1 | unchanged |
| **Forward programmes** | `t33` | none in wave 1 | unchanged |
| **Temporal** | `SeparatedReProj` | `UpdateAoView` rows over a still camera and over a teleport: `valid` 1 then 0 on the first frame after a large rotation, counter sequence | as T3/T4 |
| **Non-regression** | all | idle High Wall frame digest equal after AO-1..AO-8; `content_draws` equal; `prepare_ms` unchanged; AO resource bytes about 8.3 MB at 720p (row) | equal |

## 9. Risks

1. **The gate was misread by earlier notes** (§1.6): the AO pass runs from LOW and a native plan that skips it there would be wrong; what `t33` holds at LOW is open; `PlanAo` and T4 follow §1.4 and the live row SS9 checks all levels.
2. **The blend tail** (§1.2) shows only during key transitions; a test (T2) pins it, since a plain lerp of all 17 fields would differ from the original at weights strictly between 0 and 1.
3. **`valid` algebra** (§5.2) is now the verified one (`M = inverse(V_cur)·V_prev`); the remaining risk is the float-against-double difference of `cur_to_prev` and which views take which call site of the render step (SS17).
4. **Debug-layer warnings are fatal** (`SceneRenderer::Diagnostics`, `SP:782`) and **`Refuse()` exits the process**: every AO failure is soft, a depth-only pipeline needs `NumRenderTargets 0` and a matching `DSVFormat`, and the sampler table must be set before use.
5. **Memory**: about 8.3 MB at 720p, 18.6 MB at 1080p, 75 MB at 4K; allocated in AO-7 although nothing draws yet (flag `no-ao-resources.flag`).
6. **Shared files** (`GB`, `SP`, `MF`) are edited by phase C, K and the other waves; anchors are block names.

## 10. Open points and live rows (none taken)

| ID | Read | Expected |
|---|---|---|
| SS1 | `[[GXRS+0x5360]]+0x09…+0x0F` and the layer bytes `+0x160…+0x162`, `+0x190` per zone at HIGH | `01 01 01 01 00 00 00`; bytes 1 1 1 1 |
| SS2 | the 17 layer dwords at `layer+0x15C` | the `mapbase` set of §1.2 |
| SS3 | which gparam file and id the keys at `+0x4018/+0x4080` select, and the fallback record content | `mapbase` id 0 |
| SS9 | all five quality levels: the programmes drawn and what `t33` holds at LOW and at MEDIUM without upscale | §1.4 |
| SS13 | `layer+0xA0/+0xA4` (AO size), `layer+0xC` (height), the matrix and validity byte of one record over a still camera and after a cut | `(640, 360)`, 720, identity-like, 1; 0 after the cut |
| SS14 | the DXBC hashes bound for variants 7, 8, 9, 10 and the programme at each `layer+0x228+8v` | §2 |
| SS15 | `M[0][0]`, `M[1][1]`, `M[2][2]` of one record against `UpdateAoView` over a 44° and a 46° yaw | equal |
| SS16 | the rotation image index sequence over 40 frames at HIGH | 1…15, 0, 1… |
| SS17 | which views take site 1 (keys A and B, no normal) and site 2 (A twice, with normal) of the render step | the main view takes site 2 |

Open design points: the exact content of the bank's fallback SSAO record; what `t33` is without the upscale; the consumer of the 320×180 depth (SS6 of the earlier note); the exact `RelativeRotationOk` algebra; the variants 7, 8 and 10 names; the stencil bit `0x10` of the composite (SSAO-NATIVE risk 2) stays outside this note.

## 11. Review log

Independent review by an Opus 5.5 reader (read-only; given the note and the evidence, not conclusions; it re-read the decompiles, the evidence, the executable and the native tree at `3c7bc82a`). One blocking finding and five important ones, all verified and applied; **12 findings in all**. The reader found no problem in: the blend and the copy into the layer (§1.2, defaults read from the executable), the rotation textures against native (§1.5), the constants (§3), the matrix convention of `VelocityCurrentToPrevious` (it matches `from_uv`, `to_uv` and the shader's `dp4`), the strength and `q+9` gates, the quality rows, the `+0x3EB8` writers, and the memory figures.

| # | Severity | Finding | Applied |
|---|---|---|---|
| 1 | blocking | the render step calls `1B1960` from two sites; the note described only the one without normal; at the world site both lookups use key A | §0.1, §1.1 (both sites, SS17), §5.1, §5.4, T2: the native reads A for both keys and applies the blend tail at 0 < t < 1 |
| 2 | important | the half depth is produced by `1CB9D0` (t1, DSV-only), not by `1B30E0` (t0, colour target, unidentified programme in `layer+0x238`) | §1.1, §1.5, §2, §5.7, §5.8 |
| 3 | important | the rotation test is readable: `M = inverse(V_cur)·V_prev`, diagonal ≥ 0.7071; the default matrix is the identity; the layer keeps view and projection apart | §1.3, §5.2, T3, SS15 |
| 4 | important | `PlanAo` variant formula wrong for the separated case | §5.3: `(E && !R) ? (N?10:8) : (N?9:7)` |
| 5 | important | the gate correction was overstated (no upscale at LOW: `t33` possibly not written); prewarm list incomplete | §0.3, §1.4, §1.6, §9.1, §5.7 (seven pipelines, variants 7 and 10 included) |
| 6 | important | native anchors | `GB:4600/4709/5443/2714/2693`, `game_quality_object.cpp:11-13` |
| 7 | minor | only the V/P save depends on `DAT_144962318` | §1.3 |
| 8 | minor | labels: dword 0, 11, 12 names inferred; variants 7, 8, 10 bindings verified | §1.2, §2 |
| 9 | minor | `layer+0x240` is programme slot 3; history copy runs before the AO draw; the upscale reads `*(layer+0x68)`; `g_vDepthRangeParam.z` is `−(Start/Range)` | §1.1, §1.4, §3 |
| 10 | minor | T2 fallback wording and gparam source; T3 compared a function with itself; AO-2 depends on GL-2 | §6, §7 |
| 11 | minor | D3D12: no clears, reset of `AoLayerState` on resize, `SRV→RT` of `AoHalf[cur]`, private root signature | §5.2, §5.7 |
| 12 | minor | corrections in SSAO-D3D12-DESIGN-NATIVE added cells to 3-column tables; `default` id 10; programme set separate from `NativeLightingPrograms` | the inserted corrections are now text appended in the first cell; §1.2, §5.7 |

## Exports and evidence

- Evidence (outside Git): `evidence/I-0068/queries/ssao-ready/` (`v1` vtable slots, `v2` `1B30E0`/`1B3CD0`, `v3` greps of the `+0x60/+0x64` writers, `v4` `1B1960`, `v5` `14015AF60`/`140223780`/`1400FEFA0`, `v6` greps of `+0x3EB8`/`+0xFD0`, `dxbc/` the `DepthScale2x2OutputDepth` disassembly).
- Tools: `Invoke-Map.ps1` with `I0067Map.java` and `I0068Grep.java` (unchanged), `Read-Dxbc.ps1` (unchanged); PE reads of the executable by a throwaway script (kernel vectors, defaults, scalars).
- No native code changed.

## Implementation log (thread « Vague 1 producteurs W5 », branch `claude/friendly-feynman-70213e`, 2026-10-13: W5c)

**AO-1.** `native/renderer/ao_core.*`: `Params` (the 17 dwords, built-in values), `Blend` (140223780), `LayerState`,
`UpdateView` (1401B1960: validity by the view matrices' diagonal >= 0.70709997, the current-to-previous matrix by
`VelocityCurrentToPrevious`, the identity when invalid, freeze, counter), `PlanAo` (1401B26E0: E, R, N, U, the counter
reset, the rotation image, variants 7 to 10); `velocity.h` gained `VelocityViewAndProjection`. T1/T3/T4 (`ao_core_test`,
run by the build with the executable): dwords 4-11 equal the executable's 143D80070/143D7F5C0, dwords 14-16 the immediates;
the blend (A, B, lerp with the bytes from B and the built-in tail); the view record (first frame invalid with the
identity, identical cameras valid with the identity mapping within 1e-5, yaws 44/46/60 degrees valid/invalid/invalid,
freeze); the plan for DISABLE, LOW, MEDIUM, HIGH, MAX, without PreventGhost, an invalid record, strength 0, and the rotation
sequence 1..15, 0. Not done here: the kernel base and ramp and the four constant cores (AO-3) move to W8 with the AO stage
that reads them (the snapshot producers stay the oracle). **AO-2.** `Banks::SsaoAt` (the 18 named parameters into the 17
dwords, through `GroupWord`); T2 in `glow_bank_test`: default id 0 and id 10 equal the I-0067 dump word for word, {-1, -1}
as {1, 0}. **AO-4.** `CollectNativeAoInputs` (main view only; key A at +0x4018 blended with itself at +0x40C8, strength
[[GXRS+0x5368]+0xB0], the quality bytes from the option snapshot, freeze DAT_144962318, the layer state kept across
frames and cleared when the AO is off; `no-ao.flag`): `NativeSelectedScene::ao`, `NativeModelFrameInput::ao`, row
`native_ao_inputs`. Rows SS1-SS3 (Reference1517, observer of `FUN_1401B26E0`): in game the layer record is mapbase id 0
(4000; bytes 1, 1, 1; 1.5, 1.2 (0x3F999996), 0; 10, 0.2, 2; 0.7, 1, 0.02; 0, 0; upscale 1; 100, 200, 4000), the view record
valid with strength 1, the quality bytes HIGH (09-0C set). Native1518: the same record, valid, strength 1, the plan E, R,
N, U with variant 9 (WithNormal then SeparatedReProj and the upscale, the observed HIGH chain). Next: W8 (AO-3, AO-5 to AO-8).

## Implementation log (thread « Source du soleil puis W6 », 2026-10-13: W8, AO-3 and AO-5 to AO-7)

**AO-3.** The four constant blocks on native inputs in `light_producers` (`AoSsaoParam`, `AoKernelBase`, `AoSsaoKernel`,
`AoDownsampleOffsets`, `AoSsaoParam2`: the layer's 17 record dwords and sizes, the pass's projection and view rows, the view
record's matrix and strength; the executable constants as the verified values of section 3). `ao_constants_test` (T5, run by
the build with the executable): cbSSAOParam for 1B33D0 and 1B38D0, the kernel block for three sample offsets and cbSSAOParam2
equal the snapshot producers byte for byte on synthetic regions (those read their constants from the executable through
Rdata); the kernel lengths follow the ramp and repeat (0, -1); the downsample taps. The order of the two products of the
ramp is INFERRED (no oracle reads the kernel table itself). **AO-5.** `AcquireNativeAoPrograms`: the quad vertex programme
and the seven pixel programmes from the engine's loose repositories, each against its pin (`ao_pins`: bytes and SHA-256 of
section 2, checked against the I-0063 cache); missing ones retried, a mismatch disables the stage softly. **AO-6.**
`renderer/ao_stage`: the private root (pixel root CBVs b0-b2, one SRV table t0-t3, **static** samplers s0-s4 built from the
bank rows 0, 1, 3, 4, 6, an Open refusal when a border colour is not one of the three static ones: a deviation from the
sampler table of 5.7, the rows being constant across the quality levels), the seven pipelines prewarmed at Open (the
depth-only downsample with `DSVFormat D32_FLOAT_S8X24_UINT`, depth ALWAYS with writes, no colour target; the four variants,
the separated reprojection and the upscale on RGBA8), the quad and the targets (half depth `R32G8X24_TYPELESS` with DSV and
SRV, raw, the ping-pong pair, the full output; created in their between-frame states, never cleared). `ao_stage_test` (WARP
and hardware, debug layer): T7 pins and a flipped byte, T8 the pipelines and the targets at 1280x720, 1920x1080, 1281x721,
T9 resize, close, soft refusals, no message. **AO-7.** The frame's `NativeAoInput` carries the programmes and the bank
samplers when the plan runs and `no-ao-resources.flag` is absent; `SceneRenderer::AoReady` opens the stage at the
presentation extent on the recording thread, `Resize`/`Close`/quarantine follow the glow's; row `native_ao_resources`.
Nothing records an AO pass: the passes are wave 2 (5.8). **Live** Native1588 (Shebang Gamma, m37, HIGH): programmes
resolved and pinned, `native_ao_resources` open with 8,294,400 target bytes (5.7's 8.3 MB at 720p), the plan WithNormal then
the temporal pair once the record is valid (variant 9, E, R, N, U), 0 debug errors, the same pass refusals and the same
image as Native1586. **Not done:** AO-8 (the `ao-quality.flag` and `ao-verify.flag` aids), left for wave 2 with the passes.
**Review (fresh Opus 5.5, merge).** Applied: the eight kernel base vectors, the ramp factor and the tap constants are now
read from the executable in `ao_constants_test` (32 checks); a static sampler only needs a static border colour when it
uses a border address mode; `AcquireNativeAoPrograms` copies and hashes only once all eight are found. Recorded for wave 2:
the AO variants index `cbSampleOffset` dynamically up to entry 8 (`CB1[9]`), so the pass must upload the 256-byte block
zero-filled past the 128 bytes `AoSsaoKernel` writes; the stage's first open builds seven pipelines on the recording thread
(a one-time cost) and keeps 8.3 MB at 720p while nothing draws; "image unchanged" rests on the screenshot and equal
`content_draws` (920), not a frame digest.
