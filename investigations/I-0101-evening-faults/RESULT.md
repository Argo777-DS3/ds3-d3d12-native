# I-0101 - Les 3 plantages du 27 au soir: result

Status: complete (2026-10-28): tranche 1 written on `claude/wizardly-nightingale-566361`, finished, reviewed and merged from
`claude/kind-dhawan-5ade23`; its second live run not run (D-0229).

## Re-reading of the study (the pilot, before the code)

The [STUDY](STUDY.md)'s readings were made by Sonnet subagents. The pilot re-read the ones a fix depends on, in the study's
outputs (`evidence/I-0100/evening-faults-study/`) and in the original's bytes:

- §3 (full screen): FUN_140EF1D00 (the ungated retry, `present/ds3_a.txt`), FUN_140EF5590's end and FUN_140EF6440 (the
  minimise and the focus state gated by the GX display [144961AB0], `method/d7.txt`, `present/ds3_a.txt`) and FUN_1417DC0C0:
  as the study says. Added by the pilot: [1449644B8] is the DirectInput manager (its +0x24C is read by the keyboard's set-up
  FUN_14183BEF0, which adds DISCL_NOWINKEY (0x10) when it is 0; I-0067 NATIVE-INPUT-LATCH), so the « focus state » the native
  module never lifted is the Windows key's lock in exclusive full screen. VERIFIED.
- §1 (pose): the native side (05_selected_scene, model_pose, model_pose_binding, 13_model_frame's `complete=false` skip) as
  the study says. VERIFIED.
- §2 (model method): FUN_140667430's decompile and disassembly (`method/d1.txt`, `d2.txt`: `CALL [RAX+0x38]` at 0x140667557,
  `CMP [RAX+0x60],8` at 0x14066755A, whole compares of 30, 30 and 16 wide characters), FUN_14002DD60's assignment of +0x48 (the
  MTD's string, its last path component, its extension cut; `d7.txt`), and the slot bytes read from DarkSoulsIII.exe 1.15.2
  (`bytesat.py`): slot 4 `33 C0 C3`, slot 5 `mov rax,[rcx+70h]`, slot 6 `[rcx+78h]`, slot 7 `48 8B 81 80 00 00 00 C3`, slot
  12 `C2 00 00`. VERIFIED. That the native `NativeMaterial::shader` (the MTD record's +0x70, written by D42D50) is the string
  FUN_1419349F0 reads stays INFERRED (both are the MTD's shader path; the native programme groups already key on its stem).

## Tranche 1: the three fixes and the rule

1. **Full screen** (STUDY §3.6). A failed Present other than a lost device completes its frame and marks the presentation lost
   with the discriminator (`SceneRenderer::AdmitPresent`; the receipt's `present_lost`); a lost device keeps the refusal. At the
   engine frame's start, `FollowNativePresentationState` (02_frame_lifecycle) follows a lost presentation or an exclusive full
   screen DXGI left by itself: the buffers resized between frames (ER FUN_141E9F100), `native_display_lost`; the window
   minimised once per loss when the target mode is exclusive full screen; the full-screen mode queued once when the window is
   in front again (ER's WM_ACTIVATEAPP re-entry, beside DS3's ungated retry); and FUN_140EF5590's gated focus branch with the
   native state (the Windows key's lock lifted again in exclusive full screen).
2. **Pose** (STUDY §1.5). The refusal `native_model_pose_nonfinite_current_rows` removed: the palette layer's InvalidInput
   skips the dependent draws; the admission skips a model whose placement or object rows (row 0 with model+0x1E0) are not
   finite (`native_object_skipped`, reasons `nonfinite_placement` and `nonfinite_object_rows`, outcome code 7 « nonfinite »), which also replaces the refusal
   `native_scene_selection_nonfinite_placement`; one row `native_model_pose_nonfinite` (the model, the row, its values, source
   or gate) the first time and every 600th. Unit test `model_pose_test`.
3. **Model method** (STUDY §2.4). Slot 7 (+0x38) of the native material instance's table returns a GX MTD image
   (`gx_mtd_image.h`) built with the material, its +0x48 string the stem of the MTD's shader; slots 4 and 12 take the
   original's own bodies. The burn's look stays undrawn natively (no reader of the parameter list): a ROADMAP row. Unit test
   `gx_mtd_image_test`.
4. **« Skip the object and go on »** (FABLE-REVIEW 4.2): written in WORKFLOW « Method rules »; the Scaleform families never
   witnessed (a blend mode, a primitive state, a filter state, a mesh kind) skip that draw and are counted
   (`native_ui_skipped`) instead of `std::terminate`. The triage of the module's other refusals: below.

### The triage of the module's refusals (« skip the object and go on »)

A Sonnet census of the 488 `Refuse(` sites of the module (game_bootstrap/*.inc, effect_system.cpp; read-only, its classes
checked by the pilot on each site converted): 366 guard the module's own contracts (hooks, pins, threads, scope, registries'
ownership, start-up, device, heap) and stay; 58 fire on one object's game data on a path with a natural skip; 64 are unclear
(catch-alls mixing data and allocation or GPU failures, the asset loaders whose handle the original consumes).

Converted in tranche 1 (`NoteNativeSkip`, a row `native_object_skipped` per reason, its first four times and every 1,000th):

- The model admission (09): a non-finite placement (both places), the fade state, the draw colour, an assembled part's
  capability bytes, and the five mismatches between the game's mesh inputs and the native storage (mesh or material
  membership, the physical stream's model or extent, the layout's material): that model skipped this collect, outcome named.
- The scene walk (09): an unreadable pointer, entity policy, placement, map-piece flags, item policy or placement, assembled
  character or part, and a draw entity whose bounds getter is not D07E10: that entity, item or part skipped.
- The pose (05): unreadable source or gate rows keep the previous version.
- The probe light's clock (17), the brightness input (17: the previous colour settings kept), the post transition row (03: the
  previous intent kept), the decal requests (03: rejected as 51BF0's early returns), the debug draw requests (03: not drawn).
- The Scaleform families (game_ui_session.cpp): above.

Left for a later tranche, each needing its consumers read first: the asset loaders (material, effect archive, shader, gparam,
texture families: a skipped asset must not become a refusal further on), the movie outputs (16), the catch-alls, the cloth
sets (17) and the runaway bounds of the walk (32,768 objects, 4,096 children).

### The step review (fresh Opus, 2026-10-28)

No high finding at the tranche's head (its first commit had a broken string literal in game_ui_session.cpp, fixed by the
next). Answered: M1, a failed Present gives no count back to the frame-latency waitable object, so the next frame skips its
wait once per count kept (`latency_debt_`); M2, a Present that keeps failing is resized at the first three follows and every
60th, its row at the first three and every 600th, and the minimise and the re-entry belong to the loss of the exclusive state
only; L1, the decal family row rate-limited; L2, the non-finite skips counted in `native_main_walk_outcomes` and named
`nonfinite` (code 7) in `popin_native`; L3, the pose test covers the composed cloth pose (a NaN in the source or the gate
rows); L4 and L8, comments; L5, the focus change kept for a later frame when the DirectInput manager does not exist yet; L6,
an unreadable optional pointer (parameter animations, FaceGen owner) skips the candidate instead of meaning « absent »; L7, the
shader tag predicate reads the material's stem instead of computing it again. Left: L9 (a ResizeBuffers failure while
following a loss still refuses: the device contract), L10 (the new statics are safe as written: a mutex, interlocked counters,
engine-thread state).

### Live run 1: exclusive full screen and focus losses (`i101-fs-alttab-1`, 2026-10-28 02:27-02:32, the ASUS)

`Run-I101FullScreenLoss.ps1`: the branch's module (10bb5ff0), the Firelink path's save, exclusive full screen 1920x1080 on the
ASUS (display-screen.txt), offline, restored by hash (`restoration ok`), `Invoke-I101FocusLoss.ps1`'s Alt-Tabs; Thomas also
pressed Alt-Tab himself during the run (so the losses mix his and the helper's). OBSERVED in `evidence/I-0066/artifacts/
i101-fs-alttab-1/startup.jsonl` and `evidence/I-0101/i101-fs-alttab-1-focus.json`:

- Six losses of the exclusive state, each `native_display_lost` (Present 0x887A0001, the chain windowed, the buffers resized,
  `minimised`), then `native_display_reentry` and `native_display_mode` (full screen again, `state:true`) when the window was in
  front: the evening fault no longer closes the game, and the game came back to exclusive full screen each time. Thomas: « le
  jeu continue de fonctionner ».
- At the end Thomas closed the game himself (« c'est moi qui ai fermé le jeu »): its last frames are the menu's and then
  empty clears, a Present 0x887A0001, and ResizeBuffers answering E_ACCESSDENIED (0x80070005) on the closing window; the follow
  refused (`native_output_resize_failed`), turning a normal quit into a refusal with a dump: the step review's L9, met at a
  quit. Fixed: the follow resizes only the presenter at its own size (`FramePresenter::TryResizeBuffers`), and a failed
  ResizeBuffers takes the chain's buffers again and goes on (ER FUN_141E9F100 goes on too), the next failed Present following
  again; a lost device still refuses. The row carries `resize_hresult`.

### The second live run: not run (D-0229)

The rerun of `Run-I101FullScreenLoss.ps1` on the branch tip (announced for 03:00, its helper given `-Close`: after the Alt-Tabs it
posts WM_SYSCOMMAND SC_CLOSE to the game's window, as the close box does) was cancelled by Thomas before it started (« oui on s'en
passe et pas de contrôle », 02:48, D-0229). So the close-box quit after `TryResizeBuffers` is NOT RUN: the fix is written, built
and reviewed, never run. The losses' recovery stays OBSERVED from run 1 only (module 10bb5ff0).

### The final review (fresh Opus xhigh, 2026-10-28) and its answers

No high finding. Answered on the branch (built, unit tests passed):

- M1: the five storage mismatches of the model admission (`geometry_mesh_membership`, `stream_model_mismatch`,
  `geometry_material_membership`, `mesh_layout_material_mismatch`, `stream_extent_mismatch`) are the registries' invariants (a
  stale entry after a missed retirement), so they are refusals again, as before the tranche; so is a slot-7 call on an
  instance the material registry does not own (`native_model_material_mtd_unowned`: only its instances carry that table).
- M2: an unwitnessed Scaleform blend mode no longer refuses at its push (`PushNativeBlend`): it reaches NativeBlendState, which
  counts it, and the colour draws under it are skipped. Left: the gradient filters (kinds 4 and 5) still refuse inside their
  uniform writer; skipping them needs the push decision read first.
- L1: a lost device returns from `TryResizeBuffers` before the buffers are taken again; that the old buffers survive a failed
  ResizeBuffers is ER's premise (FUN_141E9F100), not documented by DXGI (if `GetBuffer` failed there, the follow would refuse as
  before the fix). Refuted with Streamline's proxy (I-0111, `i111-fs-quit-1`, 2026-10-31: GetBuffer 887A0001 after the quit's
  E_ACCESSDENIED): stand-in back buffers now take their place. One `ResizeChain` serves both resizes (L8).
- L2: a frame that spends a kept latency count banks it again if its own Present fails too. L3: a chain still exclusive takes
  the follow's rate limit; only an exclusive state DXGI left resizes at every follow. L5: the inline ASM path names code 7. L6:
  an unreadable entity table returns at once. L8: the scope refusal renamed `native_ui_blend_state_scope`.
- Left, recorded: L4 (a mode change other than the re-entry's still refuses on a non-device failure; no toggle row in run 1);
  L7 (the per-object refusals `05_selected_scene.inc` group and policy unreadable, slot 6 of the material table, DS3's
  FUN_140EF1F80, not handled); L8's repeated GetFullscreenState (three cheap calls a frame). The new native code paths (the
  row-0 skip, `ModelMaterialMtd`) have no unit test of their own: they live in the game-bound translation unit; the tests cover
  the palette layer and the GX MTD image.
- The step review of these answers (fresh Opus, 2026-10-28): no high or medium. Answered: one `IsDeviceLoss` for the Present,
  the follow and `TryResizeBuffers` (L1); a null entity table counted (`entity_table_null`, L2); PushNativeBlend's comment says
  what 1BD9D90 does (it returns on a matching state, draws a mode above 0x13 as 0, maps the mode before 0x288, L4). Left (L3): a
  filter or cached quad under an unwitnessed blend scope still prepares its programme before the blend decode, so an unwitnessed
  programme variant there refuses; moving the decode first would skip the uniform writer's game calls (1F75180 on HAL+0x11A0),
  not read yet.

## Closing

I-0101 closes with tranche 1 (D-0227: merged once written and reviewed). Left for a later workstream, each with its consumers
read first: the asset loaders' refusals, the movie outputs, the catch-alls, the cloth sets and the walk's runaway bounds (the
triage above), the gradient filters (M2), L4 and L7. The close-box quit after `TryResizeBuffers` was never run: a refusal at a
quit in play goes back to this RESULT.

### The start check after the merge (`start-af037cde`, 2026-10-28 03:48-03:49, the ASUS)

`scripts/Invoke-StartCheck.ps1` on the launcher's module rebuilt from `main` af037cde (D-0229 as amended at 03:33): the world
drawn (348 frames), the game closed by its close box (SC_CLOSE) and gone by itself, no refusal or failure, `restoration ok`.
OBSERVED: one `native_exit_phase_exception` 0xC0000374 at the quit (the exit fault of I-0099, whose fix was not in `main`). The
run was windowed: the close-box quit from exclusive full screen after a lost presentation stays not run.
