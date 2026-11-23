# The modern cadence with Reflex: ready plan (R1 tranche 2 and the cadence workstream)

Study thread, 2026-10-30 (Paris, evening), for the RT pilot. Read-only: `native/` at `main` 9ffa23a1 (with clangd's tree as the
reference for the line numbers below), [MODERN-CADENCE-STUDY](../I-0097-stutter/MODERN-CADENCE-STUDY.md) (form B, D-0240), I-0097
RESULT tranches 5-6, [I-0112 RESULT](../I-0112-unlocked-framerate-impact/RESULT.md), the Streamline 2.14.16 SDK on this PC
(`Tools/DS3-Decompilation/third-party/streamline-2.14.16`: `docs/ProgrammingGuideReflex.md`, `ProgrammingGuidePCL.md`,
`include/sl_reflex.h`, `sl_pcl.h`), D-0187, D-0210, D-0219, D-0221, D-0240. No run, no build, no Ghidra beyond the A0 listing
already in the Evidence Store. Statuses: OBSERVED (read here in code or a listing), SOURCE (the SDK's documents on this PC),
INFERRED, GENERAL. This file does not repeat the study; it says where each piece goes in today's code and what to settle first.

## 0. In short

1. **One frame-start wait, on the engine thread, at the end of our `EndFrame`** (`17_render_step.inc:519-520`, after the `+0x48`
   stamp, before DS3's limiter runs as the next timeline task). It waits the swap chain's frame-latency object (form B) with
   the study's deadline rule while the logic is locked at 60; the rule falls away by itself once the requested rate is above the
   screen's (the unlock). The render worker stops waiting the object (one waiter a frame); it keeps the frame ring's fence.
2. **DS3's limiter is never patched.** Locked at 60: the wait ends at or after its deadline `[clock+0x28] + [clock+0x18]·f`, so
   it finds its time elapsed and returns at once (its spin, 31-42 % of the engine thread, becomes our timed wait). Unlocked: the
   requested rate (`clock+0x354` float, `+0x358` enable, K-0019) is the player's limit or « Unlimited » (a high value), the frame
   start wait is the clock under V-Sync, the limiter the cap otherwise. The dt is DS3's own: it measures the real time between
   its passes (K-0019, median dt ≈ 1/rate), so a wait we add anywhere in the loop is in the dt, never against it.
3. **Reflex on Streamline's manual mode**: `slReflexSleep(token)` at the same point, every frame, in every mode (Off included,
   SOURCE); when Reflex is On it is the wait (D-0240: it takes our wait's place), our deadline rule still applied after it while
   the logic is locked (INFERRED: otherwise DS3's limiter spins again after the sleep). The PCL markers on the engine and render
   threads with the job serial as the frame index (the one DLSS already uses, `scene_pass_frame.cpp:617`).
4. **Present**: Present(1, 0) at once after recording where the frame-start wait is engaged (the pacer retired there),
   Present(0, `ALLOW_TEARING`) windowed and borderless with V-Sync off, Present(0, 0) in exclusive full screen; `ALLOW_TEARING` on
   the chain whenever the factory supports it. Maximum frame latency 2 kept (two frames in flight, D-0187), 1 as a measured
   variant (§4).

## 1. Today's loop, as the code runs it (OBSERVED)

Engine thread, one DS3 frame (kprofile's stamp order, `collect_profile.h:107`): `BeginFrame` hook (`0xD56780`,
`15_cpu_service_and_input.inc:342`: display cycle, presentation state, output and render extent, then DS3's `EEE130`/`EEE100`
phases) → update (the world update gate `8FA3E0`, `RecordGameStep`) → render step (collect, UI prepare) → `EndFrame` hook
(`0xD56930`, `17_render_step.inc:506`): `EEE190`, `Clear()` (`02_frame_lifecycle.inc:343`: builds the job, `++serial`, posts it
to the render worker; the post waits for the worker's previous job), the `+0x48` stamp (`:520`) → DS3's limiter (the next
timeline task, A0: `+0x20 = +0x28`, `+0x28 = QPC`, spins until `+0x18`·f has elapsed since its previous exit) → next frame.

Render worker, one job (`RunNativeFrameJob`, `02_frame_lifecycle.inc:320`): prepare (the world frame) → `WaitFrameSlot`
(`scene_pass_velocity.cpp:369`: the latency object, 100 ms bound, then the frame ring's fence) → `ExecuteFrame` (record, DLSS,
submit) → `PresentPaced` (`scene_pass.cpp:147`: `FramePacer::Pace` on a high-resolution timer and a 1 ms spin, then Present).

Chain (`backend.cpp:843-879`): `FLIP_DISCARD`, three buffers, the waitable flag, `ALLOW_TEARING` only under `vsync-off.flag`
or `original-unlock.flag` (`20_startup_phases.inc:90`), `SetMaximumFrameLatency(frame_slots_)` = 2
(`scene_pass.cpp:200`), made through Streamline's proxy factory when DLSS is ready (`backend.cpp:856-859`).

## 2. The frame-start wait (form B), piece by piece

| Piece | Where | What |
|---|---|---|
| The wait | `EndFrame`, after `:520`'s stamp and only when `Clear()` ran (not after `Close()`) | `renderer.WaitDisplayReady(deadline)`: waits the latency object (bounded 34 ms, the tranche-5 bound, and never while the window is hidden or iconic: `WaitFrameSlot`'s own test); a release that comes before `deadline − 0.5 ms` is let pass and the next one waited, once; then, if still before `deadline`, a timed wait to it (the frame pacer's `WaitUntil`, coarse timer plus the last 0.5 ms spun) |
| The deadline | read in `EndFrame` from DS3's clock (`image+0x489DD10`, already read at `:513`) | `D = [clock+0x28] + [clock+0x18] × [0x144929E00]`, QPC ticks; read only, the limiter not running (same thread). Engaged only when the requested period is not shorter than the display's (the logic locked): with the unlock `D` is always before the next release and the rule does nothing |
| One waiter a frame | `WaitFrameSlot` | skips the latency object when the engine waits it (a renderer switch set at start, as `SetPresentPacing`); the I-0101 `latency_debt_` bookkeeping moves with the waiter (a failed Present gives no count back) |
| Thread rule | `SceneRenderer` | the new entry is an engine-thread entry (`RequireThread(false, ...)`); the latency handle is replaced only by a chain re-creation, which runs from `BeginFrame` on the same thread (`FollowNativePresentationState`, `ApplyNativeDisplayRequest`), so no lock is needed beyond today's (INFERRED: the pilot checks with clangd that no worker path re-creates the chain) |
| Present | `PresentPaced` | where the wait is engaged, no `Pace`: interval 1 with V-Sync; the pacer stays for the screens the study keeps it on (non-fitting fixed rates, until T3 measures Present(1) at once there) |
| Where it applies | `PresentPaced`'s fit rule (`frame_pacer.cpp:42-47`) read on the engine side | V-Sync on and the screen fits 1/60 s steps within 10 ppm (59.94-60.000 Hz, 119.88-120 Hz); not on a VRR screen (the release follows the present), not with V-Sync off, not on 144, 165, 279.96 Hz while the logic is locked |
| Diagnostics | `cadence_trace.h` | two new kinds, `DisplayWaitStart` and `DisplayWaitEnd`, and the deadline as a value; the audit row's waited ms and the count of « release before the deadline » (the study's T2 controls read them) |

Why the end of `EndFrame` and not `BeginFrame`: the wait must come before DS3's limiter (else the limiter keeps its own phase,
I-0097 tranche 6) and before the next frame's input. Where DS3 samples input between the limiter and the update is not read (the
study's T0 (b), open): the end of `EndFrame` is before it whatever the answer.

The first frames: the object starts signalled up to the maximum latency (GENERAL), so the first two waits return at once; a
chain re-created on a resize or a screen change starts again the same way (no special case needed).

## 3. DS3's limiter and the dt (answering « neutraliser sans casser le dt »)

- **Locked at 60 (this workstream)**: nothing is written. The limiter's entry stamps `+0x20 = +0x28` (its previous exit) and
  `+0x28 = QPC`, then spins until `target − (+0x28 − +0x20)` is not positive (A0, `evidence/I-0097/artifacts/cadence-limiter-a0.txt`
  lines 120-140 and 230-275; the code is in the protected section, a thunk from 0x140EED8xx: no hook, D-0219's rule). Our wait to
  `D` makes that difference already ≥ the target, so it returns at once with `+0x28` = our wake. The dt DS3 hands the logic is
  its own measure of the frame: the screen's period on the ASUS (about 16.668 ms, D-0221 accepted subject to RT-9).
- **Confirmed before code** (the study's T0 (b)): done, §8 below. In short, the deadline is `+0x28 + T` with
  `T = [+0x18]·f` (f the counter's frequency `[0x144929E00]`), shortened only while DS3's catch-up (`+0x278`) runs; the dt the
  logic receives is the mean of the last three measured frame times, each clamped at 50 ms.
- **Unlocked (the next workstream, D-0240, I-0112 §6)**: the rate option writes `+0x354` (requested frames per second) and
  `+0x358` (enable), as `original-unlock.flag` does today in `BeginFrame` (`15_cpu_service_and_input.inc:359-360`), only when the
  option changes. « Unlimited » is a high requested rate (360 measured, I-0088; how high it may go is I-0112 §5.6's open point) and
  our loop: V-Sync on, the frame-start wait is the clock; V-Sync off or VRR, the requested rate or Reflex's `frameLimitUs` is the
  cap. The logic's dt-less sites of I-0112 §1 and §4 are fixed before any rate above 60 is offered.

## 4. Present, V-Sync, VRR and the latency (the pilot's (c))

| Screen and mode | Logic locked at 60 | Logic unlocked |
|---|---|---|
| Fixed 60 Hz, V-Sync on (the ASUS, composed) | frame-start wait with the deadline rule; Present(1, 0) at once | frame-start wait (plain back-pressure: the producer is now faster than the display); Present(1, 0) |
| Fixed high rate, V-Sync on (279.96 Hz OLED windowed, 144, 165) | no engine wait; DS3's limiter; `main`'s pacer until T3 | frame-start wait; Present(1, 0); the requested rate at or above the refresh |
| VRR (G-SYNC on the OLED in full screen; windowed only where the driver allows, D-0212) | no engine wait (the release follows the present); DS3's limiter; Present(1, 0) | the cap a few frames below the maximum refresh so the screen stays in its VRR range (GENERAL: NVIDIA's guidance for G-SYNC with V-Sync; Reflex On with V-Sync and G-SYNC applies such a cap itself, GENERAL); Present(1, 0) |
| V-Sync off | no wait of ours; DS3's limiter; Present(0, `ALLOW_TEARING`) windowed and borderless, Present(0, 0) in exclusive full screen | the player's cap (requested rate or `frameLimitUs`); the same presents |

- **`ALLOW_TEARING`** on the chain whenever `CheckFeatureSupport` allows it (`20_startup_phases.inc:90` sets it on request only):
  V-Sync can then change from the options without a new chain; the flag is never passed with an interval above 0 and never in
  exclusive full screen (`scene_pass.cpp:153` already checks the latter). SOURCE (the study's Microsoft Learn pages).
- **Maximum frame latency**: 2 kept (D-0187's two frames in flight are the ring's slots; the frame latency is DXGI's queue of
  presents, a different count: the study 6.3). With the engine thread waiting, latency 1 makes the frame start when the
  previous present is taken by the compositor, the lowest latency the loop can give; on the composed ASUS that taking comes a
  few ms before its blank (GENERAL), so with 8-10 ms of work latency 1 may turn on-time frames into held ones. Try 1 as a
  variant in the control runs, keep it only if the latency to display falls with no new hold (the study's T1 rule).
- **With Reflex On**, NVIDIA's table (SOURCE, ProgrammingGuideReflex §9): it reduces latency when GPU-bound, with VRR, or with
  V-Sync and frame generation; with V-Sync on a fixed screen and no frame generation it does not. On the ASUS at 60 with the
  logic locked, Reflex On is expected to change nothing measurable (INFERRED); its value is on the OLED (VRR) and once unlocked.

## 5. Reflex through Streamline (manual hooking), placed in the code

Today Streamline loads only behind `dlss.flag` (`streamline_host.h:2-7`, `featuresToLoad = {kFeatureDLSS}`,
`streamline_host.cpp:90-98`). Reflex needs:

1. **Loading**: `featuresToLoad` adds `sl::kFeatureReflex` (PCL is loaded by default, SOURCE); the interposer and `sl.reflex.dll`,
   `sl.pcl.dll` copied beside the module by the build (as `sl.dlss`), signature-checked as the others. Streamline initialised
   when DLSS or Reflex is asked (Reflex's option), still never when neither is. The chain made through the proxy factory whenever
   Streamline is up (Streamline's present bookkeeping, as for DLSS).
2. **Support and UI**: `slIsFeatureSupported(kFeatureReflex, adapter)` and `kFeaturePCL`; `slReflexGetState().lowLatencyAvailable`
   decides whether the Reflex option is shown (SOURCE: « the only exception »: the UI; everything else is called the same way).
3. **Options**: `slReflexSetOptions` at least once even with Reflex Off (SOURCE), then only on a change: `mode` = `eOff`,
   `eLowLatency`, `eLowLatencyWithBoost` (Thomas's three levels, RT-PT-PLAN §5, D-0240: an option, its default his call;
   NVIDIA's checklist asks On by default); `frameLimitUs` = 0 now, the unlock's cap later (driver-aware, works with Reflex Off
   too, SOURCE); `virtualKey` 0 (the ping by window message, below).
4. **The frame token**: one per frame, `slGetNewFrameToken(&token, &index)` with `index` = the job serial (DLSS already passes
   `input.serial`, `scene_pass_frame.cpp:617`): the engine thread asks the token for `serial + 1` at the end of `EndFrame` (the
   next `Clear()` takes `++serial`), the worker for `job.serial`. One index space for DLSS, Reflex and PCL.
5. **The sleep**: `slReflexSleep(token(serial + 1))` at the frame-start point of §2, every frame. Reflex Off: after our wait (it
   returns at once, SOURCE: it must still be called). Reflex On: in place of our latency-object wait (D-0240), then our deadline
   rule's remaining wait while the logic is locked (INFERRED, §0 point 3); the release-before-deadline skip of form B is not used
   then (the driver decides when the frame starts).
6. **The PCL markers** (SOURCE: always sent, whatever the mode), each with its frame's token:

| Marker | Thread | Place in the code |
|---|---|---|
| `eSimulationStart` | engine | right after the sleep, end of `EndFrame` (frame `serial + 1`) |
| `eSimulationEnd` | engine | `EndFrame` entry (`17_render_step.inc:507`), before `Clear()`: the update and the render step are done (frame `serial + 1`, the serial `Clear()` is about to take) |
| `eRenderSubmitStart` | render worker | `RunNativeFrameJob` before `ExecuteFrame` (after the slot wait), `job.serial` |
| `eRenderSubmitEnd` | render worker | after `ExecuteFrame` returns, before `PresentPaced`, `job.serial` |
| `ePresentStart`, `ePresentEnd` | render worker | around `swapchain->Present` in `PresentPaced` (`scene_pass.cpp:154-155`, next to the cadence marks), `job.serial` |
| `ePCLatencyPing` | the window's thread | on `PCLState.statsWindowMessage`, with the token of the next frame to start (`serial + 1` read atomically): a window subclass (`SetWindowSubclass` on DS3's window, from its own thread, `engine_window.h` `window_thread_id` says which) |
| `eTriggerFlash` | the window's thread | on a left button press when `flashIndicatorDriverControlled` (SOURCE): same subclass |

   Order per frame (SOURCE, the Verification HUD's rule: every start before its end): the engine's two markers come before the
   worker's (the job is posted after `eSimulationEnd`); the startup path's presents (`RenderStartup`, `scene_pass.cpp:270`) send no
   markers (no serial of a game frame).
7. **Shutdown**: unchanged (`slShutdown` before the device). The subclass removed before the window goes.
8. **Frame generation later** (RT-PT-PLAN §5) requires Reflex: the same token and markers serve it.

## 6. Order of work and controls (proposal)

1. **T0 (static, no code)**: done (§8). Left for the first control run: one journal row comparing the window's thread
   (`engine_window` snapshot `window_thread_id`) with the engine thread (`owner_thread`).
2. **T1, the chain**: `ALLOW_TEARING` whenever supported; present flags per mode (§4); nothing else (A/B against `main`).
3. **T2, the frame-start wait** behind `display-clock.flag` (the study's name), the worker's object wait removed under the same
   flag; unit control: the deadline rule with an injected clock in `frame_pacer_test` (a release before the deadline let pass
   once, the deadline met, a late frame, a hidden window's bound); two control runs on `settlement-turn` (ASUS, PresentMon,
   cadence trace), the study's T2 criteria (the wait engaged on ≥ 99 % of frames, the limiter's leftover wait under 0.1 ms p99,
   no hold without a late frame, step-to-display latency ≤ the original's + 2 ms, dt as the original's).
4. **T3, Reflex and PCL** (R1 tranche 2): loading, options, sleep, markers; checks: NVIDIA's Reflex Test utility is on this PC
   (`utils/reflex/ReflexTest.exe`, `PrintPCL.exe`) but needs the FrameView SDK installed and an administrator prompt (Thomas's
   call, WORKFLOW « Tools »); without it, Streamline's log and `slReflexGetState`'s frame reports (`latencyReportAvailable`, the
   per-frame sim, submit, present and GPU times) read in the journal: every marker present, in order, PCL not zero.
5. **T4, the other screens** (the study's T3): the OLED windowed, full screen with G-SYNC, V-Sync off, latency 1 as a variant.

## 7. Risks and open points

- The deadline ignores DS3's catch-up on purpose (§8.2): after a late frame the next frames wait for the display instead of
  running early; to watch in the control runs (the dt after a late frame, RT-9).
- The latency object's release timing in composition is not documented to the millisecond (GENERAL); the controls measure it.
- Reflex On with the logic locked at 60: the sleep and DS3's limiter both delay the frame start; the order « sleep, then the
  deadline » is INFERRED, to check in the control runs (the limiter's leftover wait stays under 0.1 ms).
- Streamline loaded for Reflex alone changes what runs on an AMD or Intel card: PCL runs there (SOURCE), Reflex reports itself
  unavailable; the chain through the proxy factory on those cards is untested here (no such card, ROADMAP's external testers).
- Elden Ring has none of this (no waitable object, no tearing flag, two spinning clocks: the study §3): nothing to copy there
  (D-0195's nuance), so no ER reading is needed.

## 8. T0, the static reading (done 2026-10-30 evening)

Sources: GhidraMCP on Thomas's Ghidra (DarkSoulsIII.exe 1.15.2, read-only): the tick `FUN_140EDAAE0`, the main loop
`FUN_140F14BB0`, the timeline run `FUN_140F0FC90`, the timing object's constructor `FUN_140EED360`; the limiter's body (in the
protected section, no function for the decompiler there) from the A0 listing (`evidence/I-0097/artifacts/cadence-limiter-a0.txt`)
and a new read-only linear listing of its exit, `140EEDD3B..140EEE140` (a pseudo-disassembler script, headless on the mirror
`ghidra-mirror-i0068`, `-readOnly -noanalysis`; the script is [tools/I0111LinearListing.java](tools/I0111LinearListing.java), run
through the main checkout's `Invoke-Ghidra.ps1` with `-ProjectDir` the mirror and the range as arguments). All OBSERVED unless marked.

### 8.1 Where the limiter sits in DS3's frame

The main loop `FUN_140F14BB0` calls the tick `FUN_140EDAAE0` while it returns true (between two ticks `FUN_140EF4B10` only
kills a window timer). The tick, in order: a protected call (`thunk_FUN_1451CEA9D`), `FUN_140EF2CE0` (the network poll timers
beside `SteamAPI_RunCallbacks`, I-0112 §4), the window object `[0x14489F118]`'s method `+0x10` (its result is the « keep running »
flag; INFERRED the Windows message pump, as Elden Ring's tick 140DEC1E0 « frame counter, message pump, the limiter, dt »,
I-0086 ER-STUDY §1; the object is the window: its `FUN_140EF5260` posts `WM_CLOSE` to its handle), **the limiter** on the
timing object `[0x14489DD10]` (the thunk `140EED810`), then **the whole timeline** through `FUN_140F0FC90` with a time object
`{vtable 0x1427457D8, dt = [timing+0x268]}`, then `FUN_140EA40B0` with `[timing+0x2BC]` (a frame-rate estimate, 8.3), then
`FUN_140EABBF0`, the frame counter `[0x14489D5C8]++`.

So, from the end of our `EndFrame` (the timeline's last native hook) to the next frame's work: the rest of the timeline, the
post-frame calls, the next tick's message pump, the limiter, then the timeline (`BeginFrame`, the update with the DirectInput
poll `FUN_14183BE70` inside the device update, NATIVE-INPUT-LATCH N5, the render step, `EndFrame`). **Every input of the next
frame is read after a wait placed at the end of `EndFrame`**: the window messages in the pump, the keyboard and mouse in the
update. The window's messages are pumped on the thread that runs the tick, the engine thread (INFERRED: a window's messages
are retrieved only by its creating thread; one journal row settles it, §6 point 1), so the PCL ping and flash subclass is
installed from the engine thread.

### 8.2 The limiter, entry to exit

- **Its period** (`140EED8B0..140EED9E9`): a mode switch (`[+0x8]` or `[+0xC]`, default mode 5 from the constructor) sets
  `+0x18` to 1/30 s (`0x3D088889`) or 1/60 s (`0x3C888889`); when `+0x358` (enable) is set, `+0x10 = 0` and
  `+0x18 = 1 / [+0x354]` (the requested rate; the constructor's default 30.0 with the enable off). `+0x35C`, when positive, ramps
  `+0x354` up by that much a frame, back to `[+0x2C0]` once past a constant (not used by the game as far as read). `+0x279` and
  `+0x27A` clear the catch-up counters.
- **Entry** (`140EEDA10`): `+0x20 = +0x28` (the previous exit), `+0x28 = QPC`; the target in ticks `R8 = [+0x18]·f`.
- **Catch-up** (`140EEDA85..140EEDB34`): the window `N` is `[+0x270]` while the window object says the game is active
  (`FUN_1423575B0`, the spin path) or `[+0x274]` otherwise (30 frames when `FUN_1408118D0` holds, the sleep path); the sum `S` of
  the last frames' durations is read from a 32-entry ring (`+0x60 + 16·i`: the duration in ticks, `+0x68`: whether it was a
  catch-up frame; index `+0x260`), stopping at the first entry not marked. When `+0x278` (catch-up on) and `S + T > (n+1)·T`,
  the target becomes `clamp((n+1)·T − S, T/3, T)`: after late frames DS3 lets the next ones run early, no faster than a third of
  the period, until the window's average is back at the period.
- **Off for one frame**: `+0x27B` non-zero skips the wait (`140EEDB59`) and is cleared at the exit (`140EEE098`): a one-shot,
  not a switch.
- **The wait**: in focus a spin (`QPC → +0x28` until `T − (+0x28 − +0x20) ≤ 0`, `140EEDBD0`); out of focus the same loop with a
  `Sleep` (`FUN_1417E8070`) when more than a constant remains (`140EEDC75..140EEDD3F`). **At the exit `+0x28` is the QPC read
  that ended the loop** (`140EEDC65`, `140EEDD49`): the deadline of the next pass is `+0x28 + T` (catch-up aside).
- **Exit** (`140EEDD63..140EEE098`): the ring gets this frame's duration `d = +0x28 − +0x20` and its catch-up mark;
  `+0x264 = min(d, 1/[+0x2C0])` with `+0x2C0 = 20.0` (the constructor): **a frame counts for at most 50 ms**; `+0x278` is
  recomputed (on while the window's frames are over budget); a 16-frame window of `d` (`+0x27C..+0x2B8`) gives
  `+0x2BC` = a frame-rate estimate (a constant over the sum; 0 when the sum is under a bound); the clamped `+0x264` is pushed
  into a 32-float history (`+0x2C4..`) and **`+0x268` = the mean of its last `[+0x34C]` = 3 values: the dt the timeline gets**;
  `+0x26C` = the mean of the last `[+0x350]` = 16. If `+0x360` is zero (a configuration byte `FUN_140813EA0(1)` sets at
  construction) or `+0x361` is set (a one-shot reset), the three are forced to 1/30 s.

### 8.3 What this changes in the plan

1. **The deadline** of §2 is `+0x28 + [+0x18]·f`, as written; it is the catch-up's upper bound. Using it ignores DS3's catch-up
   on purpose: under V-Sync the frames after a late one wait for their refresh instead of running early and queueing presents
   (the original's own catch-up on the composed ASUS is one cause of its queued presents, INFERRED). The dt stays true: it is
   measured.
2. **The dt** the logic receives is a three-frame moving mean of measured frame times, each clamped at 50 ms. So a late frame
   spreads over the next three dts instead of landing in one (the study's 6.2 point 4 reasoned on a single dt; INFERRED from
   the formula, RT-9 reads it). Under 20 frames/s the game slows (each frame counts for 50 ms at most), in the original too.
3. **For the unlock** (D-0240): the requested rate `+0x354` with `+0x358` set is the only lever needed; the smoothing and the
   50 ms clamp stay DS3's (a variable-step engine smooths its dt the same way, GENERAL); `+0x2BC`, the frame-rate estimate,
   feeds `FUN_140EA40B0` (INFERRED the dynamic quality scaler I-0112 §1 names at `140127240`, « a 60 reference »: to read in the
   unlock workstream). `+0x27B` is not needed (our wait makes the limiter's own wait empty without any write).
4. **No field is written in the cadence workstream**; reading `+0x18`, `+0x28` and `f` at the end of `EndFrame` is safe (same
   thread, the limiter not running).
