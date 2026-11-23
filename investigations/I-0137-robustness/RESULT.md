# I-0137 « La robustesse » - result

## Conclusion

Complete (2026-11-13, the E pilot; ROADMAP workstream 4 closed, its left items below in « Closing »). Active from 2026-11-12 on its
steps 3 to 5, then 1, 2 and 5 again. Tranches 1 and 2 merged `8f415270` (I-0120's A1 and its
priority 4 but A2; none proved by a run: the cases of A3, B6, C2, C4, G5 and G6 were not found in the journals read, E3's was met
once, `i101-fs-alttab-1`, and has a unit test on a real swap chain). Tranche 3 merged `57b12d2e`: part of priority 5, and A2 in part
after a witness run (the movie frame's black stand-in removed; the menu proxy's kept until a witness at the Status page).
Tranche 4 (a fresh thread, `claude/sharp-tereshkova-5cbcb6`), merged `50b89d24` (2026-11-12 15:01); its launcher rebuild and start check were
not run on their own: the next merge, I-0132 R12h's third tranche (`9e45d637`, which contains it), rebuilt the launcher and passed
its start check `start-9e45d6370` (15:46-15:48). It holds the rest of priority 5 corrected or left with its reason, and
workstream 4's step 5 in part: of the exit faults read, two are DS3's own (I-0124's, fixed natively since `ac958d0c`; the
creation's close, guarded now) and one the module's (answered at its root: a stopped Close keeps its thread); a native F14C52 fault
after I-0124's fix, through another thread (g24i, 2026-11-11), was left open (attributed and stopped in tranche 6). A2 finished (the proxy's stand-in removed).
Tranche 5 (workstream 4's step 1 in part): a selector pair whose uniform layout the native does not take, and a display of offscreen
composition or with a colour clear, are skipped and counted instead of closing the game; the selectors without programmes, the
packets and filter passes the native refuses, the instanced layouts, the text layer kinds and the HAL's unqualified methods left
with their reasons; step 2 next. Tranche 5 merged `4f1f7c84` (2026-11-12 18:45); the launcher rebuilt from `d30dd7cd` (124 PASS lines,
`evidence/I-0137/launcher-build-d30dd7cd.log`), start check `start-d30dd7cd5` passed (19:17-19:18, the ASUS, 2,262 native frames,
restored by hash).
Tranche 6 (workstream 4's step 2, and step 5's F14C52 path): a census of 4,693 journals found none of step 2's stops met since
2026-11-06 (three in late October and November, since fixed), so step 2 converts only the plain cases of the game's data whose skip
has a defined end (a map piece's runaway child list in the main walk, a Bink frame out of contract) and keeps the rest with its
reason (an unreadable MTD among them: its FLVERs would fall to the engine default MTD, a stand-in); the F14C52 path is FromSoftware's online RPC thread
(`FdpSv::RPCSystemImpl`), stopped natively at the quit as I-0124 stopped the sound thread. Merged `c347b279` (2026-11-13 06:21);
the launcher rebuilt from it (`evidence/I-0137/launcher-build-c347b279.log`, every unit test passed), start check `start-c347b2792`
passed (06:33-06:34, the ASUS, restored by hash; its quit: `native_exit_sound_stop`, `native_exit_fdp_stop` and
`native_exit_network_stop` all « signalled »). The D126B0 witness then still faulted (tranche 7).
Tranche 7 (on the branch): the tour's fault at Friede's and the Abyss Watchers' second phase (a world frame with no UI packet while a
UI build is current) presented without the overlay and counted by cause, but for two of the module's own states (no UI session, the
render step not run) that still refuse and now name their cause; the tour's own cause is not known; the D126B0 guard keyed on the
module's own record of each modifier's block, marked released when DS3 frees it, on slots 4 to 7, every argument forwarded (a defect
of tranche 4's detour, two arguments for four, found and fixed). Merged `63b113d9` (2026-11-13 09:00); the launcher rebuilt from it
(`evidence/I-0137/launcher-build-63b113d9.log`, every unit test passed), start check `start-63b113d95` passed (09:09-09:11, the ASUS,
restored by hash; its quit's three exit stops « signalled »). The CharMake witness `i66-native-i137-t7-charmake` still faulted at the
creation's close, further on (« Tranche 7 », its witness): the second failed local fix of that DS3 fault, so an RCA is next.
Tranche 8 (on the branch): the RCA: at a window close DS3's quit request precedes the c0000 release while the creation's menu
models keep updating; natively three readers of the menus' models are gated on the quit request. Merged `5fa7c0bd` (2026-11-13
15:49); the launcher rebuilt from it (`evidence/I-0137/launcher-build-5fa7c0bd.log`, every unit test passed), start check
`start-5fa7c0bd5` passed (16:02-16:04, the ASUS, restored by hash). Both its witnesses closed the creation cleanly (exit 0, no fault):
the first form's (`p10b`) and the merged form's (`i66-native-i137-t8d-charmake`, `p10d`).

### Tranche 1: A1, the original executable known by what it is, not by its folder

- **The finding, re-read in today's code (as the review states it).** `game_bootstrap_ctl.cpp` compared the target process's
  image path with `C:/Program Files (x86)/Steam/steamapps/common/DARK SOULS III/Game/DarkSoulsIII.exe` through the throwing
  `std::filesystem::equivalent`: with the game in another Steam library the controller ended in `terminate` or returned 5, and
  the module was never injected. The two launchers (`tools/Launch-DS3-DX12.ps1`, `-Test.ps1`) held the same path for their
  SHA-256 check and stopped on « Version du jeu inattendue » there too.
- **The correction.** The controller keeps its purpose (it injects only into Dark Souls III 1.15.2.0) without any path:
  `original_image.h`'s `CheckOriginalImage` takes the process's image file and asks its name (`DarkSoulsIII.exe`, any case), its
  size (88,960,032 bytes) and three words of its PE header (TimeDateStamp 0x639C4DDD, SizeOfImage 0x056F4000, CheckSum
  0x054D6C82); a refusal returns 5 as before and now prints a row naming why (`not_named_darksoulsiii_exe`,
  `image_file_unreadable`, `not_the_1_15_2_executable`). The header, not a hash of the file: the controller injects as soon as
  the process exists and the file is 85 MB; the full SHA-256 stays the launcher's, before the game starts, and the build's.
  `Build-GameBootstrap.ps1` checks the header's four values against the executable it pins by SHA-256, so the pins cannot drift
  from it. The launchers find the game through Steam's own records (`tools/Find-DS3Game.ps1`: Steam's folder from the registry,
  `steamapps/libraryfolders.vdf`, the manifest of app 374320 and its `installdir`), then hash what they found.
- **Checked.** `original_image_test` (new, run by the build with the pinned executable's path): the pinned executable accepted
  where it is; the same head and size accepted in another folder (a space and an accented letter in its name, the file name in
  another case, forward slashes) and on a path past MAX_PATH (the extended-form branch); another program, a missing file, one
  byte more or less, a 16-byte and an empty file, each pinned header word changed in turn and a header offset outside the head,
  all refused; nothing left in the temporary folder (after a failure too, by its code: no failure was provoked).
  `Find-DS3Game.ps1 -SelfTest` (run by the build too): nothing without a manifest or an executable, the game found in a third
  library on a path with a space and an accent, past a library on a drive that is not there, nothing without a Steam folder; on
  this PC, run by hand, it returns the installed game. Builds: `evidence/I-0137/build-a1` (the first form, without T7 and the
  finder's self-test, 2026-11-12 08:48-09:00) and `build-p4a` (with both: its log's PASS lines), every unit test passed.
- **Not checked.** No PC with the game in a second Steam library was at hand: the controller's and the launchers' acceptance of
  another folder rest on the two tests above (made-up folders), not on a real launch there. The open of the image file while the
  game runs it (read access, shared for reading and deleting) is first shown by the start check after the merge. An image on a
  network path longer than MAX_PATH is refused (unreadable: it fails safe). A game installed outside Steam, or a Steam whose
  registry keys are absent, is not found by the launchers (their message says so); the installer of workstream 10 will ask the
  player in that case. A file of the same name, size and header words with other contents would pass the controller; the module
  does not compare the code when it installs its hooks (they go to fixed addresses), and its loaded-code pins refuse only at the
  first native scene update. This is not a loss: the path comparison never looked at the file's contents either, and the
  launcher's SHA-256 before the start is the real check.
- **Stated plainly (D-0268).** One check changed: the controller's identity test of its target, from a path comparison to the
  file's name, size and header words. It is stricter on what the file is and no longer looks at where it is. No test, tolerance
  or reader was weakened; one unit test added.

### Tranche 2: priority 4, re-read in today's code and corrected

What the runs show of these cases:

- In 17 journals (the night endurance `full-1112-night`, 94 minutes; the ten latest start checks; Thomas's six play sessions of
  2026-11-08 and 2026-11-11; counted by a `measure-tools` subagent, one count checked by hand; none holds a character creation)
  there is no `native_encoded_texture_failure`, `native_encoded_source_unowned`, `native_ui_asset_unavailable`,
  `native_ui_offscreen_placeholder` or `native_lighting_programmes_unresolved` row; the lighting set resolved each time with 24
  point and 4 spot programmes, `organize_bytes` 11132, the three SSS programmes and the shadowed sun (`found` 1); the only
  `native_object_skipped` reason is `nonfinite_placement` (four rows in each of 4 journals, none in the 13 others).
- **A2's case is met often** (the integrity review's H1, which read more journals than those 17):
  `native_ui_offscreen_placeholder`, the SYSTEX_BLACK placeholder, has 134 rows in at least 54 journals of
  `evidence/I-0066/artifacts`: every character-creation run, the credits, the colour select, the Load Game and Status runs
  (`i66-native-1317-charmake` and on, `i66-native-2065-credits`, `2091-colorselect`, `loadgame-1164`, `status-1163`, the
  tranche 35 face runs).
- **E3's case was met once**: `i101-fs-alttab-1`'s journal holds `native_failure` « display or output change: native resize:
  HRESULT 80070005 », the throw of `FramePresenter::Resize` (the integrity review's M1). An Alt+Tab in exclusive full screen
  provokes it; it was not run again here.
- The mask composite's presence at the resolution has no row of its own: `GXRend_CompositeShadowMask` is listed resident in the
  three `light-observe` journals of tranche 35 (`i66-native-i113-t35{b,c,e}-face`), before their resolved row.

So A3, B6, C2, C4, G5 and G6 are structural corrections of cases not found in a run, as I-0121's were.

| # | The code before (READ) | The original (READ, GhidraMCP) | The correction (its plan; what was written is below) |
|---|---|---|---|
| A3 | `14_scene_step_and_storage.inc`, `AdmitEncodedTextureFamily`: every locate or decode failure of an encoded texture still calls `Refuse` | 14002B260 and 1400AC110 retire the wrapper first (vtable +0x88, AC020), ask the source for the texture (vtable +0x50) and, when that fails, return with the wrapper left empty | The same: the wrapper retired and left without a record, a counted skip row naming the reason; the models that name it then take the path of a texture bound with no data (`NativeFallbackTexture`: the loader's own SYSTEX stand-in by name, else the row waits). A refusal stays for the device's failure to make the resource |
| B6 | `05_selected_scene.inc:875-881`, `ProduceNativeSelectedInstances` still refuses when the group's or the model's fields cannot be read | | The admission's skip (`native_admission_code` 2, a counted row), as the reads around it |
| C2 | `09_model_admission.inc:872` reads the Reflection option byte at admission; no epoch tracks it | | The byte in `NativeRecordEpochs`, compared by `SameStable` and read once a collect |
| C4 | `06_pass_programmes.inc:98-137`: the first complete set kept for ever; of its optional members only `organize` and the SSS three are still read since I-0131 (the point, spot, mask and shadowed-sun programmes are looked up and never used) | | The optional members that are read asked again while one is missing (bounded); the lookups of the unread ones removed |
| E3 | `backend.cpp:972`, `FramePresenter::Resize` still throws on any `ResizeBuffers` failure | | `TryResizeBuffers`' rule: a failure that is not a device loss leaves stand-ins of the asked size and the next frames follow the presentation until a resize succeeds |
| A2 | `20_startup_phases.inc:496-513`: SYSTEX_BLACK for a menu proxy or a movie frame with no native target yet; an asset with no resident record at all refuses | 140E49320 returns 0 when the manager makes no image from the texture | 0 returned and a counted skip, for both cases (after A3 a texture left empty reaches this import) |
| G5 | `game_ui_mesh_owner.cpp:133`: the game closes at 4,096 live UI mesh items | not read (what the engine's callers do with an allocation that fails) | the bound removed |
| G6 | `primitive_textures.cpp:12, 73`: the UI texture set refuses past 1,024 images and the import then refuses | as A2 | the import returns 0 with a counted skip |

What was written (`178976ec`, then the reviews' answers):

- **A3** (`AdmitEncodedTextureFamily`). A `skip` retires the wrapper (the original's first step), writes a row for the first 32
  refusals that carry a message (`native_encoded_texture_failure` with `skipped` true) and counts through `NoteNativeSkip`, for:
  an unreadable DDS header or payload, a header or a container the decoder refuses (its `std::exception`s other than
  `bad_alloc`: an allocation failure or a foreign exception inside the decoder is the module's and still refuses, the integrity
  review's M2), a source object of another class or size
  (`native_encoded_source_unowned`, now at most 32 rows), a missing container entry, a 2D and cube mismatch, a decoded image the
  ingress would refuse (`ValidCpuTextureResource`, checked before the admission). An authored constant that cannot be read
  leaves the texture without one, counted. Still a refusal: the allocator global unreadable, and any exception left (the
  device's or the ingress's). What names a skipped texture finds a holder without a resident texture, the existing path of a
  texture bound with no data (`NativeFallbackTexture`).
- **B6.** `ProduceNativeSelectedInstances` returns false for an unreadable group or model field; the admission then skips the
  model for this collect (`instance_group_unreadable`, admission code 2), as its neighbours do. The authored identity matrix, a
  global of the image, still refuses.
- **C2.** `NativeRecordEpochs::reflection_option`, read in `CaptureNativeRecordEpochs` as the water block reads it and compared
  by `SameStable`: a change of the Reflection option re-admits the records at the next collect. The trade-off (the final
  review's L5): every record once, and the kept records out of view dropped, at a change made in the Graphics menu, where a
  per-record test would need to know which records carry a GX07 water block (the existing `water` mark is the programmes'
  water slots, not that block).
- **C4.** The set is still published once and kept (the renderer refuses a change of set inside a frame's list). What changed is
  when: a set that has its five required programmes but lacks an optional member is held back, the answer staying « unresolved »
  as while a required one is missing, for up to two seconds from the first asking that found the five; then what is there is
  published (`whole` false in its row, `native_lighting_programmes_waiting` meanwhile). **The first form was withdrawn** (the
  step review's H1): it replaced the kept set when a later asking found more members; the main collect and a menu view's collect
  of one frame could then hold two sets, and `NativeLightingResources` throws on a change inside an open list, which closes the
  game: worse than the finding. The plan's table also said « only `organize` and the SSS three are still read »: wrong, the
  point, spot, mask and shadowed-sun members are still read by `scene_pass_lighting.cpp`; all count, and all are found in the
  runs (`GXRend_CompositeShadowMask` resident in three `light-observe` journals of tranche 35, `sun_shadowed` in the start
  checks' rows). During the wait the frames are not lit; before, a late member was lost for the session.
  **A journal row changed** (D-0268, stated plainly): `native_sun_shadow_programmes` is no longer written; its content is the
  `sun_shadowed` field of `native_lighting_programmes_resolved`, which also gains `mask`, `whole` and `waited_ms` (no reader of
  the removed row in the repository's scripts). Two more rows changed: `native_ui_asset_unavailable` is written at most 32 times
  and is followed by a skip, no longer by a refusal; `native_encoded_source_unowned` at most 32 times, likewise.
- **E3.** `FramePresenter::Resize` throws only for a lost device. Another failure: the chain's buffers are taken again when it
  gives them and they are of the asked size, otherwise stand-ins of the asked size, which make every frame follow the
  presentation until a resize succeeds. Frame generation is off for a Present on stand-ins (the step review's M1: the chain may
  then be of another size than the frame's tagged resources). `TryAcquire` no longer takes buffers that are not of the presenter's size (after a refused resize to another size
  the chain still holds its former ones: the follow's refused resize would have given 97x71 targets to 131x89 frames).
  `frame_presenter_test` (new, WARP and hardware, a hidden window): a reference kept on a back buffer makes DXGI refuse the
  resize; T1 to T4 as its PASS line says (passed in `build-p4a` on the first `Resize` and in `build-p4c` on the present one; T4
  then gained a check that DXGI really refuses the same-size resize, run from the next build on).
- **G6, and A3's sequel in the UI.** `NativeUiTextureImport` answers 0 with a counted skip for a holder of another class, an
  asset with no resident native texture (what A3 leaves empty would otherwise close the game here), a failed allocation, and an
  image the UI set does not take for a reason of the data: its format (a cube, an array, a format the UI does not sample) or the
  set's bound (`PrimitiveTextureLimit`, the 1,024 now named; the bound itself stays). `ImportTexture` now says why it took
  nothing, and the module's own reasons still refuse (no session for the manager, a closed owner, a set that refuses an import
  it has room for; the integrity review's M3: the first form folded them into the skip). READ in the original
  ([ORIGINALS](ORIGINALS.md)): 140E49320 returns 0 when its manager makes no image, and its one caller, 140E48980 (the image
  repository's lookup by name), stores an image only when it is not null, so a 0 is asked again at the next lookup.
- **A2 is not corrected.** The first form removed the SYSTEX_BLACK placeholder of a menu proxy or a movie frame without a
  native target (0 and a retry instead). The integrity review (H1) showed the case is not rare: it is met at every character
  creation, at the credits, the colour select, Load Game and Status. What a menu then shows with no image instead of a black one
  was never seen, so the placeholder is back exactly as it was (its row too) and A2 waits for a witness: a character creation
  and a Status page natively with stills before and after the change, a short run to ask of the coordinator.
- **G5.** The bound of 4,096 live UI mesh items is removed from `AllocateItem`'s refusal (**a check of the module removed**,
  D-0268, stated plainly): an item lives as long as the engine's mesh (the engine evicts and destroys through the module's
  `EvictItem` and `DestroyItem`); the original evicts by its GPU pool's budget, which has no native equivalent. What the engine's
  callers do with a failed allocation was not read, so the allocation never fails for a count. In its place a witness: a row
  `native_ui_mesh_items` each time a cache's live items reach a power of two from 4,096 up.

**A reviewed list changed** (D-0268, stated plainly): `scripts/thread-affinity-allow.json` gains one entry for C4's new function
static in `AcquireNativeLightingPrograms` (`wait_from`), read and written only under that function's mutex;
`Test-ThreadAffinity.ps1` refuses a new static until it is reviewed. Its five stale entries are older and not this tranche's.

Not checked for tranche 2: none of the corrected paths has run natively (a refused texture, an unreadable instance or a missing
programme cannot be provoked in a start check; E3's refused resize can, by an Alt+Tab in exclusive full screen, and was not);
E3 alone has a test on a real swap chain. G5's live item count in a long session is unmeasured (its row would show a growth past
4,096; the memory leg of the full control, D-0230, a leak). Known and unchanged: a skipped texture that is one of the four stage textures still
refuses later (`native_stage_texture_residency_missing`, `PublishNativeStageTextures`; the step review's L7), and the start-up
Views phase refuses if a resize were refused during it (`native_view_not_resident`; L4).

Tranches 1 and 2 merged `8f415270` (2026-11-12 10:08); the launcher rebuilt from it (10:09-10:22, 124 PASS lines, `evidence/I-0137/launcher-build-8f415270.log`); start check `start-8f415270b` passed (10:23-10:24, the ASUS, 372 world frames, closed by its close box, restored by hash; its journal shows the lighting set resolved whole at the first asking: `whole` true, `waited_ms` 0, `mask` 1).

### Tranche 3: priority 5, the low findings (first part), and A2 for the movie frame

Merged `57b12d2e` (2026-11-12 11:46); the launcher rebuilt from it (11:46-11:58, 124 PASS lines, `evidence/I-0137/launcher-build-57b12d2e.log`); start check `start-57b12d2e2` passed (11:59, the ASUS, 367 world frames, restored by hash). The disk rotation (D-0260), refused at I-0113 tranches 40 and 41 and twice here while a compiler ran, ran at 12:01-12:03: 22 build folders older than 48 h, 7.2 GB, deleted (and 8 folders, 4.6 GB, at 09:24 before a compiler stopped it); 366.6 GB free on C:.

Located first in today's code ([PRIORITY-5-LOCATIONS](PRIORITY-5-LOCATIONS.md), a Sonnet pass, each row re-read before it was
changed). Corrected in a first part (`dd6f298f`), each re-read in the code:

| # | The correction |
|---|---|
| A7 | `InitializeNativeModelStreams` (checked before its copies and again at its publication, the mesh count written either way) and `UpdateNativePhysicalModelStreams` return quietly after `CloseNativeModelStreams`, as the cloth path does, instead of a refusal at a test run's close; its sibling, a model storage admitted after the close (`AdmitModelStorage`, « native model admission closed »), still refuses (left) |
| A8 | `native_post_transition_intent` and `native_scene_auxiliary_view`, stored under `scene_view_cpu_lock` and never read, are no longer stored (the intent's row keeps its generation from an atomic count) |
| A9 | `model_geometry_refusal` loses a storage's entry when the storage is destroyed (a reused address took an old refusal; the map grew) |
| B9 | `CopyModelGeometry` compares the second stream's offset and stride between its two copies, and a mismatch names its check (14) |
| B11 | an always-true `if(table)` in `ReadModelParameterAnimations` |
| C7 | `kCosC1[3]` `0xad49cba5`, the game's word at 1439F2DBC (an error of about 1e-10 in DecalCos; `decal_constants_test` bounds it against `std::cos` at 2e-6, unchanged) |
| D7 | `ds3hostio::Shared` and `ReadForProducers` take the entry under the table's lock (the reference readers `Bytes`, `Lines`, `List` left: their callers read on the thread that writes) |
| E4 | a witness picture (the RecordPixels completion, the velocity picture, the model pixel evidence) that cannot be mapped or written is dropped with a debug line instead of thrown, which faulted the renderer; a readback map of the frame's own statistics still throws |
| E5 | a view context no longer makes a readback buffer and its footprint at every target generation (never read) |
| H4 | the `lod_step` witness prints `record+0x70` and `manager+0x110` as integers, as `PopinLodChange` reads them |
| H6 | `ObservePartPoseUpdate`'s map capped at 4,096 receivers (pose-observe.flag) |
| B10, F7, F8 | stale comments rewritten (`model_frame.h`, `tonemap_stage.h`, `rt_scene.h`) |

The step review (a fresh Opus at high effort, `8f415270..92762f99`): criteria met for A7, A8, A9, B9, B11, C7, D7, E4, E5, H6
and the comments; the witness flag off in play and its path main's skip path. **H1** H4's change had put a comment in the middle
of a line, swallowing the four reads after it: every `lod_step` row would have printed zeros (the reads restored on their own
line, `c4c24d1c`; this is LIVE-RUN-NOTES' known pitfall, a mid-line comment). L1 two comments still said « the previous intent
kept » (rewritten); L2 the view context's readback members left unused (removed with their resets); L3 a model stream received
after the close skipped the mesh count the genuine constructor writes (written before the closed check). Its note that
`06_pass_programmes.inc`'s « Nothing records them before YB-8/YB-9 » is stale is taken with the second part.

**A2, in part, after its witness** (`i66-native-i137-a2-black` with the stand-in as main, `i66-native-i137-a2-none` with the test
flag `ui-no-placeholder` making the import answer 0; both natively at the character creation, the ASUS, 2026-11-12 10:39-10:50,
offline, restored by hash; `evidence/I-0137/series-a2.ps1` and its log; build `evidence/I-0137/build-p5b`). OBSERVED in the
journals: the import the stand-in served there was **the movie frame texture** (vtable 1429A4968) right after the opening movie's
close, not a menu proxy: in the black run `native_ui_offscreen_placeholder` twice (lines 54953, after `native_movie_close`; and
54987, after the creation screen's 784x784 proxy targets were made), in the other run one `native_ui_asset_unavailable` row
(`texture_table_rva` 0x29A4968, after `native_movie_close`) and one `ui_import_asset_unavailable` skip; no import of a menu proxy
without a target happened in either run (the integrity review's M1; a first version of this record said « before the proxy's
target », wrong). The stills taken about 30 s after the movie's close, `c1` to `c4`, and the movie still `movie-150`, taken before
it, differ between the runs by a mean of 0.6 to 1.5 of 255 over the game window, with peaks of 149 to 234 where things move
(the character's idle pose, the movie) (`ffmpeg -i <black> -i <none> -filter_complex "[0:v]crop=1280:720:8:30[x];[1:v]crop=1280:720:8:30[y];[x][y]blend=all_mode=difference,signalstats,metadata=print"`,
YAVG and YMAX, figures not kept beyond this record); `c2` looked at in both: the same screen. Not observed: the transition's own
frames, where the stand-in or the 0 was used (no film; the integrity review's M2). Both runs ended in the same access violation
when the pilot closed the creation screen (`FUN_140D126ED+0x6C`), as three of the four native face runs of I-0113 tranche 35 and
its Reference run did (the fourth native one crashed at `FUN_140E60346+0x37`): not this change's. So **the movie frame texture's
stand-in is removed** (it answers 0; while a movie plays its target is imported at every frame, 11,873 imports of one generation
in the none run, so the next import after an answer of 0 takes it), and **the menu proxy's stand-in stays** until a witness at
the Status page. INFERRED, not OBSERVED: that the black run's two placeholder rows were the movie frame (they carry no class; the
none run's row at the same place has the movie frame's table). The final review read every `native_ui_offscreen_placeholder` row
of the evidence store: from run 1250 on, every one sits at a movie's close or open (the credits' before-open case, now 0, not
witnessed); the menu-proxy uses are in older runs only (`status-1163`, `loadgame-1164`, 1171 to 1180), and six current
portrait runs hold none; only the Status page has no current journal. The test flag is withdrawn.
**Rows changed** (D-0268): `native_ui_offscreen_placeholder` is now written for a menu proxy only; a movie frame without its target
shows as `native_ui_asset_unavailable` and a `ui_import_asset_unavailable` skip.

E4's dropped picture (stated plainly): a witness picture that cannot be mapped or written leaves only a debug-output line (the
renderer has no journal of its own) and possibly a partial file; a run's reader would see a missing or short picture.
Left after reading, with the reason: F5 (the scene copy and depth-of-field programmes resolved and pinned with the LUT copy and never read: `AcquireNativeYebisGxPrograms` takes all three or none, so a mismatch of an unread one turns the grading off for the session; the comment says so now, the rule is unchanged); F4 (the grading LUT's copy every graded frame) until it is known whether the strip changes
under one generation (a blend between regions would freeze); B8 (the wet catalogue read every collect) until it is known whether
the table at 4731BE0 is written after the start. The rest of the list follows in a second part.

### Tranche 4: priority 5, the rest of the low findings

On `claude/sharp-tereshkova-5cbcb6` (the E pilot's new thread, from `main` `d689149c`). Each row of
[PRIORITY-5-LOCATIONS](PRIORITY-5-LOCATIONS.md) re-read in today's code before it was changed; the dead code and stale comments
(E7, E8, G8, D6, G10) removed by a Sonnet subagent at places it checked by grep, read again by the pilot; nothing proved by a run
(no live run before a merge, D-0227).

| # | The correction (READ: the code before, as the finding states it, unless said) |
|---|---|
| A6 | The cloth upload copies the engine's copy once (no zero-fill first) and hashes it only while its stream is not yet simulated (`simulated` is sticky); the rare entry replaced or released between the two locks is hashed then |
| A7's sibling | `AdmitModelStorage` after `CloseNativeModelStreams`'s registry close: the storage dropped (its prefix freed, a `native_model_storage_dropped` row) instead of a refusal, as the insertion's own closed path already did |
| B5 | The Velo rows' velocity scale and the snow inputs (scene rate, Snow Model built-in rate, gparam banks and their binding generation) leave the kept draws' global epoch: a kept row that wrote one keeps its own stamp and is rebuilt when it changes (`NativeKeptDraw::velocity_scale`, `snow`; FC_SnowRate counted written when the declared cbModelParam reaches +0x40) |
| B8 | The wet shader catalogue (43 names at 144731BE0, `.data`, its first name in the file) read once and kept (`NativeWetShaderCatalog`, a shared vector compared by address). READ through GhidraMCP: no reference to its base, only 2DD60's computed compare at an MTD load; INFERRED: nothing writes it after the start |
| C5 | A group that composed nothing is answered empty again without the repository walks while `shader_registry_changes` holds, for at most one second |
| C6 | When a layout's programmes are asked again (the program registry moved), the kept ones are returned only while every loose programme its recipe set asked for and did not find is still absent as last asked (`Programs::absent`, asked again through `FindLoadedProgramme` after a shader admission or retirement, or after a second); the composed groups' own late members are not re-asked (left, below) |
| C9 | The glare acquirer compares the last four plans' technique lists before building its technique map and key |
| D3 | The settings shadow Execute names at `[[GXRS]+0x4DD0]` and the engine bodies that test that member on other threads (61950, 1AC840) no longer overlap: a Dekker handshake (`effect_draw::EnterEngineShUse`); 1AC840 takes the device-absent path natively (the clock, then the native SH block) while the shadow is named, 61950 (a light set record change, rare) waits for the record's end. READ through GhidraMCP: 61950 re-reads the global and, with the member set, calls `LightSetRecord_Get` on `root+0x41E0` |
| D4 | The forward models' cbLight and cbFog entries built once per group key in a collect (`NativeForwardLightKey`) and shared by the models with that key |
| E6 | `native_depth_view_` removed: the depth target is always the typeless form (no caller made it before the model targets) |
| E7 | `RenderNativeModels` (no caller) removed; `MaterialColorFramePlan` moved into `frame_graph_test.cpp`; `PlanFramePlacement` and `TakePlanPassesPeak` left (the test checks the production function, and the peak's counter has no other source) |
| E8 | `06_pass_programmes.inc`'s « Nothing records them before YB-8/YB-9 » rewritten (yebis_stage.h was already corrected in tranche 3) |
| F2 | A model storage readable by the rays is also left in PIXEL_SHADER_RESOURCE (the traced shadow's pixel shader reads it) |
| F3 | An empty palette no longer marks a skinned BLAS failed for good: the frame keeps the last positions (none yet: left out) |
| F4 | The grading LUT's copy into the volume made only when (resource, generation) changes. READ: the strip is a loaded texture record (`mAA_BB_CGradingNNNN`, 16x256 RGBA8), one per area, never blended natively |
| F5 | `AcquireNativeYebisGxPrograms` resolves and pins the LUT copy alone: the scene copy and the depth-of-field table, never read, no longer turn the grading off by their absence or a pin mismatch |
| G8 | Removed: `BuildTextureTiling`, `TextureScrollOwner::Acquire/Apply`, the `ProduceTexturedPrimitiveWork`, `ProducePreparedPrimitiveWork` and `ProduceSelectedPrimitiveWork` chain and their types. Left: `PrimitiveUniformSource::Write` and `DecodeCpuTextureContainer` (their tests call them) |
| G9 | No UI image manager (the owner closed, or no R8 sampling): nothing consumed, and the HAL's image services answer 0, instead of a null dereference |
| G10 | The « 1159: ADEF50 » comment moved above the function it describes |
| D6 (part) | Removed: `ProduceModelParam`, the extents overload of `FloatMask`, `View::From` and its two helpers (no caller). The snapshot oracle in `light_producers.cpp` (about 1,000 lines, interwoven with the SSS profile code; no production caller found, the three `.txt` files published nowhere) left for a cleanup of its own |
| H5 | `ModelDrawObserved` (Reference witness) counts this thread's indexed draws (a `thread_local`), not the process's |
| S4 | A material whose diffuse view the bindless heap refused is a record without its material (its hits unread, a miss), not FC_DifMapMulCol over white |
| S5 | A walk with no named texture registry leaves the slice's set pending (made again later), as the alpha path did, instead of kept for good without roles |
| S7 | A skinned instance without a material set takes no positions view and writes no bases |
| S12 | The build refuses when `rt_view.hlsl`'s `RT_FAMILIES` and `rt_scene.h`'s `kRtViewFamilies` differ |
| S13 | `RtMaterialTable::TextureView`'s unused ticket parameter removed |

Left, with the reason:

- **A4** (the statics owning D3D12 objects destroyed by the CRT at the process's detach): left; no exit hang was seen in any run
  read (the exit faults of step 5 are faults, not hangs); the existing practice (the renderer and `bindless` leaked, `*new`) is
  the likely form if one appears.
- **B4** (the pose update's compose inside the global pose lock): moving it out needs the owner's palettes made safe against a
  concurrent retire; no contention measured.
- **G2, G3, G7** (the UI images copied and uploaded twice, the constants packed twice a draw): a redesign of the UI ingress, taken
  with workstream 4's step 1 (the interface).
- **H2**: the danger the review saw (a stray `witness-flag.txt` setting an event flag in a play save) is gone: the play launcher
  refuses to start with any `test-*` or `witness-*` file in its folder. The three probes' cost is kept (the runners' tools need
  them without a flag).
- **H3**: kept for one object by I-0114's final review L3 (one follow camera seen in play).
- **C6's composed groups**: a composed group's member linked after its composition is not looked for again (`composed_effects`
  is kept); not witnessed, and a recomposition would make every layout of that group again.
- **S2, S3, S10** (data questions: a TEXCOORD0 format outside the three read, the scroll on float-UV slices, one texture at two
  registers with different tiling): open until a witness; **S8, S9** (`rt-view.flag` only); **S11** (kept by I-0119's step
  review L4).
- **D6's oracle**, **E7's** test-only functions, **G8's** two test-only functions: above.

### Tranche 4 (continued): workstream 4's step 5, the exit faults told apart

The roadmap's three exit faults, and the character creation's close fault the A2 witness met, read again (GhidraMCP and the
records) before any change:

| Fault | Whose | What was done |
|---|---|---|
| `DarkSoulsIII.exe+0xF14C52` at a close, the I-0124 path (the tour's original, I-0139, Reference mode; earlier I-0114, I-0115, I-0123's endurance) | **DS3's own**: I-0124 read it (the CRT `_purecall` 141FD0EAC whose handler writes 0xDEAD at 140F14C52, called by the FMOD sound thread 141A3B670 on a heap the CRT's exit chain already destroyed). The tour's stack (141FD0EAC < 141A3B670 < 141A39FC0 < 1417FD480) is I-0124's | Nothing more: I-0124's sound stop at the quit (`native_exit_sound_stop`) is installed in every native run since `ac958d0c`; the Reference mode keeps the original's behaviour, so the tour's original can still fault there |
| `DarkSoulsIII.exe+0xF14C52` at a close, **another path** (I-0113's « exit fault at F14C52, not attributed », its tranche 38: `i113-t37-fogwall9-native`, g24i, build t38c, 2026-11-11, native, after I-0124's fix; the integrity review's H1) | **Not attributed.** The same `_purecall` handler, reached on thread 31440 through 141FD0EAC < 1422BA280 < 1422B00E0 < 141B608D0 < 141B5F770 < 1417FD480 (its crash report), after both exit stops had signalled (`native_exit_sound_stop` 141 ms, `native_exit_network_stop` 31 ms). READ (GhidraMCP): 141B5F770 is a thread's loop that polls the objects at its `+0x10`, `+0x20` and `+0x30` (141B608D0 calls their slot 8) until `+0x429` is set; 1422B00E0 is such an object's slot, which calls 1422BA280 while its `+8` is set. INFERRED: the exit-order kind of I-0124 and I-0099 (a DS3 thread still polling objects the exit chain destroyed); its class (who starts the thread, who sets `+0x429`) not read | Open: the thread's owner by its vtable and DS3's own stop of it, then a stop at the quit as I-0124's. Seen once |
| `FUN_140D126ED+0x6C` (D12759) at a window close during the character creation (I-0137 tranche 3's two witness runs, three of I-0113 tranche 35's four face runs and its Reference run) | **DS3's own**: I-0066's 1173 RCA observed it in the original (`i66-reference-charmakeclose-1173`, `i66-reference-bodyscale-1173`): the location update D126B0 still runs on a `CSFD4LocationBodyScaleModifier` whose c0000 body-scale table (`[[+0x58]+0x18]`) is released | Natively, a guard (`NativeBodyScaleUpdate`, always installed): a modifier whose table is gone changes no bone and returns its count; a row `native_body_scale_update_skipped` the first time. 1173 had kept parity (« no fix »); under the remaster a crash at the player's close is fixed as I-0124 fixed DS3's exit race. Reference unchanged |
| `FUN_140E602F0` with a menu open at a close (I-0113 tranche 31's close from the profile list, the « Select profile to delete » list; I-0132 R12g's RCA, the Status face) | **The module's**: `Close` runs only at the controller's stop (a bounded test run: the play controller never sets it, `game_bootstrap_ctl.cpp`), on the engine's own thread, and withdraws the Scaleform HAL (`[container+0x980]` 0) and frees the UI images; that thread then runs the menus' next frame, and E602F0 calls the HAL's slot +0x128 without a null check | At the root: after `native_closed`, a stopped `Close` keeps its thread (the window thread) waiting for the controller's `TerminateProcess` (which follows `closed` within one 20 ms poll; bounded at 15 s). A first form, a guard on E602F0 alone, was witnessed (below) and withdrawn: the next frame then faulted in E5B1F0 on a released image |

The two roadmap items « the guard on FUN_140E602F0 with a menu open » and « the Select profile to delete list » are this one
fault (I-0113's RESULT names the same faulting instruction, `MOV RAX,[RDI]` at 140E6037D with RDI the withdrawn HAL).

The witness pair (build `p6b`, the ASUS, 2026-11-12 13:31-13:41, offline, Alpha's copy restored by hash both times; the runner
`evidence/I-0137/series-p6.ps1` as it ran then: build `p6b`, tags `i137-t4-status` (Load Game slot 0, `-StatusPage`, `-Seconds 300`,
`-StaySeconds 20`) and `i137-t4-charmake` (`-Pilot CharMake -DeleteProfile`, `-Seconds 420`), both `-NoDebugLayer` on Alpha's copy;
its log `series-p6.log`; the file was then edited for the next pair). The pilot's new switch `-StatusPage`
(`Pilot-LoadGameThomas.ps1`, passed by `Run-PilotPair.ps1`): Esc, Right twice, E, two screenshots, the page left open (since
`3c642243`, also held 40 s so that a run's `-Seconds` can end there). OBSERVED:

- `i66-native-i137-t4-status`: Load Game, then the in-game menu's Status page (`status-page.jpg`), left open, then the window's
  close box: exit 0, no fault, the game's own exit (`native_exit_sound_stop` « signalled », 140 ms). The run made at least 12 menu
  proxy targets (`native_ui_offscreen_target`, a row capped at 12) and wrote no `native_ui_offscreen_placeholder` (capped at 8, so
  none) nor `native_ui_asset_unavailable`: the proxy's stand-in was never used.
  **So A2 is finished**: the menu proxy's SYSTEX_BLACK stand-in is removed too (its import answers 0 and is asked again, as the
  movie frame's since tranche 3). Not observed: a proxy imported before its target, or after its view retired (the two cases the
  stand-in served; `16_offscreen_and_movies.inc`'s retire hook's comment corrected).
- `i66-native-i137-t4-charmake`: the pilot did not reach New Game (the Rekindled page still on screen during its delete steps,
  `del-3.jpg`; `timeout waiting for New Game`), so the creation's close was not witnessed. The controller's stop came with the
  title's menus running: `native_closed`, then the E602F0 guard's row (`native_ui_image_lookup_skipped`, « no_hal »), then an
  access violation in E5B1F0 (`ReplaceImageResource`, MENUS-MAP 4932) on the window thread 2996. INFERRED: that this thread ran
  `Close` (the journal gives no thread for `native_closed`; 2996 is the engine and window thread, and `Close` runs in its frame
  lifecycle, `17_render_step.inc`) and that the image it replaced was one the close released (the dump's operand not read). Hence
  the root correction above (the thread kept after `Close`) in place of the guard.

The second pair (build `p6c`, 14:03-14:13, the same protocol, restored by hash both times; `series-p6.ps1` as it is now, its log
`series-p6c.log`). OBSERVED:

- `i66-native-i137-t4-stop`: the Status page open (`-Seconds 160`, the page held 40 s): the controller's stop came with the page
  open (« gone while the status page was open »): `native_closed`, `closed` 1, no fault, no dump, the controller's exit code
  0xD3126612. **The kept thread holds the case** that faulted in E5B1F0 at 13:41.
- `i66-native-i137-t4-charmake2`: the pilot again did not reach the creation (« creation or movie seen: False » after its delete
  steps); the stop came with the title's menus open: `closed` 1, no fault (the same situation as `i137-t4-charmake`'s fault).

Not proved by a run: the D126B0 guard (a close during the creation): the CharMake pilot no longer reaches the creation from Alpha's
copy after its delete steps (twice today; it did at 10:39 in tranche 3): a pilot question, left for the next witness (run in
tranche 7: it reached the creation and showed the guard insufficient).

### Tranche 5: workstream 4's step 1, the interface's unsupported cases skipped

On the same branch, from `main` after tranche 4. ROADMAP's step 1: « 486 of 1,316 UI selectors have no programme and an
unsupported case ends in `std::terminate` » (I-0113's STUDY-NOT-YET-NATIVE R1). Each `Fail` of the UI session read in today's code;
those that are a case of the game's data (a family the native does not draw) now skip and count, through the session's existing
`Skip` (a `native_ui_skipped` row its first four times and every 1,000th); those that are the module's own contract (a scope
unbalanced, a second selection, an exception) still fail.

| Case | The code before (READ) | Now |
|---|---|---|
| A pair whose uniform layout the native values do not take (`Begin` Unsupported; the 77 « unsupported » of p7a's `primitive-uniforms.txt`) | failed (`native_ui_unsupported_cpu_layout`) | the pair's own descriptors published, its status kept; the draw skipped (« layout_unsupported ») |
| A display of offscreen composition, or with an authored colour clear | `BeginCpuDisplay` failed | the display opens and closes as an invisible one (HAL+0x58 bit 0x20 clear, the existing path of a display with nothing to draw: no primitive or complex mesh prepared, its filters unfiltered; its scopes, masks and targets kept balanced), a skip counted once a display; a draw reaching `RecordUiDraw` in it fails (`native_ui_skipped_display_draw`) |

Still failures, the module's contract: a stale or invalid bank or mode at the selection, an invalid layout at `Begin`, a binding scope
refused before the preparation (`shader_status` now starts Invalid), a second selection, an exception; and every other non-Ready
answer of the preparation (constants the native's writer left unwritten, `WriteUniform`'s or the packing's Missing or Unsupported, a
3D-view node): only a status the selection marked as a case of the data (`data_case`) skips (the integrity review's H1).

Withdrawn after the final review (recorded as left): **a selector with no programme pair** (one of the catalogue's 486 unavailable
recipes of 1,316; build p7a's `primitive-uniforms.txt`: supported 753, unsupported 77, unavailable 486) still fails in the writer's
selection: after the selection the game's CPU algorithm binds the draw's image at the pixel programme's row 20 without a check
(`decompiled/DS3-1152-F-01F784C0.c`, `-01F81EB0.c`), and no descriptor can serve a programme that does not exist (the game's empty
row is register -1, which the image binding refuses). INFERRED by the final review: the 486 are the planar video families
(`decompiled/DS3-1152-F-01F79070.c`'s planar branch), which no native image takes today (the native images are formats 1, 2, 9 and
50 to 53), so unreached; a real skip would need the binding scope discarding that draw. The step review's « no uniform »
descriptors went with it.

Withdrawn after the step review (recorded as left): a draw's packet the producer refuses still fails (H1: the frame's payloads lent to
that packet are lost by the refusal, and the frame's own admission then fails); a filter pass whose programmes or layout the native
lacks still fails (M4: a skipped middle pass leaves its target blank and blanks the chain; the fallback is the target refusal's, pop
and draw the subtree unfiltered, to write with it); the gradient kinds 4 and 5 (L1: `NativeFiltersSupported` already refuses kinds
above 2, so the branch is unreachable).

Left in step 1, with the reason: the selector without programmes (above); an instanced vertex layout (`MapCpuVertexFormat`, HAL+0x39F18 > 1: what the engine does with an
unmapped format not read), a text layer kind the mesh writer does not generate (`game_ui_mesh_owner.cpp`: the mesh the writer
must still return to the engine), the HAL's never-qualified methods (`Unsupported<Slot>`: their return contracts unknown), and the
two withdrawn above; each needs its engine caller read first. Step 2 (the `Unsupported*` stubs that refuse, I-0101's unsorted
stops) is the next tranche.

Not proved by a run: no case above has been met natively (each would have ended the game); the start check after this tranche's
merge is to come. Builds `evidence/I-0137/build-p7a` (the first form), `build-p7b` (the step review's answers), `build-p7c`
(`ef5fabf1`, the integrity and final reviews' answers) and `build-p7d` (the last step review's answers, the tree that merges);
each: every unit test passed, 124 PASS lines; `build-p7e` (`83d60c1d`, main merged in again with the page's fourth tranche): the same.

### Tranche 6: workstream 4's step 2, and step 5's F14C52 path

A fresh thread on `claude/stoic-jennings-3bc114`, from `main` `57df4723`. [STEP2-LOCATIONS](STEP2-LOCATIONS.md) (a Sonnet pass, not
checked) re-read row by row in the code before any change; its line numbers held.

**What the journals say first** (a `measure-tools` census, `evidence/I-0137/t6-refusal-census.md` and its script beside it: every
`startup.jsonl` of the evidence store, 4,693 journals, 257 with a refusal; it counts `refusal` rows only, so a stop that ends in a
bare `std::terminate`, as a filter object's method outside a filter scope, is not in it). OBSERVED, for STEP2-LOCATIONS' stops: most
never fired (the walk's bounds, the movie planes and outputs, the cloth sets, the material, shader, gparam and texture loaders).
Those that did, each answered by its own record: in October's start-up builds the `Unsupported*` stubs
(`native_model_storage_method_not_owned` and ten `native_ui_session_method_*` slots, `i66-native-startup-*` of 2026-10-08,
qualified since), `effect_native_program_missing` (`native-15`, 2026-10-08) and a filter pass (`native_ui_filter_pass_shader_status_1_key_220000`,
`i66-native-1266` and `-1267`, 2026-10-11); since then three: I-0101 L4's `native_output_resize_failed` (`i101-fs-alttab-1`,
2026-10-28, and `i111-fs-quit-1`, 2026-10-31: the follow of a lost exclusive state, fixed by I-0101 and I-0111, the stand-in back
buffers), the movie frame's catch in its offscreen form `native_offscreen_view_failed` (`i111-q-1`, `i111-b-1`, 2026-10-30, and earlier `i66-native-1325-charmake`, 2026-10-11: the
readback map at an odd width, fixed in I-0111 `ea7f4746`), and `native_ui_movie_viewport_contract` (`i125-r6e2-gundyr`,
2026-11-06: the title movie before the window was open, fixed in I-0113 tranche 19; the refusal left covers an unreadable player).
None fired after 2026-11-06. The other refusals met since 2026-10-20 are outside step 2's list
(`native_render_worker_faulted`: the Agility SDK's debug layer refusing pipeline states, cleaned in I-0132 R11i, and a resolution
change in play, fixed in R12f; `native_cpu_shader_policy_thread`, `native_view_window_extent_changed`, `frame_begin_without_native_owner`
and `native_scene_root_frame_foreign`, in runs of I-0088, I-0096 and I-0097, I-0119 and I-0128, their outcome in those records, not
re-read here). So step 2 is mostly preventive: it converts where
the case is plainly the game's data and the skip has a defined end, and leaves the rest with its reason.

| Row (STEP2-LOCATIONS) | Read | Now |
|---|---|---|
| Walk: a map piece's children past 4,096 (`native_scene_selection_disp_children_bound`) | the list is walked by the module itself (`09_model_admission.inc`), so the bound ends a list that never comes back to its head; the rays' walk already skipped the entity (I-0117) | skipped in the main walk too: the candidates that entity gave are taken back (every walk of `12_scene_collect.inc` defers its candidates), counted `disp_children_bound`; a direct admission (no deferred list) still refuses |
| Walk: more than 32,768 objects in one scene (`native_scene_selection_object_bound`) | the visits are driven by the engine's traverser; the largest walk in the store is 2,129 objects (`i100-t6i-ariandel-n1`; 2,108 in the latest 265 journals, `full-1112-night`; `evidence/I-0137/t6-walk-max.txt` and its script, over the `native_main_walk_layers` rows: a walk's first three and every 600th) | **kept**: past 15 times the largest scene it can only be a traverser that does not end, which a skip would turn into a hang |
| Walk: the tree's CPU arena and bounds source | the module's own arena (64 MB) and its source check | kept (the module's contract) |
| Movies: a ready set, plane shape or plane buffer out of the module's reach (`native_movie_plane_set`, `_plane_shape`, `_plane_buffer`) | Bink's description of the frame (`16_offscreen_and_movies.inc`, `NativeMovieGraphicsDraw`) | that frame skipped and counted (`movie_plane_*`), the output keeping its last image as when no set is ready; the three planes' buffers checked before any copy |
| Movies: an output token not found (`native_movie_output_unknown`, 3 sites), the Bink import | the module's own registry and import | kept |
| Movies: the target's creation and the frame's collect (`native_movie_target_failed`, `_frame_failed`; the same catch's offscreen form `native_offscreen_view_failed`, met in `i111-q-1` and `-b-1`, fixed in I-0111) | catches of `std::exception` around a device resource and an allocation | kept: a device or memory failure, not a datum (a refused texture of the game's data skips since tranche 2) |
| Movies (UI): the movie viewport and the movie frame list (`native_ui_movie_viewport_contract`, `native_ui_movie_frame_count`, `_frame_null`) | the viewport's met case (the window not yet open) fixed in I-0113 tranche 19; what refuses now is an unreadable player or its viewport methods, and a movie list past 32 or holding a null: the engine's own UI manager | kept (the engine's objects the module calls into) |
| Catch-alls (`*_retirement_failed` and the like) | each wraps a whole admission or retirement of the module's registries | kept: a skip there would leave a registry half-changed |
| Material: the MTD bytes, name or shader string unreadable, the genuine parser's refusal, the parameter list unreadable (`material_cpu_bytes_unreadable`, `material_cpu_string_*`, the 11 `native_material_parameter_*`) | the game's MTD data; but a material the module leaves out is still in the engine's own repository, so its FLVERs do not meet DS3's absent-MTD case: `BuildModelMaterial` would give them the engine default MTD (its retry of D24390's), a stand-in (D-0197), and a layout built before would draw none of its meshes | **kept** (the final review's M1: a first form skipped the material and was withdrawn): a real skip needs `BuildModelMaterial` to build a FLVER material with no authored MTD, whose consumers (`08_model_programmes.inc`) assume one; never met (census) |
| Material: the input, identity and metadata contracts, the GPU association at retire, the catch | the module's pins and contracts; the catch is `bad_alloc` and the like | kept |
| Effect archive, shaders, gparams, textures (`effect_cpu_*`, `shader_*`, `gparam_*`, `texture_*`) | each admission answers the engine loader (`AdmitShader` and `AdmitEffect` return a bool, the others install the engine's fields); what the loaders do with a refusal is not read, and an effect half admitted holds a container and a parser | kept, the loaders' answer to a failure to read first; met once, in an October start-up build (census) |
| Cloth: the upload, the set's creation and release (`native_cloth_*_failed`) | catch-alls around a set half built in the engine's own vectors (objects and copies already written) | kept: the engine's release of a half-built set (23B7280) not read |
| Filters (`native_ui_filter_*`), the UI loader heap | the module's scopes and objects; the gradient kinds unreachable (tranche 5) | kept; a filter pass met once, 2026-10-11 (census) |
| The 12 `Unsupported*` stubs (15 tables) | each is a slot of an interface the module implements for the engine (scene context and view, model storage and materials, the UI shape context, image manager, mesh cache, shader policy, the HAL, filter objects): it fires only when the engine calls a method the module never wrote, and its return contract is unknown | kept: the module's contract, as WORKFLOW's « Skip the object » keeps them; when one fires, that method is written from the original (as the scene view's slots 1 to 8 were) |
| I-0101 L4 (a mode change's non-device failure), L7 (slot 6 of the material table, `05_selected_scene.inc`'s group and policy) | not changed; L4's refusal was met twice in October on the follow path, since fixed (above) | left as I-0101 left them |
| The depth of field's aperture other than the tap blurs (`yebis_dof.cpp`) | a refusal of the module's own option set | kept |

**Step 5's F14C52 path through 141B5F770** (I-0113's g24i, `i113-t37-fogwall9-native`, 2026-11-11; its crash report
`crashes/startup-crash1.txt`: an access violation at 140F14C52 under 1422BA280 < 1422B00E0 < 141B608D0 < 141B5F770, the journal ending
on `native_exit_sound_stop` then `native_exit_network_stop`). READ (a `ghidra-research` subagent, then each link checked by the pilot
with GhidraMCP, the excerpts kept in `evidence/I-0137/t6-fdp-ghidra.md`: the vtables 143AB0948 and 143AA7428, the constructors 141B5C040,
141B1E950 and 141B23550, the stop 141B5D6C0, the thunk 141B1F430, 140556ED0, 140C76AE0 and 140C81320): 141B5F770 is the
`DLKR::DLRunnable` loop of `FdpSv::RPCSystemImpl`, FromSoftware's online client's RPC system (its TCP and UDP request systems and
task system, `Nauru::Request` and `Nauru::System`); the client (`ClientLib::FdpClientImpl`) is at `[[14478ECC0]]` and its RPC system at
client+0x60; the loop ends only on the byte +0x431, set by the class's stop (slot 2, 141B5D6C0), which only the game's finalizer
reaches (140EDE1F0 > 140556ED0 > 140C76AE0 > 140C81320 > the client's slot 2 > 141B23BD0). The faulting stack is that thread's
UDP update (1422B00E0 > 1422BA280) reaching a `_purecall`. INFERRED: the I-0124 kind: a quit that does not reach the finalizer leaves
the thread polling into the CRT's exit chain, whose destroyed service it then calls. Fixed natively (`StopFdpRpcThread`, before the
network's stop in the SocketManager destructor's hook, after the sound's): only the thread part of DS3's stop (+0x431 set, the event
+0x438 set on its handle, the thread awaited at most 2 s on its handle); nothing freed, no call into DS3's objects; one row `native_exit_fdp_stop`; `no-fdp-stop.flag` leaves it out.
Not proved by a run: the fault was seen once; the row's result in the next quits is the witness.

Not changed: step 1's left items (the selector without programmes, the refused packet and filter passes, the instanced layouts,
the text layer kinds, the HAL's unqualified methods): their reasons stand (« Tranche 5 »). The census met two of them, none after
2026-10-20: a UI packet the producer refused once (`native_ui_primitive_packet_status_6`, `i74-native-43-m45-display-cycle-i76c`,
2026-10-20, a display mode cycle), and the text layer kinds 1 and 7 (`native_cpu_text_layer_*_unavailable`, the last
`i66-native-2055-m40-schinese`, 2026-10-18).

### Tranche 7: the tour's UI overlay fault, and the D126B0 guard

On the same branch after tranche 6's merge. Two faults met in runs on 2026-11-13, each the module's closing of the game where the
original goes on.

**The world frame with no UI packet** (I-0139's tour, `i139-t2-b22-friede-nat` and `i139-t2-b05-watchers-nat`, seconds after the run's
F6 started each boss's second phase; I-0139's RESULT, on the tour's branch `claude/kind-antonelli-18b0e1` until it merges). OBSERVED in
both journals: `native_render_job_failure` « current native model UI overlay » on the render worker, then the refusal
`native_render_worker_faulted`; no `native_ui_display_changed` row; the frames' `native_model_draw_submitted` `draws` (the UI's draws)
fell from 82 (Friede) and 84-92 (the Abyss Watchers) to 1 for 38 and 35 frames before the faulting frame. READ: the message had two sources. The frame lifecycle (`02_frame_lifecycle.inc`) threw it
when a world frame came with no UI packet while an earlier UI build was current (inline, and in the worker's preparation): the rule
« never a world frame without its UI » since cR2. The renderer (`scene_pass_frame.cpp`, `RenderModels`) threw it when a frame's UI
packet did not match its frame. The worker's row cannot tell them apart; I-0139 attributes it to the first (INFERRED there too: a boss
cinematic hiding the UI, so the UI displayed no movie for that frame). INFERRED: the original draws that frame with whatever its UI
drew, nothing included (the native selects the UI's movies as E5B940 does and has no packet when none is displayed or none draws); the
renderer already draws a world frame with no UI (primitive generation 0, no stencil reset; the next packet initialises its stencil).
Now: the frame lifecycle presents such a frame without the overlay and counts it by cause in a row of its own,
`native_ui_overlay_absent` (its first four times and every 1,000th per cause), the cause set by the render step and read once a frame
(`native_ui_step_cause`): `no_movie` (the game's UI displayed nothing: the case the INFERRED reading covers; which cause the tour met is
not known, the code tells them apart only from now on), `movies_without_packet` (movies displayed, every draw skipped as the game's
data, I-0101) and `display_changed` (the posted display made for another frame or extent, its packet dropped: I-0132 R12f's choice);
the module's own states with no such reason, `no_ui_session` (no session or HAL) and `step_not_run` (the render step did not run for
that frame), still refuse as cR2's rule did (the integrity review). No gate reads the new row (the start check fails on refusals and
failures only); the renderer's
check is renamed « native model UI overlay mismatch », so a fault of that kind names itself (a message changed, stated here per
D-0268). Not proved by a run; no unit test drives a world frame without UI (the renderer's path is read, not exercised: the graph's
plan key without its UI bit is new in play).

**The D126B0 guard** (tranche 4's, for DS3's own fault at a window close during the character creation). Its witness
`i66-native-i137-t6-charmake` (build `p8e`, 2026-11-13 06:34-06:39, the ASUS, Alpha's copy, restored by hash; runner
`evidence/I-0137/series-p8.ps1`, log `series-p8.log`). OBSERVED: the pilot reached the creation (05:38:47 UTC) and closed the
window; two `native_body_scale_update_skipped` rows (the table null; rows at powers of two, so at least two skips), then an access
violation at D12759 through `NativeBodyScaleUpdate+0xC1` (the call to the original), r9 = 0x3EB294A93E8B8421: the table pointer was
other data; the update and the retirements (`native_retirement_caller`) on one thread, 28316 (the window's close on it INFERRED: no row
gives its thread). READ (`evidence/I-0137/t7-bodyscale-ghidra.md`,
the final review's byte reading for the second caller): a modifier's +0x58 is an interior pointer into a `FlverResCap` node's
`FlverModel` block, `[node+0x68] + 0xC8`, the second argument of its constructor D12370, whose two callers take the c0000 node (the menu
model D690A0 and the cutscene actor `SprjRemoAnimAct`'s slot 8, 140CC2AD0); the block is freed only in the node's destructor body
D23910, after the node has left the repository's name map (1423563D0's unlink), and [node+0x68] is then cleared (the order also
ledger K-0201, CONFIRMED; « only » INFERRED from the code readings, D23910 the single function that frees the FlverModel block found by
the reviews); so the block was freed and reused, and no read through the modifier can tell. Now: the module records each modifier's
block at its construction (a hook on D12370), marks it released when D23910 frees the block that holds it (a hook on D23910, its return
passed through) and forgets it at the modifier's destructor D123F0 (a native hook now, its teardown row behind `update-observe.flag`);
the class's four slots that read the table (4, 5, 6 and 7: D12930, D124B0, D12AA0, D126B0, each `[[modifier+0x58]+0x18]` indexed by
the mapped bone, READ) skip a released modifier, or one whose table is null (rows `native_body_scale_update_skipped` with the slot):
a skipped call runs only what the original runs before its own scaling, D1A7D0, D1A830, D1A790 or D1A870 on +0x38 with the same
arguments (the parent location's same slot when there is one: READ in the bytes, `call D1A000; ... jmp [rax+slot]` with rdx and r8 forwarded,
and r9 for slots 6 and 7), and returns its count; only this modifier's body scale is left out. **A defect of tranche 4 found here**: slots 6 and 7 take
four arguments, (this, matrices, count, first) (READ: D126B0 and D12AA0 keep r8d and r9d live into D1A7D0 / D1A870; the base's slot 7,
D1A420, loops `count` bones from `first`), and the module's detours of slot 7 declared two: the Reference witness `ObserveBodyScaleUpdate`
since `2d67d08a` (2026-10-10, the 1173 RCA; hooked unconditionally in the Reference mode, natively behind `update-observe.flag`) and
tranche 4's native guard since its merge (`50b89d24`, 2026-11-12). So, INFERRED from the source (the binary at the call not read), the
original was called with r8 and r9 left by the module's own code, and the guard's skip with rdx too. READ: a modifier without a parent
location scales nothing (D1A7D0 returns 0, D126B0 skips its loop), so every update that applies a body scale has a parent, and the
parent's slot then looped an unbounded `count` from a junk `first`, writing 0x30-byte matrices through rdx: a possible out-of-bounds
write, on every effective update (the character creation, the cutscenes' player actors). Its effect on the picture and on memory since
`50b89d24` (and in the Reference mode since 2026-10-10) was not checked; no journal was searched for it. The witness
`i66-native-i137-t6-charmake` (build `p8e`) ran with the defect: its faulting instruction loaded r9 from memory (`[[rdi+0x58]+0x18]`),
not from the clobbered argument, which with the two null-table rows before supports « freed and reused », without ruling out damage
to that block by the defective forward. Suspect therefore, until taken again or argued harmless: the Reference mode's posed characters
with such modifiers since 2026-10-10 (the creation's preview, the cutscenes' player; the 1173 verdict stands:
`i66-reference-charmakeclose-1173` faulted without the witness), and the native runs with a body-scale update since 2026-11-12. Every detour now forwards every argument (slots 4 and 5: (this, matrix, bone); the Reference witness of slot 7 too). Only the module's own state is read, under its own lock (a first form looked the c0000 node up in the repository at every update: withdrawn after the
reviews, the cutscene actors making that an unlocked walk of the engine's map every frame). Left: a release on another thread between
the check and the original's read is not covered (one thread in the witness). Not proved by a run: the next CharMake witness.

**The guard's witness** `i66-native-i137-t7-charmake` (build `p9g`, the merged tree; 2026-11-13 09:03-09:08, the ASUS, Alpha's copy,
restored by hash; `series-p8.ps1` pointed at `p9g`, its log `series-p8-t7.log`). OBSERVED: the creation reached (08:08:16 UTC), the
window closed; `native_body_scale_update_skipped` rows with `released` true (counts 1, 2 and 4: the record caught the freed block);
then an access violation at `DarkSoulsIII.exe+0x12842DD` (FUN_1412843C0, under 141285230 < 141281A86 < 14081EB20 < 14081C2E0 <
140D68520, the menu model renderer's update, < the same update task 1423C1D40; rax 0 in `exception_registers`; the instruction not
decoded), on the engine thread 33136, the same as the retirements. INFERRED: the next reader of the same released c0000 data
(a Havok-side object of the menu model, `FlverModel`-linked), the D126B0 guard having worked. This is the **second failed local fix**
of DS3's own fault at the window close during the creation (tranche 4's guard, then this one): by the rule (CLAUDE.md, WORKFLOW
« Validation and RCA ») no third guard; an RCA is opened instead on the teardown order at that close (the menu model renderers retired
after the c0000 resource while their update task still runs; I-0066's 1172 and 1173 teardown witnesses the starting point), ROADMAP
workstream 4 step 5.

Builds `evidence/I-0137/build-p9a` (the overlay fix), `build-p9b` (the first guard, main merged in), `build-p9c` (the reviews'
answers, main merged in again), `build-p9d` (the second reviews' answers), `build-p9e` (the arguments forwarded), `build-p9f` (the module's own states refusing again) and `build-p9g` (main
`8efdb410` merged in: the tree that merges; every unit test passed, no warning).

### Tranche 8: the RCA of DS3's fault at the window close during the creation

On the same branch from `main` `2efc16e1` (the coordinator's order of 2026-11-13 13:56, while the fresh E thread waits for Thomas).
Two local guards had failed in turn (tranche 4's on the table pointer, tranche 7's on the module's record of the block): each let the
next reader of the released c0000 data fault (« Tranche 7 », the guard's witness). So, before any third change, the cause.

Observations (no new run):
- I-0066's 1172 and 1173 teardown witnesses (MENU-OFFSCREEN-MODELS « Corrected (1173) »), both modes: the title step leaves its
  state 9 for 10 only once the main loop's quit request is set, then its dialog dies and with it the c0000 resources; the creation's
  views (and their modifiers) are never destroyed before the step shuts down and stay in the update tasks; the original faults too
  (`i66-reference-charmakeclose-1173`, `i66-reference-bodyscale-1173`).
- `i66-native-i137-t7-charmake` (tranche 7's witness): the c0000 textures unbound for some frames (`native_texture_lookup_unbound`,
  FC_M_0000), the D126B0 guard's skips with `released` true, a view unlisted (`native_offscreen_listing` « unlist »), then the fault
  in the menu model renderer's update D68520 (> 14081C2E0 > 14081EB20 > 141281A86 > 141285230 > 1412843C0 > 1412842DD; the read
  address 0x20, rax 0), on the engine thread.

Reading (GhidraMCP, then the step review in the executable's bytes): the main loop's tick EDAAE0 sets the byte 14489D5B8 once the
window's alive check fails (`D5B8 |= !(window->vt+0x10() && !runtime->vt+0x98())`, the window `CSWindowImp`, the runtime's slot
141788CD0 constant false in this build); its only writer is that OR, never cleared (the step review's search of the code sections);
its readers the title step, the ending credits and a few others. D68520 is the method of the renderer's update task
(`SprjEzUpdateTask<SprjEzRabbitTask,CSMenuAsmModelRend>` at renderer+0xC0, set by the constructor D65DD0, called through the task's
slot +0x18 under 1423C1D40, which discards its result): it runs the renderer's slots +0xA8 (its FD4 step machine D66B80) and +0xB0
(empty), then, with its ChrAsm model (+0x6E8), the animation object +0x810 made from `FUN_140e03520(_DAT_1447965b8, L"c0000")`
(D690A0) and its parts, and ends with `[this+0xD8] = 1` (no reader of +0xD8 found: its meaning unknown).

Cause (INFERRED from the two, the order not re-witnessed): at a window close the quit request comes first; the title step's exit
releases the c0000 resources (the FLVER cap through its holders, and the animation's own resource, `FUN_140e03520`'s repository), and
the update tasks go on running the creation's renderers and modifiers on the released data in the following frames. The faulting
reader is whichever of them comes first after the release: D126B0 before tranche 7, D68520 after it; « the following frames » are the
main loop's post-loop passes (F14BB0 runs EDA940 at most 100 times after the loop ends, the faulting stacks ending at EDA9BF).

The change: three readers of the menus' models gated on DS3's quit request, latched once seen (not a change of the teardown order
itself, which stays DS3's): the renderer's update `NativeMenuModelUpdate` (D68520) is skipped whole and sets `+0xD8` as the
original's end does (a row `native_menu_model_update_skipped` the first time); the body-scale slots skip (their row's `quit` apart
from `released`, tranche 7's record); the module's own collect of the menu views, `CollectNativeOffscreenScenes`, collects none (the
last scene of each view is still drawn from the module's own snapshot until the view retires: no engine memory read). After the quit
this covers every body-scale modifier (the cutscenes' player actors too) and every menu model renderer, whatever the screen. The cost:
the menus' step machines no longer advance during the post-loop (bounded by F14BB0's 100 passes; their count against the original's
not measured). A first form ran the renderer's step slots: the final review found +0xA8 called without the context D66B80 forwards
to its children, and those states reaching the ChrAsm model (D677D0, D67730, D678C0): withdrawn.
Left: the body-scale gate's quit path not reached in either witness; the menus' step machines frozen in the post-loop (its pass
count against the original's not read); the order itself
(quit request, title step's exit, c0000's release, the renderers' updates) re-witnessed with timestamps. I-0066's « Native parity: no
fix » (MENU-OFFSCREEN-MODELS, 1173) is superseded by tranches 4, 7 and 8 (a fault of the original's, addressed natively as I-0124's;
witnessed once, below).

**Its witness** `i66-native-i137-t8-charmake` (build `p10b`, the first form above, with the step slots and the dropped context;
2026-11-13 14:57-15:02, the ASUS, Alpha's copy, restored by hash; log `series-p8-t8.log`). OBSERVED: the creation reached (14:01:35
UTC), the window closed, `native_menu_model_update_skipped` « quit_requested », the three exit stops « signalled », **exit 0, no
fault, no dump**: the first clean close during the creation in a native run (tranche 4's and 7's witnesses faulted). One run, with
the first form; the tree that merges skips D68520 whole: its own witness below.

**The merged form's witness** `i66-native-i137-t8d-charmake` (build `p10d`, the tree that merged; 2026-11-13 15:55-16:00, the ASUS,
Alpha's copy, restored by hash; log `series-p8-t8d.log`). OBSERVED: the creation reached (14:59:40 UTC), the window closed,
`native_menu_model_update_skipped` « quit_requested », the three exit stops « signalled », **exit 0, no fault, no dump** (no
`native_body_scale_update_skipped` row: the body-scale gate not reached). The post-loop's pass count not read.

Builds `evidence/I-0137/build-p10a` (the first form), `build-p10b` (the step review's answers; every unit test passed, no warning)
`build-p10c` (the final review's answers) and `build-p10d` (main `a651528c` merged in: the tree that merges; every unit test
passed, no warning).
- Tranche 8, the step review (a fresh Opus at high effort, `main...66da1191`, the executable's bytes): nothing High or Medium; the RCA's
  conclusion and the code hold. L1 D68520 is the update task's method, not a vtable slot, and its result is discarded by its caller
  (the READ corrected, the parameter renamed `context`); L2 the tick's expression completed and the writer search recorded; L3 the
  skip stopped the renderer's step machine too: its step slots now run; L4 the module's own reader of the menu views not gated: now
  gated; L5 +0xD8's meaning unknown (said); L6 the builds named, the 1173 record's « no fix » marked superseded, the post-loop passes
  named.
- Tranche 8, the final review (a fresh Opus at xhigh, `main...f8e8983e`, the executable's and `p10b`'s bytes): **H1** the step slot +0xA8
  called without the context D66B80 forwards to its children (the arity defect again): D68520 now skipped whole; **M1** three readers
  gated, not the teardown order, and the steps reach the model: worded so, the steps left out; **M2** « fixed » with no run: « addressed »,
  then the witness above; L1 the row's `quit` apart; L2 the byte latched; L3 the last scenes still drawn (said); L4 the records (the 1173
  record's tick expression, STATE and ROADMAP at the merge, p10b's outcome).
- Tranche 8, the integrity review (a fresh Opus, the same range): nothing High; the judge untouched, the gate a mechanism; **M** « fixed »
  with no witness (answered as M2 above); L the row's two causes (answered: `quit`), L the scope wider than said (all body-scale modifiers
  after the quit: said), L the collect's skip without a row (left: the menu renderer's row marks the quit), L READ claims resting on the
  reviewers' byte readings without a stored note (said here: the step and final reviews' llvm-objdump and dumpbin readings, not stored).
- Tranche 8, the step review of those answers (a fresh Opus at high effort, `f8e8983e..0bc3c496`, the executable's bytes): nothing High or
  Medium; no call into DS3 left with a wrong arity, the latch safe, the row's buffer enough, the witness's account matching (it ran
  `p10b`, the first form). L1 and L2 the witness's form said in the 1173 record and the Conclusion; L3 « Left » brought up to date; L4
  the body-scale gate not reached in the witness (said).

## Closing (2026-11-13)

ROADMAP workstream 4 ends with I-0137: tranches 1 to 8 merged, each with its start check passed. What stays, each with its reason and
what reopens it:

| Item | Status | Reason, evidence | Reopened by |
|---|---|---|---|
| Step 1: a selector with no programme pair (the catalogue's 486 unavailable recipes of 1,316) | left, a refusal | INFERRED by tranche 5's final review: the planar video families, which no native image takes (formats 1, 2, 9, 50 to 53), so unreached; never met (the census, `t6-refusal-census.md`) | a `native_ui_primitive_shader_status_*` refusal in a journal |
| Step 1: a draw's packet the producer refuses | left, a refusal | a skip needs the producer to give back the frame's lent payloads (tranche 5's step review H1); met once, 2026-10-20 (`i74-native-43-m45-display-cycle-i76c`, a display mode cycle), none since | a `native_ui_primitive_packet_status_*` refusal |
| Step 1: a filter pass without programmes or layout | left, a refusal | the subtree's unfiltered fallback composites with the filter's own last pass (`RunFilterPasses`), so a missing pass programme cannot fall back through it (tranche 5's M4); met in October before its programmes were admitted (`native_ui_filter_pass_shader_status_1_key_220000`, 2026-10-11), none since | the same refusal |
| Step 1: an instanced UI vertex layout (HAL+0x39F18 > 1), a text layer kind the mesh writer does not generate (kinds 8, 9 and above 11), the HAL's never-qualified methods | left, refusals | each needs its engine caller read first (what DS3 does with an unmapped format, which mesh the writer must still return, each method's return contract); the text layers 1 and 7 met in October were qualified since; the HAL's methods met only in the October start-up builds | the same refusals |
| Step 2: the `Unsupported*` stubs, the asset loaders, the cloth, the movie catches, an unreadable MTD | kept, refusals | the module's own interfaces and contracts, or (the MTD) a skip that would draw a stand-in (« Tranche 6 ») | a refusal met in a journal |
| Step 5: the D126B0 and D68520 readers at the creation's close | addressed | two clean witnesses (« Tranche 8 »); the teardown order itself not re-witnessed | a fault at a close in the creation |
| Step 5: `DarkSoulsIII.exe+0xF14C52` through 141B5F770 | addressed | the RPC thread's stop « signalled » at the quits read since (the start checks and the witnesses); the fault itself not met since | the fault again |
| Tranche 4's, 6's and 7's cases not proved by a run | open witnesses | listed in « Uncertainty » | a run that meets them |

None of the left items has been met in a native journal since 2026-10-20. They go to ROADMAP workstream 16 (the complete review), to
be taken when a run meets one.

## Evidence

- Tranche 2: `native/frame/game_bootstrap/14_scene_step_and_storage.inc`, `05_selected_scene.inc`, `09_model_admission.inc`,
  `08_model_programmes.inc`, `06_pass_programmes.inc`, `20_startup_phases.inc`, `native/renderer/backend.cpp`,
  `native/frame/game_ui_mesh_owner.cpp`, `native/frame/frame_presenter_test.cpp`; the journals' counts by a `measure-tools`
  subagent (the 17 journals named above); the originals 14002B260, 1400AC110, 140E49320 and 140E48980 read through GhidraMCP.
- The original's functions as read: [ORIGINALS](ORIGINALS.md).
- `native/frame/original_image.h`, `original_image_test.cpp`, `game_bootstrap_ctl.cpp`, `Build-GameBootstrap.ps1` (the pin
  check, the test's build and run), `tools/Find-DS3Game.ps1`, the two launchers.
- The builds: `evidence/I-0137/build-a1`, `build-p4a`, `build-p4c`, `build-p4f` (each with its `.log` beside it; « Reviews »).
- Tranche 6: the census `evidence/I-0137/t6-refusal-census.md` (its script `.py` beside it), the walk's largest
  `t6-walk-max.txt` (and `.py`), the F14C52 layout's excerpts `t6-fdp-ghidra.md`, the witness runner `series-p8.ps1`, the builds
  `build-p8a` to `build-p8e` (each with its `.log`).

## Reviews

- Tranche 1, the step review (a fresh Opus at high effort, `8e81341e..f5a162e6`): nothing High; criteria 1, 2, 3 and 5 met (the
  bounds, the build's regex and the three header offsets checked; no slower before the injection than the two opens of
  `equivalent`). M1 the finder threw for a library on a drive that is not there (`Join-Path`), ending the launchers before their
  message: paths by `[IO.Path]::Combine`, each library and list under its own catch, the self-test with such a library before the
  game's (fixed). L1 the path past MAX_PATH never tested (T7 added), the open of a running image and the network long path not
  said (now in « Not checked »); L2 the RESULT credited the hooks with an identity check they do not make (reworded); L3 the test
  left its sparse file behind on a failure (removed before the exit; its end checks nothing is left); L4 the finder's self-test
  run by nothing (now by the build); L5 a stale Steam folder in the registry hid the right one (every folder the registry names
  is tried).
- Tranche 2, the step review (a fresh Opus at high effort, `8a72b2d8..178976ec`): criteria met for A3, B6, C2, A2 and G6, G5
  (no half-written wrapper, no leak of the imported image, the only producer of the records' epochs, every skip reason a
  literal). **H1** C4's replacement of the kept set could close the game (a change of set inside a frame's list): C4 rewritten,
  the set published once after a bounded wait (above). **M1** frame generation on a Present whose back buffers are stand-ins of
  another size than the chain: off on stand-ins. L1 and L2 (the first C4's rate limit and its count of members) gone with the
  rewrite; L3 `Resize`'s same-size shortcut redundant and sometimes wrong (removed: `TryAcquire` checks the size); L5 two
  comments (corrected; `scene_pass_lighting.cpp`'s « the programmes resolve once per process » is true again); L4, L6, L7
  recorded under « Not checked ». Build `evidence/I-0137/build-p4a` (the merged tree before these answers, 09:13-09:24): every
  unit test passed, `frame_presenter_test` on WARP and on the hardware.
  A2's part of that review (« met ») was of the first form, withdrawn after the integrity review.
- Tranches 1 and 2, the integrity review (a fresh Opus, `main...a7c49da9`): no test, gate, reader or expectation touched; the
  four declared changes found stated truthfully; the new tests cannot pass vacuously (but T4). **H1** the RESULT said priority
  4's cases were never met, yet A2's is met at every character creation (134 placeholder rows in 54 journals outside the 17
  read): A2 withdrawn, the placeholder back as it was, the claims rewritten (« What the runs show »). **M1** E3's case was met
  in `i101-fs-alttab-1` (recorded). **M2** a decoder's catch-all (an allocation failure included) became a skip: `fatal` set for
  `bad_alloc` and foreign exceptions, which refuse again. **M3** the UI import's 0 covered the module's own states: the import
  now says why and only the data's reasons skip. L1 two more changed rows (declared; the placeholder's row is back); L2 the
  tranche 1 list pointed at the wrong build (corrected); L3 T4 did not check DXGI's refusal (it does now) and the E3 test had
  run on the earlier `Resize` (it ran on the present one in `build-p4c`); L4 the lighting set's « never met » partly inferred
  (said: the mask has no row of its own); L5 « four rows a session » (4 journals of 17); L6 the originals' reads not in the
  repository ([ORIGINALS](ORIGINALS.md) added).
- Tranches 1 and 2, the final review (a fresh Opus at xhigh, `main...e7c539f6`, the tree after the integrity review's answers):
  nothing High; criteria met for A1, A3, B6, C2, C4, E3, G5, G6 (A2 left with its reason), no normal-path regression found, the
  merges with main (I-0135, R12h) intact, no D3D11 path, no missing barrier, the originals' reads matching the mirrors'
  decompiles of 140E49320 and 140E48980. **M1** this section did not record the integrity review and called finished work
  pending (rewritten). L1 C4's wait clock read before its lock could wrap the wait and print a wrong `waited_ms`, and its
  members' total was a literal (the clock under the lock, the total from the arrays' sizes); L2 the frame start still asked
  again for a set that no longer changes (asked until the first set); L3 G5 removed a guard with nothing measuring what it
  guarded (a row `native_ui_mesh_items` at each power of two from 4,096 live items); L4 a full UI image set built and released
  an image at each refused import (said before anything is made; the imports that hold no bytes still count toward the bound:
  left); L5 C2 re-admits every record at an option change, not the water's alone (left, the trade-off stated above); L6 an
  unreadable authored constant is counted among the skipped objects though its texture is admitted (left: its reason names the
  constant); L7 a refused publication by the engine was reported as « no session » (named); L8 the image file's open could
  fail if another process held it for writing (`FILE_SHARE_WRITE` added); L9 stale records (the index row, `status.json`, the
  Evidence line: corrected).
- The step review of the final review's answers (a fresh Opus at high effort, `e7c539f6..75ddf59b`, the native files): the
  clock under the lock, the frame start's asking, the full set said first, the named publication and the share mode met their
  criteria. M1 the mesh items' witness wrote a row at every allocation while the live count sat on a power of two (60 rows a
  second under churn): it now fires once per power of two for a cache (`items_witnessed`). Its note that the allow-list called
  `reported` « guarded by the mutex » while it is an interlocked counter: the entry's text corrected. Not done: a test of the
  image file opened while another process holds it for writing.
- Builds: `evidence/I-0137/build-p4a` (the merged tree before the step review's answers, 09:13-09:24) and `build-p4c` (with
  them and R12h merged in, 09:28-09:41): every unit test passed, `frame_presenter_test` on WARP and on the hardware. The tree
  that merges (every review answered, `main` `28c39a9a` merged in) is `build-p4f` (09:57-10:07): every unit test passed (124 PASS lines, no failure), the new tests included.

- Tranche 3, the integrity review (a fresh Opus, `main...6dc18c43`): no test, gate, runner list or expectation weakened, the test
  flag fully withdrawn, C7's test unchanged, the start check's and the build's figures as recorded. **M1** the witness had met the
  movie frame texture after the movie's close, not a menu proxy, and the record said the opposite: A2 limited to the movie frame
  (the proxy keeps its stand-in), the record rewritten. **M2** no still covers the transition's frames where the stand-in or the 0
  was used (said). L1 three of the four native face runs of tranche 35, not all, crashed at the same place (corrected); L2 the
  rows' counts explained and the ffmpeg command named (its figures not kept); L3 E4's dropped picture leaves no journal row
  (stated); the changed placeholder row declared.
- Tranche 3, the final review (a fresh Opus at xhigh, `main...0048dd45`): nothing High; criteria met for A7, A8, A9, B11, C7, D7,
  E5, H6, B10, F7, the thread safety and the mid-line comments (none left); the ORIGINALS claim checked (the second reference
  to 140E49320 falls in `.pdata`: one real caller); nothing else relied on the movie stand-in; the ffmpeg figures rerun and
  matching. **M1** STATE, the Conclusion and the index said A2 removed or not corrected where the code keeps the proxy's
  stand-in (rewritten: A2 in part). L1 the reason to keep the proxy's stand-in rested on old journals (its classification of
  every placeholder row recorded above; the pending witness narrowed to the Status page); L2 the witness's labels (INFERRED
  where the rows carry no class; « asked again » backed by the per-frame imports; the credits' before-open case named);
  L3 F5 listed as corrected (moved to « left » with its true reason); L4 F8's flag clause (dropped); L5 the proxy's comment
  (corrected); L6 B9's count mismatch recorded as 0 (now 14); L7 H4 printed an unidentified word (+0x78 no longer read); L8 A7
  copied before checking the close (checked first too; the storage admission's sibling refusal recorded as left).- Builds of tranche 3: `evidence/I-0137/build-p5a` (the first part), `build-p5b` (with the witness flag, the run's build),
  `build-p5c` (A2 first form), `build-p5d` (the integrity review's answers): every unit test passed. The tree that merges (the final review's answers, `main` `f559ce6e` merged in): `build-p5f` (11:32-11:44), 124 PASS lines, no failure.
- Tranche 4, the step review (a fresh Opus at high effort, `main...83f094db`): nothing High; criteria 4 and 5 met (nothing removed has
  a caller left, no mid-line comment swallows code). **M1** `Test-ThreadAffinity.ps1` still listed the removed `RenderNativeModels`
  and failed `Test-Project` (removed from its scope). **M2** B5's snow stamp missed the snow inputs' presence: a row built on a
  frame without them would keep a wrong FC_SnowRate once they came (their presence back in the epoch, as the velocity's). L1 D3's
  handshake could deadlock on a nested use by one thread (a thread-local depth lets it in); L2 the flag held for a whole record
  when nothing was named (cleared then); L3 E602F0's signature not labelled (READ: two arguments; the guard since withdrawn); L4
  the D126B0 guard's predicate wider than the close, its return value not labelled (READ in the decompile; one row before and one
  after the title step's end); L5 the five allow-list entries outside the checker's scope (removed: these statics are each under
  their function's mutex or `composed_effect_lock`, as read by the review).
- Builds of tranche 4: `evidence/I-0137/build-p6a` (the first commit), `build-p6b` (with A7's sibling, F5 and the two guards) and
  `build-p6c` (`5abafa08`: the step review's answers, the proxy's stand-in removed, the kept thread): every unit test passed, 124
  PASS lines each.
- Tranche 4, the integrity review (a fresh Opus, `main...3c642243`): no test, check or reader weakened (the affinity scope's removed
  name, the moved test fixture, F5's pins and the allow-list checked). **H1** the roadmap's « unattributed F14C52 » was closed on the
  tour's Reference occurrence (I-0124's path) while I-0113's native occurrence after I-0124's fix goes through another thread: that
  one recorded open (its row above). M1 the Conclusion still said « guarded » (rewritten); M2 the runner cited had been edited for
  the next pair (the first pair's parameters written out); M3 `-StatusPage` and its hold named; L1 the 12 targets a capped count;
  L2 two inferences labelled; L3 five entries, not six; L4 `build-p6c` cited; L5 a stale comment promising the stand-in (corrected,
  and the second case it served named).

- Tranche 4, the final review (a fresh Opus at xhigh, `main...c5cc5902`, main merged in): nothing High; criteria 1, 2 and 4 met,
  the main merge intact. **M1** C6 asked every absent programme again at each registry move, under the layout's lock (now only
  after a shader admission or retirement, or a second after the last asking; the RESULT's C6 row said « only while », true only
  when the choice is asked again: reworded). **M2** the D126B0 guard's before/after split read a flag only a witness hook sets
  (now a count row at each power of two). L1 D3's two waits unbounded (61950's wait bounded at 2 s, then as before D3 with a row
  `native_sh_record_set_wait_timeout`; Execute's own wait kept unbounded: its step review's H1 showed that naming nothing lets the
  GPU particles read the null member, Native1975). L2 the D126B0 guard narrows the race, said. L3 the 15 s bound's comment. L4 old gparam banks held
  by the snow stamps (noted). L5 D4 a failed key's builders run again per model (remembered). L6 stale records: the D126B0
  comment, two comments on one line in `rt_scene.cpp`, the CONTRACT (step 5 now in scope), the ROADMAP's step 5, the index row,
  `status.json`, this section and A4's reason (corrected).
- The step review of the final review's answers (a fresh Opus at high effort, `c5cc5902..879d9460`): criteria met for C6, the
  D126B0 count, D4 and the records. **H1** Execute's 100 ms bound named nothing and let the GPU particles read the null member
  (reverted: its wait unbounded, as before). **M1** 61950's 100 ms bound reopened the pre-D3 fault and its count was never written
  (2 s, a row at each power of two). L1 the C6 row (« as last asked »); L2 C6's window a second (as specified).
- Builds: `evidence/I-0137/build-p6d` (`c5cc5902`, main merged in, 14:14-14:26): every unit test passed, 124 PASS lines;
  `build-p6e` (`879d9460`) and `build-p6f` (`7d4d91df`, that step review's answers; the tree that merges, with only records and the
  CharMake pilot's wait after it): every unit test passed, 124 PASS lines each.
- The CharMake pilot (`Pilot-CharMakeWitness.ps1`): after the deletion the game showed the Rekindled title for about 212 s with no
  `01_920_Movie` row before `04_000_ChrMake_Top` (`i66-native-i137-t4-charmake2`'s journal), while its New Game clicks gave up at
  120 s: it now waits up to 300 s first (reported to the coordinator for R: this morning the movie's row came at once).

- Tranche 5, the step review of those answers (a fresh Opus at high effort, `4de7f5c7..ef5fabf1`): criteria 1 to 4 met, nothing
  High or Medium. L1 the contract checks ran after the data case's return (now before it); L2 `Begin` Missing marked as the data's
  (only Unsupported is now; Missing fails); L3 stale comments (corrected); L4 the display row (corrected); L5 two answers pointing at
  removed text (said moot); L6 the builds' labels.
- Tranche 5, the final review (a fresh Opus at xhigh, `main...4de7f5c7`): criteria 2, 3 and 4 met in the code, no regression
  against main. **H1** the selector without programmes was not skipped: the game's algorithm binds its image at row 20 after the
  selection, and the no-uniform descriptors' register -1 is refused there (withdrawn, the selection fails again, above). **M1** the
  status-keyed skip (the integrity review's H1, answered by `data_case`). L1 `RecordUiDraw`'s backstop sat after the uploads (now a
  failure: a skipped display's draw reaching it breaks the invisible-display invariant); L2 moot with the no-uniform descriptors
  gone; L3 `Skip`'s string per call (left: only on a skipped draw); L4 no unit test (left: the remaining case, the unsupported
  layout, is the bank's, met by none of the test's pairs); L5 the Conclusion (a tranche 5 line).
- Tranche 5, the integrity review (a fresh Opus, `main...4de7f5c7`): no test, assertion, gate or reader changed; tranche 4's
  covering start check verified (`start-9e45d6370`: exit 0, no fault, restored). **H1** the skip was keyed on the status value, so
  the native's own constants faults and a 3D view had become skips too (only the selection's data cases skip now, named). **M2**
  the CharMake pilot's change not recorded (below). **M3** in native mode its movie shots silently never ran (its waits now accept
  the Bink movie's `native_movie_open`, and a skip of the shots is printed). L4 the descriptors' encoding worded too broadly and
  uncited, and L5 UnknownSelector outside the 486 (both moot: withdrawn with the no-uniform descriptors after the final review); L6
  the merged code's build (`build-p7c`, below).
- The CharMake pilot (`Pilot-CharMakeWitness.ps1`, R's New Game run of 16:57, `i66-native-r12e-r1-newgame`): the opening movie is a
  Bink with a `native_movie_open` row and no `01_920_Movie` row; every wait for the movie or the creation accepts it now, and
  `-MovieShots` is skipped, with a line saying so, once the creation is already open (it had timed out on the absent row).
- Tranche 5, the step review (a fresh Opus at high effort, `1ff34627..cfaf74fc`): **H1** a refused packet lost the frame's lent
  payloads (the packet's refusal fails again); **M1** the « no uniform » descriptors were all zero, not the game's encoding (rows
  0xFFFF, triples {25, -1, 0}); **M2** the module's own contract statuses had become skips (only Missing and Unsupported skip now);
  **M3** a skipped display still prepared and uploaded its draws (now an invisible display); **M4** a skipped filter pass blanked
  the chain (withdrawn); L1 the gradient branch unreachable (withdrawn); L2 the writer's unread selector field (removed); L3 a
  stale comment; L4 the start check spoken of as done (reworded).
- Tranche 6, the step review (a fresh Opus at high effort, `57df4723..a3aabdf2`; without GhidraMCP, so the layout was judged against
  the RESULT): nothing High. **M1** the Fdp stop's wake called a request system's virtual slot unchecked, at a quit where those
  objects may be reached by the exit chain: removed; the stop now only sets +0x431 and the event (SetEvent on its handle +0x440, L1:
  1417F1B60 asserted on a null handle), the loop's waits being bounded at 100 ms. L2 the stop not guarded by a heap check as the
  sound's: moot now that it calls no DS3 code; the order of the destructor and the fault stays INFERRED (the row says when it ran).
  **L3** a skipped engine default MTD would have moved the refusal to the next FLVER whose MTD is absent (moot: the MTD skip was
  withdrawn after the final review). Left: L4 the menu offscreen view admits directly (no deferred list), where a runaway child list still
  refuses (`AdmitNativeModelCandidate`'s side effects not read); L5 the diagnostic totals (`native_disp_*`, `popin_gate(0)`) keep a
  skipped entity's increments (diagnostics only).
- Tranche 6, the integrity review (a fresh Opus, `main...175da558`): no test, assertion, gate or reader weakened; `no-fdp-stop.flag`
  an opt-out of the fix, stated; the census's counts re-counted (257 journals) and the walk's 2,108 confirmed (2,129 all-time).
  **M1** the census paragraph said none of step 2's stops fired after 2026-10-11, which the census contradicts (three since: L4's
  resize, the movie catch's offscreen form, the movie viewport, each fixed): rewritten with each run and its fix, also in STATE;
  **M2** the default-MTD guard read a name not yet read on the bytes' failure (moot: the MTD skip withdrawn); L1 the filter pass and
  the effect loader met in October (said); L2 the MTD skip's « never a refusal further on » not INFERRED (moot); L3 the F14C52
  layout's READ with no stored listing: the excerpts in `evidence/I-0137/t6-fdp-ghidra.md`, the crash report cited.
- Tranche 6, the final review (a fresh Opus at xhigh, `main...175da558`, the build `p8d` on that tree; without GhidraMCP, the Fdp
  layout checked by reading the executable's bytes: every offset, vtable slot and call of the RESULT matched): nothing High; criteria
  2, 3 and 5 met. **M1** a skipped MTD is still in the engine's repository, so its FLVERs would take the engine default MTD, a
  stand-in, not DS3's absent-MTD case: the MTD skip withdrawn (its row above says what a real one needs), and with it L1 (the bytes
  and name skipped before the default's guard), L2 (the parser's reader destroyed on a path the original never reaches) and L3 (a
  re-admission's incarnation). **M2** the census claim (as the integrity review's M1). L4 `no-fdp-stop.flag` added to
  `Run-StartupTrace.ps1`'s flags and its stale-flag cleanup; L5 the stale records (the Conclusion, the investigations index, STATE's
  Next action, ROADMAP's step 2) corrected; L6 the walk's two skip branches made one; L7 the walk figure stored
  (`t6-walk-max.txt`). Builds `evidence/I-0137/build-p8a` (the first form), `p8b` (the Fdp stop), `p8c` (the step review's answers),
  `p8d` (main merged in) and `p8e` (the final reviews' answers): every unit test passed.
- Tranche 6, the step review of those answers (a fresh Opus at high effort, `175da558..339562f8`): nothing High or Medium; the
  code answers as stated (the MTD file identical to main's, the walk's branch behaving as before, the runner's two lines). L1 a
  « none since October » left (now « none after 2026-10-20 »); L2 the filter pass and an earlier offscreen catch named in the census
  rows; L3 tranche 6's evidence listed under « Evidence ».

- Tranche 7, the step review (a fresh Opus at high effort, `main...07d2cd9d`) and the final review (a fresh Opus at xhigh, the same
  range; without GhidraMCP, DS3 read from the executable's bytes): nothing High; criterion 1 (a world frame without UI drawn correctly
  downstream) met by reading. **M1** (both): the guard's per-update lookup of the c0000 node rested on an unread release order (the step
  review) and on « one constructor call », false: the cutscene actor `SprjRemoAnimAct` (140CC2AD0) builds modifiers too, so the lookup
  was an unlocked walk of the engine's map every frame of a cutscene (the final review). Answered: the release order read (the node
  unlinked before its block is freed, `t7-bodyscale-ghidra.md`) and the guard rewritten on the module's own record (a hook on the
  constructor D12370 and on the node's destructor body D23910), no engine map read on the update path. L1 (both): one count for every
  cause of a missing packet, as a skip: now a row of its own by cause; L2 the row's `object` was the frame serial (gone with it); L3
  the renamed message's comment and the « + 200 » without its dereference (corrected); L4 the other-thread release caveat restored;
  L5 the overlay premise now INFERRED with its reason; L6 (step review) a replaced c0000 node skipping a live modifier (moot: no lookup
  now); the final review's L5 (main merged again, `2fd56ec1`, rebuilt `p9c`; I-0139's RESULT on the tour's branch, said) and L6 (no unit
  test of a world frame without UI, said above).
- Tranche 7, the step review of those answers (a fresh Opus at high effort, `07d2cd9d..dbe55318`, DS3 read from the executable's
  bytes) and the integrity review (a fresh Opus, `main...dbe55318`): nothing High; the detours' signatures, returns and installation met
  (D12370's five arguments and `this` returned; D23910's single caller, its slot 1). **M1** (step review): slots 4, 5 and 6 of the same
  class read the same table unguarded: now guarded alike. **M1** (integrity review) and L1 (step review): `no_movie` also took a
  display made for another frame or extent and a missing session or HAL: those causes named apart. **M2** (integrity review): D23910's
  reading had no stored excerpt: K-0201 cited, « only » INFERRED. L2 (step review): the cause stale on a frame the step did not run: read
  once a frame and reset (`step_not_run`). L3 (step review): entries of destroyed modifiers never removed: the destructor's hook now
  native. L4 (step review): a released record applied only while the modifier still points into that block. Integrity L1 (the close's
  thread INFERRED), L2 (the draws cited from the journals), L3 (this review of the rewritten guard recorded): answered in the text.
- Tranche 7, the step review of the second answers (a fresh Opus at high effort, `dbe55318..15c6f9a6`, the executable's bytes): **H1** a
  skip called D1A7D0 / D1A830 / D1A790 / D1A870 with `+0x38` alone, while each forwards rdx, r8 and r9 to the parent's same slot, which
  writes matrices through them; **H2** slots 6 and 7 take four arguments, the detours two (slot 7 since tranche 4): both fixed (every
  argument forwarded), the text above; M1 « changes no matrix » was false (corrected: the parent's transform is applied, this
  modifier's scale left out); criteria (b) the destructor's detour and (c) the cause logic met; L1 the build named by time only.
- Tranche 7, the step review of the arguments' fix (a fresh Opus at high effort, `15c6f9a6..45678f83`, the executable's bytes): the fix
  correct, criteria (a) to (d) met; **M1** the defect also in the Reference witness since 2026-10-10 (dated and the suspect Reference
  captures named, above); L1 two stale comments (D126B0's arguments, « changes no bone »: corrected); L2 r9 forwarded only by the slots
  6 and 7 forwarders (said). Comment-only changes after `build-p9e`: the binary is that build's.
- Tranche 7, the integrity review (a fresh Opus, `main...45678f83`, the executable's bytes): nothing High; the judge untouched, no scene
  special case, no measure cut; the journals' figures and the new READs confirmed. **M** the overlay refusal had been lowered for the
  module's own states too: `no_ui_session` and `step_not_run` refuse again (`build-p9f`), and no gate reads the new row (said). **M**
  tranche 4's defect: its reach (every effective update, a possible out-of-bounds write), the witness run with it and the unsearched
  journals now said. L the new READs stored (`t7-bodyscale-ghidra.md`, « The slots' arguments »); L « left by the module's own code »
  INFERRED; L the tour's cause not known (said).
- Tranche 7, the step review of the restored refusal (a fresh Opus at high effort, `19525737..1dc1113d`): nothing High or Medium; the
  two throws reach the old handlers (Clear's callers' catches, the worker's fault and `native_render_worker_faulted`); `step_not_run`
  only on frames where the old code threw too; L1 the Conclusion and the uncertainty (now say it), L2 the step's cadence INFERRED (said),
  L3 two cause names unreachable in the row (left: they keep the enum's order), L4 a comma.

## Uncertainty / unresolved questions

- A1: see « Not checked » above.
- Tranche 4, not proved by a run: the D126B0 guard (the creation's close: the CharMake pilot did not reach the creation twice);
  D3's handshake (no run met the interleaving); B5, D4, C5, C6, F4 (no visible change expected; no comparison run). Open:
  C6's composed groups, the left items of tranche 4.
- The window close during the character creation: addressed by tranche 8 (two clean witnesses, one run each form); the order of the
  teardown itself not re-witnessed (« Tranche 8 »).
- Tranche 6, not proved by a run: none of its cases was met in a journal (the census); the F14C52 stop's row « signalled » at the
  start check's quit, the fault itself not met since. Tranche 7, not proved by a run: the overlay-absent frame (the next Friede or
  Abyss Watchers run: a refusal « current native model UI overlay: no UI session » or « ...: render step not run » there would be one
  of the two causes the tranche keeps refusing, not a failure of the fix) and the D126B0 guard (the next CharMake witness). INFERRED: that
  the render step runs every world frame EndFrame runs (K-0180 and ENGINE-STEP-THREAD-MAP leave the scheduler's cadence unknown; the
  tour's journals show the UI's draws arriving a frame each up to the fault).

## Artifacts produced or modified

- `investigations/I-0137-robustness/` (CONTRACT, RESULT, status.json); the build under `evidence/I-0137/` (disposable).
