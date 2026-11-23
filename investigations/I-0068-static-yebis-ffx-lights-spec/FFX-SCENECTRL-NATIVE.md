# I-0068 part C: a native `GXFfxSceneCtrl`, the GX effect system behind `[SprjSfxImp+0x58]`

Request (coordinator, 2026-10-10). Natively `SprjSfxImp+0x58` is published as null
(`game_bootstrap.cpp`, native SFX manager 930–938), so no effect exists and the effect lights are
missing ([EFFECT-LIGHTS](EFFECT-LIGHTS.md)). Write the specification for creating and
publishing a native `GXFfxSceneCtrl`:

- the original constructor and its dependencies;
- its GPU resources;
- the FFX rendering paths, including the four direct D3D11 functions;
- what the native backend must provide.

Static reading only (private read-only database copy). Labels: STATIC, INFERRED.

## 1. Answer first

`GXFfxSceneCtrl` is the **GX adapter of the FFX middleware**. Like YEBIS (part A), the effects
split into two layers:

| Layer | Code | Natively |
|---|---|---|
| FFX library (CPU) | `141A8xxxx..14215xxxx`: `FXManager`, FXR resources, instance and emitter update, property curves, draw-entity host creation | **keep unchanged** |
| GX adapter (`GXFFX::`, `1402xxxxx..1403Axxxx`) | `GXFfxSceneCtrl`, `GXFfxManager`, `GXFfxGraphicsResourceManager`, update/draw passes, `GXFfxDrawContext`, appearance renderers, light hosts | CPU parts kept; **GPU parts replaced by native code** |

The scene control is anchored in GX in five ways, and natively each is absent or replaced:

1. **The GX display/device object `[4961AB0]`:**
   - five constant buffers;
   - a GPU ring pool;
   - the shader set, the resource manager and the half-resolution targets;
   - the GX contexts and job queue used by the update.
2. **The GX root `[4961B50]`:** environment and wind parameters at `+0x5360`/`+0x5368`, and the
   light list `+0x41D0` used by the light hosts. Natively the root layer exists (I-0066).
3. **The GX scene graph:** a `TGXSgEntity<GXFfxSceneCtrl>` at `+0x8` is how passes reach
   `Draw`.
4. **The display listener list `[4961AB0]+0xB18`:** resize.
5. **The attached view `+0x218`:** `SprjSfxImp` view attach `E6EA10` → `FUN_14024F2C0`, already
   native on the I-0066 branch.

**Native plan in two phases.** Under D-0161 a phase is a family, not an effect.

- **Phase 1, effect lights and every CPU effect behaviour (no draw):**
  - publish a `GXFfxSceneCtrl`-layout object (0x6C0) at `[SprjSfxImp+0x58]` and `[1449623C0]`;
  - run the engine's own CPU code on it: type registration, effect creation, FXR resolution,
    library update, light hosts;
  - replace the GPU members with native ones, or null where only drawing uses them;
  - drive the update from `SprjSfxImp::Update` as the original does (§3.1).
  - The effect lights then appear without any native draw code: the hosts push into
    `[GXRS+0x41D0]`, which `CollectNativePointLights` already reads.
- **Phase 2, effect drawing, by appearance family** (E01–E10 of I-0063):
  - a native implementation of the GX draw path (§3.2): the draw passes, `GXFfxDrawContext`,
    appearance renderers, the four direct D3D11 sites;
  - natively recorded command-list work in the GX pass ids the scene control already declares.

## 2. Construction (STATIC)

### 2.1 Where and in which order (`SprjSfxImp` ctor `FUN_140E6C9D0`)

1. **Resource containers** (game → GX), all on the heap `[14479C270]`:
   - `+0x38` `SprjFfxShaderContainer` (`FUN_140D53340`);
   - `+0x40` `SprjFfxGxTextureRepository` (`FUN_140D53270`);
   - `+0x48` `SprjFfxFlverContainer` (`FUN_140D53100`);
   - `+0x50` `SprjFfxSoundManager` (`FUN_140D538A0`).
2. **`GXFfxSceneCtrl`:** `FUN_141777A00(0x6C0, 0x10)`, then
   **`FUN_14024B260(this, heap [14479C270], 0x200000, 0x80000, 1)`**. It is stored at
   **`+0x58`**.
3. **Wiring:**
   - `FUN_14024C300(sc, shaders)`: shader set (below);
   - `FUN_14024C390(sc, textures)`: resource manager `+0x158`;
   - `FUN_14024C3A0(sc, flvers)`: resource manager `+0x170`.
4. GX callbacks `[144962380] = FUN_140E6B950`, `[144962388] = FUN_140E6FC40`,
   `[144962390] = FUN_140E6FBF0` (only GX code reads them).
5. **`FUN_14024C530(sc)`** (device initialisation, below), then `sc+0xB0 = sound manager`.

### 2.2 The constructor `FUN_14024B260` (0x6C0 bytes)

**CPU fields:**

| Offset | Content |
|---|---|
| `+0x00` | vtable `143D64008` (base `GXBS::GXSystemMessageListener` `142959838`) |
| `+0x10` | heap (`[14479C270]`) |
| `+0x18` | `GXFfxManager` (set by `FUN_14024C530`) |
| `+0x20` | update-enabled byte (`SprjSfxImp+0x72` copied in each frame) |
| `+0x28` | `GXFfxGraphicsResourceManager` (0x198, ctor `FUN_14028F4D0`) |
| `+0x30/+0x34/+0x38/+0x3C/+0x40` | **GX pass ids 0x21, 0x29, 0x2D, 0x20, 0x31** (the passes effect draws go to; 0x29 is the forward pass) |
| `+0x44` | "types registered" byte |
| `+0x50..+0x68` | lists (`+0x60` points to `+0x50`) |
| `+0x110` | light/entity container (`FUN_140266BB0`); INFERRED: the `container+0x110` through which the point-light host creates its light (`FUN_140266FC0`), the host receiving the scene control as its container |
| `+0x148` / `+0x14C` | hide mask, double-buffered (EFFECT-LIGHTS §2.4) |
| `+0x150`, `+0x1B0` | two `GXBS::GXRenderToTexture` (vtable `143D4BAA0`), sized at view attach |
| `+0x210` | view HDR flag |
| `+0x218` | attached view |
| `+0x220..+0x240` | GX textures |
| `+0x250..+0x290`, `+0x2A0` | constants (identity matrix from `143D1BF90..`) |
| `+0x580`, `+0x588`, `+0x598..` | update-pass parameters, draw-entity lists |
| `+0x5B8..+0x5E4` | flags; `+0x5D0` time scale 1.0; `+0x5E0`/`+0x590` cleared by `SprjSfxImp` |
| `+0x630` | 10.0 |

- **FXR action type registration**, once, guarded by `+0x44`: GX registry `[1449651D0]` +
  FFX library registry `[144933088]`, then `FUN_14036DB30` and `FUN_141A98500` seal both
  (MENUS-MAP §14).
  - Types: `600`, `0x25E` and the other `FUN_14036D9A0` blocks, then 10000, 10002, 10003,
    10200, 10300, 10008, 10600, 10301, 10302, 605 (`0x25D`), 10012 (`0x271C`), **609 (`0x261`,
    point light)**, **11000 (spot)**, **11002 (line)**, and two property types (`0x4B`, `0x51`)
    in `[144965290]`.
  - The I-0063 map gives the families (`Build-Families.ps1`): E01 billboards 600/603/604; E02
    lines and trails 601/602/606/10012; E03 model particles 605/10015; E04 distortion 607; E05
    radial blur 608; **E06 lights 609/11000**; E07 GPU particles 10000/10001/10008/10009; E08
    light shafts 10003; E09 water interaction 10013; E10 lens flares 10014.
- `[1449623C0] = this` (singleton).
- **GPU** (each through GX's buffer helper `FUN_1419254F0(*[4961AB0], dst, desc, init)` →
  `FUN_141926720`): five constant buffers.

| Member | Size |
|---|---|
| `+0x100` | 0xE0 |
| `+0xE8` | 0x1C0 |
| `+0xF0` | 0x1C0 |
| `+0xF8` | 0x230 |
| `+0x108` | 0x190 |

- **Resource manager:** `FUN_14028F4D0(alloc 0x198, *[4961AB0], heap, this)`:
  - vtable `143D66198`;
  - stores the GX device at `+0x8` and `+0x178`;
  - creates a 0xB8 helper (`FUN_1402B83D0`);
  - `+0x160` is the FXR resource cache used by `FUN_14024ECF0` at load (CPU).
- Then `FUN_140255090(heap)` and a 0x90 helper (`FUN_14025B280`/`FUN_14025B370`), CPU.

### 2.3 Device initialisation `FUN_14024C530(sc)`

1. **GPU ring pool:** `FUN_1421392C0(sc+0xC0, *[4961AB0], size = sc+0xB8 = 0x200000, 0x10)`.
   - `sc+0xC8 = FUN_14213B500(pool)`;
   - per GX context (`[4961AB0]+0x18` count), a 0x40 sub-allocator `FUN_142139450` in the array
     `sc+0xD0` / count `sc+0xD8`.
   - INFERRED: the dynamic vertex and constant memory of effect draws and GPU particle updates.
2. **`GXFfxManager`** (0x28, vtable `143D6A078`, base `FFX::FXManagerWrapper`,
   `FUN_142155140`):
   - `[3] = *[4961AB0]` (GX device), `[4] = sc`;
   - `FUN_142155700(mgr, 0x1E, 3)` (library manager initialisation);
   - stored at **`sc+0x18`**.
3. **Scene-graph entity:** a 0x80 `TGXSgEntity<GXFfxSceneCtrl>` (vtable `143D63F90`, base
   `GXSG::GXSgEntity`) at **`sc+0x8`**:
   - visibility mask **`{0x11, 1}`** at `+0x20` (the same default as the effect create-info);
   - `+0x70` points back to the scene control.

### 2.4 Wiring helpers

- **`FUN_14024C300(sc, shaders)`:** `sc+0xE0 = FUN_140291520(*[4961AB0], sc+0x10, shaders)`, the
  **FFX shader set** (2855 lines; builds the GX programmes of the FFX appearances from the shader
  container). Also stored in the resource manager at `+0x168`.
- **`FUN_14024C390`:** resource manager `+0x158 = {heap +0x150, texture repository, 0}`.
- **`FUN_14024C3A0`:** resource manager `+0x170` = flver container.

## 3. Per-frame and event paths (STATIC)

### 3.1 Update: `SfxStep` → `SprjSfxImp::Update` `FUN_140E6E750` → SceneCtrl slot 8 `FUN_14024D000(sc, dt)`

`E6E750` sets `sc+0x20 = SprjSfxImp+0x72`, then calls `vt+0x40` **only when `+0x58 != 0`**.

`FUN_14024D000` does the following, in order:

1. **Environment.** Reads `[GXRS+0x5360]+0x48..+0x60` and `[GXRS+0x5368]+0x140/+0x1A0/+0x1B8/+0x1D0`
   and passes wind and global factors to the library (`FUN_141ACAFC0`, `141ACAE60..141ACAF60`).
2. **Resource-manager tick** `FUN_1402B7650([sc+0x28]+0x160, [sc+0x18]+8)`.
3. **Hide-mask swap** `+0x14C = +0x148`, `+0x148 = 0`.
4. **Sound manager tick** (`sc+0x248`).
5. Appearance controllers of `[144962398]` (`vt+0x50(dt)`), `FUN_14025B870([1449623A0], dt)`.
6. **When `sc+0x20` is set:**
   - flush GX (`FUN_14000B4E0([4961AB0])`) when `+0x654 > 0`;
   - build the update passes:
     - **`GXFfxUpdateDrawPass`** `FUN_140296E20` (vtable `143D6A198`): GX main context
       `**([4961AB0]+0x60)+8`, a `GXFfxDrawContext` `FUN_14025FB80`;
     - **`GXFfxProceedUpdateDrawPass`** `FUN_140296690` (vtable `143D6A180`): a 0x40000-byte
       scratch ring;
     - `FUN_140297110` combines them;
   - **library update:** `GXFfxManager` slot 3 `FUN_142155920(mgr, dt, pass)` →
     `FUN_142155180`, `FXManager vt+0x30(…)`, `FUN_142155570`;
   - the instance and emitter work is split into **jobs**: `_GXFfxManagerUpdateJob` (`143D63FE8`,
     slot 1 `FUN_14024CE30` → `FUN_141AB1680(instance, dt, ctx)`) and
     `_GXFfxEntityContextUpdateJob` (`143D63FD0`), dispatched on GX's job queue
     (`[4961BB0]` pool, `[4961AB0]+0x2C8`; single-threaded unless `[4961AB0]+0x328`);
   - per-GX-context draw contexts (`[4961AB0]+0x2D0` count) for the GPU-side update of GPU
     particles (E07).
   - **Correction (part M, STATIC):** the GPU simulation of E07 does not run here. It runs at
     draw time, in the pass-0x45 flag-0x10 submission (`FUN_1402695E0` → `FUN_1402A6A30`); see
     [EFFECTS-E07-GPU-PARTICLES-NATIVE §1](EFFECTS-E07-GPU-PARTICLES-NATIVE.md).

**Inside the library update, CPU only** (`FUN_141AB1680` → emitters `FUN_141ACA640` → action
nodes):

- the draw-entity **host factories** (registry `+0x30`) are called;
- the hosts update every frame (point light: slot 2 `FUN_1402B2E30`, writes the
  `GXPointLight`).

### 3.2 Draw: GX scene graph → entity `sc+0x8` → SceneCtrl slot 10 `FUN_14024E790(sc, passCtx)`

- `TGXSgEntity<GXFfxSceneCtrl>` slot 1 `FUN_1402517B0` = `JMP [[+0x70]]+0x50`: **the scene
  graph calls `Draw` for each pass it runs.** Slot 2 → SceneCtrl slot 11 (`RET`, no-op).
- **`FUN_14024E790`:**
  - skips when `[[4961AB0]]+0x3E1`, `sc+0x5BE`, `!sc+0x20`, or pass id `passCtx+0x124 == 0x3A`;
  - pass **0x45**: an extra submission with flag 0x10;
  - every pass: a stack `GXFfxDrawPass` (vtable `143D6A1F8`; `{passCtx, sc, sc+0x30..+0x40 pass
    ids, flag = (pass == 0x39)}`) handed to **`GXFfxManager` slot 4** (library draw), which visits
    each visible draw entity.
  - With `sc+0x590` set (multi mode), a direct list walk instead. For each entity it filters with
    the hide mask and the pass masks `passCtx+0x4B8/+0x4BC` against `owner+0x1FC/+0x200`, then
    maps the entity kind to a GX pass: kind 1 → 0x21, 5 → 0x2D, 6 → 0x29 or 0x31, others 0x29.
- **`GXFfxDrawPass` slot 4 `FUN_1402997E0` (per draw entity):**
  - visibility (`passCtx+0x418` frustum, `passCtx+0x410` bounds), distance;
  - adds a **GX pass command** `FUN_1400BF4C0(passCtx, 4, 0x18)` whose callback is
    **`FUN_140299E40`**;
  - sort key through `FUN_14020E570([passCtx+0x4B0]+0x38, 0x80, 4)`.
- **At command execution, `FUN_140299E40`:**
  - builds a `GXFfxDrawContext` (vtable `143D65688`, 29 slots);
  - calls the library draw of the entity.
  - The appearance renderer then sets states and constants through the draw-context slots 13–20
    (`FUN_140260830..FUN_140262A70`; `262A70` uses GX's texture-desc helper `141924730` and
    `141920280`) and draws.
- **The four direct D3D11 sites** (MENUS-MAP §38) belong here:

| Site | Role | Calls (STATIC) |
|---|---|---|
| `FUN_14036FD70` (resource manager slot 1) → `FUN_14036F9E0` | creates an FFX GPU resource for an FXR resource. Its records: `n²·0x780 + 0x78` bytes from the heap; texture names `GXFfxPostBumpTexture`, `GXFfxPostTexture`, `GXFfxBumpTexture`, `GXFfxTexture`, `GXFfxSelectedStateBufferTexture` | GX buffer and texture helpers on `[4961AB0]` |
| `FUN_140385F70` (table `143D7E6C0`) | large appearance geometry builder | `IASetPrimitiveTopology` (GX code 10 through `FUN_141929BA0`), `Draw`; `IASetIndexBuffer(R16_UINT)` + `DrawIndexed` |
| `FUN_140396F30` (table `143D7E840`) | appearance geometry builder | topology GX code 4, `Draw` |
| `FUN_14039E900` (table `143D7E900`) | appearance draw through an interface (`vt+0x28..+0xD8`) | `Draw` |

  The constant layouts of these three are not read.

### 3.3 Resize and view

- **View attach** `FUN_14024F2C0(sc, view)`, from `SprjSfxImp` `E6EA10`, native on the I-0066
  branch:
  - registers `sc` in the display listener list `[4961AB0]+0xB18` (`FUN_14000CE70`);
  - stores the view at `+0x218`;
  - reads the view size (`vt+0x18`, `+0x240`/`+0x660`, `+0x50/+0x54`);
  - creates **`+0x150` and `+0x1B0` at half size**, format **R8G8B8A8_UNORM** (`0x1C`) when the
    view flag `+0x74` is set, else **R16G16B16A16_FLOAT** (`0x0A`) (`FUN_1400DD0B0`);
  - creates two GX textures `+0x238/+0x240` (vtable `143D1D638`, half size, `0x1C`, 1 mip,
    flags 0x200).
  - No reader anywhere (FFX-FAMILIES-PHASE2 §6.2, STATIC negative search). The earlier INFERRED
    role is withdrawn.
- **Detach** `E6EC60` → `FUN_14024F2C0(sc, 0)`, and `FUN_14024F1D0` (unregisters, frees the
  targets).
- **Resize** arrives as listener slot 2 `FUN_14024F630(sc, w, h)` and rebuilds the same targets.

### 3.4 Shutdown

SceneCtrl slot 7 `FUN_14024C830`:

- clears `+0x218`;
- frees the targets;
- releases the per-context sub-allocators and the ring.
- The destructor is slot 0 `FUN_14024C220`.

## 4. What the native backend must provide

### 4.1 Phase 1: effects run, lights appear (no draw)

1. **Object.** Allocate 0x6C0 from the heap `[14479C270]`, 16-aligned.
   - Initialise the CPU fields exactly as `FUN_14024B260` does (§2.2 table; transcribe the field
     writes, not the GPU calls).
   - Publish it at `[SprjSfxImp+0x58]` and `[1449623C0]`.
   - Keep vtable `143D64008`, because engine code calls its slots (`vt+0x40` update, the
     listener slots) and its non-virtual methods (`24ED90`, `24EFE0`, `24F0F0`, `24ECF0`,
     `24F2C0`) read this layout.
2. **Type registration.** Run the registration block of `FUN_14024B260`: it is CPU only, the
   registry lookups and seals of MENUS-MAP §14. Without it, FXR actions are dropped at load.
3. **Resource manager** (`+0x28`):
   - the engine object `FUN_14028F4D0` with a **native device token** in place of `*[4961AB0]`;
   - its CPU cache `+0x160` resolves FXR resources at load (`FUN_14024ECF0` → `FUN_1402B7CF0`,
     CPU);
   - its slot 1 (`FUN_14036FD70`, GPU resource creation) must be native: a native texture or
     buffer per FFX resource, or a deferred no-op in phase 1.
4. **`GXFfxManager`** (`+0x18`):
   - the engine's CPU construction (`FUN_142155140`, vtable `143D6A078`, `FUN_142155700(mgr,
     0x1E, 3)`), with the native device token at `[3]`.
   - INFERRED: the library manager itself never touches the device; only GX adapter code reads
     `[3]`.
5. **Update** (slot 8, called by `E6E750`). Two options:
   - (a) keep the engine `FUN_14024D000` and provide the GX members it reads: contexts
     `[4961AB0]+0x60/+0x2C8/+0x2D0/+0x328` and the job pool `[4961BB0]`. That means
     re-creating GX objects, **not** the native route;
   - (b) **recommended:** a native `vt+0x40` that transcribes `FUN_14024D000`:
     - steps 1–5 as is (CPU);
     - the library update (`FUN_142155920`) called on the native thread with **native
       implementations of the two update passes** (`GXFfxUpdateDrawPass`,
       `GXFfxProceedUpdateDrawPass`);
     - in phase 1 their GPU calls (GPU-particle simulation, E07) can be empty; the host and
       emitter updates are CPU and run.
     - Jobs: run `FUN_14024CE30`'s loop (`FUN_141AB1680` per instance) inline or on the native
       job system.
6. **Scene entity** (`+0x8`). In phase 1 it can be a CPU object that is not attached to any
   scene. Nothing is drawn.
7. **View attach/detach.** The native `E6EA10`/`E6EC60` must call the native `24F2C0`
   equivalent: store `+0x218` and the sizes, no half-resolution targets (they have no
   reader, FFX-FAMILIES-PHASE2 §6.2), never register in `[4961AB0]+0xB18`.
8. **Verification:** EFFECT-LIGHTS §5.
   - R1 non-null, R2 sealed, R4/R6 counters;
   - R7: 163 lights at the m40 witness, families of EFFECT-LIGHTS §3 present.

### 4.2 Phase 2: effect drawing, by appearance family

1. **Pass hook.** In each native pass whose GX id is one of `sc+0x30..+0x40` (0x20, 0x21, 0x29,
   0x2D, 0x31) and 0x39/0x45, call the library draw through a **native `FXDrawPass`** (the
   contract of `GXFfxDrawPass` slots 0–5).
   - It applies the visibility of `FUN_1402997E0`, the hide and pass masks of
     `FUN_14024E790`, and the kind-to-pass mapping (1 → 0x21, 5 → 0x2D, 6 → 0x29/0x31, other
     → 0x29).
   - It records a native command per entity instead of `FUN_1400BF4C0`.
2. **Draw context.** A native `FXDrawContext` with the contract of `GXFfxDrawContext` slots
   13–20 (state and constant setup per appearance kind).
   - It uses the five constant-buffer layouts (0xE0, 0x1C0, 0x1C0, 0x230, 0x190) as upload-ring
     allocations, and the dynamic geometry ring (0x200000 bytes).
3. **Shader set.** Native PSOs from the FFX programmes named by the shader container (the
   `FUN_140291520` set). INFERRED, to check in the pilot code: the native programme
   repository loaded from the game's shader bundles may already contain them (K-0438 drew the
   game's particles in the host era).
4. **The direct D3D11 sites** (§3.2 table): native FFX resource creation, native geometry
   builders' topology and draws, as PSO topology types and draw calls.
5. ~~Half-resolution targets~~: not needed (no reader, FFX-FAMILIES-PHASE2 §6.2).
6. **Order of families** (D-0161 witnesses):
   1. E01 billboards;
   2. E02 lines and trails;
   3. E07 GPU particles (needs the update passes' GPU part);
   4. E03 model particles (`GXFfxFlverModelRenderer`, SceneCtrl slot 9);
   5. E04 distortion and E05 radial blur (frame-buffer input in t0, FFX-FAMILIES-PHASE2 §6.3);
   6. E08 light shafts;
   7. E09 water interaction;
   8. E10 lens flares.

## 5. Open points

- ~~The constant layouts~~: closed in FFX-FAMILIES-PHASE2 §6.1. Each buffer is uploaded whole from a
  draw-context block whose bytes are the reflection layout. The builders have no private buffers.
- ~~Half-resolution targets~~: no reader (FFX-FAMILIES-PHASE2 §6.2); do not allocate them natively.
- **`FUN_140291520`'s programme list**: the names of the FFX programmes it requests. Read it
  when phase 2 starts.
- Whether `FXManager` itself (library side) ever dereferences the device token: INFERRED no.
  Check with phase 1 (a null-safe token makes a mistake visible).
- `[144962398]` and `[1449623A0]` (appearance controllers updated in step 5): owners not read.

## Exports

`decompiled/DS3-1152-F-0024B260`, `0024C300`, `0024C390`, `0024C3A0`, `0024C530`, `0024C830`,
`0024D000`, `0024E790`, `0024F2C0`, `0024F630`, `0024F1D0`, `0028F4D0`, `0036F9E0`,
`00E6E750`, `00E6EA10`, `00E6EC60`, `00296E20`, `00296690`, `002997E0`, `00299E40`,
`019254F0`, `021392C0`, `02155920`. Query outputs:
`evidence/I-0068/queries/ffx/`.
