# I-0135 - L'écran Bandai Namco sur fond noir

Capability: C3 (wave 2, D-0182: « the start logos on a black background, in the manner of Elden Ring, in place of the white
Bandai Namco screen »). Asked again by Thomas on 2026-11-12 at 02:48 (the white screen hurts the eyes), relayed by the
coordinator; always on, no option row and no flag (Thomas, 02:53, relayed).

## Question

How does the native renderer show the Bandai Namco page (`05_901_Logo_BNE`) on black, with the logo in its own colours and its
white letters, without any extracted or substituted asset and without touching any other page?

## Scope

Included:

- Knowing exactly which draws are the Bandai Namco page's (the movie's identity, not a luminance heuristic).
- How its white background, its logo and its fades are made (draws, fills, images, colour transforms).
- The native change that puts that page on black, and a short witness run.

Excluded:

- Any Elden Ring file or the ER Decompilation folder; any asset made outside DS3's own draws.
- The other start pages (FromSoftware, the title: R2's `title_fx`/`BuildTitleScreen`; the precompile screen,
  `BuildPrecompileScreen` in `16b_rekindled_overlay.inc`).
- An options row or a flag (Thomas, 02:53).

A rule for one asset: D-0161 writes native rules per family or engine mechanism, never per asset; this page is named by Thomas's
D-0182 (the Bandai Namco screen on black), so its rule is that decision's, keyed to the movie's name, and touches no other page.

## Thomas's decision of 2026-11-12 05:40 (relayed by the coordinator: his decision card, his explicit choice)

The Bandai Namco logo as Elden Ring shows it (the 2022 logo, on black), taken from Thomas's own Elden Ring. An explicit
exception, for this logo only and on his PC only, to « no Elden Ring file » (the coordinator's brief) and to « no asset made
outside DS3's own draws » (the scope below): a local development script (`tools/Extract-EldenRingLogo.ps1`), run once on his
PC, reads his Elden Ring read-only and writes `er-bne-logo.dds` beside the module; nothing of Elden Ring goes into Git, the
cloud or an installer (D-0267); the module draws it when the file is there, otherwise DS3's logo made for black, which is also
the public version. Always on, no option. Recorded in `decisions/LOG.md` as D-0275.

## Inputs

- `05_901_Logo_BNE` is advanced from the start to about 3.5 s, alone among the logos, then `05_900_Logo_FromSoft`
  (OBSERVED, `evidence/I-0066/artifacts/r12e-film7/startup.jsonl`, the advance hook E53580's rows).
- The original page: white background, an orange, yellow and red blob with « BANDAI NAMCO » in white, « Entertainment » in red
  (OBSERVED, `evidence/I-0066/artifacts/i66-reference-startup-05/startup-02.png`).
- The logos are Scaleform pages (STATIC, I-0069 BINK-MOVIES-NATIVE §0.2); `data1/menu/05_901_logo_bne.gfx` is 3,072 bytes,
  stored (STATIC, archive index `evidence/I-0069/queries/bink/archive-index-movie-matches.tsv`).
- The native start frame is cleared black before the UI is drawn (`SceneRenderer::RenderStartupFrame`, scene_pass.cpp); the
  logos have no world frame.
- `GameUiSession::PrepareFrame` displays each movie through E52EE0 one at a time (STATIC, our code); Scaleform's `Display`
  calls `beginDisplay` directly (STATIC, I-0067 MENUS-MAP, `FUN_141BDA3A0`): a display begun inside that call is that movie's
  (INFERRED; the run's `native_ui_dark_logo_page` rows check it). A display deferred to `EndCpuFrame` is left unmarked.

## Hypotheses

- H1: the white background is a draw of its own (a solid fill over the page) and the logo an image transparent around the
  blob: leaving out (or blackening) that draw on this page alone is enough; the fades stay fades to black.
- H2: the logo image carries an opaque white background: the image itself has to be made dark (for instance a mask of the
  white connected to its border, computed once on the imported image), Thomas's coordinator consulted first.

## Expected evidence

The page's own structure, read statically in its `.gfx` (DS3's file, read-only, outside Git) and its image, then the page's
draws as the native records them (`native_ui_dark_logo_draw` rows, the first two frames and the first 1.5 s or more after the first) in a one-minute run.

## Exit conditions

- Success: in a native start the Bandai Namco page is black around a logo in its colours with white letters, its fades to
  black, the next pages unchanged (stills, measured, not by eye).
- Refuted: neither H1 nor H2 holds; the draws are described and the next way is put to Thomas.
- Insufficient information: the static reading and one run do not tell the background from the logo.
- Stagnation: no new discriminating evidence after two cycles.
- Budget exhausted: close the tranche and replan under `.project/WORKFLOW.md`.

## Safety and human interaction

One short announced live run (the start logos, about a minute; Thomas's saves restored by hash; never Alpha), only in a slot
the coordinator grants; no build or clangd while another thread measures.
