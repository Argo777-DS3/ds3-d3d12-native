# I-0138 tranche 5: HDR10, a first sheet (static)

2026-11-14, written by the R pilot's Opus 5.5 thread after tranche 4's merge (`d365e26f`), for the Fable 5.2 thread that
builds tranches 5 and 6 (D-0276's entry; the coordinator, 02:32: « prépare la tranche 5 en lecture et en plan, sans code »).
Read-only, nothing built or launched. Sources: [PLAN](PLAN.md) §4 to §7 and Thomas's answers there, ROADMAP workstream 11
item 5, [ER-TONEMAP-HDR](ER-TONEMAP-HDR.md), [STUDY](STUDY.md) §5, a code map of the native output chain from our tone map
to `Present` (a Sonnet reader, 2026-11-14, its claims spot-checked below with file and line), a licence survey of AgX and
ACES (a web reader, 2026-11-14, §6). Labels: **verified** (read in the code or a primary source), **proposed**, **open**.

## 0. Answer first

1. **Nothing in the native output is HDR today** (verified): from the tone map to the back buffer every target is
   `R8G8B8A8_UNORM` (`DisplayStage::Format()`, `native/renderer/display_stage.h:56`), the swap chain too
   (`backend.cpp:897`, its resize `:935`, the mode list `scene_pass.cpp:553`, the frame generation's rebuild `:621`), and no
   `SetColorSpace1`, `CheckColorSpaceSupport`, `IDXGIOutput6` or `SetHDRMetaData` exists in `native/`. The gamma is applied
   inside our tone-map pass (`post_tone.hlsl`, the `exp2(log2(c) x inv_gamma)` line), then the game's own brightness pass
   (its pinned pixel programme, a 1025-entry curve clamped to [0, 1]) writes the back buffer.
2. **Elden Ring gives no model for the output** (verified, ER-TONEMAP-HDR §4): no DXGI colour-space path on PC, only a
   vendor-API HDR in exclusive full screen whose shaders are not readable. What ER does give: the UI drawn into its own
   texture and blended last (`_BlendUI_SDR2HDR` and its siblings), and the curve and bloom before the output encode. The
   output itself is ours to design (D3D12 and DXGI 1.6, connaissance générale; the design below).
3. **Proposed: the SDR path stays byte for byte what it is** (tranche 1's oracle: the original's captured draws, 0/255),
   and HDR10 is a second output path chosen at the swap chain: a float scene-referred-to-display path, the 10-bit PQ
   encode with a blue-noise dither, the UI composed last at paper white. Five steps, each mergeable (§4).
4. **Decisions that are Thomas's** (§5): the colorimetry's highlights in HDR (how the original curve extends past 1),
   the brightness slider's meaning in HDR, the default peak and paper white, and the AgX licence question for tranche 6.
5. **Practical**: the live checks need the OLED on, Windows HDR switched on by Thomas (his yes, 2026-11-12 11:55) and
   Thomas in front of it; a still of an HDR frame cannot be judged on an SDR sheet (§3.10).

## 1. The output chain today (verified unless said)

In the order of a frame (`native/renderer/scene_pass_frame.cpp`):

| # | Pass | Target and format | Where |
|---|---|---|---|
| 1 | DLSS (with RR) or our TAA, on the lit HDR | `R16G16B16A16_FLOAT`; DLSS with `colorBuffersHDR` | `:1041-1137`, `streamline_host.cpp:180-314` |
| 2 | Depth of field, Alpha_R, motion blur, automatic exposure, glare (bloom and shafts) | float; the glare `R11G11B10_FLOAT` | `:1142-1263` |
| 3 | **Our tone map** (`ps` or `ps_reinhard`): exposure, vignette, curve, glare screen-blended, colour matrix, **gamma and the 4x4 Bayer dither** | the display intermediate, `R8G8B8A8_UNORM` | `:1252`, `post_tone.hlsl` |
| 4 | The grading LUT's draw (two draws: the LUT reads a true 8-bit image, tranche 1) | 8-bit | `post_tone_stage.cpp:94-182` |
| 5 | AA filter (without DLSS), NIS sharpening (`hdrMode=eNone`), the emissive glow | 8-bit | `:1321-1371`, `streamline_host.cpp:388` |
| 6 | Frame generation's HUD-less copy: the display pass into `Hudless()` | 8-bit, tagged `kBufferTypeHUDLessColor`; **no UI colour-and-alpha tag** | `:1381-1394`, `streamline_host.cpp:456-485` |
| 7 | **The game's UI** (the primitive pass), **then the native overlay** (the Rekindled pages, the precompile page) | into the same 8-bit intermediate | `:1395-1404`, `primitive_pass.cpp:256`, `overlay_pass.cpp` |
| 8 | **The game's brightness pass**: its pinned programme, the game's option as a curve (`pow(x, src/dst)`, clamped) | the back buffer, `R8G8B8A8_UNORM` | `:1405-1416`, `display_stage.cpp`, `display_curve.cpp:10`, `17_render_step.inc:553-557` |
| 9 | `Present` (no `Present1`), Reflex markers | | `scene_pass.cpp:171-216` |

Clamps to [0, 1] before the back buffer: the tone map's three `saturate`, every 8-bit target, the curve's entries. The
factory is `IDXGIFactory6`, the chain `IDXGISwapChain3`, created through Streamline's proxy factory when DLSS is ready
(`backend.cpp:908-916`). Full screen, borderless and the frame generation's switch rebuild or resize the chain
(`scene_pass.cpp:535-629`). Tests that assume 8 bits: `frame_presenter_test`, `display_stage_test`,
`display_curve_test`, `post_tone_stage_test`, `frame_graph_test`; the witness readback (`:1419-1428`).

## 2. What the tranche delivers (ROADMAP item 5, PLAN §5, Thomas's answers)

- The float output with a **blue-noise dither** (Thomas, 2026-10-30).
- The **10-bit PQ output** (HDR10).
- Two settings: **maximum luminance** and **paper white**.
- **The UI in HDR**.
- **The emissive glow before the tone map** (PLAN §7 point 4, Thomas's yes 2026-11-12 12:19).
- On the OLED, with Thomas (PLAN §7, his yes to Windows HDR).

## 3. Design (proposed unless said)

1. **The output format: HDR10**, `R10G10B10A2_UNORM` with `DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020`, the encode in our
   last pass (Rec. 709 to Rec. 2020 primaries, the ST 2084 curve, the dither), rather than scRGB (`R16G16B16A16_FLOAT`,
   `G10_NONE_P709`, the compositor encodes). HDR10 is what the ROADMAP names, half scRGB's bandwidth, and keeps the
   encode and its dither ours. **Open**: whether DLSS frame generation accepts a 10-bit PQ back buffer and HUD-less
   image (Streamline's DLSS-G documentation, to read before step 3; connaissance générale: it supports HDR swap chains).
2. **The probe**: `IDXGIOutput6::GetDesc1` on the window's output: `ColorSpace` (Windows HDR on or off), `MaxLuminance`,
   `MaxFullFrameLuminance`, re-read on a move to another screen and on `WM_DISPLAYCHANGE`; `CheckColorSpaceSupport`
   before `SetColorSpace1`. HDR is offered only where Windows HDR is on for that screen: no switching of the display mode
   by the game (ER's vendor-API path is not copied: it is one of « ER's known defects », exclusive full screen only).
3. **Every chain (re)creation applies the colour space**: creation, `ResizeBuffers`, full screen and borderless,
   the frame generation's rebuild (`scene_pass.cpp:578-629`), and the stand-in buffers' format (`backend.cpp:958-973`,
   hard-coded RGBA8 today) follows the chain's. `SetHDRMetaData` with the peak and paper white set (and the content's
   MaxCLL left 0 unless measured).
4. **Where SDR and HDR part**: after the glare and before our tone map. SDR keeps steps 3 to 8 of §1 exactly. HDR draws a
   third tone-map entry, `ps_hdr`, into an `R16G16B16A16_FLOAT` target: the same exposure, vignette, glare and colour
   matrix, a curve that keeps the SDR curve's mid-tones and lets the highlights rise to the peak (§5 decision 1), no
   gamma, no 8-bit dither. The `saturate`s stay in the SDR entries; the HDR entry clamps only to [0, peak / paper white].
5. **The grading LUT in HDR** (**open**, a design question for the tranche's head): the original LUT is built for
   display-encoded [0, 1] values. Two candidates: apply it to the SDR-range part (the curve's output below 1, encoded as
   SDR would be, then decoded) and carry the highlights' excess unchanged in hue; or apply it in a shaper space over the
   whole range (log or PQ), which bends the highlights the LUT never saw. Proposed: the first, with sheets.
6. **The emissive glow before the tone map**: in HDR only, onto the float HDR before `ps_hdr` (ER's order); SDR keeps the
   original's order (glow on the 8-bit image after the tone map), so the SDR picture does not change (tranche 1's rule).
   To show Thomas on a sheet as PLAN §7 point 4 asks: the glow before the curve in HDR against the original's order.
7. **The UI in HDR** (ER's model): the game's UI and our overlay draw into their own `R8G8B8A8_UNORM` target with alpha
   (cleared transparent), in SDR and in HDR alike; the last pass blends it over the scene: in SDR into the 8-bit image as
   today (byte for byte, to check: blending into a cleared target then compositing must equal blending in place, or SDR
   keeps the in-place path), in HDR decoded to linear and scaled to paper white before the PQ encode. A gain in both
   modes: the UI target is the frame generation's `kBufferTypeUIColorAndAlpha` input, which it does not have today
   (§1 row 6), so generated frames stop smearing the HUD.
8. **The game's brightness option in HDR**: the game's brightness pass is an SDR curve on display-encoded values; in HDR
   it does not run. Proposed: in HDR the game's slider moves paper white (§5 decision 2).
9. **The blue-noise dither**: a 64x64 or 128x128 tileable blue-noise texture, generated by us (void and cluster, Ulichney
   1993, a short offline script; no third-party texture, so no licence question), applied in the 10-bit encode at one
   10-bit step, offset per frame (temporally animated only if it does not shimmer under DLSS: to judge live).
10. **Witnesses and Thomas's eye**: the live check is on the OLED, Thomas looking (PLAN §5). Our stills cannot show HDR on
    an SDR sheet; the witness readback (`scene_pass_frame.cpp:1419-1428`, 4 bytes a pixel today) gets a float or 10-bit
    form for the numbers (the paper white's code values, the peak reached, no NaN), and NVIDIA's own HDR clips or Windows'
    HDR screenshot (`.jxr`) for Thomas to look at later. The SDR path keeps its byte-for-byte tests unchanged.
11. **NIS** in HDR (**open**): it runs on the 8-bit post-tone image; in HDR it would run on the float image before the
    encode with Streamline's HDR mode, or stay off in HDR at first. Proposed: off in HDR in step 3, revisited after.
12. **Settings** (the path of an existing row: `rekindled_settings.h`, `graphics_rows.h`, `02b_rekindled_graphics.inc`, the
    launcher's migration): « HDR output » (Off / On; shown only where Windows HDR is on for the screen), « Peak
    brightness » (nits, defaulted from `MaxLuminance`), « Paper white » (nits). The Advanced page's 14-slot film scrolls
    already (I-0132 R12g). PLAN's tranche 7 lists « sortie HDR » among the rows: the rows land here, their final layout
    there.

## 4. Steps in order (proposed; each merges alone with its reviews)

1. **The probe and the swap chain in HDR10** (S): `IDXGIOutput6` probe, the format and colour space at every
   (re)creation, the stand-ins, `SetHDRMetaData`, a test flag `hdr-output.flag` until the setting row; the output pass a
   plain PQ encode of the existing SDR image at paper white (an SDR picture inside an HDR signal: nothing looks
   different, which proves the chain). Tests: `frame_presenter_test` in both formats.
2. **The UI's own target** (M), in SDR first: the UI and the overlay into their target, blended last; SDR byte for byte
   (the captures' tests and a witness pair); the frame generation's UI tag. Then the same blend into the HDR output.
3. **`ps_hdr` and the float path** (L): the HDR tone-map entry (§3.4), the LUT's HDR form (§3.5), the glow before the
   curve (§3.6), the PQ encode with the blue-noise dither (§3.9); NIS off in HDR. Sheets of the curve's extension
   candidates for decision 1 (plots of the curve, and live on the OLED).
4. **The settings rows** (S): HDR output, peak, paper white; the game's brightness slider on paper white in HDR.
5. **Thomas on the OLED** (the witness): the zones' tour views in SDR and HDR, the cutscenes captured in tranche 4, the
   menus; his choices for §5.

## 5. Decisions that are Thomas's (to put to him at the tranche's head, with sheets)

1. **The original colorimetry's highlights in HDR** (PLAN §4: « mêmes tons moyens et mêmes couleurs, avec des hautes
   lumières qui montent au lieu d'écrêter »): the original sensitometric curve reaches 1 near its shoulder; in HDR either
   (a) the same curve to a knee (about 0.7 to 0.8 of paper white), then a smooth shoulder to the peak, or (b) the curve
   rescaled so that 1 maps to the peak and the mid-tones re-pinned at paper white. Proposed: (a), with plots and the OLED.
2. **The brightness slider in HDR**: it moves paper white (proposed), or it is hidden in HDR.
3. **The defaults**: peak from the screen's `MaxLuminance`; paper white 200 nits (connaissance générale: the usual default
   for games is 200 to 250).
4. **Tranche 6's AgX** (§6): no licence was found for AgX's original and its best-known approximation.

## 6. Licences for tranche 6's candidates (the web reader's survey, 2026-11-14; not a legal opinion)

| Candidate | What was found (URL read) | What it allows |
|---|---|---|
| ACES reference (aces-core, ACES 2.0 aces-output) | Apache-2.0 (`github.com/ampas/aces-dev` LICENSE, served as aces-core; `github.com/aces-aswf/aces-output`) | use and adaptation, with the licence and notices kept; no use of the ACES trademark |
| ACES fit, Narkowicz | « free to use under public domain CC0 or MIT » (`knarkowicz.wordpress.com/2016/01/06/aces-filmic-tone-mapping-curve/`) | free (CC0) |
| ACES fit, Hill (BakingLab `ACES.hlsl`) | « All code licensed under the MIT license », credit to Stephen Hill (`github.com/TheRealMJP/BakingLab`) | MIT notice and credit |
| AgX, Sobotka's original | **no licence file** in `github.com/sobotka/AgX` | no grant visible |
| AgX « minimal » (iolite) | no licence, « All rights reserved » on the page (`iolite-engine.com/blog_posts/minimal_agx_implementation`) | no grant visible |
| AgX in three.js | MIT (the repository's LICENSE; the file cites iolite and Filament PR #7236) | three.js's code under MIT, its upstream constants' terms unknown |
| AgX in Filament, Blender | **not read** (Filament is Apache-2.0 as a project and Blender GPL, connaissance générale) | to read before tranche 6 |
| PQ (SMPTE ST 2084), Rec. 2020 | not read from a primary source | the published maths and constants implemented in our code: connaissance générale, normally free; to confirm |

So: ACES is usable (the reference under Apache-2.0, the fits under CC0 or MIT); AgX needs a licensed source (Filament's
Apache-2.0 implementation is the likely one, to read) or Thomas's decision to rely on the published constants. The game is
Thomas's private project, not distributed (D-0274: the GitHub copy light and private); the notices are kept in the source
either way.

## 7. Open before code

- Streamline DLSS-G with an HDR10 chain and its HUD-less and UI inputs' formats (§3.1, §3.7).
- The LUT's HDR form (§3.5) and NIS in HDR (§3.11).
- Whether blending the UI into its own target then compositing reproduces today's in-place blending byte for byte (§3.7):
  the game's UI blend states are many; if not, SDR keeps the in-place path and only HDR uses the UI target.
- The cost: one more float target and pass in HDR; nothing in SDR.
