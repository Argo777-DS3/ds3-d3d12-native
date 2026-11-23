# I-0107 - Lumières des surfaces forward: result

## Plan

| Step | What | State |
|---|---|---|
| 1 | Static: the organize dispatch (programme, thread groups, buffers and their sizes), where t59..t62 are bound, cbSceneParam +880..+900 and their sources | done (below) |
| 2 | The organize buffers and constants natively (point and spot records from the native items) | done (`39c78c96`) |
| 3 | The native dispatch of the genuine `GXLightOrganize` into the grid, the SRVs and words on the forward rows | done (`39c78c96`, review fixes) |
| 4 | Seen in a capture run, reviews, merge | seen (`i107-look1`); final and step reviews applied; merged |

## Step 1: the original's light grid (static, 2026-10-29)

Headless Ghidra on `ghidra-mirror-menus` (sub-agent; scripts in [ghidra/](ghidra/), outputs in `evidence/I-0107/organize/`);
the pilot checked the decompiles the design rests on (`1A0080`, `212810`).

1. **The dispatch is `1401A36B0`**, called by `1A0080`'s tail after it uploads cbLightAccParam; `1A0AE0` is the spot-light mask
   pre-pass (`GXLightAcc_SpotLightMask*`), not the dispatch (NATIVE-LIGHTING-MAP corrected). `10A220` records `1A0080` (even with
   no light), `1A0AE0`, then the tile groups' `1A1DA0`.
2. **Programme**: `GXLightOrganize` (`[cam8+0x4CC]` = 64 >= 32 and `[0x144962315]` = 0, no static writer; else `_All` or `16`).
   Thread groups `ceil(W / 64)` x `ceil(H / 64)` x 1, W, H the view's extent with its border (`view+0x7C/+0x80 + 2 view+0x78`).
   Bindings: b0 cbLightAccParam; t1 `g_DepthTexture` (`[[pass+0xFD0]+0x48]`, bound earlier by a recorded command; INFERRED the
   full-size depth); t19 points, t20 spots, t21 lines; u0 `g_wLightIndices`, u1 `g_lightLookup`, u2 `g_workData` (a fourth UAV
   +0x470 is bound too, its creator unreadable statically; the programme declares u0..u2 only). Before the dispatch only
   `g_workData` is zeroed (UpdateSubresource); after it the UAVs are unbound (D3D11's UAV-to-SRV hazard).
3. **Buffers** (`14019EF60`, grow-only with the render size; tiles16 = ceil(w / 16) ceil(h / 16)): `g_wLightIndices`
   tiles16 x 1024 x 8 bytes (stride 8), `g_lightLookup` tiles16 x 12 (stride 12), `g_workData` 4; point 1024 x 64, spot 1024 x 128,
   line 1024 x 80 (structured SRVs). `1A0080` uploads n records only when n != 0 and the « OrganizeLight » byte `cam8[0]` (1) holds.
4. **Records** (`1A0080`, light_producers' `OrganizeBuffer`): point 64 bytes {item [+0x20, +0x50), +0x90, +0x94, 0, +0x98}, where
   `212810` writes +0x90 = light+0x98, +0x94 = light+0x9C, +0x98 = light+0x15C (the radius); spot 128 bytes (`214710`) {item
   [+0x30, +0x60), +0xE4, +0xE8, 1, +0x1C9 ? 0 : 1, item [+0x60, +0xA0)}, the deferred spot rows with word 14 = 1.
5. **Constants** (cbLightAccParam, 320 bytes): the deferred assembly (floats 0..67) then 68 the point count, 69 0, 70 the spot
   count, 71 the line count, 72-73 the view's near and far, 74 0.8, 75 -2.1 (`[cam8+0x4B8/+0x4BC]`, constructor constants of
   `14019D5E0`), 76 64 (uint), 77 2 (64 >> 5).
6. **The forward rows**: `1A3520` records t59 `g_LightIndices`, t60 `g_LightLookupBuffer`, t61 points, t62 spots, t63 lines from
   the same objects; cbSceneParam (`1400764D0`, not `13AA60`, which copies it): +880 the near (`P32 / -P22` of the projection),
   +884 0.8, +888 -2.1, +896 64 and +900 `ceil(W / 64)` (integers, the bytes of `[cam8+0x4CC]` and the division).

## Design (step 2-3)

- Collect: the organize records from the native point and spot lights (the point rows with word 15 = the radius, the spot rows
  with word 14 = 1), at most 1,024 each; the organize constants from the native cbLightAccParam plus the words of item 5.
- Renderer (main view): persistent buffers sized as item 3 for the render size, the records copied into them each frame, the
  genuine `GXLightOrganize` dispatched after the G-buffer with the depth, the UAVs then read by the forward rows as t59..t62
  (a native structured-buffer record of each); the depth's transitions told to the frame graph, the grid's own buffers with
  raw barriers (as the grading stage's volume). The sizes follow the presentation size (grow-only).
- The forward rows' cbSceneParam +884..+900 from item 6 instead of zero.

## Steps 2-3: the light grid natively (code, `39c78c96`; on by default, `no-forward-lights.flag` opts out)

- Collect (`12_scene_collect.inc`): the organize records and cbLightAccParam of design above, for the main view only.
- Renderer: `LightGridSources` makes the grid's buffers off the render thread (grow-only, a serial per set), the job carries the
  set; `EncodeNativeLightOrganize` copies the records, zeroes the work counter, dispatches the genuine `GXLightOrganize` after
  the lighting with the depth plane (graph pass `light_organize`, key bit 38) and leaves the indices and lookup readable for the
  forward rows; the frame slot holds the six buffers; its root and pipeline are made before the plan key, a refusal latches
  (`organize_failure_`). The programme's declarations: `evidence/I-0107/organize/rdef-GXLightOrganize.txt` (b0, t1, t19..t21,
  u0..u2, no sampler).
- Bindings: `PublishNativeLightGrid` wraps the four buffers as structured-buffer scene textures (t59..t62), carried by the record
  epochs; the main view's forward rows bind them (the offscreen and reflection views keep the empty buffers); the forward rows'
  cbSceneParam words +884..+900 from the design (zero without the grid).
- Step review (fresh Opus): nothing blocking; applied: the failure latch, the slot holding the buffers, a serial instead of raw
  pointers, the flag and a missing programme neutral (no grid made), the 1,024 cap counted (`native_light_organize_overflow`).
  Accepted: for one frame after a grid growth the forward rows read the former grid (out-of-range reads give 0, no fault).

## Step 4: seen natively (`i107-look1`, 2026-10-29 21:51-21:55, ASUS, debug layer on, `build1` = `39c78c96`)

`Run-I106Look.ps1` (the I-0106 sweeps, `test-no-ai`): both runs exit 0, 0 debug-layer errors, restoration OK; the grid
published (tiles16 3,600 at 1280x720), the organize every frame (Anor Londo 488 points and 45 spots, Irithyll 576 and 46, 20 x 12
groups, near 0.08, far 10,000). FLIP (`flip.exe`, game window, against the original's same sweep; `evidence/I-0107/flip/means.txt`):
13 of 16 views closer to the original than I-0106's final tree (e.g. Anor Londo sweep 1: 0.066 to 0.045), three further
(Anor Londo 8 0.1164 to 0.1176, Irithyll 3 0.0889 to 0.0902, Irithyll 7 0.127 to 0.154: the camera's drift between runs not
separated). By eye (`hair-al-1.png`): the character's alpha-tested hair, a forward row, darker and cooler as in the original.
INFERRED, not measured: that the grid's contents (`g_lightLookup`, `g_wLightIndices`) equal the original's, and the spot
lights' share on the forward rows; the evidence is the pictures and the FLIP means. The debug layer was on, GPU-based
validation off (it would check the states of the t59..t62 views the rows read through their tables). Also seen (not this
workstream): at Central Irithyll the original casts the character's shadow from a local light, absent natively (I-0106 shadows
two lights there; which light casts it is not checked).

## Final review (fresh Opus, xhigh, on the final tree) and its outcome

- **I1, fixed**: a reflection borrowing a main-view record kept its t59..t62 grid bindings (22 reflection frames with forward
  rows in `i107-look1-m37`); an offscreen view's prepare now binds the empty light buffers on t59..t62 (its words were already
  zero), the shared record untouched.
- **I2/I3, fixed**: a refused grid allocation (the indices are tiles16 x 8 KiB: 265 MB at 3840x2160) or root/pipeline latches
  (`LightGridFailed`), is logged once (`native_light_grid_refused`), withdraws the published grid and stops the collect's
  inputs: the rows go back to the empty buffers and zero words; the session goes on.
- **M1/M5/M6, fixed** in this RESULT, the comments (`model_frame.h`, `light_producers.cpp`'s NumCellX now read, not inferred),
  WORKFLOW's FLIP row (the executable), the index, `status.json`. **M3, fixed**: `Close` resets the grid and the organize's
  objects; the destructor's failure path keeps them (the leak list). The frame start keeps the resolved programme set.
- **Open, recorded**: M2, the final tree (`build-final` of this branch, with these fixes) not yet run in game: the start check
  after the merge and Thomas's play are its first runs. M4, GXLightOrganize resolved within the lighting set (« the first
  complete set wins »): if it loaded after the five base programmes the grid would stay off for the session (`organize_bytes`
  0 in the row; 11,132 bytes in both runs). M7, about twelve single barriers per frame (not batched; cost not measured, D-0227).

- **Step review of these fixes** (`1976ddc7`): nothing high. M1, the first form of I1 copied the borrowed record's meshes on
  every reflection collect: replaced by the prepare-time binding above. L1/L2, the grid withdrawn each frame the latch holds,
  from the frame that set it, and reported once per latch; L3, a latched grid's buffers released by the renderer. L4 (the
  frame start's two statics, engine thread only) accepted.

## Conclusion

The forward rows of the main view read the genuine GXLightOrganize grid natively (points and spots, at most 1,024 each), as
the original binds it; seen in `i107-look1`, the grid's contents inferred from the pictures and FLIP, not compared word for
word. Merged into main `aef29811` (2026-10-29); start check `start-aef29811` passed at 23:08 (to the world, closed, restored; the
organize every frame, 38 points and 6 spots, `organize_bytes` 11,132, no refusal). Open: M2 (the final tree in play), M4, M7
above; the line lights (t63) excluded (CONTRACT).
