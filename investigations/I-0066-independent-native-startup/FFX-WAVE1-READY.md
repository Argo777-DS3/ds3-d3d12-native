# The effects of wave 1 by family: effect system, door, effect lights, force fields (wind), and the light-shaft and colour-grading feeds of YEBIS, ready to implement (static)

Request (coordinator, 2026-10-11): the ready-to-implement sheet of the wave-1 effects (N1 to N4 of [WAVE1-PREP](WAVE1-PREP.md) §2.1), in the format of the [glow](GLOW-WAVE1-READY.md), [SSAO](SSAO-WAVE1-READY.md) and [YEBIS](YEBIS-WAVE1-READY.md) sheets; it must cover the colour-grading bank and the volumetric shadows (read as the YEBIS light shafts, §0.6) because YEBIS depends on them, and the FX registry that feeds the wind (the `WindSFX` regions); organised by effect family with the witnesses of the five zones; merged after an Opus 5.5 review.

Labels: **verified** = read in the executable, in the retained game data (FXR archives, gparam dumps) or in the native tree at main `933a9431`; **observed** = read in a retained live capture or log; **proposed** = this sheet's design; **inferred** = deduced, not read; **open** = needs a read or a measurement first. Nothing was launched, no native code changed. Abbreviations: `GB` = `native/frame/game_bootstrap.cpp` (line numbers are that tree's and move; anchors are function names), `SP` = `native/frame/scene_pass.cpp`, `MF` = `native/frame/model_frame.h`. FXR/MSB names as in the linked notes. Evidence outside Git: `evidence/I-0068/queries/ffx-ready/` (`a`..`q` exports of the registry, node-manager, force-field and wind functions; `bank-blocks.tsv` the goldens of §5; `zone-force-fields.tsv` the force-field actions of the WindSFX effects), `evidence/I-0068/queries/yebis-ready/b3`..`b17` (grading and light-shaft records, converters and blends).

Sources, read again for this sheet: [EFFECTS-D3D12-DESIGN-NATIVE](../I-0068-static-yebis-ffx-lights-spec/EFFECTS-D3D12-DESIGN-NATIVE.md), [EFFECTS-ENTRY-DOOR-NATIVE](../I-0068-static-yebis-ffx-lights-spec/EFFECTS-ENTRY-DOOR-NATIVE.md), [EFFECTS-PHASE1-NATIVE](../I-0068-static-yebis-ffx-lights-spec/EFFECTS-PHASE1-NATIVE.md), [EFFECT-LIGHTS](../I-0068-static-yebis-ffx-lights-spec/EFFECT-LIGHTS.md), [FFX-ATTACHED-NATIVE](../I-0068-static-yebis-ffx-lights-spec/FFX-ATTACHED-NATIVE.md), [FFX-FAMILIES-PHASE2](../I-0068-static-yebis-ffx-lights-spec/FFX-FAMILIES-PHASE2.md), [FOLIAGE-NATIVE](../I-0068-static-yebis-ffx-lights-spec/FOLIAGE-NATIVE.md) §8, [WIND-BLOCK-IMPLEMENTATION-SHEET](WIND-BLOCK-IMPLEMENTATION-SHEET.md), [YEBIS-WAVE1-READY](YEBIS-WAVE1-READY.md) §1, [YEBIS-CHAIN-CHECKLIST](../I-0068-static-yebis-ffx-lights-spec/YEBIS-CHAIN-CHECKLIST.md) §4.

## 0. Answer first

1. **The wave is seven effect families, not four steps** (table in §3). N1 and N2 are the door for the two families of requests (F1 objects and map, F2 characters and events); N3 is F3 (effect lights); N4 is F4 (force fields, hence the foliage wind); F5 (light shafts) and F6 (colour grading) are the two YEBIS banks that YEBIS needs before its library may run; F7 (the draw families E01 to E08) is not wave 1: it only inherits the system, the instances and the zone weights.
2. **Everything in N1 to N4 is engine CPU code behind one pointer, `[SprjSfxImp+0x58]`.** The native work is: build the effect system (EFFECTS-PHASE1 §2 to §4), run its per-frame slot 8 (transcribed, §2.2), delete the `7CB820` replacement (§2.4), and read the results. No source class, no creator, no force-field or light class needs a native version.
3. **Correction: slot 8 cannot "call as is" the node-manager update once one node exists.** `FUN_14025B870` (called by slot 8 `FUN_14024D000` right after the registry controllers, call at `14024D360`) hands its per-node work to a **GX job** (`FUN_1400B6480(*[144961BB0])`, the queue at `[144961AB0]+0x2C8`) as soon as the node count `[mgr+0x30]` is not zero. EFFECTS-PHASE1 §5 step 5 lists it as CALL, which is true only while no node exists. With the wind prerequisite W1 (nodes for every grass item) it would dereference the absent display object. It must be **transcribed**: run `FUN_14025C700` over the node list inline (§2.3). This is the one statement of the wind chain that touches GX; the other 17 functions read for this sheet (list in §2.3) do not.
4. **The FX registry that feeds the wind is closed** (WIND-BLOCK sheet open point 1). A grass item's node (`FUN_140245870`, class mask 4) is filled each frame by `FUN_14025C700`, which walks the **bucket list of the world area** in the registry `[144962398]` (the world handle `[[144962360]+8]`, created by the genuine `259C60` in `PublishNativeFxWorld`) and keeps the entries whose two masks match and whose object answers the box test (`vt+0x1B0`, §1.2). Those entries are created when a WindSFX effect's **force-field appearance** is built (`GXFFX::GXForceFieldWindAreaAppearance` and its three siblings: the type-table factory `140275DF0`, which tests the action type `0x283C`, allocates the appearance and calls `140276300`): `FUN_140259F60` adds the entry, `FUN_1402552C0(world, entry)` appends it to the bucket. The masks of an entry come from the effect's own root action (`FUN_14029A5C0` reads action `0x2774`, §1.2). So the foliage wind needs, in order: the effect system (N1), the door (N2) and the library update of slot 8 (so that the WindSFX effect exists and its appearance registers), then W1 and the inline node update. `FUN_140245F90` never reads the node's box: the node matters only by *membership* (the collector's box test) and by its **persistent result structure** (fade accumulators per slot, created once and kept); the wind's transform, time and a noise term of time alone give the records. So the per-frame wind-table cache of EFFECTS design §2.6/N14 is **not** exact (the per-node fade state differs between nodes created at different times), but two nodes with the same entry set and past their fade ramp should read equal records (inferred; row FF8).
5. **What the WindSFX effects really contain (verified, FXR archives of the retained game data).** `f000800080/81/82` (common effects), `f000845010` (m45) and `f000850800/01/02` (m50, m51) each carry **exactly one `WindForce` action** (type 10300 = `0x283C`, shape Box, `forceMultiplier` 1, random multiplier ±10 or ±20, fade-out 90 to 240) and, but for `845010`, one `BillboardEx` of colour alpha 0 (an invisible E01 action: it counts in the E01 weights and draws nothing). `f000031001`, the effect of the `m31_01`..`m31_09` regions, has **no force field** (three periodic emitters with three billboards): those regions produce no wind. The four force-field types are in the type-registration list the construction already calls as is (`0x27D8` cancel, `0x283C` wind, `0x283D` gravity, `0x283E` collision).
6. **Volumetric shadows = YEBIS light shafts (bank index 9), not the FFX family E08.** E08 (`GXVfxParticleAppearanceLightShaft`, action 10003 = `0x2713`) has 0 occurrences in the five witness zones (FFX-FAMILIES §7); the shafts seen at m31 and m50 are the post chain's (`LightShaftMask_SM3`, `TexCoordMadT1_S4/S2`). The feed is the 28-word record of `GXBS::GXLightShaftFilterParam` (§4.1): 21 words come from the 15 file entries, **words 9 and 10 are record-only** (ctor defaults `0` and `0x3F7FFF58`, now read), words 1 to 3, 22 and 23 are padding. Golden blocks and the converter's globals for the seven zones are in `bank-blocks.tsv`. The feed is exact in static; **which key (value id, A and B, weight) the library receives at the witness frames is live-only** (§4.1: the zone files carry only id 0, and the file `Enable` does not predict the observed shafts).
7. **Colour grading (bank index 4) is decoded to the level a builder needs, and one link stays live-only.** The record block (`record+8`, 5 words: `Enable`, `LutSourceId`, the two ids and the progress of a fade) and its blend `FUN_14021C150` are exact (§4.2); the globals `144962228..238` are 5 words (`Enable` is the LUT-copy trigger). The LUT itself is a texture the engine resolves **by name `<prefix>%04d`** (`LutSourceId` zero-padded) through a texture repository, once per frame in `FUN_14021CC20`; the prefix is a string the gparam manager returns for the record, and the two textures reach the library through fields `+0x358/+0x360` of the argument block of `FUN_1400E5110` (how the blend's output gets there was not found). So: the flag words are fed exactly (commit FW-8); the LUT texture is a live row (FF14) and until it is read the fallback of YEBIS YB-4c applies.
8. **Nine commits FW-1..FW-9** (§6), every rung opt-in by a flag file until its exit holds at the witnesses: the observers first (no behaviour), the system built but unpublished, the publish with slot 8 and the inline node update, the door, the lights, the wind bit, the light-shaft feed, the grading feed. **Sixteen live rows FF1..FF16** (§8); nine unit tests T-F1..T-F9 (§7).
9. **Five earlier statements change** (§10): EFFECTS-PHASE1 §5 step 5 (the node update), WIND-BLOCK open point 1 (closed), EFFECTS design §2.6/N14 (no wind cache), YEBIS-WAVE1-READY §1.4 (`E4700` does not need the real post object) and its grading sentence (which zones), WAVE1-PREP §2.1 (the flag name is opt-in, and the item order N1 → N2 → {N3, N4} gains the node-update transcription inside N1).

## 1. What the original does (verified)

### 1.1 The three request paths and the per-frame order (summary of the door note, with the new link)

`SprjGraphicsStep::STEP_Update` `D5B140` runs once per frame: the time tick `D57900` (native hook `UpdateNativeSimulationTime`), then `E6E750(SprjSfxImp, tick)`: pending SfxIns are spliced and started (`E6DCD0` for single and multi SfxIns), then, when `+0x58` is set, `[SceneCtrl+0x20] = [this+0x72]` and **slot 8 `24D000`**. Slot 8 (verified from the export): environment factors → resource tick → hide-mask swap → sound → time scale → **registry controllers** (`[[144962398]+0x20]` list, `vt+0x50(dt)`, return address `14024D347`) → **node-manager update `FUN_14025B870`** (call at `14024D360`) → the `+0x20` branch: the library update `FUN_142155920` through the combined pass. So a wind node is refilled from the registry state left by the **previous** frame's library update: a WindSFX effect started in frame N is seen by the nodes from frame N+1 on.

### 1.2 The wind feed: from a region to the block (new)

| Link | What | Evidence |
|---|---|---|
| Region | MSB `WindSFX` region with an FXR id; the creation path of region effects is the same door (§1.1); the effect is then an ordinary library instance | ENTRY-DOOR §4; `zone-force-fields.tsv` |
| Effect | one `WindForce` action (10300, shape `i2=Box`, `boxSize` 5 or 50, `cylinderHeight/Radius` 15/50 unused for a box, `forceMultiplier` 1, `forceRandomMultiplierMin/Max` −20/20 or −10/10, `fadeOutTime` 240 / 120 / 90 with a second value 60 / 90 / 30) plus an invisible billboard | `evidence/I-0068/queries/ffx-ready/zone-force-fields.tsv` (7 rows per table below) |
| Appearance | `GXFFX::GXForceFieldWindAreaAppearance` (vtable `143D65CC8`): slot 0 `140276290`, slot 4 `1402767C0` = the per-frame refresh of its registry entry (reads the entry through the handle at `appearance+0xD0`). **Registration happens when the appearance is built**: the type-table factory `140275DF0` (no direct caller; reached through the type registration) tests `action+0x240 == 0x283C`, allocates the 0xF0-byte appearance and calls `140276300` = `FUN_140259F60(registry, &key, &desc)` (AddEntry under the lock `registry+0xD8`: the descriptor's genre at `desc+0x30`, 1..7, builds the standard 0x240-byte entry object, vtable `143D64C68`; genre 2 = wind, 7 = cancel; the shape is at `desc+0x3C`; other genres go through the registry's virtual `+0x10`) then **`FUN_1402552C0(worldHandle, entry)`** (append to the area's bucket). Siblings: `GXForceFieldCancelAreaAppearance` (`143D65AF8`, update `1402757A0`, registration `140275470`), `GXForceFieldGravityAreaAppearance` (`143D65E38`, `14027C7C0`, `14027C370`), `GXCollisionFieldAreaAppearance` (`143D65EA8`, `14027D860`, `14027D540`) | RTTI map `I0067Rtti`; exports `i/`, `j/`, `ffx-ready/o` |
| Registry | `[144962398]` (vtable `143D64C40`): entry pointers at `+0x18[handle]` (count `+0x9C`), area records at `+0xA0[idx]` with flags `+0xC0[idx]` (count `+0xD4`), enabled byte `+0x110` (**every accessor tests it first**: a registry published with `+0x110 = 0` makes the whole chain silent; `PublishNativeFxWorld` logs it as `light_registry_enabled`). An area record holds a `std::list` of entry handles at `+0x08` (count `+0x10`) | `1402552C0`, `14029B890`, `1402594E0` |
| Entry object | `entry+0x40` kind (1 or 2), `entry+0x48` object: `+0x08` mask A, `+0x10` mask B, `+0x30` priority, `+0x34` byte copied from `desc+0x24` by `140259F60` (**verified**: `140275470`, the cancel registration, sets it to 1 with genre 7; `140276300`, the wind one, to 0 with genre 2; that it means "cancel" is **inferred** from the class name), virtual tests `+0x1A8` (kind-4 node, `1402A1E60`) and **`+0x1B0` (kind-3 node, the box, `1402A1F50`)**. **Where the masks come from (verified, reviewer-found):** `FUN_14029A5C0` reads the effect's root action `0x2774` (`Unk10100`): mask B = `(f1 != 0) | (f2 != 0) << 1 | (f3 != 0) << 2 | f13..f22 << 32..41`, mask A = `1 << f23` (all ones when `f23 < 0`), priority = `f0`. For `800081` (`f1 = f2 = f3 = f23 = 1`): mask B 7, mask A 2, priority 0, which matches a node with (A = −1, B = 4) and not the Eclipse bit 33 | `14029B4F0` and `14029B890` differ by that one slot; `14029A5C0` |
| Node | `FUN_140245870(world, 0x10, maskA, maskB)` = `FUN_14025B610(mgr, ...)`: a 0x180 object (`FUN_14025C4C0`): `+0x00` the **world handle**, `+0x10` shape kind (0 until `FUN_1402458B0(handle, box)` sets kind 3 and the 0x20-byte box at `+0x60`), `+0x120` vector of effect handles (capacity 16), `+0x148` mask A = **`-1`** for items and entities (`[142945250]` = all ones, read), `+0x150` mask B = **4** (wind class; `0x2_0000_0000` for an Eclipse model), `+0x160` result structure (0x1A54 bytes, allocated by `FUN_140245920`, **created once and kept**), `+0x170` optional per-frame callback (null for these nodes). A third creator exists: `FUN_14066F7A0` (caller `140669D90`, masks read from `14456F718/720/728`, condition `[obj+0x298]`), so nodes can exist without W1 | exports `a/`, `e/`, `f/` |
| Refill | `FUN_14025C700(node)` per frame: by shape kind 1..5 calls the collector `29BFD0`, `29BC30`, `29B890` (box), `29B4F0`, `1402553E0`, writing candidate handles into the scratch array at `node+0x100` (capacity dword `node+0x140`), then clears the vector `node+0x120` and appends them with `FUN_14025CBF0`. A collector (arguments: the area record, the shape at `node+0x60`, the scratch array, the capacity, the out count, mask A `node+0x148`, mask B `node+0x150`, the flags bytes `node+0x158/0x159`, both **1** from the constructor) does two passes over the bucket list: first, over the entries whose object has the byte `+0x34` set (the cancel areas, inferred from `FUN_140275470` setting that descriptor flag), those that pass the mask and box tests give the lowest priority `p` (start 100000000); second, it keeps every entry that passes `objectA & maskA`, `objectB & maskB`, the registry enabled byte and the box test `vt+0x1B0`, **and whose priority `+0x30` is below `p`**, until capacity. So a cancel area containing the node's box removes the winds of equal or higher priority from that node | `f/14029b4f0.c`, `q/14029b890.c`, `e/14025c700.c` |
| Evaluation | `FUN_140245F90(nodeHandle, forceObject)` (called by `D3E950` for items, `D07710` for entities): `FUN_140245700` clears the force object; if the manager flag `[mgr+0x4C]` is clear it takes the node result (`FUN_140245920` creates and zeroes it **only when absent**), `FUN_1402459A0` decays it, then for each handle of `node+0x120` it calls the entry object's virtuals (slots `+0x110` type, `+0x118`, `+0x130`, `+0x178`, `+0xD8` bounds, `+0xE0..+0xF8` time, `+0x100` transform through `FUN_1402451A0`, `+0x180/0x188`; these are virtual slots, not fields) and builds an area or a point record from the wind's bounds, a point drawn inside them (`FUN_140248070`), a fractal noise of time (`FUN_1417B3820`, 4 or 5 octaves) and per-slot fade accumulators kept in the node result (`+0xD10`, `+0xD50`, `+0x13D0`); **the node's own box is not read here** (it is read only by the collector). Then the first 8 area records and the first 8 point records with positive weight (and a handle ≠ −1) are written by `FUN_140247E80` to the force object | export `decompiled/DS3-1152-F-00245F90.c` (lines 503 to 1000; the node object is read at lines 212-217 and 547-549 only) |
| Block | `FUN_140D07E20` writes the 0x410-byte block of WIND-BLOCK §2 from the force object | WIND-BLOCK |

The WindSFX effects, verified (the `fadeOutTime` values are as printed by the schema reader, units not decoded):

| FXR | Bundle | Wind box / multipliers | Fade | Invisible billboard | WindSFX regions with this id |
|---|---|---|---|---|---|
| `800080` | commoneffects | Box 5, ±20, ×1 | 240 / 60 | yes (alpha 0, texture 11790) | `m31_00` ×1 |
| `800081` | commoneffects | Box 50, ±10, ×1 | 120 / 90 | yes | `m30_00` ×3, `m31_00` ×8, `m37_00` ×1, `m40_00` ×6 (and `m30_01` ×1) |
| `800082` | commoneffects | Box 50, ±10, ×1 | 90 / 30 | yes | `m31_00` ×10 (and `m30_01` ×2) |
| `845010` | m45 | Box 50, ±10, ×1 | 90 / 30 | no | `m45_00` ×8 |
| `850800` | m50, m51 bundles | Box 5, ±20, ×1 | 240 / 60 | yes | `m51_00` ×1 |
| `850801` | m50, m51 bundles | Box 50, ±10, ×1 | 120 / 90 | yes | `m50_00` ×1, `m51_00` ×1 |
| `850802` | m50, m51 bundles | Box 50, ±10, ×1 | 90 / 30 | yes | `m50_00` ×5, `m51_00` ×1 |

(Region counts from `msb-wind-regions-all-maps.tsv`. `850800..02` equal `800080..82` in every named field; three unnamed parameters differ (`unk_ds3_f1_42/43` are swapped, 0.5/0.1 against 0.1/0.5; `800080` is 8064 bytes against 8048). The MSB also has `WindArea` regions with no effect id: 3 at `m30_00`, 6 at `m40_00`, 6 at `m50_00`, 3 at `m51_00` (equal to the WindSFX counts there), none at `m31_00`, `m37_00`, `m45_00`; their runtime role is not decoded (§9 point 3). `800001` and `800083`, used by `m38_00` and `m30_01`, are outside the witness zones and were not read.)

### 1.3 The light-shaft and grading feed in `FUN_1400FB890` (summary; the full table is YEBIS-WAVE1-READY §1.1)

Lines 349 to 355: key lookup `FUN_14016B030(root+0x4628, context+0x4020 / +0x4088)` (A and B) → blend `FUN_14021D690(out, A+0x10, B+0x10, weight at context+0x40CC)` → converter `FUN_1400E4700(post, out)` when `post+0x2270 == 0`. Lines 452 to 488: key lookup `FUN_14017BEE0(root+0x4650, context+0x3FF8 / +0x4060)`; each record contributes **`record+8`, or the fade snapshot `record+0x250` when the byte `record+0x4EC` is set**; blend `FUN_14021C150(out, A', B', weight at context+0x40B8)`; stores to `144962228` (word 0), `14496222C`, `144962230`, `144962234` (words 1 to 3), `144962238` (word 4), `1449622C8/D0` (two 8-byte fields of the output) and three wide strings to `post+0x2100`, `+0x2130`, `+0x2160` (`FUN_1400ECA10`). A first site at lines 221 to 253 does the same blend into the **view context** (`context+0x30F0`, `FUN_14017A9F0`), gated by the Setting Info capability bit 38 (`context+0x50 & 0x4000000000`) and by `context+0x3688 == 0`; the library globals of the second site have **no capability gate** (so the group's `Enable` alone decides the library's LUT copy, consistent with YEBIS-CHAIN §4.4).

## 2. Native design: the system, slot 8, the node update, the door

### 2.1 What exists today (observed in the tree at `933a9431`)

`native_sfx_manager` (`GB:421`), `PublishNativeSfxManager` (`GB:10134-10175`, logs `effect_system:false`; `+0x38..+0x58`, `+0x60` null), `NativeSfxViewAttach/Detach` (`GB:10176-10189`, refuse when `+0x58` or `+0x60` is set), `NativeSfxStartup` (`GB:10190-`, once at archive state 4: CPU types `1A97EE0`, then `PublishNativeFxWorld` (`GB:10093`, genuine registry `2594E0/2596D0`, world `248CC0`, area handle `259C60`, node manager `25B280/25B370`) and the manager), hook table entries `GB:10526-10529` (startup, attach, detach) and `GB:10600` (`7CB820` → `DeferNativeSfxSceneRequest`, `GB:8664-8698`), the zone-entry leaf `E6E620` called from `RetireNativeZoneCpu` (`GB:8607-`) with a pinned-code check and the identity check `manager == native_sfx_manager`. The light consumer `CollectNativePointLights` (`GB:2858`); the wind consumer in `PrepareNativeModelFrame` (`GB:5618`, `put(0x80, animation_block, 0x410)` with the zero fallback); the material builder that ORs `0x2000|0x800` into `instance+0x24` (`GB:8408`). Nothing hooks or counts `E6E750`, `D5B140`, `E6DCD0`, `E6DA30`, `E6DBC0`, `7CB8F0`, `7CB750`, `D3E950`, `245F90`, `25B870`.

### 2.2 The system and slot 8 (N1): EFFECTS-PHASE1 as written, plus three additions

Everything in EFFECTS-PHASE1 §2 to §7 stands (construction in `E6C9D0`'s order, scene control and device initialisation transcribed, slot 8 single-thread branch transcribed, view attach/detach, minimal layouts). Additions of this sheet:

1. **The device token** (`mgr[3]`, `[sc+0x28]+0x08/+0x178`): a pointer into a reserved `PAGE_NOACCESS` region (not a heap object), so that the first dereference of the absent `[144961AB0]` that a transcription missed faults at a known instruction instead of reading garbage; the existing fault reporting records the faulting address. PHASE1 §7 says "any non-null native object"; this is the stricter choice.
2. **Type registration check.** The list copied from `24B260` step 24 contains the four force-field types and the light-shaft effect type: `0x27D8` (10200 cancel), `0x283C` (10300 wind), `0x283D` (10301 gravity), `0x283E` (10302 collision), `0x2713` (10003 light shaft), `0x261` (609 point light), `0x2AF8` (11000 spot). After the construction, T-F1 asserts each is present in the library registry `[144933088]` (sealed at `+0x20`) and in the GX registry `[1449651D0]` (sealed `DAT_1449651E8`).
3. **The node-manager update is transcribed, not called** (§2.3).

### 2.3 The node-manager update, transcribed (`FUN_14025B870`, inside slot 8)

Original (export `ffx-ready/f/14025b870.c`): (1) if `[mgr+0x45]` is set, decrement `[mgr+0x48]` or clear the flag at 0 (a debug hold); (2) `[mgr+0x44] = 0`; (3) take a profile timestamp and **copy the node pointers of the list `[mgr+0x10]`** (each list entry holds the node at `+0x10`) into a vector built by `FUN_14025B020`; (4) **if `[mgr+0x30]` (live nodes) is not zero**: allocate a job from `*[144961BB0]` (class vtable `143D65508`, slot 1 = `14025B1F0`), run it through `[144961AB0]+0x2C8` over the range `[0, [mgr+0x30])` in blocks of min(n, 10) (inline when the queue accepts it, else enqueued); the job body is, per node: `if ([node+0x170] && [node+0x160]) [node+0x170] called with dt; FUN_14025C700(node)`; (5) store the elapsed milliseconds in `[mgr+0x50]`. `[mgr+0x30]` is incremented in `14025BCA0` and decremented in `14025B750` together with the list; node creation (`14025B610`) and release (`14025B750`) take the lock `mgr+0x58` and may run on other tasks, `14025B750` freeing the list links.

Native (proposed, `TranscribeNodeManagerUpdate(mgr, dt)`): steps (1) and (2) as is; **step (3) kept**: copy the node pointers of `[mgr+0x10]` into a local vector (under the lock `mgr+0x58`, which creation and release take, so that a concurrent release cannot free a link under the walk); step (4) replaced by the loop over that vector calling the job body per node in order, **single-threaded, in the thread of `E6E750`**; step (5): store `0.0f`. The job body is pure CPU: `FUN_14025C700` reads the registry and writes the node (`+0x120` vector, `+0x160` result); the callback is null for every node the item and entity constructors create. **Why in line with the original:** in the original the job runs inline in the same call whenever the queue accepts it, over a copy of the pointers; any thread split it does is over independent nodes. **Check (T-F2):** run the genuine job body `FUN_14025B1F0` on a synthetic holder (`FUN_14025B020`) and compare, on the same nodes, the number of calls and the node order with the inline loop. **What the exit can and cannot show:** nodes exist without W1 only through `FUN_14066F7A0` (condition `[obj+0x298]`, unread); if none exists in the witnesses, FW-3's exit exercises the empty branch only (`[mgr+0x30] == 0`) and the inline loop is first exercised live by FW-6 (W1), so FW-3 and FW-6 must be run together at m31 before either is declared done.

GX statements of the wind chain, all read: the 18 functions `14025B610`, `14025C4C0`, `14025BCA0`, `14025B870` (this one), `14025B1F0`, `14025C700`, `14025C650`, `140245870`, `1402458B0`, `140245700`, `140245920`, `1402459A0`, `140245F90`, `14029B4F0`, `14029B890`, `1402553E0`, `1402552C0`, `140259F60`. Only `14025B870` references `[144961AB0]`/`[144961BB0]` (step 4); `1402AA130` (the FX world teardown) and `140248E30` (world init, already replaced by native stores in `PublishNativeFxWorld`) also do and are not on the wind path.

### 2.4 The door (N2): delete the deferral behind the publish

`DeferNativeSfxSceneRequest` stays while the effect system is not published (the previous behaviour, opt-out of nothing), and the hook entry `GB:10600` becomes a **forwarder** that, once `native_effect_system_published` is true, calls the original `7CB820` (and counts it, §6 FW-4). The other five creators (`E6DCD0`, `E6DA30`, `E6DBC0`, `7CB8F0`, `7CB750`) get pass-through observers (FW-1) and no replacement. Requests deferred before the publish are **not** replayed (FFX-ATTACHED risk 3). The deferral code, its struct and its generation counter are deleted in FW-9 only, after the exits hold and the flag is the default.

### 2.5 Effect lights (N3) and the ordering

Nothing is added to the consumer (`CollectNativePointLights` keeps the point-class entries of `[[GXRS]+0x41D0]+0x10`; it is called from `CollectNativeSelectedScene`, `GB:5242`, itself called from `UpdateNativeSceneCpu`, `GB:6106`, the hook on `E006A0`, table entry `GB:10493`). The ordering constraint of EFFECTS design §2.7 stays a **measurement first**: FW-5 counts `E6E750` (inside `D5B140`) against that collect in the same frame and logs both orders on 600 frames; the fix (call `E6E750` before the collect and guard the other) is written only if the effect lights lag a frame (they would show as 59 natively against 163 at the second frame only).

### 2.6 The wind (N4): W1 and the node chain

W1 of WIND-BLOCK §5 stands (policy bit 1 on `instance+0x24`, generalised by MTD-FLAG-BITS F1), with two new preconditions: (a) FW-3 (inline node update) is published, otherwise `FUN_14025B870` dereferences the absent display object at the first frame with a node; (b) the world handle `[[144962360]+8]` is valid and `registry+0x110 == 1` (checked at W1 time; refuse and log if not). The chain after W1 is the engine's: `FUN_14025B610` registers the node; `D3E950` / `D07710` update its box (`1402458B0`), evaluate (`245F90`) and set the block; native reads the block as today. W1 creates nodes for every grass item, so the first frame after W1 is the first live run of the inline loop at the zones.

## 3. The seven families, with the witnesses of the zones

Counts are upper bounds or weights from the linked notes, never live counts; "obs" marks an observed capture. Zones: `m30` High Wall (Argo's torch), `m31` Undead Settlement, `m37` Irithyll, `m40` Firelink, `m45` Ariandel, `m50` Dreg Heap, `m51` Ringed City.

### F1. Persistent object and map effects (N1 + N2)

Sources (ENTRY-DOOR §1.3): `ObjModelSfxSlot` (`ModelSfxParam` row model×1000, one single SfxIns per entry and dummy), `ObjMultiSfxSlot`, `ObjShineTreasureSlot`, the bonfire condition, the object init route, `ObjBurnSlot` (conditional). All engine code; native provides `+0x58`, slot 8 and the forwarder.

| Zone | `ModelSfxParam` objects (models) | Ids created (upper bounds) | First check |
|---|---|---|---|
| m30 | 353 (13) | 800023 ×133 (35 torches `o304863`, 98 `o004660`), 839010 ×105, 800030 ×72, 800043 ×36, 800052 ×33, 800050 ×21, 800060 ×14 | E-D8: id 800023 at dummy 200 for the 35 torches |
| m31 | 622 (10) | 839010 ×485, 831021 ×65, 831022 ×44, 800061 ×18, 800060 ×9 | E-D3: the best statistics (839010) |
| m37 | 27 (3) | 800044 ×44, 800023 ×12 | ids ⊆ table |
| m40 | 422 (3) | 839010 ×409, 800022 ×12 | bonfire flame (F1 of FFX-ATTACHED) |
| m45 | 186 (11) | 845220 ×40, 845205 ×30, 845070 ×26, 845223 ×16, 845206 ×8, 839010 ×43 | falling-snow emitters |
| m50 | 8 (3) | 850000 ×6, 850080, 850140 | weak witness: do not start here |
| m51 | 316 (10) | 851070 ×124, 839010 ×73, 800020 ×45, 800022 ×30, 838100 ×25, 851001 ×16 | grass instances (E03) |

Native work: FW-3 (publish), FW-4 (forwarder). Exits: E-D1 (creators = branch takers), E-D2 (caller histogram equals the original's, first entries `E6BED0`), E-D3 (ids ⊆ table, count ≤ instances), E-D4 (pending list empty after each `E6E750`). Pitfall carried: the SfxIns visibility gate follows the native display-group state (ENTRY-DOOR §7.1): E-D3 can miss ids while E-D1 is clean. Child spawns inside the library (FFX-ATTACHED, ENTRY-DOOR §1.3 last row) are not requests: FF13 compares library instance count with handle count.

### F2. Character, event and sign effects (N1 + N2)

Sources: the character slots (`7CBEA0` follow binding `2447C0`: held flame, lantern, burn, weapon enchantments), the character SFX manager (`7CBBA0`), the event commands, `SosSignMan` (id 30001 per visible sign, flags 8), bullets, cutscene (`E6DBC0`). The follow is the library's own update (slot 8). Witnesses: m40 with the Pyromancy Flame (F2 of FFX-ATTACHED: two lights within 1 m of the player, radius 0.5 and 2.5, the instance follows the hand with no new request); m30 with an Ember (E-D9: `7CB820` id 30001 per sign). The zone-weight columns do not apply (these effects depend on actions): no check at m37, m45, m50, m51 beyond E-D2's "same set of callers".

### F3. Effect lights, E06 (N3)

Hosts `GXFfxDrawEntityHostFrpgPointLight` (type `0x261`) push a `GXPointLight` into `[[GXRS]+0x41D0]+0x10` during the library update; spot lights (`0x2AF8`) take the map-light path (`FUN_14024ED10` → `FUN_140018380`); line lights are not read (EFFECT-LIGHTS §2.1). The 104 effect lights of the m40 view are all point lights in seven parameter families (EFFECT-LIGHTS §3: 85 small warm radius-2 lights, 8 radius-5 fires, 5 radius-3, 3 radius-6, one radius-0.5 near the player, one cold radius-2.5, one radius-3.2).

| Zone | E06 weight (FXR actions in the zone bundles) | Expected effect-light entries |
|---|---|---|
| m31 | 64 | fewer than m37; weak witness for the lights |
| m37 | 170 | native equals the original (FX9) |
| m40 | 40 | **163 in the vector = 59 map + 104 effect** (R7) |
| m45 | 132 | equal |
| m50 | 170 | equal |
| m51 | 290 | equal: the largest |

Native work: FW-5 only (counters, order). Exits: R6 (about 104 point-light factory calls at m40), R7 (163 entries), R8 to R10 only for lights that exist but stay dark (hide mask is 0 outside cutscenes; light mask low word `0x11` or `0x01`, high word `1`). `native_point_lights` equals the original's at m37, m50, m51 (FX9).

### F4. Force fields and the foliage wind (N4)

§1.2 and §2.3/§2.6. Witnesses (WIND-BLOCK §7, ENTRY-DOOR §4):

| Zone | WindSFX regions (FXR ×count) | Grass | Expected |
|---|---|---|---|
| m30 | 800081 ×3 | 39 pieces / 39 | at idle no source near the spawn: counts 0 with flag 1 (WD-1, WD-3) |
| **m31** | 800082 ×10, 800081 ×8, 800080 ×1 | 96 / 351, `o317000` ×1 | **main witness**: counts 1..8 inside a region, the grass sways |
| m37 | 800081 ×1 | 28 / 28 | one region |
| m40 | 800081 ×6 | 47 / 216 | six regions |
| m45 | 845010 ×8 | 46 / 97 | no billboard in the effect |
| m50 | 850801 ×1, 850802 ×5 (WindArea 6) | 7 / 192 | DLC |
| m51 | 850800, 850801, 850802 ×1 each (WindArea 3) | 65 / 147 (+ instanced FFX grass E03) | DLC |
| `m31_01`, `m31_03..m31_09` | `31001` ×8 each | objects `o317000` ×163 / ×124 | **no wind**: `31001` has no force field (§0.5) |

Native work: FW-3 (inline node update), FW-6 (W1). Exits: FF5 to FF9 (§8), WD-1 to WD-4 of the wind sheet. What each row decides: FF5 the entries registered (`140259F60` and `1402552C0` counts per 600 frames against the WindSFX effects created); FF6 the bucket size and node vector sizes; FF7 counts 1..8 against the original; FF8 whether two items of one region get equal records (answers the cache question; the code shape says no); FF9 the lifetime rule (a WindSFX effect has a finite fade-out: who recreates it is the region owner's rule, not decoded).

### F5. Volumetric light shafts, YEBIS bank 9 (feed, FW-7)

§4.1. Witnesses: observed shafts at **m31 (48 draws) and m50 (41)**, none at m37, m40, m45, m51; the zone files' id 0 says `Enable` 0 at m31 and 1 elsewhere, so the observed library gate is not the file's `Enable` of id 0 (FF10 reads the keys).

### F6. Colour grading, YEBIS bank 4 (feed, FW-8)

§4.2. Witnesses: observed grading draw at **m31 and m40**; the group `Enable` of id 0 is 1 at m30, m31, m40, m45, m50, m51 and 0 at m37; of the seven files only the m40 and m50 files carry a Setting Info group at all (its `Enable ColorGrading` is 1 in both; m30, m31, m37, m45, m51 have none, so the default 0 applies); `LutSourceId` m30 2, m31 1, m40 3, m50 1, m45 and m51 0, m37 −1.

### F7. Draw families E01 to E08 (not wave 1)

Wave 1 gives them a running library: instances, appearance updates, the hide-mask swap and the registry. It draws nothing, so the draw weights stay as in FFX-FAMILIES §7 and EFFECTS design §4: E01 (m31 564, m37 1490, m45 2346, m50 2066, m51 2986), E02 (42, 126, 132, 46, 124), E03 (84, 106, 172, 78, 286), E04 (24, 54, 10, 12, 22), E05 (36, 112, 82, 58, 134), E07 (10, 14, 32, 4, 14), E08 absent from the five zones (present in the m30, m33, m38 bundles). Two things wave 1 changes for them: the WindSFX effects' invisible billboards (alpha 0) are part of the E01 action counts of m30, m31, m37, m40, m50, m51 and must draw nothing visible (FX1 baselines include them); `FX13` (library instances against handles) becomes readable.

## 4. The two YEBIS feeds (new builders; the others are YEBIS-WAVE1-READY YB-1..YB-4)

### 4.1 Light shafts: the 28-word record, its blend and its converter (verified)

Record `GXBS::GXLightShaftFilterParam` (vtable `143D62230`, ctor `FUN_14021D8D0`, reflection `FUN_14021E2D0`); `FUN_16B030` returns the record, the block is `record+0x10`. Word `w` = (record offset − 0x10) / 4.

| Word | Content | Source |
|---|---|---|
| 0 | `Enable` (int) | file entry 0 |
| 1 to 3 | padding (alignment of the `float4`) | never read |
| 4 to 7 | `ColorScale` (`float4`) | file entry 1 |
| 8 | `ShaftLength` | file entry 2 |
| **9** | record-only, **default 0** (copied from B by the blend, global `1449620C0`) | ctor `+0x34 = 0` |
| **10** | record-only, **default `0x3F7FFF58`** (lerped, global `1449620BC`) | ctor `+0x38` |
| 11 | `GlareRatio` | file entry 3 |
| 12, 13, 14 | `AngleAttenuation`, `NoiseMask`, `NoiseFrequency` | entries 4, 5, 6 |
| 15, 16 | `Rad-X`, `Rad-Y` (file `AngleX`, `AngleY`) | entries 7, 8 |
| 17 | `MaskDepthThereshold` (global `1449621BC`) | entry 9 |
| 18 to 21 | `DiffractionRing`, `...Radius`, `...Attenuation`, `...SpectrumOrder` | entries 10 to 13 |
| 22, 23 | padding | never read |
| 24 to 27 | `RingOuterColor` (`float4`) | entry 14 |

The mapping is consistent three ways: the converter's registered global names (`yebis-globals.tsv`), the reflection offsets of `FUN_14021E2D0` (`+0x30` ShaftLength, `+0x3C` GlareRatio, `+0x40..+0x64` the next nine, `+0x20` and `+0x70` the colours) and the file order. The ctor defaults of the other words (Enable 0, ColorScale 1111, ShaftLength 1, GlareRatio 0.75, AngleAttenuation 10, NoiseFrequency 10, MaskDepth 2000, ring radius 0.2, attenuation 0.01, spectrum order 1, ring colour 1111) equal `default.gparam` id 0 except `NoiseMask` (ctor `0`, file 10), irrelevant when the file supplies it.

**Blend `FUN_14021D690(out, A, B, w)`** (verified, 0x70 bytes = 14 qwords): at `|w| <= ε` copy A wholesale; at `|w − 1| <= ε` copy B wholesale (`ε` = `[image+0x3D7ECE8]`); otherwise copy **B's words 0 and 9** and lerp `(b − a)·w + a` the words **4 to 8, 10 to 21 and 24 to 27**; words 1 to 3, 22 and 23 are not written.

**Converter `FUN_1400E4700(post, block)`** (verified; `post+0x2270` must be 0): factor `f` = `[[GXRS+0x5368]+0x50]` when `[GXRS+0x5360] != 0`, else `1.0` (`[image+0x3D7EE10]`); `1449620A0 = (f <= 0) ? 0 : w0`; `1449620A4/A8/AC = w4/w5/w6`; `1449620B0 = f·w7`; `B4 = w8`, `B8 = w11`, `BC = w10`, `C0 = w9`, `C4..CC = w12..w14`, `D0/D4 = w15/w16`, `1449621BC = w17`, `D8..E4 = w18..w21`, `E8/EC/F0 = w24/w25/w26`, `1449620F4 = f·w27`; and into the object `post+0x21E0/0x21E8 = w4..w7` twice (`+0x21F0/0x21F8` the copy) and `post+0x2200/0x2208 = w24..w27` twice (`+0x2210/0x2218`). It reads nothing else of `post`: **it can run on the zero-filled scratch object of YEBIS rungs V0/V1** (this corrects YEBIS-WAVE1-READY §1.4, which said the real post object); `E4890` (depth of field) still needs the real one.

**Goldens** (`bank-blocks.tsv`, id 0, `f = 1`): the 28 words and the 23 globals for the seven zones and `default`. Example, `m31_00` (observed shafts, file `Enable` 0): words `0, pad, pad, pad, 1,1,1,1, 1.5 (3FC00000), 0, 0.99999 (3F7FFF58), 0.75, 5, 10, 1.81 (3FE7AE14), 25.0808 (41C8A579), −8.85237 (C10DA34E), 2000, 0, 0.2, 0.01, 1, pad, pad, 1,1,1,1`; `1449620A0 = 0`. `m50_00`: `Enable 1`, `ColorScale (1, 0.790788, 0.743662, 1.352113)`, `ShaftLength 1.23`, `GlareRatio 0.78`, `MaskDepth 800`, `Rad-X 12.61112`, `Rad-Y 126.1167`.

**Which record, though (open, live).** The zone files carry LightShaft only at value id 0, so the golden above is "what id 0 gives". The observed shafts (m31, m50) and their absence (m37, m40, m45, m51) do not follow id 0's `Enable` (m31 is the only zone with 0). FF10 reads, at the frames that show shafts, the keys A and B of index 9 (`context+0x4020`, `+0x4088`), the weight (`+0x40CC`), the record words and the globals `1449620A0..F4`; the builder (FW-7) takes the key from the context exactly like the Bloom builder, so the gap is a data question, not a code one.

### 4.2 Colour grading: the 5-word block, the fade snapshot, the blend and the LUT (verified, one link live-only)

Record class vtable `143D621C8` (ctor `FUN_14021C510`, size 0x508): the **block** at `record+8` (ctor `FUN_1400F8CE0`, `FUN_14021C780` loads it from the group) has, in dwords: `[0]` `Enable` (default 0, file entry 0), `[1]` `LutSourceId` (default −1, file entry 1), `[2]` and `[3]` ids (default −1), `[4]` fade progress (float, default 0), a wide-string name at block `+0x18`, two more name copies at `+0xC8`, `+0x178`, and three 8-byte texture fields at `+0x228`, `+0x230`, `+0x238`. A **fade snapshot** with the same layout sits at `record+0x250`; `record+0x4EC` (byte) says a fade is in progress (then FB890 reads the snapshot), `+0x4F0` / `+0x4F4` are the elapsed time and the duration.

**Per frame** `FUN_14017BE20(bank, dt)` (reached from the post-effect update `FUN_1400499C0`; no native hook drives it) calls `FUN_14021CC20(record, dt, repo)` for the bank's default record (`bank+0x5A0`) and for every keyed record: when the record's id changed it drops the old texture; if the texture field `+0x230` is empty it **builds the name with `FUN_14001B4B0(L"%s%04d", name wide-string at +0x20, LutSourceId)`** and asks the texture repository (`vt+8`) for it; `repo` = `[[GXRS]+0x47C8]`, else `[[GXRS]+0x47B0]`, else `[1449622E0]`. The name string comes from the gparam manager (`FUN_140094B60([[GXRS]+0x4788], file id)`), so it is **a string of the gparam set, not a value of the dumps** (the dump TSVs carry integers and floats only).

**Blend `FUN_14021C150(out, A', B', w)`** with `A'`, `B'` the chosen blocks (output block of the same layout):

| Weight | out[0] | out[1] | out[2] | out[3] | out[4] | Names / textures |
|---|---|---|---|---|---|---|
| `w <= 0` | `A[0]` | `A[1]` | `A[1]` | `B[1]` | 0 (or, when `A[4] > 0`: `A[2]`, `A[3]`, `A[4]` and A's fade texture fields) | name 0 and 1 = A's, name 2 = B's; output fields `+0x228` and `+0x230` = A's `+0x228`, `+0x238` = B's `+0x228` |
| `w >= 1` | `B[0]` | `B[1]` | `B[1]` | `B[1]` | 1.0 (or, when `B[4] > 0`: `B[2]`, `B[3]`, `B[4]` and B's fade texture fields) | all B's (output `+0x228`, `+0x230`, `+0x238` = B's `+0x228`) |
| `0 < w < 1` | **`B[0]`** | `A[1]` | `A[1]` | `B[1]` | `w`; then, when A's progress is in (0,1): `out[2] = A[3]`, `out+0x230 = A+0x238`; when B's is in (0,1): `out[3] = B[3]`, `out+0x238 = B+0x238` | as `w <= 0`. The block has four 8-byte fields (`+0x228` to `+0x240`) |

Globals: `144962228 = out[0]` (**the LUT-copy trigger**), `14496222C = out[1]`, `144962230 = out[2]`, `144962234 = out[3]`, `144962238 = out[4]` (read twice by `FUN_1400E5110` as a constant and in `0 < w < ...` to choose between the two compute variants), `1449622C8 / 1449622D0` = the output's texture fields, and the three strings at `post+0x2100/+0x2130/+0x2160` (assigned with `FUN_1400ECA10`).

**The LUT reaches the library in `FUN_1400E5110`** (when `144962228 != 0`): it reads two texture objects from **`+0x358` and `+0x360`** of its third argument (assembly `1400E51C2 MOV RDI,[R8+0x358]`, `1400E51C9 MOV RSI,[R8+0x360]`; the `+0x6B0/+0x6C0` of the decompile is an `int*` indexing artefact) and, if their two size queries (`vt+0x40`, `vt+0x48`, in this order) do not return 16 and 256, or the objects are absent, or `post+0x2191 != 0`, it falls back to the library's default texture (`post+0x68`). The accepted size is 16 by 256 (16 slices of 16×16 stacked), which `LutCopy2DTo3DCs` unwraps into the 16³ LUT; YEBIS-HAL-READY §8 records a 16×16 `R32G32B32A32` source at m31, which by this test would take the fallback or be one slice: FF14 reads which. The third argument comes from `FUN_1400E8100` (reached through the data `144978854`); 0x358 − 0x230 = 0x360 − 0x238 = 0x128, so that argument probably holds a copy of the blend's output block at `+0x128` (**inferred**; the two fields would then be the ones also stored in `1449622C8/D0`). No static reader of `1449622C8/D0` was found; the copy into the argument was **not searched** (open, row FF14).

**Zone values** (`bank-blocks.tsv`, id 0, weight 0): `144962228..234` = (1,1,1,1) at m31, (1,3,3,3) at m40, (1,1,1,1) at m50, (1,2,2,2) at m30, (1,0,0,0) at m45 and m51, (0,−1,−1,−1) at m37; `144962238 = 0`.

**Native design (FW-8).** `Banks::GradingBlock(zone/key)` reads the two integers; the blend is the genuine `FUN_14021C150` called with two native-built blocks (constructed with `FUN_1400F8CE0` on 0x248-byte buffers, a name string assigned with `FUN_140033D90`, texture fields null) and an output block built the same way; a port of the table is **not** recommended (four 8-byte fields, string copies, two fade branches: T-F6 compares the genuine call with the table only to guard the table's wording); the five globals are written under the converter's condition (`post+0x2270 == 0`). The per-frame record update (`14017BE20`) is **not** reproduced: the key weight `w` already carries the transitions, and the record's own timed fade (`[4] > 0`) is a bank-level API that the native feed does not drive (consequence: a zone change cross-fades by the key weight, as the library's other groups do). Until FF14 is answered the library host may not enable the grading draw: force `144962228 = 0` and `144962224 = 0` and log it (YB-4c fallback), which removes one draw and the LUT copy at m31, m40, m50 from the V2/V4 lists.

## 5. Goldens and oracles

- `bank-blocks.tsv` (`Get-FfxBankBlocks.ps1`): per zone, the 28-word light-shaft block, the 23 `E4700` globals (factor argument default 1.0) and the five grading globals. Regenerated from `evidence/I-0068/queries/yebis/zone-gparam`.
- `zone-force-fields.tsv` (`Get-ZoneForceFields.ps1`, bundles recovered with `Recover-GameData.ps1` into a scratch directory outside the repository): the actions of the WindSFX effects with the field list of every force action.
- Oracles that need no GX: the genuine `FUN_14021D690` (blend), `FUN_1400E4700` (converter, on a 0x2400-byte scratch object), `FUN_14021C150` (grading blend), `FUN_14025B1F0` (the node job body), `FUN_140D07E20` (the wind block writer, WIND-BLOCK W5).

## 6. Commits (each builds, passes the unit tests, is reversible alone; every rung is opt-in by a flag file next to the module until its exit holds, then the default)

| # | Commit | Touches (anchors by function) | Flag | Exit |
|---|---|---|---|---|
| **FW-1** | observers, no behaviour change: pass-through counters on `E6DCD0`, `E6DA30`, `E6DBC0`, `7CB8F0`, `7CB750`, `E6E750`, `D3E950`, `D07710`, `245F90`, `25B870`, `140259F60`, `1402552C0` with identifier / flags / `_ReturnAddress` histograms and the order of `E6E750` against the light collect; row `native_ffx_door` (first three windows, then every 600th) | the hook table beside `GB:10526` and `GB:10600`; a new `native/frame/ffx_observe.cpp` | `ffx-observe.flag` | baseline rows at the five zones with every creator at 0 and `E6E750` at 1 per frame (T-F4) |
| **FW-2** | **build, do not publish**: scene control (`24B260` transcription), manager, resource manager, passes, token (§2.2), registries sealed; `+0x58` stays null | `PublishNativeSfxManager` tail; new `native/frame/effect_system.cpp` | `ffx-system.flag` | T-F1; R2; no behaviour change (`+0x58` null) |
| **FW-3** | **publish**: slot 8 transcribed (single-thread branch), the inline node update (§2.3), view attach/detach, then store `+0x58` and `sc+0xB0` | `NativeSfxViewAttach/Detach`, `effect_system.cpp` | `ffx-system.flag` + `ffx-publish.flag` | R1, R4, E-D1, E-D4, E-D5; 600 frames at m40 and m30 with zero faults on the token page; T-F2 |
| **FW-4** | the door: `7CB820` forwarder (original once published), the other creators unchanged | `GB:10600`, `DeferNativeSfxSceneRequest` | `ffx-publish.flag` | E-D2, E-D3 (five zones), E-D8 at m30, E-D9 with an Ember, E-D7 at a zone change |
| **FW-5** | effect lights: the `E6E750` / collect order rows (FW-1 data) and the `native_point_lights` equality; the ordering fix only if lights lag | `UpdateNativeSceneCpu` (`GB:5979`, collect at `GB:6106`) | `ffx-publish.flag` | R6, R7 (163 at m40), FX9 at m37, m50, m51, FX11 |
| **FW-6** | wind: W1 (policy bit 1) behind `ffx-wind.flag`, with the two preconditions of §2.6 and the `native_wind_block` row (W4) | the material builder beside `GB:8408` | `ffx-wind.flag` (+ publish) | WD-1..WD-3, FF5..FF8 at m31 |
| **FW-7** | light-shaft feed: `Banks::LightShaft28`, `LightShaftBlend`, `E4700` call on the scratch object, the globals row | `native/frame/banks.*` beside YB-1, `YebisConverters` | `yebis.flag` (YB-4b) | T-F5; the globals row equals `bank-blocks.tsv` for the id in use (FF10) |
| **FW-8** | colour-grading feed: `Banks::GradingBlock`, the blend, the five globals; the LUT trigger forced to 0 until FF14 | same | `yebis.flag` (YB-4c) | T-F6; globals equal the table; FF14 recorded |
| **FW-9** | cleanup after the exits hold at the five zones plus m30/m40: flags become defaults, the deferral code (`GB:8664-8698`, struct `GB:400`) deleted | — | — | unchanged observers |

Order: FW-1 → FW-2 → FW-3 → FW-4 → {FW-5, FW-6}; FW-7 and FW-8 depend only on YB-1 and are independent of FW-1..FW-6 (they need the YEBIS bank accessors, not the effect system). Shared files (`GB`, `SP`, `MF`) are edited by other waves: each edit is at a block named above.

## 7. Tests

| ID | What | Pass |
|---|---|---|
| T-F1 | construction invariants after FW-2: vtable `143D64008`, `sc+0x70 == sc`, pass ids `0x20/0x21/0x2D/0x31/0x29`, `+0x44 == 1`, ring size `+0xB8 = 0x200000`, registries sealed, the seven types of §2.2 present | all equal |
| T-F2 | inline node update against the genuine job body `14025B1F0` on N synthetic nodes (`245870` + `1402458B0`), empty and non-empty world bucket | same call count and order; node vectors equal |
| T-F3 | the matching rule on a synthetic world bucket: entries built with `140259F60` from a descriptor like `140276300`'s (genre 2, flag byte 0) and a cancel one like `140275470`'s (genre 7, flag 1), objects with the masks of `800081` (A 2, B 7, priority 0), appended with `1402552C0`, nodes with masks (−1, 4) and boxes inside and outside; collector `14029B890` result | inside → in the vector, outside → not; mask mismatch → not; a cancel entry of priority 0 containing the box removes the priority-0 wind |
| T-F4 | observer histogram code (pure C++) | counts by `_ReturnAddress` |
| T-F5 | light-shaft builder against `bank-blocks.tsv` (28 words for the 8 files) and the genuine `FUN_1400E4700` (scratch post; the factor is read from the scene root `[144961B50]`, so `f` = 0 and 0.5 need a scratch root object with `+0x5360` and `+0x5368` pointing to scratch objects, `+0x50` set) and `FUN_14021D690` (w = 0, 0.25, 1, A = m31, B = m37) | bit-exact |
| T-F6 | grading builder against the table and the genuine `FUN_14021C150` on native-built blocks, three branches and a fade (`[4] > 0`) | words and globals bit-exact |
| T-F7 | WIND-BLOCK T-L1, T-B1, T-S1 (unchanged) | as there |
| T-F8 | the six reflection offsets of the light-shaft record vs the converter names (static table test) | equal |
| T-F9 | `static_assert` of the new structs (`NativeEffectSystem`, observer rows) and the token-page guard (a read through the token faults, a read of `+0x58` does not) | compiles / faults |

## 8. Live rows (none taken; added to LIVE-CHECKS-CONSOLIDATED §1.13)

| ID | Kind | Orig/Nat | Read | Expected |
|---|---|---|---|---|
| FF1 | NOW | Nat | first-publish log: `light_registry_enabled` of `native_fx_world_published`, `registry+0x110`, the world handle `[[144962360]+8]` ≠ −1, its area record present (`+0xC0[idx] != −1`) | 1, valid, present |
| FF2 | NOW | Orig | at m31: `[mgr+0x30]` (live nodes), the creators of nodes by caller (`140245870` from `D3C4D0`/`D05C80`, `14066F7A0`, others) and the call count of `14025C700` per frame | nodes = items with a handle + other creators; calls = nodes; tells whether FW-3 alone exercises the inline loop |
| FF3 | ACC | Nat | after FW-3: faults on the token page over 600 frames at m40 and m30; calls of `14025B870` per frame | none; 1 per frame |
| FF4 | NOW | Orig | the type registry: presence of `0x27D8`, `0x283C`, `0x283D`, `0x283E`, `0x2713`, `0x261`, `0x2AF8` after `24B260` | all present |
| FF5 | NOW | Orig, then Nat | counts of `140259F60` and `1402552C0` per 600 frames at m31, against the WindSFX effects created (`E6DCD0` with ids 800080..82) | one registration per WindForce action instance (the registrar is the appearance, `140276300`) |
| FF6 | NOW | Orig, then Nat | the world bucket size (`[[reg+0xA0][world idx]+0x10]`) and the sizes of `node+0x120` for 20 grass items at m31 | bucket = live wind entries; node vectors 0..16 |
| FF7 | NOW | Orig, then Nat | `[anim+0x210]` / `+0x214` (area and point counts) for the same 20 items | 1..8 inside a region, equal natively |
| FF8 | NOW | Orig | two items of one region at m31 whose nodes were created at different times, then two created together: are the first area records equal once the fade ramp has passed? | equal for the second pair; the first pair may differ by the fade state; decides whether a cache is exact |
| FF9 | NOW | Orig, then Nat | WindSFX effect creations per 600 frames at m31 (`E6DCD0` id 800081 callers) and the entry lifetimes | the same cadence; tells who re-creates a finite-fade effect |
| FF10 | NOW | Orig | at a frame with shafts (m31, m50) and one without (m37, m40): the keys A/B of index 9 (`context+0x4020/+0x4088`), the weight `+0x40CC`, the record words 0..27 and `1449620A0..F4` | the globals equal the golden for the record in use; explains `Enable` |
| FF11 | NOW | Orig | `post+0x21E0..0x221F` at the same frames | equals `w4..w7` twice and `w24..w27` twice |
| FF12 | NOW | Orig | `[[GXRS+0x5368]+0x50]` and `[GXRS+0x5360]` at m31 | factor 1.0 expected |
| FF13 | NOW | Orig | library instance count against SfxMan handle count (FX13) | instances greater than handles when parents spawn children |
| FF14 | NOW | Orig | at m31 and m40: the wide strings at `post+0x2100/0x2130/0x2160`, `1449622C8/D0`, the argument block of `FUN_1400E5110` (`+0x358/+0x360` and a copy of the blend output at `+0x128`), the two objects' size queries and format, and the texture bound to `LutCopy2DTo3DCs` t0 | gives the prefix, the file and the 16×256 size; closes §4.2 |
| FF15 | NOW | Orig | `144962228..238` at m31, m40, m50, m37 | equal to the grading table of §4.2 |
| FF16 | NOW | Orig, then Nat | the order of `E6E750` against the light collect (FX11) and `native_point_lights` over 600 frames at m40 | update first; 163 |

## 9. Risks and open points

1. **Registration may not run in phase 1.** The force-field appearance is built by the type-table factory during the library update (CPU), but PHASE1 §5 step 7 skips the two renderer work lists. If the build or the registration were queued in those lists instead, no wind entry would exist: FF5 (calls of `140259F60` at m31) decides in one read. Inferred as running (the registry accessors are CPU and the appearance update `1402767C0` reads and refreshes the entry).
2. **The registry must be enabled.** `registry+0x110` gates every accessor; FF1 reads it from the first row.
3. **A finite effect needs a re-creator, and the `WindArea` regions are unexplained.** The WindSFX effects fade out (90 to 240); the region owner's cadence is not decoded (FF9). If native shows wind only briefly, the owner is a world-block SFX holder (`WorldBlockSfx`, ctor `68C7E0`) that is part of F1's door, not a missing piece of N4. The MSB `WindArea` regions (3, 6, 6, 3 at `m30_00`, `m40_00`, `m50_00`, `m51_00`, equal to the WindSFX counts there, none at `m31_00`, `m37_00`, `m45_00`) may gate the spawn (a volume the player must be in) or be a second wind source; neither is decoded, and FF9 should read the creations with the player inside and outside such a volume at `m40_00`.
4. **Position and fade state.** The wind evaluation does not read the node box; it keeps a persistent per-node fade state (§1.2). A native cache keyed by region would not be exact for nodes of different age; none is proposed (FF8).
5. **The `E6E750` thread.** Slot 8 runs in the graphics step; the item and entity updates (`D3E950`, `D07710`) run in the world update tasks (R8B open point 1). The node vector is written by one and read by the other; the original has the same split with a job in between, so the hazard is not new, but FW-1's rows record the order and FF7 compares counts.
6. **The token page.** A fault at the first transcription miss is the intended failure; it must be a reported fault, not a silent `Refuse`. If the existing crash reporter cannot name the address, FW-2 adds a vectored handler that logs it and re-raises.
7. **Light-shaft and grading keys.** The zone files carry only id 0 for both groups and the observed behaviour disagrees with id 0 at m31 and m37..m51 (F5); the builders take the key like the other banks, so a wrong key shows as wrong globals in FF10/FF15, never as a code fault.
8. **The LUT** (FF14) is the one link of the grading chain that static reading did not close; FW-8 ships with the trigger forced to 0.
9. **WindSFX billboards.** Six FXR ids (`800080..82`, `850800..02`) carry an invisible billboard (alpha 0). A native E01 path that culls alpha 0 would count differently from the original: FX1 baselines must say which.
10. **Eclipse nodes.** An item whose model has the Eclipse bit gets mask B `0x2_0000_0000`, not 4 (WIND-BLOCK §4); no entry in the five zones matches it.
11. **Callees checked by the review.** `FUN_140245F90` calls `140247E80`, `1402451A0`, `140248070`, `1417B3350`, `1417B3360`, `1417B3820` and about twenty virtual slots of the entry object (vtable `143D64C68`, targets `140258400..140259500`, the box tests `1402A1F50` and `1402A1E60`, `14025AF40`, `1417B3370`, `14006BFD0`): the reviewer read them by linear listing (they are not defined functions in the database) and found no access to `[144961AB0]`; the token page (FW-2) and the 600-frame guard (FW-3 exit) still stand.
12. **Not verified by the review.** The seal addresses `[144933088]+0x20` and `[1449651E8]` (checked by T-F1 at run time), the colour properties of the billboards other than colour 1 (only colour 1 is alpha 0), and every live row.

## 10. Corrections to earlier notes (applied with this sheet)

| Note | Statement | Now |
|---|---|---|
| EFFECTS-PHASE1 §5 step 5 | "`FUN_14025B870([1449623A0], dt)`" listed with the CALLs | CALL while no node exists; **TRANSCRIBE once a node exists** (job dispatch through the GX pool), §2.3 |
| WIND-BLOCK sheet §3.2, open point 1 | who fills the node's effect-handle list was not traced | closed: `FUN_14025C700` through the world bucket filled by the force-field appearances (§1.2) |
| EFFECTS design §2.6 and N14 | a per-frame wind table used as a cache if the evaluation is position independent (open) | the evaluation does not read the node box but keeps a persistent per-node fade state, so an exact per-region cache is not available (inferred); N14's "wind-table cache" is dropped |
| YEBIS-WAVE1-READY §1.4 | `E4700` and `E4890` both run on the real post object | `E4700` does not read `post`; it can run on the scratch object; `E4890` cannot |
| YEBIS-WAVE1-READY §0.7/§5 (YB-4c) | "m31, m40 and m50 enable grading" | observed grading draw at m31 and m40; group `Enable` 1 at m30, m31, m40, m45, m50, m51; Setting Info flag at m40 and m50; §4.2 |
| WAVE1-PREP §2.1 | flag `no-native-effect-system.flag` (opt-out); N1 as one step | opt-in `ffx-system.flag` and `ffx-publish.flag` (the review rule of the YEBIS sheet); N1 splits into FW-2 (build) and FW-3 (publish with slot 8 and the inline node update) |

## 11. Review log

Independent review by an Opus 5.5 reader (read-only; given the note and the evidence, not conclusions; it re-derived the node-manager job and the GX-access search over the whole image, the collector and registration chain, the FXR contents and region counts, the light-shaft layout and the goldens of two zones by hand, the grading block and the sites in `FB890`, and the native anchors). **Four important findings, six minor ones and two corrections to §10, all verified (the offsets of finding 2 and the region counts of finding 4 re-checked in the assembly and the TSV by the author) and applied.** The reader re-derived as exact: the GX dependency of `B870` and its job body (slot 1 of vtable `143D65508`, range and block size), the absence of GX accesses on the whole wind chain apart from `B870` (searched over the image for `AB0`, `BB0`, `B50`, `B70`, `B78`, `B80`, `BA2`), the collector's two passes and strict `< p` rule, the node creation arguments, the registration lock and call order, the FXR contents (one `WindForce` and one alpha-0 billboard; `31001` without force field; `845010` without billboard), the type ids against `24B260`'s list, the constructor defaults of words 9 and 10, the blend and the 23 converter globals, both hand-recomputed goldens (m31, m50) bit for bit, the grading block layout and loader, both `FB890` sites, the `%s%04d` format, the size test 16/256, and the zone values.

| # | Severity | Finding | Applied |
|---|---|---|---|
| 1 | important | `FUN_140245F90` never reads the node box; it uses the entry object's virtuals, a time-only noise, a point drawn in the wind's bounds, and a **persistent per-node result** (zeroed at creation only); "depends on the item position" was unjustified | §0.4, §1.2 Evaluation row (node read only at lines 212-217 and 547-549), §9.4, §10 row 3, FF8 rewritten (fade state, not position) |
| 2 | important | `FUN_1400E5110` reads the textures at `+0x358/+0x360` (assembly), not `+0x6B0/+0x6C0` (decompiler `int*` artefact); fallback also when `post+0x2191 != 0`; orientation 16 by 256, not 256×16; the LUT was declared live-only before searching these offsets | §4.2, §0.7, FF14 (the copy of the output at `+0x128` and the producer of the argument remain unsearched, said so) |
| 3 | important | the transcription dropped the pointer copy of step (3) and iterated the live list while `B610`/`B750` (lock `mgr+0x58`) change it | §2.3: copy under the lock, iterate the vector |
| 4 | important | `m31_02_00_00` carries `WindSFX 800081` ×9 (not `31001`); "`850800..02` equal `800080..82` field for field" was false (swapped `unk_ds3_f1_42/43`, file size) | §3 F4 table, §1.2 note |
| 5 | important | origin of the masks not stated: `FUN_14029A5C0` reads the root action `0x2774`; mask B 7, mask A 2, priority 0 for `800081`; the byte `+0x34` is copied from `desc+0x24` (verified), its "cancel" meaning stays inferred; the descriptor's 1..7 is a genre (2 wind, 7 cancel), the shape is at `desc+0x3C`; T-F3 must use those values | §1.2 Entry object and Appearance rows, T-F3 |
| 6 | minor | a third node creator, `FUN_14066F7A0` (`140669D90`, masks `14456F718/720/728`), so nodes can exist without W1 and FF2's expectation was too narrow; if none exists FW-3's exit never runs the inline loop live | §1.2 Node row, §2.3, §2.6, FF2 |
| 7 | minor | `81a22ba9` is the branch merge commit; `main` had moved to `933a9431` (+1087/−308 lines in `game_bootstrap.cpp`); every `GB:` anchor shifted | merged `main`, all anchors re-read at `933a9431` (§2.1, §2.5, §6) |
| 8 | minor | grading blend table: the middle branch replaces `out[2]`/`out[3]` by `A[3]`/`B[3]`, `out+0x230` takes A's `+0x228`; four 8-byte fields; only the m40 and m50 files have a Setting Info group; the "20-line port" is risky | §4.2 table and FW-8 (call the genuine function), F6 |
| 9 | minor | `E4700` reads the scene root `[144961B50]` (factor): T-F5's `f` = 0, 0.5 needs a scratch root; "DistanceZ 2000" named word 17 (`MaskDepthThereshold`) | T-F5, §4.1 wording |
| 10 | minor | T-F1 said nine types, §2.2 lists seven; `14024D365` is a return address (call at `14024D360`); `+0x110`, `+0x118`… of the Evaluation row are virtual slots; risk 11 omitted about twenty virtual slots of the entry object (read by the reviewer by linear listing, no GX access) | T-F1, §1.1, §1.2, §9 point 11 |
| 11 | correction | §10 row 3 repeated the "position dependent" statement; the registration happens when the appearance is built (factory `140275DF0`, action `0x283C`) | §10, §1.2 Appearance row, §9 point 1 |
| 12 | note | not verified by the reviewer: the seal addresses `[144933088]+0x20` and `[1449651E8]`, the colour properties of the billboards other than colour 1, all live rows | stated in §9 |

## Exports and evidence

Decompiled by Ghidra for this sheet (query outputs, not exported to `decompiled/`): `14025C4C0`, `14025BCA0`, `140269DB0`, `1402783C0`, `14239C180`, `1402691D0`, `1402777F0`, `1402A5D60`, `1402AF510`, `14239B8B0`, `1423A32B0`, `14024F9A0`, `1402AA130`, `140255210`, `1402552A0`, `1402459A0`, `140245920`, `14025C700`, `14025C650`, `14025B1F0`, `14025B4F0`, `14025B750`, `14025B870`, `14025B020`, `14025CBF0`, `14029B4F0`, `14029B890`, `1402553E0`, `14029BFD0`, `1402594E0`, `140248E30`, `140245000`, `140244EF0`, `140275470`, `1402757A0`, `140276300`, `1402767C0`, `14027C370`, `14027C7C0`, `1402552C0`, `140259F60`, `14021C150`, `14017BEE0`, `1400ECA10`, `14000E610`, `140033D90`, `14016B030`, `1400FCD40`, `14021C510`, `14021C9E0`, `14021CC20`, `14021C780`, `1400F8CE0`, `14017BE20`, `1400499C0`, `14017AAE0`, `14017B270`, `14017BA50`, `14021D8D0`, `14021D9E0`, `14021E2D0`, `140169D20`, `1400E5110`, `1400E4CE0`, `1400E8100`. Already exported: `00245F90`, `0025B610`, `00245870`, `002458B0`, `000FB890`, `0024D000`. Scripts: `Get-FfxBankBlocks.ps1`, `Get-ZoneForceFields.ps1`, `Invoke-Map.ps1` (queries `M`, `U`, `A`, `b`, `r`, `o`, `x`, `S`, `s`), `I0068Find.java`, `Read-Fxr.ps1`, `FxrDump.cs`. Game data read: `frpg_sfxbnd_commoneffects_effect`, `m30..m51_effect` bundles (recovered into a scratch directory outside the repository, read-only on the game).

## Re-anchor at main `ea0592ae` and a note for FW-1 (pilot, 2026-10-12)

Functions of section 2.1 in `native/frame/game_bootstrap.cpp` at `ea0592ae` (cite the function, the line moves): `native_sfx_manager` :477, `CollectNativePointLights` :3345 (called from the main collect :6226), `UpdateNativeSceneCpu` :7398 (hook table :12026), `RetireNativeZoneCpu` :10052 (hook :12150), `DeferNativeSfxSceneRequest` :10109 (hook entry `{0x7CB820, ...}` :12133, in the `guards` table of the native bootstrap), `NativeSfxStartup` :11534 (hook :12059), `PublishNativeFxWorld` :11622, `PublishNativeSfxManager` :11663, `NativeSfxViewAttach`/`Detach` :11705/:11712 (hooks :12061/:12062). Since that tree: the frame graph drives every barrier (D-0171; a new pass declares its uses in `GraphShadow` and is shadowed before the executor drives it), the option snapshot O2 (`NativeSelectedScene::options`), the gamma pass W3 and the present pacer cR4 (renderer side only).

**FW-1, a signature-free observer (proposal).** Of the twelve observed entries only `14025B870` has a decompiled prototype in the exports (`ffx-ready/f/14025b870.c`: `(longlong, undefined4, undefined8, undefined8)`); the others (`E6DCD0`, `E6DA30`, `E6DBC0`, `7CB8F0`, `7CB750`, `E6E750`, `D3E950`, `D07710`, `245F90`, `259F60`, `2552C0`) have only their assembly in `decompiled/` or none, and a C++ pass-through with a guessed prototype could clobber an XMM argument before forwarding (a float parameter in the second to fourth slot travels in XMM1-3). A counting stub needs no prototype: an executable thunk per hook that does `lock inc qword [counter]`, `mov rax,[rsp]`, `mov [last_return],rax`, then `jmp [original]` (RAX carries no argument in the x64 calling convention at entry; flags are free there), installed with `MH_CreateHook` like the other entries, gives the per-frame counts and a return-address sample for every entry; the identifier and flags histograms of the creators then come from typed wrappers only where a prototype is read (Ghidra, `Invoke-Ghidra.ps1`), as FW-1 asks.

## Implementation log (thread « Vague 1 effets W4 », 2026-10-12)

Branch `claude/nostalgic-pike-850e64` from main `4db93122`. Every rung is opt-in by a flag file next to the module (runner
`-EffectFlags "ffx-observe;ffx-system;ffx-publish;ffx-door"`); without a flag the native and reference paths are unchanged.

**FW-1 (`ffx-observe.flag`).** `native/frame/ffx_observe.*`: one signature-free x64 thunk per entry (saves RCX, RDX, R8, R9 and
XMM0-XMM5, calls the recorder, restores them, jumps to the MinHook trampoline; unwind data registered), so no prototype is
guessed; 12 thunks natively (7CB820 and D07710, already hooked, report by `Note`), 14 in reference; the native light collect
reports too. Row `native_ffx_door`: frames 1-3 then every 600th graphics step (D5B140), per entry calls, callers by return
address, creator identifiers and flags, the creators' `+0x58` branch, the live node count, the update/collect order. T-F4
(`ffx_observe_test`: histograms, rows, float/integer/stack arguments through the thunk, unwind lookup) passes.
Witnesses Reference/Native1478 (High Wall) and 1479 (Firelink): exit 0. Observed: in the original the zone's effects are
created once at load (212 creator calls at High Wall: `E6BFC1` single SfxIns, `E6A497` multi, `7CB98A` via 7CB8F0, `7CB8B2`
via 7CB820) and the effect update runs once per graphics step with the system (`+0x58`) set; 119 live wind nodes at High Wall
(85 to 134 at Firelink), 13 wind evaluations per frame at High Wall (66 at Firelink), 2 force-field registrations at High Wall
(9 at Firelink, from `276728` = the wind appearance) and 6 bucket appends from `CE8634` (not the wind appearance: an open
second registration path, §9 point 3). Natively, before FW-2, the same creators are called **every frame** and fail (38 per frame
at High Wall, 68 at Firelink; `7CB820` id 300 from `7CBF7E` every frame), no node, no wind evaluation; `E6E750` runs once per
graphics step in both modes; the light collect follows the effect update in the same frame (600 of 600), so FW-5's order is
already the original's. FW-1's exit expectation "every creator at 0 natively" is refuted: the engine's sources run natively
and retry while the system is absent.

**FW-2 (`ffx-system.flag`).** `native/frame/effect_system.*`: E6C9D0's remaining CPU construction (+0x38..+0x50), 24B260
transcribed (its five constant buffers created on the device token; 1A97EE0 and the FX world already native), the 14 types and
two properties registered, the registries sealed, 24C530 transcribed (no GPU ring, one null context slot, the manager with the
token, the scene entity), `+0x58` left null. Row `native_effect_system_built` (T-F1 at run time: pass ids, registration, seal,
the seven wave-1 types present). Native1480: the token named a hidden GPU creation: `2155700` (library init) builds a built-in
mesh through the resource manager's slot 1 `36FD70` → `36F9E0` → `19254F0` on the device → GX buffers on the token became
native records (`effect_system::CreateBuffer`, hook of `19254F0`). Native1481: built and checked, then the FXR loader ran the
types' GX side (`269020` → `26A120`, GPU particles), which reads the GX display object `[144961AB0]` itself → **D-0173**: the 252
reads of the display in the effect GX module (closure over direct calls from the effect classes and the type descriptors,
`I0066FfxDisplaySites.java`, `ffx_display_sites.h`, checked by the build and in memory) read a native effect display (device =
token, release queue +0x2B8, main context +0x60, zero elsewhere), views `1925BF0/1925B10` and releases `D8780/19AD90` native
for effect objects. Native1482: release at quit named `26E880` → `D8780` (fixed). Native1483: exit 0, no fault, 252 sites.

**FW-3 (`ffx-publish.flag`).** Slot 8 `24D000` hooked and transcribed (`effect_system::Update`: environment factors, resource
tick, hide mask, sound, time scale, registry controllers, the node manager inline on the calling thread with the node copy under
the manager's lock, the update/proceed passes `296E20/296690` called with the effect display's context, `297110`, the library
update through the manager slot 3, the two renderer work lists counted and not executed, `266DE0`); `+0x58` published after the
build; view attach/detach store/clear `sc+0x218`. Native1484: published, 102 nodes refilled per frame, but every creator still
retried: the FXR files were parsed natively but never registered in the system. With the system published, E0DD10's own
statement now runs (`E6E570` registers the FXR, `E6E600` releases it at retirement). Native1485 (High Wall): exit 0, no fault;
more than 2,000 FXR registered; the zone's effects are created once at load (198 creator calls, then none, as in the original);
renderer update items about 420 per frame (counted); **effect point lights appear natively: 341 lights in the collect against
299 without the system (+42)**. Wind: no evaluation yet (no WindSFX entry registered at High Wall natively; the original registers
2 there): next.

**FW-4 (`ffx-door.flag`, opt-in).** The `7CB820` hook forwards to the original once published. Native1486 (High Wall, door
open): a fault in game at `26E46F` (type 10000, GPU particles, instance creation `268E70` → `26E46F`), a read at null +0x25F0:
`26E44D` loads `[resource manager+0x168]`, the shader set that 24C300 (`FUN_140291520`, GPU programmes) creates and FW-2 skips,
then indexes its programme table. Next: build the shader set with native programme records (the same mechanism as the buffers:
its CPU tables run, its GX programme creations become native records), then FW-4's exits; FF5 (why no wind entry at High Wall natively), FW-5 rows at Firelink
against Reference (R7), FW-6.

**Pre-merge review (Opus 5.5, fresh agent, 2026-10-12).** No blocker; with no flag file the native and reference paths are
unchanged (every unconditional hook forwards with its full argument list, checked against the decompiles). Applied: the node
callback takes dt as a float in XMM0 (as `25B1F0`); released effect-object slots are reused after 1,024 others (the handle range
held 65,535 creations per process); the published and door states are atomics stored after `+0x58` (release) and read with
acquire on the loader and retirement threads; the display redirect makes every code page writable before writing any site
(all or nothing); view descriptions copied at their size (24 / 20 bytes); the engine context kept by value; the observer counts
the transcribed node update (FF3); comments (the effect display's zero page, `+0x60`, `sc+0x210` left 0). Recorded, not changed:
the thunk epilogue is not a standard unwind epilogue (no fault possible there), the effect GX objects keep the engine's
descriptions as data and must not become a call-translation layer when they get D3D12 resources, directly called engine
functions are not all pinned (the executable hash is), `ffx-wind` has no consumer yet (FW-6). The fixes build and pass the
controls; they were not run live (Thomas's pause).

**Resume after the pause (2026-10-12 23:30 to 2026-10-13).**
- **FW-4.** The shader set is built natively: 24C300's `FUN_140291520` runs on the device token and its programme creations
  (`FUN_141925210`, stream-output variant `FUN_1419250A0`) become native programme records keeping the bytecode (38 at High
  Wall). Native1487 (High Wall, door open): exit 0, no fault; **the creations equal the original's** (212 creator calls and 14
  `7CB820` requests at load, then none; the 2 wind registrations of the original appear natively); 344 scene lights.
- **FW-5.** A `light_vector` field in `native_ffx_door` reads `[[144961B50]+0x41D0]` the same way in both modes. Reference/
  Native1489 (Cemetery of Ash, then Firelink after the teleport): 48 and 48, then 152-154 and 152 (R7 met at this witness;
  the 163 of EFFECT-LIGHTS was another view). The light collect already follows the effect update in the same frame (FW-1), so
  no ordering fix. The screenshots show the candles and the bonfire lighting the shrine natively again (Native1479 without
  effects was dark).
- **FW-6.** W1: bit 1 (`g_GXFT_WindEffect`) from every MTD that enables it, only once the effect system is published.
  Native1490 (High Wall): item wind updates 19 per frame, evaluations 13 per frame, 119 nodes, 2 entries: **all equal to
  Reference1478**. WD-1/WD-2 hold by these counts; FF7/FF8 (block contents) and the sway on screen are not yet compared.
- **FW-7, FW-8** need the YEBIS bank accessors (W5f `Banks`, YB-1), which do not exist yet: they move with W5f/W10.
- **FW-9.** The system, its publish, the door and the wind policy are the default; `no-native-effects.flag` (runner
  `-EffectFlags no-native-effects`) turns W4 off for A/B, so the `7CB820` deferral stays as that path's answer (not deleted).
- Zones `m31`, `m37`, `m45`, `m50`, `m51` are not reachable from Thomas's saves (High Wall, Cemetery of Ash/Firelink): their
  rows wait for a save there.
- **Non-regression with the defaults** (`Run-PrepareGate -First 1491 -Only gate,firelink,charmake -TagSuffix -effects`):
  Native1491 (High Wall, debug layer, menus, Equipment page, collect and prepare shadows) and Native1492 (Firelink, teleport,
  phantom, cloth) exit 0 with 0 debug errors and 0 shadow mismatches; Native1493 (New Game, Bink, character creation) 0 debug
  errors, its close faults at `D12759`, the known character-creation close fault (Native1250, Reference1250, Native1317).
- **Second pre-merge review (Opus 5.5, fresh agent).** No blocker; the new hooks' signatures, the shader-set call and every
  `no-native-effects.flag` path checked against the decompiles and the pre-W4 behaviour. Applied: a null shader set refuses the
  build and the run-time check tests sc+0xE0 and its copy at the resource manager +0x168; the flag is read once in Setup
  (`native_effects_enabled`), the door and the wind policy follow the publish (the redundant door state removed); the zone and
  scene-reset rows report the real effect-system state; comments and the record kinds updated; the observer's light read
  skipped without an image (its test). Native1494 (High Wall, defaults, after the fixes): exit 0, no fault, wind 13
  evaluations a frame on 119 nodes, no door retries, 344 scene lights. Budget 1494.
