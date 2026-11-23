# R9-PLAN: the forward rows and the effects lit by the traced light

The pilot's plan (no study thread report for R9). Statuses: OBSERVED (read in code or a disassembly), INFERRED, TO CHECK.

## 0. In short

DS3 lights a forward object or a lit effect from **one probe at its position**: the instance record's three SH rows (`aSh`) that
the forward programmes evaluate per pixel with the pixel's normal, and a per-instance mask on the sun (`ShLightMask.x`). R9 keeps
that design and makes the probe traced: **one traced probe per object**, rays from the object's bounds lit as the GI's rays are
(the sun's ray, the lights, SHaRC's cache at the hit, the PT level's path), averaged over frames, read back by the CPU a few frames
later and written into the same constants. No DS3 programme changes; the deferred surfaces (R7's GI) and the forward ones then
read the same light.

## 1. What the consumers read (study A)

### 1.1 The forward rows (OBSERVED, `fxc /dumpbin`, `evidence/I-0129/dxbc/`)

- `GXFlver_ColDifSpcBumpIbl_Fwd` and `SAT_HairIbl2_Fwd_[A]`: `cbInstanceData` (cb4) per instance: `aSh[3]` at +128 (the record's
  +0x80), `ShLightMask` at +176 (+0xB0). The ambient is `max(dp4((n, 1), aSh[c]), 0)` per channel (lines 510-513 of the first),
  times the AO factor and (1 − F0), added to the lights' diffuse, times the albedo. `ShLightMask.x` multiplies `FC_DirLightCol_0`
  and `FC_DirLightSpc_0` (cb5[4], cb5[8]: line 520, 545), the light set's first directional light, the sun (D-0257: lights 1 to 3
  are the fill lights); the fill lights carry no mask.
- `_FwdSdw` (the original's fading hair, I-0113 « Tranche 21 ») adds a cascaded shadow map lookup (`t32` with a comparison
  sampler, the cascades' rows cb8[38..]) on the sun; the native draws `Fwd` (`model_pass.cpp:48`), so its forward rows take the sun
  unshadowed today, and the cascades are gone with the rays (I-0118).
- The record is ProduceInstanceRecord's (`native/world/producers.cpp:77`): rows from the engine's banks or the animation's own
  (NativeInstanceLighting, `05_selected_scene.inc:894`), the blend vector `(f − 1) · strength + 1` at +0xB0.
- On the GI frames pass 0 reads a black GB4 (`scene_pass_lighting.cpp:760`), so the record's rows reach only the forward rows and
  the decals' GB4, which nothing reads then (INFERRED from R7a: TO CHECK no other reader).

### 1.2 The effects (study A: `evidence/I-0129/study-a/`, GhidraMCP and headless scans, the pilot's re-read of 1DB60)

- The programmes (`evidence/I-0068/queries/ffx/ffx-program-layouts.tsv`, OBSERVED): the 44 bump programmes (`GXFfx*BumpTexture*`,
  `PS_VFX_*Bump*`) read `PSCommonBuffer`'s `g_directionalLightColor` (+16), `g_directionalLightColor2` (+128), `g_vAmbientSH` (+160,
  48 bytes), `g_vLightMask` (+208) and `PSLightBuffer`; the 41 `PS_VFX_*` read `cbCubeEnvSH1` (48 bytes). Disassembled
  (`GXFfxsoftBumpTexture`, `PS_VFX_SoftTextureBump`, `PS_VFX_SoftTexture`, OBSERVED): the same `max(dp4((n, 1), row), 0)` form as
  the instances' `aSh`; `g_vLightMask.x` scales the first directional light's term (cb0[13].x), as `ShLightMask.x` does; the
  non-bump `PS_VFX_SoftTexture` takes the rows' `.w` (L0) as the ambient colour.
- The writer: the draw context's commit **264230** (classic draws) and the GPU particles' update **2A4EB0** (and its twin 2ADC80 /
  2AE5A0) both call the probe query **1DB60** (`GXRS+0x46E8`, DS3's baked probe volumes) at **the effect instance's world
  translation** (ctx+0xA10; obj+0x3A10), a box about it; outputs the three SH rows (param_3, scaled by param_7 at the end) and a
  float4 mask (param_4: the probe's own value replicated; 1DB60 re-read by the pilot, OBSERVED). Then per effect: a manual SH from
  the FXR or the light set's hemisphere (1495F0) may replace it, point lights may be added, the set's strength blends it, and the
  GPU particles smooth it over time. The directional colours are the light set's (a key, no position). 1AC840 (the instances'
  probe) is not on this path.
- So the effects take the same probe as the instances, by position: a detour of 1DB60 that writes the traced rows (× param_7) and
  the sun's share into its outputs reaches the classic draws and the GPU particles alike, after which DS3's own choices (manual SH,
  hemisphere, smoothing) still apply. The probe's key there is the position's cell (an effect has no group the native holds).
- 264230's call (140265428..1402654B7, OBSERVED): RCX `[GXRS]+0x46E8`, RDX ctx+0xA10, R8 ctx+0x560 (the SH), R9 ctx+0x590 (the mask),
  the box at the stack's sixth argument, the scale (XMM12) its seventh; no return value read (a JMP to 140265522), where
  ctx+0x590 is overwritten with ctx+0x9BC replicated: the classic draws' `g_vLightMask` is the FXR's own value, never the probe's,
  so the detour's mask reaches the GPU particles' path only, and the classic bump effects keep the sun unmasked, as the original.

## 2. The traced probe

- **Where**: one probe per object the frame draws a forward row or a lit effect for, keyed by its draw group (an entity's part:
  stable across frames), at most `kRtProbeObjects` a frame.
- **Rays** (a compute entry of `rt_reflect.hlsl`, after the cache's resolve so the hits read this frame's cache): for ray k a
  uniform direction ω on the sphere and a uniform point inside the object's bounds moved along ω to where it leaves the box, so no
  ray meets the object itself; TraceSurface and HitRadiance as GiRay's (the PT level's path included), the sky or Unmet on a miss,
  a back face (the box inside a wall) not counted. Per channel the ambient row is then `f(n) = mean(L) + 2 · mean(L ω) · n`: the
  cosine-weighted mean of the radiance about n, the L1 SH irradiance over π, which is the forward programme's `dp4((n, 1), aSh)`
  form and the unit the GI pass adds (`gi`: `albedo (1 − F0) L` of a cosine ray). Check: a uniform sky of 1 gives 1; an upper
  hemisphere of 1 gives 1 up and 0 down (exact).
- **The sun**: a few rays from points of the box toward the sun (its cone, R4c's), the sun shadow's mask; the visible share into
  `ShLightMask.x` (times the engine's own value).
- **Frames**: the GPU writes this frame's estimate per object; the CPU reads it back when the frame's fence is done (the frames in
  flight, never waiting) and blends it into the object's state (an exponential average, 0.05 a frame since the final review, given on only once it moved by 3 %; a new object takes its
  first estimate whole). Objects with no traced answer yet keep DS3's baked rows.
- **When**: the ambient on the GI frames (R7's frames: the deferred ambient is traced there; elsewhere both stay baked); the sun's
  mask on the traced sun's frames.
- **Cost**: a few hundred objects × 128 rays and as many sun rays (64 before the final review): tens of thousands of rays, against about two million for the GI pass at
  1920x1080 (INFERRED; measured with `rt-gpu-measure.flag`).

## 3. Tranches

1. **R9a**: the probe pass, its readback and the CPU state; the forward rows' `aSh` and `ShLightMask.x` from it; a diagnostic flag
   that logs, per object, the baked and the traced rows; unit tests (the projection, the row form, the state's blend and eviction).
2. **R9b**: the lit effects (§1.2): a detour of 1DB60 (its outputs written after the original's), the probes keyed by the
   position's cell (a metre), the cell its box.
3. Open after R9b: the lightmapped forward rows (`*DOL_Fwd`, `t26/t27`: a map's glass, static, baked like its deferred
   neighbours were before R7: TO CHECK how many draw), the water (R11).

## 4. R9d, the fill lights' traced shadow (a trial; Thomas, 2026-11-08 09:52: « Si on trace les fill lights peut être que ce sera mieux »)

The light sets' lights 1 to 3 (DS3's unshadowed directional fill lights, kept by D-0257) light the hollows evenly and flatten the
traced GI's difference (the sheet `rt-sheet-raster-rt-ptmax.png`). The trial, behind `rt-fill-shadow.flag`, shows them shadowed:

- **Where their occlusion enters (OBSERVED, STUDY-R7-DS3-AMBIENT §1.1):** the deferred directional programme weights each light by
  `1 + w (t32.x − 1)`, t32.x the mask's AO channel (the composite's copy of t33, the neutral white since D-0251) and w the set's
  FC_SsaoWeight (row 17, a light a component). So a pass after the mask's composite that multiplies t32.x by a traced visibility
  (a draw of the reflections' root into the mask target, its red channel only, the blend ZERO / SRC_COLOR), with row 17's light 0
  weight set to 0 on those frames (DS3's weights kept for lights 1 to 3, as built after the final review; (0, 1, 1, 1) with
  `rt-fill-shadow-all.flag`), shadows the fill lights and leaves the sun (light 0, traced already) alone.
- **The visibility** (as built, RESULT « R9d »: light 1 alone against the sun casters' mask left it white): two rays a pixel, each
  picking one of the set's lights 1 to 3 in proportion to its diffuse term at the pixel times its row-17 weight (DirLights' forms), light 1's ray in a wide
  cone (45°) about its direction, a wrap light's about N + L, against what the eye sees (RtMaskViewBits, the GI's geometry: a sky-like
  occlusion, not a hard shadow); 1 where no fill light reaches the pixel. Ray Reconstruction denoises the picture on its frames; NRD's
  frames keep the noise (a trial).
- **Known side effects:** the specular ambient also reads t32.x (darker reflections where the fill is occluded: a specular
  occlusion); the forward rows keep their fill lights unshadowed (their probe could carry a fill share later); the rays' hits keep
  theirs (HitLight).
- **Then:** the same sheet (Firelink, the High Wall, Anor Londo; DLAA 1920x1080) with and without the flag, for Thomas to judge; the
  cost from `rt-gpu-measure.flag` if he wants it kept.
