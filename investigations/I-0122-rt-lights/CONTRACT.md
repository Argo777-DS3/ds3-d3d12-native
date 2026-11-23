# I-0122 - DS3 DX12 Vague 2: R4 of the RT/PT plan, the lights for the rays (Les lumières pour les rayons)

Capability: C3

## Question

Can DS3's local lights (the engine container's points, legacy points and spots, effect lights included) be held in one
world-space list on the GPU and shaded by the native from it, with DS3's own formulas read from the light programmes' DXBC, so
that the camera's pixels get the original's local lighting without the `GXLightAcc_D*Light*` draws, their shadows come from
the rays instead of the local atlas (R5b), and the same list and formulas later light the rays' hits (R6 to R8)
(RT-PT-PLAN §8 R4)?

## Scope

Included (the tranches of [R4-PLAN](R4-PLAN.md)):

- R4a: the one list (world-space records built at collect from the lights the native already selects) and the native local
  lighting pass from it, on with the rays (`rt-scene.flag`), in place of the unshadowed point, legacy point and spot batches;
  its check against the original programmes on the same frame (`rt-lights-check.flag`, a test tool).
- R4b (R5b): the shadowed lights in the native pass with one traced ray a light a pixel; the local atlas, its faces and its
  casters no longer made on the traced frames once Thomas has seen and accepted the picture (D-0237).
- R4c: many-light sampling (ReSTIR DI) for the path-traced level, where every light casts: after Ray Reconstruction is in
  (R5's remaining half), since one ray a pixel without a denoiser is noise.
- The fate of the fill lights and the baked ambient: a recommendation to Thomas (R4-PLAN §5), his to decide.

Excluded:

- The sun's pass (DS3's programme with the traced mask since I-0118 stays), the forward rows' light grid (GXLightOrganize,
  R9), reflections and GI (R6, R7), the emissive surfaces as emitters (R8, Thomas's lead of D-0237), the line lights (never
  drawn: SPOT-LINE-LIGHTS-NATIVE §4), the lighting pass that retunes the maps' lights (after R7, I-0110 §6).

## Inputs

- The light programmes (the I-0063 programme cache `C:/Users/User/.codex/I0063-cache/programs`, `GXLightAcc_DPointLight1..12`,
  `LegacyDPointLight1..12`, `DSpotLight1..4` and their `Shadowed` variants; `fxc /dumpbin`), I-0119 RESULT « The Fresnel ».
- The native's light collect (`07_scene_lighting.inc` `CollectNativePointLights`, `CollectNativeLocalShadows`), its batches
  (`12_scene_collect.inc` `ProduceNativeSceneLighting`), the lighting pass (`scene_pass_lighting.cpp`).
- The rays' scene and materials (I-0117, I-0119), the traced sun (`rt_shadow.hlsl`, I-0118).
- I-0110 §5 (the light banks and the container), §6 (the baked lighting), §7 (the emissive surfaces).

## Hypotheses

- H1: the local light programmes are a few closed formulas (I-0119 « The Fresnel ») that a native pass reproduces from the
  same items, within the accumulation targets' precision.
- H2: the container holds every light the original lights the frame with (I-0110 §5.2: the effect lights included), so one list
  built from it is the one list the rays need.

## Expected evidence

The programmes' formulas read in their disassembly (OBSERVED); `rt-lights-check`'s counts on one frame per witness zone (the High
Wall, Firelink, m37 for its 524 lights, m45): the native pass against the original programmes drawn into a separate pair of the
same format, pixels within tolerance by target; the traced local shadows' picture shown to Thomas beside the atlas's.

## Exit conditions

- Success: R4a's check within a tolerance set from the first measure at every witness zone, every disagreement explained; R4b's
  picture accepted by Thomas and the atlas removed on traced frames; R4c planned behind Ray Reconstruction.
- Refuted: the native pass cannot reproduce the programmes from the items (a term read from somewhere the items do not hold).
- Insufficient information: the check cannot tell a formula error from the targets' quantisation (then a float target pair).
- Stagnation: no new discriminating evidence after 3 cycles.
- Budget exhausted: close the tranche and replan under `.project/WORKFLOW.md`; renew a bounded budget automatically within unchanged authorized scope and existing protected boundaries.

## Safety and human interaction

Live runs announced (WORKFLOW « Live game rule »), offline, test saves restored by hash; the ASUS (`\\.\DISPLAY1` while the OLED
is off), the OLED only when Thomas is there and says so. The atlas's removal waits for Thomas's acceptance (D-0237). No
irreversible change.
