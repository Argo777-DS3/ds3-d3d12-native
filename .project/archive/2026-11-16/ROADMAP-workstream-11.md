# ROADMAP workstream 11, tranches 1 to 7 (moved out on 2026-11-16, D-0291)

The lines as ROADMAP held them before tranche 8's merge:

  1. The base and the original colorimetry, our own tone map in YEBIS's place, bit for bit (old path behind
     `post-tone-old.flag` until tranche 8). Done.
  2. The bloom, the light shafts rewritten inside it (no bridge to the old pass, D-0276; old path `post-glare-old.flag`). Done.
  3. The exposure rebased on the new pass (one target for the whole game, D-0263, D-0264); Thomas keeps the half strength
     (2026-11-13). Done.
  4. The depth of field and the motion blur, the cutscenes' lens values and draw-parameter files, the Reinhard curve of tone
     type 3 (parts 4a, 4b, 4c). Done.
  5. HDR10 (the float output with the blue-noise dither, the 10-bit PQ output, peak and paper white, the UI in HDR), with
     tranches 5b (the HDR rows on the Graphics tab as sliders, the game's brightness locked on an HDR10 chain), 5c (the
     game's own anti-aliasing filter removed, D-0278), 5d (the HDR sub-rows hidden while it is off) and 5e (the automatic
     exposure never darkens under the zone's own exposure, D-0280). Left: Thomas's judgement by eye on the OLED; **the
     emissive glow is not moved before the tone map** until he judges it there (if kept, the move goes with tranche 6's
     candidate after Elden Ring).
  6. The Rekindled colorimetry: three candidates behind test flags (AgX and ACES among them, one after Elden Ring's shipped
     curve and settings, its slight chromatic aberration kept, D-0276, without its LUT images), merged `1def62857`. Thomas's pick
     (D-0285): the look after Elden Ring alone, an option beside the original (the default), with ER's midtone lift as a curve
     of ours; tranche 7 makes its row.
  7. The settings rows proposed in R12 and not chosen: bloom, light shafts, film grain, field of view, the colorimetry, the
     chromatic aberration. Done 2026-11-16 (merged, its sheets seen by Thomas). 7b: the Rekindled look's vivid variant (D-0296).
