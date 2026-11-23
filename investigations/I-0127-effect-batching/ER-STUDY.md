# I-0127: Elden Ring's redundant-state filtering before a draw (D-0195)

Read 2026-11-07 by a Sonnet `ghidra-research` subagent, headless and read-only on the ER 2.7.1 mirrors `er-mirror-i0072-a` and `-b`
(`investigations/I-0104-native-effect-method/Invoke-ErGhidra.ps1`), with [I0127Refs.java](I0127Refs.java), I0099Decompile.java,
I0100DispUsers.java and riprefs.py; outputs in `evidence/I-0127/er-read/`. OBSERVED by that read; the pilot (Opus) re-read
`141E8F2E0` and its call in `1419E3700` (`evidence/I-0127/er-read/d1.txt`): the root signature set only when it changes,
`SetPipelineState` (list vtable +0xC8) every time the tracker is dirty; the rest is the subagent's read (no decision rests on it). Builds on I-0071 and I-0073 ER-STUDY.

## What ER does

- **Dirty bits and dirty ranges, not values.** Every state setter stores unconditionally and marks dirty: a shader resource or
  constant slot widens a [begin, end) range and ORs the root-table slots covering it into a 64-bit mask (`141AD49D0`,
  `141ACD2D0`, `141BE5280`); shaders, blend, depth and raster set the pipeline tracker's dirty byte (`141AEE730`, `141ADDB90`,
  `141ACC280`, `141ACC2E0`, `141ACC730`). Setting the same value again rebuilds the table or relooks the pipeline.
- **The draw** (`141ACAF20`): the pipeline tracker `1419E3700` only when dirty (a key of the five shaders, the states, the target
  formats, the topology bucket; the PSO cache `1419F17E0`), then the dirty tables (`141ACD170`, a thunk to `141ACE220`), then the
  draw. **SetPipelineState is not skipped** when the looked-up pipeline is the one bound (`141E8F2E0` always calls it); the root
  signature is (`141E8F2E0`: set only when it changes, then every table dirty).
- **Tables**: each dirty table gets a fresh range of the shader-visible heap and one `CopyDescriptorsSimple` (or
  `CreateConstantBufferView` for a constant buffer) per descriptor, then `SetGraphicsRootDescriptorTable` unconditionally.
  Constant buffers are table entries, not root views; their bytes are copied into a fresh upload slice at every set.
- **The content filters ER has**: the root signature; the sampler tables (a CRC64 of the descriptor handles, a hash map, always
  on, `141ACDF70`); the SRV tables by the same hash only behind a byte with no writer found (`1447F5631`, dormant: HYPOTHESIS);
  the topology compared in some setters (`141AEE730`, `141A14320`); the vertex and index buffers set directly by the draw's
  setup, unfiltered (`141E8DAF0`, `141E8DA60`); the render targets on a dirty bit, unfiltered.

## What the native stage does with it (I-0127)

The native effect stage already has the engine's per-draw state whole (`NativeEffectDraw`), so it compares values instead of
tracking dirty bits: it sets each piece of state only when it differs from what the list holds (ER's mechanism made exact; ER's
defect, a table rebuilt and a pipeline set again for an identical value, not copied), keeps ER's root-signature filter, and
extends ER's sampler-table cache to the texture tables (by content within a frame: the views' handles are valid for the frame
that holds their records). Constants stay root views (one call a slot, no descriptor), which ER does not have.
