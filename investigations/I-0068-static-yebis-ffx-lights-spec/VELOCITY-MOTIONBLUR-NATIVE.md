# I-0068 part AE: velocity and motion blur, ready to code (with upscaler compatibility)

Request (coordinator, 2026-10-10). Thomas allows motion blur in the options, so velocity and
motion blur become real families (matrix row 39). The spec covers, starting from the static
owners of REMAINING-FAMILIES §2:

- the option and its gates;
- the velocity target (format, passes, programmes);
- previous positions (skinning palette n−1, cloth, objects, camera);
- the blur pass and its place in NATIVE-PASS-ORDER;
- reference captures to take with motion blur on;
- an upscaler-compatibility section for a later DLSS: preparation only.

Labels: STATIC, OBSERVED, INFERRED. Nothing here is OBSERVED yet: no capture has motion blur
on (REMAINING-FAMILIES §2).

**Evidence** (`evidence/I-0068/queries/velocity/`):

- decompiles `102380`, `13BFC0`, `13BD10`, `E4B50`, `885F0`, `1D9910`;
- the frame renderer `103510` (lines 400–420, 2268);
- `E69A0` (lines 560–670), `FB890` (lines 413–418, 489–538);
- disassembly of `GXFlver_*_Velo*`, `GXFlver_Common_Velo`, `MotionBlur*`.

## 0. Answer first

| Item | Answer |
|---|---|
| Gates | the option byte `[GXRS+0x5360]+0x20` (GraphicsConfig `MotionBlur`) **and** the factor `[GXRS+0x5368]+0xE0 > 0` **and** pipeline velocity support `+0x3EB9` (capability bit 5 of `pipeline+0x50`) and `+0x3ED2` → the frame draws the velocity pass. YEBIS blurs when the gparam MotionBlur `Enable` holds **and** a velocity source exists |
| Velocity target | `pipeline+0xE20` (colour) with depth `pipeline+0xE30`; the object YEBIS reads is `pipeline+0xDF0`. **Encoding:** `RG = NDC delta × scale × 0.5 + 0.5`, `B = w/(w+10)`, `A = 1`; cleared to (0.5, 0.5, 1, 0). The decode `(v + 0.5/255)·2 − 1` shows **an 8-bit UNORM target** (INFERRED: RGBA8) |
| Passes | render group **0x33**: clear, then the camera velocity (`MotionBlurWriteCMBVelocity`, `13BFC0`), then the material **`Velo`** rows (shader pass 1); group **0x35**: velocity dilation (`MotionBlurDilateVelocity`, `13BD10`); then **YEBIS** motion blur (`tech_MotionBlur*`) inside the YEBIS step |
| Previous positions | camera: `VC_PrevMatrixWVP` (b0 +64) per draw and the previous view-projection (`13BFC0` keeps it at `+0x3D70+0x60..`); skinning: **b6 `cbPrevObjMatrix`** `VC_aOldObjMatrix[]`, indexed by the raw blend index (no `matricesData` offset); objects: b6 row 0 (previous world); cloth: INFERRED as skinning (to check) |
| Placement | after the forward and effect steps and the GX post blocks of `103510` (line 2268, before `102790`), and **before YEBIS** (step 12 of NATIVE-PASS-ORDER) |

## 1. Option and gates (STATIC)

| Gate | Where | Meaning |
|---|---|---|
| `[GXRS+0x5360]+0x20` (byte) | `103510` lines 409–418, `E4B50`, `13BFC0` | the graphics option (MotionBlur ENABLE/DISABLE). `+0x24` (dword) caps the sample count (`E4B50`) |
| `[GXRS+0x5368]+0xE0` (float) | the same | the motion-blur factor; ≤ 0 disables. `+0xF8` scales the sample count |
| `pipeline+0x3EB9` | set by `885F0` from capability bit 5 (`pipeline+0x50 & 0x20`); `85540` resets it | the pipeline owns a velocity target |
| `pipeline+0x3ED2` | `103510` lines 409–418 | per-frame velocity request (INFERRED: the view allows it) |
| `pipeline+0x3F70` | `102380` | camera velocity also when YEBIS exists (else YEBIS auto-generates it) |
| `view+0x9D0` | `102380` | dilation step (with YEBIS) |
| gparam **MotionBlur** | bank `[GXRS+0x4640]`, key index 6 (`frame+0x4008/+0x4070`, weight `+0x40C0`), blend `1D9910`, → `E4B50` → YEBIS globals `14496214C..` | `Enable` (forced 0 when the factor is ≤ 0), mode `144962150` (2 = application velocity texture), scale `144962158` (≥ 0.001), sample count `144962160`, … |
| YEBIS calls (`E69A0` lines 584–666) | `SetMotionBlurEnable` (`CAA20`, enable ∧ velocity object), `CAAA0`, mode `CAA60`, source: `CAB90(1, tex, w, h)` when mode 2 and a velocity object exists, else `CAB40(0, 0, −2)` (auto camera velocity); `CAAE0`; current/previous view `C95B0`/`C95F0`, projections `C9590`/`C95D0` | |

**Gparam values in the available files:**

- `default` id 0: Enable 1, BlurTimeRatio 0.06, NumBaseSamples 4, MaxBlurLength 0.1, …
- `mapbase` id 0: Enable 1, BlurTimeRatio 0.2, MaxRecurrences 2, FalloffDistanceMax 3, …
- `Setting Info` "Create MotionBlurInternalVelocityTex" = 1.

So the option alone switches it on in the world.

## 2. The velocity pass `102380` (render group 0x33 / 0x35) (STATIC)

| Sub-order | Command | Content |
|---|---|---|
| 3 | targets | colour `pipeline+0xE20`, depth `pipeline+0xE30` |
| 5 | `0xF8` | (state reset) |
| 1 | clear `1400BE8E0` | colour `0x007F7FFF` (= (0.5, 0.5, 1, 0): zero velocity, far), depth 1.0 |
| 3 | **camera velocity** `13BFC0`, when `+0x3EB9` ∧ (`+0x3F70` ∨ no YEBIS) | full-screen `MotionBlurWriteCMBVelocity` (t2 depth, s3; b0 `g_vDepthComputeParam` +32, `g_mCurToPrevScreen` +48, `g_vIntensity` +128). `g_mCurToPrevScreen` = previous view-projection × current inverse view-projection, in screen space; intensity = `obj+0x11C × factor`. First frame (`obj+0x10 == 0`): previous := current (the NoMove case) |
| 0x34 | **object velocity**: material shader pass 1 `Velo`; the `57A20` row is engine pass 0x34, preparation 5, class mask 0xFFFF, with its gates in I-0067 VELOCITY-SKINNING §7 (part AI) | §3 |
| 0x35 | **dilation** `13BD10` with YEBIS and `view+0x9D0` | `MotionBlurDilateVelocity` (t1 velocity, b0 `cbFilterParam`), inputs `pipeline+0xE68`/`+0xE08`, count and intensity from the MotionBlur gparam (`Object × Camera × …` product) |

**Then YEBIS** blurs the HDR with the velocity object `pipeline+0xDF0` (`FB890` line 494, only
with `+0x3EB9`) inside its own step. The GX `MotionBlur.ppo` (eight taps along the velocity) is
the non-YEBIS fallback; INFERRED unused in the world.

## 3. Object velocity programmes (STATIC, DXBC)

**VS** `<family>_Velo[_Skin][_Ins][@[cl]].vpo` (e.g. `GXFlver_ColDifSpcBumpIbl_Velo_Skin`):

- **inputs:** b0 `VC_MatrixWVP` (+0), **`VC_PrevMatrixWVP` (+64)**; b4 `cbInstanceData`;
  b5 `VC_aObjMatrix[]`; **b6 `VC_aOldObjMatrix[]`**; b11 `cbTexScroll`;
- **current position:** palette `cb5[(BLENDINDICES + matricesData.x)·3]` × `VC_MatrixWVP` →
  `o0` (SV_Position) and `o1`;
- **previous position:** palette **`cb6[BLENDINDICES·3]`** (no `matricesData` offset) ×
  `VC_PrevMatrixWVP` → `o2`.

**PS:**

- `GXFlver_Common_Velo.ppo`: the shared opaque one;
- `<family>_Velo_[A].ppo`: alpha-tested, adding t0, b1, b3 `FC_AlphaRef` and b4.

```
o.xy = saturate((cur.xy/cur.w − prev.xy/prev.w) · FC_DifColMul.xy · 0.5 + 0.5)
o.z  = cur.w / (cur.w + 10)      o.w = 1
```

**Scale (STATIC, part AJ):**

- b7 cbModelParam.FC_DifColMul is built from the draw state +0x310..+0x31F (`ProduceModelParamPs` puts state `+0x310` at b7 +0).
- The material preparation for the Velo pass (`5A120` case 5, lines 302–317) writes it:
  - `a` = the camera aspect (`view+0x418` → `4AFC0`: `camera+0x64` when non-zero, else `width/height`; 1.0 without a camera);
  - `k` = `1 / max(view+0xCA4, ε)`. `view+0xCA4` is 1.0 from the view constructor (`139CE0`), copied with the view; ε = `DAT_143D7ECF4` (about 2.5e-5);
  - `+0x310 = a·0.5·k`, `+0x314 = −0.5·k`, `+0x318 = 0`, `+0x31C = 1.0` (`0.5` = `143D7EDB4`, `−0.5` = `143D7F0C0`). The "Velo" preparation also sets `+0x188` from `group+0x1AA` and clears `+0x2C4/+0x2C5` (meaning not read).
- So `FC_DifColMul = (a·0.5·k, −0.5·k, 0, 1)`, and the PS gives:
  `RG = saturate(Δndc.xy · (a·0.5·k, −0.5·k) · 0.5 + 0.5)`.

**Camera velocity** writes the same encoding with a 2.5 scale (`MotionBlurWriteCMBVelocity`).

**Native producers:**

- b0 needs the previous WVP: the native view block (`game_bootstrap.cpp:3639`) holds the
  current one only;
- b6 needs the previous palette. Native pose owners already keep a two-entry ring when
  previous matrices are requested (`game_bootstrap.cpp:1860-1864`, `owner.ring = previous ? 2 : 1`).
  Lay b6 out without the `matricesData` offset;
- static objects use row 0 of b5/b6 (their previous placement; equal when still).

**Skinning side: see I-0067 VELOCITY-SKINNING (`main` 9430b975), not repeated here.** It
covers:

- the per-owner palette ring (`E+0xA80`, advanced by `16050`) and the keepPrevious sources;
- the binder `064340` (b5/b6);
- the native contract: two versions per owner when keepPrevious is set, previous = current
  on the first frame;
- the cloth Velo variant (adds b2 and b7).

**Correction to that note on the camera** (STATIC, DXBC of `GXFlver_ColDifSpcBumpIbl_Velo[_Skin].vpo`):

- the previous camera **is** bound per draw: **b0 `cbMatrixWVP` +64 `VC_PrevMatrixWVP`**, used
  (`dp4 o2.*, r, cb0[4..7]`);
- the palettes are in world space, and `VC_PrevMatrixWVP` carries the previous
  view-projection;
- the background (no geometry) gets camera motion from the full-screen `13BFC0` pass (§2).
- So native needs the previous view-projection in **two** places:
  - b0 +64 of every Velo draw;
  - `g_mCurToPrevScreen` of the camera pass.

## 4. Place in NATIVE-PASS-ORDER

Insert after step 11 (effects) and before step 12 (YEBIS):

- **11b Velocity:**
  - group 0x33: clear, camera velocity, object `Velo` rows (opaque G-buffer classes; INFERRED:
    no forward or effect rows);
  - group 0x35: dilation;
- **12 YEBIS** then runs its motion blur with this source (mode 2), before or after its other
  techniques as YEBIS orders them (YEBIS-NATIVE §3.2: read the technique order in the capture).

**Dependencies:**

- the readable depth (rank 1 of NATIVE-PASS-ORDER);
- previous camera and pose matrices;
- YEBIS (rank 3) for the blur itself.

## 5. Reference captures (motion blur ENABLE; for the pilot)

GraphicsConfig `MotionBlur` = ENABLE, other settings unchanged. (Note, I-0067 MENUS-MAP 77: `ENABLE` is not a token of the file reader, which then keeps the default level 3 = `HIGH`: option `+0x20` 1, `+0x21` 1, sample cap 5, as read in game. The valid tokens are `DISABLE`, `LOW` (camera only, cap 1), `MEDIUM` (cap 1 plus object velocity), `HIGH` (cap 5) and `MAX` (cap 8); write `HIGH` explicitly to avoid depending on the default.) One frame each, with the
constants:

| # | Scene | What it proves |
|---|---|---|
| M1 | m40 Firelink, camera rotating, player still | camera velocity only (`MotionBlurWriteCMBVelocity`), target format and size of `pipeline+0xE20/+0xDF0`, clear value, `g_mCurToPrevScreen`, YEBIS motion-blur techniques and order |
| M2 | m40, camera still, player running | object `Velo` rows: programmes, b0 `VC_PrevMatrixWVP`, b6 layout, b7 `FC_DifColMul` (velocity scale), sub-order against the camera pass |
| M3 | High Wall (m30) with an NPC and a cloth/hair character moving | skinned, cloth and hair rows (is there a cloth `Velo` VS with a previous cancel?) |
| M4 | bonfire warp or load, first frame after | reset (`13BFC0` `obj+0x10`, NoMove), YEBIS history |
| M5 | m31 or m45 with effects and water in view | which classes have velocity (forward, effects, water: expected none) |

## 6. Upscaler compatibility (preparation only; no DLSS now)

> **Part BQ (UPSCALER-DLSS-READY):** this section is now specified in full there. Three statements below are corrected: temporal reprojection through a jittered depth (SSAO) takes the jittered matrices, not the unjittered ones; the `Velo` draws' b0 block carries the current-frame jitter shift on both matrices (it cancels in the vector); the readers of the inverse projection are listed (no pixel programme reads the off-centre rows of `FC_MatrixProj`). Where this section differs, the new sheet wins.

**Motion vectors:**

- **Need:** complete per-pixel motion vectors (camera and objects), without jitter, in
  **RG16F**.
  - The DLSS convention: pixels, or normalised with a scale, pointing from the current pixel
    to its previous position (`prev − cur`).
- **Gap:** the engine's velocity is RGBA8, scaled, clamped and NDC-based (§2–3), so it cannot
  be reused as is.
- **Native:** write a **second RG16F target** in the same passes, from the same VS outputs:
  - `cur/prev` clip positions, so no new geometry pass;
  - object rows write `prev − cur` in pixels;
  - the camera pass fills the background from depth (`g_mCurToPrevScreen`);
  - both are computed with **unjittered** matrices.
- **Owners:** `102380` (pass), `13BFC0` (camera), the `Velo` VS/PS (§3), the native pose ring
  (`game_bootstrap.cpp:1860`).

**Depth:**

- The main depth is standard Z: native `LESS_EQUAL`, near = 0 (`game_bootstrap.cpp:4219`); not
  reversed.
- DLSS takes it as is, with the "inverted depth" flag off.
- Use the readable-depth copy (NATIVE-PASS-ORDER rank 1) at render resolution.

**Sub-pixel jitter:**

- **Inject it** into the projection the scene rasterises with: the native camera constants
  (`ingress::SceneCameraConstants`, `game_bootstrap.cpp:4268` cbSceneParam image; `VC_MatrixWVP`
  per draw, `game_bootstrap.cpp:3639`). Add a pixel-space offset to the projection's
  `[2][0]/[2][1]` terms.
- **Keep unjittered:**
  - the velocity matrices (`VC_PrevMatrixWVP` and the current one used for motion);
  - YEBIS DoF/motion inputs;
  - the shadow cascades (their own projections, SHADOWS-NATIVE);
  - temporal reprojection: SSAO temporal (SSAO-NATIVE), the shadow cache reprojection.
- **Position reconstruction from depth:** SSAO, decals (DECALS-NATIVE), deferred lighting and
  fog read `FC_MatrixInvViewProj`/`FC_MatrixInvProj`. These must use the **jittered** inverse
  (the depth was rasterised with it), or subtract the jitter in screen space. List each reader
  before switching.

**Render vs output resolution:**

- **At render resolution:** G-buffer, decals, SSAO, shadows' screen-space mask, lighting,
  forward, water, effects, velocity (NATIVE-PASS-ORDER steps 3–11).
- **At output resolution:** after the upscaler. That covers YEBIS (tone map, bloom, DoF; or
  before it if DLSS takes HDR: decide then), the GX after-chain (AA is replaced by DLSS; glow
  composite), UI, menus, Bink and gamma (steps 12–15).
- **Owner:** the native target creation (`native/renderer/scene_pass.cpp`, `SceneRenderer`
  target formats `ModelTargetFormats()`).

**Camera cuts and teleports (history reset):**

- the engine's own reset of the camera velocity: `13BFC0` `obj+0x10` (first-frame flag);
- bonfire warps and map loads: the world reload path (MENUS-MAP gparam application
  `CC8180`, map load);
- cut-scenes: the gparam scene key re-set `88AF0` (INFERRED: called on view changes);
- menu views use their own pipeline.
- Reset DLSS history on any of these, and on a camera jump above a threshold.

**Transparency and forward:**

- these have no velocity: forward rows 0x1F/0x29, effects E01–E07, water, veils (§4,
  INFERRED);
- build a **reactive mask** from those draws (alpha coverage into an R8 target), and a
  transparency/composition mask for effects;
- **owners:** the forward rows (`game_bootstrap.cpp:4231`), the effect passes
  (EFFECTS-PHASE2), water (WATER-NATIVE).
