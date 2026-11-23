# Menu tests and base-game coverage: static preparation (I-0066, thread « Préparation statique », 2026-10-16)

STATIC only. Nothing was launched, no save or option file was touched, `main` was not touched. Sources read: STATE and ROADMAP of 2026-10-14,
[FONTS-LANGUAGES-NATIVE](../I-0067-static-menus-subsystems-map/FONTS-LANGUAGES-NATIVE.md), [SUBTITLES-DIALOGUE-NATIVE](../I-0067-static-menus-subsystems-map/SUBTITLES-DIALOGUE-NATIVE.md),
[MENUS-MAP](../I-0067-static-menus-subsystems-map/MENUS-MAP.md) sections 50, 77, 78, [MENUS-NATIVE-GAPS](../I-0067-static-menus-subsystems-map/MENUS-NATIVE-GAPS.md),
[ENDING-CREDITS-NATIVE](../I-0069-static-menu-audio-map/ENDING-CREDITS-NATIVE.md), [DISPLAY-SWAPCHAIN-NATIVE](../I-0068-static-yebis-ffx-lights-spec/DISPLAY-SWAPCHAIN-NATIVE.md),
[D3D12-PHASE-G](D3D12-PHASE-G-GRAPHICS-OPTIONS-SHEET.md), the module (`native/frame`, `native/renderer`), the exports in `decompiled/`, the executable file (read-only), and
Reference1715's shots. New decompilations (read-only mirror, no analysis): `FUN_140EF6990`, `140EF7380`, `140EF1C50`, `140EF1C30`, `1404C5350`.
Status words as everywhere: **STATIC** read in code or data, **INFERRED** deduced, **OPEN** not known statically.
New tool: [Get-MenuPageCensus.ps1](Get-MenuPageCensus.ps1) (page names and references of the executable; section 3).

## 0. What is ready, what needs a decision

| Subject | Ready | Needs |
|---|---|---|
| FL5-FL7 and CL5 (Steam languages) | expected values per language, the exact run sequence, the pilot fix for the French notice, the native glyph path read (no cap that a CJK run reaches), rows to compare | Thomas's yes for any language beyond french, japanese, schinese, danish (section 1.6 lists four more that D-0153 asks for) |
| Credits through flag 20 | the full original chain re-read in the exports, the call signature checked on the disassembly, the aid's specification (`witness-flag.txt`), the gates, the run plan | an implementation of the aid (about 30 lines, the `witness-teleport.txt` pattern); a D-0116 announcement (the route is not read-only) |
| Audit that every menu is in the code | 104 page names in the executable, 243 references, a classification, the list of HAL refusals a page can hit, the witness queue by feature family | nothing; the only OPEN item is two names without a movie (section 3.4) |
| Resolutions and full screen under one rule | the rule, the census of the places that hold a size, the engine's request chain re-read, the test matrix, the limits of the test machine | the rule has no native implementation yet (section 4.2): it is a work item, not only a witness; Thomas's choice for 1440p and 4K on a 1080p desktop (section 4.5) |

## 1. Steam languages: FL5, FL6, FL7, CL5

### 1.1 What the test aid does (STATIC, module read)

`test-language.flag` next to the module holds a Steam name; `NativeTestSteamLanguage` (`game_bootstrap.cpp:818-849`, hook installed at `14640`) replaces the result of
`FUN_140EF3720` in both modes, so Reference and Native runs compare. It returns the id of the named language in the engine's 26-name table; the rest of the chain (id table `1429A24F0`,
filter `FUN_140EA61F0`, files, fonts) is the engine's. Reference1715 (french, m40, 2026-10-14 16:00) proved the chain: Steam id 6 replaced by 8, FL1 `region 1, flag 0, text 2, live 2, single_sku 0`,
and the original's title, both notices and main menu in French (`shots/`). The hook is called three times at startup, all with the same answer (rows in `startup.jsonl`).
Two limits of the aid: it knows only the 25 Steam names (no `latam`, so ordinal 6 `spaAR`, which needs id 27, cannot be selected), and a saved profile value other than -1 would override it (FL8; not the case in Reference1715: text 2).

### 1.2 Expected values per language (STATIC, FONTS-LANGUAGES-NATIVE sections 1-3; FL1 read at 20 s after startup)

| Test name | Steam id | Ordinal (`live`) | Folder | Fonts (`MenuFont_01` / `MenuFont_02`) | Plus string, digits | Status |
|---|---|---|---|---|---|---|
| english | 6 | 8 `engUS` | `engUS` | MatisseProN / TsukuOldMinPro | `"+"`, ASCII | every earlier run |
| french | 8 | 2 `fraFR` | `fraFR` | TsukuMinPro / TsukuOldMinPro | `" +"`, ASCII | Reference1715 done (title and menus); Native, and in-game menus, pending (FL5) |
| japanese | 13 | 0 `jpnJP` | `jpnJP` | **MatisseProN** (the file English already loads) / TsukuMinProSub (126 KB) | U+FF0B, **full-width digits** | FL6 |
| schinese | 20 | 20 `zhoCN` | `zhoCN` | SimplifiedChinese (4.8 MB) for both | U+FF0B, ASCII digits | FL6 |
| danish | 4 | 12 `danDK`, mapped to **8** by the filter | `engUS` | as English | as English | FL7: `native_test_language` id 4, FL1 `text 8, live 8`, English UI |

Consequences worth keeping in mind (they refine the old FL6 wording): japanese does not load a new large font movie (MatisseProN is also English's primary); what it changes is the second family,
`msg:` bundles, `menu:/jpnJP/80_language.tpf`, the plus string and the digit width, and the working set (1,732 characters in `menu_dlc2`, 1,805 in `item_dlc2`). schinese is the discriminating CJK run: a new 4.8 MB
font movie used for both families, 2,118 and 2,054 characters. Both exceed what a 1024 x 1024 A8 cache holds at 24 px (about a thousand cells, INFERRED), so the cache evicts and refills while an item list scrolls.

### 1.3 The native glyph path reaches no cap in a CJK run (STATIC, new)

The 256-record and first-8-calls limits quoted as F4 in FONTS-LANGUAGES-NATIVE (`game_bootstrap.cpp:13372`, `13403`, `13494-13497`) belong to the **Reference probes** (`ProduceObservedCpuGlyphUpdates`,
`ProbeNativeGlyphImageAbi`, installed only when `glyph_cpu_probe` is set, `14678-14683`, mode Reference). The live Native path is `GameUiImageOwner::UpdateImage` (`game_ui_image_owner.cpp:236-270`):
up to **4096 records** per call, each record bounded by the engine's own allocation (8 MiB per image, 256 MiB copied per call), mode `+0xFE0 == 1`, format 9, one plane, one mip. A 1024 x 1024 A8 atlas is 1 MiB, so none of these bounds
is reachable by a CJK batch (the update buffer is 256 x 512 pixels). The refusal is `return false` (the engine sees a failed update), not a crash: a failure in a CJK run would show as missing glyphs with no fault row,
so the pilot must compare screenshots, not only the log. F5 (a rewritten cell read stale in the same frame) stays a live check: see 1.5, row L5.

### 1.4 Run sequence and what to fix in the pilot (STATIC + Reference1715's shots)

- **French notice (pilot fix).** The second notice's OK sits at y 439 in French (border 427-452, `notice-offline.jpg`), 451 in English. `Pilot-TitleContinue.ps1` clicks (648, 451): the bottom edge of the French button.
  The first notice is the same in both languages (OK at 428, `notice-network.jpg`), as is the main menu (Continuer at (648, 525), `main-menu.jpg`, so `-MenuY 525` stays). Fix: scan column x 546 for the orange border run and click its middle, or add `-SecondOkY`.
  CJK layouts are not known statically: the same scan, with the first Reference screenshot of each language read before the Native run.
- **"Press any key" stall** (STATE): not explained by the notices (first notice position is unchanged in French). OPEN; the manual click worked. Without a cause, keep the manual fallback
  (`Run-StartupTrace.ps1` alone with a longer `-Seconds`) for each new language.
- **FL5 remaining**: Reference into the game in French, then the same Native run. Save: the Eta m40 zone save (`.ds3-i0039-backups/shebang-zones-eta-m40-20261014-095203/DS30000.sl2`, SHA-256
  `1E905EF8842655C605E175F574BCA5D2B46D1011E8E473142E1577A09FE24EB4`, `record.json`), which the `Run-LiveWitness.ps1 -TestSave -TestSaveSha256 -TestLanguage french` form takes. Screens to compare Reference against Native:
  title, the two notices, main menu, Esc menu, Equipment, Status, Inventory (plus-string spacing `" +"`), Load Game list. Run `Pilot-StatusScreen.ps1`, `Pilot-EquipList.ps1`, `Pilot-MenuButton.ps1` from the existing set.
- **FL6**: japanese then schinese on the same save: title, main menu, Esc menu, Equipment, Inventory, Status, a scroll of a long item list (the cache refill), a weapon with an upgrade level in the list
  (japanese: U+FF0B and full-width digits, so the glyphs must exist in MatisseProN; a missing glyph would show as a gap in both renderers, hence compare, do not assume).
- **FL7**: a title-only run is enough (FL1 and the English screens); danish never reaches a folder of its own.

### 1.5 Rows, expectations, pass criteria

| Row | Source | Pass |
|---|---|---|
| L1 `native_test_language` and `native_language_state` | startup trace | steam id and ordinal as 1.2; `single_sku 0` |
| L2 files requested (FL2, FL3) | engine's VFS logs, if the run records them; otherwise inferred from the screens | `font:/<family>/font.gfx` as 1.2, `menu:/<lang>/80_language.tpf`, the `msg:` bundles of the language |
| L3 glyph updates | the UI session's rows and failure reasons in the Native run (`UpdateImage` answers `false` without a row of its own, so also read the screens) | records per update call (expected above 256 in a CJK run, never refused), no `native_ui_*` failure, no fault |
| L4 screens | shots at the same states, Reference against Native | same text, same layout; the filters (shadow, glow) are drawn natively since 2026-10-11, so differences are about glyph content, not effects |
| L5 cache refill | a long list scrolled in schinese, then back | no stale cell (text readable after the rewrite); a stale cell would be the F5 case |
| L6 banner | YOU DIED needs a death; not required for FL5-FL7 | F6: the 14 banners are 2048 x 384, DX10, identical for all 13 languages (no HAL change) |

Not defects (data-driven, identical in both renderers): zhoCN has no menu categories 76, 77, 80, 81, 90 and an empty `ngword`, so some one-line help and key guide texts take the engine's fallback string; `jpnJP` has an extra item category 40.
The original (Reference) shows the same, so the comparison is Reference against Native, never against a table of expected strings.

### 1.6 CL5 (dialogue and subtitles in a CJK language) and the other languages

- CL5 repeats CL1: during an NPC line read `[[144763258]+0x1654 .. +0x1663]` each frame (slot 0 movie subtitle `{id, 0xFF, kind 0}`, slot 1 talk `{id >= 0, 0x00FF, kind 1}`; consumer acknowledges with an unset id).
  No script reads it yet. Pattern: [Read-MenuGateState.ps1](Read-MenuGateState.ps1) (read-only `OpenProcess`/`ReadProcessMemory` of the verified exe, one JSON line per call, `$stateOwner = Q ($image+0x4763258)` already in it). A `Read-CaptionState.ps1` is the same
  file with the 16 bytes at `+0x1654`, plus the subtitle option byte `[[1447572B8]+0x58]+0x10` (CL3: zero blanks the caption, so read it **before** the talk, never set it).
  Native must show the same queue shape and a larger glyph update at the first CJK line.
- Where to talk: any NPC reachable from the save's position (the Eta m40 save starts at Firelink; which NPC is in range is not known statically). Choose the nearest line the pilot can trigger without a purchase or a quest step.
- The movie subtitles (FDP texts, category 3, 19 entries) belong to `fdp_opening` and `fdp_endroll_ww`: the credits route of section 2 shows them, in English; a CJK credits run is not needed.
- **D-0153 asks for every supported language.** 13 ordinals can be selected on Steam; 8 distinct font bundles: (MatisseProN + TsukuOldMinPro: english), (TsukuMinPro + TsukuOldMinPro: fra ita deu spaES spaAR porBR), (MatisseProN + TsukuMinProSub: jpn),
  (Polish ×2), (TsukuMinPro ×2: rus), (TraditionalChinese + TsukuMinProSub: zhoTW), (Hangul ×2: kor), (SimplifiedChinese ×2: zhoCN). The authorised set covers four. Proposal for Thomas's agreement, four title-and-menus runs:
  `polish` (id 16 -> 10), `russian` (19 -> 11), `tchinese` (23 -> 18), `koreana` (14 -> 19). `spaAR` (id 27) needs the aid to know a `latam` name; the other Latin-script ordinals share `fra`'s bundle.

## 2. The ending credits through flag 20 (R5)

### 2.1 The chain, re-read (STATIC, exports)

`FUN_1408F5930` (called every update by `FUN_1408F5D20`) reads the **decimal** event flags 20, 21, 22, 23 and 30 (`0x14, 0x15, 0x16, 0x17, 0x1E` in the code) through `FUN_1404C5350(_DAT_144752F68, id)`: the reader tests bit `id % 1000` of the
manager's block, and returns 0 when `[manager+0x228] == 0` (flags not initialised). When any of 20-23 is set, **both** gates hold, `FUN_14078C480(_DAT_144763F38)` is false and `[param_1+0x1ED] == 0`:
`FUN_140628090`, `[1447572B8+0x7C] = 1` (20 or 22) or `2` (21 or 23), `FUN_140628960(0x3D1486)`, `FUN_140628BE0(1)`, `FUN_140628670(1)`, `FUN_140765E30(4000)`, `FUN_140628020`. Then, whether the gates held or not, the five flags are cleared with
`FUN_1404C5E20(manager, id, 0, 0)`. **A setting that arrives while a gate is closed is lost**: the aid must check both gates and set again.
Title-flow states 5 -> 6 -> 7 build `EndingStep`, whose state 0 starts the movie (ENDING-CREDITS-NATIVE section 1; `movie/fdp_endroll_ww.bik`, 647,246,828 bytes, installed, 13,002 frames, 433.8 s).

### 2.2 The setter's calling convention (STATIC, disassembly of `008F5930`)

Every site loads `RCX = [144752F68]`, `EDX = id`, `R8D = 0`, `R9D = 0` (`XOR R9D,R9D; XOR R8D,R8D; LEA EDX,[R9+0x14]`). So the prototype is `void(__fastcall*)(uintptr_t manager, uint64_t id, char value, char notify)`;
the decompiler's three-argument view hides `param_4`. Setting is `(manager, 20, 1, 0)`: with `param_4 == 0` the setter also runs its change-listener path (`FUN_1404C0A20`, `FUN_1404A7340`), the genuine one.
The manager's flag block must be initialised (`[manager+0x228] != 0`): a witness set before the world is loaded does nothing.

### 2.3 The aid (specification, not implemented; the thread is read-only)

Same pattern as `ApplyWitnessTeleport` (`game_bootstrap.cpp:2583-2623`, called from `ObserveChrManagerSchedule`, the game thread): a file `witness-flag.txt` next to the module holds `20` (default 20; 21-23 and 30 accepted, anything else refused),
read every 15 calls, removed once read. Before calling the setter log `native_credits_gate`: `manager = [image+0x4752F68]`, `[manager+0x228]`, the reader's value for 20-23 (must be 0), `[world_gate_step+0x1ED]` (the module already keeps `world_gate_step`,
`ObserveWorldUpdateGate`), and `FUN_14078C480([image+0x4763F38])`. Set only when `[manager+0x228] != 0`, `+0x1ED == 0`, `C480 == 0` and the player is in a loaded world; else retry on the next 15 calls for 120 s, then log `native_credits_gate_timeout`
and leave the flag unset. Then log `native_credits_flag` with the manager's reader result and the title-flow state. R1 (calling `FUN_140BEA570` directly) stays the fallback; its guard (`FUN_14075EB60` and `FUN_140B3CDA0` non-null) is in ENDING-CREDITS-NATIVE section 5.

### 2.4 Run plan (D-0116/D-0117; not read-only, so the restore matters)

- Announce first: leaving the world to the title may write the character (`[1447572B8+0x7C]` and the play-through counter are save data); the save, options and `steam_autocloud.vdf` are restored by hash as in every run.
  Save: the Eta m40 zone save (Firelink), a copy restored by hash; never Alpha.
- Order: load the save, wait for the world (`native_player_schedule` rows), write `witness-flag.txt`, watch the chain, **at least 520 s** after the flag (the roll lasts 433.8 s; the default pilot 420 s is too short), then the return to the title.
- Rows (CR1-CR6 of ENDING-CREDITS-NATIVE section 8): `native_credits_gate`, title-flow state 6/7, `EndingStep` states, `native_movie_open` (`fdp_endroll_ww`), frame rows and skipped frames, the movie page closing with `CSMovieImp+0x48 == 0`,
  `DAT_14489F200` at state 7 (which of `C510` / `C570`), the files written (hash the save directory before and after; expect the restore to be needed), CR3 pace and memory over 7 minutes (the opening ran 197.875 s, 0 skipped).
- One extra skip test: a second run pressing the skip key after a minute (CR2), to see whether `page+0x9D8` blocks it.
- Expected natively (STATIC): movie decoded and drawn through the proxy as the opening; **no fade plate** around it (FADE-PLATES-NATIVE F2, nothing draws plates yet; the credits code itself has no plate call), so black-to-picture and picture-to-black transitions will
  differ from the original's; that is a known gap, not a new failure. The post-credits pipeline (states 2-3: dialogue texts 450001/450000, system message `0x1061`) uses ordinary Scaleform pages: they are the first witness of those texts natively.

## 3. Audit: every game menu is in the code

### 3.1 Method and result (STATIC, `Get-MenuPageCensus.ps1`)

The census reads the executable file: every UTF-16 string beginning with a page number (`NN_NNN`) and, for each, every RIP-relative `LEA` in executable sections and every 8-byte pointer in data sections that lands on it.
Result: **104 page names, 243 references** (the script reproduces the figures; evidence not kept, regenerate with `-Out`). Names cover the five groups of MENUS-MAP section 50 plus the loading pages, logos and PC pages (`Win/` is a path prefix, not part of the name).
Reference shapes: a **page registry** (one run of `LEA`s at `0x268F720-0x2690256` that names 90 of the 104 pages, INFERRED the page resource list), **openers** (`0xA7xxxx-0xC1xxxx`, one per page or family: the constructors of section 50),
**descriptor tables** (`0xA54E90-0xA55134`: loading, fade, message and talk pages, and the title family).

### 3.2 Classification (references by page; only the exceptions are listed per page)

| Class | Pages | Reading |
|---|---|---|
| Opener plus registry (the normal shape) | every page not listed below, 83 names | opened by game code that runs unchanged natively; the HAL contract is the one of section 3.3 |
| Descriptor table only (no registry row), 10 names | `01_080_EmergencyNotice`, `01_090_SummonMessage`, `01_900_Black`, `01_910_Fade`, `03_060-062_TalkChoice1-3`, `03_070-071_UseWarpItem1-2`, `04_100_ChrMake_BG` | opened through the window descriptors `FUN_140A55030/55070` (talk menus) and loading steps; reached with the NPC and loading flows |
| Registry only (no opener reference), 7 names | `02_041_PC_MouseSetting`, `02_210_Coliseum_Dialog`, `05_904_Logo_Havok`, `03_030/032/033_Comparison_equip_*`, `03_043_EnhancedWeapon_Status` | opened by a name chosen at run time (OPEN; the comparison and enhancement-status pages have a second registry run near `0x2684858-0x2684F99`, beside the detail-status pages, INFERRED opened from them); `PC_MouseSetting` is the PC page of the Options set: it needs a live check |
| Opener only (no registry row) | `02_999_Test`, `02_041_OptionSetting_Trial`, `05_800_Trial_CharacterSelect` | test and trial pages: INFERRED not reachable on the retail PC build |
| No reference at all | `02_998_Test_Text` | dead (INFERRED) |

Every game-reachable page therefore has a code path that the native build runs as the original does; the **wiring risk is not the opening but what the page draws or needs**.

### 3.3 What can stop a page natively (STATIC, `game_ui_session.cpp`, the HAL refusals)

A page fails (loudly: `state.Fail(reason)`) only when it asks for one of these; none is a page-by-page branch.

| Refusal (reason string) | Condition | Pages that can ask (feature scan of section 78) |
|---|---|---|
| `native_ui_display_offscreen_unavailable` (line 206) | display record `+0x40` set (an offscreen composition) | cache-as-bitmap: `04_030_ChrMake_ColorSelect` and its `Win/` twin (2 placements): **the one known unwired family**; the cache-scope code (`native_ui_cache_scope_*`, lines 134-145) exists, this refusal is the direct display |
| `native_ui_display_colour_clear_unavailable` (208) | display colour clear without the `0x200` flag | none in the scan; unconfirmed |
| `native_ui_filter_gradient_unavailable` (667) | filter kinds 4-5 (gradient glow and bevel) | none used by any movie (kinds 0, 1, 2 only) |
| `native_ui_vertex_layout_instancing_unavailable` (276) | instancing above 1 | none expected |
| blend, mask, batch extent refusals (331, 245-267, 472) | blend outside the admitted six (0, 1, 3, 4, 8, 18), masks nested over 255, 4096 batches | the scan shows only Add, Multiply, Screen, Normal: covered |

So the audit's closing evidence per family is one native witness with **no `native_ui_*` failure row**: a refusal is a hard stop, not a missing effect, so a clean run of a page is a complete witness of its HAL needs.

### 3.4 Witness queue by feature family (live work, for the menus pilot; none done by this thread)

Witnessed natively: title, Load Game, pause menu, Equipment top, Status (body and face), Create Character, MessageBox, the face grid, the HUD (filters). Open, by family (MENUS-MAP section 78 §4; reach routes INFERRED from section 50 names):

1. Inventory `02_020` and Item Box `03_050` (Multiply, Screen, Add, radial gradient): Esc menu, then a bonfire or NPC for the box.
2. PC graphics `02_042`, mouse `02_041_PC_MouseSetting`, key configuration `02_043`, text select `02_044`, command list `02_045` (146 colour transforms on the graphics page): Esc, System, Options. This is also the OP-1..OP-7 sweep's route.
3. New Game pages `05_031` (brightness, 191 colour transforms) and `05_032` (controls): reachable only from a new game (a fresh character, not the saves in use); `Pilot-CharMakeWitness.ps1` is the nearest route.
4. Bonfire family: Level Up `03_010`, Attunement `03_020`, Warp `02_080`, Estus allocation `02_100`, Shop `03_000/001`, Enhanced Weapon `03_040/041/043`, comparison pages `03_030/032/033`: a bonfire and its NPCs (Firelink, m40).
5. Cache-as-bitmap: character creation colour select `04_030`: **expect `native_ui_display_offscreen_unavailable` there**; the Create Character witness may not have reached that screen (the existing pilots do not name it).
6. Small families: blood messages `01_030-035`, gestures `01_040/041`, Kick `02_060`, Ranking `02_090`, Coliseum `02_200`, text input `02_990/991`, Now Loading `02_901-904`, Emergency notice, Summon message, Title information `05_020`, Terms `05_040`, logos `05_900-905`.
7. Two names with no movie (INFERRED `02_998/02_999`, or the Trial pages): 104 names against the 102 movies of section 78; settle with a read-only archive listing (the old `Inventory-Archives.ps1` is in the archive tag), no live run.

The audit's static half is closed by this section; its live half is the queue above, and each item is a **family**, one page per feature set (D-0150, D-0161), not a page-by-page sweep.

## 4. Resolutions and full screen under one rule (D-0151, D-0170, wave-1 base-game coverage)

### 4.1 What the engine does natively today (STATIC, new reading)

- **S** is the engine's size: `FUN_140EDAA60(w, h)` writes `_DAT_1446601E8/EC` and derives the UI scale factors `S.w / 1280` and `S.h / 720` (`1427F4528` = 1280.0, `143D7F030` = 720.0; read in the file), so the engine's interface scale follows S.
  It runs from the window procedure's `WM_SIZE` (`FUN_140EF50D0`: client size windowed, selected mode full screen), which is genuine CPU code and still runs natively.
- Everything that needs GX is skipped because `_DAT_144961AB0` is null: the request `FUN_14000C240` in `FUN_140EF54A0` (so an Options-page change of resolution or screen mode is dropped), the GX resize `FUN_14000B700` in `FUN_140EF50D0`
  (so the resize listeners, the Scaleform viewport and `ResizeBuffers` never run), the deferred application `FUN_140009EA0`.
- The native output size is set **once**, from the window client rect (`game_bootstrap.cpp:13691`, `output_width/height`); a row `native_render_size` logs when S differs from it (`7123-7128`). `SceneRenderer::Resize` and `Backend::Resize` exist
  (`scene_pass.cpp:2794`, waiting every slot, resizing presenter, display, glow, tonemap, grading, ao stages) but **have no caller in the game path** (D3D12-PHASE-D-COMPLEMENT W13, unchanged), and `Resize` refuses while a primitive packet of the old extent is live.
- The mouse already follows S (`NativeMouseDeviceUpdate`, `11701-11714`) while the picture stays at the start size: an offset as soon as S differs (D3D12-PHASE-G M-3, INFERRED; OP-5 measures it).
- **A config file cannot reach another size.** `FUN_140EF6990` (the settings sanitiser, new decompilation) tests the windowed and full-screen resolutions against the two lists of `FUN_140EF9090` and resets any value not in them to **1920 x 1080**
  (`0x780 x 0x438`); the windowed list is the 12-entry table (800 x 450 ... 3840 x 2160) cut to the desktop window rect, the full-screen list is the output's `GetDisplayModeList`. An edit of `GraphicsConfig.xml` to 2560 x 1440 on a 1080p desktop is silently undone.

### 4.2 The rule

**One input, one apply point, no cached extent.**

1. **Input.** A `DisplayExtent {width, height, mode, output}` read each frame from what the engine decided: S (`image+0x46601E8/EC`) and the window mode (`CSWindowImp+0x38 -> +0xBC`: 0 windowed, 1 or 2 full screen, 2 when the exclusive flag `CSWindowImp+0x40` is set, per `FUN_140EF5590`; `FUN_140EF1C30` tests 1-2; naming 1 borderless and 2 exclusive is INFERRED). The native never chooses a size; the Options page, the window procedure and a test aid all end in S.
2. **One apply point.** At the start of a native frame on the owner thread, render worker quiesced, GPU idle: if the extent differs from the live one, apply the whole change (the native analogue of `FUN_140009EA0` then `FUN_14000B830`, DISPLAY-SWAPCHAIN section 4.3-4.4 and 5), then draw. Never inside a frame, never from the worker, never split across frames.
3. **Everything size-dependent is derived from the extent, at the apply point.** No stage keeps its own copy of a width or a ratio. The places that hold one today (census, `game_bootstrap.cpp`; the engine objects are CPU memory the native fills):
   - the presenter's buffers and every renderer target and stage (`SceneRenderer::Resize` already lists presenter, display, glow, tonemap, grading, ao; the DoF, glare, shaft, motion-blur, velocity, depth and G-buffer sets are in `Targets` / their stages: each must appear in the resize path's test);
   - the engine's own scene objects, filled with `output_width/height`: the scene context `+0x20/+0x24` and `+0x3F3C/+0x3F40` (`1924-1940`), the scene view `+0xAC/+0xB0` (`2016`), the camera `+0x5C/+0x60` (`2108`, `9828`), the auxiliary view (`2123`), the camera-size check (`8666`);
     existing views must be rewritten (or rebuilt) at the apply point, since the engine's resize listeners that did this are not running;
   - the UI session: viewport (`11755`), primitive packet extent (the refusal above: withdraw and readmit), movie planes, the face and status views' own extents (fixed, not display size: confirm they stay);
   - the Scaleform viewport `FUN_142307D70`, which `FUN_14000B830` set in the original and nothing sets now;
   - the mouse (already S).
4. **Odd sizes round up.** Half-size and lower targets are `ceil(n / 2)` (the glow chain already rounds 22.5 up to 23); test with odd extents.
5. **History is reset on a change** (R-T2/R-T5 of phase D): motion-blur velocity, any temporal resource, then the first frame at the new size is drawn from scratch.
6. **Presentation follows the engine's window code.** The engine moves and styles the window (`FUN_140EF61A0`, encrypted at rest, runs). The native only keeps the swap chain consistent: `ResizeBuffers` with every back-buffer reference released, flip-discard,
   `GetFullscreenState` truthful (the engine's retry loop reads it, `FUN_140EF1D00`), occlusion accepted. For exclusive mode: `SetFullscreenState` between `ResizeTarget` calls, the swap chain created with `ALLOW_MODE_SWITCH`, and the window association set to
   `NO_WINDOW_CHANGES | NO_ALT_ENTER` like the original (today `backend.cpp:655` passes only `NO_ALT_ENTER`; without `NO_WINDOW_CHANGES` DXGI restyles a window the engine also restyles).

### 4.3 Implementation order (for the thread that has the code)

1. Log only: extend `native_render_size` with the window mode and the swap chain's `GetDesc` (no behaviour change; settles OP-5's offset).
2. A single `ApplyDisplayExtent` at the start of the native frame (quiescing as `Resize`/`Close` do): renderer `Resize`, the engine-object rewrite of 4.2 item 3, UI session withdrawal and readmission, Scaleform viewport, history reset. Windowed resize first (manual drag and the Options list).
3. Borderless (ScreenMode `FULLSCREEN` with the engine's `Fake`), then exclusive (`Real`).
4. Unit tests with the extents of 4.4 on the stage tests that exist (`display_stage_test.cpp`, `glow_stage_test.cpp`, `dof_stage_test.cpp`, `mb_stage_test.cpp`, `grading_stage_test.cpp`, `ao_stage_test.cpp`, `frame_graph_test.cpp`): a width or height that is odd or not a multiple of 8 must give valid targets and equal results on the part they share.

### 4.4 Test matrix

Extents: **1920 x 1080** (the desktop, the reference baseline), **1280 x 720** (the current start size), **2560 x 1440**, **3840 x 2160**, one **odd** size, for example 1366 x 768 and an odd-width one such as 1001 x 563 (unit test only).
Modes: windowed, borderless, exclusive, and the transitions between them. Rows per cell: `native_render_size` `match: true`, swap chain `GetDesc` width and height, readback hash and size, `GetFullscreenState` equal to the engine's `+0xBC`, the corner and centre mouse hit tests on a menu button
(S scale factors from `FUN_140EDAA60`: 1.5 and 1.5 at 1920 x 1080), a title screenshot and an in-game screenshot compared with Reference at the same extent (the original handles all of them natively through GX), the frame rate at 60 frames/s at 1440p and 4K (K-0450 holds 60 at 720p),
GPU memory (the main targets are 97.5 MB at 1280 x 720, E6; a 9x pixel count at 4K is about 0.9 GB for the extent-bound part, INFERRED, the shadow atlases do not scale), no validation error with the debug layer, no fault on repeated changes (the V5s 30-minute resize and teleport loop stays open until this exists).
Also: alt-tab, minimise and restore, manual drag of a window edge, Options-page change windowed to borderless and back (OP-5).

### 4.5 The limit of the test machine (STATIC and the screenshots)

The desktop in Reference1715's shots is 1920 x 1080. The windowed list is cut to the desktop and the full-screen list is the monitor's modes, so **2560 x 1440 and 3840 x 2160 are not selectable in the game** on this machine (and the sanitiser undoes a file edit). Two ways, for Thomas to choose:

- **A test aid in the module** (recommended; D-0151 says size is an input): `test-resolution.flag` with `WxH` (and a mode), applied through the same apply point after the engine's own chain, by writing S through `FUN_140EDAA60` and sizing the window with `SetWindowPos` (a window may exceed the desktop), nothing installed in the game folder, nothing in the user's settings. It also gives the odd size. Cost: the engine's own WM_SIZE path then runs unchanged, so the aid tests the real chain from S onward, not the Options page.
- **NVIDIA DSR/DLDSR** in the control panel: the 1440p and 4K factors then appear in the monitor's mode list and so in the full-screen list. That is a change of the user's graphics environment, so it needs Thomas's agreement, and exclusive-mode behaviour under DSR is not the same as a native 4K display.

## 5. Order and questions

Proposed order for the menus pilot: (1) FL5 Native French run and the in-game screens; (2) FL6 japanese then schinese with CL5 in the same session where an NPC is in range; (3) FL7 title-only; (4) the credits route once the `witness-flag.txt` aid exists; (5) the audit's witness queue by family; (6) the display work of section 4 (an implementation thread, not a witness).

Questions for Thomas (none blocks 1-3): the four extra languages of 1.6; DSR or the test aid for 1440p and 4K (4.5); announcement of the credits run, which is the first route in this set that is not read-only (2.4).

## 6. Not verified here

The pilot's "press any key" stall; the cause and the exact cell of every CJK notice layout; which NPC is in range at the Eta m40 save; whether `02_041_PC_MouseSetting` and `02_210_Coliseum_Dialog` are opened by run-time names; the two names without a movie; what `FUN_14078C480` and `[step+0x1ED]` mean beyond their effect on the chain;
whether the colour-select page was ever reached by the Create Character pilots; the exclusive-mode behaviour of a flip-model swap chain on this driver. Each is named so the next thread does not assume it.

## 7. Thomas's answers (2026-10-16 07:00 Paris)

- **Languages:** add the Asian languages and at least one Cyrillic one. Decision recorded for the pilot: runs for `japanese` (0), `schinese` (20), `tchinese` (id 23 -> ordinal 18, TraditionalChinese + TsukuMinProSub), `koreana` (id 14 -> 19, Hangul x2) and `russian` (id 19 -> 11, TsukuMinPro x2, Cyrillic); `french` and `danish` as before. `polish` stays optional (own font bundle, Latin Extended).
  Thomas asked to check the Steam store page: the 13 selectable ordinals above come from the executable's own tables; the store page was **not** consulted here, so any language it lists that is absent from the 13 would be a fallback to `engUS` and worth one check.
- **Resolutions:** the recommendation (the `test-resolution.flag` aid, section 4.5) is accepted; NVIDIA DSR only if it does not pass.
- **Scheduling:** left to the coordinator.
