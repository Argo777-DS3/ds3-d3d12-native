# Wish: a dynamic HUD as Elden Ring's, shown in combat and hidden outside it

A wish of Thomas (the project chat, 2026-11-21 06:55 Paris): « J'ai une idée à rajouter au projet, l'interface dynamique comme
Elden Ring, ce serait bien mieux avec la démocratisation des écrans OLED. L'interface s'afficherait une fois en combat et en-dehors
il se cache. Faudrait inspecter la decomp d'ER j'imagine », then (06:57) « A inscrire dans le dépôt ». Not a decision; listed in
[ROADMAP](../ROADMAP.md) workstream 21, nothing in front of the open workstreams.

- Why: a HUD that stays on screen marks an OLED panel; Elden Ring's « Auto » HUD setting shows the bars and items in combat and
  fades them outside it (connaissance générale: the game's own option).
- First step, read-only: Elden Ring 2.7.1's decompilation (the ER Ghidra project, D-0195's read-only rule) for what decides
  « in combat » (the lock-on, an enemy's aggro, damage taken or dealt, the HUD's timer) and how its fade is driven; then DS3's own
  HUD (the in-game menu's film and its Group, the HP/FP/stamina bars) for where the same state can be read natively.
- Then: a row in the game's own HUD options (D-0271's rule: our rows in the game's tabs), « Auto » beside DS3's On and Off.
- Not before Thomas's go for a thread.
