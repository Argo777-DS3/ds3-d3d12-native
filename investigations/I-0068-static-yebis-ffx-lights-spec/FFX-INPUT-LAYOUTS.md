# I-0068 part AM, point 2: the FFX input layouts, read from the executable (the LAYOUT live read is closed)

Request (coordinator, 2026-10-11): reduce the live-only points of EFFECTS-IMPLEMENTATION-CHECKLIST §11 and LIVE-CHECKS-CONSOLIDATED §6. The only item that had to be read **before** coding step 3.3 was the input-layout formats of the 8 E01 programmes ("one pass-through read of the input element descs at layout creation"; "the capture holds no layout records"). They are static tables of the executable.

Labels: **STATIC** (executable bytes, decompilation, shader reflection), **INFERRED**. Nothing was launched.

## 1. Where they come from

`FUN_140291520` (the FFX shader-set builder, called from `24C300`) creates the input layouts itself, in two loops, through the generic helper `FUN_14006e450(out, device, elements, count, vsObject, …)` (→ `1419250A0` → `14192F090` → D3D11 `CreateInputLayout`, slot +0x58):

| Loop | Programmes | Element-table pointers | Element counts |
|---|---|---|---|
| 1 (call site `140292F73`) | the 158 VS of the shader set (index 0..157, `ffx-shader-set.tsv` array `apuStack_1d88`) | qword table `14473EBC0[i]` | dword table `143D22320[i]` |
| 2 (call site `1402946EC`) | 127 further entries (second name array, not part of E01–E05; §4) | qword table `14473E7C0[i]` | dword table `143D22120[i]` |

The element record is GX's own **28-byte** record (`FUN_14192F090` converts it to a 32-byte `D3D11_INPUT_ELEMENT_DESC`):

| Offset | Field | Conversion |
|---|---|---|
| +0x00 | semantic enum | `FUN_141929A90`: 0 `POSITION`, 1 `BLENDWEIGHT`, 2 `BLENDINDICES`, 3 `NORMAL`, 4 `PSIZE`, 5 `TEXCOORD`, 6 `TANGENT`, 7 `BINORMAL`, 8 `POSITIONT`, 9 `COLOR` (strings at `143A3A208`, `218`, `228`, `238`, `240`, `248`, `260`, `268`, `280`, `254`) |
| +0x04 | semantic index | copied |
| +0x08 | format | `FUN_1419294A0`: the DXGI_FORMAT value, unchanged |
| +0x0C | input slot | copied (always 0 in the tables) |
| +0x10 | aligned byte offset | copied |
| +0x14 | input class | `FUN_141929B70`: 1 = per instance, else per vertex (always 0 here) |
| +0x18 | instance step rate | copied (0) |

`Get-FfxInputLayouts.ps1` (this folder) decodes both loops from the executable, resolves the VS names, and checks each layout against the VS input signature (`ffx-vs-signatures.tsv`, chunk ISGN). Output: `evidence/I-0068/queries/ffx/layouts/ffx-input-layouts.tsv` (one row per loop and index: table address, count, slot-0 stride, element list, signature, mismatches).

**Validation (STATIC):** for all 158 loop-1 VS, every ISGN input exists in the layout with the same semantic and index and with at least as many components as the signature mask: **0 mismatches**. The layouts carry extra elements the VS does not read (for example GXFfxtexture: NORMAL/TANGENT/BINORMAL, COLOR1 as four floats, TEXCOORD1/2), which D3D accepts and D3D12 PSOs accept too.

## 2. The layouts (all slot 0, per-vertex, step 0)

Eleven layouts serve the 158 VS. `fmt@offset`; semantic names as in the signatures.

| Table | Stride | Elements (semantic format@offset) | VS indices (shader-set table) |
|---|---|---|---|
| **A** `143D22730` | 76 | POSITION0 R32G32B32_FLOAT@0, NORMAL0 R8G8B8A8_SNORM@12, TANGENT0 R8G8B8A8_SNORM@16, BINORMAL0 R8G8B8A8_SNORM@20, COLOR0 R8G8B8A8_UNORM@24, COLOR1 R32G32B32A32_FLOAT@28, TEXCOORD0 R32G32_FLOAT@44, TEXCOORD1 R32_FLOAT@52, TEXCOORD2 R32_FLOAT@56, TEXCOORD3 R32G32_FLOAT@60, TEXCOORD4 R32G32_FLOAT@68 | `texture` 0-2, `bumpTexture`, `softTexture`, `softBumpTexture` (3-5, 64-70, 75-80, 132-138), `bumpDistorsionSoftTexture` 154-155, `softTracer` 156-157 (non-tessellated, triangle list) |
| **B** `143D22630` | 88 | POSITION0 R32G32B32A32_FLOAT@0, COLOR0..3 R8G8B8A8_UNORM@16/20/24/28, COLOR4 R32G32B32A32_FLOAT@32, TEXCOORD0 R32G32B32A32_FLOAT@48, TEXCOORD1 R32G32B32A32_FLOAT@64, TEXCOORD2 R32G32_FLOAT@80 | `multiTexture`, `softMultiTexture` 6-9, 81-84 |
| **C** `143D235B0` | 32 | POSITION0 R32G32B32_FLOAT@0, COLOR0 R8G8B8A8_UNORM@12, COLOR1 R32G32B32A32_FLOAT@16 | `colorOnly` 10, 85 |
| **D** `143D225D0` | 36 | POSITION0 R32G32B32A32_FLOAT@0, COLOR0 R8G8B8A8_UNORM@16, COLOR1 R32G32B32A32_FLOAT@20 | `pointSprite` 11, 86, `tessellatePointSprite` 40, 115 |
| **E** `143D22C00` | **112** | POSITION0 R32G32B32_FLOAT@0, COLOR0 R8G8B8A8_UNORM@12, COLOR1 R32G32B32A32_FLOAT@16, TEXCOORD0 R32G32B32A32_FLOAT@32, TEXCOORD1 R16G16B16A16_UNORM@48, TEXCOORD2 R32G32_FLOAT@56, TEXCOORD3 R32G32B32A32_FLOAT@64, TEXCOORD4 R32G32B32A32_FLOAT@80, TEXCOORD5 R32G32B32A32_FLOAT@96 | `tessellate{,Bump,Soft,SoftBump}Texture` 12-35, 87-110 (the commonest E01 family) |
| **F** `143D22D00` | 80 | POSITION0 R32G32B32_FLOAT@0, COLOR0 R8G8B8A8_UNORM@12, COLOR1 R32G32B32A32_FLOAT@16, TEXCOORD0 R32G32B32A32_FLOAT@32, TEXCOORD1 R8G8B8A8_UNORM@48, TEXCOORD2 R8G8B8A8_UNORM@52, TEXCOORD3 R32_FLOAT@56, TEXCOORD4 R32G32B32A32_FLOAT@64 | `tessellateTracer{,Bump}Texture` 36-37, 111-112, soft and distortion tracers 146-153 (E02) |
| **G** `143D22900` | 48 | POSITION0 R32G32B32_FLOAT@0, COLOR0 R8G8B8A8_UNORM@12, COLOR1 R8G8B8A8_UNORM@16, COLOR2 R32G32B32A32_FLOAT@20, TEXCOORD0 R32G32B32_FLOAT@36 | `tessellateLineColorOnly` 38, 113 |
| **H** `143D22990` | 60 | as G, plus TEXCOORD1 R32G32B32_FLOAT@48 | `tessellateQuadLineColorOnly` 39, 114 |
| **I** `143D22A70` | **160** | POSITION0 R32G32B32A32_FLOAT@0, COLOR0..3 R8G8B8A8_UNORM@16/20/24/28, COLOR4 R32G32B32A32_FLOAT@32, TEXCOORD0..2 R32G32B32A32_FLOAT@48/64/80, TEXCOORD3 R32G32B32_FLOAT@96, TEXCOORD4 R16G16_UNORM@108, TEXCOORD5..7 R32G32B32A32_FLOAT@112/128/144 | `tessellate{,Soft}MultiTexture` 41-56, 116-131 |
| **J** `143D22598` | 20 | POSITION0 R32G32B32_FLOAT@0, TEXCOORD0 R32G32_FLOAT@12 | `distortion`, `distortionSoft` 57-62, 140-145 (E04) |
| **K** `143D22A38` | 32 | POSITION0 R32G32B32A32_FLOAT@0, TEXCOORD0 R32G32B32A32_FLOAT@16 | `radialBlur` 63, `SoftRadialBlur` 139 (E05) |
| **L** `143D22870` | 80 | POSITION0 R32G32B32A32_FLOAT@0, TEXCOORD0..3 R32G32B32A32_FLOAT@16/32/48/64 | `LightShaft` 71-74 (E08) |

(Glow twins use the same layout as their base programme: 75-127 repeat 0-56 with the same tables.)

**What this closes:**

- the observed patch-list strides **112 (E), 36 (D, tessellated point sprite), 160 (I)** match the layouts of the tessellated families exactly (the capture note of EFFECTS-PHASE2 §6 says "two layouts" of stride 112; the executable has **one** layout of stride 112, table E. The two PSOs of that note are two programmes sharing it: not resolved further, no consequence for the native key);
- the strides the non-tessellated builders must write (A 76, B 88, C 32, J 20, K 32, L 80) are now known; they were not observed in any capture;
- EFFECTS-PHASE2 §6 "the layout is not recoverable from the capture" and EFFECTS-IMPLEMENTATION-CHECKLIST §11 "input-layout formats (8 programmes)" are answered; E05-E04 §3 (`POSITION`, `TEXCOORD0`, formats "from the VS signature") is answered by J and K.

## 3. Native recipe (D3D12)

- One `D3D12_INPUT_ELEMENT_DESC[]` per table, built from the rows above: semantic name, index, DXGI format, slot 0, `AlignedByteOffset`, `PER_VERTEX_DATA`, step 0. The PSO key's layout field is the table (11 values).
- The vertex-buffer view stride is the slot-0 stride of the table (`max(offset + size)` of the elements: A 76, B 88, C 32, D 36, E 112, F 80, G 48, H 60, I 160, J 20, K 32, L 80). The builders reserve more bytes per control point in their largest case (`0xA0` = 160, EFFECTS-PHASE2 §7): the stride is the table's, not the reservation.
- The programme table must resolve a VS **index** to its layout table (the shader-set index is the key; `14473EBC0[i]`, `143D22320[i]`), not a VS **name**: several programmes of a name share a table and the table never depends on the pixel stage.
- Verify at the first witness only the cheap invariant: the vertex-buffer byte count of a patch-list draw is a multiple of the table stride (no capture of layout records is needed).

## 4. Loop 2 (127 entries): recorded, not decoded further

The second loop builds six distinct layouts for another programme set of the same builder (second name array, entries 0..126). Their strides are 160 (`143D22DE0`, 19 elements, 16-bit half TEXCOORD0..17), 48 (`143D23290`, entry 24), 192 (`143D23320`, entries 54-61), 32 (`143D220E8`), 16 (`143D23588`) and 336 (`143D23000`, entries 90-126, 23 elements of float4). They are not E01–E05 layouts: INFERRED to belong to the model/GPU-particle side (E03, E07), where the readers of those notes will find them in the TSV rows `loop 2`. Names are not resolved here (the second array is not in `ffx-shader-set.tsv`).

## 5. Consequence for the checklists

EFFECTS-IMPLEMENTATION-CHECKLIST §11 row 3.3 and its verdict (the only item to read before coding step 3.3), and LIVE-CHECKS-CONSOLIDATED §6 LAYOUT, are marked closed by this part. Nothing is left to read before coding step 3.3.
