# I-0108 : inventaire des réglages graphiques, original contre natif

29-30 octobre 2026 · 2e pilote · demande de Thomas (22:09 et 23:14, heure de Paris) : « vérifier si l'eau a des reflets », puis
« l'inventaire des réglages graphiques du jeu et voir si tout y est, ainsi que leur qualité ».

## Méthode

- **Statique.** La table de qualité du jeu (0x24C octets, décodée dans
  [MENUS-MAP §77.3](../I-0067-static-menus-subsystems-map/MENUS-MAP.md)) donne, pour chaque réglage et chaque niveau (DISABLE, LOW,
  MEDIUM, HIGH, MAX), les octets de l'objet de qualité `[GXRS+0x5360]` que lit le moteur. Le natif construit cet objet et le réécrit
  quand un niveau change (`native/frame/game_quality_object.cpp`, `game_bootstrap/10_options_and_passes.inc:13-33`). Pour chaque
  champ : qui le lit côté natif, et ce que le natif en fait. Relevé fait par un sous-agent Sonnet, puis vérifié par le pilote aux
  endroits qui fondent une conclusion (reflet `12_scene_collect.inc:774-900`, AA `:148-172`, DoF
  `13b_prepare_model_frame.inc:1258-1300`, lumières du reflet `07_scene_lighting.inc:205-213`, eau : `FUN_14024E790` lu par GhidraMCP).
- **En jeu.** Série `opt-lv-*` (29 octobre 23:33-23:51, ASUS, sans mesure, module du lanceur `a2a627d8`) : le lac de Crucifixion
  Woods (save de zone Iota m33, téléport de FABLE-REVIEW 1.7), original puis natif, à cinq niveaux appliqués à tous les réglages :
  MAX (le fichier de Thomas tel quel), HIGH, MEDIUM, LOW, OFF (SSAO, DoF, flou de mouvement et reflets coupés, le reste en LOW).
  Les niveaux appliqués sont vérifiés dans les journaux (`native_option_change`). Planches dans `evidence/I-0066/artifacts/opt-lv-compare/`
  (`lake-levels.png`, `low-off-lake.png`, `background-dof.png`). FLIP ne discrimine pas ici (0,20 à 0,28 à tous les niveaux, OFF
  compris : les crabes et le feuillage bougent) ; les conclusions viennent des planches et des journaux.
- Les niveaux que le menu du jeu propose pour chaque réglage ne sont pas lus (code du menu chiffré, MENUS-MAP §77.5) : la série
  utilise ceux de la table.

## Le tableau

Statut : **suivi** (le natif suit chaque niveau comme l'original), **partiel**, **ignoré** (le natif fait la même chose à tous les
niveaux), **non vérifié**.

| Réglage | Ce que règle chaque niveau dans l'original | Natif | Vu en jeu |
|---|---|---|---|
| Texture Quality | filtre et anisotropie des samplers (1, 1, 4, 8, 16) | **suivi** (banque de samplers, `10_options_and_passes.inc:44-52`) ; dans l'original la table ne règle que les samplers ; une baisse de résolution des textures par niveau n'est cherchée ni d'un côté ni de l'autre | les textures manquantes connues sont un autre sujet (ligne 7) |
| Antialiasing | le filtre AA du jeu à ON | **partiel** : le DLAA (type 4) suit le réglage ; les FXAA (types 1 à 3 des gparam) ne sont pas natifs | non vérifié (au lac : type 4) |
| SSAO | LOW à MAX ajoutent la passe, le temporel, la séparation, les normales | **suivi** (7 octets lus, `ao_core.cpp:34-45`) | journaux : qualité 1111000 en MAX, 1010000 en MEDIUM, 1000000 en LOW |
| Depth of Field | qualité effective = min(zone, réglage) | **dessinée à chaque qualité depuis la tranche 1** (avant : qualités 0 et 1 refusées, « programmes ») ; rendu non comparé octet par octet à l'original (INFÉRÉ conforme) | manquait au lac (qualité de zone 1 : `background-dof.png`, le même refus dans 48 runs natifs m33) ; dessinée depuis la tranche 1 (`opt-dof-nat-*` : 10 passes à MAX, 5 en LOW) |
| Motion Blur | passe, vitesse par objet (MEDIUM+), plafond d'échantillons 1/1/5/8 | **suivi** | journaux : passes 2 en MAX, 1 en LOW, objets sans vitesse en LOW |
| Shadow Quality | ombres, taille de l'atlas 1024 à 8192, flou | **suivi** (`shadow_state.cpp:13-26`) | pas de différence vue au lac |
| Lighting Quality | fondu des lumières (0,5 à 1,2), ombres locales (MEDIUM+), un 3e octet `+0x45` | **suivi** sauf `+0x45` **ignoré** (rôle inconnu, `FUN_14010A220`) | non discriminant au lac |
| Effects Quality | drapeaux et échelles des effets | **suivi** sauf `+0x4B` (MAX seul) et `+0x64` **non vérifiés** | non vérifié (pas d'effet au lac) |
| Reflection Quality | DISABLE : pas de reflet ; LOW : reflet en quart de taille (INFÉRÉ), sans lumières ; MEDIUM, HIGH : demi-taille, avec lumières ; MAX : en plus toutes les pièces ordinaires du décor | **partiel** : depuis la tranche 2 (étape 1), le marche-arrêt, les lumières (coupées en LOW) et les pièces de MAX sont suivis ; la taille reste demi à tous les niveaux (le quart en LOW est INFÉRÉ, lecteur non trouvé) | **différent** : en LOW l'original montre un reflet sombre, le natif un reflet éclairé ; à tous les niveaux le reflet natif est plus clair (troncs éclairés en traînées vives : l'ombre du soleil manque dans la vue reflétée, D-0189) et le crabe n'y a pas de reflet (les personnages ne semblent pas reflétés, INFÉRÉ sur un crabe) ; DISABLE identique |
| Water Surface Quality | `+0x74` autorise une famille d'effets (clé 0x23, ceux qui lisent la copie de la scène) dans une passe que le natif ne lance pas (`FUN_14024E790`, INFÉRÉ) | **ignoré** | non vérifié (rien de visible au lac) ; c'est le domaine du pilote effets |
| Shade Quality | SSS de la peau (MEDIUM+), `+0x76` (HIGH+), `+0x77` (MAX) | **suivi** pour le SSS ; `+0x76`, `+0x77` **ignorés** (lecteurs incertains) | non discriminant au lac |

## Ce qui manque ou diffère, par poids

1. **La profondeur de champ manquait à Crucifixion Woods** (qualité de zone 1), quel que soit le réglage, et partout en réglage LOW
   (qualité 0) : le sous-échantillonnage à 6 prises (`tech_MadT1_S6`) n'était pas épinglé. **Corrigé** (tranche 1, RESULT).
2. **Le reflet de l'eau** : il existe et ressemble à l'original de MEDIUM à MAX ; depuis la tranche 2 (étape 1) il suit les
   lumières du réglage (aucune en LOW) et les pièces de MAX, et il est plus proche de l'original aux deux niveaux témoins. Restent :
   (a) aucun personnage dans le reflet (INFÉRÉ sur un crabe ; le joueur à vérifier), (b) la taille en LOW (quart de taille INFÉRÉ),
   (c) un reste de clarté, dont l'ombre du soleil manquante dans la vue reflétée (D-0189) n'est plus sûrement la cause, et (d) le coût
   en MAX (2,3 ms de collecte au lieu de 0,9 ms, lu dans les journaux).
3. **Water Surface Quality** n'a aucun effet en natif ; la famille d'effets qu'il autorise n'est pas dessinée (pilote effets).
4. **Antialiasing FXAA** (types 1 à 3) absent, seulement si une zone l'utilise (non vérifié).
5. Petits octets sans lecteur natif (`+0x45`, `+0x4B`, `+0x64`, `+0x76`, `+0x77`) : rôle à lire avant de dire qu'il manque quelque chose.

## Ce qui n'est pas vérifié

- Une seule zone (le lac m33) ; les autres zones témoins à d'autres niveaux ne sont pas faites.
- Les niveaux exacts que propose le menu de chaque réglage ; les préréglages (LOW à MAX) du menu.
- Les effets selon Effects Quality et Water Surface Quality (rien à l'écran au lac).
- Les personnages absents du reflet : vu sur un crabe, pas sur le joueur (l'angle ne le montre pas).
