# I-0005 Pass A external instrumentation

This probe is observation-only and is built outside the Dark Souls III installation. It injects one DLL into the running stock executable, installs reversible in-memory hooks at the exact BuildID 10167187 RVAs, and writes CSV logs under `../sessions/`.

It does not write timing fields, change frame-rate constants, deploy a proxy DLL, or modify the game directory.

## Pinned dependency

- MinHook v1.3.4
- commit `c3fcafdc10146beb5919319d0683e44e3c30d537`
- local source path recorded by `build.ps1`

## Captured boundaries

- broad scalar path: `00D57900`, `00023410`, `000499C0`
- first virtual callback vector reached from `000499C0`
- presentation pacing: `000D5E30`
- swap-chain presentation wrapper: `0192CF00`
- specialized selector: `00DD4B60`
- central timing update wrappers: `00ED8E50`, `00EDAAE0`, with bounded before/after field snapshots
- loaded-module snapshots at probe start and stop

The injector exposes only two operations: inject into an already running PID, and signal a clean stop/flush. No DLL is copied to the game installation.
