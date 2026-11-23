# Menus workstream: what is done natively, what is missing, and what static knowledge exists

Written 2026-10-11 for the menus workstream announced in [ROADMAP](../../.project/ROADMAP.md). It reconciles the
ROADMAP's three menu items with the repository state ([STATE](../../.project/STATE.md),
[MENU-OFFSCREEN-MODELS](../I-0066-independent-native-startup/MENU-OFFSCREEN-MODELS.md)) and lists, per capability,
the native position, the static reading that exists ([MENUS-MAP](MENUS-MAP.md) sections), the static gap that
remains, and the live-only checks. Statuses are those of the native notes; nothing here was run.

## 1. The three ROADMAP items against the repository

| ROADMAP item (2026-10-10) | Position now | Evidence |
|---|---|---|
| In-game menu "does not open natively" | **Done**: the pause menu and the Equipment top open (a 2.5 s Esc: presses shorter than a native frame at 8 fps were the earlier failure), Status opens with the native character views | Native1156 against Reference1155; Native1163, Native1169 |
| Load Game screen "faults natively" | **Done**: opens (Native1159), portraits drawn natively with the FaceGen stream and the alpha-tested hair row (Native1170, 1176-1182), Argo loads (row press then OK held 400 ms) | MENU-OFFSCREEN-MODELS |
| Menu music and sound effects "if still absent" | **Done**: music (category 6, id 1) and cursor/decide requests reach the MenuMan sound object natively; Thomas hears them. FMOD is not renderer-bound | Native1175 |

The ROADMAP sequencing paragraph still says "does not open natively" and "faults natively"; it is refreshed in this change.

## 2. What is left, by capability

| Capability | Native position | Static map | Static gap | Live-only |
|---|---|---|---|---|
| Scaleform pages: HAL (blend, mask, fills, colour transform) | Title, Load Game, Equipment top, Status, Create Character, MessageBox, face grid witnessed | sections 33, 34, 36, 50, 78 | none for blend (all four modes used are admitted), masks (stencil scopes), fills | one witness per family of section 78 (Inventory/Item Box, PC graphics options, HUD) |
| **Scaleform filters** (DropShadow 137, Glow 52, Blur 90) | **native** (2026-10-11, Native1268-1270: blur, drop shadow and glow passes drawn natively through packet targets; HUD glows and blurs witnessed, pages identical to the original; kind 1 and the 7x7 branch not yet seen on screen; other kinds drawn without effect) | section 78: the three types and every parameter set; [SCALEFORM-FILTERS-NATIVE](SCALEFORM-FILTERS-NATIVE.md): the original pass loop, programme keys, uniforms and an ordered checklist | none left statically; the implementation needs targets inside a primitive packet (F1) | image comparison against the original on the HUD and a list page |
| Cache-as-bitmap (colour-select page) | `native_ui_display_offscreen_unavailable` | sections 33, 78 | same offscreen target as the filters | Create Character colour select |
| Item icons, banners, language TPF | icon sheets resident, Status icons identical | sections 52, 53, 61 | `80_language.tpf` admission per language; banner image names (section 61) | HUD banner (YOU DIED) in game |
| Fonts and languages | glyph context exists; glyph cache path is CPU | section 78: 8 font movies, 21 language ordinals, two font tables | which fallback folder the 8 ordinals without a `80_language.tpf` folder use; glyph-cache size for CJK | one non-English run (Steam setting: Thomas's agreement needed) |
| **Options pages** (graphics, mouse, keys, sound) | no native witness in the notes; **the GX quality object is not built, so no graphics option reaches the renderer** | section 77 (file, tokens, 12 slots, table decoded per level, readers); [QUALITY-OBJECT-NATIVE](QUALITY-OBJECT-NATIVE.md): how native builds the object from the live config block, with oracle bytes | the config to quality-block copy and the page logic are in encrypted code (`SprjDrawStep` 140D54xxx, `NewMenuSystem`) | read `[[144961B50]+0x5360]` and the levels once in the original; open the PC graphics page natively |
| Brightness / gamma | gamma pass and brightness option mapped | I-0068 DISPLAY-SWAPCHAIN-NATIVE; section 78 (the New Game brightness page) | none | brightness page needs a new game |
| Status character views | drawn natively (body, face) | sections 24-27, 48 | hood shadow (section 48): shadow family not implemented (SHADOWS-NATIVE) | compare the hood shadow |
| Fade plates, loading screens, movies | fade plates mapped; Bink player ready to code | sections 4, 9, 37, 38; I-0068 BINK-NATIVE-READY | none | opening movie and fade plates: native run |
| Warp / bonfire / shop / level-up pages | no native witness in the notes | sections 50, 52, 53 (`MapImage`, `Knowledge` TPF families) | the `MapImage`/`Knowledge` image admission by name | one bonfire visit |
| **Menu input at low frame rates** (Load Game confirmation flaky) | clicks and key taps are polled once per update; at 0.67 to 0.84 s per update on the Load Game screen a 400 ms pilot press is often lost | [NATIVE-INPUT-LATCH](NATIVE-INPUT-LATCH.md): chain, recorded numbers, latch correction (one effective update per press, no stretch) | none left statically | NI1 to NI7 (unit test, 20 loads, in-game tap) |
| Phantom variants through Load Game | **witnessed 2026-10-18** on Argo (embered) teleported to sign 1 (Lion Knight Albert, c0000_0007, (6.7, -6.3, 87.1); `Run-PilotPair.ps1 -Teleport -Summon`): the sign prompt, the summon dialog ("Summon this phantom?" with Albert's portrait) and the summoned white phantom as the original's (Native/Reference2069, `pair-summon-*.jpg`); the player's phantom colours and the Sword Master (sign 2) earlier (WAVE1-LAST-EFFECTS §4, §8). **Residual (for the final review):** the sign's ghost before the summon (PhantomParam kind 3, fade 0.5) is whiter natively and covers Argo's cape where the original keeps the cape in front. Same inputs in both modes: the alpha series (0.0125 to 0.25 over the fade-in, `reference_phantom_state` / `native_phantom_state`, Reference/Native2071) and the override rows (row 74 colours, Native2070); not the forward glow target (`no-forward-gb5.flag`, Native2072, unchanged). Lead: the order or depth of the 0x29 forward draws (native draws them in model order; the key order with the forward rows is an open wave-1 residual) | sections 59, 75, 76 | none | the ghost's draw order and depth against the host's parts |

## 3. Reading of the table

- The three ROADMAP items are met. The remaining menu work is **visual fidelity of the pages** (filters, offscreen
  targets, the shadow of the Status view) and **settings** (the quality object), plus witnesses of pages that have
  not been opened natively yet.
- Both big gaps have a complete static input now: the filter parameter sets and page lists (section 78) and the
  quality table and field readers (section 77). What remains for them is design and implementation.
- Live work is a short list: one read of the quality object, one witness per feature family, one non-English run,
  one bonfire/Inventory visit.

## 4. Page witnesses, menu tests (2026-10-18)

Reference against Native, the same pilot in both modes, screenshots side by side (`pair-*.jpg` in the native run), pages named by
the `ui_movie_first_advance` rows. "As the original's" means the same page, text, icons and layout on the screenshots.

| Page (movie) | Route | Runs | Result |
|---|---|---|---|
| Bonfire `03_002_Bonfire`, Travel `02_080_Warp` (map images) | `Run-AlphaCast.ps1 -BonfirePages` | Reference2066 / Native2067 | as the original's |
| Inventory `02_020_Inventory` with `02_050/058/051_DetailStatus_*` | `Run-AlphaCast.ps1 -MenuTour` | Reference2083 / Native2084 | as the original's |
| System, Game Options `02_040_OptionSetting` | same (not shown by `-MenuTour`'s Reference2083 / Native2084: its two Esc a page, I-0140 tranche 30); `-OptionsTour` | same; Reference2085 / Native2086 | as the original's, and its eight tabs (Game, Camera, Sound and display, Brightness, Network, Input device, Graphics, Quit Game), all within `02_040`, Reference2089 / Native2090 (Reference/Native2085-2086 had missed the tab row) |
| Equipment arrows `02_011_Equip`, `02_054/055_DetailStatus_*`, comparison `03_033_Comparison_equip_Arrow` (a registry-only page, MENU-TESTS-PREP 3.2) | same (the tour's last presses landed in Equipment) | same | as the original's |
| Attune Spell `03_020_Spell`, `02_053_DetailStatus_Spell` | `-MenuTour` (Reference2079-2082 only) | Reference | opened; no synthetic key took the list back (Esc, Backspace; a right click opens `02_045_PC_CommandList`): the tour now opens it last |
| Summon sign prompt and dialog (`01_010_MessageBox` family), the summoned NPC | `Run-PilotPair.ps1 -Teleport -Summon` | Reference/Native2069 | as the original's (section 2, "Phantom variants") |
| Steam languages: title, notices, main menu, in-game menu, Equipment | `Run-LanguageWitness.ps1` | FONTS-LANGUAGES-NATIVE FL5-FL7, FL9 | as the original's in French, Japanese, Simplified and Traditional Chinese, Korean, Russian, Polish, Danish (English) |
| End credits and "Begin journey 2?" | `Run-AlphaCast.ps1 -SetFlag "20 1"` | Reference2064 / Native2065 | as the original's (ENDING-CREDITS-NATIVE R5) |

Not a menu finding, seen on the same screenshots: at the Firelink bonfire the left archway's interior shows a lit brick wall
natively where the original keeps it dark (Reference2083 / Native2084 `pair-status.jpg`); a scene residual, outside the menu audit.

Character creation's colour select `04_030_ChrMake_ColorSelect` (the cache-as-bitmap family, where MENU-TESTS-PREP 3.3 expected
`native_ui_display_offscreen_unavailable`): opened in both modes through New Game, Appearance, Base Skin Color
(`Run-PilotPair.ps1 -Pilot CharMake -ColorSelect`, Reference/Native2091): the swatch grid and the RGB sliders as the original's, **no
refusal row**: the page does not reach the direct offscreen display. Both runs end on the same exception at the window close in
character creation (the same RIP and code bytes in Reference and Native: the original's own fault at closing there, not a native one).
The preview character's skin looks a little warmer natively (outside the menu audit; MENU-OFFSCREEN-MODELS).

Not witnessed by this tranche (queue of MENU-TESTS-PREP 3.4): the Item Box `03_050` (no synthetic key leaves the bonfire's attunement
list, so the tour stops there), Level Up and the shops (an NPC route), the New Game brightness and controls pages `05_031/05_032`
(after a completed creation), and the small families (blood messages, gestures, Kick, Ranking, Coliseum, text input).

Pages opened by hand (Thomas at the keyboard, 2026-10-18; `Run-AlphaCast.ps1 -HoldSeconds 600 -HoldShotEvery 10`, Alpha's copy at
Firelink), Native2092 / Reference2093: Status `02_070`, Level Up `03_010`, the Handmaid's shop `03_000/03_001`, the Item Box `03_050`
with `02_031_NumSelect`, Estus allocation `02_100`, Andre's reinforcement `03_040/03_043` and comparison `03_030`, `01_011_MessageBox_Small`,
the detail-status pages (armour, spell, item, weapon): the same pages in both modes, no `native_ui_*` refusal, the native pages as the
original's (`pair-*.jpg`, `sheet-pairs.jpg` in Native2092). Seen on the same native captures, outside the menu audit: a full-screen
brown haze at the moment of sitting at the bonfire (Native2092 `hold-33.png`): the original draws the same haze for about four seconds (`Run-AlphaCast.ps1 -BonfirePages` bursts, Reference2094 / Native2095 `sheet-sit.jpg`), so it is the game's rest effect, drawn natively as the original's.
The Travel page's map image of the Ringed City tab is black natively (Native2092 `hold-39.png`; not opened in Reference2093).
Keyboard note (Thomas): pages that ignore Esc take a right click (the command list) then its Back entry.
