# I-0013 240-OFF result

Session: `20260921T020252Z-pid8916`

## Observation

Thomas completed and successfully saved the same-route `240 OFF` cell. He
accepted the displayed approximately 239 FPS as the intended high-frequency
condition and attributed the small request shortfall to GPU performance. The
instrumented limiting component remains unmeasured, so the retained statement
is requested 240 / measured 239.511 FPS, accepted under the existing D-0019
qualification rather than claimed as actual 240.

Thomas subsequently clarified that this cell also contained an unintended
attack caused by the known DS3/ReShade overlay-close input bug. The raw trace is
preserved unchanged, but this makes its route-duration/speed comparison
confounded. It remains valid for the visual high-frequency discriminator:
Thomas reports the same player hopping as in `240 ON`.

## Instrumented capture

- Target/correction: `240 FPS` request, correction OFF.
- 4,109/4,109 rows valid; zero invalid rows.
- Measured QPC cadence: `239.5110360044615` updates/s.
- Median supplied delta: `0.00416659983` seconds.
- Same player and transform identity as the 60-OFF reference.
- Planar path length ratio versus 60 OFF: `1.0082027244097833`.
- Route-shape RMS versus 60 OFF: `0.04271525476006573` units;
  detrended RMS: `0.0630524698767553` units.
- Vertical excess increased by `0.3026586000000293` units.
- Opposing vertical distance increased by `0.14847560000004023` units.
- Vertical reversal count increased from 15 to 23.
- Route-resampled roughness ratio: `1.1145044652685157`.

## Interpretation and limit

The trace remains a valid observation of high-frequency hopping with correction
OFF, but it is not a clean speed trial because of the unintended attack. The
complete first matrix supports H1 for the visible hopping relationship; a clean
repeat is required before promoting the apparent 240-versus-60 speed difference.
