# NPC dialogue and subtitles on screen, as a native checklist (I-0067, Task 7)

STATIC (Ghidra mirror of `DarkSoulsIII.exe` 1.15.2.0) + game data read offline (`01_060_caption.gfx`, `01_000_fe.gfx`, installed `msg` bundles).
Nothing was launched. Status words: **STATIC** = read in code or data, **INFERRED** = deduced, **OPEN** = not known statically.
Companion of [`FONTS-LANGUAGES-NATIVE.md`](FONTS-LANGUAGES-NATIVE.md) (which fixes the text sources and the font plumbing and left the write path OPEN).
Evidence (outside Git): `evidence/I-0067/queries/subtitles/` (`caption-gfx-names.txt`, `hud-gfx-names.txt`).

## 0. Verdict

1. **One small Scaleform page shows every NPC line and every movie subtitle: `01_060_Caption.gfx`** (3,629 bytes, class `AppMenu::CaptionMenuWindow`,
   constructor `FUN_140B43E10`, update `FUN_140B0D9B0`). It has one clip `CaptionWindow` of **256 frames** containing a text field `Text/Text`
   in the font `MenuFont_01` (the primary family of the language, `FONTS-LANGUAGES-NATIVE.md` section 4). Fade is done by **frame number**
   (`gotoAndStop(alpha + 1)`, **INFERRED** from the virtual call `FUN_140A510C0(handle, frame)` on a 256-frame clip), not by an alpha transform. The class name `AppMenu::CaptionMenuWindow` and its binding to this page come from the RTTI/vtable read in the Ghidra mirror (not visible in the exported functions); the bound clip names (`CaptionWindow`, `CaptionWindow/Text/Text`) are in the constructor strings `1428EF3E0`/`1428EF3F0`.
2. The text reaches the page through a **two-slot queue** in the menu-state singleton (`[144763258] + 0x1654`), written by five producers
   (cutscene subtitle, bik movie subtitle, two talk-message commands, one clear) and cleared by the consumer.
3. The renderer-visible work is the one of any Scaleform text: one text field, one frame change per update, glyph-cache updates when
   the string changes. **Nothing in the path is subtitle-specific for the HAL**; what differs from the menus is the *timing* (a cutscene
   subtitle is rewritten every frame with a new fade frame) and the glyph set of the language.
4. The `TalkWindow` child of the HUD (`01_000_fe`) is **not** the dialogue window and has no writer at all (section 7B); the old FD4 dialog `FUN_140699FF0` reads the same
   queue as a possible second consumer (sections 4 and 7A). Event text (category 30) goes through a third path, the old message dialogs (section 7D).

## 1. The text sources (STATIC, recap)

FMG categories of the installed bundles (`FONTS-LANGUAGES-NATIVE.md` section 7): conversation `会話` id 1 (+ 230 DLC1, 270 DLC2), movie
subtitles `ムービー字幕` id 3, event text 30 (+ 231, 271). Getters (all through `FUN_142354090(manager, 0, category, id)`):

| Kind | Getter | Probe order | Fallback |
|---|---|---|---|
| NPC talk / cutscene line | `FUN_140E1A140` | 0x68, **1**, 0xE6, 0x10E | `?TalkMsg?` |
| movie subtitle | `FUN_140E19C70` | **3** | `?MovieSubtitle?` |

The caller wrapper is `FUN_140A67150(out, id)` -> `FUN_140A672E0(out, getter, id, "TalkMsg")` -> `FUN_140A67930`: with the debug flags
`[FUN_140B3CDA0()+0x3080/0x3081]` clear (release) the output string is the FMG pointer unchanged (`FUN_1407B7830`), so **no tag expansion happens
here**; with `+0x3080` set the text is formatted `(%d)%s` (`1428B5F20`, id prefix), with `+0x3081` it goes to a debug overlay (`FUN_140B3C5F0`), and a missing text is
formatted from `143D4BBC8` (not decoded).

## 2. The queue (STATIC)

Singleton `_DAT_144763258` (constructed by `FUN_14075BFE0`, ids set to -1):

| Offset | Content |
|---|---|
| `+0x1654` | slot 0 (8 bytes): **movie subtitle** |
| `+0x165C` | slot 1 (8 bytes): **talk message** |
| `+0x1664` | byte, "frozen" flag (a push is dropped while it is non-zero, and the flag is cleared) |
| `+0x34 + 4*i` (i < 500) | event counters written by `FUN_140765220(i, v)`: 0x37 = "caption displayed" flag (the Scaleform consumer writes 0 when the string is empty and 1 at the end of the non-empty branch, the legacy dialog writes it too), 0x93 = set to 1 by the talk-message command in mode 0 |

Entry (8 bytes): `+0` int text id (-1 = none), `+4` short **fade value** (0..255; 0xFFFF when the entry is unset), `+6` byte **kind**
(0 = movie subtitle, **1 = talk message**; this byte selects the getter in `FUN_140B0D890` and `FUN_14069A330`).
Push `FUN_140765060(entry*)`: if the frozen flag is 0, kind 1 writes slot 1, kind 0 writes slot 0; the flag is then cleared.
Reader `FUN_140761530(i)` returns the slot address for `i` < 2 (0 otherwise).

## 3. The producers (STATIC)

| Producer | Entry written | What it is |
|---|---|---|
| `FUN_140CCBC90` (update of a cutscene subtitle action; the class is **INFERRED** to be `SprjRemoSubtitleAct`, whose RTTI name exists in the binary) | `{text id from a param row, fade 0..255, kind 1}` | Reads the row through `FUN_140E3EEB0` (the push happens even when the lookup fails: id -1, kind 1) (param table index 0x11 of the param manager `_DAT_144798108`, two row layouts), computes the fade from the action timeline (`+0x224` mode 0: 0xFF; 1: fade-in by `FUN_140CC1250`; 2: fade-out by `FUN_140CC1270 - FUN_140CC1250`; 3: either, chosen by comparing `FUN_14235A490` with `143D7EDB4`; any other mode: fade 0, scaled by `143D7EE7C`, `143D7EFF4` and clamped to 0..255), pushes every update |
| `FUN_140CCBC60` | `{0, 0, kind 1}` | companion call of the same action: id 0 resolves to an empty string (`FUN_140A67150` returns the default for `id < 1`), so it blanks slot 1 |
| `FUN_140EB0890` (called by `FUN_140EAFAC0`) | `{id, fade, kind 0}` | **bik movie subtitles**: walks the subtitle events of the playing movie by the movie player time (`[obj+0xC8]` virtual `+0x70`, list at `obj+0x100`, helpers `142341650/142341670/1423413d0`), the whole producer runs only if `FUN_140764130()` is non-zero, the last matching event wins, and the pushed entry is `{event id, 0xFF, kind 0}` (**no graded fade**: the 0..1 ramp is only tested for "> 0"), or the default `{-1, 0xFFFF, kind 0}` when no event matches |
| `FUN_140F08BC0(this, id)` / `FUN_140F08C30(id, mode)` | mode 1: `{id, 0xFF, kind 1}`; mode 0: a different queue (`FUN_140765E90`) plus the counter 0x93 | talk-message command (ESD/event side, `this+0x34` is the mode); full opacity |

## 4. The consumers (STATIC)

**Scaleform: `FUN_140B0D9B0` (`CaptionMenuWindow`).** Per update: take the first of slot 0, slot 1 whose id is not -1 (slot 0, the movie
subtitle, has priority); resolve the text (`FUN_140B0D890`: kind 0 -> `FUN_140A65F40` (not exported, **INFERRED** to be the movie-subtitle resolver, by symmetry with `FUN_14069A330`) then `FUN_140469DC0`; kind 1 -> `FUN_140A67150` TalkMsg then `FUN_140469DC0`);
copy the result into the page cache (`+0xAF8` text pointer, `+0xB00` owned wide string, `+0xB28` byte 1, `+0xB30` fade byte, 0xFF when the entry fade is negative; the id and the kind are not cached), write back an *acknowledging* push of the same slot with an unset id
(`FUN_140765060`, at `140B0DACC`), then:

1. `FUN_140A3D720(page+0x9D8, text non-empty)`: show/hide the whole window;
2. `FUN_140A516F0(page+0xAA0, text)`: set the text of `CaptionWindow/Text/Text`;
3. `FUN_140A510C0(page+0xA50, fade + 1)`: go to frame `fade + 1` (1..256) of `CaptionWindow`;
4. counter 0x37: `FUN_140765220(0x37, 0)` when the text is empty, `(0x37, 1)` at the end of the non-empty branch.

The slot refresh itself is unconditional. When `FUN_140BA0430()` is true (**`[[1447572B8]+0x58] + 0x10 == 0`**) a second copy from the default-entry constructor `FUN_140B0D7A0` overwrites the cache (the same constructor makes the initial cache in `FUN_140B43E10` and the "no slot pending" case): **INFERRED (strong)** that byte 0 blanks the subtitle and a non-zero byte shows it (`FUN_140B0D7A0` is not exported). The settings object is the profile settings object; its byte `+0x10` is the
only settings byte the two consumers read, and the executable carries the option key `Game.Option.Disp.Subtitle` (`14283C030`) and
`optSubtitles` (`142985CC8`): **INFERRED** that the byte is the subtitle option; the polarity above is read from the code shape and confirmed by live row CL3).

**Legacy FD4 dialog: `FUN_140699FF0`** reads the same two slots and writes the text into a layout part through `FUN_14069A330` (kind 0 -> movie
subtitle, kind 1 -> TalkMsg), animation `F04_open_BG` (`FUN_141A0D530`); it passes `[settings+0x10]` to `FUN_14069A330` as a visibility flag and opens/closes only when the byte is non-zero: same polarity as the Scaleform consumer. It is a second consumer of the same queue (both may exist at once, **OPEN** which of the two draws in the release PC build; live row CL4 shows whether the caption page is called, and the `F04_open_BG` animation call shows the legacy one).

## 5. The page (game data)

`01_060_Caption.gfx` (GFX v11, 3,629 bytes, no ActionScript tag): main timeline 1 frame; places `CaptionWindow` (sprite 3, **256 frames**) with child
`Text` (sprite 2) containing the dynamic text field `Text` (default text `シーン 1`), `font.swf` import of **`MenuFont_01`**. Two auxiliary
strings `TalkText Window1/2` are default texts of fields in the file. `01_000_fe.gfx` (the HUD, 554,296 bytes) places `TalkWindow`
(sprite 631, 19 frames, labels `FadeIn`, `Loop`, `FadeOut`, child `Text_0`) at depth 651: no code addresses
the HUD offsets `+0xB948`/`+0xB9A8` outside the constructor and destructor (section 7B): bound but not driven, not part of the dialogue queue.

## 6. What the native backend needs (checklist and rows)

| # | Requirement |
|---|---|
| G1 | Nothing subtitle-specific in the HAL: the page is ordinary Scaleform (one text field in a movie clip). The produced text draws through the same glyph cache and font mapping as every menu (`FONTS-LANGUAGES-NATIVE.md` F1-F5). |
| G2 | The fade is a **frame change of a 256-frame clip**: the Scaleform frame advance (`gotoAndStop` through `FUN_140A510C0`) must be honoured once per update, with the clip's own per-frame transform. The native must not collapse the clip to frame 1 or cache the page as a bitmap that survives frame changes (it is not marked cache-as-bitmap in the file; to be confirmed by the existing feature scan, `menu-gfx-features.tsv`). |
| G3 | During a cutscene or a movie the string is rewritten with an unchanged text almost every frame (only the frame number moves): the HAL must not rebuild or re-upload glyphs for an unchanged string (the Scaleform text layout keeps it; this is the engine's behaviour, the HAL only sees glyph-cache updates when a new glyph appears). |
| G4 | Movie subtitles depend on the movie player time (`[obj+0xC8]` vtable `+0x70`): the native presentation of `fdp_opening.bik` / `fdp_endroll_ww.bik` and of `01_920_movie` must keep that player running (it is the Bink player of the page `01_920_movie`, a separate native item). |
| G5 | Language: the string comes from FMG categories 1 / 3 of the selected language; CJK languages enlarge the glyph working set (section 5 of `FONTS-LANGUAGES-NATIVE.md`), nothing else changes. |
| G6 | The cache overwrite depends on the settings byte of CL3 (0 blanks the caption); a missing option byte or a settings object in its default state must not be "fixed" by the native. |

**Live rows (CL), read-only:**

| Row | Action | Expected |
|---|---|---|
| CL1 | During an NPC line (for example the first Firelink Shrine talk), read `[144763258]+0x165C..0x1663` each frame | `{id >= 0, 0x00FF, kind 1}` for the duration, `-1` after the consumer's acknowledging push |
| CL2 | During the opening movie (`fdp_opening.bik`), read `+0x1654..0x165B` | `{id >= 0, 0x00FF, kind 0}` while a subtitle is active, `{-1, 0xFFFF, 0}` between subtitles |
| CL3 | Read `[[1447572B8]+0x58]+0x10` and toggle the subtitle option in the menu | value change; polarity of "on" |
| CL4 | Log `FUN_140A510C0` / `FUN_140A516F0` calls on the caption page | one frame call per update (1..256), one text call per new string |
| CL6 | Break on `FUN_140701F50` and on the readers `FUN_1406FAA00`, `FUN_140700760`, `FUN_14076F460` during an event message (for example picking up an item with an event text) | a slot of the 5-entry array at `[144763258]+0x14D8` written, then read by an FD4 dialog; shows which one draws |
| CL5 | (Thomas's agreement) Steam language `japanese` or `schinese`, repeat CL1 | same queue shape; glyph updates larger at the first CJK line. **Native2063 / Reference2060** (2026-10-18, `Run-AlphaCast.ps1 -TalkKeeper -TestLanguage japanese`: Alpha talks to the Fire Keeper at Firelink): her first line subtitled in Japanese natively as in the original (`pair-talk-*.jpg`). The `caption-observe.flag` poll of +0x1654..+0x1663 saw only the empty state in both modes during the line: the queue read is not the one this talk uses (open, the picture is the witness) |

## 7. The three points closed in Task 8 (STATIC unless marked)

**A. Which window draws on the PC build.** The Scaleform caption page is opened **unconditionally** when the menu system finishes its initial
resource list: the loop `FUN_140E505E0` (it advances an index kept in `this+200` through `FUN_140E4DD60(resources, index)`, read as a completion test: **INFERRED**; the loop runs at most twice per call and, on the first index that returns 0, calls `FUN_140765620()` (unexplained, same singleton range), then) calls
`FUN_140B3D940(FUN_140B3CDA0())`, which opens `01_060_Caption` through `FUN_140B4EE00` (page name `1428EF0A8`), next to the HUD
page (`FUN_140B3C160` -> `FUN_140AFE360`, `01_000_FE`). No setting is tested around that call. The legacy dialog `FUN_140699FF0` (class functions
`140698FE0..14069A6E0`, vtable in `.rdata` near `143FB88C0`) is created by name through the runtime class registry (the string
`FrpgMenuDlgTalkMsgbox` `142800410` is referenced by data only, no code instantiates it by a visible call): **whether it is ever instantiated is
not decidable statically**. Both consumers acknowledge the slot (the Scaleform one writes an unset id back), so if both existed the first to
run would take the line. Verdict: **the Scaleform caption page is the display path to implement**; the legacy dialog is a possible second
consumer (INFERRED unused for dialogue; live rows CL2 and CL4 settle it by the presence of `F04_open_BG` animation calls).

**B. The HUD `TalkWindow` writer: none.** A scan of the code section (`140000000-143000000`) for the operands `+0xB948]` and `+0xB9A8]` of the HUD
class finds only the constructor (`FUN_140AF8580`, `140AF86D3`) and the destructor (`FUN_140AFC710`, `140AFC834`): **no direct operand reference** (a driver that walks the children through a base pointer and an index would not show; the live rows keep this open). Control: the siblings
`MapName` (`+0xBA68`) and `TextEffect` (`+0xBA08`) are also used by the HUD update `FUN_140B018B0`, `TalkWindow` and `AutoSaveIcon` (`+0xBBE8`) are
not. **INFERRED:** `TalkWindow` is a bound but never driven clip (a leftover; its timeline `FadeIn/Loop/FadeOut` has no code owner).

**C. The subtitle option byte.** The configuration keys appear in the executable in the order `Game.Option.Disp.Blood`, `.Subtitle`, `.Brightness`,
`.HUD` (`14283C000..14283C0A0`, consecutive); the settings object bytes `+0xF`, `+0x10`, `+0x11`, `+0x12` match that order in the default
constructor `FUN_1405A0320` (`+0xF` and `+0x10` set to 1 by one word store `0x0101`, `+0x11` by `FUN_14080FFA0(1)` = the brightness default, `+0x12` and `+0x13` set to 1 by another word store made earlier in the function; the code also clamps `+0xF` to 0..2, a three-state option, which fits Blood). So **`+0x10` is the Subtitle option (INFERRED, strong), default 1 = on, non-zero = shown** (both consumers use it that way, section 4). The
value is sanitized by `FUN_1408100C0`, whose bytes are not valid code in the file (encrypted at rest, as for `FUN_14080EAF0` in `FONTS-LANGUAGES-NATIVE.md`): the clamp itself is unreadable. Writers: the option-page apply functions `FUN_140AB4FC0`, `FUN_140BE4520`, `FUN_140BE4DB0` (they copy
the page's byte array, index 8 for this option) and `FUN_1405A0930`; all four copy the option bytes in the same order.

**D. Event text (category 30).** `FUN_14046BB70` probes categories `{0x65, 0x1E, 0xE7, 0x10F}` (fallback `14297EF40`). Its only caller in the cross-reference listing is
`FUN_14046ACC0` (the function also has a second case, categories `0x4E/0xEC/0x114` with fallback `14297EFD8`, and a default tree lookup), which is called with a category and an id by the event-instruction implementations (`FUN_14047CFF0`, `FUN_14047D050`,
`FUN_14047D080`, about twenty call sites in `140471C20..140497B40`). It formats the text with `FUN_140014D10` (a `sprintf` into a `std::string`)
into an accumulator object `_DAT_144751AA0` (0xB0 bytes, created by `FUN_14046AE50` through `FUN_140482770`, freed by `FUN_14046AFF0`) and sets the state
`_DAT_14454D5D8 = 3`. The per-frame `FUN_14046B260` (called by `FUN_1404788B0`) then dispatches on that state (flag `DAT_144751AA8`): state 3 ->
`FUN_14046BA90` -> `FUN_140702090` + `FUN_14046BF50` -> `FUN_140701F50`, which fills one of **5 slots of 0x40 bytes** in the menu-state singleton
(`FUN_1407649E0` = `[144763258] + 0x14D8`, current index at `+8`, text stored by `FUN_1407027D0`). The functions that read that array through
`FUN_1407649E0` (the one-line accessor may have been inlined elsewhere: direct `[144763258]+0x14D8` accesses are not excluded) are all in the code range that holds the FD4 dialogs (`FUN_1406FA590`, `1406FAA00`, `1406FA6C0`, `1406FD770`, `140700760`, `14076F460`, `14076F880`,
`14076F640`, `14076F690`, plus `FUN_14046B820/B900/BA90` themselves): no AppMenu (Scaleform, `140A..140C`) function calls `FUN_1407649E0`.
**Consequence (INFERRED):** the caption page is **not** the consumer of event text (it reads the other structure, `+0x1654`). The readers
of the 5-slot array use the FD4 dialog layout API: `(*this)->vtable+0x78` to get the layout, `FUN_141A0DEC0(layout, name)` and `FUN_141A07C30(this, name)`
to find parts by name (strings `1428031A0`, `142804C50`, `142803218`), the same API as the legacy talk dialog `FUN_14069A330`, and no AppMenu function
calls the accessor. So the event text is most likely shown by FD4 message dialogs (GX 2D layer, section 2680 of `MENUS-MAP.md`); **that these dialogs
draw in the release build, and which font they use, is OPEN** (CL6 breaks on the readers). The font source is not
identified (`DSFont24`/`TalkFont24` appear in no string of the executable, only `DbgFont14h` does; the layout data may name them).

## 7b. Residual open points

1. Whether the legacy talk dialog is ever instantiated (CL2, CL4).
2. The unreadable sanitizer `FUN_1408100C0` of the subtitle byte, and the option page that exposes it (the existing page list of section 50 has `02_040_OptionSetting`).
3. The command that calls `FUN_140F08BC0`/`FUN_140F08C30` (the ESD/Lua id of "show talk message").
4. The font that the old message dialogs use for event text, and how its glyphs reach the GX 2D layer (the natural next static step: `FrpgMenuFontPrinter`).
5. Which functions read the 5-slot event-text array and whether any of them draws (FD4 dialog or Scaleform through another accessor).

## 8. Exports

Cited functions are exported to `decompiled/` (`DS3-1152-F-<RVA>`): `FUN_140B43E10`, `140B0D9B0`, `140B0D890`, `140A67150`, `140A672E0`,
`140A67930`, `1407B7830`, `140765060`, `140765220`, `140761530`, `14075BFE0`, `140CCBC60`, `140CCBC90`, `140E3EEB0`, `140EB0890`, `140F08BC0`,
`140F08C30`, `140699FF0`, `14069A330`, `140BA0430`, `140597290`, `14046BB70`, `14046ACC0`, `140E505E0`, `140B3D940`, `140B4EE00`, `140B3EA10`, `1405A0930` (already exported), `140AB4FC0`, `140BE4520`, `140BE4DB0`, `14046AE50`, `14046B260`, `14046BA90`, `14046BF50`, `140701F50`, `1407649E0`, `140482770`, `14047CFF0`, `14047D050`, `14047D080`, `140014D10`, `140014C90`, `140AF8580`, `140AFC710`, `140B018B0`, `1406FA590`, `1406FAA00`, `140700760`, `14076F460`.
