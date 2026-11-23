# I-0013 terrain trajectory capture add-on

This prototype extends the validated I-0012 ReShade 6.8.0/API-20 windowed host
with one bounded observation surface. Cadence control, the accepted locomotion
correction, exact-build checks and restoration logic are inherited; the capture
adds no engine hook and changes no correction formula or guard.

## Capture

The add-on starts inert. `Start route capture` is accepted only when the next
required matrix state is already active:

1. 60 FPS / correction OFF;
2. 240 FPS request / correction OFF;
3. 240 FPS request / correction ON;
4. 60 FPS / correction ON (optional low-cost control).

At each existing broad-update return, a fixed buffer records QPC, update index,
delta, player/transform identity and world `x/z/y`, where `z` is the qualified
vertical field at `transform + 0x84`. `Stop & save capture` exports outside the
game directory under `%LOCALAPPDATA%\DS3TerrainCapture\<session>`. The 65,536-row
limit never wraps; overflow or shutdown during recording preserves a failed or
partial attempt without advancing the matrix.

Use `analyze_capture.mjs` on a copied session directory:

```powershell
node analyze_capture.mjs --session <directory> --out analysis.json
```

The analyzer aligns traces by cumulative planar route progress and reports
vertical reversal/excess, roughness and reference-shape differences. It does
not assign a causal hypothesis automatically.

## Build and safety

Run `build.ps1` from PowerShell. It pins the same MinHook/ReShade/ImGui sources,
builds with warnings as errors, runs the inherited restoration suite and the
synthetic trajectory analyzer suite, verifies exact game bytes and records
source/upstream hashes.

Use standard windowed mode only. The tool is for offline/single-player testing,
does not launch DS3, and does not modify the executable or persistent game data.
Installation remains one hash-guarded `.addon64` file; removal requires DS3 to
be closed. Do not install or run it without the announced protocol.
