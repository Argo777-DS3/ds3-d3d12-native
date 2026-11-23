# Chantier 12 : synthèse de l'étude préparatoire, pour Thomas

Fil S, 22 novembre 2026. L'étude complète est [STUDY](STUDY.md) (en anglais, avec ses six annexes) ; cette page en donne
l'essentiel et les quatre choix qui te reviennent. Tout a été fait en lecture seule : aucun build, aucun lancement du jeu.
Rien ici n'était décidé à l'écriture ; tes réponses, plus bas, ont depuis tranché les quatre choix (D-0307).

## Ce que l'étude a trouvé

1. **Il manque à l'image tracée la lumière du ciel, pas des rebonds.** Aujourd'hui les rayons prennent le ciel tel qu'il est
   dessiné, et ce ciel dessiné est sombre : environ 0,02 à 0,07 au High Wall (mesure de R, I-0132). Les sondes cuites du jeu
   d'origine donnent à ciel ouvert une ambiante d'environ 0,27 à 0,29, bleu-gris. Le ciel voulu par les auteurs éclaire donc
   quatre à quatorze fois plus que son image (ordre de grandeur, à confirmer par une mesure en jeu). C'est sans doute la première cause
   (déduit, à confirmer en jeu) des extérieurs sombres, des personnages en silhouette et du joueur trop chaud.
2. **L'ambiante d'origine est froide partout.** 19 banques de sondes sur 22 sont bleu-gris à 83 % ou plus, en plein air comme
   sous couvert ; sous couvert elle est trois à cinq fois plus sombre. C'est ton « ambiante froide ». Exceptions : Ariandel
   (bleu-vert profond) et la Ringed City (gris neutre).
3. **Le cadre natif contient déjà la référence.** Le G-buffer porte encore l'ambiante cuite de chaque pixel (elle n'est
   simplement plus lue), et le moteur produit encore celle de chaque personnage. On peut donc mesurer, zone par zone et sans
   lancer l'original, l'écart entre l'ambiante tracée et celle des auteurs. Je propose de construire ce témoin avant de
   changer quoi que ce soit.
4. **La plupart des lumières posées dans les maps n'ont pas de source visible.** Au High Wall, 148 des 184 lumières de la
   banque flottent sans objet émetteur à moins de 3 m ; la moitié des 2 408 lumières du jeu sont froides ; 88 s'appellent
   « Fill » ou « Fake » dans les fichiers. À Irithyll au contraire 41 % sont sur une bougie, une torche ou une lanterne. La
   lumière des flammes, elle, vient des effets (1 269 effets portent une lumière), pas des banques.
5. **Une lumière ne sait pas à qui elle appartient.** Pour nommer les familles (bougie, torche, brasero, feu de camp, sort),
   il faut un petit registre rempli là où les effets créent leurs lumières ; les lumières de banque ont un nom lisible.
6. **Le brouillard de DS3 est une source de lumière sans lumière.** Une seule formule : chaque pixel est tiré vers une
   couleur fixe selon la distance et la hauteur, que l'endroit soit éclairé ou non. Dans une cave sombre il ajoute quand même
   sa couleur : c'est une part des voiles gris ou cyan, et très probablement le « voile de Firelink ».
7. **Elden Ring a remplacé ce brouillard par un volume en grille** (96 à 160 cellules de large, 24 à 40 tranches), éclairé
   par le soleil avec son ombre et par les lumières locales, mais avec des couleurs de brouillard tapées à la main et des
   lumières locales sans ombre. Sa structure est le modèle ; ses couleurs tapées sont ce que les rayons remplacent.
8. **Elden Ring a lui-même presque abandonné les fill lights** (intensité nulle dans 73 %, 96 % et 69 % de ses réglages) mais
   il cuit toujours son ambiante dans des volumes ; son ray tracing reste des ombres et de l'AO. Rien à copier pour l'ambiante.
9. **Les zones que tu as notées.** La Road of Sacrifices : le dos du joueur à 46 contre 87 dans l'original, plus chaud ;
   c'est le ciel manquant. Le château d'Archdragon Peak : une des ambiantes d'origine les plus claires du jeu, donc le ciel
   aussi. La Cleansing Chapel : dans sa zone (la Cathedral of the Deep) l'ambiante d'origine sous couvert est presque
   nulle (0,005 en médiane ; la chapelle seule n'est pas isolée) ; son éclairage vient donc surtout des lumières froides
   posées (62 sur 81 dans la zone) et du brouillard, pas de l'ambiante. Father Ariandel : le ciel par les ouvertures de la
   chapelle, puis le liseré orange à identifier une fois les familles nommées.

## L'ordre proposé pour le chantier

1. Les noms et les témoins : familles de lumières dans le collect, témoin de l'ambiante. Aucun changement d'image ; peut
   démarrer pendant que R finit le 11.
2. Le ciel comme source, à la force que les auteurs lui donnaient.
3. Les lieux couverts : fenêtres visibles comme émetteurs, lumières posées triées par famille.
4. La forme des flammes : une taille de source par famille (douceur des ombres), sans toucher aux intensités (D-0293).
5. Les triangles émissifs (lave, cristaux) échantillonnés comme lumières, là où un témoin le demande.
6. Le brouillard volumétrique éclairé par les vraies lumières et ombré par les rayons.
7. Le retrait des fill lights provisoires des cutscenes (D-0279, D-0282), puis le tour des zones.

Chaque tranche avec ses planches avant et après, à côté de l'original.

## Ce qui te revient

Aucune de ces questions ne bloque la tranche 1.

1. **Un ciel qui éclaire plus qu'il ne se montre.** Donner aux rayons la force d'origine du ciel en laissant son image telle
   quelle (mon conseil, sur planches), éclaircir aussi le ciel dessiné, ou garder le ciel dessiné comme seule vérité ?
2. **Les fill lights invisibles de DS3 lui-même.** Les garder comme l'éclairage voulu par les auteurs, ou retirer par famille
   celles qui n'ont pas de source une fois le ciel en place (la lecture stricte de « jamais une lumière d'appoint
   invisible ») ? Mon conseil : les nommer, régler le ciel, puis juger sur planches ; ne rien retirer avant, sinon les
   intérieurs redeviennent noirs.
3. **Les fenêtres comme émetteurs** là où une fenêtre visible est la source d'un intérieur fermé : d'accord sur le principe ?
4. **La fidélité du brouillard.** Garder le plafond d'opacité d'origine comme garde-fou et caler la couleur du brouillard sur
   la lumière du jour de la zone (mon conseil) ; et les light shafts d'écran : gardés, réglés, ou retirés là où le volume en
   fait de vrais ?

**Tes réponses** (22 novembre, 15:30 et 15:31) : pour 1 et 2 tu suis le conseil ; pour 3 oui, et la lumière des fenêtres
est celle de l'extérieur ; pour 4 tu as demandé mon conseil puis répondu « Ça me va » : plafond d'opacité gardé comme
garde-fou, couleur du brouillard calée sur le plein jour de la zone, light shafts d'écran retirés là où le volume en fait de
vrais (par zone, sur planches), les rayons posés comme effets (10003) gardés d'abord et regardés sur les mêmes planches.

## Une idée hors plan (D-0298)

Une fois le ciel devenu une vraie source et le brouillard éclairé, tout suit le soleil. Un mode photo où tu déplaces le
soleil de la zone (l'heure du jour) deviendrait possible presque gratuitement : ombres, ambiante et brouillard suivraient.
À ne faire qu'avec ton accord, après le chantier.
