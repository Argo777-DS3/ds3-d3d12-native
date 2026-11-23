# Étude : accélérer le build natif (2026-11-16, lecture seule)

## Mesures (horodatages des fichiers des builds du jour)

| Build | Début | DLL liée | Fin des tests | Durée |
|---|---|---|---|---|
| launcher `game-bootstrap-build` | 10:09:11 | 10:16:43 | 10:23:53 | 14,7 min |
| R `I-0138/build-t7v4` | 11:25:22 | 11:33:46 | 11:42:57 | 17,6 min |
| R `I-0138/build-t7v6` | 12:08:04 | 12:16:27 | 12:23:50 | 15,8 min |
| E `I-0140/build-e86` | 12:25:49 | 12:33:26 | 12:38:53 | 13,1 min |

Les 20 min viennent de deux builds lancés en même temps (10:09 et 10:18 se chevauchent) ou d'une machine chargée.

Décomposition du build launcher (Ryzen 7 5800X3D, 16 threads, 32 Go) :

- 191 compilations `cl`, 172 programmes `dxc`/`fxc`, 80 `link` : **10,5 min, toutes en série** (un seul `build.cmd`, une commande après l'autre, pas de `/MP`, pas de PCH). 15 cœurs sur 16 attendent.
- dont `game_bootstrap.obj` seul : **115 s** (une unité de 30 511 lignes : les 26 `game_bootstrap/*.inc` inclus dans un seul `.cpp`, objet de 113 Mo).
- 77 tests unitaires compilés et liés (≈ 3 min), puis exécutés en série (≈ 4 min).
- Le lien de la DLL est court (quelques secondes) : ce n'est pas le goulot.

## Pourquoi chaque correction recompile tout

1. `Build-GameBootstrap.ps1` écrit une liste de commandes et la rejoue entière : il n'y a aucun suivi des dépendances ni des dates.
2. D-0260 : un build dans son propre `-Out` efface ses `.obj` à la fin ; et les pilotes prennent un nouveau dossier par version (`build-t7v`, `t7v4`, `t7v6`), donc il n'y a jamais rien à réutiliser.
3. Le mode incrémental existant (`-ChangedUnits`) est manuel : il faut donner la liste des unités touchées (y compris celles qui incluent un en-tête modifié), avoir gardé les objets (`-KeepObjects`), et il saute **tous** les tests. Personne ne l'utilise, avec raison : une unité oubliée donne un module faux sans erreur.
4. Chaque build réécrit aussi ses en-têtes générés (`game_bootstrap_pins.inc`, `nrd_shaders.h`, `post_glare_table.h`) et réextrait les archives NRD et RTXDI : même avec un suivi des dates, tout paraîtrait modifié. À corriger dans tout schéma incrémental.

## Proposition, en trois étapes

### Étape 1 : paralléliser (le gain le plus sûr)

Le script garde tout ce qu'il fait (contrôles, pins, génération) mais, au lieu de `build.cmd`, il écrit un `build.ninja` à partir de la même liste de commandes. Ninja 1.13.2 est **déjà installé** avec Visual Studio 18 (`Common7/IDE/CommonExtensions/Microsoft/CMake/Ninja/ninja.exe`, livré par Microsoft) : aucun téléchargement. Les tests indépendants (CPU) tournent en parallèle, les tests GPU restent en série.

- Build complet : **≈ 15 min → ≈ 5 min** (chemin critique : `game_bootstrap.obj`, 115 s, plus le lien et les tests GPU).
- Effort : une demi-journée à une journée, revue comprise.
- Preuve d'équivalence : `/Brepro` rend la sortie reproductible, donc la DLL et le contrôleur du nouveau build doivent avoir **le même SHA-256** que ceux du build série sur le même commit.

### Étape 2 : l'incrémental automatique

- `deps = msvc` (`/showIncludes`) : Ninja connaît les en-têtes exacts de chaque unité et recompile seulement ce qui en dépend ; il refait aussi une unité dont la ligne de commande a changé. Les `dxc` peuvent déclarer leurs inclusions (`-MD`/`-MF`, à vérifier sur le `dxc` du SDK installé ; sinon ils dépendent de tous les `.hlsl` et des en-têtes NVIDIA) ; les `fxc` (rapides) relancés quand leur `.hlsl` change.
- Les fichiers générés écrits seulement si leur contenu change ; les archives extraites une fois, marquées par leur hash.
- Les objets dans un cache par fil (`-ObjCache`), séparé du dossier de sortie versionné : les dossiers `build-t7v*` restent des preuves distinctes, mais les objets se réutilisent d'une version à l'autre.
- Les tests : relancés ceux dont l'exécutable a changé.
- Le build de fusion et celui du launcher restent **complets et propres** (le juge ne change pas).

Gain pour une correction typique : une unité du renderer ≈ 30 à 60 s ; un `.inc` de `game_bootstrap` ≈ 2,5 min ; un `.hlsl` ≈ 30 s à 1 min. Effort : environ une journée de plus.

Décision pour Thomas : D-0260 efface les objets pour le disque. Le cache coûte ≈ 0,5 Go d'objets par fil (≈ 0,7 Go avec le PDB) ; il y a 207 Go libres sur C:. Proposition : un cache par fil, purgé par `Clear-DiskCache` à la fusion.

### Étape 3, plus tard et optionnelle : couper `game_bootstrap`

L'unité unique de 30 500 lignes impose 115 s à toute correction d'un `.inc`. La couper en 3 ou 4 unités ramènerait ce cas à ≈ 30 à 40 s, mais c'est un refactor (état partagé entre les `.inc`) avec un vrai risque. À faire seulement si l'étape 2 laisse ce cas trop lent. Les en-têtes précompilés (PCH) apporteraient peu une fois le build parallèle.

## Risques

- **Objet périmé** (le vrai danger de l'incrémental) : couvert par les dépendances exactes de MSVC, la ligne de commande dans `.ninja_log`, et le build complet obligatoire avant toute fusion ; critère d'acceptation : même SHA-256 entre un build incrémental et un build propre du même commit.
- **Charge du PC** : un build parallèle prend tous les cœurs ; un paramètre `-Jobs` (12 par défaut) à baisser pendant le run d'un autre fil. Les règles de mesure ne changent pas (PC entier libre).
- **Mémoire** : 32 Go suffisent pour 12 `cl /O2` en parallèle ; `game_bootstrap` est le plus gros.
- **Ninja dépend de l'installation de VS** : chemin vérifié au démarrage, ligne dans TOOLS.md.
- **Journal** : Ninja garde la sortie de chaque commande séparée, donc les erreurs restent lisibles.
