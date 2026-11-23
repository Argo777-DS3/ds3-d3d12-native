# R8b prerequisite: the engine reads of `PrepareNativeModelFrame`, who writes them, and an engine-side snapshot (static study)

STATIC (Ghidra mirror of `DarkSoulsIII.exe` 1.15.2.0 and `native/frame/game_bootstrap.cpp` at main `642e3993` plus this branch). Nothing was launched.
Status words: **STATIC** read in code or data, **INFERRED** deduced, **OPEN** not known statically.
Asked by the coordinator after [D3D12-PHASES-D-G-PREP.md](D3D12-PHASES-D-G-PREP.md) section 0 (fact 2) and section 2.8: R8b "needs a static study of its own". Method: ER (D-0157), read-only reference `ER-ENGINE-D3D12-METHOD-710.md` and `ER-ENGINE-D3D12-DEEP-STUDY-716.md`; no ER value is used.
Evidence outside Git: `evidence/I-0066/queries/r8b/` (call trees, reference and scan results, the task-step table, the draw-entity vtable). Exports: `decompiled/DS3-1152-F-<RVA>` for the functions listed at the end.
A first version of this note was reviewed independently; the review corrected the wind writer of the draw-entity family, the per-update key writer, the render-side claim and the call-tree table (all fixed below).

## 0. Verdict

1. **`PrepareNativeModelFrame` reads live engine memory at eight places (E1 to E8, 11 `Read` calls since E1 is four), behind three pointers stored in the selected model**: `model.group` (the 400-byte draw state), `model.animation` (animation object: wind block and flags) and `mesh.current.material` (a diagnostic read); E8 is a `.data` row. Section 4 lists further engine touches in its call tree that matter for a thread move.
2. **Seven of the eight can be copied at collect time with the same value, if nothing writes them between collect and prepare.** Every writer found is update-side engine code (the model dirty push `FUN_140C4AF10`, the draw-entity update `FUN_140D07710`, the item update `FUN_140D3E950`, the holder setters, the time-blended key chain `FUN_140D3FF50`). The render-side emit `FUN_140D079C0` **does write the live group** (`+0x98` bit 0 and the dirty byte `+0x180`) when it runs unfaded, but the native build never runs it (`FUN_140D55190` is replaced). So the argument is "no engine writer runs between collect and prepare in the native build", not "the render side only copies". This is STATIC for the writers found and INFERRED for the absence of others; **row R8B-1 settles it** with one hash per model.
3. **Collect and prepare run on the same owner thread within one frame (INFERRED, section 1)**, so today nothing interleaves; the hazard is the plan to move prepare to another thread (R13), where the next frame's update tasks overwrite the group and the wind block **in place** (no copy, no version) while the render thread reads them. Parallel worker writers (per-character steps) are not excluded by being on the same thread as collect.
4. **Proposal (section 5):** a value snapshot in `NativeSelectedModel` taken at the existing collect point (Level 1), then engine-side producers for the always-changing fields (Level 2, an adaptation of the ER method), guarded by the permanent full-scan comparison F6. Level 1 makes the eight reads value reads; **it is not enough to let prepare leave the owner thread**: the call tree of prepare also reads engine programme repositories, calls engine functions and falls back to engine gparam lookups (section 4).

## 1. Where the reads happen (STATIC unless marked, `game_bootstrap.cpp`)

- `PrepareNativeModelFrame` (`:4734`) is called from `Clear()` (`:625`; `Clear()` is also called at bootstrap `:8950`), which runs from the `EndFrame` hook (`:7318`, engine function `FUN_140D56930` reached through `SprjDrawStep` slot `+0xB0` `FUN_140D553C0`), and from `RenderNativeOffscreenViews` (`:6434`), which `Clear()` calls at `:627` right after the main prepare: **the menu offscreen preparation is on the same EndFrame thread**; `native_menu_thread` (`:6138`) only gates the fallback offscreen collect at `:6421`, and offscreen scenes are otherwise collected at `:5508`. It builds a `NativeModelFrameInput` from `NativeSelectedScene`, an immutable `shared_ptr<const>` produced at collect.
- Collect is `CollectNativeSelectedScene` (`:4586`), called at `:5507` from the native replacement of `FUN_140E006A0` (`UpdateNativeSceneCpu`, `:5381`), reached by `FUN_140DFDC50` under `FUN_140D54F80` (`call-trees-d3e950-e006a0.txt`). `FUN_140D54F80` has no recovered caller (only a `.pdata` entry) and is not a `SprjDrawStep` vtable slot, so **its order relative to the EndFrame slot, and its thread, are INFERRED** (the fade chain `D54F80 -> DFDC50` runs each frame, FADE-PLATES-NATIVE); `UpdateNativeSceneCpu` itself has no `owner_thread` check (`:6069`, `:7251` and `:7321` are BeginFrame, the render step and EndFrame). Row R8B-2 settles both by comparing the scene serial with the presentation serial and the thread ids.
- The three pointers come from the selection candidates (`:4157`, `:4237`, `:4292`):

| Family | `group` | `animation` | Notes |
|---|---|---|---|
| Map piece (`SprjModelDispEntity` children, `SprjFD4ModelItem`, vtable `2951FF8`) | `entity+0x1F0` | `item+0x70` | wind through the item update |
| Character part (`ChrAsm` parts, vtable `2947DB8`) | `entity+0x240` | `entity+0x3D0` | draw-entity layout |
| Other draw entity (`SprjModelDrawEntity`, vtable `142948EF8`) | `entity+0x240` | `entity+0x3D0` | wind through `FUN_140D07710` |

## 2. The eight reads (STATIC at the native site; the engine side in section 3)

| # | Native site | Engine source | Used for | Production class |
|---|---|---|---|---|
| E1 | `:4946-4947` (four reads) | `material+0xD0` +`0x1A` (1 B), `+0x3B8` (4 B), `+0x378`, `+0x380` (8 B each) | **log row only** (`native_selected_matdyn_unavailable`), in the branch taken when the dynamic-material block is not ready | admission-time constant; drop or capture at mesh admission |
| E2 | `:5001` | `group+0x50` (16 B) | `cbvModelParam +0x60` | state field, default (1,1,1,1) |
| E3 | `:5002` | `group+0x60` (16 B) | `cbvModelParam +0x4B0` (`VC_ModelSizeParam`, read as `1/sqrt(y)` by the grass programmes, FOLIAGE-NATIVE) | state field, default (1,1,1,1) |
| E4 | `:5009` | `animation+0x620` (1 B) | wind block valid flag | written per update |
| E5 | `:5010` | `animation+0x630` (1 B) | wind-interface flag (`+0x4C0..+0x4EF` zero only when clear) | written once at construction (items) |
| E6 | `:5019` | `animation+0x210`, 0x410 B | `cbvModelParam +0x80`, the wind block (up to 8 area and 8 point winds) | **written every update** where a wind node exists |
| E7 | `:5047` | `group+0x0..0x18F` (400 B) | pixel `cbModelParam` (`ProduceModelParamPs`) | state, written in place by many setters, part of it **every update** (section 3) |
| E8 | `:5047` | `image+0x4929B20` (16 B) | default row of `ProduceModelParamPs` | `.data` row **initialised at runtime** |

**E8 is not a link-time constant.** The image bytes at `144929B20` are zero (the `x:` dump in `default-row-4929b20-refs.txt`); the row is filled at runtime (`FUN_1400613B0` and others copy or read it; 109 functions reference the 16-byte range, none with a store reference). Cache it once after the engine's initialisation, never at module load; row R8B-4 compares two reads.

## 3. Who writes each one, when, on which thread, for how long

**Frame timeline (`fd4-steps-copy.txt`, MENUS-MAP section 3).** The engine runs FD4 task steps; the third column of the step table is the step's group (`g1` for most steps, `g63` for the per-character parallel steps `0x40` `ChrIns_UpdateDraw` and `0x41` `_UpdateDraw_End`, MENUS-MAP line 6404; `0x3F` is `g1`). "In id order" is INFERRED (MENUS-MAP). The update side is `0x00` to `0x4F`
(`CameraStep 0x43`, `UpdateLodWorld 0x4F`), `UpdateLodIns 0x50` (`g14`), `GraphicsStep 0x51`, `DrawStep 0x58` with slot `+0xB0` EndFrame and slot `+0xC0` render `FUN_140D55190` (`drawstep-vtable-slots.txt`); the names `DrawBegin`, `GameSceneDraw`, `Flip`, `DelayDeleteStep` (`0x5E`) and `FrameEnd` come from MENUS-MAP section 28, which counts by table position (the step table file leaves several of these ids unnamed).

**E4, E6 (wind) for map items (STATIC writers, OPEN step).** `FUN_140D3E950` (the item update) builds the force object from the wind node and calls its slot 0 `FUN_140D07E20` (not a defined function in the mirror; created with `C:` it writes the 0x410 block), which writes `item+0x280` (= `animation+0x210`); the same function stores `1` at `item+0x690` (= `animation+0x620`, `140D3EAFF`). In the item region (`140D3A000-140D41000`) these are the only stores to `+0x690`; the only stores to `+0x698` (the interface object) and `+0x6A0` (= `animation+0x630`, E5) are in the item constructor `FUN_140D3C4D0` (`140D3CABE`, `140D3CAC5`): **once**. The constructor clears block and flags (`FUN_1400613B0`).
`FUN_140D3E950` also calls `FUN_1400616E0(item+0x70, dt)` every update, which writes `animation+0x130..+0x171` (read by collect, `:2504`). It has two callers: `FUN_140C4D330` (the shared slot 1 of the five `*ModelIns` vtables: a task execute that forwards `this+0x10`) and `FUN_140D3D280` (item slot 10: first-use update, flag `item+0x6F0`). The step and worker that run the `ModelIns` task are **OPEN**. Lifetime: the block is overwritten in place at each update; without a wind node it stays zero and the flag zero.

**E4, E6 (wind) for the draw-entity family (STATIC; corrects the first version).** With `animation = entity+0x3D0`, the flag E4 is `entity+0x9F0`, E5 is `entity+0xA00` and the block `entity+0x5E0`. `FUN_140D07710` (vtable `142948EF8` slot `+0xC8`) is gated by `entity+0xA78 != -1` (a handle created in the constructor at `140D06288`), calls the force vtable `142948EE0` slot 0 `FUN_140D07E20` on `entity+0x5E0` and stores `[entity+0x9F0] = 1` at `140D07818`. It is called every update, through the tail jump at the end of the model dirty push `FUN_140C4AF10` (`140C4B39A -> 140D06BE0 -> [vtable+0xC8]`). So **the block is produced for these entities at every update, but only for a model that has a wind node** (`entity+0xA78 != -1`, created only for a model with a wind-policy material). Native reads that block as is; it stays zero natively because the material instance never gets policy bit 1 (see [WIND-BLOCK-IMPLEMENTATION-SHEET.md](WIND-BLOCK-IMPLEMENTATION-SHEET.md)). An earlier wording here, "native zeroes it", is withdrawn. The constructor store at `140D062D0` (`entity+0xA20`) is `animation+0x650`, not a wind flag. For the `ChrAsm` parts (vtable `2947DB8`) the update `FUN_140D01610` (slot `+0xD8`) also ends in a similar call (`140D01659`) but never produces wind: their constructor calls neither the wind predicate nor the node creation, and the only store to `+0x9F0` in the `.text` is `140D07818` (WIND-BLOCK-IMPLEMENTATION-SHEET §4).

**E2, E3: `group+0x50`, `group+0x60` (INFERRED constant).** The state initialiser `FUN_14004DF20` sets both to `(1,1,1,1)` from `1439F24B0` (`+0x50` at `14004E13C`). Scans of `140D00000-140D03A00`, `140D06000-140D08400`, `140D24000-140D26000` and `140D3F000-140D41800` for stores at `+0x50/+0x60/+0x70` found **none that targets the state** (`FUN_140D01DB0` stores 16 bytes at its own `+0x50/+0x60` for a different class with dirty byte `+0x95`, via `FUN_140D05210`; all callers of `FUN_14004E180` are emitters). Unread holder-setter areas remain (OPEN); R8B-1 and R8B-5 measure it.

**E7: the 400-byte state (STATIC mechanism, partial writer list).** One state per draw entity: the holder `X` (constructor `FUN_140D3FCB0`; `X` at `entity+0x230`, state at `X+0x10`), written **in place**. Writers found (state offsets, not `X` offsets):
- the holder setters: `FUN_140D3FE90` recomputes `+0x84/+0x85` (and stores `X+0x828`, outside the state), `FUN_140D40080` stores `+0x87` (the phantom flag, `X+0x97`), `FUN_140D01E30` stores `+0xC0/+0xC8` with dirty `+0x181`; the dirty bytes `+0x180/+0x181` are inside the snapshot;
- **the per-update key chain (new in this version):** both the model dirty push `FUN_140C4AF10` (through the tail jump into `FUN_140D07710`) and the ASM-part update `FUN_140D01610` call `FUN_140D3FF50(X, src)`, a thunk to `FUN_140D3BFA0(src, X+0x10, X+0x1A0)`, which writes `state+0xF8..+0x108` (`FUN_140D25360`), `state+0x110..+0x120` (`FUN_140D252D0`), `state+0x70..+0x80` with dirty `+0x180` (`FUN_140D254B0`) and `animation+0x658..+0x668` (`FUN_140D25180`); `FUN_140D250F0` advances these time-blended keys each update. **`group+0xF8..+0x123`, which prepare reads for the forward light (`:3099`), is therefore per-update data during gparam transitions**;
- the dirty push itself: `FUN_140C4AF10` reads `model+0x70|0x74` and calls the entity's virtual setters (`FUN_140D06A00/06AA0/06B40` placement, `FUN_140D06930`, `FUN_140D069A0`, `FUN_140D066F0` phantom id, `FUN_140D06890`, `FUN_140D06850`, `FUN_140D3BF40`, ...);
- the render-side emit `FUN_140D079C0` writes the live group (`+0x98` bit 0 from `FUN_140DD5D10`, and `+0x180 = 1`, `140D07C16/140D07C1D`) when `FUN_140D06E10` passes the live `entity+0x240` (unfaded path; the faded path copies with `FUN_14004E180`). Not run natively.
The writers of the fade (`+0xDC`), the wet words (`+0x8C..`) and the character appearance are **not traced** (MENUS-MAP line 4469 names a ChrIns writer it marks as not read; the part tint is stored by `FUN_140D009C0` at `entity+0xBD0+0x10*c`, not in the group; native `:4239-4241` and `:5049` say the same). Timing: update steps, before the draw side; thread: not established (per-character steps are `g63`, INFERRED parallel). Lifetime: one persistent state, never double-buffered.
The native collect already reads **parts** of the group (`:3440-3540`; `ProduceNativeSelectedInstances` `:2448`); prepare re-reads the whole 400 bytes later in the same frame.

**E1: the material generator fields (INFERRED admission constants).** `material+0xD0` is the material's dynamic-parameter generator block (flag, register, begin and end of the register array); material data set at load; the only consumer is the log row.

**What runs between collect and prepare (INFERRED).** The update-side writers precede `GraphicsStep`; the render-side code that writes the live group (`FUN_140D079C0`) is not run natively; so values at prepare equal values at collect, unless an unrecovered task runs after `E006A0` in the draw-side steps (R8B-1).

## 4. Other engine touches in the call tree of prepare (beyond the eight; STATIC at the native site)

| Site | Engine memory or code touched | Per |
|---|---|---|
| `ProduceNativeForwardLight` `:3095`, called at `:4864` | `group+0xF8/+0x110/+0x108/+0x120` (the per-update key above), the gparam light and fog banks (`records.Light/Fog`), and **engine functions** `FUN_1401496D0`, `FUN_14014A2D0`, `FUN_14014E900`, `FUN_14014E5D0` executed on the preparing thread | draw |
| `NativeGparamRecords` `:2996`, `:3119` | falls back to the engine lookups `FUN_140032770`, `FUN_140038E70` on engine bank pointers and reads the engine record | draw |
| `AcquireNativeLightingPrograms` (call `:5306`, body `:2640-2648`) | walks the engine programme repositories `[image+0x4792BF8]`, `[image+0x4792D58]` and reads `image+0x3D20520/0x3D20500`, repeated on every call until resolved | frame |
| `ProduceNativeEnvRegions` `:2857` (call `:5312`) | `[4961B50]+0x42B0/+0x42A8`, the region records, **engine function `FUN_1400416A0`** (`:2871`, `:2885`), entry fields `+0x8/+0xC/+0x10/+0x20/+0x28/+0x40`, a name prefix, `image+0x4929B20` | frame |

Not engine reads in prepare (corrected): `ProduceNativeLightAccParam` (`:2938`) uses the camera and constants only; the old-root light container reads (`[4961B50]+0x41D0`, `:3001-3016`) belong to the collect-time light blocks (`CollectNativePointLights` `:2695-2742`, `CollectNativeLightBlocks`); `ProduceNativeSssInputs` (`:2828`) reads the native registry and code immediates once (`:2785`).

## 5. The snapshot (ER method, D-0157)

ER's method: the current engine owner produces the data; one backend turns it into native records; the recorder owns what it uses until completion; ordered records carry the data to the thread that submits (METHOD-710; DEEP-STUDY-716 section 6 is the inferred design). The concrete ER producer `producer/001E92F0` (DEEP-STUDY-716 section 2) acquires the engine recording context and records the native upload itself while the engine's writer runs, under a lock. Applied here:

**Level 1 (value snapshot at the existing collect point; low risk).**
- `NativeSelectedModel` gains values, not pointers: `group_state` (0x190 B, one `Read` per model, replacing the partial reads of `:3440-3540` and the prepare reads E2, E3, E7; the forward-light key `+0xF8..+0x123` is inside it), `animation_flags` (E4, E5), `wind_block` (0x410 B) with `wind_valid`, and the four material scalars (E1, or drop them). E8 is a startup cache taken after engine initialisation.
- Prepare reads these members only; the pointers `model.group`, `model.animation` and the material pointer are used **nowhere in prepare**, which also removes the use-after-free exposure of a worker reading memory of a model the engine retired while a job was queued.
- Cost: about 1.2 KB per selected model per frame (a memcpy). Collect keeps its cost.

**Level 2 (engine-side producers; adapts ER's shape, after Level 1).** A native hook after `FUN_140D3E950` (items) and after `FUN_140D07710` (draw entities) publishes the wind block and flag into a backend-owned slot keyed by the entity, with a version; a hook after the key chain `FUN_140D3FF50` and the dirty push republishes the group state when a dirty bit was handled. Collect selects by version instead of copying (the data half of R8b F8). The always-changing set is the wind block, the time-blended gparam keys (`state+0x70..+0x80`, `+0xF8..+0x120`) and the effect-driven fields. The hooks run on whichever thread runs the engine writer, so each slot is a swapped immutable record under a per-entity lock; the render side takes the latest complete record (lifetime by `shared_ptr`, retirement by the existing completion tokens). This is an adaptation: ER records the upload on the engine's recording context, whereas here the slot is CPU data that the render thread turns into constants.

**Guard (F6, permanent in the diagnostic build).** Every Nth frame, build the draw inputs twice, from live reads and from the snapshot, and compare the produced constant bytes per draw (`cbvModelParam`, `cbModelParam`); log differences. Acceptance for Level 1: zero differences over the witnesses (High Wall foliage with wind, a character with cloth or a phantom, a gparam transition, the menu offscreen view).

**Thread plan.** Level 1 is necessary, not sufficient. Before prepare may leave the owner thread, the section 4 touches must become collect-time values or native code: the gparam banks and the engine light functions (extend the `NativeLightingInput` pattern: produce `light_entry` and `fog_entry` per distinct key at collect), the programme-repository walk (resolve once, cache), the environment-region engine call and the gparam lookup fallbacks. Until then prepare stays on the owner thread and only recording and presentation can move (R13 as planned).

## 6. Slices

| Slice | Content | Check |
|---|---|---|
| S1 | `group_state` snapshot at collect; E2, E3, E7 and the existing partial group reads become views of it | R8B-1 zero differences (dirty bytes `+0x180/+0x181` reported separately), constants rows unchanged on a recorded witness |
| S2 | `animation_flags` and `wind_block` snapshot (E4 to E6) | the foliage witness (EFFECTS-D3D12-DESIGN-NATIVE N4) equal before and after |
| S3 | E1 to admission or drop; E8 cached after engine initialisation | log row equal; R8B-4 |
| S4 | forward-light entries, gparam fallbacks, programme repositories and environment-region call moved to collect or resolved once (section 4) | forward-lit witness equal, no engine call in prepare (call-count row) |
| S5 | prepare on a worker with the scene as an immutable job (R13 F4) | `Refuse` and fault channel as in R13; pacing row |
| S6 | Level 2 producers behind a flag file, full scan as the reference mode | F6 comparison, no differences over a long walk |

## 7. Rows (read-only; live ones under the D-0116 protocol, announced in chat)

| Row | Action | Expected |
|---|---|---|
| R8B-1 | In a native run (no behaviour change), at collect and again at prepare, hash `group[0..0x190]` (dirty bytes `+0x180/+0x181` apart), `animation[0x210..0x61F]` with the two flags, and the `material+0xD0` fields for every selected model; log differences per 120 frames | zero differences; otherwise the model class and the first differing words identify the writer that runs between the two points |
| R8B-2 | Log `GetCurrentThreadId()` in the `E006A0` replacement and at prepare, and compare the scene serial (`:5425`) with the presentation serial (`:622`) | equal to `owner_thread`; scene produced in the same frame (settles the INFERRED points of section 1) |
| R8B-3 | Count per frame how many models have a changed `group_state` hash against the previous frame, and how many wind blocks and key words changed | the dirty rate Level 2 can exploit (R8b F8) |
| R8B-4 | Compare `image+0x4929B20` (16 B) at frame 1 and after 600 frames | equal and non-zero (after engine init) |
| R8B-5 | Reference observation (Thomas's own run only): write watch on `group+0x50/+0x60` of one model for a minute | no write after construction (E2, E3 constants) |
| R8B-6 | Count engine calls (`1401496D0` family, `1400416A0`, `32770/38E70`) and repository walks made inside prepare | zero after S4 |

## 8. Open points

1. The FD4 step and worker of the `ModelIns` task that calls `FUN_140D3E950`, and whether `ChrAsm` parts ever get wind (`FUN_140D01610`).
2. Whether any writer stores into `group+0x50/+0x60` after construction (unread holder-setter areas).
3. The step and thread of the character group writers (fade, wet, appearance), and the order of `FUN_140D54F80` against EndFrame (R8B-2).
4. The cost of the double build in F6.

## 9. Exports

`decompiled/DS3-1152-F-` + `00D3E950`, `00D3D280`, `00C4D330`, `00C4AF10`, `00D3C4D0`, `00D01E30`, `00D01DB0`, `00D034F0`, `00D03540`, `00D03590`, `00D07610`, `00D06A00`, `00D06AA0`, `00D06B40`, `00D553C0`, `00D56930`, `00D40080`, `00D3FE90`, `0004DF20`, `00D07710`, `00D3FF50`, `00D3BFA0`, `00D01610`; and from earlier notes `00D079C0`, `00D06E10`, `0005B3D0`, `00059470`.
(`FUN_140D3FE90` and `FUN_140D40080` have short assembly files because Ghidra overlaps those functions.)

## Amendments by the engine step and thread map

[ENGINE-STEP-THREAD-MAP](ENGINE-STEP-THREAD-MAP.md) answers the thread points left open here. **R8B-2 is answered with INFERRED status, not closed:** the collect, the render step, `EndFrame` and `Clear` run consecutively on the owner thread inside the `DrawStep` step (the collect from a `DrawStep`-family method, `g1`, lane 0, row 79 alone; the render step, `BeginFrame` and `EndFrame` are thread-checked by the module; the K stamps show no gap between collect and render step). One captured stack at each frame hook (map row TM-3) would close it. The writers of the character group state (fade, wet, appearance, wind) run in `ChrIns_*` steps of group `g63`: on the owner thread **and** on pool threads, all in rows 23 to 57, before the draw rows. The step of the `*ModelIns` task (E4/E6 wind block) stays OPEN: its registration function is undisassembled; map row TM-2 logs it.
