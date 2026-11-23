# I-0130 study: DS3's animations, and what Claude could write for them

Study thread, 2026-11-08 (Paris). Statuses as in [the ledger](../../knowledge/LEDGER.md): OBSERVED (read in the game's files or
measured here), STATIC (read in the decompiled executable), INFERRED (a reading not checked), PUBLIC (community sources, cited),
GENERAL (general knowledge). No game was launched. The game's files were recovered read-only outside the repository
(`C:/Users/User/Tools/DS3-Decompilation/gamedata-i0130`, the report with names, ranges and hashes in
`evidence/I-0130/artifacts/game-data-recovery.json`); nothing of them is in Git.

## 1. The answer in short

- **Reading: done here, for everything.** The skeletons and the compressed curves decode without the Havok SDK (this study's
  `tools/hkx_read.py`: 576 animations of the player and a Hollow, 0 failures, 3 million samples); the TAE events read and write
  with SoulsFormats; the behaviour graphs are Havok 2014 packfiles whose objects and links this study lists; the action scripts
  are compiled HavokScript that a public decompiler reads.
- **Writing a curve: done here, with one open check.** `tools/hkx_write.py` writes a DS3 animation file back without the Havok SDK
  (the community has no open encoder: its only route is a Havok SDK wrapper, §5). The trick: no curve fitting, one control point
  a frame on degree-1 splines, which DS3's own files already use. Read back, the re-written clips match the originals within
  0.06° (the 12-bit rotation quantisation). Not checked: that the game plays the written file (one announced run, §8).
- **So, frankly, by kind of work:**
  | Work | Feasible by Claude? | Why |
  |---|---|---|
  | Retouch an existing animation (a bone's angle, a pose held longer, a clip slowed, two clips spliced, a mirror) | **Yes**, high confidence once §8's run passes | the curves are numbers Claude reads and writes; the result is judged on renders and on the game |
  | Edit the timing: events, hit windows, cancels, sounds, effects (TAE) | **Yes**, today | SoulsFormats writes TAE3; the event meanings are the community's templates |
  | Blend or layer existing clips (an upper-body attack over a walk, a new transition) | **Yes** for offline blends written as new clips; **yes with care** for new behaviour states | offline: arithmetic on quaternions; in the graph: new clip generators and selector entries, the community's documented recipe |
  | Procedural motion (IK, look-at, procedural sway or recoil, retargeting a clip to another skeleton) | **Yes** offline (written as clips); **possible** at run time in the native module's pose hook, visual only | the engine already runs IK and twist modifiers; the native module owns the pose palettes (§4.4) |
  | Import Elden Ring's animations (the ROADMAP's plan) | **Yes for the format, uncertain for the result** | ER is Havok 2018 tagfiles, DS3 Havok 2014 packfiles: the curves convert with this study's writer; the skeletons differ by character and need retargeting; ER's TAE and behaviours do not copy as they are (§6) |
  | An entirely new animation, authored from nothing (a new attack, a new boss move) | **Partly**: a keyframed draft, yes; a motion of FromSoftware's quality, no, not alone | the hard part is not the file but the motion itself: weight, anticipation, follow-through, contacts. Claude composes motion as numbers, without seeing it move; it needs a render loop (§7) and Thomas's eye, and the result stays below an animator's or a motion capture |
  | Reimplement Havok Animation or Behavior in the engine | **Not in scope** | an idea not retained (ROADMAP); the game's own Havok runtime plays what is written |

## 2. What an animation is in DS3 (the files)

Read on the player `c0000` and a Hollow `c1100` (OBSERVED, `chr/*.anibnd.dcx`, `chr/*.behbnd.dcx`, `action/script/*.hks`).

| Piece | File | Format | What it holds |
|---|---|---|---|
| Skeleton | `anibnd` → `Skeleton.HKX` | Havok 2014.1.0-r1 packfile (magic `57E0E057 10C0C010`, file version 11, 64-bit pointers) | `hkaSkeleton`: the bones (c0000 and c1100: 104 bones, root `Master`; foot targets, `RootPos`, twist and skirt helper bones), parents, the reference pose |
| Animation clip | `anibnd` → `aXXX_YYYYYY.hkx` (the player's split by family: `c0000_a0x.anibnd` …) | same packfile | `hkaSplineCompressedAnimation` (the curves), `hkaAnimationBinding` (track → bone), and in 351 of the 576 clips read an `hkaDefaultAnimatedReferenceFrame`: the **root motion**, one (x, y, z, yaw) a frame |
| Events | `anibnd` → `aXX.tae` (player: 255 TAE files; c1100: one) | TAE3 (FromSoftware) | per animation id, a list of events (start, end, type, parameters): c1100 320 animations, 5 157 events; the player's `a00.tae` 1 720 animations, 37 506 events |
| Behaviour graph | `behbnd` → `Behaviors/cXXXX.hkx` (+ the project and character files) | Havok Behavior 2014 packfile, and the same as binary **tagfiles** `.hkt` (magic `1E0DB0CA CEFA11D0`), which carry their own type descriptions | the state machines that choose and blend clips |
| Action script | `action/script/cXXXX.hks` | compiled HavokScript (Lua 5.1 bytecode with Havok's extensions, header `1B 4C 75 61 51 0E`) | the logic that decides which behaviour event fires (`ExecEvent`, `SetVariable`, `ExecEventHalfBlend`, …) |

### 2.1 The curves (hkaSplineCompressedAnimation)

OBSERVED with `tools/hkx_census.py` on 576 clips (`evidence/I-0130/artifacts/hkx-census.txt`):

- 30 frames a second (frame duration 0.0333 s), 2 to 341 frames a clip, median 41; blocks of at most 256 frames, consecutive blocks
  sharing one frame (5 clips have two blocks; the frame read from either side differs by at most 0.0014 in quaternion units).
- One track a bone (101, 104 or 110 tracks). Each track has a 4-byte mask: the quantisation (positions 16 bits, rotations
  `THREECOMP40`, scales 16 bits: the byte `0x45` in every clip), then for position, rotation and scale whether it is absent
  (identity), static (one value) or a B-spline.
- The rotations: 35 266 tracks a spline, 20 830 static, 5 030 identity; every rotation is `THREECOMP40` (5 bytes: three 12-bit
  components, the dropped one's index and its sign). Positions are mostly static (51 139 against 3 959 splines): bones keep
  their length; translation lives in the root and a few helper bones. Scales almost never move (560 splines, the cloth and skirt
  helpers).
- Spline degrees: 26 212 of degree 1 (linear), 13 573 of degree 3 (cubic). The knots are bytes (frames inside the block).
- About 344 bytes a frame for a whole body.
- The decode is right: 2 994 383 rotation samples, the worst quaternion norm 1.0135 (a cubic between unit control points is not
  exactly unit), the median clip exactly continuous across its blocks.

### 2.2 The events (TAE)

OBSERVED with SoulsFormats 1.0.1's `TAE3` (the repository's pinned copy). An animation id `aXXX_YYYYYY` is TAE section `XXX`
(the player's weapon and style families: `a00` common, `a03` …) and animation `YYYYYY`; the TAE row names its clip file
(`a000_003006.hkt`) or imports another animation's (4 such references in c1100, 36 in the player's `a00`). The events' types by
count in c1100: sound (types 128-132) 1 965, jump tables (type 0, the windows where the script may cancel or chain) 1 264,
224 (440), 700 (332), 16 (270), effects 96 (131), 112 (111), attack behaviour 1 (44: the hit windows), … An attack such as
c1100's `3006` is 66 events: per swing an attack window (type 1, about 0.1 s), a swing sound (131) and the jump tables around them.
SoulsFormats reads and writes them; its write is not byte-identical (it relays the tables out) but keeps every event (c1100:
618 080 → 631 952 bytes, 5 157 → 5 157 events; `a00.tae` 37 506 → 37 506). The parameters' meanings are the community's
templates (DSAnimStudio, §5), not in SoulsFormats 1.0.1, which names few types.

Gameplay sits here: the hit boxes are not in the curves but in the TAE's attack events (type 1, with an `AtkParam` row) placed at
the weapon's dummy points, which follow the hand bone. A new swing is therefore a new curve **and** new attack windows.

### 2.3 The behaviour graphs (hkb)

OBSERVED by class count (`tools/hkx_read.py`'s object table): the player's `Behaviors/c0000.hkx` is 12 712 objects:
7 026 `hkbClipGenerator`, 2 589 `CustomManualSelectorGenerator` (FromSoftware's: one per animation id, it holds that id's clip
generators and picks one by the weapon family), 1 091 states in 136 `hkbStateMachine`, 45 `hkbBlenderGenerator` (389 children),
21 `hkbLayer` in 7 layer generators (the upper/lower-body partial blends: `PartBlend_Add`, `PartBlend_NoAdd`), 66
`hkbScriptGenerator` (the HavokScript callbacks), IK and look-at modifiers (`hkbFootIkControlsModifier`, `hkbHandIkControlsModifier`,
`CustomLookAtTwistModifier`, `hkbTwistModifier`). A clip generator is named `a000_009500_hkx_AutoSet_06` and plays the animation
`a000_009500`: the link between graph and clip is that name. The Hollow's own graph is 20 objects that reference two shared humanoid
graphs `c9996` (666 objects) and `c9997` (3 499): enemies of one build share their behaviour.

### 2.4 The action scripts (HKS)

OBSERVED from the bytecode's strings: the player's `c0000.hks` (793 KB) holds 543 state callbacks (`AttackBothHeavy1Start_onUpdate`
…), 289 behaviour events (`W_AttackBothDash` …) and 116 engine functions (`ExecEventAllBody` 119 calls, `GetHalfBlendInfo` 142,
`SetVariable` 73, `ExecEvent` 66, `ExecEventHalfBlend` 64 …). c1100's is 11 KB: `Event20000_onUpdate` to `Event30005_onUpdate`;
its AI (ESD and Lua AI) decides, the script forwards. A new player move needs a callback here, or the state stays locked (PUBLIC,
the Souls Modding wiki, §5).

## 3. Elden Ring next to DS3 (for the ROADMAP's import)

| | DS3 | Elden Ring | Consequence for an import |
|---|---|---|---|
| Havok | 2014.1.0-r1 (the executable's only version string, OBSERVED) | 2018.1.0-r1 (OBSERVED in `eldenring.exe`) | the classes are the same family; the files are not interchangeable |
| Container | packfiles (`.hkx`), the behaviours also as 2014 tagfiles | tagfiles `TAG0` (PUBLIC: HKLib « supports Havok 2018 ») | ER's clips are read by a tagfile reader (HKLib, MIT), then their curves written by this study's writer into DS3's packfile |
| Clip compression | spline, THREECOMP40 in every clip read | spline (PUBLIC) | the same decoder logic. Both executables hold the names of the interleaved, quantized, predictive, delta and wavelet animations (OBSERVED in their bytes), but DS3's runtime classes are nameless (§4.1): whether DS3 plays a non-spline clip is unknown, so DS3's own spline is the safe target |
| Run-time retargeting | Havok's skeleton mapper in the sampler (§4.3, INFERRED) | the same Havok family | a mapped clip may not even need offline retargeting: to check before building a retargeting tool |
| Skeletons | c0000 104 bones | ER's player has more bones (GENERAL; to count) | retargeting per bone name, with the twist and helper bones re-derived |
| TAE | TAE3 | TAE of a later version, more event types (PUBLIC) | events re-mapped by meaning, not copied |
| Behaviour | DS3's graphs, `CustomManualSelectorGenerator` | the same pattern, more classes (PUBLIC: ERClipGeneratorTool, HkbEditor) | an ER move becomes a DS3 animation id, registered in DS3's graph; ER's graph is not imported |

The community has done the BB → DS3 player version (PUBLIC: Nexus « Bloodborne Character Resource Dump (For DS3 player) »), with
hand fixes after retargeting; the DS3HavokConverter (The12thAvenger) converts 2018 XML to 2014 for physics, cloth and animation.
Extracting ER's files needs ER's archive keys, its name dictionary and the Oodle decompressor: new tools, Thomas's decision.

## 4. The engine: how DS3 loads, blends and plays them

Static map of DS3 1.15.2 by the `ghidra-research` subagent (GhidraMCP and two headless scans on `ghidra-mirror-i0066`, read-only;
its scan outputs in `evidence/I-0130/artifacts/ghidra/`), the sampler re-read by this thread. Class names are RTTI with the I-0067
vtable dumps (`evidence/I-0067/rtti/`); function roles are INFERRED from the vtable slot and the code's shape unless said.

### 4.1 Havok inside the executable, and FromSoftware's layer on it

- Havok 2014 is linked statically, with its Animation, Behavior, Physics, Cloth and AI products (the licence strings at
  `14282cc00..`, « FromSoftware_FDP_PC-PS4-XboxOne », 2016-05-27; STATIC). The reflection names (`hkClass`) are stripped: the
  runtime classes are known by RTTI only, and the concrete compressed-animation classes (spline, quantized, predictive) are reached
  through `hkaAnimation`'s vtable (`142a5a468`) without a name. So the executable alone does not say which compressions DS3 plays;
  the files say spline (§2.1).
- The Behavior runtime is complete: `hkbWorld` (`142a0ac58`), `hkbCharacter` (`142a0aea0`), `hkbBehaviorGraph` (`142a0afa8`), the
  clip, blender, layer, selector and state-machine generators, the transition effects, the IK, look-at, twist, mirror, pose-matching,
  parametric-motion and procedural-blender nodes, the ragdoll driver (all STATIC by RTTI). The FromSoftware classes of the data
  (`CustomManualSelectorGenerator`, `CustomTransitionEffect`, `CustomLookAtTwistModifier`) have no name in the executable: they are
  registered under other names or reached by an index (not resolved).
- FromSoftware's layer: `FrpgHavokManImp` (`1428329d8`), `FrpgDefaultAnimationControl` (`142841a08`, the game's
  `hkaDefaultAnimationControl`), `HvkAnim`, the loop and no-loop control listeners, `FrpgFootIkRaycastInterface`, `FrpgRagdollIns`,
  `SprjHkBehLoader` (`142967358`, the behaviour asset loader), `CSHkBehCharaCreater`, `HkBehVariableMap<…>` and
  `SprjBehaviorScriptAct::_SetVariable` (`142891e38`: how a script writes a graph variable), `ChrModel` (STATIC by RTTI).

### 4.2 Loading

The resource caps by extension: `AnibndFileCap` (`142791548`), `BehbndFileCap` (`142791e28`), `HkxFileCap`, `MovtaeFileCap`,
`LuabndFileCap`, `ChrbndFileCap`, the remo bundles' cap; the hkx repository `HkxRepositoryImp` and its `HkxResCap` (STATIC by name).
The packfile loader `14156dd00` tests the magic `0x57E0E057` (STATIC) and `140d6f3c0` wraps a loaded file, finding its objects by
class into four arrays (STATIC for the flow; which classes, INFERRED: skeletons, animations, bindings, reference frames). TAE has no
cap of its own: it sits inside the anibnd and is held by `TaeDat` (`142767cc0`). No code tests the « TAE » magic (a scan for the
constant found none): the TAE is read by offsets.

### 4.3 Each frame: clip, pose, events

- **Clip → pose** (STATIC, re-read here): `hkbClipGenerator::activate` (`141076510`) builds the animation control and sorts the
  clip's triggers by time; `generate` (`1410770b0`) sets the control's local time and calls the sampler. The control's
  `getTransformTrack` (`141285020`) calls the animation's `sampleTracks` (vtable `+0x20`) at the local time into a thread-local
  buffer of `hkQsTransform` (0x30 bytes: translation, rotation, scale) and, when the control carries a mapping (`control+0xB8`),
  starts from the target skeleton's reference pose and maps the sampled pose into it (`141273910`, with the binding's blend hint
  `binding+0x50`): Havok's **skeleton mapper**, so DS3 can play an animation authored on one skeleton onto another at run time
  (INFERRED from the layout; whether any DS3 character uses it is not read). `hkaAnimatedSkeleton::sampleAndCombineAnimations`
  (`1412819d0` → `141282ff0`) is the blend of the active controls (INFERRED; the worker not read).
- **The graph's update** (the step `HavokBehavior` 0x12 and the character modules `SprjChrBehaviorModule`, `…ScriptModule`,
  `…SyncModule`): their bodies are encrypted at rest (slot 0 a three-instruction stub jumping out of the image, the pattern of
  MENUS-MAP « Encrypted functions on this path »); readable only from a running process (the loaded-code route), not done here.
- **TAE events**: `SprjChrTaeAnimEvent` (`142894fd8`) is the characters' event handler; its slot 2 (`1409b2d10`, about 3 300
  decompiled lines, part encrypted) switches on the event type: type 0 the jump table (checked against the character's state),
  0x60 (96) the FFX spawn (`1409b3e60` reads the event and calls the character's FFX module at `ChrIns+0x1AE0`), and the sound,
  attack, SpEffect families; the event's progress is computed as (time − start) / (end − start) (STATIC). The walker that calls it
  for each event in the time window, and where an animation id finds its TAE, were not found (a virtual call).
- **HavokScript**: the VM is linked (the library name `havokscript` at `142a16070`, the opcode names `HKS_OPCODE_*`); the
  `hkbScriptGenerator` and `hkbScriptCondition` nodes call it; the registration of `ExecEvent` and `SetVariable` was not found (the
  names exist only in the compiled scripts).
- **Pose → palette** (I-0067 VELOCITY-SKINNING, STATIC): the draw entity's pose rows are written by `140015FC0` / `140016000` and
  published by `140016050`, the functions the native module replaces (§4.4). The link from Havok's pose to those rows was not read.

What the engine map changes for Claude's work: nothing in it needs replacing. The game's own Havok runtime samples, blends, maps and
fires events; new animations are data for it, written in its formats and registered in its graphs.

### 4.4 Where the native module meets the pose

The native renderer already owns the characters' pose storage: `InitializeNativeModelPose`, `UpdateNativeModelPose` and
`ComposeNativeModelPose` (`native/frame/game_bootstrap/05_selected_scene.inc:7`, `:41`, `:72`) receive each model's bone matrices
from the engine's pose update (`16000` / `16050`) and publish the palettes the vertex shaders read (current and previous, for the
motion vectors). A procedural layer placed there (a sway, a recoil, a look offset) would be **visual only**: the engine's Havok
side, the dummy points and so the hit boxes keep the unmodified pose. A gameplay-true procedural change goes through the curves or
through the behaviour graph's own modifiers, never through the palettes.

## 5. Community tools (PUBLIC; checked 2026-11-08 through GitHub's API and search)

| Tool | What it does for DS3 | Limits |
|---|---|---|
| DSAnimStudio (Meowmaritus, GPL-3.0, active 2026-05) | the viewer and the TAE editor; imports animations (FBX and others) and writes them through `CompressAnim.exe` | DS3's HKX export broke and was re-fixed across versions; the import path is called experimental |
| SoulsAssetPipeline (Meowmaritus, GPL-3.0) | the HKX classes (`hkaSplineCompressedAnimation`, `CustomManualSelectorGenerator`), the import code | the encoder is the binary `CompressAnim.exe` |
| horkrux/despacito (no licence) | the source of `CompressAnim.exe`: a wrapper of the **Havok SDK** | the Havok SDK is proprietary, not redistributable: **no open encoder exists**; this study's writer is one (degree-1, no fitting) |
| HavokLib (PredatorCZ, GPL-3.0) | reads Havok 5 to 2017 packfiles; the reference spline decoder | decompression only; no tagfiles |
| HKLib (The12thAvenger, MIT) | Havok 2018 tagfiles (ER) | not DS3's 2014 |
| DS3HavokConverter (The12thAvenger) | Havok 2018 XML → 2014 for DS3 | physics, cloth, animation through XML |
| DS3BehaviorTool (The12thAvenger) | registers new clips in `c0000.behbnd` (clip generators, CMSGs, selectors) | the player's graph |
| DSLuaDecompiler (katalash, MIT) | decompiles DS3's HavokScript | recompiling: not confirmed |
| Souls Modding wiki (« havok-behavior-editing », « the Muffin knowledge compendium DS3 ») | the recipe to add a clip, a state, an HKS callback; the BB import's pitfalls (missing jump tables, poise events, torso rotation at transitions) | wikidot; Discord holds the rest |
| Soulstruct (Grimrukh) | Blender animation import/export | DeS, DS1, DS1R only; DS3 FLVER only |

Mocap or procedural pipelines to DS3: none found. Retargeting exists (DSAnimStudio's `HKAnimRetarget`), hand-fixed afterwards.

## 6. What the writer does, and what it proves

`tools/hkx_write.py`: decodes a clip, optionally edits it (`--edit bone:T:deg` rotates track T by deg degrees about Y on every
frame, a retouch demonstration), encodes every animated channel as a degree-1 spline with one control point a frame (positions and
scales 16-bit between their minimum and maximum, rotations THREECOMP40 with the hemisphere kept continuous), splices the new data
array into the original packfile (the fixups and section offsets after it shifted, the block offsets rewritten) and reads it back.

OBSERVED (outputs under `gamedata-i0130/out`, outside Git):

| Clip | Frames × tracks | Bytes | Worst error read back |
|---|---|---|---|
| c1100 `a000_000700` (idle, 6.7 s) | 201 × 101 | 40 464 → 70 688 | 0.059°, 0.00001 m |
| c1100 `a000_001700` (with root motion) | 44 × 101 | 27 008 → 37 184 | 0.052°, 0.00001 m |
| c1100 `a000_003006` (attack combo, root motion, 2 blocks) | 270 × 104 | 76 224 → 121 744 | 0.062°, 0.00001 m |
| the same with `--edit bone:40:25` | 270 × 104 | 121 744 | 0.062° from the edited target |

It proves that Claude can write DS3's animation format without Havok's tools, at the game's own precision, files about 1.6 times
larger (no fitting; a cubic fit is the next refinement). It does **not** prove the game plays them: the reader that checks the
writer is the same study's, so a shared misreading would pass. Two independent checks: DSAnimStudio or HavokLib reading the written
file (a download, Thomas's decision), and the game itself (§8).

## 7. Why authoring from nothing is the hard part

GENERAL, and the frank part of the answer. A DS3 move is about 100 bones × 30 frames a second of curves that read as weight,
balance and intent: the anticipation before a swing, the follow-through, the feet planted while the hips turn, the root motion
that matches the steps, the hit window at the frame where the blade is fastest. Claude can write each of those as numbers and
check them as numbers (foot sliding against the ground, the weapon tip's speed, joint limits, the root motion against the feet),
and can learn the house style statistically from the 7 000 clips (the timings of each family, the poses at the hit frame). What
Claude cannot do alone is see the motion: it judges stills and curves (the project's rule: stills only, VIDEO-STUDY), so the taste
loop goes through rendered sheets of frames, the motion curves of `videosheet.py`, and Thomas watching it in game. Expected
results: retouches and recombinations of DS3's own motion at the game's quality; a new move built from pieces of existing ones
(a new combo from existing swings, a held pose, a mirrored attack) close to it; a move invented from scratch clearly below an
animator's, stiff in the ways keyframed motion without a trained eye is. Elden Ring's clips (ROADMAP) are the better source of
new motion: authored by FromSoftware's animators, they only need converting and retargeting, which Claude can do.

## 8. The next experiment (proposed, not run)

One announced run of about ten minutes: a written clip played by the game. The game reads its files from the archives; the project
has no file override yet. Two ways: a minimal loose-file override in the native module (the engine's file layer, the one
Mod Engine 2 uses: a pilot's work, reviewed) or, without code, one of the c1100 idle clips replaced by its edited version through
the community's Mod Engine 2 (an install: Thomas's decision). Observed: the Hollow plays the edited idle (the bone turned 25°), or
the game refuses or crashes on the file. The same run answers whether DS3 plays a degree-1 spline clip of any length.

## 9. Uncertainty

- The writer is checked by its own reader only (§6).
- §4's engine map is STATIC where it says so; the graph's update, the TAE walker, the HavokScript bindings and the root motion's
  use were not reached (encrypted at rest or virtual calls): the loaded-code route from a running process would read them. None
  of them is needed to write data for the game; they matter only for a native procedural layer that is gameplay-true.
- The run-time skeleton mapper (§4.3) is INFERRED from the sampler's layout.
- The subagent's map was spot-checked by this thread on the sampler only (`141285020`); a decision built on another of its
  addresses has it re-read first (WORKFLOW « Roles and delegation »).
- ER's figures are from its executable's strings and public sources; no ER file was read.
- The TAE events' meanings beyond the types named here are the community's.
