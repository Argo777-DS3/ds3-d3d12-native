# I-0099 fix plan - Plantage à la fermeture, checked against the code and the dumps

> Note (2026-10-26, on its merge into `main`): where this study says the gate runs « on both screens », that was D-0209; since
> D-0218 the gate runs on the ASUS (the OLED added when justified), its memory leg with `-Memory` when a branch touches the memory,
> the loadings or the streaming (the pop-in workstream and I-0099's fix do). The text below is the study as written.

Read-only (2026-10-25 03:05-03:20 Paris, Opus 5.5), on branch `claude/i99-study` (merged into `main` with I-0097 tranche 6's close, 2026-10-26). Nothing built, no live run,
no Ghidra session: the executable was read with `llvm-objdump` over small ranges and the four kept dumps with
`Get-I99Stack.ps1 -AllThreads`. Statuses as in [the ledger](../../knowledge/LEDGER.md). It checks [STUDY](STUDY.md)'s plan
and replaces its step 2.

## 1. What changes in the STUDY (OBSERVED)

The STUDY's mechanism is right about the fault's site (the network loop locking a destroyed mutex) but wrong about the order
in the destructor and about the race.

- **`~DLThread` never stops or joins a started thread.** `FUN_1417E86D0` tests the byte at `thread+0xC0` (`cmpb $0,0xc0(%rcx)`;
  `jne` past the block at 0x1417E8701): the resume, unlock and `WaitForSingleObject(INFINITE)` run only for a thread that was
  never started. `DLThread::Start` (`FUN_1417E8810`) sets that byte to 1, then `ResumeThread` and unlocks the thread's own mutex.
  The thread is created suspended (`FUN_1417FD790`: `_beginthreadex` with flag 4). The STUDY's « stops and joins the thread only
  after it » does not happen for the manager's thread: `~SocketChannelManager` destroys the mutex and closes the thread's handle,
  and the loop keeps running.
- **The imports**, resolved from the executable's import directory (`Get-I99Imports.ps1`): `FUN_1417FD7F0` is `ResumeThread`,
  `FUN_1417FD850` is `WaitForSingleObject` (its argument in microseconds, -1 infinite), `FUN_1417FD7E0` is `CloseHandle`.
- **DS3 has the stop.** `FUN_14198E1E0`, `SocketChannelManager::Stop`: under the `+0x170` mutex it clears the running byte
  `+0x164`, unlocks, then joins the thread (`FUN_1417E8850`, `WaitForSingleObject(INFINITE)`), then releases its channels. The
  loop tests `+0x164` after each pass (0x14198ECFD) and returns. `FUN_1419859C0` (`DLNRD::SocketManager` finalize) calls it, when
  the initialised byte 0x1449313F4 is set, and is called only from 0x141971EEB (a DLNRD finalize reached through the thunk
  0x141962860, which `DLNRD::ExitEvent` and two game functions call).
- **At a quit that stop is skipped.** The manager lives in static storage (0x144931410) built by `GetInstance` (`FUN_141986470`),
  which registers the destroyer `FUN_141986250` with DS3's singleton list (`FUN_141787AC0`). The destroyer calls
  `~DLNRD::SocketManager` (`FUN_1419853A0`), which releases the manager's channels and calls `~SocketChannelManager` at
  `this+0x50` (0x1419854CC), with no call to the stop. Nothing on that path clears `+0x164`.
- **Not a narrow race.** The loop polls with a timeout read from `+0x160`, which the constructor sets to 0x21 (33; read as
  milliseconds, INFERRED), and locks `+0x170` at every pass. Once the manager is destroyed, the loop faults at its next pass unless
  the process ends first. That fits 32 of 36 native quits and 2 of 3 bare-original ones (STUDY F5).
- **The other threads at the fault** (`threads-*.txt`). In all four dumps no thread is inside `~SocketChannelManager`, `~DLThread`
  or a join of the manager's thread. The main thread is further in DS3's shutdown: in three dumps inside `DLNRD::EventManager`'s
  slot 1 (0x14196BB20 → 0x14196B970 → 0x141981E20), joining another DLThread with `WaitForSingleObject(INFINITE)`; in
  `i97-cal-z1` blocked on the heap lock in a `free` of the memory system's teardown (0x141823910), because the faulting thread
  holds that lock while our handler sleeps. The destroyer is therefore the main thread's shutdown, before the event manager
  (INFERRED: the singleton list's runner was not read).

## 2. The fix (proposed)

DS3's own stop, bounded, run where DS3 forgot it. One hook, in our module, nothing in the game's files.

- **Hook** `~DLNRD::SocketManager`, `FUN_1419853A0` (its first 19 bytes are position-independent: `mov r11,rsp`,
  `mov [r11+8],rcx`, five pushes, `sub rsp,0x40`; one new pin). The SocketManager destructor, not the channel manager's: the
  former releases the channels the loop walks (`+0x120`, `+0x1F8`) before it calls the latter, so stopping at the channel
  manager's entry would leave that window open.
- **At entry**, with `scm = this+0x50`, `thread = scm+0x98`, `handle = *(thread+8)`: when `thread+0xC0` is 1 and
  `WaitForSingleObject(handle, 0)` times out (the thread still runs; after DS3's own stop it has ended and the hook does nothing):
  1. lock `scm+0x170` through its own slot 2 with -1, clear `scm+0x164`, unlock through slot 4: exactly the first block of
     `FUN_14198E1E0`;
  2. `WaitForSingleObject(handle, 2000)`;
  3. one row `native_exit_network_stop` (result, milliseconds waited, the destroying thread id, whether it is the main thread).
- **Then the original destructor**, unchanged: with the thread ended, `~DLThread` only closes its handle.
- **Not done**: the channel release after the join in `FUN_14198E1E0`. Today's quit never runs it either; leaving it out keeps
  the original's teardown, minus the fault. `+0x165` is not set (that byte is DS3's error path, `FUN_14198E0B0`); the loop leaves by
  the same exit as after DS3's own stop.
- **On a timeout** the original runs as today (the fault may come) and the row says so; a quit cannot hang.
- **Mode**: installed in the native mode only, so the Reference runs and the gate's original base keep the original behaviour.
  A/B by `no-exit-order-fix.flag` (the module's flag pattern, inert when absent). It is a correction of DS3's teardown, not a
  renderer fallback or a development hook (D-0084): it stays in the play version.

## 3. Risks

- **Save.** The save is written before this teardown at a menu quit (STUDY F5), and no save code runs on this path. The hook adds
  at most one poll period (about 33 ms, INFERRED) to the quit and removes the 3 s the fault handler now sleeps while holding the
  heap lock.
- **Network.** The loop's last pass and its exit path (`FUN_141990600` with `+0x168`, then the context's release) are the ones
  DS3's own stop runs; nothing new is sent or received (INFERRED). Every live test is offline (D-0116), so the online quit is not
  tested; online, the original would fault at the same moment, so the fix sends nothing the original would not have sent before
  dying (INFERRED). Thomas decides whether he tries one online quit himself.
- **Offline.** The manager's thread runs offline too (the fault comes in our offline runs).
- **A lock the loop needs.** The hook joins on the destroying thread before `~SocketManager` takes its own mutex (`this+0x18`).
  If the singleton list holds a lock the loop's last pass takes, the join times out after 2 s and the row says so (bounded).
- **Encrypted code.** 0x1419853A0 reads as plain code on disk; the pin check refuses the launch if not.
- **The other teardowns.** The event manager's join (section 1) waits for another thread; if that thread waited for the network
  loop, the fix shortens the quit, it cannot lengthen it (INFERRED).

## 4. Test protocol

One announced live run of about 25 minutes (D-0116: offline, hash-checked restore, nothing in the game folder), outside any
other session's live test.

1. **Optional, Thomas's rate question**: 5 quits of the bare original (Reference build or no module), counted from Windows'
   event 1000 and the error reports (no new tool; the STUDY's debugger watcher is not needed any more).
2. **A/B on the native**: 10 quits with the fix and 10 with `no-exit-order-fix.flag`, alternated, each half 5 by the close box
   and 5 by the menus (in-game Quit Game, then Quit Game at the title), on the test save, with `exit-dump.flag`.
3. **Proof** (all four):
   - with the fix: no `0xC0000374`, no event 1000, no error report; without it, the usual rate (expected 8 or more of 10). 0 of 10
     against 9 of 10 is p < 0.001 (Fisher); a single fault with the fix sends the work back to analysis (its dump will show where);
   - a `native_exit_network_stop` row at every quit with the fix, result signalled, wait under 100 ms;
   - the quit time (Quit Game or the close box to the process exit) not longer than without the fix;
   - the test save loads again after each quit; Thomas's saves are never written.
4. **Afterwards**: the STUDY's step 4 (the handler's exit-phase special case and its 3 s sleep removed, a heap fault at a quit
   then fails the gate's memory leg and the night endurance). Each later natural quit (gate legs, nights) adds to the sample.

The fix touches `native/`, so its merge passes the replayed path gate (D-0202) on both screens (D-0209).

## Evidence

- `evidence/I-0099/artifacts/threads-i92-mem8-main.txt`, `threads-i96-final-c-z1.txt`, `threads-i97-cal-z1.txt`,
  `threads-i97-final-z1.txt`: every thread of the four dumps, unwound.
- Tools: `Get-I99Stack.ps1 -AllThreads`, `Find-I99Calls.ps1` (direct calls and address loads of a target), `Get-I99Imports.ps1`
  (the import behind an IAT slot).
- Code read (all in `DarkSoulsIII.exe` 1.15.2.0): 0x1417E86D0, 0x1417E8810, 0x1417E8850, 0x1417FD790-0x1417FD889,
  0x14198DD20, 0x14198DEF0, 0x14198E0B0, 0x14198E1E0, 0x14198E910-0x14198ED74, 0x141990880, 0x141985250, 0x1419853A0,
  0x1419859C0, 0x141986250, 0x141986470, 0x141981E20.
