<!-- Copied into the repository on 2026-10-26 by the study thread from the project files (etudes/cadence/RELECTURE-FABLE-MODERN-CADENCE.md), Fable 5.1's review of MODERN-CADENCE-STUDY at commit 5f66b1e5, so that the repository holds it. Amendments 1, 2, 3 and 5 are integrated in the study (its §11); point 4 is Thomas's. -->
# Relecture critique du plan « Cadence, méthode DX12 moderne » (Fable 5.1, effort élevé)

Relecture en lecture seule (2026-10-26, 05:05-05:45 Paris), demandée par Thomas (accord explicite dans le fil, 05:04 Paris,
D-0198), sur `MODERN-CADENCE-STUDY.md` (copie du commit 5f66b1e5 de la branche PC `claude/zen-mendel-263329`, dossier partagé).
Aucun code, aucun test, aucun accès au PC (Thomas joue). Lu en plus, dans le dépôt GitHub (copie de `main` ca1ad277) :
WORKFLOW, D-0084, D-0169, D-0187, D-0195, D-0198, D-0211, D-0212, D-0218, D-0219, K-0479/K-0480, I-0097 RESULT (tranches 3, 5, 6),
CADENCE-STUDY (A.1-A.7), les études ER I-0076 et I-0080, et le code `native/` cité (pacer, présentation, `EndFrame`, `WaitFrameSlot`,
phases de démarrage).

**Ce que je n'ai pas pu vérifier** : les lignes PresentMon et les traces `cadence_trace` de l'Evidence Store (hors Git, sur le
PC), le listing A0 du limiteur (`cadence-limiter-a0.txt`, idem) et la branche de l'étude elle-même (absente de GitHub). Les
chiffres du §5 de l'étude sont donc pris tels quels ; j'ai seulement vérifié leur cohérence avec RESULT tranches 5-6 (elle est
bonne : mêmes retenues 10/4 contre 4/1, mêmes 8 et 1 présentations perdues, même écart post→pas de jeu 12,2-12,6 ms).

Statuts utilisés ici : **OBSERVÉ** (lu par moi dans le dépôt ou dans l'étude), **DÉDUIT** (raisonnement, non mesuré),
**GÉNÉRAL** (connaissance générale D3D12/DWM, non vérifiée ici), **AVIS** (mon opinion). Rien n'est à promouvoir au ledger.

## 0. Verdict en bref

1. **Le plan est solide dans son cœur.** Le diagnostic de l'échec de la tranche 6 (le limiteur DS3 mesure 1/60 s depuis sa
   propre sortie, donc une attente placée avant lui et qui finit avant son échéance ne déplace jamais sa phase) est cohérent
   avec A0 tel que CADENCE-STUDY A.7 le rapporte et avec l'écart constant par run mesuré (§5.6). La correction proposée,
   attendre jusqu'au premier créneau d'affichage **à ou après** l'échéance du limiteur, est la bonne : j'ai refait
   l'arithmétique (§2 ci-dessous), le verrou tient bien de lui-même sur tout écran à 60 Hz ou moins, sans toucher au limiteur.
2. **Les causes avancées sont étayées, pas seulement plausibles**, pour les trois points principaux (phase du limiteur, pertes
   de la tranche 6 par échéance de composition, retenues actuelles = images en retard). Elles reposent toutefois sur **un seul
   écran, un seul segment de 20 s, deux runs de grille** : la généralisation est à confirmer par T0, que le plan prévoit à juste
   titre avant tout code.
3. **Trois défauts à corriger avant T1/T2** : (a) la latence maximale 1 avec le DWM en mode composé peut bloquer le worker
   jusqu'à la composition, ce que l'étude ne compte pas (§3 R1) ; (b) le cas « écran légèrement plus rapide que 60 Hz » est
   contradictoire entre §6.2 et §6.5 et son arithmétique est fausse (§3 R2) ; (c) aucun critère ne borne la latence
   entrée→affichage, alors que le choix de phase en ajoute jusqu'à un rafraîchissement (§3 R3).
4. **Une alternative plus « moderne » existe et mérite un A/B dans T2** : la même règle d'échéance appliquée à l'objet
   d'attente de la chaîne (frame-latency waitable object) au lieu d'une grille de vblank reconstruite. Elle supprime le fil
   vblank, le calcul de phase et la statistique p95, et c'est littéralement la boucle des jeux D3D12 modernes (§4 A1). La
   tranche 5 l'a rejetée pour une raison de phase que la règle d'échéance corrige, pas pour une raison de mécanisme.
5. **Deux points sont à Thomas, pas au pilote** : réutiliser le fil vblank de la tranche abandonnée (D-0219 dit « plus de
   bricolage ») et accepter que le dt du jeu devienne la période de l'écran (lecture de D-0169, comme l'original sous V-Sync).

## 1. Les causes : observation, interprétation, avis

| Affirmation de l'étude | Observation (ce qui est mesuré) | Interprétation (ce qu'on en déduit) | Mon avis |
|---|---|---|---|
| Le limiteur mesure 1/60 s depuis sa propre sortie (`+0x28`) | A0 (listing, non relu ici) : `+0x20 = +0x28`, `+0x28 = QPC`, boucle jusqu'à `cible − (+0x28 − +0x20) ≤ 0` (OBSERVÉ via A.7) | Son échéance suivante est `+0x28 + +0x18 × f` | Étayé. Reste à confirmer en T0(b) : la branche « fenêtre sans focus » (sommeil puis spin) écrit-elle `+0x28` de la même façon à la sortie, et le rattrapage `+0x278` est-il actif en jeu |
| Une attente avant le limiteur ne déplace pas sa phase (tranche 6) | Écart fin d'attente → pas de jeu constant par run : 1,6-1,9 ms (`g1`), 5,9-6,3 ms (`g2`), plat sur le tour (OBSERVÉ, traces) | Le limiteur attend le reste de son 1/60 s ; sa phase est fixée par l'historique (dernière image en retard) | Convaincant : un écart différent par run et plat dans le run n'a pas d'autre explication simple. Note : avec un écran plus lent que 1/60 s de ~1 µs/image, la grille de la tranche 6 aurait fini par se verrouiller seule au bout de minutes (1,4 ms / 0,3 µs ≈ 80 s pour `g1`) ; cela conforte le mécanisme et explique pourquoi un tour de 20 s ne pouvait rien montrer |
| Les 6 pertes inexpliquées = échéance de composition (~6 ms avant le vblank) | Dans `g1` : 5 épisodes « latence 7-9 → 22-24 ms, dérive 70-290 images, perte » ; 1 remplacement dans le même intervalle (OBSERVÉ, lignes) ; les présentations de la grille à 6-9 ms du vblank (OBSERVÉ) | Le compositeur (DWM) prend les présentations ~6 ms avant le vblank ; au-delà, une image attend un rafraîchissement et la file passe à 2 | Cohérent avec le fonctionnement de DWM en `Composed: Flip` (GÉNÉRAL : DWM compose une fois par vblank, quelques ms avant). La **valeur** 6 ms vient d'un écran, un mode, un run ; la règle de phase adaptative du §6.4 (p95, hystérésis) rend le plan peu sensible à sa valeur exacte, ce qui est bien pensé |
| Les retenues actuelles sont des images en retard, pas l'horloge | 14 des 16 retenues (6 tours, pacer de `main`) suivent une présentation à 21-50 ms de la précédente ; 2 suivent des présentations à l'heure (OBSERVÉ, lignes) | Travail CPU de première vue (admissions du tour, RESULT t5 item 8) ; relève du chantier pop-in | Étayé et important : il fixe l'ordre des chantiers et justifie le critère scindé du §8. Le lien « retard → admissions » reste une corrélation de profils échantillonnés (RESULT t5 item 8 : 40 échantillons, 3 images lentes), pas une preuve causale ; le chantier pop-in le tranchera |
| Personne n'est verrouillé à l'écran ; dérive 0,5-1,3 µs/image | Latence à l'affichage dérive 9,74 → 10,36 ms (original), 10,35 → 11,90 (pacer n1) sur le tour (OBSERVÉ, moyennes par 100 images) | Les deux horloges (1/60 s du limiteur, période de l'ASUS) diffèrent de ~0,006 % | Étayé, mais **deux chiffres se contredisent légèrement** : §5.6 donne 0,3 µs/image pour la fermeture de l'écart, §5.2 0,5-1,3 µs/image pour la dérive de latence ; et la moyenne « présentations de l'original 16,6636 ms » est plus courte que 1/60 s, ce qu'un limiteur qui spinne ne peut pas produire (moyenne sur le run entier, titre et chargements inclus, DÉDUIT). À trancher en T0 avec le lecteur, sur la fenêtre du tour seulement |
| La règle des huit images explique les retards supplémentaires de la grille | 8 et 6 présentations > 25 ms (grille) contre 2 et 1 (pacer) (OBSERVÉ) | La règle transforme un retard de 3-5 ms en trou de 33 ms | Marqué DÉDUIT par l'étude, à juste titre ; peu importe pour le plan puisque la règle est abandonnée |
| La pression arrière ne régule qu'un producteur plus rapide que l'écran | Trois formes de la tranche 5 : attente 0,00 ms (OBSERVÉ, RESULT t5 items 7 et 9) | Le jeu n'est jamais une présentation en avance dans un run | Exact pour la forme testée. Mais la conclusion du §7 (« l'objet seul comme horloge : rejeté ») va trop loin : voir §4 A1 |
| Pratique moderne (objet d'attente à latence 1, attente en début d'image, règles de tearing) | SOURCE Microsoft Learn (lu par l'étude) + GÉNÉRAL | — | Correct dans l'ensemble. Deux nuances au §3 R1 (latence 1 sous composition) et R7 (la VRR en fenêtre dépend bel et bien d'un réglage pilote, « G-SYNC fenêtré », que l'OLED de Thomas n'a qu'en plein écran, D-0212) |
| ER n'a pas d'objet d'attente, deux horloges, deux spins | OBSERVÉ dans I-0076 et I-0080 ER-STUDY (relus ici) | On copie la chaîne et le worker, pas le pacing | Conforme à D-0195 et à sa nuance (« sans ses défauts ») ; D-0219 nomme explicitement la chaîne d'attente, donc l'écart avec ER est déjà décidé par Thomas |

## 2. Le mécanisme central (§6.2), vérifié à la main

Notation : `P` période réelle de l'écran, `L = 1/60 s` le pas du limiteur, `S(n)` le créneau choisi, `D(n)` l'échéance du limiteur
(`+0x28 + L`), `tol` ≈ 0,5 ms. Règle : attendre le premier créneau `S ≥ D − tol`.

- **ASUS (`P` ≈ 16,668-16,671 ms > `L`)** (DÉDUIT) : si `S(n) ≥ D(n)`, le limiteur sort aussitôt, `+0x28 ≈ S(n)` (plus
  quelques µs d'appel de tâche), `D(n+1) = S(n) + L ≤ S(n) + P = S(n+1)` : verrouillé, aucun spin, dt du jeu = `P`. Si au
  départ `D − tol ≤ S < D`, le limiteur spinne `D − S` puis sort à `D` ; l'écart `S − D` croît de `P − L` (quelques µs) par
  image jusqu'au verrou. Correct, et le spin de 31-42 % disparaît bien (il devient notre attente sur timer + 1 ms de spin ;
  AVIS : réduire ce spin final à 0,5 ms serait cohérent avec le critère « part de spin −25 points »).
- **60,000 Hz exactement** : `P = L`, l'écart `S − D` est constant ; si négatif (jusqu'à `−tol`) le limiteur spinne ce reste
  chaque image, borné à 0,5 ms. Acceptable, comme l'étude le dit.
- **Image en retard** (`EndFrame` après `S(n+1)`) : pas d'attente, le limiteur sort aussitôt à `t`, `D = t + L` tombe
  `(t − S(n+1))` après `S(n+2)` ; la règle saute donc à `S(n+3)` : **un pas de jeu de ~32 ms** (deux pas de simulation pour une
  image affichée) et **une image retenue**, puis la phase est rétablie et la file de présentation (montée à 2 pendant une
  image) redescend. C'est la règle de V-Sync, et c'est ce que le §6.2 point 4 décrit, mais l'étude ne dit pas explicitement
  que le saut coûte un pas de 32 ms **à l'image suivante**, visible comme une accélération d'un rafraîchissement. RT-9 (dt
  moyen, p95, max) le couvre si le max de l'original sert de référence : à écrire dans T2.
- **Mode 1/30 s** (`+0x18 = 1/30`) : la règle attend le créneau ≥ `+0x28 + 33,3 ms` → 30 images/s sur la grille, correct.
- **Rattrapage `+0x278`** : il raccourcit l'échéance, donc `S ≥ D` reste vrai ; le jeu ne rattrape pas (il ne le peut pas non
  plus dans l'original sous V-Sync, sauf par la file). Sans effet sur le verrou.
- **Lecture de `+0x28`/`+0x18` dans `EndFrame`** : même fil que le limiteur, qui ne tourne pas à cet instant → lecture
  cohérente, sans course (OBSERVÉ : `EndFrame` et le limiteur sont deux tâches successives du même fil, A.7). Et l'attente
  remplace un spin **au même endroit et de même durée** pour les autres fils de DS3 (la barrière de chargement de K-0481) :
  le reste du jeu ne voit aucune différence de timeline. Bon point à écrire dans l'étude.

## 3. Risques et lacunes

**R1 — Latence maximale 1 sous `Composed: Flip`** (DÉDUIT + GÉNÉRAL ; le plus important). L'objet d'attente est libéré quand
une présentation est **consommée** ; en mode composé, c'est quand DWM la prend pour sa composition, soit quelques ms avant le
vblank, pas au moment de l'appel. Avec latence 1, le worker (qui attend l'objet en début de job, `scene_pass.cpp:1719`) ne peut
commencer l'image N+1 qu'une fois N consommée. Or le plan **veut** que N arrive ≥ 10 ms avant son vblank : le worker resterait
donc bloqué ~4-10 ms chaque image, finirait son job plus tard, et le post de l'engine (`02_frame_lifecycle.inc:450`, qui attend
le job précédent) pourrait être retardé à son tour : images en retard créées par le design lui-même. Avec latence 2 (aujourd'hui)
l'objet ne bloque jamais tant que la file tient 1 (ce que la tranche 5 a mesuré trois fois). L'étude justifie 1 par « CPU
worker + GPU < un rafraîchissement », ce qui ignore la consommation par le compositeur. **À faire** : garder 2 par défaut ;
en T1 lire `waitable_wait_ms` et `waitable_timeouts` (déjà journalisés) comme critère d'engagement de la variante 1 ; ne
proposer 1 à Thomas que si l'attente mesurée reste à 0 et la latence baisse.

**R2 — Écran légèrement plus rapide que 60 Hz** (OBSERVÉ dans l'étude, DÉDUIT pour l'arithmétique). §6.2 point 5 limite
l'attente moteur aux écrans « 59,94-60,000 Hz » ; §6.5 décrit pourtant une attente moteur à 60,02 Hz (« ses créneaux
arrivent avant l'échéance une fois toutes les ~45 s »). Les deux se contredisent, et le chiffre est faux : à 60,02 Hz
(`P` = 16,661 ms < `L`), après verrou `D(n+1) = S(n) + L > S(n+1)` **à chaque image** ; la tolérance absorbe 0,5 ms, soit
~90 images, puis la règle saute un créneau : une image retenue toutes les ~1,5 s, pas toutes les 45 s (45 s est le battement
d'un limiteur libre à Present(1), c'est-à-dire le comportement du pacer actuel, pas celui de l'attente moteur). **À faire** :
supprimer la puce du §6.5 ou la réécrire : « plus rapide que 1/60 s au-delà de 10 ppm → pas d'attente moteur, pacer de `main`
et Present(1) », ce qui est la règle de la tranche 6 et celle du §6.2.

**R3 — Aucun critère sur la latence entrée→affichage** (DÉDUIT). La phase choisie place le début d'image à `vblank + offset`
avec `offset = 10 ms + p95(présentation − créneau)`, donc l'image affichée est celle du **deuxième** vblank après le créneau
environ (créneau + 4,5 ms engine + 4 ms worker + ≥ 10 ms de marge ≈ 19-20 ms, puis balayage). C'est comparable à l'original
(8,5-10,4 ms de marge + sa propre chaîne), mais rien ne le vérifie : T2 n'a qu'un plancher (« latence à l'affichage p1 ≥ 9 ms »),
qui va dans le sens contraire. **À faire** : ajouter à T2 « pas de jeu → affichage (trace `game_step` + PresentMon) : médiane
et p95 au plus celles de l'original + 2 ms ».

**R4 — Base de preuves étroite** (OBSERVÉ). Un écran (ASUS, secondaire, composé), un segment (tour de 20 s, ennemis sans IA),
2 runs de grille, 6 de pacer, 2 originaux ; l'échéance de 6 ms tirée de 5 épisodes d'un run ; les scripts awk/Python de
l'étude sont dans un brouillon de session, non reproduits. Le plan en tient compte (T0 reproduit le §5 avec un outil avant
tout code) : c'est la bonne réponse, à condition que T0 soit **bloquant** pour T1 (le texte le dit implicitement par
« Exit » ; l'écrire).

**R5 — Réutiliser le fil vblank de la tranche abandonnée** (AVIS). D-0219 : « plus de bricolage », le code de la grille reste
sur le tag. L'étude le reprend au nom de « transformer, ne pas jeter » (D-0195). Le composant lui-même est sain (blanks à
0,02 ms de la trace, RESULT t6 item 7) et ses règles ont été relues deux fois ; mais c'est bien la pièce la plus artisanale
du plan (fil dédié, confirmation à 4 blanks, ré-ancrage à 3, statistique p95, hystérésis), et l'alternative A1 (§4) s'en
passe. **À faire** : poser le choix à Thomas avant T2, avec A1 en face.

**R6 — Lecture de D-0169** (OBSERVÉ + AVIS). Le dt du jeu devient la période de l'écran (16,67-16,68 ms sur l'ASUS, 16,683 ms
à 59,94 Hz : le jeu tourne 0,1 % plus lentement). C'est exactement ce que l'original fait sous V-Sync (le Present bloquant
re-phase son limiteur), donc défendable ; mais l'étude renvoie la question à RT-9 « au moment de la fusion ». **À faire** :
la poser à Thomas **avant T2**, pas après : si la réponse est non, tout T2 tombe.

**R7 — VRR et « aucun réglage pilote »** (GÉNÉRAL). En fenêtre et sans bordure, la VRR n'est active que si le pilote l'autorise
(« G-SYNC fenêtré ») ; l'OLED de Thomas ne l'a qu'en plein écran (D-0212). La phrase « rien ne dépend d'un réglage pilote » du
§6.5 est vraie pour notre code, fausse pour ce que l'utilisateur verra. À nuancer, sans conséquence sur le design.

**R8 — Points à confirmer en T0(b), déjà listés par l'étude, que je souligne** : la branche sans focus du limiteur (sommeil
« reste moins une marge » puis spin : si `reste ≤ 0`, sort-elle sans dormir ?) ; le deuxième appelant du limiteur
(0x140ED8EE7 : écran de chargement ? film ?) ; l'ordre des tâches après le limiteur (entrée, porte de mise à jour, `BeginFrame`),
qui fixe la latence d'entrée du R3.

**R9 — Marge de 10 ms contre un offset qui déborde** (DÉDUIT). `offset = 10 + p95(W)` avec `W` ≈ 8-10 ms donne 18-20 ms, soit
plus d'une période : il faut le réduire modulo `P` et choisir le vblank de référence sans ambiguïté ; une zone lourde (engine
8-12 ms) déplace `W` et donc la phase (hystérésis 2 ms) : à chaque re-choix, une image retenue ou une file à 2 pendant une
image. Petit, mais à écrire dans le design et à compter dans T3 (OLED, flip indépendant).

**R10 — Critère scindé du §8** (AVIS) : juste. Ajouter que « retenue sans image en retard = 0 » doit être évalué sur **les deux
runs de contrôle et la série**, car c'est le seul critère qui appartient à ce chantier ; le total ≤ 0,5 dépend du pop-in.

## 4. Alternatives modernes

**A1 — L'objet d'attente de la chaîne avec la même règle d'échéance** (DÉDUIT + GÉNÉRAL ; la principale). La tranche 5 a montré
que l'objet « ne bloque jamais » : c'est un problème de **phase** identique à celui de la tranche 6 (le limiteur garde la
sienne, la libération de l'objet tombe avant `D`, et l'écart ne se ferme qu'à ~1 µs/image), pas un problème de mécanisme.
Appliquer la règle de l'étude à l'objet : à la fin de `EndFrame` (la présentation N vient d'être émise par le job N que le
post attend ; le job N+1 est posté), l'engine attend l'objet ; **si la libération tombe avant `D − tol`, il attend la
libération suivante** (une fois, 32 ms de pas de jeu, une image retenue : exactement le coût du saut du §2). Ensuite
`+0x28 =` l'instant de consommation et `D(n+1) ≤` la consommation suivante tant que `P ≥ L` : même verrou, même absence de
spin. Avantages : pas de fil vblank, pas de confirmation de grille, pas de calcul de phase ni de p95 (la consommation par le
compositeur **est** l'échéance de composition : la présentation suivante arrive naturellement ~9-10 ms avant son vblank, la
marge mesurée de l'original), vendeur-neutre, flip indépendant et plein écran couverts par le même code (l'objet y est libéré
au flip), latence 1 cohérente puisque c'est l'engine et non le worker qui porte l'attente (la passation du compte entre fils
existe déjà : tranche 5, finding H1). Inconvénients : la libération n'est pas documentée à la µs près en mode composé
(à mesurer, c'est l'objet de T2) ; il faut une borne (34 ms, comme la tranche 5) quand aucune présentation n'est en vol
(fenêtre cachée, chargement) ; et sur un écran VRR ou non multiple de 60 la libération suit la présentation, donc pas
d'attente moteur là non plus (même exclusion que le plan). **Proposition** : T2 en **deux formes derrière le même drapeau**
(`display-clock=grid` / `display-clock=waitable`), deux runs de contrôle chacune, puis la série avec la forme qui passe les
contrôles ; si les deux passent, la plus simple gagne (AVIS : A1).

**A2 — Lire le compositeur au lieu de l'inférer** (GÉNÉRAL, à vérifier). `DwmGetCompositionTimingInfo` (dwmapi, hwnd nul)
rend `qpcVBlank`, `qpcRefreshPeriod` et `qpcCompose` : le vblank et l'instant de composition de DWM sur le compteur de
performance, sans fil dédié. Si ses champs sont encore renseignés sur le Windows de Thomas (certains sont gelés depuis
Windows 8), c'est la mesure directe de l'échéance de 6 ms pour T0(c), et un ancrage de grille sans `WaitForVBlank` si la
forme grille est retenue. Vendeur-neutre. À tester en T0, un script suffit (lecture seule, pas de jeu).

**A3 — La vraie méthode moderne commence à V2** (AVIS). Tout ce qui est spécifique ici (lire `+0x28`, choisir un créneau ≥ `D`)
n'existe que parce que le cap de DS3 est intouchable et égal au rafraîchissement. Dès que V2 écrit la fréquence demandée
(`+0x354`/`+0x358`, prévu par CADENCE-STUDY A.7 V2a), le cap passe au-dessus de l'écran et la boucle moderne pure s'applique
sans lecture du limiteur : objet d'attente à latence 1 en début d'image, Present(1), point. T2 est donc un **pont** pour la
période « logique à 60 » ; sa taille (fil vblank, phase, p95, T3 sur quatre modes) doit être pesée contre sa durée de vie. A1
est aussi la forme qui survit le mieux à V2 (on retire la règle d'échéance, le reste demeure).

**A4 — Reflex / Anti-Lag 2** : d'accord avec l'étude, plus tard, sur la même attente de début d'image.

## 5. Le plan en tranches : ce qui tient, ce qui manque

- **Conformité aux règles** (OBSERVÉ) : limiteur jamais écrit (seulement lu) ; pas de wrapper (D-0084) ; chaîne ER copiée,
  pacing délibérément différent et déjà décidé par D-0219 (D-0195) ; drapeau A/B, relecture d'étape Opus élevé, relecture
  finale xhigh (D-0198) ; deux runs de contrôle avant toute série (règle de Thomas) ; seuil sur l'ASUS seul, sans `-Memory`
  (D-0218, aucun changement mémoire) ; runs annoncés hors des créneaux du pilote pop-in (règle Live). Rien à redire.
- **T0** : bon et nécessaire ; y ajouter A2 (lecture DWM) et la réconciliation des dérives (R4, 0,3 contre 0,5-1,3 µs/image,
  fenêtre du tour seulement). T0 bloquant pour T1.
- **T1** : la création avec `ALLOW_TEARING` partout où le factory le permet est juste (Present(1) l'ignore ; Present(0, flag)
  en fenêtre seulement ; Present(0, 0) en plein écran exclusif : conforme à la doc). Pour la latence 1 contre 2 : critère
  d'engagement `waitable_wait_ms` et `waitable_timeouts` (R1) ; par défaut 2 reste.
- **T2** : corriger R2 ; ajouter le critère de latence R3 et l'énoncé explicite du saut après image en retard (§2) ; A/B des
  deux formes (A1) ; porter R5 et R6 à Thomas avant d'ouvrir la tranche. Le contrôle « reste d'attente du limiteur < 0,1 ms
  p99 dans les deux runs » est le bon signe d'engagement (c'est précisément ce que la tranche 6 n'a pas eu).
- **T3** : complet ; ajouter le comptage des re-choix de phase (R9) ; sur l'OLED fenêtré (flip indépendant) l'échéance de
  composition est plus proche du vblank : une marge fixe de 10 ms y coûte de la latence pour rien, A1 s'y adapte seul.
- **T4** : hors sujet ici, d'accord.

## 6. Synthèse

Le plan répond à la demande de Thomas (une horloge, l'écran qui commande, le limiteur intact, méthode moderne) avec un
diagnostic que je juge **étayé** et une correction **juste** du défaut de la tranche 6. Je recommande de l'adopter avec cinq
amendements : garder la latence 2 tant que l'attente de l'objet n'est pas mesurée à 0 (R1) ; corriger le cas 60,02 Hz (R2) ;
borner la latence entrée→affichage (R3) ; porter à Thomas, avant T2, la réutilisation du fil vblank et la lecture de D-0169
(R5, R6) ; et faire de T2 un A/B entre la grille et l'objet d'attente (A1), en pariant sur l'objet comme forme finale parce
qu'elle est plus simple, plus proche de la méthode moderne et la seule qui survive telle quelle à V2.

Degré de sûreté : élevé sur §1-§2 (cohérence interne et arithmétique) ; moyen sur R1 et A1 (la libération de l'objet en mode
composé est GÉNÉRAL, non mesurée ici) ; les chiffres du §5 de l'étude ne sont pas revérifiés faute d'accès au PC.
