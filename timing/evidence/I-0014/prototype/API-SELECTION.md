# ReShade add-on surface and provenance

## Selection

I-0014 uses the settings-overlay surface only:

- register the module with `reshade::register_addon`;
- register a settings callback with `reshade::register_overlay(nullptr, ...)`;
- render controls through the ImGui function table supplied by ReShade;
- call only the host-independent I-0014 control API;
- register no render, Present, resource or command-list event.

This is smaller than a standalone overlay and does not make ReShade part of the
cadence or correction mechanism. A runtime-thread keyboard fallback supplies
the same essential controls in exclusive fullscreen and survives ReShade
swap-chain runtime destruction/recreation without adding a renderer hook.

## Exact upstream

- ReShade tag `v6.8.0`, commit
  `18deaa52de0c425a78b329e9cb3c497281cd00ec`.
- ReShade API version `20` from tag-local `include/reshade.hpp`.
- ImGui submodule commit
  `3912b3d9a9c1b3f17431aebafd86d2f40ee6e59c`.
- License: upstream `LICENSE.md` (BSD-3-Clause for ReShade; bundled headers
  also carry their SPDX declarations).

The tag-local API header documents add-on registration and settings-overlay
callbacks. The tag-local add-on manager searches the ReShade base path (unless
`[ADDON] AddonPath` overrides it) and accepts `.addon64` on 64-bit builds. The
current local `ReShade.ini` has no such override.

Primary sources:

- [ReShade 6.8.0 API header](https://raw.githubusercontent.com/crosire/reshade/v6.8.0/include/reshade.hpp)
- [ReShade 6.8.0 add-on manager](https://raw.githubusercontent.com/crosire/reshade/v6.8.0/source/addon_manager.cpp)
- [Official ReShade repository](https://github.com/crosire/reshade)

## Reversible ownership

The installation owns exactly
`DS3UnifiedTimingFixTest.addon64`. It does not edit `dxgi.dll`, `ReShade.ini`, the
preset, shaders or game data. Installation refuses to overwrite a different
file at that path; removal refuses to delete a file whose hash differs from the
validated bundled artifact. Both operations require DS3 to be closed.

The core performs a full executable SHA-256 check plus five exact native
signature checks before installing three hooks: the two inherited LTF hooks and
the distinct TRTF producer hook. A mismatch fails closed. Loading starts in
`STANDBY`: timing is Native, LTF is OFF, TRTF is OFF and no game state has been
changed.
