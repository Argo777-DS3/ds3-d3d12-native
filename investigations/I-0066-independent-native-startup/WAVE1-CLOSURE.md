# Wave 1 closure: milestone, reserves and GX dependencies (D-0191, closed D-0194)

**Closed (D-0194, Thomas, 2026-10-19 22:22 Paris): the first milestone « jeu jouable en natif » is accepted with the reserves of §2.**

2026-10-19, after the code cleanup ([CODE-CLEANUP](CODE-CLEANUP.md)). Wave 1 closes without chasing each remaining small gap
(D-0191): this note lists what wave 1 delivers, what stays open as an explicit reserve, and what the native path still owes to
the original's GX set-up. It is the closure's input for Thomas; the acceptance of the milestone is his.

## 1. The milestone proposed for acceptance

ROADMAP's first milestone of C3: real DS3 startup, a genuine zone loaded and rendered independently. Observed:

- DS3 starts, shows its menus, loads saves and plays with the native D3D12 renderer and no D3D11 device, at the seven
  witness zones (m30 High Wall, m31 Undead Settlement, m33 Road of Sacrifices / Crucifixion Woods, m37 Irithyll, m40 Firelink
  and Cemetery of Ash, m45 Ariandel, m50 Dreg Heap) and at Untended Graves, Cathedral of the Deep and the Dancer's hall,
  each against the original side by side (STATE « What runs natively »; the pictures of the last comparisons in
  [OPTIMISATION-REVIEW](OPTIMISATION-REVIEW.md) 8.6 and [CODE-CLEANUP](CODE-CLEANUP.md) §3).
- Thomas played on the launcher (D-0182) and heard the intro cinematic natively.

Proposal: Thomas declares the first milestone reached (« jeu jouable en natif »), with the reserves of §2. The final
acceptance (complete normal game and DLC without D3D11) stays the mandate's condition and is not claimed.

## 2. Reserves

Blocking means: can end the game or make a zone unplayable; acceptable means: a fidelity or performance gap that leaves the
game playable. None of the observed gaps is blocking; the blocking entries are risks not yet met in play.

### 2.1 Risks that could end a session (blocking if met; none met in the witness runs)

| Risk | Where | Why it is a reserve |
|---|---|---|
| A Scaleform family never witnessed (blend mode, fill kind, texture format) ends the game (`std::terminate`) | UI path, [FABLE-REVIEW](FABLE-REVIEW.md) 4.2 | A DLC or rare menu screen could use one; remedy (skip and count) is wave 2 |
| `Refuse` (process exit) on transient per-frame data (a non-finite bone or placement, an unreadable list node) instead of skipping the model | collect | Never seen in the witness runs; remedy wave 2 (FABLE-REVIEW 4.2) |
| A runtime resolution change, and full screen | presentation | Refused since the start (the output extent is fixed at start); Thomas's closure tests decide what remains |
| Process-exit heap fault 0xC0000374 | exit | Pre-existing, also in the Reference (not native); fixed natively by I-0099 (K-0484), dumped like any fault there since 2026-10-29; the Reference still raises it (a row) |
| The DLC areas beyond the witnesses (Ariandel and Dreg Heap in the last comparisons; the Ringed City m51 witnessed in the earlier zone tours, Zeta, not in the last comparisons) | coverage | Final acceptance only |

### 2.2 Visual gaps (acceptable; listed, not chased)

From STATE « Open gaps », each with its evidence:

- the Dancer of the Boreal Valley hall (High Wall): floor, carpet and lower half 0.45-0.79 of the original ([CODE-CLEANUP](CODE-CLEANUP.md) §3, found 2026-10-19);
- the Shrine Handmaid's chair white (o404854, FABLE-REVIEW 1.2);
- the mirrored water view without the sun shadow (D-0189);
- m31 wheels lighter, the m30 lighting residual, the m40 wall at 1.23 ([WAVE1-LAST-EFFECTS](WAVE1-LAST-EFFECTS.md) §1);
- the translucent NPC phantom, one summoned-phantom residual, the original's glow output not isolated (WAVE1-LAST-EFFECTS §4, §5, §8);
- effects: the m37 falling snow (E07 GXVfx appearances), E03, the sorcery and miracle families (Thomas, 2026-10-19: Lightning Spear and Way of White Corona drawn wrong), the key order ([FFX-DRAW](FFX-DRAW.md) « Open »);
- the documented departures of FABLE-REVIEW 4.3 (forward rows without IBL, SSAO or local lights; spot lights not lit; local
  shadows opt-in; FXAA types not native; glare shape 1 only; gamma fixed at 2.2; character light-probe blending omitted);
- the reflection walk's suppress bytes (OPTIMISATION-REVIEW 8.6, INFERRED: the player and candles possibly missing from the water).

### 2.3 Untested cases (reserves of wave 1, treated in wave 2 unless Thomas's tests close them)

Thomas's closure decisions (project chat, 2026-10-19 07:13-07:16 Paris, STATE): one full-screen test is added, nothing else;
the movie sound is acquired if the intro cinematic plays with its sound; the resolutions are acquired (small windows and
1080p tested), only a resolution change from the game's menu stays a reserve; the graphics levels and the DX12 launcher become
wave-1 reserves treated during wave 2; the complete game + DLC without D3D11 stays the acceptance condition.

| Case | Position |
|---|---|
| Full screen | **Crashes** (Thomas, 2026-10-19 16:36, launcher journal `thomas-play-20261019`, play-163607): Apply in Graphics runs the engine's GX display code, which reads +0x88 of a null object (access violation at DarkSoulsIII+0xB544). Blocking reserve: full screen must go through the native presentation |
| Intro cinematic with its sound | **Acquired** (Thomas, 16:40: the sound plays, in sync) |
| Resolution change from the menu | **Refused** as expected: the window keeps its start size (Thomas, 16:37); reserve |
| Character paint D2 (blood on a struck character) | **Not shown** (Thomas, 16:46): `native_paint_drain` received 19 requests and painted none (13 refused by its gate, 6 of another type). Corrected (D-0193, 8859b213): 51F80 copies the receiver's gates from the model's +224, never written natively; the drain now takes them from the native union of the model's material flags. Then four more faults found live and fixed (D-0193): PS b10 208 bytes, the cloth VS registers b2 / b7, the mask t4 read at d+0x70, and the projector depth map (rung 2b). **Shown** (Native2369, Native2375 against Reference2362, Alpha striking the Fire Keeper): thin streaks as in the original ([DECALS-PAINT-READY](DECALS-PAINT-READY.md) section 8). Reserves: no ordinary enemy struck live; blood never fades (types 1 and 2 undrawn) |
| Every level of every graphics setting (OP-0 to OP-7, `Run-OptionSweep.ps1`) | Reserve, wave 2 |
| The launcher in Thomas's daily play | Reserve, wave 2 |
| Asian and Cyrillic languages beyond the witnessed ones, DSR | Reserve (MENU-TESTS-PREP) |
| The pacer at refresh rates other than 60 Hz (144, 165 Hz, VRR) | Reserve, wave 2 (D-0169) |
| Movie and NPC dialogue audio measured by mechanism | Closed by the intro test if it passes |
| Zones never compared side by side (Catacombs, Smouldering Lake, Irithyll Dungeon, Profaned Capital, Anor Londo, Lothric Castle, Consumed King's Garden, Grand Archives, Archdragon Peak, Kiln) | Reserve; covered by family, not by zone (D-0150), the final acceptance needs them |

### 2.4 Performance

Thomas's play (2026-10-19 16:36-16:50, journal `thomas-play-20261019` in the evidence store): **Firelink Shrine** averages 60 frames/s but isolated frames take 100-140 ms; they come in bursts while the character moves (models streaming in). A texture cache by content was tried and refuted (D-0193 work, 2026-10-19 17:04-17:21: Native2320-2323, 28 reuses of 1,852 admissions, the slow frames unchanged; removed). The sample profiler on a walk at Firelink (Native2325) names the cause: during a burst of 25 frames at 50-70 ms the engine thread spends 55 % of its time in the collect, 39 % in the fresh admission of models (20 to 30 models a frame whose records are invalidated and rebuilt while their textures arrive: `records_invalid`, `texture_lookups` 400-640 a collect), against an idle engine thread between bursts; the remedy is wave 2's scene data persistence (what changed handled explicitly), with this walk as a third measurement scene. The original, the same session in D3D11 (Thomas, 16:55), is « perfectly smooth »: the hitches are native only. **Archdragon Peak** (never compared before): 4,200-5,200 draws a frame, 37-53 frames/s, collect 12-16 ms (the m45 limit, larger: wave-2 persistence); distant enemies vanish and reappear (not explained by the journal; open).

**Distant enemies vanishing and reappearing at Archdragon Peak** (Thomas, 16:50; D-0193, 2026-10-19 21:35-21:58, Eta from a copy of the m40 zone save, travel tab x 803, row « Archdragon Peak »): not reproduced. At the first bonfire the serpent-men c1390 / c1391 stay drawn for two minutes with a fixed camera (Native2376-2377), and a twelve-step camera sweep shows the same characters in both modes (Native2378, Reference2379). The native admission follows the engine's fade weights: an entity leaving the view takes geometry weight 0 (
ative_zero_fade) and is admitted again when it comes back (
ative_character_candidate with light-observe.flag). INFERRED: what Thomas saw while moving is the same fresh re-admission of models that causes the hitches (an entity coming back into view is rebuilt while its textures arrive), so it goes with wave 2's scene data persistence; reserve, to recheck by walking through the zone after that workstream.

m45 (Ariandel, the heaviest witness): 56-57 frames/s, collect about 12 ms (the per-entity collect of 1,750 models: the
first wave-2 workstream, scene data persistence); m33 1080p 60 frames/s; the zone load hitch 0.34 s. Not a blocker; 60 frames/s
everywhere is the wave-2 target.

## 3. Dependencies on the original's GX set-up

Natively the original's GX set-up (the GX device, B5E00's display and device initialisation, D59E90 and D573B0's GX scene
set-up, the GX resource repositories, the engine's gparam banks) never runs; the native module substitutes for the parts the
engine's CPU code still reads. A wrong substitute can change a whole zone: the probe layout byte image+0x4961ABC, left at 0,
took every character's probe lighting in every zone until the optimisation review set it (OPTIMISATION-REVIEW 1.4).

Inventory: [GX-DEPENDENCIES](GX-DEPENDENCIES.md) (static, 62 rows: direct writes, empty engine banks rebuilt natively, hooks
replacing GX-side functions, reads that assume a value). The zone-wide ones, the dependencies a wrong value would show in a
whole zone or in every zone:

| Dependency | Native substitute | Position |
|---|---|---|
| Probe layout byte image+0x4961ABC (D573B0) | set to 1 at the root's publication | Witnessed in every zone (OPTIMISATION-REVIEW 1.4) |
| The root's device members +0x4DD0/+0x4DD8 | left nil: the engine's device-absent branches | Witnessed in every zone; a site without such a branch would fault (Refuse) |
| The engine's gparam banks (light, fog, sun, ambient rows, Setting Info, Tone Map, Bloom, DoF, motion blur, grading, AA, shadow, SSAO, glow) | rebuilt from the admitted files (`ds3light::Banks`) | Witnessed at the witness zones; layer order, fog map position and default fallbacks INFERRED from one or two zones each |
| Quality table and object root+0x5358/+0x5360 | native object, rewritten from the options | Witnessed at the default options; other levels untested (reserve 2.3) |
| Factor object root+0x5368 (127010) | built once, its updates 127240/499C0 omitted | INFERRED constant at its constructor's values; read by glow, AO, velocity, AA, shadow blur and the YEBIS converters |
| Resolver and material flag words ([144793348]+0x48) | native layouts and three flag bits | Witnessed by families; other bits open |
| Scene context fields, the camera extent, the replaced phases D55190, E006A0, C30D0, 17E30, D1E8A0 | native transcriptions | Witnessed; their omitted side effects (the active context global [144961BF8], root +0x47F0..+0x4858 and +0x4E18) INFERRED unneeded |
| Env-map region cubes | found by an INFERRED name rule | A missing cube skips its region silently |
| Byte image+0x4961ABD (the fog distance mode, set once by D573B0) | set to 1 at the root's publication since 2026-10-19 | **Corrected** ([FOG-BYTE-4961ABD](FOG-BYTE-4961ABD.md)): natively 14E900 took the other branch of the height-fog factor (m45: -0.01 against the original's +0.01, Reference2311 / Native2312, Native2313); the native banks already held the record's word 6; the witness pictures are unchanged |

Not GX dependencies but native constants in the same place: the SYSTEX colours, the dither and sampler banks, the source gamma
2.2 (FABLE-REVIEW 4.3).

## 4. Next

On Thomas's word: the milestone accepted or not, his closure tests folded into §2.3, then wave 1 closed in STATE and ROADMAP
and the first wave-2 workstream (scene data persistence) opened. The 4961ABD divergence above is corrected ([FOG-BYTE-4961ABD](FOG-BYTE-4961ABD.md)): the fog record now takes the
original's branch; the witness pictures are unchanged.
