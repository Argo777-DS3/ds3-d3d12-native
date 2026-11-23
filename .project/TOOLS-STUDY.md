<!-- Copied into the repository on 2026-10-25 by the repository audit from the project files (etudes/outils-externes/OUTILS-FLIP-GHIDRAMCP-PIXTOOL.md), so that the repository holds it (WORKFLOW « Canonical sources »). Answered since: the Ghidra installed is 12.1.3 (C:/Users/User/Tools/DS3-Decompilation/ghidra_12.1.3_PUBLIC); pixtool.exe is in C:/Program Files/Microsoft PIX/2603.25; Python is not installed on this PC (FLIP needs it). The rules for using these tools are in WORKFLOW « Tools ». -->

# Étude : NVIDIA FLIP, serveur MCP pour Ghidra, pixtool (25/10, recherche en ligne seulement)

Rien n'a été installé ni lancé : cette étude vient uniquement de pages web. **[SOURCÉ]** = lu sur la page citée. **[GÉNÉRAL]** = connaissance générale, non vérifiée ici. Les degrés de certitude sont mes estimations.

## Classement

| Outil | Gain | Risque | Certitude que ça marche chez nous |
|---|---|---|---|
| FLIP (Python `pip`) | Élevé pour le pop-in (mesure automatique) | Faible (local, lecture d'images) | ~80 % |
| pixtool (captures GPU) | Moyen à élevé (défauts visuels, comparaison des deux rendus) | Moyen (injection d'une DLL dans le jeu) | ~60 % pour le lancement automatique, à valider par un essai à sec |
| Serveur MCP Ghidra | Moyen (moins de scripts ad hoc) | Moyen (port local, écritures dans le projet, conflits entre fils) | ~55 % sur notre base de ~60 Mo, jamais testé |

---

## 1. NVIDIA FLIP

**Ce que c'est.** Métrique d'écart entre deux images « comme l'œil humain le voit » (LDR-FLIP et HDR-FLIP), éditée par NVIDIA, licence BSD-3. Version 1.7. **[SOURCÉ]** <https://github.com/NVlabs/flip>

**Installation sous Windows (le plus simple).** **[SOURCÉ]** même page.
- Python 3.10 ou plus, puis `pip install flip-evaluator`. Aucun binaire Windows prêt à l'emploi n'est annoncé ; la version C++/CUDA se compile (solution `src/cpp/FLIP.sln` ou CMake), donc inutile ici.
- Pour ne rien voir à l'écran : lancer en `pwsh -NoProfile -WindowStyle Hidden`, ou depuis un fil Claude Code (déjà sans fenêtre).
- Le module Python est appelé par `python -I -m flip_evaluator ...` ou par un script (voir `src/python/api_example.py` du dépôt, qui montre l'API). Je n'ai pas pu lire la signature exacte de la fonction : à lire dans ce fichier avant d'écrire le script.

**Utilisation en ligne de commande.** `flip -r reference.png -t test.png` ; `flip -h` liste les options. **[SOURCÉ]**
- Sorties : une carte d'erreur PNG `flip.<ref>.<test>.<ppd>ppd.ldr.png` (nom fixe, option `--basename` pour le changer) et un fichier texte `pooled_values...txt` avec moyenne, médiane pondérée, quartiles, min, max. L'exemple de la doc affiche « Pixels per degree: 67 » par défaut. **[SOURCÉ]** <https://github.com/NVlabs/flip/blob/main/src/python/README.md>
- Valeur 0 = identique, 1 = très différent. Plus la moyenne est basse, mieux c'est. **[GÉNÉRAL]**

**Comment nos fils l'utiliseraient pour le pop-in.** **[GÉNÉRAL, ma proposition]**
1. À chaque image du trajet de caméra, enregistrer une capture « natif » et une capture « original » à la même résolution (1280×720 fenêtré, comme nos tests).
2. Lancer FLIP sur chaque paire, lire la moyenne et la carte d'erreur.
3. Série temporelle : un objet qui apparaît en retard donne un **pic de moyenne localisé** sur quelques images, avec une tache compacte dans la carte d'erreur (seuil de départ à calibrer, par exemple pixels > 0,3 formant une tache > quelques centaines de pixels). Ces seuils sont des valeurs de départ de ma part, pas de la doc.
4. Calibrage obligatoire : mesurer d'abord original contre original (deux runs identiques) et natif contre natif. Ce bruit de fond fixe le seuil. On suit déjà cette logique avec les bases par écran du Replay Gate.

**Limites quand les images ne sont pas alignées.** **[GÉNÉRAL, certitude ~85 %]**
- FLIP compare pixel à pixel (avec un filtrage lié à la distance de vue) et ne recale pas les images. Un décalage de caméra d'un ou deux pixels, un jitter, ou deux instants différents donnent des erreurs le long de tous les bords, et le signal du pop-in disparaît dans ce bruit.
- Il faut donc des paires prises à la **même image logique** (même position/caméra rejouée, c'est l'idée « rejouer les positions » mise de côté), sans animation différente (PNJ, particules, eau, ombres tramées qui changent d'une image à l'autre).
- Les différences de rendu normales (fondus tramés, éclairage natif vs D3D11) laissent un écart permanent : on suit donc la **variation** de l'écart dans le temps, pas sa valeur absolue.
- Les captures d'écran de la fenêtre doivent être sans HUD ni curseur, en sRGB, même taille.

---

## 2. Serveur MCP pour Ghidra

**Projets existants (tous Apache-2.0).**

| Projet | Mode | Ce que dit la source | Maturité |
|---|---|---|---|
| **LaurieWired/GhidraMCP** <https://github.com/LaurieWired/GhidraMCP> | Extension Ghidra (GUI) + pont Python | Décompiler, renommer, lister fonctions/imports/exports ; « Ghidra 12.1.4 », Python 3.10+ ; serveur HTTP Ghidra sur `127.0.0.1:8080` ; commande Claude Code : `claude mcp add ghidra -- uvx --from git+https://github.com/LaurieWired/GhidraMCP ghidra-mcp --ghidra-server http://127.0.0.1:8080/` **[SOURCÉ]** | ~11,5 k étoiles, 55 tickets ouverts **[SOURCÉ]** ; jeu d'outils « ciblé » **[SOURCÉ, dev.to]** |
| **bethington/ghidra-mcp** <https://github.com/bethington/ghidra-mcp> | GUI + mode sans interface | 209 outils, port `127.0.0.1:8089`, jeton d'authentification optionnel (obligatoire hors loopback), scripts désactivés par défaut, `GHIDRA_MCP_FILE_ROOT` pour limiter les chemins, chargement paresseux des outils ; Ghidra 12.1.3 ; installation via Java 21 + Maven + `python -m tools.setup` **[SOURCÉ]** | ~5,7 k étoiles, 38 tickets, très actif **[SOURCÉ]** ; installation plus lourde |
| **cyberkaida/ReVa** <https://github.com/cyberkaida/reverse-engineering-assistant> | Extension GUI + mode sans interface | Ghidra ≥ 12.0 ; HTTP sur `localhost:8080/mcp/message` ; `claude mcp add --scope user --transport http ReVa -- http://localhost:8080/mcp/message` ; travaille sur le projet **ouvert** dans Ghidra ; clé d'API si exposé **[SOURCÉ]** | ~820 étoiles, 15 tickets **[SOURCÉ]** ; adossé à Claude Code (marketplace de compétences) |
| **clearbluejar/pyghidra-mcp** <https://clearbluejar.github.io/posts/pyghidra-mcp-headless-ghidra-mcp-server-for-project-wide-multi-binary-analysis/> | Sans interface (importe des binaires) | `uvx pyghidra-mcp /chemin/binaire.exe` ; décrit « beta » ; la doc ne montre pas comment ouvrir un projet `.gpr` existant **[SOURCÉ]** | ~450 étoiles **[SOURCÉ, dev.to]** |

Comparatif : <https://dev.to/leemie8848/ai-reverse-engineering-tools-compared-rea-ida-mcp-ghidramcp-and-friends-oct-2026-3hfl>

**Notre cas (projet existant, base x64 ~60 Mo).**
- Aucune des pages ne parle de performances sur de gros binaires **[SOURCÉ : absent]**. Je ne peux donc pas garantir le comportement. **[GÉNÉRAL]** : les outils de décompilation par fonction ne dépendent pas de la taille totale une fois le projet analysé ; le coût est à l'ouverture et à la décompilation de chaque fonction. À mesurer sur une demi-douzaine de fonctions connues.
- **Version de Ghidra** : les extensions sont compilées pour une version précise (GhidraMCP annonce 12.1.4, bethington 12.1.3, ReVa ≥ 12.0). Il faut vérifier la version de Ghidra installée chez Thomas avant tout choix : c'est la première contrainte.
- **Mode GUI sur le projet ouvert** (GhidraMCP, bethington, ReVa) est le seul qui réutilise directement notre projet existant **[SOURCÉ pour ReVa ; déduit pour les deux autres]**. Le mode sans interface importe en général une copie du binaire dans un projet temporaire, donc perd nos noms et annotations. **[GÉNÉRAL ~75 %]**

**Recommandation.** Essayer **ReVa** en premier (une seule commande Claude Code, lit le projet ouvert, authentification disponible), sinon **LaurieWired/GhidraMCP** (le plus répandu, le plus simple). **bethington** seulement si on veut des outils d'écriture riches ; 209 outils chargent beaucoup de contexte. Pas de pyghidra-mcp tant qu'on ne sait pas ouvrir un `.gpr`.

**Branchement à Claude Code.** Une ligne `claude mcp add ...` par poste (voir tableau), puis `/mcp` pour vérifier. **[SOURCÉ]**

**Risques.**
- *Port réseau* : tous écoutent en local (127.0.0.1) par défaut **[SOURCÉ]**. Ne jamais les ouvrir sur le réseau ; si besoin, jeton d'authentification. Un autre programme local peut quand même s'y connecter.
- *Écriture dans le projet* : renommer/typer modifie notre base de décompilation, qui est la référence du projet. Prévoir une copie du projet avant le premier essai et une règle « lecture seule sauf accord » (désactiver les scripts, `GHIDRA_MCP_ALLOW_SCRIPTS` désactivé par défaut chez bethington **[SOURCÉ]**).
- *Conflit entre fils* : une seule interface Ghidra ouverte sert un seul serveur. Plusieurs fils en même temps = requêtes entrelacées et risque de blocage ; cela recoupe la règle « Ghidra des autres fils seulement dans un créneau donné par le pilote ». **[GÉNÉRAL]**
- *Stabilité* : projets jeunes, tickets ouverts (55 pour GhidraMCP) **[SOURCÉ]** ; installer depuis les dépôts officiels listés ci-dessus seulement (supply chain).
- Un redémarrage de Ghidra coupe le serveur.

---

## 3. pixtool (PIX en ligne de commande)

**Ce que c'est.** `pixtool.exe` installé avec PIX dans `%programfiles%\Microsoft PIX`, outil pour lancer une application en capture GPU, prendre des captures, ouvrir une capture existante, exporter la liste d'événements en CSV, sauvegarder un rendertarget ou le tampon de profondeur en PNG, extraire la capture d'écran enregistrée. **[SOURCÉ]** <https://devblogs.microsoft.com/pix/pixtool/> Aide : `pixtool --help`.

**Ce qui est documenté (exemples exacts).** **[SOURCÉ]**
```
pixtool.exe open-capture mycapture.pix3 save-resource out.png
pixtool.exe open-capture mycapture.pix3 save-resource out.png --marker=Terrain --depth
pixtool.exe open-capture citydemo.pix3 save-event-list events.csv --counter-groups=D3D*
```
(le blog utilise des tirets typographiques, il faut taper `--`). `pixtool.exe launch <exe>` existe aussi **[SOURCÉ : <https://lolbas-project.github.io/lolbas/OtherMSBinaries/Pixtool/>]**.

**Ce que je n'ai pas pu confirmer.** Les pages lues ne donnent pas la syntaxe exacte de `launch` avec arguments, d'`attach`, de `take-capture` (choix de l'image de départ, nombre d'images) ni de `save-capture`. Un projet tiers, pix-mcp, les enchaîne (`attach <pid>`, `take-capture`, `save-capture`; `launch` avec paramètre `frames`) **[SOURCÉ : <https://glama.ai/mcp/servers/elliotttate/pix-mcp>]** mais sans montrer les options. **[GÉNÉRAL ~65 %]** : la forme est du type `pixtool launch "jeu.exe" --command-line="..." take-capture --frames=1 save-capture sortie.wpix`. Il faudra vérifier avec `pixtool --help` et l'aide de chaque commande avant d'écrire quoi que ce soit.

**Par nom de processus ou en lançant.** Le lancement par pixtool injecte la DLL de capture automatiquement **[SOURCÉ]** <https://devblogs.microsoft.com/pix/programmatic-capture/>. L'attache à un processus déjà lancé est « expérimentale », et la DLL doit être chargée avant le premier appel D3D12 comme `D3D12CreateDevice` **[SOURCÉ]** <https://devblogs.microsoft.com/pix/taking-a-capture/>. Pour nous, **lancer via pixtool est donc la voie**, l'attache ne suffira pas.

**Déclencher à une image donnée.** pixtool seul : la doc lue ne le décrit pas. Deux pistes :
1. Le raccourci F11 ou Impr. écran dans le jeu (simulable par envoi de touche) **[SOURCÉ]** : pas précis à l'image près.
2. Capture programmatique dans notre propre module (`PIXGpuCaptureNextFrames` pour la prochaine présentation, `PIXBeginCapture/EndCapture`) avec la bibliothèque WinPixEventRuntime et `WinPixGpuCapturer.dll` chargée avant la création du périphérique **[SOURCÉ]**. C'est un hook de développement : conforme à la règle seulement s'il reste hors production.

**Ce qui peut gêner.**
- *Droits administrateur* : la capture GPU n'en demande pas ; la capture de timing, oui **[SOURCÉ]** <https://devblogs.microsoft.com/pix/programmatic-capture/>. (Rappel : Nsight demande déjà des droits admin, PIX GPU non.)
- *DLL injectée* : `WinPixGpuCapturer.dll` n'est pas durcie contre les usages D3D12 invalides **[SOURCÉ]**. Notre rendu natif peut déclencher des erreurs que la couche de capture ne supporte pas. Le risque est qu'un plantage vienne de PIX, pas de nous. Ne jamais lancer sur la sauvegarde de Thomas.
- *Lanceur / jeu protégé* : pointer l'exécutable principal, pas un lanceur **[SOURCÉ]**. Notre chaîne a déjà un démarrage particulier (module + lanceur DS3 DX12) : à tester en mode hors ligne et à sec avant tout test annoncé. **[GÉNÉRAL]**
- *Surcoût* : une capture GPU ralentit nettement l'image et produit un fichier lourd ; incompatible avec une mesure de cadence, uniquement pour des défauts précis.
- *Overlay* : le jeu s'ouvre normalement ; l'interface de PIX n'est pas nécessaire en ligne de commande, mais je n'ai pas lu de confirmation explicite **[SOURCÉ : absent]**.

**Usage par un fil Claude Code (hypothèse).** 1) `pixtool` lance le jeu avec le module, capture N images à un point donné, écrit un `.wpix` ; 2) `open-capture` + `save-resource`/`save-event-list` sortent les cibles de rendu en PNG et la liste des dessins en CSV ; 3) comparer les listes de dessins natif/original, et passer les PNG dans FLIP. Les captures sont des fichiers de plusieurs centaines de Mo : à garder dans le dossier d'évidence, pas dans Git.

---

## Ordre conseillé
1. **FLIP** : installation sans risque (`pip`), à essayer tout de suite sur deux captures existantes de `captures/origine-vs-natif/` pour vérifier que le signal se voit.
2. **pixtool** : essai à sec annoncé (sans le jeu de Thomas), en commençant par l'aide de la ligne de commande.
3. **Serveur MCP Ghidra** : après vérification de la version de Ghidra, sur une copie du projet.

## Questions ouvertes (aucune réponse dans mes sources)
- Version exacte de Ghidra installée chez Thomas.
- Syntaxe exacte de `pixtool launch/take-capture/save-capture`.
- Temps de réponse de ces serveurs MCP sur notre base de ~60 Mo.
- Seuils FLIP : à calibrer par nos propres mesures.
