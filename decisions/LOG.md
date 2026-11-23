# Decision log

Consequential decisions only. Summarised on 2026-10-13 (D-0177): each decision still
in force keeps a short statement of its rule; consumed or superseded decisions keep
their title. The full texts are in Git tags `archive/pre-cleanup-2026-10-13` (D-0001
to D-0176) and `archive/pre-cleanup-2026-10-02` (D-0029 to D-0083). D-0003 to D-0028
(timing, LTF/TRTF) are in [timing/decisions/DECISIONS.md](../timing/decisions/DECISIONS.md) (D-0196), with their own numbers.

**Read [IN-FORCE](IN-FORCE.md) first** (D-0291): one line per decision in force, grouped by theme, from D-0001 to the last; this
log keeps every full text; its own « In force » section below is the summary begun on 2026-10-13, kept as it was and no
longer maintained (IN-FORCE is current).

## In force

### Governance and method

- **D-0001** (2026-09-19): the project runs on persistent state, a capability roadmap,
  evidence-backed claims, bounded investigations and consequential decisions; work alone
  by default.
- **D-0002** (2026-09-19): no DS3 claim becomes canonical without provenance tied to the
  build, input identity, experiment and artifact location or hash.
- **D-0040** (2026-09-25): the architectural frame challenge of [WORKFLOW](../.project/WORKFLOW.md#architectural-frame-challenge-anti-tunnel)
  runs at consequential gates.
- **D-0195** (Thomas, 2026-10-20, first rule of wave 2): the native backend copies the architecture of Elden Ring's D3D12
  engine (FromSoftware already modernised this engine with DX12), recovered by deep decompilation, and improves it later;
  the data stay DS3's (effects, graphics parameters, models, shaders) and are verified on DS3. The existing work is
  transformed, not thrown away. Replaces D-0060. ER's known defects (stutters) are not copied (Thomas, 2026-10-21; WORKFLOW).
- **D-0197** (Thomas, 2026-10-21): a streamed resource not ready delays its model's appearance by a few frames, never a
  stand-in texture (no pop-in); the game clock untouched.
- **D-0198** (Thomas, 2026-10-21): Fable only on Thomas's explicit agreement; Opus reviews at each step and in full at the
  merge, never reduced by the weekly limit (replaces D-0176 and D-0181's quota rule).
- **D-0217** (Thomas, 2026-10-25): STATE's resume rows rewritten at each checkpoint with a « Pilot branch » row; WORKFLOW in the
  present tense; procedures as project skills, reviewers as subagents. Its stop guard removed by D-0234.
- **D-0234** (Thomas, 2026-10-29): no stop hook; a thread waits by ending its turn.
- **D-0218** (replaced by D-0227) (Thomas, 2026-10-26): the replayed path gate on one screen, the ASUS (the OLED added when justified, clarified at
  its merge; D-0209's two screens ended); its memory leg only when the branch touches the memory, the loadings or the streaming; the final review by
  a fresh Opus at xhigh stays required.
- **D-0227** (Thomas, 2026-10-28): full production: no measurement gate, thresholds, calibration, series or night endurance;
  tranches and workstreams chained in the ROADMAP's order and merged once written and reviewed; after every merge the module
  builds (unit tests) and the game starts to the world with it. Replaces D-0202's gate requirement, D-0204's endurance, D-0206,
  D-0207, D-0218 and D-0226; the gate and its readers stay as tools, used on request.
- **D-0228** (Thomas, 2026-10-28): one ordered list to the finish line, without waves or versions
  ([FINISH-LINE](../.project/FINISH-LINE.md), ROADMAP) (its « NVIDIA's technologies last » replaced by D-0235); replaces D-0224's order; « je prends le PC »
  stops builds and live runs until his « fini ».
- **D-0229** (Thomas, 2026-10-28): no live run before a merge; after a merge the launcher's module is rebuilt from `main` with
  its unit tests and a one-minute start check (two at most: launched, to the world, closed; `scripts/Invoke-StartCheck.ps1`)
  replaces D-0227's (updates D-0227).
- **D-0230** (Thomas, 2026-10-28): every three days, at the first clean checkpoint, the full control as before D-0227 (the
  replayed path gate with its memory leg, the night endurance, the resolutions), while Thomas sleeps or works, on the ASUS alone.
- **D-0235** (Thomas, 2026-10-30): full RT/PT ([RT-PT-PLAN](../.project/RT-PT-PLAN.md)): FINISH-LINE line 1 and I-0105 first, then R0 to R11, then
  T1 the textures; the raster lighting hidden behind the test launcher, removed category by category once Thomas accepts the traced
  equivalent; replaces « NVIDIA last » and « the raster stays the reference »; completes D-0178 (the settings of the plan's section 6).
- **D-0237** (Thomas, 2026-10-30 04:55): the raster lighting is removed, not hidden (option A: the D3D11 original is the
  reference); the traced shadows (R5) come right after R2.
- **D-0238** (Thomas, 2026-10-30 05:25): RTX Mega Geometry no longer planned by default; R2 in standard DXR (one BLAS a mesh, refits, a TLAS a frame),
  Mega Geometry back only if the measured build cost, memory or LOD hitches call for it; replaces the rule of 2026-10-14.
- **D-0239** (Thomas, 2026-10-30 06:37, confirmed 06:40): a study thread merges its documentation-only commits into `main` itself (no code, no
  rebuild), when no pilot is merging or rebuilding; replaces WORKFLOW's « never merges into main » for the study thread.
- **D-0240** (Thomas, 2026-10-30 08:21-08:28): Reflex stays a player option (never the frame's only clock); LTF and TRTF are experimental, studied
  for the unlocked frame rate, not copied as they are; the unlock by a variable step after an impact study (I-0112); the cadence by the
  modern D3D12 loop, not Elden Ring's; the order R1, the cadence (Reflex plugged into it), the unlock, then R2.
- **D-0242** (Thomas, 2026-10-30 23:39): the cadence by the modern loop and Reflex are done inside the unlocked frame rate's
  workstream (I-0114), measured not to take the clock while the logic stays at 60; `main`'s pacer kept meanwhile; amends D-0240's order.
- **D-0219** (Thomas, 2026-10-26): the display-grid cadence (I-0097 tranche 6) abandoned without merging its native code (no gain
  measured); the cadence returns after the pop-in workstream as « Cadence, méthode DX12 moderne », a workstream studied ahead.
- **D-0060** (2026-09-29): replaced by D-0195.
- **D-0114, D-0159, D-0163** (Thomas): Opus 5.5 at high effort is the pilot and sole
  technical owner; Haiku 5.5 does bounded checkable work at an effort Opus chooses; Fable
  is replaced by Opus 5.5 high for its tasks, the pre-merge review included; any Fable use
  needs Thomas's prior agreement (D-0152).
- **D-0118** (Thomas): new sessions and investigations are pre-authorised; Remote Control
  is turned on at session start.
- **D-0129** (Thomas): a workstream that meets its exit criteria is merged into `main` and
  the next one starts without waiting; mid-workstream merges still go to Thomas.
- **D-0176** (Thomas, 2026-10-13): replaced by D-0198 (a review at every step and the full one at the merge).
- **D-0177** (Thomas, 2026-10-13): repository cleanup; see below.
- **D-0181** (Thomas, 2026-10-16): its visual rule stays: a visual workstream closes only after a side-by-side comparison
  with the original at every witness zone, shown to Thomas; its usage-quota rule is replaced by D-0198.
- **D-0199** (Thomas, 2026-10-21): every process a thread starts in the background runs hidden, no visible cmd or console
  window on the PC (headless Ghidra included); [WORKFLOW](../.project/WORKFLOW.md#background-processes-on-thomass-pc-thomas-2026-10-21-d-0199).
- **D-0200** (Thomas, 2026-10-21): the order of the next workstreams: « Saccades, tranche 2 », the minidumps with the two
  I-0080 holes, the replayed path with its threshold, the DX12 pitfalls audit, the night endurance test (ROADMAP); the
  engine thread's third tranche, not placed by Thomas, follows them by default.
- **D-0201** (Opus, 2026-10-21, under D-0200): every native fault leaves a minidump and a report (controller-written), DRED
  on by default, the D3D12 debug layer only on request (`debug-layer.flag`) and tolerated when absent (I-0082).
- **D-0202** (replaced by D-0227 for the gate) (Opus, 2026-10-21, under D-0200): the replayed path gate blocks every merge into `main` that changes native code:
  two measure runs and one validation run of the recorded Firelink path, thresholds from `main`'s spread that only go down
  (a rise is Thomas's), its record required by `Test-Project` on `main` (I-0083, [REPLAY-GATE.json](../.project/REPLAY-GATE.json)).
- **D-0203** (Opus, 2026-10-21, under D-0200): the video memory budget is read each pacing window and acted on with the
  backend's own spare memory only, never what the game keeps (D-0197), the frame's targets at high residency priority; every
  pipeline and root signature of the game path goes through the pipeline store (I-0084).
- **D-0204** (its night endurance and memory leg replaced by D-0227) (Thomas, 2026-10-22, « Lance les 4 », relayed by the coordinator): the replayed path gate adds a memory leg (since
  D-0218 only with `-Memory`, when the branch touches the memory, the loadings or the streaming)
  (one zone loaded four times, the title's private memory per visit against a filed threshold), and a short endurance runs on
  the latest `main` every night Thomas does not play; a model layout leaves with its material (I-0085).
- **D-0206** (replaced by D-0227) (Thomas, 2026-10-23): the gate's memory leg loads its zone eight times and judges the Theil-Sen slope of the
  titles after visits 2 to 8 (threshold 40 MB a visit).
- **D-0207** (replaced by D-0227) (Thomas, 2026-10-23, relayed by the coordinator): after each merge that improves the frame times, the gate is
  recalibrated on `main` (`-Calibrate`): its frame thresholds go down toward the level reached, with the formula's noise margin.
- **D-0208** (Opus, 2026-10-23, I-0097, the order relayed from Thomas): the gate's « frames held two refreshes » count (biased
  low, I-0095) is replaced by the display's repeated refreshes (`display_repeats`, DXGI PresentRefreshCount minus frames shown,
  per window), threshold 24 from `i97-cal2`; the gate is inconclusive below three quarters of the display windows or off 60 Hz.
- **D-0209** (Thomas, 2026-10-24, I-0098): the replayed path gate is independent of the screen: one base per screen (« WxH@Hz »,
  never mixed, a pass on every required base: the 1920x1080 60 Hz and the 2560x1440 279.96 Hz, the ASUS alone since D-0218), limits relative to the run's
  frame period (1.5 P and 2.04 P), the period guarded, and the display count from PresentMon (Intel's, unelevated); the 60 Hz base's display limit starts from the native's current state (Thomas's « Oui d'accord »).
- **D-0210** (Opus, 2026-10-24, I-0097 tranche 3, Thomas's order): the native present pacer paces every frame by hand to its
  deadline, then presents with the DXGI sync interval where the screen is a whole multiple of 60 Hz (a labelled deviation from
  the original's Present(1) at once at 60 Hz): a composed 60 Hz screen's dropped frames 110-125 to 11-19 a run (K-0480).
- **D-0211** (Thomas, 2026-10-24 18:56, I-0097 tranche 4): the modern method for streamed content: the native part (textures,
  models, GPARAM, the map's pipelines) is prepared ahead and found in a cache in play, no pop-in (D-0197 stays, as a last
  resort); DS3's own loading is not forced to preload; a longer draw distance later (ROADMAP).
- **D-0212** (Thomas, 2026-10-24 15:55-16:05, I-0097 tranche 4): one pacing rule on every screen, as modern games do: paced to
  the deadline, then the screen's refreshes as the DXGI interval where they fit the cadence, Present(1) on other rates (a VRR
  screen shows each frame as it arrives), Present(0) on a slower screen; the game's V-sync off as a launcher option until the
  options menu (Present(0), tearing allowed: the screen's VRR and the driver's V-sync decide).
- **D-0213** (Thomas, 2026-10-25 01:16, I-0097 tranche 5): the replayed paths beyond Firelink are Ariandel (Epsilon), Archdragon
  Peak (Eta, travelled there in its own save copy), the Undead Settlement (Beta) and Lothric Castle (Beta, its own copy); study
  bases outside the blocking gate at first; D-0211's preparation may add up to 1 s to a zone load.
- **D-0214** (Thomas, 2026-10-25 03:02): the repository is the only reference for the working rules (WORKFLOW « Standing
  rules », the coordinator's sheet points to it); a study thread always runs one workstream ahead, read-only.
- **D-0215** (Thomas, 2026-10-25 07:55-07:58, I-0097 tranche 5): the Firelink gate's turn made a real mouse turn after the
  Nsight study, the bases recalibrated; a coherent raise decided by the pilot, its numbers in its own entry; a fresh thread at
  every clean checkpoint.
- **D-0216** (Thomas, 2026-10-25, his D-0215 delegation; the pilot's raise): the gate's thresholds after its Firelink path gained
  the real mouse turn (`i97-t5-turncal`): ASUS display_late 21 to 22; OLED frames over 2.04 P 5 to 7, over 1.5 P 27 to 30,
  display_late 15 to 16 (the numbers below).
- **D-0196** (Thomas, 2026-10-21): the original D3D11 renderer is measured as the engine thread's reference, not copied; the
  timing project (LTF/TRTF) is back under [timing/](../timing/IMPORT.md) with its own numbers (I-0005 to I-0014, K-0010 to
  K-0064, D-0003 to D-0028), its raw captures left in the Desktop folder.

### Live tests and data

- **D-0116, D-0117** (Thomas): Opus launches and drives DS3 itself for every
  investigation: protocol written and announced in chat first, game offline, protected
  files backed up and restored by hash, nothing installed in the game folder, a crash
  diagnosed before another run. Online play and game-file changes go to Thomas.
- **D-0142** (Thomas): no raw artifact retention by default; keep only what reproduces,
  audits or falsifies a claim. Retention scope: [EVIDENCE-RETENTION.json](../.project/EVIDENCE-RETENTION.json).
- **D-0260** (Thomas, 2026-11-08): the disk guard: builds drop their `.obj`, `scripts/Clear-DiskCache.ps1` rotates the
  evidence store's builds and raw frames at each merge, run save backups beyond 100 move to D:, no build under 10 GB free.
- **D-0166** (Thomas, 2026-10-12): every level of every graphics setting is tested live;
  the pilot may change Thomas's options, restored and checked by fingerprint after each run.
- **D-0174** (Thomas, 2026-10-13): zone witnesses through the Shebang save, one character
  other than Alpha per zone, saved there and kept as a hashed test save; Alpha never modified.

### Architecture of the native backend

- **D-0084, D-0096** (Thomas): the production target is a fully native D3D12 renderer;
  D3D11 is a reference only; no call translation, wrapper, bridge or fallback.
- **D-0115** (Thomas): native rendering may be developed incrementally inside the running
  game; the production target is unchanged.
- **D-0119, D-0120**: development-only observation of the original's GX and D3D11 calls
  (programme identity, constant bytes, draws) is an oracle for checking native producers,
  never production architecture.
- **D-0150, D-0161** (Thomas): coverage by families: offline inventory, representative
  witnesses; native rules are written per family or engine mechanism, never per asset.
- **D-0151** (Thomas): resolution, aspect ratio, display mode, HDR and scaling are inputs
  each frame supplies, never constants.
- **D-0153** (Thomas): every game-supported language stays in native UI coverage.
- **D-0154, D-0155** (Thomas): reuse existing native components; the engine feeds an
  integrated autonomous native backend; adaptation scaffolding is over.
- **D-0156** (Thomas): no crash-driven mini-workstreams; the direct connection design
  restarted from the Elden Ring method study716.
- **D-0157** (Thomas): DS3's engine architecture may change to follow ER's
  engine-to-D3D12 method, with explicit engine control of resource states, commands,
  submission, synchronisation and lifetime; DS3 data, layouts and ABIs evidenced
  independently.
- **D-0158**: explicit native feature-off material values replace the old MatDyn arena's
  implicit contents where authored activation is proved disabled.
- **D-0160**: native execution publishes the engine's own scene root class without its
  device members; the engine's device-absent branches apply.
- **D-0162**: native velocity follows the original's YEBIS configuration; upscaler-ready
  motion vectors come from a native resolve at render resolution.
- **D-0164**: phase C closed with R9 and R6; R7 waits for a Tier 1/2 requirement; R8a's
  gain moved to workstream K.
- **D-0168**: the render worker and the worker prepare are the default
  (`inline-render.flag`, `inline-prepare.flag` for A/B).
- **D-0171**: the frame graph drives the barriers by default (`hand-barriers.flag` for A/B).
- **D-0173**: the effect GX module reads a native effect display through 252 redirected
  sites; its GPU objects are native records.
- **D-0175** (Thomas, 2026-10-13): YEBIS is not a dependency; our own D3D12 passes
  reproduce its passes with its values, checked against the library hosted natively in
  record-only mode, each later replaceable by a modern technique among the remaster's
  options.

### Scope and sequencing

- **D-0165** (Thomas): temporal upscaling through Streamline in manual mode, an optional
  module behind a neutral interface, everything off by default, DLLs supplied by the user. (« Everything off by default » superseded: the rays and the TAA are the defaults, D-0253, D-0265. « DLLs supplied by the user » stands for the distributed mod: no NVIDIA DLL is distributed, D-0274; the development build links them, D-0252.)
- **D-0167, D-0169** (Thomas): 60 frames/s first; the unlocked frame rate comes later as a
  remaster option; the game clock is not touched; universal rules per mechanism.
- **D-0170** (Thomas): borderless window and other aspect ratios later; before the
  remaster, a static whole-menu audit and resolution/full-screen witnesses under one
  universal rule (1080p, 1440p, 4K, one odd resolution).
- **D-0172** (Thomas): shaders and pipelines precompiled into a checked disk cache with a
  loading screen and a "reload shaders" entry, after the families' pipelines are stable.

## D-0177 - Repository cleanup: keep the waves, their preparations and the decompiled code (Thomas, 2026-10-13)

Thomas (project chat, 21:39-21:40 Paris): keep only wave 1 and wave 2, their preparations
and the decompiled code, to lighten the context each session rereads; the older
investigations no longer matter. Done: tag `archive/pre-cleanup-2026-10-13` on `main`
823437f3; investigations I-0001 to I-0065 removed from `main`; I-0066 (« DS3 DX12
Vague 1 ») and its static preparations I-0067, I-0068 and I-0069 kept (the native build
reads I-0067's quality table, and the wave documents link all three); STATE, ROADMAP and
this log summarised; `native/`, `decompiled/`, `scripts/`, the ledger and the external
Evidence Store untouched. `Test-Project.ps1` counts links into removed investigations as
archived ([ARCHIVE.json](../.project/ARCHIVE.json)). Never restore the removed directories
into `main` automatically.

## Consumed or superseded (titles only)

| ID | Title |
|---|---|
| D-0029 – D-0083 | Phase, extension and integration decisions of I-0015 to I-0035 (index in tag `archive/pre-cleanup-2026-10-02`) |
| D-0085 | Native scene/pass ownership with separate lifecycle/resource entries |
| D-0086 | Accept I-0036 and implement genuine native DS3 content next |
| D-0087 | Own one whole current model's native geometry inspection pass |
| D-0088 | Acquire fresh CPU source at the later resource factory |
| D-0089 | Accept genuine native content and pursue the actual game frame here |
| D-0090 | Own a coherent native scene-to-game-frame responsibility |
| D-0091 | Complete native rendering remains the programme target |
| D-0092 | Replace the selected final-output pass and its lifecycle |
| D-0093 | Own the complete selected scene preparation/recording transaction |
| D-0094 | The next live test must be exclusively D3D12 from startup (superseded by D-0115) |
| D-0095 | Own the enclosing GraphicsStep initialization transaction |
| D-0097 | Own the GraphicsStep CPU lifecycle before registration |
| D-0098 | Complete required replacements before making D3D11 unavailable |
| D-0099 | Complete normal-game coverage, offline development and binary cutover (partly superseded by D-0115) |
| D-0100 | Common renderer envelope with incremental offline family implementation |
| D-0101 | Native scene work production next within the common owner |
| D-0102 | Coherent ordered primitive pass under the existing native frame owner |
| D-0103 | Astra may assist directly in targeted decompilation (replaced by D-0152, D-0163) |
| D-0104 | One live acquisition; crash is data, without automatic RCA |
| D-0105 | Preserve all new directions in main and transfer after a clean checkpoint |
| D-0106 | Integrate this branch into main before the new conversation |
| D-0107 | Bounded real startup acquisition after texture ownership checkpoint |
| D-0108 | Native process-entry delivery acquisition before original startup |
| D-0109 | Admit Windows system DLLs and acquire once after validation |
| D-0110 | Own the native startup transaction and retain the original CPU integration gate |
| D-0111 | One targeted original startup-code acquisition |
| D-0112 | Own loading-window extent transitions |
| D-0113 | Stop technical work and merge the preserved continuation |
| D-0121 – D-0128 | I-0041 to I-0045 integrations and next workstreams |
| D-0130 – D-0141, D-0143 – D-0148 | I-0045 to I-0062 complete, merged, successor opened |
| D-0149 | Coverage zones and the level-83 save slot (succeeded by D-0174) |
| D-0152 | Fable only on Thomas's word, except the pre-merge review (kept through D-0163) |

## D-0178 - Wave 2 is the definitive version: remastered settings replace the original ones (Thomas, 2026-10-14)

Thomas (project chat, 02:03-02:04 Paris): wave 1 is finished first as a base identical to
the original; wave 2, built on it, is the definitive version. Its remastered settings
replace the original settings with their own adjustment levels, and the original is no
longer kept as an option. One exception: the colorimetry keeps the original option (it
touches the art direction) beside a new one inspired by Dark Souls Remastered and Elden
Ring. This replaces, for wave 2, D-0175's "each later replaceable among the remaster's
options" reading of keeping both; wave 1's passes stay bit-identical to the original.

## D-0179 - The shader and pipeline cache moves to wave 2 (Thomas, 2026-10-14)

Thomas (project chat, 02:56 Paris): the shader preloading of modern games (D-0172: the
checked on-disk cache, its loading screen and the "reload shaders" entry) belongs to
wave 2. Wave 1 keeps compiling pipelines as it does now.

## D-0180 - The sun shadow mask and the SSAO join wave 1 (Thomas, 2026-10-16)

Thomas (thread « Statique régression d'image », 08:57 Paris): the screen-space sun shadow
(mask t32, with its composite and blur) and the SSAO are added to wave 1. Reason: the
static analysis (IMAGE-REGRESSION-STATIC-LEADS) shows the native scene 3 times brighter on
upward-facing surfaces (Firelink ash white, Village veiled) because the sun is unshadowed
and the AO neutral; wave 1 is the identical base and cannot match the original without
them. The shadow atlas casters needed by the mask come with it. The wave-2 remaster passes
built on them (D-0178) are unchanged. ROADMAP wave 1 and wave 2 carry the move; the ready
sheets are SSAO-WAVE1-READY, SHADOWS-WAVE1-READY, SSAO-D3D12-DESIGN-NATIVE, SHADOWS-NATIVE.

## D-0181 - Step reviews while usage allows, and side-by-side image validation (Thomas, 2026-10-16)

Thomas (project chat, 05:26 Paris, relayed by the coordinator): back to the review practice in force
before D-0176 (a light Opus review after each step, the full Opus review at the workstream's final
merge) while the weekly usage limit is low; around 60 % of the limit, D-0176's single review at the
merge again. Kept from the coordinator's proposal: before a visual workstream closes, its final image
is compared with the original side by side at every witness zone, in both sky states where the
progression changes the light (Firelink and the Undead Settlement have early and late states), and
the captures are shown to Thomas. D-0163 itself only names the reviewer (Opus 5.5 high in place of
Fable); this decision carries the cadence. WORKFLOW's pre-merge review rule carries it.

## D-0182 - Wave 2 is « Dark Souls III: Rekindled »; a launcher for Thomas (Thomas, 2026-10-14 to 16)

Thomas (project chat, during the pause of 14-16 October, relayed by the coordinator): the remaster
is named « Dark Souls III: Rekindled ». Its title screen follows Thomas's mock-up (project chat,
2026-10-15; the image, uploaded by Thomas on 2026-11-08: [title-screen-mockup.png](../.project/design/title-screen-mockup.png)): the original logo, « REKINDLED » letter-spaced in ember colour between two rules; the
title appears as in the original, then a glow crosses REKINDLED and fades; behind it, rising ember
particles, discreet smoke and glow, all sober. The start logos are on a black background, in the
manner of Elden Ring, in place of the white Bandai Namco screen. HDR10 output with only two
settings: maximum nits and paper white. Separately, now: a simple double-click launcher so that
Thomas can start the game himself on the native D3D12 renderer, without instrumentation. ROADMAP
wave 2 carries the remaster items.

## D-0183 - The sun shadow is on by default; 60 frames/s comes back through the cascade cache (Thomas, 2026-10-16)

Thomas (thread « Tests des menus », 18:23 Paris), following the recommendation: the screen-space
sun shadow (D-0180) is on by default, since the lit image then matches the original at every zone
witness, although the native renders 55-56 frames/s at the High Wall and the Undead Settlement
(m40 60; m45 22, 30 without the shadow). The next workstream restores 60 frames/s by the original's
mechanism: the cascade cache (SHADOWS-NATIVE 1.4: a cascade is kept while the camera slice stays
inside its previous volume), then the m45 collect cost. `no-sun-shadow.flag` opts out.

## D-0184 - The sun casters follow the original's selection, not a cascade cache (Opus, 2026-10-16)

D-0183's next workstream assumed the original reaches 60 frames/s by keeping its cascades (SHADOWS-NATIVE 1.4). Measured in the
original (K-0452), the cache never acts: all four cascades are redrawn every frame. Its lower cost comes from its casters: only the
G-buffer-category entities in the cascades (K-0453), where the native drew every map piece of its boxes. The native shadow walk
now visits that category; no cascade cache is built (it would also freeze moving characters' shadows, which the original does not
do). Result: 60 frames/s at m40, m37, m50, 59.9 at m30, 59.5 at m31; m45 (31) is bound by its collect, the next workstream.

## D-0185 - The FFX draws run the original's own draw side, captured at the GX context wrappers (Opus, 2026-10-17)

The classic FFX effects (E01 first) are drawn natively by running the original's draw side as it is (the scene-control draw, the
pass slot 4 recording, the command callbacks 299E40, the renderers and builders, slot 27) against native GX contexts, and
capturing every GX statement at the GX context wrappers (19202xx-1921xxx) into draw records the D3D12 stage encodes. Rejected:
transcribing slot 27 and the three builders (thousands of lines each) or faking a D3D11 context. What the draw side needs that
wave 1 left out is supplied by general mechanisms: the FFX textures and the shader set's programmes as native tokens through the
engine's own lookups, the update list's CPU particle simulation run in the native update, the pass context's view volume, frame
copy and depth. On by default with the effect system (`no-ffx-draw.flag` opts out); the GPU particles (E07) stay skipped. Evidence:
FFX-DRAW §2 (Reference1925-1972, Native1928-1972); side by side with the original: the bonfires of m37, m40 and m30, the m30 tower torch and the torch of the Argo save before Vordt's arena (Native1965-1966, 1968, 1972).

## D-0186 - The GPU particles (E07) are on by default (Opus, 2026-10-17)

The GXVfx standard particle appearances run their own simulation (2A6A30) and draw (2A7E70) against the native GX contexts, as
D-0185's classic draws do; their compute, copy, clear and indirect statements are encoded in D3D12 with hidden UAV counters taking
the binds' initial counts. Evidence: the original and the native simulation agree per system (emissions, particle count, the CPU
images of the constant blocks; Reference2001, Native2002), the live count is the original's (2-5 embers at the m37 and m40
bonfires and at m45, m31, m50), no fault and no frame-rate change (Native2002-2012; m45 59.0 against 59.1 frames/s), side by side at m37, m40, m45, m31, m50 (Native/Reference2003, 2005-2008). On by
default with the effect draws; `no-ffx-gpu.flag` opts out. The m37 falling snow, attributed to E07 by the plan, is not a GPU
particle system (FFX-DRAW "E07: the alive count answered"): it stays open in the classic family.

## D-0187 - Two frames in flight by default (Thomas, Opus, 2026-10-18)

Thomas (2026-10-18, before the resolutions): two frames in flight by default, as in modern games; the launcher must play that
configuration. The module now opens two frame slots and the waitable three-buffer chain unless `one-frame-slot.flag` or
`no-waitable-present.flag` is present; the launcher removes every flag and places only `measure.flag` and `no-debug-layer.flag`,
so it plays the default. This replaces the D-5 verdict of phase D (one slot until the frame got shorter). The kept draws (C4-2)
now hold one table copy per slot for the draws with bounded CBVs, without which two slots halved the frame rate at m45.
Evidence: PERSISTENT-RECORDS-DESIGN "C4-2 with frame slots" (Native2096-2104: m45 59 frames/s at two slots; m40 and m37 0 debug
errors and side by side with the original).
Complement (Thomas, 2026-10-18 09:54-09:59 Paris): two frames in flight in every case, wave 2 included (no option for three:
two is the D3D12 base); NVIDIA Reflex comes in wave 2 with Off, On and On + Boost.

## D-0188 - Wave 1 closes early; the final review corrects the remaining gaps (Thomas, 2026-10-18)

Thomas (2026-10-18 18:43-18:47 Paris): finish wave 1 earlier and rely on the final control review (Fable 5.1, high effort) to
correct the remaining gaps in one pass; the review also gives optimisation axes without running the optimisation workstream.
The darker native scene at Crucifixion Woods (Native2161 / Reference2160) goes to that review undiagnosed. The thread « Sang sur
persos, eau, siège » therefore stops after the Handmaid chair's identification: the water planar reflection (WT-6/WT-7), the
Crucifixion Woods cross and the chair's correction are handed to the review with their evidence
([FABLE-REVIEW-PREP](../investigations/I-0066-independent-native-startup/FABLE-REVIEW-PREP.md)).
Outcome (2026-10-18 21:25 Paris, [FABLE-REVIEW](../investigations/I-0066-independent-native-startup/FABLE-REVIEW.md)): the m33
darkness (bank group lookup through the area's live layers), the Crucifixion Woods cross (zero-bounds entities kept), the output
extent (the engine's size S at any resolution) and six code findings corrected; the water planar reflection view coded opt-in
(`water-reflection.flag`, witnessed at the lake); the Handmaid chair stays open with its evidence; the optimisation axes (§5)
and the D-0157 architecture reading (§6) are the brief of the next workstream.

## D-0189 - The water planar reflection is on by default (Thomas, 2026-10-18)

Thomas (2026-10-18 21:58 Paris, following the coordinator's recommendation): the native planar reflection view (WT-6/WT-7,
[FABLE-REVIEW](../investigations/I-0066-independent-native-startup/FABLE-REVIEW.md) 1.7) is enabled by default, as the
original always has it; `no-water-reflection.flag` opts out. Known gap to correct in the next workstream: the mirrored view
carries no sun shadow (its reflection reads a little brighter than the original's where the sun shadows the reflected scene),
and its cost (about 3 ms of collect at the lake, 45.9 against 53.3 frames/s in the lake windows) belongs to the optimisation
axes (kept draws for the view, the culler walk).

## D-0190 - Wave 2: native effect contracts replace the FFX statement replay (Thomas, 2026-10-18)

Thomas (project chat, 2026-10-18 23:07 Paris): for wave 2, replace the FFX statement-replay layer inherited from GX/D3D11
(D-0185/D-0186, [FABLE-REVIEW](../investigations/I-0066-independent-native-startup/FABLE-REVIEW.md) 6.2) by semantic effect
contracts produced by the DS3 engine (create, simulate, update, draw, destroy; emission, parameters, events, lifetime) and
consumed by the native D3D12 backend, which owns the particle buffers, UAV counters, transitions, compute, indirect draws and
synchronisation. The GPU components already built (resources, shaders, simulation) are reused; the replay is removed family by
family once each family matches the original, with no GX fallback left. ER is an architectural reference only. Not coded in
wave 1; performance gains are measured, not assumed.

## D-0191 - Wave 1 closes after two cleanups; wave 2 starts with the backend redesign (Thomas, 2026-10-19)

Thomas (project chat, 2026-10-19 05:49 Paris, agreeing with the coordinator's recommendation): after the optimisation
workstream merges, a documentation cleanup then a code cleanup, then wave 1 closes without chasing each remaining small gap
(the gaps stay listed in STATE). Wave 2 then starts with the backend redesign: bindless binding, pipeline states, several
queues and the native FFX effects (D-0190). Order of performance work (Thomas, 05:42): remove redundant work first, measure,
then parallelise what remains; the measures report the collect and the p99 frame time, not only the average.

## D-0192 - Wave-1 closure tests (Thomas, 2026-10-19)

Thomas (project chat, 2026-10-19 07:13-07:16 Paris): the wave-1 closure adds one full-screen test and nothing else; the movie
sound is acquired if the intro cinematic plays with its sound; the resolutions are acquired (small windows and 1080p were
tested), only a resolution change from the game's menu stays a reserve; the graphics levels and the DX12 launcher become wave-1
reserves treated during wave 2; the complete game and DLC without D3D11 stays the mandate's acceptance condition. Thomas runs
the last tests himself after the code cleanup. Untended Graves joins the side-by-side witnesses (07:27). Closure note:
[WAVE1-CLOSURE](../investigations/I-0066-independent-native-startup/WAVE1-CLOSURE.md).

## D-0193 - Before the wave-1 closure: the texture hitches and the D2 paint (Thomas, 2026-10-19)

After his closure tests (WAVE1-CLOSURE §2.3-2.4), Thomas (16:58 Paris): correct the native-only texture-admission hitches
(the original is smooth in D3D11) and the character paint D2 (requests received, none painted) before wave 1 closes. Full
screen goes to wave 2 with the borderless window (full screen first). Archdragon Peak's frame rate stays with wave 2's scene
data persistence (recommended: a second measurement scene beside m45).

## D-0194 - Wave 1 closed: the first milestone reached with reserves (Thomas, 2026-10-19)

Thomas (thread « Nettoyage du code », 2026-10-19 22:22 Paris, « Oui je valide »): the first milestone of C3, « jeu jouable en
natif » (DS3 starts, loads saves and plays with the native D3D12 renderer and no D3D11 device at the witness zones), is
reached, with the reserves of [WAVE1-CLOSURE](../investigations/I-0066-independent-native-startup/WAVE1-CLOSURE.md) §2. His two
conditions of 16:59 are settled: the character paint D2 is shown live (D-0193, DECALS-PAINT-READY §8) and the texture
hitches, measured as fresh model re-admission while the world streams, go to wave 2's scene data persistence. Wave 1
(I-0066) closes; the final acceptance (the complete game and DLC without D3D11) stays the mandate's condition. Wave 2 opens
with scene data persistence (ROADMAP C3, measurement scenes m45, Archdragon Peak and the Firelink walk).

## D-0195 - Wave 2 follows the architecture of Elden Ring (Thomas, 2026-10-20)

Thomas (thread « V2 : persistance des données de scène » and project chat, 2026-10-20 00:39-00:44 Paris), after I-0070's first
tranche: « Fusionne déjà ce travail mais on va quand même largement s'appuyer sur Elden Ring, FromSoftware a déjà réussi à
moderniser son moteur avec DX12 alors on va copier (et améliorer plus tard) »; « On copie juste l'architecture, évidemment que
les données (effets, paramètres graphiques, modèles 3D, shaders etc) ne seront pas les mêmes »; « On ne jette pas pour autant
notre travail, on le transforme »; « A noter comme règle principale pour ce début de la Phase 2 »; « Et supprimer ce qui
contredit cette règle pour pas qu'elle soit noyée ni ignorée ». Rule: every wave-2 design starts from the Elden Ring engine's
D3D12 architecture recovered by deep static decompilation (Sonnet 5.5 may carry it, Opus checks the sources it relies on);
the native backend copies that architecture, then improves it; DS3's data and behaviour are verified on DS3. The rule replaces
D-0060 (« ER an architectural witness, never DS3 evidence ») and the WORKFLOW wording that weighed ER against DS3-only work.
I-0070 merged as its first tranche; I-0071 studies ER's scene persistence, texture binding and distant fades.
Nuance (Thomas, 2026-10-21: « ER a des soucis de stuttering, à nous de ne pas recopier bêtement ces soucis mais de les
déduire »): ER's known defects are not copied; each design names where ER has the defect and what the native backend does
better (WORKFLOW, first rule).

## D-0196 - The timing project comes back; the original is the reference for the engine thread (Thomas, 2026-10-21)

Thomas, 2026-10-20 22:23 and 2026-10-21 00:05-00:06 Paris (« comme étude, pas forcément pour copier vu que c'est DX11 »;
« il peut intégrer les connaissances et la décompilation de mon dossier LTF TRTF au dépôt main »; « c'est moi, c'était un projet
Codex/GPT »). The original D3D11 renderer is measured as a reference for the engine thread's cost (I-0078: its main thread works
about 4.7 ms a frame at m45), not copied. The timing project (LTF/TRTF, I-0005 to I-0014, moved out on 2026-10-02) returns under
`timing/` with its documents and decompilation, its own numbers kept; its raw captures stay in the Desktop folder (D-0142).

## D-0197 - A streamed resource not ready delays its model's appearance, never a stand-in (Thomas, 2026-10-21)

Thomas (project chat, 2026-10-21 04:15 Paris, « Je choisis la seconde aussi, le pop in c'est moche »): if the native backend
stops making DS3's engine wait for a texture's creation (ER's polled loading, I-0079 ER-STUDY and I-0073), a texture not yet
resident delays the appearance of the models that use it by a few frames, as ER publishes a texture only once complete; a
neutral stand-in texture (visible pop-in) is never drawn. The game's clock stays untouched (D-0167). Done only if the
measurement after I-0079's creation fix still shows slow frames from the engine waiting on creations.

## D-0198 - Fable only on Thomas's word; reviews never reduced (Thomas, 2026-10-21)

Thomas (project chat, 2026-10-21 05:13 Paris), settling two WORKFLOW rules an audit found contradictory: 1) « Tu me proposes
et je tranche ou c'est moi qui demande directement de l'utiliser, comme il est cher » : Fable 5.1 is never started without his
explicit agreement (Opus proposes, Thomas decides, or Thomas asks); 2) « On ne limite plus rien, je veux éviter les erreurs de
code » : the Opus reviews are never reduced by the weekly usage limit: a light review at each step and the full review at
the workstream's final merge, always. Replaces D-0176 and D-0181's quota rule; D-0181's visual rule stays.

## D-0199 - Background processes run hidden on Thomas's PC (Thomas, 2026-10-21)

Thomas (2026-10-21, relayed by the coordinator with the documentation cleanup he validated): the processes and builds a
thread starts in the background run hidden, without opening a visible cmd or console window on the PC, headless Ghidra
included. Rule and means in [WORKFLOW](../.project/WORKFLOW.md#background-processes-on-thomass-pc-thomas-2026-10-21-d-0199); the
game during an announced live test is the exception.

## D-0200 - The order of the next workstreams (Thomas, 2026-10-21)

Thomas (2026-10-21, relayed by the coordinator with the documentation cleanup he validated; the stability tools « je valide
les 4 » and « Saccades, tranche 2 » before the minidumps were his earlier words of the same day, ROADMAP): after the
documentation cleanup, « Saccades, tranche 2 »; then the automatic minidumps together with the two holes I-0080 found (the
D3D12 debug layer on by default when `no-debug-layer.flag` is missing; a GPU hang that closes the game with no message and
no dump); then the path replayed identically with a blocking threshold at the merge; then the « audit des pièges DX12 »
(from I-0080 ER-STUDY section 8); then the night endurance test. The engine thread's third tranche toward 120 frames/s, which this
order does not place, is kept after them by default until Thomas says otherwise. No pop-in stays the rule for streamed
resources (D-0197).

## D-0201 - Crash dumps and GPU loss reported; the debug layer on request (Opus, 2026-10-21, under D-0200)

I-0082 ([RESULT](../investigations/I-0082-crash-dumps-gpu-loss/RESULT.md)), the first stability tool of D-0200: every native
fault (a fatal exception, a refusal, a native failure, a removed device, a GPU hang at the 5 s completion deadline, a
`std::terminate`) leaves a minidump and a symbolized report written by the controller from outside the game, and in play a
message to the player; DRED breadcrumbs and page faults are on by default (`no-dred.flag` opts out; no cost measurable on the
Firelink walk). The D3D12 debug layer is on only with `debug-layer.flag` or `gpu-validation.flag` (replacing 1118's
`no-debug-layer.flag`), and its absence is tolerated; validation runs keep it through the runner (unless `-NoDebugLayer`).
The pre-existing exit heap fault at a quit is logged, not dumped (`exit-dump.flag` to dump it for its diagnosis). Amended 2026-10-29 (I-0099 STUDY step 4, the fix VERIFIED by K-0484): natively every fatal exception at a quit is dumped and fails the start check and the memory leg (through the controller's crash record), `exit-dump.flag` removed; the Reference, the unmodified original that still raises the fault, logs a heap corruption as `reference_heap_corruption` instead of dumping it.

## D-0202 - The replayed path gate at every merge (Opus, 2026-10-21, under D-0200)

I-0083 ([RESULT](../investigations/I-0083-replayed-path-gate/RESULT.md)), the second stability tool of D-0200: the Firelink path
(walk, sprint, camera turn) recorded once in `scripts/replay/firelink.path.json` and replayed from one process. Before a merge
into `main` that changes `native/`, `scripts/Invoke-ReplayGate.ps1` (an announced live run) builds the committed tree and runs
two measure runs and one validation run; it fails on a fault, on the mean of the two measure runs over a threshold (frames over
34 and 25 ms at the frame end, frames held two refreshes, game steps over 34 ms) or on the standstill picture departing from the
reference. The thresholds come from `main`'s own spread (four runs, the formula in the record); a recalibration after an
improvement lowers them, a rise is Thomas's decision. A pass writes the record that `scripts/Test-Project.ps1` requires on
`main`. The original's numbers on the same path are filed beside the thresholds as the reference, not as the threshold.

## D-0203 - The video memory budget and one pipeline store (Opus, 2026-10-21, under D-0200)

I-0084 ([RESULT](../investigations/I-0084-dx12-pitfalls-audit/RESULT.md), [PITFALLS-AUDIT](../investigations/I-0084-dx12-pitfalls-audit/PITFALLS-AUDIT.md)),
the DX12 pitfalls audit: ER reads the video memory budget once and never reacts (I-0080). The native backend reads the local and
non-local usage and budget each pacing window; over a budget it releases, down to the excess, only the memory it holds on its
own account (free placed heaps for the card, idle staging chunks for system memory), never a resource the game keeps (D-0197:
no stand-ins, no pop-in), and the frame's targets carry a high residency priority so a demotion takes streamed textures first.
Every pipeline and root signature of the game path (the stages, the native lighting, velocity, composition, movie and FaceGen
passes included) goes through the one pipeline store, so they are kept in its library and precompiled with its list, as ER keeps
one cache per device. WARP keeps a library of its own.

## D-0204 - The memory leg, the nightly endurance and the layouts' lifetime (Thomas, 2026-10-22)

I-0085 ([RESULT](../investigations/I-0085-night-endurance/RESULT.md)), the night endurance test of D-0200, found the process's
private memory rising 151 MB a zone visit (7.4 to 15.4 GB over 54 visits, i85-long-2) and every zone's late frames rising with
it, while the 15-minute replayed path gate had passed: about 1,100 model layouts a load were keyed by their material object
(new at each FLVER load) and never retired, since the native path skips the engine's resolver cache that owns them in the
original. Rule (Opus, under D-0200): a layout leaves with its material, its engine prefix reused by the next build. Thomas
(2026-10-22, « Lance les 4 », to the coordinator's proposals): the replayed path gate adds a memory leg (Beta m31 loaded four
times from the title in one process; the title's private memory after the fourth visit against after the second, per visit,
over `memory_mb_per_visit` fails; filed at 40 MB, not calibrated from a spread), and a short endurance (30 minutes of the five
witness zones) runs on the latest `main` every night Thomas does not play, its regressions opened as workstreams
([WORKFLOW](../.project/WORKFLOW.md)).

## D-0205 - A test version with invincibility and revived bosses, never in the play version (Thomas, Opus, 2026-10-22)

Thomas (2026-10-22): to bring back the bosses already defeated in his end-game save (Shebang), all or chosen ones, and to be
invincible on F4, for his tests ([I-0089](../investigations/I-0089-test-tools/RESULT.md)). Rule (Opus, under D-0084): these are
tools of a separate test launcher (`tools/Launch-DS3-DX12-Test.ps1`, its own folder, `test-tools.flag`), absent from the play
launcher; the module code stays inert without that flag file. The saves are a sandbox: copied first, restored file by file by
SHA-256 when the game exits, a pending-restore marker refusing the next start if the console was closed early. The boss flags
are cleared through the game's own setter on a loaded world, so the boss returns when the zone reloads; no `.sl2` edit. The test
launcher never goes online.

## D-0206 - The gate's memory leg judges a robust slope over eight visits (Thomas, Opus, 2026-10-23)

The replayed path gate's memory leg (D-0204) judged the title's private memory after the fourth visit against after the second
(per visit). On 2026-10-22/23 the title's memory toggled by about 100 MB between visits, in `main` and in I-0092 alike (eight-visit
legs `i92-mem8-main`, `i92-mem8-pool`), which that two-point estimate read as +-50 MB a visit: the gate `i92-final` failed on it,
`main` failed the same leg once. Thomas (2026-10-23 01:51): make the leg robust, as severe against a real leak. Rule: eight visits;
the slope is the median of the pairwise slopes (Theil-Sen) of the titles after visits 2 to 8; the threshold stays 40 MB a visit.
Checked: both eight-visit legs pass (12.3 and 15.2), 0 false failures in 200 simulated flat legs with the 100 MB state, a 60 MB a
visit growth failing 196 of 200 ([I-0092 RESULT](../investigations/I-0092-effects-update-parallel/RESULT.md)). The gate takes
about 30 minutes.

## D-0207 - The gate's thresholds follow each performance merge down (Thomas, Opus, 2026-10-23)

The replayed path gate's frame thresholds (D-0202) were calibrated once, on 2026-10-21 (`i83-cal-m1..m4`), before the bottleneck
workstreams; since then the passing gates measure far fewer frames held two refreshes (62 allowed, 14.5 measured at
`i93-final`), so a regression of most of that gain would still pass. Thomas (2026-10-23, relayed by the
coordinator): after each merge that improves the frame times, lower the thresholds toward the level reached, with a noise margin.
Rule: after such a merge into `main`, the gate is recalibrated on `main`'s committed tree (`Invoke-ReplayGate.ps1 -Calibrate`, four
measure runs, the validation run and the memory leg, announced as a live test); the existing formula sets each threshold (the
highest of mean + 3 sd / sqrt(2), mean + 3 sqrt(mean / 2) and the highest run) and lowers it only when the new runs are lower
beyond twice their combined standard error; a threshold never rises without Thomas's decision; the memory threshold stays filed
(40 MB a visit). The calibration is recorded in `.project/REPLAY-GATE.json` with its runs.
First application (`i94-cal`, 2026-10-23 06:00, `main` with I-0094): frames held two refreshes 62 to 22 (runs 13, 11, 16, 16);
frames over 34 ms kept at 22 (mean 13 against 13.75 on 2026-10-21: not lower), game steps over 34 ms kept at 6 (1.5 against
1.75), frames over 25 ms kept at 105 (the formula gives 113: the mean rose from 84 to 92.5, a rise refused). The bottleneck
workstreams raised the unlocked frame rate (125 to 144 frames/s at m45) but did not lower the slow frames of the capped path:
those belong to the stutter workstream.

## D-0208 - The gate counts the display's repeated refreshes (Opus, 2026-10-23, I-0097, Thomas's stutter order)

The coordinator relayed Thomas's plan for the stutter workstream (2026-10-23): first correct the count of frames held two
refreshes, then recalibrate its threshold. I-0095 found it biased low: it kept a DXGI statistics sample only when PresentCount
advanced by exactly one, which misses most late frames. Rule: the gate's metric `held` is replaced by `display_repeats`, the
refreshes on which the display repeated a frame. It is read from the swap chain's frame statistics after each Present, as the
PresentRefreshCount advance minus the PresentCount advance, summed per 120-frame window (`display_refreshes` rows, with the
refresh rate). The same code reads the original's chain in its probe. The original's chain is windowed blt-model (swap effect
0), and DXGI returns `DXGI_ERROR_UNSUPPORTED` for it, so the original's display still needs PresentMon (an elevated session,
Thomas's decision). Threshold: 24, calibrated on `i97-cal2` (2026-10-23 17:31, the I-0097 branch with only the instrument
changed, runs 15, 17, 15, 15). The other thresholds were kept, because the new means were not significantly lower (frames over
34 ms 12.5, over 25 ms 74, game steps over 34 ms 0.5). The gate is inconclusive when a segment holds fewer than three quarters
of its display windows, or when the display is not at 59-60 Hz (at 120 Hz every 60 frames/s frame holds two refreshes). A
launch refused before the module ran (`prefix_refused`) is launched once more. On the walk, the display repeats about one
refresh at each area crossing, and the engine side's 25-34 ms band mostly does not reach the screen as a repeat (I-0097 RESULT).
## D-0209 - The gate is independent of the screen (Thomas, 2026-10-24, I-0098)

Thomas (2026-10-24 04:58-05:04, relayed by the coordinator: « dans l'idéal ce serait sur une base illimitée, il existe plein
de configurations différentes et justement j'ai deux écrans bien différents »): the replayed path gate no longer assumes a
60 Hz screen. Rule: `.project/REPLAY-GATE.json` holds one base per screen, keyed by its desktop mode (« 1920x1080@60 »,
« 2560x1440@280 », `scripts/Get-Screens.ps1`), each with its thresholds, calibration, reference picture and last pass; bases
are never mixed, and a merge needs a pass on every required base (`Test-Project`). Each gate places two measure runs on each
screen (the pilots move the game's window there); the validation run and the memory leg, which do not depend on the screen,
run once. The measures are relative to the run's frame period P (the median over its 120-frame windows of the mean of the
frames within 1.5 times the window's median): frames over 2.04 P and 1.5 P at the native frame end (34 and 25 ms at 60
frames/s: on 16 runs at 60 Hz the counts equal the fixed ones, 64 of 64), game steps over 2.04 P, and P itself (period_ms, a
uniformly slower run). The display count comes from PresentMon (Thomas's choice, 2026-10-24 06:09: he joined the Performance
Log Users group and installed Intel's PresentMon 2.6.0, so it runs unelevated): DXGI's frame statistics proved unreliable with
two screens (`i98-c1`: on the 60 Hz secondary screen they kept the 279.96 Hz screen's count and almost stopped; on the OLED
frames « held » 0 or 3 refreshes at 60 frames/s), and NVIDIA FrameView's PresentMon does not run outside its service. The
count is the display changes beyond the screen's cadence, in frame periods (one per repeat at 60 Hz). The 60 Hz base kept its
thresholds under the relative names, then its calibration on the new setup lowered frames over 2.04 P from 22 to 8 (D-0207);
its former display limit (20, DXGI, a single primary screen) is retired, display_late calibrated anew on each base; on the 60 Hz screen that starts from the native's defect there (K-0479), which Thomas accepted (2026-10-24 07:19, « Oui d'accord », to « sur l'ASUS, le point de départ sera donc mauvais (environ 260) »), the goal being the original's level in the stutter's tranche 3, after which D-0207 lowers it
(`i98-cal`: 1920x1080@60 8/36/137/6, 2560x1440@280 9/40/18/7, period 16.76 ms).
## D-0210 - The present pacer paces first, then syncs (Opus, 2026-10-24, I-0097 tranche 3, Thomas's order)

The cR4 pacer copied the original's rule (K-0009): at 59/60 Hz the frame goes to DXGI at once with Present(1), elsewhere it
waits for its deadline and presents with 0. Under PresentMon (I-0098) the native's presents on a composed 60 Hz screen left when
the render worker finished (14-19 ms apart), and the screen dropped 110-125 frames a run against the original's 26-79 (late
display 75-101 frame periods against 28-43). Rule: every frame waits for its deadline (anchor + interval / 60 s), then presents
with the DXGI interval x multiple where the screen is a whole multiple of 60 Hz within 1 Hz (Present(1) at 60 Hz, so an
exclusive full screen does not tear), with 0 elsewhere; a synced frame that woke within 1 ms of its deadline anchors there and steps by the screen's exact refresh period (its display path's rational rate) when within 2 % of the nominal one.
Measured: presents at 16.667 ms, 11-19 dropped a run, late display 12-17 on the ASUS windowed, the native under the original's
late display in every screen mode measured (K-0480). Two buffers, one frame in flight and the swap chain made on the target
screen did not help. The screen chosen at launch (`display-screen.txt`, the window placed before its swap chain is made) stays as the mechanism of the launcher's screen option Thomas asked for (2026-10-24 08:32).

## D-0211 - Streamed content prepared ahead, found in a cache in play (Thomas, 2026-10-24, I-0097 tranche 4)

Thomas (project chat, 2026-10-24 18:56 Paris, « On suit la méthode moderne donc », after the pop-in study
[`POPIN-RULE-STUDY-2026-10-24.md`](../investigations/I-0100-popin/POPIN-RULE-STUDY-2026-10-24.md) §5, first in the project files): as modern games do, the native part of streamed content (texture
decodes and uploads, model storages and buffers, GPARAM, the map's pipelines) is prepared ahead (during the loading screen, or
kept from DS3's earlier loads of the same content) and found by a cache lookup when DS3 asks for it in play, so DS3's loading
step, which the engine thread joins, keeps only DS3's own work. No pop-in: D-0197 stays (no stand-in, a model not ready
appears later) as the last resort. DS3's own loading policy (its FLVER CPU loads, FXR registration, map parts) is not forced
to preload (its game logic, enemies, collision and network). A longer draw distance is tried later (ROADMAP), not now.
I-0097 tranche 4 measured why (i97-t4-orig-prof-1 against i97-t4-prof-1): the joined step cost 2,549 samples natively against
the original's 1,371, the native admissions the difference.

## D-0212 - One pacing rule on every screen, and the game's V-sync as an option (Thomas, 2026-10-24, I-0097 tranche 4)

Thomas (2026-10-24 15:55 « C'est pas mieux une fonction universelle ? Comment font les jeux modernes? », 16:00 « une option on/off
pour la v-sync du jeu serait bien »): D-0210's rule depended on the screen (Present(interval x multiple) at whole multiples of
60 Hz, Present(0) elsewhere, so a fixed-rate exclusive full screen at 279.96 Hz tore). Rule: every frame paced by hand to its
deadline; with V-sync (the default) the screen's refreshes that fit the interval within 2 % are the DXGI interval (Present(1)
at 59.94-60 Hz as before, Present(2) at 120 Hz: an early frame still waits), Present(1) on any other rate (a VRR screen, G-Sync,
shows each frame as it arrives; a fixed one at the next refresh), Present(0) on a screen slower than the cadence (the original's
rule; Present(1) would queue the game down to the screen's rate). Without V-sync (`vsync-off.flag`, the launcher's `-NoVsync`,
until the remaster's options menu, ROADMAP) Present(0) on a chain allowed to tear. Measured (`i97-t4p`): the ASUS 11.1 and 15.8
late display periods, the OLED 8.3 and 9.0, V-sync off on the OLED 8.8; the gate's thresholds unchanged. Thomas's OLED has
G-Sync for full screen only (15:54): its windowed runs are not under VRR, a measuring condition of that base.

## D-0213 - The replayed paths beyond Firelink (Thomas, 2026-10-25, I-0097 tranche 5)

Thomas (thread « Étude tranche 5 », 2026-10-25 01:16 Paris, recorded in `investigations/I-0097-stutter/TRANCHE5-STUDY.md` §7):
the streaming path is the Undead Settlement with Beta (`shebang-zones-beta-m31`); Archdragon Peak with Eta, travelled there by
the bonfire in its own copy of the Shebang save, rested and saved, that copy kept hashed and installed at every run (Eta's m40
save unchanged, Alpha untouched); Lothric Castle the same way with Beta as a fourth path; Ariandel with Epsilon. The new paths
are study bases outside the blocking gate at first (Opus's advice). D-0211's preparation during the loading screen may add up
to 1 s to a zone load, to start with.

## D-0214 - The repository is the only reference for the working rules (Thomas, 2026-10-25)

Thomas (2026-10-25 03:02 Paris, relayed by the coordinator: « Tu as raison pour ta suggestion », « vaut mieux se référer au
dépôt »): the rules the coordinator kept on its own sheet are written in WORKFLOW (« Standing rules », with the Codex command for
the GitHub copy) and the sheet only points to them. A study thread always runs one workstream ahead: read-only, no merge, no
load on the PC during a live test.
## D-0215 - The gate's camera turn, and a fresh thread at every clean checkpoint (Thomas, 2026-10-25, I-0097 tranche 5)

Thomas (2026-10-25 07:55 Paris, « Ok pour tout », relayed by the coordinator): the Firelink gate's « turn » segment, which
turns nothing (the L key, RESULT tranche 5 second step item 2), is corrected in tranche 5 right after the Nsight Systems study
of the camera turn's cadence: a real turn by the mouse, the bases recalibrated on both screens. A limit may rise without
him when it is coherent (Thomas, 07:58: « Vous pouvez gérer les hausses sans moi, tant que c'est cohérent »): a changed path or
measure, never a regression, each raise with its numbers in its own entry of this log (named by `-AllowRaise`) and reported to
him. The working thread changes at every clean checkpoint, without waiting for it to be loaded (WORKFLOW,
Continuity, after every merge, item 4); a read-only study thread prepares the pop-in workstream meanwhile, respecting the
announced live tests.

## D-0216 - The gate's thresholds with the real camera turn (the pilot's raise under D-0215, 2026-10-25, I-0097 tranche 5)

The Firelink gate path's « turn » segment (the L key, which turned nothing) became the mouse turn of the study paths (two 10 s
cycles, 200 moves of 20 units in 8 s each; `scripts/replay/firelink.path.json`, path object b97181e7), so the path is 8 s longer
and now holds a real camera turn. Calibration `i97-t5-turncal` (2026-10-25 12:21-13:10, the committed tree of
`claude/quizzical-wiles-08d28d`, whose native change is the inert `cadence-trace.flag` only: `main`'s behaviour; four measure
runs a screen, a validation run, the memory leg -10.8 MB a visit, the exit heap fault 0xC0000374 at the leg's close, DS3's own).
Per-run values (frames over 2.04 P / over 1.5 P / display_late / game steps over 2.04 P): ASUS 1, 2, 4, 3 / 16, 17, 20, 18 /
9.0, 12.0, 17.48, 16.04 / 0, 1, 4, 3; OLED 4, 3, 3, 2 / 21, 20, 20, 18 / 11.97, 7.4, 8.81, 7.07 / 3, 2, 3, 0. The formula's
rises, applied (coherent: a changed path, no native change): ASUS display_late 21 to 22; OLED frames over 2.04 P 5 to 7, over
1.5 P 27 to 30, display_late 15 to 16. Kept: every other limit (the ASUS frames over 2.04 P at 8 and the OLED game steps at 7,
the new means not significantly lower; the periods). Reported to Thomas.

## D-0217 - The repository audit's cleanup: a short resume checkpoint, rules in the present tense, the stop guard on the pilot only (Thomas, 2026-10-25)

Thomas approved every suggestion of the repository audit (« J'approuve tout », 2026-10-25 21:29 Paris, on the audit,
`.project/AUDIT-2026-10-25.md`, removed 2026-11-09 with Thomas's yes, in Git's history before `56358b69`). STATE's resume rows are rewritten at each checkpoint, never appended to (`Test-Project` warns over 300 words a
row), with a « Pilot branch » row; the stop guard holds only that branch, every other thread ends its turn freely; WORKFLOW states
the rules in force in the present tense, their dated wording kept in tag `archive/pre-cleanup-2026-10-25`; repeated procedures
are the project's skills and its reviewers subagents (`.claude/skills/`, `.claude/agents/`, tracked); a study that guides a
workstream is copied into its investigation; the live-run know-how lives in `.project/LIVE-RUN-NOTES.md`, not in a private
memory; the repository's commits are signed « Thomas (DS3 Rekindled) ». Deletions of branches stay Thomas's to run (WORKFLOW).

## D-0218 - The gate on one screen, its memory leg only where it matters (Thomas, 2026-10-26)

Thomas (2026-10-26 00:09-00:10 Paris, relayed by the coordinator: « C'est encore obligé le seuil bloquant ? », « Je pense que la
relecture Opus xhigh suffit », « Je suis ta recommandation »), on the coordinator's recommendation: the replayed path gate stays,
lightened. It runs on one screen, the ASUS (1920x1080 at 60 Hz, its base already calibrated), unless the pilot justifies the
OLED (a change that only its rate, G-SYNC or full screen shows); the OLED's base is kept in `REPLAY-GATE.json`, no longer
required. Clarified at the branch's final review (2026-10-26, told to Thomas): `Test-Project` requires the ASUS's base, so a
justified OLED gate is run in addition to the ASUS (`-Screens 1920x1080@60,2560x1440@280`), never instead of it. The memory leg (eight Beta m31 visits, D-0204, D-0206) runs only when the branch touches the memory, the loadings or
the streaming (`Invoke-ReplayGate.ps1 -Memory`). The validation run under the debug layer stays. The final review of the branch by
a fresh Opus at xhigh (D-0198) stays required before every merge. Replaces D-0209's two screens for the gate; applied from I-0097
tranche 6.

## D-0219 - The display-grid cadence abandoned; the cadence by the modern DX12 method after the pop-in (Thomas, 2026-10-26)

Thomas (2026-10-26 00:31 Paris, relayed by the coordinator: « Je suis d'avis de suivre la méthode moderne des jeux DX12 plutôt
que de continuer de bricoler, après le chantier pop in »), after two control series of I-0097 tranche 6 without gain (RESULT
tranche 6 items 5, 7 and 8: the grid not engaged, then engaged and worse than `main`'s pacer, 10 and 4 frames held in the turn
against 4 and 1). The tranche stops: no third series; its native code (the engine's wait on a vertical blank grid, the vblank
thread, the presents on the grid) does not go into `main` and stays on tag `archive/i97-t6-display-grid`; `main`'s pacer
and its gate record stand. Its studies, D-0218's lighter gate, the start time past midnight of the runners and the notes merge.
The cadence comes back after the pop-in workstream as its own workstream, « Cadence, méthode DX12 moderne » (ROADMAP): the
frame pacing of modern D3D12 games (a waitable swap chain, a controlled frame latency), Elden Ring's D3D12 architecture checked on
DS3 (D-0195), studied ahead.

## D-0220 - The pop-in workstream: DS3's draw decision first, then the modern layer, without any stand-in (Thomas, 2026-10-25 and 2026-10-26)

Thomas (project chat, 2026-10-25 08:25-08:27 Paris, relayed by the coordinator: « on fera mieux que ER en supprimant ces défauts
de pop in pour se rapprocher des jeux modernes sous UE5, même si nous on a pas nanite », then « Ok, on fera la méthode moderne
alors, vu que c'est mieux ? »; recorded in [POPIN-STUDY](../investigations/I-0097-stutter/POPIN-STUDY.md) §8), and the order he
validated for [I-0100](../investigations/I-0100-popin/CONTRACT.md) (2026-10-26, relayed at 01:02): the pop-in is removed in the
whole game in two steps: first the native's draw decision brought back to DS3's (T2a, the comparison base), then a modern layer
on top (T2b: dithered fades, a margin against flicker, distant characters drawn every frame, preparation ahead), to do better
than DS3 and Elden Ring; never a stand-in (D-0197, D-0211): an object not ready appears a few frames later. The order: the
pipeline library's correction (POPIN-STUDY C5: a stamp of the module, adapter, driver and runtime, the library reset with every
new build), then T0 `popin-observe.flag` with the camera turn's first admissions, then the evaluation of preparing every model
of a zone at its load.

## D-0221 - The spot lights and local-light shadows, a workstream of its own after the pop-in (Thomas, 2026-10-26)

Thomas (project chat, 2026-10-26 17:01 Paris, relayed by the coordinator: « Et oui d'accord pour le chantier à part »), on the
proposal of I-0100 tranche 3 step 1 (RESULT): the map's spot lights are not drawn natively (45 at the Anor Londo bonfire, 46 at
Central Irithyll), nor any local-light shadow (the selection SW-7 only, opt-in), and the Anor Londo stairs' shadows are very
likely a local light's. They become the workstream « Spots et ombres locales », after the design of
[SHADOWS-WAVE1-READY](../investigations/I-0066-independent-native-startup/SHADOWS-WAVE1-READY.md) §1.5 and §3 (the spots'
per-frame items and the deferred `DSpotLight*` programmes, then the local atlas, the face passes 9..0x38 with the category 0x04
casters and the shadowed programmes), in its own thread after the pop-in workstream unless Thomas says otherwise; I-0100 does not
take them (ROADMAP).

## D-0222 - Option A: the launch LOD scale kept through the session; the order after the pop-in workstream (Thomas, 2026-10-27)

Thomas (project chat, 2026-10-27 02:05 Paris, relayed by the coordinator: « Va pour A, après tout c'est un remaster, pas une copie
de l'original »; 03:47: « la règle universelle, pas de cas par cas »), after the LOD scale study (I-0100 RESULT « The LOD scale
study »): DS3's views take their reference {height, fov} from a global its camera rewrites every frame ({720, 55°} until then), so
its LOD and draw distances are farther from a launch to the first death or travel than after it (1.32 times at a 720 client, about
2.6 times at 1440: the reference height 720 against the render's); Elden Ring sets its reference once (43°, height 0). The native
renderer keeps DS3's global at its launch pair by default (its writer 6E770 skipped: every view of every load made as at a launch,
one rule at any resolution; `no-lod-launch-scale.flag` gives DS3's own behaviour; the Reference mode stays the original unless
`lod-launch-scale.flag`). A change beyond the original (T2b): its cost not measurable at 60 Hz on the CPU and in memory at a
1280x720 client on the ASUS; its GPU cost at 1440 and 280 Hz on the OLED, where the distances are about 2.6 times B's, not
measured, to read with Thomas present. Thomas also set the order after the pop-in workstream (2026-10-27 00:20): « Spots et
ombres locales » (D-0221), then the exit heap fault I-0099, then « Cadence, méthode DX12 moderne » (D-0219).

## D-0223 - The camera turn's first admissions: made cheap, not prepared at the load (the pilot, 2026-10-27, I-0100 tranche 7)

I-0100's contract (scope item 3) asked whether preparing every model of a zone during its loading screen (D-0211) removes the
camera turn's first admissions, measured before it is built. Tranche 7 measured their cost (RESULT tranche 7): the layouts'
programme sets built at first sight, almost all of it work that depends on the effect, programme path and layout type alone.
Decision: that work is kept per recipe set (recipes, analyses, block plans, the effect found by name), as Elden Ring keeps the
shader choice beside the material (I-0071 ER-STUDY A); no preparation pass at the load. Measured: the Settlement turn's longest
admission 24-25 to under 5.3 ms, its frames over 25 ms 5-6 to 1. What a first sight still costs (about 50-60 ms of pool time on
the Settlement path, mostly a pre-existing rescan of absent loose programmes) would only move to the load. D-0211 stays the rule
for streamed content (textures, storages); this decision reverses ROADMAP's « justified by the camera turn's first admissions ».

## D-0224 - The order after the pop-in workstream, and opinions go to Thomas directly (Thomas, 2026-10-27)

Thomas (project chat, 2026-10-27 20:54 Paris, relayed by the coordinator: « les plantages de ce soir passent en 2, ensuite le
crash à la fermeture I-99 là, les spots et ombres locales, et enfin la cadence »), after three refusals in his evening play (STATE
« Open gaps »: the non-finite pose at Iudex Gundyr's death, the model method at Archdragon Peak, the lost exclusive full screen):
after I-0100, those three faults, then the exit heap fault I-0099, then « Spots et ombres locales » (D-0221), then « Cadence,
méthode DX12 moderne » (D-0219). Replaces D-0222's order. Also (Thomas, 20:48, « Pourquoi tu ne m'as pas donné ton avis ici
mais plutôt en privé au coordinateur ? »): every opinion or proposal that concerns Thomas goes to him in the thread; the private
notes to the coordinator carry only test times and « PC libre ».

## D-0225 - More delegation to Sonnet, grouped live runs, measures checked before they are believed (Thomas, 2026-10-27)

Thomas (project chat, 2026-10-27 21:59 Paris, relayed by the coordinator: « Va pour les 3, ce serait bien aussi de déléguer
davantage je pense, pour économiser de la mémoire au pilote. Les Sonnet sont de très bons assistants. Haiku j'ai moins
confiance »): a study thread opens on the three refusals of his evening play (read-only, never the game, no Ghidra during the
pilot's announced tests); the pilot groups its live runs into fewer, fuller slots; a measuring tool and a reader are checked
(a short trial, one journal read by hand) before their numbers are believed; the pilot delegates more to Sonnet subagents
(journals, analyses, reviews of documents), Haiku only for trivial mechanical work. Written in WORKFLOW « Roles and delegation »
and « Live game rule ».

## D-0226 - Lighter tests per tranche (Thomas, 2026-10-27; replaced by D-0227)

Thomas (2026-10-27 23:26 Paris: « c'est une perte de temps de voir le personnage courir dans le vide à Firelink pendant 30min à
chaque étape »; 23:28, « Je suis ton avis », on the middle way the coordinator relayed from the pilot's proposal): each tranche's
merge keeps the replayed path gate's two measure runs judged on the thresholds and its run under the debug layer (about 15-20
minutes), plus the tranche's own controls on the paths it changes (three sizes); the memory leg and a night endurance only at a
workstream's last merge (before the GitHub copy), at the latest every third tranche, and at once when a tranche touches the
cadence, the display or the memory; live runs grouped (D-0225). Given up: a slow memory growth is caught up to three tranches
later. Replaces D-0218's « memory leg when the branch touches the memory, the loadings or the streaming ».

## D-0227 - Full production: no measurement gate, one start check after each merge (Thomas, 2026-10-28)

Thomas (project chat, 2026-10-28 00:27 Paris: « Sérieusement c'est long, il passe plus de temps à tester qu'à avancer »; « Oui » to
stop the gate's calibration; 00:28: « Bon c'est risqué mais j'assume, je préfère vraiment que ça avance, fini les tests et les
seuils, on passe en full production »; 00:29, « D'accord » to keeping only the check that the game starts after each merge; in the
pilot's thread at 00:30: « J'autorise », « full production »). Rule: no replayed path gate, thresholds, calibration, measure
series, night endurance or memory leg before a merge; the pilot chains tranches and workstreams in the ROADMAP's order and merges
each into `main` once its code is written and reviewed (D-0198 unchanged); after every merge the module is built from `main` (its
unit tests run) and the game is launched once with it to the world (a short announced run, offline, the saves restored by hash,
D-0116/D-0117 unchanged). A measure is taken only when a fix cannot be written without it (said to Thomas) or on his request.
Replaces D-0226, D-0218, D-0207, D-0206, D-0204's night endurance and the gate requirement of D-0202; `Test-Project` no longer
requires a gate record. Risk accepted by Thomas: regressions of cadence, memory or pictures are found later, in play.

## D-0228 - One list to the finish line, without waves; « je prends le PC » (Thomas, 2026-10-28)

Thomas (project chat, 2026-10-28 01:03-01:08 Paris, relayed by the coordinator: « les FFX font partis de ça, il manque plein
d'effets », « ça couvre deux choses, une manière de les couvrir et les effets manquants », « j'ai plus trop envie de découper ça
par vague », « les technologies Nvidia, ce sera en dernier », « ça marche », « faut juste retirer ma session de jeu », « voilà, et
là on ne s'arrête plus »), on the study thread « ligne d'arrivée » (its page copied as [FINISH-LINE](../.project/FINISH-LINE.md)):
the arrival is the whole game and its two DLC natively without a known fault, a missing effect or a known defect the original
lacks; one ordered list of 12 to 18 workstreams (FINISH-LINE, ROADMAP « The finish line »), the pilot chaining them without
waiting; Thomas's own play session is not in it; the zones never seen natively are checked by the pilot (line 8); NVIDIA's
technologies come last, after the arrival's other items. Replaces D-0224's order (and D-0222's, D-0200's). Also (Thomas,
00:58, relayed): on « je prends le PC » the pilot only reads and writes code, no build and no live run, until his « fini »
(WORKFLOW « Thomas plays »).

## D-0229 - No live run before a merge, a one-minute start check after it (Thomas, 2026-10-28; updates D-0227)

Thomas (project chat, 2026-10-28 02:48 Paris, relayed by the coordinator with his message): asked whether I-0101's last live run
(the close-box quit after TryResizeBuffers, a fix never run) could be skipped and the branch merged with only the start check,
« oui on s'en passe et pas de contrôle ». Rule: a fix is merged once written and reviewed (D-0198 unchanged) without a live run to
verify it, and the start check D-0227 put after every merge is dropped; after a merge the launcher's module is still rebuilt from
`main` in the main checkout, its unit tests included. The coordinator read it as « no more live runs by the pilot »; live runs the
finish line itself needs (FINISH-LINE line 8, the zones never seen natively) are asked of Thomas when that line comes. A fault met
later in play is noted and fixed then. Risk accepted by Thomas: a fix that fails at run time is found in play.
Amended the same night (project chat, 03:33, to the coordinator's case for a one-minute check after each merge: « Une minute je
veux bien, pas dix »): after every merge the game is started once with the launcher's rebuilt module, carried to the world and
closed, one minute aimed at and two at most, announced in one line, no measure and no other input
(`scripts/Invoke-StartCheck.ps1`: the Firelink path's save, 60 s from the attach, offline, restored by hash).

## D-0230 - The full control every three days (Thomas, 2026-10-28; completes D-0227)

Thomas (project chat, 2026-10-28 04:25 Paris, to the coordinator's proposal of a full control every three days at the first
clean checkpoint, run while he is away: « Oui. Pendant que je dors ou que je suis au boulot la nuit »). Rule: every three days,
at the first clean checkpoint (a merged workstream, never in mid-workstream), the pilot runs the full control D-0227 dropped:
the replayed path gate (`scripts/Invoke-ReplayGate.ps1`) with its memory leg, the night endurance
(`investigations/I-0085-night-endurance/Run-Endurance.ps1`) and the control sizes (1280x720, 1920x1080, an arbitrary window);
only while Thomas sleeps (the day) or works (the night, about 19:30 to 07:00), on the ASUS alone with the OLED off, announced like
every run, stopped on « je prends le PC » or « je joue ». A regression it finds becomes the next workstream. The first is due at
the first clean checkpoint from 2026-10-31 (three days after D-0227), unless Thomas decides otherwise.

## D-0231 - Haiku 5.5 for the mechanical subagent work only (Thomas, 2026-10-28; completes D-0225; replaced by D-0245)

Thomas (project chat, 2026-10-28 08:26 Paris, to the coordinator's proposal: « Vas y dis lui »). Rule: a subagent may run on
Haiku 5.5 only for mechanical work whose result is checked whole at a glance: reformatting or ordering a file, a table or a list;
counting or sorting log lines already found; extracting numbers from a measurement record; one small change applied at places
already listed. Its prompt stays under 100k tokens (above, Haiku costs five times more). Whatever needs searching or judging stays
with Sonnet: bug hunts, dead code, a log summarised to say what failed, decompilation reading (I-0044). WORKFLOW « Roles and
delegation ».

## D-0232 - The key order of the effects with the forward rows stays on FINISH-LINE line 7 (Thomas, 2026-10-28)

Thomas (project chat, 2026-10-28 21:05 Paris, asked at 19:12 whether the key order should come before line 5: « Qu'il garde
l'ordre qu'il a choisi, ça me dérange pas »). The key order of the effects with the forward rows, the depth-writing effect draws,
the copy before each sampling pass and the bindings resolved once at the bind (I-0104 DESIGN §2, tranche 3) stay on FINISH-LINE
line 7, after the missing effects (line 5, I-0105) and the spot lights and local shadows (line 6). The model particles of I-0105
are drawn among the world's forward rows 0x29 until then.

## D-0233 - Elden Ring's blood and some of its effects, after the finish line (Thomas, 2026-10-29)

Thomas (project chat, 2026-10-29 04:51-04:55 Paris, relayed by the coordinator with his messages: « à terme on mettra le sang d'ER
qui est carrément mieux fait », « Tu peux noter c'est certain », « sauf poison et foudre je dois décider encore [...] idem pour le
sacré »). After the finish line (fidelity to the original first), the remaster replaces DS3's blood by Elden Ring's, and takes ER's
effects for frostbite, bleed, fire and Black Flame; poison and lightning wait for his decision (they differ between the two
games). Holy keeps DS3's look (Thomas, 2026-10-29 05:08: « L'aspect sacré de DS3 est joli, ça passe »); for poison he considers
ER's look recoloured purple (05:07), its feasibility not yet studied. ER's files are read on his PC only, never on GitHub or in the cloud. ROADMAP C3 wave 2, the remaster's additions.

## D-0234 - The stop guard removed (Thomas, 2026-10-29; amends D-0217)

Thomas (2026-10-29, relayed by the coordinator: « Stop allowed sert à rien, donnez moi la commande et je le vire »; 21:26: the hook
removed from the main checkout's settings with Codex, and « Oui » to the cleanup). The Stop hook `scripts/Stop-Guard.ps1` and its
key `scripts/Set-StopAllowed.ps1` are deleted; WORKFLOW « Waiting » replaces the rule. A thread waits on a run or a build with a
harness-tracked background command, so the session is not left idle.

## D-0235 - Full RT/PT replaces « NVIDIA last »; the lighting raster hidden, then removed (Thomas, 2026-10-30; replaces part of D-0228, completes D-0178)

Thomas (2026-10-30 01:21-01:28 Paris: replace the graphics settings by better ones, shadows, lighting and reflections ray- and
path-traced by levels as Cyberpunk, DLSS instead of the anti-aliasing, RTX Mega Geometry, no raster lighting left, « full RT/PT »,
Gears of War: E-Day as the example; then 03:02, relayed by the coordinator, on the study [RT-PT-PLAN](../.project/RT-PT-PLAN.md)
section 13: « 1. d'accord je te suis 2. Je te suis aussi »):
- **Order** (13.1 a): FINISH-LINE line 1 (the faults) and the effects workstream in progress (I-0105) finish first, then the RT
  workstreams R0 to R11 of the plan's section 8 (base: Agility SDK, a 12_2 device with DXR 1.2, dxc, bindless; DLSS by Streamline;
  the scene in rays; materials; lights; traced shadows with Ray Reconstruction; traced reflections; traced GI; full path tracing;
  effects lit by the traced light; the new settings menu, frame generation and the raster's removal; optimisation and the zone
  tour in PT), then T1, the textures (section 11). Replaces D-0228's « NVIDIA's technologies last » and ROADMAP's « the raster
  renderer stays the base reference »; the rest of D-0228 (one list, « je prends le PC ») stands.
- **The existing raster** (13.2, option B): during the transition the raster lighting stays in the code, never in the menu, reachable
  only through the test launcher (as the `test-*` tools), to compare on the same frame and camera; each category is removed when
  Thomas has accepted its traced equivalent in play. After R10 nothing raster remains for the lighting (the G-buffer, the UI, the
  particles and the post-processing stay raster, as in every path-traced game).
- **The finish line** (section 9): lines 1 and 5 kept (line 1 first); line 7 split (material and data defects kept, raster-lighting
  defects abandoned); line 8 split (a quick tour of the unseen zones for the faults now, the visual tour under PT in R11; the
  settings, I-0108, stop after its tranche 2 step 1: the characters missing from the water reflection and the `+0x70` divisor are
  abandoned, the traced reflections replace them).
- **Completes D-0178**: the remastered settings are the plan's section 6 (levels by RT/PT and DLSS).
- **Taken as a follow-up, without an explicit answer** (13.3): the levels of section 6, Streamline in manual mode, Mega Geometry as
  the base with a standard DXR path beside it, the textures after the RT (T1).

## D-0236 - Elden Ring's ray tracing: its place in the engine observed, its quality and workings not copied (Thomas, 2026-10-30; amends D-0195 for the RT/PT plan)

Thomas (2026-10-30 04:20 Paris, relayed): « le RT d'Elden Ring on l'étudie d'accord mais c'est vraiment pas un modèle, il est mal
foutu », then at 04:21: « Enfin, sa qualité et son fonctionnement surtout, sa place dans l'architecture moteur peut être observée ».

- What is not a model: the quality and the workings of Elden Ring's RT (patch 1.09, shadows and AO): its passes, its algorithms,
  its acceleration-structure handling as implemented. What may be observed as a reference: its place in the engine's architecture
  (where the RT work sits in the frame and the frame graph, which engine objects own it, how the scene data reach it).
- The RT/PT workstreams (R0 to R11, [RT-PT-PLAN](../.project/RT-PT-PLAN.md)) take their design from the current practice the plan
  cites (DXR 1.2, inline ray queries, Mega Geometry's clusters and partitioned TLAS, ReSTIR, Ray Reconstruction) and from DS3's own
  data; an ER reading that contradicts it is recorded, not followed.
- D-0195 stays the rule for the rest of the D3D12 engine (device, queues, frame graph, resources, effects).

## D-0237 - The raster lighting removed, not hidden; the traced shadows right after the RT scene (Thomas, 2026-10-30; amends D-0235)

Thomas (2026-10-30 04:54 Paris, relayed): « Je préfère supprimer le raster finalement, on a l'original, ça suffit »; at 04:55
« Ouais je valide » (the removal as the coordinator put it: each category removed once its traced version works and he has
accepted it, nothing kept hidden) and « Oui » (the traced shadows right after R2, proposed in the R0 thread).

- **Option A replaces option B** ([RT-PT-PLAN](../.project/RT-PT-PLAN.md) section 10): no raster lighting is kept behind the test
  launcher. A raster category (the sun cascades, the local shadow atlas, SSAO, the water's mirrored view, the cubemaps, the
  baked ambient, the deferred light accumulation) is removed from the code once its traced replacement works and Thomas has
  accepted it in play; never before (the game is never left without shadows or lighting between two workstreams). The
  comparison reference is the D3D11 original (its captures and the existing comparers), knowing a difference may be intended.
- **The order**: R0, R1, R2, then R5 (the traced shadows: they need the RT scene and the cut-out transparency only), then R3,
  R4, R6 to R11. The shadows come first as hard traced shadows (exact at one ray a pixel, no denoiser); Ray Reconstruction and
  the soft penumbrae join once R3 has identified the gloss channel its guides need. The first traced picture Thomas judges moves
  two workstreams earlier; the shadow cascades and the local atlas are the first raster removed.
- What stays raster is unchanged (section 1): the camera's G-buffer, the UI, the effects and transparents (lit by the traced
  light), the post-processing, the fog and light shafts, the sky and the decals.
- **A traced picture may differ from the original** (Thomas, 04:56: « c'est normal si c'est faux par rapport à l'original au
  début. Le baked est triché, c'est pas réaliste et le RT suivra cela. Faudra ajuster les sources lumineuses, ou en créer de
  nouvelles »): DS3's baked and fill lighting is a cheat the traced light does not reproduce; a difference from the original is
  not a defect by itself. A wrong datum (a light, a material, a missing object) is still a defect; the look is judged by
  Thomas. The plan gains a lighting pass: adjust the map's light sources (intensity, radius, colour, the fill lights that fake
  the bounce) and add new ones where the traced scene needs them, from R4 on, zone by zone with Thomas's eye. To evaluate
  (Thomas, 04:57): the emissive (« glow ») textures cheat too; under RT/PT make them true light emitters (sampled by ReSTIR) or
  readjust them.
- **Applied, the sun cascades** (Thomas, 2026-11-02 about 16:00, relayed: « je suis ta recommandation », after the High Wall
  picture, on the condition that the orientation check of his question on the outlines come first; the check ran at 16:21 and was
  posted to him at 16:29, I-0118): with the rays on, the traced sun shadow is the default and its frames make no
  cascade (casters, atlas, blur taps); the cascades stay only where the rays are off or have failed for the session, until the rays
  are the default.

## D-0238 - RTX Mega Geometry no longer planned by default: R2 in standard DXR, measured first (Thomas, 2026-10-30; amends D-0235's follow-up and the rule of 2026-10-14)

Thomas (2026-10-30 05:25 Paris, project chat): « Du coup je sais pas si Mega Geometry serait pertinent », then « Vas y » (05:25)
on the coordinator's proposal: do not plan Mega Geometry by default, build R2 in standard DXR, measure the building and
rebuilding of the acceleration structures, add it only if that cost weighs. Thomas does not want the geometry changed (relayed by the coordinator: no remake,
no HD models). The study thread found it consistent with DS3's data ([I-0110](../investigations/I-0110-rt-geometry-lights-study/RESULT.md) §4):
the map models are triangle lists with float3 positions, 2,000 to 4,000 placed map pieces a zone, characters measured at 30 000 to
42 000 triangles (about 100 000 estimated with every armour part); a standard BLAS, a refit for the skinned ones and a TLAS rebuilt
each frame are expected to carry it (INFERRED, to be measured in R2).

- **R2 in standard DXR 1.2** (the inbox runtime, I-0109): one BLAS a model mesh (its LOD-0 face set as the native already
  draws it), compacted; a refit of the skinned and deformed ones; one TLAS rebuilt each frame. No Agility SDK preview, no Windows
  developer mode, no NVAPI extension for it.
- **Mega Geometry comes back only on a measure**: the per-frame build and refit time, the BLAS memory, or LOD switches hitching,
  measured in R2 on the witness zones; the acceleration-structure layer keeps one interface so another backend can join it.
- Replaces the rule of 2026-10-14 (« Mega Geometry is not a menu option but the default technical base », ROADMAP) and the plan's
  section 4 « Quand » and section 13.3's « Mega Geometry as the base with a standard DXR path beside it »
  ([RT-PT-PLAN](../.project/RT-PT-PLAN.md)).

## D-0239 - A study thread merges its documentation-only commits into main (Thomas, 2026-10-30; amends WORKFLOW « Roles and delegation »)

Thomas (2026-10-30 06:37 Paris, project chat), on the coordinator's question whether a study thread may merge its documentation-only
commits itself when no pilot is merging: « Oui comme ça c'est permanent »; confirmed in the study thread at 06:40: « Oui ».

- A study thread may merge into `main` the commits of its branch that touch documentation only (no `native/` code, no script a
  build or a run uses, nothing that needs the launcher's rebuild), when no pilot is merging or rebuilding at that moment; it runs
  `Test-Project` first and tells the coordinator.
- Code, scripts the build or the runs use, and anything that needs a rebuild or a start check stay the pilots' to merge (WORKFLOW
  « Merging into main »).

## D-0240 - Reflex stays an option; LTF and TRTF studied, not copied (Thomas, 2026-10-30; frames the unlocked frame rate)

Thomas (2026-10-30 08:21 Paris, in the study thread, answering its proposal to make Reflex's wait the frame's one clock in R1):
« Reflex doit rester une option. Sinon LTF TRTF c'est surtout expérimental, à étudier mais pas à copier tel quel ».

- **Reflex is a player option** (Off, On, On + Boost, RT-PT-PLAN section 6): the frame keeps its own clock when Reflex is off; the
  cadence's frame-start wait (MODERN-CADENCE-STUDY, D-0219, D-0221) is that clock, and Reflex, when on, takes its place.
- **LTF and TRTF** (the timing project, `timing/`, I-0005 to I-0014) are experimental: the unlocked frame rate studies them as
  leads on what in DS3 depends on 60 frames/s, and designs its own method; their code is not copied as it is.
- **The method** (Thomas, 08:26: « Je propose d'abord B avec une étude sur ce qui est impacté »): the game logic really runs
  faster (a variable step, method B), after a static study of everything it affects ([I-0112](../investigations/I-0112-unlocked-framerate-impact/RESULT.md));
  the render interpolation over a logic kept at 60 (method A) is not chosen for now. The cadence: the display orchestrates it (his
  question answered by the modern chain of MODERN-CADENCE-STUDY); Thomas, 08:28: « Je préfère la méthode des jeux modernes,
  justement car Elden Ring est vieillot sur la technique même s'il nous a aidé »: the cadence takes the modern D3D12 loop (the
  swap chain's frame-latency object waited at the frame's start, the display as the clock under V-Sync, the game's own limit under
  VRR; [MODERN-CADENCE-STUDY](../investigations/I-0097-stutter/MODERN-CADENCE-STUDY.md) form B), not Elden Ring's two spinning limiters (its §3).
- **The order** (Thomas, 08:29: « Et je valide ta recommandation pour l'ordre »): R1 (DLSS), then the cadence by the modern loop with
  Reflex plugged into its frame-start wait (Reflex's tranche of R1 comes with or after it, never as a second clock), then the
  unlocked frame rate (method B, from I-0112's study), then R2 and the rest of the RT/PT plan in D-0237's order.
- **The target** (Thomas, 08:38: « l'idéal dans mon cas à moi c'est 120fps oui mais ça doit être basé sur une logique unlimited, comme
  les jeux modernes encore une fois, sinon on refait la même chose que l'original »): the logic is made independent of the frame
  rate, correct at any delta time (« Unlimited » included), as modern games are, not tuned for a few fixed rates; 120 frames/s is
  Thomas's own setting, one point of the range the tests cover (for example 30, 60, 120, 240 and unlimited).

## D-0241 - No driver pipeline library: the precompile list is compiled at every start (the pilot, 2026-10-30; amends I-0072's ER cache)

The native pipeline cache keeps Elden Ring's shape (one cache per device, the precompile list written by the cache, the
precompile on workers at boot) but no longer loads or stores the driver's objects through `ID3D12PipelineLibrary`. Since the
I-0109 ray-tracing self-test runs at start (30 October), the pipelines a start loaded from the library sampled through their
static samplers as point samplers (K-0486: the dark areas' banding with red and blue patches, the glow in blocks, in every
I-0111 and I-0113 run that ran the self-test and took its grading from the library; smooth with the self-test switched off), and I-0100's grey title
also came from a library. Removing the library rather than the self-test's side effect: the cache stops depending on what a
driver object loaded in another process refers to, whatever runs before it (the RT workstreams will add more), and compiling the
list costs nothing that shows: 227 pipelines in 40 to 72 ms at boot against 32 ms loaded (`i113-g4b2`, `i113-g4a3`, `i113-g4a2`),
none on the frame path. Where ER has the defect: ER loads its pipelines from its library (ER-PIPELINE-STUDY §6); whether it shows
the same sampling is not known. The list's format is 3 (`native-pipelines.v3.psc`, no library stamp nor environment: its records
name themselves). Technical choice inside the pilot's remit (backend mechanics, WORKFLOW « Architectural frame challenge »);
Thomas informed.

## D-0242 - The modern cadence and Reflex inside the unlocked frame rate (Thomas, 2026-10-30; amends D-0240's order)

The RT pilot measured the modern loop's frame-start wait (I-0111 RESULT « Tranche 2 » on the RT branch `claude/exciting-visvesvaraya-0537da`, `i111-t2-dc-*`): while DS3's limiter holds
the logic at 60 the engine is never ahead of the display, so the swap chain's frame-latency object never blocks and the display
cannot take the clock (no gain on held frames, 2.5 to 15 ms more latency to display); with the logic's requested rate above the
display's (I-0114, `i114-obs-n120`, `i114-iso-a1`) the same wait paces 92-98 % of the frames on the 60 Hz ASUS. Proposed to
Thomas (2026-10-30 20:42, thread « Finir DLSS, la cadence puis R2 »): the cadence and Reflex done within the unlock workstream
rather than as a workstream before it, `main`'s 1/60 s pacer kept until then. Thomas (23:39, relayed): « Ok go ».

- The order of D-0240 becomes: R1 (DLSS), then the unlocked frame rate with the modern cadence and Reflex (I-0114), then R2.
- The display clock (`display-clock.flag`) stays behind its flag until the unlock offers a rate above the display's.


## D-0243 - The follow camera by method B too: its laws rewritten, the fixed step only a reference (Thomas, 2026-10-31; keeps D-0240)

The RT pilot traced the 240 sprint's drift to the follow camera's own per-call laws (`140515790`, I-0114 RESULT series 8-15) and
brought the path back to 60's by running that camera's logic at a fixed 1/60 with its view interpolated between ticks (series
16-19): method A for one system, which D-0240's method B excludes, with up to 16.7 ms more camera latency. Asked (2026-10-31
12:40, thread « Finir DLSS, la cadence puis R2 »), the pilot recommending method A for the camera; Thomas (12:44): « Méthode B ».

- The follow camera's laws are corrected one by one with k (the per-frame weights and the dead band judged over 1/60, the Euler
  smoothings, CAMERA-FOLLOW-SITES §9-§13), as the locomotion's.
- The fixed step (`camera_step`) stays behind `camera-fixed-step.flag` as the reference the laws are measured against, never a
  default.

## D-0244 - The held moves' sooner start kept above 60, no faster move, one logic at every rate (Thomas, 2026-11-01; frames I-0114)

The pilot found that above 60 a held move starts about three logic steps after its key, so 25-33 ms sooner than at 60, and lasts as
much longer (I-0114 RESULT, the per-step rows of series 29-32): a walk +1 %, a lock-on strafe +1-2 %. Asked whether to give a held
move 60's duration (60's 50 ms at the start at every rate) or keep the sooner start, Thomas (07:06): « Le jeu est plus réactif à
haut frame rate c'est ça? Oui c'est le but »; (07:13) « A alors. Mais faut pas que ça fasse tricher ni décale le personnage sur une
même distance parcourue »; (07:16, relayed) the same logic for every frame rate, under 30 too.

- Option A: a held move's sooner start above 60 is kept; its extra distance on a held move (+1-3 %) is accepted.
- No cheating: every speed and every discrete move (a roll, a jump, a backstep) covers 60's distance; the strafe's speed is 60's
  (series 35), so no move is faster over the same time.
- One logic for every rate: each correction is a law of the frame's time (k = dt·60, the original's bits at 60), checked under 60
  too (30 and 20); where no law reproduces 60 (the jump's held speed under 60, I-0114 tranche 7), the original's is kept and the
  gap reported.

## D-0245 - Opus the default pilot, pilots never yield, mechanical work always to Sonnet (Thomas, 2026-11-01; replaces D-0231)

Thomas (thread « Étudier en appui des pilotes », 2026-11-01 21:25 Paris, on the study thread's summary of WORKFLOW and CLAUDE.md):
« Enlever seul responsable technique et dire que c'est le pilote par défaut »; the « do not yield » rule moves to CLAUDE.md without
« until the workstream meets its exit criteria »: « on s'arrête jamais dans le cas des pilotes. L'étude n'est pas concernée »; and
« on délègue toujours les tâches mécaniques, rapides, qui demandent peu de réflexion complexe, à des sous agents Sonnet 5.5 (niveau
d'effort au choix) ».

- Opus 5.5 at high effort is the default pilot (no longer « the sole technical owner »); the pilot owns its workstream (D-0114).
- A pilot never yields its turn, except for a necessary human intervention, a decision that is Thomas's, a destructive or protected
  action, a real technical inability, or an explicit `PAUSE` (CLAUDE.md). The study thread is not bound by this rule.
- Mechanical, quick, low-reasoning tasks are always delegated to Sonnet 5.5 subagents, at the effort the thread chooses; this
  replaces D-0231's Haiku 5.5 for the mechanical work. WORKFLOW « Roles and delegation ».

## D-0246 - Private notes between threads and the coordinator are free (Thomas, 2026-11-02; replaces part of D-0224)

Thomas (project chat, 2026-11-02 09:04, « Après tout des collègues ça communique entre eux », with his follow-ups as the
coordinator relayed them: the notes may question choices and reasoning, « ça évite l'effet tunnel d'un seul pilote », any
subject, Thomas included); the wording validated by Thomas in the open-gaps pilot's thread (10:53, « Ok »).

- Private notes between the threads and the coordinator are free, on any subject, Thomas included, without telling him; they
  serve to challenge the others' choices and reasoning, against a single pilot's tunnel.
- Anything that needs Thomas's decision, and every opinion or proposal that concerns him, goes to him directly in the thread
  (D-0224 kept for that part; its « private notes carry only test times and PC libre » replaced). WORKFLOW « Communication with
  Thomas ».

## D-0247 - The local shadow atlas removed on traced frames; the fill lights and the baked ambient kept until R7; Ray Reconstruction a player option (Thomas, 2026-11-03; D-0237)

Thomas (the R pilot's thread, 2026-11-03 18:26-18:28, Paris), on the R4b picture (I-0122) and R4-PLAN §5: « Le premier
je dis oui »; « on garde alors le deuxième, on enlèvera à R7 »; and on Ray Reconstruction (relayed from the project chat, 18:28 Paris):
« ça doit rester une option en jeu comme les autres jeux ».

- The local shadow atlas, its faces' draws and its casters' walk are no longer made on frames whose local shadows are traced
  (I-0122 R4b's second half); the shadowed lights keep their records for the rays.
- The cold fill lights (`Omni*`) and the baked ambient (GB4) stay as the original draws them until R7 (the traced GI): the baked
  ambient goes then, the fill lights are tagged for Thomas to dim or remove zone by zone.
- Ray Reconstruction (I-0123) is a player option, On by default in ray tracing, Off allowed (RT-PT-PLAN §6); until the options menu
  (R10), `rr-mode.txt` beside the module (« off ») and `no-rr.flag` for the test runs.

## D-0248 - At 50 % of the weekly limit the pilots tell the coordinator, the open-gaps pilot stops (Thomas, 2026-11-05)

Thomas (project chat, 2026-11-05 03:20 Paris): « Vu que les pilotes peuvent voir ma limite hebdo, je propose qu'ils t'envoient un
message à 50% et que tu coupes le pilote écarts après un checkpoint propre. À inscrire dans Claude.md »; the RT pilot continues
(the coordinator's brief).

- At 50 % of the weekly usage limit, every pilot sends the coordinator a private note (D-0246).
- The coordinator then stops the open-gaps pilot after its next clean checkpoint (STATE up to date, coherent commit); the RT
  pilot continues alone, as on 2026-11-02 at 75 % (STATE « Second pilot »).
- The 90 % slow-down and the 99 % pause are unchanged and apply to the pilot that continues. CLAUDE.md, WORKFLOW « Threads and
  continuity ».

## D-0249 - The traced lighting is judged by its truth, not by the original's pixels (Thomas, 2026-11-06; completes D-0235)

Thomas (project chat, 2026-11-06 10:19-10:21 Paris, relayed by the coordinator): « après si c'est pas exactement pareil c'est pas
grave, tant que le RT donne un éclairage naturel et vrai. Sauf si bug et là c'est une anomalie », then « Et ça vaut pour tout
d'ailleurs, pas juste cette scène isolée ».

- In RT/PT (the R workstreams of D-0235), the original's picture is not a pixel target: what is accepted is a natural and true
  lighting, everywhere in the game. A difference from the original is not a defect in itself; a bug (a fault, a wrong or missing
  input, light where there should be none, a broken material) stays an anomaly to fix.
- The rule is general (every scene, every zone), not one scene's exception. It changes nothing for the inputs the rays take from
  the game (materials, lights, geometry: read from the original as before) nor for the raster the plan keeps (the G-buffer, the
  effects, the UI, the post-processing), which stays faithful to the original. There is no full raster mode for the players
  (D-0237: the raster lighting is removed), and correcting raster lighting the RT/PT replaces is useless (Thomas, 10:22-10:24:
  « Enfin si E doit corriger du raster qui est couvert par le RT/PT ça sert à rien », then « Oui je valide »; this replaces a first
  wording that kept a raster mode without RT identical to the original, from a question that assumed such a mode). WORKFLOW
  « Method rules ».

## D-0250 - The open-gaps pilot corrects only what stays visible with the rays (Thomas, 2026-11-06; completes D-0249 for E)

Thomas (project chat, 2026-11-06 10:22 Paris): « Enfin si E doit corriger du raster qui est couvert par le RT/PT ça sert à
rien »; on the pilot's sort relayed by the coordinator, 10:24: « Oui je valide ».

- There is no full raster mode for the players (D-0237, D-0249); the open-gaps pilot (E) fixes only the gaps that stay visible with the traced lighting: what RT-PT-PLAN §1 keeps in raster
  (the G-buffer, the effects and particles, the UI, the post-processing). A gap whose only effect is raster lighting, shadows or
  reflections is dropped, since D-0237 removes the raster lighting for the players.
- I-0113's order after tranche 17: gap 16 (Fire Orb's explosion ring), then gap 21 (the hair fading in a fast travel); the water
  interaction (STUDY-NOT-YET-NATIVE row 9) a small check of pass 0x23; the Crystal Magic Weapon crystals dropped too: R's answer (2026-11-06):
  RT-PT-PLAN R9 lights the forward rows and model particles from the rays, so their raster inputs (t30/t31, t44, the light grid)
  go; only a base material or colour defect would stay E's.
- Dropped from E: category 0x21's sub-4 depth pre-pass (its only effect the screen-space AO and shadows around a character
  with hair). To R: `m37_00_00_00_001005` casting at every distance (R5c's rule) and the Dreg Heap's pink family (the rays'
  materials, the sky dome).

## D-0251 - A raster path with a traced equivalent is removed (Thomas, 2026-11-06; completes D-0235 and D-0237)

Thomas (project chat, 2026-11-06 16:57 Paris, relayed by the coordinator), to the question whether the water's mirror view goes
now that R6c traces the water's reflection: « Oui du coup on supprime le raster s'il y a un équivalent tracé ».

- The water's mirror view (the reflection walk and its view, `no-rt-water.flag`) is removed: the traced water (I-0125 R6c, R6d)
  is accepted.
- General rule: wherever the rays give a traced equivalent of a raster path, that raster path is removed from the code, not kept
  behind a flag or as an option, without a separate acceptance each time (D-0235's « once Thomas accepts the traced equivalent »
  given once for all; the pilot tells Thomas what it removes). The raster RT-PT-PLAN §1 keeps (the G-buffer, the effects, the UI,
  the post-processing) is not concerned; a frame the rays cannot serve yet (no top level, a refused camera) is handled by the
  workstream's design, said in its plan, not by keeping a raster mode (D-0249: there is no full raster mode).

## D-0252 - The traced GI on every traced frame: Ray Reconstruction the optimum, NVIDIA's NRD without it; SHaRC the cache (Thomas, 2026-11-06; R7)

Thomas (thread, 2026-11-06 17:18 to 17:27 Paris, to the R pilot's questions of R7-PLAN §5): « Prends SHARC et le dépôt est privé
pour l'instant mais sera public plus tard »; « la GI doit être présente même quand l'option RR est sur off. Et une fois la GI en
place, son équivalent raster est supprimé »; « Va pour NRD comme solution de base, et le RR comme solution optimale ».

- The traced GI (I-0126, RT-PT-PLAN R7) runs on every traced frame: denoised by Ray Reconstruction when the player's RR option is
  on (the optimum), by NVIDIA's NRD (ReBLUR or ReLAX) when it is off; then the baked ambient and the SSAO are removed (D-0251).
- The radiance cache is NVIDIA's SHaRC (downloaded 2026-11-06). NVIDIA's SDKs (Streamline, SHaRC, NRD) stay outside the repository
  and its GitHub copy, which will be public later (their licence forbids an open-source licence); the build reads them from
  `Tools/DS3-Decompilation/third-party/`, and NVIDIA's notice goes with the distributed module.
- I-0126 R7d (Thomas, 2026-11-07 09:59 Paris, « Choix A »): NVIDIA's RTXDI library (`NVIDIA-RTX/RTXDI-Library` `f12037fa`, the
  same licence) under the same rule, its GitHub archive hash-pinned and extracted by the build; ReSTIR GI on its reservoirs, opt-in
  (`rt-restir.flag`) and left to R8 (I-0126 RESULT « R7d »).

## D-0253 - The rays are the module's default (Thomas, 2026-11-07; completes D-0249 and D-0251)

Thomas (thread, 2026-11-07 00:48 Paris), to R6f's final review's H1 (the rays ran only with `rt-scene.flag`, which no launcher
places, so removing the water's mirror view would have left the play sessions without water reflections): « Et je choisis A ».

- The rays' scene and everything on with it (the traced sun shadow, the local lights, the reflections and the water, Ray
  Reconstruction where DLSS runs; I-0126's GI once merged) run by default, the play sessions included; `no-rt-scene.flag` cuts
  them (a test tool: the raster's remaining paths). A device without DXR 1.1 or a failed self-test still runs without the rays. (Superseded by D-0262 and R11e: a clear message and a clean close; D-0273.)
- Consequence: the start check exercises the traced paths; the replayed path gate's thresholds were calibrated without the rays
  (a gate run with them is not comparable until Thomas decides a new calibration or keeps the gate on `no-rt-scene`).

## D-0254 - The light sets' fill lights go on the traced GI's frames (Thomas, 2026-11-07; D-0251 applied to R7)

Thomas (thread, 2026-11-07 01:05 Paris), to the R pilot's proposal with R7a's diagnostic pictures (I-0126 RESULT « R7a »): « Je
suis ta recommandation ».

- On the frames the traced GI serves, DS3's light sets' lights 1 to 3 (unshadowed directional fill lights that fake the bounce;
  their colours zeroed in the deferred passes' cbLight, for the pixel and the rays' hits) are removed with the baked ambient and the
  SSAO: the traced bounce replaces them (D-0251). Lit fully in the hollows once the SSAO went, they flattened the picture. The
  forward rows (hair, translucent pieces, water, lit effect models) keep theirs, with their baked ambient, until R9 (R7a's final
  review M2).
- D-0247's « fill lights tagged for Thomas zone by zone » stays for the maps' own point lights (Omni*), not these directional ones.

## D-0255 - No GPU-driven raster workstream; a change-driven collect and rays' walk instead (Thomas, 2026-11-07)

Thomas (study thread, 2026-11-07 18:00 Paris), to S's recommendation in [STUDY-GPU-DRIVEN](../investigations/I-0127-effect-batching/STUDY-GPU-DRIVEN.md):
« Bon je suis ta recommandation ».

- No GPU-driven raster workstream (GPU culling and `ExecuteIndirect` for the world's geometry, mesh shaders): measured with the rays on,
  the raster's whole GPU share is 1.5 to 2 ms of a 17 ms frame at 1920x1080 (the rays and DLSS about 15 ms), and the render worker is
  not the limit at 1280x720; mesh shaders or a Nanite-like format stay with the HD assets after the finish line.
- The CPU lever kept: a persistent, change-driven collect and rays' walk (the native reads only what the engine changed instead of the
  whole scene each frame; m45: the collect 6.0 ms with the rays' walk 2.0 ms on the engine thread that limits the frame at 720p). Its
  place in the order and its pilot (E or R) are the coordinator's and Thomas's; the GPU lever at 1080p and above stays the rays' own
  cost (R).

## D-0256 - The PT level traces two path vertices by default, set in a file beside the module (Thomas, 2026-11-07; I-0128 R8b)

Thomas (thread, 2026-11-07 18:02 Paris), to the R pilot's question with R8's measures (I-0128 RESULT « R8a and R8b »: one vertex 3
to 5 % under the 8-vertex reference, two within 0.1 %, about +14 ms at 1920x1080 DLAA) and what other games do (Cyberpunk 2077's
Overdrive two bounces with SHaRC, Alan Wake 2's one or three): « Ok on met 2 par défaut et une option réglable par .ini (si
possible), on fait comme Cyberpunk en gros ».

- The PT level's paths: two traced vertices, then SHaRC's cache (R8b's default, merged `e53f8187`).
- The setting: `rt-bounces.txt` beside the module (one digit, 1 to 4), the same form as `rt-level.txt`, `rr-mode.txt` and
  `dlss-mode.txt`; the remaster's settings in one file or menu are R10's (RT-PT-PLAN §6).

## D-0257 - The light sets' fill lights kept on the traced GI's frames, for now (Thomas, 2026-11-07; amends D-0254)

Thomas (thread, 2026-11-07 17:51 and 18:04 Paris): « L'éclairage est pas terrible dans les zones sombres », « Si c'est à cause des
fill lights, vaut mieux les remettre pour l'instant ». Checked (I-0128 RESULT « The fill lights »): in Firelink's unlit lower corridor
and Anor Londo's hall, without them the character and the near walls take only the candles' warm light; with them the original's
cool fill comes back (the three side by side shown to Thomas, `pictures-fill1`).

- On the GI frames the sets' lights 1 to 3 (DS3's unshadowed directional fill lights) are kept, as the pixel's passes and the rays'
  hits read them; `no-rt-gi-fill.flag` (a test tool) removes them as D-0254 did.
- Known: they light the hollows unshadowed and add to the traced bounce (D-0254's reasons stand); their replacement by traced sources
  (shadowed, or the maps' sources adjusted: RT-PT-PLAN §8 « Chantier d'éclairage ») is left for later.

## D-0258 - The RT/PT plan's menu work leaves R10 for a new R12, with the title screen (Thomas, 2026-11-08)

Thomas (project chat, 2026-11-08 06:13 and 06:14 Paris), after reading RT-PT-PLAN: « mettre le chantier du nouvel écran titre sur
R10 », « faire passer R11 avant R10 », then « tout ce qui touche au menu dans R10 on l'enlève et on le met dans R12 ; le nouvel écran
titre ira donc dans R12 ».

- R10 keeps the frame generation and the raster lighting's removal; R12 (new) holds the settings menu of RT-PT-PLAN §6 and the
  « Rekindled » title screen of D-0182 (Thomas's mock-up, `.project/design/title-screen-mockup.png`).
- The order R9, R10, R11, R12 (Thomas, 06:16, « Je suis ta recommandation »): the raster lighting removed (R10) before the
  optimisation and the zones' tour (R11), the menu and the title screen last.

## D-0259 - The fill lights' traced shadow on by default with DS3's AO weights; every fill light shadowed retried in R11 (Thomas, 2026-11-08; I-0129 R9d)

Thomas (project chat, 2026-11-08 17:51 Paris, relayed by the coordinator), to R9d's sheet and its three options (off, DS3's AO weights,
every fill light): « Pour l'instant l'option 2, à terme quand on modifiera les sources lumineuses ben on on passera au 3, du moins on refera
l'essai ».

- The fill lights' traced shadow (I-0129 R9d) runs by default on the GI frames, each light set's own FC_SsaoWeight deciding which
  fill lights it shadows (`no-rt-fill-shadow.flag` a test tool); merged with I-0131.
- Every fill light shadowed (`rt-fill-shadow-all.flag`) stays a trial, tried again when the maps' light sources are reworked
  (RT-PT-PLAN §8 R11, « Chantier d'éclairage »).

## D-0260 - The disk guard: builds without objects, a rotation at each merge, save backups to D: (Thomas, 2026-11-08)

C: filled up (25 GB free, runs refused): the evidence store had grown 415 GB in two weeks (500 GB, 287 GB of it build folders, 200 GB
of `.obj`; 122 GB of raw frames) and `~/.ds3-i0039-backups` held 71 GB of per-run save backups, nothing rotating them. Thomas ran
the cleanup himself (tiers A, A2, B, C and D, 394 GB, and the run save backups older than 7 days, 55.5 GB, by his own command with no
per-file audit; EVIDENCE-RETENTION `disk_cleanup_20261108`), then, to the four guards proposed in the diagnosis thread: « oui »
(2026-11-08 23:37 Paris). A full verification build of this branch passed every control; its controller equals the launcher's
(SHA-256 0788B432…4598) and its module differs only by the embedded source and PDB paths (worktree against main checkout); its
folder held 0.25 GB without objects, the NVIDIA DLLs hard links (then deleted).

- `Build-GameBootstrap.ps1`: a successful build into its own `-Out` folder deletes its compiler objects (`-KeepObjects` keeps them
  for a later `-ChangedUnits` build; the launcher's default folder keeps them); NVIDIA's Streamline DLLs are hard links to the SDK's
  files on the same volume; no build under 10 GB free on the output's drive. A build folder drops from ~0.71 GB to ~0.13 GB.
- `scripts/Clear-DiskCache.ps1 -Apply` at each merge (skill merge-workstream) and whenever C: has under 100 GB free (Session-Start
  reports it): build folders (`ds3-build.marker`, or an older Build-GameBootstrap `build.cmd` without reports) older than 48 h, a
  run's `last-used.txt` counting, but the launcher's; runs' raw frames older than 7 days but the references'; never a folder the
  repository links into, holding `keep.txt` or listed in `_maintenance/rotation-keep.txt`; audited in `_maintenance/cleanup-rotation.jsonl`. Anything else in the evidence store stays Thomas's call.
- `Run-StartupTrace.ps1` moves the run save backups beyond the newest 100 to `D:/DS3-save-backups-archive` (`scripts/Move-SaveBackups.ps1`:
  copied, hash-checked, never deleted; at most 20 a run); the witness and named saves never move.

## D-0261 - No GitHub copy and no Codex command any more (Thomas, 2026-11-08)

Thomas (diagnosis thread, 2026-11-08 23:51 Paris): « supprime les commandes pour codex, il n'y a plus besoin ». The skill
`codex-github-copy` is removed, and with it the merge's step that posted its command (WORKFLOW, skill merge-workstream, CLAUDE.md,
AGENTS.md). The programme cache under `C:/Users/User/.codex/I0063-cache` stays: the build's controls read it.
Then (23:55): « supprime AGENTS.md, uniquement Claude travaille dessus désormais »: AGENTS.md removed, CLAUDE.md the only entry point.

## D-0262 - The raster's lighting cheat is not copied any more: the fill lights and the baked ambient go once the sources are reworked (Thomas, 2026-11-09; amends D-0257)

Thomas (thread R11, 2026-11-09 03:01 to 03:06 Paris), after R11a's pictures: « faudra retravailler les sources, car là franchement je
ne vois pas trop ce qu'apporte le RT/PT par rapport au raster », then « Du coup on n'a plus besoin de recopier la triche du raster ».

- DS3's fill lights (each light set's lights 1 to 3) and its baked ambient, kept on the GI frames by D-0257, are meant to go: the
  traced bounce replaces them. Not before the real sources (local lights, emissive surfaces, the sky) are reworked so the interiors
  do not go black (R7a's D-0254 removal): I-0132 R11d's sheets are made without the fill lights, the variants being the sources'
  settings, the raster's picture beside them; Thomas picks, and the choice replaces the cheat everywhere (by family, D-0150).
- R11d comes before the zones' tour (I-0132 R11-PLAN §7).
- Then (03:10 to 03:11): « En fait tout se passait bien, c'est à cause moi qui ai voulu remettre des fill lights et de la triche. On
  aurait dû rien toucher et simplement ajuster l'intensité » and, to the pilot's proposal to remove the fill lights and R9d's fill
  shadow: « Oui retire les, c'est de la raster, ça doit être retiré ». So the fill lights are removed from the traced frames (the
  pixel's passes and the hits), their code too (D-0237's option A), with R9d's fill shadow pass (D-0259 lapses); the sources'
  intensity is set from R11d's sheets. The light painted in the textures is left to T1 (Thomas, 03:09).
- The rule (03:12): « Et on ne simule plus la triche ni n'en créé, on travaille avec du réel, on ajuste simplement les sources »:
  no lighting cheat is simulated or created any more (no fill light, no baked ambient or cube standing in for a traced answer, no
  added fake light); the traced frames' light comes from the real sources (the sun and sky, the local lights, the emissive
  surfaces), whose settings alone are adjusted (WORKFLOW « Method rules »).
- The frames the rays do not serve (04:38 to 04:45): magenta in the test runs (`rt-fail-magenta.flag`, « Oui en magenta, comme ça
  on voit mieux »); then no fallback at all (« comme ça on n'a plus besoin de repli »): the arrival frames held until the rays'
  scene is built (a time limit), a skipped walk traced with the previous frame's top level, and a latched failure or an adapter
  without DXR a clear error message and a clean close (« Ok pour le message d'erreur, en espérant qu'il n'arrive jamais, c'est le
  but »); DS3's fallback lighting is then removed (I-0132 R11-PLAN §5 item 4).
- The sources (05:11, on R11d's sheets): « Va pour x2 » (the local lights' colours x2 the default), and « ajuster en fonction d'IRL
  quoi, reproduire le même comportement »: next, the local lights in physical terms by family (realistic intensities, the
  inverse-square falloff) with an automatic exposure (the eye's adaptation), sheets to Thomas at each step.
- YEBIS (06:25, asked whether parts of it stay): « Non c'est bon, on change rien, on supprime totalement YEBIS pour une technique plus
  moderne »: RT-PT-PLAN §8 as it stands (YEBIS entirely replaced by a modern post-processing chain after the PT); the automatic
  exposure's targets by area are measured on the original's pictures, not taken from YEBIS.

## D-0263 - The automatic exposure has one target for the whole game, not one by area (the coordinator on Thomas's universal rule, 2026-11-09; amends D-0262's last line)

D-0262's last line (« the automatic exposure's targets by area are measured on the original's pictures ») wrote the R pilot's proposal
of 06:22 (I-0132 R11-PLAN §5), which Thomas had not confirmed as such. The coordinator (2026-11-09 22:28 Paris) recalled Thomas's
standing rule « la règle universelle, pas de cas par cas » (decisions above: one universal rule, never a case by case) and that a
physical automatic exposure uses one curve. So the automatic exposure (I-0132 R11h) has one target for the whole game: the original's
mean displayed level, the trimmed mean of the luma of its pictures averaged over the witness views (0.20, R11h's first slot; 0.09 to
0.37 by view), and one universal adaptation law (Unreal's); its strength (full or half: dark views darker) and its default are put to
Thomas on R11h's sheets (thread R11h, 2026-11-09 23:15). Until he answers, everything stays opt-in (`rt-auto-exposure.flag`,
`rt-auto-exposure-half.flag`).

## D-0264 - The automatic exposure on by default at half strength; the local lights kept at DS3's x2, the physical terms a test tool (Thomas, 2026-11-09)

Thomas (thread R11h, 2026-11-09 23:55 and 23:56 Paris), on R11h's sheets and the pilot's three questions: « Je suis ta recommandation
pour 1 et 2 » (the automatic exposure the default; with a universal compensation so the dark views stay darker: its half strength,
the goal half the solved log2 factor), and to the pilot's proposal for the third (« Pour le 3 tu proposes quoi? »: one ratio for every
local light, 1/50 of DS3's x1 against the sun, inside the real range of both torches, about 1/40 to 1/1,500, and fires, about 1/7 to
1/300, connaissance générale in I-0132 LIGHT-UNITS.md): « Oui, va pour ça ». So by default: the automatic exposure at half strength with
the game's one target (D-0263); the local lights' inverse-square falloff windowed at a reach from their brightness, their intensity
1/50 of DS3's x1 (replacing R11d's x2 on the rays' list). Test tools: no-auto-exposure.flag, rt-auto-exposure-full.flag,
no-rt-local-physical.flag (DS3's falloff, x2), rt-local-physical-10.flag, rt-local-physical-200.flag. The nights and the closed interiors
(without daylight) are judged at the zones' tour (I-0132 R11c), the rule adjusted there if needed.

Then (23:56 to 23:59), on a second look: « Par contre je trouve les images moches sur les planches au dessus en toute honnêteté ». The
pilot's reading, on the corridor of Firelink at full size against the original: with the local lights 50 times weaker the interior is
dark, the automatic exposure lifts it by about 1.5 stops, and DS3's fog and the daylight seen through the arches (set for DS3's own
light balance) come up with it: a milky grey veil, the deep blacks gone; the walls and the player flat, no warm light; the flames blown
into white halos. DS3 was drawn for its own light ratios. Its recommendation, the local lights back at DS3's x2 with the automatic
exposure at half strength, the physical terms a test tool until the fog and the modern post chain (after YEBIS) are handled: « Fais ça
oui ». So the local lights stay at DS3's x2 by default (R11d), rt-local-physical-10/-50/-200.flag the test tools.

## D-0265 - The remaster's defaults after R11h's opt-in list: the Agility SDK on, ReSTIR GI with the PT, DLSS off once a TAA exists (Thomas, 2026-11-10)

Thomas (project chat, 2026-11-10 00:20 to 00:55 Paris, relayed by the coordinator), on the list of opt-in features the R pilot sent at
the end of I-0132 R11h:

- **The Agility SDK** (00:20): the device on its runtime (1.620.1, the module's D3D12 folder) by default once the pipeline states its
  debug layer refuses are cleaned (I-0132 R11i: one rule where the pipelines are made, an unused depth-stencil format dropped);
  `no-agility.flag` the test tool (Windows' inbox runtime); WARP and PIX keep the inbox runtime.
- **DLSS** (00:44, then 00:55 to the pilot's question): off by default, « on peut rajouter un TAA comme anti-aliasing par défaut, même
  si encore une fois l'option optimale reste le DLSS. C'est comme ça dans les autres jeux »: a temporal anti-aliasing the default
  without DLSS, the DLSS modes (NVIDIA's DLAA included) offered in the menu (R12), DLSS the optimum. DS3 has no temporal AA (its
  filter's four modes are spatial: FXAA, FXAA3, FXAA3HQ and DLAA.ppo, AA-NATIVE-READY), so the switch waits for the TAA (the
  coordinator, 00:56: the picture is not degraded between two workstreams): until then NVIDIA's DLAA stays the default. Without DLSS,
  NRD denoises (D-0252) and the shadows are hard (the sun's cone and the lights' spheres are RR's); the PT level runs only on the frames
  RR denoises (I-0128 R8d: NRD's signal has no path), so the PT and ReSTIR GI need a DLSS mode (the PT greyed without one). The PT under
  NRD (paths in NRD's signal and the direct light denoised) is not planned (the pilot's option C, not chosen; reopened on Thomas's
  request). Then (00:57 to 00:58): the defaults the TAA on (an On/Off option) and the RT level; the PT offered once a DLSS mode is on;
  the menu hides no option: those not available are greyed and locked. Frame generation: Streamline's DLSS-G does not need DLSS's
  upscaling, but the native's runs only on DLSS frames today (its depth and motion tagged by DLSS's path, I-0131 R10b) and forces
  Reflex On while it is loaded; Thomas (01:05): « Comme le RR, laisse le FG dépendre du DLSS »: frame generation offered only while a
  DLSS mode runs, as RR (greyed and locked otherwise).
- **Ray Reconstruction** (00:50): « Et le RR aura son option une fois le DLSS actif oui »: its own option, offered only while a DLSS mode
  runs; on by default then, as today (`rr-mode.txt`; the coordinator's reading).
- **ReSTIR GI** (00:49): « Je veux ReSTIR GI, car il y a le RR comme option optimale pour débruiter », then always on with the PT, RR or
  not: on by default at the PT level (I-0132 R11i), `no-rt-restir.flag` the test tool; unbiased over the picture (I-0128 RESULT « R8d »),
  as steady as the one ray at Firelink and a little less at the High Wall, 5 to 6.5 ms more at 1920x1080 (I-0132 RESULT « R11i »); said
  to Thomas, kept (02:12: « Non c'est bon »). The traced GI itself already runs on every traced frame
  (D-0252); ReSTIR GI is its samples' reuse.
- **Reflex** (item 5): Reflex and its Boost mode options, off by default (already so: `reflex-mode.txt`); then (00:59): « Reflex est
  obligatoire avec le FG, le mode boost reste une option dans tous les cas »: while frame generation runs the Reflex setting's Off is greyed
  and locked, the choice left between On and On + Boost (01:00, « C'est plutôt off qui devient indisponible »; the module already turns
  Off into On then, FollowNativeReflex, and keeps Boost).
- Validated as recommended (00:44 and 00:45): the PT offered in the menu after R11b; frame generation off by default, then in the menu
  (R12); 60 frames a second by default; the tessellation and the UI cache after a witness run each (the UI cache is E's); every other
  point of the list as recommended. The menu's entries (the DLSS modes, RR, Reflex and Boost, the PT, frame generation) go to R12
  (D-0258).

## D-0266 - R12's settings: a « Rekindled » page of our own, its entries and defaults (Thomas, 2026-11-10)

Thomas (thread « R pilot » and the study thread, 2026-11-10 04:21 to 05:11 Paris), to the pilots' questions:
- **The page** (04:37, « Ouais vaut mieux la seconde »): the remaster's settings on a page of our own, drawn by the native renderer in
  DS3's art direction (its fonts, frames and sounds), reachable both from the title screen and in play (the in-game Options), the
  same page and the same values in both (04:37, « À l'écran titre mais aussi en jeu, faut que ça corresponde »), rather than rows grafted
  into the game's Scaleform Options page (`02_040_OptionSetting`, `win_02_042_pc_graphicsetting`, whose page code is encrypted:
  MENUS-MAP §77). A short static study of the Options page comes before R12's code.
- **Thomas's choices after the study** (05:09 to 05:11; the study thread's
  [STUDY-R12-SETTINGS-PAGE](../investigations/I-0132-rt-optimisation-zones/STUDY-R12-SETTINGS-PAGE.md) §6): our page replaces the
  game's « Advanced Settings », at the title as in play (`02_042_PC_GraphicSetting` no longer shown), and recreates every row of it
  (« tout recréer pour avoir toute la main dessus »): the five DS3 rows still in use (texture quality, depth of field, motion blur,
  effects, water surface) written by our code to the game's graphics config, the six the rays replace gone (their replacements our RT
  categories). The game's Graphics page keeps Screen Mode, Resolution, Auto-detect and Quality Setting, with Borderless added in its
  Screen Mode row beside Full screen and Windowed, and V-sync and the frame rate added on that page (« Borderless doit être à côté des
  deux autres », « Vsync et le framerate aussi »): that page's rows are plain C++ (the study's §6.1), so no Scaleform movie is edited.
- **Its entries**, besides RT-PT-PLAN §6's: the shader precompile screen with its progress bar (04:22, the ROADMAP's open point of
  2026-10-20, our native shaders included); a sharpening bar (04:28; the pass written in I-0132 R11j, off by default, 04:30); DS3's
  chromatic aberration (its YEBIS tone map's lateral dispersion) and vignette (04:30); V-sync (04:30) and Borderless (04:35, native since
  I-0076), with the frame rate, on the game's Graphics page (above). RT-PT-PLAN §8's R12 row lists them.

## D-0267 - After R12, an installer: the renderer and our settings page only, over the player's own game (Thomas, 2026-11-10)

Thomas (2026-11-10 05:20 to 05:30 Paris, relayed by the coordinator with his messages « Oui. Et oui pas les textures dedans »,
« Oui », « D'accord »): after R12 the ROADMAP plans an installer in the manner of The Convergence's. It carries only the native renderer
and our settings page, no texture and no texture processing; it requires the player's own game (the executable and the original
files), checks the official Steam executable's hash and Valve's signature on `steam_api64.dll`, and nothing more: no DRM. ROADMAP C5
carries it.

## D-0268 - Guarding the judge: three rules against reward hacking (Thomas, 2026-11-10)

Thomas (2026-11-10 07:58 Paris, « Oui c'est bien », to the coordinator's proposal): (1) the judge is not touched silently: any change
to a test, a threshold, a gate's reference or a measuring flag is stated in the report with its reason, an unstated one counting as
a failure; (2) before every merge, an anti-cheat review by a dedicated subagent looking only for weakened tests, special cases for a
scene, effects cut during a measure and numbers without evidence (`.claude/agents/integrity-reviewer.md`, in skill
`merge-workstream` after the final review); (3) before and after stills and films for every visual gain, Thomas's eye the final judge.
WORKFLOW « Method rules » carries them (« Guarding the judge »); they extend « Reporting: honesty first » and the final review's own
checks rather than duplicate them.

## D-0269 - Names and comments written to the MCP copy of Ghidra, one writer at a time (Thomas, 2026-11-10)

Thomas (2026-11-10, 18:22 to 18:29 Paris: « Si ça peut vous faciliter le travail alors j'autorise », « Oui je t'autorise »,
« Ok, pendant qu'il renomme, les autres restent en lecture seule »; his ask of 18:26, « renommer tous les FUN d'un coup avec le savoir
du dépôt »): the read-only rule of 2026-10-29 is lifted for the copy his GhidraMCP opens
(`Tools/DS3-Decompilation/ghidra-mcp-ds3`) only. Function names and a function's head comment may be written there; one session writes
at a time, the one holding the token file `Tools/DS3-Decompilation/ghidra-mcp-ds3.writer` (the session's name and the Paris time; taken
when absent or older than 4 hours, removed at the end); every other session reads only meanwhile. A name is written only when the
repository states the function's role (no evidence, the `FUN_` stays), as `<Role>_<address>` so that the address, the repository's
key, stays in the name; the head comment says the role and the repository source. Every name written is a row of
[`decompiled/GHIDRA-NAMES.tsv`](../decompiled/GHIDRA-NAMES.tsv) (address, name, role, source), the registry of what the copy holds.
GhidraMCP cannot save: Thomas's Ghidra saves (Ctrl+S, or at its close). The `ghidra-research` subagents, the headless mirrors and
`game/ghidra` stay read-only. WORKFLOW « Tools » carries the rule.
Thomas's save (20:02) wrote the database in Ghidra 12.1.4's format (the x86 language V4.9), which the headless Ghidra 12.1.3
cannot open; with his « Oui refais les » (18:35) the three DS3 mirrors were remade from the saved copy (the earlier ones kept as
`<name>-pre-names-2026-11-10`, removed at 20:24 with his « Va pour 1 à 3 »; the copy before the names,
`ghidra-mcp-ds3-backup-2026-11-10`, kept) and the DS3 headless launchers moved to a copy of his 12.1.4 install
(`Tools/DS3-Decompilation/ghidra_12.1.4_PUBLIC`), which reads both formats; Elden Ring stays on 12.1.3.

## D-0270 - Our own Graphics page and the Advanced page behind it; Auto-detect removed (Thomas, 2026-11-10)

Thomas (the second R12 pilot's thread, 2026-11-10 19:23 to 20:40 Paris), after the first run showed the game's Graphics page has six
fixed slots (I-0132 R12C-STATIC §5), which amends D-0266's « the game's Graphics page keeps Screen Mode, Resolution, Auto-detect and
Quality Setting, with Borderless, V-sync and the frame rate added »: « Par contre je préfère que ça soit Plein Écran > Sans Bordures >
Fenêtré pour l'ordre » (19:23); « Oui on recrée la page Graphics pour y ajouter Vsync et le framerate en plus de l'option Borderless
comme initialement, le reste ça part dans Advanced comme prévu » (20:30); « Advanced Settings c'est le nouveau nom de Quality Settings
[…] C'est ce même bouton qui va rediriger sur la page que R construit » (20:32-20:33); the game's Low/Medium/High/Max preset kept at
the head of the Advanced page (« Oui laisse », 20:37); Auto-detect removed and kept OFF (« Oui on fait ça du coup, c'est mieux, vu
que ça sert à rien », 20:40, his 2026-10-31 decision of the options sheet G §7a). So: our Graphics page holds Screen Mode (Full screen >
Borderless > Windowed), Resolution, V-sync, the frame rate, then the Advanced button; the Advanced page recreates the game's quality
page (the preset, DS3's five rows still in use) with our RT categories; the game's Graphics tab only leads to them (two entries,
I-0132 RESULT « R12c's door and our Graphics page's logic »).


## D-0271 - The game's own Graphics tab and Advanced Settings page as the base of ours (Thomas, 2026-11-10); amends D-0270

Thomas (the R pilot's thread, 2026-11-10 23:02 Paris): « Vous avez pas accès à la page originale dans le code? Puis la copier comme
base ». The pilot read the game's pages (I-0132 R12C-STATIC §8): the Advanced Settings page's rows are plain C++
(`AdvancedSettingsPage_BuildRows_140AAFD10`, one row a setting), its film has fourteen row slots of one kind, a scroll bar and a greyed
state (`GrayoutItem`, the row constructor's « enabled » argument), and the Graphics tab's six slots take D-0270's five rows when each
goes to a slot of its kind. Recommended: our rows in the game's own pages through its row functions, no film copied or edited, the
look, cursor, sounds, mouse, pad and help line the game's; Thomas (23:06): « Vas y ». So: the game's Graphics tab holds
Screen Mode (Full screen > Borderless > Windowed), Resolution (the game's own row), V-sync, the frame rate and the game's Advanced
Settings button; its Advanced Settings page holds the Quality Setting preset, DS3's five rows still in use, then our RT rows, an
unavailable one greyed and locked (D-0265). D-0270's page content and Auto-detect's removal (kept OFF) stand; its native-drawn page
and the door to it are dropped. The native overlay (I-0132 R12a) serves the shader precompile screen and the « Rekindled » title
screen (R12d, R12e).
Thomas's asks while it was built (the same thread, 2026-11-11 00:44 to 02:21): the rows built again live when a governing row changes;
one Anti-aliasing row above Rendering, Off, the TAA, then DLSS, the DLSS mode in a row that appears only with DLSS (his exception to
D-0265's « never hidden »: « un sous menu apparaît »); the sharpening kept with DLSS, as a slider. Consequences recorded in I-0132's
RESULT « R12c »: the Advanced page's Defaults row left out (fourteen slots); the game's Antialiasing option, which no row offers any
more, kept On (and so saved in GraphicsConfig.xml), DLSS and the TAA needing it; « Off » turns DS3's own filter off too.
Later the same morning (the same thread, 04:29 to 05:29): the sub-rows marked « └ »; « Anti-Aliasing »; the light bounces' row
shown only at the path-traced level (his second exception to « never hidden », 05:09: « Possible de la cacher plutôt ? »); NVIDIA
DLAA its own Anti-Aliasing choice, the order Off, TAA, NVIDIA DLAA, NVIDIA DLSS, the DLSS mode row offering the scaled modes only (05:29).

## D-0272 - The protection stays untouched; encrypted functions are read through the decrypted-code overlay (Thomas, 2026-11-11)

Thomas (the study thread, 2026-11-11 01:14 Paris, « D'accord ») on I-0133's recommendation ([RESULT](../investigations/I-0133-protected-code-map/RESULT.md)
§6): 15,602 functions (3.06 MB) of DS3's code are encrypted at rest and 98.3 % of those bytes were captured decrypted on 2026-10-09
(`evidence/I-0066/artifacts/direct-code-capture-protocol-934/overlay935/overlay.bin`). So: the executable and its protection are never
patched or stripped, runs stay offline (D-0116); an encrypted function is read through that overlay laid in memory over a
12.1.4 mirror (`investigations/I-0133-protected-code-map/tools/I0066Overlay936.java`, read-only, no run) before any loaded-code
run; a full decrypted Ghidra project is built only when a workstream needs many encrypted functions at once (the gameplay work
is the likely one); no new code capture until a needed function falls outside the overlay.

## D-0273 - The general plan replaces the roadmap: five phases, twenty-one workstreams (Thomas, 2026-11-12)

Thomas (project chat, 2026-11-12 04:38 Paris): « Je propose que Fable 5.2 effort élevé, dans un fil frais, analyse la roadmap (et
ce qui est rattaché) et nous fasse un plan général de tout ce qu'il reste à faire étape par étape, chantier par chantier. Ça
remplacera notre roadmap actuelle ». The plan was written read-only from the repository's records and shown to him at about
05:00 with its recommendations and the sixteen contradictions it found between the records; his answers in the plan's thread:
« Ça me va, je suis ta recommandation » (05:13), then « Corrige les incohérences et je suis ta recommandation pour les
décisions » (05:18).

- [ROADMAP](../.project/ROADMAP.md) is replaced by the plan: phases 0 to 4, workstreams 1 to 21, their order, dependencies,
  lanes and models. The earlier text is in tag `archive/pre-roadmap-2026-11-12`. FINISH-LINE stays as the history of D-0228's
  list; RT-PT-PLAN stays the RT/PT design reference. The capability names C1, C2, C3 and C5 are kept.
- Accepted with the plan (its recommendations): the tour of the whole game (workstream 6), the compatibility probes on
  Thomas's PC (workstream 10) and one session of his on the gaps fixed but never validated (workstream 3); the public release
  after phase 2, a private installer from phase 1; I-0127's step C (ExecuteIndirect) not done, the investigation closed;
  I-0120's priorities 4 and 5 and A1 done (workstream 4); the shader compilation screen shown only when a compilation really
  happens; frame generation kept off under the in-game menus; the GI quality row measured in motion and removed if it gains
  nothing; the frame rate stays R12c's value row (the earlier roadmap's two sliders are dropped); HDR10 inside the post chain's
  workstream; T1 the textures in phase 2; a release checklist replaces the table of the 13 obligations when workstream 16
  writes it; Thomas's new pyromancy recorded as a wish.
- The contradictions corrected in the records with this entry: D-0165's « DLLs supplied by the user, everything off by
  default » is superseded (the build links NVIDIA's libraries, D-0252; the rays and the TAA are the defaults, D-0253, D-0265);
  D-0253's « a device without DXR 1.1 still runs without the rays » is superseded by D-0262 and R11e (a message and a clean
  close); the colorimetry: D-0178 is kept, the original's grading is remade in the new post chain beside a new one, D-0262
  removing YEBIS and not the original look; gap 27's fix (k held at 1 under 60 frames/s) is an exception to D-0244, looked
  at again in workstream 8; RT-PT-PLAN §6 is the proposal of 2026-10-30, the delivered rows are R12's; the formal
  qualification of the 13 obligations and the open witnesses list are abandoned (D-0228) and STATE says so; STATE's open-gaps
  row drops the raster lighting's residuals (D-0235, D-0250); WORKFLOW names Fable 5.2; the automatic native-against-original
  comparison and the review by bug class (approved 2026-10-26) take the form of workstream 6.
- Left open, each to be decided by Thomas before the workstream it touches (ROADMAP « Open decisions »): what the installer
  carries of NVIDIA's DLLs (D-0252, D-0267; workstream 10); AMD and Intel, where PT, RR and frame generation need DLSS
  (workstream 14); the repository and GitHub (D-0261 ends the GitHub copy, D-0252 spoke of one « public later », and no
  decompiled code goes on line; workstream 18). Still to check, not to decide: whether DS3's tone map has a chromatic
  aberration at all (D-0266; workstream 2).
- The reading's limits: I-0113's and I-0132's results were read through line-by-line extractions by subagents; the ledger
  and the investigations archived before I-0066 were not read.

## D-0274 - NVIDIA cards only, no NVIDIA DLL distributed, the GitHub copy light, private and Thomas's (Thomas, 2026-11-12; closes D-0273's open decisions)

Thomas (the plan's thread, 2026-11-12 05:33 Paris), on the three decisions D-0273 had left open and on the mandate's « Fable
5.1 » (05:33 and 05:35): « C'est bien Fable 5.2, à l'époque c'était encore 5.1. AMD et Intel on fait rien, j'ai pas de matos de ces marques. Pas
de DLL Nvidia distribuées. Pour l'instant on garde juste le dépôt Github en version légère, en privée, je m'occupe moi même de
le mettre à jour ».

- AMD and Intel: nothing is done for those cards (no such hardware to test on): no FSR, no XeSS, no generic denoiser, no
  testers of those cards. The earlier roadmap's final acceptance on AMD and Intel cards is dropped; a small card stays stood
  for by a limited video-memory budget on Thomas's PC (ROADMAP workstream 10).
- No NVIDIA DLL is distributed: the installer (D-0267) carries none, the player supplies them. D-0165's « DLLs supplied by the
  user » therefore stands for the distributed mod (D-0273's annotation is corrected); the development build still links them
  (D-0252). ROADMAP workstream 10 designs how the module starts and runs without them and how the player adds them.
- GitHub: the copy stays light and private, and Thomas updates it himself: no thread pushes (as before), no decompiled code
  or game file goes on line, no public repository is planned (D-0252's « public later » is not a plan today).
- Before anything goes public: a complete review of the licences and of the security (05:37: « on fera un contrôle complet
  des licences et de la sécurité avant que ça passe public, aujourd'hui c'est encore privé et local pour la plupart, c'est
  pas gênant »), which covers the NVIDIA code the module compiles in (NRD, SHaRC, RTXDI); ROADMAP workstream 18.
- Fable (05:35: « 5.1 était la dernière version de Fable mais désormais c'est 5.2 »): the records that name the model to use
  say Fable 5.2 (WORKFLOW, the mandate); dated accounts of what Fable 5.1 did stay as written.

## D-0275 - The Bandai Namco screen on black: DS3's own logo made for black, and Elden Ring's logo on Thomas's PC (Thomas, 2026-11-12; I-0135)

Thomas (2026-11-12 02:48 Paris): « virer l'écran blanc Bandai Namco qui agresse les yeux et mettre celui de Elden Ring qui est
noir » (D-0182's start logos on black); 02:53: « Pas besoin de ligne dans la page d'options, actif par défaut ». Shown DS3's
logo made for black, he noted it was still the old logo (05:40) and chose, on this thread's decision card at 05:40:29, to take
the logo as Elden Ring shows it, « Sauf si c'est illégal », his Elden Ring being installed.

- The page `05_901_Logo_BNE` is drawn on black, always, with no option row and no flag: its white rectangle leaves the black
  beneath it and its image is replaced on that page alone (I-0135 RESULT).
- The public version, and the fallback everywhere: DS3's own logo made for black, from the game's own bytes at run time
  (the white around the blob and the red letters' counters made transparent); nothing of the game is shipped.
- An explicit exception, on Thomas's PC only, to « no asset outside DS3's own draws » and to « no Elden Ring file »: a local
  development script (`Extract-EldenRingLogo.ps1`, never shipped), run once, reads his own Elden Ring read-only and writes
  `er-bne-logo.dds` beside the module; the module draws it when the file is there. Nothing of Elden Ring goes into Git, the
  cloud or an installer (D-0267); the file and its sources stay outside the Evidence Store.
- The rule is keyed to one movie by name and one texture by its TPF name: an exception to D-0161 (rules per family), covered
  by D-0182, which names this page.

## D-0276 - Workstream 11's plan: Thomas's answers on the post-processing chain that replaces YEBIS (Thomas, 2026-11-12; I-0138)

Thomas (thread of ROADMAP workstream 11, 2026-11-12 11:55 to 12:21 Paris), on the plan of
[I-0138](../investigations/I-0138-modern-post-chain/PLAN.md) §7 (the chain in Elden Ring's order with DS3's values, two
colorimetries, eight tranches):

- **HDR** (11:55, « Oui je peux activer HDR »): the HDR10 tranche runs on his OLED, Windows' HDR turned on by him then.
- **Points 2 to 4** (12:19, « Je suis ta recommandation pour les points 2 à 4 »): no settings row for the chromatic
  aberration, which DS3 never draws in the world (I-0138 STUDY §2.3: its enable 0 in every world file, none in the
  original's capture at the High Wall); it is reproduced in the three cutscene files that enable it (a row comes back for the Rekindled colorimetry, below). This closes D-0266's
  entry on it and ROADMAP workstream 2's check. Elden Ring's shipped values are read from his own Elden Ring's data
  files, read-only, on his PC only, for a candidate of the Rekindled colorimetry (as D-0275's frame: nothing of Elden
  Ring into Git's published copy, the cloud or an installer; its LUT images are not taken). The emissive glow moves
  before the tone map, as in Elden Ring, at the HDR10 tranche, judged on sheets.
- **The light shafts** (12:19, « j'aimerais éviter le travail inutile, tu proposes quoi? »; 12:21, on the pilot's proposal
  to keep the current pass bridged to the new bloom until workstream 12: « Je suis pas fan des adaptateurs franchement »;
  12:34, on the proposal that followed: « Sinon je suis ok, ça me va »): no bridge between the old pass and the new chain;
  the shafts are rewritten once inside the new bloom (tranche 2) with DS3's formulas, as Elden Ring keeps screen-space
  shafts beside its volumetric fog; workstream 12 judges on sheets whether the traced fog replaces them.
- **Elden Ring's chromatic aberration** (12:34, once its shipped values were read: on in 169 of 173 gameplay records, a
  dispersion of about 0.0012; « S'il a une légère aberration chromatique, on garde »): the Rekindled colorimetry's
  candidate made after Elden Ring keeps that slight aberration. The pilot's reading, said to him: the original
  colorimetry is unchanged (none in the world, the three cutscenes), and a settings row to switch it comes back with
  tranche 7. This amends D-0266's « none is added that YEBIS does not have » for the Rekindled colorimetry only.
  Confirmed by Thomas: a 👍 on that reading on 2026-11-12 at 12:37 (relayed by the coordinator), then in tranche 1's thread on
  2026-11-13 at 09:55, asked for a line: « Je confirme ce qui a été dit »: the aberration in the candidate made after Elden Ring only.
- **The study in the roadmap** (12:34, « N'oublie pas de joindre ton étude à la roadmap pour pas qu'elle soit oubliée »):
  ROADMAP workstream 11 carries the plan's eight tranches and links the study and its annexes.
- **The automatic exposure** (2026-11-13, tranche 4's thread, on tranche 3's sheet of five views, our half strength
  beside the full adaptation within bounds read from Elden Ring, posted there at 18:25; 18:30, « Mais toi tu recommandes
  quoi? »; the pilot's answer, 18:50: the half strength, the bounds brightening the dark places 0.4 stop more and
  flattening the contrast the traced lighting brings, an exposure after Elden Ring's to be judged again at tranche 6 with
  the Rekindled colorimetry's curve; 18:57, « D'accord on fait ça »; 18:58, « Et ok demi force »): **the half strength stays** (D-0264 unchanged), the
  bounds read from Elden Ring are not retained now; the test flag `auto-exposure-er-bounds.flag` is removed (tranche 4's
  part 4a).
- **Who pilots the rest of tranche 4** (the same exchange: 18:24, « Est ce que ton chantier demande encore Fable ou
  pas ? »; the pilot's answer: not for parts 4b and 4c, methodical work Opus 5.5 at high effort can do with the method
  and tools of the repository; Fable for tranches 5 and 6 and workstream 12; « D'accord on fait ça »): Fable finishes
  part 4a, parts 4b and 4c go to an Opus 5.5 thread, Fable comes back for tranches 5 (HDR10) and 6 (the Rekindled
  colorimetry).
- **The Rekindled colorimetry's candidates** (2026-11-13 18:55, the project chat, relayed by the coordinator: « AoG ?
  AX? Ça ressemblait à ça les noms », « Oui, on en avait parlé, on peut les utiliser »): AgX and ACES may be among the
  tone-map candidates of tranche 6.
- Not asked of him and the pilot's own: tranche 1 is compared bit for bit with the original's captured draws (no
  tolerance), and the blue-noise dither comes with the HDR10 tranche's output transform (I-0138 TRANCHE-1-READY).

## D-0277 - The threads and the coordinator may contest Thomas's choices and decisions (Thomas, 2026-11-14)

Thomas (project chat, 2026-11-14 06:06 Paris): « Je mets une nouvelle règle en place: Vous avez désormais le droit de discuter,
contester, mez choix et mes décisions », then 06:26: « Ma nouvelle règle doit aller dans le dépôt » (both relayed by the coordinator
with his words cited); in the study thread, 06:27: « fusionne claude/admiring-heyrovsky-ec1367 dans main et
inscris ma règle de contestation dans le WORKFLOW ».

- Every thread and the coordinator may discuss and contest Thomas's choices and decisions, the past ones and the new ones.
- The objection goes to him directly in the thread (D-0224), frankly, with its reasons and its evidence (a file, a run, a
  measure); then what he settles is applied, his decision staying his (D-0001's roles unchanged).
- WORKFLOW « Communication with Thomas ».

## D-0278 - The game's own anti-aliasing filter is removed: without the TAA or DLSS a frame has no anti-aliasing (Thomas, 2026-11-14)

Thomas (the R thread, 2026-11-14 11:30 Paris, on the pilot's mention of « le vieux filtre du jeu »): « Quel vieux filtre? J'étais pas
au courant »; 11:42: « Ok, supprime ce fameux vieux filtre du jeu. L'option off doit être vraiment off »; 11:43: « Si tu peux pas le
supprimer alors l'option TAA doit être renommée en TAA+FXAA pour correspondre à la réalité ». Read by the pilot at 19:10 only (its
turn had not ended since the morning).

- DS3's own filter (`GXAntiAliasingFilter`, FXAA.vpo and DLAA.ppo on the 8-bit colour after the tone map; I-0066
  AA-NATIVE-READY) is removed from the native module (I-0138 tranche 5c). It amends D-0265's rule « DS3's own filter where
  neither resolves a frame »: a frame that neither DLSS nor the TAA resolves has no anti-aliasing, on an SDR chain as on an
  HDR10 one (whose float frames the filter never ran on).
- What it changes for the player: nothing with the page's Anti-aliasing on Off (the filter was already off there since
  I-0132 R12c) nor while the TAA or DLSS runs (it never ran beside them, so no row is renamed); only a TAA or a DLSS that
  does not resolve a frame (the TAA's latch or a frame its gate refuses, DLSS not ready or latched off) no longer falls
  back on the filter.
- The pilot's morning answer to him (« en SDR, le vieux filtre du jeu prend alors le relais » when the player turns both
  off) was wrong and is corrected in the thread (19:12).

## D-0279 - Three points settled by Thomas's « Oui »: a cutscene's own lights kept for now, local copies of Elden Ring's data for trials, the options film pinned (Thomas, 2026-11-14)

Thomas (the R thread, 2026-11-14 19:57 Paris): « Oui », to the pilot's message of 19:50 that named the three entries of this log
waiting for his word in that thread.

- **A cutscene's own lights kept on a traced frame, for now** (I-0141 tranche 1, its variant B). The pilot's question
  (the R thread, 18:50, with the pictures of both variants beside the original's); his answer in the project chat, cited
  to the pilot by the coordinator: 19:35, « B oui »; 19:36, « Ce n'est que temporaire le temps d'ajuster les sources
  lumineuses comme fait DS2LightningEngine ». The lights 1 to 3 of the light sets of a cutscene's own file (both keys of a
  draw's light key block in the cutscene files' area range, 2AABBVVVV) stay on a traced frame, unshadowed: a **provisional
  exception to D-0262**'s removal of the sets' fill lights (in 39000020 they are what lights Siegward, his set's light 0
  being off), until the light sources are set by us (ROADMAP workstream 12; this reference added on Thomas's « Oui stp, pour
  que ça soit clair », project chat, 2026-11-15 03:17, confirmed « Oui » in the R thread, 05:11). With it: where a cutscene's ShadowParam names no light set, the traced
  sun takes light 0 of the cut's own set when that light is on (the coordinator's recommendation of the same day while he
  slept, I-0140 RESULT « Thomas's choice for the rest », its option 1: kept with B).
- **Local copies of Elden Ring's data for trials** (I-0138 tranche 6; his word in the project chat, 12:28, of which only the
  coordinator's wording reached the pilot, no verbatim: copying ER's data is allowed locally where it helps a trial;
  anything distributed needs another solution in every case; to be noted in the repository). D-0276's « without its LUT images » stands for everything
  distributed; the ER folders stay read-only (D-0195).
- **The HDR rows' place and form, and the options film pinned** (I-0138 tranche 5b; his request in the project chat,
  12:05, cited by the coordinator: « Je préfère que les réglages du HDR soient dans le menu graphics et pas advanced.
  Aussi, je préfère que ça soit des curseurs plutôt que des sélections. Ça peut aller par tranche de 10 ou de 50 (au choix
  de R) », « Il faudrait également que le réglage de luminosité du jeu de base se désactive si une sortie HDR est
  détectée, pour pas faire doublon ni perturber »). The three slots the tab needs are added to the options film as the game
  loads it, a rule keyed to one film by its name, size and SHA-256: **an exception to D-0161** (rules per family), as
  D-0275's was for one movie; another film leaves the tab its five rows.

## D-0280 - The automatic exposure brightens the dark views and never darkens under the zone's own exposure (Thomas, 2026-11-14)

Thomas (the R thread, 2026-11-14 22:16 Paris): « Oui pour l'auto expo corrigée », to the pilot's proposal of 20:33 with its
sheet (the original, the native, the native without the automatic exposure, at three paired moments of 32000010 and of
Archdragon Peak in play; I-0141 RESULT « 32000010 at paired instants »: the whole picture's mean level 177 / 96 / 172, 152 /
95 / 155 and 103 / 62 / 87 of 255).

- It amends D-0264: the automatic exposure stays the default at half strength toward the game's one target, and its factor
  has a lower bound of 1: it brightens a view darker than the target and leaves a brighter one at the zone's own exposure.
  Before, it darkened by one stop to a stop and three quarters the zones the game makes bright on purpose.
- The test tools stay: `no-auto-exposure.flag`, `rt-auto-exposure-full.flag` (full strength, under the same bound).
- I-0138 tranche 5e.

## D-0281 - The next R thread runs on Opus; Fable comes back for workstream 12 (Thomas, 2026-11-14)

Thomas (the R thread, 2026-11-14 22:38 Paris): « Il y a encore besoin de Fable? »; the pilot's answer (22:40): not for
what comes next; 22:42: « Va pour ça », then « A inscrire pour pas oublier ».

- Workstream 11's tranches 5 and 6, for which Thomas had asked Fable (D-0276's entry), are merged. What follows is piloted
  by Opus 5.5 at high effort, the default (D-0245): workstream 7's remainder (the cutscenes' lighting, I-0141), the settings
  the native never reads (I-0140's STUDY-UNREAD-SETTINGS), workstream 11's tranches 7 (the settings rows) and 8 (YEBIS
  removed).
- Fable 5.2 at high effort comes back for workstream 12 (physical light sources, the traced volumetric fog), as Thomas
  gave it on 2026-11-10, and for the Rekindled colorimetry only if his pick asks more than the three candidates of
  tranche 6.
- Applied the same night: the fresh R thread opened on Opus from `main` `7f56a34f8`.

## D-0282 - D-0279's provisional exception follows the map keys a cutscene overrides (Thomas, 2026-11-15)

Thomas (the R thread, 2026-11-15 03:14 Paris): « Oui », to the pilot's question of 03:08; and in the project chat, 03:14,
cited to the pilot by the coordinator: « Oui, de toute façon c'est provisoire en attendant l'ajustement des sources
lumineuses ».

- During a cutscene whose act overrides a map's light sets by the cutscene file's (I-0141 tranche 3), a draw whose light keys
  are such overridden map keys, their sources a cutscene file's sets, keeps those sets' lights 1 to 3 on a traced frame,
  unshadowed, as D-0279 keeps a cutscene's own keys': they are the scene's authored lights. A map key not overridden keeps
  D-0262's removal.
- Provisional like D-0279: until the light sources are set by us (ROADMAP workstream 12).
- I-0141 tranche 5.

## D-0283 - The reviewers look for what is wrong, from the diff and the goal, without the pilot's summary (Thomas, 2026-11-15)

Thomas (the project chat, 2026-11-15 04:39, cited to the R pilot by the coordinator): « Donner au relecteur le rôle de chercher
les problèmes, pas de valider. « Trouve ce qui ne va pas » donne de meilleurs résultats que « vérifie que c'est bon ». Donner au
relecteur le diff et l'objectif, pas le résumé du pilote. S'il lit d'abord « j'ai corrigé X avec succès », il est déjà orienté »;
then in the R thread, 04:42: « Ça vaut pour les deux. Les opus 5.5 high d'étape et les opus 5.5 xhigh à la fin. Tu as mon feu
vert ».

- The step review (Opus 5.5 high) and the final review (Opus 5.5 xhigh) are briefed to find what is wrong, never to confirm.
- Their brief holds the diff, the goal (the CONTRACT, the step's goal) and the exit criteria; not the pilot's account of what it
  did or fixed. The records the diff carries are read after the code, as claims to test.
- WORKFLOW « Roles and delegation », the subagents `step-reviewer` and `final-reviewer`, and (Thomas, the R thread, 04:45: « Fais
  en des skills comme pour le Ghidra mcp par exemple. Et inscris les dans le workflow ») the skills `review-step` and
  `review-final` that write and launch their briefs. The integrity review (D-0268) keeps its brief: checking the RESULT's claims
  against the evidence is its object.

## D-0284 - Every test of the game tour becomes a reference; a full tour again at the project's end (Thomas, 2026-11-15)

Thomas (the project chat, 2026-11-15 05:19 Paris): « Je propose que tous les tests du Tour doivent devenir une référence. Un
autre Tour complet sera fait à la fin du projet »; the tour thread's plan (05:20); Thomas in the tour thread: « À inscrire dans le
dépôt », then « Oui ».

- Every run of the game tour (I-0139: bonfires, bosses, menus, cutscenes, NPCs, credits) is kept as a reference. I-0139
  `REFERENCE.md` names each run set's runner, rows, save (by hash) and screen; `REFERENCE.tsv` each run's module (its SHA-256 from
  the run's baseline, mapped to its build folder; the build folders' commits are in the RESULT, but for `build-tour4`'s, not stated), screen, window, exit and
  restoration, and from 2026-11-15 the runner's arguments (`alphacast-args.json`, Run-AlphaCast).
- At the project's end the whole tour is run again from that list, on one screen and one build, against the reference runs
  and the original renderer. The night of 2026-11-14/15 mixed the OLED (2560x1440 window) and the ASUS (1920x1080), so a
  replay names its screen.
- WORKFLOW « Tools »: the row « The game tour as a reference ».

## D-0290 - Before a stand-in is removed or a refusal becomes an empty skip, the original's answer for that input is read and every met case witnessed (Thomas, 2026-11-15)

Thomas (the study thread, 2026-11-15 05:42 Paris, « Oui vas y », answering the proposal of
[STUDY-NARROW-WITNESS](../investigations/I-0137-robustness/STUDY-NARROW-WITNESS.md) § 5): I-0137's A2 removed the movie proxy's
SYSTEX_BLACK on one witness (the character creation) and a reading of the import's 0 path; the original's proxy answers its inner
texture and the credits page asks once, so the credits showed a « MOVIE » plate (I-0139 tranche 5, I-0140 « Next » 9).

- Before a stand-in is removed, or a refusal becomes a skip that draws nothing, the original's answer for the same input is read,
  the arguments' own path included (a proxy's slot, a holder's inner texture), not only the function's return; a change that
  relies on a retry names the caller that asks again.
- The case is witnessed in every place the journals show it met (the row the stand-in or the refusal writes names them).
- A default the original binds itself (SYSTEX_BLACK behind a movie proxy, SYSTEX_DUMMY for a texture with no data) is the
  original's picture, not a stand-in under D-0197.
- WORKFLOW « Method rules », after « Skip the object and go on ».

## D-0291 - The documentation diet: short control files, their history archived, an index of the decisions in force, a cleanup before every merge (Thomas, 2026-11-15)

Thomas found the documentation gigantic (STATE about 48 KB, WORKFLOW 49 KB, ROADMAP 36 KB, RT-PT-PLAN 43 KB, this log 164 KB,
the investigations index 47 KB) and asked, through the coordinator, for short documents the threads and the coordinator really
read, without losing a rule or a decision of his. He validated the plan in this thread before it was written (« Je valide
d'avance », 2026-11-15 05:55 Paris), then asked for the size warning and « un nettoyage de la doc avant chaque fusion » (05:58).

- Nothing is deleted: the earlier texts of STATE, WORKFLOW, ROADMAP, the investigations index and CLAUDE.md are kept verbatim in
  `.project/archive/2026-11-15/` (links rewritten to resolve there) and in tag `archive/pre-cleanup-2026-11-15` (on `ad15b002a`).
- STATE holds the current position only; its « Updated » line names the last checkpoint alone; its rows are short.
- WORKFLOW keeps every rule, shorter: the tools' rows move verbatim to `.project/TOOLS.md`, the method sections for
  investigations, coverage by families and the frame challenge to `.project/METHOD.md` (same headings, stubs left in WORKFLOW).
- ROADMAP keeps one line per finished step; the account is the investigation's RESULT.
- [IN-FORCE](IN-FORCE.md) lists every decision in force in one line, grouped by theme, with a table of those consumed or
  superseded since D-0177 and a short list to confirm; every new decision adds its line there. This log stays whole.
- The investigations index keeps one short row per investigation. RT-PT-PLAN stays as it is, a historical design out of the
  reading route (its plan is ROADMAP's workstreams 7, 11 and 12).
- Each control file has a size target (WORKFLOW « Canonical sources ») and `Test-Project` warns above it, and when STATE's
  « Updated » line holds more than one checkpoint; its warning on a long STATE row moves from 300 to 150 words, since the rows
  now hold the current position only. The targets (STATE 14 KB, WORKFLOW 36, ROADMAP 24, IN-FORCE 40, the investigations index
  28, CLAUDE.md 3.5) are each file's size after the diet with room for some growth, so that a warning means history piling up
  again, not one more rule; a target is changed only with its reason written here or in a later decision.
- The documentation is cleaned before every merge, not only at the checkpoint after it: skill `merge-workstream` step 3.

## D-0285 - The Rekindled look is the one after Elden Ring alone, beside the original, with ER's midtone lift as a curve of ours (Thomas, 2026-11-15)

Thomas's pick among I-0138 tranche 6's candidates (the project chat, relayed by the coordinator): « ACES Neutre et Elden Ring » (09:16), then
« Juste Elden Ring alors » (09:17), and « Oui » (09:18) to the R thread's proposal (09:20 by the thread's clock) to add the lift of the
midtones and shadows that ER's colour-grading LUTs make (I-0138 RESULT « ER's LUTs, read »: close to the identity otherwise) as a curve
of the project's own, since ER's LUTs cannot be distributed (D-0274).

- The Rekindled colorimetry offers one look besides the original: the candidate after Elden Ring (its curve, vignette, lateral
  dispersion and the bloom before the curve, without DS3's grey grade), with that midtone and shadow lift; AgX and ACES are not offered.
- ER's per-zone grading and its bounded automatic exposure are not part of it (the exposure stays D-0280's).
- I-0138 tranche 7 makes the settings row: **the original is the default**, the look after Elden Ring (« Rekindled ») an option
  (Thomas, the R thread, 2026-11-15 09:21: « le style Elden Ring (Rekindled) est une option, l'original est le choix par défaut »).

## D-0292 - The weekly limit read at every checkpoint (Thomas, 2026-11-15)

D-0248 says « at 50 % » without saying when a pilot looks: on 2026-11-15 the fresh threads of the morning had not read the weekly
limit, and the 50 % threshold was crossed between two threads (the coordinator applied D-0248 at 10:05 Paris; the E thread read
57 % at 10:09, after Thomas's exception). The coordinator
proposed that every pilot read the limit at each checkpoint and add it to the line it sends; Thomas answered « Oui » in the project
chat (10:10 Paris), then authorised the E thread in its own thread to write it here (« J'autorise E à inscrire D-0292 dans
decisions/LOG.md, IN-FORCE.md et WORKFLOW », 10:22).

- Every pilot reads the weekly limit (all models, and Fable's when it runs Fable) at each checkpoint and adds the percentage to the
  checkpoint line it sends the coordinator. D-0248's thresholds (50 %: the note and the open-gaps pilot's stop; 90 %: the slow-down;
  99 %: the pause) are unchanged.
- The same day Thomas made an exception to D-0248's stop for E (« Non continuez exceptionnellement », 10:08 Paris): E continued its
  tranche past 57 %. An exception for that day, not a standing rule.

## D-0293 - The local lights at DS3's own strength (Thomas, 2026-11-16)

D-0264 (2026-11-09) kept the traced local lights' colours at twice DS3's (« Va pour x2 », R11d's sheets), provisional until workstream 12
(Thomas, 2026-11-16 01:30). On the R thread's proposal Thomas agreed (01:31, « D'accord ») to an A/B of x1, x2 and the original, the
measures deciding. The A/B (I-0141 RESULT « Tranche 10 », `i141-ab-*`, 05:29 to 06:16): x1 is nearer the original at Cleansing Chapel
(4.9 of 255 under it on average, x2 7.2 over), Church of Yorshka (+9.7 against +15.5) and 45000030's later cuts (+0.9 to +15.0
against +14.8 to +38.7); in play after 32000010 x2 nearer by about 2, within the runs' spread; Firelink's start still 48.6 at x1
and 66.5 at x2 against the original's 46. Thomas, in the project chat (06:35): « j'avais dit x1 [x2] car le jeu me semblait sombre
sans fill light ... c'était qu'un artifice en attendant. On garde x1 pour toujours », then in the R thread (06:45): « Oui pour les deux ».

- The local lights take DS3's own strength by default; `rt-local-x2.flag` and `rt-local-x4.flag` stay test tools. D-0264's x2 clause
  is replaced; the light sources themselves are workstream 12's (D-0262).

## D-0294 - The automatic exposure's target lowered to 0.12 (Thomas, 2026-11-16)

The replayed-path gate's still at Firelink was brighter since D-0280's merge (`c9cc6bcf8`: the factor log2 -0.46 before, 0 after).
Thomas (the project chat, 06:33): « Faites comme bon vous semble pour l'exposition auto, ma règle n'était pas la bonne visiblement. Un
sous agent Fable 5.2 high peut trancher ». The analysis (a Fable 5.2 high subagent of the R thread, read-only; I-0141 RESULT « Tranche
10 »): the excess at Firelink is D-0264's x2 lights, not D-0280 (the still 46 in the original, 48.6 at x1, 66.5 at x2: D-0264's
darkening was hiding it); the original never adapts, and its 540 tour pictures have a median mean grey of 0.19 and a tenth
percentile of 0.116: pulling every view toward 0.20, its middle, brightens interiors whose lighting is already right (Church of
Yorshka at x1: x1.36 of the original with the exposure's lift, about x0.97 at the zone's own exposure, INFERRED). Thomas, in the R
thread (06:45): « Oui pour les deux ».

- The automatic exposure keeps D-0280's lower bound (it never darkens under the zone's own exposure) and the half strength; its one
  game-wide target goes from 0.20 to 0.12, so it brightens only views darker than the original's darkest tenth. The same in play and
  in cutscenes. D-0263's single target stands; D-0280 stands but for its target.

## D-0295 - The native build in parallel, and a thread's object cache (Thomas, 2026-11-16)

The build study ([I-0142 STUDY](../investigations/I-0142-build-speed/STUDY.md)): `Build-GameBootstrap.ps1` ran its 443 commands one
after another (10.5 min of 13 to 18 on 16 threads) and every fix rebuilt everything, its objects deleted (D-0260) and each version
in a new folder. Thomas, in the I-0142 thread: « Vas y » (12:41 Paris) to the study's two steps, then « Je confirme » (12:55) for
the second, the object cache that relaxes D-0260.

- The build runs its commands in parallel by Ninja (Visual Studio's own), `-Jobs` at once (12 by default); `-Serial` keeps the serial
  path, the reference the parallel build equals byte for byte.
- `-Cache <folder>`: a thread may keep one object cache whose compiler objects are kept (D-0260's deletion does not apply to it); the
  build reruns there only what changed (MSVC's header lists, the command lines), then publishes the module, its PDB and this run's
  results into `-Out`, one folder per version as before. The cache is rotated as any build folder (`Clear-DiskCache`, 48 h).
- The launcher's build after a merge and the build a merge's reviews read stay complete builds (no `-Cache`).

## D-0296 - The Rekindled look is the vivid variant (Thomas, 2026-11-16)

Amends D-0285. On tranche 7's sheets Thomas saw « pas beaucoup de différence entre default et rekindled » (09:55). A Fable 5.2 subagent
(read-only, Thomas's agreement for the colorimetry) measured ER's 76 grading LUTs (`evidence/I-0138/artifacts/tranche-7/vivid-er-luts/`):
per-channel tone curves with cool shadows, no saturation operator, the grey median 68 -> 68 where D-0285's lift gives 89. The vivid variant
(I-0138 RESULT « Tranche 7b ») was shown beside the original and the first Rekindled at three zones; Thomas, in the R thread (12:06):
« Oui j'aime bien ».

- The « Rekindled » colorimetry is the candidate after Elden Ring with D-0285's midtone lift and, after it, a per-channel contrast
  bringing the grey curve back to ER's median (0.8), a gentle vibrance sparing skin (0.25) and a cool black (0, 0, 10 of 255).
- The original stays the default (D-0285; Thomas, 2026-11-16 10:01: « c'est DS3 le default, Rekindled devra être choisi »), the
  player's choice kept in the settings file. Judged on SDR sheets; its HDR10 look joins the OLED's open A/B.

## D-0297 - The Rekindled look takes DS3's own vignette by area (Thomas, 2026-11-17)

Amends D-0285 and D-0296. Thomas found the Rekindled vignette too dark (« vignettage trop sombre »; no slider, the On/Off row kept). A Sonnet
subagent re-read ER's vignette (I-0141 RESULT « ER's vignette re-checked »): the project's formula was ER's, but its Power is keyed by
the hour (about 0.4 by day, 0.5 in the evening), the project had the evening's all day. Thomas, in the R thread (09:05): « Sinon on
reprend la vignette de DS3, c'est sûrement mieux vu que ça dépend des zones »; on the pilot's agreement (09:06): « Je suis d'accord ».

- With the Rekindled look the vignette is DS3's own: the area's Vignette Power (none where the area has none), on the exposed scene
  before the curve, the bloom not vignetted, as the original's programme; the player's On/Off row kept.
- Elden Ring's vignette is removed (no adapter beside it), with its words in the tone-map block (back to 224 bytes).

## D-0298 - One idea outside the plan at each checkpoint (Thomas, 2026-11-17)

Thomas, in the project chat (10:17): « J'aimerais quand même vous laissez la possibilité de penser "out of the box" et me surprendre »,
then « oui »; in the R thread (10:19): « oui vous pouvez ».

- At each checkpoint a pilot may propose to Thomas, in its thread, one idea outside the plan (a rendering technique, a tool, a
  shortcut) to surprise him.
- The idea is made only with his agreement.

## D-0299 - Original and native compared at the same moment of the game (Thomas, 2026-11-17)

The final review of I-0141 tranche 11 found the smoke's native and original samples taken 2.5 s apart in the cutscene (pressed
after a fixed delay), which turned about 100 extra entities a frame into « about 35 ». The pilot proposed sampling both modes on
the game's own clock; Thomas, in the R thread (11:33): « D'accord, à inscrire cette méthode dans le dépôt ».

- Every comparison of the original and the native takes its samples on the game's own clock in both modes (a cutscene's time,
  an effect's or a spell's own start, an animation's frame), not on the runner's wall clock or a fixed delay after a key.
- Each sample names that clock's value; a pair whose values differ is not compared.

## D-0300 - Workstream 10 keeps only the compatibility probes; the production build and the installer go to workstream 18 (Thomas, 2026-11-20)

Thomas asked whether the installer would not be more logical at the end (project chat, 07:17). The R pilot answered that
workstream 10 holds two things worth doing early, the compatibility probes and how the module starts without NVIDIA's DLLs,
and that the packaging could go with the publication. Thomas, in the R thread (09:35): « Va pour les sondes de compatibilité
uniquement, sinon pour le reste change rien, n'avance pas exprès le chantier 12 ».

- Workstream 10 (phase 1) keeps only the compatibility probes on Thomas's PC: a run under WARP, GPU-based validation, a
  video-memory budget limited to 4 then 6 GB.
- The production build without development hooks, flags or test launchers, the installer (D-0267), how the module starts and
  runs without NVIDIA's DLLs and the list of what it compiles in of NVIDIA's (D-0274), and the Agility SDK's redistribution go to
  workstream 18 (phase 3), before its licence and security review.
- Nothing else in the plan changes: workstream 12 keeps its place (after workstream 11 and workstream 7's step 3), not brought
  forward.

## D-0301 - The menu views keep their raster lighting and take the original's environment region pass (Thomas, 2026-11-21)

In Thomas's character's Quit Game portrait the armour's metal is black natively, blue-silver in the original (I-0140 tranche 27).
The likely cause (INFERRED, no A/B run): the menu views are not traced; since I-0131 R10a removed the region pass they get no specular ambient, and before it they never
made a region natively, where the original's menu views add their own (GILM0010, a 60-unit box at the origin, masks {0x11, 2}) and
its region pass draws only it there ([STUDY-MENU-ENV-REGIONS](../investigations/I-0140-effect-families/STUDY-MENU-ENV-REGIONS.md)).
The E pilot asked whether to bring that raster pass back for the menu views only. Thomas, in the E thread (03:44): « Oui vas y
remets le portrait original, de toute façon l'affichage des armes et armures dans le menu de sélection est également l'image
raster ».

- The menu views (portraits, equipment, character creation) keep their raster lighting and take the original's environment region
  pass with the original's per-view filter (their own region); I-0140 tranche 28.
- Nothing changes for a traced view: no region pass, no raster lighting there (D-0262 stands).

## D-0302 - AMD's FSR added (upscaling and frame generation), Intel's XeSS possible; D-0274's exclusion lifted for them (Thomas, 2026-11-21)

The coordinator asked Thomas in the project chat (2026-11-21 06:57 Paris) whether the exclusion of D-0274 (« cartes NVIDIA
seulement, pas de FSR ni de XeSS ») is lifted to add FSR for AMD cards, its own sheet recording an idea of 2026-11-19 (FSR 4.1 and
3.1) whose message it could not find word for word. Thomas (06:58): « Oui je confirme »; then (06:58): « On peut mettre le XeSS
mais pas sûr que grand monde l'utilisera... ».

- AMD cards get AMD's upscaling and frame generation (Thomas's yes): FSR 4.1 upscaling (the RX 9000 natively; the RX 7000 and 6000
  through AMD's driver Adrenalin 26.10.1), FSR 3.1 upscaling for older cards and FSR 3.1 frame generation, the list as the
  coordinator's sheet of 2026-11-19 records it (to check against AMD's documentation). The pilot's reading, not Thomas's words: behind
  D-0165's neutral interface beside DLSS. Who tests them is open: D-0274 said no testers of those cards, and Thomas has no AMD
  card (the sheet); not asked.
- Intel's XeSS may be added (Thomas's words); last and at low priority is the pilot's reading of his doubt that many would use it.
- D-0274's other parts stand: no NVIDIA DLL distributed, the GitHub copy light and private, a generic denoiser still excluded;
  what the traced lighting does on an AMD card (Ray Reconstruction and SER are NVIDIA's) is not decided here.
- Where (the pilot's reading): ROADMAP workstream 14 (hardware and configurations), after the NVIDIA work; nothing in front of
  the open workstreams.

## D-0303 - The Player menu views get their self-shadow by a traced ray, not the original's shadow map (Thomas, 2026-11-21)

After the Face Detail's re-key (I-0140 tranche 30) the skin's highlights match the original's (its mid-tones and darks still
brighter natively, the final review's M1, consistent with the shadow below); what is left in the Player views
(character creation, Face Detail, the Status body) is the shadow the character casts on itself (the hair on the forehead, the head
on the shoulder, the armour's folds): the original draws it for those views only with a four-cascade shadow map from ShadowParam
{1, 10 or 11} (MENUS-MAP §48); natively these views never had one. The E pilot offered, recommending the first: (1) transcribe the
original's shadow map for those views (D-0301's raster scope); (2) a traced shadow in those views; (3) leave it. Thomas, in the
project chat (12:22): « Choix 2 ».

- The Player menu views (ChrMakePreview, StatusBody; RankingBody when met) get their self-shadow from a traced ray, its direction
  the original's ShadowParam Angle for the view's id (Shadow Light Id -1); the rest of their lighting stays raster (D-0301).
- No shadow map is brought back for them; the Face and Prof views stay without shadow, as in the original.
- How close it must look to the original's filtered map is the pilot's comparison at the same moment (D-0299), reported as measured.

## D-0304 - The end of the project's re-optimisation holds a modern occlusion cull and the survey of the native's extra work (Thomas, 2026-11-21)

The R pilot read that the native draws at m32's bonfire about 346 G-buffer models where the original draws 203 and that the
original's occlusion system never runs natively (I-0141 tranche 18), and the coordinator put to Thomas in the project chat whether
an occlusion cull of our own should be done. Thomas (2026-11-21 14:19 Paris): « Comme c'est de l'optimisation, à la fin oui, à
inscrire dans le dépôt »; (14:22) « Faisons comme ça alors. Je ne connaissais pas », after the coordinator said what modern games do;
(14:27) « D'accord, enfin si les jeux modernes font ce cull pour optimiser, ça reste une bonne idée surtout que notre rendu a
maintenant du PT lourd »; then, on the R pilot's survey of the work the native does that the original avoids (14:37): « Inscrivez
tout ça dans le dépôt du coup ». The coordinator relayed his words and the details below.

- ROADMAP workstream 15 (the second optimisation review, at the end) holds: a modern occlusion cull, not a copy of the original's
  D3D11 queries, on the GPU from the previous frame's depth (Hi-Z) and perhaps per cluster of a model (Nanite's way), whether or not
  the original's occlusion explains m32's extra models; simplified models for the rays; fewer rays with a better denoiser; and the
  survey's eleven items (ROADMAP workstream 15).
- This narrows D-0255 for that workstream only: its « no GPU culling » stands for the workstreams before it; the cull is the
  re-optimisation's (D-0255 measured the raster's whole GPU share at 1.5 to 2 ms of a 17 ms traced frame at 1920x1080, the bound on
  what the cull can save; the pilot's note, not Thomas's words).
- Workstreams 12 and 13 do not wait for it; if the frame rate falls too low to test them, D-0255's CPU lever (the change-driven
  collect and rays' walk) comes forward (the coordinator's relay).
- Thomas (14:43, after the R pilot read that the rays' scene is not sorted: every loaded entity in the TLAS at LOD 0): « D'accord.
  Bon faites ce qui vous semble juste, tant que ça ne casse pas nos rayons tracés ». The methods are the pilot's; no cull or sort
  takes out of the RT/PT scene what a ray can see, and each item is accepted on a picture comparison (RT and PT) against the frame
  before it (D-0227; the comparison is the coordinator's relay).

## D-0305 - The menu views take the native's own anti-aliasing: the TAA by default, DLSS or DLAA when the option says (Thomas, 2026-11-22)

I-0140 tranches 38 and 39: the original's Status body is smoother than the native's (gradient +35 %, stepped edges natively); the
original runs its GX anti-aliasing filter (DLAA, gparam menu ids Enable 1) on the menu views' own single-sample targets after their
tone map (READ twice in its code and reached in play, I-0140 tranche 39; whether it makes the whole gap not tested), while the native gives them none (D-0278
removed DS3's filter and neither the TAA nor DLSS resolves them). The E pilot put to Thomas in the thread: an exception to D-0278
for the menu views (the original's pass) or nothing. Thomas (2026-11-22 05:46 Paris): « Ce sera le TAA par défaut pour nous /
DLSS / DLAA si c'est activé dans les options ».

- The menu views (portraits, the Status body, creation, Face Detail, equipment) are anti-aliased by the native's own chain: the
  TAA by default, DLSS or DLAA when the player's option selects them, as the main view.
- With the anti-aliasing option off, the menu views have none, as the main view (Thomas, 2026-11-22 06:04 Paris: « Et si
  l'anti aliasing est désactivé, on met rien du coup »).
- DS3's GXAntiAliasingFilter stays removed (D-0278), for the menu views too (implied by his choice, the pilot's reading).
- The pilot's method, not Thomas's words: how close the result looks to the original's DLAA is compared at the same moment (D-0299).

## D-0306 - The menu views' TAA: a tighter reconstruction (8) and its own samples on an unjittered reset (the E pilot, under Thomas's « Fais ce qui te semble le plus juste », 2026-11-22)

I-0140 tranche 40: our TAA left the Status body at 0.73 of the original's DLAA sharpness (the portrait 0.61); Thomas chose to leave
it as is with his sharpening slider (option 2), declining a menu-own sharpening and DLAA whenever possible, then (2026-11-22 09:02
Paris, in the E thread): « Fais ce qui te semble le plus juste concernant le TAA ». The pilot's choice, measured in I-0140 tranche 41:

- The menu views' TAA reconstruction is a tighter Gaussian, exp(-8 d^2) over the 3x3 instead of the main view's 2.29 (the
  Blackman-Harris fit): the body's sharpness against the original's DLAA rises from 0.72 to about 0.78 and no further at 12, the
  portrait's (a six-frame state, resolved six times per opening then kept) from 0.61 to about 0.67; the films' frame-to-frame
  change per unit of gradient at lag 1 about 20 % above 2.29's (MEASURED, `i140-t41c`, `i140-t41d`, the films `i140-t41d`'s; the
  witnesses `i140-t41f` and `i140-t41g`; only the Status body and portrait measured, the kernel applying to every tabled menu view). The main view's TAA is unchanged.
- A frame that restarts a menu view's history as its collect sees it (its first, a cut, one after a collect not drawn) is drawn
  unjittered and resolved as its own samples (the renderer's own resets start from a jittered frame); no difference was measured at
  the shots' moment (12 s after an opening).
- The rest of the gap to DLAA (about 0.2) is not the filter's; no menu-own sharpening (Thomas's no to option 1); his slider applies.

## D-0307 - Workstream 12's four choices: the sky at its authored strength, DS3's sourceless placed lights judged after it, windows as emitters of the outside light, a lit fog (Thomas, 2026-11-22)

The study ahead of workstream 12 ([STUDY](../.project/studies/workstream-12/STUDY.md), its summary [SYNTHESE](../.project/studies/workstream-12/SYNTHESE.md);
the study thread, read-only) put four choices to Thomas in its thread with an advice for each. Its grounds: the rays take the sky as
the dome is drawn (about 0.02 to 0.07 at the High Wall, I-0132 LIGHT-UNITS) where the original's baked probes give an open-air ambient
of about 0.27 to 0.29, blue-grey (an order of magnitude, INFERRED, to confirm by a witness in play); 148 of the High Wall's 184 bank
lights have no visible emitter within 3 m; DS3's fog is a fixed colour that no light enters. Thomas (2026-11-22 15:30 Paris): « 1 et 2
je suis ta recommandation / 3 oui, elles reflètent la lumière extérieure / 4. Tu recommandes quoi ? »; then, on the advice for the
fourth (15:31): « Ça me va ».

- **The sky**: the rays take the sky at the strength the original's authors baked, by one rule for the whole game with authored
  inputs; the drawn dome stays as it is; judged on sheets.
- **DS3's own placed lights without a visible source**: named by family first, the sky set, then their removal by family judged on
  sheets; none is removed before (D-0262's « the interiors do not go black »).
- **Windows as emitters** where a visible window is the source of a closed interior: agreed, and their light is the outside's (the
  zone's sky and sun at that window), not a typed value.
- **The fog**: a medium lit by the real lights and shadowed by the rays; the original's maximum opacity kept as a clamp; its colour
  set against the area's daylight (the authored colour in open air, darker in shade, indoors and at night); the screen-space light
  shafts retired where the volume draws real ones, zone by zone on sheets; the effect shafts (FXR 10003) kept at first and looked at
  on the same sheets.
- Each lever stays a test flag until Thomas has seen its sheets (D-0181, D-0268). The methods are the pilot's (workstream 12, Fable 5.2
  high, D-0281); the study's proposed order (names and witnesses first, no picture change) is advice, not part of this decision.
- Not part of it: the idea outside the plan the study offered (a photo mode that moves the sun, D-0298); Thomas (15:32): « Et oui
  ton idée hors plan est bonne »; kept as a wish ([PHOTO-MODE-SUN](../.project/wishes/PHOTO-MODE-SUN.md)), after workstream 12, on his go.
