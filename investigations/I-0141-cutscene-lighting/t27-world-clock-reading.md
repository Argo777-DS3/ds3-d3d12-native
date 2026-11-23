# T27 world clock: static reading of the dt chain (DS3 1.15.2, read-only)

Read by a Sonnet subagent for the pilot (2026-11-23). Non-canonical until the pilot rereads what a design depends on.
READ = seen in code this session; INFERRED = my reading of the shape; OPEN = not reached.
Sources: GhidraMCP on Thomas's GUI (DarkSoulsIII.exe 1.15.2, sha256 ef5e07c5...), `decompile_function_by_address`,
`disassemble_function`, `get_xrefs_to`, `get_function_xrefs`. The limiter body is encrypted at rest: read headless through the
overlay (`investigations/I-0133-protected-code-map/tools/I0066Overlay936.java`, entries `140eed810 144e79363 144f382d4 1417583a2
140eed8b0 140eedd3b`, run through the main checkout's `Invoke-Ghidra.ps1` on a throwaway copy of `game/ghidra`'s FINAL project, removed
afterwards). Outputs (first in the session's scratch, now gone):
the listing `140eed8b0.asm.txt` kept in `evidence/I-0141/artifacts/tranche-27/world-clock/`.

## 1. Who writes +0x268 and +0x26C (answer to Q1)

READ, the limiter's exit, function body `140EED8B0..140EEE0DF` (overlay export, body `[140eed8b0,140eed8d6] [140eed940,140eee0df]`):

| Address | Instruction | Meaning |
|---|---|---|
| `140eede1a` | `MOV [RBX+0x264],EAX` | frame duration clamped: min(d, 1/[+0x2C0]) (the `DIVSS XMM13,[RBX+0x2C0]` at `140eeddaa`, `[+0x2C0]` = 20.0) |
| `140eedffc` | `MOV [RCX],EAX` | clamped value pushed into the 32-float history (`+0x2C4..`) |
| **`140eee02c`** | `MOVSS [RBX+0x268],XMM1` | **dt**: sum of the last `[+0x34C]` (3) history floats, `DIVSS XMM1,XMM0` (`140eee028`) by that count |
| **`140eee059`** | `MOVSS [RBX+0x26C],XMM6` | mean of the last `[+0x350]` (16) history floats (`DIVSS` at `140eee055`) |
| `140eee073/7d/87` | `MOV [RBX+0x264/0x268/0x26C],0x3D088889` | reset to 1/30 when `[+0x360]==0` (`140eee061..068`) or `[+0x361]!=0` (`140eee06a`); `140eee091` clears `+0x361` |

- No multiplier, no game-speed float anywhere between the QPC delta and `+0x268` / `+0x26C` (READ in the whole listing: the only
  `DIVSS` are `140eed98d` (1/[+0x354] period), `140eedb9f`, `140eedc0f`, `140eedccf` (wait arithmetic), `140eeddaa`, `140eeddf0`
  (frame-rate estimate +0x2BC), `140eedf7b`, `140eee028`, `140eee055`). There is no ER-style CSFlipper game-speed field. READ.
- The constructor `TimingObject_Constructor_140eed360` (size 0x368) seeds `+0x264/+0x268/+0x26C` = `0x3D088889` (1/30), `+0x2C0`...
  READ (decompile). Other plain-stores to `+0x268`/`+0x26C` exist elsewhere in the image as disp32 stores of other classes; a raw scan
  cannot separate them (201 hits for 0x268), so "the limiter and the constructor are the only writers of THIS object" is INFERRED
  (the repository's I-0111 §8.2 reads the same).
- Entry chain: `MainLoopTick_140edaae0` calls the thunk `140eed810` at `140edabbd` (`JMP 0x144e79363`). With the overlay,
  `144e79363` is a `RET`-trampoline to `144f382d4`, another to `141758...` (function `141757ed0..141758f44`); I did not follow the
  trampoline chain to `140eed8b0` (OPEN; I-0111 §8 asserts it from runtime captures).
- Hook-relevant: the limiter's last store to `+0x268` (`140eee02c`) happens INSIDE the thunk call, so a write to `+0x268` from
  anywhere earlier in the frame is overwritten before the timeline reads it; a write must come AFTER the call returns (after
  `140edabc2`) or the read must be intercepted.

## 2. Where the world gets its dt (answer to Q2): one hand-off, yes

READ, `MainLoopTick_140edaae0` (disassembly):
1. `140edabbd CALL 0x140eed810` (the limiter, writes +0x268).
2. `140edabf4 MOVSS XMM1,[RAX+0x268]`; `140edac03..140edac1d`: builds the time object on the stack `{[rsp+0x28]=0x1427457D8,
   float [rsp+0x30]=dt}` (a 0 is stored first at `140edac0b`, then dt).
3. `140edac5a CALL 0x140f0fc90` = `TimelineRun` with RCX = `[0x14489f348]`, RDX = the time object.
4. `140edac91 MOVSS XMM6,[RAX+0x2BC]` then `140edacd3 CALL 0x140ea40b0` (`CSLoadBalancerImp_Update`, gets the frame-rate estimate, not dt).

`TimelineRun_140f0fc90` is a tail thunk: `140f0fcd3 JMP 0x14234cb90` then `14234cb93 JMP 0x14234cb00` (the real function).
READ `14234cb00`: `MOVSS XMM0,[RDI+8]` (`14234cb2d`) then **`MOVSS [0x1447415d8],XMM0` (`14234cb32`)** (a global copy of the dt),
then `CALL [RAX+0x28]` on `[RBX+0x40]` (`14234cb5b`) with the time object (`[RSP+0x20]=RDI`). Every FD4 task of the frame, update and
draw, runs under that single call (INFERRED: the timeline is the whole frame, I-0111 §8.1). Callers of `14234cb90`: `MainLoopTick`
(`140edac5a`), `WmTimerHandler_140ed8e50` (`140ed8f84`, WM_TIMER during modal loops, it does NOT call the limiter: it re-reads the
stale `+0x268`), and `ConstDtTimeObject_140eda940` (`140eda9ba`, constant dt `[143d7ed50]` = 1/30, a loop context).

Two readers of the global copy rebuild a time object for a task: `FUN_1423c1d40` (`1423c1d6a`) and `FUN_1423c5660` (`1423c567c`):
`{0x1427457d8, float [0x1447415d8]}` (READ). So the points that carry the dt are `[time+8]` (stack object) and the global
`0x1447415d8`; one write of 0.0 to the stack float before `14234cb2d` (or hooking `14234cb00`/`14234cb90` and zeroing `[RDX+8]`)
reaches both. But it reaches every task, draw/UI/menu tasks included (fade plates, Remo, menus). INFERRED consequence.

READ, `MoveMapStep_MoveMap_1408f2ca0(step, float dt)` is the world step (data xref `1449ecc3c`, called by the step machine with the
timeline dt) and contains the game's own freeze:
- `*(step+0x270)` = "running" (`*(bool*)(step+0x270) = *(step+0xa9)==0` at `1408f3...`; the code then sets it to 0 for `step+0x98` cases,
  Remo bit 0 (`FUN_140ce27a0`==0), `[144752f80]+8` predicates `FUN_1404d86e0/8710`, `step+0x1f5`, `DAT_144780769`; to 1 for `step+0x1f6`).
- If running==0 and Remo bit 0 is clear: `[[144794eb0]+0x48]=1` (cloth manager skip, `Step_Pre_ClothManager` runs only when it is 0),
  `DAT_14476927c=1`, **`[[144799c98]+0x71]=1`** (SprjSfxImp: its Update then hands dt 0 to the FFX scene control, see section 3).
- `FUN_140607570([step+0x60], running?dt:0.0, 1)` (dt 0 when not running), `FUN_1408fbc10/1408fa910(step, running, dt)` gate on `running`.
- `RegisterTaskSetByRemoRunningBit_1408fa3e0(step, running, dt, [step+0x271])`: when running==0 it registers a REDUCED task set
  (mode 2 if `step+0x98` and Remo bit 0 clear: groups 0xE, 0x2B, 0x3D; mode 1: 0xE, 0x3D); when running it registers the full set
  (0xE, 0x12, 0x3D, 0xF, 0x11, 0x14, 0x16, 0x2B, 0x3C) on `[0x14477fdb8]`. So the game already has a "world frozen, renderer alive" state;
  it is what the Remo cutscene uses. What sets `step+0xA9` in normal play: OPEN.
- `DtSelector_140dd4b60(manager [144794eb0], {_,_,float})` (call at `1408f34c8`): fills the time objects `{real dt = [timing+0x26C]
  (!), 1/30 = [143d7ed50], 1/60 = [143d7ed3c]}` and stores `[mgr+0x38] = obj[mode]`, mode = `[mgr+0x40]` (READ). So the cloth/Havok-side manager gets
  the MEAN-OF-16 `+0x26C`, not `+0x268`, in mode 0. Mode is non-zero only if some code sets `[+0x40]` (I-0112 says no setter seen).

## 3. FFX/SFX and sky/weather clocks (Q3)

- FFX/SFX: READ `SprjSfxImp_Update_140e6e750(this, time)`: `puStack = {0x1427457d8, *(float*)(time+8)}` (the timeline dt), and
  `if (this[+0x71]) { this[+0x71]=0; dt=0; }`; then `[this+0x58]` vtable `+0x40` (scene control) is called with that dt, and
  `FUN_140e687f0([this+0xe0], &obj)`. So the FFX uses the SAME timeline dt, and dt = 0 is an already-exercised value (one frame per
  `+0x71 = 1`, which MoveMapStep sets every frame while running==0 outside a cutscene). The FFX library update path
  `GXFfxManagerUpdateJob_Slot1_14024ce30` -> `FfxLibraryUpdateInstance_141ab1680(inst, [mgr+0x58] (dt float), ...)` passes dt through
  (READ). BUT the FFX node/wind evaluation `FxNode_EvaluateWindForces_140245f90` uses a fixed 1/30 step (`movss xmm15,[143D7ED50]` at
  `140246240`, I-0112 UNLOCK-SITES row; not reread): not dt-driven (READ by I-0112, INFERRED here).
  `FxNodeManager_UpdateNodeJob_14025b1f0` calls `[node+0x170]` with dt per node (callback, per I-0066 FFX-WAVE1-READY; not reread).
- Sky/shader clock: READ the chain `D57900` task -> `FUN_140d57900(this, time)` (`140d57943 MOVSS XMM1,[RDX+8]`; `140d5794d..140d57960`:
  if `[this+0xD6]` set, clear it and dt = 0 for that call) -> `FUN_140023410(root=[144961B50], dt in XMM1)` (`140d57971`) ->
  `FUN_140126ea0(root+0x4E30, dt)` which does `[+4] += dt` (so `root+0x4E34`, the cbPerFrame `+0xB0` FC_GlobalTime that the sky, water
  and flowmap shaders read, per repo `native/world/producers.h:154`, `native/frame/simulation_time.h`) plus QPC-based `[+0xC]/[+0x10]`
  fields, and `PostEffectUpdate_1400499c0(root+0x41c0, dt)` (YEBIS). So the cloud/sky-scroll shader time is `sum(dt)` of the timeline dt: dt = 0
  stops it (READ chain; "sky motion = FC_GlobalTime" is INFERRED from the repo's A22/I-0125 notes).
- Gparam transitions (weather colour blends): READ `AdvanceTimeBlendedGparamKeys_140d250f0` takes a time object, subtracts `[time+8]`
  from `[this+0x18]`; so the blend progress also follows the timeline dt.
- Weather STATE (the storm's start/stop, zone weather ids): not reached. No function named weather/sky exists in the DB; EMEVD/world-area
  logic drives it and its clock was not traced (OPEN; I-0141 RESULT.md:3493 says "runs on the world's clock", unverified).

## 4. Divisions by dt (Q4)

- READ, guarded: `CharacterMover_1409dc090`: `fStack_80 = *(float*)(time+8); fStack_7c = 0; if (0.0 < dt) fStack_7c = 1.0 / dt`
  (`_DAT_143d7ee10` = 1.0); consumer `MoverSetProxyVelocity_1409dee60` multiplies the displacement by `[param_4+0xC]` (that 1/dt) and
  by dt `[param_4+8]`. dt = 0 gives velocity 0, no inf.
- READ, guarded: `ClothInstance_SimulationStep_140dd6690`: runs only `if (0.0 < dt)` (dt read from the manager `+0x38`, `FUN_140889bc0`).
- READ: `FUN_140126ea0` divides dt by a constant (`_DAT_143d7eeb8`), not by dt. `FUN_140eed...` limiter: no division by `+0x268`.
- NOT read (OPEN): the Havok world step (`Step_HavokWorldUpdate_Post_1407d9bd0` and the world stepper; `1414800F0` seeds an hkStepInfo-like
  `{0, dt, dt, 1/dt}` with 1/60 at `[this+0x2A4/0x2A8]`, `0x3C888889`), `ChildSolver_Step_1401fd120`, `FollowCam_Update_140515790`,
  `140515790` camera, `ValueTracker_Update_14050c7b0`. I could not exclude a bare `1/dt` there.

## 5. What this means for a world-freeze hook (INFERRED, to be validated)

1. Cheapest, whole-frame: zero the float at `[time+8]` before `14234cb2d` (hook `14234cb90` entry; RDX = time object) or write
   `[0x14489DD10]+0x268` to 0 AFTER the limiter returns (after `140edabc2`, before `140edabf4` reads it). Effects: FFX (Sprj dt), FC_GlobalTime
   (sky/cloud/water scroll), gparam blends, character mover (guarded), cloth (guarded). Side effects: every other task sees 0 as well (menus,
   fades, network keepalives driven by dt, input smoothing); `+0x26C` (mean of 16) is a separate value read by `MoveMapStep` at
   `[_DAT_14489dd10 + 0x26c]` (`1408f349x`) and given to `DtSelector` (cloth/Havok manager), so zero it too or the manager keeps stepping.
2. NOT stopped by dt = 0: per-call ramps and timers with a hard-coded step (I-0112: `140D71920` and `140D736D0` -= 1/30, `1407F5260` += 1/30,
   `1407CB390` += x/60, `140245F90` 1/30 wind step, `140EF2CE0` network timers, `1400A5AA0` children at 1/30, YEBIS gets 1/60 via `1416F27E0`),
   and the Havok step if it is fixed 1/60 (OPEN). They advance once per executed task whatever dt is.
3. The game's own "world frozen, renderer drawing" state is `step+0x270 = 0` of MoveMapStep (reduced task set 0xE/0x2B/0x3D, SFX dt 0 via
   `+0x71`, cloth skipped). Writing `step+0xA9 != 0` before `1408f2ca0` computes `running` would trigger it (also calls `FUN_1407655d0(1)`:
   `[[144763258]+0x2DC] = 1`); the meaning of `+0xA9` in normal play is OPEN (INFERRED: the pause/"system menu" flag). This mirrors what
   the Remo freeze does and avoids the per-call-step problem of point 2 (those tasks are not registered while running = 0). Validate on a
   live run before relying on it.
