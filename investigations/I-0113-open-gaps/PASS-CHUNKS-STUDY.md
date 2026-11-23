# How DS3 executes a pass's sorted commands: batches, segments, chunks and their starting state

Study thread, 2026-10-30 (Paris, evening), for the I-0113 pilot (relayed by the coordinator at 19:08): the follow-up of
[ASHEN-FLASK-STUDY](ASHEN-FLASK-STUDY.md), after the live reading `i113-s9-flask-original` (19:02: in the effect pass 0x29 the
Ashen Estus Flask, `s07002`, key 59555, class 3D65F88, is the first command of its chunk; keys 59443-59488 ran on one thread,
59497-59543 on another; at the flask's draw blend 0x10 and depth 0x1E0, set outside the callbacks; natively one execution chains
59497-59543 before the flask, which inherits the additive 0x20). Read-only: GhidraMCP on Thomas's Ghidra (DarkSoulsIII.exe
1.15.2), a linear listing of two bodies without a function ([I0111LinearListing.java](../I-0111-dlss-reflex/tools/I0111LinearListing.java),
headless on the mirror `ghidra-mirror-i0068`), the policy table read from the image. No run. OBSERVED unless marked.

## Answer in short

1. **The key** (`1400BF4C0`): a 32-bit word `group << 29 | pass << 22 | sub << 16 | order`, from the pass context's `+0x120`
   (group, 3 bits), `+0x124` (pass, 7 bits), `+0x128` (sub, 6 bits) and `+0x12C` (order, 16 bits). In the scene view the pass is
   0x45 and **the engine passes 0x21, 0x29, ... are the `sub` field** (the predicate in point 4 tests pass 0x45 with sub 0x29).
   The pilot's keys (59443 ... 59555) are the low 16 bits.
2. **Recording**: each recorder (a pass context per collecting thread) appends its commands and keeps a table of
   `{key, command, flag +0x10, flag +0x11}` (0x18 bytes); every **64** entries it stable-sorts the table by key and queues it to the
   owner (`140128240`, under the owner's lock); the remainder is queued when the recorder closes (`1400B7A00`, `1401389D0`).
3. **Before execution** (`140128040`): all queued tables are concatenated and **stable-sorted by the unsigned 32-bit key,
   ascending** (`14012A4B0`, insertion sort by runs of 32 then merges; comparator `key < key`, `14012ACA0`). Equal keys keep their
   queueing order, which depends on the collecting threads.
4. **Execution** (`14013EA60`): the sorted list is walked; a predicate (the executor's slot `+0x18`, `0x140139AA0`, bytes read:
   `cmp byte [rdx+0x10],0; jz; mov al,1; ret` then the key test) is true for an entry whose flag `+0x10` is set (the recording
   pass context's `+0x422`, when its `+0x423` is clear) and for **pass 0x45, sub 0x29** (`(key & 0x1FC00000) == 0x11400000 &&
   (key & 0x3F0000) == 0x290000`) when `[[0x144961B50]+0x4E20]` is set; runs where it is false go to the main context (`14013F030`), runs where it is true are **split and sent to the GX
   workers** (`14013F120`) when multithreaded rendering is on (`[[pass ctx+0x450]+0x21F3]`). So **the effect pass 0x29 is the one
   that is cut into chunks**; the others run one segment at a time on the main context.
5. **The cut** (`140139560`, the executor's slot `+0x10`): first into **segments**, maximal runs of the same top 16 bits (group,
   pass, sub); then each segment is cut by a policy found in a 36-entry table at `0x14473D5A0` (entries `{pass, sub, fixed
   count, max, divisor, min per chunk, type, worker rule, byte}`, matched by pass and sub ranges). For pass 0x45, subs 0x1D to
   0x30 (0x29 included) the entry is `{0x45, 0x1D, 10, 0, 0, 5, 13, 0, 0}`: **`k = min(10, ⌊n/5⌋)` chunks** (`n` = the segment's
   entry count; one chunk when `k < 2`), **contiguous slices of the sorted segment of `⌊n/k⌋` entries, the first `n mod k`
   slices one longer**; at most 62 chunks a frame. Chunk `i` goes to worker `i mod [GXRS+0x2D0]` (`GXRS` = `[0x144961AB0]`; the
   policy's worker rule 0 < 6), its own deferred context.
6. **The starting state of a chunk** (`14013EEB0` with slot `+0x28`, `140139980`): each chunk run starts with a fresh
   « last state » record (zero), so its **first entry always applies the state block registered for its top 16 bits**:
   `1401284D0(owner, key & 0xFFFF0000)` looks it up in the owner's map (`+0x138`; the exact key, else the nearest one below), and
   `pass ctx->vt+8(block)` binds it. Later entries of the chunk apply it again only when the top 16 bits change. So **the first
   command of a chunk starts from the segment's block** (the pilot's blend 0x10 and depth 0x1E0 at the flask's draw); every other
   command inherits what the previous command of its chunk left (the model particles bind no blend of their own,
   ASHEN-FLASK-STUDY).
7. **What the native lacks**: it runs the whole pass in one execution (sorted by pass rank then key, `effect_draw.cpp` ~440) with
   one pending record reset once per `Execute`. To match the original: within pass 0x45/sub 0x29, cut the sorted segment into
   the same `k = min(10, ⌊n/5⌋)` slices and, at each slice start and at each change of the top 16 bits, reset the pending state
   to the segment's block (the state the original binds through `vt+8`). Other passes: reset at each top-16-bit change only.
   The worker assignment does not matter for the picture (each chunk is independent); the slice boundaries do.

## The functions (decompiled excerpts)

- `1400BF4C0` (entry allocation, the pilot's type-4 entries): `key = ((p124 & 0x7f | p120 << 7) << 6 | p128 & 0x3f) << 16 | p12C`;
  when the table holds `capacity` (64, `+0x78`) entries: `FUN_1401297e0(table, table + n·0x18)` (stable sort) then
  `FUN_140128240(owner, table, n)`, a new table of 0x600 bytes.
- `140128240`: under `owner+0x68`'s lock, push `{table, n}` into the vector `owner+0x88..+0x98`.
- `140128040`: sum the counts, copy every table into one array, `FUN_14012a4b0` (stable merge sort), store `owner+0xA8`
  (entries) and `+0xB0` (count).
- `140024740` (a scene pass's draw): `FUN_140128040(owner)`, then the executor `{vtable 0x143D521F8, owner+0xA8, owner+0xB0, ...}`
  and `FUN_14013ea60(&executor, pass ctx, [root+0x4E18] == 0, [root+0x4E19], [root+0x4E1C], [root+0x4E21])`.
- `14013EA60`: `for each entry: cVar5 = multithreaded ? executor->vt[0x18] (entry) : 0; on a change of cVar5 (entries with flag
  +0x11 set do not break a run): serial run → FUN_14013f030, parallel run → FUN_14013f120(..., threads)`, `threads =
  min([GXRS+0x2D0], the [root+0x4E1C] cap when non-zero)`.
- `14013F030`: `vt[0x10] (from, to, 1)` (one chunk per segment), then for each: `FUN_14013eeb0(chunk)` on the main context.
- `14013F120`: `vt[0x10] (from, to, threads)`, sort the new descriptors (`FUN_14013f550`), then for each: worker `= i % [GXRS+0x2D0]`
  when its rule `< 6`, else 0; a job `{start, end, count}` posted to that worker's queue (`[GXRS+0x2C8] + worker·0x530`).
- `140139560` (vt `+0x10`): the segment walk and the policy (table at `0x14473D5A0`; chunks `= fixed count` when the divisor is 0,
  else `min(⌈n/divisor⌉, max)`, then `min(·, n / min per chunk)`; split equally when `≥ 2` and more than one chunk is allowed).
- `14013EEB0` (a chunk's run): `state = {0}`; for each entry: `vt[0x28] (ctx, &state, key, flag11)` then the pass context's
  `+0x120..+0x12C` from the key, then the entry's commands through `vt[0x20]` (types 6, 7 and 8 handled apart).
- `140139980` (vt `+0x28`): `if (!flag11 && state.key != (key & 0xFFFF0000) && (block = FUN_1401284d0(owner, key & 0xFFFF0000))) {
  if (state.block != block) ctx->vt[8] (block); state.block = block; } state.key = key & 0xFFFF0000;`
- `1401284D0`: the owner's map `+0x138` (a red-black tree keyed by the top 16 bits, its lock `+0x158`): the exact block, else the
  nearest key below.

## Who registers the state blocks (added after the measurement slot, same evening)

- The commands of an entry run through the executor's slot `+0x20` (`140139A50`: types 6, 7 and 8 to `14013B110` / `14013B2A0`,
  every other type to `14013F470`). `14013F470`: type 1 sets the pass context's `+0x120` and `+0x124`; type 2 `FUN_14022ac60`;
  type 3 an object's `vt+0`; type 4 a function pointer with two arguments (the pilot's type-4 entries, `FUN_140299E40` and the
  like); **type 0xF8 calls `FUN_140128320(owner, current key, pass ctx)`**.
- `140128320`: under the map's lock, if no block is registered for `key & 0xFFFF0000`, **`block = pass ctx->vt+0x78(pass ctx,
  pass ctx[0x8A], allocator [0x144961B78])`** and insert it into the map (`FUN_1401299E0`); `owner+0x150` keeps the largest key
  registered. So a block is a **snapshot of the pass context's state** taken when the stream's 0xF8 command first runs for that
  top-16-bit key (INFERRED: `vt+0x78` captures, `vt+8` restores; neither body read). A chunk starting before that command has run
  for its key gets the nearest block below (`1401284D0`'s fallback).
- The executor's slot `+0x30` (`1401399F0`, called at the end of `14013EA60`): `FUN_140128430(owner, owner+0x150)` finds the
  block of the largest registered key and applies it through `vt+8` (the state left for what follows the pass).

## Who records the type-3 entries of scene pass 0x45, sub 0x29 (added 2026-10-30, 21:30, for the flask)

Read with GhidraMCP and the exports `decompiled/DS3-1152-F-` `00057A20`, `0005C0E0`, `00CFDE70`, `00D079C0`, `00D3DA20`; the
type argument of each call to `1400BF4C0` was found by a byte scan of `DarkSoulsIII.exe` (`mov dl, 3` before the call).

- **The type is `1400BF4C0`'s second argument** (`dl`, passed on to `140198680`, which writes it as the command byte); the third
  is the payload size. `1400BF600` is a small helper recording `{object, data0, data1}` as type 3 (one caller, `140D636F0`).
- **Every model row is a type-3 entry recorded by `14005C0E0`**, called only by the row recorder `140057A20` (and its wrapper
  `14005C060`). Per row of its list (`{sub, preparation, class mask, extra}`, 16 bytes each), for an ordinary row:
  `order = FUN_1400BF330(ctx, position, bias)`, then `0xFFFF - order` when `ctx[0x426 + sub] == 1`, then `ctx+0x128 = sub`,
  `ctx+0x12C = order`, and `entry = BF4C0(ctx, 3, 0x18)` = `{param_1 (the model-draw object, its vt+0 draws), bundle[1], the
  row's record}`. The reversal table holds 0x1F, 0x29, 0x2D, 0x31 and 0x34 (native `21_effects.inc` sets the same), so sub 0x29
  draws far to near.
- **The key** (`1400BF330`): `d = dot(position.xyzw, ctx+0x250..0x25C)` (the view-depth row of the pass context),
  `far = [[ctx+0x418]+0x58]`, `near = far × -0.1` (`143D7F0B0` is -0.1), `key = (d + bias - near) / (far - near) × 65535`,
  clamped to 0..65535 (0 when negative).
- **The position and bias come from `57A20`'s `param_6` and `param_7`**, which its three callers set (all through `140059040` or
  `140059130`):

| Caller | What it draws | position (`57A20` param_6) | bias (`57A20` param_7) |
|---|---|---|---|
| `140CFDE70` | each part of an assembled model: the player, ghosts, NPCs with parts (and what they hold: INFERRED for the flask, a part slot) | centre of the part's world bounds, `(min + max) × 0.5` from `[part+0x18]->vt+0x18` | `|(max - min) × 0.5|` (the half diagonal) `+ [[part+0x18]+0xAF0]` |
| `140D079C0` | a world `SprjModelDrawEntity` (map pieces, objects, single-model characters) | centre of its world bounds (from `entity->vt+0x18`) | half the bounds' depth extent in view space (`FUN_14009E840` with the pass's matrix) `+ [entity+0xBA0]` |
| `140D3DA20` | a menu item (`SprjFD4ModelItem`, the menus' models) | a stack vector (`fStack_c8..bc`), not read further | a stack float (`fStack_70`), not read further |

- Inside `57A20` the bias moves per row: a split model (`desc+0x10 != 0`) records **two** 0x29 rows, preparation 1 with
  class 2 at `bias + 0.5` and class 0x12 at `bias - 0.5` (corrected by tranche 5's final review: not preparation 2) (`143D7EDB4` is 0.5); the engine7 rows (sub 7) use `bias - 100`
  (`143D7EFD0`); the ordinary list (`uStack_348`: 0x29 class 0x12, the 0x10 row, the 0x1F and 0x04 rows) uses `bias` itself.
- **What precedes the flask in its chunk**, by this: any type-3 model row whose key is above the flask's (far to near): first the
  other parts of the same character, which all take the character's bounds and `+0xAF0` (CFDE70 reads `[part+0x18]`, the
  character), so they share one key (±0.5 for a split model; corrected by tranche 5's final review), then the map's forward rows
  behind it.
  The pilot's lead (the character's forward parts, hair and translucent pieces, at the flask's distance) fits: they are
  recorded by the same function with keys a few units apart. INFERRED until the pilot's run names the entries.
- The other `BF4C0` type-3 sites (`1400A6470`, `1400CA6D0`, `1400DD440`, `1400E3CF0`, `14013BE60`, `14019B690`, `1401B1960`,
  `1401B6020`, `1401B6980`, `1401B8CA0`, `1401B9670`, `1401DCF20`, `140202F00`, and `14024E790`'s type-4 sibling) were not read:
  the scan is a heuristic (the nearest `mov dl` before the call), and their passes are not known. Read them only if the pilot's
  entries name an object that is not a model row.

## Open

- What the pass context's `vt+0x78` captures and `vt+8` restores (blend, depth, raster, targets, constant buffers?): the next
  read if the native must reproduce the block exactly (its class is the pass context's, `[passCtx]`'s vtable).
- The sort of the chunk descriptors before dispatch (`14013F550`) and the « type » field (`+0x3C`, 13 here) are not read: they
  change which worker runs a chunk, not the chunk's content.
- `[root+0x4E20]` and `[[ctx+0x450]+0x21F3]` are runtime flags: the pilot's run saw the chunks on several threads, so both were
  on there (OBSERVED live by the pilot).
