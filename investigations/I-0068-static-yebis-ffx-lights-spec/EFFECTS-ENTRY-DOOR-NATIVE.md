# I-0068 part AN: the entry door of effects in the native build, end to end

Request (coordinator, 2026-10-11): specify in static the door through which effects enter the native build, from the deferral of `7CB820` to `ObjModelSfxSlot` (`140E6EB70`, `ModelSfxParam`) and the event and param requests, as ordered steps that fit EFFECTS-IMPLEMENTATION-CHECKLIST: what the native build must call or reproduce, in which order of the frame, with expected values and witnesses on the five zones. The menus thread noted that the summon signs depend on the same door (PHANTOMS-SIGNS-NATIVE).

Labels: **STATIC** (executable, decompilation, RTTI vtable map `I0067Rtti`, regulation params, MSB of the retained gamedata), **INFERRED**, **LIVE-ONLY**. Nothing was launched. Sources: [EFFECT-LIGHTS](EFFECT-LIGHTS.md), [EFFECTS-PHASE1-NATIVE](EFFECTS-PHASE1-NATIVE.md), [FFX-ATTACHED-NATIVE](FFX-ATTACHED-NATIVE.md), [LIVE-REDUCTION-AM](LIVE-REDUCTION-AM.md) §4.

## 0. Answer first

1. **The door is three functions.** Every effect the game creates through `SprjSfxImp` ends in one of `FUN_140E6DCD0`, `FUN_140E6DA30`, `FUN_140E6DBC0`, and each of the three tests `[SprjSfxImp+0x58]` (the GX effect system). The scene-control creation core `24ED90` has no other caller than `24EFE0` (called only by `E6DCD0`) and `24F0F0` (called only by `E6DA30` and `E6DBC0`). Complete caller lists in §1.
2. **Nothing has to be reproduced for the sources.** The twelve handle-API callers of `7CB820`, the object slots (`ObjModelSfxSlot`, `ObjMultiSfxSlot`, `ObjBurnSlot`, `ObjRigidOffsetSfxSlot`, `ObjShineTreasureSlot`), the character slots, the event commands and the cutscene factory are engine CPU code. They run natively already and are today answered with an empty handle only because `+0x58` is null (and, for `7CB820` alone, because the native build replaces it by `DeferNativeSfxSceneRequest`).
3. **The per-frame call exists natively.** `SprjSfxImp::Update` `E6E750` is called by `SprjGraphicsStep::STEP_Update` `D5B140`, the same function that calls the time tick `D57900` the native build already hooks. No new frame hook is needed.
4. **The work is therefore three changes and a check matrix:** publish `+0x58` (EFFECTS-PHASE1-NATIVE), delete the `7CB820` replacement (checklist step 2.1), and verify the funnel with the counters of §5 at the five witness zones, whose expected effect ids and upper-bound counts come from the MSB and params (§4). The door also opens the summon signs (`SosSignMan` → `7CB820`, id 30001): no sign-specific work.
5. **Two corrections to earlier notes.** The torch flame is **not** a `7CB820` request (LIVE-REDUCTION-AM §4): it is created lazily by `SprjFD4SingleSfxIns` through `E6DCD0`. And the nine `identifier:300` requests of the native logs are not torch flames.

## 1. The funnel (STATIC)

### 1.1 Level 3: the three creators and the scene control

| Creator | Transform input | Scene-control call | Callers (whole executable) |
|---|---|---|---|
| **`E6DCD0(this, handle, &id, transform, flags)`** | world transform block | `24EFE0` → `24ED90` | `E6BED0` (`SprjFD4SingleSfxIns` slot 3), `E6A350` (`SprjFD4MultiSfxIns` slot 3), `7CB820`, `7CB8F0` |
| **`E6DA30(this, handle, &id, matrix, flags)`** | 3x4 matrix converted to 4x4 (`140017970`) | `24F0F0` → `24ED90` | `7CB750` only |
| **`E6DBC0(this, handle, &id, transform, flags)`** | 4x4 (`140004500`) | `24F0F0` → `24ED90` | `CE5310`, `CE5260` (`SprjRemoFactory` slots 5 and 4: cutscene effects) |

Each creator: initialises the caller's handle (`141A879A0`) and its create-info (`140249950`, EFFECT-LIGHTS §2.2); **only if `[this+0x58] != 0`** calls the scene control, copies the result (`141A87A50`, `+0x20`, create-info `14024A320`), destroys the temporary (`141A879F0`) and runs **`E6F8C0(this, handle)`**: it builds a two-entry parameter set (key 0 = 0, key 10000 = a value chosen by byte `+0xF` of the object returned by `140597290`, from the table {0, `143D7F0C8`, 1.0}; 0 when `[1447572B8]` is null) and applies it to the new handle (`141A87DF0`). These are the FXR external-value slots 0 and 10000; CPU only, engine code, call as is.

Without `+0x58` the creator returns the initialised empty handle: the instance pointer at `handle+0x18` stays null, which is exactly what every caller sees natively today.

### 1.2 Level 2: the handle API and the SfxIns objects

| API | Function | Goes to |
|---|---|---|
| world transform | `7CB820(handle, id, transform, flags)` then `7CB630` (copy to the caller's storage) | `E6DCD0` |
| block local | `7CB8F0(handle, blockSfx, id, transform)` (the block argument is not used by the call) | `E6DCD0` |
| alternate matrix | `7CB750(handle, id, matrix, flags)` | `E6DA30` |
| **single SfxIns** | `SprjFD4SingleSfxIns` (vtable `142992728`, ctor `E6BB70`, created by the manager factory `E6EFF0`) | start in slot 3 `E6BED0` → `E6DCD0` |
| **multi SfxIns** | `SprjFD4MultiSfxIns` (vtable `1429922C8`, ctor `E69EF0`, factory `E6EEF0`) | start in slot 3 `E6A350` → `E6DCD0` for each of its handles |
| cutscene | `SprjRemoFactory` slots 4/5 (`CE5260`/`CE5310`) | `E6DBC0` direct; the multi factory through `CCEF00` → `E6D9D0` → `E6EEF0` |

**SfxIns lifecycle (`E6B210`, called by `E6E750` for every SfxIns of the active list):** state 0 (pending): once the owner is visible (flag bit 2 from `D03050`/`64F000`/`619800`: the owner's draw state) and no stop flag is set, slot 3 runs (`E6BED0`: read the owner's transform through its child `+0x68`, call `E6DCD0` with the id at `+0x60`, store the handle at `+0x70` and the create-info at `+0xA0`, bind the follow target `2447C0`; when the handle is empty mark the SfxIns dead `E6B040`); state 1 (running, `E6B3C0`): slot 4 (`E6C0E0`, the per-frame follow: re-read the owner transform, apply it to the handle with `141A87EE0`, re-bind the follow target `2447C0`, set the handle visibility from `E6B0F0`), and when the stop flag (bit 0) is set slot 8 (`E6C1D0`); state 2 (stopping): slot 5 (`E6C080`: unbind the follow, release `141AB18C0`); state 3 removes it. The factory `E6EFF0` links a new SfxIns into the pending list `[SprjSfxImp+0x20]`; `E6E750` splices that list into the active list `[+0x08]` (`E6FCB0`) before updating, so a SfxIns created in a frame is started in the same `E6E750` call.

### 1.3 Level 1: the request sources

Complete from the RTTI map; the callers of each handle API function are exact. "Idle" = produces effects with no player action.

| Source (class or function) | Reaches | Trigger | Idle in the witnesses |
|---|---|---|---|
| **`ObjModelSfxSlot`** (`14065A010`), param `ModelSfxParam` row model×1000 | single SfxIns (`E6EB70`/`E6EFF0`) → `E6BED0` → `E6DCD0` | the object is created and visible; one SfxIns per entry {id, dummy ref} and per dummy of the model with that reference (collected by `662D60`), per-frame follow when the update flag is set | **yes** (torches, lanterns, fires, ambient) |
| `ObjBurnSlot` (`1407C9F50`, class decoded in LIVE-REDUCTION-AM §4: ids from `ObjectParam` `+0x34..`) | `7CC4B0`/`7CC410` → `7CB8F0` → `E6DCD0` | the object burns (progress `+0x70/+0x74`) | no |
| `ObjMultiSfxSlot` (`65AB10`), `ObjRigidOffsetSfxSlot` (`7CABD0`) | `7CC4B0` → `7CB8F0` | event/script request, ObjAct | no |
| `ObjShineTreasureSlot` (`7CB390`) | `7CC4B0` → `7CB8F0` | an unopened treasure | **yes** (one per unopened treasure) |
| bonfire condition `SprjLuaEventConditionBonfire` (`45DD90`) | `7CC4B0` → `7CB8F0` | bonfire lit / available | **yes** (at a bonfire) |
| object init `140667C10` (flag `[obj+0x193]` bit 1) | `7CC4B0` slot route with the id of `ObjectParam` (default `0xC5C2E` = 810030 when the row names none) | object creation | rows with an effect only |
| `EventObjEntity` slot 6 (`52EC30`), `EventChrEntity` slot 6 (`5292A0`), `46E600`, `97CFA0` (owner kind), `SprjEventObjSfxCtrl` (`1404D5D10` → multi SfxIns `E6EA70`) | `7CB820` (and `7CC4B0`/`7CBEA0` by owner kind), multi SfxIns | EMEVD / action commands | map-event dependent |
| character slots `ChrSingleSfxSlot`, `ChrSingleOneshotSfxSlot`, `ChrRigidOffsetSfxSlot`, `ChrMultiSfxSlot`, `ChrLanternSlot`, `ChrBurnSlot`, `ChrStatueDeadSlot`, `ChrSorceryWepSlot`, `ChrMagicGoodsUseSfxSlot`, `ChrShineTreasureSlot`, `ChrSoulEatSlot`, `ChrConditionSfxSeSlot`, `ChrWepEnchantSlot`, `ChrWepEnchantWhipSlot`, `ChrWepSfxSlot` | `7CBEA0` (follow binding `2447C0`) → `7CB820` | character state | lantern and held flame yes; the rest on action |
| character SFX manager (`844400`, `846CE0`, `846E40`) | `7CBBA0` → `7CB820` | character events | no |
| signs `SosSignMan` (`853800`) | `7CB820` | an active sign record (white NPC sign id 30001, PHANTOMS §2) | at High Wall with an Ember |
| point effects with a stored position (`568AC0` fixed id 220, `4D9610`, `5306D0`) | `7CB820` | map event rows | map dependent |
| bullets (`97CDC0`, `97C970`, `977D00` via `97CFA0`) | `7CB820`/`7CC4B0`/`7CBEA0` | projectile | no |
| `7CB750` family (`7C3010` and callers `7C1AD0`, `7C50C0`, `7C52C0`, `7C54A0`, `7C5C60`, `7C5DB0`, `7C5F00`) | `E6DA30` | not classified (INFERRED: effects with an explicit matrix, e.g. hit and trace effects) | no |
| cutscene (Remo) | `E6DBC0`, multi SfxIns | in-engine cutscene | no |
| **FXR child spawns** (an effect action creating another effect, e.g. the sign controller 30001 spawning its visual) | **inside the FFX library**, not through `SprjSfxImp` | the parent effect's actions | yes, whenever a parent runs |

Last row: effects created by FXR actions never pass the three creators (`24ED90` has two callers only). The library-side creator `141ACCA20` (instance creation, called by `24ED90`) has three other callers, the **`GXFfxManager` vtable slots 5, 6 and 7** (`1421551C0`, `142155310`, `142155460`; vtable `143D6A078`, the manager PHASE1 §4 step 2 builds): that is the manager interface the library uses to spawn child instances (INFERRED from the slot position; engine code, no GX access to two levels). They need the library registries (EFFECTS-PHASE1 §3 step 24) and the instance update (slot 8), not the door. Witness: library instance count greater than SfxMan handle count.

### 1.4 Where the three conditions of a request live

A request is refused (empty handle) when any of these holds. Each is engine CPU state the native build already runs, except the first:

1. `[SprjSfxImp+0x58] == 0` (the effect system; native today).
2. The owner is not visible or not started: object slot `[obj+0x290] == 0` (no draw entity) or the slot's layer differs from `[obj+0x18E] & 7`; SfxIns visibility bit from the owner's draw state. These follow the display groups the engine computes.
3. The creation-time hide mask (EFFECT-LIGHTS §2.4) covers the effect's category: 0 outside cutscenes.

## 2. Frame order (STATIC where marked)

`SprjGraphicsStep::STEP_Update` (`D5B140`, step table `144793350`) runs, once per frame, in this order:

1. `D57900` (time tick; native hook `UpdateNativeSimulationTime`);
2. **`E6E750(SprjSfxImp, tick)`**:
   1. `[this+0xD8]` object, when present;
   2. splice pending SfxIns (`[+0x20]` → `[+0x08]`), then for each SfxIns `E6B210`: **new single and multi SfxIns start here** (`E6DCD0`);
   3. `+0x71` flag clear;
   4. `[this+0xE0]` event hook (`E687F0`), when present;
   5. `[SceneCtrl+0x20] = [this+0x72]` then **SceneCtrl slot 8** `24D000` (hide-mask swap, resource tick, controllers, library update with the native update pass: instance, emitter and host updates; the point-light host pushes its light into `[[GXRS]+0x41D0]+0x10`; EFFECTS-PHASE1 §5);
   6. sound state;
3. `D4AC90` for the two systems at `step+0xD0/+0xD8`, the step flag test `F0F4F0`.

Order against the producers (INFERRED, not decoded): the object, character and event slots run in the world update tasks; the handle API creates the GX instance **immediately** inside `E6DCD0`, and the SfxIns objects are started inside the next `E6E750` call, so a request made in frame N is alive for the update of slot 8 in frame N (when the world update precedes `STEP_Update`) or N+1. Either order is safe: the native build must not reorder them, only run the engine's own tasks. After `STEP_Update`, the draw submission follows in the render steps of the same state machine (`STEP_BeginDrawStep` `D593A0` and `STEP_WaitFinishDrawStep` `D5B1D0`; their contents are frame counters and a wait, the draw work itself is not decoded here), which is where the later checklist steps (3.x, 4.x) draw the instances.

## 3. What the native build must call or reproduce, in order

| Step | Native work | Mode |
|---|---|---|
| **D1** | publish `[SprjSfxImp+0x58]` = the scene control built by EFFECTS-PHASE1 §2-§4, and replace `E6EA10`/`E6EC60` by the view attach/detach (PHASE1 §6) | CHECKLIST 1.1 |
| **D2** | **delete the `7CB820` replacement** (`DeferNativeSfxSceneRequest`, `game_bootstrap.cpp:7268`, hook `:9148`) and let `7CB820`, `7CB8F0`, `7CB750`, `7CB630` run unchanged. Keep an observer that logs, per call, the identifier, the flags (8 for every `7CB820` caller observed) and `_ReturnAddress`; do not replay requests deferred before the system existed (FFX-ATTACHED risk 3) | CHECKLIST 2.1 |
| **D3** | nothing for `E6DCD0`, `E6DA30`, `E6DBC0`, `E6F8C0`, `E6FCB0`, `E6B210`, `E6EFF0`, the SfxIns classes and every slot class: engine code. Required to exist natively: `[144799C98]` (published), `[1447572B8]` may stay null (`E6F8C0` handles it), the object and character instances (present) | CALL (none) |
| **D4** | the per-frame `E6E750` is already called through `D5B140`; with `+0x58` set it now also runs slot 8. Native update and proceed pass objects (PHASE1 §5.7) must exist before the first frame with `+0x58` | CHECKLIST 1.2 |
| **D5** | the zone-entry leaf `E6E620` (bytes pinned natively): with `+0x58` set it must still touch only CPU fields; verify on the first zone entry that the SceneCtrl instance list is reset and creations resume | CHECK |
| **D6** | log, do not block, the requests whose identifier is absent from the FXR resources: `[[SceneCtrl+0x28]+0x160]` resolution (R3) decides; an absent resource yields an empty handle in the engine too | CHECK |
| **D7** | after the first frame with `+0x58`, run the matrix of §5 at each witness zone | CHECK |

Checklist integration: steps 1.3 (creation routes), 2.1 (hook replacement), 2.2 (object slots) and 2.3 (character handles) are the door; their checks are the rows of §5. The new checklist section "1b. The entry door" (below) carries D1-D7 with the rows.

## 4. Expected values per witness zone (STATIC: MSB, `ModelSfxParam`, `ObjectParam`)

`Get-ZoneEffectSources.ps1` reads the primary map of each zone (`mXX_00_00_00` MSB of the retained gamedata) and joins its object parts to the params. Output: `evidence/I-0068/queries/objparam/zone-effect-sources.tsv`. Counts are **upper bounds** of what the idle sources can create once every part of the map is loaded and visible; the live counts are lower (display groups, distance). The effect ids are exact.

| Zone (map) | `ModelSfxParam` objects (instances, models) | ids created at dummy 200 unless noted | `ObjectParam` effect objects (burn, conditional) | MSB SFX regions (with activation part) | Wind SFX regions | Treasure events |
|---|---|---|---|---|---|---|
| High Wall `m30_00` | 353 (13) | 800023 ×133 (torches `o304863` ×35, `o004660` ×98), 839010 ×105, 800030 ×72 (210/212 on `o304783`, `o304857`), 800043 ×36, 800052 ×33, 800050 ×21, 800060 ×14, 800062 ×6, 800061 ×4, 691094 ×1 | 319 (19 models: 130000, 830060-830066, 830070-830073, 830075/76, 830062) | 238 (227) | 3 (800081) | 73 |
| `m31_00` | 622 (10) | 839010 ×485, 831021 ×65, 831022 ×44, 800061 ×18, 800060 ×9, 800050 ×1 | 23 (3, 130000) | 76 (76) | 19 (800080-800082) | 89 |
| `m37_00` | 27 (3) | 800044 ×44 (200/201/202 on `o373023`, `o373024`), 800023 ×12 | 113 (15 models, 830062 and 130000) | 304 (253) | 1 (800081) | 93 |
| Firelink / `m40_00` | 422 (3) | 839010 ×409, 800022 ×12, 800050 ×1 | 0 | 18 (18) | 6 (800081) | 43 |
| `m45_00` | 186 (11) | 845220 ×40, 845205 ×30, 845070 ×26, 845223 ×16, 845206 ×8, 845200 ×2, 845047 ×1, 800022 ×20, 839010 ×43 | 251 (19 models, 130000 and 830062) | 140 (137) | 8 (845010) | 84 |
| `m50_00` | 8 (3) | 850000 ×6, 850080, 850140 | 63 (6 models) | 268 (268) | 6 (850801, 850802) | 61 |
| `m51_00` | 316 (10) | 851070 ×124, 839010 ×73, 800020 ×45, 800022 ×30, 838100 ×25, 851001 ×16, 851060 ×2, 851072 ×1 | 84 (6 models, 830062 and 130000) | 12 (12) | 3 (850800-850802) | 94 |

(The zone names of m31, m37, m45, m50 and m51 are the labels of the I-0063 witness series; the table keys on the map id.)

Reading the table:

- **Persistent object effects are the first witness.** After the first `E6E750` with `+0x58`, the idle SfxIns count of the zone must be at most the instance count above and non-zero in every zone but `m50_00` (8 only), and the created ids must be a subset of the listed ids. m50's 8 (one torch-like model of 6 instances plus two singles) is the weakest witness: use `m30_00`, `m31_00` or `m40_00` for the first check.
- **`839010`** is the commonest id (m31 485, m40 409): objects `o391000`/`o001240`, one entry each: a cheap and numerous ambient effect, the best statistical witness for the single SfxIns route.
- **SFX regions** (MSB `FFXID`, with `ActivationPartName`): their creation path is not decoded (the world SFX holder `WorldBlockSfx`/`WorldAreaSfx`/`WorldSfxManImp`, ctor `68C7E0`, is a per-block container); they surface through the same creators (§1.1), so the id histogram at the zone is the witness (ids in `zone-effect-sources.tsv`, e.g. `m37_00`: 837012 ×167, 837000 ×45, 837011 ×30, 837010 ×23).
- **The id-300/320 requests are not in any table** (not an MSB region id, not a `ModelSfxParam`/`ObjectParam` id): they are event, sign-table or character routes (LIVE-REDUCTION-AM §4); T1·b reads their callers.
- `ObjectParam` counts are conditional (burning objects): not witnesses before a fire test.

## 5. Check matrix (read-only, original and native at the same spot)

| ID | Read | Expected | Meaning when it differs |
|---|---|---|---|
| **E-D1** | counters on `E6DCD0`, `E6DA30`, `E6DBC0`: calls, and calls taking the `+0x58` branch | calls = calls taking the branch (after D1); original and native equal per zone within the frame budget | calls without the branch: `+0x58` lost; fewer calls natively: a source class does not run (see E-D3) |
| **E-D2** | histogram of `E6DCD0` callers (`_ReturnAddress`: `E6BED0`, `E6A350`, `7CB820`, `7CB8F0`) and of `7CB820` callers (the twelve of §1.3) | the same set natively and in the original; first entries `E6BED0` (single SfxIns) at the torch zones | a missing caller names the missing source class |
| **E-D3** | the identifier histogram of the first 600 frames after zone entry | ⊆ the ids of §4 plus event/sign/character ids; ids of §4 present with count ≤ instances | an id of §4 missing: the owning model/slot not created or not visible (display groups) |
| **E-D4** | `SprjSfxImp` active list `[+0x08]` count and pending list `[+0x20]` count (after `E6E750`) | pending 0 after each `E6E750`; active ≈ E-D3 count of long-lived effects | pending > 0 persistently: `E6E750` not running the splice (frame hook broken) |
| **E-D5** | `[SceneCtrl+0x14C]` swaps every frame; library instance count (`141AB1680` jobs) | R4 of EFFECT-LIGHTS; instance count ≥ handle count | equal counts with parents present: FXR child spawns not running |
| **E-D6** | `[[GXRS]+0x41D0]+0x10` entries | 163 at the m40 bonfire view (59 map + 104 effect), EFFECT-LIGHTS R7 | door open but no lights: host or mask (R8-R10) |
| **E-D7** | first zone entry after D1: `E6E620` effect on the SceneCtrl lists | lists reset, creations resume within one frame | creations stop: the leaf touches the effect system (D5) |
| **E-D8** | torch witness at High Wall (`m30_00` `o304863`, 35 instances): `14065A010` per object, `140E6EB70` arguments, then `E6BED0` | id 800023, dummy 200, one SfxIns per torch in view; creation follows the object becoming visible | T1·a of LIVE-REDUCTION-AM §4 |
| **E-D9** | sign witness (Argo's save with an Ember, PHANTOMS-SIGNS-NATIVE S1): `853800` then `7CB820` id 30001 | one `7CB820` per visible sign, flags 8, `sign+0x10` non-null | `+0x58` or the hook (D2) |

## 6. The summon signs

PHANTOMS-SIGNS-NATIVE states that the signs need the effect system only: E01 billboards, E04 distortion and an E06 point light per sign. With this note: the sign controller effect 30001 is created through `7CB820` (door), and the visual `3000x2` is a child spawn inside the library (§1.3 last row), so a sign needs D1 and D2, the library instance update (slot 8), then the draw steps 3.x (E01) and 6.x (E04). No sign-specific hook.

## 7. Residual risks (INFERRED, watch in the first run)

1. **Visibility gate of the SfxIns:** the visible bit of a pending SfxIns comes from `D03050`/`64F000`/`619800` reading the owner's draw state; if the native display-group state differs from the original, SfxIns stay in state 0 and the E-D3 histogram misses ids while E-D1 shows no failure.
2. **Handle destructors:** `1A879F0` and the SfxIns release (`141AB18C0`) were audited to touch no GX device global two levels deep (part AK); a deeper hit shows as a fault at object retirement, not at creation.
3. **`E6F8C0`** reads `140597290` and `[1447572B8]`; null is handled, a non-null manager must be the engine's own.
4. **Child spawns and registries:** if a parent runs but its child never appears, the library registries or the resource resolution (R2, R3) are incomplete, not the door.
5. **The unclassified `7CB750` family** (eight callers): their effects may carry ids unseen in §4; the E-D2 histogram names them.

## Exports

`decompiled/DS3-1152-F-` `00E6DCD0`, `00E6E750` (existing); read through Ghidra for this note (not exported): `E6DA30`, `E6DBC0`, `E6F8C0`, `E6BB70`, `E6B210`, `E6BED0`, `E6A350`, `E6EB70`, `E6EFF0`, `14065A010`, `D5B140`, `14268A320` (step table). Scripts: `Get-ZoneEffectSources.ps1`, `I0068Grep.java`.
