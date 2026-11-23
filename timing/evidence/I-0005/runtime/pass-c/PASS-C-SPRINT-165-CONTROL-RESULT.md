# I-0005 Pass C — measured 165-FPS sprint divergence

## Outcome

**REPRODUCED.** One complete target-165 AZERTY sprint trial travelled almost
exactly half the distance-per-accumulated-delta of the accepted target-60
baseline while preserving the same animation ID and animation progression.

This confirms Thomas's operator observation for sprint. It does not yet locate
the defect between root/requested motion, character displacement production,
and collision/physics application.

## Direct comparison

| Metric | Target 60 baseline | Target 165 trial | 165 / 60 |
|---|---:|---:|---:|
| accumulated delta | `3.016654721 s` | `3.000007211 s` | — |
| net distance | `17.780330886` | `8.848791129` | — |
| sampled path length | `17.978603194` | `8.995125910` | — |
| net distance / accumulated delta | `5.894055679` | `2.949589953` | `0.500434695` |
| animation progress / accumulated delta | `0.994474845` | `0.989899073` | `0.995398806` |

The locomotion threshold is exceeded by approximately 49.96%, while animation
remains inside the 1% relative threshold.

## First localization

- all 495 measured target-165 updates had valid input, position, and animation;
- animation ID remained `300020210` at both conditions;
- final transform displacement occurred between every successive broad-update
  sample at both conditions: approximately 60 moving steps/s at target 60 and
  165 moving steps/s at target 165;
- mean per-update step fell from approximately `0.09988` at target 60 to
  `0.01821` at target 165. The update cadence therefore increased as requested,
  but displacement amplitude was about half of the delta-scaled expectation;
- stamina remained nonzero (`86 -> 41`) throughout the 165 measurement, so the
  captured half-distance is not explained by measured stamina exhaustion;
- position was unchanged across the entry/return boundary of `00D57900` at both
  conditions. Movement is applied elsewhere between consecutive samples;
- root motion and requested/pre-collision displacement remain uninstrumented.

## Safety and provenance

- condition: `pass-c-locomotion-sprint-165-azerty-trial-set`;
- display before launch: 2560x1440, Windows-reported 279 Hz;
- central state restored exactly `165/1 -> 30/0`;
- no game-directory file was written;
- raw session: `sessions/20260919T130701Z-pid2540/`;
- derived analysis: `analysis/20260919T130701Z-pid2540-sprint-165-control/`;
- direct comparison: `comparison/sprint-60-vs-165-control/`.

## Gate

Do not correct the game and do not authorize 240. The next step is targeted
causal localization between animation output, root/requested displacement,
character movement application, and collision/physics integration. Roll remains
separately unmeasured.
