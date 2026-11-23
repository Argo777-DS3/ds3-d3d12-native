# I-0013 producer time-law prototype

This exact-build, disabled-by-default prototype hooks only the resolved
current-player producer `005E6580`, calls it exactly once, and converts its two
discrete native-60 recurrence branches to elapsed-time form:

- additive response: `s' = s + (native_s' - s) * dt / dt60`;
- multiplicative decay: `s' = s * retention^(dt / dt60)`.

Here `dt60` is the exact float32 native delta observed in every accepted 60-Hz
row. When `dt == dt60`, the hook performs no state write and returns the native
result bit-for-bit. Reset and saturated `+/-0.5` results pass through unchanged.
The intervention is independent of requested FPS and terrain; it uses the live
elapsed-time argument supplied by the producer caller.

The prototype starts with correction OFF, is current-player identity guarded,
records bounded three-second captures, and restores timing and its sole hook on
stop. It is an experimental live validation tool, not a distributable patch.
