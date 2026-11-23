# I-0099 study - Plantage à la fermeture (the exit heap fault 0xC0000374)

Read-only study (2026-10-23 and 24, Opus 5.5), merged into `main` with Thomas's yes (2026-10-24 00:16). Nothing built, no live run, no
system setting changed. Statuses follow [the ledger](../../knowledge/LEDGER.md): OBSERVED (read in a trace), CONFIRMED (read in
two independent sources), INFERRED (follows from what was read, not witnessed). A fresh Opus agent reviewed this text against
its evidence; its corrections are applied.

## Answer

The fault is the original game's own defect, not ours, and its mechanism is identified.

At a quit, DS3 destroys its network channel manager `DLNR3D::SocketChannelManager` while that manager's own network thread
is still running. The destructor destroys the manager's mutex member first, and stops and joins the thread only after it
(the members' reverse declaration order). If the thread locks that mutex in between, the call goes through the destroyed
mutex's base vtable and lands on a deleting destructor, which frees an address inside the manager's heap block. Windows' heap
refuses that free and raises `0xC0000374` (STATUS_HEAP_CORRUPTION). No frame of `game_bootstrap.dll` is on that stack. No
earlier corruption of the heap is needed to explain it (INFERRED: ntdll's failure record is not in the dumps).

Our module does not cause it, and nothing shows that it worsens it. It can change the timing of the race (INFERRED). The bare
original raised it at 2 of its 3 natural quits on record (F5).

## Findings

### F1. Windows' log: the same fault at most natural quits, in both builds (OBSERVED)

- Event 1000 (Application Error), from the log's start (2026-09-30 18:03): 754 `0xC0000374` faults of `DarkSoulsIII.exe`
  1.15.2.0 in `ntdll.dll`.
  - 359 at offset 0x117EB5 (ntdll 10.0.26100.9444, until 2026-10-13).
  - 395 at offset 0x118095 (ntdll 9711, since 2026-10-16).
  - The offset moved with a Windows update; the site in ntdll is the same.
- Windows' archived error reports (`C:\ProgramData\Microsoft\Windows\WER\ReportArchive`) keep 331 heap-fault reports, listed in
  `evidence/I-0099/artifacts/wer-reports.csv`. All fall in one bucket (`4deb66013278259da43ae5cb305033c2`, `StackHash_0000`,
  `PCH_84`).
  - 218 have a module list with `game_bootstrap.dll` and `d3d12.dll`.
  - 2 have a module list with neither, the bare original: `i92-13-original-quit` (pid 29316, 2026-10-23 00:56:38) and
    2026-10-19 16:58:57, during Thomas's play that afternoon.
  - 111 have no module list, so their origin is unknown.
- A `StackHash_0000` bucket is coarse (application, ntdll and code). On its own it does not prove a common cause; F3 and F4 do.

### F2. The kept dumps (OBSERVED)

The 22 heap-fault dumps written with `exit-dump.flag` (2026-10-21 to 2026-10-23; the controller records 32 heap faults, 10 of
them without a dump) all hold the same exception: `0xC0000374` at `ntdll+0x118095`, with parameter `ntdll+0x1CE0E0` (ntdll's
heap failure record, not in the dumps). The other dumps in `evidence/I-0066/artifacts/*/crashes/` are injected or unrelated
faults (I-0082, I-0084, I-0088).

In 18 of the 22 the faulting thread had already been ended by `ExitProcess` when the controller wrote the dump, so they hold no
stack. In 4 the thread was still alive, with its stack: `i92-mem8-main` (`main` before I-0092), `i96-final-c-z1`, `i97-cal-z1` and
`i97-final-z1`. They come from three trees on one day and two boots (ntdll's base differs, and it changes only at a restart).

### F3. The faulting stack, unwound (OBSERVED in four dumps, the call sites checked in Ghidra)

`Get-I99Stack.ps1` unwinds from the exception context with the modules' unwind data (`.pdata`). The images on disk match the
dumps by timestamp: ntdll 9711 `5fd39a05`, kernel32, and the analysed `DarkSoulsIII.exe`. The four stacks are identical
(`evidence/I-0099/artifacts/stacks.txt`). In Ghidra each return address follows its call: 0x14198EA0E `CALL [RAX+0x10]`,
0x1402D4578 the call of `free`, and 0x141FD672E the call of `HeapFree`.

| # | Return address | Function | What it is |
|---|---|---|---|
| 0-4 | ntdll+0x118095, +0x79265, +0xAE02, +0x120A1A, +0x2796A | ntdll heap | `RtlFreeHeap` path, the failure report |
| 5 | 0x141FD6734 | `FUN_141FD6718` | DS3's CRT `free`: `HeapFree(_DAT_1449536C0, 0, p)` |
| 6 | 0x1402D457D | `FUN_1402D4560` | a scalar deleting destructor: sets vptr `DLUT::DLNonCopyable` (0x14276AE90), then frees `this` if `flags & 1` (the argument is read from the disassembly) |
| 7 | 0x14198EA11 | `FUN_14198E910` | `DLNR3D::SocketChannelManager` vtable slot 0, its network loop. The call at 0x14198EA0E is the lock of its mutex member: slot 2 of `this+0x170`'s vtable, called with `0xFFFFFFFF` |
| 8 | 0x1417FD501 | `FUN_1417FD480` | DS3's thread body: it runs the thread's runnable |
| 9-10 | 0x141FE13BF, 0x141FE1566 | `FUN_141FE13A8`, `FUN_141FE1464` | the CRT thread start (`_beginthreadex`) |
| 11-12 | kernel32+0x2DDA7, ntdll+0x7EE4C | | `BaseThreadInitThunk`, `RtlUserThreadStart` |

The thread is the manager's own `DLKR::DLThread` (created by `FUN_1417E8510`, 64 KB stack). It is not a pool worker and not one
of ours.

### F4. The defect in DS3's code (OBSERVED in the decompilation; the timing window INFERRED)

Correction (2026-10-25, [FIX-PLAN](FIX-PLAN.md) §1): `~DLThread` skips its stop and join for a started thread, so the
destructor never waits for the loop; the defect is that the quit destroys the manager without DS3's own stop
(`FUN_14198E1E0`), and the loop faults at its next pass (about 33 ms) unless the process ends first. Step 2 below is
read with that.

Sources: Ghidra, read-only on `ghidra-mirror-i0066` (exports in `evidence/I-0099/artifacts/ghidra-*.txt`), and the RTTI read
from the executable by `Find-I99Owner.ps1` (`evidence/I-0099/artifacts/rtti-vtables.txt`).

- **Constructor `FUN_14198DD20`.**
  - `+0x98` is a `DLKR::DLThread` (vtable 0x143A02C10, built by `FUN_1417E8510(this+0x98, runnable=this, 0x10000, …)`). It
    runs `this->vt[0]`, the loop `FUN_14198E910`.
  - `+0x170` is a `DLUT::DLLifecycleAdapter<DLKR::DLPlainLightMutex>` (vtable 0x142766008). Its slot 2 (0x141799430) takes a
    timeout, and its slot 4 (0x1417994A0) is the loop's unlock. Reading them as lock and unlock is INFERRED from their use.
- **Destructor `FUN_14198DEF0`** (slot 1, `FUN_14198E080`, is its deleting form) works in this order:
  1. The mutex at `+0x170`: `FUN_1417993F0` deletes its critical section. Then `FUN_141799370` sets its vptr to
     `DLKR::DLPlainLightMutex` (0x1439F76B0), then to `DLUT::DLNonCopyable` (0x14276AE90).
  2. Only then the thread at `+0x98`. When the thread still runs, `FUN_1417E86D0` calls `FUN_1417FD7F0(thread+8)`, then slot 4
     of the thread's own mutex at `thread+0x30` (presumably its unlock), then joins with `FUN_1417FD850(thread+8, INFINITE)`.
- **The window.** Between steps 1 and 2 the loop keeps running and locks `+0x170` at each pass (slot 2, with `-1`).
  1. With the vptr at `DLNonCopyable`'s one-slot table, slot 2 (offset 0x10) reads the next table's slot 0. That is
     0x14276AEA0, `FUN_1402D4560`, the deleting destructor of `DLRF::DLAbstractInvokeContext`, which DS3 shares between
     classes.
  2. Called with `0xFFFFFFFF`, its `flags & 1` holds, so it frees `this+0x170`, an address inside the manager's block.
  3. The heap raises `0xC0000374`. These are frames 5 to 7 of F3.
- **A race.** The fault appears only when the loop reaches its lock inside that window (INFERRED). F5 has the rates.
- **Exit code 0 in every case.** The main thread's `ExitProcess(0)` ends the faulting thread while Windows' error report runs
  (I-0082 §5; the order is INFERRED).

### F5. Thomas's question: does the way the game is closed matter? (OBSERVED, small samples)

Heap faults are counted from the controller's crash record, and from Windows' event 1000 where no record exists.

| How the process ended (since 2026-10-21 13:40, when the controller began recording faults) | Runs | Heap fault |
|---|---|---|
| Native, the game's own quit (gate memory legs and endurance close the window's close box; one menu quit, `i92-11`) | 36 identified | 32. The 4 others (`i88-final-z1`, `i89-final-z1`, `i94-final-z1`, `i96-final-z1`) left neither a record nor an event 1000 |
| Bare original, the game's own quit | 3 | 2: `i92-13-original-quit` (menus: in-game Quit Game, then Quit Game at the title) and `i92-mem-original` (close box, 2026-10-23 02:30:03); none for `i96-01-original` |
| Bare original during Thomas's play (2026-10-19) | 1 known | 1 |
| Ended by our tools (the module's stop and its `ExitProcess`, or `TerminateProcess`) | most of the 2,236 kept runs | none expected: the game's own quit, which destroys the manager, never runs (INFERRED) |

The denominator of 36 may be incomplete: a natural quit without `exit-dump.flag` leaves no controller record, so it is counted
only through Windows' log.

The close box and the menus both raise it. Nothing accumulates between sessions: every launch starts with a fresh heap, and the
stack holds no save-file code. At the menu quits the fault comes after the game has saved and returned to the title. For the
close box this is INFERRED. Damage to the save is not expected (INFERRED: the save is written before this teardown).

### F6. Does our module play a part? (INFERRED)

- The faulting code, its thread and its object are DS3's (F3, F4). The freed address was never the start of a block, so the
  heap did not need to be corrupted beforehand.
- DS3's CRT heap handle `_DAT_1449536C0` was not read here, because the dumps hold no `.data` of the executable. If it is the
  process heap, our module, D3D12 and the driver allocate there too. That does not change the mechanism.
- Our module can change the race's timing (more threads, a busier quit). The rates seen so far are 32 of 36 native and 2 of 3
  for the bare original: too few to compare.
- Page heap (gflags `/full`) and AddressSanitizer were proposed for a wild write. This mechanism does not need them. They stay
  a fallback if the fix leaves a fault.

### F7. Side effects today (OBSERVED)

- Most natural quits leave a Windows error report (754 in 24 days) and, with `exit-dump.flag`, a dump of about 1 to 5 MB.
- The module's fault handler special-cases it (`15_cpu_service_and_input.inc`, `Fault`).
  - A heap corruption seen before the loader's shutdown flag waits 3 s before asking for a dump. Once the process ends, it is
    reduced to a `native_exit_phase_exception` row.
  - A genuine heap corruption during play is therefore dumped 3 s late (I-0082 « Uncertainty »).
  - Without `exit-dump.flag`, one at the very end of a session is not dumped at all.
- Documentation: STATE cited « I-0092 RESULT » for the bare-original evidence (Application Error 1000, ntdll 0x118095), but
  I-0092's RESULT does not describe the `i92-13-original-quit` run. This study does (F1, F5), and STATE now points here.

## Plan for the workstream (proposed, for Thomas's order)

One short tranche, measured and then merged (Thomas's rule of 2026-10-23). The work, in order:

1. **Witness on the bare original first.**
   - One announced live run: offline, hash-checked restore, nothing in the game folder, coordinated with the stutter pilot.
     About 10 minutes.
   - Five quits of the bare original under a watcher that attaches as a debugger (`DebugActiveProcess`, as `startup_trace.cpp`
     already does on DS3). The watcher writes a dump at the first-chance `0xC0000374`, with the thread frozen.
   - Expected: the F3 stack in the original. That makes F4 CONFIRMED for the original and gives its rate on a real sample
     (Thomas's question).
2. **The fix, in the module: DS3's intent without its defect.** Replaced by [FIX-PLAN](FIX-PLAN.md) §2 (the hook moves to
   `~DLNRD::SocketManager` and runs DS3's own stop, bounded); the open points below are answered there.
   - Hook the destructor `FUN_14198DEF0`. Before its first statement, stop and join the `DLThread` at `+0x98` along the member's
     own path, as `FUN_1417E86D0` does: `FUN_1417FD7F0(thread+8)`, slot 4 of `thread+0x30`, then `FUN_1417FD850(thread+8, …)`.
   - Mark the thread as stopped so that `FUN_1417E86D0` does not repeat that path. Setting its byte at `+0x98+0xC0` alone would
     also skip the slot-4 call, and the critical section would then be deleted while held. So the hook must make the same calls
     and then set the byte, or reproduce the destructor without its first block.
   - Then let the original destructor run unchanged. The engine code stays the original's; only the order of the two members
     changes.
   - The join is bounded (for example 2 s). On a timeout the original order is kept and a row is logged, so a quit cannot hang.
   - A/B by `no-exit-order-fix.flag`.
   - Open before coding:
     - read `FUN_1417FD7F0` and `FUN_1417FD850`, and check that the stop ends the loop: nothing read yet links it to the loop's
       tests of `+0x164` and `+0x165`;
     - find who calls the destructor at a quit, because the destroying thread's stack is not in the dumps. The join must not
       be taken under a lock the loop needs.
3. **Measure.**
   - Ten quits with the fix and ten without it: five by the close box and five by the menus each time.
   - Expected with the fix: no `0xC0000374`, no event 1000, and the quit no longer than today (seconds from Quit Game to the
     process exit).
   - The test save loads again after each quit. Thomas's own saves are never written (his rule of 2026-10-23).
4. **Then simplify the fault handler.**
   - Remove the exit-phase special case (the 3 s wait and the « pre-existing » row), so that any `0xC0000374` is a fault that
     is dumped at once.
   - Remove the corresponding lines from STATE, WAVE1-CLOSURE, I-0082 and the ledger (K-0464's last sentence).
   - From then on, the gate's memory leg and the night endurance fail on any heap fault at the quit.

Not proposed: a fast exit at the quit (`TerminateProcess` once the save is written). It is common in games and would hide this
fault, but it skips the original teardown, including any late save write, and it does not fix the cause.

## Limits

- The bare original's stack is not captured. Only its error-report bucket, its module list and its event 1000 are (F1, F5).
- The destroying thread and the reason for the window's length are not read. The lock and unlock meaning of the mutex slots is
  INFERRED from their use.
- 36 native and 3 original natural quits are small samples, and the native denominator may be incomplete (F5).
- `Get-I99Stack.ps1` reads saved registers at the current stack pointer rather than at the frame's base. Return addresses and
  stack pointers are right unless a caller uses a frame pointer restored from such a slot. That does not apply to these frames,
  where the chain ends cleanly at `RtlUserThreadStart`.

## Tools and evidence

- Tools:
  - `Get-I99Stack.ps1`: stack unwinding without a debugger.
  - `Read-I99Minidump.ps1`: a dump's streams, threads and memory ranges.
  - `Find-I99Owner.ps1`: `.pdata` function bounds, RTTI names, vtables and rip-relative code references.
  - `I0099Decompile.java` and `I0099Refs.java`: headless Ghidra, read-only, with `-noanalysis`.
- Evidence in `evidence/I-0099/artifacts/`: `stacks.txt`, `wer-reports.csv`, `run-endings.csv`, `rtti-vtables.txt`,
  `ghidra-stack.txt`, `ghidra-socketchannel.txt` and `ghidra-teardown.txt`.
