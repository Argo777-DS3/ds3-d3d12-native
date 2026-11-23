# I-0009 visible locomotion prototype

This prototype packages the accepted I-0007 comparison-only correction for one
bounded 165-FPS offline demonstration. It starts in `STANDBY`: injection alone
does not change timing or movement. `prepare` applies the reversible 165-FPS
override and observes the native path with correction OFF; `enable` changes only
the accepted low-XMM0 comparison operand; `disable` returns to observation-only;
`stop` disables the hooks and restores timing/debug state.

The correction is unchanged:

`normalized = raw * ((1/60) / previous_broad_delta)`

It retains exact opcode, player/owner, consecutive update, transform/position
and finite-delta guards. A mismatch fails closed and requests shutdown. Telemetry
is bounded counters plus control-transition snapshots, not an unbounded trace.

I-0008 is reused as an independent safety/control input: each active broad update
checks its delta against central timing `+0x268`, and the qualified player
animation clock `modules+0x10 -> +0x24` is counted as an observation-only OFF/ON
sentinel. The prototype does not patch the animation path.

## Build and offline validation

Run `build.ps1`, then `node validate_exact_build.mjs` from the repository root.
`BUILD-VALIDATION.json`, `EXACT-BUILD-VALIDATION.json` and
`native-test-output.txt` bind the source, accepted upstream source, exact game
bytes and binaries. The release DLL/controller are in `bin/`.

## Safety boundary

- Exact Steam BuildID 10167187 / executable 1.15.2.0 only.
- 165 FPS only; no 240/unlimited mode.
- Offline play only; no automatic launch or injection.
- No game-file modification.
- No action-specific gain, output compensation or alternate causal hook.
- Loaded DLL remains inert after a successful stop until the game exits.

The operator sequence is in [LIVE-PROTOCOL.md](LIVE-PROTOCOL.md). Do not run it
without the required advance announcement.
