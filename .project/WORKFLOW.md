# Operating workflow

The rules in force, in the present tense, each with the decision ([decisions/LOG](../decisions/LOG.md), indexed in
[IN-FORCE](../decisions/IN-FORCE.md)) or Thomas's instruction it comes from. Their dated wording and quotations are in
[archive/2026-11-15/WORKFLOW.md](archive/2026-11-15/WORKFLOW.md) and Git tags `archive/pre-cleanup-2026-10-25` and
`archive/pre-cleanup-2026-10-02` (D-0291). A new rule replaces the sentence it changes; it is not appended beside it. Repeated
procedures live in the project's skills (`.claude/skills/`, listed at the end); the tools' detailed rows in [TOOLS](TOOLS.md).

## First rule of wave 2: the architecture of Elden Ring (Thomas, 2026-10-20, D-0195)

FromSoftware already modernised this engine with D3D12 in Elden Ring: the native backend copies that architecture, then
improves it. Every wave-2 design (scene persistence, binding, pipelines, queues, loading and threading, effects) starts from the
ER mechanism recovered by static decompilation of Elden Ring 2.7.1 (read-only, headless Ghidra on a copy; Sonnet 5.5 may carry
the decompilation, Opus checks the sources a design relies on) and states how DS3 adapts it. Only the data differ: DS3's
effects, parameters, models, shaders, layouts and behaviour are verified on DS3. The existing native work is transformed, not
thrown away. ER's known defects (pipelines compiled in play, streaming hitches) are not copied: each design names where ER has
the defect (from its decompilation or as public knowledge, said which) and what the native backend does better.

## Reporting: honesty first (Thomas, 2026-10-23)

A bad result, a failure, a skipped step or an uncertainty is said as it is, in the thread, a RESULT or a commit. A gain within the
noise is reported as no gain; a step not run is named as not run. Observations and conclusions stay distinct: question →
hypotheses → experiment → observations → conclusion → canonical update.

## Canonical sources

[`INDEX.md`](INDEX.md) owns the routing table. STATE and the ledger summarise and link; they never copy evidence, derivations or
protocols. STATE holds the current position only: its resume rows are rewritten at each checkpoint, never appended to, and its
« Updated » line names the last checkpoint alone (D-0291). A study or report that guides a workstream is copied into that
workstream's investigation in its first commit; a file in the project's shared folder is never the only copy of a canonical
text (D-0214). The timing project under [`timing/`](../timing/IMPORT.md) keeps its own historical numbers (D-0196).

**Short documents** (Thomas, 2026-11-15, D-0291): each control file has a size target, and `Test-Project` warns above it
(STATE 14 KB, WORKFLOW 36 KB, ROADMAP 24 KB, IN-FORCE 40 KB, the investigations index 28 KB, CLAUDE.md 3.5 KB). History never
accumulates in them: a finished step keeps one line in ROADMAP and the investigations index, its account goes to its RESULT, a
decision's full text to the LOG and one line to IN-FORCE. A block of history or detail a control file loses is moved to
`.project/archive/<date>/`, never deleted without Thomas; a single stale, contradictory or duplicated sentence may simply be
removed (Git keeps it).

## Roles and delegation

- **Thomas**, project director: goals, ambition, scope, strategic trade-offs, protected authorizations, reporting decisions,
  Windows, NVIDIA and security settings, pushes and mass Git deletions.
- **The coordinator** (Claude, in the project chat): relays Thomas, opens threads, routes reports; its rules are the repository's.
- **The pilot** (by default Opus 5.5 at high effort, D-0245; the former « Sol »): owns the open workstream: framing,
  hypotheses, interpretation, claim statuses, integration, checkpoints (D-0114). One pilot thread at a time per workstream.
- **The threads' short names** (Thomas, 2026-11-06): **R** the RT/PT pilot (from RT-PT-PLAN §8's workstreams R0 to R11,
  D-0235), **E** the open-gaps pilot (« écarts »), **S** the study thread.
- **The study thread** (one at a time in the normal flow, for either pilot; Thomas, 2026-10-30): one workstream ahead, read-only on the code; it
  merges into `main` only its documentation-only commits, when no pilot is merging or rebuilding (D-0239); no load on the PC
  during a live test (its readers at idle priority). Its report is filed for the next pilot, who copies it into the
  investigation. ER studies ahead are made there by Sonnet subagents.
- **Subagents** (D-0159, D-0225, D-0245): the pilot delegates, to save its own context. **Mechanical, quick tasks that need
  little complex reasoning always go to Sonnet 5.5 subagents**, at the effort level the thread chooses: reformatting or ordering a file, a table or a list; counting or
  sorting log lines; extracting numbers from a record; one small change applied at places already listed; a keyword sweep.
  Sonnet 5.5 is also the default for reading journals and evidence, computing measures, analyses, decompilation reading
  (I-0044) and reviews of documents; Opus 5.5 at medium effort beside them. Each delegation states question, scope, inputs,
  non-goals, expected output and exit condition. Worker output is non-canonical until the pilot checks the primary evidence;
  framing, ambiguous interpretation and project-level conclusions are never delegated.
- **Reviewers** (D-0198, never reduced by the usage limit): a fresh Opus 5.5 at high effort after each step; before every merge
  into `main` (a tranche or a workstream's last), a fresh Opus at xhigh reviews the whole branch diff against `main`, an agent
  that neither wrote the code nor saw the thread. Both look for what is wrong, not to confirm that it works (D-0283): the brief
  gives the diff, the goal and the exit criteria, never the pilot's summary; the reviewer reads the code first and tests the
  records against it. Subagents `step-reviewer` and `final-reviewer` carry these briefs; skills `review-step` and
  `review-final` write and launch them.
- **Helper subagents** (Thomas, 2026-10-27): `measure-tools` (Sonnet) for the measuring tools' trial, reading journals and
  computing numbers, returned without interpretation; `ghidra-research` (Sonnet) for targeted references and decompilation,
  read-only, its result re-read by a fresh Opus when a decision depends on it. The pilot keeps the conclusions.
- **Fable 5.2** (the former « Astra »): only with Thomas's explicit agreement (D-0198); Opus at high effort does its tasks by
  default.

Work that needs judgement stays with the thread unless the gain exceeds the coordination cost. Explicit assignments by Thomas
take precedence. Preserve unresolved disagreement and negative results. The optimisation review and the audits (menus,
resolutions, full screen) stay with Opus at high effort. Historical records say Sol, Luna and Astra: read Opus, Haiku and
Fable. An authorization already given for a task needs no new request.

## Threads and continuity

- **The pilot's branch.** A workstream runs in its own worktree branch. The thread that opens it, and every fresh thread that
  continues it on a new branch, writes its bare branch name in STATE's « Pilot branch » row and the workstream in « Active
  workstream » in its first commit. The row says `none` only when no workstream is open.
- **Waiting.** A thread waits by ending its turn (a reminder, a task notification or a message wakes it), never in a sleep
  loop; a wait on a run or a build is a harness-tracked background command. No stop hook holds a thread (D-0234).
- **Full production** (D-0227): short tranches, each merged once its code is written and reviewed, with no measurement gate,
  thresholds, series or night endurance and no live run before a merge; after every merge the module builds with its unit
  tests and a one-minute start check runs (D-0229). A measure is taken only when a fix cannot be written without it (said to
  Thomas) or on his request. Short announced witness runs (about ten minutes) are welcome as diagnostics, never as a merge
  gate; the full control stays every three days (D-0230).
- **Documentation cleanup before every checkpoint and every merge** (Thomas, 2026-10-25 and 2026-11-15, D-0291): before the
  commit that closes a step, a handoff or a merge, the thread cleans the documentation it touched: STATE's rows rewritten to
  the current position (history to the RESULT); stale, contradictory or duplicated sentences removed from STATE, the RESULT,
  ROADMAP, WORKFLOW and the notes it changed; a rule changed in one place and not left in its old form elsewhere (memory files
  included: they only point to the repository); links checked; `Test-Project` without a STATE or size warning (from the main checkout at the clean
  checkpoint after a merge). At every clean checkpoint the
  thread also deletes the worktrees and folders its own subagents made. A smaller subagent may do the pass, then the thread
  checks it.
- **A fresh thread at every clean checkpoint** (D-0215): a coherent commit, STATE current, merged into `main` when the step
  merges, without waiting for the thread to be loaded. If the new thread cannot open yet (at night, before Thomas allows its card), the current thread goes on and changes
  at the next checkpoint. A new workstream starts only from a clean, validated `main` resumable from the canonical sources alone.
- **Completion (D-0129).** A workstream that meets its exit criteria and is validated is merged into `main` by its pilot, and the
  next one starts without waiting for Thomas (he is informed). A tranche (or a step whose merge its plan names) is merged the same
  way once written and reviewed (D-0227). Any other merge in mid-workstream needs Thomas.
- **Thomas plays.** On « je joue » every thread pauses (no live run, build, Ghidra or heavy agent) until his « fini ». On « je
  prends le PC » the pilot only reads and writes code (no build, no live run, no Ghidra) until his « fini ». His save is never
  overwritten: the launcher backs it up and restores nothing; a test run backs up the save present at its start and restores it
  at its end; tests use copies, never his own characters except Alpha as the Live game rule allows or on his request.
- **`PAUSE`.** Start nothing new, finish only an atomic operation, stop instrumentation, restore the game and environment,
  preserve evidence, update the resume checkpoint, validate, then wait. It never closes the workstream nor authorizes a merge.
- **Recovery.** Read STATE's checkpoint, inspect branches, worktrees and history, reconcile before repeating any acquisition;
  never assume `main` is newest. Recovery never authorizes a merge. After every context compression, reread INDEX's resume
  route, STATE, the current ROADMAP workstream, the mandate, [IN-FORCE](../decisions/IN-FORCE.md) and this file before any technical decision, implementation or live
  test (Thomas, 2026-10-09). Skill `resume-thread`.
- **Weekly usage limit**: at 50 % every pilot sends the coordinator a note, and the coordinator stops the open-gaps pilot after
  its next clean checkpoint while the RT pilot continues (D-0248); at 90 % slow down (fewer parallel agents, nothing optional)
  without reducing a review; at 99 % make a clean checkpoint and pause. Every pilot reads the limit at each
  checkpoint and adds the percentage to the line it sends the coordinator (D-0292).

## Investigations

How an investigation is opened, budgeted and closed: [METHOD « Investigations »](METHOD.md#investigations).

## Coverage by families (Thomas, 2026-10-06, D-0150)

Offline inventories by family, per-family or per-mechanism native rules (never one asset, D-0161), the settings axis and
the frame's inputs (D-0151): [METHOD « Coverage by families »](METHOD.md#coverage-by-families-thomas-2026-10-06-d-0150).

## Architectural frame challenge (anti-tunnel)

No native mini-workstream chosen by successive crashes (D-0156). Before a large tranche, a static-to-runtime step, a declared
implementation readiness, a high-consequence seam, or when one blocker persists (D-0040), the pilot answers the questions and
uses the recover / redesign / unknown frame of [METHOD « Architectural frame challenge »](METHOD.md#architectural-frame-challenge-anti-tunnel).

## Method rules

- **Optimise, do not stack** (Thomas, 2026-10-19): a correction replaces the path it corrects. No data is read, analysed or
  decoded twice for the same purpose; what changes only on an event is kept by content or generation and recomputed on that
  event; diagnostics run only behind their flag. Before every merge, one optimisation pass over the branch's additions (read;
  measured only on Thomas's request since D-0227).
- **Validation and RCA**: prefer deterministic tests, then instrumented observation, reproducible comparison, independent
  verification, model judgment. `SUPPORTED` is never silently treated as `CONFIRMED`; an uncertain result is validated before
  dependent work builds on it; documentation checks prove repository consistency only. After two failed local fixes, a
  recurrence, multiple regressions or lost causal understanding: stop patching and open an RCA from observations.
- **Traced lighting judged by its truth** (D-0249): in RT/PT the original's picture is not a pixel target; what is accepted is a
  natural and true lighting, in every scene; a difference from the original is not a defect in itself, a bug stays an anomaly to
  fix. No lighting cheat is simulated or created (D-0262): no fill light, baked ambient or cube standing in for a traced answer,
  no added fake light; only the real sources' settings are adjusted. The rays' inputs (materials, lights, geometry) are still
  read from the original; the raster the plan keeps (G-buffer, effects, UI, post) stays faithful to it; raster lighting the
  RT/PT replaces is not corrected (no full raster mode, D-0237; E fixes only what stays visible with the rays, D-0250).
- **Guarding the judge** (D-0268, against reward hacking): (1) a change to a test, an assertion, a tolerance or threshold, a
  gate's reference, a measuring flag or a reader is stated plainly in the RESULT with its reason; an unstated one counts as a
  failure. (2) Before every merge, after the final review, the `integrity-reviewer` subagent looks only for weakened tests,
  special cases for one scene, work cut during a measure and numbers without evidence; its findings are answered or recorded
  in the RESULT as the final review's. (3) Every visual gain is shown with before and after stills or films; Thomas's eye stays
  the final judge (D-0181, D-0249).
- **Original and native compared at the same moment** (D-0299): a comparison of the two modes (pictures, films, rows, counts)
  takes its samples on the game's own clock in both modes (a cutscene's time, an effect's or a spell's own start, an
  animation's frame), never on the runner's wall clock or a fixed delay after a key; each sample names that clock's value, and
  a pair whose values differ is not compared (I-0141 tranche 11: the smoke's two modes sampled 2.5 s apart in the scene gave
  « 35 more entities » where the same moment gives about 100).
- **No pop-in** (D-0197, D-0211): nothing is drawn with a stand-in; a model not ready appears later, as the last resort only.
- **Skip the object and go on** (FINISH-LINE line 1, I-0101): where the module meets per-object game data the original tolerates
  (a non-finite matrix, a menu family never witnessed, one object of an unknown kind), it skips that object for that frame or
  that admission, counts it and writes a rate-limited row naming it, instead of closing the game. A refusal stays for the
  module's own contracts: its hooks and pins, thread and scope checks, its registries' invariants, the device, the start-up.
  A skipped draw is not a stand-in (D-0197).
- **Before removing a stand-in or turning a refusal into a skip that draws nothing** (D-0290): read the original's answer for the
  same input, the arguments' own path included (a proxy's slot, a holder's inner texture), not only the function's return;
  find the caller that asks again when the change relies on a retry; and witness the case in every place the journals show it
  met (the row the stand-in or the refusal writes names them). A default the original binds itself (SYSTEX_BLACK behind a movie
  proxy, SYSTEX_DUMMY for a texture with no data) is the original's picture, not a stand-in.
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
- **Announcing.** Each run is announced at least 5 minutes ahead with the start and end times read from the PC clock just
  before (Paris), and whether it measures; « PC libre » at the end. **Every run slot goes through the coordinator** (Thomas,
  2026-11-06): a thread asks it for a slot, the coordinator gives and announces it and organises the runs of all the threads;
  the rest the threads settle between themselves. One DarkSoulsIII at a time: check the other sessions before a run or a
  build, and build into a folder of your own (`Build-GameBootstrap.ps1 -Out`, runners `-BuildFolder`).
- **The PC during a run.** Builds and Ghidra may run during a run that only captures or reads; a measuring run (cadence, frames
  per second, the full control) has the PC entirely free (Thomas, 2026-10-29). A measurement taken while Ghidra, a build or a
  heavy agent ran is run again, not interpreted. Two pilots in parallel (Thomas, 2026-10-30): the RT pilot and the open-gaps
  pilot (not the raster-lighting defects the RT replaces); the RT pilot has priority on the ASUS and the launcher rebuilds
  unless an open-gap fix is ready to test.
- **Before announcing**, check the run dry (Thomas, 2026-10-25): the exact command line, its effect flags accepted by
  `Run-StartupTrace.ps1`'s pattern, a capture writing where it should, a sampler whose start and end fall inside the path.
  Every run checks its launch itself (`scripts/Invoke-ReplayRun.ps1`, `launch-check.json`): the module and controller in the
  build folder and the test save at its hash before; after the settle, the module loaded and a native presenter at the
  screen's rate, else it stops before its path. While a series runs, no script it calls is edited.
- **Measures** (when one is taken: a fix that needs it, or Thomas's request; since D-0229 no live run verifies a fix before its
  merge). Before a series, one short run checks that each measuring tool (PresentMon, the sampler, a counter) attaches and
  writes its data (a tool that does not is fixed before the series), and a reader's figures are checked against one journal
  read by hand before they are reported (I-0100, D-0225). Runs are grouped: one announced slot holds every run the step needs (controls, sizes, before and after). Every
  control run of a change covers at least three render sizes, 1280x720, 1920x1080 and one arbitrary window (one universal rule,
  never a case by resolution). A measure names its screen (the ASUS by default, the OLED when only its rate, G-SYNC or full
  screen shows the change) and reads the display with PresentMon; 60 Hz and 280 Hz numbers are never mixed.
- **Test characters' progress.** A test save need not be restored after every run; a test character that reached a new zone is
  kept as a new hash-recorded test save (`Run-AlphaCast -KeepSaveLabel`). Thomas's own saves and Alpha are never overwritten.
  **Zone witnesses** (D-0174): reached with the **Shebang** save, one character other than
  **Alpha** per zone, warped to the zone's bonfire in a reference run, saved there and kept as a hash-recorded test save outside
  the game folder (as under `C:/Users/User/.ds3-i0039-backups/shebang-*`), installed with `Run-StartupTrace.ps1 -TestSave <sl2>
  -TestSaveSha256 <hash>`. Alpha is never modified; it may be used to test spells, miracles and pyromancies from a copy of the
  save only. Zone and item names are written in English, as the game shows them; Thomas's French names (Capharnaüm des Confins
  = Dreg Heap, Cité Enclavée = Ringed City, Village or Camp des morts-vivants = Undead Settlement) are only to recognise them.
- **Screens.** The OLED never stays on without Thomas (its care, 2026-10-26): when he is away, every test runs on the ASUS
  alone and nothing is left displayed on the OLED. When the OLED is on, the tests run on it (Thomas, 2026-11-06), the gate's
  required ASUS base apart (D-0218), in a 2560x1440 window, never a small window over the still desktop; when Thomas turns the
  OLED on for a test, it is the only screen used and the window at its full resolution, so that the desktop never shows
  around it (Thomas, 2026-11-14; `-GraphicsLevels
  'ScreenMode=WINDOW;Resolution-WindowScreenWidth=2560;Resolution-WindowScreenHeight=1440'`, which the start check and the
  endurance take). The endurance tour's inputs are not yet right at that size: until they are, the endurance runs on the ASUS.
- PIX (`C:/Program Files/Microsoft PIX/2603.25`) and Nsight Systems (`C:/Program Files/NVIDIA Corporation/Nsight Systems
  2026.5.1`) are used when they answer better than the journals, the sampler and PresentMon (Thomas, 2026-10-25; [TOOLS](TOOLS.md)).
- Know-how (pilots, menus, coordinates, pitfalls): [LIVE-RUN-NOTES](LIVE-RUN-NOTES.md). Procedure: skill `live-run`.

## Tools

Use the tool that answers the question best; say which one a measure comes from. Installing a new program on Thomas's PC (a
profiler, Python, a Ghidra extension, an MCP server) is his decision: the thread asks him with the exact command and what it
changes. Each tool's paths, versions, use and state on this PC are its row in [TOOLS](TOOLS.md); the external tools study is
[TOOLS-STUDY](TOOLS-STUDY.md). The constraints that bind every thread:

- PIX and RenderDoc captures: never on Thomas's save, never during a measure. Timing captures (Nsight, PIX) and Windows
  Performance Recorder need administrator rights: ask Thomas.
- Ghidra (D-0269, D-0272): GhidraMCP on Thomas's GUI first, never closed by a thread; names and head comments written there by
  one session at a time, the holder of the token file `Tools/DS3-Decompilation/ghidra-mcp-ds3.writer` (taken when absent or
  older than 4 h, removed at the end), a name only where the repository states the role (`<Role>_<address>`, a row of
  `decompiled/GHIDRA-NAMES.tsv`); after a pass of names Thomas saves, then the writer remakes the three DS3 mirrors; the
  mirrors, `game/ghidra` and the `ghidra-research` subagents read only; never a DS3 mirror under Ghidra 12.1.3; a function
  encrypted at rest read through the decrypted-code overlay before any loaded-code run; the executable and its protection
  never patched.
- NVIDIA's and Microsoft's SDKs and DLLs stay in `Tools/DS3-Decompilation/third-party/`, beside the module, never in the game
  folder; the game's shader programmes are decompiled on copies only, the draft always checked against the disassembly.

In short:

| Need | Tool |
|---|---|
| The display's cadence | PresentMon 2.6.0, unelevated (D-0209) |
| CPU time; one clock for step, waits, presents, vblanks | the native sampler (`sample-profile.flag`); `cadence-trace.flag` |
| A CPU, GPU and vblank timeline; a native frame's GPU work | Nsight Systems, PIX (`pix-capture.flag`, `pixtool.exe`; developer mode on) |
| Native against original pictures | the pair sheets, `Compare-SceneHdr.ps1`; `flip.exe` (not FLIP's Python) |
| A film's motion | `scripts/video/videosheet.py` (judged by its shape in time, never a region's mean; never a sheet of frames) |
| What Thomas saw in play | his NVIDIA clips, frames by ffmpeg |
| A frame of the original (D3D11) | RenderDoc 1.47 |
| DLSS, Ray Reconstruction, frame generation, Reflex | Streamline 2.14.16 (D-0165) |
| Decompilation | GhidraMCP first; headless Ghidra 12.1.4 on the DS3 mirrors (12.1.3 for Elden Ring); one writer of names at a time (D-0269); the executable never patched (D-0272) |
| D3D12's newer runtime | the Agility SDK 1.620.1, beside the module, never in the game folder |
| The game's shader programmes | `fxc /dumpbin`; 3Dmigoto's decompiler for a draft, checked against the disassembly |
| Reaching a moment of the game | the module's test files (`test-revive.txt`, `witness-flag.txt`, `witness-teleport.txt`, `witness-cutscene.txt`), never in the play launcher |
| The game tour as a reference (D-0284) | [I-0139 REFERENCE](../investigations/I-0139-game-tour/REFERENCE.md) and its `REFERENCE.tsv`; a replay names one screen and one build |
| The native C++ | clangd |
| Building the module | `native/frame/Build-GameBootstrap.ps1`: its commands in parallel by Ninja (`-Jobs`, 12 by default; lower it while another thread captures), `-Serial` the old serial path; `-Cache <folder>` a thread's incremental build, published into `-Out`; a merge and the launcher build complete (I-0142) |
| Session start; a copy of the project | `scripts/Session-Start.ps1` (hook); `tools/Backup-Repo.ps1` (Thomas's) |
| Waiting for a run, a build or another thread | end the turn, a task notification or `Monitor`; never a sleep loop |

## Background processes on Thomas's PC (Thomas, 2026-10-21, D-0199)

Every process a thread starts without needing a window (builds, compilers, scripts, PowerShell or cmd helpers, headless Ghidra,
readers, test runners) runs hidden: `Start-Process -WindowStyle Hidden` (or `-NoNewWindow` inside a hidden host),
`CREATE_NO_WINDOW` from native code, the same flag passed down. The game during an announced run is the exception.

## Thomas's PC and tools

- Windows, NVIDIA and security settings: pilots and scripts never change them, even with an agreement given in passing; the
  thread asks Thomas, who changes them himself. Windows Defender: no exclusion; if it removes or blocks the module again, stop
  and ask Thomas. One exception (Thomas, 2026-11-14): a thread may switch Windows' HDR on and off as its work needs; a thread
  whose own session rules forbid changing a system setting does not, and asks Thomas for Win+Alt+B instead.
- Never touch Ponytail or OmniRoute (Thomas's other tools).
- Claude Code's guard (Thomas, 2026-11-07): a thread that plans commands the guard may refuse (code from outside, a new
  dependency, a build that compiles it) gives Thomas beforehand, in one message, the exact `allow` rules to add to the main
  checkout's `.claude/settings.json` (`permissions.allow`, never `deny`); no bypass mode. Thomas's yes in a thread does not lift
  the guard. A rule matches the command as typed (the build: `pwsh -NoProfile -Command "& { & '<…>/Build-GameBootstrap.ps1' … }"`,
  no `cd … &&` before it).
- Git on Thomas's side: a push and a mass Git deletion (branches, worktrees, tags) are given to Thomas as a command to paste,
  never run by a thread. No GitHub copy and no Codex command any more (Thomas, 2026-11-08).

## Merging into `main`

Procedure: skill `merge-workstream`. The rules it applies:

- `main` is merged into the branch first: the reviewed tree is the tree that lands.
- Reviews (D-0198, D-0283): a step review after each step, and the full review of the branch diff against `main` before every
  merge, on the final tree (after the optimisation pass and `main` merged in). The pilot fixes what they find or records why
  not; the review and its outcome are noted in the RESULT. They look first for neutralised or dead code, missing resource
  transitions and barriers, and departures from the mandate (no D3D11 bridge, wrapper or translation in production).
- The documentation cleanup (Threads and continuity, D-0291) is done before the merge, not only after it.
- The optimisation pass (Method rules). A visual workstream closes only after its final image is compared side by side with the
  original at every witness zone, both sky states where progression changes the light, the captures shown to Thomas (D-0181).
- **No measurement gate** (D-0227): the replayed path gate (`scripts/Invoke-ReplayGate.ps1`, [REPLAY-GATE.json](REPLAY-GATE.json)),
  its calibration, its memory leg and the night endurance (`investigations/I-0085-night-endurance/Run-Endurance.ps1`, D-0204)
  are not run before a merge; they stay as tools, used on Thomas's request and in the full control.
- **The full control every three days** (D-0230): at the first clean checkpoint three days after the last one, the gate with its
  memory leg, the night endurance and the control sizes, while Thomas sleeps (the day) or works (the night, about 19:30 to
  07:00), on the ASUS alone with the OLED off, announced, stopped on « je prends le PC » or « je joue »; a regression it finds is
  the next workstream. STATE's « Next action » carries its due date.
- After the merge: rebuild the launcher's module from `main` in the main checkout while no game runs
  (`native/frame/Build-GameBootstrap.ps1`, its unit tests included; default output `evidence/I-0066/artifacts/game-bootstrap-build`,
  the folder the play launchers `tools/Launch-DS3-DX12*.cmd` copy: never clean it); **the start check** (D-0229):
  `scripts/Invoke-StartCheck.ps1 -Commit <main's short hash>`, announced in one line like every run, one minute aimed at and
  two at most (launched, to the world on the Firelink path's save, closed by its close box; offline, restored by hash, the
  ASUS by its key; no measure, no other input), `passed` true; a failed check is fixed on a new branch before the next
  workstream and said to Thomas. Then the clean checkpoint (STATE with « Pilot branch | none » and « Active workstream | None
  open » when the workstream is finished, a clean tree, a commit), the subagents' worktrees and folders deleted, and
  `scripts/Test-Project.ps1` passing from the main checkout.

## Evidence and checkpoints

- Raw artifacts (D-0142, hard rule): keep only what reproduces, audits or falsifies a claim (protocol, scripts, run records, logs,
  frame tables, comparison pictures, the inputs current tools read); saved frames, session exports, captures, dumps, build
  intermediates and temporary folders are disposable cache, deleted when the cycle that read them closes. Save only the frames an
  analysis needs; check free disk space before a live run. Retention and its exclusions:
  [EVIDENCE-RETENTION.json](EVIDENCE-RETENTION.json); never restore, recreate or reacquire removed evidence automatically.
- Disk guard (D-0260): a build into its own `-Out` folder deletes its `.obj` once linked (`-KeepObjects` for a later
  `-ChangedUnits` build) and hard-links NVIDIA's DLLs; no build under 10 GB free; `scripts/Clear-DiskCache.ps1 -Apply` at each
  merge and whenever Session-Start reports C: under 100 GB; Run-StartupTrace moves the run save backups beyond the newest 100 to
  `D:/DS3-save-backups-archive` (`scripts/Move-SaveBackups.ps1`). A build folder carries `ds3-build.marker`; a run folder or
  build a later step must keep gets a `keep.txt` or a line in the store's `_maintenance/rotation-keep.txt`, and a build folder
  the repository's Markdown links into, or holding a report, is never rotated (raw frames stay only when linked to themselves).
- The Evidence Store is `C:/Users/User/Desktop/DS3_evidences`, outside Git, seen through the ignored `evidence/` junction
  (`scripts/Connect-EvidenceStore.ps1` in a new checkout): never replace the junction by a local copy, never force-add evidence
  (STATE « Storage and recovery »).
- Keep experiments reversible; commit coherent validated changes with their STATE and ledger updates; do not mix unrelated
  investigations. Large or licensed artifacts stay outside Git with manifest, identity, location and hash.
- On a handoff, pause or session end, update STATE's short checkpoint on the working branch: « Pilot branch », « Active
  workstream » (the workstream, its last proved state and commit, prototype status, validations, evidence, unknowns,
  integration status), « Last finished » and « Next action ».

## Communication with Thomas

French, « tu », Paris times read from the PC clock, short; technical terms in English inside the French sentences (shadow
cascades, frame pacing, swap chain, delta time). The game's spells, items, characters and places by their English names as the
game displays them (Fire Orb, Lightning Spear, Firelink Shrine). A message that needs his action starts with « ⚠ Action
requise ». Every claim names its source (a file, a run, a line) or says « connaissance générale ». Private notes between the
threads and the coordinator are free, on any subject, Thomas included, without telling him: they serve to challenge the others'
choices and reasoning, against a single pilot's tunnel (D-0246); anything that needs Thomas's decision, and every opinion or
proposal that concerns him, goes to him directly in the thread (D-0224). The threads and the coordinator may discuss and
contest Thomas's choices: the objection is said to him frankly in the thread, with its reasons and evidence; what he then
settles is applied (D-0277). At each checkpoint a pilot may also propose to Thomas one idea outside the plan (a rendering
technique, a tool, a shortcut) to surprise him; it is made only with his agreement (D-0298).

## Skills and subagents of the project

Every thread, pilot or study, uses a skill whenever one covers the procedure at hand instead of redoing it by hand (Thomas,
2026-11-01): the project's skills below, and the session's own skills (a code review, a simplification pass, the document and
spreadsheet skills). A skill found missing or wrong for a recurring procedure is fixed or proposed, not worked around.

| Name | Kind | Use |
|---|---|---|
| `resume-thread` | skill | Starting or resuming a thread: route, branches, evidence junction, clangd, the Pilot branch row |
| `live-run` | skill | Preparing, announcing, running and closing a live run |
| `merge-workstream` | skill | The documentation cleanup, reviews, merge into `main`, the launcher's rebuild, the one-minute start check and the clean checkpoint |
| `review-step` | skill | A step review: the brief (the diff, the goal, the criteria, never the pilot's summary; to find what is wrong, D-0283) and the `step-reviewer`'s launch |
| `review-final` | skill | The reviews before a merge: the `final-reviewer`'s brief on the final tree (as `review-step`'s) and the `integrity-reviewer` |
| `review-security` | skill | A security review: the `security-reviewer`'s brief on a diff or a sweep, its findings answered and kept in the audit's report (Thomas, 2026-11-23) |
| `step-reviewer` | subagent | A fresh Opus at high effort reviewing one step's diff |
| `final-reviewer` | subagent | A fresh Opus at very high effort reviewing a workstream's final diff |
| `integrity-reviewer` | subagent | A fresh Opus before every merge, looking only for reward hacking (Guarding the judge, D-0268) |
| `security-reviewer` | subagent | A fresh Opus looking only for what in our code could harm a player or leak if published (a diff or a whole sweep; Thomas, 2026-11-23); when it runs is Thomas's to settle |
| `measure-tools` | subagent | Sonnet: the measuring tools' trial, journals read, numbers computed, no interpretation |
| `ghidra-research` | subagent | Sonnet: targeted references and decompilation, read-only, GhidraMCP first, headless on the mirrors otherwise |
