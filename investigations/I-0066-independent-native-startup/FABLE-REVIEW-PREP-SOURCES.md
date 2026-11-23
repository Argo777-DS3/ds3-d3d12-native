# Préparation de la revue de contrôle finale de la Vague 1 (Fable 5.1, effort élevé)

Document de préparation, statique et en lecture seule : aucun jeu lancé, aucun test live, aucun changement de code.
Écrit le 2026-10-18 (heure de Paris, 05 h 20) sur la branche `claude/fable-review-prep`, depuis `main` 4880ef8c
(« Workflow: clangd for the native C++ », 2026-10-18). Il sert à ce que le relecteur **vérifie au lieu de chercher** :
chaque ligne porte sa source et, quand il existe, son témoin (numéro de run Native/Reference, capture, zone).

## Comment lire ce document

| Partie | Contenu |
|---|---|
| 1 | Tous les écarts ouverts, avec source et témoin : 1.A rendu, effets, éclairage (R-01 à R-57) ; 1.B menus, interface, options, langues, films, audio, entrées, affichage, pacing (M-01 à M-56) ; 1.C constats de conception du code natif (C-01 à C-10, détail en partie 3) ; 1.D décisions assumées qui ressemblent à des écarts |
| 2 | Chaque paramètre des options du jeu et du menu principal de l'écran titre, avec comment le tester |
| 3 | Les fichiers et zones du code natif à relire en priorité, en particulier là où le code recopie les opérations D3D11 une à une (philosophie : **même rendu, conception D3D12 propre**) |
| 4 | EN SUSPENS : ce que le pilote « Tests des menus en langues » est en train de tester, marqué **inconnu** |
| 5 | Contradictions entre sources à trancher, et ce que ce document n'a pas pu établir |
| 6 | Ordre de relecture proposé |

**Statuts.** Ce sont les mots des sources, jamais promus : CONFIRMÉ, SUPPORTÉ, OBSERVÉ, INFÉRÉ, OUVERT, ABSENT ; « non documenté »
quand le dépôt ne dit rien. Une ligne « (code) » est un constat fait en lisant le code, qu'aucun document ne date.
**Zones des témoins** : m30 Haut-Mur de Lothric (sauvegarde Shebang Theta), m31 Village des morts-vivants, m37 Irithyll,
m40 Sanctuaire de Lige-Feu (Shebang Eta), m45 Ariandel, m50 Capharnaüm des Confins (DLC2), m51 Cité Enclavée.

**Faits récents qui ne sont pas encore dans le dépôt** (messages du fil « Tests des menus en langues » relayés par le coordinateur,
2026-10-18, 03 h 03 à 03 h 08 UTC) : la silhouette d'Albert est un écart OUVERT que le pilote n'a pas su corriger ; le russe est
identique à l'original (Reference2073 / Native2074) ; les fondus sont jugés « rien à dessiner, comme l'original »
(Native2066-2068, 2073). Ils sont marqués « Coord. 18/10 » : à confirmer dans STATE quand le pilote le consigne.

**Pièges de lecture** : `STATE.md` renvoie à `MENU-TESTS-PREP`, qui n'est **pas dans `main`** (branche
`claude/relaxed-almeida-957f40`, commits 4641379e et c6363d1d) ; le ledger ne porte aucune revendication sur le backend natif du jeu
dans les périmètres des parties 1.B (hors K-0441/K-0442 historiques) ; les mesures nommées « Native NNNN / Reference NNNN » sont des
runs sous `evidence/I-0066/artifacts/` (dossier externe `C:/Users/User/Desktop/DS3_evidences`, jamais à restaurer automatiquement).

## 1. Écarts ouverts

### 1.A Rendu, effets, éclairage (57 écarts, R-01 à R-57)

Tableau dressé par relecture des sources (STATE, WAVE1-LAST-EFFECTS, FFX-DRAW, SUN-SHADOWS-WAVE1, IMAGE-REGRESSION-STATIC-LEADS, W10-NATIVE-PASSES, ready-sheets, ledger). Abréviations : `I66/` = `investigations/I-0066-independent-native-startup/`, `I68/` = `investigations/I-0068-static-yebis-ffx-lights-spec/`, `GB` = `native/frame/game_bootstrap.cpp`. « (code) » = constaté dans le code. Les ID R-40 à R-45 concernent les phantoms et les PNJ, dont Albert (R-40).

#### 1.A.1 Éclairage, lightmaps, ombres

| ID | Écart | Zone / témoin | Source | Statut | Cause connue ou hypothèse | Quoi vérifier |
|---|---|---|---|---|---|---|
| R-01 | Les roues de m31 restent plus claires en natif. | m31 ; Native1860, Native1912-1917 contre Reference1796-1806, `effects-shots/<zone>-wave1.jpg` | I66/WAVE1-LAST-EFFECTS §1 l.36-37, §7 l.155-158 ; STATE l.16 (« Residuals ») | OUVERT | Cause non identifiée : ce ne sont pas des entités lightmappées, aucun jeu DOL vu ; SUN-SHADOWS §7 l.207-208 l'attribue à une ombre précalculée (INFÉRÉ). | Le jeu de lightmaps des roues ou de leur pièce dans l'original (`reference_part_mesh_draw` / DOL) ; comparer côte à côte m31. |
| R-02 | La chapelle de m37 est plus claire en natif. | m37 ; Native1885, Native1915 contre Reference1796-1806 | I66/WAVE1-LAST-EFFECTS §7 l.157 ; STATE l.16 | OUVERT | non documenté | Mesurer le HDR éclairé de la région de la chapelle (`Compare-SceneHdr.ps1`) contre la capture de référence de m37. |
| R-03 | Le mur de m40 est à 1,23 fois l'original dans le HDR éclairé. | m40 ; Native1737/1750 contre Reference1705 | I66/SUN-SHADOWS-WAVE1 §3 l.72-74 et tableau l.91 ; STATE l.17 (« Open: … the m40 wall 1.23 ») | OUVERT | non documenté ; aucune mesure de cette région après les correctifs lightmaps et Dominant n'est documentée. | Relancer `Compare-SceneHdr.ps1` à m40, image 2400, avec les défauts actuels. |
| R-04 | L'éclairage des personnages reste à 0,84-0,86 de l'original : les rangées SH ambiantes d'instance et les sondes de lumière ne sont pas produites. | m30, m37 ; Native1857 (lightmaps) ; Native1710 `native_instance_sh` = 0 | I66/WAVE1-LAST-EFFECTS §1 l.37-38 ; I66/IMAGE-REGRESSION-STATIC-LEADS L2 l.98, l.109-110 ; I68/REMAINING-FAMILIES §1 ; GB l.12801-12815 (`NativeObjectProbeLightBlend` : horloge seule) | OUVERT (« known instance SH gap ») | La banque 32A80 ne reçoit aucun fichier en natif, et le mélange de sondes 1AC840 est volontairement omis tant qu'il n'existe pas de propriétaire natif des sondes. | Lignes `native_instance_sh` / `native_instance_probe_inputs` contre les rangées +0x290 de l'original ; région « character » de `Compare-SceneHdr`. |
| R-05 | Les consommateurs de banques moteur non migrés lisent encore le défaut : échelle couleur EnvMap de région f[36], `FC_EnvMapColorScale0/1`, LightScattering (aucun lecteur natif), réglages des points lumineux 15E920 (« probablement un défaut »). | toutes ; m40 id 100 EnvMap 0,5 | I66/IMAGE-REGRESSION-STATIC-LEADS L2 tableau l.99-102, L6 l.163 ; GB l.5497 | OUVERT / INFÉRÉ (L6 : impact « négligeable » pour l'EnvMap) | La répartition des fichiers gparam du moteur est remplacée par `BindNativeGparamScene` (§L2). | Ajouter un compteur « défaut rendu par la banque » par (banque, clé) comme le propose L2 l.112-113. |
| R-06 | Le poids de fondu des liaisons gparam est ignoré en natif, ainsi que les poids de fondu enchaîné D26DD0 ; le désassemblage à un changement de variante n'est pas exercé en jeu. | dépend de la progression (Shebang Beta, fin de jeu) | I66/IMAGE-REGRESSION-STATIC-LEADS L3 l.131-132 ; I66/NATIVE-LIGHTING-MAP l.239-249 | OUVERT | Le contrôleur fait varier les poids après la liaison, et le natif ne les lit pas. | Un run sur une zone en transition de variante : lignes `native_gparam_file_unbinding` et soleil `sun0` contre l'original. |
| R-07 | La position de carte du brouillard (traduction de la zone liée) et la règle des couches de zone (la dernière couche qui contient l'ID l'emporte) reposent sur une seule zone observée chacune. | m31 (Reference1743 / Native1744), m45 (Reference1751 / Native1753) | I66/SUN-SHADOWS-WAVE1 §3 l.76-85 ; STATE l.19 (« the fog record's map position ») | INFÉRÉ (« INFERRED rule, one zone observed ») ; témoin ouvert | La règle n'a été vérifiée qu'à m31 (brouillard) et m45 (couches). | `reference_fog_set` / `native_fog_set` sur une autre zone à couches multiples ou en zone de mélange. |
| R-08 | Les jeux de lightmaps RNM ne reçoivent pas leur constante ni leurs textures par défaut (19CA80 +0x38) ; le producteur du bit 13 du paramètre de draw est absent ; une entrée nulle du jeu ne lie pas null. | zones lightmappées (m30, m31, m37) | I66/WAVE1-LAST-EFFECTS §1 l.41-43 | OUVERT (« Not applied ») | Omis à la revue d'étape. | Recenser les draws avec un jeu RNM (P[0..2] en t26..t28) : constante cbTexHdrScaleParam native contre originale. |
| R-09 | Les lanceurs d'ombre solaire natifs comptent quelques pièces de plus que l'original (m37 : 5 pièces de la même catégorie en plus ; boîte de 2 m du personnage). | m37 Native1821, m31 Native1820, contre Reference1818/1819 | I66/SUN-SHADOWS-WAVE1 §7 l.203-207 ; ledger K-0453 | OUVERT (« small ») ; K-0453 SUPPORTÉ sur deux zones | Culling des boîtes de cascade, et boîte du personnage. | Comparer les modèles des rangées de profondeur 63CB0 (Reference) à la liste des lanceurs natifs, zone par zone. |
| R-10 | Le chemin par lequel les lanceurs de l'original atteignent ses cascades n'est pas identifié (aucune admission de passe 142E80/57A20/D04B90 pour les passes 1..4). | m31 Reference1791 | I66/SUN-SHADOWS-WAVE1 §7 l.162-170 | OUVERT | Les enregistrements persistants sont un candidat. | Lecture statique ou observateur du chemin d'admission des cascades. |
| R-11 | L'ombre des pièces de la catégorie 0x02 « forward-only » (0x04) : une entité forward seule ne projette pas d'ombre en natif. | toutes | I66/SUN-SHADOWS-WAVE1 §7 l.222-223 | OUVERT (« by design ») | Non lu dans l'original. | Chercher dans l'original un lanceur de catégorie 0x04 seule dans l'atlas (`cascade-observe.flag`). |
| R-12 | Ce que la catégorie 0x02 ajoute aux passes 5..8 n'est pas lu. | toutes | I66/SUN-SHADOWS-WAVE1 §7 l.196-197 | OUVERT (« not read ») | non documenté | Lecture statique de D03EB0 / C2EC0 pour les passes 5..8. |

#### 1.A.2 Effets FFX

| ID | Écart | Zone / témoin | Source | Statut | Cause connue ou hypothèse | Quoi vérifier |
|---|---|---|---|---|---|---|
| R-13 | E03, les particules modèles : s11600 à Lige-Feu, et l'herbe instanciée de m51, ne sont pas dessinés en natif ; la classe 27EA70 plante sous `ffx-model-particles.flag`. | m40 Reference2037/2038 (invisible à la vue d'Alpha) ; Native2035 (plantage) ; m51 | I66/FFX-DRAW l.443-444, l.449-452, l.486 ; I68/EFFECTS-E03-MODEL-PARTICLES-NATIVE §2 M1-M6 ; STATE l.16 | OUVERT | Il manque une soumission FLVER native en 1400636F0 (M1-M6) ; la faute est une lecture [0+8] en 14A1EE depuis 2822A6. | Vue de m51 avec l'herbe : nombre de draws E03 dans l'original (`reference_ffx_model_draws`) contre 0 en natif. |
| R-14 | Les familles de sorts « sorcellerie » et « miracle » ne sont pas témoignées. | m40 Alpha, Reference2045 | I66/FFX-DRAW l.476-478, l.486 | OUVERT | Le pilote ne sait pas changer l'harmonisation ; seule la Boule de feu a été lancée. | Un pilote d'équipement, puis Great Soul Arrow et un miracle côte à côte. |
| R-15 | Les effets sont dessinés après toutes les rangées forward au lieu d'être triés avec elles par clé. | toutes | I66/FFX-DRAW l.208-211, l.227-228, l.486-487 | OUVERT | Choix d'encodage : un seul bloc d'effets après le forward. | Une scène où un translucide forward chevauche un effet (fumée devant un phantom) : ordre visible. |
| R-16 | Les draws dont l'entrée de profondeur écrit (+0x1E0, type de mélange 0) testent sans écrire (vue de profondeur en lecture seule) ; les passes 0x20/0x21 qui écrivent la profondeur ne sont jamais rencontrées. | toutes | I66/FFX-DRAW l.208-210, l.372-374, l.487 | OUVERT (« never met ») | La DSV de l'étage effets est en lecture seule. | Compter les draws d'effets avec écriture de profondeur dans l'original (`ffx-draw-observe`) aux six zones. |
| R-17 | L'ordre de 0x2D et 0x31 par rapport aux passes intermédiaires ; quand 0x2D et 0x31 échantillonnent tous deux la copie, 0x31 la voit sans la sortie de 0x2D. | m37, m45, m31 (quads de distorsion) | I66/FFX-DRAW l.210-211, l.427-428, l.487-488 | OUVERT / INFÉRÉ (« INFERRED design ») | Conception de `EffectCopyPass` : une seule copie refaite avant la première passe après 0x29. | Une frame où 0x2D et 0x31 échantillonnent la copie : comparer à l'original. |
| R-18 | Les classes de rendu FFX non autorisées sont sautées et comptées : 3D7E380 (3709B0) et les GXVfx 3D659C8, 3D65A88, 3D65AF8, 3D65D38, 3D65DC8, 3D65E38, 3D65EA8, 3D660B8 ; la classe 3D7E4A0 est autorisée mais pas encore rencontrée. | jamais rencontrées aux zones témoins | I66/FFX-DRAW l.479-482, l.490-491 | OUVERT | non documenté (ces classes n'ont pas de témoin) | Le compteur de classes sautées sur un parcours hors des zones témoins. |
| R-19 | Les traînées E02 et le flou radial E05 n'ont aucun témoin côte à côte documenté. | non documenté | I66/FFX-DRAW l.34, l.157, l.373-374 (« E02 and the other families ») ; I68/EFFECTS-E05-E04-NATIVE §6 | OUVERT (E02) ; non documenté (E05) | Les classes E01/E02 tournent (l.157) ; aucune ligne ne nomme un draw E02 ou E05 témoigné. | Le compte de draws E02 (traînées d'arme) et `GXFfxRadialBlur*` original/natif pendant un combat. |
| R-20 | E08, les rayons de lumière d'effet, n'a jamais de témoin : il n'existe que dans les bundles de m30, m33 et m38. | m30 (zone témoin) | I68/WATER-REFLECTION-INTERACTION-E08 §3 ; I68/COVERAGE-MATRIX ligne 37 | OUVERT (« never drawn », sans témoin) | La branche type de draw vers les indices 71–74 n'est pas isolée dans 264230. | `ffx-draw-observe` à m30 : chercher les programmes `GXFfxLightShaft*`. |
| R-21 | Le bloc brouillard b6 des flammes diffère en +0x0C et +0x2C (0,01 en natif, 0 dans l'original). | m40 | I66/FFX-DRAW l.454-455, l.465-466 | OUVERT (mots non lus par le programme de la flamme) | Le programme ne lit pas ces mots. | Recenser les programmes FFX qui lisent cb6[0].w / FC_ExtFogParam.w. |
| R-22 | Un bloc de constantes lié mais jamais mis à jour vaut zéro en natif, alors que son contenu est indéfini dans l'original. | toutes | I66/FFX-DRAW l.238-239 | INFÉRÉ | Les tampons de l'original sont créés sans données sous D3D11. | non documenté (pas de témoin d'un tel bloc lu). |
| R-23 | Les compteurs UAV cachés de E07 sont natifs (compteur de 4 octets par vue), alors que les vues D3D11 portent Flags 0. | m37, m40, m45, m31, m50 (braises) | I66/FFX-DRAW l.320-326 | INFÉRÉ (« the driver's ») | Le comportement du pilote D3D11 est supposé. | Les comptes vivants sont déjà égaux (l.348-350) ; vérifier seulement sur un système à forte capacité, s'il en existe un. |

#### 1.A.3 Particules et matériaux de surface

| ID | Écart | Zone / témoin | Source | Statut | Cause connue ou hypothèse | Quoi vérifier |
|---|---|---|---|---|---|---|
| R-24 | La neige sur les surfaces (`_SnowLt`) est refusée : trois groupes de mots neige n'ont pas de producteur natif. | m31 (variantes `…DOL_SnowLt_*`), m45 Ariandel ; 0 refus à m40 | I68/COVERAGE-MATRIX ligne 7 ; I68/REMAINING-FAMILIES §3 (correction AC) ; I66/IMAGE-REGRESSION-STATIC-LEADS L2 l.100, L6 l.164 | ABSENT (refus) | b9 `FC_Snow*` +0x60..+0x9B, b7 `FC_SnowRate`, b2 `FC_SnowMaskParam` non produits. | `native_selected_constant_unavailable` à m31 et m45 : familles `_SnowLt` refusées. |
| R-25 | Les programmes de sommet POM qui lisent `VFC_PomLightDir` (cbvModelParam +0x70) restent refusés : leur producteur (contexte +0x1290) est absent. | m31 (33 matériaux POM), m37 (32) | I66/FOLIAGE-F1-NATIVE l.6-7 ; GB l.10408-10409 ; I68/COVERAGE-MATRIX ligne 4 (« admitted », antérieur) | OUVERT (contradiction entre les deux sources) | Producteur du contexte +0x1290 non écrit. | `native_vmodel_param_unproduced` à m31 et m37 : refus POM, oui ou non. |
| R-26 | La canopée d'arbre `m009002` (feuillage orange) que l'original dessine en W2 est absente en natif. | m30 W2 (24,3, -10,3, 96,1) ; Native1210/1211 contre Reference1212 | I66/NPC-FAMILIES-NATIVE l.104-106 ; I66/FOLIAGE-F1-NATIVE l.12 ; I68/LIVE-CHECKS-CONSOLIDATED §1.5 FOL4/FOL5 | OUVERT | Première porte défaillante de FOLIAGE-NATIVE §9, non lue. | FOL4/FOL5 : rangées 57A20 de `m009002` dans l'original, et résultat d'admission natif. |
| R-27 | Le sang sur les personnages : le patch `GXBloodyMaterialPatch` reçoit le décal neutre, donc pas de sang. Les décals différés (sang d'impact, `Decal_DifSpcAm` de Lige-Feu) ne sont pas dessinés : le consommateur 19B690/198910 est remplacé par un refus. | m40, combats | GB l.6599-6611 (1184), l.1825 (`native_decal_draw_ready:false`), l.1829-1831, l.16698-16699 ; I68/DECALS-NATIVE §5 et dernier §, I68/COVERAGE-MATRIX lignes 29-30 | OUVERT / ABSENT (code). **Contradiction :** STATE l.14 cite « decals » parmi ce qui tourne en natif | Le tirage GPU des décals n'est pas écrit ; seules les requêtes CPU (types 0 et 1) sont natives. | Lige-Feu, vue du feu : nombre de `Decal_DifSpcAm` dans l'original contre 0 en natif ; image GB1-4 (D1-D6). |
| R-28 | Les rangées forward reçoivent des entrées neutres : IBL t30/t31 en cube noir, SSAO t33 blanc, RLR t44 à zéro, tampons de lumières locales t59..t62 vides, pour toute fente qu'aucune texture de matériau ne nomme. | toutes (cheveux, translucides, phantoms) | GB l.371-376, l.6612-6622, l.6708-6713 ; I68/COVERAGE-MATRIX ligne 28 | OUVERT (« explicit open policy (1114) ») | Politique ouverte explicite, héritée de l'éclairage différé sans IBL. | Comparer un draw forward (cheveux ou phantom) : mots t30/t31/t59..t62 liés dans l'original. |
| R-29 | La palette de tissu auxiliaire (route tissu de 64340) n'est pas possédée par les poses ChrAsm natives. | m30 Argo (cape) | I66/NPC-CHARACTERS l.126-132 | OUVERT (politique ouverte explicite) | non documenté | La cape d'Argo en mouvement côte à côte. |
| R-30 | Le lézard de cristal (`RecursiveCrystal`) est plus sombre et sans la lueur de l'original. | W5, Native1214 contre Reference1214, `ingame-teleport.jpg` | I66/NPC-FAMILIES-NATIVE l.86-90 | OUVERT ; témoin antérieur à W6 (lueur) et W10 (YEBIS) | Lueur et bloom alors absents ; point de copie t45 = politique explicite, L5.1 non lu. | Reprendre la paire W5 avec les défauts actuels. |
| R-31 | Les familles ennemies spéciales n'ont pas de témoin : Veil, Ghost, Fur, Eye, ShortBoneIndices, profils SSS par matériau des ennemis ; les ennemis W3, W4 (`c5270`) et W7 (`c5110`) non plus. | m30 arènes, m40 `c5110` | I66/NPC-CHARACTERS l.132-136 ; I66/NPC-FAMILIES-NATIVE l.122-125 | OUVERT (non témoigné) | non documenté | LIVE-CHECKS §1.7, W3/W4/W7. |

#### 1.A.4 YEBIS (post-traitement)

| ID | Écart | Zone / témoin | Source | Statut | Cause connue ou hypothèse | Quoi vérifier |
|---|---|---|---|---|---|---|
| R-32 | Les niveaux de DoF à 17 taps sont refusés. | aucune zone témoin ne les utilise (m40 : 9 taps, ajoutés) | I66/W10-NATIVE-PASSES §13 l.663, l.678 | OUVERT (« stay refused ») | Pas de témoin. | Recenser les presets de DoF (142B659C0) des fichiers gparam pour une valeur de 17. |
| R-33 | L'étalonnage des cinématiques (enregistrements « remo ») est refusé ; la clé de vue remo de la lueur n'est pas couverte. | cinématiques | I66/W10-NATIVE-PASSES l.678 ; I66/GLOW-WAVE1-READY §10 l.324 | OUVERT (« prefix unread ») | Le préfixe n'est pas lu, et `Banks` n'a pas de sélection `remobase`. | Une cinématique en jeu, Native contre Reference : étalonnage et lueur. |
| R-34 | Sont refusés : ColorV ≠ 1, le gamma de bloom (+0x1B0) ≠ 1, les formes teintées et les formes de glare 2-7 ; GlareShape 0 laisse t1 noir. | aucune zone témoin (pas de témoin) | I66/W10-NATIVE-PASSES l.207-214, l.250 | OUVERT ; « INFERRED equal » pour GlareShape 0 | Pas de témoin. | Recenser les groupes Bloom et ToneMap des 168 fichiers gparam pour ces valeurs. |
| R-35 | Le motion blur, enchaîné après notre surface de vitesse, diffère sur 47 et 228 texels sur 921 600 ; la vitesse objet vient d'une réduction native, et non du raster demi-taille de l'original. | m37 Reference1693 (caméra tournante) | I66/W10-NATIVE-PASSES l.540-542, l.551-560 | INFÉRÉ (compilation D3D11/D3D12 du même DXBC) ; « not done: a byte comparison of the reduction » | Une demi-unité de float sur 188 texels de vitesse. | La comparaison d'octets de la réduction contre une vitesse objet capturée. |
| R-36 | Le changement de lentille entre deux nombres de niveaux de DoF relance la mise en page de l'étage (un drain) ; tan/atan de la CRT ne sont égaux bit à bit que sur le processeur de cette machine. | toutes | I66/W10-NATIVE-PASSES l.468-470 | OUVERT (« no witness of a cost yet ») | Les nombres de niveaux suivent la lentille. | Un changement de lentille en jeu : coût du drain ; un autre processeur pour la DoF. |
| R-37 | La boucle de 30 minutes V5 (redimensionnement et téléportation) n'a pas été faite. | — | I66/W10-NATIVE-PASSES l.678 ; STATE l.18 | OUVERT (« Not done ») | — | Lancer la boucle V5. |

#### 1.A.5 Lueur (W6)

| ID | Écart | Zone / témoin | Source | Statut | Cause connue ou hypothèse | Quoi vérifier |
|---|---|---|---|---|---|---|
| R-38 | La sortie de lueur n'a aucun témoin image : l'apport de la lueur à l'image, contre l'original, n'est pas isolé. | m40 `c2150` ; Native1893/1894 | I66/GLOW-WAVE1-READY l.408-417 ; I66/WAVE1-LAST-EFFECTS §5 l.125-135 ; STATE l.19 (« W6 glow image witness ») | OUVERT | Une capture de l'original exige un hook sur le draw D3D11 de ses dix quads. | GL-8 `glow-verify`, ou `c2150` à Lige-Feu, contre un hook des dix quads D3D11. |
| R-39 | Le mode A de la lueur n'est pas implémenté ; les jumeaux FFX ne sont pas couverts ; l'échantillonneur `SS_ClampLinear` est statique, la rangée exacte de la banque n'est pas lue (simplification) ; le modèle CPU tient à 2/255 près. | toutes | I66/GLOW-WAVE1-READY §10 l.324, l.380, l.385 | OUVERT | non documenté | La rangée 4 de la banque d'échantillonneurs dans l'original ; une zone en mode A, s'il en existe une. |

#### 1.A.6 Matériaux et personnages, phantoms et PNJ

| ID | Écart | Zone / témoin | Source | Statut | Cause connue ou hypothèse | Quoi vérifier |
|---|---|---|---|---|---|---|
| R-40 | **Silhouette de phantom PNJ d'Albert (Lion Knight Albert)** : en natif la silhouette passe devant la cape d'Argo ; l'original garde la cape devant. Sont identiques à l'original : l'invocation elle-même, les couleurs, et l'alpha, qui monte de 0,01 à 0,25 pendant le fondu d'apparition dans les deux modes. Ce n'est pas la cible de lueur GB5. Albert porte un heaume fermé : il n'y a pas de visage à comparer. | m30, signe `c0000_0007` avant Vordt, sauvegarde Argo ; run 2071 et suivants (fil du pilote) | Messages du coordinateur (Thomas, note du coordinateur, fil du pilote 2026-10-18 03:03-03:04 UTC), **pas encore dans le dépôt** ; contexte : I66/WAVE1-LAST-EFFECTS §4 l.119-123 | OUVERT ; le pilote n'a pas réussi à le corriger | Piste : ordre ou profondeur des passes forward (rangées 0x21/0x29 du phantom contre la cape). Le test « natif seul sans la deuxième cible de lueur des passes avant » (run 2071 et suivants) n'a pas corrigé l'écart. | État de profondeur, écriture de profondeur et ordre des rangées 0x21/0x22/0x29 du phantom PNJ et de la cape (0x1F/0x29), dans l'original contre le natif. |
| R-41 | Le phantom PNJ blanc du signe (Albert) montre ses surfaces intérieures en natif (WAVE1-LAST-EFFECTS : cela « cache un visage vu de face » ; d'après le coordinateur, Albert porte un heaume fermé). Probablement la même famille que R-40 : non établi. | m30 (6,7, -6,3, 87,1) ; Native1878 contre Reference1879, `effects-shots/1878-ingame-phantom-181.jpg` | I66/WAVE1-LAST-EFFECTS §4 l.119-123 | OUVERT (« the cause is still open ») | L'original garde aussi la rangée de culling 0 ; prochaines lectures : rangées et états du phantom PNJ, classe du modèle, chemin de draw (PREP §2.4 R1-R3). | Les rangées et états du phantom PNJ, sa classe et son chemin (modèle simple ou ChrAsm), dans l'original. |
| R-42 | Le fantôme du signe avant l'invocation est plus opaque en natif ; la hache et le bouclier du phantom invoqué restent sombres là où l'original les blanchit. | m30 signe du Haut-Mur ; Reference/Native1911, `summon-1911-crop.jpg` | I66/WAVE1-LAST-EFFECTS §2c l.94-96, §7 l.157-158 | OUVERT ; les armes sont peut-être fermées par §8 (Native1923, Sword Master), sans nouveau témoin sur Albert | Les parties d'arme P_WP n'avaient pas leur alternative `_Phantom_Fwd` avant §8. | Reprendre la paire au signe d'Albert avec les défauts actuels. |
| R-43 | De près, l'intérieur du visage du phantom invoqué (Sword Master) reste plus sombre en natif que la translucidité blanche de l'original. | m30 signe `c0000_0008` (25,0, -25,5, 52,3) ; Native1923 contre Reference1921, `effects-shots/swordmaster-1923.jpg` | I66/WAVE1-LAST-EFFECTS §8 l.176-177 ; STATE l.16 ; coordinateur, 2026-10-18 (visage découvert, présent en natif depuis Native1923 ; ne reste que « intérieur du visage plus sombre de très près ») | OUVERT (« Residual ») | non documenté | Une paire de près sur le visage ; mots du PS `_Phantom_Fwd` du visage (FgFaceSSSIbl) natifs contre originaux. |
| R-44 | La rangée phantom 0x22 (classe 0x100) utilise `Fwd` au lieu de `FwdSdw` ; une rangée 0x29 de catégorie 0x29 est mélangée en alpha quel que soit le mode du matériau. | m30 phantoms | `native/content/model_pass.cpp` l.42-46 ; I66/PHANTOMS-NATIVE l.25-30, « Open » l.64-65 | OUVERT (« explicit policy while the native has no shadows ») | La politique date d'avant l'ombre solaire (D-0180). | Recenser les draws 0x22 de l'original (programme `FwdSdw`) ; mélange des 0x29 non-phantom. |
| R-45 | Les PNJ phantoms « modèle simple » (`SprjModelDrawEntity`, D07140) ne sont pas transcrits ; la position de 0x21 parmi les passes d'effets (P10), les états du 0x29 en fondu (PL6), les signes (P12-P14) et les lectures PL1-PL10 restent à faire. | m30 | I66/PHANTOMS-NATIVE « Open » l.62-66 | OUVERT | non documenté | PL1-PL10 sur l'original. |
| R-46 | Les modèles de catégorie 0x21/0x29 ne prennent pas de rangée de vitesse 0x34 ; l'original n'est pas vérifié. | toutes | I66/PHANTOMS-NATIVE l.70-71 | OUVERT (« unverified ») | non documenté | `Velo` rows d'un phantom dans l'original (MotionBlur ENABLE). |
| R-47 | L'état de profondeur des rangées 0x29 en fondu, dans l'original, est déduit de l'image. | m30 Native1878 / Reference1878 | I66/WAVE1-LAST-EFFECTS §6 l.149-150 | INFÉRÉ (« from the picture ») | — | Lire l'état de profondeur du draw 0x29 en fondu dans l'original. |
| R-48 | Les bits 26-28 du mot modèle +0x224 (bits de nom spx) ne sont pas produits en natif ; l'écrivain d'origine est inconnu. | toutes | I66/NPC-FAMILIES-NATIVE l.113-118 | OUVERT | Le natif ne consomme que les bits 15 et 12. | Recenser les lecteurs de ces bits dans l'original. |

#### 1.A.7 Divers dans la scène

| ID | Écart | Zone / témoin | Source | Statut | Cause connue ou hypothèse | Quoi vérifier |
|---|---|---|---|---|---|---|
| R-49 | À m31, le natif dessine deux croix en bois et un tissu que l'original ne montre pas (déjà dans Native1824). | m31 ; Native1914, Native2044 contre Reference1796-1806 / 2007 | I66/WAVE1-LAST-EFFECTS §6 l.141-143, §7 l.156 ; STATE l.16 | OUVERT | non documenté | Identifier les modèles (rangées natives à cet endroit), puis leur admission ou leur fondu dans l'original. |

#### 1.A.8 Eau et tessellation

| ID | Écart | Zone / témoin | Source | Statut | Cause connue ou hypothèse | Quoi vérifier |
|---|---|---|---|---|---|---|
| R-50 | La surface d'eau (`GXFlver_WaterBumpTri` : `WatDpt` puis `FwdSdw`) n'est pas dessinée : les passes de shader 4 et 12 ne sont pas sélectionnables, et il n'y a pas de rangées 0x24/0x27. | m37 pièce `m000023` (9 des 11 captures m37) ; m40 pièce `000012` ; eau aussi à m31, m45, m50 (données) | `native/content/model_programs.cpp` l.38-41 (passes 0, 1, 2, 6, 7 seulement) ; I68/WATER-NATIVE §0 ; I66/WATER-TESS-WAVE1-READY §0 points 1-2, §2 | ABSENT | Les commits WT-1..WT-8 ne sont pas faits (aucun journal d'implémentation eau). | m37 Gamma devant `m000023` : draws `WatDpt`/`FwdSdw` de l'original contre 0 en natif ; image. |
| R-51 | La vue de réflexion planaire (demi-résolution, caméra miroir) n'est pas native. | m37 (deuxième appel d'éclairage), m45 (vue 640x360 sous l'eau) | I66/WATER-TESS-WAVE1-READY §0 point 5, §8 points 1-2 ; I66/SHADOWS-WAVE1-READY l.389-391 ; `native/world/light_producers.cpp` l.816-819 | ABSENT | Le plan WT-6/WT-7 n'est pas implémenté. | Draws de la vue de réflexion de m37 dans l'original contre le natif. |
| R-52 | Aucun draw `_PN` tessellé n'a été dessiné en jeu. `_FT`, `_D` (TS-6) et `Dpt_PN` (ombres) ne sont pas pris en charge ; un échec de pipeline `_PN` arrête le rendu natif. | m50 arbres `m001523`, m45 `c6060` ; Native1580-1587 | I66/WATER-TESS-WAVE1-READY l.356-371 ; STATE l.15, l.19 (« the m50 tree / c6060 tessellation witness ») | OUVERT (témoin) ; derrière `native-tess.flag` (voir D-01) | Les arbres sont à 100 m de la caméra du feu ; `c6060` n'est pas dans la vue. | Une sauvegarde près des arbres de m50, ou `c6060` en vue, avec `native-tess.flag` (WL8/WL14). |

#### 1.A.9 Performances

| ID | Écart | Zone / témoin | Source | Statut | Cause connue ou hypothèse | Quoi vérifier |
|---|---|---|---|---|---|---|
| R-53 | m45 : 58,5 à 59,95 images/s ; environ un cinquième des frames dépasse 17 ms (la collecte du thread moteur). | m45 ; Native1842/1843, `cascade-casters-shots/i66-native-1843-m45-final.jpg` | STATE l.17 (troisième étape) ; mesures de §6/§7 de WAVE1-LAST-EFFECTS (59,6 / 59,5) | OUVERT ; les leviers restants sont reportés à la revue d'optimisation de la vague 2 (Thomas, 2026-10-17 01:41) | Coût de la collecte (lectures par modèle, boucle de passes). | `sample-profile.flag` à m45, en run de mesure sans couche de debug. |
| R-54 | m31 reste quelques dixièmes sous 60 images/s, même sans ombre. | m31 ; Native1801, Native1823 (59,5) | I66/SUN-SHADOWS-WAVE1 §7 l.211-216 | OUVERT | non documenté | Mesure m31 sans instrumentation, et profilage. |
| R-55 | La comparaison de cadence de l'original (intervalles de présentation, p99) n'est pas faite ; le pacer cR4 avec cD9, RT-2 et RT-9 est un témoin ouvert. | toutes | ROADMAP Wave 1 point 5 l.50-55 ; STATE l.19 | OUVERT | — | La revue d'optimisation de fin de jeu de base (D-0167, D-0169). |

#### 1.A.10 Témoins de zone à mesurer de nouveau (aucune valeur postérieure documentée)

| ID | Écart | Zone / témoin | Source | Statut | Cause connue ou hypothèse | Quoi vérifier |
|---|---|---|---|---|---|---|
| R-56 | Les derniers rapports de HDR éclairé documentés sont m31 0,73, m50 0,67, m45 0,74. Après les correctifs lightmaps, Dominant et diffuse, seuls m30 (0,98) et m37 (0,99) ont été mesurés ; ailleurs, le côte à côte est jugé « à l'œil ». | m31 Native1745, m50 1748, m45 1753 | I66/SUN-SHADOWS-WAVE1 tableau l.89-96 ; I66/WAVE1-LAST-EFFECTS §1 l.34-36 | non documenté après 2026-10-16 | — | `Compare-SceneHdr.ps1`, image 2400, aux six zones avec les défauts actuels. |
| R-57 | Les ratios de IMAGE-REGRESSION §0 sont comparés à l'original **non étalonné**. | m40, m37, m31 | I66/IMAGE-REGRESSION-STATIC-LEADS §0 l.11-13, §2 point 4 | Mise en garde de la source | — | Ne citer que des ratios contre l'image étalonnée de l'original. |

### 1.B Menus, interface, options, langues, fondus, films, audio, entrées, affichage, pacing (56 écarts, M-01 à M-56)

Abréviations : I66, I67, I68, I69 = les dossiers `investigations/I-0066...` à `I-0069...` ; « MTP » = MENU-TESTS-PREP (**hors `main`**, branche `claude/relaxed-almeida-957f40`) ; « Coord. 18/10 » = relais du pilote, pas encore dans le dépôt. M-19 et M-20 (silhouette d'Albert, visage du Maître d'armes) sont dans le tableau rendu : voir R-40 et R-43.

| ID | Écart | Zone / témoin | Source | Statut (source) | Cause connue ou hypothèse | Quoi vérifier |
|---|---|---|---|---|---|---|
| **Langues et polices** | | | | | | |
| M-01 | Langues jamais lancées : italien, allemand, espagnol (spaES), portugais brésilien, polonais (police propre `Polish`, latin étendu) | aucun run | FONTS-LANGUAGES-NATIVE §1, §6 ; MTP §1.6, §7 (hors main) | OUVERT | MTP : les ordinaux latins partagent le jeu de polices de `fra` (TsukuMinPro + TsukuOldMinPro) ; polonais « optionnel » (Thomas, 2026-10-16) ; D-0153 demande toutes les langues | décider si FL5 couvre la famille latine (D-0150) ou lancer un run titre + menus par langue ; polonais au minimum |
| M-02 | Espagnol d'Amérique latine (`spaAR`, id 27) : jamais lancé ; l'aide de test ne sait pas produire « latam » | aucun | FONTS-LANGUAGES-NATIVE §1 ; MTP §1.6 | OUVERT | id 27 = `spanish` + paramètre régional Windows d'un des 20 pays ; `latam` de Steam ne donne PAS spaAR | étendre `test-language.flag` ou documenter la non-couverture |
| M-03 | Contrôle de la page Steam du jeu : MTP dit qu'elle n'a pas été consultée ; UI-INGRESS (2026-10-08) liste 12 langues déclarées contre 13 ordinaux sélectionnables | aucun | MTP §7 ; I66 UI-INGRESS « All supported text/menu languages » | OUVERT | la liste de 12 n'a pas `spaAR` ; aucune réconciliation écrite | réconcilier les 12 déclarées et les 13 ordinaux |
| M-04 | FL-pol2 / FL-pol3 : fichiers demandés (`font.gfx`, `80_language.tpf`, `msg:`) jamais journalisés en run de langue | FL5-FL7 : jugés aux captures | FONTS §6 ; MTP §1.5 L2 | OUVERT | MTP : « inferred from the screens » si le run ne les enregistre pas | journal VFS dans une paire de langue |
| M-05 | FL-pol8 : octets `+0x2C/+0x30` du profil (langue sauvegardée, -1 attendu) jamais lus | aucun | FONTS §6, §8 point 2 | OUVERT | si un menu écrit une autre valeur que -1 : OUVERT (aucune page de langue parmi les 102 films) | une lecture `[[1447572B8]+0x58]+0x2C/+0x30` |
| M-06 | F4/F5 : nombre max d'enregistrements glyphes par appel en CJK, recharge du cache en défilant une longue liste, cellule réécrite lue périmée | FL6 : menus et page Équipement seulement | FONTS §6 F4/F5, §8 point 3 ; MTP §1.3, §1.5 L5 | OUVERT | 1 page 1024², ~1 000 cellules ; ja/zh dépassent → éviction (INFÉRÉ) ; un refus = glyphe manquant sans ligne d'erreur (MTP §1.3) | L5 : défiler une longue liste d'objets en schinese aller-retour, comparer aux captures |
| M-07 | Anciens dialogues à police bitmap (`DSFont24`, `TalkFont24`) : lesquels restent atteignables en version commerciale | aucun | FONTS §4, §8 point 5 | OUVERT | INFÉRÉ : jeu de polices « JAPANESE » des vieux dialogues | CL6 (casse sur les lecteurs) |
| M-08 | Texte FD4/Ez sur la couche 2D GX : pipeline de la police de débogage différé par le natif (`native_optional_visual_deferred debug_font`) | aucun (H1-H8, DT1-DT5) | I67 RESULT tranche 7 ; FD4-TEXT-GX-NATIVE | OUVERT (différé) | si les dialogues FD4 de texte d'événement dessinent en commercial (SUBTITLES §7 D, OUVERT), ils passeraient par là | CL6 puis DT1-DT5 |
| **Sous-titres et dialogues** | | | | | | |
| M-09 | CL5 : la sonde `caption-observe.flag` (+0x1654..+0x1663) n'a vu que l'état vide pendant la réplique, dans les deux modes | Native2063 / Reference2060 | SUBTITLES-DIALOGUE-NATIVE §6 CL5 | OUVERT (« the picture is the witness ») | la file lue n'est pas celle qu'utilise ce dialogue | trouver la structure réellement écrite pendant un dialogue PNJ |
| M-10 | CL1-CL4 et CL6 jamais lancés (file de dialogue, sous-titres du film d'ouverture, octet d'option, appels de cadre du clip, texte d'événement) | aucun | SUBTITLES §6 | OUVERT | — | les cinq lignes, lecture seule |
| M-11 | Option Sous-titres : octet `+0x10` = Subtitle (INFÉRÉ, fort), polarité, assainisseur `FUN_1408100C0` chiffré ; jamais basculée | aucun | SUBTITLES §7 C, §7b points 1-5 | INFÉRÉ / OUVERT | — | CL3 : lire avant le dialogue, basculer dans la page d'options |
| M-12 | Dialogue hérité `FrpgMenuDlgTalkMsgbox` : jamais instancié ? écrivain de `TalkWindow` du HUD : aucun trouvé | aucun | SUBTITLES §7 A-B ; FONTS §7 | OUVERT / INFÉRÉ (clip laissé, jamais piloté) | — | CL2, CL4 (présence d'appels `F04_open_BG`) |
| **Pages Scaleform** | | | | | | |
| M-13 | Filtre ombre portée (kind 1) et branche séparable 7x7 (`0x200000`) jamais vus à l'écran | titre, Esc, Équipement, HUD : Native1268-1270 | SCALEFORM-FILTERS-NATIVE §6 « Not witnessed live » ; SCALEFORM-FILTER-PASSES PW3, PW4, PL4-PL6 | OUVERT | le recensement place ces filtres sur Équipement, Inventaire, Statut, Options | PW3 (Inventaire/liste d'équipement), PW4 (Options) natif contre original |
| M-14 | Cache-as-bitmap (sélection de couleur de la création de personnage `04_030`) : jamais témoin ; MTP s'attend au refus `native_ui_display_offscreen_unavailable` | aucun | SCALEFORM-FILTERS §6 (F8 « stay open ») ; MTP §3.3, §3.4 item 5, §6 | OUVERT | STATE : W1-W2 (F8a, F9) faits ; on ne sait pas si un pilote a atteint cet écran (MTP §6) | ouvrir l'écran de couleur en création de personnage, chercher la ligne de refus |
| M-15 | File de témoins de l'audit des menus (D-0170) : aucune page de ces familles n'a été ouverte en natif : Inventaire `02_020`, Coffre `03_050` ; pages PC graphismes `02_042`, souris `02_041`, touches `02_043`, saisie de texte `02_044`, liste de commandes `02_045` ; Nouvelle partie luminosité `05_031` et commandes `05_032` ; famille du feu de camp (Montée de niveau `03_010`, Harmonisation `03_020`, Voyage `02_080`, répartition des Fioles `02_100`, Boutique `03_000/001`, Renforcement `03_040/041/043`, comparaisons `03_030/032/033`) ; petites familles (messages `01_030-035`, gestes `01_040/041`, Kick `02_060`, Classement `02_090`, Colisée `02_200`, saisie `02_990/991`, Chargement `02_901-904`, avis d'urgence, message d'invocation, info titre `05_020`, conditions `05_040`, logos `05_900-905`) | aucun témoin documenté ; le commit e8675522 ajoute `Run-AlphaCast.ps1 -BonfirePages`, sans run cité | MTP §3.4 (hors main) ; MENUS-NATIVE-GAPS §2 ; MENUS-MAP §78.4 | OUVERT | MTP : la moitié statique de l'audit est close (104 noms, 243 références) ; un run propre sans ligne `native_ui_*` est le témoin complet d'une famille | une page par famille, natif contre original |
| M-16 | Deux noms de page sans film (104 noms contre 102 films) ; `02_041_PC_MouseSetting` et `02_210_Coliseum_Dialog` ouverts par un nom choisi à l'exécution | aucun | MTP §3.2, §3.4 item 7, §6 | OUVERT | INFÉRÉ `02_998/02_999` ou pages Trial | listage d'archive en lecture seule ; ouvrir la page souris |
| M-17 | Images `MapImage` / `Knowledge` par nom (pages Voyage, feu de camp) ; bannière HUD (VOUS ÊTES MORT, rampes de flou PW7) jamais témoin | aucun | MENUS-NATIVE-GAPS §2 ; MTP §1.5 L6 ; SCALEFORM-FILTERS L5 | OUVERT | — | une visite de feu de camp ; une mort en jeu |
| M-18 | Vues de menu : ombre de la capuche dans la vue Statut ; sourcils plus pâles et profondeur des cheveux non écrite dans les portraits | Native1181-1182 (portraits) | MENUS-NATIVE-GAPS §2 ; I66 MENU-OFFSCREEN-MODELS « Native1182 » | OUVERT (au 2026-10-11) ; état depuis : non documenté | famille des ombres absente à l'époque (l'ombre du soleil est native depuis D-0183, son effet sur la vue Statut n'est pas documenté) | comparer Statut et portraits à l'original |
| **Options** | | | | | | |
| M-21 | OP-1 : niveaux de texture 0..4 jamais comparés au filtrage de l'original | aucun | STATE « Open witnesses » ; feuillet G §6, « Pilot results » Open | OUVERT | — | balayage `Run-OptionSweep.ps1 -Shape PerKey` sur TextureQuality, natif contre original |
| M-22 | OP-3 : Shade DISABLE..MAX aux portraits | aucun (O0 lu au niveau des octets dans Native1374-1378) | STATE ; feuillet G §6 | OUVERT | — | portraits à chaque niveau, natif contre original |
| M-23 | OP-4 : luminosité réglée par la page d'options en jeu ; L2 (même image) et L5 (contre l'original) jamais faits ; la prise `gamma-verify.flag` n'est pas construite | seulement `brightness-override.flag` : Native1463 (0, plus sombre) / 1464 (10, plus clair), sans original | STATE ; feuillet G §6 ; GAMMA-COLOUR-PATH-READY §5.2, journal | OUVERT | aucun pilote ne pilote la page d'options en jeu | piloter Esc > Système > Options > luminosité, 0/5/10, natif contre original |
| M-24 | OP-5 : un changement de résolution ou de mode d'écran par la page d'options, et un redimensionnement manuel, n'ont **aucun effet en natif** | aucun | feuillet G §2-§3 M-3, §6 OP-5 ; MTP §4.1 | OUVERT (statique) ; image étirée et décalage souris INFÉRÉS | requête déposée sur l'objet GX absent (`_DAT_144961AB0` nul) ; `SceneRenderer::Resize` sans appelant | voir M-48 |
| M-25 | OP-7 : effet du préréglage QualitySetting changé **en jeu** (l'édition du fichier ne recalcule pas les niveaux, QL3) | Native1263 natif seulement, pas d'original à LOW | feuillet G §6 ; QUALITY-OBJECT-NATIVE-LIVE « QL3 » | OUVERT | logique de page chiffrée | page d'options pilotée, original |
| M-26 | Le balayage de tous les niveaux (D-0166) date d'avant les familles : SSAO, ombres, DoF, flou cinétique, effets, lueur sont devenus natifs depuis ; aucun rebalayage documenté | Native/Reference1374-1378, Native1454-1456 (2026-10-12) | feuillet G « Pilot results » ; ROADMAP vague 1 point 3 | OUVERT | le feuillet : chaque famille « then replays this sweep for its own keys » ; aucune trace de ce rebalayage | rejouer OP-0 avec images natif contre original par clé |
| M-27 | Antialiasing : octet `+0x08` lu par aucune passe native ; filtre GX DLAA (`AntiAliasType` 4) absent | aucun | feuillet G §2, §3 M-4 ; I68 COVERAGE-MATRIX ligne 41 (« ABSENT ») ; YEBIS-WAVE1-READY YB-4b | OUVERT | relu 2026-10-18 : aucune passe DLAA n'est dessinée ; seul le producteur du bloc de constantes `CBDlaaParam` existe (`native/world/light_producers.cpp` 903 et 1073, compilé dans le module) | confirmer avec l'agent rendu ; option ON/OFF natif contre original |
| M-28 | Compteurs O3 « total des PSO » et « tables d'échantillonneurs » jamais faits | — | feuillet G « Pilot results » O3 ; PILOT-WORK-PLAN O3 | OUVERT | — | nécessaires pour mesurer le coût d'un changement d'option |
| M-29 | Sémantique de la page d'options chiffrée : application immédiate ou en sortant, niveaux des préréglages, Auto-detect | — | MENUS-MAP §77.5 point 1 ; feuillet G §7 point 1 | OUVERT (code chiffré) | `SprjDrawStep` 140D54xxx, `NewMenuSystem` | lecture de code chargé ou run d'original |
| M-30 | Options du son (SoundType `+0xB`, volumes `+0xC..+0xE`) : ordre musique / effets / voix INFÉRÉ, jamais testé (AU3, AU4) ; valeurs par défaut non lues | aucun | MENU-AUDIO-NATIVE §4, §6 AU3-AU4, §7 point 2 | INFÉRÉ / OUVERT | — | AU3, AU4 |
| M-31 | Options de contrôle (RotLR, RotUD, JumpButtonL3, CameraResetUD, CameraDirection, RotSpeed, Rumble) et de réseau (HideWhiteSignInSignEnemyWorld) | aucun | **non documenté** dans le dépôt (aucune occurrence dans les notes ; `Rumble` n'apparaît que comme acte Remo, REMO-CUTSCENES-NATIVE §3) | non documenté | — | carte statique à faire si ces pages entrent dans l'audit |
| **Audio** | | | | | | |
| M-32 | AU1, AU2, AU5, AU6 jamais lancés (sons Load Game et pause, continuité de la musique titre → Continuer, pas de trou au chargement, banque des événements `y`) | Native1175 (musique et sons de menu entendus) | MENU-AUDIO-NATIVE §6-§7 ; I69 RESULT « Suggested next step » | OUVERT | chemin de démarrage de `m000000010`, déclencheur des drapeaux de sourdine, chargeur `sound:`/`fmod:` : OUVERT | AU1-AU6 |
| M-33 | Audio des films mesuré par mécanisme (BL5 : voix de mastering XAudio2) | K7 drapeaux `0x4400` égaux (Reference/Native1250, 1252) ; « Thomas heard the intro cinematic natively » | STATE « Open witnesses » ; BINK-MOVIES-NATIVE BL5 ; BINK-NATIVE K7 | OUVERT | XAudio2 dans Bink, hors FMOD et hors recensement | BL5 |
| M-34 | Voix des dialogues PNJ non couvertes par le recensement (pas de PNJ au Village) ; l'audio de CL5 n'est pas documenté | Reference/Native1573-1578 (Village, sans PNJ) | NATIVE-AUDIO-CENSUS conclusion ; STATE « Open witnesses » | OUVERT | — | recensement pendant un dialogue (Gardienne du feu) |
| M-35 | Sons scriptés des autres zones et passage musique de zone / musique de boss (RL7) | aucun | NATIVE-AUDIO-CENSUS ; REMO-CUTSCENES-NATIVE R12, RL7 | OUVERT | — | RL7 sur une cinématique de boss |
| M-36 | Écoute par Thomas pendant une session native | — | NATIVE-AUDIO-CENSUS dernière phrase | OUVERT (« the remaining check ») | — | — |
| **Films, cinématiques, générique** | | | | | | |
| M-37 | Cinématiques Remo : aucun run natif documenté n'en a joué une ; RL1-RL10 jamais lancés ; lumières de cinématique (R8, RL6) | aucun | REMO-CUTSCENES-NATIVE §0, §6-§7 ; I69 RESULT itération 3 | OUVERT | — | RW1 (porte de brume du boss m30_00), Reference puis Native |
| M-38 | Étalonnage des enregistrements gparam de cinématique refusé natif (« prefix unread ») | aucun | W10-NATIVE-PASSES §12 (règle de refus) et §13 « Not done » | OUVERT | branche de préfixe de `FUN_140095220` non lue | lire la branche ; cinématique en témoin |
| M-39 | Générique R5 : CR5 (fichiers écrits par le jeu, C510 ou C570, `DAT_14489F200` à l'état 7) | Native2065 / Reference2064 | ENDING-CREDITS-NATIVE §5 R5, §8 CR5, §9 point 2 ; STATE l. 16 | OUVERT | — | CR5 avec journal des fichiers écrits |
| M-40 | Générique : CR1 et R1 (appel direct, « never run »), CR2 (passer le générique), CR3 (rythme, mémoire, synchro audio sur les 433,8 s entières), CR4 (groupe de langue, film id 3), CR6 | Native2065 : pages identiques de 10 à 420 s et dialogue « Begin journey 2? » à 480-540 s | ENDING-CREDITS §5, §8 | OUVERT | — | CR2, CR3 mesuré, CR4 |
| M-41 | Encoches GX du proxy de film laissées vides en natif (aucun lecteur CPU atteint jusqu'ici) | Native1250-1256 | BINK-NATIVE « Open » | OUVERT (surveillance) | — | — |
| **Fondus** | | | | | | |
| M-42 | Recouvrement de fondu (`NativeFadeComposite`, `SceneRenderer::EncodeFadeOverlay`, bit 8 de la clé de plan) écrit mais **jamais exercé visuellement** (composite resté 0) | Native2065 ; puis Native2066-2068, 2073 | FADE-PLATES-NATIVE §5 F2 (« unvalidated ») ; Coord. 18/10 : le pilote conclut « rien à dessiner, comme l'original » et « le dossier des fondus est clos » | code non exercé (ni écart ni suspens, Coord. 18/10) ; le dépôt dit encore « unvalidated » | — | mettre FADE-PLATES F2 et STATE à jour avec 2066-2068/2073 ; FD1 (test unitaire) possible sans le jeu |
| M-43 | Fondus restants dans la source : F6 plaque `InGameChroma` sans consommateur natif ; F5 croissance de la liste Ez (FL-fon4) ; FL-fon3 page `FadeScreen` ; FL-fon5 mélange d'index 7 ; FL-fon6 porte `14075EAD0` et courbe `140330F50` chiffrées ; FL-fon7 `PrologueWait` ; FD6 redimensionnement pendant un fondu | aucun | FADE-PLATES-NATIVE §5-§7 ; ENDING-CREDITS §7.4 | OUVERT (source) ; à réconcilier avec la clôture relayée par Coord. 18/10 | §7.4 : l'alpha de la plaque 6 reste 0 sur ce chemin | dire lesquels la clôture couvre |
| **Entrées** | | | | | | |
| M-44 | État souris par `GetAsyncKeyState` (`1423998F0`) non verrouillé (latch) | — | I66 INPUT-LATCH-NATIVE « Open » | OUVERT | — | — |
| M-45 | DI7 : écrivain de la table de repli `[[ctx+0x10]+0x60]` de `FUN_14233D540` ; quels contextes portent l'action 0x13 ; déclencheur du gestionnaire `vt+0x50` | — | NATIVE-INPUT-LATCH §6 « Open » | OUVERT | — | DI7 |
| M-46 | Manette : aucun témoin natif ni fiche ; MENUS-MAP se contredit (l. 2874 liste `XInput` dans les entrées ; l. 4790 « No XInput or Raw Input import ») | aucun | MENUS-MAP (deux lignes citées) | non documenté | — | trancher l'import XInput ; un run à la manette |
| M-47 | Souris et viewport en unités de S quand S ≠ sortie : #2 viewport de film (décalage letterbox/pillarbox), #3 taille caméra écrasée par la sortie (aspect) ; le décalage de pointeur du feuillet G (M-3) | égal sur ce PC (S = sortie) | NATIVE-READS-AUDIT « Findings » #2-#3 ; MENU-OFFSCREEN-MODELS revue 1181-1182 ; feuillet G §2 | OUVERT (Medium / Low-medium) ; #1 : la souris suit S (feuillet G) | — | avec une résolution configurée différente de la sortie (M-48) |
| **Affichage** | | | | | | |
| M-48 | Règle résolution / plein écran (D-0151, D-0170) non implémentée : taille de sortie fixée une fois au démarrage ; `Resize` sans appelant ; pas de plein écran exclusif, pas de liste de modes de l'adaptateur natif, pas d'application différée ni d'écouteurs, pas de gestion d'occlusion ; `MakeWindowAssociation` avec `NO_ALT_ENTER` seul ; aide `test-resolution.flag` acceptée par Thomas (2026-10-16), construction non documentée ; matrice 1080p, 1440p, 4K, taille impaire jamais lancée ; aliasing E7-E9 rouvert là | aucun (toutes les passes témoignées à 1280x720) | STATE obligation 12 ; ROADMAP vague 1 point 3 ; DISPLAY-SWAPCHAIN-NATIVE §1, §5 ; MTP §4 (hors main) | OUVERT | le jeu ramène à 1920x1080 toute résolution hors des deux listes (`FUN_140EF6990`, MTP §4.1) : 1440p et 4K impossibles sur ce bureau 1080p sans l'aide (ou DSR) | ordre MTP §4.3 puis matrice §4.4 |
| M-49 | Boucle V5s de 30 min avec redimensionnements et téléportations ; RT-8 (endurance 30 min) | aucun | STATE « Next action » ; W10-NATIVE-PASSES §13 « Not done » ; R13-R14 §10.2 | OUVERT | dépend de M-48 | — |
| M-50 | Alt-tab, réduire / restaurer, perte de focus en exclusif (`ShowWindow(SW_MINIMIZE)`), boucle de réessai plein écran | aucun | DISPLAY-SWAPCHAIN-NATIVE §4.2 ; MTP §4.4 | OUVERT | — | dans la matrice M-48 |
| **Fréquence d'images / pacing** | | | | | | |
| M-51 | Pacer : RP-1 (source de l'intervalle de l'original), RP-3 (latence d'entrée avec l'attente de synchro), présentation attendable avec le pacer, plein écran exclusif | Native1469 / 1470 (A/B, écran à 120 Hz) | R13-R14 « cR4 with cD9 » Open ; STATE « Open witnesses » | OUVERT | — | RP-1, RP-3 |
| M-52 | Fréquences 60, 75, 144, 165 Hz et G-Sync/FreeSync jamais témoignées (seul 120 Hz l'a été) ; à 75/144/165 le pacer prend la branche manuelle | Native1469 / 1470 seulement | ROADMAP vague 1 point 5 ; R13-R14 « cR4 » | OUVERT | — | règle universelle par fréquence |
| M-53 | RT-2 (échec de Present injecté, exception dans un job) et RT-9 (horloge : vitesse d'animation, distribution de `dt`) | aucun | STATE « Open witnesses » ; R13-R14 §10.2 | OUVERT | — | RT-2, RT-9 |
| M-54 | Revue d'optimisation : rythme de présentation D3D11 de l'original (intervalles, p99, écart) contre le natif aux résolutions D-0170 | aucun | ROADMAP vague 1 point 5 | OUVERT | — | après M-48 |
| M-55 | m45 : 57,2 à 59,95 images/s, « about a fifth of frames still over 17 ms » (collecte du thread moteur) ; m31 59,5 | Native1840, 1842, 1843 ; Native1827 | STATE l. 17 | OUVERT | leviers restants reportés à la revue d'optimisation de la vague 2 (Thomas, 2026-10-17 01:41) : voir section 4 | — |
| **Divers** | | | | | | |
| M-56 | Lanceur double-clic (D-0182) construit, jamais essayé en vrai par Thomas | — | STATE l. 17 ; decisions/LOG D-0182 | OUVERT | — | essai par Thomas |

#### 1.B.bis Tests de menus déjà faits

| Test | Natif / Original | Résultat (source) |
|---|---|---|
| FL5 français | Reference1715 (titre, deux avis, menu principal, run arrêté au titre) ; **Native2051 / Reference2050** | FL1 égal (région 1, texte 2, vivant 2, SKU unique 0) ; titre, avis, menu principal, menu en jeu, Équipement en français avec accents, comme l'original. Native2049 resté en anglais (crochet installé en Reference seulement, corrigé) (FONTS §6 FL5) |
| FL6 japonais | **Native2053 / Reference2052** | titre, menus, Équipement en japonais comme l'original, aucune ligne de refus de glyphe (FONTS §6 FL6) |
| FL6 chinois simplifié | **Native2058 / Reference2054** (Native2055 refusé au chargement : `native_cpu_text_layer_1_unavailable`, couche texte CPU kind 1 admise ensuite) | FL1 texte/vivant 20 ; menus et Équipement comme l'original (FONTS §6 FL6) |
| FL7 danois | **Native2057 / Reference2056** | FL1 8 ; menus anglais dans les deux (FONTS §6 FL7) |
| Russe | **Native2074 / Reference2073** | identique à l'original : titre, menu principal, Équipement, sans refus de glyphe (Coord. 18/10, pas encore dans le dépôt) |
| CL5 | **Native2063 / Reference2060** (`Run-AlphaCast.ps1 -TalkKeeper -TestLanguage japanese`) | première réplique de la Gardienne du feu sous-titrée en japonais comme l'original ; la sonde de file n'a vu que l'état vide (SUBTITLES §6 CL5) |
| R5 générique de fin | **Native2065 / Reference2064** (`Run-AlphaCast.ps1 -SetFlag "20 1" -Casts 0`) | pages identiques à 10, 30, 60, 120, 240, 420 s, dialogue « Begin journey 2 in current state? » à 480 et 540 s ; sortie 0, 0 faute, restauration par empreinte ok ; CR5 ouvert (ENDING-CREDITS §5) |
| Fondus | Native2066-2068, 2073 | « rien à dessiner, comme l'original » (Coord. 18/10, pas encore dans le dépôt) |
| Antérieurs (menus) | Native1156 / Reference1155 (pause) ; 1159, 1170, 1176-1182 (Load Game, portraits) ; 1163, 1169 (Statut) ; 1268-1270 (filtres HUD, Esc, Équipement identiques) | MENUS-NATIVE-GAPS §1 ; SCALEFORM-FILTERS §6 |
| Antérieurs (films) | Reference/Native1250, Native1252 (ouverture 197,875 s, 4749 images, 0 sautée, sous-titre comme l'original) ; Reference/Native1256 (Échap passe le film) | BINK-NATIVE « Live », « Open » K6 |
| Antérieurs (entrées) | Native1243-1247 (5/5 chargements en appuis de 50 ms), 1248, 1249 (touche E), 1253-1254 (Esc 50 ms en jeu, NI4, NI5) | INPUT-LATCH-NATIVE |
| Antérieurs (options, luminosité, pacing, audio) | QL Reference/Native1255, 1263 ; sweep 1374-1378, 1454-1458 ; gamma 1461-1467 ; pacer 1468-1470 ; recensement audio Reference/Native1573-1578 ; musique de menu Native1175 | sections 1-2 |

Remarques pilote (pas des écarts du natif) : blocage « press any key » de `Pilot-TitleContinue.ps1`, non expliqué (STATE l. 18 ; MTP §1.4 OUVERT) ; clavier AZERTY du pilote (avancer = VK `Z`, INPUT-LATCH-NATIVE NI5).

### 1.C Constats de conception du code natif (relevés en lisant le code, détail et lignes en partie 3)

| ID | Constat | Source (code) | Statut | Quoi vérifier |
|---|---|---|---|---|
| C-01 | **La configuration livrée n'est pas celle des tests à deux créneaux.** Par défaut : 1 créneau de trame, 2 tampons de présentation, sans objet d'attente ; `frames-in-flight.flag` et `waitable-present.flag` les changent. Le lanceur de Thomas ne pose que `measure.flag` et `no-debug-layer.flag`. À un créneau, la fin de trame fait `Drain` (attente complète du GPU). Décision consignée : « pas de gain à S1, un créneau par défaut jusqu'à RD12 » (commit 779d4bb7). STATE (ligne « What runs natively ») cite pourtant « frames in flight » parmi ce qui tourne. | `game_bootstrap.cpp` ~15013-15026 ; `scene_pass.cpp` 190, 2550, 2626 ; `tools/Launch-DS3-DX12.ps1` | CONFIRMÉ (code) | mesurer et relire le chemin d'un créneau ; décider du défaut livré |
| C-02 | Attentes complètes CPU→GPU encore présentes dans le chemin de trame lors d'un changement de disposition (flou de profondeur, lueur, flou de mouvement, grading, masques AO et ombre solaire, profondeur) | `scene_pass.cpp` 1314, 1344, 1357, 1791, 2132, 1926, 2391, 2415, 2433, 2466, 2471, 2477 | CONFIRMÉ (code) | un changement de lentille, de zone ou de résolution bloque la trame ; retrait par jeton à la place |
| C-03 | 127 transitions de ressources écrites à la main coexistent avec le graphe (qui les neutralise pour les ressources qu'il pilote) | `scene_pass.cpp` 86-102 (`Transition`, `Suppress`) | CONFIRMÉ (code) | une ressource ni déclarée ni neutralisée dont l'état d'entrée serait faux |
| C-04 | Pas d'aliasing de cibles transitoires : le plan de placement existe (`PlanFramePlacement`) et n'est appelé que par un test | `frame_graph.cpp` ; `frame_graph_test.cpp` ; aucune occurrence de `CreatePlacedResource` dans `native/` | CONFIRMÉ (code) | mémoire de cibles retenue par taille jusqu'à la fermeture |
| C-05 | Shaders propres compilés à l'exécution par `D3DCompile` (composition matérielle, fondu, hors écran, vitesse, FaceGen, témoins) | `scene_pass.cpp` ~403, 425, 457, 1523, 1538 ; `game_texture_ingress.cpp` 111 ; `backend.cpp` 462, 513 | CONFIRMÉ (code) | dépendance à `d3dcompiler` ; hors du cache de shaders prévu en vague 2 (D-0172, D-0179) |
| C-06 | Signature racine des modèles au modèle de slots D3D11 : 14 CBV, 128 SRV et 16 échantillonneurs par étape, version 1.0 volatile, budget de racine 64 sur 64 DWORD en tessellation ; tas globaux seulement si Tier 3, sinon tables par dessin | `model_draw.cpp` 17-60, 255-316 | CONFIRMÉ (code) | comportement sur un GPU Tier 1/2 ; conception propre = liaison par table de ressources |
| C-07 | Le module inclut encore `<d3d11.h>` ; une fonction crée un périphérique D3D11 matériel de passage pour lire des entrées de table virtuelle, dans le bloc `Mode::Reference` (16536) | `game_bootstrap.cpp` 93-94, 2885-2897, 16601 | CONFIRMÉ (code) pour l'existence ; gating à confirmer | qu'aucun chemin `Mode::NativeBootstrap` ni le lanceur n'atteint ces fonctions |
| C-08 | 65 drapeaux `.flag` ; chemins anciens gardés derrière des drapeaux (`legacy-*`, `hand-barriers`, `inline-*`, `rebuild-*`) ; fonctions écrites mais désactivées par défaut (`native-tess`, `ui-cache`, `yebis-host`, `ffx-model-particles`) | partie 3.7 | CONFIRMÉ (code) | code mort ou diagnostic laissé actif |
| C-09 | Code jamais exercé visuellement : `EncodeFadeOverlay` (le composite des fondus est resté à 0) | `scene_pass.cpp` 410-442, 2516-2518, 2611 | CONFIRMÉ (code, M-42) | tester unitairement ou supprimer |
| C-10 | `game_bootstrap.cpp` : 16 954 lignes dans un seul espace de noms anonyme ; `PrepareNativeModelFrame` 1 274 lignes ; `AdmitNativeModelCandidateBody` 651 | clangd, partie 3.2 | CONFIRMÉ (clangd) | lisibilité et testabilité, séparation hôte moteur / backend D3D12 (D-0157) |

### 1.D Décisions assumées qui ressemblent à des écarts

#### Rendu, effets, éclairage

| ID | Différence voulue | Source |
|---|---|---|
| D-01 | La tessellation (W7) reste derrière `native-tess.flag`, désactivée par défaut ; la tessellation en forward est en vague 2. | STATE l.15 ; I66/WAVE1-PREP §4.2 (W5e, W7) ; ROADMAP Wave 2 l.69-70 |
| D-02 | W8/W9 sont décrits « inertes jusqu'aux passes de vague 2 » (STATE l.15). En fait, D-0180 a passé l'SSAO S3-S8, le masque solaire et son composite en vague 1, et ils sont actifs (SUN-SHADOWS §1, §3-§4). Restent inertes : l'atlas des ombres locales et `shadow-local.flag`, qui sont opt-in (décision SW-9). | STATE l.15 ; I66/SUN-SHADOWS-WAVE1 §1, §4 ; I66/SHADOWS-WAVE1-READY l.393-395 ; decisions/LOG D-0180 |
| D-03 | Les lumières locales ombrées (points et spots) sont en vague 2 : l'original ombre une lumière ponctuelle par zone, et un spot à m37 (clé 56) ; le natif ne dessine pas ces ombres. | ROADMAP Wave 2 l.69-70 ; I66/SHADOWS-WAVE1-READY l.386-395 |
| D-04 | L'auto-exposition n'est jamais active en jeu atteignable ; un garde natif refuse notre tone map si elle l'était. | ledger K-0448 (SUPPORTÉ) ; I66/W10-NATIVE-PASSES §11 l.590-597 |
| D-05 | Les cascades sont redessinées à chaque frame, comme l'original (pas de cache). | ledger K-0452 (OBSERVÉ) ; decisions/LOG D-0184 ; I66/SUN-SHADOWS-WAVE1 §7 l.155-160 |
| D-06 | Cible de vitesse en R16G16B16A16_UNORM pleine résolution, au lieu du RGBA8 demi-taille de l'original ; mouvement caméra dans la résolution native, pas de passe caméra GX ; réduction 2x2 pour YEBIS. | I66/VELOCITY-NATIVE « Decisions » l.101-115 ; decisions/LOG D-0162 |
| D-07 | SSR, champs d'AO et ombres de nuages ne sont jamais dessinés, ni par l'original aux réglages de Thomas, ni en natif ; les lignes de lumière ne sont jamais dessinées non plus. | I68/REMAINING-FAMILIES §2 ; I68/COVERAGE-MATRIX lignes 23, 26 |
| D-08 | Une interaction d'eau neutre (gris 0x808080 par défaut) est exacte pour les cinq zones ; la simulation n'est pas prévue. | I66/WATER-TESS-WAVE1-READY §0 point 3, §8 point 8 |
| D-09 | YEBIS (D-0175), l'ombre solaire (D-0183), le dessin FFX (D-0185) et E07 (D-0186) sont activés par défaut, avec des opt-out `no-*.flag`. | STATE l.15-16 ; decisions/LOG D-0175, D-0183, D-0185, D-0186 |
| D-10 | La copie de trame des effets est refaite avant la première passe après 0x29 qui l'échantillonne (`EffectCopyPass`, `no-ffx-effect-copy.flag`) ; la copie de l'original pour 0x2D est INFÉRÉE contenir les effets 0x29. | I66/FFX-DRAW l.414-421 |
| D-11 | La texture par défaut FFX est liée à SYSTEX_WHITE ; aucun draw classique ne l'utilise. | STATE l.16 ; I66/FFX-DRAW l.357-358 |
| D-12 | L'éclairage par sonde des personnages est une « omission initiale explicite » (voir R-04 pour l'effet visible). | GB l.12801-12808 |

#### Menus, options, affichage, pacing

| Point | Décision / source |
|---|---|
| Fenêtre sans bordure et autres formats d'image plus tard ; avant le remaster seulement l'audit des menus et les témoins de résolution / plein écran (1080p, 1440p, 4K, une taille impaire) | D-0170 ; STATE obligation 2. **Noter** : MTP §4.3 (hors main) range quand même le mode sans bordure dans l'ordre d'implémentation (étape 3) : à réconcilier avec D-0170 |
| Cache de shaders et de pipelines (écran de chargement, entrée « recharger les shaders ») en vague 2 ; la vague 1 compile les pipelines comme maintenant (d'où des à-coups au premier usage quand une option ajoute des PSO, feuillet G §4) | D-0172, D-0179 |
| 60 images/s d'abord ; fréquence débloquée et V-sync on/off en options de la vague 2 ; l'horloge du jeu n'est pas touchée | D-0167, D-0169 ; ROADMAP vague 2 |
| Pacer : `Present(intervalle × multiple)` à 120/180/240 Hz au lieu de l'intervalle 1 de l'original à 59/60 Hz seulement (écart étiqueté) ; minuteur haute résolution + spin au lieu de `Sleep(ceil)` | R13-R14 « cR4 with cD9 » |
| m45 : pas trop longtemps sur l'optimisation ; les leviers restants attendent la revue d'optimisation de la vague 2 | Thomas 2026-10-17 01:41 (STATE l. 17) |
| HDR10, titre « Rekindled », logos sur fond noir : vague 2 | D-0182 |
| Mise à l'échelle temporelle (Streamline) : vague 2, désactivée par défaut | D-0165 |
| Réglages remasterisés qui remplacent ceux de l'original en vague 2 (sauf la colorimétrie) | D-0178 |
| Faute à `D12759` en fermant la fenêtre pendant la création de personnage : l'original fait la même faute, pas de correctif natif | MENU-OFFSCREEN-MODELS « Native1171-1172 » |
| Octets non définis de l'objet qualité (déchets de pile) différents entre modes : pas une régression | QUALITY-OBJECT-NATIVE-LIVE « QL2 » |
| Le préréglage `QualitySetting` du fichier ne recalcule pas les niveaux : comportement de l'original | QL3 |
| Ressources du flou cinétique gardées après désactivation ; ligne vitesse sur la copie appliquée ; la collecte prend les options de l'étape de rendu précédente (déviations voulues) | feuillet G « O2 » |
| Danois et les autres langues sans dossier → anglais ; catégories `zhoCN` 76, 77, 80, 81, 90 absentes → texte de repli : identique dans l'original | FONTS §1 ; MTP §1.5 |
| Glyphes absents des polices bitmap des vieux dialogues (accents, Hangul) : pas un défaut du rendu | FONTS F7 |
| Pas d'interrupteur de langue Steam livré ; l'aide `test-language.flag` est un outil de développement, Steam non touché | FONTS F9 ; STATE l. 18 |
| Clics, Entrée et E ne passent pas le film d'ouverture, Échap oui : pareil dans les deux modes | BINK-NATIVE K6 |
| Luminosité : identité hors du monde (b = 0), comme l'original | GAMMA-COLOUR-PATH-READY §3.4 |
| Slot 8 (décalques) fixé au niveau 3 toute la partie | MENUS-MAP §77.2 |
| `TalkWindow` du HUD jamais piloté (reste INFÉRÉ) | SUBTITLES §7 B |

## 2. Options du jeu et menu principal de l'écran titre : chaque paramètre et comment le tester

### 2.1 Ce que les sources établissent, et ce qu'elles ne disent pas

- **Graphismes** : `GraphicsConfig.xml` (`%APPDATA%/DarkSoulsIII`), clés lues dans l'ordre `ScreenMode`, `Resolution-WindowScreenWidth/Height`,
  `Resolution-FullScreenWidth/Height`, `Auto-detectBestRenderingSettings`, `QualitySetting`, `TextureQuality`, `Antialiasing`, `SSAO`,
  `DepthOfField`, `MotionBlur`, `ShadowQuality`, `LightingQuality`, `EffectsQuality`, `ReflectionQuality`, `WaterSurfaceQuality`,
  `ShadeQuality` (MENUS-MAP §77). Le fichier de Thomas (fenêtré 2560x1440, `CUSTOM`, tout à MAX sauf SSAO et Eau à HIGH, MotionBlur HIGH,
  Antialiasing ON, Auto-detect OFF) est sauvegardé et restauré par empreinte par les pilotes.
- **Autres options** : le dépôt n'en donne que les octets du profil et les clés de l'exécutable. Ce document a relu les chaînes de
  `game/DarkSoulsIII.exe` (lecture seule) : clés `Game.Option.Control.RotLR`, `.RotUD`, `.JumpButtonL3`, `.CameraResetUD`,
  `.CameraDirection`, `.RotSpeed`, `.Rumble` ; `Game.Option.Disp.Blood`, `.Subtitle`, `.Brightness`, `.HUD` ; `Game.Option.Sound.SoundType`,
  `.MusicVol`, `.SeVol`, `.VoiceVol` ; `Game.Option.Network.HideWhiteSignInSignEnemyWorld` ; et les noms d'options de la couche de menu
  `optCameraLR`, `optCameraUD`, `optCameraSpeed`, `optVibration`, `optLockOnAuto`, `optCamAvoidWall`, `optShowBlood`, `optSubtitles`,
  `optBGMVol`, `optSEVol`, `optVoiceVol`, `optRegisterRanking`, `optBrightness`. **La correspondance entre les deux séries n'est pas établie**
  (probable : RotLR/optCameraLR, RotUD/optCameraUD, RotSpeed/optCameraSpeed, Rumble/optVibration, Blood/optShowBlood,
  Subtitle/optSubtitles, MusicVol/optBGMVol, SeVol/optSEVol, VoiceVol/optVoiceVol, Brightness/optBrightness : INFÉRÉ ; `optLockOnAuto`,
  `optCamAvoidWall`, `optRegisterRanking`, `JumpButtonL3`, `CameraResetUD`, `CameraDirection` sans correspondance prouvée).
  **Non documenté** dans le dépôt : les libellés exacts des pages d'options non graphiques, la liste des entrées de `02_040_OptionSetting`,
  les valeurs par défaut et les plages de chacune. À relever en ouvrant les pages dans l'original (Esc, Système, Options), puis dans le natif.
- **Pages d'options** (menu Échap, Système, Options) : `02_040_OptionSetting` (concentrateur) et ses jumelles PC `win_02_040`, `win_02_041_pc_mousesetting`,
  `win_02_042_pc_graphicsetting`, `win_02_043_pc_keyconfiguration`, `win_02_044_pc_textselect`, `win_02_045_pc_commandlist` ;
  pages de nouvelle partie `05_031_newgame_brightnesssetting` et `05_032_newgame_controlsetting` (et jumelles `win_`). **Aucune de ces pages n'a
  été ouverte en natif** (M-15). Le moteur applique les options par `FUN_140AB4FC0`, `FUN_140BE4520`, `FUN_140BE4DB0`, `FUN_1405A0930` (code de page chiffré).

### 2.2 Protocole commun (déjà outillé)

- Toute série live est **annoncée dans le fil avant** : protocole écrit, hors ligne, fichiers protégés sauvegardés puis restaurés par
  empreinte (D-0116, D-0117 ; `Restore-StartupTrace.ps1` ; ligne `restoration.json` `ok:true`). Les options de Thomas sont restaurées et
  vérifiées par empreinte après chaque run (D-0166). **Ne rien lancer tant que le PC est occupé par le pilote** ; heures de Paris, début et fin
  annoncés, puis « PC libre » (mémoire du projet).
- Balayage des graphismes : `investigations/I-0066-independent-native-startup/Run-OptionSweep.ps1` (`-Shape All` : toutes les clés à un niveau,
  une paire par niveau ; `-Shape PerKey -Keys <Clé>` : une clé à la fois, les autres telles que dans le fichier de Thomas ; Antialiasing
  `OFF`/`ON` ; paires Reference + Native via `Run-PilotPair.ps1`, High Wall, appuis en jeu, page Équipement). Édition unitaire :
  `Run-StartupTrace.ps1 -GraphicsLevels "TextureQuality=LOW;SSAO=MEDIUM"`.
- Lecture de ce que le module a vraiment lu : ligne `native_quality_object` (niveaux des 12 emplacements et 0x78 octets de l'objet), lignes de
  trame, `native_sss_skipped`, `native_sampler_bank_read_failed`, `native_render_size`. L'original : `reference_quality_object`.
- Comparaisons d'image : paires de captures `pair-*.jpg`, `Compare-SceneHdr.ps1` (HDR éclairé, image 2400), `Compare-YebisReference.ps1`,
  `Run-LiveWitness.ps1` (une zone, une sauvegarde Shebang par zone).
- Les bruts de 1374-1378 datent du 2026-10-12, avant le SSAO, les ombres, YEBIS et les effets natifs : **tout rebalayage est à refaire** (M-26).
  Seul l'octet-niveau lu a été vérifié alors, pas l'image.

### 2.3 Options graphiques : niveaux, lecteur moteur, état natif, test

Niveaux 0 à 4 = `DISABLE`, `LOW`, `MEDIUM`, `HIGH`, `MAX` (table de 0x24C octets, MENUS-MAP §77.3 ; fichier
`investigations/I-0067-static-menus-subsystems-map/quality-table.tsv`). L'état natif ci-dessous est celui du code lu le 2026-10-18.

| Paramètre | Valeurs et ce qu'elles changent dans l'original | Lecteur natif aujourd'hui | Comment le tester | Ce qui existe / manque |
|---|---|---|---|---|
| `TextureQuality` (emplacement 0) | filtre et anisotropie : (1,1), (2,1), (3,4), (3,8), (3,16) pour DISABLE..MAX (`FUN_1400D3650`) | vivant : profil de `EngineSamplerBankOwner` (`sampler_bank.h`), banque publiée au changement de sélecteur | `Run-OptionSweep -Shape PerKey -Keys TextureQuality`, 5 niveaux ; zone à sols texturés rasants (m40 Lige-Feu) ; comparer une découpe de sol à grand angle, natif contre original ; lire `native_quality_object` et les lignes de banque | niveaux lus (1374-1378) ; **OP-1 filtrage jamais comparé** (M-21) |
| `Antialiasing` (emplacement 1) | `OFF` = niveau 0, `ON` = niveau 3 ; octet objet `+0x08` = 1 à HIGH/MAX ; porte du filtre `GXAntiAliasingFilter` (DLAA/FXAA) | **octet écrit, lu par aucune passe** (le natif ne contient pas de DLAA) | `-Keys Antialiasing` OFF puis ON ; bords d'objets au Haut-Mur ou à m40, natif contre original | OP-6 réglé au niveau du bloc ; image jamais comparée (M-27) |
| `SSAO` (emplacement 2) | 7 octets `+0x09..+0x0F` ; la passe tourne **dès LOW**, seul DISABLE l'enlève ; MEDIUM+ ajoute remontée temporelle, upscale, normales | SSAO natif S3-S8 (D-0180) piloté par un plan « effectif » (`ao_stage`) ; lecture des octets par le natif : à relire dans `CollectNativeSelectedScene`/`ao_stage` | `-Keys SSAO` aux 5 niveaux ; m40 et m37 (soleil), HDR éclairé par `Compare-SceneHdr` ; passage MEDIUM vers HIGH (changement de variante temporelle) ; `no-ao.flag` pour l'écart de référence | rebalayage jamais fait (M-26) ; `no-ao*` flags |
| `DepthOfField` (emplacement 3) | `+0x10..+0x1C` ; qualité effective = min(valeur de zone, `quality+0x14`) | DoF natif bit à bit (K-0446) via les fonctions du moteur qui lisent l'objet qualité (`E4890`) | `-Keys DepthOfField` ; zone à DoF actif : m40 (9 taps), m37, m31, m50 ; paire d'images + `Compare-YebisReference` ; les niveaux à 17 taps sont refusés (R-32) | niveaux sous le niveau capturé : OUVERT (feuillet G §7.3) |
| `MotionBlur` (emplacement 4) | LOW : flou caméra seul (plafond 1) ; MEDIUM : + vitesse objet ; HIGH plafond 5 ; MAX plafond 8 ; DISABLE : rien | vivant : ligne de vitesse suit l'objet qualité (`+0x20`, `+0x21`, `+0x24`) | `-Keys MotionBlur` ; caméra tournante (Reference1693 comme modèle) à m37 ; HIGH est le niveau de Thomas | octets lus ; images par niveau jamais comparées depuis K-0447 (R-35) |
| `ShadowQuality` (emplacement 5) | `+0x28` activé ; atlas max 1024, 1024, 2048, 4096, 8192 ; rapport sur la classe de la zone | ombre solaire native (D-0183) ; la lecture de la qualité par le natif (atlas, cascades) est à relire : `CollectNativeShadowState` lit `q[0x28]` et `q[0x44]` | `-Keys ShadowQuality` ; m40 et m37 (soleil) et m45 (la zone la plus coûteuse, D-0183 et D-0184) ; relever la taille d'atlas et le coût par niveau | rebalayage jamais fait ; coût par niveau non mesuré |
| `LightingQuality` (emplacement 6) | `+0x40` = 0,5/0,5/0,75/1,0/1,2 (atténuation des lumières ponctuelles) ; `+0x44`, `+0x45` octets de portes | `+0x40` lu (`CollectNativePointLights`), `+0x44` lu par l'éclairage et les ombres locales (`game_bootstrap.cpp` 3045, 8440), `+0x45` : non lu | `-Keys LightingQuality` ; scène à nombreuses lumières ponctuelles (Lige-Feu, Irithyll), comparer le rayon d'effet | Q7 (Native1255) ; `+0x45` jamais lu |
| `EffectsQuality` (emplacement 7) | 4 octets de portes + 7 flottants du bloc d'environnement FFX | effets FFX natifs (D-0185/D-0186) lisent le bloc par les fonctions du moteur ; lecture exacte à vérifier | `-Keys EffectsQuality` ; feu de camp m40 à chaque niveau ; jumeaux de lueur, distorsion (portes à MEDIUM+) | rebalayage jamais fait |
| (emplacement 8, sans clé) | octet décalques `+0x68`, fixé au niveau 3 toute la partie | — | rien à tester (ne change jamais) | — |
| `ReflectionQuality` (emplacement 9) | `+0x6C` vue de réflexion, `+0x6D` lumières, `+0x6E` masque, `+0x70` diviseur 4 ou 2 (INFÉRÉ) | **aucun consommateur : vue de réflexion absente** (R-51) | `-Keys ReflectionQuality` à m37 (réflexion de 3 maillages OBSERVÉE dans l'original), m45 ; attendre l'écart tant que R-51 est ouvert | tout |
| `WaterSurfaceQuality` (emplacement 10) | `+0x74` (0 à DISABLE/LOW, 1 au-dessus) ; porte du dessin FFX 0x39 | **eau non dessinée** (R-50) | `-Keys WaterSurfaceQuality` à m37 (`m000023`) ; l'écart d'eau domine | tout |
| `ShadeQuality` (emplacement 11) | `+0x75` porte la commande SSS (off à DISABLE/LOW, on dès MEDIUM) ; `+0x77` à MAX seulement | porte SSS lue à la collecte (O0) ; `+0x77` non lu | `-Keys ShadeQuality` ; portraits du Chargement de partie et vue Statut, peau d'un personnage en jeu | octets lus ; **OP-3 aux portraits jamais comparé** (M-22) |
| `QualitySetting` (préréglage) | `LOW`..`MAX`, `CUSTOM` ; **n'est pas recalculé au chargement** (QL3) ; la page d'options, elle, pose un jeu de niveaux (non lu : code chiffré) | aucun | dans la page, choisir chaque préréglage puis lire `native_quality_object` ; original d'abord | QL3 natif seulement ; **OP-7 jamais fait en jeu** (M-25, M-29) |
| `Auto-detectBestRenderingSettings` | logique de page seule | aucun | cocher dans la page et relire les 11 clés posées | jamais testé |
| `Brightness` (hors fichier) | 0..10, courbe `x^(2.2/(2.2+0.5(v-5)/5))` appliquée par la passe gamma avant `Present` ; hors du monde b = 0 | passe gamma native (W3), identité à 5 | `Run-PilotPair.ps1 -Brightness N` (drapeau `brightness-override.flag`) à 0, 5, 10 ; puis la page d'options pilotée (Échap, Système, Options) | Native1463/1464 (0 plus sombre, 10 plus clair) sans original ; **OP-4, L2, L5 jamais faits** (M-23) |
| `ScreenMode` (`WINDOW`/`FULLSCREEN`) | fenêtré, sans bordure, exclusif (INFÉRÉ par `Fake`/`Real`) ; application différée au début de la trame suivante | **jamais appliqué** : la requête est déposée sur un objet GX absent, `Resize` sans appelant | matrice D-0170 : un run par mode, avec alt-tab, réduire/restaurer ; relever `native_render_size` | aucun ; implémentation absente (M-24, M-48) |
| Résolution (4 clés) | listes de 12 tailles fenêtrées, modes de l'adaptateur en plein écran ; hors liste : 1920x1080 | sortie fixée au démarrage ; la souris seule suit S | 1080p, 1440p, 4K, une taille impaire (D-0170) ; aide `test-resolution.flag` acceptée par Thomas, construction non documentée | tout n'a été vu qu'à 1280x720 (M-48) |

### 2.4 Options de son, d'affichage, de contrôle et de réseau (octets du profil `[[1447572B8]+0x58]`)

Aucune de ces options n'a de lecteur de rendu : leurs lecteurs sont du code du moteur qui tourne tel quel en natif. Ce qui peut différer en natif :
l'ouverture et l'affichage de leurs pages (Scaleform, filtres, colorations), l'application effective de la valeur et la lecture par le moteur.

| Option (clé du jeu, nom de page) | Octet ou lecteur connu | Comment le tester | État |
|---|---|---|---|
| Son : type (`Sound.SoundType`) | octet profil `+0xB` (0, 1, 2), copié par `FUN_1407CF780` | page d'options en jeu, trois valeurs ; relever les lignes `sound_event_request` et écouter | jamais testé (AU3, AU4 ; M-30) |
| Son : musique, effets, voix (`MusicVol`/`optBGMVol`, `SeVol`/`optSEVol`, `VoiceVol`/`optVoiceVol`) | octets `+0xC`, `+0xD`, `+0xE` (0 à 10), cibles module `+0x160/+0x16C/+0x178` ; ordre INFÉRÉ | à 0 puis 10 pour chacune : musique de zone, effet d'une action, voix d'un PNJ (Gardienne du feu, `-TalkKeeper`) ; recensement `audio-census.flag` ; écoute par Thomas | jamais testé ; écoute de Thomas en natif = vérification restante (NATIVE-AUDIO-CENSUS) |
| Affichage : sang (`Disp.Blood`/`optShowBlood`) | octet `+0xF`, trois états (INFÉRÉ) | trois valeurs, combat dans une zone d'ennemis ; le sang des personnages et les décals ne sont pas dessinés en natif (R-27), donc l'écart attendu est le même à tous les états | jamais testé |
| Affichage : sous-titres (`Disp.Subtitle`/`optSubtitles`) | octet `+0x10`, défaut 1 (INFÉRÉ fort) ; lu par la page de légende `01_060_Caption` | `Run-AlphaCast.ps1 -TalkKeeper` avec la langue de test, option à on puis off ; CL3 | jamais basculé (M-11) |
| Affichage : luminosité (`Disp.Brightness`/`optBrightness`) | octet `+0x11` selon SUBTITLES §7 C, `+0xA` selon DISPLAY §8 : **contradiction à trancher** | voir ligne `Brightness` en 2.3 | voir 2.3 |
| Affichage : interface (`Disp.HUD`) | octet `+0x12` (INFÉRÉ) | HUD à on puis off au Haut-Mur : barre de vie, jauges, âmes ; page `01_000_FE` (filtres, dégradés, masques) | jamais testé |
| Contrôle : caméra G/D, H/B, vitesse (`RotLR`, `RotUD`, `RotSpeed` ; `optCameraLR/UD/Speed`) | octets de profil non nommés dans le dépôt | inverser puis pousser la souris ; vitesse à 0 puis 10 ; vérifie surtout le chemin d'entrée natif (`NativeMouseDeviceUpdate`, verrou d'entrée NATIVE-INPUT-LATCH) | non documenté |
| Contrôle : vibration (`Rumble`/`optVibration`), saut sur L3 (`JumpButtonL3`), recentrage caméra (`CameraResetUD`), direction (`CameraDirection`), verrouillage auto (`optLockOnAuto`), évitement du mur (`optCamAvoidWall`) | non documentés | à relever sur les pages `win_02_043_pc_keyconfiguration` et `win_02_041_pc_mousesetting` ; manette : aucun témoin (M-46) | non documenté |
| Réseau : cacher les signes blancs (`Network.HideWhiteSignInSignEnemyWorld`), enregistrement au classement (`optRegisterRanking`) | non documentés | jeu hors ligne : seulement l'ouverture de la page | non documenté |
| Touches, souris : `win_02_043`, `win_02_041`, `win_02_044` (saisie), `win_02_045` (liste de commandes) | pages Scaleform, grappes de transformations de couleur (146 sur `win_02_042`) | ouvrir chaque page, natif contre original : filtres d'ombre portée de type 1 et 7x7 jamais vus (M-13) | jamais ouvertes en natif (M-15) |

### 2.5 Menu principal de l'écran titre

Ce que montrent les captures du dépôt (`Native2051 menu.png`, français) : en haut à gauche « Hors ligne », version 1.15.2, version de régulation 1.35,
noms des deux extensions ; logo ; entrées **Continuer** (sélectionnée), **Charger partie**, **Nouvelle partie**, **Connexion** (grisée hors ligne),
puis un chevron vers le bas : **d'autres entrées sont sous le chevron et ne sont décrites par aucune source** (à relever en faisant défiler dans l'original) ;
pied de page de droits. Avant le menu : logos (`05_900_logo_fromsoft`, `05_901_logo_bne`, `05_902_logo_esrb`, `05_903_warn_illegalcopy`,
`05_905_logo_copyright`), film d'ouverture (197,875 s), « press any key » (`05_000_title`, `05_001_title_logo`), deux avis hors ligne.
Pages liées au titre : `05_010_ProfileSelect` (liste de chargement), `05_020_TitleInformation`, `05_040_TermOfService`, `05_031` et `05_032` (nouvelle partie).

| Élément | Comment le tester | Témoin existant | Manque |
|---|---|---|---|
| Logos et avertissements `05_900..905` | run de démarrage `Run-StartupTrace.ps1 -SteamLaunch -BootstrapMode Reference` puis `Native`, captures aux mêmes instants | démarrage des deux modes | pas de témoin par logo documenté ; en vague 2 les logos passent sur fond noir (D-0182) |
| Film d'ouverture, passage par Échap | Reference puis Native ; Échap passe le film, clic, Entrée et E non (pareil dans les deux modes, K6) | Reference/Native1250, 1252, 1256 | CR5 et lecture complète sur la durée (M-41) |
| « Press any key » puis les deux avis | `Pilot-TitleContinue.ps1` (clics à (648, 300), E, OK à (648, 428) puis (648, 451) ; le français place le OK du second avis à y 439) | tous les runs depuis 1123 | le pilote reste parfois bloqué à « press any key » (STATE) |
| Continuer | clic à (648, 525) après le délai du menu (l'original affiche son menu environ 6 s plus tard) | Native2051 / Reference2050 | — |
| Charger partie | `Pilot-LoadGameThomas.ps1`, `Pilot-LoadGameCycle.ps1` : liste à 10 portraits (128x128, FaceGen), confirmation « Commencer avec le profil sélectionné ? » (MessageBox 406011, curseur sur le dernier choix), appui de 400 ms ou touche E | Native1159, 1170, 1176-1182 ; 5 sur 5 chargements (Native1243-1247) | variantes de phantoms par Charger partie (état Ember requis : le signe d'Argo est à environ 40 m, MENUS-NATIVE-GAPS) |
| Nouvelle partie | création de personnage (`04_000` à `04_100`), puis `05_031` luminosité et `05_032` contrôles ; `Pilot-CharMakeWitness.ps1`, `Pilot-CharMakeThomas.ps1` | écrans de création Native1165-1182 | `05_031`, `05_032` jamais ouvertes ; cache-comme-bitmap de `04_030` (M-14) ; **modifie la sauvegarde** : Thomas puis copie d'une sauvegarde de test seulement |
| Connexion | grisée hors ligne ; son activation est en ligne : hors de la règle hors ligne des tests | capture seule | aucun test (jeu hors ligne) |
| Entrées sous le chevron | faire défiler dans l'original, noter les libellés, puis natif | aucun | non documenté |
| Information, conditions d'utilisation (`05_020`, `05_040`) | ouvrir la page si l'entrée existe | aucun | jamais ouvertes (M-15) |
| Langue du titre | `-TestLanguage <steam name>` (`test-language.flag`) : français, japonais, chinois simplifié, danois, russe faits ; chinois traditionnel et coréen en cours (suspens) ; italien, allemand, espagnol, portugais brésilien, polonais jamais lancés (M-01) | Native2051-2074 | voir parties 1.B et 4 |
| Quitter | fermeture propre de la fenêtre au titre ; la faute à `D12759` pendant la création est aussi dans l'original | Native1258-1262 | — |

## 3. Code natif à relire en priorité

Philosophie de la relecture (Thomas) : **même rendu que l'original, conception D3D12 propre**. Le rendu doit être
identique ; la manière de l'obtenir ne doit pas être une copie, instruction D3D11 par instruction D3D11, de ce que faisait
le moteur d'origine, quand D3D12 offre un modèle plus direct. Pour chaque zone ci-dessous : ce qui est recopié, la
question de conception à poser, et ce qui est à vérifier. Les numéros de ligne sont ceux du dépôt à la date de ce
document (branche `claude/fable-review-prep` depuis `main` 4880ef8c) ; ils glissent avec les merges.

### 3.1 Outillage clangd, ce qui a été fait

| Étape | Résultat |
|---|---|
| `scripts/Connect-EvidenceStore.ps1` puis `scripts/New-CompileCommands.ps1` | `compile_commands.json` écrit (ignoré par Git), 200 fichiers `.cpp` de `native/` ; `vswhere.exe` absent affiche un avertissement sans effet |
| `clangd --check` sur `effect_draw.cpp`, `effect_stage.cpp`, `game_ui_session.cpp`, `model_draw.cpp`, `scene_pass.cpp`, `sun_shadow_stage.cpp` | **aucun diagnostic** (le décompte « errors » imprimé en fin de sortie ne compte que des essais de refactoring « tweak … FAIL », pas des erreurs de code) |
| Plan de chaque grand fichier par client LSP (`documentSymbol`) : `game_bootstrap.cpp`, `scene_pass.cpp`, `model_draw.cpp`, `backend.cpp`, `effect_draw.cpp`, `effect_system.cpp`, `game_ui_session.cpp`, `game_ui_image_owner.cpp`, `game_ui_mesh_owner.cpp`, `post_draws.cpp`, `primitive_pass.cpp`, `sun_shadow_stage.cpp`, `tonemap_stage.cpp`, `glare_stage.cpp`, `ao_stage.cpp`, `display_stage.cpp`, `frame_graph.cpp`, `frame_plan.cpp`, `render_worker.cpp` | les longueurs de fonctions de 3.2 viennent de là |
| Références entre fichiers | non obtenues : clangd sans index d'arrière-plan ne voit que le fichier ouvert ; les appels entre fichiers ont été relevés par recherche de texte, à confirmer au besoin avec l'outil LSP de la session de relecture après `New-CompileCommands.ps1` |

Le build MSVC (`native/frame/Build-GameBootstrap.ps1`, avec ses tests unitaires) reste la barrière de validation ;
clangd ne la remplace pas (WORKFLOW, « Native code navigation »).

### 3.2 Carte du code (taille et poids)

- `native/` : environ 46 000 lignes de `.cpp` dans `frame/`, `renderer/` et `content/` (hors `world/`, `camera/`).
  `native/frame/game_bootstrap.cpp` pèse à lui seul 16 954 lignes dans un unique espace de noms anonyme (lignes 96-16944).
- Fonctions les plus longues de `game_bootstrap.cpp` (par clangd) : `PrepareNativeModelFrame` 1 274 lignes (9858-11131),
  `AdmitNativeModelCandidateBody` 651 (7088-7738), `Setup` 505 (16439-16943, les crochets), `PrepareNativeSelectedPasses` 248
  (8157-8404), `AdmitNativeSelectedObject` 229 (7764-7992), `ObserveLockedCpuMeshInputs` 212 (14335-14546),
  `CollectNativeSelectedScene` 211 (9378-9588), `CollectNativeYebisInputs` 203 (8992-9194).
- `native/renderer/scene_pass.cpp` : `SceneRenderer::RenderModels` 470 lignes (2089-2560), `RenderFrame` 262 (2820-3082),
  `NativeLightingResources` 227 (942-1169).
- La Vague 1 compte 65 fichiers drapeaux `.flag` distincts lus par le module (liste en 3.7).

### 3.3 Zone A — Là où le code recopie les opérations D3D11 une à une

Classées de la plus littérale à la moins littérale.

**A1. Effets FFX (E01-E07) : le flux d'instructions GX de l'original, capturé puis rejoué** (D-0185, D-0186)

- `native/frame/effect_draw.{h,cpp}` : le module exécute le côté dessin de l'original (299E40, slot 27, les builders 385F70,
  396F30, 39E900) contre des contextes GX natifs ; chaque instruction GX passe par les enveloppes 19202xx-1921xxx, accrochées,
  et s'écrit dans un `EffectDrawRecord` (en-tête de `effect_draw.h`, lignes 1-12 ; `Hooks` 243-277, `Execute` 279-347).
  Les objets GX (textures, états, programmes, anneau de sommets) sont remplacés par des **jetons** (`texture_magic` 'NFXT',
  `state_magic` 'NFXS', etc., `effect_draw.h`) ; l'anneau est une arène CPU derrière une table virtuelle d'anneau.
- `native/renderer/effect_stage.{h,cpp}` : `EffectStage::Record` (151-318) rejoue les enregistrements en D3D12. Les états
  (mélange, profondeur, rastérisation, échantillonneur) viennent de **tables de descripteurs D3D11** de la banque d'états GX
  (`effect_stage.cpp` 1-86 : `base_blends`, `independent_blends`, `twin_blends`, `FindBlend`, `FindDepth`, `FindRaster`,
  `FindSampler`) ; « les énumérations D3D12 ont les valeurs D3D11 ».
- Imitations de sémantique D3D11 à relire : compteur caché d'un UAV structuré pour reproduire `CSSetUnorderedAccessViews` avec
  compte initial (`effect_stage.h` ~122, `effect_stage.cpp` ~220, `Counter` 402-418) ; effacement d'un UAV structuré par mots de
  32 bits parce que D3D12 refuse `ClearUnorderedAccessView` sur une vue structurée (`effect_stage.cpp` ~241-249) ; topologie
  convertie « comme 1929BA0 » ; 252 sites de l'affichage d'effet redirigés (D-0173, `effect_system.cpp` `RedirectDisplay`
  224-292).
- Question de conception : le flux de commandes du jeu est le contrat d'entrée, mais le rejeu pourrait déclarer ses
  dépendances (lectures/écritures des cibles, de la copie de scène, de la profondeur) au graphe d'images au lieu de rester un
  rejeu séquentiel. Vérifier que chaque comportement implicite de D3D11 est explicite et testé : lecture hors limites des
  CBV, ressources non liées valant zéro, ordre implicite des écritures UAV, compte initial des compteurs, `DrawIndirect`
  (arguments écrits par la GPU) et barrières UAV entre `Dispatch` successifs.
- Ouvert côté données (voir 1) : E03 (particules à modèle), familles sorts et miracles, passes 0x20/0x21 jamais rencontrées.

**A2. Signature racine des modèles : le modèle de slots D3D11 par étape**

- `native/renderer/model_draw.cpp` 17-60 et `BindDevice` 255-316, `model_draw.h` 124-150 : par étape (VS, PS, HS, DS) une table
  de 14 CBV, 128 SRV et un jeu de 16 échantillonneurs, « slot = registre » ; 23 CBV racine pour le modèle (4+23 paramètres) et 7
  de plus pour la tessellation, soit **64 des 64 DWORD** de racine au maximum (`static_assert` ligne 34) ; version de signature
  racine **1.0**, descripteurs volatils ; les CBV « bornés » (VS b5, b6, b7, b12, PS b4) restent des CBV descripteurs parce que
  le shader les indexe dynamiquement et que « D3D11 renvoie zéro hors limites, une CBV racine non » (`model_draw.h` 137) ; les
  tables sont remplies d'un modèle nul même quand le programme ne lit pas le registre.
- Les shaders sont les DXBC SM 5.0 de l'original, sans recompilation (`native/content/shader_bank.cpp`) ; pas de DXIL, pas de
  liaison libre. Choix défendable pour un rendu identique, mais c'est lui qui impose les tables par étape.
- Question de conception : un budget de racine plein (64/64) en tessellation et aucun repli ; descripteurs globaux seulement si
  `ResourceBindingTier >= 3` (`c.tier3`), sinon chemin « tables héritées » par dessin (`legacy-tables.flag`). À vérifier : le
  comportement sur un GPU Tier 1/2, et que la signature 1.0 volatile n'interdit pas des optimisations de pilote évidentes.
- États figés recopiés : banque de 16 échantillonneurs D3D11 par index (`native/content/sampler_bank.h` 48-67, valeurs
  `initial_values_` ligne par ligne, profils de qualité de texture `profiles_` {1,1},{2,1},{3,4},{3,8},{3,16}) ; conversion
  D3D11 vers D3D12 des états de rastérisation (`native/frame/shadow_state.h` ~51, `model_draw.cpp` `AppendBlend`/
  `AppendDepthStencil` 328-337).

**A3. Interface Scaleform : le HAL D3D11 de l'original transcrit fonction par fonction**

- `native/frame/game_ui_session.cpp` 531-720 : `PushRenderTarget` (1BDCBD0), `PopRenderTarget` (1BDBB70), `PushFilters`
  (1BDC4E0), `PopFilters` (1BDB800), la boucle de passes de filtre (1F7B930), la colle des uniformes (1F778D0), le planificateur
  de passes (1F75440), le quad de filtre (HAL+0x3B218) sont transcrits, avec des « cibles de paquet » à la place des cibles
  D3D11 ; la partie GPU de l'original (1F80640) n'est pas exécutée. Échantillonneurs « biais -0,75, LOD 0..FLT_MAX »
  (`game_ui_session.cpp` ~693).
- `native/frame/game_ui_image_owner.cpp`, `game_ui_mesh_owner.cpp`, `game_ui_shaders.cpp`, `native/renderer/primitive_pass.cpp`
  (322 lignes) : propriétaires natifs des images et maillages de l'interface, passe primitive ordonnée.
- Non témoins : filtre de type 1 et branche 7x7 « pas encore vus à l'écran » (MENUS-NATIVE-GAPS), cache-comme-bitmap
  derrière `ui-cache.flag` **désactivé par défaut** (`game_bootstrap.cpp` ~15030, page 04_030 non validée), cibles hors écran.
- Question de conception : le HAL transcrit garde la logique de piles de cibles de l'original, pertinente tant que l'entrée est
  le code de Scaleform ; vérifier qu'aucune cible n'est conservée sans propriétaire de durée de vie (retrait par jeton).

**A4. YEBIS : la bibliothèque hébergée contre des objets D3D11 « sentinelles », et ses passes réécrites à l'identique**

- Hébergement (opt-in `yebis-host.flag`, enregistrement seul, `game_bootstrap.cpp` 8573-8870) : la bibliothèque est ouverte
  avec un contexte D3D11 factice, les appels D3D11 restants tombent sur des sentinelles et sont journalisés ; surfaces et
  tampons dynamiques de la bibliothèque détournés (`NativeYebisSurfaceUpload`, `NativeYebisSurfaceCopy`,
  `NativeYebisBufferStorage`, emplacements de la table `IGPUSurface` 142B66AE8) ; supports de formats « transcrits de
  D3D11_FORMAT_SUPPORT » (`NativeYebisFormatSupport`).
- Passes natives (par défaut) : `native/renderer/tonemap_stage`, `glare_stage`, `glow_stage`, `grading_stage`, `display_stage`,
  `post_draws.{h,cpp}` (« le dessin des programmes 2D de la bibliothèque comme l'original l'émet » : trois sommets, `$Globals`
  en b0, jusqu'à six textures, échantillonneurs « comme D3D11 les donnait »), et `native/world/yebis_*.cpp` (tonemap, glare,
  shafts, dof, motionblur, blocs, pins). Leur égalité est établie bit à bit sur les entrées du témoin (W10-NATIVE-PASSES).
- À vérifier : que l'hôte `yebis-host.flag` ne soit jamais requis par une passe native par défaut ; le code de capture
  `CaptureYebisDraw` (`game_bootstrap.cpp` 3516-3599, copies de mise en scène sur le contexte D3D11 propre de la bibliothèque)
  n'est atteint qu'avec ses drapeaux de capture ; les relayouts qui font `Drain` (3.5).

**A5. Observateurs de l'original dans le module (mode Reference seulement)**

- `game_bootstrap.cpp` inclut encore `<d3d11.h>` et `<d3d11shader.h>` (lignes 93-94). Ils servent : à `D3DReflect` (inspecteur
  CPU de bytecode, 13236-13255, 16006-16008) ; aux observateurs de l'original (2825-2900 : `ObserveCreateVertexShader`,
  `ReferenceAtlasDraw`, `ReferenceAtlasDrawEntries` qui **crée un périphérique D3D11 matériel de passage** pour lire les entrées
  de table virtuelle, 2885-2897) ; à `reference_gx_uav` et à l'inspection des banques d'états (3162-3200) ; à `CaptureYebisDraw`.
- À vérifier (mandat : aucun périphérique D3D11 en production) : que `ReferenceAtlasDrawEntries` ne soit atteint que sous
  `cascade-observe.flag` **et** `Mode::Reference` (appel à 16601, dans le bloc ouvert à 16536 `if(control->mode==Reference)`
  confirmé par comptage d'accolades : le bloc ouvert à 16536 reste ouvert au moins jusqu'à 16640), que le lanceur de jeu (`tools/Launch-DS3-DX12.ps1`, mode `native`) n'atteigne aucun de ces
  chemins, et qu'aucun `ID3D11*` ne soit instancié en mode natif.

**A6. Constantes à la D3D11**

- Les registres de constantes (VS b0, b8, b9 ; PS b8, b9 ; blocs de trame par passe, `NativeModelFrameRoot`,
  `model_draw.h` 124-133) sont recopiés par charge utile à chaque vue (`NativeModelConstantUploads::Address`, tableau à adressage
  ouvert) : c'est un équivalent du `Map(WRITE_DISCARD)` par dessin, amorti. Vérifier la durée de vie (tampon rendu au jeton de
  fin de trame) et l'alignement 256 octets.

### 3.4 Zone B — Les parties déjà propres en D3D12 (à confirmer, pas à réécrire)

- Cadre de graphe : `native/renderer/frame_plan.cpp` (planification), `frame_graph.cpp` (`PlanFrameBarriers`,
  `PlanFramePlacement`), `frame_graph_shadow.cpp` ; par défaut les barrières viennent du graphe (D-0171,
  `hand-barriers.flag` pour l'ancien mode). **À vérifier** : `scene_pass.cpp` garde 127 appels `Transition(`/`ResourceBarrier(` écrits à
  la main (`scene_pass.cpp` 86-102) ; `Suppress(resource)` les neutralise pour les ressources que le graphe pilote, les autres
  gardent l'état « par convention ». Chercher une ressource ni déclarée ni suppressée dont l'état d'entrée serait faux.
- `native/renderer/render_worker.cpp` (D-0168) : le thread de rendu consomme les enregistrements ordonnés ; `Quiesce`,
  `RethrowFault`, `WaitIdle`. À vérifier : arrêt et fautes (`QuarantineToProcessExit` des étapes laisse volontairement des
  ressources en vie jusqu'à la fin du processus).
- `native/renderer/backend.cpp` : `DeviceQueues::Drain/Signal`, `FramePresenter::Open` (flip discard, pas d'indicateur de
  tearing, `DXGI_MWA_NO_ALT_ENTER`), format de sauvegarde fixe `R8G8B8A8_UNORM`, profondeur `D32_FLOAT`.
- Géométrie statique en mémoire DEFAULT, flux dynamiques en tampons mutualisés (`legacy-upload-geometry.flag`,
  `legacy-dynamic-streams.flag` pour l'ancien comportement).

### 3.5 Zone C — Écarts entre le modèle visé (revue de conception D3D12) et le code actuel

Source : [D3D12-DESIGN-REVIEW](D3D12-DESIGN-REVIEW.md) (R1 à R16), relue contre le code.

| Réprise | État dans le code relu | Constat à vérifier |
|---|---|---|
| R1 images en vol | présent, **désactivé par défaut** : `SetFrameSlots(in_flight?2:1)` lu depuis `frames-in-flight.flag` (`game_bootstrap.cpp` ~15013-15016) ; commit 779d4bb7 « M5 : pas de gain à S1, un seul créneau par défaut jusqu'à RD12 » | Le lanceur de Thomas (`tools/Launch-DS3-DX12.ps1`) ne pose que `measure.flag` et `no-debug-layer.flag` : **un créneau**, et `scene_pass.cpp` 2550/2626 fait alors `Drain` à chaque image (`frames_.Slots()==1?gpu_.Drain(...)`) |
| R14 présentation | **désactivée par défaut** : `waitable-present.flag` donne 3 tampons et l'objet d'attente ; sinon 2 tampons ; le pacer est actif par défaut (`legacy-present.flag` pour l'ancien) | même constat : la configuration livrée n'est pas celle des tests à deux créneaux |
| R2 / R3 / R5 arène, retrait par jeton, ingestion groupée | fait pour les constantes (C4-1b/C4-2a), retrait par jeton ; `game_texture_ingress.cpp` 231 garde `SubmitIngress` + `Drain` à la fermeture | relire `Drain` hors fermeture |
| R4 géométrie en DEFAULT | fait (défaut), ancien chemin derrière `legacy-upload-geometry.flag` | — |
| R6 / R7 / R9 / R8a | fait en phase C (D-0164) : tas globaux si Tier 3, CBV racine, PSO à l'admission, dessins persistants (`no-persistent-draws.flag`) | Tier < 3 : chemin hérité |
| R10 graphe | pilote les barrières par défaut (D-0171) ; 127 transitions manuelles encore là (voir 3.4) | idem |
| R11 alias de cibles transitoires | **absent** : aucun `CreatePlacedResource`/`CreateHeap` dans `native/` (recherche de texte) ; `PlanFramePlacement` existe mais le placement n'est pas exécuté | mémoire de cibles par taille jusqu'à la fermeture du périphérique ; à mentionner, pas forcément à corriger |
| R12 une seule liste | partiel (INFÉRÉ de la lecture) : les vues hors écran et Bink continuent la liste de la trame (`cD3`, commentaire de `RenderModels`), mais `Drain(WaitMoviePlanes)` (1926) et `Drain(WaitFixture)` restent | relire |
| R13 thread de rendu | fait (D-0168) | — |
| R15 diagnostics hors du chemin | lecture arrière toutes les 120 images et sur demande ; `measure.flag` les coupe | le lanceur pose `measure.flag` |
| R16 files multiples | absent | hors périmètre Vague 1 |

**Attentes du CPU sur le GPU encore présentes dans le chemin de trame** (à lire : `Drain` = attente complète) :
`scene_pass.cpp` 1314 (`BindNativeMaskViews`, à chaque début de passe de masque/AO), 1344 et 1357 (tables AO et ombre solaire),
1791 et 2132 (recréation de la profondeur quand le pochoir change), 1926 (plans de film), 2391 / 2415 / 2433 / 2466 / 2471 / 2477
(changement de disposition du flou de profondeur, de la lueur, du flou de mouvement, du grading : « un relayout fait un `Drain` »),
2550 / 2626 (fin de trame à un créneau). Conséquence : un changement d'objectif de caméra, de zone ou de résolution bloque la
trame jusqu'à la fin du GPU. Une conception D3D12 propre retire l'ancienne ressource au jeton de trame au lieu d'attendre.

### 3.6 Zone D — Shaders propres au module et passes sans équivalent exact

Compilés à l'exécution par `D3DCompile` (dépendance à `d3dcompiler_47`, hors du cache de shaders prévu en Vague 2, D-0172/D-0179) :
composition matérielle (`scene_pass.cpp` ~403), **recouvrement de fondu** (`EncodeFadeOverlay` 410-442, écrit, **non validé**,
paramètre de plan bit 8, attend la sonde `fade-observe.flag`), composition hors écran (~457), résolution et demi-taille de
vélocité (1523, 1538), composition FaceGen (`game_texture_ingress.cpp` 111), témoins du `backend.cpp` (462, 513). Pour la
vélocité, la décision D-0162 réécrit la résolution : la fidélité à l'original est à relire (une unité demi-flottante sur 188
texels, W10-NATIVE-PASSES §10).

### 3.7 Zone E — Chemins alternatifs et drapeaux (code mort ou diagnostic laissé actif)

65 drapeaux `.flag` distincts sont lus. Catégories à contrôler :

- **Retours d'anciens chemins** (à supprimer un jour, à vérifier qu'ils ne sont pas atteints par défaut) : `legacy-tables`,
  `legacy-upload-geometry`, `legacy-dynamic-streams`, `legacy-present`, `hand-barriers`, `inline-render`, `inline-prepare`,
  `rebuild-collect`, `rebuild-prepare`, `no-persistent-draws`, `no-model-records`.
- **Par défaut désactivés** (fonctions écrites mais non livrées) : `native-tess` (tessellation, aucun dessin `_PN` témoin en jeu),
  `yebis-host`, `ui-cache`, `frames-in-flight`, `waitable-present`, `ffx-model-particles` (E03, fautif).
- **Coupe-circuit** (désactivent une fonction livrée) : `no-yebis`, `no-yebis-<passe>`, `no-sun-shadow`, `no-ao`, `no-ao-resources`,
  `no-ao-passes`, `no-glow`, `no-gamma`, `no-ffx-draw`, `no-ffx-gpu`, `no-ffx-effect-copy`, `no-native-effects`, `no-forward-gb5`.
- **Observateurs** (écrivent des lignes de journal, lisent l'original) : `light-observe`, `cascade-observe`, `shadow-observe`,
  `ffx-observe`, `ffx-record`, `ffx-draw-observe`, `yebis-observe`, `yebis-hal-observe`, `yebis-draws-capture`, `yebis-tonemap-capture`,
  `world-pictures`, `caption-observe`, `fade-observe`, `audio-census`, `sample-profile`, `test-language`, `brightness-override`.
- Commande : `Select-String` sur `ModuleFlag(L"...")` et sur `GetFileAttributesW(... L"....flag")` (deux styles de lecture
  coexistent ; certains drapeaux sont lus une fois par processus avec `static const bool`).

### 3.8 Ordre de lecture proposé

1. `native/renderer/scene_pass.cpp` `RenderModels` 2089-2560 puis `RenderFrame` 2820-3082 (le cœur de la trame et ses `Drain`).
2. `native/renderer/model_draw.cpp` + `model_draw.h` (A2 : modèle de liaison, constantes, tessellation).
3. `native/frame/effect_draw.cpp` + `native/renderer/effect_stage.cpp` (A1 : la copie la plus littérale du D3D11).
4. `native/frame/game_ui_session.cpp` 531-720 (A3 : le HAL Scaleform transcrit).
5. `native/frame/game_bootstrap.cpp` : `Setup` 16439-16943 (tous les crochets et ce qui n'est actif qu'en Reference), puis
   `PrepareNativeModelFrame` 9858-11131 et `UpdateNativeQualityObject` 7999-8080.
6. `native/renderer/frame_graph*.cpp` et `backend.cpp` (B).
7. Les passes YEBIS natives et `post_draws.cpp` (A4), avec leurs tests `*_test.cpp` (`yebis_*_test.cpp`, `*_stage_test.cpp`).

## 4. EN SUSPENS : ce que le pilote est en train de tester (données pas encore connues, marquées inconnu)

Ne pas classer ces points dans les écarts avant que le pilote (fil « Tests des menus en langues ») ait consigné ses résultats dans STATE.
Pendant ce temps le PC est occupé par ses runs live : **aucun test à lancer pour la revue tant qu'il n'a pas rendu la main**.

| Élément | Source | État |
|---|---|---|
| Chinois traditionnel (`tchinese`, id 23, ordinal 18) puis coréen (`koreana`, id 14, ordinal 19, police Hangul) : paires live Reference/Native 2075-2078, début 05 h 08 heure de Paris | message du pilote 2026-10-18 03:08 UTC (relayé) ; MENU-TESTS-PREP §7 (hors main) | **inconnu** |
| Le reste de MENUS-NATIVE-GAPS : pages jamais ouvertes, options, Inventaire, famille du feu de camp, etc. | STATE (« then MENUS-NATIVE-GAPS ») ; relais du coordinateur | **inconnu** |
| Langues latines jamais lancées (italien, allemand, espagnol, portugais brésilien, polonais) et `spaAR` : décision de couverture à prendre | M-01, M-02 | **inconnu** (non planifié dans les sources lues) |
| Corrections éventuelles de la silhouette d'Albert par le pilote après ce document | R-40 | les cycles du pilote continuent : l'état final est **inconnu** |

## 5. Contradictions entre sources et limites de ce document

| # | Contradiction ou limite | Sources | À trancher |
|---|---|---|---|
| 1 | STATE (« What runs natively ») compte les décals parmi ce qui tourne en natif ; le code écrit `native_decal_draw_ready:false` et remplace le consommateur 19B690/198910 par un refus ; seules les requêtes CPU sont natives (R-27) | STATE ; `game_bootstrap.cpp` 1825-1831 | corriger STATE ou le code |
| 2 | STATE décrit W8/W9 « inertes » ; D-0180 a passé SSAO et masque d'ombre solaire en vague 1 et ils tournent ; reste inerte l'atlas des ombres locales (D-02) | STATE ; decisions/LOG D-0180 ; SUN-SHADOWS-WAVE1 | mettre STATE à jour |
| 3 | POM admis (COVERAGE-MATRIX, antérieur) ou refusé (FOLIAGE-F1 et code : `VFC_PomLightDir` non produit) (R-25) | I-0068 COVERAGE-MATRIX ligne 4 ; FOLIAGE-F1-NATIVE ; `game_bootstrap.cpp` 10408 | lire `native_vmodel_param_unproduced` à m31 et m37 |
| 4 | STATE cite « frames in flight » comme fonctionnel ; le défaut livré est un seul créneau (C-01) | STATE ; commit 779d4bb7 | dire quel défaut est livré |
| 5 | L'octet de luminosité en jeu : `+0xA` (DISPLAY-SWAPCHAIN-NATIVE §8, feuillet G §6) ou `+0x11` (SUBTITLES-DIALOGUE-NATIVE §7 C) | I-0068, I-0066, I-0067 | une lecture du profil dans l'original |
| 6 | XInput : MENUS-MAP dit à la fois que le jeu l'utilise (ligne 2874) et qu'il ne l'importe pas (ligne 4790) ; aucun test manette | MENUS-MAP | une lecture de la table d'imports |
| 7 | UI-INGRESS déclare 12 langues, 13 ordinaux sont sélectionnables | UI-INGRESS ; FONTS-LANGUAGES-NATIVE §1 | réconcilier (M-03) |
| 8 | MENU-TESTS-PREP (source de l'audit des pages, de la règle de résolution et des réponses de Thomas du 2026-10-16) n'est pas dans `main` ; il place le mode sans bordure dans l'ordre d'implémentation alors que D-0170 le renvoie à plus tard | branche `claude/relaxed-almeida-957f40` | fusionner ou citer le fichier ; trancher le sans bordure |
| 9 | FADE-PLATES-NATIVE et STATE disent « unvalidated » ; le pilote conclut « rien à dessiner, comme l'original » (Coord. 18/10) | FADE-PLATES-NATIVE §5 ; relais | mettre les documents à jour (M-42) |
| 10 | Le deuxième ensemble de lignes « FL » : FL de FONTS-LANGUAGES-NATIVE et FL de FADE-PLATES-NATIVE portent le même préfixe pour deux choses différentes | les deux notes | lire avec le nom du fichier |

**Limites de ce document.**
- Les tableaux 1.A et 1.B ont été dressés par deux relecteurs de sources (un par périmètre) puis **vérifiés par sondage seulement** : la contradiction
  des décals (R-27) et l'absence des passes d'eau (R-50 : `model_programs.cpp` 38-41 ne laisse passer que les passes 0, 1, 2, 6, 7) ont été relues dans le code ;
  les autres lignes citent leurs sources, à confirmer au besoin.
- clangd n'a pas fourni de références entre fichiers (pas d'index d'arrière-plan) : voir 3.1.
- La correspondance des clés de l'exécutable avec les noms des pages d'options, les libellés exacts des pages et les entrées du menu titre sous le chevron
  ne sont dans aucune source ; ils sont marqués non documentés ou INFÉRÉS.
- Aucun run n'a été lancé : chaque colonne « Quoi vérifier » est un test à faire, pas un résultat.

## 6. Ordre de relecture proposé

1. Parties 5 et 1.C : fixer la base (quelle configuration est livrée, quelles lignes de STATE sont dépassées), avant de juger un écart.
2. Partie 3.8 : lire le code dans l'ordre indiqué (cœur de trame, liaison des modèles, effets, HAL Scaleform, crochets de `Setup`).
3. Écarts à plus fort effet visible, dans cet ordre de gain probable : eau et réflexion absentes (R-50, R-51), décals (R-27), neige de surface (R-24),
   POM (R-25), E03 (R-13), silhouette d'Albert et phantoms (R-40 à R-45), sondes de lumière des personnages (R-04), roues et croix de m31 (R-01, R-49),
   antialiasing (M-27), résolution et plein écran (M-48).
4. Partie 2 : rebalayage des options (M-26) une fois les zones relues ; le jeu de zones témoins est celui de la partie 0.
5. Après le retour du pilote : relire la partie 4 et reclasser.
