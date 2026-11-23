# I-0068 part K: E03 model particles ready to code, and what they lack to enter the native FLVER path

Request (coordinator, 2026-10-10): the remaining effect families in witness order, starting with
E03. Model particles go through the native FLVER path; say what is missing for them to enter
it. The format follows [EFFECTS-PHASE2-E01-E02-NATIVE](EFFECTS-PHASE2-E01-E02-NATIVE.md).

Prerequisites:

- effects phase 1 runs (FFX-SCENECTRL-NATIVE, EFFECTS-PHASE1-NATIVE);
- the phase-2 pass machinery of EFFECTS-PHASE2-E01-E02-NATIVE §3–§5: the native pass points,
  native `GXFfxDrawPass`, and the transcribed `FUN_140299E40`.

Labels: STATIC, INFERRED, OBSERVED. Evidence: `evidence/I-0068/queries/ffx/e03/`.

## 1. The machinery (STATIC)

E03 covers FXR actions 605 and 10015.

1. **SceneCtrl slot 9 `FUN_14024F940` (the factory).** It allocates a 0x30
   **`GXFFX::GXFfxFlverModelRenderer`** (vtable `143D6A298`) per model-particle instance:
   - `+0x08` allocator;
   - `+0x10` / `+0x18` init arguments;
   - `+0x20` model data;
   - `+0x28` model instance.
2. **Renderer slot +0x08 init `FUN_14029AC40(this, ctx, modelResource, …)`:**
   - **model data** `GXFfxFlverModelData` (0x78, vtable `143D6A958`, `FUN_1402BAAE0`), then
     `FUN_1402BAC10(data, modelResource)`. That creates up to two **model adapters**
     `GXFfxFlverModelDataAdapter` (0x30, vtable `143D6A988`, `FUN_1402BFCC0`):
     - `+0x10` = the GX FLVER model resource;
     - `+0x20` = a skeleton helper (0x38, vtable `143D6AA78`, `FUN_1402C04F0`), built from the
       resource's `+0x10/+0x38`;
   - **model instance** (0xC0, vtable `143D6A940`, `FUN_1402B9E90`):
     - `+0x10..+0x4F` world matrix (identity at construction);
     - `+0x50..+0x68` the adapter references (refcounted; index = LOD/variant);
     - `+0x70` the bone matrix array (count × 0x30) from the allocator;
     - `+0x78` dirty flag;
     - `+0x80` the pose block (`FUN_1402BF6A0`, bone count = `[adapter vt+0x40()]+0x18`);
     - `+0x90` count;
     - `+0xA8`;
   - `FUN_1402BA030` binds the adapters and allocates the bone array.
3. **Per frame, CPU (library calls):**
   - slot +0x28 `FUN_14029B2C0`: new world matrix → instance `+0x10..+0x4F`, then the bone
     matrices;
   - slot +0x30 `FUN_14029B300`: refresh the pose flags, then adapter `vt+0x38` (bounds
     `FUN_1402C0200`);
   - slot +0x18 `FUN_14029ADE0`: instance `vt+8`.
4. **Draw: renderer slot +0x20 `FUN_14029ADF0(this, passCtx, fxrParams)`.**
   1. Build the **400-byte draw state** (`FUN_14004DF20` default, then the FXR fields
      `fxrParams+0x10..+0x98`: colour, multipliers, flags `+0x30`, optional overrides at
      `+0x38..+0x4C` when not −1).
   2. `FUN_1401A8580(args, passCtx+0x750, passCtx+0x460, state)`: the GX model-draw arguments
      (vtable `143D5C488`, the 400-byte state copied at `args+0x240`, byte `args+0x425` from
      state `+0x87`).
   3. **`FUN_140059470(passCtx, args, …)`:** the per-draw scene constants. GX: three
      `UpdateSubresource`, VS/PS constant-buffer binds. It reads the camera at
      `passCtx+0x418`, which also identifies `param_2` as the render-pass context
      (INFERRED).
   4. `FUN_1401A86C0(args)`: the argument block's commit (two `UpdateSubresource`, VS/PS
      binds; GX).
   5. **`FUN_1402BA3A0(instance, args, 0)`:**
      - CPU: bone world matrices = instance world × each pose bone, written as 3×4 rows
        (0x30 per bone) into a frame-temporary array;
      - then **adapter `vt+0x10(adapter, args, bones)` = `FUN_1402C0130`**.
5. **Adapter draw `FUN_1402C0130` → `FUN_1400636F0(&{143D42000, 14473F6C0}, model, args, bones)`.**
   - It returns when the model is not loaded (`model+0x2B4 == 0`).
   - It clamps the LOD `args+0x18` to `model+0x1E4 − 1`.
   - Material table = `args+0x1A8`, or else `model+0x1F0`.
   - Then **`FUN_140063CB0(…, model, materialTable, args)`**: the GX FLVER mesh/material loop.

**Where it meets the world path (STATIC):**

- World entities enter at `FUN_140D079C0` → **`FUN_140059040`** (11 arguments) → `FUN_1400575C0`
  + `FUN_140057A20` → bundle `FUN_14005C0E0`. That is the entry the native selection reads
  (I-0066 DIRECT-MODEL-INPUTS, cycles 732–733).
- E03 enters **one level lower**: `FUN_1400636F0` → `FUN_140063CB0`, the mesh loop whose material
  id `+0`, flags `+0x10`, LOD bits `+0x14`, material lookup and pass/category/mask tests the
  native selection already verified (same note).
- The other caller of `FUN_1400636F0` is `FUN_140D2CA00` (not an FFX path; not read).

## 2. What E03 lacks to enter the native FLVER path

| # | Missing piece | Why (STATIC) | Native contract |
|---|---|---|---|
| M1 | **An entry at `FUN_1400636F0`** (or at the adapter slot `FUN_1402C0130`) | the native selection is fed from the `D079C0 → 59040` arguments (entity `+0xD0`, `[[entity+0xE8]+8]` model, pose `entity+0xA80`, bounds, sorting). E03 has no entity: it passes `{model resource, args, bones}` straight to the mesh loop | **NATIVE** `NativeFfxModelSubmit(model, args, bones)`. The detour runs the same model-loaded and LOD clamp tests, then hands the native selection a record `{model resource, LOD = args+0x18, material table = args+0x1A8 or model+0x1F0, state = args+0x240 (400 B), bone palette, pass = passCtx+0x124}` instead of calling `FUN_140063CB0` |
| M2 | **Model storage for FFX-owned resources** | the resource reaches the renderer through the FFX resource cache (FXR resolution, `sc` resource manager `+0x160`), not through a map or character entity | the native registry must own the FLVER of each resource `FUN_1400636F0` receives. Look it up by the resource pointer. If the FFX bundle FLVERs are not ingested, ingest them through the same native FLVER ingress when the FFX resource manager resolves them (INFERRED; check E1) |
| M3 | **The pose palette** | the world path passes the entity pose `entity+0xA80`; E03 passes a CPU array of world-space 3×4 bone rows, already including the instance world matrix (`FUN_1402BA3A0`) | the native pose-palette delivery (listed as open in DIRECT-MODEL-INPUTS) takes the array as is: count = `instance+0x90`, stride 0x30, world space. So the native instance world matrix is identity for skinned meshes, and rigid meshes use their bone 0 row (INFERRED from the mesh loop's bone indexing) |
| M4 | **The 400-byte draw state** | the same 400-byte state the world path copies (`4E180`, base opacity `+0x124`) is built here from FXR colour and alpha (§1 step 4.1), not from the entity fade | feed `args+0x240` to the native material preparation as the current state. The fade branches of `FUN_140D06E10` do not apply (there is no entity) |
| M5 | **The FFX pass** | E03 draws inside the FFX pass submission (pass ids 0x29 / 0x31 forward, 0x2D, 0x21, 0x20; `FUN_14024E790`). `FUN_140063CB0`'s pass/category/mask tests select the meshes for that pass | run the native mesh selection with the FFX pass id from `passCtx+0x124` and record into the native forward/effect encoder of that pass, sorted with the other FFX draws (the `FUN_1402997E0` key), not into the GBuffer passes |
| M6 | **Scene constants of the pass** | `FUN_140059470` and `FUN_1401A86C0` upload GX constant blocks for the model draw | the native pass's scene constants (camera from `passCtx+0x418`) and the native material constant producers. The two GX uploads are dropped (TRANSCRIBE of `FUN_14029ADF0` below) |

## 3. Ordered list

| # | Function | Mode | Native |
|---|---|---|---|
| 1 | SceneCtrl slot 9 `FUN_14024F940` (factory) | CALL | — |
| 2 | renderer init `FUN_14029AC40`, data `FUN_1402BAAE0` / `FUN_1402BAC10`, adapters `FUN_1402BFCC0`, skeleton helper `FUN_1402C04F0`, instance `FUN_1402B9E90`, bind `FUN_1402BA030` | CALL (CPU) | at init, log the model resource and look it up in the native registry (M2) |
| 3 | renderer release `FUN_14029AD60`, `FUN_1402BA170`, adapter destructor `FUN_1402BFB40` | CALL | the native registry lease is released with the adapter (refcount `FUN_1417A3610` reaching 1) |
| 4 | slots +0x18, +0x28, +0x30 (`FUN_14029ADE0`, `FUN_14029B2C0`, `FUN_14029B300`) | CALL | — |
| 5 | **slot +0x20 `FUN_14029ADF0`** | **TRANSCRIBE** | keep the 400-byte state and `FUN_1401A8580`; drop `FUN_140059470` and `FUN_1401A86C0` (GX uploads); keep `FUN_1402BA3A0` (CPU bones) |
| 6 | adapter draw `FUN_1402C0130` → **`FUN_1400636F0`** | **NATIVE** (detour `FUN_1400636F0`) | M1: native submission |
| 7 | native selection and encoding | NATIVE (existing path) | §4 |

## 4. Native draw (reuse of the native FLVER path)

- **Meshes:** the native selection of the model's meshes for LOD `args+0x18` with the material
  table, filtered by the same material/pass tests as `FUN_140063CB0`.
- **Programmes:** the material's own programmes (`gxflvershader`) for the FFX pass. In a
  forward pass that is the material's forward/translucent variant, not `Gbuf`. Use the
  recipe selection of DIRECT-MATERIAL-PREPARATION with the forward pass entry of the pass
  table.
- **Skinning:** the bone palette of M3, uploaded once per draw into the upload ring (count ×
  48 B) and bound where the material's skinned VS expects the palette.
- **States:**
  - blend from the MTD (translucent FFX models are alpha-blended or additive);
  - depth test on and write off for translucent meshes (INFERRED, as for E01);
  - the forward HDR target (+ glow target for glow materials).
- **Sort:** the FFX pass key (camera distance) of the entity that owns the renderer, so model
  particles interleave correctly with billboards.

## 5. Checks

E03 was **not witnessed live** in I-0063, so the expectations below are relative to an original
Reference run with pass-through observers (no image change), at the same views.

| # | Check | Measure | Pass |
|---|---|---|---|
| E1 | resources | every model resource seen at renderer init (§3 row 2) is found in the native registry | 100% found at m40, m51 and m45; missing ones listed by resource name |
| E2 | submissions | calls to `FUN_1400636F0` from `FUN_1402C0130` per frame | native count = original count (pass-through observer on the same function in the Reference run), at the m51 witness (highest E03 weight, 286) and m45 (172) |
| E3 | meshes | meshes drawn per submission | equal to the original mesh-loop iterations that pass the material tests (observer on `FUN_140063CB0` in the Reference run) |
| E4 | pose | bone count per submission = `instance+0x90`; palette bytes = count × 48 | equal; no skinned mesh drawn without a palette |
| E5 | image | effect-area capture at an m40 view where the Reference run shows E2 > 0 | the remaining forward difference drops at the model-particle pixels |

## 6. Risks

1. **FFX-owned FLVERs not ingested (M2).** If the native ingress only knows map, character and
   object FLVERs, every E03 submission misses. E1 is the first check.
2. **Two entries into the mesh loop.** The native selection was qualified on the
   `FUN_140059040` bundle. `FUN_1400636F0` skips `FUN_1400575C0` / `FUN_140057A20`, so any state
   those set (patch association, default patch) is absent for E03. Treat the missing patch as
   the "no patch" branch the native note already allows.
3. **Pass mismatch.** If the native material recipe has no forward entry for a mesh that the
   original draws in the FFX pass, the mesh disappears. Log unsupported recipes per pass.
4. **Palette space.** The bones are world-space and include the instance transform. Applying
   the instance world matrix again doubles the transform.

## Exports

`decompiled/`: `0024F940`, `0029AC40`, `0029AD60`, `0029ADE0`, `0029ADF0`, `0029B2C0`,
`0029B300`, `002BAAE0`, `002BAC10`, `002B9E90`, `002BA030`, `002BA170`, `002BA3A0`,
`002BFCC0`, `002C0130`, `002C01A0`, `002C0200`, `002C0270`, `002C04F0`, `001A8580`,
`001A86C0`, `00059470`, `000636F0`, `00063910`. Existing: `00059040`, `00063CB0`,
`00057A20`, `00D079C0`.
