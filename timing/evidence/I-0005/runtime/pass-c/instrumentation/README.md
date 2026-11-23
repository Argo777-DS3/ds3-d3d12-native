# I-0005 Pass C semantic-invariant probe

This probe is a separate extension of the audited Pass B instrument. It preserves the same reversible central `+0x354/+0x358` override and adds exact-build sampling of the local player position, facing angle, current animation identifier, animation clock/duration, play-speed field, HP, stamina, keyboard state, and XInput state.

The player pointer and field chains are secondary-source leads until runtime validation. The `WorldChrMan` singleton global itself is independently re-identified in the C1 executable at RVA `0x0477FDB8` by `verify_worldchrman.py`.

No file in the Dark Souls III directory is written or replaced. Captures are emitted under `../sessions/`.

## Marker hotkeys

Press the same key again to end a segment. Starting another segment automatically ends the previous one.

| Key | Segment |
|---|---|
| F5 | idle reference |
| F6 | continuous sprint/movement |
| F7 | roll |
| F8 | representative animation |
| F9 | collision/obstacle |
| F10 | camera rotation |
| F11 | physics/movement integration |
| F12 | end current segment |

`markers.csv` records exact QPC/update/delta and semantic endpoints. `semantic-state.csv` contains per-sample states and input observations. The default checked-in configuration is disabled so an accidental injection cannot alter cadence.

`probe-overhead.csv`, when present, directly times the semantic sampling window
(player/input reads, log lock, and semantic CSV write) in QPC ticks and reports
sample count, mean, and maximum microseconds.

## Deterministic locomotion trial

The optional locomotion mode is disabled in the checked-in configuration. When
explicitly enabled, `F4` arms one sprint trial. On the operator's AZERTY layout,
the operator then holds keyboard `Z+Space` with the controller neutral. The
probe requires one accumulated second
of stable digital input, opens a three-accumulated-second measurement window on
the next broad update, and closes it automatically. Releasing or contaminating
the input during measurement aborts that trial.

`locomotion-updates.csv` records pre/post broad-update player position and
facing, animation ID/clock/duration/play speed, input state, QPC, update index,
delta, and accumulated delta. `locomotion-events.csv` records arm, warmup,
measurement, completion, and abort boundaries. This observes the broad-update
boundary; it does not by itself identify root motion, movement ownership, or the
collision/physics stage.

For causal localization, `locomotion_phase_rca=1` additionally installs
observation-only exact-build hooks at `00D56780`, `00D56930`, `00D56A90`,
`00D58E20`, `00D5B140`, `00E6E750`, and `02344730`. The last target is a
generic scheduler/dispatcher call observed from multiple caller sites, so every
phase row also records the return-address RVA. `00D4AC90` is deliberately not
hooked: static evidence shows that it is a single-instruction `RET`, and MinHook
rejects that no-op function as unsupported.

`locomotion-phases.csv` records paired world-transform and animation snapshots
at those boundaries only while a locomotion trial is active. The analyzer
derives per-function displacement pairs and consecutive-boundary transition
summaries. These hooks do not alter inputs, motion vectors, collision state, or
timing fields. The current boundary result is documented in
`../PASS-C-SPRINT-DISPATCHER-RCA-RESULT.md`.

The follow-up `locomotion_write_watch=1` mode is a target-60 discovery mode and
requires the locomotion-trial, phase-RCA, and timing-override modes. It arms
three per-thread, four-byte hardware write watchpoints on the qualified player
transform coordinates only after `02344730` returns to caller `00A1429C`, and
restores the original debug registers at the matching `00DE50EC` entry. The
vectored exception handler records post-instruction RIP, integer/SIMD context,
arm/hit position, and bounded live bytes in `locomotion-write-watch.csv`.
Configuration remains disabled by default. Its ordered gate and limits are in
`../PASS-C-SPRINT-INTERNAL-STAGE-DESIGN.md`.

Primary acquisition uses three repeated target-60 trials with the established
reversible central `60/1` condition, from the same fixed
scene anchor before any 90/120/165 comparison. Free manual routes are not a
valid replacement for this reproducibility gate.

## Build

Run `powershell -NoProfile -ExecutionPolicy Bypass -File build.ps1`. The build
pins the same MinHook commit and MSVC environment as Pass B. `-DllName <name.dll>`
may be used for a distinct probe binary when an earlier DLL remains loaded in a
live process; the default remains `I0005PassC.dll`.

Run `powershell -NoProfile -ExecutionPolicy Bypass -File test_write_watch.ps1`
to compile and execute the standalone debug-register controller test. This test
does not load DS3 or prove a game-runtime writer; it verifies write-only hits and
exact restoration in a synthetic process.
