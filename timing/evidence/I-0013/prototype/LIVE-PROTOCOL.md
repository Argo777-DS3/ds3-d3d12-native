# I-0013 first live discriminator protocol

Status: prepared offline; do not execute before the build gate passes and
Thomas has been notified.

## Preconditions

- Offline/single-player mode; Dark Souls III closed before installation.
- Standard windowed mode. Do not use exclusive fullscreen.
- One short, safely repeatable slope or stair route with visually identifiable
  start and end points; use the same direction and ordinary movement input in
  every cell.
- No change to display mode, route, equipment or movement style between cells.

## Sequence

1. Install the one validated `DS3TerrainCapture.addon64` file with the manager,
   launch DS3 normally, open ReShade with `HOME`, and verify the core is ready.
2. Select 60 FPS and correction OFF. Return to the exact route start, begin
   `60-off`, traverse to the endpoint once, then stop and wait for `SAVED`.
3. Select 240 FPS and keep correction OFF. Repeat the same route as `240-off`.
4. Keep 240 FPS and enable correction. Repeat as `240-on`.
5. If returning to the start is cheap, select 60 FPS with correction ON and
   repeat once as the non-regression `60-on` control.
6. Click `Restore Native & Stop`, confirm timing/debug restoration, close DS3,
   then use the manager to remove the exact add-on file.

Report any route deviation, fall, combat interruption, menu interruption or
visible hopping separately. Do not substitute a failed attempt silently. The
capture folder remains under `%LOCALAPPDATA%\DS3TerrainCapture`; it will be
copied and hashed into the Evidence Store before analysis.
