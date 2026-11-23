# The wind block of the draw entities (`FUN_140D07710`): layout, source, native prerequisite, families, tests and witnesses (implementation sheet, static)

STATIC (Ghidra mirror of `DarkSoulsIII.exe` 1.15.2.0; native code at this branch; retained game data of I-0063 and the MSB cache, read offline). Nothing was launched, no native code changed.
Status words: **STATIC** read in code or data, **INFERRED** deduced, **OPEN** not known statically.
Asked by the coordinator after [R8B-ENGINE-READS-STATIC.md](R8B-ENGINE-READS-STATIC.md) §3, which said the original writes the block of `SprjModelDrawEntity` at every update "while native zeroes it". **This sheet corrects that statement** (section 0) and gives the implementation for the block both producers share. Related: I-0068 [FOLIAGE-NATIVE.md](../I-0068-static-yebis-ffx-lights-spec/FOLIAGE-NATIVE.md) (the item path and the shader), [EFFECTS-D3D12-DESIGN-NATIVE.md](../I-0068-static-yebis-ffx-lights-spec/EFFECTS-D3D12-DESIGN-NATIVE.md) §2.6 and step N4, [EFFECTS-ENTRY-DOOR-NATIVE.md](../I-0068-static-yebis-ffx-lights-spec/EFFECTS-ENTRY-DOOR-NATIVE.md).
Evidence outside Git: `evidence/I-0066/queries/wind/` (callers, material rows, MSB placements, wind regions). Exports: listed at the end.

## 0. Verdict

1. **The premise needs two corrections.** (a) Native does not zero the block: `PrepareNativeModelFrame` copies `animation+0x210` (0x410 bytes) whenever `animation+0x620` is set (`game_bootstrap.cpp:5009-5023`), and for the draw-entity family `animation = entity+0x3D0`, so the flag is `entity+0x9F0` and the block `entity+0x5E0`, exactly what `FUN_140D07710` writes (`140D07818`, call at `140D07800`). The block is zero natively because **the engine never produces a node for the model**. (b) The original produces it only when `entity+0xA78 != -1`, a handle created in the constructor `FUN_140D05C80` (`140D06283`) **only if some material of the model carries policy bit 1** (`FUN_1400619B0`, bit 1 of `material+0x24`, MTD parameter `g_GXFT_WindEffect`).
2. **Why native never gets a handle (STATIC, the first real blocker).** Native builds the engine's material instance itself (`game_bootstrap.cpp:7815-7828`) and writes into `instance+0x24` **only** the cloth bit `0x2000` and the SSS bit `0x800`; its own comment lists the other bits (spx-name bits, `0x4000`, `0x80`) as open, because the GX MTD parameter handler (`FUN_1400A7E40`, decrypted at runtime) that sets them does not run. Bit 1 (`0x2`, `g_GXFT_WindEffect`) is never set, so `FUN_1400619B0` returns false for every model, **for the draw entities and for the map items alike** (`FUN_140D3C4D0` calls the same predicate): no wind node is ever registered natively. The fix is one line family in the material builder (section 5, W1).
3. **The draw-entity branch is dead in the shipped data we can census (INFERRED, partial census).** `FUN_140D05C80` is called only by the two `SprjModel` constructors `FUN_140C49210`/`FUN_140C49590`, which are built only from the `ChrIns` code (`FUN_140771E10` <- `FUN_1408A9E50`, `FUN_140771F20` <- `FUN_1408BE6C0`): **characters**. Of the 8 wind MTDs (all `*_grass` plus `Eclipse_grass` and `GrassTest`), **no chr or parts model carries one** in the retained scan (120 chr containers and 2,028 parts containers named by the pinned dictionary; the dictionary does not name every chr), while 648 map-piece material rows, 178 DLC map-piece rows and 19 object rows (12 object models, all map objects drawn through `SprjFD4ModelItem`) do. So the **real consumer of the block is the item path** (`FUN_140D3E950`, FOLIAGE-NATIVE §5), with the same block, the same node mechanism and the same native prerequisite. The sheet therefore specifies one native implementation serving both producers, with the draw-entity differences marked.
4. **The block is data from running effects, not from the model.** The values come from the wind effect instances (`WindSFX` regions: FXR 800080-800083, 31001, 845010, 850800-850802) registered in the FX instance registry `[144962398]`, evaluated by `FUN_140245F90` against the node. With W1 alone the nodes exist and the block is the engine's valid zero (counts 0, flag 1); real wind needs the effect system (N1, N2 of the effects design), which is outside this sheet and listed as its dependency.
5. **Layout, source, insertion, tests and witnesses are in sections 2 to 7**; the exact 0x410-byte layout was derived from `FUN_140D07E20` and cross-checked against the shader's RDEF table (eight tables of eight `float4` plus an `int4`).

## 1. The two producers (STATIC)

| | Items (map pieces and objects) | Draw entities (characters) |
|---|---|---|
| Class | `SprjFD4ModelItem` (vtable `2951FF8`), ctor `FUN_140D3C4D0` | `SprjModelDrawEntity` (vtable `142948EF8`), ctor `FUN_140D05C80` |
| Handle | `item+0x760` (ctor `140D3C9FB`, `FUN_140245870(.., 0x10, _DAT_142945250, 4 or 0x2_0000_0000)`) | `entity+0xA78` (ctor `140D06283`, `FUN_140245870(.., 0x10, -1, 4)`) |
| Condition | `FUN_1400619B0(model)` (bit 1, wind) **or** `FUN_140061A90(model)` (bit 19, Eclipse) (`140D3C9B0`-`140D3C9FB`); the kind argument is `4` for wind and `0x2_0000_0000` when the Eclipse bit is present (it overrides wind) | `[[entity+0xE8]+8] != 0` and `FUN_1400619B0` only (`140D06261`); kind argument `4` (`R9D`, `140D06271`) |
| Per update | `FUN_140D3E950`: `FUN_1402458B0(handle, matrix)` (node transform, `140D3EA49`), local force object, `FUN_140245F90`, slot 0 `FUN_140D07E20`, flag store `[item+0x690]=1` (`140D3EAFF`) | `FUN_140D07710`: calls `FUN_140D08190` first (bounds, which updates the node transform, next row); local force object (`FUN_140245700`), `FUN_140245F90(entity+0xA78, obj)`, slot 0, flag store `[entity+0x9F0]=1` (`140D07818`); no transform update of its own |
| Node transform | every update | only when the bounds are recomputed: `FUN_140D08190` -> `FUN_1402458B0(entity+0xA78, box)` (`140D08408`, when `BA8` bit 2) |
| Block / flag | `item+0x280` / `item+0x690` | `entity+0x5E0` / `entity+0x9F0` |
| Caller | `ModelIns` task slot 1 `FUN_140C4D330`, or the item's slot 10 `FUN_140D3D280` | `SprjModel` dirty push `FUN_140C4AF10` -> tail jump `FUN_140D06BE0` -> vtable `+0xC8` |

Both are read natively through the same code in the `cbvModelParam` producer of `PrepareNativeModelFrame` (the lines that read `animation+0x620`, `+0x630` and `+0x210`): `animation = item+0x70` or `entity+0x3D0`. The constructors clear block and flag (`FUN_1400613B0`); without a handle nothing writes them.

## 2. The exact layout (STATIC; `FUN_140D07E20` and the shader RDEF)

The block is the 0x410 bytes at `animation+0x210`, copied by native to `cbvModelParam +0x80..+0x48F` (`VC_ForceParam`, the vertex programmes' `cb7[8..75]`). Offsets below are relative to the block start (add `0x80` for the cbuffer offset, which is what FOLIAGE-NATIVE §7.2 lists):

| Block offset | cbuffer offset | Member (RDEF `ForceParamST`) | Count | Filled by `D07E20` from the force object (record base + offset) |
|---|---|---|---|---|
| `+0x000` | 128 | `int4 WindForceLoopParam` | 1 | `x` = area count (`force+0x08`), `y` = point count (`force+0x690`); `z`, `w` are **not written** (keep the constructor's zero) |
| `+0x010 + 16 i` | 144 | `float4 WindForceAndTime[8]` | area `i` | area record `i` at `force+0x70+0xD0*i` (16 bytes) |
| `+0x090 + 16 j` | 272 | `float4 PointWindPositionAndTime[8]` | point `j` | point record `j` at `force+0x6E0+0xD0*j`: `x,y,z` = dwords `+0x00,+0x04,+0x08`, `w` = dword `+0x2C` |
| `+0x110 + 16 j` | 400 | `float4 PointWindForce[8]` | point `j` | `+0x30..+0x3F` |
| `+0x190 + 16 i` | 528 | `float4 WindPositionAndRange[8]` | area `i` | `force+0x90+0xD0*i` |
| `+0x210 + 16 i` | 656 | `float4 WindWaveNoiseScale[8]` | area `i` | `force+0xA0+0xD0*i` |
| `+0x290 + 16 j` | 784 | `float4 WindWaveNoiseScalePoint[8]` | point `j` | `+0x50..+0x5F` |
| `+0x310 + 16 j` | 912 | `float4 PointWindDirAndEnable[8]` | point `j` | `x,y,z` = dwords `+0x20,+0x24,+0x28`, `w` = `float(int dword +0x84)` |
| `+0x390 + 16 j` | 1040 | `float4 WindAttractionParam[8]` | point `j` | `+0x60..+0x6F` |

The shader runs `min(8, x)` iterations over the area tables and `min(8, y)` over the point tables (FOLIAGE-NATIVE §7.2). Rows beyond the counts are **not cleared** by `D07E20`: a frame with fewer winds than the previous one leaves stale rows, which the shader never reads. **Consequence for any comparison: compare the header and the first `x` and `y` rows only.** The force object (vtable `142948EE0`, slot 0 = `FUN_140D07E20`) is initialised by `FUN_140245700` and evaluated by `FUN_140245F90`: two arrays of eight 0xD0-byte records, **area records at `force+0x10`, point records at `force+0x6A0`** (reviewed: the first version said `+0x28` and `+0x6D0`, which are field addresses inside the records, not their bases), counts at `+0x08` and `+0x690`. The "source" column above gives force-object offsets, which are correct; relative to the true record base: area `WindForceAndTime` = record `+0x60`, `WindPositionAndRange` = `+0x80`, `WindWaveNoiseScale` = `+0x90`; point `PointWindPositionAndTime` xyz = `+0x40..0x4B` and w = `+0x6C`, `PointWindForce` = `+0x70`, `WindWaveNoiseScalePoint` = `+0x90`, `PointWindDirAndEnable` xyz = `+0x60..0x68` and w = `float(int +0xC4)`, `WindAttractionParam` = `+0xA0`. Tests T-L1 and W5 must build the object with these bases.

## 3. Where the values come from (STATIC for the chain, OPEN for the effect side)

1. **Node** (per model, section 1): created in the FX node manager (`FUN_140245870` -> `FUN_14025B610(_DAT_1449623A0, ..)`), its transform is the model bounds. Native already publishes the manager (`PublishNativeFxWorld`, `game_bootstrap.cpp:9481-9523`: world `4962360`, registry `4962398`, nodes `49623A0`).
2. **Evaluation** `FUN_140245F90(handle, force)`: initialises the force object, starts by initialising the force object; read in outline only: when the manager flag `[1449623A0+0x4C]` is clear it takes or creates (`FUN_140245920`) the node's result structure (`node+0x160`) and then walks the node's list of effect handles (vector at `node+0x120`) and, for each, reads the effect instance through the registry `[144962398]` (instances at `+0x18[handle]`, count `+0x9C`, enabled byte `+0x110`; instance object `+0x48` with virtual queries `+0x110` (type), `+0x118`, `+0x130`, `+0x178`, `+0xD8` (bounds)), accumulating up to eight area and eight point winds. It runs `FUN_140247E80` per record. **Who fills the node's handle list** (the effect side matching wind instances to nodes) was not traced (OPEN, §8 point 1; **closed in part BM**: the node is refilled each frame by `FUN_14025C700` from the world bucket of the registry, filled by the force-field appearances, [FFX-WAVE1-READY](FFX-WAVE1-READY.md) §1.2).
3. **Wind emitters** are the effects spawned for the map's `WindSFX` regions (`msb-wind-regions-all-maps.tsv`; the effect ids are the FXR ids 800001, 800080 to 800083, 31001, 845010, 850800 to 850802) and are alive only while the effect system runs them (EFFECTS-ENTRY-DOOR-NATIVE). `WindArea` regions (the volumes) accompany them in several maps.
4. **Timing and thread:** update-side, once per model update (`ChrIns` steps for characters, the `ModelIns` task for items); the block is overwritten in place each time (R8B §3); thread not established.

## 4. Families (STATIC data; INFERRED where the census is partial)

- **Wind policy materials = MTD parameter `g_GXFT_WindEffect` True (kind 26 -> bit 1):** `M[ARSN]_e_grass`, `M[ARSN]_l_e_grass`, `M[ARSN]_al_grass`, `M[ARSN]_l_al_grass`, `M[A]_e_grass`, `M[A]_l_e_grass`, `Eclipse_grass`, `GrassTest` (`mtd-with-wind-eclipse-water-flags.tsv`; all-MTD census 8 True of 8 rows). Shader families: the 149 vertex programmes that index `cb7` dynamically (FOLIAGE-NATIVE §7.4): every non-instanced `GXFlver_Grass*` pass and 5 `Eclipse_grass`.
- **Not in this block:** cloth. `GXFT_Cloth` materials (13 MTDs with `GXFT_Cloth` True, 121 `GXFT_ClothMatching`, `Eclipse_cloth`) go through the Havok cloth stream and `VC_isEnableCloth` (`FUN_140140850`), not through `VC_ForceParam`; `Eclipse_cloth` sets `g_GXFT_Eclipse` (bit 19) but not `g_GXFT_WindEffect`. The instanced FFX grass (`_Ins@[cl]`, m51) reads `tbForce` from the effects system (E03), not this block. Water interaction (kind 22 -> bit 17) and Eclipse (kind 37 -> bit 19) use the same node registration with other flags (FOLIAGE-NATIVE O2); **they are not part of W1** (section 5) and need their own check of what their nodes do. In the original, an item whose model has the Eclipse bit gets a node with kind `0x2_0000_0000` instead of 4, even for `Eclipse_cloth` (no wind material); after W1 alone native would give `Eclipse_grass`/`GrassTest` items kind 4 where the original gives `0x2_0000_0000` (moot for the retained data: none of the three Eclipse MTDs appears in any scanned material row).
- **Who has them (retained scan, `flver.tsv` of I-0063 and the MSB cache):** map pieces 648 material rows in the named maps (`m30_00` 42, `m30_01` 12, `m31_00` 187, `m32_00` 134, `m33_00` 84, `m34_01` 8, `m35_00` 57, `m37_00` 29, `m38_00` 5, `m39_00` 12, `m40_00` 66, `m41_00` 12) and 178 rows in the DLC maps (`m45_00` 46, `m47_00` 8, `m50_00` 7, `m51_00` 112, `m53_00` 4, `m54_00` 1); objects 19 rows in 12 models (`o301965`, `o317000`, `o323000`, `o324112`, `o325100`, `o325110`, `o325120`, `o330041`, `o331100`, `o331103`, `o331104`, `o381011`), of which only **6 are placed in the cached MSBs** (`o317000`, `o331100`, `o331103`, `o325100`, `o325110`, `o325120`; reviewed); **chr 0, parts 0** among the scanned containers. **ChrAsm parts never produce wind (settled, reviewed):** their constructor `FUN_140CFF200` (vtable `2947DB8`) calls neither `FUN_1400619B0` nor `FUN_140245870`, their slot `+0xC8` is `FUN_140D014F0` rather than `D07710`, and the only store to `+0x9F0` in the whole `.text` is `140D07818`.

## 5. Native work (implementation)

**W1. Produce the policy bits on the material instance (the prerequisite).** (Generalised by [MTD-FLAG-BITS-INVENTORY-SHEET](MTD-FLAG-BITS-INVENTORY-SHEET.md) F1: one shared word function publishes bits 1, 7 and 14 together, with the other bits staged; W1 is its bit-1 slice and the minimal form below stays valid.) In the material builder next to `*reinterpret_cast<uint32_t*>(instance+0x24)|=(cloth?0x2000u:0u)|(sss?0x800u:0u);` (`game_bootstrap.cpp:7828`), inside the existing loop over `owned->authored->parameters`, add:
```
if((parameter.name=="g_GXFT_WindEffect" || parameter.name=="GXFT_WindEffect") && parameter.values[0]!=0.0) wind=true;
...
*reinterpret_cast<uint32_t*>(instance+0x24) |= (cloth?0x2000u:0u)|(sss?0x800u:0u)|(wind?0x2u:0u);
```
Only bit 1; bits 17 and 19 stay out of scope (section 4). Effects to check before merging: (a) `FUN_1400619B0` now returns true for these models, so `FUN_140D3C4D0` and `FUN_140D05C80` call `FUN_140245870` -> `FUN_14025B610` natively for the first time (the node manager is published; row WD-2 confirms the handle is valid); (b) the nodes then exist; **whether the item update `FUN_140D3E950` runs natively at all is OPEN** (no native hook on `D3E950`, `C4D330` or `D3D280`; the `ModelIns` task step is unknown, R8B open point 1), so W1 is necessary for items but may not be sufficient; `FUN_140D07710` does run natively (observer at `game_bootstrap.cpp:9902`). When they run, `FUN_1402458B0`, `FUN_140245700`, `FUN_140245F90` and `FUN_140D07E20` execute natively for the first time: first-run observers (W4) log any fault path; further first-run risks (reviewed): the node release `FUN_1402458A0` in the destructors `FUN_140D06390` (entity) and `FUN_140D3CC00` (item) at map unload or teleport, the node-result allocation `FUN_140245920` on the FX heap, and `FUN_14025B610` silently returning -1 when the node table is full (`m33_01` alone has 1,393 grass objects), so the handle must be checked for -1; (c) other readers of bit 1 of `material+0x24`: a byte scan of the executable found none besides `FUN_1400619B0` (tests in `FUN_140C3F770`, `FUN_140C3FEC0`, `FUN_14141EF40` belong to other classes; SIB-indexed or hoisted-register uses were not covered). `FUN_140D35F80` (model load, `140D364DC`) is a third caller of `FUN_1400619B0` and caches the result in `model+0x468` bit 1 (beside `FUN_140061B00` in bit 2); W1 flips that cached bit, and no reader of `model+0x468` bit 1 was found. Row WD-3 compares the native pass rows before and after on a no-wind zone.
**W2. Prepare side: nothing to change for correctness.** The copy in the `cbvModelParam` producer (the lines with `put(0x80, animation_block...)`, about `:5047-5066`) already reads flag and block; the F1 zero rule (counts 0 when the flag is clear) is in place. The R8B Level 1 snapshot covers E4 to E6 (slice S2); the snapshot must keep the **header and the first `x`/`y` rows** meaningful (the stale-row note of section 2).
**W3. Dependency, not part of this sheet:** the effect system running the `WindSFX` emitters and filling the node lists (EFFECTS design N1, N2, N4). Until then the block is the valid zero.
**W4. Diagnostics (observers, not behaviour).** A pass-through observer on `FUN_140D3E950` and `FUN_140D07710` returning and a log row `native_wind_block` once per pacing window: number of items with `item+0x760 != -1`, of entities with `+0xA78 != -1`, number of blocks with flag set, histogram of (x, y), hash of the first non-zero block, and the count of nodes per family. The same row feeds the witnesses of section 7.
**W5. Synthetic oracle (test aid).** `native/frame/` gains a test-only function that builds a force object with known records and calls the **genuine** `FUN_140D07E20` (a pure CPU routine) to produce a block, so that tests can compare native's reading against a block whose content is known (section 6, T-L1, T-P1). It never runs in the game.

## 6. Tests

| Test | What | Pass |
|---|---|---|
| **T-L1 layout self-test** (in-process, no game logic) | fill a force object (vtable `142948EE0`, initialised by `FUN_140245700`) with distinct values in every area and point record field of section 2, set the counts to 3 and 5, call `FUN_140D07E20` and compare the 0x410 bytes with the table of section 2 | every listed byte equals its source; bytes beyond the used rows are unchanged |
| **T-P1 prepare production** | feed `PrepareNativeModelFrame`'s `cbvModelParam` producer a model whose animation flag is set and block is the T-L1 output; read the produced words | words `+0x80..+0x48F` equal the block; `PrepareReadShaderRegister` accepts the Grass interface (all 316 words) |
| **T-P0 zero rule** | flag clear / animation null / read failure | words are zero with `written` set (FOLIAGE F1), counts 0 |
| **T-B1 policy bit** | over `all-mtd.tsv` run the W1 mapping on every authored material parameter set | bit 1 is set for exactly the 8 wind MTDs and for no other; bits 0x2000 and 0x800 unchanged for all 12,142 rows |
| **T-S1 shader reads** | the 149 `.vpo` Grass/Eclipse_grass programmes through `AnalyzeConservative` | the conservative analysis marks every word of the 1,264-byte buffer as read (316 words), as FOLIAGE-NATIVE §7.3 states; the produced block must therefore cover `+0x00..+0x4EF` |
| **WD-1 handle census** (live, read-only, after W1) | log the W4 row for 600 frames per zone | items with a handle equal the zone's grass pieces in view; entities with a handle: zero (confirms section 0.3) |
| **WD-2 node validity** | after W1, `item+0x760 != -1` for every grass item admitted, no fault in the first-run observers | yes |
| **WD-3 no-regression** | compare `native_pass_rows`, `content_draws` and the constants hash row on a zone without grass in view (e.g. a title or Status menu pass and an interior) before and after W1 | equal |
| **WD-4 wind zone** (after W3) | counts `x`/`y` in 1..8 for items inside a `WindSFX` region; the grass sways | see section 7 |

## 7. Witnesses per zone

Grass pieces and their placements come from the MSB cache (`msb-grass-mappiece-placements.tsv`); wind regions from `msb-wind-regions-all-maps.tsv`.

| Zone | Grass models / placements | Grass objects (models x placements) | `WindSFX` regions (FXR x count) | Role |
|---|---|---|---|---|
| **`m30_00` High Wall** | 39 / 39 | none | 800081 x3 (`WindArea` 3) | S1/W2 witness: no wind sources nearby at idle, so counts 0 with flag 1 (WD-1, WD-3) |
| `m30_01` | 12 / 12 | none | 800082 x2, 800081, 800083 (`WindArea` 4) | first sway test with few sources |
| **`m31_00` Undead Settlement** | 96 / 351 | `o317000` x1 | 800082 x10, 800081 x8, 800080 (19) | **main wind witness** (N4 of the effects design) |
| `m31_01`, `m31_03` to `m31_09` | (pieces in the sub-maps are not in the pinned dictionary) | `o317000` x163 (`m31_01`), x124 each (`m31_03`..`m31_09`), x49 (`m31_90`); `o331100` x1 in `m31_03`..`m31_09` | 31001 x8 each (`m31_90`: 800081 x9) | object path with wind sources (6 of the 12 grass object models are placed anywhere) |
| `m32_00` | 67 / 67 | `o325100` x5, `o325110` x8, `o325120` x7 | 800082 x5, 800081 x9, 800080 (15) | |
| `m33_00` / `m33_01` | 50 / 250 | `o331103` x1,393 in `m33_01` | `m33_00`: 800081 x5 (`WindArea` 5); `m33_01`: none | **no-source object case**: 1,393 grass objects with policy bit but no wind region (counts 0, flag 1) |
| `m35_00` | 57 / 404 | none | 800081 x3 (`WindArea` 3) | heavy grass |
| `m37_00` Irithyll | 28 / 28 | none | 800081 x1 | |
| `m38_00` | 4 / 306 | none (`o381011` has no placement in the cached MSBs) | 800081 x5, 800001 (`WindArea` 5) | dense placements of few models |
| `m40_00` | 47 / 216 | none | 800081 x6 (`WindArea` 6) | |
| `m45_00` Ariandel | 46 pieces / 97 | none | 845010 x8 | DLC |
| `m50_00` / `m51_00` | 7 / 192; 65 / 147 | none | 850801, 850802 x5 (`WindArea` 6); 850800-850802 (`WindArea` 3) | DLC, plus the instanced FFX grass of `m51` (E03, not this block) |

**Characters (draw entities):** no zone has a witness in the retained data; the census of section 4 covers 120 chr containers. A complete answer is the live row WD-1 on a long play session: if any `ChrIns` entity ever shows `+0xA78 != -1`, record its model and extend this sheet with that population.

## 8. Slices and open points

| Slice | Content | Check |
|---|---|---|
| WK1 | T-L1, T-B1, T-S1 (no game) | unit tests green |
| WK2 | W4 observers and the `native_wind_block` row (no behaviour change); the expected baseline is every handle -1 | the baseline row at three zones |
| WK3 | W1 (policy bit 1) | WD-1 (meaningful only from here: items with a handle equal the grass items in view; entities zero), WD-2, WD-3 |
| WK4 | with the effects phase (W3): WD-4 at `m31_00` | counts 1..8, sway at the original frequency |

Open points: (1) **[closed in part BM, FFX-WAVE1-READY §1.2]** who fills the node's effect-handle list and the matching rule between a `WindSFX` instance and a node (the effect side of `FUN_140245F90`); (2) other readers of bit 1 of `material+0x24`; (3) a complete chr census (the dictionary names a subset) and WD-1; (4) the water-interaction (bit 17) and Eclipse (bit 19) nodes natively; (5) thread of the producers (R8B open point 1).

## 9. Exports

`decompiled/DS3-1152-F-` + `00D07710`, `00D07E20`, `00D3E950`, `00D05C80`, `00D08190`, `000619B0`, `00D3C4D0`, `00245870`, `002458B0`, `00245700`, `00245F90`, `0025B610`, `00771E10`, `00771F20`, `00C49210`, `00C4AF10`.
