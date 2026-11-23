# I-0113 gap 12: Flame Fan cut by a vertical line (study thread, static, 2026-10-31)

Written by the study thread for the gaps pilot to copy into I-0113. STATIC only. Thomas's two captures of 07:45 are not in the
evidence store (only the 07:48 clip is, `Videos/NVIDIA/Dark Souls III`), and the original was not filmed casting Flame Fan:
no comparison exists yet, so nothing below says the cut is native.

## 1. The spell's effects (VERIFIED, params and FXR read)

- Magic 2458000: SFX 529000 (a reference to 524000), 529001, 529002 (the cast); bullets 12458000 (projectile FXR **529003**,
  no hit FXR) and 12458010 (none). The FXRs are in `frpg_sfxbnd_dlc2_effect` (Ashes of Ariandel), not in commoneffects.
- Draw actions, all **blend Add**:
  - **604 MultiTextureBillboardEx**, layers 11627 + 11655, `octagonal` 1, `depthBlend` 1 (four nodes of 529002, one of 529003):
    the tessellated soft multi-texture programmes (`GXFfxtessellateSoftMultiTexture` with the `_octed` hull and domain);
  - **606 LegacyTracer**, orientation **Cross**, texture 2012, width 0.625, colour × `rgbMultiplier` 2, `color3`
    (1.5, 1.25, 1), segment duration 0.4 s, 12 segments (529002 node 4, 0.2 s delay): the trail of the sweep;
  - 603 BillboardEx 3000 (LocalYaw), 11657 (octagonal, soft), 1001 (octagonal, soft); 609 point lights.
- No 605 model, no 607 distortion, no 608 radial blur: none of 16/13's mechanisms applies.

## 2. What the programmes cannot do (VERIFIED, fxc disassembly of the I-0063 cache)

- The `_octed` hull shader has constant tessellation factors (edges 3, 1, 3, 1, inside 1) and no patch culling: it cannot cut
  the fan at a screen line.
- The `_octed` domain shader places the octagon in world space from the particle's matrix (control point TEXCOORD5..7) and
  projects with `g_ds_viewMatrix` / `g_ds_projectionMatrix`; its soft-sprite term replaces the clip z from `g_ds_nearFar*` /
  `g_ds_near` (DSCommonBuffer +336..+348): wrong or zero values there would drop or misplace whole particles, not cut them
  along one screen column.
- The soft pixel programme computes the depth texture's uv from the clip position (`pos.xy / pos.w`), not from a screen
  size: no render-resolution or DLSS scale enters it. The effect pass's scissor is the full render target
  (`scene_pass_frame.cpp:302`).

## 3. Readings of the two symptoms (INFERRED)

- The **bright vertical sliver** across the character: a **Cross** tracer is two perpendicular ribbons; the vertical one seen
  edge-on is a thin bright line, doubly bright here (`rgbMultiplier` 2, `color3` above 1). Possibly the original's look too.
  Natively the trails (classes 3D7E380 and 3D660B8) are on by default since I-0105, so a strip-order defect is also possible.
- The **flames ending on a vertical line**: an occluder's edge in the depth (every billboard here is soft: `depthBlend` 1, and
  depth-tested), the sweep not yet finished at the capture (the capture three seconds later is fuller), or a native defect;
  no static reading separates them.

## 4. Checks that decide it

1. **The original against the native** at one spot, the same cast, a few frames apart (the gaps pilot's spell runner, Thomas's
   save copy, a catalyst for pyromancies). Same cut in the original: close as the original's look.
2. **A/B `no-ffx-trails.flag`** (native): the sliver gone means it is the 606 Cross tracer; then compare its strips (vertex
   count, topology) with the original's.
3. If the cut is native and stays without the trails: hide the 604 octagonal draws (class 3D7E5C0, textures 11627 / 11655),
   then read the soft term's inputs in both modes (PS `g_ps_softSpriteParam`, DS `g_ds_softSpriteScaleOffset`, `g_ds_nearFar*`).

Sources: Magic.tsv and Bullet.tsv (`evidence/I-0105/control-spells/tools/Params.ps1`), `Read-Fxr.ps1` on
`C:/Users/User/.codex/I0033-assets/effects/frpg_sfxbnd_dlc2_effect/f00052900{0..3}.fxr`, fxc `-dumpbin` of
`gxffxshader__GXFfxtessellateSoftMultiTexture_octed.hpo` / `.dpo` and `gxffxshader__GXFfxsoftMultiTexture.ppo`.
