<!-- Archive copy of `.project/WORKFLOW.md` as it stood on main before the documentation diet of 2026-11-15 (D-0291, Thomas: « Je valide d'avance »). Text verbatim; only relative links are rewritten to resolve from this folder. Never an active instruction: the live file is `.project/WORKFLOW.md`. -->

# Operating workflow

The rules in force, in the present tense. Each names the decision (`decisions/LOG.md`) or Thomas's instruction it comes from;
their dated wording, quotations and consumed exceptions are in Git tags `archive/pre-cleanup-2026-10-25` and
`archive/pre-cleanup-2026-10-02`. A new rule replaces the sentence it changes; it is not appended beside it. Repeated
procedures live in the project's skills (`.claude/skills/`, listed at the end), loaded when they are needed.

## First rule of wave 2: the architecture of Elden Ring (Thomas, 2026-10-20, D-0195)

FromSoftware already modernised this engine with D3D12 in Elden Ring: the native backend copies that architecture, then
improves it. Every wave-2 design (scene persistence, binding, pipelines, queues, loading and threading, effects) starts from the
ER mechanism recovered by static decompilation of Elden Ring 2.7.1 (read-only, headless Ghidra on a copy of the project; Sonnet
5.5 may carry the decompilation, Opus checks the sources a design relies on) and states how DS3 adapts it. Only the data differ:
DS3's effects, parameters, models, shaders, layouts and behaviour are verified on DS3. The existing native work is transformed,
not thrown away.

ER's known defects are not copied (Thomas, 2026-10-21): ER itself stutters (pipelines compiled in play, streaming hitches). Each
design names where ER has the defect (from its decompilation or as public knowledge, said which) and what the native backend does
better.

## Reporting: honesty first (Thomas, 2026-10-23)

A bad result, a failure, a skipped step or an uncertainty is said as it is, in the thread, a RESULT or a commit. A gain within the
noise is reported as no gain; a step not run is named as not run. Observations (what happened) and conclusions (their
interpretation) stay distinct: question → hypotheses → experiment → observations → conclusion → canonical update.

## Canonical sources

[`INDEX.md`](../../INDEX.md) owns the routing table. STATE and the ledger summarise and link; they never copy evidence, derivations or
protocols. STATE's resume rows are rewritten at each checkpoint, never appended to (`Test-Project` warns over 300 words a row).
A study or report that guides a workstream is copied into that workstream's investigation in its first commit; a file in the
project's shared folder is never the only copy of a canonical text (D-0214). The timing project under
[`timing/`](../../../timing/IMPORT.md) keeps its own historical numbers (D-0196).

## Roles and delegation

- **Thomas**, project director: goals, ambition, scope, strategic trade-offs, protected authorizations, reporting decisions,
  Windows, NVIDIA and security settings, pushes and mass Git deletions.
- **The coordinator** (Claude, in the project chat): relays Thomas, opens threads, routes reports; its rules are the repository's.
- **The pilot** (by default Opus 5.5 at high effort, D-0245; the former « Sol »): owns the open workstream: framing,
  hypotheses, interpretation, claim statuses, integration, checkpoints (D-0114). One pilot thread at a time per workstream.
- **The threads' short names** (Thomas, 2026-11-06 08:56): **R** the RT/PT pilot (it runs [RT-PT-PLAN](../../RT-PT-PLAN.md) §8's
  workstreams R0 to R11, D-0235, hence the letter; « the R pilot » in the repository from 2026-11-01), **E** the open-gaps pilot
  (« écarts », I-0113), **S** the study thread.
- **The study thread** (one at a time in the normal flow, for either pilot; Thomas, 2026-10-30 05:34): always one workstream ahead, read-only on the code; it merges into `main` only its documentation-only commits (no code, nothing to rebuild), when no pilot is merging or rebuilding at that moment (D-0239); no load on the PC during a live test
  (its readers at idle priority). Its report is filed for the next pilot, who copies it into the investigation (Canonical
  sources). ER studies ahead are made there by Sonnet subagents.
- **Subagents** (Thomas, 2026-10-18, 2026-10-25, 2026-10-27 and 2026-11-01; D-0159, D-0225, D-0245): the pilot delegates, to
  save its own context. **Mechanical, quick tasks that need little complex reasoning are always delegated to Sonnet 5.5
  subagents**, at the effort level the thread chooses: reformatting or ordering a file, a table or a list; counting or sorting
  log lines; extracting numbers from a record; one small change applied at places already listed; a keyword sweep. Sonnet 5.5 is
  also the default for reading journals and evidence, computing measures, analyses, decompilation reading (I-0044) and reviews
  of documents; Opus 5.5 at medium effort beside them. Each delegation states question, scope, inputs, non-goals, expected output and
  exit condition. Worker output is non-canonical until the pilot checks the primary evidence; never delegate framing, ambiguous
  interpretation or project-level conclusions.
- **Reviewers** (D-0198, never reduced by the usage limit): a fresh Opus 5.5 at high effort after each step; before every merge
  into `main` (a tranche or a workstream's last), a fresh Opus at very high effort (xhigh) reviews the whole branch diff against
  `main`, an agent that neither wrote the code nor saw the thread. Both (Thomas, 2026-11-15, D-0283): the reviewer's job is to
  find what is wrong, not to confirm that it works (« Trouve ce qui ne va pas »); its brief gives the diff, the goal and the exit
  criteria, never the pilot's summary (what it did, what it fixed, how the run went): the reviewer reads the code first and tests
  the records against it. The project's subagents `step-reviewer` and `final-reviewer` (`.claude/agents/`) carry these briefs;
  skills `review-step` and `review-final` write and launch them (Thomas, 2026-11-15).
- **Helper subagents** (Thomas, 2026-10-27): `measure-tools` (Sonnet) for the measuring tools' trial before a series, reading
  journals and computing numbers, which it returns without interpretation; `ghidra-research` (Sonnet) for targeted references
  and decompilation with the project's read-only headless method on the mirrors (the analysed project's copies, with its names),
  its result re-read by a fresh Opus when a decision depends on it. The pilot keeps the conclusions.
- **Fable 5.2** (the former « Astra »): only with Thomas's explicit agreement (D-0198); Opus at high effort does its tasks by
  default.

Work that needs judgement stays with the thread by default: delegate it only when the gain exceeds the coordination cost
(mechanical work always goes to Sonnet, above). Explicit assignments by Thomas take
precedence. Preserve unresolved disagreement and negative results. The optimisation review and the audits (menus, resolutions,
full screen) stay with Opus at high effort. Historical records say Sol, Luna and Astra: read Opus, Haiku and Fable. An
authorization already given for a task needs no new request.

## Threads and continuity

- **The pilot's branch.** A workstream runs in its own worktree branch. The thread that opens it, and every fresh thread that
  continues it on a new branch, writes its branch in STATE's « Pilot branch » row (the bare branch name) and the workstream in
  « Active workstream » in its first commit. The row says `none` only when no workstream is open.
- **Waiting.** A thread waits by ending its turn (a reminder, a task notification or a message wakes it), never in a sleep
  loop; a wait on a run or a build is a harness-tracked background command, so the session is not left idle. No stop hook holds
  a thread (the stop guard was removed, Thomas 2026-10-29).
- **Full production** (Thomas, 2026-10-28, D-0227): short tranches, each merged once its code is written and reviewed, with no
  measurement gate, thresholds, series or night endurance; no live run before a merge; after every merge the module builds
  with its unit tests and a one-minute start check runs (D-0229; Merging into `main`). A measure is taken only when a fix cannot be written without it (said to Thomas) or on his
  request. Short targeted witness runs (about ten minutes, announced: a family drawn natively against the original, a spell
  cast, a witness row) are welcome as diagnostics, never as a merge gate (Thomas, 2026-10-29 01:12); the full control stays every
  three days (D-0230).
- **Documentation cleanup before every checkpoint** (Thomas, 2026-10-25): before the commit that closes a step, a handoff or a
  merge, the thread cleans the documentation it touched: STATE's rows rewritten to the current position (history moved to the
  RESULT); stale, contradictory or duplicated sentences removed from STATE, the RESULT, ROADMAP, WORKFLOW and the notes it
  changed; a rule changed in one place and not left in its old form elsewhere (memory files included: they only point to the
  repository); links checked. At every clean checkpoint the thread also deletes the worktrees and folders its own subagents made
  and checks that `scripts/Test-Project.ps1` passes from the main checkout without a STATE warning. A smaller subagent may do the
  documentation pass, then the thread checks it.
- **A fresh thread at every clean checkpoint** (D-0215): a coherent commit, STATE current, merged into `main` when the step
  merges, without waiting for the thread to be loaded. If the new thread cannot open yet (for instance at night, before Thomas
  allows its card), the current thread goes on and changes at the next checkpoint. A new workstream starts only from a clean,
  validated `main` that is resumable from the canonical sources alone.
- **Completion (D-0129).** A workstream that meets its exit criteria and is validated is merged into `main` by its pilot, and the
  next one starts without waiting for Thomas (he is informed). A tranche (or a step whose merge its plan names) is merged the same
  way once written and reviewed (D-0227). Any other merge in mid-workstream needs Thomas.
- **Thomas plays.** On « je joue » every thread pauses (no live run, build, Ghidra or heavy agent) until his « fini ». On « je prends
  le PC » (Thomas, 2026-10-28) the pilot only reads and writes code (no build, no live run, no Ghidra) until his « fini ». His save
  is never overwritten: the launcher backs it up and restores nothing; a test run backs up the save present at its start and
  restores that one at its end; tests use copies, never his own characters except Alpha as the Live game rule allows or on his
  request.
- **`PAUSE`.** Start nothing new, finish only an atomic operation, stop instrumentation, restore the game and environment,
  preserve evidence, update the resume checkpoint, validate, then wait. It never closes the workstream nor authorizes a merge.
- **Recovery.** Read STATE's checkpoint, inspect branches, worktrees and history, reconcile before repeating any acquisition;
  never assume `main` is newest. Recovery never authorizes a merge. After every context compression, reread INDEX's resume route,
  STATE, the current ROADMAP capability, the mandate and this file before any technical decision, implementation or live test
  (Thomas, 2026-10-09). Skill `resume-thread`.
- **Weekly usage limit**: at 50 % every pilot sends the coordinator a note, and the coordinator stops the open-gaps pilot after
  its next clean checkpoint while the RT pilot continues (Thomas, 2026-11-05, D-0248); at 90 % slow down (fewer parallel agents, nothing optional) without reducing a review; at 99 % make a
  clean checkpoint and pause.

## Investigations

Open with `scripts/New-Investigation.ps1 -Id I-NNNN -Slug name -Title "..." -Capability C3 -Owner "..." -Budget N`, then complete
`CONTRACT.md`: one precise question, scope in and out, known inputs and claims, discriminating evidence, exit conditions
(success, refutation, insufficient information, stagnation, budget) and a bounded budget. `status.json` records owner, state
(`planned`, `active`, `blocked`, `complete`, `abandoned`) and budget; one investigation, one owner. Count each meaningful
experiment or analysis cycle in `iterations_used`. A workstream in tranches stays `active` until its last tranche merges; at a
budget limit, close the tranche with a checkpoint, record the next bounded plan and extend `iterations_max` (never reset
`iterations_used`); continue only within unchanged authorized scope, never to hide stagnation. Raw outputs go under
`evidence/<id>/artifacts/`, or at an external location recorded with its hash in the manifest. Only the pilot assigns claim
statuses, never because a tool or worker asserted one. If an experiment cannot discriminate, record *insufficient information*. Close with `RESULT.md`: conclusion,
exact evidence references, uncertainty, artifacts, optional next step; then decide on ledger, decision and STATE updates,
and run `scripts/Test-Project.ps1`. A series runner is written once per need (D-0217): copy an earlier `Run-I*Series.ps1` only
when its measure differs, and keep the earlier ones as the record of how their results were made.

## Coverage by families (Thomas, 2026-10-06, D-0150)

For every major subsystem, build an offline coverage inventory first: group assets, events and resources into the families that
matter to the implementation, map each family to in-game witnesses, and test family by family. A live discrepancy outside the
inventory names a missing family: add it before fixing it. Native production is written per family or engine mechanism (MTD
family, material class, pass row, pose owner, constant producer), never keyed to one asset; a fix that serves one asset is a
defect (D-0161). Graphics settings are one more axis: list offline what each changes (passes, programmes, target sizes,
parameters); a change of size or parameter is covered by the qualified input and parameter contracts; programmes or passes the
ultra runs never drew are credited by one run at the lowest settings whose drawn programmes are compared with the ultra runs'.
Resolution, aspect ratio, display mode (windowed, borderless, exclusive full screen), later HDR, internal scaling and upscaling
are inputs each frame supplies (D-0151): the renderer holds no size, ratio or mode of its own, and a fallback to one is a defect.
List offline what depends on each (the 128-pixel light tiles, half- and quarter-size targets, borders, screen-size constants,
shadow and atlas sizes, the presentation window), then check it live at sizes chosen to break hidden assumptions (not multiples
of 128 or 4, another aspect ratio) and once per display mode.

## Architectural frame challenge (anti-tunnel)

No native mini-workstream is chosen by successive crashes (D-0156): work follows the design and the [ROADMAP](../../ROADMAP.md); a
crash informs the design. Before a large tranche, a static-to-runtime step, a declared implementation readiness, a
high-consequence seam, or when one blocker persists, the pilot answers: what exact fact is sought and why the objective needs it;
whether it is an engine invariant, a historical detail, a compatibility constraint or something the replacement may define;
what decision proving it unlocks; whether recent results reduced the blocker or only added detail; whether a smaller experiment
would answer more directly.

| MUST RECOVER / PRESERVE | MAY BE REDESIGNED | UNKNOWN |
|---|---|---|
| Required game behaviour, DS3-specific data, layouts and ABIs wherever consumed, ownership and lifetime guarantees needed for direct current input delivery. | Engine-side architecture and internal contracts following the recovered ER method (D-0157), and backend mechanics: synchronisation, retirement, allocators, queues, descriptors, fences, caches. Existing boundaries, investigation transports and capture joins do not constrain the production interface. | Not yet established. |

Outcomes: the frame confirmed, an obligation reduced, static diminishing returns, implementation is the better experiment
(surface it to Thomas), the seam must move, or the question was misframed. Do not reverse-engineer a historical mechanism merely
because the original engine used it. Before a D3D12 seam, ownership boundary or backend contract, recover the ER mechanism that
does the same job (the first rule), state what DS3 changes and which DS3 data it consumes, and record when ER has no analogue.
Same method, same architecture, different data, with explicit engine control of D3D12 resources, commands, synchronisation and
lifetime (D-0157). The ER study's tools and sources are in the
[mandate](../../NATIVE-D3D12-RESET-MANDATE.md#elden-ring-method-study); the ER folders stay read-only.

## Method rules

- **Optimise, do not stack** (Thomas, 2026-10-19): a correction replaces the path it corrects. No data is read, analysed or
  decoded twice for the same purpose; what changes only on an event is kept by content or generation and recomputed on that
  event; diagnostics run only behind their flag. Before every merge, one optimisation pass over the branch's additions (read; measured
  only on Thomas's request since D-0227).
- **Validation and RCA**: prefer deterministic tests, then instrumented observation, reproducible comparison, independent
  verification, model judgment. `SUPPORTED` is never silently treated as `CONFIRMED`; an uncertain result is validated before
  dependent work builds on it; documentation checks prove repository consistency only. After two failed local fixes, a recurrence, multiple regressions or lost causal understanding: stop patching
  and open an RCA from observations.
- **Traced lighting judged by its truth** (D-0249, Thomas, 2026-11-06): in RT/PT the original's picture is not a pixel target;
  what is accepted is a natural and true lighting, in every scene; a difference from the original is not a defect in itself, a bug
  stays an anomaly to fix. No lighting cheat is simulated or created (Thomas, 2026-11-09 03:12, D-0262): no fill light, baked ambient
  or cube standing in for a traced answer, no added fake light; only the real sources' settings are adjusted. The rays' inputs (materials, lights, geometry) are still read from the original; the raster the plan keeps (G-buffer,
  effects, UI, post) stays faithful to it; raster lighting the RT/PT replaces is not corrected (no full raster mode, D-0237; E fixes only what stays visible with the rays, D-0250).
- **Guarding the judge** (Thomas, 2026-11-10, D-0268; against reward hacking): (1) a change to a test, an assertion, a tolerance or
  threshold, a gate's reference, a measuring flag or a reader is stated plainly in the RESULT with its reason; an unstated one counts
  as a failure. (2) Before every merge, after the final review, the `integrity-reviewer` subagent looks only for weakened tests,
  special cases for one scene, work cut during a measure and numbers without evidence; its findings are answered or recorded in the
  RESULT as the final review's. (3) Every visual gain is shown with before and after stills or films; Thomas's eye stays the final
  judge (D-0181, D-0249).
- **No pop-in** (D-0197, D-0211): nothing is drawn with a stand-in; a model not ready appears later, as the last resort only.
- **Skip the object and go on** (FINISH-LINE line 1, I-0101): where the module meets per-object game data the original tolerates
  (a non-finite matrix, a menu family never witnessed, one object of an unknown kind), it skips that object for that frame or
  that admission, counts it and writes a rate-limited row naming it, instead of closing the game. A refusal stays for the
  module's own contracts: its hooks and pins, thread and scope checks, its registries' invariants, the device, the start-up.
  A skipped draw is not a stand-in (D-0197).
- **Stability first**: the remaster's additions (options, longer draw distance, new features) go to the ROADMAP, not into a
  stability workstream.
- **Decompilation** is targeted to the systems in scope, never whole-game reconstruction.

## Native code navigation (clangd)

Every thread that reads or changes `native/` navigates and checks it with clangd (Thomas, 2026-10-18). In a new checkout, and
whenever the build's sources or flags change, run `pwsh -NoProfile -File scripts/Connect-EvidenceStore.ps1`, then
`pwsh -NoProfile -File scripts/New-CompileCommands.ps1` (the ignored `compile_commands.json` needs the evidence junction for the
generated headers). Use the `clangd-lsp` plugin's LSP tool, or `& "C:/Program Files/LLVM/bin/clangd.exe" --check=<file.cpp>
--compile-commands-dir=<checkout>` (« 0 errors » expected; clangd is not on PATH). The MSVC build and its unit tests
(`native/frame/Build-GameBootstrap.ps1`) stay the gate.

## Live game rule

- Since D-0117 the pilot may launch and drive Dark Souls III itself under the D-0116 conditions: a written protocol announced
  in the thread, offline, a hash-checked restore, nothing installed in the game folder. Never an unannounced run; live
  authority is per decision (the mandate, D-0115).
- Announce each run at least 5 minutes ahead with the start and end times read from the PC clock just before (Paris), and whether
  it measures; say « PC libre » at the end. One DarkSoulsIII at a time: check the other sessions before a run or a build, and
  build into a folder of your own (`Build-GameBootstrap.ps1 -Out`, runners `-BuildFolder`). Builds and Ghidra may run during a
  run that only captures or reads (no timing); a measuring run (cadence, frames per second, the full control) has the PC entirely
  free (Thomas, 2026-10-29 07:57). Two pilots in parallel (Thomas, 2026-10-30 05:32-05:34): the RT pilot (the RT/PT
  plan, R1, R2, R5 and on) and the open-gaps pilot (the reported gaps: the effects, then the grey textures, the snow and the
  rest, not the raster-lighting defects the RT replaces); the RT pilot has priority on the ASUS and the launcher rebuilds unless
  an open-gap fix is ready to test. **Every run slot goes through the coordinator** (Thomas, 2026-11-06 09:08): a thread asks it
  for a slot, the coordinator gives and announces it, and organises the runs of all the threads; everything else the threads
  settle between themselves.
- Before announcing, check the run dry (Thomas, 2026-10-25): the exact command line, its effect flags accepted by
  `Run-StartupTrace.ps1`'s pattern, a capture writing where it should, a sampler whose start and end fall inside the path. Every
  run checks its launch itself (`scripts/Invoke-ReplayRun.ps1`, `launch-check.json`): the module and controller in the build
  folder and the test save at its hash before; after the settle, the module loaded and a native presenter at the screen's rate,
  else it stops before its path. While a series runs, no script it calls is edited (a `sed -i` once rewrote a runner's line
  ends and broke the next run). Since D-0229 no live run verifies a fix before its merge; the routine run is the one-minute start check after it; the rules below on
  measurements apply when one is taken (a fix that needs it, or Thomas's request). Before a series of measurement runs (Thomas, 2026-10-27), one short run checks that each measuring
  tool (PresentMon, the sampler, a counter) attaches and writes its data; a tool that does not is fixed before the series (I-0100:
  PresentMon left no CSV in eight runs of the LOD scale study), and a reader's figures are checked against one journal read by
  hand before they are reported (I-0100 tranche 7: a reader's fixed frame boundary counted a load as play in some runs, D-0225).
  Live runs are grouped (D-0225): one announced slot holds every run the step needs (controls, sizes, before and after), rather
  than several short series. Every control run of a change covers at least three render sizes,
  1280x720, 1920x1080 and one arbitrary window (Thomas, 2026-10-27: one universal rule, never a case by resolution; I-0100's
  option A first passed every 720 run while wrong at other sizes).
- A measurement taken while Ghidra, a build or a heavy agent ran is run again, not interpreted.
- Test characters' progress (Thomas, 2026-10-28): a test save need not be restored after every run; a test character that reached a
  new zone is kept as a new hash-recorded test save (`Run-AlphaCast -KeepSaveLabel`). Thomas's own saves and Alpha are never
  overwritten.
- Zone witnesses (D-0174): reached with the **Shebang** save, one character other than **Alpha** per zone, warped to the zone's
  bonfire in a reference run, saved there and kept as a hash-recorded test save outside the game folder (as the outfit saves
  under `C:/Users/User/.ds3-i0039-backups/shebang-*`), installed with `Run-StartupTrace.ps1 -TestSave <sl2>
  -TestSaveSha256 <hash>`. Alpha is never modified; it may be used to test spells, miracles and pyromancies from a copy of the
  save only. Zone and item names are written in English, as the game shows them (Thomas, relayed 2026-10-30: « Archdragon
  Peak »); the French names some of his messages use (Capharnaüm des Confins = Dreg Heap, Cité Enclavée = Ringed City, Village
  or Camp des morts-vivants = Undead Settlement) are only to recognise them.
- A measure names its screen (the ASUS by default, the OLED when only its rate, G-SYNC or full screen shows the change) and
  reads the display with PresentMon; 60 Hz and 280 Hz numbers are never mixed.
- The OLED never stays on without Thomas (Thomas, 2026-10-26 18:04: its care): when he is away, every test runs on the ASUS alone
  and nothing is left displayed on the OLED. When the OLED is on (Thomas there), the tests run on the OLED (Thomas, 2026-11-06
  19:09: « Merci de faire les tests sur l'OLED, il est allumé pour rien là et s'est mis en protection »), the gate's required ASUS
  base apart (D-0218); the runs go to the OLED in a 2560x1440 window (Thomas, 19:18: « faire les tests en 1440p sur mon OLED
  plutôt ? Pour éviter le bureau fixe », 19:20: « en 2560x1440 fenêtré pour garder la croix »; `-GraphicsLevels
  'ScreenMode=WINDOW;Resolution-WindowScreenWidth=2560;Resolution-WindowScreenHeight=1440'`, which the start check and the
  endurance take), never a small window over the still desktop. When Thomas turns the OLED on for a test, it is the only
  screen used and the game's window is at 2560x1440, the screen's full resolution (windowed is fine), so that the desktop
  does not show around it (Thomas, 2026-11-14 11:22 to 11:27: « Il doit être le seul écran utilisé », « il faut juste que ça
  soit la résolution maximale de l'écran pour éviter d'afficher le bureau dans le vide », « Je valide la règle dans le
  WORKFLOW »). The endurance tour is not yet right at that size (its inputs
  were off on 2026-11-06, Thomas: « le script » « décalé »): until it is, the endurance runs on the ASUS.
- PIX (`C:/Program Files/Microsoft PIX/2603.25`) and Nsight Systems (`C:/Program Files/NVIDIA Corporation/Nsight Systems
  2026.5.1`) are used when they answer better than the journals, the sampler and PresentMon (Thomas, 2026-10-25).
- Know-how (pilots, menus, coordinates, pitfalls): [LIVE-RUN-NOTES](../../LIVE-RUN-NOTES.md). Procedure: skill `live-run`.

## Tools

Use the tool that answers the question best; say which one a measure comes from. Installing a new program on Thomas's PC (a
profiler, Python, a Ghidra extension, an MCP server) is his decision: the thread asks him with the exact command and what it
changes. The tools study of 2026-10-25 ([TOOLS-STUDY](../../TOOLS-STUDY.md): FLIP, Ghidra MCP servers, pixtool) holds their sources and
limits. Windows developer mode is on since 2026-11-01 (Thomas, at a pilot's request): `pixtool.exe` reads the native PIX captures
(F8 in a test run, `pix-capture.flag`; the PIX row below and skill `live-run`).

| Need | Tool | State on this PC |
|---|---|---|
| The display's cadence (late and dropped frames) | Intel PresentMon 2.6.0, `C:/Users/User/Tools/PresentMon`, unelevated (`Invoke-ReplayRun -PresentMon`, D-0209) | in use |
| Where the CPU time goes (both renderers) | the native sampler (`sample-profile.flag`, `native/frame/Get-SampleProfile.ps1`) | in use |
| One clock for the game step, the waits, the presents and the vertical blanks | `cadence-trace.flag` with `investigations/I-0097-stutter/Get-I97Cadence.ps1` | in use |
| A CPU, GPU and vblank timeline in one view | Nsight Systems 2026.5.1, PIX 2603.25 (timing captures need administrator rights: ask Thomas) | installed |
| A native frame's GPU work, resources and draws | PIX 2603.25: `pix-capture.flag` loads its GPU capturer and takes programmatic captures (`CaptureNextFrame`, I-0100 RESULT tranche 2 item 13, about 1 GB each); `pixtool.exe` exports the event list and render targets; never on Thomas's save, never during a measure; the module runs it without the debug layer (I-0117 T7) | captures taken; Windows developer mode on since 2026-11-01 (Thomas): pixtool reads them (`save-event-list` on `I-0100/build-t2g/pix-3060.wpix`, I-0113 tranche 11) |
| Per-thread CPU zones image by image, for the stutter | Tracy (BSD): its client compiled into the module behind a flag, off by default, its cost measured; a pilot's task when the sampler and Nsight do not answer | not integrated |
| A whole-system trace (disk, CPU, GPU) | Windows Performance Recorder and Analyzer (administrator rights: ask Thomas) | not used |
| Native against original pictures | the existing comparers (the pair sheets, `Compare-SceneHdr.ps1`); NVIDIA FLIP for a perceptual difference (its executable `flip.exe -r <reference> -t <test>`, the `flip-evaluator` 1.7 package's; the package's Python `evaluate` ends the process here) | FLIP: `flip.exe -r <reference> -t <test>` works (I-0106, 2026-10-29: its « Mean » line); the Python package's `evaluate` and `load` end the process with 0xC0000409 on this PC (numpy absent): use the executable |
| A film's motion: a stutter, a freeze, an animation that restarts, a flicker, repeated pictures (Thomas, 2026-11-07, [VIDEO-STUDY](../../VIDEO-STUDY.md)) | `python -I scripts/video/videosheet.py <film> [<original>] --out <dir> [--roi name=x:y:w:h] [--control name=x:y:w:h] [--shift dx:dy] [--slit-x X]` (standard library, ffmpeg does the pixels, seconds a film): `report.txt` (each region's own jumps, their period, the frames frozen while the rest moves, the blinks (a flicker is many short ones), the camera turns, the whole-picture hitches, the repeated pictures; without a region, the frames where many blocks of the picture jump at once), `curves.png`, `footprint-*.png` (where the jumps are), `slit-*.png` (one column a frame: time as space), `views-*.png`. A motion defect is judged by its shape in time, never by a mean over a region (gap 22, 2026-11-06), with a control region and the original's film; camera fixed or a replayed path; the film is content time, the cadence stays PresentMon's; the encoder's keyframes (x264 every 250 frames, NVIDIA's clips every 30) make small jumps, set apart in the report. A thread sees stills, never motion: this tool, not a sheet of frames, not the catalogue's video plugins (Thomas, 2026-11-07: « On va garder notre outil »), not Gemini | in use |
| What Thomas saw in play | his NVIDIA clips (`C:/Users/User/Videos/NVIDIA/Dark Souls III`, named by their start time, 2560x1440 at 60 frames/s): frames extracted around the times he notes with ffmpeg 9.0.2 (Gyan's full build, installed by Thomas with winget 2026-10-26; `%LOCALAPPDATA%/Microsoft/WinGet/Packages/Gyan.FFmpeg_*/ffmpeg-*/bin/ffmpeg.exe`, `-vf fps=N,tile=CxR` for a sheet, no `drawtext`: no font configuration) | in use |
| A frame of the original (D3D11): its draws, states, constants and targets | RenderDoc 1.47 (`C:/Program Files/RenderDoc`: `renderdoc.dll`, `renderdoccmd.exe`, `qrenderdoc.exe`, the in-application API `renderdoc_app.h`; installed by Thomas 2026-11-08, the main checkout's settings allow it); PIX reads D3D12 only; never on Thomas's save, never during a measure | installed, not yet used (I-0113 tranche 35: wanted for gap 23, answered without it) |
| DLSS (Super Resolution, Ray Reconstruction, frame generation) and Reflex for R1 (D-0165: Streamline in manual mode) | NVIDIA Streamline SDK 2.14.16 (`streamline-sdk-v2.14.16.zip`, GitHub release, SHA-256 32041E51…956B as the release's digest), `C:/Users/User/Tools/DS3-Decompilation/third-party/streamline-2.14.16`; its x64 DLLs (`nvngx_dlss`, `nvngx_dlssd`, `nvngx_dlssg`, `sl.*`) signed by NVIDIA (checked 2026-10-30); downloaded with Thomas's yes (2026-10-30 04:51); the build hard-links the interposer, `sl.common`, `sl.dlss`, `nvngx_dlss`, `NvLowLatencyVk` and the others beside the module (I-0111, D-0260: one file with the SDK's, never written in place) | DLSS through Streamline since I-0111: loaded at every start unless `no-dlss.flag`, DLSS runs only with a chosen mode (`dlss` naming one in `rekindled-settings.ini` beside the module, I-0132 R12b, or `dlss-dlaa.flag` and the other `dlss-*.flag` in test runs) and the game's Antialiasing option on; otherwise the TAA (I-0132 R11j, D-0265; `no-taa.flag` the test tool); a comparison against the original passes `no-dlss`; Reflex (I-0111) and frame generation (I-0131 R10b) through the same file's `reflex` and `frame_generation` |
| Decompilation | GhidraMCP first (Thomas, 2026-10-29), for the pilot and the `ghidra-research` subagent; Ghidra headless (`Invoke-Ghidra.ps1`; 12.1.4 for DS3, `Tools/DS3-Decompilation/ghidra_12.1.4_PUBLIC`, a copy of Thomas's install, since the DS3 mirrors are in its format, D-0269; 12.1.3 for Elden Ring) on a mirror copy, read-only, when the GUI is not open, for Elden Ring, for mass searches and for results a script must rerun | GhidraMCP in use since 2026-10-28: Thomas's Ghidra 12.1.4 GUI on the copy `Tools/DS3-Decompilation/ghidra-mcp-ds3` (port 8080, the repository's `.mcp.json`), several threads may read at once (Thomas, 2026-10-29), never closed by a thread; names and head comments written there by one session at a time, the holder of the token file `Tools/DS3-Decompilation/ghidra-mcp-ds3.writer` (its session and Paris time; taken when absent or older than 4 h, removed at the end), the others reading only meanwhile; a name only when the repository states the role, as `<Role>_<address>`, each one a row of `decompiled/GHIDRA-NAMES.tsv`; GhidraMCP cannot save, Thomas's Ghidra does (D-0269); the mirrors (`ghidra-mirror-i0066`, `-i0068`, `-menus`, remade from the saved copy on 2026-11-10 with its names; the earlier ones removed with Thomas's yes, 20:24; the copy before the names kept as `ghidra-mcp-ds3-backup-2026-11-10`, Thomas 20:23), `game/ghidra` and the `ghidra-research` subagents read only; after a pass of names, Thomas saves, then the writer remakes the three DS3 mirrors from the saved copy (none open by a headless run, the locks and `tmp*.ps` left out, the earlier ones kept beside) so that every reader sees the same names; never a DS3 mirror under Ghidra 12.1.3, which fails on the 12.1.4 format while exiting 0; a function encrypted at rest (its bytes decode to invalid instructions, 7.8 % of the code, I-0133) is read headless through the decrypted-code overlay, in memory: `investigations/I-0133-protected-code-map/tools/I0066Overlay936.java <out> evidence/I-0066/artifacts/direct-code-capture-protocol-934/overlay935/overlay.bin <entry>...` on a DS3 mirror, before any loaded-code run; the executable and its protection are never patched (D-0272) |
| D3D12's newer runtime (shader model 6.9 and its ray reordering, GPU upload heaps, `CreateDeviceFactory`) | Microsoft's Agility SDK, NuGet `Microsoft.Direct3D.D3D12` 1.620.1 (`C:/Users/User/Tools/DS3-Decompilation/third-party/agility-1.620.1`, nupkg SHA-256 A8A494C3…2BE6, its DLLs signed by Microsoft, checked 2026-11-09); allowed by Thomas 2026-11-09 04:48 (I-0132 R11-PLAN §5); never in the game folder | built beside the module (`<build>/D3D12`), by default since I-0132 R11i (`no-agility.flag` the inbox runtime, which WARP and PIX keep; R11f's opt-in `agility.flag` retired once the pipeline states its 1.620 debug layer refused were cleaned) |
| The game's shader programmes (DXBC) read for a transcription (the rays' materials, skinning, wind) | `fxc /dumpbin` (Windows Kits 10.0.28000.0; `/Lx` for the literals' bits) the reference; 3Dmigoto's `cmd_Decompiler.exe -D -V` (1.3.16, `C:/Users/User/Tools/DS3-Decompilation/third-party/cmd_Decompiler-1.3.16`, zip SHA-256 5E72E067…A1B2, the exe unsigned) for a first HLSL draft with the fields' names, always checked against the disassembly (it turned the grass's `imin(8, count)` into `min(count, 0)`, 2026-11-03); on copies of the programmes, never in the game folder | installed by Thomas 2026-11-03 |
| Reaching a moment of the game in a test run (a boss, an event flag, a place, a cutscene) | the module's test files beside a build's module, never in the play launcher: `test-revive.txt` (the 26 registered boss flags; `-Bosses` of the test launcher, [I-0089 USAGE](../../../investigations/I-0089-test-tools/USAGE.md)) with F4 invincible, F5 revive again, F6 kill the strongest character within 25 m (one press a boss phase); `witness-flag.txt` « id value » (any event flag; `Run-AlphaCast -SetFlag "id v;id v"`); `witness-teleport.txt` « x y z hold » (the MSB frame; the player pinned for the hold; `-Teleport`, and `-Teleport2` with the T key step); `witness-cutscene.txt` « id [mode] » (any Remo cutscene by its id through the game's own EMEVD handler, I-0139; `-Cutscene`); the tour's runners `Run-I139Tour`, `Run-I139Boss`, `Run-I139Cutscenes`, `Run-I139Menus` ([I-0139 LIST](../../../investigations/I-0139-game-tour/LIST.md)) | in use (Thomas, 2026-11-13 18:21: the cutscene tool written here) |
| The native C++ | clangd (Native code navigation) | in use |
| Procedures and reviews | the project's skills and subagents (end of this file) | in use |
| Session start | A Claude Code hook in the main checkout's `.claude/settings.json`: `scripts/Session-Start.ps1` prints the checkout (read from the hook's input), STATE's resume rows, whether the game runs and C:'s free space (a warning under 100 GB, D-0260) | in use |
| A copy of the whole project | `tools/Backup-Repo.ps1` (Thomas, 2026-10-23): the folder, `.git` and worktrees, then the evidence store, mirrored to `D:/DS3-Backup` by robocopy (junctions not followed); started by Thomas's desktop shortcut | Thomas's; last copy 2026-10-28 |
| Waiting for a run, a build or another thread | end the turn and let the task notification, a reminder or a message wake the thread; Claude Code's `Monitor` for a stream of events. Never a sleep loop | in use |

## Background processes on Thomas's PC (Thomas, 2026-10-21, D-0199)

Every process a thread starts without needing a window (builds, compilers, scripts, PowerShell or cmd helpers, headless Ghidra,
readers, test runners) runs hidden: `Start-Process -WindowStyle Hidden` (or `-NoNewWindow` inside a hidden host),
`CREATE_NO_WINDOW` from native code, the same flag passed down. The game during an announced run is the exception.

## Thomas's PC and tools

- Windows, NVIDIA and security settings: pilots and scripts never change them, even with an agreement given in passing; the
  thread asks Thomas, who changes them himself. Windows Defender: no exclusion; if it removes or blocks the module again, stop
  and ask Thomas. One exception (Thomas, 2026-11-14 11:31, in the R thread: « je t'autorise à activer/désactiver le HDR Windows
  à ta convenance », and 11:37: « A inscrire également dans le dépôt »): a thread may switch Windows' HDR on and off as its work
  needs; a thread whose own session rules forbid changing a system setting (the R thread that day) does not, and asks Thomas
  for Win+Alt+B instead.
- Never touch Ponytail or OmniRoute (Thomas's other tools).
- Claude Code's guard (Thomas, 2026-11-07, relayed): a thread that plans commands the guard may refuse (code from outside, a new
  dependency, a build that compiles it) gives Thomas beforehand, in one message, the exact `allow` rules to add to the main
  checkout's `.claude/settings.json` (`permissions.allow`, never `deny`); no bypass mode. Thomas's yes in a thread does not lift the
  guard. A rule matches the command as typed (the build: `pwsh -NoProfile -Command "& { & '<…>/Build-GameBootstrap.ps1' … }"`, no
  `cd … &&` before it).
- Git on Thomas's side: a push and a mass Git deletion (branches, worktrees, tags) are given to Thomas as a command to paste,
  never run by a thread. No GitHub copy and no Codex command any more (Thomas, 2026-11-08).

## Merging into `main`

Procedure: skill `merge-workstream`. The rules it applies:

- `main` is merged into the branch first: the reviewed tree is the tree that lands.
- Reviews (D-0198): a step review after each step, and the full review of the branch diff against `main` before every merge,
  run on the final tree (after the optimisation pass and `main` merged in; Roles). The pilot fixes what they find or records why
  not; the review and its outcome are noted in the RESULT. They look first for neutralised or dead code, missing resource transitions
  and barriers, and departures from the mandate (no D3D11 bridge, wrapper or translation in production).
- The optimisation pass (Method rules). A visual workstream closes only after its final image is compared side by side with the
  original at every witness zone, both sky states where progression changes the light, the captures shown to Thomas (D-0181).
- **No measurement gate** (D-0227, Thomas, 2026-10-28: « fini les tests et les seuils, on passe en full production »): the
  replayed path gate (`scripts/Invoke-ReplayGate.ps1`, [REPLAY-GATE.json](../../REPLAY-GATE.json)), its calibration, its memory leg
  and the night endurance are not run before a merge; they stay as tools, used on Thomas's request and in the full control.
- **The full control every three days** (D-0230): at the first clean checkpoint three days after the last one (the first from
  2026-10-31), the gate with its memory leg, the night endurance and the control sizes, while Thomas sleeps (the day) or works
  (the night, about 19:30 to 07:00), on the ASUS alone with the OLED off, announced, stopped on « je prends le PC » or « je
  joue »; a regression it finds is the next workstream. STATE's « Next action » carries its due date. The final review by a fresh
  Opus at xhigh stays required (Reviews).
- After the merge: rebuild the launcher's module from `main` in the main checkout while no game runs
  (`native/frame/Build-GameBootstrap.ps1`, its unit tests included; default output `evidence/I-0066/artifacts/game-bootstrap-build`,
  the folder the play launchers `tools/Launch-DS3-DX12*.cmd` copy: never clean it); **the start check** (D-0229): `scripts/Invoke-StartCheck.ps1 -Commit
  <main's short hash>`, announced in one line like every run (start and end times, 5 minutes ahead, « PC libre » after it),
  one minute aimed at and two at most (launched, to the world on the Firelink path's save, closed by its close box; offline,
  restored by hash, the ASUS by its key; no measure, no other input), `passed` true; a failed check is fixed on a new branch
  before the next workstream and said to Thomas;
  make the clean checkpoint (STATE with « Pilot branch | none » and « Active workstream | None open », a clean tree, a commit);
  delete the subagents' worktrees and folders; the documentation cleanup (Threads and continuity) is done and
  `scripts/Test-Project.ps1` passes from the main checkout.

## Night endurance (D-0204)

Not run before a merge since D-0227; `investigations/I-0085-night-endurance/Run-Endurance.ps1` runs in the full control every
three days (D-0230, Merging into `main`) and on Thomas's request.

## Evidence and checkpoints

- Raw artifacts (D-0142, hard rule): keep only what reproduces, audits or falsifies a claim (protocol, scripts, run records, logs,
  frame tables, comparison pictures, the inputs current tools read); saved frames, session exports, captures, dumps, build
  intermediates and temporary folders are disposable cache, deleted when the cycle that read them closes. Save only the frames an
  analysis needs; check free disk space before a live run. Retention and its exclusions:
  [EVIDENCE-RETENTION.json](../../EVIDENCE-RETENTION.json); never restore, recreate or reacquire removed evidence automatically.
- Disk guard (D-0260): a build into its own `-Out` folder (a new one or an earlier build's) deletes its `.obj` once linked
  (`-KeepObjects` for a later `-ChangedUnits` build) and hard-links NVIDIA's DLLs; no build under 10 GB free; `scripts/Clear-DiskCache.ps1 -Apply` at each
  merge (skill merge-workstream) and whenever Session-Start reports C: under 100 GB; Run-StartupTrace moves the run save backups
  beyond the newest 100 to `D:/DS3-save-backups-archive` (`scripts/Move-SaveBackups.ps1`). A build folder carries `ds3-build.marker`;
  a run folder or build a later step must keep gets a `keep.txt` or a line in the store's `_maintenance/rotation-keep.txt`, and a
  build folder the repository's Markdown links into, or holding a report, is never rotated (raw frames stay only when
  linked to themselves).
- The Evidence Store is `C:/Users/User/Desktop/DS3_evidences`, outside Git, seen through the ignored `evidence/` junction
  (`scripts/Connect-EvidenceStore.ps1` in a new checkout): never replace the junction by a local copy, never force-add evidence
  (STATE « Storage and recovery »).
- Keep experiments reversible; commit coherent validated changes with their STATE and ledger updates; do not mix unrelated
  investigations. Large or licensed artifacts stay outside Git with manifest, identity, location and hash.
- On a handoff, pause or session end, update STATE's short checkpoint on the working branch: its rows « Pilot branch », « Active
  workstream » (the workstream, its last proved state and commit, prototype status, validations, evidence, unknowns, integration
  status), « Last finished » and « Next action ».

## Communication with Thomas

French, « tu », Paris times read from the PC clock, short; technical terms in English inside the French sentences (Thomas,
2026-10-26 05:31: shadow cascades, frame pacing, swap chain, delta time). The game's spells, items, characters and places by their English names as the
game displays them (Thomas, 2026-10-28 23:04: Fire Orb, Lightning Spear, Firelink Shrine). A message that needs his action starts with « ⚠ Action requise ». Every
claim names its source (a file, a run, a line) or says « connaissance générale ». Private notes between the threads and the coordinator are free, on any subject, Thomas included, without telling him: they serve to challenge the others' choices and reasoning, against a single pilot's tunnel (Thomas, 2026-11-02, D-0246); anything that needs Thomas's decision, and every opinion or proposal that concerns him, goes to him directly in the thread (D-0224). The threads and the coordinator may discuss and contest Thomas's choices and
decisions: the objection is said to him frankly in the thread, with its reasons and evidence; what he then settles is
applied (Thomas, 2026-11-14, D-0277).

## Skills and subagents of the project

Every thread, pilot or study, uses a skill whenever one covers the procedure at hand instead of redoing it by hand (Thomas,
2026-11-01): the project's skills below, and the session's own skills (for example a code review, a simplification pass,
the document and spreadsheet skills). A skill found missing or wrong for a recurring procedure is fixed or proposed, not
worked around.

| Name | Kind | Use |
|---|---|---|
| `resume-thread` | skill | Starting or resuming a thread: route, branches, evidence junction, clangd, the Pilot branch row |
| `live-run` | skill | Preparing, announcing, running and closing a live run |
| `merge-workstream` | skill | Reviews, merge into `main`, the launcher's rebuild, the one-minute start check and the clean checkpoint |
| `review-step` | skill | A step review: the brief (the diff, the goal, the criteria, never the pilot's summary; to find what is wrong, D-0283) and the `step-reviewer`'s launch |
| `review-final` | skill | The reviews before a merge: the `final-reviewer`'s brief on the final tree (as `review-step`'s) and the `integrity-reviewer` |
| `step-reviewer` | subagent | A fresh Opus at high effort reviewing one step's diff |
| `final-reviewer` | subagent | A fresh Opus at very high effort reviewing a workstream's final diff |
| `integrity-reviewer` | subagent | A fresh Opus before every merge, looking only for reward hacking (Guarding the judge, D-0268) |
| `measure-tools` | subagent | Sonnet: the measuring tools' trial, journals read, numbers computed, no interpretation |
| `ghidra-research` | subagent | Sonnet: targeted references and decompilation, read-only, GhidraMCP first, headless on the mirrors otherwise |
