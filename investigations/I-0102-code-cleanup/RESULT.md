# I-0102 - Nettoyage du code: result

Status: complete (2026-10-28). Tranche 1 merged (`92d77dd7`), tranche 2 in lots: 2a (`6a3b127a`), 2b, 2c and 2e
(`e3cd8b56`), 2f, the splits and 2g (`ce379c84`, branch `claude/modest-hermann-6e0c3e`); each merge's start check passed;
the whole cleanup reviewed by two fresh Opus at xhigh; the full control after the last merge passed (below).

## The census (read-only, 2026-10-28)

A Sonnet subagent's census, `evidence/I-0102/CENSUS.md` (outside Git): clang `-Wunused-*` and an AST reachability pass from
`Setup`/`DllMain` find no unreferenced function or variable in the game module's unit (`game_bootstrap.cpp` and its 22 `.inc`,
21,913 lines); what remains there is flag-gated diagnostics, A/B scaffolding of merged optimisations, repeated path boilerplate
and stale comments. Outside it: 39 files no build script builds or includes, a legacy `SceneRenderer` path, the capture-era
tools of `native/world` (11,436 lines, built by their own `Build.ps1`), dead control modes of the runner. The pilot checked each
candidate it removed against the code (grep of every name, the build).

## Tranche 1 (no behaviour change)

- The 39 orphan files of the I-0038 to I-0040 owned-startup stack (`frame/` startup host, runtime, journal, engine step, native
  service, scene work and owner, window host, the engine dispatch fixture; `content/cpu_ingress`, `material_programs.cpp`;
  `renderer/primitive_reference.cpp`; `world/capture_*regression.cpp`) and `frame/scene_work.h`: about 4,500 lines. No build
  script built them and no built file included them; their only references were each other.
- The legacy `SceneRenderer` scene-assets path, reached only from those files: `Open(…SceneAssets)`, `AdmitScene`,
  `AdmitSceneWork`, `RenderSceneWork`, `WithdrawSceneWork`, `Render`, `RenderFrame`, then the helpers only they called
  (`Content`, `SharedContent`, `MaterialContent`, `SceneComposition`, `FinalDisplay`, `Environment`, `Pipelines`, `Textures`,
  `Constants`, `ValidSceneCompositionFrame`): 674 lines of `scene_pass.cpp`. The module opens the renderer with `Start` only.
  `MaterialColorCompositionPipeline` and `OffscreenCompositionPipeline` stay (the live path calls them).
- Never-defined `I0038_REFERENCE_CONTROLS` blocks; dead helpers (`ConstantUpload`, `AppendBytes`, `ReadModelWetDrawInputs`,
  `ReadModelPreparationFlags`, `SetNativeOwner`, `ffx_observe::Enabled`, `FinalFormat`, `Verifying`, `LastResizeFence`, the
  `GlareFamily*` accessors) and write-only state (`decals_drawn_`, `paints_drawn_`, `water_drawn_`, `glare_family_failure_`,
  `output_adapter`'s `retired`).
- The witness pictures every test run wrote (`velocity-*`, `reflection-*`, `offscreen-*`, `movie-*`: 18 MB a start check, read
  by no script) only with `world-pictures.flag`, like the `world-*` pictures, through one `WitnessPictures()` and one path
  helper instead of six copies (I-0099 final review L10); those four still never in a measure run, the `world-*` pictures
  with the flag in any run as before. The velocity readback still samples in test runs (only its file goes); the rows
  `native_offscreen_pixels` and `native_movie_frame` with `"pixels":true` follow the files (no script reads them).
- Stale comments: the module's header, the `yebis-<pass>.flag` « opt-in » notes (the passes are on by default), the
  `frame-graph.flag` notes (the executor is the default, D-0171); `native/README.md`'s `frame/` row.
- Proof: the build and every unit test pass (receipt `evidence/I-0102/build-tranche1.log`); the files only other scripts
  compile (`execution_scope.cpp`, `output_adapter.cpp`, `material_admission.cpp`) checked with clangd, no diagnostic;
  `Test-ThreadAffinity` passes with `WitnessPicturePath` in its scope (its journal read reported under S-1); `Test-Project`
  passes.

### The final review (fresh Opus xhigh, 2026-10-28)

No high. Answered: M1, the `world-*` pictures had become measure-gated too (a change of the launchers' `-DiagnosticFlags
world-pictures`): kept on the flag alone; L2, the path helper in the thread-affinity scope; L4, an include nothing used
dropped; L5, `PrimitiveDepthTarget`'s comment restored; L7 and L8, STATE, the status, the README rows, the runner's
`-FrameGraph` comment. Carried into tranche 2 (L3): `RetireSceneResources`, the scene state that never leaves its defaults
(`material_draws_`, `work_mode_`, `scene_generation_`, `texture_generation_`, `scene_ready_`) and the resolve, mask and
environment branches it guards, `MaterialWorkFramePlan` and `WithPrimitivePass` (frame_plan.cpp), the orphaned
`content/material_admission`, `frame/window_host.h`'s `WindowHost`, the write-only `resize_fence_` and `final_format_`.

## Tranche 2 (planned)

From the census: the runner's dead control modes (`ReferenceGlyphCpu`/`Native`, `ReferenceCode` with `code_capture`,
`SelectedObserver`, the `None` mode's `startup_trace`) and its parameters for flags nothing reads (B.2, B.6); the diagnostics of
closed research no kept runner places (`yebis-host`, `yebis-hal-observe`, `yebis-observe` and the small ones, B.4; the
shadow and cascade observers stay for line 6, lot 2b's rule; the effects' observers `ffx-draw-observe`, `ffx-record` and `decal-observe` stay for FINISH-LINE lines
3 to 5); the A/B scaffolding of merged optimisations (`rebuild-collect`, `collect-shadow`, `rebuild-prepare`, `prepare-shadow`,
`prepare-dryrun`, B.5); the two repeated boilerplates (the module-folder lookup, 17 copies; the journal folder); the split of
the largest files (`renderer/scene_pass.cpp`, `04_reference_witnesses.inc`, `13_model_frame.inc`) into files by concern; a
decision on `native/world`'s capture-era tools (the ledger still cites some as oracles). Work done twice in play, if met, is
noted for the performance workstream, not changed here (Thomas, 2026-10-28).

The step review of these answers (fresh Opus, 2026-10-28): no high or medium; the world pictures equal `main`'s, the include not
needed, the build made after the edits; its two lows (STATE's next action, the index row's « no behaviour change ») answered at
the merge checkpoint.

### The start check after tranche 1's merge (`start-92d77dd7`, 2026-10-28 06:08-06:09, the ASUS)

The launcher's module rebuilt from `main` 92d77dd7: the world drawn (354 frames), closed by its close box and gone by itself,
controller exit 0, no refusal, failure or crash, `restoration ok`, no witness picture written (no flag).

## Tranche 2 (in lots, each merged; no behaviour change)

### Lot 2a: the runner's dead modes, the review's L3 leftovers, the two boilerplates

- The runner (`Run-StartupTrace.ps1`): the modes no script passes, `None` (the `startup_trace.exe` launch, its build folder long
  gone), `ReferenceGlyphCpu`/`ReferenceGlyphNative`, `ReferenceCode` and `SelectedObserver`, removed; `-BootstrapMode` is now
  mandatory (`Reference`, `Native`, `Original`, every caller passes one) and `-SteamLaunch` required. The switches that changed
  nothing (`-FramesInFlight`, `-WaitablePresent`, `-RenderThread`, `-PrepareThread`, `-FrameGraph`, `-NoSharedUploads`) and the
  stale-file removals of flags nothing reads went, with their pass-through in `Run-PilotPair.ps1` and `Run-PrepareGate.ps1`.
- The module and its controller: the Reference UI probes of those modes (`ProbeNativeGradientImageAbi`, the glyph ABI and CPU
  delivery probes, the CPU uniform observers and the native mesh-owner qualification in `ObserveLockedCpuMeshInputs`, their hooks
  and state, about 600 lines), `frame/code_capture.{cpp,h}`, `selected_observer*.cpp`, `startup_trace.cpp` and their build
  scripts, `PrimitiveUniformSource::ProduceCurrent2dMvp` (its only caller was a probe). The shared control block lost its two
  fields (`control_version` 4; the module and the controller are built together).
- L3 of tranche 1's review: `SceneRenderer`'s scene state that never left its defaults (`scene_ready_`, `work_mode_`,
  `material_draws_` and `MaterialDraw`, `scene_generation_`, `texture_generation_`, `incarnation_`, `resize_fence_`) and the
  resolve, lighting, display, mask and environment targets, pipelines and readbacks their never-set `*_enabled_` flags guarded
  (only the false path stays: six RTVs, the G-buffer colour 2 as the composition source), `RetireSceneResources`,
  `PassInputExtent`; `MaterialWorkFramePlan` and `WithPrimitivePass`; `content/material_admission` and `material_programs.h`;
  glow's write-only `final_format_`. Left for the lot of the historical tools: `frame/window_host.h` (its types are still used by
  the `output_adapter` prototype) and the `SceneAssets` validators (used by `content`'s material resources, a chain the review of lots
  2b, 2c and 2e found dead: lot 2f).
- Boilerplates: the module-folder lookup written once (`ModuleSibling`, beside `ModuleFlag`, which calls it) instead of 17
  copies; the journal-folder lookup in one helper (`JournalSibling`, the former `WitnessPicturePath`, in the thread-affinity
  scope), which also took four inline copies; six inline copies are left, the YEBIS oracle captures lot 2b removes.
- Also: `10_options_and_passes.inc` sizes its chunk arrays by `std::tuple_size_v<decltype(passes)>` (clang refused `passes.size()`
  on a local in a local struct; MSVC accepted it), so clang now checks the whole unit.
- Proof: the build and its unit tests pass, no warning (`evidence/I-0102/build-2a-review.log`, after the review's answers);
  clang's unused-code warnings (`-Wunused-*`, `evidence/I-0102/clang-unused-2a.txt`) on the module's unit and the changed
  files: none; what clang cannot see (members, functions of other units) checked by a grep of every removed symbol's
  consumers; `Test-ThreadAffinity` and `Test-Project` pass (`evidence/I-0102/checks-2a.txt`; the `PopinLoadFrame` stale entry
  is `main`'s too).

The final review (fresh Opus xhigh): no high, no behaviour change in any kept mode, the runner serving every caller. Answered:
M1, what only the removed probes used (`game_ui_glyph_context.{cpp,h}` and its build entries, the mesh owner's own cache
construction `ConstructCache`/`CpuCacheBytes` and its owned-storage branch, `GameUiCpuMesh`, `ValidSceneFrame`,
`WrittenFloats`, `mirror_last_serial_`, `CpuMeshContext::renderer`, two includes); M2, the journal-folder claim and the
receipts above; L1, the controller's usage line; L3, a truncated module path (`GetModuleFileNameW` filling `MAX_PATH`) is now
no path; L2, STATE's rows. Left: `PassKind::PrimitiveStencilClear`, unused, kept (removing it renumbers the pass kinds). The step
review of these answers (fresh Opus): no high or medium; its L1 (two includes `GameUiCpuMesh` alone used) answered, L2 (the
journal-folder count) answered above, L3 accepted: a module path of `MAX_PATH` characters or more now reads every flag as
absent (before, a truncated file name could still give the right folder); the build folders' paths are about 100 characters.

Merged as `6a3b127a`; the launcher's module rebuilt from it (unit tests passed, `evidence/I-0102/launcher-build-6a3b127a.log`);
the start check `start-6a3b127a` (2026-10-28 07:30-07:31, the ASUS) passed: the world drawn (348 frames), closed by its close
box and gone by itself, controller exit 0, no refusal or failure, `restoration ok`; I-0099's network stop `signalled`, no
0xC0000374 (its third clean quit in a row).

### Lot 2b: the YEBIS research diagnostics, the flags no runner places, the collect's A/B scaffolding

The rule for which diagnostics stay: those that serve the FINISH-LINE lines ahead stay (the effects' observers `ffx-*` and
`decal-*`, the shadow and cascade observers for the spot lights and local shadows, the menu, fade and caption observers for the
menu tests, `light-observe`, `caster-place`, `popin-observe`, the sampler and the cadence trace, the test tools); those of closed
research with no line ahead go. One exception, said here: the YEBIS oracles could serve FINISH-LINE line 8 (a new capture for
the AA types 1-3); they go anyway, a capture then starting from commit `6a3b127a`, where they are whole, because keeping
about 400 lines of Reference-only capture code for a possible use is what this workstream removes.

- YEBIS (W10, closed: the passes are native and on by default): the native library host (`yebis-host.flag`, the whole
  `11_yebis_host.inc`: device factory, buffers, surfaces, frame, its seven hooks) and the HAL recorder only it used
  (`world/yebis_hal.{cpp,h}`, `frame/yebis_hal_test.cpp`, its build and test lines); the Reference observers and oracle captures
  (`yebis-observe`, `yebis-hal-observe`, `yebis-draws-capture`, `yebis-tonemap-capture`, `yebis-capture-late`: about 390 lines
  of `04_reference_witnesses.inc` and their hooks); `Compare-YebisReference.ps1`, which read their rows (WORKFLOW's comparers
  row updated). The feed keeps its scratch post object (`native_yebis_post`, zeroed at each use as before); the
  `native_yebis_globals` row loses its `observed` field (always false, no reader).
- Flags no runner or launcher places (census B.2): `no-ao`,
  `no-ao-resources`, `no-paint`, `no-parallel-walk`; the `native_ao_inputs` row loses its `disabled` field (always false).
  The audio census (`audio-census.flag`) was removed, then restored on the final review's M1: the audio witnesses are open
  (STATE « Open witnesses », movie and NPC dialogue audio) and FINISH-LINE line 8's zones may need it; it is now in the
  runner's `-EffectFlags` so a zone walk can place it.
- Workstream K's A/B (census B.5, the collect's half): `rebuild-collect.flag` and `collect-shadow.flag` with the legacy per-mesh
  blocks and textures, the shadow compares, the block override only they set (`NativeSelectedMesh::BlockOverride`, the record
  outcome `override`), the programme-choice verification of shadow scenes, `LookupNativeMaterialTexture` and
  `NativeDefaultBoundTexture` (only the legacy path called them); every expression reduced to its value with both flags off.
  The runner's `-RebuildCollect` and `-CollectShadow`, the experiments' `RebuildCollect` (Invoke-ReplayRun, Invoke-ReplayGate,
  Run-I97Display) and the closed series runners built on them (`Run-CollectSeries.ps1`, `Run-PrepareGate.ps1`,
  `Get-GateSummary.ps1`) removed. The prepare's half (`rebuild-prepare`, `prepare-shadow`, `prepare-dryrun`) is lot 2c.
- Journal rows changed with the removed state (no reader): `native_yebis_globals` lost `observed`, `native_ao_inputs`
  `disabled`, `native_collect_mode` `rebuild` and `shadow`, `native_model_record_mismatch` `shadow_scene`; the record
  outcomes lost `override` (read by name, `Get-RecordSeries.ps1`).
- How the rule was applied: the YEBIS oracles (`yebis-hal-observe`, `yebis-draws-capture`, `yebis-tonemap-capture`) proved
  every W10 pass, and the hardware tests still replay their captures; a new capture (FINISH-LINE line 8's settings, the AA
  types 1-3 not native yet) starts from the recovery commit `6a3b127a`, where the oracle code is whole. `no-paint` turned
  the paint drain off and no line needs that. Census B.4's small flags are kept for the lines ahead: `pin-census` (the
  start refusals), `recipe-verify` (the programme recipes), `update-observe`, `pose-observe`, `chr-positions` (the
  update and witness work). `-RebuildCollect` and `-CollectShadow` were the duplicated paths of a merged optimisation
  (the CONTRACT's « duplicated paths, one kept »), not a tool's switch.

The final review (fresh Opus xhigh): no high, no behaviour change in any kept mode. Answered: M1, `carry_invalid` (only
the removed compare set it) and its branches; M2, the runner's six YEBIS flags; M3, the receipts
(`evidence/I-0102/clang-unused-2b2c.txt`, `build-2b2c.log`, `checks-2b2c.txt`, on the final tree of lots 2b and 2c); L1,
`ProgramsChoice::owner`; L2, the host's leftovers (the `native_yebis_host_probe` rows, the inert history service
`yebis::History` with its test, which now tests `CameraCut` directly) and the comments the host made true; L3, the stale
comments; L4, STATE, this status, the hook count, the rows above; L5, the paragraph above.

### Lot 2c: the prepare's A/B scaffolding

- The PREPARE-REDUCTION and C4-2 A/B, merged long ago (census B.5, the prepare's half): `rebuild-prepare.flag` (the former
  per-row prepare: bytecode copies, per-draw blocks, every palette check), `prepare-shadow.flag` (kept palette checks,
  matdyn and tiling blocks, frame blocks and kept draws recomputed every 30th time and compared: `NoteValueBlockShadow`,
  `NativeKeptInputDifference`, the kept-draw check at the row's end, the frame-payload prefix check, `CheckNativeEngineBytes`,
  `SetModelDrawShadowPeriod(30)`) and `prepare-dryrun.flag` (the would-be hit rate: `NativePrepareDryRun`, its digests and
  maps); every expression reduced to its value with the three flags off. Rows that only these produced or that only reported
  them lose those fields (`native_collect_mode`'s prepare fields, the prepare split's and the frame audit's
  `shadow_mismatches`; no reader). The positional counters keep their layout, their shadow and dry-run slots at zero:
  kprofile's `Prep*` counters (read by position, `Get-CollectSplit.ps1`) and the kept draws' nine outcomes. The model draw's
  own shadow (`SetModelDrawShadowPeriod`, `ModelDrawShadowMismatches`) stays: its unit tests verify the caches with it.
- The reductions were made by hand, and one was wrong: the final review of lots 2b and 2c (fresh Opus xhigh, H1) found the
  programme-set block (cP1) left as the `else` of the table-copies line once its `if(prepare_rebuild ...)` went, so every
  main-view row other than a tessellated one would have had no programme set and no world drawn. Fixed (the block
  unconditional again, as in `main`); a scan of the whole tranche's diff for an `else` left after a removed `if` finds
  no other. Its lows answered: the stale comments, the reflection carry's leftover block, the runner's six stale YEBIS
  names, STATE's and the index's I-0099 sentences, the receipts (on the final tree, below). Kept: `NativeDryRowKey`'s
  name (the kept draws' row key).
- Scripts: the runner's `-RebuildPrepare`, `-PrepareShadow` and `-PrepareDryRun`, their pass-through in `Run-PilotPair.ps1`,
  `Run-AlphaCast.ps1` and `Run-ZoneWalk.ps1`, the experiments' `RebuildPrepare` (Invoke-ReplayRun, Invoke-ReplayGate,
  Run-I97Display) and the `-PrepareShadow` the closed series runners passed (I-0074, I-0088, I-0090 to I-0094: they still
  run, without it).
- Proof (of the build, not of the behaviour: see the final review below): the build and its unit tests pass, no warning, on the final tree of lots 2b, 2c and 2e (`evidence/I-0102/build-2b2c2e.log`);
  clang's unused-code warnings on the module's unit and the changed units: none (`evidence/I-0102/clang-unused-2b2c2e.txt`);
  what clang cannot see checked by a grep of every removed symbol's consumers; `Test-ThreadAffinity` and `Test-Project`
  (`evidence/I-0102/checks-2b2c2e.txt`). Each receipt names the tree it ran on.

### Lot 2e: the historical tools

The decision (the pilot's, census G.1): `native/world`'s capture-era tools go. They are built by nothing the project runs
(their own `native/world/Build.ps1`), named by no kept script, skill or document as a tool in use, and `native/README.md` already
called them development-only; the ledger's claims made with them (K-0417 to K-0427 and their neighbours: the replays, the live
host) keep their text, and the tools that made them are recoverable from commit `6a3b127a`, the last `main` that holds them
(Git history; no restoration into `main`).

- `native/world`: the live host and simulation, the world adapter, the observers, the frame and GX replays, the light,
  producer, skin and GX checks, the probes, FLVER parsing of the host, the DX11 depth and G-buffer readers, the export, the
  build and capture scripts (31 files, 12,131 lines). Kept: the producers the module links (`cb_reads`, `producers`,
  `light_producers`, `effect_producers`, `light_tiles`, the `yebis_*` blocks and passes, `yebis_pins.h`) and
  `host_input.h` (the module's virtual-file input).
- `frame/`: the cycle-26 output-adapter prototype (`output_adapter.{cpp,h}`, `output_pins.h`, its install test and build,
  `execution_scope.{cpp,h}`, `window_host.h`: 874 lines), built by its own script only; `SceneFrameInput` (only `window_host.h`
  used it). `Run-StartupTrace.ps1` no longer looks for a running `live_host`.
- Left: `camera/` (its `shared.h` is the module's camera record and `content/Build.ps1` builds its sources; a later lot), the
  five unwired tests of census A.1 (`aa_constants_test`, `decal_constants_test`, `gparam_test`, `gbuffer_encoder_test`,
  `host_input_test`: tests of live code, to wire into the build or delete, a later lot).
- Proof: the build and its unit tests pass, no warning (`evidence/I-0102/build-2e.log`); no remaining reference to a removed
  file in any build script, source or kept document (grep); `Test-Project` passes.

### The final review of lots 2b, 2c and 2e (fresh Opus xhigh, on tree `ac22158b`)

No high; every reduction keeps the default path (checked line by line against `main`, the H1 fix in). Its point on the
evidence stands and is said here: no unit test runs the module's collect or prepare, so the hand reductions rest on the
reviews and on the start check after the merge (it catches a world without model draws, not one wrong register). Answered:
M1, the audio census restored (lot 2b above; its two pins in `Build-GameBootstrap.ps1`, L3, stay with it); M2, the
`SceneAssets` chain (`BindMaterialResources`, `ResolveMaterialResources`, `ValidSceneAssets` and its helpers and
structures, and behind them `CpuTextureLoader` with its `MaterialResourceProvider`, which nothing outside their files
uses): named for lot 2f, lot 2a's sentence corrected; L1, the programme set read directly (it is made or throws); L4, the
references to removed files (`.gitattributes`, `host_input.h`, `light_producers.h`, `gbuffer_encoder.h`, the README's build
line), the kept outcomes' comments, the receipts' wording. Left: L2, the `kept_entry` store and two pass-through locals
(harmless); `.claude/settings.local.json` still allows the removed `native/world/Restore-Live.ps1` (Thomas's file, told
him). The answers' build and checks, on their tree `57afa85c`: `evidence/I-0102/build-2b2c2e-answers.log` (the build and
its unit tests, no warning), `checks-2b2c2e-answers.txt` (`Test-ThreadAffinity`, `Test-Project`). Their step review (fresh
Opus): no high or medium; the census restored byte for byte as on `main`, the programme set never null; its four lows (this
receipt, this sentence, STATE's time, the README's test scripts) answered at the merge checkpoint.

### The merge of lots 2b, 2c and 2e (`e3cd8b56`) and its start check

The launcher's module rebuilt from `e3cd8b56` (unit tests passed, `evidence/I-0102/launcher-build-e3cd8b56.log`); the start
check `start-e3cd8b56` (2026-10-28 09:15-09:16, the ASUS) passed: the world drawn (340 frames), closed by its close box and gone
by itself, controller exit 0, no refusal or failure, `restoration ok`, no `native_model_draw_prepare_failed`, and the main view's
preparation as at `start-6a3b127a` (106 rows, 103 draws, nothing missing); I-0099's network stop `signalled`, no 0xC0000374
(its fourth clean quit in a row).

### Lot 2f: the I-0036 and I-0037 witness stack, the CPU texture chain, the unwired tests

Branch `claude/modest-hermann-6e0c3e` from `main` `bb88b0d8`.

- The I-0036 camera diagnostic and the I-0037 content host, built by their own scripts and run by nothing: `native/camera/`
  whole (adapter, controller, host, IPC fixture, snapshot and its test, `shared.h`, six scripts), `content/`'s adapter,
  controller, host, `shared.h`, `diagnostics.h`, `neutral_test`, `writer_test` and its five scripts, `renderer/`'s
  `ownership_test.cpp` with its `Build.ps1` and `Test.ps1` (the `I0036_TEST_FAULT` present failure in `Backend`). The module
  kept two values of `camera/shared.h`: the camera writer's caller return and the receiver's vtable, now in
  `frame/scene_ingress.cpp`; and one of `content/acquire.h`'s I-0037 constants, the FLVER model's vtable, which the model
  selection and the scene geometry check. The rest of `acquire` (the m40 group copy, the asset copy and descriptor, their
  constants) went; `Read`, `SetProcessRead` and `Sha256` stay.
- The witness `Backend` only those hosts and tests drove (`Open`, `Render`, `RenderContent`, `UploadContent`,
  `UseContentRequest`, `WithdrawContent`, `Resize`, `Close`) with its own structures (`NativeResources`, `NativeContent`,
  `PipelineLibrary`), `renderer/intent.{cpp,h}` (`CameraIntent`, `ValidCamera`), `ContentPacket` with its `ValidContent`
  overload, `FrameReceipt::content_request_serial`. `DeviceQueues`, `FramePresenter`, `FrameRing`, `FrameRecorder` (the
  tests use it) and the span `ValidContent` (I-0097's model storage) stay.
- The `SceneAssets` chain (tranche 1's review L3, lots 2b-2e's review M2): `SceneAssets` and its composition, lighting, mask,
  environment and display assets, `SceneMaterialLayout`, `SceneConstantRequirement`, `ValidSceneAssets` and its validators
  (`ValidComposition`, `ValidLightingTexture`, `ValidExplicitLayout`, `VertexElementBytes`, the nine material texture slots);
  behind it `content/material_resources` (`MaterialResourceProvider`, `BindMaterialResources`, `ResolveMaterialResources`),
  `content/texture_loading` (`CpuTextureLoader`), and the startup texture packages `content/cpu_packages` and
  `content/gpu_texture_packages`, which the module never linked (only `game_texture_ingress_test` exercised them; its package
  block went with them). `SceneTexture` stays (the UI, primitive and startup paths use it); `DecodeSceneDDS` and
  `ValidSceneTexturePayload`, left with no caller, went in lot 2g (the whole review's M1).
- The five unwired tests of census A.1, all of live code, now built and run by `Build-GameBootstrap.ps1`:
  `aa_constants_test` (the DLAA constants and their RDEF), `decal_constants_test` (the embedded decal oracle),
  `gparam_test` (the reader's refusals), `host_input_test` (the memory-only host input, run from the repository root) and
  `gbuffer_encoder_test` (warp and hardware).
- `native/README.md`'s rows and build paragraph. Also gone, found by clang once `Backend` was: `backend.cpp`'s own
  `Transition` and `background`, and its `d3dcompiler.h` include.
- Proof: the build and every unit test pass, the five new ones included (`evidence/I-0102/build-2f.log`, on the lot's tree
  before the two clang findings above); clang's unused-code warnings on the changed units and the wired tests
  (`evidence/I-0102/clang-unused-2f.txt`): none after those two; what clang cannot see checked by a grep of every removed
  symbol's consumers (one consumer a truncated grep missed, of `model_vtable_rva`, was caught by the build and the value kept).

### The split of the largest files (no behaviour change)

- `04_reference_witnesses.inc` (1,946 lines) in three parts by concern: `04_updates_and_test_tools.inc` (character, model and
  pose updates, the witness files, the test tools, the game step and the world gate), `04b_pass_witnesses.inc` (material
  preparation, cascades, casters, local shadow slots, decals, effects, glow, GPARAM) and `04c_scene_witnesses.inc` (AO, emits,
  probes, pop-in and LOD, light recording, lightmaps, geometry, velocity, model rows, pose rings). `13_model_frame.inc` (1,964)
  in two: `13_model_frame_inputs.inc` (vertex elements, register blocks, kept draws, decal and paint draws, the pop-in frame
  rows) and `13b_prepare_model_frame.inc` (`PrepareNativeModelFrame`, 1,385 lines: splitting the function itself would be a
  refactor, not a move, and stays out of this workstream). The lines moved unchanged (a script checked that the parts' bodies
  concatenate to the former file's); each part closes and reopens the unit's anonymous namespace at a top-level line.
  `Test-ThreadAffinity`'s allow-list follows the functions to their part.
- `renderer/scene_pass.cpp` (2,611 lines, one translation unit) in six: `scene_pass.cpp` (DDS decode, composition pipelines,
  targets, presentation and pacing, model targets, the startup frame, primitives, resize, display modes, close),
  `scene_pass_lighting.cpp`, `scene_pass_velocity.cpp`, `scene_pass_decals.cpp`, `scene_pass_views.cpp` (menu views, pixels,
  the movie) and `scene_pass_frame.cpp` (the stages opened on first use, the frame job, `RenderModels`, the graph's
  declarations), with the shared helpers in `scene_pass_internal.h` (internal linkage as before, now `inline`). Every line
  moved unchanged (checked line by line) except `inline` on the shared helpers and the barrier sites. A hand-written transition
  is counted by its site (`NoteBarrier(file,line)`: the audit's `barrier_sites`, and the graph shadow's untracked lines); with
  one file the site was (0, line), so the lines of two parts would have merged. Each part now has its own file number
  (`ScenePassFile`, from the name `source_location` gives, kept per thread: `scene_pass.cpp` 0 as before, the other parts 2
  to 6, `primitive_pass.cpp` keeping 1); the table grows from 2 to 7 files, and the graph shadow takes `file*4096+line`
  (unchanged for `scene_pass.cpp`). The lines of the sites changed anyway with every edit of the former file.
- Proof: the build and every unit test pass on the split tree (`evidence/I-0102/build-split.log`); the module's object built
  from the split `.inc` has the same machine code and data as before the split (`scripts/Compare-NativeBinary.ps1 -Objects`,
  88,633 sections, none differing: `evidence/I-0102/compare-inc-split.txt`); the scene renderer's parts are separate units, so
  their proof is the line-by-line check above, the build and the tests; clang's unused-code warnings on the six parts and the
  module: none (`evidence/I-0102/clang-unused-split.txt`); `Test-ThreadAffinity` passes with `scene_pass*.cpp` as one unit and
  its allow-list moved with the functions, the stale `PopinLoadFrame|remaining` entry (`main`'s) removed
  (`evidence/I-0102/checks-split.txt`).

### The whole cleanup's fresh review (two fresh Opus at xhigh, `git diff 9c2ec9e1 ea6085d3`)

Thomas's request (FINISH-LINE 2 bis), made as this last merge's final review: one reviewer on `native/frame/` (the module), one on
everything else (renderer, content, world, the removed `camera/`, scripts, documents), neither having written the code nor seen
the thread.

- The module: no high, no medium; no behaviour change in any kept mode, default or flagged. Every hand reduction of lots 2b and
  2c checked against the old text, the module expanded with its parts at `9c2ec9e1` and at the head and every hunk read; no
  orphan `else`; the flags the module stopped reading are placed by no kept script; the hooks, the start rows' order, the shared
  control block and the kept readers of the changed journal rows checked; the `.inc` split a pure move. Its lows: L1, the
  public `GameUiMeshOwner::Retire` lost its only caller in lot 2a (removed); L2, `Test-GameTextureIngress.ps1` could not link
  (its object list lagged the build's own link of the same test, already on `main`; removed, the build runs the test).
- The rest: no high; no behaviour change. The `scene_pass.cpp` split checked line by line, the barrier sites' file number
  checked (file 0 unchanged, the number of distinct sites the same). Its M1: a second layer of orphans in kept units, whose
  callers the cleanup removed and which clang cannot see (external linkage), and one false RESULT sentence (lot 2f said
  `DecodeSceneDDS` and `ValidSceneTexturePayload` stay for the UI and startup paths: they had no caller left). Answered by lot 2g
  below, the sentence corrected there. Its L1 (STATE, the index row, `status.json`) answered at the merge checkpoint; L2 the
  same script as the module's L2; L3, `ScenePassFile`'s thread-local now in `Test-ThreadAffinity`'s scope, reviewed.

### Lot 2g: the second layer of orphans (no behaviour change)

- Removed (no caller in any build or test): `DecodeSceneDDS`, `ValidSceneTexturePayload` and the DDS helpers `Word`,
  `BlockBytes`, `MipBytes` (`scene_pass.cpp`); `ReadCpuTextureTPF` (`texture_container`); `ReadGparamFile`; the shader bank's
  reader `content/shader_bank.cpp` (no build compiled it since lot 2e; its header keeps the programme types the model programmes
  use); `ProduceTilingScale` with its `MaterialTexture` and `TextureBinding`, `ProduceInstanceIdxOffset`, `ProduceGrassMatrices`,
  `ProduceGrassInstance` with `GrassLighting`, and then `TextureNameKey` (`world/producers`); `Analyze` and then `DestPositions`
  (`world/cb_reads`); the host input's capture-era layer (`Fallback` and its read branch, `Attach`, `Alias`, `PutLines`,
  `WatchFolder`, `Snapshot`, `Of`, `Write`, `SaveTree`, and the `watched` keys with `Watch`, never filled);
  `GameUiMeshOwner::Retire`; `Test-GameTextureIngress.ps1`.
- Named and kept, for the effects workstream (FINISH-LINE lines 3 and 4) that will rework them: the producers' capture layer in
  `world/light_producers` and `world/effect_producers` (`Snapshots` and its loaders, `UseBanks`, `UseCamera`, `UseShadowSetup`,
  `BankCheck`, `ProduceKeyed`, `ProduceSized`, `ProduceBuffer`, `WriterRva`): the module never builds a `Snapshots` nor calls
  `ds3light::Produce`, which only `ao_constants_test` reaches as its oracle, and removing the setters reaches into the producers'
  branches; and the scene renderer's stage observer (`SetObserver`, `SetPresentationObserver`, the 43 `Stage` calls and the
  « told » state they read), whose setters lost their caller in tranche 1, its calls woven into the frame's stage logic.
- Proof: the build and every unit test pass (`evidence/I-0102/build-2g.log`); clang's unused-code warnings on the changed units:
  none after the two it found (`evidence/I-0102/clang-unused-2g.txt`); `Test-ThreadAffinity` passes; every removed symbol
  grepped across `native/` with no consumer left.
- Its step review (fresh Opus): no high or medium; every removed symbol without a consumer, the host input's `watched` and
  `fallbacks` never filled by kept code, no brace or namespace damage. Answered: L1, `scene_pass.cpp`'s header comment; L2,
  `Test-ThreadAffinity` skipped every `const`-led declaration, so a mutable pointer to const (`ScenePassFile`'s `last`) went
  unseen: a pointer to const now counts (a const pointer still does not), and `last` is reviewed in the allow-list; L3, two
  `<fstream>` includes nothing used (`gparam.cpp`, `texture_container.cpp`) removed, the headers' `<filesystem>` left (their
  includers may lean on it). The answers' build and unit tests pass (`evidence/I-0102/build-2g.log`, rewritten).

### The merge (`ce379c84`), its start check and the full control (2026-10-28)

The launcher's module rebuilt from `main` `ce379c84` (unit tests passed, `evidence/I-0102/launcher-build-ce379c84.log`). Then, on
the ASUS alone, announced (11:20 to 13:38), offline, every save restored by hash, Thomas having asked for the full control today
(D-0230, the next three days later):

- The start check `start-ce379c84` passed: the world drawn (357 frames), closed by its close box and gone by itself, controller
  exit 0, no refusal or failure, `restoration ok`; I-0099's network stop `signalled`, no 0xC0000374.
- The replayed path gate `full-1028-gate` with its memory leg: PASS on the ASUS base (period 16.67 ms; frames ending over 2.04 P
  0, over 1.5 P 18.5; display late 6; game steps over 2.04 P 2; both standstill pictures within their threshold; memory -11.8 MB
  a visit), its module built from the committed tree (`evidence/I-0083/gates/build-e31072fb7952`).
- The control sizes with the launcher's module: 1920x1080 in full screen (`full-1028-1080fs`) and a 1500x844 window
  (`full-1028-1500w`), the replayed path each: no fault, restoration held; both end with the controller's 0xD3126612 and
  `closed` 1, as every `-GraphicsLevels` run since I-0100 (not this change); the gate's runs are the 1280x720 size.
- The night endurance `full-1028-night` (the five zones, 93 minutes, 51 loads; `Get-EnduranceSummary.ps1`): no fault, no crash
  dump, 60 frames/s, private memory +1.9 MB a visit, 0 to 1 frame over 34 ms a visit, closed cleanly (exit 0).

I-0102 is complete: its removals merged in four merges (about 29,000 lines), the whole cleanup reviewed, every start check and the
full control passed. Left on purpose (lot 2g above): the producers' capture layer and the scene renderer's stage observer, for
the effects workstream; `PrepareNativeModelFrame` (1,385 lines) unsplit. No work done twice in play was met.
