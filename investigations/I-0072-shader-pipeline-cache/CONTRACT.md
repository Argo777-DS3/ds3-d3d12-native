# I-0072 - Shader and pipeline cache (wave 2, D-0172, D-0195)

Capability: C3 (wave 2; Thomas 2026-10-20 00:16 Paris, « on peut mettre en place le cache de shaders »; ROADMAP C3 Wave 2,
D-0172, D-0179; first rule D-0195: Elden Ring's architecture, DS3's data).

## Question

How does Elden Ring's engine create, key, keep and persist its pipeline states, and can the native backend adopt that
architecture so that no pipeline already met on this machine is compiled or loaded on the frame path (D-0157: without ER's
compilation stutter), with a checked on-disk cache and a way to rebuild it ("reload shaders", D-0172)?

## Scope

Included:

- the ER study (ER-PIPELINE-STUDY): creation sites, key, in-memory cache, miss path, on-disk library, precompile list, roots;
- the native pipeline cache (`PipelineStore`, backend.h/.cpp) and the sites already on the V1 library (model draws, effect
  stage draw, particle and compute pipelines, point lights, primitive pass): their roots and pipelines through the cache;
- the precompile at the device's creation, on worker threads, from the list the cache writes itself;
- the measurement before/after at the three I-0070 scenes, with the pipeline library cold (a first start, or after a driver
  change) and warm, and Thomas's play journal of 2026-10-19 16:41 (about 200 pipelines compiled in play, 5-10 ms each);
- the launcher's "reload shaders" (the library removed, the list kept: the next start recompiles everything at boot).

Excluded: the stages' fixed pipelines created at their opening (not on the frame path); the first encounter of a pipeline
never met on this machine (predicting pass rows at material admission is a later tranche, measured first); a progress screen
inside the game (the precompile runs while the game boots; a screen only if the measured boot does not hide it); the FFX
replay and world-replay hosts (D-0190 replaces them).

## Inputs

- ER: the analysed Ghidra project (copies `er-mirror-i0072-{a,b}`), ER-PIPELINE-STUDY.md.
- OPTIMISATION-REVIEW §2 (axis 1, the V1 library), §8.1 (pipelines vs collect).
- Code: `native/renderer/backend.{h,cpp}` (PipelineStore), `model_draw.cpp`, `effect_stage.cpp`, `scene_pass.cpp`,
  `primitive_pass.cpp`, `native/frame/game_bootstrap/{02,20}*.inc`; launcher `tools/Launch-DS3-DX12.ps1`.
- Tools: Run-ZoneWalk, Run-AlphaCast (the I-0070 series), `native_frame_audit.pipeline_library`, `native_slow_frame`.

## Hypotheses

- H1: on a cold library the first frames of each zone compile their pipelines on the render worker (ER's synchronous miss):
  the slow windows with `pso_created` (Thomas's play: 55 pipelines and 24 frames over 34 ms in one window).
- H2: on a warm library the remaining frame-path cost is the library load of each pipeline (small), and pipelines first met
  in play (new zones) still compile.
- H3: the precompile from the list at start removes both from the frame path for every pipeline met before on this machine.

## Expected evidence

Before/after on the same build pairs, cold and warm: per-window `pso_created`, frames over 34 ms, p99, slowest frame; the
cache counters (memory hits, precompiled, waits, direct); the precompile's duration against the boot; unit controls on WARP
and hardware (roots, hits, one build per name, file round trip, damaged files); pictures unchanged.

## Exit conditions

- Success: after a warm or rebuilt start, no pipeline compiled or loaded on the frame path at the three scenes; the
  precompile finishes before the first world frame; no regression in pictures, frame times or faults; merged.
- Refuted: H1 false (the slow windows are not pipelines): record it, keep the cache only if it removes measured cost.
- Insufficient information: the counters cannot separate creation, loads and memory hits: add them first.
- Stagnation: no new discriminating evidence after 3 cycles.
- Budget exhausted: close the tranche and replan under `.project/WORKFLOW.md`.

## Safety and human interaction

Live runs under D-0117 (announced in the thread with Paris start and end times, offline, hash-checked restore, test saves).
The cache files live next to the module (the build folder, or the launcher's play folder); nothing in the game folder.
