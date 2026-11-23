# I-0068 part L: E05 radial blur and E04 distortion ready to code (the frame-copy families)

Request (coordinator, 2026-10-10), items 2 and 3, in this order: E05 radial blur, then E04
distortion. Both read the frame copy. The format follows
[EFFECTS-PHASE2-E01-E02-NATIVE](EFFECTS-PHASE2-E01-E02-NATIVE.md); this note gives only what
differs from that machinery.

Prerequisites: everything in part H.

- §3 pass points and the scene-control draw;
- §4 native `GXFfxDrawPass`, the transcribed `FUN_140299E40`;
- §5 the ring, and `GXFfxDrawContext` with slot 27 transcribed;
- §7 the builders;
- §8 **the frame copy**.

E04 and E05 add no construction step and no new GX object.

Labels: STATIC, INFERRED, OBSERVED. Evidence: `evidence/I-0068/queries/ffx/e05-e04/` and the
regenerated `ffx/ffx-shader-set.tsv`.

## 0. Correction: the shader set has 158 entries per stage (STATIC)

- `FUN_140291520`'s VS and PS name arrays (`apuStack_1d88`, `apuStack_1898`) hold **158 entries**:
  their stack bases are 0x4F0 bytes apart.
- Ghidra declares 128 and names entries 128–157 as separate variables (`puStack_1930` =
  VS[139]). The first `Get-ShaderSetTable.ps1` therefore kept indices 0–127 only.
- The script now maps them back. `ffx-shader-set.tsv` is regenerated: VS and PS 158 rows each;
  the other arrays are unchanged.
- The missing entries (VS / PS):

| Index | VS | PS | Reached from |
|---|---|---|---|
| 128–131 | `tessellate{,Soft}MultiTexture` | `tessellateBlend{,Soft}MultiTexture[2Sampler]GlowD` | glow twins of 53–56 (+75) |
| 132–137 | `bumpTexture` / `softBumpTexture` | `…_PNT1..3GlowD` | glow twins of the lit 64–69 (INFERRED pairing by name) |
| 138 | `bumpTexture` | `bumpDistorsionTextureGlowD` | glow of 70 (`0x8A`, §2.3) |
| **139** | **`GXFfxSoftRadialBlur`** | **`GXFfxSoftRadialBlur`** | E05 kind 0x25 + soft (`0x8B`) |
| **140–145** | **`GXFfxdistortionSoft`** | **`distortionSoft`, `…SoftBump` (×2), `…SoftRotation`, `…SoftTexture`, `…SoftBumpTexture`** | E04 kinds 0x1F–0x24 + soft (`0x8C..0x91`) |
| 146–149 | `tessellateSoftTracer{,Bump}Texture` | the same, + `GlowD` | E02 soft tracers |
| 150–153 | `tessellateDistortionTracerBumpTexture` | `tessellateDistortion{,Soft}TracerBumpTexture[GlowD]` | E02 bump tracers with distortion (`0x96..0x99`, §2.3) |
| 154–155 | `bumpDistorsionSoftTexture` | the same, + `GlowD` | E01 soft bump with distortion (`0x9A/0x9B`, §2.3) |
| 156–157 | `softTracer` | `softTracer`, `softTracerGlowD` | E02 non-tessellated soft tracer |

- Part H's §6 key is unchanged. The PSO cache must accept indices up to 157.
- The glow rule "+75" holds for 0–56 only; the higher glow twins are listed explicitly above.

## 1. Programme choice for E05 and E04 (STATIC, `FUN_140264230` lines 729–787, 1126–1157)

The draw kind `ctx+0x964` is written by the appearance setups (draw-context slots 13–20:
`FUN_140260830`, `FUN_140260CE0`, `FUN_140261170`, `FUN_140261600`, `FUN_140261AB0`,
`FUN_140261F60`, `FUN_140262530`, `FUN_140262A70`), with the kind the library passes. The
`soft` flag is `bVar29`.

| Kind | Index (soft) | VS | PS | Family |
|---|---|---|---|---|
| 0x1F | 57 (140) | `GXFfxdistortion` (`…Soft`) | `distortion` (`…Soft`) | E04 |
| 0x20 | 58 (141) | same | `distortionBump` (`…SoftBump`); unlit flag | E04 |
| 0x21 | 59 (142) | same | `distortionBump` (`…SoftBump`) | E04 |
| 0x22 | 60 (143) | same | `distortionRotation` (`…SoftRotation`) | E04 |
| 0x23 | 61 (144) | same | `distortionTexture` (`…SoftTexture`) | E04 |
| 0x24 | 62 (145) | same | `distortionBumpTexture` (`…SoftBumpTexture`); unlit flag | E04 |
| **0x25** | **63 (139)** | `GXFfxradialBlur` (`GXFfxSoftRadialBlur`) | same | **E05**. Soft also sets t3 = scene depth `[GXRS+0x4E10]+0x48` (else `ctx+0xA68`) |

- No glow variant exists for these kinds: there is no MRT output, so they write the forward HDR
  target only.
- **Distortion on other kinds** (STATIC, lines 1126–1157). When the FXR distortion amount
  `ctx+0x5A0 > 0`, bump billboards and tracers are switched to their distortion twins (these are programme **indices**, after the kind switch):
  - 3 → 70 `bumpDistorsionTexture`;
  - 0x4E → 138 (its glow);
  - 5 → 154; 0x50 → 155;
  - 0x25 (tracer bump) → 150; 0x70 → 151;
  - 0x94 → 152; 0x95 → 153.
  - These read **t4 `g_TraceframeBufferTexture`** (= the frame copy, FFX-FAMILIES-PHASE2 §8)
    instead of t0. They belong to the E01/E02 draws, but they need the frame copy like E04.

## 2. Inputs, constants and resources (STATIC, DXBC reflection, `ffx-program-layouts.tsv`)

| Programme | VS input | VS constants used | PS resources | PS constants used |
|---|---|---|---|---|
| `GXFfxradialBlur` | `POSITION, TEXCOORD0` | `g_vs_fade{Near,Far}{Start,Dist}` only: **no matrix**, so the position is already in clip space (INFERRED) | **t0 `g_diffuseTexture`**, t1 `g_normalTexture`, s9 | `g_color`, **`g_screenCenter`**, **`g_radialDistance`**, `g_vColorScale`, `g_ps_ColorBlendType`, `g_ps_softSpriteParam`, **`g_ps_RadialBlurSamplingLengthMaxX/Y`** |
| `GXFfxSoftRadialBlur` | `POSITION, TEXCOORD0` | + near/far, `g_vs_softSpriteParam` | t0, t1, **t3 depth**, s9 | + `g_ps_nearFar*`, `g_ps_far` |
| `GXFfxdistortion` (+`Soft`) | `POSITION, TEXCOORD0` | **`g_worldViewProjectionMatrix`**, `g_eye`, **`g_fbTexCoef`** (+ soft params) | **t0 `g_frameBufferTexture`**, t3 depth, s9 | `g_color`, near/far, **`g_waveTime`, `g_clampDistanceInverse`, `g_waveStrength`, `g_waveNum`**, `g_vColorScale`, `g_ps_ColorBlendType`, `g_ps_softSpriteParam` |
| `distortionBump` / `…BumpTexture` (+`Soft`) | same | same | t0 frame buffer, **t1 normal**, (t2 blend), t3 depth, **s1 wrap** + s9 | + `g_vNormalMapScrollDistance`, `g_hNormalMapScrollDistance` (no `g_waveNum`) |
| `distortionRotation`, `distortionTexture` (+`Soft`) | same | same | t0 frame buffer, (t2 blend), t3, s9 | as `distortion` |

**Where t0 comes from** (FFX-FAMILIES-PHASE2 §8, STATIC):

- Draw-context slot 8 `FUN_140260410(null)` sets t0 = `ctx+0xA70` (the frame copy) and flag
  `+0x9BA`, which selects the alternative sampler `[[GX]+0x2A0]+0x2E8` for pass type 5.
- The distortion PS names t0 `g_frameBufferTexture`, and the radial PS names it
  `g_diffuseTexture`. INFERRED: the radial setup also passes null to slot 8, because a radial
  blur of anything other than the scene would not need `g_screenCenter`. Confirm on the first
  witness (check L3).

**Constants:** all of them live in the five context blocks (part H §5.2). `g_fbTexCoef` is
filled by the commit from the frame copy's size (INFERRED from its name; it sits in
`VSCommonBuffer`). Natively there is nothing new to lay out: upload the blocks as they are.

## 3. Ordered list (deltas to part H)

| # | Step | Mode | Native |
|---|---|---|---|
| 1 | frame copy pipeline `+0x1108` created with the HDR target (`FUN_140085540`) | NATIVE (part H §8) | one texture of the native HDR target's size and format; R8G8B8A8 when the pipeline LDR flag `+0x74` is set |
| 2 | copy at forward pass 0x29 (`FUN_1400BFA70`), bound as GX slot 45 | NATIVE | before the first effect of pass 0x29: HDR target RENDER_TARGET → COPY_SOURCE, copy COPY_DEST, `CopyResource` (`ResolveSubresource` with MSAA), both back (copy → PIXEL_SHADER_RESOURCE) |
| 3 | `passCtx+0x4C0` = the copy, `FUN_140299E40` sets `ctx+0xA70` and `ctx+0x990` | TRANSCRIBE (part H §4) | the native copy's SRV |
| 4 | the appearance setups (slots 13–20) and slot 8 | CALL | — |
| 5 | slot 27 `FUN_140264230`: indices 57–63 and 139–145 (§1); the distortion switch `ctx+0x5A0` | TRANSCRIBE (part H §5.2) | PSO key with the index; t0 = frame copy; t3 = depth for soft; s1 for the bump variants |
| 6 | builder | TRANSCRIBE (part H §7) | **non-tessellated: triangle list** (`FUN_140396F30`) or `Draw(4)` (`FUN_14039E900`). These programmes have no hull or domain shader. INFERRED; log the builder per kind on the first witness |
| 7 | blend, depth, raster | from the bank (part H §6) | same rules: FXR blend type `ctx+0x960`, depth `+0x1E8` (test on, write off), raster `+0x208` |

**Read-after-write rule.** The original copies the scene **once per forward pass**. Every
distortion or radial blur in pass 0x29 or 0x31 samples the scene as it was before the first
effect of the pass, and not the output of an earlier distortion. Effects in passes 0x20/0x21
read the previous frame's copy (INFERRED, FFX-FAMILIES-PHASE2 §8). Keep exactly that: one
copy per frame at the start of pass 0x29, never a copy per draw. The native copy is never bound
as a render target in the same pass.

## 4. PSOs

| Field | E05 radial | E04 distortion |
|---|---|---|
| VS / PS | 63, or 139 soft | 57–62, or 140–145 soft |
| HS / DS | none | none |
| input layout | `POSITION`, `TEXCOORD0` (formats from the VS signature; stride from the builder) | same |
| topology type | TRIANGLE | TRIANGLE |
| blend | bank entry by `ctx+0x960` (usually alpha: the PS outputs the blurred or refracted colour with alpha; INFERRED) | same |
| depth | `+0x1E8` against the scene depth | same |
| RT | forward HDR only (no glow MRT) | same |
| root signature | part H's (b0 VS, b0/b1/b6 PS, t0–t4, s1, s9) | same; **s1 = wrap linear** for the bump scroll |

## 5. Checks

| # | Check | Witness | Pass |
|---|---|---|---|
| L1 | **E05 drawn** | the i63-live-33 situation (a spell or boss attack with a radial blur; it was drawn as `GXFfxSoftRadialBlur` there, OBSERVED) | at least one draw with PS index 139, t0 = frame copy SRV, t3 = depth |
| L2 | **E04 drawn at m40** | I-0063 i63-live-01 stream 1: `GXFfxdistortionBump.ppo` + `GXFfxdistortion.vpo` drawn in **47 of 1,800 frames** (OBSERVED) | the same programme pair (index 58 or 59) drawn over the same camera path; per-frame presence within ±10% of 47/1,800 |
| L3 | t0 is the frame copy | every draw with index 57–63 or 139–145 | t0 descriptor = the native copy's SRV; the copy was written earlier in the same frame (frame counter) |
| L4 | one copy per frame | copy count per frame in pass 0x29 | exactly 1, before the first effect; 0 extra copies even with several distortions |
| L5 | no feedback | the copy is never bound as RTV while an effect samples it | D3D12 debug layer clean (no RESOURCE_STATE mismatch) |
| L6 | m51 / m37 witnesses (highest E05 weights 134 / 112; E04 54 on m37) | Reference run with a pass-through observer on `FUN_140264230` counting kinds 0x1F–0x25 | native counts per kind = Reference counts |
| L7 | distortion twins | draws with indices 70, 138, 150–155 | t4 = frame copy; counted like L6 |

## 6. Risks

1. **The 128-entry table.** Indices 128–157 were missing from the earlier evidence, and the soft
   variants of E04/E05 live there. A native shader-set builder sized at 128 refuses or crashes on
   the first soft distortion.
2. **Copy format.** With the pipeline LDR flag the copy is R8G8B8A8, not RGBA16F. Create it with
   the HDR target's actual format, or the copy is invalid.
3. **Copy point.** Copying after some translucents changes what distortions refract (doubled
   particles inside heat haze). Keep the copy at the start of pass 0x29.
4. **Sampler `+0x2E8`.** Slot 8's null path also flips `+0x9BA`, which changes the sampler for
   pass type 5. Map that bank entry too, or soft particles in pass 0x2D sample with the wrong
   addressing.
5. **Radial t0.** If the radial setup passes a real texture to slot 8 rather than null, t0 is that
   texture and the effect is not screen-based. Check L3 decides.

## Exports

`decompiled/`: the appearance setups `00260830`, `00260CE0`, `00261170`, `00261600`, `00261AB0`,
`00261F60`, `00262530`, `00262A70`, and slot 8 `00260410`. Existing: `00264230`, `00291520`,
`00299E40`, `00085540`, `000BFA70`, `00396F30`, `0039E900`.
