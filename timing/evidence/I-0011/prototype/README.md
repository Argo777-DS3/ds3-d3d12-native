# I-0011 runtime FPS control and overlay

This exact-build prototype is a compact, disabled-by-default implementation of
the I-0011 contract. Loading the DLL creates control objects and installs the
two already accepted locomotion hooks, but leaves timing, correction and the
overlay off in `STANDBY`.

Press `HOME` to show or hide the compact overlay. Its dark, flat layout uses a
single blue accent, restrained status cards and simple controls in the spirit
of modern graphics utilities without copying another project's structure.
Ordinary technical counters stay in the session logs rather than the UI.

## Controls

- Slider: integer targets from 30 through 360 FPS, previewed while dragging
  and applied once when the button is released.
- Presets: 30, 60, 90, 120, 144, 165 and 240.
- `Native`: restore the exact timing pair captured before the first change.
- `Unlimited unavailable`: visible but intentionally non-operative. The
  recovered timing architecture has no proved limiter-off representation; see
  [UNLIMITED-ASSESSMENT.md](UNLIMITED-ASSESSMENT.md).
- `Locomotion Timing Fix`: queue the accepted correction ON or OFF at the next
  broad-update boundary.
- `Restore & Stop`: disable correction and hooks, restore debug registers and
  the exact original timing pair, and leave the loaded DLL inert.

The overlay is a dependency-free Win32/GDI tool window attached to the DS3
window. Opening it transfers focus from the game so DS3 releases gameplay
input and the controls can be used directly; closing it returns focus to the
game. It follows client-window movement only when the position actually
changes, and hides while both DS3 and the overlay lack focus or DS3 is
minimized. No renderer or Present hook is added.

Live testing refined that expectation: focus transfer stops character input but
does not release a usable pointer during ordinary gameplay. Open a DS3 menu
before using the overlay mouse controls. Slight panel jitter also remains.
Thomas accepted both limitations because this overlay is test infrastructure,
not the product UI; see the final live result. No additional UI patch is
scheduled in I-0011.

## Causal correction

The runtime directly includes the accepted I-0010 `prototype_core.hpp` and
`debug_config.hpp`; it does not copy or alter them. The correction remains:

`raw * ((1/60) / previous_broad_delta)`

Opcode, player/owner chain, consecutive-index, transform, position-pair,
finite-delta, exact-build and restoration guards remain mandatory. FPS control
uses only the central timing pair and never changes this formula.

## Build and offline validation

Run `build.ps1`, then `node validate_exact_build.mjs` from the repository root.
The strict build compiles with warnings-as-errors, runs the deterministic safety
suite ten consecutive times, records source/binary hashes and verifies the
accepted formula/debug cores. The exact-build validator checks the full game
executable SHA-256 plus all four native signatures before any injection.

`Invoke-Prototype.ps1` never launches DS3. Its `Inject` action re-runs the
exact-build validator before loading the inert DLL into an already-running PID.
Do not use it until the announced live protocol is authorized for execution.
