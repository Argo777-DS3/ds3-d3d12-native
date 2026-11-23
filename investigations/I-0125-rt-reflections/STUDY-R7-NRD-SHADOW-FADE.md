# R7 ahead: NRD as the denoiser when Ray Reconstruction is Off, and the traced shadow's fade (study thread)

Study thread S, 2026-11-06 (Paris), text only (R was measuring: no Ghidra, no heavy work), for the R pilot. Thomas (another
thread, 2026-11-06 17:27 Paris, relayed by the coordinator): « Va pour NRD comme solution de base, et le RR comme solution optimale ».
Non-canonical until R re-reads what a change rests on. Public facts about NVIDIA's libraries are INFERRED from their public pages
(sources at the end), not read in a repository of ours; the native's are VERIFIED at the lines cited.

## Part A. NRD (NVIDIA Real-time Denoisers) beside SHaRC and Ray Reconstruction

### A.1 Which denoisers

- **REBLUR_DIFFUSE** for R7's one-ray diffuse GI: REBLUR is the one meant for low ray counts (0.5 to 1 ray a pixel), ReLAX keeps
  more detail but was designed for RTXDI's many-light signal. If the RR-Off path should also denoise R6's reflections,
  **REBLUR_DIFFUSE_SPECULAR** takes both in one dispatch chain.
- **SIGMA_SHADOW** (optional): NRD's shadow denoiser. Today the penumbrae are sampled only on the frames RR denoises
  (`rt_material.hlsli`: « the penumbrae's samples, on the frames Ray Reconstruction denoises »), so with RR Off the shadows are
  hard. SIGMA would give the soft penumbrae with RR Off too; not needed for R7 itself.

### A.2 What it asks for, and where our frame has it

| NRD input | Content | In the native today (VERIFIED where cited) |
|---|---|---|
| `IN_MV` | motion vectors (screen space; `motionVectorScale` in the settings) | `motion_vectors_`, R16G16_FLOAT (`scene_pass.h:187`, 614), DLSS's |
| `IN_NORMAL_ROUGHNESS` | world normal and linear roughness, packed by `NRD_FrontEnd_PackNormalAndRoughness` (its encoding fixed at NRD's build) | RR's guide `rr_normal_roughness` (`scene_pass_frame.cpp:493`) is made from GB0..GB3; the same pass can write NRD's packing (the gloss GB1.x 2^(13 g) turned into a roughness as RR's guide does) |
| `IN_VIEWZ` | linear view-space depth | from the depth buffer and the projection (a small pass, or the RR guide pass writes it) |
| `IN_DIFF_RADIANCE_HITDIST` | the **demodulated** diffuse radiance (the irradiance E, not E × albedo) and the hit distance normalised by `REBLUR_FrontEnd_GetNormHitDist` | the R7 trace writes it (STUDY-R7-GI §2: E(x) and the ray's hit distance) |
| `IN_SPEC_RADIANCE_HITDIST` | the same for the specular (if R6's reflections are denoised too) | R6's reflection term and its hit distance (RR's `rr_specular_hit_distance` exists) |
| common settings | view-to-clip and world-to-view matrices (this frame's and the last), the camera jitter, frame index, `accumulationMode` (restart on a camera cut, a teleport or a load), the denoising range | the DLSS inputs already hold all of them (I-0111, RT-PT-PLAN §5) |

### A.3 The order in the frame

1. R7's trace: one diffuse ray a pixel, the hit lit (R6's `HitRadiance` plus the emission), SHaRC queried for the next bounces
   and updated (its update and resolve passes, before or after the trace: SHaRC's integration guide's order).
2. **RR Off:** NRD on the demodulated E (and the specular if chosen), then the remodulation `GB2 × (1 − F0) × E_denoised` added into
   `light_targets[2]` after GXLightCmbAlbedo (STUDY-R7-GI §2), then the composite.
3. **RR On:** no NRD: the noisy `GB2 × (1 − F0) × E` is added in the same place and RR denoises the lit HDR with its guides (RR
   wants the un-denoised signal; NRD before RR would harm it, INFERRED from RR's design).
So NRD and RR are exclusive on a frame; SHaRC serves both.

### A.4 Integration and cost

- **API:** NRD's low-level API (`nrd::CreateInstance`, `GetComputeDispatches`) hands the dispatches, their pipelines (embedded
  DXIL) and their resources to the engine, which records them itself: no other runtime; the alternative wrapper goes through NVIDIA's
  NRI layer (an extra dependency). The low-level API fits our renderer (its own root signatures, frame graph and barriers).
- **Cost** (public, NRD's README v4.17.2: RTX 4080, 1440p native, defaults): REBLUR_DIFFUSE_SPECULAR 2.45 ms, ReLAX 3.15 ms;
  the diffuse alone is cheaper (INFERRED about half); on the 4070 Ti Super about 15-20 % slower (INFERRED); at DLSS's render
  resolution (Quality: 0.67 per axis) less again, since NRD runs at the render size.
- **The licence:** NRD is under NVIDIA's RTX SDKs licence, not MIT (NVIDIA's developer page asks for it; the repository's
  `LICENSE.txt` not read here). Its acceptance and download are Thomas's (accepting terms is his, as for SHaRC and the Streamline
  SDK of I-0111); the library lives outside the repository like the other NVIDIA SDKs (WORKFLOW « Tools »). TO CHECK before code:
  the licence's redistribution terms for a mod's DLL, and SHaRC's own licence (its repository's file not read either).

## Part B. The traced shadow's fade (gap 21, I-0113 « Tranche 19 » on branch `claude/trusting-agnesi-6871a9`)

E's finding (static, INFERRED cause, a run to confirm it after 20:00): the original's casters' row (preparation 2, the cascades and
the local faces) of a fading part takes the alternate fade byte `+0x89` (5C0E0:101-108), so a part whose group sets `+0x89`
casts a shadow dithered by its opacity `+0x124` (the hair during a warp: its shadow breaks into sparse pixels while its colour
fades smoothly). Natively the traced shadow has no fade term: `RtOccluded` (`rt_material.hlsli:434-445`, VERIFIED) commits any
non-opaque candidate its alpha test keeps, and an instance is non-opaque only when its materials are (`rt_scene.cpp:223`).

### B.1 Proposal: a hashed cut at the candidate

1. **CPU:** each instance gets a shadow opacity `f` = the group's `+0x124` when `+0x89` is set and the opacity is under its
   category scalar (E's rule), else 1. It travels in `RtInstanceRow`'s free 16 bytes (the row is 256 bytes, its fields end at
   byte 240: `rt_material.hlsli:35-37`, INFERRED from the field sizes). An instance with `f < 1` takes
   `D3D12_RAYTRACING_INSTANCE_FLAG_FORCE_NON_OPAQUE` (beside `rt_scene.cpp:223`'s rule), so its triangles reach the candidate loop.
2. **Shader:** in `RtOccluded` (the sun's and the local lights' query), before `RtKeeps`, a candidate of an instance with `f < 1`
   is committed only if `hash(instance, primitive, the hit's object-space position quantised to about a texel) < f`. The same
   test in the reflection and GI visibility queries (`rt_reflect.hlsl`, R7) keeps every traced shadow consistent.
3. **The pattern:** with the frame index in the hash on the frames a denoiser runs (RR, or NRD from Part A), the shadow fades
   smoothly to `f` × its full strength (the natural reading of D-0249); without a denoiser, a stable hash (no frame index) keeps a
   still dither, the closest to the original's sparse pixels. Either way the shadow fades with the part, which is the defect.

### B.2 Cost

Only fading instances change (a warp, a LOD hand-over, a dying enemy: a handful at a time): their opaque triangles become candidates
of the loop, each costing one hash and a compare more, and an alpha-tested slice its usual texture read. The TLAS is rebuilt every
frame (R2), so the flag costs nothing to set. INFERRED negligible against the frame's rays; not measured.

### B.3 What E's run decides first

If E's run after 20:00 (`fade-shadow-alt.flag`, the raster casters taking `+0x89`) shows the original's dissolve, the cause is
confirmed and B.1 is the traced port; if not, B.1 is not needed for gap 21. B.1 can be written meanwhile behind a flag, since a
fading part casting its full shadow is wrong whatever gap 21's cause.

## Sources (public)

- NRD repository and README (inputs, the performance table): https://github.com/NVIDIA-RTX/NRD
- NVIDIA's NRD page (the RTX SDKs licence, 0.5 to 1 ray a pixel): https://developer.nvidia.com/nvidia-rt-denoiser/get-started
- SHaRC's integration guide: https://github.com/NVIDIAGameWorks/Spatial-Hash-Radiance-Cache/blob/main/docs/Integration.md
