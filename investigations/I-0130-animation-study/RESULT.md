# Investigation result

## Conclusion

Complete (2026-11-08, the study thread): [STUDY.md](STUDY.md). DS3's animations are Havok 2014.1.0-r1 packfiles (skeletons,
`hkaSplineCompressedAnimation` clips with root motion in `hkaDefaultAnimatedReferenceFrame`), TAE3 event lists, Havok Behavior
graphs (the player's 12 712 objects, 7 026 clip generators, FromSoftware's `CustomManualSelectorGenerator` per animation id) and
compiled HavokScript. This study reads all of them without the Havok SDK and writes the clips back: `tools/hkx_write.py`, the first
open encoder for DS3's format as far as the community survey found (the community's route is a Havok SDK wrapper), re-writes clips
within 0.06° of the originals, by degree-1 splines with one control point a frame. The engine's own Havok runtime samples, blends,
maps (a skeleton mapper, INFERRED) and fires the events; new animations are data for it.

Frankly, by kind of work: retouches, re-timings (TAE), offline blends and recombinations of DS3's own motion, procedural passes
(IK, retargeting) written as clips, and the conversion of Elden Ring's clips (Havok 2018 tagfiles) are within Claude's reach; a move
authored from nothing reaches a keyframed draft, below an animator's quality, because Claude judges motion through stills and
curves and needs Thomas's eye in the loop. Elden Ring's animations (ROADMAP) are the better source of new motion.

## Evidence

- `evidence/I-0130/artifacts/game-data-recovery.json`: the 7 files recovered read-only (names, archive ranges, hashes).
- `evidence/I-0130/artifacts/hkx-census.txt`: `tools/hkx_census.py` on 576 clips (c1100 and `c0000_a0x`), 0 failures.
- `evidence/I-0130/artifacts/ghidra/`: the subagent's headless scans (the packfile magic's hits, the vtable constructors).
- The writer's read-back figures: STUDY §6 (outputs outside Git, `C:/Users/User/Tools/DS3-Decompilation/gamedata-i0130/out`).

## Uncertainty / unresolved questions

- Whether the game plays a written clip: not run (STUDY §8).
- The writer is checked by this study's reader only.
- The graph's update, the TAE walker and the HavokScript bindings are encrypted at rest or virtual: not read.
- Elden Ring: no ER file read (its extraction needs new tools, Thomas's decision).

## Artifacts produced or modified

- `STUDY.md`, `CONTRACT.md`, `tools/hkx_read.py`, `tools/hkx_census.py`, `tools/hkx_write.py`.

## Suggested next step

When animation work starts (ROADMAP « Gameplay after Elden Ring »): one announced run that plays an edited c1100 clip in game
(STUDY §8), which needs a loose-file override (a pilot's native work) or Mod Engine 2 (Thomas's decision); then an independent
reader of the written file (DSAnimStudio or HavokLib, a download); then a prototype of the standing jump, the simplest of the
ROADMAP's moves (a trigger in `c0000.hks` and a state in the player's graph over DS3's existing running jump; a native hook if the
jump request is tied to running in the encrypted modules). Thomas, 2026-11-08 11:17: done when that workstream starts, not now.
The other moves, by this study's estimate (2026-11-08 11:14, not prototyped): the jump attacks (new clips, TAE attack windows,
`AtkParam` rows), the guard counter (a TAE window after a block, an HKS state, its attack; its full value comes with ER's posture),
the crouch (the stance and its attack; the lower detection from the stealth SpEffects DS3 already has, to check).
