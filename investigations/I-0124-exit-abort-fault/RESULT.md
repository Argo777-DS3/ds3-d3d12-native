# I-0124 - The game's exit fault in 141A3B670 at a close, and the endurance tour's quit not opened (La faute à la fermeture): result

## Conclusion

Complete (2026-11-03 22:45 to 2026-11-04 01:35, the R pilot). DS3's own exit-order defect (H1: the static path OBSERVED, the race
itself INFERRED): its FMOD sound thread still ran when the CRT's exit chain destroyed the heap it frees into. Tranche 1, DS3's own sound
stop run at the quit, merged `ac958d0c`; its start check `start-ac958d0c` passed (the row « signalled », 110 ms); the night endurance
run again clean (below), which completes the full control D-0230 of 2026-11-03. The fault's absence is not proven (a rare fault); the
full controls keep counting it. H3 (the tour's quit menu not opened once) did not recur in 54 visits: a residual to watch.

## The full control of 2026-11-03 (D-0230)

Main `ca6e70e5` (I-0123 merged `4a1ac742`), the launcher's module (`evidence/I-0066/artifacts/game-bootstrap-build`), 2026-11-03
21:20-22:28 Paris, the ASUS `\\.\DISPLAY1` alone (the OLED off), offline, every save restored by hash, Thomas at work
([Run-FullControl-1103.ps1](Run-FullControl-1103.ps1); its log `evidence/I-0123/artifacts/full-1103.log`):

- The replayed path gate `full-1103-gate` with its memory leg: PASS on the ASUS base (period 16.67 ms; frames ending over 2.04 P 1,
  over 1.5 P 18.5; display late 8.5; game steps over 2.04 P 1; both standstill pictures within their threshold; memory -0.5 MB a
  visit over 8 loads; `evidence/I-0083/gates/full-1103-gate.json`).
- The control sizes, the replayed path each: 1920x1080 in full screen (`full-1103-1080fs`) and a 1500x844 window (`full-1103-1500w`):
  no fault, restoration held, the controller's 0xD3126612 and `closed` 1 as every `-GraphicsLevels` run since I-0100.
- The night endurance `full-1103-night`: **stopped after 24 minutes**: the tour's twelfth visit (Anor Londo, loop 3) did not open the
  quit menu (its `v12-quit` and `v12-noquit` pictures show the player in the world, no menu, after Escape and the four clicks, twice),
  so the tour declared itself stuck and closed the window from the world; at that close the game faulted (0xC0000005 at
  `0x140F14C52`, the engine's abort handler, on a DS3 worker thread: `141A3B670`+0x184 calling CRT `abort` `141FD0EAC`; no frame of
  the module), after the module's `native_exit_network_stop` row. Restoration ok. The same fault at a close was left in I-0114
  (`i114-st4-n60`) and I-0115 (`i115-hall-n60`), not investigated then.

## The abort path, read statically (2026-11-03: a Sonnet ghidra-research subagent through GhidraMCP, re-read by a fresh Opus)

- Not an abort: `141FD0EAC` is the CRT `_purecall` (its decoded handler, then `_RT_PUREVIRT`); the handler is DS3's
  `xor rax,rax; mov dword [rax],0xDEAD` from `140F14C4F`, its store at `140F14C52`, the dump's RIP (OBSERVED, checked twice).
- `141A3B670`+0x17F is a virtual call `[[RBX]+0x18]` on `RBX = *(this+0xC70)`, the allocator of the sound manager's read-request
  queue (`+0xC58`); its vtable slot 3 is `_purecall`, so the object was already destroyed. The thread is `DLMO::MOFmodSoundManager`'s
  (Run `141A39FC0`, vtable `143A7AD48`, the global `1449323B8`), which loops while `+0x3B1` is set or its queues are not empty
  (OBSERVED, checked twice).
- `+0xC70` is written by the manager's constructor from the sound registry's `+0x18`, which `140EA9880` sets to `14465ABA8`, the
  dump's RBX: a static heap whose `atexit` destructor (`1427173B0`, registered by `142692B60`) frees its blocks and resets its vptr to
  a vtable of `_purecall` (`14279B458`) (OBSERVED). So the CRT's exit chain destroyed the heap while the sound thread still ran
  (INFERRED: the race itself is not in the dump, which holds only the faulting thread's registers).
- DS3's stop of the thread (`141A2DE20`: two messages posted to its queue, `+0x3B1` cleared, an infinite join, then its `+0x6A0`
  object destroyed and freed) is reached only from `SprjSoundStep::STEP_Finish` (task 0x36, through `SprjSoundImp`'s destructor,
  `1407CE5B0` then `141A2AD00`); a quit does not reach it (INFERRED from the crash). Once its loop ends, the sound thread itself
  releases FMOD into the heap (slot 7, `141A366B0`). H1: DS3's own exit-order race, of I-0099's kind.
- The module's I-0099 hook (`~DLNRD::SocketManager`) runs inside the CRT's exit chain (the singleton list's runner is an `atexit`
  entry) and before the heap's destructor: INFERRED from the initializers' order, then OBSERVED at eight quits (the heap's live vtable
  read at the hook every time, below).

## Tranche 1, the sound thread stopped at the quit (2026-11-03)

- **What it does** (OBSERVED in the code, build-3): at the I-0099 hook, before the network stop, DS3's own stop `141A2DE20` runs on a
  helper thread (its join has no bound) when the manager's vtable is `143A7AD48`, `+0x3B1` is set, its thread was started (`+0xC8`)
  and runs, and the heap at `14465ABA8` still has its live vtable `142990838`; the helper's end, so the whole stop, is awaited 2 s at
  most; one row `native_exit_sound_stop` (signalled, timeout_helper_left, or why skipped). `no-sound-stop.flag` leaves this stop out,
  `no-exit-order-fix.flag` both; the stop's address is pinned by the build; the start check reads the row and fails on a timeout, a
  failure or a heap already destroyed.
- **Witness quits** ([Run-I124Quits.ps1](Run-I124Quits.ps1): the one-minute start check, the close box in the world, offline, restored
  by hash, the ASUS): build-1 (before the step review's answers) `start-a124001` to `a124004`, 2026-11-03 23:10-23:14: every check
  passed, every quit's row « signalled » in 94 to 125 ms, before the network stop's « signalled », no fault; build-2 `start-a124011` to
  `a124014`, 23:26-23:31: the same, 110 to 141 ms, the heap alive at the hook every time. The fault was seen three times in many
  closes: eight clean quits cannot show it gone, only that the stop runs, ends and does not block.
- **Step review** (fresh Opus, high): nothing high. Answered (build-2): M1 DS3's stop goes on after its join (the `+0x6A0` object):
  the helper's end awaited, not the sound thread's; M2 the heap's live vtable required (an unexpected order cannot turn a clean quit
  into a pure call); L1 a timeout's row says the helper was left; L2 the started byte; L3 an invalid handle told apart; L4
  `no-sound-stop.flag`.
- **Final review** (fresh Opus, xhigh, `main...bedb9c1f`): nothing high. Answered (build-3): M1 build-2 witnessed (above), the start
  check reads the row; M2 this RESULT, STATE, status and the index; L3 `no-sound-stop` known to the runner (its pattern and stale-flag
  cleanup); L4 a manager of another type its own skip; L5 the stop's address pinned; L6 the I-0099 comment, the evidence list. Kept: L1
  (FMOD after its early release: no static pass over the later destroyers; eight clean quits); L2 (a timeout leaves the helper racing
  the heap's destructor: the original's fault class, not a hang).

## After the merge (2026-11-04)

- The launcher's module rebuilt from `ac958d0c` (unit tests passed); the start check `start-ac958d0c` passed, its `sound_stop`
  « signalled » in 110 ms (2026-11-04 00:00, the ASUS, offline, restored).
- The night endurance `night-ac958d0c` (the launcher's module, [Run-I124After.ps1](Run-I124After.ps1), 00:01-01:35, the ASUS alone,
  measure mode; `Get-EnduranceSummary.ps1`): 94 minutes, the five zones, 54 loads, no fault, no crash dump, 60 frames/s, private
  memory +1.66 MB a visit (+1.9 on 2026-10-28), the quit menu opened at all 54 visits, closed cleanly (exit 0, the sound stop
  « signalled » in 94 ms), restored by hash.

## Evidence

- `evidence/I-0066/artifacts/full-1103-night/` (crash report and dump in `crashes/`, `tour.txt`, `shots/`, `startup.jsonl`).
- `evidence/I-0124/artifacts/` (the GhidraMCP readings, `verify/` the fresh Opus's check, `context.txt` the dump's registers,
  `quits-1.log`, `quits-2.log`, the builds' logs); the witness quits `evidence/I-0066/artifacts/start-a124001` to `a124004` and
  `start-a124011` to `a124014`.

## Uncertainty / unresolved questions

- H3: the endurance tour's quit menu did not open at Anor Londo's twelfth visit (the pilot's input or a game state, not read); not
  recurred in the 54 visits of `night-ac958d0c`.
- H2 (the module making DS3's teardown late) not excluded; the module's hook is now the one that stops the thread.
- FMOD after its early release (final review L1); the fault's rate after the fix (the night endurance run again, then the full
  controls, count it).
