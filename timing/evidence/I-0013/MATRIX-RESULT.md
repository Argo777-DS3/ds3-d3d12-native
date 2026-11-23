# I-0013 first four-cell matrix result

Session: `20260921T020252Z-pid8916`

## Valid cells

| Cell | Valid rows | Measured cadence | Operator observation |
|---|---:|---:|---|
| `60 OFF` | 994/994 | 60.000510 Hz | Normal |
| `240 OFF` | 4109/4109 | 239.511036 Hz | Hopping; unintended attack input also occurred |
| `240 ON` | 4203/4203 | 239.034783 Hz | Same hopping as `240 OFF` |
| `60 ON` | 1066/1066 | 60.000510 Hz | Normal |

Thomas accepted the approximately 239 Hz measurements as the intended 240-FPS
condition; the GPU-limited shortfall is retained explicitly and is not treated
as a capture error. All four cells retained one player/transform identity and
zero invalid rows.

## Causal classification

The ordered matrix supports **H1 — native high-frequency defect** for the
bounded visible observation. The hopping is absent at 60 with correction OFF,
present at high frequency before the accepted locomotion correction, unchanged
visually at high frequency with the correction, and absent again at 60 with the
correction. The matrix therefore does not support H2 (correction-introduced) or
H3 (already covered by the correction).

This classification concerns only the correction relationship. It does not
identify collision, physics, animation, root motion or another subsystem as the
owner. The single matrix is sufficient for the first causal discriminator but
not for a precise speed invariant.

## Qualification

The `240 OFF` cell includes an unintended attack caused by the overlay-close
input bug. It remains preserved as evidence for visible hopping, but its active
duration and speed are confounded. The separate distance analysis also finds an
apparent high-frequency slowdown in the clean `240 ON` cell. Thomas explicitly
deprioritized that uncertain side observation in favor of the terrain RCA; no
speed-specific repeat is scheduled and no speed invariant is promoted.
