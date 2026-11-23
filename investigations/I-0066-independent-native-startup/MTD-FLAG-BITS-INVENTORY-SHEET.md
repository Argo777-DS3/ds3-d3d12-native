# Material policy word (`material+0x24`) and model words (`model+0x224`, `+0x468`): bit inventory and implementation sheet

Request (coordinator, 2026-10-11): inventory every bit of `material+0x24` and `model+0x468` that the original GX MTD
handler (`FUN_140140F90` / `FUN_1400A7E40`) sets; per bit compare with the native constructor, name the readers, the
MTD families (water bit 17, Eclipse bit 19, ...), the expected visible effect and the fixes. Static only; nothing was
launched. Labels: **STATIC** (read in the image or in retained data), **INFERRED** (deduced from names or context),
**OPEN** (not established).

Related: [WIND-BLOCK-IMPLEMENTATION-SHEET](WIND-BLOCK-IMPLEMENTATION-SHEET.md) (bit 1), MENUS-MAP sections 66 to 71,
[DIRECT-MATERIAL-PREPARATION](DIRECT-MATERIAL-PREPARATION.md), I-0068 WATER-NATIVE and NPC-FAMILIES-STATIC.

## 0. Result

1. **The word has three writers and one reader chain.** `FUN_1400A7E40` (per-parameter handler) ORs builtin-parameter
   bits into `reader+0x3C`; `FUN_14002DD60` (MTD loader) copies it to `gxMtd+0x88` and ORs ten shader-name bits (eleven OR sites);
   `FUN_140140F90` copies it to `material+0x24` and replaces bit 28 (program set ready); it writes nothing when `gxMtd` is null and
   ends with a tail call to `material vt+0x60(material, gxMtd)` (an empty `RET`, `FUN_140140460`). **Many readers read the
   `gxMtd+0x88` copy through `material+0x80`, not `material+0x24`** (section 2.0). `FUN_14002A120` then ORs every
   material word into the model word `model+0x224` (AND for bit 28). Bits 29 to 31 are never written.
2. **29 bits are used (0 to 28).** 18 come only from builtin MTD parameters (bits 1 to 9, 11, 13, 14, 17 to 22; bit 2
   also from the parameter name `g_BloodAmount`), 3 from a parameter or a shader name (10, 12, 15), 7 only from shader
   names (0, 16, 23 to 27), 1 from a virtual call (28). Section 2 gives every bit with origin, MTDs, readers, effect
   and native state.
3. **Native has two sources of truth and they disagree.** The native policy `ModelMaterialPolicy.flags`
   (`model_parameters.cpp:433-476`) implements the `A7E40` rule faithfully (kinds 21 to 40, `g_BloodAmount`, the
   name bits except 26 and 27, which `known_flags_mask` removes). The word in the **material instance**
   (`game_bootstrap.cpp:7851-7852`) held **only bits 11 and 13** when this sheet was written; it now also carries bit 1 (wind,
   W4 FW-6) and bit 20 (I-0113 tranche 13), `18_resource_admission.inc`. Everything the original engine code still
   reads from the instance (`model+0x224` aggregate, per-mesh `pose_policy`, the predicates) sees only those bits.
4. **Concrete defects found (section 5).** D1: bit 14 (ClothMatching, 121 MTDs, 5,790 part materials) never reaches the
   pose initialisation (`InitializeNativeModelPose` reads `model+0x224` bit 14) nor the per-mesh pose policy. D2: bit 7
   stencil. D3: the native decal-capable predicate (`NativeModelShaderTagPredicate`) tests aggregate bits 2/3 that are
   never produced. D4: wind bit 1 (known, W1). D5: bits 26 and 27 are excluded although their name tables are static.
   D6: the declaration check of `A7E40` is not reproduced where the instance word is built. D7: `+0x1C` (cloth 2 versus
   1) and `+0x18`, `+0x21` stay at constructor defaults (harmless today, section 3).
5. **No MTD sets bit 17 (water interaction).** The builtin name `GXFT_WaterInteraction` appears in no MTD of the 512 of
   `AllMaterialBnd`. Bit 17 is dead in this build; its readers (`FUN_140061B00`, `FUN_140061B90`) always return 0.
6. **Eclipse (bit 19) is carried by three MTDs** (`Eclipse_cloth`, `Eclipse_grass`, `GrassTest`), none used by any
   scanned FLVER material; **wind (bit 1) by eight MTDs, 826 map and 19 object materials.**
7. **Fixes (section 6):** F1 publish bits 14 and 7 (and 1 for W1) into the instance word with one shared function;
   F2 tie `pose_policy` to the same word; F3 keep bits 2/3 out of the instance until the paint-object constructors are understood
   (publishing them turns the decal predicate true and makes the entity and item constructors create the paint objects); F4 parity test over
   the 512 MTDs; F5 add the static name tables (bits 26/27) when a native reader needs them; F6 first-read observers on the original readers that still run.

## 1. How the word is made (STATIC)

| Step | Function | What it does |
|---|---|---|
| 1 | `FUN_1400A7E40(reader, name, param, heap, ...)` (slot 1 of `GXSR::GXFlverMaterialParamSolver`, vtable `143D48268`; encrypted at rest, decrypted copy retained in `evidence/I-0066/artifacts/direct-material-builtin-754/DS3-1152-F-000A7E40.c`; the role of `FUN_1401FEEC0` comes from the loaded-code reads of DIRECT-MATERIAL-PREPARATION, receipt 744, because `1FEEC0` is itself encrypted in the static image) | First `FUN_1401FEEC0`: a parameter that the shader's CPU declaration owner declares is handled there and **returns without touching the word**. Otherwise: the name equal to `g_BloodAmount` (14-unit compare against `143D48660`) sets `reader+0x3C \|= 4`; then the CRC32 of the name selects a row of the 65-row builtin table (`143D20580`, stride 0x40, kind at `+8`, names in `evidence/I-0068/queries/glow/builtin-material-parameters.tsv`) and a `switch(kind)` runs. The bit written is **the low bit of the parameter's first value** (`uStack_d4 & 1`), so a `False` clears it |
| 2 | `FUN_14002DD60(gxMtd, mtdBytes, ..., reader)` (called from `FUN_140D43BB0`) | Feeds every MTD parameter to step 1, then copies the five dwords `reader+0x2C..+0x3C` to `gxMtd+0x78..+0x88` (so `gxMtd+0x88 = reader+0x3C`, `gxMtd+0x80/+0x81 = reader+0x34/+0x35`). Then ORs the **name bits** (`14002E2EA..14002E4F7`, section 2.2). It is the only writer of `gxMtd+0x88` found in the GX range (scan `scan/b88.txt` matches the displacement only, not the object type, so "only" is INFERRED) |
| 3 | `FUN_140140F90(material, gxMtd)` (called by `FUN_140082220`; `FUN_140140F10` and `FUN_140082220:236` repeat only the bit-28 update) | `+0x22 = (gxMtd+0x88 >> 13) & 1`; `+0x24 = gxMtd+0x88` with **bit 28 replaced** by `material+0x70 ? [material+0x70]->vt+0x38() & 1 : 0`; `+0x21 = gxMtd+0x111`; `+0x1C = (gxMtd+0x88 & 0x2000) ? 2 : 1`; `+0x18 = FUN_140142130(material)` (class); `+0x80 = gxMtd` (reference counted) |
| 4 | `FUN_14002A120(container)` (from `FUN_140029FB0`, called by the model load `FUN_1400A0170` after all materials exist; container = `model+0x1F0`, vector at `model+0x1F8..+0x200`) | `model+0x220 = OR(material+0x18)`; `model+0x224 = OR(material+0x24)` with bit 28 replaced by the AND over materials; `model+0x228 + 4*k` (k = 0 to 8) = the same OR/AND restricted to materials whose class `+0x18` has bit k |
| 5 | `FUN_140D35F80` (model load, `140D364C1..140D36525`) | Caches three predicates in `model+0x468` (section 4) and `model+0x46C` |

Bit 28 is `[programSet + 0x3EF8]`: slot 0x38 of the set vtable `143D48C18` is `MOVZX EAX, byte ptr [RCX+0x3EF8]` (`1400A9AF0`);
the byte is cleared by `FUN_1400A9BA0` and set by `FUN_1400AA4D0`, `FUN_1400A9D40`, `FUN_140082E50` (STATIC). INFERRED
meaning: "the set's programmes are all resolved".

Caveats on the rule, all STATIC unless marked:

- **Declaration shadowing.** Step 1 skips every parameter that the SPX declares. `g_BloodAmount` is a shader constant
  (`FC_BloodAmount` in `cbMtdParam`) of at least 12 of the 15 Bloody families that the reflection table lists
  (`evidence/I-0068/queries/npc-families/fam-reflect.tsv`): for those, bit 2 comes **only** from `GXFT_PaintDecal_Mat`
  (INFERRED mapping declaration to constant). The census below ignores declarations and is therefore an **upper bound**
  for bit 2; for the other builtin names the cloth rule's exactness over the 512 MTDs (MENUS-MAP 66) shows no shadowing.
- **Order dependence.** Kind 29 (`PaintDecal_Mat`) clears then sets bit 2 by value, so a `False` after `g_BloodAmount` clears it.
  No MTD in the data does that (only `True` occurs), so order does not matter in this build.
- **Bool and Int only.** Every bit-bearing parameter is a Bool (`True`/`False`) or, for the material type, an Int.
- **Declaration shadowing is only established for `g_BloodAmount`.** The cloth rule's exactness shows `g_GX_MaterialType` and `GXFT_Cloth`
  are not shadowed; nothing shows it for `GXFT_ClothMatching`, `SnowSurface`, `WindEffect` and the others (they are not shader constants, which
  makes shadowing unlikely: INFERRED). The GX families' declaration lists were not extracted (OPEN 4).
- **MTD side effects that are not bits.** Kinds 1 to 15 (except 8, which has no case) build parameter objects; kind 0x11 (`ValidSubPhantom`) and
  0x12 (`ValidWet`) build objects `FUN_1402002B0` / `FUN_140200550` whatever the value (no word bit; 103 and 198 MTDs carry them, of which 36 are `True` for `ValidSubPhantom`); kind 0x29
  (`g_WaterInteractionSpeedParam`) builds an object; kinds 0x13/0x14 (`g_GX_bDisableDecal`, `g_GX_bDisableDepthWrite`) store the
  value in bytes `reader+0x34/+0x35`; kind 0x10 also stores `reader+0x28 = g_GX_MaterialType`.

## 2. The bits

Census source: `Census-MtdBits.ps1` over the 512 MTDs (508 with parameters in `evidence/I-0068/queries/npc-families/all-mtd.tsv`,
4 without), result in `evidence/I-0066/queries/mtd-bits/mtd-policy-bits.tsv`, per-bit summary `bit-summary.tsv`, usage by domain
`bit-usage.tsv` (FLVER materials of the I-0063 scan, ~48,900 rows). "MTDs / families" counts MTDs and distinct shader names.
"Native" cites the two native states: **policy** = `ModelMaterialPolicy.flags`, **instance** = `material+0x24`.

### 2.0 Which copy a reader sees (STATIC, review-corrected)

Four copies of the word exist: `reader+0x3C`, `gxMtd+0x88`, `material+0x24` and the aggregate `model+0x224`. Readers split as follows, and
**publishing a bit into `material+0x24` (F1) only reaches the second group**:

- **Read `gxMtd+0x88` through `material+0x80`** (null on a native material, and not fed by F1): `FUN_140142130` (bits 15, 21 and the bytes
  `+0x110/+0x111`), `FUN_140141070` (bits 0, 9, 11, 26, 27), `FUN_140082220:284` (bit 26, only on the path where the set lookup failed),
  `FUN_140060700:229/474` (bits 0, 10), `FUN_140140F90` itself (bit 13). The vtable-slot-1/2 and `140850` readers of cloth use the byte `material+0x22`.
- **Read `material+0x24`**: `FUN_1400619B0`, `61A20`, `61A90`, `61B00`, `61B90`, `FUN_1401407D0`, `FUN_140140680`, `FUN_140140850` (bits 2, 7, 11),
  `FUN_140064340` (bit 14), `FUN_14002A120` (aggregator).
- **Read `model+0x224`**: the list of section 4.

In the tables below, a reader is marked **[gx]** when it reads the `gxMtd` copy.

### 2.1 Builtin-parameter bits (`A7E40`)

| Bit | Mask | Parameter (kind) | MTDs / families | Readers in the original (STATIC) | Expected visible effect | Native |
|---|---|---|---|---|---|---|
| 1 | 0x2 | `g_GXFT_WindEffect`, `GXFT_WindEffect` (26) | 8 / 6: `GXFlver_Grass`, `GrassSpcBump`, `GrassDOL`, `GrassSpcBumpDOL`, `Eclipse_grass`, `GrassTest`; 826 map materials (17 maps) + 19 object materials | `FUN_1400619B0` (callers `FUN_140D05C80` entity ctor, `FUN_140D35F80` model load into `model+0x468` bit 1, `FUN_140D3C4D0` item ctor): creates the wind node. No other reader (scan) | grass and foliage sway (WIND sheet) | policy yes; instance yes since W4 FW-6 (once the effect system is published) |
| 2 | 0x4 | `g_BloodAmount` (name, undeclared families) and `GXFT_PaintDecal_Mat` (29) | 238 / 53 (upper bound, the name part being shadowed where declared; **118 MTDs carry `PaintDecal_Mat`**); chr 1,577, parts 8,012, obj 419, map 37 materials | `FUN_1401407D0` / `FUN_140140680` (slots 1, 2): **a material without bit 2 is skipped in shader passes 8 and 9** (`Pnt`, `PntCube`, table `14473CE60`, STATIC); `FUN_140057A20:252` (a default pointer taken when the row has none: INFERRED decal parameters); aggregate bits 2/3 feed `FUN_1400600A0` (model+0x468 bit 0). That predicate gates, in the entity constructor `FUN_140D05C80:139`, the item constructor `FUN_140D3C4D0:147` and `FUN_140CFDBA0`, the creation of a paint object through `FUN_140E000E0` (a 29-byte tail jump to `FUN_140051F80`, whose only code reference it is) and `FUN_140E000F0` (square texture), stored at `entity+0xA60/+0xA68` or `item+0xE8/+0xE9`; `FUN_14005E990` stores bit 2 as a byte (`+0x78`) and, for bit 3 (`+0x79`), allocates two small objects; its 1024x1024 target needs a fourth argument that both constructors pass as 0, so it is **not** created there (STATIC) | the gate on passes 8 and 9: INFERRED either point-light shadow (`Pnt`, `PntCube`) or paint accumulation (the names do not decide; bit 7 adds pass entries 10 `PntCol` and 11 `SO`, which favours "paint"); models with the bit get the paint object | policy yes; instance **no** (D3); no native `Pnt` pass exists |
| 3 | 0x8 | `GXFT_PaintDecal_Disp` (30) | 3 / 1 (`GXFlver_ColDifSpcBumpRNM_DeepSnow`); 0 scanned FLVER materials | same as bit 2 (aggregate 0xC predicate; `FUN_14005E990` third object) | INFERRED: displaced decal (snow footprint) | policy yes; instance no |
| 4 | 0x10 | `g_GXFT_PlanarReflection`, `GXFT_PlanarReflection` (25) | 4 / 2 (`GXFlver_WaterBumpTri` x3, `GXFlver_Water`); 58 map + 2 object materials, 11 maps | `FUN_1400575C0`: for view type 0x45, tracks the nearest reflection plane (`view+0x891`, `+0x894`, plane at `+0x8A0..+0x8AC`) and sets the draw kind byte to **0x27**; for views 1 to 0x38 and 0x39, kind 0x27 | planar reflection of water surfaces; the kind-0x27 draw is the water row of WATER-NATIVE (this is how a draw becomes kind 0x27) | policy yes (water rows absent natively); instance no |
| 5 | 0x20 | `g_GXFT_Refraction`, `GXFT_Refraction` (27) | same 4 MTDs | `FUN_1400575C0`: `view+0x892 = 1`, kind 0x27 | refraction capture requested by the view | as bit 4 |
| 6 | 0x40 | `g_GXFT_WaterDepth`, `GXFT_WaterDepth` (23) | same 4 MTDs | none found (scan of every `[reg+0x24]`, `[reg+0x88]`, `[reg+0x224]` form) | INFERRED: selects the `WatDpt` programme family through the shader pass table only | policy yes |
| 7 | 0x80 | `GXFT_SnowSurface` (31) | 3 / 1 (`..RNM_DeepSnow`); 0 scanned materials | `FUN_1400575C0`: `view+0x890 = 1`; `FUN_140057A20:504/610`: extra pass entries 10 and 11 when the global `DAT_144731BD1` and `view+0x596` are set; `FUN_1401407D0`/`140680`/`140140850`: G-buffer stencil value `\|= 2` (SSS bit 11 takes precedence, `\|= 4`) | snow surface response | policy yes; instance **no**, so the native stencil (`game_bootstrap.cpp:4881`) never sees it (D2) |
| 8 | 0x100 | `GXFT_DeepSnow` (32) | 3 / 1; 0 scanned materials | none found | INFERRED: shader-side only | policy yes |
| 9 | 0x200 | `g_GXFT_FrozenEffect`, `GXFT_FrozenEffect` (28) | 6 / 2 (`..RNM_DeepSnow` x3, `..RNM_Burn` x3); 0 scanned materials | `FUN_140141070` **[gx]** (GX item parser, after the item list): `material+0xE5 = 1`, constant rows >= 11. No model-level reader | frozen / burn response (INFERRED) | policy yes; parser not run natively (C2) |
| 11 | 0x800 | `g_GX_MaterialType` = 3 (16) | 38 / 10 (`SAT_Equip_SSS`, `GXFlver_SSSIbl_Bloody`, `SkinSSSIbl`, `SSSLightmap_lightmap`, ...); chr 77, parts 918, obj 10, map 3 | `FUN_140140850` (`BT 0xB`), `FUN_1401407D0`/`140680` (stencil `\|= 4`, writes `draw+0x20`), `FUN_140141070` **[gx]** (`FUN_14022C0D0`: SSS width), `FUN_140057A20:430` (calls `vt+0x48` on every material of a model with the bit) | subsurface skin class in the G-buffer stencil | **both** (instance `0x800`, `game_bootstrap.cpp:7852`; stencil `:4881`) |
| 12 | 0x1000 | `g_GX_ForwardRendering`, `GXFT_ForceForward` (21) and the name `GXFlver_Col` | 53 / 15 (`GXFlver_Col` x36, `Veill_NoLight`, `SAT_HairIbl2`, `*_nolight`, `Eclipse_cloth`, `GrassTest`); chr 6, map 216, obj 108, parts 1,018 materials | `FUN_14002DD60` byte `gxMtd+0x110`; `FUN_140142130` **[gx]** (class 0x80 or 0x10, via `gxMtd+0x110`); `FUN_140057A20:223/517` (forward row 0x21 decision, extra rows 4 and 0x1F for a model with the bit) | forward-only (unlit or alpha-tested hair) materials | policy yes (`policy.forward`, row 0x1F `game_bootstrap.cpp:4436`); instance no (no instance reader) |
| 13 | 0x2000 | `g_GX_MaterialType` = 2 and `GXFT_Cloth` (33) | 108 / 25 (equals the cloth rule of MENUS-MAP 66); chr 278, parts 1,164, obj 64, map 11 | `FUN_140140F90` **[gx]** (`+0x22`, `+0x1C`); `FUN_140140850` and `FUN_140064340` read the byte `material+0x22` (VC_isEnableCloth, cloth buffer route) | cloth simulation route | **both** |
| 14 | 0x4000 | `GXFT_ClothMatching` (34) | 121 / 24 (`SAT_Equip*`, `SAT_HairIbl2`, `C_ARSN_cinder*`, `..Bloody_Cm_Cloth`); parts **5,790**, chr 148, obj 33 materials | `FUN_140064340` (material bit: auxiliary + cancel palette, per the comment in `model_pose_binding.h`); `FUN_140015490` (model aggregate: pose initialisation, `+0x74`/`+0x75`); `FUN_14005C0E0:127` (a bit-14 model cannot take an optional draw path: INFERRED instancing) | cloth-matched garments follow the cloth pose | policy yes; instance **no** (D1) |
| 15 | 0x8000 | `GXFT_Emissive` (35) and the name `Emi` | 174 / 46; chr 198, parts 5,044, obj 75, map 237 | `FUN_140142130` **[gx]** (class 0x100); `FUN_140057A20:517/651` (extra rows 7 and 0x22) | emissive pass | policy yes (rows 7 and 0x22 read it, `:4424-4448`); instance no |
| 17 | 0x20000 | `g_GXFT_WaterInteraction`, `GXFT_WaterInteraction` (22) | **0 MTDs** | `FUN_140061B00`, `FUN_140061B90` (callers `FUN_140D35F80`, `FUN_140D3C4D0`) | none in this build | policy yes (never set) |
| 18 | 0x40000 | `GXFT_WaterApplyDynamicLight` (24) | 1 / 1 (`m[arsn]_water_dl`; three other MTDs carry it `False`); 41 map materials in 8 maps | none found | INFERRED: dynamic-light lighting on that water | policy yes |
| 19 | 0x80000 | `g_GXFT_Eclipse` (37) | 3 / 3 (`Eclipse_cloth`, `Eclipse_grass`, `GrassTest`); 0 scanned materials | `FUN_140061A90` (callers `FUN_140D3C4D0`, `FUN_140DD3CA0`): FX node kind `0x2_0000_0000` instead of 2/4 | eclipse-time vegetation/cloth | policy yes; instance no (moot, WIND sheet) |
| 20 | 0x100000 | `GXFT_InvalidLighting` (38) | 13 / 3 (`GXFlver_Col`, `GXFlver_ColDif`, `GXFlver_ColZFeather`: unlit and particle MTDs); map 298, obj 136 materials | `FUN_140061A20` (only caller `FUN_14027E5E0`, the factory of the 0x140-byte GX object of chunk type `0x25D`): stores the result in `object+0x129` (the scan's `14068AC00`, `shr20` of a `[RCX+0x24]` in `scan/b24-fns.txt`, takes `(+0x24 >> 20) & 0xFF` as a table index of stride 0x70 for an object whose `+0x28` holds flags: a packed 8-bit field, not a test of bit 20; STATIC, I-0113 tranche 13) | 27EA70 multiplies the model particle's colour by the draw context's `+0xAA8` (0.652) when `+0x129` is set (I-0113 tranche 13, STATIC and witnessed) | policy yes; instance **yes** since I-0113 tranche 13 |
| 21 | 0x200000 | `GXFT_Recursive` (39) | 2 / 1 (`RecursiveCrystal`: `C[Crystal]`, `M[Crystal]`); chr 5 materials | `FUN_140142130` **[gx]** (class 2, STATIC); `FUN_14005C0E0:141` (same optional-path gate as bit 14, INFERRED); `FUN_140059220:65`: for descriptor kinds 0x21 and 0x29 it issues a scene copy (command 0x2D, INFERRED) before the draw | crystal shows the scene behind it (NPC-FAMILIES-NATIVE C4) | policy yes (class 2); the copy is open |
| 22 | 0x400000 | `g_SATBlendShader` (40) | 64 / 40 (`MultiBlend*_et1*`, `SAT_MultiBlend2_*`); map 4,096, obj 194, chr 9, parts 92 | none found | INFERRED: shader-side only | policy yes |

### 2.2 Name bits (`FUN_14002DD60`, on the shader name `gxMtd+0x48`)

| Bit | Rule (STATIC) | MTDs | Readers / effect | Native |
|---|---|---|---|---|
| 0 | `RNM` or `DOL` in the name, or the name is exactly `GXFlver_Col`, or it ends in `GI` (first `GI` followed by NUL) | 64 / 17 (`GXFlver_Col` x36, `..DOL*`, `..RNM_*`, `..GI`); map 26,925, obj 767 materials | `FUN_140141070` **[gx]** (`+0xE0 = 1`, `+0x108 = 0`, rows >= 1 when the item list has no GX05; natively `BuildModelMaterial` reproduces only the GX03, GX05, GX09 items, `game_bootstrap.cpp:7857-7865`, so a bit-0 material without GX05 keeps `+0xE0 = 0`: effect on the native `cbMatDynParam` producers OPEN); `FUN_140060700:227` **[gx]** (model-load tagging, OPEN); with bit 10: `FUN_140057A20:578`, `FUN_14005B3D0:255`, `FUN_1401A9AD0:25` (light-probe/lightmap binding branch) | policy yes (`:464-469`); no native reader of the bit itself (the `141070` side effects above are the open part) |
| 10 | `DOL` and not `RNM`; also `g_GXFT_DominantLightMap` (36) | 45 / 33 (`*_lightmap`, `..DOL*`); **map 30,763, obj 750** | same readers as bit 0 plus `FUN_140060700:472` **[gx]** | policy yes; no native reader (see OPEN 1) |
| 12 | `GXFlver_Col` when the shader name is shorter than 12 characters (also sets `gxMtd+0x110` and bit 0) | 36 `GXFlver_Col` MTDs (rest of the 53 come from the parameter) | see 2.1 | policy yes (`:467-468`) |
| 15 | `Emi` anywhere in the name | with `GXFT_Emissive`, 174 | see 2.1 | policy yes |
| 16 | `Glow` anywhere in the name | 20 / 7 (`..EmiIblGlow*`, `..EmiDOLGlow`, `SSSEmiIblGlow`); chr 36, map 6, obj 7 materials | none found | policy yes, no reader |
| 23, 24, 25 | `MultiBlend4`, `MultiBlend3`, `MultiBlend2` | 18, 18, 35 MTDs; map 288 / 1,051 / 2,860 materials | none found | policy yes, no reader |
| 26 | name equals one of the **43 shader names** at `1447371E0` (stride 0x200, static `.data`; list in `evidence/I-0066/queries/mtd-bits/name-tables.tsv`) | 132 / 29; map 2,669, chr 1,429, obj 2,064, parts 2,258, sfx 8 materials | `FUN_140082220:284` **[gx]** (alternate programme set `material+0x78`, `_Phantom_Fwd`; only on the path where the set lookup failed); `FUN_140141070:507` **[gx]** (`+0xF2 = 1`, `+0x140..+0x19F` cleared, `+0x170..+0x188` constants, rows >= 30) | **excluded** by `known_flags_mask` (D5); native picks the phantom programme by file existence |
| 27 | name equals `GXFlver_SSSEmiIblGlow_Bloody_Cloth` or one of the 43 names at `144731BE0` (the same 43 names as the first table, a second array) | 132 / 29 | `FUN_140141070:426` **[gx]** (`+0xF7 = 1`, rows >= 38); second trigger: the third argument's resource flag bit 1, parallel to bit 26's flag bit 0 | excluded (D5) |

Both tables hold 43 slots but **42 distinct names** (`GXFlver_ColDifSpcBumpEmiIblGlow_Bloody` sits at `144737DE0` and `144739FE0`), and the comparison is `FUN_141FD30A0` = `_wcsicmp` (case-insensitive; F5 must compare that way). The two tables are **static initialised data** (no write reference exists; `o:`/`w:` queries only find the `LEA`s in
`FUN_14002DD60`), so the native comment "runtime shader catalog flags" (test `game_model_preparation_test.cpp:363`) does
not apply to them.

Alternate-set cross-check (STATIC): 64 families have a `_Phantom_Fwd.ppo` programme; **27 of them are not in the 43-name table**
(`SAT_Equip*`, `SAT_HairIbl2*`, `Dragon_FDP*`, `Fur_FDP*`, `SAT_MultiBlend2_*`, three Bloody variants and `GXFlver_ColDifSpcBumpIbl_Df[_Cloth]`), so for them the alternate
set exists through the other trigger (the bundle record resource flag bit 0, MENUS-MAP 67), not bit 26. Five names of the table have
no phantom programme. The two sets only **overlap**: the native rule "the programme exists" is neither a subset nor a superset of bit 26 (it also covers the resource-flag families, and it draws nothing for the five table names that have no programme, which is what the original does too if the lookup returns null: OPEN).

### 2.3 Bit 28 and the unused bits

Bit 28 (`0x10000000`): program set ready (section 1). Readers: `FUN_14005D5C0:119` (`~model+0x224 >> 28`: disables the
batched path when any material lacks it), `FUN_14002A120` (AND). Native: the material has no `+0x70` set, so the bit is 0
and the aggregate AND is 0; no native reader. Bits 29 to 31: no writer, no reader.

## 3. Fields that travel with the word and the native defaults (STATIC)

| Field | Original value | Native | Consequence |
|---|---|---|---|
| `material+0x22` | bit 13 | written (`game_bootstrap.cpp:7851`) | none |
| `material+0x1C` | 2 when cloth, else 1 | constructor default 1 (`FUN_140140290`) | `secondary_mask` in `model_pass.cpp:66` is 0xFF, so 1 and 2 both pass: harmless |
| `material+0x18` (class) | `FUN_140142130` | constructor default 0xFFFF; the class is computed natively (`ClassifyModelMaterial`) | `model+0x220` aggregate stays 0xFFFF (documented in NPC-FAMILIES-STATIC 2.1) |
| `material+0x21` | `gxMtd+0x111` | 0 | no reader of `material+0x21` itself was found; `FUN_140142130`, `FUN_1401407D0`, `FUN_140140680`, `FUN_140140850` read `gxMtd+0x111` |
| `material+0x80` (`gxMtd`) | the MTD object | not set (null) | every **[gx]** reader of section 2.0 sees null; INFERRED: the `vt+0x38` accessor returns it, which is why `NativeModelShaderTagPredicate` reads the authored shader name instead |
| `gxMtd+0x80/+0x81` | `g_GX_bDisableDecal`, `g_GX_bDisableDepthWrite` | policy fields `disable_decal` (no native reader), `disable_depth_write` (read, `game_bootstrap.cpp:4892`) | decal disable is not honoured natively |

## 4. `model+0x220`, `+0x224` and `+0x468`

`model+0x224` readers (scan `scan/b224.txt`, STATIC): `FUN_140015490` (bit 14 x3), `FUN_140051F80` (bits 2, 3; reached only through the 29-byte tail jump `FUN_140E000E0`, which the scan therefore also misses as an inlined copy),
`FUN_1400575C0` (bits 4, 5, 7), `FUN_140057A20` (bits 12, 2, 11, 7, 10, 0, 15), `FUN_140059220` (21), `FUN_14005B3D0` and
`FUN_1401A9AD0` (10, 0), `FUN_14005C0E0` (14, 21), `FUN_14005D5C0` (28), `FUN_1400600A0` (2, 3), copies in `FUN_1400B4530`
and `FUN_1400B5210`. Native replaces `FUN_140015490` (`InitializeNativeModelPose`, hook `0x15490`) and `FUN_1400600A0`
(`NativeModelShaderTagPredicate`, hook `0x600A0`) and reads `model+0x224` itself for rows 7, 0x1F and 0x22 as a fallback
(`game_bootstrap.cpp:4424-4448`, primary source `model.material_flags` = union of the policy words). The original scene
scheduling readers (`57A20`, `575C0`, `5C0E0`, `59220`, `5B3D0`, `5D5C0`, `1A9AD0`, `140850`, `1407D0`, `140680`) are
INFERRED not to run natively: the scene root `0x23410` is replaced and no draw is submitted by the original passes.

`model+0x468` (written by `FUN_140D35F80` after `FUN_14009FF80`; reset by the model constructor `FUN_140D35AF0`):

| Bit | Source | Reader | Effect |
|---|---|---|---|
| 0 | `FUN_1400600A0`: `model+0x224` bit 2 or 3, **or** any material whose shader name has an `_`-separated token starting `Bloody` (6 chars) or `DeepSnow` (8 chars) (`143D41E90`, `143D41E78`) | `FUN_140C3F6A0` (`[[x+0x10]+0x58]+0x468 & 1`); in `FUN_140C3CA10:256` a ChrAsm slot with the bit and a non-zero size field creates a per-slot paint target (`FUN_140CEDE20`, `+0x1A8`) | blood/paint capable part |
| 1 | `FUN_1400619B0` (material bit 1) | none found | cached wind flag |
| 2 | `FUN_140061B00` (material bit 17) | test at `140D36514` only; then `model+0x46C = FUN_140061B90` (maximum of `gxMtd+0x84` over materials with bit 17) | water interaction; `+0x46C` has no reader found; dead (no MTD sets bit 17) |

Native state: `NativeModelShaderTagPredicate` keeps the name-token half and the aggregate half. Because the instance word
never carries bits 2/3 (D3), the aggregate half is always false. The census finds **119 MTDs whose bit 2 or 3 is set and whose shader name has no
`Bloody`/`DeepSnow` token** (`SAT_Equip*` armour and weapons, `SAT_HairIbl2*`, `SAT_MultiBlend2_*`, `GXFlver_Equip`): **116 of them carry
`GXFT_PaintDecal_Mat`** (not shadowable by a declaration), 3 only `g_BloodAmount` (`p_wp[arsn]*`). For models made only of such materials the
original predicate returns 1 and the native one returns 0, so natively (a) the ChrAsm paint target of `FUN_140C3CA10:256` is not created and (b) **the paint object that the entity constructor `FUN_140D05C80:139` and the item constructor `FUN_140D3C4D0:147` create through `FUN_140E000E0` / `FUN_140E000F0` is not created** for them. The families concerned are not only armour and weapons: `C_ARSN_cinder*` (13 MTDs), `RecursiveCrystal`, `c5280LastBoss_em*`, `Fur_FDP`, `Dragon_FDP` and several `C_ARSN_*` character families are in the 119 (INFERRED from the census listing). Whether a native consumer needs that object is OPEN 3.

## 5. Native today: divergences

| Id | Divergence | Evidence | Consequence |
|---|---|---|---|
| D1 | Bit 14 absent from the instance word | `game_bootstrap.cpp:2166-2172` (`auxiliary_present = flags & 0x4000`, `fast` requires no bit 14) and `:3614`, `:4817`, `:4824`, `:4831` (`force_auxiliary = pose_policy.flags & 0x4000`) both read it | ClothMatching meshes bind the current palette instead of auxiliary + cancel (`FUN_140064340`); the pose initialisation may take the fast path for a single-bone gated model; only parts with `model_policy_flags` (`:2209`) get an auxiliary palette |
| D2 | Bit 7 absent | `:4881` reads `pose_policy.flags & 0x80` | stencil `\|= 2` never set for snow-surface materials (3 MTDs, no scanned FLVER use) |
| D3 | Bits 2/3 absent | `NativeModelShaderTagPredicate` tests `flags & 0xC` | decal-capable predicate false for parameter-only models (section 4) |
| D4 | Bit 1 absent | WIND sheet W1 | no wind nodes |
| D5 | Bits 26/27 not produced | `model_parameters.h:171` `known_flags_mask = 0xF3FFFFFF`, test `:363` | alternate-set trigger replaced by file existence (the sets overlap, section 2.2) |
| D6 | The instance word is derived by name in `BuildModelMaterial` (`:7842-7850`) without the SPX declaration filter and without the other rules | `PrepareModelMaterialPolicy` applies the filter later (`:3316`) and can fail (parameter type >= 12, non-ASCII name, kinds 5, 8, 13 or > 41, `unsupported_parameters` non-empty, `:3310`) | two code paths for one rule; F1 unifies the rule but **does not remove the declaration gap** (OPEN 4) |
| D7 | Fields `+0x1C`, `+0x18`, `+0x21`, `+0x80` | section 3 | harmless today |

## 6. Fixes, implementation order, tests

### F1. One shared word function, published into the instance word (D1, D2, D4; D6 only partly)

- **Where:** `native/content/model_parameters.cpp` (+ header) and `native/frame/game_bootstrap.cpp:7842-7852`.
- **What:** split `PrepareModelMaterialPolicy` so that its flag part is a pure function
  `std::optional<uint32_t> MaterialPolicyWord(std::wstring_view shader_family, std::span<const ModelBuiltinParameterDeclaration> builtins, std::span<const ModelNumericParameter> values, std::span<const ModelParameterDeclaration> declarations = {})` (an empty result keeps the failure returns of `PrepareModelMaterialPolicy`; the instance then keeps the old two-bit derivation)
  (same code as `model_parameters.cpp:437-474`; `known_flags_mask` stays as the publication mask). `PrepareModelMaterialPolicy`
  calls it and keeps the classification fields. In `BuildModelMaterial` replace the ad-hoc `cloth`/`sss` loop by
  `word = MaterialPolicyWord(family, NativeBuiltinModelParameters(), owned->authored->numeric_parameters)` (the numeric list is filled at `:7612`,
  before the instance code; `family` = file name of `owned->authored->shader` without extension, as at `:3311-3313`), then
  `instance+0x22 = (word >> 13) & 1` and `instance+0x24 = (instance+0x24 & 0x10000000) | (word & PUBLISH_MASK)`.
- **PUBLISH_MASK, staged** (the instance word is read by the original aggregator `14002A120` at model load, so it must be complete before the
  last material of the model is built, which holds because the aggregator runs after the factory loop):
  - **S1 (immediate): 0x800, 0x2000 (as today), plus 0x4000 and 0x80.** Their original readers are either replaced by native code
    (`15490`) or do not run natively; the native readers already exist (`:2166`, `:4817-4833`, `:4881`).
  - **S2: 0x2** (wind) together with W1 of the WIND sheet; the readers run natively (`FUN_1400619B0` in `FUN_140D05C80`, `FUN_140D3C4D0`, `FUN_140D35F80`).
  - **S3 (not now): 0x4, 0x8.** Publishing them makes `FUN_1400600A0` return 1 for the 119 models of section 4, so the entity constructor
    (`FUN_140D05C80:139`) and the item constructor (`FUN_140D3C4D0:147`) would run `FUN_140E000E0` (`FUN_140051F80` / `FUN_14005E990`: small CPU objects, no 1024x1024 target because the fourth argument is 0)
    and `FUN_140E000F0` (a square texture, device use not read: OPEN 3). Also the bit-2 part of the rule depends on the SPX declaration (section 1), unavailable when the instance is built.
    Until OPEN 3 is closed, leave them out; the name-token predicate already covers the Bloody families.
  - **S4 (fidelity only, after F6 shows no original reader reaches them):** 0x10..0x40, 0x100, 0x200, 0x20000..0x400000 (but 0x100000, published since I-0113 tranche 13), name bits.
- **Hazard of S1 (bit 14), review-corrected:** a ClothMatching mesh asks for the auxiliary palette (`SelectOrdinaryModelPoseBinding(.., force_auxiliary=true)`).
  There is **no fallback**: `:4822-4824` retries only when `cloth_enabled` and passes `force_auxiliary` again, so a model without an auxiliary palette has its mesh refused (gate rows).
  `model_policy_flags` (`:2209-2210`) only sets `palette_inputs.auxiliary_present`; the reference rows are read only when `owner.auxiliary_present` (`:2215`, set at init from `model+0x224`, `:2168`);
  for a composed (cloth-gated) model with a non-null reference the reference stays Unknown and `ProduceModelPose` (`model_pose.cpp:83-107`) leaves the auxiliary palette unset, so the binding is Unavailable (`model_pose_binding.h:22-26`).
  Extending `model_policy_flags` therefore does nothing once `model+0x224` carries bit 14 (redundant, drop it). Further risks: a failed `Read(reference)` or a singular inverse in the cloth palette; and `prefix.fast` (`:2172`) flips for
  single-bone gated models, which changes the original update route the engine takes. **Gate S1 on witness WB-6 first; rollback:** clear bit 14 from PUBLISH_MASK; no other state changes.

### F2. `pose_policy` from the same source

`game_bootstrap.cpp:3614` reads `mesh.material+0x24`. After F1 this is correct; add a debug assertion (non-shipping) that
`pose_policy.flags == (mesh.programs->policy->flags & PUBLISH_MASK)` when both exist, logged once per mesh (`native_material_word_mismatch`). It will legitimately fire where an SPX declaration shadows a published builtin (D6) or where the policy is absent; treat those rows as data, not as failures.

### F3. Bits 2 and 3

No instance change. Keep `NativeModelShaderTagPredicate` as is; add the missing half from the authored parameters (`g_BloodAmount` undeclared or
`GXFT_PaintDecal_Mat`/`_Disp` true) only if a native decal-target consumer is introduced. Until then the `FUN_140C3CA10:256` paint target and the constructors' paint object (`FUN_140E000E0`) are created
exactly for the Bloody/DeepSnow-token families, as today.

### F4. Parity test (host, no game)

Add to `game_model_preparation_test.cpp` a table generated from `mtd-policy-bits.tsv` (512 rows: MTD, shader family, authored builtin values, expected word) and
`Require((MaterialPolicyWord(...).value() & ~0x1C000000u) == (expected & ~0x1C000000u))` (bits 26 to 28 masked on both sides until F5; the table must carry the authored builtin values from `all-mtd.tsv`, which `mtd-policy-bits.tsv` does not). Expected word spot checks from the census:
`m[arsn]_water.mtd` (`GXFlver_WaterBumpTri`) `0x70` (bits 4, 5, 6); `m[arsn]_water_dl.mtd` `0x40070`; `m[arsn]_e_grass.mtd` `0x2`;
`m[arsn]_l_e_grass.mtd` `0x403`; `m[arsn]_l.mtd` `0x401`; `p_lg[arsn].mtd` (`SAT_Equip`) `0xC004`; `c[arsn]_sss.mtd` `0x0C000804`;
`c[crystal].mtd` `0x208004`; `m[a]_nolight.mtd` `0x101001`; `eclipse_grass.mtd` `0x80002`; `m[burn]_l.mtd` `0x201`; `p[chrcustom_hair].mtd` `0x5004`.
A second test runs with the SPX declarations of the families that declare `g_BloodAmount` and expects bit 2 only where `PaintDecal_Mat` is true; it needs the GX families' declaration lists, which are not extracted (OPEN 4), so until then it only covers the three `p_wp[arsn]*` MTDs with hand-written declarations.

### F5. Static name tables (bits 26/27)

When a native reader needs them (the `_Phantom_Fwd` trigger or the `141070` defaults `+0xF2/+0xF7`): embed the 43 names
(`name-tables.tsv`), remove 0x0C000000 from `known_flags_mask`, change the test at `:363` to check the list. Compare names with `_wcsicmp` semantics (ASCII case folding).

### F6. First-read observers (read-only)

On the original readers that can still run natively, log the first read per function: `FUN_140061A20` (caller `FUN_14027E5E0`), `FUN_140051F80`,
`FUN_14005E990`, `FUN_140E000E0`, `FUN_140C3CA10:256`. Before S3 or S4, a zone run must show zero reads of those bits; after S1 the observer `native_material_word` compares
`model+0x224` with the union of the policy words of the model's materials.

### Witnesses (live only, never from this thread)

| Id | Zone / object | Read | Expected |
|---|---|---|---|
| WB-1 | any ChrAsm part with a `SAT_Equip` material (player armour) | `[[model+0x1F8]+i]+0x24`, `model+0x224` | `0xC004` per `SAT_Equip` material and `0xE004` for `SAT_Equip_cloth` (original) versus `0x0` and `0x2000` natively today |
| WB-2 | m40 or m37 water (`m[arsn]_water`) | material word, `model+0x224` | `0x70` |
| WB-3 | a grass object (`m31`, `o317000`) | material word, `model+0x468` | `0x2`, `model+0x468` bit 1 |
| WB-4 | a lightmap map piece (`*_lightmap`) | material word | `0x401`-class word with bit 10 |
| WB-5 | `C[Crystal]` NPC (`c3100`) | word, class | `0x208004`, class 2 |
| WB-6 | after S1, the same parts | pose gate rows, `auxiliary_present`, refusals of meshes with bit 14 | no new refusal; auxiliary palette present |

## 7. OPEN

1. Effect of bits 0/10 on the draw (`FUN_140060700` tagging, `57A20:578`, `5B3D0:255`, `1A9AD0:25`): INFERRED light-probe/lightmap binding; the
   native map renderer has no reader of these bits. Needs the remaining read of `FUN_140060700` and one comparison frame on a `*_lightmap` piece.
2. Closed (I-0113 tranche 13): bit 20 sets `object+0x129` of the chunk `0x25D` object (the E03 model set), whose colour 27EA70 then scales by `+0xAA8`; `FUN_14027E5E0` runs natively (`effect_system.cpp`'s type table) and the instance word now carries the bit.
3. Whether `FUN_140E000F0` / `FUN_140CEDE20` and the paint objects of `FUN_140E000E0` touch the legacy device or are needed by anything native (they run exactly when the predicate `FUN_1400600A0` is true in `FUN_140D05C80`, `FUN_140D3C4D0`, `FUN_140CFDBA0`; `FUN_140CFDBA0` was not read); `model+0x46C` readers (none found with immediate displacements).
4. SPX declaration owners for the GX families: the retained shader bundles hold only programmes and `.metaparam` for SAT families; the declaration list
   of `GXFlver_*` families was not extracted, so the `g_BloodAmount` shadowing is known only from `FC_BloodAmount` in the reflected families.
5. Readers of bits 6, 8, 16, 18, 22, 23, 24, 25: none found with the forms scanned (loads with a following test, direct tests, `[reg+0x88]`, `[reg+0x224]`);
   SIB-indexed or register-hoisted uses are not covered.

## 8. Evidence and replay

- `Census-MtdBits.ps1` (this folder): rebuilds `evidence/I-0066/queries/mtd-bits/{mtd-policy-bits,bit-summary,bit-usage}.tsv` from the I-0068 and I-0063 data and `name-tables.tsv`.
- `Bits24Scan.java` (this folder): the read-only headless scan; outputs `evidence/I-0066/queries/mtd-bits/scan/{b24,b88,b224}.txt` and the filtered `b24-fns.txt`, `b88-fns.txt`.
- `evidence/I-0066/artifacts/mtd-bits/DS3-1152-F-000A7E40.c`: the decrypted handler (copy of the retained source 754).
- Exports (`decompiled/DS3-1152-F-<RVA>`): `0002DD60`, `00140F90`, `00140F10`, `00142130`, `0002A120`, `00029FB0`, `000619B0`, `00061A20`, `00061A90`, `00061B00`, `00061B90`, `000600A0`,
  `00D35F80`, `00D35AF0`, `00C3F6A0`, `00C3CA10`, `00064340`, `00015490`, `000575C0`, `00057A20`, `00059220`, `0005B3D0`, `0005C0E0`, `0005D5C0`, `001A9AD0`, `00051F80`,
  `0005E990`, `0027E5E0`, `00DD3CA0`, `00140680`, `001407D0`, `00060700`, `00140290`, `00E000E0`, `00D05C80`, `00D3C4D0`.
- Pass names `Pnt`/`PntCube` and the other passes: table `14473CE60` (index 8 `Pnt`, 9 `PntCube`, 10 `PntCol`, 11 `SO`, 12 `WatDpt`, 13 `Aof`, 14 `Aof_Dol`, 15 `Rsm`, 16 `Voxelization`, 17 `FwdDbg`, 18 `FwdSdwDbg`, 19 `GBufDbg`).
