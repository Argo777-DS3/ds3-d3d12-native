# I-0068 part AQ: the ground summon signs through the effects system, and the general native card by effect family (STATIC)

Request (coordinator, 2026-10-11): trace statically how the ground summon signs (open at the phantoms workstream:
[PHANTOMS-NATIVE](../I-0066-independent-native-startup/PHANTOMS-NATIVE.md),
[PHANTOMS-SIGNS-NATIVE](../I-0067-static-menus-subsystems-map/PHANTOMS-SIGNS-NATIVE.md) §6, P12–P14, PL8–PL9) are rendered:
FXR, emission, materials and shaders, entry through the `E6DCD0` door; say what native lacks; write one general native card by
effect family with witnesses. Static only: the game was not started; nothing native was edited.

Labels: STATIC (decompiled code, FXR bytes, DXBC), INFERRED, OBSERVED (earlier live captures, cited), OPEN.

## 0. Answer first

1. **A sign is three small effect layers, no material and no model.** The id the manager creates (`30001`…) is a controller: one
   `NodeSound` (sound id 30001 summon, 30011 invasion) and a proxy that spawns the visible effect `3000x2`. That effect holds
   **five E01 billboards, one E04 distortion and one E06 point light** (six billboards on the red/hostile variants). The paired
   effect `3000x0` is two more E01 billboards (a ground glow) plus a proxy to the same `3000x2`. All of it is the effect system:
   nothing in a sign uses `gxflvershader` or a FLVER (§1, §2).
2. **Native has none of it today.** `7CB820` is replaced by `DeferNativeSfxSceneRequest` (`game_bootstrap.cpp:7514`, hook `:9394`): it
   stores the request and the FXR asset (`sfx_assets[identifier]`, so the data is already loaded) and returns the genuine
   *empty* CPU handle (`handle+0x18 == 0`); no instance, no emission, no draw, no light, no sound. The sign handle at `sign+0x10`
   therefore stays an empty handle forever (§4).
3. **The sign needs no new family beyond the checklist.** It is exactly E01 (tessellated billboards, soft + glow twins, additive and
   subtractive blends), E04 (`distortionBump`, frame copy), E06 (one point light) and the CPU emission actions. Ordered work:
   [EFFECTS-IMPLEMENTATION-CHECKLIST](EFFECTS-IMPLEMENTATION-CHECKLIST.md) 1.1–1.4, 2.1, 3.1–3.3, 4, 6.1–6.2 (§5).
4. **Corrections to earlier notes (§3):** (a) `ctx+0x960` is **not** written by anything that reads the FXR blend mode in the
   classic path: it is written by the plain setter in draw-context slot `+0x60` (`1402607D0`, which Ghidra never defined as a function,
   so every earlier register search missed it) and by `FUN_1402607E0` (GPU-particle callbacks only); (b) the FXR `BlendMode` → blend type
   table is `FUN_141ACABF0`: **`Unk0` (value 0) resolves to additive**, not to alpha; (c) the controller carries a sound, it is not
   silent; (d) 30000 is not "role OPEN": it is a ground glow with a proxy to 30002.
5. **One point stays open, with a one-read live check (SG2):** how the resolved blend type becomes `ctx+0x960` for classic draws
   (§3.3). Native must not guess it; the safe baseline and the decisive check are given.

## 1. The sign, traced from the request to the pixels

| Stage | What happens (STATIC) | Native today |
|---|---|---|
| Record → id | `SosSignMan` (`142847368`) `853800`: id `variant*10000 + category` (PHANTOMS-SIGNS-NATIVE §6); the High Wall NPC signs use 30001 (INFERRED) | not reached: the sign sources (`2003[25]`) and the manager are engine CPU code and run; the id reaches `7CB820` |
| Request | `7CB820(handle, id, transform, 8)` → creators `E6DCD0` / `E6DA30` / `E6DBC0` (EFFECTS-ENTRY-DOOR-NATIVE): with `[SprjSfxImp+0x58]` (the scene control) non-null they create the instance; null → empty handle | **null branch forced by the deferral**: empty handle, request kept in `sfx/latest-deferred-request` |
| Resolve | the FXR is looked up by id in the registry (`FUN_14024ECF0`, hash of the id) and parsed; its GPU side is created by `GXFfxGraphicsResourceManager` slot 1 (`FUN_14036FD70` → `FUN_14036F9E0`: textures `GXFfxTexture`, `GXFfxBumpTexture`, …) | `sfx_assets` already holds the FXR bytes per id; no GPU side |
| Simulate | per-frame SceneCtrl update (`24D000`): emitters, spreads, modifiers, accelerations, LOD, sound, proxy spawn (library child spawn through the proxy node, manager slots 5–7, creator `141ACCA20`); the light host creates the `GXPointLight` (`FUN_1402B2410` → `FUN_140266FC0`) | not built (checklist 1.1–1.3) |
| Draw | per entity, `FUN_1402997E0` records `{FUN_140299E40, entity, data}` into the pass list; the builders (`FUN_140385F70` tessellated, `FUN_140396F30` triangle list, `FUN_14039E900` `Draw(4)`) write the ring and call the draw context (`vtable 143D65688`, §3.1) whose slot 27 `FUN_140264230` picks the programme and states | not built (checklist 3.x, 4, 6) |

The effect files (`f0000300xx.fxr`, `frpg_sfxbnd_commoneffects_effect.ffxbnd`; textures in `frpg_sfxbnd_commoneffects_resource.ffxbnd`
as TPF) were decoded with the new `Read-Fxr.ps1`; the 18 dumps are in `evidence/I-0068/queries/signs/fxr-<id>.txt` (outside Git).

### 1.1 The six sign families of files (STATIC, FXR bytes)

| File | Content | Role |
|---|---|---|
| `30001`, `30021`, `30041` | root + one `NodeSound` (**sound 30001**) + proxy node 2001 → `SFXReference` 30002 / 30022 / 30042 | controller, **summon** |
| `30011`, `30031`, `30051` | the same with **sound 30011** and proxy → 30012 / 30032 / 30052 | controller, **hostile / invasion** (INFERRED from the pairing with the red visuals) |
| `30002`, `30022`, `30042` | LOD node (thresholds 1, 40, 1000, 1000, 1000; 5 children; the light sits in the first), 1 × 609, 1 × 607, 5 × 603 | visible sign |
| `30012`, `30032`, `30052` | the same plus a sixth 603 (`s11432`, Normal blend, black colour: a dark ring) and `s50907` as Add and Subtract | visible sign, hostile |
| `30000`, `30010`, `30020`, `30030`, `30040`, `30050` | two periodic emitters, two 603 (GlobalYaw, `s11636` flipbook Add and `s11622`), a proxy → the matching `3000x2` | paired ground glow |

Billboard colour (first key of `color1`, rgb): white (1, 1, 0.878) [30002] and (1, 1, 0.761) [30000]; red (0.773, 0.031, 0.031) [30012] and
(0.867, 0.031, 0.031) [30010]; gold (0.984, 0.745, 0) [30022, 30020]; magenta (0.957, 0.329, 0.988) [30042, 30040]; 30032 mixes red and gold,
30052 magenta and red. Per-node colours are in the dumps.

## 2. The visible sign `30002`, family by family (STATIC)

Every node is `2200` (basic) with `NodeAttributes` (`attachment = Parent`), `StaticNodeTransform` (the flat nodes carry the X rotation −90°),
a periodic or one-time emitter, a shape, a spread, `ParticleModifier`/`ParticleAttributes`. These are CPU actions of the engine system
(kept; the native system transcribes the construction and the update, checklist 1.1–1.3).

### 2.1 E01 billboards (action 603, five instances)

| Node | Orientation | Texture (format) | `blendMode` field → bank type (§3.2) | Soft (`depthBlend`) | Bloom | Size × life | Emission |
|---|---|---|---|---|---|---|---|
| #10 | CameraPlane | `s11004` 128² BC1 | Add → additive | no | yes (0.2) | 0.05 wide, 1.33 s, depthOffset −0.2 | periodic 0.4 s, 5 per emission, up to 20, box 1.2 × 0.2, speed 0.15, random turns (gravity −0.5) |
| #12 | LocalSouth (flat) | `s50907` 128 × 64 BC7 | **Subtract** → type 3 (INFERRED reverse subtract) | yes | no | 1.2 × 0.6, one emission, depthBias −1 | one-time |
| #13 | LocalSouth | `s11636` 1024² BC7, 8 × 8 = 64 frames, interpolated | 0 (`Unk0`) → **additive** | yes | yes (0.1) | 0.3→0.6 × 0.2→0.5, 5 s, frame offset 0→30 | periodic 0.3 s, up to 20, rectangle emitter |
| #14 | LocalSouth | `s50908` 256 × 128 BC7, 2 columns, 4 frames | 0 → additive | yes | yes | 1.4→2.3 × 0.6→0.85, 2.67 s, colours 2 and 3 set | periodic 0.9 s, up to 5 |
| #15 | LocalSouth | `s50907` | 0 → additive | yes | yes | 1.2→1.28 × 0.6→0.64, 2.67 s, delay 0.67 s | periodic 0.9 s, up to 5 |

On `30012`/`30032`/`30052` the blends differ per node (`s50907` Add and Subtract, `s50908` Normal, `s11432` Normal, `s11636` 0).

Engine side (STATIC; `FUN_140264230` = draw-context slot 27, FFX-FAMILIES-PHASE2 §2): billboards are **tessellated** draws (OBSERVED live in
I-0063: only `GXFfxtessellate*` programmes), 1 control point per particle, programme index 12–35 (`tessellate{,Soft}Texture`, bump excluded:
no normal map), the **soft** variant when `depthBlend` (INFERRED mapping of the community field name to `ctx+0xA38` / `bVar29`), the
**`…GlowD` twin (+75) when `bloom`** (two render targets: forward HDR and glow target 1). Flipbook (`columns`, `totalFrames`,
`frameIndex`, `interpolateFrames`) is per-particle CPU/vertex data written by the builder; no programme differs. The exact kind and index
per node come from the item factory (`item+0x40`, `item+0x38`); they were not enumerated (OPEN, SG1: read the programme names live).

Textures: **BC7 (DXGI 98) for four of the six** (`s11636`, `s11432`, `s50907`, `s50908`), BC1 for three (`s11004`, `s11622`, `s06000`).
`native/content/dds_resource.cpp` already accepts BC7 and DX10 headers (read only, not changed): no loader gap.

### 2.2 E04 distortion (action 607, one instance, node #11)

`mode = NormalMap`, `normalMap = s06000` (128² BC1), size 1.2→1.5 × 0.5→0.7, `intensity` 0 → 0.5 → 0 over the life, radius 2, flat (X −90°), `depthBias −1.5`,
periodic 1.2 s up to 3, no soft flag, no bloom. Engine side: kinds 0x20/0x21 → `GXFfxdistortion` VS + `distortionBump` PS (EFFECTS-E05-E04 §1),
**t0 = the frame copy of the scene colour**, t1 = the normal map, s1 wrap and s9 clamp, blend off or alpha, no glow twin. Needs checklist 6.1
(one copy per frame at the start of pass 0x29) before the first such draw.

### 2.3 E06 point light (action 609, one instance, node #4 under the LOD node)

Diffuse and specular (1, 1, 0.878) with the fourth component 0.0588, radius 2, jitter (0.01, 0.03, 0.01), flicker interval 0.8–3 s, flicker brightness 0.3,
fade-out 30 (frames, INFERRED), shadow darkness 0.8. No draw: the host registers a `GXPointLight` (`FUN_1402B2410` → `FUN_140266FC0`, EFFECT-LIGHTS §2.1) read by the
native point path (checklist 1.4). **OPEN (SG3):** the light sits under LOD child 0 whose first threshold is 1; whether that means "distance < 1" or "from 1 upward" is
not decided statically, and decides whether a sign casts light from afar.

### 2.4 Control actions

`300` periodic, `399` one-time, `400/402/404` point/rectangle/box shapes, `500/501` spreads, `1`/`55`/`84` accelerations, `128`/`129`/`130`/`131`,
`35`, `133` LOD, `132` SFXReference, `75` NodeSound, `199` state map, root `10100/10400/10500`: all CPU, handled by the engine system. **Sound** is the only
non-graphic output: `NodeSound` hands id 30001/30011 to the audio subsystem when the controller starts (not traced further).

## 3. New engine facts of this part (STATIC)

### 3.1 The draw context (`GXFfxDrawContext`, vtable `143D65688`), slot by slot

| Slot | Function | Role |
|---|---|---|
| `+0x08` | `140260280` | writes a 4×4 matrix into `ctx+0x140` and `+0x300` (view-projection pair) |
| `+0x10`, `+0x18` | `1402602E0`, `140260340` | the other matrix pairs (`+0x200/+0x3C0`; `+0x180/+0x340/+0x5E0`) |
| `+0x20`…`+0x38` | `1402603D0`, `1402603E0`, `1402603F0`, `140260400`, `140260410` | SRV pointer setters `ctx+0x968/+0x970/+0x980/+0x978`; `+0x40` falls back to the default texture `[+0xA70]` and sets `ctx+0x9BA` |
| `+0x48`, `+0x50` | `140260760`, `1402607A0` | t3 = scene depth `[GXRS+0x4E10]+0x48`; set t0–t2 in one call |
| **`+0x60`** | **`1402607D0`** | **`ctx+0x960 = edx`: the blend type** (plain setter) |
| `+0x68`…`+0xA0` | `140260830`, `140260CE0`, `140261170`, `140261600`, `140261AB0`, `140261F60`, `140262530`, `140262A70` | the eight appearance setups: `ctx+0x964` = draw kind, then the FXR property evaluation into the constant staging |
| `+0xA8`…`+0xC8` | `140263270`, `1402632F0`, `140263300`, `140263330`, `140263370` | fade and clip constants, table `144963F10` (4 × 0x40) |
| `+0xD0`, `+0xD8` | `140260440` (not read), `140264230` | **commit (programme, states, uploads)** is `+0xD8` |

The undefined functions were disassembled with the new `I0068Dis.java` (read-only; the disassembly lives only in memory).

### 3.2 FXR blend mode → blend type (`FUN_141ACABF0(value, default)`, STATIC)

| FXR `BlendMode` | Name | Type returned | Bank entry (FFX-FAMILIES-PHASE2 §2.4) |
|---|---|---|---|
| 0 | `Unk0` | the caller's default (**2** in the factory read) | `+0x20` SRC_ALPHA / ONE (additive) |
| 1 | `Source` | 0 | `+0x08` off |
| 2 | `Normal` | 1 | `+0x10` SRC_ALPHA / INV_SRC_ALPHA |
| 3 | `Multiply` | 4 | `+0x18` DEST_COLOR / ZERO |
| 4 | `Add` | 2 | `+0x20` additive |
| 5 | `Subtract` | 3 | `+0x28` SRC_ALPHA / ONE with op code 2 (INFERRED reverse subtract) |
| other | — | 0 | off |

The resource object's resolver (vtable `143D66198`, built by `FUN_14028F4D0`) wraps it: slot `+0x30` (`14028F800`): 6 → 1, 7 → 2, else the table above with default 2; slot `+0x20`
(`14028F840`): 6 or 7 → 6, else `table[(type != 0) ? 1 : 0]` read from `resource+0x118/+0x11C`; slot `+0x18` (`14028F7F0`) `(1 << n) | 2`.
**`Unk0` is additive in the engine** (the community schema calls it "seemingly identical to Add": consistent). Three of the five sign billboards on `30002` use value 0.

The GPU-particle callbacks (`FUN_1402695E0`, `FUN_140277C00`) take the effect block's int at `+0x228` (the same enum), map `v − 1`
(0 → 1, 6 → 0x34 …) and call `FUN_1402607E0` (0 → 0, 2 → 4, 3 → 2, 4 → 3, else 1): `Normal` → alpha, `Multiply` → multiply, `Add` → additive, `Subtract` → type 3, `Source` → off.
In that path `Unk0` gives alpha (value 0 → 1), unlike the classic table. It is E07 (GPU particles), not the signs.

### 3.3 The open point (OPEN, SG2)

In the item factory read (`FUN_140373980`, action 606; the 603 factory was not located, so its layout is INFERRED the same) the item stores `item+0x3C = resolver slot +0x30 (value, 2)` (the bank type of §3.2) and `item+0x44 = resolver slot +0x20 (value, 2)`; the tessellated builder
(`FUN_140385F70`, lines 1022 and 1080) tests `item+0x44 == 0` and passes it to draw-context slot `+0x60`, i.e. **`ctx+0x960 = item+0x44`**, and never reads `item+0x3C`.
`item+0x44` is `resource+0x118` (type 0) or `resource+0x11C` (type ≠ 0) unless the value is 6 or 7. Those two ints are copied by `FUN_14036FD70` from the create-info (`+0x0C`, `+0x10`) given by the
caller of resource-manager slot 1, which was not found statically (the registry code, `FUN_14024ECF0`, does not call it). If both ints were constants, every blended classic draw would share one
blend type, contradicting the additive, alpha and multiply states OBSERVED live (I-0063: ZERO, ONE, SRC_ALPHA, INV_SRC_ALPHA, DEST_COLOR). So one of two things holds, and only a read tells which:

- the two ints are per-resource and equal the blend types (then §3.2 stands as is), or
- `item+0x44` is rewritten per draw from `item+0x3C` somewhere not yet seen.

**Native baseline until SG2:** map the FXR value with §3.2 (this is what `item+0x3C` holds) and use it as the blend type for the draw; `Unk0` → additive. This reproduces the engine's
documented intent and is the only mapping the code states explicitly. **SG2 (decisive, read-only):** at the High Wall sign or any m40 flame, read `[GXFfxGraphicsResourceManager]+0x118/+0x11C` and the
draw context `+0x960` at the commit (`FUN_140264230`) for three different billboards (Add, Normal, value 0), and the bank pointer chosen.

## 4. What native lacks for the sign, exactly

| # | Missing | Where it is specified | Checklist |
|---|---|---|---|
| 1 | the effect system (`SprjSfxImp+0x58`, scene control `24B260`, device init `24C530`, per-frame `24D000`) | EFFECTS-PHASE1-NATIVE | 1.1, 1.2 |
| 2 | the three creators take the `+0x58` branch; the `7CB820` deferral replaced by the original call (requests deferred before the system exists are not replayed) | EFFECTS-ENTRY-DOOR-NATIVE | 1.3, 2.1 |
| 3 | emission, LOD, proxy child spawn, `NodeSound`: engine CPU code, runs once 1–2 exist | this note §2.4 | 1.2 |
| 4 | the E01 draw: 2 MB ring, shader set `291520` (158 entries per stage), resource manager slot 1, builders, PSO per programme (HS/DS pattern, input layout, blend, depth test-on write-off, raster), texture SRVs t0–t3, s9 wrap-linear | FFX-FAMILIES-PHASE2, EFFECTS-PHASE2-E01-E02-NATIVE | 3.1–3.3 |
| 5 | glow: the `…GlowD` twins (MRT) and the glow chain; 4 of the 5 sign billboards set `bloom` | EMISSIVE-GLOW-NATIVE | 4.1–4.4 |
| 6 | E04: one frame copy per frame and the distortion programmes | EFFECTS-E05-E04-NATIVE | 6.1, 6.2 |
| 7 | E06: the point light host and its registry entry | EFFECT-LIGHTS | 1.4 |
| 8 | the sound id handed to the audio subsystem | not traced (no native audio path read) | — |
| 9 | the blend-type baseline of §3.2 and the SG2 check | this note | 3.3 |

Nothing in this list needs the phantom `_Phantom_` programme set or the PhantomParam path: the sign is independent of P1–P11 and is gated by the effect system only.

## 5. The general native card by effect family

Common rules (apply to every row): construct and update with the engine's own CPU system (transcribe `24B260`, `24C530`, `24D000`, creators, hosts); replace only the GPU edge (ring, resource
creation, programme apply, states, the four direct D3D11 sites of FFX-FAMILIES-PHASE2 §4); never read `[144961AB0]` from transcribed paths (checklist 1.5); upload the five context blocks as they are.

| Family (FXR actions) | Engine path (STATIC) | Native must | Witness | Signs |
|---|---|---|---|---|
| **E01 billboards** (600/603/604) | item factory → tessellated builder `FUN_140385F70` (or triangle list `140396F30`); slot 27 picks index 12–56 by kind, soft (`depthBlend`), glow (+75); blend by §3.2; depth test-on write-off; raster bank | PSO key `(index, glow, HS/DS pattern, blend type, depth, raster)`; SRV t0–t2 from the FXR textures, t3 scene depth; ring vertices; flipbook in the vertex data | I-0063 inventory (8 `tessellate*` programmes), m40 bonfire (checklist 3.6 T6), m37 ≈ 68 patch-list draws per frame (3.5) | **5 per `3000x2`**, 2 per `3000x0`; CameraPlane, LocalSouth, GlobalYaw orientations |
| **E02 lines and trails** (601/602/606/10012) | tracer indices 36–40 (+glow), `tessellateTracer*`; item `+0x38` ids 0x24/0x25 | as E01 plus tracer constants | weapon swing (3.x) | none |
| **E03 model particles** (605) | FFX FLVER renderer, material programmes of `gxflvershader` | reuse the native model path | checklist 5 (E1–E5) | none |
| **E04 distortion** (607) | kinds 0x1F–0x24 → indices 57–62 (soft 140–145), t0 = frame copy | one frame copy per frame at the start of pass 0x29; non-tessellated builders | `GXFfxdistortionBump` pair in ≈ 47 of 1,800 frames at m40 (6.2 L2) | **1 per `3000x2`** (`mode = NormalMap`, `s06000`) |
| **E05 radial blur** (608) | kind 0x25 → index 63 (139) | as E04 | a spell or boss (6.3) | none |
| **E06 effect lights** (609, 11000, 11002) | host → light registry, no draw | registry entry, point path | R7 = 163 at m40 (1.4) | **1 per `3000x2`**, under LOD child 0 |
| **E07 GPU particles** (10000…10009) | VFX set, compute, `FUN_1402695E0`/`140277C00` blend by `FUN_1402607E0` | compute root signature, update-pass hook | m45 Snowfield (7.1–7.4) | none |
| **E08 light shafts** (10003) | indices 71–74 | on witness | m30, m33, m38 | none |
| **Control, sound, LOD, proxy** (1, 35, 55, 75, 84, 128–133, 199, 300–501) | engine CPU, no device | transcribe unchanged | R4: `sc+0x14C` swaps every frame | all (the controller is only these) |

### 5.1 Sign-specific order (refines PHANTOMS-SIGNS-NATIVE P12–P14)

1. Effect system exists (checklist 1.1–1.3): the sign handle becomes a live instance (T3: `handle+0x18` non-null) and the light appears in the registry.
2. E01 draw (3.1–3.3) without glow: the three additive and one subtractive sprite layers appear on the ground.
3. Glow (4): `bloom` layers write target 1.
4. E04 + frame copy (6): the ripple.
5. Variants: only the FXR id and colours change (§1.1); no native difference between white, red, gold and magenta.

### 5.2 Witnesses and live rows (none taken; the thread is static)

| Row | Question | Read |
|---|---|---|
| SG1 | programme index and kind per sign billboard | the programme names at the commit for each of the five nodes (expected `GXFfxtessellate*Texture`, soft where `depthBlend`, `…GlowD` where `bloom`; distortion pair `GXFfxdistortionBump`) |
| SG2 | the blend type reaching `ctx+0x960` (§3.3) | `[manager+0x118/+0x11C]`, `ctx+0x960`, bank pointer, for Add, Normal and value 0 |
| SG3 | the light under the LOD node | registry count (+1) near and far (0.5 m, 5 m, 40 m, 60 m) |
| SG4 | texture formats on the sprites | SRV formats: BC7 for `s11636`, `s50907`, `s50908`; BC1 for `s11004` |
| SG5 | distortion inputs | t0 = this frame's copy, t1 = `s06000`, blend state, one copy per frame |
| SG6 | the sound | sound id 30001 started once at creation |
| SG7 | pixel witness | the High Wall NPC sign: flat additive glow on the ground, rising sparks (`s11004`), the soft ripple |

## 6. Corrections to earlier notes

- FFX-FAMILIES-PHASE2 §2.4 and EFFECTS-PHASE2-E01-E02-NATIVE §6 say "blend by the FXR blend type `ctx+0x960`": true as the bank selector, but `ctx+0x960` is set by slot `+0x60`, not by the appearance
  setups, and its value for classic draws is the open point of §3.3. The mapping of the FXR enum is `FUN_141ACABF0` (§3.2).
- PHANTOMS-SIGNS-NATIVE §6: the controller is not "no renderer, ids at `+0xAF8` and `+0xAB4`": those are the `SFXReference` (30002…) and the `NodeSound` id (30001 or 30011); 30000 is not an unknown
  role: it is two billboards and a proxy.
- The earlier note that "`FUN_1402607E0` maps the FXR BlendMode of classic billboards" (this thread's working hypothesis) is withdrawn: it serves the GPU-particle callbacks only.

## Exports and evidence

- Tools (this part): `Read-Fxr.ps1` + `FxrDump.cs` (FXR3 structure and schema names), `I0068Dis.java` (disassembly of undefined code through `Invoke-Map.ps1 -ScriptName I0068Dis.java`).
- Evidence (outside Git): `evidence/I-0068/queries/signs/fxr-30000…30052.txt` (the 18 annotated dumps).
- Decompiled (private database, re-exportable): `FUN_1402695E0`, `FUN_140277C00`, `FUN_1402607E0`, `FUN_140264230`, `FUN_140260830`, `FUN_140266780`, `FUN_14036FD70`, `FUN_140373980`, `FUN_141ACABF0`.
