# I-0096 - Mémoire et VRAM

Capability: C3

## Question

After a zone visit the native backend keeps about 1.1-1.2 GB more private memory than the original at the title and about
0.8 GB more in the zone (I-0092 RESULT, `i92-mem-original` against `i92-mem8-main`). What does the native backend hold that the
original does not, at the title after a zone's unload and in the zone, in system memory and in video memory, and how does it
come back to the original's behaviour without pop-in (Thomas: delay an appearance rather than show a neutral texture, D-0197)
and without new stutter? Thomas wants a clean base before DLSS and ray tracing.

## Scope

Included:

- The process's private bytes and its video memory (local and non-local), original against native, at the title before any
  load, in m31, and at the title after each visit; the same counters from outside for both renderers.
- What the native backend holds per category: the placed heap pools (live and free), committed resources, staging chunks, the
  texture records' CPU images, model and material registries, caches.
- Fixes in `native/` for what the native backend keeps beyond the original, after Elden Ring's mechanism (D-0195) without its
  known defects.

Excluded:

- DS3's own allocations (the engine's heaps, identical in both renderers).
- The stutter workstream (I-0095's findings on the held-frames counter stay there), the exit heap fault.
- DLSS, ray tracing, texture upscaling.

## Inputs

- Claims: K-0462 (I-0079: placed pools and staging chunks), K-0466 (I-0084: the budget read and acted on, D-0203), K-0467 and
  K-0468 (I-0085, I-0087: leaks fixed), I-0092 RESULT (memory at the title against the original).
- Runs: `i92-mem-original` (original, 4 visits of m31), `i92-mem8-main` (native `main`, 8 visits), the ballast runs
  `a84-fix1-vram4g` and `a84-fix1-vram1500` (I-0084).
- ER: [I-0079 ER-STUDY](../I-0079-motion-stutter/ER-STUDY.md) sections 1-4 (ER's placed pools: 32-256 MB heaps, first fit,
  one empty heap kept per list, no budget use) and [I-0080 ER-STUDY](../I-0080-er-dx12-pitfalls/ER-STUDY.md) section 4.
- Tools: `investigations/I-0085-night-endurance/Run-Endurance.ps1` (`-Original`, `-OnlyZones`, `-Visits`), the native
  journal's `native_present_pacing`, `native_streamed_memory` and `native_model_registry` rows, PIX and Nsight Systems
  (installed by Thomas on 2026-10-23).

## Observations before the first run (from existing evidence)

- O1 (a84 ballast runs): an 11,174 MB committed DEFAULT ballast raises the native process's private bytes from about 6,050 to
  17,240 MB: on this PC (NVIDIA, WDDM) video memory allocations are charged to the process's private bytes. Private bytes
  therefore mix system and video memory, for both renderers.
- O2 (`i92-mem8-main`): the native video memory (local usage) is 545-575 MB at the title before any load, 1,650-1,715 MB in m31,
  and 1,556 then 1,619 MB at the title after each visit: the zone's video memory is not given back at the title.
- O3 (same run): at the title after a visit the placed pools hold 21 heaps (about 1.3 GB) for 593-595 MB of live placed
  resources (354-390 MB before the first load); a heap is released only when every range in it is free, and the zone's
  survivors pin most of them.
- O4: every texture record keeps its whole CPU image (`NativeTextureRecord::content`, every mip's bytes) for the record's life,
  after its upload; 2,612 records in m31.

## Hypotheses

- H1 (title): the extra private memory at the title is the placed pools' free but pinned heaps (video memory, charged to the
  private bytes, O1) plus the live placed bytes that outlive the zone.
- H2 (zone): the extra private memory in the zone is mostly the texture records' CPU images (O4), which the original's
  D3D11 path does not keep.
- H3: the native backend's video memory in the zone exceeds the original's (targets, pools, staging).

## Expected evidence

- The same external counters for both renderers: private bytes and the GPU process memory counters (dedicated, shared, local,
  non-local, total committed) sampled every 5 s by the endurance runner, so system memory = private − GPU committed.
- The native journal: per pool heaps, live and free bytes, fully free heaps; the records' CPU image bytes; committed
  resource bytes.
- A fix is credited when the title after a visit and the zone come within the original's figures (or the remaining gap is
  named and explained), with the pictures unchanged, no pop-in (no texture shown neutral), the replayed path gate passing.

## Exit conditions

- Success: the native title after a visit and the zone at or below the original's private memory and video memory, or every
  remaining megabyte named with its owner and a reason to keep it; gate PASS; merged.
- Refuted: H1-H3 wrong; the new owner named from the measurements.
- Insufficient information: the counters cannot separate the categories; record it and name the missing measurement.
- Stagnation: no new discriminating evidence after two cycles.
- Budget exhausted: close the tranche and replan under `.project/WORKFLOW.md`.

## Safety and human interaction

Live runs under D-0116/D-0117: announced in the thread at least 5 minutes before with the PC clock, offline, the Shebang zone
save installed as a test save (Alpha untouched), every protected file restored by hash. The original run is the bare original
(D3D11, no module). Nothing installed in the game folder. If Thomas says « je joue », everything on the PC stops until « fini ».
