# Study for R7: Elden Ring's ambient occlusion and global illumination (read-only)

Date 2026-11-06. Read-only reading, no Ghidra, no game. Not committed.

Sources, in the order read:
- ER control-plane repo `C:/Users/User/Desktop/ER Decompilation` (README, AGENTS, ROADMAP, investigations I-0001..I-0008, knowledge/LEDGER.md, knowledge/claims, evidence/I-0001 and I-0003).
- DS3 side: `.project/RT-PT-PLAN.md`, `decisions/LOG.md` D-0195 and D-0236, `investigations/I-0109-modern-dx12-base/RESULT.md`, `investigations/I-0072-shader-pipeline-cache/ER-PIPELINE-STUDY.md`, `investigations/I-0066-independent-native-startup/ER-ENGINE-D3D12-DEEP-STUDY-716.md`, `investigations/I-0068-static-yebis-ffx-lights-spec/REMAINING-FAMILIES.md`.
- The one primary ER source on RT: `C:/Users/User/Desktop/DS3_evidences/I-0109/er-rt-study/` (`REPORT.md`, `out/strings1.txt`, `out/mem2.txt`, Ghidra exports `out/fn*`). It was made by the `ghidra-research` subagent on image `eldenring_2.7.1.0_25080141_1A354710.exe`, and the pilot re-read the device creation and imports. It is DS3's evidence store, not the ER folder.

## 0. Bottom line

1. The ER folder (`C:/Users/User/Desktop/ER Decompilation`) contains NOTHING on AO, GI, ambient, probes, lightmaps, IBL, radiance cache, denoising or ray tracing. Its work is the D3D12 baseline, submission, input and async-payload lifetimes and the renderer boundary (I-0001..I-0008). Its LEDGER has no claim (`knowledge/LEDGER.md:3`, « starts empty »). Its only RT mention is `investigations/I-0008-renderer-boundary/DIRECTOR-MANDATE.txt:755`, « full ray-tracing path » listed OUT OF SCOPE. OBSERVED, by grep of README, AGENTS, `.project`, `investigations`, `knowledge`, `decisions`, `scripts`, and `evidence` (matches only in `evidence/I-0001/artifacts/rtti-raw-offsets.txt`, not read in detail).
2. Everything known of ER's RT is the DS3 repository's I-0109 static reading (R0, 2026-10-30), plus strings and the stage table in the same evidence. It covers AO and the sun shadow only. No raster-ambient reading of ER exists anywhere in either repository.
3. ER's RT AO is occlusion only: a ray-gen `GXRayGen_ao` with a ray count, min and max length, a scale and offset, a denoiser and a distance fade. It is not a radiance bounce and not GI. Nothing in the record shows a probe, radiance cache or irradiance volume in ER.

## 1. How ER's raster ambient works

OBSERVED: nothing is documented. No ER reading exists of its ambient term, AO technique, lightmaps, probes, SH or env maps.

OBSERVED (names only, `DS3_evidences/I-0109/er-rt-study/out/mem2.txt:24-28`, the stage-name table at 143c28a90, 12 entries): `DEFERRED_GEOMETRY_DEFERRED_DECAL`, `DEFERRED_GEOMETRY_WET`, `DEFERRED_GEOMETRY_POST`, `DEPTHSCALE`, `VOXELIZATION`, `RAYTRACING_POSTGEOM`, `DEFERRED_SHADOW`, `DEFERRED_AOFIELDS_PRE`, `DEFERRED_AOFIELDS`, `DEFERRED_AOFIELDS_POST`, `DEFERRED_SSR`, `FORWARD_SSR`. Also `CascadedShadowAtlas` (`out/strings1.txt:35`). The class names are not enumerated here; only the stage names are read.

INFERRED (from these names; same engine family as DS3): ER's raster path has a deferred pipeline with an « AO fields » stage (pre, main, post) and SSR. DS3 has a class `GXSR::GXAOFieldsRenderer` with baked per-map AO volumes (`GXAOFieldsFileData`, I-0068 REMAINING-FAMILIES.md:72). The stage name `DEFERRED_AOFIELDS` suggests ER keeps that baked-volume AO. TO CHECK: that this stage is the same technique as DS3's; the stage name alone does not establish it.

TO CHECK: what `VOXELIZATION` is (a GI voxel grid, shadow or fog voxelization, or something else). It sits before `RAYTRACING_POSTGEOM`. Nothing read says; do not assume GI.

Not found anywhere (grep and reading): an SSAO/GTAO/HBAO name for ER, lightmap, SH, irradiance volume, probe, IBL or radiance cache strings for ER. Absence in the strings read (`strings1.txt` is a targeted string list, not a full string dump) is not proof of absence in the engine.

Comparison with DS3 (DS3 side, from the plan; not re-verified in this study): DS3 composes ambient from a baked lightmap or instance SH into the G-buffer target GB4 (the « ambient irradiance » channel, RT-PT-PLAN.md:35, the origin « à vérifier »), SSAO into t32, and composes both in `GXLightDeferredAO` (as given in the brief, not re-checked here). DS3 also has `GXAOFields` baked volumes and `GXLightDeferredAOWithSsr`, never drawn in any witness (I-0068 REMAINING-FAMILIES.md:62-72, COVERAGE-MATRIX.md:62). So ER and DS3 plausibly share the same AO-fields family and the same lineage; this is INFERRED from stage and class naming only.

## 2. What ER's ray-traced mode traces for ambient

### 2.1 OBSERVED, from the I-0109 reading (`DS3_evidences/I-0109/er-rt-study/REPORT.md`)

- Entry points: `GXRayGen` (generic), `GXRayGen_ao`, `GXRayGen_dirLightShadow` ray generation shaders; miss shaders `GXMiss_shadow`, `GXMiss_radiance`; per-material hit groups with closest-hit, and any-hit for alpha; payload 12 bytes, attributes 8 bytes, recursion depth 1 or 2 (`REPORT.md` « Pipeline », builder `FUN_141a3cad0`).
- A radiance miss shader `GXMiss_radiance` and a generic `GXRayGen` exist. TO CHECK: what they are used for. They are not shown to be used by AO. They could be a radiance query for RT reflections or a leftover; the record does not say. The brief's « possibly RT reflections » has no support in the record beyond this name.
- Execution: DXR state object plus `DispatchRays` (no inline ray query seen). One dispatch per enabled effect (AO, sun shadow) in the per-frame executor `FUN_141be7310`, after the TLAS build. Results are denoised (temporal feedback; à-trous for AO) and faded by distance into the raster result (`REPORT.md`, « Per-frame executor »).
- Place in the frame: stage `RAYTRACING_POSTGEOM` between `VOXELIZATION` and `DEFERRED_SHADOW`, i.e. after the G-buffer is built and before the deferred shadow and AO-field stages (stage table `mem2.txt:24-28`; the placement is INFERRED from table order, the table is a stage-name array, not a proven execution order).
- Exposed parameters (strings in `out/strings1.txt:38-70`, names only, values NOT read):
  - master: `CS.Graphics.IsEnableRayTracing`, `Gconfig_RaytracingQuality` (up to 5 levels per the report), `RaytracingQuality`, `RayTracing Enable AO`, `RayTracing Enable DirLightShadow`.
  - AO: `RayTracing AO Ray SampleCount`, `... Ray LengthMin`, `... Ray LengthMax`, `... Ray EnableAlpha`, `... AO Scale`, `... AO Offset`, `... PostProcess EnableDenoise`, `... PostProcess FeedbackWeight`, `... PostProcess ATrousFilterSteps`, `... Composite EnableFadeRate`, `... Composite FadeStartDist`, `... Composite FadeRangeDist`.
  - sun shadow (for contrast): Ray SampleCount/LengthMin/LengthMax/EnableAlpha, ConeRadius, AdjustRayStartPosition, EnableDenoise, FeedbackWeight, Composite fade.
  - scene scope: `Draw.RaytracingLimitInstanceNum`, `CS.Graphics.Raytracing.Lod.{DistScaleMin,ActivateDistMin,PlayDist}`, `IsEnableRayTracingRegisterSkydomeMapParts`.
- Acceleration structure: TLAS rebuilt whole each frame (`PREFER_FAST_TRACE`, capacity 1000 growing to 2000, extra instances dropped); static BLAS built once; skinned BLAS by compute skinning then refit, staggered by distance, rebuilt after N refits, no compaction (`REPORT.md`, « Acceleration structures »).

So the ambient mode is a classic RTAO: SampleCount rays per pixel (value unknown) of bounded length (LengthMin to LengthMax, values unknown), a hit/miss visibility, Scale and Offset remapping, a temporal accumulation with FeedbackWeight, an à-trous spatial filter with configurable steps, then a distance fade-out into the raster AO result.

### 2.2 What is NOT established

- OBSERVED absent: no ER evidence of a radiance bounce, a cache (screen, world, probe), irradiance or SH from rays, or any multi-bounce.
- Values: ray counts, lengths, filter step counts, feedback weight, fade distances and what each of the 5 quality levels changes are NOT read. They are parameter names. The tables behind `RaytracingQuality` (`142c017d8`, referenced at `143b4d110`) were not decoded. TO CHECK if wanted; it would need a static read of those tables or a param-file dump (heavy, and not for now on this shared PC).
- INFERRED from names: the AO is a hemisphere-ray visibility AO written to a buffer used by the deferred AO stage; whether it replaces or modulates the raster AO fields term is TO CHECK.
- Denoiser: a hand-written temporal-feedback plus à-trous (names `FeedbackWeight`, `ATrousFilterSteps`); the algorithm itself (reprojection, variance guidance, history validation) was not read. Not a vendor denoiser (no NRD/DLSS-RR import was seen; `REPORT.md` shows only d3d12.dll and the inbox runtime). INFERRED from imports and absence of DLSS in ER (the public record, general knowledge, not read here: ER has no DLSS).

### 2.3 Public record (general knowledge, not read in the ER folder)

ER patch 1.09 (2023-02) added the RT option for the PC: RT shadows and RT AO (the DS3 plan cites 1.09, RT-PT-PLAN.md:58); the public coverage described it as GPU-heavy with little visual gain, and a reported crash/stutter behaviour. Public coverage did not describe RT reflections or GI for ER. INFERRED agreement with the strings: shadows plus AO only. This is public knowledge, not verified against a primary source here.

## 3. Reusable for R7, and not

Rule in force: D-0195 (copy ER's engine architecture) is amended for the RT/PT plan by D-0236: ER's RT QUALITY and WORKINGS are « not a model » (Thomas, 2026-10-30), its PLACE in the architecture may be observed. This governs the verdicts below.

Reusable (place and plumbing, per D-0236; OBSERVED unless noted):
- The RT world is separate from the camera draw lists: models register BLAS instances world-wide (`GXRtWorld`, flat vector under a lock). DS3's R2 RT scene already follows this (`I-0109` RESULT.md:43; `I-0117`).
- The stage lies after the G-buffer and before the deferred shadow and AO stages: a pre-lighting insertion. For R7 this means traced GI results arrive before `GXLightDeferredAO` composes ambient. INFERRED from table order, TO CHECK against ER's real frame graph before copying it.
- Scope control exposed as parameters: an instance cap, a LOD/distance gate for what enters the RT world, a quality enum, a per-effect enable, a distance fade of the traced term into the raster one. The distance fade and LOD gating are worth keeping as ideas for the radiance cache's range; they are INFERRED to work as named.
- A temporal feedback plus à-trous structure is the standard recipe; DS3's R7 will use Ray Reconstruction (I-0123) and/or a ReSTIR/cache reuse instead.
- RT disabled in cutscenes (`CSRemoDisableRayTracingAct`, INFERRED): worth a hook point for DS3 cutscenes.

Not reusable / not applicable:
- ER's RT is occlusion-only (visibility at bounded range). It gives no irradiance, no color bleeding, no sky/sun bounce, no multi-bounce: it cannot substitute DS3's GB4 (baked ambient irradiance) or the one-bounce + radiance cache target of the RT level, nor ReSTIR GI + cache at PT. The replacement of the baked ambient term has no ER precedent.
- The workings are rejected by D-0236 as « mal foutu »: DXR state objects with `DispatchRays`, no inline ray query (DS3 uses DXR 1.2 and inline RayQuery in R0), TLAS rebuilt whole every frame with a hard 2000-instance cap that silently drops (DS3's R2/Mega Geometry plan uses a partitioned TLAS), skinned BLAS refits without compaction. Known ER defects: RT forced off on adapters whose name contains « gtx » and « geforce »; the 12_0 device level; instance cap.
- Shader packaging (offline DXIL libraries in encrypted archives) and the inbox-runtime-only device route differ from DS3's dxc build and DXR 1.2.
- Unknown ER numbers (rays per pixel, lengths) cannot be a calibration target; the DS3 AO radius and sample counts have to come from DS3's own data and a comparison with the D3D11 original (D-0237: the original is the comparison reference).

## 4. Questions for the R7 plan this study leaves open (TO CHECK)

1. What does ER's `VOXELIZATION` stage produce and who reads it (GI voxels or not)? A short static read around the stage's executor would settle it; it needs Ghidra, so not now.
2. Is the `DEFERRED_AOFIELDS` result combined with the RT AO (multiplied, replaced)? Needed only if one wants to know whether ER keeps baked AO under RT.
3. What do `GXMiss_radiance` and `GXRayGen` do (an unused radiance path, reflections, or a debug path)?
4. DS3 side, not ER: the baked-ambient origin (lightmaps versus instance SH versus probes) in GB4 is already a TO CHECK of the plan (RT-PT-PLAN.md:35). Its answer, not ER, decides what R7 has to replace. This study cannot supply it.

## 5. What the folder does not cover (summary)

- ER raster AO technique, baked ambient/GI (lightmaps, probes, SH, env maps, IBL): not covered, anywhere. Only stage names (`DEFERRED_AOFIELDS*`, `VOXELIZATION`, `DEFERRED_SSR`) were seen, via DS3's I-0109 evidence.
- ER RT AO values (ray count, distance, quality levels): names only; no values read.
- ER RT reflections and GI: no evidence either way; the public record says shadows and AO only.
- ER denoiser algorithm: only the existence of temporal feedback and an à-trous filter.
- The ER repo itself (`C:/Users/User/Desktop/ER Decompilation`) contains no rendering-effects work at all; its ledger is empty.

Statuses: ER has an RT AO that is occlusion-only with its own denoiser, OBSERVED (names, flow) / values TO CHECK; its raster ambient, UNKNOWN; its relevance to R7's GI, small (place in the frame and the distance-fade idea reusable, workings excluded by D-0236).
