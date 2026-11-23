# Native-60-preserving producer time law

Let `h0 = float32(0.0166666005)` be the exact native timestep, `h` the live
elapsed-time argument, `s` the state before `005E6580`, and `sn` the result
produced by the unmodified native call.

For the active additive branch, the native operation is
`sn = s + 0.2 * response`. The response can therefore remain inside the native
function while its accumulated step is converted from per-update to per-time:

`corrected = s + (sn - s) * (h / h0)`

For constant response over an interval, summing this expression produces the
same integral independently of update count. This is the explicit-Euler
timestep conversion appropriate to the recovered additive operation. It is not
an interpolation-gain guess.

For the alternate decay branch `sn = retention * s`, equal decay over equal
wall time requires:

`corrected = s * retention^(h / h0)`

The implementation is guarded as follows:

- only the exact current-player producer identity is eligible;
- correction starts disabled and requires explicit enable;
- finite `0 < h < 0.1` is required;
- exact native `h == h0` returns `sn` without a state write;
- reset zero and saturated `+/-0.5` results pass through unchanged;
- all other additive results are clamped only to the original `[-0.5,+0.5]`
  bounds;
- the original producer is called exactly once and no new engine call is made.

The prototype is a reversible validation instrument. A live 60-ON capture must
prove bit-exact no-write identity, followed by one 240-ON route confirming the
formula and visible outcome.
