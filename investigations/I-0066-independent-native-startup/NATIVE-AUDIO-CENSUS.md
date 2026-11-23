# Native audio census (Thomas 2026-10-13: all the game's audio in the native path)

Status: **no native gap at the Undead Settlement, idle and in combat** (controlled pairs at the end); NPC dialogue and movie audio not covered. The first comparisons were confounded (below). Opened by the request in ROADMAP
(music, English voices, sound effects, ambience).

## Method (audio-census.flag, both modes)

- `FMOD::Event::start` exported by `fmod_event64.dll` (the FMOD Designer event API the game imports; starts that bypass this
  export are not covered), named through `Event::getInfo`: row `audio_event_name` per new name with the game-image return
  addresses, `audio_census` summaries by category letter (periodic, not final: count the name rows for totals).
- `FUN_1404AD2D0(out, category, id)`, which forms the sound names (category letter from the table `142786578`, nine digits)
  for its twelve callers: row `audio_request_name` per new (category, id) with the caller chain.
- `FUN_140E72D50(owner, position, request, filter, emitter)`, the characters' play step: row `audio_play_pair` per new
  (incoming request, formed name). The sibling branch of `FUN_140E72C20` (mode 2: filter then `86C010`) and `E72E40` are not
  probed.
- `fmod_event64.dll` is bound by the exported mangled names (which encode the signatures); it is not version- or hash-pinned.

## Runs and why they do not show a gap (2026-10-13)

Reference1563/1565/1567/1569 against Native1564/1566/1568/1570 (Shebang Beta, Undead Settlement, 150 s): the native path
started fewer names (about 22 against 53-58, about 370 starts against 540-610), mostly character sounds (`c124...` of the
nearby enemies, the player's `c0000...` hit and movement sounds, two `v` names, a few `s` and `z` names). Reference1571 and
Native1572 (screenshots 40 s apart, `t1.jpg`/`t2.jpg`) explain most of it: **in the original run the player stood outside
the shelter with an enemy beside him and then died** (souls 0, back at the bonfire), while the native player stood still at
the bonfire. The original runs were driven by `ref-title.sh` (keys and clicks on a timer), which reached the game sooner
and whose later clicks landed in game; their enemies' footsteps, hits and the player's death sounds come from that fight.
So the two modes did not play the same scene, and the comparison cannot attribute the difference to the native path. Other
corrections: the native path also starts a name the original did not (`z003020001`, Native1566/1568/1570), `c202105003/4` are
formed in both modes, the id rewrite in the play step (category 8 footstep requests leaving as category 1 with the ground's
digits) is seen in both modes, with the native pairs a subset of the original's (by filter or emitter, not decided), and
the native world update ran at about 30 Hz (dt 0.034) against 60 Hz.

What holds: the FMOD service, its thread and the menu, music (`m000000001`), object and ambient cues run natively, and the
characters' play step forms names natively.

## Next

Done below (controlled pairs). Initially planned: both modes driven by the same pixel-gated title pilot, no input in game, the census counted from the
first in-game frame over the same game time; then a listening run. Only a difference in that pair is a native gap, followed
to its producer by mechanism.

## Controlled pairs: no native gap (2026-10-13)

Both modes driven by `Pilot-TitleContinue.ps1` (pixel-gated; the original with `-Trace none -ContinueDelayMs 6000`, its main
menu appearing later), same Shebang Beta save at the Undead Settlement bonfire.

| Pair | Scene | Original | Native |
|---|---|---|---|
| Reference1573 / Native1574 | idle at the bonfire (screenshots `t1.jpg`: same place, still) | 495 starts, 22 names | 498 starts, 24 names (the original's 22 plus `c110008000`, `z003020001`) |
| Reference1575 / Native1576 | three attacks and a roll at 100/115 s, 50 s after | 47 names | 31 names: the player's swing, step and roll sounds equal; in the original a c1240 enemy came and fought 18 s after the attacks, not natively in that run |
| Reference1577 / Native1578 | the same with 60 s after | 756 starts, 55 names (a 8, c 481, o 229, s 5, v 2, y 4, z 15) | 764 starts, 52 names (a 8, c 433, o 283, s 5, v 2, y 2, z 19): the enemy fights in both; the remaining differences are the fight's own variation (names on either side) |

Conclusion: at this zone the native path starts the same sounds as the original, idle and in combat (music, ambience,
objects, the player's and the enemies' sounds, the two voice cues of the fight). Not covered: NPC dialogue voices (no NPC
here), other zones' scripted sounds, and movie audio (Bink mixes its own through XAudio2, outside FMOD, not counted
here). Thomas's listening during his next native play session is the remaining check.
