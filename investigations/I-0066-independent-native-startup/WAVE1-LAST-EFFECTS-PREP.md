# Wave 1, last missing effects: static preparation (phantom colours, the summoned NPC phantom, the W6 glow capture)

Written 2026-10-17 for the pilot of the « Lenteur du Bourg » thread, at Thomas's request (« ils peuvent préparer les derniers effets manquants de la vague 1 »). STATIC only: nothing was launched, no save or option file was touched, `main` was not touched.
Mechanisms, never single cases (D-0161). Sources: [PHANTOMS-NATIVE](PHANTOMS-NATIVE.md), I-0067 [PHANTOMS-SIGNS-NATIVE](../I-0067-static-menus-subsystems-map/PHANTOMS-SIGNS-NATIVE.md) and MENUS-MAP 41-42, 47, 55-56, 59, 74-76,
[GLOW-WAVE1-READY](GLOW-WAVE1-READY.md), I-0068 GLOW-CHAIN-NATIVE, [W10-NATIVE-PASSES](W10-NATIVE-PASSES.md) (the Reference capture pattern), the module (`native/frame/game_bootstrap.cpp`, `native/renderer/scene_pass.cpp`, `glow_stage.*`), the exports in `decompiled/`,
and new read-only decompilations (`FUN_1408A9E50`, `1408BE6C0`, `140C32190`, `140C48910`, `140D07140`, `140D3FE90`, `140D40080`, `140C4AF10`, `140C49210`; a read-only mirror, no analysis) and the RTTI slot table (`evidence/I-0067/rtti/vslots.tsv`).
Status words: **STATIC** read in code or data, **INFERRED** deduced, **OPEN** not known statically.

## 0. Summary

| Subject | State today | What this note adds | First live step |
|---|---|---|---|
| Red, yellow (gold), purple phantoms | the mechanism is **data**: PhantomParam row -> five colours -> `cbMatDynParam` rows 26-30, one programme for every colour. White (80), red (81) and kind 3 (74) match the original (Native1222-1224) | the colour table with the rows a role gives, the expected constants, what the HDR terms of the gold and purple rows do, the forced-id witness list, what is **not** colour (signs, row 0x22, order 0x21) | `witness-phantom.txt` with 85, 86, 91 (then 88, 82, 83, 181), Reference and Native in the same frame |
| Summoned NPC phantom, face (Sword Master at Vordt's sign, no face natively, Thomas) | never witnessed natively; PHANTOMS-NATIVE says "c0000_0007 is ChrAsm". **The code says the opposite for an `EnemyIns`** (section 2.2) | the two routes of the phantom id, the chain of a head in each, a ranked list of causes for a missing face, the exact reads that settle it | read one c0000 NPC at Firelink: draw-entity class and part list, **no summon needed** |
| W6 glow | ten passes implemented and drawn after the tone map and grading, structurally witnessed (Native1550-1561), **image witness open**, GL-8 (`glow-verify`) never coded | an exact-oracle design (Reference capture of the original's ten draws, offline test on our stage, same pattern as T12-T17), GL-8 as a readback, the family witnesses with positions | `glow-capture.flag` in a Reference run at Firelink (c2150) |

**Context (Thomas, 2026-10-16 23:57, confirmed through the coordinator):** in the Argo save (slot 0), in front of Vordt's arena, the character stands at a summon sign; once summoned, the NPC (« Maître d'armes » in French, the Sword Master) **has no face in the native build**. The repository's notes say nothing about it, so the case was not known to the static threads before this message. The sign next to the arena is sign 1 of MENUS-MAP 59 (`3002780`, (6.7, -6.3, 87.1)), whose NPC is `c0000_0007`, NpcParam 27920, PhantomParam 80 (white); that this NPC is the Sword Master is INFERRED from the place (the name is Thomas's; no name table was read). Whether the original shows the face was not stated (to compare, R3). Section 2 prepares the chain of a summoned NPC's face and of its phantom rendering for every NPC, with this case first.

## 1. Red, yellow and purple phantoms (one mechanism for every colour)

### 1.1 What decides the colour (STATIC, PHANTOMS-SIGNS-NATIVE 1-4, re-checked against the native code)

A draw is a phantom when its state byte `+0x87` is 1: the model or part has a PhantomParam id >= 1 whose row exists. The id comes from the resolver `FUN_14088A950` (`chr+0x1F38`, `chr+0x1A58`, NpcParam `+0x1CC` / `+0x238`, else RoleParam of the role); `FUN_1409CAF30` pushes the seven `ExParam_Phantom*` overrides and calls slot 214.
The row (0x30 bytes, 102 rows) gives five colours with five weights, a fade, a kind and two subtract flags; `FUN_14022E930` turns them into `cbMatDynParam` rows 26-30 (`Edge`, `Front`, `Diff`, `Spec`, `Light` = `(r^2 a s, g^2 a s, b^2 a s, a)`), and the **same** `<family>_Phantom_Fwd[_[A]]` pixel programme draws every colour (the alternate set differs from the normal one only in the `Fwd` and `FwdSdw` pixel slots). So a new colour is a new row, never new code, provided the family has its alternate set.

| Colour | PhantomParam id | Roles that use it (RoleParam) | Edge | Front | Diff | Spec | Light | Kind, fade |
|---|---|---|---|---|---|---|---|---|
| white | 80 | 1, 50001, 80001 (co-op) | (.5 .5 .5 .5) | 0 | (.5 .5 .5 .5) | (1 1 1 1) | 0 | 0, 1 |
| **red** | 81 (and 60 for NPCs) | 2, 10002, 50002, 80002, 90002 (invaders) | (.6 0 0 .6) | 0 | (.2 .2 .2 .2) | (.2 .2 .2 .2) | 0 | 0, 1 |
| blue | 82 | 90001 | (.007 .077 .308 .5) | 0 | (.1 .1 .1 .1) | (.1 .1 .1 .1) | 0 | 0, 1 |
| pale blue | 83 / 84 | 10001 | (.333 .369 .6 .6) | 0 | (.022 .039 .4 .4) | same | **(1.107 2.222 3.076 5)** | 0, 1 |
| **gold ("yellow")** | 85 | 20001 (co-op) | (.9 .4 0 .9) | (.03 .001 0 .03) | (.3 .046 0 .3) | same | 0 | 0, 1 |
| gold-red | 86 | 20002 (invader) | (.8 0 0 .8) | 0 | (.3 .149 0 .3) | same | **(4 1.993 0 4)** | 0, 1 |
| magenta | 88 | 30001, 30002 | (.372 0 .249 .5) | 0 | (.1 .1 .1 .1) | (.1 .1 .1 .1) | 0 | 0, 1 |
| **purple** | 91 | 40001, 60001, 70001 (summon), 40002, 40013, 60002, 70002 (invasion) | (.092 .236 .6 .6) | 0 | (.1 0 .062 .1) | (.1 0 .035 .1) | **(3 0 .904 3)** | 0, 1 |
| kind-3 twins | 74, 181, 185, 186, 187 | RoleParam `+0x68` (reader not found, OPEN) | as 80, 81, 85, 86, 88 | | | | | **3, 0.5** |

The roles' colours are INFERRED from the values; the covenant names are not in the repository. Only the white NPC sign exists offline: gold, magenta, purple and red signs and phantoms of other players come from the network jobs, so **in a normal offline play only white and the NPC rows (60, 74, 76, 80, 81, 88, 92, 187, 33) can appear**; the other colours are reached by forcing the id.

### 1.2 What native does (STATIC + PHANTOMS-NATIVE live results)

- **ChrAsm parts** (the player, replay ghosts): the original's `FUN_140CFDE70` runs at draw time, which native does not, so native transcribes its stack-state change per part (`game_bootstrap.cpp` P4: `+0x87 = 1`, alpha times the row fade, kind rule) and reads the id from the part descriptor `+0x44`. The routing to rows 0x21, 0x22 and 0x29 and the constants (P3, rows 26-30) are done and unit-tested on white.
- **Single models** (a `SprjModelDrawEntity`): the id reaches the entity through genuine CPU code (`FUN_140C4AF10` -> `FUN_140D066F0` -> `FUN_140D07140` sets `entity+0xC38` and, through `FUN_140D40080` and `FUN_140D3FE90`, the state holder bytes `+0x97` (phantom), `+0x95`, `+0x94` (kind)); these run natively because the engine's update runs. Native reads the same group bytes
  (`group+0x87`, `+0x84`, `+0x85`, lines 4294 and 6700) for its category selection and for the alternate programme (line 7783), so a single-model phantom should already select 0x21/0x29 and `_Phantom_Fwd` **with no transcription** (INFERRED: never witnessed; PHANTOMS-NATIVE listed single models as "not covered" for want of a witness, not of a code path). Its constants (P3) depend on the material's phantom block `material+0xD0`, which for single models is initialised by `FUN_141070` (not run natively; an explicit policy gives the same initial values to a material whose family has its alternate programme).
- **Witnessed**: Native1222 (80) against Reference1218; Native1223 (81) against Reference1223; Native1224 (74, kind 3, fade 0.5) against Reference1224, on the player through `witness-phantom.txt`. Other ids were never drawn.

### 1.3 What the gold and purple rows ask of the pipeline (new, from the constants)

1. **`Light` above 1.** Rows 83/84 (1.1, 2.2, 3.1, alpha 5), 86 (4, 2, 0, alpha 4) and 91 (3, 0, .9, alpha 3) add `saturate(-N.L) * Light.rgb` to the diffuse light and to the specular (`pow(saturate(-N.V), 4) * Light.rgb`) before `* mulCol`: the phantom's pixel can exceed 1 in the HDR target. Natively the forward result goes to the HDR scene (R11G11B10 or RGBA16F as the family writes), so the glare, the glow's extraction (GB5 stays untouched: forward draws with one output do not write it, but the SAT and Fur families write `o1`) and the tone map see those values.
   The witness for 86 and 91 therefore tests the **chain after the pass** too: compare the lit HDR and the final image of the same frame (the W10 comparison script, `Compare-YebisReference.ps1`), not only the programme output.
2. **Subtract rows 3-11** use `s = -1` (a negative constant); row 85 is the only one with a non-zero `Front`.
3. **The squares.** `(r g b)` are squared (gamma-2 decode of the row's bytes, INFERRED); a unit test that recomputes rows 26-30 from the bytes of every row used would catch a wrong rounding. The existing `PhantomParameters` unit test covers white and the subtract sign only: add rows 81, 82, 85, 86, 88, 91 with the values above (from `evidence/I-0067/queries/phantom/phantomparam-decoded.tsv`; the table of 1.1 is the expected output).
4. **Alternate-set coverage.** The pixel programme exists for 64 shaders; a family without `_Phantom_Fwd` keeps `mat+0x70` and draws plain (no phantom colour) or, if its alternate set exists with a null slot, **is not drawn** (`FUN_140140850` returns). Native logs `native_phantom_forward_fallback` when the alternate programme is not resolved (the plain `Fwd` is then drawn): count it per colour run; a non-zero count on the face or hair of a phantom is a lead for section 2.

### 1.4 Witness plan (Reference and Native, the same frame; the player, so no Ember or summon)

| Run | `witness-phantom.txt` | Expected |
|---|---|---|
| T-P1 | `85` | gold rim and body, `Front` term: the original's picture; no `fallback` rows on the body parts |
| T-P2 | `86` | gold-red with the bright `Light` term: the HDR above 1 reaches the tone map the same way |
| T-P3 | `91` | purple, same as 86 for the HDR |
| T-P4 | `88`, `82`, `83` | magenta, blue, pale blue (`Light` 5 in alpha) |
| T-P5 | `181` or `185` | kind 3, fade 0.5: category 0x29 for every part; compare with 74 (Native1224) |
| T-P6 | `0` releases | the player returns to the ordinary draw (G-buffer rows) |

`Run-PilotPair.ps1 -Phantom id` and `Pilot-LoadGameThomas.ps1 -Phantom id` exist. Rows: `native_phantom_rows` (the override rows a phantom-capable material sees; first four), `native_phantom_forward_fallback`, `witness_phantom`. Pass: the picture matches the original in the same frame on the phantom's pixels (mean and maximum difference on the silhouette), 0 debug errors, no fault.
Cost note: a forced phantom id turns the whole player into rows 0x21/0x29; read the frame time against the ordinary frame.

### 1.5 What is not colour (the remaining wave-1 gaps of the phantoms)

- **Row 0x22 and `FwdSdw`** (the emissive class 0x100, only with `+224` bit 15): the explicit policy is `Fwd`; the native has no shadows for the alternate set. OPEN until the original draws a phantom with a class-0x100 mesh (a weapon with an emissive enchant).
- **The position of row 0x21 among the effect passes** (P10): taken as after the forward depth copy, never measured (PL7).
- **Signs.** The sign effects (`30001` white ... `30051` magenta, controller + visual `3000x2` with five billboards, one distortion, one point light) need the effect system (E01, E04, E06). Offline only the white NPC sign exists. A native witness of the other colours needs a test aid that creates the effect id at a position (`7CB820(handle, id, transform, 8)`, deferred by `DeferNativeSfxSceneRequest`, `game_bootstrap.cpp:7268`): not planned unless Thomas wants the signs; the colours are in the effect data (red (.867 .031 .031), gold (.984 .745 0), magenta (.957 .329 .988); white (1 1 .76..0.88)).
- **The fade-in**: the 0x29 -> 0x21 switch at `fade == 1.0` is read from the same bytes; PL2 (the bytes over time) was never taken.

## 2. The summoned NPC phantom, and its face

### 2.0 The case that was reported, and what it already says (INFERRED from the confirmed context)

- The NPC **is drawn** (Thomas saw a summoned character without a face), so its model passes whatever gate lists it, its body takes the phantom or plain draw, and only the face is missing. That rules out the causes that remove a whole model (a FaceGen texture that never reports ready keeps a **menu view's** whole model unlisted, MENUS-MAP 25; in the world the head part would be missing alone). The remaining causes are per part or per material: C3, C4, C5, C6 and the "no FaceGen for an NPC" branch of 2.3 below.
- The reach: Argo's save, the Ember state, Vordt alive, the requirement flag 13005392 and the player within 10 m of the NPC standing at the sign point (MENUS-MAP 59, which gives the conditions and the 2003[25] event); the sign is where Thomas stands. `Pilot-LoadGameThomas.ps1 -Teleport "6.7,-6.3,87.1" -Look` places the camera there (it exists, `witness-teleport.txt`); the summon itself needs the in-game action (interact with the sign), which a pilot can press (`E`).
  Without the Ember, force the phantom state on the same NPC instead (2.4, the aid), which tests the **rendering** of a phantom face; the summon adds the NPC's **own** build (its ChrAsm or model creation after the event moves it into the arena, EMEVD `20005714`).
- Same-frame comparison: Reference with the same summoned NPC (R3), then Native.

### 2.1 Where such an NPC is, and how to reach one without a summon

The High Wall NPC summons are MSB enemy parts of model `c0000` (`m30_00`: `c0000_0007` NpcParam 27920 at the sign point (6.7, -6.3, 87.1), `c0000_0008` 27320 at (25.0, -25.5, 52.3), `c0000_0009` 25120; MENUS-MAP 59). The sign needs an Ember, Vordt alive, the requirement flag and the player within 10 m; Argo's start is 40 m from sign 1, but **`c0000_0008` stands 6 m from Argo's start** and is loaded (MENUS-MAP 69 W1).
`c0000` is the generic human model: the MSB census (`evidence/I-0067/queries/chr/msbenemy.tsv`) lists dozens of c0000 enemy parts in every zone, with NpcParam rows 21000-29xxx. At **Firelink (`m40_00`) there are 23 of them**, among them `c0000_0012` (NpcParam 27300) at (279, -37.4, 638.4), `c0000_0013` (27321) at (124.9, -64.2, 547.3), `c0000_0030` (25300) at (285.7, -47.5, 606.9), `c0000_0031` (26103) at (301.1, -12.8, 629.3); the others sit at (273, -98.4, 602.6) until an event moves them.
So the face question can be studied on a normal c0000 NPC at Firelink or the High Wall with no Ember and no summon: the NPC is a human with its own head whatever its phantom state.

### 2.2 The class of a c0000 NPC decides the chain (STATIC, new)

| | `PlayerIns`, `ReplayGhostIns` | `EnemyIns` |
|---|---|---|
| Model builder (vtable slot 9) | `FUN_1408BE6C0` (`FUN_1408CAA50` for the ghost) -> `FUN_140C32190` -> `FUN_140C44D20` -> `SprjAsmModel` and `SprjAsmModelDrawEntity` | `FUN_1408A9E50` -> `FUN_140771E10` (`ChrModel`) -> `FUN_140C49210` -> `FUN_140D05C80` (`SprjModelDrawEntity`) |
| Phantom id (slot 214) | `FUN_1408C0640`: mode 0 loops **all 44 parts** (`FUN_140C48910(asm, part, id, fade)` stores the id and fade in each part descriptor, `+0x44`) | `FUN_1408AA630`: `[chr+0x48]+0x120 = id`, `+0x74 |= 0x20`; the next `FUN_140C4AF10` calls `FUN_140D066F0` -> `FUN_140D07140` (`entity+0xC38`, state bytes `+0x97/+0x95/+0x94`) |
| Per-part state | `FUN_140CFDE70` at draw time (native: P4) | none (one model, one state) |

Facts that bound the question (all STATIC): `FUN_140C32190` has **one** code caller (`FUN_1408BE6C0`); `FUN_140C48910` has one caller (`FUN_1408C0640`); `FUN_1408A9E50` never builds a `SprjAsmModel`. So an NPC that is an `EnemyIns` is **never** a ChrAsm model, and the phantom colour reaches it by `entity+0xC38`, not by part descriptors.
Two statements of earlier notes meet here and disagree: MENUS-MAP 64 §6 says "human enemies and NPCs (c0000) use ChrAsm" (INFERRED) and PHANTOMS-NATIVE says "the summoned NPC `c0000_0007` is ChrAsm (covered by P4)"; the chrscan of the game data says `c0000.flver` has **136 bones and no mesh** (`evidence/I-0067/queries/chr/chrscan.tsv`), which is what a ChrAsm skeleton looks like and **not** what a single-model character needs.
The construction code does not settle it: the MSB class that spawns `c0000_xxxx` may be a `PlayerIns`-family spawn, or the `EnemyIns` model builder may merge equipment FLVERs into the one `ChrModel` through `FUN_140772440(model, equipment, ...)` (its arguments come from the model resource `+0xA0/+0xB0`, not decoded here). **OPEN, and decisive: one live read answers it** (2.4, read R1).

### 2.3 The chain of a head, both routes (STATIC, MENUS-MAP 24-27, 44, 56, 60 and PHANTOMS-SIGNS-NATIVE)

1. **A ChrAsm head** is slot 0 (`part+0x118 == 0`): the part file `HD_`/`FC_`/`FG_`..., FaceGen objects in `part+0x130` created by `FaceGenManImp` (`[14477DA08]`): a shape instance (`+0x10`, also `model+0x758`) and a `FrpgFgTextureIns` (`+0x18`, also `model+0x770`) whose GX texture is made at run time (format 0x1D, not a TPF). The per-frame update `FUN_140C3F980` fills it; the part is ready only when `FrpgFgTextureIns+0x91 == 1` (`FUN_140C3F720`); a texture that never reports ready keeps the whole model out of the scene's list (STEP_Init_Play gate `FUN_140C3B340`). The shape is a vertex-stream patch (`SprjFgModelVertexStreamPatch`, `FUN_14051B940`) written into a dynamic vertex buffer per FLVER mesh (the native uses a per-instance upload ring). Face material families `FgFaceIbl_Bloody` and `FgFaceSSSIbl_Bloody` are in the 43-name table, so they have the alternate set and `FgFaceIbl_Bloody_Phantom_Fwd` exists (DXBC checked, MENUS-MAP 75 and PHANTOMS-SIGNS-NATIVE 4).
   The kind-0 part also waits for its FaceGen resource in the parts gate `FUN_140C42620` before it is committed. Natively the FaceGen provider answers **only the `FG_face_gen` key** (Native1181), the only key a player's head asks for.
2. **A single-model head** (an `EnemyIns` c0000) is one FLVER, or merged ones: no FaceGen object exists for it unless the model builder creates one (not seen in `FUN_1408A9E50`'s callees); its face is then plain textures of the resource, and a phantom state is one state for the whole model.
3. **The phantom programme for the face** is the same alternate set (`FgFaceIbl_Bloody_Phantom_Fwd`, `EyeIbl_Phantom_Fwd`, `HairIbl2`...), IBL kept, five colours from rows 26-30; there is no face-specific phantom code.

### 2.4 What can make a face missing: ranked causes and the read that separates them

| # | Cause | Where it shows | Original or native |
|---|---|---|---|
| C1 | the head is **absent by data**: the NPC's helmet hides the face, or the model has no head mesh (the c0000 body has none, 2.2) | Reference shows the same | both: not a defect |
| C2 | the **FaceGen texture never reports ready** for this NPC (ChrAsm route): the model never enters the scene, or its head part is not committed | head or whole NPC missing; `FrpgFgTextureIns+0x91`, `part+0x130` | native (the NPC's face is not a player profile face: the provider answers only `FG_face_gen`) |
| C3 | the face material's **alternate programme is unresolved** natively (`native_phantom_forward_fallback`, "alternate's textures not ready") or its alternate set has a null slot | the face draws plain (not white) or, in the original, not at all (`FUN_140140850` returns) | native (fallback) or both (null slot) |
| C4 | the face's **vertex patch** is not applied (morph into the dynamic vertex buffer): a face at the bind shape, possibly collapsed | a flat or degenerate head | native (per-instance ring) |
| C5 | the head part's **descriptor `+0x44` is 0** (the id is written to the parts of a PlayerIns only through mode 0 of `FUN_1408C0640`; a different mode skips parts) | the head keeps the ordinary draw while the body is white | both |
| C6 | the phantom **fade** and category: the head is a separate fading controller per part (MENUS-MAP 64): rows 0x29 vs 0x21 differ between parts | the head is translucent or late | both |

Reads (read-only, through the existing observers; each row is per part):

- **R1 (both modes, no phantom needed):** for one c0000 NPC at Firelink or the High Wall: the **draw-entity vtable** (`142947DB8` Asm or `142948EF8` single), the chr vtable (`1428545D8` EnemyIns, `142857158` PlayerIns), and for a ChrAsm the part list (descriptors `+0xB00`, stride 0x50: binding `+0x10`, flags `+0x38`, id `+0x44`; the slot-0 part's `+0x130` and `[+0x130]+0x18+0x91`). `native_character_survey` / `native_character_early` already log model data name `+2B8`, bone count, bounds and the refusal reason per `SprjModelDrawEntity` (they omit `SprjAsmModelDrawEntity`, which is how a c0000 NPC would be missing from them).
- **R2 (phantom state):** force the id on the NPC (the aid below), then read `state+0x84/0x85/0x87/0xDC` of each part (ChrAsm) or of the holder `entity+0x230` (`+0x94/+0x95/+0x97`, single), the materials' shader names, and per draw: `mat+0x78` present, the pixel programme bound, drawn or returned.
- **R3 (the original):** the same R2 in the Reference run, plus a hook on `FUN_140140850` counting calls that return without drawing in a phantom context, per material name. A face that is absent in the original (C1, null slot) ends the question.
- **The aid:** `witness-phantom.txt` writes `chr+0x1F38` of the **player** only (`ApplyWitnessPhantom`). To force an NPC, extend it with a target: `id [entity-id]`, found by walking the WorldChrMan sets (`manager+0x1C8` sets, `manager+0x1D0+0x18*k`, element count at the set's first dword) for the chr whose entity id matches (`4000500` = `c0000_0012` at Firelink; `3000191` = `c0000_0008` at the High Wall); the entity-id offset in `ChrIns` is **OPEN** (read it from `FUN_14078C480`'s neighbours or from the survey). For an `EnemyIns` the writable source is still `chr+0x1F38` (resolver order, STATIC).

### 2.5 Implementation consequences (by family, after R1)

- If the NPCs are **single models**: native already follows the engine's state bytes; the work is the material phantom block of single-model materials (the family policy of section 1.2) and a witness. Nothing per NPC.
- If the NPCs are **ChrAsm**: P4 applies; the open item is how the ids reach the parts of an `EnemyIns`-class NPC (mode of slot 214), then C2-C5.
- Either way the face is a **family** (head part + face material + FaceGen readiness), to be witnessed once for one NPC and once for the player's phantom head (Native1222-1224 showed the player's silhouette; the face was not singled out).

## 3. The W6 glow capture

### 3.1 State (STATIC, GLOW-WAVE1-READY implementation log and `scene_pass.cpp`)

The ten passes are implemented (`GlowStage`), wired after the tone map and the grading and before the UI (`scene_pass.cpp:2394`, into the 8-bit colour; consistent with the original's `FUN_1401BB810` after YEBIS), on by default (`no-glow.flag` removes it), forward draws of programmes with `SV_Target1` write GB5 (`Forward2`, 74 of 333 forward pixel programmes). Live: ten passes a frame, 0 debug errors, sizes 1280x720, 320x180, 160x90, 80x45, 40x23 and the scales 1, 1.5, 0.5 (Native1550-1561).
**Open: the image witness.** No run has shown the glow's contribution in an image: the m37 pair (Native1556/1557) differs only where the scene animates, and GB5's alpha was never measured. **GL-8 (`glow-verify.flag`, `glow-factor.flag`) is not in the code** (`grep` finds neither flag). Since then the Firelink decal stop is gone (m40 runs at 60 frames/s), so the planned witness place is reachable.

### 3.2 What "captured" should mean: an exact oracle, as for the YEBIS passes

Comparing two runs is weak (the sky and the animation differ between runs, the sheet says so). The W10 passes were closed with **captures of the original's own inputs and outputs** and an offline test that feeds the inputs to our stage and compares words (T12, T14-T17; `Compare-YebisReference.ps1`). The glow admits the same, and its draw structure makes it easier:

- **Reference capture (new `glow-capture.flag`, Reference runs only).** Observe `FUN_1401BB810` (already hooked as `ObserveGlowFilter`, `game_bootstrap.cpp:3058`, `15346`) and the quad draw `FUN_14007F820` (4 vertices, called by every pass), with a thread-local "inside the glow" flag set around the original. After each quad draw, copy through staging textures of the **GX immediate context** (`[4961AB0]+0x10 -> +0x8`, K-0004; the YEBIS capture uses `[HAL device+0x10]`, which is a different context object) the bound shader resources and render target, exactly as `CaptureYebisDraw` does (it already handles R11G11B10, RGBA16F, RGBA8 and the blend state row): per pass `t0` and the output, the blend state, and once per frame the three constant buffers `cb0/cb1/cb2` (the blur taps and `cbGlowParam`).
  Ten draws, ten pairs. GB5 (`t0` of pass 1) is RGBA16F, the sources and levels R11G11B10, the last output the 8-bit colour: all in `CaptureTexelBytes`. Frame choice: the first detailed frame after a settle count, as the tone-map capture does; rows `reference_glow_capture` (pass, format, size, bytes).
- **Offline test T8** (in `glow_stage_test`, next to T4-T7): feed the captured GB5 and the six values to `GlowStage` on WARP and hardware and compare each pass's output word for word with the capture; the same adapter and driver give exactness (the tone-map tests required exactness only on the same adapter; the glow T5's 2/255 was against a CPU model, not against the original's own output). The programmes are the original's DXBC, pinned by SHA-256, so any difference lies in the state (viewport, sampler, blend) or the constants.
- **GL-8 as a native readback.** `glow-verify.flag`: every 120th frame, read back S and L0..L3 (means, maxima) and GB5's alpha maximum into one row, **and** dump the raw targets in the Reference capture's file format so one compare script serves both (`Compare-GlowReference.ps1`, from `Compare-YebisReference.ps1`). `glow-factor.flag` forces `[[GXRS+0x5368]+0x80]` for the gate rows (G6).
- **Gate rows to read in the original first** (GL1-GL3, GL8): the six values and the factor (`reference_glow_record` exists), GB5's alpha over an emissive and a plain-emissive model, the engine's residency of the four programmes.

### 3.3 Where and what to look at (STATIC, GLOW-WAVE1-READY 8; positions from the MSB census)

| Family | Model | Where | Expected |
|---|---|---|---|
| G-buffer Glow (56 families, `o5.w = FC_GlowScale.w`) | `c2150` (`EmiIblGlow_Bloody`) | Firelink `m40_00` (309.3, -35.4, 617), the Eta m40 zone save; also `m45_00` (seven `c2150` parts, e.g. (17.7, 26.7, -33.8)) and `m51_00` (-403.6, -98.2, -243.9) | GB5 alpha about 1 over the emissive texels, S non-zero there, a halo at three scales (weights 1.5 and 0.5); none with `no-glow.flag` |
| Same, second model | `c5270` | `m30_00` (28.2, -9.8, 158.1) | the same |
| Plain emissive (no glow) | `c5110` at Firelink (142.9, -68.9, 594.6); `c1090` at `m30_00` (68.4, 41.4, -8.1) and `m30_01` | bright, **no halo**, GB5 alpha 0 | the negative control, same frame as the positive one if they share a view |
| Forward Glow (276 programmes with `o1`) | `SAT_Equip*_Fwd`, `Veill_Curtain`, cinder `_Fwd` | wherever such materials draw | `native_forward_gb5` counts, GB5 changed only under forward geometry |
| FFX `...GlowD` (44) | flames, auras, spells | waits for the effects phase (the campfire flames are the other static thread's subject) | not part of W6 |
| Map `M[ARSN]_em` bowls | High Wall torch bowls | after the map families are admitted | halo on the bowls |

The Firelink bonfire flames (FFX) are **not** a G-buffer Glow witness: they only gain a halo once the effects path writes the second target. Do not read their absence against W6.
Order of the live work: a Reference capture at c2150 (positive) and at c5110 (negative) from the zone save with the teleport aid (`witness-teleport.txt`), T8 offline, then a native `glow-verify` run at the same places, then the image pair (same frame, same camera, `no-glow.flag` as the control).

### 3.4 Risks (from GLOW-WAVE1-READY 9, re-read)

The taps are one **target** texel apart (not source); R11G11B10 has no alpha and clamps at 65504; the debug layer's warnings are fatal; the final composite adds into the 8-bit colour **after** the grading (a different position would change every halo); the glow sizes follow `ceil(W / (4 * 2^i))` at any presenter size (row GL6 reads the original's at another size: this is also an input to the resolution rule of MENU-TESTS-PREP section 4).

## 4. Order, owner and questions

Order for the pilot: (1) forced-id colour pairs (section 1.4), cheap and independent; (2) read R1 on a Firelink c0000 NPC, then R2/R3 once the face case is known; (3) `glow-capture.flag` and T8; (4) GL-8 and the image pair. Everything here is by family; none needs an Ember or a summon.
Open: whether the original shows the Sword Master's face (R3), the entity-id offset of `ChrIns` for the NPC aid, whether the colour signs matter for wave 1, the class of the MSB c0000 spawns.

## 5. Not verified here

The class of a c0000 NPC (the contradiction of 2.2); that a single-model phantom already takes the alternate programme natively; the GX immediate context offset for the capture (`[4961AB0]+0x10 -> +0x8` is K-0004, not re-read here); that every pass of the glow reads its inputs through the same context; whether the original's glow runs at a size other than 1280 x 720 (GL6); the frame the Reference capture will fall on.
