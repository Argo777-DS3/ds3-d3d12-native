# I-0099 result: Plantage à la fermeture (the exit heap fault study)

## Conclusion

Complete as a study (2026-10-24, read-only, nothing built or run). The heap fault `0xC0000374` at the quit is DS3's own
defect, and our module neither causes it nor, by any trace, worsens it. The details are in [STUDY](STUDY.md).

DS3's destructor of `DLNR3D::SocketChannelManager` destroys its mutex member before it stops and joins its network thread.
If that thread locks the destroyed mutex in the window, the call lands on a deleting destructor that frees an address
inside the manager's block, and the heap raises the fault. The stack is the same in four dumps across three trees, with no
frame of ours. The decompiled destructor shows the order. The bare original raises the same fault, at a menu quit and at a close-box quit.

The window is a race: 32 of 36 native natural quits and 2 of 3 bare-original ones raised it. Menu quits raise it too, so the way the game is closed
does not matter, and nothing accumulates between sessions (Thomas's question, STUDY F5).

## Suggested next step

The workstream of [FIX-PLAN](FIX-PLAN.md) (2026-10-25, read-only: the fix and the test protocol checked against the code and
the dumps; it corrects STUDY F4's order and replaces step 2). STUDY's original plan:

1. Witness the original's stack and rate with a debugger-attached watcher (one announced live run).
2. In the module, stop and join the manager's thread before its destructor, with a bounded join and A/B by
   `no-exit-order-fix.flag`.
3. Ten quits with and ten without the fix.
4. Remove the fault handler's exit-phase special case, and the « pre-existing » notes from STATE, WAVE1-CLOSURE, I-0082 and
   K-0464.

## Evidence

`evidence/I-0099/artifacts/` (`stacks.txt`, `wer-reports.csv`, `run-endings.csv`, `ghidra-*.txt`). The dumps are in
`evidence/I-0066/artifacts/*/crashes/`.

## The fix (2026-10-28, FINISH-LINE line 2)

Re-read by the pilot with GhidraMCP (Thomas's Ghidra 12.1.4 on the copy `ghidra-mcp-ds3`, read-only) before the code, all as
FIX-PLAN §1 says (VERIFIED): `~DLNRD::SocketManager` 0x1419853A0 (its first 19 bytes position-independent; it releases the
channels at +0x1F8, then calls `~SocketChannelManager` at this+0x50, 0x1419854CC, with no stop); its only caller the scalar
deleting destructor 0x141985520, reached through slot 1 by the singleton list's destroyer 0x141986250; `Stop` 0x14198E1E0 (the
+0x170 mutex through slots 2 with -1 and 4, +0x164 cleared, then the join of the DLThread at +0x98); `~DLThread` 0x1417E86D0
(the join only when +0xC0 is 0, a thread never started); the wait wrapper 0x1417FD850 (the handle at the DLThread's +8); the
loop's test of +0x164 at 0x14198ECFD.

Written (`15_cpu_service_and_input.inc`, `StopNetworkThreadThenDestroy`; installed in `22_setup.inc`, native mode only, inert
under `no-exit-order-fix.flag`; one new pin 0x19853A0): at the destructor's entry, when the DLThread was started and its handle
does not signal at once, Stop's first block on DS3's critical-section mutex (accepted when its TryEnter and Leave slots are
0x141799470 and 0x1417994A0 and its initialised byte +0x30 is set; taken through TryEnter for at most 500 ms instead of
Stop's unbounded slot 2, +0x164 cleared, released through Leave), then a join
bounded at 2 s and one row `native_exit_network_stop` (result, milliseconds, the thread, whether it is the engine's owner
thread); then the original destructor, unchanged. Not done, as the FIX-PLAN says: Stop's channel release after its join, the
error byte +0x165.

NOT RUN before its merge (D-0229): FIX-PLAN §4's A/B (10 quits with, 10 without) is not made, and the fault
handler's exit-phase special case (STUDY step 4) stays until a quit with the fix is seen (seen five times since, K-0484; removed 2026-10-29, last section). Evidence will come from the start
checks after each merge (`Invoke-StartCheck.ps1` closes the game by its close box) and the natural quits. Reading: every quit
with the fix writes one `native_exit_network_stop` row (no row means the hook did not run); « signalled » with no `0xC0000374`
is one consistent quit, and since about 4 of 36 native quits were already clean without the fix, the fix is VERIFIED only
after 5 such quits in a row; a « skipped_* », « lock_timeout » or « timeout » row, or a fault after « signalled », sends it
back to analysis.

Step review (fresh Opus, 2026-10-28; the offsets checked against the executable with llvm-objdump): no high. Answered: the lock
bounded (TryEnter slot 3 for 500 ms, re-read with GhidraMCP: 0x141799470 returns 0 when taken; the row says `lock_timeout`
otherwise) (L1; its table test was wrong, corrected after the final review below); `no-exit-order-fix.flag` in Run-StartupTrace's stale-flag
list (L5); `Invoke-StartCheck.ps1` refuses an existing run folder or a running game (M1), passes only when the game left by
itself after the close (M2), reports setup refusals and the controller's ready value (L2), checks the window and the posted
close (L3), refuses a screen not attached (L4).

Final review (fresh Opus xhigh, 2026-10-28), one high: H1, the hook compared the mutex's table with 0x1439F76B0, the base
class `DLKR::DLPlainLightMutex`'s, which a live channel manager's mutex holds only inside its base constructor: the channel
manager's constructor overwrites it with 0x142766008 (`DLUT::DLLifecycleAdapter<DLKR::DLPlainLightMutex>`, 0x14198DE35,
re-read with GhidraMCP), so the fix never ran. Answered: the mutex is accepted by its TryEnter and Leave slots (both tables
share them, 0x142766008 read) and its initialised byte +0x30 (cleared by 0x1417993F0, read); every quit now writes one row,
a skip with its reason (`skipped_unreadable`, `skipped_not_started`, `skipped_mutex`, `skipped_ended`). Also answered: the
start check passes only when the controller neither stopped nor killed the game (M1, its exit code reported; the
controller's bound 110 s, the world awaited 90 s); the reading's sample (L1, above); the status title's encoding (L2); the
A/B flag accepted by Run-StartupTrace's `-EffectFlags` (L4); the start check's screen resolved by its key, the ASUS
`1920x1080@60` (L5); the announcement and a failed start check in WORKFLOW (L6, L8). Left: L9 (the module's hash not compared
with the commit's build), L10 (the velocity and reflection pictures every native run writes, 22 MB a start check: FINISH-LINE
2 bis). Built after the answers into `%TEMP%/claude/i99-build` (unit tests passed, 04:18).

Step review of these answers (fresh Opus, 2026-10-28): the hook meets its criteria (both tables share slots 1 to 4, the
destroyed mutex's table 0x14276AE90 has others; every call target checked; one row before the original on every path). One
high in the start check, answered: its kill codes were written as wrong decimals (0xD2126611/12), now 3541198353 and 3541198354
(0xD3126611/12, checked), so a game the controller stops at its bound fails the check; the comment says which code is which
(L1); a crash at the quit other than 0xC0000374 fails it and is reported (L2). Noted (L3): after a « timeout » row the network
thread may leave its loop while the original destructor runs, an interleaving the unpatched game never has; that row already
sends the fix back to analysis.

### The first witness: the start check after the merge (`start-9c2ec9e1`, 2026-10-28 04:40-04:41, the ASUS)

`scripts/Invoke-StartCheck.ps1` on the launcher's module rebuilt from `main` 9c2ec9e1: the world drawn (363 frames), the close
box posted at 04:41:03 and the game gone by itself, controller exit 0, no refusal, failure or crash, `restoration ok`. OBSERVED:
`native_exit_network_stop` « signalled », 31 ms, on the engine's owner thread; no `native_exit_phase_exception` (the run before
the fix, `start-af037cde`, had one 0xC0000374). One consistent quit of the five the reading asks for (SUPPORTED, not VERIFIED).

Second witness: `start-92d77dd7` (2026-10-28 06:09, after I-0102 tranche 1's merge): « signalled » in 15 ms on the owner
thread, no 0xC0000374. Two consistent quits in a row of the five the reading asks for.

Third witness: `start-6a3b127a` (2026-10-28 07:30, after I-0102 lot 2a's merge): « signalled » in 16 ms on the owner thread, no
0xC0000374. Three consistent quits in a row of the five the reading asks for.

Fourth witness: `start-e3cd8b56` (2026-10-28 09:15, after I-0102 lots 2b, 2c and 2e's merge): « signalled » in 16 ms on the
owner thread, no 0xC0000374. Four consistent quits in a row of the five the reading asks for.

Fifth witness: `start-4f15f9f7` (2026-10-28 09:23, a launch and quit Thomas allowed for it, 08:16 relayed; the module of
`e3cd8b56`, nothing changed in `native/`): « signalled » in 15 ms on the owner thread, no 0xC0000374. Five consistent quits in a
row: the fix is VERIFIED by the reading above (ledger K-0484, `CONFIRMED`).

## STUDY step 4 (2026-10-29)

Step 4 done 2026-10-29 (the I-0105 pilot's branch): the handler's exit-phase case, its 3 s wait, `native_exit_phase_exception` and `exit-dump.flag` removed; `Invoke-StartCheck.ps1` fails on any crash at the quit, the heap fault included, and the gate's memory leg and the endurance summary count the controller's crash record (the game's own crash row is lost with its thread at a quit: final review M1); the endurance runner's comments updated; the Reference (the unmodified original, which still raises the fault) logs a heap corruption as `reference_heap_corruption` instead of dumping it; D-0201 amended; K-0464, WAVE1-CLOSURE and I-0082 DESIGN marked. Step review (fresh Opus, high, `96407cc2..e4b86d16`): its M1 (Reference quits dumped) and M2 (the memory leg blind to a crash dump) applied, its lows applied. Final review (fresh Opus, xhigh, `96407cc2...09560682`): its M1 (the memory leg blind to a quit's dump) and lows applied.
