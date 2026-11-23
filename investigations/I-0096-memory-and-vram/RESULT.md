# I-0096 result: memory and VRAM

## Conclusion

The native backend now behaves like the original for memory and video memory over zone visits (Beta m31 loaded from the
title four times, medians per stay; counters read from outside the process, the same for both renderers):

| | Original (`i96-01`) | Native before (`i96-02`) | Native after (`i96-04`) |
|---|---|---|---|
| Title before any load: private / VRAM (dedicated) | 5,886 / 465 MB | 5,831 / 547 MB | 5,518 / 543 MB |
| Zone: private / VRAM | 7,235-7,564 / 1,634-1,877 MB | 8,044-8,284 / 1,653-1,657 MB | 7,244-7,468 / 1,653-1,657 MB |
| Title after a visit: private / VRAM | 6,400-6,547 / 904-997 MB | 7,329-7,619 / 1,563-1,625 MB | 6,226-6,358 / 984-985 MB |
| Title after a visit: system memory (private − GPU committed) | 5,394-5,426 MB | 5,531-5,747 MB | 5,006-5,138 MB |
| Zone: system memory | 5,477-5,537 MB | 6,155-6,376 MB | 5,353-5,576 MB |

Two owners explained the whole gap (H1 and H2 of the contract, both supported; H3 refuted: in the zone the native video memory
equals the original's):

1. **The texture records' CPU images (zone +0.8 GB, title +0.3 GB).** Every admitted texture kept its whole decoded image
   (`NativeTextureRecord::content`, every mip's bytes) for its life after its upload: 797 MB in m31, 304 MB at the title. Nothing
   read those bytes after the upload (the UI import binds an admitted texture as is). A record now keeps the image's description
   only (name, extent, format, per-subresource shape, float metadata); the bytes are staged by the upload and freed with the
   admission, as D3D11 frees a created texture's initial data. The menu offscreen targets' zero images (up to 64 MB each, never
   read) went the same way.
2. **The last zone's kept draws at the title (VRAM +0.65 GB).** The kept draws (I-0070/I-0090, the prepared draws carried
   between frames) were reset only at the next main-view prepare; at the title nothing is prepared, so they held 180 of the zone's
   textures (172 MB, `m50_00_*` distant-view and sky textures among them) and 29 of its geometry buffers (26 MB), which pinned the
   zone's 64 MB placed heaps: 1,280 MB of heaps for 595 MB live. A pending reset is now applied from the frame's owner thread when
   no world is drawn (a busy prepare is left to the next window, a reset already applied is not applied again), and with two
   windows in a row without a world every spare empty heap is released at once (ER's trim releases them every call, I-0079
   ER-STUDY 2); in the world, still one a window.

Not changed: the FLVER storages' CPU bytes (344 MB in m31, 43 MB at the title) are read later by the model streams and the CPU
geometry join; with them the native system memory is at the original's in the zone, so they stay (a lead if memory must go below
the original). Pipelines stay kept (no recompilation in play, I-0072).

ER (D-0195): ER's placed pools (32-256 MB heaps, first fit) keep one empty heap a list and release the others each call, never
look at the budget after start, and have the same pinning exposure (a long-lived resource placed among a zone's keeps its heap);
the native pools keep ER's shape with 64/32 MB heaps and best fit, release empty heaps as ER does when no world is drawn, and the
kept-draw release removes the pinning the native backend added itself.

## Observations

- O1: video memory allocations are charged to the process's private bytes on this PC (an 11,174 MB ballast took a native run
  from about 6,050 to 17,240 MB, I-0084's `a84-fix1-vram4g`): private bytes compare system plus video memory, so both are read
  apart now (`gpu_committed_mb`, `system_mb`).
- O2: the original keeps about 450 MB of video memory and 120 MB of system memory after its first visit too (title 465 to 904-997 MB
  VRAM); the native backend after the fixes keeps the same video memory (543 to 984 MB) and less system memory.
- O3: no fault in any run; the exit heap fault 0xC0000374 at the close (the original's own, I-0092) dumped in `i96-02` and
  `i96-03`, not in `i96-04`.
- O4: `i96-04` missed its first load once (the menus' click; retried by the tour, no fault), so its zone and title stays are
  numbered 2-5.

## Tools

`Run-Endurance.ps1` samples the GPU process memory counters with the private bytes (one `Get-Counter` call, about every 6.5 s);
`investigations/I-0096-memory-and-vram/Get-I96MemorySummary.ps1 -Run <runs>` gives the medians per stay. The native journal's
`native_streamed_memory` row has each pool's heap, live and empty-heap figures; `native_model_registry` the CPU image bytes, the
withdrawn records and destroyed storages still held (with the largest names), the FLVER CPU bytes and the process heap.

## Reviews

Step review (Opus, commit 24eb63c3): Medium 1, the owner-thread release could wait on the render worker's first prepare of a zone
and then discard its new draws: fixed (try_to_lock, recheck under the lock). Low 2, records and storages could be destroyed under
the census locks: fixed (collected, released after). Low 3, releasing every spare heap on a loading screen's edge: now after two
windows without a world. Low 4, diagnostic accuracy (empty heaps counted before the fence reclaim; noted). Low 5, the offscreen
zero images: removed. Final review: see below.

Final review (Opus, the branch against `main`): no blocking defect. Low-medium 1, the full heap trim also ran on long loading
screens (heap churn while the next zone takes heaps): now only after two windows without a world and without an admission. Low 2,
the summary's title stays opened at the quit's `left`, before the title was shown: they open at the title reached now, and the
tour's `-TitleSeconds` lengthens them (the four-visit runs above have about one robust title stay each, their last). Informational
3, the UI import refuses (never draws neutral) a description-only record it could not bind as is: unreachable today.

## Gate

- `i96-final` (10:20): inconclusive in fact, recorded FAIL: both measure runs stayed on « Press any button » (their pictures the
  title), validation 0 error, memory -1 MB a visit.
- `i96-final-b` (11:02): FAIL on the measures: game steps over 34 ms 16 against 6 (runs 17, 17, 14, 16), frames over 34 ms 25.5
  against 22, held 27.75 against 22; pictures 0.72-0.89, memory -5.6 MB a visit. A/B on the same path the same hour: `main`
  (`i96-ab-main`) 13.5 / 95.5 / 14 / game steps 3.5, the branch (`i96-ab-branch`) game steps 14-17. Cause: the census row's
  `HeapSummary` walked the process heap under its lock every 600 frames (22 of 23 slow game steps of `i96-ab-branch-m1` fall in the
  window right after that row); removed (commit cf61ce12). The branch without it (`i96-ab-g-m1`): game steps over 34 ms 0, frames
  over 34 ms 14, over 25 ms 70, held 14.
- `i96-final-c` (12:41): one measure run stayed on the title again (inconclusive), the other at `main`'s level, memory -6.5 MB.
- The title: nine launches that day stayed on « Press any button » (`main` 0 of 12, the branch 9 of 34). Not the game: the title
  pilot's network-notice test (a few orange pixels) answered true before the game's window was painted, on Steam's library left
  open behind it after the morning's restart (the DARK SOULS III cover lies under those pixels; `i96-pix-1..4`, no input, both
  builds hit within the first 1.5 s, nothing after), so the pilot stopped pressing; no 01_010_MessageBox movie in those runs. The
  branch's start-up timing met it more often. The pilot now needs the notice twice and goes back to pressing when the offline
  notice does not follow (`Pilot-TitleContinue.ps1`, `notice_retries=` in its last line).
- `i96-final-d` (14:25, the final tree, commit 8883d51d's `native/`): **PASS**. Frames over 34 ms at the frame end 12.5, over 25
  ms 83.5, held two refreshes 13.5, game steps over 34 ms 2; pictures 0.96 and 0.85; memory 7.7 MB a visit (Theil-Sen over the
  titles; threshold 40). Its eight-visit leg read with the GPU counters: title before any load 5,572 MB private / 547 MB VRAM, the
  zone 7,250-7,481 / 1,653-1,657 MB, the title after visits 6,274-6,288 / 920 MB (the original's 6,400-6,547 / 904-997 MB,
  `i96-01`). No fault in play; the exit heap fault 0xC0000374 at the close (the original's own, I-0092).

Not run: a second original run with longer title stays (`-TitleSeconds`); Thomas asked to resume the normal course after the
title episode, and the original's robust last title stay (`title-4`, 6 samples) agrees with its shorter ones. The gate's
thresholds were not recalibrated (D-0207 applies to performance merges; this one changes memory, and its measures are within
the thresholds).

## Evidence

Runs `evidence/I-0066/artifacts/i96-01-original`, `i96-02-native-main`, `i96-03-native-images`, `i96-04-native-kept` (each with
`memory.csv`, `tour.txt`, the native ones with `startup.jsonl`); builds `evidence/I-0096/artifacts/i96-build-b` (`main` with the
census), `i96-build-c` (images), `i96-build-d` (kept draws), `i96-build-e` (step review).

## Uncertainty

- One zone (m31), four visits a run, one run a configuration; the other witness zones are covered by the night endurance.
- The pictures: the gate's standstill picture; no visual change is intended (the records' bytes were never drawn from).

## Artifacts produced or modified

`native/frame/game_texture_ingress.{h,cpp}`, `native/frame/game_ui_images.cpp`, `native/renderer/backend.{h,cpp}`,
`native/frame/game_bootstrap/01_state.inc`, `02_frame_lifecycle.inc`, `13_model_frame.inc`, `14_scene_step_and_storage.inc`;
`investigations/I-0085-night-endurance/Run-Endurance.ps1`; `investigations/I-0096-memory-and-vram/`.
