# I-0068 part G: effects phase 1 ready to code: publish `GXFfxSceneCtrl`, run E06 lights

Request (coordinator, 2026-10-10, step 3):

- an ordered list of the original functions to **call** or **transcribe**, to publish the scene
  control and run the effect lights;
- the minimal memory layouts the native code must fill.

Sources:

- STATIC decompilation of every function named below, with the GX globals each one touches
  (`evidence/I-0068/queries/ffx/phase1/`);
- the I-0066 native bootstrap (`native/frame/game_bootstrap.cpp`, commit `5138e013`), which already
  publishes `SprjSfxImp` with its CPU members only.

Labels: STATIC, INFERRED.

**Rule of the list.** A function is marked:

- **CALL** when it touches no GX object (`[144961AB0]`, its job pool `[144961BB0]`, its contexts);
- **TRANSCRIBE** when it does, and only the GX statements are replaced;
- **NATIVE** when the object must be the native one (a contract implemented natively).

Allocations go through the engine heap `[14479C270]` (`FUN_141777A00(size, align, heap)`), as in
the original.

## 1. Starting point (I-0066, STATIC)

- `PublishNativeSfxManager` already writes `E6C9D0`'s CPU stores in order into the native
  `SprjSfxImp` (vtable `2992EF8`, 0xF0) at `[144799C98]`.
- It leaves `+0x38..+0x50`, `+0x58`, `+0xD8`, `+0x60` and `+0xE0` null (`+0x68`, the stop table, built since I-0141 tranche 13d).
- `E6EA10`/`E6EC60` are native and refuse when `+0x58` is set. **Phase 1 changes that** (§2 step 9).
- Effect point lights need no new consumer: `CollectNativePointLights` already reads
  `[[GXRS]+0x41D0]+0x10`, the vector the point-light host fills (EFFECT-LIGHTS §2.5).

## 2. Construction, in `E6C9D0`'s order

| # | What | Original | Mode | Notes |
|---|---|---|---|---|
| 1 | shader container `SprjFfxShaderContainer` (0x30) → `SprjSfxImp+0x38` | `FUN_140D53340` | CALL | no GX global (calls `FUN_1400ACA70`) |
| 2 | texture repository (8) → `+0x40` | `FUN_140D53270` | CALL | — |
| 3 | FLVER container (0x58) → `+0x48` | `FUN_140D53100` | CALL | (`FUN_140256BC0`) |
| 4 | sound manager (8) → `+0x50` | `FUN_140D538A0` | CALL | — |
| 5 | **scene control** (0x6C0, align 16) → `+0x58` | `FUN_14024B260(sc, heap, 0x200000, 0, …)` | **TRANSCRIBE** | §3 |
| 6 | shader set → `sc+0xE0` and `[sc+0x28]+0x168` | `FUN_14024C300` → `FUN_140291520(*[GX], …)` | **skip in phase 1** (both stay 0) | it creates GPU programmes; phase 2 |
| 7 | texture handler → `[sc+0x28]+0x158` | `FUN_14024C390(sc, textures)` | CALL | CPU (0x18 record) |
| 8 | FLVER handler → `[sc+0x28]+0x170` | `FUN_14024C3A0(sc, flvers)` | CALL | one store |
| 9 | GX callbacks `[144962380/388/390]` = `E6B950/E6FC40/E6FBF0` | `E6C9D0` stores | CALL (stores) | only GX code reads them |
| 10 | **device initialisation** | `FUN_14024C530(sc)` | **TRANSCRIBE** | §4 |
| 11 | `sc+0xB0 = sound manager`, publish `SprjSfxImp+0x58 = sc` | `E6C9D0` stores | stores | `[1449623C0] = sc` is written by step 5 |
| 12 | view attach/detach | native `E6EA10`/`E6EC60` | **NATIVE** | call the §6 attach instead of refusing |

- The rest of `E6C9D0` (`FUN_1400C2400` view draw list `+0x60`, `FUN_140D648A0`,
  `FUN_140D52CA0`, `FUN_140E67CB0`, debug menu `E6F630`) stays as I-0066 decided. Phase 1 does not
  need it (INFERRED: none of the update paths of §5 reads them). `FUN_141AB0150/05E0`, the stop table `+0x68`, is built since
  I-0141 tranche 13d: the entry update `E6E750 -> E6B210 -> E6B3C0` reads `+0x68`.

## 3. Scene control constructor `FUN_14024B260`: transcription (STATIC)

**Keep every statement except the GX ones, in order:**

1. Vtables `142959838` → **`143D64008`** (keep it: engine code calls its slots and its
   non-virtual methods read this layout).
2. CPU fields `+0x08..+0x88`:
   - `+0x10` heap;
   - `+0x20` = 1 (word);
   - `+0x30..+0x40` = −1;
   - `+0x50/+0x58/+0x60 = +0x50` (list);
   - `+0x88` = 1.
3. Heap check `vt+0x18` bit 5.
4. `+0x90..+0xA0` = 0, `+0xA8` heap, `+0xB0` = `14473F7F0`, `+0xB8` = **0x200000** (ring size),
   `+0xC0..+0x108` = 0.
5. **`FUN_140266BB0(sc+0x110, heap)`**: the light/entity container (CALL).
6. The two `GXRenderToTexture` shells: `+0x150` and `+0x1B0` (vtable `143D4BAA0`, heap at
   `+0x158`/`+0x1B8`). Fields only; they stay empty (FFX-FAMILIES-PHASE2 §6.2).
7. `+0x1D0..+0x248` = 0.
8. Identity matrix at `+0x250..+0x28F` (from `143D1BF90..`).
9. `FUN_14006E780(sc+0x2A0)`.
10. `+0x580` = `param_5`, `+0x588` = `param_4`, or the heap when 0.
11. `FUN_140253150(sc+0x638)`.
12. List `+0x660` (`FUN_140253390`).
13. Vector `+0x678` (`FUN_1402533F0`, 16 entries).
14. `+0x698` = 7, `+0x6A0` = 8, `+0x6A8` = 1.0.
15. `DAT_14496237C` = 0.
16. `FUN_14025E7F0([144929E28])`.
17. **`[1449623C0] = sc`.**
18. ~~Five `FUN_1419254F0(*[GX], …)`~~ (constant buffers `+0x100` 0xE0, `+0xE8` 0x1C0, `+0xF0` 0x1C0,
    `+0xF8` 0x230, `+0x108` 0x190). **Phase 1: leave the five members null.** Their only reader is the
    draw commit (draw-context slot 27, FFX-FAMILIES-PHASE2 §6.1), which does not run.
19. Flags `+0x5B8..+0x5E4`:
    - `+0x5D0` = 1.0, `+0x5E0` = 1 (word), `+0x5B8` = 1;
    - second identity at `+0x5F0..+0x62F`;
    - `+0x630` = 10.0.
20. List cleanup (`FUN_140253330`, `FUN_1402541C0`, `FUN_141A879F0`).
21. `+0x5C2` = 0, `+0x5C8` = 1, `+0x658` = 0, `+0x650` = −1, `+0x6B0` = 0, `+0x5E4` = 1, `+0x654` = 0.
22. `+0x70 = sc`.
23. **Pass ids:** `+0x3C` = 0x20, `+0x30` = 0x21, `+0x38` = 0x2D, `+0x40` = 0x31, `+0x34` = 0x29.
24. **Type registration**, guarded by `+0x44` (CALL every function as is):
    - `FUN_141A97EE0(heap, heap)`, `FUN_14036D9A0`;
    - then for each type `FUN_140253710(registry, desc, heap)` and, when it returns true,
      `FUN_141AB3A60(desc', heap)`;
    - types 10000, 0x2712, 0x2713, 0x27D8, 0x283C, 0x2718, 0x2968, 0x283D, 0x283E, 0x25D,
      0x271C, **0x261 (point light, factory `FUN_14028F200`)**, **11000 (spot, `FUN_14028F290`)**,
      0x2AFA (line, `FUN_14028F320`);
    - then `FUN_140253990(&144965290, …)` for property types 0x4B and 0x51;
    - then `FUN_14036DB30`, `FUN_141A98500` (seals), and `+0x44 = 1`.
    - The descriptor tables are the function-pointer sets in the decompile (`14024A840…`); copy
      them exactly.
25. **Resource manager:** `FUN_14028F4D0(alloc 0x198, deviceToken, heap, sc)` → `+0x28`.
    - CALL with a **native device token** in place of `*[144961AB0]`.
    - The constructor only stores it (`+0x08`, `+0x178`) and builds its 0xB8 helper
      `FUN_1402B83D0` (CPU).
    - Its GPU slot 1 (`FUN_14036FD70`) must not run in phase 1: patch the slot natively to a no-op
      that returns success, or keep FXR resources without GPU side (INFERRED: lights need none).
26. `FUN_140255090(heap)`; a 0x90 object `FUN_14025B280` + `FUN_14025B370(obj, heap)` (CALL).

## 4. Device initialisation `FUN_14024C530`: transcription (STATIC)

1. ~~`FUN_1421392C0(sc+0xC0, *[GX], 0x200000, 0x10)`~~ (GPU ring pool) and its per-context
   sub-allocators `sc+0xD0[i]`, `FUN_142139450`.
   - **Phase 1:** `sc+0xC0 = sc+0xC8 = 0`, `sc+0xD8` = 1 (one native context), and `sc+0xD0` = one
     null slot.
   - INFERRED safe: only the draw-side items read the ring (§5).
2. **`GXFfxManager`:**
   - alloc 0x28; `FUN_142155140(mgr)` (CALL; base vtable `143D6A0D8`);
   - `*mgr = 143D6A078`, `mgr[3]` = **native device token**, `mgr[4]` = sc;
   - `sc+0x18 = mgr`;
   - **`FUN_142155700(mgr, 0x1E, 3)`** (CALL: library manager `FUN_141ACC260`, 0x60, and its init
     `FUN_141ACD590`; no GX access).
3. **Scene entity** `sc+0x8`:
   - the 0x78 object built at the end of `C530`: vtables `143D63F90`/`143D63FC0`, `[0xE] = sc`, mask
     words, `+0x20 = 0x100000011`; CALL-equivalent stores.
   - It stays unattached to any scene in phase 1 (`+0x18` null, so no `vt+0x28` call).

## 5. Per frame: SceneCtrl slot 8 (`vt+0x40`, called by `SprjSfxImp::Update` `E6E750`): transcription of `FUN_14024D000`, single-threaded branch

Keep, in order (CALL each named function):

1. **Environment.**
   - Read `[GXRS+0x5360]+0x48..+0x64` and `[GXRS+0x5368]` (`+0x140/+0x1A0/+0x1B8/+0x1D0` factors).
   - Call `FUN_141ACAFC0`, `FUN_141ACAE60`, `FUN_141ACAEA0`, `…EE0`, `…F20`, `…F60` (library wind and
     global factors).
   - GXRS is `[144961B50]`, present natively.
2. **Resource tick:** `FUN_1402B7650([sc+0x28]+0x160, [sc+0x18]+8)`.
3. **Hide mask:** `+0x14C = +0x148`, `+0x148 = 0`.
4. **Sound:** when `sc+0x248` is set, `FUN_140255FD0`, `FUN_140255DF0`.
5. **Time scale and controllers:**
   - `dt *= sc+0x5D0`;
   - controllers of `[144962398]` (`vt+0x50(dt)`) and `FUN_14025B870([1449623A0], dt)` (**correction, part BM: CALL only while no node exists; with any node it dispatches a GX job through `[144961BB0]` / `[144961AB0]+0x2C8`, so it is transcribed inline, [FFX-WAVE1-READY](../I-0066-independent-native-startup/FFX-WAVE1-READY.md) §2.3**);
   - `dt = 0` when `DAT_14496237E`.
6. **When `sc+0x20` is set:**
   - `+0x5A0 = +0x598`;
   - the GX flush `FUN_14000B4E0` (when `+0x654 > 0`) becomes a no-op that clears `+0x654`.
7. **Library update with a native update pass** (replaces `FUN_140296E20` + `FUN_140296690`, which
   read the GX main context `**([GX+0x60])+8` and build `GXFfxDrawContext`s):
   - Build the combined pass with the original **`FUN_140297110`** (CALL; CPU: scratch from the sc
     heap, 0x80000 linear allocator `FUN_1417FCB80/BC0`). Its arguments are:
     - native update and proceed passes;
     - `sc+0xC8` (0 in phase 1);
     - `sc+0x580`, `sc+0x588`, `&sc+0x30`, `sc+0xE0`.
   - Combined-pass contract (vtable `143D6A138`, STATIC):
     - slots 1/2 (begin/end): no-ops;
     - slot 3 `FUN_1402975E0`: enqueues a renderer work item into the update or proceed list;
     - slots 4–6 return 1; slot 7 returns 0.
   - Call **`[sc+0x18] vt+0x18` = `FUN_142155920(mgr, dt, combined)`** (CALL). It sets the pass,
     calls begin, runs `FUN_142155180`, the library update `vt+0x30`, `FUN_142155570`, end, and
     clears the pass.
     - **The instance, emitter, action and host updates happen here.** The point-light host is
       created by factory `FUN_14028F200` (ctor `FUN_1402B2410`) and updated by `FUN_1402B2E30`.
       It pushes its `GXPointLight` through `FUN_140266FC0(sc+0x110)` into `[[GXRS]+0x41D0]+0x10`.
   - **Phase 1: do not execute the two work lists.** The original hands them to GX jobs (proceed list
     `auStack_2E78`, update list `puStack_5BC8`). Each item is a renderer update
     `item->vt+8(item, drawContext, data, 0)` (`FUN_140296D80`, `FUN_140296FF0`): draw-side work
     (vertices, GPU particles).
     - INFERRED: no light depends on them.
     - Destroy the passes as `FUN_140297410`, `FUN_1402969F0`, `FUN_140296F40` do (their list storage).
   - **Native update/proceed pass objects (NATIVE)** need only:
     - the list members the combined pass appends to: update pass `+0x2D30` vector (16-byte items),
       proceed pass `+0x98` table (8-byte heads);
     - their destructors.
     - Minimal layout: §7.
8. The multi-threaded branch (`DAT_144962376 != 0`) is not transcribed. Leave that flag 0.

## 6. View attach / detach (`FUN_14024F2C0` equivalent, NATIVE)

- **Attach:**
  - store the view at `sc+0x218`;
  - read its size the original way (`vt+0x18`, `+0x240/+0x660`, `+0x50/+0x54`);
  - **no** `GXRenderToTexture` targets;
  - **no** registration in `[GX]+0xB18`.
- **Detach:** clear `+0x218`.
- **Resize:** nothing in phase 1.

## 7. Minimal memory layouts the native code fills

**`GXFfxSceneCtrl`** (0x6C0, align 16): every field of §3 and §4. The CPU paths of phase 1 read:

| Offset | Content |
|---|---|
| `+0x00` | vtable `143D64008` |
| `+0x08` | scene entity (§4.3) |
| `+0x10` | heap |
| `+0x18` | `GXFfxManager` |
| `+0x20` | update-enabled byte (copied each frame from `SprjSfxImp+0x72`) |
| `+0x28` | resource manager (0x198) |
| `+0x30..+0x40` | pass ids |
| `+0x44` | types registered |
| `+0x50..+0x68` | list |
| `+0x70` | self |
| `+0xB0` | sound manager |
| `+0xB8` | ring size |
| `+0xC0..+0xD8` | ring (null), context count 1 |
| `+0xE0` | shader set (null in phase 1) |
| `+0xE8..+0x108` | constant buffers (null in phase 1) |
| `+0x110` | light/entity container (`FUN_140266BB0`) |
| `+0x148/+0x14C` | hide mask |
| `+0x218` | view |
| `+0x248` | sound object (or 0) |
| `+0x580/+0x588` | update parameters |
| `+0x598/+0x5A0` | entity list heads |
| `+0x5B8..+0x5E4` | flags |
| `+0x5D0` | time scale |
| `+0x654` | flush counter |

**`GXFfxManager`** (0x28):

| Slot | Content |
|---|---|
| `[0]` | vtable `143D6A078` |
| `[1]` | library manager (`FUN_141ACC260`, 0x60) |
| `[2]` | the object returned by `vt+0x48` during `FUN_142155700` (library side; not decoded) |
| `[3]` | **native device token** |
| `[4]` | scene control |

**Resource manager** (0x198, `FUN_14028F4D0`):

- `+0x08` = `+0x178` = device token; `+0x150` heap; `+0x158` texture handler; `+0x160` FXR
  cache; `+0x168` shader set (0); `+0x170` FLVER handler;
- slot 1 (GPU resource creation) is no-op in phase 1.

**Native update pass** (replaces `GXFfxUpdateDrawPass`, original 0x2D50+ bytes):

- vtable with the slots the combined pass and teardown use (dtor, begin/end no-ops);
- a 16-byte item vector at `+0x2D30` (begin/end/capacity) that receives the items and is never
  executed in phase 1.

**Native proceed pass** (replaces `GXFfxProceedUpdateDrawPass`): the same, with its item table at
`+0x98`.

**Device token:** any non-null native object. INFERRED: in phase 1 nothing dereferences it
(stored only). The checks below catch a dereference.

## 8. Verification (EFFECT-LIGHTS §5, read-only)

| Check | Expectation |
|---|---|
| R1 | `[[144799C98]+0x58]` non-null and equal to `[1449623C0]` |
| R2 | registries sealed (`DAT_1449651E8`, `[144933088]+0x20`); type 0x261 present |
| R4 | `sc+0x14C` swaps every frame; the library update runs once per frame |
| R6 | point-light host count grows when a witness effect starts (m40 bonfire) |
| R7 | `[[GXRS]+0x41D0]+0x10` reaches **163** entries at the m40 witness (59 map + 104 effect), with the effect families of EFFECT-LIGHTS §3; `CollectNativePointLights` draws them with no new code |
| Crash guard | any access to `[144961AB0]` from the paths above is a transcription miss: the list in §3–§5 names every GX statement found statically |

## 9. Residual risks (INFERRED, to watch in the first run)

1. A CPU path that reads `sc+0xE0` (shader set) during effect creation (renderer selection).
   - Mitigation: if R6 stays 0, build the shader set CPU-side only: `FUN_140291520` without
     programme creation (phase 2 work).
2. The combined pass slot 3 dereferencing the null ring `[pass+8]` when an emitter enqueues work.
   - Mitigation: a native ring object with the allocator interface (`vt+0x30` size,
     `vt+0x50` alloc) backed by heap memory.
3. Spot (11000) lights take the map-light path (`FUN_140018380`, `[GXRS+0x41D0]`), which is
   native too. Line lights (0x2AFA) are not read.

## Exports

`decompiled/DS3-1152-F-<RVA>`: `00E6C9D0`, `0024B260` (existing), `0024C530`, `0024D000`,
`00296E20`, `00296690`, `00297110`, `0024CE30`, `0028F4D0`, `02155140`, `02155700`, `02155920`,
`00266FC0`, `00266BB0`, `00D53340`, `00D53270`, `00D53100`, `00D538A0`, `0024C300`, `0024C390`,
`0024C3A0`, `00296D80`, `00296FF0`, `002969F0`, `00296F40`, `002975E0`.
