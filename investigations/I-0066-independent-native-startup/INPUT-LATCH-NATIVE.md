# Native input latch (menus workstream, started 2026-10-11 08:05, branch `claude/menus-input-latch` from main fa87a972)

Specification: I-0067 [NATIVE-INPUT-LATCH](../I-0067-static-menus-subsystems-map/NATIVE-INPUT-LATCH.md) (N1-N8, NI1-NI7).

## What is in the build

- **Observer (N1):** a `WH_GETMESSAGE` hook on the game window's thread, installed at the first native mouse update (event
  `native_input_latch_observer`), removed when the native session stops. Each `WM_*BUTTONDOWN` (and double click) removed
  from the game window's queue increments that button's press count; `native_mouse_down` follows down/up for diagnostics.
- **Mouse objects (N3/N4):** 23891E0 runs once per update for each of the game's **three** mouse objects (Native1229: three
  addresses per update, each with its own state machine), so the latch is per object: after the original update, a press
  the object has not seen and its state machine does not hold (it began and ended between two updates) gets one forced
  press update (`+8` edge, `+0xA` held, `+9` clear); the original makes the release edge at the next update. Presses since
  the last update merge into one. Event `native_input_latch` (`device: mouse`, object, button), bounded.
- Native1230-1231: every short (50 ms) pilot press on the Load Game screen is forced on all three objects, and the UI
  pointer query (`ui_event_pointer`) sees `buttons = 1` at the row and at OK.

## The first search for the decide source (Native1225-1233)

- With only the Win32 objects latched, the decide action (`0x13`, `input_confirm_update`) never pulsed for a forced press
  (0 loads in 25 tries). Not the source either: the GetAsyncKeyState mouse state 1423998F0 (a latch there forced nothing,
  that path is not polled on these screens; removed). Other GetAsyncKeyState/GetKeyState readers in the image: 140723FE0
  (modifier keys), 140A54570, 140A54CD0, 140EF5C61.

## The decide action: DirectInput (NATIVE-INPUT-LATCH §6, static thread, D1-D6)

- The decide action reads the DirectInput devices: mouse poll `14183C540` (`GetDeviceState(DIMOUSESTATE2)`, buttons at
  `+0x1AC..`, bit 7) and keyboard poll `14183BE70` (256 bytes at `+0x1A0`), once per `Device::Update` (`141895C80`), mapped
  into the binding bits by `14189CE40`/`14189CAD0`.
- **D1-D2 natively:** both polls are wrapped (inline hooks, RVAs pinned). After a successful original poll, a press counted
  by the observer since that device's last poll and absent from the buffer sets bit 7 of its byte for that one poll (mouse
  button bytes; keyboard scancode bytes, extended keys +0x80, auto-repeats ignored); the next poll releases it. A failed
  poll is never forced. Event `native_input_latch_di` (bounded). Four DirectInput mouse devices are polled per update.
- Native1235-1247, Load Game with 50 ms presses: every press reaches the action (pulse one update later), the dialog opens
  on the row press and the OK press loads Argo. The load completes up to 9 s after the OK pulse at native speed; the pilot
  now waits for `title_step_state` 4 (20 s) instead of 8 s, which had read 1235-1242 as failures on try 1 (Native1241's own
  trace shows the load from try 1). **Native1243-1247: 5 of 5 loads on the first try**; Reference1238 (original, 50 ms):
  first try. A two-update hold of the Win32 objects (1239-1242) came from that misread and is not kept.

## Open

- Keyboard latch, NI3-NI7: witnessed (sections below).
- The GetAsyncKeyState mouse state (1423998F0) is not latched.
- Pilot: `-PressMs 50` works natively; the default stays 1200 until the other pilots move; `-SettleMs` (cursor placement
  before a press, default 600) added.

## Pre-merge review (fresh Opus 5.5 agent, high effort, D-0163)

Nothing blocking (no D3D11, bounded diagnostics, hooks only in native mode, one game thread for the polls and 23891E0, no
stuck inputs, the module is pinned so a hook left at exit cannot dangle). Fixed: the DirectInput polls return their bool in
AL, so the wrappers test the low byte (a failed poll kept a non-zero HRESULT in RAX and was forced) and return the original
value; with swapped mouse buttons the logical button is mapped to the physical DirectInput button; the observer counts only
messages whose root window is the game window. Recorded: Pause, NumLock and Print Screen scancodes come out swapped (not
bound by the menus); a press that lands on a failed poll is consumed, not carried to the next good poll. Native1248 after
the fixes: Load Game with 50 ms presses loads on the first try.

## Keyboard and title witnesses (Native1249, Reference1249)

- Load Game decided with the key `E` (120 ms taps, pilot `-ConfirmKey 69`): the original and native both load on the first
  try; natively the keyboard latch forced the two taps (`native_input_latch_di` `device: keyboard`, 8 rows: each tap on every
  polled keyboard device that had not seen it). The decide action is bound to `E` too, which settles the open key binding.
- NI3 (title at about 60 updates per second): the first forced DirectInput update of Native1243 and 1249 comes after the
  title's notices are closed, on the Load Game screen; the title clicks needed no latch.
- NI6 by construction (the hooks sit in the native list only); NI7 by construction (root-window filter). NI4/NI5 (in-game taps
  and holds) not run.

## In-game witnesses (Native1253-1254, Load Game on Argo, pilot `-InGameTaps`)

- **In-game menu with short presses.** The game runs at about 5 frames per second (204 ms updates).
  - A 50 ms Esc opens `02_000_IngameTop`, and a second 50 ms Esc closes it, in both runs. Before the latch, a 2.5 s hold
    was needed (Native1156).
  - In Native1254 the latch forced both Esc taps (`native_input_latch_di` keyboard input 1, one row per polled device),
    and the menu opened 235 ms after the forced poll.
  - In Native1253 the taps fell inside a poll and needed no forcing.
- **NI4.** A 50 ms F tap was forced once per keyboard device in Native1253 (input 0x21), and caught without forcing in
  Native1254. Its in-game effect is not visible at this spot.
- **NI5.**
  - With the forward key held 3 s, the player walks forward to the summon sign prompt (Native1254), and no latch row
    appears for the held key.
  - The pilot's keyboard is AZERTY: forward is VK `Z`. Native1253 held VK `W`, the key at QWERTY Z's place, and the
    player did not move.
- **Diagnostics.** Since 1253 the latch rows are bounded per device, so keyboard rows still appear after the Load Game
  mouse rows.
- **Not caused by the latch:** closing the window in game ends both runs on the known area-reload refusal
  `native_env_region_cpu_generation`, which is still open.

Pre-merge review (fresh Opus 5.5 agent, high effort, D-0163): nothing blocking; claims checked against the 1253/1254 traces and screenshots; the latch row bound is per device type (4 rows per keyboard tap); comment fixed (about 200 ms per in-game update).

## Quit in game: the area-reload refusal (Native1257, reverted attempt)

- Closing the window in game ends on `native_env_region_cpu_generation` (Native1253-1256).
- Native1257 tried treating a stale handle as the default region, as 652D0/65320 do for an invalidated handle: they
  return the default region `root+0x4330`.
- **What the run showed:**
  - The refused handles have a **matching generation with a null record** (`handle` 65578.., slot generation 1): the
    slot was released while its wrappers were still alive.
  - The original's 652D0 would return that null record, so the original never reaches this state. The native release
    order of env regions differs from the original's.
  - With the fallback, teardown continued and faulted at `B4F2`: the object at `[rbx+0x380]` is null in the world/character
    unload path (callers `8EA1AC`, `9080..`).
- The attempt was reverted (e6391fbc), so the refusal stays explicit.
- **Next:** an RCA of the native area-unload order. Read the env-region release (650F0/66640) and who frees the record
  before the wrapper, against a Reference read of the same quit. Then the `+0x380` owner.

### Root cause and fix (Native1258-1260)

- **Native1258** carried a witness with a brace error: every env-region request was refused and the game stopped
  during the load. The error was corrected for 1259.
- **Native1259 witness:** the stale handle has a matching generation and a null slot record. It comes from the
  map-part teardown (stack RVAs `8F4F9B -> 6054E4/604B26 -> 621CDB/621596 -> 60FFF6 -> 61B08B -> 61DEFF -> 61E687 ->
  652140 -> D09376 -> D09F47 -> 664D0`).
- **Cause:** `664D0`, the env-region release, clears the slot's record and only then destroys the wrapper through its
  slot 0 (`652A0`), and frees it. The native `652A0` hook first validated the live record, which is already cleared on
  this path. The original `652A0` checks nothing (INFERRED from its 16-byte pin: it re-stores the vtable, as a destructor does).
- **Fix (1260):** the native wrapper retirement requires only the wrapper's identity and registration. The retired
  wrapper leaves the registry, because its memory is freed right after.
- **Native1260:**
  - The refusal is gone: at least four wrappers retired (the log is capped at four), no stale handle, no refusal.
  - The quit then faults at `B4F2` with a **null object** (`rbx = 0`, the function's `this`). It reads `[this+0x380]`.
    The caller chain is `8EA12C -> 8E93BB -> 8E8CDC -> 9084E3 -> 23C1DA5 -> 23C213D -> 23938B8 -> 2375AC3 -> 234CB5E ->
    EDAC5F -> F14C0C`.
  - Native1257's fault was the same (`8EA1AC` chain). It is a separate native gap: a member that is never built
    natively reaches the unload path.
- **Next (new tranche):** read `8EA0..8EA1xx`, `8E93BB` and `8E8CDC` to find which pointer is null natively, against a
  Reference read of the same quit.

Quit RCA pre-merge review (fresh Opus 5.5 agent, D-0163): nothing blocking. The 664D0 order was confirmed from `env-region-1145`, and the 66640 path is also handled. Refusing `flags != 0` stays correct. Erasing the retired wrapper also fixes a latent duplicate-address refusal. The note wording was fixed (retired count capped, the 652A0 claim marked inferred).

### Quit in game closes cleanly (Native1261-1262)

- **`B4E0`, the old GX device sync** (Native1261). Its callers pass the old display `[144961AB0]`, or the device itself in its own methods. Both are nil or absent
  natively. The function flushes the device's command queue `[device+0x380]` unless it is called on the render thread.
  - Natively no GX queue exists, so it is an explicit omission: a nil device does nothing; a device (never present
    natively) is refused.
  - Event `native_gx_device_sync_omitted`. Native1261 recorded three calls, from callers `8EA12C`, `62D1F3` and `DFF805`.
- **`17E30`, the active render context publication** (Native1262).
  - The original stores its argument in `[144961BF8]` when the root has `+0x4DD8`. The area teardown publishes nil.
  - Natively nil now withdraws the active context, its camera and its selection (`native_scene_context_withdrawn`)
    instead of refusing.
  - A non-nil foreign context is still refused, with the caller logged.
- **Native1262:** Load Game, then the window closed in game.
  - The game unloads (`02_903_NowLoading2`) and goes back to the startup logos (`05_901_Logo_BNE`), as Reference1255 does.
  - The process exits with code 0, no fault and no refusal. **The quit in game closes cleanly natively.**

Quit pre-merge review (fresh Opus 5.5 agent, D-0163): nothing blocking. The hook is in the native list only. The nil no-op is safe: native resources retire under native fences. The withdraw runs under `scene_view_cpu_lock`. Recorded: a selection still in flight could republish a stale scene after the withdraw (an existing race, also on the retire path). An area change without a quit, with the context withdrawn and then published again, is not witnessed.
