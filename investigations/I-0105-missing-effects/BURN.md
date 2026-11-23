# I-0105 tranche 3: the burning objects' look

2026-10-28, static. DS3 1.15.2 read by a `ghidra-research` worker (Sonnet) with headless Ghidra on the mirror
`ghidra-mirror-i0066` (read-only; scripts [I0105Str.java](I0105Str.java), [I0105BuiltinRows.py](I0105BuiltinRows.py),
[I0105Callers.java](I0105Callers.java) and I-0099's `I0099Decompile.java`); outputs `evidence/I-0105/burn/`. The pilot re-read
the three facts the code rests on (marked « checked »).

## Answer

DS3 draws a burning object with its material's own burn programme (`GXFlver_ColDifSpcBumpIbl_Burn`, `..._RNM_Burn`) and drives the
burn by one constant, `FC_BlendRate` (cbMatDynParam +0xC0, read by the Burn pixel programmes, `burn_layouts.tsv`):

1. The material's GX item `GX13` (parser 141070, checked `decomp11.txt` l.149-159) stores its three data words at material +0xF8,
   +0xFC, +0x100 and sets +0xE9.
2. The burn update `667430` (an `NS_SPRJ::ObjIns`, through 66B430 and 666C60) writes, for each material whose MTD is one of the two
   Burn MTDs, a parameter row of kind 7 (target.x = the burn progress f, 0 to 1; strength 1) into the object's
   `GXMaterialParamModifier` (`[[obj+0x290]+0xF8]`), and removes it when f is 0; the draw item's parameter owner is that object
   (INFERRED for the map objects the native walks: it reads `[item+0x6E8]` for the disp items, `entity+0xA40` for the draw entities)
   (`D3DA20`'s descriptor +0x20 = `item+0x6E8`, the pointer the model instance stores there; the item's identity INFERRED, its base
   constructor encrypted). `BurnCloth_cloth` materials get a row keyed « BlendRate » instead (a generated SPX parameter, which the
   native already animates by key).
3. At each draw `22D620` copies the material's dynamic block (+0xD0..) and, when +0xE9 is set, moves +0x100 toward the kind-7 row's
   target by its strength (checked: `decomp7.txt` l.248-251, `146850(rows, name, 7)`); `22E930` writes it at cbMatDynParam +0xC0
   as `{d, 0, 0, 0}` (checked: `decomp8.txt` l.820-828), and, with +0xDC or +0xE9, the +0x30 block from +0xF8/+0xFC.

Natively before this tranche: no GX13 (nor GX01) parse (so +0xE9 and +0xDC clear, no +0xC0 and no +0x30 from them) and no reader
of kind-7 rows (the built-in parameter table's selectors are 1, 2, 3, 26, 27 only). The Burn pixel programmes read `FC_BlendRate`
(all 20 of them, `burn_layouts.tsv`), which nothing wrote: their rows were refused (INFERRED from the code, `PackShaderConstant`
Missing for a read word nobody wrote; no witness row seen), so the Burn materials' meshes were likely not drawn at all. After
this tranche they draw, with GX13's value or the creation zero.

## The native code

- `18_resource_admission.inc`: GX13 (+0xF8..+0x100, +0xE9) and GX01 (+0xF8, +0xDC) parsed as 141070.
- `09_model_admission.inc`: the cbMatDynParam block +0xC0 from +0xE9 and +0x100, moved toward the draw's kind-7 row (name match,
  else the first universal row); the +0x30 block already followed +0xE9.

## Not settled

- Which materials carry GX13 (data: the map FLVERs' GX items, Archdragon Peak's burnable objects); no cached programme of
  `GXFlver_ColDifSpcBumpRNM_Burn` exists, so its constant layout is not read.
- Elden Ring: not read for this mechanism (a DS3 material constant, no D3D12 seam).
