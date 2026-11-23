# I-0005 Pass C — 165 FPS locomotion divergence RCA

## Trigger observation

After the valid `20260919T121245Z-pid6348` 165-FPS capture, Thomas reported a
reproducible gameplay anomaly at real 165 FPS: animation appeared to retain
normal speed while the distance travelled by the character during sprint and
roll actions was incorrect.

The sprint component is now `CONFIRMED` by deterministic measurement. The first
direct comparison produced `0.500435` of the accepted target-60 distance per
accumulated delta while animation progression retained a `0.995399` ratio with
the same animation ID. A later matched phase-localization pair reproduced the
result at `0.501323` movement and `0.995399` animation ratios and bounded every
sampled final-transform displacement to one protected runtime transition. The
internal requested/root-motion versus collision/application stage remains
unresolved. The roll component remains `OBSERVED`. Both keep the 240-FPS gate
closed.

The existing human-repeated sprint, roll, attack, obstacle, camera, and movement
segments are insufficient to refute this observation because their path,
heading, duration, and action mix vary across conditions.

## Question

Under identical and reproducible sprint and roll stimuli, at
which boundary does the first framerate-dependent divergence appear?

```text
animation progression
  -> root-motion or animation displacement
  -> requested/applied character displacement
  -> collision/physics integration
  -> final world transform
```

## Required deterministic sentinel

For every condition, record at minimum:

- exact starting and ending world position and facing;
- net distance and path length;
- wall-clock duration and QPC endpoints;
- broad-update count and accumulated broad delta;
- animation state/ID, clock, duration, and progression;
- per-update world-position displacement before and after the broad update;
- input-state onset/hold/release boundaries tied to update indices;
- any defensible root-motion or requested/applied displacement vector found
  before and after collision/physics.

## Acquisition design

Do not use a free manual route as the primary comparison. Build the sentinel in
two layers:

1. Instrument the stable player/transform and animation chains at the entry and
   return of the broad update, deriving per-update displacement and animation
   progress from one clock domain.
2. Use a fixed scene anchor, fixed facing, action-specific digital locomotion
   state (sprint and roll must be evaluated separately), and a
   probe-timed steady-state window. Exclude onset and release transients from the
   primary distance-per-accumulated-delta comparison. Prefer an operator-held
   digital input whose measurement window is opened and closed by the probe only
   after stable input is observed; escalate to controlled input substitution only
   if repeated anchored trials fail the reproducibility gate.

The initial reproducibility gate is three same-condition trials under the
established reversible central target-60 condition (`60/1`).
All input/position/animation samples must be valid, the animation-ID sequence
must match, start-position spread must be at most 0.05 unit, start-facing spread
at most 0.02 radian, and coefficients of variation for net distance, path length,
and distance per accumulated delta at most 1%. Cross-framerate acquisition may
begin only after this gate passes. A locomotion divergence is discriminating at
more than 2% from the target-60 distance-per-delta baseline; animation progression
uses a 1% relative threshold.

## Static/runtime localization

Targeted analysis may add hooks only where evidence supports their ordering
relative to animation, movement, or collision. Candidate vectors must first be
identified by current-build data flow or correlated runtime change; unknown
fields remain hypotheses. Logging a pre/post broad-update world position is an
observation boundary, not proof that `000499C0` itself owns locomotion.

The accepted direct sprint comparison is target 60 versus target 165 under
D-0006. Position changed on every interval in both conditions, excluding skipped
movement updates as the primary mechanism. It did not change between entry and
return of `00D57900`, so that hook lies before the observed final-transform
application boundary.

Static call ordering showed that `DS3-1152-F-00D5B140` invokes `00D57900`, then
conditionally invokes `00E6E750` and performs a final bounded tail.
`00D4AC90` is a single `RET` in this exact build, so it cannot own movement and
was removed from the live hook set after MinHook rejected the no-op function.

The completed phase expansion bracketed `00D56780`, `00D56930`, `00D56A90`,
`00D58E20`, `00D5B140`, `00E6E750`, and generic scheduler/dispatcher target
`02344730`. At both target 60 and 165, no hooked function changed player
position between its own entry and return. All non-zero sampled displacement
occurred between two consecutive `02344730` calls:

```text
02344730:after, caller return RVA 00A1429C
  -> protected/runtime continuation
02344730:before, caller return RVA 00DE50EC
```

The transition carried 180/180 moving intervals at target 60 and 494/494 at
target 165. The matched comparison retained only `0.501323` of target-60 net
distance per accumulated delta while animation progression retained `0.995399`.
This is a final-world-transform application boundary, not yet proof of a
specific root-motion, requested displacement, collision, physics, or transform
commit owner. See `PASS-C-SPRINT-DISPATCHER-RCA-RESULT.md`.

## Gate

- Do not correct any consumer or timing field.
- Do not authorize 240.
- The sprint divergence is reproduced and localized to the protected transition
  above, but its internal motion/collision stage must be identified before any
  correction or reopening of the ordered high-FPS sequence.
