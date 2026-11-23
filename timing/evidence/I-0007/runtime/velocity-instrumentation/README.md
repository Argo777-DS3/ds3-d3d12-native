# Velocity source mirror

Observation-only isolated successor to the accepted increment mirror. Source
selection, exact memory layout, bounds, hypotheses and operator procedure live
in [the protocol](../VELOCITY-PROTOCOL.md).

Build using `build.ps1`; run `test_write_watch.ps1` to validate all four hardware
slots, execute-trap resumption and exact Windows debug-state restoration.
The checkpoint-disabled config refuses live initialization. No causal patch.
