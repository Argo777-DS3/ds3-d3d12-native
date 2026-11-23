# Étude : regarder des vidéos (fil S, 2026-11-07)

Demande de Thomas (2026-11-06 23:54, project chat) : « ce serait bien de vous donner la possibilité de regarder des vidéos ».
L'étude elle-même n'a pas écrit de code ; l'outil qui en est sorti est au §5. Les prototypes et les images des essais sont hors Git, dans
`C:/Users/User/Desktop/DS3_evidences/_studies/video-2026-11-07/` (`prototypes/`, Python 3.12 de la bibliothèque standard
seulement, ffmpeg 9.0.2 fait le travail sur les pixels). Gemini pour les vidéos : idée déjà écartée par Thomas, non relancée.
Rien n'est envoyé hors du PC.

Statuts : **OBSERVÉ** = mesuré sur les films cités ; **INFÉRÉ** = interprétation non vérifiée.

## 1. Ce que Claude perçoit réellement

- Claude ne reçoit pas de flux vidéo : seulement des **images fixes**, chacune réduite à environ 1,15 mégapixel (côté long
  ≈ 1 568 px) et coûtant ≈ largeur × hauteur / 750 jetons (une image 1296x758 ≈ 1 300 jetons). Un film de 40 s à 60 i/s
  (2 400 images) est hors de portée image par image ; quelques dizaines d'images par tour au plus.
- Entre deux images, Claude **ne voit pas le mouvement** : il compare des états. Une saccade, un scintillement, une animation
  relancée sont invisibles sur une planche d'images, même dense, sauf si le temps est converti en espace (section 3).
- Les jugements fins sur une image (plus sombre, plus saturé) ne sont pas fiables à l'œil (mémoire « Measure pictures, not by
  eye », 2026-11-04) : un chiffre d'abord.
- Une planche réduit chaque image : 20 images dans 1 616 px de large = 320 px chacune, les détails disparaissent.
  `frames/blink5305.jpg` d'I-0115 (une planche mal découpée d'une salle sombre) n'est lisible par personne.

Le défaut du 6 novembre n'était pas de perception : sur l'écart 22, la première mesure (la moyenne du changement image à
image dans l'arche) était semblable dans les deux rendus ; c'est la **forme dans le temps** (petits changements continus contre
gel puis sauts) qui différait, et une moyenne l'efface. L'œil de Thomas voit cette forme directement.

## 2. Ce que les fils font aujourd'hui

`Run-I113Barrier.ps1`, `Run-AlphaCast.ps1`, `Run-ZoneWalk.ps1`, `Run-I115Steered.ps1` filment la fenêtre avec ffmpeg
`gdigrab` à 60 i/s (x264 ultrafast, crf 16) ; les fils en tirent des planches (`fps=N,tile=CxR`), des paires côte à côte, et
des moyennes (signalstats YAVG, différences d'images). Limites connues : `gdigrab` n'est pas synchronisé sur les présentations du
jeu (doublons et sauts de capture possibles) ; x264 pose une image clé toutes les 250 images, où une zone immobile change un peu (un faux saut périodique) ; les zones sombres sont
écrasées à 8 bits.

## 3. Les moyens, essayés sur des films existants

### 3.1 Courbe de mouvement par région (prototype `vmotion.py`)

ffmpeg calcule `|image n − image n−1|` (`tblend=all_mode=difference`) en 16 bits puis la moyenne par cellule d'une grille
(`scale=GWxGH:flags=area`) : une valeur par cellule et par image, en ~2 s pour 40 s de film. Sorties : CSV, quantiles, images
« pics » (au-delà de k écarts robustes), images immobiles, raster cellules × temps.

Règle indispensable : **toujours des régions témoins**. Sur la paire `i113-t21-bridge2` (OBSERVÉ, images 15 à 530, caméra
fixe) :

| Région | Natif | Original |
|---|---|---|
| Lointain (900:100, 300x300) | pics à 58, 119, 240, 362, 424 (+15 dans le film : 73, 134, 255, 377, 439), 7 à 10 fois la médiane | aucun pic périodique |
| Personnage (570:330, 150x400) | aucun pic à ces images | aucun |
| Arbres à mi-distance (380:150) | aucun pic à ces images | aucun |

Le personnage et la mi-distance ne sautent pas : ce n'est ni un saut de capture ni un hitch de présentation (ceux-là touchent
tout ce qui bouge). Sur tout le film : natif 11, 73, 134, 255, 377, 439, puis 2329, 2390 dans la même vue, intervalle 61 à 62
images (~1,02 s), quelques périodes sous le seuil. Ces numéros-là comptent le temps du film (ffmpeg rééchantillonné) ; l'outil
du §5 compte les images décodées, une de moins après le trou de temps du début de ce film (10, 72, 133, …).

### 3.2 Carte de l'empreinte d'un saut (prototype `jumpmap.py`)

Pour chaque cellule de 8x8 px : mouvement moyen aux images de saut divisé par sa médiane, posé en rouge sur une image du film
(`ov.png`). OBSERVÉ : dans le natif, tout le lointain au-delà de la rambarde (le pont, la porte de la barrière, les montagnes à
droite du pilier) saute ; 2 815 cellules sur 15 390 au-dessus de 4, contre 89 dans l'original (des flocons). L'écart 22 n'est
donc pas la barrière seule (INFÉRÉ : un update ~1 Hz du lointain, cause non lue). Transmis à E le 2026-11-07 vers 01:05.

### 3.3 Fente temporelle (slit-scan)

Une colonne de pixels (x = 1000, y 150 à 350) prise à chaque image et empilée de gauche à droite (`format=rgb24,crop=1:200:1000:150,tile=509x1`) :
le temps devient l'axe horizontal, une seule image montre la forme du mouvement (`slit.png`). OBSERVÉ : dans le natif, la brume
du lointain dérive en continu pendant ~1 s puis repart d'une autre position (couture verticale nette), à chaque période ; dans
l'original, des ondulations continues, sans couture, et beaucoup plus de flocons. C'est la forme « animation relancée »
(l'hypothèse de Thomas), pas « figée » (INFÉRÉ). De tous les moyens essayés, c'est celui qui fait **voir** une saccade à Claude.

### 3.4 Images de différence

`tblend=all_mode=difference` amplifié ×6 (`lutrgb`), quelques images en planche (`diff-native-55-60.png`, `dno.png`) : montre
d'un coup ce qui a changé entre deux images ; c'est ce qui a mis en évidence la zone du lointain avant la carte.

### 3.5 Scintillement, disparition brève (prototype `transient.py`) : échec sur le cas d'I-0115

Détecteur : un saut de luminance d'une cellule qui revient en moins de W images. Sur `i115-hall-n60` (le clignotement des flesh
masses vu par Thomas, opacité 0,083 puis retour en 11 images) : **rien de distinguable** (OBSERVÉ) : la caméra avance, la salle est
sombre, les sauts dus au mouvement noient le signal dans les deux rendus. Comme I-0115 l'avait noté pour ses films à 10 i/s, il
faut les lignes du journal (`Get-Blinks.py`) ou une caméra fixe. Limite honnête : ces outils marchent caméra fixe (ou
mouvement identique et rejoué) ; avec une caméra qui bouge, il faut stabiliser (`vidstabdetect`, présent dans le build) ou
comparer à l'original sur le même chemin rejoué, non essayé.

### 3.6 Filtres ffmpeg déjà présents (non essayés ici)

`freezedetect` (gel d'une zone recadrée), `scdet` (coupes de plan : pour découper un film en vues, et caler une paire dans le
temps sur les rotations de caméra), `mpdecimate` (doublons de capture), `photosensitivity` (flashs), `siti`, `vmafmotion`,
`drawgraph` (trace une métadonnée en courbe dans une image, sans police), `ddagrab` (capture par Desktop Duplication).

## 4. Limites qui restent

- Le film montre le **temps du contenu**, pas celui de l'affichage : la cadence et les images en retard restent à PresentMon et
  `cadence-trace.flag`. `gdigrab` à 60 i/s fixe peut doubler ou sauter une image ; `ddagrab` (GPU, Desktop Duplication) suit
  mieux les mises à jour réelles, à essayer à sec.
- Filmer coûte du CPU (encodage) : jamais pendant un run qui mesure.
- Compression : non mesurée en général. Mesuré : les zones immobiles des films du pont changent de 0,01 à 0,10 niveau d'une
  image à l'autre (la revue finale) ; un scintillement synthétique de 10 niveaux se lit 9,65 en x264 crf 16 contre 10,0 sans perte ;
  les images clés (250 images en x264, 30 dans les clips NVIDIA) font de petits sauts, que l'outil met à part. Pour un
  scintillement faible, un court film sans perte (`-c:v ffv1`, plusieurs centaines de Mo par minute).
- Paires natif/original : caméras décalées de quelques pixels (≈ 7 px au pont), régions à placer par rendu ; temps à caler.
- Zones sombres : 8 bits écrasés ; relever le gamma avant l'analyse aide l'œil, pas la mesure.
- Un seuil n'est valable qu'après calibrage sur l'original (son bruit propre : neige, vent, brume).

## 5. Proposition, et ce qui en est fait

Thomas a dit oui (2026-11-07 01:14, Paris). Fait : l'outil [`scripts/video/videosheet.py`](../scripts/video/videosheet.py) (point 1,
sans la détection de coupes par `scdet` : les vues sont les intervalles entre les rotations de caméra qu'il mesure), la règle du
point 2 dans WORKFLOW « Tools », la façon de filmer dans LIVE-RUN-NOTES. Points 3 et 4 : pas faits.

Sans région donnée (OBSERVÉ, `i113-t21-bridge2-native`, film entier, version revue de l'outil) il retrouve seul les sauts du
lointain : 10, 72, 133, 254, 376, 438, puis 1230, 1352, 1474, 1596 (la vue du pilier, une troisième vue), 2328, 2389, chacun sur
150 à 214 blocs de 32 px sur 920 ; période 61 images (1,02 s), 12 groupes sur 12 (des trains au hasard d'autant d'événements
n'en font autant dans aucun des 73 tirages, seuil 5 %) ;
l'original : aucun groupe de 18 blocs ou plus. Sur un clip NVIDIA de Thomas (2026-11-02 08:54, caméra en mouvement) :
31 images répétées, dont un arrêt de 6 images (7 à 12) puis, caméra en mouvement, une répétition toutes les 6 à 10 images
(INFÉRÉ : moins de 60 images neuves par seconde, du jeu ou de l'enregistreur).

La revue finale (fraîche, xhigh, sur des films réels et synthétiques) puis deux revues de ses corrections ont trouvé 4 High et 9
Medium, corrigés avant la fusion. Un scintillement présent la plupart du temps relevait son propre seuil : les clignotements se
comptent maintenant contre la partie calme de la série, un par aller-retour (un scintillement compte ses cycles), et un clignotement
long, qui revient en 15 images, compte aussi. Un saut fort d'une zone était pris pour une rotation de caméra : une rotation demande
maintenant 5 images où plus de la moitié des cellules bougent au-delà de 4 fois leur mouvement habituel ; plus court, c'est un
« hitch » signalé, pas retiré (reste : un changement de lumière ou un fondu de brume sur plus de la moitié de l'image pendant
5 images compte aussi comme une rotation). Les images clés de l'encodeur ne mettent à part que les petits sauts (3 niveaux au plus),
et seulement là où les cellules immobiles de l'image changent elles aussi (x264, NVIDIA ; pas un codec sans perte). La période se
cherche sur un réseau au quart d'image près, notée par la probabilité binomiale de tenir autant d'événements par hasard (et si tous les événements du réseau
sont espacés d'un multiple de sa période, la période est ce multiple : un train toutes les 120 images n'est plus lu à 60), une rafale comptant pour un événement ; elle n'est donnée que si moins de 5 % de
trains au hasard d'autant d'événements font aussi bien (1 % pour les lignes des blocs, plus nombreuses), en jusqu'à 500 tirages.
Un bloc qui a aussi des sauts à mi-période peut afficher la demi-période (un bloc du pont : 30,5). Les images sont numérotées dans l'ordre du décodage, sans rééchantillonnage ; un
film abîmé ne bloque plus l'outil (vérifié sur un film aux octets altérés : 25 s, les erreurs de décodage signalées). Vérifié sur un
film synthétique, en ffv1 et en x264 : un scintillement à chaque image, 29,9 cycles/s ; trois clignotements de 12 images ; des
sauts toutes les 30 images (période 30, 18 sur 18) ; un gel de 59 images ; une image répétée (450).

Les plugins du catalogue (2026-11-07) : `watch-video` et `video-to-code-skill`, de la communauté, extraient des images clés et
transcrivent le son (Whisper, local ou en ligne) : toujours des images fixes, aucune mesure du mouvement. Thomas (01:23) : « On va
garder notre outil » ; pas à reproposer.

La proposition d'origine :

1. **Un outil du dépôt** (le pilote E, ou une petite tranche d'outillage) : `scripts/Get-VideoSheet.ps1` (ou les prototypes en
   Python, déjà sans dépendance) qui, pour un film ou une paire et une liste de régions (plus une grille automatique), produit
   **une feuille vidéo** : les vues du film (coupes par `scdet`, une image chacune), les courbes par région avec les régions
   témoins, la liste des événements (pics, période par autocorrélation, part d'images immobiles, alternances de luminance), la
   carte d'empreinte et une ou deux fentes temporelles placées sur la région. Deux ou trois images que Claude lit en un tour, et
   des chiffres. Coût estimé : une demi-journée de pilote, tests compris.
2. **Une règle dans WORKFLOW « Tools »** : un défaut de mouvement (saccade, scintillement, relance) se juge par la forme dans le
   temps (courbe par cellule, fente temporelle), jamais par une moyenne sur une région ; toujours une région témoin et l'original.
3. **Capture** : essayer `ddagrab` à sec et un mode court sans perte dans les runners (option), quand un fil en a besoin.
4. Option, plus tard et seulement si Thomas veut : un petit numéro d'image codé dans un coin du rendu de test (flag de test,
   hors production) pour relier chaque image du film à l'image du jeu et repérer doublons et sauts de capture.

L'œil de Thomas reste le meilleur détecteur : ces outils ne le remplacent pas, ils permettent à un fil de confirmer, localiser et
chiffrer ce que Thomas voit, et parfois de le voir avant lui caméra fixe.
