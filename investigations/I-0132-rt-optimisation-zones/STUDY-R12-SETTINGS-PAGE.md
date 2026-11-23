# R12 : la page « Rekindled » des réglages, étude statique

10 novembre 2026, fil d'étude (S), pour le pilote R avant le code de R12 (D-0266 ; RT-PT-PLAN §6 et §8, ligne R12). Étude
statique seulement : aucune exécution du jeu, aucun code natif écrit. Sources : le binaire 1.15.2 (SHA-256 `EF5E07C5…`) lu par
GhidraMCP et par les lecteurs de [study-r12-tools/](study-r12-tools/) ; les fichiers du jeu recopiés en lecture seule dans le
scratchpad par `I-0066/Recover-GameDataByName.ps1` (pages `.gfx`, `font.gfx`, `01_common.tpf`, les FMG `menu` engUS et fraFR) ;
le natif de `main` `851ff840` et la branche `claude/r11j-taa` pour la netteté. Un sous-agent `ghidra-research` (Sonnet) a fait
le premier relevé de la page ; ce qui fonde la recommandation (le constructeur, ses deux appelants, les libellés, les listes de
choix, `FUN_140C20E20`, les textes FMG) a été relu par le fil dans GhidraMCP et dans les données. Les statuts : **OBSERVÉ** (lu dans le code ou les
données), **INFÉRÉ** (raisonné), **OUVERT**.

## 0. En bref

1. **La page Graphismes avancés du jeu n'est pas chiffrée.** `MENUS-MAP` §77.5 la disait chiffrée : ses onze lignes sont posées
   par du C++ en clair (`FUN_140AAFD10`), une ligne par clé de `GraphicsConfig.xml` (Texture … Shade, dans l'ordre du fichier),
   avec leurs textes FMG et leurs listes de valeurs. Le film Scaleform `02_042_PC_GraphicSetting` est muet : son ActionScript ne
   contient que des scripts de timeline (OBSERVÉ). Ce qui reste chiffré, c'est la mise à jour de `NewMenuSystem` et la copie
   config → bloc (MENUS-MAP §2, §77).
2. **La porte naturelle de notre page est « Advanced Settings ».** Le jeu ouvre la page des onze lignes par un seul constructeur,
   `FUN_140BE3150`, depuis le répartiteur de noms (`PcGraphicsDetailSetting`) et depuis une chaîne de travaux (`FUN_140AA8750`).
   La même page Options (`02_040_OptionSetting`) sert à l'écran titre (System) et en jeu (Esc, System) : un seul crochet couvre
   les deux (INFÉRÉ pour l'écran titre, à vérifier au premier run de R12).
3. **Ce que notre natif peut réutiliser sans rien emprunter hors du jeu installé :** les polices des menus (contours vectoriels
   `DefineFont3` dans `font.gfx`, avec avances et crénage), les cadres, curseurs, barres et ornements de `01_common.tpf` (BC7,
   déjà chargés par le jeu et importés par le natif), les sons du menu (`f000000100` curseur, `200` validation, `300` valeur
   changée, par `FUN_14076B930`), les actions d'entrée du jeu (clavier DirectInput, manette XInput, souris), et l'injection de
   nos dessins dans le paquet d'UI du natif (`native_ui_frame`, après les dessins Scaleform).
4. **Recommandation pour R :** une petite boîte à outils 2D native (texte, cadres, curseur, barre, case, info-bulle), qui sert
   aussi à l'écran de précompilation et à l'écran titre ; la page ouverte à la place de « Advanced Settings », l'entrée des menus
   du jeu fermée pendant qu'elle est ouverte, et un fichier de réglages unique à côté du module.
   **Décision de Thomas** (§6, 10 novembre 05:09) : notre page remplace « Advanced Settings » et recrée elle-même toutes ses
   lignes, dont les cinq de DS3 qui servent encore ; les six que le RT remplace disparaissent.

## 1. La page Options du jeu et sa page Graphismes avancés

### 1.1 L'arbre des pages (OBSERVÉ, FMG engUS `menu_dlc2`)

- **Options** (`02_040_OptionSetting`, PC : `menu/win/02_040_optionsetting.gfx`) : une fenêtre d'options générique de 0x1168
  octets (vtable finale `1428C7558`), qui connaît treize sous-pages par nom (table de pointeurs ASCII `1445D1F10`, lue par
  `FUN_140AB34B0`) : `Top`, `LanguageSetting`, `ControllSetting`, `CameraSetting`, `EnvironmentSetting`, `BrightnessSetting`,
  `NetworkSetting`, `PCSetting`, `PCMouseKey`, `PCGraphic`, `MouseOption`, `GraphicOption`, `GameEnd`. Le film a une liste
  d'onglets (`TabList`, `BackTabList`) et une liste de fenêtres (`WindowList`).
- **Graphics** (texte 700002, sous-page `PCGraphic`) : Screen Mode (720100), Resolution (720101), Auto-detect best rendering
  settings (720102), Quality Setting (720103), **Advanced Settings** (720104).
- **Advanced Settings** (titre 721000 ; film `02_042_PC_GraphicSetting`, fenêtre ouverte sous le nom `PcGraphicsDetailSetting`,
  sous-page `GraphicOption`) : les onze lignes du §1.2.
- Ailleurs : Brightness, Sound and display (110100), Network (110300), Camera Options (110001), Controls (114000), Mouse et Key
  Bindings (`PcKeyConfig`, `02_043_PC_KeyConfiguration`, `02_041_PC_MouseSetting`), Quit Game. Le titre a son propre « System »
  (401304) et « Options » (115000) (INFÉRÉ : la même page `02_040` ; la table des descripteurs de pages contient `02_040` deux
  fois, sortes 4 et 14, §1.4).

### 1.2 Les onze lignes et la table d'options (OBSERVÉ)

`FUN_140BE3150` construit la fenêtre (`FUN_140BE3CD0`, descripteur {sorte 4, `02_042_PC_GraphicSetting`}, foncteur
`FUN_140BE3270`) ; `FUN_140BE3270` appelle `FUN_140AB34B0(…, L"GraphicOption", &FUN_140AAFD10, &FUN_140AAE950, 0, 0)`.
`FUN_140AAFD10` pose les lignes par `FUN_140AA4210(fenêtre, libellé, &état[0x65+i], choix, &défaut[i])` :

| Ligne | Octet de l'état | Libellé (FMG) | Choix (valeur : texte) | Clé du fichier, champ de `CSGraphicsConfig` |
|---|---|---|---|---|
| 0 | `+0x65` | 721100 Texture quality | 1 Low, 2 Medium, 3 High, 4 Max | `TextureQuality`, `+0x1C` |
| 1 | `+0x66` | 721101 Antialiasing | 0 Off, 3 On | `Antialiasing`, `+0x20` (ON 1 devient 3) |
| 2 | `+0x67` | 721102 SSAO | 0 Off, 2 Medium, 3 High | `SSAO`, `+0x24` |
| 3 | `+0x68` | 721103 Depth of field | 0 Off, 1, 2, 3, 4 | `DepthOfField`, `+0x28` |
| 4 | `+0x69` | 721104 Motion blur | 0 Off, 1, 2, 3 | `MotionBlur`, `+0x2C` |
| 5 | `+0x6A` | 721105 Shadow quality | 0 Off, 1, 2, 3, 4 | `ShadowQuality`, `+0x30` |
| 6 | `+0x6B` | 721106 Lighting quality | 1 à 4 | `LightingQuality`, `+0x34` |
| 7 | `+0x6C` | 721107 Effects quality | 1 à 4 | `EffectsQuality`, `+0x38` |
| — | — | 721108 (vide : le slot 8, l'octet des décals, sans clé) | — | — |
| 8 | `+0x6D` | 721109 Reflection quality | 0 Low (le niveau 0 de la table : pas de reflet), 3 High, 4 Max | `ReflectionQuality`, `+0x3C` |
| 9 | `+0x6E` | 721110 Water surface quality | 0 Low, 3 High | `WaterSurfaceQuality`, `+0x40` |
| 10 | `+0x6F` | 721111 Shader quality | 1 à 4 | `ShadeQuality`, `+0x44` |

Les textes des choix : 722021 Low, 722022 Medium, 722023 High, 722024 Max, 722040 On, 722041 Off (fraFR : « Qualité des
textures », « Anticrénelage », « Occlusion ambiante », …). Les listes de choix sont des tableaux d'entrées de 0x78 octets
(octet `+0` la valeur, texte à `+8`, 32 au plus) faits par `FUN_140A98F30`, `FUN_140A99600`, `FUN_140A990A0`, `FUN_140A99310`,
`FUN_140A99B50`, `FUN_140A997F0`, `FUN_140A99960`. La correspondance ligne → clé est établie par `FUN_140C20E20`, qui remplit les
défauts : il construit une config (`FUN_140EF6700`, 1920x1080, préréglage 3), applique le préréglage (`FUN_140EF7340`), puis
copie les onze `int` `+0x1C..+0x44` en octets, le seul remappage étant `Antialiasing == 1 → 3` (lu dans le code) ; l'ordre est
celui du fichier (MENUS-MAP §77.1).

**Une ligne** (`FUN_140AA3D30` → `FUN_140AA36E0`, objet de 0x1618 octets, `FUN_140AA5220`) garde le pointeur vers sa valeur
(`+0xDE8`), une copie de son défaut (`+0xDF8`), ses choix (`+0xE00`, nombre à `+0x1608`) ; l'index affiché est la première
entrée dont l'octet vaut `*valeur` ; `FUN_140AAC4E0` (vtable de ligne `1428C7928`, troisième entrée) change la valeur dans un
sens donné et redessine (INFÉRÉ). D'autres sortes de lignes existent : liste (`FUN_140AA40F0`), curseur 0 à 10 pas 1
(`FUN_140AA43C0`, page Brightness).

**« Défauts »** : `FUN_140AAE950` ouvre la boîte « Restore all default settings. Okay? » (40500, `FUN_140AA7BE0`) ; les défauts
sont ceux de `FUN_140C20E20` (INFÉRÉ : le préréglage High).

**OUVERT** : qui remplit `état+0x65..` depuis la config vivante, qui écrit la valeur choisie, et quand la page applique et
enregistre (`GraphicsConfig.xml` est écrit par `FUN_140EF75A0`, MENUS-MAP §77.1) ; les sons et les actions lus par la ligne.

### 1.3 Ce que lit le natif aujourd'hui

Le natif lit les douze niveaux à `[image+0x4796298]+0xA8` à chaque rendu (`10_options_and_passes.inc`, `ReadNativeOptionLevels`,
`UpdateNativeQualityObject`) : ce que la page du jeu écrit lui parvient déjà. Sous le RT (RT-PT-PLAN §6, D-0262), six lignes ne
commandent plus rien : Antialiasing (remplacé par le TAA et DLSS, D-0265), SSAO, Shadow, Lighting, Reflection et Shader quality
(le SSS reste « à décider » au §6). Restent utiles : Texture (filtrage), Depth of field, Motion blur, Effects, Water surface.

### 1.4 Les descripteurs de pages (OBSERVÉ)

Un initialiseur statique (code non désassemblé `14268F9FA..142690270`, lu par `ScanInit.java`) remplit environ 69 descripteurs
de 16 octets {sorte, drapeau, nom} à `144799520..144799940`, dont les pages d'options : `02_040_OptionSetting` (sorte 4 ;
une seconde entrée de sorte 14 à `144799320`), `05_031_NewGame_BrightnessSetting`, `05_032_NewGame_ControlSetting`,
`02_041_PC_MouseSetting`, `02_042_PC_GraphicSetting`, `02_043_PC_KeyConfiguration`, `02_044_PC_TextSelect`.

## 2. Les points d'entrée

| Entrée | Où | Ce qu'il faudrait | Statut |
|---|---|---|---|
| **A. À la place d'« Advanced Settings »** | `FUN_140BE3150` (appelé par le répartiteur `FUN_140BE2F80` pour `PcGraphicsDetailSetting` et par `FUN_140AA8750`, une chaîne de travaux référencée seulement comme donnée) | un détour de `FUN_140BE3150` ou de son foncteur `FUN_140BE3270` ; la fenêtre du jeu reste ouverte mais cachée (ses dessins sautés dans le paquet d'UI) et sourde (porte d'entrée fermée, ci-dessous) ; à la fermeture de notre page, une annulation rendue à la fenêtre cachée la ferme par le chemin du jeu | OBSERVÉ pour les adresses ; INFÉRÉ pour la fenêtre cachée |
| B. Un bouton de plus sur la page Graphics | la sous-page `PCGraphic`, lignes posées en C++ par `FUN_140AB07D0` (§6.1) | une ligne-bouton de plus (`FUN_140A8F920`) : possible, mais Thomas a choisi la place d'« Advanced Settings » (§6) | non retenue |
| C. Une touche affichée sur la page Graphics | le natif sait quelle fenêtre est ouverte (`[[14478DA40]+0xA0]`, sa vtable) | un rappel de touche dessiné par nous (« Rekindled [touche] ») et notre page sur cette touche ; pas de crochet dans le code des pages | possible, moins découvrable |
| D. Une ligne de plus au menu titre | `05_000_Title` (liste C++ de `TitleTopDialog`) | ajouter une commande à une liste C++ dont le constructeur n'est pas lu | OUVERT |

**Fermer l'entrée du jeu pendant que notre page est ouverte :** toutes les requêtes d'entrée des menus Scaleform passent par
`FUN_140A5B930(ctx)` (MENUS-MAP §51) ; la faire répondre « non » pendant que notre page est ouverte rend la fenêtre du jeu
sourde, au titre comme en jeu, sans toucher aux actions elles-mêmes, qui continuent d'être mises à jour.

**À vérifier au premier run de R12 (lecture seule, sans changer le jeu) :** que « System » à l'écran titre mène à la même page
Graphics et au même `FUN_140BE3150` ; que fermer la fenêtre cachée par une annulation ne réécrit pas la config (ou alors avec
les valeurs que notre page a posées, §3.5).

## 3. Ce que le natif peut réutiliser

### 3.1 Le dessin : le paquet d'UI du natif

Le rendu natif reçoit chaque image l'UI préparée sous la forme d'un `PrimitiveWorkPacket` (`17_render_step.inc`, `native_ui_frame`
pris de `GameUiSession::TakePreparedFrame`) : programmes, tampons, textures, dessins ordonnés, cibles intermédiaires
(`primitive_work.h`). Notre page ajoute ses propres dessins **après** ceux des films : des quads texturés et des quads de glyphes,
avec les mélanges déjà là (`PrimitiveBlend` : alpha, additif, prémultiplié, multiplication, écran), les ciseaux, les masques de
pochoir et les cibles intermédiaires (pour une ombre floutée). Aucun nouveau passage de rendu ; l'UI garde sa composition (et
plus tard le HDR de D-0182). Les textures de `01_common.tpf` sont déjà importées par le natif quand les menus les demandent
(`game_ui_image_owner`, MENUS-MAP §52-53) : notre page référence les mêmes enregistrements par leur nom.

### 3.2 Les polices (OBSERVÉ)

Les quatre familles des menus sont des `DefineFont3` (contours quadratiques, avances, crénage) dans `data1/font/<famille>/font.gfx` :

| Fichier | Police | Glyphes | Rôle (langue anglaise, FONTS-LANGUAGES-NATIVE §4) | Accents français |
|---|---|---|---|---|
| `MatisseProN` | FOT-Matisse ProN DB | 7 567 | `MenuFont_01` (corps des menus) | incomplets (37 lettres latines accentuées) |
| `TsukuOldMinPro` | FOT-TsukuOldMin Pro R | 208 | `MenuFont_02` (titres, en anglais) | complets |
| `TsukuMinPro` | BNE-TsukuMin Pro LB | 7 638 | `MenuFont_01` des langues européennes | complets |
| `Polish` | BNE-TsukuMin Pr5 LB | 7 647 | polonais | complets |

Le natif peut lire ces contours au moment voulu et les rastériser lui-même dans un atlas A8 (une couverture suréchantillonnée
4x4 aux tailles de la page suffit), avec l'ombre portée des menus (noire, flou 2x2, distance 2, angle 0,8 rad : MENUS-MAP §78.2)
faite par un second atlas flouté. Lire le fichier : par le système de fichiers virtuel du jeu (le chargeur de films l'ouvre par
`font:/<famille>/font.gfx`, FONTS-LANGUAGES-NATIVE §4 ; la fonction d'ouverture est à trouver, OUVERT) ; jamais une copie livrée
avec le module. `DbgFont14h` (police bitmap de débogage, 14 px) existe mais n'a pas l'allure de DS3. La langue de la page : celle
du jeu (l'ordinal `14479BA10`), anglais par défaut ; nos libellés à nous sont à écrire au moins en anglais et en français.

### 3.3 Les cadres et les pièces (OBSERVÉ, décodées en PNG dans le scratchpad)

`menu/01_common.tpf` (126 textures, BC7 sauf cinq BC1) contient ce que dessinent les pages Options :

- `MENU_OptionBase` (2048x1024) : le grand panneau sombre au bord déchiré, deux colonnes, et six icônes dorées de catégories ;
- `MENU_Top` (1024x1024) : les cadres de panneau à filet doré, le trait orange sous un titre, les icônes du menu principal ;
- `MENU_ItemParts` (1024x512) : la lueur orange du curseur (barres et halo), les cadres de champ, les flèches gauche et droite,
  les marques, les barres de curseur ;
- `MENU_BaseDeco` (256x256) : l'ornement de coin ; `MENU_Base`, `MENU_BaseU` (2048x2048) : les fonds ; `MENU_BrightnessSetting`
  (BC1) : l'image de la page Luminosité ; `MENU_L_Title`, `MENU_ProfileSelect_Cursor`, `MENU_KickCard_Cursor` : bandeaux et
  curseurs.

Les pages utilisent ces textures comme atlas : les rectangles exacts sont dans les remplissages bitmap des films
(`02_040_optionsetting.gfx`, `02_042_pc_graphicsetting.gfx`, matrices des `DefineShape`), à extraire une fois (mécanique,
`swftags.py` donne les étiquettes ; MENUS-MAP §78 a le lecteur `Get-GfxFeatures.ps1`). `MENU_PC_SettingParts` (pièces des pages PC)
n'est pas dans `01_common.tpf` (OUVERT : son paquet).

### 3.4 Les sons (OBSERVÉ pour le chemin, INFÉRÉ pour le sens)

`FUN_14076B930(id)` joue l'événement `f<id>` sur un canal du menu (MENU-AUDIO-NATIVE §3) ; les menus Scaleform passent par
`FUN_140A67CA0`, joué à la fin de la mise à jour de `NewMenuSystem`. Pour notre page : 100 (curseur), 200 (validation), 300 (valeur
changée), 1010 ou 1000 (ouverture), appelés depuis le thread du jeu (là où tourne la mise à jour des menus), pas depuis le rendu.
Le sens exact de 400, 600 et 900 n'est pas lu ; une trace de `FUN_1404AD510` en naviguant dans la page du jeu le donnerait.

### 3.5 Les entrées

- Clavier : DirectInput (`FUN_14183BE70`, 256 octets à `device+0x1A0`), coopératif en arrière-plan ; souris : `GetKeyboardState`
  et la position du natif (MENUS-MAP §51) ; manette : le binaire contient `XINPUT1_3.dll` (OBSERVÉ comme chaîne ; MENUS-MAP §51
  disait « pas d'import XInput », à corriger).
- Les actions de menu du jeu (validation `0x13`, impulsion `+0xC4`, répétition 0,2 s puis 1/15 s : NATIVE-INPUT-LATCH) suivent la
  configuration des touches du joueur : lire les impulsions de ces actions (et non les touches brutes) garde les mêmes touches que
  les pages du jeu. Les identifiants de haut, bas, gauche, droite et annulation sont OUVERTS (les sites de `FUN_14233DE40`).
- Le loquet de NATIVE-INPUT-LATCH (une pression entre deux mises à jour livrée comme une mise à jour) vaut aussi pour notre page.

### 3.6 L'écriture des valeurs de DS3 depuis notre page

Si notre page reprend les cinq lignes utiles de DS3 (§6), le chemin le plus sûr écrit dans l'état de la fenêtre cachée du jeu
(`état+0x65..+0x6F`, les pointeurs `+0xDE8` des lignes) plutôt que dans `CSGraphicsConfig` : l'application et l'enregistrement
de `GraphicsConfig.xml` restent alors ceux du jeu (INFÉRÉ ; dépend du chemin d'application, OUVERT au §1.2).

## 4. Nos réglages : où ils vivent et quand ils prennent effet

Aujourd'hui chaque choix est un fichier d'un mot à côté du module, lu **une fois au démarrage** (`01_state.inc`) : `rt-level.txt`,
`dlss-mode.txt`, `rr-mode.txt`, `reflex-mode.txt`, `fg-mode.txt`, `rt-bounces.txt`, `frame-rate.txt` (`15_cpu_service_and_input.inc`),
`sharpen.txt` (R11j), `borderless.flag`, `vsync-off.flag`, `no-taa.flag`, `no-auto-exposure.flag`. Le lanceur de Thomas écrit
certains d'entre eux dans son dossier de jeu `%LOCALAPPDATA%/DS3-DX12` (`tools/Launch-DS3-DX12.ps1` : `-FrameRate`, `-Reflex`).

Recommandation :

- **Un seul fichier de réglages** (par exemple `rekindled-settings.ini`) à côté du module, écrit par la page à la validation, lu
  au démarrage ; les fichiers de test (`*.flag`) gardent la priorité, comme aujourd'hui. À côté du module plutôt que dans
  `%APPDATA%/DarkSoulsIII` : les runs de test, qui ont leur propre dossier, ne touchent jamais les choix de Thomas. Le lanceur écrit
  dans ce fichier au lieu des `.txt` (ou les retire).
- **Quand une entrée prend effet**, par entrée, décidé par R au code : en direct (TAA, netteté, aberration chromatique, vignettage,
  plafond d'images, Reflex), après une recréation (mode DLSS, RR, niveau RT/PT, rebonds), au prochain lancement (génération
  d'images tant que la chaîne de Streamline se fait au démarrage ; sans bordures, posé au démarrage de la fenêtre). Une entrée qui
  attend le prochain lancement le dit sur la page (« au prochain lancement »), comme le font les jeux PC.
- **Les règles de D-0265 dans le modèle, pas dans le dessin** : une table des entrées (valeur, choix, défaut, disponibilité selon
  les autres valeurs : PT, RR et génération d'images seulement avec un mode DLSS ; Reflex Off verrouillé avec la génération
  d'images ; une option indisponible grisée et verrouillée, jamais cachée), testée par des tests unitaires comme `dlss_mode_test`.

## 5. Recommandation pour le pilote R

1. **Une boîte à outils 2D native, d'abord**, petite et testée : atlas de glyphes depuis `font.gfx` (avances, crénage, ombre),
   quads texturés depuis les atlas de `01_common.tpf` (rectangles tirés des films), curseur orange, flèches, barre de curseur, case,
   ligne d'aide, avec mise à l'échelle par la hauteur de sortie (la page de DS3 est faite pour 1280x720, MENUS-MAP §51). Elle
   sert ensuite l'écran de précompilation (sa barre de progression) et l'écran titre « Rekindled » de D-0182.
2. **La page** : le modèle du §4 (table d'entrées, règles de D-0265, défauts de D-0265 et D-0266), la navigation par les actions
   du jeu, les sons du §3.4, le dessin dans le paquet d'UI ; par catégories (Rendu, Upscaling et anticrénelage, Images et latence,
   Image, Affichage, et les lignes de DS3 du §6), chaque ligne avec sa phrase d'aide comme la page du jeu (le second texte d'une
   ligne, id + 100000).
3. **La porte** (§6, Thomas : toute la main) : l'entrée A, notre page à la place d'« Advanced Settings ». Premier choix, sans
   fenêtre du jeu : le détour de `FUN_140BE3150` rend au cadre des menus un travail qui ne montre rien et qui se termine quand notre
   page se ferme (le cadre `FUN_140BAFDC0`/`FUN_140BAFE10` attend des travaux : en trouver un neutre du jeu, ou en écrire un petit
   natif sur sa vtable, à lire au code) ; pendant ce temps `FUN_140A5B930` répond « non », et la page Graphics du jeu reprend la
   main à la fin du travail. Repli, si ce travail se montre fragile : laisser le jeu construire sa fenêtre mais la cacher et la
   rendre sourde, puis lui rendre une annulation à la sortie ; nos valeurs restent écrites par notre code (§6), jamais par cette
   fenêtre.
4. **Avant le code** : un court run (sur annonce, sans changer le jeu) pour les points ouverts qui fixent la porte : le titre mène
   au même `FUN_140BE3150` ; ce que la page Graphics du jeu fait au retour (relit-elle la config ?) ; les ids d'action de
   navigation et les sons de la page du jeu (une trace de `FUN_1404AD510` et des sites de `FUN_14233DE40`). Le reste se lit
   statiquement au fil du code.
5. **Découpage proposé** : R12a la boîte à outils 2D (texte depuis `font.gfx`, pièces de `01_common.tpf`) et un écran de test ;
   R12b le modèle des réglages (table d'entrées, règles de D-0265, fichier unique, tests unitaires) et le passage des `.txt` du
   module et du lanceur à ce fichier ; R12c la page et sa porte (§2, §6), les cinq lignes de DS3 écrites par nous ; R12c bis, dans la page Graphics du jeu, Sans
   bordures dans la ligne Screen Mode et les lignes V-sync et fréquence d'images (§6.1) ; R12d l'écran de
   précompilation ; R12e l'écran titre « Rekindled » (D-0182). Chaque entrée qui ne peut pas changer en direct le dit sur la page.

## 6. La décision de Thomas (10 novembre, 05:09 Paris)

La question : D-0266 gardait les réglages de DS3 « là où ils sont », mais la page avancée du jeu offre six lignes que le RT a
remplacées (anticrénelage, SSAO, ombres, éclairage, reflets, shaders), qui resteraient visibles et sans effet. Thomas, à la
recommandation du fil d'étude : « Je préfère l'option de R qui est de tout recréer pour avoir toute la main dessus » (lecture du
coordinateur, retenue ici) :

- **Notre page « Rekindled » remplace « Advanced Settings »**, au titre comme en jeu ; la page `02_042_PC_GraphicSetting` du jeu
  n'est plus montrée.
- **Toutes ses lignes sont recréées par nous**, y compris les cinq de DS3 qui servent encore (qualité des textures, profondeur de
  champ, flou de mouvement, effets, surface de l'eau) ; les six que le RT remplace disparaissent, leurs remplaçantes sont nos
  catégories RT. Notre code lit et écrit lui-même les champs de `CSGraphicsConfig` de ces cinq lignes (§1.2) et l'enregistrement
  de `GraphicsConfig.xml` (le fichier du jeu, par son écrivain `FUN_140EF75A0`, MENUS-MAP §77.1, ou par une écriture native des
  mêmes clés) : aucune dépendance à l'état de la fenêtre du jeu, d'où la porte du §5.3 sans fenêtre cachée.
- Screen Mode, Resolution, Auto-detect et Quality Setting restent sur la page Graphics du jeu ; **sauf l'affichage** (Thomas,
  05:10 et 05:11) : « Borderless doit être à côté des deux autres », « Vsync et le framerate aussi » : Sans bordures dans la ligne
  Screen Mode du jeu, à côté de Plein écran et Fenêtré, et la V-sync et la fréquence d'images sur la même page Graphics du jeu
  (§6.1).
- Un préréglage choisi par la ligne Quality Setting du jeu écrit encore les onze niveaux : sans effet sur les six remplacés, il
  change les cinq lignes reprises, que notre page relit à son ouverture.

### 6.1 Sans bordures, la V-sync et la fréquence d'images dans la page Graphics du jeu (OBSERVÉ pour le code, INFÉRÉ pour l'écran)

**C'est faisable sans toucher au film Scaleform**, parce que la page Graphics est elle aussi posée par du C++ en clair :
`FUN_140AB07D0` (ses appelants ne sont pas lus) ajoute, dans l'ordre, Screen Mode (libellé `FUN_140AABDD0`,
720100 ; ligne à foncteurs `FUN_140AB1C20`, vtables de foncteurs `1428C7A28`, `1428C7A68`, `1428C7AA8`), Resolution
(`FUN_140AABD90`, 720101 ; `FUN_140AB1C20`, foncteurs `1428C7AE8`, `1428C7B28`, `1428C7B68`), Auto-detect (`FUN_140AABD50`, 720102 ;
ligne de choix `FUN_140AA40F0` sur l'octet `état+0x63`, choix `FUN_140AAB920`), Quality Setting (`FUN_140AABCD0`, 720103 ;
`FUN_140AB18C0`) et Advanced Settings (`FUN_140AABC90`, 720104 ; ligne-bouton `FUN_140A8F920`, foncteur `1428C7C68`).

- **La ligne Screen Mode** lit un booléen : son lecteur (foncteur `1428C7A28`, appel `FUN_140AB4AD0`) rend
  `FUN_140B07AA0() = ([14489F118]+0x78 == 1)`, le plein écran ; sa liste de choix (foncteur `1428C7A68` → `FUN_140AA46C0` →
  `FUN_140AA7750` → `FUN_140A9A220`) pose deux entrées, 722000 « Full-screen » et 722001 « Windowed » (liste de 32 entrées au
  plus). Pour Sans bordures : un détour de `FUN_140A9A220` ajoute une troisième entrée ; les foncteurs de lecture et d'écriture de
  cette ligne sont remplacés par les nôtres, qui passent d'un booléen à trois valeurs (Plein écran, Sans bordures, Fenêtré) ;
  Sans bordures garde `ScreenMode` `FULLSCREEN` dans `GraphicsConfig.xml` (le jeu n'a que deux jetons, MENUS-MAP §77.1) et se
  note dans notre fichier de réglages ; le natif le pose par son chemin de fenêtre (I-0076, aujourd'hui `borderless.flag` lu au
  démarrage). Le texte « Borderless » (« Sans bordures ») n'existe dans aucun FMG : notre texte passe par un détour du lecteur de
  textes (`FUN_140A67060` → `FUN_140A672E0`) pour un id réservé, dans la langue du jeu.
- **La V-sync et la fréquence d'images** : deux lignes de plus, ajoutées par notre code à la fin de `FUN_140AB07D0` avec la
  fonction de ligne du jeu (`FUN_140AA40F0`, liste de choix sur un octet à nous, ou `FUN_140AB1C20` à foncteurs), leurs libellés et
  leurs choix par le même détour du lecteur de textes ; leurs valeurs vivent dans notre fichier de réglages (aujourd'hui
  `vsync-off.flag` et `frame-rate.txt`, lus au démarrage).
- **Ce qui reste à voir au premier run** : que la liste du film `02_040` affiche sept lignes sur cette sous-page (d'autres
  sous-pages en ont plus : INFÉRÉ oui) ; que l'écriture du mode d'affichage par le jeu (différée, DISPLAY-SWAPCHAIN-NATIVE) et la
  nôtre ne se contredisent pas ; quand chacune des trois prend effet (en direct ou au prochain lancement, dit sur la page).

## 7. Points ouverts

- Le remplissage de l'état de la page depuis la config vivante, l'écriture de la valeur choisie, l'application et
  l'enregistrement (§1.2).
- Le constructeur des lignes de `Top` (celui de la page Graphics est `FUN_140AB07D0`, §6.1), et le chemin du titre (System, Options) vers la page Options.
- Les ids d'action de navigation et d'annulation ; le sens des sons 400, 600, 900.
- La fonction du système de fichiers qui ouvre `font:/…/font.gfx` pour le natif ; le paquet de `MENU_PC_SettingParts`.
- Le rôle de `FUN_140AA8750` (référencé seulement comme donnée).

## 8. Outils et reproduction

[study-r12-tools/](study-r12-tools/), Python sans dépendance (`python -I`), sur des copies dans un dossier temporaire, jamais
dans le dossier du jeu :

- `u16scan.py <exe> <mots>` : chaînes UTF-16 du binaire avec leur adresse ; `rel32.py <exe> <adresses>` : références
  `rip`-relatives à des adresses, y compris dans le code que Ghidra n'a pas désassemblé ;
- `swftags.py`, `gfxinfo.py <gfx>` : étiquettes d'un film, images externes, symboles, chaînes ActionScript ; `font3.py <font.gfx>` :
  nom de la police, nombre de glyphes, couverture ;
- `tpf.py <tpf.dcx> [noms]` : liste d'un TPF (et le décompresse) ; `bcdec.py` : décodeur BC1/BC3/BC7 (tables de partitions
  écrites de mémoire, sans le mode 2 à trois sous-ensembles au-delà de la partition 15 : vérifié par des blocs synthétiques et à
  l'œil, pas contre un décodeur de référence) ;
- `fmgdump.py <msgbnd.dcx> <regex> <id min> <id max>` : textes d'un FMG par id ou par motif ;
- `ScanInit.java`, `FuncSummary.java` : scripts Ghidra sans écriture, lancés par `Invoke-Ghidra.ps1 -ProjectDir` sur le miroir
  `ghidra-mirror-menus`.

Les fichiers du jeu ont été recopiés par `investigations/I-0066-independent-native-startup/Recover-GameDataByName.ps1 -Archives Data1`
avec les motifs `^data1/(font/[^/]+/font\.gfx|menu/(win/)?0[25]_0[04].*|menu/01_common.*|menu/05_000.*)` et
`^data1/msg/(engus|frafr)/menu`, sortie et rapport dans le scratchpad (octets sous licence, jetables).
