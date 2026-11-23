# R12's plan: the Rekindled page, the precompile screen, the title screen

**Course change (D-0271, Thomas, 2026-11-10 23:06 Paris, « Vas y »; it replaces D-0270's drawn pages):** the Graphics and Advanced
pages stay the game's own Scaleform pages, with our rows inside them (R12c below). The native overlay of R12a (both tranches merged)
serves the shader precompile screen (R12d) and the « Rekindled » title screen (R12e) only.

The R12 thread, 2026-11-10 (Paris), after the study thread's [STUDY-R12-SETTINGS-PAGE](STUDY-R12-SETTINGS-PAGE.md) and Thomas's
choices (D-0265, D-0266; RT-PT-PLAN §8 R12). Each tranche merges once written and reviewed (D-0227), with its start check.

| Tranche | What | Exit |
|---|---|---|
| R12b | One settings file, `rekindled-settings.ini` (the model, D-0265's rules, the module's readers, the launcher's migration); D-0266's chromatic aberration and vignette, and the entries' table of when each takes effect, come with their readers in R12c | merged (RESULT « R12b ») |
| R12a | A native 2D toolbox drawn after the game's UI, in two tranches. **The first** (merged with RESULT « R12a »): the overlay pass, DS3's menu fonts read from `font.gfx` (DefineFont3 outlines rasterised into a glyph atlas, the menus' drop shadow, a font chain), textured quads with a colour, a test screen behind a flag. **The second**: DS3's menu pieces (`01_common.tpf`'s frames, cursor, arrows) from the textures the native already holds | the first: a still of the test screen (text in DS3's fonts, French accents) shown to Thomas; the second: a frame piece drawn |
| R12c | (D-0271, Thomas 2026-11-10 23:06, after D-0270) Our rows in the game's own pages through its row functions (R12C-STATIC §8): the Graphics tab Screen Mode (Full screen > Borderless > Windowed), Resolution (the game's row), V-sync, the frame rate, the game's Advanced Settings button; the Advanced Settings page the Quality Setting preset, DS3's five rows (the game's labels, choices and state bytes), then our RT rows (`graphics_rows.h`: D-0265's rules, greyed and locked when unavailable); the cursor, sounds, inputs and help lines the game's, the tab's Defaults row kept, the Advanced page's left out (fourteen slots); the Advanced page's rows built again live when a governing row changes (Thomas, 2026-11-11 00:44) | in play, both pages reached, every Advanced row changing what it names (or saying « au prochain lancement »), a greyed row locked, a chosen preset kept (RESULT « R12c »); the pages reached from the title, Resolution and Borderless chosen from their rows and a later launch reading the page's file moved to R12f's exit (the final review's M2) |
| R12c bis | The door to a native page and the display, preset and DS3 rows' API (second R12 pilot, merged `0f9a608d`); replaced by R12c's rows in the game's pages (D-0271), its settings edit and screen-mode switch kept | merged (RESULT « R12c's door and our Graphics page's logic ») |
| R12f | (Thomas, 2026-11-11 06:21 and 06:22) Every setting applied live, in two tranches. **The first**: the module's readers take the page's settings live (`NativeSettingsCurrent`, the test flags still first): the TAA, the sharpening, the anti-aliasing kind and the DLSS mode, Reflex, RR, the RT or PT level and the bounces at the next engine frame (the renderer already decides them a frame at a time, its histories reset on a gap); the frame rate (DS3's limiter's pair put back at 60, the I-0114 corrections installed at every native start, k held at 1 at DS3's 60); the V-sync (the paced chain made tearing-capable, a Present tearing only without V-sync); the launcher's `-Borderless` and `-NoVsync` written as the settings' keys when absent instead of `borderless.flag` and `vsync-off.flag` (the final review's L11). **The second**: the frame generation, the swap chain made again when it is switched on or off, as NVIDIA's guide asks (ProgrammingGuideDLSS_G §18: DLSS-G loaded or unloaded with `slSetFeatureLoaded`, the chain then without or with our frame-latency object, §12.1); then NIS for the sharpening (Streamline's `sl.nis`, already in the SDK the project holds, its sharpen-only mode) and the sub-rows' separator | merged `c9db5355` (RESULT « R12f »: each row's change seen in play without a relaunch, the pages from the title, Resolution and Borderless from their rows; open there: frame generation in exclusive full screen, a later launch reading R12f's file) |
| R12g | (Thomas, 2026-11-11 06:25 to 06:34) The new rows: « └ GI quality » at the PT level (the GI's paths a pixel, 1 to 4, ReSTIR GI's initial candidates); the vignette (DS3's, On or Off), the last row (Thomas, 15:19); our scroll over the Advanced page's rows (the film's fourteen slots do not scroll: a window of fourteen built again when the cursor wraps past it) with the film's scroll bar (Thomas, 15:19); no anisotropy row: DS3's Texture Quality sets only that filter (the R12g Ghidra study), so Thomas kept the game's row (15:20 to 15:22; his wish for later, [TEXTURE-QUALITY](../../.project/wishes/TEXTURE-QUALITY.md)) | each row seen changing what it names in play, the page scrolled down and up past its edges with its bar, every RT help line in the box; merged `47ace3ed` (RESULT « R12g »: open there the GI quality's check, the mouse, the French help lines) |
| R12d | The shader precompile screen with its progress bar (our native programmes included) | the screen at a first start with an empty pipeline store; merged `fa1ffd43` (RESULT « R12d »: no freeze at the live switch to path tracing, the screen at the bottom right) |
| R12h | (Thomas, 2026-11-12 02:34 and 02:35 Paris: « je m'attendais à une nouvelle fenêtre juste avant l'écran titre », « une nouvelle page même ») R12d's corner line made a page of its own: black over the whole picture, centred, the heading, the bar, the percentage and when the step is long; the title not drawn while it is up | the page at a first start with an empty pipeline store, then R12e's word starting after it; stills in English and French (`precompile-screen-hold.flag`); merged (RESULT « R12h »: the page only past 3 s, D-0273, so not shown on a warm start; the English stills; the game's window black from its creation, no white frame in `r12h-page1fr3`; the French still and the word after a shown page in `r12h-page3fr`, the title bar dark; no real page on a cold driver cache) |
| R12h step 2 | (ROADMAP workstream 2's step 2) The native passes' 31 run-time HLSL compilations moved to build time (fxc), the same bytecode | merged at workstream 2's closure (RESULT « R12h step 2 »; on an empty store the native passes' pipelines still made at first use, carried to workstream 7) |
| R12e | The « Rekindled » title screen (D-0182, [title-screen-mockup.png](../../.project/design/title-screen-mockup.png)), in two tranches (below) | merged `40a92818`; its second look (Thomas's night of 2026-11-11/12) next (RESULT « R12e ») |

## R12e's design (the R pilot, 2026-11-11)

- **What** (D-0182's mock-up, then Thomas's night and morning of 2026-11-11/12): under DS3's logo, « REKINDLED » letter-spaced in
  ember colour in the titles' font (TsukuOldMinPro) between two rules, a crimson halo; a fade-in, then a glow crossing it every 10 s;
  DS3's logo tinted cream and burnt (a multiply layer); the mock-up's burnt ring and sword behind; 320 embers in a wind (about 12 %
  of the small ones in front of the title), a faint brown smoke at the corners. The start logos on black and HDR10 are not R12e's.
- **When**: the UI movies' advance hook reads each advancing movie's name: `05_001_Title_Logo` marks the title (500 ms of grace); its
  pages (`05_0*`, the Options `02_0*`) fade the word out and dim the rest (a 1.5 s hold), a message box (`01_01*`, a 400 ms hold) keeps
  the word at 35 % with our glow and embers out over its area, the loading screens (`02_9*`) and character creation (`04_*`) hide it.
  One row when it shows and when it hides (`native_rekindled_title`). DS3's fonts are read only while the logo shows.
- **Tranches**: the first draws the word, its rules, the fade-in and the glow (the overlay after the UI); the second the embers and
  the smoke, in a screen layer after the game's UI and before the overlay (a screen blend: light on its black, nothing on its white
  logo; an underlay before the UI showed nothing in `r12e-film3`, the title movie read as opaque).
  The second look (Thomas's night of 2026-11-11/12): the embers redone, a tint layer (a multiply blend) for the logo, the ring.
- **Exit**: stills and a film of the title for Thomas, in French and English, at 1280x720 and 1920x1080; the precompile screen
  unchanged; the Load Game list and the message boxes readable over the title.

## R12d's design (the R pilot, 2026-11-11)

- **Why**: the pipeline store compiles at every start what earlier sessions created (D-0241's list); a native pass first used in a
  session compiles at its first frame. I-0132 R12g's runs froze 12.3 and 9.7 s at the live switch to path tracing (`native_slow_frame`,
  the record's time; the PT level's paths and ReSTIR GI's draws). A sweep of every native pipeline site (an Explore worker) found the
  rays' own pipelines device-only and lazy: `RtReflections::Make`'s thirteen kinds, `RtLocalLights`' two.
- **The warm-up**: at start, with the rays usable, a worker thread (below normal, beside the precompile's) makes those fifteen (and
  SER's state object with SER on: below) through the store from the same descriptions as their passes (`Describe`, one per class), writing no member: the passes then find them in the
  store's memory, or wait for that name while it is built. Left out (small or diagnostic): the TAA, the exposure, the overlay, the
  velocity and composition passes (their HLSL compiled at their first use), the
  passes that need the game's programmes (in the list after one session). `no-native-warm.flag` the A/B.
- **The screen**: the native overlay (R12a) draws a line and a bar at the bottom of the picture, « Compiling shaders NN % » in DS3's
  fonts once its menus loaded them (the bar alone before), while the precompile or the warm-up runs; shown once it lasts half a second,
  hidden for good when both are done. The overlay's font observer is installed by default (`no-rekindled-overlay.flag` the A/B);
  `precompile-screen-hold.flag` holds the screen 4800 frames for its stills. SER's state object (outside the store) warmed too after
  `r12d-pt1` (8.3 s at its first PT frame), kept for MakeSer.

## R12h's design (2026-11-12)

- **What**: R12d's screen (a line and a bar at the bottom right, over the start logos and the title) becomes a page: black over the whole
  output, centred in DS3's 1280x720 units, « Compilation des shaders » in the titles' font (TsukuOldMinPro, the body fonts behind it for
  a code it lacks), a 440-unit bar (DS3's rule colour over a dark track), the percentage under it, and one dim line: « Cette étape n'est
  longue qu'au premier lancement et après une mise à jour du pilote graphique. » The same progress as R12d (the store's precompile and
  the native warm-up); DS3's fonts once its menus loaded them, the bar alone before.
- **When**: only while the work still runs 30 frames and 3 s after our first Step (the precompile and the warm-up start during
  `Start`, before it): the pilot's threshold for D-0273's « only when a compilation really happens », a warm start being over within
  about 1 s on that clock (`r12d-pt2` 1,028 ms; 0.2 to 1.3 s in R12h's runs); then held at least
  1.5 s (no flash), faded in over 0.25 s and out over 0.5 s once both are done; a page whose progress has not moved for 60 s closes
  anyway with a `stalled` row (no black page for the session if a compile hangs or the store never starts its precompile). The game boots behind it; the title screen's word (R12e) is drawn
  only once the page is over, so its intro starts after the page. Left: DS3's own title animation and the start logos play under
  the page (the movies are not held), and a button pressed during the page reaches the game (the Load Game list may then be what the page
  uncovers).
- **The window black from our start** (Thomas, 2026-11-12: « le blanc agresse les yeux »): `start-732bb5ed5`'s desktop captures showed
  the game's window white from 18.25 s to 28.05 s, before any logo (its client never painted until our first Present; our `Start`
  holds the window's thread about 7 s, `renderer.Start`). `Start` now, once, on the window's thread before the swap chain: the class's
  background black and the client filled black (`native_window_black` with its time from `Start`; `no-black-window.flag` the A/B).
  `r12h-page1fr2`: white 13.95-16.33 s (2.4 s, all before our `Start`), black from 16.44 s (before: 18.25-28.05 s). The rest: from
  `Setup` (the module injected as soon as the process exists), MinHook hooks on every path we know of, DS3's own not read (its
  user32 calls resolved at run time): `RegisterClassExW`/`A` and `RegisterClassW`/`A` (a class whose procedure lies in the game's
  image registered with a black background, a `native_window_black_class` row each), `CreateWindowExW`/`A` and `ShowWindow` (the
  game's visible top-level window filled black, `native_window_black_early`), a window made before them blackened at once; pass-throughs from
  `Start` on (never the window procedure). The title bar dark too (in windowed mode the light theme's white bar stayed;
  `DwmSetWindowAttribute` `DWMWA_USE_IMMERSIVE_DARK_MODE`, Windows 11, its HRESULT in the rows; the module links dwmapi), with the
  showing and at `Start`, under the same flag. `SetWinEventHook` not taken: removed with the thread that set it (`Setup`'s ends early).

## R12a's design

- **Where it draws**: a native overlay pass (`native/renderer/overlay_pass`) after `ui->Encode` in both the startup frames
  (`RenderStartupFrame`) and the world frames (`RenderModels`), into the same target, before the gamma pass, so the page sits above
  the game's menus and goes through the same display curve. Its own root signature, PSO and shader (HLSL compiled by `D3DCompile`, as
  the other native passes), one quad list a frame. The study (§3.1) proposed our draws inside the game's UI packet with no new pass;
  the packet's draws use the game's own Scaleform programmes and their constant layouts, so a pass of ours with its own programme is
  simpler and independent of them (the pilot's choice, recorded here); it is in the frame graph's plan key (bit 59).
- **What it draws**: an overlay frame (`overlay_frame.h`): quads (a screen rectangle, a texture rectangle, a colour, a texture index,
  coverage or colour sampling), built on the engine thread by the page and handed to the renderer as the latest frame (a shared
  pointer swapped under a lock; one frame of lag is invisible on a menu). The atlases (glyph coverage, the shadow, DS3's menu textures)
  are uploaded when they change.
- **The fonts** (`font_outline.h`, `font_raster.h`, header-only, unit-tested): DefineFont3 read from the movie (SWF shape records,
  advances, kerning), each glyph rasterised at the needed size by exact coverage (font-rs's accumulation), its blurred copy for the
  shadow, a shelf atlas. The fonts' bytes are read from the game's own memory, where its menus loaded them (the repository
  below), never a copy beside the module.
- **The pieces** (R12a's second tranche): DS3's menu textures (`01_common.tpf`'s) as the native's texture ingress already holds them,
  by name; the rectangles measured on the textures decoded offline (`study-r12-tools/decode.py`, `piece_boxes.py`; the output
  `evidence/I-0132/artifacts/r12a-pieces-measure.txt`), not from the films' bitmap fills (the study's §3.3, not read).
- **Scaling**: the page's layout in DS3's 1280x720 units, scaled by the output height.

## The game's files for the overlay (static reading, 2026-11-10)

A `ghidra-research` worker (Sonnet, GhidraMCP on DS3 1.15.2, no script; the addresses below re-read by the pilot where the code
depends on them) answered how the module can read `font:/<family>/font.gfx` (the live answers since: below, « Read live »):

- **The Scaleform opener does not read the game's file system** (OBSERVED): its OpenFile (`FUN_140E4E150`, opener vtable `14298ED10`
  slot +0x08, registered by `FUN_140E527B0` as state 0xC) serves bytes from an in-memory repository, `GfxRepositoryImp` (the global
  `144796CE8`, made by `FUN_140E4B8C0`), when the setting `Scaleform.SwfCache` is on, else a CRT `_wfopen` of the raw path that cannot
  resolve `font:/`.
- **The repository** is filled by the game's asynchronous file requests: `FUN_140503D30(file manager 1447559C8, wide path, 0, 0)`
  queues a `GfxFileCap`, whose completion (`FUN_1404F1370`) copies the raw bytes into the repository under the wide path
  (`FUN_140E0EA10`); the cap's destructor removes them. The menus request their two fonts this way (`FUN_140E5A2F0`), so once the
  menus are up the repository holds them. **The lookup** `FUN_140E0E980(repository, const wchar_t* path)` returns the node (or null):
  its bytes at `node+0x60`, its size (an int) at `node+0x68`. Case-insensitive, `/` and `\` folded. No lock on lookup (OBSERVED);
  called from the game's main thread, where the menus run (INFERRED safe).
- **The menus' `.tpf`** go through another cap (`TpfFileCap`) that hands the bytes to the texture parser and drops them: the raw
  `01_common.tpf` is not kept. The pieces come instead from the textures the native already holds (the texture ingress's records, by
  name), R12a's second tranche.
- **Read live** (R12a's witnesses `i132-r12a2-overlay` to `i132-r12a5-overlay`): the repository keys the **resolved** path,
  `data1:/font/<family>/font.gfx` (the menus' films `data1:/menu/...`), not the `font:/` form, which found nothing; the fonts arrive
  there, so `Scaleform.SwfCache` is on (its default no longer matters here). A family the game did not load (TsukuMinPro in English)
  is requested through `FUN_140503D30` with that path and arrives the same way.
- **A thread risk** (R12b's final review L8, R12a's step review M1 and final review M2): the lookup takes no lock while the file caps
  add and remove nodes on another thread than the engine's (OBSERVED in `i132-r12a6-overlay` and `i132-r12a7-overlay-debug`). R12a never calls the lookup: an observer of the insertions
  keeps each family's node as it is inserted, and the engine thread copies the bytes from it once (guarded reads); the inserting and
  the engine threads are named in rows (`native_overlay_repository_insert`'s `thread`, `native_overlay_engine_thread`).

## The pages' code

Our rows in the game's pages (D-0271): `02b_rekindled_graphics.inc` (the game-side glue, its record [R12C-STATIC](R12C-STATIC.md) §8)
and `graphics_rows.h` (the rows' model, unit-tested); the second R12 pilot's door and its API were dropped with D-0271, its settings
edit and screen-mode switch kept.

Next after R12c (Thomas, 2026-11-11 06:21: « ça peut pas se faire en direct ? »): the settings applied live where they can be (the TAA,
the sharpening, Reflex, the frame rate at once; the anti-aliasing kind, the DLSS mode, RR, RT and PT, the bounces through a short
recreation); then (Thomas, 06:22: « c'est pas possible de tout faire en temps réel ? ») the frame generation too (DLSS-G loaded at
start, switched live, Reflex as it requires) and the V-sync (the swap chain made with tearing allowed, the sync interval chosen each
frame): every setting live; then NIS and the sub-rows' separator.

New rows after those (Thomas, 2026-11-11 06:25 to 06:34, « si t'as d'autres idées d'options », « ou autre, s'il manque des options »):
the texture quality and the anisotropic filtering (x2 to x16) as two rows; a GI quality at the PT level (the paths per pixel, beside the
bounces); the vignette (DS3's tone map has one) On/Off: R12g above (DS3's Texture Quality is only the filter: the game's row kept). Proposed to Thomas the
same day, not yet chosen: the bloom (DS3's glow), the light shafts, the film grain, the field of view, an HDR output. A volumetric fog is
not in DS3 (none in our renderer either): a new effect, later, rather with the PT. The chromatic aberration: only DS3's own, never
an added one (Thomas, 06:36); D-0266 names one in DS3's YEBIS tone map (a lateral dispersion), not found in our renderer's code:
to verify in the original's tone-map programme before any row.

The pages' words: English and French ours; every other language our rows in English, the game's own words in its language;
another language only with official terms (the game's FMG words, NVIDIA's localised names), never invented (Thomas, 2026-11-11 04:33). The
sharpening: NVIDIA's NIS in place of our pass next (Thomas, 03:53); AMD's CAS later with FSR, not a priority (Thomas, 03:53). The sub-rows' separator line a quarter shorter (Thomas,
05:11; the line is an unnamed part of the slot's drawing in the film: how to size it to find; shortened from its left end, Thomas 05:12; his mock-up `evidence/I-0132/artifacts/r12-mockups/subrow-separator-thomas-2026-11-11.png`) next with NIS.
