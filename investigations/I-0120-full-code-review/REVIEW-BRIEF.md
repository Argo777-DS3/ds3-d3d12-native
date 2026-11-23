# I-0120 reviewer brief

The brief given to each of the eight fresh Opus reviewers (xhigh), base `831023b8`. Their lots are below the brief.

You are one of eight fresh reviewers of the whole native code of « Dark Souls III: Rekindled », read-only. You neither wrote
this code nor saw the threads that did. Review ONLY your lot (below); read code outside it only to follow a call, a type or a
guard that a finding in your lot depends on.

## What the code is

A C++20 module loaded into the running Dark Souls III 1.15.2 (x64, Windows) that replaces the game's D3D11 renderer with a
native D3D12 one. It hooks the engine's functions (detours and pins at fixed addresses), reads the engine's live CPU objects
(scene, models, lights, effects, UI) and draws them itself. `native/frame/game_bootstrap.cpp` textually includes
`native/frame/game_bootstrap/*.inc` in numeric order: one translation unit, so a name used in a later `.inc` may be defined in an
earlier one. Two frames are in flight (D-0187); a render worker thread records and submits while the game's own threads run the
simulation (D-0168); a frame graph derives the resource barriers (D-0171). Diagnostics and experiments are gated by `*.flag`
files read at start (for example `no-yebis.flag`, `rt-view.flag`, `pix-capture.flag`). Ray tracing (DXR) and DLSS (NVIDIA
Streamline) were added recently. Unit tests (`*_test.cpp`) are out of scope.

## The checkout

Root: `C:/Users/User/Desktop/DS3 Decompilation/.claude/worktrees/stoic-bartik-4857fb`, frozen at commit `831023b8`. Never edit,
create, commit, build, launch or run anything that changes files there; read-only Git commands are fine. clangd is available
for navigation: `& "C:/Program Files/LLVM/bin/clangd.exe" --check=<file.cpp> --compile-commands-dir=<root>` (slow on the big
unit; prefer Grep and Read).

## What to look for, in this order

1. Correctness that hurts a player: crashes, GPU hangs or device removal, memory corruption, use-after-free, data races with
   the game's threads or between the render worker and the game thread, lifetime and synchronisation with two frames in flight
   (fences, retirement, descriptor or upload-ring reuse before the GPU is done), resource states and barriers (missing,
   wrong or redundant transitions, UAV barriers), error paths that leak or leave state half-built, integer overflow, sizes
   and indices from game data used without bounds, uninitialised members, undefined behaviour.
2. Reads of the engine's memory: every pointer chased from game objects must be guarded the way the module's own
   conventions guard it (look for its guarded-read helpers); a fixed address or offset used without its pin or check.
3. The mandate: no D3D11 device, bridge, wrapper, translation or fallback in the production path (D3D11 types may appear only
   as layouts read from the engine's memory); nothing drawn with a stand-in (no placeholder texture, mesh or colour shown in
   place of a missing real one: D-0197); where game data is bad, the object is skipped, counted and logged, not the game closed
   (« skip the object »), while the module's own invariants still refuse.
4. Optimise, do not stack: work repeated every frame that only changes on an event; the same data decoded twice for the same
   purpose; a diagnostic that runs or allocates when its flag is off; dead or neutralised code (a branch that can never run,
   a flag nobody reads, a function nobody calls, an earlier attempt left beside its replacement).
5. Lower: clear logic errors with no current symptom, misleading comments that contradict the code.

Do not report style, naming, formatting, comment wording or « could be refactored » opinions. Do not propose rewrites.

## Verify before you report

For every finding, read the callers and the guards on the path and say why they do not prevent the failure. Classify it:
- CONFIRMED: you followed the path and the failure follows from the code as written;
- PLAUSIBLE: the failure depends on a runtime condition you could not settle from the code (say which).
Drop anything you cannot tie to a concrete scenario. A short list of true findings is worth more than a long list of guesses.

## What to return

Your final message is the report, nothing else, in this form (English):

```
LOT {LOT} — {NAME}
Files read: <count>; not read and why: <list or none>

| # | Sev | Status | File:line | Finding (one sentence) | Failure scenario | Fix (one sentence) |
|---|-----|--------|-----------|------------------------|------------------|--------------------|
```

Severity: high (a crash, hang, corruption, wrong image in normal play, or a D3D11 dependency in production), medium (wrong
under a reachable condition, a leak, a measurable per-frame cost, a diagnostic active without its flag), low (dead code, a
latent error, a misleading comment). Rank by severity, at most 25 rows; then one line per severity that has no finding. Then,
under « Notes », at most five lines on anything systemic you saw (a pattern repeated across files) that the table cannot carry.

## Your lot

Your lot is the one named in your prompt; its files are listed under « Lots » below.

## Lots

### Lot A — Module, lifecycle and threading

- `native/frame/game_bootstrap.cpp`
- `native/frame/game_bootstrap_ctl.cpp`
- `native/frame/game_bootstrap_includes.h`
- `native/frame/game_bootstrap_shared.h`
- `native/frame/game_bootstrap/01_state.inc`
- `native/frame/game_bootstrap/02_frame_lifecycle.inc`
- `native/frame/game_bootstrap/03_engine_cpu_owners.inc`
- `native/frame/game_bootstrap/14_scene_step_and_storage.inc`
- `native/frame/game_bootstrap/15_cpu_service_and_input.inc`
- `native/frame/game_bootstrap/17_render_step.inc`
- `native/frame/game_bootstrap/20_startup_phases.inc`
- `native/frame/game_bootstrap/22_setup.inc`
- `native/frame/crash_dump.cpp`
- `native/frame/crash_dump.h`
- `native/frame/engine_window.cpp`
- `native/frame/engine_window.h`
- `native/frame/scene_ingress.cpp`
- `native/frame/scene_ingress.h`
- `native/frame/simulation_time.h`
- `native/frame/frame_store.h`
- `native/renderer/render_worker.cpp`
- `native/renderer/render_worker.h`
- `native/renderer/work_pool.h`
- `native/renderer/frame_pacer.cpp`
- `native/renderer/frame_pacer.h`
- `native/renderer/device_removal.cpp`
- `native/renderer/device_removal.h`
- `native/renderer/cadence_trace.h`
- `native/world/logic_rate.h`
- `native/world/frame_rate_option.h`

### Lot B — Scene collection and the model's frame

- `native/frame/game_bootstrap/05_selected_scene.inc`
- `native/frame/game_bootstrap/12_scene_collect.inc`
- `native/frame/game_bootstrap/13_model_frame_inputs.inc`
- `native/frame/game_bootstrap/13b_prepare_model_frame.inc`
- `native/frame/game_scene_geometry.cpp`
- `native/frame/game_scene_geometry.h`
- `native/frame/game_model_selection.cpp`
- `native/frame/game_model_selection.h`
- `native/frame/shadow_state.cpp`
- `native/frame/shadow_state.h`
- `native/renderer/model_frame.h`
- `native/world/sort_key.h`
- `native/world/view_blend.h`
- `native/world/env_region.h`

### Lot C — Model programmes, admission and content

- `native/frame/game_bootstrap/06_pass_programmes.inc`
- `native/frame/game_bootstrap/08_model_programmes.inc`
- `native/frame/game_bootstrap/09_model_admission.inc`
- `native/frame/game_bootstrap/10_options_and_passes.inc`
- `native/frame/game_bootstrap/18_resource_admission.inc`
- `native/content/aa_constants.cpp`
- `native/content/aa_constants.h`
- `native/content/decal_constants.cpp`
- `native/content/decal_constants.h`
- `native/content/model_common_textures.cpp`
- `native/content/model_common_textures.h`
- `native/content/model_palette_indices.cpp`
- `native/content/model_palette_indices.h`
- `native/content/model_parameters.cpp`
- `native/content/model_parameters.h`
- `native/content/model_pass.cpp`
- `native/content/model_pass.h`
- `native/content/model_pose.cpp`
- `native/content/model_pose.h`
- `native/content/model_pose_binding.h`
- `native/content/model_programs.cpp`
- `native/content/model_programs.h`
- `native/content/model_textures.cpp`
- `native/content/model_textures.h`
- `native/content/model_vertex_layout.h`
- `native/content/paint_constants.cpp`
- `native/content/paint_constants.h`
- `native/content/sampler_bank.h`
- `native/content/sampler_inputs.h`
- `native/content/shader_bank.h`
- `native/content/shader_constants.h`
- `native/content/shader_interface.cpp`
- `native/content/shader_interface.h`
- `native/content/shader_reads.h`
- `native/frame/game_quality_object.cpp`
- `native/frame/game_quality_object.h`
- `native/frame/gx_mtd_image.h`

### Lot D — Lighting and effects

- `native/frame/game_bootstrap/07_scene_lighting.inc`
- `native/frame/game_bootstrap/21_effects.inc`
- `native/frame/effect_system.cpp`
- `native/frame/effect_system.h`
- `native/frame/effect_draw.cpp`
- `native/frame/effect_draw.h`
- `native/renderer/effect_stage.cpp`
- `native/renderer/effect_stage.h`
- `native/renderer/scene_pass_lighting.cpp`
- `native/world/light_producers.cpp`
- `native/world/light_producers.h`
- `native/world/light_tiles.cpp`
- `native/world/light_tiles.h`
- `native/world/producers.cpp`
- `native/world/producers.h`
- `native/world/cb_reads.cpp`
- `native/world/cb_reads.h`
- `native/world/host_input.h`

### Lot E — Renderer core

- `native/renderer/backend.cpp`
- `native/renderer/backend.h`
- `native/renderer/frame_plan.cpp`
- `native/renderer/frame_plan.h`
- `native/renderer/frame_graph.cpp`
- `native/renderer/frame_graph.h`
- `native/renderer/frame_graph_shadow.cpp`
- `native/renderer/frame_graph_shadow.h`
- `native/renderer/scene_pass.cpp`
- `native/renderer/scene_pass.h`
- `native/renderer/scene_pass_internal.h`
- `native/renderer/scene_pass_frame.cpp`
- `native/renderer/scene_pass_views.cpp`
- `native/renderer/scene_pass_velocity.cpp`
- `native/renderer/scene_pass_decals.cpp`
- `native/renderer/scene_view.cpp`
- `native/renderer/scene_view.h`
- `native/renderer/velocity.h`
- `native/renderer/model_draw.cpp`
- `native/renderer/model_draw.h`
- `native/renderer/model_geometry.h`
- `native/renderer/content.cpp`
- `native/renderer/content.h`
- `native/renderer/primitive_pass.cpp`
- `native/renderer/primitive_pass.h`
- `native/renderer/gbuffer_encoder.h`
- `native/renderer/tess_constants.cpp`
- `native/renderer/tess_constants.h`

### Lot F — Ray tracing, post stages, YEBIS, DLSS and shaders

- `native/renderer/rt_scene.cpp`
- `native/renderer/rt_scene.h`
- `native/renderer/raytracing_base.cpp`
- `native/renderer/raytracing_base.h`
- `native/renderer/shaders/rt_material.hlsli`
- `native/renderer/shaders/rt_probe.hlsl`
- `native/renderer/shaders/rt_selftest.hlsl`
- `native/renderer/shaders/rt_shadow.hlsl`
- `native/renderer/shaders/rt_skin.hlsl`
- `native/renderer/shaders/rt_view.hlsl`
- `native/renderer/ao_stage.cpp`
- `native/renderer/ao_stage.h`
- `native/renderer/ao_core.cpp`
- `native/renderer/ao_core.h`
- `native/renderer/aa_stage.cpp`
- `native/renderer/aa_stage.h`
- `native/renderer/glare_stage.cpp`
- `native/renderer/glare_stage.h`
- `native/renderer/glow_stage.cpp`
- `native/renderer/glow_stage.h`
- `native/renderer/glow_math.cpp`
- `native/renderer/glow_math.h`
- `native/renderer/grading_stage.cpp`
- `native/renderer/grading_stage.h`
- `native/renderer/tonemap_stage.cpp`
- `native/renderer/tonemap_stage.h`
- `native/renderer/sun_shadow_stage.cpp`
- `native/renderer/sun_shadow_stage.h`
- `native/renderer/display_stage.cpp`
- `native/renderer/display_stage.h`
- `native/renderer/display_curve.cpp`
- `native/renderer/display_curve.h`
- `native/renderer/post_draws.cpp`
- `native/renderer/post_draws.h`
- `native/renderer/yebis_stage.h`
- `native/renderer/streamline_host.cpp`
- `native/renderer/streamline_host.h`
- `native/renderer/upscaler.h`
- `native/renderer/dlss_mode.h`
- `native/frame/display_blobs.h`
- `native/world/yebis_blocks.cpp`
- `native/world/yebis_blocks.h`
- `native/world/yebis_dof.cpp`
- `native/world/yebis_dof.h`
- `native/world/yebis_glare.cpp`
- `native/world/yebis_glare.h`
- `native/world/yebis_motionblur.cpp`
- `native/world/yebis_motionblur.h`
- `native/world/yebis_pins.h`
- `native/world/yebis_shafts.cpp`
- `native/world/yebis_shafts.h`
- `native/world/yebis_tonemap.cpp`
- `native/world/yebis_tonemap.h`

### Lot G — UI, movies, primitives and texture ingress

- `native/frame/game_bootstrap/16_offscreen_and_movies.inc`
- `native/frame/game_ui_bindings.cpp`
- `native/frame/game_ui_bindings.h`
- `native/frame/game_ui_cpu_shape_context.cpp`
- `native/frame/game_ui_cpu_shape_context.h`
- `native/frame/game_ui_image_owner.cpp`
- `native/frame/game_ui_image_owner.h`
- `native/frame/game_ui_images.cpp`
- `native/frame/game_ui_images.h`
- `native/frame/game_ui_mesh_output.cpp`
- `native/frame/game_ui_mesh_output.h`
- `native/frame/game_ui_mesh_owner.cpp`
- `native/frame/game_ui_mesh_owner.h`
- `native/frame/game_ui_primitive.cpp`
- `native/frame/game_ui_primitive.h`
- `native/frame/game_ui_session.cpp`
- `native/frame/game_ui_session.h`
- `native/frame/game_ui_shaders.cpp`
- `native/frame/game_ui_shaders.h`
- `native/frame/primitive_constants.cpp`
- `native/frame/primitive_constants.h`
- `native/frame/primitive_programs.cpp`
- `native/frame/primitive_programs.h`
- `native/frame/primitive_program_catalog.inc`
- `native/frame/primitive_textures.cpp`
- `native/frame/primitive_textures.h`
- `native/frame/primitive_work.cpp`
- `native/frame/primitive_work.h`
- `native/frame/game_primitive_geometry.cpp`
- `native/frame/game_primitive_geometry.h`
- `native/frame/game_primitive_image_source.cpp`
- `native/frame/game_primitive_image_source.h`
- `native/frame/game_primitive_mesh_source.cpp`
- `native/frame/game_primitive_mesh_source.h`
- `native/frame/game_texture_ingress.cpp`
- `native/frame/game_texture_ingress.h`
- `native/content/acquire.cpp`
- `native/content/acquire.h`
- `native/content/dds_resource.cpp`
- `native/content/dds_resource.h`
- `native/content/gpu_texture_upload.cpp`
- `native/content/gpu_texture_upload.h`
- `native/content/texture_container.cpp`
- `native/content/texture_container.h`
- `native/content/texture_tiling.cpp`
- `native/content/texture_tiling.h`
- `native/content/texture_uv_scope.h`
- `native/content/gparam.cpp`
- `native/content/gparam.h`
- `native/content/content_hash.h`

### Lot H — Diagnostics, witnesses and profilers

- `native/frame/game_bootstrap/04_updates_and_test_tools.inc`
- `native/frame/game_bootstrap/04b_pass_witnesses.inc`
- `native/frame/game_bootstrap/04c_scene_witnesses.inc`
- `native/frame/game_bootstrap/19_ui_witnesses.inc`
- `native/frame/ffx_observe.cpp`
- `native/frame/ffx_observe.h`
- `native/frame/ffx_display_sites.h`
- `native/frame/sample_profile.cpp`
- `native/frame/sample_profile.h`
- `native/frame/sample_symbolize.cpp`
- `native/frame/collect_profile.h`
- `native/frame/dof_capture.h`
- `native/frame/glare_capture.h`
- `native/frame/mb_capture.h`

