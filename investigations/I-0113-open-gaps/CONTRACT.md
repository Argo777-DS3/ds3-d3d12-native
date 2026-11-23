# I-0113 - Les écarts ouverts

Capability: C3 · the open-gaps pilot (Thomas, 2026-10-30 05:32-05:34: two pilots in parallel, this one on the textures and the
visible defects; the raster lighting's defects left to the RT workstreams), opened 2026-10-30 after
[I-0105](../I-0105-missing-effects/RESULT.md) closed.

## Question

Why does the native renderer draw these surfaces differently from the original, and what per-family fix (D-0150, D-0161) makes
them equal to it?

## Scope

Included, in this order (STATE « Next action » and « Open gaps »), each fix a tranche merged once written and reviewed (D-0227):

1. **Anor Londo's cathedral walls**, grey and untextured after the great stairs (Thomas, 2026-10-30 04:44, in `main`). First the
   witness `native_texture_white_fallback` (`08_model_programmes.inc`, `NativeFallbackTexture`): which material textures fall back
   to SYSTEX_WHITE there, then why (the path, the `_l` bank or atlas, the holder resolved through D24490).
2. **The Shrine Handmaid's chair o404854**, white wood (FABLE-REVIEW §1.2): the same witness, then the resident texture the
   native samples against the original's.
3. **The missing Load Game portrait** (the Dreg Heap character, `i100-tr7-night`; an I-0100 lead).
4. **The banding and the red and blue specks in the dark areas without DLSS** (`evidence/I-0111/artifacts/i111-zoom-start.png`):
   compared first with the original.
5. Then the rest of STATE's open-gaps row by visibility.

Excluded:

- The raster lighting's defects (the RT workstreams replace it), the RT pilot's files (I-0111: Streamline, the jitter, the
  velocity), the effects already closed by I-0105 unless a gap names them.

## Inputs

- The witness rows: `native_texture_white_fallback`, `native_texture_unbound`, the lightmap lookups (`08_model_programmes.inc`).
- [FABLE-REVIEW](../I-0066-independent-native-startup/FABLE-REVIEW.md) §1.2 (the chair: alpha test active, diffuse
  `m40_00_chair_01_a` resolved, the wood white).
- Zone saves: Gamma at the Anor Londo bonfire `shebang-zones-gamma-m37-anorlondo-20261026-123818` (SHA-256 E9623689…3703411,
  [LIVE-RUN-NOTES](../../.project/LIVE-RUN-NOTES.md)); the Firelink saves for the chair.
- Tools: GhidraMCP (port 8080, read-only), the pair sheets and FLIP for the pictures, ffmpeg.

## Hypotheses

- H1: the walls' material textures are not found by the native lookup (a path or bank the native does not resolve) and fall back
  to SYSTEX_WHITE, which the lighting greys.
- H2: the textures are found but bound to the wrong slot or sampled with the wrong programme (the material's MTD family).
- H3: the textures are resident only through a holder the native does not follow (a streamed `_l` bank).

## Expected evidence

The witness rows of a native run at Anor Londo (which paths, which slots, which MTD), the same view captured natively and on
the original, then the fix's picture against the original's (FLIP), the tested surface in view.

## Exit conditions

- Success: each listed gap drawn as the original's on pictures (FLIP, the surface in view), or recorded with its cause and why
  it stays open.
- Refuted: a hypothesis the witness rows contradict is recorded and the next one taken.
- Insufficient information: a gap the witness cannot name is recorded with what would name it.
- Stagnation: no new discriminating evidence after 3 cycles on one gap: move to the next gap and record it.
- Budget exhausted: close the tranche and replan under `.project/WORKFLOW.md`; renew a bounded budget automatically within
  unchanged authorized scope and existing protected boundaries.

## Safety and human interaction

Short targeted witness runs (about ten minutes, announced under the Live game rule, offline, hash-checked restore, on the ASUS),
on the zone saves, or on a copy of Thomas's save when a gap needs his character (Thomas, 2026-10-31: the casts from a copy of his new save; his own save is never overwritten). The RT pilot has priority on the ASUS and the launcher rebuilds unless a fix here is
ready to test; the coordinator gives the slots.
