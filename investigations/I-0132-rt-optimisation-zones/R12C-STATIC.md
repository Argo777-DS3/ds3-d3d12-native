# R12c bis et la porte de R12c : la lecture statique

10 novembre 2026 (Paris), le second pilote R12, avant le code (D-0266 ; [STUDY-R12-SETTINGS-PAGE](STUDY-R12-SETTINGS-PAGE.md) §5.3,
§6.1 ; [R12-PLAN](R12-PLAN.md)). Lecture par GhidraMCP (le binaire 1.15.2, SHA-256 `EF5E07C5…`) et, pour les fonctions que le
projet n'a pas désassemblées, par `I0106Fn.java` en session sans écriture sur le miroir `ghidra-mirror-menus` (fonctions créées en
mémoire seulement). Statuts : **OBSERVÉ** (lu dans le code ou les données), **INFÉRÉ**, **OUVERT**.

## 1. La ligne Screen Mode et Sans bordures (R12c bis)

### 1.1 Ce que pose `FUN_140AB07D0` (OBSERVÉ)

`FUN_140AB07D0(fenêtre, état)` (appelée seulement par pointeur, `.pdata`) ajoute les cinq lignes de la sous-page Graphics. La
ligne Screen Mode : `FUN_140AB1C20(fenêtre, libellé, &lecteur, &écrivain, [pile] &liste)`, trois `std::function` de MSVC 2015
(objet de 0x20 octets : stockage en ligne 0x18, pointeur d'impl à `+0x18` ; vtable d'impl à sept entrées : copie, déplacement,
`_Do_call` à `+0x10`, type, `_Delete_this` à `+0x20` avec un booléen « libérer », destructeur, `_Get`) :

| Foncteur | Vtable | `_Do_call` | Ce qu'il fait |
|---|---|---|---|
| lecteur | `1428C7A28` | `FUN_140AB4AD0` | `bool()` : `FUN_140B07AA0()` = `[14489F118]+0x78 == 1` (le mode appliqué, plein écran) |
| écrivain | `1428C7A68` | `140AB5140` (non désassemblé : `cmp byte [rdx],1 ; sete cl ; jmp FUN_140B07C40`) | `void(const uint8_t&)` : `[14489F118]+0xC0 = (v == 1)` (le mode demandé) |
| liste | `1428C7AA8` | `FUN_140AB4AF0` | `Liste*(Liste* ret)` : `FUN_140AA46C0` → `FUN_140AA7750` → `FUN_140A9A220(ret)` |

Chaque `_Do_call` n'est référencé que par sa vtable (recherche du pointeur absolu dans le fichier : une occurrence chacun), et
`FUN_140A9A220` n'a qu'un appelant (`FUN_140AA7750`, appelé seulement par `FUN_140AA46C0`, appelé seulement par
`FUN_140AB4AF0`) : un détour de chacune ne touche que cette ligne.

**La liste** (`FUN_140A9A220`) : `ret+0xF08` (le nombre) = 0, puis deux entrées de 0x78 octets ajoutées par
`FUN_140A9B560(liste, &entrée)` (au plus 32 ; entrées à partir de `liste` aligné sur 8) : {1, texte 722000 « Full-screen »},
{0, texte 722001 « Windowed »}. Une entrée : `+0` l'octet de valeur ; `+8` un texte (un qword puis une chaîne de 0x30 octets :
tampon de 16 octets, taille `+0x10`, capacité `+0x18`, allocateur `+0x20`) ; `+0x40` un second texte, vide (qword 0, chaîne
faite par `FUN_14007E060(chaîne, L"", FUN_140A67240())`). Le texte d'un id : `FUN_140A67060(sortie, id)` ; la copie de chaîne
`FUN_140002B50(dst, src)` ; une chaîne se libère si capacité ≥ 8 par `allocateur->vtable à +0x68 (allocateur, tampon)`. Le
constructeur de copie d'une entrée : `FUN_140A95160` ; son destructeur : `FUN_140A95570`.

**Le choix affiché** (`FUN_140AB50A0`, `_Do_call` du rafraîchisseur que `FUN_140AB1C20` construit, vtable `1428C7FA8`) : il
refait la liste, appelle le lecteur et prend la première entrée dont l'octet vaut l'octet rendu (comparaison d'octets) ; donc un
lecteur qui rend 2 désigne une entrée de valeur 2. La ligne elle-même (`FUN_140AB4C30`, vtable `1428C7F68`) ouvre son choix par
`FUN_140AB3A70` avec la liste, la valeur lue et l'écrivain : l'écrivain est appelé à chaque choix (INFÉRÉ : par le travail du
choix).

### 1.2 L'application du mode par la fenêtre (OBSERVÉ, `FUN_140EF5590`, chaque image)

`CSWindowImp` (`[14489F118]`) garde la config appliquée à `+0x68` (résolutions fenêtrée `+0x68` et plein écran `+0x70`, mode
`+0x78`, puis Auto-detect `+0x7C`, préréglage `+0x80` et les onze niveaux `+0x84..+0xAC`) et la demandée à `+0xB0` (mêmes
champs, mode `+0xC0`, niveaux `+0xCC..+0xF4`). Chaque image : `FUN_140EF7380` compare, `FUN_140EF6760` recopie la demandée
dans l'appliquée ; puis si `FUN_140EF1C30(état +0x38)` (plein écran ou sans bordures) diffère de `+0x78 == 1`, le mode de la
fenêtre devient `+0x78 == 1 ? 1 + (+0x40 != 0) : 0` (`FUN_140EF1C50`), et `FUN_140EF61A0` le réapplique ; la demande d'affichage
suit par `FUN_140EF54A0` (le natif la met en file, `QueueNativeDisplayRequest`, qui lit `+0x40`).

Conséquence : passer de Plein écran à Sans bordures (ou l'inverse) ne change pas `+0x78` ; la fenêtre du jeu ne le voit pas.
Le natif sait déjà le faire (`MarkNativeBorderless`, `StartNativeBorderless`, I-0076) : `+0x40`, `FUN_140EF1C50(état, 2 ou 1)`
et une demande d'affichage native.

### 1.3 Le dessin retenu (le premier ; remplacé par le §6 : la liste, le lecteur et l'écrivain de Screen Mode ne sont plus détournés)

- **La liste** : un détour de `FUN_140A9A220` appelle l'original, puis réordonne en Plein écran (1), Sans bordures (2), Fenêtré (0)
  (Thomas, 18:23 : « Plein Écran > Sans Bordures > Fenêtré ») : l'entrée Fenêtré copiée (`FUN_140A95160`) puis détruite en place
  (`FUN_140A95570`), le nombre ramené à 1, l'entrée Sans bordures faite comme l'original fait les siennes (texte d'un id réservé),
  puis la copie de Fenêtré ajoutée.
- **Le lecteur** (détour de `FUN_140AB4AD0`) : `+0x78 == 1 ? (+0x40 ? 2 : 1) : 0`.
- **L'écrivain** (détour de `140AB5140`) : 0 → `+0xC0 = 0` ; 1 ou 2 → `+0x40` posé à (v == 2), `+0xC0 = 1` ; si le mode appliqué
  est déjà le plein écran et que `+0x40` change, la bascule native (§1.2) ; le choix enregistré dans notre fichier de réglages
  (`borderless`, R12b), `GraphicsConfig.xml` gardant `FULLSCREEN` (le jeu n'a que deux jetons).
- **Le texte** : un détour de `FUN_140E1A060(dépôt, id)` (le lecteur de textes du menu que `FUN_140A67060` passe à
  `FUN_140A672E0`, qui rend un `const wchar_t*`) rend nos chaînes pour nos ids, dans la langue du jeu (`[14479BA10]` : 2 fraFR,
  sinon l'anglais). Ids réservés : 729000 à 729999 et leurs aides 829000 à 829999 (le libellé d'une ligne lit son aide à id +
  100000, `FUN_140AB6E50`) ; aucun n'existe dans les FMG `menu` engUS et fraFR (OBSERVÉ, `fmgdump.py`).

## 2. V-sync et la fréquence d'images (R12c bis ; retirés de la page du jeu au §5, ils sont dans notre page, §6)

- **La place** : après Resolution. Le libellé d'Auto-detect `FUN_140AABD50` n'est appelé que par `FUN_140AB07D0` (OBSERVÉ), juste
  après la fin de la ligne Resolution ; nos deux lignes sont ajoutées dans son détour, avant l'original, pendant un appel de
  `FUN_140AB07D0` (le détour de celle-ci donne la fenêtre).
- **La forme** : la ligne à foncteurs `FUN_140AB1C20`, avec nos propres impls de `std::function` à l'ABI du jeu (§1.1 : sept
  entrées, sans état, toujours en ligne) ; leurs listes faites comme au §1.3 ; le libellé par `FUN_140AB6E50(sortie, id)`.
- **La prise d'effet** : V-sync (`SetPresentSync`, `SetPresentTearing`) et le plafond d'images (`SetPresentUnpaced`, les
  corrections de logique installées au démarrage, `22_setup.inc`) ne se posent qu'au démarrage du rendu (le rendu refuse
  après l'ouverture) : les deux lignes enregistrent dans notre fichier, et leur aide dit « au prochain lancement ».

## 3. La porte de R12c (lecture statique)

- **Le constructeur** `FUN_140BE3150(sortie, ctx)` rend un travail compté par références (`+8` le compteur ; `FUN_1417A35D0`
  ajoute, `FUN_1417A3610` retire et détruit par la vtable `[0]` à zéro) fait par `FUN_140BE3CD0` → `FUN_140BB5E20` (la fenêtre
  `02_042`, foncteur `FUN_140BE3270`). Ses deux appelants : `FUN_140BE2F80` (le répartiteur par nom, `PcGraphicsDetailSetting`)
  et `FUN_140AA8750` (le bouton Advanced Settings) (OBSERVÉ).
- **La chaîne du bouton** (`FUN_140AA8750`) : A = travail de foncteur `1428C7CE8` (`FUN_14059FB70(…, fenêtre+0x10F0)` puis le
  rappel `fenêtre+0x10E8` avec 1), B = `FUN_140BB0250(FUN_140BE3150(…))`, C = travail de foncteur `1428C7CA8` (le même rappel avec
  0) ; mis en suite par `FUN_140BAFE10` et poussés dans la file de la fenêtre (`FUN_140A4C560(fenêtre+0x10, …)`).
- **Un travail neutre du jeu** : `FUN_140BAF080(sortie, param)` fait un travail de foncteur vide (`FUN_140545FA0`, impl
  `142905A38`) ; un travail de foncteur (`FUN_140545400`, vtable `1427A7C90` : `[0]`, `[1]` destructeur, `[2]` sa mise à jour
  `FUN_140546E00` qui appelle le foncteur une fois) se termine à sa première mise à jour (INFÉRÉ).
- **Le dessin retenu pour la porte** : le détour de `FUN_140BE3150` ouvre notre page et rend ce travail neutre ; tant que notre
  page est ouverte, `FUN_140A5B930` (la porte d'entrée de tous les menus Scaleform, ses treize sites) répond « non » ; la page
  Graphics du jeu reste donc sourde sous la nôtre. OUVERT, pour le run du §5.4 : ce que font A et C (le rappel `+0x10E8` cache-t-il
  la page Graphics ?) et un travail qui durerait tant que notre page est ouverte (les états de fin d'un travail ne sont pas lus).
- **Les cinq valeurs de DS3** : elles vivent dans `CSWindowImp` ; on écrit la config demandée (`+0xCC` Texture, `+0xD8`
  DepthOfField, `+0xDC` MotionBlur, `+0xE8` EffectsQuality, `+0xF0` WaterSurfaceQuality) et la fenêtre l'applique à l'image
  suivante (§1.2). Que les niveaux lus par le rendu (`[144796298]+0xA8`, `ReadNativeOptionLevels`) suivent la config de la
  fenêtre passe par la copie config → bloc, chiffrée au repos (MENUS-MAP §77.5) : INFÉRÉ, à voir au run du §5.4 (écrire un niveau
  dans `+0xB0..`, lire `+0xA8` deux images plus tard). L'enregistrement : `FUN_140EF5AB0(fenêtre)` écrit `%APPDATA%/DarkSoulsIII/GraphicsConfig.xml` depuis
  `fenêtre+0x68` par `FUN_140EF75A0` (clés ScreenMode, les quatre résolutions, Auto-detect, QualitySetting, puis les onze niveaux,
  dans l'ordre du fichier) ; le jeu l'appelle depuis `FUN_1406291C0` (l'enregistrement de la partie, INFÉRÉ). Notre page
  l'appelle sur le thread du jeu, une image après l'écriture (que `+0x68` ait reçu la demande).
## 4. Les actions et les sons des menus (pour la page de R)

Relevé d'un sous-agent `ghidra-research` (GhidraMCP, sans écriture), les points dont dépend le dessin relus par le pilote
(`FUN_140EBB520`, le petit relais de 0xF à `140EB6EE0`, `FUN_140A67CA0`).

**La requête** (OBSERVÉ) : `FUN_140EBB520(dispositif, id)` est faux si la liaison `FUN_14233DE40(dispositif, id)` n'est pas
enfoncée ; sinon, si l'id est dans l'ensemble `dispositif+0xA8` (les ids « à front », enregistrés par `FUN_140EBB6E0` :
0x13, 0x10, 0x3C, 0x3D, 0x44, 0x45, 0x41, 0x26 à 0x29, 0x55, 0x56, 0x52, 0x59 à 0x5F), vrai seulement à l'image où il s'enfonce ;
les autres ids sont vrais tant qu'ils sont enfoncés. `FUN_140EBB510(dispositif, 0x13)` est l'impulsion répétée de 0x13 (0,2 s puis
1/15 s, `FUN_140EBB5B0`). Les enveloppes des menus (`FUN_140A5B9D0..FUN_140A5C3F0`) passent toutes par la porte `FUN_140A5B930`.

| Id | Sorte | Rôle (INFÉRÉ par leurs appelants, sauf mention) |
|---|---|---|
| 0x26, 0x27, 0x28, 0x29 | front | haut, bas, gauche, droite de la croix (`FUN_140A5B4F0`, OBSERVÉ) |
| 0x2C, 0x2D, 0x2E, 0x2F | maintenu | haut, bas, gauche, droite du stick (`FUN_140A5B610`, OBSERVÉ) |
| 0x0F | maintenu | valider (clavier, manette) : seul, il déclenche le rappel d'un bouton (`FUN_140A8F550`, `FUN_140A9FA30`) |
| 0x13 | front, et répété | le clic : avec un test du rectangle sous la souris (flèches d'un pas, lignes) |
| 0x10 | front | annuler, retour (même code de résultat que l'annulation dans la page des touches) |
| 0x4A, 0x4B | maintenu | onglet précédent, suivant |
| 0x5E, 0x5F | front | plus et moins d'un pas, et la molette (`FUN_140A5C3F0`, `FUN_140A5C3D0` → `FUN_140A60970`) |
| 0x42/0x43, 0x48/0x49 | maintenu | grands pas d'un curseur (avec et sans bouclage, `FUN_140A53FE0`) |

**Les sons** (OBSERVÉ pour le chemin) : `FUN_140A67CA0(requête)`, requête {id, priorité, canal, étiquette} ; le canal 0x29 est la
file de l'UI (un son par image, la priorité la plus haute gagne, le même id deux images de suite tu ; `FUN_140A67BD0` la vide),
qui joue `f<id>` (`FUN_1404AD510(…, 2, id, -1)` fait le nom : la lettre `"acfopsmvxbgyze"[2]` puis neuf chiffres). Ids lus dans
le code des pages : 700 priorité 0 quand la valeur d'un pas change (`FUN_140AC8860`, `FUN_140A53FE0`, `FUN_140A53970`) ; 100
priorité 2 à la validation d'un bouton (`FUN_140AB4D70`, `FUN_140AA71A0`) ; 200 priorité 2 (ou 700) au retour d'une boîte
(`FUN_140ABB250`) ; 400 priorité 5 à une case basculée (`FUN_140ACD0A0`). Le son du déplacement du curseur et celui de
l'annulation ne sont pas dans ce code (INFÉRÉ : joués par le film Scaleform) : à relever au run du §5.4 (une trace de
`FUN_1404AD510` en naviguant dans la page du jeu). Le §3.4 de l'étude (100 curseur, 200 validation, 300 valeur changée) est donc
à corriger par ce run.

## 5. Les emplacements de la page Graphics (OBSERVÉ au run `r12cb-graphics1`, puis dans le film)

Le premier run (2026-11-10 20:10-20:13, ASUS, `build-r12cb3`, `-OptionsTour`) a montré nos deux lignes posées par le code : la
fréquence d'images s'affichait bien (« Maximum frame rate : 60 (Dark Souls III) »), mais V-sync s'affichait comme un bouton sans
libellé, et Advanced Settings avait disparu. La raison est dans le film `menu/win/02_040_optionsetting.gfx` (lu par un petit lecteur
de `PlaceObject` dans le scratchpad) : la fenêtre `PCGraphic` (sprite 140) n'a que **six emplacements**, chacun d'un type fixe :
`Item_0_0`, `Item_1_0` et `Item_3_0` sont du type 139 (un libellé, une valeur, une marque : les lignes à foncteurs
`FUN_140AB1C20` et `FUN_140AB18C0`), `Item_2_0` et `Item_4_0` du type 131 (bouton avec marque, saisie, liste, curseur : la ligne
de choix `FUN_140AA40F0`, et le bouton d'Advanced Settings), `Item_5_0` du type générique 92. Le jeu en remplit cinq (Screen Mode,
Resolution, Auto-detect, Quality Setting, Advanced Settings). Une ligne posée dans un emplacement d'un autre type prend le dessin de
cet emplacement ; au-delà du sixième, une ligne n'a pas d'emplacement. Les autres sous-pages ont aussi leurs emplacements comptés
(`ControllSetting` six, `CameraSetting` sept, `PCMouseKey` cinq).

Conséquence : sans modifier le film, la page Graphics ne reçoit qu'une ligne de plus (dans `Item_5_0`, après Advanced Settings).
Les lignes V-sync et fréquence d'images sont retirées du code (`build-r12cb4`) ; leur place est la décision de Thomas (A : la page
Rekindled ; B : V-sync en sixième ligne ; C : Auto-detect retiré pour les deux). Thomas a ensuite choisi de recréer la page (§6).

## 6. Notre page Graphics (Thomas, 2026-11-10 20:30 à 20:40) et l'entrée de l'onglet

**La décision** (après la capture du run `r12cb-graphics1`) : « on recrée la page Graphics pour y ajouter Vsync et le framerate en
plus de l'option Borderless », le reste dans Advanced ; Advanced Settings remplace Quality Setting, et c'est le même bouton qui
mène à la page de R ; le préréglage Low/Medium/High/Max reste, en tête de la page avancée (« Oui laisse », 20:37) ; Auto-detect
retiré et gardé OFF (20:40), comme décidé le 31 octobre (feuillet G des options, §7a). Notre page Graphics : Screen Mode (Plein
écran > Sans bordures > Fenêtré), Resolution, V-sync, Fréquence d'images, puis le bouton Advanced.

**L'entrée** : l'onglet Graphics du jeu, quand la porte est armée, ne pose plus qu'une ligne. Le premier essai (`build-r12cb6`, run
`r12cb-door1`, 21:05 à 21:08, porte prouvée : ouverte, demande prise, refermée 180 images plus tard) posait le bouton Advanced
Settings du jeu (`FUN_140A8F920`) : dans le premier emplacement, du type « valeur », il montrait le texte provisoire du film
(« 設定（仮） »), et l'onglet ajoutait sa ligne Defaults. D'où le second dessin (`build-r12cb7`) :

- une ligne de valeur comme Screen Mode (`FUN_140AB1C20`, nos trois fonctions à l'ABI du jeu), « Graphics settings / Paramètres
  graphiques », dont la valeur est le mode et la résolution du moment (« Full-screen, 1920x1080 », id 729002 bâti à la volée) ;
- son appui : `FUN_140AB4C30` (le `_Do_call` des lignes de valeur : `add rcx, 8 ; jmp FUN_140AA7900`, qui ouvre leur liste de
  choix) est détourné ; une ligne dont la fonction de liste (`+0xC0`, son impl à `+0xD8`) est la nôtre ouvre notre page ;
- la ligne Defaults : un patch de `140AB3857` (le répartiteur des onglets y range `FUN_140AAEC10`, qui pose une ligne de texte
  40330) n'a pas retiré la ligne Defaults au run `r12cb-door2` (21:32 à 21:36 : l'entrée affichait « Windowed, 1280x720 », son
  appui a ouvert la porte, mais Defaults restait dans le deuxième emplacement avec le texte provisoire) ; le patch est retiré
  (`build-r12cb8`). À la place, **deux entrées** : « Graphics settings » et « Advanced Settings » (720104 du jeu, le préréglage en
  valeur, 722020 à 722024), qui ouvrent la page à sa section (`RekindledDoorSection`) ; Defaults tombe alors dans le troisième
  emplacement, du type bouton, comme dans la page d'origine (OBSERVÉ au run `r12cb-door3`).

**Les sons relevés** (`sound-observe.flag`, run `r12cb-door1`, catégorie 2, `f<id>`) : tôt dans le run 1010, 100, 1000, 1020 (INFÉRÉ de leur ordre : les écrans du titre) ;
chaque changement d'onglet 500 ; 30000 et 30100 autour de la fenêtre d'options ; 0 et 999999999 (aucun son). À relire avec un run
qui navigue dans les lignes (curseur, valeur changée, annulation).

## 7. Preuves complémentaires (revue d'étape, OBSERVÉ dans GhidraMCP)

- **La ligne de valeur et son libellé** : `FUN_140AB07D0` réutilise le même tampon de libellé d'une ligne à l'autre et ne le
  détruit qu'à la fin (`FUN_140A49F80`) ; `FUN_140AB1C20` le reçoit donc par adresse, sans le consommer, et notre appelant le
  détruit une fois. Le libellé : un texte de 0x38 octets (un qword, une chaîne de 0x30) puis l'aide à `+0x38` ; `FUN_140AB6E50(sortie, id)`
  lit l'aide à id + 100000.
- **L'appui d'une ligne de valeur** : `FUN_140AB4C30` = `add rcx, 8 ; jmp FUN_140AA7900` ; le prologue de `FUN_140AA7900` ne lit que
  `rcx` (`rdx` et `r8` sont écrits avant toute lecture : `lea edx, [r12+8]`, `mov r8, [14479C268]`). Le détour transmet quand même les
  quatre registres.
- **Les listes de résolutions** : `FUN_140EF9090(liste, fenêtre, plein écran)` construit le vecteur elle-même (`[0]`, `[1]`, `[2]` mis à
  zéro, `[3]` l'allocateur `[14479C258]`) et y pousse des paires {largeur, hauteur} de 8 octets (`FUN_140EF9CF0` ; les modes DXGI de
  28 octets n'en sont que la source) ; l'appelant libère par `allocateur->vtable à +0x68 (allocateur, début)` (`FUN_140B07A10`).
  `FUN_1423575E0(fenêtre)` rend la poignée de la fenêtre (`ShowWindow` dans `FUN_140EF5590`).
- **Le texte vide** `143D1DA80` : huit octets nuls (`L""`), passé par `FUN_140A9A220` à `FUN_14007E060`.
- **Le travail neutre** `FUN_140BAF080(sortie, 2)` : la même valeur que `FUN_140AA8750` passe à `FUN_140545FA0` (`FUN_140BB18C0(&v, 2)`) ;
  son sens n'est pas lu (INFÉRÉ : une priorité ou une sorte). Ce chemin (l'Advanced Settings du jeu par `FUN_140BE3150`) n'a pas
  encore servi en run : l'onglet armé n'a plus le bouton du jeu ; il reste le répartiteur par nom.
- **La table des préréglages** : `ApplyGraphicsPreset_140EF7340` (nom du registre D-0269) commence par écrire les défauts (1920x1080,
  plein écran, Auto-detect 1, préréglage 3), puis applique le préréglage qu'il vient d'écrire : il ne sait pas appliquer un autre
  préréglage, d'où la table recopiée de ses quatre branches (`kPresetLevels`).

## 8. Les pages du jeu comme base (D-0271 ; lecture statique du fil R, 2026-11-11, GhidraMCP sur DS3 1.15.2, relue par le pilote)

Thomas (2026-11-10 23:02, « Vous avez pas accès à la page originale dans le code? Puis la copier comme base », puis « Vas y » à 23:06) :
nos lignes vont dans l'onglet Graphics du jeu et dans sa page Advanced Settings, avec les fonctions de ligne du jeu ; l'overlay natif
(R12a) sert à l'écran de précompilation et à l'écran titre.

### 8.1 L'onglet Graphics (`AddPCGraphicRows_140AB07D0`, OBSERVÉ)

- Les foncteurs de Screen Mode et de Resolution sont sans état : un seul mot, la vtable, l'impl sur lui-même. `FUN_140AB1C20(fenêtre,
  libellé, a3, a4, a5)` reçoit dans l'ordre **lire, écrire, la liste** (Screen Mode : `1428C7A28` lire, `do_call FUN_140AB4AD0` le plein
  écran ; `1428C7A68` écrire ; `1428C7AA8` la liste, `do_call FUN_140AB4AF0` → `FUN_140AA46C0`). Le §6.1 de l'étude nommait `1428C7A68`
  la liste : c'est l'écriture. Resolution : `1428C7AE8` lire, `1428C7B28` écrire, `1428C7B68` la liste ; on les repose tels quels.
- **Auto-detect** : `AddChoiceRow_140AA40F0(fenêtre, libellé, octet, choix, défaut*, actif)` sur `état+0x63`, les choix On/Off de
  `FUN_140AAB920` (deux entrées de 0x78 octets, le compte à `+0xF8`, 40300 et 40301), détruites par `FUN_140AA6360`. Le 6e argument est
  « actif » (1 ici) : `FUN_140AA3FA0` passe `!actif` au constructeur et range `actif` à `+0x78` de l'entrée. La V-sync prend cette ligne.
- **Le préréglage** (`FUN_140AB18C0`) : ses foncteurs capturent `état+0x63` ; l'écriture (`1428C7BE8`, `do_call FUN_140AB49B0`) range
  la valeur à `état+0x64` puis appelle la vfunc `0x90` de la fenêtre ; la lecture (`1428C7BA8`, `FUN_140AB4BE0`) rend `état+0x64`.
- **Advanced Settings** : `AddButtonRow_140A8F920(fenêtre, libellé 720104, foncteur, fonction vide)`, le foncteur `1428C7C68` à deux
  mots (la vtable, la fenêtre) ; son appui est la chaîne de `FUN_140AA8750` (le §3), qui crée la sous-page par
  `GraphicOptionPageFunctor_140BE3270` → `CreateOptionSubPageByName_140AB34B0(…, L"GraphicOption", AdvancedSettingsPage_BuildRows,
  la ligne Defaults)`.
- Les libellés : `FUN_140AABDD0` 720100, `FUN_140AABD90` 720101, `FUN_140AABD50` 720102, `FUN_140AABCD0` 720103, `FUN_140AABC90` 720104,
  tous `FUN_140AB6E50(sortie, id)` (lus dans les octets des fonctions).

### 8.2 La page Advanced Settings (`AdvancedSettingsPage_BuildRows_140AAFD10(page, état)`, OBSERVÉ)

- La sous-page (0x1168 octets, `CreateOptionSubPageByName`) range son état à `page+0x10F0` et appelle le constructeur des lignes avec
  `(page, page+0x10F0)`, puis la ligne Defaults (`OpenRestoreDefaultsBox_140AAE950`, un bouton), puis `FUN_140A913B0(page, 0)` qui pose
  les lignes dans les emplacements (`page+0x9D8`, la liste des lignes `page+0x1070`, des entrées de 0x80 octets).
- Onze lignes, chacune `AddRowWithLabelAndChoices_140AA4210(page, libellé, état+0x65+i, choix, &défauts[i])`, défauts de
  `GraphicsSettingsFillDefaults_140C20E20`. L'ordre et les octets : `+0x65` Texture (721100, `FUN_140A99B50`), `+0x66` Antialiasing
  (721101, `FUN_140A98F30`), `+0x67` SSAO (721102, `FUN_140A99600`), `+0x68` Depth of field (721103, `FUN_140A99310`), `+0x69` Motion
  blur (721104, `FUN_140A990A0`), `+0x6A` Shadow (721105, `FUN_140A99310`), `+0x6B` Lighting (721106, `FUN_140A99B50`), `+0x6C` Effects
  (721107, `FUN_140A99B50`), `+0x6D` Reflection (721109, `FUN_140A99960`), `+0x6E` Water surface (721110, `FUN_140A997F0`), `+0x6F`
  Shader (721111, `FUN_140A99B50`). Les listes sont celles de `AppendChoice` (entrées de 0x78 octets, le compte à `+0xF08`,
  `FUN_140A9B560`), détruites entrée par entrée par `FUN_140A95570`.
- `AddRowWithLabelAndChoices` appelle `FUN_140AA3D30(page, libellé, octet, choix, défaut*, foncteur {1428C83E8, page}, fonction vide,
  actif = 1)`. Le foncteur `1428C83E8` (`do_call` à `140AB4C40`, non défini dans Ghidra, lu dans ses octets) fait `page+0x1154 = 5`
  puis la vfunc `0x90` de la page, `FUN_140AB69A0`, qui appelle la fonction rangée à `page+0x10A8` avec l'état (`page+0x10F0`). Ce que
  fait cette fonction n'est pas lu ; le premier run (r12g-pages1) l'a vue appliquer l'état à la config tout de suite (la texture en direct).
- Le constructeur de la ligne (`FUN_140A9BE10` par `FUN_140AA36E0`) lit son défaut comme un `int` de 4 octets.

### 8.3 L'état grisé (OBSERVÉ)

- Le film `02_042_pc_graphicsetting.gfx` (lu par un petit lecteur de `PlaceObject` dans le scratchpad) a **quatorze emplacements**
  `Item_0_0` à `Item_13_0`, tous du sprite 81 (ses `Widgets`, le sprite 68 : `Item_0_0`, `GrayoutItem`, `Caption`, `HitLeftArrow`,
  `HitRightArrow`, `Cursor`), et une barre `ScrollBarV`.
- Les deux constructeurs de ligne (`FUN_140A9BE10`, `FUN_140AA5220`) cherchent `Item_0_0` (`1428C3A68`) et `GrayoutItem`
  (`1428C3AC0`) ; ligne inactive (`actif = 0`) : `Item_0_0` caché (`ShowHideWindow_140A3D720(…, 0)`), le texte posé dans
  `GrayoutItem` (`FUN_140A9E660`), et ni foncteur de valeur ni son de pas : grisée et verrouillée. Ligne active : `GrayoutItem` caché.
- L'état grisé est fixé à la construction : une ligne ne change pas d'état sans que la page soit reconstruite. Le chemin qui
  reconstruit les lignes d'une page ouverte est au §8.4.
### 8.4 Reconstruire les lignes d'une page ouverte (OBSERVÉ ; vu en run, `r12g-pages2`)

- **La mise à jour des fenêtres** `FUN_140A4CA90(fenêtre, dt dans XMM1, entrées, …)` (son prologue range XMM1 dans XMM6) traite les
  entrées puis appelle la vfunc `0x48` de la fenêtre : à son entrée pour notre page, aucune ligne de la page n'est sur la pile.
- **La liste des lignes** (`page+0x1070`, entrées de 0x80 octets, la ligne comptée par références à `+0x70`) se vide comme le
  destructeur de la page la vide (`FUN_140AA6F00` → `FUN_140A90390(liste, début, fin)` : chaque ligne relâchée, son libellé détruit) ;
  une ligne prend l'emplacement `Item_<taille de la liste>_0` (`FUN_140A90E20`, le format `1428C0C38`) ; `FUN_140A913B0(page, curseur)`
  lie la liste aux emplacements (`FUN_140A8BDA0` : le nombre d'éléments, le curseur porté sur la ligne choisissable la plus proche) ;
  le curseur se lit à `page+0x9D8+0xB4` (`FUN_140A42AD0`).
- **Une ligne grisée** écrit son texte dans `Widgets/ComboBox/GrayoutItem` (`1428C0A90`, `1428C3AC0`) sans le montrer ; une ligne
  active le cache. Après une reconstruction, l'emplacement d'une ligne devenue grisée le montre donc nous-mêmes
  (`ShowHideWindow_140A3D720(…, 1)`), chaque poignée relâchée par `FUN_140E63410` sur son `+0x28`.
- La vtable de la sous-page est `1428C7558` (`CreateOptionSubPageByName_140AB34B0`) ; sa vfunc `0x90` est `FUN_140AB69A0`.
- **Après une reconstruction** (vu en run) : les fabriques de ligne cachent les quatre autres widgets de l'emplacement sans montrer le
  leur ; chaque ligne montre donc le sien (le ComboBox, ou le curseur) ; la page se reconnaît aussi à sa liste de lignes (son début
  noté à chaque construction). Les fautes d'abord prêtées à la reconstruction (`FUN_140E60346` sous `FUN_140A4CA90`) étaient l'arrêt
  des runs bornés, menu ouvert (la RCA, RESULT « R12c ») : la reconstruction a été survécue.

### 8.5 La ligne à curseur (OBSERVÉ)

- Les fabriques de ligne posent chacune un des cinq widgets de l'emplacement et cachent les quatre autres (`Widgets/Slider`,
  `Widgets/ComboBox`, `Widgets/DropdownList`, `Widgets/TextInput`, `Widgets/Button`, `1428C0A80` à `1428C0AD8`) : la liste de choix
  `FUN_140AA36E0` et `FUN_140A9B610` (ComboBox), le bouton `FUN_140A8E8A0`, la saisie `FUN_140A9F250`, le curseur `FUN_140AA3AC0`
  (constructeur `FUN_140AA5940`) et `FUN_140A8BBD0` (`FUN_140A8C1F0`).
- **La ligne à curseur** `FUN_140AA43C0(page, libellé, octet de la valeur, plage, défaut*)`, la plage `{int min, int max, int pas, un
  octet}` (`FUN_140AAC6A0` : `{0, 10, 1, 0}` sur `état+2` ; c'est la page « BrightnessSetting », la luminosité du jeu, et non la
  vitesse de caméra comme écrit d'abord ici : le répartiteur des onglets à `140AB3820` la range avec ce nom, relu le 2026-11-14
  pour I-0138 tranche 5b) ; son rappel de changement est le foncteur
  `1428C8368` du jeu (`{vtable, page}`, son `do_call` à `140AB4AC0` n'appelle que la vfunc `0x90` de la page) : pas de foncteur à
  nous, d'où l'octet du curseur surveillé à la mise à jour de la page. Elle n'a pas d'argument « actif » : un curseur indisponible
  se pose en ligne de choix grisée.