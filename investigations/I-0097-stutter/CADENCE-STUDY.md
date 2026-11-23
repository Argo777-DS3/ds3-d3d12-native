# Cadence study: one clock, universal anticipation, an automatic pop-in counter

> Note (2026-10-26, on its merge into `main`): where this study says the gate runs « on both screens », that was D-0209; since
> D-0218 the gate runs on the ASUS (the OLED added when justified), its memory leg with `-Memory` when a branch touches the memory,
> the loadings or the streaming (the pop-in workstream and I-0099's fix do). The text below is the study as written.

Read-only study (2026-10-25 08:43-08:50 Paris, branch `claude/popin-plan-study`), asked by Thomas (project chat, 08:42 Paris,
relayed by the coordinator), written while the I-0097 tranche 5 pilot runs its announced tests: no game, no build, no Ghidra;
the code in `native/`, the decompiled functions already in the repository and journals already in the Evidence Store were read.
Statuses: OBSERVED (read in the code, a decompile or a journal here), INFERRED, GENERAL (general knowledge, not checked here).
Nothing here is promoted to the ledger. A copy for Thomas is in the project files under `etudes/`.

## A. One clock

### A.1 What exists today (OBSERVED)

- **DS3's limiter** runs on the engine thread: it spins on the performance counter (around 0x140EEDBE9-0x140EEDC65, I-0078
  `i78-original-04/05`; 31 % of the engine thread's samples at Firelink, I-0084; 40-42 % of the turn's frames at the
  Settlement, I-0097 tranche 5 RESULT item 6). Its rate is the central timing object's requested rate (`[0x14489DD10]+0x354`,
  enable `+0x358`, K-0019 in `timing/`). Its function was not found in the repository's exports (the clock helpers around it,
  `00EED810`, are thunks into the protected code at 0x144E79363, `timing/evidence/I-0005/static/central-clock-core`).
- **The frame stamps it measures from**: the original's frame end `D56930` takes `clock+0x40` (`EEE190`) before the post to
  its render thread and `clock+0x48` (`EEE160`) after the post returns (R13-R14 pacing sheet §8.3); the native `EndFrame` does
  the same (`17_render_step.inc` lines 512-519: `EEE190`, then `Clear()` and its post, then `clock+0x48` from QPC).
- **Our pacer** runs on the render worker, not on the engine thread: `SceneRenderer::PresentPaced` (`scene_pass.cpp` line 852)
  waits for its deadline on a high-resolution waitable timer plus a 1 ms spin (`frame_pacer.cpp`), the deadline stepping by the
  screen's exact refresh period when the screen fits 60 Hz within 2 % (D-0210, D-0212), then presents with Present(1). The
  frame-latency wait is also on the worker (`WaitFrameSlot`, line 1722). The engine thread only meets the worker at the post,
  which waits for the previous job (`02_frame_lifecycle.inc` line 450; two frames in flight, D-0187).
- **In the original** the render thread's Present(1) blocks on the vertical blank (34 % of the capped main thread at m45,
  I-0078), so the display is the master and the limiter follows it; natively nothing on the engine thread waits for the
  display except the post when the worker is a frame behind.

So the native has **two clocks**: DS3's limiter (1/60 s from its last stamp, on the engine thread) and our pacer (the screen's
refreshes, on the worker), coupled only when the post has to wait. INFERRED, to confirm by the pilot's Nsight run: when the two
periods differ (a screen at 59.95 or 60.02 Hz against 60.000) the jobs drift against the deadlines until a frame waits or misses
a refresh, the beat the turn shows (17-18 ms and under 16 ms frames where the original sits at 16-17, RESULT item 5).

### A.2 Proposal: the display's grid as the only master

1. **One grid**: our pacer's deadlines (the screen's exact period, re-anchored at each real wake as today).
2. **The engine thread waits on it**, not the worker: the engine's frame boundary is held until the slot of its frame on that
   grid, before `clock+0x48` is taken, as the original's blocking Present does. DS3's limiter then finds its 1/60 s already
   elapsed and does not spin (its CPU time is freed, the same quantity as I-0084's 31 %), and the game's dt equals the display
   period. The worker presents as soon as the frame is recorded, with Present(1).
3. **The game's clock is not touched** (D-0169): the requested rate stays 60; only where the stamp is taken moves, which is what
   R13-R14's open witness RT-9 checks (animation speed and the dt distribution against the original). STATE still lists RT-9 as
   an open witness.
4. **The one case to settle first** (A0): a screen slightly faster than 60 Hz (60.02 Hz) has a period below DS3's 1/60 s, so the
   limiter would wait a few microseconds every frame and the drift comes back. Two answers, to choose once the limiter's
   reference is read: the grid steps by 1/60 s there (one repeated refresh about every 50 s, as the original), or the limiter's
   wait is skipped when ours has already waited. Reading the limiter needs a light Ghidra pass outside the announced tests.
5. **Efficiency**: the wait already uses a high-resolution timer and a 1 ms spin (A.1); the latency stays two frames in flight
   with the waitable chain (D-0187); input is read at the start of the engine frame, after the wait, so the wait does not add
   input latency (GENERAL: this is the reason modern DX12 games wait on the frame-latency object at the start of their loop).
   NVIDIA Reflex (ROADMAP option) is the vendor version of the same just-in-time wait (GENERAL); it fits on this design later.

### A.3 Other limiters on the PC

- **RTSS** (OBSERVED 08:47 Paris): installed (`C:\Program Files (x86)\RivaTuner Statistics Server`), not running then; its
  Global profile limits to 141 frames/s (`Limit=141`, `SyncLimiter=0`), no Dark Souls III profile. At our 60 cap it does not
  engage. GENERAL: RTSS limits by sleeping inside Present, so a limit at or under the game's cap adds a third clock on the
  worker; recommendation: no RTSS limit for the game (or one above the cap). The module can log at start whether the RTSS hook
  is loaded in the process, so a measured run knows (cheap; to add with A1).
- **NVIDIA driver** (GENERAL, settings not read: they are Thomas's): the driver's Max Frame Rate is another limiter, to leave off
  or above the cap; the forced V-Sync Thomas keeps for the OLED with G-SYNC (STATE, 2026-10-24 23:58) is the usual G-SYNC
  setting and stays: on the OLED in full screen each frame is shown when presented, so our grid is the display timing there;
  windowed, the OLED is not under VRR (D-0212) and forced V-Sync equals our Present(1). NVIDIA's Low Latency Mode does not
  apply to DX12 games (GENERAL). Pilots never change these settings (WORKFLOW Standing rules).
- **An overlay was running at 08:56** (OBSERVED): a white « 60 » frame-rate counter sits at the OLED's top right on
  `evidence/I-0066/artifacts/i97-t5b-ariandel-oled-n1/start.png`; the NVIDIA overlay (Thomas, 09:10); its processes run since 06:20,
  so through the good 06:48 run too. Overlays hook Present (GENERAL).
- **Measuring with them**: a run states RTSS running or not and the screen mode; the gate's bases are taken without RTSS.

### A.4 Link with the unlocked frame rate (D-0169)

The remaster's option (two sliders, 10-400 frames/s, ROADMAP) then sets one number: our grid's target. DS3's requested rate is
raised with it, which is the game-clock change D-0169 keeps for that option, with the LTF/TRTF fixes (`timing/`). With one clock
the option has a single owner; without it, two limiters would have to agree at every setting.

### A.5 Steps (each with a dry check, A/B on both screens and the gate before a merge)

- **A0** (read-only): DS3's limiter function, its reference stamp and how it waits (light Ghidra on the mirror, outside tests).
  ER's frame pacing too: the repository has no study of it (no match in the ER folder's notes), so this reads ER's present path
  once (D-0195), read-only, never with DS3 running.
- **A1**: the engine thread waits on the grid (behind a flag, A/B), RT-9's witness (dt distribution, animation speed, against
  the original); measured on the turn on both screens with PresentMon and the pilot's Nsight timeline.
- **A2**: the slightly-faster-screen case per A0; the RTSS-hook row.

### A.6 After the pilot's three tries: DS3's limiter waiting on the display (2026-10-25, 12:06-12:13 Paris)

The pilot's result (`claude/quizzical-wiles-08d28d`, RESULT « Tranche 5, third step » items 7 and 9, OBSERVED there): three
forms of « the display commands » (the engine thread or the worker waiting the frame-latency object) never blocked, because
DS3's limiter waits 12.2-12.6 ms after our post stamp every frame and steps 1/60 s from its own reference, while the ASUS
refreshes every 16.669 ms: 5-14 frames held a turn against 2-3 with the current pacer, which stays. So the display can only
command once the limiter's own 1/60 s is replaced by the display.

**Where the limiter is** (from the repository and the pilot's sampled profiles; read-only, no Ghidra):
- Its spin is the loop 0x140EEDBE9-0x140EEDC65 (self samples at +EEDBF1, +EEDC13, +EEDC30, +EEDC44, +EEDC51, +EEDC57), whose
  call to `QueryPerformanceCounter` returns to +EEDC60 (`i97-t5-prof-settlement-n3/sample-profile-slow-report.txt`: +EEDC60
  inclusive 42.3 %, `RtlQueryPerformanceCounter` 42.0 %). It is called from DS3's frame step at 0x1423C1D9F (return
  0x1423C1DA4, 41.3 % inclusive), the instruction after the call that runs our `EndFrame` (return 0x1423C1D94, where `Clear`
  and `EndFrame` sit): **the limiter runs right after the frame end, before the next frame's start** (OBSERVED).
- Its function is not in the repository's exports: the I-0005 range dump of the central clock stops at `00EED810` (a thunk
  into DS3's protected code at 0x144E79363) and resumes at `00EEE100` (`timing/evidence/I-0005/static/central-clock-range.asm.txt`).
- What it measures: the rate is the central timing object's requested rate (`[0x14489DD10]+0x354`, enable `+0x358`, K-0019 in
  `timing/`); the clock's stamps are `+0x30` (`EEE130`, frame start), `+0x38` (`EEE100`), `+0x40` (`EEE190`, frame end before
  the post) and `+0x48` (`EEE160`, after the post; the native `EndFrame` writes it itself, `17_render_step.inc` line 519)
  (decompiles in `timing/evidence/I-0005/static/central-clock-core`, OBSERVED). Which stamp the limiter adds 1/60 s to is not
  read; the pilot's 12.2-12.6 ms after `+0x48` with an engine frame of about 4.1-4.5 ms suggests the frame start (`+0x30`),
  INFERRED.

**How ER does it** (I-0080 ER-STUDY, checked by Opus there): the same design. A QPC-timed limiter (140E84830, targets 1/20 to
1/360 s by an option mode, 1/60 usual) spins on `QueryPerformanceCounter` when the window has focus (sleeps to about 5 ms before
the deadline without focus); the main thread is then coupled to the GPU only by its frame-lag fence wait (141AFC9A0, lag 2,
`Sleep(0)` polling) and Present's sync interval. ER does not make the display command either: copying ER gives the two clocks we
have (D-0195: copy ER, without its defects).

**The safest way to plug it on the presentation's return** (no observation hook in play, the game clock untouched: its rate,
its requested rate and its timing fixes stay as they are, D-0169):
1. **A0 first** (read-only, a light Ghidra pass on the mirror outside the announced tests): the limiter's function, the stamp it
   measures from, how it compares (from its own last deadline, which accumulates, or from the stamp, which does not) and how it
   leaves its loop.
2. **Preferred form, L-stamp**: the native already writes the clock's phase stamp `+0x48` in its production `EndFrame`. If A0
   shows the limiter measures 1/60 s from a stamp the frame end writes, the native writes that stamp from the display's grid
   (the moment the display slot of the previous frame opened, from the present statistics or the latency object) instead of
   « now »: the limiter's wait then ends on the display's cadence, its code and its rate untouched, no patch in DS3's protected
   code. It changes where the engine's dt is measured, which is exactly R13-R14's open witness RT-9 (animation speed and dt
   distribution against the original): RT-9 decides, and D-0169's reading (« the game clock is not touched ») is put to Thomas
   before the merge.
3. **Fallback, L-rate**: if the stamp cannot be set safely, the engine thread waits the display slot at the frame end and DS3's
   requested rate is raised just above the target (for example 61 frames/s), so its limiter never waits before the display
   does. Simple, but it writes the requested rate: Thomas's decision under D-0169.
4. **Not proposed**: patching or hooking the limiter's loop itself (it sits next to DS3's protected code, and the module's code
   pins refuse a changed byte), or a diagnostic hook kept in play.

**Risks**: the dt distribution and the animation speed (RT-9); a missed refresh gives a 33 ms step, as in the original; a
window that stops presenting (minimised, occluded, a screen change) must fall back to the limiter's own 1/60 s by a bound; the
OLED windowed at 279.96 Hz and a VRR screen have no fitting refresh: they keep the current pacer (the pilot's
`FramePacer::DisplayPaces` rule); V-sync off and the future unlocked rate keep one owner (A.4); the frame-latency object's count
must stay consistent between the threads (the pilot's step review finding H1).

**The test that proves the gain** (the pilot's `Run-I97Cadence.ps1`, the Settlement's 20 s mouse turn on the ASUS with the chain
made there, four runs each, alternated with the current pacer): frames held over 1.5 refreshes a turn 2-3 → at most 0.5 on
average (the original 0); the presents' phase in the refresh p10-p90 under 1 ms (the original locked at one phase); RT-9: the game
step's dt mean, p95 and max within the original's, and the locomotion over the Settlement path within 0.5 % of the current
build's; then the gate on both screens (D-0209), the OLED's numbers unchanged.

### A.7 Thomas's decision: a modern limiter that replaces DS3's (2026-10-25 19:16 Paris)

Thomas (project chat, relayed by the coordinator): he validates the target, a modern limiter in place of DS3's; the logic stays
at 60 frames/s for now, **temporarily**: the remaster (V2) unlocks it. Steps: A0 (read the limiter), the replacement's design
(the wait on the display at the frame's start; 60 frames/s with V-Sync, with G-SYNC, without any driver setting, on AMD too),
then the path to V2 (the logic decoupled). No code before Thomas validates.

**What « protected code » is, and why not patch it** (OBSERVED unless marked):
- DS3 ships with a commercial anti-tamper layer (Arxan, as the repository names it, MENUS-MAP and FADE-PLATES-NATIVE). Its
  marks in the executable: two extra code sections beside `.text` (`.text` at 0x4DBD000, 9.4 MB, `.interpr`, `.bind`: PE headers
  of `game/DarkSoulsIII.exe` read at 19:20); thunks that jump into them (`00EED810` → 0x144E79363, which lies in that second
  `.text`); functions **encrypted at rest** and decrypted only while the game runs (the repository reads them from a running
  process, `Read-LoadedCode.ps1`, `decompiled/*.loaded*.c`); guards at function entries (`8DF920`, LEGACY-ROOT-INVENTORY-903).
- What we risk by modifying it: such layers check their own code and the code they guard while the game runs and react when
  it differs (a crash, a corrupted state, a silent switch to a degraded path: GENERAL, the behaviour of this family of products,
  not measured on DS3); a patched byte may also be overwritten when the guard decrypts it again; and our own module refuses to
  start when DS3's code bytes differ from the known ones (the launch « pins », STATE « Launches refused at start », two
  functions accepted only in their decrypted form). Online, a modified game can be flagged by FromSoftware (GENERAL); our runs
  are offline (D-0116).
- **The limiter itself is not there**: its loop at 0x140EEDBE9-0x140EEDC65 is plain code in the file (bytes read at 19:20:
  SSE arithmetic on 64-bit counters, `cvtsi2ss`/`divss`/`comiss`, then `lea rcx,[rbp+7F]; call [rip+...]` to
  `QueryPerformanceCounter` and `mov [rbx+28],rax`, storing the counter it read into the object at `rbx`). The repository's
  export missed it because Ghidra had no function defined there (the range dump shows `NO_FUNCTION`), not because it is
  encrypted. So DS3's limiter can be wrapped like the engine functions the module already hooks in production (`BeginFrame`,
  `EndFrame`, the pose updates), without touching the protected sections.

**More from the file's bytes** (read at 19:20-19:25, decoded by hand, to confirm by A0):
- Before the loop, at 0x140EEDA00: `cmp [rbx+27A],…; mov [rbx+270],…; mov rax,[rbx+28]; mov [rbx+20],rax;
  call [QueryPerformanceCounter]; mov rax,[rbp+67]; mov [rbx+28],rax; movss xmm0,[rbx+18]`, then 64-bit to float conversions:
  the limiter keeps its **previous counter at `+0x20` and the current one at `+0x28`** of its object and compares the elapsed
  time with a target at `+0x18`. So it measures from its own previous pass, not from our `+0x48` stamp (INFERRED from the
  decode). The fields `+0x278`/`+0x27B` it tests are the ones `00EED5B0` and `00EED7F0` return (the I-0005 exports).
- No padding separates the loop from `00EED810`, the thunk into the protected section: the function's entry is most likely that
  thunk (the protection moved the function's first instructions into its own section and jumps back into the plain body),
  INFERRED. Hooking the limiter's entry would therefore mean hooking a protected thunk: to avoid.
- The profiles' addresses are « return address minus one » (the sampler's convention): the calls return to 0x140EEDC61 and
  0x1423C1DA5. At 0x1423C1D80 DS3 invokes timeline tasks through their virtual tables (`call [rax+8]` and `call [rax+10]`): our
  `EndFrame` and the limiter are two such tasks, the limiter the next one.

**Consequence for the design: no hook on the limiter at all.** Since DS3's limiter measures from its own previous pass, a wait
of ours placed at the end of our `EndFrame` (a production hook the module already owns), until the display's next slot, puts
the engine on the display's grid; DS3's limiter, running next, then finds its 1/60 s already elapsed (on a screen at or below
60 Hz) and returns at once, every frame, re-phased by our wait. It stays in place as a safety cap: if our wait is bounded or
skipped, DS3 still caps at 60. The pilot's three forms failed because they waited on the frame-latency object, which never
blocks while the game is never a present ahead (RESULT item 9); this one waits on a **deadline** of the display's grid, which
does block.

**A0** (light Ghidra on the mirror, `-readOnly -noanalysis`, one function created and decompiled at the loop's start, outside
the pilot's announced runs): the limiter's entry, its object (`rbx`: the central timing object `[0x14489DD10]` or another),
every field it reads and writes (`+0x28` is one), the deadline it computes (from its own last deadline, or from a stamp), and
how it leaves the loop.

**A0 done** (2026-10-25 19:33 Paris, in the pilot's free slot 19:31-19:36; `CadenceLimiterRead.java` on `ghidra-mirror-i0066`,
`-readOnly -noanalysis`, nothing saved; listing in `evidence/I-0097/artifacts/cadence-limiter-a0.txt`; OBSERVED in the
disassembly, `rbx` being the object whose `+0x354`/`+0x358` are K-0019's requested rate and enable, i.e. the central timing
object):
- **Entry and callers**: the thunk `0x140EED810` into the protected section; the bytes 0x140EED815-0x140EED86A are not valid
  code (the protection's relocated prologue), the plain body starts after them. Two direct calls, from 0x140ED8EE7 and from
  0x140EDABBD (DS3's frame step, the 0x140EDAxxx function I-0078 profiled), and one data reference at 0x144A53140.
- **Its target period** `+0x18`: set by a mode switch (`+0x8`/`+0xC`, `+0x15`/`+0x16`, jump table at 0x140EEE0E0) to 1/30 s
  (`0x3D088889`) or 1/60 s (`0x3C888889`); when `+0x358` is set, `+0x18 = 1 / +0x354` (the requested rate). `+0x35C` ramps
  `+0x354` toward a value at `+0x2C0`.
- **Its reference**: `+0x20 = +0x28` (the counter it stored at its previous exit), `+0x28 = QueryPerformanceCounter()`, the
  target in counts = `+0x18` × the counter's frequency (`[0x144929E00]`). **It measures 1/60 s from its own previous exit**,
  as inferred.
- **A catch-up**: with `+0x278` set, it sums the last `+0x270` (or `+0x274`) frame times of a 32-entry history (`+0x260` its
  index, entries from `+0x60`) and shortens the target, down to two thirds of it, when the past frames ran late.
- **An off switch**: `+0x27B` set skips the wait entirely (jump to the exit).
- **Two waits**: with the flag a call to 0x1423575B0 returns (on `[0x14489F118]`; INFERRED: the window has focus, as ER's
  limiter) it **spins** on `QueryPerformanceCounter`, storing each read in `+0x28`, until the remaining time is not positive
  (0x140EEDBD0-0x140EEDC70); otherwise it sleeps (0x1417E8070, the remaining milliseconds less a margin) then spins
  (0x140EEDC75-).
So the design below holds: a wait of ours that ends on the display's slot, at least 1/60 s after the limiter's previous exit,
leaves its loop with nothing to wait; its fields keep being written by DS3's own code. Its catch-up only ever shortens its
target, so it cannot lengthen a frame we paced. Its off switch `+0x27B` is not needed (and writing it would be a game-timing
change under D-0169).

**The replacement: our wait, then DS3's own limiter** (A0 confirms the premise):
1. **Our wait** at the end of our `EndFrame` (no new hook, no observation code in play), then DS3's limiter unchanged as the
   next timeline task: when our wait ends at or after its own deadline, its loop exits at once, and every field it writes (the
   counters it stores) is written by DS3's code as today. The game clock and the requested rate stay untouched (D-0169); the
   limiter's spin, 31-42 % of the engine thread, disappears. (Were A0 to show the limiter measures from another stamp, the
   same wait moves to where that stamp is taken.)
2. **Our wait is one grid, the display's**, held where the frame starts (the limiter runs right after `EndFrame`, A.6), not on
   the render worker: deadline = the last displayed slot + the screen's exact period when V-Sync is on and the screen fits
   60 Hz within 2 % and is not faster than 1/60 s (59.94-60.00 Hz: DS3's 1/60 s has always passed when our slot comes); a
   high-resolution timer for the coarse part and a short spin, as today's pacer; the render worker then presents at once with
   Present(1). The slot is anchored to the display, not to « now »: the frame statistics where they are reliable, else the
   vertical-blank thread of the pilot's `cadence-trace.flag` made a production component (one thread waiting
   `D3DKMTWaitForVerticalBlankEvent` on the window's screen).
3. **Other screens** (D-0212's rule kept): a screen slightly faster than 60 Hz (60.02 Hz), a fixed rate that is not a multiple
   of 60 (144 Hz, the OLED windowed at 279.96 Hz), a VRR screen (G-SYNC, FreeSync) and V-Sync off: our timer steps exactly
   1/60 s and the worker presents with Present(1) (or 0 when V-Sync is off): with VRR each frame is shown when presented, on a
   fixed screen at the next refresh, as the original. Nothing depends on a driver setting or a vendor: the frame-latency object,
   DXGI's statistics and the vertical-blank wait are vendor-neutral (GENERAL), so AMD behaves the same.
4. **Bounds**: a window that stops presenting (minimised, occluded, a screen change) caps our wait at 1/60 s past DS3's deadline
   and falls back to DS3's limiter alone; the first frames after a resize or a load re-anchor.
5. **Proof** (A.6's test): the Settlement's mouse turn on the ASUS, frames held over 1.5 refreshes 2-3 → at most 0.5 a turn (the
   original 0), the presents' phase p10-p90 under 1 ms, RT-9 (dt and animation speed within the original's), the engine
   thread's spin gone; then the gate on both screens.

**Thomas's go** (2026-10-25, about 19:57 Paris as the coordinator relays it, and in this thread at 21:03: « Tu as mon feu
vert »): the limiter step is a tranche of its own, after tranche 5's merge, opened by a fresh thread: our wait on a deadline of
the display's grid at the end of our `EndFrame`, no hook, DS3's limiter untouched; A.6's test proves it.

**The path to V2 (the unlocked rate)**: with one owner of the cadence, the remaster's option (two sliders, 10-400 frames/s,
ROADMAP) sets one number, our grid's target. Two ways to decouple the logic, in this order:
- **V2a, variable step**: DS3's requested rate (`+0x354`/`+0x358`) set to the same target, so the game steps at the display's
  rate with its own variable dt, plus the timing project's fixes of the mechanisms that break above 60 (locomotion LTF, terrain
  response TRTF, `timing/` I-0007 to I-0014) and the others found per mechanism (D-0167: universal rules, never case by case).
  Closest to what the timing project already proved at 165 and 240 frames/s.
- **V2b, fixed step with interpolation** (GENERAL: the modern engines' way): the game's logic kept at a fixed 60 Hz step and
  every rendered frame interpolated between the last two logic states (camera, characters' poses, objects, effects). It removes
  the frame-rate bugs by construction but needs every drawn state to keep two versions: a large workstream, for later.

**An outside example** (relayed by the coordinator, 2026-10-25 19:24, not read or checked here): the fork
`Supermedo/bloodborne_pc` (PS4 code run natively, a Vulkan renderer derived from shadPS4) presents on a thread of its own locked
to the vertical blank with one frame ahead (`BB_PRESENT_THREAD`, `BB_FRAMES_AHEAD=1`), the same idea as A.7's single display
grid. For V2's upscalers (ROADMAP: DLSS through Streamline, D-0165; FSR as a possible second), it builds its own motion vectors
from the depth and the previous frame's matrices with a Halton jitter for FSR 3.1/4; our velocity is today filled by the object
rows and the camera motion only (VELOCITY-IMPLEMENTATION-CHECKLIST 6.4): the depth-and-matrices reconstruction is a candidate for
what the forward draws, effects and water do not write. To read at its source before any use.

## B. Universal anticipation

### B.1 What the turn is (OBSERVED)

The pilot's sampled turn at the Settlement (`i97-t5-prof-settlement-n3`) shows the slow frames carry no more work than the
others: they wait (I-0097 RESULT tranche 5 item 6). So the turn's jitter is a cadence matter (A), not something to prepare.

### B.2 What still costs at a first sight (OBSERVED in the repository)

- A model's first programme build costs 3.5-28 ms in the collect (STATE, the class « first steps after a standstill »);
- a pipeline first met on a PC still compiles at its first frame (I-0072, the on-disk cache covers the next runs);
- a kept record unseen for 120 collects is dropped (kept up to 3,600 while its storage is registered and its textures stay,
  I-0070 RESULT), so a turn back to an old view rebuilds it.

### B.3 Proposal: prepare everything loaded, not what the camera might see

- **Universal** means independent of the camera: when DS3 admits a model (and at the load for the whole zone), its programmes,
  pipelines for every pass it can take, and its kept record are prepared off the engine thread, whatever the view. A turn, a
  fast move or a teleport then only finds ready entries. GENERAL: Unreal Engine's PSO precaching works this way (at component
  load, not at first draw, « skip the draw until ready » for the rest); ER builds its draw records every frame on parallel jobs
  and keeps no per-view prediction (I-0071 ER-STUDY A, OBSERVED).
- **No camera prediction**: predicting the turn would cover one case; preparing what is loaded covers all, at a memory cost to
  measure (records kept while their model is loaded instead of 120 collects).
- **Links**: it is D-0211's « prepared ahead » applied to our own per-model work, the pop-in plan's T3/T5
  (`POPIN-STUDY.md`), and it removes first-time costs that would break A's regular cadence.

## C. An automatic pop-in counter

### C.1 What exists (OBSERVED)

The replayed paths are deterministic with `test-no-ai.flag`; `Capture-ScreenSeries.ps1` takes a picture every 2 s on a
composed screen; each run's `motion-marks.txt` aligns the two renderers (the Settlement pairs `n2`/`o2`: native picture k+1
against original picture k). A side-by-side sheet of five pairs was made in this session with a 15-line PowerShell script
(System.Drawing, no dependency): on it a distant hollow the original shows is absent natively on one pair (a hint, one pair).

### C.2 Options and cost

| Option | What it counts | Cost | Reliability |
|---|---|---|---|
| C-a: side-by-side sheet after every run (native beside the original, path-aligned) | nothing by itself; Thomas sees it from his phone | small: one script and a call at the end of `Run-I97PathBases`, no build | as good as the eye; no false alarms |
| C-b: image difference counter (tiles that differ between the two renderers at the same path point) | regions that differ | medium: alignment by motion mark only, the character's position drifts between runs (Thomas), lighting differs slightly | poor: drift and animation give false pops; needs 10 pictures a second and manual review |
| C-c: the per-entity visibility journal of the pop-in plan (T0, both renderers) | pops (absent to drawn without a fade) and flicker per entity, with distances | one tranche: native and reference hooks behind a flag, a reader, a step review, the gate before merging | exact; the counter Thomas asks for |

Recommendation: C-a now (it costs little and works without Thomas at the PC), C-c as the real counter inside the pop-in
workstream's T0, C-b not done.

## D. Recommendations, in order

1. C-a, the side-by-side sheet, with the next path bases (no build).
2. A0, then A1 and A2 in the stutter workstream: one clock, the display's, held on the engine thread, the game clock untouched.
3. The pop-in workstream (POPIN-STUDY T0 to T6, with C-c as its counter), whose T3/T5 carry B's universal preparation.
4. Later, with the remaster's options: Reflex on the same wait, and the unlocked rate as one target (D-0169).
