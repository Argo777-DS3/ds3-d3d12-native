# I-0014 unified timing-fix test add-on

This is a lightweight ReShade 6.8.0 settings panel backed by a ReShade-free
cadence and correction cores. It is test infrastructure, not a product mod or
a distance-measurement instrument.

## What it exposes

- numeric 30–360 FPS control and presets 30/60/90/120/144/165/240;
- measured actual broad-update FPS;
- exact Native cadence restoration;
- independent `LTF — Locomotion Timing Fix` ON/OFF;
- independent `TRTF — Terrain Response Timing Fix` ON/OFF;
- explicit unavailable Unlimited (no fake cap);
- `Restore Native & Stop`, which restores and leaves the loaded add-on inert
  for the rest of that process.

The add-on starts with Native cadence, LTF OFF and TRTF OFF. It contains no
actor/FPS gain table, empirical compensation or ReShade render/Present hook.
The correction and cadence runtime remains alive across ReShade swap-chain
runtime recreation.

## Build

Run `build.ps1` from PowerShell. The strict build pins MinHook, ReShade 6.8.0
and its ImGui submodule, compiles with warnings as errors, runs the independent
core safety suite ten times, validates formula/guard identity, validates the
full exact game executable and writes provenance records.

Validated output: `bin\DS3UnifiedTimingFixTest.addon64`.

## Install, inspect, remove

With Dark Souls III closed:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\Manage-Addon.ps1 -Action Status
powershell -NoProfile -ExecutionPolicy Bypass -File .\Manage-Addon.ps1 -Action Install
```

Installation copies exactly one file and does not edit the existing ReShade
configuration. To remove it after closing the game:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\Manage-Addon.ps1 -Action Uninstall
```

The manager verifies the exact game build and refuses overwrite/removal on a
hash mismatch.

## Independent first test

Use the tool only in offline/single-player conditions. The installed ReShade
build has full add-on support and ReShade itself warns that such builds are for
single-player use.

Launch DS3 normally, open ReShade with `HOME`, then use the
`DS3 Unified Timing Fix Test` settings in ReShade's add-on area. Numeric cadence
and correction requests are applied on the next broad-update boundary. `Native`
restores the captured original timing pair. The two required distance checks
are directly available as 60 and 240 presets with LTF ON / TRTF OFF, followed
if desired by the same two presets with both corrections ON.

The same controls are available in exclusive fullscreen without relying on
overlay interaction:

- `Ctrl+F6`: toggle LTF;
- `Ctrl+F7`: toggle TRTF;
- `Ctrl+F8`: LTF OFF, TRTF OFF and Native cadence;
- `Ctrl+F9`: 60 FPS;
- `Ctrl+F10`: 240 FPS;
- `Ctrl+F11`: Native cadence;
- `Ctrl+F12`: restore and stop the runtime.

Use `Restore Native & Stop` (or `Ctrl+F12`) before ending the test; after Stop,
restart DS3 to use the controls again. Windowed and exclusive-fullscreen
operation are both required, but exclusive fullscreen still needs Thomas's
first manual confirmation because I-0012's earlier failure had no causal
localization.

This first test is intentionally Thomas-directed and uninstrumented. No distance
probe, automated route or capture procedure is included. If later measurement
is useful, it will be added as a separate evidence-bearing step.

## Failure behavior

The core refuses any executable other than the exact BuildID 10167187 image,
checks its full SHA-256 and five native signatures, and fails closed on guard,
timing-object or restoration errors. The panel reports `FAILED` and the guard
counter. LTF restores its debug-register state; TRTF restores the exact last
native producer state it overwrote; cadence restores the captured timing pair.
Process exit removes all three in-memory hooks. The only persistent I-0014
game-directory artifact is the removable `.addon64` file.
