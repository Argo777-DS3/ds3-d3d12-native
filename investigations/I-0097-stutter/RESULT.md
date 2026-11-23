# I-0097 result: the stutter (Saccades), tranches 5 and 6

## Tranche 6: the display's grid (2026-10-25/26, abandoned, D-0219; its code on tag `archive/i97-t6-display-grid`, not merged)

1. **Thomas's direction** (CADENCE-STUDY §A.7, his go 2026-10-25 19:57 and 21:03, relayed by the coordinator): at the end of our
   `EndFrame` (the production hook the module already owns), wait for a deadline of the display's grid; no new hook; DS3's limiter
   never modified, left as the cap at 60; V-Sync, G-SYNC and AMD without a driver setting; the logic stays at 60 frames/s
   (temporary, V2 unlocks it). Test: the Settlement's camera turn on the ASUS, frames held over 1.5 refreshes from 2-3 a turn
   to at most 0.5, the animation speed equal to the original's; the gate on both screens before the merge.
2. **Baseline** (`main`'s pacer, runs `i97-t5k-settlement-asus-p1` to `-p4`, `Get-I97Cadence.ps1`, sweep segment): frames held
   over 1.5 refreshes 3, 2, 3, 3; the presents' phase in the refresh p10-p90 4.5-4.7, 7.9-10.1, 2.3-9.5, 8.5-13.0 ms; after the
   post DS3's limiter and frame start 12.4-12.5 ms (p50); game dt p50 16.67, p99 16.89-17.31 ms. The original (`-o1`): 0 held,
   phase 7.7-8.2 ms, dt 16.67 ms on every step.
3. **Design** (`DisplayGrid`, `native/renderer/frame_pacer.*`): a thread of the renderer waits each vertical blank of the
   output the game's swap chain is on (`IDXGIOutput::WaitForVBlank`, any vendor; the output re-read every 120 presents; the
   thread idles while the screen gives no grid, and shares a control block that its stop empties) and anchors the grid on the
   blanks, smoothed (an eighth of each sample's offset), stepped by the display path's nominal period (the anchor moves to every
   blank, so a nominal rate 0.1 % off the real one costs a few ticks). The engine thread, after our `EndFrame`'s post and DS3's
   `+0x48` stamp, waits until the first refresh at least (refreshes - 1/2) periods after its previous slot, with a
   high-resolution timer and a 1 ms spin; DS3's limiter, the next timeline task, measures 1/60 s from its own previous exit
   (A0) and finds it elapsed on a screen at or below 60 Hz. A frame past its slot after eight frames on time (an isolated
   spike) waits for the next refresh (V-sync's rule, as the original's blocking Present: one refresh repeated, then the grid
   again); any other late frame starts at once (DS3's limiter the only cap), so a load at or over one refresh runs at its own
   rate. The worker presented each frame as soon as it was recorded, with the refreshes a frame holds (the series of item 7; the
   presenting thread's points that followed were never measured). Only where `FramePacer::FitRefreshes` holds
   (V-sync on, the refreshes within 2 % of 1/60 s steps: 59.94-60 Hz, 119.88-120 Hz), a frame's refreshes at the nominal rate
   are not shorter than DS3's 1/60 s (within 10 ppm: a screen at 60.02 Hz keeps the pacer, A.7 point 3), the anchor is
   confirmed (four consecutive blanks one period apart on its grid: a variable-refresh screen, whose blanks follow the presents
   several nominal periods apart, never confirms; once confirmed, a lone late wake is ignored and three agreeing off-grid
   blanks re-anchor, another screen) and fresh (a blank and a visible present within 250 ms, the anchor at most 1 s old);
   elsewhere (the OLED at 279.96 Hz windowed, a VRR screen, 144 Hz, V-sync off, a hidden window, before the blanks confirm)
   the pacer paces as before. `no-display-grid.flag` keeps the pacer alone (the A/B). The game clock, DS3's requested rate and
   its stamps are not touched (D-0169). Unit control in `frame_pacer_test` (with the wake's jitter, a late wake, mixed VRR
   blanks, a nominal rate off the real one). Diagnostics behind `cadence-trace.flag`: the engine's grid wait and the grid
   thread's blanks (`Get-I97Cadence.ps1` compares them with the trace's own D3DKMT vblank thread); the frame audit row's
   `grid` field (frames waited, ms a frame, refreshes, period). Known limit, accepted: on a screen at exactly 60.000 Hz (or
   up to 10 ppm faster) DS3's limiter exit can ratchet up to the largest wake delay seen and spin that much a frame until a
   late frame resets it (bounded, as the original); the 59.99 Hz ASUS is not concerned.
4. **Step review** (a fresh Opus at high, `30ab58ee`): two high findings (the reader's pipeline failed on every grid run; an
   unconfirmed anchor kept the grid active, so a VRR screen at 120 or 240 Hz would have had it), two medium (a load over one
   refresh fell to 30 frames/s under the first late-frame rule; a screen faster than 60 Hz had the grid, against A.7), four low
   (the stale-flag cleanup, the reader without vblank marks, the 1/30 s mode noted, a diff artefact): fixed in `c0c651a0`; its
   check of the fixes (one medium: a load around one refresh still lost a third of its frames, the late-frame streak resetting
   on one frame on time; three wording and test-path lows) fixed by the eight-frame rule above (`33fd4839`).
5. **First control series, the grid never engaged** (`i97-t6c-settlement-asus-g1`, `-n1`, `-g2`, `-n2`, `-o1`, 2026-10-25
   23:10-23:27, ASUS; `Get-I97Cadence.ps1`): OBSERVED, the engine's grid wait 0.00 ms on every frame and the audit row's
   `grid` `[0,nan,0,nan]`: two defects. (a) The default constructor's `Frequency()` resolved to the class's own member, which
   read the still uninitialised clock: frequency 0, so no grid was ever configured (the unit control injects its own clock;
   it now checks the system one). (b) The first anchor, DXGI's frame statistics, is unusable on the composed ASUS: the
   `SyncQPCTime` the presenting thread read stayed unchanged for up to 30 s (`dxgi_sync_vblank` rows of `-g1`: 48 changes in
   165 seconds). Hence the vertical blank thread of item 3. The runs are therefore four of the current pacer: frames held over
   1.5 refreshes 4, 3, 2, 4 (the baseline's 3, 2, 3, 3), the original 0; game dt p50 16.67 ms in all, as the original. Thomas
   watched them: « la caméra ne semblait pas lente » before 23:13 (`-g1`, the pacer in fact), « un peu lent » at 23:16 (`-n1`,
   the pacer; its trace holds 3 frames in the turn).
6. **Step review of the vertical blank thread** (a fresh Opus at high, `bb242ad9`): one high (a period estimated from the
   waking thread's samples, its base one raw wake, was noisier than the 10 ppm rule: the grid would have been refused or
   flapped at 60.000 Hz and for seconds on the ASUS), two medium (a lone late wake re-anchored a confirmed grid; confirmations
   added up across a session, so a VRR screen's occasional single-refresh blanks could confirm it), five low (a timed-out stop
   left a thread a later start revived; the thread woke at 280 Hz where no grid can be; the smoothed anchor briefly in the
   future; a dead field and the tests without jitter; the reader's blank matching): all fixed (the estimate removed, the
   nominal period stepping, the 10 ppm rule on it; consecutive confirmations; three agreeing off-grid blanks to re-anchor;
   the shared control block; the idle thread; the tests extended).
7. **Second control series, the grid engaged** (`i97-t6d-settlement-asus-g1`, `-n1`, `-g2`, `-n2`, `-o1`, 2026-10-26
   00:05-00:22, ASUS; `66c67a48`): OBSERVED, the engine waited the grid on 1,192 and 1,189 of 1,197 frames (p50 10.9 and
   6.4 ms), the grid thread's blanks on the trace's own (offset p10-p90 0.00-0.02 ms), the audit row's period the ASUS's nominal
   16.6667 ms. Frames held over 1.5 refreshes: grid 10 and 4, pacer 4 and 1, the original 0; PresentMon dropped 8 and 1
   presents in the grid runs' turn, none with the pacer; presents' phase p10-p90 7.5-12.2 and 1.5-15.5 ms; game dt p50
   16.67 ms everywhere. The gap from DS3's `+0x48` stamp to its next game step stayed the pacer's (12.60 and 12.49 ms p50
   against 12.24 and 12.38): about 1.7 ms (`-g1`) and 6.1 ms (`-g2`) of it fell after our wait, so the grid did not set the
   engine's frame start alone; why (the premise of A.7 that DS3's limiter then finds its 1/60 s elapsed) was not read further.
   The drops in the turn (PresentMon rows, the sweep window; corrected by the final review, which found the first reading wrong):
   2 of `-g1`'s 8 were the present of a late frame (30-34 ms after the one before), replaced by the next present 5-7 ms
   later; the other 6 had neighbours 14-21 ms away; `-g2`'s one came after a 39 ms gap. Pairs of presents 0.3-2.5 ms apart exist only at the run's start (the title and loading). INFERRED,
   partial: a late frame's present overtaken by the next one within the same refresh explains a quarter of the drops; the other drops' cause is open.
   The presenting thread's points (at most one present a refresh, half a period after the blank, `58fa4158`) answered that
   quarter; built in part, never measured. Thomas found the runs long (the 80 s walk before the 20 s turn): the turn-only path
   `settlement-turn` (the turn twice, then four more) for a next series. An incident: the series' first launch started at
   23:59:57, five minutes before its announced start, 00:05 (the runner read « 00:05 » as the ending day's); stopped 7 s later
   before the game started, no run folder, the save untouched; the runners now roll a start time past midnight.
8. **Conclusion, the tranche abandoned** (Thomas, 2026-10-26 00:31 Paris, relayed by the coordinator: « Je suis d'avis de
   suivre la méthode moderne des jeux DX12 plutôt que de continuer de bricoler, après le chantier pop in »; D-0219): no gain was
   measured. The first series ran without the grid (two defects); the second, with the grid engaged and DS3's limiter left
   untouched, held 10 and 4 frames in the turn against 4 and 1 for `main`'s pacer. No third series. Nothing of the grid goes
   into `main`: its code and unit control, the reader's grid lines and the runner's `-Grid` and `-NoOriginal` options stay on
   tag `archive/i97-t6-display-grid` (`58fa4158`, branch `claude/amazing-neumann-f6cab9`, commits `30ab58ee` to `58fa4158`);
   `main`'s pacer is unchanged and its gate record stands. What merges: the studies (CADENCE-STUDY, POPIN-STUDY, I-0099's
   FIX-PLAN), D-0218's lighter gate, the start time past midnight in `Run-I97Cadence.ps1` and `Invoke-ReplayGate.ps1` (as
   `Run-Endurance.ps1` had), the turn-only path `settlement-turn` and the notes. For the next cadence workstream (ROADMAP
   « Cadence, méthode DX12 moderne », after the pop-in workstream, studied ahead on Elden Ring's D3D12 frame pacing and checked
   on DS3), OBSERVED: DXGI's `SyncQPCTime` did not follow the vertical blanks on the composed ASUS (unchanged for up to 30 s);
   a `WaitForVBlank` thread followed the trace's own blanks to 0.02 ms; with the engine waiting the blanks, the gap from DS3's
   stamp to its next game step did not change (1.7 to 6.1 ms after our wait), and the presents sent as soon as recorded lost
   8 and 1 in a turn on the composed window. Not established: why DS3's frame start did not follow our wait, and the cause of
   most dropped presents.
9. **Final review** (a fresh Opus at xhigh, on `main...claude/i97-t6-close`, no `native/` change): one high (item 7's cause read
   on the run's whole PresentMon rows, the title and loading included: corrected from the turn's rows), two medium (item 8's
   lessons beyond the data; STATE's gate sentence), nine low (the studies' « both screens », D-0204 and D-0209 pointing to D-0218,
   the gate script's header, the OLED added and never instead of the ASUS, `-Memory` refused with `-Evaluate`, RESULT wording,
   the archive tagged, the notes, the memory threshold's source): fixed in `e5402f16`, except a shared start-time helper for
   the older runners (historical; the pitfall is in LIVE-RUN-NOTES). Its check of the fixes: no high or medium; its lows fixed
   (the dropped present is the late one, D-0218's clarification dated, this item).

## Tranche 5, first step: the GPARAM listing (G6, 2026-10-25, branch `claude/i97-tranche5`)

1. **The cause of the sprint's extra slow frames** (the study [TRANCHE5-STUDY](TRANCHE5-STUDY.md) §8, verified in the code;
   K-0482): not the sprint. DS3 reloads the same map parts every 10 s cycle and both renderers stall twice at each reload; the
   second stall is 24.2-24.8 ms in the original at every reload, natively it grew from 19.3 to 28.4 ms over the path's twelve
   reloads and crossed 25 ms at the ninth, the first sprint cycle. What grew is the GPARAM bank publication
   (`PublishNativeGparamBanks`): a retired GPARAM was tombstoned but kept its entry in the bank folder's listing, so its next
   admission listed it once more, and `Banks::Load` walked every entry at each of the 20 publications a reload, the tombstones
   throwing inside `ReadGparam` (0.19 to 0.76 ms a publication; over the night endurance 0.1 to 0.75).
2. **Fixed (G6, `cecac0b3`)**: the bank folder's files set to exactly the current ones, a retired
   file tombstoned and unlisted (`ds3hostio::PublishFolder`, `Unlist`, after the final review); the parsed banks are unchanged (the same current files, the same latest generation). Unit control
   `gparam_listing_test` (fifty reloads of ten files: one listing entry a file, the retired files neither listed nor loaded).
3. **Measured** (OLED, unsampled, measure mode): two control runs `i97-t5c-1-a`, `-2-a`: the publication flat at 0.13 ms over
   the twelve reloads (`main` 0.19 to 0.76), the second stall 18.1-19.4 ms (`main` 20.2 to 28.6; `Get-I97ReloadStalls.ps1`).
   Alternated series `i97-t5s1` (6 pairs, `main` = gate build `9dc735301519` against `t5-g6`; `Get-I97Series.ps1`): frames over
   25 ms at the frame end 22.7 to 17.2 a run (-5.0 standard errors), over 34 ms 3.2 to 2.5, game steps over 25 ms 20.2 to 15.2
   (-7.4: the first move of that count), over 34 ms 2.3 to 1.3; game step 1 % low 52.6 to 53.0 frames/s. The original: 12-14
   frames and 12-13 game steps over 25 ms.
4. **Review and gate**: the final review by a fresh Opus at xhigh found no high or medium finding; its four low ones fixed
   (`1b9c91b6`: the publication in one host step the test calls, the test's reads from memory, the tombstone skip removed as
   unreachable, a comment). **Gate `i97-t5-final` (2026-10-25 03:33-04:05): PASS on both screens.** ASUS: frames over 2.04 P 1
   (tranche 4: 4), over 1.5 P 17 (21), display_late 10.0 (17.5), game steps 1.5; OLED: 2.5 (3), 19.5 (21.5), 9.8 (10.8), 3.5
   (2); memory -1 MB a visit; the exit heap fault 0xC0000374 at the memory leg's close (DS3's own, I-0099).
5. **Next**: D-0207's recalibration of `main`, then the replayed paths of D-0213 (TRANCHE5-STUDY §2-§4), then I-0099.

## Tranche 5, second step: the replayed paths beyond Firelink (2026-10-25 05:15-07:30, in progress)

1. **D-0207's recalibration** (`i97-t5-maincal`, merged `b8004080`): frames over 1.5 P ASUS 36 to 27, OLED 34 to 27; the other
   limits kept (three rises refused).
2. **The Firelink path's « turn » turns nothing** (OBSERVED, `i97-t5-recon-firelink-turn`, the ASUS): the L key held 5 s leaves
   the camera still (six identical pictures); the gate has never measured a camera turn. Changing the gate's path changes its
   thresholds: left for a decision. The new paths turn the camera by the mouse (200 moves of 20 units in 8 s: one turn).
3. **Pictures need a composed screen**: on the OLED (Independent Flip) a GDI screen capture can return a stale picture
   (`i97-t5-recon-*`, INFERRED); on the ASUS it follows the game (`i97-t5-recon2-*`). `Invoke-ReplayPath` takes `shot` events
   (reconnaissance only), `scripts/Capture-ScreenSeries.ps1` a picture every 2 s.
4. **The paths** (`scripts/replay/ariandel.path.json`, `settlement.path.json`): Ariandel (Epsilon): walk 3 s out and back x8,
   sprint 2 s x4, the mouse turn x2; the Undead Settlement (Beta): walk x8 and the turn x2, no sprint (the enemies push Beta
   off the line and a sprint took him over the edge, `i97-t5-prof-settlement-n2`; his position varies between runs, Thomas: a
   source of variance). Enemies: `test-no-ai.flag` sets DS3's debug byte 14477FE9F (a block of debug bytes 14477FE94-FEAB read
   by the AI target and move steps 1405CE840/1405CE8E0, INFERRED from the decompile) from the world update gate in both
   renderers; seen working by Thomas (the enemies move but no longer attack). Before it Beta died in every Settlement run.
5. **First numbers** (whole runs of 134 s, unsampled): Ariandel natively on the OLED (`i97-t5c-ariandel-1`, `-2`): 4 and 2
   frames over 25 ms, 1 and 0 over 34 ms. The Settlement on the ASUS without AI, both renderers (`i97-t5-noai-settlement-n2`,
   `-o2`, the sprint still in): over 25 ms 4 against 2, over 34 ms 0 against 1; during the 18 s camera turn 75 frames over 17 ms
   natively against 24, during the walk 99 against 134. The turn's native frames are 17-18 ms (225) or under 16 (96) where the
   original's sit at 16-17 ms (1,050 of 1,079): a cadence jitter, not heavy frames; PresentMon (`i97-t5-prof-settlement-n`) 5
   late displays in the 18 s turn against 3 in the 78 s walk (Thomas: « ça rame légèrement quand la caméra tourne »).
6. **Sampled** (`i97-t5-prof-settlement-n3`, the ASUS, frames over 18 ms against under 17 ms): the engine thread's profile is
   the same in both (`RtlQueryPerformanceCounter` 42 % and 40 %, DS3's own spin at EEDC13-EEDC57): the slow frames carry no more
   work, they wait. INFERRED: DS3's own frame limiter (its QPC spin) and the native pacer's deadline beat against each other on
   the composed 60 Hz screen; the original presents at once and locks to the refresh. A pacing item (D-0212), to measure on
   the OLED too before any change.
7. **Noted, not treated (Thomas: tranche 5 is the stutter only)**: native pop-in at the Settlement (none in the original on the
   same path), distant enemies flickering, the intermittent grey loading page; ROADMAP « Pop-in workstream ».
8. **Step review of `test-no-ai`** (D-0198, a fresh Opus at high, 2026-10-25 07:36): no high or medium finding. Lows fixed: the
   original probe's comment names the debug byte; the gate judges no run that logged `test_no_ai` (inconclusive). Left: the
   `test_no_ai` row carries no count (one row a reset, bounded).
9. **Gate `i97-t5-step2` (2026-10-25 07:48-08:33): PASS on both screens.** ASUS: frames over 2.04 P 2, over 1.5 P 19,
   display_late 16.9, game steps 2; OLED: the first two runs' frames over 2.04 P 4 and 7 (mean 5.5, limit 5), confirmed by two
   more (2 and 5): 4.5, 17.5, 10.3, 2; memory 1.7 MB a visit; the exit heap fault 0xC0000374 at the memory leg's close (DS3's
   own, I-0099). The native change is the test tool only (inert without its flag): the first pair's 7 is the path's variance.
## Tranche 5, third step: one clock (2026-10-25, branch `claude/quizzical-wiles-08d28d`, complete)

1. **Thomas's direction** (2026-10-25 08:52-08:54, relayed by the coordinator, on CADENCE-STUDY A): one clock, « l'écran et donc
   le pilote qui commande » (the NVIDIA driver and the screen): our pacer imposes no timer of its own and follows the
   presentation's back-pressure that the driver and the screen govern; correct without a forced driver setting (other machines,
   AMD); what depends on the NVIDIA settings is noted. The ASUS has no G-SYNC (fixed 60 Hz); the OLED has G-SYNC in full screen
   only; the driver caps at 277 frames/s; RTSS is installed and not running; NVIDIA's overlay frame counter is on in every OLED
   PresentMon run since 2026-10-24 05:38 (the gate's and the bases' conditions, `Get-I97Overlay.ps1`).
2. **Nsight Systems unelevated** (`nsys profile --trace=wddm`, 08:51, 3 s): the DXGI and DxgKrnl allocation and present events
   only, no vertical blank, no GPU queue, no context switch (« System profiling might have not been started correctly »);
   sampling and context switches need administrator rights. The timeline is therefore taken inside the module.
3. **`cadence-trace.flag`** (diagnostics only, both renderers): the screen's vertical blanks (a thread waiting
   `D3DKMTWaitForVerticalBlankEvent` on the game window's screen), DS3's game step and its dt at the world update gate, the
   native engine's frame end and post stamp, the worker's latency wait, pacer and Present, the original's pacer
   D5CB0; one row a kind a second on the performance counter, the counter and the tick count read together (the motion marks);
   PresentMon's QPCTime is on the same counter. Reader `Get-I97Cadence.ps1` (a synthetic run's numbers checked).
4. **`display-clock.flag`** (the A/B of Thomas's direction): no wait of our own before Present (`FramePacer::Choose`); where the
   screen's refreshes fit the cadence with V-sync on (59.94-60 Hz, 119.88-120 Hz: `FramePacer::DisplayPaces`) the engine thread
   waits the swap chain's frame-latency object before DS3 stamps its frame start (`WaitDisplaySlot`, 34 ms bound, a count
   handed to the worker), so the engine's frames follow the display's retirement of presents as the original's blocking
   Present makes its main thread do; elsewhere (the OLED windowed at 279.96 Hz, a VRR screen, V-sync off) DS3's own limiter is
   the only clock and the worker presents at once. The game clock is not touched (D-0169). The ASUS's measured refresh is
   16.669 ms (59.99 Hz, `i97-t5-step2-1920x1080-60-m1`'s displays): today's pacer steps at it while DS3's limiter steps at
   16.667 ms, the two clocks of the study.
5. **Step review** (a fresh Opus at high): one high finding (the latency count could drift when the engine and the worker read
   the shared flag at different moments: now an explicit hand-off), two medium (the bound, a claim in the comments), eight low;
   all fixed (`00d79cda`).
6. **The grey native title** (a visual defect, Thomas's agreement to diagnose it, 09:37): grey on every native OLED run this
   morning (`i97-t5b-ariandel-oled-n1` to `-n3`, `i97-t5r-ariandel-oled-n1` after a PC restart), never in the original; black
   with `no-pipeline-library.flag` (`i97-t5g-nolib-1`, seen by Thomas). The build folder's pipeline library and its precompile
   list were moved aside (Thomas's agreement, 09:39: `evidence/I-0097/pipeline-library-archive-20261025-0945`, hashes in its
   README), the library rebuilt from empty (`i97-t5w-main`, title black). For the pop-in workstream (ROADMAP): why a library
   gives a wrong UI pipeline although a pipeline's name is its whole description.
7. **The camera turn traced** (`Run-I97Cadence.ps1`, Settlement, the 20 s mouse turn, enemies without AI; runs
   `i97-t5k-settlement-*`, 2026-10-25 10:10-11:25; `Get-I97Cadence.ps1 -Segment sweep`): OBSERVED, two clocks natively: after
   each post DS3's limiter waits 12.2-12.6 ms (p50) before the next game step, then our pacer waits 4-13 ms before Present. On
   the ASUS with the chain made there (`-LaunchOnScreen`) the current pacer holds 2-3 frames over 1.5 refreshes a turn (p1-p4),
   the original none (o1: 0 frames held, 0 game steps over 18 ms; its presents, timed when Present returns as the native's
   are, at 7.7-8.2 ms after the vertical blank p10-p90, p1's at 4.5-4.7: both locked in that turn, final review M4). On the
   OLED, counted against the content's period, the native holds 3 frames in the turn (oled-p1), the original none (oled-o1). The display
   clock's first two forms (the engine thread waiting the frame-latency object, with three then one present queued) never
   blocked (display wait 0.00 ms): DS3's limiter stayed the clock, the presents drifted across the refresh and 5-14 frames were
   held (d1, d2, dm1, e1, e2, em1). On the OLED (279.96 Hz) the vblank thread misses vertical blanks (a period of 4.7-5.5 ms
   measured): its phases are not read there.
8. **The turn's heavy frames** (OBSERVED): natively 1-5 game steps over 25 ms a turn on both screens in every variant, the
   original 0 (ASUS) and 0 (OLED). Sampled (`i97-t5k-prof-n1`, three slow frames, 40 samples): the engine thread waits its pool
   in `AdmitNativeDeferredCandidates` (09_model_admission.inc:1161, a third of the slow frames: the models the turn brings into
   view admitted at their first sight) and in the post (a fifth: the previous job held in the pacer's wait), DS3's limiter spin
   the rest. The admission part is the pop-in workstream's universal preparation (CADENCE-STUDY B, POPIN-STUDY T3/T5).
9. **The display clock's third form** (`t5-cadence3`, the worker waiting the frame-latency object with one present queued, no
   timer of ours; f1-f3, fm1, oled-f1): the worker's latency wait 0.00 ms too, 5-9 frames held and 3-8 dropped on the ASUS.
   INFERRED from the three forms: DS3's limiter steps 1/60 s from its own last frame start (12.2-12.6 ms after our stamp every
   frame), the ASUS's refresh is 16.669 ms, so the game is never more than a present ahead of the display within a run (one
   frame of drift takes about 8,000 frames) and no back-pressure wait can block; the display can only command once DS3's limiter
   no longer measures its own 1/60 s, i.e. its wait replaced by a wait on the display (CADENCE-STUDY A0: its function and
   reference stamp, in DS3's protected code at 0x144E79363; not done). **Decision for the tranche (the pilot, under the
   coordinator's supervision, Thomas away)**: no pacing change: the current pacer (its timer at the screen's exact period,
   re-anchored at a late frame: its presents' phase in the refresh p10-p90 spread over 0.2 to 7.2 ms in the ASUS turns p1 to p4) is the best measured form natively (2-3 frames held a
   turn against the display clock's 5-14); `display-clock.flag` removed (`cadence-trace.flag` kept, diagnostics only). Proposed
   to Thomas: A0, then DS3's limiter waiting on the display (his « one clock »), as its own step.
10. **The turn's first admissions counted** (the target for the pop-in workstream's T3/T5; `i97-t5k-admit-n1`, `main`'s build,
    the ASUS, `crossing-observe.flag`: collects whose admission took 3 ms or more): around the Settlement turn's start
    (collect serials 8489-8599, the turn starting near frame 8521, INFERRED: a serial counts frames) 15 such collects, about 400 fresh admissions, the collect's admission up to 26 ms (serial 8508),
    the slowest single admissions the map parts `m31_00_00_00_*` (4-9.4 ms each) and the characters c1100 (16.7 ms) and c1200
    (14.4 ms at the zone's start); during the walk five collects of 3.7-6.5 ms (14-16 fresh each). The remedy (prepare every
    loaded model before its first sight, D-0211, CADENCE-STUDY B) goes to the pop-in workstream (the coordinator, 2026-10-25 12:07).
11. **The gate's path with the real turn, recalibrated** (D-0215, D-0216; `i97-t5-turncal`, 12:21-13:10): the Firelink path's
    L-key segment replaced by the mouse turn of the study paths (path object b97181e7); means ASUS 2.5 / 17.75 / 13.63 / 2, OLED
    3 / 19.75 / 8.81 / 2 (frames over 2.04 P, over 1.5 P, display_late, game steps over 2.04 P); the formula's rises applied under
    D-0216 (ASUS display_late 21 to 22; OLED 5 to 7, 27 to 30, 15 to 16); memory -10.8 MB a visit.
12. **Gate `i97-t5-step3` (2026-10-25 18:57-19:29): PASS on both screens** with the real turn: ASUS frames over 2.04 P 2, over
    1.5 P 18, display_late 16.0, game steps 1; OLED 1.5, 17, 8.2, 1; memory 15.7 MB a visit (the criterion: the Theil-Sen slope
    of the title after visits 2 to 8 under 40 MB a visit, D-0206; the same native code gave -1, 1.7 and -10.8 in the three
    previous legs); the exit heap fault 0xC0000374 at the leg's close (DS3's own, I-0099).
13. **Final review** (a fresh Opus at xhigh, 2026-10-25 evening): no high finding; the native change inert without its flag, the
    raises recomputed equal to D-0216. Fixed: STATE and this section brought up to date (M1); a killed cadence run's flag removed
    by the next launch and a cadence-traced run inconclusive in the gate (M2); the ROADMAP's portrait note restated from the
    counts (M3); both renderers' present phases taken when Present returns, item 7 restated (M4); the display-clock remnants in
    the reader (L1); `Get-I97ReloadStalls` skips the mouse turn (L2); the reader's holds against the content's period (L10).
    Left for the next change of `native/` (diagnostics behind the flag; a gate needed): the marks reusing the counter reads
    beside them (L6), the writer's one-second spin (L7), the vblank thread's reopen rows (L8), the reserve before `on` under the
    lock (L9); noted: the scripts' shared run loop (L5), the gate's `original` records of the old path (L3), the sheet capture's
    270 s cap (L4). Optimisation pass (WORKFLOW): the branch adds one diagnostics flag (a relaxed load a mark when off) and no
    path beside an existing one (the display clock removed); its gate numbers are at or under the new calibration.
14. **The study bases per screen** (D-0213, `Run-I97PathBases.ps1 -Series i97-t5b2`, 2026-10-25 19:37-20:48, `main`'s build with
    the rebuilt pipeline library after one Ariandel warm-up, enemies without AI, PresentMon; the morning's series `i97-t5b` with
    the old library is left incomplete and superseded). Means a run per segment, native (n: OLED 4 runs, ASUS 2) against the
    original (o: OLED 2, ASUS 1), frames over 25 ms at the frame end / game steps over 25 ms / PresentMon late displays:
    Ariandel ASUS walk 2.0/1.5/3.5 against 0/0/1.0, sprint 1.5/1.5/4.8 against 0/0/1.0, turn 0.5/1.0/1.5 against 0/0/0;
    Ariandel OLED walk 1.5/1.5/1.9 against 0.5/0.5/1.2, sprint 1.2/1.2/1.9 against 0/0/0.1, turn 0.8/1.0/0.3 against 0.5/0.5/1.2;
    Settlement ASUS walk 1.5/1.5/4.6 against 0/0/2.0, turn 2.0/1.0/1.0 against 0/0/0; Settlement OLED walk 2.0/2.0/2.9 against
    0.5/0.5/1.4, turn 3.2/1.2/0.9 against 0/0/0 (`Get-ReplayMeasure.ps1`). The side-by-side sheets
    (`evidence/I-0097/sheets/i97-t5sheet-ariandel-1..3.jpg`, `-settlement-1..2.jpg`, the ASUS, 20:48-21:02).

## Conclusion (tranche 4, 2026-10-24/25)

1. **The band's cause, against the original** (K-0481): the 25-34 ms frames are DS3's own timeline barrier, the engine thread
   waiting for a loading step on a task worker in both renderers; that step cost natively 2,549 samples a walk against the
   original's 1,371, the native texture and model admissions inside it the difference.
2. **Fixed:** the texture container read under an SEH guard instead of VirtualQuery (G1), and the textures' uploaded content
   kept by its hash and found again when DS3 reloads it (G4, D-0211: 75 % of the path's texture admissions): the step 1,436-1,475
   samples; unsampled on the OLED (series `i97-t4s2`) frames over 25 ms 25.7 to 21.7 a run, over 34 ms 3.7 to 2.2, the game
   step 1 % low 46.8 to 52.5 frames/s (the original 51.0). Not fixed: frames over 25 ms stay above the original's 12-14 and game
   steps over 25 ms did not move (20.5 to 20.2); the model storages' copy inside the step (tranche 5).
3. **The pacer's one rule** (D-0212, Thomas): paced, then the screen's fitting refreshes, Present(1) elsewhere, Present(0) on a
   slower screen; the game's V-sync off by the launcher (`-NoVsync`; its .cmd was removed 2026-10-31, Thomas, the option stays) and the screen chosen at
   launch (`-Screen`). PresentMon (`i97-t4p`): ASUS 11.1 and 15.8 late periods, OLED 8.3 and 9.0, V-sync off on the OLED 8.8.
   Thomas's NVIDIA V-sync stays on over the game's (23:58), as G-Sync setups recommend. The swap chain re-created on a screen
   change was not done: tranche 3 found the chain made on the screen from the start changed nothing (`i97-t3d2`).
4. **Tried and dropped**: G3 (kept storage blocks, the step only 5 % lower for 128 MB kept), G2 (texture uploads on a queue: the
   step at the original's level but stand-ins bound, against D-0197).
5. **Gate `i97-t4-final3` (2026-10-25 01:03-01:40): PASS on both screens.** ASUS: frames over 2.04 P 4, over 1.5 P 21
   (tranche 3: 28), display_late 17.5 (limit 21; the control runs 11.1 and 15.8), game steps 1.5; OLED: 3, 21.5 (25), 10.8
   (limit 18), 2; memory -16.5 MB a visit; the exit heap fault 0xC0000374 at the memory leg's close (DS3's own, I-0099). Two
   attempts before were inconclusive: `i97-t4-final` (Windows Defender removed the gate's module at 00:29:53 as
   Behavior:Win32/DefenseEvasion.A!ml, a false positive on the injected module; Thomas: no exclusion unless it recurs) and
   `i97-t4-final2` (stopped: a comment placed mid-line by the final review's fix had swallowed the presenter's window, so its
   rate kept the start screen's 280 Hz on the ASUS, `bcffacf9`); every replayed run now checks its launch (WORKFLOW,
   `launch-check.json`). **Reviews** (D-0198): step reviews of G2 (its defect found, reverted), G4 (memory
   policy: users counted, budget trim, nothing kept after Close) and the pacer (120 Hz and slower screens); the final review by a
   fresh Opus at xhigh (WORKFLOW, Thomas 21:25): no high or medium finding; its low ones fixed (`fd926f66`: the guard's range,
   the pacer's fallback period, the decode counters, a flush) except the cache's unit test, a driver query repeated on a miss
   and the settled trim's possible firing on a long loading pause (to watch in the logs).
6. **Next (tranche 5)**: the storages' content cache (G5), class D (the first steps after a standstill), the replayed paths
   beyond Firelink (Thomas: a streaming zone, Archdragon Peak, Ariandel; ROADMAP), then the exit fault (I-0099).

## Findings (2026-10-24 15:15-23:12, branch `claude/i97-tranche4`)

On screen the native already shows fewer late frames than the original (PresentMon: 10-18 against 32-43 a run); the gap left
is at the engine: frames over 1.5 P at the frame end 22-31 a run against the original's 12-19, game steps 19-25 against 12-14.

1. **Where the band's frames go** (`i97-t4-prof-1`, `-2`: `Run-I97Profile.ps1 -Ds3Threads` on the OLED, 30 and 31 band
   frames, `Get-I97FrameProfile.ps1`; OBSERVED): the engine thread waits in DS3's task join (FUN_1423C1DC0 at +0x3FB polling the
   task system's lock 0x141799430: Enter/LeaveCriticalSection 11-12 % each of its band-frame samples against about 1.5 % in
   ordinary frames) while one loader thread runs `NativeSfxAssetLoad` (17-19 % of its band-frame samples, absent from ordinary
   frames): DS3's own FXR registration E6E570 -> the FXR CPU parser 2155F90/21561A0 and DS3's heap (1AA6xxx-1AB3xxx), the
   original's statement since W4; and GPARAM admissions (`AdmitGparam` 6.5 %, its `ReadGparam` 3.6 %, `PublishNativeGparamBanks`
   5.9 % with `Banks::Load` 4.75 %). The same joins exist in the original (I-0074, I-0079); INFERRED: the native's own work on
   the engine thread in those frames (the collect about 4 ms, `UpdateNativeSceneCpu` 19-20 % of the samples either way) pushes
   more join frames over 1.5 P than the original's.
2. **E1, a GPARAM parsed once** (`2dcdfb73`): `ReadGparam` parsed every GPARAM only to count its values for a log row, then the
   banks parsed it again; removed. Two control runs (`i97-t4e1-a`, `-b`): frames over 1.5 P 24 and 30 against `main`'s 22-28
   (`i97-t3-maincal-*280*`): no gain shown (kept as the removal of a duplicate parse, D-0198's rule; the gate will tell).
3. **The original profiled on the same path** (`i97-t4-orig-prof-1`, 2026-10-24 18:24, Reference with the sampler on its main
   thread and its 32 task and loader threads, `Run-I97Profile.ps1 -Original -Ds3Threads`; OBSERVED): the join is the original's
   too. The lane runner FUN_1423C1DC0 is DS3's timeline (its rows are barriers, ENGINE-STEP-THREAD-MAP); in the band frames the
   main thread polls the barrier as long as the native's (6.2 samples a frame against 5.9) while one task worker runs the
   loading step 509BCF (a lane-1 step; its sub-jobs wait on DS3's file and decompression thread). The DS3 part is equal (FXR
   registration 87 samples against 103). What differs is the step's whole cost over the walk: **2,549 samples natively against
   the original's 1,371**, its texture path D5209C 1,453 against 795, its model CPU load A0170 674 against 88 (the native
   admissions inside it: OBSERVED by return addresses, `Get-I97StepCost.ps1`). The collect is not
   the cause: overlapping it with the join is impossible (the join is at row 6, the collect at row 79 needs rows 16-72) and
   it grows only 2.4 ms in the band frames. INFERRED: the sampler itself lengthens the joined step (sampled, the original has
   30 frames over 1.5 P against 12-14 unsampled; the native 29-31 against 24-30), so sampled runs compare steps, not counts.
4. **G1, the texture container read under an SEH guard** (`792ec707`): `ReadableExtent`'s VirtualQuery of every container was
   586 of the 986 samples of the native texture admissions; the decode now runs under `GuardedEngineRead` (an access, guard
   or page-in fault refuses as before). Two control runs (`i97-t4g1-prof-1/2`): the step 2,514-2,549 to 1,818-1,837, the
   texture path 1,456 to 693-706 (under the original's 795).
5. **G3, the storages' bytes from kept blocks** (tried, dropped): a fresh allocation's first touch makes the copy 4 times
   slower in a micro-benchmark (2 MB: 0.35 ms against 0.08 ms), but two control runs (`i97-t4g3-prof-1/2`) moved the step only
   to 1,734-1,738 (the storage 435 to 364-405) for up to 128 MB kept in a zone: removed.
6. **G2, the texture upload on a queue of its own** (tried, reverted): the step fell to 1,266-1,382 (`i97-t4g2-prof-1/2`), the
   original's level, but the step review found a pending texture seen as absent (`Acquire` returns only resident records):
   the `_l` package or a SYSTEX fallback was bound and kept (a stand-in, against D-0197), the engine's size queries answered 0,
   and the waits meant for the stage textures and UI imports never waited.
7. **D-0211 (Thomas, 18:56, the modern method)**: the native part of streamed content prepared ahead and found in a cache in
   play, no pop-in, DS3's own loading not forced. **G4, the textures' content kept** (`a5cb4203`): each uploaded texture's
   resource and description kept under the XXH64 of its engine bytes (with its float metadata, size, name and family), 1 GB,
   released at the title; a reload of the same content gets a resident record at once, sharing the resource. On the path
   (`i97-t4g4-prof-1/2`): 7,489 found against 2,476 prepared (75 % of the texture admissions are DS3 reloading map parts),
   557 MB kept; the step 1,436-1,475 (the original 1,371), the texture path 279-297. Its step review (`ec8b9e40`): a kept
   content in use costs nothing more and only retired ones count against the 1 GB, by their placed size, the least recently
   retired released first; the video memory budget trims them before the heaps; nothing is kept once the ingress closes.
8. **A/B series `i97-t4s2`** (2026-10-24 22:28-23:12, the OLED, 6 alternated runs each, unsampled; `main` against G1+G4 with its
   review; OBSERVED): frames over 25 ms at the frame end 25.7 (sd 2.7) to 21.7 (2.4), -2.7 se; over 34 ms 3.7 to 2.2; game
   steps over 25 ms 20.5 to 20.2 (no change); pooled 1 % low at the game step 46.8 to 52.5 frames/s (the original 51.0,
   `i97-check2`), at the frame end 45.3 to 48.6. The DXGI display repeats rose 16,490 to 21,712 a run: DXGI's statistics are
   not reliable with two screens (K-0479), the display is judged by PresentMon. The band's frames still wait in the join as
   long (`i97-t4g4-prof-1`: 6.2 samples a band frame), now with fewer of them; frames over 25 ms stay above the original's 12-14.
   A first series `i97-t4s1` (21:44) started no run (the two builds passed as one argument).
# I-0097 result: the stutter (Saccades), tranche 3

## Conclusion (tranche 3, 2026-10-24): the native's presents on a 60 Hz screen

Thomas (2026-10-24 08:39: tranche 3 before the exit fault; 08:32 the screen chosen at launch; 08:34 the three screen modes).
Measured under PresentMon (I-0098), late display in frame periods over the replayed path (OBSERVED unless said):

1. **The cause.** On the ASUS (60 Hz, secondary, composed by Windows) the native dropped 110-125 frames a run and showed 75-101
   late periods against the original's 28-43: its pacer handed a 60 Hz screen to DXGI (Present(1) at once, the original's rule),
   so each Present left when the render worker finished (intervals 14-19 ms) and its display latency jumped between one and two
   queued frames, a frame dropped at each return (`i97-t3d2`). Ruled out by two runs each: two buffers without the waitable
   object (84.5, 63; `i97-t3d1`), the swap chain made on the ASUS from the start (101, 79; `i97-t3d2`, the screen chosen at
   launch), one frame in flight (86.7, 75.5; `i97-t3d3`).
2. **The fix (D-0210).** The pacer paces every frame by hand to its deadline, then presents with the DXGI sync interval where
   the screen is a whole multiple of 60 Hz (Present(1) at 60 Hz; Present(0) at 279.96 Hz as before); a synced frame that woke
   within 1 ms anchors at its deadline (step review: no slow drift), and steps by the screen's exact refresh period where
   known (final review: a 59.94 Hz screen would otherwise fall behind a 60.000 pacer every 16.7 s; not run, the ASUS is about
   59.995 Hz). Presents leave at 16.667 ms (p5-p95 16.6655-16.6683): ASUS windowed 12-17 late periods, 11-19 dropped a run
   (synced: `i97-t3d6-k1/k2` and `-t3m1-nw60` on the build anchoring at the wake, `-t3c1-nw60a/b` on the build anchoring at
   the deadline). Presented unsynced after the wait (`i97-t3d5`: 11.2, 12) did as well; the synced variant was kept so an
   exclusive full screen cannot tear. The screen-mode numbers below are from the build anchoring at the wake, except
   `i97-t3c1-nb280`.
3. **The screen modes** (`i97-t3m1`, `i97-t3c1-nb280`, one run each): ASUS borderless 7, full screen 10; OLED windowed 9.2,
   borderless 17.7, full screen 16.5; the original windowed 42.8 (ASUS) and 32.5 (OLED), full screen 33.4 (OLED). The native
   shows fewer late frames than the original in every mode measured; frame-end and game-step counts unchanged. Not measured:
   the original in full screen on the ASUS (its window opens on the primary screen before the pilot can move it).
4. **A borderless start on a screen larger than the configured resolution refused its views** (`i97-t3m1-nb280`: a 2560x1440
   OLED, a 1920x1080 configuration, `native_view_window_extent_changed`), pre-existing since I-0076 (borderless was tried on a
   1920x1080 screen only). The client a requested mode gives is now the views' start client: `i97-t3c1-nb280` starts and plays.
5. **The screen chosen at launch** (Thomas): `display-screen.txt` next to the module names a screen; the window is placed there
   before its swap chain is made (`Invoke-ReplayRun -LaunchOnScreen`). It works (`"placed":true,"refresh_hz":60`) but changes
   nothing to the drops; for the launcher (D-0182), a later step.

6. **The gate (`i97-t3-final`, 2026-10-24 13:30-14:02): PASS on both screens.** ASUS: display_late 12.8 (the base's
   calibration runs 74-114), frames over 2.04 P 3.5, over 1.5 P 28, game steps 3; OLED: 9.9, 1.5, 25, 2; memory +1 MB a visit.

## Next step (tranche 4)

The first steps after a standstill (class D: a model's first Programs build of 3.5-28 ms in the collect, to move off it after
ER's publish-when-complete), class B (56-60 ms, 0-2 a run), the 25-34 ms band (24-26 a run against the original's 13-14); the
swap chain re-created when the window changes screen (Thomas's idea, not needed by the drops); the launcher's screen choice.

## Evidence and reviews (tranche 3)

Runs under `evidence/I-0066/artifacts/`: `i97-t3d1-*` to `i97-t3d6-*`, `i97-t3m1-*`, `i97-t3c1-*` (each with `presentmon.csv`);
tool `Run-I97Display.ps1`; builds `evidence/I-0097/builds/t3-dev` (disposable). Step review (Opus, fresh): the anchor's drift
(fixed), a stale `display-screen.txt` (removed at a run's start), the unit test's boundaries (added), comments; not done: phase
locking to the screen's vblank (the remaining 11-19 drops a run may be the beat of two 60 Hz clocks; a lead if needed).
# Tranche 2

## Conclusion (tranche 2, 2026-10-24)

Thomas's direction: as modern games do, prepare the coming area ahead and off the engine thread so the engine never waits; for
the crossing frame, after ER without its defects. Done and measured on the replayed Firelink path (OBSERVED unless said):

1. **The crossing frame's cause.** A per-admission instrument (`crossing-observe.flag`, `native_admission_slow` rows: each
   worker's share, the slowest admission's model and its time per collect phase) showed that the crossing collect's +4 ms
   (7.5 ms against about 3) was one admission: a character's FaceGen face part (FG_A_0100) spending 3.8 ms in its texture
   phase, waiting on `face_image_lock` while DS3 rebuilt another character's face image (the FaceGen producer 8362C0 and the
   image's upload ran under that lock; `native_face_image_initialized` and `_updated` precede every crossing's slow admission,
   `i97-t2-obs-3`). E5 runs the producer, the pixels' copy, the upload and the composition outside the lock, the owners' hooks
   keeping their turns under `face_owner_lock` (the update runs on the FaceGen worker, the composition every frame on the main
   thread, which skips a frame while an update runs), and a recomposed face counts one more withdrawal once published.
2. **E5 against `main`** (`i97-t2c`, 8 alternated runs each, 02:59-03:58): frames over 34 ms 12.8 to 5.4 a run (-9.0 se; the
   original 3), crossing frames (34-50 ms) 12.3 to 3.9, their length 37.3 to 35.3 ms; display repeats 11.9 to 9.5 (-3.0 se); frame-end
   0.1 % low 27.0 to 29.9 frames/s (the original 29.9), 1 % low 45.0 to 46.0; frames over 25 ms 25.6 to 24.3 (-1.7 se, no gain).
   Game steps over 34 ms 1.4 to 3.0 (+2.7 se): `main` gave 3.0 in the previous series (`i97-t2a`), so probably noise, not shown
   to be (the gate's threshold is 6).
3. **Not kept.** E3 (the admission's shared locks: the texture registry and face lookups read shared, the scene copy once per scene,
   one thread per scene claiming a programme's scene tables) gave nothing on its own (`i97-t2a`: frames over 34 ms 13.1 and 13.0,
   crossing collect 7.63 and 7.53 ms); its changes stay, as E5's base and because they remove lock traffic, not as a measured
   gain. E4 (the layouts' Programs kept by content across reloads) had 4 reuses a run in its control runs and was removed: the
   slow Programs builds (3.5-28 ms for one model, `MeshPrograms`) are first builds, in the first steps after the standstill (I-0095
   class D, inside the gate's 2 s trim).
4. **Refused launches.** `pin-census.flag` (`i97-t2-obs-1`): two minutes into a normal run only two pins differ from the file
   since the setup, 0x810E90 and 0xE6E620 (the hooks' own patches counted apart): both now accepted when their bytes are the
   known decrypted code (21_effects.inc's quality prefix, the generated `loaded_sfx_scene_reset_code`); any other differing pin
   still refuses, and the refusal names each. Not yet seen live: a late setup that this accepts (the refusals came about 1 in 130
   runs).
5. **Load time against the original** (Thomas, 2026-10-23 18:14): Gamma m37 loaded from the title, 5 loads each, from the click on
   the save's OK to the first game step after it (`Get-I97LoadTimes.ps1`; the original runs with the module in Reference mode,
   `Run-Endurance.ps1 -Original -Probe`): native 6.71 s (6.64-6.75), original 6.62 s (6.56-6.66), about 90 ms more natively. A
   warp by bonfire is not piloted (not measured).

## Next step (tranche 3)

The first steps after a standstill (class D: collects of 25-30 ms when the walk starts, a model's first Programs build of
3.5-28 ms on one admission worker): build a model's Programs off the collect (posted when its layout is admitted, or at its first
meeting, and the model drawn from the first collect that finds them, D-0197's rule), after ER's publish-when-complete; class B
(56-60 ms, 0-2 a run) still unexplained; the 25-34 ms band at 24-26 a run against the original's 13-14.

## Evidence (tranche 2)

Runs under `evidence/I-0066/artifacts/`: `i97-t2a-*` (main against E3), `i97-t2-obs-1` to `-4` (control runs with
`crossing-observe.flag`, `pin-census.flag`), `i97-t2b-1-a` (the stopped E4 series), `i97-t2c-*` (main against E5),
`i97-t2-load-native`, `i97-t2-load-original`. Logs `evidence/I-0097/i97-t2*.log`; builds `evidence/I-0097/builds/t2-*`
(disposable). Tools: `Get-I97Crossing.ps1`, `Get-I97LoadTimes.ps1`.

## Reviews (tranche 2, D-0198)

- E3 (Opus, fresh): the pin census rebuilt (pins kept before the hooks, a hook's jump counted apart, read with Read); the scene
  copy from the record epochs; Get-I97Crossing counts a relaunched run once.
- E4 (Opus, fresh): texture choices per layout, a release queue, a periodic sweep; then E4 removed.
- E5 (Opus, fresh): one high-severity race found and fixed (the update and the composition share the status +0x70 from two
  threads: face_owner_lock); a recomposed face's withdrawal; the close's guard renamed (C4456).

# Tranche 1

## Conclusion

Complete for its first tranche (2026-10-23). The order relayed from Thomas was: correct the held count, recalibrate it, measure
with PresentMon if he allows its elevation, then I-0095's first lead. Done and measured:

1. The display's repeats are counted without bias, for the native chain (D-0208, gate threshold 24). The original's chain gives no
   statistics, so its screen still needs PresentMon (asked of Thomas, no answer yet: not run).
2. I-0095's first lead (the engine thread joining texture creation) has no target on this path: profiles of the slow frames show
   the engine thread creating no texture. It waits in DS3's task phases for a loading task, about half of which was native
   admission work.
3. That work was cut in two steps, each measured over 8 alternated runs a build. Frames over 25 ms went from 75.6 to 25.5 a run
   (the original: 13-14). Game steps over 25 ms went from 63.1 to 20.3 (the original: 12-13). The 1 % low went from 40 to 45
   frames/s (the original: 50).
4. Not moved: the area-crossing frame (about 13 frames over 34 ms a run against the original's 3), the band's remainder (25.5
   against 14) and the worst frames (56-65 ms, I-0095 class B). These are tranche 2: the crossing frame's native collect, where
   the engine waits for the admission pool's first admissions (FaceGen composition among them), and class B.
5. Launches refused at start: 4 in about 400 native runs, cause open; the gate and the series relaunch them (STATE, open gaps).
6. Load time against the original (Thomas's question): not measured. The endurance tour times the original with a fixed wait. The
   `game_step_resumed` row now dates a load's end in both renderers, but the original runs uninstrumented in the tour.

## Next step

Tranche 2 (its own thread): the crossing frame (I-0095 item 2: a first admission not joined by the engine thread), class B (a
profile at 45 ms), the kept-draw expiry period (item 4), and the original's screen with PresentMon if Thomas allows it.

## 1. Step 0: what reaches the screen (OBSERVED)

- The « frames held two refreshes » count (I-0095 §4: a sample kept only when DXGI PresentCount advanced by exactly one) is
  replaced by `display_refreshes` rows (`ds3native::DisplayRefreshes`, `native/renderer/frame_pacer.h`): per 120-frame window,
  the frames shown (PresentCount advance), the refreshes they occupied (PresentRefreshCount advance) and the repeated refreshes
  (the difference, summed per window), with the refresh rate. The step review moved it from SyncRefreshCount (the vblank DXGI
  sampled its clock on, which jitters) to PresentRefreshCount.
- The same code reads the original's swap chain in its probe (D5CB0's first argument holds the Present thunk's object, whose
  first field is the chain, K-0005/K-0007): the chain is windowed blt-model (`original_display_probe`: swap effect 0, two
  buffers) and `GetFrameStatistics` returns `DXGI_ERROR_UNSUPPORTED` (0x887A0004). The original's screen needs PresentMon
  (elevation: Thomas's decision, asked 2026-10-23).
- Native on the gate path (`i97-check2-native`, `i97-cal2-m1..m4`): 15-20 repeated refreshes a run while 70-87 frames exceed 25
  ms at the engine's frame end: the presentation queue absorbs most of the 25-34 ms band; the repeats sit mostly in the windows
  of the area crossing (about one a cycle). The game steps stay uneven (63-81 over 25 ms against the original's 12-13), and DS3
  steps its world with the measured interval, so the band can still show as uneven motion without a repeated refresh (INFERRED).
- Gate: `display_repeats` replaces `held` (D-0208), threshold 24 from `i97-cal2` (runs 15, 17, 15, 15); the other thresholds
  kept (means not significantly lower). The gate is inconclusive without three quarters of the display windows or off 60 Hz,
  and relaunches a run refused at start once.
- `frame-dump.flag`: every frame's time in the timing rows (both renderers); `Get-I97Lows.ps1`. Exact lows on the path
  (`i97-check2`): frame end 1 % low 40.0 frames/s native against 50.0 original, 0.1 % low 27.0 against 29.9, worst 38.6 against
  34.3 ms; game step 1 % low 40.3 against 51.0.

## 2. Where the slow frames' time goes (OBSERVED, `i97-prof-1`, `i97-prof-3`)

Sampler over the walk (75,000 samples a thread), the frames over 25 ms (86 and 111) against the frames under 17.5 ms (about
6,600), samples per frame (`Get-I97FrameProfile.ps1`, `Compare-I97Profiles.ps1`, `Get-I97ThreadBusy.ps1`; `i97-prof-3` with every
thread DS3 started, `sample-ds3-threads.flag`):

- The engine thread never creates or decodes a texture or a model in those frames (no TPF creator 140D4E100, no admission in its
  stacks). I-0095's item 1 as planned (the texture creation out of the engine thread's join) has no target on this path.
- The engine thread's extra time (about 6.7 samples a slow frame, 9.2 in an ordinary frame) is in DS3's task phases: the task
  join 23C1DA4 polling 23C21BB (+1.95), its critical section 179945F (+1.6 entering, +1.1 leaving), the phase sequencer 23C1330
  and its pending-count check 23C11A0 under their lock (+2.5, absent from ordinary frames), the native collect's admission
  (+0.6) and the pacer (+0.8).
- What those phases wait for: one DS3 worker runs a loading task (509BCF / 2347A82, under its own join) for 4.7 samples a slow
  frame against 0.3 in ordinary frames; about half of it is native work in the admissions it calls: `ValidContent`'s scan of every
  vertex and index (85 samples in 111 frames), a copy of the whole storage into a packet only to validate it (56), the storage's
  zero fill before its read (46), the static buffer's staging copy and placed creation (43), `ReadableExtent`'s VirtualQuery (19),
  GPARAM parsing (about 25), material building (15). The rest is DS3's own loading (FLVER CPU load, FXR registration, the
  texture registry), as in the original.
- Another DS3 thread (its file or decompression worker, 18B4DBA...) is busy 11.6 samples a slow frame against 1.5: DS3's own
  reload of the map part, which the original does too.

## 3. E1: the redundant native work out of the loading task (`819b8bbf`)

The storage validated in place (a span overload of `ValidContent`), its bytes not zeroed before the read (`UnsetAllocator`), the
same TPF container not queried again on the same thread (after its review: by exact address and extent).

A/B series `i97-s1` (2026-10-23 18:33-19:37, 8 runs each alternated, `Get-I97Series.ps1`; `step0b` = `main`'s code with step 0's
instrument; one E1 launch refused at start, relaunched): OBSERVED

| per run, mean (sd) | step0b | E1 | difference |
|---|---|---|---|
| frame end over 25 ms | 75.6 (7.8) | 45.6 (5.3) | -30.0, -9.0 se |
| frame end over 34 ms | 12.8 (1.2) | 13.4 (0.9) | +1.2 se, none |
| game step over 25 ms | 63.1 (10.4) | 37.6 (3.0) | -25.5, -6.7 se |
| game step over 34 ms | 1.1 (0.8) | 2.0 (1.3) | +1.6 se, none |
| display repeats | 14.4 (2.5) | 12.6 (1.8) | -1.6 se, none |

Pooled lows (about 60,500 frames a build): frame end 1 % low 40.0 to 42.9 frames/s, 0.1 % low 27.0 to 26.8; game step 1 % low
40.7 to 44.3. The original (`i97-check2-original`, one run): 50.0 and 51.0. The band halves; the crossing frame (over 34 ms) and
what the display repeats do not move.

## 4. E2: the model storage's GPU buffer and geometry check on a queue of their own (after ER, `cc5b3b60`)

ER publishes loading work when it is complete and its engine thread never joins it (I-0073 ER-STUDY); DS3's engine joins its
loading task, so the native part of that task moves out of it instead: `AdmitModelStorage` registers the storage at once and
posts its GPU buffer (staging copy, placed creation, ingress record) to an in-order `TaskQueue` (`StorageQueue`); the geometry
hook still reads the mesh slices from the engine's CPU views inside DS3's call and posts `ValidContent` and the publication of the
meshes to the same queue, after the buffer. A model met before its meshes are published is drawn from a later frame (D-0197's
rule, here for a model's geometry). ER's defect not copied: ER creates an upload buffer and waits on a copy per resource on its
loading threads (I-0079 ER-STUDY); the queue reuses pooled staging and waits for nothing.

A/B series `i97-s2` (2026-10-23 20:01-21:00, 8 runs each alternated, E1 against E2, no launch refused): OBSERVED

| per run, mean (sd) | E1 | E2 | difference |
|---|---|---|---|
| frame end over 25 ms | 42.9 (4.4) | 25.5 (1.8) | -17.4, -10.4 se |
| frame end over 34 ms | 12.9 (1.1) | 12.6 (1.1) | none |
| game step over 25 ms | 34.5 (2.1) | 20.3 (1.2) | -14.2, -17.0 se |
| game step over 34 ms | 2.9 (1.5) | 1.6 (1.1) | -2.0 se |
| display repeats | 12.9 (1.6) | 10.9 (1.4) | -2.7 se |

Pooled lows: frame end 1 % low 43.1 to 45.2 frames/s, game step 44.4 to 46.9 (the original 50.0 and 51.0). From the workstream's
start (`step0b`) to E2: frames over 25 ms 75.6 to 25.5 (the original 13-14), game steps over 25 ms 63.1 to 20.3 (the original
12-13). The step review's fixes (normal priority, the job's reference released before its count, withdrawn storages skipped,
every exception refused, drops logged, `geometry_pending` keeping queued storages out of the missing-geometry names, the queue's
backlog and post-to-publish delay in the registry row, the census read under the lock) are in E2b, not in this series.

## Uncertainty

- One path (Firelink), one PC (Ryzen 7 5800X3D, RTX, 60 Hz). The original's screen is not measured (blt-model chain).
- The sampler's frame marks attribute a sample to the frame that was running; the profile shows correlation of time with slow
  frames, not the chain of waits itself.

## Reviews (D-0198)

- Step 0 (Opus, fresh agent): SyncRefreshCount replaced by PresentRefreshCount; refresh rate in the rows; the gate inconclusive
  without display windows or off 60 Hz; the original probe's outcome row; frame-dump.flag in the stale-flag cleanup.
- E1: the readable-range cache keyed by exact container; the pin retry confirmed by a second pass (later removed).
- E2: normal priority (a below-normal thread could hold the registry lock the collect reads under); the job's storage reference
  released before its count; withdrawn storages get no buffer; every exception refused; drops logged; `geometry_pending` keeps
  queued storages out of the missing-geometry names; the queue's backlog and post-to-publish delay in the registry row; the census
  read under the lock.
- Final (whole branch against `main`): no high-severity defect, no D3D11 bridge, diagnostics behind flags or in Reference mode.
  Fixed: the pin retry removed (no benefit, i97-s1-1-b), the refusal naming the differing pins; the original's display probe
  stopping once unsupported; repeats per sample with shortfalls apart; the geometry job checking Close first; the backlog read
  without wrapping; the series requiring display rows; a stray line-ending change reverted. Not changed: the thresholds against E2's
  level (D-0207's recalibration on `main` follows the merge).

## Evidence

- Runs under `evidence/I-0066/artifacts/`: `i97-check-*`, `i97-check2-*` (step 0), `i97-cal-*` (refused), `i97-cal2-*` (the
  calibration), `i97-prof-1`, `i97-prof-3` (profiles; reports `sample-profile-{slow,fast,cross}-report.txt`, the raw samples of
  `i97-prof-1` deleted, D-0142), `i97-s1-*`, `i97-s2-*` (the series), the gate `i97-final-*`.
- Logs `evidence/I-0097/*.log`; builds `evidence/I-0097/builds/` (disposable cache).
- Tools: `investigations/I-0097-stutter/` (`Get-I97Lows.ps1`, `Get-I97FrameProfile.ps1`, `Compare-I97Profiles.ps1`,
  `Get-I97ThreadBusy.ps1`, `Run-I97Profile.ps1`, `Run-I97Series.ps1`, `Get-I97Series.ps1`).
