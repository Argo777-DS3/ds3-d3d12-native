# I-0013 proxy/terrain boundary assessment

## Result

The first matrix does not show a new large terrain-independent oscillation in
the already sampled final player root. After resampling by normalized planar
route progress, detrended vertical-profile correlations are 0.997943 for
60-off versus 240-off and 0.996396 for 60-on versus 240-on. The correction
pair correlations are 0.990099 at 240 and 0.998298 at 60. Most opposing
vertical events recur at comparable route positions. This leaves modest
rate-dependent differences visible in the samples, but the operator-visible
hopping is not represented as a large new final-root trajectory.

This selects the physics-backed player-proxy transform versus final-root
boundary as the smallest next discriminator. It does not yet assign the defect
to collision, physics, animation or rendering.

## Exact-build dataflow

Read-only Ghidra exports from the exact 1.15.2.0 project establish the following
bounded chain:

1. `0088AFC0(PlayerIns, out48)` reads `PlayerIns+0x50`, then copies its
   `+0x120..+0x14F` 48-byte block.
2. `023569E0 -> 02356A00` rearranges that 48-byte block into a matrix-like
   representation. This is a transform conversion, not evidence of collision
   semantics.
3. `00DD3850` and `00DD3B50`, already identified as slots 6/7 of
   `SprjBehaviorCharacterProxyDriver`, consume that current transform on the
   forward/reverse exchange.
4. `009A3CF0` independently obtains the same transform, converts it and copies
   48 bytes into a downstream object. This confirms that the getter is an
   active bridge rather than an analysis-only convenience.
5. A neighboring movement-update chain reached from `005C7160` uses a distinct
   `modules+0x68` object. Its `009DC090 -> 009DEE60` path supplies both elapsed
   time and reciprocal elapsed time, reads/writes a wrapper's `+0xA0` float4
   through `00DDA080/00DDA750`, and stores a separate `modules+0x68+0x120`
   vector. This is not the `PlayerIns+0x50+0x120` source copied by `0088AFC0`.

The similar offsets initially made those two chains look identical; exact
caller/object reconstruction separates them. The true writer of the 48-byte
`PlayerIns+0x50+0x120` block is the `005C6900/005C6F10/005CAA20` family. Each
calls `005C8100` and copies its 48-byte result to the cache. `005C8100` obtains
the base transform through `009D97B0` from `modules+0x68` current fields
`+0x80..+0x8C`, passes it through `00D99C40` and protected `00D9A3C0`, then
applies optional local transformations. The `+0x3CC` term conditionally added
to the second position lane has only one bounded writer in the `009D0000`
region: constructor `009D8340` loads it from resource data. It is not a
demonstrated per-update terrain correction.

The neighboring vector chain still proves that this update region contains
explicit `dt` and `1/dt` arithmetic, but it neither identifies nor clears the
terrain defect. The dynamic discriminator remains the phase relationship among
the current `modules+0x68` transform, its cached proxy copy and the final root.

## Retrospective runtime lead

The accepted I-0007 target-60 capture supplies 360 complete stable-player
exchanges. Consecutive changes in proxy lane 1 and player-root vertical position
have correlation 0.773087. In the rejected overflowed target-165 prefix, 192
complete stable-player rows show proxy lane-1 steps from -0.014206 to +0.012970
while the sampled player root is nearly static (minimum -0.0000077, maximum
zero; correlation -0.024120).

That target-165 session remains rejected as a whole and used a different route.
Its prefix is only a lead that justifies observing this boundary; it does not
prove a high-rate defect, lane semantics, collision ownership or causality.

## Reused I-0008 temporal control

I-0008 is retained as an independent downstream control, not reacquired. Its
accepted target-165 observation recorded 495 complete player
generator/control/ring chains. In all 495, the generator and control receive
the variable central delta bit-for-bit, the native mode-1 formula reproduces
the ring value bit-for-bit, and the argument differs from fixed `1/60`.

Its wider retained audit contains 7,371 stable ten-update intervals across
60/90/120/165. At 165, 3,429/3,476 match the preceding ten measured deltas
within `1e-5`; each of the 47 residuals is closer to the measured variable-delta
sum than to `10 * (1/60)`. Those residuals include movement, sprint, roll,
collision and idle marker regions but lack the live mode identity needed for a
more specific claim.

Therefore a hidden fixed-60 substitution in the already qualified player
animation clock is refuted for its captured path and is not the leading
explanation for I-0013. This does not prove all animation modes correct, nor
does it prove the terrain defect is physical. It lets the next capture use
animation as a time-correct downstream witness while discriminating the
physics-backed proxy from the final root.

## Next discriminator

The prepared I-0013 observer retains only the current PlayerIns-backed
`SprjBehaviorCharacterProxyDriver` and records the inherited four sites:

- current `modules+0x68` position, conditional lane-1 term and selector state
  at every site;
- forward selected 48-byte block;
- forward derived 48-byte block;
- reverse selected block;
- reverse current player-proxy 48-byte block;
- instantaneous final player root and elapsed-time context.

One three-second native trial at 60 and one at 240 on the same slope/stair route
can distinguish:

- divergence already present at the movement source or selected cache;
- divergence introduced between forward and reverse exchange;
- a rate-comparable proxy with hopping appearing only downstream;
- or insufficient signal, requiring an earlier physics/contact boundary.

The observer is read-only. It uses no correction and no inferred vertical
offset. Enemy owners are discarded before the row counter advances, keeping
each trial below the inherited 4,096-row safety guard.

## Fix acceptance rule

An acceptable eventual correction must expose and repair a general mathematical
invariant. Candidate forms include a displacement/velocity conversion using the
actual `dt`, or a discrete recurrence converted to a time-continuous law. It
must not contain a 240-specific multiplier, hand-tuned vertical compensation,
terrain-specific threshold or post-hoc visual clamp.

Before acceptance, the chosen formula must preserve native 60 behavior, behave
continuously across intermediate and high rates, and pass the same terrain
route with correction off/on while retaining exact cleanup. This document does
not propose or authorize a patch.
