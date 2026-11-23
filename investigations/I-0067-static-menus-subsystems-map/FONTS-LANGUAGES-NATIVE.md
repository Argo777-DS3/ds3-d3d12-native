# Fonts and languages of the Scaleform interface, as a native checklist (I-0067, Task 6)

STATIC (Ghidra mirror of `DarkSoulsIII.exe` 1.15.2.0) + game data read offline (Data1 archive, installed `msg`, `menu`, `font` files).
Nothing was launched. Status words: **STATIC** = read in code or data, **INFERRED** = deduced, **OPEN** = not known statically.
Baseline of the notes: [`MENUS-MAP.md`](MENUS-MAP.md) section 78.3 (this note replaces the "INFERRED" parts of it with the exact chain).
Evidence (outside Git): `evidence/I-0067/queries/fonts-languages/` (`ordinal-table.tsv`, `fmg-list-engUS-jpnJP-zhoCN.tsv`,
`80_language-banner-matrix.tsv`).

## 0. Verdict

1. **The language is decided once, at startup, by the engine, from `SteamApps()->GetCurrentGameLanguage()`** (section 1). The native
   backend never sees it. A run in another language changes only which *files* the engine requests (`msg`, `menu/<lang>/80_language.tpf`,
   two `font.gfx` movies) and how many glyph updates arrive; no renderer object differs.
2. **Only 13 of the 21 ordinals can be selected on a Steam PC build** (given the two items marked INFERRED/OPEN in section 1: `ED01E0` is an identity round trip and `DAT_14479BEDC` is 0) (`jpnJP`, `fraFR`, `itaIT`, `deuDE`, `spaES`, `spaAR`, `porBR`, `engUS`, `polPL`,
   `rusRU`, `zhoTW`, `korKR`, `zhoCN`). Every other Steam language (danish, dutch, finnish, norwegian, swedish, turkish, portuguese,
   bulgarian, czech, greek, hungarian, romanian, thai, ukrainian, and any Steam language code absent from the 26-name table, for example `latam`, which does NOT give `spaAR`) falls back to `engUS` (8). `engGB` (1) is not a fallback: no Steam id maps to it. The installed Data1 has
   exactly these 13 folders for `msg`, and for `menu/<lang>/80_language.tpf`.
3. **The HAL contract is language-independent**: the Scaleform glyph cache keeps its defaults (1 page, 1024 x 1024, A8, CPU updates) for
   every language (section 5). What changes is the glyph *working set* (about 800 distinct characters for English menus, 1,700 Japanese,
   2,100 Simplified Chinese): the native glyph path must not assume the English batch sizes (checklist F4).
4. The game install used by every witness so far is **English** (`appmanifest_374320.acf`: `"language" "english"`), so the expected
   ordinal of every earlier live row is **8 (`engUS`)**, folders `engUS`, fonts `MatisseProN` + `TsukuOldMinPro`.

## 1. How the language is chosen (STATIC)

Startup initializer `FUN_140EA5860` (called once):

| Step | Code | Result |
|---|---|---|
| region | `FUN_140ED00C0(obj,FUN_140EA6130()=1)` (encrypted, not readable) | `14479BA04`, range 0..3 asserted by every getter (`JP`, `NA`, `EU`, `AS`, names at `144658718`) |
| second flag | `FUN_140ECFE80(obj, FUN_140EA6090())`, with `FUN_140EA6090` reading `FUN_140EE3000` (encrypted) | `14479BA08`, 0 or 1 (getter `FUN_140EA5720` asserts <= 1) |
| Steam id | `FUN_140EF3720` | 0..27, see below |
| DS3 ordinal of the platform | `FUN_140EA6020`: `p = FUN_140ECFFA0(obj, table1429A24F0[id])` (the table holds DS3 ordinals; `ECFFA0` is encrypted) | `p` |
| text language | `L = FUN_140EA61F0(region, FUN_140ED01E0(obj,p))` | `14479BA0C` and the live global **`14479BA10`** |
| two more settings | `FUN_140EA6450(region, FUN_140ED0300(obj,p))` and `FUN_140EA6320(region, FUN_140ECFD60(obj,p))` | `144658178/180` and `14465817C` (a scan of the whole code section, `140000000-143000000`, finds no reader of these three addresses (the initializer and `FUN_140EA5AD0` only write them): they are inert on PC) |

**Steam name -> id (`FUN_140EF3720`).** It calls `SteamApps()` (import `144DBC554`), the interface slot at `+0x20` (`GetCurrentGameLanguage`),
hashes the returned name (`h = h*0x89 + c`) and compares it with 26 names registered at `14489E9C0` (stride 0x48, id at `+0x40`):

| id | Steam name | id | Steam name | id | Steam name |
|---|---|---|---|---|---|
| 1 | brazilian | 10 | greek | 19 | russian |
| 2 | bulgarian | 11 | hungarian | 20 | schinese |
| 3 | czech | 12 | italian | 21 | spanish |
| 4 | danish | 13 | japanese | 22 | swedish |
| 5 | dutch | 14 | koreana | 23 | tchinese |
| 6 | english | 15 | norwegian | 24 | thai |
| 7 | finnish | 16 | polish | 25 | turkish |
| 8 | french | 17 | `portugues` (sic) | 26 | ukrainian |
| 9 | german | 18 | romanian | | |

The 26 names (including the spelling `portugues`) were read in the Ghidra mirror, they are not in the repository exports. No Steam, no match, or a null `SteamApps()` returns **0**. **Latin American Spanish:** id 21 (`spanish`) is refined with
`GetSystemDefaultLocaleName`: if the name is at least 4 characters long (the test is `return value >= 5`, the count includes the terminating null) and its characters 3-4 (the country of `es-MX`) equal one of the
20 codes `US CA MX CR DO CO AR CL PY SV NI GT PA VE PE EC UY BO HN PR` (`1446627D0`), the id becomes **27**, else it stays 21. (`portugues` is
shorter than Steam's `portuguese`: it never matches, id 0; the end result is the same, see next.)

**Id -> ordinal (`1429A24F0`, 28 dwords):** `0->8, 1->7, 2->8, 3->8, 4->12, 5->9, 6->8, 7->14, 8->2, 9->4, 10->8, 11->8, 12->3, 13->0,
14->19, 15->13, 16->10, 17->17, 18->8, 19->11, 20->20, 21->5, 22->15, 23->18, 24->8, 25->16, 26->8, 27->6` (full table with folders:
`ordinal-table.tsv`).

**Filter (`FUN_140EA61F0`).** `x < 0x15` indexes `*(int*)(entry+0x10)[x]`, else `entry+0x18` (default); `entry = base + region*0x20`;
`base = 1446587E0` if `DAT_14479BEDC` (`FUN_140ED0F40`, encrypted) is non-zero, else `144658760`.

| Table | Supported list (`entry+0`, count at `+8`) | Array (`+0x10`) | Default (`+0x18`) |
|---|---|---|---|
| `144658760` (4 entries, same list and array) | `1429A29C0`: **8, 0, 2, 6, 7, 3, 4, 5, 10, 11, 18, 19, 20** (13) | `1429A2A00`: `0 8 2 3 4 5 6 7 8 8 10 11 8 8 8 8 8 8 18 19 20` | 8 (region 0), 0 (regions 1-3) |
| `1446587E0` (single-language SKU) | region 0: `{0}`; regions 1-3: `{8}` | `1429A2AC0` (all 0) / `1429A2A60` (all 8) | 0 / 8 |

So the supported ordinals map to themselves, `engGB`(1), `nldNL`(9) and 12-17 map to 8 (`engUS`). For an input below 0x15 (always the case on PC, the id table holds only 0..20) the result does not depend on the region; the region-dependent default (8 or 0) only applies to inputs >= 0x15.
**INFERRED:** `ECFFA0`/`ED01E0` are a round trip DS3 ordinal <-> platform code (console builds need it), identity on PC; **OPEN:**
`DAT_14479BEDC` is expected 0 on Steam (live row FL1).

**A saved setting can override the platform (STATIC).** The profile/settings object constructor `FUN_1405A0320` sets `+0x2C` and `+0x30`
to `-1`. On load (`FUN_1405A0740`, from the settings block read into `+8`, its `+0x2C/+0x30`) and when a settings block is applied (`FUN_1405A0930`, from the incoming block `+0x18/+0x1C`, it does not write the object's own `+0x2C/+0x30`) the game calls `FUN_140EA5A20(value,1)` and `FUN_140EA5AD0(value2,1)`:
`-1` recomputes from the platform (the chain above), any other value goes through the same filter, and the commit flag writes
`14479BA10`. A save written with the default `-1` follows Steam; **whether any menu writes another value is OPEN** (no language option page
exists among the 102 menu movies of section 78).

## 2. What the ordinal selects (STATIC)

The getters take the ordinal (the callers pass `14479BA10`; `FUN_140EA5A10`, `FUN_140EA4D20`, `FUN_140EA5840` read it directly) and assert `< 0x15`:

| Getter | Table | Meaning |
|---|---|---|
| `FUN_140EA5D70` | `144658390` | wide folder name: `jpnJP engGB fraFR itaIT deuDE spaES spaAR porBR engUS nldNL polPL rusRU danDK norNO finFI sweSE turTR porPT zhoTW korKR zhoCN` |
| `FUN_140EA5BE0` | `14479BE30` (filled from `1429A2380`, 8-byte ASCII) | the same names as narrow strings |
| `FUN_140EA5DE0` | `144658440` | first font family (`MenuFont_01`) |
| `FUN_140EA5E60` | `1446584F0` | second font family (`MenuFont_02`) |
| `FUN_140EA5E20` / `FUN_140EA4D20` | `1446585A0` | **plus string** of the language (upgrade suffix of item names, composed by `FUN_140E41330`): U+FF0B for `jpn`, `nld`, `12-18`, `zhoCN`; `" +"` for `fra ita spaES pol rus`; `"+"` for the others |
| `FUN_140EA5A10` | bytes at `144658648` | 1 for ordinal 0 only: that language writes the upgrade level with full-width digits (`14298AEA8`, U+FF10..FF19) instead of ASCII digits |
| `FUN_140EA5840` | | returns the ordinal (one reader in the code, `FUN_140C7DCD0`) |

Font families by reachable ordinal (secondary in brackets): `jpn` MatisseProN [TsukuMinProSub]; `engUS` MatisseProN [TsukuOldMinPro];
`fra ita deu spaES spaAR porBR` TsukuMinPro [TsukuOldMinPro]; `pol` Polish [Polish]; `rus` TsukuMinPro [TsukuMinPro];
`zhoTW` TraditionalChinese [TsukuMinProSub]; `kor` Hangul [Hangul]; `zhoCN` SimplifiedChinese [SimplifiedChinese]. This confirms
section 78.3 (the roles "primary/secondary" are now fixed by the `MenuFont_01/02` mapping, section 4).

## 3. Files per language (STATIC + Data1)

| Path pattern (VFS) | Builder | Per-language files in the installed Data1 |
|---|---|---|
| `msg:/<lang>/item_{master,dlc1,dlc2}.msgbnd` | `FUN_140EA4910` (level from encrypted `FUN_14080EAF0`: 0 / 1 / other) | 13 x `item_dlc1`, 13 x `item_dlc2` |
| `msg:/<lang>/menu_{master,dlc1,dlc2}.msgbnd`, else `menu.msgbnd` | `FUN_140EA4BC0` (level 0 / 1 / 2 / other) | 13 x `menu_dlc1`, 13 x `menu_dlc2` |
| `msg:/<lang>/ngword.msgbnd` | `FUN_140EA4C60` (used by `FUN_140DE9860`) | 13 |
| `msg:/<JP|NA|EU|AS>/sellregion.msgbnd` | `FUN_140EA4D30` (region name) | none found |
| `menu:/<lang>` then `80_language.tpf` | `FUN_140EA49A0` (request index 6, section 53) | 13 `80_language.tpf.dcx` (padded size 1.13-1.48 MB each) |
| `font:/<family1>/font.gfx`, `font:/<family2>/font.gfx` | `FUN_140EA4AB0`, `FUN_140EA4E10` (appended to the menu resource list by `FUN_140E5A2F0`) | 8 families in `data1/font/<family>/font.gfx` (sections 2 and 4) |

Observed in the hashed archive indexes (`/msg/<lang>/<name>.msgbnd.dcx` hash rule of `Inventory-Archives.ps1`, verified on
`menu_dlc2`): `item_master`, `menu_master`, `menu` and `sellregion` do not exist under any of the 13 language folders or the four regions in
Data1-5 or DLC1-2 (the index has no Data0 rows). **INFERRED:** the installed game always runs the highest level (`*_dlc2`), whose bundles are cumulative (the English
`menu_dlc2` carries the base categories 1, 2, 3, 30, 70, 76-81, 90, 200-206, the DLC1 categories 230-239 and the DLC2 categories
270-279). **OPEN:** the value returned by the encrypted `FUN_14080EAF0` (live row FL3 shows which path is opened).

The 13 `80_language.tpf` files hold the same **14 banner textures**, `2048 x 384`, DX10, TPF format 102, 786,580 bytes each:
`MENU_YOU_DIED`, `MENU_BONFIRE_LIT`, `MENU_VICTORY_ACHIEVED`, `MENU_REVIVAL_FROM_DECEASED`, `MENU_TARGET_WAS_DESTROYED`,
`MENU_LORD_OF_CINDER`, `MENU_LOAD_OF_CINDER_FALLEN`, `MENU_INVADER_BANISHED`, `MENU_MATCH_START`, `MENU_DRAW`, `MENU_RETRIEVAL`,
`MENU_BERSERKER_OK`, `MENU_REDHUNTER_OK`, `MENU_YOU_WIN`. The pixel content has 7 to 9 distinct variants over the 13 languages (for
example `MENU_YOU_DIED`: `jpnJP`, `engUS`, `zhoTW`, `korKR`, `zhoCN` share one bitmap; `fraFR`, `itaIT`, `deuDE`, `spaES`, `spaAR`, `porBR`,
`polPL`, `rusRU` each have their own; matrix in the evidence). Sizes and format do not change: **the HAL needs nothing new for a
language switch on the banners.**

## 4. How fonts reach Scaleform (STATIC, mechanism INFERRED from the Scaleform API shape)

1. `FUN_140E5A2F0` builds the menu resource list: per request descriptor two paths (`FUN_140E5F290`/`FUN_140E5F1E0` over the tables
   `1447992E0`/`144799350`, chosen by `FUN_140908B00`), then the two font paths `font:/<family1>/font.gfx` and `font:/<family2>/font.gfx`,
   each requested with `FUN_140503D30`.
2. `FUN_140E4DCF0` / `FUN_140E4DD00` (thunks into `FUN_140E5AEC0` / `FUN_140E5AFA0`) take the path from `FUN_140EA4AB0` / `FUN_140EA4E10`, normalize it (`FUN_140E5F0A0`) and call `FUN_140E5CF30(menu, "MenuFont_01", path1)` and `(menu, "MenuFont_02", path2)`: the path is loaded
   as a movie definition (`FUN_141BB5A30` on the Scaleform loader), its fonts are added to the font library (`FUN_141BE5820`), a font name is
   read from it (`FUN_140E5F000`, INFERRED to be the first font of the movie, formatted through `14298D0B8`) and the font map receives `MenuFont_0N -> that name` (`FUN_141BE49A0`, scale 1.0).
   The 90 menu movies import their fonts through `font.swf` (section 78), so **the font identity of every text field is resolved here,
   once per language**, not in the movie.
3. The engine registers a Win32 font provider on `GetDC(0)` (`FUN_140E527B0`, `FUN_141BDF760`): a device-font fallback inside Scaleform for a
   glyph or family missing from the font library. It runs on the CPU and feeds the same glyph cache; it does not touch the HAL.
4. Instance names embed a font tag in some pages (for example the HUD child `Text_0/Text_TsukuMin_Pro_E` of `MapName`): they are labels,
   not lookups.

Old-style text (`FrpgMenuFontPrinter`, `EzMenuFontPrinter`, the 449 `FrpgMenuDlg*` dialogs, drawn by GX in the 2D layer) is **INFERRED** to use **bitmap fonts** (the names `DSFont24` and `TalkFont24` appear in no string of the executable, only `DbgFont14h` does, so the binding is by data; old dialogs are a probable (INFERRED, OPEN) path for event text, see `SUBTITLES-DIALOGUE-NATIVE.md` section 7D):
`data1/font/JAPANESE/DSFont24` (19 textures) and `TalkFont24` (20 textures), each texture 512 x 512 A8, plus `DbgFont14h`. Their character
maps (`.ccm`, 7,117 and 7,112 glyph records) cover ASCII (95), only 8-10 Latin-1 letters, Greek (48), Cyrillic (66), kana (177), 6,356 CJK
ideographs, full-width forms (160), and **no** Latin Extended, **no** Hangul. Against the installed English `menu_dlc2`, 7 characters are
missing in `DSFont24` (9 in `TalkFont24`); against Simplified Chinese, 660. **INFERRED:** these fonts are the single-folder (`JAPANESE`)
bitmap set of the old dialogs; localized body text of the shipping UI does not depend on them (accented Latin letters would be absent).
**OPEN:** which old dialogs remain reachable in a release build (the `FrpgMenuDlgTalkMsgbox` path of section 7 is one).

## 5. The glyph cache (STATIC + live baseline)

Constructor `FUN_141C6DE10` sets the Scaleform glyph-cache parameters (cache `+0x20..+0x58`); the HAL initializer `FUN_141BD8190` constructs the cache (call at `141BD8556`) and calls no setter, `FUN_140E527B0` only registers Scaleform states, and the live baseline below shows the defaults
(a later setter was not searched for): **texture 1024 x 1024, 1 texture (`+0x28`), max slot height 0x30, slot padding 2,
update buffer 256 x 512 pixels (`+0x34/+0x38`), `+0x40` = 500, floats at `+0x3C..+0x50`** (field names INFERRED from the Scaleform layout).
`FUN_141C702E0(cache, hal, third argument)` initializes it (the pilot passes its fill manager as the third argument): the image-manager capability query for format 9 must return bit `0x20` set and bit
`0x80` clear, which gives **mode `+0xFE0 = 1` (CPU updates of an A8 image)**; mode 2 (`bit 0x20` missing) or 0 (`bit 0x80` set) would select
other paths the native does not implement.

Live baseline of the pilot (English, `i66-reference-startup-35/startup.jsonl`): `reference_glyph_cpu_policy` actual caps 434, selected 306;
`reference_native_cpu_glyph_initializer`: mode 1, pages 1, 1024 x 1024; the first updates are 1 record (270 bytes) and 1 record (209 bytes).

Working-set facts that change with the language (measured on the installed FMG bundles, distinct characters over the `menu_dlc2` and
`item_dlc2` bundles):

| Language | `menu_dlc2` | `item_dlc2` |
|---|---|---|
| `engUS` | 802 | 84 |
| `jpnJP` | 1,732 | 1,805 |
| `zhoCN` | 2,118 | 2,054 |

At 24 pixels a 1024 x 1024 cache holds on the order of a thousand glyph cells (**INFERRED** from slot height 0x30 and padding 2): the
Japanese and Chinese item lists exceed it, so the cache evicts and refills during scrolling. **Consequence:** many more records per
update call and rewrites of already-used rectangles than any English witness shows.

## 6. Native checklist and live rows

| # | Requirement | Where |
|---|---|---|
| F1 | The native backend is language-blind: no branch on the ordinal, no asset name hard-coded to `engUS`/`MatisseProN`/`80_language`. All file opens stay engine-side (VFS requests of section 3). | design |
| F2 | The glyph context keeps asserting mode 1 and capability bits `0x20`/`0x80`; it must also accept `cache+0x80 == 1` only because the defaults give 1 texture: record a loud failure (not a crash) if a future build changes `NumTextures`, `Width`, or `Height`. Read width/height from the image the engine asks for, never from constants. | `native/frame/game_ui_glyph_context.cpp` (checks `+0xFE0`, `+0x80`, image `+0x40`) |
| F3 | A8 glyph atlas, one image, resident for the whole session; updates are rectangles `(x,y,w,h)` with pitch, mip 0 only. | pilot already does this |
| F4 | **No batch cap**: the 256-record, first-8-calls limits in `game_bootstrap.cpp` (`count>256`, `call<=8`, lines 7951, 7982, 8072-8074) are probe limits. A production path must size its staging dynamically; the update buffer is 256 x 512 pixels (131,072 A8 bytes), so a batch is bounded by that area over the smallest glyph rectangle (several hundred records for CJK is plausible; **OPEN**, never observed). | `game_bootstrap.cpp` |
| F5 | Eviction rewrites a rectangle that an earlier draw of the *same frame* may still sample: keep the existing fence discipline (`native_glyph_unqualified_fence_retirement` refusal) and add a witness that a rewritten cell is not read stale. | glyph context |
| F6 | `80_language.tpf` banners: 14 textures, 2048 x 384, TPF format 102 (DX10), same on all 13 languages. Nothing language-specific; they are requested as menu resource index 6 (section 53). | none |
| F7 | Bitmap fonts of the old dialogs: 512 x 512 A8 sheets (19 + 20 + the debug font). They are ordinary textures through the existing texture path; coverage is limited (section 4), so absent glyphs are expected for accents and Hangul and must not be treated as renderer defects. | none |
| F8 | Text fields in the HUD page (`TalkWindow`, `CaptionWindow`) need nothing new for the language; the font family changes the metrics, not the draw. | HUD page `01_000_fe` |
| F9 | Do not ship a switch that sets the Steam language: it is a user-environment change. A non-English verification run needs Thomas's agreement (WORKFLOW). | rule |

**Static witnesses (SW):** SW1 ordinal table `144658390` and the 21 names; SW2 Steam table `1429A24F0` (28 dwords above); SW3 supported list
`1429A29C0` (13) and filter array `1429A2A00`; SW4 `font:/%s/font.gfx` at `1429A1F90`; SW5 13 language folders for `msg` and `80_language.tpf`;
SW6 14 banners `2048 x 384`; SW7 bitmap font coverage (counts above); SW8 install language `english`.

**Live rows (FL), all read-only unless said:**

| Row | Action | Expected |
|---|---|---|
| FL1 | After the title, read `14479BA04`, `14479BA0C`, `14479BA10`, `DAT_14479BEDC` (`14479BEDC`) | region 0..3; `BA0C = BA10 = 8`; `BEDC = 0`. **Observed** (Reference1715, French test language, 2026-10-14): region 1, `BA08` 0, `BA0C = BA10 = 2`, `BEDC = 0` |
| FL2 | Log the Scaleform file requests of the menu resource list | `font:/MatisseProN/font.gfx`, `font:/TsukuOldMinPro/font.gfx`, `menu:/engUS` + `80_language.tpf` |
| FL3 | Log the `msg:` opens | `item_dlc2`/`menu_dlc2` (or the level actually used) under `engUS`, plus `ngword` |
| FL4 | Glyph cache after the first menu frame: cache `+0x20/+0x24/+0x28/+0xFE0` | `1024 1024 1 1` (already observed in `i66`) |
| FL5 | (Thomas's agreement) Steam language `french`: ordinal 2, fonts `TsukuMinPro` + `TsukuOldMinPro`, accented glyphs appear in the glyph updates | ordinal 2. Reference1715: ordinal 2, the original's title and menus in French. **Native2051 / Reference2050** (`Run-LanguageWitness.ps1`, Eta save, 2026-10-18): FL1 equal (region 1, text 2, live 2, single-SKU 0); title, notices, main menu, the in-game menu and the Equipment page in French with their accents, the native pages as the original's (`pair-*.jpg`). Native2049 had stayed English: the test hook was installed in Reference runs only (fixed) |
| FL6 | (Thomas's agreement) Steam language `japanese`, then `schinese`: ordinals 0 and 20, `font.gfx` of 10.2 MB / 4.8 MB, maximum records per update call, cache refills during an item list scroll | ordinals 0, 20; no failure at more than 256 records. **Japanese: Native2053 / Reference2052** (2026-10-18): the title, the menus and the Equipment page in Japanese, the native pages as the original's, no glyph refusal row. **Simplified Chinese: Native2058 / Reference2054**: FL1 text/live 20; Native2055 refused at the load (`native_cpu_text_layer_1_unavailable`: the CPU text layer kind 1, 1D00920, the clipped rectangles filled through the scratch tessellator), now admitted: the menus and the Equipment page as the original's |
| FL7 | (Thomas's agreement) Steam language `danish` (unsupported) | ordinal 8. **Native2057 / Reference2056**: FL1 text/live 8, English menus in both |
| FL8 | Settings object `+0x2C/+0x30` of the live profile (`[[1447572B8]+0x58]`) | `-1` |
| FL9 | (Thomas, 2026-10-16 and 2026-10-18: the Asian languages and one Cyrillic) Steam languages `russian` (Cyrillic, TsukuMinPro x2), `tchinese` (TraditionalChinese + TsukuMinProSub), `koreana` (Hangul x2) | ordinals 11, 18, 19. **Russian: Native2074 / Reference2073; Traditional Chinese: Native2076 / Reference2075; Korean: Native2078 / Reference2077** (2026-10-18, `Run-LanguageWitness.ps1`, Eta save): FL1 text/live 11, 18, 19 in both modes, single-SKU 0; the title, notices, main menu, the in-game menu and the Equipment page in each script, the native pages as the original's (`pair-*.jpg` in the native runs), no `native_ui_*` failure or glyph refusal row. **Polish** (Latin Extended, its own font bundle): Native2088 / Reference2087, ordinal 10, the menus and Equipment page with ę, ń, ł, ś as the original's |

## 7. Subtitles and dialogue text (STATIC, bounded)

**Where the strings are (installed `msg` bundles, English counts):** the FMG categories (ids are the BND file ids):

| Category | Id (base / DLC1 / DLC2) | Entries (engUS, base+DLC1+DLC2) | Notes |
|---|---|---|---|
| conversation (`会話`) | 1 / 230 / 270 | 2,468 + 332 + 417 | NPC dialogue |
| blood messages (`血文字`) | 2 / 239 / 279 | 5,910 + 2 + 3 | player messages |
| movie subtitles (`ムービー字幕`) | 3 / - / - | 19 | the two `bik` movies: `fdp_opening`, `fdp_endroll_ww` |
| event text (`イベントテキスト`) | 30 / 231 / 271 | 1,225 + 85 + 64 | |
| menu common texts | 70, 76-81, 90 | 74, 722, 557, 161, 120, 111, 93, 27 | in-game menu, shared, dialogs, key guide, one-line help, tags |
| FDP texts | 200-206 / 232-239 / 272-279 | 1,113 ... | menu text, system messages for `win64`, `ps4`, `xboxone` |

Not every language has every category: `zhoCN` `menu_dlc2` has no ids 76, 77, 80, 81, 90 (32 files against 37 for `engUS` and `jpnJP`) and an
empty `ngword`; `jpnJP` has an extra item category 40. The installed `sound/*.fsb` have no language folder (690 `sound` rows in the archive index, none language-keyed) and `movie/` holds two `bik` files (installed-folder observation, not in the index), so the subtitle text is INFERRED to be the only language dependence of dialogue.

**Lookup (`FUN_142354090(manager, 0, category, id)`, a binary search over id ranges of the loaded FMG):** each text kind has a wrapper that
probes categories in order and returns a fixed fallback string if all miss: talk `FUN_140E1A140` probes `{0x68, 1, 0xE6, 0x10E}` (fallback
`?TalkMsg?`), movie subtitle `FUN_140E19C70` probes `{3}` (`?MovieSubtitle?`), blood messages `FUN_140E19420` probes `{0x6B, 2, 0xEF, 0x117}`;
the same pattern covers names, descriptions, key guide (`0x7A/0x4F`), one-line help (`0x79/0x50`). The first id of each list (102-122) is
absent from every installed bundle (`menu_dlc1`, `menu_dlc2`, `item_dlc1`, `item_dlc2` checked): the first probe always misses; **OPEN**
whether it is a console-only override slot.

**Display.** The HUD page `01_000_fe` (constructor `FUN_140AF8580`) owns the children `TalkWindow`, `TextEffect`, `MapName`, `DirectionText`,
`OperateGuide`, `CaptionWindow/Text/Text` (`FUN_140B43E10`); `01_920_movie` is the movie page; `03_060..062` are the dialogue choices;
`01_030..035` and `01_050` blood and grave messages. The dialogue and subtitle text is **not** written to `TalkWindow`: it goes to the separate page `01_060_Caption` (`CaptionWindow/Text/Text`), see [SUBTITLES-DIALOGUE-NATIVE.md](SUBTITLES-DIALOGUE-NATIVE.md); the writer of the HUD `TalkWindow` text remains **OPEN**. The only readers of the talk and movie-subtitle wrappers found are the old dialog helpers `FUN_14069A330` (talk message box
`FrpgMenuDlgTalkMsgbox`, text set with `FUN_141A0C180`) and `FUN_1406ED760`: they draw with the bitmap fonts of section 4. Consequently the
native work for subtitles is exactly the work of sections 4 and 5 (Scaleform text through the glyph cache, old dialogs through GX); there
is no subtitle-specific renderer. Option key present in the executable: `Game.Option.Disp.Subtitle` (`14283C030`, no reader found).

**Rows (SL).** SL1 with an English install, trigger one NPC talk and check the text field family (expected `MatisseProN` metrics) and that
the glyph updates stay small; SL2 play the `fdp_opening` movie (title first launch) to see `MovieSubtitle` (19 entries); SL3 non-English
verification on the same cases only with Thomas's agreement. The unread part (who writes `TalkWindow` text, whether subtitles use
`TextEffect`) is the next static step if the dialogue path becomes a priority.

## 8. Open points

1. Value of `DAT_14479BEDC` and of the encrypted `FUN_14080EAF0` on the Steam build (FL1, FL3).
2. Whether a saved profile ever carries a language other than `-1` (FL8).
3. Maximum glyph records per update in CJK runs (FL6) and the cache refill behaviour (F4, F5).
4. The write path of dialogue text is now fixed (SUBTITLES-DIALOGUE-NATIVE.md); only the HUD `TalkWindow` writer stays open.
5. Which old bitmap dialogs are still reachable in the release build (section 4).

## 9. Exports

Functions cited here are exported to `decompiled/` under the repository naming (`DS3-1152-F-<RVA>`): `FUN_140EF3720`, `140EA6020`,
`140EA6090`, `140EA5860`, `140EA61F0`, `140EA6320`, `140EA6450`, `140EA5D70`, `140EA5DE0`, `140EA5E20`, `140EA5E60`, `140EA5A10`, `140EA5A20`,
`140EA5AD0`, `140EA5BE0`, `140EA60F0`, `140EA49A0`, `140EA4910`, `140EA4BC0`, `140EA4AB0`, `140EA4E10`, `140EA4C60`, `140EA4D20`, `140E41330`,
`140E5A2F0`, `140E5CF30`, `140E4DCF0`, `140E4DD00`, `140E5F000`, `140E527B0`, `141C702E0`, `141C6DE10`, `1405A0320`, `1405A0740`, `1405A0930`,
`140598070`, `141BD8190`, `140E1A140`, `140E19C70`, `140E19420`, `140E1A370`, `142354090`, `14069A330`, `140DE9860`.
