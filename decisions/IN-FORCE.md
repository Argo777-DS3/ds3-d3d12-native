# Decisions in force

One line per decision still in force, grouped by theme; the full text is in [LOG](LOG.md), whose text wins where a line here
is shorter than it. Written on 2026-11-15 (D-0291); every new decision adds its line here in the commit that records it in the
LOG, and a decision that replaces another moves the old one to the table at the end. A thread reads this file, not the LOG,
unless a question needs a decision's full text. Timing decisions D-0003 to D-0028 are in [timing/](../timing/IMPORT.md); the decisions consumed before D-0177 are
the LOG's « Consumed or superseded (titles only) » table. A decision recorded on a branch not yet merged appears here when its branch merges.

## Governance, threads and communication

- **D-0001** (2026-09-19): the project runs on persistent state, a capability roadmap, evidence-backed claims, bounded investigations and consequential decisions; work alone by default.
- **D-0002** (2026-09-19): no DS3 claim becomes canonical without provenance tied to the build, input identity, experiment and artifact location or hash.
- **D-0040** (2026-09-25): the architectural frame challenge of WORKFLOW runs at consequential gates.
- **D-0118** (Thomas): new sessions and investigations are pre-authorised; Remote Control is turned on at session start.
- **D-0129** (Thomas): a workstream that meets its exit criteria is merged into `main` and the next one starts without waiting; mid-workstream merges still go to Thomas.
- **D-0214** (Thomas, 2026-10-25): the repository is the only reference for the working rules (the coordinator's sheet only points to it); a study thread always runs one workstream ahead, read-only, no merge but its documentation-only commits (D-0239), no load on the PC during a live test.
- **D-0215** (Thomas, 2026-10-25): the working thread changes at every clean checkpoint, without waiting for it to be loaded (the gate-raise clause is in the gate bullets below).
- **D-0217** (Thomas, 2026-10-25): STATE's resume rows are rewritten at each checkpoint, never appended to, with a « Pilot branch » row; WORKFLOW states rules in the present tense; procedures are project skills, reviewers subagents; commits signed « Thomas (DS3 Rekindled) »; branch deletions stay Thomas's. Its stop guard removed by D-0234.
- **D-0234** (Thomas, 2026-10-29): no stop hook (`Stop-Guard.ps1`, `Set-StopAllowed.ps1` deleted); a thread waits on a run or build with a harness-tracked background command.
- **D-0239** (Thomas, 2026-10-30): a study thread merges its documentation-only commits into `main` itself (no `native/` code, no script a build or run uses), when no pilot is merging or rebuilding; `Test-Project` first, coordinator told; code and scripts stay the pilots' to merge.
- **D-0224** (Thomas, 2026-10-27): every opinion or proposal that concerns Thomas goes to him directly in the thread (its « private notes carry only test times » part replaced by D-0246; its order replaced, see below).
- **D-0246** (Thomas, 2026-11-02): private notes between threads and the coordinator are free, on any subject, Thomas included, to challenge others' choices; anything needing Thomas's decision or concerning him goes to him in the thread.
- **D-0248** (Thomas, 2026-11-05): at 50 % of the weekly limit every pilot sends the coordinator a private note; the coordinator then stops the open-gaps pilot after its next clean checkpoint; the RT pilot continues; the 90 % slow-down and 99 % pause are unchanged.
- **D-0292** (Thomas, 2026-11-15): every pilot reads the weekly limit at each checkpoint and adds the percentage to its checkpoint line for the coordinator (D-0248's thresholds unchanged).
- **D-0261** (Thomas, 2026-11-08): no GitHub copy command and no Codex command any more (skill `codex-github-copy` and AGENTS.md removed); CLAUDE.md is the only entry point; `C:/Users/User/.codex/I0063-cache` stays for the build's controls.
- **D-0273** (Thomas, 2026-11-12): the general plan (phases 0 to 4, workstreams 1 to 21) replaces the roadmap and its orders ([ROADMAP](../.project/ROADMAP.md)); FINISH-LINE stays as history, RT-PT-PLAN the RT/PT design of 2026-10-30 (historical since D-0291); capability names C1, C2, C3, C5 kept; its open decisions closed by D-0274.
- **D-0274** (Thomas, 2026-11-12): NVIDIA cards first (its exclusion lifted by D-0302 for AMD's FSR, upscaling and frame generation, and Intel's XeSS; a generic denoiser still excluded); no NVIDIA DLL distributed (the player supplies them); the GitHub copy stays light and private, Thomas updates it, no thread pushes, no decompiled code or game file on line, no public repository planned; before anything goes public, a complete licence and security review (workstream 18); records naming Fable say Fable 5.2.
- **D-0302** (Thomas, 2026-11-21): AMD cards get FSR 4.1 upscaling (RX 9000; RX 7000/6000 through Adrenalin 26.10.1, to check), FSR 3.1 upscaling for older cards and FSR 3.1 frame generation (the list from the coordinator's sheet); Intel's XeSS may be added (« last » the pilot's reading of his doubt); who tests them open (D-0274 said none; not asked); the rays on AMD not decided (workstream 14).
- **D-0277** (Thomas, 2026-11-14): every thread and the coordinator may discuss and contest Thomas's choices and decisions, frankly, with reasons and evidence, in the thread (D-0224); then what he settles is applied.
- **D-0291** (Thomas, 2026-11-15): the control files stay short, each under a size target `Test-Project` warns about; their history goes to the RESULTs, the LOG and `.project/archive/<date>/`, never deleted without Thomas; this index lists the decisions in force; the documentation is cleaned before every merge (skill `merge-workstream`).

## Delegation, models and reviews

- **D-0114, D-0159, D-0163** (Thomas): Opus 5.5 high is the pilot; Fable is replaced by Opus 5.5 high for its tasks, the pre-merge review included; any Fable use needs Thomas's prior agreement (D-0152). (« Sole technical owner » and the Haiku clause replaced by D-0245.)
- **D-0198** (Thomas, 2026-10-21): Fable only on Thomas's explicit agreement (Opus proposes, he decides, or he asks); a fresh Opus reviews each step, and a fresh Opus at xhigh reviews the whole branch before every merge into `main` (D-0198's own rule, restated by D-0218 and kept unchanged by D-0227), never reduced by the weekly limit; briefs per D-0283.
- **D-0181** (Thomas, 2026-10-16): its visual rule stays: a visual workstream closes only after a side-by-side comparison with the original at every witness zone (both sky states where the light changes), shown to Thomas; its quota rule is replaced by D-0198.
- **D-0225** (Thomas, 2026-10-27): the pilot groups live runs into fewer, fuller slots; a measuring tool and a reader are checked (a short trial, one journal read by hand) before their numbers are believed; delegate more to Sonnet subagents (Haiku clause replaced by D-0245).
- **D-0245** (Thomas, 2026-11-01): Opus 5.5 at high effort is the default pilot and owns its workstream; a pilot never yields its turn except for a human intervention, a decision that is Thomas's, a destructive or protected action, a real technical inability or an explicit `PAUSE`; mechanical, quick tasks always go to Sonnet 5.5 subagents, at the effort level the thread chooses.
- **D-0268** (Thomas, 2026-11-10): the judge is not touched silently (any change to a test, threshold, gate reference or measuring flag is stated with its reason, else a failure); before every merge an anti-cheat review by the integrity-reviewer subagent; before and after stills and films for every visual gain, Thomas's eye final.
- **D-0281** (Thomas, 2026-11-14): the next R thread runs on Opus 5.5 high (workstream 7's remainder, the unread settings, workstream 11's tranches 7 and 8); Fable 5.2 at high effort comes back for workstream 12 and for the Rekindled colorimetry only if his pick asks more than the three tranche-6 candidates.
- **D-0283** (Thomas, 2026-11-15): the step review (Opus 5.5 high) and the final review (Opus 5.5 xhigh) are briefed to find what is wrong, never to confirm, from the diff, the goal (CONTRACT) and the exit criteria, not the pilot's account; skills `review-step` and `review-final` write the briefs; the integrity review keeps its brief.

## Live tests, evidence, the PC and storage

- **D-0116, D-0117** (Thomas): Opus launches and drives DS3 itself for every investigation: protocol written and announced in chat first, game offline, protected files backed up and restored by hash, nothing installed in the game folder, a crash diagnosed before another run; online play and game-file changes go to Thomas.
- **D-0142** (Thomas): no raw artifact retention by default; keep only what reproduces, audits or falsifies a claim ([EVIDENCE-RETENTION.json](../.project/EVIDENCE-RETENTION.json)).
- **D-0166** (Thomas, 2026-10-12): every level of every graphics setting is tested live; the pilot may change Thomas's options, restored and checked by fingerprint after each run.
- **D-0174** (Thomas, 2026-10-13): zone witnesses through the Shebang save, one character other than Alpha per zone, saved there and kept as a hashed test save; Alpha never modified.
- **D-0199** (Thomas, 2026-10-21): every background process a thread starts runs hidden, no visible cmd or console window (headless Ghidra included); the game during an announced live test is the exception.
- **D-0201** (Opus, 2026-10-21): every native fault leaves a minidump and a controller-written report; DRED on by default (`no-dred.flag`); the D3D12 debug layer only with `debug-layer.flag` or `gpu-validation.flag`, absence tolerated; a fatal exception at a quit is dumped natively (amended 2026-10-29).
- **D-0205** (Thomas, Opus, 2026-10-22): invincibility on F4 and revived bosses are tools of the separate test launcher (`tools/Launch-DS3-DX12-Test.ps1`, `test-tools.flag`), never in the play version; saves are a sandbox restored by SHA-256; the test launcher never goes online.
- **D-0227** (Thomas, 2026-10-28): full production: no replayed path gate, thresholds, calibration, measure series, night endurance or memory leg before a merge; tranches merge once written and reviewed (D-0198); a measure only when a fix cannot be written without it or on his request; the gate and its readers stay as tools on request.
- **D-0228** (Thomas, 2026-10-28): the arrival is the whole game and its two DLC natively without a known fault, missing effect or defect the original lacks; on « je prends le PC » the pilot only reads and writes code, no build, no live run, until his « fini » (the one-list order replaced by D-0273; NVIDIA-last by D-0235).
- **D-0229** (Thomas, 2026-10-28): no live run before a merge; after every merge the launcher's module is rebuilt from `main` with its unit tests and a one-minute start check (two at most: launched, to the world, closed, offline, restored by hash; `scripts/Invoke-StartCheck.ps1`).
- **D-0230** (Thomas, 2026-10-28): every three days, at the first clean checkpoint and never in mid-workstream, the full control (replayed path gate with its memory leg, night endurance, control sizes) while Thomas sleeps (the day) or works (the night, about 19:30 to 07:00), on the ASUS alone with the OLED off, announced, stopped on « je prends le PC » or « je joue ».
- **D-0260** (Thomas, 2026-11-08): the disk guard: builds drop their `.obj` (`-KeepObjects`; a thread's `-Cache` keeps them, D-0295), NVIDIA DLLs are hard links, no build under 10 GB free; `scripts/Clear-DiskCache.ps1 -Apply` at each merge and under 100 GB free; run save backups beyond 100 move to `D:/DS3-save-backups-archive`.
- **D-0295** (Thomas, 2026-11-16): the build runs its commands in parallel by Ninja (`-Jobs`; `-Serial` the byte-identical reference); a thread may keep one object cache (`-Cache`), rebuilt incrementally and published into a per-version `-Out`; merges and the launcher build stay complete (I-0142).
- **D-0269** (Thomas, 2026-11-10): names and head comments may be written to the MCP copy of Ghidra (`ghidra-mcp-ds3`) only, one session at a time (token `ghidra-mcp-ds3.writer`), only where the repository states the role, as `<Role>_<address>`, each a row of `decompiled/GHIDRA-NAMES.tsv`; other Ghidras read-only; DS3 headless on 12.1.4, ER on 12.1.3.
- **D-0272** (Thomas, 2026-11-11): the executable and its protection are never patched or stripped, runs stay offline; encrypted functions are read through the decrypted-code overlay (`I0066Overlay936.java`) before any loaded-code run; a full decrypted project only when many are needed; no new code capture until one falls outside the overlay.

## Native architecture

- **D-0084, D-0096** (Thomas): the production target is a fully native D3D12 renderer; D3D11 is a reference only; no call translation, wrapper, bridge or fallback.
- **D-0115** (Thomas): native rendering may be developed incrementally inside the running game; the production target is unchanged.
- **D-0119, D-0120**: development-only observation of the original's GX and D3D11 calls is an oracle for checking native producers, never production architecture.
- **D-0150, D-0161** (Thomas): coverage by families: offline inventory, representative witnesses; native rules are per family or engine mechanism, never per asset (exceptions D-0275, D-0279).
- **D-0151** (Thomas): resolution, aspect ratio, display mode, HDR and scaling are inputs each frame supplies, never constants.
- **D-0153** (Thomas): every game-supported language stays in native UI coverage.
- **D-0154, D-0155** (Thomas): reuse existing native components; the engine feeds an integrated autonomous native backend; adaptation scaffolding is over.
- **D-0156** (Thomas): no crash-driven mini-workstreams; the direct connection design restarted from the Elden Ring method.
- **D-0157** (Thomas): DS3's engine architecture may change to follow ER's engine-to-D3D12 method, with explicit control of resource states, commands, submission, synchronisation and lifetime; DS3 data, layouts and ABIs evidenced independently.
- **D-0158**: explicit native feature-off material values replace the old MatDyn arena's implicit contents where authored activation is proved disabled.
- **D-0160**: native execution publishes the engine's own scene root class without its device members; the engine's device-absent branches apply.
- **D-0162**: native velocity follows the original's YEBIS configuration; upscaler-ready motion vectors come from a native resolve at render resolution.
- **D-0168**: the render worker and the worker prepare are the default (`inline-render.flag`, `inline-prepare.flag` for A/B).
- **D-0171**: the frame graph drives the barriers by default (`hand-barriers.flag` for A/B).
- **D-0173**: the effect GX module reads a native effect display through 252 redirected sites; its GPU objects are native records.
- **D-0175** (Thomas, 2026-10-13): YEBIS is not a dependency; our own D3D12 passes reproduce its passes with its values, each later replaceable by a modern technique (YEBIS entirely replaced: D-0262).
- **D-0190** (Thomas, 2026-10-18): native effect contracts produced by the DS3 engine (create, simulate, update, draw, destroy) consumed by the native backend replace the FFX statement replay, family by family once each matches the original, no GX fallback left.
- **D-0195** (Thomas, 2026-10-20): the native backend copies the architecture of Elden Ring's D3D12 engine, recovered by deep decompilation, and improves it later; data stay DS3's and verified on DS3; existing work transformed, not thrown away; ER's known defects (stutters) are not copied. Replaces D-0060.
- **D-0236** (Thomas, 2026-10-30): Elden Ring's RT is studied, not a model: its place in the engine architecture may be observed, its quality and workings are not copied; D-0195 stays for the rest of the D3D12 engine.
- **D-0196** (Thomas, 2026-10-21): the original D3D11 renderer is measured as the engine thread's reference, not copied; the timing project (LTF/TRTF) lives under [timing/](../timing/IMPORT.md) with its own numbers.
- **D-0197** (Thomas, 2026-10-21): a streamed resource not ready delays its model's appearance by a few frames, never a stand-in texture (no pop-in); the game clock untouched.
- **D-0203** (Opus, 2026-10-21): the video memory budget is read each pacing window and acted on with the backend's own spare memory only, never what the game keeps; targets at high residency priority; every pipeline and root signature of the game path goes through the one pipeline store.
- **D-0211** (Thomas, 2026-10-24): the native part of streamed content (textures, models, GPARAM, the map's pipelines) is prepared ahead and found in a cache in play, no pop-in (D-0197 the last resort); DS3's own loading is not forced to preload.
- **D-0212** (Thomas, 2026-10-24): one pacing rule on every screen: paced to the deadline, then the DXGI interval where the screen's refreshes fit within 2 %, Present(1) on other rates, Present(0) on a slower screen; V-sync off as `vsync-off.flag` / `-NoVsync` until the options menu.
- **D-0219** (Thomas, 2026-10-26): the display-grid cadence is abandoned (its code only on tag `archive/i97-t6-display-grid`); `main`'s pacer stands (the cadence's place: D-0240, D-0242).
- **D-0222** (Thomas, 2026-10-27): option A: the native renderer keeps DS3's LOD-reference global at its launch pair (writer 6E770 skipped) by default; `no-lod-launch-scale.flag` gives DS3's own behaviour; universal rule, no case by case.
- **D-0223** (the pilot, 2026-10-27): the first-sight work of model layouts is kept per recipe set, as ER keeps the shader choice beside the material; no preparation pass at the load; D-0211 stays the rule for streamed content.
- **D-0204** (Opus, 2026-10-22): a model layout leaves with its material, its engine prefix reused by the next build (the rest of D-0204 replaced by D-0227).
- **D-0241** (the pilot, 2026-10-30): no driver pipeline library: the precompile list (`native-pipelines.v3.psc`, format 3) is compiled at every start; the pipeline cache otherwise keeps ER's shape.
- **D-0165** (Thomas): temporal upscaling through Streamline in manual mode, an optional module behind a neutral interface; « everything off by default » superseded by D-0253 and D-0265; « DLLs supplied by the user » stands for the distributed mod (D-0274).
- **D-0187** (Thomas, Opus, 2026-10-18): two frames in flight by default in every case (no option for three); the module opens two frame slots and the waitable three-buffer chain unless `one-frame-slot.flag` or `no-waitable-present.flag`; Reflex Off, On, On + Boost.
- **D-0252** (Thomas, 2026-11-06): the traced GI runs on every traced frame (Ray Reconstruction when on, NVIDIA's NRD otherwise), SHaRC the cache; NVIDIA's SDKs stay outside the repository and GitHub copy, read from `Tools/DS3-Decompilation/third-party/`; RTXDI hash-pinned, ReSTIR GI opt-in then (see D-0265).
- **D-0255** (Thomas, 2026-11-07): no GPU-driven raster workstream (GPU culling and `ExecuteIndirect` for the world's geometry, mesh shaders; mesh shaders or a Nanite-like format with the HD assets after the finish line), but workstream 15's occlusion cull (D-0304); the CPU lever kept: a persistent, change-driven collect and rays' walk, its pilot to be placed by the coordinator and Thomas.
- **D-0304** (Thomas, 2026-11-21): workstream 15, the end's re-optimisation, holds a modern occlusion cull (GPU, Hi-Z from the previous frame's depth, perhaps per cluster), simplified models for the rays, fewer rays with a better denoiser, and the survey of the native's extra work (ROADMAP 15); workstreams 12 and 13 do not wait for it, D-0255's CPU lever comes forward if the frame rate falls too low to test; the methods are the pilot's, provided no cull takes out of the RT/PT scene what a ray can see (Thomas), each accepted on a picture comparison (the coordinator's relay).
- **D-0275** (Thomas, 2026-11-12): the page `05_901_Logo_BNE` is drawn on black always, no option row; public version DS3's own logo made for black from the game's bytes; on Thomas's PC only `er-bne-logo.dds` from his own ER via a local script, nothing of ER into Git, cloud or installer.
- **D-0278** (Thomas, 2026-11-14): DS3's own anti-aliasing filter (`GXAntiAliasingFilter`) is removed; a frame that neither DLSS nor the TAA resolves has no anti-aliasing, on SDR as on HDR10.
- **D-0290** (Thomas, 2026-11-15): before a stand-in is removed or a refusal becomes a skip that draws nothing, the original's answer for the same input is read (the arguments' own path included), the caller that asks again named when a retry is relied on, and every case the journals show met is witnessed; a default the original binds itself is its picture, not a stand-in (WORKFLOW « Method rules »).

## Rendering defaults and the RT/PT line

- **D-0178** (Thomas, 2026-10-14): the remastered settings replace the original ones with their own levels, the original no longer kept as an option; the colorimetry keeps the original option beside a new one (original grading remade in the new post chain, D-0273).
- **D-0182** (Thomas, 2026-10-14 to 16): the remaster is « Dark Souls III: Rekindled »; title screen after Thomas's mock-up (`.project/design/title-screen-mockup.png`); start logos on black; HDR10 with two settings, maximum nits and paper white; a double-click launcher for Thomas.
- **D-0235** (Thomas, 2026-10-30): full RT/PT ([RT-PT-PLAN](../.project/RT-PT-PLAN.md)): shadows, lighting and reflections traced by levels, no raster lighting left; the G-buffer, UI, particles and post-processing stay raster; replaces « NVIDIA last ». Its « DLSS instead of anti-aliasing » replaced by D-0265 (the TAA the default without DLSS); its settings (the plan's §6) a proposal, the rows delivered being R12's (D-0273).
- **D-0237** (Thomas, 2026-10-30): the raster lighting is removed, not hidden, once its traced replacement works and Thomas accepted it; never before; the comparison reference is the D3D11 original; a traced picture may differ from it; light sources are adjusted, emissive textures evaluated as emitters; with rays on, no sun cascades are made.
- **D-0238** (Thomas, 2026-10-30): RTX Mega Geometry is not planned by default; R2 in standard DXR 1.2 (one BLAS a mesh, refits, one TLAS a frame); Mega Geometry returns only on a measure of build cost, memory or LOD hitches; geometry is not changed.
- **D-0247** (Thomas, 2026-11-03): the local shadow atlas is not made on frames whose local shadows are traced; Ray Reconstruction is a player option, on by default in ray tracing (`rr-mode.txt`, `no-rr.flag`); its fill-light clause is superseded by D-0262.
- **D-0249** (Thomas, 2026-11-06): in RT/PT the original's picture is not a pixel target: a natural, true lighting is accepted everywhere; a bug (fault, wrong or missing input, light where none belongs, broken material) is still an anomaly; no full raster mode for players; raster the RT/PT replaces is not corrected.
- **D-0250** (Thomas, 2026-11-06): the open-gaps pilot fixes only gaps that stay visible with the traced lighting (G-buffer, effects, UI, post-processing); gaps whose only effect is raster lighting, shadows or reflections are dropped (its listed order of gaps is consumed).
- **D-0251** (Thomas, 2026-11-06): wherever the rays give a traced equivalent of a raster path, that path is removed from the code, not kept behind a flag, without a separate acceptance each time; the pilot tells Thomas what it removes; no raster mode for unserved frames.
- **D-0253** (Thomas, 2026-11-07): the rays' scene and all on with it run by default, play sessions included; `no-rt-scene.flag` is a test tool; its « device without DXR runs without rays » clause is superseded by D-0262 (message and clean close).
- **D-0256** (Thomas, 2026-11-07): the PT level traces two path vertices by default, set in `rt-bounces.txt` beside the module (one digit, 1 to 4), like `rt-level.txt`, `rr-mode.txt`, `dlss-mode.txt`.
- **D-0262** (Thomas, 2026-11-09): no lighting cheat is simulated or created any more: the fill lights (sets' lights 1 to 3) and the baked ambient are removed from traced frames, code included; light comes from the real sources whose settings alone are adjusted; no fallback for frames the rays do not serve (clear message, clean close); YEBIS entirely replaced; local lights' colours x2 (DS3's own strength since D-0293). Provisional exceptions for cutscenes: D-0279, D-0282; the untraced menu views keep their raster lighting: D-0301.
- **D-0263** (the coordinator on Thomas's universal rule, 2026-11-09): the automatic exposure has one target for the whole game (the original's mean displayed level, 0.20; 0.12, its darkest tenth, since D-0294) and one adaptation law (Unreal's), never one target by area.
- **D-0264** (Thomas, 2026-11-09): the automatic exposure is on by default at half strength (test tools `no-auto-exposure.flag`, `rt-auto-exposure-full.flag`); the local lights' x2 replaced by DS3's own strength (D-0293), the physical terms test tools (`rt-local-physical-10/-50/-200.flag`); lower bound of 1 added by D-0280, the target 0.12 by D-0294.
- **D-0265** (Thomas, 2026-11-10): defaults: Agility SDK on (`no-agility.flag`); a TAA the default without DLSS (DLAA until it exists), DLSS the optimum; RR and frame generation offered only while a DLSS mode runs; PT needs a DLSS mode; ReSTIR GI on with the PT (`no-rt-restir.flag`); Reflex off by default, mandatory with FG; 60 frames a second by default.
- **D-0276** (Thomas, 2026-11-12/13): workstream 11's answers: HDR on his OLED; no chromatic-aberration row for the original colorimetry (a row for the Rekindled one, ER's slight aberration kept); light shafts rewritten once inside the new bloom, no bridge; half-strength auto exposure kept; AgX and ACES may be tone-map candidates; ER's data read locally only.
- **D-0279** (Thomas, 2026-11-14): a cutscene's own light sets' lights 1 to 3 stay on a traced frame, unshadowed, a provisional exception to D-0262 until we set the light sources (workstream 12); with it, where a cutscene's ShadowParam names no light set, the traced sun takes light 0 of the cut's own set when it is on; local copies of Elden Ring's data allowed for trials, nothing distributed; HDR rows in the Graphics tab as sliders, the options film pinned by name, size and SHA-256.
- **D-0280** (Thomas, 2026-11-14): the automatic exposure stays the default at half strength toward the game's one target with a lower bound of 1: it brightens darker views and leaves brighter ones at the zone's own exposure. The target 0.12 since D-0294.
- **D-0293** (Thomas, 2026-11-16): the traced local lights at DS3's own strength for good (the A/B, I-0141 tranche 10); `rt-local-x2.flag` and `rt-local-x4.flag` test tools; replaces D-0264's x2 clause.
- **D-0294** (Thomas, 2026-11-16): the automatic exposure's one target 0.12 (was 0.20), D-0280's lower bound and the half strength kept: it brightens only views darker than the original's darkest tenth.
- **D-0296** (Thomas, 2026-11-16): the Rekindled look is the vivid variant (D-0285's curve and lift, then a contrast to ER's grey median, a gentle vibrance sparing skin, a cool black); the original stays the default.
- **D-0297** (Thomas, 2026-11-17): the Rekindled look takes DS3's own vignette by area (the area's Vignette Power, the On/Off row kept); Elden Ring's vignette removed.
- **D-0298** (Thomas, 2026-11-17): at each checkpoint a pilot may propose one idea outside the plan to surprise him, made only with his agreement.
- **D-0299** (Thomas, 2026-11-17): original and native are compared at the same moment of the game: samples on the game's own clock in both modes, the clock's value named, unequal pairs not compared.
- **D-0285** (Thomas, 2026-11-15): the Rekindled look is the one after Elden Ring alone, beside the original, with ER's LUTs' midtone and shadow lift as a curve of ours (no AgX, no ACES offered); the original stays the default, the Rekindled look an option; the row is I-0138 tranche 7's.
- **D-0282** (Thomas, 2026-11-15): during a cutscene act that overrides a map's light sets, draws using those overridden map keys keep the cutscene sets' lights 1 to 3 on a traced frame, unshadowed, provisional like D-0279; a map key not overridden keeps D-0262's removal.
- **D-0233** (Thomas, 2026-10-29): after the finish line the remaster replaces DS3's blood by Elden Ring's and takes ER's frostbite, bleed, fire and Black Flame effects; poison and lightning wait for his decision; holy keeps DS3's look; ER's files read on his PC only.
- **D-0301** (Thomas, 2026-11-21): the menu views (portraits, equipment, creation) keep their raster lighting and take the original's environment region pass (their own region, GILM0010); nothing changes for a traced view.
- **D-0303** (Thomas, 2026-11-21): the Player menu views (creation, Face Detail, Status body) get their self-shadow from a traced ray in the direction of the original's ShadowParam Angle for the view's id; no shadow map brought back; Face and Prof views without shadow; the rest of their lighting raster (D-0301).
- **D-0305** (Thomas, 2026-11-22): the menu views take the native's own anti-aliasing, the TAA by default, DLSS or DLAA when the option selects them, none with the option off, as the main view; DS3's filter stays removed for them too (D-0278).
- **D-0306** (the E pilot under Thomas's delegation, 2026-11-22): the menu views' TAA reconstructs with exp(-8 d^2) (the main view's 2.29 unchanged) and restarts a history its collect resets from an unjittered frame taken as its own samples; no menu-own sharpening, his slider applies.
- **D-0307** (Thomas, 2026-11-22): workstream 12: the rays take the sky at its authored strength (the drawn dome unchanged); DS3's placed lights without a visible source are named, then judged for removal by family after the sky, none before; visible windows become emitters of the outside light; the fog is a lit medium (the original's opacity cap a clamp, its colour set against the area's daylight), the screen-space light shafts retired where it draws real ones; all on sheets.

## Plan, scope and sequencing

- **D-0167, D-0169** (Thomas): 60 frames/s first; the unlocked frame rate comes later as a remaster option; the game clock is not touched; universal rules per mechanism.
- **D-0170** (Thomas): borderless window and other aspect ratios later; before the remaster, a static whole-menu audit and resolution/full-screen witnesses under one universal rule (1080p, 1440p, 4K, one odd resolution).
- **D-0172** (Thomas): shaders and pipelines precompiled into a checked disk cache with a loading screen and a « reload shaders » entry, after the families' pipelines are stable (moved to the remaster by D-0179; screen shown only when a compilation really happens, D-0273).
- **D-0240** (Thomas, 2026-10-30): Reflex stays a player option; LTF and TRTF are experimental, studied not copied; the unlock runs the logic faster by a variable step (method B) after an impact study (I-0112); the cadence takes the modern D3D12 loop, not ER's; the logic is correct at any delta time, 120 frames/s one tested point. Its order replaced by D-0242.
- **D-0242** (Thomas, 2026-10-30): the modern cadence and Reflex are done inside the unlocked frame rate's workstream (I-0114), `main`'s 1/60 s pacer kept meanwhile; the display clock (`display-clock.flag`) stays behind its flag until the unlock offers a rate above the display's.
- **D-0243** (Thomas, 2026-10-31): the follow camera is method B too: its laws corrected one by one with k; the fixed step (`camera-fixed-step.flag`) stays only as a measuring reference, never a default.
- **D-0244** (Thomas, 2026-11-01): above 60 a held move's sooner start is kept (extra distance +1-3 % accepted); every speed and every discrete move (a roll, a jump, a backstep) covers 60's distance; one logic at every rate (k = dt x 60), checked under 60 too; where no law reproduces 60 the original's is kept and the gap reported (gap 27's fix an exception, D-0273).
- **D-0267** (Thomas, 2026-11-10): after R12 an installer carries only the native renderer and our settings page, no texture, no texture processing; it requires the player's own game, checks the official Steam executable's hash and Valve's signature on `steam_api64.dll`, no DRM (no NVIDIA DLL: D-0274).
- **D-0300** (Thomas, 2026-11-20): workstream 10 keeps only the compatibility probes (WARP, GPU-based validation, 4 and 6 GB video memory); the production build, the installer (D-0267's « after R12 » replaced), the start without NVIDIA's DLLs and the Agility SDK's redistribution go to workstream 18; workstream 12 not brought forward.
- **D-0284** (Thomas, 2026-11-15): every test of the game tour (I-0139) is a reference ([REFERENCE](../investigations/I-0139-game-tour/REFERENCE.md)); the whole tour is run again from it at the project's end, on one screen and one build.

## Interface, options and release

- **D-0266** (Thomas, 2026-11-10): the remaster's settings in DS3's art direction, the same at the title and in play; entries include the shader precompile screen, a sharpening bar (off by default), vignette, V-sync and Borderless with the frame rate; its page of our own dropped by D-0271; amended by D-0270, D-0276.
- **D-0270** (Thomas, 2026-11-10): Graphics holds Screen Mode (Full screen > Borderless > Windowed), Resolution, V-sync, the frame rate, then Advanced; the game's preset stays at the head of Advanced; Auto-detect removed and kept OFF; the native-drawn page dropped by D-0271.
- **D-0271** (Thomas, 2026-11-10/11): our rows go in the game's own Graphics tab and Advanced Settings page through its row functions, no film edited (the options film pinned, D-0279); unavailable rows greyed and locked (D-0265) except the DLSS mode row and the light bounces' row (shown only when relevant); one Anti-Aliasing row: Off, TAA, NVIDIA DLAA, NVIDIA DLSS; sharpening a slider.

## To confirm

In force by the LOG's text, but perhaps consumed or narrowed by later work; the next thread that meets one confirms it from the
LOG and the records, then keeps its line above or moves it to the table.

- **D-0208** (Opus, 2026-10-23): the gate's display count is `display_repeats` from the swap chain's statistics, superseded in source by D-0209's PresentMon count; inconclusive below three quarters of the display windows or off 60 Hz.
- **D-0209** (Thomas, 2026-10-24): the gate is independent of the screen: one base per desktop mode, never mixed, limits relative to the frame period P (1.5 P, 2.04 P), display count from Intel's PresentMon; base required is the ASUS 1920x1080@60 alone (D-0218 replaced by D-0227).
- **D-0216** (the pilot's raise under D-0215, 2026-10-25): the gate's thresholds after the real mouse turn: ASUS display_late 22; OLED frames over 2.04 P 7, over 1.5 P 30, display_late 16; a threshold rises only when coherent (changed path or measure, never a regression), numbers in its own entry, named by `-AllowRaise`, reported to Thomas.
- **D-0213** (Thomas, 2026-10-25): replayed paths beyond Firelink are Ariandel (Epsilon), Archdragon Peak (Eta, own save copy), the Undead Settlement (Beta) and Lothric Castle (Beta, own copy), study bases outside the blocking gate; D-0211's preparation may add up to 1 s to a zone load.
- **D-0164**: phase C closed with R9 and R6; R7 waits for a Tier 1/2 requirement; R8a's gain moved to workstream K.
- **D-0185** (Opus, 2026-10-17): the classic FFX draws run the original's draw side captured at the GX context wrappers into draw records the D3D12 stage encodes; on by default (`no-ffx-draw.flag`); to be replaced family by family by D-0190.
- **D-0186** (Opus, 2026-10-17): the GXVfx GPU particles (E07) run their own simulation and draw against native GX contexts, on by default (`no-ffx-gpu.flag`); same replacement by D-0190.
- **D-0210** (Opus, 2026-10-24): the present pacer paces every frame by hand to its deadline, then presents; its screen-dependent interval rule is replaced by D-0212; the launcher's screen option (`display-screen.txt`) stays.
- **D-0258** (Thomas, 2026-11-08): the settings menu and the Rekindled title screen live in R12 (R10 keeps frame generation and the raster lighting's removal); order R9, R10, R11, R12.
- **D-0232** (Thomas, 2026-10-28): the key order of the effects with the forward rows stays on FINISH-LINE line 7, after the missing effects and the spot lights; model particles drawn among the world's forward rows 0x29 until then.
- **D-0221** (Thomas, 2026-10-26): the map's spot lights and local-light shadows are their own workstream (SHADOWS-WAVE1-READY §1.5 and §3), not taken by I-0100.
- **D-0220** (Thomas, 2026-10-25/26): the pop-in is removed in the whole game in two steps (DS3's draw decision, then a modern layer: dithered fades, margin, distant characters every frame, preparation ahead), never a stand-in.
- **D-0206** (Thomas, Opus, 2026-10-23): the gate's memory leg judges eight visits, the slope the median of the pairwise slopes (Theil-Sen) of visits 2 to 8, threshold 40 MB a visit; replaced by D-0227 in the LOG's text; the full control (D-0230) runs the gate with its memory leg again, so this method may still be the one used: to confirm.
- **D-0207** (Thomas, Opus, 2026-10-23): the gate's thresholds are lowered after a merge that improves the frame times, by its calibration formula; a threshold never rises without Thomas's decision; replaced by D-0227 in the LOG's text (no recalibration at each merge); whether its rules govern a recalibration in the full control (D-0230 is silent) is to confirm.

## Consumed or superseded since D-0177

| ID | Title (short) | Why (consumed / superseded by D-xxxx, in part or whole) |
|---|---|---|
| D-0060 | ER an architectural witness only | Superseded by D-0195 |
| D-0176 | Single review at the merge | Superseded by D-0198 |
| D-0177 | Repository cleanup of 2026-10-13 | Consumed (one-off, done; never restore removed directories into `main` automatically stays its note) |
| D-0179 | Shader and pipeline cache to wave 2 | Consumed: wave terminology dropped (D-0228); the rule lives in D-0172 |
| D-0180 | Sun shadow mask and SSAO join wave 1 | Consumed: raster lighting removed (D-0237, D-0251, D-0252) |
| D-0183 | Sun shadow on by default | Consumed: traced sun shadow default, cascades off with rays (D-0237) |
| D-0184 | Sun casters follow the original's selection | Consumed: the cascade raster is removed with the rays (D-0237); to confirm |
| D-0188 | Wave 1 closes early, final review | Consumed (one-off) |
| D-0189 | Water planar reflection on by default | Superseded by D-0251 (mirror view removed) |
| D-0191 | Wave 1 closes after two cleanups | Consumed (one-off) |
| D-0192 | Wave-1 closure tests | Consumed (one-off) |
| D-0193 | Texture hitches and D2 paint before closure | Consumed (one-off) |
| D-0194 | Wave 1 closed | Consumed (milestone reached) |
| D-0200 | Order of the next workstreams | Superseded by D-0228, then D-0273 |
| D-0202 | Replayed path gate at every merge | Superseded in part by D-0227 (gate requirement removed; gate stays a tool, see D-0230) |
| D-0218 | Gate on one screen, memory leg where it matters | Superseded by D-0227 (its restatement of D-0198's review before every merge stands as D-0198's) |
| D-0226 | Lighter tests per tranche | Superseded by D-0227 |
| D-0231 | Haiku 5.5 for mechanical work | Superseded by D-0245 (Sonnet 5.5) |
| D-0254 | Fill lights removed on traced GI frames | Superseded by D-0257, then D-0262 |
| D-0257 | Fill lights kept on GI frames for now | Superseded by D-0262 |
| D-0259 | Fill lights' traced shadow on by default | Lapses under D-0262 |
