---
name: merge-workstream
description: Close a step, a tranche or a workstream of the DS3 project and merge it into main (optimisation pass, the documentation cleanup, reviews on the final tree, Test-Project, the launcher's rebuild and the one-minute start check, the clean checkpoint and the fresh thread). Use when a tranche or a workstream meets its exit criteria.
---

# Merge into `main`

The rules are WORKFLOW « Merging into `main` » and « Threads and continuity ». A tranche (or a step whose merge its plan names)
merges this way without Thomas; any other mid-workstream merge needs him. In order:

1. **Commit** the branch's work with RESULT, STATE, ledger and decision updates as the evidence justifies. The `step-reviewer`
   subagent (fresh Opus, high) has reviewed each step on the way.
2. **Optimisation pass** over the branch's additions, read: duplicate paths and repeated work removed (measured only on Thomas's
   request, D-0227). A visual workstream: the side-by-side at every witness zone shown to Thomas (D-0181).
3. **Merge `main` into the branch** (`git merge main`): the reviewed tree is the tree that lands. Then **the documentation
   cleanup before the merge** (Thomas, 2026-11-15, D-0291), on that tree: every document the branch touched cleaned (WORKFLOW
   « Threads and continuity »): STATE's rows rewritten to the current position and its « Updated » line naming this checkpoint
   alone, a finished step one line in ROADMAP and the investigations index (its account in the RESULT), each new decision's
   line in `decisions/IN-FORCE.md`, stale, contradictory and duplicated sentences removed; `Test-Project` without a STATE or
   size warning (a block of history a file must lose goes to `.project/archive/<date>/`, never deleted without Thomas).
4. **Full review on that final tree** (skill `review-final`): the `final-reviewer` subagent (fresh Opus, xhigh) on `git diff main...HEAD`, given only
   the diff, the goal (the CONTRACT) and the exit criteria, briefed to find what is wrong, never the pilot's summary of what it did
   or fixed (D-0283). Fix what it finds or record why not; note the
   review and its outcome in the RESULT. A fix that changes code gets a step review of its own. Then the `integrity-reviewer`
   subagent (WORKFLOW « Guarding the judge », D-0268) on the same diff with the RESULT section: weakened tests, special cases for one
   scene, work cut during a measure, numbers without evidence; answered or recorded in the RESULT the same way.
5. **No measurement gate** (D-0227, full production): no replayed path gate, calibration, memory leg or night endurance.
6. **Validate**: `pwsh -NoProfile -File scripts/Test-Project.ps1 -MergeGate` passes on the branch.
7. **Merge** in the main checkout (`git -C "C:/Users/User/Desktop/DS3 Decompilation" merge --no-ff <branch>`), with no game
   running and the main checkout clean. Then `Test-Project` from the main checkout.
8. **After the merge**: rebuild the launcher's module in the main checkout while no game runs
   (`native/frame/Build-GameBootstrap.ps1`, its unit tests included, default output `evidence/I-0066/artifacts/game-bootstrap-build`);
   **the start check** (D-0229): announce it in one line (5 minutes ahead, « PC libre » after), then `pwsh -NoProfile -File scripts/Invoke-StartCheck.ps1 -Commit
   <main's short hash>` (one minute aimed at, two at most: launched, to the world, closed; offline, restored by hash, the ASUS;
   no measure), `passed` true (a failed check is fixed on a new branch before the next workstream, Thomas told); no live run before the merge.
9. **Clean checkpoint**: the documentation cleanup again for what the merge changed (step 3; links checked, `Test-Project`
   without a STATE or size warning); STATE rewritten short (« Pilot branch | none » and
   « Active workstream | None open » only when the workstream is finished; otherwise the workstream stays named and the next
   thread writes its own branch; « Last finished », « Next action »); the worktrees and folders your own subagents made deleted;
   the disk rotation (D-0260) from the main checkout, no game or build running: `pwsh -NoProfile -File scripts/Clear-DiskCache.ps1
   -Apply` (build folders older than 48 h and runs' raw frames older than 7 days; never the launcher's build, a folder the
   repository links into or holding a report, a `keep.txt`, a line of the store's `_maintenance/rotation-keep.txt`; its line in the RESULT);
   a commit; the weekly limit read (the session's usage tool, `mcp__ccd_session_mgmt__get_usage`, or the app's usage card) and its percentage in the checkpoint line sent to the coordinator (D-0292).
   Other branches and worktrees are deleted by Thomas, from a command you give him.
10. **Fresh thread** for the next step or workstream (D-0215); if it cannot open yet, go on here and change at the next
    checkpoint.
