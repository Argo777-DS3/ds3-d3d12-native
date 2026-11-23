# Phantoms and summon signs natively: PhantomParam by id, the `_Phantom_` programme set, constants, routing, ground signs (STATIC; checklist P1-P18, live rows PL1-PL10)

Written 2026-10-11 for the menus workstream. It consolidates and completes, from the code and the regulation data, MENUS-MAP sections [41, 42, 47](MENUS-MAP.md), 59 and 74-76
and the I-0068 note [PHANTOMS-SIGNS-FOGWALLS-NATIVE](../I-0068-static-yebis-ffx-lights-spec/PHANTOMS-SIGNS-FOGWALLS-NATIVE.md) (owned by the YEBIS thread, not edited here).
Nothing was run. Labels: STATIC (code, DXBC or data read), INFERRED, OPEN. New in this note: the full PhantomParam/RoleParam decode with the final shader constants
(`Get-PhantomTable.ps1`), the proof that the alternate shader set differs from the normal one only in two pixel slots, the exact routing table (which state gives list 0x21, 0x29 or 2),
the DXBC check of six families (plus the hair form of MENUS-MAP 47), and the sign effect files (structure and colours).

## 0. Answer first

| Item | Result |
|---|---|
| What makes a draw a phantom | `state+0x87 = 1`: the model or part has a **PhantomParam id >= 1 whose row exists** (MENUS-MAP 76). Single models: `FUN_140D07140` (`entity+0xC38`, row via `FUN_140E35EF0`); assembled models (the player, ghosts, NPCs with parts): `FUN_140CFDE70` per part from `desc+0x44`. Ids come from the resolver `FUN_14088A950` (RoleParam `+4` / `+4C..54`, NpcParam `+0x1CC` / `+0x238`) and from weapon enchants |
| The shader set | the **alternate set `mat+0x78`** (`FUN_140082E50`) equals the normal set (`FUN_140D467E0`) built by the same code (`1400A9BA0`, `1400AA630` passes 0..16, `1400AA4D0`) **plus `FUN_1400A9EF0`, which overwrites only the pixel slots of passes 0 `Fwd` and 4 `FwdSdw`** (and their `[A]` forms; the `[AA]` slots are written too but no `_[AA]` programme exists in the game, so they stay null) with `<shader>_Phantom_<pass>`. Every other pass (depth, Gbuf, Velo, shadow...) is the same programme. Of the 11 `_Phantom_` files per shader (14 with the debug passes `FwdDbg`, `FwdSdwDbg`, `GBufDbg`) only `Fwd`, `Fwd_[A]`, `FwdSdw`, `FwdSdw_[A]` are reachable; `Dpt`, `DptA`, `Gbuf`, `Gbuf_[A]`, `FwdSSdw`, `FwdSSdw_[A]`, `Velo_[A]` are dead data |
| Vertex shaders | unchanged: `<family>_Fwd[_Skin][_Ins]` (the alternate set builds the same vertex table) |
| Pixel shader | one shape for all families (checked on `SAT_Equip`, `SkinIbl_Bloody`, `EyeIbl`, `FgFaceIbl_Bloody`, `ColDifSpcBumpIbl_Bloody`, `Fur_FDP`, `SAT_HairIbl2`): the normal forward lighting **including the two IBL cube samples** (`t30`, `t31`, four `sample_l` as in the normal `_Fwd`; corrects MENUS-MAP 47 "no IBL"), plus the five phantom colours from `cbMatDynParam` rows 26-30 (`+0x1A0..+0x1EF`), rim = `lerp(Edge, Front, t)`; a second render target only in the SAT and Fur families |
| Colours | row-driven (section 2): white 80, red 81, blue 82, pale blue 83/84, gold 85/86, magenta 88, purple 91, NPC 74/76, with the final constants computed for all 102 rows |
| Routing | an opaque phantom goes to **list 0x21** (effect pass type 1, depth written), a fading or special-kind one to **list 0x29**; the emissive class 0x100 goes to row 0x22 for models with `+224` bit 15 (section 5). Native today reads the same bytes but **refuses category != 2** (`model_pass.cpp:35`), so phantoms are not drawn |
| Signs | `SosSignMan` creates one effect per sign through `7CB820`: id **30001 white, 30011 red, 30021 / 30031 gold, 30041 / 30051 magenta**; each is a controller FXR (no renderer) that spawns the visual `3000x2` (5 billboards, 1 distortion, 1 point light). Only the white NPC sign exists offline |
| Native work | P1-P6 data and constants, P7-P11 programmes and rows, P12-P14 signs, P15-P18 tests and witnesses |

## 1. Which id a character gets (STATIC)

The resolver `FUN_14088A950` takes, in order: `chr+0x1F38`; `chr+0x1A58` (unless -2); `NpcParam+0x1CC` (>= 0); `NpcParam+0x238` when the team type is flagged (table `1445B6800`); **RoleParam** of the role.
`FUN_1409CAF30` (ChrIns slot 36, every frame, only when the id is >= 0 and its row exists) pushes the seven `ExParam_Phantom*` into the draw's override list and **always** calls vtable `+0x6B0` (slot 214), which stores the id
(`FUN_1408AA630` for enemies, `FUN_1408C0640` for players and replay ghosts). The normal host player has id -1 (RoleParam row 0 is -1 everywhere), so it draws the plain `<family>_Fwd`.

**RoleParam** (42 rows of 0x80 bytes; decoded by `Get-PhantomTable.ps1`, columns read from the resolver and the sign code):

| Role | PhantomParam (+4, by rank +4C/50/54) | Sign SFX (+0x30), pair (+0x34) | Reading (INFERRED from colours) |
|---|---|---|---|
| 1, 50001, 80001 | 80 | 30001, 30000 | white co-op summon |
| 2, 10002, 50002, 80002, 90002 | 81 | 30011, 30010 | red invader |
| 10001 | 83 (rank 83/84/84) | 30001, 30000 | pale-blue co-op |
| 20001 | 85 | 30021, 30020 | gold co-op (Sunlight) |
| 20002 | 86 | 30031, 30030 | gold-red invader |
| 30001 / 30002 | 88 | 30041, 30040 / 30051, 30050 | magenta (summon / invader) |
| 40001, 60001, 70001 | 91 | 30001, 30000 | purple, white-sign summon |
| 40002, 40013, 60002, 70002 | 91 | 30011, 30010 | purple, red-sign invader |
| 90001 | 82 | 30001, 30000 | blue co-op |
| 10 / 11 | 63 / 74 | none | special |
| `x0013` rows | 83 / 86 / 88 / 91 / 81 / 82 (80013: none) | none | rank variants, no sign |

Column `+0x68` holds a second PhantomParam id (74 for role 1, 181 for 2, 185 for 20001, 186 for 20002, 187 for 30001/30002): the **kind-3 twins** of 80, 81, 85, 86, 88 (same colours, `kind 3`, fade 0.5); the reader of `+0x68` was not found (OPEN).
Columns `+0x38` (63010/63020/63060/63070), `+0x3C` (5510...), `+0x40`, `+0x44` (3311xx), `+0x48` are unread here.

**NpcParam** (`npcparam-phantom-fields.tsv`): `+0x1CC` = 80 (29 rows, the white NPC summons of the High Wall: 27920, 27320, 25120...), 81 (16 invader rows), 60 (26), 88 (3), 92 (4), 74 (2), 187, 33;
`+0x238` = 76 (88 rows, through the team-type route).

## 2. The PhantomParam row and its constants (STATIC)

Row = 0x30 bytes (102 rows, ids 1-209, `Data0.bdt` regulation). Layout from the accessors `FUN_140E35F30..FUN_140E363E0`:

| Offset | Field | Accessor |
|---|---|---|
| +0x00 / +0x04 / +0x08 / +0x0C / +0x10 | float weights `a` of Edge, Front, Diffuse, Specular, Light | `E36020`, `E360B0`, `E35F90`, `E361D0`, `E36140` (`.w`) |
| +0x14.. / +0x17.. / +0x1A.. / +0x1D.. / +0x20.. | rgb bytes (/255) of the same five colours | same |
| +0x24 | fade (float, 1.0 for most rows) | `E35F30` |
| +0x28 | float, no reader found (OPEN; 1.0 or 0.3-0.75) | none |
| +0x2C | **kind** byte (rows have 0, 1 or 3) | `E35F50` |
| +0x2D / +0x2E | IsEdgeSubtract / IsFrontSubtract | `E363C0` / `E363E0` |

`FUN_14022D620` lerps the material block defaults toward the override values by the entry weight (INFERRED 1.0: the entry's `[5]`; PL4 reads it) and `FUN_14022E930` writes `cbMatDynParam`:

```
Edge  (+0x1A0, row 26) = ( r^2 * a * s, g^2 * a * s, b^2 * a * s, a ),  s = -1 if IsEdgeSubtract else 1
Front (+0x1B0, row 27) = ( same with IsFrontSubtract )
Diff  (+0x1C0, row 28) = ( r^2 a, g^2 a, b^2 a, a )      Spec (+0x1D0, row 29), Light (+0x1E0, row 30): the same, s = 1
```
(`r, g, b` = byte/255; the squares look like a gamma-2 decode of sRGB bytes, INFERRED; the subtract constant is `-1.0` at `143D7F0C8`.)

Final constants of the rows the game uses (all 102 in `evidence/I-0067/queries/phantom/phantomparam-decoded.tsv`; (r g b a)):

| Id | kind, fade | Edge (416) | Front (432) | Diff (448) | Spec (464) | Light (480) |
|---|---|---|---|---|---|---|
| **80** white | 0, 1 | (.5 .5 .5 .5) | 0 | (.5 .5 .5 .5) | (1 1 1 1) | 0 |
| **81** red | 0, 1 | (.6 0 0 .6) | 0 | (.2 .2 .2 .2) | (.2 .2 .2 .2) | 0 |
| 60 | 0, 1 | (.6 0 0 .6) | 0 | (.2 .2 .2 .2) | (.2 .2 .2 .2) | 0 |
| 82 blue | 0, 1 | (.007 .077 .308 .5) | 0 | (.1 .1 .1 .1) | (.1 .1 .1 .1) | 0 |
| 83 / 84 pale blue | 0, 1 | (.333 .369 .6 .6) | 0 | (.022 .039 .4 .4) | same | (1.107 2.222 3.076 5) |
| **85** gold | 0, 1 | (.9 .4 0 .9) | (.03 .001 0 .03) | (.3 .046 0 .3) | same | 0 |
| 86 gold-red | 0, 1 | (.8 0 0 .8) | 0 | (.3 .149 0 .3) | same | (4 1.993 0 4) |
| 88 magenta | 0, 1 | (.372 0 .249 .5) | 0 | (.1 .1 .1 .1) | (.1 .1 .1 .1) | 0 |
| **91** purple | 0, 1 | (.092 .236 .6 .6) | 0 | (.1 0 .062 .1) | (.1 0 .035 .1) | (3 0 .904 3) |
| 74 | **3**, .5 | (.394 .394 .394 1) | 0 | (1 1 1 1) | (1 1 1 1) | 0 |
| 76 (NPC route) | **3**, .1 | (1 1 1 1) | 0 | 0 | 0 | (.394 .394 .394 1) |
| 181 / 185 / 186 / 187 | **3**, .5 | = 81 / 85 / 86 / 88 | | | | |
| 33, 40, 41, 75 | **1**, .7 / .2 / 0 / .6 | see the file | | | | |

Reading: Edge is the colour at silhouettes, Front the colour facing the camera (the rim term below); Diff multiplies the albedo; Spec scales the specular and the specular highlight; Light is added with the dynamic light term. The diffuse of the white row 80 is 0.5, so a white phantom is a half-brightness silhouette lit by a full-strength specular.

## 3. How the row reaches the draw state (STATIC, MENUS-MAP 76 completed)

**Assembled models** (`FUN_140CFDE70`, per part, `short [desc+0x44] > 0` and a row): `state+0x87 = 1`; `mulCol = (r x 1.0, g x 1.0, b x 1.0, a x fade)` (the constants at `143D7F380/84/EE10` are 1.0: the part's own colour is kept and only its alpha is multiplied by the row fade); `kind = E35F50(row)`; `if kind == 0: kind = (mulCol.w < 1) ? 1 : 0xFF`; `state+0x84 = (byte)kind`. `state+0x85` is untouched (0). 
**Single models** (`FUN_140D07140` -> `FUN_140D3FE90/D40080`): `X+0x97 = 1`; `iVar3 = kind` (kind 0 gives 1 with `X+0x97`); `state+0x85 = (iVar3 != 0)`, `state+0x84 = iVar3`.
Both feed `FUN_1401A8580`, whose `DC+0x425 = state+0x87` selects `mat+0x78`.

**The routing `FUN_140057A20`** (descriptor kind 2, opaque material; `b84 = state+0x84`, `b85 = +0x85`, `b87 = +0x87`, `fade = state+0xDC = mulCol.w`):

```
if b85 == 0 and b84 in {0x00, 0xFF}:  list = 2 (ordinary);  if b87: list = 0x21, and if fade < 1: 0x29
elif (b85 == 1 or b84 == 1) and fade == 1.0:  list = 0x21 if b87 else 2
else:  list = 0x29
if DAT_144962314 (forced translucent): list = 0x29
```

| Case | b84 | b85 | b87 | fade | List |
|---|---|---|---|---|---|
| host player, plain | 0xFF | 0 | 0 | 1 | 2 (G-buffer, rows 5/6/7) |
| white NPC (80, single model), faded in | 1 | 1 | 1 | 1 | **0x21** |
| same, fading | 1 | 1 | 1 | < 1 | **0x29** |
| player phantom (80/81/85/91...), faded in | 0xFF | 0 | 1 | 1 | **0x21** |
| same, fading | 1 | 0 | 1 | < 1 | **0x29** |
| row of kind 1 (33, 40, 41, 75) | 1 | 0 (parts) | 1 | row fade < 1 | 0x29 |
| row of kind 3 (74, 76, 181...) on a part | 3 | 0 | 1 | any | **0x29** |
| row of kind 3 on a single model | 3 | 1 | 1 | 1 / < 1 | 0x21 / 0x29 |

The native already transcribes this selection (`game_bootstrap.cpp:3324-3333`, `ordinary_category`) from the same bytes; the missing part is downstream (section 5).

## 4. The programme set (STATIC)

- **Two sets per material.** `FUN_140082220` builds the normal set (`mat+0x70`, resolver `+0x60` = `FUN_140D245A0` -> `FUN_140D467E0`) and, when the bundle record's resource flag bit 0 is set or `gxMtd+0x88` bit 26 holds (the 43-name table of MENUS-MAP 75), the alternate set (`mat+0x78`, resolver `+0x68` = `FUN_140082E50`).
  `FUN_140140850/140680` takes `mat+0x78` when `mat+0x78 != 0 and ctx+0x425 != 0`, else `mat+0x70`.
  `82E50` and `D467E0` use the same builders (`A9BA0` allocation, `AA630` for passes 0..16 skipping passes 15 and 16 unless the debug byte is set, `AA4D0`); `82E50` runs the `AA630` loop only if the bundle record (resolver `+0x48`) exists and `rec+0x30 != 0` (else it only names the set with `A9D40`), and always ends with **`A9EF0`**; `D467E0` runs the loop and `AA4D0` unconditionally.
- **`FUN_1400A9EF0`** loops over the 20-entry pass-name table `14473CE60` (Fwd, Velo, Dpt, DptA, FwdSdw, FwdSSdw, Gbuf, Gbuf2, Pnt, PntCube, PntCol, SO, WatDpt, Aof, Aof_Dol, Rsm, Voxelization, FwdDbg, FwdSdwDbg, GBufDbg) and, for the passes with `(pass & ~4) == 0` (0 and 4), stores `repository(<shader>_Phantom_<passname>)` in the pixel table `set+0x18+pass*0x18` and, for the `[A]` and `[AA]` slots of passes other than 2 and 3, `<..>_[A]` / `<..>_[AA]`.
  So **a phantom draw differs from a normal draw of the same material only in the `Fwd` and `FwdSdw` pixel programmes.**
- **A material without `mat+0x78`** (not in the table, no resource flag) keeps `mat+0x70` even when `DC+0x425` is set: such a part is drawn by the plain forward programme in list 0x21/0x29 (no phantom colours). A phantom family without a `_Phantom_` file would get a null pixel slot and `FUN_140140850` returns without drawing (MENUS-MAP 74-75).
- **Inventory** (`shader.tsv`): 64 shaders with phantom files (Equip, SAT_Equip, Skin/FgFace/Eye, Hair, ColDifSpcBump* and Mul/Blend/Emi/Tr/SSS variants, Fur_FDP, Dragon_FDP, MultiBlend Bloody/Equip; each with `_cloth`/`_Cloth`). The offline-cached subset (704 files) was checked: all reachable programmes (`Fwd`, `Fwd_[A]`, `FwdSdw`, `FwdSdw_[A]`) were reflected, 256 files.
- **Pixel shader shape** (DXBC of six families, `evidence/I-0067/queries/phantom-dxbc/`; the hair form is MENUS-MAP 47.1): all read `cb2[26..30]` and none of them read `cb2[26..30]` in the normal `Fwd` (which reads rows 32, 33, 35..39):

```
diffuse = albedo * Diff.rgb
light   = dynamic-light term + saturate(-N.L_) * Light.rgb
spec    = (accumulated specular + saturate(-N.V_)^4 * Light.rgb) * Spec.rgb
t       = (clamp(|N.V|, 0.1, 0.7) - 0.1) * 1.666667
rim     = lerp(Edge.rgb, Front.rgb, t)               // t = 0 at silhouettes, 1 facing the camera
rgb     = max(diffuse * light + spec + rim, 0) * mulCol ; then height and distance fog ; a = albedo.a * mulCol.w
```
  The IBL cube samples of the normal programme are kept (the earlier "no IBL" reading was wrong). `o0` = HDR colour; `SAT_Equip` and `Fur_FDP` also write `o1 = (r8 * cb3[9].xyz, sat(cb3[9].w) * a)`, the other four families write `o0` only, like their normal `_Fwd`. Subtract flags (`s = -1`) darken the rim (rows 3-11 use them).

## 5. Rows, state and order for the native pass (STATIC; row selectors INFERRED where marked)

`57A20` emits, for a model in category **0x21**: one row `{pass 0x21, preparation 0, class mask 0xFEFF}` (all classes except the emissive class 0x100); with the view flag `[model+0x224] bit 15` also `{0x22, 0, 0x100}`; with the pass parameter `+0x10` (the split case of row 5) the game emits `{4, preparation 1, mask 0xFFFD}` and the `{0x21, 0, 0xFEFF}` row, both at priority p (row 0x22 at p + 0.5); the `p +/- 0.5` pair belongs to the generic category branch, i.e. category 0x29 split = `{0x29, 1, 0xFFFF}` at p + 0.5 and `{0x29, 0, 0xFFFF}` at p - 0.5.
For category **0x29**: `{0x29, preparation 0, class mask 0xFFFF}` (the generic row; the normal forward family uses mask 0x12). INFERRED: the shader pass of rows 0x21 and 0x29 is `Fwd` (0), and row 0x22 is the `FwdSdw` pass (4) that `A9EF0` also patches.

| Row | Depth state (FORWARD-DEPTH-NATIVE, `FUN_1400D21F0`) | Order |
|---|---|---|
| 0x21 | INFERRED from the shared pass number: effect pass type 1 = bank `+0x1E0`, LESS_EQUAL, **write ALL** | with the effect passes, after the forward depth copy (INFERRED) |
| 0x29 | test-only `+0x1E8` for effect passes; for models the material policy of FORWARD-DEPTH-NATIVE 4.1 | after the 0x1F rows |

Native today: `NativeModelPassStatus` accepts only category 2 (`model_pass.cpp:35` returns `Category`), rows are 5/6/7/0x1F/0x29/0x34 (`game_bootstrap.cpp:4137`).

## 6. The ground signs (STATIC)

`SosSignMan` (vtable `142847368`), `853800(man, record)`: for an active record without effect it builds the transform from the record position and yaw, looks up the sign effect, calls **`7CB820(handle, id, transform, 8)`** and keeps the handle at `sign+0x10`; an existing effect is moved by `1A87F50`; an inactive record destroys it (`4D2AC0`).
The id: `row = variant*10000 + category`, `category = table[14464C0F8 + type*0x38]` (type = record byte `+0x22`), `variant` = record short `+0x1E` (< 16), then RoleParam `+0x30`. The table (the first 14 entries are used, `category < 14`; first dword): types 0, 3, 4, 6, 10, 12, 13 -> category 1 (summon), 1, 2, 5, 7, 8, 9 -> category 2 (invasion), 11 -> 0 (none).
The NPC signs of the High Wall (`2003[25] 0, npc, region, summoned, dismissed`, MENUS-MAP 59) use type 0, variant 0 (INFERRED, the `2003[25]` handler was not read): role 1, **effect 30001**. All other types come from the network jobs (`CSSosSign*Job`) and do not exist offline.

**The sign effects** (`f0000300xx.fxr`, `frpg_sfxbnd_commoneffects_effect.ffxbnd`, 3 files per sign type, read with SoulsFormats `FXR3`; action ids as in the I-0063 family map):

| File | Size | Renderers (action: count) | Role |
|---|---|---|---|
| `30001`, `30011`, `30021`, `30031`, `30041`, `30051` | 2,816 B | none (`75, 128-132, 199, 399, 400, 500, 10100, 10400, 10500` x1) | **controller**: the id the manager creates; its own id is the header dword at `+0x0C`; it also holds the id of the visual (`+0xAF8` = 30002, 30012, 30022...) and another id at `+0xAB4` (30001 in 30001/30021/30041, 30011 in 30011/30031/30051: meaning OPEN) |
| `30002`, `30022`, `30042` (and `30012`, `30032`, `30052`: 43,760 B) | 39,712 B | **603 x5** (E01 billboards), **607 x1** (E04 distortion), **609 x1** (E06 point light), 1/35/55/84/128-133/199/300/399/400/402/404/500/501 control actions | the visible sign |
| `30000`, `30010`, `30020`, `30030`, `30040`, `30050` | 11,200 B | 603 x2, control actions | the paired effect (RoleParam `+0x34`); its role (appearance state or removal) is OPEN |

The colours are float triplets in the effect data, identical positions across variants (differences between the files of one size class):

| Variant | Billboard colour (rgb, 0..1) found at `+0x2804..+0x2858` / `+0x8C94..+0x90BC` |
|---|---|
| white (30000, 30002) | (1, 1, 0.761) / (1, 1, 0.878) |
| red (30010) | (0.867, 0.031, 0.031) |
| orange-gold (30030; visual 30022: (0.984, 0.745, 0.02); 30020 has another layout) | (0.984, 0.745, 0.0) |
| magenta (30040, 30050, 30042) | (0.957, 0.329, 0.988) |

So the sign needs the effect system only: E01 billboards, E04 distortion and an E06 point light per sign (EFFECTS-IMPLEMENTATION-CHECKLIST steps 1-3, 2.3). The `7CB820` deferral (`DeferNativeSfxSceneRequest`, `game_bootstrap.cpp:7268`, hook at `:9148`) is the gate.

## 7. Implementation checklist (ordered; expected values and witnesses)

| # | Step | Expected | Witness |
|---|---|---|---|
| P1 | **Param tables.** Read PhantomParam (type 0x3A, 0x30-byte rows, binary search by id `FUN_140E36260`), RoleParam, NpcParam fields (`+0x1CC`, `+0x238`) from the game regulation; accessors of section 2 | 102 PhantomParam rows; 42 RoleParam rows; row 80 = fade 1, kind 0, edge byte (255,255,255), weights (.5 0 .5 1 0) | host test against `phantomparam-decoded.tsv` |
| P2 | **Resolver.** Per character the id by the order of section 1; id < 0 -> none | host player none; role 1 -> 80; NpcParam 27920 -> 80; team-route NPC -> 76 | unit test on the tables; PL1 |
| P3 | **Constants.** `cbMatDynParam` rows 26-30 from the row with the formulas of section 2 | row 80: Edge (.5 .5 .5 .5), Front 0, Diff (.5 .5 .5 .5), Spec (1 1 1 1), Light 0; row 91: Edge (.092 .236 .6 .6), Light (3 0 .904 3); subtract rows 3-11 negative | unit test of the 102 rows; PL4 for the weight |
| P4 | **Draw state.** For a model with id: `state+0x87 = 1`; assembled parts: `mulCol.w *= fade`, `state+0x84` by the kind rule; single models: `+0x84/+0x85` by `D3FE90` | table of section 3 | PL2 (the bytes on a summoned NPC) |
| P5 | **Category.** keep the existing selection (`game_bootstrap.cpp:3324`) and let it reach the pass | white NPC 0x21, fading 0x29; host player 2 | PL2 |
| P6 | **Alternate set per material.** Build `mat+0x78` only for the 43 names + resource-flag materials; select it per draw when `state+0x87 != 0` | a material outside the rule keeps `mat+0x70` | PL3 |
| P7 | **Programmes.** For `Fwd` (0) and `FwdSdw` (4) with the alternate set: pixel `<shader>_Phantom_<pass>[_[A]]`, vertex unchanged; every other pass unchanged | the six dead `_Phantom_` pass files never selected | unit test on `SelectModelPrograms` with an intent `phantom = true` |
| P8 | **Rows.** Add model rows 0x21 (prep 0, mask 0xFEFF), 0x22 (prep 0, mask 0x100, only with `+224` bit 15), 0x29 generic mask 0xFFFF for category 0x29; accept categories 0x21/0x29 in `EvaluateOrdinaryModelPass` | an NPC of 80 appears in 0x21; classes 0x100 in 0x22 | native run, PL5 |
| P9 | **State of the rows.** 0x21: depth test LESS_EQUAL + write, opaque blend; 0x29: alpha blend per material, test (write per material); targets as the programme writes them (HDR; the second target for the SAT and Fur families) | no G-buffer write for a phantom | PL6 |
| P10 | **Order.** 0x21 with the effect passes after the depth copy (INFERRED), 0x29 with the forward rows | | PL7 |
| P11 | **Fade.** `mulCol.w` = `a x fade` reaches instance row 7; fade-in drives the 0x29 -> 0x21 switch | 0x29 while fading, 0x21 at 1.0 | PL2, picture |
| P12 | **Sign records** from the NPC events (`2003[25]`) into a native `SosSign` list with type 0, variant 0 | one record per active white sign | emevd dump of MENUS-MAP 59 |
| P13 | **Sign effect.** Resolve the effect id as in section 6 (type -> category, variant, RoleParam `+0x30`) and create it through the effect system at the record transform | NPC sign: effect **30001**, at (6.7, -6.3, 87.1) +/- 1 m for sign 1 | S1/S2 of the I-0068 note |
| P14 | **Sign content.** The controller 30001 must spawn 30002 (5 billboards, distortion, point light) | white-yellow billboards (1, 1, 0.76..0.88) | picture; PL9 |
| P15 | **Forced types.** A native-only test feeds ids 81, 85, 91 to the white NPC | b2 rows of section 2; red rim, gold, purple | host test + picture |
| P16 | **Argo witness.** High Wall, Ember, Vordt alive: the sign within 10 m; summon the NPC `c0000_0007` | sign effect 30001; the NPC draws list 0x21 with `_Phantom_Fwd` | PL1, PL5, PL8 |
| P17 | **Negative.** Player hair and eyebrows keep `<family>_Fwd` | | PL3 |
| P18 | **Debug layer clean** with a phantom on screen | 0 errors | native run |

## 8. Live rows (none was taken)

| Row | What to read | Expected |
|---|---|---|
| PL1 | `entity+0xC38` / `desc+0x44` of the summoned NPC; the resolver result | 80 |
| PL2 | `state+0x84/85/87/DC` of its draw group, before and after the fade | (1,1,1,1.0) single model; list 0x21 then 0x29 during the fade |
| PL3 | for each material of the NPC: does `mat+0x78` exist; which pixel programme `FUN_140142430` binds | `_Phantom_Fwd[_A]` for the 43-name / flagged families |
| PL4 | `cbMatDynParam` rows 26-30 of one draw, and the override entry weight `[5]` | section 2 values (white: (.5 .5 .5 .5) ...) |
| PL5 | the engine rows emitted for the NPC (pass, preparation, mask) | `{0x21, 0, 0xFEFF}` (+ `{0x22, 0, 0x100}`) |
| PL6 | blend and depth state of a row-0x21 draw, targets | write ALL; targets per family |
| PL7 | order of the 0x21 draw against the depth copy and the effect passes | after the copy |
| PL8 | `SosSignMan` record: type byte, variant, effect id, handle | 0, 0, 30001 |
| PL9 | the sign effect's draws (E01 x5, E04, E06) | |
| PL10 | `PhantomParam` entry weight and the reader of row `+0x28` and of `RoleParam +0x68` | OPEN items of section 9 |

## 9. Open items

- OPEN: the override-entry weight `[5]` (taken as 1.0), the reader of PhantomParam `+0x28`, of RoleParam `+0x68` and `+0x34`, and of `+0x38/+0x3C/+0x44/+0x48`.
- OPEN: the shader pass of rows 0x21 / 0x22 / 0x29 (taken as `Fwd` / `FwdSdw` / `Fwd`) and the position of row 0x21 among the effect passes.
- OPEN: which phantom parts have `mat+0x78` (the 43 names and the resource-flag SAT bundles); PL3.
- OPEN: the weapon-enchant id table of `FUN_1405B31B0` (21 entries) and the covenant names behind the roles (colours only here).
- Not offline: invasion, gold, magenta and purple signs and phantoms (network jobs).

## Exports

`decompiled/DS3-1152-F-` + `00E35EF0`, `00E35F30`, `00E35F50`, `00E36260` (existing), `00E35F90`, `00E36020`, `00E360B0`, `00E36140`, `00E361D0`, `00E363C0`, `00E363E0` (exported for this note), `0022D620`, `0022E930`, `009CAF30`, `00CFDE70`, `00057A20`, `00082E50`, `00D467E0`, `000A9EF0`, `0088A950`, `00146850`.
Tool: `Get-PhantomTable.ps1` (decodes `phantomparam.tsv` and `roleparam.tsv` into the constants above). Evidence outside Git: `evidence/I-0067/queries/phantom/` (`phantomparam-decoded.tsv`, `roleparam-decoded.tsv`), `evidence/I-0067/queries/phantom-dxbc/`.
