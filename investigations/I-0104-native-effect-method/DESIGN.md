# I-0104: the native effect base (design)

2026-10-28, the pilot (Opus 5.5). From [ER-EFFECTS-STUDY](ER-EFFECTS-STUDY.md) (D-0195) and I-0103's
[CENSUS](../I-0103-effects-census/CENSUS.md) §5, §7, §8. Native tree at `main` `2e7b10f7`.

## 1. The method, in one paragraph

Elden Ring keeps DS3's effect system whole and puts a D3D12 device context under it; its effects are not special at the D3D12
boundary. The native does the same: the engine's effect side stays the engine's (the update, the recording into the sorted pass
stream, the callbacks, the renderers that evaluate the FXR properties and fill the ring, slot 27 that fills the constant images),
and the native implements the context those renderers draw through. What D-0190 removes is the **replay**: today the native
captures D3D11-shaped statements on stand-in objects, then translates them a second time on the engine thread, then once more in
the encoder. In the base, each statement lands in the native effect context in its final form (the programme, the layout and the
bank states resolved once at the bind, the constant images and the vertices written once into the frame's arena), the draw is a
finished native record, and the encoder draws it at its key among the pass's rows. The contract between engine and backend is that
record: produced by the engine's own draw side, owned and drawn by the backend (programmes, pipelines, upload memory, descriptors,
the GPU particles' buffers, counters and barriers).

## 2. What changes, by tranche

### Tranche 2: the base for the classic draws (E01 603, 604, 600 and, on the same path, 602, 607, 608, 601)

The classic families differ only in their builder and programmes, which the engine keeps; they all draw through slot 27 and the
ring. One path serves them all, so they move together; E01 (79 % of the drawing occurrences) is the one witnessed first.

1. **The native effect context** (`frame/effect_draw`): the wrappers' detours write a typed state: programmes (`NativeEffectProgram`,
   resolved from the programme token at the bind), the input layout (decoded once per element table), the bank offsets, the
   samplers, the views (resolved at the bind through the engine's resolver: an FFX texture's TextureIngress record and view, the
   frame copy, the depth, an effect record's view). The constant buffers: `UpdateSubresource` on an effect buffer writes its bytes
   **into the frame's arena** (256-aligned) and the buffer remembers that range; a draw binds ranges, never copies of bytes.
2. **The ring is the frame's arena**: the ring's map returns arena memory; the frame owns it and moves it to the renderer (no copy
   of a 2 MB arena at the end).
3. **The draw is final at `Draw`/`DrawIndexed`**: a `NativeEffectDraw` with its arena ranges and resolved bindings, appended to the
   frame. `BuildNativeEffectFrame`'s second translation of the classic draws is removed; a draw missing a programme, a layout or a
   state is dropped where it is met and counted.
4. **Encoder** (`renderer/effect_stage`): the arena uploaded once a frame; the constant blocks bound as root views at
   `arena + offset` (no upload per draw); the pipeline key a fixed struct, hashed (no formatted string).
5. **Witnesses behind their flag**: `effect_draw`'s string-keyed `calls`, `bound` and `draws` counters and the trap counts run only
   with `ffx-record.flag` (CENSUS §7 point 2).
6. **The scene renderer's dead stage observer removed** (`SetObserver`, `SetPresentationObserver` and their `Stage` calls; no caller
   since I-0102 tranche 1, CENSUS §8).

The E07 statements (compute, copy, clear, indirect draw) keep their path in this tranche, with their constants in the arena too.

### Tranche 3: the order and the copies (the E01 correctness points, CENSUS §5)

- **The key order with the forward rows**: DS3 records its model rows and its effects into the same 0x29 pass context, each with a
  key from `1400BF330(passCtx, position, bias)` (reversed back to front for 0x1F, 0x29, 0x2D, 0x31, 0x34), and plays them sorted.
  Natively the forward rows are made by the native's own transcription of `57A20` in walk order, without a key: the base gives each
  forward row the key `BF330` gives it, and the encoder draws 0x29 as one merge of rows and effects by key. Read so far (DS3,
  `evidence/I-0104/ds3/key.txt`, `57a20-refs.txt`, `59040-refs.txt`): `BF330(ctx, position, bias)` = `(dot(ctx+0x250 depth row,
  (position, 1)) - f·c + bias) / (f - f·c) · 65535`, clamped to 0..0xFFFF (`f` the camera's far plane `[ctx+0x418]+0x58`, `c` the
  constant `143D7F0B0`), then `0xFFFF - key` where the pass's reversal byte `ctx+0x426+pass` is 1 (`BF210`). The row recorder
  `14005C0E0` gets them from 57A20's 6th and 7th arguments (forward row 0x29: bias + `143D7EDB4`), which its callers pass from the
  drawn entity: for a character part (`CFDE70` → `59040`) the world box's centre and its half diagonal plus the model's `+0xAF0`;
  for a map piece (`D079C0` → `59040`) the box's centre and half the third extent of its transformed box (`9E840`) plus the
  piece's `+0x174`; for a draw entity (`D3DA20` → `59130`) the centre and half the third extent of its world box (`286010`). The
  transforms take the pass context's `+0x10` matrix (the view), so « the third extent » is the box's half depth along the view.
  Checked at the character part's call in the disassembly (`140CFE440`..`4A5`, `59040` at `140059040`: 57A20's 6th argument is
  `59040`'s 10th, the box centre, its 7th the 11th, the float bias); the map piece and draw entity calls read in the decompiler only.
  Constants (Thomas's DS3 copy, read-only): `c` = -0.001 (`143D7F0B0`), scale 65535 (`143D7F078`; `BF3F0` 65534), the row offsets
  0.5 (`143D7EDB4`) and 100 (`143D7EFD0`). 57A20 records the ordinary 0x29 and 0x1F rows with the bias as given; a model with
  `+0x10` set gets two 0x29 rows instead, at bias + 0.5 and bias - 0.5.
- **What the merge costs the renderer**: the 0x29 rows and the effects then alternate, so each switch re-sets the root signature
  and heaps and moves the depth between DEPTH_WRITE (rows) and DEPTH_READ (effects that sample it), each declared to the frame
  graph. The effect pass 0x21 (phantoms), captured today but drawn in no pass, joins the 0x21 rows the same way.
- **The depth-writing draws** (+0x1E0, blend type 0): whether the original's effect pass binds the depth writable while it samples
  it; then the native follows it (today they test without writing).
- **The scene copy per sampling pass**: 0x31 sees 0x2D's output if the original copies again before 0x31 (to read: the pass
  context's frame copy `+0x4C0` and who refreshes it).

### Tranche 4: the GPU particles (10000) and the leftovers

- E07's statements into the context as the classic ones (done in tranche 2: emitted at once, constants in the arena). Not done,
  moved with tranche 3: the bindings resolved once at the bind (classic and E07), and ER's GPU particle path read (its buffers,
  counters and barriers, ER-EFFECTS-STUDY §4).
- The producers' capture layer (CENSUS §8): `world/effect_producers` removed (done); `world/light_producers` kept as it is (its AO
  families are `ao_constants_test`'s oracle; the rest of its capture layer has no caller, left for a code cleanup, RESULT).

## 3. What line 5 then writes in the base

- **10012 and 10003**: their renderers draw through the same GX wrappers (the census's classes `3D660B8`, `3D65A88`): allowing
  their class and covering any statement the context does not yet take.
- **606** (`3D7E380`): draws through the draw context's slots (engine code already running natively) and the GX wrappers, like
  10012: allowing its class (I-0105 tranche 2 read it; the earlier « pass context's slots, the context gains them » was wrong).
- **605 E03**: FLVER models, through the native model renderer (EFFECTS-E03), not this context.

## 4. Proof

No live run before a merge (D-0227). Each tranche: the unit tests for its pure parts (the arena ranges, the pipeline key, the merge
order), clangd, the build and its unit tests, `Test-Project`, a step review and a fresh Opus at xhigh on the final tree; after the
merge, the launcher's rebuild and the one-minute start check, which reaches the Firelink world and its bonfire (E01, E07).
