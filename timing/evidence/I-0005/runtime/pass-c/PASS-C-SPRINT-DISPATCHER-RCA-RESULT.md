# I-0005 Pass C — sprint world-transform boundary RCA

## Outcome

The matched target-60 and target-165 dispatcher captures localize all sampled
sprint world-transform displacement to the same protected runtime transition:

```text
02344730 after, caller return RVA 00A1429C
  -> protected/runtime continuation
02344730 before, caller return RVA 00DE50EC
```

No instrumented function changed player position between its own entry and
return. Across consecutive observations, the transition above carried 100% of
the non-zero sampled displacement in both conditions: 180/180 target-60
intervals and 494/494 target-165 intervals.

This places the measured amplitude loss after normal animation progression and
inside the bounded runtime continuation that makes the final player world
transform visible. It does not yet distinguish requested/root displacement from
collision/physics integration or final transform commit.

## Matched comparison

| Metric | Target 60 | Target 165 | 165 / 60 |
|---|---:|---:|---:|
| broad updates | `181` | `495` | — |
| accumulated delta | `3.016656784 s` | `3.000023309 s` | — |
| net distance | `17.796794373` | `8.872739823` | — |
| sampled path length | `18.062578023` | `8.996009415` | — |
| net distance / accumulated delta | `5.899509174` | `2.957556962` | `0.501322547` |
| animation progress / accumulated delta | `0.994474878` | `0.989899156` | `0.995398856` |

Within the localized transition, mean displacement per moving interval was
`0.100347656` at target 60 and `0.018210545` at target 165. The different
per-update values are expected to depend on delta; the discriminating result is
that displacement per accumulated delta is still only `0.501323` of the
target-60 reference while animation progression per delta is `0.995399`.

## Negative localization evidence

- `00D57900`, `00D5B140`, `00E6E750`, `00D56780`, `00D56930`, `00D56A90`,
  `00D58E20`, and `02344730` showed zero position displacement between their
  own observed entry and return boundaries.
- `00D4AC90` is a one-instruction `RET` in this build. MinHook correctly
  rejected it as unsupported; it is a no-op and is not a locomotion owner.
- The preceding outer-phase capture placed all target-60 displacement between
  `00D56A90:after` and `00D5B140:before`, which motivated the dispatcher
  expansion.
- Static reconstruction identifies `00DE50EC` as the return after a direct
  `02344730` call inside the `00DE5040` dispatcher. The runtime code around
  `00A1429C` is not defensibly reconstructible from the current static image;
  no semantic name is assigned to that side of the boundary.

## Provenance and safety

- target-60 raw session:
  `sessions/20260919T140436Z-pid29496/`;
- target-60 derived analysis:
  `analysis/20260919T140436Z-pid29496-locomotion-dispatcher-60/`;
- target-165 raw session:
  `sessions/20260919T141032Z-pid26664/`;
- target-165 derived analysis:
  `analysis/20260919T141032Z-pid26664-locomotion-dispatcher-165/`;
- comparison:
  `comparison/sprint-dispatcher-60-vs-165/locomotion-comparison.json`;
- static boundary evidence:
  `../../static/locomotion-dispatcher-boundary/` and
  `../../static/locomotion-outer-phase/`.

Both runs restored the exact central state to `30.0/0`. No game-directory file
was modified. The checked-in probe configuration is disabled.

## Gate and next question

Sprint divergence is reproduced and bounded to the protected transition, but
its internal stage remains unresolved. Do not correct the game and do not
authorize 240. The next investigation should identify and sample the requested
or root-motion vector and the pre/post collision/applied displacement on the two
sides of this boundary. Roll remains a separate `OBSERVED`, unmeasured action.
