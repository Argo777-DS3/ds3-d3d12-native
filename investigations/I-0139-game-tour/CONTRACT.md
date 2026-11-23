# I-0139 - Le tour de tout le jeu en natif

Capability: C3. ROADMAP workstream 6 (« The tour of the whole game », plan, D-0273). Opened 2026-11-12 on the coordinator's
note relaying Thomas's go; branch `claude/kind-antonelli-18b0e1`.

## Question

Over a closed list of everything the game shows (every zone and bonfire, every boss with its entry, phases and death, the
scripted events, the cutscenes, the NPCs and merchants, every menu, the endings and credits, the arrivals in both DLC), does
the native renderer show each row with no fault, no skipped family in the journal, and a picture right beside the original's?

## Scope

Included:

- The closed list itself ([LIST.md](LIST.md), built from the game's data and the repository's records), each row with the way
  the test launcher reaches it (a test save, a Travel tab and row, a teleport, a revived boss, a flag, a pilot's keys).
- The runs that visit the rows (the ASUS, at night, every slot through the coordinator), and for each row the three answers:
  a fault, a skipped family in the journal, the picture against the original.
- Each defect filed to ROADMAP workstream 4 (robustness), 5 (families not yet native) or 7 (RT/PT leftovers), with a report to
  the coordinator.

Excluded:

- Fixing the defects (the receiving workstreams fix them); a measure of cadence or cost (workstreams 8 and 15); a real
  playthrough (Thomas's own stays the final judge, ROADMAP limit).
- New test tools beyond what a row cannot be reached without (each one named in the list first).

## Inputs

- Claims and records: [I-0089 USAGE](../I-0089-test-tools/USAGE.md) and [boss-flags.json](../I-0089-test-tools/boss-flags.json)
  (the test launcher, the 26 boss flags); I-0132 [R11-PLAN](../I-0132-rt-optimisation-zones/R11-PLAN.md) §12 (Eta's fourteen
  Travel tabs, `Run-R11cTour.ps1`); I-0069 [REMO-CUTSCENES-NATIVE](../I-0069-static-menu-audio-map/REMO-CUTSCENES-NATIVE.md),
  [ENDING-CREDITS-NATIVE](../I-0069-static-menu-audio-map/ENDING-CREDITS-NATIVE.md),
  [BINK-MOVIES-NATIVE](../I-0069-static-menu-audio-map/BINK-MOVIES-NATIVE.md); I-0067
  [MENUS-MAP](../I-0067-static-menus-subsystems-map/MENUS-MAP.md) and
  [MENUS-NATIVE-GAPS](../I-0067-static-menus-subsystems-map/MENUS-NATIVE-GAPS.md); LIVE-RUN-NOTES.
- Evidence: `evidence/I-0069/queries/` (EMEVD cutscene and credits queries), `evidence/I-0067/queries/`, the R11c tour's
  journals and test saves (`eta-r11c-<zone>`).
- Tools: SoulsFormats 1.0.1 and BinderTool (the pinned read-only extraction of I-0045/I-0068), the runners of I-0066 and I-0132.

## Hypotheses

- H1: the zones' tour of R11c (ten zones at eleven bonfires, no fault) extends to every bonfire with no fault.
- H2: the rows never visited natively (boss phases and deaths, cutscenes, endings, credits, merchants, most menus) hold
  defects that the zones' tour could not show.

## Expected evidence

For each row: its run's journal (faults, `prefix_refused`, skip rows by family, unserved frames), its pictures (the
lossless world pictures where the look matters) beside the original's at the same place, and the row's verdict with its
evidence path. A row that the launcher cannot reach is recorded as such, with what it would need.

## Exit conditions

- Success: every row of the closed list visited, each with its three answers and its defects filed.
- Refuted: not applicable (a list, not a claim); H1 and H2 are answered by the rows.
- Insufficient information: a row that cannot be reached by the launcher without a new tool stays « not reached », named.
- Stagnation: two nights with no row closed.
- Budget exhausted: close the tranche and replan under `.project/WORKFLOW.md`.

## Safety and human interaction

Live runs on Thomas's PC under D-0116/D-0117: each slot asked of the coordinator, announced with start and end times (Paris),
offline, saves restored by hash, « PC libre » at the end; Alpha (Thomas's real game) never modified; the ASUS alone when
Thomas is away (the OLED never left on). No push; `integrity-reviewer` before any merge, and a merge only on the
coordinator's go.
