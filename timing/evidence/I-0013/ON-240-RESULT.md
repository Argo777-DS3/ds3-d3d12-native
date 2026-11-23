# I-0013 240-ON result

Session: `20260921T020252Z-pid8916`

## Observation

Thomas completed and successfully saved the same-route `240 ON` cell. He reports
the same visible player hopping as in `240 OFF`; he also clarified that the OFF
cell had shown the issue. The correction therefore neither introduces the
observation nor visibly removes it in this route.

## Instrumented capture

- Target/correction: `240 FPS` request, correction ON.
- 4,203/4,203 rows valid; zero invalid rows.
- Measured QPC cadence: `239.03478278063963` updates/s.
- Median supplied delta: `0.00416659983` seconds.
- Same player and transform identity as both preceding cells.
- Planar path length ratio versus 60 OFF: `1.0311095383250495`.
- Route-shape RMS versus 60 OFF: `0.11452969412967011` units;
  detrended RMS: `0.1416585973397361` units.
- Vertical excess increased by `0.4328896000002658` units versus 60 OFF.
- Opposing vertical distance increased by `0.21286760000000982` units.
- Vertical reversal count is 25 versus 15 at 60 OFF and 23 at 240 OFF.
- Route-resampled roughness ratio versus 60 OFF is `1.1205611943779688`, close
  to `240 OFF`'s `1.1145044652685157`.

## Provisional causal classification

The first three ordered cells support H1: the visible issue is present at high
frequency without the accepted locomotion correction and remains present with
it. They do not support H2 (correction-introduced) or H3 (already covered by the
correction). The ON route is 3.11% longer than the reference, so the exact
amplitude difference between OFF and ON is not promoted to an invariant.

This result classifies the correction relationship only. It does not identify
collision, physics, animation, root motion or another subsystem as owner. The
cheap `60 ON` control remains required to close the initial matrix.
