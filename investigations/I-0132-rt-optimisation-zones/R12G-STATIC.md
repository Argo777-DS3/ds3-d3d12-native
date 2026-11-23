# R12g's static reading: Texture Quality and the options list's scroll

Two `ghidra-research` workers (Sonnet, GhidraMCP on DarkSoulsIII.exe 1.15.2, sha256 `ef5e07c5…` checked by `get_program_info`, read-only,
nothing written to Ghidra), 2026-11-11 (Paris), for I-0132 R12g. Their reports summarised here; the pilot re-read the parts the code
depends on against the repository's own records (sampler_inputs.h, sampler_bank.h, R12C-STATIC §8).

## 1. What DS3's « Texture Quality » changes

**Answer: only the material samplers' filter and anisotropy** (OBSERVED for every reader found; « no texture resolution change anywhere »
INFERRED: the TPF/DDS mip-parsing job and the quality object's listeners were not followed).

- The level word (the options owner `[144796298]+0xA8`, slot 0) is read by `ApplyGraphicsLevelsToGX_140ced9a0` (from `0x140D55190` →
  `0x140DFDD20`), which copies the twelve levels to `CopyGXQualityLevels_140023660` (GXRS+0x5320..+0x534C, unless the lock byte
  GXRS+0x5350). Each is the other's only caller.
- `UpdateGXQualityObject_140023790` rewrites the quality object from its table when a level changes; the Texture entry is two ints
  (+0x00 the filter level, +0x04 the anisotropy), passed to `SetTextureFilterAndAnisotropy_1400d3650` (its only caller, `140023ba5`), which
  maps the filter level through `{0, 6, 7, 8}` and writes sampler descriptors 12 and 13 only (filter at +0, anisotropy at +0x14), then
  recreates the sampler states that changed. No texture resource, mip, LOD bias or budget is touched.
- The game's own debug « Graphics Config » page (`FUN_140025310`, `FUN_140020760`) labels slot 0 **`TextureFilter`** and edits the pair as
  an enum {0, 1, 2, 3} and {1, 2, 4, 8, 16}.
- The levels Low to Max select the profiles {2,1} trilinear, {3,4}, {3,8}, {3,16} (sampler_bank.h's profiles, the native's copy).
- No « Mip », « .tpf » or streaming string beyond Havok's; `TpfTextureCreator_Create_140d4e100` and `GxDevice_CreateTexture_1419256d0`
  read no quality level.

Consequence for R12g: « textures and anisotropy as two rows » would be two rows for one filter. An anisotropic filtering row first took
Texture Quality's place (`5a4e02501`); Thomas kept the game's row (2026-11-11 15:20 to 15:22) and wished a real texture quality with the
anisotropy apart for the day the game's textures are upscaled ([TEXTURE-QUALITY](../../.project/wishes/TEXTURE-QUALITY.md)).

## 2. Whether the options list scrolls past its slots

**Answer: no, not as our page is built** (the mechanism OBSERVED in code, the conclusion INFERRED; not tested live beyond `r12g-pages1`'s
fifteenth row).

- The list base is `page+0x9D8` (L, vtable `1428adf88`): its element count L+0xB0, cursor L+0xB4, slots found in the film L+0xB8 (x) and
  L+0xBC (y), a y scroll L+0x2F8, total and visible rows L+0x2FC and L+0x300, the scroll bar's sub-object at L+0x180.
- `FUN_140A40500` counts the film's `Item_%d_%d` cells: 14 along x and 1 along y for `02_042_pc_graphicsetting.gfx`, so the list scrolls in
  pages of fourteen (`FUN_140A41A40`: total rows = ceil(count / 14), visible 1, the ScrollBarV shown when visible < total).
- A row takes the slot `Item_<list size>_0` when it is made (`FUN_140A90E20`): a fifteenth row points at `Item_14_0`, which the film does not
  have. The scroll callback (`FUN_140A905A0` → `FUN_140A8DCB0`) only shows or hides cells and sets captions; nothing re-binds a row's widget
  to another slot.

Consequence for R12g: our own scroll, a window of fourteen rows of our list built again (the rebuild path of R12C-STATIC §8.4) when the
game's cursor wraps past the window's edge (graphics_rows.h `ScrollAfterMove`).

## 3. The scroll bar

A third worker (the same method, and the film read with a small SWF reader): the film's `ScrollBarV` (sprite 97) holds `ArrowUp`, `ArrowDown`,
an `Outline` track (684 px) and **`Bar`**, a mask (ClipDepth) over the lit segments: the thumb is what `Bar` reveals, moved by its `_y`
and sized by its `_yscale` (OBSERVED in the film; that a scaled mask shortens the lit part INFERRED until seen).
- The bar's object is the list's +0x180 (S); `FUN_140A52F20(S, root, visible)` finds the three children, keeps `Bar`'s place (S+0x16C its
  y, S+0x170 its x, S+0x174 the travel to `ArrowDown`) and calls the draw `FUN_140A53700(S)`: `ShowHideWindow(S, total > visible)`, then
  `FUN_140A3D640(S+0xB8, x, position / total * travel + y0)` and `FUN_140A3D6A0(S+0xB8, 1.0, visible / total)` (OBSERVED in the disassembly:
  the two floats in XMM1 and XMM2). S+0xB8 is the list's +0x238.
- The cursor's moves (`FUN_140A442B0`, `FUN_140A42580`, `FUN_140A44950`) read the list's +0x2F8 (S's position): the list's +0x2F8 to +0x300
  are never written by us.
- Consequence: the game's bind of our rows hides the bar (its own rows never exceed its slots) and places `Bar` full length; its plain
  cursor moves do not redraw it (the thumb stayed placed after moves in `r12g-rows2`). Our page shows the bar and places `Bar` with the
  two setters at its next update after each build, and when the window or the list changes (02b `ShowRekindledScrollBar`). Not read: the
  game's other callers of the bar's draw (the help box, a confirmation box, a screen-mode change), which would hide it until the next build.

The workers' scripts and outputs (`I0066FindInstructions` on `0x5320]` and `0x4e08]`, `I0067FieldUsers` on 5360) stayed in the pilot's
scratchpad; their findings are the ones above.
