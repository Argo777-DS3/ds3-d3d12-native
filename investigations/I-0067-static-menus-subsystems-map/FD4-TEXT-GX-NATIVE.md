# FD4 dialog text on the GX 2D layer, as a native checklist (I-0067, Task 9)

STATIC (Ghidra mirror of `DarkSoulsIII.exe` 1.15.2.0) + game data read offline (`data1/font/DbgFont14h.*`, `data1/shader/debugFont.shaderbnd`).
Nothing was launched. Status words: **STATIC** = read in code or data, **INFERRED** = deduced, **OPEN** = not known statically.
Context: [`SUBTITLES-DIALOGUE-NATIVE.md`](SUBTITLES-DIALOGUE-NATIVE.md) section 7 (event text and the legacy talk dialog are read by FD4 dialogs),
[`FONTS-LANGUAGES-NATIVE.md`](FONTS-LANGUAGES-NATIVE.md) section 4 (old-style text), `MENUS-MAP.md` section 34 (old dialogs recorded and replayed in the 2D layer `+0x40`).
Evidence (outside Git): `evidence/I-0067/queries/fd4-text/` (`debugFont-vs.asm`, `debugFont-ps.asm`, `dbgfont14h-coverage.txt`).

## 0. Verdict

1. **The FD4/Ez dialog text is drawn by the GX debug-font renderer, not by Scaleform (STATIC from the command-buffer text record to the GX submit; the edge from the FD4 part `SetText` to the record is INFERRED).** The only font the executable names for it is `DbgFont14h`
   (`font:/DbgFont14h.tpf`, `font:/DbgFont14h.ccm`, shaders `debugFont_vs/ps`): 17 pages of 512 x 512 BC1, **15,165 glyphs including all of Latin-1,
   Latin Extended-A/B, Greek, Cyrillic, kana, 10,277 CJK ideographs and 2,350 Hangul syllables** (so the 13 shipped languages render with one font).
2. **The pilot deliberately does not build this pipeline**: `game_bootstrap.cpp` `CompleteNativeResourcePhase` logs
   `native_optional_visual_deferred family=debug_font` and skips `FUN_142355720` (it loads the font table and the texture through two resources, then calls `FUN_1403A4AC0`, the D3D11 resource creation, and `FUN_1403A4FE0`, the font load).
   Consequence: **the printer object `_DAT_144964928` and its font entries never exist in the native build**. A replayed text record (`FUN_1407F9B90`, `FUN_142378630`)
   then reaches a null printer: `FUN_142378630` writes through `*_DAT_144964928`, `FUN_1407F9B90` dereferences the lookup result, and both assert through
   `FUN_144E60057` when the singleton is null. So **an FD4 dialog text in the native build faults or asserts rather than drawing nothing**, unless no such record is
   ever produced (open point 1) or a native guard exists elsewhere (none seen). The Scaleform caption page (NPC lines, subtitles) is unaffected.
3. The GPU side is tiny: **one vertex shader, one pixel shader, one `Texture2DArray` (17 BC1 slices, INFERRED from the 17 TPF textures and the array declaration of the pixel shader), one 6-float4 constant block, indexed triangles of
   24-byte vertices**. Layout and vertex generation are CPU code the engine already runs; the native only has to provide what `FUN_1403A4AC0` creates.
4. Whether any FD4 dialog is instantiated in the release PC build is **OPEN** (`SUBTITLES-DIALOGUE-NATIVE.md` 7A, 7D); the debug draw (FPS and similar,
   `Graphics.DbgDraw.*` keys) uses the same renderer.

## 1. The path from a dialog to pixels (STATIC unless marked)

| Step | Code | What happens |
|---|---|---|
| 1. text request | FD4 dialog part `SetText` (`FUN_141A0C180`, vtable `+0xB8` of the part) -> `FrpgMenuDc` (vtable `142815DD8`) text primitive | the Ez drawing context records, it does not draw |
| 2. record | `FUN_140757520` (in the `FrpgMenuDc` code block; **INFERRED** to be its text primitive, it is not a slot of the vtable `142815DD8`) -> `FUN_1407F84A0` | appends to the `MenuDrawCommandBuffer` (rotating 256 KB stores): replay pointer `FUN_1407F9B90`, a 16-byte rect (4 floats), the length in bytes, a **UTF-16 copy of the string** |
| 3. replay (frame update, layer `+0x40` pass, `MENUS-MAP.md` 34) | `FUN_1407F9B90` | looks up the font printer `FUN_142355560(_DAT_144964928, id)` with `id = [[[ctx+0x28] + 0x10 + [[ctx+0x28]+0x20]*8] + 0x40] + 0x84`, runs `FUN_141923340` with a null vertex output (the measure pass resolves the alignment into a start position and an end corner), scales that position by two ratios (`_DAT_14489D5D0/D4`), then calls the Ez draw `FUN_14232FB20(ctx, pos, text)` |
| 4. GX command | `FUN_14232FB20` -> `FUN_14237E020` | records into the GX layer command stream: replay pointer `FUN_14237C7E0`, position x, y, z, UTF-16 string |
| 5. GX replay | `FUN_14237C7E0` -> `FUN_142378630` (`FUN_1423782C0` is the offset-taking twin of `FUN_142378630`; its caller is not visible in the exports; both select by the mode at `[[param_1+0x18]+0x70]`: `< 2` screen-space 2D through `FUN_1403A4520`, `< 4` 3D with a matrix) | sets rectangle, colour, scale on the printer (`+0x1C..+0x40`), picks the buffer pool (`[_DAT_144961AB0 + 0x80]` selects `+0x70` or `+0x10`) |
| 6. submit | `FUN_1403A4520` (2D) / `FUN_1403A53E0` | see section 3 |

**Layer.** The dialogs reach the GX 2D layer `+0x40` replay (`FUN_1403A6030`, blend state 7, `MENUS-MAP.md` 34), after Scaleform. The pixel order inside the
layer is the command order (fade plates and dialog rectangles are recorded by the same buffer).

## 2. The font and its GPU resources (STATIC)

Resource names (`1429AF830`, `1429AF860`, `1429AF890` in a descriptor triple at `144660228`; shaders also as `title:/Font/debugFont_ps.ppo` and `debugFont_vs.vpo`
at `142959658`, loaded in the title resource step of `FUN_140D59E90`):

| File | Archive entry | Content |
|---|---|---|
| `font:/DbgFont14h.tpf` | `data1/font/DbgFont14h.tpf.dcx` (452,384 bytes) | 17 textures, each 512 x 512 DXT1 (BC1), one mip, 131,200 bytes (128-byte header + 131,072) |
| `font:/DbgFont14h.ccm` | `data1/font/dbgfont14h.ccm.dcx` (116,515 bytes, 445,680 uncompressed) | header with count 15,165 at `+0x10` and the glyph table offset `0x13F38` at `+0x18`; 24-byte glyph records starting with the code point |
| `shader:/debugFont.shaderbnd` | `data1/shader/debugFont.shaderbnd.dcx` | two members: `debugFont_vs.vpo` (1,276 bytes), `debugFont_ps.ppo` (752 bytes), both plain DXBC (SM 5.0) |

**Coverage of `DbgFont14h.ccm`** (code points counted from the glyph table; `dbgfont14h-coverage.txt`): ASCII 95; Latin-1 96; Latin Extended-A/B 39; Greek 48;
Cyrillic 66; punctuation and symbols (U+2000-2BFF) 304; kana 177; CJK ideographs 10,277; Hangul 2,350; full-width forms 162. Against the bitmap
fonts of section 4 of `FONTS-LANGUAGES-NATIVE.md` (`DSFont24`, `TalkFont24`: 7,117 and 7,112 glyphs, no Hangul, no Latin Extended, 8-10 Latin-1 letters),
this is the only font of the executable that covers the accents of French, German, Spanish and Polish and Korean, which is why it is the likely FD4 font (the
names `DSFont24` and `TalkFont24` appear in no string of the executable).

**Shaders (disassembled, `debugFont-vs.asm`, `debugFont-ps.asm`).**

```
VS (vs_5_0)  cbuffer cbPerFrame (cb0, 6 float4): g_matWVP row_major float4x4 @0 (only rows 0, 1 and 3 are read), g_reqScreenSize @64 (unused),
             g_color @80, g_ScreenCenter @96 (unused)
  inputs  v0.xy POSITION, v1.xyz TEXCOORD (u, v, array slice), v2.xyzw COLOR
  o0.xy = v0.x * cb0[0].xy + v0.y * cb0[1].xy + cb0[3].xy ; o0.zw = (0, 1)
  o1 = v1.xyz ; o2 = v2 * cb0[5]
PS (ps_5_0)  s0 fontSampler, t0 fontTexture : Texture2DArray<float4>
  o0 = t0.Sample(s0, v1.xyz) * v2
```

So the vertex is (INFERRED from the signature, the stride 0x18 in the layout and submit code, and the packed 32-bit colour the layout writes per glyph) **24 bytes: float2 position (8), float3 texcoord with the page index in z (12), 4 bytes of colour (as unorm8)**; the colour in the
constant block (`g_color`) multiplies the vertex colour. The glyph sheets are sampled RGBA and multiplied: the BC1 data is white glyphs on black with
the alpha carried by the colour/black convention of the sheet (**OPEN**: whether the engine relies on black = transparent through the blend state or on an
alpha channel; BC1 has at most 1 bit of alpha).

## 3. Layout and submission (STATIC)

**Layout `FUN_141923340(text, fontTable, layout*, box*, outVertices, outExtent*, endChar)`** (CPU, no GPU access):

- `fontTable`: `+0xC` float (line height basis), `+0x10` a 128-byte ASCII to glyph-index table, `+0x90` the sorted code point array, followed by glyph records of 32
  bytes (8 dwords; the first three floats are summed for the advance, the remaining fields, presumably texture rectangle and page, were not decoded); non-ASCII code points are found by binary search; surrogate pairs are folded to one
  key; `'\n'` (10) starts a new line; `'\r'` (13) is ignored.
- `layout*`: `[0]` size, `[1]` extra spacing, `[2]` line step, `[3]` scale; byte `+0x10` selects fixed-width cells: full-width and non-ASCII-Latin glyphs get the
  full size, ASCII and half-width katakana (U+FF61..FF9F) half of it (constants `143D7EDB4` = 0.5, `143D7EE10` = 1.0).
- `box*`: `[0..1]` origin, `[2..3]` the far corner (right, bottom; the wrap width is `[2] - [0]`), `[4]` horizontal alignment 0 left / 1 centre / 2 right, `[5]` vertical 0 top / other values centre or bottom; an optional
  wrap flag breaks lines at the box width (`FUN_141924170` closes a line and applies the alignment offset).
- Output: **4 vertices of 0x18 bytes per glyph** (`FUN_141924070` writes one glyph), plus four floats through the extent pointer (start position x, y and end corner x, y).
- Inline colour tags `#c[RRGGBB]` and `#a[AA]` in the text are parsed (`FUN_141923DA0`, counted by `FUN_141923100`) into a packed 32-bit colour, default `0xFFFFFFFF`, written to the vertices.
- `FUN_141923100(text, fontTable)` returns the vertex count (4 per drawable glyph).

**Submission `FUN_1403A4520`:**

1. vertices `n = FUN_141923100`, indices `6 * n / 4`;
2. one dynamic allocation of `2 * indices + 24 * n` bytes from the GX dynamic buffer pool (`FUN_14213B690`, ring of `0x40000`-byte buffers created in
   `FUN_1403A4AC0`): **vertices first, then the 16-bit indices** at `buffer + 24 * n`;
3. indices per glyph `k`: `4k, 4k+1, 4k+2, 4k+2, 4k+3, 4k` (two triangles);
4. GX state id `0x39` (`ctx+0x278`, `[ctx+8]+0x28`), buffer offset stored in `ctx+0x400`, the constant block assembled inline on the stack (the 4 x 4 matrix from the printer at
   `+0x60..+0x9F`, 8 zero bytes, `g_color` from the colour bytes `+0x42, +0x41, +0x40, +0x43` divided by 255 in that order, and two floats at offset 96 that the vertex shader does not read) and passed to `FUN_141920300`, then `FUN_141921390(ctx, ctx+0x10, indexCount, 0)` (taken as the indexed draw):
   an indexed draw of `indexCount` indices.

**Resources created by `FUN_1403A4AC0` (D3D11 objects through the GX FW wrappers; names INFERRED from the call order and descriptor shapes):** the ring of dynamic buffers (`FUN_1421392C0` with 0x40000 bytes and alignment 0x10, then one entry per frame slot through `FUN_142139450`; `FUN_1419254F0` creates a separate buffer from a 0x70-byte descriptor), then `+8` a blend state (`FUN_141925360`), `+9` a depth-stencil state (`FUN_1419253B0`), `+0xA` and `+0xB` two rasterizer states
(`FUN_141925400`, differing in one flag), `+0xC` a sampler state (`FUN_141925450`), and a 0xB0-byte printer object; the shaders and the texture array come from
the title resources. The exact blend, rasterizer and sampler values are in the stack descriptors of `FUN_1403A4AC0` (not decoded: **OPEN**, live row DT3).

## 4. Native checklist

| # | Requirement |
|---|---|
| H1 | Build the debug-font pipeline in the native frame instead of deferring it: vertex shader, pixel shader (DXBC above, or an equivalent HLSL), 24-byte input layout (`POSITION` float2, `TEXCOORD` float3, `COLOR` unorm8x4), one `Texture2DArray` BC1 512 x 512 x 17, one linear/point sampler `s0`, constant block `cb0` of 6 float4. |
| H2 | `FUN_142355720` / `FUN_1403A4FE0` must run (or be reproduced) so that the font table (`.ccm`) and the texture slices come from the engine's own resource loader (`FUN_142338CA0`, names at `printer+0x90/+0x98`, defaults `143D27280`/`143D271A0`); the native side must not re-read the archive. |
| H3 | Vertex and index memory: the engine writes them into the "GX dynamic buffer" returned by `FUN_14213B690`; the native GX must supply that allocator with CPU-visible memory and perform the draw on the 2D layer `+0x40` replay, after Scaleform. |
| H4 | Draw state: indexed triangle list of `6 * quads` 16-bit indices, no depth test or write (2D screen space, the VS writes z = 0, w = 1), alpha-capable blend (layer `+0x40` uses blend state 7, `MENUS-MAP.md` 34), culling off; confirm against the stack descriptors (DT3). |
| H5 | Constants: rows 0, 1 and 3 of `g_matWVP` (a 2D affine matrix to clip space, built from the 1920 x 1080 canvas), `g_color` (modulates the vertex colour). |
| H6 | The ASCII/UTF-16 path needs no language logic; one font covers all 13 languages (section 2). |
| H7 | Ordering and fences: the text is recorded in the same GX command stream as the fade plates; no separate flush is needed. |
| H0 | Until H1-H5 are done, **guard the replay**: a recorded text must not reach the null printer (the faults of verdict 2); either keep FD4 dialog creation off in the native build or implement the pipeline first. |
| H8 | A text-less witness is not a pass: the check is that a known dialog string appears (live rows). |

**Static witnesses (SW):** SW1 `DbgFont14h.tpf` 17 x (512 x 512 DXT1); SW2 `.ccm` 15,165 glyphs with the coverage counts; SW3 the two DXBC programs and the vertex format;
SW4 indices per quad `4k, 4k+1, 4k+2, 4k+2, 4k+3, 4k`; SW5 `native_optional_visual_deferred family=debug_font` in `game_bootstrap.cpp` (lines 8661-8664).

**Live rows (DT):**

| Row | Action | Expected |
|---|---|---|
| DT1 | Reference run: read `_DAT_144961AB0` (renderer) and `_DAT_144964928` (printer map) after the title | both non-null; the map holds the printer ids used by `FUN_142355560` |
| DT2 | Reference run, break on `FUN_1403A4520` during a dialog (for example a message box or an event message) | `n` vertices, `2*6n/4 + 24n` bytes, state `0x39`, indexed draw |
| DT3 | Dump the descriptors passed to `FUN_141925360/1419253B0/141925400/141925450` in `FUN_1403A4AC0` (stack at the calls) | the blend/depth/raster/sampler values for H4 |
| DT4 | Native run with the pipeline enabled: the same dialog | the text is visible at the reference position; one draw per string |
| DT5 | (Thomas's agreement) a run in another language | accents and CJK/Hangul glyphs from the same sheets |

## 5. Open points

1. Which FD4 dialogs are instantiated and shown in the release PC build (`SUBTITLES-DIALOGUE-NATIVE.md` 7A, 7D; DT2 settles it).
2. The id -> printer map `_DAT_144964928` (how many printers, which sizes): `FUN_142355560` returns the entry for the id read from the layer; the set of ids was not enumerated.
3. The values of the blend, depth-stencil, rasterizer and sampler states (DT3) and the exact use of the BC1 alpha.
4. `EzMenuFontPrinter` and `FrpgMenuFontPrinter` (RTTI names exist at `1445899C8`, `1445899F8`): their relation to the text printer map was not read; the replay
   path above (record -> `FUN_1407F9B90` -> Ez draw) does not need them by name.

## 6. Exports

Cited functions are exported to `decompiled/` (`DS3-1152-F-<RVA>`): `FUN_141923340`, `1403A4420`, `1403A4520`, `1403A53E0`, `1403A4AC0`, `142355720`, `142355560`, `1423555C0`,
`1407F9B90`, `1407F84A0`, `14232FB20`, `14237E020`, `14237C7E0`, `14237C140`, `1423782C0`, `142378630`, `140D59E90`, `1407F7CD0`, `1407F8CD0`, `1407F8DC0`, `140757520`, `141923100`, `142355720` (also), `1403A4FE0` (not exported).
