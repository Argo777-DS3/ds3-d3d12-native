# I-0013 producer-state observer

This exact-build observer retains the qualified single hook at `005C8100` and
adds ordinary memory reads for the state maintained by resolved producer
`005E6580`: owner `+0x2DC`, retention `+0x378`, mode byte `+0x374`, and the
central observed delta `+0x268`. It also retains carried `+0x250` and both
concrete target RVAs.

The observer invokes no producer or protected operation and applies no
correction. Its analyzer classifies exact adjacent-state relations and reports
the residual first contribution reconstructed from `carried - producer_state`.
That residual is explicitly an algebraic bound, not a direct virtual-call
return measurement.

A matched 60/240 route pair is required because the target-60 provenance run
resolved identity but cannot distinguish which contribution carries the
cadence-dependent oscillation or which branch of the state recurrence is live.
