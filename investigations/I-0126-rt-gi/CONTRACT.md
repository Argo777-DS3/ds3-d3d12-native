# I-0126 - L'illumination globale tracée

Capability: C3 (RT-PT-PLAN §8 R7; D-0235, D-0237, D-0247, D-0249)

## Question

Can the deferred surfaces' diffuse ambient (pass 0's `GB4 × t32.x × t32.y`: the baked lightmaps, the instance SH, the decals' SH)
and the SSAO's part in it be replaced by a traced diffuse bounce, denoised by Ray Reconstruction, with a world-space radiance cache
for the further bounces at the RT level and ReSTIR GI at the PT level, without a stand-in and without counting a light twice?

## Scope

Included:

- A native traced diffuse pass on the main view's frames that already trace (the RT and PT levels), its place in the lighting
  (after `GXLightCmbAlbedo`, before the SSS and the composite: STUDY-R7-GI §2), its hit radiance (R6's `HitRadiance`, plus the hit's
  emission), its misses (R6d's sky, else the light set's hemisphere rows).
- Pass 0's diffuse ambient and the SSAO's channel left out on the frames the GI serves; the shadow mask kept.
- A spatially hashed radiance cache at the RT level (SHaRC), fed by its own paths from a share of the pixels (R7-PLAN §3), read by the GI's hits (and later by R6's
  reflection hits and R9's effects).
- ReSTIR GI at the PT level, after the RT level works (R4-PLAN §7's lesson: its reuse checked for bias before it stays).
- The frames without Ray Reconstruction: what the GI does then (Thomas's choice, R7-PLAN §5).

Excluded:

- The forward rows' ambient (R9), the water's (R9, R11), the specular ambient (R6, done), the maps' Omni fill lights (D-0247: tagged
  for Thomas zone by zone; the light sets' directional fill lights go on the GI frames, D-0254; kept by default since D-0257), the emissive textures as sampled emitters (RT-PT-PLAN §8 lead, later), the
  full PT of R8 (several bounces with ReSTIR DI and GI together).

## Inputs

- Canonical claims and records: R6-PLAN §1 (DS3's ambient), I-0122 RESULT (the RT and PT levels of the direct light), I-0123 (RR's
  guides), I-0125 RESULT (R6a's `RtAmbientOf`, R6b's `HitRadiance`, R6d's sky).
- The study thread's reports, copied here: [STUDY-R7-GI](STUDY-R7-GI.md), [STUDY-R7-DS3-AMBIENT](STUDY-R7-DS3-AMBIENT.md),
  [STUDY-R7-ER-GI](STUDY-R7-ER-GI.md) (non-canonical until the pilot re-reads what a change rests on).
- Tools: clangd and the MSVC build with its unit tests; the start check after every merge (D-0229); short announced witness runs as
  diagnostics only (D-0227).

## Hypotheses

- H1: the traced bounce plugs in after `GXLightCmbAlbedo` as an additive full-screen pass, and RR denoises it with the guides it
  already has (INFERRED, STUDY-R7-GI §2).
- H2: dropping pass 0's whole diffuse ambient on GI frames leaves no baked term behind (the decals' GB4 included, the stencil bit
  0x10 never set by the G-buffer: STUDY-R7-GI §4c, one stencil read TO CHECK).
- H3: the lightmaps' colour holds no direct sun (their alpha is the sun's mask), so a cold-start fallback from GB4 does not count
  the traced sun twice (INFERRED, STUDY-R7-GI §4b; one comparison TO CHECK).

## Expected evidence

The unit tests of each tranche's pure parts; the native's code read with clangd; the pictures of a short witness run per tranche
(the GI frame against the same frame with the baked ambient, the zone's mean brightness read with ffmpeg signalstats before any
claim, `measure-pictures-not-eye`); the start check after each merge.

## Exit conditions

- Success: the RT level's GI (one bounce, the cache for the further bounces) on by default with the rays and RR, the PT level's
  ReSTIR GI behind its level or recorded as left out with why; pass 0's diffuse ambient and the SSAO gone from those frames; the
  frames without RR as Thomas chose; every tranche reviewed and merged, the start checks passed.
- Refuted: the bounce cannot be denoised by RR at one ray a pixel within the frame (a measure would show it): the plan changes.
- Insufficient information: a datum the hits need cannot be read (record it, ask).
- Stagnation: two failed local fixes of the same defect: an RCA.
- Budget exhausted: close the tranche and replan under `.project/WORKFLOW.md`; renew a bounded budget automatically within unchanged
  authorized scope and existing protected boundaries.

## Safety and human interaction

Live runs only as short announced diagnostics through the coordinator's slots, offline, restored by hash, the ASUS when Thomas is
away. Downloading NVIDIA's RTXGI SDK (SHaRC) or any other SDK is Thomas's decision (asked with its source and size). No change to
Windows or NVIDIA settings.
