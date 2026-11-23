# STUDY ahead of workstream 12: the light sources, the ambient, the volumetric fog

2026-11-22, the study thread (S, Fable 5.2 high), from main `27e59a137`, for the pilot of ROADMAP workstream 12 (Fable 5.2 high,
D-0281), who copies this folder into its investigation in its first commit (WORKFLOW « Canonical sources »). Read-only: no build,
no live run, no game launched; decompilation read through GhidraMCP (DS3) and headless Ghidra 12.1.3 on a mirror (Elden Ring).
Tags: **OBSERVED** (read by this thread in a file, the code or a table, named), **WORKER** (read by a Sonnet worker, its report
an annex below, re-read here where said), **INFERRED** (reasoned, its basis named), **TO CHECK** (needs a run, a capture or a
reading not made). Nothing here is a decision: §7 lists what is Thomas's.

Annexes (workers' reports, non-canonical until the pilot re-reads what a design relies on; their scripts and tables are in
`C:/Users/User/Tools/DS3-Decompilation/study-ws12/`, outside Git):

- [CENSUS-DS3-LIGHT-FOG](CENSUS-DS3-LIGHT-FOG.md): every LightSet, Fog, LightScattering, Dominant Light and Env Map value of the
  168 GPARAM files, by area and id.
- [CENSUS-BTL-FAMILIES](CENSUS-BTL-FAMILIES.md): the 2,431 placed lights of the 29 light banks by family, and which sit at a
  visible emitter (High Wall, Irithyll).
- [CENSUS-BTPB-PROBES](CENSUS-BTPB-PROBES.md): the 22 baked probe banks decoded: the ambient the authors gave each place.
- [DS3-FOG-AND-LIGHT-CREATORS](DS3-FOG-AND-LIGHT-CREATORS.md): DS3's fog from its parameters to its pixels, the unused
  scattering and GI-volume code, who creates run-time lights, the light objects' fields (GhidraMCP).
- [ER-LIGHT-FOG-VALUES](ER-LIGHT-FOG-VALUES.md): Elden Ring's shipped LightSet, VolumetricFog, FogVolume and RayTracing values.
- [ER-VOLUMETRIC-FOG](ER-VOLUMETRIC-FOG.md): Elden Ring's volumetric fog read in its code (§5.3).

## 0. Answers first

1. **The traced picture lacks the sky's light, not bounces.** The rays take the sky as the dome is drawn (a displayed
   radiance of about 0.02 to 0.07 at the High Wall, OBSERVED in I-0132 LIGHT-UNITS §4), where the original's baked probes give an
   up-facing ambient of about 0.27 to 0.29 in open air, blue-grey (WORKER, the High Wall and the Cemetery side of m40): the authors'
   sky lights four to fourteen times more than its picture (INFERRED: the two figures are in the same units by the formulas, no run
   compared them). R7b had already measured that the GI's sky misses bring nothing visible at Firelink (61.44 against 61.43,
   I-0126 RESULT). This is the first lever of the dark outdoors, the dark silhouettes and the warm player (§3).
2. **The original's ambient is cold everywhere, and three to five times darker under cover than in open air**: 19 of the 22 probe
   banks are 83 to 100 % blue-grey (median R/B about 0.65 to 0.69), covered or not; covered probes' median level is 0.30 (High Wall)
   and 0.18 (m40) of the open ones (WORKER). Thomas's « ambiante froide » is that colour.
3. **The light sets' hemisphere colours are not a usable sky**: their intensity is 0 in 180 of 344 area-id rows (Firelink's set
   0 included) and a neutral grey 0.25 in most others (WORKER; three areas decoded by hand here, OBSERVED). The authored sky is in
   the lightmaps and the probes, and in light 1 of the sets, a blue directional « sky fill » at 5 to 30 % of the sun (OBSERVED).
4. **The native frame already holds the reference to measure against**: the G-buffer's GB4 still carries the baked ambient of
   every pixel (pass 0 only reads a neutral black in its place, `scene_pass_lighting.cpp:347`, OBSERVED), and the engine's baked
   SH rows of every object are still produced. A witness comparing the traced ambient with them, zone by zone, is cheap and
   should come before any change (§3.4).
5. **Most placed lights have no visible source.** 80 % of the High Wall's 184 bank lights are free-floating (148; no emitter
   within 3 m), half of the game's 2,408 point and spot bank lights are cold, 88 are named Fill or Fake by their authors; at
   Irithyll 41 % sit at a candle, torch or lantern (WORKER). The flames' own light is not in the banks: it is an effect light
   (FXR action 609: 2,457 occurrences in 1,269 FXR files). What to do with DS3's own invisible fills is Thomas's (§7).
6. **A light object does not know its owner.** Three classes enter the engine's container (point, spot, line; line never
   used by a shipped effect); the family of an effect light is readable only from the effect side (the host's light pointer and
   its create-info), a bank light by its name string (WORKER). So « the families named from the collect » needs one small
   registry fed where the effect hosts create their lights (§2.4).
7. **DS3 has one fog and it is a light source without a light**: a distance term and a height ramp that blend every pixel
   toward a fixed colour, whatever lights the place; no scattering, no shaft, no volume (the LightScattering group is at its
   default zeros in all 168 files and nothing reads it; WORKER and OBSERVED in the native transcription). In a dark place the
   fog still adds its colour: part of the veils (§4.1).
8. **Elden Ring replaced that fog by a froxel volume** (its GPARAM group `Fog` is gone; `VolumetricFog` is on in every
   gameplay file that holds it): 96 to 160 cells wide, 24 to 40 logarithmic slices, a fixed density and a height law, one
   phase coefficient (0.2), the sun through 8 shadow samples with a jitter and a 0.8 feedback, every point and spot light
   with a scattering scale of its own injected unshadowed, and sky and sun colours typed for the fog; fourteen compute
   programmes, the deferred pixels fogged by a composite after the lighting, the forward ones by sampling the volume
   (WORKER). Its place and shape are the model (D-0195); its typed fog colours are what the rays replace (§4.3, §5).
9. **Elden Ring itself dropped most of the fill lights**: lights 1, 2 and 3 of its sets have a zero intensity in 73 %, 96 % and
   69 % of its rows, its hemisphere is a constant 0.1 in 81 %, and it has no lightmap group; its ambient is a baked irradiance
   volume (INFERRED from its bake's code and files) (WORKER). Its ray tracing is still shadows and AO only: nothing to copy
   for the ambient (D-0236).
10. **The physical-ratio experiment is not to be redone.** Thomas saw it and kept DS3's own strengths (D-0264, D-0293). In this
    workstream « physical » means real sources, real shadows, real shapes and a lit medium, with DS3's authored strengths as the
    baseline and sheets for every family rule.
11. **Proposed order** (§6): names and witnesses first (no picture change), then the sky as a source, the covered places,
    the flames' shapes, the emissive triangles, the volumetric fog, and last the cutscenes' provisional fills (D-0279, D-0282)
    and the zones' tour.

## 1. Where the native lighting stands (the baseline workstream 12 starts from)

All OBSERVED in the code on main `27e59a137` (`native/renderer/shaders/rt_reflect.hlsl`, `rt_lights.hlsli`,
`scene_pass_lighting.cpp`, `native/frame/game_bootstrap/07_scene_lighting.inc`, `12_scene_collect.inc`) unless tagged.

- **The sun**: light 0 of the pixel's light set (the set index is per pixel, GB3.w), its shadow one traced ray (a cone on the
  frames Ray Reconstruction denoises). Several sets live in one frame (15 ids in Firelink's file, 43 in the High Wall's,
  WORKER): one traced sun direction a frame, each surface's colours from its own set.
- **Lights 1 to 3 of the sets** (the fills): zeroed at the collect on traced frames (D-0262), except a cutscene's own sets
  (D-0279, D-0282, provisional until this workstream). The hits still evaluate them (zero).
- **The local lights**: every enabled point and spot light of the engine's container `[[GXRS]+0x41D0]`, bank lights and effect
  lights alike, at DS3's own strength (D-0293), DS3's falloff; shadowed by rays (one chosen light a hit at the PT level).
  The record the rays read (`RtLight`, 128 bytes) holds a kind (point, legacy point, spot), a shadowed bit and the test tool's
  physical bit: **no owner, no family**. Line lights (the third class) are not collected; no shipped FXR uses their action
  (11002: zero occurrences, I-0103 CENSUS), so nothing is lost (INFERRED from that census).
- **The ambient**: the diffuse GI, one cosine ray a pixel (ReSTIR GI at the PT level), its hit lit by the sun, the local
  lights and its emission, its own ambient from NVIDIA's radiance cache (SHaRC); a miss takes **the forward sky as its
  programmes draw it** (`SkyRadiance`: the dome, its zenith, its cloud sheets, fogged from the ray's origin), else nothing.
  No hemisphere, no cube, no lightmap, no SH seed (I-0132 R11d). The forward rows and the effects take a traced probe an
  object through the constants DS3's own programmes read (I-0129 R9).
- **GB4**: the G-buffer programmes are the original's, so GB4 still holds the baked ambient (the lightmaps of 89 to 97 % of
  the map pieces, the instance SH of the rest, I-0110 §6); on a traced frame pass 0 binds a neutral black in its place
  (`gb4_black`, `scene_pass_lighting.cpp:347` and `:418`). The baked picture is therefore in every native frame, unread.
- **The emission**: a hit's `g_EmissiveTexture × FC_EmissiveColor` is added where a bounce ray meets it; no sampling of
  emissive triangles as lights. 8 of Firelink's 1,258 traced slices and 16 of the High Wall's 4,708 emit (I-0132 R11h):
  DS3's fires are effects and point lights.
- **The fog**: DS3's own formula (§4.1) in the composite and in the forward programmes; the reflections' hits fogged along
  their leg, the GI's bounces not.
- **The exposure**: one target for the game (0.12, half strength, a lower bound of 1: D-0263, D-0280, D-0294): it only lifts
  views darker than the original's darkest tenth.

What Thomas saw of it: « je ne vois pas trop ce qu'apporte le RT/PT » (D-0262), « L'original est toujours plus beau » at
Firelink (ROADMAP 12), the physical ratios « moches » (D-0264). The measured gaps: outdoors at 0.56 to 0.74 of the original's
mean level in the tour, the player's back at the Road of Sacrifices 46 against 87 of 255 and warmer, (64, 40, 33) against
(98, 82, 76) (I-0141 tranche 16), Father Ariandel's limbs lit but dark (I-0140).

## 2. The sources' families

### 2.1 The table

« Authored in » is where the game's data hold the family; « at run time » what the engine or the native holds; the last
column is this study's proposal, each one for sheets and Thomas's eye.

| # | Family | Authored in | At run time | Natively today | Proposed in workstream 12 |
|---|---|---|---|---|---|
| 1 | The sun | LightSet light 0, by area and id (angle, diffuse and specular colour, an intensity 0 to 20) | `cbLight` rows, one set a pixel | traced, one direction a frame | unchanged; the several suns of one frame recorded as they are |
| 2 | The sets' fills | LightSet lights 1 to 3: light 1 a blue directional at 0.05 to 0.3 (the « sky fill »), lights 2 and 3 the « LocalLight » pair with a wrapped falloff, often 0 | the same rows | removed (D-0262); kept in cutscenes (D-0279, D-0282) | stay removed; the cutscene exceptions lifted once the sky and the ambient are in (§6 tranche 7) |
| 3 | The hemisphere | LightSet Hemi Color Up and Down (colour², times its 4th float) | the SH rows of an object without a probe | removed | not a source: 0 in 180 of 344 rows, grey elsewhere (§3.1) |
| 4 | The baked ambient | lightmaps (`*_gi_*_dol_*`, family DOL), probes (`.btpb`), the Dominant Light scales | GB4, the instances' SH rows | removed from the lighting, still produced | the reference of the witness (§3.4), never a light |
| 5 | The sky | the dome's and clouds' forward models and textures | forward rows | a GI or reflection miss takes its drawn radiance | **the first source to set** (§3.3) |
| 6 | Bank lights at an emitter | BTL points and spots near a candle, a lantern, a torch, a window; 148 names say so | `GXPointLight`, `GXSpotLight`, a name string | traced, DS3's strength | kept; a source size by family (§2.5) |
| 7 | Bank lights without an emitter | BTL « 動的 » (dynamic), `Omni`, `Fill`, `Fake`, place-named points, half of them cold | the same objects | traced, DS3's strength | named by the collect; kept or removed by family on sheets: Thomas's (§7 question 2) |
| 8 | The bake's key light | BTL type 2, one a bank (23), brightness 0.25 to 50 | never loaded | absent | nothing: never a run-time light in the original either |
| 9 | Effect lights | FXR action 609 (point; 2,457 in 1,269 FXR), 11000 (spot; 7 in 4) | the same classes, created by the effect hosts, colour, radius and position from the FXR's curves with a noise | traced as the bank lights | the flames' family and source size from the owning effect (§2.4, §2.5) |
| 10 | Emissive surfaces | `*Emi*` materials (`g_EmissiveTexture`, `FC_EmissiveColor`), `EmiGlow` | GB5 | seen by bounce rays only; `EmiGlow` without a material in the rays (workstream 7 step 3) | sampled as lights where a witness shows they matter: lava, crystals (§2.6) |
| 11 | Effect sprites | FXR billboards and particles | raster only | not in the rays' scene | unchanged: their light is family 9's |
| 12 | The fog | Fog (11 parameters), by area and id | `cbFog` rows | DS3's formula | a lit medium (§4) |
| 13 | Light shafts | LightShaft (YEBIS, 12 of 18 areas), FXR 10003 | screen-space passes, effect draws | native (I-0138, I-0140) | their fate with the volume (§4.3, §7 question 4) |
| 14 | Env cubes | Env Map ids and scales, region boxes | `SpecularAcc` | replaced by traced reflections | nothing |

Not families of light, read on the way (WORKER, DS3-FOG-AND-LIGHT-CREATORS §2, §3): the `LightScattering` group (a Rayleigh
and Mie parameter set) is all default zeros in the 168 files and its bank has no reader found; the engine creates a GI volume,
three reflective shadow map scenes and a voxel renderer for which no shader programme exists in any bundle (a dead pipeline,
INFERRED from the missing programmes); `AOFields` is in `mapbase` only.

### 2.2 The bank lights (WORKER, [CENSUS-BTL-FAMILIES](CENSUS-BTL-FAMILIES.md))

- 2,431 lights in 29 banks: 2,174 points, 234 spots, 23 directional « KeyLight » rows (the bake's, never loaded: I-0110 §5.1).
- By colour of the 2,408 points and spots: 1,201 cold (50 %), 581 neutral, 626 warm. By name: 921 « 動的 » (dynamic), 856
  place or function phrases, 244 `Omni`, 202 `Spot`, 88 `Fill` or `Fake`, 49 `ADD:`, 48 « ポイントライト ». Only 148 names
  carry a source word (candle, chandelier, lantern, torch, bonfire, hearth), all the candle and torch ones warm.
- **High Wall** (bank 0000, 184 points and spots, the bank's frame the MSB's): 7 at a named flame or bonfire, 10 at an object
  with an unlabelled effect, 3 near a light-shaft or stained-glass effect region, 16 at an object without a known light
  effect, **148 free-floating** at 3 m (180 at 1.5 m). The 133 torch objects carry their flame as an effect; almost no bank
  light sits on them.
- **Irithyll** (bank 0000, 523 lights, the frame fitted by the worker, INFERRED): 213 at a candle, torch or lantern effect
  region within 3 m (146 within 1.5 m), 299 free-floating.
- **Firelink** not classified by place (the bank's and the MSB's frames differ, the fit ambiguous); by name 6 spots and 38
  « dynamic » points, 28 of them « 拠点内部 » (the shrine's interior). The five candle regions of m40 coincide with five effect
  lights: Firelink's candle light is the effects'.
- The zones Thomas named: the Cathedral (m35) 81 lights, 62 cold; the Road of Sacrifices (m33) 64, 47 cold; Archdragon Peak
  (m32) 43, 30 cold; all « dynamic » points with 6 to 8 spots (OBSERVED in the worker's `zone-summary.tsv`). Their place
  against emitters is not known: only three MSBs are recovered locally.
- Bank 0001 against 0000: 53 lights fewer at the High Wall, another 39-light bank at Irithyll Dungeon, two « opening » spots
  swapped for two « dynamic dark » points at Firelink: a second lighting state of the zone (which loads when: open since I-0110).

### 2.3 Who creates lights at run time (WORKER, [DS3-FOG-AND-LIGHT-CREATORS](DS3-FOG-AND-LIGHT-CREATORS.md) §4, §5)

- Three classes enter `[[GXRS]+0x41D0]`: `GXPointLight` (vtable `143D4E808`, 0x1B0 bytes), `GXSpotLight` (`143D4E8E8`,
  0x330), `GXLineLight` (`143D4E888`, 0x1D0). Four push sites: `1400180D0`, `140018490` (the bank loader), `140266FC0` (an
  effect's point), `1402670C0` (an effect's line). No area, ambient or capsule class, no debug creator.
- The effect side: action 609 → `14028F200` → host `1402B2410` → `140266FC0`; 11000 → `14028F290` → `1402B4080` →
  `14024ED10` → `140018380`; 11002 → `14028F320` → `1402B5BA0` → `1402670C0`. A held torch, a lantern, a spell, a bullet and
  a cutscene light are all effect hosts.
- A light holds no owner or host pointer. Only a bank light has a name string (`+0x30`), the bank's four words
  (`+0xBC..+0xC8`) and a world matrix (`+0x110`). From an effect: `host+0x28` is its light, `[[host+8]+0x10]+0x40` its
  create-info with the owner category at `+0x1F8` (I-0068 EFFECT-LIGHTS §2.2 decodes the categories).
- No source radius, size or length on a point or a spot; no scattering or volumetric scale on any light; one specular
  brightness. The flicker is the effect's (FXR curves, a fractal noise on the position) or the bank light's own object at
  `+0x170`.

### 2.4 How the collect can name a light's family (INFERRED design, nothing built)

1. **Bank lights**: the name string at `+0x30` read once per light object (the pointer is stable for the bank's life): the
   prefix class, and a class « at an emitter » or « free » computed at the bank's load from the distance to the nearest
   effect light of a flame family or emissive object (the engine's own lists, no MSB needed). The worker's offline table
   (`lights-classified.tsv`) checks the rule at the High Wall and Irithyll.
2. **Effect lights**: a small registry keyed by the light's address, written where the point and spot hosts create and
   destroy their light (`140266FC0`, `14024ED10`; the native effect system runs these, D-0190), holding the FXR id and the
   create-info's owner category. The family (candle, torch, brazier, bonfire, lantern, spell, weapon buff, boss) then comes
   from one offline table, FXR id → family, made from who refers to each FXR (ObjectParam and ModelSfxParam, the MSB's SFX
   regions, bullets, SpEffect): `evidence/I-0068/queries/ffx/effect-light-families.tsv` is a start; a Sonnet census closes it.
3. **In the record**: `RtLight.kind` has free bits above the shadowed and physical bits; a family index there costs nothing
   and lets a test flag switch one family off or scale it for a sheet, the mechanism R11d's flags already use.
4. **In the journal**: `native_light_near` rows carry the family, so a sheet's caption can say what lights the view.

### 2.5 The flames as sources of their shape

- Today a local light's shadow is softened by a sphere whose radius is a share of its range, capped at 0.5 m, and the ray
  stops 0.1 m short of the light (its housing): one rule for every light.
- With the family known, the sphere is the flame's: a candle a couple of centimetres, a torch about 10 cm, a brazier or a
  bonfire a few tens (values for the sheets, connaissance générale). It changes the softness of the shadows and the size
  of the highlights, not the light's strength, so it does not reopen D-0293.
- The strengths stay DS3's. A family's strength moves only on a sheet that shows why (the fire-lit interiors at 1.4 to 1.7
  times the original's level in the tour were measured before D-0293 and D-0294: to re-measure first, STUDY-ALL-GAPS row 5).
- The flame's sprite is not in the rays' scene: a wet floor reflects the fire's light, not its picture. Outside this
  workstream unless Thomas asks.

### 2.6 The emissive triangles

- The lava of the Smouldering Lake is a large emitter: a cosine ray meets it often, so the GI may already carry it; a small
  strong emitter (a crystal, an ember, a lamp's glass) is met rarely: noise, or nothing. Which case each zone is in is
  TO CHECK with the existing test tools (`rt-emissive-x4.flag`, the GI's stability reader) before any code.
- If a witness shows it: the emissive triangles of the frame's materials listed with their power and sampled as the local
  lights are (one chosen by its power, one shadow ray), or through ReSTIR's direct-light reservoirs (RTXDI is already in
  the build for the GI's). Workstream 7's step 3 comes first: `EmiGlow` has no material in the rays today.
- New emitters « where a visible object is one » (RT-PT-PLAN §8): a window's glass, a lantern's pane. A census of the
  candidate materials by family is not made (§8).

## 3. The ambient

### 3.1 What the original's ambient is

- **The lightmaps** (family DOL, 89 to 97 % of the map pieces): a baked irradiance with a dominant direction, scaled by the
  area's Dominant Light rows (0.2 to 1.2 over the 17 areas that have them, WORKER) (I-0126 STUDY-R7-DS3-AMBIENT §3.1). Their
  textures are not decoded here (the maps' texture archives are recovered as headers only).
- **The probes** (WORKER, [CENSUS-BTPB-PROBES](CENSUS-BTPB-PROBES.md); 22 banks decoded by hand from the engine's reading,
  every size relation matching): 72 bytes a probe, an L0 and L1 SH per channel and a weight that is 0 or about 1 and
  separates open from covered places (INFERRED: an authored sky exposure).
  - Level: open-air probes' median L0 luma 0.104 (High Wall) and 0.164 (m40's west part), covered ones 0.031 and 0.030:
    under cover the ambient is 0.30 and 0.18 of the open one. The up-facing term of an open probe: (0.262, 0.283, 0.382) at
    the High Wall, (0.235, 0.264, 0.363) at m40; its down-facing term 0 and 0.077.
  - Direction: an open probe's light comes from above (96 % at the High Wall); a covered probe's mostly from below (51 to
    54 %: the bounce off the floor).
  - Colour: blue-grey, the same in open and covered places (median R/B 0.686 both at the High Wall; 0.66 and 0.62 at m40);
    83 to 100 % of the probes « cold » in 19 of 22 banks. Exceptions: Ariandel a deep teal, the two Ringed City banks
    neutral or exactly grey, one warm pocket at the High Wall.
  - By zone (median over the valid probes; open and covered medians in the annex §5): the darkest Irithyll Dungeon (0.001,
    covered 0.0008), the Cathedral of the Deep (m35: 0.028, covered 0.0046), Ariandel (0.028), the Dreg Heap (0.023); the
    brightest m46 (0.234), Archdragon Peak (m32: 0.140), m41 (0.111). The Road of Sacrifices (m33): open 0.115, covered 0.053.
  - Firelink Shrine itself has no probe in the recovered bank (the nearest is 123 m away): an object there takes the
    hemisphere, whose intensity is 0 in 12 of m40's 15 ids (INFERRED consequence; a second bank not recovered may exist).
- **The hemisphere** (OBSERVED for m30, m33, m40 decoded by hand; WORKER for the rest): colour² times the 4th float.
  Firelink's set 0: (1, 1, 1) × 0; the High Wall's: 0.25 up and down; the Road of Sacrifices': 0.055 up, 0.25 down. Over the
  344 area-id rows of the 21 base files the 4th float is 0 on both colours in 180. It is a fallback for an object without a
  probe, not the authors' sky.
- **Light 1 of the sets**: a blue directional, (0.60 to 0.71, 0.75 to 0.84, 1.0) at an intensity of 0.05 to 0.3, against a
  sun of 0.8 to 1.6 (WORKER table, three areas OBSERVED): the authors' sky fill on the side the sun leaves dark.
- **The fixed ambient under the shadows**: the floor `FC_ShadowColor` under the sun's shadow, removed with the rest.

So the original lights its shadows with four authored terms (lightmap, probe, light 1, the shadow floor), all cold, all removed
by D-0262, and the traced frame has one term in their place: what the rays bring back.

### 3.2 Why the rays bring back too little

- **The sky is taken as it is drawn.** LIGHT-UNITS §4 measured the High Wall's sky at 0.02 to 0.07 before the exposure,
  with sunlit surfaces at 0.05 to 0.2 (OBSERVED in a traced HDR, region statistics). An up-facing surface under an open sky
  of radiance L gets an ambient of L in the units GB4 and the probes use (the GI's cosine ray cancels Lambert's term:
  `rt_reflect.hlsl`, « GB2 × (1 − F0) × L »). The authors' open-air ambient is 0.27 to 0.29 (§3.1): **four to fourteen times
  what the drawn dome gives** (INFERRED: both figures read in the same units by the formulas; the dome's cosine-weighted mean
  and the probes' value at one place were not compared in a run: TO CHECK with the witness).
- R7b's measure agrees: at Firelink the GI with and without its sky misses gave the same mean level, 61.44 and 61.43, and
  57.7 and 57.9 (I-0126 RESULT « R7b »): the dome's light is within the noise of the picture.
- A real sky is not that dim either: under a clear noon the sky gives about a tenth to a fifth of the sun's illuminance,
  far more under an overcast or a low sun, DS3's usual weather (connaissance générale). DS3's own ratio, 0.27 against a sun
  of about 1.1 at the High Wall, is a plausible evening sky; its drawn dome is an art picture, darker than what it stands for.
- **The bounce is there.** The cache gives the hits their own ambient (an unbounded bounce in effect), the PT level two
  path vertices by default: nothing suggests missing bounces; R11d's interiors kept their level without the cheats where
  local lights are near. What a closed interior lacks is §3.3's.
- **The albedo**: DS3's diffuse textures carry painted shade, so every bounce returns less than a de-lit texture would
  (workstream 13's; INFERRED, not measured).

### 3.3 The levers, in the order the evidence gives them

**A. The sky as a source at its authored strength.** The rays that reach the forward sky take its radiance times a factor
that brings its light to what the authors baked, the drawn dome unchanged (or changed too: Thomas's, §7 question 1).

- One rule for the whole game, with authored inputs, as the sun's colour is authored by area: the factor is the ratio of
  the zone's open-air baked ambient (the probes' up-facing term, read through the engine's own probe query, which the
  native already calls, or the lightmaps' GB4 on sky-facing pixels) to the dome's cosine-weighted mean radiance, both
  measured in the frame's own units. No value typed by zone.
- What it serves at once, through code that exists: the GI's misses, the cache, the probes of the forward rows and the
  effects, ReSTIR's reused misses. The player under an open sky, the Road of Sacrifices' moss, Father Ariandel's limbs under
  the chapel's opening are the witnesses (STUDY-ALL-GAPS rows 1, 2, 4).
- Where the specular lobe's ray meets the sky, the picture of the sky should stay the drawn one (a puddle shows the sky the
  player sees): the factor belongs to the diffuse paths. A path mixes lobes at the PT level: the pilot's design, said here
  so it is not missed.
- Its cost in noise: a brighter sky seen through a small opening is the classic hard case of a one-ray GI; ReSTIR GI
  serves it at the PT level, the cache and Ray Reconstruction at the RT level; a direct sampling of the sky through the
  openings is the next step if a witness shows boiling (TO CHECK).
- Under D-0262 this adjusts a real source and creates none. It is still a sky that lights more than it shows: said to
  Thomas as such.

**B. The openings of the covered places.** With A, an interior takes the sky through every opening its geometry has. Where
the level design closed it (a door shut, a window painted on the wall), the original's cold interior ambient was the bake's:
its sky, its key light (one per bank, never loaded at run time; brightness 50 in Irithyll Dungeon's second bank), its
lights. Two honest answers, by family: the window's surface as an emitter where a visible window is the source (§2.6), or
the place left to its real lights and the exposure. The probes say which places the authors themselves kept dark: the
Cathedral's covered ambient is 0.005 and the dungeon's 0.001, so the Cleansing Chapel's light in the original is its placed
lights (62 of m35's 81 are cold points), not its ambient (INFERRED from the two censuses; the chapel's own probes and
lights are not isolated here).

**C. DS3's placed cold lights.** They are in the traced frame today, at DS3's strength. They are part of the cold look and
they are, for most, invisible fills (§2.2). Whether they stay is §7's question 2; the study's advice is to name them first
(§2.4), set the sky (A), then judge their removal by family on sheets, never before: D-0262 itself said the interiors must
not go black first.

**Not proposed**: the probes, the lightmaps or the hemisphere as a light (D-0262: no baked ambient standing in for a traced
answer); a sky model computed from the sun (a new art direction nobody asked for); the bake's key light recreated (it has
no visible source).

### 3.4 The witness to build first (no picture change)

- **For the map's pixels**: a statistic made on a traced frame from two things it already has: the GI's irradiance at the
  pixel before the albedo, and GB4 divided by (1 − F0). The log ratio and the two colours, accumulated by class: sky-facing
  normals or not, lightmapped or SH (the G-buffer tells), near or far. One row a zone in the journal, a false-colour view
  for a sheet. It says, without the original running, how far the traced ambient is from the authored one and in which
  colour, before and after each lever.
- **For the characters and objects**: the engine's baked SH rows for an object (its own probe pick, still produced) beside
  the traced probe's rows R9 writes in their place: the same statistic by object. R9a's first reading already found « the
  traced ambient of DS3's order, warmer » on four characters (I-0129 RESULT).
- **Offline**: the probes' tables by zone (the annex) are the authored targets where no run is taken.
- It is a measuring tool: it lights nothing, and its numbers are evidence for Thomas's choices, not targets to hit pixel
  for pixel (D-0249).

### 3.5 The cases noted for this workstream

| Case | What the evidence says | Lever |
|---|---|---|
| The player dark and warm under the sky (Road of Sacrifices 46 against 87) | the cold terms removed; the placed lights and the sun bounce warm | A; witness on the player's probe |
| Father Ariandel's limbs | lit HDR dark over a right G-buffer; two window spots kept natively; an orange rim not identified | A through the chapel's openings, B; then the rim (a set's light 1 to 3, or an effect light: the family names will say) |
| The Cleansing Chapel | covered ambient 0.005 in the original: the look is the placed cold lights and the fog | name the bank lights there; C; the fog (§4) |
| Archdragon Peak's castle (m32) | among the brightest authored ambients of the base game (0.140; open 0.177); natively 60 % of the original's level with the automatic exposure of the time, 84 % without it (STUDY-ALL-GAPS row 1, before D-0280) | A |
| Firelink's interior | no probe in the recovered bank; a blue-grey on the back walls in the original (the lightmap, INFERRED) | A through the open roof and arches; the witness on GB4 there |
| André's grey hair | a forward row lit by its traced probe | re-look after A (Thomas's note) |

## 4. The fog

### 4.1 DS3's fog (WORKER for the engine side; the shader formula OBSERVED in the native transcription `RtForwardFog` and `FogRecord`)

- One record of 11 parameters by area and id (`Fog`: Color, heightColor, Dist, heightDist, DistOffset, HeightOffset,
  heightFogDensity, heightFogIntencity, ToUpper, Alpha, Contrast), built into six constant rows by `14014E900`, blended
  between two records on an area fade (`14014E5D0`). The same rows feed the composite, every forward programme, the sky
  domes and the effects' programmes.
- A pixel at the horizontal distance d from the camera goes toward `Color²` by
  `sat(a − a · 2^(−(d − DistOffset) / Dist))`, a = `Alpha × Color.a`: an exponential medium that starts at DistOffset, halves
  the view every Dist metres and never hides more than a. Then a clamped linear ramp in height toward `heightColor²`. Both
  colours are multiplied by `1 + Contrast × (L − 1)`, L the pixel's own luminance: a bright pixel gets a brighter fog.
- Values (WORKER, 150 area-id rows): Dist 0 to 2,000 m, median 120; the cap a between 0 and 3, median 0.43; DistOffset
  median 13.5 m. Firelink: half every 60 m from 15 m, at most 0.256, colour² about (0.17, 0.21, 0.27). The Road of
  Sacrifices: half every 30 m from 15 m, at most 0.35. Irithyll: Dist 1,000 with a cap of 0.7.
- No light enters the formula: not the sun's direction, not a shadow, not a local light. The fog is a fixed colour laid
  over the picture, the same in a sunlit yard and in an unlit cellar of the same area. Under a lighting that is otherwise
  true it is the one term that still adds light where there is none: the grey or cyan veils over dark views, the « Firelink
  veil » R7 left unexplained (its fog is the blue-grey above, INFERRED the source), the milky interiors of R11h's sheets
  when the exposure lifted them.
- Nothing else in the original is volumetric: the light shafts are screen-space (YEBIS) or effect draws (FXR 10003).

### 4.2 Elden Ring's fog in its data (WORKER, [ER-LIGHT-FOG-VALUES](ER-LIGHT-FOG-VALUES.md) §4; its code in §5.3)

- The group `Fog` no longer exists. `VolumetricFog` (48 parameters) is in 236 of 295 files and on in every gameplay file
  that holds it; an « analytical fog » beside it is on in 16 of 150.
- The grid: `VolumeWidth` 96 (100 of 157 rows) up to 160, `VolumeDepth` 24 to 40 slices, from 0.1 m to a far depth of 100
  to 2,000 m, the fog ending at up to 10,000 m.
- The medium: a fixed density (0 in half the rows, at most 0.01) and a height law (a distribution, a density scale of
  0.0004 to 0.12, an offset); one phase coefficient, `ScatteringG` 0.2 everywhere; absorption 0.
- The light: the directional light named by a bank and an id, with **authored fog colours** for the side facing it and the
  opposite side (`DirectColor`, `OppositeColor`), 8 shadow samples with a dither, a 4-phase jitter, a reprojection with a
  feedback of 0.8; a sky colour (`SkyLight SkyColor`), authored and keyed by the time of day; the local lights through one
  scale (1 in most files) and a distance scale (0.5).
- `FogVolume` (143 files): presets of placed volumes with their own ray march (8 to 64 samples, steps of 0.1 to 0.2), a
  Perlin noise density that scrolls, and for 109 of 886 rows their own light.
- So ER's fog is lit and shadowed, by the sun's shadow maps, in colours its artists typed. A traced DS3 has the visibility
  and the light itself: the grid and its passes are the model, the typed colours are what the rays give.

### 4.3 A volumetric fog for the traced DS3 (INFERRED design, nothing built; values for sheets)

**The shape (ER's, D-0195).** A view-aligned volume of cells (ER's 96 × 54 × 24 to 160 × 90 × 40 as the low end; 64 slices on
an exponential spacing if the cost allows), three compute passes and one application:

1. **The medium and the light a cell**: the density from the area's Fog record (below); the in-scattered light from
   - the sun: one traced ray a cell and a frame toward the frame's sun from a jittered point of the cell, in the place of
     ER's 8 shadow-map samples; a phase function of one coefficient (ER's 0.2 as the start);
   - the local lights: the lights of the cell's entry in the world light grid the hits already use (`rt_light_grid.h`),
     unshadowed in a first tranche, then one chosen by its strength with its ray, as the hits do at the PT level;
   - the ambient: one ray in a uniform direction a cell and a frame, its hit lit as a GI hit is (the cache at the hit) or
     the sky of §3.3: the kernel R9's probes already run by box.
2. **The history**: each cell reprojected into the previous frame's volume and blended, reset on the velocity's cut rule.
   ER reprojects only its sun shadow and its placed volumes' density (feedback 0.8) and rebuilds the scattered light each
   frame (§5.3); with one ray a cell the traced terms need the history themselves (0.9 to 0.95 is the usual range,
   connaissance générale): the price is a lag behind a moving light, to judge on a torch carried through fog.
3. **The integration** front to back: for every cell the light scattered toward the eye up to it and what is left of the
   view behind it.
4. **The application**: for the deferred pixels a native pass right after `GXLightComposite`, whose own fog rows are zeroed
   for the main view (« a zero entry leaves the colour », `RtForwardFog`'s comment), before Ray Reconstruction's copy as
   today's fog is. For the forward rows, the sky domes and the effects, whose programmes are the original's and read
   `cbFog`: their rows rewritten per draw from the volume at the object's place (the medium's own law for the amount, the
   volume's scattered colour for the colour), the route R9 took for their ambient. A view the rays do not serve (a menu
   view, D-0301) keeps DS3's fog.

**The medium from DS3's data** (one universal rule, authored inputs):

- Extinction `ln 2 / Dist` a metre, uniform, in place of the camera-centred start at DistOffset; the height term as a
  layer anchored in the world at `HeightOffset` in place of a ramp that follows the camera's height. Both changes move the
  near view (no fog-free bubble around the camera, a layer the player can walk out of): for Thomas's eye.
- The cap a: a real medium has none. Kept as a floor on what the volume lets through, so the distant scenery stays as
  visible as the authors left it; said as a fidelity clamp, not physics.
- The scattering colour: the authored fog colour is what the medium returned under the area's daylight. The medium's
  albedo is that colour divided by the light the same medium receives in open air under the area's sun and sky (A's
  strength): then the open yard keeps its authored fog colour, and a shadowed, covered or night place gets the darker fog
  its light gives. The `Contrast` term (the fog brightened by the pixel behind it) goes: it was a stand-in for light.
- Several records in one frame (the set is per pixel, and an area fade blends two): the volume takes the camera's area's
  record, a fade its blend; a pixel whose set names another record is the case to witness first (how many per frame:
  TO CHECK in the journals).

**What it gives**: the sun's shafts through arches, trees and windows from the traced visibility itself; halos round the
flames in a foggy place; dark fog in dark places. **What it puts in question**: the screen-space light shafts (12 of 18
areas, drawn inside our bloom since I-0138) and the effect shafts 10003 would double real ones: kept as authored art,
retuned, or retired where the volume makes them is Thomas's (§7 question 4; D-0276 already rewrote them once « no bridge »).

**Cost** (INFERRED, to measure): 96 × 54 × 40 cells are about 207,000 rays for the sun and as many for the ambient a frame,
a tenth of one full-screen pass at 1920x1080; the application is one sample a pixel.

**Not in the first version**: placed fog volumes with noise (ER's `FogVolume`; DS3 has no such data, its local mists are
effect sprites), multiple scattering, the fog in the reflections' and the GI's legs (the reflections keep DS3's formula
along their leg until the volume is trusted).

## 5. Elden Ring, subject by subject

### 5.1 The light sets (WORKER)

- The same four directional lights and one hemisphere pair as DS3, in 26 named parameters; `giOcclusionRate` removed,
  nothing added.
- The sun's intensity (its colour's 4th float, INFERRED) has a median of 0.35 over 2,656 gameplay rows.
- **The fills are mostly off**: lights 1, 2, 3 at a zero intensity in 73 %, 96 % and 69 % of the rows; with the intensity
  applied their medians against the sun are 0.013, 0 and 0.014.
- The hemisphere's intensity is 0.1 in 81 % of the rows, its lower colour black in 64 %; against the sun its median is
  0.144 (0.105 on the overworld).
- No lightmap group (`Dominant Light` is gone), no probe, sky or ambient group in the files. ER's ambient is a baked
  irradiance volume (INFERRED, [ER-VOLUMETRIC-FOG](ER-VOLUMETRIC-FOG.md) Q3): the executable holds a bake with the states
  Voxelize, Injection, Bounce, ComputeSh, Denoise, Encode, its outputs `%s.ivInfo`, a repository of them by map, and each
  light carries « Irradiance Volume Direct / Indirect Light / Scale » and « Bake Shadow » rows. The `VOXELIZATION` stage I-0126's
  ER study left open serves that bake (INFERRED, its executor not found), not the fog and not a run-time GI. So ER still
  bakes its ambient; DS3's rays replace what ER bakes.

### 5.2 Its ray tracing (WORKER, with I-0109 and I-0126 STUDY-R7-ER-GI)

- The `RayTracing` group (47 files: default, mapbase, 45 cutscenes): the directional light's shadow (1 ray a pixel) and an
  ambient occlusion (2 rays in 216 rows, 4 in 24), denoised with a feedback of 0.9. No reflection, no GI, no fog. Nothing
  to copy for the ambient (D-0236); the values answer the « TO CHECK » of I-0126's ER study.

### 5.3 Its volumetric fog in code

WORKER, [ER-VOLUMETRIC-FOG](ER-VOLUMETRIC-FOG.md) (headless Ghidra on ER 2.7.1, 47 functions; addresses there). The shader
bytecode is not in the executable: programme names, bindings and the CPU side only.

- **Where it lives**: no class of its own; a member of `GXSR::GXSkySimulation`, one Execute a view (`141c5cc30`).
  Programmes `GXVolumetricFog_*`: 14 compute, 1 vertex, 4 pixel.
- **The volume**: `VolumeFogTex` RGBA16F, w × h × (d + 2); w and h multiples of 8 from `VolumeWidth` and the aspect, at
  least 16, at most a quarter of the screen and 640; d = `VolumeDepth` scaled by a quality ratio (config data, not
  reached), 4 to 128. Slices logarithmic in depth between `NearDepth` and `FarDepth` with `NearOffset`. Accumulation
  volumes beside it (a density, the local lights' light, the sun's shadow in R8 with a ping-pong pair).
- **The frame's steps, in order**: the constants and a Halton jitter (bases 2, 3, 5 over `JitterCount` frames); the clears;
  an optional per-column maximum scene depth (culling); **the local lights, one compute dispatch a light** over its cells
  (`WriteLocalLightPoint`, `WriteLocalLightSpot`); **the sun's shadow by cell** (`DirLightShadow`, or its reprojected
  variant with a post-filter), `ShadowSampleCount` samples from what looks like the sun's cascades (INFERRED); the
  in-scattering write (three variants: unshadowed, shadowed, pre-shadowed); **the solve**, one thread a column (the
  integration along the depth, INFERRED); **the application**, a full-screen composite on the scene colour from the volume
  and the scene depth.
- **Its place**: after the deferred lighting (`DrawTiming` 0) or after the water (the shipped value in most files); the
  forward, water and transparent draws get the volume bound at a scene texture slot (a default texture when the fog is
  off) and fog themselves; a separate pass removes the in-scattering the screen-space reflections picked up.
- **Which lights**: the sun with colours typed in the fog's own parameters (`DirectColor`, `OppositeColor`), its direction
  looked up from the light set; **every point and spot light within a sphere round the camera whose `ScatteringScale`
  (a per-light field ER added, with a per-light phase override) is above 0**, its colour times that scale and the fog's
  `LocalLightScale`, without any shadow; a constant sky colour for the ambient; nothing emissive.
- **Placed volumes** (`GXFogVolume`): oriented boxes from a game entity, injected into the cells as density by a compute
  pass before the solve, and also drawn by a reduced-resolution ray march after the water.
- **No other fog**: no `Fog` group, no fog constants; the volume's group carries an analytic height fog for the distance
  beyond the volume.

What a traced DS3 takes from it (D-0195: the place and the shape; D-0236: not the quality): the logarithmic cell volume
tied to the screen in multiples of 8, the order injection → shadow → scatter → solve → composite, the composite after
the deferred lighting with the volume offered to the forward draws, the per-light scattering scale as a family setting
(§2.4 gives the family). What it does better: the sun's and the lights' visibility by rays instead of shadow maps and
none, the fog's colours from the light instead of typed values, the ambient from the traced sky and bounce instead of a
constant. ER's known weak points here: unshadowed local lights leak through walls in fog, and the typed colours can
disagree with the scene's light (INFERRED from the code read, not from play).

## 6. A proposed order for workstream 12

Each tranche merges alone with before and after sheets beside the original (D-0181, D-0268); the levers are test flags
until Thomas has seen them. After workstream 11 and workstream 7's step 3 (ROADMAP).

| # | Tranche | What it changes in the picture | Its witnesses |
|---|---|---|---|
| 1 | Names and witnesses: the light families in the collect and the journal (§2.4), the ambient's witness (§3.4), the FXR-to-family table (a Sonnet census) | nothing | the witness's table over the tour's zones (one slot of short runs), the families at Firelink, the High Wall, Irithyll, the Cathedral |
| 2 | The sky as a source (§3.3 A) | open-air shade, the characters under the sky, the interiors' openings | the High Wall, the Road of Sacrifices, Archdragon Peak, the Cemetery of Ash, Ariandel's chapel; the player's back; Father Ariandel |
| 3 | The covered places (§3.3 B, C): emitters at the visible windows, the placed lights by family | the interiors Thomas named | the Cleansing Chapel, Firelink's interior, Anor Londo's hall, Irithyll Dungeon |
| 4 | The flames' shapes (§2.5) | the softness of the fire-lit shadows, the highlights | Firelink's bonfire and candles, a torch corridor of the High Wall |
| 5 | The emissive triangles as sources where a witness asks (§2.6) | the lava's and the crystals' light | the Smouldering Lake, a crystal cave |
| 6 | The volumetric fog (§4.3): a) the volume with the sun and the sky on the deferred pixels, b) the local lights, c) the forward rows, the sky and the effects, d) the light shafts' fate | the veils, the shafts, the night and interior fog | the Road of Sacrifices, Irithyll, the Cathedral, Farron Keep, 45000030's cutscene |
| 7 | The cutscenes' provisional fills removed (D-0279, D-0282), the dead rows and constants of the removed terms cleaned, the zones' tour and the cutscenes again | the cutscenes' faces | the tour's reference (D-0284) |

The fog comes after the sky because its albedo is set against the sky's strength. Tranche 1 can start while R closes
workstream 11: it changes no picture.

## 7. What is Thomas's to decide

1. **The sky that lights more than it shows.** The authors' sky ambient is four to fourteen times its drawn dome's
   radiance at the High Wall (INFERRED). Give the rays the authored strength and leave the dome as drawn (the study's
   advice, on sheets), brighten the drawn sky too, or keep the drawn radiance as the only truth?
2. **DS3's own invisible fills.** Most placed lights have no visible source (80 % at the High Wall; 88 named Fill or
   Fake). Keep them as the game's authored lighting, or remove the sourceless ones by family once the sky is in (the strict
   reading of « jamais une lumière d'appoint invisible »)? The study's advice: name them, set the sky, then judge on sheets.
3. **Windows as emitters** where a visible window is the source of a closed interior: agreed in principle since RT-PT-PLAN
   §8; the census of the candidate surfaces is to make.
4. **The fog's fidelity**: the cap kept as a clamp and the colour set against the area's daylight (the advice), and the
   screen-space and effect light shafts kept, retuned or retired where the volume draws real ones.

None blocks tranche 1.

**Thomas's answers** (the study's thread, 2026-11-22 15:30 Paris): « 1 et 2 je suis ta recommandation » (the rays take the
sky at its authored strength, the drawn dome left as it is, on sheets; the placed lights named first, the sky set, then
their removal by family judged on sheets, nothing removed before); « 3 oui, elles reflètent la lumière extérieure » (the
windows as emitters agreed, and their light is the outside's: the zone's sky and sun at that window, not a typed value);
4 asked back (« Tu recommandes quoi ? »), the study thread's advice posted right after, and his « Ça me va » at 15:31: the
cap kept as a clamp, the fog's colour set against the area's daylight, the
screen-space shafts retired where the volume draws real ones (by zone, on sheets), the effect shafts 10003 kept at first
and looked at on the same sheets. Recorded as D-0307 ([decisions/LOG](../../../decisions/LOG.md),
[IN-FORCE](../../../decisions/IN-FORCE.md)).

## 8. Uncertainty and what is left to check

- The factor of §3.2 compares two figures taken in different ways (a region statistic of one traced HDR; the probes'
  median by bank). The witness of §3.4 replaces it by a per-pixel measure. Until then « four to fourteen » is an order of
  magnitude.
- The probes' weight read as « open » or « covered » is the worker's inference; so is the reading of the stored SH as a
  radiance projection. The probes' L1 vectors were read unrotated by the map's transform (up and down are unaffected).
- Whether the lightmaps hold the sun's direct term (I-0126's open point) decides how GB4 is compared on sunlit pixels: the
  witness should class sunlit and shadowed pixels apart.
- Firelink Shrine has no probe in the recovered bank; whether a second bank exists is not known.
- Only three MSBs are recovered: the emitter test is made at the High Wall and Irithyll (the latter through a fitted
  frame, the fit using the same parts as the test). The run-time rule of §2.4 does not need them.
- « No reader of LightScattering » and « the GI volume never draws » rest on missing programmes and all-default data, not on
  a traced gate; two code sites reading the bank lie in code Ghidra shows no function for.
- The spot item builder and the effect spot host were not read; FXR 11002's absence is the census's.
- Which fog record each pixel's set names within one frame, and how many records a frame holds, is not counted.
- The candidate emitter surfaces (windows, panes) are not counted by material family.
- ER: the meaning of a colour's 4th float as an intensity is inferred from DS3's formula; which fog ids a map refers to
  was not read.
- Every Japanese gloss of a light's or a region's name is a worker's reading.

## 9. Reproducing

Each annex names its scripts and inputs; the tables are in `C:/Users/User/Tools/DS3-Decompilation/study-ws12/`
(`ds3-census`, `btl-families`, `btpb`, `ds3-code`, `er-values`, `er-fog`). The pilot copies the ones its claims rest on under
its investigation's `evidence/<id>/` when it opens it. Inputs already in the evidence store:
`evidence/I-0066/queries/gparam-all/`, `evidence/I-0110/btl-census.tsv`, `evidence/I-0138/artifacts/er-values/`; the probe
and light banks in `C:/Users/User/Tools/DS3-Decompilation/gamedata-i0110/`.
