# I-0068 part Z: FFX attached to objects and characters (torches, braziers, bonfires, auras), ready to code

Request (coordinator, 2026-10-10). The pilot saw that the flame of the wall torch at Argo's spot
(High Wall, m30_00) is missing natively. Specify the family "FFX attached to objects and
characters":

- how an MSB object or a character puts its effects on its dummy polys;
- how their position is updated;
- what the native effect system (E01–E06) lacks to draw them;
- runtime checks at the High Wall and in a witness zone.

MENUS-MAP is only cited (§54 is the torch's static census).

Labels: STATIC, OBSERVED, INFERRED.

**Decompiles exported:**

- creation and handle: `007CB820`, `007CB8F0`, `007CB750`, `007CB630`;
- slots: `007CC4B0`, `007CC410`, `007CC880`, `0065AB10`, `0052EC30`, `005292A0`, `007CBBA0`,
  `007CBEA0`;
- follow binding: `002447C0`.

## 0. The torch (summary of MENUS-MAP §54) and native status

- **The torch:** `o304863_8030`, the object at (−31.32, 2.01, 85.24).
  - Its model has **one dummy, reference 200**, on `Model_Dmy` at (0, 0.437, −0.137).
  - It has no ObjectParam row and no entity id. No SFX region lies nearby.
  - Its warm light is not in the light bank. INFERRED: an effect light.
- **Native1181:**
  - every SFX request is `native_sfx_request_deferred`;
  - **id 300** was requested 10 times (`f000000300.fxr`; INFERRED: the torch flame);
  - the effect system is published with `fx_rendering:false`.
- **Native code:**
  - `DeferNativeSfxSceneRequest` replaces `7CB820` (`native/frame/game_bootstrap.cpp:8628`).
    It returns the engine's **empty** result: no instance at `handle+0x18`
    (`game_bootstrap.cpp:6758`).
  - The other routes (`7CB8F0`, `7CB750`) run the original code against the native SFX manager
    (`PublishNativeSfxManager`, `game_bootstrap.cpp:8182`), whose effect member
    `[SprjSfxImp+0x58]` is null. So **every** route yields an empty handle (EFFECT-LIGHTS §1).

## 1. How an object or a character attaches its effects (STATIC)

### 1.1 The common handle

**`SfxHandle`** (vtable `1427682C0`, 0x470 bytes), built by `7CB630` from a creation result:

| Field | Content |
|---|---|
| `+0x10` | the GX effect handle (`1A879A0` constructs, `1A879F0` destroys); its instance is at `+0x18` of the result (null natively) |
| `+0x30` | the id |
| `+0x40` | the FX instance info (`249950`) |

**Three creation routes**, all into the SFX manager `[144799C98]`:

| Route | Function | Creator |
|---|---|---|
| world transform | **`7CB820(handle, id, transform, flags)`** | `E6DCD0` |
| block-local | **`7CB8F0(handle, blockSfx, id, transform)`** | `E6DCD0` |
| — | `7CB750(handle, id, transform, flags)` | `E6DA30` |

All three need `[SprjSfxImp+0x58]` (EFFECT-LIGHTS §1).

### 1.2 Persistent effects on an object's dummies: `ObjMultiSfxSlot` (`65AB10`, slot update)

Slot fields:

| Field | Content |
|---|---|
| `+0x20/+0x24` | effect id (current / requested) |
| `+0x28/+0x2A` | **dummy reference id** (current / requested, short; < 0 = the object's origin) |
| `+0x2C` | flags: bit 4 = local→world through `7CC760`; **bit 5 = follow**; bit 8 = applied |
| `+0x2E` | stop mode: 0 kill, 1 fade, 2 stop emission |
| `+0x30` | handle count |
| `+0x38` | the handle array (0x470 each) |
| `+0x40` | fade alpha |

**On a request** (requested ≠ current):

1. `662D60(obj, &list, dummyRef)` collects **every dummy of the object's model with that
   reference** (0x40-byte transforms).
2. One `SfxHandle` per dummy, created by **`7CC4B0(handle, obj, id, transform)`**:
   - it takes the object's map block `[obj+0x18]+0x13`;
   - then the block's SFX manager `[[14475ABD0]+0x10]+0x20 + block × 0x70 + 0x58`;
   - then `7CB8F0`.
3. Without a dummy reference, `7CC410` creates at the object's origin (`664440`).

**Each frame, with bit 5 set:**

- the dummy transforms are re-read (`662D60`, and `7CC760` with bit 4);
- each handle gets **`7CC880(handle, transform)` → `1A87F50(handle+0x10, transform)`**: the
  effect is moved to the dummy.

**Stop:**

- mode 0 destroys the handles (`4D2AC0`);
- mode 1 fades `+0x40` (step `143D7EED8`);
- mode 2 clears the follow binding and stops emission (`1A88370`, `1A87CB0`).

### 1.3 One-shot effects on object or entity dummies

- **`52EC30(owner, dummyRef, id)`**: for objects. Up to 100 dummies with that reference
  (`662B40`), one `7CB820` per dummy at its world transform; otherwise the object origin. **No
  follow.** No direct caller: it is reached through a function table. INFERRED: the event and
  action commands.
- **`5292A0(entity, dummyRef, id)`**: the same for any entity, through its own dummy query
  (`[entity+0x10] vt+0x3E8`).

### 1.4 Effects attached to a character (follow binding)

- **`7CBEA0(handle, chr, id, transform, model, dummy)`**:
  1. `7CB820` at the transform;
  2. reads the character's binding inputs (`7CBA60`, `88A190`, `889D00`, `88B040`: model,
     dummy, offsets);
  3. **`2447C0(handle+0x10, target, offset, extra)`** stores the follow target in the GX handle
     (`+0x48..+0x5C`, `+0x70`) and attaches it (`1A88370(handle, handle+0x30, 0x430,
     callback 244A80)`).
  - Callers in the `87xxxx` character-slot range: `87AEA0`, `87CC70`, `87E0E0`, `87F280`.
    INFERRED: the weapon, lantern, burn and condition slots of EFFECT-LIGHTS §4.
- **`7CBBA0(handle, &id, transform)`**: `7CB820`, then `2447C0` with `D27770(1)` and neutral
  offsets. Used by the character SFX manager (`844400`, `846CE0`, `846E40`, which look up
  characters through WorldChrMan `8D9F20`).
- **Boss auras and weapon effects** are these character handles. **Torches, braziers and bonfire
  flames** are object slots (§1.2), or one-shot dummy effects (§1.3). The bonfire condition
  `45DD90` → `7CC4B0` (EFFECT-LIGHTS §4) goes through the same block route.

## 2. Position update (STATIC)

| Kind | Who moves the effect |
|---|---|
| **object slot with follow** (`+0x2C` bit 5) | the slot's update, each frame: `7CC880` → `1A87F50(handle+0x10, world transform of the dummy)` |
| **character-attached** (`2447C0` binding) | the **effect system's own update** (`SprjSfxImp::Update` `E6E750` → SceneCtrl slot 8 `24D000`, FFX-SCENECTRL-NATIVE §3.1). It reads the follow target stored at `handle+0x48..+0x5C` each frame. INFERRED: through callback `244A80` |
| **one-shot at a transform** | never moved: spawned at the dummy's world transform |

The dummy transform itself is CPU work: the model's dummies × the object or character pose. It
already runs natively, because the CPU world update runs.

## 3. What the native effect system lacks (E01–E06)

| # | Missing piece | Spec |
|---|---|---|
| 1 | **The effect member `[SprjSfxImp+0x58]`** (GX FFX scene control). Without it, `E6DCD0`/`E6DA30` return empty handles on every route | EFFECTS-PHASE1-NATIVE §2–§4 (construction `24B260`, device init `24C530`) |
| 2 | **Replace the deferral hook.** `DeferNativeSfxSceneRequest` on `7CB820` must give way to the original `7CB820`, or a native equivalent that returns the created instance, once #1 exists | `native/frame/game_bootstrap.cpp:6758`, `:8628` |
| 3 | **Per-frame update** (`E6E750` → slot 8 `24D000`): emitters, particles and **follow targets** | EFFECTS-PHASE1-NATIVE §5 |
| 4 | **`1A87F50`/`1A88370` on a live handle.** The slot's position push and the follow attach must reach the instance. They do when #1–#3 exist: they are GX CPU calls on the handle | this note §1.2, §1.4 |
| 5 | **Draw** of the flame. The torch and bonfire fires are classic billboards/sprites, E01 (`GXFfxtexture`/`softTexture` and glow twins), plus E02 trails for some auras, plus the frame copy for distortion | EFFECTS-PHASE2-E01-E02-NATIVE; E04/E05 for heat haze (EFFECTS-E05-E04) |
| 6 | **The effect light** (E06): the warm light on the pillar is a light created by the flame instance and registered in the FX light registry (`[4962398]`, already native, empty). The point-light path (Native1054) must read it | EFFECT-LIGHTS §2.1, §2.5; SPOT-LINE-LIGHTS-NATIVE for spots |
| 7 | **The bowl's emissive** (`GXFlver_ColDifSpcBumpEmiIbl`) is a material family, not FFX. Admit it in the native G-buffer path | MENUS-MAP §54 family 4 |

**Order:** 1 → 2 → 3 (the flame exists and moves, visible through its light once 6 is done) →
5 → 7.

## 4. Runtime checks

### 4.1 High Wall, Argo's torch (`o304863_8030`, m30_00)

| # | Check (read-only) | Original expectation | Native today (INFERRED) |
|---|---|---|---|
| T1 | **Superseded by LIVE-REDUCTION-AM §4:** the torch flame is an `ObjModelSfxSlot` (`14065A010`) fed by `ModelSfxParam` row 304863000 {800023, 200} and created through `140E6EB70`, not `7CB820`; the id-300/320 `7CB820` requests are event or param-row routes (12 callers classified there). Read: counter on `14065A010`, arguments of `140E6EB70`, caller RVA at `7CB820` for 300/320 | the torch handle exists (effect 800023, dummy 200); the 300/320 callers are named | id 300 deferred 10× (Native1181), caller unknown; torch route now identified statically |
| T2 | the request transform | position = torch transform × dummy 200 (0, 0.437, −0.137) ≈ (−31.32, 2.45, 85.38) ± rotation | the same CPU transform (the request runs) |
| T3 | the created instance | `handle+0x10` holds a non-null instance (`result+0x18`) | null (`native_sfx_deferred_result_contract`) |
| T4 | follow or static | for a slot: `+0x2C` bit 5 and `7CC880` calls each frame; for a one-shot: no update | — |
| T5 | effect light | one registry entry within 0.5 m of T2, warm colour (the pillar light) | registry empty |
| T6 | image | flame sprites over the bowl (E01 draws at the torch's screen position) and warm light on the pillar | sconce dark |

### 4.2 Witness zone m40 (Firelink Shrine, I-0055 / I-0063 captures)

| # | Check | Original expectation |
|---|---|---|
| F1 | the bonfire: the condition `45DD90` → `7CC4B0` → `7CB8F0` with the bonfire's flame id, at the bonfire dummy | one instance; the E01 fire draws (GXFfx tessellate/soft textures, observed in the m40 listing: `GXFfxtessellateBlendSoftTexture[GlowD]`, `GXFfxtessellateTexture`) |
| F2 | the player's held flame (`ChrLanternSlot` `8755D0`, character route `7CBEA0`/`2447C0`) | two lights within 1 m of the player (radius 0.5 and 2.5, EFFECT-LIGHTS §4); the instance follows the hand: its position changes with the animation with no new request |
| F3 | counts | the m40 listing's effect draws per frame equal the native ones after steps 1–5 |

## 5. Risks

1. **The torch's exact owner** (§4.1 T1). Slot (§1.2) and one-shot (§1.3) behave differently: a
   one-shot flame would be a looping FXR spawned once at a fixed transform. T1 settles it in
   one run; the native work (§3) does not depend on it.
2. **1A87F50, 1A88370 and 244A80** are GX handle internals, not transcribed here. **Part AK (STATIC, two call levels):** the handle functions 1A879A0, 1A879F0, 1A87CB0, 1A87F50, 1A88370, 2447C0 and the removal 4D2AC0, with their callees (1A87E70, 1A884E0, 1AB18C0, 1AB18F0, 1AB1080, 1AB1140, 1AB1A60, 1A87C10, 1A883D0, 2498D0), read or write no GX device global ([144961AB0], [144961B50]).
   They are CPU calls on the handle. They can run unchanged once the effect system exists (EFFECTS-PHASE1). Deeper callees (14029EC90, 14025EE00, 140E6C6C0, 141AB00B0, 141AB0320, 141AB04E0) were not read: live-only, covered by the crash guard of EFFECTS-IMPLEMENTATION-CHECKLIST step 1.5.
3. **The deferral hook** retains requests (`sfx/latest-deferred-request`). When the real path
   arrives, requests made before the effect system was ready must not be replayed twice.
