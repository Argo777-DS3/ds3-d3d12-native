# I-0013 minimum comparative capture design

## Decision

Extend the validated I-0012 windowed ReShade host at its existing broad-update
hook. Add no engine hook, no render callback and no candidate collision patch.
The capture reads the already qualified player transform after each completed
broad update and stores a bounded in-memory trajectory for asynchronous export.

The transform fields retain the established Pass-C ordering:

- `transform + 0x80`: world `x`;
- `transform + 0x84`: world `z` (vertical);
- `transform + 0x88`: world `y`.

This ordering is a bounded player-position observation surface, not a claim
about the owner of terrain response.

## Record and bounds

Each sample contains QPC, broad-update index, update delta, player and transform
identity, validity, and `x/z/y`. One capture holds at most 65,536 samples
(about 273 seconds at 240 updates/s). The update hook performs only guarded
reads and a fixed-array write. File creation and CSV export occur on the
existing runtime worker, never in the update hook.

Start requires a valid exact-build player/transform chain and the next matrix
cell's already-applied cadence/correction state. Cadence or correction changes
are rejected while recording/exporting. Invalid samples are retained as invalid
rows and counted; identity changes invalidate those rows. Overflow closes the
attempt as failed rather than silently wrapping.

## Matrix state machine

Successful exports advance only in this order:

1. `60-off`;
2. `240-off`;
3. `240-on`;
4. `60-on` (optional low-cost non-regression control).

A failed/overflowed attempt does not advance. Stopping the add-on during an
active capture preserves a partial file but does not advance. Captures are
written beneath `%LOCALAPPDATA%\DS3TerrainCapture\<session>` so no capture data
is written into the game directory.

## Offline analysis

The deterministic analyzer removes invalid rows, verifies monotonic timestamps
and update indices, and reports measured cadence, planar route length, net
vertical displacement, vertical total variation, opposing/reversal distance,
reversal count, maximum vertical step and route-progress-resampled roughness.
Comparisons align vertical position by normalized cumulative planar progress;
they do not align by wall-clock time or assume identical input timing.

The primary discriminator uses `60-off` as the native reference:

- `240-off` divergent and `240-on` not worse/improved: native high-frequency
  behavior (H1), with improvement specifically supporting H3;
- `240-off` comparable and `240-on` divergent: correction side effect (H2);
- inconsistent route/cadence/validity or mixed metrics: H4.

Thresholds will be declared from synthetic detection limits and the acquired
reference variability before classification. A visual report alone cannot
select a hypothesis, and none of these metrics assigns collision ownership.
