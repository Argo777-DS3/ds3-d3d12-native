# Study: the cadence by the modern D3D12 method (« Cadence, méthode DX12 moderne »)

> Copied into `main` on 2026-10-30 from its study branch, unchanged below this note (the study thread of I-0110/I-0112, D-0239).
> Since then (D-0240, Thomas 2026-10-30 08:21-08:29): the cadence takes the modern D3D12 loop, form B (the swap chain's
> frame-latency object), not Elden Ring's; the vblank grid (form A) is no longer compared; Reflex stays an option plugged into
> the frame-start wait, never the only clock; the workstream comes right after R1 (DLSS), before the unlocked frame rate and R2.
> The logic's unlock (§1 « V2 », §8 T4) is a variable step after I-0112's impact study, LTF and TRTF studied, not copied.

Read-only study (2026-10-26 04:45-06:00 Paris, branch `claude/zen-mendel-263329`), asked by Thomas through the coordinator
for the workstream D-0219 placed after the pop-in workstream (ROADMAP « Cadence, méthode DX12 moderne »). Written while the
pop-in pilot works on the same PC: no game launched, no build, no Ghidra; read: the code in `native/`, the repository's studies
(CADENCE-STUDY, I-0080 and I-0076 ER-STUDY, I-0097 RESULT tranches 5-6), the A0 listing of DS3's limiter, and the PresentMon
rows and cadence traces of the runs already in the Evidence Store (§5, read before 05:00, before Thomas's announced test).
Statuses: **OBSERVED** (read here in code, a listing or a run's rows), **INFERRED** (deduced, not measured), **SOURCE**
(read this session on Microsoft Learn), **GENERAL** (general knowledge, not checked here). Nothing is promoted to the ledger.
Fable 5.1 reviewed the plan on Thomas's agreement (2026-10-26 05:05-05:45 Paris, D-0198); its amendments 1, 2, 3 and 5 are
integrated below, its point 4 is Thomas's (§11). A copy for Thomas is in the project files (`MODERN-CADENCE-STUDY.md`).

## 0. The answer in short

1. **Why the tries failed** (OBSERVED, §5): DS3's limiter steps 1/60 s from its own previous exit and the ASUS refreshes about
   every 16.67 ms: the two clocks have the same period within 0.01 % (the latency to display drifts 0.5-1.3 µs a frame,
   §5.2). A wait placed **before** the limiter in the frame
   (tranche 6's, at the end of our `EndFrame`) cannot move it: in each grid run the gap from our wait's end to DS3's next game
   step stayed constant (1.6-1.9 ms p10-p90 in `-g1`, 5.9-6.3 in `-g2`, a different value a run): the limiter kept its own
   phase. A back-pressure wait (the frame-latency object, tranche 5's three forms) only paces a producer **faster** than the
   display by a margin; DS3's gains on the ASUS by about a microsecond a frame, so it never blocked within a turn.
2. **The six unexplained drops** (OBSERVED, §5.5): they are the composition deadline. On the composed ASUS a present needs to
   reach the compositor about 6 ms before its vertical blank; the grid presented each frame as soon as recorded, which put its
   presents 6-9 ms ahead of their blank, on that deadline. A present that crosses it is shown one refresh later (a frame held);
   from then one present stays queued (the display latency jumps from 7-9 to 22-25 ms) until two presents meet in one
   composition interval and the compositor discards the older (the drop). Five of the six drops end such an episode; the sixth
   is a miss replaced within the same interval. K-0480 described the same switch in tranche 3.
3. **What holds frames today** (OBSERVED, §5.4): with `main`'s pacer, 14 of the 16 frames held in six turns follow a present
   that came 21-50 ms after the one before: **late frames** (the CPU's first-sight work the turn brings, RESULT tranche 5 item 8),
   not the clock. Only 2 follow on-time presents. The original holds none and has no late frame. Neither renderer is locked to
   the display: both drift by 0.5-1.3 µs a frame against it; the original simply sits 8.5-10.4 ms ahead of the blank with
   ±0.2 ms of jitter.
4. **What modern D3D12 games do** (SOURCE and GENERAL, §2): flip model, a frame-latency waitable chain at latency 1 (2 when
   CPU plus GPU exceed a refresh), the wait **at the start of the frame, before input and update**, Present(1) at once after
   recording, tearing allowed for V-Sync off and variable refresh; with V-Sync on the display is the clock and any CPU cap stands
   aside. Elden Ring does not do this (no waitable object, no tearing flag, a spinning QPC limiter and a spinning frame-lag wait:
   two clocks, §3).
5. **The design proposed** (§6): the modern chain (T1), then one clock by **one wait at the end of our `EndFrame`, up to the
   first display slot at or after DS3's limiter's own deadline**, read (never written) from the limiter's fields `+0x28` (its
   last exit) and `+0x18` (its period). DS3's limiter, running next, then always finds its 1/60 s elapsed and returns at once:
   the engine's frame starts on the display's grid, the limiter's spin (31-42 % of the engine thread) disappears, its code, rate
   and fields are untouched. The slot's phase is chosen so each present reaches the compositor at least 10 ms ahead of its
   blank (the original's margin). VRR and non-fitting screens keep DS3's limiter as the one clock with Present(1) (the screen
   follows), V-Sync off presents with tearing. The display's event is either a vertical-blank grid (form A) or the swap chain's
   own frame-latency object under the same deadline rule (form B, the closer to the modern loop): T2 compares them (§11).
6. **The criterion « frames held ≤ 0.5 a turn » depends on the pop-in workstream**: most holds today are late frames, which only
   the universal preparation of loaded models (POPIN-STUDY T3/T5, CADENCE-STUDY B) removes. The cadence workstream's own
   criterion is therefore split (§8): no hold or drop without a late frame (the clock's part), and ≤ 0.5 in total once late
   frames are ≤ 0.5.

## 1. The fixed rules (Thomas, relayed 2026-10-26)

One clock, the display commands. DS3's limiter is never killed or modified (it sits next to the protected code and the game's
logic breaks above 60). The logic stays at 60 for now; V2 unlocks it (variable step with the LTF/TRTF fixes, then fixed step
with interpolation, CADENCE-STUDY A.7). Elden Ring's D3D12 architecture is copied with DS3's data verified on DS3, without ER's
defects (D-0195, I-0080); ER is never launched with DS3. No wrapper and no D3D11 fallback (D-0084, D-0096). It must work with
V-Sync, G-SYNC and on AMD, without any driver setting, windowed, borderless and full screen.

## 2. What modern D3D12 games do

### 2.1 The elements

| Element | Modern practice | Ours today (OBSERVED) | Elden Ring (OBSERVED in I-0076/I-0080 ER-STUDY) |
|---|---|---|---|
| Swap effect | `FLIP_DISCARD` (flip model; required for tearing and VRR, SOURCE) | `FLIP_DISCARD` (`backend.cpp:978`) | `FLIP_DISCARD` |
| Back buffers | 2-3 (GENERAL) | 3 (`20_startup_phases.inc:99`) | a caller's value, not recovered |
| Frame-latency waitable object | created with the chain; « wait before rendering each frame », including the first (SOURCE) | created, waited by the render worker at the start of its job (`scene_pass.cpp:1719`) | none |
| Maximum frame latency | 1 by default for a waitable chain, « the least possible latency »; 2 when CPU plus GPU exceed a refresh (SOURCE) | 2, the frame-slot count (`scene_pass.cpp`, `presenter_.max_latency=frame_slots_`) | none (the fence ring's lag 2 instead) |
| Where the wait sits | the start of the game frame, before input and update (SOURCE: the DirectXLatency sample's loop) | the render worker, a frame behind the engine | the main thread's frame-lag fence wait, spinning on `Sleep(0)` |
| Present with V-Sync | Present(1, 0) at once after recording (SOURCE, GENERAL) | our pacer's timer to its deadline, then Present(1) (`frame_pacer.cpp`, `scene_pass.cpp:850-866`) | Present(syncInterval, 0) |
| V-Sync off, VRR | `ALLOW_TEARING` on the chain whenever supported, Present(0, `ALLOW_TEARING`) windowed and borderless; the flag never with an interval above 0 (SOURCE) | the chain flag only when V-Sync off or the unlock flag is asked (`20_startup_phases.inc:109`), Present(0, `ALLOW_TEARING`) windowed (`scene_pass.cpp:856`) | no tearing flag |
| A CPU cap with V-Sync | the display is the clock; a cap at the refresh rate stands aside (GENERAL: Unity ignores its target rate when V-Sync counts; caps at the refresh rate with V-Sync are the classic beat) | DS3's limiter at 1/60 s plus our pacer: two clocks | ER's QPC limiter at 1/60 plus Present(1): two clocks |
| Frames in flight | 2 (GENERAL) | 2 (D-0187) | lag 2 |
| Present statistics | reliable in full screen and independent flip; not a pacing source in composition (GENERAL) | unusable on the composed ASUS: `SyncQPCTime` unchanged up to 30 s (RESULT tranche 6 item 5, K-0479) | not used |
| Vendor latency tech | Reflex, Anti-Lag 2: the same just-in-time wait at the frame start (GENERAL) | ROADMAP option, later | none |

Sources read this session: Microsoft Learn « Reduce latency with DXGI 1.3 swap chains »
(https://learn.microsoft.com/en-us/windows/uwp/gaming/reduce-latency-with-dxgi-1-3-swap-chains) and « Variable refresh rate
displays » (https://learn.microsoft.com/en-us/windows/win32/direct3ddxgi/variable-refresh-rate-displays).

### 2.2 How the modern method makes the display command, and its condition

With a waitable chain at latency 1 the wait returns when the previously presented frame has been taken by the display, so a
loop « wait, update, render, Present(1) » runs once per refresh and starts each frame just after a refresh (SOURCE: the sample's
comment, the application « will only render after each VSync »). This is back-pressure: the wait blocks only while a present is
still queued. It follows (INFERRED, and what tranche 5 measured three times) that **it paces only a producer faster than the
display**. A game whose own cap equals the refresh rate is never a present ahead, so the wait returns at once and the cap stays
the clock; that is why modern engines let V-Sync replace the cap rather than run both. DS3's cap cannot stand aside (§1), and its
period equals the 60 Hz screen's to 0.01 %: the modern method has to be adapted at that one point (§6).

## 3. Elden Ring (D-0195)

OBSERVED in the repository's ER studies (I-0076 ER-STUDY « Swapchain » and « Present »; I-0080 ER-STUDY §3, checked by Opus
there): `FLIP_DISCARD` windowed at creation, `MakeWindowAssociation(hwnd, 3)`, Present(syncInterval, 0) with flags 0 and the
occluded status accepted, no `ALLOW_TEARING`, no frame-latency waitable object; frames in flight held by a ring of six fence
tokens with lag 2, the main thread polling `GetCompletedValue` with `Sleep(0)`; a QPC limiter (140E84830, 1/20 to 1/360 s, 1/60
usual) spinning while focused, sleeping to about 5 ms before its deadline otherwise; a render worker that presents.
So ER has the same two clocks as DS3 and as our native (its limiter and the display), plus two busy waits. What we copy: the
flip chain, the presenting render worker, the window association, frames in flight by fence tickets (ours already retains per
resource, better than ER). What we do not copy (D-0195's nuance): the two clocks, the spins, no waitable object, no tearing.
No ER reading is needed for this workstream beyond these studies.

## 4. Our chain today (OBSERVED)

- **Chain**: `FramePresenter::Open` (`backend.cpp:969-1000`): `FLIP_DISCARD`, 3 buffers, `ALLOW_MODE_SWITCH`, the waitable flag,
  `ALLOW_TEARING` only when asked and supported, `SetMaximumFrameLatency(max_latency)` with `max_latency` the frame-slot count
  (2), `MakeWindowAssociation(NO_WINDOW_CHANGES|NO_ALT_ENTER)`; `Resize` keeps the creation's flags.
- **Engine thread**: DS3's frame step runs our `EndFrame` (`17_render_step.inc:505-519`: the clock's `+0x40` stamp, `Clear()`,
  whose post waits for the render worker's previous job, `02_frame_lifecycle.inc:450`, then the `+0x48` stamp), then DS3's limiter
  as the next timeline task (CADENCE-STUDY A.6), then the next frame; `BeginFrame` (`15_cpu_service_and_input.inc:327`) stamps
  `+0x30`.
- **DS3's limiter** (A0 listing `evidence/I-0097/artifacts/cadence-limiter-a0.txt`, reread here): at entry `+0x20 = +0x28` (its
  previous exit), `+0x28 = QPC` (0x140EEDA10-0x140EEDA26); the target is `+0x18` (1/60 s, 1/30 s, or `1/+0x354` when `+0x358` is
  set), shortened by its catch-up when `+0x278` is set; it loops `QPC -> +0x28` until `target - (+0x28 - +0x20)` is not positive
  (0x140EEDBD0-0x140EEDC70). So between two passes `+0x28` holds its last exit and its next deadline is `+0x28 + +0x18 × f`
  (f = `[0x144929E00]`), at most that when the catch-up shortens it.
- **Render worker**: prepare, `WaitFrameSlot` (the latency object, bounded 100 ms, then the frame ring's fence), record,
  `PresentPaced` (`scene_pass.cpp:850`): `FramePacer::Pace` waits to `anchor + step` on a high-resolution waitable timer and a
  1 ms spin, the step the display path's period when it fits 1/60 s within 2 % (the ASUS's path gives the nominal
  16.6667 ms), the anchor re-set to « now » after a late frame; then Present(1) (Present(0) without V-Sync or on a slower screen).

## 5. What the screen did in the measured turns (OBSERVED, a new reading of existing rows)

Method: PresentMon 2.6 rows of `DarkSoulsIII.exe` within each run's `segment_sweep` motion marks (the 20 s mouse turn at the
Undead Settlement, the ASUS, enemies without AI), mapped by `presentmon-clock.txt`; a « hold » is a displayed frame whose
`msBetweenDisplayChange` exceeds 25 ms; a latency step is `msUntilDisplayed` rising by more than 10 ms from one displayed frame
to the next. Runs: tranche 6's second series `i97-t6d-settlement-asus-{g1,n1,g2,n2,o1}` and tranche 5's baseline
`i97-t5k-settlement-asus-{p1..p4,o1}` (`evidence/I-0066/artifacts/`). The awk and Python lines used are in §10.

| Run | Renderer | Drops | Holds | Gaps before the holds (ms) | Latency to display p10 / p50 / p90 (ms) | Latency steps |
|---|---|---|---|---|---|---|
| t6d-g1 | grid | 8 | 10 | 35 7 5 34 14 17 18 17 17 17 | 7.1 / 9.0 / 23.0 | 5 |
| t6d-g2 | grid | 1 | 4 | 19 41 6 34 | 16.6 / 19.6 / 21.5 | 5 |
| t6d-n1 | `main`'s pacer | 0 | 4 | 21 32 25 50 | 9.5 / 11.5 / 11.9 | 1 |
| t6d-n2 | `main`'s pacer | 0 | 1 | 22 | 9.4 / 16.7 / 16.8 | 1 |
| t5k-p1 | `main`'s pacer | 0 | 3 | 23 32 30 | 12.1 / - / 12.3 | |
| t5k-p2 | `main`'s pacer | 0 | 2 | 22 22 | 6.7 / - / 9.0 | |
| t5k-p3 | `main`'s pacer | 0 | 3 | 23 26 21 | 14.4 / - / 14.5 | |
| t5k-p4 | `main`'s pacer | 0 | 3 | 18 33 17 | 14.7 / - / 22.1 | |
| t6d-o1 | original | 0 | 0 | | 9.9 / 10.1 / 10.4 | 0 |
| t5k-o1 | original | 0 | 0 | | 8.5 / - / 9.0 | |

### 5.1 One presentation mode for everyone

Every row of these runs is `Composed: Flip` with sync interval 1, the original's too (its D3D11 chain is converted by Windows'
optimisations for windowed games, which Thomas keeps on: INFERRED). The original's Present call lasts 0.04 ms (p10-p90): it does
not block in Present on this screen.

### 5.2 Nobody is locked to the display

The ASUS's displayed frames are 16.6706-16.6724 ms apart on average (whole runs `t6d-o1`, `t6d-n1`), the presents 16.6636
(original) and 16.6678 (pacer). The latency to display drifts steadily over the turn, by 100-frame means: original
9.74 → 10.36 ms, pacer `n1` 10.35 → 11.90, pacer `n2` 16.64 → 16.82: 0.5-1.3 µs a frame (the filtered means above are too coarse to give that difference; the drift is the measure). So the original
is not phase-locked either: it is a free clock whose presents jitter ±0.2 ms and sat 8.5-10.4 ms ahead of their blank in both
turns, far enough from the deadline of §5.3 for the drift (a few ms over minutes) not to cross it within a turn.

### 5.3 The composition deadline

A present displayed at its vblank was 5.7-9 ms ahead of it in the grid rows; when the next one would have been 5.7-6.2 ms
ahead it was shown one refresh later (latency 22.4-23.8 ms): on the composed ASUS a present needs about 6 ms of margin. The value
is one screen and one mode, read in PresentMon's display time; it is the compositor's (INFERRED) and differs in independent flip.

### 5.4 Today's holds are late frames

With `main`'s pacer, 14 of the 16 holds of the six turns follow a present 21-50 ms after the previous one (the frame was ready
4 ms or more after its deadline: CPU work, the turn's first admissions, RESULT tranche 5 items 8 and 10); 2 follow presents
17-18 ms apart (p4, whose latency rose to a queued 22 ms: its phase crossed the deadline). The pacer's own drift (it steps the
nominal 16.6667 ms against the screen's slightly longer refresh, 1.5 ms over `n1`'s turn) and its re-anchoring at « now » after each late frame
put its phase anywhere: `p2` sat at 6.7-9.0 ms, at the deadline, by chance without a miss in that turn.

### 5.5 Tranche 6's drops explained

The grid ran the engine on the blanks and the worker presented each frame as soon as recorded, so its presents landed 6-9 ms
ahead of their blank (`g1` p10 7.1): on the deadline. In `g1`'s turn the rows show the sequence five times: an on-time present
(gap 16.7-17.8 ms) arrives too late for its blank, the previous frame is held, the latency steps from 6-8 ms to 22-24 ms (one
present now queued), drifts there for 70-290 frames, and ends when two presents fall in one composition interval and the older
is discarded (rows 327 → 399 of the turn: latency 7.3 → 22.9 at the miss, 23.2-23.9 before the drop, 8.5 after). The drops at
rows 226, 399, 693, 866 and 1040 end such episodes; the drop at 109 is a miss replaced within the same interval (its successor
14 ms later); 17 and 24 are the late frames the final review identified. INFERRED: the grid's extra late presents (8 and 6 over
25 ms against the pacer's 2 and 1) come from its rule that a frame past its slot after eight on time waits for the next refresh,
which turns a frame 3-5 ms late into a 33 ms gap.

### 5.6 Why DS3's frame start did not follow the grid

From the `cadence_trace` rows of `g1` and `g2` (event times rebuilt from each row's first value and deltas; the game step taken
at the world update gate): the time from the end of our grid wait to DS3's next game step is 1.60 / 1.70 / 1.90 ms (p10 / p50 /
p90) in `g1` and 5.89 / 6.08 / 6.29 in `g2`, flat across the turn by 100-frame medians (1.8 → 1.6, 6.2 → 5.9: about 0.3 µs a
frame). OBSERVED: a constant offset a run, different between runs. INFERRED with A0 (§4): DS3's limiter measures 1/60 s from its
own previous exit `+0x28`. A wait that ends at slot `S(n)` before it leaves the limiter's deadline `+0x28 + 1/60 s` unchanged
whenever that deadline is later; the limiter then waits the rest itself, and its next deadline is again 1/60 s after that exit.
Its phase against the grid only changes by the difference of the two periods each frame, here a fraction of a microsecond. The
offset each run kept was set by its history (a late frame re-phases the limiter), about 1.4 ms in `g1` and 5.8 ms in `g2` beyond
the 0.3 ms of fixed work before the game step (INFERRED from the pacer runs' 12.4-12.5 ms post-to-step gap against the limiter's
12.2-12.6).

### 5.7 Why tranche 5's display clock never blocked

Its three forms waited on the frame-latency object (on the engine thread with three then one present queued, then on the worker
with one): 0.00 ms each time (RESULT tranche 5 items 7 and 9). By §2.2 and §5.2: the engine produces at the display's rate within
0.01 %. The presents gain on the screen by 0.5-1.3 µs a frame (the latency rises), so a present would stay queued only after
several thousand frames without a late frame, minutes; every late frame starts the count again. Within a turn back-pressure has
nothing to hold (INFERRED; `t6d-n2`'s latency of one queued refresh, 16.6-16.8 ms, may be such a bound state reached by chance).

## 6. The design proposed

### 6.1 The principle

The modern method's frame structure (the frame starts when the display is ready, Present(1) right after recording, a controlled
latency), with the display's slot as the event the frame starts on, because DS3's cap cannot stand aside. The wait is placed
where DS3's limiter can no longer win: **up to the first display slot at or after the limiter's own deadline**, so its loop
always finds its time elapsed.

### 6.2 The engine's wait (the one clock on fitting screens)

1. **Where**: the end of our `EndFrame`, after `Clear()`'s post and the `+0x48` stamp (the production hook the module already
   owns; tranche 6's place), so the limiter is the next task.
2. **Until when**: `D = [clock+0x28] + [clock+0x18] × f` (the limiter's next deadline, read only); the wait ends at the first
   slot `S >= D - tolerance` of the display's grid (the tolerance, about 0.5 ms, keeps a deadline a few microseconds past a slot
   from losing a refresh on a screen at exactly 60.000 Hz: the limiter then spins at most that). The limiter then exits at once
   with `+0x28 = S`, so from the next frame `D <= S(next)` on any screen at or below 60 Hz: the lock holds by itself; the
   limiter's code, rate, enable, catch-up and off switch are untouched, and its spin, 31-42 % of the engine thread (I-0084,
   I-0097 tranche 5 item 6), becomes our timed wait.
3. **The grid**: the vertical blanks of the window's output from tranche 6's thread (`IDXGIOutput::WaitForVBlank`, any vendor,
   followed the trace's own blanks to 0.02 ms, RESULT tranche 6 item 7), its rules kept (the period confirmed by four blanks, a
   variable-refresh screen never confirms, re-anchoring on three agreeing blanks, idle without a grid), from tag
   `archive/i97-t6-display-grid` (D-0195's « transform, not throw away »; reused as the comparison form only, D-0221).
   Slot = blank + phase offset (6.4). Form B (§6.7) uses the swap chain's frame-latency object instead of the grid.
4. **Late frames**: a frame whose `EndFrame` comes after its slot does not wait; the limiter exits at once and its next deadline
   is 1/60 s after that late exit, so the next frame waits to the slot after the following one: the late frame and its successor
   take three refreshes for two game steps (dt about 1.5 refreshes each, the game time still real time), one refresh held, then
   the lock is back (V-Sync's rule, as the original). T2's dt maximum is judged against the original's with that in mind.
   Tranche 6's eight-frame rule is not kept (§5.5).
5. **Where it applies**: V-Sync on and the screen's refreshes fit 1/60 s steps and are not shorter than 1/60 s by more than
   10 ppm (59.94-60.000 Hz, 119.88-120 Hz with two refreshes a frame: `FramePacer::FitRefreshes`' rule and tranche 6's 10 ppm
   rule). Elsewhere (§6.5), a screen at 60.02 Hz included, no engine wait.
6. **Consistency** (Fable, §2 of its review): `EndFrame` and the limiter are successive tasks of the same thread, so `+0x28` and
   `+0x18` are read while the limiter is not running; the wait replaces the limiter's spin at the same place and for the same
   duration, so DS3's other threads (the loading barrier of K-0481) see the same timeline. The final spin of our wait may drop
   from 1 ms to 0.5 ms.

### 6.3 The presentation (the modern chain)

- The worker presents at once after recording with Present(1) where the engine waits (6.2); `FramePacer`'s timer is retired
  there (its job, an even present spacing, is done by the engine's lock).
- The waitable object stays, waited at the start of the worker's job, as the bound on the present queue, **at maximum latency 2,
  today's value** (Fable's amendment 1). In composition the object is released when the compositor takes a present, a few ms
  before its blank, not at the Present call (GENERAL); at latency 1 the worker would then wait each frame until its previous
  present is taken, the more so as the design wants presents 10 ms ahead (6.4), and the engine's post, which waits for the
  worker's previous job, could inherit that wait: late frames made by the design. Latency 1 is tried in T1 only as a variant,
  engaged when `waitable_wait_ms` stays at 0 and `waitable_timeouts` at 0 (both already in the journal), and put to Thomas only if
  the latency to display then drops. It is the present queue, not the two frames in flight of D-0187, which stay.
- `ALLOW_TEARING` on the chain whenever the factory supports it (so V-Sync can change without a new chain), Present(0,
  `ALLOW_TEARING`) windowed and borderless with V-Sync off, Present(0, 0) in exclusive full screen (SOURCE).

### 6.4 The phase: presents clear of the composition deadline

The slot's offset from the blank is chosen so each present reaches the compositor at least 10 ms ahead of its blank (the
original's measured margin; §5.3's deadline is about 6 ms): offset = 10 ms + the p95 of (present − slot) over the last few
hundred frames, re-chosen only when it moves by more than 2 ms (hysteresis), from our own timestamps (the blank and the present
call), no PresentMon needed in play. The offset (18-20 ms with 8-10 ms of work) is taken modulo the period against a reference
blank named without ambiguity; each re-choice can hold one frame or queue one present for a frame, so T3 counts the re-choices.
In independent flip (the OLED windowed, full screen) the deadline is closer to the blank; the same rule only costs latency
there, to measure (T3). Form B needs no phase rule: the compositor's taking of the previous present is its event (§6.7).

### 6.5 The other screens and modes

- **Variable refresh (G-SYNC, FreeSync)**: the blanks follow the presents and never confirm a period: no engine wait, DS3's
  limiter is the one clock and the screen shows each frame as it arrives (Present(1); V-Sync on in VRR means no tearing below
  the screen's maximum). Our code needs no driver setting; but variable refresh in a window or borderless is only on when the
  driver allows it (NVIDIA's windowed G-SYNC; Thomas's OLED has G-SYNC in full screen only, D-0212), which is what the player
  sees, not what the design depends on.
- **Fixed rates that do not fit 60** (144, 165, 279.96 Hz): no grid; DS3's limiter and Present(1); 60 frames/s on such a screen
  repeats refreshes unevenly by nature (the original likewise); `main`'s pacer stays there until T3 shows Present(1) at once is
  as good.
- **A screen faster than 1/60 s by more than 10 ppm** (60.02 Hz): no engine wait; `main`'s pacer and Present(1), as tranche 6's
  rule and 6.2 point 5. (Corrected after Fable's review: with the engine's wait there, the deadline would fall after the next slot
  every frame, the tolerance absorbing about 90 frames before a skip: one frame held about every 1.5 s, not every 45 s as the
  first text said; 45 s is the beat of a free limiter with Present(1), today's behaviour there.)
- **V-Sync off**: no wait of ours; DS3's limiter; Present(0, `ALLOW_TEARING`).
- **Window hidden, minimised, moved to another screen, resized, loading**: the bounds of tranche 6 (no grid while it is not
  confirmed and fresh, a wait capped at one refresh past DS3's deadline, re-anchoring after a resize): the same renderer's own
  timeouts, not a fallback to another renderer (D-0084).
- **AMD, Intel**: DXGI's `WaitForVBlank`, the waitable object and QPC are vendor-neutral (GENERAL); no AMD card here, so unit
  controls and the external testers of the ROADMAP.

### 6.6 What changes for the game (D-0169)

Nothing is written into DS3's timing: the requested rate, its enable, the limiter's fields and the clock's stamps stay DS3's own
(`+0x48` is the native `EndFrame`'s as today). The game's dt becomes the display's period on fitting screens (about 16.668 ms on
the ASUS, the limiter measuring from its exit at the slot), as the original's on a 60 Hz screen within 0.01 %. RT-9 (the dt
distribution and the animation speed against the original) measures it; Thomas accepted that reading of D-0169
(D-0221, §11), subject to RT-9.

### 6.7 Form B: the swap chain's object as the display's event (Fable's amendment 5)

The same deadline rule with the frame-latency object instead of the grid: at the end of our `EndFrame` the engine waits the
object; a release that falls before `D - tolerance` is let pass and the next one is waited (once: the late-frame cost of 6.2
point 4). From then `+0x28` is the moment the compositor took a present and `D(next)` comes before the next taking while the
screen is not faster than 1/60 s: the same lock and no spin, without the vblank thread, the grid's confirmations or the phase
rule (the taking is the composition deadline, so the next present lands about where the original's do, INFERRED). Vendor-neutral;
the same code in independent flip and full screen (the object is released at the flip). To settle in T2: the release's timing
in composition (not documented to the microsecond, GENERAL), the count handed between the engine and the worker (tranche 5's
step review H1: one waiter a frame), its maximum latency (1 or 2, measured with the form), a bound when no present is in flight
(34 ms, as tranche 5; hidden window, loading). Not where the screen is VRR or does not fit 60 (the release follows the present).
Fable's view (AVIS): the form most likely to win, the simplest and the one that survives V2 unchanged (V2 removes the deadline
rule and keeps the loop).

## 7. Not proposed

| Option | Why not |
|---|---|
| Writing DS3's requested rate (« L-rate », CADENCE-STUDY A.6) or its off switch `+0x27B` | modifies the limiter: Thomas's rule (§1) |
| Hooking or patching the limiter's loop | its entry is a thunk into the protected section (A0); Thomas's rule |
| The frame-latency object as the clock without the deadline rule | measured three times not to block (§5.7); with the rule it is form B (6.7) |
| DXGI frame statistics as the anchor | unusable on the composed screen (RESULT tranche 6 item 5) |
| A wait before the limiter that ignores its deadline (tranche 6's grid) | the limiter keeps its own phase (§5.6) |
| Presents as soon as recorded without the engine's phase chosen | they land wherever the work ends, at the composition deadline in `g1` (§5.5) |
| RTSS, the driver's frame limit or V-Sync setting | a third clock; driver settings are Thomas's and must not be required |
| A wrapper, D3D11 or a translation layer | D-0084, D-0096 |
| The Windows composition swap chain API (`presentation.h`, target present times) | GENERAL: its D3D12 support and availability not checked; not needed for this design |
| Reflex or Anti-Lag now | later, on the same frame-start wait (ROADMAP options) |

## 8. Plan in short tranches

Every tranche that changes `native/`: behind a flag (A/B against `main`), unit controls, the step review by a fresh Opus at high
(D-0198); then **two control runs before any series** on the turn-only path `settlement-turn` (the ASUS, the chain made there,
enemies without AI, PresentMon), read at once against the tranche's « engaged » checks; only if both pass, the alternated series
(four runs each, the original once or twice); the final review at xhigh; the replayed path gate on the ASUS (D-0218; the OLED in
addition when T3 justifies it); merge. Every live run under WORKFLOW « Live game rule » (announced, offline, hash-checked
restore), outside the pop-in pilot's announced runs.

**T0, reading and the reader (no native code).** (a) `Get-I97Present.ps1` (or `Get-I97Cadence.ps1` extended): per segment,
the drops, holds, the gap before each hold, late presents (over 1.2 refreshes after the previous), latency steps and their
pairing with the drops, the latency p1/p10/p50/p90 and its drift in µs a frame; checked by reproducing §5's table on the same
runs. (b) In a Ghidra slot the pop-in pilot gives (light, `-readOnly -noanalysis` on the mirror): confirm the limiter's
`+0x28` at exit and its deadline formula with the catch-up and the 1/30 s mode (§4), and DS3's frame task order after the
limiter (input, world update gate, `BeginFrame`), which fixes the input latency the wait implies. (c) From `g1`/`g2`'s traces:
the grid thread's blanks against PresentMon's display times (the offset of PresentMon's display time to the blank), and
`DwmGetCompositionTimingInfo` (dwmapi, null window: `qpcVBlank`, `qpcRefreshPeriod`, `qpcCompose`) read by a script on the
desktop, no game, to see whether it gives the compositor's deadline directly (Fable's A2; some of its fields are frozen on recent
Windows, GENERAL). (d) The two drift figures reconciled on the turn's window only: §5.6's 0.3 µs a frame is the grid's step
against the limiter's (the grid stepped the nominal period, re-anchored to the blanks), §5.2's 0.5-1.3 µs the screen against the
presents; and the original's whole-run mean present gap (16.6636 ms, shorter than 1/60 s, which a spinning limiter cannot give)
recomputed on the turn. Exit: the reader reproduces §5; the deadline formula confirmed or corrected. **T0 blocks T1.**

**T1, the modern chain, no change of cadence.** `ALLOW_TEARING` created whenever supported; the present flags per mode (6.3);
maximum latency 2 kept, latency 1 as a variant under `main`'s pacer engaged only if `waitable_wait_ms` and `waitable_timeouts`
stay at 0 (6.3). Controls: the chain's flags and latency in the journal, V-Sync on unchanged (holds and drops as `main`). Series
criteria: no regression (holds, drops, latency to display, frames over 25 ms, gate); latency 1 proposed to Thomas only if the
latency to display drops with it. V-Sync off: tears in windowed and borderless only where allowed, full screen accepted.

**T2, one clock on the ASUS, two forms.** Thomas's two answers given (D-0221, §11). The engine's wait of 6.2 in two forms
behind the same flag (`display-clock.flag` holding `grid` or `waitable`): form A, the grid (6.2-6.4) with the worker's Present(1)
at once (6.3); form B, the swap chain's object (6.7). **Two control runs per form**, then the series with the form that passes its
controls; if both pass, the simpler (form B) unless the series says otherwise. Controls (both runs of a form): the wait engaged
on 99 % or more of the turn's frames; DS3's limiter's own remaining wait after ours under 0.1 ms p99 (the post-to-step gap minus
our wait about the fixed 0.3 ms in **both** runs: tranche 6's signature was a different value a run); latency to display p1 at
least 9 ms; no latency step without a late frame. Series criteria: holds and drops not preceded by a late present = 0 a turn,
judged on **both control runs and the series** (the one criterion this workstream owns alone); holds in total ≤ 0.5 a turn on
average when the turn's late presents are ≤ 0.5 (otherwise both counts reported, the late ones going to the pop-in workstream's
preparation); **the latency from the game step to the display (the trace's `game_step` against PresentMon's display time),
median and p95 at most the original's plus 2 ms** (Fable's amendment 3: the phase adds up to a refresh, and a floor alone would
not bound it); RT-9: the game dt's mean and p95 within the original's, its maximum judged with the late-frame step of 6.2
point 4, the locomotion over the path within 0.5 % of the original's; the engine thread's spin share down by at least 25 points;
frames over 25 ms not above `main`'s.

**T3, the other screens and modes.** The ASUS borderless and full screen; the OLED windowed (independent flip, 279.96 Hz: no
engine wait; `main`'s pacer against Present(1) at once), full screen with G-SYNC (VRR: no engine wait, the limiter the clock);
V-Sync off; a 59.94 Hz mode only if Thomas offers one in Windows' display settings (his to change), otherwise the unit controls;
with form A, the phase re-choices counted (6.4). Exit: every mode at least `main`'s (holds, drops, latency), the ASUS's three
modes meet T2's criteria; the gate.

**T4, later.** Reflex (On, On + Boost) on the same frame-start wait (ROADMAP); V2's single target for the unlocked rate
(CADENCE-STUDY A.7: variable step with the LTF/TRTF fixes, then fixed step with interpolation).

Order: after the pop-in workstream (D-0219), whose universal preparation removes most late frames; T2's total-holds criterion
assumes it.

## 9. Verified and supposed

| Claim | Status |
|---|---|
| Every renderer is `Composed: Flip`, sync 1 on the ASUS; the original's Present returns in 0.04 ms | OBSERVED (rows) |
| Neither renderer is locked to the display; drift 0.5-1.3 µs a frame | OBSERVED (rows) |
| A present needs about 6 ms of margin before its blank on the composed ASUS | OBSERVED in these rows; that it is the compositor's deadline is INFERRED |
| Tranche 6's six unexplained drops end episodes started by deadline misses | OBSERVED (rows) |
| The grid's extra late presents come from the eight-frame rule | INFERRED |
| 14 of 16 pacer holds follow late presents | OBSERVED (rows); their cause, the first-sight CPU work, from RESULT tranche 5 items 8 and 10 |
| The gap after the grid wait is constant a run and differs between runs | OBSERVED (traces) |
| DS3's limiter keeps its own phase against a wait before it | INFERRED from A0 and the gap |
| The limiter's next deadline is `+0x28 + +0x18 × f` between passes | OBSERVED in the A0 listing; its exit value and the catch-up's effect to confirm (T0) |
| A wait to the first slot at or after that deadline locks the engine and removes the spin | INFERRED; T2's controls prove or refute it |
| Back-pressure only paces a faster producer | INFERRED, consistent with tranche 5's three forms |
| Modern practice (waitable at latency 1, wait at the frame start, tearing rules) | SOURCE (Microsoft Learn), the rest GENERAL |
| ER's chain and pacing | OBSERVED in the repository's ER studies |
| AMD behaves the same | GENERAL (vendor-neutral DXGI calls); not testable here |

Risks: the deadline and the original's margin are one screen and one mode; reading the limiter's fields ties the module to DS3
1.15.2.0's layout (the module's pins already do); the phase offset adds up to a refresh of latency from the frame's start to the
display (bounded against the original in T2); a latency-1 variant only on measurement (6.3); a screen at exactly 60.000 Hz rests
on the tolerance of 6.2; the evidence base is one screen, one 20 s segment and two grid runs (Fable R4), hence T0 before any code.
Fable's further reading (its A3, AVIS): everything specific here (reading `+0x28`, the deadline rule) exists only while DS3's cap is
untouchable and equal to the refresh; from V2a, with the requested rate above the screen's, the plain modern loop applies (the
object at the frame start, Present(1)); T2 is a bridge for the « logic at 60 » period, its size to weigh against that life.

## 10. Evidence read

- Rows: `evidence/I-0066/artifacts/i97-t6d-settlement-asus-{g1,n1,g2,n2,o1}/presentmon.csv`, `-motion-marks.txt`,
  `presentmon-clock.txt`, `startup.jsonl` (`cadence_trace` rows); `i97-t5k-settlement-asus-{p1,p2,p3,p4,o1}/presentmon.csv`.
- The turn's window: `qpc + (segment_sweep_start - tick) × frequency / 1000` to the same for `segment_sweep_end`, from
  `presentmon-clock.txt`'s « tick qpc frequency » (the files have CRLF line ends).
- Holds and drops: awk over the window (columns `Dropped`, `msBetweenPresents`, `msUntilDisplayed`, `msBetweenDisplayChange`,
  `msInPresentAPI`, `PresentMode`, `SyncInterval`).
- The gap after the grid wait: a 30-line Python reader (Python 3.12, `py -3.12 -I`) rebuilding `game_step`, `grid_wait_end` and
  `engine_post_done` times from the `cadence_trace` rows (`first` plus the `d` deltas), the last grid wait end before each game
  step in the turn; its text is below, to be folded into T0's reader (run from the evidence folder: `py -3.12 -I gap.py <run>`).
- The limiter: `evidence/I-0097/artifacts/cadence-limiter-a0.txt` lines 120-140 and 230-275.
- Code: `native/renderer/backend.cpp:969-1006`, `native/renderer/frame_pacer.{h,cpp}`, `native/renderer/scene_pass.cpp:850-866`
  and `1719-1735`, `native/frame/game_bootstrap/20_startup_phases.inc:85-111`, `02_frame_lifecycle.inc:270-335`,
  `17_render_step.inc:505-519`, `15_cpu_service_and_input.inc:327-355`.

```python
import json,sys,bisect,statistics as st
run=sys.argv[1]
ev={}
for line in open(run+'/startup.jsonl',encoding='utf-8',errors='replace'):
    if '"cadence_trace"' not in line: continue
    try: r=json.loads(line)
    except: continue
    k=r['kind']; t=r['first']; L=ev.setdefault(k,[]); L.append(t)
    for d in r.get('d',[]): t+=d; L.append(t)
for k in ev: ev[k].sort()
tk,qpc,fr=[int(x) for x in open(run+'/presentmon-clock.txt').read().split()]
mm={l.split()[0]:int(l.split()[1]) for l in open(run+'/motion-marks.txt') if l.strip()}
s=qpc+(mm['segment_sweep_start']-tk)*fr//1000; e=qpc+(mm['segment_sweep_end']-tk)*fr//1000
gs=[t for t in ev.get('game_step',[]) if s<=t<=e]
we=ev.get('grid_wait_end',[]); pd=ev.get('engine_post_done',[])
gaps=[];after=[]
for t in gs:
    i=bisect.bisect_left(pd,t)-1; j=bisect.bisect_left(we,t)-1
    if i>=0: gaps.append((t-pd[i])/1e4)
    if j>=0 and we and (i<0 or we[j]>=pd[i]): after.append((t-we[j])/1e4)
def q(a,p): a=sorted(a); return a[int(p*(len(a)-1))] if a else float('nan')
print(run.split('/')[-1],'steps',len(gs),'post->step p50 %.2f'%q(gaps,.5),'gridEnd->step p10 %.2f p50 %.2f p90 %.2f n=%d'%(q(after,.1),q(after,.5),q(after,.9),len(after)))
# stability over the turn: per 100-frame chunk median
ch=[ '%.1f'%q(after[i:i+100],.5) for i in range(0,len(after),100)]
print('  gridEnd->step medians per 100 frames:',' '.join(ch))
```

## 11. Fable 5.1's review (2026-10-26 05:05-05:45 Paris) and Thomas's two questions

The review (French, copied beside this study as [MODERN-CADENCE-REVIEW-FABLE.md](MODERN-CADENCE-REVIEW-FABLE.md)) found the core
sound: the diagnosis of tranche 6 and the deadline rule, its arithmetic redone by hand (the lock holds by itself on any screen
at or below 60 Hz without touching the limiter). It could not reread the Evidence Store's rows (not on GitHub) and took §5's
numbers as given, checking them against RESULT tranches 5-6. Integrated here (the coordinator's instruction, 2026-10-26):
1. **Maximum latency 2 kept** (6.3, T1): latency 1 only as a measured variant.
2. **The screen faster than 60 Hz** corrected (6.2 point 5, 6.5): no engine wait beyond 10 ppm; the earlier « every ~45 s » was
   wrong (about every 1.5 s with the wait).
3. **A bound on the latency from the game step to the display** in T2 (median and p95 at most the original's plus 2 ms).
5. **T2 as an A/B of two forms**, the grid and the swap chain's object under the same deadline rule (6.7), two control runs each.
Also taken from it: the late frame's three refreshes for two steps (6.2 point 4), the timeline consistency (6.2 point 6), the
offset modulo the period and its re-choices (6.4), the windowed VRR nuance (6.5), T0 blocking T1, `DwmGetCompositionTimingInfo`
and the drift reconciliation in T0, the « no hold without a late frame » criterion on the control runs too, V2's plain loop (§9).

**Thomas's two choices** (Fable's point 4; answered 2026-10-26 05:30 Paris, « oui aux deux », D-0221: the vblank thread as the comparison form only, the dt on the screen's period subject to RT-9). The questions as put:
- **Reuse the vblank thread of the abandoned tranche** (form A), or keep only form B, which needs no thread of its own. D-0219
  asked to stop tinkering; the thread itself measured well (0.02 ms), but it is the plan's most hand-made piece.
- **Accept that the game's dt becomes the screen's period** on fitting screens (about 16.668 ms on the ASUS (from the drift, §5.2), 16.683 ms at
  59.94 Hz), instead of DS3's 1/60 s: a reading of D-0169 (« the game clock is not touched »: nothing is written into it, but
  its dt follows the display). The review says the original does the same under V-Sync; on the ASUS it does not (its Present
  returns in 0.04 ms and it is not locked, §5.1-5.2: its dt stays 1/60 s). What follows the dt stays in real time; any part of
  DS3 that steps a fixed 1/60 s would run 0.1 % slower at 59.94 Hz (not known; RT-9 measures it). Without this answer T2 has no
  design.
