# I-0079 ER study: where texture and buffer memory comes from (2026-10-21)

Elden Ring 2.7.1 (`eldenring_2.7.1.0_25080141_1A354710.exe`), static only: read-only headless Ghidra 12.1.3 (below-normal
priority, `-readOnly -noanalysis`) on `er-mirror-i0070-a`; scratch exports in `%TEMP%/er-i0079` (not kept). All addresses are ER
VAs. OBSERVED = read in code; INFERRED = a reading. Slots are confirmed in the assembly. **Correction to the brief**: on
ID3D12Device, CreateCommittedResource is 0xD8, **CreateHeap is 0xE0**, CreatePlacedResource is 0xE8 (0xD0 is
GetCustomHeapProperties). GetResourceAllocationInfo is 0xC8, GetCopyableFootprints 0x130, CreateFence 0x120.

## 1. Placed or committed: who supplies a heap block

- OBSERVED, 141E928E0 (`FUN_141e928e0`): arm choice is `param_14 == 0 || *(param_14+0x48) == 0` -> CreateCommittedResource
  (+0xD8, 141E92CB5, HeapFlags arg = 0); otherwise CreatePlacedResource (+0xE8, 141E92C4F) with heap = `*(*(param_14+0x48)+8)`
  and offset = `*(param_14+0x50)`. Both arms exist for buffers and textures; this is the only CreatePlacedResource and the only
  CreateHeap user in the D3D12 wrapper range (scan of every `CALL [reg+0xD8/0xE0/0xE8]` in the whole image; the others are
  command-list calls or non-D3D12).
- `param_14` is the 4th argument of each creator (141E88450 textures, 141E98000 / 141E987F0 buffers, 141E9B200), itself the
  7th argument of the 141E89xxx wrappers (OBSERVED in asm: R9 -> `[RSP+0x68]` of the 141E928E0 call).
- OBSERVED, the placement is decided one level above, in the GX resource-manager methods:
  - Textures: **1419F13F0** (called by the TPF builder callback 141A02600, which jumps to it; builder vtable `GXCGTextureBuilder_TPF`
    at 142F0D388, 2D build = slot 0x50 = 141A02480). It calls `141B047A0` (= `XOR R9D,R9D; JMP 141B03A00`) only if
    `renderer->[7]` (the physical-memory manager) is non-null, then passes the filled 0x58-byte struct (`LEA [RSP+0x40]` at
    1419F1509-14F0) to 141E88BE0 -> 141E89BF0 -> 141E8B6F0 -> 141E88030 -> 141E88450 -> 141E928E0. If the placement call returns
    null the struct stays zero and the texture is committed.
  - Buffers: 1419F0BB0 -> `141B04790` (`MOV R9B,1; JMP 141B03850`) -> 141E89A10 -> 141E8B240 -> 141E97D50 -> 141E98000.
    Third kind: 1419F1580 -> `141B047B0` (`R9=0`) -> 141B03BB0 -> 141E89D40 -> ... -> 141E9B200. 141E987F0 (via 141E8B480 /
    141E88B90) has no placement call found (callers only reached through data tables).
  - Callers that pass 0 for the struct (always committed): 1419F4F80, 1419F7BD0 (partly), 141C62E70, 141C76770, 141C77AC0,
    141C78450, 141C78DE0 and other render-target/utility creators (OBSERVED in the asm before `CALL 141E88BE0`: `[RSP+0x38]=0`).
- OBSERVED, the **placed arm is taken for a texture when all hold** (141B03A00 asm 141B03A30-141B03AA2):
  1. manager exists (renderer config byte at config+0x190 non-zero, 141AF5440 -> 1419F0990 creates the 0x130-byte manager);
  2. desc flags (ushort +0x1E) & 0x60 == 0 (not render target, not depth/stencil; the same bits 0x20/0x40 select the clear value in
     141E928E0);
  3. desc kind byte (+0x1C) is not 2 or 3 (INFERRED: kind 2 is the cube path, 141A02630);
  4. static `DAT_143c27ed0` == 1 (its image value) so flag bit 7 of the desc must be 0; `DAT_1447f5719` (runtime, 0 in the image)
     must be 0 or desc bit 8 clear;
  5. the allocation info from `GetResourceAllocationInfo` (device +0xC8, 141E9E9F0, with a 4 KiB small-resource attempt first)
     has size <= manager+0x10 (**8 MiB default**, config +0x2C overrides), and size/alignment fit one heap block of the pool
     (141B04090: `size <= group+0x18`, `alignment <= group+0x20`; group alignment = `DAT_143c27ec8` = 0x10000 in the image);
  6. a heap block can be found or a new heap created (CreateHeap failure -> committed fallback).
  So large textures (> 8 MiB, i.e. big 4K/8K and some cubes/volumes), RT/DS and most render targets are committed. Static buffers
  use the same test with their own desc offsets (+4/+6), a pool group 0xA0 (or 0x70 when flags & 3), no RT/DS exclusion bits other
  than 0x60 in the buffer flags.
- Pool group choice for textures (OBSERVED, 141B03AD1-141B03ADF): alignment <= 0x1000 -> group at manager+0xD0, else group
  at +0x100. Buffers -> +0xA0 (or +0x70). Whether most TPF textures are placed in practice (size/alignment histogram) is NOT known.
- Initial data does not change the arm: the same struct is used with or without `param_4` (data); the data path (staging,
  COPY queue) is applied after either arm. Note `1419F13F0` forces the creation-state hint to 1 when data is present.

## 2. Heaps: creation, sizing, sub-allocation, release

- OBSERVED, CreateHeap (+0xE0) is called only in **141E9E530** (wrapper; `D3D12_HEAP_DESC`: SizeInBytes = block size,
  Properties = 1430A45E8 {DEFAULT} or 1430A4600 {CUSTOM, CPU_PAGE_PROPERTY=NOT_AVAILABLE, POOL L1} when kind bit0 is set,
  Alignment = 0x10000 (or 0x400000 if the requested alignment is > 0xFFFF and != 0x10000), Flags from 141E91C60:
  kind bit1 -> 0x04 DENY_BUFFERS, bit2 -> 0x40 DENY_RT_DS, bit3 -> 0x80 DENY_NON_RT_DS). Kind 6 = 0x44
  (ALLOW_ONLY_NON_RT_DS_TEXTURES, texture pools), kind 0xC = 0xC0 (ALLOW_ONLY_BUFFERS, buffer pools). **No CREATE_NOT_ZEROED
  (0x1000) and no CREATE_NOT_RESIDENT (0x800) anywhere.** Wrapper 141E89E50 builds the `CGPhysicalMemory` wrapper
  (vtable 1430A76B0, size 0x18, +8 = ID3D12Heap).
- OBSERVED, pool layout (141B04480): four groups of 0x30 bytes at manager+0x70/+0xA0/+0xD0/+0x100, each = {list A (default
  properties), list B (custom L1; only created if the non-local `QueryVideoMemoryInfo` budget at manager+0x38 is non-zero),
  kind, block size, alignment, critical section}. Default block sizes when config is 0: group0 **128 MiB** (kind 0xC),
  group1 **128 MiB** (kind 0xC), group2 **32 MiB** (kind 6, small-aligned textures), group3 **256 MiB** (kind 6); max single
  allocation 8 MiB. The renderer-config fields (141AF5440: config+0x194..0x1A4) that override them were NOT traced, so
  the runtime values are unknown. The manager also calls `IDXGIAdapter3::QueryVideoMemoryInfo` (slot 0x70, 141E8AFD0) for LOCAL
  and NON_LOCAL at init; no later use of the budget found (OBSERVED init only).
- OBSERVED, growth (141B04090): try existing blocks of the list (141B03EF0, vtable +0x30 of each allocator), else
  create one new heap of the group size (141B03E00), append it (141B04A10) and allocate from it. No cap on the number of
  heaps found; no budget check.
- OBSERVED, allocator class **`GXBS::GXPhysicalMemoryHeap`** (RTTI, vtable 143058310, source `...\Source\Base\CG\GXPhysicalMemoryHeap.cpp`,
  ctor 141C5ACF0, init 141C5B050): Allocate = slot 0x30 (141C5AE00, under a critical section), Free = slot 0x38 (141C5AED0). The
  inner allocator (141F1AAA0 init, **141F1B4D0 alloc**, **141F1B550 free**) is a first-fit allocator over a linked list of free gaps
  with a node pool (nodes 0x28 bytes, 0xFF0-byte blocks): sizes rounded up to 16, power-of-two alignment (min 16), the free walks
  the used list linearly by offset and coalesces neighbours. **Not a buddy and not TLSF** (INFERRED from the list code: first
  fit, address-ordered, O(n) free). Granularity is therefore 16 bytes inside a 64 KiB-aligned heap.
- OBSERVED, release: the allocation is carried by a 0x20-byte `CGUserDataMemoryDeleter` (vtable 1430A4208, ctor 141E9DF60,
  holds allocator + offset) stored in the resource wrapper at `+0x18` (141E904F0 is an XCHG store/release). When the
  `CGTexture2D` (vtable 1430A7030) dies, its dtor 141EAB780 first releases the ID3D12Resource (141E88110) then the base
  dtor (141E90490) destroys the deleter, whose dtor 141E9DFF0 calls `allocator->Free(offset)` (+0x38) **immediately, with no
  fence in this chain**. Any delay comes from the caller that drops the last reference (the delayed-release policy 2 /
  two-frame pin of I-0073 section 1, INFERRED to cover this). On creation failure the deleter is destroyed at once
  (1419F13F0). Empty heaps are trimmed by 141B04700 (via 1419F1850, called from 1419E77F0 when its `param_2 == 0`, INFERRED
  per frame): in each list, the **second and later fully-free heaps are Released**, one empty heap per list is kept.

## 3. Staging buffer: per call, no pool, no zeroing

- OBSERVED, 141E928E0 creates a **new committed UPLOAD buffer per call**: CreateCommittedResource (+0xD8, 141E92F00) with
  heap type 2, HeapFlags 0, initial state 0xAC3 (GENERIC_READ), size = total of GetCopyableFootprints (+0x130), maps with an
  empty read range (+0x40), copies, unmaps (+0x48) and Releases it at the end of the same call. No ring, no pool, no reuse (the
  persistent UPLOAD ring 141E8F770 is a separate per-frame path, I-0073 section 2).
- OBSERVED, CPU staging: textures are copied row by row with memcpy (141E92F68-ish loop, `FUN_14251e710`) into the footprint
  layout; **no memset of texture upload memory**. Buffers: one memcpy of the data, then a memset of the tail
  `bufferSize - dataSize` only (141E92F7C, `FUN_14251eb60` with R8 = size difference, JZ-skipped when equal).
- INFERRED: committed resources and the committed UPLOAD buffer are zero-initialised by the D3D12 runtime (HeapFlags 0); the
  placed path avoids that cost for the destination. The destination is created in the initial state from
  141E91A90/141E91F90 (COMMON-class), the copy list has no barrier (I-0073).

## 4. Residency APIs

- OBSERVED: none (**corrected by [I-0080 ER-STUDY](../I-0080-er-dx12-pitfalls/ER-STUDY.md)**: ER has MakeResident, Evict and SetResidencyPriority wrappers, 141EA2BA0 and 141EA1880, their callers not found). No `ID3D12Device::MakeResident` (0x110) / `Evict` (0x118), no `ID3D12Device3::EnqueueMakeResident` (0x170),
  no `SetResidencyPriority` (0x150) or `D3D12_HEAP_FLAG_CREATE_NOT_RESIDENT` (0x800) in the D3D12 wrapper range
  141E8xxxx-141EBxxxx; the hits of those slot offsets there (141E8DF8B, 141E8F54B, 141EA1A09, 141EA1C09, 141EA2D03, 141EA2D0B)
  were read as command-list calls (they sit in command-list setup functions; 141EA2BA0 = render-target binding). Only
  `IDXGIAdapter3::QueryVideoMemoryInfo` (init) exists. INFERRED: ER leaves residency to the OS/driver, no explicit budget management.
  Caveat: a whole-image scan only covered `CALL [reg+disp]`; an indirect call through other forms was not searched.

## 5. Not found / unknown

- The runtime values of the pool sizes and max allocation (config +0x190..0x1A4, source not traced), and the real placed vs
  committed ratio for TPF textures.
- What kind values 2 and 3 of the GX desc mean beyond "cube path" for 2, and the exact meaning of flag bits 0x80/0x100 gates.
- Whether 141E987F0 buffers (second buffer creator) are ever placed (no placement call found for them).
- Which thread/frame the delayed release hands the last reference back on (see I-0073 ER-STUDY, section 1).
- Implication for DS3 (INFERRED, not tested): ER keeps small/medium static textures and buffers in large 64 KiB-aligned
  default heaps (32-256 MiB blocks, first-fit sub-allocation, CPU-side only, no per-resource driver allocation or zeroing),
  while large and render-target resources are committed; each creation still pays a fresh committed UPLOAD buffer and a
  COPY-queue fence wait on the worker.
