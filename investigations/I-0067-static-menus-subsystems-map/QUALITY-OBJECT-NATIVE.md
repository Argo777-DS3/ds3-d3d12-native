# Native construction of the GX quality object `[GXRS+0x5360]` from the graphics configuration (STATIC; INFERRED where marked)

Written 2026-10-11. Builds on [MENUS-MAP](MENUS-MAP.md) section 77 (keys, tokens, the twelve slots, the decoded 0x24C-byte table,
every reader). This note says **how the native backend builds the object**, in what order, with which expected bytes. Nothing was run.

## 0. Position

- The original builds the object in `FUN_1400220A0` (GX rendering-system init) and rewrites it in `FUN_140023790` whenever one of the
  twelve level integers `GXRS+0x5320..0x534C` differs from its applied copy `GXRS+0x52F0..0x531C`. The levels come from the config
  block `SprjDrawStep+0xA0` by `D55190 -> DFDD20 -> CED9A0 -> 23660`.
- The native root constructor (`native/frame/game_bootstrap.cpp`, the comment before `allocate`) builds `GXRS+0x5368` only. `+0x5358`
  (table) and `+0x5360` (object) stay null: **no graphics option reaches the native renderer** except through the pieces below.
- What native already does with the same inputs:
  - `NativeRenderStep` (hook of `D55190`) reads the settings owner `[image+0x4796298]` every frame and passes `settings+0xA8` (slot 0,
    Texture) to `native_sampler_bank.PublishExternalSelector` (`RefreshNativeSamplerSettings`). So **the twelve levels are live in native
    mode at `settings+0xA8 + 4*slot`** (slot 8 at `+0xC8` is unused by the original copy, section 77). `settings+0x94/+0x98` carry the brightness
    (`native_colour_settings`).
  - `CollectNativePointLights` reads `[root+0x5360]+0x40` when the object exists (else the fade scale is 1).
  - The host-qualified pass-constant transports read it: `CBFxaaParam` (`1B9790`, region 5 = the object, `+8` the enabling byte, `light_producers.cpp`)
    and `1B5720` (shadow record, region of 0x80 bytes of the object, `world_adapter.cpp`). With a null object they see the "absent" case.

## 1. Source of the levels (decision)

- **Take the levels from the live config block, not from the XML.** `levels[k] = u32 at [image+0x4796298] + 0xA8 + 4*k`, `k` in 0..11, with
  `levels[8] = GXRS applied copy` (the original passes `GXRS+0x5310` for slot 8; natively keep the constant 3). Reasons: the engine's own CPU
  loader and the Options page write this block (INFERRED: only the read side D55190 -> CED9A0 was read); the file tokens, the preset logic and the Antialiasing conversion (below) are all upstream of it
  and need not be re-implemented; a change made in the Options page reaches the native object the same frame.
- Read the twelve values twice and accept them only when both reads agree (the pattern of `RefreshNativeSamplerSettings`); out-of-range values
  (`> 4`) are refused for that frame and the previous object is kept. The original does not check (table read beyond the slot's rows, or one past
  the end for slot 11), so this is a deliberate safety deviation.
- **Open conversions, settled by one live read (row QL1):**
  - `Antialiasing`: the file token is `ON` = 1 / `OFF` = 0 (the reader stores 0 or 1 in `config[8]`), yet the table row for slot 1 is `0,0,0,1,1`
    over levels `DISABLE..MAX`. A level of 1 would give a byte of 0 (anti-aliasing off), so the block must carry a level of 3 or 4 for `ON` (INFERRED:
    the encrypted copy from the config maps `ON` to a high level). Read the block level for Thomas's `Antialiasing ON`.
  - `QualitySetting` presets (`LOW`, `MEDIUM`, `HIGH`, `MAX`, `CUSTOM`): whether the block holds the per-key values of the file (`CUSTOM`) or values
    recomputed from the preset. Compare the block with the file for a non-`CUSTOM` preset.

## 2. Storage and lifetime

1. In the native root constructor, next to the `+0x5368` allocation (`allocate(0x1F8, 8)` lambda over the engine allocator `FUN_141777A00(size, align, heap)`):
   - `table = allocate(0x24C, 4)`; **call `FUN_1400219B0(table)` in place**: it is a pure CPU builder (immediates, three constant loads, one `memcpy`, no
     device call; `Get-QualityTable.ps1` evaluates it without running it). Its bytes equal `quality-table.tsv`.
   - `object = allocate(0x78, 4)`, zero-filled; store `root+0x5358 = table`, `root+0x5360 = object`.
   - initial levels `root+0x5320 = 2` and `root+0x5324..0x534C = 3` (twelve ints, as `FUN_1400220A0`), applied copy `root+0x52F0..0x531C` zero so the first update
     rewrites the object; `root+0x5350 = 0` (the lock byte tested by `FUN_140023660`). The original init also clamps the Effects level to at least 1 on its first fill (`000220A0` lines 168..171) and `FUN_140023790` does not: with the initial level 3 this is invisible, but a sweep of slot 7 at 0 (Q3) must expect row 0 from the update path. The original publishes the pointer before filling (`220A0` order); filling first is a declared deviation. Bytes `+0x69..0x6B` are never written by the original (padding); the zero fill is harmless.
2. The object and the table live as long as the root (the original has no free path before the GX destructor); no native owner other than the root.
3. Publish order: fill the object completely (section 3) **before** storing `root+0x5360` (readers test the pointer, then read fields).
4. Update thread: `NativeRenderStep` (the same context that already reads the block). Writes are in place field by field, as in the original; readers on
   other threads tolerate a torn frame exactly as they do in the original.

## 3. The update (transcribe `FUN_140023790`, lines 108..200 of its export; no other part of that function)

Guard: the whole rewrite runs only when `[144961AB0]+0x3E1 == 0` (original); natively treat as 0. For `s[0..11]` the validated levels:

| Object bytes | Source in the table |
|---|---|
| `+0x00..0x07` | `table[s0*8 .. +8]` (two ints) |
| `+0x08` | `table[0x28 + s1]` |
| `+0x09..0x0C`, `+0x0D..0x0E`, `+0x0F` | `table[s2*7 + 0x2D]` (4), `+0x31` (2), `+0x33` (1) |
| `+0x10..0x1F` | `table[(s3+5)*16 .. +16]` |
| `+0x20..0x27` | `table[0xA0 + s4*8 .. +8]` |
| `+0x28..0x3F` | `table[0xC8 + s5*0x18 .. +0x18]` |
| `+0x40..0x47` | `table[0x140 + s6*8 .. +8]` |
| `+0x48..0x67` | `table[0x168 + s7*0x20 .. +0x20]` (flag dword then seven floats) |
| `+0x68` | `table[0x208 + s8]` |
| `+0x6C..0x73` | `table[0x210 + s9*8 .. +8]` |
| `+0x74` | `table[0x238 + s10]` |
| `+0x75..0x76`, `+0x77` | `table[(s11+0xBF)*3 .. +2]`, `+2` |

`+0x69..0x6B` stay zero. After the writes copy `current -> applied` (twelve ints). Replace the calls inside the original block as follows:
- `FUN_1400D3650(textureManager, obj[0], obj[1])`: native sampler parameters. `obj[0]` (1, 2, 3, 3, 3) selects the filter mode (`{0, 6, 7, 8}[a]` in the original,
  stored at `texmgr+0x5E8`), `obj[1]` is the anisotropy limit (1, 1, 4, 8, 16, INFERRED). The native sampler bank is already driven by the Texture level through the
  selector (`PublishExternalSelector`); the pair is what the native sampler creation reads when it builds anisotropic filters.
- The listener loop over `[root+0x4E08]` (objects called through their first vtable slot with the quality object; `FUN_1400887C0` unregisters one, called by the destructors `FUN_140020440`, `140085220`, `140022A80`, `140023100`, `1400FDE80`, `1400FE020`; the registrants are INFERRED to be the same GX scene and pass objects): **none exists natively**; consumers read the object.

Reference oracle for the unit test: `Build-QualityObject.ps1 -Table quality-table.tsv -Levels ...` prints the 0x78 bytes. Examples (also the expected bytes for the live row QL2):

- Initial levels (2, then 3 x 11), from the script: `+0x00` `03 00 00 00 04 00 00 00 01 01 01 01 01 00 00 00`, `+0x10` `01 00 00 00 03 00 00 00 05 00 00 00 05 00 00 00`,
  `+0x20` `01 01 00 00 05 00 00 00 01 00 00 00 01 00 00 00`, `+0x30` `01 00 00 00 80 00 00 00 00 10 00 00 00 00 00 00`, `+0x40` `00 00 80 3f 01 01 00 00 01 01 01 00 00 00 80 3f`,
  `+0x50` four more 1.0 floats then `+0x60` `00 00 80 3f 00 00 80 3f 01 00 00 00 01 01 00 00`, `+0x70` `02 00 00 00 01 01 01 00`.
- Thomas's `GraphicsConfig.xml` (Texture MAX, SSAO HIGH, DepthOfField MAX, MotionBlur DISABLE, Shadow/Lighting/Effects/Reflection/Shade MAX, Water HIGH, Antialiasing `ON`
  assumed level 4): `+0x00` `03 00 00 00 10 00 00 00 01 01 01 01 01 00 00 00`, `+0x10` `01 00 00 00 05 00 00 00 07 00 00 00 07 00 00 00`, `+0x20` `00 00 00 00 00 00 00 00 01 00 00 00 02 00 00 00`,
  `+0x30` `01 00 00 00 80 00 00 00 00 20 00 00 02 00 00 00`, `+0x40` `9a 99 99 3f 01 01 00 00 01 01 01 01 00 00 80 3f`, `+0x50..0x60` seven 1.0 floats,
  `+0x68` `01 00 00 00 | 01 01 01 00` (`+0x6C` flags), `+0x70` `02 00 00 00 01 01 01 01`.

## 4. Effect of publishing the object on code that already runs natively

Publishing turns "absent" into real values in these places; each must be re-checked (no other native code reads the object today):

| Reader | Absent now | With the object | Check |
|---|---|---|---|
| `CollectNativePointLights` (`game_bootstrap.cpp` ~2440) | fade scale 1 | `+0x40` = 1.2 at `MAX`, 0.5 at `DISABLE` | point-light fade at the High Wall against the original |
| `CBFxaaParam` producer (`1B9790`, transport 31) | "configuration absent": fade 1 | enabling byte `+8` = 1 (AA) selects the fade from `[root+0x5368]+0x98` | only matters when the native frame runs the FXAA pass (it does not yet): none |
| `1B5720` shadow-record transport (region of `[G]+0x5360`) | zeros | `+0x28 = 1`, map size `+0x38 = 8192` at `MAX`, `+0x2C/+0x30/+0x3C` from the table | shadow work is not native yet (SHADOWS-NATIVE); the producer starts receiving real numbers |
| Native sampler | selector only | unchanged | none |

## 5. Ordered checklist

| # | Step | Expected value / rule | Check |
|---|---|---|---|
| Q1 | Allocate table and object in the root constructor; call `FUN_1400219B0` | `memcmp(table, quality-table.tsv bytes) == 0` | unit test over the committed TSV (no game needed) |
| Q2 | Write the initial levels and the lock byte | `root+0x5320 = 2`, the other eleven `= 3` | native read after boot |
| Q3 | `UpdateQuality()` = section 3 transcription, with the Build-QualityObject oracle | the three example images above | unit test: the two examples plus each slot swept 0..4 against the script |
| Q4 | Read the live levels each frame from the settings owner, double read, range check; skip unchanged | refuse `> 4`; update only when a level changed | change a graphics option in the Options page (once reachable) or edit the file between runs |
| Q5 | Publish `root+0x5358/0x5360` only after the first successful fill | readers never see a partial object | boot log line `native_quality_object published` with the 12 levels |
| Q6 | Map the texture pair to native sampler creation | `(a, b)` = (3, 16) at `MAX`: anisotropic with 16 | native sampler descriptor dump |
| Q7 | Re-baseline the three readers of section 4 | see table | image comparison at the High Wall; point-light count unchanged |
| Q8 | Do not wire consumers per feature in this step | each feature family reads its fields when its native pass lands (shadow `+0x28..0x3F`, SSAO `+9..+0xF`, velocity `+0x20/+0x21/+0x24`, reflection `+0x6C..0x73`, DoF `+0x10..0x1C`, effects `+0x48..0x67`, SSS `+0x75`, decal `+0x68`, water `+0x74`) | the readers' list in MENUS-MAP 77 |

## 6. Live rows

| Row | What | Expected |
|---|---|---|
| QL1 | In the original with Thomas's file, read `[image+0x4796298]+0xA8 .. +0xD4` (twelve ints) and `[[144961B50]+0x5360]` (0x78 bytes) | levels `4, ?, 3, 4, 0, 4, 4, 4, 3, 4, 3, 4` with `?` = 3 or 4 (Antialiasing ON); object bytes as the Thomas example |
| QL2 | Same read in native mode after Q5 | equal to QL1 byte for byte |
| QL3 | Same read with `QualitySetting` set to `LOW` in the file (hash-checked backup and restore, as the velocity pilot does) | shows whether the block follows the preset or the per-key values |
| QL4 | With `MotionBlur` = `HIGH` | object `+0x20..0x27` = `01 01 00 00 05 00 00 00` (matches Reference1186) |

Exports used: `decompiled/DS3-1152-F-` + `000219B0`, `000220A0`, `00023660`, `00023790`, `000D3650`, `00CED9A0`, `00D55190`, `00DFDD20`, `000236E0`, `000887C0`.
Tools: `Get-QualityTable.ps1`, `Show-QualityTable.ps1`, `Build-QualityObject.ps1`, `quality-table.tsv`.
