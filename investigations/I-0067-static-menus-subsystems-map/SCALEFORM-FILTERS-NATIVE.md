# Scaleform filters and cacheAsBitmap in the native HAL: ordered implementation checklist (STATIC; INFERRED where marked)

Written 2026-10-11 for the menus workstream. Inputs: [MENUS-MAP](MENUS-MAP.md) section 78 (what the 102 menu movies use),
section 36 (HAL methods), the decompiled HAL code (`decompiled/DS3-1152-F-01BDC4E0`, `01BDB800`, `01BDBFC0`, `01F78E00`,
`01F75440`, `01F75180`, `01F778D0`, `01F7A590`, `01F7AC90`, `01F7B930`, `01F7B130`, `01BDEE90`, `01BD9D90`, `01C64F00`)
and the native HAL in `native/frame/game_ui_session.cpp`. Nothing here was run.

## 0. Position

- What the pages need: **DropShadow** (137 placements, 53 pages), **Glow** (52, 3 pages), **Blur** (90, the HUD movie), and
  **cacheAsBitmap** (2 placements, the colour-select page). No other filter type, no `Drawable*` call (section 78).
- Native today: `state.table[0x240/8] = NativeFilterCapabilities` returns 0, so the original `PushFilters` takes its
  "unsupported" branch (`01BDC4E0`, the `cVar3 == 0` test), `PushNativeFilter`/`PopNativeFilter` only keep the CPU scope, and the primitive
  is drawn **without** its filter. `NativeCacheScope` refuses any primitive that already owns a cached target
  (`native_ui_cached_filter_target_not_owned`) and `BeginCpuDisplay` refuses offscreen displays
  (`native_ui_display_offscreen_unavailable`).
- Native design that the work must fit: the native HAL **records** `frame::PrimitiveDraw` packets (`native/frame/primitive_work.h`)
  that one `PrimitivePass::Encode` replays into a single colour + stencil target. There is no render-target change inside a
  packet and no draw can sample a target produced earlier in the same packet. Filters need exactly that (section 4, F1).
- Method (the project's): keep the original **CPU** algorithm (pass planning, parameters, uniform writes) running through the
  native shader interface (`PrepareNativeUiCpu` over `1F784C0`, the CPU uniform writer `1F78BC0`/`1F78AD0`); replace only the
  GPU-touching calls.

## 1. The data the filter code reads

- The filtered primitive: `primitive+0x48` = FilterSet, `primitive+0x18` = cache state (0 plain, 1 cache being built,
  2 cache present), `+0x20/+0x28` cached target references, `+0x30` child list (the filtered display tree). Accessors:
  `FUN_141CC2AB0(set)` = count, `FUN_141CC2AA0(set, i)` = filter object.
- Filter object (offsets read by `1F75440`, `1F75180`, `1F778D0`; field names INFERRED from their use):

| Offset | Use | Type |
|---|---|---|
| `+0x10` | kind: 0..5 blur family (0 blur, 1 drop shadow, 2 glow, 3 bevel, 4 gradient glow, 5 gradient bevel; 0xB: never reaches the blur planner, `01F75440` returns 0 passes for it), 8 colour matrix, 10 displacement map | int |
| `+0x18` | flags: bits 0..2 mode (selects the final-pass programme), `0x10`, `0x20`, `0x40`, `0x80` modifiers of the final programme | uint |
| `+0x1C` | pass count | int |
| `+0x20`, `+0x24` | blur X and Y, **in twips** (the code multiplies by 0.05 to get pixels) | float |
| `+0x28`, `+0x2C` | shadow offset X and Y (scaled by -0.05 and the view scale) | float |
| `+0x30` | strength (final pass alpha factor) | float |
| `+0x34`, `+0x38` | colours, 4 bytes each (`FUN_141D12410` unpacks to 0..1 floats) | bytes |

  The Scaleform loader fills these from the SWF filter list; the native run keeps that loader (CPU). The kinds used by the
  menus are only 0, 1 and 2. The first native read per kind (section 5, row L1) confirms the field values below.

## 2. The original flow, step by step (what F3..F7 transcribe)

1. **Capability** `FUN_141F78E00` (slot `+0x240`): `HAL+0x11A0` vtable `+8` returns a feature level; for level < 2 and any filter whose
   kind is not 8 the answer is 0 (flag `DAT_144951BD0` set), otherwise 1.
2. **`PushFilters`** `FUN_141BDC4E0` (slot `+0x1D0`), supported branch, plain primitive (`+0x18 == 0`):
   - `applyDepthStencilMode(1)` (`+0x2B0`) and raster mode 0 (`+0x2F8`);
   - the filtered tree's bounds are read from the first child: floats at `entry+0x10/+0x1C/+0x24/+0x2C`, where `entry` is selected
     through the byte table `1446E9BA4` (stride 5, index `child+0xF & 0xF`, times 16). Origin `x0 = +0x1C`, `y0 = +0x2C`; size
     `w = +0x10`, `h = +0x24` (each truncated to int); the rectangle is `{x0, y0, x0 + w, y0 + h}` and the target size `{w, h}`
     (lines 33..47 of the export);
   - a temporary render target of that size is created (`vt+0x10`, `FUN_141F73750`: texture-manager `vt+0x30` target, plus a
     depth-stencil surface when the flag is set), pushed with **`PushRenderTarget`** (`vt+0x20`, flags `8`, the rectangle, clear
     colour 0 = transparent), blend state 1 (normal) is applied (`FUN_141BD9D90(HAL, 1, 0, 1)`), flag `HAL+0x58 |= 0x80`;
   - the pair {primitive, target} is pushed on the vector at `HAL+0x178` (16-byte entries, `FUN_141BDD980`).
   - Cached primitive (`+0x18 != 0`): flags `|= 0x100`, `HAL[0x227] = stack depth`, the cache marker `vt+0xC8()+0x30 = 1`; no new target.
3. The subtree is drawn between push and pop by the normal primitive path (`DrawProcessedPrimitive`, masks and blends nested).
4. **`PopFilters`** `FUN_141BDB800` (slot `+0x1D8`): pops the entry, then
   - plain: **`vt+0x2E0` = `FUN_141F7B930`** (the pass loop);
   - cached: **`vt+0x2E8` = `FUN_141F7A590`**, clears the `0x300` flags and the `+0x30` marker.
5. **Pass loop** `FUN_141F7B930(HAL, entry)`, entry = {primitive, subtree target}:
   - `n = FUN_141CC2AB0(set)`; `vt+0x300` (bind the HAL quad vertex array), `applyDepthStencilMode(1)`, blend 18 (FilterBlend)
     with `FUN_141BD9D90(HAL, 0x12, 1)`, `vt+0x290(1)`;
   - for each filter `f` (in set order): `P = FUN_141F75440(HAL+0x234, f, HAL+0x214, passList)` fills the **programme keys** of its
     `P` passes (section 3). For kinds 1..5 the unfiltered source target is **kept** (`aplStack_248[2]`) so the final pass can
     combine the blurred result with it;
   - for each pass `p` of each filter: unless it is the very last pass of the very last filter, ensure a second temporary
     target of the same size exists (`vt+0x10`), `PushRenderTarget` it (flags `8`, its full rectangle), build the view matrix
     (8 floats from `HAL[0x4B]` vtable `+8` with the target size), call **`vt+0x358` = `FUN_141F7AC90(matrix, identityCxform
     = DAT_1446E91C0, filter)`** (it runs `FUN_141F778D0`, which writes the uniforms, then `vt+0x300`, then `vt+0x340(6, 1)`
     = draw 6 indices of the quad), `PopRenderTarget(4)` (`vt+0x28`), and **swap** the two targets (source <-> destination);
   - the last pass of the last filter is not rendered into a temporary target: after the loop the code restores the target
     below, rebuilds the **matrix** argument with `FUN_141C66270(.., HAL[0x4B]+0x1E0, childEntry)` (uniform `0xD`); the cxform argument is the identity `1446E91C0`, or the child entry selected through the byte table **`1446E9BA0`** when `child+0xF & 1`; applies the
     top-of-stack blend (`FUN_141BDEE90`, default 1) and draws that final pass directly (`vt+0x358`), then
     `FUN_141BD9D90(.., 0, flag)` restores the blend. When the set is empty (`P == 0`) it calls `vt+0x2E8` instead.
   - `HAL+0x284 += n` (statistics), the temporary targets are released (`vt+0x28` on each).
   - If a temporary target cannot be created (`vt+0x10` returns null) the loop gives up, sets one of the flags
     `DAT_144951BD4/BD5/BD6`, releases and stores an empty cache result (`FUN_141C64F00(primitive, 0, 0, 0)`): the primitive is
     (this is a diagnostic flag, not a skip): on both failure paths (`01F7B930` lines 119..137 and 179..200) the loop re-takes the subtree
     target, ends the filter loop, and the code after the loop still calls `vt+0x358` with the pass reached (`uVar14`), so the subtree is composited
     through that pass's programme (in practice pass 0, strength 1). Only the cached build (`01F7A590` state 1, flag `BD6`) draws nothing. Native keeps this:
     on a failed target allocation, draw the subtree with the pass reached, never drop it.
6. **Cached primitive** `FUN_141F7A590(HAL, primitive)` (`+0x18` selects):
   - `1` (build): one draw of the **last pass of the last filter** (index `P-1`) into ONE new target, stored by `FUN_141C64F00(primitive, 2,
     {target}, 1)` (count 1) with `target+0x20+0x10 = primitive`; then `vt+0x2E8` again, now in state 2. (The pair blurred + retained source is what
     `01F7B930` stores, count 2, when the cache is persistent;)
   - `2` (present): draws the cached target as a textured quad: programme index `0x101` (the `OR 0xC` before the test makes it the only live base; +2, +3 when `HAL+0x214` bit 0,
     +0x40 when bit 1, +8 when bit `0x10`), uniforms 0xD (8 floats ortho), 0x15 (8 floats target-rect to UV), 0x14 texture;
     blend `FUN_141BDEE90`; then `FUN_141C5B3B0(vt+0xD0(), 1)` and `FUN_141C64F00(primitive, 0, 0, 0)` unless `vt+0xB8` says the
     cache is persistent.

## 3. Programme keys and uniforms per filter (`FUN_141F75440`, `FUN_141F75180`, `FUN_141F778D0`)

Constants read from the image: threshold `143B4E400` = 12800.0; pixel scale `143D7ED58` = 0.05; `143D7EDB4` = 0.5; `143D7EE7C` = 2.0;
`142AFF190` = 1e-12 (minimum offset); `142AFF964` = -0.05; `143D7EE10` = 1.0; `143D7EFF4` = 255.0; colour defaults `1446E91C0` = multiplier (1,1,1,1), addend (0,0,0,0) and
the integer tail `1,1,2,2,4,4,1,1`. `FUN_141FD27D4` is a ceiling function (INFERRED from its bit logic).

Pass planner for kinds 0..5 (`P` = number of passes, `k[i]` = key of pass `i`; premult = `HAL+0x214 & 1`, which `FUN_141BD9D90` sets when the mapped blend entry is 3 or 4, INFERRED Multiply and Screen):
- `small = (blurX * blurY < 12800)` in twips squared (that is, 32 px squared: 5x5 px is small, 7x7 px is large).
- `small`: `P = max(passes, 1)` (kind 0xB: `P = 0`); `k[0..P-2] = 0x220000`.
- `large`: `P = 2 * passes`; `k[0..P-2] = 0x200000` (even pass = horizontal, odd = vertical).
- last pass `k[P-1]`: mode = `flags & 7`. Mode 0 (plain blur): `0x220000` (small) or `0x200000` (large). Modes 1, 2:
  `0x280000`; mode 3: `0x2D0000`; mode 4: `0x288000`; mode 5: `0x2C4000`; then `+0x400` if `flags & 0x80` or (mode 1 and `flags & 0x40`
  and not `flags & 0x10`) when `flags & 0x20` is clear; `+0x800` (or `+0x1000` when the base has bit `0x40000` or mode 4) when
  `flags & 0x20` is set; `+0x2000` when `flags & 0x40` or `flags & 0x10`; **`+1` when premult**.
- Other kinds: 8 (colour matrix) one pass `0x180000` (+1 premult); 10 (displacement) one pass `0x300000` (`0x300200` or `0x300100` when the field `+0x30` is 2 or 3).

Blur uniforms (`FUN_141F75180`, per pass index `i` of `P`, view scale `s`):
- `bx = max(1, ceil(0.05 * blurX))`, `by = max(1, ceil(0.05 * blurY))` pixels, clamped to 1 pixel; the offsets are 0 when `passes == 0`.
- Keys `0x200000/1` (large, separable): the offset is always written to component `[0]` and `[1] = 0`; even `i`: `off = (bx - 1) * 0.5 * s.x` and the texel-size uniform has its **second** component zeroed (`param_7[1] = 0`); odd `i`: `off = (by - 1) * 0.5 * s.y` and its **first** component zeroed (`*param_7 = 0`). The axis is therefore selected by zeroing one component of uniform `0x16`, not by the offset.
  `weightScale = 1 / ceil(2 * off + 1)`.
- Other keys, last pass (and `i != 0`): `off.x = 0`, `off.y = (by - 1) * 0.5 * s.y`; otherwise (combined small pass) both offsets, `weightScale = 1 / (ceil(2*offx + 1) * ceil(2*offy + 1))`.
- Component `[2]` = 1.0, except on the last pass where it is the filter strength `+0x30`. Offsets are clamped to at least 1e-12.

`FUN_141F778D0` uniform slots (shader-interface indices): `0xD` 8 floats (ortho matrix of the target); `0x15` 8 floats (rect to UV of the
source: `{sx, 0, 0, offx*sx}, {0, sy, 0, offy*sy}` with `sx = srcRectW / texW`); `0x14` source texture (clamp sampler 3); `5` and
`4` the cxform multiplier and addend (`DAT_1446E91C0` or the child's); `8` the blur block `{offX, offY, strength, weightScale}`;
`0x16` the texel size `{1/texW, 1/texH}` (written after `01F75180`, with one component zeroed for the separable keys, see above). When the programme is a shadow/glow final pass (`key - 0x280000 < 0x53002`): `0xE` 2 floats
`offset * -0.05 * view scale`, `0x13`/`0x12` the retained original source texture and its texel size, `0x10` (kinds 0..3) or texture `9` (the colour
ramp, kinds 4 and 5) with the colour from `+0x34` and `+0x38`; kind 3 adds `0x11`. After the writes `FUN_141F80640(shader, 1)` commits.

Worked pass lists for the filters the menus use (field twips = 20 x pixels; flags read live):

| Filter | Field blur | `small` | passes | P | keys |
|---|---|---|---|---|---|
| DropShadow 1x1 (2), 2x2 (54), 3x3 (9), 4x4 (2), 5x5 (46) | 20..100 | yes | 3 or 1 | 3 or 1 | `P-1` x `0x220000`, then `0x280000` + flag bits |
| DropShadow 7x7 (24) | 140 | **no** | 3 or 1 | 6 or 2 | `P-1` x `0x200000` alternating H, V, then the last |
| Glow 5x5 p3 (35) | 100 | yes | 3 | 3 | `0x220000`, `0x220000`, `0x280000` + bits |
| Glow ramp 0.7..4.3 px (8) | 14..86 | yes | 1..2 | 1..2 | as above |
| Blur 0..3 px p1..p3 (90) | 0..60 | yes | 1..3 | 1..3 | all `0x220000` |

## 4. Ordered implementation checklist

Each step: what to build, the expected value, the check. Steps F1..F2 are shared with cacheAsBitmap and with every later offscreen page effect.

| # | Step | Expected value / rule | Check (witness) |
|---|---|---|---|
| F1 | **Targets inside a packet.** Extend `PrimitiveWorkPacket` with `targets` (id, width, height, format, clear colour, depth-stencil flag) and a `target` id on `PrimitiveDraw` (0 = the frame target). `PrimitivePass::Encode` encodes the draws as ordered segments by target, with `RENDER_TARGET <-> PIXEL_SHADER_RESOURCE` barriers between a producing and a sampling segment; a target sampled by a later draw is bound through `PrimitiveTexture::external` (existing 1169 mechanism) | Targets pooled by size and format and retired by fence (section 36 "pooled RTs"). Format: R8G8B8A8_UNORM until the texture manager format table (`46F8EB0`, `FUN_141F83DC0`) is read (UI-INGRESS 60-64); stencil D24S8 only for the depth-stencil flag | unit test on `ProducePrimitiveWork` ordering; D3D12 debug layer clean on one filtered frame |
| F2 | **Capability.** `NativeFilterCapabilities` becomes the transcription of `FUN_141F78E00`: return 1 when every filter kind is in the supported set {0, 1, 2, 8, 0xB} (0xB takes no pass and goes to the cached draw; 3, 4, 5 and 10 stay refused until implemented: the CPU-only branch of `PushFilters` then runs, as today); count the refusal in the debug counter (`DAT_144951BD0` analogue) | menus only use kinds 0, 1, 2 | a page with a shadow no longer logs the capability refusal |
| F3 | **`PushNativeFilter`**, plain primitive: transcribe the supported plain path of `01BDC4E0` (lines 47..69 and 83..100 of the export) with native calls: depth-stencil mode 1, raster 0, integer rectangle from the child bounds (byte table `1446E9BA4`), `CreateTempTarget(w, h)` = F1 target, `PushTarget(rect, clear = transparent)`, blend normal, `HAL+0x58 |= 0x80`, entry on the `HAL+0x178` vector with the native target handle in the second field (`+8`) | the entry's `+8` is non-null (today it is null and `PopNativeFilter` fails `native_ui_filter_target_not_owned` on non-null) | temp target size equals the rectangle size (L2) |
| F4 | **Subtree draws go to the temp target.** Every `Emit*` draw recorded between push and pop carries the temp `target` id; masks and blend scopes nested inside keep their own handling on that target (stencil is per target: a depth-stencil surface is needed when masks are used inside a filtered tree) | the HUD movie nests masks (19) and filters; check one such subtree | image of the subtree target equals the original's (L2) |
| F5 | **Pass planner + uniforms through the original CPU code.** At `PopNativeFilter` call `FUN_141F75440` for each filter and run the `FUN_141F778D0` uniform writes through the shader interface of the session (the same machinery as `NativeUiPrepare`). Do not transcribe the numeric formulas of section 3; use them as the unit-test oracle | for DropShadow 2x2 p3: keys `0x220000, 0x220000, 0x280000+bits`; 7x7 p3: `0x200000 x5` then the last; blur offsets `(bx-1)*0.5*scale` | a native unit test over the five rows of the worked table |
| F6 | **Pass loop** (transcribe `01F7B930`): ping-pong between two temp targets (a third retained for the original source on kinds 1..5), view matrix per target size, quad of 6 indices, programmes from the session's UI bank by key, blend FilterBlend (key 18) while the intermediate passes run, `PopTarget` per pass, swap. Failure rule: a failed `CreateTempTarget` ends the filter loop, the subtree is composited through the pass reached (section 2, step 5), and the counter (`BD4/BD5` analogue) increments; only the cached build draws nothing | `P` draws + 1 composite per filtered primitive; at most 3 temp targets alive | draws per filtered primitive in a trace equal `sum(P) + 1` |
| F7 | **Final composite** into the target below: matrix rebuilt with `FUN_141C66270` (uniform `0xD`), cxform = identity or the child entry through `1446E9BA0`, top-of-stack blend (`FUN_141BDEE90`), then the previous blend restored. For a DropShadow/Glow final pass the shader samples both the blurred target and the retained source (slots `0x14` and `0x12`) | the object is drawn once, shadow behind it (composite-source on in every menu filter) | title text shadow visible (L3) |
| F8 | **Cached primitives** (`+0x18 != 0`, used by cacheAsBitmap): transcribe the three states of `01F7A590` (section 2, step 6): one cache target in state 1 (a pair when the cache is persistent) owned by the native primitive-cache record (`FUN_141C64F00` semantics, `+0x20/+0x28`), freed when the primitive releases; `NativeCacheScope` stops refusing when it owns the pair; the state-2 draw is one textured quad (programme index from `HAL+0x214`) | cache survives frames until the Scaleform side invalidates it | colour-select page: swatch strip equals the original (L4) |
| F9 | **Colour matrix kind 8** (not used by the menus): one pass `0x180000`, uniforms 4 (4 floats) and 5 (16 floats). Implement only when a page needs it; keep it in the capability set so the code path exists | the `Drawable*` calls stay unimplemented (section 78: none used) | not witnessed |
| F10 | **Frame-end hygiene**: `BeginCpuDisplay`/`EndCpuDisplay` assert the filter stack, cache scopes, blend and mask stacks are empty (already: `native_ui_display_end_scope`); add the target pool trim at frame end | no leaked temp targets after a page change | 600 frames at the Title and in game with the debug layer |

## 4b. Decisions the review forced (read with the table above)

- **What a temporary target is.** `FUN_141F778D0` and `FUN_141F7B930` dereference the target objects (`targets[0]->vt+0x18()` returns a texture object whose `+0x38/+0x3C` are the size, the target
  rectangle is at `+0x30..+0x3C`, `+0x28/+0x2C` are read by the pass loop, `vt+8/+0x10/+0x28` are called). Decision: **a native temporary target is a Scaleform-shaped CPU object** (the
  engine's own render-buffer/texture classes built through the session's image manager, GPU slot nil, the same method as the portraits' "genuine CPU objects, GX objects nil"), with a side
  table from that object to the native D3D12 resource. Then `01F778D0`, `01F7B930`-side reads and `01F7AC90` run as the original CPU code (F5, F6); only `PushRenderTarget`,
  `PopRenderTarget`, `vt+0x10` and the draw submission are native. The alternative (a plain native handle) forces transcribing `778D0` and `7B930` fully and is not recommended.
  The shader interface (`vt+0x10(key)`) must resolve the filter keys `0x2xxxxx` from the session's UI programme bank (`PrepareNativeUiCpu` already does for fills).
- **Projection and viewport on push/pop.** The original `PushRenderTarget` (`01BDCBD0`) and `PopRenderTarget` also switch the viewport rectangle (`HAL+0x1144..0x117C`)
  and the projection matrix object (`HAL[0x4B]`, `+0x1C0..+0x1CC` offsets, `+0x160/+0x180` identity blocks). Without that switch the subtree draws recorded between push and pop land with the frame
  projection. In the packet model: each recorded draw carries the viewport and the matrix constants of the target it was recorded for (`PrimitiveDraw::viewport`, the constants block); the target
  switch must therefore be a state change of the **recorder**, which the native `NativeViewport` already models for the frame.
- **Ownership (F1).** `primitive_pass.h` keeps device, queue and targets with `SceneRenderer`; `Encode` takes one colour/stencil handle. The pool of temporary targets (RTV, DSV, SRV per size
  and format) is owned by `SceneRenderer`, handed to `Encode` per segment, and `PrimitiveWorkSource` needs the same `targets` and per-draw `target` fields as `PrimitiveWorkPacket`.

## 5. Live and capture rows

| Row | What | Expected |
|---|---|---|
| L1 | Break once on `FUN_141F75440` per filter kind (original, Title or HUD): read `filter+0x10/+0x18/+0x1C/+0x20/+0x24/+0x28/+0x30` | kind 1 for DropShadow with blur 40..140 and strength 1.0; flags value to record; pass count 1..3 |
| L2 | Original and native: size and content of the subtree target of the first HUD glow and of a list-page text shadow | same width and height; same alpha silhouette |
| L3 | Title: the logo and menu text drop shadows (black, 2 px, distance 2, 45 degrees) | present, soft, no halo outside the blur |
| L4 | `04_030_chrmake_colorselect` and `win_04_030_chrmake_colorselect` through Create Character | state 1 then 2 of the cache; no per-frame rebuild |
| L5 | HUD in game (High Wall): the 35 blue glows (colour 64, 92, 255, strength 1.5, 3 passes) and the banner blur ramps (YOU DIED) | same intensity as the original; blur ramps animate |
| L6 | Frame-time cost: the filtered HUD frame's draws and target allocations per frame (the native rate is 7 to 8 fps at the time of writing) | pooled targets, no per-frame allocations after warm-up |

## 6. Native implementation (2026-10-11, branch `claude/jolly-brattain-8412e6`)

Code: `native/frame/game_ui_session.cpp` (block "SCALEFORM-FILTERS-NATIVE F2..F7"), `native/frame/game_ui_shaders.cpp`
(`PrepareNativeUiFilterCpu`), `native/frame/primitive_work.*`, `native/renderer/primitive_pass.*`. Inputs added by I-0069
[SCALEFORM-FILTER-PASSES](../I-0069-static-menu-audio-map/SCALEFORM-FILTER-PASSES.md) (programmes, slot map, census).

- **F1 revised (D3D12 transient targets, D-0157).** A packet target is one *use*: every push of a temporary target
  (the original clears at each `PushRenderTarget` flag 8, `1F7BD30`) gets a new packet target, cleared to transparent at
  its first reference (written or sampled). `PrimitivePass::Build` gives each use a pooled resource of its size for its
  life (first to last referencing draw); a use starting at a draw takes its resource before the uses ending at that draw
  return theirs, so a draw never reads and writes one resource. The build holds the resources until its fenced
  retirement. Limits raised to 4096 draws, textures and targets per UI packet.
- **4b decision changed.** `1F778D0` ends with the GPU commit `1F80640`, which dereferences the D3D11 shader objects at
  writer `+0xFF8`/`+0xFF0`: it cannot run against the native shader-interface object. So the pass loop (`1F7B930`) and
  the uniform glue (`1F778D0`, blur family) are transcribed natively, and every computation inside them stays the
  original CPU code: planner `1F75440`, blur block `1F75180`, writers `1F78AD0`/`1F770A0`/`1F81EB0`, colour unpack
  `1D12410`, composite matrix `1C66270`, blend selection `1BD9D90`. Temporary targets are CPU stand-ins shaped as the
  original reads them (target `+0x28` size, `+0x30` rectangle, vt+0x18 texture; texture `+0x38` extent, vt+0x68 bind).
  The HAL quad (`HAL+0x3B218`, stride 12) is owned natively: two triangles over the unit square, batch index 0.
- **Push/PopRenderTarget** (`1BDCBD0`/`1BDBB70`) are transcribed for their CPU part: viewport block `0x1144..0x117F`,
  `HAL+0x58` bits `0x10/0x20`, a genuine copy of the matrix state (`1C65EF0` + vt+0x30), user matrix identity, clip
  offsets and identity blocks; native masks and the stencil mode are per target (the original's per-entry mask count).
  A draw recorded while a target is current lands on it, with the viewport `0x114C..` when bit `0x10` is set (`1F7BFF0`).
- **Blend arguments** read from the loop's listing (`decompiled/DS3-1152-F-01F7B1B0.asm.txt`): push `(1, 0, 1)`, loop
  `(0x12, 1, 0)`, composite `(top, 1, 1)`, restore `(top, 0, HAL+0x58 bit 4)`; `top` is the innermost blend primitive's
  mode (this HAL keeps that stack natively).
- **Explicit policies.** The cache result is the non-persistent one (`1C64F00(primitive, 0, 0, 0)`; the original
  stores the targets only when the render-buffer manager keeps filter caches, not read). `+0x18` is the stored cache
  result mode, written only by `1C64F00` from the pass loop, the cached draw and PopBlendMode, all replaced here, so it
  stays 0 natively (re-review); filter sets that are empty (cacheAsBitmap) or hold kinds other than 0..2 keep the
  CPU-only branch: drawn without the effect, which for cacheAsBitmap is the same image uncached (F8/F9 open).
  A UI packet's sampler descriptors are shared per distinct pair (a shader-visible sampler heap holds 2048).
  An empty or unbounded subtree rectangle draws the subtree without its filters (counted).
- **Trace rows:** `native_ui_filter_plan` (each kind/flags/passes once: fields, keys, target size) and
  `native_ui_filter_frame` (first 8 filtered frames, then every 300th: native, unfiltered, refused, passes, target uses).
- **F2 corrected by Native1264-1265 (OBSERVED, then read in code).** The capability slot `+0x240` is also read by the
  render queue: the filter item's prepare method `1C64340` calls it, then the cache scope `+0x1C8` with
  `ending = (FilterSet == nil)`; the end of a filter is a separate marker primitive with a nil FilterSet (its emit
  `1C638A0` calls PopFilters), and the blend item `1C642F0` does the same with mode `0x15`. Answering 0 for the nil set
  left the scope opened by the begin unclosed (`native_ui_display_end_scope ... caches1`, Native1265). The answer is now
  1 for every primitive, as the original's at feature level 2 and above, and `NativeCacheScope` closes the innermost
  scope without requiring the same primitive.
- **Uniform rows (Native1266-1267).** The HAL writes float2 values into float4 rows (`texscale`, `offset`,
  `srctexscale`) and only instance 0 of the batched VS 507 row (576 floats, stride 6): the native writer completes the
  rows it has written with zeros (never read); a row with no written value stays missing.
- **Fresh review (Opus, 2026-10-11) applied:** premultiplied Add blend (state 28, ONE/ONE; a filter composite inside an
  Add blend), frame-plan texture limit 4096 like the packet, refusals (drawn unfiltered, counted) for an invisible
  display, a rectangle empty after truncation and a target above 4096 a side, failure reporting instead of an
  exception unwinding through engine frames, mask references released on close, one texture-to-target map per build.

### Live witnesses (Native1264-1269, Reference1264/1269, Thomas's save, Argo at High Wall)

| Run | Result |
|---|---|
| Native1264, 1265, 1266, 1267 | title and Load Game drawn; refused at the first HUD frame (cache scope, then uniform rows); restored by hash |
| **Native1268** | **in game, exit 0, 0 faults, no refusal, D3D12 validation clean**: HUD filters run natively (frame rows: 1 filtered primitive, 2 to 4 passes, 2 to 4 target uses per frame); plans seen: glow kind 2 p3 `[220000, 220000, 280000]` (blur 100 twips, strength 1.5, colour `FF405CFF` = the HUD blue glow, 50x50), blur kind 0 p1 `[220000]`, glow p2 `[220000, 280000]` (strength 0.75), glow p1 blur 0 strength 0 `[280000]` (the "no glow" red default). HUD crops equal to Reference1264 (`i66-native-1268/hud-compare-1268.png`); title equal (`title-compare-1268.png`) |
| **Native1269 / Reference1269** | Esc menu opened and closed (50 ms Esc), both exit 0; the menu is identical (`i66-native-1269/esc-compare-1269.png`); same plans as 1268 |

| **Native1270 / Reference1270** | Equipment page opened from the Esc menu (`-EquipPage`), both exit 0: the page is identical (`i66-native-1270/equip-compare-1270.png`); same plans as 1268, no kind 1 |

Not witnessed live: a drop shadow (kind 1) and the large separable branch (`0x200000`, 7x7): none was drawn as a
filter primitive on the title, the Esc top, the Equipment page or the HUD in these runs (the census counts placements
in the movies, not shapes on screen); the census (I-0069 section 5) places them on the
Equipment, Inventory, Status and Options pages (PW3, PW4). F8 (cacheAsBitmap, colour select) and F9 (colour matrix)
stay open: those primitives are drawn without their effect, as before.

Exports for the functions used here are in `decompiled/DS3-1152-F-` + `01BDC4E0`, `01BDB800`, `01BDBFC0`, `01F78E00`, `01F75440`, `01F75180`,
`01F778D0`, `01F7A590`, `01F7AC90`, `01F7B930`, `01F7B130`, `01BDEE90`, `01BD9D90`, `01C64F00`, `01D12410`, `01F78AD0`, `01F81EB0`, `01F80640`,
`01F770A0`, `01F73750`.

See also (I-0068 part AZ, 2026-10-11): [SCALEFORM-CACHE-COLORMATRIX](../I-0068-static-yebis-ffx-lights-spec/SCALEFORM-CACHE-COLORMATRIX.md) for F8 (cacheAsBitmap: path, native slice F8a/F8b), F9 (the colour matrix is used by no movie), why text-field filters never reach the HAL, and the witness pages for a simple drop shadow (Options page, character-creation command list) and the synthetic test for the 7x7 branch.
