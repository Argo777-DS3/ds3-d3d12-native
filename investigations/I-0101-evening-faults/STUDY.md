# Study: the three refusals of Thomas's 2026-10-27 evening play

Study thread (read-only, 2026-10-27 evening, Paris), branch `claude/eager-mcnulty-166985`, for the pilot of the next workstream
after I-0100 (D-0224, n°2 of the order). The observations are in I-0100 [RESULT](RESULT.md) « Thomas's evening play ». The pilot
copies this study into the new investigation in its first commit (WORKFLOW « Canonical sources »).

Statuses: OBSERVED (a journal, a crash report), VERIFIED (read in a decompile with the script named; Sonnet `ghidra-research`
readings, the parts a decision depends on to be re-read by the pilot or a fresh Opus), INFERRED (a reading, or public DXGI
knowledge cited as such). The scripts and their outputs of this study are in `evidence/I-0100/evening-faults-study/` (`present/`, `pose/`, `method/`), listed per section;
the decisive readings are to be re-read before the fix is written.

Each fix ends with a native unit test and a short announced witness; the three can share one workstream and one gate.

Recommended order: §3 (the full screen: certain to recur, any Alt-Tab), then §1 (the pose: a refusal where the original goes on),
then §2 (the model method: a missing slot to write).

## 1. The non-finite pose at Iudex Gundyr's death

### 1.1 What happened (OBSERVED)

The test launcher, 2026-10-27 20:42, `%LOCALAPPDATA%/DS3-DX12-Test/test-20261027-203531.jsonl` and its crash report: Gundyr's
phase 2, killed with a fireball; `native_model_pose_nonfinite_current_rows` from `UpdateNativeModelPose`
(05_selected_scene.inc ~77) under `UpdateNativeModelPosePublic` (~121), our replacement of DS3's public pose update FUN_140016000;
game stack FUN_140D07710+0x1FA ← FUN_1407728A0+0x117 ← FUN_140890950+0x488 ← FUN_1423C1DC0. The journal does not name the model
nor the first non-finite row (the refusal logs neither).

### 1.2 What the original does (VERIFIED, DS3 1.15.2)

- The chain: FUN_140890950 is ChrIns slot 36 (the character's frame update); at 0x140890DD6 it calls its `NS_SPRJ::ChrModel`
  [chr+0x48] slot 8, FUN_1407728A0, which calls SprjModel::Update FUN_140C4AF10; that ends with a tail jump to the draw entity's
  slot 0xC8, FUN_140D07710 (`SprjModelDrawEntity`, vtable 0x142948EF8; hence no C4AF10 frame on the stack). FUN_140D07710 calls
  FUN_140016000(E+0xA80, rows `[E+0xB18+0x18*((E+0xC3C>>3)&1)]`, Q rows `[E+0xB78]`, 0) at 0x140D07906 on its cloth branch
  (`[14477FEC8]+0x170 != 0 && E+0xA70 != 0 && FUN_140DD5D10() == 0`), else FUN_140015FC0 without Q. The failing model is
  therefore a cloth-bearing one (E+0xA70 set), with reference rows.
- The rows are world-space bone matrices: FUN_140D06A00 composes the world transform [M+0x20] with the animated local pose
  [M+0x50] (FUN_140D33EE0) and FUN_140D076A0 copies them into the entity. NaN can come from the local pose (animation, ragdoll,
  blend; its writer not traced, INFERRED) or from the Q inverse: FUN_1400179C0 inverts a 4x4 with `divps 1/det` and no
  singularity test, so the engine itself makes Inf/NaN rows from a singular Q.
- FUN_140016000 has **no finiteness test anywhere** (float loads, products, 16-byte copies; no compare on row data, no clamp). All
  three paths (the in-place fast path at receiver +0x74, the queued drain, the 16050 body with its ring advance) copy the rows as
  they are; the upload FUN_141920300 is `ID3D11DeviceContext::UpdateSubresource` of the bytes. The claim of RESULT holds: **the
  original sends non-finite rows to the GPU and goes on.**
- The only NaN-sensitive original step found: FUN_140D08190 (run first in FUN_140D07710 when E+0xBA8 bit 2 is set, every pose
  write) builds the bounds E+0xB80..0xB9C from the same rows and keeps them only when `min <= max` on three lanes; with NaN the
  test fails and the box becomes a zero box (that the NaN reaches it through FUN_140D2DB00 is INFERRED: not decompiled).
- No dead or hidden filter: neither FUN_140D07710 nor its callers test a death or draw flag before the pose update (the draw
  enable, E+0xC3C bit 0, is read on the draw side, FUN_140D079C0). A dying character's pose is updated and uploaded every frame
  while dt != 0.

### 1.3 What our module does with such rows (VERIFIED in `native/`)

- Before the refusal, the palette layer already handles them as designed (content/model_pose.h ~37-43): `ProduceModelPose`
  (content/model_pose.cpp ~55-125) marks the dependent palettes InvalidInput and publishes the version; `ProduceClothCancel`
  (world/producers.cpp ~304) is plain float arithmetic (NaN in, NaN out, no exception).
- Downstream: `SelectOrdinaryModelPoseBinding` (content/model_pose_binding.h ~18-32) answers Unavailable for a palette not
  Ready, and 13_model_frame.inc ~1380-1421 skips that mesh's draw for the frame (`native_selected_palette_unavailable`,
  `unsupported_palettes`), no refusal; the upload (~1423-1440) runs only for a Ready palette, so non-finite rows never reach the
  GPU natively.
- One unchecked consumer: 05_selected_scene.inc ~832, `input.object_rows = selected.pose->matrices.front()` (row 0, when
  model+0x1E0 is set) feeds the instance record (world/producers.cpp): a NaN there would reach the model's instance constants
  (INFERRED: wrong pixels for that model, not a fault).
- So the refusal at ~77 is the only place that turns the rows into a process exit, and it fires after the version is published.

### 1.4 Cause

The game produced non-finite bone rows for one cloth-bearing model during Gundyr's death (OBSERVED: the refusal; which model,
which row, and whether the NaN is in the source rows A, the gate rows G or their product is not logged). The original tolerates
them; our module refuses where the original goes on. The NaN's producer (the local pose writer of M+0x50, or the singular Q
inverse) is INFERRED and does not need to be known for the fix.

### 1.5 Minimal fix, faithful to the original

1. **Remove the refusal** (05_selected_scene.inc ~76-77). The CPU ring copy (`owner.matrices`, the receiver's pointer) keeps
   the rows as they are, as the original's 16000 does.
2. **Keep the palette layer's rule** (no non-finite row uploaded): that model's draws needing the palette are skipped while its
   rows are not finite. The original draws them with NaN bones, whose triangles the rasterizer drops; the difference is the
   finite part of a mesh weighted to finite bones, for the frames of the fault only (a transient at death). This is a skipped
   draw, not a stand-in (D-0197). Say so in the code's comment and the RESULT.
3. **Row 0 of the instance record**: treat a non-finite row 0 like an unavailable palette (that model not drawn this frame)
   instead of feeding NaN to its instance constants.
4. **One diagnostic row**, rate-limited (the first per model, then one per 600 frames): `native_model_pose_nonfinite` with the
   model's name, the row index, the 12 values, and whether A or G holds it (the gate rows are already read). It names the
   producer at the next occurrence without a run designed for it.
5. Test: a unit test of the pose update with a NaN row (no refusal, the palette InvalidInput, the draw skipped and counted, a
   finite version afterwards draws again); live, Thomas's play or a witness save before Gundyr (the boss is reached in a few
   minutes from Firelink's start), killed in phase 2, the row present and no refusal.

Scripts (`evidence/I-0100/evening-faults-study/pose/`): `run.ps1 -Script <java> -Out <file> <args...>` (refuses when DarkSoulsIII runs);
`I0099Decompile.java` outputs `a1.txt` (140016000), `a2.txt` (140D07910, 1407729B7, 140D2D040, 141920300, 1400179C0,
140017CE0, 140890DD8), `a3.txt` (140C4AF10, 140D08080, 140DD5D10, 140D08430, 140D08190, 140D3BF40, 140D3FF50, 140D400F0,
140890950, 140772FE0); `PoseDis.java` (`b1.txt`); `I0066FindInstructions.java` (`c1.txt`, a field scan, noise). The repo's
decompiled/DS3-1152-F-0001{5FC0,6000,6050}.c, 00D06A00.c, 00D076A0.c, 00D08190.c, I-0067 MENUS-MAP §62 and VELOCITY-SKINNING.
Not reached: FUN_140D33EE0, FUN_140D2DB00, FUN_141FD0F30, the writer of M+0x50, a CPU reader of the receiver's ring (none found
by a field scan; the hypothesis is that none exists).

## 2. The unowned model method at Archdragon Peak

### 2.1 What happened (OBSERVED)

`main`'s launcher module, 2026-10-27 20:13, `%LOCALAPPDATA%/DS3-DX12/play-20261027-201033.jsonl`: `native_model_method_unowned`
caller RVA 6714714 (0x66755A), then `native_model_storage_method_not_owned`. Just before, a batch of first-sight `popin_lod`
records (o320400, o000402, o000400, m32_00_00_00_005524, c0000). Stack: 0x66755A ← 0x66B5D2 ← 0x6670A3 (FUN_140666C60) ←
0x66D71B ← 0x67B93E ← 0x674628 ← 0x6831A1 ← 0x6082FA (FUN_140607570) ← FUN_1408F2CA0.

### 2.2 What the original does (VERIFIED statically, DS3 1.15.2)

- The function holding 0x66755A is FUN_140667430 (the stack's « FUN_1406674FE » is a label inside it): `CALL [RAX+0x10]` at
  0x140667540, `CALL [RAX+0x38]` at 0x140667557. Callers: FUN_14066B430 (at 0x14066B5CD) ← FUN_140666C60 (0x14066709E) ←
  FUN_14066D660 ← FUN_14067B830 ← FUN_1406745C0 ← FUN_1406830F0 ← FUN_140607570.
- The object is a map object of the « Obj » family (its class name not established): its LOD slot `+0x18E & 7`, burn flags
  `+0x191`, burn slots `+0x1A8`, model holder `[[+0x250]+0x80+lod*8]` (holder +0x60 the model), and `[+0x290]+0xF8` a list of
  material parameters.
- `model+0x1F0` is the model's `GXBS::GXModelMaterialSet` (vtable 0x143D1D4F0, I-0067 RTTI `vslots.tsv`), filled by
  FUN_1400A0170 → FUN_140029FB0 with each ordinal's instance from the resolver's +0x28, which is our `BuildModelMaterial`. Its
  +0x10 (0x14002A110, `return begin[index]`) returns the material instance. **The refused object is our material instance:
  `ModelMaterialTable()` (18_resource_admission.inc ~387), whose slot 7 (+0x38) is `UnsupportedModelStorage`.** The 2-slot layout
  table is not involved (the layout prefix is never in this container).
- The original class is `GXFLV::GXFlverMaterial` (vtable 0x143D52878, 13 slots, constructor FUN_140140470, allocated by
  FUN_140082CC0, which `BuildModelMaterial` already calls). Its slot 7, 0x140140410, is `mov rax,[rcx+0x80]; ret`: a getter of
  the material's refcounted GX MTD object (stored by FUN_140140F90, I-0067 MENUS-MAP §66-67). No D3D11 use.
- FUN_140667430, for every material of the model: its name (+0x30), `mtd = vt38(material)`, the wstring at `mtd+0x48` (MSVC
  layout: inline buffer, size +0x58, capacity +0x60) **without a null test** (0x14066755A is `CMP [RAX+0x60],8`), compared whole
  against `GXFlver_ColDifSpcBumpIbl_Burn`, `GXFlver_ColDifSpcBumpRNM_Burn` and `BurnCloth_cloth`. On a match it upserts or removes
  an entry of the object's parameter list for that material (id 7 for the two Burn MTDs, `BlendRate` for the cloth) with the burn
  progress `f` (FUN_1407C8970: the burn slot's elapsed/total, 1 when finished, 0 before).
- When it runs: FUN_14066B430, once a frame per object, calls it only while the object burns (`+0x191 & 1` set, `& 2` clear, the
  timer FUN_1407C89A0 running), and once at the stop flag `+0x364` to restore. Burns start by the request `+0x362` (allowed when
  FUN_1406605D0(owner): bit 6 of the byte at `[owner+0x18]+10`, probably the object param's « burnable », INFERRED), by a fire
  hit (FUN_140663A80, event-queue bit 4, processed by FUN_140657A10 under FUN_140666C60), or at the object's (re)initialisation
  for a pre-burned object (FUN_140667F50 from FUN_140669D90). Every material of a burning object passes through the call, Burn
  MTD or not.

### 2.3 Cause

A burnable map object at Archdragon Peak started burning (INFERRED: a fire hit, Thomas plays a pyromancer; or a pre-burned object
initialised on first sight, the `popin_lod` batch just before), and DS3's burn update asked our material instance for its GX MTD
object, a slot our table refuses. Natively `+0x80` is zero (FUN_140082CC0 zeroes it, FUN_140140F90 does not run), so the
original's getter itself would return null and DS3 would fault at 0x14066755A. The object that burned is not identified.

### 2.4 Minimal fix

1. **Slot 7 of `ModelMaterialTable()`**: a native `ModelMaterialMtd(receiver)` returning a native-owned block of at least 0x68
   bytes, zeroed, with an MSVC `std::wstring` image at +0x48 (inline when under 8 characters, else a pointer; size +0x58,
   capacity +0x60) holding the material's MTD name; the same pointer every call, kept in `NativeModelMaterial` (found by receiver
   in `model_materials` under `model_material_lock`) and living as long as the instance. **Never stored at the instance's +0x80**:
   FUN_140140560 releases `[+0x80]` through a refcount at +8 and a virtual call (decompiled/DS3-1152-F-00140560.c).
2. **The name** (VERIFIED, `method/d7.txt`): gxMtd +0x48 is the stem of the **shader name read inside the MTD file**, not the
   MTD file's own name: FUN_14002DD60 reads a string from the MTD data (FUN_1419349F0), takes its last path component (D1760),
   cuts the extension (D16E0) and assigns it to +0x48, case as authored (the compare is case-sensitive). FUN_140D43BB0 writes
   the MTD's own name at +0x18, another field. So the block's string is the stem of `authored->shader` (D1760 then D16E0, as
   `NativeModelShaderTagPredicate` already does, 18_resource_admission.inc ~617), not `BuildModelMaterial`'s `key`. That
   `NativeMaterial::shader` is the same MTD field FUN_1419349F0 reads is INFERRED (to check in the MTD reader).
3. **The other slots** of 0x143D52878 met by the same family are likely +0x30 (`[+0x78]`) and +0x20 (returns 0, HYPOTHESIS);
   slot 4 (return 0) and slot 12 (empty) can take faithful bodies at once; the others stay refusals until a path meets them
   (the 13 slots, 5-7 VERIFIED from bytes, the others from their first bytes: 0 `140530` deleting destructor; 1 `1407D0`
   predicate on +0x70, +0x1C, +0x24; 2 `140680` predicate (K-0210's selected entry 140063F01); 3 `140E90` calls +0x38 then the
   resolver's +0x50; 4 `140320` returns 0; 5 `1403F0` returns [+0x70] (owned natively); 6 `140400` returns [+0x78]; 7 `140410`
   returns [+0x80]; 8 `1421C0` an archive call; 9 `1420E0` reads +0xF1 and [+0x440]; 10 `140420` (owned); 11 `141070` the GX item
   parser; 12 `140460` empty). Slot 3 also calls +0x38, so the same block serves it.
4. **What stays missing**: the burn's look. The parameter-list entries (id 7, `BlendRate`) have no native reader (`native/` has no
   ParamModifier, BlendRate or Burn consumer), so a burning object will no longer refuse but will not show its burn natively. A
   ROADMAP row, not this fix (stability first, WORKFLOW).
5. **A diagnostic** at the first call per material: its name and MTD stem, so the next occurrence names the object.
6. Test: a unit test of the block's layout (read back as DS3 reads it); live, a fire hit on a burnable object (a firebomb or a
   fireball on wooden objects; which objects carry `_Burn` MTDs is a data question: the native `materials` registry or the MTD
   binder), on Eta's Archdragon Peak save (`shebang-zones-eta-m32-archdragon`) if the object is found there.

Scripts (`evidence/I-0100/evening-faults-study/method/`): `run.ps1` (hidden, idle, refuses when DarkSoulsIII runs); `scripts/I0099Decompile.java`
and `scripts/MAll.java` (decompile, `a:` disassemble, `d:` qwords, `r:` references, `s:`/`x:` displacement uses), outputs
`d1.txt`…`d6.txt`; `investigations/I-0100-popin/bytesat.py` for the slot bytes; `scan362.py` (a raw scan for `+0x362` stores, none
found). Not reached: the gxMtd name writer (above), the setter of `+0x362` and the object's class name (FUN_140660F90,
FUN_140665970, FUN_14066ADD0, FUN_140660070, the callers of FUN_1406605D0), the models using the Burn MTDs.

## 3. Present 887A0001 after the exclusive full screen is lost

### 3.1 What happened (OBSERVED)

`main`'s launcher module, OLED, exclusive full screen 2560x1440 (FindClosestMatchingMode at 60 Hz: the journal's `refresh_hz` 59),
title screen (`05_000_Title` movies advancing), journal `%LOCALAPPDATA%/DS3-DX12/play-20261027-200537.jsonl`:

- the start-up's full screen: line 11460 `native_display_toggle switched:true` (CSWindowImp's retry FUN_140EF1D00 through our
  hook of FUN_14000B590), 14885 `native_display_request` 2560x1440 mode 1, 16465 `native_display_mode state:true`;
- Print Screen: the capture tool took the foreground; serial 861 `native_slow_frame interval_ms 722.2` (INFERRED: the display
  mode restored to the desktop's);
- serial 862: `native_render_job_failure "native startup present: HRESULT 887A0001"`, then `native_render_worker_faulted`.
  No `native_display_*` row between 861 and 862 (no engine-side mode change).
- The failing call is `IDXGISwapChain::Present(1, 0)` on the render worker (`PresentPaced`, scene_pass.cpp ~858, through
  `Check(..., "native startup present")` ~2892): pacing on, interval 1, no tearing row, so flags 0. The tearing hypothesis is
  refuted for this run.

### 3.2 What the original does (VERIFIED, DS3 1.15.2)

- Its swap chain is the legacy bit-block model: FUN_14192C220 fills `Windowed=1, SwapEffect=0` (DXGI_SWAP_EFFECT_DISCARD) and
  `Flags=ALLOW_MODE_SWITCH` (FUN_1419299C0), then `MakeWindowAssociation(hwnd, 3)`. Correction to
  I-0068 DISPLAY-SWAPCHAIN-NATIVE §3.2, which reads +0x44 as the swap effect: +0x44 is the flags.
- Present: GX end of frame FUN_1400063D0 → FUN_1400D5CB0 → FUN_142139260 / FUN_14192CF00:
  `r = Present(interval, 0); if (r != 0 && r != 0x087A0001) FUN_14192A4D0(r);`. Exact codes, no `FAILED()`; FUN_14192A4D0 maps
  the code to an engine enum (INVALID_CALL → 5, DEVICE_REMOVED → 0xD, DEVICE_RESET → 0xE) and both callers are `void`: **the
  original ignores every Present failure**; it neither stops nor skips nor recovers on it.
- Lost full screen: FUN_14000B540 (GetFullscreenState of the first chain) is polled every engine frame by CSWindowImp
  (step FUN_140EF5590, retry FUN_140EF1D00, WM_SIZE in FUN_140EF4D10). When the target mode is full screen and the chain is not:
  the frame step minimises the window (`ShowWindow(hwnd, SW_MINIMIZE)`); WM_SIZE (restored) toggles back through FUN_14000B590
  (`SetFullscreenState(!current)`, `ShowWindow`, then FUN_14000B700 = ResizeBuffers at the current size); the retry
  FUN_140EF1D00 (`FUN_14000B540() != (mode == 1)` → FUN_14000B590; on failure `SetActiveWindow` and a 10-frame cooldown).
- The window procedure FUN_140EF4D10 has no WM_ACTIVATE, WM_ACTIVATEAPP or WM_KILLFOCUS case (the class procedure
  `thunk_FUN_144F0691B` is obfuscated: what it does before is not reached). Focus is read by `GetForegroundWindow() == hwnd`
  (FUN_1423575B0): without it the game pauses (+0x63).

### 3.3 What Elden Ring does (VERIFIED, ER 2.7.1, the D3D12 architecture we copy, D-0195)

- Present FUN_141E9F630: `Present(interval, 0)`; on 0 or 0x087A0001 it signals the frame's fence and refreshes the back-buffer
  index; otherwise it maps the code (same table) and returns, the result dropped by FUN_141AFCB60.
- Lost full screen, at the device frame start FUN_1419E77F0 → FUN_1419ED670, per chain: GetFullscreenState FALSE while the
  entry says full screen → wait for the GPU, release the targets, **ResizeBuffers at the chain's own size** (FUN_141E9F100,
  GetDesc on failure), mark the entry windowed, rebuild the targets, notify the listeners.
- Re-entry: WM_ACTIVATEAPP (FUN_140E91A70, wParam != 0, the mode full screen, not exclusive now) queues a mode request for the
  next frame; deactivation does nothing. The tick FUN_140E92690 minimises the window while the chain is not exclusive and the
  mode is full screen, as DS3.
- ER's defects (named, not copied): a Present failure is ignored; on a failed Present the frame's fence is never signalled
  (INFERRED: a later wait on it could stall; FUN_141AFC9A0 not read in full); re-entry rests on WM_ACTIVATEAPP alone.

### 3.4 What our module does (VERIFIED in `native/`)

- Our chain is FLIP_DISCARD (D3D12 requires the flip model), ALLOW_MODE_SWITCH, the latency waitable object, 3 buffers;
  `MakeWindowAssociation(NO_WINDOW_CHANGES | NO_ALT_ENTER)` (backend.cpp `FramePresenter::Open`).
- `Check` throws on any failing HRESULT, so the worker faults, `BeginFrame` (15_cpu_service_and_input.inc ~333) rethrows and
  refuses. OCCLUDED (a success code) passes.
- Nothing notices that DXGI left full screen by itself: ResizeBuffers is called only by `FramePresenter::Resize`, reached from an
  engine size change, `TogglePresentationFullscreen` and `ApplyDisplayMode`. GetFullscreenState is read only for the tearing
  flag, for the engine's FUN_14000B540 (`NativeDisplayFullscreen`), in the toggle, the mode change and Close.
- The GX display [144961AB0] is null natively, so every CSWindowImp branch gated by it never runs: the minimise on a lost full
  screen, WM_SIZE's re-entry toggle, FUN_140EF6440, FUN_140D10DE0, and (to verify) the unpause on focus return. Only the ungated
  retry FUN_140EF1D00 reaches our hooks (22_setup.inc ~599: 0xB540, 0xB590): that is the path that will put full screen back.

### 3.5 Cause

INFERRED, the most consistent reading (H1): the capture took the foreground, DXGI took the chain out of exclusive full screen
(the 722 ms frame), and the next Present on our flip chain, whose buffers were not resized after that transition, returned
DXGI_ERROR_INVALID_CALL. Both originals answer the same transition by polling GetFullscreenState at the frame start, and ER (flip
model, like ours) resizes the buffers there, before presenting again. Public D3D12/DXGI guidance says a flip-model chain needs
ResizeBuffers after a full-screen transition (cited from knowledge, not from a document read here). Not separable statically:
H2, the engine thread's retry toggle racing the worker's Present (against it: no toggle row between 861 and 862); H3, a Present
on a window no longer in front. The discriminator in §3.6 step 1 separates them on the first recurrence.

### 3.6 Minimal fix, faithful to the original and to ER

1. **A failed Present is not fatal unless the device is lost.** In `PresentPaced`'s callers (the startup present ~2892, the
   scene present, and the others that `Check` a Present): DXGI_ERROR_DEVICE_REMOVED / DEVICE_RESET keep today's path (DRED rows,
   refusal); any other failure is recorded (once per episode, a row `native_present_failed` with the HRESULT and the
   discriminator: GetFullscreenState, GetDesc's Windowed and size, IsIconic, `GetForegroundWindow() == window`, the client
   rectangle, GetFrameStatistics' result), the frame completes normally (**its fence signalled and its slot released**, the
   defect ER has), and a `presentation_lost` flag is set for the engine thread. As the original: the error never stops the game.
2. **ER's frame-start follow (FUN_1419ED670), natively.** A `FollowNativePresentationState()` in `BeginFrame` before
   `ApplyNativeDisplayRequest`: if the presentation was set exclusive (the state our toggle and `ApplyDisplayMode` record) and
   GetFullscreenState now says windowed, or the flag of step 1 is set, then under `WithPresentationIdle`: `Resize` at the
   current size (ResizeBuffers), the recorded state windowed, a row `native_display_lost`. Polled once a frame, the cost of one
   GetFullscreenState.
3. **The minimise both games do.** In the same follow, when the engine's target mode is full screen (CSWindowImp
   [14489F118] +0x78 == 1, not borderless) and the chain is not exclusive: `ShowWindow(hwnd, SW_MINIMIZE)` once per loss, the
   role of DS3's GX-gated branch in FUN_140EF5590 and ER's FUN_140E92690. The re-entry needs no new code: DS3's ungated retry
   FUN_140EF1D00 toggles back through our hook when the window is in front again (its cooldown and SetActiveWindow on failure).
4. **The focus « pause » is never lifted natively** (VERIFIED, `method/d7.txt`, `d9.txt`, the end of FUN_140EF5590,
   0x140EF58EB-0x140EF5A5E): focus lost → `if (!win[+0x63]) { FUN_1417DC0C0(mgr, 1); win[+0x63] = 1; }`, ungated; focus held →
   `else if ((short)win[+0x64] > 1 && [144961AB0] != 0) { f = FUN_14000B540([144961AB0]); if (win[+0x62] != f || (win[+0x63] &&
   f)) { FUN_1417DC0C0(mgr, f == 0); win[+0x63] = (f == 0); } win[+0x62] = f; }`. With the GX display null, nothing calls
   FUN_1417DC0C0(mgr, 0) after a focus loss. FUN_1417DC0C0 sets `[mgr+0x24C]` and calls each child's +0x140 (`mgr` = [1449644B8]);
   what it does is not read. It is not a plain game pause: the original passes `f == 0`, so a focused **windowed** game gets 1
   too (INFERRED: a « not focused or not exclusive » state, the mouse capture or the sound; DS3 keeps playing online when
   unfocused). Its other callers only set 1 (FUN_140EF4400, the window's destructor; FUN_140EF4580, the minimise timer). So the
   native difference is narrow: after a focus loss, a game back in exclusive full screen keeps the state 1 where the original
   sets 0 (+0x62's start value not found). What Thomas sees after an Alt-Tab today is to ask him. Fix, the role of the gated branch:
   in the same frame-start follow, when the window has the foreground again and +0x63 is set, `FUN_1417DC0C0(mgr, f == 0)` and
   +0x63 with `f` our `PresentationFullscreen()`, +0x62 = f, exactly the original's condition with our state for FUN_14000B540.
   FUN_140EF1F80 has the same gate before FUN_14000B540 (it can reset win+0xBC): to read when the fix is written.

Test: an announced run on the ASUS in exclusive full screen, three focus losses (a helper that brings another window to the
front, then back; Print Screen if the capture tool is installed), each followed by a return: no refusal, `native_display_lost`
then `native_display_toggle switched:true`, the game unpaused, the frames over 25 ms back to normal. The borderless window
(`-Borderless`) is unaffected (no exclusive state) and is the control.

Scripts (`evidence/I-0100/evening-faults-study/present/`): `Run-Ghidra.ps1` (read-only, hidden, idle), `I0099Decompile.java` outputs
`ds3_a.txt`…`ds3_d.txt`, `er_a.txt`, `er_b.txt`; `I0066FindInstructions.java` (`CALL.qword.ptr.\[[A-Z0-9]+.\+.0x40\]`,
`ds3_call40.txt`); `PresCallers.java` (`ds3_presentcallers.txt`). Not reached: FUN_140EF1F80, FUN_1417DC0C0, ER FUN_1419EF6E0's
body (taken from I-0076 ER-STUDY), ER FUN_141AFC9A0, the obfuscated window procedure, the exhaustiveness of DS3's Present sites.
