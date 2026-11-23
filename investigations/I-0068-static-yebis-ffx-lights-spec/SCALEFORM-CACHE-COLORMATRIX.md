# Scaleform cacheAsBitmap and the colour matrix: original path, who uses them, native correction, witness pages (I-0069, 2026-10-11)

Request (coordinator, 2026-10-11, after the filters merge `1782f036`): the two open gaps of the Scaleform filters, F8 (cacheAsBitmap, the colour-select page) and F9 (colour matrix), are drawn without effect natively. Study them statically in the original (path, programmes, targets, pages), propose a native D3D12 correction that fits the current orchestration, with witnesses, and find a witness page for a simple drop shadow and one for the 7×7 blur, which the pilot has not seen on screen yet. Builds on [SCALEFORM-FILTER-PASSES](../I-0069-static-menu-audio-map/SCALEFORM-FILTER-PASSES.md) and [SCALEFORM-FILTERS-NATIVE](../I-0067-static-menus-subsystems-map/SCALEFORM-FILTERS-NATIVE.md). Nothing was launched, native code was not changed.

Labels: **STATIC** (decompilation, DXBC, the recovered `.gfx` files), **OBSERVED** (the retained traces and captures of Native1264-1270 / Reference1264-1270), **INFERRED**, **OPEN**. Evidence (outside Git): `evidence/I-0069/queries/scaleform-filters/context/` (`placements.tsv`, `strings.tsv`, `tree.txt` from `Scan-GfxFilterContext.ps1`) and `.../cache/` (HAL vtable reads). The 110 recovered movies (102 menu pages and 8 font movies, every `.gfx`/`.swf` of the eight archives) were read from a disposable scratch copy.

## 0. Verdict

1. **The colour matrix is unreachable in the shipped game.** No placement in the 102 menu movies carries a ColorMatrix filter (filter id 6), no ABC constant pool names `ColorMatrixFilter`, `ColorTransform`, `filters` or `cacheAsBitmap`, and the 8 other movies are font movies. The HAL even reserves kind 8 for the low feature level: `FUN_141F78E00` answers "unsupported" for every filter *except* kind 8 below feature level 2. F9 therefore has no page and no live witness; the correction is a small completeness slice with a synthetic test (section 5.3).
2. **cacheAsBitmap has two placements and one shape each**: `04_030_chrmake_colorselect` (sprite 57, depth 54, a sprite holding one shape) and its `win_` twin (sprite 62, depth 84). Both are reached by character creation. The original draws such a primitive through the **same temporary-target machinery as a filter with no pass**: the subtree goes into a target, then one textured quad (programme family `0x103`, VS 375, PS 1000/1001) composites it (section 1).
3. **The original keeps the result between frames.** The HAL's render-buffer object (`HAL+0x288`, slot `+0x18`, `MOV AL,1`) reports that caches persist, so after the first frame the primitive holds its target and later frames draw only the cached quad. Native today re-renders every filtered primitive each frame (the explicit non-persistent policy of SCALEFORM-FILTERS-NATIVE §6): equal image for static content, more work per frame.
4. **Text-field filters never reach the HAL.** 119 of the 137 filtered drop-shadow placements and all 24 of the 7×7 ones sit on text fields (`edittext`); the retained Native1268 trace shows the HUD's text shadows (the 4×4 soul counter, the 2×2 item name) on screen while no kind-1 plan was ever logged. They are drawn by the engine's CPU text meshes ("glyph/shadow quads", `FUN_141CFB810`), already native and equal to the original. Consequences: the pilot's missing kind 1 is explained, and **no shipped movie reaches the large separable blur branch (`0x200000`, 7×7) through the HAL**.
5. **Witness pages** (section 4): a simple drop shadow (kind 1, one pass, key `0x280000`) on a non-text placement is on the Options page (`02_040_optionsetting`, Windows twin `win_02_040_optionsetting`), the inventory pop-ups and the character-creation command list; the 7×7 branch has no page and gets a synthetic test against the shader maths instead.
6. **Correction proposal**: F8a, one more branch of the pass loop for an empty filter set (the cached quad drawn from the subtree target, non-persistent), then F8b persistence through the engine's own cache store (`FUN_141C64F00`), then F9 for completeness (section 5).

## 1. The cached primitive in the original (STATIC)

A primitive's field `+0x18` is its cache state (0 none, 1 filter results kept, 2 final bitmap kept); only `FUN_141C64F00(primitive, state, targets, count)` writes it.

| Step | Function | What it does |
|---|---|---|
| begin | `PushFilters` `FUN_141BDC4E0` | state 0: temporary target of the first child's bounds (integer rectangle), pushed with clear to transparent, blend normal, `HAL+0x58 |= 0x80`. State ≠ 0: **no new target**, `HAL+0x58 |= 0x100`, `HAL[0x227]` = stack depth, the queue context's marker `vt+0xC8()+0x30 = 1` (the children are not emitted); the push is CPU-only when `HAL+0x58 & 0x300` |
| subtree | the normal primitive path | draws into the temporary target (state 0 only) |
| end | `PopFilters` `FUN_141BDB800` → pass loop `FUN_141F7B930` | for a **set with passes**: the planned passes, then the final composite. For an **empty set** (`P == 0`, the cacheAsBitmap case): no pass |
| store | end of `FUN_141F7B930` (lines 232-243) | `render buffer->vt+0x18()` is true (see below), so `FUN_141C64F00(primitive, P==0 ? 2 : 1, {retained targets}, P==0 ? 1 : 2)`: **state 2 and the single subtree target for an empty set**, state 1 and the (blurred, source) pair otherwise |
| draw | `FUN_141F7A590` through `vt+0x2E8` | state 2 (and, for an empty set, right after the store): the cached quad (below). State 1 (a later frame of a filtered primitive): one final pass from the kept pair into a new target, store state 2, then the quad |
| later frames | `PushFilters` cached branch + `FUN_141F7A590` | the children are skipped, the stored target is drawn again; `FUN_141C64F00(primitive, 0, 0, 0)` is called only when the render buffer reports **no** persistence, which it does not |

**Persistence flag.** HAL slot `+0xB8` is `LEA RAX,[RCX+0x288]` (`FUN_141BDB270`), the embedded object constructed by the base HAL constructor `FUN_141BD8190` with vtable `143AD6690`; its slot `+0x18` is `141BDB5C0` = `MOV AL,1; RET`. So every cache result is kept. Invalidation is the engine's: Scaleform's CPU render tree replaces or resets a primitive when its content changes, which releases the stored targets (the stand-ins are reference counted through their vtable slots `+8`/`+0x10`).

**The cached quad (state 2, `FUN_141F7A590` lines 160-250).**

| Item | Value |
|---|---|
| programme key | base `0x101`; `+2` = `0x103`; `+3` instead when `HAL+0x214` bit 0 (premultiplied blend: `0x104`); `+0x40` when bit 1; `+8` when bit `0x10`. Catalog entries in `native/frame/primitive_program_catalog.inc`: `0x103` VS 375 / PS 1000, `0x104` VS 375 / PS 1001, `0x10B`/`0x10C` VS 391, `0x143` VS 375 / PS 1002 |
| uniforms | `0xD`: the composite matrix `FUN_141C66270(matrix_state+0x1E0, child placement)` (the same writer as the filter composite); `0x15`: rect to UV `{fx,0,0,rect.left·fx; 0,fy,0,rect.top·fy}` with `fx = rectWidth/textureWidth`, `fy = rectHeight/textureHeight` (1 for a whole target); `0x14`: the stored target, sampler byte 3 (linear, clamp); the colour transform through `FUN_141F770A0` (identity `1446E91C0`, or the child's entry through byte table `1446E9BA0` when `child+0xF & 1`) |
| state | blend `FUN_141BD9D90(HAL, top-of-stack blend, 1)` before and `(top, 0, display bit)` after; depth-stencil `vt+0x2F8` and the mask stencil test `vt+0x2B0` from the target's mask entry (the same block as the filter composite, `F7B930` lines 244-258); the HAL quad (`vt+0x300`) and `vt+0x340(6, 1)` |
| output | the target below, with the primitive's blend: **the subtree is composited as an isolated group** (drawn into a transparent target first), not drawn in place |

The shaders themselves are the engine's ordinary textured-quad programmes (sample times colour transform, saturated); they are already in the native bank (the catalog lines above).

## 2. cacheAsBitmap in the movies (STATIC, `placements.tsv`, `tree.txt`)

| Page | Placement | Cached content | Blend / transform |
|---|---|---|---|
| `menu_04_030_chrmake_colorselect` | sprite 57 (the `Slider` of the colour page), depth 54, character 54 | one child shape (character 53) | none; no colour transform, no blend, no filter |
| `menu_win_04_030_chrmake_colorselect` | sprite 62, depth 84, character 59 | the same structure | none |

So the cached group is a single shape on its own: compositing it in a transparent target and then as a quad differs from drawing it in place only by the isolation (premultiplied edges, the integer-rectangle truncation of the target's origin, one resampling at 1:1). The page is the colour sliders of character creation (hair, skin and eye colour), reached through New Game; `FilterChildPlacement`'s bounds apply as for every filtered primitive.

## 3. The colour matrix (STATIC)

| Question | Answer |
|---|---|
| Who creates kind 8? | only the SWF filter list (`PlaceObject3`, filter id 6) and the AS3 class `flash.filters.ColorMatrixFilter` (`FUN_141E35740` builds the AS3 objects, class tables for kinds 0..5, 8, 10). **Neither occurs**: 0 of 230 filter/cache placements, 0 ABC mentions in 102 movies, none in the 8 font movies |
| What is it for in the HAL? | `FUN_141F78E00` (capability): below feature level 2, every filter set must contain only kind 8, otherwise unsupported; at level ≥ 2 everything is supported. Kind 8 is the fallback filter of low-end hardware |
| Programmes | key `0x180000` (`+1` premultiplied): VS 371/373, PS 1265-1268 (SCALEFORM-FILTER-PASSES §1, in I-0069) |
| Pixel maths (PS 1265) | `t = tex(uv)`; `r = M·t` (rows `cb0[1..4]`); `o = cb0[0] · saturate(t.a + cb0[0].w) + r` |
| Uniforms (`FUN_141F778D0`, kind 8, lines 191-226) | `cxmul` (slot 5, 16 floats) = the filter's 4×4 matrix (`filter+0x18..+0x57`) scaled by the object's colour-transform multiply terms and its alpha; `cxadd` (slot 4, 4 floats) = `(cxform add / 255 + matrix offset (+0x58..+0x67) · multiply) · alpha` |
| Passes | one (`0x180000`); source texture sampler 3; no retained source |

## 4. Which filtered placements reach the HAL, and the witness pages

### 4.1 Text fields do not (OBSERVED + STATIC)

By the type of the placed character: **edittext 119, sprite 22** (placements with a character; 89 further rows are animation updates of an existing placement in the HUD movie, no new character). Observed: Native1268 draws the HUD with the 2×2 item-name shadow and the 4×4 soul-counter shadow visible in both modes and logs `native_ui_filter_plan` only for the sprite glows and blurs; the Title (two 5×5 p3 text shadows) logs none. The text path emits the shadow as extra layers of the text mesh (`FUN_141CFB810` layer kinds, "glyph/shadow quads"). INFERRED that all text-field filters take it; the contrary would show as kind-1 plans on the Title and Equipment pages, which never appeared.

### 4.2 Non-text placements that reach the HAL (STATIC)

| Page (movie) | Sprite / depth | Filter (planned keys) | How the pilot reaches it (to confirm) |
|---|---|---|---|
| **Options** `02_040_optionsetting`, PC twin `win_02_040_optionsetting` | 112 / 1, character 95 | drop shadow 5×5, distance 5, 45°, p1 (`[280000]`) | Esc menu → System → Options (PC build uses the `win_` movie) |
| Inventory pop-ups `02_030_inventory_commandlist`, `02_031_numselect`, `02_032_sortmenu`, `02_033_iconhelp`; `win_02_045_pc_commandlist` | 17, 27, 37, 13, 32 | same 5×5 dist 5 p1 | Inventory → an item's action list, quantity and sort pop-ups |
| Character-creation command list `04_010_chrmake_commandlist` | 15 / 1 | 5×5, distance 10, 10°, p1 (`[280000]`) | New Game, the list opened from the creation top page |
| Warp list `02_080_warp` | 80 / 1 | same distance 10 | bonfire warp |
| Blood-message lists `01_034`, `01_035`; Colosseum `02_200` | 43, 146 | 5×5 dist 5 p1 | not on the High Wall route |
| Kick dialog `02_060_kick` (BG) | 27 / 1 | 2×2 p3 (`[220000, 220000, 280000]`) | multiplayer only |
| Quick-item list `02_000_ingametop`, `Item_0` | 84 / 1 | 2×2 p1 | Esc menu top (visible only with the quick-item clips shown) |
| HUD `01_000_fe` | sprites 58, 112 | 2×2 / 1×1 p1 | chat icon; rare |

**Simple drop shadow witness: the Options page** (and as a second one the character-creation command list): one filtered sprite, one pass, key `0x280000` with `fsize.z = strength 1`, `scolor` black, `offset` = distance 5 at 45° (`-0.05 × 5 × (cos, sin) × scale`), source retained (`srctex` = the subtree target, `t1 tex` = the same target at P = 1), PS 1273.

**7×7 witness: none through the HAL.** All 24 placements with a 7×7 blur are text fields; the large branch (`blurX·blurY ≥ 12800` twips², keys `0x200000` alternating H/V then `0x280000`) cannot occur in a shipped movie. The 7×7 shadows the pilot has seen on screen (Equipment, Inventory headers, the Options title) are text layers. The branch is witnessed synthetically: a control test that feeds a 7×7 p3 plan (`[200000 ×5, 280000]`) through `RunFilterPasses` into a fixture frame and compares the result with a CPU reference of PS 1269/1273 (the box filter `sum(tex(uv + x·texscale))·weight`, §1 of SCALEFORM-FILTER-PASSES in I-0069) on a known alpha mask; the planner part already has its unit test (SCALEFORM-FILTERS-NATIVE F5, the five worked rows).

## 5. Native correction

Orchestration points: `native/frame/game_ui_session.cpp` blocks `NativeFiltersSupported`, `PushNativeFilterBody`, `RunFilterPasses`, `PopNativeFilterBody`, `RecordFilterPass`, `FilterUniforms` (lines 667-805 and the helpers above them); `native/frame/primitive_work.*`, `native/renderer/primitive_pass.*` for packet targets.

### 5.1 F8a: the empty set, non-persistent (smallest correct slice)

1. `NativeFiltersSupported`: a non-null FilterSet with **count 0** and `+0x18 == 0` is supported (today `!count` refuses). The end markers (nil FilterSet) never reach `PushFilters`, so they are unaffected; the capability answer stays 1.
2. `PushNativeFilterBody`: unchanged (the plain path: rectangle from the child bounds, `CreateFilterTarget`, `PushFilterTarget`, normal blend, bit `0x80`).
3. `RunFilterPasses`: `count == 0` skips the loop and the retained-target handling; it sets depth mode 1 and blend `0x12` as the original does, then `PopFilterTarget`, restores the depth mode, builds the composite with `1C66270` and the child colour transform as for a filter composite, applies the top blend, and calls a new `RecordCachedQuad` instead of the final `RecordFilterPass`; then `1C64F00(primitive, 0, 0, 0)` (the existing non-persistent store).
4. `RecordCachedQuad` = `RecordFilterPass` with a `CacheUniforms` writer: key from `hal+0x214` as in section 1, uniforms `0xD`, `0x14` (clamp linear, the subtree target's texture), `0x15` (`fx = rectW/texW`, 1 here), the colour transform; none of the blur-block uniforms (`8`, `0x16`, `0xE`, `0x10`, `0x12`, `0x13`), which the cache programmes do not declare. The vertex layout check (two elements, `R32G32_FLOAT` + `R8_UINT`) is expected to hold (same HAL quad, VS 375) and fails closed otherwise (counted refusal, drawn without the effect).
5. Trace rows: `native_ui_cache_plan` once per distinct (key, target size) and a `cached` counter in `native_ui_filter_frame`.

D3D12 reading: the subtree target is a packet use like any filter target (first referenced draw clears it, last use returns it to the pool by life); the cached draw samples it in the same packet, so no new resource kind is needed.

### 5.2 F8b: persistence through the engine's own store (optional, flag-switchable)

- Replace the explicit non-persistent store by `1C64F00(primitive, 2, {stand-in}, 1)` where the stand-in is the existing `FilterTarget` object given reference-count methods (`vt+8`, `vt+0x10`) and a destructor that **retires** a retained native texture through the fenced retirement used by packets.
- The packet needs a **retained target** kind: a resource owned by the stand-in, created at the first build and sampled in later frames; `PrimitivePass::Build` then takes it as an external texture (the existing `PrimitiveTexture::external` route of the menu offscreen views) instead of an aliased use.
- `NativeFiltersSupported` returns "cached" when `+0x18 == 2` (and `PushNativeFilterBody` takes the original's cached branch: bit `0x100`, marker, no target); `PopNativeFilterBody` draws the quad from the stored stand-in. The original's engine-side invalidation then works unchanged because it is CPU code.
- The same store serves the filtered primitives (state 1: blurred + source pair): the HUD filters would then run their passes once per content change instead of every frame (2 to 4 passes and target uses per frame today, `native_ui_filter_frame`). This is a performance and fidelity gain, not a correctness fix, and is the part of the review's R8 family that applies to the UI.
- Risks: lifetime across a page change (the stand-in dies with the primitive; retirement must wait for the last frame that sampled it, phase B tokens), memory (retained targets of the HUD glows are small, 50×50), and a stale cache if the invalidation signal is missed (compare with `scan-compare`-style shadow runs: every Nth frame also run the uncached path and diff).

### 5.3 F9: the colour matrix, completeness only

Extend the supported set to kind 8 in `NativeFiltersSupported` (planner `FUN_141F75440` already yields one pass `0x180000`/`0x180001`) and add the kind-8 branch of `FilterUniforms` (`cxadd` slot 4, `cxmul` slot 5 from the formula of section 3, texture slot `0x14`, uniform `0x15`, matrix `0xD`; programmes VS 371/373, PS 1265-1268). The layout of VS 371/373 differs from VS 507 (`vfuniforms` with 2 float4 per instance), which `RecordFilterPass`'s layout check accepts only if the elements match: verify with the unit test. No page uses it, so the slice is gated by a synthetic test (a fixture frame with a hand-built kind-8 filter object: identity matrix gives the source, a swap matrix gives swapped channels) and carries no live row.

## 6. Witnesses

| Row | Where | Action | Expected |
|---|---|---|---|
| CB1 | unit | an empty FilterSet primitive through the push/pop pair in the control chain test | one subtree target use, one quad with key `0x103`, `+1` under a premultiplied blend; frame-end hygiene (`native_ui_display_end_scope`) clean |
| CB2 | unit | the colour-select placement shape under a known transform | the cached quad equals the in-place draw within one 8-bit step in the interior and differs only on the rectangle border |
| CB3 | live, Native and Reference | New Game → character creation → hair/skin colour sliders (`04_030`) | `native_ui_cache_plan` once; the slider strip equal to the Reference (compare crop) |
| DS1 | live | Options page (`02_040_optionsetting` / `win_` twin) | `native_ui_filter_plan` kind 1, flags with composite bit, passes 1, blur `[100,100]`, offset about 3.5 px per axis (distance 5 at 45°), keys `[280000]`; the panel's soft shadow equal to the Reference |
| DS2 | live | character-creation command list (`04_010`) | kind 1, distance 10 at 10°, keys `[280000]` |
| DS3 | live | an inventory action list pop-up (`02_030`) | kind 1, 5×5 dist 5 p1 |
| LB7 | unit/fixture (synthetic) | a 7×7 p3 plan, six passes | keys `[200000 ×5, 280000]`; the result matches the CPU reference of PS 1269/1273 on an alpha mask; targets alive ≤ 3 |
| CM1 | unit (synthetic) | a kind-8 filter, identity then channel-swap matrix | one pass `0x180000`; output equals the CPU evaluation of PS 1265 |
| TX1 | live | the HUD and the Title in the same runs | still no kind-1 plan from text fields (confirms section 4.1) |

## 7. Open points

1. Whether the render-buffer persistence answer holds for every code path that calls `FUN_141F7A590` (it is the only reader of `HAL+0x288` slot `+0x18` found; the slot is a constant `true`).
2. The exact menu path to Options and to the colour sliders is the pilot's to confirm (the static side only knows the movies and their placements).
3. The text-filter route is OBSERVED indirectly (no plans for text); decoding the text node's own `SetFilters` override would make it STATIC.
4. VS 375/391 uniform tables were not disassembled (the cached quad uses the engine's standard textured-quad programmes already decoded by the native bank); the layout check in `RecordFilterPass` is the guard.

## Exports

Existing exports used: `decompiled/DS3-1152-F-` + `01F7A590`, `01F7B930`, `01BDC4E0`, `01BDB800`, `01C64F00`, `01F778D0`, `01F78E00`. Read through Ghidra and not exported: `141BDB270`, `141BDB5C0` (undefined getters, `I0068Dis.java`), `141BD8190`, `141F72580`, `141E35740`, `141CDFC70`, `141C26540`, `141C27170`, `141CFB810`. Tools: `Scan-GfxFilterContext.ps1` (placement context, character types, all eight filter ids, ABC mentions, display-list dumps); `Inventory-Archives.ps1` (I-0063, pattern `\.(gfx|swf)`).
