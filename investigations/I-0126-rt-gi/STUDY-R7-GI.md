# R7 ahead: the traced GI that replaces DS3's SSAO and baked ambient (study thread, synthesis)

Study thread S, 2026-11-06 (Paris), read-only, for the R pilot's R7 (RT-PT-PLAN §6 and §8; D-0247: the baked ambient GB4 goes at
R7, the fill lights are tagged for Thomas; D-0249: the traced lighting is judged by its truth, not by the original's pixels).
Non-canonical until R re-reads what a change rests on. Two worker reports carry the detail and their sources:
[STUDY-R7-DS3-AMBIENT](STUDY-R7-DS3-AMBIENT.md) (DS3's ambient and SSAO by family, the frame order, the baked data, the
insertion point) and [STUDY-R7-ER-GI](STUDY-R7-ER-GI.md) (Elden Ring). Re-checked by S: the pass order and its lines in
`native/renderer/scene_pass_lighting.cpp` (355 the SSAO and mask, 406 pass 0 GXLightDeferredAO then the sun, 417 the traced local
lights, 427 GXLightCmbAlbedo, 439 GXLightComposite, 441 the RR guides), the light targets' formats (`:198`: 0 and 1
R11G11B10_FLOAT, 2 and 3 R16G16B16A16_FLOAT), `RtAmbientOf` (`shaders/rt_material.hlsli:336`), ER's RT strings and stage table
(`DS3_evidences/I-0109/er-rt-study/out/strings1.txt`, `mem2.txt:24-28`).

## 1. What R7 replaces, and what it must leave

- **Replaced:** the diffuse ambient term of pass 0, `GB2 × GB4 × t32.x × t32.y` (GB4 the baked lightmap on the lightmapped families,
  about 97 % of m30's and 89 % of m40's map pieces; `(1 − F0) max(0, SH(N))` from the instance's SH rows on the Ibl families; 0 on
  Col/ColSh), and with it the SSAO's role on that term (t32.x). The specular ambient (`SpecularAcc × t32.x t32.y`) is already
  traced since R6 and is not R7's.
- **Left as it is:** the composite and its masks (t32.y and the sun mask GB1.z on the stencil-0x10 pixels: STUDY-R7-DS3-AMBIENT
  §1, which pixels set 0x10 is TO CHECK), the traced direct light (sun, R4's local lights), the SSS, GB5's emission.
- **Not at the source.** Zeroing the baked ambient through `cbPerFrame +0xCC` (FC_DominantColorScale) or the SH rows would also
  remove the forward rows' ambient, which keep their raster lighting until R9: R7 should drop the term in pass 0 on GI frames
  (a pass-0 variant, or a neutral GB4 input), not in the producers (INFERRED, the worker's §3 and §4.3).
- **The fill lights** (`Omni*`, D-0247): tagged for Thomas zone by zone, not removed by R7's code.

## 2. Where the traced bounce plugs in (INFERRED design, agreed with the worker's §4)

A native full-screen pass right after GXLightCmbAlbedo (`:427`) and before the SSS and the composite: it adds `GB2 × (1 − F0) ×
E(x)` into `light_targets[2]` (R16G16B16A16, so a noisy term does not band as it would in target 0's R11G11B10), `E(x)` the
traced diffuse irradiance at the G-buffer pixel. Before the SSS so the skin keeps its blur of the whole diffuse light; before the
RR copy of the lit HDR so Ray Reconstruction denoises it with the guides it already has (albedo, specular albedo, normal and
roughness from GB0..GB3). A frame without GI skips the pass and keeps pass 0's ambient. `E(x)`: one cosine-distributed ray a
pixel (or half resolution at the lowest level), shaded at its hit by the hit radiance R6 already writes (`HitRadiance`: the sun
with its traced ray, the local lights of the list, the hit's own ambient) plus the radiance cache below for the further bounces.
Missing at hits today (the worker's §4.3): the hit's emission and translucency; the emission matters for a GI that lights a room
from its fires (the bonfires, the candles are emissive pieces), so R7 should add it to `HitRadiance`.

## 3. The radiance cache: a recommendation

| Level | Proposal | Why |
|---|---|---|
| RT (one bounce) | one diffuse ray a pixel, the hit lit directly (as R6), **SHaRC** (NVIDIA's Spatially Hashed Radiance Cache) for the bounces after the first | a world-space hash grid, shader-only sources with an integration guide (part of the RTXGI SDK 2.0, DX12 and Vulkan; public, not read in a repository: INFERRED), no neural network, no extra runtime; its update and resolve passes run beside our DXR passes; the cache is also what the effects and transparents read at R9 (RT-PT-PLAN §6 « éclairés par le cache de radiance »), and what R6's reflection hits can use instead of their baked ambient |
| PT | **ReSTIR GI** for the first bounce's paths, SHaRC (or NRC) to end the paths early | the plan's §6 PT row; ReSTIR GI's reuse needs the care R4's ReSTIR DI showed (written twice, found biased, left out: R4-PLAN §7), so it comes after the RT level works |

Not recommended first: **DDGI probes** (stable and cheap, but they leak light through thin walls, and DS3's dungeons, catacombs and
interiors are thin-walled), and **NRC** (the Neural Radiance Cache: better quality, but a neural runtime and tensor work beside DLSS
and RR on a 4070 Ti Super; a later option for the PT level). Both are INFERRED from their public descriptions, not measured here.

**Its seed and its misses.** A ray that leaves the world takes the sky's radiance (R6d's dome in the rays, merged `75eb6c5e`), else
the light set's hemisphere rows (sets buffer rows 12..14, `((U − D)/2 · axis, (U + D)/2)`: the worker's §3, STATIC). The baked GB4
(`RtAmbientOf`, within 5 % of the raster's on 90-100 % of the pixels per zone, R6a) can stand for an empty cache cell while the cache
warms (a new zone, a teleport), fading out as the cell fills; it must not stay in the sum once the cache answers, or the bounce is
counted twice. Whether the lightmaps hold the sun's direct term too: probably not (§4b: their alpha is the baked sun mask the sun pass
multiplies), so the cold-start fallback should not count the traced sun twice; one comparison settles it (§4b).

## 4. Elden Ring (D-0195; its RT workings are not a model, D-0236)

ER's ray tracing (patch 1.09, public) traces **occlusion only** for the ambient (`GXRayGen_ao`, with ray count, min and max length,
alpha, a scale and offset, a temporal feedback and an à-trous filter, a distance fade into the raster AO) and the sun's shadow,
through a DXR state object, after the G-buffer and before `DEFERRED_SHADOW` and `DEFERRED_AOFIELDS` (OBSERVED in its strings and
stage table; the values not read). No radiance bounce, no cache, no irradiance: nothing for replacing GB4. Reusable: the place only
(a traced ambient stage before the deferred ambient composition, a per-effect enable, a distance fade of the traced term), which §2
already follows. ER's TLAS cap, its vendor gate and its 12_0 device are its known defects, not to copy.

## 4b. The lightmaps (added by S, the same day)

- **Their data** (`evidence/I-0063/artifacts/scan/texture-formats.tsv`, the textures named `*_dol_0*`, OBSERVED): every one is
  DXGI 98 (BC7_UNORM, linear), per-piece atlases from 16x16 to 512x512 (most at 64 to 256: 8,665 at 64x64, 7,805 at 128x128,
  5,532 at 256x256), in every map (m30 to m41 counted); their source paths are `…\tex\_<piece>_gi_0000_00_dol_00.tif` and
  `_dol_01.tif` (`scan/textures.tsv`: `g_DOLTexture1` and `g_DOLTexture2`), the baked GI by its name (INFERRED from the name).
- **The sun is not in their colour** (INFERRED, strong): the lightmapped G-buffer programmes write `t26.a` (the first lightmap's
  alpha) into GB1.z (GBUF-FAMILIES, OBSERVED), and GB1.z is the sun's mask on the common pixel (STUDY-R7-DS3-AMBIENT §1): the
  lightmap carries the baked sun visibility in its alpha for the sun pass to multiply, so its colour is the rest (sky and bounce);
  a cold-start fallback from it would not count the traced sun twice. One check settles it: a lightmapped surface in full sun
  against one in the same map's shadow, their GB4 compared (the sun's colour would show in the sunny one's GB4 if it were baked in).

## 4c. The decals' GB4 and the stencil bit 0x10 (added by S, the same day)

- **The decals write their own SH ambient into GB4** (VERIFIED in `gxdecal__Decal_DifSpc.ppo`, fxc `/dumpbin`; the 40 `gxdecal`
  pixel programmes all declare SV_Target4, and their output 4 is GB4 in the Decal7 order [SpecularAcc, GB1, GB2, GB3, GB4, GB5,
  GB0], DECALS-NATIVE §7): `o4.xyz = (1 − F) × max(0, FC_CubeEnvSH1 · (N, 1))`, F the decal's Fresnel-weighted specular, N the
  normal read from the G-buffer (t13), the SH from `cbCubeEnvSH1` (b11); `o4.w` the decal's weight, blended over the surface's
  own GB4. The decal's albedo goes to GB2 (`o2`). So dropping pass 0's whole `GB2 × GB4 × t32` term drops the decals' ambient
  with the surface's, and the traced `GB2 × (1 − F0) × E` keeps the decal's albedo: no baked tint left (INFERRED from the two).
- **Stencil bit 0x10 is not written by the G-buffer draws** (VERIFIED): the two G-buffer applies `140140680` and `140140850` build
  the stencil reference at `state+0x20` from 1 (the decal receiver), |8 (pass type 6), |4 (SSS skin, material +0x24 bit 11),
  |2 (+0x24 bit 7), at most 0xF; the native sets the same bits (`13b_prepare_model_frame.inc:392-398`). The only call of the
  GX depth setter `141920F00` with an immediate 0x10 in the image is the composite's second draw itself (`14010CC20`: its
  constants `3D7F370`/`3D7FE70`, ref 0x10, the reader), found by a scan of the call sites (references through a variable not
  covered). INFERRED: no G-buffer pixel carries 0x10, so the composite's draw 1 (GB1.z on the ambient) never passes, and GB1.z
  is the sun mask everywhere; one stencil read of a frame (TO CHECK) confirms it.

## 5. Risks and checks for R7's plan

- **RR Off** (a player option, D-0247): a 1-ray-a-pixel diffuse term needs a denoiser of its own then (a temporal accumulation
  with the motion vectors and a spatial filter, or NVIDIA's NRD), or the GI level is not offered without RR. A decision for Thomas
  when R7's plan is written.
- **The decals' GB4 and the stencil-0x10 pixels**: both read in §4c; neither leaves a baked term behind once pass 0's ambient
  is dropped.
- **The forward rows** keep the baked ambient until R9: a GI frame then mixes a traced ambient on the deferred surfaces with the
  baked one on the forward rows (the hair, the translucent pieces); acceptable as a step, listed for R9.
- **Measure, not by eye** (2026-11-04): the GI's brightness against the baked ambient read with ffmpeg signalstats per zone before
  any claim; D-0249 judges a natural, true light, not equality, but a zone gone much darker or brighter is an anomaly to explain.
- **The open points** left: IEM's consumers; one stencil read to confirm that no pixel carries bit 0x10 (§4c).
