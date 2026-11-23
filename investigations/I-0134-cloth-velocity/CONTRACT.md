# I-0134 - La vélocité du tissu simulé

Capability: C3

Origin: I-0113 tranche 39 handed on « the cloth's own velocity (the simulated vertices' motion, absent from the original's Velo
programme too) to the R pilot after R12e » (I-0113 RESULT « Tranche 39 », « Handed on »); the coordinator opened this thread on
2026-11-12 00:09 (Paris).

## Question

Can the Velo rows of a simulated cloth mesh carry the simulated vertices' own motion, so the TAA, DLSS, RR and NRD reproject
a moving cape instead of treating it as still against its frame?

## Scope

Included:

- The Velo rows (engine pass 0x34) of a cloth mesh whose simulated copy is bound and whose material enables the cloth
  (`VC_isEnableCloth` = 1, the admission's 1184 rule): their vertex programme's previous position.
- The previous simulated copy kept with the current one (the cloth upload hook), bound as a third vertex stream to those rows.
- The velocity picture witness (`world-pictures.flag`) and a film of a moving cape, before and after (D-0268 (3)).

Excluded:

- A cloth mesh whose copy is bound but whose material leaves the cloth off (skinned by the palette with the copy as stream 0):
  counted, left as the game's Velo programme does it.
- The rays' cloth (I-0113 tranche 37), the shadows, the G-buffer rows: unchanged.
- The motion blur's own velocity (it reads the same target: it gains the cloth's motion as a side effect, not tuned).

## Inputs

- The cloth Velo programmes (`fxc /dumpbin` of the I-0063 cache, 192 vertex and 90 pixel programmes whose names hold `cloth`):
  the cloth branch (`movc` on `VC_isEnableCloth`, cbMatDynParam +0x80) places stream 0 by `VC_World` (b7 rows 0-2) for the
  current clip position and by `VC_PrevWorld` (b7 rows 3-5) for the previous one, from the same `POSITION`: the simulated
  delta is absent (READ).
- `NativeClothVertexUpload` (17_render_step.inc): one immutable native copy per simulated frame (23D5BC0); the cloth instance
  frame I+0x50 (current) and I+0x90 (previous) as `VC_World` / `VC_PrevWorld` (09_model_admission.inc, 1185).
- Tools: clangd, the module's build and unit tests, `world-pictures.flag`'s velocity pictures, `Run-AlphaCast.ps1` films.

## Hypotheses

- H1: a native Velo vertex programme for these rows, the game's own pixel programme kept (`GXFlver_Common_Velo` or the family's
  `_Velo_[A]`), placing the previous simulated copy by `VC_PrevWorld`, gives the cape non-zero object motion where it moves.
- H2: the ghosting seen on capes under the TAA comes from that missing motion (a film before and after decides).

## Expected evidence

- Unit tests: the programme compiles; its output signature equals the cloth Velo programmes' (SV_Position, TEXCOORD0 to 3);
  the draw's third stream validated.
- A run (the ASUS, offline, restored by hash): velocity pictures with the cape's object pixels before (motion of the frame only)
  and after; a film of the cape under the TAA, before and after.

## Exit conditions

- Success: the cloth's motion in the velocity pictures and the film's ghosting reduced, reviewed (step, final xhigh,
  integrity), merged.
- Refuted: no difference in the pictures (the copies do not differ between frames, or the pairing is wrong): recorded.
- Insufficient information: no moving cape at the witness view.
- Stagnation: two failed fixes, then an RCA.
- Budget exhausted: close the tranche and replan under `.project/WORKFLOW.md`.

## Safety and human interaction

Live runs only in slots the coordinator gives (D-0116, D-0117), a test save restored by hash, never Alpha's real save.
