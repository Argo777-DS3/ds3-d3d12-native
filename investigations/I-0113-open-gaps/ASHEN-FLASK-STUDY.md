# I-0113 gap 5, the Ashen Estus Flask drawn see-through: static study

Study thread, 2026-10-30 (Paris, evening), for the I-0113 pilot (its question: which blend and depth the original applies to a
model particle's draw in `636F0 → 63CB0 → 64610`, and where the native's « pending states » come from). Read-only: GhidraMCP on
Thomas's Ghidra (DarkSoulsIII.exe 1.15.2), a pseudo-disassembler listing ([I0111LinearListing.java](../I-0111-dlss-reflex/tools/I0111LinearListing.java)) of two small bodies the project has no function for
(headless on the mirror `ghidra-mirror-i0068`, `-readOnly -noanalysis`), `native/frame/effect_draw.cpp` and
`13b_prepare_model_frame.inc`, the I-0068 note [EFFECTS-E03-MODEL-PARTICLES-NATIVE](../I-0068-static-yebis-ffx-lights-spec/EFFECTS-E03-MODEL-PARTICLES-NATIVE.md)
and I-0101's STUDY (the material classes). No run. Labels: OBSERVED (decompiled or listed here), INFERRED.

## Answer in short

1. **Blend: nothing on the model's path binds one.** From `636F0`'s entry to the draw call, the original runs the mesh loop
   `63CB0`, the mesh draw `64610`, the effect adapter's table (`{143D42000, 14473F6C0}`, its draw `1A9640`, its « set
   material » a bare `RET`), the material's `Begin` (`GXFLV::GXFlverMaterial` slot 2, `140140680`), the shader binds
   (`140142430`: PS, VS, HS, DS, GS only), the MTD's constant producers (`14002E5C0`: constant buffers only) and the raster and
   depth commit (`19C590`): **none calls the context's blend wrapper** (`141920ED0`, nor its setter `140015220`). So the blend
   of a model particle's draw is **the one the deferred context holds at `636F0`'s entry**: whatever the FFX commands drawn
   before it on that context left. The argument commit `1A86C0`, which the native short-circuits, can bind a blend
   (`[[GXRS+0x4DD0]+0x2F0 + 8·i]` with `i = args+0x2C4`), but `29ADF0`, the E03 path, sets that byte to −1 before the commit, so
   on this path it binds none (it stores `args+0x204` = the FFX command's `+0x30` byte instead; its reader is not found).
2. **Depth: the material rebinds it at every draw.** `140140680` always writes a depth-stencil state into the argument
   block's state record (`args+0x100`: `+0x18` the state, `+0x20` the stencil reference, dirty bit 2), which `64610`'s commit
   `19C590` binds through the context's depth wrapper (`141920F00`) right before the draw. Which state: for a material whose
   class (`material+0x18`, from `142130`) has none of the bits `0x10D` (or with `args+0x200` bit 17), and `args+0x18C` bit 10
   clear: `[[0x144961AB0]+0x2A0]+0x1E0` when `args+0x188 == 1` (or non-zero and the MTD's `+0x81` clear), else `+0x1E8`;
   otherwise a forward pair `[[0x144961B50]+0x4DE8]+0x868` / `+0x860` (by the MTD's `+0x111` and `args+0x188`) with a stencil
   reference built from `args+0x18C` bit 2, the MTD's `+0x80`, `args+0x1C == 6`, `material+0x24` bits 7 and 11. So the native's
   use of the context's pending depth for a model particle is not the original's rule; the material decides. (The value the
   native names `0x1E8` may well be this same table entry: then the depth is the same, tested and not written; not checked.)
3. **Raster**: `64610` sets the record's cull bits per face set (the table at `0x14473C9F8`, as the native already reads,
   `content.h:22`) and its bit 3 from the material flags; committed by `19C590` through `141920CA0`.
4. **Consequence for the see-through Ashen flask** (INFERRED): since the model particle inherits the blend, the result depends on
   the **order** of the FFX commands on the context before it. The native resets its pending record once per `Execute`
   (`effect_draw.cpp` ~437) and sorts the commands by pass then key (`~440-443`): if the original's command just before the
   Ashen flask in pass 0x29 bound an opaque (or no) blend and the native's sorted predecessor an additive one (`0x20`), the
   native flask turns additive while the original's stays opaque. In pass 0x21 the flask is the first model drawn natively
   (nothing pending, the row's opaque state), as in the tranche-3 fix.

## What the native does today (OBSERVED, for the record)

- `ModelDraw` (the `636F0` hook, `effect_draw.cpp:302-317`) captures `pending.blend` and `pending.depth` (the context's bound
  tokens) into the submission, then `13b_prepare_model_frame.inc:385-394` draws the particle with them when present
  (« the original's 1A86C0 binds none of its own; i105-bolt-ref-11 at 636F0's entry: additive 0x20, depth tested unwritten
  0x1E8 »). Point 1 confirms the blend half of that rule; point 2 says the depth half should come from the material instead.

## What would decide it (live, the pilot's)

1. **The original's blend at `636F0`'s entry for `s07002`** in pass 0x29 (the Ashen flask) and 0x21 (the orange one): the same
   Reference observer as `i105-bolt-ref-11` (the context's bound blend and depth tokens at the entry, with the model's name). If
   the Ashen flask's blend there is opaque, the native's command order or replayed state before it differs (point 4): compare
   the original's command sequence in 0x29 just before it with the native's (`ffx-record.flag` on both sides). If it is additive
   there too, something on the path binds a blend after all (none found here: the MTD's producers in `14002E5C0` are the least
   read).
2. **The depth at the draw**: the original's bound depth after the material `Begin` (the observer at `19C590`'s depth call, or
   at the draw wrapper) against the native's `0x1E8`.

## The path, step by step (OBSERVED)

| Step | Function | States it binds |
|---|---|---|
| 1 | `29ADF0` (FFX model renderer slot +0x20): a 400-byte draw state from `4DF20`, its `+0x84` set to −1 and `+0x85` = the command's `+0x30`; the argument block `1A8580(…, passCtx+0x750, passCtx+0x460, state)` | none |
| 2 | `59470` (scene constants), `1A86C0` (the arguments' commit) | `1A86C0`: a blend only when `args+0x2C4 != −1` (never here); VS b11 and PS b7-type constants; it leaves `args+0x100` with no depth state and dirty bit 2 |
| 3 | `2BA3A0` (bone matrices) → the adapter `2C0130` → `636F0(&{143D42000, 14473F6C0}, model, args, bones)` | none |
| 4 | `636F0` → `63CB0` (the mesh loop; the material table `args+0x1A8` or `model+0x1F0`) | per material: `Begin` (step 5); its callbacks `args+0x1B8`/`+0x1C0` (none set by `1A8580`, INFERRED) |
| 5 | `140140680`, `GXFlverMaterial::Begin`: the programme (`[mat+0x70]` or `[mat+0x78]` vt+0x10), `140142430` (shaders), `14002E5C0` (MTD constants), `22D620`, `2302C0` (GX04 tessellation) | shaders, constant buffers; **the depth state into `args+0x100`** |
| 6 | `64610`: the raster bits per face set, `64340` (bone buffers), the table's `+8` (`RET`), `19C590` (commit), the table's draw `1A9640` (index buffer, topology, `DrawIndexed` through `0xC6020`) | raster, depth (from step 5), then the draw |

The param-8 table of `636F0` (`143D42010`: `+0` and `+8` a bare `RET`, `+0x10` `xor eax,eax; ret`) and the adapter table's
`+0x10`/`+0x18` (`xor al,al; ret`) bind nothing (byte reads at `0x140063660` and `0x140057320`).
