# I-0103: the effects' census (FINISH-LINE line 3)

2026-10-28, the pilot (Opus 5.5). Static: the game's data (I-0063's whole-game FXR scan) and the decompiled game (Thomas's
Ghidra on the DS3 copy, read-only), against the native tree at `main` `85ac5564`. No live run.

## 0. Answer first

- **The game draws its effects through 12 FXR action kinds** (plus 2 that light, 609 and 11000, and 2 that only push particles). The engine
  maps each kind to one renderer class through one registry of 22 types (§2); the native's allowed list is a list of those
  classes, so every family's native status follows from the kind (§3).
- **Drawn natively today** (OBSERVED at the witness zones and Alpha's Fire Orb; 600 INFERRED, below): 600, 602, 603, 604,
  607, 608 and the GPU particles 10000. **Allowed, never witnessed**: 601 (31 FXRs). Together 87 % of the drawing occurrences (26,140 of 30,004).
- **Skipped natively, used by the game** (at the census; 10003 native since I-0140 tranche 10) (13 % of the drawing occurrences, 3,864 of 30,004; 991 of
  3,293 distinct drawing FXRs): **E03 model particles
  605** (752 FXRs in every bundle, behind `ffx-model-particles.flag`, which faulted after the load in Native2035), **606**
  (212 FXRs, 17 bundles, class `3D7E380`), **10012** (125 FXRs, 13 bundles, class `3D660B8`; OBSERVED skipped at m30, Native1968), and **10003 light shafts**
  (3 FXRs, m30, m33 and m38 only; where m30 places its shaft is not read). Effect **spot** lights 11000 (4 FXRs) take the map's spot path, native since I-0106, to the rays
  (D-0253): m45's two in the native light list at their windows (I-0140 tranche 27; that they are the FXRs' INFERRED from place).
- **FINISH-LINE's « 9 familles sautées »**: of the nine skipped classes, three draw and are used (606, 10012, 10003), one is
  used but draws nothing (10301, a force field: its draw slot is `ret`), five are never used by the game's data (10002,
  10008, 10200, 10302, 10600): nothing to write for those.
- **Outside the FFX list** (§5): the decal materials drawn with a non-Decal class, the burning objects' look (no native reader
  of the Burn MTDs' parameters), the blood's fade, the translucent NPC phantom, the surface snow rows of the per-frame block
  never produced, and four E01 correctness points (key order, depth-writing draws, the copy seen by 0x31, passes 0x20/0x21).
- **Leads for what Thomas saw** (INFERRED, §6): the Estus Flask missing from the hand and Lightning Spear and Way of White
  Corona « drawn wrong » (2026-10-19) fit the families skipped in `commoneffects` (605 E03 above all, then 606 and 10012);
  the Ringed City's grass is E03 (I-0068). The m37 snow is drawn natively since Native2021: STATE's « m37 snow » was stale.

## 1. Method and sources

- **The game's effects**: `evidence/I-0063/artifacts/scan/fxr.tsv` (I-0063, 2026-10-06: SoulsFormats 1.0.1 over every
  effect bundle of Data1-5 and DLC1-2; 148,615 rows of bundle, FXR, action, count). Summed by
  [`Get-FxrActionCensus.ps1`](Get-FxrActionCensus.ps1) into `evidence/I-0103/census/actions.tsv` (61 kinds) and
  `actions-by-bundle.tsv` (932 rows). The bundles: `commoneffects`, `dlc1`, `dlc2` and the maps m30-m41, m45, m46, m50,
  m51, m53, plus one loose FXR (`f000430002`). An occurrence counts an action in an FXR file, not its screen time or whether a
  map places it.
- **The registry**: `FUN_14024B260` (the GX FFX system's construction) registers 14 types with `FUN_140253710`, and its call
  `FUN_14036D9A0` registers the 8 classic ones (seven sibling calls `FUN_14036DB90` .. `FUN_14036E010`, and 606 inline); `FUN_14036DB30` seals the table
  `[1449651D0]` (0x48-byte rows, count `[1449651D8]`). Each descriptor's slot +0x20 is the constructor, which tests the
  action kind at `[node+0x18]+0x240` and builds the renderer; its first vtable store is the class. The native's own
  transcription of the 14 rows (`native/frame/effect_system.cpp`, `types[]`) agrees slot for slot, and the classes match
  FFX-DRAW's census of the renderer vtables.
- **The native's status**: `effect_draw::Execute`'s allowed list (`native/frame/effect_draw.cpp`): a recorded command whose
  renderer class is not listed is counted `skipped_renderer_<class>` and not run. Journals: those counts are written only
  with `ffx-record.flag` (the `native_ffx_execute` row), held by 46 of the `i66-native-*` runs (20 of the 71 `i66-native-20xx`).
  The classes they count as skipped: `3D65878` (E07, before D-0186), `3D7E740` and `3D7E8C0` (602 and 608, before they were
  allowed, Native2005 and 2034), `3D65F88` (E03, Native2036-2041) and, once, **`3D660B8` (10012)**: 60 commands in a 600-frame
  window at m30, the High Wall (`i66-native-1968-m30-e01`, journal line 42409), a zone compared side by side for effects
  (Native/Reference1968, 2048) where the skipped trail went unseen. FFX-DRAW's « never met at the witness zones » was wrong
  for it. No other never-allowed class appears in the evidence store.

## 2. The GX effect type registry (STATIC, read at the addresses)

| Action | Hex | Constructor (slot +0x20) | Renderer class | Draw slot 8 |
|---|---|---|---|---|
| 600 | 0x258 | `375C70` | `3D7E440` | builder (ring, slot 27) |
| 601 | 0x259 | `378DE0` | `3D7E4A0` | `378F50` |
| 602 | 0x25A | `3923F0` | `3D7E740` | `392560` |
| 603 | 0x25B | `37D260` | `3D7E5C0` | builder |
| 604 | 0x25C | `385DF0` | `3D7E680` | `385F70` (tessellated) |
| 605 | 0x25D | `27E5E0` (`27E230`) | `3D65F88` | `27EA70` (FLVER models) |
| 606 | 0x25E | `3702D0` | `3D7E380` | `3709B0` (the draw context's slots and the ring through `36F280`/`36F350`, I-0105 tranche 2; « no ring » and « the pass context's slots » were wrong) |
| 607 | 0x25F | `396DA0` | `3D7E800` | `396F30` (triangles) |
| 608 | 0x260 | `39E780` | `3D7E8C0` | `39E900` (`Draw(4)`) |
| 609 | 0x261 | none (slot +0x30 `28F200`) | point effect light | - |
| 10000 | 0x2710 | `268E70` (`26A050`) | `3D65878` GXVfx standard particle | `2695E0` |
| 10002 | 0x2712 | `26EC30` | `3D659C8` | - |
| 10003 | 0x2713 | `270F80` (`274680`) | `3D65A88` light shaft | `271B40` |
| 10008 | 0x2718 | `277490` (`2786C0`) | `3D65D38` | - |
| 10012 | 0x271C | `288C80` | `3D660B8` | `2891A0` |
| 10200 | 0x27D8 | `275190` | `3D65AF8` | - |
| 10300 | 0x283C | `275DF0` | `3D65CC8` wind area (force field) | `ret` |
| 10301 | 0x283D | `27BF40` | `3D65E38` force field | `ret` (`27D0F0`) |
| 10302 | 0x283E | `27D270` | `3D65EA8` | - |
| 10600 | 0x2968 | `27BB70` | `3D65DC8` | - |
| 11000 | 0x2AF8 | none (slot +0x30 `28F290`) | spot effect light (the map-light path, EFFECT-LIGHTS §2) | - |
| 11002 | 0x2AFA | none (slot +0x30 `28F320`) | effect light | - |

Two more types go to another registry (`[144965290]`, `FUN_140253990`): 0x4B and 0x51, properties without a renderer.

## 3. Every family, its weight and its native status

Status since this census (2026-11-07): 605, 606 and 10012 are drawn by default since I-0105 (STATE « What runs natively »); the table keeps the 2026-10-28 statuses. Within the drawn families, the calls that natively do nothing without a word (the per-rectangle scene copies of 607, 608 and the distortion trails; the skips counted only with `ffx-record.flag`) are in [I-0113 STUDY-SILENT-CALLS](../I-0113-open-gaps/STUDY-SILENT-CALLS.md).

Whole game: occurrences / distinct FXRs / bundles. « Witness bundles » are m31, m37, m45, m50, m51 (and m40, Firelink).
The family names E01-E08 are I-0063's grouping (FFX-FAMILIES-PHASE2 §1).

| Action | Family | Whole game | Native status | Evidence |
|---|---|---|---|---|
| 603 | E01 billboards | 21,448 / 2,845 / 20 | **drawn** | OBSERVED: every witness zone's bonfires and torches (FFX-DRAW N6, Native1964-1968) |
| 609 | E06 point lights | 2,457 / 1,269 / 17 | **lit** (the native point lights) | OBSERVED: effect lights fed (FW-3), Firelink flames |
| 605 | E03 model particles | 2,671 / 752 / 20 | **skipped by default**; `ffx-model-particles.flag` faults after the load (Native2035) | OBSERVED: `skipped_renderer_3D65F88` in Native2036-2041; Firelink's only E03 draw `s11600` (Reference2037) |
| 608 | E05 radial blur | 1,441 / 752 / 17 | **drawn** | OBSERVED: the 2D/5 strips of Fire Orb's casts (Native2036) |
| 604 | E01 multi-texture billboards | 1,382 / 462 / 18 | **drawn** | OBSERVED: the tessellated E01 draws at the witness zones (Native1955-1966) |
| 600 | E01 point sprites | 817 / 316 / 20 | **drawn** (INFERRED) | allowed, 72 renderers of its class made at m37 (Native1945) and never counted skipped; the journals keep no per-class execution count |
| 606 | E02 lines and trails | 792 / 212 / 17 | **skipped** (class `3D7E380`) | STATIC: not in the allowed list; present in every witness bundle but never recorded at the views run |
| 607 | E04 distortion | 461 / 204 / 20 | **drawn** | OBSERVED: the distortion quads (Native2027-2030) |
| 10012 | E02 trails | 395 / 125 / 13 | **skipped** (class `3D660B8`) | OBSERVED: skipped at m30 (Native1968, 60 commands a window); in m30, m31, m37, m45, m50, m51 and `commoneffects` |
| 602 | E02 lines | 311 / 120 / 17 | **drawn** | OBSERVED: Fire Orb's casts (Native2036) |
| 10000 | E07 GPU particles | 236 / 95 / 16 | **drawn** | OBSERVED: the bonfire embers at every witness zone (D-0186) |
| 10300 | wind areas | 208 / 104 / 12 | no draw; built natively (allowed) | STATIC: draw slot `ret` |
| 601 | E02 lines | 44 / 31 / 5 | **allowed, never witnessed** | STATIC: allowed since FFX-DRAW's closing; in m32, m38, m39, m51 and `commoneffects` |
| 11000 | E06 spot lights | 7 / 4 / 3 | **native** (the map's spot path, I-0106; m45's two kept in the light list and traced, identity INFERRED from place) | STATIC: EFFECT-LIGHTS §2; m33, m45, `commoneffects`; I-0140 RESULT « Tranche 27 » |
| 10003 | E08 light shafts | 6 / 3 / 3 | drawn since I-0140 tranche 10 (class `3D65A88`, its own half-size targets) | STATIC: m30, m33, m38 only; seen natively in the cutscenes 30000010 and 38000020 (`i140-e31`) |
| 10301 | force field | 5 / 5 / 1 | no draw (skipped, draws nothing) | STATIC: draw slot `ret`; `commoneffects` |

The other 45 kinds in the scan (0-503, 10100, 10400, 10500) are the FXR library's control and property actions: none is in
the renderer registry (75 and 81 are the property registry's 0x4B and 0x51), none draws through a renderer.

## 4. Where the skipped families are (occurrences / FXRs per bundle)

- **605 (E03)**: `commoneffects` 469/269, m51 286/83, m33 280/67, m34 262/71, m30 206/42, m45 172/57, m40 128/25, m38
  118/34, m35 110/37, m41 108/17, m37 106/30, m32 90/21, m31 84/28, m50 78/29, `dlc2` 78/47, `dlc1` 52/34, m39 26/12,
  m46 6/3, m53 6/3, the loose `f000430002` 6/3. In every zone, and the heaviest in the spells' and items' bundle.
- **606**: `commoneffects` 171/84, m30 98/29, m37 68/15, m34 68/16, m40 60/9, m33 48/10, m51 48/10, `dlc2` 48/17, m38
  42/11, m41 32/10, m31 28/9, m35 22/7, m45 16/2, m32 16/7, m39 14/5, `dlc1` 7/5, m50 6/3.
- **10012**: m45 102/16, `commoneffects` 74/51, m51 38/9, m50 36/6, m37 34/10, m30 22/8, m41 22/5, `dlc2` 16/10, `dlc1`
  15/8, m33 12/3, m35 12/3, m34 10/3, m31 2/1.
- **10003**: m30, m33, m38, 2/1 each. **11000**: m45 4/2, m33 2/1, `commoneffects` 1/1. **601**: `commoneffects` 18/18,
  m39 10/5, m32 6/3, m38 6/3, m51 4/2.

## 5. Effect gaps outside the FFX draw list

| Gap | Status | Source | Line |
|---|---|---|---|
| Decal materials (GX00 `M[ARSN]_e`, `_tr`, `_al` map overlays, the eyebrows) keep their non-Decal class natively | wrong | ROADMAP (I-0100 tranche 6), K-0483 | 7 |
| A burning object's look: slot 7 of the native material answers DS3's burn update (I-0101), but no native reader of the Burn MTDs' parameter list | missing | I-0101 RESULT, ROADMAP line 5 | 5 |
| The blood without its fade (wave-1 reserve) | wrong | ROADMAP « Wave-1 reserves to close » | 5 |
| The translucent NPC phantom; one summoned-phantom residual | wrong | WAVE1-LAST-EFFECTS §4, §8 | 7 |
| The surface snow: the per-frame block's snow rows (+0x60..) are never produced natively (`ProduceNativePerFrameGlobals` writes +0x00..+0x0F and +0x20..+0x5F, `producers.cpp` +0xB0 and +0xC0..+0xDF; +0x60..+0xAF by none); I-0063 found snow on surfaces set in m30 and m38's draw parameters | unknown (m38 never seen; m30 compared, no snow difference noted) | `native/frame/game_bootstrap/12_scene_collect.inc`, I-0063 INVENTORY | 5 (check first) |
| E01 correctness: the effects drawn after every forward row instead of sorted with them by key; depth-writing draws (+0x1E0) tested without writing; 0x31 sees the copy without 0x2D's output; passes 0x20 and 0x21 never met | wrong (INFERRED visible only in some views) | FFX-DRAW « Open » | 4 (the base) |
| The glow: the original's output not isolated (W6) | unknown | WAVE1-LAST-EFFECTS §5 | - (abandoned unless a defect is seen) |

FXR water interaction (10013) and lens flares (10014) have no occurrence in the game's data (I-0063): nothing to write.

## 6. Leads for what was seen in play (INFERRED, to settle in line 5)

- **The Estus Flask missing from the hand while drinking, the drink's effect drawn** (Thomas, 2026-10-28 08:42): a held
  item's model shown for an animation fits an E03 model particle (605), the family skipped by default. Settled by reading the
  drink's FXR (its 605 action and model) or by one announced drink with `ffx-record.flag` (`skipped_renderer_3D65F88` at the
  drink).
- **Lightning Spear and Way of White Corona « drawn wrong »** (Thomas, 2026-10-19, WAVE1-CLOSURE): the spells live in
  `commoneffects`, where the skipped families weigh 605 269 FXRs, 606 84, 10012 51. Settled by one Alpha cast of each with
  `ffx-record.flag` (Alpha from a copy of the save, WORKFLOW « Live game rule »).
- **The Ringed City's grass**: E03 (EFFECTS-E03, m51's instanced grass).
- **The Irithyll snow** (FINISH-LINE line 5): the m37 courtyard snow is about thirty point-sprite draws (programme
  `GXFfxtessellatePointSprite`; which action kind issues them is not read), drawn natively as the original's since its screen-size fix (FFX-DRAW 2026-10-17, Native/Reference2021). STATE's open gaps still named it from
  FFX-DRAW's earlier « Open »: corrected.

## 7. Order for lines 4 and 5

1. **Line 4, the base** (D-0190): the native effect contract on the classes already drawn, E01 first (603, 604, 600: 79 % of
   the drawing occurrences), with the E01 correctness points of §5 written into it (the key order with the forward rows, the
   depth-writing draws, one copy per sampling pass).
2. **Line 4, the migration**: 607 and 608 (the frame copy), 602 and 601 (601 witnessed then), 10000 (compute and indirect).
   `effect_draw`'s string-keyed `calls`, `bound` and `draws` counters run every frame outside `ffx-record.flag` (only their log
   is gated): line 4 gates or removes them.
3. **Line 5, by weight**: **605 E03** first (752 FXRs, every zone, the likely Estus Flask, spells and Ringed City grass; a
   native FLVER submission, EFFECTS-E03); then the trails **606** and **10012** (337 FXR uses together, 326 distinct FXRs); the burn look; the blood's
   fade; the surface snow after its check; **10003** light shafts with the zones never seen (line 8: m33, m38; m30's shaft placement to read). The effect
   spot lights 11000: native with the map's spots (I-0140 tranche 27).
4. Nothing for 10002, 10008, 10200, 10302, 10600 (no use in the data), 10013, 10014; 10300 and 10301 draw nothing.

## 8. What I-0102 left to this line

- **The producers' capture layer** (`world/light_producers`: `Snapshots`, its loaders, `UseBanks`, `UseCamera`,
  `UseShadowSetup`, `BankCheck`, `ProduceKeyed`, `ProduceSized`, `ProduceBuffer`, `WriterRva`, `ds3light::Produce`;
  `world/effect_producers`: `ds3fx::Produce`): nothing in the module builds a `Snapshots` or calls either `Produce`; only
  `ao_constants_test` uses `ds3light::Produce` as its oracle. `ds3fx::Produce` recomputes the words slot 27 (`264230`) writes
  into the effect blocks (VSCommonBuffer, DSCommonBuffer, PSCommonBuffer, cbFog) from a capture: reusable knowledge for the
  native effect contracts, not a production interface. **Kept for line 4**, which either takes its formulas into the native
  block producer or removes it with the snapshot loaders (and gives `ao_constants_test` a fixed oracle).
- **The scene renderer's stage observer** (`SetObserver`, `SetPresentationObserver`, the 43 `Stage` calls in
  `renderer/scene_pass*`): no caller of its setters since I-0102 tranche 1, a dead diagnostic. **Removed in line 4's first
  tranche**, which reworks the effect stage's place in the frame anyway (one reviewed change, no behaviour change).

## 9. Limits

- The scan counts FXR files in the effect bundles; an effect a map never places, or a spell no character casts, weighs the same
  as a common one. The weights rank work; they do not prove visibility.
- A family's native status is STATIC from the allowed list unless marked OBSERVED; a « drawn » family can still be drawn
  wrong in a view never compared (601 never witnessed; Lightning Spear and White Corona were « wrong », not missing).
- The registry and the classes were read once by the pilot; the native's `types[]` table and FFX-DRAW's class census agree
  with it, which is the independent check.
