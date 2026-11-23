# YEBIS host and recorder HAL: implementation sheet (W10 YB-6, YB-6b, YB-7; thread « Suite W10 YEBIS », 2026-10-13)

Working sheet of the native YEBIS host: what the original does (read in the exports, labels as in
[YEBIS-WAVE1-READY](YEBIS-WAVE1-READY.md)), what was observed live, and the implementation route. It complements
[YEBIS-HAL-READY](../I-0068-static-yebis-ffx-lights-spec/YEBIS-HAL-READY.md) (the 77 slots) and
[YEBIS-D3D12-DESIGN-NATIVE](../I-0068-static-yebis-ffx-lights-spec/YEBIS-D3D12-DESIGN-NATIVE.md) (the recorder model). New
exports: `evidence/I-0068/queries/yebis/hal-impl/a` (Invoke-Map: `1774B60`, `1772810`, `1775B50`, the creators `176DB70`,
`176CD00`, `176C260`, `176DF60`, `176E3C0`, `176E2D0`, the vtables of `IGPUTexture`, `IGPUSurface`, `CTinyEffect` and the device).

## 1. Observed (Reference1615, m37)

- The HAL device is created once by the factory `170ACC0` (vtable `142B598C8`); `E2B20` runs three times on one post object
  (re-opened when `post+0x2278` is raised by `E4070`); `post+8` is the engine's GX context wrapper (vtable `142990838`);
  **`post+0x21D0` holds the `IPfxContext`** (vtable `142B4C4A0`, five slots, refcount at `+8`; created by `thunk 1416C8710`).
- `CPostEffect` is the global singleton `[1448E5E38]` (0xC90 bytes, constructed in `1416C87D0`, then `Initialize` `1416EBF80`).
- Every library draw runs inside `E5110` and goes through the effect commit; m37 world frames: 46 `DrawIndexed(3)` with a stable
  technique list (YEBIS-WAVE1-READY implementation log, YB-6b); the title view: 4 draws.

## 2. Read in the exports (verified)

- **Open (`E2B20`)**: `IPfxContext` created once into `post+0x21D0`; flags from the Setting-Info globals `144961F78..FB4`; the GX
  context `**([144961AB0][2]+8)` is passed in the descriptor to `1416C87D0(ctx, &desc, [144961FB8], [144961FBC], flags | 0x90000000)`
  (desc word 0 = that context); then the GX resources of `post+0x28..+0x70` through the GX creators `1419254F0`, `141925830`,
  `141925B10`, `1419256D0`, `141925BF0` (constant buffers, the 16x16x16 LUT and its strip, the DoF table), only when
  `post+0x28` is null.
- **Device factory `170ACC0`**: allocator `1416CE840(PTR_14469AB80, 0xA50)`, constructor `17080E0` (vtable, viewport `+0x9A4..`,
  `+0x9B8 = 1.0`, the 0x400/0x80/0x400 tables at `+0x40`, `+0x440`, `+0x4C0`, render-target slots `+0x8D0..+0x948`), init
  `17074C0(device, desc, d3d_context)`: `GetDevice` into `+0x20`, the context into `+0x10` and `+0x18`, the states of `17071E0`, then
  slots 38, 39(-1), 41(0) and 49(zero factor).
- **Device slots (native set)**: 6 `SetRenderTarget(i, surface)` keeps the surface at `+0x8D0+8i`, binds `OMSetRenderTargets`
  from the surface (`+0x120` count, `+0x128` RTVs, `+0x168` DSV) and calls slot 15 with `{0, 0, surface+0xB0, +0xB4, 0, 1}`;
  7 keeps the depth surface at `+0x910`; 15/16/17/18 call `RSSetViewports`/`RSGetViewports`/`RSSetScissorRects` directly (no
  layout field); 40 `ApplyRenderState(index, cached)`: index -1 = `+0x9E8`, cache `+0x9E4`, blend state `[+0x40+0x10 i]` with factor
  `+0x9EC`, rasterizer `[+0x48+0x10 i]` (bits 0-2 blend, 3-4 mask, 5 scissor); 50 `ApplySamplerState(stage, mode, cached)`: cache
  `+0x9FC+4 stage`, samplers `[+0x440+8 mode]` (stages 16-19 vertex); 52 `Clear(flags, colour, depth, stencil)` on RT0's RTVs and
  DSV (bit 28 colour, bits 0-1 depth/stencil); 53 unbinds vertex streams; 57 `Draw`; 58 `DrawIndexed` with the index buffer
  `+0x998` (`[ib+0xB0][ib+0xB8]`, format `ib+0xE0`, offset `+0x9A0`).
- **Draw path `171BE50`** (CTextureUtil): device slot 40; the effect object at `+0x38` (slots 29 begin, 31 begin pass, 38 bind
  the vertex buffer `+0x21778` with the declaration `+0x21658`, 32 end pass, 30 end; its vtable is not `CTinyEffect`'s 26-slot
  table: open); the vertices through `176EA50`/`176EC30` (lock/unlock of the library's dynamic buffer, ring at `+0x21768`); device
  slot 8 `SetIndices(+0x21780)`; slot 58 `DrawIndexed(5 = strip, n)`.
- **Effect commit `1775280`** (thunk `1776880`, CTinyEffect slot 23): the pass `[effect+0xA0][effect+0x128 - 1]`; its VS and PS
  records `[[pass]+0xB8][pass+0x20 / +0x24]` (record `+0x10` name, `+0x30` shader handle, -1 until `1774B60` creates it from the
  blob `+0x40`/`+0x3C` through `ID3D11Device` `effect+0x80`, then `1772810` builds the variable bindings, `+0x38` stage 1 vertex 2
  pixel, `+0x50..+0x58` the bindings, `+0x68` reflection with the `$Globals` size at `+0x54`); the context is `effect+0x78`; it
  unbinds `effect+0x90`/`+0x94` SRVs, nulls GS/HS/DS, maps the constant buffer `effect+0x88` (discard), sets VS/PS, writes each
  binding (type 1 scalars at the binding's stride, 2 vectors, 3 matrices transposed, 4 textures: the texture objects' SRV `+0xD8`
  and their slot 19 for the sampler mode, then `PSSetShaderResources`/`VSSetShaderResources`), unmaps, binds the CB to b0 of both
  stages. End (`1775B50`, slot 24) unbinds the texture slots.

## 3. Route

Route (b) of YEBIS-WAVE1-READY 4.5: the host creates the `IPfxContext` and calls `1416C87D0` itself; the device factory is the
native one (hook of `170ACC0`: the same allocation and constructor, a native init, a vtable copy with the native slots); the GX
resources of `E2B20`'s tail are native resources of the stage (YB-8). Record-only first (YB-7): every native slot records, the
D3D11 interfaces the library still holds are **sentinels** whose every method logs its index and caller once and fails, so
the live run lists exactly the call sites that still need a native replacement (diagnostic only, never a translation).

## 4. Next

1. Native probe (`native_yebis_host_probe`): whether the post object, `post+8` and the GX context exist in native mode.
2. YB-6: the sentinels, the native device object and slots, the recorder, the companions (`IGPUTexture` slots 2, 7, 8, 9;
   `IGPUSurface` 2, 6, 7, 8, 12, 13; the dynamic buffer lock), the commit and end transcriptions; T7.
3. YB-7: the host, record-only, compared with Reference1615's 46-draw list at m37.

## 5. Native probe (Native1616, m37, `yebis.flag`)

`native_yebis_host_probe` at the first gated frame and the 600th: `[GXRS+0x4DD8]` is **0** natively (no post holder, so no post
object, no `post+8`, no `IPfxContext`), and `[144961AB0]` is **0** (no GX context table for `E2B20` to use). The native host
therefore owns every object of the open itself: its own post-like parameter block (the converters already run on a native scratch
object), the `IPfxContext` (`1416C8710`), the call of `1416C87D0` with a native descriptor (the sentinel context in word 0, the
flags `E2B20` builds from `144961F78..FB4`, `[144961FB8]`, `[144961FBC]`), and the per-frame driver (`E5110`/`E69A0`'s library calls)
re-expressed natively; `E2B20` and `E5110` themselves are not callable natively (they read the engine's GX context). The 6,460
frames ran clean (0 faults, files restored by hash).

## 6. YB-6a: the recorder and the native device slots (implemented)

`native/world/yebis_hal.{h,cpp}`: `Recorder` (the current targets with their `+0xA8` descriptions, depth surface, viewport,
scissor, render-state index, sampler modes per stage, input layout; records `Clear`, `Draw`, `DrawIndexed` with the technique id
of the last `Commit`), the native device slots 6, 7, 15, 16, 17, 18, 40, 50, 52 (depth in XMM3), 53, 57, 58 (E_UNEXPECTED without the
index buffer at `+0x998`, as the original), 59, each in the original's calling convention and finding its recorder from the
device; `DeviceVtable` (the original table with the native slots written over it, or a trap where no original is given); the
sentinels (`Sentinel(Device|Context)`: 200-method objects that fail, AddRef/Release returning 1, each method reported once with
its caller). Slots 6/7 keep the surfaces' counts through their slots 0 and 1, and slot 6 derives the viewport through slot 15, as
`17082E0` does. T7 (`yebis_hal_test`, run by the build): a fake library drives the slots through the vtable with Reference1615's
m37 frame (46 `DrawIndexed(3)` into the observed DXGI formats and sizes, the technique committed before each): all 46 recorded
in order with target, derived viewport, technique, render state and sampler mode; no slot outside the native set reached;
scissor by rectangle, by box and from the viewport; Clear with its depth; invalid indices refused; surface counts balanced;
the sentinel fails and reports once (10 checks).

Companion reading (`hal-impl/c`): the texture's slot 19 (`176D560`, the commit's sampler call) only calls device slot 50 with its
own slot-14 mode, so it stays the original's; the library's dynamic buffers (`176ED80` create, `176E650` storage, `176EA50`
lock, `176EC30` unlock) call the D3D11 context `[[buffer+0x18]+0x10]` directly, so they are free functions to replace by hooks
in the native host, as are the effect's commit and end.

## 7. The native host: open and frame (read, `hal-impl/d`)

- **Open** (`E2B20`, asm `1400E2B44..E2E0B`): the library context comes from `1416CC930` (thunk of `1416C8710`: an 0x18-byte
  `IPfxContext`, vtable `142B4C4A0`); the parameter block (0x100 bytes) from `1400E23C0` (defaults), then `+0x58` = `[143D7EDE8]`,
  `+0x68` = `[143D7ED9C]`, `+0x4C` = 0x10, `+0x70` = 1, `+0x7C` = 7, `+0x8C` = -1, `+0xC0` = -1, `+0x90..+0xBF` = three copies of
  `[143D802E0]`, `+0xC8` = `[144962270]`; the flags in `ESI` (`0x90000000`, `0x10000` or `0x20000` from `144961F78`, and the bits of
  `144961F7C..FB4`); then `1416C87D0(context, &{D3D11 context, 0}, [144961FB8], [144961FBC], flags, &block)` (integer arguments in
  `R8D`/`R9D`); `1416C87D0` builds the singleton `CPostEffect` `[1448E5E38]` and calls `Initialize` `1416EBF80`, which creates
  the device through `170ACC0` from the D3D11 context.
- **Frame**: `E5110` calls the driver `E69A0` (about 80 library setters from the globals `144961F70..` plus the GX textures of
  the post object for the LUT and the scene: `FUN_1416CA5D0`, `FUN_1416CA660`) and then, when `post+0x20` is set,
  `1416C9330(context, 1)`, which runs `CPostEffect` `1416E8200`: the frame's library work (the draws observed inside `E5110`).
  `14007F820` is GX state only.
- **Native host (YB-7)**: (1) open as above with the sentinel D3D11 context in the descriptor; (2) the factory `170ACC0` hooked:
  the original allocation (`1416CE840(PTR_14469AB80, 0xA50)`) and constructor `17080E0`, `+0x10`/`+0x18` the sentinel context,
  `+0x20` the sentinel device, `[1448E6B80]` = the device, the vtable copy of `142B598C8` with the native slots, the CALL part of
  `17074C0` (fields `+0x8C0`, `+0x8C4`, `+0x8C8`, slots 38, 39(-1), 41(0), 49); (3) the creators, buffers, effect commit and end as
  native replacements; (4) per frame: the driver's setters re-expressed without the GX objects (native tokens for the scene and LUT
  textures), then `1416C9330`, the recorder collecting the frame; (5) record-only: nothing is encoded; any access violation inside
  the library disables the host for the session (SEH) with a row.

## 8. Live iterations of the native host (record-only, `yebis.flag;yebis-host.flag`)

| Run | Change | Result | Next |
|---|---|---|---|
| Native1617 (m37) | YB-7a: the open (IPfxContext, parameter block, `1416C87D0`), the native device factory, sentinels | `CPostEffect` constructed; `Initialize` returns `E_INVALIDARG` after one sentinel call: context method 112 `GetType` (caller `1416EC2EC`): the library requires an immediate context. No fault, 6,525 frames, files restored | the context's `GetType` answers 0 (immediate); then `Initialize` reads the default surfaces (`slot 11 -> CPostEffect+0x20`, `slot 10 -> +0x28`) and calls the surface's slot 5: native companion surfaces needed (the default LDR target, the depth) |

Reading for the companions (`hal-impl/e`): an `IGPUSurface` (0x170 bytes, constructor `176C2B0` over the texture base `1771420`/`176BB90`,
`+0x18` the device) describes its resource in a 0x6C-byte block at `+0x38` that `176B540` fills from the D3D11 description
(Texture2D: dimension 3 at `+0x64`, then format, width, height, array size, mip count, usage, bind flags (`+0x50`, 0x20 render
target, 0x40 depth), CPU access, misc, sample count and quality, and the mip-level sizes at `+0x98..+0xA0`); `17711A0` derives the
library description at `+0xA8` (format, 6, width, height, sample mapping, quality); `17713C0` asks the device's format support
(device slot 74 and `ID3D11Device` method 29). A native surface is therefore the original object with that block filled from the
native resource description, which the native creators (device slots 60-67) build without any D3D11 object.
| Run | Change | Result | Next ||---|---|---|---|
| Native1618 | GetType answered; native default surface (+0x9C0) | the device is built; the effect asks the context's `GetDevice` (method 3, `17724D3`) and dereferences it: fault caught, host off | the context's `GetDevice` hands out the device sentinel |
| Native1619 | `GetDevice` | `Initialize` reaches the first texture: `CreateTexture2D` (device method 5) from the creator `176DB70` (slot 61), `E_FAIL` | native creators |
| Native1620 | native creators: slots 60, 61, 62, 64, 65, 66, 67 build the original companions natively (`CreateNativeYebisTexture`, `CreateNativeYebisSurface`: resource block, library description, mip surfaces) | 10 resources, then `UpdateSubresource` (context 48, surface slot 6 `176C868`) and `CreateBuffer` (device 3, `176E80F`): fault | surface uploads kept by the host; the dynamic buffers' storage, lock and unlock in host memory (hooks `176E650`, `176EA50`, `176EC30`) |
| Native1621 | uploads, buffers | 5 uploads, 5 buffers (192 x 768, 8, 32 x 768, 45,056 x 3, 3,072), then `CheckFormatSupport` (device 29 from slot 74) and a fault in surface slot 12 (`176CBDB`) | device slots 72 and 74 native; surface slots 12 (copy, recorded) and 13 native |
| **Native1622** | slots 72, 74; surface copies | **the library opens natively: `1416C87D0` returns 0**, 128 native resources (the DoF levels in R16G16B16A16_FLOAT 784x294, 392x147..., the glare pyramid in R11G11B10_FLOAT 392x331 down to 7x6, the adaptation target 640x540, the outputs at 1280x720), no sentinel call left in the open, no fault, 6,509 frames | the per-frame driver (`E69A0` re-expressed) and `1416C9330` in record-only mode; the effect commit |
| Native1623 | YB-7b: one library frame per gated frame (`1416C9330(context, 1)`), the effect commit (`1775280`) and end (`1775B50`) native | no fault, but `1416E8200` returns `E_FAIL` with no record: the scene is not begun (its first byte) | begin the scene as `E69A0`'s last call does |
| Native1624 | `1416C9310(context, 0x10000000, colour 0.3764706 x3 1, [143D7F144], 0, -1)` before the frame | the frame starts: state resets reach the context sentinel (methods 43, 36, 35, 8, 25, 31, 37: void setters, harmless), the effect's constant buffer creation (device 3, unused natively); a fault in `1770670` (the effect wrapper's slot 38): the vertex declaration was null, its D3D11 input layout having failed (device 11) | device slot 70 native (the declaration's CPU part, the layout being its element list); CTinyEffect slot 25 (streams) native |
| **Native1625** | declarations, streams | **the library renders frames natively in record-only mode: 34 `DrawIndexed` + 1 `Clear` per frame, result 0, stable from frame 1 to 3,000+, no fault**: the glare pyramid in R11G11B10 from 392x331 down to 7x6 with its ups, a 640x540 target, the tone map into R8G8B8A8 1280x720. Without the driver's setters (`E69A0`) the parameters are the open's defaults: no DoF, motion blur or light shafts yet, and the pyramid starts at 392x331 where the original's m37 frame starts at 210x177. The technique names were empty in the rows (the commit ids were not reset per frame: fixed) | the driver `E69A0` re-expressed, then the comparison with Reference1615's 46 draws |
| Native1626, 1627 | YB-7c: the driver is the original `E69A0`, called as `E5110` calls it: the native post object (the converters' object, persistent with the host), an engine-context block whose `+0x210` is the camera record built from the native scene camera (pose, field of view `+0x50`, near `+0x54`, far `+0x58`), the globals `144961F70` twice, null GX objects (`[1449622F0]`, `[1449622F8]`: the depth and LUT branches skip), the previous frame's camera record; `[144961AB0]` points to a zeroed table for the duration of the call (its only GX read is the multi-view state `+0xB80`) | a fault in `memcpy` called from `171C0A2` (the CTextureUtil vertex write after a null lock) on frame 1 of each run (cause OPEN; the review found that `E69A0` takes an eighth argument, a float E5110 computes, which these runs did not pass) | the fault report gains the accessed address and the image return addresses of the stack |
| Native1628 | lock refusal rows | no fault, no refusal (the frame-1 fault did not recur); title view 2 draws, world 32 draws: the light shafts appear (mask + 5 radial blurs) and the tone map is `TonemapHDR_Dither_Exposure_Matrix_Gamma` as in the original; the glare pyramid still starts at 392x331 | the original re-opens the library when the Setting Info changes (`87FB0`: `post+0x2278` raised by `E4070` -> `E2B20`) |
| Native1629 | re-open on `post+0x2278`; device slot 5 native (descriptor copy, targets released) | the re-open faults in slot 11: the default surface died with `CPostEffect`'s un-initialisation | the host keeps its own reference on the default surface; slot 5 re-binds it |
| **Native1630** | the default surface kept | **one re-open at the zone, then 34 draws + 1 clear per frame, stable, no fault: the glare pyramid now has the original's sizes** (210x177, 105x89, 53x45, 27x23, 14x12 with ups 106x90, 54x46, 28x24), the light shafts at 210x177, `MadT2`, `MadT1_S1` to 420x354 and the tone map, as Reference1615's draws 16-45. Differences left: (1) the first 14 original draws (DoF 10, `Alpha_R`, motion blur 3) need the depth and velocity inputs the driver takes from GX objects (null natively); (2) the glare source chain starts with `MadT1` at 1280x720 and `MadT1_S4` steps where the original has `MadT1_S1` at 640x360 and 320x180; (3) the pyramid's down and up steps use `MadT1_S4` and `MadT1_S1` where the original uses `MadT1_S1` and `MadT1_S4` (a filter-capability choice, OPEN) | the depth and velocity inputs as native textures for the driver; the S1/S4 choice |

## 9. Reference frames and the comparison (D-0175)

Under D-0175 the hosted library is the reference, and the true oracle is the original itself: the Reference observer
(`yebis-hal-observe.flag`) now captures, per draw of its detail frames, the pass, the programmes (blob SHA-256), the target, the
textures by slot and the **$Globals block** (`BuildYebisCommit`, read only, the original commit running after it).
Reference1632 (m37): 46 draws with their constants. `Compare-YebisReference.ps1` pairs draws by pass and target size and lists
the constant words that differ. Native1631/1633 against Reference1632: the 14 first original draws (DoF, `Alpha_R`, motion blur)
and the S1/S4 down/up variants have no native pair (inputs and the variant choice above); the paired draws differ in the
glare weights and texel offsets that follow from them, the bright-pass exposure words (1.52 in the original, 64.5 natively: the
adapted exposure the library holds natively is not the original's), the light-shaft position word 42 (NaN natively: the sun
position needs the camera the library expects) and the tone map's glare weights. Native1633 validates the review's fixes: the
driver's read of `144961AB0` redirected at its site (row `native_yebis_driver_read_redirected`), the eighth argument passed, one
re-open, 33 draws + 1 clear per frame, no fault.

The next work (D-0175) is our own D3D12 passes, starting from the tone map, their constants computed natively from the globals
and the camera and checked word for word against the Reference captures per zone; the host stays opt-in as a development aid.
