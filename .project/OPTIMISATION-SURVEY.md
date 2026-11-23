# The native's extra work: the survey for ROADMAP workstream 15 (D-0304)

Written 2026-11-21 by the R pilot (I-0141 tranche 18 and Thomas's questions, relayed by the coordinator). It lists the work the
native does that the original avoids or does twice, with the picture unchanged. It comes from the records, with no run; a cost is
not measured unless the item says so (D-0227). Thomas: « Comme c'est de l'optimisation, à la fin oui, à inscrire dans le dépôt »,
« Inscrivez tout ça dans le dépôt du coup » (14:19 and 14:37 Paris).

**The constraint** (Thomas, 14:43: « Bon faites ce qui vous semble juste, tant que ça ne casse pas nos rayons tracés »): the methods
are the pilot's. No cull or sort takes out of the RT/PT scene what a ray can see (a shadow, a reflection, a bounce from outside the
view). Each item is accepted on a picture comparison, RT and PT, against the frame before it (the coordinator's relay).

1. **A modern occlusion cull.** The coordinator's description, which Thomas accepted (« Faisons comme ça alors. Je ne connaissais
   pas », 14:22): not a copy of the original's D3D11 queries. It runs on the GPU from the previous frame's
   depth (Hi-Z), perhaps per cluster of a model (Nanite's way). It lightens the raster passes, not the rays. The original's own
   occlusion system never runs natively. At m32's bonfire the native's G-buffer holds about 346 models against 203; with its occlusion
   nodes and cells forced visible the original draws 137 of the 143 extra (I-0141 tranche 18, OBSERVED; the occlusion alone,
   INFERRED). The bound on the gain: the raster's whole GPU share
   is 1.5 to 2 ms of a 17 ms traced frame at 1920x1080 (D-0255).
2. **The rays' scene sorted.** It is not sorted today (read in `12_scene_collect.inc`). The TLAS holds every entity the engine has
   loaded: the walk's culler holds the whole world, after Thomas's word of 2026-10-30 that the rays see everything loaded. That is
   4,742 instances in Anor Londo's hall (I-0132). Each BLAS is at LOD 0, the finest, with no distance or size cull. To add: the far
   or tiny instances out of the TLAS, or behind an instance mask for the rays that do not need them; simplified models (a coarser
   LOD) for the far ones; fewer rays with a better denoiser (the PT's cost).
3. **The collect and the rays' walk read the whole scene each frame**, not what changed. This is D-0255's CPU lever: at m45 in 720p
   the collect takes 6.0 ms and its rays' walk 2.0 ms (MEASURED). R11g moved the walk to its own worker.
4. **The UI ingress** (I-0120 G2, G3, G7; read in the code):
   - the glyph atlas (1 to 4 MB) is copied each frame and compared byte by byte;
   - each CPU UI image gets a second GPU upload that is never sampled;
   - the constants are packed twice a draw.
5. **R11b's RT leftovers** (I-0132 RESULT; read in the code):
   - every pixel is traced (the stencil sorts only at the apply);
   - SER's scratch is kept while it idles;
   - SER's constants and the auto exposure's block are uploaded twice.
6. **The Firelink walls' follow effect** asks E6DCD0 once or twice a frame, without end (I-0113, calls OBSERVED).
7. **The TAA's reactive forms** (I-0113, I-0140; read):
   - the 3x3 pass and the weighting's copy;
   - a held translucency resolves its mask pipeline per row, outside the cache;
   - the fade-mask pipelines are made after the reactive path latched off.
8. **Animated map objects' pose hand-over** (14066BBB0) runs about 1.5 times the original's rate (I-0100, OBSERVED, to check
   again).
9. **Admission records rebuilt for a few models** (c1200, c1240, FG_A_0100, o392550…): about 2.6 ms a frame of pool work (I-0100,
   OBSERVED, old: to check again).
10. **Small diagnostics on by default:** three witness-file probes every 15th schedule call, and a row formatted each run (I-0137,
    I-0140).
11. **The effects' root CBVs and the D3D12 audit's leftovers.**
    - The effects' 234 root CBVs a frame, and their arena (about 170 KB) copied twice: 0.13 to 0.18 ms at Firelink (I-0127,
      MEASURED; ExecuteIndirect not done).
    - From the D3D12 audit (I-0084, counts MEASURED): transients not aliased (about 141 MB), the uploads on the DIRECT queue, the
      D3D11-shaped root signature, about 140 barriers a frame.
12. **The 1.5 m cull margin on map pieces** (I-0141 tranches 19 and 22, OBSERVED at m32 in one run on light-observe.flag's whole-filter path, D-0222's opt-out: 85 piece instances that the view's culler rejects kept by the margin, the five of tranche 19 drawn in the G-buffer, each wholly outside a side plane by 0.03 to 1.5 m):
    the margin is for characters' meshes wider than their bones' box (D-0220); a map piece's box bounds its geometry, so testing
    pieces against the view's own culler would drop that work (a change of the main walk, D-0220's scope to check).
