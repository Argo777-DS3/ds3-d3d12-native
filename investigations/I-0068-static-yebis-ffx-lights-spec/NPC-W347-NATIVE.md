# I-0068 part AR: the enemy witnesses W3, W4 and W7 (emissive, vertex offset, veil, boss rows): what native refuses or does not produce, by static replay (STATIC)

Request (coordinator, 2026-10-11): the enemy witnesses W3, W4 and W7 stayed open at the NPC workstream
([NPC-FAMILIES-NATIVE](../I-0066-independent-native-startup/NPC-FAMILIES-NATIVE.md), MENUS-MAP §69). For each one: the character, material
and shader families concerned, then what native refuses or does not produce (logged refusals, constant-buffer words with no producer, …), by the
method used for the Grass family ([FOLIAGE-NATIVE](FOLIAGE-NATIVE.md)); general corrections per family with witnesses. Nothing was launched; native
code was read, not changed.

Labels: STATIC (executable data, shader bytecode, FLVER/MTD/SPX game data, native source on `main` `e36aca5c`), INFERRED, OPEN (needs a run).
Evidence (outside Git): `evidence/I-0068/queries/npc-w347/` (`chr-flver.tsv`, `family-reads.tsv`, `default-family-reads.tsv`, `generated-parameters.tsv`,
`spx-admission.tsv`, `recipe-programmes.tsv`, `ps-outputs.txt`, `m30_00-emevd.txt`).

## 0. Answer first

1. **The three witnesses are five material families on four models**: emissive `…EmiIbl_Bloody` / `…EmiIblGlow_Bloody` (class `0x100`, engine row 7),
   `FlowmapVertexOffset` (class 1, row 5), the veil `Veill_NoLight` and `Veill_NoLight_cloth` (class 2, forward row `0x29`), plus the ordinary and cloth
   Bloody families the working witnesses already draw (§1).
2. **Replaying the native code on the data finds no remaining static refusal** for any of them. Eight checks, each a mechanical replay of the native rule on the
   game data, all pass (§2): programme resolution (all recipes, all families), register coverage (every constant-buffer register read has a producer branch),
   word-level coverage against the default family that already draws, the generated (`snp`) parameter reader (`Ready`, ordered branch), effect admission readers
   (113 of 113 pass), texture bindings, pixel output counts against the six G-buffer targets, and mesh geometry (≤ 14 meshes, ≤ 38,802 vertices, 16-bit indices).
   The earlier refusals of these families were all removed by C1–C5 and the index-width fix; this part confirms nothing else stands behind them.
3. **What native will therefore not do is fidelity, not refusal** (§3): the glow chain (`EmiIblGlow` output is read by lighting as emissive, but no glow extraction
   runs), tessellation (the veil is drawn flat), shadows on forward rows, model-word bits 26–28.
4. **What still can refuse is runtime state that static reading cannot give** (§4): the capability masks (the `Capabilities` refusal), the model category and fade variants of a boss, the
   cloth stream/world ownership of a boss model, and whether the boss entity is drawn outside its encounter. They are cheap to read; §6 lists the rows.
5. **W3 as defined cannot work**: at the W3 teleport neither `c2150` nor `c1090` exists in the original either (Native1213: no row words in either mode); their placement
   is event-gated. Better witnesses for the same families exist (§5): `c2150` at Firelink (`m40_00`, (309.3, −35.4, 617), 50 m from W6, TestAnim save), `c1090` and `c2150`
   together at `m30_01` (about (1.6…47, 41…47, 175…196)).
6. **Predicted native rows** per witness (§3.3) are given so the first run can be read against them.

## 1. The witnesses, statically

Models and materials (`chr-flver.tsv` from `Scan-ChrFlver.ps1`; classes from `char-material-classes.tsv`, NPC-FAMILIES-STATIC §2.4):

| Witness | Model (what it is) | Meshes | Bones | Families (meshes) |
|---|---|---|---|---|
| W3 | `c2150` (m30_00 (−40.8, 16.6, 77.6), entity 3000630) | 4 | 42 | `ColDifSpcBumpIbl` class 1 (0, 2); **`EmiIblGlow`** class `0x100` (1, 3) |
| W3 | `c1090` (m30_00 (−56.4, 23.8, 69.7), entity 3000611) | 11 | 193 | `ColDifSpcBumpIbl` class 1 (0, 3, 9) and 4 (4–8, 10); **`FlowmapVertexOffset`** class 1 (1: 21,732 vertices); **`EmiIbl`** class `0x100` (2) |
| W4 | `c5270` (m30_00 (28.2, −9.8, 158.1), entity 3000899, boss, encounter) | 13 | 353 | **`EmiIblGlow`** `0x100` (0, 1); `ColDifSpcBumpIbl` class 4 (2, 3: **ShortBoneIndices**, mesh 3 has 38,802 vertices); **`ColDifSpcBumpIbl_Cloth`** class 4 (9–12); **`Veill_NoLight`** class 2 (4); **`Veill_NoLight_cloth`** class 2 (5–8) |
| W7 | `c5110` (m40_00 (142.9, −68.9, 594.6), entities 4000800 and 4000830, boss) | 14 | 299 | `ColDifSpcBumpIbl` class 1 (0, 2, 5, 6) and 4 (7–10); **`FlowmapVertexOffset`** class 1 (1, 3: 3,820 and 19,683 vertices); `ColDifSpcBumpIbl_Cloth` class 4 (11–13); **`EmiIbl`** `0x100` (4) |

All face sets are 16-bit (six per mesh). Layouts: the ordinary meshes `Position, Normal, Tangent×2, BoneIndices(Byte4B), BoneWeights, VertexColor1, UV0`; the cloth meshes carry
only `Position, Normal, Tangent` in stream 0 (bone indices and UV in stream 1, the two-stream route of Native1063); `c5270` uses `ShortBoneIndices` in the `e_Decal` meshes and the veil
(`BoneIndices0:ShortBoneIndices@20`, `UV1:Short2toFloat2`, `UV2:Byte4B`); `FlowmapVertexOffset` is `Position, Normal, Tangent, BoneIndices, BoneWeights, UV0:UV, UV1:Byte4C`.

Alternative placements of the same families (`msbenemy.tsv`, I-0067): `c2150` at `m40_00` (309.3, −35.4, 617; entity 4000390); `m30_01` (46.9, 41, 190.3) and (67.3, 30.5, 168.7);
`m31_00`, `m37_00`, `m45_00`, `m51_00`. `c1090` only at `m30_00` (five, three of them at (−30…−79, −43, 215…254)) and `m30_01` (four around (0…39, 26…47, 175…196)).

## 2. The eight replays (all STATIC, all pass)

| # | Native rule replayed | Source | Result for the seven families |
|---|---|---|---|
| R1 | programme selection: `SelectModelPrograms` names `<family>_<Gbuf|Fwd|Velo>[_Skin][@[cl]].vpo` and `….ppo`, `_[A]` for pixel variant 1; layout type > 1, deformation ≠ 0 and phantom outside Fwd are refused | `model_programs.cpp` | **resolved** for every family × pass × layout × pixel variant (`recipe-programmes.tsv`; the plain Velo pixel is the shared `GXFlver_Common_Velo`) |
| R2 | register coverage: a read register needs a producer branch (`PrepareReadShaderRegister(..., {})` otherwise) | `game_bootstrap.cpp:4850…5200` | 336 programmes: **every register has a branch** (VS b0 b1 b2 b4 b5 b6 b7 b8 b9 b11, PS b1…b9); only VS b10 `cbInstanceIdxParam` has none, and it is read by the 32 `_Ins` programmes (instanced draws are refused earlier, gate 28) |
| R3 | word coverage against the default family (which draws): new words read | `family-reads.tsv` vs `default-family-reads.tsv` | EmiIbl: tiling row 6 (slot 6, `g_EmissiveTexture`), `cbMatDynParam` row 2 (`FC_EmissiveColor`, produced since C2); EmiIblGlow: + `cbMtdParam` row 9 (`FC_GlowScale`, MTD `g_GlowScale` Float4 (1,1,1,1)); Flowmap: tiling rows 2/4/6, `cbMatDynParam` rows 39–41 (generated), `cbPerFrame` row 11 (`FC_GlobalTime`, produced for both stages since C3); Veil: tiling row 2, rows 39–40. `cbInstanceData` row 7: x and w both inside the produced `0x70..0x7F` group colour |
| R4 | generated (`snp`) parameters: `ReadModelGeneratedParameters` (register ≥ `0x27`, one GXMD record, supported row/descriptor types, ordered branch) | `model_parameters.cpp:78`, `Check-GeneratedParameters.ps1` | **Ready, ordered branch** for every Flowmap, Veil, Veil_cloth, cinder and RecursiveCrystal material of the witness set (22 rows; register `0x27`, keys 125 / 147 / 258 / 134 equal in SPX and GXMD) |
| R5 | effect admission: `ReadModelParameterDeclarations` and `ReadModelTextureDeclarations` must not throw (else `effect_cpu_failure`) | `AdmitEffect`, `model_parameters.cpp:61`, `model_textures.cpp:24`, `Check-SpxAdmission.ps1` | **113 of 113** SAT effects pass (no envelope, collision, alignment or slot ≥ 128 error) |
| R6 | texture bindings: every shader texture register needs a material binding by SPX/builtin name or a scene slot (38, 45, 41/42, 30/31/33/44/59–62) | `SelectModelShaderTextures`, `game_bootstrap.cpp:3666…3714`, `spx-admission.tsv`, `builtin-textures.tsv` (`I0068Table.java`: the engine's builtin tables `473CBB0`/`473CB78`) | **all bound**: Veil SPX `AlbedoMap` t2, `MaskTexture` t1, `NormalMap` t3, the FLVER material carries exactly those three types, the Fwd pixel programme reads t2; Flowmap SPX `AlbedoMap` t1, `DisplacementMap` t2, `FlowMap` t3, `NormalMap` t4, `ReflectanceMap` t5, all five on the FLVER, Gbuf pixel reads t1 t3 t4 t5 and the vertex stage t2 (open since C3); EmiIbl `g_EmissiveTexture` is builtin slot 6 (kind 12) |
| R7 | output counts against the six G-buffer targets (`SceneRenderer::ModelTargetFormats`, 6 formats) and the one forward target | `ps-outputs.txt` | default and Flowmap Gbuf write o0–o4, **EmiIbl and EmiIblGlow o0–o5** (target 5 = emissive, read by the native lighting as t8), the veil forward programmes o0 |
| R8 | geometry limits: ≤ 1024 meshes, 16/32-bit index width per slice, vertex formats of `NativeVertexElementBytes`, pose rows ≤ 4096, palette padded to `max(256, rows)` | `game_bootstrap.cpp:4630,4703,5080`, `model_pose*.h` | ≤ 14 meshes, ≤ 38,802 vertices, 16-bit; `ShortBoneIndices` and `Short2toFloat2` map to supported DXGI widths; `c5270` 353 rows, `c5110` 299 rows, padded palettes address every index |

Forward state of the veil (STATIC, `game_bootstrap.cpp:4811…4838`): `blend_mode` 2 is the admitted alpha blend, `g_GX_bDisableDepthWrite` True keeps depth write off, one HDR target; forward `cbSceneParam`
reads (`FC_MatrixInvViewProj`, `FC_MatrixInvView`, rows 8–11 and 20–22) are camera words the scene producer writes; no `FwdSdw` recipe is selected, so the shadow words (rows 28, 29, 54) are not read.

## 3. What native does not produce (fidelity, not refusal)

| Gap | Family | Where it shows | Owner / spec |
|---|---|---|---|
| glow extraction and chain (`EmissiveToGlowPS` from GB5) | EmiIblGlow only (c2150, c5270; part AS: `EmiIbl` of c1090 and c5110 does not bloom in the original either, its GB5 alpha is 0) | the Glow parts light but do not bloom | [GLOW-CHAIN-NATIVE](GLOW-CHAIN-NATIVE.md) |
| tessellation (`g_TessellationMode 2`) | veil (c5270) | flat silhouette (hull/domain stages never selected, NPC-FAMILIES-STATIC §2.3) | fidelity item |
| the flowmap motion of `FlowmapVertexOffset` | c1090, c5110 | needs `FC_GlobalTime` (C3 produces it) and the three VS textures; static value only if time stalls | C3 witness |
| model-word bits 26–28 (spx-name bits) | every character | not consumed by any row | PRODUCERS-UNION §1.3 |
| shadows on forward rows (`FwdSdw`) | veil | none (no shadow system yet) | SHADOWS-NATIVE |
| effect-system children of the bosses | c5270 (veil sparks), c5110 | absent until the effects phase | EFFECTS-IMPLEMENTATION-CHECKLIST |

### 3.3 Predicted rows (what `native_pass_rows` and the new witness events should show)

Counts are prepared meshes per row of the model, by the class partition of §1 (engine 5 is split into classes 1 and 4 when the model mask has bit 2; the total is the same):

| Witness | Row 5 (classes 1 and 4) | Row 7 (class `0x100`) | Row `0x29` (class 2) | Velocity `0x34` |
|---|---|---|---|---|
| `c2150` | 2 | 2 | 0 | 4 |
| `c1090` | 10 (class 1: 4, class 4: 6) | 1 | 0 | 11 |
| `c5270` | 6 (class 4: `e_Decal` 2, cloth 4) | 2 | 5 (veil 1, veil_cloth 4) | 13 |
| `c5110` | 13 (class 1: 6, class 4: 7) | 1 | 0 | 14 |

(Velocity only while the MotionBlur quality option holds, NPC-FAMILIES / VELOCITY-IMPLEMENTATION-CHECKLIST; row 7 additionally needs the model union bit 15 produced since C1, expected set for all four.)

## 4. What can still refuse, only at run time

| Gate | Condition (STATIC) | Why it may differ for these models | Read |
|---|---|---|---|
| `Capabilities` (gate 10 + reason) | `mesh.material_masks` (material +8/+0x10) AND `group` capability masks (`entity+0x240+0xC0`) both zero | boss groups are not the groups the working witnesses carry | `native_capability_gate` (exists) |
| 4, `Category` (gate 10 + reason) | `ordinary_category` 2, 0x21 or 0x29 | boss fade/phantom states | `native_character_engine7_row`, `native_pass_rows` |
| 3, `ModelClassMask` (gate 10 + reason) | `+0x220` against the row class mask | bosses | `reference_character_row_words` (Reference) |
| cloth ownership | cloth meshes bind the simulated copy (`cloth_stream`, `cloth_world`) or fall back to the ordinary current route | a boss's cloth objects | `native_cloth_*`, `native_selected_palette_unavailable` |
| spawn | the entity exists and is drawn | event-gated placements (W3), boss encounter (W4) or resting pose (W7) | MSB entity ids; `native_pass_rows` |
| streaming | the model's geometry joined (`native_character_geometry_missing`) | 353 and 299 bones, 38,802-vertex meshes | the same event |

## 5. Corrections by family, with witnesses

| Family | State | Correction | Witness (first choice, alternatives) |
|---|---|---|---|
| **Emissive** `EmiIbl[Glow]` | rows, union (C1), `FC_EmissiveColor` (C2), 6th target, lighting t8: all in place | none to the refusal path; glow chain is the next fidelity step (EMISSIVE-GLOW) | `c2150` at Firelink (`m40_00`, (309.3, −35.4, 617)) with the TestAnim save: row 7 count 2, `FC_EmissiveColor` = GX03 (1.0), `FC_GlowScale` (1,1,1,1); `c1090` (GX03 5.0) at `m30_01` |
| **Vertex offset** `FlowmapVertexOffset` | VS textures and `cbPerFrame` (C3), generated words Ready | none; check the time word advances and the three VS textures are bound | `c1090` at `m30_01` (1.6, 46.7, 188); `c5110` at W7; row 5 count 1 resp. 2, `cbPerFrame` row 11 changes per frame |
| **Veil** `Veill_NoLight[_cloth]` | forward row `0x29`, alpha, depth write off, generated words Ready, flat variant | none to the refusal path; tessellation is fidelity | `c5270` at W4 during the fight: row `0x29` count 5, blend alpha, 1 target |
| **Boss rows / large skeleton** | palettes padded to ≥ 256 rows; short bone indices | none; confirm no `native_selected_palette_unavailable` for `c5270` (353 rows) | W4, W7 |
| **Ordinary + cloth Bloody** | draw natively (W1, W2) | none | same models, rows 5 |

## 6. Live rows (none taken)

| ID | Read | Expected |
|---|---|---|
| NW1 | `native_pass_rows` and `native_character_engine7_row` for `c2150`, `c1090` | rows of §3.3; `flags224` bit 15 set |
| NW2 | `native_capability_gate`, `native_selected_constant_unavailable`, `native_selected_texture_unavailable`, `native_recipe_unresolved`, `native_character_geometry_missing` over the witness | none (the static replays predict none) |
| NW3 | `native_cloth_*` and `native_selected_palette_unavailable` for `c5270` cloth meshes | none |
| NW4 | `cbPerFrame` row 11 on two consecutive frames of a `FlowmapVertexOffset` draw | differs |
| NW5 | image: emissive parts lit and flat (no bloom), veil flat with alpha, flowmap parts moving | as §3 |
| NW6 | Reference: the same models' `reference_character_row_words` (model +0x220/+0x224) | bit 15 set |
| NW7 | the boss entity at W7 before the fight | drawn or not (decides the W7 procedure) |

Procedure (static advice): W3 at `m40_00` Firelink for `c2150` (TestAnim save, after W6), `c1090` at `m30_01` (reachable on foot after `c5270`, MENUS-MAP §69) or the three at `m30_00` (−30…−79, −43, 215…254),
event-gated like the first (OPEN: the `m30_00` EMEVD initialises `c2150` and the two reachable `c1090` through events 20005341, 20005415 and 20005417, whose definitions are in the common function file
that the retained data does not hold).

## Exports and evidence

- Tools (this part): `Scan-ChrFlver.ps1`, `Scan-ProgramReads.ps1`, `Check-GeneratedParameters.ps1`, `Check-SpxAdmission.ps1` (read-only replays of native rules on game data), `I0068Table.java` (a table of name pointers in the canonical database).
- Evidence (outside Git): `evidence/I-0068/queries/npc-w347/`.
- Reused: NPC-FAMILIES-STATIC `all-mtd.tsv`, `char-material-classes.tsv`, `fam-diff.txt`; I-0067 `msbenemy.tsv`, `mapfam.tsv`, `Get-EmevdDump.ps1`.
