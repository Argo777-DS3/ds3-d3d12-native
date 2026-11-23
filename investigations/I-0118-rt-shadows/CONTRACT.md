# I-0118 - DS3 DX12 Vague 2: R5 of the RT/PT plan, the traced shadows (Les ombres tracées)

Capability: C3

## Question

Can the shadows of DS3's lights be traced through I-0117's scene in rays (R2) in place of the raster's cascades and local atlas,
first the sun, sharp at one ray a pixel (D-0237), so that Thomas can judge the first traced picture (RT-PT-PLAN §8 R5)?

## Scope

Included:

- R5a: the sun's shadow traced into the shadow and AO mask's w (the term the shadowed sun pass and the forward rows read) in
  place of the original's mask layer (GXScreenSpaceCascadedShadow_Soft and its blur), behind `rt-shadow.flag`; the raster and
  traced pictures of the same view shown to Thomas, said honestly what is traced and what is not.
- Then, in tranches: the cascades' caster draws and atlas removed once Thomas accepts the traced sun (D-0237: removed, not
  hidden); the local lights' shadows (the local atlas); the alpha-tested and the wind's grass judged on sight (R2-PLAN T2).

Excluded:

- Penumbrae, denoising and Ray Reconstruction (with R3, D-0237), the materials beyond the alpha test (R3), the lights' list (R4),
  reflections and GI (R6, R7).

## Inputs

- I-0117's scene (the top level, the alpha records, the instance masks: `RtMaskSun` the sun casters), its control view's camera
  (`RtViewCameraOf`).
- The mask layer's states and outputs (SunShadowStage; TESSSCENE-SSMASK-NATIVE §2: w the shadow term, xyz the packed depth for
  the blur; SUN-SHADOWS-C4-STATIC-PREP: OutputColor (0, 0, 1, 1) off the G-buffer stencil bit 0x08).
- The sun's direction: the shadow frame's `sun` (the LightSet's light 0 the cascade fit takes; toward the sun: y = +0.74 at
  Firelink, `native_shadow_state`).

## Hypotheses

- H1: a full-screen pass over the G-buffer pixels with one ray toward the sun, its term in the mask's w, gives the scene's sun
  shadow without the cascades' resolution, extent or fade limits.
- H2: the ray's origin, rebuilt from the depth and pulled back toward the eye, is enough against self-shadowing with the rays'
  LOD-0 geometry (the raster may draw other LODs).

## Expected evidence

The unit test (a quad shadowing a surface on the hardware device), then runs at Firelink and the High Wall with and without
`rt-shadow.flag`: the same view's pictures side by side, the shadows present where the geometry is, no acne, the frames
unchanged elsewhere.

## Exit conditions

- Success: Thomas judges the traced sun shadow (D-0237: the look is his to judge); a data defect (an object missing from the
  shadow, a wrong direction) is fixed.
- Refuted: the traced term cannot replace the mask's w without the layer's other outputs.
- Insufficient information: the pictures do not tell the shadow apart (another view or zone).
- Stagnation: no new discriminating evidence after 3 cycles.
- Budget exhausted: close the tranche and replan under `.project/WORKFLOW.md`; renew a bounded budget automatically within unchanged authorized scope and existing protected boundaries.

## Safety and human interaction

Live runs announced (WORKFLOW « Live game rule »), offline, test saves restored by hash; the ASUS while Thomas sleeps, the OLED
when he is there. No irreversible change.
