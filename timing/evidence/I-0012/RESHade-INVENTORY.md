# I-0012 ReShade and inherited-core inventory

Observed offline on 2026-09-21 before any I-0012 installation or game launch.

## Existing game-directory state

- Exact game directory:
  `C:\Program Files (x86)\Steam\steamapps\common\DARK SOULS III\Game`.
- `DarkSoulsIII.exe` was not running (`0` matching processes).
- Existing ReShade proxy: `dxgi.dll`, file version `6.8.0.2155`,
  5,592,064 bytes, SHA-256
  `0CEE63F9C9F13F3AC909C5B4903F4DBB4B719A7AB3B4F13B0DEAF83C814B94F7`.
- The proxy is unsigned. ReShade's own documentation identifies the unsigned
  build as the build with full add-on support.
- Export inspection confirms `ReShadeRegisterAddon`,
  `ReShadeUnregisterAddon`, `ReShadeRegisterOverlay`,
  `ReShadeRegisterOverlayForAddon`, `ReShadeRegisterEvent`,
  `ReShadeGetImGuiFunctionTable` and `ReShadeLogMessage`.
- No `.addon`, `.addon32` or `.addon64` file and no add-on directory was
  present.
- `ReShade.ini` only contains shader/texture paths, the HOME overlay key and a
  disabled proxy-library section. It contains no `[ADDON]` override.
- `ReShadePreset.ini` is empty. `reshade-shaders` contains only the installed
  standard shader and texture files.
- `ReShade.log` is the 982-byte installer/bootstrap message saying that, if
  read after a launch, ReShade was probably not loaded. It contains no runtime
  or add-on load result. Therefore add-on loading remains unproved until
  Thomas's independent first test.

No I-0012 file was written to the game directory during this inventory.

## Reusable I-0011/I-0010 core

The inherited implementation separates naturally into these reusable pieces:

1. finite cadence state (`target/1`) and exact Native restoration of the
   captured timing pair;
2. QPC-measured actual update FPS;
3. update-boundary request application;
4. the accepted comparison normalization and guards included directly from
   `evidence/I-0010/prototype/prototype_core.hpp`;
5. exact debug-register configuration/restoration included directly from
   `evidence/I-0010/prototype/debug_config.hpp`;
6. exact hook/signature/build identity, fail-closed behavior and restoration.

The I-0011 Win32/GDI overlay, its focus handling, mouse interaction and
external controller are host concerns and are not inherited by the I-0012
core. Genuine Unlimited remains unsupported because no limiter-off engine
state was established.

## Exact-build constraints retained

- Steam AppID 374320 / BuildID 10167187 / executable 1.15.2.0.
- Full executable SHA-256:
  `EF5E07C55222F14FFDDECF2C724A0C2A95CEF0D4DA0E075B0DE0BB108B69498C`.
- Four runtime signatures: broad update `0x00D57900`, spatial boundary
  `0x02344730`, causal comparison `0x005C73D8`, recurrence exit `0x005C7435`.
- Exact opcode, player/owner chain, consecutive update index, transform,
  position-pair, finite previous-delta and safe update-boundary guards.
- Correction formula remains
  `raw * ((1/60) / previous_broad_delta)`.
- Start is inert. Stop must disable correction/hooks and restore the exact
  timing pair and saved debug state.

## Cycle-1 conclusion

The existing ReShade install is a plausible full-add-on host and requires no
replacement or configuration edit. The smallest reversible I-0012 footprint is
one uniquely named `.addon64` file in the existing ReShade base directory.
Runtime loading remains `UNKNOWN` until Thomas's independent test; all other
statements above are offline observations or inherited confirmed constraints.
