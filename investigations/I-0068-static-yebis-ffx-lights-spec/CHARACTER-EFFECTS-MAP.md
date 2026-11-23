# I-0068 part X: effects attached to enemies and NPCs (native state and static owners)

Request (coordinator, 2026-10-10): for the current enemies/NPCs workstream, map the effects
attached to characters, with each one's native state and static owner:

- weapon and spell FFX;
- boss auras;
- dissolution and death fade;
- status effects (poison, frost, bleeding);
- lighting from lights attached to the character.

MENUS-MAP is not modified: static thread 1 owns cloth there. It is only cited.

Labels: STATIC, OBSERVED, INFERRED.

## 0. Common ground

| Fact | Status | Proof |
|---|---|---|
| **Non-player characters are not drawn by production native** (bodies, armour, materials) | ABSENT | COVERAGE-MATRIX row 15. The native specification for them is MENUS-MAP §55 (enemies, NPCs, NPC phantoms) |
| **The GX effect system is not run natively.** No effect is created: every FFX family (E01–E07) and every effect light is missing | ABSENT | EFFECT-LIGHTS §1 (the omitted system `[SprjSfxImp+0x58]`); no `GXFfx` code in `native/frame` (COVERAGE-MATRIX rows 32–36) |
| The effect families are specified for D3D12 | — | EFFECTS-PHASE1, EFFECTS-PHASE2-E01-E02, EFFECTS-E03, EFFECTS-E05-E04, EFFECTS-E07, EFFECT-LIGHTS, FFX-SCENECTRL-NATIVE |

**So for enemies and NPCs, every item below depends on two prerequisites:**

1. the NPC body path (MENUS-MAP §55);
2. a native effect system (EFFECT-LIGHTS §1, then the E-family specs).

## 1. Map of character effects

| Effect | What it is in the original (rendering) | Static owner (CPU side) | Native state on `main` | Spec to follow |
|---|---|---|---|---|
| **Weapon FFX** (trails, enchantments, buffs) | FFX E02 trails (`GXFfxtessellateTracer*`, indices 36–37), E01 billboards/glow, E03 model particles; often an effect light (E06) | `ChrWepSfxSlot` (`5B5BA0`), `ChrWepEnchantSlot`, `ChrWepEnchantWhipSlot`, `ChrSorceryWepSlot` (EFFECT-LIGHTS §4); creation `249950` through `E6DA30`/`E6DBC0`/`E6DCD0` → SceneCtrl `24F0F0`/`24EFE0` | ABSENT (no effect system) | EFFECTS-PHASE2-E01-E02, EFFECTS-E03, EFFECT-LIGHTS |
| **Spell FFX** (sorceries, pyromancies, miracles) | the cast effect on the caster (character slots), then the **bullet**: its FFX and explosion (E01/E02/E04 distortion, E05 radial blur, E06 light) | `ChrMagicGoodsUseSfxSlot`, `ChrSingleOneshotSfxSlot`/`TraceSfxSlot`; bullets `97CFA0`, `SprjBulletExplosionState` `9759F0` (EFFECT-LIGHTS §4) | ABSENT | EFFECTS-PHASE2-E01-E02, EFFECTS-E05-E04, EFFECT-LIGHTS |
| **Boss auras** | FFX attached to dummy polys (`ChrMultiSfxSlot`, `ChrSingle*SfxSlot`): E01/E02/E03, sometimes E07 GPU particles. Plus **emissive materials** (`…EmiIbl[Glow]` families: GBuffer5 emissive, then the glow post chain `EmissiveToGlow` / `Sample13Weight` / `GlowComposite`, seen in the m40 listing) | effect slots as above; material: the model's MTD | FFX ABSENT. Emissive: PARTIAL; the composite reads GB5 (`native/renderer/scene_pass.cpp:1004`) but the glow chain is absent (COVERAGE-MATRIX row 41) | E-family specs; TONEMAP-SKIN §17 (glow); MENUS-MAP §57 (emissive family) |
| **Death fade and dissolution** | the model's **fade opacity** → `cbInstanceData.ditherFadeAlpha` (+256) with the dither texture t38 in the `_[A]`/`DptA` programmes. The corpse is removed after the fade (`ChrFadeOutDeleteSlot`). Petrified deaths use `ChrStatueDeadSlot`. Soul-absorption FFX: `ChrSoulEatSlot` | `ChrFadeOutDeleteSlot` (RTTI `NS_SPRJ`, vtable `1427C8510`, slot 0 `5B1470`); the model LOD/fade path (MENUS-MAP §62: draw enable, LOD fade) | **PARTIAL for the player only:** native writes the instance opacity per geometry fade (`native/frame/game_bootstrap.cpp:2230`). NPCs absent | MENUS-MAP §62 and §55; this map for the owners |
| **Phantoms** (NPC phantoms, invaders, summons) | phantom programmes and colours `FC_Phantom{Edge,Front,Diff,Spec,Light}Color` (cbMatDynParam +416..+480) | the ExParam setter **`9CAF30`** (`ExParam_PhantomEdgeColor`…, the table at `1445B3978`), called from the character update (`890950`, `8C1BC0`) and `5B34F0` | ABSENT (no `Phantom` material path; COVERAGE-MATRIX row 10) | MENUS-MAP §47 |
| **Status: wet / poison / toxic / bleed / frost** | two parts: **(a) the condition FFX** on the body (`ChrConditionSfxSeSlot`, `ChrBurnSlot` for burning) and **(b) material ExParams** blended into `cbMatDynParam`: wet `FC_WetDiffuseColor` / `FC_WetParam1` / `FC_WetRefrectanceColor` / `FC_WetParam2` (+0x230..+0x26F), damage `FC_DamagedNormalParam` / `UvTiling` (+0x200..+0x22F); frost `FC_FROZEN_RATE` (cbModelParam +44) and snow (`FC_SnowRate`) | (a) EFFECT-LIGHTS §4 slots. (b) the per-frame character update **`8C1BC0`** (ChrIns/PlayerIns vtable +0x120, `native/frame/game_bootstrap.cpp:121`) calls **`9CB270`** (wet: `ExParam_WetDiffuseColor`, `WetReflectanceColor`, `WetParam`, values from param lookup `E43FF0`) and **`9CAA80`** (`ExParam_DamagedNormal_DamageRate`). INFERRED: the frost writer of `FC_FROZEN_RATE` goes through the draw state `1A86C0`; not read here | (a) ABSENT. (b) PARTIAL: native zeroes the wet and damage blocks when the material disables them (`native/content/model_parameters.cpp:31`, `:35`) and owns the ExParam animation table lookup (`native/content/model_parameters.cpp:245`, `ExParam_DamagedNormal_DamageRate`). Correction (part Y): the CPU update runs natively and native already reads the resulting table (`[item+0x6E8]`) and wet words for the player; NPC parts only need the same wiring (CHR-FADE-EXPARAM-REFLECTION §2) | (b) needs a spec from `9CB270`/`9CAA80`/`E43FF0` (owners listed here); (a) EFFECT-LIGHTS |
| **Lights attached to characters** (lantern, held flame, weapon buffs, spell glow, boss light) | **effect lights** created by the character's FFX (point lights in the GX effect system), drawn by the point-light path. Map lights carried by objects are in the light container `[[root]+0x41D0]` | `ChrLanternSlot` (`8755D0`) and the weapon/spell slots → SfxIns → effect light; EFFECT-LIGHTS §1–§4. MENUS-MAP §54 traces one map torch end to end | Map container lights DONE (Native1054–1055). **Character lights ABSENT** (the player's held flame, radius 0.5, and its companion, 2.5, are missing at m40, EFFECT-LIGHTS §4) | EFFECT-LIGHTS; SPOT-LINE-LIGHTS-NATIVE (if a light is a spot) |
| **Blood on bodies** | the instance decal provider t41/t42 on the `Bloody` materials. World blood splashes are deferred decals | MENUS-MAP "Decals (blood and snow)"; the GXDecalManager chain of DECALS-NATIVE | body: neutral decal (MENU-OFFSCREEN-MODELS); world: CPU only | MENUS-MAP; DECALS-NATIVE |

## 2. Order for the enemies/NPCs workstream (proposal)

1. **NPC bodies** (MENUS-MAP §55). Nothing else shows without them.
2. **Fade and removal** (`ChrFadeOutDeleteSlot`, instance opacity): the native player path
   already writes the opacity, so this extends to NPC instances.
3. **Material ExParams in the character update** (`8C1BC0` → `9CB270`, `9CAA80`, `9CAF30`):
   CPU-only blends into `cbMatDynParam`. This gives wet, damage and phantom looks without
   any effect system.
4. **The effect system** (EFFECT-LIGHTS §1). It unlocks, at once:
   - weapon/spell FFX;
   - auras;
   - condition FFX;
   - character lights (through the existing point-light path).
5. **The glow post chain** (TONEMAP-SKIN §17) for emissive auras.

## 3. Open points

- **The frost tint writer** (`FC_FROZEN_RATE`): not read. Start from `1A86C0`'s
  `cbModelParam` assembly.
- **Which condition looks are material-only and which are FFX-only.** It comes from
  `SpEffectVfxParam` (the param name string `142989700`, stored at `14464FCE0`). Read its rows
  per condition before coding (poison, toxic, bleed, frost, curse).
- `pixelDissolve` (string `143B156C0`, stored in the `.rdata` table at `143B13AE0`) is not
  traced. INFERRED: the DS3 death dissolve is the dither fade above, not this name.
