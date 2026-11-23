# I-0068 part T: water surfaces (M05, `GXFlver_WaterBumpTri`), ready to code

Request (coordinator, 2026-10-10): after the decals, water at the same ready-to-code level.
Water is the second family without a spec in the coverage matrix (row 6).

Labels: STATIC, OBSERVED, INFERRED.

**Evidence** (`evidence/I-0068/queries/water/`):

- `water-draws.tsv`: the two water draws in the 12 retained captures that contain them, from
  `Get-ProgramDraws.ps1`;
- the m40 named listing (I-0053 `gpu-order-i52-live-03-A-frame-300.txt`).

**Programme interfaces:** reflected from the game's own DXBC (`fxc /dumpbin` on the I-0063
programme cache, outside the repository).

> **Part BO (WATER-TESS-WAVE1-READY):** t54..t57 are bound by the frame renderer `FUN_140103510` (slots 0x36..0x39) with defaults (t54 black, t57 grey 1x1), not by `FUN_1401BF180`; the water pixel programmes read no IBL; the frame copy sits after `WatDpt`. Where this note differs, the new sheet wins.

## 0. Native status (`main`)

| Part | Native | Proof |
|---|---|---|
| shader passes | **only 0 (Fwd), 6 (Gbuf), 7 (Gbuf2)** are selectable; water needs **12 (WatDpt)** and **4 (FwdSdw)** or 0 (Fwd) | `native/content/model_programs.cpp:41` (`SelectModelPrograms` refuses other passes) |
| engine rows | native builds rows 5, 6, 7, 0x1F, 0x29 only; the water rows 0x24/0x27 are missing | `native/frame/game_bootstrap.cpp:3869` |
| water material parameters | the generic `cbMatDynParam` producer reads MTD values (the `FC_Water*` fields are ordinary declarations); `GXFT_WaterInteraction` is a parsed built-in | `native/content/model_parameters.cpp:17`, `native/frame/game_model_preparation_test.cpp:324` |
| forward inputs (`cbLight`, `cbFog`, light lists, IBL) | **PARTIAL** (the forward rows exist; light buffers empty, IBL black) | `native/frame/game_bootstrap.cpp:2813`, COVERAGE-MATRIX row 28 |
| frame copy, reflection, interaction map | **ABSENT** | COVERAGE-MATRIX row 31 |

**So water is absent:** no row, no pass, no inputs. The specification follows.

## 1. The original pass (OBSERVED)

**Where.**

- **m40, by name:** the listing names `GXFlver_WaterBumpTri_WatDpt` then
  `GXFlver_WaterBumpTri_FwdSdw`, once per frame.
- **The same pair, by position,** in I-0055 `stream1` and in the I-0063 `i63-live-03..07`
  stream captures: programme ids 7864/7856.
  - It is the only "draw → copy → draw" pattern with shared textures in those captures.
  - The intervening copy is the scene HDR copy, as in m40.
- **Correction (part W): also at m37** in `i63-live-10`, with the same ids (9261 indices) and
  a planar reflection view. The other four witness zones (m31, m45, m50, m51) draw none; their
  water-like meshes are `ColDifSpcBumpTrIbl` and `Slime` variants (M05/M08 shared programmes)
  drawn in the G-buffer.
- **The mesh:** 30 indices in I-0055, 3456 in the I-0063 streams, 9261 at m37; triangle list;
  one draw per pass per frame.

**Order** (m40 listing), after `GXLightComposite` and the opaque forward draws:

| # | Event | Target | Notes |
|---|---|---|---|
| 1 | clear `12484` (640×360 **R16G16_FLOAT**) to **(0.502, 0.502)** | — | the water interaction map's neutral value; nothing writes it at Firelink (I-0055 has the same clear 3×, no writer) |
| 2 | **`WaterBumpTri_WatDpt`** | HDR `12439` (RGBA16F) + `12447` (GB5, RGBA16F); depth read | the **underwater fog** (§2.2) |
| 3 | **CopyResource HDR `12439` → `12440`** | — | the scene copy ("refraction"; also read later at t45 by forward/FFX draws, `59470` binds slot 0x2D) |
| 4 | **`WaterBumpTri_FwdSdw`** | HDR + GB5; **depth write** | the lit water surface |
| 5 | **CopyResource depth → `12438`** | — | the readable depth now includes the water surface |
| 6 | depth downsample ×2 again, then the transparent forward and particle draws | | |

**States** (capture descriptors; I-0055 and the streams agree):

| | WatDpt (ps 7864) | FwdSdw (ps 7856) |
|---|---|---|
| blend (all RTs, not independent) | colour SRC_ALPHA / INV_SRC_ALPHA / ADD; alpha ZERO / ZERO; mask RGBA | colour SRC_ALPHA / INV_SRC_ALPHA / ADD; alpha ONE / ZERO; mask RGBA |
| depth-stencil | depth on, **no write**, LESS_EQUAL; stencil off | depth on, **write**, LESS_EQUAL; stencil off |
| rasterizer | SOLID, **cull BACK**, FrontCCW = 1, depth clip | the same |
| draw | triangle list, `DrawIndexed` | the same mesh |
| PS constant buffers | b1, b2, b8, b9 (bound: b1–b4, b7–b9) | b1–b6, b8, b9 |

## 2. Programmes (STATIC, DXBC)

`GXFlver_WaterBumpTri` is an ordinary FLVER material family in `GXFlverShader`. Its passes are
`Dpt`, `DptA`, `Fwd`, `FwdSdw`, `FwdSSdw`, `Gbuf`, `Velo` and `WatDpt`, with `_[A]` and `_Skin`
variants.

**Pass-table index:** the engine's pass-name table at `14473CE60` holds `WatDpt` at index
**12**. The `u"WatDpt"` string `143D48990` is stored at `14473CEC0`. The known indices are
0 Fwd, 1 Velo, 2 Dpt, 3 DptA, 4 FwdSdw, 5 FwdSSdw, 6 Gbuf, 7 Gbuf2, 12 WatDpt.

### 2.1 VS (`_WatDpt.vpo` = `_Fwd.vpo` interface)

- **Inputs:** POSITION, NORMAL, three TANGENTs (uint), COLOR1, TEXCOORD0/1 (int). This is the
  standard FLVER layout.
- **Constant buffers:**
  - b4 `cbInstanceData`;
  - b5 `cbObjMatrix`;
  - b8 `cbSceneParam`;
  - b11 `cbTexScroll` (the water scroll; native `ProduceTexScroll` exists).
- **Outputs:** two `SV_ClipDistance` (user clip planes, from `cbInstanceData.userClipPlane`).

### 2.2 `_WatDpt.ppo`: the underwater fog

- **Inputs:** t0, t3, t7, t10, t16 (the material's diffuse and bump layers) and **t56
  `g_DepthTexture`**.
- **Constant buffers:**
  - b1 `cbTextureTilingScale`;
  - b2 `cbMatDynParam` (`FC_WaterNormalUvScrollSpeed`, `FC_WaterMiddleUvScrollSpeed`,
    `FC_WaterFogParam1`);
  - b8 (`FC_ScreenSize`, `FC_DepthComputeParam`);
  - b9 (`FC_GlobalTime`).
- **Output:** **o0 only**.
  - `rgb = FC_WaterFogParam1.rgb`.
  - `a = mask × (1 − 1 / (1 + max(0, sceneDepth − waterDepth) × FC_WaterFogParam1.w))`.
    - `mask` combines the layers and the vertex alpha².
    - `sceneDepth` is t56 linearised with `FC_DepthComputeParam`.

So the water tints the scene behind it by its thickness, before the HDR copy.

### 2.3 `_FwdSdw.ppo`: the surface

**Textures:**

- the material layers t0, t1, t3, t7, t8, t10, t16;
- **t32 `g_ShadowTexture`**: the sun cascade atlas (8192² R16, SHADOWS-NATIVE), through
  comparison sampler s12;
- **t54 `g_ReflectionTexture`**;
- **t55 `g_RefractionTexture`**;
- **t56 `g_DepthTexture`**;
- **t57 `g_WaterInteractTexture`**;
- the forward light lists **t59–t62**;
- also bound: IBL t30/t31.

**Constant buffers:**

- b1, b2: `FC_WaterNormalUvScrollSpeed`, `FC_WaterDiffuseUvScrollSpeed`,
  `FC_WaterMiddleUvScrollSpeed`, `FC_WaterFogParam0`, `FC_WaterFresnel`, `FC_WaterFadeParam`,
  `FC_ParallaxParam`;
- b3 `cbMtdParam`;
- b4 `cbInstanceData` (SH `aSh`, env data);
- b5 `cbLight`;
- b6 `cbFog`;
- b8 `cbSceneParam`: the shadow matrices `FC_ProjSpaceToShadowMatrix[4]`, `FC_CascadeSelectDist`,
  the light-tile constants;
- b9.

**Output: o0 only. Input:** `SV_IsFrontFace`.

`_Fwd.ppo` (shader pass 0) is the same surface without the shadow map.

**The capture limit.** The capture tool records PS slots t0–t47 only, so the t54–t57 bindings
are not observed. **STATIC**: the binder `1BF180` (a forward renderer's input setup) binds:

- **t54** = `[frame+0x1D0]+0x240`, or `+0x660` by `DAT_144961BA0`, at `+0x28`;
- **t55** = `[frame+0x1108]` SRV;
- **t56** = `[frame+0xFD0]` SRV (the readable depth).

`frame` is the frame renderer `103510`, whose `+0xFD0` / `+0x1110` / `+0x1118` are the copies
of DECALS-NATIVE §1.

**INFERRED:**

- t55 is the HDR copy `12440`, taken in step 3, right before the surface.
- t54 is the planar-reflection target. Firelink renders no reflection view in the m40 listing,
  so it holds a cleared or default image there.
- t57 is the `12484` interaction map.

Whether the FLVER water draw receives these slots from `1BF180` or from its own pass setup is
open (risk 1).

### 2.4 The engine rows and passes (STATIC)

**Rows** (`57A20`, asm `140058A28..140058A94`). The row builder emits rows as
`{engine pass, preparation, class mask, 0xFF}`. Native already reproduces three of them:
`{0x29, 0, 0x12}`, `{0x1F, 0, 0x80}` and `{7, 3, 0x100}`.

For a draw descriptor of **kind 0x27** (the water material), it emits **two rows**:

- **`{0x24, 6, 0xFFFF, 0xFF}`**: the water-depth row;
- **`{0x27, 0, 0xFFFF, 0xFF}`**: the surface row. The `MOV qword [RBP+0xD0], 0x27` writes
  preparation 0.

A kind-0x27 descriptor gathered by a **view of group 0x39** gets no row (line 245).

**Preparation → shader pass** (`5A120`, the `switch` on the row's preparation):

| Preparation | Shader pass |
|---|---|
| 0 | forward: 0 `Fwd`, or **`4 + (view+0x710 == 3)`** when `view+0x710 ≠ 0`, i.e. **4 `FwdSdw`** or 5 `FwdSSdw` by the view's shadow mode |
| 1, 2 | 2 `Dpt` |
| 3 | 6 `Gbuf` |
| 4 | 7 `Gbuf2` |
| 5 | 1 `Velo` |
| **6** | **12 `WatDpt`** |
| 9–12 | 13–16 |

So the water is:

- engine pass **0x24** → `WaterBumpTri_WatDpt`;
- then engine pass **0x27** → `WaterBumpTri_Fwd` / `FwdSdw` / `FwdSSdw`, as the view's shadow
  mode selects. m40 runs with mode 1 or 2: `FwdSdw`.
## 3. Ready-to-code order (D3D12)

| # | Step | Native |
|---|---|---|
| 1 | **Rows and passes** | for kind-0x27 descriptors (water materials), add the two rows `{0x24, 6}` and `{0x27, 0}` (§2.4). Draw them as `WatDpt` (shader pass 12), then the surface, with shader pass **0** `Fwd` while native shadows are absent, and 4 `FwdSdw` once they exist (`view+0x710`), as the original. Extend `SelectModelPrograms` to passes 4, 5 and 12, keeping its VS fallback rule |
| 2 | **Resources** | the HDR copy (same format as the scene HDR); the readable depth copy (shared with decals and forward depth); the interaction map 640×360 `R16G16_FLOAT`; a reflection texture (start with a 1×1 or cleared target, INFERRED Firelink behaviour) |
| 3 | **Clear** | the interaction map to (0.502, 0.502) each frame |
| 4 | **`WatDpt`** | after the composite and the opaque forward draws; RTV HDR (+GB5 with write mask 0: the programme declares o0 only); depth read-only LESS_EQUAL; cull back; blend as observed; t56 = readable depth |
| 5 | **Copy** | HDR → HDR copy (also the t45 frame copy of later forward/FFX draws, FFX-FAMILIES §8) |
| 6 | **`FwdSdw` / `Fwd`** | depth write on; t54 reflection, t55 HDR copy, t56 depth, t57 interaction; t32 shadow atlas (or the `Fwd` variant); t59–t62 the forward light lists (native rows already allocate them); b5/b6 from `ProduceNativeForwardLight`; b4 instance data with SH and env; b2 from the generic `cbMatDynParam` producer |
| 7 | **Copy** | depth → readable depth again (the second downsample and the later soft particles see the water) |

**Constants.** No new producer is needed for b1, b2, b3, b5, b6, b9, b11: these are the forward
material path's blocks. **b8** needs the shadow fields only for `FwdSdw` (SHADOWS-NATIVE).

## 4. Checks

| # | Check | Pass |
|---|---|---|
| W1 | structure (Firelink still camera) | clear of a 640×360 RG16F target, `WatDpt`, HDR copy, `FwdSdw`, depth copy, in that order, one water mesh per pass, same index count as the original (I-0055: 30) |
| W2 | states | as §1 (cull back, depth read then write, blend alpha ZERO/ZERO then ONE/ZERO) |
| W3 | t54–t57 | a capture with the slot range extended to 64 (or a PIX-style read): t55 = HDR copy, t56 = depth copy, t57 = interaction map; record what t54 is at Firelink |
| W4 | image | the HDR after step 6 against the original at the same frame: mean absolute difference ≤ 2/255 over water pixels; the opaque scene unchanged |
| W5 | depth | the readable depth after step 7 contains the water surface (forward/particle soft blending at the shore matches) |

## 5. Risks and open items

1. **t54–t57 provenance** is STATIC through `1BF180`, a binder whose own draw is not the FLVER
   water. Confirm with W3 before relying on it.
2. **Engine pass order.** Rows 0x24 and 0x27 are drawn by the frame renderer's pass list.
   OBSERVED order: after the composite and the opaque forward draws, with the HDR copy
   between them. Place them identically, and check W1.
3. **Reflection.**
   - The planar reflection view (`WaterSurfaceQuality`; Thomas's setting is HIGH) is not drawn
     at Firelink.
   - m37 runs a second lighting call for a water reflection camera (`native/world/light_producers.cpp:818`),
     so other zones render one.
   - That view needs its own spec.
4. **Interaction (D07).**
   - The ripple simulation (`GXWaterInteractionManager`, programmes
     `GXWaterInteractionHeightMap/NormalMap`) writes the 640×360 map.
   - At Firelink it stays neutral.
   - Its spec is separate (COVERAGE-MATRIX row 42).
5. **Tessellated variants** (`WaterBumpTriInteraction*_FT`, hull/domain shaders) are not drawn
   in any retained capture.

See also [D3D12-MASTER-PLAN](D3D12-MASTER-PLAN.md) §6 S3: the refraction copy `t55` is the same object as the effects frame copy `t45` (`pipeline+0x1108`); one demand-driven `FrameCopy` serves both in the D3D12 plan.
