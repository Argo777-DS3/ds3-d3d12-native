# I-0005 hypotheses — after runtime Pass B

## H1 — presentation pacing is the engine timestep

**Disposition: refuted as an established connection.** I-0004's QPC deadline reaches Present. I-0005 found a separate central timing object and a separate broadly distributed update scalar; no static data flow connects the I-0004 state to the broad update path.

This does not prove runtime independence; it means the equality cannot be assumed.

## H2 — `context+0x08` is a major engine-update delta

**Disposition: confirmed as a live broadly distributed update delta.** Runtime Pass A observed 14,222 exact forwarding triples at approximately 59.48 calls/s. Pass B changed it causally to approximately `1/90` and `1/120` while preserving exact propagation. This does not make it a universal gameplay timestep or prove all consumer invariants.

## H3 — central object `0x14489DD10` produces multiple time domains

**Disposition: supported.** It owns QPC phase stamps, multiple current/history floats, mode flags, and the causally validated FPS override pair. Pass B preserved the strong immediately preceding `+0x268` relationship at 60/90/120, but protected producer arithmetic and exact original-producer identity remain unresolved.

## H4 — `00DD4B60` is the global fixed-timestep switch

**Disposition: refuted.** It does choose dynamic/`1/30`/`1/60`, but its output belongs to a distinct specialized object and no flow to the broad update distributor is observed. Runtime Pass A selected only the dynamic entry on the one observed object.

## H5 — one blanket `60 -> 120` edit can establish framerate independence

**Disposition: refuted as a defensible strategy.** The binary contains at least presentation pacing, a protected central controller, a broad update scalar, a specialized fixed/dynamic selector, and local resource/timeline `1/60` uses. Their contracts differ.

## H6 — static analysis alone can identify animation, movement, physics, and camera invariants

**Disposition: partially resolved, high-level identities still not established.** Pass B corrected the `000EFDA0` calling-convention reading: both live virtual targets forward the delta to `002124A0`. Active `000EFDA0` helper state and `0021CC20` elapsed state advance by delta at 60/90/120. Neither is given an animation, movement, physics, or camera name without additional evidence.

## H7 — the historical `+0x354/+0x358` patch is a minimal intervention boundary

**Disposition: confirmed as the minimal cadence-control boundary.** Reversible exact-build tests showed that the pair changes update cadence, distributed delta, and presentation interval selection at 60/90/120, with exact restoration. It is not yet confirmed as a complete framerate-independence patch for all gameplay consumers.

## H8 — Present cadence causes the observed broad delta cadence

**Disposition: refuted as the only supported causal model.** A common upstream intervention at `+0x354/+0x358` changed both broad-update and Present cadence together at 60/90/120 while the manual presentation wait stayed inactive. Pass B supports a shared central control, not evidence that Present itself produces the broad delta.

## H9 — active bounded elapsed-state consumers integrate a fixed amount per update

**Disposition: refuted for the observed consumers.** Active `002124A0` state under `000EFDA0` and active `0021CC20 +0x4F0` advanced by the supplied variable delta at 60/90/120. Their call counts follow updates; their observed elapsed integration follows delta.

## H10 — historical `WorldChrMan` field chains expose current-build player and animation sentinels

**Disposition: partially supported at the singleton boundary; downstream semantics remain hypothesis.** Exact-build static bytes independently resolve the candidate singleton global to RVA `0x0477FDB8`. The proposed position, facing, animation identity, animation clock/duration, play-speed, HP, and stamina chains come from a version-matched external table and have not yet been behaviorally validated in the current process. Pass C must confirm or reject each field at 60 FPS before cross-framerate use.

## H11 — the approximately half-rate final offset is introduced by one input to the final writer

**Disposition: statically factored, dynamically unresolved.** Exact-build data
flow proves `X = T + s * ((Cxyz,1) - T)` as sequential packed float32
operations, with `C` selected from priority `+0xB0` or fallback `+0xA8` and
`s` read from one-shot field `+0x1C0`. A single neutral observation at RVA
`009DC559` can capture all factors in the same invocation. No 60/165 mirror has
yet measured whether source selection, inhibition, candidate offset, scalar,
or more than one factor explains the observed ratio.
