# I-0124 - The game's exit fault in 141A3B670 at a close, and the endurance tour's quit not opened (La faute à la fermeture)

Capability: C3

## Question

Why does a DS3 worker thread reach the engine's abort handler (`0x140F14C52`, from CRT `abort` `141FD0EAC`, called by
`141A3B670`+0x184 on the thread run by `141A39FC0`) when the game is closed by its close box with the player in the world, a fault
seen three times now (I-0114 `i114-st4-n60`, I-0115 `i115-hall-n60`, the full control's `full-1103-night`), and is it DS3's own
exit-order defect (as I-0099's) or a state the module leaves; and why did the endurance tour's quit menu not open at Anor Londo on
its twelfth visit (the cause of that close)?

## Scope

Included: static decompilation of the abort path and its thread (read-only), the dump of `full-1103-night`, the earlier dumps if
kept, the tour's journal and pictures; a fix in the module when the cause is the module's, or the bounded witnesses I-0099's method
used (the bare original's close from the world) when it is DS3's; the tour's quit made robust if it is the pilot's.

Excluded: I-0099's heap fault (its own workstream, FIX-PLAN), anything in the rays.

## Inputs

- `evidence/I-0066/artifacts/full-1103-night/` (crash report, dump, journal, tour, shots); the D-0230 full control of 2026-11-03.
- I-0099 (STUDY, FIX-PLAN, its minidump readers), I-0066 D3D12-R13-R14 sheet (the same abort site at a process exit, a module
  static destructor then), I-0114 and I-0115 RESULT (the fault seen at a close).

## Hypotheses

- H1: a DS3 thread outlives an object the exit sequence destroys (DS3's own order, as I-0099's), independent of the module.
- H2: the module's exit work (the network stop, a release into engine memory) removes something the thread still uses.
- H3: the tour's quit failed on input focus or a game state (the bonfire prompt), not on the renderer.

## Expected evidence

The decompiled check that calls abort, the object it reads in the dump, the order of the exit; for H3, the tour's inputs and the
journal around visit 12.

## Exit conditions

- Success: the cause named with its evidence and either fixed (the module's) or documented with the original's witness (DS3's).
- Refuted / insufficient information: recorded as such, with the next bounded witness.
- Stagnation: no new discriminating evidence after 3 cycles.
- Budget exhausted: close the tranche and replan under `.project/WORKFLOW.md`.

## Safety and human interaction

Static work first; live witnesses announced (D-0116), offline, restored by hash, the ASUS while Thomas is away.
