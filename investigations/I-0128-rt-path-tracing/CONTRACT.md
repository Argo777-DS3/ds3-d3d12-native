# I-0128 - Le path tracing complet

Capability: C3 (RT-PT-PLAN §8 R8; D-0235, D-0237, D-0249, D-0251, D-0252)

## Question

Can the PT level (`rt-level.txt` « pt », on the frames Ray Reconstruction denoises) carry its traced paths past their first hit
(several bounces, diffuse and glossy, each vertex lit by the sun's ray, one sampled local light and its emission, the path ended in
SHaRC's cache), with ReSTIR's reuse kept only where it is unbiased and steadier under RR than the one ray, at a GPU cost measured
on the 4070 Ti Super, without a stand-in and without counting a light twice?

## Scope

Included:

- The traced passes' GPU cost, measured in the module (one timestamp pair a pass, behind a flag), at both levels: R7d's open
  question, and what the bounce count is chosen from.
- Several bounces at the PT level, for the GI's paths and the reflections' (and ReSTIR GI's initial sample, which is the GI's ray):
  the hits' ambient and specular ambient traced one or more vertices further, the cache at the path's last vertex.
- The bias of the cache at the end of a path, checked with R7d's tool (`rt-gi-mean.flag`) against long paths without the cache.
- The light sampling at the path's vertices (today every light of the list a vertex), if the measure says it weighs.
- ReSTIR GI's frames under RR (R7d: correlated in time): decorrelation, then the PT default or opt-in kept, with why; R7d's left
  points (its L5: the check's frame counts and an interior; L7: the GI ray written twice; L12: the seed fix on NRD's frames).
- ReSTIR DI's reuse for the primary surfaces' local lights only if a picture shows a noise RR leaves (R4-PLAN §7).

Excluded:

- The RT level (one bounce, the cache at the first hit: unchanged, R7), the frames without RR (the RT level by RT-PT-PLAN §6).
- The emissive surfaces as sampled emitters (RT-PT-PLAN §8 lead, later); the forward rows and effects lit by the rays (R9); the
  settings menu (R10); the zone tour under PT (R11).

## Inputs

- Canonical records: [I-0126 RESULT](../I-0126-rt-gi/RESULT.md) (R7a to R7d), [R7-PLAN](../I-0126-rt-gi/R7-PLAN.md) §3 (SHaRC), §8
  (ReSTIR GI), [I-0122 R4-PLAN](../I-0122-rt-lights/R4-PLAN.md) §7 (the PT direct light, the reuse left out), I-0125 RESULT (the
  reflections), RT-PT-PLAN §6 (the levels) and §8.
- The native code: `native/renderer/shaders/rt_reflect.hlsl` (HitRadiance, TraceRadiance, GiSample, ReSTIR GI, the cache),
  `rt_lights.hlsl` (R4c), `scene_pass_lighting.cpp`, `rt_scene.cpp`, read with clangd.
- Tools: the MSVC build and its unit tests; the start check after every merge (D-0229); short announced diagnostic runs through the
  coordinator's slots (D-0227); R7d's bias check and `Compare-GiMean.py`; ffmpeg signalstats for any brightness claim.
- No study thread report exists for R8; the plan is the pilot's ([R8-PLAN](R8-PLAN.md)).

## Hypotheses

- H1: at the PT level the cache at the path's first hit carries a bias (its cells' size, its lag) that one or two traced vertices
  more reduce by the albedos along the path; measurable with the bias check against long paths without the cache (INFERRED).
- H2: one more bounce costs about one GI pass more (a trace, its sun ray, its local light ray) a path; the vertices' loop over every
  light weighs where the list is long (INFERRED, TO CHECK by the measure).
- H3: ReSTIR GI's instability under RR comes from its temporal reuse's correlation (R7d's inference); a decorrelated reuse
  (permuted temporal neighbours, a smaller history) keeps it unbiased and steadier (TO CHECK).

## Expected evidence

The unit tests of each tranche's pure parts; the passes' GPU times from the module's timestamps (three sizes, both levels, the ASUS);
the bias check's ratios against the long-path reference; R7d's stability figure (mean |ΔY| of two displayed frames 300 apart, still
camera); pictures of the PT level before and after, brightness by signalstats; the start check after each merge.

## Exit conditions

- Success: the PT level traces several bounces by default at a measured cost, the cache's bias at the end of a path measured; ReSTIR GI
  the PT default or recorded opt-in with why; each tranche reviewed (step and final) and merged, the start checks passed.
- Refuted: the bounces cost more than the PT level's budget (RT-PT-PLAN §7) for no measured gain: recorded, the PT level keeps one
  bounce, Thomas told.
- Insufficient information: a datum the paths need cannot be read: recorded, asked.
- Stagnation: two failed local fixes of the same defect: an RCA.
- Budget exhausted: close the tranche and replan under `.project/WORKFLOW.md`; renew a bounded budget automatically within unchanged
  authorized scope and existing protected boundaries.

## Safety and human interaction

Live runs only as short announced diagnostics through the coordinator's slots, offline, restored by hash, copies of the saves, on
the ASUS while Thomas is away (WORKFLOW « Live game rule »). No new download (RTXDI, SHaRC and NRD are already pinned outside Git);
no change to Windows or NVIDIA settings; nothing deleted (R7d's caches wait for Thomas).
