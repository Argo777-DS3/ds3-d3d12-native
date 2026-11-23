# Sorts de contrôle : où le jeu range leurs effets (étude statique, 2026-10-29 21:05 Paris)

Copiée de `evidence/I-0105/control-spells/` dans l'investigation par le pilote (D-0214), 2026-10-29 ; la section « Ce que le natif
en fera » est du pilote.

Pour le pilote effets (I-0105). Lecture seule des fichiers du jeu 1.15.2 (exe EF5E07C5…), rien lancé, aucun build.
Statuts : OBSERVÉ = lu dans les données ; INFÉRÉ = déduit, non vérifié en jeu.

## Sources et outils

- Outils du dépôt : `investigations/I-0068-static-yebis-ffx-lights-spec/Read-Fxr.ps1` (vidage FXR3 annoté par le schéma @cccode/fxr),
  le recensement I-0063 (`evidence/I-0063/artifacts/scan/fxr.tsv`, `mtd.tsv`), les FXR déjà extraits
  `C:/Users/User/.codex/I0033-assets/effects/`, la méthode de `I-0067/Get-ParamRows.ps1` (régulation Data0.bdt déchiffrée en mémoire)
  et de `I-0068/Recover-NamedGameData.ps1` (BHD5/BDT par nom).
- Petits scripts de cette étude (copies dans `evidence/I-0105/control-spells/tools/`, hors Git) : `Recover.ps1` (ressources sfx, msg engUS, `chr/c0000.anibnd.dcx`),
  `Params.ps1` (Magic, Bullet, BehaviorParam_PC…), `Names.ps1` (FMG, liste des ressources), `Tae5.ps1` (évènements TAE de `a00.tae`),
  `Flv.ps1` (matériaux des modèles d'effet), `draws.py` (actions de dessin par FXR). Les octets du jeu restent dans le scratchpad de la
  session, pas ici.
- Tables produites : [control-spells-fxr-draw-actions.txt](control-spells-fxr-draw-actions.txt) (chaque FXR : actions de dessin, modèle, texture, blend, orientation),
  [control-spells-tae-a00-spawnffx.txt](control-spells-tae-a00-spawnffx.txt) (évènements d'animation qui appellent ces FXR).

## Trois endroits où un sort range ses effets (OBSERVÉ)

1. **Magic** (id = id de l'objet) : `+0x30` (et `+0x34`) = FXR d'incantation dans la main ; `+0x64`, `+0x68`, `+0x6C` = ids Bullet.
2. **Bullet** : `+0x04` = FXR du projectile (sfxId_Bullet), `+0x08` = FXR d'impact (sfxId_Hit), `+0x68` = Bullet enchaîné.
3. **TAE du personnage** (`c0000.anibnd`, `a00.tae`) : évènement type 96 (spawn FFX), `Unk00` = id FXR. Storm, Stake et Arrow y mettent
   leur effet principal ; anim 40xxx main droite, 42xxx main gauche (mêmes FXR, mêmes temps).

Tous les FXR sont dans `frpg_sfxbnd_commoneffects_effect` sauf Lightning Arrow (`frpg_sfxbnd_dlc2_effect`) ; leurs modèles dans
`frpg_sfxbnd_commoneffects_resource` (s08290 dans `dlc2_resource`).

| Sort (en jeu) | Magic | Incantation | Bullets → FXR projectile / impact | TAE (anim, temps) |
|---|---|---|---|---|
| Lightning Spear (référence) | 3550000 | 525500 | 13550000 → 525501 / 525504 ; 13550100 → 525502 / 525503 | — |
| Great Lightning Spear | 3551000 | 525510 | 13551000 → 525511 / 525514 ; 13551100 → 525512 / 525513 | — |
| Sunlight Spear | 3552000 | 525520 | 13552000 → 525521 / 525524 ; 13552100 → 525522 / 525523 | — |
| Lightning Storm | 3553000 | 525530, 525531 (vide) | 13553000…13553800 sans FXR projectile ; impact 525533 (13553300, 13553700) | 40336 : **525532** à 1,0 s |
| Lightning Stake | 3710000 | 527100, 527101 (vides) | 13710000 → 527102 (vide) → 13710100 → 13710200 | 40334 : **527103** à 1,07 s |
| Lightning Arrow (DLC2) | 3509000 | 528600 (vide) | 13509000 → 528602 / 528603 | 40050 (0-2 s) : 528601, 528604, 528606 ; 40150 (maintien, 0-4,8 s) : 528601, 528604, 528608 ; 40350 (tir, 0-0,5 s) : 528601, 528605, 528607 |

Bullet de Lightning Spear : 13550000 vit 0,1 s sur 3 m, 13550100 vit 0,5 s à 30 m/s (le lancer). INFÉRÉ : 525502 est la lance en vol
(le cœur droit s03000 et les traceurs), 525501 l'éclat de départ à la main.

## Familles d'effets par sort (OBSERVÉ dans les FXR)

Actions : 602 QuadLine, 603 BillboardEx, 604 MultiTextureBillboard, 605 Model (particule modèle E03), 606 LegacyTracer (traceur),
607 Distortion, 608 RadialBlur, 609 PointLight, 10000 GPUStandardParticle. Aucun de ces sorts n'utilise 10012 (DynamicTracer).

| Sort | 605 modèles | 606 traceurs (texture) | Autres | Nouveau par rapport à Lightning Spear |
|---|---|---|---|---|
| Lightning Spear | s03000, s10002 (blend Add) | 10003 et 2012 (dans 525502) | 602 aux impacts, 603, 608, 609 | — |
| Great Lightning Spear | s03000, s10002 (jusqu'à 4 émetteurs s10002) | 10003 et 2012 ; un 606 de plus dans l'incantation 525510 | impact 525513 : 608 en blend Normal, textures 11403, 11444 | rien de neuf en famille : mêmes chemins que LS |
| Sunlight Spear | comme GLS | comme GLS | comme GLS | **même structure que GLS**, seules les couleurs et les graines changent (diff de 525512/525522 : couleurs orangées) |
| Lightning Storm | s10002 (525532) | aucun | **10000 GPU particle** (525532), 603 en orientation GlobalYaw (texture 50065), impact 525533 en **blendMode 7** | particules GPU, blend 7, pas de traceur |
| Lightning Stake | s10002 (527103) | aucun | **10000 GPU particle**, **607 Distortion** (11434), 603 en **blendMode 7** et un 603 sans blend (13009) | particules GPU, distorsion, blend 7 |
| Lightning Arrow | s10002, **s11603**, **s08040**, **s08290** (sans blend : l'arc) | 2012 seulement | 602, 603, 608, deux 609 | trois modèles et deux familles de matériau nouveaux |

Matériaux des modèles (FLVER du jeu, `mtd.tsv` I-0063 pour la famille ; OBSERVÉ) :

| Modèle | Où | MTD | Famille (programme) | Blend MTD |
|---|---|---|---|---|
| s03000 | LS, GLS, SS | `S[A]_add_Soft` | **GXFlver_ColZFeather** | 4 (add) |
| s10002 | tous | `S[A]` | GXFlver_Col | 0 |
| s11603 | Arrow | `S[A]_add_Soft` | GXFlver_ColZFeather | 4 |
| s08040 | Arrow (vol, impact, 528604) | `S[ARSN]` | **GXFlver_ColDifSpcBumpIbl** (famille éclairée, une première pour une particule modèle) | 0 |
| s08290 | Arrow (528606-528608, l'arc, 12 os, matériau `WP_A_1360_m`) | `S[A]_al` | GXFlver_Col | 2 (alpha) |

s11600, vu en direct pendant les runs de Lightning Spear, n'est dans aucun de ces FXR (INFÉRÉ : un autre effet actif à l'écran).

## Pour les comparaisons suivantes (INFÉRÉ)

- **Great Lightning Spear et Sunlight Spear** passent par exactement les mêmes familles que Lightning Spear (605 s03000/s10002 en
  additif, 606 textures 10003/2012) : ce sont des contrôles directs de la tranche 7 (blend du contexte d'effet) sans nouveau chemin.
- **Lightning Storm et Lightning Stake** testent d'autres chemins : les particules GPU 10000 (dont la taille de profondeur a changé,
  pas encore comparée), le `blendMode 7` (nom inconnu du schéma, à identifier dans la table de blend du moteur), la distorsion 607
  (Stake). Leurs effets principaux partent de l'animation, pas du Bullet : leurs Bullets sont invisibles.
- **Lightning Arrow** est le seul à amener une particule modèle d'une famille éclairée (s08040, GXFlver_ColDifSpcBumpIbl) et un
  modèle à os (l'arc s08290) ; il demande le DLC2 et la Foi de l'objet.
- Le temps des TAE donne où regarder : Storm à 1,0 s et Stake à 1,07 s après le début de l'animation.

## Ce que le natif en fera (lecture du code de `main` 7a87d54f, pilote, 2026-10-30)

- **Les blends 6 et 7 du schéma** : le natif ne lit pas le blendMode des FXR ; les setters GX du moteur posent une entrée de la
  banque d'états, que `effect_stage.cpp` (`FindBlend`) couvre sans trou de `+0x08` à `+0x1D0` (base, indépendants, copies
  alpha-to-coverage, jumeaux). Une entrée inconnue serait refusée et comptée (« blend entry unknown »). INFÉRÉ : pas de chemin
  neuf ; le témoin est l'absence de ce refus pendant Storm et Stake.
- **Les particules GPU 10000** (Storm, Stake) et **la distorsion 607** (Stake) sont déjà dessinées en natif (STATE « What runs
  natively ») ; la taille de profondeur des particules GPU a changé (`3927ab10`) sans comparaison à l'original : Storm et Stake la
  donnent.
- **Les modèles E03** : le natif n'admet une soumission de `636F0` que dans la passe 0x29, sans table de matériaux propre
  (`args+0x1A8` nul) et avec des os (`12_scene_collect.inc`, compteurs `other_pass`, `own_materials`, `no_bones`). INFÉRÉ, à
  témoigner : s08040 (famille éclairée), s11603 et l'arc s08290 de Lightning Arrow peuvent tomber dans un de ces cas ; le relevé
  natif `native_effect_model_*` et le relevé de l'original `reference_ffx_model_draws` (passe, os, matériaux par modèle) le disent.
