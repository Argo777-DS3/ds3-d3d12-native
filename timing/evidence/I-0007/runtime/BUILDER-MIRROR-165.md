# Source builder: the amplitude loss is created by local scaling

## Runtime evidence

[Session 20260919T175749Z-pid20840](sessions/20260919T175749Z-pid20840/)
passed the four-site pairing and validity gates: 494 windows / 1976 hits,
one source object/thread, 495 valid sprint measurement rows, animation
`300020210`. Cadence was 164.417849 updates/s and 164.206065 Presents/s.
Facing changed 0.048687 rad; this is localization, not final patch acceptance.
Matching injector stop succeeded, COMPLETE reports debug-state restoration,
and exact `165/1 -> 30/0` restoration is recorded. Configuration disabled.

The active virtual target is `005E57F0`. Its output norm per accumulated delta
is `5.9241563043`; `009C8E80` leaves all four lanes unchanged on all 494 calls.
The final source norm rate is `2.9620781521`: exactly half. Every final vector
is bit-exact float32 `modified_vector * source_object[+25C]`, with `+25C=0.5`
at all four sites on all 494 windows. The existing 60 returned-vector reference
is `5.9271446350`: the unscaled 165 output is compatible to about 0.05%.

The creating application is `005CA707 MULPS XMM0,XMM1`, storing the scaled
source at `005CA70A`; scalar load/broadcast is `005CA6ED/005CA6FB/005CA6FF`.
Unlike earlier transmitters, this stage first turns a reference-compatible
vector into the approximately half-amplitude one. Its supplied scalar still
requires causal reconstruction: bypassing the multiply is not authorized as
a substitute for understanding why the scalar is reduced.

Raw mirror SHA-256:
`0F161E5C0CD5A5F7C487018CC5379457A3D27A3A1F85AC2C7C072A925C878775`.
[Analysis and all hashes](analysis/20260919T175749Z-pid20840/builder-summary.json),
[stage values](analysis/20260919T175749Z-pid20840/builder-stages.json), and
[bounded live code](analysis/20260919T175749Z-pid20840/builder-live-code.json).
The live code windows are stable across 1976 observations each; the modifier
bytes are real runtime code despite its opaque disk export. This is reusable
decompilation evidence, not proof of whole-function/subsystem reconstruction.

## Scalar producer: reconstructed, not yet live-qualified

The exact-offset search is justified by the now-proven `+25C` scalar input;
same-offset results are leads, not common object types. It points to
`005C72B0`, which follows the same builder `+8 -> player+1F90 -> modules+68`
chain and calls the already-traced `009DC090`. See
[selected exports and constants](../static/source-scale/).

After a player virtual gate, and when mode equals 3 or builder `+260 > 5`,
with transform `+A2 != 0`, it obtains the transform through `009D9650` and
computes `d = length(getter_position - transform[+90])`. Getter is transform
`+80`, optionally adding `+3CC` to its middle lane when `+1F0 == +3C8`.
The previously established writer stores the pre-commit position at `+90`;
this uses its known data layout, not a reopened causal-writer hypothesis.

At `005C73D0`, the code multiplies d by **30**, then compares against **1**
at `005C73D8`. There is no delta in this distance test. The update law is:

- If `30*d < 1`: when s > 0.5, s becomes float32 `s*0.8`; otherwise s=0.5.
- Otherwise: when s < 1, s becomes float32 `s*1.2`; otherwise s=1.

These are branch-dependent recurrences, not strict clamps: a multiplication
can transiently overshoot a limit. The stable reduced value is 0.5. Constant
bits are independently exported; names do not assign gameplay semantics.

## Causal interpretation and remaining discriminator

A fixed per-update distance threshold is cadence-dependent: more updates per
second reduce each distance even for unchanged speed. Once selected, scaling
also decreases later displacement, potentially sustaining the reducing branch.
This explains a plausible path to the observed stable half amplitude; the
actual feedback comparison/branch has not yet been observed. Do not promote
this static explanation to a demonstrated complete cause before that check.

The [feedback mirror](FEEDBACK-PROTOCOL.md) observes only the comparison and
the resulting scalar, including actual getter/previous positions, source delta
and exact live code. No repeated virtual/source capture is required. The old
60 reference remains reused. A possible continuous correction is to compare
distance over its actual elapsed interval, calibrated to preserve the 60-FPS
decision. Its time pairing and effect still require proof; no patch is applied.
