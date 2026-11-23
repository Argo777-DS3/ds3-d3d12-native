# Code cleanup before the wave-1 closure (D-0191)

Workstream « Nettoyage du code », 2026-10-19, branch `claude/hungry-leakey-6c71bf` from `main` 49d70bf2. Owner Opus 5.5.

Rule (coordinator brief and Thomas, project chat 2026-10-19 06:50 Paris): no change of behaviour or of picture; dead code,
diagnostics and duplicate paths removed; responsibilities clarified by mechanical moves of blocks as they are, no rewrite and no
renderer redesign (that is wave 2). Proof before the merge: `Test-Project.ps1`, the m45 collect and p99 not degraded, the
pictures side by side at the witness zones, one Opus review.

## 1. `game_bootstrap.cpp` split into 22 parts (372e5873)

Observation: the module's 18,569-line file was one anonymous namespace, definitions in dependency order.

Change: the namespace's 18,459 lines are cut, unchanged and in order, into `native/frame/game_bootstrap/NN_name.inc` by
responsibility (state, frame lifecycle, engine CPU owners, reference witnesses, selected scene, pass programmes, scene lighting,
model programmes, model admission, options and passes, YEBIS host, collect, model frame, scene step and storage, CPU service and
input, offscreen and movies, render step, resource admission, UI witnesses, startup phases, effects, setup). The includes move to
`game_bootstrap_includes.h`; `game_bootstrap.cpp` keeps the include order and `DllMain`. It stays one translation unit: the same
inlining and the same static initialisation order as before (separate translation units would move code across calls the
optimiser inlines today, on the collect's hot path: a wave-2 design question, not a cleanup). Each part includes the one before
it (`#pragma once`), so clangd checks any part on its own: 0 diagnostics on parts 01, 09 and 22.

Proof: `scripts/Compare-NativeBinary.ps1 -Objects` compares two COFF objects section by section, independent of the order in
which the compiler emitted them (COMDAT sections by their symbol, the other data sections cut by their symbols, relocations by
target symbol and offset, debug information excluded). Against `main`'s `game_bootstrap.obj`: 75,154 sections, 0 differences.
The DLL itself is not byte-identical: the compiler emits the read-only data in another order once the source spans several files
(observed: two name tables swapped places in `.rdata`, and its static-guard numbers `?$S<n>` follow the file structure), which
moves addresses but no instruction or value.

`scripts/Test-ThreadAffinity.ps1` reads the parts as one text and keys its findings by part; nine allow-list entries that no
longer matched any code (stale before this workstream) are dropped.

## 2. Dead code, dead flags and closed A/B switches

Inputs: the flag inventory of every `*.flag` read under `native/` (86 names: what each does, which runner places it, which
documents cite it), and clang's unused-code warnings over the module (`-Wunused-function -Wunused-variable
-Wunused-member-function -Wunused-const-variable -Wunreachable-code ...`: one unused function, one unused member, one unused
variable; nothing unreachable).

Removed (the default path of each is unchanged; the flags are no longer read and no runner places them):

| Flag or item | What it kept | Why it goes |
|---|---|---|
| `sun-shadow.flag`, `shadow-fit.flag`, `shadow-atlas.flag` | Opt-ins of the sun shadow, its fit and atlas | Redundant since D-0180 C4 made them the default (`!no-sun-shadow`); they only differed together with `no-sun-shadow.flag` |
| `engine-instance-rows.flag` | The engine's zero instance light rows (the m33 darkening) | Optimisation-review A/B, closed (OPTIMISATION-REVIEW 1.4) |
| `fade-geometry-pass.flag` | Every walk weighing fade pass 5 | Optimisation-review A/B, closed (8.4) |
| `no-native-sh.flag`, `no-probe-layout.flag` | The device-absent probe SH and the other probe layout | Optimisation-review A/B, closed |
| `post-layout-drains.flag` | The mid-frame `WaitPostLayout` drains | Optimisation-review axis 2, closed; the stages renew their heaps; the `WaitPostLayout` slot stays so the audit row keeps its columns |
| `ffx-default-null.flag`, `ffx-viewport-zero.flag` | Effect A/B witnesses (FFX-DRAW) | Closed witnesses |
| `no-forward-gb5.flag` | Every forward row on Forward1 | Glow wave A/B, closed (GLOW-WAVE1-READY) |
| `no-water.flag` | The former rows before the water rows 0x24/0x27 | Water wave A/B, closed; no runner could place it |
| `no-ao-passes.flag`, `texture-fallback-off.flag` | AO stage inert; no texture-choice keep | No runner, no document; hand-placed A/B only |
| `legacy-upload-geometry.flag`, `legacy-dynamic-streams.flag` | Static FLVER in UPLOAD memory; one committed buffer per stream update | Phase D A/B, closed; runner switches `-LegacyGeometry`, `-LegacyDynamicStreams` removed (Run-StartupTrace, Run-PilotPair) |
| `legacy-tables.flag` | Every draw on the per-draw tables | Phase D A/B, closed; the per-draw path stays for the draws that do not fit and as the unit tests' hook; `-LegacyTables` removed |
| `NativeYebisRecordName`, `NativePlannedPrograms::clear`, `missing_owner` | Nothing (unused) | clang `-Wunused-*` |

The runners' allow-list (`Run-StartupTrace.ps1 -EffectFlags`) loses these names and the names it accepted that the module no
longer read (`yebis`, `yebis-<pass>`, `ffx-execute`, `ffx-gpu`, `water-reflection`); its stale-file removal keeps every former
name and now also lists four flags the module still reads that it missed (`all-groups`, `chr-positions`, `no-water-reflection`,
`pose-observe`: a file left by a killed run would have changed the next default run). Two comments named runner parameters that never existed
(`-NoGamma`, `-NoTextureFallbackKeep`): corrected or removed.

Kept on purpose (live tools, not dead): the measurement and witness flags (`measure`, `sample-profile*`, `*-observe`, captures,
`world-pictures`), the opt-outs of default features (`no-yebis*`, `no-sun-shadow`, `no-glow`, `no-aa`, `no-water-reflection`,
`no-native-effects`, `no-ffx-*`, `no-pipeline-library`, `no-gamma`, `no-debug-layer`, and the hand-placed `no-ao`, `no-ao-resources`,
`no-paint` and `audio-census`, documented as their workstreams' methods), the opt-ins still in development
(`native-tess`, `shadow-local`, `ffx-model-particles`, `ui-cache`, `yebis-host`), and the oracles and fallbacks the wave-2
workstreams need (the collect and prepare rebuilds and their shadows `rebuild-collect`, `collect-shadow`, `rebuild-prepare`,
`prepare-shadow`, `prepare-dryrun`, the persistence switches `no-model-records`, `no-persistent-draws`, the threading and
presentation switches `inline-render`, `inline-prepare`, `one-frame-slot`, `no-waitable-present`, `legacy-present`, and
`hand-barriers` with `graph-shadow`).

## 3. Proof before the merge

- **Build and controls**: `Build-GameBootstrap.ps1` (own folder `code-cleanup-build`), 98 PASS rows, exit 0; `Test-Project.ps1`
  PASS; `Test-ThreadAffinity.ps1` PASS (64 statics reviewed, none stale).
- **Pictures** (Native2286-2293, the witness zone saves, `Run-ZoneWalk -Measure -NoDebugLayer -NoKProfile -Shots 1`, against the
  previous merge's Native2279-2285 and the original's Reference2246-2256, 4x3 luminance regions of the game window): every
  region within 1.00 +- 0.01 of the previous merge except animated sky and effect regions (m37 r0c1 0.94, m31 r0c1-r0c2 0.97,
  m50 r0c0 0.91, m30 r1c0 0.89), where the cleanup is as near the original as the previous merge or nearer (m50 r0c0 1.00
  against 1.11, m30 r1c0 1.02 against 1.15): run-to-run motion of clouds, embers and fog, not a change. m33 (Native2285 was
  captured at another window size) compared by eye: the same picture. Sheets kept in the session scratchpad (not evidence).
- **m45 collect and frame time** (eight runs of the same save and protocol with `-StaySeconds 120`, alternating the cleanup
  build and `main`'s build of 49d70bf2, in both orders: Native2294-2301, 68-71 pacing windows each after the first three):

  | Build | Runs | Collect ms (median per run) | Frames/s | Slowest frame of a window, p90 (ms) |
  |---|---|---|---|---|
  | main 49d70bf2 | 2295, 2297, 2298, 2300 | 11.80, 11.89, 11.97, 12.06 | 57.1, 56.8, 56.5, 56.0 | 25.6, 26.5, 25.2, 26.6 |
  | cleanup | 2294, 2296, 2299, 2301 | 12.29, 12.02, 12.11, 12.09 | 55.2, 56.3, 55.8, 56.1 | 34.3, 24.2, 26.1, 25.3 |

  Conclusion: the frame time is not degraded (p90 25.2-26.6 against 24.2-26.1 apart from the cleanup's first run, the first
  m45 load after the zone series). The collect is 0.1-0.2 ms higher in the medians (1-2 %), inside the spread of `main`'s own
  runs (0.26 ms); the split's code is proved identical and the removals only take tests out of the collect path, so the
  difference is INFERRED to be run-to-run noise or code placement (the data layout of the DLL moved), not added work.

- **Untended Graves** (Thomas, 2026-10-19 07:27: the dark Cemetery of Ash, never witnessed): `Run-AlphaCast -BonfirePages
  -TravelPress "263,191;388,380"` from Alpha's copy (the Cemetery of Ash tab icon, then the « Untended Graves » row; the tab
  arrow at 1100,191 moves one tab left and is not reliable: Reference2302). Reference2306, Native2307 (cleanup), Native2308
  (main): the cleanup equals main in every region (0.99-1.00); native against the original 0.99-1.05 except the ravine's sky
  r0c2 1.14 (the same on main).
- **Found on the way** (a press that missed the tab: the « Dancer of the Boreal Valley » bonfire of High Wall of Lothric,
  Reference2303, Native2304 cleanup, Native2310 main through `-TravelPress "323,191;400,380"`): the cleanup equals main (1.00);
  natively the hall's floor and carpet and the lower half are much darker than the original's (regions 0.45-0.79): a
  pre-existing wave-1 gap, recorded in STATE, not chased here.

## 4. Pre-merge review (Opus 5.5 agent, D-0176, 2026-10-19 07:50)

Verdict: mergeable; no change of behaviour or picture found, the split's proof holds (concatenated parts byte-identical to
`main`'s lines, every part included once, in order). Applied (b8c5f6cb): the stage headers no longer promise a caller drain
(`glare_stage.h`, `grading_stage.h`, `ao_stage.h`, `sun_shadow_stage.h`; three test messages), `Compare-NativeBinary.ps1`
honours the relocation overflow flag (the proof reran: 0 differences), `Get-NativeReads.ps1` (I-0067) scans the `.inc` parts,
the runner's stale-file cleanup gains four flags still read, comments naming the former `yebis.flag` and `water-reflection.flag`
opt-ins corrected. Left as notes: the comparer compares a relocation to a section symbol by its addend only and trims up to 15
trailing zero bytes per data chunk (both cannot hide a change of this split, whose source text is identical); the startup rows
of the removed switches are gone from the traces (no script reads them).
