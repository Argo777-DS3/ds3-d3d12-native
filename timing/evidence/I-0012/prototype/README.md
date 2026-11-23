# I-0012 DS3 timing validation add-on

This is a lightweight ReShade 6.8.0 settings panel backed by a ReShade-free
cadence/correction core. It is test infrastructure, not a product mod or an
enemy/boss measurement instrument.

## What it exposes

- numeric 30–360 FPS control and presets 30/60/90/120/144/165/240;
- measured actual broad-update FPS;
- exact Native cadence restoration;
- the unchanged accepted locomotion timing correction ON/OFF;
- explicit unavailable Unlimited (no fake cap);
- `Restore Native & Stop`, which restores and leaves the loaded add-on inert
  for the rest of that process.

The add-on starts inert. It contains no actor-specific correction, no FPS gain
table and no ReShade render/Present hook.

## Build

Run `build.ps1` from PowerShell. The strict build pins MinHook, ReShade 6.8.0
and its ImGui submodule, compiles with warnings as errors, runs the independent
core safety suite ten times, validates formula/guard identity, validates the
full exact game executable and writes provenance records.

Validated output: `bin\DS3TimingValidation.addon64`.

## Install, inspect, remove

With Dark Souls III closed:

```powershell
powershell -NoProfile -File .\Manage-Addon.ps1 -Action Status
powershell -NoProfile -File .\Manage-Addon.ps1 -Action Install
```

Installation copies exactly one file and does not edit the existing ReShade
configuration. To remove it after closing the game:

```powershell
powershell -NoProfile -File .\Manage-Addon.ps1 -Action Uninstall
```

The manager verifies the exact game build and refuses overwrite/removal on a
hash mismatch.

## Independent first test

Use the tool only in offline/single-player conditions. The installed ReShade
build has full add-on support and ReShade itself warns that such builds are for
single-player use.

Launch DS3 normally, open ReShade with `HOME`, then use the
`DS3 Timing Validation` settings in ReShade's add-on area. Numeric cadence and
correction requests are applied on the next broad-update boundary. `Native`
restores the captured original timing pair. Use `Restore Native & Stop` before
ending the test; after Stop, restart DS3 to use the controls again.

This first test is intentionally Thomas-directed and uninstrumented. No
humanoid, dog or Gundyr capture procedure is imposed here. If later measurement
is useful, it will be added as a separate evidence-bearing step.

## Failure behavior

The core refuses any executable other than the exact BuildID 10167187 image,
checks its full SHA-256 and four native signatures, and fails closed on guard,
timing-object or restoration errors. The panel reports `FAILED` and the guard
counter. Process exit removes all in-memory hooks; the only persistent I-0012
game-directory artifact is the removable `.addon64` file.
