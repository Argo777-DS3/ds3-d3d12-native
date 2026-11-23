# I-0068 part AG: phantoms, summon signs and fog walls, ready to code

> **Correction (I-0067 MENUS-MAP 75-76, 2026-10-11).** The `_Phantom_<pass>` programmes are **not** in every shader set: only in the alternate set `mat+0x78` (built by `82E50`), used when `DC+0x425 = state+0x87 = 1`, i.e. when the model or part has a PhantomParam id >= 1 whose row exists. A normal host player has none (RoleParam row 0 is -1), so its hair and eyebrows keep the plain `<family>_Fwd[_A]` that native already builds; the "wrong PS even for non-phantom hair" statements below (table row "Phantom shader", step 1, check P4) are withdrawn. A phantom or an NPC with a PhantomParam id needs the whole alternate set. Writer chain: MENUS-MAP 76.

Request (coordinator, 2026-10-10). Thomas wants to test phantoms later: red, yellow (co-op) and
purple. Cover:

- phantom rendering: shader, tint per type, transparency, outline;
- the summon signs on the ground;
- the mist and boss fog walls;
- their native gates;
- checks and witnesses. Argo has an NPC sign 40 m away near Vordt, and an Ember is needed.

**Coordinated with, not modified:**

- MENUS-MAP §59 (the High Wall NPC signs);
- §72 (Ghost `c1480`);
- §41/§42/§46/§47 (phantom programmes, constants, PhantomParam rows), cited by section.

Labels: STATIC, OBSERVED, INFERRED.

**Code read:** `main` `88daeed7`, and the pilot branch `claude/npc-enemies` `9e6278ba` for
`model_programs.cpp`. The MSB and events of `m30_00` come from the local copies.

**Evidence:** `evidence/I-0068/queries/phantoms/` (the sign manager, `853800`, the MSB object
list).

## 0. Answer first

| Item | Original (STATIC) | What native lacks today |
|---|---|---|
| **Phantom shader** | **only a draw that uses the alternate shader set `mat+0x78`** (chosen when the draw context byte `DC+0x425` = the entity state byte `+0x87` is non-zero; that set is built by `140082E50`, the only caller of `1400A9EF0`) binds `<shader>_Phantom_Fwd[_FwdSdw][_[A]/_[AA]]`. A normal draw uses `mat+0x70`: the plain `<shader>_Fwd[_A]` (MENUS-MAP §75, which withdraws §42). **Correction (part AL): part AG had every forward draw of the 64 families use the phantom programme; that was wrong.** Rim = lerp(EdgeColor, FrontColor) by \|N·V\|; no IBL (§47.1) | the forward recipe builds `<family>_Fwd` (`native/content/model_programs.cpp:52-57`): **right for normal draws, including the player's hair**. A phantom draw (`+0x87` ≠ 0) would need the `_Phantom_Fwd` programme; native already reads that byte (`game_bootstrap.cpp:3156`, as `instance_category`, which also selects category 0x21) |
| **Tint per type** | b2 `cbMatDynParam` rows 26–30 (`FC_PhantomEdge/Front/Diff/Spec/LightColor`, +416..+495) from the PhantomParam row of the resolver `88A950`, `rgb²·a`, subtract sign on edge and front (§47.1–2) | no producer: the 22E930 transcription stops at +0x190 (`game_bootstrap.cpp:3524-3584`). The PhantomParam/RoleParam resolver is not reached |
| **Routing and transparency** | a phantom's opaque materials go to forward list **0x21** (group `+0x87`), or to **0x29** when the fade `+0xDC` < 1; `mulCol.w` = fade (§47.3) | rows are 5/6/7/0x1F/0x29 only (`game_bootstrap.cpp:3869`); 0x29 takes class `0x12`, so class 1/4 phantom meshes stay in the G-buffer (`model_pass.cpp:51`) |
| **Outline** | **shader-side**: Edge/Front rim colours (white, red, gold, magenta…) with the subtract flags. No outline FFX found. Auras and summon/vanish bursts are character SFX (SpEffect VFX) | the effect system (EFFECTS-IMPLEMENTATION-CHECKLIST steps 1–3, attached step 2.3) |
| **Summon signs** | `SosSignMan` (RTTI `NS_SPRJ::SosSignMan`, vtable `142847368`). Each sign is **an SFX created by `7CB820` at the sign's world transform** (`853800`), kept at `sign+0x10`, moved by `1A87F50`, removed by `4D2AC0` | `7CB820` is deferred natively (`game_bootstrap.cpp:6758`, `:8628`): empty handle, so no sign is visible |
| **Fog walls** | MSB objects **`o000401`** (boss/area gates) and `o000400` with an **object SFX at dummy 101** (`2006[4] obj, 101, id`, common events `20005780`, `20005820`); `o000499` are hidden helpers (INFERRED) | the same: every SFX route yields an empty handle (FFX-ATTACHED §0) |

## 1. Phantom rendering: ready to code

### 1.1 Programmes (STATIC, MENUS-MAP §75; corrected in part AL)

- **Which draws:** the 64 families with `_Phantom_` programmes (MENUS-MAP §41 list: all character,
  equipment, hair, skin, face and eye families) use the phantom pixel programme **only when the
  draw's state byte `+0x87` is non-zero** (`DC+0x425`, which selects the alternate set `mat+0x78`).
  - That is the same byte that routes a draw to forward list 0x21/0x29 (§1.3); its writer is not
    found (MENUS-MAP §75); INFERRED: the phantom/ghost state of the entity.
  - A normal character, the player included, draws the plain `<family>_Fwd[_[A]]`.
  - The phantom look comes from the programme, the constants (§1.2) and the routing (§1.3).
- **Native step 1:** in `SelectModelPrograms` (`native/content/model_programs.cpp:52-57`), for
  `shader_pass == 0` (Fwd) **and a draw whose group byte `+0x87` is non-zero**, take the pixel name
  `<family>_Phantom_Fwd[_[A]].ppo` when that programme exists, else `<family>_Fwd`. The VS stays
  `<family>_Fwd[_Skin]`. A draw with the byte clear is unchanged.
  - Do the same for `FwdSdw` once the shadowed forward exists.
- **A phantom draw of a family without `_Phantom_`:** a null pixel programme makes `140850` return
  without drawing (MENUS-MAP §75; the "depth-only" reading of §47.3 point 4 is unsupported). Native
  should skip the colour draw, not substitute `_Fwd`.

### 1.2 Constants: b2 rows 26–30 (STATIC, MENUS-MAP §47.1–2)

- **Source:** the resolver `88A950` → `9CAF30` (`ChrIns` slot 36, every frame). The first
  defined source wins:
  1. `chr+0x1F38`;
  2. `chr+0x1A58`;
  3. NpcParam `+0x1CC`;
  4. NpcParam `+0x238`;
  5. RoleParam (by covenant rank).
- **Path:** the result goes into the draw's override list as the seven `ExParam_Phantom*`
  (names at `143D628D0..143D62A28`), then `22D620` lerps the material block defaults toward
  them (weight `[5]`), then `22E930` writes b2.
- **Native step 2:** call (or transcribe) the resolver once per phantom character. Write rows
  26–30:
  - `+416` Edge = `rgb²·a·s`, `.w = a`;
  - `+432` Front, the same;
  - `+448` Diff = `rgb²·a`;
  - `+464` Spec = `rgb²·a`;
  - `+480` Light = `rgb²·a`.
  - `s = −1` when IsEdgeSubtract/IsFrontSubtract.
  - Without a row, keep the material defaults (MENUS-MAP §42 INFERRED).
  - Native already consumes the character's `[item+0x6E8]` ExParam table (CHARACTER-EFFECTS-MAP
    corrected); the phantom names are part of it.

**Tints** (PhantomParam, MENUS-MAP §47.2):

| Type | Row | Edge | Diff | Light |
|---|---|---|---|---|
| white (summons, NPC 27920/27320/25120 of the High Wall) | 80 | white a .5 | white a .5 | — |
| **red** (invaders) | 81 (roles 2, 10002, …; NPC rows 21440, 21640, 22140, 22240, 22340, 27440/41, 27890/91, 28040, 29240/41, 29340/41, 29440, 29540) | red a .6 | white a .2 | — |
| **yellow/gold** (co-op, Warrior of Sunlight, role 20001) | 85 | orange-gold a .9 | a .3 | — |
| **purple** | 91 (roles 40001..70013) | (100,160,255) | magenta-purple (255,0,200) | (255,0,140) a 3 |
| magenta | 88 (roles 30001…; NPC 21200, 27720, 27740) | (220,0,180) | white .1 | (255,120,0) |
| others | 60, 74, 76, 82–84, 86, 92 | see MENUS-MAP §47.2 | | |

### 1.3 Routing and transparency (STATIC, MENUS-MAP §47.3)

**Original** (`57A20`, opaque material, descriptor kind 2):

- group `+0x87` set (phantom) → list **0x21**;
- with fade `+0xDC` < 1 → **0x29**;
- the global `144962314` forces 0x29;
- `+0x84/+0x85` refine.

INFERRED: 0x21 = forward opaque, 0x29 = forward translucent.

**Native step 3:**

- read the draw group's `+0x84/+0x85/+0x87/+0xDC` (the group bytes are already read for the
  native selection);
- for a phantom model, add rows **0x21** (class mask covering the opaque classes 1/4/`0x100`,
  depth tested and written, opaque blend) and **0x29** (fade < 1: alpha blend, depth tested,
  not written);
- route the model's G-buffer meshes there instead of rows 5/6/7.

**Places to change:**

- the row list at `game_bootstrap.cpp:3869`;
- the class masks of `EvaluateOrdinaryModelPass` (`model_pass.cpp:51`);
- the forward state at `game_bootstrap.cpp:4231-4246`.

**Fade:** `cbInstanceData.mulCol.w` (instance row 7) = the fade; the PS multiplies colour and
alpha by `mulCol` (§47.1).

**Open:** the writer of the group bytes `+0x87` and `+0xDC` (MENUS-MAP §47.3, not read). Witness
it on a summoned NPC (§4, P2).

### 1.4 Outline and effects

- **The outline is the rim term of the phantom PS** (Edge/Front colours). Nothing else is
  needed for it.
- **Summon and vanish bursts and phantom auras** are SpEffect VFX, which are character-attached
  SFX (FFX-ATTACHED §1.4, CHARACTER-EFFECTS-MAP). They come with the effects workstream
  (EFFECTS-IMPLEMENTATION-CHECKLIST step 2.3). INFERRED: no phantom-specific GX filter (none
  found by name; the ExParam path is the only phantom-specific renderer code).

## 2. Summon signs (STATIC)

- **Owner:** `SosSignMan` (RTTI `.?AVSosSignMan@NS_SPRJ@@`, vtable `142847368`).
  - Jobs `CSSosSign{Create,Update,DownloadSignList,Summon,Remove,Reject}Job` (online).
  - Event commands: `SetSosSignPos`, `SetSosSignWarp`, `ClearSosSign`; NPC signs through
    `2003[25]` (MENUS-MAP §59).
- **Visual:** `853800(man, signRecord)`:
  1. **Keying:** the sign records are kept in a tree keyed by the record id (`man+0x20`). Each
     sign object (0x480) holds an `SfxHandle` at `+0x10`.
  2. **When the record is active** (`+0x23` set) and the object has no effect yet (`+0x470`
     bit 0 clear), it:
     - builds the transform from the record position `+0x08..+0x10` and the yaw `+0x14`
       (`4D9B10`);
     - picks the SFX id: `14464C0F8 + type·0x38` (sign type byte `+0x22`) gives a category,
       then `variant·10000 + category` (`9F9790`, variant = short `+0x1E` < 16,
       category < 14) is a **RoleParam** row id. `E3C4A0` looks up param index **0x45**, which is
       RoleParam (MENUS-MAP §29/§47). The row's `+0x30` is the **sign SFX id** (part AJ, STATIC +
       regulation):
       - roles 1, 10001, 40001, 50001, 60001, 70001, 80001, 90001 (white and the like): **30001**;
       - roles 2, 10002, 40002/40013, 50002, 60002, 70002, 80002 (red): **30011**;
       - role 20001 (Sunlight, gold): **30021**; role 20002: **30031**;
       - roles 30001 / 30002: **30041** / **30051**;
       - +0x34 holds the paired id (30000, 30010, …; INFERRED: the other sign state);
       - **the category table** `14464C0F8` (stride 0x38) gives type 0 → 1 and type 1 → 2, so an
         NPC white sign (variant 0) reads role 1 → **SFX 30001**. Rows: `evidence/I-0067/queries/phantom/roleparam.tsv`;
     - calls **`7CB820(handle, id, transform, 8)`** and stores the handle (`4D2930`).
  3. **An existing effect** is moved with `1A87F50(handle+0x10, transform)`.
  4. **An inactive record** destroys it (`4D2AC0`) and clears bit 0.
- **The NPC sign of the High Wall** (MENUS-MAP §59, `2003[25] 0, npc, region, summoned, dismissed`)
  goes through the same manager (INFERRED: the record type byte selects the white sign).
- **Native gate:** the `7CB820` deferral (`game_bootstrap.cpp:6758`, `:8628`). Once the effect
  system exists and the hook is replaced (EFFECTS-IMPLEMENTATION-CHECKLIST 1.1–2.1), signs
  appear with no sign-specific code: an E01-family effect at a world transform.

## 3. Fog walls (STATIC + game data, `m30_00`)

| Object (MSB) | Model | Event entity | Position | Use |
|---|---|---|---|---|
| `o000401_2000` | `o000401` | 3001800 | (27.7, −25.5, 44.9) | **Vordt's boss fog**: `20005800/20005801` (boss flag 13000800, region 3002800), `20005820` (SFX), `20005840/41` |
| `o000401_5000` | `o000401` | 3001830 | (−75.6, −49.4, 367.2) | boss fog of 13000830 (`20005800/01/20/37/10`) |
| `o000401_4000` | `o000401` | 3001890 | (27.6, −8.6, 130.1) | area fog, `2006[4] 3001890, 101, 3` in the map's own event |
| `o000400_0000` | `o000400` | 3001780 | (3.8, 2.2, 170.4) | `20005780(3001780, 2)` |
| `o000499_0000/1/2` | `o000499` | 3001650, 3001705, 3001260 | | helpers (3001705 gets `2006[4] …, 90, 830095`) |

**Mechanism:**

- `2005[3]` toggles the object (and its collision);
- **`2006[4] object, 101, sfxId`** creates the wall's effect at the object's **dummy 101**
  (`2006[5]` deletes it). The id comes from the caller: 2 for `o000400` (`20005780`), 3 for the
  area fog (INFERRED for the boss fogs: the `20005820` parameter, 3);
- the visible wall is that SFX: FXR billboards and distortion. INFERRED: E01 with E04 twins;
  the `o000401` model is the collision gate.
- **Native gate:**
  - the object effect route (FFX-ATTACHED §1.2–1.3: `65AB10` slot or `52EC30`/`7CC4B0` block
    route) yields an empty handle until EFFECTS-IMPLEMENTATION-CHECKLIST steps 1–3;
  - the object model draws through the ordinary path (check it is not an invisible-material
    model).

## 4. Checks and witnesses

**Argo's save, the High Wall `m30_00`.** Conditions for the NPC sign (MENUS-MAP §59):

- an **Ember** (SpEffect 490);
- Vordt alive (13000800 off);
- requirement 13005392 (Basin of Vows; INFERRED polarity);
- the player within **10 m** of sign 1 at (6.7, −6.3, 87.1), **about 40 m from Argo's spot**.

The pilot walks there; read-only observation.

| # | Check | Pass |
|---|---|---|
| S1 | counter on `853800` with `+0x23` set, then `7CB820` from `853800` | one call per visible sign, **SFX id 30001** (white, RoleParam 1), transform at (6.7, −6.3, 87.1) ± 1 m |
| S2 | the sign handle `sign+0x10` (after step 2.1 of the checklist) | non-null; E01 draws at the sign's screen position |
| P1 | summon NPC `c0000_0007` (entity 3000190, NpcParam 27920 → PhantomParam 80) | its body draws in forward list **0x21** with `<shader>_Phantom_Fwd` PS |
| P2 | group bytes `+0x87`, `+0xDC` of its draw group (witness for §1.3 Open) | `+0x87` set; `+0xDC` < 1 during the fade-in, then 1 |
| P3 | b2 rows 26–30 of a P1 draw | white: Edge (0.5, 0.5, 0.5, 0.5) etc. (`rgb²·a`) |
| P4 | hair forward of the **player** (non-phantom) | PS = the plain `…_Fwd_[A]` (`DC+0x425` = 0), **not** `_Phantom_` (MENUS-MAP §75); native `<family>_Fwd` is already right. Corrected in part AL |
| F1 | Vordt's fog `o000401_2000` (3001800) while Vordt is alive | `2006[4]` → object SFX at dummy 101; the wall's draws at (27.7, −25.5, 44.9) |
| F2 | area fog `o000401_4000` (3001890) and `o000400_0000` (3001780) | the same, with ids 3 and 2 |

**Red, purple, yellow:**

- **Red NPC invaders:** the NpcParam rows with PhantomParam 81 (§1.2) are not on `m30_00` (the
  local MSB has none). Their maps are not read here: the pilot can find the first one on the
  route.
- **Yellow (85) and purple (91):** RoleParam, so player phantoms. They are online-only,
  INFERRED: no offline witness.
- **Static substitute:** check P3 with a forced row. Feed rows 81/85/91 into a white NPC's draw
  in a native-only test, and compare the PS output against the DXBC formula of MENUS-MAP §47.1.
- **Ghost `c1480` (`m37`, MENUS-MAP §72):** a forward ghost family (blend 2, emissive with
  `GlowScale.w` 0.7). It is not a phantom and follows the ordinary forward and glow steps.

## 5. Order

1. Programmes (§1.1), for phantom draws only (`+0x87` ≠ 0). Nothing changes for the player's hair (P4 corrected in part AL).
2. Constants and the resolver (§1.2).
3. Routing 0x21/0x29 and fade (§1.3), with P1–P3 on the High Wall summon.
4. Signs and fog walls: no specific code. They arrive with EFFECTS-IMPLEMENTATION-CHECKLIST
   steps 1–3 and 2.1 (S1–S2, F1–F2).
