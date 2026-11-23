# I-0005 Pass C — target-60 sprint baseline

## Outcome

**PASS with explicit anchoring deviation (D-0005).**

The automatic strict reproducibility gate remains `false`: the three manually
reset Cemetery of Ash starts differed by up to `7.687050451` world units and
`0.06218148` radian, exceeding the declared `0.05` / `0.02` limits. Thomas
explicitly accepted the target-60 sprint series as the bounded comparison
baseline because its normalized action metrics were strongly reproducible.

## Measurement

- condition: `pass-c-locomotion-sprint-60-azerty-trial-set`;
- stimulus: `F4`, then AZERTY `Z+Space` held with neutral controller;
- complete trials: `3/3`;
- valid input, position, and animation samples: `100%` in every trial;
- animation-ID sequence: identical, `[300020210]`;
- accumulated delta mean: `3.016654721 s`;
- net distance mean: `17.780330886`, CV `0.077356%`;
- sampled path length mean: `17.978603194`, CV `0.328245%`;
- net distance per accumulated delta mean: `5.894055679`, CV `0.077357%`;
- animation progress per accumulated delta mean: `0.994474845`;
- pre/post `00D57900` within-update displacement: zero; displacement is visible
  between successive broad-update samples, so this hook does not yet localize
  the owning movement/collision stage;
- timing restoration: exact `60/1 -> 30/0`.

## Scope

This is a bounded target-60 sprint-rate reference, not proof of an exactly
repeated path or collision environment. It does not qualify roll, root motion,
requested displacement, collision/physics ownership, 165 FPS, or 240 FPS.

## Evidence

- raw session: `sessions/20260919T125904Z-pid26776/`;
- derived trials: `analysis/20260919T125904Z-pid26776-sprint-60/`;
- strict comparator output: `comparison/sprint-60-reproducibility/`;
- decision: `decisions/LOG.md`, D-0005.
