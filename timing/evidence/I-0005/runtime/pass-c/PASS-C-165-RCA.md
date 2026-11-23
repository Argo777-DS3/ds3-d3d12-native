# I-0005 Pass C — 165 FPS cadence RCA

## Decision

**FAIL for the 165 FPS cadence gate under the current display environment.**
The probe requested and retained `165/1`, but broad updates and Presents stabilized
at approximately 120 Hz. The 165 semantic sentinels remain usable as observations,
but this session cannot qualify 165-FPS behavior and does not authorize 240/280.

## Session

Session: `20260919T115655Z-pid10108`

- Broad-update rate: 119.989448 Hz.
- Present rate: 119.878341 Hz.
- Median broad delta: `0.00833333377` s, approximately `1/120`, not `1/165`.
- Semantic samples: 18,317; position, animation, and controller presence valid in
  100% of samples.
- Direct sampler cost: 15.0500 us mean and 595.4 us maximum, or 0.24832% of a
  165-FPS frame on average.
- Override applied `30/0 -> 165/1` and restored exactly `165/1 -> 30/0`, both
  success `1`.

## Discriminating evidence

- Central-state logs retained `fps354=165, flag358=1` in 36,634 captured rows.
- Present used interval 0 for all 18,317 active-update calls; 18 startup/transition
  calls used interval 1.
- I-0004 manual presentation pacing reported `active=0` throughout.
- `force_present_zero=0`; the probe did not substitute a Present result.
- The specialized selector remained in dynamic mode 0 on all 18,333 calls.
- Windows reported the active video mode as 2560x1440 at 119 Hz.
- DS3 `GraphicsConfig.xml` reported `ScreenMode=WINDOW` at 2560x1440.

The close match among desktop refresh, broad-update rate, and Present rate, while
the central request remains 165 and internal/manual pacing stays inactive,
supports a current windowed display/driver/compositor ceiling. This causal owner
is `SUPPORTED`, not `CONFIRMED`: a matched rerun after setting the active display
to at least 165 Hz is required.

## Semantic observations below the failed cadence gate

All marker chains remained valid. Idle and camera markers preserved the player
transform, and the obstacle tail plateau remained below 0.01 unit. The median
animation-clock increment remained close to `10 x` the supplied `1/120` broad
delta. These facts do not establish 165-FPS behavior because actual cadence never
exceeded 120.

## Gate

Do not run 240 or 280. Before retrying 165, close DS3 and select a Windows display
refresh of at least 165 Hz on the game display, preferably the highest available
mode needed by the later ordered targets. Then re-run the same 165 marker sequence
with no other intervention change.
