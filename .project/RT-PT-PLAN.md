# Rekindled en full RT/PT : le plan

> **Document historique** (D-0291, 2026-11-15) : le plan d'étude du 30 octobre, gardé tel quel et hors de la lecture par défaut. Ce qu'il reste à faire est dans la [ROADMAP](ROADMAP.md) (chantiers 7, 11 et 12) et les décisions en vigueur dans [IN-FORCE](../decisions/IN-FORCE.md) ; ce texte ne les remplace pas.

> Copie dans le dépôt (D-0214) de l'étude du fil « plan RT/PT » (fichier partagé RT-PT-PLAN.md), validée par Thomas le 30 octobre 2026 à 03:02 (Paris) : section 13, point 1 (a) et point 2 (option B) ; le point 3 pris comme suivi, sans réponse explicite. Décision : D-0235 ([decisions/LOG](../decisions/LOG.md)). **Modifié le 30 octobre à 04:55 (D-0237)** : option A (le raster d'éclairage supprimé dès que sa version tracée marche et que Thomas l'a acceptée, rien de caché ; l'original D3D11 sert de référence) et les ombres tracées (R5) juste après R2. Le ray tracing d'Elden Ring n'est pas un modèle par sa qualité ni son fonctionnement ; sa place dans l'architecture du moteur peut être observée (Thomas, 30 octobre 04:20, D-0236). **Modifié le 30 octobre à 05:25 (D-0238)** : RTX Mega Geometry n'est plus prévu d'office ; R2 se fait en DXR standard, la construction des structures de rayons est mesurée, et Mega Geometry ne revient que si ce coût pèse (section 4).

30 octobre 2026, 02:30 (Paris) · étude en lecture seule (aucun code, aucun lancement du jeu) · à valider par Thomas avant tout chantier · demandes de Thomas du 30 octobre 01:21-01:28 : remplacer les réglages graphiques par de meilleurs, ombres, éclairage et reflets en RT/PT par niveaux comme Cyberpunk, DLSS à la place de l'antialiasing, RTX Mega Geometry, plus du tout d'éclairage raster (« full RT/PT »), Gears of War: E-Day en exemple, textures upscalées plus tard et pilotables par Claude.

Étiquettes : **vérifié** (lu dans le code ou une source citée), **estimé** (un ordre de grandeur, pas une mesure), **à vérifier**.

## 0. En bref

1. **Faisable, et le natif part avec une bonne base.** La géométrie des cartes est déjà en mémoire vidéo de façon permanente (un tampon par modèle, en triangles, positions en float3) : c'est exactement ce qu'il faut pour construire les structures de ray tracing. Les lumières (soleil, points, spots, lumières d'effets) sont déjà lues par le natif. Les vecteurs de mouvement existent pour le décor, les personnages et la caméra. Une fiche DLSS complète existe déjà (UPSCALER-DLSS-READY, dix commits décrits, jamais écrits).
2. **Ce qui manque est gros mais connu** : le socle DirectX moderne (le natif crée aujourd'hui un appareil de niveau 11_0, sans DXR, avec les shaders compilés à l'ancienne), le skinning des personnages en compute (aujourd'hui fait dans les shaders de sommets), la liste des objets hors de l'écran (le natif ne voit que ce qui est dans le champ de la caméra), un tableau des matériaux lisible par les rayons, le débruitage.
3. **« Full RT/PT » veut dire : tout l'éclairage tracé, rien d'éclairé en raster.** Même Gears of War: E-Day, Cyberpunk en path tracing ou Alan Wake 2 dessinent encore en raster ce que la caméra voit en premier (le G-buffer), l'interface, les particules et le post-traitement. C'est moins cher et l'image est identique. Ce qui disparaît, c'est tout l'éclairage raster : cascades d'ombres, atlas d'ombres locales, SSAO, reflet plan de l'eau, cubemaps, éclairage ambiant cuit.
4. **Ordre proposé : le socle, puis DLSS, puis la scène en rayons, puis les ombres, les reflets, l'illumination globale, et enfin le path tracing complet.** Environ 12 à 15 chantiers. DLSS vient tôt parce que le RT en a besoin (Ray Reconstruction pour débruiter, la génération d'images pour la fluidité).
5. **Coût estimé sur ta 4070 Ti Super** : en 1440p, path tracing avec DLSS Qualité ou Équilibré et Ray Reconstruction, autour de 55 à 85 images/s, 100 à 150 avec la génération d'images. En 1080p, 60 images/s tenues sans génération d'images. À mesurer, pas promis.
6. **Mega Geometry** : utilisable sur ta carte, déjà dans Gears E-Day et Alan Wake 2, mais sa version DirectX standard n'est qu'une préversion depuis le 28 octobre. Le gain sur la géométrie d'origine de DS3, peu dense, sera modeste ; il sert surtout à mettre à jour vite des milliers d'objets et paiera vraiment avec des modèles HD. ~~Je propose de bâtir la couche de structures de rayons dès le départ pour Mega Geometry, avec un chemin DXR standard à côté pour déboguer et comparer.~~ Remplacé par D-0238 : DXR standard d'abord, Mega Geometry seulement si la mesure le demande.
7. **Ce que ça change à la ligne d'arrivée** : les plantages, les effets manquants et les défauts de matériaux restent à faire (ils existent dans n'importe quel moteur) ; tout ce qui reproduit l'éclairage raster de l'original s'arrête. La règle « NVIDIA en dernier » (D-0228) et « le raster reste la référence » (ROADMAP) sont à remplacer par une décision.
8. **Deux décisions te reviennent** (section 13) : l'ordre global (RT tout de suite ou après les plantages), et le sort du raster existant (le supprimer ou le garder caché comme référence le temps de la transition ; je recommande de le garder caché puis de le supprimer catégorie par catégorie).

## 1. Ce que « sans raster » veut dire, avec Gears of War: E-Day en référence

**Vérifié** (sources en fin de page) : Gears of War: E-Day (The Coalition, Unreal Engine 5.6, sorti le 6 octobre 2026) n'a **aucun chemin d'éclairage raster** : illumination globale et reflets par Lumen matériel, ombres des lumières locales par MegaLights et par ombres tracées, aucune option pour couper le ray tracing, une carte avec ray tracing matériel obligatoire. Il propose aussi RTX Mega Geometry en option (NVIDIA seulement, au moins 12 Go de mémoire vidéo). Il n'a pas de mode path tracing ni de Ray Reconstruction.

Ce qu'E-Day, Cyberpunk en path tracing et Alan Wake 2 gardent en raster, et que Rekindled gardera aussi :

| Reste en raster | Pourquoi | Ce que ça devient chez nous |
|---|---|---|
| **La vue de la caméra (G-buffer)** : quelle surface est vue à chaque pixel, sa couleur, sa normale | Tracer le premier rayon donne la même image en plus cher. Chez nous c'est un vrai avantage : le G-buffer est rempli par les ~4 000 shaders de matériaux de DS3 lui-même, donc le premier impact est exact pour chaque matériau | Gardé tel quel ; seul ce qui l'éclaire change |
| **L'interface, les menus, les polices, les films** | De la 2D | Inchangé |
| **Les particules et effets FXR, les surfaces transparentes** (voiles, verre, surface de l'eau) | Les rayons gèrent mal des milliers de quads transparents ; tous les jeux les composent en raster | Dessinés en raster, mais **éclairés par la lumière tracée** (cache de radiance) au lieu des lumières raster ; l'eau garde sa surface raster, son reflet et sa réfraction deviennent tracés |
| **Le post-traitement** : tone map, glow, profondeur de champ, flou de mouvement, étalonnage | Travail sur l'image finie | Gardé (et modernisable plus tard, D-0175) |
| **Le brouillard volumétrique et les rayons de lumière** | Volumes à l'écran | Gardés, mais ombrés par la visibilité tracée |
| **Le ciel, les décalques** | Fond et peinture sur le G-buffer | Gardés ; le ciel sert aussi de lumière d'environnement aux rayons |

Ce qui **disparaît** : les cascades d'ombres du soleil et leur atlas, l'atlas des ombres locales (I-0106), le SSAO, le reflet plan de l'eau (sa vue miroir à 3 ms, D-0189), les cubemaps d'environnement, l'éclairage ambiant cuit (le canal GB4 « ambient irradiance » ; **à vérifier** : il vient des lightmaps des pièces de carte), et l'accumulation des lumières en différé (les programmes `GXLightAcc_*`).

Conséquence à accepter : **plus aucune carte sans ray tracing matériel** (GTX et plus anciennes exclues, comme E-Day). Il faut une RTX 20 ou plus, une Radeon RX 6000 ou plus, ou une Intel Arc.

## 2. Ce que le natif fournit déjà (lu dans le code)

| Besoin du RT/PT ou de DLSS | État dans le natif | Verdict |
|---|---|---|
| Géométrie des cartes | Un tampon par modèle FLVER en mémoire vidéo, permanent, avec une clé stable (`AdmitNativeStaticBuffer`, `14_scene_step_and_storage.inc:245`) ; triangles seulement, index 16 ou 32 bits ; positions float3 | **Prêt** pour des BLAS |
| Personnages animés | Skinning fait dans le shader de sommets, palettes d'os calculées sur le CPU et gardées pour l'image courante et la précédente (`05_selected_scene.inc:298`) | Entrées prêtes, **le skinning en compute manque** |
| Tissus, objets physiques | Réécrits par le CPU à chaque image dans des tampons à part | **Mise à jour par image** à écrire |
| Herbe et vent | Calculés dans le shader de sommets ; l'herbe instanciée de la Cité Enclavée (m51) est refusée par le natif | **À faire** (et m51 manque déjà en raster) |
| Objets hors écran | La liste est coupée au champ de la caméra (`WalkNativeLayers`) ; les ombres et le reflet de l'eau ajoutent leurs propres marches ; aucune liste du monde chargé | **Manque** : reflets et rebonds en ont besoin |
| Transparence découpée (feuillages) | `g_AlphaRef` par matériau, classe 4 | Données prêtes, **any-hit à écrire** |
| Matériaux | G-buffer connu en partie : normale, albédo, réflectance, ambiant, émissif ; le canal de brillance (roughness) **pas encore identifié** | **Tableau des matériaux pour les rayons à faire** |
| Lumières | Soleil (GPARAM), points (position, couleur, rayon), spots (I-0106), lumières d'effets, grille de lumières (I-0107, jusqu'à 1 024 points et 1 024 spots) | **Prêt** côté données ; une liste unique à faire |
| Vecteurs de mouvement | R16G16_FLOAT, décor, personnages (avec la pose précédente) et caméra ; rien pour les effets, l'eau et les rangs forward | **Bon pour l'opaque**, masque réactif à faire |
| Jitter de caméra | Absent, mais un seul point d'injection propre (`producers.cpp:33-60`), déjà conçu dans UPSCALER-DLSS-READY §3.2 | **Conçu, pas écrit** |
| Résolution de rendu | Une seule taille pour tout (pas de « render scale ») | **À faire** (fiche DLSS §3.1) |
| Image HDR linéaire | `R16G16B16A16_FLOAT` jusqu'au tone map | **Prêt** pour DLSS |
| Profondeur | D32 + stencil, Z standard | Prêt |
| Graphe de frame et barrières | Ajouter une passe compute ou RT entre G-buffer et éclairage = une déclaration et un bit de clé, comme `light_organize` | **Prêt** |
| Appareil D3D12 | Niveau 11_0, `ID3D12Device` simple, liste de commandes simple, runtime système, pas d'Agility SDK, shaders DXBC compilés en SM 5.0 | **Manque** : niveau 12_2, Device5+, CommandList4+, DXR, dxc |
| Elden Ring, étudié mais pas copié (D-0236) | Le moteur D3D12 d'Elden Ring a des pipelines de ray tracing (ombres et occlusion depuis la mise à jour 1.09 ; ER-PIPELINE-STUDY §8) | **À lire** : comment FromSoftware gère les structures de rayons pour les pièces de carte et les personnages |

Ordre de grandeur de la scène (**vérifié** dans les fiches) : au Haut mur de Lothric, 126 modèles, 1 001 maillages, environ 1 060 draws par image ; jusqu'à 2 745 rangs préparés en m30. C'est très léger pour le ray tracing moderne.

## 3. Ce qui manque, en clair

1. **Le socle DirectX moderne** : l'Agility SDK (D3D12Core livré à côté du module ; `DarkSoulsIII.exe` n'exporte pas `D3D12SDKVersion`, donc la sélection passe par `ID3D12SDKConfiguration1::CreateDeviceFactory`, **à vérifier** dans le processus du jeu), un appareil 12_2 avec DXR 1.2 (opacity micromaps et réordonnancement des rayons, accélérés en matériel sur ta RTX 40), dxc pour les shaders DXIL, une table de descripteurs « bindless » pour toutes les textures et tous les maillages.
2. **La scène vue par les rayons** : une BLAS par maillage à l'admission, le skinning des personnages en compute, la mise à jour des tissus et objets physiques, le vent, la transparence découpée, une TLAS par image qui couvre aussi ce qui est derrière la caméra.
3. **Les matériaux pour les rayons** : quand un rayon touche une surface hors écran (un reflet, un rebond), il faut savoir sa couleur sans le G-buffer : un tableau par matériau (albédo, normale, réflectance, brillance, émissif, seuil alpha). C'est le gros travail de lecture des matériaux de DS3.
4. **Les lumières pour les rayons** : une liste unique (soleil, ciel, points, spots, lumières d'effets, surfaces émissives) et l'échantillonnage de nombreuses lumières (ReSTIR, le RTXDI de NVIDIA).
5. **Le débruitage** : Ray Reconstruction de DLSS, qui remplace les débruiteurs classiques.
6. **DLSS lui-même** : les dix commits de UPSCALER-DLSS-READY (taille de rendu séparée, jitter, masque réactif, remise à zéro de l'historique, profondeur à la taille de sortie pour la profondeur de champ).

## 4. RTX Mega Geometry

**Ce que c'est** : au lieu d'une structure de rayons par objet entier, la géométrie est découpée en petits paquets de triangles (« clusters », 256 triangles au plus) construits et réutilisés par le GPU, plus une TLAS « partitionnée » qui ne reconstruit que les zones de la scène qui ont bougé. Le gain : des milliers d'objets et des millions de triangles mis à jour vite, moins de mémoire, moins de travail CPU.

**Sur ta carte** : oui. NVIDIA le supporte sur toutes les RTX ; les RTX 50 ont du matériel dédié en plus, ta RTX 40 passe par le chemin standard. En jeu : Alan Wake 2 (2025, gain mesuré d'environ 13 % et 1 Go de mémoire vidéo en moins sur une 4090, surtout dans les scènes de végétation) et Gears E-Day (en option, déconseillé à la plupart des joueurs par Wccftech, au moins 12 Go ; tu en as 16).

**Côté API** (**vérifié**) : Microsoft a publié le 28 octobre 2026 une **préversion** DirectX (Agility SDK 1.722.0-preview, Shader Model 6.10, Mode développeur de Windows obligatoire) : bonne pour développer, pas pour jouer tous les soirs. L'autre accès est l'extension NVAPI de NVIDIA, celle qu'utilise Alan Wake 2 (**à vérifier** dans le SDK : sa disponibilité publique et ses noms d'appels).

**Avec la géométrie de DS3** (texte d'origine, remplacé par la décision ci-dessous) : les modèles de 2016 sont peu denses ; le découpage en clusters apportera peu sur eux. Ce qui servirait : la TLAS partitionnée (le décor ne bouge pas, seuls les personnages et objets mobiles sont remis à jour, ce qui soulage le CPU, notre point faible d'après I-0095) et la mise à jour des personnages. Le vrai gain viendra avec des modèles et une végétation plus denses.

**Décision (D-0238, 30 octobre 05:25) : pas d'office.** Thomas ne veut pas toucher à la géométrie (pas de modèles HD). Les données de DS3 ([I-0110](../investigations/I-0110-rt-geometry-lights-study/RESULT.md) §4) : des listes de triangles en float3, quelques milliers d'instances placées par zone (2 273 au Haut mur, 4 057 à Lige-Feu), 15 à 16 millions de triangles uniques au LOD 0 par zone, des personnages mesurés à 30 000 à 42 000 triangles (de l'ordre de 100 000 estimés avec toutes les pièces d'armure). Une BLAS par maillage compactée, une mise à jour (refit) des personnages et une TLAS refaite à chaque image devraient suffire en DXR standard (estimé, à mesurer en R2). R2 mesure le temps de construction et de refit par image, la mémoire des BLAS et les changements de LOD ; Mega Geometry ne revient que si l'un d'eux pèse, derrière la même interface. Le texte qui suit est la proposition d'origine, remplacée.

**Proposition d'origine** : ta règle (14 octobre) est que Mega Geometry n'est pas une option du menu mais la base technique. Je proposais de concevoir la couche des structures de rayons sur son modèle dès le chantier R2 (clusters par maillage, partitions par bloc de carte), avec deux moteurs derrière la même interface : DXR standard (pour déboguer et comparer, et pour les cartes non NVIDIA) et Mega Geometry (NVAPI d'abord, DirectX quand il sortira de préversion), qui devient le défaut dès qu'il est mesuré au moins aussi bon.

## 5. DLSS, Ray Reconstruction, génération d'images, Reflex

- **DLSS Super Resolution** (modèle transformer de DLSS 4.5) remplace l'antialiasing de DS3. Attention à un piège de nom : le « DLAA » qui tourne aujourd'hui dans le natif est le filtre d'origine de DS3 (Directionally Localized Anti-Aliasing, un cousin du FXAA), **rien à voir avec le DLAA de NVIDIA**. Les modes proposés : DLAA (NVIDIA, pleine résolution), Qualité, Équilibré, Performance, Ultra Performance.
- **Ray Reconstruction** (DLSS 4.5, disponible sur RTX 20 à 50) débruite le RT/PT ; il exige DLSS (ou le DLAA de NVIDIA) et des guides en plus : albédo diffus, albédo spéculaire, normales et brillance, distance des impacts.
- **Génération d'images** : x2 sur ta RTX 40 (la multi-génération est réservée aux RTX 50). Elle a un intérêt particulier pour DS3 : la logique du jeu reste à 60 images/s (D-0167), et la génération donne 120 images affichées **sans toucher à l'horloge du jeu**. Elle exige Reflex et une interface séparée de la scène (le natif dessine déjà l'UI à part, c'est faisable).
- **Reflex** : Off, On, On + Boost (déjà au ROADMAP), deux frames en vol gardées (D-0187).
- **Streamline ou NGX direct** : D-0165 dit Streamline en mode manuel, DLL fournies par l'utilisateur ; la fiche DLSS proposait NGX direct parce que l'interposeur de Streamline enveloppe l'appareil (interdit par le mandat). Le mode manuel de Streamline n'enveloppe rien et donne aussi Reflex et la génération d'images : je recommande Streamline en mode manuel, conforme à D-0165.

### Les entrées de DLSS, de Ray Reconstruction et du RT (Thomas, 30 octobre 05:38-05:39 ; relu dans [UPSCALER-DLSS-READY](../investigations/I-0066-independent-native-startup/UPSCALER-DLSS-READY.md))

| Entrée | Dans le natif | Où elle se fait |
|---|---|---|
| Jitter de la caméra | Absent ; un seul point d'injection (le producteur de la projection), ses lecteurs triés en trois groupes (fiche §1.2, §3.2) | R1 |
| Matrices caméra courante et précédente | La view-projection précédente gardée par vue (`native_velocity_history`) ; les trois matrices de reprojection dérivées (fiche §3.3) | R1 |
| Motion vectors | `R16G16_FLOAT`, décor, caméra et personnages skinnés (palettes d'os courante et précédente, b5/b6) | Faits ; à compléter en R1 : les effets, l'eau, les rangs forward, les voiles ; **à vérifier** : les tissus et le vent animés dans les vertex shaders |
| Depth | D32, Z standard ; une depth à la taille de sortie pour YEBIS (fiche §3.7) | Existe ; la copie à la taille de sortie en R1 |
| Exposure | Celle de YEBIS de l'image précédente, passée à DLSS (fiche §3.6) | R1 |
| Masque reactive et transparency | Un bit de stencil posé par les draws blended (effets, forward, eau, voiles) et un petit draw natif (fiche §3.4); fait par I-0111 puis retiré par I-0116 (les modèles DLSS actuels ne le lisent pas); la couche de transparence vient avec Ray Reconstruction | R1 |
| Interface et post après l'upscale | DLSS entre la velocity (slot 12) et YEBIS (slot 13) : YEBIS, glow, UI et gamma à la taille de sortie (fiche §0.4) | R1 |
| Mip bias | `log2(rendu/sortie) - 1` sur les samplers des matériaux seulement (fiche §3.8) | R1 |
| Remise à zéro de l'historique | Un compteur d'époque sur les sept propriétaires d'état d'une image à l'autre (fiche §1.5, §3.5) | R1 |
| Guides de Ray Reconstruction : normales, albédo diffus et spéculaire, roughness, distance des impacts | Normales et albédo dans le G-buffer ; le spéculaire tiré de la réflectance ; la roughness **pas encore identifiée** ; la distance des impacts sortira des passes tracées | R3 (roughness, albédos), puis R5 et R6 (distances) |
| UI séparée de la scène, image sans HUD | L'UI est déjà dessinée à part | Pour la génération d'images, R10 (le menu, R12) |

## 6. Les réglages proposés

**État au 12 novembre 2026 (D-0273)** : les réglages livrés sont ceux de R12 ([R12-PLAN](../investigations/I-0132-rt-optimisation-zones/R12-PLAN.md), D-0266, D-0271) ; la liste par catégorie ci-dessous est la proposition du 30 octobre, gardée comme historique.

Un réglage principal **Rendu : Ray tracing / Path tracing** (pas de « Off », comme E-Day), avec des préréglages, puis par catégorie :

| Catégorie | Ray tracing (plus léger) | Path tracing (le maximum) | Niveaux proposés |
|---|---|---|---|
| **Ombres** (soleil et lumières locales) | Soleil et lumières que le jeu marque comme ombrées, 1 rayon par pixel, ombres douces selon la taille de la lumière | Toutes les lumières, échantillonnées par ReSTIR | Moyen, Élevé, Ultra |
| **Éclairage direct** (nombre de lumières) | Les lumières proches, en nombre limité | Toutes, sans limite pratique (ReSTIR) | Moyen, Élevé, Ultra |
| **Reflets** (dont l'eau) | Surfaces assez lisses seulement, demi-résolution | Toutes les surfaces, pleine résolution, plusieurs rebonds | Bas, Moyen, Élevé, Ultra |
| **Illumination globale** (remplace SSAO et ambiant cuit) | Un rebond, cache de radiance | Plusieurs rebonds (ReSTIR GI et cache de radiance) | Moyen, Élevé, Ultra |
| **Effets et transparents** | Éclairés par le cache de radiance | Pareil, en plus fin | Suit la catégorie ci-dessus |
| **DLSS** | DLAA, Qualité, Équilibré, Performance, Ultra Performance | Pareil | 5 modes |
| **Ray Reconstruction** | Oui par défaut | Oui, obligatoire | On, Off en RT seulement |
| **Génération d'images, Reflex** | Off, On ; Off, On, On + Boost | Pareil | |

**Les défauts et les règles du menu (Thomas, 10 novembre, D-0265)** : par défaut le niveau RT, un TAA (On/Off, On ; DLSS éteint par défaut une fois ce TAA écrit, le DLAA de NVIDIA reste le défaut jusque-là), Reflex et Boost éteints, la génération d'images éteinte, 60 images/s, ReSTIR GI avec le PT. Le menu (R12) n'a aucune option cachée : celles qui ne sont pas disponibles sont grisées et verrouillées. Le PT n'est disponible qu'avec un mode DLSS (il ne tourne que sur les frames que RR débruite) ; Ray Reconstruction est proposée quand un mode DLSS tourne (On par défaut) ; la génération d'images aussi, comme RR, seulement avec un mode DLSS (Thomas, 01:05 : elle ne tourne déjà que sur les frames DLSS) ; avec elle, le Off de Reflex est grisé et verrouillé, le choix restant entre On et On + Boost.

Ce qui reste des 11 réglages d'origine (inventaire I-0108) : textures (filtrage), profondeur de champ, flou de mouvement, effets, qualité de la surface de l'eau. L'antialiasing, le SSAO, les ombres, l'éclairage, les reflets et l'ombrage (SSS ; **à décider** : garder le SSS d'écran sur les personnages, comme le font la plupart des jeux en PT) sont remplacés.

## 7. Le coût attendu sur ta RTX 4070 Ti Super

**Estimé, pas mesuré.** Point de repère : Cyberpunk 2077 en path tracing (Overdrive), en 1440p sur une RTX 4070 Ti (un peu moins rapide que ta Super) : environ 27 images/s sans DLSS, 55 à 70 avec DLSS, environ 100 avec la génération d'images. DS3 est bien plus léger en géométrie et en lumières, mais le coût du path tracing dépend surtout du nombre de pixels et de rebonds, donc moins de ce que contient la scène.

| Écran | Ray tracing | Path tracing | Avec génération d'images |
|---|---|---|---|
| ASUS 1920x1080, 60 Hz | 90 à 130 images/s en DLSS Qualité | 70 à 100 en DLSS Qualité | inutile à 60 Hz |
| OLED 2560x1440, 280 Hz | 80 à 110 en DLSS Qualité | 55 à 85 en Qualité ou Équilibré | 100 à 150 (PT), 150 et plus (RT) |

Mémoire vidéo : DS3 en utilise peu ; structures de rayons, tampons de Ray Reconstruction et caches ajoutent de l'ordre de 2 à 4 Go. Tes 16 Go suffisent. Le CPU : la mise à jour des structures de rayons et le skinning s'ajoutent à un moteur déjà limité par le CPU (I-0095) ; c'est un argument pour tout faire côté GPU (la TLAS partitionnée de Mega Geometry seulement si la mesure de R2 le demande, D-0238). L'objectif d'environ 120 images/s (ROADMAP) passera par la génération d'images.

## 8. Les chantiers, dans l'ordre

| # | Chantier | Ce qu'il livre | Ce que tu vois | Taille |
|---|---|---|---|---|
| R0 | **Socle** : Agility SDK, appareil 12_2 et DXR 1.2, dxc, bindless, captures PIX et Nsight ; lecture d'Elden Ring (ses pipelines RT, comment il gère les pièces de carte et les personnages) | Rien de visible, le jeu identique | Le jeu tourne comme avant | 1 |
| R1 | **DLSS** : les dix commits de la fiche UPSCALER-DLSS-READY par Streamline en mode manuel, masque réactif, vecteurs des effets et de l'eau ; Reflex | DLSS et le DLAA de NVIDIA à la place du filtre de DS3 | Image plus nette, plus d'images/s | 1 à 2 |
| R2 | **La scène en rayons** : liste du monde chargé autour du joueur, BLAS par maillage, skinning compute, tissus, vent, feuillages découpés (opacity micromaps), TLAS par image ; en DXR standard, avec la mesure de la construction et des refits (D-0238) ; une vue de contrôle « albédo tracé » | Tout ce qui existe est dans les rayons | La vue de contrôle (mode test) | 2 |
| R3 | **Matériaux pour les rayons** : le tableau des matériaux depuis les MTD de DS3, la brillance identifiée | Les reflets et rebonds auront la bonne couleur | Vue de contrôle des matériaux | 1 à 2 |
| R4 | **Lumières pour les rayons** : la liste unique, ReSTIR DI, le sort des lumières de remplissage et de l'ambiant cuit de DS3 | Base de l'éclairage tracé | | 1 |
| R5 | **Ombres tracées et Ray Reconstruction** | Remplace cascades et atlas local | **Premier vrai rendu RT à juger** | 1 |
| R6 | **Reflets tracés**, dont l'eau | Remplace la vue miroir de l'eau et les cubemaps | Reflets avec personnages et ombres ; témoin : le ciel dans l'eau de l'arène d'Iudex Gundyr, absent du reflet raster natif (I-0113 écart 18, Thomas 2026-11-01 : « On attend les reflets RT ») | 1 |
| R7 | **Illumination globale tracée** | Remplace SSAO et ambiant cuit | Le niveau « Ray tracing » complet | 1 à 2 |
| R8 | **Path tracing complet** (ReSTIR DI et GI, plusieurs rebonds) | Le niveau maximum | Le niveau « Path tracing » | 1 à 2 |
| R9 | **Effets et transparents éclairés par la lumière tracée** | Particules, voiles, eau cohérents | | 1 |
| R10 | **Génération d'images, retrait du raster** (le menu passé à R12, D-0258) | Plus d'éclairage raster, la génération d'images de DLSS | | 1 |
| R11 | **Optimisation et tour des zones en PT** (avec la retouche des sources lumineuses : l'essai de l'ombre tracée sur toutes les fill lights, `rt-fill-shadow-all`, refait, D-0259) | Les zones jamais vues, regardées directement en PT | | 1 à 2 |
| R12 | **Le menu des nouveaux réglages et le nouvel écran titre** (Thomas, 8 novembre, D-0258 : tout ce qui touche au menu, sorti de R10 ; l'écran titre de D-0182, sa maquette [title-screen-mockup.png](design/title-screen-mockup.png)) | Le menu de la section 6 et l'écran titre « Rekindled », dans la direction artistique de DS3 ; l'écran de précompilation des shaders avec sa barre de progression, nos propres shaders natifs compris (ROADMAP, ouvert depuis le 20 octobre ; Thomas, 10 novembre 04:22 : « Oui inscris le ») ; une barre de sharpening (la passe de R11j, éteinte par défaut ; Thomas, 04:28-04:30), la V-sync, l'aberration chromatique et le vignettage (ceux de DS3 : la dispersion latérale et le vignettage de son tone map YEBIS, TONEMAP-SKIN ; Thomas, 04:30) ; le mode d'affichage Sans bordures aux côtés de Plein écran et Fenêtré (le natif le fait déjà, I-0076, par `borderless.flag` ; Thomas, 04:35) ; ces réglages sur une page « Rekindled » à nous, dans la direction artistique de DS3, à la place de « Advanced Settings » (toutes ses lignes recréées), Sans bordures, la V-sync et la fréquence d'images dans la page Graphics du jeu (D-0266, [STUDY-R12-SETTINGS-PAGE](../investigations/I-0132-rt-optimisation-zones/STUDY-R12-SETTINGS-PAGE.md)) ; puis l'installateur (D-0267, ROADMAP C5) | Le menu final, l'écran titre | 1 |

**Total : 13 à 18 chantiers** (R12 ajouté le 8 novembre, D-0258 ; l'ordre R9, R10, R11, R12). Tu peux juger le rendu dès R5 ; R1 apporte déjà quelque chose de visible.

**Ordre retenu (D-0237, 30 octobre 04:55 ; D-0240, 08:29)** : R0, R1, puis la cadence par la boucle D3D12 moderne (Reflex branché sur son attente de début d'image, toujours une option) et le déblocage du framerate (pas variable, après l'étude d'impact I-0112), puis R2, puis **R5** (les ombres n'ont besoin que de la scène en rayons et de la transparence découpée), puis R3, R4, R6 à R11. Les ombres arrivent d'abord nettes (exactes à un rayon par pixel, sans débruiteur) ; Ray Reconstruction et les pénombres douces s'ajoutent quand R3 a identifié le canal de brillance dont ses guides ont besoin. Les cascades et l'atlas des ombres locales sont le premier raster supprimé.

**Un rendu tracé différent de l'original n'est pas un défaut en soi (Thomas, 30 octobre 04:56, D-0237)** : l'éclairage cuit et les lumières de remplissage de DS3 trichent, la lumière tracée suit la physique. Au début, l'image sera fausse par rapport à l'original, et c'est normal. Reste un défaut une donnée mal lue (une lumière, un matériau, un objet absent des rayons) ; l'aspect, c'est Thomas qui le juge. **Chantier d'éclairage ajouté** : ajuster les sources lumineuses des cartes (intensité, rayon, couleur, les lumières de remplissage qui simulaient le rebond) et en créer de nouvelles là où la scène tracée en a besoin, à partir de R4, zone par zone, à l'œil de Thomas. **Piste à évaluer** (Thomas, 04:57) : les textures emissive (le « glow ») trichent aussi ; avec le RT/PT, les faire émettre vraiment de la lumière (émetteurs échantillonnés par ReSTIR) ou les réajuster.

Ensuite : **T1, les textures** (section 11). Pas de modèles HD : la géométrie de DS3 reste celle d'origine (Thomas, relayé le 30 octobre 05:25, D-0238).

**Le post-traitement plus tard** (Thomas, 30 octobre 05:56 : « Le look de DS3 sera déjà grandement modifié avec le RT de toute façon. Bref on changera pour un truc plus moderne mais pas maintenant ») : les passes YEBIS restent pendant R0 à R11 ; une chaîne de post-traitement moderne (tone mapping, HDR, profondeur de champ, flou de mouvement, bloom) les remplacera dans un chantier à part, après le path tracing (D-0175 : chaque passe YEBIS est remplaçable).

**Les sources lumineuses refaites en physique, plus tard** (Thomas, 9 novembre 2026 23:01 : « Il va falloir refaire toutes les sources lumineuses du coup à terme? Peut être en rajouter des nouvelles », après les planches de I-0132 R11h où le rapport physique pur, appliqué seul, rendait l'image laiteuse et plate, D-0264) : un chantier à part, après le tour des zones (R11c) et le post-traitement moderne ci-dessus, famille par famille avec des planches : (1) les sources par famille (bougie, torche, brasero, feu, magie) à leur intensité réelle, une fois connu l'effet propriétaire de chaque lumière ; (2) les flammes comme vraies sources de lumière de leur forme, échantillonnées par les rayons, au lieu d'un point invisible ; (3) le brouillard éclairé par les vraies lumières (volumétrique tracé) au lieu de sa couleur fixe ; (4) T1 pour la lumière peinte des textures ; (5) de nouvelles sources seulement là où un objet visible en est une (lanterne, fenêtre), jamais une lumière d'appoint invisible (D-0262). Les outils de test de R11h (`rt-local-physical-10/-50/-200.flag`, l'exposition automatique) servent de base. La démarche (Thomas, 10 novembre 00:06, « Oui on peut faire comme lui ») : celle du mod DS3 Lighting Engine, tout re-régler ensemble (les lumières et le brouillard), mais avec les rayons et une règle par famille ; rien n'est repris du mod.

**Le pilote des chantiers les plus durs techniquement** (Thomas, 10 novembre 00:13 : « pour les plus durs techniquement on prendra Fable 5.2 en high, j'ai une réinitialisation gratuite spécialement pour Fable, autant s'en servir ») : les sources refaites en physique (avec le brouillard tracé et les flammes comme sources) et le remplacement de YEBIS par le post-traitement moderne sont pilotés par Fable 5.2 en effort high, dans leur propre fil ; les autres chantiers gardent le pilote par défaut (WORKFLOW « Roles and delegation »).

## 9. Ce que deviennent la ligne d'arrivée et les chantiers en cours

| Ligne de FINISH-LINE | Sort proposé |
|---|---|
| 1, les plantages (Gundyr, Pic de l'Ancien Dragon, plein écran après Alt-Tab, « sauter l'objet et continuer ») | **Gardée, en premier** : ils ferment le jeu quel que soit le rendu |
| 2 à 4, 6 | Faites. Le travail de la ligne 6 (lecture des spots, ombres marquées) est réutilisé par R4 et R5 ; son atlas d'ombres disparaît à R5 |
| 5, les effets manquants (I-0105, en cours) | **Gardée** : les effets restent dessinés en raster même en PT. Les rayons de lumière 10003 (3 effets) peuvent attendre le brouillard ombré par les rayons. Leur éclairage sera refait par R9 |
| 7, les défauts connus | **Coupée en deux** : gardés, ceux de matériaux et de données (murs d'Anor Londo sans texture, décalques, chaise blanche de la Servante, portraits du menu Charger, l'ordre des effets avec les rangs forward) ; abandonnés, ceux d'éclairage raster (sol de la salle de la Danseuse si c'est de la lumière, roues de m31, mur de m40, ombre du reflet de l'eau) |
| 8, les zones jamais vues et les réglages | **Zones** : un tour rapide maintenant pour les plantages seulement, le tour visuel sous PT (R11). **Réglages (I-0108, second pilote)** : on s'arrête à l'étape 1 qui se fusionne ; l'étape 2 (aucun personnage dans le reflet de l'eau) et le diviseur `+0x70` sont abandonnés, les reflets tracés les remplacent |
| Après la ligne d'arrivée | La cadence, le HDR10, le titre Rekindled restent ; « les technologies NVIDIA en tout dernier » est remplacé par ce plan |

Décisions à réécrire si tu valides : D-0228 (NVIDIA en dernier), la phrase du ROADMAP « le raster reste la référence », et D-0178 complétée (les réglages remastérisés sont ceux de la section 6).

## 10. Le raster existant : le supprimer ou le garder caché

**Option A, le supprimer** : dès qu'une catégorie tracée est acceptée, son code raster part (cascades, atlas local, SSAO, vue miroir, cubemaps, accumulation différée). Moins de code et rien à entretenir. On perd la comparaison image par image dans le même build.

**Option B, le garder caché le temps de la transition** : le raster reste dans le code, jamais dans le menu, activable seulement par le lanceur de test (comme les outils `test-*`). Il sert à vérifier, sur la même image et la même caméra, que les rayons lisent les bonnes lumières et les bons matériaux et que la direction artistique tient. Il est supprimé catégorie par catégorie quand tu as accepté l'équivalent tracé en jouant.

**Décision (D-0237, 30 octobre 04:55) : A.** Thomas : « on a l'original, ça suffit ». Chaque catégorie raster part dès que sa version tracée marche et qu'il l'a acceptée en jouant, jamais avant. Le texte qui suit est la recommandation d'origine, remplacée.

**Recommandation d'origine : B, limitée dans le temps.** Le jeu D3D11 d'origine reste une référence externe, mais la comparaison dans le même build est le seul moyen rapide de séparer « le rayon lit une mauvaise donnée » de « le PT donne une autre lumière, et c'est voulu ». Une fois R10 terminé, plus rien de raster ne reste pour l'éclairage.

## 11. Les textures, après le RT, et ce que Claude peut piloter

Le natif a son propre chargement des textures : le plus simple sera un **dossier de remplacement** (une texture HD remplace l'originale par son nom au chargement, sans toucher aux archives du jeu), puis DirectStorage plus tard (déjà au ROADMAP).

Outils locaux, tous pilotables par Claude en ligne de commande, rien en ligne :

| Outil | Pilotable | Usage |
|---|---|---|
| **chaiNNer** | Oui : `chainner run chaine.chn` sans interface, avec un fichier de valeurs à remplacer (depuis la v0.17) | Chaînes d'upscale avec les modèles ESRGAN d'OpenModelDB, traitement par lots |
| **Real-ESRGAN (ncnn-vulkan)** | Oui, binaire en ligne de commande | Upscale x4 simple et rapide |
| **Outils IA de RTX Remix** (ComfyUI-RTX-Remix, PBR Fusion 4, PBRify) | Oui : ComfyUI et le toolkit Remix ont une API REST locale | Upscale et génération de cartes de matériau (brillance, métal, normales) pour le RT |
| **texconv** (DirectXTex de Microsoft) | Oui | Conversion DDS et recompression BC après l'upscale |
| **Topaz Gigapixel** | Plutôt en interface | Possible, moins adapté aux lots |

Remarque : RTX Remix lui-même ne s'applique pas à DS3 (il vise les jeux DirectX 8 et 9) ; seuls ses outils IA de textures servent.

Pièges : les normal maps ne passent jamais par un modèle de couleur (upscale des deux canaux puis renormalisation, ou modèle dédié) ; DS3 a déjà des cartes de normale et de réflectance, donc la génération de matériaux sert seulement à compléter ce qui manque (la brillance si elle est absente), sans remplacer ce que les artistes ont fait ; la direction artistique doit rester celle de DS3 (pas de « look IA »). Chantier **T1** : une skill du dépôt qui extrait, upscale, recompresse et place les textures d'une zone, puis un passage par zone à juger.

## 12. Risques

1. **La direction artistique.** DS3 a été éclairé pour le raster : lumières de remplissage posées pour simuler le rebond, ambiant cuit, brouillard. Le PT ajoute son propre rebond : risque d'intérieurs plus clairs ou plus sombres, d'ambiance changée. Parade : R4 lit le rôle de chaque type de lumière, comparaison aux captures de l'original, et c'est ton œil qui juge.
2. **Les objets hors écran.** DS3 ne prépare que ce qui est dans le champ et charge la carte par morceaux : un objet absent des rayons donne un trou dans un reflet ou une fuite de lumière. Parade : la liste du monde chargé de R2 et la vue de contrôle.
3. **Les personnages, tissus, vent** : une ombre qui ne suit pas l'animation se voit tout de suite.
4. **Les matériaux vus par les rayons** : une erreur de lecture ne se voit que dans les reflets et rebonds, donc tard. Parade : la vue de contrôle des matériaux de R3.
5. **Les API jeunes** : Mega Geometry DirectX en préversion (Mode développeur), NVAPI propre à NVIDIA, Agility SDK choisi depuis l'intérieur du processus de DS3 (jamais essayé).
6. **Les performances** : chiffres estimés ; moteur limité par le CPU ; latence de la génération d'images.
7. **Le verrou NVIDIA** : DLSS, Ray Reconstruction, génération d'images et Mega Geometry sont propres à NVIDIA. Pour AMD et Intel, plus tard, il faudra FSR ou XeSS et un débruiteur générique. Sans raster, aucune carte sans ray tracing matériel.
8. **Le temps** : 12 à 17 chantiers, plus que ce qui reste à la ligne d'arrivée actuelle ; le jeu reste jouable entre deux chantiers (chaque fusion garde le jeu qui démarre).
9. **L'éclairage peint dans les textures** (Thomas, 2026-11-03 04:16) : certaines textures ont de la lumière, des reflets ou de l'ombre peints dans l'albedo, ou un gloss poussé pour simuler un effet ; sous le RT, ils feraient double emploi avec le vrai éclairage. Pour l'instant les rayons copient fidèlement les données du raster (R3) ; le traitement de ces textures est à décider plus tard, vers Fresnel et R4. Aucun travail dessus avant.

## 13. Ce que je te demande de valider

1. **L'ordre global** : (a, ma recommandation) la ligne 1 (plantages) et le chantier d'effets en cours se terminent, puis R0 à R11 ; (b) R0 tout de suite, les plantages en parallèle.
2. **Le raster existant** : option B (caché le temps de la transition, puis supprimé) ou option A (supprimé dès qu'une catégorie tracée est acceptée).
3. **Le reste du plan tel quel** : les niveaux de la section 6, Streamline en mode manuel, ~~Mega Geometry comme base avec le chemin DXR standard à côté~~ (remplacé par D-0238 : DXR standard, Mega Geometry seulement sur mesure), les textures après le RT (T1).

## Sources

- Code natif et fiches du dépôt : `native/renderer/backend.cpp:403-407`, `native/frame/game_bootstrap/14_scene_step_and_storage.inc:245`, `05_selected_scene.inc:295-298`, `12_scene_collect.inc:~907`, `native/renderer/scene_pass_velocity.cpp`, `native/world/producers.cpp:33-60`, [UPSCALER-DLSS-READY](../investigations/I-0066-independent-native-startup/UPSCALER-DLSS-READY.md), [VELOCITY-IMPLEMENTATION-CHECKLIST](../investigations/I-0068-static-yebis-ffx-lights-spec/VELOCITY-IMPLEMENTATION-CHECKLIST.md), [I-0108 INVENTORY](../investigations/I-0108-graphics-options/INVENTORY.md), [ER-PIPELINE-STUDY](../investigations/I-0072-shader-pipeline-cache/ER-PIPELINE-STUDY.md), [FINISH-LINE](FINISH-LINE.md), [ROADMAP](ROADMAP.md), [decisions/LOG](../decisions/LOG.md) D-0165, D-0167, D-0178, D-0187, D-0195, D-0228.
- Gears of War: E-Day : [Wccftech, analyse PC](https://wccftech.com/how-to/gears-of-war-e-day-pc-performance-analysis-tuning-guide-how-to-get-the-best-experience-on-pc/), [Mixed News](https://mixed-news.com/en/gears-of-war-e-day-ray-tracing-always-on-steam-deck-performance/), [NVIDIA](https://www.nvidia.com/en-us/geforce/news/gears-of-war-e-day-geforce-rtx-tech-partnership-dlss-ray-tracing/).
- Mega Geometry : [DirectX Developer Blog, préversion du 28 octobre 2026](https://devblogs.microsoft.com/directx/clas-ptlas-preview/), [Tom's Hardware](https://www.tomshardware.com/pc-components/gpus/testing-nvidias-rtx-mega-geometry-tech-vram-reducing-tech-a-leap-forward-for-path-traced-rendering), [Wccftech, Alan Wake 2](https://wccftech.com/alan-wake-2-rtx-mega-geometry-performance-vram-improvements/).
- DLSS 4.5 et Ray Reconstruction : [TechPowerUp](https://www.techpowerup.com/review/nvidia-dlss-4-5-ray-reconstruction/), [NVIDIA, intégration par Streamline](https://developer.nvidia.com/blog/how-to-integrate-nvidia-dlss-4-into-your-game-with-nvidia-streamline).
- Coût du path tracing : [The FPS Review, Cyberpunk 2.0 sur RTX 40](https://www.thefpsreview.com/2023/09/21/cyberpunk-2077-2-0-dlss-3-5-performance-tested-across-geforce-rtx-40-series/4).
- Elden Ring 1.09 : [Elden Ring wiki, 1.09](https://eldenring.wiki.gg/wiki/1.09).
- Textures : [chaiNNer v0.17 (ligne de commande)](https://newreleases.io/project/github/chaiNNer-org/chaiNNer/release/v0.17.0), [ComfyUI-RTX-Remix](https://github.com/NVIDIAGameWorks/ComfyUI-RTX-Remix), [outils IA de RTX Remix](https://docs.omniverse.nvidia.com/kit/docs/rtx_remix/latest/docs/howto/learning-aitexturetools.html), [PBRify_Remix](https://github.com/Kim2091/PBRify_Remix).
