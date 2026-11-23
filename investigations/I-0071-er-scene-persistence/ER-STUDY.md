# I-0071 ER study: scene persistence, texture binding, distant fades (2026-10-20)

Elden Ring 2.7.1 (`eldenring_2.7.1.0_25080141_1A354710.exe`), static only: read-only headless Ghidra 12.1.3 on three copies of
the analysed project (`C:/Users/User/Tools/DS3-Decompilation/er-mirror-i0070-{a,b,c}`), no ER launch. Three Sonnet 5.5
studies (D-0195); their scripts and C/ASM exports are in the session scratchpad (`er-study-A/raw/b1..b26`, `er-study-B/raw/
e1..e31`, `er-study-C/exports/f1..f37`), not kept as evidence. Addresses are ER absolute VAs, never DS3 facts. Status: OBSERVED
(read in the code by the study), INFERRED (its reading). **Checked by Opus** marks the claims whose decompile Opus read.

## A. Per-model render data: what ER keeps from one frame to the next

- **No prepared draw per instance or mesh.** Each frame the persistent scene is walked and the per-draw records and a
  byte-coded command stream are rebuilt in per-thread linear frame allocators (141AB6900, 141A7ADF0 copies the instance
  matrices into frame memory, 141AD35B0 enqueues), OBSERVED. 141B457A0 concatenates the per-thread 0x18-byte records, sorts
  them by a 32-bit key (pass, view, sequence) and publishes the array, then clears the chunk vector: **checked by Opus**.
  Allocators are reset once a frame (141AF6120). Nothing of frame N's stream is replayed at N+1.
- **What persists sits below the draw** (OBSERVED): the scene (entities in layers with a hash and an AABB tree, maintained by
  add/remove/update-draw-mask events, 141AE9CF0/141AE9EA0/141AEA110, 141B0DDB0), geometry buffers, the material CPU parameter
  blocks, the shader choice as a direct table per material, pass and variant (141AC6CB0), CPU-visible SRV descriptors on the
  view objects, a global PSO cache with a 128-bit key and last-use stamps (1419E3700, 1419F17E0), and a six-deep occlusion
  query ring (141A47590).
- **Redundancy is removed inside a command list, not across frames**: a material is applied only when the material, its
  key or the variant changes from the previous mesh (141AAB2F0, **checked by Opus**); a 64-bit dirty mask per root-table slot
  rebuilds only the changed descriptor tables (141ACD170, all dirty on a root-signature change 141E8F2E0); the pipeline is
  looked up only when its tracker is dirty.
- **Invalidation is by events and dirty masks**, no generation counter on draw data (nothing is cached at that level).
- **Parallelism**: five fork-join levels with an atomic ticket (141EBC020) and pooled per-worker contexts: sub-scenes, layer
  traversal (GXLayerTraverseJob), entity Draw (GXEntityDrawParallelJob 141B4CBE0), instance records (141A79F00), and the
  recording of sorted chunks into fresh command lists (141B3E890 plans up to 0x6C chunks, 141B3D940 records); chunks are
  ordered by a key derived from the sort key, so submission follows the draw-list order, not worker timing.
- D3D12: descriptor tables only (no bundles, no root constants found on the graphics path); constants through a
  fence-stamped upload ring; SRVs copied from CPU staging descriptors into a shader-visible ring per table build.
- Not decoded: 141AB9D60 (pass/LOD/shader-class decisions), the indirect/cluster path (`DAT_143C26BC1`).

## B. Texture binding and arrival

- **Materials never hold a texture**: a material slot holds an index into the model's texture set; each entry holds a counted
  pointer to a stable holder `CSGxTexture` whose +0x10 is the GX texture (OBSERVED). The registry entry (`TexResCap`, holder at
  +0x78) exists before the data and its holder starts on `SYSTEX_DUMMY`: 140B84270 sets the holder's inner to the dummy when
  given nothing, else to the new texture (**checked by Opus**).
- **An arrival builds a whole new GXTexture2D (resource and SRV) and swaps it into the holder** (140B7E940); nothing in the
  model draw is rebuilt: the next bind reads the holder's current SRV, sets the slot's dirty bit, and the descriptor-table
  cache (keyed by a CRC of the tuple of view pointers, 141ACDD90) misses once. No in-place descriptor edit, no mip streaming
  found (OBSERVED).
- **Low resolution then full**: `CSMiddleGxTexture` uses its primary holder when `IsValid()`, else its fallback holder
  (140B7F4A0, **checked by Opus**); the dummy reports not valid. That the fallback is the `_l` package is INFERRED.
- `GXModelTextureSet::ReplaceTexture` (1419E20C0) swaps holders by name CRC; nothing else is rebuilt.
- Unload resets the holder to the dummy (140B84110) or frees the entry; users are not notified. The wrapper releases the
  D3D12 resource and the CPU descriptor slot immediately; the GPU-safe deferral of the texture path was **not found**
  (other owners pin resources through a frame-delayed unref, 141A973A0 -> 141AF4380). DS3 keeps its own fenced retirement.
- System textures: `SYSTEX_WHITE/BLACK/GRAY` (+`_l`, `dummy128`) registered at start (140B20730); `SYSTEX_DUMMY/ERROR/BLINK/
  FASTBLINK` are blink objects whose `IsValid()` is false.

## C. Distant characters: LOD, fades, culling

- Per view and per frame, the disp entity's draw entry (140B31550) computes the camera-eye distance times a zoom factor,
  the LOD from a param row (table id 0x44, INFERRED LodParam): five borders with margins, **hysteresis on the border next to
  the current LOD** (140D44440), or a continuous mode with a blend ratio (140D442D0); the fade weight is a linear ramp over
  [start, start + length] with **no hysteresis and no timer** (140D44120, **checked by Opus**). The main view uses the
  entity's alpha (+0x224); passes 2..0x2C multiply the geometry weight (+0x17C), as DS3's split.
- **Every LOD and fade distance is scaled by a load balancer** (OBSERVED): `CSLoadBalancer` (update 140E0F220, **checked by
  Opus**) moves a level 0..20 from a frame-rate-like measure (up after 5 frames below its band, down after 20 above, holds of
  10 and 30 frames); the level selects per-row scale factors from `LoadBalancerDrawDistScaleParam` / `...New...` rows,
  recomputed every frame (140D44AF0, 140D3F040). A lower frame rate shrinks the LOD borders and the fade ends of every entity,
  and changes the character update budgets (Near/Far omission, 140510600) at the same time.
- Traversal culls by frustum and by AABB-to-eye distance against a per-pass maximum (141AE0610); occlusion results keep a
  node visible 5 to 6 frames (141BF2BF0).
- Candidate causes of distant characters vanishing and reappearing while moving (INFERRED, ranked): the load-balancer scale
  moving with the frame rate; camera distance for fades against player distance for updates; no hysteresis on the fade.

## DS3 counterparts found so far

- DS3's executable carries the same load balancer: RTTI `.?AVCSLoadBalancerImp@NS_SPRJ@@`, strings `LoadBalancerParam`,
  `LoadBalancerDrawDistScaleParam`, `CPU LoadBalancer`, `OmissionUpdate` (2026-10-20, byte search). Its input and level under
  the native renderer are to be read (the native frame rate at Archdragon Peak was 37-53 frames/s in Thomas's play, the
  original about 60).
- DS3's texture holder: the material lookup D24490 returns a holder whose vtable is image+0x29559E8 and whose +0x10 is the
  GX texture (I-0070 re-reads it): the same holder-and-inner pattern as ER's `CSGxTexture`.

## Point 1 result: DS3's load balancer does not move (2026-10-20)

Thomas chose the order 1, 2, 3 (00:43 Paris). DS3's `CSLoadBalancerImp` (RTTI `.?AVCSLoadBalancerImp@NS_SPRJ@@`, vtable
1429A17D8, constructor 140EA3F50 called from 140EDBF3C, instance at [image+0x479B9F8]) has ER's shape: update 140EA40B0 takes
its input from [[image+0x489DD10]+0x2BC] into +0x08, band +0x10/+0x14 (23 / 27 at construction), level +0x2C (0..20, up after
+0x18 = 5 frames under the band, down after +0x1C = 20 over it, holds +0x24 = 10 and +0x20 = 30), and forces the level to 0
while its counter +0x60 (or +0x5C) is below 1 (OBSERVED in the decompile). Polled live during the I-0070 Archdragon Peak walk
and an eight-step camera sweep (`Watch-LoadBalancer.ps1`, 250 ms, `evidence/I-0071/artifacts/*-balancer.jsonl`):
Reference i71-reference-04, native main i71-native-05, native merged i71-native-06: the level stays 0 throughout in all three,
also when the input falls under 23 (17 and 8 samples in the native runs after the load). Conclusion: the load balancer is not
the cause of the distant enemies vanishing; the native draws what the engine's fade weights say (I-0070). Open: whether the
original shows the same disappearances at the same place (ER's camera-distance fade has no hysteresis, INFERRED cause).
