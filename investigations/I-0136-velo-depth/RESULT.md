# I-0136 - La précision de profondeur des lignes Velo: result

Branch `claude/velo-depth`, from `main` `77746bcf`, 2026-11-12 from 08:00 (Paris). Question and scope: [CONTRACT](CONTRACT.md).

## Cause (READ, the mechanism INFERRED)

- The Velo rows (engine pass 0x34) are the game's own programmes: they project absolute world positions through an absolute
  view-projection (`VelocityMatrixBlock`, 12_scene_collect.inc) where the G-buffer programmes subtract `SC_CameraPos` first and
  project camera-relative positions. They were tested against the scene depth (LESS_EQUAL, unwritten) with a constant bias of -64
  and a slope bias of -2 (13b).
- I-0134's Fable 5.2 read (INFERRED, a float32 emulation): at Firelink's coordinates (|camera| about 650) clip z and w carry
  1e-5 to 1e-4 more rounding, a few hundred depth ulps at 3.5 m against the bias of 64; flat triangles fail whole, sloped ones pass,
  and the failure flips between about 0 and 20 % of a character's outline with the matrix's low bits (OBSERVED in earlier runs:
  I-0134 RESULT, `evidence/I-0134/holes.txt`).

## As built

- 13b: the rows draw into a depth of their own: written, LESS_EQUAL, no bias (the nearest row wins). `velo-depth-legacy.flag`
  (a test tool, `Run-StartupTrace.ps1`'s list) keeps the former test against the scene depth with its bias; one flag drives the rows'
  state and the renderer's choice (`NativeVelocityInput::own_depth`).
- The renderer (`scene_pass_velocity.cpp`): the rows' depth has the scene depth's format and render extent (DEPTH_WRITE between
  frames, remade with the scene depth's generation, declared to the frame graph as `velocity_depth`); after the rows, an occlusion
  pass reads both depths' plane 0 and clears a row's pixel back to the clear value (0.5, 0.5, 1, 0: the camera's motion) only where
  the scene depth is nearer than the row's by more than 2 e / w, w the view distance from the scene depth (d = zs (1 - n / w), zs =
  f / (f - n)), e = 8 ulp(|camera| + w): the rows' extra rounding twice over at Firelink (e = 4.9e-4 there, the read's measure at
  most 1e-4), plus a slope term: the rows' clip x and y round too, a shift of up to 2 e / w in NDC (e w' / w pixels for a target w'
  wide, at least half a pixel), which on a grazing surface moves the depth by the surface's step a pixel; the step is the smaller
  one-sided difference of the scene depth a direction (across a silhouette, the near side's own step, not the jump behind it; at the
  image's border the one that exists). A known limit: an occluder one pixel thin (a blade, a chain, a strand) has the jump on both
  sides, so a hidden row within half the jump behind it keeps its motion there. The pixel is
  kept at the sky, without a lens (zs <= 1) and where the row is nearer than the scene (as before).
- The leak this tolerance allows behind a nearer surface (the step review's estimate, near 0.1 m as documented for the main camera,
  not read at run time): about 1 % of the distance with |camera| between 512 and 1024 (3.4 cm at 3.5 m, 49 cm at 50 m), twice that
  between 1024 and 2048; the former bias allowed about 0.5 mm at 3.5 m and 9.5 cm at 50 m. Only a camera-relative re-basing of the
  rows' world rows and palettes (excluded) would remove both the rounding and the leak.
  The slope term adds (step_x + step_y) x shift, at least half of each direction's smaller one-sided step of the scene depth: on a
  grazing visible surface (a floor, an oblique wall) it can exceed the 2 e / w term, and it grows with e / w x the extent (more than two
  pixels of shift at |camera| 4096 and more near the camera). It replaces the former slope bias of -2 (two depth steps of the row's own
  surface a pixel, against the scene), so where the scene's surface is the row's own the two are of one size; behind another surface
  it is that surface's step, not the row's. No run measured the leak: the holes' reader counts only missing row pixels, which a looser
  tolerance lowers by construction (the integrity review's note).
- Behaviour changes: the rows among themselves now keep the nearest (before: every row passing the scene test wrote, in draw order;
  a translucent part in front of a moving character now always gives its own motion, the step review's L1); a Velo row drawn without
  a depth test, if the game has any, is now tested (none found, L2).

## Reviews

- **Step review** (a fresh Opus at high effort, `5c795455`): H1 the rows' depth undeclared to the frame graph (every frame a
  mismatch, the graph's executor never running): declared, but its use as read-write still failed the plan (the final review's H1). L6 the occlusion pass's creation after the block's guard (a failure not
  retried): its own guard. L7 stale comments: rewritten. Kept and stated here: L1, L2 (behaviour changes above), L3 (|camera| + w
  underestimates |p| at the screen's corners by up to about 2x: inside the 8-ulp margin), L4 (the leak above), L5 (the rows' depth
  allocated with the legacy flag too: about 66 MB at 3840x2160, the test flag's cost), L8 (the scene depth read twice on frames
  without NRD: small), L9 (the flag list's conflict with main: resolved, both flags kept). Criteria 1 to 5 held otherwise.

- **Optimisation pass** (read): one depth clear and one full-screen pass a frame with velocity (five loads a pixel where a row wrote);
  the rows' depth one more target (D32S8 at the render extent); the scene depth's plane 0 read twice on frames without NRD when the
  graph's executor does not drive them (kept: small); nothing per draw.

## Runs

Series 1 and 2 ran earlier forms of the tolerance (series 1 none of the slope term, series 2 the quad derivatives'); series 3 the
merged form (the one-sided step, the border case, the shift 2 e / w).

`series-i136-velo.ps1` (`evidence/I-0136/`, 08:36-08:46 on 2026-11-12, the ASUS 1920x1080 window, offline; the `build-a` folder as
rebuilt from `9d5a34ad` merged with main's tranche 41 (`7b2a87c1`), `build-b.log`; I-0134's save, `thomas-alpha-cape-20261111-124806`, restored by hash; I-0134's keys and
hold, the TAA, `world-pictures.flag`): `i136-velo-legacy-1`, `-own-1`, `-legacy-2`, `-own-2` (legacy: `velo-depth-legacy.flag`).
All closed clean (exit 0, no fault, 7,010 to 7,524 native frames, restored). The frame graph (its final review's H1, OBSERVED in the
journals' `graph_shadow`: frames, mismatch frames, uses, executed frames): legacy-1 [4443, 0, 24440, 4430], legacy-2 [4504, 0,
24586, 4491]; own-1 [4372, 4372, 739081, 0] and own-2 [4514, 4514, 760986, 0], with 2,359 and 2,424 rows of a plan error 8 (a
transient's first use a read: the rows' depth declared read-write after an undeclared clear): with the own depth the graph's
executor never ran and the hand-written barriers did the work (the GPU's work correct, the graph's check lost). An earlier version of
this section read a different counter (the model draws' state reuse, `"mismatches":0`) as the graph's: wrong, corrected here.

- **The holes (OBSERVED, `evidence/I-0134/holes.py` on the 40 velocity pictures, `evidence/I-0136/holes.txt`: unwritten pixels
  enclosed by the outline):** `legacy-1` 15.00, 16.85 and 21.24 % at frames 2520, 2610 and 2700 (the bad regime), 0.31 to 1.83 % at
  its other seven; `legacy-2` 0.38 to 1.07 % (the good regime all through); `own-1` 0.09 to 1.85 %, `own-2` 0.30 to 1.68 %: no picture
  of the own-depth runs above 1.9 %. The remaining 0.1 to 1.9 % is the good regime's floor in both modes (and I-0134's before-runs,
  0.41 to 1.81 %): INFERRED to be real gaps of the outline (between the legs and the cape's tatters), not inspected one by one.
- **Series 2** (`series-i136-velo2.ps1`, 09:27-09:32, `build-c` from `f35def1d`, the final review's fixes; the same save, keys and
  window): `i136-velo-own-debug` under the D3D12 debug layer and `i136-velo-own-3` without it, both clean (exit 0, no fault, 6,900
  and 7,236 native frames, restored). The frame graph (`graph_shadow`): [4291, 0, 24059, 4278] and [4639, 0, 25022, 4626], no plan
  error: no mismatch frame, the executor on 4,278 of 4,291 and 4,626 of 4,639 frames (a new plan key compared on its first frames,
  by design; `graph_barriers` 94 to 106 barriers a frame in the world windows, as the legacy runs'); the debug
  layer's errors 0 in every audit row (`"debug_errors":0`). Holes (`holes2.txt`): 0.30 to 1.89 % and 0.35 to 1.61 %, the same floor.
- **Series 3** (`series-i136-velo3.ps1`, 10:14-10:19, `build-c` rebuilt from `065519a2`, `build-f.log`; the same save, keys and
  window): `i136-velo-own-5` under the TAA (the rays' GI with NRD: `native_rt_gi` 4,596 frames, 4,596 with NRD, the rows before the
  lighting) and `i136-velo-own-dlaa` under DLAA (Ray Reconstruction on 3,600 frames, NRD on none: the rows after the forward rows, the
  other call site), both clean (exit 0, no fault, 7,244 and 7,333 native frames, restored). The frame graph: [4640, 0, 25186, 4627]
  and [4687, 0, 27329, 4674], no plan error. Holes (`holes3.txt`): 0.35 to 1.58 % and 0.17 to 1.34 %, the same floor.
- **By eye** (`run1/legacy2700-own2700-own3060.png`: legacy-1's frame 2700, own-1's 2700 and 3060, brightened): the legacy picture
  riddled with flat triangles, the own-depth ones solid.
- Series 1 and 2 drew every world frame's rows at the NRD call site (`native_rt_gi` frames equal to its NRD frames; the own-debug
  run's graph summary has `velocity` and `velocity_occlusion` on 4,201 of 4,201 frames); series 3's DLAA run the other. Not run: a farther zone
  (larger coordinates: the doubled tolerance, and at |camera| 4096 and more a pixel shift above two pixels near the camera); the PT
  level, DLSS's other modes; a scene where an occluder hides a moving row (the leak). Two runs a mode: the regime is random, so the legacy mode's bad regime in one run of two is not
  a rate.

- **Final review** (a fresh Opus at xhigh, `main...0aa84d4a`): **H1, not merged:** the frame graph's plan failed on every own-depth
  frame (above): the rows' depth now a written use in its first pass. H2 this RESULT read the wrong counter: corrected above. M1
  `Close()` left the four new objects (a reopened device would bind the old device's pipeline and heap): released. L1 the rows' clip
  x and y round too (a fraction of a pixel's shift, a depth error on grazing surfaces the former slope bias covered): half the scene
  depth's step to a neighbour added to the tolerance. L2 `own_depth` defaulted to the legacy mode: true by default. L3 the occlusion
  pass never ran under the debug layer: series 2 runs one. L4 the legacy state set then overwritten: in an `else`. L5 the rows'
  depth D32S8 (8 bytes a pixel) for a depth alone: kept (the rows' pipelines keep the scene's depth format). L6 the build's naming:
  corrected above.

- **Second final review** (a fresh Opus at xhigh, `main...9dfe3e2c`): nothing High. M1 the slope term from the quad derivatives took
  the jump across an occluder's silhouette (a hidden row's motion kept along every silhouette): the smaller one-sided step, times the
  rows' pixel shift. M2 « NRD not run » was the reverse (every world frame at the NRD call site): restated above, the other call site
  named as not run. L1 the barriers' count and the executor's frames: restated. L2 the occlusion's target now read-write, the pass
  declared before its clears. L3 the half-pixel cover only Firelink's scale: the shift now grows with e / w. L4 (the non-NRD frames'
  double transition, a D32 own depth: kept), L5 (the integrity review: next), L6 the legacy path a second production path, kept as
  the A/B tool until a leak witness runs: stated.

- **Step review of the slope term** (a fresh Opus at high effort, `9dfe3e2c..34e3d68e`): nothing High or Medium; criteria 1 to 4
  held. L1 the border pixels lost the slope term (the clamp made one difference zero): the existing difference taken there. L2 the
  shift half the depth term's derivation (x and w both round): 2 e / w. L3 a one-pixel occluder keeps the jump: stated in « As
  built ».

- **Integrity review** (D-0268, `main...edf40d15`): nothing High; the numbers it sampled match the evidence. M1 the leak stated for
  the depth term only: the slope term's part stated (« As built »). Lows: series 1 and 2 ran earlier forms (said at the head of
  « Runs »; series 3 ran the merged form); « shows » restated as « consistent with » with the counts; the builds' logs name no commit
  (said in « Uncertainty »).

- **After the merge** (`f559ce6e`; the launcher rebuilt 11:29-11:42, every unit test passed, `I-0136/launcher-f559ce6e.log`; start
  check `start-f559ce6e9` 11:43-11:44, the ASUS, offline, restored by hash): passed (340 world frames).

## Uncertainty

- The tolerance's constant (8 ulps) rests on the read's emulation and one zone's measure, not on a bound proven for every
  programme; a row whose rounding exceeds it would lose pixels as before, one whose occluder sits within it leaks its motion.
- The mechanism (the absolute projection's rounding) stays INFERRED. The A/B is consistent with the change removing the holes: the
  former test's bad regime in 1 run of 2 (3 pictures of 20), none in the own-depth runs (0 of 6 runs, 58 pictures, three forms of
  the tolerance); the removal rests mainly on the mechanism (a tolerance of about 1 % of the distance against a rounding of 1e-4).
- The builds' logs name no commit: each run's build is named by the commit it was started from, by the times.
