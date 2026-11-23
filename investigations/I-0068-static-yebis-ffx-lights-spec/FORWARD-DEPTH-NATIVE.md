# I-0068 part N: forward depth writes, ready to code

Request (coordinator, 2026-10-10), first open point left by the menus workstream (Native1182: "0x1F
depth not written"):

- In the original, which forward passes (0x1F, 0x29, 0x21…) write depth?
- With which depth-stencil states, and in which order?
- Who reads the depth afterwards (soft particles t3, DoF, light shafts)?
- What must native do?

Labels: STATIC, OBSERVED, INFERRED. Evidence: `evidence/I-0068/queries/depth/`:

- `depth-draws-i63-live-10.tsv`: every depth-bound draw of the five retained witness captures,
  with its state, targets, depth SRVs and topology (`Get-DepthStateDraws.ps1`);
- `depth-classes-per-frame.tsv`;
- decompile indexes.

## 1. The rule (STATIC)

### 1.1 The GX depth-stencil bank (`FUN_1400D21F0`, `[[4961AB0]+0x2A0]`)

The GX descriptor is DepthEnable, write (GX 1 = ALL), function (GX n → D3D n+1, `FUN_141929EF0`),
StencilEnable.

| Entry | Depth | Write | Function | Stencil |
|---|---|---|---|---|
| `+0x1D8` | off | — | ALWAYS | off |
| **`+0x1E0`** | on | **ALL** | **LESS_EQUAL** | off |
| **`+0x1E8`** | on | **ZERO** | **LESS_EQUAL** | off |
| `+0x1F0` | on | ZERO | LESS | off |
| `+0x1F8` | on | ALL | ALWAYS | off |
| `+0x200` | off | — | ALWAYS | on (ref, mask 0xFF) |

### 1.2 Model draws: the material pass "apply" (`FUN_140140680` / `FUN_1401407D0` → common tail `FUN_140140850`)

- Reference1114 witnessed this apply for shader passes 0 `Fwd` and 6 `Gbuf`.
- `param_1` = the material pass object, `param_3` = the GX model-draw arguments (constructed by
  `FUN_140236290`, which the E03 builder `FUN_1401A8580` also uses), `lVar7` = the material
  parameter block (`param_1 vt+0x38()`).

```
if ((pass flags param_1+0x18 & 0x10D) == 0 || (args+0x40 bit 17)) && !(args+0x18C bit 10):
    mode = args+0x188                                   // FUN_140236290 sets −1 by default
    DS   = (mode == 1 || (mode != 0 && (lVar7 == 0 || lVar7+0x81 == 0))) ? bank+0x1E0 : bank+0x1E8
else:
    DS   = G-buffer stencil states [GXRS+0x4DE8]+0x868 / +0x860 (by lVar7+0x111 and mode)
```

- **Default (`mode = −1`): depth is written unless the material's byte `+0x81` is set.**
- INFERRED: `+0x81` is the MTD built-in `g_GX_bDisableDepthWrite` (A7E40 kind 20). Native already
  reads it as `policy.disable_depth_write`, `native/content/model_parameters.cpp`.
- `mode = 0` forces no write and `mode = 1` forces a write. Callers set them through `args+0x188`
  (writers listed in `queries/depth/d6-index.txt`).
- The forward family (`+0x18C` 2240/2176 in Reference1114, bit 10 clear) takes the first branch
  (INFERRED: forward has no stencil marking).
- **Hair MTDs** (I-0067 `mtd-hair-face.tsv`): `P[ChrCustom_Hair]` (alpha-tested, the 0x1F row),
  `P[ChrCustom_Hair]_al` and `_al_Cloth` (alpha-blended, the 0x29 row), eyes, face and skin all
  have **`g_GX_bDisableDepthWrite=False`**, so the original writes their depth. Only
  `C[ARSN]_al_hair` disables it.

### 1.3 Effects (part H)

- FFX slot 27 uses `+0x1E0` (write) for pass type 1 (**GX pass 0x21**) and `+0x1E8` (test only)
  for every other effect pass (0x20, 0x29, 0x2D, 0x31).
- E07 particles use `+0x1E8` (part M).

## 2. What the captures show (OBSERVED, i63-live-10 `<zone>-s3/frame-60`, 3 frames each, per frame)

Draws with the depth target bound, by targets and state (`depth-classes-per-frame.tsv`):

| Class | m31 | m37 | m45 | m50 | m51 |
|---|---|---|---|---|---|
| G-buffer (6 targets), test + write + stencil | 96 | 265 | 71 | 91 | 79 |
| **two RGBA16F targets (lit HDR + second), triangle list, test + write (LESS_EQUAL), no stencil** | **20** | **24** | **20** | **21** | **21** (+35 with stencil) |
| same targets, triangle list, test only | 5.7 | 6 | 4 | 8 | 4 |
| same targets, patch list (FFX tessellated), test only, reading a depth SRV | 5 | 62 | 10 | 5 | 5 (+1 writing) |

- The forward model rows are the triangle-list draws into the two RGBA16F targets (INFERRED from
  the targets and the topology; the programme bytecode of these captures was removed).
  **About four out of five of them write depth in the original.** The rest are materials with
  `g_GX_bDisableDepthWrite` or a forced mode 0.
- **A depth copy exists.** Each zone has a second 1280×720 `R32G8X24_TYPELESS` texture (bind
  DS|SRV), filled by **`CopyResource(copy ← main depth)` three times per frame**.
- The draws that sample depth read that copy, never the bound depth target. They are:
  - the FFX patch draws (t3 for soft particles);
  - the forward test-only draws;
  - the deferred local lights (the R11G11B10 pairs).
- Within the recorded command lists, a copy follows the forward depth-writing draws and precedes
  the draws that read it (for example ctx …391728: forward write → copy → half-resolution
  depth → forward test-only → FFX reading the copy). The global order of the lists is not
  recorded (INFERRED from the per-list order).

## 3. Readers of the forward depth

| Reader | Input | When | Effect of a missing forward depth |
|---|---|---|---|
| FFX soft particles (`soft*` programmes, E04/E05 soft variants) | t3 = `[GXRS+0x4E10]+0x48`, the depth copy (FFX-FAMILIES-PHASE2 §2.3) | effect passes after the copy | particles are not faded or occluded against hair and other forward meshes |
| forward test-only materials (soft forward) | the depth copy | same pass, after the copy | — |
| E07 GPU particles | depth map in the update CS (`g_csDepthMap`, collisions) and soft PS t3 | pass 0x45 and the draw passes | particles pass through forward meshes |
| YEBIS depth of field | depth → blur factor (draw 300, YEBIS-TECHNIQUES §4 row 1) | after the scene | forward meshes take the background's blur (hair blurred like the far wall) |
| YEBIS light shafts | depth → scene alpha (`Alpha_<ch>`) | after the scene | shafts leak through forward meshes |
| deferred lighting, SSAO | the G-buffer depth | before the forward rows | unaffected |

## 4. Native contract

1. **State per forward draw.** For model rows 0x1F and 0x29:
   - LESS_EQUAL, depth test on;
   - **write when `args+0x188 == 1`, or when it is −1/other and `!policy.disable_depth_write`**;
     no write when it is 0;
   - stencil off.
   - This replaces Native1181/1182's explicit open policy (always no write).
2. **Effects:** FFX pass type 1 (pass 0x21) writes depth (`+0x1E0`); the others test only.
3. **Resource state:** the scene depth is DEPTH_WRITE during the forward rows. It is no longer
   DEPTH_READ for the whole forward pass.
4. **Depth copy after the writes.** Keep the original's two resources:
   - the bound depth;
   - a readable copy at the same size and format.
   - Refresh the copy after the depth-writing forward draws (0x1F, then the writing 0x29 draws),
     and before the first draw that samples depth (FFX soft, E07, test-only forward).
   - Feed the same copy to YEBIS (DoF, light shafts).
   - Draws never sample the depth they write: the copy also avoids a DEPTH_WRITE/SRV conflict.
5. **Order:**
   1. G-buffer;
   2. deferred lighting (reads the G-buffer depth);
   3. forward row 0x1F (writes);
   4. forward row 0x29 (writes or not per material);
   5. depth copy;
   6. effect passes (0x20/0x21 writing for type 1, 0x29/0x2D/0x31 test only);
   7. YEBIS.
   - INFERRED: the original's 0x29 rows that read the copy are drawn after it. Native keeps
     0x1F ahead of 0x29, as already implemented.

## 5. Checks

| # | Check | Pass |
|---|---|---|
| D1 | forward draws per frame at the five witness zones, split by write/no-write | about 20–24 writing, 4–8 test-only per frame, as in §2 (same views) |
| D2 | Argo's hair in the Load Game portrait and in game (High Wall): depth at a hair texel | equals the hair surface depth, not the background's (readback of the native depth against the Reference run at the same texel) |
| D3 | soft particles in front of hair or a cloak | faded at the hair silhouette as in the original (I-0063 m37 capture view) |
| D4 | DoF at a portrait or close-up | hair sharp with the face, not blurred with the background |
| D5 | debug layer | no DEPTH_WRITE/SRV conflict; one copy per point, three per frame as in the original or fewer if the native merges them (log the count) |

## 6. Risks

1. **Wrong material byte.** If `+0x81` is not the MTD flag, opaque-looking veils could start
   writing depth. Mitigation: D1's split must match the original per zone.
2. **Copy too early.** A copy before the 0x1F/0x29 writes reproduces today's defect for every
   depth reader. A copy too late breaks forward materials that read it in the same pass.
3. **Alpha-blended hair writing depth** (`_al`, write on in the original) can hide later
   transparent draws behind it. That is the original behaviour; keep the original sort order.
4. **Mode overrides.** The callers that set `args+0x188` to 0 or 1 (`d6-index.txt`) were not
   all read. A native path that ignores them writes where the original does not.

## Exports

`decompiled/`: `00140680`, `00140850`, `000D21F0`, `00236290`, `001A8580` (existing).
