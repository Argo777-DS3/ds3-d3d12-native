# I-0135 result: L'écran Bandai Namco sur fond noir

Branch `claude/e-bne-dark` (worktree `.claude/worktrees/e-bne-dark`, made at 02:51 Paris; the investigation opened at 02:59), for
Thomas's request of 2026-11-12 02:48 (D-0182, D-0275), through the coordinator. Seen in three checks (05:59, 06:22, 06:24);
the final and integrity reviews answered (below); not merged.

## Conclusion

Reached on the screen (OBSERVED, the checks of 06:22-06:24, build `5464cd6c6`): the Bandai Namco page is black around its
logo, its fades are fades to black, the FromSoftware page is unchanged. Without any other file the page shows DS3's own logo
made for black (the public version); with `er-bne-logo.dds` beside the module (Thomas's PC, written once from his Elden Ring),
Elden Ring's 2022 logo, at 80 % of its Elden Ring size. Merged `5e7aff0a` (Thomas, 08:13: « Fusionne »); the start check `start-5e7aff0aa` passed and shows the page as above.

## Found (static reading, 2026-11-12 05:16-05:30)

- **The page's layers (READ, `05_901_logo_bne.gfx` from Data1, 3,072 bytes, read with `tools/gfx_read.py`).** Stage 1920x1080,
  30 frames/s, 152 frames, labels FadeIn (15 frames), Loop, FadeOut (16 frames). Root depth 1: shape 2, a full-stage solid
  **black** rectangle, never changed. Root depth 2: the sprite `Top` (6), which holds sprite 5: shape 3, a full-stage solid
  **white** rectangle, then sprite 4 at (960, 540) with **blend mode 3 (Multiply)**, which places the image `MENU_BNE_Logo`
  (character 1, `GFX_DefineExternalImage2`, 768x768, file `MENU_BNE_Logo.tga`) at (-384, -384): centred. The fades are `Top`'s colour transform, alpha only
  (0.04 to 0.94 in, 0.93 to 0 out). The movie's background colour (102, 102, 102) is not drawn natively (the display record's
  flag, below).
- **The image (READ, `menu/02_title.tpf`, entry `MENU_BNE_Logo`: DXT1, 768x768, one mip, decoded with
  `tools/logo_dark_preview.py`).** No transparent pixel; its border is white everywhere; the white connected to the border is
  61.7 % of the image; the white not connected to it is the 11 letters BANDAI NAMCO (42,651 pixels, inside the blob), the
  counters of the two « e » and the « a » of « Entertainment » (954 pixels) and a 3-pixel sliver of the ™. « Entertainment » and
  the ™ are red on the white.
- **So H1 alone fails (READ).** Without the white rectangle the image is multiplied over the black: the whole page goes black.
- **Identity (OBSERVED, STATIC in our code).** `05_901_Logo_BNE` is the first logo, advanced from the start to about 3.5 s,
  then `05_900_Logo_FromSoft`, then `05_000_Title` at about 7.1 s (`evidence/I-0066/artifacts/r12e-film7/startup.jsonl`).
  `GameUiSession::PrepareFrame` calls E52EE0 for one movie at a time: a display begun inside the Bandai Namco movie's call is
  that page's, by its name (movie+0x28), not by its look.
- **Black behind the UI (STATIC, our code).** The logos have no world frame; `RenderStartupFrame` clears to (0, 0, 0, 1) before
  the UI; `BeginCpuDisplay` refuses a display record's background colour unless its flag 0x200 is set.
- **A per-pixel luminance inversion is rejected (the coordinator, 02:57):** the white letters and the white background are
  the same colour.

## The change (D-0182, the coordinator's agreement of 05:19)

- `native/frame/dark_logo.h` (header only, no build script change): `DecodeBc1` and `DarkLogoImage`, the rule:
  1. white: min(R, G, B) >= 245; the non-white pixels labelled (4-connected); the blob, the largest component;
  2. a white component is kept (a letter on the blob) when it does not touch the border and touches the blob; every other
     one is background (the outer white, the counters of « Entertainment », the ™ sliver);
  3. colour-to-alpha from white (alpha = max(255 - c), colour unmixed) on the background, on the non-blob components that
     touch it (the red letters; identity on their saturated core) and on the blob's pixels within 3 pixels of it; the blob's
     interior, the kept letters and their counters (the 7 orange counters of B, A, D, O enclosed by a kept letter) untouched.
- `native/frame/game_ui_session.cpp`, `DarkLogoAdapt`, on the page's draws only (`dark_logo_display`), before their images take
  the frame's identities: an untextured draw is drawn in Multiply (the black rectangle stays black, the white one leaves the
  black under it, whatever encodes their colour); the image's Multiply draw samples the copy (RGBA8, straight alpha, made once
  from the game's bytes and kept; a frame's lent copy only takes the blend) and blends in premultiplied alpha. An image of
  another shape, or one the rule refuses, is left as the game draws it (`refused` in the page rows). The journal's page rows
  count the call's `dimmed` and `imaged` draws.
- The program of a Multiply draw outputs its colour premultiplied by its alpha: INFERRED from the blend table (Multiply is
  DEST_COLOR / INV_SRC_ALPHA, I-0067 MENUS-MAP « Blend states », which fades correctly only from a premultiplied source); if
  it is wrong, the logo would not fade (the run's fade frames tell).
- **Checks.** The offline preview (`tools/logo_dark_preview.py`, the copy composed on black, `bne_dark_on_black.png`, outside
  Git beside the extraction): BANDAI NAMCO white, the blob intact, « Entertainment » and the ™ red with black counters; no
  light pixel (min > 100) outside the letters' box; the 205 dim low-saturation pixels are the anti-aliased edge (channel max
  <= 39), not a fringe. A first rule touched the counters of the white letters (a dark line inside B, A, D, O, seen at x4) and
  was corrected. The C++ copy equals the preview's raw copy byte for byte (a scratch harness compiled with clang++ 23.1.3, not in
  the build; 0 of 2,359,296 bytes differ); it refuses a wrong size and a truncated block array. clangd: no diagnostic.

## Elden Ring's logo (Thomas, 05:40; read 05:42-05:50, Elden Ring 2.7.1, read-only)

- **Where (READ).** `tools/Read-EldenRingArchive.ps1` with `tools/ErArchive.cs` (a local reader, never shipped): the BHD5
  indexes RSA-decrypted with the five public keys `eldenring.exe` carries in clear (PEM), 40-byte entries (64-bit name hash,
  `h * 0x85 + c` over the lower-case path), the BDT range read exactly, its keyed ranges AES-ECB-decrypted, the DCX (Kraken)
  decompressed by the game's own `oo2core_6_win64.dll`. No Ghidra, nothing downloaded, nothing written in the game folder.
- **Elden Ring's page (READ, `/menu/05_901_logo_bne.gfx`, Data0, modified 2023-10-30).** The same timeline as DS3's (152 frames
  at 30 frames/s, FadeIn 15 frames from 0, FadeOut 16); a black full-stage rectangle under a black one inside `Top`, and the
  image `MENU_BNE_Logo` placed at (480, 446) at half scale, normal blend (no Multiply): 960x220 centred at (960, 556).
- **The image (READ, `/menu/hi/02_title.tpf.dcx`, Data0).** `MENU_BNE_Logo`, BC1 1920x440, opaque: the 2022 logo (the red
  speech bubble, BANDAI NAMCO in white) on a black ground baked in. Nothing to cut out.
- **The layout kept (DECIDED here).** DS3's page draws its image as a 768x768 square centred on the stage; Elden Ring's logo is
  960 wide. Without changing the draw's vertex constants (not read; a run would journal them), the logo is fitted to the
  square's width at its aspect (768x176, area-averaged: 80 % of Elden Ring's size) and centred at y 556 as in Elden Ring, on
  opaque black, with DS3's page timeline and fades.
- **The script (`tools/Extract-EldenRingLogo.ps1`).** Run once on the player's PC: writes `er-bne-logo.dds` (768x768 RGBA8,
  uncompressed DDS) beside the module (`%LOCALAPPDATA%/DS3-DX12` by default, `-Out` a test build's folder). Tested into
  `C:/Users/User/Tools/DS3-Decompilation/er-bne` (SHA-256 1B174D59…C171); its page preview at half size there
  (`er_page_mod_half.png`, Elden Ring's own layout beside it in `er_page_er_half.png`).
- **The module (`LoadErLogo`, `frame::ParseLogoDds`).** At the page's first image draw it looks for the file once; a file of
  exactly that shape replaces the logo made for black (same draw, same premultiplied blend; `er` in the page rows); any other
  file, or none, leaves the fallback. The parser read the script's file (589,824 opaque pixels) and refused a truncated file
  and another size (scratch harness, clang++ 23.1.3, not kept; the build's `dark_logo_test.cpp` replaces it, below).

## Run start-732bb5ed5 (2026-11-12 05:59-06:00, the coordinator's slot; the build of `732bb5ed5`, the start check)

- **The check (OBSERVED, `evidence/I-0066/artifacts/start-732bb5ed5`).** Passed: the world drawn (364 frames), the game closed
  by itself, no refusal, no failure, the saves restored by hash (Shebang's eta-m40 save, not Alpha).
- **The page's identity holds (OBSERVED, the journal).** 3 marked draws a frame (`native_ui_dark_logo_page`, frames 1, 2 and 90
  at 1.5 s): two untextured rectangles, their vertex colours black (000000ff) then white (ffffffff), then the image (fill kind 6,
  768x768, format 71 = BC1). `dimmed` 2: both rectangles went to Multiply.
- **The image was not replaced (OBSERVED): `imaged` 0.** The game's image is a resident resource bound as is (`external` 1): the
  module freed its bytes at its upload (I-0096), and the rule skipped it. So the page was black without its logo (the
  captures, `evidence/I-0135/artifacts/start-732bb5ed5-captures`, the sheet `start-732bb5ed5-sheet.jpg`: black in the game's
  1280x720 window where the page shows). A failure of `732bb5ed5`, corrected after it (below).
- **Seen in passing, not this change's (OBSERVED, the sheet).** The game's window stays white about 10 s before its first frame,
  before any logo; handed by the coordinator to the precompile screen's thread.
- **The correction (code, after the run).** The admission keeps a copy of the one texture named `MENU_BNE_Logo` (its TPF entry
  name at decode, `14_scene_step_and_storage.inc`, `dark_logo_kept.h`, the coordinator's option a; a `native_ui_dark_logo_kept`
  row), the UI session makes the logo for black from it once and frees it, and recognises the image by its resource; with
  Elden Ring's file the image is replaced whatever it is. The rectangles are dimmed only when the image will be replaced.

## Runs start-5464cd6c6 and start-5464cd6c6e (2026-11-12 06:22-06:24, the coordinator's slot; build `5464cd6c6`)

- **Both checks passed (OBSERVED, `evidence/I-0066/artifacts/start-5464cd6c6` and `start-5464cd6c6e`):** the world drawn (344
  and 358 frames), the game closed by itself, no refusal, no failure, the saves restored by hash. The second differs from the
  first only by `er-bne-logo.dds` in the build folder (`Extract-EldenRingLogo.ps1 -Out`, between the two).
- **The journal (OBSERVED).** Both runs: `native_ui_dark_logo_kept` (768x768, format 71 = BC1, 294,912 bytes); every journaled
  frame `dimmed` 2, `imaged` 1, the image RGBA8 (28) in premultiplied alpha (blend 3); `er` false then true.
- **The screen (OBSERVED, measured; captures `evidence/I-0135/artifacts/start-5464cd6c6-captures` and
  `start-5464cd6c6e-er-captures`, picked by the journal's ticks: 0.3, 1.5 and 3.2 s after the page's first advance and 1.5 s
  into FromSoftware's; the sheet `start-5464cd6c6-pages.jpg`).** Ffmpeg's YAVG on two background crops of the game's window:
  0 in all eight pictures; on the logo's crop: 61, 110, 74 (DS3's logo: fade in, full, fade out), 11, 27, 20 (Elden Ring's),
  and 43.09 on FromSoftware's page in both runs. The pictures: `cap-00298-0029744`, `cap-00310-0030938`, `cap-00327-0032640`,
  `cap-00345-0034438` (run 1) and `cap-00257-0025644`, `cap-00269-0026844`, `cap-00286-0028542`, `cap-00305-0030443` (run 2);
  the crops (ffmpeg `crop=w:h:x:y` on the 960x540 pictures, the window's client from y 15): background `120:60:20:40` and
  `120:60:500:300`, logo `260:120:190:120`. The integrity review's own crops agree (background YMAX 0; the fades). By eye: DS3's blob in its colours with BANDAI NAMCO white and « Entertainment »
  red; Elden Ring's red bubble with BANDAI NAMCO white; no fringe seen at that scale.
- **So the premultiplied output of the Multiply program (INFERRED above) holds:** the logo fades with `Top`'s alpha.
- **FromSoftware's page:** equal in both branch runs, and unchanged by construction (only draws inside the page's own display
  are touched); not compared with a run of `main` (INFERRED unchanged from the code, OBSERVED equal across the two runs).
- **Provenance.** `build-bne` and `build-bne2` were built from the working tree then committed as `732bb5ed5` and `5464cd6c6`
  with no edit in between; main brought no native change (the merge `e0ba10f99`).

## After the merge (2026-11-12 08:50-09:09)

- Merged into `main` as `5e7aff0a` (main `4cab0003` merged into the branch first, `c2675356f`; its tree built with its
  tests, `evidence/I-0135/build-bne6`, 120 PASS). The launcher's module rebuilt from `main` (120 PASS).
- `Extract-EldenRingLogo.ps1` run once for Thomas: `er-bne-logo.dds` in `%LOCALAPPDATA%/DS3-DX12`, where
  `tools/Launch-DS3-DX12.ps1` runs the module (SHA-256 1B174D59…C171).
- **The start check `start-5e7aff0aa` (09:07-09:08, OBSERVED):** passed (346 world frames, closed by itself, no refusal or
  failure, restored by hash). It runs the launcher's build folder, without the Elden Ring file: the public fallback, the
  code of `4d0d134db` seen for the first time. The journal: `native_ui_dark_logo_kept` once; frames 1, 2 and 91: `dimmed` 2,
  `imaged` 1, `restored` 0, `er` false. The captures (`evidence/I-0135/artifacts/start-5e7aff0aa-captures`, picked by the
  journal's ticks: `cap-00303-0030237`, `cap-00315-0031441`, `cap-00332-0033134`, `cap-00350-0034937`; the sheet
  `start-5e7aff0aa-pages.jpg`), the same crops as above: background 0 in all four; the logo's crop 61, 110, 74 (the same as
  the check of `5464cd6c6`), 23 on FromSoftware's page 1.5 s in (43 in the branch's checks: the picture fell at another
  point of that page's fade in; not compared further).

## Final and integrity reviews (2026-11-12 06:30-06:40, on `main...HEAD`, main `465e41560` merged in at `e0ba10f99`)

- **Integrity (integrity-reviewer):** no high or medium finding; the judge untouched, no weakened check, the special case
  stated and contained, nothing cut during a measure, the numbers matching the journals and its own crops. Its low findings
  answered here: the pictures and crops named (L1), FromSoftware's page labelled as compared (L2), the scratch harness said
  not kept (L3), the records refreshed (L4).
- **Final (final-reviewer, xhigh):** no high finding. Answered in code (`73245ca4d`, its own step review below): M1 the kept
  copy only for a BC1 768x768 texture, and a display whose image was not replaced gives its rectangles back their blend
  (never a black page without its logo); L1 the page's images not lent, a replaced image registered without its snapshot
  origin; L2 60 frames after the page has gone, the logo, its resource and the kept copy released and nothing more kept;
  L3 no byte comparison; L4 the per-draw rows removed; L5 `dark_logo_test.cpp` in the build's tests. M2 `er-bne-logo.dds`,
  which the second check's build folder held in the Evidence Store, moved at 06:31 to
  `C:/Users/User/Tools/DS3-Decompilation/er-bne/er-bne-logo-build-bne2.dds` (the coordinator's agreement: a build file, not
  evidence). M3 the records refreshed and D-0275 written. L6 provenance stated above; L7 FromSoftware's page labelled.
  Kept: the logo for black is made on the frame thread at the page's first draw (one pass over 768x768, once).
- **Step review of `73245ca4d` (step-reviewer):** no high or medium finding. Its lows answered in `4d0d134db`: a copy admitted
  while the page is released is dropped (L1); a display whose rectangles were given their blend back counts them as
  `restored` in the page rows, not `dimmed` (L2); the comments on lending and lifetime (L3); the test now covers the blob's
  edge band with a colour of no zero channel (unmixed at the edge, untouched inside) and BC1's four- and three-colour blocks
  (L4). Kept (L5): the release follows 60 frames with movies but without the page, and needs the page to have shown; a page
  never shown keeps the copy (about 288 KB) and the name check for the run.
- **Visual check of `4d0d134db`:** at the start check after the merge, with captures (the coordinator's plan); its build
  `evidence/I-0135/build-bne4`.

## Step review (Fable 5.2, step-reviewer, 2026-11-12 03:41-03:48, read-only, the diff `14bfb35f`..`30f7b31f`)

No high finding. Fixed: M1 the image columns read before the frame's remap (the image and vertex buffer now read from the frame);
M2 an empty or partial journal not interpretable (the page rows; frames counted at the first marked draw; a sample by time, not
the 120th frame); M3 vertex colours not journaled (the first vertex, the elements, viewport and scissor); L2 the draw index reset
for every movie (now only for the page's calls); L3 a dead test; L4 a non-finite constant (printed 0, `finite` false); L5 the
local `frame` shadowing the namespace; L6 D-0161 recorded in the CONTRACT; L7 these records; the reader's limits (initial fill
styles only, embedded image tags named, the external image tags' leftover bytes, PlaceObject3's blend mode). Kept: L1 a deferred
display would be left unmarked (STATIC: `Display` calls `beginDisplay` directly; the page rows check it in the run); L4 the
journal stays unconditional until I-0135 closes (then removed or put under a trace).

## Uncertainty / unresolved questions

- A display deferred to `EndCpuFrame` would not be marked (the page would stay white); the page rows check it.
- `gfx_read.py` first misread sprite 4's PlaceObject3 (a class name read where DS3's exporter writes none); corrected, it places
  character 1, the image.

## Artifacts produced or modified

- `evidence/I-0135/artifacts/recover-bne-gfx.json`, `recover-bne-tpf.json`, `recover-bne-tpf2.json`: the read-only recovery
  reports (names, ranges, hashes; no game bytes).
- Outside Git and the Evidence Store, never published: `C:/Users/User/Tools/DS3-Decompilation/gamedata-bne` (the extracted
  gfx and TPFs, `bne_logo.dds`, the previews `bne_dark_on_black.png`, `bne_dark_mask.png`, `bne_dark_zoom.png`,
  `bne_dark_zoom2.png`, the raw copy `bne_dark_straight.rgba`).
- Outside Git, never published: `C:/Users/User/Tools/DS3-Decompilation/er-bne` (Elden Ring's page and title textures read for
  this, `er_bne_logo.dds`, the test `er-bne-logo.dds`, the page previews).
- `evidence/I-0135/build-bne` to `build-bne4`: the builds of `732bb5ed5`, `5464cd6c6`, `73245ca4d` and `4d0d134db` (module,
  controller, tests); `build-bne2` no longer holds `er-bne-logo.dds` (moved out, above).
