# Plan : la chaîne de post-traitement moderne à la place de YEBIS (ROADMAP chantier 11)

12 novembre 2026, pilote du chantier 11 (Fable 5.2, effort high). Plan soumis à Thomas ; rien n'est lancé tant qu'il n'a pas
répondu. L'étude qui le fonde est [STUDY](STUDY.md) (en anglais, avec ses sources) ; ses annexes : [CENSUS-WORLD](CENSUS-WORLD.md),
[CENSUS-CUTSCENES](CENSUS-CUTSCENES.md), [ER-POST-CHAIN](ER-POST-CHAIN.md), [ER-TONEMAP-HDR](ER-TONEMAP-HDR.md). Les tailles
sont des estimations, pas des mesures. « Connaissance générale » marque ce qui ne vient pas du dépôt.

## 1. En bref

- **YEBIS sert peu dans le monde et beaucoup dans les cinématiques.** Dans les 18 zones : une seule courbe filmique, une
  exposition fixe par zone, une saturation, un vignettage, une LUT par zone, un bloom, des light shafts dans 12 zones, et la
  même profondeur de champ et le même flou de mouvement partout. Les cinématiques ajoutent une vraie profondeur de champ
  d'objectif, des fondus par la couleur, un vignettage lié au champ de vision et quelques effets de lentille.
- **Elden Ring n'a plus YEBIS.** FromSoftware l'a remplacé par ses propres filtres, dans un ordre fixe, avec le même modèle de
  paramètres que DS3 en plus riche. DS3 contient déjà les ancêtres de ces filtres. La nouvelle chaîne suit donc cette
  architecture (D-0195), avec les valeurs de DS3.
- **Notre rendu natif ne fait pas tourner YEBIS, mais dessine encore ses shaders** (lus dans l'exécutable du jeu) pour six
  passes, et refuse les configurations jamais vues dans les zones témoins : d'après les données, presque toutes les
  profondeurs de champ des cinématiques. La nouvelle chaîne est paramétrique, elle n'a plus de refus.
- **Le plan : huit tranches**, chacune jugée par toi sur planches avant/après. D'abord la colorimétrie d'origine refaite à
  l'identique dans notre chaîne (rien ne change à l'écran), puis le bloom, l'exposition, la profondeur de champ et le flou
  de mouvement, le HDR10, la nouvelle colorimétrie, les réglages, et enfin le retrait de YEBIS.
- **Quatre décisions te reviennent** (section 7), chacune avec ma recommandation.

## 2. Ce que YEBIS fait dans DS3

### Dans le monde (les 100 fichiers de zones du jeu)

| Fonction | Ce que les données demandent |
|---|---|
| Tone map | La courbe « sensitométrique » de YEBIS partout, gamma 2,2 partout |
| Exposition | Fixe par zone : 1,52 (Irithyll) à 2,4 (Archdragon Peak) ; l'exposition automatique de YEBIS n'est jamais active dans une zone atteignable |
| Couleur | Saturation 0,8 à 0,9 ; pas de réglage de valeur ni de contraste |
| Vignettage | 0 à 0,4 selon la zone, sans lien avec le champ de vision |
| LUT d'étalonnage | Une par zone (16x16x16) ; six zones n'en ont pas (dont Ariandel et la Ringed City) |
| Bloom | Seuil 0,2 à 0,5, intensité 1,8 à 8, 6 ou 8 niveaux |
| Light shafts | Dans 12 zones sur 18 (calculés à l'écran, pas dans le volume) |
| Profondeur de champ | La même partout : un léger flou des lointains |
| Flou de mouvement | Le même partout, selon l'option du jeu |
| Étoiles, reflets fantômes, halo, rémanence, flare anamorphique, flou de feedback, autofocus | Jamais demandés |
| Aberration chromatique | **Jamais dessinée** : la valeur existe dans quatre zones mais son interrupteur est à 0 partout, et une capture de l'original au High Wall n'en montre aucune |

Le tableau zone par zone est dans STUDY §2.1.

### Dans les cinématiques (49 fichiers, 146 réglages de tone map, 232 de profondeur de champ)

| Fonction | Ce que les cinématiques demandent |
|---|---|
| Profondeur de champ | Active dans 208 réglages sur 232, en valeurs d'objectif : ouverture F 0,01 à 80, distance de mise au point 0,1 à 200, trois tailles de capteur |
| Couleur animée | Valeur de 0 (fondu au noir) à 4 (flash), contraste, gamma de 0 à 4,62, exposition de 0 à 5,09 |
| Vignettage | Jusqu'à 1,35, et lié au champ de vision dans 58 réglages |
| Effets de lentille | Un glare « personnalisé » dans 13 réglages (dont une étoile), l'aberration chromatique dans 4 réglages de 3 cinématiques |
| Seconde courbe (Reinhard) | 2 réglages d'une cinématique |

Chaque plan d'une cinématique passe d'un réglage à un autre par un fondu : la chaîne doit interpoler tous ses paramètres à
chaque image.

## 3. Ce qu'Elden Ring fait à la place

Lu dans son exécutable (statique, deux lectures Sonnet ; ses valeurs par zone et ses shaders sont dans ses fichiers de
données, non lus).

| Ordre | Étape d'ER | Chez nous aujourd'hui |
|---|---|---|
| 1 | Profondeur de champ (objectif simulé, demi-résolution) | Les passes de YEBIS |
| 2 | TempAA | Notre TAA ou DLSS (déjà à cette place) |
| 3 | Flou de mouvement (caméra et objets) | Les passes de YEBIS |
| 4 | Glow émissif, **avant** le tone map | Le glow du jeu, **après** le tone map, sur 8 bits |
| 5 | Un seul filtre de tone map : exposition, bloom, light shafts, courbe lue dans une table, matrice de couleur, LUT 3D avec fondu entre deux LUT, vignettage, aberration chromatique | Le glare, les light shafts, le tone map et l'étalonnage de YEBIS, plus notre exposition automatique |
| 6 | Encodage de sortie, puis AA spatial et sharpening | La passe gamma du jeu |
| 7 | Interface dessinée dans une texture, mélangée à la fin | Interface dessinée directement sur l'image 8 bits |

**Ce qu'ER règle vraiment** (lu après coup dans ses fichiers de données, avec ton accord du 12 novembre ;
[ER-VALUES](ER-VALUES.md)) : sa courbe est un Reinhard étendu, pas la sensitométrique de DS3 ; son exposition automatique
est active presque partout en jeu mais bornée (elle peut assombrir de 1,5 EV et éclaircir de 0,4 à 0,5 EV, lentement) ;
sa matrice de couleur est neutre, son look tient dans sa LUT (qui change avec l'heure dans le monde ouvert) et dans sa
courbe ; son bloom s'ajoute avant la courbe ; son vignettage est actif partout (environ 0,5) ; une légère aberration
chromatique est active presque partout en jeu ; son HDR est son image SDR dont les hautes lumières sont étendues après
coup. (La première version de ce plan disait à tort que sa courbe par défaut était la sensitométrique de DS3 : c'est le
défaut du code, pas ce que le jeu utilise.)

Les défauts d'ER que nous ne copions pas (lus dans sa décompilation) : son exposition automatique relue par le CPU avec
environ quatre images de retard (la nôtre reste entièrement sur le GPU) ; son HDR passe par les API des fabricants et le
plein écran exclusif (le nôtre passe par DXGI, donc aussi en fenêtré et sans bordures) ; sa table de courbe et sa LUT
refaites à chaque image (notre courbe est calculée dans le shader, notre LUT refaite seulement quand elle change) ; pas
de dither de sortie trouvé (le nôtre en blue-noise). Je n'ai pas pu établir si le HDR d'ER s'active vraiment sur PC.

## 4. La chaîne proposée

Dans l'ordre d'une image, tout en HDR linéaire jusqu'à l'encodage de sortie :

| # | Passe | Ce qu'elle remplace | D'où viennent ses valeurs |
|---|---|---|---|
| 1 | DLSS avec Ray Reconstruction, ou notre TAA | Inchangé | |
| 2 | **Profondeur de champ** : objectif simulé (ouverture, mise au point, capteur) ou plages proches et lointaines, à demi-résolution | Les 10 dessins de YEBIS | Le groupe Dof de DS3, tel quel |
| 3 | **Flou de mouvement** : vitesses caméra et objets, reconstruction | Les dessins de YEBIS | Le groupe MotionBlur et l'option du jeu |
| 4 | **Glow émissif** en HDR, avant le tone map (à partir de la tranche HDR) | Le glow sur 8 bits | Le groupe Glow Filter |
| 5 | **Exposition** : celle de la zone, fois notre facteur automatique à demi-force (D-0263, D-0264) | Notre passe actuelle, rebranchée | Le groupe Tone Map |
| 6 | **Bloom** : seuil, pyramide, recomposition ; les **light shafts** s'y ajoutent | Les 20 à 35 dessins du glare de YEBIS | Les groupes Bloom et LightShaft |
| 7 | **Tone map en une passe** : courbe calculée dans le shader, matrice de couleur, vignettage, LUT 3D | Le tone map et l'étalonnage de YEBIS | Les groupes Tone Map et ColorGrading |
| 8 | **Encodage de sortie** : gamma et luminosité du jeu en SDR, ou PQ en HDR10 ; dither blue-noise | La passe gamma du jeu | Les réglages |
| 9 | Sharpening, interface, présentation | Inchangé en SDR | |

**Deux colorimétries** (D-0178, D-0273), au choix dans le menu :

- **Originale** : la courbe, la matrice et les LUT de DS3, avec ses valeurs zone par zone. En SDR, l'image doit être celle
  d'aujourd'hui (voir tranche 1). En HDR10, mêmes tons moyens et mêmes couleurs, avec des hautes lumières qui montent au
  lieu d'écrêter.
- **Rekindled** : une nouvelle, dans l'esprit de Dark Souls Remastered et d'Elden Ring. Je ne la fige pas ici : elle se
  choisit sur planches à la tranche 6, entre deux ou trois candidates.

## 5. Les tranches

Chacune se fusionne seule, avec ses revues, et te montre des planches avant/après (D-0268). L'implémentation commence après
le chantier 2, comme le prévoit la ROADMAP.

| # | Tranche | Ce qu'elle livre | Ce que tu vois | Taille |
|---|---|---|---|---|
| 1 | **Le socle et la colorimétrie d'origine** | Notre passe de tone map (courbe, matrice, vignettage, LUT) à la place des shaders de YEBIS ; l'ancienne chaîne gardée derrière un drapeau pour comparer | **Rien ne change**, au bit près : la passe est comparée aux dessins capturés de l'original | M |
| 2 | **Le bloom et les light shafts** | Notre pyramide de bloom et nos light shafts à la place du glare de YEBIS | Planches aux zones témoins, dont Irithyll et Undead Settlement (les deux extrêmes du bloom) | M |
| 3 | **L'exposition** | L'exposition automatique rebranchée sur la nouvelle courbe, l'exposition donnée à DLSS | Rien de nouveau ; les mêmes planches que R11h pour vérifier | S |
| 4 | **La profondeur de champ et le flou de mouvement** | Les deux passes neuves, avec les valeurs d'objectif des cinématiques | Deux ou trois cinématiques filmées à côté de l'original (l'intro de Vordt, celle de Friede) | L |
| 5 | **Le HDR10** | La sortie en flottant avec dither blue-noise, puis 10 bits PQ, les deux réglages (luminance maximale, blanc papier), l'interface en HDR, le glow émissif avant le tone map | Sur ton OLED, avec toi | L |
| 6 | **La colorimétrie Rekindled** | Deux ou trois candidates sur planches, SDR et HDR | Tu choisis | M |
| 7 | **Les réglages** | Les lignes proposées en R12 et pas encore choisies : bloom, light shafts, grain de film, champ de vision, sortie HDR, le choix de colorimétrie, et l'aberration chromatique de la colorimétrie Rekindled | Les nouvelles lignes du menu | M |
| 8 | **Le retrait de YEBIS** | Les shaders de YEBIS, leurs blocs de constantes, leurs refus et leurs tests retirés | Rien | S |

Par rapport à l'ordre de la ROADMAP : le HDR10 passe avant la nouvelle colorimétrie, pour que tu la juges dans les deux
sorties, et le dither blue-noise arrive avec lui (il change les octets de l'image, la tranche 1 n'en change aucun).

Notes par tranche :

- **Tranche 1.** Sa fiche de conception est écrite : [TRANCHE-1-READY](TRANCHE-1-READY.md). L'oracle est l'original
  lui-même : les tests unitaires tiennent déjà ses dessins de tone map et d'étalonnage capturés aux zones témoins, et la
  nouvelle passe doit rendre les mêmes octets sur les mêmes entrées. Il n'y a donc pas de seuil de tolérance. Pour cela
  la tranche garde le dither 4x4 de l'original et son passage par 8 bits avant la LUT. Réalisé le 13 novembre en deux
  dessins comme l'original, et non en une passe unique : l'écriture 8 bits de la carte n'est pas l'arrondi du manuel, seule
  une vraie image 8 bits la reproduit ([RESULT](RESULT.md) « Tranche 1 »). Le dessin unique revient avec la sortie HDR.
  La courbe est calculée dans le shader et non lue dans une table comme chez ER : c'est exact et moins cher. La seconde
  courbe (Reinhard, deux réglages d'une cinématique) vient à la tranche 4.
- **Tranche 2.** YEBIS ajoute son glare **après** la courbe, par un mélange « écran » ; pour la colorimétrie d'origine je
  garde cet ordre, sinon les tons moyens bougent. Le glare « personnalisé » des 13 réglages de cinématiques et la forme 3
  d'une variante d'Archdragon Peak demandent d'abord une capture de l'original pour savoir ce qu'ils dessinent.
- **Tranche 4.** Aujourd'hui le natif refuse, d'après les données, presque toutes les profondeurs de champ des
  cinématiques : c'est sans doute là que la différence avec l'original est la plus visible. À confirmer par un RUN. La
  sélection exacte des réglages par plan est dans les fichiers de cinématiques, pas encore extraits sur ce PC.
- **Tranche 5.** Demande l'OLED allumé, toi présent, et le HDR de Windows activé par toi. Première question de conception :
  l'interface dans une texture mélangée à la fin, comme ER (ce qui donnerait aussi sa couche d'interface à la génération
  d'images), ou l'interface dessinée directement dans un format plus large. À étudier en tête de tranche.
- **Tranche 6.** La candidate « à la Elden Ring » peut reprendre sa courbe (Reinhard étendu, contraste 1,3 à 1,55), sa
  matrice neutre et son bloom ajouté avant la courbe. Elle ne peut pas reprendre ses LUT (ce sont des fichiers d'ER, on
  ne les distribue pas). Elle garde sa légère aberration chromatique (Thomas, 12 novembre 12:34 : « S'il a une légère
  aberration chromatique, on garde »), avec une ligne de réglage pour la couper à la tranche 7 ; la colorimétrie
  d'origine n'en a toujours aucune en jeu. Son exposition automatique bornée est une variante à te montrer à la
  tranche 3, à côté de notre demi-force. Candidates de courbe à ajouter aux planches (Thomas, 13 novembre 18:55,
  dans le chat du projet, relayé par le coordinateur : « AoG ? AX? Ça ressemblait à ça les noms », puis « Oui, on en
  avait parlé, on peut les utiliser ») : **AgX et ACES**, à côté de la candidate à la Elden Ring ; l'ordre des tranches
  ne change pas. L'exposition à la Elden Ring est à rejuger là, avec la courbe (proposition du pilote de la tranche 4,
  13 novembre, dans son fil).
- **Tranche 7.** Le grain de film n'existe ni dans DS3 ni dans ER : ce serait un effet nouveau, éteint par défaut. Le champ
  de vision n'est pas du post-traitement : il demande sa propre petite étude de la caméra.

## 6. Ce qui reste inconnu

Aucun de ces points ne bloque le plan ; chacun est levé dans la tranche qui en a besoin.

- Ce que dessinent la forme de glare 3 et le glare personnalisé dans l'original (une capture).
- Quels réglages chaque plan de cinématique choisit et avec quel fondu (les fichiers de cinématiques à extraire).
- Les images des LUT d'ER et le passage exact de ses paramètres aux constantes de ses shaders (ses valeurs et ses
  formules, elles, sont lues : [ER-VALUES](ER-VALUES.md)).
- Si une cinématique joue vraiment sans sa profondeur de champ en natif aujourd'hui (déduit du code et des données, jamais
  vu : aucune cinématique n'a encore été jugée en natif).
- Le coût de la nouvelle chaîne. Elle fait moins de passes que la chaîne YEBIS (environ 40 dessins par image
  aujourd'hui), je m'attends à un coût égal ou moindre, sans l'avoir mesuré.

## 7. Les décisions qui te reviennent

1. **Les light shafts.** Ils sont calculés à l'écran, et le chantier 12 apportera un brouillard volumétrique tracé qui en
   fera de vrais. Recommandation : les refaire dans la nouvelle chaîne maintenant (12 zones en dépendent) et les remplacer
   au chantier 12. L'autre choix : les retirer dès maintenant et attendre le chantier 12.
2. **L'aberration chromatique.** DS3 n'en dessine jamais dans le monde, seulement dans trois cinématiques. Recommandation :
   pas de ligne de réglage (elle n'agirait sur rien en jeu), et on la reproduit dans ces trois cinématiques. Cela ferme la
   vérification laissée ouverte par D-0266 et la ROADMAP.
3. **La colorimétrie Rekindled.** Veux-tu qu'une des candidates reprenne les vraies valeurs d'Elden Ring ? Il faudrait lire
   ses fichiers de données en lecture seule (comme pour le logo, I-0135). Recommandation : oui, c'est la référence que tu
   as citée avec Dark Souls Remastered.
4. **Le glow émissif avant le tone map** (comme ER) à la tranche HDR. Il change un peu l'aspect des surfaces lumineuses ;
   tu juges sur planches. Recommandation : oui, c'est nécessaire pour le HDR.

Et une question pratique pour la tranche 5 : ton OLED est-il bien HDR, et acceptes-tu d'activer le HDR de Windows pour
ces essais ?

Réponse de Thomas (12 novembre, 11:55) : « Oui je peux activer HDR ». La tranche 5 se fait donc sur l'OLED, le HDR de
Windows activé par lui le moment venu. Réponses de Thomas (12 novembre, 12:19) : « Je suis ta recommandation pour les points 2 à 4 » : pas de ligne de réglage
pour l'aberration chromatique, reproduite dans les trois cinématiques (2) ; les valeurs d'Elden Ring lues dans ses fichiers
de données, en lecture seule, pour une candidate de la colorimétrie Rekindled (3) ; le glow émissif avant le tone map à la
tranche HDR (4). Pour le point 1 : « j'aimerais éviter le travail inutile, tu proposes quoi ? ». Le pilote a d'abord proposé (12:21) de
garder la passe actuelle raccordée au nouveau bloom jusqu'au chantier 12 ; Thomas (12:21) : « Je suis pas fan des
adaptateurs franchement ». Proposition du pilote (12:23), acceptée par Thomas (12:34, « Sinon je suis ok, ça me va ») : les light
shafts réécrits une fois dans nos shaders, à l'intérieur du nouveau bloom (tranche 2), avec les formules de DS3 déjà
transcrites et testées (`native/world/yebis_shafts`). Elden Ring garde lui aussi des light shafts calculés à l'écran dans
son filtre de tone map à côté de son brouillard volumétrique ([ER-POST-CHAIN](ER-POST-CHAIN.md) §4, `LightShaft_*`) : au
chantier 12, on juge sur planches si le brouillard tracé les remplace ou si les deux restent. YEBIS est ainsi retiré en
entier à la fin du chantier 11.
