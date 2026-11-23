# I-0013 base-transform boundary observer

This is the one-hook replacement designed after the reproduced cycle-14 cache
divergence. It detours exact-build `009D97B0`, then retains only calls from the
single `005C8100` callsite (`return RVA 005C812C`) whose `RCX` equals the current
player's `modules+0x68` movement object.

For each retained invocation it records movement position immediately before
and after the original call, the 48-byte base transform returned through `RDX`,
and the prior 48-byte cache contents. The prior cache is explicitly a temporal
reference, not the final output of the same `005C8100` invocation. The capture
uses QPC wall time because this hook has no elapsed-time argument.

The observer calls the original `009D97B0` exactly once and does not call
`00D99C40`, protected `00D9A3C0`, any cache writer or any behavior-proxy
exchange. It uses no debug register, vectored exception handler or thread
context write. Its only engine-state write is the already accepted reversible
central cadence pair. It starts at target 60, remains inert until F4, limits a
capture to 2,048 retained calls / three wall-clock seconds, fails closed at
five seconds and restores the original timing pair exactly on stop.

The discriminator is binary: opposing motion in the returned base translation
routes the RCA upstream or to intra-update movement state; a monotone base with
an oscillating prior/final cache routes it to the later protected/local/cache
path. Neither outcome alone assigns collision or terrain ownership.

## Executed result

Session `20260921T054031Z-pid27432` completed the bounded 60/240 protocol with
exact restoration and shutdown. All active returned base translations were
bit-exact and monotone; the prior cache reproduced the high-rate oscillation.
See [`BASE-TRANSFORM-DISCRIMINATOR-RESULT.md`](../BASE-TRANSFORM-DISCRIMINATOR-RESULT.md).
