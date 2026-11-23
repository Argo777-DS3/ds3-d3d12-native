# I-0068 part O: the generic shadow pass, ready to code (atlas, cascades, ShadowParam, skinned casters, lighting)

Request (coordinator, 2026-10-10), second open point after the menus workstream:

- prepare the generic shadow pass in "ready to code" form: atlas, cascades, ShadowParam, skinned
  casters (player and NPCs), and the reading in the lighting;
- check whether native already draws the world's shadows, and what is missing for characters.

MENUS-MAP §48 (Player menu views) is the menu case of the same machinery and is referenced, not
repeated.

Labels: STATIC, OBSERVED, INFERRED. Evidence: `evidence/I-0068/queries/shadow/`:

- `shadow-atlas-draws-i63-live-10.tsv`: caster draws per atlas, viewport and state in the five
  retained witness captures (`Get-ShadowAtlasDraws.ps1`);
- decompile indexes;
- the programme interfaces quoted below come from I-0063's `program-resources.tsv` (DXBC
  reflection).

> **Part BN (SHADOWS-WAVE1-READY):** the cascade fit, culling volume and constants are pure CPU functions; the atlas edge follows the quality row (§0.3 there); the local atlas edge and tile bases are configuration values (defaults 1024 / 512 / 256; `FUN_1400B24E0`, `140048B80`); slot 3 of the light classes acts on per-frame items built by `212810`/`2136B0` under the gate of `FUN_14010A220`; the container `[GXRS+0x41D8]` is already built natively. Where this note differs, the new sheet wins.

## 0. Native status today (checked on `main`, native code read)

| Part | Production native (`native/frame`, `native/renderer`) | Reusable pieces elsewhere |
|---|---|---|
| shadow atlas, cascades, caster draws | **absent**: no atlas resource, no depth-only pass, no caster selection | — |
| screen-space sun shadow / composite mask t32 | **neutral white** (explicit open policy, `model_frame.h`); `scene_pass.cpp` has a "paired shadow mask" pipeline fed only by offline reference inputs (`scene_pass.h`: "until native shadow producers exist") | `native/world/light_producers.cpp`: `cbCompositeShadowMask` (10CC20), the shadow sub-block of `cbSceneParam` from the ShadowParam record (`ShadowFieldsFromBanks`, I-0045), `cbPerScene` (1D6D20, `FC_ProjSpaceToShadowMatrix`), `cbLightAccShadowInfo` for shadowed point and spot lights |
| sun light | `GXLightAcc_DDirLight1`, unshadowed (the engine would select `DDirLightShadowed1[_Cloud]`) | — |
| point/spot shadows | none | as above (host-era producers) |
| shadow cameras | none | I-0064: `cbMatrixWVP` and `cbSceneParam` of 13AA60's views (main camera and sun cascades), point-light cube faces (10C7E0) |
| characters | drawn in the G-buffer, skinned from engine palettes (K-0404, Native1104–1112) | the same palettes serve the casters |

**So native draws no world shadows and no character shadows.** The scene is lit as if fully
unshadowed (t32 = 1).

## 1. The original pipeline (STATIC + OBSERVED)

Frame order (K-0407, OBSERVED):

1. **cascaded shadow atlas**;
2. G-buffer;
3. depth downsample;
4. SSAO;
5. **screen-space sun shadow** (+ blur, + composite with SSAO);
6. env-map regions;
7. `GXLightDeferredAO`;
8. sun;
9. `GXLightOrganize`;
10. point/spot lights (**shadowed variants read the local-light atlas**);
11. `CmbAlbedo`, SSS, Composite;
12. forward, post.

### 1.1 Resources (STATIC `FUN_1401B5390`, `FUN_1401B5720`; OBSERVED sizes)

**Cascaded shadow object** (the pipeline's shadow member; `FUN_1401B5390(obj, desc)`):

- `+0x10`: the atlas target, created through `FUN_1402383C0` with `{w, h}` = `+0xC00/+0xC04`
  (halved when desc byte `+9` is set);
- `+0x140..+0x158`: **four cascade sub-targets** (0x130 objects, `FUN_1402381B0`). A fifth at
  `+0x160` exists only with desc byte `+9`;
- `+0xA30`: a 0x1B0-byte constant buffer, **`cbPerScene`**;
- `+0xA38..+0xA50`: **four rasterizer states, one per cascade**, each with the depth bias of its
  cascade record (`+0x91C`, stride 0x50);
- `+0xC18 = 1`: ready.

**Size** (`FUN_1401B5720`, from the ShadowParam record):

- the record's size class `+0x90` gives the per-cascade size: 0 → 2048, 1 → 1024, 2 → 2048,
  3 → 4096 (int constants);
- `×[14473DAF8]` when `obj+0xC09` (INFERRED ×2);
- then the global quality: `size·[shadow+0x2C]/[+0x30]`, clamped to `[+0x34, +0x38]`
  (`[GXRS+0x5360]`);
- a change re-creates the atlas (`FUN_1401B55F0` then `FUN_1401B5390`).

**OBSERVED** (i63-live-10, Thomas's settings):

| Atlas | Format | Use |
|---|---|---|
| **8192×8192** | `R16_TYPELESS` (D16 depth, R16 SRV) | **four 4096×4096 quadrants = the four sun cascades** (viewports (0,0), (4096,0), (0,4096), (4096,4096)); I-0045's "shadow-map size 8192" |
| **4096×4096** | `R16_TYPELESS` | **512×512 tiles**: one per shadowed local light (point cube faces 10C7E0 / spot); 1–3 tiles in use per witness frame |

- INFERRED mapping to MENUS-MAP §48's fields: the pipeline's `+0x1520` atlas and the
  double-size `+0x2160` one. This has not been read for the world pipeline.

### 1.2 ShadowParam and cascades (STATIC)

1. **Record.** `FUN_140088AF0`/`FUN_140088A10` choose the ShadowParam keys (world: `{map area, 0}`
   at map load, I-0045; menus: `{1, id}`). They blend the two bank records (`[GXRS+0x4670]`,
   `FUN_14018C590`) into the pipeline (`FUN_1401DAF40`, which also computes the Angle
   direction).
2. **Clamps** (`FUN_1401B5720`): Near `+0x34` → state `+0xA70`. The three splits `+0x38/+0x3C/+0x40`
   (+`+0x44/+0x48`) are forced increasing with a minimum gap (`143D7ED70`) into `+0xA74..+0xA80`.
   Then the far fade, filter, bias, depth rate and kernel fields (MENUS-MAP §48 table for
   their names and menu values).
3. **Cascade fit** (`FUN_1401B6B80`, `FUN_14023A0C0`): per cascade a light view-projection fitted
   to the camera slice. It produces the cascaded-shadow state (`+0xA90`, `+0xAB0`, `+0xAC0`…
   copied into `cbSceneParam +0x1C0..` by `6F920`; native comments in
   `light_producers.cpp`).
4. **Direction** (`FUN_1400F90F0`, MENUS-MAP §48 table):
   - Shadow Light Id −1 → the ShadowParam Angle;
   - otherwise the light set's light 0, or the blend of two light sets.
   - Built only when the pipeline has its shadow flag, `[[GXRS+0x5360]+0x28]` (shadows on) and
     GX is not paused.
5. **Constants:**
   - `cbPerScene` (1D6D20): per cascade `FC_ProjSpaceToShadowMatrix` and the slice data;
   - each cascade view's `cbSceneParam` and `cbMatrixWVP` (13AA60's views, I-0064 producer:
     the view pose and the inverse rotation times projection).

### 1.3 Casters (OBSERVED + STATIC)

- **Programmes** (shader pass 2 `Dpt`, 3 `DptA` of the pass table, Reference1114): VS
  `<family>_Dpt[_Skin][_Ins][@[cl]]`, and for alpha-tested materials PS `<family>_DptA`.
  - **VS interface:** b0 `cbMatrixWVP`, b4 `cbInstanceData`, **b5 `cbObjMatrix` (the skin
    palette)**, b8 `cbSceneParam`; cloth adds b2 `cbMatDynParam`, b7 `cbvModelParam`.
  - **`DptA` PS:** t0 diffuse (alpha test), t38 dither, b1 tiling, b2, b3 `cbMtdParam`, b4, b7.
  - Generic fall-backs: `GXFlver_Shadow[A]_Dpt[_Skin][_Ins]`.
- **Drawn every frame at m40** (I-0063 i63-live-01, 1,800 of 1,800 frames):
  - `C_ARSN_cinder_Dpt_Skin` / `…_DptA_Skin` (the player's body family);
  - `SAT_Equip_cloth_DptA_Skin` and `SAT_Equip_DptA`;
  - `GXFlver_HairIbl_Bloody_DptA`;
  - `GXFlver_…_Dpt_Skin@[cl]`, `GXFlver_…_Dpt@[cl]` (rigid).
  - **Characters cast.**
- **States:**
  - depth test and write, LESS_EQUAL, stencil on (`1/1/4/1`; a few `1/1/4/0`);
  - triangle list;
  - one viewport per cascade quadrant or light tile;
  - the cascade's rasterizer with bias (§1.1).
- **Per frame and view** (`shadow-atlas-draws-i63-live-10.tsv`, three frames each):

| Zone | Sun cascades (4096² quadrants): depth-only + alpha-tested draws | Light tiles (512²) |
|---|---|---|
| m31 | (0,0) 22+8, (4096,0) 22+8, (0,4096) 0+3, (4096,4096) 0+3 | 1 tile, 22+5 |
| m37 | (0,0) 5, (0,4096) 33+5, (4096,0) 30+5, (4096,4096) 26 | 2 tiles, 22+5 each |
| m45 | (4096,0) 23+5, (0,4096) 1, (4096,4096) 2 | 3 tiles, 22+5 each |
| m50 | (0,0) 22+5, (4096,0) 22+5 | 2 tiles, 22+5 each |
| m51 | (4096,0) 22+5 | 2 tiles, 22+5 each |

- **The set "22 depth-only + 5 alpha-tested" repeats in every view that contains it.**
  INFERRED: the player character, whose 27 meshes are cast into every cascade and light tile
  that sees them. Map casters appear on top in some cascades (m37).
- Static map shadowing is not re-drawn every frame: the cascade cache of §1.4 explains how.
- **Submission** (STATIC, partial):
  - the cascade views are scene views (13AA60's views) whose GX passes call the model draw
    `FUN_140057A20`;
  - for those pass ids its row is the default `FUN_14005C060(…, pass, …, 0xFFFF)` (INFERRED: the
    shadow passes' ids were not enumerated);
  - the material apply `FUN_140140680` then picks the Dpt/DptA recipe.

### 1.4 The cascade cache and `GXRend_CopyShadowMapDepth` (STATIC; part P, 2026-10-10)

> **Observed 2026-10-16 (SUN-SHADOWS-WAVE1 §7):** inactive as shipped. Reference1786-1789 (m40, m30, m31): every cascade is
> recorded every frame, `FUN_1401B5B40` is never called, `+0xC09` = 0. The native redraws every cascade every frame, as the original.

**Where the copy programme lives.**

- `GXRend_CopyShadowMapDepth` is field `+0x38` of the deferred-lighting programme set
  `[[GXRS+0x4DE8]]`, loaded by `FUN_1401010A0`.
- The set: `+0x00` `GXLightCmbAlbedo`, `+0x08` `GXLightComposite`, `+0x10` `GXLightDeferredAO`,
  `+0x18` `…WithSsr`, `+0x20/+0x28` debug, `+0x30` `GXRend_CompositeShadowMask`, **`+0x38`
  `GXRend_CopyShadowMapDepth`**, `+0x40` `GXEnvDecal`.
- Its only draw site is `FUN_140108430(set, ctx, cascade, source, oldViewProj, newViewProj)`:
  - full-screen, **depth state `+0x1F8` (ALWAYS, write)**, raster `+0x208`;
  - target = the cascade's depth view (`cascade+0x118`, else `+0x100`);
  - PS t1 = `source`, b6 `cbCopyShadowMapDepth` from the two matrices;
  - the PS writes the source depth reprojected from the old cascade projection to the new one.

**Per frame and per cascade** (`FUN_1400F9B50` loop over the 4 cascades, cascaded-shadow object
`obj`):

1. **Invalidation of all cascades:** when the shadow direction of this frame differs from the
   previous one (component compare against `143D7F960`), `obj+0xC1D+i = 0` for i = 0..3.
2. If `obj+0xC1D+i == 0` (no usable content): **full record** of the cascade pass
   (`FUN_1401B6020`).
3. Otherwise **`FUN_1401B5B40(obj, i, camera)`** decides whether to keep:
   - a forced invalidation byte `[14496231C + i]` makes it return "redo" and clears the byte
     (the writer was not found statically);
   - else, when the cascade has content (`obj+0xC19+i`), the 8 corners of the current camera
     slice are transformed into the previous cascade's projection, and each is tested against
     the unit volume;
   - INFERRED from the decompiled part (its tail was not recovered): "keep" when all corners
     stay inside;
   - **keep → no pass is recorded for that cascade this frame**: its depth from earlier frames
     is reused as is;
   - redo → full record.
4. **Full record** `FUN_1401B6020` (cascade i, GX pass id i+1), command stream of the cascade pass:
   - sub-pass 1: callback `FUN_1401B6B80` cmd 0 (cascade camera from the record);
   - sub-pass 3: cmd 1 (viewport, target, samplers);
   - **with `obj+0xC09`: cmd 4 = clear the cascade depth, then, if the record's byte `+0x1A`
     is set, draw `GXRend_CopyShadowMapDepth` from the record's source (`+0x20`)**;
   - sub-pass 5: the casters (scene graph);
   - sub-pass 0x3C: cmd 2 (end) and cmd 3 (the `cbPerScene` upload and the `cbSceneParam`
     shadow block, `6F920` into `[GXRS+0x4DD0]+0x5E0`);
   - at the end, `obj+0xC1D+i = 1` (usable next frame).
5. **Reprojection mode**, when `DAT_14473DAD4 && obj+0xC09 && obj+0xC1D+i` at record time:
   - the cascade's texture and the spare "cache" target `obj+0x160` are **swapped**
     (`FUN_1401B7FF0`, refcounted handles), so the previous frame's cascade depth becomes the
     source `+0x20`;
   - the record also carries the previous cascade matrices (`+0x30..+0xAF`);
   - cmd 4 then copies that depth, reprojected, into the freshly cleared cascade;
   - the cascade culling volume `obj+0x570+i·0xE0` receives, from `FUN_1401B4AC0`, the
     **inverted planes of the previous cascade volume** (flag `+0x640+i·0xE0 = 1`).
   - INFERRED: static casters wholly inside the previous volume are culled, since their depth
     is already in the copy. Casters outside it, and the dynamic ones (player, NPCs), are drawn
     on top.
   - `obj+0xC09` is the creation flag that also halves the per-cascade size and creates the
     fifth target (`FUN_1401B5390`, desc byte `+9`).

**What the observations mean.** In the witness captures, several cascades show only the player
set (22+5) or nothing. Those are cascades either kept (no pass) or redrawn in reprojection mode
(map depth from the copy, the dynamic casters redrawn).

**Native contract (closes the risk):**

- **Reference behaviour, recommended first: redraw every cascade every frame with all casters**
  (static and dynamic) and no copy.
  - This produces the depth the cache only approximates (the cached content is either
    identical, because nothing moved, or a reprojection of it). The image matches within depth
    precision.
  - It costs more caster draws (all map casters in all four cascades).
- **Optional optimisation, same structure as the original:**
  1. keep a cascade when its slice stays inside the previous volume and the direction is
     unchanged;
  2. in reprojection mode, ping-pong two depth textures per cascade;
  3. copy the old one into the new one through the original `GXRend_CopyShadowMapDepth`
     (ALWAYS/write, full-screen) with `cbCopyShadowMapDepth` from the old and new
     view-projections;
  4. cull static casters with the inverted previous volume;
  5. always draw dynamic casters.
  - **A kept cascade must still receive the dynamic casters.** INFERRED: the original keeps
    cascades only when it can (a moving player near the camera is in cascade 0, which a moving
    camera rarely keeps). Natively, prefer redrawing a cascade that contains a dynamic caster.

### 1.5 Receivers (STATIC interfaces)

| Programme | Inputs | Output |
|---|---|---|
| **`GXScreenSpaceCascadedShadow_PCF4` / `_PCF9` / `_Soft`** (VS `GXScreenSpaceShadow`) | t1 scene depth, **t32 the cascade atlas**, s12 `SS_Shadow` (comparison), s7 point clamp, (`_Soft`: t38 dither, s6), b0 **`cbPerScene`**, b8 `cbSceneParam` | screen-space shadow (by filter mode: PCF4, PCF9, soft) |
| blur | `cbBlurParam` (1D7850) | blurred mask |
| **`GXRend_CompositeShadowMask`** | t3 GBuffer1, t33 SSAO, b5 `cbCompositeShadowMask` (10CC20) | **t32 shadow × AO mask** of the lighting |
| `GXLightDeferredAO` | t6, t7, **t32** | irradiance and specular × mask |
| **`GXLightAcc_DDirLightShadowed1[_Cloud]`** | t0, t1, t3, t5, **t32**, b0, b5 `cbLight`, b8 (`_Cloud`: t22/t23 cloud shadows) | sun |
| `GXLightAcc_D{Point,Spot}LightShadowed{1..4}`, `Legacy…`, `SpotLightMaskShadowed1` | **t7..t10 `g_LightShadowTexture[]`** (the local atlas), s12, t33, b2 **`cbLightAccShadowInfo`** (tile rectangles) | shadowed local lights |
| forward `_Fwd` (view state `view+0x710` = 0) | **no shadow input** (part AT: the world views also use `_FwdSdw` and `_FwdSSdw` when the state is 1 or 3, TESS-SHADOW-FORWARD-NATIVE §4) | — |
| forward `_FwdSdw` (menu views, Reference1114) | t32 the view's atlas, s12 | MENUS-MAP §48 |
| particles | `CS_VFX_StandardParticleGenerateSdw` (3D texture), `PS_VFX_…Sdw` | particle shadows (E07, later) |

## 2. Ready-to-code order

| # | Step | Mode | Native |
|---|---|---|---|
| 1 | ShadowParam keys and blend (`88AF0`, `88A10`, `1DAF40`) | CALL (CPU) | — |
| 2 | sizes and clamps `FUN_1401B5720` | TRANSCRIBE | keep the CPU clamps; replace the re-creation `1B55F0`/`1B5390` by native resources |
| 3 | **atlas** | NATIVE | one `R16_TYPELESS` texture at the computed size (8192² with Thomas's settings): DSV D16_UNORM, SRV R16_UNORM; four quadrant viewports and scissors |
| 4 | cascade fit `1B6B80`, `23A0C0`, direction `F90F0`/`F9B50` | CALL | the state object fields feed §2.6 |
| 5 | per-cascade raster states (bias from `+0x91C`) | NATIVE | the PSO's DepthBias / SlopeScaledDepthBias per cascade (the bank values converted as GX converts them) |
| 6 | **cascade views' constants**: `cbMatrixWVP`, `cbSceneParam` per cascade; `cbPerScene` | NATIVE producers (port the I-0064 / I-0045 producers from `native/world`) | root CBVs |
| 7 | **casters**: for each cascade, the native model selection over the cascade's frustum with the Dpt / DptA recipes | NATIVE | §3 |
| 8 | screen-space cascaded shadow (PCF4/9/Soft by FilterMode), blur, composite with SSAO | NATIVE passes with the original programmes | t32 real instead of white |
| 9 | sun: `DDirLightShadowed1[_Cloud]` as the engine selects | NATIVE switch | — |
| 10 | local lights' tiles (the 4096² atlas, cube faces 10C7E0, spot) and the shadowed point/spot programmes | NATIVE, after the sun | `cbLightAccShadowInfo` from the existing producer |
| 11 | menu views (MENUS-MAP §48) | same code, with the view's atlas and `FwdSdw` | — |

**Pass state:**

- clear the atlas depth to 1 (LESS_EQUAL);
- depth write on, stencil as the original (`1/1/4/1`; INFERRED: the stencil is not read by the
  receivers, so it can be off natively);
- no colour target;
- states: DEPTH_WRITE for the casters → PIXEL_SHADER_RESOURCE for the screen-space pass and the
  shadowed lights.

## 3. Characters: what is missing for player and NPC casters

Native already has, from the G-buffer path:

- the meshes;
- the skin palettes (`cbObjMatrix`, K-0404);
- the cloth cancel palette;
- the material classes;
- the per-model `+0x220/+0x224` unions (Native1101–1112).

Still missing:

1. **The Dpt and DptA recipes.** Shader passes 2 and 3 of the pass table:
   - VS `<family>_Dpt_Skin[@[cl]]` and `_DptA_Skin`;
   - the DptA PS with the material's diffuse alpha test and dither;
   - recipe selection as for `Gbuf` (DIRECT-MATERIAL-PREPARATION) with the pass entry 2/3.
2. **One selection per shadow view.** Each cascade, and each light tile that contains the
   character, re-uses the character's selection with that view's `cbMatrixWVP`/`cbSceneParam`.
   The original casts the player set (27 draws) into every view that sees it.
3. **The same palettes for every view of the frame.** The pose is the frame's pose, uploaded
   once.
4. **NPCs** (MENUS-MAP §55): the same path once their G-buffer draws are native.
5. **Alpha-tested hair and cloth** (5 of the 27): DptA with the alpha reference of the MTD
   (`P[ChrCustom_Hair]` 128).

## 4. Checks

| # | Check | Pass |
|---|---|---|
| S1 | atlas | 8192×8192 R16 with four 4096 quadrants at Thomas's settings; size follows the ShadowParam class and the quality clamps |
| S2 | casters per view | per zone and quadrant, the counts of §1.3 (same views); the player set 22+5 in each view that contains the player |
| S3 | mask | the native t32 against the original mask at the same frame (I-0045's offline `light_check … shadow=` route); then the lit HDR within K-0426 with shadows on |
| S4 | character shadow | High Wall (Argo): the player's shadow on the ground and the hood's shadow on the face in the Status/Equipment Player views (MENUS-MAP §48) |
| S5 | local lights | the shadowed point/spot programmes chosen where the original chooses them, tiles as in §1.3 |
| S6 | cache equivalence | native full redraw against the original atlas at a still camera (kept cascades): depth equal within 1 D16 unit; after a camera turn (reprojection), the screen-space mask t32 equal within the PCF filter's tolerance |

## 5. Risks

1. ~~The static-shadow cache~~: closed in §1.4. Map shadows reach the atlas through redraws or
   reprojected copies of earlier frames' cascades. The native reference redraws everything.
   Remaining unknowns:
   - the writer of the forced-invalidation bytes `[14496231C+i]` (INFERRED: area or teleport
     changes);
   - the tail of `FUN_1401B5B40`;
   - the exact cull rule of the `+0x640` volume for dynamic objects.
2. **Bias.** Without the per-cascade raster bias, shadow acne or peter-panning appears; take the
   GX values, not tuned ones.
3. **Atlas format.** R16 depth means 16-bit precision. A D32 native atlas changes comparisons in
   the PCF programmes, which sample R16 UNORM.
4. **Cost.** 8192² D16 is 128 MB. Keep it at the configured size: K-0442 treats the shadow-map
   size as a setting input.
5. **The shadow pass ids** were not enumerated statically (§1.3). The native schedule is
   cascade-driven, so it does not need them, but the engine's per-pass caster culling and row
   choice (57A20) must be reproduced by the native selection.

## Part AU (2026-10-11): the world shadow system completed — layers, state, windows, matrices, mask, constants, D3D12 design, ordered steps

Request (coordinator, 2026-10-11, with Thomas's D-0157 reminder and his note to draw on Elden Ring): complete this note to prepare the world shadow system that native lacks: the shadow layers
(`FUN_1401DD490`, `FUN_1401B8F30`), the cascade atlas and its matrices, the screen-space mask, the state `view+0x710`, who writes `cbSceneParam` (cb8) and `cbTessScene`; end with ordered native steps,
witnesses and live rows. Where useful, propose a D3D12 design that changes the engine architecture (D-0157) instead of copying D3D11. Nothing was launched; native code was read, not changed.
Labels: STATIC (decompiled code, RTTI, DXBC of the cached programmes, native source at `f6a54120`), OBSERVED (cited captures), INFERRED, OPEN. Evidence (outside Git): `evidence/I-0068/queries/shadows/`.

## 6. The shadow layers (STATIC, RTTI by `I0067Rtti.java`)

The three functions are slot 0 of GX scene-renderer classes (`GXSR::`). Each is a **pass-command callback**: `(this, execContext, command{code, data})`, recorded into the pipeline's pass list at a numbered sub-pass
(`param_2+0x128`) and executed in sub-pass order.

| Class (vtable) | Callback | Lives at | Commands (code → effect) |
|---|---|---|---|
| **`GXCascadedShadowScene`** (`143D5D0D0`; ctor `1401B5010`) | `FUN_1401B6B80` | pipeline `+0x1520` (and `+0x2160`, the double-size atlas) | **0** cascade camera: fit `FUN_14023C1E0` into `+0x170 + c·0x100`, render setup `FUN_140238680`, direction → state `+0xA60`; **1** begin the cascade target (viewport, samplers; `view+0x718 =` record byte); **2** end (`FUN_140238840`, `view+0x718 = 1`, `view+0x410 = view+0x130`); **3** finalise: fit `FUN_14023A0C0`, **copy the shadow block into `[GXRS+0x4DD0]+0x5E0` (`FUN_14006F920`)** and upload `cbPerScene` (`FUN_1421397C0`); **4** clear (+ `GXRend_CopyShadowMapDepth` in reprojection mode, §1.4); **5 `view+0x710 = 1`**; **6 `view+0x710 = 0`** |
| **`GXScreenSpaceShadow`** (`143D5D238`; member of the pipeline at `+0x2DB8`, ctor in `1400843C0`; impl `GXScreenSpaceShadowImpl` `143D5EB98`) | `FUN_1401B8F30` | pipeline `+0x2DB8`; mask targets at layer `+0x08` and `+0x10` | **0** draw the mask: mode word 0 = clear to 1.0 (no shadow, `FUN_1400BE870` clear callback), 1 = single shadow map (`GXScreenSpaceShadow_PCF4/PCF9`), 2 = cascaded (`GXScreenSpaceCascadedShadow_PCF4/PCF9/Soft`); the second target is used when the blur is on (`[[GXRS+0x5368]+0x128] × record+0x24 > 0` and filter flag bit 0); **2 `view+0x710 = 3`** and bind the mask as the view's t32 source (`view+0x12A0 = FUN_1401B8EA0(...)`, bound through `view+0xC98`); **3 `view+0x710 = 1` if it was 3** |
| **`GXRsmScene`** (`143D5EC40`; ctor `1401DC790`) | `FUN_1401DD490` | not on the world path in `FUN_140103510` (no recorder there; INFERRED: the reflective-shadow-map scene of the menu/probe pipelines) | 0 setup/draw, 1 constants, **2 `view+0x710 = 1`**, **3 `view+0x710 = 0`** |
| `GXSgShadowSceneImpl` (`143D4A378`, ctor `1400C30D0`) | slot 4 `FUN_1402057D0` | the shadow sub-scene | calls the preparation `FUN_1400F90F0` (direction) and `FUN_1400F9B50` (cascade records, §1.4) |

Correction to the scope of this note's earlier text: `FUN_1401DD490` is the **reflective shadow map** scene, not a cascade layer; the cascade layer is `FUN_1401B6B80`. The earlier writer search for `view+0x710` (TESS-SHADOW-FORWARD-NATIVE §4.1) missed
`FUN_1401B6B80` codes 5 and 6; the state has **three** writers.

## 7. The recording and the windows (STATIC, `FUN_140103510`)

`FUN_140103510` records the pipeline's frame. For the world pipeline (`pipeline+0x7C == 3`) it tests the **shadow gate**: `pipeline+0x3EBC` (set when the pipeline was built with its shadow flag, §1.1) **and**
`[[GXRS+0x5360]+0x28]` (the global "shadows on" option; absent object counts as on).

| Gate | Recorded (sub-pass numbers are the pipeline's command indices, the same space as the engine pass ids 0x21, 0x29, 0x39, 0x45) |
|---|---|
| closed | `FUN_1401B8B60(ss, cmd, 0xD)` (0x13 when `DAT_14473D0DB`): the mask is **cleared to white** by `FUN_1400BE870`; **no window command at all**, so `view+0x710` stays 0 and every forward row is `Fwd` |
| open | `FUN_1401B8A60(ss, cmd, 0xD|0x13, atlas)`: the mask draw (mode 2, atlas = `pipeline+0x2DA8` override else `+0x1520`); then the windows below |

Windows (open gate; `iVar28` is the pipeline's variant integer: 0 or non-zero):

| Variant | Cascade windows (`FUN_1401B6980`: code 5 at A, code 6 at B) | Screen-space windows (`FUN_1401B8CA0`: code 2 at A, code 3 at B) |
|---|---|---|
| `iVar28 == 0` | one window **[0, 0x3C]** | one window **[2, 3]** |
| `iVar28 != 0` | **[0x17, …]** and **[0x1E, 0x2E]** | **[0x17, 0x1B]** and **[0x1E, 0x21]** |

Consequences for forward rows (INFERRED from the ranges; the forward pass is engine pass `0x29` = 41, the phantom rows `0x21` = 33, `0x22`, `0x1F`):

- the **cascade window contains pass 0x29** in both variants → forward rows of the world are **`FwdSdw`** whenever shadows are on and no screen-space window is open;
- a screen-space window sets 3 and its end sets **1** (not 0): rows inside it are `FwdSSdw`, later rows return to `FwdSdw`; the `iVar28 != 0` windows end at 0x1B and 0x21, so the 0x21 phantom rows would be the `FwdSSdw` ones;
- the cascade end (code 6) sets 0: rows after the window (pass 0x45, 57 and beyond) are `Fwd`.
This matches OBSERVED: hair drawn with `FwdSdw_[A]` (14) and `Fwd_[A]` (4), water with `FwdSdw`, `FwdSSdw` hair in m51 (I-0063).

## 8. The state `view+0x710` (GXSceneContext shadow mode)

`view+0x710` is the scene context's shadow-receive mode (the field behind the debug enum `GXSCN_SHADOW_MODE`; `GXSCN_SHADOWMAP_FILTER_MODE_ENUM` is the PCF filter). It is a plain dword, mutated by the commands above **in
execution order**, read by `FUN_14005A120` (forward shader pass: 0 → `Fwd`, else `4 + (mode == 3)`) and by `FUN_14010D260`, `FUN_14019FB20` (the lighting side, `== 3` tests).

| Value | Meaning | Forward pass | Receiver reads |
|---|---|---|---|
| 0 | no shadow window | `Fwd` | nothing |
| 1 | cascade atlas receive | `FwdSdw` | `t32` = atlas, `s12` comparison, `FC_ShadowMapParam` (row 28), `FC_ShadowColor` (29), `FC_CascadeSelectDist` (54), the cascade matrices (rows 38…53, indexed dynamically) |
| 3 | screen-space mask receive | `FwdSSdw` | `t32` = mask (swizzle `.wxyz`: channel `w`), `s8`, `FC_ShadowColor` (29) |

## 9. The atlas, the cascades and their matrices (STATIC + existing producers)

- **Resources**: §1.1 (atlas `+0x10`, four cascade sub-targets `+0x140..+0x158` of 0x130-byte objects, `cbPerScene` `+0xA30`, four rasterizer states `+0xA38..`, `+0xC18` ready). Observed: 8192² R16 with four 4096² quadrants.
- **State object fields** (`GXCascadedShadowScene`, written by `1B5720` from the ShadowParam record and by the cascade fit): `+0xA60` direction; `+0xA70` near; `+0xA74…+0xA84` the clamped splits and fade; `+0xA90` →
  `FC_ShadowMapParam`; `+0xAA0` `FC_CascadeSelectDist`; `+0xAB0` `FC_ShadowColor`; `+0xAC0…+0xAEF` depth rate, scale, kernel scale, texel sizes; **`+0xAF0 + 0x40·n` the four cascade view-projection matrices** (n = 0..3).
- **`cbPerScene`** (`FUN_1D6D20`, 336 bytes; native producer `PerScene` in `native/world/light_producers.cpp`, qualified against the engine): `{transpose(inverse(pass+0x50)) [0..63], 4 × transpose(camera pose × cascade matrix n) [64..319], CascadeSelectDist (= state+0xAA0) [320..335]}`; built only when `[state+0x110]` is set.
- **ShadowParam record → constants** (`ShadowFieldsFromBanks`, oracle-checked): near, three splits and the fade clamp with the executable epsilon; `FC_ShadowMapParam = (fadeEnd, 1/max(fadeRange, tiny), ½/size, ½/size)`; `FC_ShadowColor` from the record's packed colour and `k[]`; texel scales `1/size`;
  `FC_CascadeSelectDist`-related row `{1, d0/den₀, d0/den₁, d0/den₂}` (`+0x250`). The record is `{map area, id 0}` of `mapbase`/`default` (menus: `{1, id}`).
- **Bias**: per-cascade rasterizer depth bias from the record (`+0x91C`, stride 0x50), converted as GX does.

## 10. The screen-space mask (STATIC, DXBC)

| Programme | Inputs | Notes |
|---|---|---|
| `GXScreenSpaceCascadedShadow_PCF4/PCF9/Soft` | t1 scene depth, **t32 atlas**, `s12` `SS_Shadow` (comparison), `s7`; b0 **`cbPerScene`**, b8 `cbSceneParam` (`FC_ShadowMapParam` @448; `Soft`: `FC_ScreenSize`, depth rate/scale/kernel, texel scales, `FC_ShadowViewKernelScale`, t38 dither `s6`) | writes `o0.x` (shadow factor) after the distance fade `(farStart − dist) × scale` (`cb8[28]`) |
| `GXScreenSpaceShadow_PCF4/PCF9` | same without the cascade select | single shadow map (menu views, mode 1) |
| `GXScreenSpaceShadow_Blur` | t1 depth, `s7`; b1 `cbBlurParam {g_vDepthComputeParam, g_avSampleOffsets[16]}` | depth-aware blur (`cbBlurParam` producer `FUN_1D7850`, native `BlurParam`: 12 kernel taps `{x/W·s, y/H·s, gauss}`, `s = [[GXRS+0x5368]+0x128] × record+0x24`) |
| `GXRend_CompositeShadowMask` | t3 GBuffer1, **t33 SSAO**, `s7 s8`; b5 `cbCompositeShadowMask {FC_Mult, FC_Add}` (executable constants per upload site) | produces the lighting's **t32 = shadow × AO** |

The mask the forward `FwdSSdw` reads is the layer's own target (`FUN_1401B8EA0`), not the composite: closed in part AW (TESSSCENE-SSMASK-NATIVE §2): a four-channel 8-bit target (INFERRED `R8G8B8A8_UNORM`), channel `w` = shadow term, `x y z` = packed depth, read by `FwdSSdw` as `.w` with sampler `s8` (`SS_WrapLinear`); only the choice between the `+8` and `+0x10` object stays a live read (SH5).

## 11. Who writes cb8 (`cbSceneParam`) and `cbTessScene` (STATIC)

Both are blocks of the **exec context** (`GXRenderExecContext`): `FUN_14013AA60` (slot 1) copies, from the source context, `+0xCB0` (the 960-byte `cbSceneParam` image, `FUN_14013A580`), `+0x1070` (the 0xC0-byte `cbMatrixWVP` image) and
**`+0x1130` (the 0xC0-byte `cbTessScene` image, `FUN_14013A930`)**, then uploads them (`cbSceneParam` at `13AFA1`, WVP after it, `cbTessScene` to `[[144961B50]+0x4DD0]+0x2D0`).

- No direct store to `+0x1130…+0x11EF` exists outside the zeroing initialiser `FUN_1400843C0` and these copies (whole-database search, both `MOV`-family stores and `LEA` bases); the same holds for `+0xCB0…` and `+0x1070…`: the images are
  produced by **bulk copies of prebuilt camera records** (the `CED670` camera output and the shadow state), not by field stores. The native design therefore builds them from the camera and the shadow state, as `native/world` already does for the first two.
- **`cbSceneParam`** (960 bytes): camera rows `0…27` and `1…` by `ProduceSceneCamera`; **shadow sub-block `[0x1C0, 0x370)` = `FUN_14006F920`'s copy of `[GXRS+0x4DD0]+0x5E0`, which the cascade's code 3 writes**; forward-only fields `+0x260…+0x35F`
  (`FC_ProjSpaceToShadowMatrix[4]` = transpose(`FUN_14001CE40`(camera pose, state+0xAF0+0x40n))), `+0x360` (`FC_CascadeSelectDist` = state+0xAA0), `+0x374` slice scale/bias, `+0x380/+0x384` light-tile width and cell count
  (`ForwardFields`, INFERRED tile width).
- **`cbTessScene`** (reflection of the hull/domain programmes): `{g_f4x4ViewProjection @0, g_f4Eye @64, g_f4ViewVector @80, g_f4GUIParams2 @96, g_f4ViewFrustumPlanes[4] @112, g_fGlobalTime @176}`. The hull uses eye, view vector, GUI params and the four planes; the domain only the
  view-projection. **Content closed in part AW (TESSSCENE-SSMASK-NATIVE §1)**: the absolute-world view-projection (not the camera-relative matrix), the absolute eye, the unit vector toward the camera, the four inward world side planes, the clock; setter = slot `+0x70` `FUN_14013B100`; `g_f4GUIParams2.x` scales the distance fade (`cb5[6].x`), value OPEN (live row TS1).

## 12. D3D12 design (D-0157), and what Elden Ring shows

The ER decompilation folder holds no study of ER's shadow passes (a `grep` finds only RTTI and symbol strings); the usable comparison is its renderer architecture
(`DS3-ER-RENDERER-BOUNDARY-MAP`): typed operation records built by the render thread, resource barriers finalised **before** native encoding, per-frame allocator pools reset on fences. Following it, and since D3D12 makes the
resource state explicit, the shadow system should not reproduce D3D11's order-dependent mutable state:

1. **Explicit per-view shadow descriptor instead of `view+0x710`.** Compute once per frame a `ShadowFrame {mode per engine pass, atlas SRV, mask SRV, cascade/blur constants}` from the original's windows (§7: gate, variant, ranges) and let each forward row look its
   pass up: `Fwd` / `FwdSdw` / `FwdSSdw`. Same results, no dependence on command execution order, testable in isolation (a table, not a state machine). Keep the original rule as the reference and log the original's value to validate (TS2).
2. **One full `cbSceneParam` per view, produced whole.** The shadowed programmes index the cascade matrices dynamically, so the read analysis requires the whole 960 bytes (as for the Grass `cbvModelParam`): produce the complete image once per view (camera, shadow block, forward block) into the upload ring and bind one root CBV
   for every draw of that view; no per-draw rebuild.
3. **Atlas as a placed 8192² R16 resource, cascades as four viewports of one DSV**, one barrier DEPTH_WRITE → PIXEL_SHADER_RESOURCE after the last caster (original programmes sample an atlas with quadrant offsets inside the matrices, so the layout stays; a texture array would need new shaders).
4. **Cascade cache without GX reprojection**: keep a retained *static-caster depth* per cascade (valid while direction and slice are unchanged) and `CopyTextureRegion` it into the working cascade before drawing the dynamic casters; this replaces `GXRend_CopyShadowMapDepth` and its two-texture ping-pong
   (§1.4), uses an explicit resource lifetime, and degrades to the reference full redraw.
5. **Mask once, shared**: the screen-space pass writes the mask target; the blur reads it; the composite writes t32; the `FwdSSdw` rows read the mask target through the same descriptor table as the lighting (no re-binding through a view field).
6. **Tessellation constants from the same `ShadowFrame`/camera record** (no separate producer): `cbTessScene` built from the camera each view.

## 13. Ordered native steps, witnesses and live rows

| # | Step | Witness |
|---|---|---|
| 1 | read the gates each frame: `pipeline+0x3EBC` equivalent (the view has a shadow flag), `[[GXRS+0x5360]+0x28]`, the pipeline variant | SH1: both values and the variant for the main view at the High Wall and m40 |
| 2 | ShadowParam record and the cascade state (`1B5720` clamps CALL, fit `23A0C0` CALL): `Banks::Shadow(area, 0)` and the state object fields of §9 | the shadow sub-block against the oracle within the existing `light_check` shadow route (S3) |
| 3 | atlas and four cascade viewports, per-cascade bias (steps 3 and 5 of §2) | S1 |
| 4 | casters: Dpt/DptA recipes for the player and map (§3), one selection per cascade | S2 (22+5 per view) |
| 5 | `cbPerScene` (`PerScene`), `cbBlurParam` (`BlurParam`), the full `cbSceneParam` image per view (§11) | word-for-word against the engine's blocks (SH3) |
| 6 | screen-space mask: `GXScreenSpaceCascadedShadow_*` by FilterMode, blur, composite (t32) | S3: mask equal within PCF tolerance; lit HDR within K-0426 |
| 7 | sun switch `DDirLightShadowed1[_Cloud]` | S4: the player's shadow on the ground |
| 8 | forward receive: `ShadowFrame` (§12.1), pass names 4 and 5 in `SelectModelPrograms`, the shadowed forward PSOs with t32/s12 | SH2: the original's per-row mode (hair, water) equals the table; image: hair and ghosts shadowed in shade |
| 9 | local-light tiles and shadowed point/spot programmes (§2 step 10) | S5 |
| 10 | tessellation constants (`cbTessScene`, `cbPNTriangles`) with the veil's `_PN` | TS1, TS3 |

Live rows (none taken):

| ID | Read | Expected |
|---|---|---|
| SH1 | `pipeline+0x3EBC`, `[[GXRS+0x5360]+0x28]`, the variant `iVar28`, and the `pipeline+0x2DA8` override for the main view | 1, 1, 0 or 1, 0 |
| SH2 | `view+0x710` at each forward row (engine pass, shader pass) in a still frame with the player in shade | 1 for 0x29, 3 only inside a screen-space window |
| SH3 | the 960-byte `cbSceneParam` and the `cbPerScene` actually uploaded for one forward row, against the native producers | equal |
| SH4 | the sub-pass numbers where the cascade and screen-space windows are recorded in `FUN_140103510` (the argument values at the four call sites) | §7 table |
| SH5 | the format and channel of the target bound as t32 for a `FwdSSdw` row, and whether it is the layer's `+8` or `+0x10` | four 8-bit channels (INFERRED `R8G8B8A8_UNORM`), channel `w`; only `+8` against `+0x10` remains (part AW) |
| SH6 | `[[GXRS+0x5368]+0x128]` (blur scale) and the record `+0x24` | > 0 |

## Exports

`decompiled/`: `001B5390`, `001B5720`, `001B6B80`, `0023A0C0`; part P: `001010A0`, `00108430`, `001B5B40`, `001B6020`, `001B7FF0`, `001B4AC0`, `001B4830`, `001B5EC0`, `001B5010`. Existing: `000F90F0`, `000F9B50`,
`00088A10`, `001DAF40`, `0010CC20`, `001D6D20`, `001D7850`, `00057A20`.
