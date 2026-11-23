# I-0138 tranche 2: the bloom and the light shafts, ready to implement (static)

**Built on 2026-11-13: [RESULT](RESULT.md) « Tranche 2 » says what was done and where it departs from this sheet (the block
and the technique names kept, the witness check's place, the five pins added). The text below is the sheet as written.**

2026-11-13, the R pilot's thread (Fable 5.2), written while tranche 1 waited for its merge slot. Read-only: the native tree at
tranche 1's final tree, the original's programmes extracted and disassembled, [STUDY](STUDY.md), [PLAN](PLAN.md) §5 and
[ER-VALUES](ER-VALUES.md) §6. Nothing built or run. Labels: **verified** (read in the code, a disassembly or a record of this
repository), **proposed**, **open**.

## 0. Answer first

1. **Proposed: the original bloom and shafts remade exactly, as tranche 1 remade the tone map.** The glare family's draws
   stay what they are (they are already ours: the plan of `yebis::BuildGlare` and `BuildShaft`, the library's CPU formulas
   transcribed and tested word for word); what goes is what is still YEBIS's, **its pixel and vertex programmes read from the
   game's image** and its 4,096-byte `$Globals` block. They are replaced by a small family of programmes of our own
   (`post_glare.hlsl`) and a compact block a draw. The oracle exists and is exact: T13 (each captured draw) and T14 (228
   surfaces at 9 zones) are byte for byte on the capture's adapter today. **Nothing changes on screen.**
2. **This departs from PLAN §5's wording for tranche 2** (« notre pyramide de bloom », judged on sheets): a new pyramid
   cannot give the original colorimetry's bytes (per-channel Gaussian radii by wavelength, a screen blend after the curve),
   and PLAN §4 asks the original colorimetry's SDR picture to be today's. Tranche 1 showed that exactness is reachable and
   cheap to prove. The modern pyramid has its place where the picture is meant to change: the Rekindled colorimetry's
   candidate made after Elden Ring (tranche 6: its bright pass, three levels at 1, 0.5 and 0.25, added before the curve).
   **For Thomas**: said to him with tranche 1's result; if he prefers the plan's first form, §6 gives it.
3. **The light shafts come with it, inside the same family** (D-0276: no bridge to an old pass): their mask, radial
   blurs and add are three more programmes of `post_glare.hlsl`; `Alpha_R` too.
4. **The same programmes serve the depth of field and the motion blur's blurs** (`MadT1_S{n}` is shared), so tranche 4
   starts with part of its work done.
5. **Refusals**: those that were limits of the programmes' count go (a composite of more than eight levels); those whose
   original picture nobody has seen stay refused until a capture exists (glare shape 3, the custom glare, a tinted shape, a
   bloom gamma other than 1, the shafts' second composite): PLAN §6's open points, unchanged.
6. The old path stays behind a test flag until tranche 8, with a check on the witness frames (as `post-tone-verify.flag`).

## 1. What the family draws today (verified)

`native/world/yebis_glare.cpp` (`BuildGlare`), `yebis_shafts.cpp` (`AlphaRDraw`, `BuildShaft`, `AppendShafts`), drawn by
`GlareStage` through `PostDraws` with the library's programmes found by their pins:

| Step | Draws | The library's technique | Surfaces |
|---|---|---|---|
| `Alpha_R` (areas with shafts) | 1 | `Alpha_R` (alpha only, write mask 8) | the scene's alpha |
| Source chain | 1 to 2 | `MadT1_S1` (a 2:1 bilinear copy) | RGBA16F, halved while at least the base |
| Bright pass | 1 | `BrightPassT1_[Vignette_]THRESHOLD_LUMINANCE_REMAP_S1` | RGBA16F at the base (0.375 of the width at the witness zones) |
| Levels (6 to 8, capped by the base width) | 2 a level, plus a 2:1 copy from the third on | `MadT1_S{taps}`, taps 3, 7, 11 or up to 15, horizontal then vertical, a weight a channel (radii in the wavelengths' ratio) | R11G11B10 |
| 2x of the small levels | 0 to 4 | `MadT1_S4` (four taps at 0.375 texel) | R11G11B10 |
| Composite | 1 | `MadT{N}` or `MadLumGammaT{N}` (N levels, a weight each, a luminance gamma) | R11G11B10 at the base |
| Shafts (a record) | 1 mask, 1 to 4 radial blurs, 1 add | `LightShaftMask_SM3`, `TexCoordMadT1_S{2,3,4}`, `MadT2` | R11G11B10 |
| Output | 1 | `MadT1_S4` | R11G11B10, 2x the base (the tone map's t1) |

20 to 35 draws a frame, all small. Their GPU time has never been measured: **open**, to read with `rt-gpu-measure`'s kind
of timer before arguing a cost either way.

## 2. The programmes to write (verified on their disassembly)

Extracted by their pins and disassembled with their literals as bits:
`evidence/I-0138/artifacts/tranche-2-programmes/` (`extract_pinned.py`, 34 listings). All shader model 4 without the
refactoring flag, as tranche 1's: `fxc /T ps_5_0 /Gis` with `mad()` where the original has one gives the same operations.

| Ours | In place of | What it computes |
|---|---|---|
| `taps` (a count as a compile-time constant: 1 to 15) | `ps_MadT1_S{n}` with `vs_2D_OffsetTex2D{n}` | `sum of tex(uv + offset[i]) x weight[i]`, in the original's order: tap 1 first (`mul`), then tap 0, 2, 3 ... (`mad` each); the first eight coordinates added in the vertex programme and interpolated, the others added in the pixel programme to coordinate 0 |
| `taps_given` (2, 3, 4) | `ps_MadT1_S{n}` with `vs_2D_Tex2D{n}` | the same sum, the coordinates given whole at the vertices (the shafts' radial blurs) |
| `bright` (with and without the vignette) | `ps_BrightPassT1_*` | `c = t0 x e`; `l = mean(c)`; `n = c / (l + 2^-22)`; `d = max(l - threshold, 0)`; the remap `(d + sqrt((d + r.x) d + 1) - 1) r.w` where `d > 0`; `x 1/F`, clamped to [0, 64000]; alpha kept |
| `add` (N = 2 to 8, with and without the luminance gamma) | `ps_MadT{N}`, `ps_MadLumGammaT{N}` | `s = sum of t[i] x w[i]` (t1 first, then t0, t2 ...); with the gamma: `s / (mean(s) + 2^-22) x clamp(mean(s), 0, 64000)^g` |
| `shaft_mask` | `ps_LightShaftMask_SM3` | a polynomial arctangent of the vignette-space direction, four film-grain samples around it, the bright pass's colour above a depth threshold, a four-term exponential falloff of the distance (59 instructions; transcribe line by line) |
| `alpha_from_depth` | `ps_Alpha_R` | `saturate(s d + o)` into the alpha |

- **A draft was compiled and compared** (`draft-post_glare.hlsl` and `compare-draft.txt` in the same folder; not built into
  the module, never drawn): for the taps (1, 3, 4, 5, 7, 9, 11, 13, 15, with their vertex programmes at 1, 4 and 15), the
  given coordinates (2, 3, 4), the add (2, 6, 7), the add with the gamma (3 to 8), the bright pass and `Alpha_R`, fxc gives
  the original's operations in the original's order in 26 of 27 cases; the 27th, the vignette bright pass, has
  `mad` then `mov_sat` for the original's `mad_sat` (the form tranche 1's tone map drew byte for byte). Two things learnt:
  under `/Gis` a `for` over the taps is kept as a loop unless it is marked `[unroll]`; the comparison is of the operations'
  names and order, the operands are for the tranche's tests.
- **Open** before writing: `shaft_mask` (not drafted: 59 instructions to transcribe), the shafts' vertex programmes
  (`vs_2D_Tex2D2`, `3`, `4`), the sampler of each input (already a field of the plan's draws, from the captures).
- The literals to keep as bits (tranche 1's lesson, the disassembly prints them rounded): `0x34800000` (2^-22),
  `0x477a0000` (64,000), `0x3eaaaaab` (1/3), the arctangent's five coefficients, the falloff's four exponents.
- The vertex programmes add a constant to the coordinate per output (`add o[i] = uv + offset[i]`) and scale the position by
  (1, -1) with z 0.1: ours do the same additions on the same three vertices, so the rasteriser interpolates as it does today.

## 3. Design (proposed)

- **`native/renderer/shaders/post_glare.hlsl`**, compiled by the build with fxc and IEEE strictness into headers, one
  entry a shape and a count (about forty small programmes; the build's loop over the counts, as `rt_reflect.hlsl`'s
  entries). A unit embeds them: `post_glare_programs.{h,cpp}` returns the pair for a draw's kind and count.
- **The block** (`native/renderer/post_params.h`): one struct for the family, 16 float4 of offsets and 16 of weights,
  the bright pass's and the composite's few scalars, about 550 bytes, in place of 4,096. `GlareDraw::block` becomes it;
  `Put(g, word, value)` becomes named fields (`yebis_glare_test`'s word-for-word check then compares the named fields with
  the captured words at the same offsets: the goldens stay the captured words).
- **`GlareDraw::technique`** (a string naming a library technique) becomes a kind and a count; `PostDraws` takes our
  programmes; `AcquireNativeGlarePrograms` and the pins it reads go from the default path.
- **`GlareStage`** keeps its surfaces, layouts and retirement: it is ours. Its seam is unchanged (the scene HDR and the plan
  in, the tone map's t1 out), as its header foresaw.
- **The composite above eight inputs**: `add` chained (eight, then the sum with the rest) only if a record asks for it;
  the only shipped world record that does, `m32_00_0100` (ten levels), is refused for its glare shape 3: leave the refusal
  and name it, rather than write an untested path.
- **The old path** behind `post-glare-old.flag`, and `post-glare-verify.flag` writing both t1 surfaces on a witness frame
  (the check declared as its own graph pass from the start: tranche 1's step review).

## 4. Tests (proposed)

| Test | Oracle |
|---|---|
| `post_draws_test` (T13) run on our programmes: every captured draw of the family (m37's 24, the shaft captures at m37, m31, m50, m40, m30) | the original's captured output, 0 differing bytes on the capture's adapter; T13's rule elsewhere, unchanged |
| `glare_stage_test` (T14) on our programmes: the 228 surfaces at 9 zones | the same |
| Ours against the library's programmes on the same device: every draw of the plans at the 9 zones and at synthetic extents (an odd size, 16:10, 3840x2160) and inputs (NaN, infinities, negatives, 64,000 and above) | byte equality on any adapter |
| `yebis_glare_test`, `yebis_shafts_test` | unchanged: the plan's values are the same words |

T13 and T14 keep running on the library's programmes until tranche 8, beside the new runs. No tolerance is added.

## 5. Risks and open points

- R11G11B10 targets: the card converts the float output itself, in both paths the same way (a draw for a draw), so
  tranche 1's finding (K-0489) does not bite here; it would if draws were merged. **Do not merge draws** in this tranche.
- The 15-tap programme adds seven offsets in the pixel programme: the sum `uv + offset` must be the original's single add.
- Exactness holds on this PC's card and driver; elsewhere the comparison with the library's programmes on the same device
  is the proof (as tranche 1).
- What glare shape 3 and the custom glare draw is still unknown (PLAN §6): they stay refused, named in the journal's row.
- The film grain (the shafts' mask input) is still the executable's 256x256 block read at start: it is data, not a
  programme, and stays until tranche 8 decides (as tranche 1's dither table).

## 6. The other form (PLAN §5's first wording), if Thomas prefers it

A new pyramid: one bright pass, a chain of 2:1 downsamples with a wide filter, the levels composed while upsampling,
the shafts' mask and one radial pass inside it. About 12 draws instead of 20 to 35. It gives a bloom of the same family
but not the original's bytes: the per-channel radii and the level weights have to be matched by eye on sheets at every
area (Irithyll and the Undead Settlement first, the two ends of the threshold and luminance range), in both
colorimetries. It is the right tool for the Rekindled candidate; for the original colorimetry it trades a proof for a
judgement.

## 7. Commits (each builds and passes the unit tests)

1. `post_glare.hlsl`, its programmes' unit, the block; the taps and add shapes; T13 on them.
2. The bright pass, the shafts' three shapes, `Alpha_R`; T13 and T14 on our programmes, the comparison with the library's.
3. The frame and the prepare block: our programmes by default, the two flags; the records.
