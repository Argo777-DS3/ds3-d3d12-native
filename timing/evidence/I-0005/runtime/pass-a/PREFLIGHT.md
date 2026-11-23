# I-0005 Pass A runtime preflight

Checked 2026-09-19 before launch and rechecked after the run.

## Baseline identity

- Game directory: `C:\Program Files (x86)\Steam\steamapps\common\DARK SOULS III\Game`
- File count: 89
- Total bytes: 26,649,084,250
- Executable: `DarkSoulsIII.exe` 1.15.2.0, 88,960,032 bytes
- SHA-256: `EF5E07C55222F14FFDDECF2C724A0C2A95CEF0D4DA0E075B0DE0BB108B69498C`
- Authenticode: valid; signer `FromSoftware,Inc.`
- Steam metadata: AppID 374320 / BuildID 10167187

The file count and total size matched the clean I-0001 Steam baseline. No existing game file was changed for instrumentation.

## Proxy and overlay checks

The game directory contained none of the known interception candidates checked before launch: local `dxgi.dll`, local `d3d11.dll`, `dinput8.dll`, `winmm.dll`, ReShade, NUE, ASI loaders, RTSS/RivaTuner, or Special K artifacts.

The module snapshots taken inside the process at probe start and stop found:

- Windows system `dxgi.dll` and `d3d11.dll`;
- the standard Steam `gameoverlayrenderer64.dll`;
- the declared observation probe `I0005PassA.dll`;
- no module name/path matching ReShade, NUE, RTSS, RivaTuner, Special K, Game Bar, or NVIDIA Overlay.

Steam was already configured for offline mode. No non-loopback established Steam connection was observed during the final preflight. User-level NVIDIA Overlay and Game Bar processes were stopped before launch. The Steam overlay is recorded rather than misrepresented as a vanilla graphics proxy.

## Instrumentation boundary

The probe was injected from the repository after the stock executable started. It installed eight hooks, all with `MH_OK`, and wrote logs outside the game directory. It did not write timing fields, constants, code, or game files. The probe was stopped cleanly and flushed before the game exited.

Evidence: `sessions/20260919T083240Z-pid12832/hook-status.csv`, `modules-start.csv`, `modules-stop.csv`, `session-info.txt`, `READY`, and `COMPLETE`.
