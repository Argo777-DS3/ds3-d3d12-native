# The menu views' environment regions (static read for tranche 28, D-0301)

Read 2026-11-21 by two `ghidra-research` subagents (GhidraMCP on DarkSoulsIII.exe 1.15.2, sha256 ef5e07c5…; read-only; raw scans of
`game/DarkSoulsIII.exe` in `.text`, no headless mirror). Not yet re-read by a fresh reviewer: a decision built on a link below is
re-read first. Addresses are 0x140000000-based, written short. STATIC = read; INFERRED = not read.

Why: Thomas's character's Quit Game portrait shows its armour's metal black natively, blue-silver in the original (I-0140 RESULT
« Tranche 27 »). Natively a view that does not trace clears SpecularAcc (I-0131 R10a removed the region pass; before it the pass ran
for the main view only, « the menu views make no region »). Thomas, 2026-11-21 03:44: « remets le portrait original » (D-0301).

## 1. The menu view makes its own region (STATIC)

- The offscreen views' base constructor `D65DD0` (Prof, Player, Face views) calls `D64C30`, then `D0C2F0(this+0x6D0 object)`
  (call at 140D66113).
- `D0C2F0` names `L"GILM%04d"` with 10 (format 142949C68), finds it with `TextureRegistry_FindByName` `D4ED20`, and when the env bank
  holds the key ({`GparamKindToArea`(1) = 1, 10}, `D0C9F0`; `41990` finds it) adds a region with `65D60` to the manager GXRS+0x42B0
  and stores its masks with `65320`: `0x200000011`, i.e. {0x11, 2}. `65D60` returns 0 while the manager is off (GXRS+0x42C0).
- The box: minimum 142949C80 = (-30, -30, -30, 1), maximum 142949C90 = (30, 30, 30, 1); the angle 0 (`CrtCosf`/`CrtSinf` with XMM0
  zero): a 60-unit cube at the origin, identity axes, in the scene's world space.
- Area 1 is `default.gparam`: its Env Map row 10 has ID 10 and ColorScale 1.0 (row 0: 1.5)
  (`evidence/I-0068/queries/gparam/gamedata__param_drawparam_default.gparam.tsv`). The cube's name: `3FEB0` builds the prefix
  `"" + "GILM"` for a non-map gparam (`D28490` gives `mAA_BB_` only for kinds 7/0xB), `6CED0` the name « %s%04d » = GILM0010
  (the 128x128 R16G16B16A16_FLOAT cube I-0113 tranche 35 found bound by the menu forward rows).
- The other region adders: `D0A860` and `D0CA20` (from `D0B8A0`, `936700`) with masks {0x11, 1}; `65790` (from `GXRoot_Init`
  48B80) fills the 0x200 entries with {0x11, 1} and bucket flag 1, then sets the manager on.

## 2. The deferred region pass and its per-view filter (STATIC unless marked)

- Select `67070(manager, passctx, view)`, called only from `1094F0` (at 140109C9E) in `RecordWorldPipeline` `103510`, when
  `*(int*)(view+0x78) == 1` (D64C30 sets it) and `view+0x3ECF` (set by `85540` when the flags' bit 27 and GXRS+0x42C0; the menu
  flags 0x8422014 have bit 27). The executor `675B0` filters nothing; it picks its depth and raster states by whether the camera is
  inside the box (`17B66B0` sphere test, then `17B5A60`).
- Per region of the list manager+0x38..+0x40, in order:
  1. `fade = region+0xC8`, or manager+0x2C0 when manager+0x2BD;
  2. `e = *(manager+0x58) + 0x18 * index` (`69940`: the region's handle index);
  3. masks: `(e+8 & passctx+0x4B8) && (e+0xC & passctx+0x4BC)`;
  4. frustum: the box (+0x10 half extents, +0x20/+0x30/+0x40 axes, +0x50 centre) through `(passctx+0x130)->vt[+0x58]`
     (thunk to `17BBC40` at this+0x160; an OBB against the camera frustum, the plane math not read: INFERRED);
  5. distance (only when fade > 0): the eye `*(passctx+0x418)+0x30`, its offset to the centre projected on the axes, clamped to the
     half extents, `dist` the length of the difference; refused when `fade <= dist`;
  6. bucket: refused when manager+0x2DA and not e+0x12 (+0x2DA is set to 0 by the constructor `65370` and no other store was found:
     never applies, INFERRED).
- The view's pair: `RecordWorldPipeline` writes passctx+0x4B8 from `CollectPassLightList` `FCE30`'s argument, built by the base
  scene's slot 4 `2059F0` as `{(pair.lo & 0xFFFFFF00) | 1, pair.hi}` from the composite's +0xE0/+0xE4 (`2046B0`). The composite's
  constructor `2043A0` gives {1, 1}; its slot +0x30 (`2046A0`, `mov [rcx+0xE0], rdx`) is called by `D64C30` with 0x200000000 =
  {0, 2}; the main scene (`DFEEB0`) does not call it.

| View | mask 0 | mask 1 | world regions {0x11, 1} | menu region {0x11, 2} |
|---|---|---|---|---|
| main | 1 | 1 | pass | refused (2 & 1 = 0) |
| D64C30's menu views | 1 | 2 | refused (1 & 2 = 0) | pass |

- So in a menu view only its own region is drawn (STATIC arithmetic). The forward rows' key choice `66C80` uses the same mask test
  with the pair of `GXSgDrawScene_Run` `C2EC0` (`*(drawscene+0x10)` +0x14/+0x18, the same layout, INFERRED): for the menu view the
  forward rows take {1, 10} from the same region, which the native already binds (`NativeMenuRegionIbl`, I-0113 tranche 35).
- The menu model's position is not read (`PortraitRenderCallback` D658B0 is encrypted at rest); the Status body view's eye (0.380,
  1.636, -2.513) (MENU-OFFSCREEN-MODELS) is inside the box; that the Quit Game portrait's camera is too (distance 0, the executor's
  camera-inside branch) is INFERRED from it, another view.

## 3. For the native (tranche 28)

- The removed code: `git show 2842bcbb7` (I-0131 R10a step 5: `EncodeNativeEnvRegions`, its root, pipelines, box, views, cube
  ranges and the region programmes) and `git show 9ffa3f20f -- native/frame/game_bootstrap/07_scene_lighting.inc`
  (`ProduceNativeEnvRegions`, CB_ENVMAP_REGION as K-0424, called only `if(!offscreen)`).
- For a menu view: the region list filtered as above with the pair {1, 2} (the old transcription applied no mask and no frustum
  test); natively the menu region exists only if `D0C2F0` ran with the manager on and GILM0010 registered (to witness: the menu
  view's selected regions, their keys and cubes). The native saw the manager off at the Face Detail's creation (I-0113 tranche 35,
  `09_model_admission.inc` NativeMenuRegionIbl): `65D60` then adds no region there, so the witness comes before the code.
  Witnessed by tranche 28 (I-0140 RESULT « Tranche 28 »): natively `D0C2F0` ran 11 times with GILM0010 registered, but its key
  gate `41990({1, 10})` failed (the engine's env-map bank holds only its default entry natively) and the manager was off at 10 of
  them; the fix answers the gate from the native banks and lets the native region registry take D0C2F0's call.
- Nothing changes for a traced view (the main view keeps SpecularAcc from the rays).
- Not settled: the writer of `drawscene+0x10`; the frustum plane math; encrypted writers of +0x4B8, +0x2DA, +0x10; the forward
  scales (4E2A0 writes the group's +0x140/+0x144 from the chosen entries' +0xC, 1.0 for {1, 10}, by the first read; the native
  takes the group's own keys' 1.5, OBSERVED on every sampled group by I-0113 tranche 14 (`07_scene_lighting.inc`): the two
  disagree, and the answer would touch the main view's ChooseNativeRegionIbl too).
