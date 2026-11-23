# I-0067 mechanism map (static)

Static reading of DarkSoulsIII.exe 1.15.2.0 (canonical database, read-only). Addresses are
absolute (`0x140000000 + RVA`); exported functions are in `decompiled/DS3-1152-F-<RVA>.c`.
Class names come from MSVC RTTI (`I0067Rtti.java`): the complete inventory, 10,700 vtables
and 96,278 virtual slots, is in the Evidence Store at `evidence/I-0067/rtti/classes.tsv`
(SHA-256 `5AC0988E…`) and `vslots.tsv` (`C8B00F09…`), regenerated in about 10 s by
`Invoke-Map.ps1 -Out <dir> -ScriptName I0067Rtti.java`. Ghidra's own symbol table has no
class names for this program; these tables are the way to name `FUN_` addresses.
Raw query outputs cited below (call-site lists, data dumps, string searches) are kept as
`evidence/I-0067/queries/qNN-index.txt` and registered with hashes in the evidence manifest.

Status words follow the ledger: STATIC = read in code/data; INFERRED = interpretation.

## 1. Menu offscreen model renders (family) and the Load Game fault

### Classes (STATIC, RTTI)

| Class | vtable | Role |
|---|---|---|
| `NS_SPRJ::CSMenuAsmModelRend` | base (FD4 step: `SprjStepLocal<CSMenuAsmModelRend>`) | menu model renderer driven as a step |
| `NS_SPRJ::CSMenuProfModelRend` | `14295D938` (23 slots) | save-slot profile portrait |
| `NS_SPRJ::CSMenuPlayerModelRend` | `14295D508` | player view in menus (INFERRED: the Status/Equipment character view of K-0441, 560x768) |
| `NS_SPRJ::CSMenuFaceModelRend` | `14295D0D8` | character-creation face view |
| `NS_SPRJ::CSOffscreenGxTexture` | `14295DA78` (17 slots) | the GX texture each renderer draws into (secondary vtable inside the object) |
| `NS_SPRJ::CSOffscreenRend` | `14295DB30` | offscreen renderer base |

Shared base constructor `FUN_140D65DD0(obj, name, width, height, flag, …)` builds the
step, the offscreen target (`FUN_140D644F0`/`FUN_140D64C30(target, width, height)`),
three model/draw slots and the appearance block. Per-class size tables:
`CSMenuProfModelRend` uses `14295D5D0`, ten rows of 0x18 bytes `{name, 128, 128, 0, 0x14}`:
ten 128x128 portraits. The data after the `CSMenuFaceModelRend` vtable holds rows of 0x310 x 0x310 (784x784) and 0x230 x 0x300 (560x768); which constructor reads them is not yet established.

### Lifetime of the ten save-slot portraits (STATIC)

- Owner: `AppMenu::TitleTopDialog` (vtable `1428D39C8`, 11 slots). Constructor
  `FUN_140AD8900` (object 0xCD0, built by the factories `FUN_140BF3960`/`FUN_140BF38B0`; the factories were read in a query session and are not exported)
  calls `FUN_140AE5B70`.
- `FUN_140AE5B70`: releases any previous portraits (`FUN_140AED060`), then for slot 0..9
  allocates 0x910 bytes from the general heap `[14479C270]` and constructs
  `CSMenuProfModelRend` (`FUN_140D6B780(obj, slot)`), storing it in the global array
  **`14478D680[10]`**.
- `FUN_140AED060`: for each slot, `FUN_1408190B0([14477D778], obj)` releases it, then the
  slot is set to 0. Called by the constructor (re-creation) and by the TitleTopDialog
  destructor `FUN_140AD9820` (slot 1 `FUN_140ADE080`, scalar deleting destructor).
- Per frame/update: `FUN_140ADE0B0` visits non-null slots (`FUN_140D66900`).

So the portraits exist exactly while the title top dialog exists.

### Load Game summary path to the faulting copy (STATIC)

`AppMenu::MenuSaveDataSummary` job lambda (`_Func_impl` slot 2 `FUN_140AE8DC0`) →
`FUN_140AD7DD0` → `FUN_140ADAAA0` → **`FUN_140ADEF50`**:

```
for slot in 0..9:
    obj = 14478D680[slot]                 // no null check
    rec = FUN_1405ACA10(saveList, slot)   // save summary record
    if rec && FUN_140C318B0(slot):
        FUN_140D6B9F0(obj, rec+0x150)     // appearance -> obj+0x4E0
        FUN_140D675A0(obj, rec+0x4C) ... FUN_140D668A0(obj, slot*2) ...
```

`FUN_140D6B9F0` starts with `FUN_140D67590` (12-byte thunk: `rcx += 0x4E0; jmp
FUN_14057F880`). `FUN_14057F880` copies the 0xEC-byte appearance block (22 entries through
`FUN_140A2F480`, 22 dwords, 12 bytes, one more entry); its first store is at
**`14057F8A3`** `MOV [RCX+8], EAX`.

### Native1116 explained (STATIC + the pilot's observation)

The observed fault `57F8A3` with `rcx = 0x4E0` is this store with `obj = 0`:
`14478D680[slot]` was null for a slot whose save record is valid. Only the
`FUN_140D67590` thunk adds 0x4E0; the only `CSMenuProfModelRend` caller of it is the
MenuSaveDataSummary path above, so the fault belongs to the Load Game list.

Static owners of a null slot (to be discriminated at runtime; INFERRED ranking):

1. The `TitleTopDialog` destructor (`FUN_140AD9820` → `FUN_140AED060`) ran before the Load
   Game summary job, or the summary runs under a menu flow where no TitleTopDialog exists.
2. `TitleTopDialog`'s constructor (`FUN_140AD8900` → `FUN_140AE5B70`) never ran in the native
   flow.
3. The heap `[14479C270]` returned null (unlikely: it is the general heap used by
   `CSPartsModelIns`, file caps, model draw entities).

Discriminating runtime check for the pilot (no fix implied): at the Load Game screen read
the ten qwords at `14478D680` and whether a `TitleTopDialog` (vtable `1428D39C8`) is alive;
optionally break on `FUN_140AE5B70` and `FUN_140AED060` during title → Load Game.

### What the renderer must provide for this family (INFERRED from the constructors)

Each menu model renderer owns an offscreen GX texture of its own size (portraits 128x128,
other rows 784x784 and 560x768, owner not yet established), runs as an FD4 step, and draws a
character model (appearance block at +0x4E0, model/draw slots) into that texture, which the
Scaleform menu then samples. A native backend must create these offscreen targets and draw
their model views; the menu logic itself does not depend on the backend except through
these objects. Covering it by family: one witness each for Prof (Load Game list), Player
(Status/Equipment) and Face (character creation).

## 2. In-game menu opening (NewMenuSystem) and Native1113

### Owners (STATIC)

| Object | Where | Created by |
|---|---|---|
| `NS_SPRJ::MenuMan` instance (0x1F78 bytes) | global `[144763258]` | MenuMan init `FUN_140763C10` (`FUN_14075BFE0`), also written by `FUN_140761000` |
| `AppMenu::NewMenuSystem` (0x3098 bytes, vtable `1428E6520`) | global `[14478DA40]` (also kept at `[144763268]`) | `FUN_140763C10` → `FUN_140B3E9C0` → ctor `FUN_140B3AE50`, **only when** `DAT_1447632B0` (= `FUN_14080EF40()`, computed once) is non-zero |
| `AppMenu::NewIngameTopMenuWindow` (vtable `1428F34C8`, 19 slots) | root window of NewMenuSystem (`[sys+0xA0]`) | factory lambda `_Func_impl` `1428F3838` → ctor `FUN_140B658D0` |
| `InGameMenuStep` (vtable `14285E5C8`; K-0069 group 0x47) | game-flow step | ctor `FUN_1408EB340` (from `FUN_1408EA490` ← `FUN_1408E93E0`) |

MenuMan accessors read `[144763258]` at fixed offsets (`FUN_1407614E0`..`FUN_140761B30`);
`FUN_140761610(i)` returns the dword `[[144763258] + 0x34 + 4*i]` (a per-index menu
request/state array).

### Opening path (STATIC, read from a forced listing)

The NewMenuSystem update starts at `140B3E1D0` with a protection stub (`JMP 144E25F7D`);
Ghidra's function there has a 5-byte body, so its callers/callees are missing from the
database. Its readable body from `140B3E217` contains:

```
if [sys+0xA0] == 0:                                   // no root window open
    if FUN_14075EB60() and FUN_140761610(8) == 7:     // gate + MenuMan state[8] == 7
        [sys+0x158] = 1
        w = FUN_140B3EBB0(sys+0x40)                   // open "MenuMan.IngameTop"
        FUN_140B3DD10(sys, w)                         // install as root window
    else if [sys+0x140] and !FUN_140763F10() and !FUN_14075EBC0() and FUN_140A5BC10(...):
        FUN_140B37F50(...) → FUN_140B3DBE0(...)       // second root kind (not identified)
else: update the open root window ([sys+0xE8] vtable +0x10, FUN_140B3C550 ...)
```

`FUN_140B3EBB0` reads the configuration key `MenuMan.IngameTop` (UTF-16 at `1428E6528`) and
calls the name dispatcher `FUN_140B52030`, which tries three registries
(`FUN_140BF4620`, `FUN_140B0BCD0`, `FUN_140BE2F80`) and then compares the name with
`Top` (`1428EF410` → `FUN_140B53AA0` → `FUN_140B66A70` → NewIngameTopMenuWindow),
`Inventory` (`1428183A0` → `FUN_140B515C0`), `EquipTop` (`1428EF418` → `FUN_140B4F3D0`),
`EquipSel…` (`1428EF430`), …; default `FUN_140B53AA0`.

`NewIngameTopMenuWindow`'s constructor creates no renderer object (Scaleform scene binding
`FUN_140A4B7C0` only). The Status screen (`AppMenu::PlayerStatusMenu`, ctor
`FUN_140B71320`) is the menu that uses the player offscreen model render: it calls the
`CSMenuPlayerModelRend` appearance copy `FUN_140D6A8A0` (section 1 family).

### Encrypted functions on this path (STATIC)

`FUN_14075EB60` (the opening gate) and `FUN_14080EF40` (the NewMenuSystem creation
condition) are unreadable at rest: their bytes decode to invalid instructions, unlike the
plain code around them. Static reading cannot recover them; the project's read-only
loaded-code route (`I-0066/Read-LoadedCode.ps1` + `I0066LoadedCode.java`, from a running
process) can. Ghidra function bodies that start with such stubs are truncated, which is why
`FUN_140B3EBB0`, `FUN_140BF3960` and `FUN_140BF38B0` showed no caller (their callers sit in
undisassembled code; `I0067Map.java` query `e:` finds them by raw rel32 scan).

### Native1113 (in-game menu does not open natively): static owners

The opening window needs no renderer object, so the refusal is upstream of the window.
Static candidates, to discriminate at runtime (INFERRED, no fix implied):

1. `[14478DA40]` is null: NewMenuSystem was not created (`FUN_14080EF40` returned 0).
2. `[[14478DA40]+0xA0]` is non-null: another root window is still installed (a title-flow
   or dialog window never removed in the native flow), so the top menu is never opened.
3. MenuMan state `[[144763258]+0x54]` never becomes 7 when Esc is pressed (input route).
4. The encrypted gate `FUN_14075EB60` returns false.

Runtime check for the pilot: on Esc in game, read `[14478DA40]`, `[[14478DA40]+0xA0]`,
`[[144763258]+0x54]`, and whether `FUN_140B3EBB0` is reached; and, read-only, acquire the
loaded bytes of RVAs `0x75EB60`, `0x80EF40` and `0xB3E1D0` once to decompile the gates.

### Update after Native1120 (pilot measurements + decrypted gates)

Pilot measurements in native at Firelink (`evidence/I-0066/artifacts/menu-gates-loaded-1120/`,
I-0066 branch): `[14478DA40]` non-null, `[sys+0xA0] = 0`, `[[144763258]+0x54] = 0` before
and after Esc. Owners 1 and 2 are excluded; the blocked condition is MenuMan state[8].

Decrypted bytes acquired by the pilot (`ingame-before-esc.json`, identical after Esc),
overlaid in memory with `I0067Map.java` query `L:` (hash- and identity-checked):

- `FUN_14075EB60` = cached `FUN_14080EF40()` result (static bit `1447632B4`, value
  `1447632B0`): the same flag that decided NewMenuSystem creation. NewMenuSystem exists
  natively, so this gate passes (owner 4 excluded).
- `FUN_14080EF40` computes the gate once (static bit `14477D5C4`): it reads the configuration key
  `MenuMan.NewSystem` (UTF-16 at `14283D8E8`) with `FUN_14179ADC0([14477D4B0], key, 1)`, stores the
  result in `DAT_14477D5C0` and returns it (the `.c` export types it void; see the `.asm.txt`
  at `14080F037`/`14080F053`). `[144929E28]` is used as an allocator-like object for the
  temporary string (INFERRED role).
- `140B3E1D0` stays a 5-byte jump into protected code `144E25F7D` at runtime: it is a
  relocation stub, not an encrypted body; its caller is the MenuMan update
  `FUN_140767AC0`.

**Writer of state[8] = 7 (STATIC).** The MenuMan state setter is `FUN_140765220(index,
value)` (`[[144763258] + 0x34 + 4*index] = value` when `index < 500`; 678 call sites, `queries/q38-index.txt`). Argument recovery at
every site finds exactly one `(8, 7)` writer: `1408FB7CD` in **`FUN_1408FB6D0`**, the
in-game menu input mapper, called every frame by the in-game step update `FUN_1408F9AD0`
(itself reached only indirectly):

```
ctx = input context from map [[1449647A0]+0x38] keyed 0x144751927 (FUN_140462780 creates it)
if !FUN_140BEA4E0() and !FUN_1407641B0():      // menu not blocked
    FUN_140765600(1)  -> [MenuMan+0x2E4] = 0
    FUN_140765640(1)  -> [MenuMan+0x2E8] = 0, state[0] (+0x34) updated
    if FUN_1407FDB10(ctx): FUN_140765220(8, 7) // menu button triggered -> open request
    if [FUN_140D0FC00([144791A38])+0xC] > 0: [MenuMan+0x1E29] = 1
else:                                           // blocked
    FUN_140765600(0)  -> [MenuMan+0x2E4] = 1
    FUN_140765640(...); [MenuMan+0x1E29] = 1
    other button checks FUN_1407FE350(ctx) / FUN_1407FE470(ctx) -> [144791248] actions
```

`FUN_140BEA4E0` blocks when the flag `+0x142` of the object returned by `FUN_140B3CDA0` (INFERRED: the NewMenuSystem instance, it reads `[14478DA40]`) is set or when
`FUN_140CE2790([144791248])` (bit 0 of `[[144791248]+8]+0x154`) is set.
`FUN_1407641B0` returns `FUN_140719FE0()` (constant 0) or clears `[MenuMan+0xD8]`.

**The menu-button test `FUN_1407FDB10(ctx)` is encrypted at rest** (its bytes decode to a
jump outside the image), as are its neighbours `FUN_1407FE350` and `FUN_1407FE470`.

Remaining static owners of Native1113 (INFERRED ranking):

1. `FUN_1407FDB10(ctx)` returns false natively: the menu action is not seen in the input
   context (keyboard/pad route or window focus of the native presentation window).
2. The blocked branch runs (`FUN_140BEA4E0` true): `[MenuMan+0x2E4]` would read 1.
3. `FUN_1408FB6D0` does not run at all (the in-game step update not executing).

Runtime discrimination (read-only): `[[144763258]+0x2E4]` (0 = allowed branch ran, so
owner 1; 1 = blocked branch, owner 2); and the loaded bytes of RVAs `0x7FDB10`, `0x7FE350`,
`0x7FE470` to decompile the button tests. Whether keyboard movement works natively in the
same run also separates a general input-route defect from a menu-action-only one.

## 3. Menu music and sound effects

### Owners (STATIC)

- Audio middleware: FMOD through `DLMO::MOFmodSoundManager` (vtable `143A7AD48`, 126
  slots; interface `DLMO::IMOSoundManager` `143A76408`), cues `DLMO::MOFmodCue`, emitters
  and bank packs (`MOFmodBankPack`). Game services: `NS_SPRJ::SprjSoundImp` (`142997368`),
  `SprjFD4SoundImp` (`142996860`), sound instances `SprjFD4SoundIns` (`142996A28`),
  `SprjFD4SystemSoundIns` (`142996C08`), `SprjFD4LocateSoundIns`; steps `SoundStep` and
  `SprjSoundStep` (K-0088). Bink movies use `Movie_FMOD_Bink_CG2::MVMoviePlayerImplBink`
  for their sound.
- Menu sound object: global **`[144763868]`** (0x1F0 bytes), created by the MenuMan init
  `FUN_140763C10` → `FUN_14076B850` → ctor `FUN_14076AFA0`; 57 (0x39) SE channel handles
  at `+0x14 + 8*i`, BGM handle `+0x1DC` with current BGM id `+0x10`, one more handle
  `+0x1E4`. A second MenuMan-owned table `[144763850]` (0xA08 bytes, ctor `FUN_140769FC0`)
  is used by menu code `FUN_1406A4AF0` and `FUN_1408F8250` (role not read).
- Sound request naming `FUN_1404AD2D0(category, id)`: one letter from the table at (dump `queries/q35-index.txt`)
  `142786578` (`a c f o p s m v x b g y z e`, index = category) followed by the 9-digit id;
  `FUN_1404AD510` builds and issues the request, `FUN_141A299F0` plays it on a handle and
  `thunk_FUN_141A29EB0` stops it.

### Menu sound API (STATIC)

| Function | Category | Use |
|---|---|---|
| `FUN_14076B930(id)` | 2 `f` (menu SE) | play on the current channel (`FUN_14076BA90`); 178 call sites |
| `FUN_14076B8B0(ch, id)`, `FUN_14076BC30(obj, ch, id)` | 2 `f` | play on channel `ch` |
| `FUN_14076B920(id)`, `FUN_14076BBA0(obj, id)` | 6 `m` (music) | switch menu BGM if different or stopped; `AppMenu::TitleTopDialog` calls `FUN_14076B920(1)` |
| `FUN_14076B9F0()` | | stop menu BGM |
| `FUN_14076B960(id)` | 7 `v` | play on handle `+0x1E4` |

### What the renderer has to do with it

Nothing in these paths touches the graphics device: menu SE are requested by menu logic
(INFERRED: from menu window code) and played by FMOD. A native defect here would
come from the menu logic not running (sections 1–2), not from rendering. Title music was
heard natively (I-0066 witness 486), which agrees.

## 4. Loading screens, fades, screen capture and movies

Mechanism families (D-0161) and their owners. Existing claims reused: K-0106 (SprjDrawStep
publishes `CSFakeLoadingScreenImp`/`SprjAdhocDrawImp`, read by `0x00D55190`), K-0107/K-0109
(`SprjGraphicsImp` `0x00D573B0`: `+0x60` is `SprjScreenCapture`), K-0175 (`RendManImp`
constructor `0x140DFC850`).

| Family | Classes (vtable) | Owner and creation (STATIC) | Renderer request (INFERRED unless noted) |
|---|---|---|---|
| Fade plates | `SprjFadeImp` (`14294A500`), `SprFD4FadeSystem` (`14294A250`), `SprFD4FadePlate` (`142949D70`) | `RendManImp` ctor `FUN_140DFC850` → `FUN_140D0F6E0` builds the fade system (`FUN_140D0DFE0`) and **7 named plates** (`FUN_140D0D110`, names from `14463E0C8`, dump `queries/q45-index.txt`): `Title`, `MapIn`, `InGame`, `Remo`, `InRemo`, `Ending`, `InGameChroma`; each plate holds two RGBA colours (`+0x10`, `+0x20`, from `FUN_1417A3730`) | full-screen colour overlays composited at the plate's layer (consumer not yet read) |
| Fake loading screen | `CSFakeLoadingScreenImp` (`14295DB80`) | global `[144793C28]` (0x18 bytes), created by the SprjDrawStep ctor `FUN_140D53EE0` (`FUN_140D6C700`), flag `+8` set to 1 | read by the draw side `0x00D55190` (K-0106) |
| Screen capture | `SprjScreenCapture` (`14295E2A0`), `SprjScreenCaptureIns` (`14295E308`), `SprjEzScreenCapture` (`14295DEB0`) | `SprjGraphicsImp` `FUN_140D573B0` → ctor `FUN_140D6DE70` (stored at graphics `+0x60`, K-0109); instances `FUN_140D6E4C0` (size field `0x80`) | copy of a rendered frame into a texture (used by loading/fake loading and menus; consumers not yet read) |
| Movies | `CSMovieImp` (`1429A45F0`), `CSMovieIns` (FD4 step, `1429A50D8`), `CSMovieGxTexture` (`1429A4968`, 17 slots), Bink player `Movie_FMOD_Bink_CG2::MVMoviePlayerImplBink`, `MVBinkGraphics` | `CSMovieImp` ctor `FUN_140EAC5B0` from the system init `FUN_140EDED40`; movie textures `FUN_140EADB80` (default texture source `[[144792E58]+0x60]`) | Bink frames uploaded into a GX texture each movie frame; Bink open `0x01ADF230` (existing export). Full static map (planes, shaders, constants, RGBA target, XAudio2 audio, what the native build omits): [I-0069 BINK-MOVIES-NATIVE.md](../I-0069-static-menu-audio-map/BINK-MOVIES-NATIVE.md) |
| Loading/fade/movie screens (UI) | `FadeScreen` (`1428EF858`), `LoadingScreen` (`1428EF8B8`), `KnowledgeLoadingScreen` (`1428EF918`), `MovieScreen` (`1428EFA08`); old-style `FrpgMenuDlgNowLoading`, `FrpgMenuDlgFadePlate`, `FrpgMenuDlgMovieDisplay` | ctors `FUN_140B49F50`, `FUN_140B44C40`, `FUN_140B45140`; helper `CSNowLoadingHelperImp` (`142884B78`, ctor `FUN_140959E70` from `FUN_140EDED40`) | Scaleform screens: no renderer object beyond the UI path, except a movie texture for `MovieScreen` |
| Camera fade | `CSCamFadeSlot` (`14288CC98`), `CSCamFadeSlotInstance` | `FUN_1409891A0` | fades geometry close to the camera (material parameter, scene side) |

What the native backend has to supply for transitions: draw the fade plates, provide the
screen-capture copy, upload Bink movie frames into their GX textures, and keep the fake
loading screen draw path; the loading screens themselves are UI windows.

## 5. Effects (SFX / FFX)

### Layers (STATIC, RTTI + constructors)

| Layer | Classes (vtable) | Creation |
|---|---|---|
| Game service | `NS_SPRJ::SprjSfxImp` (`142992EF8`); step `SfxStep` (`142863868`, ctor `FUN_1409051A0`); world `WorldSfxManImp` (`1427E6B60`, ctor `FUN_14068C7E0`, consumes the world header, K-0070) | `SprjSfxImp` ctor `FUN_140E6C9D0`, called from `FUN_140D5AA40` (I-0066 legacy inventory: global `4799C98`) |
| Resource containers (game → GX) | `SprjFfxShaderContainer` (`142957138`, ctor `FUN_140D53340`), `SprjFfxGxTextureRepository` (`142956FD8`, `FUN_140D53270`), `SprjFfxFlverContainer` (`142956F68`, `FUN_140D53100`), `SprjFfxSoundManager` (`142957230`, `FUN_140D538A0`), `SprjFfxDebugDrawer` | all built by `FUN_140E6C9D0` |
| Engine FFX core | `GXFFX::GXFfxManager` (`143D6A078`, ctor `FUN_14024C530`; slot 9 builds `GXFfxAdapter` `143D66148`), `GXFfxSceneCtrl` (`143D64008`, ctor `FUN_14024B260`), `GXFfxGraphicsResourceManager` (`143D66198`, ctor `FUN_14028F470`) | manager and scene control from `FUN_140E6C9D0`; resource-manager creator not read (INFERRED: inside the FFX manager) |
| Per-frame draw | `GXFfxDrawContext` (`143D65688`, 29 slots; built by SceneCtrl slot 8 `FUN_14024D000`), `GXFfxDrawPass` (`143D6A1F8`; built by SceneCtrl slot 10 `FUN_14024E790`), `GXFfxDrawPassMulti` (`143D64108`), `GXFfxUpdateDrawPass`, jobs `GXFfxUpdateRootJob`/`GXFfxUpdateJob`/`GXFfxDrawPassProceedJob` | SceneCtrl |
| Appearance families | `GXVfxStandardParticleAppearance` (`143D65878`), `GXVfxSparkParticleAppearance` (`143D65D38`), `GXVfxParticleAppearanceLightShaft` (`143D65A88`), `GXVfxFluidAppearance` (`143D659C8`), `GXFfxParticleAppearanceHost_Model` (`143D64B18`) + `GXFfxFlverModelRenderer` (`143D6A298`), `GXWaterInteractionAppearance` (`143D65DC8`), force-field areas (`GXForceField{Wind,Gravity,Cancel}AreaAppearance`), `GXCollisionFieldAreaAppearance` | per effect definition (FXR, K-0281) |
| Effect-spawned lights | `GXFfxDrawEntityHost{FrpgPoint,Spot,Line}Light` | per effect |

### Mapping to the I-0063 effect coverage families (INFERRED from class names)

- E03 model particles → `GXFfxParticleAppearanceHost_Model` drawn by `GXFfxFlverModelRenderer`.
- E06 effect lights → `GXFfxDrawEntityHost*Light`.
- E08 light shafts → `GXVfxParticleAppearanceLightShaft`.
- GPU particle simulation of K-0436 → the standard/spark particle appearances.
- Water interaction and fluid are separate appearance families with no I-0063 witness yet.

The renderer contract is the GX layer under `GXFfxDrawPass`/`GXFfxDrawContext` and
`GXFfxGraphicsResourceManager`; the game side (SprjSfxImp, SfxStep) only requests effects.
Covering effects natively means one witness per appearance family, not per effect.

## 6. Phantoms and ghosts

### Phantom shading parameters (STATIC)

`PlayerIns`/`ReplayGhostIns` virtual slot 154 (`FUN_1408C7B30` → `FUN_1409CAD60(chr, kind)`):
for kind 1 `P_Ghost_FDP_Gesture` and kind 2 `P_Ghost_FDP_Replay` (UTF-16 names at
`142898A48`/`142898A70`; kind 0 = none), it looks the named material up in the repository
`[144792560]` (`FUN_140D434B0`) and copies four shader parameters from it into the
character (`FUN_1409CBBE0`): **`g_GlowScale`, `g_GhostEdgeColor`, `g_GhostEdgeParam`,
`g_BlendMode`**. The render side resolves `g_GhostEdgeColor` by name in a material's
parameter table (`FUN_140D40790` → `FUN_140D430A0(mat, 0x31, name)`).

This joins the I-0066 forward-pass finding: 86 armour pieces carry `P_Ghost_FDP` materials
drawn only by the forward pass (blend 2). So phantom rendering = forward-pass `P_Ghost_FDP`
programmes + these four per-character constants.

### Classes (STATIC)

- `CSDisplayGhost` (`1428A19A8`, 22 slots; ctor `FUN_1409F67B0` ← `FUN_1405309D0` ←
  `FUN_14053A730`).
- `ReplayGhostIns` (`142858AA8`, 228 slots, a `PlayerIns`-shaped character; ctors
  `FUN_1408CA2B0` (from `FUN_14089E560` ← `FUN_14089FE80` ← `FUN_1408D8F60`),
  `FUN_1408CA460`, `FUN_1408CA5B0`).
- Network side (no renderer role): `GhostDownloadJob`, `GhostUploadJob`,
  `BloodstainGhostDownloadJob`, `CSSosSignSummonJob`, `FrpgNetWanderingGhostDb`,
  `Frpg2RequestMessage::*Ghost*`/`*SummonSign*`.

### Open

RESOLVED below (PhantomParam). Original note: the summoned-player phantom (I-0065 Argo case) is not proved to use `FUN_1409CAD60`: its
kinds cover gesture and replay ghosts. The summoned-phantom parameter source is the next
static question (INFERRED candidate: a phantom parameter table selected by the
character's team/phantom type).

### Update after Native1121: the menu-button test (decrypted)

Pilot measurements (Native1121, `evidence/I-0066/artifacts/i66-native-cloth-cancel-1121/`):
`[[144763258]+0x2E4] = 0` and state[8] = 0 before and after Esc, keyboard movement works.
So the mapper's allowed branch runs and `FUN_1407FDB10(ctx)` returns false. Its decrypted
bytes (`menu-input-loaded.json`, RVAs `0x7FDB10`, `0x7FE350`, `0x7FE470`, 512 bytes each)
were overlaid with query `L:` (export `decompiled/DS3-1152-F-007FDB10.loaded1121.*`):

```
FUN_1407FDB10(ctx):
  if FUN_14080B810():                 // encrypted, unread: selects the test path
      ok = FUN_1408029A0(ctx)          // encrypted, unread
  else:
      ok = FUN_14233DE40(ctx, 0x21)    // input action 0x21 triggered on this context
           and the action's entry in [ctx+0xA0] is absent or its flag +0x20 is 0
  if ok:
      if FUN_1407642B0() != 0 → false  // menu-blocked reasons mask (below)
      if FUN_1407FDF20(ctx) or FUN_1407FD4C0(ctx) → false   // encrypted, unread
      ok = ok and [[1447572B8]+0x10]+0x18 >= 1                // PlayerGameData field (INFERRED: hit points)
  chr = FUN_1408D9D70([14477FDB8])     // local player character
  if chr and ok: chr vtable +0x2D0 → +0x110 must be true; FUN_1409E3CF0([chr+0x1F90]+0x18) false;
                 chr state flags 0x75 and 0x88 (FUN_140A00930) clear unless [14477FE94]
  … (continuation past 0x7FDD10 not acquired)
```

`FUN_14233DE40(ctx, action)` is the generic input-context action test: an action is
suppressed when present with its flag set in `[ctx+0x88]`; otherwise it walks the action's
key bindings in `[[ctx+0x38]]` and tests the key bitset `[[[ctx+0x10]+8]+0x50]`
(`FUN_14233D540`), gated by `[ctx+0x20]` and per-binding device checks `FUN_14233D690` /
`FUN_14233D630` (maps at `[ctx+0x40]`). `FUN_1407FE350(ctx)` is the same test for action
`0x1F`; `FUN_1407FE470(ctx)` evaluates bindings of action `0x21` (analog `FUN_14233D490`).

**Menu-blocked reasons `FUN_1407642B0()` (STATIC, plain code)**: a bit mask computed only
from MenuMan state `s[i] = [[144763258]+0x34+4*i]`, a few MenuMan fields and four bytes:
`s[2]`→0x200, `s[3]`→1, `[MM+0x4C]`→0x20000, (`s[0x11]`|`s[0xB4]`|`s[0xB5]`|`[14475F3A8]`)→8,
(`s[0x2A]` or `s[0x13]` or `s[0x3B]` or `s[0xB1]`|`[144760678]` or (`s[0xB0]` and
`[1447605EA]`))→0x8000000, `[MM+0x7C]`→4, `s[0x1F]`→0x800, `s[0x33]`→0x40000000,
`[MM+0xB8]`→0x20, `[MM+0xB4]`→0x10, `[MM+0xA0]`→0x400, `s[8]`→0x40, `s[9]==4`→0x80,
`s[9]` in 1..3→0x80000, `s[0x34]`→0x100000, `[MM+0x60]`→0x1000, `[MM+0x64]`→0x2000,
`[MM+0x68]`→0x4000, `s[0x40]`→2, `s[0x38]`→0x2000000, `[MM+0x138]`→0x8000,
`[MM+0xC4]`→0x10000, `s[0x28]`→0x40000, `s[0x15]`→0x400000, `[MM+0x90]`→0x200000,
`[MM+0x98]`→0x800000, `s[0x1A]`→0x1000000, `s[0x2F]`|`[1447609A0]`→0x10000000,
`[MM+0x2DC]`→0x100, `[MM+0x8C]`→0x4000000; finally, if the mask is 0 or `[MM+0x118] > 0`
and a NewMenuSystem root window exists, 0x800008. Each `s[i]` is an "another menu or
dialog is open" state (e.g. `s[0x13]` belongs to `FrpgMenuDlgEnvOption`).

What the native side can check from memory alone: this mask (non-zero blocks the menu);
`[[1447572B8]+0x10]+0x18 >= 1`; the player character pointer. Still encrypted and unread:
`FUN_14080B810`, `FUN_1408029A0`, `FUN_1407FDF20`, `FUN_1407FD4C0` and the continuation of
`FUN_1407FDB10` from `0x7FDD10`.

### Update after Reference1124: complete menu-button test and the state-2 observation

Reference1124 (original renderer, `evidence/I-0066/artifacts/i66-reference-outfits-1124/`):
before Esc state[8] = 0, `[MM+0x2E4]` = 0, `[[1447572B8]+0x10]+0x18` = 669 (the displayed
maximum hit points); just after Esc state[8] = **2** and a root object is installed in
NewMenuSystem (vtable `142905F60`, in the AppMenu MenuJob vtable region; no RTTI locator
found for it). Natively state[8] stays 0 and no root object appears.

**The 7 → open route still holds (STATIC).** 7 is a one-frame request: the NewMenuSystem
update consumes it and `FUN_140B3EBB0` immediately writes state[8] = 0 (call site
`140B3EBFF`, `(8, 0)`), then the opened menu sets other values: the only `(8, 2)` writer is
`FUN_1406EC300` (and `(8, 3)` in `FUN_1406EC280`/`FUN_1406EB790`, the latter
`FrpgMenuDlgStartTab` slot 16), INFERRED to be the opened menu's own state. A read after
the opening therefore sees 2, never 7. Natively the root object never appears, so the
request was never made (if 7 had been written and not consumed, the read would show 7).

**Complete test (decrypted bytes of RVAs `0x7FDB10`+`0x7FDD10`, `0x80B810`, `0x8029A0`,
`0x7FDF20`, `0x7FD4C0`; exports `*.loaded1121`/`*.loaded1124`):**

```
FUN_1407FDB10(ctx) -> bool
  if FUN_14080B810():                          // config bool "Game.SwitchGestureMenuButtonToStart"
                                               // read once (FUN_14179ADC0([14477D4B0], key)) into DAT_14477D550
      ok = FUN_1408029A0(ctx)                   // that layout: menu = an action among 0x2E, 0x20, 0x13..0x16
                                               // with binding 0x20C (gesture/start swap)
  else:
      ok = FUN_14233DE40(ctx, 0x21) and no flagged entry 0x21 in [ctx+0xA0]
  if ok and (FUN_1407642B0() != 0              // menu-blocked mask (formula above)
             or FUN_1407FDF20(ctx)             // another action (0x2E, via FUN_1407FDDB0) takes precedence
             or FUN_1407FD4C0(ctx)):           // action 0x20 pressed and not suppressed
      ok = false
  if ok and [[1447572B8]+0x10]+0x18 < 1: ok = false          // hit points
  chr = FUN_1408D9D70([14477FDB8])                           // local player character
  if chr: require ok, chr->vt[0x2D0]()->vt[0x110](), !FUN_1409E3CF0([chr+0x1F90]+0x18),
          and (if !byte [14477FE94]) chr flags 0x75 and 0x88 clear ([chr+0x11C8])
  require [MM+0x83C] <= 0.0                                  // FUN_1407614E0() + 0x10
  talk = [14489F1E0] (SprjWorldTalkManImpl, vtable 1429B9338; created by FUN_14094C050)
  require (talk+0x1FDC == -1 or byte talk+0x1FD8) and talk+0x1FEC <= 0 and talk+0x1FF0 <= 0
  return ok
```

Every clause except the input-action evaluation itself is a plain memory read. Native-vs-
reference discrimination (read-only, in game, before Esc): `DAT_14477D550`; the
menu-blocked mask inputs (state table, MenuMan fields, the four bytes); `[MM+0x83C]`;
`talk+0x1FD8/0x1FDC/0x1FEC/0x1FF0`; `[[1447572B8]+0x10]+0x18`; the player character
pointer and its `[chr+0x11C8]` flags. If all of these match the reference, the failing
clause is the input action `0x21` itself: then compare, at the moment Esc is held, the
context's key bitset `[[[ctx+0x10]+8]+0x50]` and its gate byte `[ctx+0x20]`, where `ctx` is
the in-game context found by `FUN_1408FB6D0` in `[[1449647A0]+0x38]` (key `0x144751927`).

### Phantom colours from PhantomParam (STATIC)

Every character's per-frame update `ChrIns` slot 36 (`FUN_140890950`; `PlayerIns`/
`ReplayGhostIns` slot 36 `FUN_1408C1BC0`, `EnemyIns` slot 36 `FUN_1408AA980`) calls
**`FUN_1409CAF30(chr, request)`** (also used by `ChrWepEnchantVfxSlot` slot 3
`FUN_1405B34F0`). It reads one **PhantomParam** row (`FUN_140E35EF0` → `FUN_140E36260(row,
id)`: `FUN_140E3DB80([144798108], 0x3A)` returns the SoloParamRepository resource for param
index 0x3A = `PhantomParam` (name table `14464FF80`), then a binary search on row id) and
writes seven per-character material extra parameters through `FUN_1409CAE60`:
`ExParam_PhantomIsEdgeSubtract`, `ExParam_PhantomIsFrontSubtract`,
`ExParam_PhantomEdgeColor`, `ExParam_PhantomFrontColor`, `ExParam_PhantomDiffColor`,
`ExParam_PhantomSpecColor`, `ExParam_PhantomLightColor` (name pointers `1445B3978..39A8`).
Colours come from the row as three bytes /255 plus a float (`FUN_140E36020`: bytes
`+0x14..+0x16`, float `+0`), with a default from `14479809x` when the row is absent.

So phantom rendering has two parameter sources feeding the forward-pass `P_Ghost_FDP`
programmes: the PhantomParam ExParams above for every phantom-type character (summoned
phantoms included), and the four `g_*` values of `FUN_1409CAD60` for gesture and replay
ghosts. The renderer side resolves these names in the material's parameter table
(`FUN_140D40790`/`FUN_140D430A0`). Generic fact: `FUN_140E3DB80(repo, index)` is the
param-resource getter of `SoloParamRepositoryImp` (`[144798108]`).

### Fade plates: how they are drawn (STATIC)

`RendManImp` (global `[144796298]`, 0xE0 bytes, ctor `FUN_140DFC850` called from
`FUN_140D59C10`) owns `SprjFadeImp` (`[144791A38]`). Each frame the RendManImp method
`FUN_140DFDC50` (called from the draw side `FUN_140D54F80`) calls `SprjFadeImp` slot 1
**`FUN_140D0FE60`**:

1. `SprFD4FadeSystem` slot 1 `FUN_140D0E910` updates every plate (`FUN_140D0D510`).
2. Unless `FUN_14075EAD0()`, `SprFD4FadeSystem` slot 2 **`FUN_140D0E790(plate, list,
   0x780, 0x438)`** draws each visible plate (alpha `+0x2C > 0`) into the 2D draw list
   `[RendManImp+0x40]`: blend state 7 on the list's current layer, the plate RGBA packed by
   `FUN_1402D94F0`, then a full-screen rectangle `FUN_14232FDF0` in a **1920x1080 virtual
   canvas** (size from `(0x780+2, 0x438+2)` scaled by constants).
3. Plate 6 (`InGameChroma`) is not drawn as a quad: its colour is passed to
   `FUN_140E004F0([RendManImp+0x10], colour)` (INFERRED: a colour-grading tint).

**Correction (I-0069, [FADE-PLATES-NATIVE.md](../I-0069-static-menu-audio-map/FADE-PLATES-NATIVE.md)):** `FUN_140D0E910` composes the seven plates (in plate order, "over") into one RGBA colour at `system+0x20..0x2C`, and `FUN_140D0E790` records **one** rectangle for it, not one per plate. The menu input gate `FUN_140A5B930` tests that plate 2 is *finished* (`FUN_140D0FCB0`, remaining time at most 0), not its alpha.

Plate colours are set by `FUN_140D0FD40(fade, plate, colour)` and read by
`FUN_140D0FC00(fade, …)`; fade actions such as `FUN_140D11530`/`FUN_140D116B0` animate
plate 2 (`InGame`). The menu input mapper refuses or delays input while that plate's alpha
`+0xC` is above 0 (section 2).

Renderer contract for the family: draw the 2D list of `RendManImp+0x40` (full-screen quads
with blend state 7, in a 1920x1080 virtual canvas scaled to the real output) and apply the
`InGameChroma` tint.

## 7. How the original game produces the save-slot portraits (STATIC)

Each `CSMenuProfModelRend` (section 1) is an offscreen character render driven by the
generic `CSMenuAsmModelRend` step machine.

### Construction (`FUN_140D65DD0`, base of the family)

1. `FUN_140D644F0(texture, name)`: creates a `CSOffscreenGxTexture` (`FUN_140D6C0F0`) and
   registers it by name in the texture repository `[144792F58]` (`FUN_140D4A120`), so that
   Scaleform finds it. Portrait names are **`SYSTEX_Menu_Profile00` .. `09`** (row pointer
   of the size table `14295D5D0`; the rows also give 128x128). It keeps `[RendManImp+8]`
   (the RendManImp offscreen list) at `+0x18`.
2. `FUN_140D64C30(offscreen, w, h, …)`: builds a private GX scene graph for this texture:
   two camera/view objects (`FUN_14004AFF0`, at `+0x20`/`+0x28`), a scene (`FUN_140CED370`,
   `+0x30`), a GX scene object (`FUN_14004FDB0`, `+0x38`), a second scene list
   (`FUN_1400C2400`, `+0x40`), a GX render target created through the factory at
   `14295BA48/50` with the size `w x h` (`+0x48`; cleared once to (0,0,0,1) with
   `FUN_141920200`, flag `+0x78 = 1`, clear colour `+0x6C = 0x141414`), a light object
   (`FUN_140D039E0`, `+0x50`) whose colours come from `14478CC68..CC84`, and registers the
   render callback **`FUN_140D658B0`** (encrypted at rest) with the offscreen scheduler
   (`FUN_140D65080(&14295BB10, this, FUN_140D658B0)`).
3. Camera and light parameters come from the param **`MenuOffscrRendParam`** (index 0x58,
   via `FUN_140E336E0`; the row id is the size table's `+0x14` field), applied by
   `FUN_140D6B8D0` (construction) and `CSMenuProfModelRend` slot 22 `FUN_140D6BBF0`.

### Step machine (`FUN_140D66B80` executes; state table filled by `FUN_14268AB80` at `144793990`)

| State | Function | Work |
|---|---|---|
| `STEP_Init` | `FUN_140D677B0` | next |
| `STEP_Wait_Request` | `FUN_140D67D10` | waits for a request flag (`+0x6C8`) |
| `STEP_Init_Setup` | `FUN_140D678C0` | copies the appearance block (`FUN_140D664D0`), creates a **`ChrAsmModelRes`** (0xB18, `FUN_140C42230`) |
| `STEP_Wait_Setup` | `FUN_140D67D50` | loads the character parts from the appearance and equipment (`FUN_140C42620`), then builds the model (`FUN_140D690A0`, `FUN_140C3BA60`) |
| `STEP_Finish_Setup` | `FUN_140D67730` | applies pose/appearance (`FUN_140C3B460`, `FUN_1408816D0`) |
| `STEP_Init_Play` | `FUN_140D677D0` | starts the animation, then **starts rendering** (`FUN_140D64B50`) |
| `STEP_Wait_Play` | `FUN_140D67940` | keeps rendering while active; toggles with `FUN_140D653A0`/`FUN_140D65140` |
| `STEP_Finish_Play` | `FUN_140D676D0` | renders 5 more frames (`+0x87C`), then **stops rendering** (`FUN_140D65140`) and releases (`FUN_140D68EB0`) |
| `STEP_Finish` | `FUN_140D67690` | back to the request state on a new request |

"Start rendering" (`FUN_140D64B50`) adds the render target `+0x48` to RendManImp's
offscreen list (`FUN_1400C0230([offscreen+0x18], target)`); "stop" removes it. While
listed, the render callback draws the private scene (the character model with its own
camera and light) into the 128x128 target each frame; afterwards the texture keeps the last
image, which the Load Game list shows by name.

### What the native backend must provide for this family

Render a small private scene (one assembled character, camera and light from
`MenuOffscrRendParam`) into a named texture while it is in RendManImp's offscreen list,
through the same model/material path as the world; keep the result when it leaves the list.
The render callback `FUN_140D658B0` is encrypted: its loaded bytes (RVA `0xD658B0`) are
needed to read the exact pass list it records.

## 8. Who creates the title top dialog (and so the portraits) (STATIC)

The Load Game portraits live as long as `AppMenu::TitleTopDialog` (section 1). Its
creation chain:

- **`FUN_140BF5080(windowStack, sceneHolder)`** opens the title top window. Unless a
  pre-title notice applies (`FUN_140908B00`: `[144781B80] > 0`; `FUN_14090CA70`:
  `[144782060] == 4`; `FUN_14090CAA0`: either), it pushes one of two
  `std::function<MenuWindow*(SceneHolder&)>` factories (vtables `1429189D8` and
  `142918918`, slot 2 = thunks `140BF79D0`/`140BF79E0` → `FUN_140BF38B0`/`FUN_140BF3960`,
  which allocate 0xCD0 bytes and run the TitleTopDialog constructor `FUN_140AD8900`) through
  `FUN_140BB4F60`; in the notice case it first pushes the notice window (factories
  `142918998`/`142918958`, `FUN_140B9D0E0`).
- Callers: the title steps of the game flow `FUN_140908CC0`, `FUN_1409095D0`,
  `FUN_14090AA40`, `FUN_14090CDE0` (the `TitleStep`/`E3TitleStep`/`SimpleTitleStep`/
  `TrialTitleStep` family) and the window-by-name registry `FUN_140BF4620` (first resolver
  of the dispatcher `FUN_140B52030`, section 2) and `FUN_140BF35E0`.

So the portraits exist from the title step's opening of the title top until the dialog is
destroyed (`FUN_140AD9820`). The native Load Game fault (null portrait) therefore points to
a native title flow where the Load Game list runs without this dialog alive (for example
after the pre-title notice path, or after the dialog was popped). Runtime check, unchanged:
read `14478D680[0..9]` and whether a `TitleTopDialog` (vtable `1428D39C8`) exists when the
Load Game list opens.

### Note on the menu input context (section 2)

The input context the menu mapper uses (key `0x144751927` in `[[1449647A0]+0x38]`) is the
same one used by movement (`PadManipulator` slot 9, `PlayerIns` slot 22 and about thirty
other readers). Keyboard movement working natively therefore shows that the context and
its device route exist; the failing part of the menu test is specific to action `0x21`
(its bindings, its suppression entry in `[ctx+0x88]`, its flagged entry in `[ctx+0xA0]`) or
to the configuration switch `Game.SwitchGestureMenuButtonToStart` (`DAT_14477D550`).

### Menu sound effect ids (STATIC, call sites of `FUN_14076B930`, 180 sites)

Immediate SE ids passed (category `f`): 0 (57 sites), 100 (48), 200 (37), 300 (8), 81 (3),
999999999 (2), 1, 18, 75, 162, 270, 400 (1 each), 19 sites with a computed id. INFERRED:
0/100/200 are the cursor, confirm and back sounds of every menu window. Menu music is set
by `FUN_14076B920(1)` from the TitleTopDialog constructor and from `FUN_140ADA260`.

## 9. Screen capture and movie textures: who consumes them (STATIC)

This section answers the open questions of section 4.

- **`SprjScreenCapture` is a screenshot-to-file facility.** It is not a texture source for
  menus or loading screens.
  - Its vtable `14295E2A0` is followed by the format list `bmp`, `jpg`, `png`, `tif`, `gif`
    and `wmp` (`14295E310`).
  - Requests come from `FUN_140D6CD30`: it builds a path, then queues a 0x60-byte
    `SprjScreenCaptureIns` (`FUN_140D6E120`, then `FUN_140D6E4C0`) with format, size and
    flags.
  - `FUN_140D6CD30` is called through the EzTask wrappers `FUN_140D6CFA0..FUN_140D6D4E0` and
    the debug path `FUN_140CD9BF0`.
  - The native backend does not need it for menus.
- **Movies reach the UI by name, through a texture proxy.**
  - The `CSMovieIns` constructor `FUN_140EAE1D0` creates a `CSMovieGxTexture` (`FUN_140EADB80`).
  - It registers that proxy under the movie's name in the texture resource manager:
    `FUN_140D4A120([144792F58], name, proxy)`. This is the same manager the repositories of
    TONEMAP-SKIN section 12 query.
  - `CSMovieGxTexture` is a GX texture proxy:
    - `+0x10` is the inner texture, by default `[[144792E58]+0x60]`;
    - `+0x18` is the current frame;
    - the SRV slot `+0x10` returns `[this+0x18]` when it is set, and otherwise forwards to the
      inner texture's slot;
    - size and format slots behave the same way (`+0x3C/+0x40/+0x48` when playing).
  - So `MovieScreen` and other Scaleform consumers see a texture object that was never built
    by the GX texture construction boundary.
  - The writer of `+0x18`, the Bink frame upload, is INFERRED on the `CSMovieIns` /
    `MVBinkGraphics` side; it was not read.
- **Family (D-0161):** `CSMovieGxTexture` belongs to the GX texture proxy family that shares
  slot 0 with `IGXTexture`. The family also includes `SprjGxTexture`, `SprjMiddleGxTexture`,
  `SprjDummyBlinkGxTexture`, `CSOffscreenGxTexture` and `GXBS::GXTextureBaseProxy` (RTTI
  annotation of vtable `1429A4968`).
- **Native consequence (INFERRED):** an ingress keyed by constructed GX textures has to
  resolve proxies through their slots, or through the name registry `[144792F58]`, to find the
  current resident texture.

## 10. Effect point lights: creation and registration in the GX light list (STATIC)

This section answers the pilot's report that 104 effect lights are missing natively, for
example the fire-orb flame and bonfire embers.

### The light manager and the shared list

- `[GXRS+0x41D0]` is `GXSR::GXLightManager` (vtable `143D1C4B8`, constructor `FUN_140017EF0`).
  `FUN_140048B80` creates it at `blk+0x10` with `blk = GXRS+0x41C0`; the env map bank is
  `blk+0xE8 = GXRS+0x42A8`.
- Its `+0x10` vector is the light list.
- **Effect lights and map lights go into the same vector** through the same `push_back`,
  `FUN_14001AE00(mgr+0x10, &light)`:
  - map lights: `FUN_1400182F0` / `FUN_140018490` then `FUN_1400180D0`;
  - effect lights: `FUN_140266FC0`.
- The native run has all map lights, so the list and its collection into `GXLightOrganize`
  work. The missing effect lights were never created.

### Chain for one effect point light (all CPU)

1. **Action types are registered.** The `GXFfxSceneCtrl` constructor `FUN_14024B260` registers
   the FXR action types into the global registry `[1449651D0]`, which counts entries at
   `[1449651D8]` (`FUN_140253710`, 0x48-byte entries, sorted by id). Each entry holds the
   type id at `+0` and the draw-entity-host factory at `+0x30`.
   - Type `0x261` (609, point light): factory `FUN_14028F200`.
   - Type `0x2AF8` (11000): factory `FUN_14028F290`, which builds a 0xF0-byte host
     (`FUN_1402B4080`).
   - Type `0x2AFA` (11002): factory `FUN_14028F320`, which builds a 0xD0-byte host
     (`FUN_1402B5BA0`).
   - The 11000 and 11002 hosts are INFERRED to be the spot and line hosts, from the
     `GXFfxDrawEntityHostSpotLight`/`LineLight` classes.
2. **The host is built.** The factory `FUN_14028F200(action, params)` builds a
   `GXFfxDrawEntityHostFrpgPointLight` (0x120 bytes, vtable `143D6A838`, base
   `FFX::FXDrawEntityHost`, constructor `FUN_1402B2410`). It passes the container
   `[[[action+0x10]+0x38]+8]+0x220` and the action transform `action+0xA8`.
3. **The light is created.** The constructor calls `FUN_140266FC0(container+0x110)`, which
   works under the container lock (`+0x28`, slots `+0x10`/`+0x20`):
   - it allocates a 0x1B0-byte `GXSR::GXPointLight` from `[[GXRS+0x41D0]+8]`;
   - it constructs it with `FUN_1400EFCB0`;
   - it pushes it into `[GXRS+0x41D0]+0x10`;
   - it stores it at `host+0x28`.
   The constructor then copies the action parameters, for example the light's
   `+0x184..+0x18C`. It sets `light+0xB0 = 0` (off) until the first update.
4. **The light is updated each frame.** Host slot 2, `FUN_1402B2E30`:
   - evaluates the FXR property curves through the tables `[1449327A0]`/`[144932E90]`;
   - writes colour times intensity at `light+0x150..+0x15C`;
   - sets the visibility flag `light+0xB0` (and `+0xB1`) when it changes;
   - the light is visible only when the scaled intensity is positive, the owner mask test
     passes (`[owner+0x1F0]+0x14C` against `owner+0x1F8`), and the optional distance limit
     passes.

### Where the native run can lose them (INFERRED, to discriminate at runtime)

- No step of this chain touches the device or `[GXRS+0x4DD0]`.
- The factory is reached only through the FFX middleware callbacks, by the registry lookup
  `FUN_140253670`/`FUN_1402544C0`. No static caller is visible.
- Effect lights are therefore missing natively only if:
  - the FFX instances never build their draw entities (the FFX scene update or draw
    preparation is not run), or
  - the registry was never filled (the `GXFfxSceneCtrl` constructor did not run, or `DAT_1449651E8`
    blocks it).
- Runtime discriminators:
  1. the registry count `[1449651D8]`, nonzero with an entry of id 0x261;
  2. the number of calls to `FUN_1402B2410` / `FUN_140266FC0`;
  3. the size of `[GXRS+0x41D0]+0x10`, about 163 entries in the original against 59 natively;
  4. calls to `FUN_1402B2E30`, which show whether hosts exist but stay off (`+0xB0 = 0`).

## 11. The portrait scene in detail: what the native pipeline has to reproduce (STATIC + one encrypted gap)

This section completes section 7 for a native reproduction.

### Scene object

`FUN_140D64C30` builds the scene for each `CSMenuProfModelRend`.

- **Camera.** Two GX camera/view objects (`FUN_14004AFF0`): `+0x20` (0x120 bytes, two views)
  and `+0x28` (0x90). A camera holder `+0x30` (`FUN_140CED370`, linked to `+0x28` by
  `FUN_140CED790`).
- **Scene content.** `+0x38` is a `GXSgGeneralScene`-type object (`FUN_14004FDB0`, flags set
  by `FUN_14004FEC0`). `+0x40` is a layer/scene list (`FUN_1400C2400`), attached by
  `FUN_1400C1E30`. This is where the character model is placed.
- **Renderer.** `+0x48` is a **`GXSR::GXSgCompositeScene`** (vtable `143D4A108`), created by
  `GXSR::GXSgSceneFactory` (vtable `14295BA48`, slot 1 `FUN_1400C3410`) from the descriptor
  `{flags 0x8422014, width, height, …}`.
  - `FUN_1400C3410` allocates a **0x4170-byte render pipeline instance** (`FUN_1400843C0`).
    The `GXRenderingSystem` constructors `FUN_1400213A0`/`FUN_1400220A0` instantiate the same
    pipeline type for the world.
  - It configures that instance with `FUN_140085540` at the portrait size. The shadow map
    size is `2 * [GXRS+0x4E90]`, or 0x400 when that is 0. Descriptor bit 21 selects a
    4-sample setting (`0x400000004` instead of 1).
  - **The portrait therefore uses the world's own GX scene pipeline at 128x128, as a second
    instance.**
- **Wiring.** `vt+0x30` sets flags `0x200000000`, `vt+8` attaches the scene `+0x38`, and
  `vt+0x28` attaches the camera `+0x20`. `FUN_1400C1C60(scene, 100)` sets a priority or order.
  `FUN_140088AF0` sets a mode pair `{FUN_140D27770(1), param}`.
- **Clear.** The output texture (`vt+0x18` then `+0x1E0`, `FUN_140087F80`) is cleared once to
  (0, 0, 0, 1). Then `+0x78 = 1` and the background `+0x6C = 0x141414`.
- **Light.** `+0x50` is a light object (`FUN_140D039E0`, 0x40 bytes), linked to the scene
  list (`FUN_140D03AC0`). Its colours come from `.data 14478CC68..14478CC87` (two RGBA
  vectors, `FUN_140D03C00`). Three further fields are zeroed (`FUN_140D03BD0/BE0/BF0`).
- **Render callback.** `FUN_140D65080(&"sceneGUIWidget", this, FUN_140D658B0)` is registered
  through `[this+0x60]` (`vt+0x60`, with argument 1). `FUN_140D658B0` is encrypted at rest;
  its loaded bytes (RVA `0xD658B0`) were requested from the pilot.

### Camera from `MenuOffscrRendParam` (`FUN_140D6B8D0`; row from the size table's `+0x14`)

1. The row is read through `FUN_140E336B0`.
2. Row `+0x00..+0x0C` gives `obj+0x88C..+0x898`, with `w = 1.0`.
3. Row `+0x18`, `+0x10` and `+0x14` are multiplied by the constant `.rdata 143D7ED40` (INFERRED
   degrees to radians) into `+0x900`, `+0x8A4` and `+0x8A0`. Row `+0x0C` goes to `+0x89C`.
4. `FUN_140D6BC20` builds the camera transform. Its inverse (`FUN_140004500`) is stored at
   `+0x8C0..+0x8FF`.
5. The camera holder `[obj+0xB8]+0x30` receives:
   - the projection parameters `+0x50..+0x5C` from `obj+0x900..+0x90C`, where `+0x904` is the
     aspect `w/h`;
   - the view matrix `+0x10..+0x4F` (`FUN_1404095F0`).
6. The view is copied into the two views of `+0x20`. The second copy is made only when
   `DAT_144961BC2 == 0`.

### Output to the menu

"Start rendering" (`FUN_140D64B50`) does two things:

- it adds the composite scene to RendManImp's offscreen list (`FUN_1400C0230`);
- it points the named texture's inner texture (`[this+8]+0x18`, the `CSOffscreenGxTexture`
  `SYSTEX_Menu_ProfileNN`) at the scene's final colour texture.

The Load Game list samples the portrait through that texture proxy (section 9). "Stop"
(`FUN_140D65140`) removes the scene from the list; the proxy keeps the last image.

### Native reproduction (INFERRED)

1. Per portrait, run the native character pipeline into a 128x128 target, with:
   - the `MenuOffscrRendParam` camera above;
   - the one light from `14478CC68`;
   - one `ChrAsmModelRes` model built from the save's appearance block
     (`obj+0x4E0`, steps of section 7);
   - background `0x141414`.
2. Expose the result under `SYSTEX_Menu_Profile00..09`.

The exact pass subset (G-buffer and lighting, or a forward subset) awaits the bytes of
`FUN_140D658B0`.

## 12. The offscreen character-view family: every consumer and the one shared entry (STATIC)

### The two encrypted functions are debug-only

The pilot's loaded bytes (Native1160, `evidence/I-0066/artifacts/i66-native-loadedcode-1160/`,
overlaid with `L:`) show that neither function holds rendering or registration work.

- **`FUN_140D658B0` is not a render callback.** For event 1 it builds a debug GUI widget for
  the scene (`sgScene`, `.rdata 14295BB40`; `FUN_140D64480`, `FUN_1404526E0`). It is
  registered under `sceneGUIWidget`.
- **The portrait therefore has no custom pass list.** It is drawn as an ordinary
  `GXSgCompositeScene` listed in RendManImp's offscreen list.
- **`FUN_140D57EB0`** (end of the `SprjGraphicsImp` constructor) also only registers debug GUI
  widgets: seven `FUN_140D57C30` entries in `[144964480]`. The bytes are truncated after 512.
  - So the place that hands the repositories to their consumers (section 11 of TONEMAP-SKIN)
    is still unknown, and the earlier INFERRED attribution to `FUN_140D57EB0` is withdrawn.

### Scene flags: portrait against the main scene (STATIC; meanings partly INFERRED)

| | Main scene (`FUN_140DFEEB0`) | Character views (`FUN_140D64C30`) |
|---|---|---|
| How the flags are set | default `FUN_1400C2DA0` = `0x1000951EE1F`, minus `0x4010000`, plus `0x100` when `Graphics.IsEnableScaleRender` | fixed `0x8422014` |
| Bits | {0,1,2,3,4,9,10,11,13,14,15,20,22,24,27,40} | {2,4,13,17,22,27} |

What the shared bits do in the pipeline setup `FUN_140085540`:

- bit 13 creates the render-target stage `FUN_140087290`; its targets use the shared pool
  `EsramShared_Top`. INFERRED to be the G-buffer, so the views use deferred shading;
- bits 22 and 27 add further targets.

What the views lack compared with the main scene:

- **bit 10:** `FUN_140024740` uses the YEBIS exposure only when bit 10 is set. The views
  therefore have no YEBIS tone map; `+0x3E88` is not an exposure but a MotionBlur field, read there for `view+0xCA4` (I-0113 RESULT « Tranche 27 », section 27 below); the views are tone-mapped by the GX tone map filter at `pipeline+0x3060`;
- **bit 11:** the half-resolution targets (`+0x1120`, `+0xEB0`);
- **the `0x10040081E8` group:** the depth-downsample stage `+0x3EC0` (`FUN_1401CB6B0`,
  `ScaledDepth2x2`);
- **the `0x420` group:** the stage at `+0x3EB9`;
- **bits 20 and 40:** the stages `+0x2E80` and `+0x2ED8`.

Bit 17 is set only for the views. A capture of an original frame while a view is on screen
would give the exact draw list.

### Every consumer: three classes, one shared constructor

| Class (vtable) | Constructor | Size table (row = name, w, h, flag, `MenuOffscrRendParam` id) | Owners (STATIC; INFERRED where marked) |
|---|---|---|---|
| `CSMenuProfModelRend` (`14295D938`) | `FUN_140D6B780` | `14295D5D0`: `SYSTEX_Menu_Profile00..09`, 128x128 | `TitleTopDialog` (`FUN_140AE5B70`): Load Game list |
| `CSMenuPlayerModelRend` (`14295D508`) | `FUN_140D6A3B0` | `14295D1A0`: `SYSTEX_Menu_ChrMakePreview`, `…NudePreview`, `…Preview2`, `…NudePreview2` (784x784), `SYSTEX_Menu_StatusBody` and `SYSTEX_Menu_RankingBody` (560x768) | `CharMakePreviewWindow` (`FUN_140A84920`, rows 0..3); `PlayerStatusMenu` (`FUN_140B71320`, row 4, the Status screen) |
| `CSMenuFaceModelRend` (`14295D0D8`) | `FUN_140D69770` | `14295C9D0`: `SYSTEX_Menu_ChrMakeGrid00..14` and `KickFace00..02` (128x128), `StatusFace` (two rows) and `RankingFace` (512x256), `SignFace` (128x128) | `SignDialog` (`FUN_140BB76F0`, row 0x15); `FUN_140B72980`, called from `PlayerStatusMenu` (INFERRED status face); `FUN_140B20070` and `FUN_140B598A0` (INFERRED character-creation grid, kick and ranking faces) |

All three constructors call the shared base **`FUN_140D65DD0`**, which does the following.

1. **`FUN_140D644F0(name)`** builds a `CSEzOffscreenRend` (0x70 bytes, vtable `14295BA70`),
   stored at `obj+0xB8`. It holds:
   - `+8`: a `CSOffscreenGxTexture` proxy (`FUN_140D6C0F0`);
   - `+0x10`: the texture resource registered under the `SYSTEX_Menu_*` name
     (`FUN_140D4A120([144792F58], name, proxy)`);
   - `+0x18`: `[RendManImp+8]`, the offscreen list.
2. **`FUN_140D64C30(holder, w, h, …)`** fills the holder's GX scene graph (section 11):
   - `+0x20`/`+0x28`: camera views;
   - `+0x30`: camera holder;
   - `+0x38`: scene;
   - `+0x40`: layer list;
   - `+0x48`: `GXSgCompositeScene`;
   - `+0x50`: light;
   - `+0x60`: the debug GUI object.
3. It sets up the step machine and the three model slots (section 7).

`GXSgSceneFactory` has no other caller than the main scene (`FUN_140DFEEB0`).

### What every consumer expects back (STATIC readers)

- **Camera holder:** `[holder+0x30]` must be valid. It is written by `FUN_140D6B8D0` and by
  the `MenuOffscrRendParam` re-application (Prof slot 22 `FUN_140D6BBF0`, Face
  `FUN_140D698C0`, Player at construction).
- **Composite scene and the proxy's inner texture:**
  - `[holder+0x48]` (the composite scene) and its output texture (`vt+0x18`, then
    `+0x1E0`) must be valid;
  - "start rendering" (`FUN_140D64B50`) lists the scene and points the proxy's inner texture
    at that output;
  - "stop" (`FUN_140D65140`) removes it.
- **Model:** the scene and layer list must accept the `ChrAsmModelRes` model.
- **Menus:** they reach only the named proxy `SYSTEX_Menu_*`, through the texture registry
  `[144792F58]`.

### One native correction for the whole family (INFERRED)

Either create the GX scene graph objects of `FUN_140D64C30` natively, or replace them with
native objects that keep `holder+0x20..+0x50` valid. Then render each listed composite scene
with the native character pipeline into its named proxy. That covers:

- the Load Game portraits;
- the Status screen (body 560x768 and face 512x256);
- character creation (four previews at 784x784, fifteen grid faces at 128x128);
- the sign, kick and ranking faces.

## 13. Drawing phantoms: programmes, constants and colour source (STATIC + INFERRED selection)

This section completes section 6 and the PhantomParam part.

### Programmes: a phantom variant per material shader

- `FUN_140082E50(shaderSetManager, shaderName)` creates or finds the 0x3F00-byte shader set of
  a material shader. Its name is at `+0x3EB8`.
  - It loads the programmes of 17 passes (`FUN_1400AA630`).
  - Then **`FUN_1400A9EF0`** loads the phantom variants by name from the shader bundle (or
    from `[GXRS+0x4DD0]` without a bundle).
- The phantom variants are:
  - **`<shader>_Phantom_Fwd`** (pass 0, `Fwd`), with `<…>_[A]` (alpha test) and `<…>_[AA]`
    (alpha to coverage);
  - **`<shader>_Phantom_FwdSdw`** (pass 4), with `_[A]` and `_[AA]`.
- Formats: `%s_Phantom` and `%s_%s` at `.rdata 143D48A18`/`143D48A50`. The pass names come from
  the table `14473CE60`: `Fwd`, `Velo`, `Dpt`, `DptA`, `FwdSdw`, `FwdSSdw`, `Gbuf`, `Gbuf2`,
  `Pnt`, ….
- They are stored at `set+0x18/+0x20/+0x28` (Fwd) and `set+0x78/+0x80/+0x88` (FwdSdw).
- **A phantom is therefore drawn in the forward pass with the `_Phantom_Fwd` variant of each
  of its materials.** Phantom variants exist for the forward pass only; none exists for the
  G-buffer.

### Constants: the material constant block `+0x1A0..+0x1EF`

`FUN_14022D620` is the per-draw material parameter builder (called from `FUN_140140850`).

- It copies the material block (`FUN_14022E0F0`).
- It blends each `ExParam_Phantom*` in by the character's weight `w = entry[5]`. The value is
  looked up by index (`FUN_140146850`) or by CRC32 of the name (`FUN_140146920`; hashes in
  `144962330..58`, computed by `FUN_14022B7C0`).
- It calls the constant writer **`FUN_14022E930`**.
- When the block flag `+0x22` is set (a material that carries phantom parameters), that
  writer fills five float4 of the material constant block `[ctx+8]`. This is the same block
  that holds `FC_SSS_ID` at `+0x190`.

| Offset | Value |
|---|---|
| `+0x1A0` | edge colour: `rgb² * a * s`, `.w = a`, with `s = -1` when `ExParam_PhantomIsEdgeSubtract`, else 1 |
| `+0x1B0` | front colour: same, with `s` from `ExParam_PhantomIsFrontSubtract` |
| `+0x1C0` | diffuse colour `rgb² * a`, `.w = a` |
| `+0x1D0` | specular colour `rgb² * a` |
| `+0x1E0` | light colour `rgb² * a` |

Block offsets: subtract flags at `+0x70`/`+0x71`; colours at `+0x80`, `+0x90`, `+0xA0`,
`+0xB0` and `+0xC0`.

### Colour source per character

- PhantomParam (section 6, `FUN_1409CAF30`, every character each frame) writes the seven
  `ExParam_Phantom*` values. The row is selected by the character's phantom type (white,
  red, blue and the other families).
- Gesture and replay ghosts also get `g_GlowScale`, `g_GhostEdgeColor`, `g_GhostEdgeParam` and
  `g_BlendMode` from `P_Ghost_FDP_Gesture`/`Replay` (`FUN_1409CAD60`).

### Open (INFERRED)

- Which draw switches a character's materials from their normal programmes to `set+0x18..`
  was not located statically. It is INFERRED to happen when the character carries phantom
  ExParams; the feature `GXFT_ValidSubPhantom` (id 0x11, `.rdata 143D21040`) is a candidate.
- A capture of an original frame with a summoned phantom would confirm it, for example by
  finding forward draws using `*_Phantom_Fwd` programmes, with the constants above at
  `+0x1A0..`.

### Native requirements

1. Forward pass only.
2. For each material of a phantom character, the `<shader>_Phantom_Fwd` programme (or its
   `[A]`/`[AA]` variant).
3. The material constant block with `+0x1A0..+0x1EF` filled from PhantomParam as above.
4. For phantom shadows, `_Phantom_FwdSdw`.

## 14. How FFX draw entities (effect lights included) come to exist: registration, load-time resolution, gates (STATIC; the instance-time call site is open)

### 1. Type registration, done once in the `GXFfxSceneCtrl` constructor `FUN_14024B260`

The constructor runs in this order:

1. It creates its GPU constant buffers through `FUN_1419254F0(*[144961AB0], …)` (sizes 0xE0,
   0x1C0, 0x1C0, 0x230, 0x190).
2. It enters the type registration only if the byte `this+0x44 == 0`:
   - `FUN_141A97EE0`, then `FUN_14036D9A0` and its seven siblings `FUN_14036DB90..E010`
     (types 600, 0x25E, …);
   - then one block per FXR action type: 10000, …, 0x261 (point light, factory
     `FUN_14028F200`), 0x2AF8, 0x2AFA, …
3. Each type is entered twice:
   - into the GX descriptor registry `[1449651D0]` (`FUN_140253710`; entry `+8` validator,
     `+0x30` draw-entity-host factory);
   - into the FFX library type registry `[144933088]` (`FUN_141AB3A60` then `FUN_141AB3880`),
     as a 0x18-byte record `{id, resolver FUN_140253670, getter FUN_1402536F0}`.
4. Both registries are then sealed. Lookups work only after sealing:
   - GX side: `FUN_14036DB30` sets `DAT_1449651E8 = 1` and sorts. `FUN_140253670` returns
     nothing before that.
   - FFX side: `FUN_141A98500` seals eight library registries; the last one,
     `FUN_141AB3690`, sets `[144933088]+0x20 = 1`. `FUN_141AB3800` tests that byte.
     `FUN_141A98500` is also called from the library initialisation `FUN_141A985B0` (via
     `FUN_140915160`).

### 2. Load-time resolution of each FXR action node (`FfxFileCap`/`FfxbndFileCap` slot 6)

The chain runs from the file cap down to each action node:

1. `FUN_140E0D860`, then `FUN_140E0DD10`, then `FUN_140E6E570([144799C98] = SprjSfxImp, …)`.
   This **requires `[SprjSfxImp+0x58]`** (the FFX manager) to be non-null.
2. `FUN_14024ECF0(mgr, id, data, size)` looks up the resource manager `[[mgr+0x28]+0x160]`
   (`GXFfxGraphicsResourceManager`), then calls `FUN_1402B7CF0`.
3. The FFX library builds the resource (`FUN_142157260`/`FUN_142155F90`, then
   `FUN_142159090`).
4. For each action node, `FUN_141AB36C0` runs:
   - the registry must be sealed;
   - the record for the node id must exist;
   - the resolver writes the GX index into `node+4`;
   - the getter returns the GX entry;
   - the entry's validator runs (`14024A9E0` for 0x261 checks the node's counts);
   - the entry's `+0x10` runs.
   Any failure returns 0, which drops the node from the resource.

### 3. Instance time (open)

- No static reference calls a GX entry's `+0x30` (the host factory).
- No code reads `[1449651D0]` outside registration, lookup, seal and destruction.
- The factory is reached from the FFX middleware through the resolved node, by an indirect
  call that Ghidra does not see.
- Once called, the factory chain is CPU only (section 10).

### Gates that a native run can lose (INFERRED; read them at runtime)

| Gate | Read | Original expectation |
|---|---|---|
| Scene control constructed with its GPU buffers (`FUN_1419254F0` on the GX device) | `GXFfxSceneCtrl` exists; `this+0x44 == 0` at construction | yes |
| GX descriptor registry filled and sealed | `[1449651D8]` > 0 with an id 0x261 entry; `DAT_1449651E8 == 1` | yes |
| FFX type registry sealed | byte `[144933088+0x20] == 1`; count `[144933098]` | yes |
| FFX manager and resource manager present | `[[144799C98]+0x58]`; `[[that+0x28]+0x160]` | non-null |
| FXR resources resolved | calls to `FUN_14024ECF0`/`FUN_141AB36C0` during map load | many |
| Hosts created | calls to `FUN_14028F200` / `FUN_1402B2410` | about 104 lights |

One original-game break on `FUN_14028F200` gives the return address, and so the
instance-time call site, in a single run.

## 15. Lifetime of an offscreen character view, and the Native1164 shutdown crash path (STATIC)

### What `FUN_140D19000`/`FUN_140D126B0` read (the pilot's crash stack D12759 ← D1904E ← D1AA60 ← D1B190)

The crash is on the **animation (FD4 location) side**, not in the offscreen scene graph.

- **The functions involved:**
  - `FUN_140D126B0` is slot 7 of `CSFD4LocationBodyScaleModifier`. It reads
    `[[this+0x58]+0x18][bone*4]`, the per-bone byte table of its linked skeleton, and crashes
    there. It maps the bone through `[this+0x68]` (`FUN_140D3BCF0`).
  - `FUN_140D19000` is slot 7 of `SprjFD4LocationModelSpaceToWorldSpaceModifier`. Its
    `+0x38` and `+0x58` are its two link nodes (`FUN_140D1A830`, `FUN_140D1A7D0`): the
    source and the target location, that is the model and the skeleton.
  - `FUN_140D1B190` (reached through `FUN_140D1C820`/`FUN_140D1C870`) is the global FD4
    location manager's update, `[144791BD0]`. It walks the registered locations (`+8` list,
    flag `+0x10` bit 0, dirty count `+0x14`) and updates each one through `FUN_140D1AA60`
    (`vt+0x10`, then `vt+0x18`). It then runs the `+0x48` list (`vt+0x40` with
    `[RendManImp+0x30]`).
  - The manager update is skipped while `[144793348]+0x60` exists and `FUN_140D6E220()` is
    true.
- **Consequence:** replacing `FUN_140D64C30` (`holder+0x20..+0x50`) does not touch these
  objects. The crash is a location modifier that is still registered while the skeleton or
  model it links to has been freed.

### The view's model objects (CSMenu*ModelRend fields)

| Field | Object | Created by |
|---|---|---|
| `+0x6E0` | character model resource `c0000` | `FUN_140C4BD30`, in `FUN_140D690A0` (STEP_Wait_Setup) |
| `+0x810` | hkx skeleton instance | `FUN_14081C6A0`, set up by `FUN_14081C4B0` |
| `+0x818` | skeleton-to-model mapping | `FUN_140D3B0A0` |
| `+0x820` | `SprjFD4LocationAnimSkeletonToMdlObjIdxModifier` | `FUN_140D15E20` links the model location (`FUN_140C4D390(+0x6E0)`, node `+0x38`) and the skeleton (`FUN_14081E100(+0x810)`, node `+0x58`) |
| `+0x870` | a second location modifier | |
| `+0x6E8` | the assembled model | posed every frame |
| `+0x6D8` | `ChrAsmModelRes` | `FUN_140C42230` |
| `+0x6D0` | `[obj+0x6D0]` | released by `FUN_140D0C990`/`FUN_140D0C2E0` |
| `+0xB8` | the `CSEzOffscreenRend` holder | section 12 |

### Per frame

- **Step machine.** The `CSMenuAsmModelRend` step machine (slot 9 `FUN_140D66B80`) runs. In
  `STEP_Wait_Play` (`FUN_140D67940`) it:
  - starts or stops listing the scene (`FUN_140D64B50`/`FUN_140D65140`);
  - when the appearance changed, rebuilds the appearance (`FUN_140593F10`, `FUN_1405945C0`);
  - poses the model: `FUN_1408816D0(+0x844)`, `FUN_140C3B460(+0x6E8)`;
  - loads the parts: `FUN_140C42620(+0x6D8, …)`;
  - rebuilds the model: `FUN_140C3BA60(+0x6E8, …, +0x6D8)`.
- **Owner update.** The owner's per-frame visit (Prof: `TitleTopDialog`, `FUN_140ADE0B0`)
  calls `FUN_140D66900`. It only clears the holder's scene output texture
  (`[holder+0x48]` `vt+0x18`, then `+0x1E0`) to (0, 0, 0, 1).
- **Location manager.** Independently, `[144791BD0]` updates every registered location
  modifier, including those of these models.

### Destruction (STATIC order)

1. The owner releases the view: `FUN_140AED060`, then `FUN_1408190B0([14477D778], obj)`.
   - It sets the finish request `obj+0x6CA = 1` (`FUN_140D67580`).
   - It queues the object on the deferred list of `[144964930]` (`FUN_1423559C0`, which
     calls `FUN_140309300` with kind 6).
   - So the object is deleted later, not immediately (INFERRED: by that task system).
2. The base destructor `FUN_140D66280` runs:
   1. `FUN_140D68EB0` frees in this order: `+0x6E8` model, `+0x6F0`, `+0x870` (unregister
      `FUN_140D1A770`, then release), `+0x820` (unregister `FUN_140D1A770`, then release),
      `+0x818`, `+0x810` skeleton, `+0x6E0` model resource.
   2. It frees `+0x6D8` `ChrAsmModelRes`, then `+0x6D0`.
   3. `FUN_140D65140`(holder) removes the scene from the offscreen list. Then
      `FUN_140D65160`(holder) runs.
   4. It deletes the holder (`FUN_140D64710`):
      - if it is still listed (`+0x58`), it is removed from `[holder+0x18]` with
        `FUN_1400C02D0`;
      - it unregisters the named texture (`FUN_140D4F4B0([144792F58], holder+0x10)`);
      - it releases the proxy `+8` and calls `FUN_140D65970`;
      - it releases `+0x48` (composite scene), `+0x40`, `+0x38`, then `+8` (all
        reference-counted). Correction (section 28): `FUN_140D65160` has already released
        and nulled `+0x38/+0x40/+0x48`, after queuing the composite for deferred deletion.
   5. It tears down the appearance and step members.

### What a native minimal owner must guarantee (INFERRED)

- The view's own destructor already unregisters its two location modifiers before freeing the
  skeleton and the model. A crash in the manager update therefore means one of two things:
  - a modifier was freed or invalidated without `FUN_140D1A770`. That is the case when the
    object, or its `ChrAsmModelRes` parts, are destroyed outside `FUN_140D66280`, or after
    the skeleton was freed by another path;
  - or the location manager keeps updating during shutdown after the deferred-deletion queue
    `[144964930]` has dropped the object without running its destructor.
- A minimal native owner should:
  1. keep the step machine running until `STEP_Finish_Play` and its release
     (`FUN_140D68EB0`) completes;
  2. let the deferred queue run the real destructor;
  3. never free `holder+0x20..+0x50` before `FUN_140D64710`. `FUN_140D64710` releases
     `+0x38/+0x40/+0x48` by reference count and removes `+0x48` from the RendManImp list
     only if `+0x58` is set; `+0x20`/`+0x28`/`+0x30`/`+0x50` are left to `FUN_140D65970`.
     Native replacements must therefore be reference-counted objects with a valid `vt+0` and
     `+8` count, or the holder's release must be bypassed for them.
- **Open:** the reader of the RendManImp offscreen list (per-frame scene execution) was not
  located.

## 16. Who draws the offscreen list each frame (STATIC)

- **The list.**
  - `[RendManImp+8]` is a plain reference-counted vector of composite scenes: begin, end,
    capacity, allocator (`FUN_140204B10`). Its pointer is also kept in the global
    `[144964A00]`.
  - `FUN_1400C0230(list, scene)` adds a scene; `FUN_1400C02D0` removes it.
  - **The main scene is in the same list**: `StageRend` (`FUN_140DFEEB0`, created by
    `FUN_140DFDAB0` as `[RendManImp+0x10]`) adds its `GXSgCompositeScene` there. The
    character views add theirs on "start rendering" (`FUN_140D64B50`).
- **The reader.** It is the RendManImp draw `FUN_140DFD270`, called from the `SprjDrawStep`
  draw `FUN_140D55190` (K-0106) once per frame. In order:
  1. It sets `[[144961AB0]+0x20]` to the frame context (`FUN_14000A1F0`), then prepares the
     stage (`FUN_140E00270`) and `SprjGraphicsImp` (`FUN_140D57890`).
  2. **`FUN_1400BFEF0([RendManImp+8], context)` draws every listed scene**, then calls
     `FUN_140D578C0`.
  3. The screen-capture hooks run (`FUN_140D6E270(cap, 0/1/2)`), interleaved with step 4.
  4. The 2D layers `+0x30`, `+0x38`, `+0x40`, `+0x48` and `+0x50..+0x68` are rendered
     through `FUN_1423306F0` (Scaleform/HUD, INFERRED). Before `+0x30`, the stage hook
     `FUN_140E00680` runs.
- **`FUN_1400BFEF0`:**
  1. It builds pairs `{key = -1 - [scene+0x10], scene}` and sorts them stably ascending
     (`FUN_1400C0920`).
     - That is a descending priority `[scene+0x10]`. The views set 100 (`FUN_1400C1C60(scene, 100)`
       in `FUN_140D64C30`); the main scene keeps its constructor value (INFERRED 0).
     - **So the character views are drawn before the main scene.** Ties keep insertion order.
  2. Pass 1: for each scene, `scene->vt+0x18()`, then `->vt+8()` on the returned object
     (target and pipeline preparation).
  3. Pass 2: for each scene, **`scene->vt+0x38(context, index)`**. For
     `GXSgCompositeScene` this is slot 7 `FUN_1402046B0`, the same draw entry as the main
     scene:
     - it gets its pipeline (`vt+0x68`);
     - it prepares the view (`vt+0x50`, with its fields `+0x38` (attached scene), `+0x48` and
       `+0xE0`, roles of the last two not read);
     - it records the pipeline (`vt+0x60`);
     - it lets each sub-item in `+0x18..+0x20` add work (`vt+0x40`).
  4. It submits the recorded lists: `FUN_14000A210([144961AB0], [GX+0x10 or +0x70], count << 29)`.
- **View and target.**
  - Each view is drawn with its own camera holder (`holder+0x20`/`+0x28` views, attached
    with `vt+0x28`), into its own pipeline targets (`vt+0x18`, then `+0x1E0`), within the
    same frame context and command submission as the main scene.
  - Nothing distinguishes the two in this loop except priority and object.

## 17. How a menu view's character is attached to its own scene, what it draws, and what differs from the world (STATIC)

### Attachment

1. In `STEP_Wait_Setup` (`FUN_140D67D50`), after the parts load (`FUN_140C42620(+0x6D8)`) and the
   model build (`FUN_140D690A0`, `FUN_140C3BA60`), the step calls
   **`FUN_140C3AD90(model +0x6E8, [holder+0x50])`**.
2. `FUN_140C3AD90` stores the context at `model+0x10`. For each of the model's 24 part slots
   (`+0x28..+0xE8`) it calls `FUN_140C4C3C0(part, context)`. That call moves the part's model
   instance (`[part+0x18]`):
   - out of its previous context (`[instance+0x70]`, `FUN_140D03BC0`, layer `vt+0x48`);
   - into the new context's layer: `FUN_140D03BA0(context)`, then layer `vt+0x40` (add), then
     `FUN_140D03BB0`;
   - and sets `instance+0x70 = context`.
3. `holder+0x50` is that **scene registration context** (`FUN_140D039E0`, 0x40 bytes). Its
   `+0` is the layer `holder+0x40` (`GXSgLayerFlat`, set by `FUN_140D03AC0`). It holds eight
   floats from `.data 14478CC68..87`, which are zero in the image.
4. The layer is attached to the general scene `holder+0x38` (`FUN_1400C1E30`), and that scene
   to the composite scene `holder+0x48` (`vt+8`). So the parts are drawn only by the view's
   composite scene.
5. **The world uses the same mechanism:**
   - `StageRend`'s scene builds the same context type (`FUN_140D5CE50`, called from `FUN_140DFEEB0`);
   - in-game character models use `FUN_140C3AD90` too (`FUN_140CC2AD0`).

### Draw entities

- The model is a full character assembly: model resource `c0000`, hkx skeleton, and the
  `ChrAsmModelRes` parts built from the save's appearance and equipment.
- Its parts are the same FLVER parts and materials as the player's in-game body and armour.
- They are drawn by the same composite pipeline type (section 11). **Material families are
  therefore the same as for the player in game.**

### What differs from the world

- **Scene parameters (gparam).** `FUN_140D64C30` calls `FUN_140088AF0(pipeline, {FUN_140D27770(1), row+0x2C})`.
  - `FUN_140D27770(1)` = `.rdata 14294EFB0[1]` = **area 1**; `row+0x2C` is the
    `MenuOffscrRendParam` field.
  - `FUN_140088AF0` writes this one key, with blend weight 0, into the scene key pair
    (`+0x3FC0..+0x3FCC`) and into **all 13 graphics-parameter bank pairs** (A at
    `+0x3FE0..`, B at `+0x4048..`, weights `+0x40A8..`). These are the banks `FUN_1400FB890`
    interpolates for tone map, glare, light set, fog, env map and the others (section 1 of
    TONEMAP-SKIN).
  - **So a view lights, fogs, applies IBL and tone-maps from the area-1 gparam entry chosen by
    its `MenuOffscrRendParam` row**, not from the current map.
- **Camera.** `MenuOffscrRendParam` only adds the camera (section 11).
- **Pipeline stages.** The view's pipeline flags `0x8422014` against the world's (section 12):
  no YEBIS tone map stage (the GX tone map filter instead, I-0113 RESULT « Tranche 27 »), no depth downsample, no half-resolution stage, and
  without the stages of bits 20 and 40.
  - Whether fog and shadows run, and with which values, follows from those stages and the
    area-1 gparam entry. Which stage builds shadows was not mapped (INFERRED: the cascade
    atlas is sized by `2 * [GXRS+0x4E90]` or 0x400 in `FUN_1400C3410`, so shadows exist).
- **Map content.** The view's layer holds only the character: no map geometry, map lights or
  env map regions are attached.
  - The env map, light-set and fog entries still resolve through the same banks (sections 10
    and 11 of TONEMAP-SKIN) under area 1.

## 18. The menu model's classes and drawables, for Native1166 (STATIC + one encrypted gap)

### The two objects the pilot saw

- **`+0x6E8` (vtable `142924A20`) is the `ChrAsmModel`.**
  - Its vtable has no RTTI locator (the qword before it is code, `140C33F90`).
  - It sits right after `DLRuntimeClassImpl<ChrAsmModel>` (`1429249D8`) and next to
    `ChrAsmModel::Util_ApplyChrMdlMtxAndBodyScale::InfoImpl` (`142924A48`).
  - Its methods are the `FUN_140C3xxxx` the view calls: `C3AD90` (attach), `C3BA60` (part
    rebuild), `C3B460` (pose) and `C3B810` (`model+0x318 = holder`).
- **Its `+8` object (vtable `1429263E8`).** That address lies 0x10 inside the
  `NS_SPRJ::SprjModel` vtable (`1429263D8`; locator at `1429263D0`). INFERRED: a
  SprjModel-family object; not proved.

### The menu model does not use `SprjAsmModelDrawEntity`

- `SprjAsmModelDrawEntity` (vtable `142947DB8`) is built by `FUN_140CFF200` and `FUN_140CFF810`.
  - `FUN_140CFF200` is reached only from the in-game character chain `FUN_140C44D20`, then
    `FUN_140C32190`, then `FUN_1408BE6C0`, then `FUN_1408CAA50`.
  - `FUN_140CFF810` is reached from `FUN_140CFFB40`.
- The menu `ChrAsmModel` builds its parts with **`FUN_140C3CA10(model, slot, appearance, res)`**,
  called from `FUN_140C3BA60` for slots 0..0x17. For each slot it:
  1. takes the part resource's model data `[res+0x80+i*8]+0x60`, or creates a fresh
     `NS_SPRJ::SprjFD4FlverModelData` (0x470 bytes, `FUN_140D35AF0`);
  2. allocates a **0x148-byte part** (`FUN_140C3DB40`, encrypted at rest, RVA `0xC3DB40`);
  3. stores it at `model+0x28+slot*8`.
- **The drawable is `[part+0x18]`.** It is what `FUN_140C4C3C0` registers. Its class is set by
  the encrypted part constructor; its loaded bytes would name it.

### Where to take them for drawing

- `FUN_140C3AD90(model, [holder+0x50])` calls `FUN_140C4C3C0(part, ctx)` for each non-null
  part slot.
- That call adds `[part+0x18]` to the context's layer: `[ctx+0]` = **`holder+0x40`**
  (`GXSgLayerFlat`), through its `vt+0x40` (slot 8, `FUN_1400C2700`). It also sets
  `[part+0x18]+0x70 = ctx`.
- **So the view's drawables are the items of `holder+0x40`**, the layer's item vector at
  `+0x10..+0x18`. Each one is drawn through its interface at `+0x10` when the layer is
  traversed (`FUN_1400C2630`, the mask test on `item+0x20` against the context).
- They also stay reachable from the model's part slots `+0x28..+0xE8` (`+0x18` of each part).

## 19. MenuOffscrRendParam rows: concrete camera and gparam values (STATIC, game data)

### Source

- `Get-ParamRows.ps1` (read-only) decrypts the regulation in memory with the public key, as
  I-0066 `Map-ArmourItems.ps1` does, and dumps one param's rows without a paramdef.
- The full dump of `MenuOffscrRendParam` is in `evidence/I-0067/queries/menuoffscrrendparam.tsv`.
  - It comes from `Data0.bdt` with SHA-256 `955016AB…`: 11 rows of 0x40 bytes, ids 0, 1, 2,
    3, 10..15 and 20.

### Field roles

The roles come from the readers `FUN_140D6B8D0` (Prof), `FUN_140D698C0` (Face) and the
`FUN_140D6A3B0` loop (Player).

| Row offset | Use |
|---|---|
| `+0x00..+0x08` | target point (x, y, z; `w = 1`) |
| `+0x0C` | distance |
| `+0x10`, `+0x14` | two angles in degrees (stored at `+0x8A4`, `+0x8A0`) |
| `+0x18` | field of view in degrees (stored at `+0x900` in radians) |
| `+0x1C..+0x28` | row 0 only: limits (2.2, 2.8, −89, 89; INFERRED zoom and pitch range for character creation) |
| `+0x2C` | the gparam id under area 1 (section 17) |

The fov is checked against the pilot's Native1166 reads: Status 0.698 rad = 40°, and the
512x256 view 0.1745 rad = 10°.

### Which row each view uses (size tables, section 12)

| View | Row(s) | Target (x, y, z) | Distance | Angles | Fov | gparam id |
|---|---|---|---|---|---|---|
| `SYSTEX_Menu_StatusBody` (560x768; Player row 4: rows `+0x14` = 2 and `+0x18` = 2) | 2 | (0, 0.85, 0) | 2.66 | 17.189, −8.594 | 40° | 11 |
| `SYSTEX_Menu_RankingBody` (Player row 5) | 3 | same as 2 | 2.66 | 17.189, −8.594 | 40° | 11 |
| `ChrMakePreview` / `NudePreview` / `Preview2` / `NudePreview2` (784²) | 0 (and 1 as the second preset) | row 0: (0, 0.85, 0); row 1: (0, 1.55, −0.09) | 2.66 / 1.2 | 17.189, −8.594 / 10.313, −13.751 | 40° / 16.5° | 11 / 10 |
| `SYSTEX_Menu_ChrMakeGrid00..14` (128²) | 10 | (0, 1.58, 0) | 1.2 | 1.719, −14.85 | 15° | 10 |
| `SYSTEX_Menu_KickFace00..02` (128²) | 11 | (−0.015, 1.54, 0) | 1.1 | 1.719, −14.85 | 15° | 15 |
| `SYSTEX_Menu_StatusFace` (512x256, face row 18) | 12 | (0, 1.56, −0.07) | 1.48 | 2.6, −23 | 10° | 13 |
| `SYSTEX_Menu_StatusFace` (face row 19) | 15 | (0, 1.59, 0) | 1.39 | 2.6, −23.34 | 12° | 12 |
| `SYSTEX_Menu_RankingFace` (512x256) | 13 | (0, 1.58, 0) | 1.2 | 1.719, −14.85 | 15° | 10 |
| `SYSTEX_Menu_SignFace` (128²) | 14 | (0, 1.58, 0) | 1.1 | 1.719, −14.85 | 15° | 15 |
| `SYSTEX_Menu_Profile00..09` (Load Game, 128²) | 20 | (−0.017, 1.55, 0) | 1.6 | 1.719, −13.751 | 8° | 14 |

The angle order (pitch or yaw first) and the exact eye construction are in `FUN_140D6BC20`
(Prof) and `FUN_140D69EE0` (Face), not re-derived here.

### Shadow stage of the views (open)

- The pipeline instance sizes its shadow atlas at `2 * [GXRS+0x4E90]`, or 0x400 when that is
  0 (`FUN_1400C3410`).
- The shadow-casting light is INFERRED to be the directional light of the area-1 light set
  selected by the row's gparam id. The scene keys of section 17 make every bank, the light
  set included, resolve under {1, id}.
- The shadow pass list and the cascade count were not mapped. A capture of an original frame
  with the Status screen open would give them directly.

## 20. Native1167: the vtable `0x295B690` item is not a character part (STATIC)

### What the pilot observed

- **`14295B690` is `NS_SPRJ::CSSgEzDraw`.**
  - Its secondary vtables are `14295B6C0` (`+0x10`, the layer-item interface) and `14295B6D0`
    (`+0x70`).
  - Its constructor `FUN_140D63530` is called **only** from the RendManImp constructor
    `FUN_140DFC850`, which stores it at `RendManImp+0x28`. The same constructor stores a
    `GXSgLayerFlat` at `RendManImp+0x20`.
  - Its draw interface (`+0x70`, slot 0 `140D63750`) calls
    `FUN_1423306F0([this+8], ctx+0x460)`, the same 2D renderer as the HUD layers of
    section 16. `[this+0x78]` is a 0x60-byte helper (`FUN_140D63780`).
- **The insertion at return address `0xDFDB28`** is in `FUN_140DFDAB0`, when `StageRend` is
  created:
  - it attaches `RendManImp+0x20` to `StageRend`'s scene (`FUN_1400C1E30`);
  - then it adds `RendManImp+0x28` (the `CSSgEzDraw`) to that layer through `vt+0x10`
    (`GXSgLayerFlat` slot 2, `FUN_1400C24E0`).
  - **This is the main stage's 2D/Ez draw item, not a menu character part.**

### Menu part insertions

- They come from `FUN_140C4C3C0`, through the layer's `vt+0x40`. `GXSgLayerFlat` slot 8
  (`1400C2700`) only jumps to slot 2 (`C24E0`). Their return address is therefore inside
  `FUN_140C4C3C0`.
- Their items are `[part+0x18]`, of a class set by the encrypted part constructor
  `FUN_140C3DB40`.
- What is known of `[part+0x18]` (STATIC):
  - `+0x70` is its context;
  - `+0x2B0/+0x2B8` hold a value derived from the parts resource (`FUN_140C4D160`, with
    "dirty" `+0x371 = 1`; INFERRED a material or colour pair from `FUN_1405B0800`);
  - the model data comes from the part resource `[res+0x80+i*8]+0x60` (a
    `SprjFD4FlverModelData`).

### Pose

- The bone source is the hkx skeleton instance `obj+0x810` (`FUN_14081C6A0` on the `c0000`
  resource, started with `FUN_14081C4B0(skel, 0, 0, 0)`). `STEP_Init_Play` (`FUN_140D677D0`)
  also starts the model animation (`FUN_140C3C1A0(model, …)`, `FUN_140C3B450(model, -1)`).
- It is mapped onto the model objects by `SprjFD4LocationAnimSkeletonToMdlObjIdxModifier`
  (`obj+0x820`, `FUN_140D15E20`). That modifier is updated by the global FD4 location manager
  `[144791BD0]` (section 15).
- `FUN_140C3B460` applies appearance colours (`FUN_140C3B870`, slots 0..), not the pose.
- INFERRED: the bone palette of the parts is the location manager's output for this
  modifier.

### Loaded bytes that would close the gap

| RVA | Function |
|---|---|
| `0xC3DB40` | part constructor: class and layout of `[part+0x18]` |

## 21. Menu view camera: the exact eye and view construction (STATIC)

`FUN_140D6BC20` (Prof) and `FUN_140D69EE0` (Face) contain identical code. They read the values
the param reader stored:

- `t = (+0x88C, +0x890, +0x894)`: the row's target;
- `d = +0x89C`: the distance;
- `a = +0x8A0`: row `+0x14` in radians;
- `b = +0x8A4`: row `+0x10` in radians.

The degree-to-radian factor is `.rdata 143D7ED40` = 0.0174533. `FUN_141FD1740` is cos and
`FUN_141FD1A40` is sin, checked on the Y-rotation rows `{cos a, 0, −sin a}`, `{0, 1, 0}`,
`{sin a, 0, cos a}` built from the constants `143D1C480`/`143D1BF90`.

```
Z   = (sin a · cos b,  −sin b,  cos a · cos b)        // third row of Rx(b)·Ry(a), row vectors
eye = t − d · Z
f   = normalize(t − eye)                              // = Z
r   = normalize(cross((0,1,0), f)) = normalize((f.z, 0, −f.x))
u   = normalize(cross(f, r))
W   = rows { (r,0), (u,0), (f,0), (eye,1) }           // camera world matrix, left-handed, +Z forward
```

- The degenerate case of each normalisation, a zero length, falls back to the constants
  `143D1C160..17C`.
- The caller stores `inverse(W)` (`FUN_140004500`) at `obj+0x8C0..+0x8FF`. That is the
  **view matrix**.
- It writes the view, through `FUN_1404095F0`, into the camera holder `[obj+0xB8]+0x30`
  (`+0x10..+0x4F`). The projection parameters `+0x50..+0x5C` are taken from
  `obj+0x900..+0x90C`:
  - fov in radians, from the row `+0x18`;
  - aspect `w/h` (Prof/Face `+0x904`; the Player view stores its aspect at `+0x8F4`);
  - near and far. The pilot measured 0.08 and 10000.
- The same view is copied into the two views of `holder+0x20`.

### Worked example: Status (row 2)

- `a = −8.594°`, `b = 17.189°`, `d = 2.66`, `t = (0, 0.85, 0)`.
- `Z = (−0.1427, −0.2955, 0.9445)`.
- `eye = (0.380, 1.636, −2.512)`.
- The camera stands in front of and above the character and looks down at the target.

## 22. Shadow of the menu views (STATIC + INFERRED where marked)

### The key

`FUN_140088AF0(pipeline, {1, id})` (section 17) also calls **`FUN_1400888B0(pipeline, key)`**.
It sets the **ShadowParam keys** of the pipeline:

- `A = +0x3FA8/+0x3FAC` and `B = +0x3FB0/+0x3FB4` both get `{1, id}`, with weight
  `+0x3FB8 = 0`;
- when the key changed (and `+0x3EBD` is set), it clears four cascade cache flags
  `+0x2D7D..+0x2D80`.

So a view's shadow settings come from the ShadowParam entry of area 1 with the row's gparam id
(section 19: 11 for Status, 14 for the portraits, …). That is the same id as its light set,
fog and tone map.

### The settings

`FUN_140088A10(pipeline)` blends the ShadowParam bank `[GXRS+0x4670]` records of those keys
(`FUN_14018C590`, `FUN_1401DAF40`) into `pipeline+0x90`.

`FUN_1401B5720(state, record)`, called from `FUN_1400F9B50` and `FUN_1400F90F0` (the per-scene
shadow preparation), builds the cascaded-shadow state from that record. The record layout is
the one the pilot's `Banks::Shadow` already uses:

| Record offset | Field |
|---|---|
| `+0x34` | Near (stored at `state+0xA70`) |
| `+0x38` | Far Fade Start |
| `+0x3C` | Far Fade Dist (stored at `state+0xA78`) |
| `+0x40/+0x44/+0x48` | three cascade split distances, clamped increasing (so **four cascades**, matching the four cache flags) |
| `+0x50` | FilterMode |
| `+0x54` | Depth Offset |
| `+0x70/+0x71` | CullFlip / AlignTexel |
| `+0x84` | Blur |
| `+0x8C` | Shadow Color |
| `+0x90` | Resolution |

### The light

INFERRED: the casting light is the directional (sun) light of the area-1 light set with the
same id. `FUN_1400F90F0` reads the light-set record of the scene key (`FUN_140032770([GXRS+0x41E0], key)`,
`+0x40..+0x9F`) next to its call to `FUN_1401B5720`. These are the pilot's `Banks::Sun` fields.

### The pass

INFERRED: it is the same cascaded shadow atlas pass as the world (K-0407: first pass of the
frame), run by the view's own pipeline instance.

- The atlas size is `2 * [GXRS+0x4E90]`, or 0x400 when that is 0 (`FUN_1400C3410`).
- The casters are the items of the view's layer, that is the character.

## 23. Where phantom programmes sit in the shader set, and the search for their reader (STATIC; selection site still open)

### The shader set

The shader set is `GXSR::GXFlverShaderGroup` (vtable `143D48C18`, constructor `FUN_1400A9BA0`,
0x3F00 bytes). Its layout:

| Offset | Content |
|---|---|
| `+0x18..+0x1F7` | a per-pass table, 20 passes × 3 variants (normal, `[A]`, `[AA]`) |
| `+0x1F8..+0x33F7` | the main programme table |
| `+0x33F8..`, `+0x3718..`, `+0x3E98..` | further tables |
| `+0x3EB8` | the shader name |
| `+0x3EF8` | a flag, returned by slot 7 `FUN_1400A9AF0` |

- `FUN_1400AA630`, the per-pass loader, writes `+0x18 + pass*0x18`.
- **`FUN_1400A9EF0` overwrites passes 0 (`Fwd`) and 4 (`FwdSdw`) with the `<shader>_Phantom_<pass>`
  programmes** (section 13).
- The release loop (`1400AB400`) walks passes 0..19 and skips 0x11..0x13.

### Programme getter

Slot 2, `FUN_1400AA280(out, a, pass ≤ 0x13, b, …, alphaVariant, flag9)`, reads the **main** table:

```
index = (((flag9 != 0) + (a + pass*4) * 2) * 5 + b) * 2 + (alphaVariant == 0)
```

It falls back to the other alpha variant when the slot is empty. Companion tables
`+0x33F8`/`+0x3718` are read for `b > 0`. Slot 4 `FUN_1400AB310` returns the same entry's
`vt+0x18`/`vt+0x20` objects, and slot 5 (`1400AB3A0`) returns the entry itself.

### Reader of the phantom table: not found statically

- A scan of every instruction of the image for `[reg + reg*8 + 0x18]` (276 matches,
  `g:` query) and `[reg + reg*8 + 0x78]` finds only the two writers (`1400AA630`, `1400AB13B`).
  The other matches are other objects (stride 0x50 tables at `+0x510`).
- The feature-name table `{GXFT_ValidSubPhantom, 0x11}` (`.rdata 143D21040`) has no code
  reference; it is a name table, not a code switch.
- INFERRED: the forward recorder reads the phantom entries through fixed offsets
  (`+0x18/+0x20/+0x28`, `+0x78..`) on a per-character condition.

### How to close it

At runtime, put a read breakpoint on a phantom character's material shader set at `+0x18..+0x28`
during the forward pass. A summoned phantom works, as does a replay or bloodstain ghost. The
hit gives the selection site and its condition in one test.

## 24. Status view: which layer items a view draws, and why it renders once (STATIC; two points INFERRED)

### Layer traversal mask

`GXSgLayerFlat` slot 1 `FUN_1400C2630(layer, traverser)` draws an item only if both halves of
`item+0x20` share a bit with the traverser mask `traverser+8`:
`(item.lo & t.lo) != 0 && (item.hi & t.hi) != 0`. The layer's own mask `+0x30/+0x34` is the OR
of its items' masks, added on insertion (slot 5 `1400C26D0`, called by add `FUN_1400C24E0`).

**Traverser mask of a view (`t`)**, built per pass by `GXSgDrawScene` slot 0 `FUN_1400C2EC0`:

- `t.hi` = `[composite+0xE4]`;
- `t.lo` = (`[composite+0xE0]` & `0xFFFFFF00`) | pass bit.
- The composite field comes from the draw context that `GXSgCompositeScene` slot 7 (`FUN_1402046B0`)
  hands to each sub-scene: `{+0: composite+0x38, +8: composite+0x48, +0x10: index, +0x14: composite+0xE0}`,
  stored by `GXSgBaseSceneImpl` slot 4 (`FUN_1402059F0`).
- `composite+0xE0` defaults to `{1, 1}` (constructor `FUN_1402043A0`). Composite slot 6
  (`1402046A0`) sets it. **`FUN_140D64C30` (view holder setup) calls it with `0x200000000`:
  a character view traverses with `t.hi = 2`.**
- Pass bit, from the pass id at `[pass+0x124]`:

| Pass id | Bit |
|---|---|
| 1..4 | `0x40` |
| 5..8 | `0x02` |
| 9..0x38 | `0x04` |
| 0x39 | `0x10` |
| 0x46 | `0x20` |
| 0x3A, 0x3B..0x40, any other | `0x01` |

  Which pass ids are the shadow cascades is not established. INFERRED: 5..8, the only group
  of four that menu parts enter.

### Item mask (`item+0x20`)

- `GXSgEntity` constructor `FUN_1400C9470`: `{lo 1, hi 1}`.
- Setter `FUN_140060190(entity, mask64)`. It writes `+0x20`; if the value changed and the entity
  is in a layer, it ORs the mask into the layer (layer `vt+0x28`).
- World model entities (`SprjModelDrawEntity`, `SprjAsmModelDrawEntity`: `FUN_140D05C80`,
  `FUN_140CFF200`, slot 20 `FUN_140D009E0`, tail calls `140D01312`, `140D0743C`, `140D07491`)
  always pass `hi = 1`.
- **Menu character parts are `SprjModelDispEntity`** (constructor `FUN_140D03EB0`; it has no
  plain-code caller, INFERRED reached from the encrypted part constructor `0xC3DB40`). Their
  mask comes from the part seed:
  - `lo` starts at 1; seed byte 0 bits change it: `0x02` → `|2`, `0x04` → `|4`, `0x10` → `&6`,
    `0x40` → `|0x10`, `0x80` → `&0x16`;
  - `hi` = seed `+8`.
- The part seed is chosen by `FUN_140C3C3B0(model, seed, slot)`:
  - model flag `0x10` selects preset 4;
  - flags `2|4|8` select preset 5;
  - otherwise preset 3.
- Presets are filled at startup by `FUN_140C4D6F0..8B0`:

| Preset | Byte 0 | `+8` (hi) | Entity mask |
|---|---|---|---|
| 3 (world character) | `0x4E` | 1 | `{0x17, 1}` |
| 4 (menu) | `0x0E` | **2** | **`{7, 2}`** |
| 5 | `0x00` | 1 | `{1, 1}` |

- The menu model gets flag `0x10` from `FUN_140C3B7A0(model, 1)` in `FUN_140D690A0`
  (`STEP_Wait_Setup`).
- **So a view draws exactly the items whose `hi` has bit 2:**
  - its parts, in passes with bits 1, 2 or 4;
  - not world-seeded or default-mask entities (`hi = 1`), which a view always skips.
- A traversal with `t.hi = 1` does the opposite: it skips every menu part and keeps only
  `hi & 1` items.
- The per-mesh mask `+0x2B0` (`FUN_140C4D160`, equipment hide bits) is separate. It does not
  affect this test.

### Why the Status view renders once

- **The list does not drop scenes.** `FUN_1400BFEF0` draws every listed composite scene every
  frame and never removes one. A view is drawn as long as `FUN_140D64B50` has listed it and
  `FUN_140D65140` has not removed it. Priority (100) only orders views before the main scene.
- **The Status request asks for one frame.** `FUN_140B72980` (called by the PlayerStatusMenu
  constructor `FUN_140B71320` and by `FUN_140AB7280`):
  1. sets the play request `+0x6C8 = 1` (`FUN_140D67540`);
  2. calls `FUN_140D67560`, which sets **`+0x6C9 = 1` unless `FUN_14075EBA0()` is true**;
  3. sets the start delay `+0x878 = 2` (`FUN_140D668A0`).
- **Step sequence** (indices from `FUN_14268AB80`: 5 `Init_Play`, 6 `Wait_Play`, 7 `Finish_Play`,
  8 `Finish`; `vt+0x20` sets the step, `vt+0x30` sets `+0x50` to run the next step in the same
  tick, `FUN_140D683A0`/`140D680D0`):
  1. `Init_Play` (`FUN_140D677D0`), each frame once the parts are loaded:
     - it advances the animation (`FUN_140C3C1A0`, `FUN_140C3B450(-1)`);
     - while `+0x878 >= 1` it only decrements the counter;
     - on the third ready frame it lists the scene (`FUN_140D64B50`) and runs `Wait_Play`
       in the same tick.
  2. `Wait_Play` (`FUN_140D67940`) tests, in order:
     - `+0x6CA` (finish request): step 7;
     - `+0x6C9`: clear it, step 7, without running it this tick;
     - `+0x6C8`: step 1 (new request);
     - otherwise the continuous branch. It re-lists the scene if `+0x58 == 0`, unless
       `+0x6CC != 0` (the only write found in `140B00000..140E00000` is the constructor's, to 0). It also re-poses and
       updates the model each frame.
  3. `Finish_Play` (`FUN_140D676D0`):
     - on its first tick it removes the scene (`FUN_140D65140`);
     - five ticks later it releases the model (`FUN_140D68EB0`) and moves to `Finish`.
  4. `Finish` (`FUN_140D67690`) waits for the next request.
- With `+0x6C9` set, **the scene is in the list for exactly one RendManImp draw.** The
  composite's output texture, which the named texture proxy points at, keeps that image while
  the menu samples it.
- A new request repeats the cycle with a new snapshot: `FUN_140B72980` runs again, INFERRED on
  an equipment or appearance change.
- **`FUN_14075EBA0` is encrypted at rest** (RVA `0x75EBA0`). Its other callers
  (`FUN_140D69EB0`, `FUN_140D6B180`, `FUN_140D6BBF0`) re-apply the `MenuOffscrRendParam` camera
  every frame when it is true. INFERRED: it is a live-tuning (debug) switch, false in retail.
  - If so, one frame is the original behaviour, not a native fault.
  - If it were true, the view would stay listed and render every frame.

### For the native pilot

- Traverse a character view's layer with `{lo = pass bit, hi = 2}`. Bit 1 is the main pass.
  Expect its parts at `{7, 2}`, vtable `1429489A0` (`SprjModelDispEntity`).
  - Items with `hi = 1` in that layer are skipped by the original too.
  - A runtime dump of the 34 items (`vtable`, `+0x20`) separates the two groups.
- For one frame per request:
  - render the view once when it is listed;
  - keep its output texture alive and uncleared after `FUN_140D65140` removes it;
  - re-render only on a new request.
- Runtime check of the INFERRED switch: step `+0x4C` should go 5 → 6 → 7 within one tick, and
  `Finish_Play` should run on the next tick. `+0x6C9` should be 1 after the request and 0 after
  `Wait_Play`.

Exports: `decompiled/DS3-1152-F-000C2630`, `000C24E0`, `000C2EC0`, `000C1FB0`, `000C9470`,
`00060190`, `002043A0`, `00142E80`, `00D03EB0`, `00C3C3B0`, `00C4D5D0`, `00D009E0`,
`00B72980`, `00D67690`, `00D683A0` (plus `002046B0`, `002059F0`, `00D64C30`, `00D677D0`,
`00D67940`, `00D676D0`, `00D64B50`, `00D65140`, `000BFEF0` from earlier passes). Listings
(seed presets and initialisers, composite slots, draw-entity tail calls, request setters, step
`vt+0x30`, traverser vtables) are in `evidence/I-0067/queries/status-once/`.

## 25. Status: part textures, the readiness gate, and StatusBody against StatusFace (STATIC; INFERRED where marked)

### How a menu part gets its textures

1. **Files.** `ChrAsmModelRes` (`FUN_140C42620`) builds each part's file name
   (`func_0x140592570`) and requests it through the protected stub
   `thunk_FUN_14545B3F6([1447559C8], name, res+0x10)`. The load is asynchronous and `res+0x10`
   counts what is pending. `STEP_Wait_Setup` builds the model only after this reports
   everything loaded (`+0x6CB`).
2. **File textures are found by name, never requested.**
   - The material resolver `FUN_140D24490` (the one the pilot reuses) normalises the path
     (`FUN_1402FF080`), then calls `FUN_140D4ED20([144792F58], name)` →
     `FUN_142356180(bank+0x60, name)`.
   - That function is a pure hash lookup: case-insensitive, `\` read as `/`. It returns the
     registered entry or null, and never issues a load.
   - So a part texture resolves only if its TPF is already registered in the texture
     resource manager when the material is prepared.
   - INFERRED: the part TPFs come from the same part files and register the same way as for
     the in-game player. A name missing at preparation time stays missing in any resolver
     that does not retry.
3. **The head is different: its face texture is produced at runtime, not loaded.**
   - Slot 0 (`part+0x118 == 0`) gets FaceGen objects in `part+0x130` (0x28 bytes, from
     `FUN_140C3E430`/`FUN_140C3DFA0`), created by **`FaceGenManImp` `[14477DA08]`**:
     - `+0x10`: `FUN_14082A7F0` (0x140-byte instance, also stored at `model+0x758`);
     - `+0x18`: **`FrpgFgTextureIns`** (vtable `142843378`, `FUN_14082A910` →
       `FUN_140835820`, 0xA0 bytes, also stored at `model+0x770`).
   - `FrpgFgTextureIns` creates its own GX texture with `FUN_140836C40`:
     - format `0x1D`, one mip, bind 8 (shader resource only);
     - created directly through the device wrapper `FUN_1419256D0` and view `FUN_141925B10`;
     - wrapped in a 0x70 `FUN_14002AC20` object;
     - not a TPF texture.
   - The part update `FUN_140C3F980` drives it each frame. When its state `+0x70` is 2:
     1. `FUN_1408368C0` sets the overlay parameters from the appearance part
        (`[part+0x140]+0x60`);
     2. `FUN_140836E40` sets the state to 3. **If the texture reports ready (`vt+0x68`)**, it
        uploads the CPU face image (`FUN_141920300`, row pitch `w*4`), records a 2D pass
        through `[144796298]+0x78` (`FUN_14232FD10`, `FUN_142330050`; exact role not read),
        then sets the state to 0 and **`+0x91 = 1`**.
   - If the texture is not ready at that moment, the state stays 3 and `+0x91` stays 0.
     Statically, nothing in this function retries.
   - When the part's `[+0x138]+0x20` flag is set, `FUN_140C3F980` calls `FUN_14082B090`, which
     sets the instance's `+0x90 = 1` (INFERRED: a rebuild request to `FaceGenManImp`).
4. **`+0x371`** is not a texture state. It is the draw entity's "per-mesh mask changed" flag,
   set together with `+0x2B0/+0x2B8` (`FUN_140C4D160`, `FUN_140D05270`). No reader was found
   in `140C30000..140D20000`.

### The readiness gate of every view

`STEP_Init_Play` lists the scene only when `FUN_140C3B340(model)` is true (section 24). That
needs every part slot to pass `FUN_140C3F720(part)`:

- `part+0x130 == 0`, or `[part+0x130]+0x18 == 0`: ready;
- otherwise the value of **`FrpgFgTextureIns+0x91`**.

So only the head's FaceGen texture can hold a view back. It does so forever if the texture
creation or its readiness query fails natively (INFERRED for the native backend: this texture
belongs to the directly created writable family that the installed ingress does not own).

### StatusBody against StatusFace

| | `SYSTEX_Menu_StatusBody` (560x768) | `SYSTEX_Menu_StatusFace` (512x256) |
|---|---|---|
| Class, size table row | `CSMenuPlayerModelRend` row 4 (`14295D220`: w `0x230`, h `0x300`, flag 1, param rows 2/2) | `CSMenuFaceModelRend` rows 18/19 (flag 0) |
| Name registration, list, priority | same: `FUN_140D644F0` → `FUN_140D4A120([144792F58], name, proxy)`; RendManImp list; priority 100 | same |
| Pipeline descriptor (`FUN_140D65DD0` → `FUN_140D64C30` → `FUN_1400C3410`) | the table flag passes its own descriptor: byte 0 = 1, **flags `0x400852201F`** (bits 0–4, 13, 17, 20, 22, 27, 38) | default built in `FUN_140D64C30`: bytes 0..2 cleared, **flags `0x8422014`** (bits 2, 4, 13, 17, 22, 27) |
| Composite sub-scenes | `GXSgPaintSceneImpl`, then `GXSgEnvMapSceneImpl` when `[GXRS+0x52E8]` (`FUN_1400C3FE0`), then **`GXSgShadowSceneImpl`**, then `GXSgBaseSceneImpl` | Paint, (EnvMap), Base: **no shadow scene** |
| Request (`PlayerStatusMenu` constructor `FUN_140B71320`) | **only if** `FUN_140C22DE0(data)` and `FUN_140C22800(data)` are non-null; then equipment `FUN_140D6A8A0`, request `+0x6C8`, delay `+0x878 = 2` | always, through `FUN_140B72980`, with the snapshot flag `+0x6C9` (section 24) |
| Rendering after start | continuous: `Wait_Play` keeps it listed | one frame per request |

- Nothing in registration or priority separates the two views.
- Statically, what can keep the body unlisted:
  1. the request is skipped (null data pointers);
  2. its model never passes the FaceGen gate;
  3. its scene graph has a shadow sub-scene first among the per-pass scenes.
     A native path that expects the face layout (Paint, Base) or handles only the default
     descriptor treats the body differently.
- Corrections:
  - section 12 ("character views: fixed `0x8422014`") holds only for the Prof and Face
    families. The Player family (`ChrMakePreview*`, `StatusBody`, `RankingBody`) uses
    `0x400852201F` and has the shadow sub-scene;
  - sections 17 and 22 (view shadow) therefore apply to the Player views only.

### Runtime checks for the pilot

- **Body request.** After the Status screen opens, body view `+0x6C8`/`+0x4C`; it is 0 and
  step 1 if the request was skipped.
- **Readiness.** `[[obj+0x6E8]+0x28]+0x130 → +0x18 → +0x91` for the head, and its `+0x70`
  state. A stuck 3 means the face texture was not ready.
- **Texture names.** For each of the 34 items, the material texture names and whether
  `FUN_142356180([144792F58]+0x60, name)` finds them at preparation time and later.

Exports: `decompiled/DS3-1152-F-00C3B340`, `00C3F720`, `00C3E140`, `00C3E430`, `00C3F980`,
`0082A910`, `00835820`, `00836E40`, `00C42620`, `00B71320`, `000C3FE0` (plus `00836C40`,
`00D65DD0`, `00D6A3B0`, `00D69770`, `000C2DA0`, `00D24490`, `00D4ED20`, `02356180` from
earlier passes).

## 26. Menu part materials: every input that depends on the in-game character, and its menu value (STATIC; INFERRED where marked)

### Draw chain of a menu part (it differs from the world entity)

- **Menu.** `SprjModelDispEntity` slot 1 `FUN_140D04B90`:
  - its draw-state group is at **`entity+0x1F0`** (400 bytes, defaults `FUN_14004DF20`);
  - for each `SprjFD4ModelItem` in `entity+0x90` it calls the item's slot 11
    **`FUN_140D3DA20(item, pass, group, entity)`**.
  - The item builds the draw descriptor, then reaches the common recorder `FUN_140057A20`
    through slot 2 (`FUN_140059130`).
- **World.** `SprjModelDrawEntity` `FUN_140D06E10`: its group is at `entity+0x240`, and its
  slot 18 `FUN_140D079C0` builds the descriptor.
- Same recorder and the same descriptor offsets, but other source fields. These are the
  menu sources:

| Input (descriptor or group offset) | World source | Menu source (`SprjFD4ModelItem`) | Menu value |
|---|---|---|---|
| Instance decal provider, t41/t42 (descriptor `+0x98`) | `[entity+0xA60]+0x50` | **`[item+0x740]+0x50`** | see "Decals" |
| Model flags for the common decal (`+0x224` bit 2, option `14473C7E5`) | model data | same model data (`item+0x58`) | unchanged |
| Face texture (descriptor `+0xA0`) | — | `[item+0x770]+0x30` (`FrpgFgTextureIns`) once `+0x91`; before that `[item+0x58]+0x358` or `+0x360` when `item+0x6F2` (choice `+0x6F3`) | see "FaceGen" |
| FaceGen shape instance (descriptor `+0x28`) | — | `item+0x758` (`FaceGenManImp` instance) | head only |
| Part tint (group `+0x40..+0x4F`, dirty `+0x180`) | entity | **`item+0x780..+0x78F`**, copied at each draw | see "Colours" |
| Wet words a, b, c (group `+0x8C/+0x90/+0x94`) | wet producer on `entity+0x240` | `entity+0x1F0` group | **0, 0, 0** (dry) |
| Material ex-params (`ExParam_*`) | in-game character modifiers | none | **feature off** |

### Decals (blood and snow)

- `item+0x740` is created only in `FUN_140D3C4D0`, the item set-up, as
  `FUN_140E000E0([RendManImp+0x10] = StageRend, item, modelData, 0)`. All three must hold:
  1. `StageRend` exists;
  2. the set-up's `param_5 > 0`;
  3. `FUN_1400600A0(modelData)`: model flag `+0x224` bit 2 or 3, or a material name containing
     `_Bloody` or `_DeepSnow`.
- The release is `FUN_140E00260` (`FUN_140D3CC00`).
- **`FUN_140D3C4D0` has no plain-code caller.** The menu part is built by the encrypted
  `CSPartsModelIns` constructor (`0xC3DB40`), so `param_5` for menu parts is not readable
  statically.
- Runtime check: `[part+0x10]+0x740`.
  - If it is null, the recorder takes the common branch (`+0x224` bit 2, then the common
    decal image, unless `14473C7E5`). This is the pilot's common decal path.
  - If it is non-null, it is a stage decal receiver. INFERRED: the menu never projects hits
    or footprints onto it, so its content equals "no decal", and the common image is the
    equivalent native choice.

### FaceGen (head only, slot 0)

- `item+0x758`: shape instance (`FUN_14082A7F0`).
- `item+0x770`: `FrpgFgTextureIns` (section 25):
  - its GX texture `+0x30` replaces descriptor `+0xA0` once `+0x91 = 1`;
  - before that, descriptor `+0xA0` holds the model data's own texture `+0x358`/`+0x360`, or
    nothing.
- The world player uses the same FaceGen objects. The menu has its own instances on the
  menu `CSChrAsmModelIns`, built from the save's appearance.

### Colours (skin, hair and the other appearance colours)

`CSChrAsmModelIns` `FUN_140C3B460(model, appearance)` runs at set-up and in every
`STEP_Wait_Play` frame:

1. It builds eight RGBA colours from the appearance. `FUN_140592DD0(appearance, i)` maps bytes
   `+0x44 + 4i` through the colour ranges of `FUN_140E2DA50`; the alpha is the fourth byte
   scaled.
2. It caches them at `model+0x290 + 0x10*c` (`FUN_140C3B870`).
3. It writes them to the parts' `item+0x780` (`FUN_140C4D180`) by slot class (`FUN_140C3B250`).

| Colour index | Appearance index | Part slots |
|---|---|---|
| 0 | 0, plus the head's offset `FUN_140C3F610(part 0)` | every slot not listed below (body, arms, legs, head) |
| 1 | 1 | 6 |
| 2..7 | 2, 3, 4, 8, 5, 6 | 0x11..0x16, in order |

INFERRED names: colour 0 is skin and colour 1 is hair; 2..7 are the FaceGen face layers.
FaceGen types 0x11..0x16 come from `FUN_140C3E430`.

### Material ex-params

- The `ExParam_*` names (table `1445B3970..39C8`) are only read by in-game character code:
  - `FUN_1409CAA80` sets `DamagedNormal_DamageRate` per equipment part (from `FUN_1408C1BC0`);
  - `FUN_1409CB270` sets `WetDiffuseColor`, `WetReflectanceColor` and `WetParam` from a wet
    param row (from `FUN_1408C1BC0`);
  - `FUN_1409CAF30` sets the seven `Phantom*` ex-params (from `FUN_1405B34F0`, `FUN_140890950`);
  - `FUN_1409CAD60` sets `g_GlowScale`, `g_GhostEdgeColor`, `g_GhostEdgeParam` and
    `g_BlendMode` (from `FUN_1408C7B30`).
  - All four go through the modifier `FUN_1409CAE60`.
- None of them is reached from the menu family (`CSMenu*ModelRend`, `CSChrAsmModelIns`,
  `CSPartsModelIns`). So a menu model has:
  - no damage (rate 0);
  - no wet colours or weights (dry);
  - no phantom colouring.
- INFERRED: this is the pilot's explicit inactive contract (D-0157/D-0158).

### What the pilot supplies at once for a menu part

1. **t41/t42:** the common decal image when `[item+0x740]` is null, and also when it is
   non-null (INFERRED, see above).
2. **b2 damage and wet:** the feature-off values (damage rate 0, wet weights 0). The group's
   wet words are 0.
3. **Tint:** `item+0x780` into group `+0x40`, every draw.
4. **Head:** the FaceGen texture at descriptor `+0xA0` once `FrpgFgTextureIns+0x91`, else the
   model data's own texture.
5. **Everything else** (FLVER textures by name, MTD values, SPX tables) is the same as for the
   world player.

Exports: `decompiled/DS3-1152-F-00D04B90`, `00D06E10`, `00D3DA20`, `00D3C4D0`, `00E000E0`,
`000600A0`, `00C3B870`, `00C3B250`, `00C4D180`, `00592DD0`, `009CAA80`, `009CB270`,
`00C3B950`, `00D079C0` (plus `0004DF20`, `00C3B460`, `009CAD60`, `009CAF30`, `00057A20`).

## 27. Status view: background, lights, fog, IBL and tone values (STATIC data; consumers INFERRED where marked)

### Which gparam the view reads

- `MenuOffscrRendParam` carries no light, ambient or cubemap value of its own. It only supplies
  the camera and the gparam id `+0x2C` (section 19): **Status body = row 2, id 11**.
- `FUN_140088AF0(pipeline, {1, 11})` sets the same key `{1, 11}` on the pipeline's 13 bank
  slots:
  - key A at `+0x3FD8 + 8i`, key B at `+0x4040 + 8i`, weight `+0x40A8 + 4i` = 0;
  - the shadow keys through `FUN_1400888B0` (section 22).
- **Area 1 is `default.gparam`.** Small areas index the name table `14294F020` (`FUN_140D27D30`
  formats `gparam:/%s.gparam`):
  - 1 `default`, 2 `M_template`, 3 `ML_template`, 4 `S_template`, 5 `empty`, 6 `MapBase`,
    10 `RemoBase`, 12 `eclipse` (the eight startup files);
  - 7 `m00_00_0000`, 8 `MapOverride_0000`, 9 `MapOverride_forRemo`, 11 `s00_00_0000`,
    13 `MDLVW_`.
  - The pilot's `Banks::File` maps area 1 to `default` too. A group without id 11 falls back to
    `{1, 0}` (`Banks::Resolve`, `140032770`).
- Values below are read from `param/drawparam/default.gparam.dcx` (the I-0045 extraction,
  SHA-256 in the TSV) with the new read-only `Get-GparamValues.ps1` (same layout as
  `native/content/gparam.cpp`).
  - Full dump: `evidence/I-0067/queries/status-light/default-gparam.tsv`.
  - Ids present: LightSet, Fog, Env Map, ShadowParam, SSAO, Setting Info, Tone Map and
    Anti-Alias have 0 and 10..15; the other groups only 0, so id 11 falls back to 0 there.

### Background (the grey)

- **Per frame, the clear colour is the gparam `Setting Info` "BG Color"**, not the constant:
  - `FUN_140087FB0` (pipeline per-frame update) runs while `pipeline+0x4144` ("BG Color - Ref -
    GParam", debug GUI `FUN_140089960`) is 0, which the constructor `FUN_1400843C0` sets;
  - it requires the Setting Info bank `[GXRS+0x4610]` to know both keys of slot 0;
  - it then blends the two records' `+0x80..+0x8C` by `+0x40A8`, clamps to 0..1, scales by 255
    and **overwrites `pipeline+0x6C`** (debug label "BG Color"), packed A, R, G, B from the high
    byte down.
- The constant `0x141414` that `FUN_140D64C30` stores at `+0x6C` (with `+0x78 = 1`) is only the
  initial value. The one-time clear of the output texture at set-up is (0, 0, 0, 1)
  (`FUN_141920200`).
- **Value for `{1, 11}`: BG Color = (0.070588, 0.070588, 0.070588, 1) = `0xFF121212`**, that is
  18/255 grey. `0x141414` would be 20/255.
- The consumer (I-0113 tranche 29, GhidraMCP): for the menu views (pipeline+0x78 set) FUN_140103510's key 0x1A order 1 draws a
  full-screen quad of FUN_140112b40(+0x6C) into the forward HDR (R, G, B squared and byte-rounded: 18/255 gives 1/255; VERIFIED),
  depth LESS_EQUAL without write, before the forward rows; the world (+0x78 zero) clears its target with the same colour at
  key 0 order 1 (I-0113 RESULT « Tranche 29 »).

### Lights (`LightSet`, id 11)

| Light | Angle (rad, pair) | DiffColor (r, g, b, intensity) | SpecColor | Sharpness | SsaoWeight |
|---|---|---|---|---|---|
| Directional 0 | (−0.837758, 2.30383) | (1, 1, 1, **2**) | (1, 1, 1, 2) | 2.5 | 0.8 |
| Directional 1 (**follows the camera**) | (0.034907, 1.25664) | (0.962099, 0.913793, 0.845481, 0.5) | (0.962099, 0.922414, 0.883382, 0.5) | 15 | 0 |
| LocalLight 1 (enabled) | (−0.174533, −2.6529) | (0.880466, 0.927114, 1, 0.326531) | (0.897959, 0.959184, 1, 0.3) | 2 | 0 |
| LocalLight 2 (enabled) | (−0.523599, −0.820305) | (0.970845, 0.938776, 0.865889, 0.1) | (0.965015, 0.918367, 0.889213, 0.1) | 10 | 0 |

- **Ambient:**
  - Hemi Color Up = Hemi Color Down = (0, 0, 0, 1). There is no hemisphere ambient;
  - giOcclusionRate 0, LightMask Influence 0.
- INFERRED: the per-instance SH (TONEMAP-SKIN section 10) has no map probe in the view. Its
  fallback, these hemisphere rows, is therefore zero.
- The angles are converted to directions the way the pilot's `Banks::Sun` already does for map
  sets. Area 1 has no map offset. "Light1 FollowedCamera = 1" makes directional 1 relative to
  the view camera.

### Fog, IBL, shadow, tone (id 11)

- **Fog:**
  - Color (0.264789, 0.309859, 0.292958, 0.8), Dist 2, DistOffset 0, Alpha 0.5, Contrast 1;
  - height fog: Color (0, 0, 0, 1), heightFogDensity 100, heightFogIntencity −0.01,
    heightDist 0, HeightOffset 0, ToUpper 0.
- **IBL:**
  - EnvMap:ID **10**, ColorScale **1**. The group has no prefix string, so the cube name is
    **`EnvMap_0010`** (prefix `EnvMap_`, `FUN_14006CED0`, TONEMAP-SKIN section 11), resolved
    through the env-map repository;
  - INFERRED: as the view has no env map regions (section 17), this default entry is its IBL.
- **ShadowParam:**
  - Angle (−1.01229, 2.51327), Shadow Light Id −1, Near 2;
  - cascades 5 / 5.5 / 6, Far Fade Start 8, Far Fade Dist 10;
  - FilterMode 2, Depth Offset 0, Volume Depth 100, Slope Scaled Depth Bias 0.005;
  - Cull Flip 0, AlignTexel 1, DepthRate 0.23, DepthScale 15.417, KernelScale 0.67,
    DepthOffsetScale −0.07;
  - Blur Count 4, Blur Radius 8, Shadow Color −16777216 (`0xFF000000`), Resolution 1.
  - This applies to the Player views only (section 25).
- **Tone Map:**
  - Exposure 2, Gamma 2.2, ToneType 2, ToneMappingFactor 32, FilmicContrast 0.5;
  - AutoExposure 0, MiddleGray 0.18, MinExposure 0.1, MaxExposure 10;
  - second block: Exposure 4.60001, AvgLuminance 9, ToneMapMode 7, Contrast 0.5, Gamma 2.2,
    UseTable 1, Enable AutoExposure 0.
  - The views have no YEBIS stage (no bit 10, section 12). `FUN_140087FB0` copies the
    MotionBlur bank's blended values (keys `+0x4008/+0x4070`, `FUN_1401D9910`) into
    `+0x3E80..+0x3EA4` when `+0x3EB9` is set (the menu views clear it), not the tone map's. Corrected by I-0113 tranche 27 (RESULT « Tranche 27 »): there is no fixed exposure at `+0x3E84..+0x3EA4`; `+0x3E88` is a MotionBlur field read by `FUN_140024740` for `view+0xCA4` (0.2 / max([+0x3E88], 1e-6)); the menu views (no YEBIS, bit 10 clear) are tone-mapped by the GX tone map filter at `pipeline+0x3060` (`FUN_1401C48D0` → `FUN_1401C48E0`), whose inputs are the Tone Map bank `[GXRS+0x4618]`'s second block (Exposure, AvgLuminance, Scaler, ToneMapMode, Contrast, Gamma, UseTable) blended by `FUN_1400FB890` into the filter's block `pipeline+0x3080..+0x30E0`; TabledToneMapPS draws for modes 0 and 7 with UseTable.
- **SSAO** (id 11) and **Anti-Alias** (FXAA type 4) values are in the TSV.

### For the native Status view

| Item | Value |
|---|---|
| Clear | `0xFF121212` |
| Lights | the four lights of the table; directional 1 in camera space |
| Ambient | none (hemisphere 0) |
| IBL | `EnvMap_0010` × 1.0 |
| Fog | the id 11 values |
| Shadow | Player views only |
| Exposure | the Tone Map bank's second block, through the GX tone map filter (I-0113 RESULT « Tranche 27 ») |

All values come from `default.gparam` id 11. A view with another `MenuOffscrRendParam` row
takes its own id (10, 12..15) from the same file.

Exports: `decompiled/DS3-1152-F-00087FB0`, `00089960`, `00D27D30` (plus `00088AF0`, `000888B0`,
`00D64C30`). Tool: `Get-GparamValues.ps1`.

## 28. Menu sounds in practice, and the teardown order of an offscreen view (STATIC; INFERRED where marked)

### Sounds

Section 3 holds the owners. What is played, and from where:

- **Two request paths, one player.** Both end in `FUN_1404AD510(category, id)`, then
  `FUN_141A299F0` on a channel of the MenuMan sound object `[144763868]`.
  - **Old dialogs** (`FrpgMenuDlg*`, code `14069xxxx..14077xxxx`) call `FUN_14076B930(id)`
    directly: 181 sites, category `f`.
  - **Scaleform menus** (`AppMenu`, code `140A4xxxx..140BFxxxx`, about 100 callers) call
    `FUN_140A67CA0(request {id, priority, channel})`:
    - channel `0x29` is deferred: the highest priority of the frame is kept (`14478D1D0`)
      and played by `FUN_140A67BD0` via `FUN_14076B8B0(channel, id)`;
    - channel `0x2E` plays the id as category `v` on handle `+0x1E4`;
    - any other channel plays at once.
  - **`FUN_140A67BD0` runs at the end of the `NewMenuSystem` update** (call at `140B3E7E1`,
    inside the protected update of section 2). Scaleform menu sounds therefore need that
    update to run, not the renderer.
- **Event names.** The category letter plus 9 digits (section 3), for example `f000000100`.
  Most frequent ids:

| Id | Old dialogs | Scaleform | Context read (INFERRED meaning) |
|---|---|---|---|
| 100 | 48 | 21 | selection changes in list handlers (`FUN_140709870`): **cursor move** |
| 200 | 37 | 5 | end of the same handlers, after the choice: **decide** |
| 300 | 8 | 1 | cycling a 3-state option (`FUN_140714AC0`): **value change** |
| 400 | 1 | 12 | Scaleform windows (role not read) |
| 600, 900 | — | 3, 2 | Scaleform (role not read) |
| 18, 75, 81, 162, 270 | 1–3 each | — | specific dialogs |
| 999999999 | 2 | 1 | "no sound" sentinel |

  - 77 old-dialog sites and most Scaleform sites pass the id in a register, from data tables
    or window state.
  - **Exact mapping:** log `FUN_1404AD510`'s category and id at runtime while navigating.
    That is cheaper than reading every handler.
- **Music.** The title top dialog starts `m000000001` (`FUN_140ADA260` → `FUN_14076B920(1)`).
  `FUN_140AE8930` (title dialog step) stops the menu BGM (`FUN_14076B9F0`).
- **Banks.**
  - Resident banks: `sound:/fdp_main`, `sound:/fdp_smain` and their `_dlc1`/`_dlc2`
    variants.
  - Character banks: `sound:/frpg_c%04d.fev/.fsb/.itl`.
  - Mixer: `fdp_mixer.fev` (`FmodEventFilePath_0` in the sound configuration).
  - INFERRED: the `f`, `m` and `v` menu events live in the resident `fdp_main`/`fdp_smain`
    banks; no menu-specific bank is loaded on open.
- **Rendering dependence: none** (confirmed and extended in [I-0069 MENU-AUDIO-NATIVE.md](../I-0069-static-menu-audio-map/MENU-AUDIO-NATIVE.md): the banks are `fdp_main.fev` for the `f` effects and `fdp_smain.fev` for the `m` music). No path above touches GX objects. The only coupling is
  indirect: Scaleform sounds play from the `NewMenuSystem` update, and old dialogs from their
  own update. Silence natively means those updates do not run (sections 1–2).

### Teardown of an offscreen view (Load Game portraits and every view)

This corrects section 15: the scene graph is released in `FUN_140D65160`, not in the holder
delete. Expected order:

1. **Owner** (`TitleTopDialog` destruction for the portraits):
   - `FUN_140D67580` sets the finish request `+0x6CA`;
   - `FUN_1408190B0([14477D778], obj)` queues the view object on the deferred queue
     `[144964930]` (`FUN_1423559C0`). The object lives until that queue runs.
2. **View destructor `FUN_140D66280`** (run by the queue):
   1. `FUN_140D68EB0` releases the character:
      - model `+0x6E8`: its parts leave the layer through `FUN_140C4C980` → layer `vt+0x48`;
      - `+0x6F0`;
      - `+0x870` and `+0x820` (location modifiers unregistered first, `FUN_140D1A770`);
      - `+0x818`, skeleton `+0x810`, resource `+0x6E0`.
   2. It releases `+0x6D8` (`ChrAsmModelRes`), then `+0x6D0`.
   3. `FUN_140D65140(holder)` removes the composite from RendManImp's list when `+0x58` is
      set.
   4. **`FUN_140D65160(holder)`, the scene teardown:**
      1. unlist again if still listed (`FUN_1400C02D0`);
      2. registration context `+0x50`: `FUN_140D03A20` drops its layer reference, then it is
         freed;
      3. composite `+0x48`: `vt+8(0)` detaches the general scene, `vt+0x28(0)` clears the
         camera binding;
      4. **`FUN_1408192B0([14477D778], composite)` takes one reference and queues the
         composite on the same deferred queue `[144964930]`**, then the holder drops its own
         reference;
      5. general scene `+0x38`: `FUN_1400C1F40` detaches layer `+0x40`; then `+0x40` and
         `+0x38` are released;
      6. camera holder `+0x30` is destroyed; views `+0x28` and `+0x20` are freed.
   5. The holder object is deleted, running `FUN_140D64710`:
      - `+0x58` is already 0, so `FUN_140D65160` is a no-op;
      - **the named texture is unregistered** (`FUN_140D4F4B0([144792F58], +0x10)`);
      - the proxy `+8` is released;
      - `FUN_140D65970` runs (encrypted at rest);
      - the `+0x48/+0x40/+0x38` releases find null.
   6. The step members and the appearance are torn down.
3. **Later, when the deferred queue drops the composite:** `GXSgCompositeScene` destructor
   `FUN_1400C2DD0`.
   - **It hands its pipeline instance `+0xF8` to the GX retirement list** `[+0xF0]+0x28`
     (`FUN_140023530`). GPU resources are freed only when GX processes that list.
   - Then the base destructor `FUN_140204480` releases its sub-scenes `+0x18..+0x28` and its
     `+0x38` (pipeline), `+0x40` and `+0x48` references.

### What the native side must respect (INFERRED from this order)

- **The output texture outlives the scene graph.** The named proxy is unregistered after the
  composite is queued. The composite and its pipeline (whose `+0x1E0` target is the proxy's
  inner texture) are freed only by the deferred queue, then the GX retirement list.
  - A native backend that frees the output texture on unlist or on `FUN_140D65160` leaves the
    proxy, and any Scaleform sampler of `SYSTEX_Menu_Profile0N`, pointing at freed memory.
- **Native stand-ins must survive the deferred queue.** `FUN_1408192B0` adds a reference and
  the queue later calls `vt+0` with flag 1. A native composite needs a valid `vt+0` and the
  `+8` count until then, and must not be torn down synchronously by the holder.
- **Retire, do not free.** The pipeline instance goes to the GX retirement list. Its native
  replacement must be retired after the last frame that used it, like any other D3D12
  resource still in flight.
- **Order in one call:** model and modifiers first, then unlist, scene graph, camera, then the
  name and proxy. The location modifiers must be unregistered before the skeleton and the
  model are freed (section 15).

Exports: `decompiled/DS3-1152-F-00D65160`, `00D64710`, `00D66280`, `000C2DD0`, `00204480`,
`00023530`, `008192B0`, `00D03A20`, `000C1F40`, `00A67CA0`, `00A67BD0`, `00714AC0`,
`00709870`, `00ADA260`, `00AE8930`.

## 29. Phantoms end to end, and what Load Game adds over Continue (STATIC; INFERRED where marked)

### How the engine draws a phantom (summary of sections 6, 13 and 23, plus the new data)

1. **Programmes.** Each material shader set also loads `<shader>_Phantom_Fwd` (pass 0) and
   `<shader>_Phantom_FwdSdw` (pass 4), with `[A]` and `[AA]` variants, at `set+0x18..+0x28`
   and `+0x78..+0x88`. They exist only for the forward pass; there is no G-buffer variant.
   - The site that swaps a phantom's draws to these slots is still not found statically
     (section 23). A runtime read breakpoint on a phantom's shader set at `+0x18..+0x28` gives it.
2. **Pass order.** A phantom is a forward draw: after the G-buffer and lighting, with the
   other forward materials. Blend mode 2 for `P_Ghost_FDP` (I-0066 forward finding).
   Its shadow uses `_Phantom_FwdSdw` in the shadow passes.
3. **Constants.** `FUN_14022D620` blends the seven `ExParam_Phantom*` values by the
   character's weight into the material block. `FUN_14022E930` then writes five float4 at
   `+0x1A0..+0x1EF` (edge, front, diffuse, specular, light; `rgb² · a`, with the edge and front
   sign negated by the subtract flags; section 13).
4. **Gesture and replay ghosts only:** `g_GlowScale`, `g_GhostEdgeColor`, `g_GhostEdgeParam`
   and `g_BlendMode` are copied from the `P_Ghost_FDP_Gesture` and `P_Ghost_FDP_Replay`
   materials (`FUN_1409CAD60`, from `PlayerIns`/`ReplayGhostIns` slot 154 `FUN_1408C7B30`).
   Their values are the materials' own MTD parameters (not read here).

### Where the seven `ExParam_Phantom*` come from for a phantom character

- **Every frame**, `ChrIns` slot 36 (`FUN_140890950`; `PlayerIns`/`ReplayGhostIns`
  `FUN_1408C1BC0`) asks the character's model-parameter holder `[chr+0x1F90]+0xD8` for a
  PhantomParam row. The id is `chr->vt+0x3A8()`, and it is applied only when it is ≥ 0, with
  `FUN_1409CAF30(holder, {0, id, 0})`.
- **Row id** (`FUN_14088A950`; `ReplayGhostIns` first tries its own table, `FUN_1408CA7E0`):
  1. an explicit override at `chr+0x1F38`, when not negative;
  2. else `chr+0x1A58`, unless it is `0xFFFFFFFE`;
  3. else `[chr->vt+0x178()+8]+0x1CC` (INFERRED: the NPC parameter), or `+0x238` of the same object when `FUN_1409F6480` holds;
  4. else, for players: **RoleParam** (param index 0x45) of the character's role, read by
     `FUN_140E3C220(row, tier)`:
     - `tier` = covenant rank from the character's vow bytes (`FUN_1405A6E40`), cut at
       10/30/80 (`FUN_1405A55C0`);
     - it selects `+0x4` (base) or `+0x4C/+0x50/+0x54` (vow rank 1..3).
- **RoleParam → PhantomParam** (regulation dump `evidence/I-0067/queries/phantom/roleparam.tsv`):

| RoleParam row | PhantomParam id (base, rank 1..3) |
|---|---|
| 1, 50001, 80001 | 80 |
| 2, 10002, 50002, 80002, 90002 | 81 |
| 10, 11 | 63, 74 |
| 10001, 10013 | 83, 83, 84, 84 |
| 20001 | 85 |
| 20002, 20013 | 86 |
| 30001, 30002, 30013 | 88 |
| 40001, 40002, 40013, 60001..60013, 70001..70013 | 91 |
| 90001, 90013 | 82 |
| 0, 3..9, 12..14, 80013 | −1 (no phantom colouring) |

- **PhantomParam row layout** (from the getters `FUN_140E35F90..140E361D0`; 0x30-byte rows,
  `evidence/I-0067/queries/phantom/phantomparam.tsv`):
  - colour bytes /255 plus a float weight `a`;
  - edge = bytes `+0x14..16`, `a +0x00`; front = `+0x17..19`, `a +0x04`;
    diffuse = `+0x1A..1C`, `a +0x08`; specular = `+0x1D..1F`, `a +0x0C`;
    light = `+0x20..22`, `a +0x10`;
  - edge-subtract `+0x2D`, front-subtract `+0x2E`.
  - Examples:
    - **80** (role 1): edge (255, 255, 255) a 0.5; front 0; diffuse white a 0.5; specular
      white a 1; light 0;
    - **81** (role 2): edge (255, 0, 0) a 0.6; diffuse and specular white a 0.2; light 0.
- **Defaults when no row resolves:** the getters return `14479809x..` (for example
  `14479809[8..C]` for the edge). Those are runtime-initialised globals; they are zero in the
  image. With the id below 0, no ExParam is written and the material keeps its own values
  (no phantom tint).

### Why Load Game can fail natively when Continue works

Both paths build and destroy the same `TitleTopDialog`. Its ten `CSMenuProfModelRend`
portraits and their composite scenes exist in both; their teardown at game start (section 28)
queues ten composites and retires ten pipelines either way. **What only Load Game adds:**

1. **The portrait requests.** `MenuSaveDataSummary` → `FUN_140ADEF50`, for each slot with a
   save:
   - appearance (`FUN_140D6B9F0`), equipment, flags;
   - **start delay `slot*2`** (`FUN_140D668A0`);
   - request (`FUN_140D67540`) and **snapshot** (`FUN_140D67560`, section 24).
   - With Continue, every portrait stays in `STEP_Wait_Request`: no model, no listing.
2. **Up to ten character builds:**
   - `ChrAsmModelRes` part-file loads (`thunk_FUN_14545B3F6`, asynchronous);
   - ten `CSChrAsmModelIns`/`CSPartsModelIns` models;
   - `SprjFD4ModelItem` items registered in each view's layer;
   - location modifiers registered with the FD4 location manager `[144791BD0]` (section 15).
3. **Ten FaceGen face textures** (section 25): `FrpgFgTextureIns` created by `FaceGenManImp`.
   - Each makes a texture directly through the device wrapper `FUN_1419256D0`/`FUN_141925B10`;
   - it is filled by `FUN_141920300` and a 2D pass;
   - each portrait waits for it before rendering.
4. **Ten one-frame renders:** the composites are listed for one RendManImp draw each, staggered
   by the start delay; Scaleform then samples `SYSTEX_Menu_Profile00..09`.
5. **A heavier teardown** when the save is chosen (`TitleTopDialog` destructor →
   `FUN_140AED060` → deferred queue):
   - `FUN_140D68EB0` frees real models: parts leave the layers, and the location modifiers are
     unregistered before the skeletons;
   - `FaceGenManImp` handles are released (`FUN_14082ABF0`, `FUN_14082ADD0`);
   - **each `FrpgFgTextureIns` destructor (`FUN_140835BD0`) puts its two GX textures
     `+0x20/+0x28` on the GX deferred-delete queue `[14477D778]` (`FUN_140818F30`)** and
     releases `+0x30`.
   - All of this happens while the game starts loading the save.

INFERRED ranking for a native crash present only in Load Game:

1. The FaceGen textures: created and released through the device wrapper path, outside the
   installed texture ingress (section 25).
2. A native draw or cache that still holds a portrait's items or textures after
   `FUN_140D68EB0` freed them. The model is freed before the scene is unlisted, and in
   snapshot mode the scene left the list long before.
3. The location-modifier and skeleton order (section 15).

### Runtime discriminators

- **Crash point.** Its frame against the ten `FUN_140D66280` calls on the deferred queue, and
  whether it falls inside `FUN_140835BD0` or `FUN_140818F30`'s later processing.
- **Portrait states.** `14478D680[0..9]` objects: step `+0x4C`, `+0x6E8` model, and the head's
  `FrpgFgTextureIns +0x70/+0x91` (section 25).
- **Experiment.** With Load Game and a single save (one portrait) against several, the
  failure should scale with the number of built portraits.

Exports: `decompiled/DS3-1152-F-009CAF30`, `00890950`, `0088A950`, `008CA7E0`, `00E3C220`,
`00E3C4A0`, `005A55C0`, `005A6E40`, `00E36020`, `00ADEF50`, `00835BD0`, `00835DF0`.

## 30. Effect lights from creation to the per-pass light list: every filter (STATIC; INFERRED where marked)

Sections 10 and 14 cover registration and creation. This section adds what happens between
the shared light vector and the lights a pass draws. It then gives one decision tree for the
pilot's 59 against 163.

### 1. Creation (sections 10 and 14, recalled)

1. The FXR point-light action (type 0x261) resolves at FXR load (`FUN_141AB36C0`, sealed
   registries).
2. At instance time the middleware calls the factory `FUN_14028F200` → host
   `GXFfxDrawEntityHostFrpgPointLight` (`FUN_1402B2410`). Spot (11000) and line (11002) hosts
   have their own factories.
3. `FUN_140266FC0` allocates a `GXPointLight` (0x1B0 bytes) and **pushes it into
   `[GXRS+0x41D0]+0x10`**, the vector map lights share.
4. **Defaults of every light** (base constructor `FUN_1400F1A40`, then `FUN_1400EFCB0` for a
   point light):
   - `+0x58 = 1`;
   - **mask `+0x60 = 0xFFFFFFEF`, `+0x64 = 1`**;
   - **`+0xA0 = 1`**;
   - **priority `+0xA4 = 100`**;
   - type `+0xA8`;
   - `+0xB0 = 0x10001` (visible bytes `+0xB0/+0xB1`);
   - `+0xB4 = 2`.
   - The host then sets `+0xB0 = 0` until its first update (section 10).

### 2. Per frame: the host decides visibility (`FUN_1402B2E30`, host slot 2)

The host writes colour × intensity at `light+0x150..+0x15C` unless `light+0xCD` (locked). It
then computes `visible`. **The light is hidden when any of these holds:**

1. **Intensity.** The scaled intensity (FXR curves × fade × owner factor) is ≤ 0.
2. **Distance.** The owning effect has a distance block: owner property `[[host+8]+0x10]+0x70`
   of kind 2000 with more than 2 entries, whose record tag `0x2904` holds `{enabled, maxDist}`
   at `+0xA0`. The light is hidden when it is enabled and the effect's position
   (`FUN_141ACD3F0(...)+0x30`) is farther than `maxDist` from the reference point of the
   update (INFERRED: the camera).
3. **Owner category hidden.** With `owner` the object the update reads first (INFERRED: the host's effect instance), the light is hidden when
   `owner+0x1F8 & ~[owner+0x1F0]+0x14C == 0`, that is when every category bit of the owner is
   set in the manager's hide mask `+0x14C`.
   - INFERRED: `[owner+0x1F0]` is the effect manager or scene-control state, and `+0x14C` a
     per-category display mask.
   - **One wrong value there hides a whole family of effect lights at once.**
- Then `light+0xB0 = visible` (and `+0xB1`) when it changed.

### 3. Per pass: collection into the pass light list (`FUN_1400FCE30`)

`FUN_1400FCE30` is the pass scheduler that `GXSgBaseSceneImpl` slot 4 (`FUN_1402059F0`, section
24) calls with the scene mask `{(composite+0xE0 lo & ~0xFF) | 1, composite+0xE4}`, `{1, 1}` for
the world. It collects lights only when:

- `pipeline+0x3EBE != 0`;
- and the pass is not 0x39, unless `[GXRS+0x5360]+0x6D`.

For each light of `[GXRS+0x41D0]+0x10`, **in vector order**:

1. `light+0xB0 != 0` (visible);
2. `light+0xA0 & 0xFFFFFFFF != 0`;
3. the collector (vtable `143D622B0`, slot 1 `FUN_140222FF0`) requires
   **`light+0x60 & mask.lo` and `light+0x64 & mask.hi`**:
   - the world mask `{1, 1}` needs bit 0 of both; the defaults pass;
   - a menu view (`hi = 2`) never collects a default light;
4. then the light's culling `vt+0x18(light, view)` (`GXPointLight` slot 3 `1400EFF10` → the
   view's `vt+0x50` against the light at `+0x150`): a view-volume test.
5. The collector stops when its count reaches `+0x40`. Here that is `0x7FFFFFFF`, so **there is
   no budget in this collector**. The priority variant (slot 3 `FUN_140223130`) replaces the
   lowest `+0xA4` when full; it is not the one used here.

`FUN_140103510` then prepares the pass with the collected list.
`GXLightingSystem::_RenderLights<GXDrawPointLight, 4/12>` draws them in batches of 4 or 12.
That is a batch size, not a cap (RTTI template arguments).

### Decision tree for 59 natively against 163 originally

| Measure (runtime) | If short | Owner |
|---|---|---|
| Size of `[GXRS+0x41D0]+0x10` | lights never created | section 14 gates: registry count `[1449651D8]` and seal `DAT_1449651E8`, FFX library seal `[144933088]+0x20`, FFX manager `[[144799C98]+0x58]`, calls to `FUN_14028F200`/`FUN_1402B2410` |
| Lights with `+0xB0 = 1` among them | hosts hide them | §2: intensity (FXR curves need the effect's time), distance block `0x2904`, **hide mask `[owner+0x1F0]+0x14C`** against `owner+0x1F8` |
| Lights passing `+0xA0`, `+0x60/+0x64` against the scene mask | mask mismatch | a host or the pilot writing `+0x60/+0x64` differently from `0xFFFFFFEF/1` |
| Lights passing the view test | culling | position or radius at `+0x150..` not written (host update not run), wrong view |
| Pass collection running at all | none collected | `pipeline+0x3EBE`, pass id 0x39 rule |

**Fastest single check:** dump, for each entry of `[GXRS+0x41D0]+0x10`, its vtable (point,
spot or line), `+0x60/+0x64`, `+0xA0`, `+0xB0` and its host's owner `+0x1F8` with
`[owner+0x1F0]+0x14C`, in the original and natively at the same spot.

- If the vectors already differ in size, the loss is creation-side (section 14).
- If they have the same size, the first differing column names the filter.
- **A whole family missing** (for example every bonfire-ember light) points at the
  owner-category hide mask (§2.3) or at one FXR action type that never resolved (section 14,
  per-type registry entry).

Exports: `decompiled/DS3-1152-F-000FCE30`, `00222FF0`, `00223090`, `00223130`, `000F1A40`,
`000EFCB0`, `002B2E30`.

## 31. Every menu offscreen view, and a small set of native witnesses (STATIC; INFERRED where marked)

### Census

- The holder `CSEzOffscreenRend` (`FUN_140D644F0`) has one caller, the shared base
  `FUN_140D65DD0`. That base has three callers, the three classes of section 12.
- **So every menu offscreen view is a character (`ChrAsmModelRes`/`CSChrAsmModelIns`).** There
  is no weapon, item or object preview: the equipment, shop and item menus draw no 3D model.

| View (texture) | Size | Class, size-table row | `MenuOffscrRendParam` row → gparam (section 19) | Creator (owner) | Mode |
|---|---|---|---|---|---|
| `SYSTEX_Menu_StatusBody` | 560×768 | Player, row 4 | 2 → 11 | `PlayerStatusMenu` `FUN_140B71320` | continuous |
| `SYSTEX_Menu_StatusFace` | 512×256 | Face, row 18 (0x12) | 12 → 13 | `FUN_140B72980` from `PlayerStatusMenu` | snapshot |
| same texture, second face row | 512×256 | Face, row 19 (0x13) | 15 → 12 | `FUN_140B72980` from `FUN_140AB7280` (owner window not identified) | snapshot |
| `SYSTEX_Menu_ChrMakePreview`, `…NudePreview`, `…Preview2`, `…NudePreview2` | 784×784 | Player, rows 0..3 | 0 → 11, with row 1 → 10 as the second preset | `CharMakePreviewWindow` `FUN_140A84920` (from `FUN_140A85DF0`) | continuous |
| `SYSTEX_Menu_ChrMakeGrid00..14` | 128×128 | Face, rows 0..14 | 10 → 10 | `FUN_140B20070(slot, row, appearance)`, from the character creation code `FUN_140B29580`; stored at `14478D940[slot]` | snapshot, start delay `slot*2` |
| `SYSTEX_Menu_KickFace00..02` | 128×128 | Face, rows 15..17 | 11 → 15 | `FUN_140B598A0`, from `FUN_140B44320`; appearances of session players (`FUN_1408D9BB0([14477FDB8], …)`) | snapshot |
| `SYSTEX_Menu_SignFace` | 128×128 | Face, row 21 (0x15) | 14 → 15 | `SignDialog` `FUN_140BB76F0` | snapshot |
| `SYSTEX_Menu_Profile00..09` | 128×128 | Prof, rows 0..9 | 20 → 14 | `TitleTopDialog` `FUN_140AE5B70`; requested by `FUN_140ADEF50` (Load Game) | snapshot, start delay `slot*2` |
| `SYSTEX_Menu_RankingBody` / `RankingFace` | 560×768 / 512×256 | Player row 5 / Face row 20 | 3 → 11 / 13 → 10 | **no constructor call found** | INFERRED unused |

- "Snapshot" means the creator calls `FUN_140D67560`: one frame per request (section 24).
  "Continuous" views never call it.

### What is shared with the Status view (STATIC)

- **Scene and mask:**
  - `FUN_140D64C30` sets the composite mask `{0, 2}` for every view (section 24);
  - `FUN_140D690A0` (`STEP_Wait_Setup`, shared) sets the model flag `0x10` for every view, so
    parts are seeded with preset 4 → entity mask `{7, 2}`.
- **Materials and lights:**
  - the same SprjModelDispEntity draw chain and inputs (section 26);
  - the same `default.gparam` lookup by the row's id (section 27), only with a different id.
- **Readiness:**
  - the FaceGen gate for every view whose model has a head (section 25);
  - the same holder teardown (section 28).

### Where views leave the Status schema

| Difference | Views | Detail |
|---|---|---|
| **Pipeline descriptor** | Player (size-table flag 1) | own descriptor: **shadow sub-scene**, flags `0x400852201F` (section 25). Face and Prof use the default `0x8422014`, without shadow |
| **Camera construction** | Player / Face / Prof | `FUN_140D6A720` (two presets, rows `+0x14`/`+0x18`) / `FUN_140D698C0` → `FUN_140D69EE0` / `FUN_140D6B8D0` → `FUN_140D6BC20` (section 21) |
| **Interactive camera** | ChrMakePreview | row 0 carries the limits `+0x1C..+0x28` (2.2, 2.8, −89°, 89°: zoom and pitch range); the second preset (row 1) is a face close-up (1.55 m target, fov 16.5°) |
| **Different part set** | `NudePreview*` | INFERRED: the same model with equipment removed (the `Nude` names); same pipeline |
| **Model source** | ChrMakeGrid | templates from character creation (`[1447572B8]+0x10+0x2B0`, `FUN_140D69BE0(…, flag)`), not the save |
| | KickFace, SignFace | other players' appearances, from the session and the sign; **online only** |
| | Profile | the ten save slots (section 29) |
| **Many at once** | ChrMakeGrid (15), Profile (10) | staggered starts, ten to fifteen ChrAsm builds and FaceGen textures at the same time |
| **Gparam id** | per row | 10, 11, 12, 13, 14, 15: same file, different light, fog and env values (section 27 dump) |
| **Effects or extra scenes** | none | no view adds an effect, a map scene or another model class |

### Native witnesses (smallest set covering every variant)

| # | Witness (offline) | Covers |
|---|---|---|
| 1 | **Status** (done) | Player with shadow, continuous body + Face snapshot (512×256) |
| 2 | **Character creation, body preview** | Player 784², two camera presets, interactive zoom and pitch, `NudePreview` part set, gparam 11 and 10 |
| 3 | **Character creation, face grid** | 15 Face snapshots at 128² at once, template appearances, gparam 10, FaceGen under load |
| 4 | **Load Game list** | 10 Prof snapshots, gparam 14, teardown on game start (sections 28 and 29) |
| — | Kick and sign faces | same Face mechanism as 3 with gparam 15; need a multiplayer session, so they are not offline witnesses |

Exports: `decompiled/DS3-1152-F-00B20070`, `00B598A0` (plus `00D65DD0`, `00D6A3B0`, `00D69770`,
`00D6B780`, `00B72980`, `00ADEF50` from earlier passes).

## 32. Character creation: preparing witnesses 2 and 3 (STATIC; INFERRED where marked)

### Windows and their views

| Window (class) | Created by | Views it owns | Released by |
|---|---|---|---|
| **`AppMenu::CharMakePreviewWindow`** (vtable `1428BEDF8`, 0xCB0 bytes) | factory `FUN_140A85DF0` → constructor `FUN_140A84920` | **four `CSMenuPlayerModelRend`, size-table rows 0..3**, kept at `window+0xA30..+0xA48` | destructor `FUN_140A85960`: the four views go to the deferred queue (`FUN_1408190B0([14477D778], view)`) |
| **`FaceSelectDialog`** (anonymous namespace, vtable `1428E2958`) | `FUN_140B29580`, from three sites `FUN_140B17E90`, `FUN_140B18750`, `FUN_140B1AA00` | **fifteen `CSMenuFaceModelRend`, rows 0..14**, global `14478D940[0..14]`, built by `FUN_140B20070(slot, row, appearance, flag)` | destructor `FUN_140B1E630` → `FUN_140B21F70`: the fifteen views go to the same deferred queue |
| `CharMakeTopDialog`, `CharMakeCommandSelectDialog`, `CharMakeIconSelectDialog` | menu stack | none | — |

- **Menu structure:** the top menu and its entries come from the regulation params
  `CharMakeMenuTopParam` (313 rows of 32 bytes) and `CharMakeMenuListItemParam` (219 rows).
  - Dumps: `evidence/I-0067/queries/charmake/`.
  - Row field 0 is the entry kind (1 icon/list, 2 and 3 value entries, 10 header, 11
    separator); the text ids are `420xxx`/`423xxx`.

### What loads, in order (STATIC)

1. **Opening the creation screen builds the preview window** and its four views at once:
   - `Preview` (row 0, nude flag `+0x918 = 1` meaning equipped), `NudePreview` (row 1, 0),
     `Preview2` (row 2, 1) and `NudePreview2` (row 3, 0), via `FUN_140D6B130`;
   - each takes the working character's appearance (`[[1447572B8]+0x10]`, `FUN_1405A6A20`),
     its flags `+0xAA`/`+0x10C` and its equipment `+0x2B0` (`FUN_140D6A8A0`);
   - each gets a **request without snapshot and without start delay**.
   - **All four views then run continuously**: four 784² Player pipelines, each with its
     shadow sub-scene, listed every frame.
   - `FUN_140A510C0(window+0x9F0, mode)` selects which of the four textures the Scaleform
     page shows. Mode is 1..4, from `+0xA50/+0xA54/+0xA58` (INFERRED: equipped or nude, and
     preset 1 or 2).
2. **Nude toggle.** `FUN_140D6A8A0` with `+0x918 == 0` zeroes the twelve equipment slots
   (`FUN_140869EB0(…, i, 0)`, i = 0..11) and installs the default bare parts from
   `[14478CC88]` (`FUN_140A2CEE0(FUN_140E398F0(i))`).
   - The nude views are separate objects; switching only changes the displayed texture. All
     four keep rendering.
3. **Presets.** Each Player view reads two `MenuOffscrRendParam` rows:
   - row 0: body, target 0.85 m, distance 2.66, fov 40°;
   - row 1: face close-up, target 1.55 m, distance 1.2, fov 16.5°.
   - `FUN_140D6A720` picks the preset with `+0x88C`. The angles and distance are stored per
     preset at `+0x900..+0x914`.
   - INFERRED: the appearance sub-menus switch to the close-up.
4. **Interactive camera.** Row 0 carries the limits `+0x1C..+0x28`: distance 2.2..2.8, pitch
   −89°..89°. The window updates the view's angles and distance each frame within them
   (INFERRED: from the stick or mouse), then `FUN_140D6A720` rebuilds the camera
   (section 21 formula).
5. **Face grid.** When a face-selection entry opens, `FUN_140B29580` builds `FaceSelectDialog`
   and, for each of up to 15 templates (`param+0xE90` count), calls `FUN_140B20070(slot, row,
   template appearance, flag)`:
   - the template comes from `[[1447572B8]+0x10]+0x2B0` with `FUN_140D69BE0(…, flag)`;
   - request **with snapshot** (`FUN_140D67560`) and **start delay `slot*2` frames**
     (`FUN_140D668A0`).
   - The fifteen ChrAsm builds, part loads and FaceGen textures therefore overlap. Their
     single-frame renders are spread over about 30 frames.
6. **Readiness.** Every view waits for its parts and its head's FaceGen texture before its
   first render (section 25). The grid shows the last image of each.

### Input path (INFERRED from the game's menu flow, to confirm on screen)

- **Body preview:** Title, press a button, **New Game**. The creation screen opens on the top
  dialog with the preview window: witness 2 is on screen immediately.
  - Moving between top entries (gender, origin, gift) changes the equipment and the displayed
    texture.
  - The appearance entries switch to the face close-up.
- **Face grid:** from the top dialog, open **Appearance**, then the face or base template
  entry that shows a grid of faces: that is `FaceSelectDialog`, witness 3.
  - Three code sites open it (`FUN_140B17E90`, `FUN_140B18750`, `FUN_140B1AA00`), so up to
    three entries share the grid.
  - Runtime check: a break on `FUN_140B29580` names the entry.

### Native risk points

| Risk | Why |
|---|---|
| **Four continuous 784² views with shadow** while the screen is open | four Player pipelines (section 25 descriptor) listed every frame, only one displayed |
| **Fifteen Face snapshots plus four Player views at once** | about 19 ChrAsm models, their part files and 19 FaceGen textures created through the direct device path (section 25) in a few frames; staggered starts, one render each |
| **Nude toggle** | equipment zeroed and default parts installed: a new part load and model rebuild in `STEP_Wait_Play` (rebuild on appearance change, section 15), without leaving the list |
| **Two presets** | different camera rows (fov 40° against 16.5°) and the same gparam id 11/10 pair (section 19). The view, not the scene, changes |
| **Interactive camera** | per-frame camera rebuild within limits; the native camera must follow `FUN_140D6A720` each frame, not only at set-up |
| **Grid closing while builds are pending** | `FUN_140B21F70` queues views that may still be loading or waiting on FaceGen; their teardown must tolerate unbuilt models (section 28 order) |

### Leaving creation and starting the game

1. Closing the face grid: `FaceSelectDialog` destructor → `FUN_140B21F70` → 15 views to the
   deferred queue.
2. Confirming the character: the creation windows close.
   - `CharMakePreviewWindow` destructor `FUN_140A85960` → four views to the deferred queue;
   - then its sub-objects (`FUN_140A85A20(window+0xA70)`, …) are released.
3. **Each queued view is destroyed later** by the deferred queue `[144964930]` with the order
   of section 28:
   - model and modifiers, unlist, scene graph;
   - composite to the deferred queue and pipeline to the GX retirement list;
   - name, proxy.
   - This runs while the new game loads. It is the same pattern as Load Game (section 29),
     with up to nineteen views instead of ten, four of them Player views with shadow.

Exports: `decompiled/DS3-1152-F-00A84920`, `00A85DF0`, `00A85960`, `00B1E630`, `00B21F70`,
`00D6B130`, `00B29580` (plus `00B20070`, `00D6A8A0` from earlier passes).

## 33. The 2D menu layer: Scaleform, its D3D11 backend, offscreen textures in the interface, fades (STATIC + OBSERVED from I-0054/I-0055; INFERRED where marked)

Reused: the pilot's UI-INGRESS (HAL constructor `FUN_141F72680`, factory `FUN_140E5B250`, mixed
initializer `FUN_141F75920`, 1,316 programme selectors and 603 VS/PS of I-0038, samplers, CPU
geometry and images) and section 16 (the RendManImp draw). This section adds where Scaleform
sits in the frame, its states, the `SYSTEX_Menu_*` path and the remaining direct D3D11 calls.

### Backend: Scaleform's own D3D11 HAL on GX's device and immediate context (STATIC)

- **Not GX.** `SprjScaleformSystem` setup `FUN_140E5B250` builds a Scaleform render HAL
  (`FUN_141F72680`, 0x3B5C0 bytes, vtable `143B4DD40`) with a `SprjScaleformThreadCommandQueue`
  (`system+0x978`), keeps it at `system+0x980` and calls its init (`vt+0x38`, `FUN_141F75920`)
  with:
  - the **`ID3D11Device`** of GX (`FUN_1419264F0([144961AB0])`) and the **GX main context's
    `ID3D11DeviceContext`** (`[[GX+0x10]+8]`);
  - the calling thread id and config `0x100`.
- In the init the HAL takes the immediate context from the device (`GetImmediateContext`,
  device `vt+0x140`) and records whether the given context is deferred (`GetType`, `vt+0x380`,
  at `HAL+0x3AFA0`). With the GX main context it is immediate: I-0055 saw every interface draw
  on the immediate context.
- The GFx loader (`FUN_140E527B0`, kept at `system+0x988`) receives the states: file opener
  (`SprjScaleformFileOpener`), log, a Win32 font provider (`GetDC(0)`), AS2 support, a
  translator (state `0x28`) and **`SprjScaleformImageCreator`** (vtable `14298ED38`), built on
  the HAL's texture manager (`HAL vt+0x128`).
- So every Scaleform draw is a **direct D3D11 call by the HAL**, outside GX's recording, on the
  same immediate context. GX only prepares the targets before and re-applies its state after.

### Where it draws in the frame (STATIC; OBSERVED order)

`RendManImp` draw `FUN_140DFD270` (section 16), after the scene list:

1. 2D layer `+0x30` (with the stage camera hook `FUN_140E00680`), then layer `+0x38`;
2. screen-capture hook 1;
3. **Scaleform**: `FUN_140E4DA30([144798EF8])` → `FUN_140E5B940(system)`, unless
   `RendManImp+0x90` (writer not found);
4. layer `+0x40` (fade plates, section 6), also skipped when `+0x90` is set;
5. screen-capture hook 2, then layers `+0x48`, `+0x50..+0x68`.

The 2D layers are FD4 Ez draw lists (`FUN_140D637F0`, double-buffered, 128 KB), replayed by
`FUN_1423306F0` → `FUN_14237D5D0` through GX and its common states. Only step 3 bypasses GX.

**`FUN_140E5B940`, per frame:**

1. **Begin** (`FUN_140E60E00`; queued as `140E60FF0` on the GX command queue
   `[GX+0x398+[GX+0x3B8]*8]` with `FUN_1400CEEB0` when the frame context `+0x21F3` is 0), only
   if a HAL and at least one movie exist:
   1. **stencil reset** `FUN_1400CE7D0` through GX: depth-stencil view only (the frame's main
      depth-stencil `[[GX+0x88]+0xE8]`), common blend `+0x198` (colour writes off),
      depth-stencil `+0x200` (stencil on, write mask 0xFF, pass REPLACE, reference 0), one
      4-vertex strip (OBSERVED: `GXSimple_Color_F`, depth-stencil 3 only);
   2. GX state flush (`FUN_1400C5C60`), then context `vt+0x378` (`Flush` in the
      `ID3D11DeviceContext` order, `FUN_141928190`);
   3. **`OMSetRenderTargets(1, LDR RTV, main DSV)`** directly (`FUN_141920F40`), with GX's
      shadow copy updated (`+0x4F8`, `+0x538`, dirty `+0x59D`). The RTV is
      `[[GX+0x88]+0xA8+[GX+0xB88]*8]+0x70`: OBSERVED the 8-bit LDR target (`R8G8B8A8` at the
      render resolution, I-0054), not the back buffer;
   4. HAL `vt+0x58` (`FUN_141BD9FC0`, INFERRED `BeginFrame`: texture-manager and mesh-cache frame begin).
2. **Movies.** Under lock `system+0xC38`, the movies of `system+0x9D8` (count `+0xAE0`) whose
   flag `+0x8C` is set are taken, flag cleared, and sorted ascending by `+0x20` (at most 32).
   - For each: `FUN_140E52EE0` (queued as `140E61020`). Under the movie's locks, if `+0xA0`, it
     captures the movie's display tree (`FUN_141BD6600`, INFERRED `NextCapture`), has the HAL
     draw it (`FUN_141BDA3A0(HAL, …)`, INFERRED `Display`), then runs `FUN_140E53810`.
3. **End**: HAL `vt+0x60` (`FUN_141BDAAC0`, INFERRED `EndFrame`; queued as `140E61000`).
4. Back in `FUN_140E4DA30`: when the `SprjGraphicsImp+0xB8` mode changes, GX re-binds its
   render target and depth (`FUN_1400C5B00`) or resets its target state (`FUN_140206C80`).
   - Role of the mode not read; `+0xBC/+0xC0` select it.

- **OBSERVED, HUD frame (I-0054/I-0055):** `GXSimple_Color_F` (stencil reset), about 80
  Scaleform draws into LDR target 4 with depth-stencil 3 (57 indexed, 20 plain, 3 instanced),
  two Scaleform filter targets cleared and drawn, then **`GXSimple_Single2DTexture_F` copies
  LDR → back buffer**, then Present.
- **So the interface is drawn after tone mapping and post filters, in 8-bit, before the copy
  to the back buffer.** The fade plates of layer `+0x40` come after it and cover it.

### Inside the HAL, per scene (STATIC)

- **Scene begin** (`FUN_141F730A0`, HAL `vt+0x78`, INFERRED `BeginScene`):
  - shader-manager begin (`FUN_141F803E0`);
  - `FUN_141F80350` unbinds the CS, DS, GS and HS shaders (context `+0x228`, `+0x200`,
    `+0xB8`, `+0x1E0`);
  - `IASetIndexBuffer(null)`, `IASetVertexBuffers(0 buffers)`, topology TRIANGLELIST;
  - with a deferred context only, re-binds its target.
- **Default target.** The HAL adopts what GX bound: `OMGetRenderTargets` (`FUN_141F79AC0`,
  `FUN_141F796E0`), with the texture description from `GetResource`/`GetDesc`.
  - The DSV format decides stencil and depth support: formats 20 (D32_FLOAT_S8X24) and 45
    (D24_UNORM_S8) give both; 40 (D32_FLOAT) and 55 (D16) give depth only.
- **Scene end** (`FUN_141F75080`, HAL `vt+0x80`, INFERRED `EndScene`): `FinishCommandList` only with a deferred context; on its
  render thread it processes its queue (`HAL vt+0x70`).

**Blend states** (`FUN_141F79850`): 41 `CreateBlendState`.

- 0..19 are one per Scaleform blend mode (INFERRED names, library enum order) with blending on
  and write mask RGBA.
- 20..39 are the same with SrcBlend `SRC_ALPHA` replaced by `ONE` (premultiplied source).
- 40 is blending off with write mask 0 (masks drawn into stencil only).
- Mode table `1446E8F40`, D3D maps `143B4E150` (ops) and `143B4E160` (factors):

| # | Mode (INFERRED) | Colour: op, src, dst | Alpha: op, src, dst |
|---|---|---|---|
| 0, 1 | None, Normal | ADD, SRC_ALPHA, INV_SRC_ALPHA | ADD, ONE, INV_SRC_ALPHA |
| 2 | Layer | ADD, SRC_ALPHA, INV_SRC_ALPHA | ADD, SRC_ALPHA, INV_SRC_ALPHA |
| 3 | Multiply | ADD, DEST_COLOR, INV_SRC_ALPHA | ADD, ONE, INV_SRC_ALPHA |
| 4 | Screen | ADD, INV_DEST_COLOR, ONE | ADD, ONE, INV_SRC_ALPHA |
| 5, 6, 7, 13, 14 | Lighten, Darken, Difference, Overlay, HardLight | ADD, ONE, ZERO | ADD, ONE, ZERO |
| 8 | Add | ADD, SRC_ALPHA, ONE | ADD, ONE, ONE |
| 9 | Subtract | REV_SUBTRACT, SRC_ALPHA, ONE | ADD, ONE, INV_SRC_ALPHA |
| 10 | Invert | ADD, INV_DEST_COLOR, INV_SRC_ALPHA | ADD, SRC_ALPHA, INV_SRC_ALPHA |
| 11 | Alpha | ADD, ZERO, ONE | ADD, ZERO, SRC_ALPHA |
| 12 | Erase | ADD, ZERO, ONE | ADD, ZERO, INV_SRC_ALPHA |
| 15 | Overwrite | ADD, ONE, ZERO | ADD, ZERO, ONE |
| 16 | OverwriteAll | ADD, ONE, ZERO | ADD, ONE, ZERO |
| 17 | FullAdditive | ADD, ONE, ONE | ADD, ONE, ONE |
| 18 | FilterBlend | ADD, SRC_ALPHA, INV_SRC_ALPHA | ADD, ONE, INV_SRC_ALPHA |
| 19 | Ignore | ADD, ZERO, ONE | ADD, ZERO, ONE |

**Depth-stencil states** (`FUN_141F79D40`): 8 `CreateDepthStencilState` from `1446E8E20` (9
ints each: depth test, depth write, stencil, colour write, depth func, stencil func, pass, fail,
depth-fail). Read and write masks are 0xFF, back face = front face. Applied by `FUN_141F795B0`
(`OMSetDepthStencilState(state, ref)`; a colour-write change re-applies the blend mode).

| # | Mode (INFERRED) | Depth | Stencil | Colour |
|---|---|---|---|---|
| 0 | Invalid | sentinel (all 2) | | |
| 1 | Disabled | off | off | on |
| 2 | StencilClear | off | ALWAYS, pass REPLACE | off |
| 3 | StencilClearHigher | off | LESS_EQUAL, pass REPLACE, fail KEEP | off |
| 4 | StencilIncrementEqual | off | EQUAL, pass INCR, fail KEEP | off |
| 5 | StencilTestLessEqual | off | LESS_EQUAL, KEEP | on |
| 6 | DepthWrite | write, ALWAYS | off | on |
| 7 | DepthTestEqual | test EQUAL, no write | off | on |

"Colour off" is realised with blend state 40. Masks therefore use the frame's main stencil,
reset to 0 by the `GXSimple_Color_F` quad before the first movie.

**Other states and resources** (STATIC; pilot sections cited):

- **Rasterizer** (`FUN_141F79EC0`): 3 states. Entries 0 and 2 are SOLID, entry 1 WIREFRAME;
  all have cull NONE and depth clip on.
- **Samplers** (texture manager `FUN_141F82AA0`, vtable `143B4E870`): 4 = {POINT, LINEAR} ×
  {WRAP, CLAMP}, W CLAMP, LOD bias −0.75, MaxLOD FLT_MAX.
- **Constant buffers** (`FUN_141F799E0`): 8 × 0xF00 bytes, DYNAMIC, CPU write. Filled per draw
  by `FUN_141F80640`: `Map` WRITE_DISCARD, `Unmap`, `VS/PSSetConstantBuffers`, `VSSetShader`,
  `PSSetShader`, `IASetInputLayout`.
- **Shaders:** Scaleform's embedded DXBC (603 VS/PS, 1,316 selectors; I-0038), created by
  `FUN_141F81A90` (`CreateVertexShader`, `CreatePixelShader`, `CreateInputLayout`).
- **Geometry:** HAL mesh cache (`HAL+0x3AFF0`; pilot `1F7D0A0`/`1F7E360`). OBSERVED dynamic
  buffers of 1.7 MB (vertices) and 1.4 MB (16-bit indices), mapped by the HAL (no GX path).
  - `IASetVertexBuffers` in `FUN_141F7BCB0` (strides `143B4E144`).
- **Textures** (texture manager):
  - creation `FUN_141F83DC0` (`CreateTexture2D`, SRV, RTV) and depth-stencil surfaces
    `FUN_141F83B50` (`CreateTexture2D`, DSV);
  - updates `FUN_141F84DA0` (Map, CPU conversion) and `FUN_141F84BA0`
    (`CopySubresourceRegion`), upload `FUN_141F85430`;
  - binding `FUN_141F83440`/`FUN_141F84A70` (`PSSetShaderResources`, `PSSetSamplers`).
- **Filter targets** (Flash `BlurFilter`, glow, drop shadow; `flash.filters.BlurFilter` at
  `143B16F88`):
  - render target push `FUN_141F7BD30` (`ClearRenderTargetView`, `OMSetRenderTargets`) and
    viewport `FUN_141F7BFF0` (`RSSetViewports`);
  - the filter passes `FUN_141F7A130/7A590/7ADC0/7B1B0` (INFERRED from their HAL calls);
  - OBSERVED: two such targets on the HUD frame.

### Offscreen views in the interface: name → GX texture → Scaleform texture (STATIC; INFERRED where marked)

1. The view registers its `CSOffscreenGxTexture` proxy under `SYSTEX_Menu_*` in the texture
   registry `[144792F58]` (section 12, `FUN_140D4A120`).
2. The menu movie asks for **`img://<name>`**.
   - INFERRED from the image-creator path: the protocol string is at `143AD7224`, and the text
     form `<img src='img://%s'>` is at `1428B6390`.
   - The `.gfx` files themselves were not read.
   - Some pages switch between prepared images from C++: `FUN_140A510C0(window+0x9F0, mode)`
     calls the page object's `vt+0x148` with the mode (INFERRED `gotoAndStop`, ChrMake
     preview, section 32).
3. `SprjScaleformImageCreator` slot `+0x10` **`FUN_140E4E790`** (INFERRED `LoadProtocolImage`):
   - widens the URL (`%hS`, `14298D0B8`) and strips the protocol (`FUN_140E4E660`);
   - asks the Scaleform texture repository **`[1447983B8]`** (`ScaleformTexRepositoryImp`, map
     `+0x60` by case-folded name, `FUN_140E48280`).
   - Hit: the image is referenced again (`vt+8`) and returned.
4. Miss: `FUN_140E48980(repo, name, texture manager)`:
   - looks the name up in **`[144792F58]`** (`FUN_140D4ED20`) and takes the entry's GX texture
     `+0x60`;
   - `FUN_140E49320` (under lock `[144793D28]`) asks that GX texture for its object
     (`vt+0x20`) and passes **that object's `+0x18`, the D3D11 texture (INFERRED)**, to the
     texture manager `vt+0xD8` (INFERRED: Scaleform texture from an existing D3D11 texture);
   - it wraps the result in a **`CSTextureImage`** (0x70 bytes, vtable `14298BDE8`, built by
     `FUN_140E478F0` with the texture's format, size `+0x38/+0x3C` and use 0x100) and stores it
     in the map.
   - If the name is not registered yet, nothing is created and nothing is cached. INFERRED: the
     movie shows no image until a later request.
5. The movie then samples it like any bitmap: a pixel shader with one of the 4 samplers, and
   the blend of the clip's mode (table above).
6. `CSTextureImage` destructor `FUN_140E4A1F0` removes the name from the repository
   (`FUN_140E49770`) and releases the Scaleform texture.

**Consequences for a native backend:**

- **The binding is made once per name and image, not per frame.** The UI keeps the D3D11
  texture object it got on the first request. INFERRED: it also keeps its own view of it and a
  COM reference.
- The view's colour target must therefore be **the same resource for the whole life of the
  image**. A view rebuilt under the same name is not seen while the old `CSTextureImage` lives
  (section 28 teardown: the name is unregistered from `[144792F58]`, not from `[1447983B8]`).
- The image's lifetime follows the movie (Scaleform reference counting), not the view.
  INFERRED: a teardown that frees the target while a page still references the image is the
  D3D11-tolerated case (the COM reference keeps it alive). A native resource needs an explicit
  reference from the UI or an ordered release.
- **The texture also reaches the HAL through `vt+0x20`/`+0x18`, not through the GX SRV slot.**
  A native GX texture proxy must answer that path with the native resource.

### Fades, background blur, other menu full-screen effects (STATIC)

| Effect | Mechanism | Path |
|---|---|---|
| **Fade screens** (`FadeScreen`, vtable `1428EF858`, ctor `FUN_140B49F50`) | an AppMenu window bound to the Scaleform clip `Fade` (`Fade/Root/ArtsIcon`, `Fade/Root/Stock`, `1428D8810`) | **Scaleform** (the fade is the movie's own animation) |
| **Fade plates** (`SprjFadeImp`, 7 plates, section 6) | full-screen quads, blend state 7, 1920×1080 virtual canvas | **GX**, layer `+0x40`, after Scaleform, so over the interface |
| Who sets plate colours | `FUN_140D0FD40(fade, plate, colour)` from about 30 sites: title and transition code (`14075E360`, `1407641B0`, …: plate 0), warps and events (`1408E9B20`, `1408F15F0`, …: plates 2 and 5), cut-scenes and debug (`140CCxxxx`: 3 and 4), `FUN_140D11530`/`140D116B0` (plate 2) | no AppMenu window sets a plate |
| `InGameChroma` (plate 6) | colour passed to the stage (`FUN_140E004F0`) | stage colour tint, not 2D |
| **Background blur** | **none in the engine for menus**. The blur families in the executable are YEBIS (Gaussian, motion blur, the GX feedback-blur scene `GXSgFeedBackBlurSceneImpl`, created by `FUN_1400C30D0`/`FUN_1400C3410`) and Scaleform's Flash filters | a Flash blur filters display objects of the movie into the HAL's filter targets. It cannot read the 3D scene. INFERRED: menu dimming is authored in the `.gfx` (translucent fills) |
| Old dialogs (`FrpgMenuDlgFadePlate`, `FrpgMenuDlgNowLoading`, `FrpgMenuDlgMovieDisplay`) | FD4 dialog classes (vtable `1427F3B98`, …) | draw path not read in this pass |
| Movies in menus | `CSMovieGxTexture` by name (section 9) | the same image-creator path, with the movie proxy registered under its name |
| Menu 2D extras | `FUN_140A3B720`/`FUN_140A3B990` (from `FUN_140A39560`, role not read) add rectangles to layer `+0x40` (`FUN_1423300E0`) | GX layer |

### Direct D3D11 calls left on this path (to replace in native)

| Where | Calls |
|---|---|
| Set-up `FUN_140E5B250` / HAL init `FUN_141F75920` | device and context handed to the HAL; `GetImmediateContext`, `GetType`; `CreateBlendState` ×41, `CreateDepthStencilState` ×8, `CreateRasterizerState` ×3, `CreateSamplerState` ×4, `CreateBuffer` ×8 (CBs), shader and layout creation (`FUN_141F81A90`), mesh-cache buffers; `OMSetRenderTargets` (initial bind) |
| Per frame, game side (`FUN_140E60E00`) | context `Flush` (`vt+0x378`), `OMSetRenderTargets(LDR, main DSV)` |
| HAL per scene and draw | `OMGetRenderTargets`, `GetResource`/`GetDesc`; `HS/DS/GS/CSSetShader(null)`; `IASetIndexBuffer`, `IASetVertexBuffers`, `IASetPrimitiveTopology`, `IASetInputLayout`; `VS/PSSetShader`; `Map`/`Unmap` (CBs, vertex and index buffers); `VS/PSSetConstantBuffers`; `PSSetShaderResources`, `PSSetSamplers`; `OMSetBlendState`, `OMSetDepthStencilState`; `RSSetState`, `RSSetViewports`; `DrawIndexed`, `Draw`, `DrawIndexedInstanced` (OBSERVED 57, 20, 3) |
| HAL filters and render targets | `CreateTexture2D`, `CreateShaderResourceView`, `CreateRenderTargetView`, `CreateDepthStencilView`; `ClearRenderTargetView`, `OMSetRenderTargets` |
| HAL texture updates (glyph cache, gradients) | `Map`/`Unmap`, `CopySubresourceRegion` |
| Offscreen views and movies → UI | `CSTextureImage` creation takes the D3D11 texture from the GX object (`FUN_140E49320`) |
| Deferred-context case | `FinishCommandList` (`FUN_141F75080`; not used with the GX main context) |

Through GX (already on the native GX side): the stencil-reset quad `GXSimple_Color_F`, the 2D
layers (fade plates), and the LDR → back-buffer copy `GXSimple_Single2DTexture_F`.

### Native contract in short

1. **Order:** scene and post, then layers `+0x30`/`+0x38`, then the **stencil reset to 0**,
   then Scaleform into the LDR target with the main depth-stencil, then layer `+0x40` (fade
   plates) and the later layers, then the copy to the back buffer.
2. **One native implementation of the HAL's responsibilities:** the 41 blend, 8 depth-stencil,
   3 rasterizer and 4 sampler states above as PSO and sampler choices, the 8 constant blocks
   per draw, CPU geometry, image updates, and filter targets with their own stencil.
3. **The `img://SYSTEX_Menu_*` resolution:** the name registry entry gives the native resource
   of the view's target, held by the UI image until Scaleform releases it.
4. **No GX state** may be assumed preserved across the UI. The original re-applies GX's
   bindings after the HAL through GX's dirty flags.

Exports: `decompiled/DS3-1152-F-00E5B250`, `00E527B0`, `00E4DA30`, `00E61000`, `00E4E790`,
`00E48280`, `00E48980`, `00E49320`, `00E4A1F0`, `000CE7D0`, `01F75920`, `01F79850`,
`01F79D40`, `01F79EC0`, `01F799E0`, `01F82AA0`, `01F730A0`, `01F75080`, `01F795B0`,
`01F796E0`, `01F79AC0`, `01F80350`, `01F80640`, `01F7BD30`, `01F7BFF0`, `01F83B50`,
`01F83DC0`, `01F84BA0`, `01BD9FC0`, `01BDAAC0`, `00B49F50`, `023306F0`, `00A510C0` (plus
`00E5B940`, `00E60E00`, `00E52EE0`, `01F81A90`, `00DFD270`, `00DFC850` from earlier work).
Raw table dumps and reference lists: `evidence/I-0067/queries/scaleform/`.

## 34. Open points of section 33, and every D3D11 path outside GX (STATIC; INFERRED where marked)

### Old dialogs (`FrpgMenuDlg*`): drawn through GX, in 2D layer `+0x40` (STATIC)

The old FD4 dialog system (`FrpgMenuSys` `142816848`, `FrpgMenuDialog`, 449 `FrpgMenuDlg*`
RTTI classes on the `EzMenu` library) does not touch D3D11 and is not Scaleform.

1. **Recording.** `FrpgMenuSys` slot `+0x28` `FUN_14075A880` calls `FUN_141A0D8E0(sys,
   [[RendManImp+0x88]+8])`. That runs each open dialog's draw (`vt+0x90`) with the drawing
   context **`FrpgMenuDc`** (vtable `142815DD8`, a subclass of `EzMenu::EzMenuDC`).
   - `RendManImp+0x88` is `NS_SPRJ::MenuRend` (`142743058`, constructor `FUN_1403A5EF0`); its
     `+8` is that `FrpgMenuDc`.
   - Its primitives (`FUN_140756FA0`, `140757140`, `140757290`, `1407573C0`, …) append records
     to a **`MenuDrawCommandBuffer`** (`142836D40`, constructor `FUN_1407F7960`). The records
     are function pointer plus data (for example `FUN_1407F7CD0` records `FUN_1407F8EC0`), in
     three rotating 256 KB stores.
   - Then `MenuRend+0x10 = 1` (frame ready).
2. **Replay**, in the RendManImp frame update `FUN_140DFDD20` (called from `SprjDrawStep`
   `FUN_140D55190`):
   - when the frame is ready, `FUN_1403A6030(MenuRend, [RendManImp+0x40])` points the command
     buffer at the **2D layer `+0x40`**, sets that list's blend state 7 and colours, and runs
     every record;
   - the records emit Ez draw primitives through `FUN_1423300E0`/`FUN_14232FFD0` (via
     `FUN_1407FABF0`/`1407FAFE0`), and text geometry through `FUN_141923340` (CPU);
   - `FUN_1403A6040` then rotates the stores.
3. **Draw:** layer `+0x40` is replayed by GX after Scaleform (section 33), in the same list as
   the fade plates.

**Native consequence:** the old dialogs are a GX 2D-layer client like the fade plates. Nothing
in them needs a D3D11 replacement once the Ez draw layers are native.

### `RendManImp+0x90` (STATIC)

- **Only the constructor writes it** (`FUN_140DFC850`: `MOV byte ptr [RSI+0x90], 0`).
- No other byte, word, dword or qword store to `+0x90` was found:
  - none in a function that reads `[144796298]`;
  - none in the RendManImp code range `140D50000..140E10000`;
  - no address of it taken with a LEA in those functions.
- Its two readers are the skips in `FUN_140DFD270` (Scaleform, layer `+0x40`).
- INFERRED: a development toggle ("hide 2D"), always 0 in this build. The native backend can
  treat both draws as unconditional.

### Scaleform HAL methods (vtable `143B4DD40`, 0x3C8 bytes)

Names marked CONFIRMED are taken from the method's own profiler marker: each method opens its
scope with `Events(vt+0x118)->Begin(…, "HAL::<name>")`, strings at `143AD6CF8..` and
`143B4E108..`. The other names are INFERRED from the body.

| Slot | Function | Name | Basis |
|---|---|---|---|
| +0x00 | `141F72E00` | destructor | |
| +0x08 | `141F73630` | create a render target on a texture `(Texture*, bool withDepthStencil)` (INFERRED) | texture manager `vt+0x28`, depth-stencil surface `vt+0x38` |
| +0x10 | `141F73750` | create a temporary render target (INFERRED) | texture manager `vt+0x30` |
| +0x18 | `141BDDCD0` | `SetRenderTarget(target, bool)` (INFERRED) | calls `+0x20` with flags `0x11` |
| +0x20 | `141BDCBD0` | **`PushRenderTarget`** | CONFIRMED |
| +0x28 | `141F76450` → `141BDBB70` | `PopRenderTarget(flags)` (INFERRED) | pops the 0x340-byte target stack `HAL+0x1A8` |
| +0x30 | `141BDB5D0` | `IsInitialized` (INFERRED) | `HAL+0x58 & 1` |
| +0x38 | `141F75920` | `InitHAL(params)` | section 33 |
| +0x40 | `141F78F40` | `ShutdownHAL` (INFERRED) | releases device and context |
| +0x48 | `141F76460` | `PrepareForReset` (INFERRED) | base, then pops the default target (`+0x278`) |
| +0x50 | `141F76D00` | `RestoreAfterReset` (INFERRED) | base, then recreates the default buffer (`+0x270`) |
| +0x58 | `141BD9FC0` | **`BeginFrame`** | CONFIRMED |
| +0x60 | `141BDAAC0` | `EndFrame` (INFERRED) | pair of `+0x58` in `FUN_140E5B940` |
| +0x68 | `141BDAE90` | frame statistics reset (INFERRED `FinishFrame`) | clears the counters of `vt+0xC8` |
| +0x70 | `141F79370` | executes recorded command lists (deferred context only) | `HAL+0x3AFD8` |
| +0x78 | `141F730A0` | **`BeginScene`** (D3D1x) | CONFIRMED (`"HAL::BeginScene-SetState"`; base `141BDA0A0` carries `"HAL::BeginScene"`) |
| +0x80 | `141F75080` | `EndScene` (INFERRED) | pair of `+0x78` |
| +0x88 | `141BDA700` | queue a render item (INFERRED `Draw(RenderQueueItem)`) | ring at `HAL+0xFA8` |
| +0x90 | `141F731D0` | **`ClearSolidRectangle`** | CONFIRMED |
| +0x98 | `141BDAFA0` | `GetContextNotify` (INFERRED) | returns `HAL+0x10`, passed to `NextCapture` (`FUN_140E52EE0`) |
| +0xA0..+0x110 | getters | fields `+0x268`, `+0x260`, `+0x288`, `+0xFD0`, `+0xFC8`, `+0x3B228`, `+0x218`, `+0x248`, `+0x240`, `+0x2A0`, `+0xD48`, `+0xF20`, `+0x250`, `+0x258` | one-line returns |
| +0x118 | `141F75430` | `GetEvents` (profiler) | returns `HAL+0x3B350`; every marker above goes through it |
| +0x120 | `141BDB2D0` | copies the 6-float view rectangle at `+0x270` | |
| +0x128 | `141F757D0` | **`GetTextureManager`** | used by the image creator (section 33) |
| +0x130 | `141F75730` | `GetMeshCache` | returns `HAL+0x3AFF0` (pilot) |
| +0x138..+0x178 | small methods | including `+0x150` (raster mode ≠ 2 supported), `+0x160` (`HAL+0x5C`), `+0x168`/`+0x170`/`+0x178` (stereo / display-pass settings at `[HAL+0x258]+0x280..`, INFERRED) | |
| +0x180 | `141BDDC90` | set display pass `HAL+0x5C` (INFERRED stereo eye) | |
| +0x188 | `141BDA7A0` | stereo-aware display wrapper (pass 1, then pass 2) | |
| +0x190 | `141F741F0` | **`DrawProcessedPrimitive`** | CONFIRMED (the pilot's observer address) |
| +0x198 | `141F73840` | **`DrawProcessedComplexMeshes`** | CONFIRMED |
| +0x1A0 | `141BDC7F0` | **`PushMask_BeginSubmit`** | CONFIRMED |
| +0x1A8 | `141BDABD0` | `EndMaskSubmit` (INFERRED) | between push and pop |
| +0x1B0 | `141BDB9C0` | **`PopMask`** | CONFIRMED (`"Scaleform::Render::HAL::PopMask"`) |
| +0x1B8 | `141F76490` | **`ShaderHAL::PushBlendMode`** | CONFIRMED |
| +0x1C0 | `141F75C00` | **`ShaderHAL::PopBlendMode`** | CONFIRMED |
| +0x1C8 | `141BDBFC0` | blend or filter primitive handling (role not read) | |
| +0x1D0 | `141BDC4E0` | **`PushFilters`** | CONFIRMED |
| +0x1D8 | `141BDB800` | `PopFilters` (INFERRED) | |
| +0x1E0 / +0x1E8 | `141BDD010` / `141BDBEC0` | push / pop on the stack `HAL+0x1C0` (INFERRED user matrix or view) | |
| +0x1F8..+0x228 | `141F74900`, `141F74550`, (`141F74630`), `141F74CA0`, `141F74640`, `141F74D80`, `141F74740` | **`DrawableCxform`, `DrawableCompare`, (forwarder to `+0x210`), `DrawableMerge`, `DrawableCopyPixels`, `DrawablePaletteMap`, `DrawableCopyback`** | CONFIRMED (except the forwarder) |
| +0x250 | `141BDE1E0` | **`beginDisplay`** | CONFIRMED |
| +0x258 | `141BDE800` | display end (INFERRED `endDisplay`) | |
| +0x260 | `141F7BFF0` | viewport update (`RSSetViewports`) | |
| +0x270 | `141F79AC0` | create the default render buffer from the bound RTV/DSV (INFERRED `createDefaultRenderBuffer`) | |
| +0x280 | `141F7BD30` | bind and clear a target (`ClearRenderTargetView`, `OMSetRenderTargets`) | |
| +0x288 | `141F79550` | colour-write and blend re-application | |
| +0x2A0 | `141F7BA60` | shader programme choice by fill (0x17 / 0xC) | |
| +0x2B0 | `141F795B0` | **`applyDepthStencilMode`** | CONFIRMED |
| +0x2B8 | `141F796E0` | depth/stencil capability check (DSV format) | |
| +0x2C0 | `141F7ADC0` | **`drawMaskClearRectangles`** | CONFIRMED |
| +0x2F8 | `141F796B0` | raster mode (`RSSetState(raster[i])`) | |
| +0x300 | `141F7BCB0` | vertex buffer binding (`IASetVertexBuffers`) | |
| +0x310 / +0x318 | `141F7BE20` / `141F7BF00` | primitive and batch draws (pilot: batch draw) | |
| +0x2C8..+0x2F0, +0x308, +0x320..+0x370 | other methods | filter passes (`141F7A590`, `141F7B150`, `141F7A130`, …), not named | |

The method that draws a captured movie tree, `FUN_141BDA3A0` (called directly from
`FUN_140E52EE0`), is INFERRED `Display(MovieDisplayHandle)`. It reads the tree's viewport and
calls `beginDisplay`.

### Every D3D11, DXGI and D3DX path, and which ones are outside GX (STATIC)

**Method.**

- **Imports:** every D3D-family import and its callers.
  - `D3D11.DLL::D3D11CreateDevice`, `DXGI.DLL::CreateDXGIFactory`;
  - `D3DX11_43::D3DX11SaveTextureToFileW`, `D3DX9_43::D3DXCompileShader`;
  - `D3DCOMPILER_43::D3DReflect` and `D3DGetBlobPart`;
  - `D3D9.DLL::D3DPERF_GetStatus`.
  - There is no `Direct3DCreate9`, D3D12 or OpenGL import.
- **COM interface IDs** present in the image and their users (`iids.ps1` scan):
  `ID3D11Texture2D` `143A3A2B0`, `ID3D11InfoQueue` `143A3A320`, `IDXGIFactory` `1429B6A78`,
  `IDXGIDevice` `143A3A2C0`, `ID3D11ShaderReflection` `142BDB468`.
- **Device and context hand-offs:** where code outside GX receives GX's device or context.
- **Direct callers of GX's D3D11 layer:** every call into GX's D3D11 wrapper functions
  (`141920180..141933xxx`) from outside GX's own code ranges (`140000000..1402FFFFF`,
  `142130000..1423FFFFF`, `1403A4xxx..1403A5xxx` helpers called from there).

Wrapper → D3D11 method (context vtable offset):

| Wrapper | Method | Wrapper | Method |
|---|---|---|---|
| `141920180` | ClearState | `1419206E0` | VSSetShader |
| `141920200` | ClearRenderTargetView | `141920660` | VSSetConstantBuffers |
| `141920300` | UpdateSubresource | `141920DA0` | PSSetShaderResources |
| `1419203F0` | IASetInputLayout | `141920D20` | PSSetSamplers |
| `141920420` | IASetPrimitiveTopology | `141920E20` | PSSetConstantBuffers |
| `141920460` | IASetIndexBuffer | `141920EA0` | PSSetShader |
| `1419204C0` | IASetVertexBuffers | `1419208A0` / `141920A50` / `141920C00` | HS / DS / GSSetShader |
| `141920CA0` | RSSetState | `141920CD0` / `141920CF0` | RSSetViewports / RSSetScissorRects |
| `141920ED0` | OMSetBlendState | `141920F00` | OMSetDepthStencilState |
| `141920F40` | OMSetRenderTargets | `141921370` | Draw |
| `141921860` / `141921940` | Map / Unmap | `141921AC0` | GetType |
| `141928190` | Flush | `1419264F0` | device getter |

Device side (STATIC):

- state creation `141925360`/`1419253B0`/`141925400`/`141925450`;
- buffer `1419250A0`;
- textures and views `1419256D0`, `141925A30`, `141925B10`, `141925BF0`, `1419281C0`
  (SRV), `141928420`;
- releases `1419322D0..141932C00`.

**A. Own D3D11/DXGI objects outside GX** (they replace nothing in GX and must each get a
native replacement):

| Path | Entry | What it does with D3D11 | Native |
|---|---|---|---|
| **Scaleform** (menus, HUD, text, Flash filters) | `FUN_140E5B250` → HAL | the whole renderer (section 33) | replace |
| **YEBIS 2 post-processing** (`PPFX::IGPUDevice` `142B598C8`) | GX `FUN_1400E2B20` → `FUN_1416C87D0(…, {GX main context}, …)` → `FUN_14170ACC0` → `FUN_1417074C0` | takes the GX main context `**(GX[2]+8)`, its device (`GetDevice`), own resources and states, **raw draws on the immediate context** (OBSERVED I-0061: the post chain's immediate draws); **`D3DReflect`** on its shaders (`FUN_1417715B0`) | replace (the pilot's host already carries the post chain, K-0433) |
| **Display-mode list** (`FUN_140EF9580`, from `FUN_140EF9090`) | `CreateDXGIFactory` → adapter 0 → output 0 → `GetDisplayModeList(R8G8B8A8_UNORM)` | no rendering; fills the settings' resolution list (`FUN_140EF9D80` per mode) | equivalent DXGI query (or the native adapter's) |
| Legacy `DLGR` D3D9 shaders (`DLVertexShader`/`DLPixelShader`, `FUN_1418EE2F0`) | `D3DXCompileShader` + `IDirect3DDevice9` `+0x2D8`/`+0x350` | **unreachable**: no D3D9 device can be created (no `Direct3DCreate9`) | none |

**B. GX-owned direct calls outside the command path** (part of GX's D3D11 layer; listed so the
native GX side covers them):

| Path | Function | Calls |
|---|---|---|
| Device creation | `FUN_14192D650` | `D3D11CreateDevice`, `IDXGIDevice`, `ID3D11InfoQueue` (debug) |
| DXGI factory, swap chain | `FUN_14192C220`, `FUN_141933780`, `FUN_141933B90`, `FUN_14192D360` | `CreateDXGIFactory`, `IDXGIDevice`; back buffer as `ID3D11Texture2D` |
| Screenshots to file (section 9) | `FUN_140205BA0`, `FUN_140223580` | **`D3DX11SaveTextureToFileW`** |
| Shader blobs | `FUN_141930420` | `D3DGetBlobPart` |
| PIX check | `FUN_14000A570` | `D3DPERF_GetStatus` (no rendering) |

**C. Game code that calls GX's D3D11 layer directly** (not through the GX scene or command
lists, so these sites must be redirected to native equivalents):

| Path | Functions | D3D11 work |
|---|---|---|
| **Bink movies** (`MVBinkGraphics` `143A9CDF8`, ctor `FUN_141ADFE30`) | `FUN_141AE0830` (init: states through `141925360..450`, buffer `1419250A0`), `FUN_141AE0BA0` / `FUN_141AE0E50` (`BinkAllocateFrameBuffers`, one texture per plane `1419256D0`), **`FUN_141ADFF10`** (per frame: `UpdateSubresource` of the Y/cR/cB(/A) planes, then a full state set and draw for the YUV→RGB pass), `FUN_141AE0660` / `FUN_141AE0570` (release) | textures, states, uploads and a YUV→RGB draw through the wrapper |
| Movie texture binding (`CSMovieGxTexture`, section 9) | `FUN_140EADE50` | resource type check (`1419281C0`), SRV description (`141928420`), size (`141924730`) of Bink's texture |
| Offscreen menu views | `FUN_140836C40` / `FUN_140836E40` (FaceGen face texture: create `1419256D0` and upload `141920300`, section 25), `FUN_140D64B90` / `FUN_140D64C30` (`ClearRenderTargetView`) | |
| Scaleform glue | `FUN_140E5B250`, `FUN_140E5A220`, `FUN_140E60E00` | `OMSetRenderTargets`, `Flush`, device getter (section 33) |
| GUI shader binding (`GXGuiShader_Tex1` / `GXGuiShader_Color`) | `FUN_140D10940` | IASetInputLayout, HS/DS/GSSetShader |
| Material/vertex helpers (role not read) | `FUN_140D3F7C0` (buffer `1419254F0`), `FUN_140D3FA10` (`IASetVertexBuffers`) | |
| Dynamic buffer upload on the context `[GX+0x60]` (role not read) | `FUN_14051B940` | Map/Unmap (`141921860`, `141921940`, `141921AC0`, `141922B40`) |
| Immediate geometry draws (role not read) | `FUN_14036F9E0`, `FUN_140385F70`, `FUN_140396F30`, `FUN_14039E900` | IASetPrimitiveTopology, IASetIndexBuffer, Draw |

**Not D3D:**

- the cursor is the Win32 cursor (`ShowCursor`, `SetCursor`, `LoadCursorW`, `Set/GetCursorPos`,
  cursor resource `Rsrc_Cursor_1`); Scaleform pages may also draw their own;
- debug menus (`FD4DebugMenuDraw`) and Ez debug drawing go through the GX Ez layers;
- `FSHKPlus::Graphics::FSHKPlusDC` (`143CD01B8`) is an abstract draw-callback interface (pure
  virtual slots `141FD0EAC`); no implementer with device access was found;
- input (`XInput`, `DirectInput8`) and audio (`BinkOpenXAudio2`) are not graphics.

**Runtime cross-check (OBSERVED, I-0060/I-0061).** On the immediate context per frame: only
YEBIS's post draws and Scaleform, besides the development adapter. Everything else runs in GX's
deferred command lists. Uploads (Map/Unmap, `UpdateSubresource`) also reach the immediate
context; Bink and the face texture belong to these.

**Open.**

- Roles of the four 0x1403xxxxx immediate draw functions, of `FUN_14051B940` and of the
  material helpers.
- The `+0x2C8..+0x370` HAL filter methods.
- Whether `FSHKPlusDC` has a runtime implementer (no RTTI found).

Exports: `decompiled/DS3-1152-F-0075A880`, `00756B60`, `007F7960`, `007F7CD0`, `003A5EF0`,
`003A6030`, `003A6040`, `00DFDD20`, `01A0D8E0`, `00EF9580`, `017074C0`,
`0170ACC0`, `01ADFF10`, `01AE0830`, `01AE0BA0`, `018EE2F0`, `00EADE50`, `00D10940`,
`0051B940`, `01BDA3A0`, `01BDA700`, `01BDAFA0`, `01F73630`, `01F73750`.
Scan script and raw lists: `evidence/I-0067/queries/d3d-paths/`.

## 35. Title → character creation: who destroys the Load Game portraits, the deferred queue, and quitting (STATIC; INFERRED where marked)

Pilot context (Reference1172 / Native1172):

- In the original, New Game destroys the 10 `CSMenuProfModelRend` before creation.
- In native mode `TitleTopDialog` survives until the game is closed. The close then faults in
  `FUN_140D126B0` (`CSFD4LocationBodyScaleModifier`, freed data; section 15).

### The only destroyer of the 10 portraits: `TitleTopDialog`'s destructor

```
TitleTopDialog::~ (FUN_140AD9820; deleting form FUN_140ADE080 = vtable 1428D39C8 slot +8)
  → FUN_140AED060: for i in 0..9:
        FUN_1408190B0([14477D778] DelayDeleteMan, 14478D680[i]); 14478D680[i] = 0
          → FUN_140D67580(view)                      // finish request +0x6CA = 1
          → lock DelayDeleteMan+0x10
          → FUN_1423559C0([144964930], view, CSDelayDeleter_Delete<CSMenuAsmModelRend> 144596A08, 6)
```

- The only other caller of `FUN_140AED060` is the portrait creation `FUN_140AE5B70`, which
  releases the previous set before creating a new one.
- **No `TitleTopDialog` destruction means no `+0x6CA` and no queuing**, so the portraits keep
  their location modifiers registered (section 15).

### The deferred queue `[144964930]`: `DelayDeleteStep`, task group 0x5E

- `DelayDeleteStep` (`SprjStepTask<DelayDeleteStep>` `1429B28A8`, step vtable `1429B2938`)
  is a child of `SprjSystemStep`.
  - It is created in `FUN_140EDE770` and registered with `FUN_140F0EB60(slot, 0x5E, step)`.
  - Its states: Init `FUN_140EE41E0` (creates `[144964930]` and `[14477D778]`), Update
    `FUN_140EE42B0`, Finish `FUN_140EE4140`.
- **Update** (once per frame):
  - `FUN_142355A10` decrements each entry's count (`+0x20`). When it goes below 0 it calls the
    deleter `vt+8(obj)`, which runs the real destructor `FUN_140D66280` (sections 15 and 28).
    A view is therefore destroyed on the 7th Update after it was queued.
  - Then `FUN_140819450` handles the DelayDeleteMan list.
- **Finish** (shutdown) frees the pending nodes **without calling their deleters**
  (`FUN_142355AC0`). At exit the original leaks still-queued objects; it does not destroy
  them.
- **Frame position.** The task-group name table (`1429BA588`, one UTF-16 name pointer every
  0x20; read from the file with `taskgroups.ps1`) gives the end of the frame:

| 0x50 | … | 0x58 | 0x59 | 0x5A | 0x5B | 0x5C | 0x5D | **0x5E** | 0x5F | 0x60 |
|---|---|---|---|---|---|---|---|---|---|---|
| GraphicsStep | … | DrawStep | DrawBegin | GameSceneDraw | AdhocDraw | DrawEnd | Flip | **DelayDeleteStep** | RecordHeapStats | FrameEnd |

- The queue therefore runs right after `Flip`. A native frame that stops at the draw or flip
  groups would never empty it.
- **The pilot measured that this is not the Native1172 cause:** the portraits are never
  queued.

### What closes `TitleTopDialog` at New Game (window lifecycle, not the title step)

- `TitleStep` (`1428651B0`) does not call the dialog destructor. The destructor is reached
  only through the window's `vt+8`, from the NewMenuSystem update. That update is protected
  code (stub `140B3E1D0` → `144E25F7D`, section 2), readable only through the loaded-code
  route.
- **Close** (`vt+0x50` of every AppMenu window, `FUN_140A4E7C0(window, result)`):
  1. if `+0x358 == 0`: plays the close sound (`FUN_140A67CA0`) and sets `+0x358 = 1`;
  2. builds a continuation (vtable `1428AFE48`, `{window, result}`);
  3. `FUN_140A4E460` moves the window's Scaleform root (`FUN_140A4F3E0`) to the frame label
     **`FadeOut`**, or `Disabled_FadeOut` when the root is on `Disabled` (labels at
     `1428AFAD8..1428AFAF0`; `gotoAndPlay` `FUN_140A51120`). Without the label, the root is
     hidden (`FUN_140A3D720(root, 0)`);
  4. it queues the continuation (`FUN_140545FA0`, result 2). INFERRED: the window is deleted
     when the FadeOut animation ends.
- **New Game chain:**
  1. The New Game handler `FUN_140AE9070` (command table `1428D5138` of `TitleTopDialog`)
     sets `[14478D868] = 3` (no reader found statically).
  2. It calls `FUN_140BEA6B0`: movie 1 is requested (`FUN_140EACC80` → `FUN_140EACA10`), then
     a menu job opens `MovieScreen` (`FUN_140BF1520` → `FUN_140B3DBE0`; functor `142916AD8`
     slot +0x10 → `FUN_140BE67D0` → `FUN_140B528F0`).
  3. `MovieScreen`'s update `FUN_140B54F90` closes it (`vt+0x50(this, 2)`) as soon as
     `CSMovieImp+0x48 == 0` (`FUN_140EAC940`). The skip input stops the movie through
     `FUN_140EACCD0`.
  4. INFERRED: the job continuation then closes `TitleTopDialog` the same way, and its
     deletion runs the portrait release above.

### The movie step (`CSMovieIns`, states from `FUN_142692DA0`) is not the blocker

| State | Function | Role |
|---|---|---|
| 0 Init | `FUN_140EAF7B0` | next |
| 1 Wait_Request | `FUN_140EAFC00` | waits for `+0x148` (request); a set `+0x149` clears `+0x148/+0x149/+0x14A` |
| 2 Init_Setup | `FUN_140EAF7E0` | creates the Bink player (`FUN_141ADE670`). **No player:** `+0x148/+0x149 = 0`, `+0x14A = 0`, back to state 1 |
| 3 Wait_Setup / 4 Finish_Setup | `FUN_140EAFC50` / `FUN_140EAF720` | open; `+0x149` aborts |
| 5 Init_Play / 6 Wait_Play | `140EAF7D0` / `FUN_140EAFAC0` | play; `+0x149` stops |
| 7 Finish_Play | `FUN_140EAF670` | releases the player, sets **`+0x149 = 1`**, state 1 |
| 8 Finish | `140EAF660` | — |

- **"Finished" means back in state 1 with `+0x148 = 0`.** The CSMovieImp update
  `FUN_140EACD60` then clears its current movie `+0x48`, and `MovieScreen` closes.
- The no-player path reaches the same end one frame earlier, without `+0x149`.
- A request refused upstream (movtae resource `CSMovieImp+0x40` not loaded, or `+0x5C`/`+0x5D`
  busy) leaves `+0x48 = 0`, and `MovieScreen` also closes at once.
- The pilot confirmed that forcing "finished" does not release the title.

### Native preconditions to check (INFERRED ranking)

1. **The FadeOut animation never completes.** `MovieScreen`'s and then `TitleTopDialog`'s
   Scaleform root must advance their timelines.
   - The advance runs on the game thread in `SprjScaleformStep` `STEP_UpdateB`
     (`FUN_140E504D0` → `FUN_140E4DD70` → `FUN_140E55900` → `SprjScaleformSwfPlayer::Update`
     `FUN_140E53580`).
   - The captures for drawing are separate (`FUN_140E5B490` / `FUN_140E5B580` →
     `FUN_140E52490`).
   - A movie that is not created or not advanced natively (no HAL, no image creator, Update
     skipped) never ends FadeOut.
2. The close is never requested: `MovieScreen` is never created or never closes, or the
   continuation to `TitleTopDialog` is missing.
3. The protected NewMenuSystem update, which deletes finished windows, does not run.

Read-only discriminants: `TitleTopDialog+0x358` after New Game; the root clip's current label
and frame; calls to `FUN_140E53580`, `FUN_140B528F0` and `MovieScreen+0x358`; the loaded
bytes of `0xB3E1D0` and its target.

### Quitting the game (STATIC, partial)

- **Window procedure `FUN_1423578A0`:**
  1. input first (`FUN_14233FF00([1449647A0], …)`), then `FUN_1423574E0`;
  2. otherwise `WM_DESTROY` → `PostQuitMessage(0)`, and `DefWindowProcW` (so `WM_CLOSE` →
     `DestroyWindow` → `WM_DESTROY`).
- The class is registered by `FUN_142357690`; the game's own window set-up is `FUN_140EF46A0`.
- The quit then unwinds the task system's steps:
  - `DelayDeleteStep` Finish drops the pending deletions;
  - `SprjFD4LocationStep` Finish (`FUN_140D1C2B0`) destroys the location manager
    `[144791BD0]`;
  - its two update tasks (`FUN_140D1C870` and `FUN_140D1C820`, task ids 0x26/0x27, from
    `FUN_140D1B590`) stop with it.
- The relative order of the children's Finish at exit was not read.
- **In the original, at exit, no portrait view remains with registered modifiers:** they were
  destroyed at New Game. The creation views (`CharMakePreviewWindow`, section 32) are queued
  by the window's destructor and, at exit, dropped by `DelayDeleteStep` Finish.

### Back to the title from character creation

INFERRED, not read statically: the menu **cancel** action (pad B / Circle; keyboard default
INFERRED Esc) on the creation top dialog asks for confirmation, then returns to the title.
The binding lives in the key-configuration data, not in code.

Exports: `decompiled/DS3-1152-F-00AD9820`, `00ADE080`, `00AED060`, `008190B0`, `023559C0`,
`02355A10`, `02355AC0`, `00EE41E0`, `00EE42B0`, `00EE4140`, `00EDE770`, `00AE9070`,
`00BEA6B0`, `00BF1520`, `00B54F90`, `00A4E7C0`, `00A4E460`, `00EAF7E0`, `00EAFC00`,
`00EAF670`, `00EACD60`, `00EAC950`, `00EAF570`, `00E504D0`, `00E4DD70`, `023578A0`,
`00D1C2B0`, `00D1B590`, `02692DA0`. Task-group script: `evidence/I-0067/queries/d3d-paths/taskgroups.ps1`.

## 36. Native replacement of the Scaleform HAL: method specification and D3D12 equivalents (STATIC; INFERRED where marked)

Inputs reused:

- section 33 (states, frame order, `img://` path) and section 34 (method names, D3D calls);
- the pilot's owned pieces already on `main` (`native/frame/primitive_programs.*`,
  `primitive_program_catalog.inc`, `game_ui_*`, `renderer/primitive_pass.*`): programme bank,
  CPU geometry, images, uniforms and samplers.

This section is the contract a native HAL must honour. It does not prescribe the pilot's code
layout.

### What the original HAL does, as one draw (STATIC, `DrawProcessedPrimitive` `FUN_141F741F0`)

For each batch of a primitive (list `+0x28`, batch `+8` link):

1. **Programme choice.** `FUN_141F784C0(shader manager HAL+0x11A0, fill, flags, batch type
   +0x28, instance data +0x30, …)` picks the VS/PS pair:
   - the selector comes from fill type, batch type and vertex format;
   - the per-draw flags start from `HAL+0x214`, plus bit 0x10 from the fill.
   - The pilot owns this algorithm (`1F784C0` callbacks, K-0369/K-0370).
2. **Fill setup.** `vt+0xB8` (`FUN_141BDB270` → `HAL+0x288`, the profile) then `vt+0x78`:
   textures and samplers, through the texture manager (`PSSetShaderResources`,
   `PSSetSamplers`).
3. **Uniforms.**
   - Instanced batches (type 2) set the per-instance uniforms with `vt+0x338(count, …)`; the
     others use `vt+0x330`.
   - `FUN_141F80640` uploads the dirty blocks (`Map` WRITE_DISCARD into the 8 × 0xF00 dynamic
     buffers, `Unmap`, `VS/PSSetConstantBuffers`) and binds `VSSetShader`, `PSSetShader` and
     `IASetInputLayout`.
4. **Geometry**, when the mesh-cache item and its two buffers are resident:
   - `vt+0x290(flags >> 5 & 1)` re-applies the blend or raster state for the batch;
   - `vt+0x318` (`FUN_141F7BF00`): `IASetIndexBuffer(ib, DXGI_FORMAT_R16_UINT (0x39), 0)`,
     `IASetVertexBuffers(0, 1, vb, stride, offset)`, returning the first index (`+0x88 >> 1`);
     instanced batches use `vt+0x310` (`FUN_141F7BE20`);
   - draw: `vt+0x348` (`141F7ADA0`, `DrawIndexed(count +0x60, start, base +0x58)`) or, for
     type 2, `vt+0x350` (`141F7AD70`, `DrawIndexedInstanced`).
5. **Mesh-cache housekeeping:** the item is moved to the cache's "used this frame" list (LRU).

### Method specification

Each line gives the method, its role, the D3D11 work, and the expected D3D12 equivalent. Names
come from section 34: CONFIRMED by profiler marker, otherwise INFERRED.

| Method (slot) | Inputs → effect | D3D11 in the original | D3D12 equivalent |
|---|---|---|---|
| `InitHAL` (+0x38) | device, context, thread, config `0x100` | `GetImmediateContext`, `GetType`; creates 41 blend, 8 depth-stencil, 3 raster and 4 sampler states, 8 CBs, texture manager, shader manager (`HAL+0x11A0`), mesh cache (`HAL+0x3AFF0`) | one root signature; PSOs created lazily per key (below); 4 static samplers; an upload ring for constants; owned VB/IB heaps (the pilot's `game_primitive_geometry`); no device or context from the game |
| `ShutdownHAL` (+0x40) | — | releases all of the above | fence-retire everything (D3D12 objects can still be in flight) |
| `PrepareForReset` / `RestoreAfterReset` (+0x48/+0x50) | swap-chain reset | drops and recreates the default render buffer | resize: drop the wrapper of the LDR target and re-adopt it |
| **`BeginFrame`** (+0x58) | per frame, before the movies | texture-manager and mesh-cache frame begin (pending updates, LRU epoch) | open the frame's upload ring and descriptor ranges; apply pending image updates |
| `EndFrame` (+0x60) | after the last movie | frame end of the managers | close the frame; fence value for ring and LRU reuse |
| +0x68 / +0x70 | statistics reset / deferred-list execution | none with the immediate context | none |
| **`BeginScene`** (+0x78) | before a movie's display | unbinds HS/DS/GS/CS, `IASetIndexBuffer(null)`, `IASetVertexBuffers(0)`, topology TRIANGLELIST; adopts GX's RTV and DSV (`OMGetRenderTargets`) | begin recording on the UI command list; target = native LDR target, DSV = native main depth-stencil (stencil must be 0, section 33) |
| `EndScene` (+0x80) | after the display | — (deferred context only: `FinishCommandList`) | close or submit the UI list in the frame's order |
| `Draw(RenderQueueItem)` (+0x88, INFERRED) | queues tree items | none | CPU only |
| **`ClearSolidRectangle`** (+0x90) | rect, colour, blend flag | a quad with a solid-colour programme (`FUN_141F78AD0`, `FUN_141F80640`) | same quad through the native primitive path |
| `GetContextNotify` (+0x98), getters (+0xA0..+0x178) | CPU | none | CPU |
| `Display` (`FUN_141BDA3A0`, direct) | captured tree | calls `beginDisplay` and the tree's render queue | CPU traversal is the game's; only the HAL calls below reach the GPU |
| **`beginDisplay`** (+0x250) | background colour, viewport | viewport (`+0x260`, `RSSetViewports`), optional background clear | `RSSetViewports`/`RSSetScissorRects` on the native list |
| display end (+0x258) | — | pops state | — |
| **`PushRenderTarget`** (+0x20) | frame rect, target, flags, clear colour | target stack push (0x340-byte entries at `HAL+0x1A8`); `+0x280` `FUN_141F7BD30`: `ClearRenderTargetView` + `OMSetRenderTargets`; viewport | resource barrier to RENDER_TARGET, clear, OMSetRenderTargets, viewport |
| `PopRenderTarget` (+0x28 → `141BDBB70`) | flags | restores the previous target and viewport | barrier the popped target to PIXEL_SHADER_RESOURCE; re-bind the previous one |
| create RT on texture (+0x08), temporary RT (+0x10) | size, depth-stencil wanted | texture manager: `CreateTexture2D` + RTV + SRV (`FUN_141F83DC0`); depth-stencil surface `CreateTexture2D` + DSV (`FUN_141F83B50`) | pooled RT+SRV textures and D24S8 (INFERRED format) surfaces from a native heap, keyed by size |
| **`PushMask_BeginSubmit`** (+0x1A0) | mask primitive | `applyDepthStencilMode` (+0x2B0) to StencilIncrementEqual or StencilClearHigher (`+0x2B8` checks stencil support); blend state 40 (no colour) | PSO variant with that depth-stencil desc and colour write mask 0; `OMSetStencilRef(level)` |
| `EndMaskSubmit` (+0x1A8) | — | switches to StencilTestLessEqual, colour on | PSO depth-stencil variant 5; stencil ref = mask depth |
| **`PopMask`** (+0x1B0) | — | `+0x2B0`/`+0x2B8`; decrements by redrawing or clearing (`drawMaskClearRectangles` +0x2C0, `FUN_141F7ADC0`) | same, with a quad draw in the stencil-only PSO |
| **`PushBlendMode`** / **`PopBlendMode`** (+0x1B8/+0x1C0) | blend primitive | blend stack; layer blends render into a temporary RT (`PushRenderTarget`) and composite back | PSO blend index from the stack top; layer via a pooled RT |
| **`PushFilters`** (+0x1D0), `PopFilters` (+0x1D8) | filter primitive (blur, glow, shadow, bevel, colour matrix) | renders the subtree into a temporary RT, then the filter passes `141F7A590`/`141F7B150`/`141F7A130` (ping-pong between pooled RTs, `vt+0x300` vertex array, `vt+0x340` draw), composites with FilterBlend (blend entry 18 or 38) | same passes with the filter PS programmes from the bank; OBSERVED two filter targets on the HUD frame |
| **`DrawProcessedPrimitive`** (+0x190) | primitive and batch range | above | per batch: PSO(key), root CBVs or a ring offset, SRV table + sampler choice, IB/VB views, DrawIndexed(Instanced) |
| **`DrawProcessedComplexMeshes`** (+0x198) | text / complex meshes and matrices | same chain, per-mesh matrices (`vt+0x320/0x330`) | same |
| **`Drawable*`** (+0x1F8..+0x228): `Cxform`, `Compare`, `Merge`, `CopyPixels`, `PaletteMap`, `Copyback` | BitmapData operations | full-screen-quad passes into image targets (`FUN_141F77130`, `772C0`, `773E0`, `775A0`, `777B0`) | quad passes into native image RTs. INFERRED rare in DS3 menus; implement on demand |
| **`applyDepthStencilMode`** (+0x2B0) | mode 0..7, ref | `OMSetDepthStencilState(state[mode], ref)`, cached | part of the PSO key + `OMSetStencilRef` |
| raster mode (+0x2F8) | 0..2 | `RSSetState` | PSO key field (fill solid / wireframe) |
| **`drawMaskClearRectangles`** (+0x2C0) | rects | stencil-only quads | same |
| vertex array (+0x300/+0x310/+0x318) | buffers, format, offset | `IASetVertexBuffers`, `IASetIndexBuffer(R16_UINT)` | `IASetVertexBuffers`/`IASetIndexBuffer` views on owned heaps |
| draws (+0x348/+0x350) | counts | `DrawIndexed`, `DrawIndexedInstanced` | same |

### Native pipeline-state key

From the original state tables (section 33):

```
PSO = { VS index (339..510), PS index (908..1338) and input layout
        (from the VS descriptor: count at +0x2C, 32-byte records at +0x40),
        blend index 0..40 (20 modes, 20 premultiplied variants, 40 = no colour),
        depth-stencil mode 1..7,
        raster 0..2 (cull NONE, depth clip on),
        RT format (R8G8B8A8_UNORM for the LDR target and the pooled RTs; INFERRED for the pool),
        DSV format (the main depth-stencil; pooled surfaces INFERRED D24_UNORM_S8_UINT),
        topology TRIANGLELIST, 1 sample }
```

Root signature: one table or root descriptors per stage.

- **VS and PS constants.** The original keeps 8 dynamic 0xF00-byte buffers and binds them to
  the VS and PS slots `FUN_141F80640` uses. Native: root CBVs into the upload ring, sized to
  the layout from the descriptor's uniform table (`+0x10`, 250 bytes; the pilot's
  `PrimitiveUniformLayout`).
- **Textures:** an SRV table for up to the PS's texture count. INFERRED up to 4, for the
  multi-texture video and gradient programmes.
- **Samplers:** the four originals (point/linear × wrap/clamp, LOD bias −0.75) as static
  samplers on s0..s3, or a 4-entry sampler table indexed per texture stage. The pilot already
  runs all four natively (UI-INGRESS 121-125).

### Lifecycle

| When | Original | Native duty |
|---|---|---|
| boot | `FUN_140E5B250` creates and inits the HAL on GX's device and context | create the native HAL instead; never give Scaleform a D3D11 object |
| loader states | the image creator is built on `HAL vt+0x128` (texture manager) | native texture-manager interface that can wrap native images and named offscreen targets (section 33) |
| per frame | RendManImp → `FUN_140E5B940`: GX stencil reset, LDR+DSV bind, `BeginFrame`, per movie (`+0x20` order) `NextCapture` + `Display`, `EndFrame` | same order on the UI command list, after layers +0x30/+0x38 and before +0x40 |
| per movie | `BeginScene` … draws … `EndScene` | state cache reset per scene (IA unbound, topology set) |
| filters | pooled RTs, created on demand, kept across frames | pool keyed by size and format, fence-retired |
| masks | main stencil from 0, incremented per nested mask, decremented on pop | same; the native stencil must be cleared to 0 before the first movie |
| images | glyph cache and gradients updated by Map/convert (`FUN_141F84DA0`), `CopySubresourceRegion` (`FUN_141F84BA0`), upload (`FUN_141F85430`) | the pilot's native image owner (UI-INGRESS 60-125) |
| offscreen views | `CSTextureImage` holds the D3D11 texture from `vt+0x20/+0x18` | hold the native resource with a reference until Scaleform releases the image |
| shutdown | `ShutdownHAL`, texture and mesh-cache teardown | retire after the last frame's fence |

### Scaleform DXBC to take over (STATIC, from the pilot's catalogue)

- **603 programmes:**
  - **172 vertex shaders**, programme indices 339..510, 209,728 bytes;
  - **431 pixel shaders**, indices 908..1338, 526,972 bytes.
  - Each is pinned by size and SHA-256 in `native/frame/primitive_program_catalog.inc`
    (`program_pins`).
- **1,316 selector recipes** `{selector, record, VS, PS, available}` in the same file; 830 have
  a programme pair. The others are variants absent from this build.
- **Source in the executable** (exact PE hash, `DecodeOriginalPrimitiveProgramBank`):
  - tables of 8-byte descriptor pointers at **`1446F90C0`** (vertex) and **`1446FC330`**
    (pixel), indexed by programme index;
  - descriptor `+0x10`: uniform table (250 bytes); `+0x18`: semantic table (75 bytes); `+0x20`:
    DXBC payload;
  - vertex descriptors: element count `+0x2C`, 32-byte input-layout records from `+0x40`;
  - the pairs selected by `FUN_141F81A90` / `FUN_141F784C0` (K-0369/K-0370).
- **Families** (INFERRED from the selector layout; the payloads carry no names):
  - solid and textured fills (`ClearSolidRectangle`, gradients, images), each with cxform and
    premultiplied variants, plus instanced (batch type 2) and complex-mesh variants;
  - glyph text;
  - the YUV / YUVA video programmes;
  - the filter programmes (blur, shadow, glow, bevel, colour matrix);
  - the Drawable programmes.
- The native renderer already consumes these bytes as owned D3D12 programmes. Nothing needs to
  be re-extracted.

### What remains to specify or acquire

1. **Filter passes in detail:** the PS indices and constant layouts per filter type, and the
   ping-pong order of `141F7A590`/`141F7B150`/`141F7A130`. **Read in [SCALEFORM-FILTERS-NATIVE](SCALEFORM-FILTERS-NATIVE.md)**
   (the real pass loop is `141F7B930`; keys, uniforms and an ordered native checklist). The programme behind each key, the DXBC math, the uniform-slot map, the states and the page census are in [I-0069 SCALEFORM-FILTER-PASSES.md](../I-0069-static-menu-audio-map/SCALEFORM-FILTER-PASSES.md).
   - One capture of a frame with a filtered page (for example a text glow on the title) gives
     them faster than static reading.
   - The I-0055 HUD frame already has two filter targets.
2. **Pooled RT and depth-stencil formats:** INFERRED R8G8B8A8 and D24S8. The texture manager's
   format table `46F8EB0` (UI-INGRESS 60-64) and the create calls `FUN_141F83DC0` /
   `FUN_141F83B50` give the exact codes.
3. **Exact mask sequence for nested masks**, which modes on push and pop. Verify on a page
   with nested masks (inventory lists).
4. **The `Drawable*` paths:** probably unused in DS3. Confirm with a hit counter before
   implementing.

Exports: `decompiled/DS3-1152-F-01F741F0`, `01F73840`, `01F7BF00`, `01F7BE20`, `01BDC7F0`,
`01BDB9C0`, `01F76490`, `01F75C00`, `01BDC4E0`, `01F7A590`, `01F7B150`, `01F7A130`,
`01F7ADC0`, `01BDE1E0`, `01F731D0`, `01BDCBD0`, `01BDBB70`, `01F74900`, `01F74550`,
`01F74CA0`, `01F74640`, `01F74D80`, `01F74740`, `01F83DC0`, `01F83B50`.

## 37. Bink video player: planes, YUV→RGB pass, composition and end of playback, for a native replacement (STATIC; INFERRED where marked)

Classes (RTTI):

- `Movie_FMOD_Bink_CG2::MVMoviePlayerImplBink` (vtable `143A9CD08`);
- `MVBinkGraphics` (vtable `143A9CDF8`, ctor `FUN_141ADFE30`);
- the game side `CSMovieImp` / `CSMovieIns` (sections 4, 9 and 35).
- Decoding is Bink's (`BINKW64.DLL`). The graphics side goes through GX's D3D11 layer
  directly (section 34 C).

### Opening (`MVMoviePlayerImplBink` slot +0x18, `FUN_141ADF230`)

1. **Open.** `BinkOpen("movie:/%s.bik" (`1429A45F8`), flags)`.
   - Flags: `0x400` always (INFERRED `BINKNOFRAMEBUFFERS`: the game supplies the frame
     buffers), `0x100` when `+0x93`, `0x4000` with a sound track, `0x1000000` with an IO size.
   - Then the speaker volumes per track (`BinkSetSpeakerVolumes`).
2. **Plane textures:** graphics slot +0x18 (`FUN_141AE0BA0`) → `BinkGetFrameBuffersInfo` and
   `BinkAllocateFrameBuffers`.
   - For each frame-buffer set and each present plane, `FUN_141AE0E50` creates a texture
     (`FUN_1419256D0`): width × height of the plane, 1 mip, **format `0x3D` = `R8_UNORM`**,
     bind SRV.
   - Planes: **Y** (full size, `+0x6C/+0x70`), **cR** and **cB** (half size, `+0x74/+0x78`),
     **A** optional.
   - Each set keeps its four SRVs at `+0x160 + set*0x48` and its textures at `+0x140..+0x158`.
3. **Output texture** of the player:
   - `width × height` of the video (`HBINK+0/+4`), 1 mip, **`R8G8B8A8_UNORM` (0x1C)**, bind
     SRV | RTV (0x28);
   - SRV (`FUN_141925B10`, `TEXTURE2D`) → `player+0x38`; RTV (`FUN_141925A30`) →
     `player+0x40`. Player state 2 = open.
4. **Fixed pipeline objects** (graphics slot +8, `FUN_141AE0830`):

| Object | Field | Content |
|---|---|---|
| sampler | `+0x20` | GX desc (INFERRED linear, clamp) |
| depth-stencil | `+0x28` | depth off, stencil off |
| rasterizer | `+0x30` | default (INFERRED solid, cull none) |
| input layout | `+0x38` | 1 element (`float2` position), from the VS |
| **VS** | `+0x40` | DXBC **656 bytes at `1446CAAB0`** |
| **PS (opaque)** | `+0x48` | DXBC **896 bytes at `1446CAD50`** |
| PS (alpha) | `+0x50` | DXBC **1012 bytes at `1446CB0D0`**; created, not bound by the draw below |
| vertex buffer | `+0x58` | 32 bytes, immutable: `(0,0) (1,0) (0,1) (1,1)` (`143A9CE38`), stride 8 |
| constant buffer | `+0x60` | **144 bytes** (9 × float4) |

### Per video frame (`CSMovieIns` `STEP_Wait_Play` → player slot +0x68 `FUN_141ADEAF0`)

- **Where it runs:** on the game thread, in the movie step's update, **not** inside the
  RendManImp frame.
  - The GX context used is `[[[144961AB0]+0x60]]+8`, the same context as the other direct
    uploads; INFERRED immediate.
  - The GX state is saved first (`FUN_141ADF7C0`) and restored after (`FUN_141ADEED0`).
1. **Decode.** `BinkWait`, `BinkDoFrameAsyncWait(1000)`, `BinkShouldSkip` / `BinkNextFrame`
   loops, `BinkDoFrameAsyncMulti`. Graphics slot +0x30 (`FUN_141AE0800`) waits for the async
   decode and marks the set `+0x7C` ready (`+0x180 + set*0x48`).
2. **Draw** (graphics slot +0x38, **`FUN_141ADFF10(graphics, HBINK, GX context, RTV)`**):
   1. **Upload the planes** of the ready set: `UpdateSubresource` (`FUN_141920300`) of Y,
      cR, cB and A from Bink's CPU buffers (pointer and pitch at `graphics + (set+2)*0x40`,
      `+0xC` per plane).
   2. **Constants** (144 bytes, `UpdateSubresource` of `+0x60`):
      - `c0` = constant `143D7FEF0`;
      - **`c1..c4` = 64 bytes copied from `HBINK+0xD0..+0x10F`** (INFERRED: Bink's colour-space
        matrix and offsets for YUV→RGB);
      - `c5..c8`: position transform and UV scales: `(w/Ytex_w, h/Ytex_h)` for luma,
        `((w/2)/Ctex_w, (h/2)/Ctex_h)` for chroma (the planes are allocated larger than the
        video).
   3. **Bind and draw:**
      - `PS/VSSetConstantBuffers(0, 1, cb)`, `PSSetSamplers(0, 1, sampler)`, `RSSetState`;
      - viewport and scissor = `(0, 0, w, h)`;
      - `OMSetDepthStencilState(+0x28, 0)`;
      - `PSSetShaderResources(0, 4, {Y, cR, cB, A})`;
      - `VSSetShader(+0x40)`, `PSSetShader(+0x48)`;
      - `OMSetBlendState(null)` (opaque);
      - **`OMSetRenderTargets(1, player output RTV, no DSV)`**;
      - triangle strip, input layout, `IASetVertexBuffers(stride 8)`, **`Draw(4)`**.
3. **Publish.** The player returns its output SRV, and `FUN_140EADE50(CSMovieGxTexture
   [CSMovieIns+0xB8], SRV)` stores it as the proxy's current frame (`+0x18`, checks
   `TEXTURE2D`, copies size and format).

### Composition into the frame (STATIC + INFERRED)

- The video image is **not drawn by GX or by the RendManImp layers**. It is a texture:
  - `CSMovieIns`'s constructor (`FUN_140EAE1D0`) registers its `CSMovieGxTexture` in the name
    registry `[144792F58]` under the movie instance's name (`FUN_140D4A120`, section 9);
  - **Scaleform's `MovieScreen` page** samples it through `img://<name>` and the image-creator
    path of section 33 (`CSTextureImage` → D3D11 texture through `vt+0x20/+0x18`).
  - INFERRED: the name is fixed per `CSMovieIns` and referenced by the `MovieScreen` `.gfx`.
    The creator call that passes it was not read.
- **The frame therefore reaches the screen in the Scaleform pass** (section 33): into the LDR
  target, after the scene, before the fade plates.
- The proxy forwards to its default texture (`[[144792E58]+0x60]`) until the first frame is
  published.
- **Native caution:** `CSTextureImage` binds once. The player's output texture is created at
  open and stays the same for the whole playback, so the binding stays valid. A native player
  must keep **one stable output resource per open**, not one per frame.

### End of playback and its link with `MovieScreen` (section 35)

| Event | Path |
|---|---|
| request | New Game `FUN_140AE9070` → `FUN_140BEA6B0` → `CSMovieImp` `FUN_140EACA10` (needs the movtae resource `+0x40`) → `FUN_140EAC950` → `CSMovieIns` `FUN_140EAF570`: `+0x148 = 1`; `CSMovieImp+0x48` = the instance |
| play | `CSMovieIns` states 2→6: open (player slot +0x18), wait setup, play (slot +0x68 every update, as above) |
| natural end | `STEP_Wait_Play` (`FUN_140EAFAC0`) sees the player's end (`vt+0x40`) → `STEP_Finish_Play` `FUN_140EAF670`: proxy frame cleared (`FUN_140EADE50(…, 0)`), player stopped and released, **`+0x149 = 1`**, back to state 1 |
| skip | `MovieScreen` update `FUN_140B54F90`: the skip input → `FUN_140EACCD0` → `FUN_140EAF630` (`+0x149 = 1`) → the play state stops the player |
| finished | `STEP_Wait_Request` clears `+0x148/+0x149`; the `CSMovieImp` update `FUN_140EACD60` clears `+0x48`; **`MovieScreen` closes itself (`vt+0x50(this, 2)`) as soon as `FUN_140EAC940` (`+0x48 != 0`) is false** |
| no player (native today) | `STEP_Init_Setup` without a player: `+0x148/+0x149/+0x14A = 0`, state 1, same result one frame earlier |

### D3D12 equivalent

| Original | Native |
|---|---|
| plane textures `R8_UNORM` (Y full size, cR/cB half size, A optional) × frame-buffer sets | the same textures, one set per Bink frame-buffer set; each frame, copy the decoded plane rows from Bink's CPU buffers into an upload ring, then `CopyTextureRegion` (respecting `D3D12_TEXTURE_DATA_PITCH_ALIGNMENT`) |
| output `R8G8B8A8_UNORM` RT+SRV at video size, created at open | one RT+SRV resource per open, kept until the player closes and retired after the last frame that sampled it |
| VS 656 B, PS 896 B (PS alpha 1012 B) DXBC in `.rdata` | the same bytecode (take it from the PE like the Scaleform bank), one PSO per PS: input `float2` POSITION, triangle strip, blend disabled, depth off, cull none (INFERRED), RT `R8G8B8A8_UNORM`, no DSV |
| CB 144 bytes, VS+PS slot 0 | root CBV b0 (VS+PS visibility) from the upload ring; c1..c4 from `HBINK+0xD0`, c5..c8 computed as above |
| 4 SRVs t0..t3, one sampler s0 | descriptor table t0..t3, static sampler s0 (INFERRED linear clamp) |
| `UpdateSubresource` + draw on the GX context in the movie step | record on the native upload/UI list before the UI pass of the same frame, with barriers: planes COPY_DEST → PIXEL_SHADER_RESOURCE, output RENDER_TARGET → PIXEL_SHADER_RESOURCE |
| `CSMovieGxTexture+0x18` = output SRV; Scaleform samples by name | the native movie proxy answers the image-creator path (section 33) with the stable output resource |
| end, skip, `MovieScreen` close | unchanged game logic: only the player's slots +0x18, +0x30, +0x38, +0x68 and the end query `+0x40` need native implementations |

### Open

- The movie instance's registered name and the `.gfx` that samples it.
- The exact sampler and raster descriptors (GX encoding; static reading of `FUN_141925450` /
  `FUN_141925400` gives them).
- When the alpha PS `+0x50` is used (not by `FUN_141ADFF10`).
- Bink's `HBINK+0xD0` layout: INFERRED colour space; confirm by disassembling the PS
  constants.

Exports: `decompiled/DS3-1152-F-01ADFF10` (section 34), `01AE0830`, `01AE0BA0`,
`01ADF230`, `01AE0E50`, `01ADEAF0`, `01ADF740`, `01ADF710`, `00EB0890`, `00EAE1D0`.

## 38. Closing section 34's lists B and C: owners and native specifications (STATIC; INFERRED where marked)

YEBIS stays with the I-0068 thread. Owners were found through the vtable that stores each
function (`b:` query, then the RTTI record just before it).

### `FUN_14051B940`: FaceGen vertex-stream patch (`SprjFgModelVertexStreamPatch`, vtable `1427A01F8`, slot 3)

- **Role.** It writes the FaceGen-morphed shape of a head into the head model's vertex
  buffers. That is the face shape of every character with an appearance: player, menu views,
  phantoms.
- **Steps:**
  1. Under the lock `[144793C48]`, on GX context `[[GX+0x60]]` (INFERRED the immediate
     context used for uploads), for each mesh `+0xA8[]` and each vertex buffer of the model.
  2. It scans the vertex declaration (28-byte records) for:
     - position (`semantic 0`, format code 2 or 6);
     - normal (`semantic 3`, formats in mask `0x40000044`);
     - tangent or extra (`semantic 9`, format `0x1C`).
  3. `Map` (`FUN_141921860`, GX map type 3; INFERRED write-discard or write) and
     `FUN_141922B40` (buffer check).
  4. For each vertex (`+0x7C` count, remap `FUN_140773590`): it writes the morphed position
     from the FaceGen result (`FUN_140833DF0`, 12-byte records) and the normal from
     `FUN_140833DE0` (conditional `FUN_1407735F0`).
  5. `Unmap` (`FUN_141921940`).
- **Native:** CPU writes into the owned vertex storage of that model instance (the world
  renderer's per-instance vertex streams), before the frame that draws it. There is no GPU
  dependency: the inputs are CPU arrays. The FaceGen texture (below) is the other half of the
  face.

### The four "immediate draws" at 0x1403xxxxx: GX's FFX module (`GXFFX`), not game code

- `FUN_14036FD70` is slot +8 of **`GXFFX::GXFfxGraphicsResourceManager`** (vtable
  `143D66198`, resource names `GXFfxPostBumpTexture`, `GXFfxPostTexture`, `GXFfxBumpTexture`,
  `GXFfxTexture`, `GXFfxSelectedStateBufferTexture`, …). It calls `FUN_14036F9E0`, which
  creates the FFX buffers (`FUN_1419254F0`).
- `FUN_140385F70`, `FUN_140396F30` and `FUN_14039E900` are slots of RTTI-less FFX render
  classes (vtables `143D7E6C0`, `143D7E840` and `143D7E900`, after `FutBinaryMemC`). They are
  large effect-geometry builders: they bind index and vertex buffers and the topology, and
  issue `Draw`.
- **They belong to GX's effect renderer**: the same namespace as `GXFfxGraphicsResourceManager`,
  inside GX's code range in the address order, reached only through FFX vtables.
- Native: the native FFX/particle path (the host already renders the game's particles, K-0438).
  Their constant and geometry specification belongs to the effects workstream. **Nothing
  here is a menu or UI path.**

### Screenshots to file (`FUN_140205BA0`, `FUN_140223580`, GX)

| Function | Call | Format |
|---|---|---|
| `FUN_140205BA0` (from `FUN_1400C4250` and data refs) | `D3DX11SaveTextureToFileW(ctx, texture->vt+0x20 → +0x18, fmt, path)` | requested type 0 → BMP, 1 → JPG, 3 → PNG, 4 → DDS, 10 → caller's value (D3DX11 `IFF` codes) |
| `FUN_140223580` | `D3DX11SaveTextureToFileW([[GX+0x10]+8], texture, 4, path)` | always DDS (`FUN_14000B4E0` flushes GX first) |

- The game's screenshot facility (`SprjScreenCapture`, section 9: bmp/jpg/png/tif/gif/wmp,
  debug and EzTask paths) ends here. **No runtime path in the shipped game needs it.**
- **Native:**
  1. `CopyTextureRegion` into a readback buffer, fence-wait;
  2. encode with WIC (BMP, JPEG, PNG, TIFF, GIF, WMP) or write a DDS header plus the rows;
  3. off the frame path, after the fence.

### Display-mode list (`FUN_140EF9580`, from `FUN_140EF9090`)

- `CreateDXGIFactory(IDXGIFactory)` → `EnumAdapters(0)` → `EnumOutputs(0)`.
- Then `GetDisplayModeList(DXGI_FORMAT_R8G8B8A8_UNORM, 0)`: count first, then the
  `DXGI_MODE_DESC` array (28 bytes each).
- Each mode goes to `FUN_140EF9D80(owner, mode)`: the settings' resolution and refresh lists.
- **Native:** the same DXGI 1.x query on the adapter and output the D3D12 device uses
  (`IDXGIFactory4` → `EnumAdapterByLuid(device LUID)` → output 0 → the same call with the same
  format and flags). Pass the same 28-byte records to `FUN_140EF9D80`, so the options menu sees
  identical data. No D3D12 object is needed.

### FaceGen face texture and offscreen-view clear (direct GX D3D11 layer)

- **Texture creation helper `FUN_140836C40(w, h, format, mips, init, cpuFlag, kind, heap, …)`:**
  - `FUN_1419256D0` creates the texture: 1 sample, bind SRV (`8`), or SRV|UAV (`0x88`) when
    the flag is set; `kind == 2` sets a misc flag;
  - `FUN_141925B10` creates a `TEXTURE2D` SRV (mip count from `FUN_141924730` or
    `FUN_14002D3D0`);
  - the result is wrapped in a 0x70-byte GX texture object (`FUN_14002AC20`).
  - It is used by the FaceGen texture (`FrpgFgTextureIns`, section 25).
- **Upload `FUN_140836E40`** (state 2 → 3):
  1. `UpdateSubresource` of the CPU face image (`+0x10/+0x14` size, `+0x18` data, row pitch
     `w*4`) under the lock `[144793C48]`;
  2. then a 2D pass through RendManImp's lists, which updates `+0x20..+0x3C` of the list layer
     and sets blend state 7 (section 25).
  - The readiness flag `+0x91` follows (section 25 gate).
- **View clear `FUN_140D64B90`** (start of rendering, section 16): the view's composite
  `+0x48` → pipeline `vt+0x18` → its target `+0x1E0` (`FUN_140087F80`) → RTV `+0x30`.
  `ClearRenderTargetView(rtv, (0, 0, 0, 1))` (`FUN_141920200`). `FUN_140D64C30` does the same
  on set-up.
- **Native:**
  - face texture: a native texture of the same size and format, with a copy from the CPU image
    through the upload ring before the frames that sample it. Keep the state 2 → 3 → ready
    sequence intact, because the view gate waits on it (section 25);
  - clear: the native view target cleared to (0, 0, 0, 1) when listing starts, as the
    pipeline's first operation of that view.

### GX GUI shader binding (`FUN_140D10940`, `SprjGuiShaderManager` vtable `14294A9B8`, slot 3)

- **Role.** It sets up a GUI draw (Ez 2D layers, fade plates, old dialogs) for a style index.
  - The manager's table `14463E4C0` (12 bytes per style) picks a vertex programme
    `GXGuiShader_Tex1…` (names from `14463E538`), a pixel programme `GXGuiShader_Color…`
    (`14463E550`) and an input layout `param_1 + 8 + 8*k`.
  - The programmes are found by name in the shader repositories `[144792D58]` (VS,
    `FUN_140D48F10`) and `[144792BF8]` (PS, `FUN_140D47ED0`) and bound through GX
    (`FUN_14006E2F0`, `FUN_14006E090`).
  - It then sets the input layout and **unbinds HS, DS and GS** directly (`FUN_1419203F0`,
    `FUN_1419208A0`, `FUN_141920A50`, `FUN_141920C00`), updating GX's shadow fields.
- **Native:** part of the native 2D-layer pipeline. One PSO per style (VS/PS by name from the
  game's programme repository, the style's input layout, the layer's blend state such as 7
  for plates). The HS/DS/GS unbinding has no D3D12 counterpart: a graphics PSO without those
  stages.

### Updated status of section 34's lists

| Item | Owner | Native owner |
|---|---|---|
| `FUN_14051B940` | FaceGen vertex patch | world renderer, per-instance vertex streams |
| `FUN_14036FD70`/`F9E0`, `385F70`, `396F30`, `39E900` | GX FFX | effects / particle workstream |
| `FUN_140205BA0`, `FUN_140223580` | GX screenshots (debug) | readback + WIC/DDS, optional |
| `FUN_140EF9580` | settings display modes | same DXGI query on the D3D12 adapter |
| `FUN_140836C40`/`E40` | FaceGen texture | native texture + upload ring, same readiness states |
| `FUN_140D64B90`/`C30` | view target clear | native view pipeline |
| `FUN_140D10940` | GX GUI shader styles | native 2D-layer PSOs |
| `FUN_140D3F7C0`/`3FA10` | material helpers | (not read in this pass) |

Exports: `decompiled/DS3-1152-F-0051B940` (section 34), `0036FD70`, `00385F70`,
`00396F30`, `0039E900`, `00205BA0`, `00223580`, `00836C40`, `00836E40`, `00D64B90`,
`00D10940`.

## 39. How gparam reaches a menu view: file, banks, key, per-frame read; and the vertex-stream-patch helpers (STATIC; INFERRED where marked)

Section 27 has the values (`default.gparam`, id 11: BG `0xFF121212`, lights, fog, IBL, tone).
This section is the path that delivers them.

### 1. Request at start-up: `SprjGraphicsStep::STEP_Initialize_forRenderingSystem`

- The state function is **`FUN_140D59E90`** (state table `FUN_14268A320`, entry `1447933D0`,
  name `142958C10`). It runs once the step's prerequisite resource is in state 4.
- It calls `FUN_140D573B0` (`SprjGraphicsImp`) and requests the resident files through the
  resource manager `[1447559C8]`. Among them, **eight gparam files**:
  - names from `FUN_140D27D30(kind)`: `gparam:/%s.gparam` with the name table `14294F020`;
  - **kind 1 = `default`**, then 2 `M_template`, 3 `ML_template`, 4 `S_template`, 5 `empty`,
    6 `MapBase`, 10 `RemoBase`, 12 `eclipse`;
  - each through **`FUN_140503E60(resMan, path, 0)`**. The returned handles are kept in the
    step's vector `+0x148..+0x158`, so the files stay resident.
- The archive form is `param/drawparam/default.gparam.dcx` (the game's virtual path
  `gparam:`).

### 2. Load completion → `CSGparamResCap` → GX parameter file

1. The file capsule (`NS_SPRJ::GparamFileCap`, vtable `1427955A8`) finishes in
   **`FUN_1404F19E0`**. It derives the kind from the name (`FUN_140D27BD0`) and asks the
   repository **`[144791FF8]`** (`CSGparamRepositoryImp`, vtable `14294ECB0`).
2. `FUN_140D25D60(repo, name, bytes, size)` creates a **`CSGparamResCap`** (0x150 bytes,
   ctor `FUN_140D26700`, vtable `14294EF00`) in the name map `repo+0x78` (`FUN_1423562A0`),
   then parses it with **`FUN_140D272C0(cap, bytes, size)`**:
   - `+0x60` = kind (`FUN_140D27970`; default = 1);
   - `+0x64` = **area** (`FUN_140D27790`, table `14294EFB0`: kind 1 → **1**, 2 → 2, 3 → 3,
     4 → 5, 5 → 6, map kinds 6..11 → encoded map numbers, 12 → 7, 13 → 1);
     `+0x68` = an area override (−1 by default; set for map files by `FUN_140D117E0`);
   - `+0x7C` = the per-kind value from table `14294EFE8` (`FUN_140D27BA0(kind, 0)`; 1 for
     default). It is passed to the bank registration as the file's default id or slot.
     INFERRED: the role is not read; ids inside the file stay their own;
   - kinds 2..5: the bytes are copied (`+0x140/+0x148`);
   - **GX parse:** `FUN_140093F60([GXRS+0x4788], bytes, size)` → GX parameter-file handle
     `+0x70`. GX names it (`FUN_140095100`, `FUN_140093E30`).
     - `GXRS` = `[144961B50]`; the GX parameter reader is `GXBS::GXParamReader`
       (`143D5FBF8`).
3. **Automatic registration of resident kinds.** At the end of `FUN_140D272C0`, every kind
   outside 6..13, plus kind 10, is applied at once: `FUN_140D275F0(cap, +0x7C, weight)`, with
   weight 1.0 (0.0 for kind 10). **`default.gparam` is therefore in the banks from start-up**,
   with no map involved.

### 3. Registration into the banks: `FUN_140049D00([GXRS]+0x41C0, handle, area, id, cap+0x80, weight)`

- One call distributes the file into **every group bank** of the GX parameter system:
  - the light-set map `[GXRS+0x41E0]` (`FUN_140030D80` creates one 0x350-byte record per
    (area, id); TONEMAP-SKIN section 10);
  - then the managers at `+0x28`, `+0x30`, `+0xE8` and `+0x430..+0x4D0` of the bank block:
    about 25 groups (fog, env map `+0x42A8`, shadow, SSAO, lens flare, …, Setting Info
    `+0x4610`, ToneMap `+0x4618`, Bloom `+0x4620`; each through its own `FUN_14003xxxx` or
    `FUN_1401xxxxx` add);
  - finally the file is indexed by area in the hash at `+0x4D8` (key `area ^ 0xDEADBEEF`).
- Inverse: `FUN_14004A060` (used by the unapply `FUN_140D26DD0` and the cap destructor
  `FUN_140D26800`).
- Extra ids per file can be added later through `FUN_140D26AB0` / `FUN_140D26C00` (lists of
  up to 8). Map and cut-scene files are applied through `FUN_140D26A60` / `FUN_140D26A70`
  (`FUN_140D117E0`, `FUN_140CE6350`, `FUN_140CDEE50`, `FUN_140962790`).
- Lookup in every bank is by (area, id), **falling back to (1, 0)** and then to the bank's
  built-in default record (TONEMAP-SKIN section 10, `Banks::Resolve` on the pilot side).

### 4. The view's key: `FUN_140088AF0(pipeline, {1, id})`

- Callers:
  - **`FUN_140D64C30`** (offscreen holder set-up, with the row's id; section 12);
  - `FUN_140D65410` (INFERRED: re-key on row change);
  - `FUN_140D253F0` (outside the menus).
- It writes the same key into:
  - the scene key `+0x3FC0..+0x3FCC`, weight `+0x3FD0 = 0`;
  - the shadow keys (`FUN_1400888B0`, section 22);
  - **the 13 bank slots**: key A `+0x3FD8 + 8i`, key B `+0x4040 + 8i`, weight
    `+0x40A8 + 4i = 0`;
  - two more key pairs `+0x40E0..+0x40F0` and `+0x40F8..+0x4108`.
- With A = B and weight 0, every blend returns the {1, id} record.

### 5. The per-frame read: `FUN_140087FB0` (pipeline update) and the draw-time readers

- **Background** (section 27): while `pipeline+0x4144 == 0`, the Setting Info bank
  `[GXRS+0x4610]` resolves both keys of slot 0. Their `+0x80..+0x8C` are blended by `+0x40A8`,
  clamped, ×255 and packed into **`pipeline+0x6C`** (`0xFF121212` for id 11).
- **Exposure:** the Tone Map bank's second block, through the GX tone map filter's block `pipeline+0x3080..+0x30E0` (not `+0x3E84..+0x3EA4`, which is a MotionBlur copy; see section 27 and I-0113 RESULT « Tranche 27 »).
- **Lights, ambient, fog, IBL, shadow:** read at draw time from the same banks with the scene
  key (sections 22, 27 and TONEMAP-SKIN 10–11). There is no copy into the pipeline.

### What the native pipeline of a menu view must do

1. **Load once, at start-up, `default.gparam(.dcx)` as area 1.** The pilot's
   `native/content/gparam.cpp` reader already parses it.
   - Keep every group and every id (0 and 10..15 are present).
   - Add, if wanted, the seven other resident files as their areas (2, 3, 5, 6, 7, 10 and the
     map-encoded ones). The menu views need area 1 only.
2. **Resolve with the original rule:** (area, id) → (1, 0) → the group's built-in default.
   - This is the pilot's `Banks::Resolve`, with area 1 from `default`.
   - A group without id 11 falls back to id 0 (section 27).
3. **For each view, take the id from its `MenuOffscrRendParam` row `+0x2C`** (section 19:
   Status body 11, face 13/12, ChrMake 11/10, Grid 10, Kick and Sign 15, Profile 14) and
   use it as the key **for all groups at once**, with no blending.
4. **Per frame:**
   - clear colour = Setting Info "BG Color" of the key (`0xFF121212` for 11);
   - exposure = the Tone Map bank's second block through the GX tone map filter (no YEBIS in the views; I-0113 RESULT « Tranche 27 »);
   - the four lights of the LightSet record (directional 1 in camera space);
   - fog, IBL cube `EnvMap_<id>` × scale;
   - shadow parameters for the Player views only.
5. **No per-map override:** the views use area 1, so map gparams and their blends never
   apply to them.

### `FUN_140D3F7C0` / `FUN_140D3FA10`: the vertex-stream patch base (`SprjFD4ModelVertexStreamPatch`, vtable `1429522A0`, base `GXBS::GXModelVertexStreamPatch` `142952280`)

- **`FUN_140D3F620`** (base constructor) → **`FUN_140D3F7C0`** (set-up):
  - for each mesh of the model (`+0xA0` meshes at `+0xA8`), it finds the vertex buffer that
    carries semantic 0 (position);
  - it **creates a replacement vertex buffer** of `vertexCount (+0x7C) × stride (+0x68)`
    bytes (4-aligned), usage dynamic, bind vertex, CPU write (`FUN_1419254F0`);
  - the buffers go into `+0x10[mesh]`, the count into `+0x18`.
- **`FUN_140D3FA10(patch, drawCtx, drawItem)`** (draw time): when the patch exists and is
  active (`+0x1C`, set by the writer `FUN_14051B940`, section 38) and the item is a plain mesh
  (`*item == 0`):
  - **`IASetVertexBuffers(0, 1, patch VB of that mesh, stride, 0)`** directly, in place of
    the model's slot-0 stream (`FUN_1419204C0`);
  - it updates GX's shadow fields (`ctx+0x280/+0x380/+0x400`, wrapper `+0x30/+0x130/+0x1B0`).
- **`SprjFgModelVertexStreamPatch`** (`FUN_14051B890`) is the FaceGen subclass: its slot 3
  `FUN_14051B940` fills these buffers with the morphed positions and normals.
- **Native:** a per-instance position (and normal) stream in owned storage. The draw binds it
  as vertex-buffer slot 0 for that mesh instead of the shared model stream; the other streams
  stay the model's. Create it at instance set-up, write it on appearance change, and keep it
  until the instance is released.

Exports: `decompiled/DS3-1152-F-00D59E90` (existing), `00D27D30`, `00503E60`, `004F19E0`,
`00D25D60`, `00D272C0`, `00D275F0`, `00049D00`, `00D26800`, `00D26DD0`, `00D27790`,
`00D27BA0`, `00D26A60`, `00D26A70`, `00088AF0` (section 27), `00D3F7C0`, `00D3FA10`,
`00D3F620`, `0051B890`, `0268A320`.

## 40. Opening character creation from the title, and the shutdown order of the system steps (STATIC; INFERRED where marked)

### Correction to section 35 (pilot measurement)

- `TitleTopDialog` lives until the game closes, **in the original as well**.
- `TitleStep` (vtable `1428651B0`) stays in state 9 (`FUN_14090B570`), running its menu job
  runners `+0x28` (`FUN_140A3E180`) and `+0x88` (`FUN_140BB1AA0`) and the MenuMan state.
  - It moves to state 10 only when **`DAT_14489D5B8 != 0`** (game close requested; written at
    `140EDAD95`, read at `140628006`).
  - Then its destructor `FUN_140909EB0` → `TitleTopDialog::~` → the portrait release of
    section 35.
- So the portraits are released at close, not at New Game. The "45 s" were the time until the
  window was closed.

### Who opens `04_000_ChrMake_Top` (STATIC)

| Path | Chain |
|---|---|
| A (title job sequence) | `TitleTopDialog` command functors `1428D4438` (→ `FUN_140AE95E0` → `FUN_140AD79D0`) and `1428D4378` (→ `FUN_140AE8E90` → `FUN_140AD72E0`) → **`FUN_140AE3FE0`**, a sequence of about ten chained menu jobs (continuations `1428D4B68..1428D49A8`, each waiting for its predecessor's result 2 through `FUN_140545FA0`/`FUN_140BAFE10`) → link **`FUN_140AE4F60`**, which branches on `FUN_140A67B80({0xC})` → `FUN_140B1FEE0` builds the lambda `1428E3D38` → `FUN_140B2F3E0` → `FUN_140B0E8A0` → **`FUN_140B19CA0`**, the page `04_000_ChrMake_Top` (`1428E2328`) |
| B (by name / functor) | `FUN_140B3EBB0` → name dispatcher `FUN_140B52030` → `FUN_140B51570`, or functor `142916868` (`FUN_140BEF6A0` → `FUN_140BE6790` → `FUN_140BE7E30`) → `FUN_140B203C0` builds lambda `1428E3E38` → `FUN_140B2F090` → `FUN_140B0E860` → **`FUN_140B18AB0`**, same page |

- Creation therefore opens **as a menu job inside `TitleStep`**, when the preceding job of the
  title sequence returns.
- INFERRED: the preceding job is the opening movie's `MovieScreen` (closed with result 2,
  section 35). Creation does not need a step change.

### Shutdown order of `SprjSystemStep` (state table `FUN_142694CD0`)

| State | Function | Waits for (`FUN_140F0EB20`) | Then requests the end of (`FUN_140F0EBE0`) |
|---|---|---|---|
| Wait_forGameFlow | `FUN_140EDF5F0` | the game-flow child `+0x260` (title, game) to finish; quit pending through `FUN_140F0F4F0` | `+0x260` |
| Finish_forGameFlow | `140EDE760` | — (empty) | — |
| Wait_forPreGraphicsStep | `FUN_140EDF8D0` | **`DelayDeleteStep` `+0xC8`**, `+0x1B8`, `+0x1D0`, `+0x230` | GraphicsStep `+0x158` (after `FUN_140D566A0`) |
| Wait_forGraphicsStep | `FUN_140EDF6E0` | `+0x158` | **ResStep `+0xF8`**, PadStep `+0x128`, `+0x218`, `+0x248` |
| Wait_forPostGraphicsStep | `FUN_140EDF860` | `+0xF8`, `+0x218`, `+0x248`, `+0x128` | FileStep `+0x110` |
| Wait_forFileStep | `FUN_140EDF5B0` | `+0x110` | `+0x140` (INFERRED sound) |
| Wait_forSoundStep | `FUN_140EDF980` | `+0x140` | — |
| Wait_forHavokStep | `FUN_140EDF740` | — | Remo `+0x1A0`, **FD4LocationStep `+0x188`**, DbgMenu `+0xE0`, `+0x200`, `+0x1E8` |
| Wait_forOtherStep | `FUN_140EDF7F0` | those five | — |
| STEP_Finish | `FUN_140EDE660` | — | destroys every child in reverse slot order (`FUN_140F0EAB0` → `FUN_140F0F930`) |

- The child slots come from `FUN_140EDE770`, section 35:
  - `+0xC8` DelayDeleteStep, `+0xE0` DbgMenu, `+0xF8` ResStep, `+0x110` File, `+0x128` Pad;
  - `+0x158` Graphics (`FUN_140D58580`), `+0x170` Scaleform, `+0x188` FD4Location
    (`FUN_140D1B590`), `+0x1A0` Remo.
- **Order that matters for the views:**
  1. the game flow ends (`TitleTopDialog::~` and `CharMakePreviewWindow::~` queue their views
     with count 6);
  2. **`DelayDeleteStep` finishes first.** Its Finish `FUN_140EE4140` frees the pending entries
     without destructors;
  3. **ResStep finishes before the location step**: the character resources (c0000, the hkx
     skeleton data) are released while the location manager still exists;
  4. **the location step finishes almost last** (Finish `FUN_140D1C2B0` destroys
     `[144791BD0]`).

### Why a live view can crash only in native mode (INFERRED)

- A view that is still registered when ResStep releases its skeleton data crashes as soon as a
  location update (tasks 0x26/0x27, `FUN_140D1C870`/`FUN_140D1C820`) runs again.
- The original avoids it when the queued views are really destroyed: when `DelayDeleteStep`
  runs at least seven Updates (task group 0x5E, after Flip) between the end of the game flow
  and its own Finish request. `FUN_140D66280` then unregisters the modifiers (section 15).
- In native mode, if the frames that run during the game-flow shutdown skip group 0x5E (or
  run fewer than seven), the views are dropped without destructors, and the location update
  later reads freed data.
- **Read-only checks** (original against native):
  - calls to `FUN_140EE42B0` between `FUN_140AD9820` and `FUN_140EE4140`;
  - whether `FUN_140D66280` runs for the 10 Prof and 4 Player views at close;
  - whether `FUN_140D1C870`/`FUN_140D1C820` run after ResStep's end.
- **Probable native fixes:**
  - keep the task groups, 0x5E included, running during the shutdown waits;
  - or drain the deferred queue by calling its deleters before requesting `DelayDeleteStep`'s
    end.

Exports: `decompiled/DS3-1152-F-0090B570`, `00909EB0`, `00AE3FE0`, `00AE4F60`, `00AD79D0`,
`00AE95E0`, `00B19CA0`, `00B18AB0`, `00B2F090`, `00B2F3E0`,
`00EDE660`, `00EDF5F0`, `00EDF8D0`, `00EDF6E0`, `00EDF860`, `00EDF5B0`, `00EDF980`,
`00EDF740`, `00EDF7F0`, `02694CD0`, `00F0EAB0`.

**Pilot measurement after section 40 (2026-10-10).** The original game crashes at exactly the
same place (same instruction, same stack, `0xC0000005`) when the window is closed during
character creation.

- The close-time crash is therefore **original behaviour**, not a native divergence.
- The "probable native fixes" above are not required for parity. They stay as notes on the
  mechanism.

## 41. Phantom programme selection, phantom material families, and the three face-grid entries (STATIC; INFERRED where marked)

### Phantom selection: what was found

- **Feature id 0x11 = `GXFT_ValidSubPhantom`** (enum name table `143D21040`/`143D21060`,
  names `143D47B70`/`143D47BE0`; GUI reflection only, no code reference). GX feature ids are
  bit numbers of the per-draw feature word. **Withdrawn in section 42:** bit 17 of that word is the
  `Gbuf2` pass flag, and the phantom pixel programmes are the `Fwd`/`FwdSdw` entries of every
  set.
- **The only bit-0x11 consumer in GX's code range is `FUN_140200840`**: draw-state selection,
  a slot of a GX draw-state class (data refs `144985F70`).
  - With **bit 17 (0x20000)** of the draw's feature word, the blend index becomes **base +
    0x14**, or 0x14 when the material has none. That is the common blend table
    `[[GX+0x2A0]+8+8i]`.
  - The bit also takes the "extra state" branch: `param_2+6 = 1` and a state record pushed on
    the context (`ctx+0x21F1`, `+0x2070 + 16n`). The same branch serves bits 16 and 11.
  - Neighbouring bits: 16 → +0xF, 12 → +10, 21 → +0x33, 15 → +0x19, 19 → index 0x32.
- **Where the phantom programmes are taken:**
  - `FUN_1400A9EF0` overwrites the per-pass table entries of passes 0 (`Fwd`) and 4
    (`FwdSdw`) at `set+0x18 + pass*0x18` with `<shader>_Phantom_<pass>` (`[A]`, `[AA]`);
  - the main-table getter (slot 2, `FUN_1400AA280`) never reads that per-pass table;
  - no static reader of `set+0x18..+0x88` was found (section 23's scan, repeated here for the
    slot functions of `GXFlverShaderGroup` `143D48C18`).
  - INFERRED: the forward/shadow recorder reads the per-pass entry when the draw carries
    feature bit 0x11, the same bit that switches the blend above. The setter of bit 0x11 on a
    character's draws (from the PhantomParam application `FUN_1409CAF30`, section 29) was not
    reached statically.
- **Fastest closure at runtime** (one test with a summoned phantom or a bloodstain/replay
  ghost):
  - a read breakpoint on its shader set `+0x18..+0x28`;
  - a log of the feature word `param_3[0]` passed to `FUN_140200840` for that character's
    draws.

### Material families with a phantom variant (from the shader inventory, `evidence/I-0063/artifacts/scan/shader.tsv`)

896 phantom programmes in `gxflvershader.shaderbnd.dcx` (462), `shaderbdle.shaderbdlebnd.dcx`
(242) and their debug bundles. They cover **64 shaders**:

| Family | Shaders (each with `_Phantom_Fwd`/`_FwdSdw` and `[A]`/`[AA]`) |
|---|---|
| Equipment | `GXFlver_Equip`, `_Cloth`, `_Cm`, `_Cm_Cloth`, `_Df`, `_Df_Cloth`, `_Df_Cm`, `_Df_Cm_Cloth`; `SAT_Equip`, `_cloth`, `_Df`, `_Df_cloth`, `_SSS`, `_SSS_cloth`, `_Veill`, `_Veill_cloth`, `_em`, `_em_cloth`; `SAT_MultiBlend2_BlendOpacity_Equip`, `_Cm`, `_Cm_cloth`, `_cloth` |
| Body, skin, face | `GXFlver_SkinIbl_Bloody`, `GXFlver_SkinSSSIbl_Bloody`, `GXFlver_FgFaceIbl_Bloody`, `GXFlver_FgFaceSSSIbl_Bloody`, `GXFlver_EyeIbl` |
| Hair | `GXFlver_HairIbl2`, `GXFlver_HairIbl_Bloody`, `_Cloth`; `SAT_HairIbl2`, `_cloth` |
| Generic character (`ColDifSpcBump…`) | `…BumpIbl`, `_Cloth`, `_Df`, `_Df_Cloth`; `…BumpIbl_Bloody` (+ `_Cloth`, `_Cm`, `_Cm_Cloth`, `_Df`, `_Df_Cloth`, `_Df_Cm`, `_Df_Cm_Cloth`); `…BumpBlendIbl_Bloody` (+ `_Cloth`); `…BumpEmiIbl_Bloody` (+ `_Cloth`); `…BumpMulIbl_Bloody` (+ `_Cloth`); `…BumpMulMaskIbl_Bloody` (+ `_Cloth`); `…BumpTrIbl_Bloody` (+ `_Cloth`) |
| SSS character | `GXFlver_SSSIbl_Bloody` (+ `_Cloth`), `GXFlver_SSSEmiIblGlow_Bloody` (+ `_Cloth`) |
| Special | `Dragon_FDP`, `Dragon_FDP_cloth`, `Fur_FDP`, `Fur_FDP_cloth`; `SAT_MultiBlend2_BlendOpacity_Bloody`, `_cloth` |

**Suggested witnesses**, one per programme family a summoned white phantom actually wears:

1. armour pieces (`GXFlver_Equip*` / `SAT_Equip*`);
2. skin and FaceGen face (`…SkinSSSIbl_Bloody`, `…FgFaceSSSIbl_Bloody`);
3. hair (`HairIbl*`);
4. eyes (`EyeIbl`);
5. a cloth-simulated part (`_Cloth`/`_cloth`).

Fur, dragon and multi-blend cover special NPC phantoms (later).

### The three `FaceSelectDialog` sites ↔ creation menu entries (STATIC)

The creation command builder **`FUN_140B1E7F0`** switches on the `CharMakeMenuTopParam` row's
entry kind (field `+0x00`) and wraps each entry in a menu-job lambda:

| Entry kind | Lambda vtable | Site | `CharMakeMenuTopParam` row(s), text id | Behaviour |
|---|---|---|---|---|
| **12** | `1428E4838` (built by `FUN_140B24990`) | **`FUN_140B1AA00`** | rows **18** and **159**, text **420049**, page 2 (first entry after the headers 420006/420007 of the appearance page) | grid of templates (fields `+0xEB0`, `+0xF20/+0xF28`, `+0x1D4`) |
| **14** | `1428E36B8` | **`FUN_140B18750`** | row **299**, text **420009**, page 2, last entry | scans the 15 slots (`+0xE90` count, 0xF8 each, a "valid" byte). **When none is valid, shows message 420010** (`FUN_140A649F0(0x668AA)`) instead of the grid |
| **15** | `1428E36F8` | **`FUN_140B17E90`** | row **298**, text **420008**, page 2, second to last | opens the grid at once (`FUN_140B29580(…, 0, 1)`) |

- Row 16 (kind 24, text 420006, page 1 → 2) is the entry that opens the appearance page.
  INFERRED: "Appearance".
- INFERRED meaning:
  - kind 12 (420049) is the face-template grid at the top of the appearance page; rows 18 and
    159 are its two sections (INFERRED one per body type);
  - kinds 15/14 (420008/420009) are a save and load pair over 15 saved-appearance slots. The
    load entry refuses with 420010 when no slot is valid.
- The text ids are confirmed; the English strings were not read (the menu FMG is not in the
  offline extraction).

**Keys (INFERRED, to confirm on screen; bindings are in the key-configuration data, not
code):**

1. From the creation top dialog, move to the appearance entry (row 16) with the arrow or
   direction keys and confirm (keyboard confirm key, pad A/Cross).
2. **The first entry of the appearance page (row 18, text 420049) opens the 15-face grid**
   (`FUN_140B1AA00`).
3. The last two entries of that page (420008, 420009) open the same dialog in its
   saved-slot form.

Runtime check: a break on `FUN_140B29580` names the entry through its caller's return address
(`140B1AA00` / `140B17E90` / `140B18750`).

Exports: `decompiled/DS3-1152-F-00200840`, `00B1E7F0`, `00B24990`, `00B17E90`,
`00B18750`, `00B1AA00`, `000A9EF0`, `000AA280`.

## 42. Phantom programmes revisited (correction to section 41), the creation menu's labels and structure, and the NPC phantom row (STATIC; INFERRED where marked)

### Correction: bit 0x20000 of the draw context is the `Gbuf2` pass, not `ValidSubPhantom`

Section 41 assumed that GXFT ids are bit numbers of the per-draw feature word. **That is
withdrawn.**

- The word read by `FUN_140200840` (`param_3[0]`) is the **draw context's `+0x200`**. The
  struct passed as `param_3` is `ctx+0x200`: word, `+0x204` column, `+0x208` the
  material-override list. `FUN_140140850` sets `ctx+0x208` just before applying the material.
- **`FUN_14005A120`** sets up a draw context for a pass descriptor (kind `*param_3`). It writes
  the pass index `ctx+0x1C` and the pass bits of `ctx+0x200`:

| Pass kind | `ctx+0x1C` (pass, table `14473CE60`) | `ctx+0x200 |=` |
|---|---|---|
| 0 | unchanged | — |
| 1 | 2 `Dpt` | `0x80800` |
| 2 | 2 `Dpt` | `0x40000` |
| 3 | 6 `Gbuf` | `0x10100` |
| **4** | **7 `Gbuf2`** | **`0x20100`** |
| 5 | 1 `Velo` | — |
| 6 | 12 `WatDpt` | `0x1000` |
| 9..12 | 13..16 (`Aof`, `Aof_Dol`, (`143D46078`), `Voxelization`) | `0x10100` |

  - Other bits from the same function: `0x100000` (descriptor `+0x1A9`), `0x4000`/`0x8000`
    (`gx+0x714` = 1/2).
- So `0x20000` marks the **second G-buffer pass**, and `FUN_140200840`'s blend index +0x14 is
  that pass's blend set (+0xF for `Gbuf`, bit 16).
- **`GXFT_ValidSubPhantom` (0x11) is a material-parameter declaration only.** The enum
  (`143D21040`) pairs each id with its parameter name (`ValidSubPhantom`).
  - Its class `GXMaterialParamConstant_ValidSubPhantom` (vtable `143D603E8`) has empty apply
    slots: `+0x10` `FUN_140200500` and `+0xA0` `FUN_140200510` return at once. `+0x88`
    `FUN_140200450` is its clone.
  - Compare `ValidWet` (vtable `143D60498`), whose slots `1402007A0`/`1402007B0` do work.
  - No code sets or tests it at draw time.

### Where the phantom pixel programmes are read (STATIC)

- **The shader set** (0x3F00 bytes, built by `FUN_140082E50` for one shader name) has two tables:
  - `+0x1F8 + …`: the **vertex** programmes by pass and permutation, filled by `FUN_1400AA630`
    for passes 0..16;
  - **`+0x18 + pass*0x18 + v*8`** (v = 0 plain, 1 `[A]`, 2 `[AA]`): the **pixel** programme
    of each pass. `FUN_1400AA630` fills it with `<shader>_<pass>`.
- **Then `FUN_1400A9EF0` (always called last by `FUN_140082E50`) overwrites the `Fwd` (0) and
  `FwdSdw` (4) entries** with `<shader>_Phantom_Fwd` / `_Phantom_FwdSdw` and their `[A]`/`[AA]`
  variants. The format strings are `143D48A18` `%s_Phantom`, `143D48A50` `%s_%s`, `143D48A40`
  `%s_[A]` and `143D48A88` `%s_[AA]`.
- **Reader:** the set's getter `FUN_1400AA280` (slot 2, called from `FUN_140140850` with pass
  `ctx+0x1C`, variant `uVar9`) returns:
  - `out[0]` = the vertex programme from `+0x1F8`;
  - **`out[1]` = `set + 0x18 + (pass)*0x18 + variant*8`**: the per-pass pixel table.
  - `FUN_140142430` binds `out[1]` with `PSSetShader` (`FUN_141920EA0`, context slot 0x48) and
    `out[0]` with `VSSetShader` (`FUN_1419206E0`, slot 0x58); `out[2..4]` are HS, DS and GS.
  - Section 41's "no static reader of `set+0x18`" was wrong: the getter is that reader.
- **Consequence.** For the 64 families of section 41, **every `Fwd` and `FwdSdw` draw uses the
  `_Phantom_` pixel programme**. Nothing switches programmes per character.
  - INFERRED: the phantom look comes only from the material constants `+0x1A0..+0x1EF`
    written from PhantomParam (section 29). With no PhantomParam row, those constants keep the
    material's own values, so the programme renders as an ordinary forward draw.
  - INFERRED: a phantom character is drawn in the forward pass instead of the G-buffer. The
    routing was not read; `GXFT_ForceForward` and translucency are the candidates.
  - Open: what the repository lookup (`FUN_1400A9B00`/`9B50` → repository `vt+0x30`/`+0x20`)
    returns when `<shader>_Phantom_Fwd` does not exist, for shaders outside the 64 families.
- **Alternative shader set.** `FUN_140140850` uses `item[0xF]` instead of `item[0xE]` when
  `ctx+0x425` is set. That byte is copied from pass descriptor `+0x87` (`FUN_1401A8580`) or
  pipeline `+0x3F7D` (`FUN_140204C30`). Its role was not identified.
- **Native:**
  1. for those 64 families, build the `Fwd` and `FwdSdw` PSOs with the `_Phantom_` pixel
     programmes (plain, `[A]`, `[AA]` by alpha mode);
  2. feed the material block `+0x1A0..+0x1EF` from PhantomParam for phantom characters;
  3. keep material defaults otherwise.
- **Runtime check:** on a phantom draw, break on `FUN_140142430` and compare `out[1]` with
  `set+0x18` (pass 0).

### Character creation: menu labels and the path to the face grid (STATIC + game text)

The labels come from `msg/engus/menu_dlc2.msgbnd.dcx`:

- `FDP_メニューテキスト` holds the entry names;
- `FDP_一行ヘルプ` holds the one-line help;
- `FDP_ダイアログ` holds the dialogs.

The table joining them with `CharMakeMenuTopParam` is
`evidence/I-0067/queries/charmake/charmakemenutopparam-labels.tsv`.

| Id | Entry name | Help / dialog |
|---|---|---|
| 420006 | Face Presets | Select a preset face. |
| 420007 | Face Detail | Fine-tune facial appearance. |
| 420008 | Save to Favorites | Save present settings as a favorite. (dialog: "Save present settings as a favorite?" is 420000) |
| 420009 | Load Favorite | Load saved favorite. |
| 420010 | Physique | dialog: **"No favorites saved."** |
| 420049 | **Similar Face** | Select face similar to present face. |
| 420500 | Defaults | Return appearance to default. |

**Menu structure** (`FUN_140B1E7F0`):

- Row kind 10 opens a sub-page and kind 11 closes it. When `FUN_140B257C0` refuses a kind-10
  row, the builder skips its whole block.
- Row field `+0x10` is the display condition:
  - 0: always;
  - 1 / 2: by `FUN_140B30D00` (the two beard rows);
  - **3: menu `+0x140 == 0`**;
  - **4: menu `+0x140 != 0`**.

**The two modes:**

| Mode | Entries shown (`CharMakeMenuTopParam` row) |
|---|---|
| `+0x140 == 0` (the page the native pilot sees) | Gender (1), Age (2), Voice (3), Physique (4), Build Detail (9), Base Skin Color (147), Skin Color (148), **Face Detail (158)**, Save to Favorites (298), Load Favorite (299), Defaults (300) |
| `+0x140 != 0` | Age, Voice, Physique, Build (8), Build Detail, **Face Presets (16)**, **Face Detail (17)**, Base Skin Color, Skin Color (288), Save, Load, Defaults |

- **Path to the 15-face grid (`FUN_140B1AA00` → `FUN_140B29580`):** **Appearance → Face Detail
  → Similar Face.**
  - Similar Face is the first entry of the Face Detail sub-page, before Features.
  - Its rows are 159 in mode `+0x140 == 0` and 18 in the other mode.
- Face Presets (kind 24) is the separate list of presets, not the grid.
- Save to Favorites (298, kind 15) and Load Favorite (299, kind 14) open the same dialog in
  its saved-slot form. Load with no valid slot shows dialog 420010.
- INFERRED: the meaning of `+0x140` (the two creation contexts) was not read.

### The PhantomParam row of an NPC phantom (STATIC + regulation)

`FUN_14088A950` resolves the row in this order:

1. `chr+0x1F38`;
2. `chr+0x1A58` (unless −2);
3. **`NpcParam+0x1CC`** when ≥ 0;
4. **`NpcParam+0x238`**, when the team type `chr+0x70` (`FUN_1408894B0`) is flagged in table
   `1445B6800` (`FUN_1409F6480`; my reading flags types 3, 4, 10 and 11, names not confirmed)
   and the value is ≥ 0;
5. RoleParam (section 29).

NpcParam values (`evidence/I-0067/queries/phantom/npcparam-phantom-fields.tsv`):

| Field | Value (PhantomParam) | NpcParam rows |
|---|---|---|
| `+0x1CC` | **80** | 29 human NPCs: 21000, 21005, 21420, 21421, 21520, 25020, 25120, 25121, 25220..25223, 25420, 25820, 25821, 26020, 26021, 26320, 27320, 27321, 27420, 27520, 27521, 27620, 27820..27822, 27920, 29220 |
| `+0x1CC` | 81 | 16 rows (21440, 21640, 22140, 22240, 22340, 27440, 27441, 27890, 27891, 28040, 29240, 29241, 29340, 29341, 29440, 29540) |
| `+0x1CC` | 60 / 88 / 74 / 92 / 187 / 33 | 8902xx..8905xx / 21200, 27720, 27740 / 21001, 21006 / 130710..130721 / 27721 / 514010 |
| `+0x238` | **76** | 88 rows (21002, 21100, 21400..21405, 21500, 22000, 24800..24803, 25000, …) |

- PhantomParam **80**: edge white a 0.5, diffuse white a 0.5, specular white a 1 (the player
  white phantom, RoleParam 1).
- PhantomParam **76**: edge white a 1, light (160, 160, 160) a 1, everything else 0.
- INFERRED: an NPC summoned from a sign takes 80 when its row fixes it, otherwise 76 through
  the team-type route. NPC names were not resolved; the in-game NpcParam id decides.

Exports: `decompiled/DS3-1152-F-0005A120`, `00140850`, `00142430`, `000AA630`, `00082E50`,
`001A8580` (existing), `00200450`, `009F6480`, `00B257C0`; `000A9EF0`, `000AA280`,
`00200840`, `00B1E7F0`, `0088A950` (earlier sections).

## 43. In-game gparam: map files, area and key selection, crossfades, and light orientation (STATIC; INFERRED where marked)

This section continues section 39 (resident files, banks, menu keys) for the world.

### Short answers for the native pilot

*Correction (I-0066, 2026-10-13, measured): the area's own file (kind 7) is bound by the area controller `FUN_140CE6350`
through `D26A70 -> D275F0` with the map's transform already in its `+0x80` (yaw 28 at m40, about 179 at m37, 204 at m31;
Reference1537-1539, 1546), and `FUN_140CDEE50` writes that field statically; the "identity only" below holds for the other
files. See I-0066 NATIVE-LIGHTING-MAP "Sun source".*

1. **MapOffset is not read on the gparam path.**
   - The only rotation the light code applies is a per-file transform. It is the
     `CSGparamResCap` 3×4 matrix `+0x80..+0xAF`, which **only the constructor `FUN_140D26700`
     writes, as identity** (`1439F9720`).
   - In the gparam module (`140D24000..140D29000`) it is only passed on, by `FUN_140D26AB0`,
     `FUN_140D26C00`, `FUN_140D275F0` and `FUN_140D26A70` to `FUN_140049D00`.
   - The MSB event accessors (`FUN_140E14240` count, `FUN_140E14260` get, `FUN_140E141F0` find)
     are called for event types 5, 7, 12, 14, 15 and 16 only. They are never called for 9,
     MapOffset in SoulsFormats' MSB3 numbering (the numbering is INFERRED).
2. **The bank records do not rotate by the map.**
   - The transform above becomes a quaternion in each LightSet record (`+0x2D0`, set by
     `FUN_140030D80` through `FUN_1400362B0`). It rotates every directional light, but it is
     identity for every shipped file.
   - **Native `Banks::Sun` needs no MapOffset yaw.** It needs the angle formula below,
     applied in world space.

### Directional light orientation (STATIC)

A LightSet record (0x350 bytes, built per (area, value id) by `FUN_140030D80`) holds four
lights of 0x70 bytes at `+0x40 + i*0x70`. In each light:

- `+0x00`: the world direction (float4);
- `+0x30` / `+0x34`: angle X and angle Y, **in radians at runtime**;
- `+0x60` / `+0x64`: angle offsets.
- INFERRED: the file's degrees are converted when the fields are bound (`FUN_14014BD30`).
  The blend lerps angles with wrap periods π/2 and π (`143D7EE40`, `143D7EEC0`).

**`FUN_14014BA90`** runs after the record is filled, for each light:

1. `X += offX`, `Y += offY`;
2. `v = (cos X · sin Y, −sin X, cos X · cos Y, 0)`;
   - `FUN_141FD1A40` is sinf: it returns x for |x| < 2⁻¹³ and x − c·x³ for small x;
   - `FUN_141FD1740` is cosf;
   - the sign constants are `143D1C4A0` = (1, −1, 1, 1) and `143D1BFA0` = (0, 1, 0, 0);
3. `dir = normalize(q ⊗ v ⊗ q⁻¹)` with `q = record+0x2D0` (`FUN_140149040`), which is
   identity in practice.

**Blend of two keys** (`FUN_1400F90F0`, keys A and B from the pipeline `+0x20`/`+0x28`, weight
`+0x30`):

- each angle is the wrap-aware lerp of `angle + offset` (`FUN_1400F8FD0`);
- then the same formula;
- then rotation by `slerp(qA, qB)` (`FUN_1400FEA40`).
- With only key A, the record's precomputed `+0x40` is used.

### Which files a map loads, and who asks (STATIC)

**Requester: the map step `FUN_14061B590`.**

1. It reads the map's **`Ceremony` param** row (param index 0x39, name `142989C38`; row id =
   map id + ceremony, for example 40000000 and 40000010) through the map-info repository
   `[1447914C8]` (`FUN_140CE7100` → `FUN_140E272A0`).
2. From the row it takes:
   - `+0x08` (u16) → `+0x418`, the gparam file id base;
   - `+0x0A` (u16) → `+0x41C`, the MapOverride id;
   - `+0x0C`, `+0x10`, `+0x14` → `+0x428`, `+0x424`, `+0x420` (other per-ceremony resources).
3. It then requests **ten** files:
   - `gparam:/m%02u_%02u_%04u.gparam` (`FUN_140D27DB0`, format `14294F520`) with AA and BB from
     the map id and number `i*100 + base%100`, for i = 0..9;
   - for example `m40_00_0000`, `0100` … `0900` for ceremony 0, and `m40_00_0010` … `0910` for
     ceremony 10;
   - plus `gparam:/MapOverride_%04u.gparam` (`FUN_140D27E20`) from the row's `+0x0A`.
   - The regulation dump is `evidence/I-0067/queries/gparam/ceremonyparam.tsv` (54 rows).
4. Cut scenes load `s%02u_%02u_%04u` and an m-file through `FUN_140CDE5B0` (remo state table
   `FUN_142687F70`).

**Parse (section 39 path):**

- the name `mAA_BB_XXXX` gives kind 7 (`FUN_140D27970`);
- area = `((AA+100)·100 + BB)·10000` (`FUN_140D27790`; **140000000 for m40_00**);
- **map files are not auto-applied**: kinds 6..13 except 10 are skipped by `FUN_140D272C0`.

### How a map's files enter the banks (STATIC)

Map-info entries (0x110 bytes each, array `[1447914C8]+0x20`, ctor `FUN_140CE5E80`):

- `+0x08`: map id;
- `+0x0C`: area code (`FUN_140D278C0`);
- `+0x18..`: the Ceremony values;
- `+0x48..+0x97`: the ten file caps;
- `+0x98..+0xA8`: five active file indices;
- `+0xAC..+0xBC`: five target file indices.

1. **Targets come from the player's collision part** (`FUN_1408FA7F0`).
   - The part is the player's `+0x1AC8`, or an override from `[144791248]`
     (`FUN_140CE26E0`).
   - `FUN_140CE6040` reads:
     - **slot 0 = file 0** always;
     - **slots 1..4 = the part's gparam bytes `+0x3C..+0x3F`** (`FUN_140E17640`; 0 means
       none);
     - **fade time = part `+0x40`** (`FUN_140E17690`; negative means the default
       `143D7EEB8`).
   - The entry is "the player's map" when the part's map id matches.
2. **Apply** (`FUN_140CE6350`, per entry):
   - when a slot's target changes, the old file is withdrawn with the fade
     (`FUN_140D26DD0`);
   - the new file is applied with **`FUN_140D26A70(cap, slot, fade)`**, which registers it with
     area = cap `+0x68` (override) or `+0x64` (from the name) and **layer = 5 + slot**
     (`FUN_140D27BA0(7, slot)`, table `14294EFE8`);
   - the area-wide layer stack (`FUN_140096560`) builds one record per value id found in the
     file (`FUN_140030D80`, ids from `FUN_140096310`).
   - The Ceremony row's `+0x1C` (MapOverride) is applied separately with `FUN_140D26C00`.
3. **World key** (`FUN_1408FA7F0` → `FUN_140D52AE0([1447914C8]+0x28, {area, 0}, fade)`):
   - when the player is in the map, **the world uses the single key (area, 0)**, crossfaded
     from the previous key over the fade time;
   - the crossfade class is section 39's slot `{+0x190 key, +0x198 previous key, +0x1A0
     weight 1→0, +0x1A4 duration}`;
   - `FUN_140D24FC0` / `FUN_140D52AE0` set it, and `FUN_140D250F0` advances it each frame.
4. **Per frame** (`FUN_140E006A0`):
   - the world renderer picks the gparam state `+0x508`, else `+0x500`, else its own `+0x348`;
   - `FUN_140D52B70` then writes the pipeline keys:
     - with a valid all-key: `FUN_140088AF0(key)`, or `FUN_140088C30(prev, key, 1 − w)`
       during a fade;
     - otherwise the twelve per-group slots `+0x08..+0x168` (`FUN_140D25270(slot, group)`),
       plus the shadow slot `+0x148` (`FUN_140D25450`) and its `+0x170/+0x180` pair.
5. **Per-part keys** (outside the camera key):
   - `FUN_140CC8180` (map parts) keys each drawn part with (map area, the part's `+0x30A`
     id);
   - its blend from `FUN_140CC8530` uses the part's short ids at `+0x7A` and `+0x10A`, and the
     map area from `FUN_140CCDA60` or `FUN_140CCD9F0`;
   - the result goes into the LightSet, Fog, LightScattering, EnvMap and ShadowMap banks
     (`FUN_140031A20`, `FUN_1400380A0`, `FUN_14003C900`, `FUN_140040DD0`, `FUN_140045CA0`)
     at layer `FUN_140D27BA0(9)`.
   - `FUN_140CC6810` and `FUN_140C4D060` set per-group slots for other owners (characters,
     events). INFERRED: which owner uses which group was not read.

**Lookup rule** (`FUN_140032770`, the same in every bank):

1. exact (area, id);
2. else (1, 0);
3. else the bank's default.

There is no fallback inside an area.

- **Native consequence:** the world reads **(140000000, 0)** for m40_00. If the m40 files
  have no value id 0, the original would also fall back to `default.gparam`.
- The keys `100/110/300` measured by the pilot are value ids inside the files. In the
  original they are reached only by part keys (step 5) or events.

### What native mode must do (from the pilot's note: `AdmitGparam` replaces `D272C0`, `BindNativeGparamScene` replaces `49D00`)

1. Load the ten `mAA_BB_<i*100 + base%100>` files plus `MapOverride_<id>` named by the
   Ceremony row of the current map and ceremony.
2. Register them under area `((AA+100)·100 + BB)·10000`:
   - file 0 always at layer 5;
   - files chosen by the collision part's four bytes `+0x3C..+0x3F` at layers 6..9;
   - higher layer overrides lower (INFERRED from the layer ids; the layer merge
     `FUN_140096560` was not read).
3. Camera key = (area, 0), crossfaded over the part's `+0x40` seconds on map or part change.
4. Directions:
   - the formula above, in world space;
   - **no MapOffset or MSB yaw**;
   - angles in radians after the binder's conversion.

### Open (runtime checks)

- Whether the m40 files contain value id 0. Read them with the pilot's `gparam.cpp`; a
  break on `FUN_140032770` with key (140000000, 0) answers it.
- How layers 5..9 merge (`FUN_140096560`, `FUN_140096DA0`): override or weighted.
- The binder's degree-to-radian conversion (`FUN_14014BD30` field descriptors).

Exports: `decompiled/DS3-1152-F-0061B590`, `00D27DB0`, `00D27E20`, `00D27970`, `00D27890`,
`00CE7100`, `00E272A0`, `00CE5E80`, `00CE6040`, `00CE6350`, `00CE7280`, `008FA7F0`,
`00D52AE0`, `00D52B70`, `00D24FC0`, `00D253F0`, `00D26700`, `00E14240`, `00E14260`,
`00E141F0`, `00E14540`, `00CC8530`, `00CC6810`, `00E17640`, `00E17690`, `00CDE5B0`,
`00149040`, `000F8FD0`; existing `00D26A70`, `00CC8180`, `0014BA90`, `000F90F0`, `00032770`,
`00030D80`.

## 44. Hair and eyebrows in the character views: materials, programmes, per-instance data, the face-texture override, and what can null them (STATIC; INFERRED where marked)

Pilot measurement (2026-10-10):

- in native mode, hair and eyebrows are drawn (forward pass, alpha blend) but leave no trace;
- their AlbedoMap slot holds the 16×16 FLVER placeholder `FC_M_0000_hairoff_a`, found by name
  in the registry `[144792F58]`;
- the output alpha is therefore 0;
- the part masks are identical in both modes (`C4D160`), and the instance colour is
  (1, 1, 1, 1).

### Materials (game data, `mtd/AllMaterialBnd.mtdbnd.dcx`)

| MTD | Programme | Blend | Alpha ref | Notes |
|---|---|---|---|---|
| `P[ChrCustom_Hair]` | `SAT_HairIbl2` | 0 | 128 | **`g_GX_ForwardRendering=True`**: forward pass, alpha test (`[A]`) |
| `P[ChrCustom_Hair]_al` | `SAT_HairIbl2` | **2 (alpha blend)** | 1 | forward |
| `P[ChrCustom_Hair]_cloth`, `_al_Cloth` | `SAT_HairIbl2_cloth` | 0 / 2 | 128 / 1 | the same, with cloth |
| `C[Hair]` | `GXFlver_HairIbl_Bloody` | 0 | 0 | deferred (`Gbuf`), older hair |

- **Texture slots of the `SAT_HairIbl2` MTDs:**
  - `g_VectorTexture`;
  - `SAT_HairIbl2_snp_Texture2D_0_AlbedoMap_0`;
  - `…_1_NormalMap_0`;
  - `…_2_ReflectanceMap_0`.
- The pilot's `snp_0_Texture2D_0` is the **AlbedoMap**. Its alpha is the hair opacity.
- **`g_FaceGenType`** (read by `FUN_140D40790` into the Sprj material-parameter set
  `+0x1C0`, vtable `142952508`, with `g_useHairColor` at `+0x1C4`) **exists only in the old
  `FRPG` MTDs** (`Ps_FaceGen[DS][M]` = 1). No DS3 hair or face MTD has it, so it is not the
  override rule.

### Programmes (DXBC of `GXFlver_HairIbl2`, `gxflvershader.shaderbnd.dcx`; reflection and disassembly)

| Programme | Inputs used |
|---|---|
| `_Fwd` / `_Phantom_Fwd` PS | `g_DiffuseTexture` t0, specular t1, bump t3, vector t23, IBL t30/t31, SSAO t33, RLR t44, light lists t59..t62; `cbMtdParam` b3 (`FC_DifMapMulCol`, `FC_SSAOParam`, row 5 alpha ref), `cbInstanceData` b4, `cbLight` b5, `cbFog` b6, `cbModelParam` b7 (rows 2 and 6), `cbSceneParam` b8, `cbPerFrame` b9 |
| `_Fwd_[A]` | the same, plus `cbMatDynParam` b2 `FC_isDitherAlpha` and the dither texture t38 |
| `_Gbuf_[A]` | five targets; the same alpha and discard |

**Output of `_Fwd`:**

- `o0.rgb` = lit colour × `Instance[7].rgb`, then fog;
- **`o0.a` = albedo.a × vertex colour.a × `Instance[7].w`**;
- the albedo rgb is `sat(sqrt(tex) × cbModelParam[6].rgb)²`.
  INFERRED: `cbModelParam[6]` is the part tint (hair colour 1, section 25).

**Discard in `[A]`** (any condition kills the pixel):

1. `cbMtdParam[5].x > 0` and the final alpha is 0;
2. `(dither − 0.5) × cbMtdParam[5].y + albedo.a·vertex.a < cbMtdParam[5].x`. This is the
   alpha reference, 128/255 for `P[ChrCustom_Hair]`;
3. dither fade: `cbModelParam[2].x > 0`, `FC_isDitherAlpha ≠ 0` and
   `(dither − 0.5)·0.999 + Instance[16].x < 0.5`.

### `cbInstanceData` (b4): 17 rows per instance, built by `FUN_14005B3D0`

| Row | Content |
|---|---|
| 6 | `.x` gparam value for the `Gbuf`/`Gbuf2` passes, else 0; `.y` flag from ctx `+0x128` |
| **7** | **group `+0xD0..+0xDC`: colour multiplier and fade alpha** (default (1, 1, 1, 1) by `FUN_14004DF20`) |
| 8..10 | ambient SH (LightSet `+0x290..+0x2BF`, A/B blend by group `+0x108`) |
| 11 | light scale (LightSet `+0x2C0`) |
| 12 | `+0xD0..+0xDC` of the material's draw object, unless its `+0xE1` |
| 13 | group `+0x148..+0x150`, `.w` = group `+0x13C` |
| 14, 15 | group `+0x154..+0x168` ranges, `.w` = group `+0x140` / `+0x144` |
| **16** | **`.x` = group `+0x124`** (dither alpha, default 1.0) |

The menu entity scales group `+0x124` by its visibility fade (`FUN_140D04B90`).

### The face-texture override: the path as far as it is read

1. `FUN_140D3DA20` (item slot 11) fills the draw struct (stack `-0x258`; this is section 26's
   "descriptor"):
   - **`+0xA0` = `[item+0x58]+0x358`, or `+0x360` when `item+0x6F3`**, provided `item+0x6F2`
     is set;
   - **then `[item+0x770]+0x30`** (the FaceGen texture) once `FrpgFgTextureIns+0x91`.
2. `FUN_140059130` → `FUN_140057A20` (struct = `param_8`) → **`FUN_14005C0E0`: draw record
   `+0x1D0` = struct `+0xA0`** (`14005C923`).
3. Replay: **`FUN_14005A120`** (pass context set-up; `param_3` is the record) → **draw context
   `+0x1B0` = record `+0x1D0` when it is non-null** (`14005AEB1`).
4. **Open:** the reader of draw-context `+0x1B0` that binds it to a texture slot was not found.
   - The candidates in the GX range with this offset belong to other classes:
     `FUN_140198910`, `FUN_14019FB20`, `FUN_14005B3D0` (`+0x1B0` of a model).
   - INFERRED rule: it replaces the AlbedoMap (the `snp_0` slot) of the materials of the parts
     that carry the flag, in place of the FLVER placeholder `…_hairoff_a`.
   - Runtime check: on a hair draw, read the SRV bound by `PSSetShaderResources` (context
     `vt+0x40`) for the AlbedoMap register, and compare it with `[item+0x58]+0x358`.

**The flags and the model-data textures:**

- `item+0x6F2` has a byte setter at `140D3E1E0` (`MOV [RCX+0x6F2],DL; RET`) with no plain-code
  caller. INFERRED: the encrypted `CSPartsModelIns` constructor sets it.
- `item+0x6F3` is set through `FUN_140C4CDC0(part, v)`, called only by the in-game
  `FUN_140648520` (bit 2 of state `+0xE0`; INFERRED "hide hair").
  - In a menu view it stays 0, so **`+0x358` is the texture used**.
- `[item+0x58]+0x358/+0x360` have no writer in plain code (the hits for those offsets belong
  to other classes). INFERRED: the hair-on and hair-off textures of the model, resolved by the
  encrypted model loader.

### Everything that can null the hair contribution (checklist for the pilot)

1. **AlbedoMap alpha 0.** Placeholder `…_hairoff_a` instead of `[item+0x58]+0x358`, or the
   FaceGen texture: **this is the measured cause**.
2. Vertex-colour alpha 0 (the FLVER's vertex colour; `v3.w` in the PS).
3. `Instance[7].w` (group `+0xDC`) = 0, or a fade from the entity (group `+0x124` × visibility).
4. `Instance[16].x` < 0.5 with dither enabled (`FC_isDitherAlpha`, `cbModelParam[2].x > 0`):
   every pixel is discarded.
5. Alpha reference: `P[ChrCustom_Hair]` uses 128, so an albedo alpha below 0.5 is discarded.
6. Blend state: blend mode 2 = alpha blend `SrcAlpha / InvSrcAlpha` (INFERRED from the GX
   blend table). A zero alpha writes nothing visible.
7. Depth:
   - forward draws come after the opaque body in the same view, with a depth test against it
     and depth write on (`g_GX_bDisableDepthWrite=False`);
   - a reversed depth function or a cleared depth gives nothing or everything.
8. Colour write mask: the forward blend states of the GX table keep RGBA (INFERRED).

Evidence: `evidence/I-0067/queries/hair/` (disassembly of the four `GXFlver_HairIbl2` PS, `mtd-hair-face.tsv`).

Exports: `decompiled/DS3-1152-F-0005B3D0`, `0005C0E0`, `00D40790`, `0004DF20` (existing),
`00D3DA20` (existing), `00057A20` (existing), `00C4CDC0`, `00648520`, `00198910`.

## 45. Section 43's open points: angle units, the layer stack, and a correction to TONEMAP-SKIN section 10 (STATIC + game data; INFERRED where marked)

### LightSet angles are radians in the file (no conversion)

- `default.gparam` stores the `LightSet` group's `Angle` parameters as GPARAM **type 12**,
  two floats per value. Read with `Get-GparamValues.ps1`; the file is
  `evidence/I-0067/queries/status-light/param_drawparam_default.gparam.dcx`.
- Example, value id 0 (directional lights 0..3):
  - (−0.837758, 2.30383), that is −48° and 132°;
  - (0.383973, −2.3911);
  - (−0.174533, −2.6529);
  - (−0.523599, −0.820305).
  - These are radians: the multiples of π/180 are exact.
- The record binder copies field bytes as they are (`FUN_14014BD30`, `FUN_141FD0F30(dest, src,
  size)`). `FUN_14014BA90` feeds them straight into sinf and cosf.
- **There is no degree-to-radian step.** The `[deg]` parameters of other groups (for example
  `LightScattering` 光源X軸回転角度[deg]) are separate parameters with their own units.
- Native: `dir_i = (cos X·sin Y, −sin X, cos X·cos Y)` with the file values. Value id 0, light
  0 gives (0.497, 0.743, −0.448).

### The per-area layer stack (`FUN_140096560`)

Per area, the GX parameter system keeps a record of **17 layer slots** (0x20 bytes each; the
`param_4` index is the layer):

| Slot field | Content |
|---|---|
| `+0x00` | file handle (−1 when empty) |
| `+0x08` | active byte (`weight > 0`) |
| `+0x14` | weight (float) |

The layers in use:

| Layer | Source |
|---|---|
| 1 | `default.gparam` (kind 1; table `14294EFE8`) |
| 2 | template files (kinds 2..5) |
| 3 | kind 13 |
| 4 | kind 6 (`MapBase`) |
| **5..9** | **map files `mAA_BB_XXXX` (kind 7)**: slot 0 (always file 0) and slots 1..4 (from the collision part), section 43 |
| 10, 11, 12, 13, 14 | kinds 8, 9, 10 (`RemoBase`), 11 (cut-scene `s` files), 12 (`eclipse`) |

- Per-part keys (`FUN_140CC8180`) use layer `FUN_140D27BA0(9)` = 11.
- Registering a file (`FUN_140049D00` → `FUN_140096560`) sets the slot's handle and weight.
  When the weight is positive, `FUN_140098170` marks every record of that area dirty, and its
  values are rebuilt from the active layers.
- Withdrawing a file (`FUN_14004A060`) clears the slot.
- The per-id records are created for every value id found in any layer (`FUN_140030D80` →
  `FUN_140096310`).
- `FUN_140096DA0(area, …, id)` asks each layer, through the group object `vt+0x58`, whether it
  defines that id.
- **INFERRED merge rule:** the per-value rebuild through the group objects (`vt+0x58/+0x70`)
  was not read. The layout (ordered slots, one weight per slot) suggests the following, to
  confirm at runtime:
  - layers apply in increasing order;
  - each layer that defines the value lerps the accumulated value towards its own by its
    weight;
  - so a higher layer overrides a lower one, with the fades of section 43 as weights.

### Correction to TONEMAP-SKIN section 10

- Step 3 there says that `+0x2D0` "receives the map rotation (`FUN_1400362B0`)".
- In fact `FUN_1400362B0` decomposes the gparam file's own transform, CSGparamResCap
  `+0x80..+0xAF`. **Only the constructor `FUN_140D26700` writes it, as identity**: no
  `CSGparamResCap` or `CSGparamRepositoryImp` slot writes it (vtables `14294EF00`,
  `14294ECB0`, all in the scanned range).
- So `+0x2D0` is the identity quaternion for every shipped file (section 43).
- The `.btpb` SH probes are different: they **are** rotated by the map transform
  (`[[map+8]+0x28]+0x5C0`, `FUN_140003FB0`).

Exports: `decompiled/DS3-1152-F-00096560`, `00098170`, `00096DA0`, `00096310`, `0014BD30`
(section 43 path).

## 46. Hair alpha in `SAT_HairIbl2`, the alpha-reference constant, the scene camera's output object, and the phantom lookup fallback (STATIC; INFERRED where marked)

### Correction to section 44: the DS3 hair programme is `SAT_HairIbl2`, with no vertex colour

Section 44's PS analysis used `GXFlver_HairIbl2`. The `P[ChrCustom_Hair]` MTDs use
**`SAT_HairIbl2`** (`shaderbdle.shaderbdlebnd.dcx` → `SAT_HairIbl2.shaderbdle`). The DXBC
comments give the constant-buffer names.

- **VS input** (`SAT_HairIbl2_Fwd.vpo`):
  - POSITION, NORMAL (uint), TANGENT (uint), TEXCOORD0 (int), TEXCOORD1 (float);
  - `_Skin` adds BLENDINDICES and BLENDWEIGHT.
  - **There is no COLOR**: FLVER vertex colours have no effect on hair.
- **PS `SAT_HairIbl2_Fwd_[A]`:**
  - t3 = AlbedoMap (sampler s0); `a = t3.a`;
  - the colour term is multiplied by `a` (`mul r4, r4, (1,1,1,a)`);
  - **`o0.w = a × mulCol.w`**;
  - discard when any of these holds:
    1. **`a² + (dither − 0.5)·FC_AlphaRef.y < FC_AlphaRef.x`**;
    2. `FC_AlphaRef.x > 0` and `o0.w = 0`;
    3. `cbModelParam[2].x > 0`, `FC_isDitherAlpha ≠ 0` and
       `(dither − 0.5)·0.999 + ditherFadeAlpha < 0.5` (dither texture t38 at screen/4).

`PerInstanceData` (`cbInstanceData` b4, two instances, 532 bytes), named by the DXBC:

| Offset | Field | Source (section 44) |
|---|---|---|
| 0 | `mWorld` (float4x3) | |
| 48 | `atlasUvScaleOffset` | |
| 64 | `userClipPlane` | |
| 80 | `matricesData` (uint4) | |
| 96 / 100 / 104 | `envData` / `dynamicLightDisable` / `envPad` | row 6 |
| **112** | **`mulCol`** | group `+0xD0..+0xDC` |
| 128 | `aSh[3]` | LightSet `+0x290..` |
| 176 | `ShLightMask` | |
| 192 | `vDominantRotateParam` | |
| 208 / 224 / 240 | parallax-corrected cubemap origin, AABB and env scales | |
| **256** | **`ditherFadeAlpha`** | group `+0x124` |

### `FC_AlphaRef` (`cbMtdParam` b3, offset 80): `GXFlverMaterialParamAlphaRef` (vtable `143D60648`, slot 2 `FUN_140200D40`)

- If `g_AlphaRef.x ≠ 0`:
  - **`x = (ref − 0.0001) / 255`**, **`y = ref2 / 255`** (constants `143D7ED00` = 0.0001 and
    `143D7EFF4` = 255);
  - `z = 1.0` when the material's dither object (`+0x20` → `+0x10`) is set, else 0.
- If `g_AlphaRef.x = 0`: `(−100, −180)` (`143D7F118`), which never discards.
- `P[ChrCustom_Hair]` (128, 0) → x = 0.50196: the pixel is discarded when `a² < 0.502`, that
  is `a < 0.708`.
- `P[ChrCustom_Hair]_al` (1, 0) → x = 0.0039.
- **Native:** passing the MTD value without the `/255` (x = 128) discards every hair and
  eyebrow pixel, even with the right texture and `mulCol = 1`.

### Scene camera at zone load: `FUN_140CED670` and the GX camera object (pilot's `BuildSceneView` refusal)

- **`FUN_140CED670(cam, out)`** writes only:
  - `out +0x00..+0x3F`: the pose (`FUN_14004B040`);
  - `+0x50`: fov, re-derived when the aspect is below `142945248`;
  - `+0x54/+0x58`: near and far (cam `+0x58/+0x5C`);
  - `+0x5C/+0x60`: screen size (globals `1446601E8/EC`);
  - `+0x64`: aspect (cam `+0x54`).
- **It never writes `+0x68` or `+0x70`.**
- In the caller `FUN_140D1DE70`, `out` is a stack local built by the GX camera constructor
  **`FUN_14004AFF0` → `FUN_14004B5C0`**:
  - identity pose; fov from `14473F6B8`; near 0.1; far 5000;
  - size from `[GX+0x88]+0xF8/+0xFC`; aspect 0;
  - **`+0x68`: one byte set to 0**;
  - **`+0x6C..+0x83`: three qwords set to 0** (so `+0x70` = 0).
  - **Bytes `+0x69..+0x6B` are never written**: they keep stack garbage in the original too.
- The object then goes to the pipeline (`FUN_14006E9E0`): `+0x6C..+0x80` are copied to
  `+0x2AC..+0x2C0`.
  - INFERRED: `+0x68` is a special-projection flag, and `+0x6C..+0x80` are projection
    offset, tile or sub-rectangle words.
  - None of them is set on the zone-load path.
- **Native:** read `+0x68` as a byte, or zero the object before the original constructor runs.
  A 32-bit read of `+0x68` sees the three garbage bytes.

### Phantom pixel-programme lookup fallback (section 42's open point)

- The lookup goes through the set's repository adaptor (section 42) to
  **`GXFlverShaderContainer`** (vtable `143D42F78`): slot 6 `FUN_1400714D0` (PS) and slot 4
  `FUN_1400711F0`.
- These lowercase the name (`FUN_141FD2CF4`) and look it up in the container's map
  (`+0x68`).
- **When the name is absent they return 0** (`RDI` is kept null; the found node's `+0x50`
  otherwise).
- So for a shader without `_Phantom_` programmes, `FUN_1400A9EF0` writes null into the
  `Fwd`/`FwdSdw` pixel entries.
- INFERRED: those sets are never drawn in the forward passes, or the forward pass of a
  null-PS set is skipped (the getter returns the entry as is).

Evidence: `evidence/I-0067/queries/hair/` (`SAT_HairIbl2` disassembly added).
Exports: `decompiled/DS3-1152-F-00200D40`, `00CED670`, `00D1DE70`, `0004AFF0`,
`0004B5C0`, `0006E9E0`, `000714D0`, `000711F0`.

## 47. The phantom family for the native pilot: constant layout, colours per phantom type, activation, missing programmes (STATIC + regulation; INFERRED where marked)

### 1. Constant layout (lifts the INFERRED of sections 13 and 42)

- The material constant block that `FUN_14022E930` writes (`[ctx+8]`) is **`cbMatDynParam`,
  register b2**: the DXBC of `SAT_HairIbl2_Phantom_Fwd` places `FC_SSS_ID` at 400, matching
  section 13's `+0x190`.
- The phantom fields of `cbMatDynParam`:

| b2 offset (row) | DXBC name | Written from (block offset, `ExParam` index) |
|---|---|---|
| 416 (26) | `FC_PhantomEdgeColor` | block `+0x80`, index 10: `rgb²·a·s`, `.w = a`; `s = −1` when `IsEdgeSubtract` (index 8, block `+0x70`) |
| 432 (27) | `FC_PhantomFrontColor` | block `+0x90`, index 11: the same, `s` from `IsFrontSubtract` (index 9, `+0x71`) |
| 448 (28) | `FC_PhantomDiffColor` | block `+0xA0`, index 12: `rgb²·a` |
| 464 (29) | `FC_PhantomSpecColor` | block `+0xB0`, index 13: `rgb²·a` |
| 480 (30) | `FC_PhantomLightColor` | block `+0xC0`, index 14: `rgb²·a` |

- The builder `FUN_14022D620` runs only when the block flag `+0x22` is set. It takes each value
  from the draw's override list (`FUN_140146850(list, key, index)`, else by name CRC
  `FUN_140146920`, hashes `144962330..58`). It lerps the block default towards that value by
  the entry's weight `[5]`. The subtract flags are lerped as bytes.
- **What the PS does with them** (`SAT_HairIbl2_Phantom_Fwd`, end of the shader):
  - `diffuse = albedo × DiffColor.rgb`;
  - `lightTerm = r7·r2.x + saturate(−N·L)·LightColor.rgb + r8` (dynamic lights);
  - `spec = (spec_accum + saturate(−r8.x)⁴·LightColor.rgb) × SpecColor.rgb`;
  - `rim = lerp(EdgeColor, FrontColor, (clamp(|N·V|, 0.1, 0.7) − 0.1)/0.6)`;
  - `rgb = max(diffuse·lightTerm + spec + rim, 0) × mulCol`; `a = albedo.a × mulCol.w`;
  - then the usual height and distance fog.
  - **There is no IBL term** (WITHDRAWN 2026-10-11, PHANTOMS-SIGNS-NATIVE section 4: the DXBC of every `_Phantom_Fwd` still samples the two IBL cubes t30/t31 like the normal `_Fwd`). The negative edge/front colours (the
    subtract flags) darken the rim.

### 2. Which PhantomParam row: per phantom type (sections 29 and 42)

The resolver `FUN_14088A950` (per character, every frame, through `ChrIns` slot 36 →
`FUN_1409CAF30`) takes, in order:

1. `chr+0x1F38`;
2. `chr+0x1A58`;
3. `NpcParam+0x1CC`;
4. `NpcParam+0x238` (when the team type is flagged);
5. **RoleParam** of the role, column `+0x4`, or `+0x4C/+0x50/+0x54` by covenant rank (vow
   bytes cut at 10/30/80).

Colours decoded from the rows (bytes `+0x14..+0x22` edge, front, diffuse, specular, light;
weights `a` at `+0x00..+0x10`):

| PhantomParam | Edge | Diffuse / specular | Light | Used by (RoleParam / NpcParam) |
|---|---|---|---|---|
| **80** | white (255,255,255) a .5 | white a .5 / white a 1 | — | role 1, 50001, 80001 (white summon); 29 NPC rows (`+0x1CC`) |
| **81** | **red** (255,0,0) a .6 | white a .2 / .2 | — | role 2, 10002, 50002, 80002, 90002 (invaders); 16 NPC rows |
| 82 | blue (30,100,200) a .5 | white .1 | (200,230,255) | role 90001, 90013 |
| 83 / 84 | pale blue (190,200,255) | blue (60,80,255) a .4 | (120,170,200), light a 5 | roles 10001, 10013 by rank |
| **85** | **orange-gold** (255,170,0) a .9 | (255,…) a .3 | — | role 20001 (INFERRED the gold "yellow" summons) |
| 86 | red (255,0,0), front (255,50,0) | (255,180,…) a .3 | (255,180,0), light a 4 | roles 20002, 20013 |
| 88 | magenta (220,0,180) | white .1 | (255,120,0) | roles 30001, 30002, 30013; NPC 21200, 27720, 27740 |
| **91** | (100,160,255), diffuse **magenta-purple** (255,0,200) | spec (255,0,150) | (255,0,140), light a 3 | roles 40001..70013 (INFERRED the purple phantoms) |
| 76 | white a 1 | — | grey (160,160,160) | NPC `+0x238` route (88 rows) |
| 60 | red a .6 | white .2 | — | NPC 8902xx..8905xx |

The full rows are in `evidence/I-0067/queries/phantom/phantomparam.tsv`, and the RoleParam
table is in section 29. Covenant names are INFERRED from the colours; the role ids are
STATIC.

### 3. Activation

1. **Programmes.** For the 64 families of section 41, the shader set's pixel table always
   holds the `_Phantom_` programmes in `Fwd` and `FwdSdw` (section 42: `FUN_1400A9EF0` runs
   last in `FUN_140082E50`).
   - There is no per-character programme switch.
   - **Being drawn in the forward pass is what makes a character a phantom.**
2. **Forward routing** (`FUN_140057A20`, an opaque material with descriptor kind 2):
   - with group `+0x85 = 0` and `+0x84 ∈ {0xFF, 0}`:
     - group `+0x87` set sends the draw to list **0x21**;
     - when, in addition, the fade **`+0xDC` < 1** (`mulCol.w`), it goes to **0x29**;
   - with `+0x85 = 1` or `+0x84 = 1` and fade = 1: list 0x21 (with further conditions);
   - otherwise 0x29;
   - the global `144962314` forces 0x29.
   - INFERRED: 0x21 and 0x29 are the forward opaque and forward translucent lists. The phantom
     `ChrIns` sets `+0x87` and the fade on its draw group (`entity+0x240`). The writer of those
     group bytes was not read.
3. **Constants.** Every frame: `ChrIns` slot 36 → `FUN_1409CAF30` (id ≥ 0) → the seven
   `ExParam_Phantom*` in the draw's override list → `FUN_14022D620`/`FUN_14022E930` → b2
   rows 26..30.
   - Menu views never get them (section 26).
4. **Missing `_Phantom_` programme** (section 46): `GXFlverShaderContainer` returns 0, so the
   `Fwd`/`FwdSdw` pixel entries of that set are **null**.
   - In the original, a forward draw of such a material binds `PSSetShader(null)`
     (`FUN_140142430` passes the null through). INFERRED: with no pixel shader D3D11
     rasterises depth only, so the part is invisible in colour.
   - This concerns only shaders outside the 64 families. All character, equipment, hair, skin,
     face and eye families have the programmes.
   - **Native:** skip the colour draw for a null PS (depth only, to match), or log it. Do not
     substitute the normal `_Fwd` PS: the original does not.

### Native checklist

1. `Fwd`/`FwdSdw` PSOs of the 64 families with the `_Phantom_` PS (`[A]` for alpha-ref
   materials).
2. b2 (`cbMatDynParam`) rows 26..30 from PhantomParam, through the `rgb²·a`
   transformation, with the subtract sign on edge and front.
3. The PhantomParam row chosen by the resolver order above (RoleParam for players, NpcParam
   for NPCs).
4. `mulCol` (instance row 7) carries the phantom fade alpha. The forward-translucent list
   (0x29) is used when it is below 1.

Evidence: `evidence/I-0067/queries/hair/SAT_HairIbl2_Phantom_Fwd.asm` and `…_Phantom_Fwd_A.asm`.
Exports: `decompiled/DS3-1152-F-0022D620`, `0022E930`, `0022E0F0`, `00057A20` (existing),
`0088A950`, `009CAF30`.

## 48. Shadow in the Player menu views (the hood shadow): light, map, parameters (STATIC + game data; INFERRED where marked)

This section completes sections 19, 22 and 25. **Only the Player family casts shadows**:

- `ChrMakePreview*`, `StatusBody` and `RankingBody` use pipeline flags `0x400852201F`, with
  bit 0 set and the `GXSgShadowSceneImpl` sub-scene;
- the Face and Prof families (`0x8422014`) have neither.

### The shadow map

- **Pipeline set-up `FUN_140085540`, when flag bit 0 is set:**
  - one shadow atlas at **`pipeline+0x1520`** (`FUN_1401B5390`);
  - its size comes from the descriptor (`2·[GXRS+0x4E90]`, or 0x400 when that is 0, set by
    `FUN_1400C3410`), scaled by the global shadow quality `[GXRS+0x5360]`:
    `size·[+0x2C]/[+0x30]`, clamped to `[+0x34, +0x38]`;
  - then `pipeline+0x3EBC = 1`.
  - Flag bit 28 would add a second, double-size atlas (`+0x2160`, `+0x3EBD = 1`). The Player
    flags do not set it.
  - Without bit 0, `+0x3EBC = 0`.
- **Per frame** (`FUN_1400F90F0` and `FUN_1400F9B50`, the shadow preparation): the cascades are
  built only when `pipeline+0x3EBC`, `[[GXRS+0x5360]+0x28]` (global shadows on) and
  `[GX+0x3E1] = 0` all hold.
- Casters: the view layer's items, the character itself. This is what puts the hood's shadow
  on the face.

### Parameters: ShadowParam `{1, id}`, with the id from `MenuOffscrRendParam +0x2C`

- `FUN_140088AF0` (section 17) sets the ShadowParam keys `+0x3FA8/+0x3FB0` to `{1, id}`.
- `FUN_140088A10` blends the two bank records (`[GXRS+0x4670]`, `FUN_14018C590`) into
  `pipeline+0x90` (`FUN_1401DAF40`); the record id lookup stamps the area at record `+0x30`.

Values in `default.gparam` (area 1) for the Player views' ids:

| ShadowParam | **id 11** (StatusBody, ChrMake row 0) | **id 10** (ChrMake face close-up, row 1) |
|---|---|---|
| Angle (rad) | **(−1.01229, 2.51327)** = (−58°, 144°) | **(−0.733038, 2.42601)** = (−42°, 139°) |
| Shadow Light Id | **−1** | **−1** |
| Near | 2 | 0.1 |
| Cascade splits | 5 / 5.5 / 6 | 1.2 / 1.8 / 2.5 |
| Far fade start / dist | 8 / 10 | 2.5 / 3 |
| FilterMode | 2 | 2 |
| Depth Offset / Slope bias | 0 / 0.005 | −0.001 / 0.005 |
| DepthRate / DepthScale / KernelScale | 0.23 / 15.417 / 0.67 | 0.75 / 7.083 / 2 |

(ids 12..15 have the id-10 values; id 0 uses (−0.750492, 0.942478).)

### The direction: ShadowParam's own Angle, not the sun (corrects section 22's INFERRED)

`FUN_1401DAF40` computes a direction from the blended ShadowParam Angle, with the same formula
as the light sets:

- `d = (cos X·sin Y, −sin X, cos X·cos Y)`;
- stored at state `+0x10`;
- the light-set keys `{area, Shadow Light Id}` at `+0x20/+0x28`, the weight at `+0x30`.

`FUN_1400F90F0` then chooses the direction:

| Keys | Shadow direction |
|---|---|
| **Shadow Light Id = −1** (A or B key invalid) | **state `+0x10`**, the ShadowParam Angle |
| only key B invalid | the light-set record of key A, `+0x40` (light 0 direction) |
| both valid | wrap-lerped light-0 angles of both light sets, rotated by the slerped `+0x2D0` (section 43) |

- Every menu id has **Shadow Light Id = −1**, so **the menu shadow uses the ShadowParam Angle**.
- It is not the direction of the LightSet's directional light 0: for id 11 the light 0 angle
  is (−0.837758, 2.30383), the shadow angle (−1.01229, 2.51327).
- Directions for id 11: d = (0.3115, 0.8480, −0.4287); for id 10: d = (0.4876, 0.6691, −0.5609).
  INFERRED: the vector points towards the light, as for the light sets.

### What native mode must provide for a Player view

1. A shadow atlas per Player view pipeline (size above), cleared each frame, with the
   character as caster.
2. Four cascades from ShadowParam `{1, id}`: Near, the three splits, the far fade and the
   filter, bias, depth-rate and kernel values above.
3. **The light direction from the ShadowParam Angle when Shadow Light Id = −1**, through the
   formula above, in world space with no MapOffset.
4. The receiving forward and G-buffer passes sample this atlas with the shadow colour
   (`Shadow Color` `0xFF000000`) and the blur settings (count 4, radius 8).
5. Face and Prof views: no shadow pass.

Exports: `decompiled/DS3-1152-F-00085540` (existing), `00088A10`, `0018C590`, `001DAF40`,
`000F90F0` (existing).

## 49. The face grid (`FaceSelectDialog`, "Similar Face"): opening path and its fifteen render views (STATIC; INFERRED where marked)

### Opening path

1. **Appearance → Face Detail → Similar Face** (section 42): the creation menu row
   `CharMakeMenuTopParam` 159 (or 18), entry kind 12, text 420049.
2. The builder `FUN_140B1E7F0` wraps kind 12 in the lambda `1428E4838` (`FUN_140B24990`). On
   confirm it runs **`FUN_140B1AA00`**:
   1. `FUN_140B1C380`: the creation object's template provider `[+0x18]` `vt+0x10` fills a
      0xEA0-byte template list. INFERRED: the faces similar to the present one; the generator
      is virtual and was not read.
   2. It copies the list into a 0xEB0-byte job (vtable `1428E48B8`, `+0x1D4` = creation
      context).
   3. It calls **`FUN_140B29580(menu, owner, templates, job)`**.
3. **`FUN_140B29580`** builds the dialog:
   - Scaleform page **`04_050_ChrMake_IconGrid`** (`1428E2448`);
   - **for each template slot i < `templates+0xE90`** (at most 15; 0xF8-byte appearance records):
     - `FUN_140B20070(i, i, &record_i + 4, nudeFlag)` creates the view;
     - the image name **`SYSTEX_Menu_ChrMakeGrid%02d`** (`1428E2A40`) replaces the movie's
       placeholder **`MENU_DummyFace_%02d`** (`1428E2A28`) through the image path of
       section 33.
   - The other two sites, `FUN_140B17E90` (Save to Favorites) and `FUN_140B18750` (Load
     Favorite), fill the list from the 15 saved slots instead.
4. **Closing:** the dialog destructor `FUN_140B1E630` → `FUN_140B21F70` sends the fifteen views
   to the deferred queue (section 35).

### The fifteen views (`FUN_140B20070`)

| Field | Value |
|---|---|
| Class | `CSMenuFaceModelRend` (0x910 bytes, ctor `FUN_140D69770(view, row = i)`) |
| Size-table row | **i** = 0..14 → `SYSTEX_Menu_ChrMakeGrid00..14`, **128×128**, Face family |
| Pipeline | default descriptor, flags `0x8422014`: **no shadow**; Paint and Base sub-scenes (section 25) |
| Appearance | `FUN_140D675A0(view, &template_i.appearance)`: template i, not the working character |
| Equipment | `FUN_140D69BE0(view, [[1447572B8]+0x10]+0x2B0, nudeFlag)`: the working character's equipment, nude when the flag is set |
| Flags | `+0xAA` of the working character (`FUN_140D67670`); `+0x10C` unless nude (`FUN_140D67680`) |
| Request | `FUN_140D67540` (request) + **`FUN_140D67560` (snapshot)**: one frame per request (section 24) |
| Start delay | **`FUN_140D668A0(view, i·2)`**: slot i starts 2i updates later |
| Registry | `14478D940[i]` |

**Camera and gparam** (`MenuOffscrRendParam`, section 19, the ChrMakeGrid row):

- target (0, 1.58, 0), distance 1.2, angles (1.719, −14.85), fov 15°;
- gparam id **10**: the key `{1, 10}` for every bank; ShadowParam is unused here (no shadow
  sub-scene);
- background is Setting Info "BG Color" of id 10.

### What native mode needs to open and fill the grid

1. The kind-12 entry path to `FUN_140B1AA00` (unchanged game code). The template provider must
   return a list with `+0xE90` > 0, otherwise no view is created.
2. Up to fifteen Face views, rows 0..14:
   - each its own 128×128 target;
   - FaceGen texture and ChrAsm build per template;
   - snapshot on request, with the 2i staggered start;
   - **all fifteen at once**: the heaviest menu case.
3. The `img://SYSTEX_Menu_ChrMakeGrid%02d` registrations, so the IconGrid movie finds the
   fifteen images.
4. Release through the deferred queue when the dialog closes.

**Runtime checks:**

- a break on `FUN_140B29580`, then `templates+0xE90` (the count);
- `14478D940[0..14]` non-null;
- each view's `+0x6C8` request and its readiness gate (section 25).

Exports: `decompiled/DS3-1152-F-00B1AA00`, `00B29580`, `00B20070` (existing), `00B1C380`.

## 50. In-game menus: pages, how each opens, offscreen views, icons, and the profile confirmation (STATIC; INFERRED where marked)

### Every Scaleform page of the shipped menus (`menu:/%s.gfx`, `menu:/Win/%s.gfx`)

| Group | Pages (`.rdata` UTF-16 names) |
|---|---|
| 01 common | `01_000_FE`, `01_010_MessageBox`, `01_020_Sign`, `01_030_BloodMessage`, `01_040_GestureTop`, `01_050_GraveMessage`, `01_060_Caption`, `01_070_CommandList`, `01_080_EmergencyNotice`, `01_090_SummonMessage` |
| **02 in-game** | `02_000_IngameTop`, `02_010_EquipTop`, `02_020_Inventory`, `02_030_Inventory_CommandList`, `02_040_OptionSetting`, `02_050_DetailStatus_Player`, `02_060_Kick`, `02_070_Status`, `02_080_Warp`, `02_090_Ranking`, `02_100_AllocationOfEstus`, `02_110_DetailKeyGuide` |
| **03 NPC / bonfire** | `03_000_ShopTop`, `03_010_LevelUp`, `03_020_Spell`, `03_030_Comparison_equip_Weapon1`, `03_040_EnhancedWeapon`, `03_050_ItemBox`, `03_060_TalkChoice1`, `03_070_UseWarpItem1` |
| 04 creation | `04_000_ChrMake_Top` … `04_100_ChrMake_BG` (sections 40, 42, 49) |
| 05 title | `05_000_Title`, `05_010_ProfileSelect`, `05_020_TitleInformation`, `05_040_TermOfService` |

### How they open

- **Pause menu:** Esc / Start → menu input mapper (section 2) → state[8] = 7 → NewMenuSystem
  → name `Top` in `FUN_140B52030` → **`FUN_140B53AA0`**:
  - **`02_000_IngameTop`** (`FUN_140B66A70`);
  - or **`FUN_140B537E0`** ("System") when `FUN_140A67B80({0x14})`.
  - Its command entries are functors. Status is one of them (`140B66C21` → `FUN_140B52C90`).
- **By name: `FUN_140B52030(owner, name)`** (from `FUN_140B3EBB0`; talk scripts and NPC menus
  open dialogs this way):

| Name (`.rdata`) | Constructor → page |
|---|---|
| `Inventory` | `FUN_140B515C0` → `02_020_Inventory` |
| `EquipTop` | `FUN_140B4F3D0` → `FUN_140B798A0` → `02_010_EquipTop` |
| `EquipSelect` | `FUN_140B53EC0`/`FUN_140B4F320` (job) |
| `Warp` | `FUN_140AF2B40` (job; the page `02_080_Warp` is built by `FUN_140AF2C60`, from `FUN_140BE7820`) |
| `EstusFlaskAllocate` | `FUN_140B4F410` → `02_100_AllocationOfEstus` |
| `Depository` | `FUN_140B4F180` → `03_050_ItemBox` |
| `Status` | `FUN_140B52C90` → **`FUN_140B5BD30` → `02_070_Status`** |
| `System` | `FUN_140B537E0` |
| `LevelUp` / `DeceasedLevelUp` / `ZeroSoulsLevelUp` / `StatusReallocate` | `FUN_140B52AF0` / `FUN_140B4EFE0` / `FUN_140B53B10` / `FUN_140B53640` → **`03_010_LevelUp`** |
| `Item/Buy` / `Item/Sell` / `Item/Repair` / `Item/Reinforce` | `FUN_140B50430(…, 2000)` / `FUN_140B51280` / `FUN_140B50F90` / `FUN_140B50B00` (Inventory-style page `FUN_140B5ABB0`; reinforcement → `03_040_EnhancedWeapon` `FUN_140B59F30`) |
| `CharaMake` / `IngameCharaM…` | in-game character creation (`FUN_140B52AA0`; section 40 path B) |
| `Ranking` | `FUN_140B52D60` → `02_090_Ranking` |

- **Attunement** (`03_020_Spell`, `FUN_140B51A30`) is not in the name table: it opens from
  `FUN_140B48930` and `FUN_140BE8E10` (talk menus).
- `03_000_ShopTop` / `03_060_TalkChoice1` / `03_070_UseWarpItem1` are set by
  `FUN_140A55030`/`FUN_140A55070` (talk-menu window descriptors).

### Offscreen 3D views in these menus

**Only the Status screen has one** (section 31: every menu offscreen view is a character).

- `PlayerStatusMenu` (`FUN_140B71320`, built by the factory `FUN_140B48CB0`, which is
  referenced only as data):
  - **`SYSTEX_Menu_StatusBody`**: Player row 4, gparam 11, shadow (section 48), continuous;
  - **`SYSTEX_Menu_StatusFace`**: Face row 18, gparam 13, snapshot.
- `02_050_DetailStatus_Player` (`FUN_140B75340`, from `FUN_140B75450`, called by
  `FUN_140B6AE70`, `FUN_140B772F0` and `FUN_140B89E00`) is the stats panel of the equipment
  and level-up screens. **No 3D view.**
- **Equipment, inventory, shop, level-up, attunement, smithing, item box: no 3D model**, only
  icons and text.
- INFERRED link: `02_070_Status` (`FUN_140B5BD30`) is the page that hosts `PlayerStatusMenu`.
  The factory `FUN_140B48CB0` has only a data reference.

### Item icons: TPF sheets bound to placeholder images

- The movies reference **`MENU_DummyIcon_<sheet>`** images.
  - **`FUN_140E5DB90`** (the container's movie loader, from `FUN_140E5A810`/`FUN_140E5B670`; corrected in section 52)
    registers 17 or more `{MENU_Icon_xxxxx, MENU_DummyIcon_xxxxx}` pairs (`.rdata
    142990B78..`, 0x28 apart): sheets `00000`, `00001`, `01000`..`01002`, `02000`,
    `03000`..`03003`, `04000`, `04001`, `05000`, `06000`, `07000`, `08000`, `09000`, ….
  - It does so through `FUN_140E58EA0` and `FUN_140E52CE0` into the resource map
    `+0x9A8`/`+0x988`.
- Sheet files: **`menu:/%s.tpf`** (`FUN_140E5F340`; `menu:/Knowledge/%s.tpf` and
  `menu:/MapImage/%c%c.tpf` for those families).
- Equipment slot icons use `EquIcon_%04d`.
- **Native divergence points:**
  1. each sheet's TPF must be loaded and registered under its `MENU_Icon_xxxxx` name before
     the movie asks for `MENU_DummyIcon_xxxxx`; a missing sheet leaves the placeholder;
  2. the image path is section 33's (`ImageCreator` → registry `[144792F58]`): the same
     contract as the `SYSTEX_*` views, with TPF textures instead of render targets;
  3. the icon UVs come from the movie (the sheet is an atlas). The renderer only supplies the
     texture.

### The profile confirmation ("Start with selected profile?", 406011)

- `FUN_140ADB920` (Load Game → slot) calls **`FUN_140BBBD50(owner, 406011, {100, 2, 0x29,
  1428B8150})`**:
  - a generic **MessageBox** window (`FUN_140BB7040`, desc kind 0x14, page
    **`01_010_MessageBox`**, command list `01_070_CommandList`);
  - two choices (`FUN_140BBE790`), **default cursor on the last one** (`+0x20 = count − 1`);
  - result chained to the load action `FUN_140AE9DD0` through `FUN_140BAFE10`.
- **Input:** handled by the NewMenuSystem update, which is encrypted (section 2), through the
  input context's actions. No dialog-specific delay or key test exists in plain code.
- Native observation: `E` opens and confirms, while click, Enter and Space do not.
  - Whether the original accepts the click there is a runtime question.
  - If it does, the native gap is in the Scaleform mouse route: coordinates through the
    movie viewport against the native window size, or events not delivered. It is not in
    rendering: the Scaleform hit-test runs on the CPU against the timeline.

### Native checklist for the in-game menus

1. Pages load from `menu:/` and `menu:/Win/` (`.gfx`), the same HAL as the title (section 36).
2. The icon sheets above, registered under their `MENU_Icon_*` names.
3. The Status screen's two views (sections 24–27, 48); no other 3D.
4. Every NPC and bonfire menu is opened by name through `FUN_140B52030` (game code, unchanged)
   or by the talk-menu functions. Nothing renderer-specific.

Exports: `decompiled/DS3-1152-F-00B52030`, `00B53AA0`, `00B66A70`, `00B798A0`, `00B515C0`,
`00B5BD30`, `00B52C90`, `00B75340`, `00B48CB0`, `00E5DB90`, `00E5F340`, `00ADB920`,
`00BBBD50`, `00BB7040`.

## 51. Menu input, original against native: devices, the shared gate, and the menu hit-test's unit (STATIC; INFERRED where marked)

Context: in native mode the MessageBox 406011 (section 50) takes no choice. The pilot measured a
click at (492, 396) in 1280×720 presentation units, inside the drawn OK (x 390..596, y 384..409),
with the button bit going 1 then 0. E, Enter and Space do not validate either. The single centred
OK of the "quit" notice, on the same page, takes a click after a few tries.

### Devices and the window (unchanged in native)

- **Keyboard: DirectInput 8.** The device-kind mapper `FUN_1417DB650` returns GUID_SysKeyboard
  (kind 1, `143CB7950`) or GUID_SysMouse (kind 2, `143CB7940`).
  - Set-up `FUN_14183BEF0`: SetCooperativeLevel(hwnd `device+0x190`, flags) with **`0xA`
    (non-exclusive | background)** by default, `6` (non-exclusive | foreground) when config byte
    `+0x24F` is set, and `| 0x10` (no Windows key) when `+0x24C` is 0. Then Acquire.
  - Poll `FUN_14183BE70`: GetDeviceState(256 bytes, `device+0x1A0`). On DIERR_INPUTLOST it
    unacquires; on any failure it drops the device (`+0x198 = 0`). `FUN_14183BB70` re-applies the
    level and re-acquires when `+0x2A1` is set.
  - With the default background level, **the window does not need to be in the foreground** for
    the keyboard.
- **Mouse buttons: `GetKeyboardState`** in `FUN_1423891E0` (VK_LBUTTON, VK_RBUTTON, VK_MBUTTON):
  - per button: `+8` pressed edge (one update), `+9` released edge, `+0xA` held, `+0xC` hold time;
  - this reads the calling thread's key state, which is updated by that thread's message
    processing. The game's own window, WndProc and pump are untouched in native, so this holds.
- **Device update:** `FUN_142340050` (device manager, vtable `1444F8600`) calls the mouse update
  for each context that has one (`+0x80`), then the action contexts (`FUN_14233CDC0`). The
  confirm-type action update `FUN_140EBB5B0` sets `+0xC4` for one update on press, then repeats
  after 0.2 s every 1/15 s.
- No XInput or Raw Input import, and no AttachThreadInput. (Correction 2026-11-10: the executable does name `XINPUT1_3.dll`;
  how it loads it, by import or at run time, is not read; STUDY-R12-SETTINGS-PAGE §3.5.)

### The gate shared by every menu input query

Every UI query (`FUN_140A60A50` with a predicate functor, about 30 wrappers `FUN_140A5B9D0..`, and
the pointer `FUN_140A60700`) first calls **`FUN_140A5B930(ctx)`**. It is true only when all of
these hold:

1. `ctx[0] ≠ 0`;
2. `[[14478DA40]+0x98] = 0` (NewMenuSystem field);
3. MenuMan `state[0xAC]` (`[[144763258]+0x2E4]`) = 0. It is written only by `FUN_140765600`, from
   the in-game step `FUN_1408FB6D0`, so it is untouched at the title;
4. when NewMenuSystem `+0x140` is set (set on entering the game by `FUN_140764890`, cleared on
   leaving by `FUN_1407647D0`): fade plate 2 alpha (`[plate+0x38]`) ≤ 0.

At the title, term 4 is inactive. The pilot's pointer rows show button transitions, which
`NativeUiPointerState` only produces when this gate is true, so the gate was open at the click.

### The menu hit-test is C++, in units of the screen-size globals

The title list and dialog windows do **not** rely on Scaleform's own mouse handling. For example
`FUN_140A43160`, `FUN_140A44DE0` and `FUN_140A53AF0`:

1. `FUN_140A5C010` (click query);
2. `FUN_140A5B3F0`: the pointer position as two floats, the output of `FUN_140A60700`;
3. `FUN_140E63AA0(element, point, tolerance)`:
   - it converts the point to stage units with **`FUN_140E65E70`**;
   - it inverts the element's world matrix (`Matrix2x4::SetInverse`), maps the point into it
     and tests the element's bounds;
   - the list items go through `FUN_140A3FFA0` → `FUN_140A526C0` → `FUN_140E63AA0`.

**`FUN_140E65E70` interprets the point in units of S = (`[1446601E8]`, `[1446601EC]`)**:

```
F = movie frame rect (player vt+0x50), a = Fw/Fh, s = S.w/S.h
if a < s: ox = (S.w − a·S.h)/2, k = Fh/S.h      (pillarbox)
if a > s: oy = (S.h − S.w/a)/2, k = Fw/S.w      (letterbox)
else:     k = Fh/S.h
stage = (point − (ox, oy)) · k
```

S is the configured screen resolution:

- `FUN_140EDB220` reads it from the configuration;
- the setter is `FUN_140EDAA60`;
- `FUN_140EF50D0` sets it, then resizes the GX display to the same size (`FUN_14000B700`).

INFERRED: so the display `+0x198/+0x19C` that `FUN_1423891E0` scales the cursor by equals S, and
in the original the pointer and the hit-test share one unit.

### Native comparison

| Piece | Original | Native |
|---|---|---|
| Cursor position (`mouse+0x20`) | `display+0x198/0x19C` (= S) × client position / client size | **output size** × client position / client size (`NativeMouseDeviceUpdate`) |
| Pointer state `FUN_140A60700` | position − GX target viewport origin `[[144961AB0]+0x88]+0x100/+0x104` | position, origin 0 (`NativeUiPointerState`) |
| Movie viewport `FUN_140E53880` | buffer and rect = **S**, origin 0, flags 0, scale 1. Tile offsets only in the multi-target mode (`FUN_1400CC430`) | buffer and rect = **output size** (`NativeUiMovieViewport`) |
| Hit-test `FUN_140E65E70` | point in S units | unchanged: **still S units** |
| Buttons, keyboard, edges, gate | as above | unchanged (original bodies run) |

### Most probable cause (mouse): unit mismatch, S against the native output size

When S ≠ the output size, the point passed to the hit-test is scaled by `Fh/S.h` instead of
`Fh/720`. With S = 1920×1080 and a 1920×1080 stage (INFERRED: the configured resolution, not
measured):

- the click at (492, 396) is tested at stage (492, 396);
- the OK drawn at x 390..596 lies at stage x 585..894.
- **A click on the drawn OK misses.** A wide centred button can still be hit by clicking right
  of or below its drawn position, which fits "after a few tries".
- The draw is right because the native viewport and the native target agree with each other.
  Only the C++ test uses S.

**Runtime check:**

1. At the dialog, read the dwords `image+0x46601E8` and `image+0x46601EC`. Different from
   1280×720 confirms the mismatch.
2. Confirmation without code: click at (492·S.w/1280, 396·S.h/720). For S = 1920×1080 that is
   (738, 594). It should validate OK.
3. **Native fix:** compute the cursor position in `NativeMouseDeviceUpdate` with S instead of the
   output extent. That is the original contract (display = S). Leave the viewport as is: the hit
   test does not read it.
   - If S and the output have different aspect ratios, `FUN_140E65E70` adds a pillarbox or
     letterbox offset that the native draw does not have. Then also set S to the output size
     through `FUN_140EDAA60`.
   - That setter has other readers: `FUN_140CED670` camera size, the offscreen menu view sizes
     `FUN_140D6CFA0..D6D4E0`, `FUN_140D10DE0`, `FUN_140D56E90` and `FUN_140E51340`.

**Update 2026-10-11:** the remaining irregular failures of the confirmation are not a unit problem: a press that falls between two updates (one sample per update) is lost, and the native Load Game updates last 0.67 to 0.84 s, see [NATIVE-INPUT-LATCH](NATIVE-INPUT-LATCH.md).

### Keyboard (E, Enter, Space): not explained by S

- The keyboard and the decide action do not go through the pointer unit.
- Statically, no dialog-specific key test exists. The decide is an action resolved by the
  binding table (`FUN_14233DE40`), and the "A:OK" guide shows the pad glyph. Which keyboard keys
  are bound is data; it is not readable statically.
- **Runtime checks:**
  - the action object's `+0xC4` on E, Enter and Space at the dialog (`ObserveInputConfirmUpdate`
    already logs it);
  - the dialog's cursor index (`window+0x20`, set to `+0xF88 − 1` by `FUN_140BBBD50`);
  - the input-mode byte `[[14489F358]+0x18]` (`FUN_140A5BF70`, tested by the hover branch of
    `FUN_140A43160`).
  - If `+0xC4` pulses and nothing validates, the window refuses the decide; if it does not pulse,
    the binding is the gap.

Exports: `decompiled/DS3-1152-F-00A60700`, `023891E0`, `00A5B930`, `00E53880`, `00E63AA0`,
`00E65E70`, `00EDAA60`, `00EF50D0` (existing), `00EDB220` (existing), `00A43160`, `00A44DE0`,
`00A53AF0`, `00A60A50`, `00A5B3F0`, `00A5C010`, `00A5BF70`, `0183BEF0`, `0183BE70`, `017DB650`,
`02340050`, `00EBB5B0`.

## 52. Menu icon sheets end to end, and what native mode does with them (STATIC, native code read on the pilot branch; INFERRED where marked)

### Correction to section 50

**`FUN_140E5DB90` is the movie loader** of the UI container, not a separate resource set-up.

1. It looks the movie up in the cache `+0x9A8` (`FUN_140E58EA0`). Otherwise it loads it through
   the loader `+0x988` (`FUN_140E52CE0`), with the path `menu:/Win/%s.gfx` (`FUN_140E5F290`) and
   then `menu:/%s.gfx` (`FUN_140E5F1E0`).
2. Then, **only when MenuMan `+0x30` is set** (`FUN_140764170`), it binds the 17 icon sheets
   into the new movie.

### The 17 sheets

`MENU_Icon_` followed by `00000`, `00001`, `01000`, `01001`, `01002`, `02000`, `03000`..`03003`,
`04000`, `04001`, `05000`, `06000`, `07000`, `08000`, `09000`.

- Each is paired with `MENU_DummyIcon_<same>` (`.rdata 142990B78`, entries 0x28 apart).
- File path (`FUN_140E5F340`):
  - `menu:/%s.tpf` by default;
  - `menu:/Knowledge/%s.tpf` for names containing `MENU_Knowledge`;
  - `menu:/MapImage/%c%c.tpf` for `MENU_MapImage` names (two characters taken from the name).

### Original chain for one sheet

| Step | Function | What it does |
|---|---|---|
| 1. request | `FUN_140E5FC40(container, name)` (from the container update `FUN_140E5C080`), or `FUN_140E605B0` (set an element's image by name, through `FUN_140E5B0F0`) | builds the path, then queues a file request in list `+0x1480` unless the same path is already there |
| 2. load | file capability (unchanged loader). The TPF's textures are created as GX textures from encoded DDS and registered by name in the CPU texture registry `[144792F58]` | — |
| 3. lookup and import | `FUN_140E60160(container, name)` → `FUN_140E602F0` → **`FUN_140E48980`** | if not cached: `FUN_140D4ED20([144792F58], name)` → holder `entry+0x60` → **`FUN_140E49320(cache, name, manager, holder)`** → ImageSource (`FUN_140E478F0`), cached in `cache+0x60` |
| 4. bind | `FUN_140E5F650(container, movie, "MENU_DummyIcon_…", image)` → **`FUN_140E5F5D0`** | the movie's resource by export name (player vt+0xD8). If it is an image resource (`(type & 0xFF00) = 0x100`), `FUN_140E5B1F0` replaces its image |
| 5. late binding | container update `FUN_140E5C080` | for each node of the global list `[1447992C0]` whose file capability reaches state 4 (`+0x70`), it calls `FUN_140E60160` on the name, then binds (INFERRED: this list holds the menu TPF packages, so a sheet that finishes loading after a movie is bound then) |

- Icon UVs come from the movie (the sheet is an atlas). The renderer supplies only the texture.
- A missing sheet: the step 3 lookup gives holder 0, `FUN_140E49320` returns 0, nothing is
  cached, and step 4 does nothing. The placeholder image stays.

### Native mode (pilot branch, `native/frame/game_bootstrap.cpp`)

- **No icon-specific native code exists, and none is needed.** Steps 1, 2 (CPU part), 4 and 5 are
  unchanged game code. Two generic hooks carry the rest.
  - **Step 2 GPU:** `0x2B260` → `AdmitEncodedTexture`. It decodes the TPF entry's DDS and creates
    a resident native texture synchronously (`textures.AdmitOwned`, with an upload fence) for the
    game's texture object. The game registers the name itself.
  - **Step 3:** `0xE49320` → **`NativeUiTextureImport`**. It reads the holder (vtable
    `29559E8`, texture `+0x10`), takes the resident native record, makes a UI image through the
    UI session (`ImportTexture`), then runs the genuine `FUN_140E478F0` to build the ImageSource.
    - A null holder returns 0, as in the original.
- **Native divergences to know:**
  1. **A sheet with a holder but no resident native texture is fatal.**
     - `NativeUiTextureImport` logs `native_ui_asset_unavailable` and refuses
       (`native_ui_asset_import_failed`), where the original would simply wrap the GX texture.
     - It happens if a sheet's DDS format or layout is refused by `AdmitEncodedTexture`.
       `unqualified_encoded_texture_source` or `native_encoded_texture_failure` would come first.
  2. **The witness name is misread.** `name` is a `std::wstring` object, not characters. For
     names longer than 7 characters (all sheets), the text is at the pointer stored at `+0x00`
     when the capacity at `+0x18` > 7. As written, the row prints pointer bytes. Read through that
     pointer to get `MENU_Icon_xxxxx`.
  3. **The bind at movie load depends on MenuMan `+0x30`.** When it is 0, sheets reach a movie
     only through the late path (step 5). INFERRED: this is a game state flag, not a renderer
     one.

### Not located statically

What requests the 17 `MENU_Icon_*.tpf` files. They are not requested by `FUN_140E5DB90`, which
only looks them up. The global list `[1447992C0]` is only read, by `FUN_140E5C080`; it is
initialised statically by `FUN_14268F640`. A per-element request goes through
`FUN_140E605B0`/`FUN_140E5FC40`.

### Runtime checks (one in-game menu opening, e.g. Inventory)

1. **Imports:** count the `native_ui_asset_image_imported` rows (17 sheets expected, with their
   sheet sizes), with the name read as in point 2 above.
2. **Registry lookups:** `FUN_140D4ED20([144792F58], "MENU_Icon_xxxxx")` must return non-zero for
   the 17 names.
3. **Binding:** `FUN_140E5F5D0` should return 1 once per sheet per movie.
4. **Gate:** MenuMan `+0x30` (`[[144763258]+0x30]`) at the movie load.
5. **Load requests:** if a sheet is missing, look for its `menu:/MENU_Icon_xxxxx.tpf` file
   capability in the loader. That shows whether the request or the load is the gap.

Exports: `decompiled/DS3-1152-F-00E5DB90`, `00E5F340` (existing), `00E58EA0`, `00E52CE0`,
`00E60160`, `00E5F650`, `00E5F5D0`, `00E48980`, `00E49320`, `00E5C080`, `00E5FC40`,
`00E605B0`, `00E5B0F0`, `00764170`.

## 53. Who loads the icon sheets: the UI startup step and the page-group TPFs (STATIC + game data)

This section closes section 52's open point.

**The 17 `MENU_Icon_*` sheets are not separate files.** They are textures inside
**`menu:/01_Common.tpf`**:

- 2048 wide, height 32..2048 per sheet;
- DDS DX10, DXGI 98 (BC7_UNORM), one mip.

The placeholders `MENU_DummyIcon_*` are textures of **`menu:/05_Dummy.tpf`** (160×160, DXT1).
The Data1 archive contains the menu TPFs `01_common`, `02_title`, `03_chrmake`, `04_nowloading`,
`05_dummy`, `06_platform` and `<language>/80_language`. The listing is in
`evidence/I-0067/queries/menu/menu-tpf-textures.tsv`.

### Requests (`FUN_140E5AAA0(container, index)`)

- Name table `[1446525D8 + 8·index]`:

  | Index | File |
  |---|---|
  | 0 | `01_Common` |
  | 1 | `02_Title` |
  | 2 | `03_ChrMake` |
  | 3 | `04_NowLoading` |
  | 4 | `05_Dummy` |
  | 5 | `06_Platform` |
  | 6 | `80_Language`, path `%s/%s.tpf` with the language folder from `FUN_140EA49A0` |

- Path `menu:/%s.tpf`, then **`FUN_140507470([1447559C8], path, 0, 0)`**: the file request. The
  handle goes to `container+0xBF8+8·index`, and the request is made only if that slot is empty.
- The callers are thin container methods on `[144798EF8]+8`:
  - `FUN_140E4DC60`: indices 0, 2, 4, 5;
  - `FUN_140E4DC40`: index 3;
  - `FUN_140E4DCB0`: index 1;
  - `FUN_140E4DC20`: index 6.

### When

All four are called in sequence by **one state handler of the UI startup task** (the code at
`140E50364..140E50427`; Ghidra did not disassemble its first bytes). It then calls the progress
function `FUN_140E50820` and sets `task+0x58 = 1`. So the request happens **once, during UI
start-up**, before any menu movie is loaded.

- That handler is not one of the natively replaced UI steps: `NativeUiStartupTask` replaces
  `FUN_140E500C0` and `WaitUiCpuConsumers` replaces `FUN_140E506C0`. So in native mode the
  original code issues the requests.
- When `01_Common.tpf` has loaded, its textures are created through `AdmitEncodedTexture` and
  named in `[144792F58]`. From then on, every movie load (`FUN_140E5DB90`, with MenuMan `+0x30`)
  finds the 17 names.
- **Runtime check (native):** the `native_encoded_texture_resident` rows with 2048-wide BC7
  textures (format 98) after UI start-up, then the registry lookup of `MENU_Icon_00000`.

`FUN_140E5FC40`/`FUN_140E605B0` (section 52) serve the per-element images (`Knowledge`,
`MapImage`, list `+0x1498`). For example, each pending image is requested by
`FUN_140E5C080` after half a second (`+0x1AC`, timer `+0x1A8`). They do not serve the sheets.

Exports: `decompiled/DS3-1152-F-00E5AAA0`, `00E4DC60`, `00507470`.

## 54. High Wall of Lothric, Argo's scene: the chained corpse, the wall torch and its light, by family (STATIC + game data; INFERRED where marked)

Pilot observation, Native1181 (`argo-origine-vs-natif.jpg`):

- the original draws a large corpse wrapped in chains, with a lantern, and a lit wall torch that
  throws warm light on the pillar;
- native mode draws the sconce, unlit and with its bowl dark, and no corpse.

Sources: MSB `m30_00_00_00.msb` and light banks `m30_00_00_00_0000/0001.btl` (Data5), object and
character binders (Data3/Data4), read with SoulsFormats. Listing:
`evidence/I-0067/queries/m30-argo/m30-argo-scene.txt`.

### Locating the scene (INFERRED)

Argo's saved position was not decoded. The scene is identified by elimination:

- the corpse model (below) has 7 placements in m30_00;
- only **`c1105_0011`** stands next to a wall torch: `o304863_8030`, 3.2 m away;
- every other `c1105` is 10 m or more from the nearest of the 35 `o304863` torches.

The screenshot shows a torch and a corpse about 3 m apart.

### 1. The corpse: an enemy character, not an object

| Field | Value |
|---|---|
| MSB part | **`Enemy c1105_0011`** at (−32.72, −0.07, 83.19), rotation Y 136° |
| MSB parameters | NpcParam 110500, ThinkParam 110501, entity 3000251, hit collision `h002200`; DrawGroup1/2 all bits, DispGroup1 0 |
| Nearby event region | `領域_墓点灯後の亡者大ワープ先 A`. INFERRED: `亡者大` = "large hollow", the role of `c1105` |
| Model | `chr/c1105.chrbnd`: FLVER with **120 bones**, 15 meshes, 2.2 × 2.7 × 3.6 m. **Skinned and animated** (`c1105.anibnd` 1.9 MB, `behbnd`), **cloth** (`c1105_c.hkx` + `c1105_c.clm2`), physics `c1105.HKX`/`hkxpwv` |
| Materials | `C[ARSN]_Decal` and `C[ARSN]_e_Decal` → **`GXFlver_ColDifSpcBumpIbl_Bloody`** (alpha ref 0 and 128, `g_GX_bDisableDecal` true); `C[ARSN]_e_Cloth_Decal` → **`GXFlver_ColDifSpcBumpIbl_Bloody_Cloth`** (mesh `#07#_Cloth`) |
| Textures | `c1105_0705`, `c1105_1003`, `c1109_{armour2, cappa2, helmet2, scarf_01/02, shoes2, body2, light}` |

- INFERRED: `c1109_light` is the lantern, and the chains are part of the body meshes. No separate
  chain or lantern part lies within 2.5 m.
- **Draw path of a non-player character** (STATIC):
  1. character creation `FUN_1408A9E50`/`FUN_1408BE6C0` → `FUN_140771E10`/`FUN_140771F20`;
  2. that builds **`NS_SPRJ::ChrModel`** (vtable `14281DCB8`) over `FUN_140C49210`/`FUN_140C49590`;
  3. those create a **`SprjModelDrawEntity`** (vtable `142948EF8`, constructor `FUN_140D05C80`).
  - The player's equipment goes through `FUN_140C32190` → `FUN_140C44D20` →
    `SprjAsmModelDrawEntity` (`142947DB8`).
  - So the enemy uses the **same draw-entity class as map objects**, but with a skinned FLVER
    whose pose comes from the character's animation.

### 2. The torch, its flame and its light

| Field | Value |
|---|---|
| MSB part | **`Object o304863_8030`** at (−31.32, 2.01, 85.24), rotation Y −180, entity −1, LightID 0, LanternID 0, DispGroup1 `0x10000000` |
| Model | `obj/o304863.objbnd`: FLVER only (no anibnd, no physics, no textures of its own); 3 bones (`o304864`, `Model_Dmy`, `o001`), **one dummy, reference 200**, on `Model_Dmy` at (0, 0.437, −0.137) |
| Materials | `WP_A_9569_em`: `M[ARSN]_em` → **`GXFlver_ColDifSpcBumpEmiIbl`** (emissive texture `m30_00_o304863_02_e`); `o304864`: `M[ARSN]` → `GXFlver_ColDifSpcBumpIbl` |
| ObjectParam | **no row 304863** |

- **No MSB SFX region** lies within 2 m of any of the 35 torches. The nearest regions are fog,
  leaves or wind.
- The part has **no entity id**, so no EMEVD instruction can address it.
- INFERRED: **the flame is an FFX effect attached by the engine to dummy 200 of the model.** The
  path that spawns it was not located.
- **The light:** the light bank near the torch holds only cold fill lights:
  - `Omni263` at 2.4 m, colour (197, 227, 255), radius 6;
  - `Omni214` at 7.8 m, (123, 202, 255).
  - No warm map light exists there. INFERRED: the warm light is the **point light carried by the
    flame effect** (an FFX effect light, sections 10, 14 and 30), not a light-bank light.
- **Native witness** (Native1181 `startup.jsonl`):
  - every SFX request is `native_sfx_request_deferred` with `effect_created:false`;
  - the FX world and SFX manager are published with `fx_rendering:false`/`sfx_draw_ready:false`;
  - id **300** was requested 10 times; `f000000300.fxr` exists in `commoneffects`. INFERRED: the
    wall torch flame.
- **The bowl glow:** `GXFlver_ColDifSpcBumpEmiIbl` does not appear among the native families
  logged in that run, and the bowl is dark natively. To check: whether this emissive family is
  admitted.

### Families the native backend does not cover yet (D-0161)

| # | Family | What it covers here | Why native misses it (INFERRED where marked) | Runtime check |
|---|---|---|---|---|
| 1 | **FFX effect instances** (particles, billboards, ribbons) | torch flame, probably the lantern glow | The native SFX path stops at the CPU parse. Requests are deferred and no effect instance exists, so nothing can be drawn | `native_sfx_request_deferred` rows; an effect instance after the request |
| 2 | **Effect point lights** (FFX-owned lights in the deferred light list, sections 10, 14 and 30) | the warm light on the pillar | They are created by the effect instance; with family 1 absent there is no light. The FX light registry `[4962398]` is built natively but nothing registers or draws its lights | registry entries after the flame is created; light-list size per pass |
| 3 | **Non-player characters** (`ChrModel` → `SprjModelDrawEntity`, skinned FLVER, animation pose) | the corpse `c1105` | The selection admits `SprjModelDrawEntity` (map objects), but a character puts a skinned model with a **character animation pose** and a **bone palette** behind it. The native palette and pose producers were proved for the player (`SprjAsmModelDrawEntity`) and for objects. INFERRED: the character's pose is either not read or refused, or the entity is filtered earlier | at `AdmitNativeSelectedObject` for this entity: bit 0 of `+0xC3C`, category against `world_mask`, `FUN_140D03430`, then the `AdmitNativeModelCandidate` refusal code (`zero_fade`, `missing_bindings`, `instance_streams`, `missing_geometry`, `no_meshes`) and `native_selected_palette_unavailable` rows with `asm_part −1` |
| 3a | **Cloth meshes** (`*_Cloth` programmes, `clm2` simulation, dynamic vertex streams) | the corpse's cape | `NativeDynamicMeshBufferSet` leaves dynamic streams unowned (`native_dynamic_mesh_buffers_unavailable`): those meshes are an explicit omission | that row for `c1105`'s model |
| 4 | **Emissive map objects** (`GXFlver_ColDifSpcBumpEmiIbl`, `_em` MTDs) | the sconce bowl | absent from the native family log of that run; the bowl draws dark | `native_capability_gate` and admission rows for `GXFlver_ColDifSpcBumpEmiIbl` |

- These are families, not assets. Every torch of this kind (FFX flame and its light on an
  object dummy) depends on families 1 and 2, and on family 4 when its model has an emissive
  material. Every enemy and NPC whose model is not ChrAsm depends on family 3.
- The shader family of the corpse (`GXFlver_ColDifSpcBumpIbl_Bloody`) already draws for the
  player's equipment. The gap is the owner and pose mechanism, not the programme.

Exports: `decompiled/DS3-1152-F-00771E10`, `00771F20`, `00C49210`, `00D05C80`. The
draw-entity constructors were found with `I0067Map.java` queries `o:142948ef8` and
`o:142947db8`.

## 55. Native specification: non-player characters (enemies, NPCs, NPC phantoms) (STATIC, native read on `claude/zen-lamarr-25edc3` `5138e013`; INFERRED where marked)

This section closes family 3 of section 54. The specification is written to be coded from.

**Main result.** The pose and palette mechanism the native backend already owns is generic:

- it is keyed by the 0x88-byte pose receiver (`FUN_140015490` init; `FUN_140016050`,
  `FUN_140015FC0`, `FUN_140016000` updates);
- a non-player character's draw entity uses the **same receiver class** as the player's parts,
  at a different address;
- its rows are produced by unchanged CPU code.

What remains is to make the selection reach that entity, to prove the admission inputs, and to
decide on cloth.

### 1. The two draw paths

| | Player (proved natively) | Non-player character |
|---|---|---|
| Creation | `FUN_1408BE6C0` → `FUN_140C32190` → `FUN_140C44D20` | `FUN_1408A9E50` / `FUN_1408BE6C0` → `FUN_140771E10`/`F20` (`NS_SPRJ::ChrModel`, vtable `14281DCB8`, derived from `SprjModel` `1429263D8`) → `FUN_140C49210`/`FUN_140C49590` |
| Draw entity | `SprjAsmModelDrawEntity` (vtable `142947DB8`, constructor `FUN_140CFF200`) | **`SprjModelDrawEntity`** (vtable `142948EF8`, constructor **`FUN_140D05C80`**, which calls `FUN_140015420` then `FUN_140015490` through the thunk `FUN_140015B00`) |
| Model | one model per part: descriptors `+0xB00` (stride 0x50, binding `+0x10`, flags `+0x38`), objects `+0xB10` (stride 0x108) | **one model**: binding `[E+0xE8]`, model `[binding+8]` |
| Pose receiver | `part+0x50` | **`E+0xA80`** |
| Insertion into the world | — | `ChrIns` slot 18 `FUN_1408853C0`: entity `[[chr+0x48]+8]`, dynamic layer of `[144796298]` (`FUN_140DFDA10`). The native world log shows 227 `SprjModelDrawEntity` inserts from that caller (RVA `0x885584`), so **characters do reach the native world layer** |

### 2. Who computes the bones, and where the rows live

The `SprjModel` object `M` (the `ChrModel`) has these fields:

| Offset | Content |
|---|---|
| `+0x08` | the draw entity `E` |
| `+0x10` | skeleton source (its vt+0x18 is called by slot 6 `FUN_140C4ABB0`; `+0x10` holds rows) |
| `+0x20` | the world transform |
| `+0x50` | the local pose `{+0x0C count, +0x10 rows}` |
| `+0x58` | the mode |
| `+0x70/+0x74` | dirty flags |
| `+0x1E0` (`[M+0x3C·8]`) | an optional second pose source (vt+0x20 → `+0x10` rows) |
| `+0x23C` | scale |

- INFERRED: the character animation (behaviour/Havok) writes `M+0x50` before the model update;
  the writer was not traced.

**Per frame, in order:**

1. **`ChrModel` slot 8 `FUN_1407728A0` → `SprjModel::Update` `FUN_140C4AF10`.** When
   `(M+0x70 | M+0x74) & 3`:
   - mode 0: `FUN_140D06A00` → rows = `FUN_140D5B7E0(count)` (per-frame scratch, 0x30 bytes a
     row) composed by **`FUN_140D33EE0(rows, M+0x20, M+0x50, [M+0x10]+0x10)`**;
   - mode 1: `FUN_140D06AA0`, composed by `FUN_140D33E50`;
   - mode 2: `FUN_140D06B40`, a plain copy of `[M+0x60]`.
   - Then **`E` vt+0xA0 = `FUN_140D076A0(E, rows)`**, and, when `M+0x1E0`, **`E` vt+0xA8 =
     `FUN_140D075C0(E, rows2)`**.
2. **`FUN_140D076A0` (write).** It copies `count·0x30` bytes into the write container:

   ```
   w = (E+0xC3C >> 4) & 1
   memcpy([E+0xB18 + 0x18·w], rows, [E+0xB14 + 0x18·w] · 0x30)
   if (E+0xCA & 0x10) E+0xBA8 |= 8        // swap requested
   E+0xBA8 = ((E+0xBA8 & 1) << 1) | (E+0xBA8 & ~2) | 5
   ```

   The two containers are `E+0xB08` and `E+0xB20` (0x18 each: `+0x0C` count, `+0x10` rows).
3. **`FUN_140D075C0` (reference rows).** It copies into `[E+0xB78]`, count `[E+0xB74]`.
4. **`SprjModelDrawEntity` update, vt+0xC8 = `FUN_140D07710(E, frame)`.** When `frame+8` (dt) ≠ 0:
   - **without cloth**, that is `[14477FEC8]+0x170 = 0`, or `E+0xA70 = 0`, or `FUN_140DD5D10()`:
     - if `E+0xBA8 & 8`, the swap `FUN_140D08430` toggles bits 3/4 of `E+0xC3C`;
     - then **`FUN_140015FC0(E+0xA80, [E+0xB18 + 0x18·((E+0xC3C>>3)&1)], 0)`**;
   - **with cloth**: **`FUN_140016000(E+0xA80, [FUN_140D08080(E)+0x10], [E+0xB78], 0)`**. The
     reference rows feed the cloth cancel composition.
5. **Placement:** vt slot 18 `FUN_140D079C0` passes the container selected by **bit 4** to
   `FUN_1407E2300`. That is what the native selection already reads at
   `game_bootstrap.cpp:3829`.

`E+0xC3C` is a dword bit field: bit 0 enabled, bit 3 read container, bit 4 write container,
bit 5 set by `FUN_140D07330`. `E+0xBA8` holds the update flags.

### 3. How the original renderer reads it

- The receiver `E+0xA80` is the same 0x88-byte prefix the native code owns
  (`NativeModelPosePrefix`): `gpu[4]`, `matrices[2]` at `+0x20`, index `+0x68`, ring `+0x6C`,
  count `+0x70`, fast `+0x74`, model `+0x80`.
- The original updates upload the rows to the palette buffers, which the skinned VS reads at
  **b5** (`VC_aObjMatrix`, indexed by `(BLENDINDICES + matricesData.x)·3`), and at **b12**
  (cancel) for cloth composition. This is the convention already implemented for the player
  (`game_bootstrap.cpp:4415..4440`).
- The rows are world-space, with `M+0x20` composed in. INFERRED from `FUN_140D33EE0`'s
  arguments: the same convention as the player's parts, which draw correctly.

### 4. What the native code must provide

| Need | Status in `5138e013` | Action |
|---|---|---|
| Pose owner for `E+0xA80` | **already**: `InitializeNativeModelPose` hooks `0x15490`, reached from `FUN_140D05C80` | verify for one character (check 3) |
| Per-frame pose rows | **already**: `UpdateNativeModelPoseSingle` (`0x15FC0`) and `Public` (`0x16000`) consume the genuine containers written by `FUN_140D076A0` | none |
| Palette (b5) and cancel (b12) | **already**: generic `ModelPoseOwner`, identity cancel when absent, padding to 256 rows | none |
| Entity class in the selection | **already** (`SprjModelDrawEntity` branch, `game_bootstrap.cpp:3810..3833`) | none |
| Shader family | `GXFlver_ColDifSpcBumpIbl_Bloody` proved for the player's equipment | the `_Bloody_Cloth` programme (check 6) |
| Non-player refusal witnesses | **missing**: `NoteNativeAsmMissingBinding` and the pose witnesses only log asm parts (`neutral_instance_decal`) | log the same refusal codes for a `SprjModelDrawEntity` whose model is a character (counted by the class of `[[chr+0x48]]`) |
| Cloth dynamic buffers (3a) | `NativeDynamicMeshBufferSet` (`0x23B7960`, from `FUN_1423633A0`) leaves the set empty, so `FUN_1423D6010`'s consumer skips the cloth deformation. Physical model streams (`0xD3F7C0` init, `0x51B940` update from the physics positions `FUN_140833DF0`/`833DE0` through the remap `FUN_140773590`) **are** owned natively | first phase: draw cloth meshes skinned only (pose b5, identity cancel); then own the dynamic set: creation `23B7960`, CPU update, selected use, retirement |

**Reuse.** Everything from the pose receiver onwards is the player path. A non-player character
differs only in:

- the entity class: the existing branch;
- the source of the rows: genuine CPU code;
- cloth through the dynamic set, which is open.

### 5. Probable refusals, in the order the native code tests them

`AdmitNativeSelectedObject` (`SprjModelDrawEntity` branch):

1. `pointer(pointer(E)+0x98) ≠ D07E10` → **Refuse**. This cannot happen for this class.
2. `classify(culler, E+0xB80)` false: bounds outside the frustum.
3. **`!(E+0xC3C & 1)`**: entity disabled.
4. **`!(E+0x70 & world_mask)`**: category outside `FUN_140DFD240`'s mask.
5. **`FUN_140D03430(E) = 0`**: the eight visibility words. INFERRED: draw groups; MSB
   `c1105_0011` has DrawGroup1/2 all bits, DispGroup1 0.
6. `[E+0xE8] = 0` or `[binding+8] = 0`.

`AdmitNativeModelCandidate`. The counters are in `NativeSelectedScene`; none is logged for a
non-asm entity today.

7. `zero_fade`: `ReadModelFadeStateAt(E, layout)` then `SelectModelFade(…, 5)` gives no intent.
   INFERRED: a character fade (dormant or distance) or the MSB `IsShadowOnly` flag. For
   `c1105_0011` SoulsFormats names that byte `IsShadowOnly = True`; the label is unproved.
8. `missing_bindings`: `ReadCurrentModelInputsAt(model, E+0xA60, E+0xA48, E+0xA80)` fails, or
   the patch `[source+0x50]` is neither a native physical stream nor
   `GXBloodyMaterialPatch` (`3D41DF0`). **The most likely refusal for a cloth character**
   (INFERRED): its physical-stream receiver must exist in `model_streams`.
9. `instance_streams`: instanced input, refused outright.
10. `missing_geometry`: `model_storage` has no meshes for `[model+0xE8]`.
11. Preparation: `unsupported_palettes` (pose not in `model_poses` → `model.pose` null), the
    capability gate of an unadmitted programme (`_Bloody_Cloth`), or textures (`c1105.texbnd`).

### Measurable checks on the witness `c1105_0011` (High Wall, Argo slot)

Address: the `ChrIns` whose entity id is **3000251**, `E = [[chr+0x48]+8]`,
`M = [chr+0x48]`.

| # | Measure | Expected (INFERRED from the static reading) |
|---|---|---|
| 1 | `[E]` | `image+0x2948EF8` |
| 2 | `E+0xC3C` (dword), `E+0x70`, `FUN_140DFD240(manager)`, `FUN_140D03430(E)` | bit 0 set; `E+0x70 & mask` ≠ 0; visibility 1. Compare with the original game at the same place |
| 3 | prefix at `E+0xA80`: `+0x70` count, `+0x80` model, `+0x20` matrices[0], and `model_poses.contains(E+0xA80)` with `updates` increasing | count = the model's bone count (FLVER: 120), model = `[[E+0xE8]+8]`, updates +1 per frame |
| 4 | containers: `[E+0xB14]`, `[E+0xB2C]` (counts), `[E+0xB18]`, `[E+0xB30]` (rows), `E+0xBA8`, bits 3/4 of `E+0xC3C` over two frames | counts = bones, rows ≠ 0, bit 4 written each frame |
| 5 | `M+0x58` (mode), `M+0x5C` (count), `M+0x70/+0x74` (dirty) | mode 0, count = bones |
| 6 | per view, the selection counters before and after `E` (`zero_fade`, `missing_bindings`, `instance_streams`, `missing_geometry`, `no_meshes`), and the refusal site | the first non-zero counter is the cause |
| 7 | preparation rows for `[[E+0xE8]+8]`: draws, `unsupported_palettes`, family keys | families `GXFlver_ColDifSpcBumpIbl_Bloody` and `_Bloody_Cloth` |
| 8 | cloth: a `native_dynamic_mesh_buffers_unavailable` row for this model, and a `native_model_stream_initialized` row whose model is this model | identifies which cloth path the model uses |

Exports: `decompiled/DS3-1152-F-00D07710`, `00D076A0`, `00D075C0`, `00D08430`, `00D08080`,
`00C4AF10`, `007728A0`, `00D06A00`, `00D06AA0`, `00D06B40`, `008853C0`, `023B7960`;
`00D05C80`, `00771E10`, `00771F20` and `00C49210` already exist (section 54).

### Appendix: menu portrait hair (pilot, Native1181)

Measured: the Load Game view contains no `HR_A_0106` model; 5 parts fail at site 1 with a model
of vtable `2951018`, storage 0, meshes 0, source 0.

**Placeholder models.** `FUN_140C3CA10(asm, slot, …, partsSet, lod)` is called by
`FUN_140C3BA60` for slots 0..0x17.

1. It reads the part resource `R = FUN_140C424A0(partsSet, kind) = [partsSet + kind·0x60 + 0x38]+0x80`.
2. It takes the FLVER model data `[R + 0x80 + 8·lod]+0x60` (lod ≤ 3).
   - That model data is created by `FUN_140D23DA0` when the resource's FLVER file has loaded:
     `SprjFD4FlverModelData` constructor `FUN_140D35AF0(heap, data, size, name)`; with data
     present, `FUN_140D36690` initializes it.
3. **If that pointer is null when the slot is built**, it creates `SprjFD4FlverModelData(heap,
   0, 0, "")`: empty, no storage, no meshes.
4. It builds the part (`FUN_140C3DB40`, 0x148 bytes) at `asm+0x28+8·slot` with it.

- The 5 parts are slots built before their resource's FLVER was ready, and never rebuilt.
  INFERRED: the original rebuilds the slot once the resource is ready; the trigger was not
  traced.
- `FUN_140D36690` initializes synchronously (vt+0x28) when the helper `[[144793348]+0x80]` is
  null. That is the native case, since `FUN_140D573B0` (which creates
  `CSFD4FlverModelDataInitializeHelper` there) never runs. The model-data initialization itself
  is therefore not the gap.

**Names.**

| Object | Field |
|---|---|
| loaded model data | `std::wstring` at `+0x2B8` (length `+0x2C8`, capacity `+0x2D0`) |
| placeholder | empty name |
| part resource `R` | `+0x10` part name (C3CA10 tests an `_L` suffix); `+0x80+8·lod` FLVER resources (`+0x10` name, `+0x60` model data) |

Witness: a hook on `FUN_140C3CA10` logging slot, `R+0x10` and whether `[R+0x80+8·lod]+0x60` is
null, and whether the same slot is called again later.

**The texture override is per key** (answers where `ctx+0x1B0` is read).

- `FUN_140140680` (material preparation) reads `ctx+0x1B0`. When it is non-null it calls
  `FUN_1401ADEB0(material+0xA8, …, provider)`, otherwise `FUN_1401AD990`.
- For each texture entry of the material (stride 0x38: `+0x00` register, `+0x08` default
  holder, `+0x10` kind, `+0x28` key), it asks `provider->vt+0x10(key)`. A null answer keeps the
  material's own texture.
- With `item+0x6F2 = 0`, `FUN_140D3DA20` sets the provider to `FrpgFgTextureIns+0x30` once `+0x91`
  is ready. So the FaceGen provider decides key by key.
- The face part's hair-shell meshes (`FC_F_0000` #04#..#16#, `P[ChrCustom_Hair]_al`) author
  `FC_F_0000_hairoff_a` as their albedo.
- Pilot measure (Native1181): the FaceGen provider answers only the `FG_face_gen` key. The
  shell keeps its FLVER texture, in the original too.
- **The model class** of the 5 parts is `SprjFD4FlverModelData` (`model_table_rva` 43323416 =
  `0x2951018`). `0x142950E18` is not a vtable start: it is slot 4 of
  `DLRuntimeClassImpl<MdlObj>`.
- **The hair item** of a menu view comes from the same child list as the other parts (one
  `SprjFD4ModelItem` per ChrAsm slot). Only its model is the placeholder. Cloth
  (`HR_A_0106_c`) acts later, through streams, and does not decide whether the item exists.

`HR_A_0106.partsbnd`: FLVER with 247 bones and 13 meshes, `P[ChrCustom_Hair]` and
`P[ChrCustom_Hair]_cloth` (#66#..#68#), one mesh `#60#` on `HR_A_0000` textures, with cloth
`HR_A_0106_c.clm2`/`hkx`.

Additional exports: `00C3CA10`, `00C424A0`, `00D35AF0`, `00D23DA0`, `00D36690`, `00140680`
(existing), `001ADEB0`.

## 56. When a ChrAsm slot is (re)built in a menu view, and why a placeholder is never replaced (STATIC; INFERRED where marked)

This section follows section 55's appendix (menu portrait hair).

**Answer: there is no rebuild triggered by a FLVER becoming ready.** Readiness is checked
*before* the slots are built. Once a slot has a part, only a change of resource record or LOD
replaces it.

### Who calls `FUN_140C3BA60` (build every slot, 0..0x17)

Only two state handlers of the menu offscreen model renderer (`CSMenu*ModelRend`, the class of
sections 1, 24 and 49):

1. **`FUN_140D67D50`, the loading state.**
   - `ready = FUN_140C42620(partsSet [R+0x6D8], appearance R+0x2F8, face, …)`;
   - when `ready` and `FUN_140D690A0(this)` (the `c0000` character resources are present and the
     view's model `+0x6E0` is built) are both true:
     - `FUN_140C3BA60(asm [R+0x6E8], appearance, partsSet)`;
     - masks `FUN_140C3B400`;
     - attach `FUN_140C3AD90(asm, [[R+0xB8]+0x50])`;
     - `FUN_140D68280`, then the next state (vt+0x30).
   - Otherwise it stays in this state and tries again next update.
2. **`FUN_140D67940`, the display state, every update.**
   - When the face parameters changed (0xF4-byte compare of `+0x70C` against `+0x200`), they are
     rebuilt;
   - `R+0x6CB = FUN_140C42620(partsSet, R+0x110, …)`;
   - **`FUN_140C3BA60(asm, R+0x110, partsSet)` is called again every update**, then masks and
     `FUN_140C3B950`.

### What `FUN_140C3CA10` tests for an existing slot

`FUN_140C3F6D0(part, R, lod)` returns "rebuild" only when `part+0x120 ≠ R` (the part's resource
record) or `part+0x128 ≠ lod`.

- A part built with a placeholder model keeps the same `R` and LOD, so **it is never rebuilt
  when `[R+0x80+8·lod]+0x60` appears later.**
- In the original this cannot happen if the gate below holds.

### The gate: `FUN_140C42620` (the parts set, 29 kinds, 0x60 per kind)

| Field | Content |
|---|---|
| `set+0x08` | countdown |
| `set+0x10` | in-flight request handle |
| `set+0x28` | "parameters changed" |
| `set+0x2E` | "committed this call" |
| kind `k`, `+0x30` / `+0x34` | current / requested part id |
| kind `k`, `+0x38` / `+0x40` | current / pending resource |
| kind `k`, `+0x48..+0x88` | attributes, also current/pending |

1. While `set+0x10 ≠ 0`, nothing happens. INFERRED: the batch is still loading; the loader clears
   the handle.
2. With no request in flight and the countdown at 0: for each kind whose id changed, it issues a
   file request with the pending resource in `+0x40` (`thunk_FUN_14545B3F6([1447559C8], name,
   set+0x10)`), then sets the countdown to 2.
3. Countdown > 0: it decrements. At 0 it **commits**:
   - `+0x38 ← +0x40` and the attributes;
   - releases the old resource (`FUN_140818CB0`);
   - sets `+0x2E = 1`;
   - kind 0 additionally waits for the FaceGen resource `[[set+0x40]+0x80]+0x80]+0x60` (field
     `set+0xB10`, `FUN_14082A910`/`FUN_14082B090`) before committing.
4. `FUN_140C424A0(set, k)` returns `[set + k·0x60 + 0x38]+0x80`. That is the `R` that
   `FUN_140C3CA10` reads, so a slot only sees **committed** resources.

### What fails natively (INFERRED, to measure)

The placeholder appears when, at the moment `FUN_140D67D50` builds the slots, the committed
record `R` has no FLVER model data at `[R+0x80+8·lod]+0x60`. Two possibilities:

- **(A) The model data is created after the commit.** The FLVER resource exists at
  `[R+0x80+8·lod]`, but `FUN_140D23DA0` (FLVER resource completion, from `FUN_140D22630`) has not
  run yet. Its `+0x60` is null, so the request handle `set+0x10` was cleared, or the countdown
  expired, before the FLVER sub-resource finished. In the original the handle covers the whole
  partsbnd, sub-files included (INFERRED).
- **(B) The model entry is absent.** `[R+0x80+8·i]` itself is null for the index `i`
  `FUN_140C3CA10` uses: 1..3 for slots 8..10 and 12..14, 0 otherwise. That index is called
  "lod" above; it may be a sub-model index rather than a level of detail (INFERRED).

Neither the native gate nor the loader is replaced by the pilot's hooks. Native changes that can
reach (A):

- texture admission (`0x2AF40`, `0x2B260`);
- model admission (`0xA0430`, `0xA0170`, `0xA05C0`);
- the physical streams (`0xD3F7C0`);
- **`NativeDynamicMeshBufferSet` (`0x23B7960`)**, which runs for each admitted mesh of a loading
  model. A hair model with cloth is exactly that case.

Any of them that refuses, defers or returns early inside the FLVER completion chain leaves
`+0x60` null.

### Measures (one Load Game portrait)

1. **Hook `FUN_140C3BA60`.** At each call, for the hair kind `k`: `R = [set+k·0x60+0x38]+0x80`,
   `R+0x10` (name), `[R+0x80..+0x98]` (four LOD resources) and their `+0x60`, plus `set+0x08`,
   `set+0x10`, `set+0x2E`.
   - (A) shows `[R+0x80+8·lod] ≠ 0` with `+0x60 = 0`;
   - (B) shows `[R+0x80+8·lod] = 0`.
2. **Hook `FUN_140D23DA0`** (or its caller `FUN_140D22630`) and log the resource name `+0x10`, to
   see whether and when `HR_A_0106`'s model data is created relative to step 1.
3. `part+0x120` and `part+0x128` for the hair slot (`asm+0x28+8·slot`), to confirm that
   `FUN_140C3F6D0` sees no change afterwards.
4. In the original game, the same values at the same call: the FLVER model data is expected to
   be present at the first build.

**The fix belongs at the family level** (D-0161):

- either guarantee that the FLVER completion runs before the parts batch is committed (case A);
- or have the native rebuild rule mirror the engine. The engine has no such rule, so the
  preferred route is to find which native hook defers `FUN_140D23DA0`.

Exports: `decompiled/DS3-1152-F-00D67940`, `00D67D50`, `00C3BA60`, `00C3F6D0`, `00C42620`,
`00D690A0`.

**Pilot measure after section 56 (Native1181, both modes):** the hair slot `HR_A_0106` is built
with its real FLVER, identically in the original and in native mode. **Causes (A) and (B) of
section 56 are refuted for this case.** It is the native selection that drops its meshes
(section 57).

## 57. Native specification: dynamic cloth, the `_Bloody_Cloth` programme, and the emissive family (STATIC, native read on `5138e013`; INFERRED where marked)

### 1. Dynamic cloth (capes, cloth hair, `c1105`)

**Who creates it.** The cloth instance `FUN_140DD5FD0` is created for a model with cloth by:

- `FUN_140772270` (`ChrModel`);
- `FUN_140C37B70` (a ChrAsm part);
- `FUN_140C4C490` (`SprjModel`).

It stores its Havok cloth object at `+0x38`, and `FUN_140DD5D10(cloth)` = "cloth active"
(`+0x38 ≠ 0` and `FUN_142362990() = 0`).

The setup `FUN_1423633A0(cloth, desc)` then builds, in order:

1. two Havok objects (`+0x50`, `+0x58`, `FUN_14234D390`);
2. **the dynamic mesh set**: `FUN_1423B7890` (0x60 bytes) followed by **`FUN_1423B7960(set,
   gxModel desc[12], desc[7], desc[9])`**;
3. if that call returns non-zero (or no set exists): the cloth simulation object
   `FUN_14239C0F0` (0x140 bytes) and its binding `FUN_14239C180(sim, set, desc[7])`;
4. a name object (`+0x10`);
5. optionally `FUN_142362C00(cloth, 1)`.

**The set (`FUN_1423B7960`), one entry per cloth mesh:**

- A mesh `m` of the GX model (`+0xA8`, stride 0x48) qualifies when:
  - its material `[model+0x1F8][mesh+0]` has **`+0x22` set** (the material's cloth flag, the
    same byte the native code reads as `cloth_enabled`);
  - `FUN_1423C5070(desc7, …)` accepts it;
  - `FUN_1423B6C50` gives a vertex-buffer index `vb` (vertex headers at `model+0x158`, stride
    0x90) with data at `+0x80`.
- For it the set holds:
  - **a D3D11 dynamic vertex buffer** (`FUN_1419254F0`):
    - `ByteWidth = (count[vb+0x7C] · stride[vb+0x68] + 3) & ~3`;
    - usage 2 (DYNAMIC), bind 1 (VERTEX);
    - stored at `set+0x18[i]`;
  - **a CPU copy** (allocator vt+0x48), initialized with `memcpy` of the authored bytes
    `[vb+0x80]`;
  - an entry `{int vb, uint bytes, ptr copy}` (16 bytes) in `set+0x38..+0x48`.
- `set+0x58` holds the total bytes, `set+0x08` the model, `set+0x10` the owner.

**What a cloth mesh contains** (`HR_A_0106`, FLVER): **two vertex buffers**.

| Buffer | Layout | Bytes |
|---|---|---|
| stream 0 | **L1**: Position (float3) @0, Normal (uint) @12, Tangent (uint) @16 | 20 |
| stream 1 | **L4**: 4 bytes @0 (bitangent sign), BoneIndices @4, BoneWeights @8, UV @12, UV @20 | — |

- The dynamic buffer is the copy of **stream 0**: the simulated position, normal and tangent.
- Non-cloth meshes use one buffer, layout L3 (all elements interleaved).

**How the original reads it.**

1. At draw, the cloth drawable (`FUN_1423C9D80` → `FUN_1423D6010`; and `FUN_1423D62B0`) asks
   **`FUN_1423B6DB0(set, out, …, mesh)`**. That function finds the copy entry whose `vb`
   matches the mesh's vertex buffer, then **`FUN_1423B74A0`** builds a 0x78-byte stream
   descriptor that **replaces the semantics Position (0), Normal (3) and Tangent (6)** with the
   dynamic buffer (format, offset, stride `[vb+0x68]`), leaving the other semantics on the
   authored buffer.
2. `FUN_1423D6010` copies the descriptor's four stream slots into the drawable
   (`+0x10/+0x38/+0x50/+0x68`, plus `+0x28` buffer, `+0x30` offset).
3. INFERRED: the simulation writes the CPU copies and uploads them to the dynamic buffers each
   simulated frame (Map/discard). The writer was not located in the Havok code
   (`0x14239…`, `0x1423C…`).

**The cloth vertex shader** (`GXFlver_ColDifSpcBumpIbl_Bloody_Cloth_GBuf_Skin.vpo`, compared
with `…_Bloody_GBuf_Skin.vpo`):

- inputs: POSITION, NORMAL, TANGENT0 (uint3), **BINORMAL0 (uint x, the bitangent sign)**,
  TANGENT1, BLENDINDICES, BLENDWEIGHT, COLOR1, TEXCOORD0. The plain skinned VS has no
  BINORMAL0;
- extra constants: **b2 `VC_isEnableCloth` (offset 128)** and **b7 `cbvModelParam.VC_World`
  (48 bytes)**;
- `matrix = VC_isEnableCloth ? VC_World : Σ palette[(BLENDINDICES + matricesData.x)·3]·w`.
  With cloth enabled, every vertex is transformed by the world matrix alone: the dynamic
  positions are already in model space.
- **The pixel shader is byte-identical** to `…_Bloody_GBuf.ppo` (MD5 `294A41D1…`).

**Native today:**

- `NativeDynamicMeshBufferSet` leaves the set empty and returns 1, so the cloth setup continues
  and the original descriptor would be empty;
- the native draw uses the authored stream 0;
- `VC_isEnableCloth` = 1 exactly when the native pose carries a cloth reference
  (`game_bootstrap.cpp:3541`), and `cbvModelParam` is produced (`:4331`).
- So a cloth mesh with `VC_isEnableCloth = 1` draws its authored positions × `VC_World`: **the
  bind pose, not following the bones**. With 0 it is skinned by the palette.

**Native contract to implement (per cloth mesh):**

| Phase | What to do |
|---|---|
| creation | at `0x23B7960`, keep the engine shape: copies `{vb, bytes, ptr}` with the authored stream-0 bytes, plus one native upload buffer per entry |
| CPU update | after the simulation step, the copy holds the current stream-0 bytes: the same 20-byte L1 layout, `count` vertices |
| selected use | for a mesh whose `vb` has an entry, bind the native buffer as **stream 0** (Position/Normal/Tangent) and the authored buffer for stream 1 |
| retirement | `FUN_1423B7DC0` (from `FUN_1423630D0`) frees the copies and buffers |
| double buffering | none in the engine set: one buffer per mesh, rewritten. The native code needs its own fenced versions, as for `model_streams` |

**Cloth and the end of a FLVER load (link with section 56 cause A, now refuted):** none of
these steps gates the FLVER model data. The cloth instance is created after the model, and
`FUN_1423B7960` failing only skips the simulation object.

**Why a cloth model can be dropped by the native selection (to measure for `HR_A_0106`):**

1. **Geometry, whole model:** `CopyOnce` (`game_scene_geometry.cpp`) accepts two streams per mesh
   only when the second has the same vertex count (`+0x7C`) and a valid stride and offset;
   otherwise reason 15 → `missing_geometry` (logged by `native_model_geometry_refusals` and,
   for offscreen parts, `native_asm_part_geometry_missing`).
2. **Layout (site 4):** a cloth mesh's elements are split across two streams (L1 + L4), and the
   cloth VS needs BINORMAL0 from stream 1.
3. **Programme:** the `_cloth` MTDs (`P[ChrCustom_Hair]_cloth` → `SAT_HairIbl2` cloth variant;
   `C[ARSN]_e_Cloth_Decal` → `_Bloody_Cloth`) need their own VS in the shader bank (capability
   gate).
4. **Palette/pose:** with `VC_isEnableCloth = 1` and no cloth reference rows, `PoseOwner` cancel
   and auxiliary status (the `native_selected_palette_unavailable` fields `cloth`, `compose`,
   `cancel_status`).

Only 1 and the site-1..5 missing-binding codes drop the whole model. 2..4 drop single meshes.
The pilot sees no `HR_A_0106` mesh at all, so check 1 first, then `zero_fade` and
`instance_streams`, which are silent today.

### 2. `_Bloody_Cloth` against `_Bloody`

| | `_Bloody` | `_Bloody_Cloth` |
|---|---|---|
| PS (GBuf) | `294A41D1…` | **identical** |
| VS inputs | POSITION, NORMAL, TANGENT0/1, BLENDINDICES, BLENDWEIGHT, COLOR1, TEXCOORD0 | + **BINORMAL0** (uint x) |
| VS constants | b4, b5, b8, b11 | + **b2 `VC_isEnableCloth`**, + **b7 `VC_World`** |
| Passes | the same set (Dpt/DptA/Fwd/GBuf/Velo/Pnt…, `_Skin`, `_PN`) | the same, with `_Cloth` VS |

The native producers for b2 (`VC_isEnableCloth`) and b7 (`cbvModelParam`) already exist. What
remains is the two-stream layout (BINORMAL0 from stream 1) and the dynamic stream 0.

### 3. The emissive family `GXFlver_ColDifSpcBumpEmiIbl` against `GXFlver_ColDifSpcBumpIbl`

| | `ColDifSpcBumpIbl` GBuf PS | `ColDifSpcBumpEmiIbl` GBuf PS |
|---|---|---|
| Textures | t0 diffuse, t1 specular, t3 bump | + **t6 `g_EmissiveTexture`** (sampler **s8 `SS_WrapLinear`**) |
| Constants | b1 tiling (4 rows), b3, b4 | b1 tiling (**7 rows**: an emissive tiling row), **+ b2 `FC_EmissiveColor` (offset 32)**, b3, b4 |
| Outputs | **5** targets (SV_Target0..4) | **6** targets (SV_Target0..5): **GBuffer5 = emissive** |
| Forward PS | `…Ibl_Fwd` 28 KB | `…EmiIbl_Fwd` 31 KB (emissive added) |

The native side already has these:

- material class `0x100` for `Emi` families (`ClassifyModelMaterial`);
- a 6-target G-buffer (`scene_pass.cpp:348`) with GBuffer5 composed as t8 (`:1004`);
- `FC_EmissiveColor = {f,f,f,1}` with `f = source+0x34` when cbMatDynParam flag `0x0E` is set
  (`game_bootstrap.cpp:3536`).

**The dark bowl therefore comes from one of these (to measure on `o304863`'s mesh
`WP_A_9569_em`):**

1. the `EmiIbl` programmes are not admitted (capability gate row), so the mesh draws with
   another path;
2. t6 is unbound or a dummy (`m30_00_o304863_02_e` should be resident), or s8 is missing;
3. `FC_EmissiveColor` is zero or unwritten (flag `0x0E` and `source+0x34`);
4. **class `0x100` is routed to a pass whose PSO has fewer than 6 targets**, so SV_Target5 is
   discarded;
5. the composite does not add t8.

Also INFERRED: part of the original glow on the bowl is the flame effect's point light
(section 54, family 2), not the emissive term.

Exports: `decompiled/DS3-1152-F-023633A0`, `023B6DB0`, `023B74A0`, `023B7DC0`, `00DD5FD0`,
`00DD5D10`, `023D6010`, `023B7960` (existing). Evidence: `evidence/I-0067/queries/cloth/`
(VS/PS reflection and disassembly of the four programmes).

## 58. The cloth writer and uploader, and the two open readers of S (STATIC; INFERRED where marked)

### 1. Who writes the cloth copies and uploads them: `FUN_1423D5BC0`

Each cloth mesh's drawable holds the stream descriptor built by `FUN_1423B74A0` at
`drawable+0x120` (section 57). Its fields, from the readers:

| Offset | Content |
|---|---|
| `+0x00` | vertex-buffer index |
| `+0x08` | owner (the cloth skin, set `+0x10`) |
| `+0x10..+0x3F` | four stream slots, 0xC bytes each: `+0` enabled, `+1` flag, `+4` offset, … |
| `+0x40` | **D3D11 dynamic vertex buffer** (set `+0x18[i]`) |
| `+0x48` | **CPU copy** (set `+0x38` entry `+8`) |
| `+0x58` | **bytes** |

**`FUN_1423D5BC0(drawable, status)`** is a slot of the drawable's vtable (`143D7DF38`; Havok
display object). For each cloth mesh, when the simulation has produced a frame:

1. **Stop** (`status = 1`) when the upload pool `[144964088]` is absent, there is no descriptor,
   or `desc+0x40` (the buffer) is null.
2. **Replicate the duplicated vertices in the copy** (corrected in section 65: not a conversion). `out = [[desc+8]+0x28]+0x18 + 16·desc[0]`
   (`{+4 count, +8 data}`), then:
   - when `[[desc+8]+0x28]+0x0C = 0`: `FUN_14034F1B0(desc, out.data, out.count)`: for each
     packed pair (u32, dst high 16 bits, src low 16 bits) and each enabled slot of the descriptor,
     `memcpy(copy + slot.offset + stride·dst, copy + slot.offset + stride·src, slot.size)`;
   - otherwise `FUN_14034F090()`: the same with u64 pairs (src low 32 bits, dst high 32 bits).
3. **Take a deferred context** from the pool: `ctx = FUN_1423A0130(pool)`, retried with
   `FUN_1417E8060` (yield) until one is available. It is stored at `drawable+0x128`.
4. **Map** the buffer (`FUN_141922B40(vb, 3, …)`, otherwise `FUN_141921860(ctx, ctx+0x10, vb, 0,
   3, &mapped)`; mode 3 of the wrapper. INFERRED: write-discard).
5. **`memcpy(mapped, desc+0x48, desc+0x58)`**: the whole copy, every time.
6. **Unmap** (`FUN_141921940(ctx, ctx+0x10, vb, 0)`), release the context (`FUN_1423A00B0(pool,
   ctx)`), `status = 0`.

The neighbouring slot **`FUN_1423D5D70`** reads the same copy on the CPU (stream slots plus
offsets) to rebuild the drawable's bounding box (`FUN_1412BBAF0` → `drawable+0x80`, copied to
`+0xC0..+0xFC`).

**Contract for the native owner (per cloth mesh):**

| Item | Value |
|---|---|
| Size | `count[vb+0x7C] × stride[vb+0x68]`, rounded up to 4 |
| Layout | stream 0 (L1: float3 position, packed normal, packed tangent) |
| Cadence | one full rewrite per simulated frame, through the drawable slot above. No partial updates, no ring in the engine |
| Ordering | copy written (step 2), then uploaded (step 5) in the same call. The CPU copy also serves the bounds (`FUN_1423D5D70`) |
| Native mapping | hook `FUN_1423D5BC0` (or follow step 2 and replace steps 3–6). Publish a new **immutable version** of the stream-0 bytes with a fence, keyed by the set entry `{vb}`. The selected draw binds the version current at preparation, as `model_streams` does. Keep the engine's `status` return |

**Native today:** the set is empty (`NativeDynamicMeshBufferSet`), so `desc+0x40 = 0` and
`FUN_1423D5BC0` returns at step 1. The simulation output exists but is never copied.

### 2. The two readers of S left open in `NATIVE-READS-AUDIT.md`

**`FUN_140D10DE0` is slot 5 of `NS_SPRJ::SprjGuiSystem`** (vtable `14294ABB8`, constructor
`FUN_140D10C80` from `FUN_140D106E0`).

- That class belongs to the developer GUI framework (`GuiFramework` tweakers:
  `TGUIOnOffTweaker`, `TGUIComboTweaker<CSDebugSfxMove::_MoveType>`, …).
- The slot scales a GUI position by `S / ([14489D5D8] − GX viewport origin·k)` when the GX
  display exists.
- INFERRED: it only serves debug menus, which retail play does not open.
- **Verdict: no visible or input effect in native retail play.**

**`FUN_140E51340` is the constructor of a UI container movie** (vtable `14298ECF8`, from
`FUN_140E5E680`).

- It creates the Scaleform player (`FUN_140E52C50`) and sets an **initial** viewport through
  player vt+0x60: buffer and rect = S, origin 0, scale 1.
- It then sets vt+0x70(1) and vt+0xA8(4) (INFERRED: the view scale mode).
- From the first movie update on, `FUN_140E53880` (natively `NativeUiMovieViewport`) replaces
  that viewport with the output extent.
- **Verdict:** no lasting effect. At most the first frame of a movie, before its first update,
  carries the S viewport. Input is unaffected, because the C++ hit test uses S
  (`FUN_140E65E70`) independently of the movie viewport (section 51).
- The only S-related gap that remains is the one of section 51 / `NATIVE-READS-AUDIT` item 2:
  S's aspect against the output's.

Exports: `decompiled/DS3-1152-F-023D5BC0`, `023D5D70`, `00D10DE0`, `00E51340`.

## 59. NPC summon signs on the High Wall (m30_00, m30_01) and their conditions (STATIC + game data; INFERRED where marked)

Source: `event/m30_00_00_00`, `m30_01_00_00`, `common` and `common_func.emevd` (DS3 EMEVD
0xCD, arguments block at header `+0x78`), and the MSBs. Dumps:
`evidence/I-0067/queries/summon/`.

**Mechanism.** The map's event 0 initializes common event **`20005701`** with these parameters:

| Parameter | Meaning |
|---|---|
| `p0` | boss-defeated flag |
| `p1` | summoned flag |
| `p2` | dismissed flag |
| `p3` | NPC character entity |
| `p4` | sign region |
| `p5` | requirement flag |

Its body:

1. ends if `p2` is set;
2. ends if the boss `p0` is defeated;
3. waits, when `p5 ≠ 0`, for flag `p5` ON;
4. then waits on, all together:
   - **`3[26]` with argument 1** (INFERRED: the player is host, in their own world);
   - **the player has SpEffect 490** (`4[5]`; INFERRED: the embered state, so an **Ember** is
     needed);
   - the NPC character is loaded (`4[7]`);
   - **the player is within 10 m of the NPC** standing at the sign point (`3[3]`, 10.0);
5. then **`2003[25]` with arguments `0, p3, p4, p1, p2`** places a white NPC summon sign.

`20005714`/`20005710` move the summoned NPC into the boss arena. `20005720` handles dismissal.

| Map | Sign region (name) | Position | NPC (MSB) | NpcParam | Boss flag | Requirement `p5` |
|---|---|---|---|---|---|---|
| m30_00 | `3002780` 白霊１ サインポイント | (6.7, −6.3, 87.1) | `c0000_0007`, entity 3000190 | 27920 | 13000800 (Vordt: arena check region `3002829` フルアーマー獣) | **13005392** |
| m30_00 | `3002781` 白霊２ サインポイント | (25.0, −25.5, 52.3) | `c0000_0008`, 3000191 | 27320 | 13000800 (Vordt) | **9500** |
| m30_00 | `3002783` 白霊 御使い前 サインポイント | (−56.7, −39.6, 281.8) | `c0000_0009`, 3000192 | 25120 | 13000830 | **70000007** |
| m30_01 | `3012705` 白サイン_ボス前 | (−15.7, 56.7, 217.7) | 3010191 | 25222 | 13010800 | 70000008 |
| m30_01 | `3012710` 白サイン_ボス前_奇跡屋の騎士 | (−10.8, 54.6, 250.8) | 3010192 | 25821 | 13010800 | 70000017 |

**The requirement flags:**

- **13005392** is maintained by `m30_00` event 13005390 (active until Vordt dies). It follows
  whether the player holds **goods 2117, Basin of Vows** (`3[16]`, item type 3). The exact
  polarity is INFERRED: most likely ON while the Basin is not yet owned, the normal state before
  Vordt.
- **9500** is set by no event of `m30_00`, `m30_01`, `common` or `common_func`. It comes from
  another map or a quest. Not readable without the save's flags.
- **70000007** is set by `common` event 9100 from flag 1103 (progression). Not readable without
  the save's flags.

**Nearest to Argo** (estimated at `c1105_0011`, (−32.7, −0.1, 83.2), section 54): **sign 1,
about 40 m away**, near Vordt's arena. Sign 2 is about 70 m away. The `m30_01` signs use their
own map origin, so distances to them are not comparable.

**Why neither mode shows a sign where Argo stands:**

- the nearest sign point is 40 m away, and a sign appears only within 10 m;
- the player must be embered (Ember);
- Vordt must still be alive;
- the requirement flag must be met.

Exports: none (data only).

## 60. Eyebrows (`FG_A_2012`, `FG_A_7002`): pass, blend, the hair or eyebrow colour, and what can lighten them natively (STATIC + game data; INFERRED where marked)

### Material, textures, pass and blend

Game data: `data2/parts/FG_A_2012.partsbnd.dcx`, `FG_A_7002.partsbnd.dcx`. Each holds `.flver`,
`.tpf`, `.egm`, `.tri` and `.flver2tri`; `FG_A_7002` also has an `.anibnd`. INFERRED from the
part numbers: `FG_A_20xx` are eyebrows and `FG_A_7xxx` are eyelashes.

| Item | `FG_A_2012` | `FG_A_7002` |
|---|---|---|
| Meshes | 1 (7 bones) | 2 (8 bones) |
| Material | `#00#` → **`P[ChrCustom_Hair]_al`** | the same |
| AlbedoMap `…_a` | 512×256, **DXGI 99 = `BC7_UNORM_SRGB`** | the same |
| NormalMap `…_n` | DXGI 98 `BC7_UNORM` | the same |
| ReflectanceMap `…_r` | DXGI 72 `BC1_UNORM_SRGB` | the same |
| VectorTexture `…_v` | ATI2 (BC5) | the same |
| Face sets | **`CullBackfaces = True`** | the same |

`P[ChrCustom_Hair]_al` (section 44) is `SAT_HairIbl2` with `g_BlendMode = 2`,
`g_AlphaRef = 1`, `g_GX_ForwardRendering = True` and depth write on.

**Pass: 0x29, not 0x1F.**
- The material class is 2: blend mode ≠ 0 makes it transparent (`ClassifyModelMaterial`, the
  `142130` rule).
- `57A20`'s forward row is (engine pass **0x29**, preparation 0, class mask **0x12**). It
  admits class 2.
- The 0x1F row the pilot added for the hair (Native1182) has class mask 0x80, which is
  forward, opaque, alpha-tested. That is `P[ChrCustom_Hair]` (blend 0, alpha ref 128), not
  `_al`.
- So the eyebrows and eyelashes draw once, in 0x29, after the opaque body and the 0x1F hair.

**Blend in the original:**
- Material blend 2 goes through the table `14473E5C0[row·6 + blend]`, read by `FUN_140200840`:
  row 0 gives index **1**.
- Entry 1 of the GX common blend table (`FUN_1400D21F0`, group 0, which the forward draws
  use) is:
  - colour **`SRC_ALPHA / INV_SRC_ALPHA`, ADD**;
  - alpha **`ONE / ZERO`**;
  - write mask RGBA.
- The GX blend enum is 0 ZERO, 1 ONE, 4 SRC_ALPHA, 5 INV_SRC_ALPHA, 8 DEST_COLOR (2 is
  multiply, 3 add, 4 subtract).
- Feature bit 15 adds 0x19 (the alpha-to-coverage copies) and bit 17 adds 0x14 (phantom).
  INFERRED: neither is set on a normal eyebrow draw.

### The pixel programme (`SAT_HairIbl2_Fwd_A`, `evidence/I-0067/queries/hair/`)

```
a        = t3 (AlbedoMap, s0)
tint     = sat(sqrt(a.rgb) × cbModelParam[6].rgb)²        // FC_SkinMatchColor, b7 offset 96
wet      = lerp(tint, FC_WetDiffuseColor, wetWeight)      // cbMatDynParam rows 35..38
albedo   = lerp(wet, decal.rgb + wet·decal.a, FC_BloodAmount)  // t41, BloodAmount = 0.9
lit      = albedo·(direct + SH/IBL·SSAO) + hair specular + rim
o0.rgb   = lit × Instance[7].rgb, then fog
o0.a     = a.a × Instance[7].w                             // group +0xD0..+0xDC
discard  if a.a² + (dither − 0.5)·FC_AlphaRef.y < FC_AlphaRef.x, or o0.a = 0, or the dither fade
```

**The eyebrow colour is `FC_SkinMatchColor` (PS `cbModelParam` b7 offset 0x60).** It
multiplies the albedo in a gamma-2 space. There is no other colour input in this programme.

### Where the appearance colour comes from, in game and in the menu

**`g_useHairColor` is not on this path:**
- Only two old FRPG MTDs carry it, `Ps_Eye[DS]` and `Ps_FaceGen[DS][M]` (a scan of the 512
  files in `mtd/AllMaterialBnd`).
- `FUN_140D40790` writes it to material-parameter set `+0x1C4`.
- No reader of `+0x1C4` was found in `0x140D30000..0x140D50000`.

**In game (the player `SprjAsmModelDrawEntity`, vtable `2947DB8`):**

1. `FUN_1408C1BC0` (the in-game character update) converts eight appearance colours with
   `FUN_140592DD0(appearance + 0x698, i)`.
   - The indices are 0, 1, 2, 3, 4, 8, 5, 6.
   - They are stored at **`asmModel+0x1C0 + 0x10·c`** (c = 0..7), where asmModel = player+0x20B0.
   - Each change sets dirty bit 15 of `asmModel+0xF8`.
2. `FUN_140C48180` (the asm model update) sees bit 15 and calls draw entity vt+0x130 =
   **`FUN_140D009C0(E, c, &colour)`**, which writes **`E+0xBD0 + 0x10·c`** for c < 8. This
   function is not disassembled by Ghidra; linear listing.
3. The colour index of each part is the byte at **descriptor +0x48**, where descriptor =
   `[E+0xB00] + 0x50·i`.
   - It is written by `FUN_140C48950` from `FUN_140C34CD0`'s 13-row slot table.
   - Part 6 (slot 5, hair) gets colour **1**.
   - The FaceGen parts 0x26..0x2B (slots 0x16..0x1B) get colours **2..7**.
   - Body and armour parts keep the default.
4. At draw, `FUN_140CFDE70` (each part, from `FUN_140D02A30`):
   - copies the entity group `E+0x240` to a **stack group** (`FUN_14004E180`);
   - replaces **group+0x40 with `E+0xBD0 + 0x10·descriptor[+0x48]`**;
   - sets dirty +0x180.
   - Group+0x40 becomes draw state +0x280, which becomes `FC_SkinMatchColor`.
   - In the same copy it also overrides +0x84/+0x87 and **+0xD0..+0xDC** (`mulCol` scaled by
     the part fade handle at descriptor +0x44), +0xC0..+0xCF (`FUN_1405B0800`, dirty +0x181),
     and +0x98 bit 0 (cloth state).

**Menu (`CSChrAsmModelIns`):**
- The colours are kept at `model+0x290 + 0x10·c` (`FUN_140C3B870`).
- They are written to `item+0x780` of the matching slot (`FUN_140C3B250`: slot 6 → 1,
  0x11..0x16 → 2..7).
- `FUN_140D3DA20` copies `item+0x780` to group+0x40 before each item (section 26).

INFERRED: which colour index of 2..7 is the eyebrow colour (rather than beard, eyelashes or
eye) was not established. The FaceGen type order `FUN_140C3E430` (0x11..0x16 → appearance
2, 3, 4, 8, 5, 6) is the same in both paths.

### Natively today (pilot branch `14c168c6`)

**Menu parts:** `candidate.draw_colour = item+0x780` is copied to state+0x280
(`game_bootstrap.cpp:3724`, `:4390`). This is correct per item.

**World `SprjAsmModelDrawEntity` parts** (`:3790`):
- `candidate.group = entity+0x240` is read as is, with no draw colour.
- So `FC_SkinMatchColor = E+0x280`, the shared group's +0x40. The original never writes it:
  the per-part value exists only in `CFDE70`'s stack copy, and Reference1097 measured it at
  zero.
- Every world character part, eyebrows, beard, hair and skin alike, then gets **`tint = 0`**:
  black albedo, with only specular and rim light left.
- The same copy's per-part `mulCol` fade (+0xD0..+0xDC) is also skipped.
- **Fix (generic, by family):** for each `SprjAsmModelDrawEntity` part i, produce state+0x280
  from `E+0xBD0 + 0x10·[desc_i+0x48]`, and apply `CFDE70`'s other group overrides, exactly as
  the menu path already does with `item+0x780`.

### Everything that can make the eyebrows paler natively (checks, in order)

1. **Tint.**
   - In the world, see above. Black albedo plus strong hair specular and rim light
     (`g_HairSpecularIntensity 0.3/3`, `g_RimLightingIntensity 3`) reads as grey strands.
   - In the menu, verify that state+0x280 equals the eyebrow item's `+0x780` on the 0x29
     draw, and not colour 0 or 1.
2. **Alpha source.**
   - `o0.a = a.a × Instance[7].w`. A `mulCol.w` below 1 (group +0xDC, or a menu visibility
     fade written there instead of +0x124) lets the skin through.
   - The alpha test uses `a.a²` against 1/255. That threshold is meaningful only for alpha
     near 0.
3. **Blend.**
   - Native colour `SRC_ALPHA / INV_SRC_ALPHA` matches the original.
   - Native alpha is `ONE / INV_SRC_ALPHA`; the original is `ONE / ZERO`. This changes only the
     target alpha. It matters only if a later pass reads the alpha of the forward target
     (INFERRED: the tone map does not).
4. **sRGB.**
   - The native SRV format is the DDS header's (`DecodeNativeTextureDDS` → `NativeTextureView`),
     so `BC7_UNORM_SRGB` decodes to linear, as the shader expects (`sqrt` then square).
   - INFERRED: the GX loader uses the same header format. A UNORM view would brighten every
     albedo, not only the eyebrows.
5. **Sampler and mips.** The AlbedoMap uses `SS_DiffuseMap` (s0). A coarser mip, or a filter
   without anisotropy on 2–4-texel strands, averages alpha toward 0. Compare the sampler bank
   entry and LOD bias with the original.
6. **SSAO (t33).** It darkens the ambient term of the forward hair. A white dummy natively
   brightens the brows. Compare the SRV bound at t33 on the 0x29 draw.
7. **Lighting inputs.**
   - The forward `cbLight` and `cbFog` (`ProduceNativeForwardLight`), IBL t30/t31 and the
     point-light lists t59..t62.
   - The hair specular (`cbMtdParam` rows 20..22 `FC_HairSpecularParam0..2`) dominates once
     the albedo is dark, so a wrong row makes dark brows look pale.
8. **Double blend.** Not possible through the rows: class 2 is admitted only by the 0x29 row.
   Count the draws of the eyebrow mesh per view to confirm one.
9. **Decal t41.** `FC_BloodAmount = 0.9` lerps toward `decal.rgb + albedo·decal.a`.
   - The native common absent decal is (0, 0, 0, 1), which is neutral.
   - A provider texture with rgb > 0 would add 0.9·rgb and lighten.
10. **Depth.** The original writes depth for `_al` (`g_GX_bDisableDepthWrite = False`), and the
    native forward state does not. That changes how later forward strands overlap (more
    coverage natively, so darker), not the brightness of one layer.

**Not an eyebrow cause, but a family-wide divergence:** native model draws always use
`CullMode BACK` (`game_bootstrap.cpp:4215`). The FLVER face sets carry `CullBackfaces`, and
`HR_A_0106`'s hair meshes 0, 1, 3, 5, 7..11 are **`CullBackfaces = False`** (two-sided cards).
INFERRED: the GX rasterizer selection honours that flag. Natively the back faces of those cards
are culled. The eyebrow and eyelash face sets are `True`, so they are unaffected.

Exports: `decompiled/DS3-1152-F-008C1BC0`, `00C48180`, `00C34CD0`, `00CFDE70`,
`00D02A30`, `00D00810`, `00200840`, `000D21F0`.

## 61. In-game HUD: Scaleform pages, icons and markers, what is not Scaleform, and where native mode can diverge (STATIC + game data; INFERRED where marked)

Data: the `.gfx` files of `data1/menu/` (tag inventory with a small SWF reader; the binary images
stay in the scratchpad). Code: section 50's page table, section 33 (the HAL) and section 51 (the
cursor and S).

### The HUD movies

| Page | Opened by | Content (tags, exported images, ActionScript identifiers) |
|---|---|---|
| **`01_000_FE`** (554 KB, 374 sprites, 21 text fields, 46 external images) | `FUN_140AFE360` (AppMenu window factory `FUN_140BB5110`, page name `1428D8730`), from `FUN_140B3C160` ← `FUN_140B3EA10` ← `FUN_140B3D910` ← `FUN_140764890` (the game step that opens the in-game menu system) | Everything listed below |
| **`01_001_FE_Soul`** | `FUN_140AFB790` (from `FUN_140AF9510`) | Soul counter: `GetSoul`, `TotalSoul`, image `MENU_PlayerHUD` |
| `01_060_Caption` | — | The dialogue and subtitle window (`CaptionWindow`, 256 frames, `Text/Text`, `MenuFont_01`); see [SUBTITLES-DIALOGUE-NATIVE.md](SUBTITLES-DIALOGUE-NATIVE.md) |
| `01_002_FE_SaveIcon` | — | Autosave icon |
| `01_090_SummonMessage`, `01_080_EmergencyNotice`, `01_050_GraveMessage`, `01_020_Sign`, `01_030_BloodMessage` | — | Other messages and signs |

What `01_000_FE` contains:
- **Player bars:** `MENU_PlayerHUD`, `MENU_PlayerHUD2`, `MENU_HUD_Status`, `HP_Base`, `Gauge`
  (HP, FP, stamina).
- **Equipped items:** `ItemPanel`, `Item_0_0..Item_4_0`, `ItemIcon`, `ItemBreakMark/2`,
  `MENU_ItemPanel_02`, `MENU_ItemParts`, `ReloadedIcon`.
- **Status effects:** `ConditionIcon`, `MENU_Condition`.
- **Attributes:** `AttributeIcon`, `MENU_AttributeIcon`.
- **Lock-on:** `MENU_Lockon01`, `Lockon_Ani`, `TargetCursor_Ani`, `MENU_Reticle`.
- **Area name:** `MapName_Ani`, `MapName_txt_Ani`, `MENU_MapName_Underline`.
- **Action prompts:** `Act_Message_Text_Ani`, `ActionButton_Ani`, `CautionText_Ani`.
- **Chat:** `TalkWindow_Ani`, `Chat_IconImage`.
- **Banners (full-screen text images):** `MENU_YOU_DIED`, `MENU_BONFIRE_LIT`,
  `MENU_LORD_OF_CINDER`, `MENU_LOAD_OF_CINDER_FALLEN` (the game's spelling),
  `MENU_TARGET_WAS_DESTROYED`, `MENU_VICTORY_ACHIEVED`, `MENU_YOU_WIN`, `MENU_DRAW`,
  `MENU_RETRIEVAL`, `MENU_REVIVAL_FROM_DECEASED`, `MENU_INVADER_BANISHED`,
  `MENU_REDHUNTER_OK`, `MENU_BERSERKER_OK`, `MENU_MATCH_START`.
- **Online:** covenant and arena `MENU_PledgeIcon`, `PledgeIcon_HighlightAnim`, `Medal`,
  `MENU_ColiseumMedals`, `Score`, `TotalScore`.
- **Placeholders:** `MENU_DummyIcon_00000..09000` (16 slots).
- Fonts `MenuFont_01/02`. The `Fade` clip is here too.

**INFERRED:** the boss bar and its name, and the enemy bar shown after a hit, are clips of
`01_000_FE`. The legacy classes `FrpgMenuDlgBossGauge`, `FrpgMenuDlgFEEnemyGauge`,
`FrpgMenuDlgPCGauge`, `FrpgMenuDlgFloatingPCGauge`, `FrpgMenuDlgObjGauge` and
`FrpgMenuDlgResistGauge` have RTTI but **no code reference to their vtables** (`1427F5588`,
`1427F6AB8`, `1427FC388`, …): they are DLRF registrations from the older engine, not the
retail HUD. The `AppMenu` side has `CSFeView` (`1428D88E8`), `CSFeGaugeComponent`,
`CSFePlayerGaugeComponent` and `CSFeAutoHideCtrl`.

### Images and icons

- **Most `MENU_*` images** are textures in `menu:/01_Common.tpf`, BC7 (section 52). Examples:
  `MENU_PlayerHUD`, `MENU_PlayerHUD2`, `MENU_HUD_Status`, `MENU_Lockon01`, `MENU_Reticle`,
  `MENU_Condition`, `MENU_AttributeIcon`, `MENU_ItemPanel_02`, `MENU_ItemParts`,
  `MENU_MapName_Underline`, `MENU_KUROOBI`, `MENU_PledgeIcon`, `MENU_ColiseumMedals`,
  `MENU_MessageBox`, `MENU_AutoSave`.
- **Banners** are not in `01_Common` (the 208-row inventory). INFERRED: they are in
  `menu/<lang>/80_language.tpf`, chosen by language.
- **Item icons:**
  - the movie references `MENU_DummyIcon_xxxxx` (from `05_Dummy.tpf`);
  - the runtime replaces each with the sheet `MENU_Icon_xxxxx` of `01_Common` (16 sheets);
  - the C++ side then picks the cell for the equipped item;
  - binding goes through the HAL image-creator path of section 52 (`FUN_140E60160` →
    `FUN_140E48980` → `FUN_140E49320`).

### What is not drawn by Scaleform

| Element | Path | Native status |
|---|---|---|
| Lock-on marker and reticle | **Scaleform** (`01_000_FE` `TargetCursor`/`Lockon_Ani`). `FUN_140AF89D0` uses `"TargetCursor"` (`1428D8B88`) and places the clip in screen space from the target's world position (INFERRED: through the HUD coordinate helper `FUN_1406C73B0`, × virtual/S.w × S.w) | Follows the native Scaleform HAL. It depends on the camera projection the C++ side reads |
| Full-screen fades | `SprjFadeImp` plates on GX layer `+0x40`, after Scaleform (sections 33, 38) | Separate native owner |
| Hit blood, sparks, the "Bonfire" flame and `MENU_*` 3D signs | **FFX effects** (I-0068) | Outside this section |
| Damage and resistance numbers | No retail path (the `ResistGauge` and debug menus are developer tools) | — |
| Bloodstain and message signs on the ground | FFX and SFX objects. Their **text** opens `01_030_BloodMessage` | — |

### Where native mode can diverge (checks, in order)

1. **Movie viewport against S** (`NATIVE-READS-AUDIT` items 1–2):
   - `NativeUiMovieViewport` uses the output extent;
   - `FUN_140E65E70` and the HUD placement use S and the movie frame aspect.
   - When the output aspect differs from S, positioned clips (lock-on cursor, enemy bar,
     floating text) drift from their world target, while full-screen clips look right.
2. **World-to-screen inputs of the positioned clips:**
   - the lock-on and target clips are placed by C++ from the game camera;
   - natively, the camera the game code reads must be the one the scene is drawn with. A
     native projection with another FOV or aspect makes the marker miss the target even with
     equal viewports.
3. **Icon binding:** `MENU_Icon_*` must replace the `MENU_DummyIcon_*` placeholders. A refused
   or late import (`native_ui_asset_unavailable`, section 53) shows the placeholder square or
   nothing.
4. **Language TPF:** the banners (YOU DIED, BONFIRE LIT, …) need `80_language.tpf` for the
   current language to be imported like `01_Common`.
5. **Fonts and the glyph cache** (`MenuFont_01/02`, glyph cache `+0xFE0`, section 53 audit):
   the soul counter, item counts, area name and messages.
6. **Filters:** the HUD frame uses two filter targets (section 33, I-0055), for glows and
   shadows on the bars and banners. The native HAL must support `PushFilters`/`PopFilters`
   (section 36), or those effects disappear.
7. **Blend modes** of the HUD layers (section 33's 0..18 table). Bars and banners use the
   normal and add modes. Refused modes are logged as `native_ui_blend_mode_%u_unavailable`.
8. **Ordering:** the HUD is drawn after tone mapping into the 8-bit target, then copied to
   the back buffer. The GX fade plates come after it.

Exports: `decompiled/DS3-1152-F-00AFE360`, `00AFB790`.

## 62. Characters and objects on the High Wall: draw enable, bounds, LOD fade, the dynamic tree layer and its traversal (STATIC; INFERRED where marked)

Answers to the pilot's non-player-character questions (`claude/npc-enemies`), in the order they
were asked.

**Correction to section 54.** The "Argo" position used there is `c1105_0011`'s own MSB
position. The pilot measured that **no `c1105` model data exists in either mode** in this save
(the name at `[[E+0xE8]+8]+0x2B8`), so `c1105_0011` is not instantiated (INFERRED: dead and not
yet respawned, or disabled).
- `c1105` has its own FLVER (`data3/chr/c1105.chrbnd.dcx`: `c1105.flver`, 120 bones, 15 meshes,
  `C[ARSN]_Decal`/`_e_Decal`, cloth `c1105_c.hkx`). It is not `c1100` (118 bones).
- The object missing in front of the camera is **`o001050_3208`**, 1.9 m from that point.
  - It is a small breakable: 1 bone, `M[ARSN]`, a broken variant `o001050_1.flver`, physics.
  - MSB: DrawGroups 1–2 all bits, DispGroup1 bit 31, `LodParamID` 22.

### 1. `SprjModelDrawEntity` draw enable (`E+0xC3C` bit 0)

- The constructor `FUN_140D05C80` sets it: `C3C & ~0x3E | 1`.
- Its only other writer is **vt+0xD0 = `140D074A0`** (not disassembled by Ghidra): `C3C = C3C
  & ~1 | v`. The only caller is the thunk `FUN_140D06850`.
- That thunk is called from **`FUN_140C4AF10`**, the CS model wrapper update (object M, with
  E = [M+8]).
  - The call happens when the dirty word `M+0x70 | M+0x74` has bit 7, or `FUN_140D3BF40(M+0x98)`
    changes.
  - The value written is `(M+0x8B >> 2) & (M+0x8C >> 3) & (M+0x8D >> 4) & 1`.
  - These are three visibility bits, set to 1 by the creators `FUN_140C49210`/`FUN_140C49590`.
- So bit 0 = 0 means the game hid the model, and the original does not draw it either.
- Other writers of C3C:
  - vt+0xE8 `140D072E0` clears bits 1–2;
  - `FUN_140D07330` sets bit 5;
  - `FUN_140D08430` swaps the matrix buffers (bits 1–4).

### 2. Bounds `E+0xB80` and registration in the tree

**Per-model update chain:**
1. ChrModel update `FUN_1407728A0` (a vtable slot).
2. `FUN_140C4AF10`:
   - When placement is dirty (`(M+0x70|M+0x74) & 3`), mode M+0x58 picks
     `FUN_140D06A00` (pose compose `FUN_140D33EE0`), `FUN_140D06AA0` (`D33E50`) or
     `FUN_140D06B40` (a copy). All three end in **vt+0xA0 = `FUN_140D076A0`**: it writes the
     bone matrices, then `BA8 |= 5`.
   - Then it calls vt+0xC8 = **`FUN_140D07710`** (entity update).
3. `FUN_140D07710` calls first **`FUN_140D08190`**, which runs only if `BA8` bit 2 is set:
   - the box starts empty;
   - `FUN_140D2DB00([E+0xE8], box, the matrices at E+0xB08/+0xB20 chosen by C3C bit 3)` fills
     it from the bones;
   - an invalid box becomes a zero box;
   - it writes the world boxes `E+0x110` and `E+0x1B0` (flags +0x185, +0x225);
   - **`FUN_14002D800(E, box)`**:
     - `E+0x40` = box;
     - **`[E+0x18]` vt+0x30**;
     - the occlusion node `[E+0x68]+0x20` = box;
   - `BA8 &= ~4`.

**The constructor does not initialise B80.** An entity that never received `FUN_140D076A0`
keeps no valid bounds and stays where the tree first put it.

### 3. LOD and distance fade (the native `zero_fade` fields)

The native fade layout (normal track +0x160, weight +0x16C, geometry weight +0x170, variants
+0x178/+0x17C, alternate track +0x200) is the **embedded LOD controller at `E+0xF0`** (alternate
at `E+0x190`). It is built by `FUN_140D1C940(E+0xF0, id)`:
- LodParam row = `FUN_140E0FAF0([144796DD8], id)`;
- id = byte +3 of `FUN_140D01D20`'s record;
- initial values: weights 1.0, LOD 0;
- it is registered in the LOD manager `[[144796298]+0x10]+0x4F0` by `FUN_140D1DCC0`.

**Only writer: `FUN_140D1CCE0(record, mgr)`**, each frame, through FD4 tasks created by the
manager's constructor `FUN_140D1D150`:
- the step-0x4F task `FUN_140D1DE70` builds a GX camera from the game camera `[144789CD8]`
  (`FUN_140989E10`, `FUN_140CED670`) and copies it into the manager (`FUN_14006E9E0`: position
  +0x80, size +0x110/+0x114, scale +0x150);
- three step-0x50 tasks `FUN_140D1DE10` each take a third of the list.

What `FUN_140D1CCE0` does, when there is a LodParam and record+0x95 is set:
- `d = |centre(record+0x20 = E+0x110) − mgr+0x80| × mgr+0x150`;
- it selects the LOD with a cross-fade (+0x70/+0x74/+0x78);
- **+0x7C (weight) and +0x80 (= E+0x170, the geometry weight for passes 1..0x38) move one step
  per frame toward 1 or 0**:
  - toward 1 while `d < table[14463F5A0 + 4·param+0x2C]·param+0x14 + [144791DF8]`, ±
    `param+0x18` hysteresis (for +0x80: `14463F6A0`, `param+0x1C/+0x20`, `[144791DFC]`);
  - the steps are dt / fade time (`FUN_140D1CAF0`);
  - the first frame (+0x91) is instant.

A zero geometry weight therefore means "farther than the draw distance" for the manager's
camera.

### 4. The dynamic layer and how the visitor gets its entities

**Classes (RTTI):**

| Object | Class | Notes |
|---|---|---|
| `[[144796298]+0x10]+0x50` (`FUN_140DFDA10`) | **`GXSgLayerDynamicTree`** | vtable `143D4A448` (`0x3D4A748` holds UTF-16 text in the image) |
| its tree at +0x28 | **`GXSgNumTree`** | vtable `143D4A428`; nodes `GXSgNumTreeNode` (`143D61A38`), root `143D619D8` |
| `[[144796298]+0x10]+0x58` (`FUN_140DFDA20`) | occlusion system | vtable `143D4A4C8`, see below |

**Insertion (`FUN_1408853C0`):**
- `layer vt+0x40 = FUN_1400C89A0(layer, E)`:
  - the link `E+0x28 = {E, node}`;
  - a 13-bucket hash set at +0x88;
  - the NumTree insert by `E vt+0x18` → `FUN_140D07E10` → **E+0xB80**;
  - **`E+0x18 = layer`**.
- `FUN_14002D860(E, occlusion)` creates the occlusion node (`FUN_1400C9E60`: 0x80 bytes,
  AABB = `E+0x40`, state 4, pushed to +0x118).

**Move:** `FUN_14002D800` → `layer vt+0x30 = FUN_1400C8C00` → node vt+0x20 =
**`FUN_140207B40`**. Synchronously, it moves E down into the child cell that contains E+0xB80,
or grows the node's own bounds (+0x80) and masks (+0xD0/+0xD4, and +0xD8 up the tree).

**Traversal:** `FUN_1400C1FB0(world, V)` → each layer `vt+8`. For this layer: `FUN_1400C8BC0`
→ `FUN_1400C86B0` → root `FUN_14020A700` → nodes **`FUN_140208180`** (recursive).

For each node:
1. It is skipped when it has neither entities nor the +0xE4 flag.
2. **Subtree mask:** `V.lo & node+0xD8` and `V.hi & node+0xDC`.
3. **Occlusion,** only if V vt+0x28 (`byte V+0x10`) is set. The subtree is skipped when its
   occlusion cell `[node+0x18]` has "visible until" (+0x44) ≤ the occlusion frame (+0x160) and
   state (+0x78) ≤ 2.
4. **Frustum:** V vt+0x18 = `FUN_140234840` on the loose bounds `node+0xA0`, then culler vt+0x60
   (0 = outside, 2 = inside: the descendants are accepted without testing).
5. The children (or parallel jobs when V vt+0x38 `byte V+0x11`).
6. **The node's own entities:** mask `node+0xD0/+0xD4`, frustum on `node+0x80`, then for each
   link `(E+0x20).lo & V.lo` and `(E+0x20).hi & V.hi`, then **V vt+0x50(E)**.

The original rappel `FUN_140142E80` (EntityCollect / WorldRenderer) then:
- skips E when the occlusion test is on and its occlusion node state is ≤ 1 and not stale;
- frustum-tests E+0xB80 (unless the parent was fully inside);
- calls E vt+8 directly, or queues it.

**The visitor's category byte (`FUN_1400C2EC0`, `GXSgDrawScene`)** is `(scene+0x14 qword &
~0xFF)` | a bit chosen by engine pass:

| Engine pass | Bit |
|---|---|
| 1..4 | 0x40 |
| 5..8 | 0x02 |
| 9..0x38 | 0x04 |
| 0x39 | 0x10 |
| 0x46 | 0x20 |
| other | 0x01 |

The native traversal uses `|1`. The pilot then checked: every character carries the right bit,
so this was not the cause (324 against 323 objects after widening).

**Occlusion system (`143D4A4C8`).** It is not a filter natively, because the visitor bytes
+0x10..+0x12 are 0. In the original:
- **D3D11 occlusion queries** are recorded in engine pass 0x45 (`FUN_1400CA6D0`) and executed
  on the GX side (`FUN_1400CAC50`);
- the per-view bookkeeping `FUN_1400CA050` (from `FUN_140E00270`) sets node state 4 visible,
  0 hidden by the cell hierarchy, 1 hidden by its own query (< 11 samples), 3 retest;
- near the camera (`max(cam+0xB4, node+0x10)` sphere) a node is always visible;
- the cell hierarchy is the 0x14020Cxxx/0x14020Dxxx functions (states +0x78, counters +0x40/+0x4C).


### 5. Map objects (`SprjModelDispEntity` holding o-models): live test, LOD, bounds

Map objects are not in the dynamic tree. The two inserters into `GXSgLayerDynamicTree` are
character code:
- `FUN_1408853C0`, a ChrIns with its model at +0x48;
- `FUN_1408B76A0`, a PlayerIns with its model at +0x20B0.

An object registers through `FUN_14067A5A0` (MSB object creation) → **`FUN_140662610(objIns,
src, layer, …)`**:
- `objIns+0x2F8 = src`;
- `FUN_140C4C3C0`, then `FUN_140C4DAF0` (dynamic texture set-up of the model);
- `FUN_140C4C660`.

Its draw entity is a `SprjModelDispEntity` (vtable `29489A0`, constructor `FUN_140D03EB0`).

**Live test** (`FUN_140D04B90`): `FUN_140D03050(E)` → `FUN_14064F000(E+0x39C, ·)` →
`FUN_140619800`, which is true when `DrawGroups ∧ DispGroups ≠ 0`.

| Words | Meaning | Writer |
|---|---|---|
| `E+0x39C..+0x3BB` | the part's **DrawGroups** (zero in the constructor) | `FUN_140C4CCF0(M, g)` = `[M+0x18]+0x39C`, once at set-up, from `FUN_140669D90` (groups `FUN_140659E20` of the MSB part) and `FUN_14064AD20` |
| `[E+0x70]+0x08..+0x27` | the block's current **DispGroups** (a `FUN_140D039E0` object created by `FUN_14061DF10` at `block+0x3E8`) | **`FUN_140621730` → `FUN_14061AA60`** every frame. The field step `FUN_140606BC0` (`FUN_140607570` ← `FUN_1408F1EB0`, step table `.rdata 1440398D8`) ORs the DispGroups of the collisions the player is in. `FUN_140621930` resets them |
| `[[144796298]+0x10]+0x514..+0x533` | the global DispGroups, used when `E+0x70` is null | `FUN_140DFDAA0`, from `FUN_14062DDF0` (← `FUN_1408FB610` ← `FUN_1408F9AD0`) |

For `o001050_3208` the DrawGroups are `{FFFFFFFF, FFFFFFFF, 0, …}`. A failing live test
therefore means the current DispGroups are zero in words 0–1.

**LOD:** the constructor builds the same controller `FUN_140D1C940` at **`E+0xB0`** (id =
`byte E+0x84`, the LodParam; MSB `LodParamID` 22 for `o001050`) and at **`E+0x150`**. The native
layout {0x120, 0x1C0, 0x130} is +0xB0+0x70, +0x150+0x70 and +0xB0+0x80. Same writer
`FUN_140D1CCE0` as in subsection 3.

**Bounds:**
- `classify(culler, E+0x40)` is `FUN_14006F310`. It returns 0 at once when
  `FUN_14001FFF0(culler+0x220)` is false; otherwise it tests **6 planes at culler+0x160..+0x1BF**.
- Runtime checks: the culler flag and planes against the game camera (`[144789CD8]` →
  `FUN_140989E10`, `FUN_1404095F0`) and the player position (`[[14477FDB8]+0x80]`).

Exports: `decompiled/DS3-1152-F-00C4AF10`, `007728A0`, `00D06A00`, `00D06AA0`, `00D06B40`,
`00D07710`, `00D08190`, `00D08430`, `00D1C940`, `00D1CCE0`, `00D1D150`, `00D1DE10`,
`00D1DE70`, `0006E9E0`, `000C89A0`, `000C8BC0`, `000C8C00`, `0020A700`,
`00208180`, `00207B40`, `0002D800`, `0002D860`, `000C9E60`, `000CA050`, `000CA6D0`,
`000C2EC0`, `000C1FB0`, `00234620`, `00234840`, `00142E80`, `008853C0`, `008B76A0`,
`00662610`, `00C4CCF0`, `00D03050`, `0064F000`, `00619800`, `00D03EB0`, `00621730`,
`0061AA60`, `00606BC0`, `0006F310`.

## 63. Current DispGroups: from the player's ground collision to each map block's group source (STATIC; INFERRED where marked)

Follow-up to section 62 subsection 5. **Pilot measures, natively:**
- the camera, LOD and culler planes match the original;
- Argo is near (30.7, −24.1, 54.9), so `o001050_3208` is genuinely outside the view;
- objects within 10 m (`o302590` ×7, `o004660`) fail the live test because their source
  `[E+0x70]+8..` is all zero, while two other instances pass.

### The chain

1. **The player's collision.**
   - Player P = `FUN_1408D9D70([14477FDB8])`: `[[14477FEC8]+0x150]` when that is set, otherwise
     `[WorldChrMan]+0x80`.
   - The current hit is **`P+0x1AC8`**. Its only setter is **`FUN_14088F870(chr, hit)`**:
     - it refuses while `chr+0x1ADC` ≠ 0;
     - it accepts a non-null hit only with `hit+0xE8` bit 0 set;
     - it copies the hit's groups to `chr+0x1A5C..+0x1ABB` and updates `+0x1ABC`/`+0x1AC4`.
   - Callers: `FUN_14088E480` (INFERRED: the per-frame ground probe), `FUN_14089B170`,
     `FUN_14096C240`, `FUN_140606AA0`, `FUN_140608A00`, `FUN_140528230`, `FUN_140528DA0`.
2. **Copy to the field area FA** (`FUN_1408FAE00`, called by `FUN_1408F9AD0`).
   - It runs when the player exists and `[14477FDB8]+0x90` = 0.
   - It calls **`FUN_1406072B0(FA, P+0x1AC8)`**:
     - `FA+0x60` = the hit when it is non-null;
     - when `FA+0x40` < 0, `FA+0x20` = `hit+0x5B` (the block index).
   - `FA+0x68` is a forced hit (`FUN_140607300`, from `FUN_140CD9BF0`), used once.
3. **Group computation.** The state `FUN_1408F1EB0` (state table `.rdata 1440398D8`) runs
   `FUN_140607570(FA)` → **`FUN_140606BC0(FA, hit, connect)`**:
   - **A null hit returns at once**: nothing is written, so the groups keep the zero of
     `FUN_140621930`.
   - `groups[]` starts at zero.
   - `groups[hit+0x5B] |= hit+0x7C..+0x9B` (the hit's DispGroups).
   - **When `connect` is true:**
     - for each node of the list `hit+0xE0` (node[0] = next, node[1] = connect collision C), and
       each block b (`[FA+0x10]+0x20`, stride 0x70, count `+0x18`; map id from
       `FUN_14044DEE0`, packed AA BB CC DD);
     - if `[C+0xB8]+4..+7` matches byte by byte (−1 is a wildcard), then
       `groups[b] |= C+0x6C..+0x8B` (`FUN_140E13170`).
   - **`connect`** = (`X+0x5A` ≠ 0 and `X+0x44` = 1) or `X+0x44` = 2, with X =
     `[14489C7A8]` (`FUN_140EBF780` reads +0x44, `FUN_140EBFAD0` tests = 1). INFERRED: the
     area's load or session state.
4. **Writing:**
   - `FUN_140621730` → `FUN_14061AA60` writes, for every block i < `[FA+8]+0x12B8` (stride
     0x438 at `[FA+8]+0x12C0`), `block+0x20..+0x3F` and `[block+0x3E8]+8..+0x27`. That is
     the group source an object reads through `E+0x70`.
   - The same groups are copied per block into `[14477DBC0]+0x28` (stride 0xA8),
     `[14475BAE8]+0x28` (0x20118), `[14477FDB8]+0x28` (0x138, the character side),
     `[14475C170]+0x28` (0x40288), `[14475C198]+0x28` (0x90, at +0x38) and `[14475C1A8]+0x28`
     (0x58).

**Consequence:** an object whose source is all zero sits in a block other than the hit's, and
gets no group from a connect collision. That happens when `connect` is false, or when no
connect collision of the hit names that block.

### Runtime checks (same frame, both modes)

1. `P+0x1AC8` is non-null.
2. `FA+0x60` (or +0x68) equals it, and `hit+0x5B` gives its block.
3. **`[14489C7A8]`: byte +0x5A and dword +0x44** (the `connect` condition).
4. For an object at zero:
   - find b with `[[FA+8]+0x12C0 + 0x438·b + 0x3E8]` = `[E+0x70]`;
   - check that b ≠ `hit+0x5B`;
   - note block b's map id.
5. Walk `hit+0xE0`: some C must have `[C+0xB8]+4..+7` matching b, and groups `C+0x6C..`
   with the object's bit (for example word 2 bit 0x200).

### Who writes `X = [14489C7A8]`

- **Construction:** `FUN_140EBF410` (vtable `1429A7108`), from the game set-up `FUN_140EDED40`.
  It sets **+0x44 = 2** and +0x40 = 2, and registers its FD4 task (`X+0x10`, step 4, callback
  **`FUN_140EBF820`**).
- **+0x44 each frame (`FUN_140EBF820`):**
  - `X+0x6C` when it is ≠ −1 (a forced value);
  - otherwise `[X+8]` vt+0x20 (a provider built by `FUN_140EC0640`, vtable `1429A7538`;
    INFERRED: a mode or session state).
  - When the state is 1, it creates the helpers at +0x48/+0x50; otherwise it releases them.
- **+0x5A/+0x5B:** `FUN_140EBFB00(X, b5B, b5A)`.
  - `FUN_1408F0600` (a field state) writes `+0x5B = [·]+0x247` and `+0x5A = 0`.
  - `FUN_140628F10` and `FUN_140ADBE40` write `+0x5A = 1`.
  - `FUN_140BCF100` writes 0.

**What native mode can change:**
- If X's task never runs, +0x44 stays 2 and `connect` stays true.
- If it runs and the provider answers 1 while `+0x5A` = 0, or answers 0, then `connect` is false.

**Runtime check:** X+0x44, X+0x6C, `[X+8]` vt+0x20's value, and X+0x5A, compared with the
original on the same frame.

Exports: `decompiled/DS3-1152-F-00606BC0` (existing), `00607570`, `008F1EB0`, `008FAE00`,
`0088F870`, `008D9D70`, `00EBFAD0`, `00EBF780`, `00E13170`, `00605440`, `00EBF410`, `00EBF820`, `00EBFB00`, `008F0600`.

## 64. Face-set culling, the last per-part overrides, the FD4 frame steps, and character cloth end to end (STATIC; INFERRED where marked)

### 1. `CullBackfaces`: field, reader, rule

**The field is per face set, not per mesh.**
- GX face sets are at `[model+0x120]` (0x20 bytes each).
- A mesh (`model+0xA8`, stride 0x48) holds the count at **+0x28** (u16) and the index array at
  **+0x30** (i32).
- In face set R:
  - `R+0x00` flags: bit 31 is the `0x80000000` variant, and `byte R+3 & 0xF` is the LOD level;
  - **`R+0x0C` is the cull class 0, 1 or 2**.
- **Measured by the pilot (section 65): class 2 is BACK; by the mirror table, 0 is NONE and 1 is FRONT.** The earlier inference (1 = BACK) was wrong. The loader's writer was not read.
  Runtime check: `[model+0x120]+0x20·i+0xC` for `HR_A_0106`.

**Reader: `FUN_140064610`** (the Default mesh draw, K-0204), for each kept face set:
- `cull = table14473C9F8[mode][R+0xC]`, with table = {{0, 1, 2}, {0, 2, 1}, {0, 0, 0}};
- the result goes into bits 1..2 of the rasterizer key `[ctx+0x100]+0x10`, and dirty `+0x28 |= 1`;
- the mode comes from the pass context:

| Mode | Condition | Effect |
|---|---|---|
| **2** | `ctx+0x180` bit 9, or `ctx+0x184` = 2 | always none |
| **1** | `ctx+0x180` bit 7, or `ctx+0x184` = 1 | 1 and 2 swapped |
| **0** | otherwise | the face set's class |

- Other key bits: bit 0 = `ctx+0x180` bit 3; bit 3 = `record+0x18 & 0x18`; bit 7 = the
  multisample flag set by `FUN_14019C590`.
- `FUN_14019C590` → `FUN_1400235E0([144961B50], key)` →
  `[[144961B50]+0x4DE8]+0x60 + 8·(low byte | high 16 bits << 16)` (a map when
  `(key & 0x60) = 0x60`) → `FUN_141920CA0` (`RSSetState`).
- **Measured (section 65): 0 = NONE, 1 = FRONT, 2 = BACK.** Mode 1 swaps FRONT and BACK.
- The MTD does not take part. The pass does, through `ctx+0x180`/`+0x184`.

### 2. `FUN_140CFDE70`'s remaining part overrides (section 60)

**`+0xD0..+0xDC` (`mulCol`)**, when the short `desc+0x44` ≥ 1:
- controller `h = FUN_140E35EF0(id)` (lookup `FUN_140E36260`);
- `alpha = [h+8]+0x24` (1.0 without a controller);
- **`mulCol.w *= alpha`** (the xyz factors are 1.0);
- `group+0x87 = 1`;
- `group+0x84` = −1 or +1 from `FUN_140E35F50` (`[h+8]+0x2C`, 0..3, or 4 when absent) and
  from whether alpha < 1;
- the per-part countdown is `desc+0x40` (`FUN_140C48180`, bit 4).

**`+0xC0..+0xCF`** = `desc+0x00..+0x0F` (128 bits) ORed with bit 127 (`FUN_1405B0800`,
`FUN_1405278C0(0x7F)`). Dirty +0x181. INFERRED: the part's enable mask.

### 3. The FD4 frame steps

The step table is at `.rdata 1429BA580`: 0x20 bytes per entry, `{id, UTF-16 name, group}`. Full list:
`evidence/I-0067/queries/fd4/fd4-steps.txt`. The steps relevant here, in execution order (INFERRED: by id):

| Id | Step | Id | Step |
|---|---|---|---|
| 0x0E | `WorldChrMan_PrePhysics` | 0x2F | `HavokClothUpdate_Pre_ClothManager` |
| 0x0F–0x16 | `ChrIns_Prepare` … `ChrIns_PrePhysicsSafe` | 0x30–0x32 | `ScaleformStep`, `ScaleformCapture`, `ScaleformCaptureFE` |
| 0x1B | `TaskLineIdx_InGame_MoveMapStep` | 0x37 | `HavokClothUpdate_Post_ClothManager` |
| 0x23 | `HavokClothUpdate` | 0x38 | `HavokClothUpdate_Post_ClothModelIns` |
| 0x24 | `AdhocHavokClothPostUpdate` | **0x39** | **`HavokClothUpdate_Post_UpdateVertex`** |
| 0x26/0x27 | `LocationUpdate_PrePhysics` / `_PostCloth` | **0x3A** | **`HavokClothVertexUpdateFinishWait`** |
| 0x28/0x29 | `HavokWorldUpdate_Pre` / `_Post` | 0x3B–0x3D | `ChrIns_PostPhysics`, `WorldChrMan_PostPhysics` |
| 0x2A/0x2B | `ChrIns_PreCloth(Safe)` | 0x3F–0x41 | `ChrIns_UpdateDraw_Begin` / `_UpdateDraw` / `_End` |
| 0x2C–0x2E | `HavokClothUpdate_Pre_AddRemoveRigidBody` / `_ClothModelIns(Safe)` | 0x43 | `CameraStep` |
| | | **0x4F / 0x50** | **`UpdateLodWorld` / `UpdateLodIns`** (section 62, the LOD manager tasks) |
| | | 0x58, 0x5A | `DrawStep`, `GameSceneDraw` |

Group 63 marks the per-character parallel steps.

### 4. Character cloth: who computes the vertices, when, and what the renderer reads

**Creation** (section 57):
1. `FUN_140DD5FD0` (from `ChrModel` `FUN_140772270`, a ChrAsm part `FUN_140C37B70`, or
   `SprjModel` `FUN_140C4C490`).
2. `FUN_1423633A0`:
   - the dynamic mesh set `FUN_1423B7960`: one D3D11 dynamic VB and one CPU copy of stream 0
     (L1, 20 bytes) per mesh whose material has `+0x22`;
   - then `FUN_14239C0F0`/`FUN_14239C180` (Havok `hcl` cloth instance and binding).
   - Its upload system `[144964080]` (and `[144964078]`) comes from `FUN_142359F70`, created at
     game set-up (`FUN_140EDED40` → `FUN_1407D9C60` → `FUN_140DD41C0`).
   - That system also creates the **context pool** `[144964088]` (`FUN_1423A01F0`/`FUN_1423A0370`:
     7 contexts made with `FUN_1419261E0`/`FUN_141928580` on the GX device; INFERRED: deferred
     D3D11 contexts).

**Simulation (INFERRED from the step names; the registrations were not read):**
- the chr pre-cloth steps (0x2A/0x2B);
- `HavokClothUpdate_Pre_*` (0x2C–0x2F): rigid bodies, cloth instances, the manager;
- `HavokClothUpdate` (0x23): the Havok Cloth world step (`hclWorld`, job queue);
- `HavokClothUpdate_Post_*` (0x37/0x38).

**Vertex update (0x39 `HavokClothUpdate_Post_UpdateVertex`)** — INFERRED pairing with
`FUN_1423D5BC0`:
- for each cloth drawable, `FUN_1423D5BC0` (section 58) replicates the duplicated vertices inside the
  CPU copy (`FUN_14034F1B0`/`FUN_14034F090`; corrected in section 65);
- it takes a context from the pool (`FUN_1423A0130`; yields with `FUN_1417E8060` until one is
  free);
- it does `Map(WRITE_DISCARD)`, `memcpy` of the whole copy, then `Unmap`;
- it returns the context (`FUN_1423A00B0`).
- `FUN_1423D5D70` rebuilds the drawable bounds from the same copy.
- **0x3A `HavokClothVertexUpdateFinishWait`** waits for those jobs. INFERRED: it also hands the
  pool's command lists to the immediate context before the draw steps (0x58/0x5A).

**What the renderer reads:**
- At draw, `FUN_1423B6DB0` → `FUN_1423B74A0` builds a stream descriptor that **replaces
  Position, Normal and Tangent with the dynamic VB** (stride `[vb+0x68]`). The other semantics
  stay on the authored VB.
- `FUN_1423D6010` copies it into the drawable.
- The cloth VS (`…_Cloth…`, b2 `VC_isEnableCloth`) then uses **`VC_World` only**: the simulated
  positions are already in model space.

### 5. Native contract (generic, by family)

| Phase | Native |
|---|---|
| CPU simulation | Keep the engine's Havok Cloth steps 0x23..0x38. They are game-side CPU work |
| 0x39 | Replace steps 3–6 of `FUN_1423D5BC0` (context, `Map`, `memcpy`, `Unmap`). After step 2 (the CPU copy is current), publish an immutable fenced version of the stream-0 bytes keyed by `{set entry, vb}` |
| 0x3A | The native wait is the version publish. There are no deferred-context command lists to execute |
| Draw | For a mesh whose vb has an entry: stream 0 = the version current at preparation, stream 1 = the authored buffer. `VC_isEnableCloth` = 1 and `cbvModelParam.VC_World` from the pose owner |
| Bounds | `FUN_1423D5D70` keeps working on the CPU copy |
| Today | `NativeDynamicMeshBufferSet` leaves the set empty, so `desc+0x40` = 0 and the upload returns at step 1. The cloth draws the bind pose × `VC_World` (section 57) |

### 6. Enemies with special skinning (INFERRED where marked)

- **Human enemies and NPCs (`c0000`)** use `ChrAsm`: `SprjAsmModelDrawEntity`, one part per
  armour slot (section 60). Each part has its own draw descriptor, colour index, cloth
  (`FUN_140C37B70`) and fade controller. The pose is shared.
- **Monsters (`c1xxx`–`c6xxx`)** use one `ChrModel` → `SprjModelDrawEntity`. Their cloth comes
  from `FUN_140772270`. Weapons are inside the character FLVER or are separate equipment
  models. INFERRED: this was not enumerated per boss.
- **Multi-model bosses:** not enumerated. The engine side is the same for each model (one
  `ChrModel` per model, its own pose owner and cloth set). A multi-FLVER `chrbnd` needs one
  native pose and palette owner per model, not per character.

Evidence: `evidence/I-0067/queries/fd4/fd4-steps.txt`. Exports: `decompiled/DS3-1152-F-00064610`
(existing), `0019C590`, `000235E0`, `00E35EF0`, `00E35F30`, `00E35F50`, `005B0800`, `005278C0`,
`02359F70`, `023A01F0`, `023A0370`, `023A0130`, `023A00B0`, `0239C0F0`.

## 65. The cloth dynamic-buffer object, the exact tasks of steps 0x28–0x3A, and the cull-class measurement (STATIC; INFERRED where marked)

Follow-up to sections 57, 58 and 64. These are the pilot's questions on the object that
`FUN_1419254F0` stores in `set+0x18[i]` and `FUN_1423B74A0` stores at `desc+0x40`.

### 0. Corrections

- **Cull class (section 64).** The pilot measured that face-set class **2 is BACK**: with that
  value, floors and armour face the right way. By the mirror table, **0 is NONE and 1 is
  FRONT**. Section 64 inferred 1 = BACK, which was wrong; it is corrected in place.
- **`FUN_14034F1B0`/`FUN_14034F090` (sections 58 and 64)** do not convert simulation output. They
  **replicate duplicated vertices inside the CPU copy** (subsection 1.4). Corrected in place.

### 1. The dynamic-buffer object (GX buffer handle)

#### 1.1 Creation

`FUN_1423B7960(set, model, clothData, owner)` handles each mesh i admitted by subsection 1.5:

1. **GX buffer description** `d`:
   - `d[0]` = ByteWidth = `(vb+0x7C count × vb+0x68 stride + 3) & ~3`;
   - `d[1]` = usage 2 (DYNAMIC); `d[2]` = bind 1 (VERTEX); `d[3]` = CPU access 1 (WRITE);
   - `d[4]` = misc 0.
2. **`FUN_1419254F0(device = [[144961AB0]], &obj, d, init = 0, alloc = set[0], 0, 0)`**:
   - refuses (returns 8) when ByteWidth is 0, `misc & 0x87C5` is set, `d[2] & 0x40` is set, or
     a structured stride is invalid;
   - otherwise calls `FUN_141926720(&obj, 0, 0, dev, dev+0x23D0, dev+0x23E0, d, init, alloc, …)`.
3. **`FUN_141926720`** validates usage, bind and CPU flags, then:
   - allocator `A = FUN_141921D50(alloc, dev+0x23D0)`, which is `set[0]` when non-null;
   - **`FUN_14192B300(&h, dev, d, init, A, …)`** builds the 16-byte native handle `h`;
   - **`obj = FUN_141777A00(0x28, 8, A)`**, then **`FUN_141932410(obj, &h, A)`**;
   - it returns 9 when the allocation fails, after releasing `h` (`FUN_14192B480`).
4. Back in `FUN_1423B7960`:
   - `set+0x18[i] = obj`. The vector is indexed by mesh number and holds null for non-cloth meshes;
   - the CPU copy is `copy = set[0]->vt+0x48(ByteWidth)` (allocate). Then
     `memcpy(copy, vb+0x80, ByteWidth)`: the copy starts as the **authored vertex bytes**;
   - it pushes `{int mesh i, uint ByteWidth, void* copy}` (16 bytes) into `set+0x38`;
   - `set+0x58 += ByteWidth`.

#### 1.2 The 0x28-byte object

| Offset | Content |
|---|---|
| `+0x00` | vtable **`143A3A428`** (RTTI locator `143A3A420`). The base class vtable is `143A3A890` (`FUN_141934260`/`FUN_141934230`) |
| `+0x08` | allocator `A` (the set's `set[0]`, or the device default `dev+0x23D0`) |
| `+0x10` | int **reference count, 1 at creation** |
| `+0x18` | `ID3D11Buffer*` (COM), or 0 |
| `+0x20` | the "alt" CPU resource (below), or 0 |

**Slot 0 is the only virtual: `FUN_1419324A0(obj, flags)`, the scalar deleting destructor.**
- It resets the vtable, releases the handle (`FUN_14192B480(obj+0x18)`), and resets to the base
  vtable.
- With `flags & 1` it also frees through `thunk_FUN_141fd6718`, the global delete.
- **Every engine release uses flags 0, then frees through the allocator**:
  slot 0 called as `(obj, 0)`, then **`A->vt+0x68(A, obj)`** (free). Example asm: `1423B739A`
  `MOV RDX,R14 / MOV RCX,RSI / CALL [RAX+0x68]`.

**Reference count:**
- `FUN_1417A35D0` is an atomic increment and returns the old value.
- `FUN_1417A3610` is an atomic decrement and **returns the old value**.
- **A return of 1 means that was the last reference**, so the object is destroyed. All five
  release sites (`FUN_1423B7280`, `FUN_1423B74A0`, `FUN_140360450`, `FUN_1423D6010`,
  `FUN_1423D62B0`) test `== 1`.

**`FUN_14192B480(h)`** (release of the handle):
- when `h+0` (COM) is set, it calls `h+0`->vt+0x10 (`Release`);
- otherwise, when `h+8` (alt) is set, it frees alt with `[alt+0]`->vt+0x68.

**`FUN_14192B4D0(obj+0x18, out)`** returns the GX description:
- **COM**: `ID3D11Buffer::GetDesc` (vt+0x50), translated back to GX enums
  (`FUN_14192AF80`/`AFB0`/`AFC0`/`AFD0`): `out[0]` ByteWidth, `[1]` usage, `[2]` bind, `[3]` CPU,
  `[4]` misc, `[5]` stride.
- **alt**: it copies the seven dwords at `[alt+8]`.
- **`FUN_1423B74A0` stores `out[0]`, the ByteWidth, at `desc+0x58`** (asm `1423B76B9`
  `LEA RDX,[RSP+0x48]`, then `MOV EAX,[RSP+0x48]; MOV [RSI+0x58],EAX`).

**`FUN_1419281C0(obj+0x18)`** returns the resource kind:
- COM: `GetType` (vt+0x38), where 1 is a buffer;
- alt: the dword at `alt+0x18`.

#### 1.3 The engine already has a CPU-only buffer form: misc flag `0x4000`

When `d[4] & 0x4000`, **`FUN_14192B300` creates no D3D11 object**:
- `alt = A->vt+0x50(ByteWidth + 0x50, align 0x10)`;
- `h = {0, alt}`;
- alt's layout:

| Offset | Content |
|---|---|
| `alt+0x00` | allocator A |
| `alt+0x08` | → `alt+0x20` |
| `alt+0x10` | → `alt+0x40` |
| `alt+0x18` | kind 1 |
| `alt+0x1C` | 1 |
| `alt+0x20..+0x3B` | the GX description (7 dwords) |
| `alt+0x40` | `{→ alt+0x50, 0}` (initial data descriptor) |
| `alt+0x50` | ByteWidth bytes, zeroed, or copied from `init` |

`0x4000` is not in `FUN_1419254F0`'s refusal mask (`0x87C5`). INFERRED: creating the cloth
buffers with `d[4] = 0x4000` gives a fully engine-managed token: refcount, destructor, allocator
and `GetDesc` (alt path), with no D3D11 call. Only the GPU-facing users of `obj+0x18` (subsection
1.4) must then be replaced.

#### 1.4 Every reader of `set+0x18[i]` and `desc+0x40`

| Function | When | What it does with the object |
|---|---|---|
| `FUN_1423B7960` | set creation | creates it and stores it at `set+0x18[i]` (ref 1 = the set) |
| `FUN_1423B6DB0` → **`FUN_1423B74A0(desc, model, owner, mesh, faceSet, set+0x18[mesh], copy)`** | descriptor build (`FUN_1423B6F50` lists the (mesh, face set) pairs; the matching `set+0x38` entry gives the copy) | **`desc+0x40 = obj`**: releases the previous value (dec, destroy on 1), **increments**, sets `desc+0x48 = copy`, `+0x50 = vb+0x80` (authored bytes), `+0x58 = ByteWidth`, `+0x5C = vb+0x7C` (count), `+0x60..` the index data |
| `FUN_140360450(dst, src)` | descriptor copy into `drawable+0x120` (from `FUN_1423D6010`) | releases `dst+0x40`, stores `src+0x40`, **increments** |
| **`FUN_1423D6010`** | drawable creation | builds a stack descriptor, copies it (above), **releases the stack reference**. If `desc+0x40` ≠ 0, the drawable's streams 0..2 (`+0x10/+0x38/+0x50`) get **data pointer 0**, the count `desc+0x5C` and the format byte. Only the non-null test reads the object |
| **`FUN_1423D62B0`** | drawable refresh | builds a stack descriptor. If `+0x40` ≠ 0: streams 0..2 get **data = `desc+0x50`** (authored), count, format. Then releases the stack reference. Only the non-null test reads the object |
| **`FUN_1423D5BC0`** (drawable vt+0x48) | per simulated frame | step 1: `desc+0x40` ≠ 0. Then `FUN_141922B40(obj, 3, ctxType)` (validity: `FUN_141922830` reads usage, and mode 3 needs usage 2), **Map** `FUN_141921860` (`ctx` vt+0x70 with **`obj+0x18`**), `memcpy`, **Unmap** `FUN_141921940` (vt+0x78 with `obj+0x18`) |
| **`FUN_142362C70`** (cloth instance, draw callback, DATA reference) | GX draw, pass flag `ctx+0x180` bit 8 | when the instance's `+0x62` flag is set: **`IASetVertexBuffers`** (`FUN_1419204C0`, `ctx` vt+0x90) slot 0 with **`[[CI+0x38]+0x18][mesh]+0x18`**, stride `[param_3+0x10]+0x68`, offset 0 |
| `FUN_1423B7280` | set release (head of `FUN_1423B7DC0`) | frees every copy (`set[0]->vt+0x68(set[0], copy)`), then for each `set+0x18[i]`: dec, destroy on 1 (slot 0 with 0, then `obj+8`->vt+0x68). Clears both vectors |
| `FUN_1423B7DC0` | set destruction | `FUN_1423B7280`, then frees the vector storage `set+0x38` (allocator `set+0x50`) and `set+0x18` (allocator `set+0x30`) |
| `FUN_1423D5830` | drawable destructor | `FUN_1423D5790`/`FUN_1423D5710` release the drawable state. INFERRED: including `drawable+0x120`'s reference; the body of `FUN_1423D5710` was not decompiled (bad instruction data) |

So only three places touch the D3D11 side, all through `obj+0x18`:
- `FUN_141922B40`/`FUN_141922830` (usage, through `GetDesc` or alt);
- Map and Unmap in `FUN_1423D5BC0`;
- `IASetVertexBuffers` in `FUN_142362C70`, a GX draw callback that native drawing does not run
  (INFERRED).

Everything else is the reference count, the non-null test, and `GetDesc` for ByteWidth.

#### 1.5 Allocators of the helpers

| Helper | Role | Allocator |
|---|---|---|
| `FUN_14034EBA0(vec, &it, pos, n, &fill)` | `vector<obj*>` insert of n copies of `fill` (the resize of `set+0x18` to the mesh count) | `vec+0x18` = **`set+0x30`** |
| `FUN_14034EE20(vec, n)` | reserve, 16-byte elements | `vec+0x18` (the temporary list in `FUN_1423B7960` uses the arena `FUN_14177A330`) |
| `FUN_14034EEB0(vec)` | grow ×1.5, 16-byte elements | `vec+0x18` |
| `FUN_14034F7E0(vec, pos, first, last)` | range insert of `{mesh, bytes, copy}` entries into `set+0x38` | `vec+0x18` = **`set+0x50`** |

**`set[0]` (`param_1[0]`) is the allocator of the CPU copies and of the buffer objects**:
`vt+0x48` allocates and `vt+0x68` frees. The two vectors have their own allocators at
`set+0x30` and `set+0x50`.

Set layout:

| Offset | Content |
|---|---|
| `+0x00` | allocator |
| `+0x08` | model |
| `+0x10` | owner |
| `+0x18/+0x20/+0x28/+0x30` | `vector<obj*>` (by mesh) and its allocator |
| `+0x38/+0x40/+0x48/+0x50` | `vector<{mesh, bytes, copy}>` and its allocator |
| `+0x58` | total bytes |

#### 1.6 `FUN_14034F1B0` / `FUN_14034F090` (corrected)

- Signature `(desc, pairs, count)`.
- The enabled slots of the descriptor: `desc+0x10`, `+0x1C`, `+0x28`, `+0x34`. Each slot is
  `{+0 enabled, +2 size byte, +4 offset, +8 stride}`.
- **For each pair and each enabled slot:**
  `memcpy(copy + off + stride·dst, copy + off + stride·src, size)`, with `copy = desc+0x48`.
- **Pair formats:**
  - `FUN_14034F1B0`: u32 pairs, dst = high 16 bits, src = low 16 bits. Used when
    `[[desc+8]+0x28]+0x0C` = 0.
  - `FUN_14034F090`: u64 pairs, src = low 32 bits, dst = high 32 bits.
- The pairs are `[[desc+8]+0x28]+0x18 + 16·desc[0]` → `{+4 count, +8 data}`, per mesh.
- **They only propagate the simulated values from each source vertex to its duplicates**
  (UV-seam and split-normal copies). The simulated positions, normals and tangents are already
  in the copy.
- INFERRED: Havok writes them there. `FUN_14239C180` (instance binding) receives the set and
  hands it to the `hcl` set-up through a provider object on the stack (vtable `143D7B0C8`); the
  exact buffer registration was not followed.

#### 1.7 Admission of a mesh (`FUN_1423B7960`)

A mesh i is admitted when all of these hold:
1. `e = [model+0x1F8][mesh+0x00 (u16)]` exists and **`e+0x22` ≠ 0** (the per-material cloth
   flag of section 57).
2. **`FUN_1423C5070(clothData, materialName)`**:
   - `materialName` is the wide string at `[model+0x20] + 0x30·(mesh+0x16) + 0x20`, converted to
     narrow;
   - true when it equals one of the names `[x+0x10] & ~1` in the lists `[[clothData+0x38]+0x20]`
     (count `+0x28`) → `[y+0x28]` (count `+0x30`).
   - INFERRED: the `hcl` buffer definitions' mesh or material names.
3. **`FUN_1423B6C50(i, model+0x98, model+0x148, model+0x170)`**:
   - returns the first vertex buffer of mesh i (`mesh+0x2A` count, `+0x38` indices) whose
     layout (`model+0x170`, 0x18 bytes per entry; members 0x1C bytes) has a member with
     semantic 0 (POSITION) and index 0;
   - −1 when there is none (not admitted).
4. That vertex buffer's **`vb+0x80` (authored data) ≠ 0**.

#### 1.8 Native contract (generic, by family)

| Item | Contract |
|---|---|
| Token | Either the engine's own 0x28-byte object created with `d[4] = 0x4000` (alt, no D3D11), or a native object with the same layout: vt slot 0 = destructor taking flags (call `(obj, 0)`), `+0x08` = an allocator whose vt+0x68 frees, `+0x10` = refcount starting at 1, `+0x18` = 0, `+0x20` = alt `{alloc, →desc, →init, kind 1, 1, desc[7], …}`. `GetDesc` then answers ByteWidth from alt |
| Copies | Keep `FUN_1423B7960`'s allocation and authored initialisation through `set[0]`, so Havok's writes and `FUN_14034F1B0` act on the same bytes |
| Upload | Hook `FUN_1423D5BC0` after step 2 (the duplicate replication): skip `FUN_141922B40`, the pool context, Map and Unmap. Publish `desc+0x48` (`desc+0x58` bytes) as an immutable fenced version keyed by `desc+0x40`. Return `status = 0` |
| Draw | The native mesh draw binds that version as stream 0 when the cloth instance's `+0x62` flag is set (the flag `FUN_142362C70` tests). Otherwise it binds the authored stream |
| Release | Respect the refcount protocol: the destroy happens when the decrement returns 1 |

### 2. Steps 0x28–0x3A: the exact registrations and callbacks

**The cloth manager** (`FUN_1407D8440`, the object `FUN_1407D9C60` runs on):
- its task objects use vtable `1428329A0`;
- each task is `{vt, …, +0x18 owner, +0x20 callback}`, registered with `vt+0x20(task, step)`.

| Task | Step | Callback | What it does |
|---|---|---|---|
| `+0x0F8` | 0x28 `HavokWorldUpdate_Pre` | `UNK_1407D9C40` | — |
| `+0x120` | 0x29 `HavokWorldUpdate_Post` | `FUN_1407D9BD0` | world post-step (`+0x80` vt+0x38; `FUN_142354B00(+0x68)`) |
| `+0x148` | 0x2C `…Pre_AddRemoveRigidBody` | `FUN_1407D9B10` | applies the queued add and remove of cloth rigid bodies (`FUN_142357EA0`/`FUN_142357ED0`) on the upload system's `+0x10` |
| `+0x170` | 0x2F `…Pre_ClothManager` | `FUN_1407D9B50` | `FUN_142359A60(S, cloth+0x30)` (profiler 0xE): `S+0x60`, `S+0x18` (`FUN_1423A10C0`) and `S+0x10` steps |
| `+0x198` | 0x37 `…Post_ClothManager` | `FUN_1407D9A90` | `FUN_142359A00(S)` (profiler 0xF): `S+0x10` post step |
| **`+0x1C0`** | **0x39 `…Post_UpdateVertex`** | **`FUN_1407D9AD0`** | profiler 0x13: **`FUN_140DD8DB0(V)`** |
| **`+0x1E8`** | **0x3A `…VertexUpdateFinishWait`** | **`FUN_1407D9B90`** | profiler 0x14: **`FUN_140DD8CC0(V)`**; profiler 0x15: **`FUN_140DD8F70()`** |

Here:
- S = `[144964978]`, the cloth upload system of `FUN_142359F70`.
- The cloth system C = `[144794EB0]` (`FUN_140DD41C0`).
- **V = C+8, the vertex-update dispatcher** (`FUN_140DD85C0`, vtable `1429717B0`):
  - `V+0x08` = mode (**3 at construction**);
  - `V+0x10/+0x18` = the instance queue (count `+0x18`), lock `V+0x28`;
  - `V+0x60` = the list of worker records.
- **One worker** (`FUN_140DD9310`):
  - it owns the thread **`SprjClothVertexUpdateThread`** (`FUN_1417E8510`, stack 0x10000);
  - its run loop `FUN_140DD9540` pops instances (`FUN_140DD8AC0`) and runs `FUN_140DD6560` on
    each until the queue is empty, then signals its event `+0x20`.

**0x39 `FUN_140DD8DB0(V)`**, when `FUN_140D56DA0([144793348])` holds:

| Mode | Action |
|---|---|
| 1 | pop and run every queued instance inline (`FUN_140DD6560`) |
| 2 | when the queue is non-empty, wake every worker (`FUN_140DD9630`) and wait for them (`FUN_140DD9520`) |
| **3** | when the queue is non-empty, **wake the workers whose `+0x10` ≠ 0**. The others are woken at 0x3A |

**0x3A:**
- `FUN_140DD8CC0(V)` (mode 3): wake the workers with `+0x10` = 0, then **wait for every worker's
  event**.
- **`FUN_140DD8F70`** (when `S+0x58`, the context pool, exists): `FUN_1423A0810(pool)`, then,
  when `pool+0x70` = 0:
  - **`FinishCommandList`** on each pool context (`FUN_141930E80` → `FUN_141921C20`, vt+0x390,
    into `pool+0x18[i]`);
  - **`ExecuteCommandList(cmdlist, FALSE)`** on the immediate context
    (`[[144961AB0]+0x10` or `+0x70]+8`; `FUN_1419280D0` → `FUN_141921AF0`, vt+0x1D0).
  - This confirms section 64's inference: the uploads recorded on the pool's deferred
    contexts reach the GPU here.

**Per cloth instance** (`FUN_140DD56D0`, task vtable `1429710E0`; registered by `FUN_140DD5DF0`
and `FUN_140DD5E80`):

| Task | Step | Callback | What it does |
|---|---|---|---|
| `+0x118` | 0x2D `…Pre_ClothModelIns` | `FUN_140DD6BC0` | enable and reset transitions (`FUN_142362C00`), `FUN_142363090` (rigid-body add and remove per `+0x48`/`+0x49`) |
| `+0x148` | 0x2E `…Pre_ClothModelInsSafe` | `UNK_140DD6CA0` | — |
| `+0x178` | 0x38 `…Post_ClothModelIns` | `FUN_140DD6970` | **`FUN_140DD6690`: the instance simulation step** (dt > 0): `FUN_142362DA0` (`FUN_14239AA20`, the `hcl` instance step and bounds) or `FUN_142362F40` (with an animation source `FUN_140D18080`). Then **queue the instance on V** (`FUN_140DD4A20` → `FUN_140DD8C40`), or, when V's mode is 0, run `FUN_142362D80` inline |
| `+0x1A8` | 0x38 | `FUN_140DD6620` | the same without the simulation step (`FUN_140DD6800` first) |

`FUN_140DD6560` and the inline path both call **`FUN_142362D80(CI)`**:
- it **sets `CI+0x62 = 1`** when `CI+0x30` (the `hcl` instance) exists and `CI+0x60` (enabled)
  is set;
- that is the flag the draw callback `FUN_142362C70` tests before binding the dynamic buffer.

**Static gap:**
- The caller of the drawable slot `FUN_1423D5BC0` (vt+0x48 of drawable vtable `143D7DEF0`) was
  not found: there is no direct reference, and the `CALL [R+0x48]` sites scanned in
  `0x142350000..0x142400000`, `0x1412A0000..0x1412E0000`, `0x140340000..0x140380000` and
  `0x140200000..0x140240000` do not match.
- INFERRED: it runs between the 0x38 simulation and 0x3A, because 0x3A executes the pool's
  command lists and the pool is used only by `FUN_1423D5BC0`. `FUN_1423A0130` has no other
  caller.
- Runtime check: a breakpoint on `1423D5BC0` gives the step and the thread.

### 3. Other pilot findings recorded

- **Bloodied materials.** The `_Bloody` draws of NPCs and skinned objects were refused for want
  of t41 and t42 (`GXBloodyMaterialPatch`): the neutral decal policy applied only to the
  player. That is consistent with section 60's t41 analysis, applied to every family.
- **Palette padding.** Padding the palette to 256 rows overflowed on poses of 282 bones
  (`o302590`, section 64). The palette must be sized from the pose's bone count, not from a
  fixed 256.

Exports: `decompiled/DS3-1152-F-007D8440`, `007D9AD0`, `007D9B90`, `007D9A90`, `007D9B50`,
`007D9B10`, `007D9BD0`, `00DD8DB0`, `00DD8CC0`, `00DD8F70`, `00DD85C0`, `00DD9310`, `00DD9540`,
`00DD9630`, `00DD9520`, `00DD8AC0`, `00DD8B40`, `00DD8C40`, `00DD6560`, `00DD56D0`, `00DD6BC0`,
`00DD6970`, `00DD6620`, `00DD6690`, `00DD5DF0`, `00DD5E80`, `02362D80`, `02362C70`, `02362DA0`,
`023A0810`, `01926720`, `01932410`, `019324A0`, `01934260`, `0192B300` (existing), `019281C0`,
`01922830`, `01922B40`, `01921860`, `01921940`, `019204C0`, `023B7280`, `0034EBA0`, `0034EE20`,
`0034EEB0`, `0034F7E0`, `0034F1B0`, `0034F090`, `023B6C50`, `023C5070`, `023D62B0`, `00360450`.

## 66. The material cloth flag `e+0x22`, reaching a model's cloth instance, and the skinning and multi-model outliers of the character files (STATIC + game data; INFERRED where marked)

Pilot questions after section 65:
- Natively, every mesh is rejected by `FUN_1423B7960` on `e+0x22` = 0, with `e = [model+0x1F8][mesh+0]` (run 1185: 17 sets, no admitted mesh).
- How does a selected object reach its cloth instance and set?
- Which characters have skinning or model shapes outside the norm?

### 1. Who writes `e+0x22`, and from what

**Writer: `FUN_140140F90(e, gxMtd)`.** It is called by the material builder `FUN_140082220`
(resolver slot 0x28) right after `gxMtd = resolver->vt+0x40(name)`. It writes:

| Field | Value |
|---|---|
| `e+0x80` | `gxMtd`, with a reference at `gxMtd+8` (the previous one is released) |
| `e+0x18` | `FUN_140142130(e)`, the material class (the `142130` rule) |
| **`e+0x22`** | **`(gxMtd+0x88 >> 13) & 1`**, the cloth flag |
| `e+0x24` | `gxMtd+0x88`, with **bit 28 replaced** by `e+0x70 ? e+0x70->vt+0x38() & 1 : 0` |
| `e+0x21` | the byte `gxMtd+0x111` |
| `e+0x1C` | 2 when the cloth bit is set, otherwise 1 |

It ends with a tail call `e->vt+0x60(e, gxMtd)`. Everything before that is CPU work on the two
objects.

**`gxMtd`** is the 0x120-byte GX MTD object, built from the MTD file:
- `FUN_140D43BB0` (the Sprj MTD resource) calls `FUN_14002D8E0`, then
  **`FUN_14002DD60(gxMtd, mtdBytes, …, reader)`**;
- the reader is a stack object from `FUN_1400A7DD0` (vtable `143D48268`), bound to the shader
  repository `[144793348]+0x38`.

`FUN_14002DD60` then:
1. passes each MTD parameter to the reader (`reader->vt+8`) and collects the parameter objects
   (`+0x90`, `+0xB0`);
2. sets **`gxMtd+0x88 = reader->vt+0x28()[4]`**. The slot `FUN_1400A7DC0` returns `reader+0x2C`,
   so the value is the dword at **`reader+0x3C`** (0 at construction);
3. ORs in bits taken from the **shader name** (the spx, `gxMtd+0x48`):

| Bit | Rule |
|---|---|
| `0x4000000` | the name is in the 43-entry table at `1447371E0` (`GXFlver_SSSEmiIblGlow_Bloody`, …) |
| `0x8000000` | the name equals `GXFlver_SSSEmiIblGlow_Bloody_Cloth`, or is in the table at `144731BE0` |
| `0x400` | contains `DOL`, unless it contains `RNM` |
| `0x1000` (and `gxMtd+0x110`) | contains `GXFlver_Col` while the name is short |
| `0x1` | `GI` |
| `0x8000` | `Emi` |
| `0x10000` | `Glow` |
| `0x800000` / `0x1000000` / `0x2000000` | `MultiBlend4` / `3` / `2` |

**The cloth bit `0x2000` is not a name bit.** It comes from `reader+0x3C`, which only the
per-parameter slot `reader->vt+8` (`FUN_1400A7E40`) can write. That slot is **decrypted at run
time**: statically, Ghidra sees only an `INT1`. It cannot be read statically.

**Rule from the data (INFERRED; exact over the 512 MTDs of `AllMaterialBnd`; table
`evidence/I-0067/queries/chr/mtd-cloth-rule.tsv`):**

**cloth ⇔ `g_GX_MaterialType == 2` or `GXFT_Cloth == True`.**

- **Coverage:** 108 of the 110 MTDs whose spx contains `cloth`, and none of the 402 others.
  - 95 carry `g_GX_MaterialType = 2`: `SAT_Equip_cloth`, `SAT_HairIbl2_cloth` and every
    `_Bloody_Cloth`.
  - 13 carry `GXFT_Cloth = True`: the SSS cloths (`g_GX_MaterialType = 3` marks SSS) and the
    veils.
- **The two exceptions** are `C[Ghost]_Cloth` and `S[Ghost]_Cloth` (`GXFlver_SOPos_Cloth`).
  They carry neither parameter, so their bit is unknown.

**Native contract:**
- If the native MTD lease **is** the `gxMtd` built by `FUN_140D43BB0`/`FUN_14002DD60` (the
  original CPU path), `+0x88` already holds the bit. `BuildModelMaterial` then transcribes
  `FUN_140140F90`'s fields from the table above.
- Otherwise it applies the data rule.
- **Runtime check on the original:** `[e+0x80]+0x88 & 0x2000` for `C[ARSN]_e_Cloth` (expected
  1), `C[ARSN]_SSS_e_Cloth` (1), `C[ARSN]_e` (0) and `C[Ghost]_Cloth` (unknown).

**Downstream users of the flag:**
- **`FUN_140141F80(e)`** sizes the per-context parameter buffers (`e+0x88`, entries of 0x28
  at `e+0x98`):
  - count = max(`gxMtd+0x108` + 1, **9 when `e+0x22`**, `e+0x4E8` + 1), in 16-byte rows;
  - it allocates on the GX device (`[144961B50]+0x4DD0`), so it is not pure CPU.
- **`FUN_140140850`** (the material's per-draw apply) writes
  **`VC_isEnableCloth` (b2 offset 0x80) = `e+0x22` ∧ `ctx+0x180` bit 8 ∧ ¬`ctx+0x180` bit 10**.
  - This is the only writer of that constant on this path.
  - Bit 8 is also the cloth-pass flag tested by `FUN_142362C70` (section 65).

**Not established:**
- the original material vtable;
- its slots `vt+0x58` and `vt+0x60`;
- the writers of `e+0x60` and `e+0x210..+0x250`.

### 2. From a selected object to its cloth instance and set

Common chain:
- **I** is the cloth instance (`FUN_140DD56D0`, 0x1F0 bytes, created by `FUN_140DD4780`) and
  is filled by `FUN_140DD5FD0`;
- **CI = `I+0x38`** (`FUN_140DD5CF0`) is the `hcl` cloth object built by `FUN_1423633A0`;
- **set = `CI+0x38`**, created per CI, so **one set per instance**.

| Owner | Where I lives | Source |
|---|---|---|
| **ChrModel** (`FUN_140772270`) | **`ChrModel+0x1E0`**. On success, `FUN_140D07FC0(E)` is also called when `ChrModel+8` ≠ 0, and the matrices are copied (`FUN_140D33CD0`) | readers `FUN_140772160`, `1407721D0`, `772A10`, `772B80`, `772CB0` |
| **`SprjModelDrawEntity` E** | **`E+0xA70`** is read as I by `FUN_140D079C0`: `FUN_140DD5D10`/`FUN_140DD5CF0` give `I+0x38`, and the matrices come from `I+0x50`/`I+0x90`. **Its only static writer is the constructor `FUN_140D05C80` (0)**: no other store to `+0xA70` exists in `0x140000000..0x142600000`. INFERRED: set through a path not disassembled. Use `ChrModel+0x1E0` as the reliable source | `FUN_140D079C0`, `FUN_140D07710` |
| **ChrAsm part** (`FUN_140C37B70(asmModel, part, partRes)`) | **`[asmModel+0xD8] + 0x18 + 0x50·part`** (`FUN_140C487D0`, which also sets `asmModel+0xF8 |= 0x200000`); also the record `[asmModel+0x2A8 + 8·part]+0x18`. INFERRED: `[asmModel+0xD8]` is the descriptor array seen at `E+0xB00` (same stride 0x50), so **I = `desc_part+0x18`** | `FUN_140C37B70` |
| **SprjModel** (`FUN_140C4C490`) | **`SprjModel+0xE0`**, and **`item+0x750`** with `item = [SprjModel+0x10]` (the item constructor `FUN_140D3C4D0` sets `+0x750` to 0) | `FUN_140C4C490` |

**`set+0x08` (the model `FUN_1423B7960` reads)** = `CI+0x40` = **`FUN_140D23D20(src[2])` =
`[src[2]+0x68]+8`**:
- it is the **shared GX model of the FLVER resource**, not the per-instance draw model;
- for a ChrAsm part, `src = FUN_140C40D10(partRes)`: `src[0] = partRes+0x100`,
  `src[1] = partRes+0x120`, `src[2] = partRes+0x80` (slot i). Then `FUN_140DD3C70` takes the
  `hcl` data from `[partRes+0xE0]+0x80`;
- the per-instance buffers are in each instance's own set, so the index is
  **I → CI → set → `set+0x18[mesh]`**.

### 3. Skinning outliers of the character files

Data: the 165 `chr/cNNNN.chrbnd.dcx`:
- 125 named by the dictionary;
- 40 DLC files named by their path hash (`/chr/cNNNN.chrbnd.dcx`, FromSoftware hash ×37).

Tables in `evidence/I-0067/queries/chr/`: `chrscan.tsv`, `palscan.tsv`, `layscan.tsv` and
`maskscan.tsv`. The binaries stay in the scratchpad (D-0142).

| Finding | Detail |
|---|---|
| **One FLVER per `chrbnd`** | All 165. No character file holds several FLVERs |
| **`BLENDINDICES` are global skeleton indices** | Meshes with a 1-entry or empty palette reference bones up to 83 (`c1170`). Meshes with palettes reference bones below the skeleton size. The native palette is the **whole pose, in skeleton order, from row 0** (the skinned VS's `(BLENDINDICES + matricesData.x)·3` with offset 0) |
| **Skeleton sizes** | 79 of 165 have ≤ 128 bones and 86 have more. **Above 256: `c5180` 590, `c2100` 551, `c3090` 532, `c3141` 459, `c1321` 446, `c1320` 445, `c6010` 361, `c5270` 353, `c5110` 299, `c5030` 291**; `c5020` 256; `o302590` (object) 282. Size the palette from the pose (590 × 3 = 1770 rows ≤ 4096) |
| **16-bit bone indices** | 7 characters have `ShortBoneIndices` layouts because their indices exceed 255: **`c1320` (max 411), `c1321` (406), `c2100` (512), `c3090` (424), `c5180` (529), `c5270` (292), `c6010` (295)**. The native vertex input must accept `BLENDINDICES` as 4×u16 as well as 4×u8 (`Byte4B`) |
| **Large per-mesh palettes** | The FLVER palette lists (`mesh.BoneIndices`) reach 549 entries (`c5180`), with 27 meshes over 128. They do not change the indices (above) |

### 4. Multi-model and composite characters

- **There is no multi-FLVER character.** Variants, weapons, heads, capes and wings are **meshes
  of the single FLVER, selected by draw mask**.
  - 104 of 165 FLVERs carry `#NN#` mask indices in material names.
  - Examples:
    - `c1170`: `weapon1`/`weapon2`/`bow`/`shield1`/`shield2` on masks 11..15;
    - `c1280`–`c1283`: crossbow, sword, spear, shield;
    - `c6100`: 21 masked meshes (axe, bow, hammer, helmets, heads);
    - `c1210`/`c1211`: heads 01..04 with their hair cloth;
    - `c5180`: 19 masked meshes;
    - `c5220`–`c5227`: head variants.
  - Full list: `maskscan.tsv`.
  - Natively, each mesh's mask bit (`#NN#`) must be tested against the character's display
    mask, generically by family, before drawing. This is the same mechanism as the parts'
    enable mask, section 64.2.
- **Separate weapon models** exist only for **human NPCs and the player** (`c0000`, ChrAsm
  parts, section 60). Monsters' weapons are FLVER meshes (above).
- **Several characters for one boss:** each is its own `ChrIns` with its own `ChrModel`
  (`ChrModel+0x1E0` cloth, its own pose).
  - INFERRED: a `ChrIns` holds one `ChrModel`. `FUN_140772270` keeps one I per `ChrModel`, and
    no second-model field was found.
  - The native owner must therefore stay per `ChrModel` (pose, palette, cloth), never per boss.
- **Skinned objects** (`o302590`, section 64) follow the `SprjModel` path (`item+0x750`) with the
  same pose and palette rules.

Exports: `decompiled/DS3-1152-F-00140F90`, `0002DD60`, `00D43BB0`, `000A7DD0`, `00141F80`,
`00140850`, `00C37B70`, `00C487D0`, `00C4C490`, `00772270`, `00D23D20`, `00DD3C70`, `00C40D10`,
`00DD4780`, `00DD5CF0`, `00DD5D10`, `00D01580`, `007721D0`, `00772160`.

## 67. The original material class `GXFlverMaterial`: vtable, the GX item parser (vt+0x58), and the writers of `+0x60` and `+0x200..+0x278` (STATIC; INFERRED where marked)

Follow-up to section 66. The native `BuildModelMaterial` runs only `FUN_140082CC0`,
`FUN_1401B0AB0` and the ordinal; its material vtable has no slot 0x58.

### 1. The classes

**Material factory** `FUN_140082220` is slot 5 of both `NS_SPRJ::SprjFlverResourceFactory`
(vtable `14294E6B8`) and `GXSR::GXFlverResourceFactory` (`143D44118`). It receives the FLVER
header and the material index i; the material record is `header[2] + 0x20·i`.
- `+0x04`: wide name;
- `+0x08`/`+0x0C`: texture count and first texture;
- **`+0x14`: offset of the material's GX item list**.

**`GXFLV::GXFlverMaterial`**, vtable **`143D52878`**, 13 slots:

| Slot | Function | Slot | Function |
|---|---|---|---|
| 0 | `140140530` | 7 | `140140410` |
| 1 | `1401407D0` | 8 | `1401421C0` |
| 2 | `140140680` | 9 | `1401420E0` |
| 3 | `140140E90` | 10 | `140140420` |
| 4 | `140140320` | **11 (vt+0x58)** | **`FUN_140141070`**, the GX item parser |
| 5 | `1401403F0` | **12 (vt+0x60)** | **`FUN_140140460`**: an empty `RET`. The tail call of `FUN_140140F90` does nothing |
| 6 | `140140400` | | |

**The MTD reader** of section 66 is `GXSR::GXFlverMaterialParamSolver` (vtable `143D48268`,
9 slots). Its slot 1 (`FUN_1400A7E40`, the per-parameter handler that sets the cloth bit) is
runtime-decrypted: the static bytes decode as `INT1 / IN / INSD …`.
- The only typed parameter classes are `GXFlverMaterialParamBlendMode`, `AlphaRef`,
  `Specular`, `TessellationMode`, `Sss` and `WaterInteractionWaveSpeed`. None of them is
  cloth-specific.
- **To read the rule exactly**, capture the decrypted bytes of `1400A7E40` during a run
  (`Read-LoadedCode`, as in I-0066 `i66-native-loadedcode-1160`), then decompile them with
  `L:`.

### 2. The material build order in the original (`FUN_140082220`)

1. `mat = factory->vt+0x58()`;
2. `gxMtd = factory->vt+0x40(name)`. When it is null: `FUN_140140F10(mat, factory->vt+0x60(…))`;
3. **`FUN_140140F90(mat, gxMtd)`** (section 66);
4. **`mat->vt+0x58(mat, gxItems = header + record[+0x14] (or null), 0)` = `FUN_140141070`**;
5. **`FUN_140141F80(mat)`** (parameter buffers, on the GX device);
6. **`FUN_1401B0AB0(mat, name)`**: `mat+0x30` = name, **`mat+0x60` = ~CRC32(name bytes)**
   (table `14472BE30`);
7. `mat+0x64` = i (the ordinal).

### 3. `FUN_140141070`: what each GX item writes

The list is items `{u32 id, u32 unk, u32 size, data…}`, chained by `size`, ending at
`0x7FFFFFFF`. The ids are FourCCs `GXnn` (`0x3n3n5847`) and `GXMD`.

**Material fields:**
- **`mat+0x4E8`** is the highest constant row used, initialised to −1. Each item raises it to
  its row number; it then sizes the buffers in `FUN_140141F80`.
- **`mat+0x284`** is a 16-bit "texture slot" mask, one slot record per bit.

FourCC note: the id `0xAABB5847` is the bytes `47 58 BB AA`, so it reads `GX` + `BB` + `AA`. For
example, `0x30395847` is `GX90`.

| Item | Writes |
|---|---|
| `GX00` | `FUN_14022B7C0(mat+0xD0, item)` |
| `GX01` | `+0xF8` = d[3], `+0xDC` = 1, row ≥ 3 |
| `GX02` | `+0x110`, `+0x114`, `+0x120` = d[3..5]; `+0x118`, `+0x11C` = float d[6..7]; `+0xDD` = 1; row ≥ 5 |
| `GX03` | `+0x104` = d[3], `+0xDE` = 1, row ≥ 2 |
| **`GX04`** | **`+0x254` = d[3] & 1, `+0x250` = d[4]**, `+0x258`/`+0x25C` = (int)float d[5..6], `+0x260` = d[7], `+0x274` = d[8], `+0x278` = d[9], `+0x270` = d[10], `+0x26C` = d[11], `+0xDF` = 1 |
| `GX05` | `+0x108` = d[3], `+0xE0` = 1, row ≥ 1 |
| `GX06` | `+0x10C` = d[3], `+0xE1` = 1, row ≥ 10 |
| **`GX09`** | **`+0x200..+0x20C` = d[3..6]**, `+0xE3` = 1, row ≥ 6 |
| `GX12` | `+0x124`, `+0x128`, `+0x134` = d[3..5]; `+0x12C`, `+0x130` = float d[6..7]; `+0xE8` = 1; row ≥ 7 |
| `GX13` | `+0xF8..+0x100` = d[3..5], `+0xE9` = 1, row ≥ 12 |
| `GX17` | `+0x410` = d[3], `+0xF3` = 1, row ≥ 31 |
| `GX35` | `+0x3C8` = d[3], `+0xEB` = 1, row ≥ 9 |
| `GX36` | `+0x3D0..+0x3DC` = d[3..6], `+0xEC` = 1, row ≥ 13 |
| `GX37` | `+0x3E0..+0x3EC` = d[3..6], `+0xED` = 1, row ≥ 14 |
| `GX39` | `+0x3F0..+0x3FC` = d[3] ×4, `+0xE6` = 1, row ≥ 15 |
| `GX40` | `+0x400..+0x40C` = d[3] ×4, `+0xE7` = 1, row ≥ 16 |
| `GX80` | `+0x4A0` = d[3], `+0xF4` = 1, row ≥ 32 |
| **`GX90`** | **`+0x254` = 1, `+0x250` = 2**, `+0x258`/`+0x25C` = (int)float d[3..4], `+0x260` = d[5], `+0x274` = d[6], `+0x278` = d[7], `+0x270` = d[8], `+0x26C` = d[9], `+0xDF` = `+0xE4` = 1 |
| Slot records `GX20`, `GX21`, `GX22`, `GX23`, `GX24`, `GX25`, `GX26`, `GX27`, `GX28`, `GX29` | `+0x284 |= 1 / 2 / 4 / 8 / 0x10 / 0x20 / 0x40 / 0x80 / 0x200 / 0x400`, with `{type, d[3..4]}` at `+0x288` (0), `+0x29C` (7), `+0x2B0` (1), `+0x2C4` (8), `+0x2D8` (2), `+0x2EC` (9), `+0x300` (3), `+0x314` (10), `+0x33C` (4), `+0x350` (11) |
| Slot records `GX30`, `GX31`, `GX32`, `GX33`, `GX34`, `GX38` | `+0x284 |= 0x800 / 0x1000 / 0x2000 / 0x4000 / 0x8000 / 0x100`, at `+0x364` (5), `+0x378` (12), `+0x38C` (6), `+0x3A0` (13), `+0x3B4` (14), `+0x328` (16) |
| `GX07`, `GX08`, `GX14`, `GX15`, `GX16`, `GX81`, `GX82`, `GXMD` | helpers on the sub-object `mat+0xD0`: `FUN_14022C180`, `14022C420`, `14022BD90`, `14022BF70`, `14022C2B0`, `14022BB90`, `14022BC90`, `14022C4B0` (`GX81`, `GX82`, `GX14`, `GX15` also take `gxMtd`; `GXMD` needs `gxMtd` and a non-null third argument) |
**After the list (uses `gxMtd = mat+0x80`):**

| Condition | Effect |
|---|---|
| `gxMtd+0x88` bit 27, or the third argument's `[+0x18]+0x40` bit 1 | `+0xF7` = 1, row ≥ 38 |
| `gxMtd` has a parameter named as the string at `143D48320`, its vt+0x38 is true, and `+0xEF` = 0 | `FUN_14022BEC0(mat+0xD0, gxMtd)` |
| bit 11, and `+0xF1` = 0 | `FUN_14022C0D0(mat+0xD0)` |
| bit 9 | `+0xE5` = 1, row ≥ 11 |
| bit 0, and `+0xE0` = 0 | `+0x108` = 0, `+0xE0` = 1, row ≥ 1 |
| bit 26, or the third argument's flag bit 0 | `+0xF2` = 1, `+0x140..+0x19F` cleared, `+0x170..+0x188` = the constants at `143D7FEF0`, row ≥ 30 |

**`mat+0x210..+0x24F`** are the eight qwords `+0x140..+0x178` of the sub-object at `mat+0xD0`:
- they are zeroed by its constructor `FUN_14022B2C0` (`14022B4E6..14022B517`);
- INFERRED: the GX item helpers above fill them (pointers). They were not decompiled one by one.

**`+0x250`** comes from `GX90` (= 2) or `GX04` (= d[4]); `+0x254` from the same items.

**`mat+0x60`** = ~CRC32 of the material name (`FUN_1401B0AB0`), which the native already runs.

### 4. Native contract

| Item | Contract |
|---|---|
| Inputs of `FUN_140141070` | the material (with `+0x80 = gxMtd` set by `FUN_140140F90` first, and the sub-object at `+0xD0` constructed), the FLVER's GX item list for that material (`header + record[+0x14]`), and 0 |
| Purity | INFERRED: CPU only. The parser body only writes fields; the `14022…` helpers were not all read |
| Order | `FUN_140140F90` → `FUN_140141070` → `FUN_140141F80` (device) → `FUN_1401B0AB0` → ordinal |
| Calling it natively | It is not virtual-dispatched from the native object: call `FUN_140141070` directly on a native object whose layout matches `GXFlverMaterial` up to `+0x4EC`, or transcribe the item table above |

### 5. Skinning outliers, completed (section 66.3)

- The bone-index member types are `0x11` `Byte4B` (4 × u8) and **`0x18` `ShortBoneIndices`
  (4 × u16, 8 bytes)**.
- The native input layout must declare BLENDINDICES as R8G8B8A8_UINT for `0x11` and as
  **R16G16B16A16_UINT for `0x18`**. INFERRED: the GX layout table's DXGI value for `0x18` was
  not read.
- The seven characters with `0x18` reach index 529 (`c5180`, 590 bones). The palette must be
  sized from the pose bone count: 590 × 3 = 1770 rows for `c5180`.

Exports: `decompiled/DS3-1152-F-00141070`, `00140460`, `001B0AB0`, `0022B2C0`, `00082220`.

## 68. The GX item helpers of `GXFlverMaterial`, which of them touch the device, and the enemy families with particular materials (STATIC + game data; INFERRED where marked)

Follow-up to section 67. The helpers receive the sub-object `sub = mat+0xD0`, so a helper
offset `+X` is `mat+0xD0+X`. Their `sub+0x418` is `mat+0x4E8`, the highest constant row.

### 1. The helpers, one by one

| Item | Helper | Writes (as material offsets) | Device |
|---|---|---|---|
| `GX00` | `FUN_14022B7C0` | the flags `mat+0xD0` (bit 0) and `+0xD1` (bit 1, version 0x65), then `+0xD4` and `+0xD8`. On first use it hashes the `ExParam_*` names (`PhantomSpecColor`, `PhantomLightColor`, `WetDiffuseColor`, `WetReflectanceColor`, `WetParam`, `DamagedNormal_DamageRate`) into globals `14496234x` | no |
| `GX07` | `FUN_14022C180` | `mat+0x1A0..+0x1FC` (20 dwords; version ≥ 0x67 adds `+0x1F8/+0x1FC`, ≥ 0x68 adds `+0x1B0/+0x1B4`), `mat+0xE2` = 1, row ≥ 23 | no |
| `GX08` | `FUN_14022C420` | `mat+0x27C`, `+0x280` = d[3], d[4], clamped to (0, max]: defaults 0.1 and 0.5. `mat+0xE5` = 1, row ≥ 11 | no |
| `GX14` | `FUN_14022BD90` | `mat+0x420..+0x42C` (a colour: d[3..5] with version 0x65, otherwise the default at `143D7F6D0`), plus d[6]. `mat+0xF0` and `+0xEF` = 1, and `+0xEE` = 1 with version 0x65. **`mat+0x430`** = the MTD parameter named `g_DiffuseColor`. Row ≥ 24 | no |
| `GX15` | `FUN_14022BF70` | **`mat+0x440`** = the MTD parameter `g_SSSWidth`. **`mat+0x438` = a new SSS profile entry** from `FUN_140113D80([1449622F8], param)`: 0x40 bytes; colour = d[3..5]; width from the parameter; index at `+0x30`, appended to the global profile list. `mat+0xF1` = 1, row ≥ 25 | no (CPU registry, allocator `[144961B80]`) |
| `GX16` | `FUN_14022C2B0` | `mat+0x140`, `+0x141` (booleans), `mat+0x150..+0x19F` (5 × 16 bytes), `mat+0xF2` = 1, row ≥ 30 | no |
| `GX81` | `FUN_14022BB90` | `mat+0x4A4` (bit), `mat+0x4B0..+0x4BF`. **`mat+0x4D0`** = the MTD parameter `g_DamagedNormal1UvTiling`. `mat+0xF5` = 1, row ≥ 33 | no |
| `GX82` | `FUN_14022BC90` | the same for slot 2: `mat+0x4A5`, `+0x4C0..+0x4CF`, **`mat+0x4D8`** = `g_DamagedNormal2UvTiling`. `mat+0xF6` = 1, row ≥ 34 | no |
| `GXMD` | `FUN_14022C4B0` | `mat+0x4F0` = the third argument, index tables at `mat+0x448`/`+0x500`, and **a GPU constant buffer** at `mat+0x490` | **yes**, through `FUN_140147CA0` → `FUN_1419254F0` (`[144961AB0]`, bind 4). It runs **only when the third argument is non-null**. `FUN_140082220` passes 0, so **it is inert in the FLVER material build** |
| (after the list) | `FUN_14022BEC0` | when `gxMtd` has `g_DiffuseColor`: the default colour at `mat+0x420`, `mat+0x430` = the parameter, `mat+0xEF`/`+0xF0` | no |
| (after the list) | `FUN_14022C0D0` | when `gxMtd+0x88` bit 11 is set and the parameter `g_SSSWidth` exists: `mat+0x440` = the parameter, `mat+0x438` = 0, `mat+0xF1` = 1, row ≥ 25 | no |

**Consequences:**
- **`FUN_140141070` is CPU only in the material build path.** The only side effect outside the
  material is the global SSS profile list (`GX15`).
- **Correction to section 67:**
  - `mat+0x210..+0x24F` (`sub+0x140..+0x178`) are written by **none** of the helpers. They stay
    at the zero of `FUN_14022B2C0` through the build;
  - INFERRED: they are filled later, at texture binding or per draw. Their writer is still open.
- **The device is used one step earlier, by the MTD loader.** `FUN_14002DD60` creates the
  MTD's parameter buffer `gxMtd+0xF8` through the same `FUN_140147CA0` (one 16-byte row per
  parameter set, plus 1). A native MTD lease built by the original `FUN_14002DD60` has
  therefore already called `FUN_1419254F0`.

### 2. Enemy families with particular materials

Data: the materials of the 165 character FLVERs joined with `AllMaterialBnd`
(`evidence/I-0067/queries/chr/chrmat.tsv`: chr, MTD, spx, blend, material type, true `GXFT_`
flags, mesh count).
- 561 rows.
- Two MTDs are absent from `AllMaterialBnd`: `P[ARSN]_e_Decal` (`c1380`) and `P[ARSN]_Decal`
  (`c1390`).
- The default families are `GXFlver_ColDifSpcBumpIbl_Bloody` (280 uses) and `_Cloth` (101).

**The "probable native gate" column is INFERRED** from the parameters and the passes of
sections 44, 57 and 60.

| Family | spx (MTD) | Characters | Material traits | Probable native gate |
|---|---|---|---|---|
| **Ghost** | `GXFlver_ColDifSpcBumpEmiMulIblGhost` and `_Cloth` (`c1480_Ghost_FDP`) | `c1480`, `c1490` | blend 2 (alpha), depth write on, `g_AlphaRef` 1, diffuse and specular colours 0, **`g_GlowScale` (1,1,1,0.7), `g_GhostEdgeColor` (3,3,4), `g_GhostEdgeParam` (0.3,3)**. The cloth variant carries type 2 | class 2 → **forward pass 0x29** (the `57A20` row); the `…Ghost` pixel family and its edge constants (`cbMtdParam`); the emissive and glow output; cloth for `c1480` |
| **SSS** | `GXFlver_SSSIbl_Bloody` (37 uses), `_Cloth` (3), `GXFlver_SSSEmiIblGlow_Bloody` (3) | `c1130`, `c1320`, `c1321`, `c1340`, `c1400`, `c2020`, `c2070`, `c2090`, `c2120`, `c2130`–`c2132`, `c2140`, `c2170`, `c2180`, `c2280`, `c3080`, `c3140`, `c3141`, `c3190`, `c3200`, `c3250`, `c5020`, `c5180`, `c5210`, `c5250`, `c6020`, `c6120`, `c6121`, `c6200`, `c6201`, `c6240`, `c6280`, `c6310`, `c6331` | `g_GX_MaterialType` 3, `g_SSSWidth` 0.025, `g_SSSStrength` (1,1,1); the cloth variants use `GXFT_Cloth` | the **SSS profile** (`GX15` → `mat+0x438`, global list `[1449622F8]` and its index `+0x30`) and the skin SSS pass of `TONEMAP-SKIN`. Without the profile, the material has no SSS parameters |
| **Veil** | `Veill`, `Veill_cloth` (`c2170[Veil]`), `Veill_NoLight`, `Veill_NoLight_cloth` (`c5270[Veil]`) | `c2170`, `c2230` (`Veill`); `c5270`, `c6232` (`NoLight`) | blend 2 (`c6232`: 3), **depth write off**, **`g_TessellationMode` 2** (with `g_DisplacementEnable` 1 for `Veill`), `NoLight` = `GXFT_ForceForward` | forward pass; **hull and domain programmes** (tessellation); `NoLight` without lighting inputs |
| **Emissive and glow** | `GXFlver_ColDifSpcBumpEmiIblGlow_Bloody` (16), `_Cloth`, `GXFlver_ColDifSpcBumpEmiIbl_Bloody` (5), `C_ARSN_cinder_em` (9) and `_cloth`, `C_ARSN_em` (4), `GXFlver_SSSEmiIblGlow_Bloody` | `EmiIblGlow`: `c1200`, `c1201`, `c1283`, `c1310`, `c2132`, `c2150`, `c2230`, `c5160`, `c5270`, `c6060`, `c6200`, `c6201`, `c6210`, `c6211`, `c6250` (blend 2), `c6260`. `EmiIbl`: `c1090`, `c5110`, `c5280`, `c6250`, `c6260`. `cinder_em`: `c1450`, `c3040`, `c5020`, `c5250`, `c5260`, `c5280`, `c6020`, `c6260`. `C_ARSN_em`: `c3050`, `c3060`, `c5160`, `c6090` | `…Glow`: blend 2, depth write off, `g_GlowScale`, so forward. `cinder_em` / `C_ARSN_em`: opaque, `GXFT_Emissive`, `PaintDecal_Mat`, `ClothMatching`, plus SAT node parameters (`C_ARSN_cinder_em_snp_0_float_*`) | the emissive G-buffer channel (opaque) or forward plus glow (the `…Glow` families); for the SAT `cinder` families, the node constants from the MTD parameters |
| Translucent | `GXFlver_ColDifSpcBumpTrIbl_Bloody` and `_Cloth` | `c1220`, `c1280`–`c1282` (cloth), `c2170`, `c3140`, `c3141`, `c5221`–`c5227` | **corrected (I-0068 NPC-FAMILIES-STATIC 2.4): MTD blend 0, depth write on, `g_AlphaRef` 0 or 128** | **deferred: engine row 5, class 1 (4 for `_e_`), G-buffer; not forward** |
| Additive | `GXFlver_ColDifSpcBumpIbl` with `C[ARSN]_Add` (blend 4) | `c2030`, `c2040` | blend 4 | forward; the additive GX blend entry |
| Alpha (`_al`) | `GXFlver_ColDifSpcBumpIbl_Bloody` with `_al*` (blend 2) | `c1391`, `c5180`, `c6020`, `c6090`, `c6100`, `c6120`, `c6121`, `c6240`, `c6250`, `c6310`, `c6330`, `c6331` | blend 2 (hair cards, decals) | forward pass 0x29 (section 60's eyebrow case) |
| Tessellated displacement | `GXFlver_ColDifSpcBumpFlowDisplacement` (`c5110[FlowDisplacement]`) | `c1320`, `c1321`, `c3140`, `c3141` | **`g_TessellationMode` 2, `g_DisplacementEnable` 1** | hull and domain programmes |
| Vertex offset | `FlowmapVertexOffset` | `c1090`, `c1220`, `c2060`, `c5110` | `g_DisplacementEnable` 1, no tessellation | the vertex programme's flow-map offset (a VS texture read) |
| Fur | `Fur_FDP` (`C[Fur]`) | `c6030`, `c6040`, `c6050`, `c6100` | `g_AlphaRef` 50, `GXFT_Emissive` | alpha test, emissive |
| Crystal | `RecursiveCrystal` (`C[Crystal]`) | `c3100`, `c6020` | **`GXFT_Recursive`**, `GXFT_Emissive` | INFERRED: a recursive (refraction) pass |
| Eyes | `GXFlver_EyeIbl_Bloody` | `c1400`, `c6020`, `c6120`, `c6121` | opaque | its own pixel family |
| SAT multiblend | `SAT_MultiBlend2_BlendOpacity_Equip_Cm` and `_cloth` | `c1101`, `c1102`, `c1106`, `c6100`, `c6260` | `PaintDecal_Mat`, `ClothMatching`. The name bit `0x2000000` (`MultiBlend2`, section 66) | the SAT multi-blend family |
| Masked | `GXFlver_ColDifSpcBumpMulMaskIbl_Bloody` | `c5140` | opaque | its own pixel family |

Exports: `decompiled/DS3-1152-F-0022B7C0`, `0022C180`, `0022C420`, `0022BD90`, `0022BF70`,
`0022C2B0`, `0022BB90`, `0022BC90`, `0022C4B0`, `0022BEC0`, `0022C0D0`, `00147CA0`, `00113D80`.

## 69. Enemies and NPCs of the witness zones, their material families, and the smallest witness set (game data; INFERRED where marked)

Aim (D-0161): verify each family on few witnesses.

**Data:**
- The MSBs `m30_00` (High Wall), `m30_01`, `m31_00`, `m37_00`, `m40_00` (Cemetery of Ash and
  Firelink Shrine), `m45_00`, `m50_00`, `m51_00` and `m51_01`; MSB3 enemy parts, 1,416 rows.
- The families come from section 68's `chrmat.tsv`: the union over a character's materials.
  Draw-mask variants (section 66.4) may hide some meshes.
- Tables in `evidence/I-0067/queries/chr/`:
  - `msbenemy.tsv`: map, part, model, MSB position, NPC parameter, entity id;
  - `mapfam.tsv`: the first instance of each model per map, with its families.
- The MSB positions are in the same map space as the pilot's Argo position of section 63.

### 1. Models per zone (instance counts) and families other than plain opaque

**`m30_00` (High Wall):**
- 24 models:
  - c0000 (4), c0100 (2), c1000 (5), c1090 (5), c1100 (95), c1105 (7), c1130 (8), c1190 (4);
  - c1200 (3), c1280 (5), c1282 (1), c1290 (1), c1390 (1), c1442 (3), c2020 (2), c2021 (2);
  - c2090 (1), c2120 (1), c2150 (1), c2240 (1), c3141 (1), c3170 (1), c3250 (1), c5270 (1).
- Families:

| Family | Models |
|---|---|
| cloth | c1100, c1105, c1190, c1290, c1390, c1442, c2240, c3170 |
| SSS | c1130, c2020, c2120; c2090 and c3250 (with cloth) |
| emissive | c2150; c1200 (with cloth) |
| emissive + vertex offset | c1090 |
| translucent + cloth | c1280, c1282 |
| **veil** + emissive + cloth | **c5270** (353 bones, 16-bit indices) |
| tessellation + translucent + SSS + cloth | c3141, at Y = −300 (INFERRED: a dormant placement) |

**`m30_01`:**
- c1101 (24) and c1106: **SAT multi-blend** + cloth;
- c1281: translucent + cloth;
- c1310, c2150: emissive;
- c1090: emissive + vertex offset;
- c2120: SSS;
- c3080: SSS + cloth;
- c3141 (2): tessellation + translucent + SSS + cloth;
- plus c0000, c1000, c1100, c1290, c1442, c3160.

**`m31_00`:**
- c5180: **SSS + emissive + alpha** (590 bones);
- c1200: emissive + cloth;
- c1310, c2150, c3060: emissive;
- c2020, c2280: SSS;
- cloth: c1100, c1230, c1240 (57), c1260, c2160, c3020, c3090;
- plus c0000 (11), c0100, c1000, c1070, c2021, c2110.

**`m37_00`:**
- **c1480: ghost + cloth; c1490: ghost**;
- **c2030: additive** + cloth;
- **c2170: veil + translucent + SSS + cloth**;
- **c5140: masked multiply**;
- c5150: SAT cinder + cloth;
- translucent: c5225, c5226, c5227;
- SSS: c2120, c2131;
- emissive: c2150;
- cloth: c1230, c1350 (63), c1410, c2100, c2160, c2210, c2250, c2290, c3020, c3110, c5240.

**`m40_00`:**
- **c3100 (3): crystal (recursive)**, at the TestAnim start;
- **c1400: eyes + SSS + cloth**;
- **c1450: SAT cinder, emissive**;
- c5110 (2): emissive + vertex offset + cloth;
- SSS: c2020, c3190, c3200;
- emissive: c2150;
- cloth: c1250, c1430 (41), c2160, c2210;
- plus c0000 (23), c1000, c1300, c2021.

**`m45_00`:**
- **fur: c6030, c6040, c6050**;
- **c6100: SAT multi-blend + fur + alpha + cloth**;
- c6020: emissive + cinder + alpha + eyes + crystal + SSS;
- c6120, c6121: eyes + SSS + alpha + cloth;
- c6090: emissive + alpha;
- c6060: emissive + cloth;
- c6130: cinder;
- cloth: c6000 (53), c6010, c6070, c6081;
- c2150: emissive.

**`m50_00`:**
- **c6232: veil**;
- **c5020: SSS + emissive + cloth**;
- c6240: alpha + SSS + cloth;
- c6250: emissive + alpha;
- c1201, c1283: emissive + cloth;
- cloth: c2160, c6230, c6320.

**`m51_00` / `m51_01`:**
- c6260: emissive + SAT multi-blend + cloth;
- c6210, c6211: emissive;
- c6200, c6201: emissive + SSS + cloth;
- alpha + SSS: c6310, c6331;
- c6330: alpha + cloth;
- c6280: SSS + cloth;
- c2120: SSS;
- c2150: emissive;
- cloth: c3160, c6270, c6290, c6300, c6320.

### 2. Smallest witness set with the two saves available (Argo at the High Wall, TestAnim at the Cemetery of Ash)

Each witness is one camera stop. The position is the MSB placement of the nearest instance;
the distance from Argo is given for `m30_00`.

| # | Zone | Approximate position | Models in view | Families covered |
|---|---|---|---|---|
| W1 | `m30_00` | Argo's start (30.7, −24.1, 54.9) | c1100 `c1100_0424` (4 m), c0000 (6 m), c1000 | plain opaque, character cloth, ChrAsm parts |
| W2 | `m30_00` | about (10, −6, 95) | c1280 (26.3, −11.3, 96.1), c1105, c2021 and c2020 (−14.5, −4.3, 103.4), c1290 | **translucent + cloth**, **SSS** |
| W3 | `m30_00` | about (−48, 20, 74) | c2150 (−40.8, 16.6, 77.6), c1090 `c1090_0000` (−56.4, 23.8, 69.7) | **emissive**, **vertex offset** |
| W4 | `m30_00` | the arena at (28, −10, 158) | **c5270** (entity 3000899, a boss: it must be alive and its fight active), c3250 (27.6, −8.7, 163.6) | **veil** (tessellation, forward, no depth write), emissive glow, SSS + cloth, 16-bit indices and 353 bones |
| W5 | `m40_00` | TestAnim's start (−12.3, −54.9, 467.3) | **c3100** ×3 | **crystal (recursive)** |
| W6 | `m40_00` | Firelink Shrine, about (270, −53, 608) | **c1400** (267.9, −56, 607.4), **c1450** (277.9, −49.5, 614.1), c3190, c3200 | **eyes**, **SAT cinder emissive**, SSS |
| W7 | `m40_00` | (142.9, −68.9, 594.6) | **c5110** (entities 4000800 and 4000830, a boss) | emissive + vertex offset + cloth |

**Correction (I-0068 NPC-FAMILIES-STATIC, 2026-10-11):** W2's "translucent" family is a **deferred** G-buffer family (row 5, class 1/4, MTD blend 0, depth written), not a forward one; W2 therefore tests the G-buffer path plus the `FC_TranslucencyScale` constant, not the forward pass. See section 72.4.

**Coverage:**
- Seven witnesses, five in `m30_00` and two in `m40_00`, cover 11 families: plain, cloth,
  ChrAsm, translucent, SSS, emissive, vertex offset, veil, crystal, eyes, SAT cinder.
- W1–W3 and W5 need no fight. W4 and W7 are boss arenas.

**Not reachable with these saves** (one extra witness each, with a new save):

| Families | Witness | Model |
|---|---|---|
| Ghost, additive, masked multiply, the other veil (`c2170`) | `m37_00` | c1480/c1490, c2030, c5140, c2170 |
| SAT multi-blend, dependable tessellation | `m30_01` (reachable on foot from the High Wall after `c5270`) | c1101 at (45.8, 15.3, 105.9), c3141 |
| SSS + emissive + alpha, 590 bones and 16-bit indices | `m31_00` | c5180 at (176.7, −181.2, −957) |
| Fur | `m45_00` | c6030/c6040/c6050 |

INFERRED: the boss models (`c5270`, `c5110`, `c5180`) appear only during their encounter.
Their witness depends on the save's progression.

## 70. `mat+0x210..+0x24F` and the enemies' SSS (STATIC + game data; INFERRED where marked)

### 1. `mat+0x210..+0x24F` is an ExParam target block, never written on the material

- In `sub = mat+0xD0` coordinates the block is `sub+0x140..+0x17F`. The material's own copy
  is **only zeroed** (`FUN_14022B2C0`). Neither `FUN_140141070`, nor its helpers (section 68),
  nor the draw writes it.
- **At draw** (`FUN_140140850` → **`FUN_14022D620(sub, paramBuffer, exParams = ctx+0x1D8, …)`**):
  1. when the draw has a per-character ExParam set whose entry matches the material
     (`+0x80` set, or `+0x78` = the material's index), it copies the sub-object to the stack
     (`FUN_14022E0F0`);
  2. it blends the character's ExParam values into the copy with each entry's weight
     (`value[5]`). The ExParam ids or name hashes are those `GX00` computes (section 68).
  3. **When `sub+0x27` (`mat+0xF7`) is set** (`gxMtd+0x88` bit 27, or the third argument's
     flag; section 67), ids 0xF and 0x10 blend into `sub+0x140` and `+0x150`, and id 0x11
     into `+0x170`/`+0x174`.
  4. **`FUN_14022E930(sub, paramBuffer, src, ctx)`** fills the material constant buffer from
     `src`, which is the blended copy or the material itself. It reads `src+0x140..+0x17C`
     (`mat+0x210..+0x24C`) and the flag `src+0x178` (`mat+0x248`).
- So the values come **from the character's ExParams**, and the material contributes only
  zeros.
- **Native contract:** zero on the material. Per draw, blend the per-character ExParam entries
  0xF, 0x10 and 0x11 (lerp by weight) when `mat+0xF7` is set, then write the result into
  the constant rows. The gated families are the `0x8000000` names: the
  `GXFlver_SSSEmiIblGlow_Bloody` list of section 66.

### 2. The SSS of enemies

TONEMAP-SKIN already covered the SSS stage, its activation, profiles and kernel. The
character side is now [TONEMAP-SKIN section 18](TONEMAP-SKIN.md#18-the-sss-of-enemies-and-npcs-materials-per-material-profiles-what-the-native-needs-game-data--static-inferred-where-marked).

**Key fact:** all 77 SSS materials of the 35 SSS characters carry `GX15`, so each needs its
own linked instance profile.
- 23 of these materials have a strength below 1 (0.1–0.4).
- **Offset note:** TONEMAP-SKIN section 8's "material" offsets (`+0x21`, `+0x368`, `+0x370`,
  `+0x418`) are sub-object offsets: `mat+0xF1`, `+0x438`, `+0x440`, `+0x4E8`, as in section 68.

Exports: `decompiled/DS3-1152-F-0022D620`, `0022E930`, `0022E0F0`.

## 71. The screen-space SSS stage for enemies, and the Fur and Eye families (STATIC + game data + pilot source; INFERRED where marked)

### 1. The SSS stage: what is specified, what is implemented, what the enemies still need

**Specification already complete:**
- TONEMAP-SKIN covers the stage:
  - section 3: objects, the per-frame `cbSSS`, recording, the two draws;
  - section 4: depth-stencil, blend and rasterizer states, decoded;
  - section 7: the eight activation conditions;
  - section 8: profiles, `FC_SSS_ID` and the kernel;
  - section 18: the enemy materials.
- **The pilot branch implements it** (`claude/npc-enemies` `9e6278ba`, Native1137/1093;
  read-only reading):
  - `scene_pass.cpp` runs **BlurX then BlurY between `GXLightCmbAlbedo` and
    `GXLightComposite`** (lines 1120–1130), with `b1` = `cbSSS` (0x120 per profile, at most
    0xE3) and `b8` = `cbSceneParam`;
  - the inputs are `t1` depth, `t3` GBuffer1 (`g_SSSStrengthTexture`) and `t36` = irradiance
    (BlurX) or the temporary (BlurY), with the temporary the size of the irradiance target in
    R16G16B16A16_FLOAT;
  - the depth-stencil state is depth off, stencil read mask 4, EQUAL, reference 4;
  - `game_bootstrap.cpp` builds the profiles, including the GX15-linked instance (`2493–2560`),
    and `FC_SSS_ID` (`3585–3599`).
- TONEMAP-SKIN section 2's "native consequence" (no SSS stage natively) is therefore out of
  date.

**For the pass order (I-0068 `NATIVE-PASS-ORDER`, to report, not edited here):**
- Row 8 lists "…; SSS; CmbAlbedo; Composite".
- The original order (TONEMAP-SKIN section 3, K-0407 capture) and the native implementation
  are **CmbAlbedo → SSS (BlurX, BlurY) → Composite**.
- The SSS slot is after `GXLightCmbAlbedo` and before `GXLightComposite`.

**What enemies still depend on: the stencil mark.**
- The blur runs only on stencil bit 4 (value 4).
- The G-buffer draw sets it when **material `+0x24` bit 0x800** holds (`FUN_140140850`;
  native `game_bootstrap.cpp` around line 4283: `stencil |= 4` when `pose_policy->flags & 0x800`).
- `+0x24` is `gxMtd+0x88` copied by `FUN_140140F90` (section 66). **Bit 11 (0x800)** is the
  SSS bit; `FUN_140141070` uses it for the default SSS width (`FUN_14022C0D0`, section 68).
- Like the cloth bit 13, it is set by the runtime-decrypted MTD parameter handler.
- **Data rule (exact over the 512 MTDs): SSS ⇔ `g_GX_MaterialType == 3`.** The 38 MTDs with
  type 3 are exactly the 38 whose spx contains `SSS`, and none of the others.
  - So: bit 0x800 ⇔ type 3, as bit 0x2000 ⇔ type 2 or `GXFT_Cloth` (section 66).
  - INFERRED: the decrypted handler was not read.
- **Native check:**
  1. for an enemy SSS material (for example `c2020` `#00# body`, `C[ARSN]_SSS`), the native
     `+0x24` must have 0x800;
  2. otherwise the stencil stays 9, the blur skips its pixels, and only `FC_SSS_ID` is right.

### 2. Fur (`Fur_FDP`, `C[Fur]`): `c6030`, `c6040`, `c6050`, `c6100` (all `m45_00`)

- **A SAT bundle** (`shaderbdle/Fur_FDP.shaderbdle`; there is also `Fur_FDP_cloth`). Its
  programmes are the usual set: `Gbuf`, `Gbuf_[A]`, `Fwd`, `FwdSdw`, `FwdSSdw` (and
  `Phantom_*`), `Velo`, `Dpt`, `DptA`, `Pnt`, `PntCol`, with `_Skin` vertex variants.
  **There are no shell or fin passes.**
- **MTD:**
  - `g_AlphaRef` 50, so the alpha-tested `_[A]` pixel programmes are used (`Gbuf_[A]`,
    `DptA`);
  - blend 0, depth write on;
  - `GXFT_Emissive`, `GXFT_PaintDecal_Mat`, `GXFT_ValidWet`;
  - `g_GlowScale` (1, 1, 1, 1).
- **G-buffer pixel programme** (`Fur_FDP_Gbuf.ppo`, reflected):
  - **six outputs**, `SV_Target0..5`: GB5, the emissive target, is written;
  - it reads `SV_IsFrontFace`;
  - constant buffers `cbMtdParam` (`FC_DifMapMulCol` @0, `FC_BloodAmount` @96,
    `FC_GlowScale` @144), `cbMatDynParam`, `cbTextureTilingScale`, `cbInstanceData`.
- **Textures by SAT node name:**

| Slot | Name | FLVER texture type |
|---|---|---|
| t1 | `Fur_FDP_snp_0_Texture2D_4` | `…_Texture2D_4_NormalMap_1` |
| t2 | `Texture2D_3` | `…_3_NormalMap_0` |
| t3 | `Texture2D_2` | `…_2_AlbedoMap_1` |
| t4 | `Texture2D_1` | `…_1_AlbedoMap_0` |
| t5 | `Texture2D_0` | `…_0_ReflectanceMap_0` |

  - The samplers are `s0` `SS_DiffuseMap` and `s8` `SS_WrapLinear`.
  - The decals are at t41/t42.
- The skinned vertex programme is the standard one (`b5` palette, `b4` instance, `b8` scene,
  `b11` scroll).
- **Probable native gate (INFERRED):**
  1. the family key `Fur_FDP` admitted;
  2. **SAT texture binding by node name** (`…_snp_Texture2D_N_*` → `tN+1`, INFERRED: the same mechanism
     as the other SAT bundles);
  3. the `_[A]` programme chosen from `g_AlphaRef` ≠ 0;
  4. the six-target G-buffer state (GB5) for the glow;
  5. two-sided handling through `SV_IsFrontFace`.

### 3. Eyes (`GXFlver_EyeIbl_Bloody`, `C[Eye]`): `c1400` (`m40_00`), `c6020`, `c6120`, `c6121` (`m45_00`)

- **A GXFlver family** in `gxflvershader.shaderbnd`, with the usual programmes (`GBuf`,
  `Fwd`, `FwdSdw`, `Dpt`, `DptA`, `Velo`, and `_Skin`, `_Ins`, `@[cl]` variants).
- **G-buffer pixel programme** (`GXFlver_EyeIbl_Bloody_Gbuf.ppo`, reflected):
  - **five outputs**, `SV_Target0..4`: no GB5;
  - `SV_IsFrontFace`;
  - textures `g_DiffuseTexture` t0, `g_SpecularTexture` t1, `g_BumpmapTexture` t3, decals
    t41/t42;
  - `cbMtdParam` (`FC_DifMapMulCol`, `FC_BloodAmount`).
  - **The same interface as the default `GXFlver_ColDifSpcBumpIbl_Bloody` G-buffer
    programme**; only the pixel arithmetic differs.
- MTD textures: `g_DiffuseTexture`, `g_SpecularTexture`, `g_ShininessTexture`,
  `g_BumpmapTexture`, `g_BloodMaskTexture`, `g_HighLightTexture`. The last one is not read
  by the G-buffer programme. INFERRED: forward only.
- MTD parameters: opaque, blend 0, alpha ref 0.
- **Probable native gate (INFERRED):** the family key `GXFlver_EyeIbl_Bloody` admitted with
  the default G-buffer bindings. No new state.
- The witness is W6 of section 69 (`c1400` at Firelink Shrine), which also covers SSS:
  `c1400` has SSS skin and eyes.

## 72. Tessellated, translucent, additive, ghost and flow families of the enemies, and the static facts on the cloth writer (STATIC + game data; INFERRED where marked)

Shader data: the bundles of `shaderbdle.shaderbdlebnd` (SAT families) and
`gxflvershader.shaderbnd` (GXFlver families), reflected with `Get-DxbcReflection.ps1`.
Per-material GX items come from the 165 character FLVERs (`chrtess.tsv`).

### 0. The cloth writer (pilot question, before the families)

- **The cloth simulation runs between steps 0x2F and 0x37, not at 0x38.**
  - **Step 0x2F** (`FUN_1407D9B50`) runs only when `[144794EB0]+0x48` = 0. It calls
    `FUN_142359A60(S = [144964978])`:
    - `S+0x60` → `FUN_1423A0B40`: for each registered cloth instance (`FUN_1423633A0` →
      `FUN_1423A0990`), when `CI+0x60` ≠ `CI+0x61` it adds `CI+0x30` to the hcl world
      `S+0x10` (`FUN_142358290`), or removes it (`FUN_1423583F0`);
    - `S+0x18` → `FUN_1423A10C0`;
    - `S+0x10` → `FUN_142358640`: the world step with dt (stored at `+0x10C`); jobs are
      dispatched when `+0x30` is set (`+0x109` = 1).
  - **Step 0x37** (`FUN_1407D9A90`) → `FUN_142359A00` → `FUN_142358570`: waits for the jobs.
  - **Step 0x38** only computes bounds and copies matrices (`FUN_142362DA0` →
    `FUN_14239AA20`), then sets `CI+0x62`.
- **The cloth display buffers are Havok buffers built by a FromSoftware factory:**
  - `FUN_14239C180` hands Havok (`FUN_14202B130`, a switch on the hcl buffer definition type)
    a factory with vtable `143D7D520`;
  - **slot +0x20** (`FUN_1423C9D80`) creates the dynamic display buffer (vtable `143D7DEF0`,
    0x130 bytes, Havok base `FUN_1412086D0`). It calls `FUN_1423D6010`: the streams
    `+0x10`/`+0x38`/`+0x50` get **ptr 0**, and the descriptor goes to `+0x120`;
  - **slot +0x28** (`FUN_1423C9C60` → `FUN_1423D62B0`) creates the static buffers, with ptr =
    the authored bytes;
  - the buffer's own virtuals are `+0x40` = `FUN_1423D5D70` (bounds) and `+0x48` =
    `FUN_1423D5BC0` (commit/upload).
- **Static gap:**
  - no static code writes `drawable+0x10` except `FUN_1423D6010`/`FUN_1423D62B0`;
  - the neighbouring FromSoftware cloth-manager class (vtable `143D7B0D0`) has slots +0x80
    (`14239EBC0`) and +0xC8 (`1423A1230`) that are runtime-decrypted;
  - INFERRED: the vertex writer runs in the hcl jobs between 0x2F and 0x37, through that
    encrypted code.
- **Runtime checks given to the pilot:**
  1. a write breakpoint on the copy;
  2. `S+0x10` dt and flags at 0x2F and 0x37;
  3. `CI+0x60`/`+0x61`;
  4. the drawable's stream pointers after 0x37 in both modes.

### 1. Tessellation: how a material selects its programme

- The programme-set loader `FUN_1400AA630` loads, per pass, a non-tessellated variant and
  four tessellated ones. The suffix table is `14473CA90` = `_FT`, `_FT_D`, `_PN`, `_PN_D`.
  Each variant has a vertex, a hull (`.hpo`) and a domain (`.dpo`) programme.
- **The variant comes from the FLVER material's GX items, not from the MTD**
  (`GXFlverMaterial` slot 2 `FUN_140140680`):

| `mat+0x250` (mode) | `mat+0x254` (displacement) | Variant |
|---|---|---|
| 2 | 0 / 1 | **3 `_PN` / 4 `_PN_D`** |
| 1 | 0 / 1 | **1 `_FT` / 2 `_FT_D`** |
| 0 | — | 0, none |

  - Set by `GX04` (`+0x254` = d[3] & 1, `+0x250` = d[4]) or `GX90` (= 2, 1), section 67.
  - The MTD's `g_TessellationMode`/`g_DisplacementEnable` do not decide. For example, `c2170`'s
    veil has MTD mode 2 but no `GX04` mode, so it is not tessellated.
- **It is forced to 0** for pass kinds `ctx+0x1C` = 1 or 0xB, when `ctx+0x18C` bit 4 is set,
  or when `ctx+0x18` ≠ 0. The result is stored at `ctx+0x20`.
- The material's shader set binds HS and DS when the variant has them (`FUN_140142430`: VS,
  PS, HS `FUN_1419208A0`, DS `FUN_141920A50`, GS).
- **Tessellated enemy materials** (data, `chrtess.tsv`):

| Character | Materials | Variant |
|---|---|---|
| **`c5270`** | `veil`, `veil_cloth`, `veil2_cloth`, `Waist_cloth`, `Shoulder_cloth` (`Veill_NoLight[_cloth]`); `Line_cloth` (default `_Bloody_Cloth`) | mode 2, displacement 0: **`_PN`** |
| `c2230` | `whitehood` (`Veill`, `Veill_cloth`) | mode 2, displacement 0: `_PN` |
| `c6060` | `Hair_cloth` (default `_Bloody_Cloth`) | `_PN` |
| `c3140`, `c3141` | `GXFlver_ColDifSpcBumpFlowDisplacement` | mode 1, displacement 1: **`_FT_D`** |

  Every other material of the 165 FLVERs has `GX04` mode 0.
- **Interface of the tessellated stages** (reflected):
  - **VS** (`*_Skin_PN*.vpo`): no `cbSceneParam`. It outputs the skinned position before
    projection (`b4` instance, `b5` palette, `b11` scroll; the cloth variants add `b2`
    `VC_isEnableCloth` and `b7` `VC_World`).
  - **HS:** `cbTessScene` (`b5`; `g_f4GUIParams2` @96) and **`cbPNTriangles`** (`b6`;
    `g_f4TessFactors` @0).
  - **DS:** `cbTessScene.g_f4x4ViewProjection` @0 (`b5`), `cbSceneParam` (`b8`). The `_D`
    variants add **`g_DisplacementTexture` t5** with `SS_DisplacementMap` s4, plus
    `cbvModelParam` (`b7`) and `cbTextureTilingScale`.
  - The producer of `cbTessScene` and `cbPNTriangles` was not identified (the names exist only
    in the shader reflection). Open.

### 2. Veil (`c5270` first: W4 of section 69)

| Item | `Veill_NoLight[_cloth]` (`c5270`, `c6232`) | `Veill[_cloth]` (`c2170`, `c2230`) |
|---|---|---|
| MTD | blend 2 (`c6232`: 3), depth write off, `GXFT_ForceForward` | blend 2, depth write off |
| Pass | forward (`Fwd`, `FwdSdw`, `FwdSSdw`) | forward |
| PS inputs | `Veill_NoLight_snp_0_Texture2D_2` t2 (s0 `SS_DiffuseMap`); `cbMtdParam.FC_DifMapMulCol`; `cbSceneParam` (`InvViewProj`, `InvView`); `cbFog`. **No lighting** | `Veill_snp_0_Texture2D_2` t2 and `_1` t3; `cbLight` (directional lights 0..3), light lists t59..t62, `g_SSAOTexture` t33; `cbFog`; `cbSceneParam` |
| Tessellation | `c5270`: `_PN` (section 1) | `c2230`: `_PN`; `c2170`: none |
| Cloth | the `_cloth` VS reads `VC_isEnableCloth` and `VC_World` | same |

**Native gate (INFERRED):**
1. a SAT family with texture binding by `snp` name;
2. forward pass with blend 2 (or 3) and depth write off;
3. for `c5270`: **HS and DS** with `cbTessScene`/`cbPNTriangles`, or the non-tessellated
   variant as a fallback (`_PN` only rounds the silhouette; the vertices are the same);
4. cloth on the `_cloth` meshes.

### 3. Ghost (`c1480`, `c1490`; `m37_00`)

- Family `GXFlver_ColDifSpcBumpEmiMulIblGhost` (`c1480_Ghost_FDP`), and `_Cloth` for `c1480`.
- **MTD:** blend 2, depth write on, `g_AlphaRef` 1, `g_DiffuseMapColor` and
  `g_SpecularMapColor` 0, `g_GlowScale` (1, 1, 1, 0.7).
- **Forward PS** (class 2, pass 0x29):
  - textures `g_DiffuseTexture` t0, `g_SpecularTexture` t1, `g_BumpmapTexture` t3,
    `g_EmissiveTexture` t6, and a second layer `…2` at t7, t8, t10, t13;
  - IBL t30/t31, SSAO t33, **`g_RLRTexture` t44**, light lists t59..t62;
  - `cbMtdParam` (`FC_DifMapMulCol`, `FC_SSAOParam`, `FC_GlowScale`),
    `cbPerFrame.FC_GlobalSpecularMask`, `cbLight`, `cbFog`.
- **`g_GhostEdgeColor`/`g_GhostEdgeParam` are declared in `cbMtdParam` but not used** by the
  `Fwd`/`Gbuf` programmes of the plain and `_Cloth` variants (reflection "used" flags). So no
  edge term is needed for these programmes.
- **Native gate (INFERRED):** the forward family with the two-layer texture set and the RLR
  texture t44, and the emissive glow scaled by `GlowScale.w` 0.7.

### 4. Translucent (`GXFlver_ColDifSpcBumpTrIbl_Bloody[_Cloth]`)

- Characters: `c1220`, `c1280`–`c1282` (cloth), `c2170`, `c3140`, `c3141`, `c5221`–`c5227`.
- **Correction (I-0068 NPC-FAMILIES-STATIC 2.4 and 3.1, from the MTD data and the executable's class rule):**
  for every character MTD of the family `g_BlendMode` is **0**, `g_GX_bDisableDepthWrite` is False and
  `g_AlphaRef` is 0 or 128. The family is **deferred**, not forward: engine row 5, material class 1 (4 for
  `_e_`), G-buffer programmes `Gbuf` / `Gbuf_[A]`; "Tr" is a thin-surface translucency term in the lighting,
  not alpha blending. The earlier text here (MTD blend 2, depth write off, `g_AlphaRef` 1, forward pass 0x29,
  class 2, "the same row as the eyebrows") is **withdrawn**. Only the `_al_tr` MTDs (hair cards, map foliage) have
  blend 2.
- The cloth variant has `_PN` programmes too, but no enemy material selects them.
- **Native gate:** the ordinary G-buffer rows 1 and 4; `FC_TranslucencyScale` is not written natively (zero),
  see NPC-FAMILIES-STATIC 3.1 for the live read that remains.

### 5. Additive (`GXFlver_ColDifSpcBumpIbl`, `C[ARSN]_Add`, blend 4)

- Characters: `c2030`, `c2040` (and `_add_cloth_decal`).
- **Forward PS:** the default interface, without blood.
- **Native gate:** forward with the **additive** GX blend entry for material blend 4 (the
  `14473E5C0` table of section 60). No other difference.

### 6. `GXFlver_ColDifSpcBumpFlowDisplacement` (`c3140`, `c3141`)

- MTD: opaque; `g_TessellationMode` 2 and `g_DisplacementEnable` 1. Its **GX04 gives `_FT_D`**.
- **G-buffer PS:** t0, t1, `g_ShininessTexture` t2, t3 and **`g_FlowTexture` t17**, plus
  `cbPerFrame` (time).
- **DS (`_FT_D`):** `g_DisplacementTexture` t5 / s4.
- **Native gate:** HS and DS (flat tessellation plus displacement), the flow texture t17 and the
  per-frame time. **`c3141` in `m30_00` sits at Y = −300** (section 69); its witness is in
  `m30_01`.

### 7. `FlowmapVertexOffset` (`c1090`, `c1220`, `c2060`, `c5110`)

- A SAT bundle **without tessellation**.
- **G-buffer skinned VS:** reads **`FlowmapVertexOffset_snp_0_Texture2D_5` (t2, s8
  `SS_WrapLinear`)**, plus `cbMatDynParam`, `cbPerFrame`, `cbSceneParam`, the palette and the
  scroll. It is a **vertex-texture fetch**.
- **G-buffer PS:** `snp` textures 6, 4, 3 and 1 at t1, t3, t4, t5, plus `cbPerFrame`.
- **Native gate:** SAT `snp` binding **on the vertex stage** (t2) as well as the pixel stage,
  `cbPerFrame` in both stages, and the G-buffer state.
- Witnesses: `c1090` (W3) and `c5110` (W7) of section 69.

Tables: `evidence/I-0067/queries/chr/chrtess.tsv` (materials with GX04 or GX90 or an MTD
tessellation mode).

Exports: `decompiled/DS3-1152-F-000AA630`, `00142430`, `023C9D80`, `023D5EE0`, `023A0B40`,
`02358640`, `02358570`.

## 73. Producers of `cbTessScene` and `cbPNTriangles`, and the previous-frame palette (STATIC; INFERRED where marked)

### 1. `cbPNTriangles` (b6 of the hull and domain programmes): per material, at each draw

**`FUN_1402302C0(sub = mat+0xD0, ctx, exparams)`** is called by the material draw-apply
`FUN_140140850`. It runs when `mat+0xDF` (set by `GX04`/`GX90`) and `mat+0x250` (the
tessellation mode) are both non-zero. It fills 0x30 bytes:

| Row | x | y | z | w |
|---|---|---|---|---|
| 0 (`g_f4TessFactors`) | min(float(u32 `mat+0x258`), 64) | min(float(u32 `mat+0x25C`), 64) | `mat+0x274` (float) | `mat+0x278` − `mat+0x274`, or 0.001 when that is ≤ 0.001 |
| 1 | 1 − `mat+0x260` | min(`mat+0x264`, 0.99) | 1.0 | float(u32 `mat+0x268`) |
| 2 | `mat+0x254` ? (`mat+0xE4` ? exparams`+0x24` : 1) × `mat+0x26C` : 0 | `mat+0x270` | — | — |

- Values from `GX04` (section 67): `+0x254` = d[3] & 1, `+0x250` = d[4],
  `+0x258`/`+0x25C` = (int)float d[5..6], `+0x260` = d[7], `+0x274` = d[8], `+0x278` = d[9],
  `+0x270` = d[10], `+0x26C` = d[11].
- `GX90` gives the same fields with `+0x250` = 2 and `+0x254` = 1.
- `+0x264` and `+0x268` keep the constructor's 0 (`FUN_14022B2C0`).
- INFERRED reading: row 0 = edge and inside tessellation factors (capped at 64) and a distance
  fade (start, range); row 2.x = displacement scale.
- **Upload and binding:**
  - `UpdateSubresource` into the buffer `[[144961B50]+0x4DD0]+0x2C8`;
  - then slots 5 and 6 = {`[…]+0x2D0` (`cbTessScene`), `[…]+0x2C8` (`cbPNTriangles`)} on the
    domain stage (`FUN_1419209D0`) and the hull stage (`FUN_141920820`, `HSSetConstantBuffers`,
    vt+0x1F0).

### 2. `cbTessScene` (b5 of the hull and domain programmes): per view

- **`FUN_14013AA60(view, source)`** (the per-view constant upload that also writes
  `cbSceneParam`, I-0064) copies the source draw context's 0xC0-byte block `+0x1130`
  (`FUN_14013A930`) and uploads it to **`[[144961B50]+0x4DD0]+0x2D0`**.
- `g_f4x4ViewProjection` is at @0 and `g_f4GUIParams2` at @96.
- **Writer of the block:** not found statically.
  - No direct store to `+0x1130..+0x11EF` exists outside the zeroing initialiser
    `FUN_1400843C0` and the copies `FUN_140139F40`/`FUN_14013AA60`.
  - INFERRED: the view's view-projection, as in `cbSceneParam`.
- **Runtime read:** the buffer `[[144961B50]+0x4DD0]+0x2D0`, or the view's `+0x1130`, on the
  `c5270` veil draw.

### 3. The previous-frame palette (velocity)

Separate note for the velocity specification:
[VELOCITY-SKINNING](VELOCITY-SKINNING.md).
- The pose owner keeps a ring of 1 or 2 palettes, with 2 when keepPrevious holds:
  `SprjModelDrawEntity` `E+0xCA` bit 4, ChrAsm parts `E+0xBB0` bit 6.
- The binder `FUN_140064340` puts the current palette in b5 `cbObjMatrix` and the previous one
  in b6 `cbPrevObjMatrix`.

Exports: `decompiled/DS3-1152-F-002302C0`, `0013AA60`, `00015490`, `00015FC0`, `00016000`,
`00016050`, `00015B10`, `00064340`.

## 74. Independent check: is the `_Phantom_` pixel programme used by a normal player? (STATIC; INFERRED where marked) - WITHDRAWN, see section 75

Check of the claim "FUN_1400A9EF0 replaces the Fwd/FwdSdw slots by `<shader>_Phantom_Fwd` for the 64 families, including for a non-phantom character" (I-0068 PHANTOMS-SIGNS-FOGWALLS-NATIVE section 1; section 42 here).

**Confirmed (read in the exports):**
- `FUN_140082E50` (the shader-set factory) always ends with `FUN_1400A9EF0(set, …)` (call at `14008314E`, reached on both branches). There is no character, team or phantom condition anywhere in it. A9EF0 only tests the device assertion, then loops over the 20 pass names and acts when `(pass & ~4) == 0`, i.e. passes 0 (`Fwd`) and 4 (`FwdSdw`).
- For those passes it writes the set's per-pass pixel table `set+0x18+pass*0x18` (`+0x18/+0x20/+0x28` for pass 0, `+0x78/+0x80/+0x88` for pass 4) with the repository lookup `repository->vt+0x20(<shader>_Phantom_<pass>)`; the `[A]` and `[AA]` slots only for passes other than 2 and 3. `FUN_1400AA630` had filled the same slots with the plain `<shader>_<pass>` just before, and A9EF0 overwrites them. The result never goes into a second table.
- The only reader of that table is the getter `FUN_1400AA280` (`out[1] = set+((pass+1)*3 + variant)*8`), called by `FUN_140140850`/`140680` with the draw's pass and variant. So for any set built by `82E50`, a `Fwd` draw binds `<shader>_Phantom_Fwd[_A]` and a `FwdSdw` draw `<shader>_Phantom_FwdSdw[_A]`, whoever the character is. The plain `<shader>_Fwd` programme of a set is not reachable through this getter.
- Hair and eyebrows: the `_al` MTD of the player uses `GXFlver_ColDifSpcBumpIbl_Bloody` (and hair `GXFlver_HairIbl2` / `SAT_HairIbl2`). All three have `_Phantom_Fwd`, `_Phantom_Fwd_[A]`, `_Phantom_FwdSdw` and `_Phantom_FwdSdw_[A]` in `evidence/I-0063/artifacts/scan/shader.tsv`. So the forward pass 0x29 draw of the eyebrows (class 2, alpha-tested `_[A]`) uses `GXFlver_ColDifSpcBumpIbl_Bloody_Phantom_Fwd_[A].ppo`, not `GXFlver_ColDifSpcBumpIbl_Bloody_Fwd_[A].ppo`.

**Condition under which the `_Phantom_` slot is chosen:** none per character. It is chosen at set construction for every set; the phantom look comes from the material constants `+0x1A0..+0x1EF` (PhantomParam, section 13), not from the programme.

**Not confirmed / refuted:**
- "Without `_Phantom_` the original does a depth-only draw" is not supported by the code. `FUN_140140850` returns without drawing when the pixel programme of a non-depth pass is null (`puVar3 == 0 → return 0`); nothing falls back to depth. INFERRED: for a shader without any `_Phantom_Fwd` (for example `GXFlver_ColDifSpcBumpEmiIblGlow_Bloody`, `…EmiMulIblGhost`, which are absent from the 64) the repository lookup returns null, so either those sets are drawn through the alternate set `mat+0x78` (selected when `ctx+0x425`, copied from `[pipeline+0x3F7D]` by `FUN_140204C30` and from the pass descriptor by `FUN_1401A8580`), or the lookup falls back. Which one was **not** read: the repository slot `vt+0x20` and the writer of `ctx+0x425` are open.
- INFERRED: with weight 0 / no PhantomParam row the `_Phantom_Fwd` programme renders like a normal forward draw (same inputs), so the visible difference for a player is small. The native `<family>_Fwd` is therefore a plausible approximation, but it is not what the original binds. If the pilot wants pixel parity with the original on hair and eyebrows, build the `_Phantom_Fwd[_A]` variant for the 64 families and feed `+0x1A0..+0x1EF` with the neutral values of a non-phantom character.

**Runtime check (one test):** break on `FUN_140142430` during the eyebrow forward draw and read the pixel shader name; expected `GXFlver_ColDifSpcBumpIbl_Bloody_Phantom_Fwd_[A]`.

Exports: `decompiled/DS3-1152-F-00082E50`, `000A9EF0`, `000AA630`, `000AA280`, `00140850`, `00140680`, `00204C30`, `001A8580`.

## 75. Correction to sections 42 and 74: the `_Phantom_` set is the alternate set `mat+0x78`, chosen by `DC+0x425` (STATIC; INFERRED where marked)

Sections 42 and 74 said that every shader set carries the `_Phantom_` pixel programmes in its Fwd/FwdSdw slots. **That is withdrawn.** Only the sets built by `FUN_140082E50` carry them, and those sets are not the ones a normal draw uses.

**Where each set comes from (`FUN_140082220`, the material builder; resolver vtable `14294E6B8`, slots read from the table):**

| Resolver slot | Function | Role |
|---|---|---|
| `+0x28` | `FUN_140082220` | the material builder itself |
| `+0x40` | `FUN_140D24390` | MTD by name |
| `+0x48` | `FUN_140D24430` | bundle record by name |
| **`+0x60`** | **`FUN_140D245A0`** | repository lookup `FUN_140D46350` → entry `+0x60` = the **normal** set |
| **`+0x68`** | **`FUN_140082E50`** | the **alternate** set, built with `FUN_1400A9EF0` last |

- **Normal set (`mat+0x70`).** `FUN_140D46350` creates the repository entry and calls `FUN_140D467E0`, which builds the 0x3F00 set with `FUN_1400A9BA0`, `FUN_1400AA630` (passes 0..16) and `FUN_1400AA4D0`. **There is no `FUN_1400A9EF0` in it.** `FUN_140140F10` stores it in `mat+0x70`. Its Fwd and FwdSdw slots are the plain `<shader>_Fwd[_A]` and `<shader>_FwdSdw[_A]`.
- **Alternate set (`mat+0x78`).** `FUN_140082220` calls resolver `+0x68` (`FUN_140082E50`) and `FUN_140081AF0` stores the result in `mat+0x78`, in two cases:
  - the bundle record's resource flag bit 0 (`[[rec+0x18]+0x40] & 1`);
  - `gxMtd+0x88` bit 26 (`0x4000000`): the shader name is in the 43-entry table `1447371E0` (stride 0x200).
  - `FUN_1400A9EF0` has exactly one caller, `FUN_140082E50` (call at `14008314E`). So `<shader>_Phantom_<pass>` exists only in `mat+0x78`.
- **The 43 names** (read from `.data`): `GXFlver_SSSEmiIblGlow_Bloody`, `_SSSIbl_Bloody`, `ColDifSpcBumpMulIbl_Bloody`, `…MulMaskIbl_Bloody`, `…Ibl_Bloody_Df_Cm`, `…EmiIbl_Bloody`, `…EmiIblGlow_Bloody`, `…BumpIbl`, `…BumpIbl_Bloody`, `…TrIbl_Bloody`, `EyeIbl`, `FgFaceIbl_Bloody`, `FgFaceSSSIbl_Bloody`, `HairIbl2`, `HairIbl_Bloody`, `SkinIbl_Bloody`, `SkinSSSIbl_Bloody`, `Equip`, `Equip_Cm`, `Equip_Df`, `Equip_Df_Cm`, `…BlendIbl_Bloody`, `…Ibl_Bloody_Df`, `EyeIbl_Bloody`, `…Ibl_Bloody_Df_Bloody`, and their `_Cloth` forms. This is the character-shader list; the SAT bundles come in through the resource-flag case.
  - INFERRED: `EmiIblGlow_Bloody` and the `Ghost` family have `mat+0x78` too (name in the table, or not), but no `_Phantom_Fwd` file exists for them in `shader.tsv`; what `FUN_1400A9EF0` stores for a missing programme (a null from the repository lookup) was not read.

**Which set a draw uses (`FUN_140140850` / `FUN_140140680`):**

```
set = mat+0x70
if (mat+0x78 != 0 && ctx+0x425 != 0) set = mat+0x78
```

- `ctx` here is the model draw context `GXFlverModelDC`. Its constructor `FUN_1401A8580` copies the entity's 400-byte current state to `DC+0x240` and sets **`DC+0x425 = state+0x87`**. The state is the one `FUN_140D079C0` receives (`param_3`, tested at `140D07D44`).
- **`state+0x87` is also read by `FUN_140057A20`** (section 7) for kind-2 rows: with `state+0x84` ∈ {0, 0xFF} and `state+0x85` = 0, a draw whose fade `state+0xDC` is below 1.0 goes to the **forward pass 0x29** instead of 0x21 when `state+0x87` is set. And `FUN_140D079C0` ORs it into the draw struct.
- Default: `FUN_14004DF20` (state constructor) zeroes the first 0xA0 bytes, so `state+0x87` = 0; `+0x84` = 0xFF, `+0x85` = 0, `+0x86` = 0.
- **The writer of `state+0x87` was not found by this search - it is found in section 76** (the byte is `holder+0x97` of the state holder at entity+0x230, set from a PhantomParam row; no direct store to `+0x87`). Searched over the whole image: no byte, word or dword store, no `OR`/`AND`/`SETcc`, no `[reg+reg*k+0x87]` form, and none at the entity offset `+0x2C7`, `+0x2C4` as a dword. The only byte stores to `+0x87` belong to other classes (`FUN_140593460`/`140594180` = a face-parameter copier, `FUN_140E2C030`, `FUN_14102F150`). INFERRED: it is written through a state copy or a packed field.
- **`DC+0x425` has a second class of writer:** `FUN_140204C30` sets `+0x425` of the **view context** (vtable `143D61278`, the object `FUN_14024E790` and `FUN_1402997E0` read) from `[pipeline+0x3F7D]`. That is a different object: it gates the extra forward item in those two functions together with `[[GXRS+0x5360]+0x74]`. Do not confuse the two. `[pipeline+0x3F7D]` is last frame's `+0x3F7C`, set by views that merge a flag in their destructor (`FUN_1401389D0`, view `+0x891`) and by `FUN_140103510`.

**Consequence for the player's hair and eyebrows (`_al`, class 2, pass 0x29):**
- A normal draw (state `+0x87` = 0, so `DC+0x425` = 0) uses `mat+0x70`: the plain `GXFlver_ColDifSpcBumpIbl_Bloody_Fwd_[A]` pixel programme. **The native `<family>_Fwd` is right for the player.** The claim of I-0068 that the original binds `_Phantom_Fwd` for a non-phantom character is **false**, and so was section 74's confirmation. The pilot must not switch to `_Phantom_`.
- The `_Phantom_` programmes are drawn only when the entity's state byte `+0x87` is non-zero. INFERRED: that is the phantom/ghost state of the entity (summoned phantom, replay ghost); it then also forces the forward pass 0x29 while fading.
- The "depth-only draw without `_Phantom_`" claim is still unsupported (section 74): a null pixel programme makes `FUN_140140850` return without drawing.

**Runtime check (one test):**
1. break at `FUN_1401A8580` (`+0x425` store at `1401A85C8`/`1401A85CF`) during the player's eyebrow draw: `DC+0x425` must be 0;
2. repeat with a summoned phantom or a replay ghost: expect 1 and `mat+0x78`;
3. a write breakpoint on `entity+0x240+0x87` on the phantom gives the writer.

Exports: `decompiled/DS3-1152-F-00082220`, `00082E50`, `000A9EF0`, `00D467E0`, `00D46350`, `00081AF0`, `00140F10`, `00140850`, `00140680`, `001A8580`, `00057A20`, `00D079C0`, `0004DF20`, `00204C30`.

## 76. Writer of `state+0x87` (`DC+0x425`): the PhantomParam id reaches the draw state through two chains (STATIC; INFERRED where marked)

Closes the open point of sections 47 (activation, point 2) and 75 (writer not found). The earlier search looked for stores at `state+0x87` and `entity+0x2C7`. There are none because the writes go through a **helper object that holds the state**: its base is 0x10 bytes before the state, so the byte is `holder+0x97`, never `+0x87`. A scan for stores covering `0x87` or `0x2C7` could not find it; the way in was the call chain from the shader-set selection back to the callers of the holder's setter.

### 1. The state holder `X` (constructor `FUN_140D3FCB0`, vtable `142952340`)

- `X+0x10` is the 400-byte draw state (`FUN_14004DF20` initialises it). So `state+0x84/0x85/0x87` are `X+0x94/0x95/0x97`, and `state+0xDC` (fade, `mulCol.w`) is `X+0xEC`. `X+0x190` is the dirty byte (`state+0x180`), `X+0x828` an `int` "kind".
- In `SprjModelDrawEntity` (vtable `142948EF8`) `X` sits at **entity+0x230**, so the state is the entity+0x240 of section 47 and `state+0x87` is entity+0x2C7. `FUN_140D06E10` (slot 1) passes entity+0x240 to the draw slot 18 (`FUN_140D079C0`).
- The PhantomParam lookup is `FUN_140E35EF0(out, id)` -> `FUN_140E36260(out, id)`: `id < 0` gives a null row, otherwise a binary search in `[param repository 144798108][type 0x3A]` (`FUN_140E3DB80`) over `(id, rowIndex)` pairs. INFERRED: type 0x3A is PhantomParam. The row is 0x30 bytes, the accessors match section 47's columns, and the game-data rows have `+0x2C` in {0, 1, 3} (`evidence/I-0067/queries/phantom/phantomparam.tsv`).
- Row accessors: `FUN_140E35F30` = float `row+0x24` (1.0 for 85 rows; 0.5, 0.2.. for the others), `FUN_140E35F50` = kind byte `row+0x2C` (0..3, else 4), `FUN_140E35F90` = DiffColor `rgb = bytes row+0x1A..0x1C / 255, a = row+0x08`.

### 2. Chain A: a single-model entity (`SprjModelDrawEntity`)

`SprjModel` (vtable `1429263D8`, constructor `FUN_140C49210` sets **`model+0x120 = -1`**) pushes `model+0x120` to its draw entity in `FUN_140C4AF10` (slot 5, update): dirty bit `0x20` of `model+0x70|0x74` -> `FUN_140D066F0` (thunk, `jmp [vt+0xC0]`) -> **`FUN_140D07140`** (entity slot 24):

```
entity+0xC38 = id
row = PhantomParam(id)                       ; FUN_140E35EF0
if (row) { X(entity+0x230).+0x97 = 1  (FUN_140D40080, dirty on change); kind = FUN_140E35F50(row) }
else     { X+0x97 = 0;                       kind = -1 }
FUN_140D3FE90(X, kind)                       ; X+0x828 = kind, recompute +0x94/+0x95
```

`FUN_140D3FE90` / `FUN_140D40080` then set `state+0x84/+0x85`:
- `iVar3 = kind` when `kind` is non-zero; for `kind == -1` it is 1 when `state+0xDC < 1.0` (fade), otherwise 1 again if `X+0x97` is set, else 0 (on the `FUN_140D07140` path `kind == -1` implies `X+0x97 == 0`; `FUN_140D40080` runs first with the stale kind and the final values are `FUN_140D3FE90`'s); for `kind == 0` it is 1 only if `X+0x97` is set.
- `X+0x97 == 0`: `state+0x85 = iVar3`, `state+0x84 = 0xFF`.
- `X+0x97 != 0`: `state+0x85 = (iVar3 != 0)`, `state+0x84 = iVar3`.
- So a plain (non-phantom) fade gives `state+0x85 = 1`, `+0x84 = 0xFF`, `+0x87 = 0`: the "`+0x85 = 1`" side of the routing in section 47. A phantom row gives `+0x87 = 1`.

**Writer of `model+0x120`** for a character model: `FUN_1408AA630`, `EnemyIns` slot 214 (vtable offset `0x6B0`): it calls the `ChrIns` base `FUN_1408902D0` (`chr+0x1A50 = row float +0x24`, or 1.0 without a row), then `[chr+0x48]+0x120 = request.id` (request `+4`) with `+0x74 |= 0x20` when it changed.

### 3. Chain B: parts of an assembled model (`SprjAsmModelDrawEntity`: players, ghosts, NPCs with parts)

`SprjAsmModel` (vtable `142925DE8`) keeps one 0x50-byte descriptor per part at `model+0xD8` (the player has 0x2C parts):
- `FUN_140C48910(model, part, id, weight)` writes **`desc+0x44 = (short)id`** and `desc+0x40 = weight` and sets `model+0xF8 |= 0x200010`. `FUN_140C44D20` initialises `+0x44` to `0xFFFF` (= none).
- `FUN_140C48180` (update, slot 2) forwards bit `0x15` through `FUN_140CFFC10` -> entity slot 20 = `FUN_140D009E0`, which copies each 0x50-byte descriptor (including `+0x40..+0x4F`) into the entity array `[entity+0xB00]`.
- The part draw `FUN_140CFDE70` (the per-part draw of the assembled-model entity) builds the part's local state copy and then:
  ```
  if (short [desc+0x44] > 0) {                          ; id 0, -1 and 0xFFFF skip
     row = PhantomParam(id)
     if (row) {
        state+0x87 = 1
        fade       = row float +0x24
        state.mulCol = (rgb * constants, a * fade)       ; state+0xD0..0xDC
        kind = FUN_140E35F50(row)
        if (kind == 0) kind = (mulCol.w < 1.0) ? 1 : -1  ; i.e. 1 or 0xFF
        state+0x84 = (byte)kind
     }
  }
  ```
  Here `state+0x85` is **not** touched (unlike chain A). The local state goes into `FUN_140059040` and from there into `FUN_1401A8580`, whose `DC+0x425 = state+0x87`.
- **Writer of the descriptor id**: `FUN_1408C0640` (`PlayerIns` and `ReplayGhostIns` slot 214, vtable offset `0x6B0`; input `{target, id, weight * [143D7ED50]}`). `target 0` = all 0x2C parts; 1..8 = the equipment slot groups (targets 1 and 2 pick the first part through `FUN_140C3A510`, 3..8 use the fixed first parts 7, 0x13, 0xB, 0xF, 0x17, 0x1B; each takes a 4-bit slot mask from `ChrIns` slot `0x300` with a selector among -2, -1 and 0..5), 9..12 = parts 2..5.

### 4. Who calls slot 214 (the only caller of vtable `+0x6B0`)

`FUN_1409CAF30(holder, request)` looks up the row, pushes the seven `ExParam_Phantom*` (section 47 point 3) into the draw's override list, then **always** calls `[holder+8]->vt+0x6B0(request)`. Its callers:
1. `FUN_140890950` (`ChrIns` slot 36, per-frame update; `PlayerIns` slot 36 `FUN_1408C1BC0` calls it at `1408C1FAA`; `EnemyIns` slot 36 `FUN_1408AA980` has its own copy at `1408AAAB5`): request `{0, id, 0}` with `id = chr->vt+0x3A8()` (slot 117 = `FUN_14088A950`, the resolver of section 47 point 2; `ReplayGhostIns` has `FUN_1408CA7E0`). It is **only called when `id >= 0`**; otherwise the part ids stay `0xFFFF` and the model id `-1`.
2. `FUN_1405B34F0` (`ChrWepEnchantVfxSlot` slot 3, vtable `1427C8970`): the weapon enchant goes through the phantom row too. The request is `{hand target mask (FUN_140868BB0: 4, 6, 8 or 2), id = FUN_1405B31B0(enchant value, ...), 1/dt}`, then a per-part intensity with `FUN_140C488A0(model, part, float, flag)` (`desc+0x3C` float, `desc+0x38` bit 4, dirty `0x200000`). INFERRED: this is the glow of an enchanted weapon; the id table of `FUN_1405B31B0` (21 entries 0x12..0x3B) was not decoded.

### 5. What this fixes in the earlier sections

- `state+0x87` (hence `DC+0x425`, hence the use of the alternate set `mat+0x78` of section 75) is **1 exactly when the model or the part has a PhantomParam id >= 1 whose row exists**. It is not a phantom or ghost state of the entity as such.
- **Normal host player**: the resolver gives `-1` unless `chr+0x1F38` or `chr+0x1A58` hold an id, and `RoleParam` row 0 is `-1` in every column (`roleparam.tsv`; so are roles 3..9 and 12..14). No request is sent, `state+0x87 = 0`, and the plain `<shader>_Fwd[_A]` is used: **section 75's conclusion for the player is now STATIC for the code path** (the resolver's `chr+0x1F38`/`chr+0x1A58` values in a live host remain a runtime fact). Native `<family>_Fwd` is right for the host's hair and eyebrows.
- **Alternate-set users** (INFERRED that their material has `mat+0x78`, section 75): white, red and gold summoned phantoms and invaders (roles 1, 2, 10001.., ids 80..91), NPCs with `NpcParam+0x1CC >= 0` (82 rows: id 80 x29, 81 x16, 60 x26, 74 x2, 88 x3, 92 x4, 187, 33), the `NpcParam+0x238` route (88 rows, id 76) when the team type is flagged, replay ghosts, and weapons under an enchant. An NPC drawn with the `_Phantom_Fwd` look needs the **whole alternate set**, not only a Fwd variant: every pass reads `mat+0x78` when `DC+0x425 != 0`.
- The "invisible when `_Phantom_` is missing" idea of section 47 point 4 is not supported (section 74): a null pixel programme returns without drawing, and a material without `mat+0x78` keeps `mat+0x70`.

### Native checklist

1. Per model or part: `phantomId` (short, `-1` none) from the resolver, the NPC row or the enchant request; `phantomRow = PhantomParam[id]` (type 0x3A, 0x30-byte rows).
2. If the row exists: `state+0x87 = 1`, `mulCol.w *= row+0x24` (and `mulCol.rgb` times the constants `143D7F380`, `143D7EE10`, `143D7F384`), `state+0x84` by the kind rule above (asm parts) or the `FUN_140D3FE90` rule (single models), and select the alternate programme set for all passes.
3. Set none of it for `id < 0` (the host player).
4. Runtime check (one test): break on `FUN_140C48910` for a summoned phantom (expect `id = 80..91`) and on `FUN_140D07140` for an NPC with `NpcParam+0x1CC = 80`.

Exports: `decompiled/DS3-1152-F-` + `00D3FCB0`, `00D40080`, `00D3FE90`, `00D07140`, `00E35EF0`, `00E35F30`, `00E35F50`, `00E36260`, `00CFDE70`, `00C48910`, `00C48180`, `00C488A0`, `00C4AF10`, `00D009E0`, `008C0640`, `008C0240`, `009CAF30`, `008902D0`, `008AA630`, `00890950`, `008AA980`, `008C1BC0`, `0088A950`, `005B34F0`. Tools: `I0067Stores.java` (stores covering a structure offset at any width), `I0067VCalls.java` (indirect calls and tail jumps through a vtable displacement).

## 77. Graphics settings (`GraphicsConfig.xml`, the Options page) down to the GX quality object, and what native has of it (STATIC; INFERRED where marked)

WORKFLOW's graphics-settings axis ("list offline what each setting changes") and the in-game **Options** page (`02_040_OptionSetting`, section 50) had no map. This section gives the file, the token table, the 12 setting slots, the 0x24C-byte quality table decoded per level, every reader of the quality object found by a register-tracking scan, and the native gap. Tools: `Get-QualityTable.ps1` (evaluates the table builder without running the game; its output is `quality-table.tsv`), `Show-QualityTable.ps1` (decodes it), `I0067FieldUsers.java` (users of the fields behind a pointer member), `I0067FuncStrings.java`. Exports: `decompiled/DS3-1152-F-` + `000219B0`, `000220A0`, `00023660`, `00023790`, `000D3650`, `00CED9A0`, `00D55190`, `00DFDD20`, `00EF75A0`, `00EF8430`, `000236E0`, `001B18F0` (RVAs).

### 1. The file and its tokens

- **Writer `FUN_140EF75A0`, reader `FUN_140EF8430`** (both reached through data references in vtable `14424DBF8`; the writer through `FUN_140EF5AB0`; `GraphicsConfig.xml` = `1429B6188`). Key order, as written (`PTR` table `144662AD0`, 8-byte entries): `ScreenMode`, `Resolution-WindowScreenWidth/Height`, `Resolution-FullScreenWidth/Height`, `Auto-detectBestRenderingSettings`, `QualitySetting`, `TextureQuality`, `Antialiasing`, `SSAO`, `DepthOfField`, `MotionBlur`, `ShadowQuality`, `LightingQuality`, `EffectsQuality`, `ReflectionQuality`, `WaterSurfaceQuality`, `ShadeQuality`. In the in-memory config object (`int` fields, `QualitySetting` +0x18 ... `ShadeQuality` +0x44, one `int` per key in the same order).
- **Token tables** (16-byte entries {UTF-16 string, value, mask}): quality keys (`1429B6300`, six entries): `DISABLE` 0, `LOW` 1, `MEDIUM` 2, `HIGH` 3, `MAX` 4, `CUSTOM` 5. `Auto-detect...` and `Antialiasing` (`1429B6360`): `ON` 1, `OFF` 0. `ScreenMode` (`1429B6380`): `WINDOW` 0, `FULLSCREEN` 1 (two entries only; the strings `Fake` and `Real` beside `FullScreenMode` at `1429B6230..1429B6250` suggest that FULLSCREEN is split into a fake (borderless) and a real (exclusive) mode when applied, INFERRED, DISPLAY-SWAPCHAIN-NATIVE).
- **The reader matches the string and otherwise keeps the field's previous value** (`default: 0` only on the switch). So an unknown token such as `ENABLE` is **not** a token: the field keeps its default. The velocity reference runs wrote `<MotionBlur>ENABLE</MotionBlur>` and read option `+0x20 = 1`, `+0x21 = 1`, sample cap 5: exactly level 3 (`HIGH`) of the table below, i.e. the default level. The valid values are `DISABLE`, `LOW`, `MEDIUM`, `HIGH`, `MAX`: `LOW` gives camera blur only (cap 1), `MEDIUM` adds per-object velocity (`+0x21`), `HIGH` cap 5, `MAX` cap 8.

### 2. From the config to the GX quality object

- The runtime config block lives at **`SprjDrawStep+0xA0`** (`SprjDrawStep` vtable `142957778`). Slot 24 `FUN_140D55190` (a draw-step method, INFERRED per frame) calls `FUN_140DFDD20`, which calls `FUN_140CED9A0`: it reads twelve `int` levels at block `+0x08..+0x34`, takes the slot-8 value from the applied copy (`GXRS+0x5310`), and calls **`FUN_140023660`**, which copies the twelve levels to `GXRS+0x5320..+0x534C` (unless the byte `GXRS+0x5350` is set).
- **`FUN_140023790`** (GX rendering-system update, from `FUN_1400C2E30`) compares `+0x5320..` with the applied copy `+0x52F0..` (the whole rewrite sits under `[144961AB0]+0x3E1 == 0`; `FUN_1400220A0` clamps the Effects index to at least 1 at its first fill, `FUN_140023790` does not, so row 0 is used at run time for `DISABLE`); when one differs it rewrites the **quality object `[GXRS+0x5360]`** (0x78 bytes, allocated and first filled by `FUN_1400220A0`) from the **quality table `[GXRS+0x5358]`** (0x24C bytes, copied from `D573B0`'s local table built by **`FUN_1400219B0`**), then calls `FUN_1400D3650` for the texture setting and notifies the registered listeners (`GXRS+0x4E08` list, each listener called through its first vtable slot with the object). `FUN_1400220A0` starts with slot 0 = 2 and slots 1..11 = 3.
- **Slot order** (slot = level `int` index; one `int` of the config block): 0 `TextureQuality`, 1 `Antialiasing`, 2 `SSAO`, 3 `DepthOfField`, 4 `MotionBlur`, 5 `ShadowQuality`, 6 `LightingQuality`, 7 `EffectsQuality`, **8 no XML key** (the decal byte; the block field `+0x28` is skipped by `FUN_140CED9A0`, so slot 8 keeps level 3 for the whole run: a scan of stores covering `GXRS+0x5340` finds only `FUN_1400220A0` and `FUN_140023660`, `I0067Stores.java`), 9 `ReflectionQuality`, 10 `WaterSurfaceQuality`, 11 `ShadeQuality`. The slot to key assignment is established by the field semantics (4 = MotionBlur by `+0x20`/`+0x24`, 5 = shadow map size 8192 at `MAX`, 7 = FFX environment block, 9 = reflection flags with a half/quarter target size, 11 = SSS gate `+0x75`), by the key order of the writer and by the skipped slot; it is not a read of the config-to-block copy itself, which sits in encrypted `SprjDrawStep` code (`140D54xxx`, no function).
- The six preset tokens (`QualitySetting` = `LOW`/`MEDIUM`/`HIGH`/`MAX`/`CUSTOM`, and "Auto-detect") are applied by the Options page; the level sets they imply were not read (encrypted page code).

### 3. The table, per setting and level (decoded by `Show-QualityTable.ps1`)

Levels 0..4 = `DISABLE`, `LOW`, `MEDIUM`, `HIGH`, `MAX`. Checks that the decode is right: `MotionBlur HIGH` = (`+20` 1, `+21` 1, cap 5) is what Reference1186 read in game, and `ShadowQuality MAX` = map size 8192 is the atlas of SHADOWS-NATIVE.

| Slot, key | Object field(s) | `DISABLE` | `LOW` | `MEDIUM` | `HIGH` | `MAX` |
|---|---|---|---|---|---|---|
| 0 Texture | `+0x00,+0x04` (a, b) | 1, 1 | 2, 1 | 3, 4 | 3, 8 | 3, 16 |
| 1 Antialiasing | `+0x08` byte | 0 | 0 | 0 | 1 | 1 |
| 2 SSAO | `+0x09` dword, `+0x0D` word, `+0x0F` byte (7 flags `+9..+0xF`) | all 0 | `+9` | `+9,+0xB` | `+9,+0xA,+0xB,+0xC` | all 7 |
| 3 DepthOfField | `+0x10,+0x14,+0x18,+0x1C` | 0 0 0 0 | 1 0 1 1 | 1 2 3 3 | 1 3 5 5 | 1 5 7 7 |
| 4 MotionBlur | `+0x20` on, `+0x21` object velocity, `+0x24` cap | 0 0 0 | 1 0 1 | 1 1 1 | 1 1 5 | 1 1 8 |
| 5 Shadow | `+0x28` on, `+0x2C`, `+0x30`, `+0x34`, `+0x38` size, `+0x3C` | 0 1 4 128 1024 -4 | 1 1 4 128 1024 -4 | 1 1 2 128 2048 -1 | 1 1 1 128 4096 0 | 1 2 1 128 8192 2 |
| 6 Lighting | `+0x40` float, `+0x44`, `+0x45` | 0.5, 0, 0 | 0.5, 0, 0 | 0.75, 1, 1 | 1.0, 1, 1 | 1.2, 1, 1 |
| 7 Effects | `+0x48` four flag bytes, `+0x4C..+0x64` seven floats | 00 00 00 00; .5 2 .5 .5 .5 .5 1.5 | 00 00 00 00; .75 1.5 .75 .75 .75 .75 1.5 | 01 01 01 00; .9 1.2 .9 .9 .9 .9 1.2 | 01 01 01 00; 1 1 1 1 1 1 1 | 01 01 01 01; 1 1 1 1 1 1 1 |
| 8 (no key) | `+0x68` byte | 0 | 1 | 1 | 1 | 1 |
| 9 Reflection | `+0x6C` on, `+0x6D` lights, `+0x6E` mask, `+0x70` dword | 0 0 0, 4 | 1 0 0, 4 | 1 1 0, 2 | 1 1 0, 2 | 1 1 1, 2 |
| 10 Water | `+0x74` byte | 0 | 0 | 1 | 1 | 1 |
| 11 Shade | `+0x75` word, `+0x77` byte | 00 00 / 0 | 00 00 / 0 | 01 00 / 0 | 01 01 / 0 | 01 01 / 1 |

Meanings that the code supports (STATIC): Texture `(a, b)` goes to `FUN_1400D3650(textureManager, a, b)`: `texmgr+0x5E8 = {0, 6, 7, 8}[a]` (sampler filter mode) and `texmgr+0x5FC = b` (INFERRED: maximum anisotropy 1/1/4/8/16 at `DISABLE`..`MAX`), and it rebuilds the 16 pooled sampler states when their 0x38-byte descriptor changed; MotionBlur and Shadow as above (`+0x28` is `[[GXRS+0x5360]+0x28]`, "shadows on" of SHADOWS-NATIVE, `+0x38` the shadow map size); Effects: the FFX environment block read by `GXFfxSceneCtrl` (`+0x48..+0x64`, EFFECTS-PHASE1); decal byte `+0x68` gates deferred decals (`FUN_140051AA0`, DECALS-NATIVE); Reflection: `+0x6C` enables the reflection view (`FUN_1400FB0E0`, `FUN_140103510`, the material block `FUN_14022E930` zeroes `+0xF8/+0xFC` of its input when it is 0), `+0x6D` lets the pass-0x39 reflection collect lights, `+0x6E` adds mask bit 1 to the reflection pass (`FUN_1400FCE30`, CHR-FADE-EXPARAM-REFLECTION), `+0x70` is **4 or 2: the reflection target size divisor (INFERRED, no reader found by the scan)**; Water `+0x74` gates the FFX 0x39 draw (`FUN_14024E790`, `FUN_1402997E0`, `FUN_140103510`); Shade `+0x75` is the SSS command gate of TONEMAP-SKIN section 4 (reader `FUN_140101700`, `FUN_1401094F0`, `FUN_140087290`/`FUN_140089280`); the SSAO flags select the AO source (`FUN_1401B18F0` returns the half-resolution or the reprojected AO target from `+9`, `+0xB`, `+0xE`; `FUN_1401B1960`, `FUN_1401B26E0` read `+9` and `+0xA`/`+0xD`); DepthOfField is read by the DoF passes (`FUN_1400E4070`, `FUN_1400E4890`, `FUN_1401CED20`, `FUN_1401D47C0`, from `FUN_1400FB890`); AntiAliasing by `FUN_1400FB890` and `GXAntiAliasingFilter` (`FUN_1401B9790`). The Lighting float (0.5..1.2) and the bytes `+0x44/+0x45` (read by `FUN_14010A220`, `FUN_1401A0080`) are not named further (INFERRED a light-buffer scale and two feature gates).

Full reader list (register-tracked scan, `fu/fields.tsv`, 178 accesses over 5360 and 5368): `+0x20`: `FUN_140103510`, `FUN_1400E4B50`, `FUN_14013BE60`, `FUN_14013BFC0`; `+0x21`: `FUN_140057A20`; `+0x28`: `FUN_1400F90F0`, `FUN_1400F9B50`, `FUN_140103510`, `FUN_1401094F0`; `+0x2C/+0x30/+0x34`: `FUN_140085540`, `FUN_1401B5720`; `+0x40/+0x44`: `FUN_14010A220`; `+0x48..+0x64`: `FUN_14024D000`, `FUN_1402997E0`; `+0x76`: `FUN_14005CEC0`, `FUN_14005D5C0` (the scan may have followed a different base there).

### 4. What native has

- **Native does not build the quality object.** The native root constructor (`native/frame/game_bootstrap.cpp`, comment before `allocate`) builds `GXRS+0x5368` (`FUN_127010`) and states that `+5358/+5360` (the quality tables) are not built and that every consumer tests them. The one native reader (`CollectNativePointLights`) reads `[root+0x5360]+0x40` only if the pointer is non-null. So the engine's own gates that read this object see no object (the comment says each tests it; to be confirmed by one native read of `[root+0x5360]`, section 5), and **no graphics option reaches the native renderer** (INFERRED from that comment and from the absence of any native reader other than the light-fade one): texture filtering, anti-aliasing, SSAO/DOF/motion-blur levels, shadow size and cascades, effect scales, decals, reflections, water and SSS are fixed by what native implements, not by `GraphicsConfig.xml` or the Options page.
- **Contract to implement** (a native service, no D3D11): build the 0x78-byte object and the 0x24C-byte table exactly as above (the table is data, section 3; `FUN_1400219B0` has no calls), take the twelve levels from the same source as the original (the config block `SprjDrawStep+0xA0`, kept by the engine; or `CSGraphicsConfig` through `FUN_140EF8430` once the file is read) and run the `FUN_140023790` rewrite on change; the texture entry becomes native sampler parameters (filter mode and anisotropy) of the native sampler pool, not `FUN_1400D3650`. Each native pass then reads its own fields from that one object instead of a constant, in the order of the implementations: shadow size and on/off (SHADOWS-NATIVE), SSAO flags (SSAO-NATIVE), velocity (`+0x20`, `+0x21`, `+0x24`), reflection (`+0x6C`, `+0x6D`, `+0x70`), effects flags and scales (FFX), SSS (`+0x75`), decals (`+0x68`), DoF, anti-aliasing.
- Resolution, aspect and display mode are not in this object: `ScreenMode` and the four resolution keys feed the window path (DISPLAY-SWAPCHAIN-NATIVE).

**Native construction, step by step:** [QUALITY-OBJECT-NATIVE](QUALITY-OBJECT-NATIVE.md) (the levels are live at `[image+0x4796298]+0xA8`, the update to transcribe, oracle bytes, live rows).

### 5. Open (live or encrypted)

1. **Update 2026-11-10 (study thread, [STUDY-R12-SETTINGS-PAGE](../I-0132-rt-optimisation-zones/STUDY-R12-SETTINGS-PAGE.md) §1):** the
   Advanced Settings page is plain C++ (`FUN_140BE3150` → `FUN_140AAFD10`: eleven rows, one per key from `TextureQuality` to
   `ShadeQuality`, their FMG labels and choice lists; defaults `FUN_140C20E20`, `Antialiasing` ON shown as 3); its movie
   `02_042_PC_GraphicSetting` has no row logic. Still open there: who fills and applies the page's values. The rest of this item
   stands. The config to block copy and the Options page that edits the config (encrypted `SprjDrawStep`/`NewMenuSystem` code): which preset sets which levels; what `Antialiasing ON` maps to (the table is `DISABLE..MAX`, ON/OFF has two tokens); whether the page applies at once or on leaving; `ScreenMode` apply is deferred (DISPLAY-SWAPCHAIN-NATIVE).
2. Read `[[144961B50]+0x5360]` (0x78 bytes) and the levels `+0x5320..+0x534C` in the original with Thomas's file (expected from his `GraphicsConfig.xml`: Texture MAX = 3,16; AA ON; SSAO HIGH; DOF MAX; MotionBlur DISABLE; Shadow/Lighting/Effects/Reflection/Shade MAX; Water HIGH), and once with another set, to settle the AA and the preset mappings. This is a read of the process, not a change of the game.
3. The reader of `+0x70` (reflection target divisor) and the meaning of the Lighting float.
4. Menu views: the quality object is read by the menu views as well (shadow map in the Status view, section 48; SSS in the portraits). (INFERRED) Native menu rendering therefore uses its own constants for these.

## 78. Scaleform feature inventory of all 102 menu movies, the fonts per language, and what the native HAL lacks (STATIC + game data; INFERRED where marked)

Section 61 read only the HUD movie. This is the family inventory of every shipped menu movie (WORKFLOW: build the coverage inventory offline first), so that the native Scaleform HAL (section 36) is checked against what the pages really use, not against the title alone.

**Data and method.** The 102 movies of `data1/menu/` and `data1/menu/Win/` (the `.gfx` list of section 50, plus the sub-pages and the PC pages) were recovered read-only with `investigations/I-0063-coverage-effect-families/Inventory-Archives.ps1 -Pattern 'menu/.*\.gfx' -Archives Data1` into the scratchpad (licensed bytes, disposable). `Get-GfxFeatures.ps1` walks every tag, recurses into sprites and parses `PlaceObject2/3` (matrix, colour transform, clip depth, filter list, blend mode, cache-as-bitmap), the style arrays and the edge records of `DefineShape1..4`. The result per movie is `menu-gfx-features.tsv` (counts, external image names, imports, filter parameters). No parse error occurred in any of the 102 files.

### 1. What the movies are

- Uncompressed `GFX` version 11, **ActionScript 3** (124 `DoABC` tags, no AS2 action tag, no AS2 init action), no sound, no video, no morph shape, no `DefineFont`, no `DefineScalingGrid`.
- **2,167 shapes, 17,469 edge records** in total; images are all external: 1,030 `DefineExternalImage2` (tag 1009) naming `MENU_*.tga` (the 01_Common.tpf textures of section 53). **No font is inside a page: 90 movies import their fonts from `font.swf`** (`ImportAssets2`), the shared font movie of section 3 below.

### 2. Rendering features used, with the native state

| Feature | Use in the 102 movies | Native HAL (`native/frame/game_ui_session.cpp`) |
|---|---|---|
| Fill kinds | solid; bitmap fill type `0x40` (the only bitmap kind); linear gradient 7 (2 pages: `01_000_fe`, `05_000_title`); radial gradient 11 (11 pages: HUD, Title, Inventory, Options, Warp, Shop, Enhanced Weapon, Item Box, Blood Message top, PC key configuration, Options); 42 gradient stops; spread mode reflect; no focal gradient, no `0x41..0x43` bitmap | gradients and bitmap fills go through the engine's own UI programmes and batches; witnessed on the Title; others unwitnessed (live) |
| Blend modes | Add (8) 141 placements in 57 pages, Multiply (3) 30 in 8 pages, Screen (4) 23 in 14 pages, Normal (1) 3 in `01_000_fe`; **no other mode** | `PushNativeBlend` / `NativeBlendState` admit 0, 1, 3, 4, 8, 18: **covers every mode used** |
| Masks (clip depth) | 145 placements in 37 pages (HUD, lists, Inventory, Options, Title, Brightness...) | stencil mask scopes (`PushNativeMask`, `EndNativeMask`, `PopNativeMask`): implemented, nesting up to 255 |
| Colour transform with add terms | 2,397 placements in 44 pages; 145 to 191 per page on the Options pages (graphics, mouse, key configuration), Colosseum and the two New Game pages (brightness, controls) | CPU batch factors (`factor_offset`) in the vertex layout; unwitnessed beyond the Title (71 on `05_000_title`) |
| **Filters** | **DropShadow 137 placements in 53 pages; Glow 52 in 3 pages; Blur 90 in 1 page (`01_000_fe`)**; no bevel, gradient glow, convolution or colour matrix | **None: `NativeFilterCapabilities` returns 0 and the primitive is drawn without its effect** (the code says so). Every text and icon drop shadow, the HUD glows and the banner blur ramps are absent |
| Cache as bitmap | 2 placements (`04_030_chrmake_colorselect`, `win_04_030_chrmake_colorselect`) | needs an offscreen target: `native_ui_display_offscreen_unavailable` |

**Filter parameters** (what a native implementation has to reproduce; all 279 placements read):
- **DropShadow (137):** every one opaque black, strength 1, composite-source on, no inner shadow, no knockout; blur 2x2 (54), 5x5 (46), 7x7 (24), 3x3 (9), 1x1 (2), 4x4 (2); distance 2 (93), 3 (24), 5 (12), 1 (6), 10 (2); angle 0.8 rad (132; 0, 0.2, 0.5 and 3.1 rad once or twice); passes 3 (83), 1 (53), 2 (1).
- **Glow (52):** colour (64, 92, 255, 255), blur 5x5, strength 1.5, passes 3 (35, the HUD frame); black, blur 3x3, strength 1, passes 1 (6); zero strength (3); an animated ramp of eight (colours from (228, 13, 36) to (92, 78, 218), blur 0.7 to 4.3, strength 0.2 to 1.3, passes 1 to 2); one grey (204, 204, 204) blur 5x5. All composite-source, none inner or knockout.
- **Blur (90):** radius 0 to 3 in small steps (the banner and HUD animations), equal X and Y, passes 1 (34), 2 (20), 3 (36).
- Pages by filter: Glow and Blur only on `01_000_fe`, Glow also on `01_040_gesturetop` and `02_000_ingametop`; DropShadow on 53 pages including all in-game list pages.

Consequence: **the one HAL feature that the menus use and native lacks is the filter family (and the offscreen cache of the colour-select page)**, in the three types above. Blend, mask, gradient and colour-transform needs are within the HAL already coded. The title and the Load Game pages have been witnessed natively without shadows, which is why the loss has not been seen as a failure.

### 3. Fonts and languages

- Font movies: `font:/%s/font.gfx` (`1429A1F90`), one per family: `MatisseProN` (10.2 MB), `TsukuMinPro` (8.4 MB), `TsukuMinProSub` (126 KB), `TsukuOldMinPro` (95 KB), `Polish` (8.4 MB), `TraditionalChinese` (11.4 MB), `Hangul` (8.5 MB), `SimplifiedChinese` (4.8 MB), all in `data1/font/<family>/font.gfx`. The pages reference the glyphs through `font.swf` imports. Old-style text (`FrpgMenuFontPrinter`, `EzMenuFontPrinter`) uses bitmap fonts `data1/font/JAPANESE/DSFont24.tpf/.ccm`, `TalkFont24.tpf/.ccm` and the debug `DbgFont14h` (`font:/DbgFont14h.tpf/.ccm`).
- **Language ordinals** (`FUN_140EA5D70`, table `144658390`, 21 entries): 0 `jpnJP`, 1 `engGB`, 2 `fraFR`, 3 `itaIT`, 4 `deuDE`, 5 `spaES`, 6 `spaAR`, 7 `porBR`, 8 `engUS`, 9 `nldNL`, 10 `polPL`, 11 `rusRU`, 12 `danDK`, 13 `norNO`, 14 `finFI`, 15 `sweSE`, 16 `turTR`, 17 `porPT`, 18 `zhoTW`, 19 `korKR`, 20 `zhoCN`. Current ordinal: global `14479BA10`.
- **Font tables by ordinal** (`FUN_140EA5DE0` and `FUN_140EA5E60` read `144658440[ordinal]` and `1446584F0[ordinal]`; they feed `MenuFont_01` and `MenuFont_02`, see [FONTS-LANGUAGES-NATIVE.md](FONTS-LANGUAGES-NATIVE.md) section 4; `FUN_140EA5E20` reads a different table, `1446585A0`, the per-language plus string):

| Ordinals | First table | Second table |
|---|---|---|
| 0 jpn | MatisseProN | TsukuMinProSub |
| 1 engGB, 8 engUS | MatisseProN | TsukuOldMinPro |
| 2-7 fra, ita, deu, spaES, spaAR, porBR; 9 nld; 12-17 dan, nor, fin, swe, tur, por | TsukuMinPro | TsukuOldMinPro |
| 10 pol | Polish | Polish |
| 11 rus | TsukuMinPro | TsukuMinPro |
| 18 zhoTW | TraditionalChinese | TsukuMinProSub |
| 19 kor | Hangul | Hangul |
| 20 zhoCN | SimplifiedChinese | SimplifiedChinese |

- **Language TPF** `menu/<lang>/80_language.tpf` (the banners, section 61): the Data1 archive holds 13 folders: `rusRU`, `fraFR`, `korKR`, `itaIT`, `deuDE`, `spaAR`, `engUS`, `jpnJP`, `zhoTW`, `polPL`, `spaES`, `zhoCN`, `porBR`. The ordinals `engGB`, `nldNL`, `danDK`, `norNO`, `finFI`, `sweSE`, `turTR`, `porPT` have no folder of their own and can never be selected: `FUN_140EA61F0` maps them to `engUS` (8) (STATIC, exact chain from the Steam language name in [FONTS-LANGUAGES-NATIVE.md](FONTS-LANGUAGES-NATIVE.md) section 1).
- Native consequence: the glyph path is CPU (Scaleform rasterises the font movie glyphs into its glyph cache, uploaded through the HAL texture updates of section 36); native owns a glyph context (`native/frame/game_ui_glyph_context.cpp`). The languages need no renderer difference, only that the right `font.gfx` and `80_language.tpf` are admitted as resident files, and that the glyph cache texture size (dependent on the character set) is not assumed. The language of the native witnesses is that of the test install (not recorded in the notes).

### 4. Witness set by family (for the live rows)

One page per distinct feature set, all reachable in game or from the title:
1. HUD `01_000_fe` (in game, High Wall): filters (DropShadow, Glow, Blur), linear and radial gradient, masks, Normal blend; banners through the language TPF.
2. Inventory `02_020` or Item Box `03_050`: Multiply, Screen and Add blends, radial gradient, 14 multiply placements.
3. PC graphics options `win_02_042_pc_graphicsetting` (Esc, System, Options): the heaviest colour-transform page (146) and the first page for section 77's settings.
4. New Game brightness page `05_031` (new game only, so a fresh character: not reachable with the saves in use, INFERRED): 191 colour transforms.
5. Character creation colour select `04_030`: cache as bitmap and mask (already reachable, section 32).
6. Language: one run with another game language (a Steam setting, so a change of the user environment: only with Thomas's agreement) to admit another `font.gfx` and `80_language.tpf`; a CJK language is the discriminating one (glyph cache size).

Exports: none new (data analysis). Tools: `Get-GfxFeatures.ps1`; table `menu-gfx-features.tsv`.
