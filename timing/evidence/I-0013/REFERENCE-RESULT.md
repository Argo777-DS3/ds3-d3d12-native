# I-0013 60-OFF reference result

Session: `20260921T020252Z-pid8916`

## Observation

Thomas completed the announced same-route native reference in standard windowed
mode and reported that behavior was normal. The add-on displayed a successful
save and advanced the next required matrix cell to `240-off`.

## Instrumented capture

- Target/correction: `60 FPS`, correction OFF.
- 994/994 rows valid; zero invalid rows.
- Measured QPC cadence: `60.00051009497101` updates/s.
- Median supplied delta: `0.0166666005` seconds.
- One stable player and transform identity across all valid rows.
- Planar path length: `44.63135119003403` units.
- Net vertical displacement: `-4.506317100000004` units.
- Vertical total variation: `7.203964099999965` units.
- Opposing vertical distance: `1.3485107000000056` units.
- Vertical reversal count: `15`.
- Maximum absolute vertical step: `0.03654480000000149` units.
- Route-resampled second-difference RMS: `0.011431285320049125`.

## Interpretation and limit

This is the valid native reference, not evidence that every vertical reversal is
a defect. The route contains real slope/stair geometry and direction changes;
the metrics become discriminating only through the retained same-route 240-OFF
and 240-ON comparisons. No causal hypothesis is selected yet.
