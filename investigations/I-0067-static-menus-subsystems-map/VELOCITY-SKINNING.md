# Velocity, skinning side: how the original keeps the previous frame's palette (STATIC; INFERRED where marked)

Input for the velocity and motion-blur specification of the second static thread
(`VELOCITY-MOTIONBLUR-NATIVE`, I-0068). This note covers only the skinned and model side.
Sources:
- the decompiled pose owner `FUN_140015490`, `FUN_140015FC0`, `FUN_140016000`,
  `FUN_140016050`, `FUN_140015B10`;
- the per-draw palette binder `FUN_140064340` (called by the default mesh draw
  `FUN_140064610`);
- the reflection of the `Velo` vertex programmes.

## 1. What the velocity vertex programmes read

`GXFlver_ColDifSpcBumpIbl_Bloody_Velo_Skin.vpo` (the same for `_Velo.vpo`) uses:

| Slot | Buffer | Content |
|---|---|---|
| b0 | `cbMatrixWVP` | `VC_MatrixWVP` @0 |
| b4 | `cbInstanceData` | `VC_InstanceData` |
| **b5** | `cbObjMatrix` | **`VC_aObjMatrix`, the current palette** |
| **b6** | `cbPrevObjMatrix` | **`VC_aOldObjMatrix`, the previous palette** |
| b11 | `cbTexScroll` | `VC_TexScroll_0` |

- The cloth variant (`…_Cloth_Velo_Skin.vpo`) adds b2 `VC_isEnableCloth` and b7
  `cbvModelParam.VC_World`.
- The pixel programme `…_Velo_[A].ppo` only does the alpha test (`g_DiffuseTexture`,
  `cbModelParam.FC_DifColMul`).
- **No previous camera matrix is bound to these programmes.**
  - The palette rows are world-space with the object matrix composed in (MENUS-MAP section 62),
    so `VC_aOldObjMatrix` carries both the previous bone motion and the previous object motion.
  - INFERRED: both positions are projected with the same `VC_MatrixWVP`, and camera motion is
    handled elsewhere (the second thread's camera reprojection).

## 2. The pose owner keeps a ring of palettes

The pose prefix is 0x88 bytes: `SprjModelDrawEntity` `E+0xA80`, the native
`NativeModelPosePrefix`.

| Offset | Field |
|---|---|
| `+0x00` | per-mesh palette buffers, one 0x20-byte set per ring slot (used when `model+0x1E0` = 0) |
| `+0x08` | whole-skeleton palette buffers, one per ring slot (used when `model+0x1E0` ≠ 0) |
| `+0x10`, `+0x18` | auxiliary and cancel palettes (cloth composition, b12), one per ring slot, created when `model+0x224` bit 14 |
| `+0x20`, `+0x28` | **CPU matrix rows, two arrays**: `matrices[index]` is written by `FUN_140015FC0`/`FUN_140016000` |
| `+0x60` | the 0x20-byte per-mesh tables (`[+0x60][mesh]`: `+0x18` bone count, `+0x20` bone indices, `+0x16` default bone) |
| `+0x68` | **index**, the current ring slot |
| `+0x6C` | **ring = 1 + keepPrevious**, where keepPrevious is the third argument of `FUN_140015490` |
| `+0x74`, `+0x75`, `+0x76` | fast path, single, cloth-this-frame flags |
| `+0x78` | allocator |
| `+0x80` | model |

**Per frame (`FUN_140016050`):**
1. `index = (index + 1) % ring`;
2. upload this frame's rows into the slot `index`:
   - with `model+0x1E0` = 0: per mesh, the rows of the mesh's bone list
     (`[+0x60][mesh]+0x20`), or of its default bone (`+0x16`) when the list is empty;
   - otherwise the whole skeleton;
   - through `UpdateSubresource` (`FUN_141920300`) on the immediate or deferred context.
3. The slot `index − 1` (mod ring) still holds **last frame's palette**.

**Who keeps a previous palette** (`FUN_140015490`'s third argument):

| Owner | Pose | keepPrevious |
|---|---|---|
| `SprjModelDrawEntity` (`FUN_140D05C80`): characters' `ChrModel` and map objects | `E+0xA80` | **`E+0xBA5` = `E+0xCA` bit 4** (copied at construction) |
| ChrAsm parts (`FUN_140D009E0`) | `[E+0xB10]+0x50·…` | **`E+0xBB0` bit 6** |
| `FUN_140D16F60` | `+0xA0` | its own flag (register `SIL`) |
| `FUN_14087AEA0` (pose of `[+0x4B0]`) | | **0: ring 1, no previous** |

With ring = 1, "previous" is the current slot, so that owner produces no skinned velocity.

## 3. The binder: which buffers go to b5 and b6 (`FUN_140064340`)

For a draw of mesh m (`param_3+2` = m's palette table index), with pose P:

```
cur  = P+0x68;  prev = (cur - 1) mod P+0x6C
if ([P+0x80]+0x1E0)  palette(slot) = [P+0x08][slot]            // whole skeleton
else                 palette(slot) = [[P+0x00] + 0x20*slot][m] // per mesh
ctx->vt+0x28(palette(cur), …)    // b5 cbObjMatrix
ctx->vt+0x30(palette(prev))      // b6 cbPrevObjMatrix
```

**Cloth and auxiliary branch:** taken when material `+0x24` bit 14 is set, or when the cloth pass
flag (`ctx+0x180` bit 8) is set, `e+0x22` is set and `P+0x76` is set. It binds:
- `ctx->vt+0x38([P+0x10][cur], [P+0x18][cur])`;
- `ctx->vt+0x40([P+0x10][prev], [P+0x18][prev])`.

These are the current and previous auxiliary and cancel pairs.

## 4. Native contract (generic, by family)

| Item | Contract |
|---|---|
| Ring | Keep **two** palette versions per pose owner when keepPrevious holds (`E+0xCA` bit 4 for `SprjModelDrawEntity`, `E+0xBB0` bit 6 for ChrAsm parts); one otherwise |
| Order | At each pose update, advance the slot, then publish the new rows. The previous version stays immutable for this frame's draws |
| Binding | Velocity draws: b5 = current version, b6 = previous version (the same version when ring = 1). Cloth and auxiliary: the current and previous pairs |
| Contents | The same rows as b5 (world-space, object matrix composed in, whole skeleton or per mesh as `model+0x1E0` says) |
| First frame | Previous = current (zero velocity), as the original ring is filled on the first update |
| Size | Up to 590 bones for the largest skeleton (MENUS-MAP section 66.3) |

Runtime checks on the original:
- `E+0xA80+0x6C` (ring) and `+0x68` (index) on a character over two frames;
- the two buffers that `FUN_140064340` binds for a `Velo` draw.

## 5. Non-skinned objects (map pieces, objects, disp items)

- **The same vertex programmes and the same buffers.** The rigid `…_Velo.vpo` reads b5
  `cbObjMatrix` and b6 `cbPrevObjMatrix`, like the skinned one. A rigid mesh is a one-row
  palette: its default bone, `[+0x60][mesh]+0x16`, when the mesh's bone list is empty.
- **With a pose owner** (a `SprjModelDrawEntity`, including map pieces and objects, or a
  `SprjFD4ModelItem` through the same prefix):
  - the ring of section 2 applies;
  - the previous rows come from slot `index − 1`;
  - keepPrevious is `E+0xCA` bit 4.
- **Without a pose owner** (`FUN_140064340`, `param_4[1]` = 0):
  - `FUN_140064B30` uploads the current rows inline into b5 (`ctx->vt+0x18`): per mesh bone
    list, default bone or whole skeleton, as above;
  - **`FUN_140064DE0` uploads the previous rows into b6 (`ctx->vt+0x20`) only when the binder's
    `+0x198` (`param_2[0x33]`) is non-null**. That field is the previous-rows pointer the caller
    supplies with the draw. When it is null, b6 is not written for that draw.
  - The writer of the binder's `+0x198` was not found statically.
- **No previous camera matrix** reaches these programmes (section 1). The row's previous object
  transform is the only previous-frame input on the model side.
- **Which draws write velocity:**
  - the material pass 1 `Velo` (the table `14473CE60`), reached by row preparation 5
    (`FUN_14005A120`: preparation 5 → `ctx+0x1C` = 1), with the `Velo` programme of the
    material (`_[A]` for alpha-tested ones);
  - **the `FUN_140057A20` row that requests preparation 5 for a model was not located** in the
    kind-2 row list read here: G-buffer {5, 3, …}, {6, 3, 8}, second G-buffer {10/11, 4, …},
    depth {4/5, 1, …}, emissive {7, …, 0x100}, forward {0x29, 0, 0x12}, AO {0x10, 9/10}. Open,
    for the second thread.
  - INFERRED: the per-owner gate is keepPrevious (a ring of 1 gives zero skinned and object
    velocity).

## 6. What writes no velocity (for a reactive mask)

INFERRED from the passes and programmes. Correction from section 7: the model velocity row
has class mask 0xFFFF, so the blended model materials of the first three rows below may
also get a `Velo` draw when their entity passes the section 7 gate. Their velocity coverage
is to be checked on a capture. The other rows draw only in effect, decal, 2D or post passes,
which have no `Velo` row:

| Group | Members |
|---|---|
| Translucent and blended materials (class 2, forward 0x29) | `GXFlver_ColDifSpcBumpTrIbl_Bloody[_Cloth]`, every `_al`/blend 2 material (hair and eyebrow cards `P[ChrCustom_Hair]_al`, `C[ARSN]_al*`), additive blend 4 (`c2030`/`c2040`), `…EmiIblGlow…` (blend 2, no depth write) |
| Ghost and phantom | `GXFlver_ColDifSpcBumpEmiMulIblGhost` (blend 2); every phantom draw (`<shader>_Phantom_Fwd` variants exist for the forward pass only, MENUS-MAP section 26) |
| Veils | `Veill`, `Veill_NoLight` (blend 2/3, depth write off, `ForceForward`) |
| Water | `WatDpt` (row 0x24) and the surface (row 0x27, forward) |
| Effects | the FFX families E01–E08 (billboards, trails, model particles, distortion, radial blur, GPU particles, light shafts), the `GlowD` twins, object- and character-attached effects (I-0068) |
| Decals | deferred `Decal_*` boxes, blood decals |
| Screen-space and 2D | SSS stage, SSAO, fog, Scaleform HUD and menus, `SprjFadeImp` plates, YEBIS post |
| Cloth (partial) | the cloth `Velo` programme reads `VC_World` when `VC_isEnableCloth` is set (MENUS-MAP section 57). INFERRED: the simulated vertex motion itself is not in the velocity, only the object's |

## 7. The velocity row and its gate (`FUN_140057A20`)

**The row:** after the per-kind rows (label `140058D25`), `FUN_140057A20` records one more row
through `FUN_14005C060`, the single-row helper:
- engine pass **0x34**, preparation **5** (`Velo`), class mask **0xFFFF**, end 0xFF;
- sort key `param_7`.

**It is recorded only when all of these hold:**

| # | Condition | Source |
|---|---|---|
| 1 | **`view+0x595` ≠ 0** | the view (`param_2`) has velocity enabled |
| 2 | **draw-struct `+0xD` ≠ 0** | `FUN_140059040`/`FUN_140059130` copy it to the descriptor's byte 1; `FUN_140057A20` tests both |
| 3 | the group `iStack_4d8` is neither 0x1B nor 0x1F | `(group − 0x1B) & ~4` ≠ 0 |
| 4 | **`[[144961B50]+0x5360]+0x21` ≠ 0** | the current quality-settings object: motion blur on |

**Writer of draw-struct `+0xD` for a `SprjModelDrawEntity`** (`FUN_140D079C0`, the struct at
`RSP+0x60`; `140D07B47..140D07B8D`):

`+0xD = E+0xBA5 && E+0xBA6 && E+0x184`

- `E+0xBA5` and `E+0xBA6` are both set from `E+0xCA` bit 4 at construction (`FUN_140D05C80`).
  It is the same bit as keepPrevious.
- INFERRED: `E+0x184` is a per-frame "may write velocity" flag. Its writer was not read.

So an entity writes velocity only when its view, its keepPrevious bit, its `+0x184` and the
quality setting all allow it. The class mask 0xFFFF admits every material class: blended
classes too. INFERRED: the blended materials' `Velo` programmes alpha-test only.

## 8. The rigid previous rows: `GXFlverModelDC+0x198`

- The binder object of `FUN_140064340` is the **`GXSR::GXFlverModelDC`** model draw context
  (vtable `143D5C488`; bases `GXFLV::GXFlverModelDCBase` `143D5C4E8` and `GXBS::GXModelDC`
  `143D5C548`; base constructor `FUN_140236290`, which sets `+0x198` to 0).
- **Its slots:**

| Slot | Function | Role |
|---|---|---|
| 3 | `FUN_1401A9030` | inline current rows (b5) |
| 4 | **`FUN_1401A9140`** | inline previous rows (b6) |
| 5 | `FUN_1401A9250` | bind the b5 buffer |
| 6 | `FUN_1401A9310` | bind the b6 buffer |
| 7, 8 | `FUN_1401A9340`, `FUN_1401A9390` | the auxiliary and cancel pairs |

- **Inline uploads use pooled constant buffers by row count:** `[[144961B50]+0x4DD0]+0x3A8 +
  8·k`, with k = 0 for ≤ 2 rows, 1 for ≤ 4, 2 for ≤ 16, and 3 or 4 above (4 when more than
  64), through `UpdateSubresource`.
- **`DC+0x198` (the previous rows pointer of a draw without a pose owner):**
  - its writer was not found statically;
  - every static store to `+0x198` in `0x140000000..0x140300000` belongs to other classes
    (scissor rectangles `+0x18C..+0x19C`, resource initialisers), except the
    copy-constructor-like `FUN_14006F920` and the base constructor's zero;
  - INFERRED: it is set from the draw record by the draw replay;
  - **runtime check:** a write breakpoint on `DC+0x198` (`DC` = the `GXFlverModelDC` that
    `FUN_140064340` receives in `RDI`) during a velocity draw of a rigid object.


Exports: `decompiled/DS3-1152-F-00015490`, `00015FC0`, `00016000`, `00016050`, `00015B10`,
`00064340`, `00064B30`, `00064DE0`, `0005C060`, `001A9140`, `00236290`.
