# Project state

Updated: 2026-11-23 18:05 (Paris): R's I-0141 tranche 28 merged (`2b76eec52`), start check passed.

This file holds the current position and the resume conditions only (D-0291): method in [WORKFLOW](WORKFLOW.md), the plan in
[ROADMAP](ROADMAP.md), decisions in [IN-FORCE](../decisions/IN-FORCE.md) and the [LOG](../decisions/LOG.md), claims in the
[ledger](../knowledge/LEDGER.md), each result in its investigation ([index](../investigations/README.md)). The earlier texts of
this file are in [archive/2026-11-15/STATE.md](archive/2026-11-15/STATE.md) (the checkpoints since 2026-11-02, one per line)
and in Git tags `archive/pre-cleanup-2026-10-25` and earlier ([ARCHIVE.json](ARCHIVE.json)).

C3 (the native D3D12 backend) is the current capability and is not yet accepted; what is left is the general plan of
[ROADMAP](ROADMAP.md) (D-0273); the formal qualification of the 13 obligations is abandoned (D-0228;
[below](#coverage-of-the-13-production-obligations)). The first milestone (real startup, a genuine zone rendered
independently) was accepted with reserves on 2026-10-19 (D-0194, « jeu jouable en natif »); the final acceptance (the
complete game and DLC without D3D11) stays the mandate's condition.

## Current resume checkpoint

Each row is rewritten at every checkpoint, never appended to, and stays short (`Test-Project` warns over 150 words a row):
a workstream's history goes to its RESULT and the investigations index.

| Item | Current position |
|---|---|
| Pilot branch | R: none at this checkpoint (to delete, merged: `claude/r-i138-t8b` to `claude/r-i138-t10`, `claude/r-i141-t11` to `claude/r-i141-t28` and the empty `claude/r-i141-t23`, `claude/r-i132-r12h2`; S's `claude/confident-hofstadter-e7438c`; their worktrees as `git worktree list` shows them). The tour: none. E: none at this checkpoint (to delete, merged: `claude/e-i140-t22` to `claude/e-i140-t43`, their worktrees as `git worktree list` shows them); workstream 5, [I-0140](../investigations/I-0140-effect-families/RESULT.md). |
| Active workstream | R: workstream 7 step 7, I-0141 tranches 1 to 28 merged (28: 32000010's excess at cuts 1 and 5 is the sun's light at the traced hits; D-0279, D-0282, D-0293, D-0294, D-0297). Workstream 11, [I-0138](../investigations/I-0138-modern-post-chain/RESULT.md): tranches 1 to 10 (D-0285, D-0296, D-0299, D-0290); left the original's lit shape-3 frame in play (ROADMAP). Workstream 2 closed ([I-0132](../investigations/I-0132-rt-optimisation-zones/RESULT.md)). E: workstream 5, [I-0140](../investigations/I-0140-effect-families/RESULT.md) tranches 1 to 43 merged (« Last finished »); step 5's crops closed by Thomas 2026-11-20 (its strips for his eye); left steps 2 to 4 (ROADMAP). Workstreams 4 and 6 closed ([I-0137](../investigations/I-0137-robustness/RESULT.md), [I-0139](../investigations/I-0139-game-tour/RESULT.md)), their left items carried to workstream 16. |
| Second pilot | Two pilots when both are open: R, the RT/PT line (priority on the ASUS and the launcher's rebuilds unless an open-gap fix is ready to test, [WORKFLOW](WORKFLOW.md#live-game-rule)), and E, the open gaps; when both run, E stops after its next clean checkpoint at 50 % of the weekly limit (D-0248). The open points of the RT workstreams closed on 2026-11-09 are in I-0132's R11-PLAN §6. |
| Programme | Wave 1: [I-0066](../investigations/I-0066-independent-native-startup/RESULT.md) « DS3 DX12 Vague 1 », complete (D-0194). Wave 2, the remaster « Dark Souls III: Rekindled » ([ROADMAP](ROADMAP.md), D-0273), current, under its first rule D-0195 (Elden Ring's architecture, without ER's known defects). Static preparations I-0067 to I-0069 complete. |
| Working baseline | `C:/Users/User/Desktop/DS3 Decompilation`, branch `main`. Each workstream runs in its own worktree branch and merges into `main` when its code is written and reviewed (D-0129, D-0198, D-0283). Full production (D-0227): no gate before a merge; after every merge the launcher's module is rebuilt from `main` (unit tests) and the one-minute start check runs (D-0229, `scripts/Invoke-StartCheck.ps1`). |
| What runs natively | DS3 starts, loads saves and plays with the native D3D12 renderer and no D3D11 device: menus, character creation, Bink movies, the world, characters and armour, deferred lighting and the forward rows, SSS, decals, water, effects ([I-0103 CENSUS](../investigations/I-0103-effects-census/CENSUS.md)), our post chain in YEBIS's place (I-0138) with the automatic exposure (D-0264, D-0280), the TAA (DLSS with a chosen mode, D-0265), the frame graph, two frames in flight, the render worker, the screen modes, the pipeline cache, the Agility SDK. By default the rays (D-0253): the traced sun and local lights with their shadows, reflections, water, GI (ReSTIR at the PT level), RR where DLSS runs, SER on the PT level; no raster lighting, no lighting cheat, no fallback (D-0262). Frame generation behind `fg-mode.txt` (off); FMOD audio natively; tessellation behind `native-tess.flag`. |
| Last finished | R: [I-0141](../investigations/I-0141-cutscene-lighting/RESULT.md) tranche 28 (merged `2b76eec52` 2026-11-23, start check passed): 32000010's clear excess in the diffuse GI and the reflections at cuts 1 and 5 is the sun's light at the traced hits, about half each at cut 1 (without it cut 1 122.0 against the original's 121.8); the original's characters get only a small instance SH and a sun mask of 0 the native's hits ignore (OPEN); too bright for Thomas (18:01); the storm's runs differ more than the modes at cut 4, single runs settle nothing; [I-0138](../investigations/I-0138-modern-post-chain/RESULT.md) tranche 10 (merged `bf49a54f1`): the bloom plans seen natively, their strength not settled. Next full control due 2026-11-24. E: [I-0140](../investigations/I-0140-effect-families/RESULT.md) tranches 41 (D-0306, the menu views' TAA; merged `0426072b6` 2026-11-22) to 43 (the portrait's 2 px), their measures in the RESULT. |
| Next action | R: I-0141 « Suggested next step » 3 in a fresh thread: the characters' sun mask at the traced hits first (Thomas: too bright). E: the next I-0140 item in a fresh thread (RESULT « Next » 11): the original's listing rule for the briefly drawn views, its conditions named (tranche 43's « Outcome »), the wobble; the dark bin's half (the cloth's pose, INFERRED); the title's option 2 later; Fire Orb at 2560x1440 (the OLED); step 2's paint; the held and neutral paths. SER's first setup before a first PT frame (about 200 ms). Thomas: his eye in play on the fixed gaps, the mist, Yorshka's brightness (I-0141 t22), Fire Orb and portrait sheets (I-0140 t25, t26), R11c's sheets, I-0138 t10's bloom sheets, the title's embers, the OLED, the shafts' brightness. Live runs: the ASUS (the OLED unplugged, Thomas 2026-11-21). |
| Thomas's sessions | None announced. On « je joue » or « je prends le PC », builds and live runs stop until his « fini » (WORKFLOW). If he asks: the test launcher with `-NoAi` and `-PopinObserve` ([USAGE](../investigations/I-0089-test-tools/USAGE.md)). |
| Open gaps | The 27 gaps' final statuses head [I-0113's RESULT](../investigations/I-0113-open-gaps/RESULT.md): 16 open in workstream 5 (1920x1080 alike by measure, Thomas's eye pending; 2560x1440 not compared; 17 closed by Thomas, 23 fixed under the TAA by I-0140 tranche 12 and validated by Thomas 2026-11-17), with the menus' portrait cases; nine fixed gaps wait for Thomas's eye in play. Launches refused at start: 4 of about 370 native runs. The menu tests (Asian and Cyrillic languages, resolutions) are workstream 14's; the launcher (D-0182) not yet tried live by Thomas. Residuals: WAVE1-LAST-EFFECTS §4, §5 and §8; the families the native skips or draws wrong ([I-0103 CENSUS](../investigations/I-0103-effects-census/CENSUS.md)); step 5's audit (I-0140 tranches 13 to 24, its strips for Thomas's eye): all workstream 5's. Every known or suspected departure from the original, ranked: [STUDY-ALL-GAPS](../investigations/I-0140-effect-families/STUDY-ALL-GAPS.md). |
| Open witnesses | The list is abandoned (D-0228); what still matters is carried by ROADMAP: the movie and NPC dialogue audio by workstream 16 (the tour, closed, did not check it), the pacer by workstream 8, GPU-based validation by workstream 10, resolutions and languages by workstream 14. |
| Live environment | Live runs under D-0117 through `Run-LiveWitness.ps1`; the rules in [WORKFLOW](WORKFLOW.md#live-game-rule), the know-how in [LIVE-RUN-NOTES](LIVE-RUN-NOTES.md). Screens: the ASUS 1920x1080 at 60 Hz (secondary, composed by Windows, no G-SYNC) and the OLED 2560x1440 at 279.96 Hz (primary, Independent Flip, G-SYNC in full screen only); hardware-accelerated GPU scheduling and the optimisations for windowed games on; NVIDIA's overlay frame counter on in the OLED PresentMon runs, the driver's frame limit 277 frames/s, RTSS not running. Night endurance: last `full-1116-night` (2026-11-16, 92 minutes, 54 loads, clean). Windows Defender: no exclusion unless it removes a module again (WORKFLOW); the controller's `Wacatac.B!ml` detection (a false positive, INFERRED) allowed by Thomas by threat ID, machine-wide (2026-11-23). Security: the [audit](studies/security-audit/REPORT.md) merged (`c728e936c`); next its H1 in a fresh thread, with the play-launcher check its « Left » names. |
| Tools | [TOOLS](TOOLS.md); pilots and readers in [I-0066](../investigations/I-0066-independent-native-startup/); `native/frame/Build-GameBootstrap.ps1` builds the module (its commands in parallel by Ninja, `-Jobs`; a fix's build through a thread's object cache, `-Cache <folder> -Out <new folder>`, 9 s to about 3 min without the tests, D-0295; once in seven its module differed, unexplained, [I-0142](../investigations/I-0142-build-speed/RESULT.md)) and runs the unit tests; clangd for `native/` ([WORKFLOW](WORKFLOW.md#native-code-navigation-clangd)). |

## Storage and recovery

Canonical evidence: `C:/Users/User/Desktop/DS3_evidences`, outside Git, exposed by
the ignored `evidence/` junction. In a new checkout run
`pwsh -NoProfile -File scripts/Connect-EvidenceStore.ps1` before using evidence.
Retention keeps I-0050 onward ([EVIDENCE-RETENTION.json](EVIDENCE-RETENTION.json));
never restore removed evidence automatically (historical binary captures of I-0050 to I-0065
were removed on purpose; reports, measurements, comparison images, scripts and the I-0065
capture-host-test fixture stay; I-0066 is excluded). New evidence needs an external backup.
Disk guard (D-0260, 2026-11-08): builds drop their `.obj`, `scripts/Clear-DiskCache.ps1 -Apply` rotates builds and raw
frames at each merge and under 100 GB free, run save backups beyond 100 go to `D:/DS3-save-backups-archive`; the 2026-11-08
cleanup (about 394 GB of evidence and 55.5 GB of run save backups) is in EVIDENCE-RETENTION.
Retired-worktree evidence is under `_retired-worktrees/2026-10-08/` in the external store
(provenance in its `index.json`); identical archived files are deduplicated by SHA-256,
unique files consolidated, differing versions preserved. Investigations I-0001 to I-0065 are in
the archive tag only; read them with `git show` when a question needs them, never
restore them into `main`.

## Coverage of the 13 production obligations

The formal qualification of these 13 obligations is abandoned (D-0228, FINISH-LINE) and production acceptance stays unmet. The table is kept as what the release checklist of ROADMAP workstream 16 must cover; it leaves this file when that checklist is written (D-0273). Native in-game execution is reached
for startup, menus, loading, world, characters, lighting, effects and presentation;
complete normal-game and DLC coverage, display modes, transitions and streaming remain
to qualify. Claim scope is in the [ledger](../knowledge/LEDGER.md).

| # | Obligation | Remaining qualification |
|---|---|---|
| 1 | Process startup and rendering entry | Complete entry routes. |
| 2 | Window/configuration, input, lifecycle | Full screen, borderless and resolution changes natively (I-0076); other aspect ratios (D-0170); full lifecycle. |
| 3 | Engine graphics service contract | Complete direct contract. |
| 4 | Device, queues, presentation startup | Complete startup envelope. |
| 5 | CPU loading, progression, cancellation | Complete progression and cancellation. |
| 6 | Models, materials, shaders, textures | Every family at every witness zone; the pipeline cache is in place (I-0072), a pipeline's first encounter on a PC still compiles. |
| 7 | Menus, UI, fonts, movies | Other languages, end credits, phantom variants ([MENUS-NATIVE-GAPS](../investigations/I-0067-static-menus-subsystems-map/MENUS-NATIVE-GAPS.md)), whole-menu audit (D-0170). |
| 8 | Timing, input, camera, world update | Pacer under D-0167/D-0169. |
| 9 | Scene preparation, culling, scheduling | Native selected-zone execution at every witness zone. |
| 10 | Effects, passes, composition | Wave-2 shadowed local lights and tessellation passes; native effect contracts (D-0190). |
| 11 | Transitions, scene changes, streaming | Independent native coverage. |
| 12 | Presentation, resize, repeated frames | Witnessed on one 1920x1080 monitor (I-0076); 1440p, 4K, an odd resolution, two monitors (D-0170). |
| 13 | Shutdown, drain, retirement, window release | Complete lifecycle and resource retirement. |

Use PowerShell 7 for `scripts/Test-Project.ps1`.
