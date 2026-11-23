# A reactive mask for the temporal passes (I-0113 tranche 39; for the R pilot's TAA and DLSS)

Two open gaps share one remedy: the temporal passes keep history where the picture changes with no motion vector.

- **The Firelink mist** (gap 24's look, RESULT « Tranche 39 »): measured, the mean Sobel gradient of the arch 22.7-25.3 with the TAA
  (the default AA, D-0265), 26.9-32.3 with `no-taa.flag`, 28.9-32.7 in the original (`evidence/I-0113/t37/g24m-*`). The mist is an
  animated effect: its frames blend in the history.
- **Gap 23, option A** (Thomas, 2026-11-08: « On corrige oui si ça n'impacte pas le DLAA »): a fading character drawn as the 0x29
  forward rows blended SRC_ALPHA / INV_SRC_ALPHA (RESULT, gap 23): with the TAA or DLSS the fade doubles the body.

## The mask

A render-extent R8_UNORM target, cleared to 0 each frame, written to 1 (or the draw's alpha) where:

1. an effect draw covers the pixel (the 0x29, 0x2D and 0x31 effect passes; the half-size effect layer's colour alpha, which
   `scene_pass_frame.cpp` already keeps as `effect_layer_.color`, upsampled with its composite);
2. a forward row with a fade (the model's fade weight below 1) covers the pixel.

The effects' programmes are the game's (DXBC): no extra render target can be added to them. Two producers fit without touching them:
the effect layer's alpha (1), and a difference mask (the HDR copied before the effect passes, |after - before| thresholded) for
the full-resolution effect draws; the fading rows' own pass can write the target directly (our programmes' state).

## Its consumers (the R pilot's stages)

- The TAA (`taa_stage`): the history's weight times (1 - mask) where the mask is set (FSR's reactive mask rule).
- DLSS: not a consumer. I-0116 tranche 2 found that DLSS 310.9's models read `kBufferTypeBiasCurrentColorHint` only under the
  deprecated preset F (its guide §3.15; series 5: the mask forced to 1, the presented pictures unchanged), and removed I-0111
  tranche 4's mask for that reason. The mask acts under the native TAA only; DLSS, DLAA and Ray Reconstruction keep their own
  inputs (the R pilot's condition, 2026-11-11: no separate path under PT or RR).

## As built (tranche 39)

No R8 target: the TAA's Resolve computes the mask per pixel from the lit HDR copied once the 0x1F, 0x21 and 0x22 rows (and the
water) are drawn, before the 0x29 rows, the effects and the effect layer's composite (`CopyTaaBase`, a TAA frame only, when a 0x29
row or an effect follows, not on a frame whose forward rows were left unsorted), against the current colour: the largest channel
difference, both exposed and through `c / (1 + max(c))`, taken through a smoothstep from 0.01 to 0.03, takes up to 0.9 of the
history's weight away (FSR 2's thresholded form, softened; the first form, nothing under 0.02 then the difference itself at most
0.9, moved the mist little: RESULT « Tranche 39 »). Every 0x29 row is after the base: the fading rows (gap 23), but also a static
or depth-writing 0x29 row wherever it changes the pixel, which then takes almost no history (as in the original, which has no
TAA). The exposure used is the area's, not the automatic exposure's factor applied later in the tone map (as the TAA's own
weights). Set at one view (Firelink's west entrance); large-coverage effects and other zones not witnessed. `no-taa-reactive.flag`
leaves it out (the A/B).

## The fading rows' own mask (I-0140 tranche 12)

The study's producer 2, as built: a row of a model whose fade moved this frame (not an effect's model particle, not a phantom) draws again, by a second pipeline of its own programmes, into an R8 fade mask
(cleared to 1, 0 where it draws) once the forward and effect rows are drawn, on a TAA frame whose reactive base is copied; over a
marked pixel's 3x3 the TAA takes the history's whole weight and the pixel's own sample. The difference mask above keeps its form for
the fog and the effects (the own sample there would crawl with the jitter). Witnessed live only on its first, broader gate
(`i140-e64`, `build-e66`: every category-0x29 row but a depth pre-pass); the merged gate not yet. Account, films and reviews: I-0140 RESULT « Tranche 12 ».
