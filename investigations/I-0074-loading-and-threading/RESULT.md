# I-0074 result: what held the engine thread at arrivals, and the render worker's path, after Elden Ring

Complete (2026-10-20). Runs `evidence/I-0066/artifacts/i74-native-NN-*` (measure mode, no debug layer unless named
`debug`), builds `evidence/I-0074/artifacts/i74{a..i}-build`, series runner [Run-I74Series.ps1](Run-I74Series.ps1) (the
I-0070/I-0071 saves and key paths), ER reference I-0073 `ER-STUDY.md` (branch
`claude/i0073-er-loading-copy`, merged into main on 2026-10-21: [ER-STUDY](../I-0073-er-loading-copy-recording/ER-STUDY.md)).

## Conclusion

1. **The textures were never decoded on the engine thread** (OBSERVED: `texture_decodes_owner` 0 in every I-0070/I-0071 run).
   The engine thread waited for them: DS3's TPF creator 140D4E100 posts one fragment per texture and joins them blocking
   (I-0073 4.1; ER polls instead, 14023B5A0). In the slow frames of the Firelink walk the engine thread polls the task
   system's locks (1423C1DC0 -> 141799430/1417994A0) while the five loader threads run `AdmitEncodedTextureFamily`, 80 % of
   them allocating, zeroing, copying and freeing the whole TPF container for each texture (6 MB on average, 15.6 GB a walk;
   i74-native-02, `sample-profile-frame3764-report.txt`). Now the container is read in place (its extent checked committed
   and readable), the entry validated as before and its image moved into the record: texture admissions 11.5 s -> 3.2 s a
   Firelink walk (longest 79 -> 7 ms), 11.4 -> 4.7 s at Archdragon Peak (i74-native-03..06).
2. **The global descriptor heap was full at Archdragon Peak** (64 chunks of 8,192 in use, about 188 draws a frame on per-draw
   tables, two shader-visible heaps created a frame, i74-native-07). Raised to the bound every tier allows (122 chunks), and
   its returned ranges now merge with their neighbours (the arrival filled the larger heap once and the unmerged small free
   ranges kept about 50 draws a frame on per-draw tables, i74-native-17); sampler tables take only their bound extent. Per-draw
   tables at Archdragon Peak: 188 -> 0 a frame (i74-native-20).
3. **Point 3c, the recording in chunks (ER 141B3E890 / 141B3D940):** the G-buffer draws are split into near-equal chunks of at
   least 128 draws (at most 16) recorded on a recording pool (three workers beside the render worker) into lists of the frame
   slot, each chunk with its own bind state and constant uploads (64 KB arena blocks), submitted after the list so far and
   before the list that continues the frame, in chunk order, in one ExecuteCommandLists (ER's ordered submission). The
   admission pool and the recording pool are one `WorkPool` class (`renderer/work_pool.h`).
4. Smaller repeated work removed on the way: the glare programmes' `.rdata` scan and hashing once per technique set (about
   20 ms at an arrival), the frame slot holding every draw holder (the slot now holds the frame input), the instance blocks
   looked up at each prepare (now with their pass row).

| Scene (window) | Before (i74a/i74c) | After | Runs |
|---|---|---|---|
| Firelink walk, frames over 34 ms / 25 ms | 44 / 98 | 25 / 75 | 03 / 11 |
| m45 record ms (A/B in one build, then the arena blocks) | 4.61 (no chunks), 3.51 (chunks) | 1.97 | 14, 13 / 22 |
| m45 prepare ms | 5.17 | 4.55 | 10 / 22 |
| Archdragon Peak (windows 66-100): frames/s, prepare, record | 49.3, 11.0, 8.0 | 59.1, 8.8, 4.2 | 08 / 23 |
| Archdragon Peak, draws on per-draw tables a frame | 188 | 0 | 08 / 20, 23 |

D3D12 validation on with the chunked recording: 0 messages over 2,756 + 2,708 + 2,750 m45 frames (i74-native-12, 16, 19).
Guards on the final build (i74-native-24, Firelink walk, `record-revalidation` and the prepare shadow): 198,917 record
comparisons, 20,246 kept-draw shadow checks and 64 parallel pass-row checks, 0 mismatches. Pictures: m45 against the I-0071
merged build (i70-native-54) 0.98-1.01 per region; Archdragon Peak arrival and stay shots within the run-to-run spread of
the sky and fog (the ground rows 0.99-1.02; the upper rows vary 0.83-1.14 between two builds before the changes too).
Profiles: `sample-profile-*report.txt` in runs 02 (Firelink, slow frames and frame 3764), 07 (the Archdragon Peak arrival),
18 and 25 (m45 render worker); the raw samples were removed after their reports (D-0142).

Pre-merge review (Opus 5.5, D-0176), applied in build `i74j-build` (checked by runs 26-27: 4,756 m45 frames under validation, 0 message; 198,406 record comparisons, 20,146 kept-draw checks and 64 pass-row checks, 0 mismatches): no High finding.
- Medium, M1: the frame ring's quarantine (unproved completion) now leaks the slots' extra lists and allocators too, as it
  does the slot list (they could be in flight).
- Low, applied: the recording-chunks switch is atomic (L2); every pass row is made with its instance blocks, compared by the
  parallel pass-row guard, and a row without them is refused (L3); the slot no longer holds the lighting twice (L5); the
  unused `DecodeCpuTextureContainerEntry` removed (L5); the glare index is retried after a failed read (L6); a refused chunk
  close names the debug layer's message (L7); a descriptor range returned twice or overlapping is refused loudly (L8, the
  suggested unit test of the free list not written).
- Low, recorded: the in-place TPF read relies on the engine holding the container through the fragment (DS3's creator
  140D4E100 joins its fragments under its lock, I-0073 4.1) and checks the extent once, without the former guarded copy (L1);
  the chunks' payload tables do not share copies across chunks (L4: the `blocks` counter shows the copies, the recording time
  fell); the old per-draw holder copy stays for `RenderNativeModels` (L5); on a twelve-thread machine the two pools and the
  engine and render threads take every hardware thread before the game's own (L9).

## Uncertainty / open

- One run per scene and build; Archdragon Peak varies from run to run (its arrival and its sky). The recording pool's three
  workers overlap the next frame's collect on the admission pool; not measured on a machine with fewer than twelve threads
  (the pool is then empty and the recording stays on one thread).

- The render worker's prepare (8.8 ms at Archdragon Peak, 4.6 ms at m45) is the longer worker path for 120 frames/s: prepare in
  chunks (ER prepares each draw in the worker that records it) is the next step, with the shadow casters' recording.
- The arrival frame after a fast travel (collect 200-250 ms: every model admitted afresh) and the harness's own screenshot
  frames (WM_PRINT through the pumped wait, 130-150 ms: not game hitches) remain.
