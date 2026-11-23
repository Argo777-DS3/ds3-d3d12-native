> Copy (2026-10-26, I-0100 opening, D-0214) of the project-file study `etudes/pop-in/ANCIENNE-ETUDE-POP-IN.md` (first named `etude-pop-in/ETUDE-POP-IN.md`), read-only, 2026-10-24, the input of D-0211. Kept as written, in French.

# La règle anti pop-in (D-0197) face aux jeux modernes

Étude en lecture seule du 24/10/2026, rien n'est changé dans le dépôt. La décision revient à Thomas.

Statuts : **[dépôt]** = lu dans le dépôt (lien donné) ; **[web]** = source en ligne (liens en bas) ; **[général]** = connaissance générale, non vérifiée ici.

## 1. Ce que dit la règle aujourd'hui

D-0197 (`decisions/LOG.md`, 21/10) : si le moteur natif cesse de faire attendre DS3 pendant la création d'une texture, une texture pas encore prête **retarde de quelques images l'apparition du modèle** qui l'utilise. Aucune texture neutre de remplacement n'est jamais affichée. L'horloge du jeu n'est pas touchée. La règle ne s'applique que si les mesures montrent encore des images lentes dues à ces attentes. **[dépôt]**

Deux faits du dépôt changent un peu l'idée de départ :

- **DS3 d'origine attend.** Son créateur de textures lance un travail par texture, puis attend en bloquant que tout soit fini, sous un verrou (I-0073 ER-STUDY §4 et §4.1). L'original n'a donc pas de pop-in de texture sur ce chemin, mais il paie l'attente en saccades. **[dépôt]**
- **Elden Ring n'est pas « zéro substitut ».** ER ne publie qu'une texture complète : pas de mips partiels, et son moteur ne l'attend jamais (I-0073 ER-STUDY §1). Mais le support d'une texture **démarre sur `SYSTEX_DUMMY`** avant l'arrivée des données, et `CSMiddleGxTexture` passe à un **support de secours** quand le principal n'est pas valide (I-0071 ER-STUDY B, vérifié par Opus). Que ce secours soit la version basse résolution `_l` est seulement INFÉRÉ. Ce qu'ER dessine réellement tant qu'il est sur le dummy n'a pas été lu. DS3 a le même schéma, avec un support initialisé sur SYSTEX_DUMMY (I-0071 RESULT point 2). **[dépôt]**

Lien avec les saccades : la bande 25-34 ms de la tranche 4 correspond au moteur qui attend la fin des tâches de chargement de DS3 (FXR + GPARAM natif), d'après la fiche du projet. La règle décide donc aussi de ce qu'on a le droit de faire pour supprimer cette attente : un effet ou un éclairage qui arrive quelques images plus tard. **[dépôt / fiche]**

## 2. Les approches

| Approche | Qui l'utilise | Pour DS3 : avantages | Pour DS3 : inconvénients | Lien avec les saccades |
|---|---|---|---|---|
| **A. Attendre en bloquant** (DS3 d'origine) | DS3 d'origine **[dépôt]** | Image toujours juste, rien à concevoir | C'est la cause des images lentes à l'arrivée des ressources (I-0079 RESULT, causes 1 à 3) | **Pire cas** : chaque création est une saccade |
| **B. Retarder l'apparition** (D-0197 actuel) | Proche d'ER au niveau d'une texture (publiée seulement complète) **[dépôt]** | Jamais de texture fausse ; le moteur n'attend plus | L'objet **apparaît d'un coup** quelques images plus tard, ce qui reste une forme de pop-in (de l'objet entier). Un objet proche ou un sol qui manque se voit beaucoup. La collision est séparée du rendu chez FromSoftware **[général]**, donc le jeu reste juste | **Supprime la saccade** côté rendu ; le coût passe en latence d'affichage |
| **C. Mips progressifs** (petit mip d'abord, puis netteté) | Unreal : pool de streaming, biais de mip pour tenir le budget **[web 1]** ; Sampler Feedback Streaming sous DX12 : charge seulement les mips échantillonnés, boucle de retour sur plusieurs images **[web 2, 3]** | Ne bloque jamais ; gros gain mémoire ; le flou se voit moins qu'un trou | **Flou puis netteté visible** (pop-in de texture, exactement ce que Thomas trouve moche) ; les TPF de DS3 rangent toute la chaîne de mips dans un même fichier **[général]**, donc il faudrait découper les lectures ; complexe ; Sampler Feedback demande du matériel et des shaders adaptés **[web 2]** | Bon contre les saccades **d'E/S et de mémoire**, mais nos saccades viennent de la création et de l'attente, pas du débit (I-0079) |
| **D. Secours basse résolution** (paquet `_l`) | ER, probablement (INFÉRÉ) **[dépôt]** | Le moteur d'origine a déjà la mécanique du support de secours | Ça reste un substitut (flou puis net) ; on ne sait pas si DS3 fournit ces données : **à vérifier** | Même effet que C |
| **E. LOD, imposteurs, HLOD** | ER : 5 seuils de LOD avec hystérésis, fondus, distances ajustées par un équilibreur de charge (I-0071 ER-STUDY C) **[dépôt]** ; Nanite : géométrie en grappes chargée à la demande, transitions sans perte visible selon Epic **[web 4]** | Moins de données pour le lointain ; DS3 a déjà le même équilibreur (I-0071, point 1) | Concerne la **géométrie lointaine**, pas une texture en retard ; créer des imposteurs revient à changer les données du jeu | Indirect : moins de créations à l'arrivée |
| **F. Fondu ou tramage à l'apparition** | Unreal : transition de LOD tramée **[web 5]** ; ER : rampe de fondu linéaire (I-0071 ER-STUDY C) **[dépôt]** | **S'ajoute bien à B** : l'objet en retard apparaît en fondu au lieu d'un coup ; DS3 sait déjà faire des fondus alpha (I-0071 C : « as DS3's split ») | Il faut un passage de shader prévu pour ça ; un fondu de 5 à 10 images peut se voir de près | Aucun coût en saccades, il masque seulement la latence de B |
| **G. DirectStorage (+ décompression GPU GDeflate)** | Ratchet & Clank, Spider-Man 2 sur PC **[web 6, 7]** | Moins de CPU pour lire et décompresser, utile pour de gros volumes **[web 6]** | Il faut **recompresser les données** (DCX → GDeflate) ; gain nul sur PC haut de gamme selon Nixxes (cité **[web 8]**) ; sur certaines cartes, la décompression GPU **fait baisser les 1 % bas** : +18-25 % de 1 % bas en la coupant sur RTX 4090 dans Spider-Man 2 **[web 7]** | **Hors sujet pour nos saccades** : DS3 pèse peu, et nos images lentes viennent de la création, de l'attente et des verrous, pas du disque (I-0079, I-0073) |
| **H. Précharger en avance** (prédire avant l'entrée dans le champ) | Pratique courante des jeux en monde ouvert **[général]** | Réduit à la fois le pop-in et l'attente ; ne change pas les données | Plus de mémoire ; DS3 décide déjà quoi charger, par blocs de carte **[général]** | Fait de la place pour que B ne se voie presque jamais |

## 3. Lecture

1. **D-0197 tient la route, mais la vraie question est autre.** Le choix « pas de substitut » ne coûte rien en saccades si le moteur cesse d'attendre (B). Le coût est un retard d'apparition. Les jeux modernes ne cherchent pas « zéro pop-in ». Ils cachent ce retard : fondu (F), préchargement (H), et flou plutôt que trou (C/D).
2. **Le modèle ER est moins strict que la règle.** ER ne publie que des textures complètes, mais il part d'un dummy et a un support de secours (I-0071). Copier ER à la lettre autoriserait donc un substitut. D-0197 va plus loin qu'ER, et c'est un choix de qualité, pas une copie.
3. **Pour nos saccades actuelles** (attente de FXR et de GPARAM), seul B change quelque chose : un effet ou un réglage d'éclairage qui arrive quelques images plus tard. C, D, E et G ne visent pas cette cause.
4. **DirectStorage** : peu d'intérêt pour DS3, et du risque (données à recompresser, 1 % bas sur certaines cartes). À garder pour plus tard, s'il y a un jour de gros packs de textures.

## 4. Options pour Thomas (à trancher)

- **Garder D-0197 tel quel** : retard sans substitut, apparition d'un coup.
- **D-0197 + fondu** (suggestion d'Opus) : même règle, mais l'objet en retard apparaît en fondu court au lieu d'un coup. Le préchargement passe en piste pour que ce retard reste rare. Aucun substitut.
- **Assouplir pour le lointain seulement** : petit mip ou secours `_l` autorisé au-delà d'une distance, jamais de près. Cela suppose de vérifier que DS3 a ces données.
- **Mips progressifs partout** (style Unreal) : déconseillé, c'est le pop-in de texture que Thomas trouve moche, et ça ne traite pas nos saccades.

## 5. Piste de Thomas (24/10, 18h52) : tout précharger pendant l'écran de chargement et étendre la zone

### Ce que DS3 charge en jeu, hors écran de chargement

- En marchant à Firelink, un fil de DS3 lit et décompresse des fichiers : c'est **le rechargement des morceaux de carte par DS3 lui-même**, que l'original fait aussi (I-0097 RESULT §2). **[dépôt]**
- La tâche de chargement que le moteur attend fait deux choses. D'une part le travail natif : vérification et copie des modèles, création des tampons, lecture des GPARAM, construction des matériaux. Cette part a déjà été réduite par E1 et E2. D'autre part **le chargement propre de DS3** : chargement CPU des FLVER, enregistrement des FXR, registre des textures (I-0097 RESULT §2-4). **[dépôt]**
- Vers 68 textures par seconde arrivaient en marchant, avant les corrections (I-0079 RESULT). **[dépôt]**
- Une zone (m31) occupe 1,6 à 1,9 Go de VRAM, comme l'original (I-0096). La carte du PC, une RTX 4070 Ti SUPER, a 16 Go **[général]**. La place en VRAM ne manque donc pas. La taille d'une carte entière chargée d'un coup **n'est pas mesurée**. On peut la mesurer sans lancer le jeu, en additionnant les archives de la carte.
- Le chargement d'une sauvegarde prend 6,71 s en natif contre 6,62 s dans l'original (I-0097 RESULT). **[dépôt]**

### Faisabilité, en deux parties

1. **Préparer d'avance la part native : faisable, et c'est l'approche des jeux modernes.** Pendant l'écran de chargement, on décode et on envoie au GPU les textures et les modèles de toute la carte, on lit les GPARAM, on construit les pipelines. Ensuite, quand DS3 les demande en jeu, ce n'est plus qu'une recherche dans un cache : la tâche que le moteur attend raccourcit. Unreal fait la même chose pour ses pipelines : il compile à l'avance et demande à l'écran de chargement d'attendre que ce soit fini **[web 9]**. Le surcoût est quelques secondes de chargement et de la mémoire ; l'estimation est à mesurer.
2. **Faire précharger DS3 lui-même : difficile et risqué.** Le chargement CPU des FLVER, les FXR et les morceaux de carte relèvent de la logique du moteur de DS3. Le forcer veut dire changer sa politique de chargement, avec un risque sur les ennemis, les collisions et le réseau. Ces structures n'ont jamais été prévues pour plusieurs fils (ROADMAP). C'est pourtant cette part qui reste dans la bande 25-34 ms après E1 et E2. **[dépôt + INFÉRÉ]**

### Effet attendu sur les saccades de traversée

- Partie 1 : retire ce qu'il reste de travail natif dans la tâche attendue et les « premières fois » (la première construction des programmes d'un modèle, 3,5 à 28 ms dans la collecte, STATE). C'est un gain probable, mais **pas mesuré**.
- Partie 2 : seule elle s'attaquerait à la part propre à DS3, que l'original paie aussi.
- Dans les deux cas, presque plus rien n'arrive en retard. La règle D-0197 ne servirait plus qu'en secours, ce qui va dans ton sens.

### Étendre la zone d'affichage

- DS3 a le même équilibreur de distances qu'ER (`LoadBalancerDrawDistScaleParam`, I-0071 ER-STUDY). **[dépôt]**
- Dans ER, un mod met à 1 les « Draw Distance Scale End » de `LoadBalancerNewDrawDistScaleParam`, avec un impact jugé « minimal », mais sans chiffres **[web 10]**. Le levier existe donc côté paramètres. C'est un réglage visuel qui coûte du GPU et de la mémoire, plutôt une option V2 du lanceur.
- Plus de distance veut dire plus de choses à charger et à dessiner, donc plus de risque de saccades si la partie 1 n'est pas faite avant.

### Ce que font les jeux modernes

- **Préparer à l'écran de chargement**, et **ne pas dessiner tant que ce n'est pas prêt** : c'est la stratégie par défaut d'Unreal pour un pipeline pas prêt (« Delay : skip the draw until the PSO is ready »). C'est **exactement D-0197**. Les autres choix d'Unreal sont un matériau par défaut, qui serait un substitut, ou le blocage, qui donne une saccade **[web 9]**.
- Les mondes ouverts sans écran de chargement, comme ER, ne peuvent pas tout charger d'avance et streament en continu (I-0073) **[dépôt]**. DS3, découpé en zones avec des écrans de chargement, se prête mieux au préchargement **[INFÉRÉ]**.

Recommandation d'Opus : la partie 1 est le bon prochain essai. Elle se teste en deux passages de contrôle, puis en série A/B si le gain est plausible. Elle colle au mandat. La partie 2 et l'extension de la distance passent en piste ROADMAP.

## Sources web

1. Unreal Engine, *Texture Streaming Configuration* : https://dev.epicgames.com/documentation/en-us/unreal-engine/texture-streaming-configuration-in-unreal-engine
2. Microsoft DirectX blog, *Coming to DirectX 12 — Sampler Feedback* (2019) : https://devblogs.microsoft.com/directx/coming-to-directx-12-sampler-feedback-some-useful-once-hidden-data-unlocked/
3. Xbox GDK, *Why Sampler Feedback* : https://devdocs.xbox.com/build/core-features/graphics/newfeatures/sfs/why-sf
4. Unreal Engine, *Nanite Virtualized Geometry* : https://dev.epicgames.com/documentation/unreal-engine/nanite-virtualized-geometry-in-unreal-engine
5. Unreal Engine API, *AllowDitheredLODTransition* : https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/Engine/AllowDitheredLODTransition
6. NVIDIA, *Accelerating Load Times for DirectX Games and Apps with GDeflate for DirectStorage* : https://developer.nvidia.com/blog/accelerating-load-times-for-directx-games-and-apps-with-gdeflate-for-directstorage
7. Tom's Hardware, *Testing DirectStorage with GPU decompression — do Blackwell GPUs have the upper hand?* : https://tomshardware.com/pc-components/gpus/testing-directstorage-with-gpu-decompression-do-blackwell-gpus-have-the-upper-hand
8. DSOGaming, Ratchet & Clank PC et DirectStorage : https://dsogaming.com/?p=174525
9. Unreal Engine, *PSO Precaching* : https://dev.epicgames.com/documentation/en-us/unreal-engine/pso-precaching-for-unreal-engine
10. Wccftech, mod Elden Ring de distance d'affichage : https://wccftech.com/elden-ring-new-mod-increases-draw-distance-with-minimal-performance-impact/amp/

Recherche non concluante : je n'ai pas trouvé d'analyse technique sérieuse (type Digital Foundry) qui mesure le pop-in d'Elden Ring. Je n'affirme donc rien sur ce qu'on voit en jeu dans ER.
