# I-0068 part AI: the velocity and motion-blur workstream, one implementation checklist

Request (coordinator, 2026-10-10): an ordered checklist for the next workstream, in the form of
EFFECTS-IMPLEMENTATION-CHECKLIST. It merges:

- VELOCITY-MOTIONBLUR-NATIVE (I-0068 part AE);
- I-0067 VELOCITY-SKINNING §1–8 (on `main`).

Each step gives its preconditions, its checks and its witnesses:

- **M** = reference captures with MotionBlur ENABLE (VELOCITY-MOTIONBLUR §5);
- **ACC** = acceptance after the step;
- **NOW** = readable today on the original.

The upscaler part is kept apart (§10), as preparation only.

**Witnesses** (LIVE-CHECKS-CONSOLIDATED §1–2):

- TestAnim's save at Firelink (`m40_00`): M1, M2, M4;
- Argo's save on the High Wall (`m30_00`): M3, an NPC and cloth;
- `m31` or `m45`: M5, effects and water in view;
- GraphicsConfig `MotionBlur` = ENABLE for every row.

## 0. Two points settled while merging (STATIC)

**1. The previous camera is bound per draw.**

- I-0067 VELOCITY-SKINNING §1 reads "no previous camera matrix".
- The DXBC of `GXFlver_ColDifSpcBumpIbl_Bloody_Velo[_Skin].vpo` (the family it cites) declares
  `VC_PrevMatrixWVP` at b0 +64 and uses it: `dp4 o2.*, …, cb0[4..7]` (evidence
  `evidence/I-0068/queries/velocity/vv/`). The same holds for `ColDifSpcBumpIbl_Velo[_Skin]`.
- So each `Velo` draw needs **b0 = {current WVP, previous WVP}**. The full-screen camera pass
  (step 4) covers only the background.

**2. The three velocity passes follow each other.**

| Group | What | Source |
|---|---|---|
| **0x33** | targets, clear, camera velocity | `102380`, `13BFC0` |
| **0x34** | the object `Velo` rows: engine pass 0x34, preparation 5, class mask 0xFFFF | VELOCITY-SKINNING §7 |
| **0x35** | dilation | `13BD10` |

**Two option words are read, and both must be mirrored:**

| Word | Read by | Gates |
|---|---|---|
| `[[GXRS+0x5360]+0x20]` | `103510`, `E4B50`, `13BFC0` | the pass and YEBIS |
| `[[GXRS+0x5360]+0x21]` | `57A20` | the object row |

INFERRED: both follow the MotionBlur option. M1 reads both.

## 1. Preconditions (outside this workstream)

| # | Precondition | Why |
|---|---|---|
| P1 | readable depth (NATIVE-PASS-ORDER rank 1) | the camera pass reads `g_DepthTex`; the dilation and YEBIS read depth |
| P2 | **YEBIS tone map and the HAL** (rank 3; YEBIS-HAL-READY) | the blur itself is YEBIS (`tech_MotionBlur*`). Without YEBIS, steps 1–8 can be built and checked on the velocity target alone |
| P3 | NPC bodies (pilot branch) | character velocity and M3 |
| P4 | native pose owners with keepPrevious (ring of two) | `game_bootstrap.cpp:1860-1864` already supports `ring = previous ? 2 : 1` |

## 2. Gates (step 1)

| Step | Work | Checks | Witness |
|---|---|---|---|
| 1.1 | mirror the gate chain each frame: option `+0x20`/`+0x21`, factor `[[GXRS+0x5368]+0xE0]` > 0, pipeline capability `+0x50` bit 5 → `+0x3EB9` (`885F0`), per-frame `+0x3ED2`, `view+0x595` | **M1:** read the five words on the original with MotionBlur ENABLE and DISABLE. ACC: the native gate equals the original's frame by frame (log) | Firelink |
| 1.2 | the gparam **MotionBlur** group: bank `[GXRS+0x4640]`, key index 6 (`frame+0x4008/+0x4070`, weight `+0x40C0`), blend `1D9910` → the 14 globals of `E4B50` (`14496214C..1449622C0`) | ACC: the native globals equal the original's at the same spot (M1 read). `default` id 0 and `mapbase` id 0 values are in VELOCITY-MOTIONBLUR §1 | Firelink |

## 3. Targets and previous camera (steps 2–3)

| Step | Work | Checks | Witness |
|---|---|---|---|
| 2.1 | velocity colour target (`pipeline+0xE20`), its depth (`+0xE30`), the object given to YEBIS (`+0xDF0`). INFERRED: RGBA8 UNORM | **M1:** the format and size of `+0xE20`/`+0xE30`/`+0xDF0` (render or half size?) | Firelink |
| 2.2 | the clear: colour (0.5, 0.5, 1, 0), depth 1.0 (`1400BE8E0` block `0x007F7FFF`, 1.0) | M1: the clear values | Firelink |
| 3.1 | **previous view-projection:** keep last frame's unjittered view-projection per view | ACC: on a still camera, previous = current (velocity 0.5/0.5 everywhere) | Firelink |
| 3.2 | **b0 of every `Velo` draw** = {current WVP, previous WVP}. The native view block holds the current one only (`game_bootstrap.cpp:3639`) | **M2:** b0 bytes of a `Velo` draw: +64 = the previous WVP | Firelink |
| 3.3 | reset: first frame after a load, warp or cut | **M4:** `13BFC0` `obj+0x10` = 0 → previous := current (NoMove); YEBIS history | Firelink bonfire warp |

## 4. Camera velocity (step 4, group 0x33)

| Step | Work | Checks | Witness |
|---|---|---|---|
| 4.1 | `13BFC0` → full-screen `MotionBlurWriteCMBVelocity` (t2 depth, s3, b0: `g_vDepthComputeParam` +32, **`g_mCurToPrevScreen`** +48, `g_vIntensity` +128 = `obj+0x11C × factor`). It runs when `+0x3EB9` ∧ (`+0x3F70` ∨ no YEBIS) | **M1** (camera rotating, player still): the matrix and intensity bytes; ACC: the velocity target equals the original's (mean abs diff ≤ 2/255) | Firelink |

## 5. Previous palettes (step 5; VELOCITY-SKINNING §2–5, §8)

| Step | Work | Checks | Witness |
|---|---|---|---|
| 5.1 | **ring per pose owner:** two versions when keepPrevious (`SprjModelDrawEntity` `E+0xCA` bit 4; ChrAsm `E+0xBB0` bit 6; `87AEA0` none); advance, then publish (`16050` order); first frame previous = current; up to 590 bones | NOW: `E+0xA80+0x6C` (ring) and `+0x68` (index) on a character over two frames (VELOCITY-SKINNING §4); ACC: the native ring index advances once per pose update | High Wall (Argo, NPC) |
| 5.2 | **binding:** b5 = current, b6 = previous (`64340`). Per mesh or whole skeleton by `model+0x1E0`. Cloth and auxiliary pairs (current and previous) when material `+0x24` bit 14, or cloth pass ∧ `e+0x22` ∧ `P+0x76` | **M2/M3:** the two buffers bound for a `Velo` draw; the cloth Velo VS reads b2 `VC_isEnableCloth` and b7 `VC_World` | Firelink (player), High Wall (cloth NPC) |
| 5.3 | **b6 indexing:** the Velo VS indexes b5 with `BLENDINDICES + matricesData.x` and b6 with the raw `BLENDINDICES` (DXBC, VELOCITY-MOTIONBLUR §3). Lay b6 out so that raw indices address the previous rows | ACC: for a skinned mesh with `matricesData.x` ≠ 0, the previous rows land on the same bones (zero velocity on a still character) | High Wall |
| 5.4 | **rigid draws without a pose owner:** the previous rows pointer `GXFlverModelDC+0x198` (writer open, §8). Native: the entity's previous placement | NOW: write breakpoint on `DC+0x198` during a rigid `Velo` draw (VELOCITY-SKINNING §8) | Firelink (a moving object) |

## 6. Object velocity rows (step 6, group 0x34)

| Step | Work | Checks | Witness |
|---|---|---|---|
| 6.1 | the row: engine pass **0x34**, preparation 5 → shader pass 1 `Velo`, class mask 0xFFFF. It is added only when `view+0x595` ∧ draw `+0xD` (= `E+0xBA5 ∧ E+0xBA6 ∧ E+0x184`) ∧ group ∉ {0x1B, 0x1F} ∧ option `+0x21`. Native: the row list (`game_bootstrap.cpp:3869`), `EvaluateOrdinaryModelPass` (shader pass 1, preparation 5) and `SelectModelPrograms` (pass name `Velo`; today only passes 0/6/7, `model_programs.cpp:40-48`) | **M2:** the `Velo` draws, their programmes, sub-order after the camera pass; ACC: native draw count = original at the same view | Firelink (player running) |
| 6.2 | programmes: `<family>_Velo[_Skin][_Ins][@[cl]].vpo`; PS `GXFlver_Common_Velo.ppo` (opaque) or `<family>_Velo_[A].ppo` (alpha test). Output `RG = Δndc·FC_DifColMul.xy·0.5 + 0.5` (saturated), `B = w/(w+10)`, `A = 1` | build b7 `FC_DifColMul` = (aspect·0.5·k, −0.5·k, 0, 1), k = 1/view+0xCA4 (VELOCITY-MOTIONBLUR §3). **M2:** its bytes in a `Velo` draw equal this (aspect = width/height, k = 1) | Firelink |
| 6.3 | state: depth test against `+0xE30`; colour to `+0xE20`; opaque | M2: the depth target bound in group 0x34 (main depth or `+0xE30`) | Firelink |
| 6.4 | coverage: class 0xFFFF admits blended classes (VELOCITY-SKINNING §6–7). Effects, water, decals and 2D draw none | **M5:** the draws of group 0x34 with forward, effects and water in view | m31 or m45 |
| 6.5 | `E+0x184` per-frame flag (writer open, §7) | NOW: read `E+0x184` of the player and an NPC with MotionBlur ENABLE | High Wall |

## 7. Dilation (step 7, group 0x35)

| Step | Work | Checks | Witness |
|---|---|---|---|
| 7.1 | `13BD10` (with YEBIS and `view+0x9D0`) → `MotionBlurDilateVelocity` (t1 velocity, b0 `cbFilterParam`), inputs `pipeline+0xE68`/`+0xE08`, count and intensity from the MotionBlur gparam | **M1/M2:** one dilation draw per frame, its inputs and constants; ACC: the dilated target equals the original's | Firelink |

## 8. YEBIS motion blur (step 8)

| Step | Work | Checks | Witness |
|---|---|---|---|
| 8.1 | the YEBIS calls of `E69A0`: `SetMotionBlurEnable` (`CAA20`, enable ∧ velocity object), `CAAA0`, mode `CAA60` (2 = application velocity), source `CAB90(1, pipeline+0xDF0, w, h)` (else `CAB40(0, 0, −2)`, auto camera), `CAAE0`; current/previous view `C95B0`/`C95F0`, projections `C9590`/`C95D0` | **M1:** the YEBIS technique sequence with motion blur (`tech_MotionBlurVelocity*`/`…Dir…`/`…Depth…`, `MakeCameraVelocity_*` if auto) and its place among the 41 m40 calls (YEBIS-HAL-READY §8) | Firelink |
| 8.2 | the image | ACC: the back buffer during a camera turn against the original (same path): blur length and direction; no blur on a still camera | Firelink |

## 9. Order and exit

**Order:** 1 → 2 → 3 → 4 → 5 → 6 → 7 → 8.

- Steps 1–7 can be built and checked before YEBIS (P2): the velocity target is visible on its
  own.
- 8 needs YEBIS.

**Exit:**

- M1–M5 read once on the original, then matched on native;
- the debug layer is clean over 600 frames at Firelink with MotionBlur ENABLE;
- with MotionBlur DISABLE, native draws nothing of steps 2–8, exactly as today
  (REMAINING-FAMILIES §2).

## 10. Upscaler preparation (kept apart; no DLSS work now)

> **Part BQ:** specified in [UPSCALER-DLSS-READY](../I-0066-independent-native-startup/UPSCALER-DLSS-READY.md); items 3 (the temporal passes) and 6 (the coverage target) are corrected there.

Source: VELOCITY-MOTIONBLUR §6. The items to prepare while building steps 2–6, without
implementing an upscaler:

1. **Motion vectors:**
   - a second **RG16F** target written by the same passes (camera pass and `Velo` rows);
   - `prev − cur` in pixels;
   - **unjittered** matrices.
2. **Depth:** the readable-depth copy at render resolution. Standard Z (`LESS_EQUAL`, not
   reversed).
3. **Jitter:**
   - **inject it** into the scene projection (native camera constants, `game_bootstrap.cpp:4268`,
     `:3639`);
   - **keep unjittered:** step 3's previous/current matrices, shadows and the temporal passes;
   - **reconstruct positions** with the jittered inverse (SSAO, decals, lighting, fog).
4. **Resolutions:**
   - render resolution: steps 3–11 of NATIVE-PASS-ORDER;
   - output resolution: YEBIS, the GX after-chain, UI, gamma.
5. **History reset:** the same triggers as step 3.3 (`13BFC0` `obj+0x10`, warps, loads, cut-scene
   key changes).
6. **Reactive mask:**
   - from the draws without velocity (6.4): forward, effects, water, veils;
   - an R8 coverage target written by those draws.
