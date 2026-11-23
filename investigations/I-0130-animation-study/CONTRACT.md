# I-0130 - Étude : les animations de DS3 (Havok, TAE, behaviors) et ce que Claude pourrait en écrire

Capability: C3 (with ROADMAP's « Gameplay after Elden Ring », which plans to copy Elden Ring's animations). Study thread (read-only on
the code, never the pilot), opened 2026-11-08 on the coordinator's request after Thomas's question: « Je me demande si Claude serait
capable d'écrire de nouvelles animations pour DS3 en étudiant longuement celles du jeu de base » (relayed 2026-11-08 10:56 Paris);
Thomas, 11:03: « dans la roadmap il est prévu d'importer des animations d'ER », so the study compares with Elden Ring where it helps.

## Question

How are DS3's animations built (Havok skeletons and compressed curves, TAE events, Havok Behavior graphs, HavokScript), how does the
engine load, blend and play them, what does the repository and the community already read and write, and therefore what can Claude
write for DS3: retouches, blends, procedural motion, entirely new animations?

## Scope

Included: the game's animation files recovered read-only outside the repository (`chr/c0000` the player and `chr/c1100` a Hollow:
`anibnd`, `behbnd`, `action/script/*.hks`); DS3 1.15.2 static decompilation (GhidraMCP or headless, read-only); the native code as it
stands on `main` (read only); public community tools (read, nothing installed); small Python readers and writers in `tools/` that
need no Havok SDK.

Excluded: any change to `native/`, any build or game launch (two pilots are active; a run would go through the coordinator), a
reimplementation of Havok in the engine (an idea not retained, ROADMAP), the game's files or decompiled code on GitHub, Elden Ring's
data extraction (it needs new tools: Thomas's decision).

## Exit conditions

- Success: the formats and the engine path mapped with their statuses, the community tools listed with their limits, a frank answer
  per kind of animation work, and the next experiment that would turn the main uncertainty into an observation.
- Insufficient information: a part that static reading cannot answer is named with the run that would answer it.
- Budget: 4 cycles.

## Safety and human interaction

No live run in this study. The one experiment that needs the game (a written animation played in game) is proposed, not run.
