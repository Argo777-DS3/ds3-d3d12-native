# I-0116 - Les compléments de DLSS

Capability: C3 · [ROADMAP](../../.project/ROADMAP.md) between R1 and R2 (Thomas, 2026-10-31 12:30: « inscris les, dans un ordre
cohérent »): DLSS's two complements after I-0111 (R1) and I-0114 (the unlocked rate). The RT pilot, opened 2026-11-01 06:15 in
the I-0114 thread (Thomas asleep, no fresh thread before his next message; the coordinator's relay 06:04), branch
`claude/i0115-dlss-complements` (named before the number was found taken: I-0115 is the Silver Knight study's).

## Question

What must the native renderer give DLSS for the effects (the FFX particles, trails and E03 model particles) so that DLSS
reconstructs them as it does the opaque scene: the same sub-pixel jitter as the scene they are drawn into, and the motion
the effects' pixels have, without stand-ins (D-0197) and per family (D-0150)? And is the YEBIS motion blur's camera velocity
worth un-jittering while the YEBIS passes are to be replaced (Thomas, 2026-11-01 03:02: « On ne gardera pas YEBIS »)?

## Scope

Included, in tranches, each merged once written and reviewed (D-0227):

1. **The effects' jitter.** The FFX vertex programmes (16 `GXFfx*`) read `g_projectionMatrix` at offset 128 of their VS b0
   `VSCommonBuffer` (the game's captured block; `g_viewProjectionMatrix` at 0 is unused: UPSCALER-DLSS-READY's
   `projection-vars.tsv`, `evidence/I-0068/queries/upscaler-ready/`); the GPU particles' geometry programmes read `m_gViewProjMatrix` at offset
   208 of `gcbSceneParam` (b9). While DLSS jitters the scene, the effects' rasterising projection takes the same NDC shift
   (`upscaler::JitterNdc`, the scene camera's), applied to the native copy of the block at draw time, never to the game's.
   E03 model particles go through the model path (its own constants: to read).
2. **The effects' motion.** Whether the effects should write motion vectors at all (blended, non-occluding: NVIDIA's guide
   leaves them to the reactive mask, which they already set, I-0111 tranche 4) or only their camera motion; decided from DLSS's
   programming guide and a native capture, then written or recorded as not wanted.
3. **The YEBIS motion blur's camera velocity** (a sub-pixel error today): only if cheap, else recorded as left to YEBIS's
   replacement.

Excluded: the SSAO's sub-pixel error (left as it is, Thomas 2026-10-31 12:24: replaced by the traced GI); any change at DLSS off
(the native's output bit for bit as today without DLSS); the game's own constant blocks (copies only).

## Inputs

- [UPSCALER-DLSS-READY](../I-0066-independent-native-startup/UPSCALER-DLSS-READY.md) §1.2, §3.3, §5 (the reactive mask), the
  rasterising matrices outside `cbSceneParam`; [I-0111 RESULT](../I-0111-dlss-reflex/RESULT.md) (« Known limits of this tranche »:
  the effects rasterise with the game's own unjittered matrices and have no motion vectors).
- The native: `native/renderer/upscaler.h` (`JitterNdc`), `ApplyUpscalerJitter` (`01_state.inc`), the effect draws
  (`native/frame/effect_draw.cpp`: the captured constant ranges by slot, `VsB0` among them), the velocity inputs
  (`CollectNativeVelocityInputs`, `12_scene_collect.inc`).

## Hypotheses

- H1: the effects rasterise unjittered (the captured VS b0's projection), so against the jittered scene and depth they sit up to
  half a render pixel off, which DLSS sees as a moving edge on every jitter phase (shimmer of thin effects, soft particles'
  depth fade).
- H2: with the projection jittered like the scene's, the effects' pixels line up with the depth buffer and DLSS's history.

## Expected evidence

Unit control of the projection's shift (the words changed exactly as the camera's, untouched with DLSS off); a native frame's
FFX draws read with their VS b0 words before and after (the `ffx-draw-observe` rows); pictures of a thin effect (the Estus
flame, Lightning Spear) at DLAA with and without the shift, against the original; the one-minute start check after each merge.

## Exit conditions

- Success: the effects jittered with the scene under DLSS, their motion decided and written or recorded, reviewed and merged;
  nothing changed with DLSS off.
- Refuted: the effects already jittered (H1 false): record and close tranche 1.
- Insufficient information: a family whose rasterising constants cannot be told apart.
- Stagnation: two failed local fixes open an RCA.
- Budget exhausted: close the tranche and replan under `.project/WORKFLOW.md`.

## Safety and human interaction

Live runs under WORKFLOW « Live game rule » (announced, offline, hash-checked restore); the OLED with Thomas's leave; nothing
installed in the game folder; Thomas's saves never touched.
