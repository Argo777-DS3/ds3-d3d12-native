# I-0104 tranche 1: how Elden Ring takes its effects to D3D12

2026-10-28, the pilot (Opus 5.5), static. Elden Ring 2.7.1 (`eldenring_2.7.1.0_25080141_1A354710.exe`) read with headless
Ghidra 12.1.3, read-only, on the copies `er-mirror-i0070-a`, `-i0072-a`, `-i0072-b` ([Invoke-ErGhidra.ps1](Invoke-ErGhidra.ps1));
DS3 1.15.2 on the mirror `ghidra-mirror-i0066`. Script: [I0104Vt.java](I0104Vt.java) (a vtable's slots, then each slot or
named function decompiled). Outputs under `evidence/I-0104/er/` and `evidence/I-0104/ds3/` (the RTTI inventory `classes.tsv`
by I-0067's `I0067Rtti.java`). Everything below was read by the pilot in those decompiled outputs (no worker); ER is a
reference for the architecture only, never evidence of DS3's behaviour (D-0195).

## 0. Answer first

- **ER keeps DS3's effect system and changes the layer under it.** The FXR library, the scene control entity
  (`TGXSgEntity<GXFfxSceneCtrl>`), the per-node draw callback recorded into the pass's sorted command stream, the renderers that
  evaluate the FXR properties and write their vertices into a ring, and a draw context that holds CPU images of the constant
  blocks: all are DS3's shape. ER rewrote the renderers as « appearances » (RTTI `FXSingleAppearanceBillboard`,
  `GXFfxAppearanceBillboardEx`, `...PointSprite`, `...Line`, `...QuadLine`, `...Tracer`, `...RadialBlur`, `...Distortion`,
  `...Model`, `GXVfxStandardParticleAppearance`), one per DS3 action kind.
- **Under it, the GX device context is D3D12** and effects are not special there: the draw context writes its state into the
  device context's fields with a dirty flag; the draw asks the state tracker for a pipeline built from pooled ids (I-0072),
  flushes the root bindings, and draws on the D3D12 command list. Constant blocks are copied from the draw context's CPU image
  into frame-transient upload memory at the bind; vertices and indices come from a transient ring and are bound on the D3D12
  list directly.
- **Effects run at their sorted place.** The callback runs when the pass's sorted stream plays, interleaved by key with the
  other rows of that pass; the key comes from the sort layer the entity picks, its position and a depth bias.
- **Distortion**: ER added a draw-context slot that copies the scene region under each distortion draw just before it (with a
  16-pixel margin), so a distortion sees everything drawn before it, effects included. DS3 has no such slot: its draws sample the
  pass's frame copy.
- **Hypotheses (CONTRACT)**: H1 confirmed for the classic billboards (the family read through); H2 refuted, ER has no typed
  effect packet between its renderers and D3D12. H3: ER sorts its effects with the other rows of the pass (confirmed); that DS3's
  original does the same is FFX-DRAW's reading, not re-read here.

## 1. The chain, with addresses (ER 2.7.1)

| Step | ER | DS3 1.15.2 (the same step) |
|---|---|---|
| Scene control draw, per pass | `GXFfxSceneCtrl` slot 14 `141CA3B90`: for a pass id `[ctx+0x114]` in 2..0x2C, the library draw `141C9C0C0` over the effects with a functor (`141CDB760`); pass 0x38 records extra commands | `24E790`, called for GX pass 0x45, records every engine pass at once (FFX-DRAW §2) |
| Entity offered to the pass | `GXFfxDrawPass` slot 4 `141CDB7E0`: culls, picks the sort layer (0x24, 0x27, 0x2B, 0x2F, 0x32, 0x3C by the entity's kind), allocates a 0x110-byte data block from the pass context (`141AD1FC0`), sets the key (`141AD3690(ctx, layer, position, bias)`), records the callback `141CDC210` with the entity and the block (`141AD35B0`, commit `141AD3C30`) | slot 4 `2997E0`; command `BF4C0`, key `ctx+0x12C`, commit `BF2D0`; callback `299E40` |
| Callback, at playback | `141CDC210`: builds the draw context on the stack (`141D361A0`, class `GXFfxDrawContext@I`, 0xB70 bytes), fills view, fog, light and colour data from the pass context and the block, calls the node's draw (`entity vt+8`) | `299E40`: builds `GXFfxDrawContext` (`FXCG2DrawContext` base), calls the node's renderer slot 8 |
| Renderer, CPU side | the cluster adapter's slot 7 (`141D0F3D0` for billboards) evaluates the FXR properties per particle (the evaluator tables `14484CBE0`, `14484D2D0`) and writes 0x48-byte particle records and an index list into a ring mapped through `[14484CBD8]+0x28` (vt+0x48 map, vt+0x68 unmap) | the builders (`385F70`, `396F30`, `39E900`, ...) write vertices into the ring `sc+0xD0` |
| Renderer, draw | `GXFfxAppearanceBillboardEx` slot 8 `141D0A090`: fills the draw context, allocates the vertex range (`141D47F70` → `141E9DC40` → `141E8F770`, the device's transient allocator), then `141D48070`: topology cached at `dc+0x268` (`141E8DA20`), `IASetVertexBuffers` (`141E8EC50`) and `IASetIndexBuffer` R16_UINT (`141E8EBC0`) on the D3D12 list, then the device context's draw `141ACAF20` | the same builders; GX statements through the D3D11 wrappers `19202xx-1921xxx` |
| Per-draw setup | draw context slot 26 `141D36C30` (DS3's slot 27 `264230`): evaluates the scene parameters into the CPU images, then writes the device context directly: blend, depth and raster state handles from the global bank `[[1447F33E0]+0x5A8]` (`141D39410`: by blend type and alpha-to-coverage), samplers, textures t0..t7 (`141D3D110`: null slots get the default texture), and the constant blocks | slot 27 `264230`, through the GX wrappers (state bank `[display+0x2A0]`, default texture `[display+0x270]`) |
| Constant blocks | `141ACF120(cb, device context, image, size)`: allocates transient upload memory on the list (`141E8EB70`) and copies the image: VS b0 0x290 bytes from `dc+0x130`, PS b0 0x340 from `dc+0x610`, DS b0 0x50 from `dc+0x950`, HS/GS 0x250 from `dc+0x3C0`; the slot's handle stored and the root binding marked (`141ACD210(tracker, stage, slot, 1, cb)`) | `UpdateSubresource` (`1920300`) of the same images into GX constant buffers |
| Draw | `GXBS::GXDeviceContext` draw `141ACAF20`: the state tracker `[dc+0x25C8]` (`1419E3700`) gives the pipeline when the state is dirty (the cache of I-0072), the root bindings `[dc+0x25D0]` are flushed (`141ACD170`), then `141E8E0D0` draws on the D3D12 list; dispatch `141ACAD20` the same for compute | `Draw`/`DrawIndexed` wrappers `1921370`, `1921390` |
| Distortion copy | draw context slot 25 `141D3E5B0` (new in ER): when the draw asked for the scene (`dc+0xAA0`, set by slot 8 `141D3CCE0` with a null texture), the screen rectangle of the draw plus 16 pixels is copied from the scene into the copy texture (`1419F3320`, a state-saving copy draw) | none (DS3's slot 25 `140263370` is ER's slot 24 `141D392F0`); the pass context's frame copy `+0x4C0` |

The GPU particles (`GXVfxStandardParticleAppearance@I`, vtable `14305D7C0`, draw slot 8 `141D01230`) were read to their calls only:
their update and draw go through the same device context (dispatch `141ACAD20`); their buffers and barriers are not read yet (DESIGN §2, with tranche 3).

## 2. What ER does that the native does not (the adaptation for DS3)

The native has the same two halves (the engine's draw side runs, a D3D12 encoder draws), with scaffolding between them that ER
does not have:

1. **Capture then translate, instead of a device context.** The native hooks DS3's D3D11 wrappers, hands the renderers token
   objects whose vtables trap, captures each GX statement into a fat record (eleven constant byte vectors per draw), then
   `BuildNativeEffectFrame` translates tokens into programmes, layouts, bank offsets and views a second time, and the stage keys
   its pipelines by a formatted string. ER's device context resolves each binding once, into the state it draws with.
2. **Out of order.** The native runs every effect callback on the engine thread in its own loop and the stage draws them after
   all the forward rows of 0x29; ER (and DS3's original, FFX-DRAW) draws each at its key among the pass's rows.
3. **Copies.** Constants and vertices are copied into the record, the ring arena copied into the frame, then uploaded; ER copies
   each constant image once, into upload memory, at the bind.
4. **One scene copy per frame for the passes after 0x29.** DS3's original binds the pass's frame copy; ER copies the region under
   each distortion draw. The native copies before the first sampling pass only, so 0x31 does not see 0x2D's output (CENSUS §5).

## 3. ER's defects, not copied

- A pipeline missing from its cache is created synchronously on the recording thread (I-0072): the native keeps its pipeline
  cache and records the effect pipelines with the others.
- The effect update's barriers polled with `Sleep(0)` (I-0086 §4): not part of this line.
- The region copy costs a state save, a copy draw and a restore per distortion draw; adopting it would also change DS3's look
  (DS3 never sampled its own effects in a distortion): kept as DS3's frame copy, the copy point corrected (§2 point 4).

## 4. Open

- ER's GPU particle buffers, counters and barriers (not read; with tranche 3, DESIGN §2).
- Whether ER's effect passes keep the depth writable (the native's depth-writing effect draws test without writing).
- The keys of DS3's forward rows natively (the merge of point 2 needs them): DS3 side, read for the design.
