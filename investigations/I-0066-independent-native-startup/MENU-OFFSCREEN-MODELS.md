# Menu offscreen model renders: native design (I-0066, 1160)

Family (D-0161): the save-slot portraits `CSMenuProfModelRend` (Load Game, 10 x 128x128,
`SYSTEX_Menu_Profile00..09`), the player view `CSMenuPlayerModelRend` (Status/Equipment,
560x768) and the face view `CSMenuFaceModelRend` (character creation, 784x784). Static map:
[I-0067 MENUS-MAP](../I-0067-static-menus-subsystems-map/MENUS-MAP.md) sections 1, 7, 8, 11.

## Measured state

- Reference1157: the ten portrait slots `14478D680[0..9]` exist at the title (vtable
  `14295D938`) and the Load Game list shows rendered heads.
- Native1158: every slot is nil, because `NativeMenuProfilesStartup` replaces `AE5B70`
  (explicit visual omission: the constructors build the old GX scene graph).
- Native1159 (8758670d): `ADEF50` transcribed with a skip for nil portraits, so the Load Game
  list opens natively with the game's dummy faces (`MENU_DummyProfileFace_0N`), 0 faults.
- Native1160: loaded bytes of the encrypted render callback `D658B0` and of `D57EB0` acquired
  (`evidence/I-0066/artifacts/i66-native-loadedcode-1160/loaded-code-d658b0-d57eb0.json`).

## What the original constructs per renderer (STATIC, decompiles in `menu-portraits-1158`)

`D65DD0` (base): FD4 step (state table `144793990`), `D644F0` offscreen texture (a
`CSOffscreenGxTexture` proxy registered by name in `[144792F58]` through `D4A120`; RendManImp
list `[[144796298]+8]` at +0x18), three model slots, the appearance block (+0x4E0), then
`D64C30(texture, w, h)`:

| Field | Object | Natively |
|---|---|---|
| +0x20 | two view objects (`4AFF0` x2) | CPU (INFERRED) |
| +0x28 | view (`4AFF0`) | CPU (INFERRED) |
| +0x30 | scene (`CED370`, `CED790`) | CPU (INFERRED) |
| +0x38 | GX scene object (`4FDB0`, `4FEC0`) | GX: replace |
| +0x40 | scene list (`C2400`) | GX: replace |
| +0x48 | `GXSgCompositeScene` from factory `14295BA50` (descriptor `{0x8422014, w, h}`), configured through its vtable, cleared by the old device (`141920200` via `[[144961AB0]+0x60]`) | GX: replace by a native offscreen target owner |
| +0x50 | light (`D039E0`, colours `.data 14478CC68..CC84`) | CPU values, native light |
| +0x60 | render callback registration `D65080(&14295BB10, this, D658B0)` | native callback |

Per frame `ADE0B0 -> D66900` clears +0x48's colour through the old device; start/stop
(`D64B50`/`D65140`) add/remove +0x48 in the RendManImp offscreen list and point the texture
proxy at its colour.

## Native design (planned, not implemented)

1. Construction: keep `AE5B70`/`D6B780`/`D65DD0` and the step machine genuine; replace only
   `D64C30` (native offscreen owner: CPU view/scene/light objects kept, GX objects +0x38/+0x40/+0x48
   replaced by a native target record of w x h), `D66900` (native clear) and `D64B50`/`D65140`
   (native list membership + proxy binding). The texture proxy `SYSTEX_Menu_ProfileNN` stays the
   genuine CPU registration; its holder points at a native texture record (as the FaceGen target
   `AdmitComposedFace`).
2. Model: the step machine builds a `ChrAsmModelRes` from the save appearance (`D678C0`,
   `D67D50`: `C42230`, `C42620`, `D690A0`, `C3BA60`); its draw entity is the same family as the
   world asm parts, so the native selection path (`candidate.group=entity+0x240`,
   `animation=entity+0x3D0`) can collect it with the portrait's own camera.
3. Drawing: while listed, render the collected meshes through the native model pipeline into
   the w x h target with the camera of `D6B8D0` (`MenuOffscrRendParam`) and the light +0x50,
   clear colour (0,0,0,1), background 0x141414; the exact pass list comes from `D658B0`'s loaded
   bytes (Native1160).
4. Witnesses: one per family member (Prof via Load Game, Player via Status, Face via character
   creation), compared with the original.

## Native1161: the Status screen faults natively (same family)

In game, Esc then the Status icon: access violation at `0C3475` (old scene factory), stack
`B714F5` (PlayerStatusMenu ctor `B71320`) -> `D6A41C` (`CSMenuPlayerModelRend` ctor) -> `D660D9`
(`D65DD0`) -> `D64E44` (`D64C30`) -> `0C3475`. The player view is built when the Status screen
opens, so the native backend must replace `D64C30` for the whole family before Status,
character creation and Load Game portraits can work. Restoration verified, Thomas's save intact.

## Native1162-1163: Status opens natively (offscreen image omitted)

`D64C30` is replaced by `NativeOffscreenSceneGraph` (genuine CPU views, scene and light; GX
scene, layers and composite scene nil; no old-device clear or callback). Native1162: the 560x768
StatusBody construction passes, then the UI import of its proxy (CSOffscreenGxTexture, vtable
`14295DA78`) refused (no native image). `NativeUiTextureImport` now imports `SYSTEX_BLACK` for
such a proxy (explicit visual omission). Native1163: the Status screen opens and works (detail
view shown), two offscreen scenes built (560x768 StatusBody, 512x256), restoration verified.
Still open in this family: the native offscreen draw (character views), Load Game portraits
(their construction can now be enabled), character creation.

## Native1164: genuine portrait construction (reverted)

With `NativeMenuProfilesStartup` removed, `AE5B70` built the ten portraits natively through
`NativeOffscreenSceneGraph` (ten 128x128 scenes, `ADEF50` skipped none, the Load Game list showed
ten black placeholders). On window close an access violation followed at `D12759` (rcx 0), stack
`D1904E` -> `D1AA8F` (`D1AA60`, updatable object vt+0x18) -> `D1B1F7` (`D1B190` update list) ->
task `23C1DA5`, during shutdown (SFX records retiring). The portrait models' update path needs the
native offscreen owner beyond the CPU graph (D19000 reads +0x38/+0x58 of its argument). The change
was reverted: portraits stay omitted until the native offscreen draw owns these objects.

## Native1165-1167: native targets, model link, scene and layer

- 1165 (d7ceb833): each named proxy gets a native w x h R8G8B8A8 target (`TextureIngress::AdmitOffscreenTarget`,
  cleared to the scene background 0x141414, shared with the UI image ingress through a same-sized CPU image).
- 1166 (0daedb17): `C3B810(model, holder)` is hooked to map holder -> `ChrAsmModel` (vtable `142924A20`); the
  view camera is the holder's scene object +0x30 (pose +0x10, fov/aspect/near/far +0x50..+0x5C; Status 40 deg).
- 1167: the holder's CPU scene +0x38 (`4FDB0` + `4FEC0`, the same world class as the native main view) and flat
  layer +0x40 (`C2400`) are built genuinely, the light +0x50 is the layer's registration context (`D03AC0`), and
  the menu model's parts register in the layer (`C24E0`, node table `0x295B690`, caller `0xDFDB28`). Status stable.

Next: read the drawable `0x295B690` (I-0067 static request) to admit it in the native selection, then render the
selection with the holder camera into the holder's native target (a w x h lit pass: second target set in the
scene renderer).

## Native1168-1169: native draw of the menu views

- 1168 (e845c30a): `C3AD90(model, context)` witness: the ChrAsmModel's parts (model +0x28 + 8i, 22 for StatusBody,
  14 for StatusFace) have drawables `[part+0x18]` of class `SprjModelDispEntity` (`0x29489A0`, the map-piece class the
  native selection already admits); the renderer gained a per-size target set (`RenderNativeOffscreen`).
- Listing (witness hooks `D64B50`/`D65140`): Init_Play `D677D0` (caller `0xD67891`) lists both views once the parts
  are loaded; the next step `D676D0` (`0xD676EE`) unlists them on its first call. The original draws each view during
  that single listed frame and keeps the image (I-0067 MENUS-MAP 24). The renderer destructor `D66280` unlists again
  (`0xD66346`) before freeing the holder.
- Native policy (explicit, differs from the original's single draw): a holder seen listed is armed and its view is
  selected every frame on the native scene CPU update (right after the main selection, `CollectNativeOffscreenScenes`)
  and drawn at presentation before the UI (`RenderNativeOffscreenViews`), until `D66280` retires it (models, proxies,
  scenes erased). Rationale: the first listed frame can precede the first pose (Native1169: nonfinite item placement,
  now waited for) and resident textures.
- Camera: the holder view +0x28 is a camera world matrix at +0 (rows right, up, forward, eye: StatusBody eye
  (0.380, 1.636, -2.513), matching MENUS-MAP 21) and fov/near/far/aspect at +0x50; the lens words +0x68/+0x70 are not
  the main camera output's projection flags/tile count (+0x68 = 584.0f), so they are zero for the menu perspective.
- Decals: the menu parts carry an instance decal provider (blood) whose contract is not owned; they take the same
  neutral decal (t41/t42 absent decal) as the world character's ASM parts (without it t41 had no binding and only the
  eyes were drawn).
- Result (Native1169 `i66-native-status-lens-1169`): StatusBody 30/30 meshes drawn (19 deferred + 11 forward), StatusFace
  38/38 (24 + 14), 0 debug errors, Status opened, closed and reopened, views retired, restoration verified. Image witness
  `offscreen-560x768-20.png` (raw RGBA readback of the view target on its 20th draw).

Open in this family: the view background (the lit HDR composition makes it black, the original target is cleared
to 0x141414 and the scene composed over it), the menu's own light/fog/tone map (gparam zone 1 entry of the view row,
MENUS-MAP 22: shadow entry {1, id}), the visible Status page that shows these views (keyboard key for "Switch" not
found by the pilot), Load Game portraits (construction faults at shutdown, Native1164), character creation.

## Native1169 (continued): menu inputs, background, on-screen Status page

- Menu part inputs (MENUS-MAP 26): per-item draw colour `item+0x780` replaces group `+0x40..+0x4F` in PS cbModelParam
  (FC_SkinMatchColor; D3DA20 copies it before each draw), FaceGen owner `[item+0x770]` for the `FG_face_gen` slot,
  neutral decal; menu parts are character parts (`neutral_instance_decal` also drives the 1103 material class union).
- Model release: `D68EB0` (renderer model objects free; face views call it after their single draw, the destructor too)
  is hooked to disarm the holder before its model goes; the view's last image stays in its target.
- Background: a view without the GX tone map's table has its target cleared to the view background, then the lit HDR
  composed only where the scene depth is below 1 (`OffscreenCompositionPipeline`, depth GREATER at z = 1). Since I-0113
  tranche 29 a tabled view (the GX tone map) has no clear: it draws the original's background quad (the BG Color squared, 18/255 to 1/255, FUN_140112b40) into its lit
  HDR where the scene depth is 1, before its forward rows, and composes over every pixel (I-0113 RESULT « Tranche 29 »). The background is the Setting Info
  `BG Color` of the view key `{D27770(1) = 1, id}` (`D64C30` argument 6: Status body 11, face 13) through `15EC10` /
  `15E920` on `[GXRS+0x4610]`. Native1169 witness: the bank exists but does not contain `{1, 11}` / `{1, 13}`
  (`bank_state` 1), so the set-up constant 0x141414 is used; same for the light/fog key: area 1 (`default.gparam`)
  seems absent from the native GX banks (to investigate: who fills them, I-0067 MENUS-MAP 27).
- UI: the primitive UI pass uploaded the imported image's CPU bytes (zero) instead of sampling the shared native
  resource. `PrimitiveTexture::external` now carries a native render target (records with `render_target`, only the
  menu offscreen targets) that `PrimitivePass::Build` binds directly; the UI session's texture dedup includes it.
- Pilot: in the Status screen the "Switch" page (face + body) opens with F (`0x46`) held 600 ms (shorter presses are
  lost at native frame rates); scratch pilot `StatusCycle.ps1` retries R/F until the screenshot changes.
- Result: Native1169 `i66-native-status-shared2-1169/status-page.jpg`: the Status page shows the native face portrait
  and the native full body; Status closed and reopened, views retired, restoration verified, no fault.
- Bank witness (`i66-native-status-banks-1169`, `native_gparam_bank_keys`): natively `15EC10` (Setting Info
  contains) is false for every `{D27770(0..13), 0..15}` key, while the LightSet resolver `32770` returns a record for
  every key including unknown areas (its fallback), so the menu's own Setting Info / LightSet rows are not
  distinguishable natively yet. Open: compare the Setting Info bank and the `{1, 11}` LightSet record with the
  original (Reference run) before choosing; the Status views keep the 0x141414 background and the resolver's light.

## Native1170: Load Game portraits

- `AE5B70` now runs genuinely (the omission `NativeMenuProfilesStartup` is removed): ten 128x128 Prof views (gparam 14).
- Native1170 `i66-native-loadgame-portraits-1170`: the Native1164 shutdown fault (`D12759`) no longer occurs with the
  native view lifetime (armed views, `D68EB0` model release, `D66280` retirement): window close exits 0, no fault,
  no refusal. The portraits stayed empty because the views were only selected on the native scene CPU update, which
  does not run at the title.
- Menus outside the world: `RenderNativeOffscreenViews` selects the armed views itself when no scene update selected
  them since the last presentation, only on the menu thread (witness `native_offscreen_threads`: menu = presentation =
  same thread, so no menu teardown can interleave).
- Result (`i66-native-loadgame-portraits2-1170/loadgame-2.jpg`): the Load Game list shows the native portraits (Alpha
  with its hood, the new characters' bare heads), clean exit, restoration verified.

## Native1171-1172: character creation

- Thomas's save (fewer than 10 profiles; the test save is full) is used with the runner's hash-checked restore; the
  creation is never confirmed. Title flow: press-any-button click, quit-notice OK (648, 472), New Game (648, 586).
- New Game first runs the opening movie step `EAF7E0`, whose sole factory `141ADE670` builds a player initialised with
  the old device `[*144961AB0]` (Native1171: access violation `EAF8F6`). Explicit omission: `NativeMoviePlayerFactory`
  returns no player (the original's own "no player" path); the movie texture `CSMovieGxTexture` (vtable `29A4968`,
  RTTI) imports `SYSTEX_BLACK` like the offscreen proxies.
- UI: a shape style with word `+0xC` set (character creation menu) was refused by the native mesh writer; the genuine
  generator `1C3AB10` now runs for it, with the output-format check kept.
- Result (`i66-native-charmake8-1171/c5.jpg`): Create Character shows the native preview (Knight, 4 Player views
  784x784 gparam 11: equipped/nude x 2, 26 + 1 and 23 + 14 draws).
- Open (Native1172): closing the window while Create Character is open faults natively at `D12759` (`D126B0` reads
  `[[rdi+0x58]+0x18]` = 0; stack `D1904E` (`D19000` skeleton/pose update) <- `D1AA8F` (`D1AA60` updatable) <- `D1B1F7`
  <- task `23C1DA5`), after the SFX records retire; the views were not retired before. Reference1172 (same path) exits
  0, so the fault is native-specific. Same site as Native1164 (portraits at shutdown, which no longer fault).
- Static (Native1172 fault): the faulting object (`rdi`) is `NS_SPRJ::CSFD4LocationBodyScaleModifier` (vtable
  `294B458`, RTTI; ctor `D12370`, dtor `D123F0`), created by the menu model setup `D690A0` (call `D69448`) with
  `+0x58 = [D225A0([144791E08], L"c0000") + 0x68] + 200` (the c0000 character resource's body-scale data), `+0x60 =
  renderer +0x844`; it is registered on the ChrAsm (`D12C90`, `C4D350`) and stored at renderer `+0x870`, released by
  `D68EB0`. At the native shutdown the c0000 resource data (`+0x18`) is already gone while the Create Character
  renderers are still alive (no `D66280`/`D68EB0` before the fault), and the update list (`D1B190` -> `D1AA60` ->
  `D19000` -> `D126B0`) still runs. Reference exits cleanly on the same path. Next (RCA): compare the shutdown order
  (menu teardown vs c0000 resource release vs update-task stop) between Reference and Native with witnesses on
  `D123F0`, `D68EB0`, the c0000 resource release and the last `D1B190` call.
- Teardown witnesses (both modes: `D66280`, `D68EB0`, `D123F0`, `8190B0` enqueue, `EE42B0` flush), Reference1172 vs
  Native1172 (`i66-reference-teardown-1172`, `i66-native-deferred-1172`): the DelayDeleteStep flush `EE42B0` runs every
  native frame. In Reference the ten `CSMenuProfModelRend` portraits are destroyed at New Game (the
  `AppMenu::TitleTopDialog` destructor `AD9820` -> `AED060` -> `8190B0`, deleted on the 7th flush). Natively
  `TitleTopDialog` survives until the shutdown (its `AED060` enqueue happens then, with the Create Character views from
  `A85960`), so the portraits and their body-scale modifiers stay alive during the whole creation (INFERRED cause of
  the shutdown fault). The only native difference on this path is the omitted movie player (`EAF7E0` "no player"
  path: `+0x29`/`+0x14A` cleared, step 1 = `EAFC00` idle). Movie step table (`142692DA0`): 0 `EAF7B0`, 1 `EAFC00`
  idle, 2 `EAF7E0` open, 3 `EAFC50`, 4 `EAF720`, 5 `EAF7D0`, 6 `EAFAC0`, 7 `EAF670` stop, 8 `EAF660`. Open: who
  destroys TitleTopDialog at New Game in the original and how it depends on the movie (static request sent).
- Experiment (Native1172 `i66-native-movieend-1172`, reverted): setting the movie's `+0x149` (the stop step's
  "ended" flag) after the no-player open step does not destroy TitleTopDialog; the fault remains.
- Reference1172 `i66-reference-titleorder-1172` (timestamps): the movie player is created at New Game (tick
  597649625, caller `EAF8D4`) and `TitleTopDialog::~` runs about 45 s later (tick 597694500, caller `ADE094`), i.e.
  when the opening movie ends; the ten portraits are enqueued at once and destroyed 109 ms later. So the original
  closes the title when the movie finishes playing; the native omission never reaches that end. Next: find the
  movie-end signal that the title/New Game flow waits for (caller `ADE094` = `ADE080`, TitleTopDialog deleting
  destructor; who calls it) and produce it natively for the omitted movie (I-0067 static request in progress).
- **Corrected (1173): the shutdown fault is the original game's own, not a native defect.** Witnesses in both modes
  (pass-through: AppMenu close `A4E7C0`, TitleStep states `90A4A0`/`90A520` and destructor `909EB0`, the title
  destructor's stack, the body-scale modifier update `D126B0`; pilot `Pilot-CharMakeWitness.ps1`, trace-driven):
  - TitleStep runs states 0, 1, 2, then 9 (`90B570`: window stack and menu jobs) in both modes; it leaves 9 for 10
    (`90AFB0`, exit) only when `DAT_14489D5B8` is set, and that byte is the main loop's quit request (`EDAAE0`:
    `D5B8 |= !window->vt+0x10()`). The title dialog dies with the step (`909EB0` -> holder release `B9E360` ->
    `BB4A30`/`BB6360` -> `927A20` -> `ADE080`), so in both modes it lives until the window is closed. The "45 s after
    New Game" of Reference1172 was the pilot's window close, not the movie end (the original's opening movie lasts
    about 200 s and cannot be skipped by click, Enter or E).
  - Create Character opens inside TitleStep state 9 in both modes (the title's job sequence after MovieScreen,
    MENUS-MAP 40); natively it opens at once because the movie is omitted.
  - Closing the window with Create Character open faults in the original too: `i66-reference-charmakeclose-1173`
    (no `D126B0` witness) and `i66-reference-bodyscale-1173` exit 0xC0000005 at `D12759` with the same stack as
    Native1172/`i66-native-bodyscale-1173` (`D1904E` <- `D1AA8F` <- `D1B1F7` <- `23C1DA5` <- `EDA9BF`). In both modes the
    four ChrMake Player views' modifiers read a valid c0000 table until `TitleTopDialog::~`, and 15 ms (original) or
    141 ms (native) later `[[modifier+0x58]+0x18]` is 0 while the location update still runs; the queued views are
    never destroyed before the step shutdown. Reference1172 closed during the movie, before any creation view existed,
    hence the apparent difference. Native parity: no fix (a native-only divergence would not be the original's
    behaviour). Superseded (2026-11-13): addressed natively as a fault of the original's, I-0137 tranches 4, 7 and 8 (three readers of
    the menus' models gated on DS3's quit request; clean witnesses of both forms, `i66-native-i137-t8-charmake`,
    build p10b, a first form, and `i66-native-i137-t8d-charmake`, the merged form). The quit byte's full expression:
    `D5B8 |= !(window->vt+0x10() && !runtime->vt+0x98())` (I-0137 RESULT « Tranche 8 »).

## Native1174: menu views take their gparam (background, lights, fog)

- Cause (measured, `native_light_blocks`): natively the engine's GX parameter banks never receive a file. `D272C0` is
  `AdmitGparam` (so `D275F0`'s auto-apply of the resident files never runs) and `49D00` is `BindNativeGparamScene`
  (bookkeeping only), so `32770`/`38E70` return their built-in default record for every key: sun0 (0.7071, 0.7071, 0,
  50), fog0 (1, 0, 0, 0.01) for the menu key {1, 11}, {1, 0} and the m40 keys {140000000, 100/110/300} alike. The
  Setting Info bank `[GXRS+4610]` is empty too (`bank_state` 1 in 1169).
- Native (family: resident gparam area, MENUS-MAP 39): `NativeGparamRecords` builds the lookup-layout records (light
  set at +0x40, fog set at +0x10) from the admitted files (`ds3light::Banks::Sun/Fog`, the I-0045/I-0063 records
  qualified against the engine's) for keys of area 1 and {-1, -1} -> {1, 0}, with the engine's absent-id rule ({1, 0});
  the engine builders `1496D0`/`14A2D0`/`14E900`/`14E5D0` then run unchanged. The view background is the file's
  Setting Info "BG Color" of the exact key (`Banks::BackgroundColor`, like `15EC10`), used as the target's optimized
  clear value too (fixed per view; a mismatching clear value is a debug-layer error, `i66-native-menugparam-1174`).
- Result (`i66-native-menugparam2-1174`, Reference `i66-reference-status-1174`): Status body background 18/255 (id 11),
  face 22/255 (id 13); the body view is lit by LightSet id 11 like the original
  (`i66-native-menugparam2-1174/status-body-origine-vs-natif.jpg`). Remaining visible difference: the original's
  shadow under the hood (Player views have the shadow sub-scene, MENUS-MAP 22/25), and no view tone map [historical; written since I-0113 tranche 27 for modes 0 and 7, seen live in mode 0 (I-0113 tranche 27 after the merge: the Quit Game portrait's face matched, mode 7 not yet seen)] (GXToneMapFilter,
  TONEMAP-SKIN 17).
- Open (lighting family, not menus): the world's map keys keep the engine lookup, so the world is lit by the bank
  default natively; building map records needs the map's MSB MapOffset yaw (`Banks::Sun` rotation), which the native
  banks do not carry yet.
## Native1175-1179: sound, portraits (FaceGen, hair), world gparam, camera byte, phantom witness attempts

- Menu sound (1175): Thomas hears the main menu music and the menu sound effects natively; witness
  `sound_event_request` (4AD510, both modes): the MenuMan sound object [144763868] exists natively and receives the
  music (category 6, id 1) and the cursor/decide requests (category 2, ids 100/0/1010/999999999). ROADMAP item met.
- Face grid (1175): "Face Presets" is a list of ten presets (no 128 views); Appearance opens Gender/Age/Voice/...; the
  grid is Appearance -> Face Detail -> Similar Face (MENUS-MAP 49, FUN_140B1AA00 -> B29580). Not yet witnessed natively.
- Load Game portraits (1176-1177): the part mesh masks (C4D160 -> drawable +2B0/+2B8, the capability words) are equal in
  both modes (`i66-*-meshmask-1176`). The faces lacked their FaceGen shape: D3DA20 passes `[item+758]` as descriptor
  +28, which is the part's SprjFgModelVertexStreamPatch (measured: `[item+758]` is the D3F7C0 receiver of the part's model;
  `i66-native-streamrefs-1177`); menu parts now bind that patch's current streams (`native_menu_face_stream owned`), the
  portrait faces match the original's shape (`i66-native-facestream4-1177/beta-origine-avant-apres.png`).
- Hair and brows of portraits (open): the forward SAT_HairIbl2 draws are prepared and drawn natively with their real
  albedo textures (HR_A_0000_a, FG_A_2012_a, FG_A_7002_a, BC7 sRGB; the head's own strands sample the FLVER placeholder
  `FC_M_0000_hairoff_a`), with FC_AlphaRef (1/255, `_al` variant), instance colour row 7 = (1, 1, 1, 1) in both modes
  (`menu_item_draw`, Reference1179) and item+6F2 = 0 in both modes (the descriptor texture override is not used);
  drawn opaque (diagnostic 1177, reverted) they show dark hair and brows inside the scalp silhouette, blended they leave
  no trace. Two native defects remain: the output alpha of these draws is near zero, and the view composition keeps only
  pixels where the deferred depth is below 1 (forward-only pixels beyond the silhouette are dropped; for the tabled views no
  longer since I-0113 tranche 29, which left the Quit Game portrait's hair as dark as before). Programmes dumped:
  `i66-native-hairps-1177/hair-forward-ps.dxbc`, `i66-native-hairvs-1179/hair-forward-vs.dxbc` (I-0067 MENUS-MAP 44, 46).
- World lighting (1178): the map keys use the admitted map gparam files too (MENUS-MAP 43: no map rotation); the world
  takes the map's light set and fog (`i66-native-worldgparam-1178/origine-avant-apres.jpg`); brightness still differs
  (no YEBIS tone map; MENUS-MAP gamma pass).
- Camera (1179): the High Wall load (Argo) refused natively in BuildSceneView because the camera output's +0x68 was read
  as a word; 14004B5C0 writes only its byte (+0x69..+0x6B are stack padding of the D1DE70 local, I-0067 static); read as
  a byte now (`native/frame/scene_ingress.cpp`).
- Phantom witness (1179, blocked): Thomas's save (Argo, High Wall, in front of an NPC summon sign) is selected in Load
  Game natively, but the "Start with selected profile?" MessageBox (01_010_MessageBox) does not take a click on OK, E,
  Enter or Space reliably natively (one load out of eight attempts, `i66-native-argo2-1179`); the quit notice's OK does
  take a click. Open: native UI input on this MessageBox (I-0067 static reading in progress), then the white phantom
  (PhantomParam 80) witness. Pilot `Pilot-LoadGameThomas.ps1` (trace-driven, screenshots per confirmation attempt).
- Face grid (1180, `i66-native-facegrid5-1180/grid-*.jpg`): Appearance -> Face Detail -> Similar Face opens
  FaceSelectDialog natively and its 128x128 ChrMakeGrid views fill in progressively (staggered starts, FaceGen per view);
  five of fifteen shown 25 s after opening at the native frame rate, all without hair (same hair defect as the portraits).
- Native UI input (1180): at 7 fps, clicks are unreliable on the "Start with selected profile?" MessageBox (plain clicks,
  E, Enter, Space and 400 ms held presses all failed in `i66-native-argo6..9`); the same page's quit notice takes clicks
  after retries. Open: native pointer/button sampling against the native frame cadence (A60700 per frame), before the
  phantom witness.

## Native1181: review fixes, load confirmation, summon place

- Pre-merge review (fresh agent, 2026-10-10): no blocker. Applied: the offscreen `native_selected_draw_preparation`
  rows are bounded (12, then every 600th); `SceneRenderer::offscreen_sets_` is cleared in RetireSceneResources and Close
  and its resources are detached in the quarantine branch of the destructor; `CollectNativeOffscreenScenes` runs one
  collection at a time with a non-blocking lock (the scene CPU update holds `scene_view_cpu_lock`, the presentation the
  submission lock; a skipped collection keeps the previous frame's views). Signatures checked by the review against the
  decompile: 90A4A0/90A520 (float in XMM1, R8), 909EB0 (one-parameter destructor), 1ADE670 (no argument).
- Mouse unit (I-0067 MENUS-MAP 51): the UI hit test reads the point in the configured resolution S (image+46601E8/+46601EC),
  which the original's display+198/19C carries; `NativeMouseDeviceUpdate` now scales the client cursor to S (output size as
  fallback). Measured S = 1280x720 = output on this PC (`native_mouse_position_owned`, `i66-native-mouse-1181`): no
  numeric change here, the contract matters for other configured resolutions.
- Load confirmation (`i66-native-mouse-1181`, `i66-native-summon-1181`): the row press then a 400 ms press on OK loads Argo
  natively (TitleStep state 4 with new_game_mode 1 one or two presses after the row); the 1180 pilot misread it because
  TitleStep's destructor only runs once the area is loaded (pilot now waits for state 4).
- Summon place (Argo, High Wall, Thomas's save): neither the original (`i66-reference-summon-1181/summon-*.jpg`) nor native
  shows a summon sign or a "Summon" prompt where Argo stands; E does nothing in both modes. Hypothesis (not checked): NPC
  signs need the Embered state. The phantom witness needs Thomas's indication (ember or another place).
- Divergence seen in passing (`i66-reference-summon-1181/summon-1.jpg` against `i66-native-summon-1181/summon-1.jpg`, same
  camera): the original draws a large pale corpse-like object with chains and a lantern in front of Argo, and the torch's
  light and flame on the left wall; native draws neither. Not diagnosed (world render scope, not menus): native pass
  refusals on that frame are gates 18 (156), 21 (2699), 30 (4).
- Menu icons: the 17 icon sheets (01_Common.tpf, BC7 2048 wide) are resident natively (`native_encoded_texture_resident`
  2048-wide format 98 rows, 944 `native_ui_asset_image_imported` rows in `i66-native-summon-1181`), and the Status page
  shows the same icons as the original (`i66-native-menugparam2-1174/status-1a.jpg` against
  `i66-reference-status-1174/status-1a.jpg`). Closed.
- Hair of the portraits, re-measured on Argo (Thomas's save; earlier hair diagnostics used hooded test-save characters
  and were inconclusive). Excluded: `VC_TexScroll` (VS b11) = (1,1,0,0) (`native_offscreen_tex_scroll`,
  `i66-native-texscroll-1181`); FC_AlphaRef 0.0039; UV words of the hair meshes in [16, 2040] / 2048
  (`native_hair_uv_words`, `i66-native-hairuv2-1181`, slot 1 for the FaceGen brows); the authored albedo maps have real
  alpha (offline BC7 endpoint read of the dumped DDS `i66-native-hairtex-1181/tex-*.dds`, `i66-native-hairuv2-1181`:
  HR_A_0106_a 58 % of blocks reach alpha 224+, FG_A_2012_a brows 8 % non-transparent, FC_F_0000_hairy_a 82 % below 16).
  Found: Argo's view holds face, head (skin and the seven "hairy" shell meshes on FC_F_0000_hairy_a), two equipment
  models, eyes and two brow models, but no hair model, although HR_A_0106_a is admitted; five part models (vtable rva
  2950E18) are refused with source 0 / storage 0 (`native_asm_part_missing_inputs`, site 1). Open (I-0067 question
  sent): where a menu part model's FLVER comes from when that source is null, and what the original binds on the head's
  hairy shell (draw context +0x1B0). Brows are drawn with their real map and UVs but stay invisible (open).
  Follow-up (`i66-native-hairmodel-1181`, witness not kept): no child of the menu view's SprjModelDispEntity is skipped
  before admission (class, model, instance, transforms all present). The head's texture choice row reads item+6F2 = 0,
  [model+358]/[+360] set but not native textures, FrpgFgTextureIns+91 = 1 and its texture set +30 = 0 natively (the
  native composition admits the target +28 only). Leading reading (INFERRED, not verified): under the headwrap the
  original draws no hair model; the head's hairy shell takes a texture from the FaceGen texture set (+30, through draw
  record +1D0 / draw context +1B0) in place of the FLVER placeholder, and that set is absent natively.
  Corrected (same day): CFDE70's texture set +30 (SprjFgMdlTextureSet, 51B5C0) only replaces the FG_face_gen slots
  (LEGACY-ROOT-INVENTORY-903, 1113), which native binds from the composed target; the hairy shell keeps its FLVER map in
  the original too, so Argo's black fringe comes from the hair model. `i66-native-menuclasses-1181` (witness not kept):
  the menu views visit one entity class only (SprjModelDispEntity, 29489A0); the five refused part models have model
  vtable 2950E18 (not the FLVER model class 2951018) and no storage. Open: where the original's hair model of a menu view
  comes from (I-0067 question). That run's game exited more than 20 s after its controller, so the runner stopped before
  restoring; the protected files were restored by hand with the runner's own procedure (`restoration.json`, note field;
  the save never changed, GraphicsConfig.xml rewritten by the game was restored).

## Native1182: the alpha-tested forward row (hair)

- Measured (`i66-native-slotbuild-1181`, `i66-reference-slotbuild-1181`, C3CA10 witness not kept): Argo's hair slot 6
  (HR_A_0106) is built with its real FLVER in both modes; the two empty placeholder parts (FG_A_5000/6000, slots 22/23)
  are the same in the original. The part masks are identical in both modes.
- Cause (`i66-native-hairgate2-1181`, `native_offscreen_hair_refused`): HR_A_0106's meshes have material class 0x80
  (ClassifyModelMaterial: forward and not transparent, P[ChrCustom_Hair] blend 0, alpha ref 128) and model +220 = 0x82;
  native evaluated only 57A20's transparent forward row (0x29, class mask 0x12). 57A20 also adds, for a model with +224
  bit 12, the row (engine pass 0x1F, preparation 0, class mask 0x80) (`decompiled/DS3-1152-F-00057A20.c`).
- Fix: `EvaluateOrdinaryModelPass` accepts the forward row 0x1F with class mask 0x80; `PrepareNativeSelectedPasses`
  adds it ahead of 0x29 for models with +224 bit 12; the draws take the forward state (explicit open policy: depth tested,
  not written; the original writes it). Result `i66-native-hairpass-1181/argo-cheveux-origine-avant-apres.png`: Argo's
  black hair is drawn in the Load Game portrait as in the original; in game (High Wall) no visible change
  (`ingame-1.jpg`); controls pass. Open: brows stay fainter than the original; hair depth not written.
- Phantom witness (I-0067 MENUS-MAP 59): the NPC sign is about 40 m from Argo, near Vordt's arena (region 3002780), and
  needs the Embered state (an Ember) within 10 m; not piloted yet.
- Pre-merge review of the Native1181-1182 delta (fresh agent): no blocker. Kept open: 0x1F depth not written (needs the
  depth view in DEPTH_WRITE around those draws); the 0x1F row also applies in the world (High Wall checked, no visible
  change); NativeUiMovieViewport still declares the output size while the cursor is in configured-resolution units (equal
  on this PC; to verify with a configured resolution different from the output); no unit test covers the forward rows of
  EvaluateOrdinaryModelPass. The collection lock was made file-local.
