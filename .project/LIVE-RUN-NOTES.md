# Live-run notes

Practical tricks for driving Dark Souls III in an announced live run (WORKFLOW « Live game rule »). They were kept in Claude's
private memory until the repository audit of 2026-10-25 moved them here, so that every thread sees them. Rules
live in WORKFLOW; this file holds only know-how. Add a line when a run teaches something; remove it when a tool makes it moot.

## Pilots and runners

- A cost A/B between two builds or flags (I-0128 R8c, 2026-11-08): Run-ZoneWalk's looks are timed in seconds, so the faster half
  turns at an earlier frame and the run means mix views; compare the windows before the first look (Get-R8Gpu `-Windows`, the
  window ending at frame 1,800) or run `-Still`. The walk shots can show the desktop when the window loses the foreground: read the
  view from the world-pictures (`world-NNNN.rgba`).
- A still view for an accumulation (a GI mean, a burst of pictures; I-0128 R8d, 2026-11-08): Anor Londo's hall teleport
  (`m37hall`, hold 30) leaves Gamma running in place, the camera follows him and the GI mean restarts every frame. Firelink's bonfire
  view (`m40`, inside the shrine) and the High Wall's hold still; check a dump's frame count (its alpha) before reading a mean.
- Scripted pilots in `investigations/I-0066-independent-native-startup/`: `Pilot-TitleContinue.ps1` (intro, two offline
  notices, Continue; `-Trace none` for Reference runs; it scales its 1280x720 coordinates to the real client rectangle),
  `Pilot-NativeWitness.ps1` (title, in-game pacing, capture, close), `Pilot-EquipList.ps1` (equip by inventory row and column;
  rank = owned items sorted by EquipParamProtector sortId, 5 per row), `Pilot-QuitAndKeepSave.ps1` (Quit Game, then copy the
  save), `Pilot-Game.ps1` (shot, click, key, hold, wheel, look; refuses input unless DarkSoulsIII is in the foreground).
  Since I-0132 R11c (2026-11-10) Pilot-Game brings the game's window back first (AttachThreadInput, then a click on its own caption when
  Windows refuses: explorer kept the foreground for a whole series with the Claude app minimized); the older text below is the history.
  The Claude desktop app can take the foreground during a run (I-0125 R6e, 2026-11-06: four runs of nine, twice at a coordinator's
  message, twice with none): the rest of that run's input is refused and its later pictures show the app. Ask the coordinator for
  silence during a run, check every picture, and run again what lost the foreground.
- `Run-StartupTrace.ps1 -TestSave <sl2> -TestSaveSha256 <hash>` installs a test save and restores Thomas's save by hash.
- Never start the Run-StartupTrace job in one tool call and drive the game from another: a Start-Job dies with the pwsh that
  started it, so the restoration never runs (2026-10-17; recovery: `Restore-StartupTrace.ps1 -Baseline <run>/baseline.json`).
  Use the one-process runners (`Run-LiveWitness.ps1`, `Run-AlphaCast.ps1`).
- A flag file put by hand in a build folder is removed by `Run-StartupTrace.ps1` before the launch (its stale-flag cleanup): a flagged copy of a build folder runs without its flag (I-0113 tranche 22: tranche 21's three A/B runs were so). Pass flags with `-EffectFlags` (Run-AlphaCast, Run-Endurance, Run-I113Barrier take it) and check the run's `baseline.json` `effect_flags` or a row only the flag writes.
- `Run-StartupTrace.ps1 -EffectFlags` has a `ValidatePattern` allow-list (two groups): a new `*.flag` name must be added to
  both first, or the Start-Job inside `Run-ZoneWalk.ps1` dies silently at binding (no artifact folder, no game, the title pilot
  waits 240 s).
- A boss through its fog gate (I-0139, `Run-I139Boss.ps1`): the teleport pins the player for its whole hold, so keys pressed inside it do nothing: a 6 s hold 2 m before the fog, no camera sweep after it (the character turns away), a 300 ms step forward, then E (« Traverse the fog »). F6 on a boss with phases starts the next phase (Iudex: 1,054 HP to 0, then his second form at 1,037): one F6 a phase, a wait between. A Claude session that restarts kills its background bash waiters but not the pwsh runners they started: re-arm the chain, check the logs.
- `Run-ZoneWalk.ps1 -Teleport "x y z hold"` needs hold > 0 seconds (0 is rejected silently, `witness_teleport applied:false`);
  use 60-90 for a static witness.
- `Run-AlphaCast.ps1 -SaveCopy/-SaveCopySha256` takes another save copy; `-Continue` loads the last played character (Eta in
  `shebang-zones-eta-m40`). The Fire Keeper stands at a random spot each load: loop until `native_paint_drain` appears.
  Thomas's character copy `thomas-alpha-20261031-0851` (Crystal Magic Weapon, gap 17) holds the staff in the left hand: cast with
  `-Button right` (the default left click swings the sword: `i113-t14-crystal-*` cast nothing).
- Set `DS3_PILOT_SCREEN` (`\\.\DISPLAY1`) from the PowerShell tool, not from Bash: Bash drops a backslash and every pilot input
  then throws « not an attached screen », the game staying at the title (`i117-t0-on`, 2026-11-01). The names move with the
  screens: the ASUS is `\\.\DISPLAY2` while the OLED is on and `\\.\DISPLAY1` once Thomas turns the OLED off (2026-11-02, a series
  aimed at an absent screen): take the name from `scripts/Get-Screens.ps1 -Key 1920x1080@60` (as `Invoke-StartCheck` does) before each run.
- `reference_light_recording` (view context keys) needs `shadow-observe.flag`, not `light-observe.flag`.
- Before interpreting a measurement, check the run's `baseline.json` (`"measure": true`, `"debug_layer_disabled": true`):
  a runner passing `[bool]$x` through `Start-Job -ArgumentList` once ran five runs under the debug layer (I-0079).
- The native sampler writes only after 75,000 samples a thread: on a short path give `Run-I97Profile -EndMark`.
- The original-probe Reference mode installs only the pacer and world-gate hooks: test tools must hang there.
  `test-no-ai.flag` stops enemies attacking, but they still walk and may push the player off a measured path (Thomas, 2026-11-02);
  `test-freeze-ai.flag` (I-0114 tranche 8) also stops the moves of every character but the local player and a phantom-like one, and the AI step (DS3's own debug bytes 14477FEA0-A1, I-0114 RESULT « Eighth tranche »): use it for a
  measured path, but a grab still happens (series 39, I-0115 2026-11-02: the flesh mass on the cathedral hall's ceiling drops on the player, a grab, in both
  renderers with test-freeze-ai, so the 30 s `anorlondo-stairs-hall` path ends in a death; which system still acts under the
  freeze is not established; F4's refilled HP does not stop a grab, Thomas); do not sprint near ledges with enemies around. A path that needs the enemies' AI on (I-0114 series 36: Gamma killed on Anor Londo's stairs) runs with `test-tools.flag` and an F4 at its start: the player's HP refilled each frame, the AI unchanged (Thomas, 2026-11-01; one hit above the maximum HP still kills; `test-tools` is not yet among `Run-StartupTrace.ps1`'s effect flags, nor F4 among a path's keys: both to add the first time).

## Menus and input

- Keyboard AZERTY, ZQSD movement. The Shebang save (character Alpha, slot 0) is bound to Thomas's Steam account.
- Back to the last bonfire used without dying (a reload, as a death's or a zone change's): the item on the first quick-item slot at the bottom of the pause menu (Thomas, 2026-11-02, « Coiled Sword », not sure of the name: in DS3 the Homeward Bone returns to the last bonfire; check it on a screenshot).
- The options pages by keys (I-0132 R12f, `Run-AlphaCast -OptionsTour -OptionsSteps` and `-TitleSteps`): the arrows move the cursor (Right/Left step a choice row and a slider), E (vk 69) confirms and opens a value row's list; Enter does nothing. A Resolution or Screen Mode change opens the game's « Keep current settings? » with No chosen: Left then E keeps it. At the title the pages open from System (Down four times, E) but Esc does not close them there.
- Esc closes the whole in-game menu (no « back »). Some pages (bonfire Attune list, others) ignore Esc: a right click opens
  the command list (02_045_PC_CommandList), then click its « Back » entry. Equipment lists scroll only with the wheel or mouse.
- Pilot units are screen pixels (`Pilot-Game` SetCursorPos). Esc-menu icons at y 390: 263 Equipment, 329 Inventory,
  394 Status, 460 Messages, 525 System; System tabs at y 192 from x 323, 60 apart.
- Travel tabs at y 191, 60 px apart from x 263 (Archdragon Peak x 803, Kiln 863); a travel from the bonfire menu: `Run-AlphaCast -Continue -BonfirePages -TravelPress "803,191;400,280" -KeepSaveLabel ...`, then check the tab and the row on its pictures; `-LookSweep "n,dx,s"` turns the camera after
  arrival. The replay's L key does not turn the camera: turn with mouse `move` events (200x20 units in 8 s = one turn).
- Eta (the m40 zone save) holds every bonfire of the game and both DLCs: its Travel page's fourteen tabs, the zones' rows, and the
  tour's runner (`Run-R11cTour.ps1`) are in I-0132 R11-PLAN §12; the bonfires reached in R11c are kept as `eta-r11c-<zone>` test saves.
- Teleport targets: the MSB frame is the teleport frame (m30: four arrivals within 0.16 m of the MSB's player-start parts `c0000_*`);
  teleport to a `c0000_*` part or a SpawnPoint region (a Sonnet worker reads them with SoulsFormats, as Get-MsbGparamSlots.ps1 loads
  it), never to a map piece's position (its model's origin, not a floor: the 2026-10-30 falls). Bonfire objects are `o000100`.
- A zone's views through Run-R7aGi need its 150 s default: 60 s closed the game before the looks after a long zone load (I-0132 R11c).
- Eta (`-Continue` in the m40 zone save) saved at the Archdragon Peak bonfire: `shebang-zones-eta-m32-archdragon-20261027-172201`
  (SHA-256 B202E7934D05649FD22CDE718B878384D0D4F755A7E2709510EF90447FC6519D), made natively (`i100-t7-archdragon-save2`); in a
  Reference run the same `-TravelPress "803,191;400,280"` opened High Wall's tab instead (`i100-t7-archdragon-save`, its copy kept
  as `i100-t7-misfire-eta-highwall-*`): check `go-1.png` before keeping a travelled save. Eta's travel page opens on its last tab.
- Alpha (Shebang slot 0, Firelink, Fire Orb) is in every zone-save copy: Load Game, wheel up, first row; left click casts.
- Thomas's keys for spells (2026-10-28 22:55): Right arrow (the arrow block by the space bar) switches the right-hand catalyst
  (pyromancy flame, talisman, sorcerer's staff); Up arrow switches the attuned spell, pyromancy or miracle
  (`Run-AlphaCast -SpellsPerCatalyst n`). Alpha stands at Firelink Shrine in the save copies (no ledge near the bonfire).
- Thomas's keys for weapon arts (2026-11-17 09:01): the left Ctrl casts the weapon art; F takes the weapon in two hands
  (I-0140 tranches 19 to 22 tried Shift with the right button and missed the bows' arts).
- Extended keys (the arrows, Home..PageDown, Insert, Delete) need KEYEVENTF_EXTENDEDKEY, or the game reads the keypad's scan code:
  `Pilot-Game.ps1` sets it (i105-spells-1 had no effect without it). `-BootstrapMode Reference` refuses `-NoDebugLayer`.
- Gamma (m37 zone save) loads at Central Irithyll a step behind the bonfire: `-ApproachMs 400` before resting; its travel page opens on the Irithyll tab, Anor Londo at (367, 480). Gamma saved at the Anor Londo bonfire: `shebang-zones-gamma-m37-anorlondo-20261026-123818` (SHA-256 E96236897D815D1B1339FD49D3AA8D7308C185AA64AC9224A6F72A4638703411, Thomas's great stairs in view). The original-probe mode cannot be driven by the menu pilot (its window is out of sight): its rows come from replayed paths.
- The endurance tour's Load Game rows (Pilot-EnduranceTour's fixed y) may hold other characters before the tour's first quit (INFERRED): `-OnlyZones 'Gamma m37'` alone loaded an interior, not Gamma's bridge; and a missed load renumbers the visits (I-0113 tranche 21). To film a zone as the full tour shows it, replay its sequence (`Run-I113Barrier.ps1 -Zones 'Beta m31,Gamma m37'`) and wait for the zone's own « in world » line, not a visit number.
- A film for `scripts/video/videosheet.py` (WORKFLOW « Tools »): hold the camera still (or replay the same path in both renderers), keep in frame a control that moves all the time and should not show the defect (an animated character, foliage, snow), film the original the same way, and never film during a measure (the encoder's CPU). For a faint flicker film a few seconds lossless (`-c:v ffv1`); the encoder's keyframes make small jumps, which the tool sets apart. In a pair the cameras differ by a few pixels (7 at the Irithyll bridge): `--shift`.
- `Pilot-Game.ps1 -Action shot` writes JPEG under the .png name (ffmpeg needs `-c:v mjpeg` before `-i`): never judge banding or
  fine colour on run pictures; use the lossless `world-pictures.flag` readbacks (I-0113). `-Action hold` takes 5,000 ms at most: a
  longer walk is several `-KeyPath` steps (`Z5000,Z5000`).
- The Shrine Handmaid's chair at Firelink: Eta's m40 zone save, `-Teleport "254.8 -54.5 604.7 90"` then four `-300,0` looks
  (`Run-I113Chair.ps1`). Gamma at Anor Londo walks up the great stairs from its bonfire facing them; the bonfire is at
  (528.1, -146.9, -1085.1) in the teleport frame, the cathedral's entrance stairs near (589, -130.2, -1081.5).
  The great staircase hall behind the main building's side entrance (I-0113 gap 1, the grey walls): `-Teleport "611.5 -129.6 -1042.4 30"`
  (4.6 m back from the m37 MSB's `Env_Point353`; the MSB frame equals the teleport frame at Anor Londo): at the point itself
  (616.1 -128.6) Gamma is caught in the giant's rubble and the game reloads within a minute. The down arrow switches the Estus Flask and
  the Ashen Estus Flask (Thomas, 2026-10-31).
- Iudex Gundyr (I-0113 tranche 10d): the test launcher revives him (`-Bosses iudex`; in a run, `test-tools.flag` and `test-revive.txt`
  `14000800` in the build folder); from the Cemetery of Ash bonfire (about 103.5 -49.6 507.0) the fog gate is at 125.6 -64.1 558.6
  (`witness_player_position`, chr-positions.flag), E to traverse it; travelling straight to the Iudex Gundyr bonfire puts the player
  in the arena with a Gundyr who does not fight, and F6 then writes 0 HP without a death. On the OLED shots are 2560 wide (pilot
  units are its pixels). The test launcher's `-DiagnosticFlags a,b` needs `pwsh -Command "& '...ps1' ..."`, not `-File`.
- A second rest after a warp (I-0113 tranche 18, `Run-AlphaCast -SecondTravel`): at the arrival the « Rest at bonfire » prompt stays down until an input, even 15 s later; a 60 ms Z step, then E, rests. After a `-Continue` at a bonfire the same holds, and natively the prompt comes later: two 60 ms steps a second apart, then E (`Run-AlphaCast -ApproachMs 60 -ApproachKeys "90,60"`; one step rested the original but not the native, I-0113 tranche 41). In Travel, Firelink Shrine is the fixed bottom row at (380, 613) on every tab. The original's named oracle follows 2,000 draws of a name (about 2.5 s of a character's hair): open its window late with `-SecondTravelNamedAfter`. A Fire Orb cast with the camera turned to Archdragon Peak's cliff bursts in the sky, not on the ground (`i113-t18-orb-*`).
- Every spell (I-0113 tranche 11): `alpha-allspells-attuned-20261101-140409` (SHA-256 73840003C4ABA856959F4642D5F492D8F5BAF82E456B36388252EE4D67895349), Alpha-faith with the game's 106 spell goods (F7), saved by Quit Game at Firelink, attuned Profaned Flame, Seething Chaos, Lightning Stake, Lightning Arrow; `alpha-allspells-pyro-20261101-164410` (SHA-256 EF4634FDA017AC756EFF86A0F4002A902AFF1164ED5CBE12018ACDD2A8C1E786), the same with Profaned Flame, Seething Chaos, Flame Fan, Fire Orb (Seething Chaos current; Up: Flame Fan, Fire Orb, Profaned Flame) (Attunement 4; the pyromancy flame in the right hand). `-Continue` loads it. In Attune Spell the arrows move the list's cursor (Up at the top row scrolls), E confirms; a click on a spell selects it and confirms; a right click opens the command list (its Back at 790,508 leaves the list); the bonfire's Leave at (328, 373).
- The « item obtained » windows (the F7 give tool, any award) are closed with E, not Esc (Thomas, 2026-11-01): while they are up, Esc and the menu pilots do nothing, so a Quit Game after them fails (`i113-t11-give-native`).
- Pixel tests read the screen, not the game window: before the window is painted they see what lies behind (Steam's library
  left open showed the DS3 cover under the notice test). The title pilot confirms the notice twice and retries.
- PIX and the debug layer (I-0117 T7): right after a capture the layer reported acceleration structures as unbacked on calls the
  module was not making, and the game closed (`i117-t5j`, `i117-t7`, `i117-t7b`); since T7 the module does not turn the layer on
  with `pix-capture.flag` (the `native_debug_layer` row's `pix_capture`; the layer asked for beside it is refused). `Run-I117T0 -Stay 90` keeps the game open until the captures are written.
- A PIX burst at a spell's moment (I-0113 tranche 12): `pix-capture.flag`, `pix-f8.txt` in the build folder (« delay spacing » in presented frames: F8 then takes four captures, the first `delay` frames after the key; the periodic captures are then skipped) and `Run-AlphaCast -CaptureAt "k,-1"` (F8 sent just before cast k's click): for Flame Fan at Firelink, `100 8` caught the fan open. The pilot's keys land 0.5 to 1 s after they are sent, so a positive `-CaptureAt` ms lands after a short effect. The game freezes during each capture, so the run's video is useless there; read the captures with pixtool (`save-screenshot`, `save-event-list --counter-groups=D3D*`, `save-resource --global-id`, `export-to-cpp` for buffers). A script written through a Bash heredoc loses a backslash of `\\.\DISPLAY1`: build the name as `('\\' + '.\DISPLAY1')` in PowerShell.

## Screens and capture

- GDI screen captures on the OLED (Independent Flip) can be stale: take pictures on the ASUS (composed by Windows).
- While the OLED is on, the runs go there in a 2560x1440 window, its close box kept (Thomas, 2026-11-06 19:18-19:20: no fixed desktop):
  `-GraphicsLevels 'Resolution-WindowScreenWidth=2560;Resolution-WindowScreenHeight=1440'` (Run-AlphaCast, Run-I113Warp, Run-I100PopinSeries,
  Invoke-ReplayRun), `DS3_PILOT_SCREEN` from `scripts/Get-Screens.ps1 -Key 2560x1440@280`. Pilot-Game scales the positioned
  actions authored for the 1280x720 window (client at 8, 31) onto the game's client (I-0113 tranche 19; Pilot-TitleContinue scales
  its own and passes `-Exact`); not yet tried live when written.- The display is read with Intel's PresentMon 2.6.0 in `C:/Users/User/Tools/PresentMon`, unelevated (Thomas is in Performance
  Log Users; I-0098). NVIDIA FrameView's PresentMon needs elevation. Started without `--v1_metrics` from a hidden nested
  PowerShell (I-0100's LOD scale runner), it wrote no CSV in eight runs while the gate's (`Invoke-ReplayRun -PresentMon`, with
  `--v1_metrics`, its `msGPUActive` column) worked the same evening: keep the gate's command. A killed PresentMon leaves its ETW
  session (`logman query -ets`; stop your own with `logman stop <name> -ets`). The GPU's share without PresentMon: Windows'
  counter `\GPU Engine(pid_<pid>_*engtype_3D)\Utilization Percentage`, summed, once a second (unelevated; French decimal comma).
- Video memory counts in the process's private bytes on this PC: compare memory with `Run-Endurance.ps1`'s GPU counters
  (`Get-I96MemorySummary.ps1`). Never call HeapSummary (or any heap walk) in a periodic native row: it holds the process heap's
  lock (game steps over 34 ms 3.5 to 16 in the gate).
- Every D3D12 runtime error, even without the debug layer, faults the native run at once.

## PowerShell and tools

- PowerShell names are case-insensitive: in a series script, `$flags=...` inside the loop overwrote the `$Flags` parameter, so the
  first `-off` run's `no-rt-water` stayed on every later run (I-0125 `i125-r6c1`, `i125-r6c2`). Give a loop's own variable a distinct
  name, and check each run's `baseline.json` `effect_flags` before reading it.
- `Build-GameBootstrap.ps1 -ChangedUnits a,b` must be called through `pwsh -Command "& './native/frame/Build-GameBootstrap.ps1'
  -NoControls -ChangedUnits a,b"` (with `-File` the list binds as one string), on a folder whose full build ran with `-KeepObjects` (D-0260:
  a build deletes its objects otherwise). Passing arrays (`-ShotsMs`, `Run-I100PopinSeries -Runs`) needs `& script`
  in-process, not `pwsh -File`.
- PowerShell names are case-blind: never name a parameter `$Save` next to a variable `$save`. The same holds in a
  one-liner: `$P` and `$p` are one variable (a simulation went wrong that way, I-0097 tranche 6).
- A `-NotBefore HH:mm` given before midnight for a time after it was read as the ending day's and the series started at once
  (i97-t6d, 2026-10-25 23:59:57, five minutes before its announced start; stopped before the game started). `Run-I97Cadence.ps1`,
  `Invoke-ReplayGate.ps1` and `Run-Endurance.ps1` roll such a time to the next day; the older runners do not: give them a time
  of the same day, or check their first line before leaving them.
- `Start-Process` with a path containing spaces needs one quoted `-ArgumentList` string, not an array.
- `native/frame/Build-GameBootstrap.ps1 -Out` needs an absolute path: a relative one breaks its `build.cmd` (« 'evidence' n'est pas reconnu », I-0119 build-1).
- Edit C++ with the Edit tool, not through a Bash heredoc (a two-backslash character literal came out with one, I-0082).
- In a worktree, `Invoke-Ghidra.ps1` needs the `game/` folder: call the main checkout's copy with `-ProjectDir` of the mirror.
- The Reference census `reference_effect_pass_blends` (I-0113 tranche 30) stops at its 300,000th effect draw, a minute or so after the
  first effects: a spell cast minutes later is never in it; compare a burst at the cast with `effect-census.flag` (tranche 31).
- Thomas's save and its copies hold ten profiles: New Game opens « Cannot create any more profiles. Delete an existing profile...? »
  with NO highlighted (I-0113 tranche 31). For the creation screen use `Run-PilotPair -TestSave <copy> -TestSaveSha256 <hash>
  -DeleteProfile` (Thomas's yes, 2026-11-08): a profile is deleted in the installed copy only, restored away at the run's end.
- From Bash, `pwsh -NoProfile -File script.ps1 *> log` lets Bash expand `*` into file names (the script got the folder's names as
  its parameters, I-0113 tranche 31): redirect inside `pwsh -NoProfile -Command "& 'script.ps1' *> 'log'"`.
- The auto-mode classifier sometimes refuses `grep | head | cut` on evidence `.jsonl` files: use the Grep tool or PowerShell.
- Pair sheets (native row above original row) were made with a scratch `Make-PairSheet.ps1 -Names a,b`; not kept in Git.
- The Edit tool can rewrite a file's mixed line endings on lines it did not touch (I-0100, `05_selected_scene.inc`): check
  `git diff` for removed lines you did not mean before a commit.

## Pipeline cache

- No driver pipeline library since I-0113 (after the RT self-test its loaded pipelines sampled their static samplers as point
  samplers: the dark areas' banding and the glow in blocks, K-0486). The precompile list `native-pipelines.v3.psc` beside the
  module is compiled while the game boots at every start (227 pipelines in 40 ms, `i113-g4b2`). A fresh build folder has no list:
  its first native run compiles every pipeline at its first use (196 on the frame path in `i100-c5-oldlib-fix`), so its frame times
  are a cold run's. The older `native-pipelines.v2*.library*` files of a build folder are no longer read.
- A scripted edit (python, sed) whose new text ends with a `// comment` can swallow the rest of the original line (C++ and HLSL;
  clangd's `--check` missed it several times, I-0132 R12e 2026-11-12). Before a build: `git diff | grep "^+" | grep -E "//[^\"]*;" |
  grep -v "^+ *//"` and read every hit.
