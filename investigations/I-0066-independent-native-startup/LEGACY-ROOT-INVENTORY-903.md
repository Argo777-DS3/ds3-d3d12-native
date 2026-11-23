# Old renderer root consumers on the native path (903)

Question: after native Continue, zone loading faults successively in engine code
that still reads the old D3D11 renderer's root objects (Native898 at `23B79C9`
through display `4961AB0`; Native899 at `1AC921` through `4961B50`+0x4DD0).
Which engine consumers still depend on those roots, grouped by family, so that
their native owners are designed together instead of one fault at a time
(D-0156, WORKFLOW coverage by families)?

Method: read-only full-instruction operand scans of the DS3 1.15.2 database
(`evidence/I-0066/artifacts/direct-zone-legacy-inventory-903`: `inventory.txt`,
`globals.txt`, `boundary.txt`, `cpu-consumers.csv`, scripts and consoles). The
"GX range" 140004000–1402C0000 is the old renderer's code region observed in
this scan; "CPU consumers" are referencing functions outside it. Static
references do not prove runtime reach; the native runs establish reach.

## Roots

| Global | GX-range functions | CPU consumers | Native state |
|---|---|---|---|
| `4961AB0` display object | 412 | 87 | absent (null) in native execution |
| `4961B50` scene/draw root | 447 | 37 | absent; read at `1AC921` |
| `4961B80` | 166 | 2 (`3A4AC0`, FaceGen `836C40`) | not established |
| `4961B38` | 45 | 1 (`2357210`) | not established |
| `4961ABC/ABD`, `4961BC5` | few | startup `D573B0`, `D6E840/D6E9B0` | handled by existing startup owners |

Of the 130 CPU-consumer edges, 17 are functions the native module already owns
(startup, frame, UI, pointer). The 113 others form these clusters:

| Cluster | Functions (root) | Probable family (to verify) |
|---|---|---|
| `3A28B0–3A5420` | 7 (AB0, B80) | unknown CPU service |
| `651A60–653290` | 5 (B50) | unknown |
| `836C40/836E40` | 3 (AB0, B80) | FaceGen image (known native owner exists) |
| `8E9EE0`, `931F30`, `932540` | 3 | unknown |
| `A54680` | 1 (AB0) | UI widget |
| `CC8180–CED9A0` | 4 (B50) | scene/world |
| `D09F00–D3FFB0` | 20 (B50, AB0) | character/model loading and update (Native899 stack D0166D) |
| `D4A2D0–D6E9B0` | 15 (AB0) | startup/frame services, partly owned |
| `DFD270–DFF7C0` | 4 (AB0) | render step neighbours |
| `E5A220–E6DDD0` | 5 (AB0) | UI (session owned) |
| `EAF7E0–EF9090` | 10 (AB0) | main loop/window/configuration |
| `2357040–23D0510` | 33 (AB0, B38) | physics/cloth/input middleware (mouse `23891E0`, dynamic mesh `23B7960` handled) |

In addition, CPU code calls GX-range functions that read these roots without
being counted above, e.g. `1AC840` (Native899 fault) from character update
`D01610`. The boundary scan `boundary.txt` lists 83 direct CPU→GX edges into
functions reading `4961AB0`; edges into functions reading `4961B50` are the
next scan.

## Reach and null guards (905–906)

Static forward closure over direct calls (depth ≤ 6, `reach.txt`) from the
functions on the Native898/899 loading stacks reaches 1803 functions, 29 of
which read these roots: `23B7960` (handled), model pose `15490` and streams
`D3F7C0` (native hooks exist), FaceGen `836C40`, material `D27040`, and 23
GX-range helpers (`2B700`, `2BD50`, `2C100`, `314D0`, `37B50`, `3C3B0`, `40A50`,
`45920`, `4A060`, `57470`, `61C60`, `A2C90`, `DE5A0`, `DE6F0`, `DEC00`, `12EFD0`,
`1346A0`, `156100`, `1889A0`, `18C210`, `18F350`, `1922B0`, `1951A0`).
Virtual calls are not followed, so this is a lower bound (`1AC840`, reached
through a thunk/virtual path, is absent).

A register-local guard scan of all 2396 root loads (`guards-*.txt`) finds 73
followed by a null test, 1615 dereferenced before any test, 708 undetermined.
The engine assumes these roots always exist. A null root is therefore not an
engine-supported state: relying on engine "absent" branches only works where a
member (like `+0x4DD0`) is tested, not for the roots themselves.

## Consequence for the design

Zone loading needs native contracts for at least the character/model loading
and update family (`D0xxxx–D3xxxx` with `4961B50`), the physics/cloth family
(`235xxxx–23Dxxxx`) and the scene/world cluster. Each must be designed as one
owner (creation, CPU update, selected use, retirement) reusing the existing
native producers, with explicit omissions where a family is not yet supported.
No old root object is manufactured. This inventory is the input for that
design; it does not qualify any runtime path.

Because the roots are assumed by hundreds of unguarded sites, a fault-by-fault
treatment cannot converge. The coherent D-0157 option is a native engine root
layer: native-owned objects that provide the CPU-visible interface the engine
actually consumes from `4961B50` (scene/draw manager), `4961AB0` (display) and
`4961B80` (GX heap), with genuine engine CPU data where the engine reads data
and native D3D12 services where it requests GPU work, built from the member
offsets the reached consumers use. This is the ER organisation (engine-owned
scene/render roots directly backed by the D3D12 backend), not a D3D11 stand-in:
no old GPU object, device or readiness flag is created. Next: extract the
member offsets read by the reached sites and classify each as CPU data,
GPU request or device presence.

## Families handled so far and next boundary (908–910)

| Family | Engine site | Native handling | Evidence |
|---|---|---|---|
| Mouse position | `23891E0` (`4961AB0`+0x198/19C) | position from native presentation extent | Native898 |
| Dynamic mesh buffers | `23B7960` | engine's empty set, explicit omission | Native899 |
| Character probe lighting | `1AC840` (`4961B50`+0x41E0/+0x4DD0) | engine's own device-absent branch (blend clock only), explicit omission | Native900 |

Native900 (module908 50E23F57…, offline, restored ok) passes `1AC840`
(`native_object_probe_lighting_unavailable`) and faults at `C9E6A` with RCX0:
character render setup `8B76A0` takes the scene drawable owner from the render
service `4796298` (`DFDA20` = service+0x10 → +0x58), absent in native, and
registers a 0x80-byte drawable (`147300`) into it through `2D860 -> C9E60`
(heap +0x80, list +0x118). `DFDA10` (+0x50) is present. This is the model
drawable registration family, the entry of the selected-scene path in
[DIRECT-CONNECTION](DIRECT-CONNECTION.md); it must be designed with the native
selected scene owners, not omitted.

### Drawable owner (911–917)

Diagnostic module915 (B97160AB…) hooks `2D860` only to record the layout when
the owner is absent and leave the drawable unregistered (`diagnostic_only`, not a
contract). Native901 (offline, restored ok): the render service scene
(service+0x10, table RVA 297A3C0) has `+0x50` = a view (class table `3D4A108`,
not one of the native-tracked views/contexts) and `+0x58` = null; the run then
faults at `1777ED2` (heap allocation with null heap, from `8BED20`), a direct
consequence of the missing drawable.

Source: the render service constructor `DFEEB0` builds `+0x50` from `C8780`
(0x90) and creates `+0x58` only when the graphics service `4793348`+0x78 (a model
render manager) exists: `B2410(manager)` allocates 0x340 bytes from the
manager's heap (+8) and constructs the drawable scene `C9510(obj, heap, manager)`
then `C9B60`, attached to `+0x50`+0x10 by `C86F0`. Character setup `8B76A0`
registers each model drawable there (`2D860 -> C9E60 -> 147300`). In native
execution `4793348`+0x78 is absent, so the drawable scene is never created.

Design consequence: the native model scene owner must provide this drawable
scene (registration, update, withdrawal) and feed the native selected scene
directly, the ER join from current model selection to native draws. Whether
`C9510` can be constructed as genuine CPU state over a native model manager, or
the registry must be re-owned natively, is the next source question.

### Model render manager (918–927)

`D573B0` (old resource bootstrap, replaced natively by
`ConstructNativeResourceConsumers`) creates graphics-service child `+0x78`:
0x70 bytes, `B1DC0(obj, heap)` (CPU: vtable `143D49850`, heap, pool header)
then `B1F60 -> B1E50` (GPU: 400 old-device queries `19C790`, two old buffers
+0x60/+0x68). Static reach from `B1DC0/B2410/C9510/C9B60/C9E60/147300` touches the
display root only through `B1E50`. Module927 (56D7C7D1…) constructs the
genuine CPU manager in the native resource phase and omits its GPU initializer
(occlusion queries and the two buffers are explicit omissions).

Native902 (offline, restored ok): `native_model_render_manager_cpu`; no drawable
owner refusal: the drawable scene exists, characters register, and native
selection now sees two objects (new unsupported class `3D4A1E8`). The next
stop is the engine's own fatal assertion `1777DE0` in character setup
`8BE6C0` (at `8BED1B`): the SFX manager singleton `4799C98`, deliberately not
published by native SFX startup, is required to register character effects
(`E6E6F0`).

## Members actually read by the consumers (907)

Member-offset scan (`members.txt`) over the 29 reached functions, the 130
CPU-consumer edges and `1AC840`: 46 distinct root members. `4961B50`:
`+0x41D0` (6 consumers), `+0x41E0/+0x41E8/+0x41F0` (probe/lookup sources, incl.
`1AC840`), `+0x42A8/+0x42B0` (character/model `D0C2F0–D1EB80`), `+0x4600`,
`+0x46E8`, `+0x4788` (15 GX helpers and `D272C0`, `D573B0`), `+0x47F0–+0x4858`
and `+0x4E18` (written by startup `D573B0` and settings `D1E8A0/D1EA10`),
`+0x4DD0` (device presence), `+0x5368`, `+0x5C8`, `+0x0`. `4961AB0`: `+0x0`
(19), `+0x10` (17, physics/cloth region), `+0x70/+0x80` (15 each), `+0x88`
(viewport, UI), `+0x198/+0x19C` (extent), `+0x20`, `+0x60`, `+0x2A0`, `+0x2D0`,
`+0x3B8–+0x3E1`, `+0xB58–+0xB88`. `4961B80`: `+0x0` (heap). Each member must
be classified (CPU data, GPU request, device presence) before the native root
layer is implemented.

## SFX manager and encrypted engine code (930–935)

The SFX manager singleton `4799C98` (0xF0, vtable `142992EF8`) is created by
`D5AA40 -> E6C9D0` and read by 30 engine functions. Its constructor is mixed:
CPU members (two lists `+0x08/+0x20` with heap `479C288`, callbacks
`+0x38..+0x50` from `D53340/D53270/D53100/D538A0`, `+0x60` from `C2400`/`D648A0`,
`+0xD8` from `D52CA0`, `+0xE0` from `E67CB0`) and one GPU member, the GX effect
system `+0x58` (`24B260`, 0x6C0, wired by `24C300/24C390/24C3A0/24C530`, with
the GX callbacks `4962380/388/390` that only GX code reads). `+0x68` and
`4773698` come from `1AB0150/1AB05E0`, a sorted CPU id→value table (the
classifier's "device" verdict was a false positive: `141924250` is a search),
and all 8 readers of `4773698` handle null explicitly.

Most decodable manager methods test `+0x58` before using the effect system
(`E6DA30`, `E6DBC0`, `E6DCD0`, `E6E720`, `E6ECB0`, `E6E570`, update `E6E750`):
a manager without effect system is an engine-supported state. Exceptions to
resolve: `E6EA10/E6EC60` pass `+0x58` to `24F2C0`, `874F10` reads `+0x58`
directly, `E6E4D0/E6E600` forward to GX thunks.

Several methods cannot be read statically because their bytes are encrypted
at rest (high-entropy instructions in the database): `E6E6F0` (called by
character setup `8BE6C0` and `88FE50`), `E6E6E0`, `E6E620`, the constructor
tail `E6F630` and the `+0xE0` constructor `E67CB0`. Cycle 934–935 adds a
Reference-only acquisition (`code_capture`, controller mode `referencecode`):
the executable sections in memory are compared with the relocated disk image
and each differing run is recorded, then laid over the database in memory by
`Code-Overlay.ps1` and `I0066Overlay936.java`
(`evidence/I-0066/artifacts/direct-code-capture-protocol-934`).

Reference935 (offline, in game, restored ok): 7769 records, 3.53 MB of code that
differs from the disk image, in 7169 runs (`overlay935`). With the overlay the
encrypted methods decode (`sfx936`–`sfx938`): `E6E6F0` stores two tokens at
`+0xC0..+0xCC` (CPU setter), `E6E6E0` sets `+0x72`, `E6E620` resets the tokens,
`E67CB0` is a CPU constructor, `E6F630` registers debug-menu items (global
`4964480`) and writes `+0x58`+0x5C5 when that menu exists. `874F10`, `E6E4D0`
also test `+0x58`. Only view attach `E6EA10` and detach `E6EC60` (render
service `DFEEB0`/`DFF7C0`) pass `+0x58` to GX `24F2C0` unconditionally; the
destructor `E6D220` tolerates every optional member. The GX flags written by
the constructor (`4962376..390`, `473DAD7`) are read only by GX code (`sfxflags939`).

Design (module940): `NativeSfxStartup` publishes a manager of the genuine class
built from `E6C9D0`'s CPU stores only (lists through `E6ED20` with heap
`479C288`, flags, identity matrix `39F9750`, neutral tokens `46402E0`, quality
from `810E90`); every GX member stays null. `E6EA10/E6EC60` are native (record
the view, no GX). `8C68F0`'s native retirement resets the manager by the genuine
`E6E620`; the native `7CB820` deferral requires that manager. Native940 passes
character setup and stops at the next family: character light construction
`CE8090` registers its shape through the GX light registry
(`244EC0/244ED0` → `259B60`, global `4962360`), absent natively (`fault941`).

## FX world (942–956)

The effect system `24B260` also creates three CPU structures that character and
model code use directly (`fx945` surface: ~30 GX API entry points, CPU callers
`CE8090/CE8670/CE8730` character lights, `66F7A0..670B30`, `D05C80`, `D3C4D0`,
physics `2423A1490..`): the FX world `4962360` (`248CC0`), its light registry
`4962398` (`2594E0`/`2596D0`) and node manager `49623A0` (`25B280`/`25B370`).
Their constructors classify CPU (`fx947/fx949`) except `248E30`'s per-buffer
random textures/views (old device `1419254F0/141925BF0`, count from the display)
and the `25D220/25E960` sub-objects. Module956 builds them genuinely on heap
`479C270`, without those GPU parts (buffer count 0). Native956: registry enabled,
character lights pass; next stop `1D6F1` from ambient-probe selection `D3FFB0`.

## Native scene root layer (957–969, D-0160)

`D3FFB0/D3E240` call `1D650(G+0x46E8)` and `61950` reads `G+0x4DD0`, with
`G = 4961B50` absent. The sub-object map (`root958`) shows the CPU surface of G:
receivers `+0x41C0` (lighting params, `49D00/4A060`), `+0x42B0` (env regions,
`65D60/664D0/66640`), `+0x46E8` (ambient probes, `1D650`), `+0x0` (`D62340`), and
direct member loads in `CC8180`, `D0C2F0/D0C990/D0D060`, `D1EA10`. G's own
constructor `213A0` (0x53B0) is CPU: it builds `+0x50` (`843C0`, display-guarded)
and, through `486A0`, the sub-managers at `+0x41C0`, `+0x42B0` (`65370`, the same
constructor the native env registry already used), `+0x46A0` (`151F10`) and
`+0x46E8` (`1D160`). Device `+0x4DD0/+0x4DD8`, `48B80` and the display parts of
`B5E00` are GPU. Module969 constructs G natively (no device), moves the native
env registry onto `G+0x42B0` and the gparam binding onto `G+0x41C0`; the engine's
own device-absent branches now apply wherever device presence is tested.

Native969 (offline, restored ok) published the root and then faulted at startup:
with a root present `D57900` runs the root frame update `23410` → `499C0`, which
walks the pass objects `48B80` would have built. The root's guarded sites
(`guards-sites.txt`, 27 functions) are the code that activates with a root:
probe registration `651A60..653290`, `931F30/932540` (`+41D0`, `+46E8`),
`D1EB80` (`+42A8`), `87FB0` (`+4610`), `89960` (`+41C0`), and readers of
`+5360/+5368` (`E4460`, `FB890`, `10A220`, `13BFC0`, `1B8F30`, `1CED20`,
`1D47C0`, `1D7850`, `24D000`, mostly testing `+5360`). Classification `root973`:
all 29 sub-object constructors of `48B80` are CPU, all their initializers GPU;
`B5E00`'s `127010` (`+5368`), `127AD0/127ED0`, `12BF60`, `12D390` are CPU.
Module974 builds the root with `+5368` and the 29 constructors (plus `93CE0` on
`+5C8`), no initializer; `23410` is native (genuine clock `126EA0`; the pass
objects' frame update `499C0` is an explicit omission).

Native974 (offline, restored ok) passed startup, menu and Continue and loaded the
zone (native textures and model geometry), then faulted at `1472A5`: the native
texture ingress itself called `147110 → 1471E0`, which creates an old constant
buffer through the display. Module978 keeps `147090/147110`'s CPU part (the
authored texture constant, `native_texture_constants`) without that buffer; an
audit of the 137 engine functions the module calls (`calls977`) finds no other
display/device path. Native978: the game reaches the world in native execution,
with the in-game HUD drawn by the native D3D12 UI path and no fault; the world
itself is not drawn yet (zone models loaded, `scene_instance_selected=false`).

## World registration and selection (980–995)

Reference982 and Native983 (offline, restored ok, Native983 closed cleanly with
3939 native frames and no fault) observe the world layer inserts (dynamic tree
`C89A0`, flat `C24E0`). Both modes insert the same classes from the same
callers: `SprjModelDispEntity` (`29489A0`, region attach `D02F50`, map pieces),
`SprjModelDrawEntity` (`2948EF8`, `885584`), `SprjAsmModelDrawEntity`
(`2947DB8`, character setup `8B783E`) and `CSSgEzDraw` (`295B690`, flat). In
game the native collector meets 311 objects but admits none: it reads only
`SprjModelDrawEntity`, while the map pieces are `SprjModelDispEntity`, whose
emit `D04B90` applies its fade/variant (`+0x120..+0x13C`, `+0x1C0..+0x1DC`,
parameters `+0x1F0`) to each child of its list `+0x90` through child `+0x58`.
Native selection must admit that class and its children (next).

Native995: the map-piece children are `SprjFD4ModelItem` (`2951FF8`, base
`GXFlverModelDrawItemBase`, emit `D3DA20`). Decompiled owner/item fields
(`disp992/994`, `fd4m997-1001`): the `SprjModelDispEntity` holds the fade tracks
(`+0x120`/`+0x1C0`, geometry weight `+0x130`, the DrawEntity layout shifted by
−0x40), the parameter group `+0x1F0` (same group-relative fields), eligibility
(`+0x388` bit0, visibility `D03050/64F000/619800` on `+0x39C`, category
`DFD8D0/D043C0/D04350` on `+0x38C`) and bounds `+0x40`; each item holds its model
`+0x58`, transforms `[+0x6F8]+0x60` (first matrix), pose `[+0x710]+0xA0`
(constructed by `D16F60` through the natively hooked `15490`), stream source
`+0x740` (its `+0x50` patch), light object `+0x70` and geometry suppression
`+0x71A`. Module1002 generalizes native selection to a candidate with these
explicit owner fields (`ReadCurrentModelInputsAt`, `ReadModelFadeStateAt`).

Native1002 (offline, restored ok): 215 map models admitted with native geometry
leases, no programme resolved. Native1003: every miss is a group that is not a
loaded effect (`GXFlver_ColDifSpcBumpDOL`, D24430 → 0). Material build `82220`
(`prog1009–1015`) then renames `…Ibl_BlC` → `…Ibl_Bloody_Cloth`, `Displacement`
→ `GXFlver_ColDifSpcBumpIbl`, `MixDisplacement` → `…IblMixDisp`, `Specular` →
`GXFlver_ColDifNewSpcBumpIbl`, and resolves the group through resolver `+0x60`
(`D245A0` → repository `4792928` → `D467E0`), which composes a
`GXFlverShaderGroup` by name (`<group>_<pass>`, passes `Fwd, Velo, Dpt, DptA,
FwdSdw, FwdSSdw, Gbuf, Gbuf2, Pnt, …`, variants `_[A]/_[AA]`, commons
`GXFlver_Common_*`) from programmes the old device (`root+0x4DD0`) knows. With no
device natively, module1016 applies the same renames and uses the admitted
effect that owns the `<group>_Gbuf*` programmes; selection uses the group name.

## Loose programme groups, LOD and engine5 rows (1016–1026)

Native1016: no admitted effect owns those programmes. The device callbacks of
`D467E0`/`AA630` (`shader1021–1024`, vtable `143D48C78` → device vtable
`14294D978` +0x20…+0x60) resolve by name in per-stage repositories: pixel
`D1E640` → `4792BF8` (`D47ED0`), vertex `D1E710` → `4792D58` (`D48F10`), then
`D1E570`/`D1E3D0`/`D1E4A0` (`4792B08`, `4792838`, `4792A18`, tessellation and
geometry). Each repository `+0x60` is the `2356180` hash container (bucket count
`+0x1C`, buckets `+0x20`; node name wstring `+0x10`, next `+0x50`), the wrapper is
`node+0x68`, created by the stage factories we own (`AdmitShader`). For pass Gbuf
`AA630` names pixel `"<group>_Gbuf"` (+`_[A]`, `_[AA]`) and vertex
`"<group>_Gbuf<skin><ins><channel>"` (`_Skin/_MorphSkin/_Morph`, `_Ins`, `@[cl]`);
`GXFlver_Common_*` vertex programmes serve only Aof/Voxelization. Module1025
walks both repositories read-only and owns the matching admitted programmes as
one composed native effect; Native1025 (3229 pixel, 7267 vertex records) composed
`GXFlver_ColDifSpcBumpDOL` with 25 programmes, `missing_program_family 0`.

LOD: the DispEntity fade variant (`+0x138`/`+0x13C`) is the LOD passed to each
item (`D3DA20` descriptor `+0`); `63CB0` takes descriptor `+0x18`, falls back to the
highest loaded LOD below it per model `+0xB4`, and keeps meshes whose `+0x14` mask
holds that bit. Module1026 applies this (`SelectOrdinaryModelLod`). Native1026
pass refusals: engine5 `Flags` (split rows) and engine6 `MaterialClass`. `57A20`
(`native-stream-cpu-711`) with dword `14473CBAC==2` and model `+0x220` bit2 builds
two engine5 rows with class masks 1 then 4 instead of one mask-5 row; module1027
evaluates those rows.

## First native world draw (1028–1045)

Native1045 draws 73 map meshes per frame through the native D3D12 renderer
(`native_model_draw_submitted`, debug errors 0, screenshot
`i66-native-map-perframe-1045/ingame-native-world.png`). The chain, each step
from decompiled engine code or the withholding-era host producers (I-0063
inventory `DS3_evidences/I-0063/inventory`, `native/world/producers`):
- programmes: plain `<group>_Gbuf` keys composed too (1031); extension-keyed names
  matched exactly, bare keys per stage (1032);
- palettes: dynamically indexed one-row map palettes bound padded to the declared
  cbuffer (1029);
- samplers: common s0..s12 table of `10AAD0/10AB80` (1033);
- textures: holders of the `D24490` registry stay empty for full-resolution map
  textures; the host rules bind `<name>_l` and DOL lightmap `..._atlas000` through
  the same lookup (1039). Natively the default registry `4792EC0` (`SYSTEX_DUMMY`,
  `D4A7D0` from `D5AC70`) and the GX system textures (`961AB0+270`) are absent
  (D514C0 observer, 1037);
- constants: `cbTexHdrScaleParam` (lightmap TPF float metadata, 1039),
  `cbMatDynParam +0x10` (1040), `cbMtdParam` by RDEF name for composed groups and
  shader-generator values (1041), instance lighting from `32A80` on
  `[root]+0x41E0` (native root, 1043; pass row scale 1 explicit policy, open),
  unaddressed instance elements zero (1044), `cbPerFrame` environment rows from
  `134A20` on `[root]+0x4680` with the native scene context keys `+0x4110/+0x4118`
  and PS `cbModelParam` from the group copy (1045).
Open: deferred lighting composition, grass `cbvModelParam`, MultiBlend
`cbMatDynParam`, system textures, characters.

### System textures (1047)

`6C00` (GX owner init, `961AB0+0x270`, 17 entries) creates the system textures
read by `D4A2D0` (`B580(owner,i)` → `SYSTEX_*` records): entries 0..13 are 1x1
2D textures, format 87 (`B8G8R8A8_UNORM`) except entry 5 (`18PERCENT_GRAY`,
format 91 sRGB); 14..16 are 1x1 cubes. Colours (0xAARRGGBB): 0 WHITE ffffffff,
1 BLACK ff000000, 2 GRAY ff808080, 3 SPEC 800a0a0a, 4 BLACKSPEC 01000000,
5 18PERCENT_GRAY ff757575, 6 NORMAL ff8080ff, 7 ZERO 00000000, 8 RED ffff0000,
9 YELLOW ffffff00, 10 MAGENTA ffff00ff, 11 GREEN ff00ff00, 12 BLUE ff0000ff,
13 CYAN ff00ffff; cubes 14 ff000000, 15 ffffffff, 16 ff808080. `SYSTEX_DUMMY/
ERROR/BLINK/FASTBLINK` (`D4A7D0` from `D5AC70`) are `D4A910` timed objects (period
1.0 and 0.1), not plain colours. A native system texture bank can own these
exact CPU contents; not yet implemented.

### First native deferred lighting (1049–1050)

Native1050: the native renderer lights the G-buffer with DS3's own deferred
programmes (AO, sun, albedo combine, composite with fog) fed by the engine's CPU
light/fog builders; details and open policy in
[NATIVE-LIGHTING-MAP.md](NATIVE-LIGHTING-MAP.md) "Native implementation".

### Characters (1056–1059)

Characters are `SprjAsmModelDrawEntity` (`2947DB8`): emit `D00200` (gates `+0xBB0`
bit0, `D03430`, `+0x70` & world mask; the `SprjModelDrawEntity` fade layout) then
`D02A30` over its parts: count `+0xAF8` (44 for the player), descriptors `+0xB00`
(stride 0x50: binding `+0x10`, flags `+0x38`, visible when bits 0–3 are set and
bit 4 clear), part objects `+0xB10` (stride 0x108, class `2947678`, emit `CFDE70`
= the `D079C0` descriptor with model `[binding]+8`, matrix container
`entity+0xA60` (`+0x18` when `+0xBB0` bit3), stream source `part+0x28`, parameter
animations `[part+0x40]`, storage `part+0x50`). Module1057 admits these parts.
Geometry (1058): `CopyModelGeometry` now accepts any 4-aligned stride 12..256
(was 32/44/48): 328 of 387 FLVER storages joined (135 before); still refused:
35 multi-stream meshes (check 3) and 15 non-R16/LOD-set index tables (check 9).
Palette (1059): the skinned VS declares `VC_aObjMatrix[2]` but indexes the whole
engine palette (`(BLENDINDICES + matricesData.x) · 3`); the full pose palette
(62 rows for the player) is bound zero-padded to 256 rows, and palette refusals
fell to 0. Next stop: equipment materials (`GXFlver_Equip.spx`) have no
resolved `g_DiffuseTexture` natively.
Native1061–1065: a neutral common decal (t41/t42) for the parts' per-character decal
provider (explicit open policy) drew the shield and sword (1061); two-buffer FLVER
meshes (second buffer at input slot 1, same storage and vertex count) joined 363
geometries (1063); `matricesData.x` (+0x50) is produced as the native palette base
(0) (1064); and every instance word `5B3D0` leaves unwritten (+0x80..+0x10F, written
only for draw types 0/3/4) keeps the instance array's creation zero (1065). Result:
272 native draws per frame (94 before), the map far more complete. Still open for
the character body: parts without a native pose (`model_poses` has no entry for
their `part+0x50` storage) and `GXFlver_EyeIbl` `cbMatDynParam`.

Native1066–1073 (character body, decompiled CFDE70/D01C30/D01610/D2D040/16000/22E930/
D009E0/C34420/C47CD0/D00040): the part draw descriptor's parameter-animation owner is
descriptor+20 = `D01C30(entity,i)` = `[entity+B18]+10*i` (`[part+40]` goes to
descriptor+28, the DD5CF0 slot; corrected 1066). Part poses are initialized by `D009E0`
(15490, ring = `[entity+BB0]` bit 6) and updated per frame by the entity's vtable+D8
`D01610` (gate context+8 != 0) -> `D2D040` -> `16000`, the public pose entry (+74 fast
in place; else, when `[144961B40]` and `14473D0DA`, a DF570 queue for the old upload
drain DF150 -> 16050; else 16050). `16000` is now owned natively like 16050/15FC0 (1067).
`22E930`'s fixed flagged blocks after its generator values are produced natively:
+0E -> +20 {f,f,f,1}; +0C|+19 -> +30 {a,b,-0.5a,0.5-0.5b}; +0D -> +40; +13 -> +60
`FC_ParallaxParam` {a,-(ab),c,-(cd)} from +130..+13C; +18 -> +70; a clear flag leaves
earlier words or the creation zero (1068-1069). Result: every character
`cbMatDynParam` ready, 282 draws, five character parts drawn. Still open: the in-game
body parts (7 and 13 meshes) keep a never-updated native pose, because `D01610` is
called (through `C34420`, vtable slot 1429247E8, -> `D00040` -> vtable+D8) only for the
menu character; in game the character entity received one call with weight 0 and none
after (1072-1073 pass-through witnesses). Next: the upstream caller and its gate.

Native1074–1084 and Reference1076–1083 (pass-through witnesses; Reference = original D3D11
renderer with the same observers): the per-frame character update chain is WorldChrMan
scheduling `8DD2E0` (an FD4 task; dispatcher return `23C1DA5`; called from `8DFB05`) ->
`88E1F0(chr, mode)` (mode 0 = full: registers tasks 0x10/0x15/0x2A/0x3B, sets
`[chr+1A0D]` bit0, then the model EzTask) -> `SprjEzVoidTask<SprjEzRabbitNoUpdateTask,ChrIns>`
(vtable `284F440`; `65CAC0` arms +18 and registers; `888C00` calls the function at task+28 on the owner at task+20, once
when armed) with action `891AB0` (bit0 and `[chr+18]+10 == 4`, then PlayerIns vtable+120
`8C1BC0` with dt `[chr+108]`) -> `[chr+20B0]` ChrAsmModel tick `C48180` (dirty flags, then
vtable+18 `C34420`) -> `D00040` -> asm draw entity vtable+D8 `D01610` -> `D2D040` -> `16000`.
Reference: `8DD2E0` and `88E1F0(player, 0)` run every frame. Native: the player's gating
fields are identical (control state 4, interval 0, flags clear), but `8DD2E0` itself runs
only 6 times after load and never again (Native1084), so no character receives its
per-frame model update. Next: why the WorldChrMan scheduling task stops in native mode.
Static 1085: `8DD2E0` is tail-called at the end of `8DF920`, the action of the WorldChrManImp
task at +0x31A8 (`SprjEzUpdateTask<SprjEzRabbitTask,WorldChrManImp>`, vtable `285AED0`,
constructed in `D6A30` beside sibling RabbitTasks at stride 0x30 and ten
`SprjEzVoidTask<SprjEzTask,WorldChrManImp>` (vtable `285AF08`) registered once there for
steps 0x10-0x41). The RabbitTask execute `65D290` runs the action only when armed (+18,
set by register `65CAC0`) and otherwise calls vtable+28, so the WorldChrMan update stops
when nobody re-arms it each frame. `8DF920`'s entry jumps first into the Arxan guard
`144E67691`. The per-frame re-arming site is still to be identified.

Static 1086 and Native1086-1093 (character body drawn natively). The WorldChrMan update RabbitTask +31A8 is
re-armed every frame by MoveMapStep (state table `459F630`, STEP_MoveMap = `8F2CA0`) -> `8FA3E0(step, running, dt,
p4)` -> `8DBDA0(WorldChrManImp, mode)`: mode 0 (running = `[step+270]`) registers +31A8 at step 0xE plus +31D8,
+3208, +32C8..+33B8; modes 1/2 (pause, Remo cutscene, event or menu states) register only +3238/+3268/+3298.
Native1086: running = 1 on every logged call. Correction to 1084: "8DD2E0 runs only 6 times" was the witness cadence
(first 6 calls, then every 300th) on runs that ended before call 300; Native1087's probe shows every witness hook
intact (first byte E9) and the player's model update (D01610, weight 0.05) runs each frame. The real cause of the
missing body was downstream: (1) the body parts 0/1 of the player's ChrAsmModel entity
(`[[[WorldChrMan]+80]+20B0]+8`) carry `[[part+28]+50]` = `GXSR::GXBloodyMaterialPatch` (vtable `3D41DF0`, RTTI),
not a physical model stream (D3F7C0 receiver); the GXSR material patch is polymorphic (vt+18 applies it per mesh:
`D3FA10` binds vertex slot 0 for physical streams, `5E8A0` binds only decal textures t41-t43/t53 for the bloody
patch), and native admission refused any unregistered patch (1088-1090; fixed: a bloody patch keeps the authored
geometry, its decal textures stay under the neutral decal provider policy); (2) the body materials' pixel
cbMatDynParam word 0x190 FC_SSS_ID had no producer (1092): `22E930` (asm `14023002A`-`1402300A0`) writes, when
source+21 is set, +190..+19F = {float(id),0,0,0}, id 0 without provider `[source+370]`, else `[[source+368]+30]` when the
provider's vt+30 query is true and `[source+368]` is set, otherwise `[[provider+10]+30]` (1093, produced natively).
Native1093: body parts drawn (part 1 13/13, part 0 5/7: two meshes miss textures), 300 content draws, 0 debug errors,
posed player body visible (screenshot `i66-native-sss-id-1093/native1093-corps-joueur.png`). Still open: part 0's two
textureless meshes, armour/skin textures (body drawn grey), the three other 44-part owners that never receive an
update (weight-0 entities) and part 42 (current inputs unreadable).
Native1094-1096: the drawn body meshes are `GXFlver_SkinSSSIbl_Bloody` with their genuine textures (FC_M_0000_a 2048
albedo, _r, _n, _m SSS mask, dither, neutral decals); the pose snapshot advances every frame (generation 35 -> 4005,
body pose differs between runs). 22E930's +20 -> +180 FC_DynamicDiffuseColor block (asm `14022FF53`-`14023002A`:
provider `[source+360]` absent -> 16 bytes at `4929940`; else colour `[+350]` when provider+54 and object+1E, else
`*provider->vt+70`; scale `[+35C]` when provider+41 and object+1F, else `[provider+50]`; alpha `[492994C]`) is now
produced natively; for the body materials flags +20/+21 are clear (both words stay zero), so the grey skin is not a
missing cbMatDynParam word. Open: the skin tone source (likely the SSS/skin composition outside the explicit open
lighting policy, not established) and the face's FaceGen composition target.
Reference1097 and Native1098, static 1099 (armour). Under the original renderer the player wears full armour; the
grey native body is the bare skin under it (the draw entity +280, read as FC_SkinMatchColor, is zero under the
original renderer too; ChrAsmModel+280 is a position). The armour meshes (programme family `SAT_Equip`, policy flags
0xC004) classify as material class 0x100 (142130: flags & 0x8000) and the hair (`SAT_HairIbl2`, forward transparent)
as class 2; native evaluates only engine passes 5/6 (class masks 1|4 or split rows 1/4, and 8). `57A20`'s row list
(entries: engine pass, preparation, class mask, secondary 0xFF) adds, for model +0x224 bit 15, the row (7, 3, 0x100)
(and a depth row (7, 1, 0x100)); bit 12 adds (4, 1, 0x80) and (0x1F, -, 0x80); without context+0x10 the forward row
(0x29, -, 0x12). So the armour is drawn by engine pass 7 and the hair by the forward pass 0x29: next, decompile pass
7's shader pass/programme selection and render state before admitting it natively.
Native1100 (negative): native pass preparation now evaluates an engine7 row (class 0x100) for models with +224 bit 15
(explicit open policy: engine5 G-buffer state), but the armour parts 2-6 are unchanged (same gate-21 counts, no new
draws): the selected part models do not carry +224 bit 15, so 57A20's `lVar20` (`param_9->vt+8(param_9, 0)`) is
not the part model native reads, or the row comes from another caller. Next: witness +220/+224 of the armour part
models and decompile the caller chain of 57A20 for SprjAsmModelDrawEntity parts (D02A30 -> CFDE70 -> D079C0) to find
the object whose +224 gates the engine7 row.
Native1101, Reference1102, Native1103: natively every character-part model keeps +220 = 0xFFFF and +224 = 0, while
under the original renderer they hold the union of the part's material classes (parts 2-5: 0x100; part 1: 0x111;
part 0: 5) and policy flags (armour 0xC004, bit 15); the model constructor `9DC20` zeroes both and the accumulating
writer runs on a path native model admission does not execute (not yet located). Native selection now produces both
unions for character parts from its own classification (current-LOD meshes) and pass preparation uses them for the
engine5 split and the engine7 row (open policy: engine5 G-buffer state). Native1103: armour parts 2-5 now reach draw
preparation (3/12/2/7 meshes) but are not drawn: their pose palette (cbObjMatrix) is unavailable because their native
pose receivers (part+50, initialized) never receive an update; D01610 sends type-1 parts to `D2D040` and type-2 (bone
attached) parts to `D2CED0`. Next (1104): a pass-through D2D040 witness per receiver.
Native1104-1112 (armour drawn natively). D2D040 runs for every part each frame (body directly through 16000; armour
through its retarget map over the 136 entity matrices) and the native 16000 owner updates all receivers (1104-1105).
The armour (SAT_Equip) then needed, in order: the cancel palette (VS b12 cbClothCancelObjMatrix): native pose owners
took auxiliary_present only from the part model's +224 bit 14, left at 0 natively; the native union of material policy
flags now supplies it, giving cancel = identity rows (1106-1107); VC_isEnableCloth (+80, 140850: written only with
material+22, = draw state +180 bit 8 and not bit 10; open policy 1 only with a native cloth reference, creation zero
otherwise) (1108, 1110); the generated SPX word +624 (authored words kept despite one parameter-animation row: open
policy until 22D620/22E5C0 animation is decompiled) (1109); and cbMtdParam FC_AlphaRef, written by no declaration:
produced from the MTD by name as for composed groups (ProduceMtdParams), with the creation zero for partly written
variables (1111-1112). Native1112: armour parts 2-5 fully drawn (3/3, 12/12, 2/2, 7/7), 121 draws, 328 content draws,
0 debug errors, 0 faults: the player is drawn in full armour (screenshot
`i66-native-armour-mtd-builtin-1112/native1112-joueur-en-armure.png`). Still open: the face (FaceGen composition
target), part 6 and the hair (class 2, forward pass 0x29), part 0's two face meshes, three weight-0 asm owners, part 42.
Static 1113 and Native1113 (FaceGen face composition; exports `face-composition-1113`, run
`i66-native-face-composition-1113`). The face diffuse of `GXFlver_FgFace*` materials is the FaceGen composition target
FrpgFgTextureIns+28 (512x512, DXGI 0x1C, bind 0x28 = SRV|RTV through 2BCE0/2BD50; 2D410 keeps 0x1C, so resource and views
are UNORM). Chain: ChrAsm loader C42620 creates the FgTextureIns (82A910 -> 835820, +B10) and waits for the CPU image
(8362C0 on the FaceGenMan worker, status +70 = 2); C336F0 hands it to the part record (C488E0: [asm+D8]+28+i*50, dirty
bit 21); C48180 bit 21 -> drawEntity vt+A0 = D009E0 copies it to part object +48 (part objects +B10, stride 108).
C36FC0/C3F980 then call 8368C0 (mark layer: +40 texture from asm slot 0x1C's model, +48/+4C centre, +50 angle in
degrees, +54 scale, +60 colour = FaceGen colour 7) and 836E40: at +70 == 2 it sets 3, uploads +18 into +20, draws +20
over the whole target with the 2D drawer (232FD10 -> 2378 9E0: centre 256,256, extent 512, texcoords 0..1, colour0 1),
then, when +40 is set, a rotated square (2330050 -> 2379C90: corners c +- (s/2)(cos,sin) +- (s/2)(-sin,cos) in a 2D
virtual space x*2048, y*1024 whose extent is [[4961AB0]+88]+F8/FC, texcoords (0,0),(1,0),(0,1),(1,1), colour +60);
it ends with +70 = 0, +91 = 1. CFDE70 passes the texture set +30 (SprjFgMdlTextureSet, 51B5C0) only when +91 is set;
it replaces the model textures flagged 0x80 by D39170 (file stem `FG_face_gen`, default `SYSTEX_Dummy`).
Native (1113): 836E40 hooked; the base layer is drawn on the native queue into a native 512x512 UNORM target
(`TextureIngress::AdmitComposedFace`, explicit open policy: linear clamp sampling), engine status +70 = 0 / +91 = 1 are
kept; `FG_face_gen` slots bind the part's composed target (part object +48, +91) or `SYSTEX_Dummy`. Native1113: player
mark layer present (centre 0.47/0.12, angle 0, scale 0.27, colour ~0) and reported, not drawn (explicit open policy:
the 2D virtual extent comes from the old display record, absent natively); part 0 7/7 drawn (was 5/7), 331 content
draws, 0 debug errors, 0 faults, restoration verified. The face itself was not visible (closed helmet; the in-game
menu did not open under the pilot, so the helmet could not be removed). Still open: the mark layer, part 6 and the
hair (class 2, forward pass 0x29), three weight-0 asm owners, part 42.
Armour family inventory 1113 (D-0161; `armour-families-1113`, script `Scan-ArmourMaterials.ps1`, Haiku 5.5 worker,
spot-checked against the I-0063 scan: same 1176 armour FLVER leaves). 590 LOD0 armour pieces (HD/BD/AM/LG, base game
and DLC): 260 only `SAT_Equip` (the family drawn natively by Native1112), 163 `SAT_Equip` + `SAT_Equip_cloth` (drawn,
cloth static under the open cloth policy), 66 other opaque families (mostly `GXFlver_ColDifSpcBumpIbl_Bloody`, also
MultiBlend2 Equip, SSS, em), 15 with blend-2 materials (veils, `_al`) and 86 phantom-only (`P_Ghost_FDP`, blend 2):
the latter two need the forward pass. Face/hair: `P[ChrCustom_Hair]_al` (SAT_HairIbl2, blend 2) on 156 FLVERs,
`P[ChrCustom_Hair]` (alpha ref 128) on 74. Only the player's current set is witnessed; the rest is inferred by family.
Reference1114 (`i66-reference-forward-1114`, pass-through 140680 witness): in game the original renderer prepares
GXFlverMaterial with shader passes 6 Gbuf (state +18C 196/964/9156), 0 Fwd (+18C 2240/2176, 86k calls), 2 Dpt, 12
WatDpt; 4 FwdSdw only around the title/menu; never 7. Pass table (3D52xxx pointer array at 473CE60): 0 Fwd, 1 Velo,
2 Dpt, 3 DptA, 4 FwdSdw, 5 FwdSSdw, 6 Gbuf, 7 Gbuf2, 8 Pnt, 9 PntCube, 10 PntCol, 11 SO, 12 WatDpt, 14 Aof, 16
Voxelization, 17 FwdDbg, 18 FwdSdwDbg, 19 GBufDbg. 57A20's forward row is (engine pass 0x29, preparation 0, class mask
0x12, secondary 0xFF); with context+10 two rows at sort keys +-0.5. So the forward family draws `<family>_Fwd`
programmes. The 2D virtual extent [[4961AB0]+88]+F8/FC read during scene draws was 1280x720 (window size); its value
during the FaceGen mark draw is not established. The in-game menu opens under the original renderer (Esc, R removes
the selected equipment slot) but not natively (Native1113): a native menu/UI family gap to inventory.
Native1115-1117 (native forward pass family; runs `i66-native-forward-1115`, `-forward-inputs-1116`/`1116b`,
`-forward-scene-1117`). Native evaluates 57A20's forward row (0x29, preparation 0, class mask 0x12) with the `_Fwd`
programmes (recipes 6-8) and draws into the lit HDR image after GXLightComposite (depth tested, not written). Inputs:
PS cbLight/cbFog = the model's own 0x120 light entry and 0x60 fog entry built by the engine's builders from its group
key (same layouts as the deferred arrays); explicit open policies: IBL t30/t31 black cube, SSAO t33 white, RLR t44
zero, local-light buffers t59..t62 empty (one zero element each) and the light-grid words of cbSceneParam zero, MTD
blend 0 opaque / 2 alpha blend (others refused, reported). Generic engine rules added on the way: a texture record the
loader binds with no data (D514C0 null) takes the default holder SYSTEX_DUMMY (D4A7D0/D4A910: GX system entry 0
B580(owner,0), SYSTEX_WHITE), by record name (e.g. SYSTEX_DummyVector, dummy128, absent from every archive);
cbModelParam's parallax-corrected cubemap words keep the creation zero when both boxes are disabled. Native1117: 13
forward draws per frame (player hair 11, part 6, other), 345 content draws (331 before), 0 debug errors, 0 faults,
restoration verified; screenshot `native1117-passe-avant(-zoom)`. The hair stays hidden by the closed helmet, so the
forward result is not visually compared yet. Native1116 (pilot deviation, Load Game list) faulted at 57F8A3 (rcx 0x4E0)
on the native Load Game screen: menus family, deferred to the menus workstream (ROADMAP "Sequencing").
Still open for characters: local lights in forward (clustered lists), IBL/RLR, the FaceGen mark layer, phantom variants
(menus first), armour family witnesses with the Shebang save, three weight-0 asm owners, part 42.
