# R8, « Path tracing complet » : the plan

2026-11-07, the R pilot (RT-PT-PLAN §8 R8, after I-0126 R7). Statuses: OBSERVED (read in the native's code or a cited primary
source), INFERRED, TO CHECK. The rules it follows: D-0237 and D-0249 (a traced picture judged by its truth; a datum misread is a
defect), D-0251 (a raster path with a traced equivalent goes), D-0197 (no stand-in), D-0252 (RR the optimum, NRD without it; SHaRC;
RTXDI under NVIDIA's licence outside Git), RT-PT-PLAN §6 (the PT level: « plusieurs rebonds (ReSTIR GI et cache de radiance) »,
« toutes les lumières, échantillonnées par ReSTIR »).

## 0. In short

The PT level today (OBSERVED, `rt_reflect.hlsl`, `rt_lights.hlsl`): the primary surfaces' sun shadow traced, every local light of
the tile cast with one light a pixel sampled in proportion to its terms (R4c), the specular ambient one GGX ray a pixel on every
surface (R6), the diffuse ambient one cosine ray a pixel (R7a); each ray's hit lit by `HitRadiance` (the sun with its ray, one local
light of the whole list with its ray, its emission) and its own ambient from SHaRC's cell (R7b), or from the baked seed
(`RtAmbientOf`) where the path's footprint is finer than a cell (a mirror, a short segment); the hits have no specular ambient.
ReSTIR GI is written and unbiased but opt-in (R7d). So the PT level's paths stop at their first hit: the cache (or the baked seed)
answers there. R8 makes them go on, measures what that costs, and settles ReSTIR's reuse.

## 1. Elden Ring and the frame challenge

ER has no path tracing (its RT is shadows and a short-range AO-like term; STUDY-R7-ER-GI, RT-PT-PLAN D-0236): no analogue to
copy. The method is the standard one of the shipped PT games (Cyberpunk 2077's Overdrive, Alan Wake 2: a few traced bounces, a
radiance cache at the path's end, ReSTIR for the first vertices; connaissance générale) on the native's own pieces.

- **Sought**: what « several bounces » changes in the picture and what it costs. Engine invariant: DS3's materials and lights as the
  rays already read them (R3, R4); redesigned: how far a path goes before the cache answers.
- **What it unlocks**: the PT level as RT-PT-PLAN §6 defines it; the settings' tiers (R10) need each bounce's cost.
- **Smaller experiment first**: the cost is unknown (R7d's open question, H2): R8a measures the traced passes before any bounce is
  added; the bias check (R7d's tool) measures the cache's error at the end of a path before the default bounce count is chosen.

## 2. The tranches

| # | Content | Seen |
|---|---|---|
| R8a | The traced passes' GPU times in the module (`rt-gpu-measure.flag`): a timestamp pair around each pass (the scene's build, the sun's shadow, the local lights, the cache's update and resolve, the reflections, the water, the GI and its ReSTIR stages or NRD, RR), averaged and logged every 600 frames (`native_rt_gpu` rows), a diagnostic that never fails a frame | Firelink and the High Wall, both levels, at 1280x720, 1920x1080 and an arbitrary window on the ASUS: the table in the RESULT |
| R8b | Several bounces at the PT level (§3): the GI's and the reflections' paths go on past their first hit, B traced vertices before the cache (`rt-bounces.txt`, 1 to 4, default from the measure); the long-path reference for the bias check (`rt-pt-no-cache.flag`, 8 vertices, no cache) | the bias check (B = 1, 2, 3 against the reference), the cost at each B, pictures of the PT level before and after (signalstats), the three sizes |
| R8c | The hits' local lights from a world grid of the frame's lights (§6, decided after R8a's measure; first planned as RIS from a presampled table, RTXDI's « local light presampling », left aside: the grid changes no picture) | the cost against the same build without the grid; the GI mean's equality |
| R8d | ReSTIR GI under RR (R7d): its temporal reuse decorrelated (the temporal neighbour permuted, as RTXDI's « permutation sampling »; a smaller history cap), measured with R7d's bias check and stability figure; the PT default if unbiased and at least as steady as the one ray, else opt-in kept with why; R7d's L5 (an interior, 1,000 frames), L7 (the GI ray written once), L12 (the seed fix on NRD's frames seen). ReSTIR DI's reuse only if a picture of R8b shows a local-light noise RR leaves (R4-PLAN §7) | the bias check and the stability figure at Firelink, the High Wall and an interior |

Each tranche: a step review (fresh Opus, high), the optimisation pass, the final review before its merge (fresh Opus, xhigh), the
launcher rebuilt and the start check after it. R8a and R8b share one run slot when both are written (D-0225: runs grouped), the
measure being what R8b's default is chosen from (said to Thomas: a measure a choice cannot be made without, D-0227).

## 3. R8b as designed

**The path.** At the PT level only (`RT_REFLECT_PT`), a ray's hit no longer takes its ambient from the cache at once: `HitRadiance`
gives the hit's direct light (unchanged: the sun with its ray, one local light with its ray, the emission) plus a **continuation**:
one direction drawn from the hit's two lobes, the diffuse (cosine about the shading normal, its weight albedo × (1 − F0)) or the
glossy (one GGX sample of the visible normals, α = r², its weight DS3's specular ambient term at the hit, E = F0 + (1 − F0)
(1 − sat(N·V))⁵ g², the term `Trace` already uses for the primary surface), chosen in proportion to their luminances, the chosen
weight divided by its probability (an unbiased choice between the two). The next vertex is traced, lit the same way, and so on. The
path's B-th vertex takes its ambient from the cache as today (`HitAmbient`, the footprint rule of R7b's final review M1 kept: a
cell answers only where the path's footprint covers it, else the seed) and no specular ambient; a miss at any vertex brings the
forward sky, else the cubes or the hemisphere rows (`Unmet`), as the first leg's misses. A vertex whose next direction goes under
its triangle's horizon ends the path there with its cache term (the cache's update's rule).

- **B**: the traced vertices of a path, 1 the R7 behaviour (the cache at the first hit). The PT default chosen from R8a's and
  R8b's measures (2 expected: the cache one vertex further, its error weighted by an albedo, about 0.3 to 0.5 in DS3's maps,
  INFERRED). `rt-bounces.txt` (one digit, 1 to 4) beside the module for the runs; R10's menu takes it later (RT-PT-PLAN §6's
  tiers).
- **The fog**: as today, the reflected leg from the primary surface fogged (R6d), no fog on the further legs (R7a: DS3's fog is the
  eye's).
- **What it changes** (INFERRED, to be seen): the PT level's mirrors and the water no longer show the baked seed at their hits (a
  sharp reflection's footprint is finer than a cell, so today its hit keeps `RtAmbientOf`: a baked term at the PT level); the hits
  get a specular ambient (a polished floor seen in a reflection, a wet wall in the GI); the cache's lag (10 frames) and its cells'
  size reach the eye through one more albedo.
- **ReSTIR GI** (opt-in until R8d): its initial sample is the GI's ray, so it carries the path's radiance at its sample point
  unchanged in form (the reservoir stores the second vertex's outgoing radiance toward the receiver, as Ouyang et al.'s).
- **RR's guides**: unchanged (the primary surface's albedos, normal, roughness, the specular hit distance of the first leg).
- **The reference** (`rt-pt-no-cache.flag`, a diagnostic): B = 8 and the last vertex without the cache or the seed (its ambient
  zero): the energy left out is the product of eight albedos (under 1 % with albedos of 0.5, INFERRED), so its mean is the paths'
  truth within that; the bias check compares B = 1, 2, 3 with the cache against it over the still camera's frames.
- **Cost** (H2): each continuation adds a trace, a sun ray, a local light's ray and the vertex's loop over the list (R8c if it
  weighs), for the GI's and the reflections' paths.
- **Not changed**: the RT level (the cache at the first hit), the cache's own update paths (R7b: 4 % of the pixels, three hits), the
  water's target (`water`: its hits keep the first-hit rule; one more path a texel would double its cost for a target the forward
  rows blur, INFERRED; revisited in R11 if a picture asks).

## 4. R8a and R8b's run slot (one slot, the ASUS, offline, restored by hash, the Shebang zone saves)

[Run-R7aGi.ps1](../I-0126-rt-gi/Run-R7aGi.ps1) with R8's modes (`rtgpu`, `pt1gpu` to `pt4gpu`, `ptrgpu`; `pt1mean` to `pt3mean`,
`ptref`), [Get-R8Gpu.ps1](Get-R8Gpu.ps1) for the times, R7d's Compare-GiMean.py for the bias.

- **A, the measure** (the PC entirely free: no build, no Ghidra, no other run; `-Measure`: no debug layer): Firelink (`m40`) and the
  High Wall (`m30`) at 1920x1080, every cost mode; Firelink at 1280x720 and an arbitrary window (1597x899) in `rtgpu` and `pt2gpu`
  (the three sizes, WORKFLOW). DLAA (the render extent the window's), RR on (the PT level's frames are RR's).
- **B, the bias check** (no timing): Firelink and the High Wall at 1280x720, the camera still (`-Still`), `pt1mean`, `pt2mean`,
  `pt3mean` against `ptref`.

## 5. Checks before the code depends on them

- The timestamps' placement: every traced pass is a draw or a dispatch recorded on the main list between the scene's build and RR,
  so one pair a pass measures its GPU time without overlap from another pass of the list (the GPU runs a list's draws in order, with
  overlap only where no barrier separates them: a pass's time may include the tail of the one before; stated with the figures).
- The PT level's frames are only RR's (R4c's rule, OBSERVED in `scene_pass.h`'s `rt_pt`): the runs set DLSS on (DLAA) and RR on.
- Measure, not by eye: every brightness claim by ffmpeg signalstats on the same crop (2026-11-04).

## 6. R8c as designed (2026-11-07, after R8a's measure)

**The measure that asks for it** (I-0128 RESULT « R8a and R8b »): a path vertex costs about as much as the first leg, 6 to 7.5 ms
for the GI and as much for the reflections at 1920x1080, and both witness zones hold about 160 local lights, every one of which a
hit's HitLight evaluates (OBSERVED in rt_reflect.hlsl: the PT level's choice among the whole list, the RT level's sum over it; the
cache update's vertices too). R4-PLAN §1 left the hits without a world-space structure (« with R6 »); R6 and R7 kept the whole list.

**The structure**: the frame's list binned into a world grid about the camera (written: [rt_light_grid.h](../../native/renderer/rt_light_grid.h), from the draft; built on the
collect thread with the list: 32 × 16 × 32 cells of 8 m, 256 × 128 × 256 m, snapped to the cell size), each cell the indices of the
lights whose support's bounding sphere meets it, in the list's order; a hit outside the grid loops over the whole list. Every term
of RtLocalLight is exactly zero outside the light's support (OBSERVED in rt_lights.hlsli: a point's falloffs from d ≥ R, a spot's in
its mode from d ≥ R, without it from q.w ≥ R or outside its cone), so a hit's sum over its cell is the whole list's with zero terms
left out (the same bits), and the PT level's choice, whose random number is drawn per light index only for a light of positive
target, picks the same light: **R8c changes no picture**, only the cost. A spot whose rows do not bound it (checked: the written header) goes into
every cell. ER has no analogue (its local lights stay raster, R4-PLAN §1); RTXDI's ReGIR is the published form of the idea (a
world grid of presampled lights), not needed while the cells' lists are short.

**Seen**: the same cost modes as R8a (Firelink and the High Wall at 1920x1080, RT and PT B = 1 and 2), each with and without the grid
in the same build (`no-rt-light-grid.flag`: the fresh baseline below); the bias check's equality (a still view's GI mean with and
without the grid, ratio 1 within the floor); the cells' list lengths in a row.

Since D-0257 the GI frames keep the sets' fill lights: R8a's and R8b's figures were taken without them, so R8c's and R8d's comparisons
against those figures run with `no-rt-gi-fill` or take a fresh baseline (the rays' cost does not change: DirLights evaluates those rows either way).

## 7. R8d as designed (2026-11-08)

**What R7d left** (I-0126 RESULT « R7d »): ReSTIR GI unbiased over the picture (its native 1/Z resampling), a local residual of 1 to 2 %,
and its displayed frames less steady under RR than the one ray's (mean |ΔY| between pictures 300 frames apart, Firelink 0.84 to 0.94
against 0.65 to 0.68): INFERRED its reused samples correlated in time, which RR's history takes for signal.

**The decorrelation** (rt_reflect.hlsl's gi_temporal and gi_shade; every choice made without looking at the samples, so the 1/Z form
stays unbiased; NVIDIA's RTXDI v3.1.0, read in the pinned archive outside Git, is the source of the three ideas):
- the temporal neighbour permuted: RTXDI's `RTXDI_ApplyPermutationSampling` on the reprojected pixel (its 4 × 4 block mirrored about a
  frame's random offset: a bijection, so on a still camera no history feeds two pixels and a sample moves to another pixel each frame;
  the reprojected pixel itself where the permuted one leaves the screen; RTXDI's DI default, off by default in its GI). Default on in the build, `rt-restir-no-permute.flag` R7d's form;
- the history's cap (RTXDI's GI default 8, kept by default): `rt-restir-history-N.flag`, N 1, 2, 4 or 16;
- a share of the pixels shaded from their initial sample (the one ray's estimate, kept a frame in the surfaces' buffer's third part),
  drawn per pixel and frame: a mixture of two unbiased estimators. RTXDI's ReSTIR PT does this under RR (`decorrelationFactor` 0.4, in
  its « stagnancy » mode driven by the reservoirs' ages smoothed over a neighbourhood and frames, its duplication map, with the history
  20 and its duplication-based reduction); the ages are read from the reservoirs, a sample-dependent choice (biased), so only the uniform form is written: `rt-restir-decorrelate.flag`, 0.4.

**The stability figure** (`world-burst.flag`: the 31 world frames after 2400 presented alone beside the 300th frames' pictures;
[Get-Stability.py](Get-Stability.py)): the consecutive frames' mean |ΔY| (flicker), each pixel's standard deviation of Y over the 32
frames (mean, median, p90: the median is robust to the moving grass and fire), and R7d's 300-apart figure.

**Also**: R7d's L7, the GI's ray written once (`GiRay`: `gi`, gi_signal and ReSTIR's initial sample); L12, the seed fix on NRD's
frames seen (the RT level's GI mean on NRD's frames against RR's, `nrdmean` and `rtmean`); R8's final review L8, the RT level's cost
with PathFrom compiled in (a measure against a throwaway build whose HitRadiance never calls it).

**Seen** (one slot, the ASUS): the cost block first, the PC entirely free (`-Measure -Still`, Firelink 1920x1080: `rtgpu` in this
build and in the throwaway build, `pt2gpu`, `ptrgpu`); then the bias check and the stability figure at 1280x720, the camera still, at
Firelink, the High Wall and an interior (Anor Londo's great staircase hall, `m37hall`), 1,000 frames or more: `ptone` (the reference),
`pt7`, `ptp`, `ptp4`, `ptpd` (and `ptp2`, `ptp4d` if the first ones ask), each with `world-burst`; `nrdmean` and `rtmean` at Firelink.
**The default**: ReSTIR GI the PT level's GI if a form is unbiased (within the floor) and at least as steady as the one ray by these
figures, else opt-in kept with why; its cost said to Thomas (R8a: +4.4 to 8.5 ms at 1920x1080 before R8c).

**Outcome** (RESULT « R8d »): every form unbiased over the picture; the permuted form with the share about as steady as the one ray,
not steadier, for +5.2 ms: opt-in kept, that form its default (`rt-restir-no-decorrelate.flag` replaced the slot's
`rt-restir-decorrelate.flag`); the history's cap left at 8. Anor Londo's hall was not a still view (the teleport's hold moves the
character); Firelink's shrine is the interior measured. PathFrom compiled only into the PT level's builds (`RT_PATHS`).
