# I-0013 distance and traversal-time comparison

Session: `20260921T020252Z-pid8916`

## Method

The comparison uses the raw per-update positions. It removes stationary samples
before the first and after the last movement interval so that time spent in the
overlay is not counted. Sensitivity was checked with planar movement thresholds
of 0.001, 0.002 and 0.005 units per update. The accumulated planar route length,
net displacement and QPC elapsed time are then measured over that active window.

## First-matrix result

| Cell | Active duration | Planar path | Planar path speed |
|---|---:|---:|---:|
| `60 OFF` | 11.5000 s | 44.6314 | 3.8810 units/s |
| `240 OFF` | 12.0293–12.1087 s | 44.9397–44.9923 | 3.7157–3.7358 units/s |
| `240 ON` | 12.4986 s | 46.0198 | 3.6820 units/s |
| `60 ON` | 11.6163 s | 45.1678 | 3.8883 units/s |

For the same correction state:

- `240 OFF` versus `60 OFF`: planar path is 0.69–0.81% longer and net planar
  displacement is only 0.33–0.45% larger, but active path speed is 3.74–4.26%
  lower.
- `240 ON` versus `60 ON`: planar path is 1.89% longer and net planar
  displacement is only 0.48% larger, but active path speed is 5.31% lower.
- `60 ON` versus `60 OFF`: active path speed differs by only about +0.19%, which
  is a useful internal control for the method.

## Interpretation and limit

The captures end at nearly the same net displacement, but their active traversal
times are not the same: both high-frequency cells took longer. This is evidence
of an **apparent 240-FPS slowdown** in this manual route, not yet a proved engine
invariant. The `240 OFF` attack directly confounds that cell, and manual input,
route curvature and the visible hopping can affect elapsed time and accumulated
path length. The clean `240 ON` cell shows the same direction, so the attack
alone cannot explain the complete signal.

Promote a speed difference only if a future same-state 240/60 comparison
reproduces under continuous constant input and no attacks. Thomas explicitly
deprioritized a speed-specific repeat so I-0013 can remain focused on the
terrain hopping RCA. The machine-readable values and threshold sensitivity are
in `distance-comparison.json`; the analysis code is preserved in
`analysis/compare_distance.mjs`.
