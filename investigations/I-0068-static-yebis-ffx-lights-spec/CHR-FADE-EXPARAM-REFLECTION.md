# I-0068 part Y: character fade and ExParams (all characters), and the reflection view's part filter, ready to code

Request (coordinator, 2026-10-10):

1. Turn the first two items of CHARACTER-EFFECTS-MAP §2 into ready-to-code specs:
   - the death fade and instance opacity for every character (`ChrFadeOutDeleteSlot`);
   - the wet and damage ExParams (`9CB270`, `9CAA80`, from `8C1BC0`).

   The NPC bodies stay with the pilot.
2. Then specify the filter of parts admitted into the planar reflection view.

MENUS-MAP is not modified.

Labels: STATIC, OBSERVED, INFERRED.

**Decompiles exported:** `00890950`, `00888C70`, `005B1520`, `009CAE60`, `009CAA80`,
`008C1BC0`.

## 0. Key finding

**Both chains are CPU work in the character update. Production native already consumes their
results for the player's parts.** The CPU world update runs natively: the character hooks in
`native/frame/game_bootstrap.cpp:112–140` are pass-through witnesses.

So the work for "all characters" is:

- wiring the same readers to every character draw entity;
- the handful of rules below.

It is not a new producer.

## 1. Fade and instance opacity

### 1.1 The original chain (STATIC)

| Step | Code | Effect |
|---|---|---|
| fade request | `888C70(chr, time, target, keep)` | `chr+0x1A44 = target` (when in [0,1]); `chr+0x1A48 = −chr+0x1A44 / time` (the rate); `chr+0x1A0C` bit 5 = `keep` |
| death fade-out | `ChrFadeOutDeleteSlot` (vtable `1427C8510`), update `5B1520` | product `P = chr.vt+0x3A0() × chr+0x1A44 × chr+0x1A40 × chr+0x1A4C × chr+0x1A50`. **If P ≤ 0:** `88F2A0(chr, 1)` (INFERRED: the removal) and slot flag `+0xC` bit 14 cleared. **Else, once** (`slot+0x24`): `888C70(chr, slot+0x20, 1.0, 0)` starts the fade from opaque over `slot+0x20` seconds; `chr+0x1F4C = 1` |
| per-frame integration | `890950(chr, dt)` (the character update, called by `8C1BC0`) | `chr+0x1A44 += rate × dt`. At ≥ 1: alpha 1 and the rate cleared. At ≤ 0: both cleared, and with bit 5 of `+0x1A0C` the removal flag `chr+0x1A08 \|= 0x40` |
| hand-off to the model | `890950` | the character model `M = [chr+0x48]` gets **`M+0x168 = P`** (the same product), dirty bit `M+0x74 \|= 0x100`; then `M.vt+0x40(dt)` updates the model's draw entities |
| draw-entity opacity | INFERRED from the native reader: the model update copies `M+0x168` to each draw entity's **group opacity `group+0x124`** | — |

The other factors of `P`, all CPU: `chr+0x1A40`, `+0x1A4C` (written by `8727B0` and `87E8A0`),
`+0x1A50` (`8902D0`) and `chr.vt+0x3A0`. INFERRED: invisibility, phantom transparency and
cutscene hiding. They need no special handling: the product is what the model sees.

### 1.2 What native already does (`main`)

- Reads the fade state of a draw entity: `ReadModelFadeStateAt`
  (`native/frame/game_model_selection.cpp:28`).
  - Group opacity is `group+0x124`; flags `+0x98`; fade enables `+0x88/+0x89`.
  - The two LOD tracks are `+0x120/+0x1C0`; the geometry weight `+0x130`.
  - Character parts use the layout `{0x120, 0x1C0, 0x130, 0x1F0}`
    (`native/frame/game_bootstrap.cpp:3719`).
- Selects the fade per engine pass: `SelectModelFade` (`game_model_selection.cpp:137`).
  - opacity = group opacity × track blend × weight;
  - a second intent during a LOD cross-fade.
- Writes the opacity into the instance record (`ditherFadeAlpha`):
  `native/frame/game_bootstrap.cpp:2230`.
- Switches to the alpha-tested pixel variant when the opacity is below the category scalar
  (`game_bootstrap.cpp:3914`, `:4392`).

### 1.3 Ready to code for all characters

| # | Step |
|---|---|
| 1 | For every NPC/enemy draw entity the pilot admits (MENUS-MAP §55), set `fade_owner` = that entity, with the same layout `{0x120, 0x1C0, 0x130, 0x1F0}` as the player's parts. Nothing else: the CPU update fills `group+0x124` |
| 2 | **Skip the draw when the selection has no intent** (opacity 0 or suppressed). `SelectModelFade` already returns `count = 0`; the NPC path must honour it like the player path |
| 3 | **Removal:** do not delete GPU state on `chr+0x1A08 & 0x40`. The engine's removal (`88F2A0`) retires the character; native releases its draw entity when the selection no longer reports it |
| 4 | Shadow and depth passes: apply the same opacity to the `Dpt`/`DptA` casters (`SelectModelFade` with their engine pass), so a fading body's shadow fades too (SHADOWS-NATIVE) |

**Checks:**

- F1: kill an enemy; `group+0x124` ramps 1 → 0 over the slot time. Native draws it with the
  alpha variant below the category scalar, then stops drawing it.
- F2: the instance record's `ditherFadeAlpha` equals `group+0x124 × weight` every frame of the
  fade.
- F3: the player's fade (respawn, fog walls) is unchanged.

## 2. Wet and damage ExParams

### 2.1 The original chain (STATIC)

**In `8C1BC0`** (character update; `ctrl = [[chr+0x1F90]+0xD8]`, the character's ExParam
controller):

- **Damage:** `9CAA80(ctrl, [chr+0x1FA0]+0x228)`.
  - It tests the character's SpEffect list against two id tables (`868480(0..5)`,
    `868460(0..3)`) and sets damage levels 3–12.
  - It queues `ExParam_DamagedNormal_DamageRate`.
- **Wet:**
  - With wet sources (`9F1970([chr+0x1F90]+0x108)`, a list of level/param pairs): the highest
    level above the threshold sets `chr+0x1F40 = 1`, `chr+0x1F44 = chr+0x1F48 = level`. Then
    `9CB270(ctrl, {param id, …})`, repeated for slots 3–8 when the source flag is set.
  - Without sources: `9CB270` with the default id `88BBD0(chr)`.
  - `9CB270` reads the param row (`E43FF0`, then `E44030`/`E440C0`/`E44150`) and queues
    `ExParam_WetDiffuseColor`, `ExParam_WetReflectanceColor` and `ExParam_WetParam` (float4 each).
- **Queue** `9CAE60(ctrl, request)`: a vector of 0x98-byte named requests at `ctrl+0x10..+0x18`.
  It updates the value (`+0x70..+0x80`) and sets dirty `+0x94` for an existing name; otherwise
  it appends (`9CB820`).

**Then `890950`:**

- phantom (`9CAF30`), `9CAA70`, `9CAE50`;
- **`9CB550(ctrl, dt)`**: the controller's per-frame blend;
- it copies **`chr+0x1F40/+0x1F44/+0x1F48` to the model `M+0x16C/+0x170/+0x174`** (dirty bits
  `0x4000000`, `0x8000000`, `0x10000000`).

**On the draw side** (what native already reads):

- the part's **parameter-animation table** `[item+0x6E8]+8`: 0x90-byte rows with
  name/key/target/strength/material index;
- the wet draw words at **`entity+0x2CC..+0x2D4`** (order b, a, c).

INFERRED: the table is filled from the controller queue (`9CB550`/`ctrl+8 vt+0x6B8`), and the
wet words from `M+0x16C..+0x174` through the model update.

### 2.2 What native already does (`main`)

- Reads the parameter-animation table: `ReadModelParameterAnimations`
  (`native/frame/game_model_selection.cpp:99`); owner `[item+0x6E8]`
  (`native/frame/game_bootstrap.cpp:3716`).
- **Damage:** `ProduceModelDamageParameters` (`native/content/model_parameters.cpp:243`). It
  uses the key CRC of `ExParam_DamagedNormal_DamageRate` and writes
  `FC_DamagedNormalParam`/`UvTiling` (b2 +0x200..+0x22F), called at
  `native/frame/game_bootstrap.cpp:3428`.
- **Wet:** `ProduceModelWetParameters` (`model_parameters.cpp:292`), with:
  - the material's wet block (`ReadModelWetMaterialParameters`, material `+0x210..+0x24F`);
  - the draw words (`ReadModelWetDrawInputsAt`, `game_model_selection.cpp:47`);
  - the shader activation catalog (`ReadModelWetShaderCatalog`, 43 families);
  - called at `game_bootstrap.cpp:3442–3455`.
- When the material disables them, the wet and damage b2 blocks are written as zero
  (`model_parameters.cpp:31`, `:35`).

### 2.3 Ready to code for all characters

| # | Step |
|---|---|
| 1 | For each NPC/enemy part, pass `parameter_animations = [item+0x6E8]` and the wet group (`entity+0x240`, read at `+0x2CC`) exactly as the player's ASM parts. The producers are shared; no NPC-specific code |
| 2 | Keep the table **per part** (`material_index`, material-name rows, universal rows): an enemy's damage applies only to the materials its rows name |
| 3 | No per-frame native work: the engine's `8C1BC0`/`890950`/`9CB550` already run for every character the world updates |

**Checks:**

- X1: in rain or water at m37, the player's and an enemy's table hold `ExParam_Wet*` rows; b2
  +0x230..+0x26F is non-zero and matches the original's constant buffer.
- X2: hit an enemy with a damage-tinting effect; the `ExParam_DamagedNormal_DamageRate` row
  appears and b2 +0x200 follows.
- X3: with the setting off or a non-wet family, the zero blocks stay (unchanged behaviour).

**Open (read before relying on X1):** the exact copier from `ctrl`'s 0x98-byte queue to the
0x90-byte table, `9CB550` or `ctrl+8 vt+0x6B8`. Native does not need it, because it reads the
table, but the witness should confirm the table updates every frame for NPCs as it does for the
player.

## 3. The reflection view's part filter (STATIC)

**`FB0E0`** passes the scene query a **mask**: the main view's 64-bit mask with its **low byte
replaced by 0x10** (`*param_8 & 0xFFFFFF00 | 0x10`, the high word copied).

**`FCE30`**, for view group 0x39:

- adds **bit 0x01** when the option `[[144961B50]+0x5360]+0x6E` is set (`uStack_cd8 |= 1`);
- runs the same scene-tree query (`[144961B78] vt+0x18(mask)`) and the same frustum test, on
  the **mirrored view**;
- walks the light container only when `+0x6D` is set and the frame's `+0x3EBE` allows it.

**So the admitted parts are the scene-tree objects whose draw category intersects 0x10.** That
is, parts registered for reflection; and all ordinary category-1 parts too when `+0x6E` is set.
The tree, the culler and the per-object `D043C0` category test are the main view's.

**Native already runs this query for the main view:**
`CollectNativeSelectedScene` builds `mask = (policy & ~0xFF) | 1`
(`native/frame/game_bootstrap.cpp:3992`, "C2EC0 main pass0 category"). It visits through the
engine's own query and category tests (`:3700–3703`).

**Ready to code:**

| # | Step |
|---|---|
| 1 | Run `CollectNativeSelectedScene` a second time per frame when the reflection is armed (WATER-REFLECTION-INTERACTION-E08 §1.2 gates), with the **mirrored view** and the culler built from it |
| 2 | Mask = `(policy & ~0xFF) \| 0x10`, plus `\| 0x01` when `[[144961B50]+0x5360]+0x6E` != 0 |
| 3 | Rows: drop water descriptors (kind 0x27; `57A20` gives them no row in group 0x39) and skip the decal pass |
| 4 | Lights in the reflection only when `[[144961B50]+0x5360]+0x6D` != 0 |
| 5 | Render into the 640×360 target (part W) |

**INFERRED:** at m37 the reflection holds two map parts and the sky dome (part W §1.1), which
fits a reflection-only category on a few parts with `+0x6E` off.

**Check R3:** the native reflection gather at m37 returns the same three meshes (index counts
5520, 6108, 789).

**The option bytes** `+0x6C` (reflection on), `+0x6D` (lights) and `+0x6E` (all ordinary parts)
belong to the same settings object. INFERRED: driven by WaterSurfaceQuality; Thomas's setting is
HIGH. Read their writer with the settings table if a zone shows more parts than expected.
