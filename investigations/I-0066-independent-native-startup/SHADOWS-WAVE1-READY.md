# Shadows of wave 1: cascades, local-light shadows and filtering (producers, state, resources and constants, no draw), ready to implement (static)

Request (coordinator, 2026-10-11): prepare the "shadows wave 1" sheet the way the glow, SSAO, YEBIS and effects sheets were prepared ([GLOW-WAVE1-READY](GLOW-WAVE1-READY.md), [SSAO-WAVE1-READY](SSAO-WAVE1-READY.md), [YEBIS-WAVE1-READY](YEBIS-WAVE1-READY.md), [FFX-WAVE1-READY](FFX-WAVE1-READY.md)): cascades, local-light shadows and filtering, with the witnesses of the five zones; merged after an Opus 5.5 review. It details items S1, S2, S3 and S5 of [WAVE1-PREP](WAVE1-PREP.md) §2.3 and extends them to the local lights and the mask filtering (S4, the casters, stays wave 3; the mask and composite draws and the forward receive are wave 2).

Labels: **verified** = read in the executable, in the retained gparam dumps or in the native tree at main `933a9431`; **observed** = read in a retained live capture or note; **proposed** = this sheet's design; **inferred** = deduced, not read; **open** = needs a read or a measurement first. Nothing was launched, no native code changed. Abbreviations: `GB` = `native/frame/game_bootstrap.cpp` (line numbers move; anchors are function names), `LP` = `native/world/light_producers.cpp`, `SP` = `native/frame/scene_pass.cpp`. Evidence outside Git: `evidence/I-0068/queries/shadow-ready/` (`a`..`w` exports of the local-light, container, allocator, fit and filter functions; `shadow-world-blocks.tsv` the goldens of §7), `evidence/I-0068/queries/shadow/shadow-atlas-draws-i63-live-10.tsv` (the observed caster draws), `evidence/I-0068/queries/gparam/` (ShadowParam of `default`, `mapbase`, templates).

Sources, read again for this sheet: [SHADOWS-NATIVE](../I-0068-static-yebis-ffx-lights-spec/SHADOWS-NATIVE.md) (all of it), [TESS-SHADOW-FORWARD-NATIVE](../I-0068-static-yebis-ffx-lights-spec/TESS-SHADOW-FORWARD-NATIVE.md) §4, [TESSSCENE-SSMASK-NATIVE](../I-0068-static-yebis-ffx-lights-spec/TESSSCENE-SSMASK-NATIVE.md) §2, [D3D12-MASTER-PLAN](../I-0068-static-yebis-ffx-lights-spec/D3D12-MASTER-PLAN.md) §2, §6 S5, [QUALITY-OBJECT-NATIVE](../I-0067-static-menus-subsystems-map/QUALITY-OBJECT-NATIVE.md), `native/world/light_producers.cpp` (`Banks::Shadow`, `ShadowFieldsFromBanks`, `PerScene`, `BlurParam`, `ShadowInfo`, `SpotShadowInfo`), `native/frame/game_quality_object.cpp`.

## 0. Answer first

1. **Wave 1 for the shadows draws nothing; it builds the data every later shadow pass reads.** Seven pieces, in this order: the gates (S1), the world record `R` and its clamps (S2a), the cascade fit and culling volume as engine CALLs on native-built records (S2b), the constants `cbPerScene`, `cbBlurParam` and the shadow words of the 960-byte `cbSceneParam` (S5), the cascade atlas resource, its four viewports and per-cascade bias (S3), the **local-light shadow selection and tile allocation** (new, §3) and the **filter selection table** (new, §4). The casters (S4) are wave 3, the mask, composite and sun draws wave 2.
2. **Every world zone uses the same ShadowParam record.** The seven zone files carry no `ShadowParam` group, `Banks::Shadow` falls back to `mapbase` id 0 (verified, `LP:1301-1325`): FilterMode **2**, size class **3**, splits 2 / 8 / 15, fade 15.1 to 65.1, blur count 1 radius 2, DepthRate 0.1, DepthScale 10, KernelScale 1, shadow colour opaque black. The zones differ by the **sun direction** (`Shadow Light Id` is 0, not −1, so the direction is light 0 of each zone's LightSet, `FUN_1400F90F0`, which every zone file carries), by what the engine casts into the cascades (observed draw counts, §5), by their local lights and by their quality settings; the golden words are in `shadow-world-blocks.tsv` for atlas edges 2048, 4096 and 8192.
3. **The atlas edge follows the quality row, and the reference setting gives the observed 8192².** `FUN_1401B5720` verified: class size (0 → 2048, 1 → 1024, 2 → 2048, 3 → 4096), × 2 only for the double-size object, then `S = clamp(num·size / den, min, max)` with the quality row `(on, num, den, min, max, blurAdd)` = DISABLE `0 1 4 128 1024 −4`, LOW `1 1 4 128 1024 −4`, MEDIUM `1 1 2 128 2048 −1`, HIGH `1 1 1 128 4096 0`, MAX `1 2 1 128 8192 2`. With class 3: 1024 / 1024 / 2048 / 4096 / **8192**; each cascade occupies a quadrant of `S/2`; the blur count is `R.BlurCount + blurAdd` clamped at 0 (MAX: 3).
4. **The cascade fit, the culling volume and the matrix assembly need no transcription and touch no GX object.** `FUN_140239610` (the fit), `FUN_140238B40` (single-map variant), `FUN_14023C1E0` (caster culling volume) and `FUN_1402388B0` (matrix assembly) were searched for `[144961AB0]`/`[144961BB0]`: none (reviewer-confirmed). Three neighbours are **not** pure: `FUN_14023A0C0` (the constants builder) reads `[GXRS+0x4DD8]` and calls `(+0x730)->vt[0x40]` (the dither-texture size) while `PublishNativeSceneRoot` requires `+0x4DD0/+0x4DD8` to be zero; `FUN_1401B5720` calls `FUN_1401B55F0`/`FUN_1401B5390` (GX resources) and the virtual locks `+0xC10` as soon as the size changes; `FUN_140238680` (the dispatcher around the fit) registers the GX clear `FUN_1419201A0` when `+0x120 && +0x100`. So: CALL the four pure functions on native-built records; transcribe the clamps and sizes of `1B5720` and the words of `23A0C0` as native pure functions (the sizes and words exist already as `ShadowFieldsFromBanks`); never call the three impure ones. What stays open is the producer of the light direction and the exact input records (§2.3).
5. **Local-light shadows are decoded from the gate to the recording (new), and the objects the slot acts on do not exist natively.** The gate is in `FUN_14010A220` (lines 116-126, and `FUN_1401094F0` 598-608): `pipeline+0x78 == 1 && pipeline+0x3EBF && [[GXRS+0x5360]+0x44]` (the quality Lighting byte, 1 from MEDIUM). Slot 3 of `GXDrawPointLight` (`FUN_140212C40`) and of `GXDrawSpotLight` (`FUN_140214350`) acts on the **frame items** of the lights (0x120 bytes for a point light, built each frame by `FUN_140212810` from `GXPointLight` slot 6 `FUN_1400EFF50`; 0x280 bytes for a spot, `FUN_1400F1010` → `FUN_1402136B0`), sorted by `+0x10` descending (`FUN_1400FF3B0`) before slot 3 runs — so tile allocation order depends on that sort. The metric is `item+0x9C = max(viewZ − r, 0.1)·[ctx+0x280]/(2r)` (spot `+0x260`: `/[light+0x1E8]`) and the cast flag is `light+0xB1 && light+0x94 > 0`. Slot 3 decides (a level 0..3 from three thresholds, metric ≤ 15.0), allocates tiles from a **quad-tree allocator** (`FUN_140242460`, `[GXRS+0x41D8]+0x10`) and appends items of **0x1850 bytes** to the container vector `+0x260` (capacity 48); `FUN_14010C110`/`FUN_14010AFE0` record one scene pass per item (pass `i + 9`, indices 0..47). The container is **already built natively** (`PublishNativeSceneRoot`, lighting-children entry `{0x18, 0x2F8, 8, 0xB24E0}`, `GB:9883`); `FUN_1400B27A0` (allocator + atlas, config 512 / 256 / 1024 by default) is not run. Native's `CollectNativePointLights` transcribes `EFF50`/`212810` **without building the 0x120 items** and ignores spots (`spots_open`), so SW-7 must build the items (call `212810`/`2136B0` on a native context, or transcribe them).
6. **Filtering is a table, not a state machine.** The mask draw selects the cascaded programme by `FilterMode`: 0 (and any value ≥ 3) → `PCF4`, 1 → `PCF9`, 2 → `Soft` (verified by the reviewer in `FUN_1D6D20` lines 363-371 and the name chain of `FUN_1D76B0`). The world record says **2 = `GXScreenSpaceCascadedShadow_Soft`**, whose extra inputs are the record words `DepthRate`, `DepthScale`, `KernelScale` (`cbSceneParam` `+0x230`) and a dither texture. The blur runs when `[[GXRS+0x5368]+0x128] × record Blur Radius > 0` **and the effective blur pass count (`obj+0x5C`, written by `1B5720`) is non-zero** (`FUN_1D7850` line 76); the parity of that count chooses the mask target (`+0x10` if odd, `+8` if even: `FUN_1B8F30`). The forward rows take `Fwd`, `FwdSdw` or `FwdSSdw` from a per-pass window table (SHADOWS §7), which wave 1 expresses as a pure `ShadowFrame` table.
7. **Nine commits SW-1..SW-9** (§6), every rung opt-in by a flag file until its exit holds; nine tests T-W1..T-W9 (§7); sixteen live rows SL1..SL16 (§8). The two large unknowns are named, not hidden: the layout of the cascade-fit input records (§2.3) and the producers of the pipeline gate and variant (§2.1); both have a static route and a live row.
8. **Six earlier statements change** (§10): SHADOWS §0 "no input exists" (the fit, culling volume and clamps are CPU), SHADOWS §1.1 "4096² atlas of 512² tiles" (edge and tile base are configuration values, defaults 1024 / 512 / 256), WAVE1-PREP S2 "transcribe or CALL on a native `GXCascadedShadowScene`" (neither), the quality-row wording of MASTER-PLAN S5, the local-light eligibility (the slot acts on frame items, not on lights) and the face count/field of view of the cube faces.

## 1. What the original does (verified)

### 1.1 Frame structure (summary of SHADOWS-NATIVE §1, §6, §7)

`FUN_140103510` records the world pipeline. Under the **shadow gate** (`pipeline+0x3EBC` and `[[GXRS+0x5360]+0x28]`, an absent object counting as on) it records the cascade layer (`GXCascadedShadowScene::FUN_1401B6B80`, commands 0..6), the screen-space mask layer (`GXScreenSpaceShadow::FUN_1401B8F30`) and, for every shadow-casting local light, one scene pass through `FUN_14010C110`. A closed gate clears the mask to white and records no window, so `view+0x710` stays 0 and every forward row is `Fwd`. The cascade cache (SHADOWS §1.4: keep a cascade when the new slice stays inside the previous volume, reprojection mode, forced invalidation bytes `[14496231C+i]`) is an optimisation the native reference does not need.

### 1.2 The ShadowParam record and the world key (verified)

`R` (0x94 bytes, `Banks::Shadow`): `Angle` +0x00 (2 words), `Shadow Light Id` +0x24, `Near` +0x34, `Far Fade Start` +0x38, `Far Fade Dist` +0x3C, the three cascade distances +0x40/+0x44/+0x48, `FilterMode` +0x50, `Depth Offset` +0x54, `Volume Depth` +0x64, `Slope Scaled Depth Bias` +0x6C, `Cull Flip` +0x70, `AlignTexel` +0x71, `DepthRate` +0x74, `DepthScale` +0x78, `KernelScale` +0x7C, `DepthOffsetScale` +0x80, `Blur Count` +0x84, `Blur Radius` +0x88, `Shadow Color` +0x8C (ARGB), `Shadow Map Resolution` +0x90 (size class); words not in the file keep the `1400839F0` defaults (`+0x5C` 0.7, `+0x60` 1.0, `+0x68` 0.005, `+0x6C` 3.0 before the file overrides it). The world key is `{map area, 0}`; the zone files carry no group, so the lookup lands in `mapbase` (the same record at every zone); the menus use `{1, id}` of `default` (ids 0, 10..15).

| Word | `mapbase` id 0 (world) | `default` id 0 (menus) |
|---|---|---|
| Near | 0.1 | 0.1 |
| Cascade distances 0→1, 1→2, 2→3 | 2, 8, 15 | 1, 1.5, 2 |
| Far Fade Start / Dist | 10 / 50 | 1 / 10 |
| FilterMode | **2** | 1 |
| Depth Offset / Slope Scaled Depth Bias | −0.006 / 0.005 | −0.001 / 0.005 |
| DepthRate / DepthScale / KernelScale / DepthOffsetScale | 0.1 / 10 / 1 / 0 | 1 / 5 / 1.5 / −0.085 |
| Blur Count / Blur Radius | 1 / 2 | 0 / 2 |
| Resolution class | **3** | 0 |
| AlignTexel / CullFlip | 1 / 0 | 1 / 0 |

### 1.3 Sizes, splits and the state clamps (verified, `FUN_1401B5720`)

Class size → quality scale → clamp (§0.3); a change of `(width, height)` at `obj+0xC00/+0xC04` re-creates the atlas (`FUN_1401B55F0` then `FUN_1401B5390`, GX resources: not called natively). The state: `+0xA70` = Near; `+0xA7C` = max(Near + ε, c0); `+0xA80` = max(c0 + ε, c1); `+0xA84` = max(c1 + ε, c2); `+0xA74` = max(c2 + ε, FadeStart); `+0xA78` = FadeDist, with ε = `[143D7ED70]` = 0.1. The four cascade records (`+0x8F0 + 0x50·n`, copied from the record, blur count in each) feed the per-cascade rasterizer bias (the conversion to a D3D12 depth bias is open, §9.4). For the world record: near 0.1, splits 2 / 8 / 15, fade start 15.1, fade end 65.1.

### 1.4 The constants (verified, with the native producers that already exist)

`ShadowFieldsFromBanks` (`LP:233-257`) derives the `cbSceneParam` shadow words from `R`: `+0x1C0 = (fadeEnd, 1/max(fadeDist, 1e-6), ½/S, ½/S)` (`FC_ShadowMapParam`), `+0x1D0 = (ca·(1 − cr), ca·(1 − cg), ca·(1 − cb), R+0x68)` (`FC_ShadowColor`), `+0x230 = (DepthRate, DepthScale, KernelScale, 1/randomTexture)`, `+0x240 = (1/S, 1/S, R+0x80, 0)`, `+0x250 = (1, d0/den0, d0/den1, d0/den2)`; `PerScene` (`FUN_1D6D20`, 336 bytes: inverse pass matrix, four `camera pose × cascade matrix`, `CascadeSelectDist`); `BlurParam` (`FUN_1D7850`). They are all expressed on engine-state **snapshots** today (`ShadowState`, `ShadowFields`); wave 1 re-expresses them on a native `ShadowState` (S5, §2.4).

### 1.5 The local-light shadow chain (verified; new)

| Link | What | Evidence |
|---|---|---|
| Container | `[GXRS+0x41D8]`, built by `FUN_1400B24E0(this, heap)` (called from the root init `FUN_140048B80`): item vector `+0x260/+0x268/+0x270` of **0x1850-byte items**, capacity `+0x280` = **0x30 (48)**; quad-tree allocator at `+0x10` (initialised by `FUN_140242340(this+0x10, {edge, edge>>5})` inside the config setter), atlas resource at `+0x1F8`, "ready" byte `+0x258`, frame counter `+0x284`, mode byte `+0x288` (copied each frame from `[GXRS+0x4E1A]` by `FUN_140023790`), config dwords `+0x28C..+0x2B4`: spot base tile `+0x28C` (default 0x200), `+0x290..+0x29C` = 2, 4, 6, 10, point base tile `+0x2A0` (default 0x100), `+0x2A4..+0x2B0` = 2, 2, 2, 2, atlas edge `+0x2B4` (default 0x400) | `shadow-ready/v/1400b24e0.c`, `1400b27a0.c` |
| Config | `FUN_1400B27A0(container, desc[11])`: allocates the allocator and the atlas (`FUN_1400DCB80`, size `{edge, edge}`, format code 0x37 = D16_UNORM) and stores the eleven config dwords; the root init fills `desc` from the config block: spot tile = `cfg[1]` else 0x200, point tile = `cfg[0]` else 0x100, edge = `cfg[2]` else 0x400, the four values 2/4/6/10 and the point values 2/2/2/2 constant | `shadow-ready/w/140048b80.c` lines 271-291 |
| Eligibility, point | `GXDrawPointLight` slot 3 `FUN_140212C40(item, camera, container)` acts on the **0x120-byte frame item**: needs byte `+0xA4` (shadow cast) and metric `m = [+0x9C] ≤ 15.0` (`[143D7EF60]`); level = 3, or 2 if `m < 2.0` (`[143D7EE7C]`), 1 if `m < 0.4` (`[143D7EDAC]`), 0 if `m < 0.1` (`[143D7ED70]`); far = radius `[+0x7C]`, near = far × 0.001 (`[143D7ED10]`); calls `FUN_1400B2DC0(container, +0x80, camera+0xD0, near, far, level, +0xBC, flag +0x14, [+0x18])`. The item is built each frame by `FUN_140212810` (line 193: `+0x9C = max(viewZ − r, 0.1)·[ctx+0x280]/(2r)`) from `GXPointLight` slot 6 `FUN_1400EFF50`; the cast flag is `light+0xB1 && light+0x94 > 0`. Items are sorted by `+0x10` descending (`FUN_1400FF3B0`) and slot 3 is called in that order only under the gate of §1.1 (`FUN_14010A220` l. 116-126) | `shadow-ready/l/140212c40.c`, `f/14010a220.c` |
| Eligibility, spot | `GXDrawSpotLight` slot 3 `FUN_140214350` on the 0x280-byte item built by `FUN_1402136B0` (from `FUN_1400F1010`): byte `+0x1C8`, metric `[+0x260] ≤ 15.0` (`FUN_1402136B0` line 676: `max(…, 0.1)·[ctx+0x280]/[light+0x1E8]`), thresholds 0.08 (`[143D7ED6C]`), 0.3 (`[143D7EDA0]`), 2.0; calls `FUN_1400B29D0(container, +0x1D0, level, rect[4], …)` | `shadow-ready/l/140214350.c` |
| After a hit | the light stores the item at `+0xB0` (point) / `+0x1C0` (spot) and the frame counter, sets its **programme variant id** `+0x1C` (point: 0x6D, or 0x66 when it was 0x65; spot: one of 0x68, 0x6F, 0x6A, 0x71 by flags `+0x1C9`, `+0x1CA`), and the projection words `+0x50 = f/(f−n)`, `+0x54 = −n·f/(f−n)` | same |
| Allocation | `FUN_1400B2DC0` (point; the six face tiles of `[+0x2A0] >> level` are always allocated; refuses when the item count + 6 exceeds `+0x280`; with `container+0x288 == 0` it appends **six single-face items** (`+0x378` = the face index 0..5), with `≠ 0` one item with `+0x378 = 6`) and `FUN_1400B29D0` (spot, one tile of `[+0x28C] >> level`, retrying with the next smaller level up to 3, `+0x378 = 0`); both through `FUN_140242460(allocator, rect, size)`: size rounded up to a power of two, level from `root/size`, a free node found by `FUN_140242590` (search `FUN_1402426C0`, all CPU), rectangle `{x0, y0, x1, y1}` in atlas texels | `shadow-ready/t/`, `k/`, `l/` |
| Item | 0x1850 bytes: six face poses (stride 0x90 from `+0x20`), `+0x378` face index/count as above (`14010AFE0` tests `== 6`), `+0x360/+0x368/+0x370` target handles, `+0x37C/+0x380` size and clear, per-face records (`+0x3C0 + 0x2E0·n`), `+0x17E0` twelve qwords of face rectangles, `+0x1840/+0x1844` flags. Face cameras: the field of view is **not π/2** but `2·atan(h/(h − 3))` with the face rectangles reduced by 3 texels (`[143D7EEB8]` = 3.0, reviewer-found), the tile aspect, near and far above; `FUN_14004B5C0` (called by `B29D0`, `B2DC0`, `6E780`) reads `[144961AB0]+0x88` under a non-null guard, so natively the screen extent of the face cameras is 0 | `shadow-ready/c/14010afe0.c`, `k/1400b2dc0.c` |
| Recording | `FUN_14010C110(pipeline, …)` (called from `FUN_140103510`) walks the items; `FUN_14010AFE0(item i)` names the pass `LightShadow<i>`-style (`[143D4F1A0]`), pass id `i + 9`, stops after index 47 (`0x38 < i + 9`, i.e. 48 items); builds the face views (`FUN_14010C7E0` uploads the face matrices to `[[GXRS+0x4DD0]+0x330]` and the seven face buffers at `+0x340 + 8·i`), clears, draws the casters through the cascade-style scene callback, and with `DAT_144961BF5 ≠ 0` draws the cube in one pass (`FUN_140228A70`; the label `OnePass CubeShadow`). A job variant (`GXDynamicLightShadowTask`, vtable `143D4F5B8`) does the same on the GX job pool when `DAT_14473CE5A` and `DAT_14473CE59` are both non-zero. Per frame the engine first runs `FUN_140023790` (copies `[GXRS+0x4E1A]` into `container+0x288` and calls `FUN_1400B3B20`, which empties the item vector and re-initialises the allocator when `+0x258 == 1`) | `shadow-ready/d/14010c110.c`, `b/14010bfa0.c`, `f/140023790.c` |
| Reading | the lighting passes read the tiles as `t7..t10` (`g_LightShadowTexture[]`), `s12`, `t33`, and `cbLightAccShadowInfo` `{w, h, 1/w, 1/h}` per bound map (`1A55B0` point batches of up to four lights, programmes `DPointLightShadowed1..4`, `DSpotLightShadowed1..3`; producer `ShadowInfo`/`SpotShadowInfo`, exact on 660 of 664 batches) | `LP:75, 121, 133`; `decompiled/…001A55B0` |

### 1.6 The mask and its filters (verified interfaces; the selection order is inferred)

The mask layer records the draw for mode word 0 (clear to 1), 1 (single map) or 2 (cascaded); the cascaded programmes are `GXScreenSpaceCascadedShadow_PCF4`, `_PCF9`, `_Soft` (set slots `+0x38`, `+0x40`, `+0x48` of the programme set built by `FUN_1401D76B0`), the single-map ones `GXScreenSpaceShadow_PCF4`, `_PCF9` (`+0x28`, `+0x30`), the blur `GXScreenSpaceShadow_Blur` (`+0x50`), the vertex shader `+0x20`. `FilterMode` 0 (and any value ≥ 3) selects `PCF4`, 1 `PCF9`, 2 `Soft` (`FUN_1D6D20` lines 363-371 and the name chain of `FUN_1D76B0`, reviewer-verified). `PCF4` and `PCF9` read t1, t32 (the atlas), `s12`, `cbPerScene`, `cbSceneParam`; `Soft` adds t38 (dither), `s6`, `FC_ScreenSize`, the depth rate, scale and kernel words and the texel scales. The blur runs when `[[GXRS+0x5368]+0x128] × record Blur Radius > 0` and the effective blur pass count (`obj+0x5C`) is non-zero (`FUN_1D7850` line 76; the parity of that count selects the mask target `+8`/`+0x10` in `FUN_1B8F30`); it reads `cbBlurParam` (`{g_vDepthComputeParam, g_avSampleOffsets[16]}`, 12 taps `{x/W·s, y/H·s, gauss}`). The mask target is four 8-bit channels, the shadow term in `w` (TESSSCENE-SSMASK §2). `GXRend_CompositeShadowMask` writes t32 = shadow × AO for the lighting with `cbCompositeShadowMask` `{FC_Mult, FC_Add}` (executable constants per upload site).

## 2. Cascades: design (proposed)

### 2.1 Gates (S1)

`CollectNativeShadowGates(scene, view)`, called beside `CollectNativeVelocityInputs`, returns `{shadowsOn, variant, mapOverride, localEnabled}`:

- `shadowsOn` = the quality object byte `+0x28` (**readable now**: the object is built natively since Native1255, `game_quality_object.cpp`; absent object counts as on);
- the pipeline gate `pipeline+0x3EBC`, the variant `iVar28` and the override `pipeline+0x2DA8`: the native process has **no GX pipeline object**. Proposed source (rungs, each recorded in `native_shadow_gates`): (a) the world main view has the shadow flag set (the pipeline is built with it for `pipeline+0x7C == 3`, SHADOWS §7), variant 0, no override; the menu views are not in wave 1; (b) the values read at the original at the High Wall and m40 (row SL1) replace the assumption. A wrong guess changes only which windows the `ShadowFrame` table opens, so it is detected by the forward-row mode row (SL14), not by a crash;
- `localEnabled`/mode byte `[GXRS+0x4E1A]`: **not zero natively** — the root constructor `FUN_1400213A0` (run by `PublishNativeSceneRoot`) stores `+0x4E1A = 1` (instruction `140021469`). But `FUN_140023790` (the per-frame copy into `container+0x288` and the call of `FUN_1400B3B20`) does not run natively, so `container+0x288` stays 0 (the constructor value) and diverges from the original, and the item vector is never reset: SW-7 must do both each frame (copy the byte; empty the vector and the allocator). Row SL10 expects 1.
- the **local-light gate** (new in this version): `pipeline+0x78 == 1 && pipeline+0x3EBF && [[GXRS+0x5360]+0x44]` (`FUN_14010A220`, `FUN_1401094F0`); the quality Lighting byte `+0x44` is natively readable (1 from MEDIUM, MENUS-MAP 7781); the two pipeline fields have the same status as `pipeline+0x3EBC` (assumed, row SL1).

### 2.2 The record, the sizes and the state (S2a)

`ShadowRecord` = `Banks::Shadow(area, id)` moved out of `native/world` into `native/frame` (a thin re-export of the same parsing, same tests), plus a pure `ShadowSizes(R, qualityRow)` and `ShadowStateClamps(R)` that transcribe §1.3 (the clamps already exist as `ShadowFieldsFromBanks`; the sizes are new). Outputs: `R`, `atlasEdge S`, `cascadeEdge S/2`, `blurCount`, the state words. Golden: `shadow-world-blocks.tsv` (§7). The produced words feed the cascade fit (§2.3) and the constants (§2.4).

### 2.3 The cascade fit and culling volume (S2b): CALL on native records

Verified: `FUN_140239610(out16 viewProj, out matrix2, camera, lightDir4, texelScale, size, alignTexel, blend)` computes the 8 world-space corners of the slice frustum from the camera record (`FUN_14006EF80(camera, corners)` uses the record's matrix words `+0x50..+0x8F`), builds the light basis from `lightDir` (`FUN_140238020`), the bounding extents (sphere radius `r`, plus `_DAT_14496410C·r/size` margin), optionally snaps the centre to the texel grid (`alignTexel`: `floor(x·size/extent)`) and writes the orthographic view-projection (`1/(far − 0)` depth scale; `blend` ≠ 1 interpolates two candidate bases). `FUN_14023C1E0(cullObj, camera, lightDir, near, far)` builds the culling planes and bounds (`+0xD0/+0xE0` min/max, `+0xF0` plane count, planes from `+0x10`). `FUN_1402388B0` multiplies the matrices of the shadow-map object. None references a GX device object.

Proposed: native builds the **inputs** and calls the genuine functions. Reviewer-corrected field attribution: `FUN_14006EF80(camera, corners)` reads the camera record at `+0x140/+0x144/+0x148/+0x14C` (field of view, distances, aspect), while `FUN_140239610` itself reads the pose at `+0x50..+0x8F` and receives size, AlignTexel and the `+0x2C` scale as **arguments**; the shadow-map object fields `+0x08` (mode), `+0x28`, `+0x120/+0x121` (half-size flag), `+0x100` (target) are read by the dispatcher `FUN_140238680`, which is **not** called (§0.4). `param_5` ("texelScale") is added to the depth extent and used as a `1/x` depth scale: it is more likely a depth range (inferred). The camera record layout is therefore largely readable statically. The **light direction** `lightDir` comes from `FUN_1400F90F0`/`F9B50` (SHADOWS §1.2.4): the world record has `Shadow Light Id = 0` (not −1), so the direction is the light 0 of each zone's LightSet (present in every zone file), not the record's `Angle`; its native producer is not designed here (open, row SL4). Then store the matrices at `ShadowState +0xAF0 + 0x40·n`. **Open:** the exact field list is settled by a live snapshot (row SL4); until then the CALL runs only inside the test process against the oracle (T-W3). The reference behaviour stays "redraw every cascade every frame with all casters" (SHADOWS §1.4); the keep decision `FUN_1401B5B40` (no GX access, tail not recovered) is an optional later optimisation.

### 2.4 The constants (S5)

`ShadowState` (native struct mirroring `+0xA60..+0xAEF` of §1.4) and a `SceneViewInput` (camera pose, view-projection) replace the snapshots in `ForwardFields`, `PerScene`, `BlurParam`, `SceneImage`/`ViewWvp` and `ShadowFieldsFromBanks`; the full 960-byte `cbSceneParam` image is produced per view (SHADOWS §12.2: the shadowed programmes index the cascade matrices dynamically, so any unproduced word refuses the draw). The Region-based originals stay as oracles; each re-expressed producer is re-qualified word for word against the engine's block (row SL5).

### 2.5 The atlas, the four viewports and the bias (S3)

`SceneRenderer::NativeShadowResources()`: one `R16_TYPELESS` placed resource of `S × S` (D16_UNORM DSV, R16_UNORM SRV), cleared to 1.0; the viewports `{(0,0), (S/2,0), (0,S/2), (S/2,S/2)}` of size `S/2`; one DSV; states `DEPTH_WRITE → PIXEL_SHADER_RESOURCE` with the helpers of `SP:66-74`. Memory: 128 MiB at MAX (S = 8192), 32 MiB at HIGH. Flag-gated inert resource first (SW-6a), viewports and clears second (SW-6b). Per-cascade PSO rasterizer: constant and slope-scaled depth bias from the cascade records (`Depth Offset`, `Slope Scaled Depth Bias`, scaled by the atlas format): the conversion the engine applies in `FUN_1401B5390` (`+0xA38..` rasterizer states) is **open** (row SL3 reads the four states' bias values); the placeholder is the record values with the D16 scale `1/65535`, marked as a guess. **Closed by SL3 (SW-6b log): no constant bias; slope-scaled bias = R+0x6C; cull none; depth clip on.**

### 2.6 `ShadowFrame` (forward-row modes and windows)

A pure table `ShadowFrame { gateOpen, variant, for engine pass p: mode(p) ∈ {Fwd, FwdSdw, FwdSSdw}, atlasSrv, maskSrv }` computed once per frame from the windows of SHADOWS §7: closed gate → every pass `Fwd`; open gate, variant 0 → cascade window [0, 0x3C] (state 1) and screen-space window [2, 3] (state 3, then back to 1); variant ≠ 0 → cascade windows [0x17, …] and [0x1E, 0x2E], screen-space windows [0x17, 0x1B] and [0x1E, 0x21]; after the cascade end the state is 0 (rows from pass 0x45 are `Fwd`). Forward pass 0x29 (41) is inside the cascade window in both variants. Test T-W5 checks the table against these ranges; row SL14 compares it with the original's `view+0x710` per row.

## 3. Local-light shadows: design (proposed)

1. **Container (SW-7a).** The container exists: `PublishNativeSceneRoot` builds `[GXRS+0x41D8]` with the genuine `FUN_1400B24E0` (size 0x2F8) with the other lighting children, and does **not** run `FUN_1400B27A0` (its atlas creation is GPU). Wave 1 adds the CPU half of that function natively: `FUN_140242340(container+0x10, {edge, edge>>5})` (allocator), the eleven config dwords `+0x28C..+0x2B4` from the engine's own configuration (`Draw.SpotLightShadowmapResolution` 512, `Draw.PointLightShadowmapResolution` 256 defaults; the link from these option strings `14283D300/350/3A0` to `cfg[0..2]` is inferred, not shown; the reference setting gave the atlas edge **4096** and 512² tiles, observed), the "ready" byte `+0x258`, and creates the D16 local atlas itself (4096² = 32 MiB) in place of `FUN_1400DCB80`. The per-level values 2, 4, 6, 10 and 2, 2, 2, 2 are kept at their constructor defaults (meaning not read).
2. **Per light (SW-7b).** Build the frame items the slot acts on: call `FUN_140212810` (point, 0x120 bytes) and `FUN_1402136B0` (spot, 0x280 bytes) on a native context that supplies `ctx+0x10..+0x4C`, `ctx+0xD0` and `ctx+0x280` (the fade/coverage scalar), or transcribe them (`CollectNativePointLights` already transcribes `EFF50`/`212810` for point lights without building the item, and skips spots); sort the items by `+0x10` descending (`FUN_1400FF3B0`); under the gate of §2.1 call slot 3 on each with the native container. The slot bodies and `1400B2DC0/1400B29D0/140242460/140242590/1402426C0/1402422A0` are CPU. The result is the item vector, the variant ids per light (`+0x1C`) and the tile rectangles. The metric `+0x9C` is **produced** by `212810` (§1.5), so the former "open" is closed; what remains open is the native source of the context scalar `ctx+0x280` and of `light+0xB1`/`light+0x94` for map lights (row SL9). Every frame first: copy `[GXRS+0x4E1A]` into `container+0x288` and call `FUN_1400B3B20` (§2.1).
3. **Cost guard.** At most 48 items, six per point light in either mode: a scene with 8 shadowed point lights exhausts the vector; the engine silently drops the rest. Natively the same (the function refuses), logged as `native_local_shadow_refused`.
4. **Receivers (wave 2).** `cbLightAccShadowInfo` from the existing producers on the native item sizes; the lighting selects `DPointLightShadowed1..4` / `DSpotLightShadowed1..3` from the variant id set in step 2 (the same ids the engine writes, so `SelectLightProgrammes` can key on them).
5. **Recording (wave 3 with the casters).** The six face passes per item (pass `i + 9`) and the `DAT_144961BF5` single-pass variant are recorded by the caster draw; wave 1 only fills the items.

The one-frame-in-flight assumption of the container (item vector reset by `FUN_1400B3B20` at frame start, frame counter `+0x284`) matches the native single-frame cadence; with frames in flight (phase D) the vector is a per-slot resource (D3D12-PHASE-D-IMPLEMENTATION-SHEET).

## 4. Filtering: design (proposed)

A table `ShadowFilterPlan(R, quality)`: mask programme by `R.FilterMode` (0 and ≥ 3 `PCF4`, 1 `PCF9`, 2 `Soft`, verified; the world gives 2), the extra inputs of `Soft` (dither texture t38 with `s6`, the `+0x230` words), the blur on/off (`[[GXRS+0x5368]+0x128] × Blur Radius > 0` **and** the pass count non-zero) and its `cbBlurParam` (12 taps from the same product and the mask size, `BlurParam` already exists), the number of blur passes (`max(0, Blur Count + blurAdd)`) and the mask target parity (odd → `+0x10`, even → `+8`), the mask target description (four 8-bit channels, screen size, channel `w`), the composite constants. The native programme set is the six programmes of `FUN_1401D76B0` (fingerprints in PROGRAM-FINGERPRINTS: `GXScreenSpaceCascadedShadow_PCF4/PCF9/Soft`). Wave 1 builds the plan and the resources (mask target and blur ping-pong as plan resources, dither texture lookup) and logs it; wave 2 records the passes.

## 5. Witnesses of the five zones (plus m30, m40)

All zones use the same `R` (§1.2), so the **record, the sizes and the state words are one golden for all zones**; the zone-dependent witnesses are what the engine casts and which local lights shadow (SHADOWS §1.3 table, observed in `i63-live-10`; counts are depth-only + alpha-tested draws per view, the player set being 22 + 5):

| Zone | Cascade quadrants with casters | Local-light tiles | Forward receive | Role in this wave |
|---|---|---|---|---|
| m31 Undead Settlement | (0,0) 22+8, (4096,0) 22+8, (0,4096) 0+3, (4096,4096) 0+3 | 1 tile (a single 512² viewport: a **spot**, inferred), 22+5 | none in the captured views (the m31 water is in the data only, WATER-TESS-WAVE1-READY 1.1) | first witness for S3 and the state words; one local light |
| m37 Irithyll | all four: 5, 33+5, 30+5, 26 | 2 tiles (spots, inferred), 22+5 each | water `FwdSdw` (OBSERVED, one mesh pair per frame, [WATER-TESS-WAVE1-READY](WATER-TESS-WAVE1-READY.md) 4); ghost in shade (`c1480`) expected `FwdSdw` | all cascades active: the full atlas layout; two local lights |
| m45 Ariandel | (4096,0) 23+5, (0,4096) 1, (4096,4096) 2 | **3 tiles** (spots, inferred), 22+5 each | — | the most local lights (allocator levels, eligibility) |
| m50 Dreg Heap | (0,0) 22+5, (4096,0) 22+5 | 2 tiles | — | two cascades |
| m51 Ringed City | (4096,0) 22+5 | 2 tiles | hair `FwdSSdw` | the screen-space window (mode 3) |
| m30 High Wall | not captured | not captured | player hair `FwdSdw_[A]` | first live zone for the gates (SL1) |
| m40 Firelink | player set every frame (I-0063 `i63-live-01`) | **none** (inferred from one frame: the local atlas held 0xFFFF everywhere, I-0052) | — | negative witness: no local shadow caster, container empty |

(Cascades without casters in a capture are either kept from earlier frames or redrawn in reprojection mode with only the dynamic casters; the native reference redraws all.)

## 6. Commits (each builds, passes the unit tests, is reversible alone; every rung is opt-in by a flag file until its exit holds, then the default)

| # | Commit | Touches (anchors by function) | Flag | Exit |
|---|---|---|---|---|
| **SW-1** | gates: `CollectNativeShadowGates`, row `native_shadow_gates` (quality byte, assumed pipeline gate and variant, `+0x4E1A`) | beside `CollectNativeVelocityInputs`; `NativeSelectedScene` | `shadow-gates.flag` | row at the five zones; SL1 recorded (original values) |
| **SW-2** | `ShadowRecord`, `ShadowSizes`, `ShadowStateClamps` in `native/frame` (re-export of `Banks::Shadow`), row `native_shadow_state` (R digest, S, blur count, state words) | new `native/frame/shadow_state.*`; `Banks` re-export | `shadow-state.flag` | T-W1, T-W2 against the goldens; row equals `shadow-world-blocks.tsv` at atlas edge `S` of the quality row |
| **SW-3** | cascade fit **in the test process only**: native-built `CascadeCamera` / `GXShadowMapLite` + CALL of `140239610` and `14023C1E0`; compared with a snapshot of the original object | `native/frame/shadow_fit_test.cpp` | — | T-W3 (after row SL4 supplies the snapshot) |
| **SW-4** | `ShadowState` struct, the per-frame fit CALL in production, the sun direction (light 0 of the zone LightSet through `FUN_1400F90F0`, producer open: row SL4), `ShadowFrame` table | `shadow_state`, `scene_pass` | `shadow-fit.flag` | T-W4, T-W5; matrices equal the oracle at three camera poses |
| **SW-5** | constants on `ShadowState`: `PerScene`, `BlurParam`, shadow words of the 960-byte `cbSceneParam` image, per view | `scene_ingress.*`, `LP` re-expression | `shadow-blocks.flag` | T-W6 word for word (SL5) |
| **SW-6** | atlas resource inert (a), viewports, clears and per-cascade bias (b) | `SceneRenderer::NativeShadowResources`, `scene_pass.h` | `shadow-atlas.flag` | resource sizes by quality row (T-W2); +128 MiB at MAX; debug layer clean for 600 frames |
| **SW-7** | local lights: CPU half of the container configuration (a), per-light slot 3 and items (b), local atlas resource (c) | new `native/frame/local_shadow.*`; `CollectNativePointLights` caller | `shadow-local.flag` | T-W7; at m37 and m45 the items and tiles equal the original's (SL8, SL9); m40 empty |
| **SW-8** | `ShadowFilterPlan` and the mask/blur plan resources, row `native_shadow_filter` | `shadow_state`, `scene_pass` | `shadow-filter.flag` | T-W8; the plan at the five zones equals SL12 |
| **SW-9** | cleanup after the exits hold at the five zones plus m30/m40: flags become defaults | — | — | unchanged rows |

Order: SW-1 → SW-2 → SW-3 → SW-4 → {SW-5, SW-6, SW-7, SW-8}; SW-7 does not depend on SW-2 (its inputs are the configuration block and the lights, not ShadowParam) and is independent of the cascades. Shared files (`GB`, `SP`, `scene_ingress`) are edited by other waves: each edit is at a block named above. Interaction with the other sheets: SW-6 uses the 0-render-target / D16 / patch-free envelope of MODEL-DRAW-ENVELOPE-READY (W0a), the frame plan resources of phases E/F, and the root CBV slots of the master plan §3.1; the mask target is a plan transient (aliasable), the atlases are persistent.

## 7. Tests and goldens

- **Goldens** (`shadow-world-blocks.tsv`, `Get-ShadowWorldBlocks.ps1`): the 0x94-byte `R` for `mapbase` id 0 and for `default` ids 0, 10..15, and, for atlas edges 2048, 4096, 8192, the state words (`+0xA70..+0xA84`) and the `cbSceneParam` words `+0x1C0`, `+0x1D0`, `+0x230`, `+0x240`, `+0x250`. World example at S = 8192: near `3DCCCCCD`, splits `40000000`, `41000000`, `41700000`, fade start `4171999A` (15.1), fade dist `42480000`, `+0x1C0 = (42823333, 3CA3D70A, 38800000, 38800000)`, `+0x250 = (1, 3EA22222, 3E8AF8B0, 3D1B565B)`.

| ID | What | Pass |
|---|---|---|
| T-W1 | `ShadowRecord` against the TSV for the 8 records, and against `Banks::Shadow` | bit-exact |
| T-W2 | `ShadowSizes` for the five quality rows × the four classes, with the double-size flag | 1024, 1024, 2048, 4096, 8192 for class 3; the same table for the others; blur count `max(0, BlurCount + blurAdd)` |
| T-W3 | genuine `140239610` and `14023C1E0` on native-built records against the snapshot of the original (3 poses) | matrices equal within 1 ulp; planes equal |
| T-W4 | native `ShadowState` against `ShadowFieldsFromBanks` and the engine-state snapshot words | word for word |
| T-W5 | `ShadowFrame` for both variants and both gate states against SHADOWS §7 ranges | equal (the end of the window `[0x17, …]` is not specified by SHADOWS §7: that window is checked only for its start) |
| T-W6 | `PerScene`, `BlurParam`, the shadow words of the image on the native state against the Region-based oracle | word for word |
| T-W7 | the allocator against the genuine `FUN_140242460/590/2A0` on a natively-built allocator (levels, rectangles, exhaustion), and slot 3 against the oracle on synthetic lights (levels at the thresholds 0.1/0.4/2.0 and 0.08/0.3/2.0, the 15.0 cutoff, the 48-item guard) | equal (the end of the window `[0x17, …]` is not specified by SHADOWS §7: that window is checked only for its start) |
| T-W8 | `ShadowFilterPlan` for `FilterMode` 0/1/2 and the blur gate | table equal |
| T-W9 | resource descriptions (atlas, local atlas, mask) by quality row; `static_assert` of the new structs | equal / compiles |

## 8. Live rows (none taken; added to LIVE-CHECKS-CONSOLIDATED §1.14)

| ID | Kind | Orig/Nat | Read | Expected |
|---|---|---|---|---|
| SL1 | NOW | Orig | `pipeline+0x3EBC`, `[[GXRS+0x5360]+0x28]`, the variant `iVar28`, `pipeline+0x2DA8` at m30, m40 (and the five zones) | 1, 1, 0, 0 (assumed) |
| SL2 | NOW | Orig | the ShadowParam record the pipeline holds (`88A10` blend result, 0x94 bytes) at each zone | equal to the `mapbase` golden at all seven |
| SL3 | NOW | Orig | the four cascade rasterizer states (`+0xA38..`): depth bias and slope bias values, atlas format | the bias conversion of §2.5 (taken: Reference1502, SW-6b log) |
| SL4 | NOW | Orig | a copy of the `GXShadowMap` object (0x130 bytes) and the camera record passed to `140239610` for cascade 0..3 at three poses, and the returned matrices | the input layout and the oracle for T-W3 |
| SL5 | NOW | Orig, then Nat | the 960-byte `cbSceneParam` and `cbPerScene` of one deferred and one forward row at m31 | equal natively |
| SL6 | NOW | Orig | `obj+0xC00/0xC04`, `obj+0xC09`, `obj+0xC1D+i` and `[14496231C+i]` over 600 frames at m31, m37 | S = 8192 at MAX; the keep pattern of §5 |
| SL7 | NOW | Orig | quality rows `+0x28..+0x3F` at the five levels | the table of §0.3 |
| SL8 | NOW | Orig, then Nat | the container: `+0x280`, `+0x284`, `+0x288`, `+0x28C..+0x2B4`, item count `(+0x268 − +0x260)/0x1850`, atlas size, at m37, m45, m31, m40 | 48; edge 4096 at the reference; 2 / 3 / 1 / 0 items |
| SL9 | NOW | Orig | per shadowed light: the frame item (`+0xA4`/`+0x1C8`, `+0x9C`/`+0x260`, `+0x7C`, `+0x10` sort key), `ctx+0x280`, `light+0xB1`/`+0x94`, the level chosen, rectangles, `+0x1C` before and after, point vs spot | thresholds of §1.5; the sources of the scalars; spot confirmed at m31, m37, m45 |
| SL10 | NOW | Orig | `[GXRS+0x4E1A]`, `container+0x288` after `FUN_140023790`, `DAT_144961BF5`, `DAT_14473D39C`, `DAT_14473CE59/5A`, and the gate `pipeline+0x78`, `pipeline+0x3EBF`, `[[GXRS+0x5360]+0x44]` | 1, 1; the local-light mode and gate values |
| SL11 | NOW | Orig | the config block values `Draw.PointLightShadowmapResolution`, `Draw.SpotLightShadowmapResolution`, `Draw.DirLightShadowmapResolution` | 256, 512 (defaults) or the reference values |
| SL12 | NOW | Orig | the programme bound for the world mask draw (`PCF4`/`PCF9`/`Soft`) and the blur draws per zone | `Soft` + blur at the reference quality |
| SL13 | NOW | Orig | the mask target format and size, and whether the layer's `+8` or `+0x10` object is bound | `R8G8B8A8_UNORM`, screen size |
| SL14 | NOW | Orig, then Nat | `view+0x710` for every forward row (engine pass) at m37 (water and ghost), m30 (hair), m51 (hair) | the `ShadowFrame` table |
| SL15 | NOW | Orig | `FilterMode` word the mask layer receives and the blur gate inputs `[[GXRS+0x5368]+0x128]`, record `Blur Radius` | 2; > 0 |
| SL16 | ACC | Nat | debug layer over 600 frames with the atlas, the local atlas and the mask plan resources | clean |

## 9. Risks and open points

1. **The cascade-fit input layout (§2.3).** The functions are pure but their input records are read from the decompile, not a captured object; T-W3 cannot be written until SL4. Static route: finish the field list of `GXShadowMap` from `140239610`, `14023C1E0`, `140238680`, `1402388B0` and `1401B6B80` case 0/3.
2. **The pipeline gate and variant natively (§2.1).** A wrong assumption changes the forward rows' mode and the mask draw; detected by SL14.
3. **Sources of the item inputs for map lights (§1.5, §3).** The metric and the cast flag are produced (`212810`, `2136B0`); the native sources of the context scalar `ctx+0x280` and of `light+0xB1`/`light+0x94` for map lights are not read. The observed tiles are single 512² viewports, so the witnesses are **spots** (a point light always allocates six tiles); native collects no spot today, so the m37/m45 exits of SW-7 need the spot chain. The "512" is the constructor default; only the atlas edge 4096 is a setting.
4. **Bias conversion (§2.5): closed by SL3** (Reference1502): no constant bias, slope-scaled bias = R+0x6C, see the SW-6b log.
5. **The container's other members.** The constructor stores 11 config dwords and three vectors (size 0x2F8, already built natively). The members `+0x2B8/+0x2D8` (id sets for programme registration, `FUN_1400B3CD0`) are used by `FUN_1400B3BA0` (frame end, calls `FUN_140101420` on `[GXRS+0x4DE8]`, the programme set): native must not call that end-of-frame function (it touches the GX programme set); the sets are only needed to register the shadowed programmes, which native selects directly.
6. **Memory.** 128 MiB (cascade atlas at MAX) + 32 MiB (local atlas at 4096) + mask and blur targets: within the master plan budget but the largest single resources; keep the quality-row scaling (HIGH: 32 + 32 MiB).
7. **Threads.** The state, the constants and the items are immutable per-frame snapshots built on the owner thread (R13/R8b); the slot-3 calls run in the collect step.
8. **Frames in flight (phase D).** The container's vector and the mask are per-slot; the cascade atlas is shared if the casters of frame N+1 are recorded after the lighting of frame N has finished reading it (fence), else per-slot (D3D12-PHASE-D-IMPLEMENTATION-SHEET).
9. **Not covered:** the casters (wave 3), the mask, composite, sun and forward draws (wave 2), the reflective shadow map scene `FUN_1401DD490` (menus/probes), particle shadows (E07), the cloud shadow variant `DDirLightShadowed1_Cloud`, the menu views' `{1, id}` cascades.

## 10. Corrections to earlier notes (applied with this sheet)

| Note | Statement | Now |
|---|---|---|
| SHADOWS-NATIVE §0 | "native has no producer; the shadow state exists only as engine reads" | the fit, culling volume, clamps and constants builders are pure CPU functions; the producers exist as snapshots only; the sizes follow the quality row (§0.3) |
| SHADOWS-NATIVE §1.1 | local atlas "4096², 512² tiles" as fixed | the local atlas edge, the spot tile (512) and the point tile (256) are configuration values with defaults 1024 / 512 / 256 (`FUN_1400B24E0`, `140048B80`); 4096 and 512 are the reference setting |
| WAVE1-PREP S2 | cascade fit "transcribing `1B5720`/`23A0C0` or an engine CALL on a natively constructed `GXCascadedShadowScene`" | neither: CALL the pure fit functions on native-built records (the scene object's constructor creates GX resources) |
| WAVE1-PREP S3 | atlas size class "0→2048, 1→1024, 2→2048, 3→4096 clamped by the quality object" | verified, with the exact formula and rows (§0.3) |
| SHADOWS-NATIVE §1.3/§3 and the draft of this sheet | "the slot 3 of the light classes acts on the lights"; "field of view π/2, six faces with `+0x378` = 6" | slot 3 acts on frame items built by `212810`/`2136B0`; with `container+0x288 == 0` a point light is six single-face items, field of view `2·atan(h/(h−3))` (§1.5) |
| D3D12-MASTER-PLAN S5 | "Shadows: `quality+0x28`, size `+0x38`, LOW and up, size 1024..8192" | the rows are `(on, num, den, min, max, blurAdd)`; the size also scales with `num/den` and the class (§0.3) |

## 11. Review log

Independent review by an Opus 5.5 reader (read-only; given the note and the evidence, not conclusions; four Ghidra runs of its own). **Two blocking findings, six important ones and a list of minor ones, all verified (the gate lines, the `+0x4E1A` store, the blur condition and the `23A0C0` access re-checked by the author) and applied.** The reader re-derived as exact: the size formula and the quality rows, the split clamps and the golden words (`+0x1C0` = 65.1, 0.02, 2^-14; `+0x250`), the absence of ShadowParam groups in the seven zone files, the filter order 0/1/2, the container constructor and its defaults, the CPU/GPU split of `B27A0`, the native construction of the container, the thresholds and variant ids of both slots, the retry rule, the allocator rounding, the zone counts and the `ShadowFrame` windows.

| # | Severity | Finding | Applied |
|---|---|---|---|
| 1 | blocking | slot 3 acts on the per-frame items (`212810`/`2136B0`), which native does not build; the metric is produced there; §9.3 route wrong | §0.5, §1.5, §3.2, §9.3 |
| 2 | blocking | the gate is `FUN_14010A220` l. 116-126 (`pipeline+0x78`, `+0x3EBF`, quality `+0x44`); `[GXRS+0x4E1A]` is a mode byte, set to 1 natively; `FUN_140023790` and `FUN_1400B3B20` do not run natively; items sorted by `+0x10` | §0.5, §1.5, §2.1, §3.2, SL10 |
| 3 | important | `23A0C0`, `1B5720`, `238680` are not GX-free | §0.4 |
| 4 | important | blur gate also needs the non-zero pass count; "bit 0" is the pass-count parity choosing the mask target | §0.6, §1.6, §4, T-W8 |
| 5 | important | `+0x378` is a face index (mode 0) or 6 (mode ≠ 0); FOV is `2·atan(h/(h−3))` | §1.5 Allocation and Item rows, §3.3 |
| 6 | important | the observed single 512² tiles are spots; 512 is the constructor default | §5, §9.3, §3.1 |
| 7 | important | sun direction from light 0 of the LightSet (Shadow Light Id 0) | §0.2, SW-4, §2.3 |
| 8 | minor-important | input attribution of the fit (`6EF80` reads `+0x140..`; `139610` reads the pose; dispatcher fields) | §2.3 |
| 9 | minor | 48 items not 47; unsupported "8 bytes per texel"; option-string link inferred; SW-7 independent of SW-2; `1402426C0` listed; `14004B5C0` guard; eligibility missing in §10; T-W5 window end; m40 inference | applied (§1.5, §3, §6, §7, §10) |
| 10 | note | not verified by the reviewer: the writer of `+0x284`, the meaning of `ctx+0x280`, the bias conversion (SL3), the tail of `1B5B40` | §9 |

## Exports and evidence

Decompiled by Ghidra for this sheet (query outputs): `14010AFE0`, `14010BFA0`, `14010C110`, `14010C7E0`, `140103510` (existing), `1400B24E0`, `1400B27A0`, `1400B29D0`, `1400B2DC0`, `1400B3B20`, `1400B3BA0`, `140212C40`, `140214350`, `140242460`, `140242590`, `1402422A0`, `140239610`, `140238B40`, `14023C1E0`, `140238680`, `1402388B0`, `1401D76B0`, `140048B80`, `140023790`, `140024740`, `1401B8F30` (existing), `140020E80`. Existing: `001B5720`, `0023A0C0`, `001B6B80`, `001B5B40`, `001A55B0`, `001A6690`. Scripts: `Get-ShadowWorldBlocks.ps1` (new), `Invoke-Map.ps1`, `I0068Find.java`, `I0068Grep.java`.

## Implementation log (pilot, 2026-10-13, W5a)

**SW-2.** `native/frame/shadow_state.*`: 1B5720's class sizes, the quality-row clamp and blur passes (`ShadowSizes`), its state
clamps (`ShadowClamps`) and the cbSceneParam shadow words (`SceneWords`, the single-precision derivation of
ShadowFieldsFromBanks without snapshots). T-W1/T-W2 (`shadow_state_test`, run by the build): 576 words of
`shadow-world-blocks.tsv` for the 8 records and the edges 2048/4096/8192 equal, except the three cascade blend ratios
(+0x250.y..w), which may differ by one unit in the last place because the golden was computed by PowerShell in double precision
(the engine and this code divide in single precision); the sizes for the five quality rows equal 1024/1024/2048/4096/8192 with
blur passes 0/0/0/1/3. **SW-1 (diagnostic part).** Row `native_shadow_state` (first two, then every 600th collect): the
quality object's shadow byte, row and Lighting byte from the scene's option snapshot, R = `Banks::Shadow({area, 0})`
(the area of the view context's gparam key), the sizes, the state and the root's `+0x4E1A`; the pipeline gate and variant are
written as assumed (open, row SL1). Native1495 (High Wall, MAX): shadows on, Lighting 1, row (2, 1, 128, 8192, 2), area
130000000, FilterMode 2, class 3, atlas 8192, 3 blur passes, near 0.1, splits 2/8/15, fade 15.1/50, local mode 1: the values
§0.2/§0.3 predict. Next: SW-3 (the cascade fit in the test process: needs row SL4's snapshot of the original), SW-4 onward.

**Row SL4 (Reference1496, High Wall, MAX; `shadow-observe.flag`, observer of `FUN_140239610`, 156 sampled calls).** The fit is
called four times per view, one per cascade, by `1B6343`/`1B63EA` (menu and title views, size 2048) and `23872B` (the world
view, size 4096 = S/2 at S = 8192). For the world view: depth range 100.0 (= R+0x64 Volume Depth), align 1 (R+0x71), blend 1.0;
the camera record's lens words `+0x140` field of view 0.7505, `+0x144`/`+0x148` the slice (0.1, 2), (2, 8), (8, 15), (15, 65.1)
(the near, the splits and the fade end of SW-2's state, in that call order), `+0x14C` aspect 1.7778; the pose at `+0x50..+0x8F`
is the main camera's world matrix; the light direction (-0.589, 0.695, 0.413) at High Wall. So the record native must build is
{pose, field of view, slice, aspect}, the scalar arguments come from R and the sizes, and the order of the four calls is 0..3 with
the slices of SW-2. The sampled inputs and both output matrices per call are in the run's journal (`reference_cascade_fit` rows),
the oracle for T-W3.

**SW-4, first form (diagnostic, `shadow-observe.flag`).** In the shadow-state row the native build calls the genuine
`FUN_140239610` four times on a native camera record {the scene camera's pose at `+0x50`, its field of view `+0x140`, the SW-2
slice `+0x144/+0x148`, its aspect `+0x14C`} with depth range R+0x64, size S/2, align R+0x71 and blend 1.0, and the light
direction of light 0 of the zone's LightSet `{area, 0}` (`Banks::Sun`, record +0x00). Native1497 (High Wall): the sun direction
(-0.589249, 0.694658, 0.412596) equals the original's (-0.58925, 0.69466, 0.4126, Reference1496), so the light producer of §2.3
is closed: light 0 of the zone's LightSet through the native banks. The four cascade matrices equal the original's within the
camera's idle motion between the runs: rotation words identical (-0.573577, 0.71934, -0.412596), translation (61.3727, 22.3658,
90.1202) natively against (61.3722, 22.366, 90.1204) in the original at a camera 0.05 apart; the same for the three other
cascades. Fixed on the way: the title key {-1, -1} now resolves to {1, 0} as the engine does. Next: SW-4 production (the matrices
kept in a native `ShadowState` per frame), SW-5 constants, SW-6 resources, SW-7 local lights, SW-8 filter plan.

**SW-4 production (`shadow-fit.flag`, opt-in until SW-9).** Each collect produces `NativeSelectedScene::shadow_frame` (R, the
sizes, the state, the LightSet sun, the four cascade view-projections and matrices by the genuine fit); no consumer yet.
Native1498 (High Wall, measure run, no debug layer, effects by default): exit 0, 60 frames/s, collect 4.9 ms (5.2 in
Native1473 before W4 and SW-4), prepare 3.9 ms: no measurable cost.

**Row SL5 (Reference1499, 1500; `shadow-observe.flag`: observers of 13AA60, the cbSceneParam image each view uploads, and of
1D6D20, whose second argument is the pass and third the cascaded-shadow state).** The world image carries +0x1C0 (65.1, 0.02,
1/16384, 1/16384), +0x1D0 (1, 1, 1, 0.005), +0x230 (0.1, 10, 1, 1/32), +0x240 (1/8192, 1/8192, 0, 0), +0x250 (1, 0.3167, 0.2714,
0.0379), +0x360 FC_CascadeSelectDist (2, 8, 15, 0); the state holds +0xA60 the sun direction (equal to native's), +0xA70.. the
SW-2 words, +0xA90 the +0x1C0 words, +0xAA0 the select distances, and at +0xAF0 + 0x40 n the cascade's world-to-atlas matrix,
which is **the fit's two outputs multiplied (view-projection x matrix) then mapped to the atlas quadrant** (x' = 0.25 x + 0.25 + qx,
y' = -0.25 y + 0.25 + qy, quadrants (0, 0), (0.5, 0), (0, 0.5), (0.5, 0.5)); the image's +0x260 FC_ProjSpaceToShadowMatrix[4] =
transpose(the pass +0xD0 camera world matrix x atlas n). The random texture edge is 32 (1/32 in +0x230.w).

**SW-5 (producers; no consumer yet).** `shadow_state`: `Multiply` (1CE40's order), `AtlasMatrix`, `CascadeWords`; the shadow frame
keeps the four atlas matrices and the image words +0x1C0..+0x36F. T-W4/T-W6 against row SL5 (`shadow-sl5-oracle.tsv`,
Reference1500, run by the build): the atlas matrices from the original's fit outputs equal the state's within 3.1e-5 (fits and
state sampled at different frames of an idle camera), the projection-space words from the state equal the image's, the select
distances, +0x1C0, +0x1D0, +0x230, +0x240 equal exactly and the ratios within one unit in the last place. The words are not yet
merged into the drawn cbSceneParam: the shadowed programmes would then sample an atlas that does not exist (SW-6 and wave 2/3).
Next: SW-6 (the atlas resource, inert), SW-7, SW-8.

**SW-6a (`shadow-atlas.flag`, needs `shadow-fit.flag`; inert).** The main view requests an R16_TYPELESS depth atlas of the
shadow frame's edge (D16 clear 1.0, state DEPTH_WRITE), re-created when the edge changes and counted by the E0 target-bytes
audit; nothing draws into it or samples it yet. Native1501 (High Wall, debug layer on): exit 0, 6523 frames with 0 debug errors;
target bytes 218 MiB in game against 90 MiB in Native1498 without the atlas (+128 MiB = 8192 x 8192 x 2 at MAX), and 122 MiB
after the return to the title, whose record (area 1) gives 4096: the size follows the zone's record and the quality row (T-W2
exit of SW-6 for the resource). Next: SW-6b (viewports, clears, bias; row SL3), SW-7, SW-8.

**SW-6b (viewports, clears, cascade rasterizer; row SL3).** Static: `FUN_1401B5390` creates the four cascade rasterizer states at
state +0xA38 + 8 n from a GX description {fill 0, cull 0, front counter-clockwise 1, depth bias 0, bias clamp 0, slope-scaled bias =
state +0x91C + 0x50 n, depth clip 1} (`FUN_14192F860` maps GX fill 0 to SOLID and cull 0 to NONE); `FUN_1401B5720` fills the four
cascade records at state +0x8F0 + 0x50 n, so +0x91C is R+0x6C (`Slope Scaled Depth Bias`), +0x914 R+0x64 and +0x920 the
`Cull Flip` byte R+0x70; `FUN_1401B6020` (the per-cascade recording) re-creates the state when the record's slope changes. The
§2.5 guess (record values scaled by 1/65535) is wrong: there is no constant bias in the rasterizer, and `Depth Offset` (R+0x54)
reaches the shaders, not the rasterizer. Row SL3 (Reference1502, `shadow-observe.flag`, the GX descriptions read back by
`FUN_14192F970`): the four states equal that description, slope 0x3BA3D70A = 0.005; kept as the oracle row `sl3_rasterizer`.
Native: `shadow_state` `CascadeViewport` (the quadrants of `AtlasMatrix`, size S/2) and `CascadeRasterizer`; the shadow frame
keeps both; the main frame clears the four viewports of the atlas to depth 1.0 (D16 DSV), and an atlas of another edge is
retired by ticket with its view heap. Tests: viewports against the atlas matrices' quadrants, the rasterizer against SL3.
Native1503 (High Wall, debug layer on): exit 0, 6494 frames with 0 debug errors; targets as Native1501 (218 MiB in game).
SW-6's exit holds (sizes by quality row and record, +128 MiB at MAX, debug layer clean). The shadow-draw PSOs that will use the
rasterizer are wave 2. Next: SW-7 (local lights), SW-8 (filter plan).

**Rows SL8-SL11 taken (Reference1504 High Wall, 1505 Firelink, 1506 High Wall; `shadow-observe.flag`: the container cell in
`reference_per_scene`, and `reference_local_shadow_slot` rows from observers of 212C40 and 214350).** Container `[GXRS+0x41D8]`
in the original: ready +0x258 = 1, capacity +0x280 = 48, mode +0x288 = 1 (= root +0x4E1A), config dwords spot tile +0x28C =
0x800, per-level 2/4/6/10, point tile +0x2A0 = 0x200, 2/2/2/2, atlas edge +0x2B4 = 0x1000; quality Lighting byte 1; one-pass
cube `144961BF5` = 0; job variant off (`14473CE59` = 1, `14473CE5A` = 0); frame counter +0x284 read 0. Items in game: **1 at High
Wall and 1 at Firelink** (the sheet expected m40 empty: wrong). The one hit at High Wall (1506) is a map point light at
(-137.7, 26.5, 18.7), radius 10, colour (0.5, 0.38, 0.34), cast byte +0xA4 = 1, metric +0x9C = 7.84 (level 3), +0x14 = 1, +0x18 = 2;
slot 3 sets +0x1C 0x6C -> 0x6D, +0x50 = 1.001, +0x54 = -0.01, +0x5C = +0xA0, +0xB0 the container item, and writes six face
rectangles at +0xBC (tiles of 128 texels reduced by 3: (3, 3, 125, 125), (131, 3, 253, 125), ...). Firelink's spot lights cast but
their metric (15.1, 16.4) stays above the 15.0 cutoff in the sampled frames. Source of the config (static): `FUN_140D573B0` passes
`FUN_1400B5E00`/`FUN_1400220A0` a block whose +0x1C point tile = `FUN_140810D70(0x200)`, +0x20 spot tile = `FUN_140811100(0x800)`,
+0x24 edge = 0x1000 (constant), +0x18 = `FUN_14080CAF0(0x800)`; the three getters sit in protected code (no decompilation) and
returned their default arguments at the reference (SL11 by the container words). `FUN_140023790` calls `FUN_1400B3B20` (empties
the vector through `1400B42C0`, re-initialises the allocator levels when ready) then copies +0x4E1A into +0x288;
`FUN_140242340(container+0x10, {edge, edge>>5})` allocates the allocator nodes from the container heap (CPU).
**SW-7 design (for the next thread):** SW-7a configures the native container with B27A0's CPU half (242340 and the eleven dwords,
from D573B0's arguments as observed; ready = 1 once the native local atlas of the edge exists), and each frame runs 23790's two
statements; SW-7b builds the 0x120 point items (CollectNativePointLights transcribes EFF50/212810 without them) and the 0x280
spot items, sorts by +0x10 and calls the genuine slot 3 on the native container; exit: the High Wall light above gives the same
item and rectangles natively (Firelink and High Wall replace m37/m45, which have no save).

**SW-7 (thread « Vague 1 producteurs W5 », branch `claude/friendly-feynman-70213e` from 9f726ad5; `shadow-local.flag`,
opt-in until SW-9).** Row SL1 (Reference1507, `shadow-observe.flag`, observer of `FUN_14010A220`): its third argument is the
view context the native collect already reads (keys +0x3FD8/+0x4040, ratio +0x40A8), with +0x78 = 1, +0x7C = 3, +0x3EBC = 1,
+0x3EBF = 1, +0x2DA8 = 0 in game; the SL9 rows gained the pass words: +0x10 the view, +0xD0 its inverse (the camera world
matrix, product with the view within 4e-5 of the identity), +0x280 = 1.0 on every row. Native1508: the native context
carried +0x3EBF = 0, because `NewNativeSceneContext` did not transcribe `FUN_1400843C0`'s shadow flag bytes (the qword
0x01010001 at +0x3EBC; `FUN_1400885F0` later copies +0x3EBE into +0x3EBF); now transcribed (the neighbouring 843C0 flags
+0x3EB0, +0x3EB8..+0x3EBB stay open, nothing native reads them). Code (`CollectNativeLocalShadows`, called at the end of
`CollectNativePointLights`, the 1F1C60 fade now one function `NativeLightFade` for both): SW-7a once per container the
CPU half of `FUN_1400B27A0` on `FUN_140048B80`'s block (spot tile 0x800, 2/4/6/10, point tile 0x200, 2/2/2/2, edge
0x1000: `FUN_140D573B0`'s arguments, its two getters in protected code returned their defaults at the reference), the
allocator by the genuine `FUN_140242340` on {0x1000, 0x80} (6 levels), ready +0x258; each main-view collect the two
statements of `FUN_140023790` (genuine `FUN_1400B3B20`, mode +0x4E1A into +0x288). SW-7b: the lights of the category, kept
by `FUN_1400FCE30`'s visibility rule (visitor vtable `143D622B0` slot `140222FF0`: the light's own test, point `1400EFF10` = the
frustum's +0x50 on the sphere +0x150, on a frustum record built by the genuine `FUN_14006E780` + `FUN_14006E9E0` from the
CED670 camera bytes, checked equal to the scene's pose), faded by `NativeLightFade`, get their 0x120 item from the genuine
`FUN_140212810` on a native pass record {+0x10 view, +0xD0 camera world, +0x280 1.0}; stable sort on the unsigned +0x10
descending; under the gate context+0x78 == 1 && +0x3EBF && the quality Lighting byte, each item's genuine slot 3 runs on
the pass record and the container. SW-7c: an inert R16 local atlas of the container edge (4096², 32 MiB) beside the
cascade atlas (`SceneRenderer::NativeDepthAtlas`, one helper for both). Runs: Native1509 (no visibility rule yet) shadowed
two lights behind the camera and gave spot items whose sort keys (195, 51970) differ from the original's (6 to 11):
the visibility rule was added and spot lights are left open (their per-frame volume state, read by `FUN_1402136B0` and by
their visibility test on +0x220, is not produced natively; Native1510 rows `spots_open`). Rows SL9b (Reference1511/1513,
observer of `140222FF0`; Native1512/1514 `native_light_visibility`): the main-view frustum records equal the original's
but for the camera's idle motion, and the kept set agrees light by light where both sample the same view (the
reference's windows mix the main view 1280x720 with a 640x360 view). **Exit (Native1514 against Reference1507, Thomas's
save at the Firelink approach):** one shadowed point light at (104.855, -49.43, 507.08), radius 6, in both: sort key
12000, metric 0.0083333, programme variant 0x66, depth words (1.001001, -0.006006), the container's single 0x1850 item,
and the six face rectangles identical to the texel ((3, 3, 509, 509), (515, 3, 1021, 509), (3, 515, 509, 1021),
(515, 515, 1021, 1021), (1027, 3, 1533, 509), (1539, 3, 2045, 509)); 0 debug errors, clean close. Observed in
Native1508-1512 and not in 1514: a refusal `native_decal_cpu_query_family_not_owned` (decal request kind 1 from
`140051BDB`) as the pilot closed the game at this save's location, also with the slots off (1508): an unowned decal
family, open, not caused by SW-7. Open: spot lights (with the spot family), the high-resolution m37/m45 witnesses
(Shebang saves, D-0174), the casters and receivers (waves 2-3). Next: SW-8 (filter plan), SW-9 (defaults).

**SW-8 (the mask plan; `shadow-fit.flag`).** `shadow::ShadowFilterPlan` (pure, T-W8 in `shadow_state_test`): the programme by
the record's FilterMode (0 and >= 3 PCF4, 1 PCF9, 2 Soft with the dither and +0x230 inputs), the blur scale
`[[GXRS+0x5368]+0x128]` (1 without the object) x R+0x88, the blur when it is positive and the pass count of SW-2 is non-zero,
the final mask target by the count's parity. Kept in the shadow frame; the row `native_shadow_state` carries it with the view
context's gate words. Rows SL12/SL15 (Reference1515, observers of 1D6D20's argument 5 and of `FUN_1401D7850`): in game the
world mask layer receives FilterMode 2 (Soft), the blur block global 1, radius 2, count 3, two mask targets and no third;
a second layer of the title and menu views receives FilterMode 1 (PCF9) and count 2. Native1516 (same save): the world
plan Soft, soft inputs, scale 2, blur on, 3 passes, final target the second (odd): equal to the original's inputs. The view
context's +0x7C reads 0 natively (3 in the original, SL1; nothing native reads it yet), +0x3EBC 1, +0x2DA8 0. Choice: the mask
and blur targets are not created in wave 1 (the plan describes them; the existing paired mask target of the lighting
remains; the passes and their transients come with the wave-2 recording). SL13 (the targets' format) is left to that
recording. Next: SW-9 (defaults) after the zone witnesses (D-0174).

**Zone witnesses (D-0174; 2026-10-13).** The Shebang save (copy of `shebang-alpha-prepared`): Reference1519 sent Beta to the
Undead Settlement (m31), Gamma to Central Irithyll (m37), Delta to the Dreg Heap's Earthen Peak Ruins (m50), Epsilon to the
Corvian Settlement (m45) and Zeta to the Ringed City Streets (m51), each saved there (`.ds3-i0039-backups/shebang-zones-*`,
SHA-256 in each `record.json`; Alpha untouched, roster checked). `Pilot-ZoneTour.ps1` (Load Game per character) loaded
m50 and m45 only (the list scroll differs from the setup run), so Reference1520/Native1521 cover m50 and m45 with every
quit and reload in one run (native: 24,891 frames, 0 debug errors, no refusal), and the other zones were taken with their
own save through Continue: Reference1522-1525 and Native1528-1531 (each 0 debug errors, no refusal). Results per zone,
original against native at the same camera:
- **Local-light shadows (SW-7):** equal at every zone: m31 one shadowed point light (key 12000, rectangles equal), m37 the
  same point light and rectangles, m45 the light at (46.75, -59.45, -342.55) (rectangles equal; the sort key follows the
  camera's idle drift in both), m50 two lights (keys 12000 and 9, both rectangle sets equal to the texel), m51 one light
  (key 1545 against 1544, one camera step apart). The original also shadows a spot light at m37 (key 56): spots are open
  natively (46 spot lights skipped there). Tour note: the original's light-visibility rows mix the main view (1280x720)
  with a 640x360 planar-reflection view at m45 (camera mirrored below the water), which keeps fewer lights.
- **Mask plan (SW-8):** the world record and quality row give Soft, blur scale 2, 3 passes at every zone, as the original.
- **Glow and AO inputs (W5c/W5d):** equal at every zone: mapbase (1, 1, 1, 1.5, 1, 0.5) at m31, m37, m50, m51 and the m45
  file's own Glow Filter (1, 1, 0, 3, 1, 0); the AO record mapbase at every zone, plan E R N U, variant 9.
- **The sun direction (SW-4) is wrong at two zones, from its source:** equal at m45, m50 and m51 to the word; at m37 the
  original's direction is the native one turned by 180 degrees about the vertical (yaw 26 degrees against -154; same
  elevation), at m31 native falls back to default.gparam's light (the area's file is not found by its name) where the
  original uses the m31 record. The shadow frame reads the same native bank records as the native deferred lighting
  (`Banks::Sun`, `native_light_blocks` sun0 shows the same two errors), so the gap is the lighting family's: natively the
  bank records lack the map transform the engine applies (I-0045: the MSB MapOffset yaw, bit-exact at m40 with 28
  degrees; natively no map offset is supplied, so m40's sun lacks its 28 degrees too) and the area's resident variant file
  (m31_00_0010 in this save, I-0063). Open, next item after the W5 merge (it changes the lit image at m31, m37 and m40).
Decision on SW-9: the flags `shadow-fit`, `shadow-atlas` and `shadow-local` stay opt-in until wave 2 gives them a consumer
(the atlases would cost 160 MiB at MAX for nothing; the fit and the slots cost CPU each frame); the glow and AO inputs and
the tessellation parse (with its flat fallback) are on by default.

**Open after the W5 review (2026-10-13).** The native local-shadow visibility (SW-7, `shadow-local.flag`) applies the
light's own frustum test and FCE30's walk gate (context +0x3EBE) only. The visitor 222FF0 also requires `light+0x60 & visitor+0x18`
and `visitor+0x1C & light+0x64` and stops at its capacity `visitor+0x40`; FCE30 adds the 0x39 / quality+0x6D condition. These
are not reproduced (the light masks are logged in `native_light_visibility`), so a kept/items difference in SL9b points here
first. To close with the wave-3 casters, from the visitor's fields as FCE30 fills them.

**W5 review fixes (2026-10-13).** Observer comments match their sampling and the local-shadow slot rows are capped at 40,000;
the dead `native_offscreen_preparing` guards are replaced by the documented main-view-only call path and FCE30's +0x3EBE gate;
an atlas keeps edge 0 until its creation succeeds. Native1532-1534 were the reverted sun-binding attempt (sun source gap, see
STATE); Native1535 stayed on the title (menu clicks missed, no in-game frame); Native1536 (m51, `shadow-fit;shadow-atlas;shadow-local`)
rendered the zone with 0 debug errors and no refusal.
