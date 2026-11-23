# Sun shadows (cascades): static preparation of the caster path

Thread « Statique régression d'image » (2026-10-16), for the pilot's route in `SUN-SHADOWS-WAVE1.md` (C1 caster selection, C2 rows and
recipes, C3 draws into the atlas). Read-only: no game, no save, no `main`. Labels: **STATIC** (decompile or listing read),
**INFERRED**, **OPEN** (not established). Functions are `FUN_14xxxxxxx`; new exports in `decompiled/`: `0006E590`, `0006F360`, `001B4D80`,
`001B4E30`, `017BBC40` (the rest were already there: `00057A20`, `0005C060`, `0005C0E0`, `0005A120`, `00063CB0`, `00140680`, `001B4830`,
`001B6020`, `0006F310`).

## 0. Answer first

| # | Question | Answer |
|---|---|---|
| 1 | Rows of 57A20 for the cascade passes | **Confirmed, with three corrections.** The branch is `*(int*)(pass+0x124) - 1 < 0x38` (pass ids 1..0x38) and row type `!= 0x27`. One row `{type, prep 2, class mask 0x107, secondary 0xFF}`; two rows (secondary 2 on model 0, secondary 1 on model 1) when model 1 exists and differs. `iStack_4d8` is the **pass id**. The row's *type* is the model's own (`2`, or `0x21`/`0x29` after the fade remap), not a shadow type; the **preparation 2** is what makes it a depth draw. |
| 2 | Prep 2 to `Dpt` / `DptA` | `FUN_14005A120` maps preparation 2 to **shader pass 2** (state `+0x1C`) and sets state bit `0x40000`; `FUN_140140680` turns shader pass 2 into **3 (DptA)** when the template is alpha-tested (`template+0x7C != 0`) or the draw fades (state bit `0x100000`); only the VS is required for pass 2, VS and PS for 3. Bit `0x40000` plus **material `+0xD1 != 0`** refuses the draw (a no-cast flag, writer not found). |
| 3 | Mesh mask, per-mesh culling, LOD | Class mask 0x107 is tested twice: against the model union `model+0x220` (5C0E0) and against `material+0x18` per mesh (63CB0). The secondary mask is tested against `material+0x1C` (1, or 2 for template bit 13). There is **no per-mesh « casts a shadow » bit**: the casting flags are the entity mask, `material+0xD1` and the model/material class. Per-mesh culling is the OBB test through the culler's vtable `+0x28`, only for single-instance models, writing a 128-bit visible mask. LOD: **OPEN** (§3.4). |
| 4 | `FUN_1401B4830` volume against the main culler | Different class, same plane convention (outward, `inside <= 0`), world space, **6 planes at `+0x10`** (order left, right, bottom, top, near, far) against the main culler's 6 planes at `+0x160` and its AABB at `+0x220`. The OBB slot (`+0x58`) of the cascade volume tests the planes at `+0x10`, and also the second set at `+0x70` when `+0xD0` is set. |

## 1. The rows (57A20, 5C060, 5C0E0)

**What the arguments are (STATIC).** `FUN_14005C060(ctx, pass, type, time, prep, class_mask, secondary, context)` packs one 16-byte row
`{u32 type, u32 prep, u32 class_mask, u32 secondary}` and calls `FUN_14005C0E0(ctx, pass, rows, count, ...)`; at 57A20's call sites the
stack words are `uStack_528` the time offset (`param_7` or `param_7 +- _DAT_143d7edb4`), `uStack_520` the preparation, `uStack_518` the class
mask, `uStack_510` the secondary mask and `ppbStack_508` the context `{group, ?, ?, model, instance record ...}` (its `+0x18` is the model
5C0E0 reads). The `{+0, +4, +8, +0xC}` order is read in 5C0E0 (`puVar19[-1] == 0` tests the preparation, `*puVar19` ORs the class
mask, `uStack_b8[-3..0]` copy the four words into the queue item at `+0x28` (short type), `+0x2A` (short class), `+0x2C` (secondary),
`+0x30` (byte prep); the item's first dword is the preparation).

**The cascade branch (57A20 `LAB_140058d9f`, STATIC).**

```
iVar18 = *(int *)(pass + 0x124)            // iStack_4d8 : the pass id
if (iVar18 == 0x42) { ... }                // other branches first (0x42, 0x43, 0x44 are not shadows)
if (iVar18 - 1U < 0x38 && type != 0x27) {  // pass ids 1..0x38
    alt = model_set->vt+8(1)               // second model of the draw record, or null
    if (alt == null || alt == model0) secondary = 0xFF
    else { row(type, prep 2, class 0x107, secondary 2) with model 0 ; secondary = 1 ; model = alt }
    row(type, prep 2, class 0x107, secondary)            // with model (0, or alt)
}
```

- **Pass ids (STATIC + INFERRED).** `FUN_1401B6020` (per cascade) tests `*(int*)(pass+0x124) - 1U < 4` before it installs the
  `1B4830` volume: **ids 1..4 are the four sun cascades** (SHADOWS-WAVE1-READY already says the local-light faces are `i + 9`, ids 9..0x38,
  48 of them). Ids **5..8 are not identified** (MENUS-MAP §62 inferred them as cascades; the bit table gives them 0x02, the same
  family as the G-buffer for characters; I did not find who records them). The same row is added for every shadow-family pass 1..0x38, so a
  native cascade pass can reuse the call as is.
- **Class mask 0x107** = classes 1 (standard), 2 (transparent/recursive), 4 (alpha-ref) and 0x100 (Emi) of `ClassifyModelMaterial`
  (PRODUCERS-UNION §1.1); it excludes 0x80 (forward), 8 and 0x10 (special). Row accepted when `model+0x220 & mask != 0` (5C0E0).
- **Row type.** `uVar34 = *param_3` of the draw record; when it is 2 and the group record exists it is replaced by 0x21 (alpha fade
  `+0xDC < 1` with `+0x84/+0x85/+0x87` set) or 0x29 (the other fade states) before the branch. The cascade row carries that
  type; it does not choose the shader pass (the preparation does).
- **Other conditions before the branch.** `iVar18 == 0x3A && record+0x86 == 0` and `type == 0x27 && iVar18 == 0x39` skip the model
  (not shadow ids). `record+0x86 != 0` sets `pass+0x893` (a per-pass flag, meaning not read).
- **Time offset.** The row call passes the draw's time `param_7` unchanged for shadow rows (the `+/- _DAT_143d7edb4` pair belongs to the
  forward/dither rows of the G-buffer branch).

## 2. Preparation 2 to the shader pass (5A120, 140680)

`FUN_14005A120(pass, state, item)` switches on the item's first dword (the preparation `*param_3`, called `uVar2`):

| prep | state `+0x1C` (shader pass) | state `+0x200` bits added |
|---|---|---|
| 0 | not set here (the material's own: Gbuf by `+0x710`, Fwd) | |
| 1 | 2 | `0x80800` (and special case when the pass `+0x128 == 0x3B`) |
| **2** | **2** | **`0x40000`**; state `+0x30 |= 0x80` when `pass+0x718` is set |
| 3 | 6 (Gbuf) | `0x10100` |
| 4 | 7 (Gbuf2) | `0x20100` |
| 5 | 1 (Velo) | |
| 6 | 0xC | `0x1000` |
| 9..0xC | 0xD..0x10 | `0x10100` |

The pass table the native already pins (`game_bootstrap.cpp:2698`): 0 Fwd, 1 Velo, **2 Dpt**, **3 DptA**, 4 FwdSdw, 5 FwdSSdw, 6 Gbuf, 7 Gbuf2.
`FUN_140140680(material, ctx, state)` (the material's slot 2, SelectModelPrograms' original):

1. `iVar4 = state+0x1C` (the shader pass). A null template, or `iVar4 - 8 < 2` with the instance flags `+0x24` bit 2 clear: return 0.
2. **`state+0x200` bit 18 (`0x40000`, set by prep 2) and `material+0xD1 != 0`: return 0.** This is the only per-material « does not cast »
   gate on the path. `material+0xD1` (and `+0xD0`, the « special » byte of `142130`) are stored by none of the 7 direct byte stores
   found for `+0xD1` (they belong to other classes: `1401CB6B0`, `140C52B70`, `140C98650`, `140FF9BC0`, `14108EE80`, `14108EFB0`,
   `141C8D550`), so the writer is a bulk copy or a word store (**OPEN**; a hardware watchpoint on one material of a tree, or the
   MTD policy bits, would name it; no MTD policy bit of the inventory is called a shadow bit).
3. `iVar9 = state+0x1C`; when it is 2 and (`state+0x200` bit 20, the fade/dither bit that 5A120 sets from the item's `+0x1A9`, **or**
   `template+0x7C != 0`, the alpha test): **`iVar9 = 3` (DptA)** and the stage-2 variant flag `uVar10` is cleared.
4. The programme lookup is `(*selector+0x10)(selector, &out, ctx, iVar9, ...)`. For `iVar9 == 2` only **`out[0]` (the VS)** must be non-null
   (the PS is allowed to be null: depth only); for every other pass **both** `out[0]` and `out[1]` are required, else the draw is
   refused (return 0). So Dpt = VS only, DptA = VS + PS.
5. The state's `+0x18C` bits that matter on this path: bit 2 (set when `iVar9 == 3/4` in 5A120), bit 6 (`0x40`, set when the pass id's
   low 7 bits are not `0x39` and `state+0x168` bit 1 is clear), bit 8, bit 10 (`DptA` alpha dither), bit 11. The native already
   carries these for the G-buffer state; the cascade state is the same function with preparation 2.
6. The draw state words the native must write for the cascade: the shader pass 2 or 3 as above; the cascade rasterizer (slope bias,
   `FUN_1401B6020`'s state re-creation); no colour target; `pass+0x420 = 0x101`, `pass+0x423 = 1`, `pass+0x128 = 0` set by 1B6020.

## 3. Which meshes draw

### 3.1 Entity level (STATIC, OBSERVED once)

The world walk keeps an entity when `(E+0x20).lo & V.lo` and `(E+0x20).hi & V.hi` (MENUS-MAP §62). `V.lo` is `(composite+0xE0 & ~0xFF)`
OR a pass bit chosen by the pass id (table in §62): ids 1..4 give **0x40**, 5..8 0x02, 9..0x38 0x04, 0x39 0x10, 0x46 0x20, others
0x01. World model entities (`FUN_140D05C80`, `FUN_140CFF200`) build `E+0x20` as `lo = 1`, `| 2` and `| 4` from the part flags
(`+0xC8` bits), `& 6`, `| 0x10`, `& 0x16`, and **never 0x40**, with `hi = 1`. So the traverser test as written would reject every world
entity in ids 1..4. **OPEN and important:** either the cascade passes do not use this visitor test (they draw through the world
renderer's rappel `FUN_140142E80`, which calls the entity's vtable `+8` after the culler, MENUS-MAP §62), or `V.lo` carries more bits
than the table says. Test for the pilot: log, for one frame at m40, the `E+0x20` of the first 50 entities and the `V` mask the
original builds for pass ids 1..4 (the old Reference observer already reads the visitor; one row).

### 3.2 Model and material level (STATIC)

- Row accepted when `model+0x220 & class_mask != 0` (5C0E0, `lVar25 == 0 || (+0x220 & uVar23) != 0`). The union is the OR of the mesh
  classes (PRODUCERS-UNION §1.1) that native already computes for every model (`selected.material_mask`).
- In `FUN_140063CB0` (per-mesh selection of the item): a mesh draws when
  1. `mesh.flags & maskA == reqA` with `maskA/reqA` from the draw state `+0x180` bit 1 (`2`) and bit 2 (`|1`): mesh flag bit 1 and/or bit 0;
  2. the mesh's LOD word `mesh+0x14 >> lod & 1`;
  3. the material (looked up by the mesh's material index): `material+0x18 & item+0x40 != 0` (item+0x40 = the row's **class mask 0x107**),
     `material+0x1C & item+0x44 != 0` (the row's **secondary mask**), and the material's pass-support words
     (`+8`, `+0x10`) intersect the item's `+0x30/+0x38` words;
  4. per instance record: the instance class masks `+0x18/+0x20` against the material's, and the **instance mesh mask**
     (128 bits, `+0x28/+0x30`) for the mesh ordinal.
- `material+0x1C` is **1, or 2 when the template flag `+0x88` bit 13 (0x2000) is set** (`FUN_140140F90` line 41; `0x2000` is the
  cloth bit of the native policy flags, `WIND-BLOCK` sheet). So the two-row split with an alternate model separates **cloth from the
  rest** (secondary 2 = cloth meshes of model 0, secondary 1 = the others of the alternate model), and `0xFF` takes both.

### 3.3 Per-mesh culling (57A20 lines 340-394, STATIC)

Applies when `*(int*)(pass+0x4E8) == 1`, the model exists, the draw record `param_10` has exactly **one instance** (`+0x70 == 1`), its model
has its bounds valid (`*(param_10+0x80)+0x1E0 != 0`), its current matrix slot is non-null, and **`pass+0x410` (the pass culler) is set**.
For each mesh bound (`model+0xC8`, stride 0x18: min xyz, max xyz) it builds an **oriented box** from the instance matrix times the model
local transform (`model+0x1D0`): three unit axes (rows of the 3x4, normalised), half extents `h * |axis|`, centre, the 0x50-byte block
`{half extents vec4, axis x, axis y, axis z, centre}`, then calls the culler's vtable **`+0x28`** with it. Return 0 = outside: the mesh's
bit (index < 0x80) is cleared in a 128-bit mask that starts all ones (the two qwords are the `instance mesh mask` of 3.2). Both
cullers' `+0x28` is the same wrapper `FUN_14006E590`, which copies the block and calls vtable `+0x58`:
main culler (`143D42D58`): `+0x58 = FUN_14006F360 = ADD RCX, 0x160 ; JMP 1417BBC40` (the frustum object at `+0x160`: 8 corners then
the 6 planes); cascade volume (`143D5D168`): `+0x58 = FUN_1401B4E30` (below).

### 3.4 LOD (OPEN)

63CB0 starts from `item+0x18` (the LOD index in the draw state), then lowers it to the nearest LOD whose bit is set in the model's LOD
mask `model+0xB4` (`item+0x18 = iVar9`) and restores the original value at the end; a mesh is drawn if its own LOD word has that bit.
**Where `item+0x18` comes from for a cascade pass is not established**: `FUN_1401A9530` (the state initialiser called at the end of
`FUN_1401A8580`) and the per-model selector `ppuStack_480 = *(param_8+0x98)` are the candidates. Native already selects the LOD for the
main view; the cheapest safe rule is to reuse that selection and apply the same lowering and the same per-mesh LOD word, then compare
the number of casters per cascade with the original at one frame (a Reference row of the item's LOD for pass ids 1..4).

## 4. The cascade culling volume (1B4830, 1B6020, 1B4E30)

**Construction (STATIC).** The shadow object's constructor `FUN_1401B5010` places four volume records at `obj+0x570 + i * 0xE0`
(`param_1[0xAE], [0xCA], [0xE6], [0x102]`), each with vtable `143D5D168`. `FUN_1401B6020(obj, pass, lightDir, i, flag)`:

1. `FUN_14023C1E0(obj+0x170 + i*0x100, pass+0x130, lightDir, ...)` builds the generic light-space volume and `pass+0x410` is first set
   to it (or to `pass+0x130` when the first is null);
2. two fits: the first (`FUN_140239610` or the single-map `FUN_140238B40`, by `obj+0x18 == 0`) gives the pass's view and projection
   (`FUN_140203FB0(pass, &viewOut, &projOut)`); a second one over the cascade record `obj+0x140+i*8` (its `+0xF0` size halved when
   `+0x121`, `+0x39`, `+0x2C`) refits the outputs;
3. if **`pass+0x124 - 1 < 4`** (ids 1..4 only): **`FUN_1401B4830(obj+0x570+i*0xE0, &A, &B)`** fills the volume from the second fit's two
   outputs, and `pass+0x410 = that record` (`lVar11`), falling back to `lVar12` when null. When `DAT_14473DAD4` and `obj+0xC1D+i` are set,
   `FUN_1401B4AC0` fills the second plane set at `+0x70` from the cascade's other matrices (`+0x60..+0xDF`, the reused-frame volume)
   and the byte `+0xD0` becomes 1, else 0.

**`FUN_1401B4830(rec, A, B)` (STATIC, exact).** With `B` the projection-like matrix and `A` the view-like one (INFERRED roles; they are the two outputs of the fit) (16 floats, rows `B[0..3], B[4..7], B[8..11], B[12..15]`)
it writes **six clip-space planes** at `rec+0x10..+0x6F` as `-(col3 +- col_k)`:

| `rec+` | plane (4 floats) |
|---|---|
| 0x10 | `-(B[3]+B[0], B[7]+B[4], B[11]+B[8], B[15]+B[12])` left |
| 0x20 | `-(B[3]-B[0], B[7]-B[4], B[11]-B[8], B[15]-B[12])` right |
| 0x30 | `-(B[3]+B[1], B[7]+B[5], B[11]+B[9], B[15]+B[13])` bottom |
| 0x40 | `-(B[3]-B[1], B[7]-B[5], B[11]-B[9], B[15]-B[13])` top |
| 0x50 | `-(B[2], B[6], B[10], B[14])` near |
| 0x60 | `-(B[3]-B[2], B[7]-B[6], B[11]-B[10], B[15]-B[14])` far |

then replaces each plane `p` by `p'[i] = A[4i]·p[0] + A[4i+1]·p[1] + A[4i+2]·p[2] + A[4i+3]·p[3]` (row `i` of `A` dotted with the 4-vector; read from the stores, 57A20-style SSE order): world-space planes, **outward**
(a point is inside when every `plane . (x,y,z,1) <= 0`), **not normalised** (only sign tests are made). Note the ordering of the stores
(+0x50 near before +0x60 far) and that the same arguments `(A, B)` are the `FUN_140203FB0` arguments of step 2.

**Against the main culler (STATIC).** `143D42D58` stores its six planes at `+0x160` (the frustum object `FUN_1417BBC40` and
`FUN_14182F000` read them), the world AABB at `+0x220`, and `FUN_14006F310` (slot `+0x60`) tests an AABB in two stages: `FUN_14001FFF0`
on `+0x220` first (outside: 0), then each of the six planes with the box corner picked per plane sign (the « negative vertex »: all
positive = outside; a mix = crossing): **0 outside, 1 crossing, 2 inside**; the entity-level `V` test used by the walk calls this slot.
The cascade volume `143D5D168` has no `+0x220` AABB stage; its AABB slot `+0x60 = FUN_1401B4D80` and OBB slot `+0x58 = FUN_1401B4E30`
test the plane set `+0x10` with `FUN_142118750` (AABB) and `FUN_142118860` (OBB) and, **when `+0xD0` is set, also the second set at `+0x70`**
(INFERRED from the decompile, which drops the return value: the second call's result is returned, so a box must be inside both volumes, the current cascade's and the reused-frame one).
The AABB path of the cascade volume is skipped when `DAT_1449640E9` is set (a development switch, 0 in the image).

**What native should do (proposal, consistent with the pilot's C1).** Do not rebuild the planes by hand: allocate a 0xE0-byte record with
vtable `image + 0x3D5D168`, call `FUN_1401B4830` with the same `(A, B)` the native already feeds `FUN_140203FB0`, set `+0xD0 = 0`
(wave 1 has no reused-frame set), and use the engine's own slot `+0x28` (or `+0x58`) for the per-mesh OBB test and `+0x60` for entity AABBs,
as the native already does for `140239610` (« engine CALLs on native-built records »). The pass culler pointer is then that record;
the main view keeps `143D42D58`. A test that does not need the game: for the four captured cascades of Reference1705/1706 compare
the six planes built natively with the six planes the original stores (read `rec+0x10..+0x6F` in the original at the caster draw).

## 5. What I could not close

1. The pass ids 5..8 (what they draw, whether they are cascades of a second light or the cloud variant); the pilot's `1..4` is confirmed.
2. The entity-level mask of §3.1 (the visitor bit 0x40 against entity masks that never carry it).
3. The writer of `material+0xD1` and the meaning of the no-cast flag (§2.2).
4. The LOD source for pass ids 1..4 (§3.4).
5. `pass+0x4E8 == 1` (which passes enable per-mesh culling; assumed true for the cascades because the cascade volume implements the slot).
6. The two fits of `1B6020` (the volume's fit differs from the pass's own); the pilot's `FUN_140239610` call replicates the first.
7. The semantic of the `+0xC1D+i` / `DAT_14473DAD4` second volume (a temporal reuse of the cascade): out of wave 1 unless the
   original's shadow frames are bit-compared.

## 6. Checks to add to the pilot's C1-C3 (no game needed unless stated)

- C1: the walk keeps a model for cascade `i` when its **entity** test (3.1, open) and the engine culler `vt+0x60` on the model bounds
  return nonzero; per-mesh OBB through `vt+0x28` for single-instance models; counts per cascade logged (original vs native: live).
- C2: assert, for every cascade row, `{prep 2, class 0x107, secondary 0xFF or (2, 1)}`; the recipe is `Dpt` unless `template+0x7C != 0` or the draw
  fades (`DptA`); VS only for `Dpt`; `material+0xD1 != 0` refuses (log the count: expected 0 at m40).
- C3: `pass+0x420 = 0x101`, `+0x423 = 1`, `+0x128 = 0`; the pass view and projection from the first fit; the culler from the second.
