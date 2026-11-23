# I-0068 part AL: the open NPC families W2–W7 (translucent, SSS, emissive, vertex offset, veil, crystal, eyes, cinder): what static reading settles, and the live reads that remain

Request (coordinator, 2026-10-10 23:55 UTC): the NPC workstream left in `STATE.md` a list of open
families and of unverified witnesses W2–W7 of MENUS-MAP 69 (translucent, SSS, emissive, crystal,
eyes). For each family: settle in static what can be settled (shaders, constants, gates, pass order)
in an I-0068 note, and mark the rest live-only with where to read, so the pilot can take them in one
block.

Labels: **STATIC** (executable, shader bytecode, MTD/FLVER game data, native source on `main`
`8013aad7`), **INFERRED** (follows from STATIC facts, not read directly), **LIVE-ONLY** (needs a run
of the original or of the native build). No new OBSERVED in this part: nothing was launched.

Files read but not edited: MENUS-MAP 66–75 (the menus thread owns them; corrections are listed in
§2.4 and go to that thread), TONEMAP-SKIN 18.

**Evidence** (`evidence/I-0068/queries/npc-families/`, external store, not in Git):

- `all-mtd.tsv`: every parameter of the 513 MTDs of `AllMaterialBnd` (SoulsFormats, `Dump-AllMtd.ps1`);
- `mtd-classes.tsv`, `char-material-classes.tsv`: the native classification replayed over the 72 MTDs
  that characters use (`Classify-Mtds.ps1`);
- `all-chr-materials.tsv`, `all-chr-gx.tsv`: for the 125 `chr/cNNNN.chrbnd` of `Data3` (the 40 DLC
  characters are not in that archive): material, MTD, GX item ids and values, FLVER texture types
  (`Dump-FlverGx.ps1`);
- `fam-reflect.tsv` (with the SHA-256 of each programme blob, usable to find a draw in a capture),
  `used-vars.tsv`, `fam-used.txt`, `fam-diff.txt`, `sig-all.tsv`, `cb-reads.tsv`, `asm/` (the
  programme interfaces and the disassembly of the key pixel programmes).

## 0. Answer first

| Family (witness) | Native today (STATIC) | First refusal or gap | Settled statically | Live-only (block in §7) |
|---|---|---|---|---|
| **Translucent** `…ColDifSpcBumpTrIbl_Bloody[_Cloth]` (W2: c1280 cloak) | **deferred, engine 5, class 1/4** (not forward) | `FC_TranslucencyScale` unwritten (zero) | class, rows, programmes, interface (identical to the default family), the one extra instruction, the GX05 value (1.0) | which programme the original binds (confirms the MTD reading) |
| **SSS** `…SSSIbl_Bloody[_Cloth]` (W2 c2020, W6 c3190/c3200/c1400) | engine 5, stencil bit 4 from `+0x24` `0x800` (Native1185), profile from GX15 | `FC_TranslucencyScale` as above | programmes, textures, `FC_SSS_ID`, stencil, the 77 profiles, GX15 values | `FC_SSS_ID` slot and `cbSSS` record of one enemy |
| **Emissive** `…EmiIbl[Glow]`, `SSSEmiIblGlow`, cinder, Fur (W3 c2150, c1090; W7 c5110; W6 c1450) | **class `0x100` → engine-7 row** | row 7 needs `+0x224` bit 15: **the union is not produced for `SprjModelDrawEntity`** (37 characters, 202 meshes never drawn); then `FC_EmissiveColor` (GX03) unwritten | gates, order, constants, GX03 values (1.0, 5.0, 0.5) | model union words in the original, `cb2` emissive row |
| **Vertex offset** `FlowmapVertexOffset` (W3 c1090, W7 c5110) | class 1, engine 5 | **two certain refusals**: vertex-stage texture (`model_textures.cpp:86`), vertex-stage `cbPerFrame` (`game_bootstrap.cpp:4518`) | both gates, the VS interface | the VS values (snp words, time) |
| **Veil** `Veill_NoLight[_cloth]` (W4 c5270) | class 2 → forward row `0x29`, flat variant (tessellation unread) | none new beyond the forward-state items of FORWARD-DEPTH-NATIVE; blend 3 (`c6232`) not admitted | class, rows, interface, tessellation fallback, no-light PS | variant bound by the original (`_FT`/`_PN`?), blend/depth state |
| **Crystal** `RecursiveCrystal` (W5 c3100) | class 2 → forward row `0x29` | **`t45` unbound**: the shader samples the HDR frame copy there | t45 meaning, the two targets, the order | what the original binds at t45 and when |
| **Eyes** `GXFlver_EyeIbl_Bloody` (W6 c1400) | class 1, engine 5 | none | interface, `FC_ParallaxParam` = 0 for the witness (GX09 `{0,0.5,0,0.5}` → zeros), `FC_SkinMatchColor` produced from the group | `FC_SkinMatchColor` value of c1400 |
| **Cinder** `C[ARSN][cinder]` (W6 c1450) | class `0x100` → row 7 | the union (above) | interface (6 outputs, `cbSceneParam`, `cbPerFrame`) | snp words |

The five static changes (§7, C1–C5), in order of reach:

1. produce the model union (`+0x220`/`+0x224`) for every model, not only ASM and menu parts (§2.1);
2. parse the GX items 03 and 05 (09 is zero for the witness) in `BuildModelMaterial` (§2.2);
3. give the vertex stage its textures and `cbPerFrame` (§4.2);
4. bind a scene-colour copy as slot 45 (§5.2);
5. select `_Phantom_Fwd` only for draws whose group byte `+0x87` is set (§8).

Other families of the NPC workstream (§8): Ghost and Fur need nothing beyond C1 and the forward
binding; tessellation is never selected (flat variants are valid); enemy SSS profiles are closed;
the "`Phantom_Fwd` parity point" is **withdrawn** (MENUS-MAP 75): the native `<family>_Fwd` is right
for the player and every normal draw.

Corrections (to MENUS-MAP, for the menus thread, not edited here): the translucent family is **not**
blend 2 / depth write off / forward (§2.4); GX parameter `+0xF7` is the Wet block, not an SSS/glow
gate (§2.2). Corrections to earlier I-0068 parts, applied: EMISSIVE-GLOW step 2 and
NATIVE-GATES-AUDIT §2.5 were incomplete (§2.2); PHANTOMS-SIGNS-FOGWALLS-NATIVE §0/§1.1/P4,
COVERAGE-MATRIX row 10 and LIVE-CHECKS P4·PH had every forward draw of 64 families use the
`_Phantom_` programme, which MENUS-MAP 75 withdrew.

## 1. Method

1. **Native classification replayed.** `PrepareModelMaterialPolicy` and `ClassifyModelMaterial`
   (`native/content/model_parameters.cpp:399-450`) are replayed in `Classify-Mtds.ps1` over the MTD
   parameters of every material a character uses. The builtin kind table is the one the pilot
   extracted from the executable (`evidence/I-0066/artifacts/direct-material-policy-759/policy-data-759.json`).
   Validation: the armour family `SAT_Equip` (`p_bd[arsn].mtd`) gets class `0x100`, as the armour row
   the pilot drew in Native1100-1112. Not replayed: the shader-source declarations that override a
   builtin (INFERRED: none of the classification parameters is declared by these families).
2. **Programme interfaces.** The 126 programmes the native pipeline would select for the families
   (Gbuf, Gbuf `[A]`, Fwd, Fwd `[A]`, skinned VS) come from the I-0063 cache; the interface of each
   (resources, constant variables flagged used by the compiler, outputs) is in `fam-reflect.tsv`,
   `used-vars.tsv`, `sig-all.tsv`, and diffed against the default family in `fam-diff.txt`.
3. **GX lists.** The FLVER material GX item lists of the 125 `Data3` characters were parsed
   (`{id, unk, size, data}` chain, as `141070` and the pilot's `NativeSssProfileOf` read it).
4. **Native producers.** For each word a programme reads, the producer in `native/frame/game_bootstrap.cpp`
   was located by line; absence of a producer is established by a search of `native/` for the
   variable names and for the material offsets.

## 2. Findings that cut across families

### 2.1 The row gate: the model union is produced only for ASM and menu parts (STATIC)

- `57A20` adds the **engine-7 row** (class mask `0x100`) only for a model with `+0x224` bit 15, and
  the **row `0x1F`** (class mask `0x80`) only with bit 12 (`game_bootstrap.cpp:3966-3977`; the model
  mask is not applied to row 7, `:4004`).
- The native union (`game_bootstrap.cpp:3720-3724`) runs inside `if(c.neutral_instance_decal)`.
  That flag is true for ASM parts (`:3880`) and menu parts (`:3799`). The `SprjModelDrawEntity`
  candidate (`:3912-3930`: enemies, NPCs, corpses, skinned map objects) never sets it.
- So for those models `material_flags` is empty and `Read(model+0x224)` returns the construction
  word (`+0x220` = `0xFFFF`, `+0x224` = 0; INFERRED from the Native1103 comment at `:3717`, which
  gives these construction words for parts).
  Rows 7 and `0x1F` are never added.
- Row 5 takes class mask `5` (`model_pass.cpp:51`), so a class-`0x100` mesh is filtered at
  `MaterialClass` (gate code 21) and is not drawn at all.
- **Reach (STATIC, replay in §1):** 37 of the 157 characters of `chrmat.tsv`, 202 meshes, have class
  `0x100`: `c1090 c1200 c1201 c1211 c1283 c1310 c1450 c2132 c2150 c2230 c2270 c3040 c3050 c3060 c5020
  c5110 c5150 c5160 c5180 c5250 c5260 c5270 c5280 c6020 c6030 c6040 c6050 c6060 c6090 c6100 c6130
  c6200 c6201 c6210 c6211 c6250 c6260`. No character mesh has class `0x80`, so bit 12 is not needed
  for them.
- **Fix** (already specified in PRODUCERS-UNION §1.4 for map pieces): run the union loop for every
  admitted model. It reads only `mesh.material_class` and `policy->flags` of already-bound meshes.

### 2.2 The GX item parser is not run: three constant words depend on it (STATIC)

- `BuildModelMaterial` (`game_bootstrap.cpp:6692-6742`) builds the material with `82CC0`, `1B0AB0`,
  the ordinal and two data-derived bits of `+0x24` (cloth `0x2000`, SSS `0x800`). It does **not**
  run `140F90` (class, `+0x1C`, `+0x21`, the rest of `+0x24`, `+0x80`) nor the GX item parser
  `141070` (MENUS-MAP 67). The natives replace single items piecemeal: the SSS profile from GX15
  (`NativeSssProfileOf`, `:2534`), the texture slot records GX20–GX34 (`texture_tiling.cpp:43-56`), the cloth bit.
- The cbMatDynParam producer (`game_bootstrap.cpp:3556-3636`, a transcription of `22E930`) reads its
  inputs from the material object at `mat+0xD0` (flag byte, then value). Where the flag is clear it
  writes the buffer's creation zero. The flags are written only by `141070`, hence by nothing.
- Mapping, checked against MENUS-MAP 67 (item → field) and `:3571` (field → word):

| GX item | Material fields (`source = mat+0xD0`) | cbMatDynParam word | Read by (Gbuf/Fwd/VS programmes) | Native |
|---|---|---|---|---|
| **GX03** | flag `mat+0xDE`, value `mat+0x104` | `+0x20` `FC_EmissiveColor` = `{v,v,v,1}` | `EmiIbl`, `EmiIblGlow`, `SSSEmiIblGlow` | **absent** (`:3586`, flag never set) |
| **GX05** | flag `mat+0xE0`, value `mat+0x108` | `+0x10` `FC_TranslucencyScale` | `TrIbl`, `TrIbl_Cloth`, `SSSIbl`, `SSSIbl_Cloth`, `SSSEmiIblGlow` | **absent** (`:3556-3564`) |
| **GX09** | flag `mat+0xE3`, values `mat+0x200..0x20C` | `+0x60` `FC_ParallaxParam` = `{a, −ab, c, −cd}` | `EyeIbl_Bloody` | **absent** (`:3589`) |
| GX15 | helper `22BF70` → SSS link | `+0x190` `FC_SSS_ID` | SSS families | **present** (`:3636`) |
| GX04 / GX90 | `mat+0x250`, `+0x254` (tessellation mode, displacement) | none (mesh programme variant) | all tessellated | **absent**, see §2.3 |
| GX01, 02, 12, 13, 14 | `+0xDC`, `+0xDD`, `+0xE8`, `+0xE9`, `+0xF0` | `+0x30`, `+0x40`, `+0x70`, `+0x30`, `+0x180` | map families (MultiBlend, POM, dynamic diffuse), none of the enemy families | absent, not needed here |

- **The other cbMatDynParam words the enemy programmes read are all produced:** `FC_isDitherAlpha`
  (`:3512`), `VC_isEnableCloth` (`:3600`), `FC_SSS_ID`, the Wet rows `560..623`, and the SAT
  generated (`snp`) words (`:3472`). The query over `used-vars.tsv` (every non-`snp`, non-Wet word
  of cbMatDynParam read by any programme of the families) returns six names: the three produced ones above (`FC_isDitherAlpha`,
  `VC_isEnableCloth`, `FC_SSS_ID`) and the three GX-fed ones of the table.
- **The values are in the data (STATIC, `all-chr-materials.tsv`):**
  - GX05 is on all 83 SSS, SSS-emissive and translucent materials of the 125 characters; value 1 on
    66, 1.5 on 1, 2 on 3, 4 on 2, 5 on 11 (every witness material: **1.0**);
  - GX03 is on all 30 emissive and ghost materials: 0.07 (`c5180`, 6), 0.15 (`c1310`), 0.35
    (`c2230`), **0.5** (`c5270` `EmiIblGlow`, 2), **1** (`c1200 c1480 c1490 c2132 c2150`, 11), 1.5
    (`c5160`, 2), 2 (`c5280`, 5), **5** (`c1090` and `c5110` `EmiIbl`, 2);
  - GX09: one material (`c1400` eye), words `{0, 0.5, 0, 0.5}`, so `FC_ParallaxParam` is
    `{0, −0, 0, −0}`: **the native zero is already right for the witness**.
  - `C[ARSN][cinder]` and `Veill*`, `RecursiveCrystal`, `Fur`, `Flowmap` carry none of GX03/05/09.
- **Contract for the pilot** (minimal): in `BuildModelMaterial`, walk the FLVER GX chain of the
  material (`header + record[+0x14]`, items `{id, unk, size, data}` to `0x7FFFFFFF`) and write
  GX03 (`mat+0xDE`=1, `mat+0x104`=d[3]), GX05 (`mat+0xE0`=1, `mat+0x108`=d[3]) and GX09
  (`mat+0xE3`=1, `mat+0x200..0x20C`=d[3..6]). The producers at `:3556-3589` then work as written.
- **Same gap on map pieces.** `M[ARSN]_em` bowls (`FC_EmissiveColor`) and `M[ARSN]_tr` carry the same
  GX items. NATIVE-GATES-AUDIT §2.5 and EMISSIVE-GLOW step 2 ("admit the Emi families") therefore
  need this item as well as the union; both notes carry a pointer to this section.
- **Correction on `+0xF7`:** the block `22E930` gates with `mat+0xF7` writes `0x230..0x26F` of the
  dynamic buffer, i.e. the **Wet** rows `560..623`, not an SSS or glow value (`0022E930.c:735-783`).
  Natively the Wet rows come from `SelectModelWetActivation`/`ProduceModelWetParameters`
  (`:3493-3508`). INFERRED: the missing `0x4000000/0x8000000` name bits of `gxMtd+0x88` need not be
  produced for these families (the Wet rows are what the default programmes read, `used-vars.tsv`).

### 2.3 Tessellation is never selected: every mesh takes its flat variant (STATIC)

- The mesh deformation is read from `mat+0x250`/`+0x254` (`game_bootstrap.cpp:3262-3263`), written
  only by GX04/GX90, which nothing parses. It is always 0, so `SelectModelPrograms` never receives
  `deformation` ≠ 0 and takes the plain `…_Fwd`/`…_Gbuf` programme.
- Data (`all-chr-materials.tsv`): the materials whose GX04 carries a mode are `c2230` (`Veill`,
  `Veill_cloth`: mode 2), `c5270` (the five `Veill_NoLight*` materials and `Line_cloth`: mode 2),
  `c3140`/`c3141` (`FlowDisplacement`: mode 1, displacement 1).
- Consequences: the veil is drawn without hull/domain stages (silhouette not rounded), the
  displacement of `c3140`/`c3141` is absent, `cbTessScene`/`cbPNTriangles` (open in MENUS-MAP 73) are
  not needed natively. This turns MENUS-MAP 72's "HS and DS" native gate into a fidelity item.
- Note for the live block: the SAT bundles `Veill` and `Veill_NoLight` list only `_FT` and `_FT_D`
  programmes in the cache (`Veill_NoLight_Fwd_FT.*`), no `_PN`, while MENUS-MAP 72 gives `c5270`
  mode 2 = variant `_PN`. What the original binds there is LIVE-ONLY (§7, W4).

### 2.4 The native class of every character MTD, and a correction (STATIC)

`mtd-classes.tsv`; class → native row: `1`/`4` engine 5 (split rows 1 then 4), `8` engine 6, `0x100`
engine 7, `0x80` row `0x1F`, `2` row `0x29`.

| SPX family | Classes | Row | Remarks |
|---|---|---|---|
| `GXFlver_ColDifSpcBumpIbl_Bloody[_Cloth]` | 1; 4 for `_e_` (AlphaRef 128); 2 for `_al`, `_add` | 5; `0x29` | `_e_` uses the `[A]` pixel variant |
| `…TrIbl_Bloody[_Cloth]` | 1; 4 for `_e_` | **5** | MTD blend **0**, depth write on |
| `…SSSIbl_Bloody[_Cloth]` | 1; 4 | 5 | type 3 → `+0x24` bit `0x800` |
| `…SSSEmiIblGlow_Bloody` | `0x100` | 7 | |
| `…EmiIbl_Bloody`, `…EmiIblGlow_Bloody[_Cloth]` | `0x100` (`_em_al_glow`: 2) | 7 | |
| `…EyeIbl_Bloody` | 1 | 5 | |
| `FlowmapVertexOffset` | 1 | 5 | |
| `…FlowDisplacement` | 1 | 5 | flat (§2.3) |
| `Veill`, `Veill_cloth`, `Veill_NoLight[_cloth]` | 2 | `0x29` | blend 2 (`c6232`: 3) |
| `RecursiveCrystal` | 2 (Recursive bit 21) | `0x29` | |
| `C[ARSN][cinder]`, `_em`, `_cloth`, `C_ARSN_em` | `0x100` | 7 | |
| `Fur_FDP` | `0x100` | 7 | Emissive precedes the alpha-ref test; `Gbuf_[A]` |
| `…EmiMulIblGhost[_Cloth]` | 2 | `0x29` | |

Totals (`char-material-classes.tsv`, 157 characters, 1,774 meshes): class 1 = 488 meshes, class 4 =
1,030, class `0x100` = 202, class 2 = 50 (21 characters: `c1391 c1480 c1490 c2030 c2040 c2170 c2230
c3100 c5180 c5270 c6020 c6090 c6100 c6120 c6121 c6232 c6240 c6250 c6310 c6330 c6331`), 4 without MTD.

**Correction (translucent).** MENUS-MAP 69 (table, "translucent + cloth"), 68 (family table) and
72.4 give the translucent family as blend 2, depth write off, `g_AlphaRef` 1, forward pass `0x29`. The
MTD data says otherwise: for every character MTD of the family (`C[ARSN]_tr`, `_tr_Decal`,
`_e_tr`, `_e_tr_Decal`, `_tr_Cloth_Decal`, `_e_tr_Cloth_Decal`, `M[ARSN]_e_Cloth_Decal`,
`c3141_C[ARSN]_e_tr_Decal`) `g_BlendMode` is **0**, `g_GX_bDisableDepthWrite` is **False**, and
`g_AlphaRef` is 0 or 128. Only the `_al_tr` MTDs (blend 2, `P[ARSN]_al_tr` hair cards,
`M[ARSN]_al_tr` map foliage) match the earlier description, and no character FLVER material uses
them except through the hair path. The I-0063 scan already had `blend 0` for `C[ARSN]_tr_Decal`
(`evidence/I-0063/artifacts/scan/mtd.tsv`). "Tr" is thin-surface translucency in the G-buffer, not
alpha blending (§3.1).

## 3. W2: translucent + cloth, and the SSS of enemies (c1280, c1105, c2021, c2020, c1290)

### 3.1 Translucent `GXFlver_ColDifSpcBumpTrIbl_Bloody[_Cloth]` (STATIC)

- **Users** (125 `Data3` characters): `TrIbl_Bloody` 20 materials in `c1220 c2170 c3140 c3141 c5221 c5222
  c5223 c5225 c5226 c5227`; `TrIbl_Bloody_Cloth` 3 materials in `c1280 c1281 c1282` (W2: the
  `c1280_cloak_cloth` material, MTD `M[ARSN]_e_Cloth_Decal`, GX05 = 1.0).
- **Class and row:** class 1 (`_tr_Decal`) or 4 (`_e_tr`, AlphaRef 128), so **engine 5**, split rows
  1 then 4, shader pass 6 (Gbuf), pixel variant `[A]` for class 4. Not forward, not blended, depth
  write on (§2.4). The programme set in the cache is complete: `…_Gbuf.ppo`, `…_Gbuf_[A].ppo`,
  `…_GBuf_Skin.vpo`, and for the cloth family `…_GBuf_Skin@[cl].vpo` (native tries `@[cl]` first and
  falls back, `model_programs.cpp:52-57`).
- **Interface:** the same resources and the same used constant variables as `…ColDifSpcBumpIbl_Bloody`
  (`fam-diff.txt`: no `+res`, no `+cb`). Textures t0, t1, t3, t41, t42 and samplers s0–s2; cbuffers
  b1 tiling, b2, b3 (`FC_DifMapMulCol`, `FC_SpcParam`, `FC_AlphaRef`, `FC_BloodAmount`), b4.
  The extra FLVER textures (`g_ShininessTexture`, `g_BloodMaskTexture`, `g_DisplacementTexture`) are
  not declared by the programme and are not bound, as for the default family.
- **The whole difference is one instruction** (`diff` of the two disassemblies, `asm/`): the default
  pixel programme writes `o1.y = 0`; the translucent one writes
  `o1.y = COLOR1.y × cb2[1].x × 0.1` (vertex colour 1, channel y; `cb2[1].x` = `FC_TranslucencyScale`).
  The input signature gains the `y` component of `COLOR1` (`v3.yzw` instead of `v3.zw`); the skinned
  vertex programme already carries `COLOR1` in the default family.
- **Consumer:** the deferred directional-light programmes sample the G-buffer 1 texture (`t3`) and
  test `GB1.y > 0` (`GXLightAcc_DDirLight1.ppo`, `sample … t3` then `lt 0, r4.y`), multiply by 10 to
  undo the 0.1 packing, and add a back-lit term (`dp3_sat(-N, L)`, exponent 32). INFERRED: this is
  the transmission term; its magnitude is at most `1.0 × mask × 1.0`, small next to the emissive
  case.
- **Native gap:** only `FC_TranslucencyScale` (GX05, §2.2). Everything else the family needs is the
  path the default family already takes (row 5, `Gbuf`/`Gbuf_[A]`, `GBuf_Skin[@cl]`). Until GX05 is
  parsed the family draws as the default family with `GB1.y = 0`.
- **Pass order:** G-buffer (engine 5) → deferred light accumulation (the translucent term lives in
  the light programmes) → CmbAlbedo → Composite. Nothing is needed in a forward pass.

### 3.2 SSS of enemies `GXFlver_SSSIbl_Bloody[_Cloth]` (STATIC)

- **Users:** 51 + 2 materials (125 `Data3` characters; 77 materials over 35 characters in all, with
  the DLC), all with MTD `g_GX_MaterialType` 3, `g_SSSWidth` 0.025, `g_SSSStrength` (1,1,1),
  `g_SSSStrength_UseCAParam` True, GX05 and GX15. W2: `c2020` body (GX15 0.3, class 4); W6:
  `c3190`, `c3200` (0.3), `c1400` skin (1.0).
- **Same family as the player skin.** The player's skin materials are `P[ARSN]_sss` →
  `GXFlver_SSSIbl_Bloody` (`chrmat.tsv`), which the native build draws and blurs (Native1137-1141).
  So the programme set, the texture binding and the SSS stage already run on this exact family;
  nothing in the programme is enemy-specific.
- **Programme delta against the default family** (`fam-diff.txt`, `asm/`): one more texture `t21`
  `g_SSSMaskTexture` (the FLVER type is `g_ScatteringMaskTexture` on all 51 materials; INFERRED: mapped through the MTD texture list, as for the player's SSS materials) and sampler
  `s8`; the same five outputs. It writes `GB1.y` as in §3.1, **`GB1.w = FC_SSS_ID / 255`**
  (`cb2[25].x × 0.003922`, word `+400`) and **`GB2.w` = the mask sample**.
- **Stencil:** the G-buffer draw sets stencil `9`, and `|4` when `+0x24` has `0x800`
  (`game_bootstrap.cpp:4316-4318`); `GXSssBlurX/Y` read bit 4 only. `+0x24` bit `0x800` is produced
  from `g_GX_MaterialType == 3` (`:6733-6742`, Native1185); MENUS-MAP 71 shows the rule exact over the 512 MTDs (the 38 SSS MTDs are exactly the type-3 ones).
- **Profile:** `NativeSssProfileOf` (`:2534`) reads the GX15 item of the FLVER material and needs the
  MTD block `g_SSSWidth`. All 77 materials carry GX15. In the 125 `Data3` characters the strengths
  are `1/1/1` (49), `0.3` (7), `0.1`, `0.2/0.2/0.21`, `0.4`, `1/0.4/0.4` (1 each): six distinct
  strength triples with one base width, so six profiles plus the default, far below the 0xE3 entries of `cbSSS`.
- **`g_SSSStrength_UseCAParam`** is True for every enemy SSS MTD and every one has a GX15 instance,
  so TONEMAP-SKIN 8's rule selects the instance profile (`FC_SSS_ID` = the instance slot). The
  "strength from the character's extra parameters" reading is therefore not exercised by any enemy
  material; the open comment at `game_bootstrap.cpp:2505` can be closed for them. INFERRED: the
  switch only matters when no instance exists.
- **`FC_SSS_ID` is an index, not a value to match:** the slot is assigned per frame in the original
  (TONEMAP-SKIN 18, `+0x36`); the native registry numbers profiles in its own order. Parity is
  visual (the blurred result), not numeric.
- **Variants:** `…SSSEmiIblGlow_Bloody` (7 materials, in `c2132` and `c5180`) is class `0x100`: row 7,
  so it also needs the union (§2.1) and GX03. `…SSSIbl_Bloody_Cloth` (2) is class 4 with `@[cl]`.
- **Gap:** `FC_TranslucencyScale` (GX05), as §3.1. Nothing else.

### 3.3 What W2 settles and what it cannot (summary)

| Item | Status |
|---|---|
| Translucent class/row/programme/interface | settled (this part) |
| SSS programme, stencil, profile, strengths | settled; same family as the player |
| `FC_TranslucencyScale` | GX05 parse (§2.2); expected values 1.0 for every witness material |
| Which programme the original really binds for `c1280_cloak_cloth` | LIVE-ONLY (§7, block W2) |
| Visual parity of the SSS blur on an enemy | LIVE-ONLY (§7, block W2) |

## 4. W3 and W7: emissive and vertex offset (c2150, c1090 at W3; c5110 boss at W7)

### 4.1 Emissive G-buffer families `…EmiIbl_Bloody`, `…EmiIblGlow_Bloody[_Cloth]`, `…SSSEmiIblGlow_Bloody` (STATIC)

- **Users** (125 `Data3` characters): `EmiIblGlow` 13 materials (7 characters), `EmiIbl` 7 (3:
  `c1090`, `c5110`, `c5280`), `SSSEmiIblGlow` 7 (`c2132`, `c5180`); the `EmiIblGlow` characters are `c1200 c1310 c2132 c2150
  c2230 c5160 c5270`. W3: `c2150` (2 `crystal` materials, GX03 = 1.0)
  and `c1090` (`#23`, `EmiIbl`, GX03 = **5.0**); W7: `c5110` boss (`Material #17`, `EmiIbl`,
  GX03 = **5.0**); `c5270` (W4) carries two `EmiIblGlow` materials with GX03 = 0.5.
- **Class and row:** `0x100` (the family name contains `Emi`, flag `0x8000`); MTD blend 0 (the one
  `c[arsn]_em_al_glow.mtd` is blend 2 → class 2, row `0x29`). Row **7** (engine 7, preparation 3,
  shader pass 6 = `Gbuf`), after rows 5 and 6 in the same G-buffer pass. It exists only with the
  union (§2.1).
- **Programme delta against the default family** (`fam-diff.txt`; disassembly diff in `asm/`):
  - one texture, **t6 `g_EmissiveTexture`** (FLVER type present on all 27 materials) and sampler
    `s8` (`SS_WrapLinear`);
  - **six outputs**, `SV_Target5` = GB5 (the default family has five):
    `GB5.xyz = t6(uv × cb1[6].xy + cb1[6].zw).xyz × FC_EmissiveColor.xyz`,
    `GB5.w = FC_GlowScale.w` (Glow variants; `cb3[9].w`, word `+144` of `cbMtdParam`);
  - the tiling table gains row 6 (emissive tiling), fed by the slot record **GX32** (`+0x284 |= 0x2000`),
    whose slot records GX20–GX34 the native build already reads (`texture_tiling.cpp:43-56`; GX32 is slot 6; the materials carry `GX20 GX22 GX24 GX26 GX32 GX30`);
  - `FC_EmissiveColor` (b2 `+0x20`) and, for `Glow`, `FC_GlowScale` (b3 `+0x90`, from the MTD,
    `g_GlowScale` (1,1,1,1) on `C[ARSN]_em_e_Glow`).
- **Native status:** the six-target G-buffer pass and the composite that adds GB5 exist
  (EMISSIVE-GLOW §3, "DONE for admitted families"). Missing, in order: (1) the union (§2.1), without
  which the mesh is filtered out of row 5 and row 7 does not exist; (2) `FC_EmissiveColor` (§2.2:
  GX03 flag never set, so the emission is black even after (1)); nothing else is enemy-specific.
- **Pass order:** G-buffer rows 5, 6, 7 → deferred lights → CmbAlbedo (+SSS) → Composite adds GB5 →
  glow filter (EMISSIVE-GLOW steps 3–8, from GB5) → tone map. The enemy rows 7 therefore feed the
  same glow chain as map pieces.
- **Expected constants for the live comparison** (§7): `cb2[2]` (`FC_EmissiveColor`) =
  `(5,5,5,1)` for `c1090 #23` and `c5110 Material #17`, `(1,1,1,1)` for `c2150 crystal`, `(0.5,0.5,0.5,1)`
  for `c5270`; `cb3[9]` (`FC_GlowScale`) from the MTD.

### 4.2 Vertex offset `FlowmapVertexOffset` (STATIC)

- **Users:** 7 materials in 4 characters (`c1090` ×1, `c1220` ×2, `c2060` ×2, `c5110` ×2). MTD
  `C[FlowVertexOffset].mtd`: blend 0, depth write on, `g_DisplacementEnable` 1, `g_TessellationMode`
  0, no `GXFT_` flags, AlphaRef 0: class **1**, row 5, `Gbuf`/`Gbuf_[A]`, skinned `Gbuf_Skin`.
  The SAT bundle also has a `_em` variant (`FlowmapVertexOffset_em`, 6 outputs, `FC_GlowScale`), used
  by no character in the 125 `Data3` ones (no `_em` MTD in `chrmat.tsv`).
- **What the vertex programme does** (`asm/FlowmapVertexOffset__Gbuf_Skin.vpo.asm`):
  `uv = (tex0 × cb11 + cb11.zw) + FC_GlobalTime × snp_float_0_0`, mapped through tiling row 5,
  `h = Texture2D_5.SampleLevel(s8, uv, 0).x`, offset = `h × COLOR1.x × snp_float_1_0` (applied to the
  skinned position along the vertex direction). It reads `cbMatDynParam` words `+624`/`+628`
  (SAT generated words), `cbPerFrame.FC_GlobalTime` (`+176`), `cbSceneParam`, the palette `b5`,
  `b11` scroll, `b12` cancel palette. The pixel programme reads `snp` words `+632`/`+636`,
  `FC_GlobalTime` and four textures (t1, t3, t4, t5).
- **Textures:** the FLVER carries `FlowmapVertexOffset_snp_Texture2D_{6 AlbedoMap, 5 DisplacementMap,
  4 FlowMap, 3 NormalMap, 1 ReflectanceMap}_0`. Pixel stage t1/t3/t4/t5 ← 6/4/3/1; **vertex stage
  t2 ← 5** (the displacement map).
- **Two certain native refusals (STATIC, both read in source):**
  1. `SelectModelShaderTextures` returns `Unsupported` when `shader.stage != Pixel` and the shader
     declares a texture (`native/content/model_textures.cpp:86`). The vertex programme declares
     `Texture2D_5` at t2, so `shader_textures[recipe][0].status` is not `Ready`, and the draw is
     refused at `game_bootstrap.cpp:4696-4701` (`TextureWitness`, reason 1, stage 0).
  2. `cbPerFrame` (b9) is produced only for the pixel stage (`game_bootstrap.cpp:4518`,
     `else if(stage && slot==9 && …)`). The vertex programme reads `FC_GlobalTime` from it; the block
     stays unproduced and the draw is refused as `native_selected_constant_unavailable` (stage 0,
     slot 9, family `FlowmapVertexOffset`).
- **Fix** (small): (a) allow vertex-stage shader-resource views in the native input assembly of
  models (texture selection, descriptor visibility, the `s8` sampler is already in the common bank
  table, `common_banks[8]`); (b) produce `cbPerFrame` for the vertex stage with the same bytes as
  the pixel stage (drop the `stage &&`). No new data is needed: the texture is a normal FLVER
  texture and the time is the one the pixel stage already uses.
- **Ordering of the refusals:** the texture refusal comes first in the loop; both appear in the same
  frame under different events. After the fix the family is an ordinary class-1 deferred mesh.

### 4.3 What W3 and W7 settle and what they cannot

| Item | Status |
|---|---|
| Emissive row, programmes, GB5 formula, constants, producers, order | settled (this part) |
| Union for enemy models | one produced word, settled (§2.1); original value LIVE-ONLY |
| `FC_EmissiveColor` | GX03 parse; expected 1.0 / 5.0 / 0.5 |
| Flowmap refusals and fix | settled |
| Flowmap values (snp words `+624/+628/+632/+636`, time) in the original | LIVE-ONLY (§7, blocks W3 and W7) |
| W7: the boss must be alive and the fight active (MENUS-MAP 69) | save-dependent |

## 5. W4 and W5: veil (c5270 boss) and crystal (c3100)

### 5.1 Veil `Veill_NoLight[_cloth]` and `Veill[_cloth]` (STATIC)

- **Users** (125 `Data3` characters): `Veill_NoLight` 1 + `_cloth` 4 materials, all in `c5270`
  (`veil`, `veil_cloth`, `veil2_cloth`, `Waist_cloth`, `Shoulder_cloth`); `Veill` 2 + `_cloth` 2 in
  `c2170`/`c2230`. DLC: `c6232` (`Veill_NoLight`, blend 3). `c5270` also carries two
  `EmiIblGlow` materials (GX03 = 0.5, row 7) and default families.
- **Class and row:** MTD blend 2 (`c6232`: 3), depth write off, `GXFT_ForceForward`. The replay
  gives class **2** (blend precedes the forward flag), so the **forward row `0x29`**
  (preparation 0, shader pass 0 = `Fwd`), after the deferred lights and the opaque forward rows
  (NATIVE-PASS-ORDER row 9). The `Gbuf` programmes of these bundles exist but are never selected for
  this class.
- **`Veill_NoLight_Fwd` pixel programme** (`asm/Veill_NoLight__Fwd.ppo.asm`, 53 instructions; **no
  lighting, no IBL, no SSAO, no light lists, no depth texture**): it rebuilds the world position from
  an interpolated clip-space `TEXCOORD` with `FC_MatrixInvViewProj` (b8 `+128`), takes the camera
  from `FC_MatrixInvView` (`+320`), computes a Fresnel-like term
  `pow(sat(N·V) + snp_float_2_0, snp_float_1_0)`, blends `snp_color_0` (RGB, words `+624..+632`) with
  the albedo texture `Veill_NoLight_snp_0_Texture2D_2` (t2, sampler `SS_DiffuseMap` s0), multiplies by
  `FC_DifMapMulCol` (b3) and by the per-instance `mulCol` (`VC_InstanceData[idx].mulCol`, row 7 of the
  record), applies the height fog of `cbFog` (b6) and writes one target.
- **`Veill_Fwd` (`c2170`, `c2230`)** adds lighting: `cbLight`, light lists t59–t62, `g_SSAOTexture` t33,
  a second texture t3 (`Texture2D_1`), sampler s8. Same row.
- **Native status:** forward row `0x29` exists and draws eyebrows and hair cards (Native1115-1117,
  1181); blend 2 is admitted and blends 3 and 4 are not (FORWARD-DEPTH-NATIVE, COVERAGE-MATRIX row 28),
  which excludes `c6232` and the additive `c2030`/`c2040`. Depth write off is the current behaviour.
  The programmes need only: texture t2 (FLVER `Veill_NoLight_snp_Texture2D_2_AlbedoMap_0`), the snp
  words (generated parameters, the mechanism that already drove `SAT_Equip_Veill`, Native1128),
  `cbFog` (forward policy), `cbSceneParam` (the inverse matrices are produced, `producers.h:27-28`)
  and the cloth VS for the `_cloth` materials (cloth done, Native1185).
- **Tessellation:** flat variant, §2.3. The `_PN` question (SAT bundles list only `_FT`/`_FT_D`) is
  LIVE-ONLY and has no native consequence.
- **Gap:** none specific. The forward-state items of FORWARD-DEPTH-NATIVE and the two-target forward
  binding still apply to every `0x29` draw. The only unsettled point is `ShortBoneIndices`: `c5270`
  has 353 bones with 16-bit bone indices (MENUS-MAP 66/67). The native layout is the engine's own
  element table (`BuildModelLayout` receives the engine's rows, `game_bootstrap.cpp:6592-6620`, and
  `DecodeModelVertexLayout` accepts formats 1–99), and the palette is padded to `max(256, rows)`
  (Native1184) so 353 × 3 rows fit; INFERRED that the draw passes. The outcome tally of the pseudo
  part 1000 names the gate if not (§7, block W4).

### 5.2 Crystal `RecursiveCrystal` (STATIC)

- **Users:** `c3100` (3 materials `crystal`, `headcrystal`, `Mouth_Crystal_Inside`; DLC `c6020`).
  MTD `c[crystal].mtd`: blend 0, `GXFT_PaintDecal_Mat`, `GXFT_Emissive`, **`GXFT_Recursive`** →
  class **2** (bit 21 precedes the emissive class), forward row `0x29`, blend 0, depth write on.
- **Forward pixel programme** (32,612 bytes; `asm/RecursiveCrystal__Fwd.ppo.asm`): textures t1
  (`Texture2D_5`), t3 (`_3`), t4 (`_2`), t5 (`_1`); IBL t30/t31, SSAO t33, decals t41/t42, RLR t44
  (`sample_l`), light lists t59–t62; samplers s0, s1, s8, s9, s11; constants b1, b2 (snp words
  `+624..+668`), b3 (`FC_BloodAmount`, `FC_GlowScale`), b4, b5 `cbLight`, b6 `cbFog`, b7, b8; **two
  outputs**: `o0` lit colour, `o1` = GB5 (`xyz = t1 × snp_color_1 × FC_GlowScale.xyz`).
- **`t45` is the HDR frame copy (STATIC, new).** `RecursiveCrystal_snp_0_Texture2D_0` at t45 is
  sampled at `SV_Position.xy / FC_ScreenSize + normal.xy × snp_float` with the clamp-aniso sampler
  s11: a screen-space **refraction** of the scene colour. The FLVER has no `Texture2D_0` for the
  crystal (it carries `_1 Reflectance, _2 Albedo, _3 Normal, _4 and _5 Shininess`), so this is not a
  material texture: it is the engine's slot 45, the scene copy the original binds with `59470`
  (WATER-NATIVE; NATIVE-PASS-ORDER row 10). That is what `GXFT_Recursive` selects.
- **Native refusals, in order:**
  1. **t45 unbound.** The scene slots the native build admits for model draws are 30, 31, 33, 38, 41,
     42, 44 and 59–62 (`game_bootstrap.cpp:3380-3403`); no material texture names t45, so
     `SelectModelShaderTextures` returns `Missing` and the mesh is refused
     (`native_selected_texture_unbound`, slot 45, `RecursiveCrystal_snp_0_Texture2D_0`).
  2. The second target: native forward binds one (FORWARD-DEPTH-NATIVE, EMISSIVE-GLOW §5 step 1).
- **Ordering problem (STATIC + INFERRED):** the original copies HDR to the scene copy at the water
  pass (NATIVE-PASS-ORDER row 10), which comes **after** forward rows `0x1F`/`0x29` (row 9). So when
  a crystal is drawn in row 9, the copy bound at t45 is either the previous frame's or one made by
  another pass before row 9. Which one is LIVE-ONLY (§7, block W5: the event index of the last write
  of the t45 resource before the crystal draw, and its size and format).
- **Native contract:** create a scene-colour copy resource (the one WATER-NATIVE step 3 and the E04/E05
  effects need), bind it as scene slot 45 for forward model draws, and write it at the point the
  original does (to be read in W5). Until then the crystal can be admitted by binding any valid
  texture at t45 (the refraction then samples a wrong image but nothing is refused).
- **Other inputs are produced:** snp words (`RecursiveCrystal` declarations), `FC_GlowScale` (MTD),
  IBL/SSAO/RLR/light lists (explicit forward policy, `game_bootstrap.cpp:3399-3403`),
  `cbLight`/`cbFog` (`:4538`).

### 5.3 What W4 and W5 settle and what they cannot

| Item | Status |
|---|---|
| Veil class, row, interface, minimal constants, tessellation fallback | settled |
| Veil `_PN` variant in the original; blend and depth state of the draw | LIVE-ONLY (§7, W4) |
| `ShortBoneIndices` on `c5270` | INFERRED fine; the outcome tally names the gate (§7, W4) |
| Crystal row, programme, `t45` meaning, refusals | settled (t45 is new) |
| What the original binds at `t45` and when it was written | LIVE-ONLY (§7, W5) |
| W4 requires the boss alive and the fight active | save-dependent (MENUS-MAP 69) |

## 6. W6: eyes, cinder, and the Firelink SSS (c1400, c1450, c3190, c3200)

### 6.1 Eyes `GXFlver_EyeIbl_Bloody` (STATIC)

- **Users:** one material in the 125 `Data3` characters (`c1400` `eye`, MTD `C[eye]_Decal`); DLC
  `c6020`, `c6120`, `c6121` (a data read, §7). Class 1, row 5, `Gbuf`.
- **Interface:** the default family plus `cbModelParam` (PS b7, `FC_SkinMatchColor` `+96`) and
  `cbMatDynParam.FC_ParallaxParam` (`+96`). Textures t0, t1, t3, t41, t42; `g_HighLightTexture` is in
  the FLVER but not read by this programme.
- **Arithmetic** (`asm/GXFlver_EyeIbl_Bloody__Gbuf.ppo.asm`): `uv' = uv + view.xy × (bump.w ×
  ParallaxParam.x + ParallaxParam.y)` (an iris parallax, `bump` = `t3`); the diffuse (`t0`) is sampled
  at `uv'`; `colour = sat(SkinMatchColor × diffuse.a + colour)` (a skin-match tint under the
  diffuse alpha). With the witness's GX09 words `{0, 0.5, 0, 0.5}` the block is `{0, −0, 0, −0}`:
  the parallax is zero, so the GX09 parse changes nothing for `c1400` and the native zero is correct.
  The tint term does depend on `FC_SkinMatchColor`.
- **Native status:** `cbModelParam` is produced from the selected group (`game_bootstrap.cpp:4494-4515`,
  `FC_SkinMatchColor` = group `+0x40` = entity `+0x280`, the engine's own CPU value). Nothing is
  missing: **settled, no gap**; the value for an enemy is the engine's (LIVE-ONLY check, §7).
  The DLC eyes may carry other GX09 values: a data read of their FLVER, not a run.

### 6.2 Cinder `C[ARSN][cinder]` and relatives (STATIC)

- **Users:** `C_ARSN_cinder` 49 materials (7 characters), `_cloth` 20 (6), `_em` 14 (5),
  `_em_cloth` 1, `C_ARSN_em` 8 (3). W6: `c1450` (5 materials). Class `0x100` (GXFT_Emissive), row 7.
- **Programmes:** SAT bundle `C[ARSN][cinder]` (the cache names carry the brackets; the MTD spx name
  is the sanitised `C_ARSN_cinder`): `Gbuf`, `Gbuf_[A]` (AlphaRef 128 on the `_e` MTDs), skinned VS
  (`_cloth` bundle for the cloth materials).
- **Interface** (`fam-reflect.tsv`): six outputs; textures `Texture2D_5` t1 (albedo), `_4` t2 (normal),
  `_3` t3 (reflectance), `_1` t4 (blend mask), `_0` t5 (emissive), `SS_WrapLinear` s8, decals t41/t42
  (all present in the FLVER); `cbMatDynParam` word `+624`; `cbMtdParam` builtin variables, `FC_GlowScale`
  and nine SAT numeric words `+432..+560`; **`cbSceneParam`** `FC_MatrixInvViewProj`, `FC_MatrixInvView`
  and **`cbPerFrame`** `FC_GlobalTime` in the pixel stage (both produced); VS `b11` rows 0 and 2,
  `b12` cancel palette.
- **Native gap:** the union only (§2.1). The GX items 03/05/09 are absent from these materials
  (`all-chr-materials.tsv`), so §2.2 does not touch the family. The cancel palette follows the
  identity policy already in place (`game_bootstrap.cpp:4553`).

### 6.3 SSS at Firelink (`c3190`, `c3200`, `c1400` skin)

All three are `GXFlver_SSSIbl_Bloody` class 1 with GX15 0.3, 0.3 and 1.0 (§3.2). Nothing beyond §3.2.

### 6.4 What W6 settles and what it cannot

| Item | Status |
|---|---|
| Eye programme, constants, parallax (zero) | settled; nothing to produce |
| Cinder interface and producers | settled; gap = the union |
| Value of `FC_SkinMatchColor` (entity `+0x280`) on `c1400`, both modes | LIVE-ONLY (§7, W6); expected equal, it is engine CPU data |
| DLC eye GX09 words | data read |
| Cinder snp words in the original (for visual parity) | LIVE-ONLY (§7, block W6) |

## 7. The live reads, grouped in one block per witness stop

Order of work for the pilot: **code the five static changes first** (list below), then run the stops
W2 → W3 → W5 → W6 (no fight: W2, W3 and W5 per MENUS-MAP 69; W6 is the Firelink NPCs), then
W4 and W7 (the boss arenas). Each stop is one camera position; the reads of a stop are taken in the **original** (a
capture or a break at the named function) and the **native** build (the named event), same
position, same frame budget.

**Static changes that need no live read first (each is settled above):**

| # | Change | Where | Unlocks |
|---|---|---|---|
| C1 | produce the model union for every model | `game_bootstrap.cpp:3720` (drop the `neutral_instance_decal` condition) | rows 7/0x1F for 37 characters, 202 meshes (§2.1) |
| C2 | parse GX03, GX05, GX09 into the material (`mat+0xDE/0x104`, `+0xE0/0x108`, `+0xE3/0x200..0x20C`) | `BuildModelMaterial`, `:6692-6742` | emission, translucency (§2.2) |
| C3 | vertex-stage texture binding, `cbPerFrame` for the vertex stage | `model_textures.cpp:86`, `game_bootstrap.cpp:4518` | `FlowmapVertexOffset` (§4.2) |
| C4 | a scene-colour copy bound as slot 45 | native scene slots `:3399-3403`; WATER-NATIVE step 3 | `RecursiveCrystal` (§5.2) |
| C5 | forward `_Phantom_` selection when the group byte `+0x87` is set (phantom draws only) | `model_programs.cpp:52-57` | phantoms (PHANTOMS §1.1, corrected) |

**Block 0: once, at any stop with an enemy in view (both modes)**

| Id | Read | Where | Expected |
|---|---|---|---|
| L0.1 | the model words `+0x220`/`+0x224` of each `SprjModelDrawEntity` model | original: the model argument of `57A20` at the row decision (the rows it adds); native: `Read(model+0x220/+0x224)` at `game_bootstrap.cpp:3970-3977` (extend the `native_asm_engine7_row` witness to the pseudo part 1000) | bit 15 set for `c2150`, `c1090`, `c5110`, `c5270`, `c1450`; bit 12 clear; native `0xFFFF`/0 before C1 |
| L0.2 | the refusal tally before the changes | native events `native_asm_part_outcomes` (pseudo parts 1000/1001, per family key), `native_selected_texture_unbound` (slot, stage), `native_selected_constant_unavailable` (stage, slot, family) | `FlowmapVertexOffset`: texture stage 0 and slot 9 stage 0; `RecursiveCrystal`: slot 45; class `0x100` families: gate 21. Rerun after C1–C4: every refusal must be gone |

**Block W2: `m30_00` about (10, −6, 95); `c1280`, `c1105`, `c2021`, `c2020`, `c1290`**

| Id | Read | Where | Expected |
|---|---|---|---|
| L2.1 | the pixel programme of the `c1280_cloak_cloth` draw | original: capture, match the blob in the table of §9 (`TrIbl_Bloody_Cloth` `Gbuf_[A]` against its `Fwd`) | `Gbuf_[A]`, five render targets (confirms §2.4: deferred) |
| L2.2 | `cb2` row 1 (`FC_TranslucencyScale`) of that draw | original: the PS constant buffer b2, `+0x10` | 1.0 (GX05); native 0 before C2 |
| L2.3 | `c2020` body: stencil reference, `cb2` row 25 (`FC_SSS_ID`), the `GXSssBlurX` `cbSSS` record of that id | original: draw state and b2 `+400`; b1 of the blur | stencil 13 (9 \| 4); strength 0.3, width 0.025; the id is an index (compare the blurred result, not the number) |

**Block W3: `m30_00` about (−48, 20, 74); `c2150`, `c1090`**

| Id | Read | Where | Expected |
|---|---|---|---|
| L3.1 | `FC_EmissiveColor` and `FC_GlowScale` | original: b2 `+0x20`, b3 `+0x90` of the `c2150 crystal` and `c1090 #23` draws | `(1,1,1,1)` and `(5,5,5,1)`; `FC_GlowScale` from the MTD |
| L3.2 | the GB5 texel under those draws | original: render target 5 after the row; native: the 6th target | non-zero; equal after C1–C2 |
| L3.3 | `FlowmapVertexOffset` vertex stage (`c1090` skin material) | original: vertex SRV t2 (resource, size, format), b2 `+624/+628`, b9 `+176` | the displacement map (FLVER `…_Texture2D_5_DisplacementMap_0`); the snp words and the time |

**Block W5: `m40_00` TestAnim start (−12.3, −54.9, 467.3); `c3100` ×3**

| Id | Read | Where | Expected |
|---|---|---|---|
| L5.1 | the SRV at t45 of the crystal `Fwd` draw: resource, size, format, and the event index of its last write before the draw | original: capture of the pixel SRV table and the preceding copy/clear events | the HDR scene colour; whether it is this frame's copy or the previous frame's (§5.2) |
| L5.2 | the render targets and states of the crystal draw | original: RTV list, blend, depth-stencil | `[HDR, GB5]`, blend off, depth write on |
| L5.3 | b2 `+624..+668` | original | the SAT words for the native comparison |

**Block W6: `m40_00` Firelink, about (270, −53, 608); `c1400`, `c1450`, `c3190`, `c3200`**

| Id | Read | Where | Expected |
|---|---|---|---|
| L6.1 | `FC_SkinMatchColor` of the `c1400` entity | original and native: entity `+0x280` (PS `cbModelParam` `+96`) | equal (engine CPU data) |
| L6.2 | the cinder family of `c1450` | original: b3 `+432..+560`, b2 `+624`, b9 `+176`; the number of row-7 draws | the SAT words; the draw count to match native after C1 |
| L6.3 | SSS of `c3190`, `c3200`, `c1400` skin | as L2.3 | strengths 0.3, 0.3, 1.0 |

**Block W4: the arena (28, −10, 158); the boss `c5270` alive**

| Id | Read | Where | Expected |
|---|---|---|---|
| L4.1 | the vertex, hull and domain programmes bound at the veil draw | original: the six shader slots of the draw (SHA of the blobs; the hash table of §9 has the plain `Fwd` ones) | `Veill_NoLight_Fwd_Skin` with or without `_FT`; whether any hull/domain is bound (§2.3) |
| L4.2 | blend, depth-stencil and target count of the veil draw | original: draw state | blend 2, depth write off, one target (row `0x29`) |
| L4.3 | `c5270` outcome in native (353 bones, 16-bit indices) | native event `native_asm_part_outcomes` (pseudo part 1000), `native_character_candidate` | `admitted`; otherwise the gate code names the layout or palette problem |

**Block W7: `m40_00` (142.9, −68.9, 594.6); the boss `c5110`**

L3.1, L3.2, L3.3 for `c5110` (`Material #17` `EmiIbl`, GX03 = 5.0; two `FlowmapVertexOffset`
materials), plus L0.1.

**Data reads (no run):** the FLVER GX lists and MTDs of the 40 DLC characters (`c6020 c6030 c6040 c6050
c6090 c6100 c6120 c6121 c6232 …`, archives other than `Data3`) for: GX09 of the DLC eyes, `Fur_FDP`
users (class `0x100`, `Gbuf_[A]`), `c6232` (blend 3, not admitted today). Method: `Dump-FlverGx.ps1`
after unpacking the DLC archive with BinderTool, as done here for `Data3`.

## 8. The other open families of the NPC workstream

| Family | What static says | Status |
|---|---|---|
| **Ghost** `…EmiMulIblGhost[_Cloth]` (`c1480`, `c1490`) | class 2, forward row `0x29`; the forward programme reads two texture layers (t6–t8, t10, t13, all in the FLVER), `FC_GlowScale`, RLR t44 (explicit policy), `cbSceneParam` inverses, and **no GX-fed word** (GX03 is on the FLVER but unused: no `FC_EmissiveColor` in the programme). `g_GhostEdgeColor/Param` unused (MENUS-MAP 72) | no family-specific gap beyond the forward two-target binding; blend 2 admitted |
| **Fur** `Fur_FDP` (`c6030`…`c6100`, DLC) | class `0x100` (Emissive precedes the alpha test), row 7, `Gbuf_[A]`; six outputs; the Gbuf programme reads no GX-fed word (`fam-used.txt`: Wet rows and SAT words only) | needs only C1; the FLVER is a data read |
| **Tessellated** (`Veill*`, `FlowDisplacement`) | never selected natively (§2.3); flat variants are valid draws | closed as a fidelity item |
| **Auxiliary cloth palette** (b12 `cbClothCancelObjMatrix`) | read by the skinned VS of the cinder bundle (`cbClothCancelObjMatrix`); native binds an identity cancel for current-route draws (`game_bootstrap.cpp:4553-4560`) | explicit policy stands; visual check at W2 (`c1280` cloak) |
| **Other `140F90` words** (`+0x18`, `+0x1C`, `+0x21`, spx-name bits of `+0x24`, `+0x80`) | the class (`+0x18`) is recomputed natively from the same rule; `+0x1C` is read only as `material_secondary` against mask `0xFF` (any non-zero value passes; characters draw, so non-zero); the `0x800`/`0x2000` bits are produced; the name bits `0x4000000`/`0x8000000` select the alternate (phantom) set and the Wet gate, neither needed for the offline families (§2.2, MENUS-MAP 75) | no further word is needed by W2–W7; closed unless a phantom is drawn |
| **`ShortBoneIndices`** (7 characters, palette up to 1770 rows) | layout from the engine's own element table, palette padded to `max(256, rows)` | INFERRED fine; verified by L4.3 on `c5270` |
| **`Phantom_Fwd` parity** (STATE: "add the variant for the 64 families") | **withdrawn**: `_Phantom_` programmes exist only in the alternate set chosen when the draw's state byte `+0x87` is set (MENUS-MAP 75). A normal draw, the player's hair included, takes the plain `<family>_Fwd`: the native choice is right and must not be changed. PHANTOMS-SIGNS-FOGWALLS-NATIVE §0, §1.1, P4, COVERAGE-MATRIX row 10 and LIVE-CHECKS P4·PH are corrected in this part | closed for normal draws; C5 applies to phantom draws only |
| **Enemy SSS profiles** | all 77 materials have GX15; six strength triples; `UseCAParam` true on all, so the instance profile always applies (§3.2) | closed statically |

## 9. Programme hashes (for matching draws in a capture)

SHA-256 of the DXBC blob, first 16 hex characters; full values in
`evidence/I-0068/queries/npc-families/fam-reflect.tsv`. The DXBC checksum of these and of every other programme (effects, glow, velocity, shadows, lights, decals) is in PROGRAM-FINGERPRINTS (part AM). If a capture identifies programmes by another
hash, match by size and by the resource list in the same file.

| Family | Programme | Bytes | SHA-256 (first 16) |
|---|---|---|---|
| `GXFlver_ColDifSpcBumpTrIbl_Bloody_Cloth` | `Gbuf.ppo` | 9548 | `7484FAAC008D4EE2` |
| `GXFlver_ColDifSpcBumpTrIbl_Bloody_Cloth` | `Gbuf_[A].ppo` | 12348 | `C684703D7916981B` |
| `GXFlver_ColDifSpcBumpTrIbl_Bloody_Cloth` | `GBuf_Skin.vpo` | 11244 | `EF3B95FE0FA6127D` |
| `GXFlver_SSSIbl_Bloody` | `Gbuf.ppo` | 9752 | `BDE982D1A18615BA` |
| `GXFlver_SSSIbl_Bloody` | `Gbuf_[A].ppo` | 12552 | `7B829D3F709FD2D1` |
| `GXFlver_SSSIbl_Bloody` | `GBuf_Skin.vpo` | 6572 | `8C9B851160967E7E` |
| `GXFlver_ColDifSpcBumpEmiIblGlow_Bloody` | `Gbuf.ppo` | 9832 | `32CD922EA4A998B5` |
| `GXFlver_ColDifSpcBumpEmiIblGlow_Bloody` | `Gbuf_[A].ppo` | 12584 | `2A84C2C3749877BB` |
| `GXFlver_ColDifSpcBumpEmiIbl_Bloody` | `Gbuf.ppo` | 9816 | `A4C865C4505C6AF2` |
| `FlowmapVertexOffset` | `Gbuf.ppo` | 11808 | `D9A04FA055FB26CB` |
| `FlowmapVertexOffset` | `Gbuf_Skin.vpo` | 11784 | `7CE4059BB6775AAD` |
| `Veill_NoLight` | `Fwd.ppo` | 11896 | `B7FDC9EA628E46E4` |
| `Veill_NoLight` | `Fwd_Skin.vpo` | 6468 | `2877F3A5FA4048EA` |
| `RecursiveCrystal` | `Fwd.ppo` | 32612 | `12C4DE0AF13E7E67` |
| `GXFlver_EyeIbl_Bloody` | `Gbuf.ppo` | 11600 | `86E857A7111FD348` |
| `C[ARSN][cinder]` | `Gbuf.ppo` | 15252 | `E209FE3518E96206` |
| `C[ARSN][cinder]` | `Gbuf_[A].ppo` | 18004 | `E70966D2B853CEC4` |
