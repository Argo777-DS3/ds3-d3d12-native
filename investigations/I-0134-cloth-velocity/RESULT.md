# I-0134 - La vélocité du tissu simulé: result

Branch `claude/exciting-roentgen-508879`, from `main` `b5e0b48c`, 2026-11-12 from 00:10 (Paris). Question and scope: [CONTRACT](CONTRACT.md).

## Cause (READ)

- The game's cloth Velo vertex programmes (192 whose names hold `cloth`, `fxc /dumpbin` of the I-0063 cache; one output layout:
  SV_Position, TEXCOORD0 the current clip position, TEXCOORD1 the previous one, TEXCOORD2 the normal, TEXCOORD3 the texture
  coordinate) choose, by `movc` on `VC_isEnableCloth` (cbMatDynParam +0x80), `VC_World` (cbvModelParam rows 0-2) for the current
  position and `VC_PrevWorld` (rows 3-5) for the previous one, both applied to the same `POSITION` (stream 0, the current simulated
  copy). The motion of the cloth instance frame is carried, the simulated vertices' own motion is not: a cape swinging against its
  frame reads as still. The original has the same defect; the native renderer's TAA, DLSS, RR and NRD read these rows.
- Their pixel programmes (`GXFlver_Common_Velo` and 90 `_Velo_[A]`) read TEXCOORD0, TEXCOORD1 and, for the alpha-tested ones,
  TEXCOORD3; none reads TEXCOORD2.
- Three of the 192 have no cloth branch (`BurnCloth_Velo`, `BurnCloth_Velo_Skin` and `Eclipse_cloth_Velo`, skinned by the palette):
  they do not read `VC_isEnableCloth` and keep the game's programme.

## As built

- `NativeClothVertexUpload` (17_render_step.inc) keeps, with each new copy, the copy it replaced when that one was itself simulated
  (its resource and pooled version only: no chain) and was uploaded the frame before (a second upload in one frame keeps the
  replaced copy's own previous one, so the frame's whole motion), and stamps the copy, under the lock that pairs it, with the engine
  frames ended so far (`native_cloth_frames`, counted with `lifecycle_frames`); the admission binds the previous copy beside the
  current one (`NativeSelectedMesh::cloth_previous`) only while the current copy is this frame's upload (without a step this frame the
  cloth did not move against its frame, which the game's programme draws); `native_cloth_upload_age` counts the ages met, behind
  `cloth-velocity-observe.flag`.
- A Velo row (pass 0x34) of a mesh drawing its simulated copy whose produced `VC_isEnableCloth` word is 1 (the admission's
  1184 rule) takes `MakeNativeClothVelocitySet` (scene_pass_velocity.cpp): a native vertex programme with the game's outputs, the
  previous position from the previous copy (slot 2, `PREVPOSITION`, slot 0's layout) placed by `VC_PrevWorld`; the game's pixel
  programme kept. The set is made once per recipe (`NativeClothVelocityProgramSetOf`), only when the game's vertex programme reads
  `VC_isEnableCloth`, `VC_World`, `VC_PrevWorld`, `VC_MatrixWVP`, `VC_PrevMatrixWVP` and `VC_TexScroll_0` and writes the same
  outputs, and the layout has a float `POSITION0` in slot 0 and an integer `TEXCOORD0`; otherwise, and before a second upload, the
  game's programme stays. `ModelGeometryBinding::previous` (slot 2) is validated against slot 0's layout and bound only for a set
  that reads it.
- `no-cloth-velocity.flag` keeps the game's programme (the A/B; `Run-StartupTrace.ps1`'s flag list).
- Slot 2 needed the encoder's stream limit raised from two to three (`gbuffer_encoder.h`: its contract checks and its bind state).
- Journal rows: `native_cloth_velocity_set` (the first 24 recipes tried, with their family and whether the native set was made),
  `native_cloth_velocity_rows` (rows drawn with it).
- Test: `model_draw_test` « I-0134 »: a game-like cloth Velo programme and a Velo-like pixel programme on the debug layer; the native
  set draws the previous copy's motion ((32, 64, 191) where the game-like programme draws (96, 64, 191): `VC_PrevWorld`,
  `VC_PrevMatrixWVP` and the scroll's offset each shift the image), refuses slot 2 unbound, and is not made for a programme without
  the cloth branch, with `VC_PrevWorld` elsewhere, with other outputs, with a float texture coordinate or with POSITION outside
  slot 0. Against the game's own programme: `Build-GameBootstrap.ps1` passes the I-0063 programme cache to `model_draw_test` (a new
  second argument; without it the PASS line says « not checked »), which draws `SAT_Equip_cloth_Velo_Skin` (read in place, a missing
  file fails the test) on its whole input layout beside the native set: equal with the current copy as the previous one, the previous
  copy's motion with it. Not reached by a unit test: the age-0 pairing, the second upload in one frame, the gate on the produced word,
  the TEXCOORD2 refusal (code reading and the reviews only, the refusal itself now tested; the 91 Velo pixel programmes of the cloth families read no TEXCOORD2, the
  integrity review's parse).

## Reviews

- **Step review** (a fresh Opus at high effort, `b5e0b48c..eb6aaba6`): H1 the commit did not build (C4456 under /WX, a shadowed
  name): renamed; H2 every native cloth Velo draw would have thrown at encode (`BindGBufferDraw` took at most two streams): the
  encoder takes three; M1 the previous copy paired without a frame, so a cloth no longer uploading would repeat its motion and an
  upload per view would cancel it: the frame stamp and the age rule above, the ages counted for the run; L1 the first simulated copy
  paired with the authored one: only a simulated copy is kept; L2 the guard checked names only: the registers, offsets and sizes the
  native programme reads are checked; L3 the test's identity matrices: distinct ones; L4 a counter's name matched a reviewed static:
  renamed and reviewed (`thread-affinity-allow.json`); L5 the programme compiled under the recipe lock at first use (one small hitch,
  a throw caught once): kept, noted. Criteria 1 to 4 held as written; criterion 1 is contradicted by the run (the final review's
  H1, below).
- **Final review** (a fresh Opus at xhigh, `main...83249654`): **H1, not merged:** in the TAA runs' velocity pictures, the pixels
  inside the character's outline that no Velo row wrote (alpha 0, so camera motion) were 0.41 to 1.81 % before and 3.05 to 5.77 %
  after (8 pictures of 9; 0.43 % in the ninth; `evidence/I-0134/holes.py`, its output `holes.txt`), sharp-edged polygons over the cape and shoulders (`evidence/I-0134/run1/coverage-before-after-3150.png`,
  the review's crop; checked by eye on it). The journals' object pixels a frame (`medians.py`, `medians.txt`: the velocity rows of
  `startup.jsonl`, every 30th frame): medians TAA 137,555 before, 132,588 after; DLAA 137,825 before, 106,425 after, the DLAA drop
  in the run's first part (its first 40 rows 142,334 before, 99,760 after; its last 40 125,561 and 125,478). So the native rows cover less than the game's: their position, their texture coordinate (the
  `_Velo_[A]` programme discards below its alpha test, `SAT_Equip_cloth_Velo_[A]`: TEXCOORD3 x cbTextureTilingScale, t1's alpha
  against cbMtdParam and cbInstanceData's mulCol) or something not yet seen differs; the code reading found no difference (the
  game's cloth branch and G-buffer programme read, the depth bias unchanged). M1 a previous copy older than one frame: refused now
  (the stamps of both copies). M2, M3 the test and the guard compare against a restatement of the programme, not the real one: open
  (the diagnostics below first). M4 the RESULT's interpretations (one upload a frame, the idle sway) marked as such below. L1 the
  ages' counters behind `cloth-velocity-observe.flag`. L2 slot 2's buffer held through the stream owner only (the same
  `NativeClothStream`): kept, stated here. L3 the lock a row: kept. L4 the index row and status: updated.
- **H1 answered: not this branch's (a Fable 5.2 read, Thomas's yes 2026-11-12 02:54; OBSERVED on earlier runs, the mechanism
  INFERRED).** The same sharp-edged holes are in the velocity pictures of runs made before this branch with the game's programme
  alone and a character without cloth (`holes.txt`): `i132-r11j5-m40-taa-1920x1080` frames 1620 and 1890 to 2340, 18.0 to 20.5 % of
  the outline's pixels unwritten, frames 1530, 1710 and 1800 0.02 to 0.05 %; `i132-r11n2-launcher-m40-nrd-1920x1080` 1620 to 2340,
  17.8 to 21.5 %, 1530 0.02 %; `i132-r11j5-m30-taa-1920x1080` 1620 to 2340, 0.01 to 0.02 % (each run's first picture, at the
  arrival, 10.4 to 12.7 %),
  the camera still; and in this branch's after-run on the hat, head, shoulders and arms, drawn with the game's programme, while
  `velocity-2970` carries the cape's own motion with 0.43 % (checked by eye on the read's crops, kept as
  `evidence/I-0134/run1/fable-*.png`). The read compiled the native programme with `fxc`: its position and texture coordinate are
  the game's cloth branch instruction for instruction. Its mechanism (INFERRED, a float32 emulation with an assumed camera): the Velo
  rows project absolute world positions through an absolute view-projection (`12_scene_collect.inc` 265) where the G-buffer projects
  camera-relative (the comment of `13b_prepare_model_frame.inc` 547-550), so clip z and w carry rounding of a few hundred depth ulps at
  Firelink's coordinates against a constant bias of 64; flat triangles fail whole, sloped ones pass, and the failure flips between
  about 0 and 20 % with the matrix's low bits. A defect of the Velo rows since they exist, handed on (below); this branch's merge does
  not wait on it. The diagnostics written for H1 (the current copy or no copy as slot 2) were removed: they could not discriminate.
- **The read's other points:** a second upload in one frame paired its previous copy with that frame's first: now the replaced
  copy's own previous one (the last frame's), so the row carries the frame's whole motion (Medium, fixed); an upload-less frame (age 1)
  repeats the last step's motion once (Low, as stated in « Uncertainty »); `cloth_velocity_tried` set before the compile (Low, kept:
  a compile failure is a deterministic defect the unit tests catch); the guard checks reads and outputs, not the `movc` branch (Low:
  189 of 192 have it, the three without do not read `VC_isEnableCloth` and are refused). The final review's M2 answered with the
  game's own programme: `model_draw_test` now draws `SAT_Equip_cloth_Velo_Skin` from the I-0063 cache (read in place) beside the native
  set on its layout (equal without motion of its own; the previous copy's motion with it).

- **Integrity review** (D-0268, `main...a34b082f`): nothing High; it reran the numbers from the evidence (the journals' rows, the
  ages, `velstat.py`, the cache counts, samples of the holes): they match, except the medians, restated above with their reader.
  M1 the DLAA drop not addressed: stated as unexplained (« Uncertainty »), the after-runs' extra holes as INFERRED. Lows: the
  build script's new argument and the real-programme check named in « As built »; the changes no unit test reaches named there; the
  hole counts' reader kept (`holes.py`, `holes.txt`) and the m30 run named; the visual gains by eye on one run each (« Uncertainty »).
- **Second final review** (a fresh Opus at xhigh, `main...a3aac0d6`, the tree built as `build-b`): nothing High; H1's answer holds
  (it compiled the native programme with `fxc` and found the position, previous position and texture coordinate the game's cloth
  branch instruction for instruction; it counted the earlier runs' holes and this run's on the parts the game's programme draws);
  criteria 1 to 3 hold. M1 the handed-on defect had no item: STATE « Next action » now. L1 the previous copy at age 1 repeated the
  last step: age 0 only. L2 the zero normal unguarded: a pixel programme reading TEXCOORD2 refused. L3 the gate re-derived
  `VC_isEnableCloth`: the produced word read. L4 the stamp outside the pairing lock: inside. L5 two frame counters (kept: one is
  atomic for the other threads). L6 the RESULT's inaccuracies (the three programmes, the ages' flag, frame 1620, the tests' status):
  corrected. L7 the index rows: rewritten at the checkpoint. These fixes rebuilt into `build-b` (`build-c.log`).

- **Step review of the lows' fixes** (a fresh Opus at high effort, `a3aac0d6..063c2ec0`): criteria 1 to 5 hold; M1 the TEXCOORD2
  check looped over a member of a temporary `shared_ptr` (the shared interface freed before the loop under C++20 when the cache is
  cleared by another thread): held in a local; L1 two comments (the age-0 rule): reworded; L2 the refusal untested: a pixel
  programme reading TEXCOORD2 now refused in `model_draw_test`. Rebuilt into `build-b` (`build-d.log`).
- **Optimisation pass** (read): the native set is made once per recipe; the previous copy is the upload already made (no new copy,
  one more pooled version held a cloth object); the cloth Velo rows read one more vertex stream; the counters and the ages' histogram
  sit behind `cloth-velocity-observe.flag` except one interlocked counter a row; nothing else per frame.

## Runs

`series-i134-cape.ps1` (`evidence/I-0134/`, 02:07-02:18 on 2026-11-12, the ASUS 1920x1080 window, offline; `build-a`, built from
`4e49fed4`; a fresh copy of Thomas's Alpha with the cape at Firelink, `thomas-alpha-cape-20261111-124806`, SHA-256 2713CD6B…, restored
by hash after each run): four runs of one build, filmed (`-VideoKeys`: forward 1.2 s, back toward the camera, the half turns, a 12 s
hold): `i134-cape-taa-before` (`no-cloth-velocity`, `world-pictures`), `-taa-after` (`world-pictures`), `-dlaa-before`
(`dlss-dlaa`, `no-cloth-velocity`), `-dlaa-after` (`dlss-dlaa`). All closed clean (exit 0, no fault, 7,134 to 7,545 native frames,
restoration ok). Not a measuring run: one run a case, no noise floor.

- **The native programme serves the cape (OBSERVED, journals).** `native_cloth_velocity_set`: `SAT_Equip_cloth`, recipe 10, native
  in both after-runs; `native_cloth_velocity_rows` 9,000 rows (its last sampled count) in each; none in the before-runs (the flag).
- **The uploads' cadence (OBSERVED, `native_cloth_upload_age` at each run's last row):** taa-after 17,794 admissions of a copy of
  the same frame, 2 of the last frame, 200 older (left with the game's programme by the age rule), 4 without a previous copy;
  dlaa-after 17,762, 2, 232, 4; the before-runs 39,366, 18, 2,606, 10 and 17,758, 2, 236, 4. So one upload a frame, before the
  admission, about 99 % of the admissions in the after-runs (INFERRED: the ages count admissions, not uploads a cloth object); the older ones not identified (a cloth not simulated for a while, INFERRED).
- **The TAA (OBSERVED by eye, `run1/zoom-taa-1.9s-3.1s.png`: the films from 1.9 s to 3.1 s at 5 a second, the cape's crop
  brightened; before above, after below):** before, the cape swinging during the run turns translucent and its lower part fades into
  the ground (its 4th to 6th pictures); after, it stays opaque with its tatters sharp. The poses differ a little between the two
  films (one run each).
- **DLAA (OBSERVED by eye, `run1/zoom-dlaa-1.9s-3.1s.png`):** a smaller difference, the cape's tatters slightly sharper after; DLSS
  already holds the cape better without its motion.
- **The velocity pictures (OBSERVED, `velstat.py` on the TAA runs' `velocity-*.rgba`, 4 x the motion in 8 bits: 0.25 px steps; their
  frames do not match between the runs and none falls in the keys' seconds):** object pixels moving at least 0.5 px: before 13 to
  34,728 a picture at 0.5 px at most (the 2nd to 10th pictures; the 1st, at the arrival, 10 px), after 17,893 to 88,091 at 0.5 to
  1.25 px at most (the 2nd to 9th; the 1st 3.5 px). The cape's idle sway now carries its motion (INFERRED: the counts cover the whole character); a weak witness (coarse
  encoding, unmatched frames), and these pictures show H1's holes.

- **After the merge** (`5274abeb`; the launcher rebuilt 07:37-07:51, every unit test passed, `I-0134/launcher-5274abeb.log`; start
  check `start-5274abeb9` 07:52-07:53, the ASUS, offline, restored by hash): passed (359 world frames). The disk rotation (08:01) refused: another thread's compiler running; left to the next checkpoint.

## Uncertainty

- The pairing: the previous copy is the last frame's upload, the simulation's own step (I+0x90, `VC_PrevWorld`, is taken as the
  same step's frame: INFERRED, not read); the run's ages showed the uploads before the admission in about 99 % of the admissions.
- A mesh whose copy is bound with the material's cloth off (skinned by the palette from the copy) keeps the game's programme.
- Handed on: the Velo rows' depth precision (all Velo rows, the cloth's and the game's: the holes above), STATE « Next action »; the
  fix direction the read gave: a camera-relative Velo projection, or a bias sized from the camera's distance to the origin.
- Not run: NRD and the PT level with the native rows (the run was TAA and DLAA with RR); a live run of the code changes after the
  run (the pairing rules, the diagnostics' removal, the gate on the produced word): the unit tests, the game's own programme among
  them (`build-b.log` and `build-c.log`'s PASS lines, WARP and hardware), for what they reach (« As built »). The TAA film's comparison
  is one run each and, by the second final review's count, the after-run fell in the worse precision regime (more of the cape on
  camera motion; INFERRED: that the after-runs' extra holes are the precision regime and not the native rows is shown on the parts
  the game's programme draws and on earlier runs, not on the cape itself): against the improvement seen.
- Unexplained: the DLAA after-run's object pixels in its first part (99,760 against 142,334, the median of the first 40 rows); the
  DLAA runs have no velocity pictures. Its last 40 rows equal the before-run's.
- The visual gains are judged by eye on one run each (the stills kept), not measured.
