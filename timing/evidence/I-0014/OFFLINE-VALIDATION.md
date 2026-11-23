# I-0014 phase-1 offline validation

Date: 2026-09-22

## Outcome

Pass. No Dark Souls III launch occurred. The exact game executable was read
only for its full SHA-256 and five native byte signatures. The compiled x64
ReShade API-20 add-on starts with Native cadence, LTF OFF and TRTF OFF.

## Independent correction checks

| Path | Offline result | State ownership |
|---|---|---|
| LTF | Accepted formula/guard header is included directly; guarded comparison and debug-register plan compile unchanged. | Dedicated request, active flag, transition counter, comparison counter, correction counter and two inherited hook paths. |
| TRTF | Native-reference no-write, additive, exponential decay, reset and clamp branches pass exact deterministic checks. | Dedicated request, active flag, transition/call/write/passthrough counters, current-player identity and producer hook at `0x005E6580`. |
| Coexistence | LTF ON + TRTF ON passes the pure lifecycle model; changing either control leaves the other's active state unchanged. | No shared correction boolean, formula, hook site or telemetry counter exists. |

The ten-run suite exercises OFF/ON/OFF/ON for each correction, simultaneous
activation and stop planning. LTF restores its saved debug state. TRTF records
the exact native producer state before each overwrite and restores that value
when TRTF is turned off or the runtime stops. Cadence restores the exact timing
pair captured before the first numeric override.

## Host and display-mode checks

- ReShade owns only registration and settings UI; neither correction core
  includes ReShade headers or symbols.
- Windowed control uses the existing settings panel.
- Exclusive fullscreen has a `GetAsyncKeyState` runtime-thread fallback that is
  independent of ReShade runtime destruction/recreation:
  `Ctrl+F6` LTF, `Ctrl+F7` TRTF, `Ctrl+F8` restore all, `Ctrl+F9` 60 FPS,
  `Ctrl+F10` 240 FPS, `Ctrl+F11` Native and `Ctrl+F12` stop.
- No Present, draw, resource or command-list event is registered.

The fullscreen mechanism is structurally qualified offline. It is not a live
claim: Thomas must confirm that the controls and achieved cadence behave in
exclusive fullscreen on the target display.

## Build and safety

- Exact target: Dark Souls III 1.15.2.0 / BuildID 10167187,
  SHA-256 `EF5E07C55222F14FFDDECF2C724A0C2A95CEF0D4DA0E075B0DE0BB108B69498C`.
- Pinned MinHook v1.3.4, ReShade 6.8.0/API 20 and ImGui revisions match the
  canonical I-0012 dependencies.
- Compilation uses x64 C++20, warnings as errors and UTF-8 source handling.
- PE inspection confirms the expected exports, no static ReShade DLL import and
  only the expected Windows dependency set.
- The one-file manager refuses a wrong game hash, a different file at its
  target path, a running game, or a known older timing add-on conflict.
- The install → remove → reinstall cycle passed with bundled/installed SHA-256
  `6341A19F0C2FBE5C9461BF960F1DC006C08E710CDBED82ED252D783DFF281904`;
  existing ReShade files retained their hashes. The add-on is installed for
  Thomas's test and DS3 remains closed.
- Unlimited remains explicitly unsupported; no high-cap substitute exists.

## Boundary

No distance probe, automated route, acquisition or new RCA was added. The suite
does not establish distance equivalence, fullscreen runtime success or broader
gameplay correctness. Those remain downstream of Thomas's manual test.
