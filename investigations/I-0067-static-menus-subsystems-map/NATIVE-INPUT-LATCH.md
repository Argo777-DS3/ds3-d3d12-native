# Why the native "Load Game" confirmation sometimes fails, and the general native correction (STATIC + recorded run rows; INFERRED where marked)

Written 2026-10-11 for the failed 02h51 velocity pilot run, corrected the same day after an independent read of the run rows and of the chain.
Inputs: STATE, [MENU-OFFSCREEN-MODELS](../I-0066-independent-native-startup/MENU-OFFSCREEN-MODELS.md)
"Native1179-1181", [MENUS-MAP](MENUS-MAP.md) sections 2, 50, 51, the recorded run
`evidence/I-0066/artifacts/i66-native-velocity-1189` (`startup.jsonl`, `confirm-1..5.jpg`) and the decompiled input chain.

> **Correction of the same day (Task 11, see section 6).** The decide action `0x13` does **not** read the Win32 mouse objects of sections 1-4. It reads a bit array filled from the **DirectInput** mouse and keyboard devices (`FUN_141895C80` -> poll -> `FUN_14189CE40`). The latch of sections 3-4 is still right for the Scaleform pointer, but the action needs the latch at the DirectInput poll. `FUN_14233CDC0` named in section 1 is an **output-direction** function (it pushes action values to the object at `ctx+0x30`), not the state read.

## 0. Answer

The confirmation fails because **the game samples the mouse buttons once per update and the native update interval on the Load Game screen is 0.67 to 0.84 s**.
A press shorter than that interval (the pilot holds 400 ms) is down at one sample or at none: with probability about `1 - 0.4 / 0.7 = 43 %` per press it is down at **no** sample,
and then the game receives nothing at all (no edge, no pulse). One sample is enough: the recorded run shows that every press that was sampled once produced its decide pulse one update later.
At 60 fps (the original, the title screen) a 100 ms click spans six updates; on the native Load Game screen a 400 ms press spans less than one.
The fix is general and belongs to the native input layer: **latch every press that happens between two updates and deliver it as one effective update** (mouse buttons and keyboard),
so that no input is lost whatever the frame rate. The pilot holds are a workaround, not a fix.

## 1. The chain, with the functions (STATIC)

1. **Device update** `FUN_142340050` (device manager, vtable `1444F8600`): per device entry of one loop iteration it calls the mouse update `FUN_1423891E0` and then the
   action contexts `FUN_14233CDC0` (lines 63-66 of its export; MENUS-MAP 51 is right on this order).
2. **Mouse buttons** (`FUN_1423891E0`, export `023891E0`): one `GetKeyboardState` call per update (queue-synchronous: it reflects only the messages the game thread pumped, so only clicks delivered
   to the game window); per button the fields `+8` pressed edge, `+9` released edge, `+0xA` held, `+0xC` hold time (left; right at `+0x10..+0x14`, middle at `+0x18..+0x1C`). The edges live exactly **one update**;
   a press that begins and ends between two calls leaves no trace. The position is copied from `GetCursorPos` in the same call (native: `NativeMouseDeviceUpdate` writes it).
3. **Keyboard**: `FUN_14183BE70` polls DirectInput `GetDeviceState` (256 bytes at `device+0x1A0`) once per update; a tap between two polls is lost the same way.
4. **The decide action** (`code 0x13`, "confirm"), update `FUN_140EBB5B0`: `FUN_14233DE40(action, 0x13)` queries the binding (device state, data-driven keys or
   mouse button). If down and the timer `+0xC0` is negative (first update): timer = 0.2 s and **`+0xC4 = 1` (one pulse)**; while still down the timer falls by
   the update's `dt` (`param_2+4`, the real update time) and every time it passes zero it reloads 1/15 s and pulses again (auto-repeat); when up: timer = -1, `+0xC4 = 0`.
   The pulse is read by `FUN_140EBB510(action, 0x13)` (condition 13), which is the **click query** `FUN_140A5C010` -> `FUN_140A60A50` with the predicate `FUN_140A601B0`
   (`FUN_140EBB510(.., 0x13)`). So a menu "click" and the keyboard decide are the same action pulse.
5. **UI queries** (`FUN_140A60700` pointer, the ~30 `FUN_140A5B9D0..` wrappers) all go through the gate `FUN_140A5B930`, then read the mouse object. The pointer query reads
   the edge `mouse+8` (native `NativeUiPointerState`). The dialog window (class `MessageBoxDialog`, vtable `1429092D8`, built by `FUN_140BBBD50`, choices `FUN_140BBE790`)
   decides the selected choice on the pulse; the result chains to the load action `FUN_140AE9DD0` and `TitleStep` state 4.

**One update of lag, and what it means.** In the recorded run the pointer query shows `buttons = 1` (edge) and the action update shows the pulse **one update
later**: 60 fps phase: edge at tick 635049484, pulse at 635049500 (`updates` 882 -> 883), next update `pressed_c4 = 0`; Load Game phase: edge at 635102609, pulse at
635103281 (`updates` 1652), 672 ms later. The same one-update delay holds for all five presses of the run that produced an edge (635102609 -> 635103281, 635120656 -> 635121312,
635139250 -> 635139984, 635149687 -> 635150421, 635167687 -> 635168375). Cause (INFERRED): the binding state read by the action is cached from the previous update
(`FUN_14233DE40` -> `FUN_14233D540`, flags at node `+0x20` set after the context update, `FUN_142340050` lines 100-104), or the action's context comes before the mouse in the entry list; the call order inside one
iteration is not the cause.
**Consequence: the pulse needs the button seen down at ONE update, not two.** At 656 to 734 ms update intervals a 400 ms press is down at no more than one sample, yet five of five sampled
presses pulsed. Presses lost are those down at no sample. A "two-update stretch" would be harmful, see N3.

## 2. The numbers behind "sometimes" (recorded run, `startup.jsonl`, `input_confirm_update.updates` against `tick`)

| Phase (ticks, ms) | Updates | Interval per update |
|---|---|---|
| Title, 635045296 -> 635060359 | 641 -> 1494 | 17 to 18 ms (60 fps) |
| Load Game screen from 635064125: 1597 -> 1598 | 1 in 765 ms | 0.77 s |
| 635064890 -> 635070781 | 1598 -> 1605 | 0.84 s |
| 635097828 -> 635103281 | 1644 -> 1652 | 0.68 s |
| 635114609 -> 635121312 | 1669 -> 1679 | 0.67 s |

Reading the five tries (each = a 400 ms press on the first row `(640, 151)`, then a 400 ms press on the dialog's OK `(492, 396)`):

- **Tries 1 to 3: the row press was lost** (no `ui_event_pointer buttons = 1` at `(640, 151)`), no dialog opened (`confirm-1..3.jpg` show the list with Argo highlighted and no dialog). The OK press
  that followed was sampled (edges at 635102609, 635120656, 635139250, each with its pulse one update later) but acted on blank list area.
- **Tries 4 and 5: the row press was sampled** (`buttons = 1` at `(640, 151)`, edges at 635149687 and 635167687, pulses at 635150421 and 635168375, new UI contexts 136290756736 / 136290757248 with
  eligibility 0 -> 1 at the same updates): the dialog "Start with selected profile?" opened (`confirm-4.jpg`, `confirm-5.jpg`: OK and CANCEL, **OK highlighted**). **The OK press was then lost**:
  only `buttons = 0` rows at `(492, 396)` at 635161437 and 635179484. The run ends without `title_step_state` 4 (its rows are 0, 9, 1, 9, 2, 9 and then 10 at shutdown).
- One cause covers all ten presses: **a press that falls between two updates is not seen**. No gate closing (eligibility stays 1 for context 757440 from 635090515), no `TitleStep` state 4, no `menu_window_close` after 635055312.
- It succeeds only when both presses of a try happen to be sampled, which is why earlier runs (130 to 170 ms updates in game, Native1181) loaded after "one or two presses".
- The earlier reading "auto-repeat from a long hold dismisses the dialog, default cursor on the cancel side" is **not supported**: the screenshots show OK highlighted, and the row pulse at 635168375 with the dialog open (try 5)
  changed nothing visible (the click query hit-tests the position, which is not on a choice). Auto-repeat is therefore only a nuisance (a second pulse at the same place), not a dismissal.

## 3. Why it is native-specific and general

- Original at 60 fps: a normal click (60 to 150 ms) is 4 to 9 updates long, the chain above never loses it. Only taps shorter than one update (about 17 ms) are lost, which humans do not produce.
- Native: the interval is a function of the native frame cost (title 17 ms, in game 130 to 170 ms, Load Game portraits 0.67 to 0.84 s). Every digital input of the game (menus, attacks, interaction) has the
  same dependence; the Load Game confirmation is the first place where it cannot be worked around by holding longer.
- The cost of the Load Game frames is a separate performance item (portrait views drawn natively every frame): not part of this fix, but it sets how bad the symptom is.

## 4. Native correction (general): latch

Ordered steps. Scope: mouse buttons first (the pilot's path), then keyboard (same latch object), then any other device that polls per update.
Principle: **every physical press that begins and ends between two updates is delivered as one effective update (edge then release), and nothing else changes.**

| # | Step | Rule | Check |
|---|---|---|---|
| N1 | **Physical observer.** Default: a hook on the game window's message pump that records `WM_LBUTTONDOWN/UP`, `WM_RBUTTONDOWN/UP`, `WM_MBUTTONDOWN/UP`, `WM_KEYDOWN/UP`, `WM_SYSKEY*` with their ticks (exact times, and only input addressed to the game window, exactly what `GetKeyboardState` shows today). Fallback only: a sampler thread on `GetAsyncKeyState` at 500 Hz or more, **gated on the game window being the foreground window** (that call sees every physical press on the desktop, so without the gate the user's typing in another window would reach the game). For each input keep `phys_down` and `latch` (set at a down event, cleared when consumed) | no dependence on the game thread's cadence; no input from other windows | unit test on a synthetic timeline |
| N2 | **Latch state per input** `{phys_down, latch, eff_prev}`, written by the observer, read once per device update by the wrapper, under one lock | one writer, one reader | |
| N3 | **Effective state** at an update: `eff = phys_down OR latch`; `latch` is cleared by the update that consumes it. A press shorter than the interval therefore yields exactly one effective update; `eff_prev` ensures it is followed by an update with `eff = false` (a release edge), so two presses between two updates merge into one (accepted). **There is no minimum of two updates**: the recorded run shows one down sample yields a pulse, and a second forced update would make the action see "down" at two consecutive updates, with `dt` = 0.67 s above the 0.2 s timer: `FUN_140EBB5B0` would reload 1/15 s and pulse a second time on every short click | one effective update for any press shorter than the interval; unchanged behaviour for presses spanning two or more updates (including the original's own auto-repeat for a real hold) | sequence tests below |
| N4 | **Mouse application** in `NativeMouseDeviceUpdate` (already a wrapper of `23891E0`), placed **before** its early returns (reference mode `display != 0`, stop/closed; its lines 6412-6414) so that reference mode keeps the original behaviour (NI6): after the original update, for each button compare `eff` with the state machine's `+0xA` (held): `eff && !held` -> `+8 = 1`, `+0xA = 1`, `+9 = 0`; `!eff && held` -> `+9 = 1`, `+8 = 0`, `+0xA = 0`, `+0xC = 0`; else leave the original's fields (they already follow the held case, `+8 = 0`, hold time accumulates). The fields `14233CDC0` reads right after the mouse update are the ones written here, so the hook is at the right place | the original state machine runs unchanged | edge rows below |
| N5 | **Keyboard application**: wrapper after `FUN_14183BE70` (the DirectInput poll, `GetDeviceState(0x100, device+0x1A0)`) setting the high bit of the 256-byte buffer at `device+0x1A0` for every latched key, scancode from `MapVirtualKey(vk, MAPVK_VK_TO_VSC)` (extended keys: scancode `+0x80`). Same `eff` rule | keys the game does not bind are not forwarded (filter to the binding table or to the keys the DirectInput buffer ever showed down) | tap `E` for 20 ms on a frame of 0.7 s: the action sees it |
| N6 | **Do not change the action update or its auto-repeat** (`FUN_140EBB5B0`): a short click yields one pulse; a real hold longer than one update repeats as in the original (at 0.67 s per update that is one extra pulse per update after the first, as the original would produce) | | click 60 ms: one `input_confirm_update` transition `1 -> 0` |
| N7 | **Diagnostics**: one row per latched-only update `{event: native_input_latch, tick, device, input, phys_down, latched, forced}` (bounded: first 64 plus a heartbeat), and the count of presses seen only through the latch | proves the latch worked | rows in the next run |
| N8 | **Pilot** (interim, before N4): a fixed hold cannot be both reliable and single-pulse at 0.67 s per update (a hold covering one sample for sure, above 0.84 s, also spans two samples and auto-repeats). Instead press, **poll `startup.jsonl` until the `ui_event_pointer` row with `buttons = 1` at the target appears, then release at once**: the release then falls before the next update and the game sees exactly the latch case. This applies to the row press and to the **OK press alike** (tries 4 and 5 lost the OK press). After N4, replace the holds of `Pilot-LoadGameThomas.ps1` and the title-flow clicks by 50 ms presses | the pilot stops needing retries | 20 consecutive loads |

Reference sequences (updates at `t = 0, 0.67, 1.34 ...`, one update lag of the action as in the recorded run):

| Physical press | Without latch | With latch (expected effective updates) |
|---|---|---|
| `0.10 .. 0.20` (between updates 0 and 1) | nothing | update 1: down + edge (pulse at update 2), update 2: up + release edge |
| `0.60 .. 0.70` (spans update 1) | edge at 1, down at 1 only (pulse at 2, as the run shows) | identical |
| `0.00 .. 0.40` (the pilot's hold) | edge at 0 if the press began before it, else nothing | edge at the next update, one pulse |
| `0.00 .. 1.50` (a real hold) | as the original | unchanged; auto-repeat as the original |

## 5. Witnesses and live rows

| Row | What | Expected |
|---|---|---|
| NI1 | Unit test of N1-N3 with the four sequences above and two clicks 0.3 s apart inside one interval | the table's effective updates; the two clicks merge into one effective update |
| NI2 | Native Load Game with a 50 ms press on the first row, then on OK (cursor placed first), 20 runs | dialog opens and `title_step_state` 4 each time; `native_input_latch` rows with `latched` on updates of 0.67 s |
| NI3 | Same at the Title (17 ms updates) | unchanged: no latch-only update (a 50 ms click is already 3 updates) |
| NI4 | In game (130 to 170 ms updates): a 40 ms `F` tap | the interaction fires once (the old behaviour lost it a fraction of the time) |
| NI5 | Hold `W` for 3 s | unchanged movement; no latch-only update |
| NI6 | Reference mode (original renderer): all rows equal the original (the wrapper must not change the original's behaviour at 60 fps: with presses of 100 ms no latch-only update appears; the latch sits before the early returns of `NativeMouseDeviceUpdate`) | no `native_input_latch` rows with `latched` |
| NI7 | Typing in another window while the game runs | no input reaches the game (N1 gate) |

Open (not decidable statically): the binding of action `0x13` to the mouse button and to keys is data (`FUN_14233DE40`); which keyboard keys decide in the title is unknown (E, Enter and Space did not decide in Native1179; `F` is used elsewhere by the pilots).
The exact cause of the one-update lag (binding-state cache versus entry order) is INFERRED, and the correction does not depend on it.
The hover selection of the dialog list (`FUN_140A43160` hover branch, input-mode byte `[[14489F358]+0x18]`) was not read to the end: the pilot places the cursor first, which is also what the latch preserves.

Exports: `decompiled/DS3-1152-F-` + `023891E0`, `00EBB5B0`, `00A5C010`, `00A60A50`, `00A60700`, `00A5B930`, `00A5BF70`, `00BBBD50`, `00BB7040`, `00A43160`, `00A44DE0`, `0183BE70` (if present), `00EBB510`.

## 6. Correction: where the decide action really gets its press (STATIC, Task 11)

**The chain from the physical button to action `0x13` (all STATIC; function names from the exports and the Ghidra mirror):**

1. **Device update, once per manager update.** `FUN_142340050` (device manager, vtable `1444F8600`) first calls `FUN_14234FF10(holder, dt)` for the four device holders (`mgr+0x18..0x30`). Inside it, after the feature query, `CALL [RAX+0x38]` at `1423500E0` is **`Device::Update(dt)` = `FUN_141895C80`** (taken as slot 7 of the device vtable; it is followed by a tail call into `FUN_142388FB0`). Only after the four devices, per group (and when `mgr+0x130 == 0`), does it call the Win32 mouse object `FUN_1423891E0` and the output update `FUN_14233CDC0`.
2. **`FUN_141895C80(device, dt)`:** `elapsed += dt`; if the device pointer is missing (slot 2 returns non-zero when `device+0x198` is null) it retries at most once per `+0xCC` seconds (default 1.0, set by `FUN_141895170`) through the virtual at `+0x110`; otherwise **every update**: `FUN_141895F20` clears the state arrays, then the virtual poll at `+0xD8` (slot 27), and **only if the poll succeeded** each profile attached to the device maps its bindings (`FUN_141898190` -> `FUN_1418980C0` (not exported) -> the profile mapper, `FUN_14189D6E0` -> `FUN_14189CE40`, twin `FUN_14189D6A0` -> `FUN_14189CAD0`). **At the end of the update, on the success path and on the failure path alike, `memcpy(device+0x50, device+0x180, [device+0x48])` (`FUN_141FD0F30`, at `141895EE3` and `141895D95`) overwrites the array at `+0x50`**: what `FUN_14233D540` reads at `[[ctx+0x10]+8]+0x50` is therefore the content of `+0x180`, the array written by the twin `FUN_14189CAD0`, unless `FUN_14189CE40` runs from a caller outside this update (**OPEN**). On a failed poll no mapper runs, the slot-10 handler runs and the copy still happens, so the bits keep the last mapping left in `+0x180` (**not shown to be cleared**).
3. **The mappers** `FUN_14189CE40` / `FUN_14189CAD0` walk the profile's bindings; for each one they ask the device `vt+0xF8(code)` ("is this code down"), then the modifier virtuals `vt+0x128(m1)` (base class: `m == 0`) and `vt+0x130(m2)` (base class: false), and set the bit of the binding's **input id** in the device's bit array: `device+0x50` (`param_2[10]`) for `14189CE40`, **`device+0x180` (`param_2[0x30]`) for the twin `14189CAD0`**; with the final copy of step 2 the `+0x180` content ends up in the array at `+0x50`, which is the `[[ctx+0x10]+8]+0x50` read by `FUN_14233D540` for the action test. Both mappers gate on the same `vt+0xF8(code)`; they also keep a previous-frame list for the "sticky" binding flag (`flag & 1`), not described here.
4. **The mouse device** is the DirectInput class with vtable `143A10018` (constructor `FUN_14183C050`): `SetDataFormat`, `SetCooperativeLevel(hwnd = [obj+0x190], 0xA)` = `DISCL_NONEXCLUSIVE (2) | DISCL_BACKGROUND (8)` by default (`0x6` = `NONEXCLUSIVE | FOREGROUND (4)` when `[GlobalObj+0x250]` is non-zero; DirectInput values: exclusive 1, non-exclusive 2, foreground 4, background 8; an earlier draft of this section had them swapped), `SetProperty`, `Acquire`. Its poll (slot 27, **`FUN_14183C540`**) calls `GetDeviceState(0x14, obj+0x1A0)` (a `DIMOUSESTATE2`): axes at `+0x1A0/+0x1A4/+0x1A8` converted to floats at `+0x1B8..`, **buttons `rgbButtons[0..7]` at `+0x1AC..+0x1B3`**. Its `vt+0xF8(code)` (`FUN_14183C280`, a tiny function Ghidra did not create) is a jump table over codes 0..7 that tests bit 7 of the corresponding button byte (`TEST byte [obj+0x1AC+code], 0x80`; bytes read from the image). **The left button is therefore read from the DirectInput buffer, not from `GetKeyboardState`, not from `GetAsyncKeyState`, and not from the Win32 mouse object `FUN_1423891E0`.**
5. **The keyboard device** is the DirectInput class `143A0FD08` (constructor `FUN_14183B7F0`): poll `FUN_14183BE70` = `GetDeviceState(0x100, obj+0x1A0)`, key down = bit 7 of the scancode byte; same mapper, same bit arrays.
6. **Failure mode.** Both polls treat **any** failing `GetDeviceState` (negative `HRESULT`) as fatal for the device: the pointer `+0x198` is set to 0 and the device stays dead until the once-per-second re-creation succeeds (`DIERR_INPUTLOST` only adds an `Unacquire` before the same nulling). The constructor ignores the result of its initial `Acquire`, so a failed first acquire gives `DIERR_NOTACQUIRED` on the first poll and the same dead state. With the default background level a lost focus alone does not unacquire the mouse. Whether the native build hits this is **INFERRED only** (live rows DI1-DI2 settle it). While the device is dead or the poll fails, no mapper runs and **the action cannot see a new press**, whatever the Win32 objects say.

**Where a 50 ms click is lost when updates are slow (STATIC):** at the poll in step 4. The array is rebuilt every update by the mapper from the **instantaneous** DirectInput button state at that poll (`FUN_141895F20` resets the analog vectors; the final copy publishes the array); a press that begins and ends between two polls leaves no trace. At 60 fps a click spans several polls; at a 0.7 s update the chance that a 50 ms click covers a poll is about 7 %. The Win32 mouse object (sections 1-3) is sampled microseconds later in the same `FUN_142340050` call (`GetKeyboardState`), so a press is seen by both or by neither; the pointer and the action differ because they read **different sources**, which is why the pilot's forced Win32 fields changed the pointer but never the action.

**General correction (replaces N4 for the action path; N1-N3 keep their meaning):**

| # | Step | Rule |
|---|---|---|
| D1 | **Hook the DirectInput polls**, not only the Win32 object (and check DI7 first): wrap `FUN_14183C540` (mouse, vtable `143A10018` slot 27) and `FUN_14183BE70` (keyboard, vtable `143A0FD08` slot 27), by patching the vtable entries (`+0xD8`) or an inline hook. | The original runs first. |
| D2 | **After a successful original poll** (non-zero return), for each latched physical press not seen in the buffer: set bit 7 of the matching byte (`obj+0x1AC+button` for the mouse; the scancode byte at `obj+0x1A0+scancode` for the keyboard) for this one update; the original release comes at the next poll, which gives the edge pair the action expects. | Exactly one effective update per merged press (N3); never force a failed poll. |
| D3 | **Do not touch `FUN_14189CE40`, the bit arrays or the action update.** The mapper and the action timer (`FUN_140EBB5B0`, 0.2 s first repeat) stay original: a short click then yields one pulse. | Keeps auto-repeat as in the original. |
| D4 | **Observer** unchanged (N1): the same press counts feed the Win32 latch (pointer) and the DirectInput latch (action). | One observer, two consumers. |
| D5 | **Diagnostics:** one bounded row per forced DirectInput update `{event: native_input_latch_di, device: mouse or keyboard, button or scancode, poll_ok, forced}`. | Proves the path. |
| D6 | **If the DirectInput device is dead in the native build** (pointer `+0x198` null, polls failing), fix that first: the window handed to the constructor (`[obj+0x190]`) must be the top-level window that is the foreground window, with the cooperative level the build intends; the latch cannot help while the poll fails. | Check with the live rows. |

**Live rows (DI), read-only:**

| Row | Action | Expected |
|---|---|---|
| DI1 | On the Load Game screen, read the mouse device (vtable `143A10018`): `+0x198` (non-null), `+0x190` (the game window), and `+0x1AC` during a long physical left press | `+0x198` non-null; bit 7 set while pressed |
| DI2 | Break on `FUN_14183C540` and log its return value and the buffer `+0x1A0..+0x1B3` each call, for 10 s with 2 s presses | return 1; button byte `0x80` while pressed |
| DI3 | Same on `FUN_14183BE70` with the key `E` | scancode byte `0x80` while pressed |
| DI4 | Read the left-button bit in `[[ctx+0x10]+8]+0x50` for the menu context (`FUN_14233D540`) around the click | set for the poll that saw the button down, clear at the next |
| DI5 | With D1-D2: a 50 ms press on a row, then on OK, 20 runs | dialog opens, `title_step_state` 4, `input_confirm_update` pulse each time |
| DI6 | Reference mode (original renderer), 60 fps | no forced DirectInput update (a 100 ms click spans polls) |
| DI7 | When the `+0x50` bit is clear, `FUN_14233D540` falls back to a second table: input id -> `[[ctx+0x28]+0x40]` map -> `[[ctx+0x10]+0x60]` map -> a byte; read that byte for the left button around a click | shows whether the mouse button reaches the action through this fallback (its writer is not identified: **OPEN**) |

**Open (not decidable statically):** the writer of the fallback table `[[ctx+0x10]+0x60]` of `FUN_14233D540` (the left button could reach the action through it instead of the bit array: then the latch belongs there, DI7); that `[[ctx+0x10]+8]` is the DirectInput device object (assumed from the layout); which contexts hold action `0x13` and with which device (keyboard context, mouse context, or both: the binding data decides); whether the native window is the foreground window with cooperative level `10` when the mouse device is created (DI1-DI2); the trigger of the failure handler `vt+0x50`.

Exports added: `decompiled/DS3-1152-F-` + `01895C80`, `01895F20`, `01898190`, `0189CE40`, `0189CAD0`, `0189D6E0`, `0183C540`, `0183BE70`, `0183C050`, `0183B7F0`, `01895170`, `0234FF10`, `02340050`, `0233CDC0`, `0233D540`, `0233DE40`, `0233FF00`. The bytes of the tiny vtable functions Ghidra did not create (`0183C280`, `0183C240`, `01895A80`, `01895A70`) are in `evidence/I-0067/queries/input-di/vtable-bytes.txt`.
