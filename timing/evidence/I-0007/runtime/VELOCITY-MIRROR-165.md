# Velocity source: reduced before rotation

## Accepted observation

Session [20260919T174451Z-pid23064](sessions/20260919T174451Z-pid23064/)
used the previously announced target-165 velocity mirror, with the exact game,
DLL and injector hashes checked before injection. Thomas supplied the sprint.
The [analysis](analysis/20260919T174451Z-pid23064/velocity-summary.json) and
[reproducer](analyze_velocity_mirror.mjs) preserve the raw identities.

- 494 complete ordered windows, each containing one source write followed by
  the three execution observations; one selected owner and thread, no invalid
  reads, overflow or incomplete window.
- 495 measurement samples, all valid input/position/animation; animation ID
  `300020210`. Update/Present rates 164.379003 / 164.155287 Hz, median delta
  `0.00606059982`. Facing change 0.048232 rad: localization trial, not a new
  matched acceptance baseline or terrain-independent speed estimate.
- Stop reported hooks disabled and debug registers restored. Timing returned
  exactly `165/1 -> 30/0`; all I-0007 configurations were disabled. DS3 was
  subsequently observed closed. No causal patch or game-file change.

Raw mirror SHA-256:
`EA8C4236768C78B1FB3836B95FF43F9C723F282EEAA56327A36701C748B66D5C`.
Other raw hashes and cadence are in the same analysis directory.

## Calculation established

The input source vector norm per accumulated time is `2.9623333615`, already
approximately half the previous target-60 returned-vector reference
`5.927144635`. Its rotated norm rate is `2.9623333586`.

All 494 rotation XYZ outputs match the reconstructed `012BCFB0` polynomial
bit-exactly. Quaternion norm lies in `[0.9999999616, 1.0000000340]`; the local
rotated vector is unchanged through the subsequent optional-replacement stage
on every call. Every final horizontal lane is exactly float32
`rotated_lane * inverse_delta`. The additional vertical component means the
full final XYZ norm rate is slightly different (`2.9629319940`); do not claim
all four final lanes equal that simple multiplication.

The same flag set holds on all 494 calls: `+A2=1`, `+1DF=1`; `+D0`, `+160`,
`+1D8`, `+1DA`, `+1DB`, `+1DC`, `+1DD` are zero. `+130=1`. Thus yaw orientation
is selected, the replacement/clamp and alternate-vector selection branches
are inactive, and the source is already reduced before rotation/conversion.
This is an observed amplitude carrier, not the first causal formula.

## Active source writer and next bounded reconstruction

The source watch fires once per window at post-RIP `009DB9CC`, after
`009DB9C5 MOVAPS [RCX+E0],XMM0` in `009DB980`. The preceding instruction loads
`[R8]`. All 494 captured XMM0 vectors match the written field and subsequent
rotation input exactly; RCX identifies the selected owner and `R8=RBP`.
The captured code bytes contain that exact load/store pair.

[Exact-build exports](../static/source-writer/) show the sole direct code
reference to `009DB980` at `005CA9D0`, inside `005CA570`. Its `RBP` is the
different source object's `+1D0` field. This caller relation is statically
established and consistent with the live registers; its return address was
not captured, so this is not a separately observed call stack.

The bounded source preparation is:

1. `005CA642`: virtual call through source-object vtable `+D8`, output `+1D0`.
2. `005CA659`: `009C8E80` receives the same output pointer.
3. Conditional `005CA707` multiplies the vector by source-object `+25C`, when
   three gates are false and the scalar is below one; a norm/time diagnostic
   is stored to `+260`. Later conditional zeroing and calls remain possible.
4. `005CA9D0` copies the resulting vector into selected owner `+E0`.

The raw export of `009C8E80` has invalid/opaque instructions and cannot supply
its true calculation. `005CB7F0` was inspected because it is a later call on
the source object; no direct `+1D0` write is visible in its exported body,
but its calls are not thereby proved irrelevant. The live target of virtual
`+D8`, the stage values and `+25C` are absent from previous captures. Stale
volatile XMM contents at the setter do not prove an earlier branch or factor.

## Next discriminating evidence

Use the [builder mirror](BUILDER-PROTOCOL.md) once at 165 to capture before and
after the virtual producer, after `009C8E80`, and before the final copy. It
also retains the active target, bounded source-object operands and bounded
live code windows for both directly implicated producers. This separates
three preparation stages in one run and avoids automatic 60/165 reacquisition.
If the virtual output is already reduced, trace that actual target rather
than changing the downstream scalar. No promise that this is the final RCA
capture; sprint 60/165 and same-patch roll acceptance are still mandatory.
