# Producer-state discriminator result

## Accepted pair

Session `20260921T065209Z-pid15448` completed the announced same-process pair
with exact hook/timing restoration before DS3 was closed:

- target 60: 181/181 valid rows, 60.002048 sampled calls/s, normal behavior;
- target 240: 719/719 valid rows, 239.597162 sampled calls/s, familiar visible
  hopping reported by Thomas.

Both concrete targets stayed stable. In every one of the 900 rows, carried
`+0x250` is bit-identical to stateful producer `005E6580`'s owner `+0x2DC`.
The contribution reconstructed from actor getter `008BC9A0` is exactly zero.
Producer mode is `1` throughout, retention is the constant float32 `0.6`, and
there are no reset, decay or `+/-0.5` clamp transitions in the captured route.

The state's opposing vertical distance per planar route unit rises from
`0.189482` at 60 to `0.477158` at 239.597, a `2.518230x` amplification. This
locates the reproduced symptom in the active additive state recurrence rather
than the actor contribution or the alternate decay branch.

## Time law

The exact-build operation is `state <- state + 0.2 * response` once per update.
Its caller supplies the live elapsed-time argument. Converting the discrete
native-60 increment to a time integral gives:

`state' = state + (native_state' - state) * dt / dt60`

where `dt60` is the exact native float32 delta `0.0166666005`. The inactive
multiplicative branch has the corresponding continuous law:

`state' = state * retention^(dt / dt60)`

These laws contain no requested-FPS lookup, terrain constant or empirical
offset. At the native reference delta the prototype performs no write and
returns the original result bit-for-bit.

Offline replay of the captured high-rate increments reduces the high/60
opposing-density ratio from `2.518230` to `1.036901`. This is supporting model
evidence only because the routes differ in length and future response may
depend on the corrected prior state. Live causal and visual validation remains
mandatory.
