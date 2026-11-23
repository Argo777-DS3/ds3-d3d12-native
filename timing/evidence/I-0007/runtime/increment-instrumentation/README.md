# Increment producer mirror

Isolated derivation of the completed candidate watch, keeping its source,
binary and evidence unchanged. This is observation-only, not a causal patch.
See [the dedicated protocol](../INCREMENT-PROTOCOL.md) for exact addresses,
discriminating hypotheses, restoration and operator steps.

Build with `build.ps1`; run `test_write_watch.ps1` for the shared mixed
execute/write DR7 configuration and exact Windows debug-state restoration.
The disabled config deliberately refuses live initialization instead of
falling back to the inherited broad exploration hook mode.
