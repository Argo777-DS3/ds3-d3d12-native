# I-0068 part AM: static reduction of the live-only points (checklist §11, NPC-FAMILIES §7)

Request (coordinator, 2026-10-11): reduce in static the points still live-only in EFFECTS-IMPLEMENTATION-CHECKLIST and in §7 of NPC-FAMILIES-STATIC (formats, expected values, programme hashes), so that the next live tests are as short as possible. One commit per point. Not touched: MENUS-MAP (menus thread), native code (velocity pilot).

Labels: **STATIC**, **INFERRED**, **LIVE-ONLY**. No launch.

| # | Point | Result | Where |
|---|---|---|---|
| 1 | `view+0xCA4` (velocity scale k) | closed: stored by `140024740`, = MotionBlur MaxBlurLength for YEBIS views | [VELOCITY-CA4-WRITER](VELOCITY-CA4-WRITER.md) |
| 2 | LAYOUT: E01 input-layout formats | closed: static tables, 11 layouts, 0 mismatches | [FFX-INPUT-LAYOUTS](FFX-INPUT-LAYOUTS.md) |
| 3 | CSDOWN: dispatch site of `CounterDownSize` | closed negative: no dispatch site exists | §3 below |
| 4 | programme fingerprints for matching draws | table of 11,641 blobs (SHA-256 and DXBC checksum), FFX index map, key families | [PROGRAM-FINGERPRINTS](PROGRAM-FINGERPRINTS.md) |
| 5 | T1: owner of the torch flame | the flame is an ObjModelSfxSlot fed by ModelSfxParam row 304863000 {800023, 200}, created through 140E6EB70, not 7CB820; the 12 callers classified | §4 below |
| 6 | (part AN, separate note) the entry door of effects: three creators, per-zone expected ids, check matrix | [EFFECTS-ENTRY-DOOR-NATIVE](EFFECTS-ENTRY-DOOR-NATIVE.md) |
| 7 | (part AO, separate note) the native YEBIS chain except motion blur: ordered checklist P1-P24, inputs, gparam defaults and per-zone values, witnesses | [YEBIS-CHAIN-CHECKLIST](YEBIS-CHAIN-CHECKLIST.md) |

## 3. CSDOWN: no dispatch site of `CS_VFX_*CounterDownSize*` exists (E07 risk 6)

Question (LIVE-CHECKS §6, EFFECTS-E07 risk 6): the programmes `CS_VFX_StandardParticleCounterDownSize{Highest,Higher,MiddleHigher,Middle,MiddleLower,Lower,Lowest}`, `CS_VFX_SparkParticleCounterDownSize…` (same ladder) and `CS_VFX_WanderingVisionCounterDownSize` are in the shader-set builder (`291520`), but their dispatch was not found. Does a system use them?

**STATIC answer: no code of the executable dispatches a compute programme outside the sites below, so a CounterDownSize programme can only run through one of them, and none selects it by name.**

Every GX compute dispatch is a recorded command through two wrappers (and their replay `141929250`, `141921320`):

| Wrapper | Meaning | Callers (whole executable) |
|---|---|---|
| `FUN_1400C5EB0(ctx, x, y, z)` | `Dispatch` | `2A6A30` (1 site, emit: `Dispatch(max(ceil(n/128),1),1,1)`), `2A7E70` (2 sites: InitSort, BitonicSort, `Dispatch(1,1,1)`), `2B01E0` (1 site, spark emit); outside VFX: `1401A36B0`, `1401C5C60`, `1401C5EE0`, `1400E5110` (YEBIS and post blocks, not FFX) |
| `FUN_1400C5F40(ctx, args)` | `DispatchIndirect` | `2A6A30` (update), `2B01E0` (spark update) only |

- Reads of the system's programme fields in the E07 module: `S+0x28C0` (update CS), `+0x28C8`/`+0x28D0` (sort), `+0x28D8` (emit) only (`2A6A30`, `2A7E70`, `2B01E0`); there is no field for a fifth compute programme, so nothing reads a CounterDownSize record there.
- The names of the ladder (`Highest … Lowest`, 7 entries, no base variant) match the update-CS ladder (`CS_VFX_StandardParticle{,Highest,…}`, 8 entries) but have no consumer: the strings `143D691B0..143D6A010` are referenced only by the builder (no pointer-table reference).
- The kernel itself (`rUavCounter` t0 → `wUavCounter` u0, a 16-byte counter UAV) is a counter-scaling step; EFFECTS-E07 §3.1 already does that work with `CopyResource(+0x3A88 ← +0x3AF8)`, so no extra step is needed for the standard flow.

**Consequence for the native code:** do not implement a CounterDownSize pass; the native encoder's "log any CS dispatched outside E07 §3" check (E07 risk 6) is the only live guard and is expected to log nothing. INFERRED (not read at run time): the programmes are shipped but unused by DS3 (the set also carries unused programmes such as `WanderingVision`), which is one further reason the code was not found. A negative of this kind is closed by the exhaustive caller list above, not by the absence of a hit in one search.

CSDOWN is removed from the live list (LIVE-CHECKS §6, checklist §11 row 7).

## 4. T1: who owns the torch flame (FFX-ATTACHED §4.1): the owner is `ObjModelSfxSlot`, driven by `ModelSfxParam`

Question (EFFECTS-IMPLEMENTATION-CHECKLIST §11 row 2.2, "T1: 12 candidate callers of `7CB820`, none identified statically"): what creates the flame of the High Wall torch `o304863_8030` (dummy reference 200), so that the effects step 2.1/2.2 hooks the right function?

**STATIC answer (INFERRED where marked): the flame is not created through `7CB820`.** It belongs to an object slot class the notes had not named, `NS_SPRJ::ObjModelSfxSlot` (vtable `1427DF7D8`, slots `140659DF0` dtor, `140659EA0`/`140659EC0` reset, **`14065A010` update**), fed by the regulation param **`ModelSfxParam`** (param index 45, descriptor `14464FEB0`).

1. **The param.** `ModelSfxParam` (129 rows of 128 bytes; dumped with `Get-ParamRows.ps1 -Param ModelSfxParam`, `evidence/I-0068/queries/objparam/ModelSfxParam.tsv`) is keyed by **model number × 1000**: the torch model `o304863` has row **304863000**. The row holds up to 8 entries of 16 bytes `{int effect id, int dummy reference, 0, 0}`. The torch row has one entry **{800023, 200}**: effect **800023** (`f000800023.fxr`, INFERRED) on **dummy 200**, which is exactly the one dummy of the torch model (MENUS-MAP 54: reference 200 on `Model_Dmy`). The neighbouring torch model `o304857` has three entries {800030, 210}, {800030, 212}, {800043, 214}. Of 4,130 dwords in the param, the dummy references are 200 (112 entries), 210 (10), 201 (3) and 10 (3).
2. **The slot.** `14065A010` (called once per frame per object through the slot list of the object instance) runs, on its first frame, for each of the 8 entries whose id is ≥ 0 and whose reference is > 0: it collects every dummy of the object's model with that reference (`662D60`) and, for each, creates a handle with **`FUN_140E6EB70(SfxMan [144799C98], out, object, &entry.id)`**. That function creates a CPU `SfxIns`-type object (0x4D0 bytes, `140E6BB70`, linked into the manager list `[SfxMan+0x20]` by `140E6EFF0`) and gives it the owner's fade, colour and transform (`140659E20`, `140660240`, `140660120`, `140C4CC50`); it does **not** call `7CB820`, `7CB8F0` or `7CC4B0`. The slot then keeps the handles: on a visibility change it sends a 32-byte value to each handle (`140E6AD80`, the owner's fade or colour state), and each frame the update flag [param_3+0x18] is set it re-reads the dummy transforms (0x40-byte matrices) and sends them with `140E6AB90` (follow); the reset slots release the handles (`140E6ACF0`, `140E6AC00`).
3. **Consequence for the hooks.** The native build defers `7CB820` only (`DeferNativeSfxSceneRequest`). The nine deferred `identifier:300` and two `320` requests of the High Wall runs (Native1179/1181 `startup.jsonl`) therefore are **not** the torch flames of `ObjModelSfxSlot`; their owner is another route (the table below). The torch flame goes through `SfxMan` `140E6EB70` and is currently **invisible** in the deferral log: it needs its own hook (step 2.1) next to the `7CB820` one.
4. **The `ObjectParam` route (`ObjBurnSfxSlot`/`ObjMultiSfxSlot`) is not the torch either:** `ObjectParam` has 96 of 705 rows with an effect (ids 130000, 830062, 830070-830073, …), none with id 300, and **no row 304863**. Its default effect id when a row names none is `0xC5C2E` (810030) in `140E69900`, used by the object-instance init `140667C10` together with the model-number sound `model × 1000`.

The twelve callers of `7CB820`, classified (STATIC, from the RTTI vtable map `I0067Rtti` and the code):

| Caller | Owner and meaning | Source of the id |
|---|---|---|
| `52EC30` | `EventObjEntity` slot 6: one-shot effect on object dummies (event/action command) | command argument |
| `5292A0` | `EventChrEntity` slot 6: same for any entity | command argument |
| `46E600` | effect at an entity looked up by id (`1404CFC20`), EMEVD-style | argument |
| `97CFA0` | effect at an entity chosen by owner kind (character: `7CBEA0`, object: `7CC4B0`, else `7CB820`) | argument |
| `7CBBA0`, `7CBEA0` | character handles (`2447C0` follow binding) | argument |
| `853800` | sign effect (`SosSignMan`) | `RoleParam` row (PHANTOMS §2) |
| `568AC0` | timed, distance-culled point effect at a stored position (`+0xC..`) | **constant `0xDC` = 220** (`140568AB0`) |
| `4D9610`, `5306D0` | point effects at a stored position, id from a param row (`+0x34` of the row found by `kind × 10000 + n`) | param row |

So no `7CB820` caller carries id 300 as a constant: the 300/320 requests come through the argument of the event-type callers (`52EC30`, `5292A0`, `46E600`, `97CFA0`) or the param-row callers. They are **not** the torch.

**What is left live (one pass-through read at the High Wall, replaces T1):**

| ID | Read | Expected |
|---|---|---|
| T1·a | counter on `14065A010` for the torch object (object `o304863_8030`), and the arguments of `140E6EB70` | called; id 800023, dummy 200, one handle per dummy of the model (one) |
| T1·b | caller RVA at `7CB820` for the id-300 and id-320 requests (`_ReturnAddress`) | one of `52EC30`, `5292A0`, `46E600`, `97CFA0` (event routes) |
| T1·c | the model-sound call `1404AD240` at the torch (`model × 1000 = 304863000`) | called once per object instance with flag bit 0 of `[obj+0x193]` |

For the native build: step 2.1 must also let `140E6EB70` / `140E6EFF0` / `140E6BB70` run (they are CPU objects like the other handles), and the SfxIns object's own update must feed the effect system created in step 1.1 (to be read in FFX-ATTACHED §2 when the object type of `140E6BB70` is decoded; it is the same class family as the `E6C9D0` containers).

## 5. MAPBASE: the gparam (area, id) registration and lookup rules, decoded; one read left

Question (LIVE-CHECKS §6 MAPBASE; PRODUCERS-UNION-SNOW-GLOW §3.3): under which area does `MapBase` register, so that the Glow key finds `mapbase` id 0 (Enable 1, mode B)? The answer needs three rules; two are now STATIC, the third is one read.

**Rule 1: the registered id of each file kind** (`FUN_140D27BA0`, table `14294EFE8`, kind names `14294F020`; kind = the name-table index used by `FUN_140D59E90`):

| Kind | File | Registered id | Area |
|---|---|---|---|
| 1 | `default` | 1 | 1 (resident, applied at once) |
| 2, 3, 4, 5 | `M_template`, `ML_template`, `S_template`, `empty` | 2 | resident |
| **6** | **`MapBase`** | **4** | the map's area (see rule 2) |
| 7 | `m<aa>_<bb>_<nnnn>` map files (five per area: ids 5..9) | 5 + i (i < 5) | the map's area |
| 8 | `MapOverride_<nnnn>` | 10 | the map's area |
| 9 | `MapOverride_forRemo` | 11 | resident |
| 10 | `RemoBase` | 12 | resident |
| 11 | `s<aa>_<bb>_<nnnn>` | 13 | the map's area |
| 12 | `eclipse` | 14 | 7 (resident) |
| 13 | `MDLVW_…` | 3 | model viewer |

**Rule 2: the area of a registration** (`49D00(bank, data, area, id, …)`; callers `D275F0`, `D26A60`, `D26A70`, `D26AB0`, `D26C00`): resident kinds use the holder's own area (`holder+0x68`, or `holder+0x64` when it is −1; `FUN_140D27670`); map kinds are registered **per map area** by `D26AB0`/`D26C00` with `area = FUN_140D278C0(mapId)` = `((b3 + 100) · 100 + b2) · 10000` (b2, b3 = the two block bytes of the map id; −1 when a byte is ≥ 100), at most eight areas per holder. So `MapBase` registers under **every map area the map step enumerates, with id 4**: the INFERRED "under the map's area" of PRODUCERS-UNION §3.3 is now STATIC (the area comes from the map id, the id from the kind table).

**Rule 3: the lookup** (`FUN_140175C60(bank, key)`, key = (area, id) of 8 bytes):

1. key (−1, −1) → (1, 0);
2. exact (area, id) in the two-level hash (outer by area, inner by id);
3. else the fallback entry **(area 1, id 0)** (hash constants `0x5EADBEEE`, `0x5EADBEEF`);
4. else the bank's built-in record (`bank+0x1B8`).

The key of a consumer is a pipeline key pair: for the Glow filter, index 10 (`pipeline+0x4028` / `+0x4090`). `FUN_140D52B70` writes indices 0..9 from the per-layer keys when the current area key matches (`[+0x190]`, `[+0x194]` against the globals `1446402E0/E4`) and otherwise `FUN_140088AF0` writes **every** index from the single key at `+0x198`. `FUN_140D25450` writes a separate key at `pipeline+0x3FA8` (not index 10).

**What follows:** with the table above, the Glow lookup finds `mapbase` id 0 only if the key's id is **4** (registered `MapBase`, any map area registered) or if the exact entry is missing and the fallback entry (1, 0) exists. Nothing registers id 0 under area 1 (default is id 1), so the observed chain (Glow mode B, Enable 1) implies the view key id is 4 or that the file-internal id 0 maps onto registered id 4 (rule 1 sets the registered id; the file's own value ids, such as `default` Glow id 10, are separate data). Still **LIVE-ONLY (one read, already in G4·AUDIT)**: the two keys `pipeline+0x4028` and `pipeline+0x4090` (area, id) at a world frame, and the `49D00` calls for kind 6 (area, id). Expected: id 4 and a map area code from rule 2 (for `m30_00`: b3 and b2 of the map id).

Native consequence (PRODUCERS-UNION §3.3 "Native"): implement the registration with the kind table and the per-area rule above, and the lookup with the four steps; do not key a Glow read to `mapbase` by file name.
