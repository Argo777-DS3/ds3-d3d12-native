# I-0089 - Résultat : outils de test, boss ranimés et invincibilité

## Conclusion

Thomas peut, dans une version de test seulement, ranimer les boss déjà vaincus de sa sauvegarde Shebang (tous, ou ceux qu'il
choisit) et être invincible avec F4. Le lanceur normal n'a rien de tout cela (D-0084: jamais en production).

- **Les 26 drapeaux « boss vaincu »** sont lus dans les scripts du jeu (STATIC): le registre de `common.emevd`, événement 970
  (26 lignes d'initialisation `2000[0] <rang>,970,<drapeau>`), dont 22 sont aussi le premier argument du gabarit de combat de boss
  20005500 des scripts de carte ([boss-flags.json](boss-flags.json)). Les noms anglais du jeu sont déduits de la carte et de
  l'ordre des identifiants (`inferred`), ou laissés ouverts quand l'ordre ne tranche pas entre deux boss d'une carte (`unknown`).
- **Le module** (`04_reference_witnesses.inc`, bloc « Test tools ») ne fait rien sans `test-tools.flag` à côté de lui; seul le
  lanceur de test place ce fichier, dans un dossier à part (`%LOCALAPPDATA%\DS3-DX12-Test`). Avec lui: F4 bascule l'invincibilité
  (PV du joueur remis au maximum à chaque appel de l'ordonnanceur de WorldChrMan, sur le thread de jeu; bip aigu/grave, titre de
  fenêtre `[TEST : invincible]` posés sur un thread à part), et les drapeaux choisis (`test-revive.txt`: `all` ou des
  identifiants du registre) sont effacés par le setter d'événements du jeu `1404C5E20(gestionnaire, id, 0, 0)` une fois le monde
  tournant depuis 180 appels (F5: de nouveau, en attente que le gestionnaire soit prêt).
- **Le lanceur de test** (`tools/Launch-DS3-DX12-Test.ps1`, `.cmd`): sauvegardes copiées puis remises par empreinte à la
  fermeture du jeu (marqueur `PENDING.json` si la restauration est due, `-Restore`), option `-Shebang` (la sauvegarde Shebang
  de `shebang-alpha-prepared-20261007`, empreinte vérifiée et liée au compte Steam). Mode d'emploi: [USAGE.md](USAGE.md).

## Evidence

Cinq exécutions annoncées (D-0116, hors ligne, copie de la sauvegarde de zone Eta de Firelink, restauration par empreinte
`ok: true` chaque fois, 0 faute): `i89-test-1` à `i89-test-4` (journal `startup.jsonl`, lignes `test_*`; les images ont été
supprimées, elles montraient le bureau):

- Test 1: `test_tools_enabled`; `test_invincible {found, plausible, hp 669, hp_max 669}` (la mise en page du module de données
  du personnage `[[[manager+0x80]+0x1F90]+0x18]`, PV en `+0xD8` et maximum en `+0xDC`, est cohérente en jeu: PV égaux au
  maximum au repos); titre `[TEST : invincible]`; aucun menu du jeu ouvert par F4 (pas de conflit de touche). La liste de boss
  n'avait pas été lue (variable PowerShell écrasée dans mon pilote, corrigée).
- Test 2: `test_revive_selection count 2`, `test_revive cleared 2`.
- Test 3: `test_invincible_refill` aux PV 591, 526 et 545 sur 669 (de vrais dégâts, remplis); une chute mortelle a tout de même tué
  le personnage: la limite du coup unique.
- Test 4: voyage au bonfire « Iudex Gundyr » avec Iudex et Champion Gundyr ranimés: **Iudex Gundyr est debout dans son arène**
  (vu à l'écran), alors qu'il est vaincu dans la sauvegarde.
- Test 5 (le lanceur lui-même, `Run-LauncherTest.ps1`, 08 h 20): `Launch-DS3-DX12-Test.ps1 -Shebang -Bosses iudex`, marqueur `PENDING.json` présent pendant le jeu, la Shebang installée, `test_revive cleared 1`, F4 (`test_invincible` 669/669), F5 (`test_revive again true`: la touche et l'attente du gestionnaire prêt fonctionnent), jeu fermé, puis "Sauvegardes restaurées", marqueur supprimé, **toutes les sauvegardes identiques à l'avant par SHA-256** (0 changé, 0 nouveau). Les chemins non joués: la fermeture anticipée de la console (le marqueur et `-Restore`, essayés sur un faux dossier de sauvegardes seulement), `-Keep`, les deux `.cmd`.
- Contrôles de la compilation: tests unitaires du module passent; relecture Opus de l'étape appliquée (restauration des
  sauvegardes durcie, marqueur de restauration due, F4/F5, titre hors du thread de jeu); porte de replay: voir STATE.

## Uncertainty / unresolved questions

- L'invincibilité remplit les PV; un coup ou une chute qui dépasse le maximum tue encore. Le vrai drapeau « pas de dégâts » du
  jeu demande une décompilation (non faite, pour ne pas deviner un décalage).
- Noms des boss `unknown`/`inferred`: confirmer en voyant chaque boss; seul Iudex Gundyr l'a été (le drapeau 14000800).
- La lecture des drapeaux (getter `1404C5350`) n'est pas utilisée: aucun test « tous à 1 dans la Shebang ».
- Les touches : F4 n'a eu aucun effet du jeu constaté (le titre et le journal le prouvent; les deux bips ne sont pas captés par le pilote). F5 : voir le test du lanceur ci-dessous. La table des touches par défaut du jeu n'est pas lue.
- L'octet « prêt » du gestionnaire de drapeaux (`[144752F68]+0x228`) est la première condition du setter `1404C5E20` (decompiled/DS3-1152-F-004C5E20.c), pas une lecture indépendante.
- Un F4 pressé pendant un chargement (joueur nul) annonce « activé » puis « coupé » : le titre peut rester sur « invincible ». Corrigé dans USAGE, pas dans le module (le gate est passé sur cet arbre).
- Steam Auto-Cloud est actif: la sauvegarde de test peut partir au cloud pendant un test.

## Artifacts produced or modified

[boss-flags.json](boss-flags.json), [USAGE.md](USAGE.md), [Run-TestTools.ps1](Run-TestTools.ps1) (pilote des essais),
`tools/Launch-DS3-DX12-Test.ps1/.cmd`, `tools/Launch-DS3-DX12-Test-Bosses.cmd`, `native/frame/game_bootstrap/04_reference_witnesses.inc`.
