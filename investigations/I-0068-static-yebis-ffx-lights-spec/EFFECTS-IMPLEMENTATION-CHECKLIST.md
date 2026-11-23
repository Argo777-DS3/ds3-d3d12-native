# I-0068 part AF: the "effects" workstream, one implementation checklist

Request (coordinator, 2026-10-10): prepare the effects workstream, probably after NPC bodies.

One checklist that merges:

- EFFECTS-PHASE1-NATIVE;
- E01–E07 (EFFECTS-PHASE2-E01-E02, EFFECTS-E03, EFFECTS-E05-E04, EFFECTS-E07; E06 = EFFECT-LIGHTS);
- FFX-ATTACHED-NATIVE;
- EMISSIVE-GLOW-NATIVE.

Each step gives its checks and witness zones.

This is a sequencing document. The specifications stay the sources: each step names the
section to code from. Labels and terms are as in those specs.

**Witnesses:**

- the five `i63-live-10` zones (m31, m37, m45, m50, m51);
- Firelink (m40, I-0063 `i63-live-01` stream and I-0053 listing);
- the High Wall (m30, Argo's torch `o304863_8030`).

**Read-only first:** every "check" is a pass-through observation (original and native, same
spot) or a capture comparison, in the D-0116 conditions.

## 0. Preconditions (outside this workstream)

| # | Precondition | Why | Source |
|---|---|---|---|
| P0.1 | NPC bodies drawn natively | attached auras, weapon effects and character lights (F2, T-checks) need the bodies; the pilot has them on its branch | MENUS-MAP §55 |
| P0.2 | **shared copies:** readable depth, HDR frame copy at forward pass 0x29 | soft particles, E04/E05, water | NATIVE-PASS-ORDER rank 1; EFFECTS-PHASE2 §8 |
| P0.3 | **forward depth + forward targets [HDR, GB5]** (RT1 = RT0's blend) | effects sort and test against depth; GlowD twins write GB5 | FORWARD-DEPTH-NATIVE; EMISSIVE-GLOW §5 step 1, §8.1 |
| P0.4 | model union for every model (map Emi families on engine 7) | the bowl and lamp emissive under the flames | PRODUCERS-UNION-SNOW-GLOW §1.4 |

## 1. Phase 1: the effect system exists (no draw)

Source: **EFFECTS-PHASE1-NATIVE §2–§7**.

| Step | Work | Checks (pass before the next step) | Witness |
|---|---|---|---|
| 1.1 | construction in `E6C9D0` order: containers (CALL), **scene control `24B260` (TRANSCRIBE)**, texture/FLVER handlers (CALL), **device init `24C530` (TRANSCRIBE)**, publish `SprjSfxImp+0x58 = sc` | R1 `[[144799C98]+0x58]` = `[1449623C0]` ≠ 0; R2 registries sealed, type 0x261 present | any map; m40 |
| 1.2 | per-frame SceneCtrl slot 8 `24D000` (TRANSCRIBE, single-thread branch), view attach (§6, NATIVE) | R4 `sc+0x14C` swaps every frame; library update once per frame | m40 |
| 1.3 | creation routes `E6DCD0`/`E6DA30`/`E6DBC0` now take the `+0x58` branch | EFFECT-LIGHTS R5 counters (many during map load and actions) | m40 |
| 1.4 | **E06 effect lights:** nothing new to draw; the point path (Native1054) reads the FX light registry | EFFECT-LIGHTS R6 about 104 hosts; **R7 `[[GXRS]+0x41D0]+0x10` = 163 at m40 (59 map + 104 effect)**, by family (§3); R8–R10 if a light stays dark | m40 bonfire area |
| 1.5 | crash guard | no access to `[144961AB0]` from the transcribed paths (a hit = transcription miss) | 600 frames, m40 and m30 |

## 1b. The entry door (part AN): every request reaches the effect system through three creators

Source: **EFFECTS-ENTRY-DOOR-NATIVE §1-§5**. It needs 1.1 and 1.2. The door is `E6DCD0`, `E6DA30`, `E6DBC0` (the only callers of the scene-control creation core), each testing `[SprjSfxImp+0x58]`. All request sources are engine CPU code; native work is the system (1.1), the removal of the `7CB820` replacement (2.1) and the checks below. `E6E750` is already called every frame through `SprjGraphicsStep::STEP_Update` `D5B140`.

| Step | Work | Checks (pass before the next step) | Witness |
|---|---|---|---|
| D1 | publish `+0x58` (1.1) and the view attach/detach (PHASE1 §6) | E-D1: creator calls = calls taking the `+0x58` branch | any map |
| D2 | **delete `DeferNativeSfxSceneRequest`** (`game_bootstrap.cpp:7268`, hook `:9148`); `7CB820`, `7CB8F0`, `7CB750`, `7CB630` run unchanged; keep a pass-through observer (id, flags, `_ReturnAddress`); no replay of earlier deferred requests | E-D2: the caller histogram of `E6DCD0` and `7CB820` equals the original's | `m30_00` High Wall, `m40_00` |
| D3 | nothing to reproduce for the sources (object, character, event slots, `SprjFD4Single/MultiSfxIns`, Remo): engine code | E-D3: ids created in 600 frames ⊆ the ids of the zone table (ENTRY-DOOR §4), counts ≤ instances | all five zones |
| D4 | native update and proceed pass objects exist before the first frame with `+0x58` (PHASE1 §5.7); `E6E750` then runs slot 8 | E-D4: pending SfxIns list empty after every `E6E750`; E-D5: hide-mask swap and library instance count | any map |
| D5 | first zone entry with `+0x58`: the pinned leaf `E6E620` | E-D7: SceneCtrl lists reset, creations resume | zone change |
| D6 | log requests whose FXR resource is absent (R3) | the engine returns an empty handle too; native and original agree | m40, m31 |
| D7 | lights (E06) through the door | E-D6: 163 at the m40 bonfire view | m40 bonfire |
| D8 | torch (single SfxIns, `ModelSfxParam`) and sign (`SosSignMan`) witnesses | E-D8: id 800023 at dummy 200 for the 35 `o304863`; E-D9: `7CB820` id 30001 per visible sign, flags 8 | High Wall |

## 2. Attached effects: the flames exist and move

Source: **FFX-ATTACHED-NATIVE §1–§4**. It needs 1.1–1.3.

| Step | Work | Checks | Witness |
|---|---|---|---|
| 2.1 | **replace the deferral hook** `DeferNativeSfxSceneRequest` on `7CB820` (`game_bootstrap.cpp:6758`, `:8628`) by the original call. Do not replay requests deferred before the system existed (risk 3) | T3 `handle+0x10` non-null (`result+0x18`) | High Wall torch |
| 2.2 | object slots (`65AB10`, `7CC4B0` block route, per-frame `7CC880` follow) and one-shot dummy effects (`52EC30`/`5292A0`) run unchanged (CALL) | T1 owner of id 300 (caller RVA); T2 transform ≈ (−31.32, 2.45, 85.38); T4 follow (slot) or none (one-shot) | High Wall `o304863_8030` |
| 2.3 | character handles (`7CBEA0`/`7CBBA0`, follow binding `2447C0`) move with the update of 1.2 | F2 the held flame follows the hand with no new request; two lights within 1 m (radius 0.5 and 2.5) | m40, player with the Pyromancy Flame |
| 2.4 | effect lights of the attached flames | T5 one registry entry within 0.5 m of T2, warm colour; F1 bonfire flame instance at its dummy | High Wall, m40 bonfire |

## 3. Phase 2: E01 billboards and E02 trails (the first draws)

Source: **EFFECTS-PHASE2-E01-E02 §2–§8** (with FFX-FAMILIES-PHASE2).

| Step | Work | Checks | Witness |
|---|---|---|---|
| 3.1 | construction additions: **GPU ring 2 MB (NATIVE)**, sub-allocator, **shader set `291520` (TRANSCRIBE**: native programme records from the shader container), resource manager slot 1 (TRANSCRIBE), SFX scene layer (CALL), **view attach (NATIVE)** | the 128-entry VS/PS lists and glow twins resolved; resources created for the FXR textures | m40 |
| 3.2 | pass points and the scene-control draw at the native passes (§3), `GXFfxDrawPass` and pass context (§4), draw context (§5.2) | P4: no draw in pass 0x3A; 0x39 only for glow-capable entities | m40 |
| 3.3 | PSOs from the STATIC tables (§6); the three builders (TRANSCRIBE, §7): tessellated patch lists, triangle list, `Draw(4)`; stride from the input layout | P1 the eight E01 programmes, frequency ranking; P2 all E01 draws are 1-control-point patch lists with HS/DS; P3 blends off/alpha/additive/multiply, **GlowD twins write two targets**; P5 ring ≤ 2 MB per frame | m40 |
| 3.4 | frame copy bound for the slot-8 fallback (P0.2) | — | — |
| 3.5 | zone counts | Z1 m37 ≈ 68 patch-list draws per frame; Z2 strides 112 (≈ 40), 36 (≈ 27), 160 (1); Z3 m31 5 per frame, m51 6 (stride 112) | m37, m31, m51 |
| 3.6 | the image | P6: the remaining m40 forward/effect gap drops; T6 flame sprites over the High Wall bowl, warm light on the pillar; F3 m40 effect draw count per frame = listing | m40, High Wall |

## 4. Glow (needs 3.3's GlowD twins and P0.3, P0.4)

Source: **EMISSIVE-GLOW-NATIVE §5, §8** and PRODUCERS-UNION-SNOW-GLOW §3.

| Step | Work | Checks | Witness |
|---|---|---|---|
| 4.1 | glow resources: source 1280×720 R11G11B10, four levels ¼…1/32 | — | — |
| 4.2 | extraction `EmissiveToGlowPS` (GB5 → source); `Sample13Weight` ×4 (taps and weights §8.2, scale = BlurSampleScale); `GlowCompositeSinglePS` ×5 additive | G1 per frame: 1 extraction, 4 blurs, 5 composites, the last into the final target | all five + m40 |
| 4.3 | constants: `g_GlowScaleOffset.x` = Blur RgbScale × f (levels) and Final ColorScale × f (last), f = clamp(`[[GXRS+0x5368]+0x80]`); mode B from New Glow Enable; gate Enable | G6 factor 0 → the original skips all 10 draws, native follows; values from `mapbase` id 0 (1.5, 0.5) unless a zone overrides | any |
| 4.4 | the image | G2 GB5 equal over emissive materials (m31/m37); G3 forward rows write GB5 (m40); G4 halos near lamps/bonfire; G5 the High Wall bowl glows | m31, m37, m40, m45, High Wall |

## 5. E03 model particles

Source: **EFFECTS-E03 §3–§5**.

| Step | Work | Checks | Witness |
|---|---|---|---|
| 5.1 | renderer init/release (CALL), log the model resource, find it in the native registry | E1 100% of model resources found (m40, m51, m45) | m51, m45, m40 |
| 5.2 | slot +0x20 `29ADF0` (TRANSCRIBE: keep state and CPU bones, drop GX uploads); adapter draw `1400636F0` detour → native FLVER submission (forward variant, blend from MTD, depth test on, write off, FFX sort key) | E2 submissions = original count (m51 286, m45 172 weights); E3 meshes per submission; E4 bone count and palette bytes | m51, m45 |
| 5.3 | the image | E5: the forward difference drops at model-particle pixels; **the m51 grass** appears (34 draws per frame) | m51, m40 |

## 6. E04 distortion and E05 radial blur (the frame-copy families)

Source: **EFFECTS-E05-E04 §1–§5** (deltas to part H).

| Step | Work | Checks | Witness |
|---|---|---|---|
| 6.1 | one frame copy per frame at the start of pass 0x29 (P0.2); `passCtx+0x4C0` = its SRV | L3 t0 = the copy written this frame; L4 exactly one copy; L5 no feedback (debug layer clean) | any |
| 6.2 | programme indices 57–63 and 139–145 (§1); non-tessellated builders; PSO §4 (s1 wrap for bump) | L2 E04 at m40: `GXFfxdistortionBump` pair in about 47 of 1,800 frames; L6 per-kind counts at m51/m37; L7 distortion twins (70, 138, 150–155) | m40, m51, m37 |
| 6.3 | E05 | L1 PS index 139 with t0 = copy, t3 = depth | a spell or boss with radial blur (i63-live-33 situation) |

## 7. E07 GPU particles

Source: **EFFECTS-E07 §2–§7**.

| Step | Work | Checks | Witness |
|---|---|---|---|
| 7.1 | resources `2A49F0` / release `2AA130` (TRANSCRIBE with fence retire) | G1 two systems: 2,048 and 8,192 records, 360,448 and 1,441,792 B, stride 176 | m45 Snowfield bonfire |
| 7.2 | simulation `2A6A30` and sort/draw `2A7E70` (TRANSCRIBE), constant uploads as root CBVs; pass 0x45 flag-0x10 submission before any particle pass | G2 one update dispatch per system per frame, one indirect draw per pass; G3 live counts about 1,710 and 5,130, dispatch groups about 1,770–1,780; G5 no zeroed start | m45 |
| 7.3 | sparks (`2B01E0`/`2B1700`) after the standard path | G6 m40 `CS_VFX_StandardParticle*`, GlowLP writes the glow target; G7 debug layer clean over 600 frames | m40 |
| 7.4 | the image | G4 the falling flakes (I-0063 texel (1211, 100) at frame 40) | m45 |

## 8. Later (no witness yet)

| Item | When | Source |
|---|---|---|
| E08 effect light shafts (indices 71–74 on the E01/E02 machinery) | a capture at m30, m33 or m38 | WATER-REFLECTION-INTERACTION-E08 §3 |
| ambient A01–A12 | they are FXR actions of E01/E02/E03/E07: they come with those steps | COVERAGE-MATRIX row 38 |
| velocity for effects | none (effects have no velocity; reactive mask for a future upscaler) | VELOCITY-MOTIONBLUR-NATIVE §6 |

## 9. Order and exit criterion

**Order:** 0 → 1 → 2 → 3 → 4 → 5 → 6 → 7.

- Steps 5–7 are independent of each other once step 3 exists. They can be split across
  sessions.
- 4 can start right after 3.3.

**Exit:**

- every check above passes at its witness;
- the m40, m31, m37, m45 and m51 back-buffer gaps attributed to effects (I-0063) fall to the
  non-effect residue;
- 600 frames per witness with the D3D12 debug layer clean.

## 11. Static completeness audit (part AK)

For each step: are the static inputs needed to code it settled? Columns: shaders and
programmes, constants and resources, pass order, gates. **S** = settled in the named section;
**O** = OBSERVED in the retained frames; **L** = live-only (one read or one check, named).

| Step | Shaders / programmes | Constants / resources | Pass order | Gates | Live-only items |
|---|---|---|---|---|---|
| 1.1 construction | — | S PHASE1 §2–4, §7 | S | S | R1, R2 |
| 1.2 per-frame update | — | S PHASE1 §5 | S (slot 8, once per frame) | S | R4; the guard 1.5 |
| 1.3 creation routes | — | S EFFECT-LIGHTS §4 | S | S (`+0x58` branch) | R5 |
| 1.4 effect lights | — | S EFFECT-LIGHTS §2–3 (constant `143D7FEF0` = (1, 1, 1, 1), part AK) | S | S (R9 flags) | R6–R10. Line lights: no witness, not drawn in any capture |
| 1.5 crash guard | — | — | — | S | none |
| 2.1 hook replacement | — | S: the handle internals touch no device global to two call levels (FFX-ATTACHED risk 2, part AK) | S | S | T3 |
| 2.2 object slots | — | S FFX-ATTACHED §1.2–1.3, **plus `ObjModelSfxSlot` (`14065A010`) fed by `ModelSfxParam` row model×1000, created through `140E6EB70`, not `7CB820`** (LIVE-REDUCTION-AM §4) | S | S | T1·a (counter on `14065A010` and `140E6EB70` at the torch: expected effect 800023 on dummy 200), T1·b (caller of the id-300/320 `7CB820` requests: event routes), T4 |
| 2.3 character handles | — | S §1.4 | S | S | F2 (the follow binding semantics are INFERRED) |
| 2.4 attached lights | — | S EFFECT-LIGHTS §3 families | S | S | T5, F1 |
| 3.1 construction additions | S: the programme bytecode source is the shader container (`SprjSfxImp+0x38`), 158 entries per stage (PHASE2 §2, E05-E04 §0) | S ring 2 MB (§5.1) | S | S | none |
| 3.2 pass points | — | S draw context §5.2 | S PHASE2 §3, SCENECTRL §3.2 (0x21, 0x29, 0x2D, 0x20, 0x31, 0x39, 0x45) | S (P4) | P4 |
| 3.3 E01/E02 PSOs | S names, indices and stages (§6, `ffx-program-layouts.tsv`) | S constant layouts (TSV); **O** blend (five states), depth (test on, write off; write on when opaque), raster (cull none), **sampler s9 = linear WRAP** (§6.1, part AK) | S | S | none: the **input-layout formats** are static tables of the executable (11 layouts for the 158 VS, formats, offsets and strides, 0 mismatches against the VS signatures; FFX-INPUT-LAYOUTS, part AM) |
| 3.4 frame copy | — | S §8 | S (start of pass 0x29) | S | L4, L5 |
| 3.5–3.6 checks | — | — | — | — | ACC only |
| 4.1–4.4 glow | S (EMISSIVE-GLOW §2) | S §8.2–8.3, PRODUCERS-UNION §3 (values from `MapBase` id 0) | S NATIVE-PASS-ORDER step 13 | S | G4·AUDIT (area of the `MapBase` registration) |
| 5 E03 | S: the materials' own forward programmes | S model ingestion design (E03 §2–4); **L** the FFX-owned FLVER ingestion and the default patch behaviour (INFERRED) | S | S | E1, E3 |
| 6 E04/E05 | S programmes 57–63, 139–145 (E05-E04 §1–2) | S; the blend state of non-tessellated draws is not observed (no capture frame holds one) | S one copy per frame | S | L3 (radial t0), the sampler bank entry `+0x2E8` (E05-E04 risk 4), L2 |
| 7 E07 | S (E07 §4) | S (E07 §1–3, §5) | S pass 0x45 submission | S | G3 only. `CounterDownSize`: no dispatch site exists in the executable (the only `Dispatch`/`DispatchIndirect` callers are the four E07 sites and non-FFX post blocks; LIVE-REDUCTION-AM §3), so no pass to implement; the native encoder still logs any CS dispatched outside E07 §3 |

**Verdict.** The checklist can be executed from step 1 on. No step is blocked by a
static-only question. The live-only items above are read during the step that needs them, with
the rows of LIVE-CHECKS-CONSOLIDATED. Nothing has to be read **before** coding step 3.3: the
input-layout formats, the one item that had to be, were read from the executable in part AM
(FFX-INPUT-LAYOUTS).