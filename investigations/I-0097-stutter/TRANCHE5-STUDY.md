# I-0097 tranche 5 preparation study: replayed paths beyond Firelink

Read-only study (2026-10-25 00:55-01:40 Paris, branch `claude/inspiring-ardinghelli-f9e964`), written while tranche 4
(`claude/i97-tranche4`) runs its gate: no game, no Ghidra, no build was started; only journals already in the Evidence Store
were read. Inputs: STATE, ROADMAP (C3 stability, « Replayed paths beyond Firelink », Thomas 2026-10-24 23:30), D-0174,
D-0197, D-0209 to D-0212, K-0481, I-0097 RESULT (tranche 4 as on its branch at `bcffacf9`), I-0070 RESULT, I-0083 CONTRACT,
the pop-in rule study [`POPIN-RULE-STUDY-2026-10-24.md`](../I-0100-popin/POPIN-RULE-STUDY-2026-10-24.md). Statuses: OBSERVED (read in a journal here), INFERRED,
GENERAL (general knowledge, not checked here). Nothing here is promoted to the ledger.

## 1. What the Firelink path already says, split by cycle (OBSERVED, new)

`Get-I97Cycles.ps1` (this branch) cuts a replayed run by the path's own cycles (8 walk cycles of 10 s, 4 sprint cycles of
10 s, 2 turns of 6 s), untrimmed. Series `i97-t4s2` (OLED, 6 runs each) and the two original runs `i97-check-original`,
`i97-check2-original` (2026-10-23, before the two-screen setup: their screen is not recorded the same way). Means a run:

| Cycle | main: frames > 25 ms | G1+G4 | original | textures admitted (both builds) | model storages admitted |
|---|---|---|---|---|---|
| walk 1 | 4.0 | 3.3 | 1.0-2.0 | 677 | 158 |
| walk 2-8 (each) | 1.0-2.5 (mean 1.6) | 1.0-2.0 (mean 1.4) | 1.0 | 677 every cycle | 158 every cycle |
| sprint 1-4 (each) | 2.3-4.5 (mean 2.9) | 2.2-3.8 (mean 2.7) | 1.0-2.0 (mean 1.1) | 678-690 | 157-159 |
| turn 1-2 | 0 | 0-0.2 | 0 | 0 | 0 |

1. **The Firelink path is one identical reload, ten times.** Every 10 s cycle DS3 reloads the same map parts: 677 textures,
   158 model storages, 644 material and layout CPU records, 106 effect CPU records retired, 10 GPARAMs, 6 environment regions,
   the same counts in every cycle and run (`i97-t4s2-1-b`, all native events by cycle). From cycle 2 on, G4 finds every texture
   in its cache; this is why the path favours the cache (Thomas's remark) and why it cannot show a first-encounter cost.
2. **The original pays exactly one slow frame per reload** (its own joined step, K-0481), in both original runs. The native
   pays 1.4 a walk cycle and 2.7 a sprint cycle with G1+G4. **Half of the remaining gap is in the sprint cycles**
   (10.6 against the original's 4-5 over the four), although they admit the same content as the walk cycles. Section 8
   explains it: the sprint is not the cause; the sprint cycles are the path's last four reloads, and a native cost grows
   with every reload (the first reading here, « the join meeting a heavier native frame », was wrong).
3. **The camera turn tests nothing at Firelink**: no admission and no slow frame in either renderer. The suspect « the collect
   grows when new models enter the view » is therefore never exercised by the gate; at Archdragon Peak the standing camera
   turns gave 185 frames over 34 ms before I-0070 and 64 after (I-0070 RESULT).
4. The gate trims the first 2 s of each segment, which hides class D (the first steps after a standstill, walk 1 above:
   3.3-4.0 against 1-2). The new paths' readings should report the untrimmed first cycle separately.

## 2. The new paths

Common shape (INFERRED from point 1): each path is out-and-back with repeated cycles, so the **first cycle of a run measures
the first encounter** (content DS3 loads for the first time in the process) and the later cycles measure DS3's reloads,
which the caches serve. One run gives one cold sample, so the cold numbers need the run count (section 4). Paths are authored
as data (`scripts/replay/<name>.path.json`, the Firelink schema: keys, holds and relative mouse moves), not captured from
play: no recorder exists, and authored timings are exact. DS3's keyboard camera is J/L (left/right) and I/K (up/down)
(GENERAL; the Firelink path's turn uses L).

| Path | Save | Reach | Content | Structure (proposal) | Main risk |
|---|---|---|---|---|---|
| `archdragon` (m32) | Eta in its own copy of the Shebang save (Thomas, section 7) | reference run: Firelink bonfire, travel to « Archdragon Peak » (I-0070: travel tab x 803), rest, quit (saves), copy and hash outside the game folder | the heaviest scene measured: 4,200-5,200 draws, collect 12-16 ms in wave 1 | settle; a slow 360° sweep standing (L held, 4 cycles); I-0070's strafing walk before the distant serpent-men (proven not to draw them in); a short out-and-back to the first stairs | serpent-men near the bonfire; I-0070's walk did not aggro them |
| `ariandel` (m45) | Epsilon (`shebang-zones-epsilon-m45`, Corvian Settlement) | as recorded | the m45 measurement scene (D-0193); dense effects (snow, fires): the FXR suspect's best witness (INFERRED) | settle; sweep; out-and-back along the street; repeated | Corvians near the bonfire (GENERAL): the reconnaissance decides the street |
| `streaming` (m31 or m30) | Beta (`shebang-zones-beta-m31`, Undead Settlement) or Theta (`shebang-zones-theta-m30`) | as recorded | new map parts entered while walking | one long outward walk (30-60 s, at least two map parts first entered, checked by the admissions' first-seen share), then back, twice | hollows on every route (GENERAL); a drift ends comparability |

Note on Theta: its save stands at the **High Wall of Lothric** under the eclipse (W10-NATIVE-PASSES, Reference1704), not in
Lothric Castle proper; Lothric Castle needs a travel to its own bonfire in the reference run. Recommendation: **Beta in the
Undead Settlement first** (its save exists and already served I-0096's zone visits); Theta as the
second if the reconnaissance finds no enemy-free route at m31.

Fallback when no walk stays deterministic: a chain of `witness-teleport.txt` positions (both modes, `Run-ZoneWalk -Teleport`,
held while airborne) along the route, a hold of a few seconds each. It streams the same map parts in both renderers without
enemies, but in jumps, not at walking pace: a harsher and less natural load, kept only as a fallback.

## 3. Recording and replay protocol

Each step is an announced live run under D-0116/D-0117 (WORKFLOW, the PC clock read before posting, start and end in Paris
time, « PC libre »), only when no other thread uses the game (memory `parallel-sessions-shared-pc`), never during a gate.

1. **Save preparation** (one Reference run, Archdragon only): travel, rest, quit, copy, SHA-256, `record.json`; Alpha
   untouched; Thomas's saves restored by hash.
2. **Reconnaissance** (one Reference run per path, about 15 minutes): the candidate routes walked with `chr-positions.flag`
   (the player's position every 300 schedule calls, both modes) and screenshots every few seconds. A Reference run accepts
   no flag but `frame-dump` today (`Invoke-ReplayRun.ps1` line 50): allow `chr-positions` there, a tooling change outside
   `native/`. The route kept is enemy-free (no hit taken, no lock-on, no enemy in combat stance on the shots) and returns to
   its start.
3. **Authoring**: the path file with its save and hash, `settle_seconds`, the segments; reviewed against the reconnaissance's
   positions.
4. **Two control replays per path on `main`** (the project rule: two control passes before any A/B series), measured:
   the path is accepted when both complete (`replay complete`), their end positions agree within 1 m (`chr-positions`), and
   their per-cycle admissions agree within 2 % (`Get-I97Cycles.ps1`). Otherwise the route is changed, or the teleport fallback
   is used, and the two controls are run again.
5. **Pipeline cache state**: the first run on a new path compiles its new pipelines once on this PC (I-0072); the
   reconnaissance and controls warm it, the bases are warm. A cold-cache run (the player's real first visit) is a separate
   measurement, labelled, never mixed with the warm ones.
6. **Loading time**: each run also records the load from the title to control (D-0211 moves work into the loading screen;
   m37's load is 6.71 s natively against the original's 6.62, I-0097 tranche 2). Any added loading time is reported beside
   the stutter gain; its tolerance is Thomas's decision.

## 4. Bases to take per screen

D-0209 rules apply: one base per path and per screen mode, never mixed; numbers relative to the run's period P; display from
PresentMon; 60 Hz and 280 Hz never mixed. The new paths are **study bases first, not gate legs**: adding them to the blocking
gate lengthens every merge (the gate is already about 30 minutes) and is Thomas's call once they prove stable.

| Base | `main` runs | Original runs | Why |
|---|---|---|---|
| each path @ « 2560x1440@280 » (OLED, primary) | 4 | 2 | the engine numbers (Independent Flip, the cleaner screen); the cold first cycle needs at least 4 runs |
| each path @ « 1920x1080@60 » (ASUS, secondary) | 2 | 1 | the composed screen's display behaviour (K-0479, K-0480) on heavier scenes |
| Firelink, existing bases | none new | none new | re-read by cycle with `Get-I97Cycles.ps1` |

`main` here is `main` after tranche 4's merge (G1, G4, the one pacing rule), so its bases start from where tranche 5 starts.
Cost (INFERRED from `i97-t4s2`: 12 runs in 44 minutes): about 4.5 minutes a run, so 3 paths x 9 runs is about 2 h, plus
3 reconnaissances (45 min), 6 control replays (30 min) and the Archdragon save (15 min): about 3 h 30 of PC time, which can
be split over nights Thomas does not play (D-0204's night endurance then waits or is folded in).

Readings per run: the gate's metrics (frames over 2.04 P and 1.5 P at the frame end, game steps over 2.04 P, P,
display_late), plus per cycle: frames over 25 and 34 ms, game steps over 25 ms, textures and storages admitted, and G4's
`native_texture_kept` hits and misses (a first-encounter cycle has mostly misses). One sampled run per path and renderer
(`Run-I97Profile.ps1 -Ds3Threads`, `-Original`) for `Get-I97StepCost.ps1`, compared sampled against sampled only (the
sampler lengthens the joined step, K-0481).

## 5. Remaining suspects of the 25-34 ms band, ranked

| # | Suspect | Status today | What the new paths add | Test |
|---|---|---|---|---|
| 1 | **The native admissions of first-seen content inside DS3's joined step** (texture decode and upload, storage copy and geometry check, material and layout CPU records, GPARAM) | K-0481: the step 2,549 samples against the original's 1,371, 1,436-1,475 with G1+G4 at Firelink, where 75 % is reloads | the cold cycles, where G4 cannot help | D-0211 part 1: prepare the zone's native part during the loading screen, found by a cache lookup in play. Open design question: the content key (XXH64 of the engine's DDS bytes and metadata) must be computable from the files read ahead; to verify on a recorded path by the hit rate of prepared content |
| 2 | **The model storages' copy, not cached** (the model CPU load A0170 674 samples against 88; 158 storages each Firelink cycle) | K-0481 « tranche 5 »; G3 (kept blocks) tried and dropped | the reload share at m32 and m31 | G5: storages kept by content as G4 does for textures, then prepared ahead with suspect 1 |
| 3 | **The collect growing when new models enter the view** (a model's first Programs build 3.5-28 ms in the collect, class D) | never exercised by the gate (section 1, point 3) | the Archdragon sweep (I-0070: 64 frames over 34 ms standing after I-0070) | Programs built when the layout is admitted, off the collect, or for the whole zone during the loading screen; the model drawn from the first collect that finds them (D-0197) |
| 4 | **The GPARAM bank publication growing at every reload** (section 8; first in Thomas's order, 2026-10-25 02:23) | OBSERVED growth, mechanism INFERRED from the code and the samples | every path, and long play | section 8's first trial |
| 5 | **FXR registration** | DS3's own and equal (87 against 103 samples, K-0481): not a native cost on Firelink | m45's effect density | the native share of `NativeSfxAssetLoad` at m45, native against original; nothing to change if equal (D-0211: DS3's own loading is not forced) |
| 6 | Pipelines met for the first time | the pipeline cache (I-0072); a first encounter on a PC still compiles | the cold-cache run | report only; precompiling a zone's pipelines at the loading screen belongs with suspect 1 |

Class B (56-60 ms, 0-2 a run) stays unexplained; the new paths' worst frames are listed with it.

## 6. Rules kept

- **No pop-in** (D-0197, D-0211): nothing is drawn with a stand-in; a model whose content is not ready appears later, as the
  last resort only. Measure it: the number of models held back and for how many frames (I found no dedicated counter in the
  journal of `i97-t4s2`; one is added with the first prepare-ahead change), and the standstill pictures against the
  original at fixed points of each path. The 31 `native_texture_white_fallback` rows at Firelink are the once-per-path
  witness of SYSTEX_WHITE bindings (FaceGen paths, the dummy texture), not a pop-in.
- Two control runs before any A/B series; A/B series alternated in one series (`Run-I97Series.ps1`), with the builds passed
  as separate arguments (the `i97-t4s1` lesson; `Get-I97Cycles.ps1` splits its comma list for the same reason).
- Sampled runs compare steps, never counts.

## 7. Thomas's choices (thread « Étude tranche 5 », 2026-10-25 01:16 Paris)

1. **The streaming path: the Undead Settlement with Beta** (`shebang-zones-beta-m31`).
2. **Archdragon Peak: Eta** (« Prends le personnage que tu veux (Eta donc?) »): in a copy of the Shebang save, Eta travels to
   Archdragon Peak by the bonfire, rests and the game saves; that save is kept as a hashed test save and installed at every
   run (`Run-StartupTrace.ps1 -TestSave`), Eta's m40 save unchanged, Alpha untouched. **The same with Beta for Lothric
   Castle** (Thomas's suggestion): a fourth path, Beta travelled to the Lothric Castle bonfire in its own copy, recorded after
   the three others.
3. **The new paths stay study bases, outside the blocking gate**, at first (Opus's advice followed).
4. **The loading-time tolerance for D-0211's preparation: + 1 s on a zone load**, to start with.

Handover (the coordinator, 01:16): the saves are made in game only after tranche 4's gate frees the PC (« PC libre », about
01:50) and its merge, in tranche 5's own thread, each run announced 5 minutes before. These choices are recorded here; their
decision number is given when tranche 5 opens on `main` (tranche 4's branch already adds D-0211 and D-0212).

## 8. The sprint lead: a GPARAM publication that grows at every reload (2026-10-25 02:24-02:52 Paris, read-only)

Thomas's order (2026-10-25 02:23, relayed): tranche 5 starts with the sprint's extra cost, then I-0099. Read from existing
journals and samples only, while the PC ran tranche 4's recalibration (the readers at idle priority).

1. **Two stalls at every reload, in both renderers** (OBSERVED, `Get-I97ReloadStalls.ps1`; `i97-t4s2` b, the two original
   runs; the same in `i97-t4s2` a and `i97-t4-maincal` on both screens). DS3 reloads the same map parts once a cycle; the
   engine then stalls twice, about 6.5 frames apart. The first stall is the joined loading step (K-0481) and is equal:
   32-36 ms natively, 33-34 in the original. The second is 24.2-24.8 ms in the original at every cycle, just under the 25 ms
   line. **Natively it grows at every reload: 19.3 ms at the first, then 20.7, 21.4, 22.4, 22.6, 23.9, 23.2, 23.3, 26.3, 26.9,
   27.9, 28.4 at the twelfth.** It crosses 25 ms at the ninth reload, the first sprint cycle: the « sprint » frames are the
   late reloads, not the sprint. Natively the frame after it is short (7-8 ms: the pacer catching up), which is
   `Get-I97SprintSplit.ps1`'s « pairs ».
2. **What grows is the GPARAM bank publication** (OBSERVED, the module's own counters, `native_program_analysis`
   `gparam_publishes` and `gparam_publish_ms`): 20 publications a reload, 0.19 ms each at the first reload, then +0.05 ms
   at each reload, 0.74-0.76 ms at the twelfth (3.8 to 14.8 ms a reload, `i97-t4s2` b and `i97-t4-maincal` OLED alike). It does
   not stop at a zone change: over the night endurance `i97-night-1` (29 minutes, 14 loads) the cost of a publication went
   from about 0.1 to 0.75 ms.
3. **The samples agree** (OBSERVED, `i97-t4g4-prof-1` and `-2`, the second stalls of the first three reloads against those
   of three late ones, symbolised with build `t4-g4`'s `sample_symbolize.exe`, outputs outside the Evidence Store): on the
   other threads `AdmitGparam` and `PublishNativeGparamBanks` go from 10 to 38 samples, `Banks::Load` 36 and its
   `ReadGparam` 28; the late leaves include the GPARAM reader (`Reader::Utf16`, `Require`) and C++ exception unwinding
   (`FrameUnwindToState`, `FH4::ReadUnsigned`, `RtlVirtualUnwind2`). The engine thread waits longer in DS3's barrier
   (`23C21BB`, the task lock `179945F`): it joins the loading task in which DS3 admits the GPARAMs. The native collect does
   not grow (`UpdateNativeSceneCpu` 6-9 samples early and late).
4. **The mechanism** (INFERRED from the code, `native/frame/game_bootstrap/18_resource_admission.inc` `PublishNativeGparamBanks`,
   `native/world/host_input.h`, `native/world/light_producers.cpp` `Banks::Load`): each publication lists the bank folder's
   files in the host's virtual folder. A file that leaves (DS3 retires the GPARAM) is tombstoned by `ds3hostio::Remove`,
   which keeps its listing entry. When DS3 admits it again, its path is no longer in `gparam_host_paths`, so
   `ds3hostio::AddListing` appends it **a second time**. After N reloads each of the ten files is listed N+1 times, and
   `Banks::Load` walks every entry at every publication. A duplicate of a changed file is parsed again, since it is compared
   with the previous banks' source, not this pass's. A tombstoned entry (an empty byte vector) goes through `ReadGparam`,
   which throws, caught by `Banks::Load`'s `catch(...)`. The cost is linear in the number of reloads since the process
   started.
5. **Consequence beyond the path** (INFERRED, linear extrapolation): in a long session the second stall keeps growing with
   every map reload (at 0.05 ms a publication a reload, about 1 ms a reload more each time), with no upper bound until
   the game is closed.

**First trial, for tranche 5's thread** (no code written here):

- The change: a path is listed once. Either `PublishNativeGparamBanks` keeps the set of paths it ever listed and lists only
  new ones, or (cleaner) the host gains a call that sets a virtual folder's listing to exactly the current files, so a
  retired file also leaves the listing. In `Banks::Load`, a key already loaded in this pass is skipped and an empty byte
  vector is skipped without a throw (defensive). The parsed banks are unchanged: same files, same latest generation.
- A unit test in `host_input_test` or a new one: admit, retire and admit the same ten files 50 times, then check that the
  listing holds ten entries, that each publication parses only the changed files, and that no parse throws.
- Expected (INFERRED): the publication flat at about 0.19 ms, the second stall flat at about 19-20 ms (under the original's
  24.5), so about 4.7 fewer frames over 25 ms a run on the path (sprint 4.0, late walk 0.7: 21.7 to about 17 trimmed, the
  original 12-14), the same on game steps. The rest of the gap is then the first cycle (class D) and scattered frames.
- The checks: two control runs (replayed Firelink, OLED, unsampled) on the change, read with `Get-I97ReloadStalls.ps1`
  (second stall and ms a publication flat across the twelve reloads), no refusal, the gate's standstill picture equal to
  `main`'s (GPARAM drives the lighting); then the alternated A/B series against `main` (6 pairs), then the gate.

## Evidence

Section 8: `Get-I97ReloadStalls.ps1` and `Get-I97SprintSplit.ps1` (this branch) on `i97-t4s2-{1..6}-{a,b}`,
`i97-t4-maincal-{2560x1440-280,1920x1080-60}-m{1..4}`, `i97-check-original`, `i97-check2-original`; `i97-night-1`'s
`native_program_analysis` rows; the samples of `i97-t4g4-prof-1` and `-2` (build `evidence/I-0097/builds/t4-g4`, whose
`game_bootstrap.dll` matches the runs' module hash) symbolised for the marks of the second stalls (prof-1 3904, 4499,
5103 against 8766, 9377, 9946; prof-2 4174, 4773, 5373 against 9036, 9667, 10234).

Read only: `evidence/I-0066/artifacts/i97-t4s2-{1..6}-{a,b}` (a = `main`, b = G1+G4, by `native_texture_kept`'s presence),
`i97-check-original`, `i97-check2-original`. Tool: `investigations/I-0097-stutter/Get-I97Cycles.ps1` (this branch).
