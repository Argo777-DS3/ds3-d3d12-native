# I-0068 part AD: producers of model +0x220/+0x224, of the three snow words, and the Glow Filter mapping

Request (coordinator, 2026-10-10), ready to code. The pilot now draws NPC bodies on its branch.

1. The original producer of model `+0x224` (bit 15 and the rest of the word) for map pieces and
   objects: who writes it, and from what. Native should produce it for every family, not only
   character parts.
2. The producers of the three snow words (`23790`, `1A86C0`, `22E930`), with the sources of
   their values.
3. The mapping of `+0x14` and `+0x20` to the gparam `Glow Filter`.

Labels: STATIC, OBSERVED, INFERRED.

**Evidence:**

- `evidence/I-0068/queries/producers/`: the decompiles and query indexes of this part;
- `Find-DispWrites.ps1`: a raw displacement-write scan of the executable.

## 0. Answer first

| # | Question | Answer |
|---|---|---|
| 1 | Who writes model `+0x220`/`+0x224` | **The writer is not found statically** (§1.3). **What it contains is known.** `+0x220` is the OR of the material classes of the model's meshes. `+0x224` is the OR of their policy flags. Both are per material instance and come from `140F90` (§1.1). The values agree with Reference1102. **Ready to code:** run the native union of `game_bootstrap.cpp:3667-3671` for every selected model, map pieces and objects included (§1.4) |
| 2a | b9 `FC_Snow*` (`23790`) | the gparam **Snow** group: bank `[GXRS+0x45F8]`, scene keys `frame+0x3FC0/+0x3FC8`, weight `frame+0x3FD0`, blend `131780`. Field-by-field layout in §2.1 |
| 2b | b7 `FC_SnowRate` (`1A86C0`) | gparam **SnowModel** `Snow Rate` (bank `[GXRS+0x4600]`, per-model keys `group+0x70/+0x78`, weight `group+0x80`) × the scene's Snow `Scene Snow Rate` (`[GXRS+0x4E70]`). §2.2 |
| 2c | b2 `FC_SnowMaskParam` (`22E930`) | the FLVER material's **GX list item `GX06`**: its first value `s` → `{s, 0, −0.5·s, 0}`. §2.3 |
| 3 | `+0x14`, `+0x20` | **STATIC:** `+0x14` = **Blur RgbScale**, `+0x20` = **NewGlow Final ColorScale**. The whole parameter block is in §3. One inconsistency is open (§3.3) |

## 1. Model `+0x220`/`+0x224`

### 1.1 What the union is made of (STATIC)

**The class:** `140F90` (the material instance set-up) stores the class returned by `142130`
at **instance `+0x18`**.

- `142130` reads the template at `[instance+0x80]`. With a null template the class is 1.
- With the instance special flag `+0xD0`, the class is `0x10` when template `+0x110` is set,
  otherwise 8.
- Otherwise:
  - transparent (template `+0x111`) or template `+0x88` bit 21 (Recursive) → **2**;
  - forward (template `+0x110`) → **`0x80`**;
  - template `+0x88` bit 15 (Emi/Emissive) → **`0x100`**;
  - alpha-ref (template `+0x7C`) → **4**;
  - otherwise **1**.
- This is exactly `ClassifyModelMaterial` (`native/content/model_parameters.cpp:444-450`).

**The flags:** `140F90` copies template `+0x88` to **instance `+0x24`**.

- It replaces bit 28 with `[instance+0x70]->vt+0x38()`.
- It sets `+0x22` = template `+0x88` bit 13.
- `82220` (the per-material loader, vtable `143D44118` slot 5) repeats the bit 28 update.
- The template `+0x88` bits are the policy flags native rebuilds (`model_parameters.cpp:408-441`).

**The union:** model `+0x220` (classes) and `+0x224` (flags) are the OR of these two words over
the model's mesh materials.

- Reference1102 measured, under the original renderer:
  - classes: part 0 → 5, part 1 → `0x111`, parts 2–5 → `0x100`;
  - flags: armour `0xC004` (bit 15).
- The native union (`game_bootstrap.cpp:3667-3671`, Native1103–1112) produces the same values.

### 1.2 Who reads them (STATIC)

`57A20` adds rows from these words:

- `+0x224` bit 15: engine 7 (class `0x100`);
- `+0x224` bit 12: rows 4 and 0x1F (class `0x80`);
- `+0x224` bit 2 (line 252);
- `+0x224` bits 0, 7, 10 and 11: other row tests (decompile lines 430–610; their meaning is not read here);
- `+0x220` bit 2: the engine 5 split.

### 1.3 The original writer was not found (STATIC, negative)

Searched:

- **Executable listing:** every store, OR, AND, BTS and wide (xmm/qword) store at `+0x220` or
  `+0x224`, over the whole executable (I0067Map `g:` queries).
- **Raw bytes:** `89`, `09`, `81 /1`, `83 /1` and `C7` writes with displacement `0x224` or
  `0x220` (`Find-DispWrites.ps1`, 31 and 309 sites), including code that is not disassembled
  in the listing.
- **Instance-word accumulation:** every `OR r32, [r+0x18]` and `OR r32, [r+0x24]` site.

Result:

- Every hit belongs to another class: views, cameras, copies, constructors, AES, Havok.
- The model constructor `9DC20` zeroes `+0x220..+0x227` (qword `[0x44]`). Its four virtual
  slots and the model build path `D35AF0` do not write them either.

INFERRED: the writer runs in code the database does not hold as instructions (protected or
packed code), or it reaches the fields through a base pointer to a sub-object.

**Witness that settles it:** one run under the original renderer with a hardware write
watchpoint on `model+0x224` of one map piece with an Emi mesh. It logs the writer's RIP. This
is optional: native does not need the writer.

### 1.4 Ready to code: the union for every model

1. In `NativeSelectedScene` admission, compute `classes |= *mesh.material_class` and
   `flags |= mesh.programs->policy->flags` over the model's meshes for **every** candidate, not
   only `c.neutral_instance_decal` (`game_bootstrap.cpp:3667-3671`).
   - Store them in `selected.material_mask` and `selected.material_flags`.
   - Pass preparation already prefers these over the live words (`:3873`, `:3879`, `:3907`).
2. **Completeness:** the original ORs every material of the model. Native's union covers the
   selected current-LOD meshes.
   - Use all meshes of the model's material set when available, so that a LOD change does not
     drop a row.
   - Keep the current rule (refuse the union when a mesh has no class or policy) as a
     fallback.
3. **Effect:** map pieces and objects with Emi/`Interior` meshes get the engine-7 row.
   - This admits rows 8 and 11 (NATIVE-GATES-AUDIT §2.4–2.5).
   - It also admits the `0x1F` row for forward non-transparent map materials (bit 12) and the
     engine 5 split for alpha-tested ones (`+0x220` bit 2).
4. **Checks:**
   - G1 of NATIVE-GATES-AUDIT (m37 `Interior` and High Wall bowl drawn on engine 7);
   - no change in M01 draw counts at the five witness zones.

## 2. The snow producers

### 2.1 b9 `cbPerFrame` snow rows (`23790`, decompile lines 307–336) (STATIC)

**Source:** the gparam **Snow** group (default.gparam group 20, 11 parameters).

- **Bank:** `[GXRS+0x45F8]`, lookup `12F400`. A missing key falls back to the bank's built-in
  record at `bank+0x300`.
- **Keys:** `frame+0x3FC0` and `+0x3FC8`, the scene key pair `88AF0` writes.
- **Weight:** `frame+0x3FD0`.
- **Blend:** `131780(out, A+0x10, B+0x10, t)`.
  - `t ≈ 0` or `t ≈ 1` copies A or B.
  - Otherwise every float is lerped. The angles use a wrap-around lerp (`1316D0`) over
    [−π/2, π/2] and [−π, π].

**Entry layout** (the `131780` base; constructor `12DCC0` gives the built-in defaults). Each
field's cbPerFrame variable comes from the snow PS RDEF.

| Entry | Gparam field (INFERRED by order and RDEF name) | Default | cbPerFrame |
|---|---|---|---|
| `+0x60`, `+0x64` | Angle (β, α) | from dir (0, 1, 0): β = −π/2, α = 0 | via the direction |
| `+0x70..+0x7C` | direction, computed: `(cos β·sin α, −sin β, cos β·cos α, 0)` (INFERRED: `141FD1A40` = sin, `141FD1740` = cos) | (0, 1, 0, 0) | **+0x60 `FC_SnowDir`** |
| `+0x80` | Scene Snow Rate | 1.0 | not in cbPerFrame. `23790` stores it at **`[GXRS+0x4E70]`** (line 321) for `FC_SnowRate` |
| `+0x90`, `+0x94` | Snow Tiling Scale (a, b) | (1, 1) | **+0x70 `FC_SnowTilingScale` = `{a, b, (1−a)·0.5, (1−b)·0.5}`** (`[143D7EDB4]` = 0.5) |
| `+0xA0..+0xAC` | Albedo Scale | (0.8, 0.8, 0.8, 1) | +0x80 `FC_SnowAlbedo` |
| `+0xB0` | Reflectance | 0.04 | **+0x90 `FC_SnowReflectance`** |
| `+0xB4` | Ground Plane Height (g) | 0 | **+0x94 `FC_GroundPlaneHeight`** |
| `+0xB8` | Snow Plane Height (h) | 0.5 | **+0x98 `FC_RcpSnowHeightRange` = 1/(h − g)** |
| `+0xBC` | Normal Scale | 0.02 | +0x9C `FC_SnowNormalScale` |
| `+0xC0` | Normal Blur Radius | 3.2 | (not in cbPerFrame) |
| `+0xD0`, `+0xD4` | Sparkle Tiling Scale (s, u) | (1, 1) | +0xA0 `FC_SnowSpakleTinlingScale` = `{s, u, (1−s)·0.5, (1−u)·0.5}` |

**Ready to code:**

- extend the native per-frame producer (`ProducePerFrame`, `native/world/producers.cpp:416`)
  with these rows, from the native gparam banks (the pilot's `Banks::Resolve`, with the same
  keys and weight);
- with no Snow entry, use the defaults above.

### 2.2 b7 `FC_SnowRate` (`1A86C0`, lines 206–223) (STATIC)

`FC_SnowRate = lerp(L(k0)+8, L(k1)+8, w) × [GXRS+0x4E70]`, with:

- `L = 46300([GXRS+0x4600], ·)`: the **SnowModel** bank (INFERRED: its constructor `44930` and
  editor `44BC0` sit beside it; the editor names "SnowModel Snow Rate");
- `+8` = **Snow Rate**. The built-in record is at `bank+0x118`; its default Snow Rate
  (`+0x120`) is **1.0** (`44930`);
- `k0 = state+0x2B0`, `k1 = state+0x2B8`, `w = state+0x2C0`. In native terms
  (`state+0x240` = group, `game_bootstrap.cpp:4389`): **`group+0x70`, `group+0x78`,
  `group+0x80`**;
- `[GXRS+0x4E70]` = the scene's Snow `Scene Snow Rate` (§2.1).

`FC_SnowMinDot = 0` and `FC_SnowMaxDot = 1` are constants, already produced natively.

**Ready to code:** in `ProduceModelParamPs` (`native/world/producers.cpp:372`), add `+0x40`
from these inputs. With no SnowModel entry and no Snow entry, the value is 1.0 × 1.0.

The same block also leaves `+0x14`/`+0x18` unproduced: `FC_EnvMapColorScale0/1` =
`416A0([GXRS+0x42A8], state+0x2F0 / +0x2F8)+0xC`, the EnvMap scales (TONEMAP-SKIN). The snow
PS does not read them.

### 2.3 b2 `FC_SnowMaskParam` (`22E930` flag `+0x11`) (STATIC)

- **Value:** `22E930` (asm `14022F4BD`) writes `+0xA0 = {s, 0, −0.5·s, 0}` when
  `source+0x11` is set, with `s = source+0x3C` and `source = material+0xD0`.
- **Source of `s`:** the material set-up `141070` reads the FLVER material's **GX list**.
  - For item **`GX06`** (id `0x36305847`), it sets material `+0xE1` (= `source+0x11`) to 1 and
    material `+0x10C` (= `source+0x3C`) to the item's **first value** (`param_2[3]`, after the
    id, unk and size words; decompile line 256).
  - It also raises `+0x4E8` to at least 10.
- `22D620` (parameter animation) copies the source (`22E0F0`). No animation kind changes these
  two fields.
- **Ready to code:**
  - native already holds `material->gx_records` (damage parameters use them);
  - in the 22E930 transcription (`game_bootstrap.cpp:3524-3540`), add
    `block(0xA0, gx06 present, {s, 0, -0.5f*s, 0})`;
  - without `GX06`, the block keeps the creation zero (the current rule of `block`).

**Snow is ready to code once 2.1–2.3 exist:** the three refused words are then produced.

## 3. The `Glow Filter` mapping (STATIC)

### 3.1 From gparam to the filter

| Step | Function | What |
|---|---|---|
| bank | `[GXRS+0x4658]`, lookup `17EF20` (fallback record `bank+0x140`, constructor `17DBE0`) | entry `+8`: **Enable, BlurSampleScale, Blur RgbScale, Blur AlphaScale, New Glow Enable, NewGlow Final ColorScale** (the order of the gparam group and of the editor `17DF20`) |
| keys, blend | `FB890` lines 656–683: keys `frame+0x4028/+0x4090`, weight `frame+0x40D0`; blend `2227C0` | floats lerped; the two enables taken from the second entry (from the first when `t ≈ 0`) |
| filter | `GXGlowFilter` at `pipeline+0x2E80` (constructor in `843C0`) | `+0x08` BlurSampleScale, `+0x0C` Blur RgbScale, `+0x10` Blur AlphaScale, `+0x14` New Glow Enable, `+0x18` Final ColorScale, `+0x1D` Enable; `+0x1C` = keep the debug values (written 0 only in the constructor) |
| draw param | `1BB6E0` builds `1BB810`'s 0x30-byte block | see §3.2 |

The whole step runs only when `frame+0x3ECA` is set.

### 3.2 `1BB810`'s parameter block

| Offset | Content |
|---|---|
| `+0x00` | pointer (argument 7) |
| `+0x08` | the source target (argument 5) |
| `+0x10` | **BlurSampleScale** (the blur `scale`, EMISSIVE-GLOW §8.2) |
| `+0x14` | **Blur RgbScale**: `g_GlowScaleOffset.x` of the extraction and level composites (× clamp(factor)) |
| `+0x18` | **Blur AlphaScale**: `g_GlowScaleOffset.y` (not read by the shader) |
| `+0x1C` | **New Glow Enable**: mode B (the four-level chain) |
| `+0x20` | **NewGlow Final ColorScale**: `g_GlowScaleOffset.x` of the final composite (× clamp(factor)) |
| `+0x24` | **Enable** (gate) |
| `+0x25` | flag (argument 6) |
| `+0x28` | argument 8 |

**Defaults:**

- built-in record: Enable 0, BlurSampleScale 20, RgbScale 3, AlphaScale 1, New Glow 0,
  Final 1;
- filter constructor: 10, 1, 1, 0, 1, Enable 0;
- default.gparam id 10: 0, 20, 3, 1, 0, 1.

### 3.3 The inconsistency is resolved (part AE)

**The first reading was too narrow.** Only `m40_00_*` and `default` were dumped.

**Source of the Glow values** (STATIC data): the base gparam **`mapbase.gparam` id 0** carries
the Glow group.

| Field | Value |
|---|---|
| Enable | 1 |
| BlurSampleScale | 1 |
| Blur RgbScale | 1.5 |
| Blur AlphaScale | 1 |
| New Glow Enable | 1 (mode B, as OBSERVED) |
| NewGlow Final ColorScale | 0.5 |

This is identical in `gamedata/` and `gamedata-base/`.

**Other files checked** (all retained I-0045 gparam copies, group by group):

| File | Glow | Snow | MotionBlur |
|---|---|---|---|
| `default` | id 10 (Enable 0) | none | yes |
| `mapbase` | **id 0** (above) | none | yes |
| `remobase` | id 0: Enable 1, BlurSampleScale 2, RgbScale 2, AlphaScale 1, New Glow 1, Final 0.5 | none | yes |
| `m_template` | none | none | partial (11 rows) |
| `empty`, `eclipse`, `ml_template`, `mapoverride_0000/0002` | none | none | none |
| `m40_00_0000/0010/0510` (base and DLC copies) | none | none | none |
| `mapoverride_0001`, `mapoverride_0003` | none | **id 0** (Scene Snow Rate 0, Angle (−π/2, 0), SnowModel Snow Rate 1) | none |

INFERRED: `remobase` is the base for "remo" (cut-scene) views.

**Keys** (STATIC):

- The Glow filter reads key index **10** of the pipeline key arrays (A `+0x3FD8+8i`,
  B `+0x4040+8i`, weight `+0x40A8+4i`; setters `884F0`/`88530`).
- `D52B70` (per-area draw parameters) sets indices **0–9 only**.
- Index 10 is written only by `88AF0`, which sets every index (0–12) to the view key.
- So Glow resolves through the view key (see the m40 file set below).
  - `MapBase` is resident kind 6 (name table `14294F020`; the string `u"MapBase"` is at `14294F148`).
  - `MapOverride_%04u` is formatted by `D282E0`/`D27E20`.

**No other writer:**

- `pipeline+0x2E9C/+0x2E9D` are written only by the pipeline constructor and the `FB890` copy.
- The debug editor `17DF20` binds its own struct.

**Gparam files for m40** (STATIC + regulation; part AJ, from MENUS-MAP §39 and §43 and the
Ceremony param `evidence/I-0067/queries/gparam/ceremonyparam.tsv`):

- **Resident, requested at start-up** (`FUN_140D27D30` kinds, name table `14294F020`):
  - kind 1 `default` (area 1, applied at once);
  - kinds 2/3/4/5 `M_template`, `ML_template`, `S_template`, `empty`;
  - kind 6 **`MapBase`**;
  - kind 10 `RemoBase` (applied at once with weight 0);
  - kind 12 `eclipse` (area 7).
  - Kinds 6..11 are map kinds with an encoded-map area and are applied by the map step, not at
    once.
  - INFERRED: `MapBase` is therefore applied under the map's area.
- **Per map** (map step `14061B590` with the Ceremony row map id + ceremony):
  - ten files `m40_00_{i·100 + base}`, i = 0..9, with base = row `+0x08`:
    - row 40000000: base 0 → `m40_00_0000`;
    - row 40000010: base 10 → `m40_00_0010` and `m40_00_0510` (the ones that exist);
    - row 40000020: base 0;
  - `MapOverride_%04u` with row `+0x0A` = **0** for every m40 row → **`MapOverride_0000`**,
    which has no Glow and no Snow.
- **The Glow key:** index 10 is set by `88AF0` to the view/area key.
  - The banks resolve (area, id), then fall back to (1, 0), then to the built-in record.
  - With `MapBase` under the map's area (INFERRED), the world key finds **`MapBase` id 0**:
    Enable 1, mode B. This matches the observed chain.
  - `default` (area 1) has Glow only at id 10.
- **Remaining live read:** the area under which `MapBase` registers (the `49D00` call for kind
  6). One read; LIVE-CHECKS-CONSOLIDATED §1.1 "gparam ids".

**Native:**

- resolve the Glow group with the same key (view/area key → (1, 0) → built-in record);
- with `mapbase` loaded, this gives mode B with `x = 1.5 × f` for the levels and `0.5 × f` for
  the final composite.
