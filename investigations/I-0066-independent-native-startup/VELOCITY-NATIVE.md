# Velocity and motion blur natively (workstream started 2026-10-11, branch `claude/velocity-motionblur`)

Specification: I-0068 [VELOCITY-IMPLEMENTATION-CHECKLIST](../I-0068-static-yebis-ffx-lights-spec/VELOCITY-IMPLEMENTATION-CHECKLIST.md),
[VELOCITY-MOTIONBLUR-NATIVE](../I-0068-static-yebis-ffx-lights-spec/VELOCITY-MOTIONBLUR-NATIVE.md),
[LIVE-CHECKS-CONSOLIDATED](../I-0068-static-yebis-ffx-lights-spec/LIVE-CHECKS-CONSOLIDATED.md) and I-0067
[VELOCITY-SKINNING](../I-0067-static-menus-subsystems-map/VELOCITY-SKINNING.md).
Thomas (2026-10-11): velocity for the whole game (camera, characters, cloth, objects); MotionBlur may be enabled in the
options for tests, Thomas's graphics configuration saved before each run and restored after; motion vectors compatible
with a later DLSS, without jitter or DLSS now.
Witness place: Argo (Thomas's save, Load Game row 0), High Wall; `Run-PilotPair.ps1 -MotionBlur` (the runner sets
GraphicsConfig `MotionBlur` ENABLE after its hash-checked backup and restores every protected file by hash).

## Static 1186 (`evidence/I-0066/artifacts/velocity-static-1186/q1`..`q8`, Ghidra queries on a private mirror)

- **Clear** (`1400BE8E0` with the 102380 block): colour only. The block's depth pointer is null, so the velocity pass never
  clears a depth; the colour bytes give (0x7F, 0x7F, 0xFF, 0x00) / 255 = (0.498, 0.498, 1, 0).
- **Targets** (`85540`): the groups `pipeline+0xDF0` and `+0xE50` are created at half the render size (`w >> 1`,
  `h >> 1`), format 0x1C (R8G8B8A8_UNORM), with the depth object `pipeline+0x3980`; `+0xE20`/`+0xE30` are the DF0
  group's render target and depth, `+0xE08`/`+0xE68` the two groups' textures (dilation input and output).
- **Camera velocity draw**: GXBS::GXMotionBlurFilter (RTTI, vtable 143D527B8) slot 0 = `14013C7E0` dispatches the
  type-3 commands: count 1 -> `14013D810` (camera velocity), count 2 -> `14013DD50` (dilation), count 0 -> the GX blur.
  `14013D810(filter, context+0x460, depth object, record, intensity)` uploads cbFilterParam:
  `g_vDepthComputeParam` = (context+0x518, context+0x528) = (P[2][2], P[3][2]) of the projection,
  `g_mCurToPrevScreen` = the record's 4x4 transposed, `g_vIntensity` = (aspect x I, I, 0, 0) with aspect = camera+0x64
  or width/height, I = the record's intensity; programme `filter+0xE0` (MotionBlurWriteCMBVelocity, I > 0) else
  `+0xE8` (NoMove).
- **Intensity**: `13BFC0` records `filter+0x11C x [[GXRS+0x5368]+0xE0]`; `filter+0x11C` = pipeline+0x3E8C is written by
  `87FB0` from the MotionBlur gparam blend (`1D9910` word 17) = **Camera Blur Intensity** (words 15/16/17 = Motion /
  Object / Camera Blur Intensity, word 14 the Enable byte, word 18 Blur ReccurenceCount, bytes 0x4C-0x4E the three
  inner-simulation-time flags, word 20 Base Fps; field order of `default.gparam`).
- **Velo pixel programmes** (`AA630`, pass 1): the plain variant is the shared loose `GXFlver_Common_Velo` (repository
  callback 0), the alpha-tested one `<family>_Velo_[A]`; vertex programmes `<family>_Velo[_Skin][_Ins][@[cl]]`.
- **Velocity row** (`57A20` at `140058D25`): `view+0x595` and (descriptor byte 1 or draw +0xD) and the model category
  not 0x1B/0x1F and `[[GXRS+0x5360]+0x21]` -> `5C060(.., 0x34, ..)` with preparation 5, class mask 0xFFFF, 0xFF.
- **Velocity byte per class**: `D079C0` (SprjModelDrawEntity) `E+0xBA5 && E+0xBA6 && E+0x184`; `CFDE70` (ChrAsm parts)
  `E+0xBB0` bits 6 and 7 `&& E+0x184`.

## Reference1186 (`i66-reference-velocity-1186`, MotionBlur ENABLE, configuration restored by hash)

- Gates in game: option `+0x20` = 1, `+0x21` = 1, sample cap 5, factor `[[GXRS+0x5368]+0xE0]` = 1, YEBIS present,
  `pipeline+0x3EB9` = 1, `+0x3ED2` = 1, **`+0x3F70` = 0**.
- **The GX camera velocity pass never runs** (`13BFC0` and `13D810` not called once): with YEBIS present and
  `+0x3F70` clear, 102380 skips it, and YEBIS makes the camera motion itself (`CAB40`/`MakeCameraVelocity_*`). The
  velocity target holds the clear and the object Velo rows only.
- Objects (`vt+0x30` format): `+0xE08`/`+0xE68` R8G8B8A8_UNORM, 921,600 bytes = 640 x 360 x 4 at 1280 x 720 (half size);
  `+0x3980` R32G8X24_TYPELESS 1,843,200 bytes (half-size depth); `+0xFD0` the same format at full size.
- Velo draws exist: the material preparation counts pass 1 (Velo) 189,138 times against 1,586,707 Gbuf (12 %).
- Pose rings (`16050`): 403 owners with ring 1, 59 with ring 2 (keepPrevious).
- Velocity bytes (`D079C0`): `E+0xBA5`/`E+0xBA6` 1 for every SprjModelDrawEntity seen; `E+0x184` varies by entity and over
  time (the player 1; NPCs 1 then 0, or 0).
- Velocity byte per class, completed: `D3DA20` (SprjFD4ModelItem children of a SprjModelDispEntity: map pieces and
  objects) copies the disp entity's `+0x388` bit 1.

## Reference1187-1188 (`i66-reference-velocity-1187`, `-1188`; MotionBlur ENABLE)

- `57A20`'s view (`param_2`): `+0x595` = 1 on the world views, 0 on the title view; its `+0xCA4` reads 0. It is not the
  object 5A120 scales with.
- **Velo preparation** (`5A120` case 5, hooked after the call): `param_1` is the pass context (its `+0x418` camera, the same
  object 13BFC0 reads), `+0xCA4` = **0.1** on every sampled context (title and game, 60,000 preparations), so
  **FC_DifColMul = (8.889, -5, 0, 1)** at 16/9 (k = 10). The constant 1.0E-5 (`143D7ECF4`) only replaces a non-positive
  `+0xCA4` (not a maximum). Static writers of `+0xCA4`: only `139CE0` (1.0) and the copies `139F40`/`13AA60`; the 0.1 writer
  is open (question to the static threads).
- **Quality row**: GraphicsConfig MotionBlur ENABLE = level 3 (`[4796298]+0xB8`, copied by `CED9A0` to `GXRS+0x5330`), row
  (1, 1, 5) of the quality table at `+0xA0 + 8 x level` (`219B0` fills it: levels 0-4 = (0,0,0) (1,0,1) (1,1,1) (1,1,5) (1,1,8)).
  `220A0` builds `GXRS+0x5358`/`+0x5360` from it; `23790` refreshes `+0x5360` when a level changes.

## Native1187-1188 (`i66-native-velocity-1187`, `-1188`)

- 1187: no velocity: natively `[GXRS+0x5360]` does not exist (PublishNativeSceneRoot builds the root through 213A0 and
  127010 only). Fixed in 1188: the row is read from the genuine table (`219B0`, called once on a native buffer) at the options'
  level (`NativeMotionBlurQualityRow`); `[GXRS+0x5368]+0xE0` (the 127010 object, entry 6, default 1.0) is read as is.
- 1188: level 3, row (1, 1, 5), factor 1; 13 selected models carry the velocity byte (player parts, NPCs); 81 Velo draws per
  frame after the first pose updates (the first frames refuse parts without a pose yet). The velocity picture
  (`velocity-2400.png`: R/G = motion, blue = object velocity) shows Argo's whole silhouette covered by her Velo rows with no
  depth-test holes (bias policy). The selection's view reads 0 at `+0xCA4`, so k was 1.0E5 there and every object velocity
  saturated (object motion vectors at most 0.01 px).

## The view scale, closed (I-0068 VELOCITY-CA4-WRITER, static thread)

5A120's `param_1` is the GX view `140024740` builds on its stack (ctor `139CE0`); its `+0xCA4` is stored there: for the
YEBIS (world) view `[144962158]` = max(field [3] **MaxBlurLength** of the blended MotionBlur gparam, 0.001) (`E4B50`);
otherwise 0.2 / max(`pipeline+0x3E88`, 1E-6). MaxBlurLength is 0.1 in every GPARAM file that has the group, so k = 10.
Native (1191): produced from the pipeline context's MotionBlur keys (`+0x4008`, `+0x4070`, weight `+0x40C0`, blended as
`1D9910`) over the admitted GPARAM files (`Banks::GroupFloat`: the area's file, mapbase when its group is empty, `{1, 0}`
for a missing key): keys (-1, -1) at the High Wall start -> default.gparam, 0.1, FC_DifColMul (8.889, -5, 0, 1) at 16/9,
equal to Reference1188.

## Native1189-1191 (`i66-native-velocity-1189`..`-1191`)

- 1189: the Load Game confirmation failed natively (known pilot flakiness); clean close, configuration restored.
- 1190 (k from the measured 0.1) and 1191 (k from the gparam): 81 Velo draws per frame (92 when an NPC enters the view),
  every sampled frame complete, 0 debug errors, 0 faults, close exit 0xD3126606 (the known env-region refusal).
- Motion vectors (sampled every 30th frame): camera still, object pixels 0.1-0.3 px (idle animation) and camera pixels
  below 1E-4 px; during the pilot's camera turns 10-35 px camera motion and 13-28 px object motion; one 50-degree turn
  in a single native frame gives up to 2,346 px (a genuine reprojection under the 60-degree cut policy).
- Pictures (R/G = 128 + 4 x motion in pixels, blue = object velocity): `i66-native-velocity-1190/velocity-2340.png` (camera
  turning: a smooth background field, Argo with her own small motion), `velocity-1980.png`/`-2250.png` (still), 
  `i66-native-velocity-1191/velocity-2160.png` (fast turn). Argo, her cape (cloth Velo VS with `VC_World` current and
  previous) and her sword are covered without depth-test holes.

## Decisions and policies of this workstream (D-0162)

- **Camera velocity.** With YEBIS present the original never draws the GX camera pass (Reference1186-1188): its velocity
  target holds only the object Velo rows, and YEBIS makes the camera motion itself. The native target therefore holds the
  clear and the Velo rows only, as the original; the camera motion lives in the native motion-vector resolve (from the scene
  depth and 13BFC0's current-to-previous matrix), not in a GX pass the original does not run.
- **Target.** R16G16B16A16_UNORM at render resolution (the original's encoding, more bits, full size) instead of the
  original's half-size R8G8B8A8 group (`pipeline+0xDF0`, depth `+0x3980`); the Velo rows test the full-size scene depth
  (bias -64, slope -2) instead of the half-size copy. The YEBIS motion blur, when it lands, receives the half-size RGBA8
  target from a 2x2 reduction of this one (its input contract), at the dilation step.
- **Motion vectors (upscaler preparation).** R16G16_FLOAT, previous minus current position in pixels, render resolution,
  unjittered (no jitter exists natively). Object pixels (alpha 1) decode their Velo encoding with the rows' FC_DifColMul;
  the other pixels take the camera reprojection of their depth. A cut (new view, reset, more than 10 m or 60 degrees between
  two selections) restarts the previous view-projection from the current one (zero camera motion): the history-reset
  trigger for a later upscaler.
- **Quality row.** Natively the root has no `+0x5360`; the MotionBlur row is read from the genuine table (`219B0`) at the
  options' level. GraphicsConfig tokens are DISABLE/LOW/MEDIUM/HIGH/MAX (MENUS-MAP 77; "ENABLE" falls back to HIGH):
  LOW draws no object rows (row (1, 0, 1)); DISABLE (row (0, 0, 0)) gives no velocity at all natively.

## Checklist status (I-0068 VELOCITY-IMPLEMENTATION-CHECKLIST)

| Step | Status |
|---|---|
| 1.1 gates | done: quality row (option `+0x20`/`+0x21`, cap), factor `[GXRS+0x5368]+0xE0` read; `view+0x595`: the world view only (the native selection's view object reads 0; Reference: 1 on world views, 0 on the title) |
| 1.2 MotionBlur gparam | MaxBlurLength produced (view scale); the 14 YEBIS globals belong to the YEBIS step |
| 2.1 targets | done, policy above (format and size measured: Reference1186) |
| 2.2 clear | done: colour only, (0.5, 0.5, 1, 0) (the original's (0x7F, 0x7F, 0xFF, 0) on 8 bits) |
| 3.1-3.3 previous camera, b0, reset | done (b0 {VC_MatrixWVP, VC_PrevMatrixWVP} absolute, unjittered; reset policy above) |
| 4 camera pass | not drawn, as the original with YEBIS (`+0x3F70` = 0); camera motion in the resolve |
| 5.1-5.3 previous palettes | done: ring 2 owners publish the replaced version as previous, ring 1 bind the current one (b6 = b5); b6 laid out like b5: the raw blend indices address it because the native instance record binds every palette from row 0 (matricesData.x = palette_base 0) |
| 5.4 rigid draws without a pose owner | not reached: every selected draw has a pose owner; `DC+0x198`'s writer stays live-only |
| 6.1-6.3 Velo rows | done: row 0x34 per class velocity byte (D079C0, CFDE70, D3DA20), programmes (VS family, PS Common_Velo or `_Velo_[A]`), FC_DifColMul, state |
| 6.4 coverage | class mask 0xFFFF; effects, water, decals and 2D draw none (natively absent anyway) |
| 6.5 E+0x184 | read natively per entity; 13 models carry the byte at the High Wall start |
| 7 dilation, 8 YEBIS blur | wait for the native YEBIS (P2): both run only with YEBIS and feed only it |
| exit: debug layer clean 600 frames | done (Native1190/1191, more than 600 frames with velocity, 0 debug errors) |
| exit: DISABLE draws nothing | by construction (row (0, 0, 0)); not run live (Thomas keeps MotionBlur on) |

## Pre-merge review (fresh agent, 2026-10-11) and Native1192

Findings fixed: (1) the target and resolve were gated on the row byte `+0x21`; now on the pass byte `+0x20` (102380's
gate), the 0x34 rows on `+0x21` (LOW gives camera motion vectors without object rows); (2) a view-projection that cannot be
inverted no longer ends the process (`Refuse`): that frame has no velocity and the history restarts; (3) the velocity device
objects are released in `SceneRenderer::Close` (a reopened renderer recreates them); the witness picture is allocated before
the readbacks are mapped. Checked clean by the review: no dead code, barriers and between-frame states, mandate (no D3D11),
draw lifetime, b0 column storage, b6 path, resolve maths, locks, recipe arrays, pose ring, no per-asset case. Doubts kept
open: float32 absolute view-projection against the depth bias far from the origin (holes absent at the High Wall), the cut
policy for a later upscaler. Native1192 after the fixes: 17 sampled frames, 81 Velo draws, camera turns 9-27 px, 0 debug
errors, 0 faults, configuration restored.
