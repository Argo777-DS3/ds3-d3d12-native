# I-0129 - Les effets et transparents éclairés par la lumière tracée

Capability: C3 (RT-PT-PLAN §1 and §8 R9; D-0235, D-0237, D-0254, D-0257, D-0258)

## Question

On the frames the traced GI lights (the RT and PT levels), can the forward rows (the hair, the phantoms, the fading and
translucent models, the FFX model particles) and the lit FFX effects (the bump particles, the GPU particles) take their ambient
and their sun from the traced light instead of DS3's baked probes and unshadowed constants, through the constants DS3's own
programmes already read, at a small measured GPU cost, without a stand-in and without counting a light twice?

## Scope

Included:

- The forward rows' ambient: the instance SH rows (the instance record's +0x80..+0xBF, `cbInstanceData` rows 8 to 11) that the
  forward `*Ibl*_Fwd` programmes evaluate per pixel; the lightmapped forward rows (`*DOL_Fwd`, `t26/t27`) decided in R9-PLAN.
- The lit effects' ambient and directional light: `PSCommonBuffer`'s `g_vAmbientSH`, `g_hemiLightColor0/1`,
  `g_directionalLightColor(2)` of the bump programmes, `cbCubeEnvSH1` of the GPU particles (study A, R9-PLAN §1).
- The sun's visibility at those objects (the forward rows draw `Fwd`, unshadowed, natively; the effects read no shadow).
- A traced probe per object, where DS3 queried its baked probe: rays from the object's bounds, their hits lit as the GI's (the
  sun's ray, the lights, SHaRC's cache), averaged over frames by object; the CPU reads it back frames later and writes the
  constants. Its cost measured with R8a's timestamps.
- Diagnostics behind flags, unit tests of the pure parts.

Excluded:

- The water's lighting (R11 with the m30/m31 water, unless study A shows it is the same constants); its traced reflection is
  R6c's.
- The effects as emitters in the rays (the flames' point lights already light the hits; the emissive surfaces as sampled emitters
  stay RT-PT-PLAN §8's lead); the effects' look itself (E's, I-0113).
- The fill lights' removal (kept, D-0257; R9d only shadows them, a trial behind `rt-fill-shadow.flag`, R9-PLAN §4); the raster lighting's removal and the frame generation (R10); the menu (R12).
- The light shafts and the volumetric fog shadowed by the rays (RT-PT-PLAN §1: later).

## Inputs

- Canonical records: [RT-PT-PLAN](../../.project/RT-PT-PLAN.md) §1 and §8; [I-0126 STUDY-R7-DS3-AMBIENT](../I-0126-rt-gi/STUDY-R7-DS3-AMBIENT.md)
  §2.3 (the forward rows' ambient, OBSERVED in `ColDifSpcBumpIbl_Fwd`) and §3.2 (the instance SH's source); I-0126 and I-0128
  RESULTs (the GI, SHaRC, the PT paths); I-0113 RESULT « Tranche 21 » (the fading hair's `FwdSdw` in the original).
- The effects' programmes and constants: `evidence/I-0068/queries/ffx/ffx-program-layouts.tsv` (OBSERVED: 44 bump PS read
  `PSLightBuffer` and `PSCommonBuffer`'s light words, 41 `PS_VFX_*` read `cbCubeEnvSH1`).
- The native code: `native/world/producers.cpp` (ProduceInstanceRecord), `05_selected_scene.inc` (NativeInstanceLighting),
  `native/frame/effect_draw.cpp` (the captured statements and their constants), `native/renderer/shaders/rt_reflect.hlsl`
  (TraceRadiance, the cache), `rt_scene.cpp`, read with clangd.
- No study thread report exists for R9; the plan is the pilot's ([R9-PLAN](R9-PLAN.md)), with study A (Ghidra, read-only).

## Hypotheses

- H1: DS3 lights each forward object and each lit effect from one probe at its position (OBSERVED for the instances, §3.2; TO
  CHECK for the effects); a traced probe at the same position fills the same constants, so no DS3 programme changes.
- H2: a few hundred objects a frame, 32 to 64 rays each, cost well under a millisecond (INFERRED, TO CHECK by the measure).
- H3: the probes' latency of the frames in flight is invisible for ambient light (DS3 already blends its probes over time; TO CHECK).

## Expected evidence

The unit tests (the SH projection and its row form, the per-object ring); the probes' GPU time (R8a's timestamps, the ASUS);
pictures at Firelink's shrine and the High Wall of the player's hair, a phantom and the bonfire's smoke, before and after, the
brightness by signalstats against the deferred neighbours; the start check after every merge.

## Exit conditions

- Success: the forward rows and the lit effects take the traced probes' ambient and sun on the GI frames by default, each tranche
  reviewed (step and final) and merged, the start checks passed, the cost measured.
- Refuted: a consumer cannot be fed through its constants (a position not readable, a programme that needs a per-pixel term):
  recorded, the alternative put to Thomas.
- Insufficient information: a datum cannot be read: recorded, asked.
- Stagnation: two failed local fixes of the same defect: an RCA.
- Budget exhausted: close the tranche and replan under `.project/WORKFLOW.md`; renew a bounded budget automatically within unchanged
  authorized scope and existing protected boundaries.

## Safety and human interaction

Live runs only as short announced diagnostics through the coordinator's slots, offline, restored by hash, copies of the saves, on
the ASUS while Thomas is away (WORKFLOW « Live game rule »). No download; no change to Windows or NVIDIA settings.
