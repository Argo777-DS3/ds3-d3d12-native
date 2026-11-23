# I-0068 part B: effect lights (FFX), static diagnosis and runtime checks

Request (coordinator, 2026-10-10): the native scene light list holds 59 lights against 163 in
the original. Write a diagnostic plan by effect family, find statically the owner categories
`owner+0x1F8` and the mask `[owner+0x1F0]+0x14C` (defaults and writers), and turn this into
runtime checks the pilot can run. Inputs: [MENUS-MAP](../I-0067-static-menus-subsystems-map/MENUS-MAP.md)
sections 10, 14, 30 and 38; Reference1152 / Native1153 (`Read-LightState.ps1`, I-0066).
Executable 1.15.2.0; static reading only, on a private read-only copy of the canonical database.

## 1. Answer first

**All 104 lights belong to one cause, upstream of every family filter: natively there is no
GX effect system, so no effect is ever created.** (STATIC + OBSERVED; the runtime check R1 below
confirms it in one read.)

- The engine's SFX service `SprjSfxImp` (`[144799C98]`) keeps its GX effect system,
  `GXFFX::GXFfxSceneCtrl`, at `+0x58`.
- Every effect creation tests that pointer. For example `FUN_140E6DCD0` (SfxIns creation, reached
  by `SprjFD4SingleSfxIns`/`SprjFD4MultiSfxIns`):
  1. it initialises the caller's handle and its create-info (`FUN_140249950`);
  2. **only if `[this+0x58] != 0`** does it create the effect through the scene control
     (`FUN_14024EFE0`) and copy the result back.
  - `FUN_140E6DA30`, `FUN_140E6DBC0` and the update `FUN_140E6E750` follow the same rule
    (I-0066 LEGACY-ROOT-INVENTORY-903 "SFX manager", 930–935).
- The native SFX manager on the I-0066 branch is published **without `+0x58`**, deliberately:
  `native/frame/game_bootstrap.cpp` "Native SFX manager (930-938)": "Omitted, and tested by the
  engine before use: the GX effect system +0x58 (24B260/24C530) …".
- Consequences:
  - no effect instance;
  - no FFX update, so no draw-entity host (`FUN_14028F200` and siblings are reached only
    from the FFX library update);
  - no `GXPointLight` pushed by `FUN_140266FC0`.
- The observation matches exactly:
  - the 59 native lights are the first 59 entries of the original vector, same values;
  - the 104 others sit at indices 59..162, appended after the map lights;
  - **every one of them is missing natively, in every parameter family** (table §3).
  - A filter (hide mask, light mask, distance, culling) would leave the lights in the vector.
    The vector itself is short, so the loss is on the creation side (MENUS-MAP §30 decision
    tree, first row).

The owner category `+0x1F8` and the hide mask `+0x14C` are decoded in §2. **They cannot explain
the loss:** outside in-engine cutscenes the mask is 0.

## 2. Static facts (new or corrected)

### 2.1 Two creation paths, by light kind

| FXR action type | Factory | Host (vtable) | Light creation | Light object |
|---|---|---|---|---|
| `0x261` (609) point | `FUN_14028F200` | `GXFfxDrawEntityHostFrpgPointLight` (`143D6A838`, ctor `FUN_1402B2410`, 0x120) | `FUN_140266FC0(container+0x110)`: allocates from `[[GXRS+0x41D0]+8]`, `FUN_1400EFCB0`, **pushes into `[GXRS+0x41D0]+0x10` directly** under the container lock | `GXPointLight` 0x1B0 (vtable `143D4E808`) |
| `0x2AF8` (11000) spot | `FUN_14028F290` | `GXFfxDrawEntityHostSpotLight` (`143D6A888`, ctor `FUN_1402B4080`, 0xF0) | `FUN_14024ED10` → `FUN_140018380([GXRS+0x41D0], 0)`: allocates 0x330, `FUN_1400F09B0`, registers with `FUN_1400180D0` (the map-light path) | spot light 0x330 (INFERRED vtable `143D4E8E8`) |
| `0x2AFA` (11002) line | `FUN_14028F320` | `GXFfxDrawEntityHostLineLight` (ctor `FUN_1402B5BA0`, 0xD0) | not read | — |

- **Correction to the STATE wording** ("1402B4080 → 14024ED10 → 140018380"): that chain belongs
  to the **spot** host. All 104 missing lights are **point** lights (vtable `3D4E808` in
  Read-LightState). Their path is `FUN_1402B2410` → `FUN_140266FC0`.
- Both hosts take their light parameters from the FXR action record (`[action+0x18]+0x260`,
  `+0x270`), CPU only. Neither touches a device.

### 2.2 The owner object and its defaults

- In the host code, `owner = [[host+8]+0x10]+0x40`, that is the action's FFX instance, then its
  user block.
- That block is the GX **effect create-info**:
  - constructor `FUN_140249950`, copies `FUN_140249E90` / `FUN_14024A320`;
  - it travels with every SfxIns (`FUN_140E6DCD0` builds it at `handle+0x30`; more than 60 game
    owners call `FUN_140249950`, listed in §4).

| Offset | Default (`FUN_140249950`) | Use |
|---|---|---|
| `+0x60..+0x90` | `+0x60` and `+0x84..+0x90` = `1.0`; `+0x64..+0x83` from the 16-byte constant at `143D7FEF0` = (1.0, 1.0, 1.0, 1.0) (read in part AK) | host: colour × intensity, specular scale |
| `+0x1F0` | `0`, then the creating `GXFfxSceneCtrl` | scene control (hide mask, distance reference) |
| **`+0x1F8`** | **`0xFFFFFFFF`** (every category bit set) | category bits tested against the hide mask |
| `+0x1FC` | **`0x11`** | light mask low word (light `+0x60`) and draw-pass mask low (`pass+0x4B8`) |
| `+0x200` | **`1`** | light mask high word (light `+0x64`) and draw-pass mask high (`pass+0x4BC`) |
| `+0x204` / `+0x208` | `0` / `0` | override flag and value for light `+0xAC` |
| `+0x20C` / `+0x20D` | `0` / `0` | override flag and value for light `+0xB1` |
| `+0x210`, `+0x214` | `0.1`, `10.0` | (not traced) |
| `+0x21C`, `+0x220` | `π/4`, `π/4` | (not traced) |
| `+0x420` / `+0x421` | (in the copied part) | force bit 4 of the light mask on or off |

### 2.3 Light mask (corrects MENUS-MAP §30 point 1)

- The point and spot host constructors, and the point host on every update
  (`FUN_1402B2E30`), overwrite:
  - **light `+0x60 = owner+0x1FC`**, with bit 4 set or cleared by `owner+0x420`/`+0x421` or by
    the FXR parameter at record `+0x64`;
  - **light `+0x64 = owner+0x200`**.
- The light base constructor's defaults `0xFFFFFFEF/1` therefore never survive for effect
  lights.
- With the create-info defaults `0x11/1` (or `0x01/1`), the world scene mask `{1, 1}` collects
  them. A menu view (`hi = 2`) does not.

### 2.4 The hide mask `[owner+0x1F0]+0x14C`

- **`[owner+0x1F0]` is `GXFfxSceneCtrl`.** It is the singleton `[1449623C0]` written by its
  constructor `FUN_14024B260`, and also `[SprjSfxImp+0x58]`.
- **The mask is double-buffered:**
  - `+0x148` accumulates during a frame;
  - SceneCtrl slot 8 (`FUN_14024D000`, per frame) copies `+0x148` into `+0x14C`, then clears
    `+0x148`.
- **The only writer that sets bits is `FUN_140E6E720`**: `SprjSfxImp`: `+0x148 |= ~visibleSet`.
  - Its single caller is the in-game map step `FUN_1408F2CA0` (STEP_MoveMap). It calls it only
    when the event-state object `[144791248]` reports `FUN_140CE2790` and `FUN_140CE2730`, and
    not `FUN_140CE27B0`.
  - INFERRED: during an in-engine cutscene (Remo), when the effects outside the cutscene's
    categories are hidden.
  - The other OR/AND stores to a `+0x148` field (`14197A1A0`, `14197B9C0`, `14197E590`,
    `14232AB00`) are in unrelated modules.
- **Test in the host update:** the light is hidden when `owner+0x1F8 & ~mask == 0`.
  - In gameplay the mask is 0. With the default category `0xFFFFFFFF`, nothing is hidden.
  - A category can only be hidden by a cutscene's mask that covers all of the effect's bits.
- The same masks filter the effect **draws**: SceneCtrl slot 10 `FUN_14024E790` requires
  `(owner+0x1F8 & ~mask) != 0`, `pass+0x4B8 & owner+0x1FC` and `pass+0x4BC & owner+0x200`.
- **Distance block `0x2904`:** the reference point is `FUN_141ACD3F0([[SceneCtrl+0x18]+8])+0x30`
  (INFERRED: the camera or listener of the scene control).

### 2.5 What the GX effect system is made of (scope of the native replacement)

`GXFfxSceneCtrl` (0x6C0; ctor `FUN_14024B260`, wired by `FUN_14024C300/C390/C3A0` and
`GXFfxManager` `FUN_14024C530`, all from `SprjSfxImp`'s ctor `FUN_140E6C9D0`).

**CPU parts, needed for effect lights:**

- **FXR action type registration** (guarded by `+0x44`):
  - into the GX registry `[1449651D0]`, sealed by `DAT_1449651E8`;
  - into the FFX library registry `[144933088]`, sealed at `+0x20`.
- **Effect creation**:
  - `FUN_14024ED90` / `FUN_14024EFE0` / `FUN_14024F0F0` create the effects;
  - `FUN_14024ECF0` → `[[mgr+0x28]+0x160]` (`GXFfxGraphicsResourceManager`) resolves FXR
    resources at load (MENUS-MAP §14).
- **Per-frame update** (slot 8 `FUN_14024D000`):
  - the hide-mask swap;
  - `_GXFfxManagerUpdateJob` (vtable `143D63FE8`), whose slot 1 `FUN_14024CE30` runs the FFX
    library update `FUN_141AB1680` per instance (actions run, host factories are called, hosts
    update);
  - `_GXFfxEntityContextUpdateJob` (`143D63FD0`).
- **Light hosts.** CPU only (§2.1).

**GPU parts (not needed for lights):**

- the five constant buffers of the constructor (`FUN_1419254F0(*[144961AB0], …)`; sizes 0xE0,
  0x1C0, 0x1C0, 0x230, 0x190);
- `GXFfxManager`'s buffer (`FUN_1421392C0(…, *[144961AB0], …)`);
- the resource manager's GPU resources (`FUN_14028F4D0(…, *[144961AB0], …)`; `FUN_14036F9E0`,
  MENUS-MAP §38);
- the draw contexts (`GXFfxDrawContext`, built in slot 8) and draw passes (slot 10).

All of them dereference the GX display object `[144961AB0]`, which is absent natively
(LEGACY-ROOT-INVENTORY-903). That is why the native bootstrap omits `+0x58` today.

**Native contract (INFERRED from the above; layout is the pilot's choice):**

- A native effect system at `[SprjSfxImp+0x58]` must provide the CPU parts with the engine's
  own code: registration, creation, resource resolution, per-frame library update and hosts.
- It must give the GPU members native resources, or none, as long as no draw pass runs.
- Effect lights then appear without any draw:
  - the hosts push `GXPointLight`s into the shared vector;
  - the native point-light pass already consumes that vector: `CollectNativePointLights` (I-0066 branch, `game_bootstrap.cpp`, 1054) keeps the point-class entries of `[[GXRS]+0x41D0]+0x10` with `+0xB0` set and `+0xA0 & 1`, so effect point lights need no new native code there.
- Particles, trails and the four direct D3D11 sites of MENUS-MAP §38 (`FUN_14036FD70`,
  `385F70`, `396F30`, `39E900`) come next, through the draw passes.
  - Read only for topology: `385F70` uses `IASetPrimitiveTopology` (GX code 10 through
    `FUN_141929BA0`), `Draw`, `IASetIndexBuffer(R16_UINT)` and `DrawIndexed`; `396F30` uses GX
    code 4 and `Draw`; `39E900` uses `Draw`.
  - Their constant layouts are not read.
- Under D-0161 the unit of work is the appearance family (MENUS-MAP §5), not the effect.

## 3. The 104 lights by parameter family (OBSERVED, Reference1152)

`Get-EffectLightFamilies.ps1` → `evidence/I-0068/queries/ffx/effect-light-families.tsv`. One family
is one FXR light definition instanced N times. The scale varies per instance when the FXR curves
animate it.

| N | Radius | Colour | Strength / specular | Fade distance | Scale | INFERRED source |
|---|---|---|---|---|---|---|
| 85 | 2 | (0.47, 0.31, 0.20) | 1 / 6 | 80 | 1 | one small warm light repeated over the map (x 120..286): candles or similar object SFX |
| 8 | 5 | (1, 0.63, 0.35) | 1 / 3 | 0 | 0.88–0.99 (flicker) | fires (bonfire, braziers) |
| 5 | 3 | (1, 0.67, 0.55) | 3.5 / 3.5 | 0 | 1 | one fire-type effect with several lights, around x 273–285 |
| 2 + 1 | 6 | (1, 0.51, 0.39) | 0.6 / 2 and 1.2 / 5 | 0 | 0.59–0.99 | large flickering fires |
| 1 | 0.5 | (1, 0.63, 0.37) | 3.2 / 3.2 | 0 | 1 | small flame close to the player (pyromancy flame or held torch) |
| 1 | 2.5 | (0.88, 1, 0.96) | 0.3 / 0.1 | 0 | 1 | cold light next to the previous one (same spot, ±0.5 m) |
| 1 | 3.2 | (1, 0.63, 0.35) | 0.6 / 3 | 0 | 0.93 | fire |

All are point lights (type 0x261). There is no spot or line effect light in this scene.

## 4. Diagnostic plan by effect family

**Family = game owner of the effect** (who builds the create-info and calls SfxIns creation).

- Owners found as callers of `FUN_140249950`, with their creation route `FUN_140E6DA30` /
  `E6DBC0` / `E6DCD0` → SceneCtrl `14024F0F0` / `14024EFE0`.
- After the common cause (§1) is fixed, **each family is one witness**: if a family's lights are
  still missing, its owner's CPU system is not running natively.

| Effect family (owner) | Static owners | Expected lights in the m40 witness | Witness check |
|---|---|---|---|
| **Map / object SFX** (map parts, objects, bonfires) | `WorldSfxManImp` (ctor `FUN_14068C7E0`), `ObjMultiSfxSlot` (`FUN_14065AB10`), `ObjBurnSfxSlot`, `ObjRigidOffsetSfxSlot`, `ObjShineTreasureSlot`, bonfire condition `FUN_14045DD90` → `FUN_1407CC4B0`, `FUN_1407CB750`/`CB820`/`CB8F0` | the 85 small lights and the fires (families 1–4 of §3) | lights with radius 2 / 5 / 3 / 6 present at the same vector indices |
| **Character SFX** (weapons, enchantments, lantern, burning, conditions) | `ChrWepSfxSlot` (`FUN_1405B5BA0`), `ChrWepEnchant[Whip]Slot`, `ChrLanternSlot` (`FUN_1408755D0`), `ChrBurnSlot`, `ChrSingle[Oneshot/Trace]SfxSlot`, `ChrMultiSfxSlot`, `ChrMagicGoodsUseSfxSlot`, `ChrConditionSfxSeSlot`, `ChrSoulEatSlot`, `ChrSorceryWepSlot`, `ChrStatueDeadSlot` | the player's held flame (radius 0.5) and its companion (radius 2.5) | the two lights within 1 m of the player |
| **Bullets** (spells, projectiles, explosions) | `FUN_14097CFA0`, `SprjBulletExplosionState` (`FUN_1409759F0`) chain | none in the idle witness; cast a pyromancy to see one | light count rises while a projectile flies |
| **Cutscene (Remo)** | `SprjRemoFactory` slots 4/5 (`FUN_140CE5260`/`5310`), `SprjRemoSfxAct` | none in gameplay; also the only writer of the hide mask | hide mask `+0x14C` non-zero only during a Remo |

**Order:**

1. common gates (R0–R5);
2. map/object;
3. character;
4. bullets;
5. Remo, last (it needs a cutscene).

## 5. Runtime checks for the pilot (read-only, original and native at the same spot)

Addresses are image-relative (`image + RVA`). "Counter" means a pass-through hit counter of the
kind used for Native1072–1093.

| # | Read | Original expectation | Native today (INFERRED) | Meaning when it differs |
|---|---|---|---|---|
| **R0** | `[4961AB0]` (GX display object) | non-null | null (inventory 903) | why the GPU members of the effect system cannot be built with engine code as is |
| **R1** | `[[4799C98]+0x58]` and `[49623C0]` | equal, non-null (`GXFfxSceneCtrl`, vtable `143D64008`) | **0 / 0** | **common cause of §1: no effect can be created** |
| R2 | byte `[SceneCtrl+0x44]`, count `[49651D8]` and an entry with id `0x261`, byte `[49651E8]`, byte `[[4933088]+0x20]` | 1, > 0 with 0x261, 1, 1 | not built | type registries not filled or not sealed: FXR actions are dropped at load (MENUS-MAP §14) |
| R3 | `[[SceneCtrl+0x28]+0x160]` (resource manager); counter on `FUN_14024ECF0` during map load | non-null; many calls | — | FXR resources not resolved |
| R4 | counter on `FUN_14024D000` (SceneCtrl update) and on `FUN_14024CE30` (library update job) | once per frame; many per frame | 0 | effects exist but never update, so no host is created |
| R5 | counters on `FUN_140E6DCD0`/`E6DA30`/`E6DBC0` with the `+0x58` branch taken; on `FUN_14024EFE0`/`14024F0F0` | many while the map loads and the player acts | entry calls without the branch | per-owner creation (§4 families) |
| R6 | counters on `FUN_14028F200` (point factory), `FUN_1402B2410`, `FUN_140266FC0` | about 104 in the m40 witness | 0 | hosts and lights created |
| R7 | `Read-LightState.ps1` (existing): count, then entries 59..162 by family (§3 table) | 163 | 59 | after the fix: the missing family names the owner (§4) |
| R8 | `[SceneCtrl+0x148]`, `[SceneCtrl+0x14C]` | 0 and 0 in gameplay | — | non-zero outside a cutscene would hide every effect whose categories it covers |
| R9 | per effect light: dwords `+0x60`/`+0x64` | `0x11` or `0x01`, then `1` | — | other values come from the create-info (`owner+0x1FC/+0x200`); `hi & 1 == 0` excludes the light from the world pass |
| R10 | counter on `FUN_1402B2E30` (point host update) and the light's `+0xB0` | once per frame per host; `+0xB0 = 1` | — | 0 visible lights with hosts present means intensity 0 (no effect time), distance block, or hide mask |

**Decision:**

- R1 null natively → implement the native effect system (§2.5) first. Nothing downstream can
  work before it.
- Then R2 → R3 → R4 → R6 in order. The first failing row names the missing step.
- Then R7 by family: a whole family still absent names its owner system (§4).
- Then R8–R10 for lights that exist but stay dark.

## Exports

`decompiled/DS3-1152-F-002B2410`, `002B4080`, `0024ED10`, `00018380`, `00266FC0`, `0028F200`,
`0028F290`, `00249950`, `00249E90`, `0024A320`, `00E6E720`, `008F2CA0`, `00E6DCD0`, `0024D000`,
`0024E790`, `0024B260`, `0024CE30`, `0024CCB0`; `002B2E30` and `000F1A40` were already exported
by I-0067. Query outputs: `evidence/I-0068/queries/ffx/`.
