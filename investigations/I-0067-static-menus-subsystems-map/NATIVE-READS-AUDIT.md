# Native reads of engine structures: width, type, unit, initialisation (I-0067, STATIC)

- **Scope:** read-only audit of `native/` on the pilot branch `claude/zen-lamarr-25edc3` at
  `cfbdd92e`. Line numbers refer to that commit. Engine types are proven from the binary
  (`I0067Map.java` queries) unless marked INFERRED.
- **Why:** two bugs of one family were already found:
  - camera `+0x68`, a byte read as a dword (fixed by Native1179);
  - the cursor written in output units where the engine expects the screen size S (MENUS-MAP
    section 51).
- **No fix is applied here.** The pilot decides the corrections.

## Method

1. **Mechanical inventory.** `Get-NativeReads.ps1` lists every `Read(address, &variable, size)`
   in `native/` (883 reads), with the declared type of the destination.
   - Every size equals the destination's size, except `game_bootstrap.cpp:4976`. That one is a
     deliberate 16-byte prefix read into a larger struct.
   - **So no width error exists inside the native code.** The family to audit is the agreement
     between the native type and the engine's.
   - Inventory: `evidence/I-0067/queries/native-reads/native-reads.tsv`.
2. **Engine side.** For every read that gates behaviour, plus the logging reads that disagree
   with a sibling read, the access width at the same offset was read in the engine functions
   that own the structure.
3. **Strings.** Every native read of an engine name was checked for the MSVC `basic_string`
   layout: characters inline when the capacity at `+0x18` is ≤ 7, otherwise a pointer at `+0x00`;
   length at `+0x10`.
4. **Units.** Every native use of the output extent written into engine structures, and every
   engine reader of S (`[1446601E8]`, `[1446601EC]`), was checked.

## Findings

| # | Native (file:line) | Engine field | Engine type (proof) | Native read or write | Severity |
|---|---|---|---|---|---|
| 1 | `game_bootstrap.cpp:5898..5907` `NativeMouseDeviceUpdate` | mouse `+0x20/+0x24` cursor position | int32 pair **in units of S**. `FUN_1423891E0` scales by display `+0x198/+0x19C`, which `FUN_140EF50D0` sets to S (`FUN_14000B700`). The consumer `FUN_140E65E70` divides by S | written in **output-extent** units | **High.** Confirmed live: the MessageBox 406011 click works once converted to S. The fix is not in `cfbdd92e` |
| 2 | `game_bootstrap.cpp:5945..5950` `NativeUiMovieViewport` | movie viewport buffer and rect | S (`FUN_140E53880` uses `[1446601E8/EC]`) | output extent | **Medium when the aspect of S ≠ the output's.** The draw follows this viewport, but `FUN_140E65E70` computes its pillarbox and letterbox offsets from S's aspect against the movie frame. The pointer then no longer lands where the button is drawn, even in S units. Equal aspects: no effect |
| 3 | `game_bootstrap.cpp:1297..1298` (`ProduceNativeSceneCamera`) and `4711..4712` (`UpdateNativeSceneCpu`) | camera output `+0x5C/+0x60` size | int32 S, written by `FUN_140CED670` | overwritten with the output extent | **Low to medium.** `+0x64` (explicit aspect, from camera `+0x54`) and `+0x50` (fov) stay the original's. INFERRED: if camera `+0x54` follows S and the window's aspect differs, the projection aspect and the target disagree. The source of camera `+0x54` was not read |
| 4 | `game_bootstrap.cpp:7816` `NativeUiTextureImport` | `name` argument | **pointer to a `std::wstring`** (`FUN_140E48980` passes its local string object to `FUN_140E49320`) | read as 64 wide and 64 narrow characters at the object | **Low (diagnostic).** For every name longer than 7 characters (all icon sheets) the `native_ui_asset_unavailable` row prints pointer bytes, on the path that ends in `Refuse` |
| 5 | `game_bootstrap.cpp:3752..3753` | ChrAsm entity `+0xBB0` | **dword bit field**. `FUN_140D009E0`/`FUN_140D01180`: `AND 0xFFFFFFDF`; `TEST byte 0x40`; whole-dword moves in `FUN_140D028C0`/`FUN_140D01610` | BYTE named `enabled` | **Low (logging only).** Only bits 0..7 are logged. The same field is read as uint32 at `1682` and `3774` |
| 6 | `game_bootstrap.cpp:3817..3820` | model entity `+0xC3C` | dword bit field (`FUN_140D07330` sets bit 5; `FUN_140D076A0` reads bit 4; `FUN_140D07710`/`FUN_140D08080`/`FUN_140D08190` read bit 3) | BYTE, tests bit 0 | **None today.** Bit 0 is in the low byte. Fragile if a later test needs a bit ≥ 8 |
| 7 | `game_bootstrap.cpp:3694..3695` | map piece entity `+0x388` | dword, written by `FUN_140D03EB0`; `FUN_140D04B90` tests byte bit 0 | uint32, tests bit 0 | None (equivalent) |

## Engine readers of S whose native input changed

These carry no native read of the wrong width, but they are the same unit family. Native mode
never writes S.

| Engine function | Use of S | Native situation | Status |
|---|---|---|---|
| `FUN_140E65E70` | pointer → movie stage | items 1 and 2 | confirmed |
| `FUN_140D10DE0` (vtable slot only, `144A33A0C`) | scales a position by S / (virtual size − GX viewport origin) | **skipped natively**: it runs only when the display `[144961AB0]` exists, so positions stay unconverted | debug GUI only (`SprjGuiSystem` slot 5, MENUS-MAP section 58): no retail effect |
| `FUN_140E51340` (from `FUN_140E5E680`) | UI render set-up with S as the buffer size | native UI target = output extent | initial viewport of a new movie only, replaced at its first update (MENUS-MAP section 58) |
| `FUN_1406C73B0` | HUD coordinates: × (virtual/S.w) × S.w | unit-neutral | no issue |
| `FUN_140D6CFA0`, `FUN_140D6D180`.., `FUN_140D6D0B0` | screen-capture and render-scale targets at S, S/2, S·percent | GX path, replaced natively | no issue |

## Fields the original never initialises

- **Camera `+0x69..+0x6B`:** `FUN_14004B5C0` writes only the byte `+0x68`; the three bytes after
  it are stack padding. Already handled at `scene_ingress.cpp:88..90` (byte read).
- No other read of a never-written field was found among the checked offsets. This category was
  not searched exhaustively: it needs the constructor of every structure read.

## Checked and consistent

These widths match the engine:

- camera `+0x68` byte and `+0x70` dword (`scene_ingress.cpp:90`);
- pose record `+0x68` dword (active index, `FUN_140016050` increments it) and `+0x70` dword
  (`scene_ingress.cpp:55..56`);
- model `+0x224` dword and `+0x1E0` byte (`FUN_140015490`, `FUN_140016050`);
- material `+0x250` dword and `+0x254` byte (`FUN_140140680`, `FUN_140141070`);
- material parameter block `+0x3B8` dword (`FUN_14022C4B0`, `FUN_14022E930`);
- FaceGen `+0x70` dword status 0..3 (`FUN_1408362C0`, `FUN_140836E40`);
- glyph cache `+0xFE0` dword (`FUN_141C762E0`, `FUN_141C77120`);
- UI task `+0x50/+0x54` dword (`FUN_140E4FB30`) and `+0x58` byte (`140E5041D`);
- menu TPF file capability `+0x70` byte state (`FUN_140E5C080`);
- action `+0xC4` byte and `+0xC0` float (`FUN_140EBB5B0`);
- mouse `+8/+0x10` bytes, `+0x28` int32 wheel (`FUN_1423891E0`, `FUN_140A60700`);
- wheel conversion `−wheel/120` (`FUN_140A60700`'s multiply-high is the same division);
- movie `+0x28`, a `const wchar_t*` (`FUN_140E53580` passes it as a format argument).

Strings with correct small-string handling: `game_bootstrap.cpp:2314`, `2607`, `5286`, `6201`,
`7715` (gparam names), `content/acquire.cpp:58`, `content/cpu_ingress.cpp:52`,
`frame/game_model_selection.cpp:116`.

## Not covered

- The 121 uint32 and 26 int32 reads used only in witness rows, except the ones listed.
- Reads through the other files' `Load<T>` helpers beyond the ones named above.
- Writes into engine structures, except the pointer and viewport outputs (both match the
  original's layout) and items 1..3.
