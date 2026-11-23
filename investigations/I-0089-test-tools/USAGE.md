# Version de test : mode d'emploi (Thomas)

Pour tes tests seulement. Le lanceur de jeu (`Launch-DS3-DX12-Borderless.cmd`) ne change pas et n'a rien de tout cela.

## Double-clic

| Fichier (dossier `tools/`) | Ce qu'il fait |
|---|---|
| `Launch-DS3-DX12-Test.cmd` | La version de test avec ta sauvegarde actuelle : F4 = invincibilité. |
| `Launch-DS3-DX12-Test-Bosses.cmd` | La sauvegarde **Shebang** installée toute seule, tous les boss ranimés, F4 et F5. |
| `Launch-DS3-DX12-Test-Popin.cmd` | Pour observer le pop-in sans te faire tuer : ta sauvegarde actuelle, les ennemis sans IA (ils ne te prennent pas pour cible), chaque objet dessiné noté dans le journal (`%LOCALAPPDATA%\DS3-DX12-Test\test-*.jsonl` ; à côté, `investigations\I-0100-popin\Watch-PopinPlay.ps1 -Test` le suit en direct), F4. |

## Les touches (en jeu)

- **F4** : invincibilité activée ou coupée. Bip aigu = activée, bip grave = coupée. Le titre de la fenêtre affiche `[TEST : invincible]`.
- **F5** : ranime de nouveau les boss choisis (bip moyen) ; sans choix (lanceur simple), il ranime **tous** les boss de la sauvegarde en cours.
- **F6** : tue l'ennemi aux PV maximum les plus hauts à moins de 25 m (le boss de l'arène) ; bip aigu = fait, bip grave = rien trouvé (I-0113, pour voir sa mort). Il peut aussi tuer un PNJ amical (une marchande à Firelink) : la sauvegarde de test est restaurée à la fermeture.
- **F7** : donne au personnage chargé les objets listés dans `test-give.txt` à côté du module (le dossier de build du run ; le lanceur de test ne le pose pas : à copier à la main) (une ligne par objet : `g` + numéro pour un objet comme un sort, `w` une arme, `p` une armure, `a` un anneau ; la liste des 106 sorts du jeu est `investigations/I-0113-open-gaps/test-give-spells.txt`). Bip aigu au début et à la fin. Les fenêtres « objet obtenu » défilent ensuite : E pour les valider. À n'utiliser que sur une copie de sauvegarde (I-0113).
- **F11** : harmonise dans l'emplacement 0 le sort suivant de `test-attune.txt` à côté du module (un identifiant de sort par ligne, `g` devant facultatif), le sélectionne et remplit les PM ; une ligne `test_attune` par appui. Sans ce fichier : un bip grave, rien d'autre. Sert à l'audit des sorts (`investigations/I-0140-effect-families/Run-I140Spells.ps1`, STUDY-ATTUNE).
- Après avoir ranimé : repose-toi à un feu de camp ou voyage. La zone se recharge et le boss est de retour dans son arène.
- Limites : un F4 pressé pendant un chargement peut laisser le titre sur « invincible » alors qu'elle est coupée (appuie de nouveau). L'invincibilité remet les PV au maximum à chaque image. Un seul coup (ou une chute) qui dépasse tes PV maximum te tue encore.

## Les sauvegardes

Le lanceur copie tes sauvegardes dans `%USERPROFILE%\.ds3-test-backups\<date>`, joue sur une copie (ou sur la Shebang), puis remet tes fichiers tels quels, vérifiés un par un, quand tu fermes le jeu. Ne ferme pas la fenêtre du lanceur avant la fin. Si elle a été fermée trop tôt, le lancement suivant refuse de continuer et dit quoi faire :

```
pwsh -File tools/Launch-DS3-DX12-Test.ps1 -Restore
```

## Choisir les boss

```
pwsh -File tools/Launch-DS3-DX12-Test.ps1 -Shebang -Bosses vordt,iudex
pwsh -File tools/Launch-DS3-DX12-Test.ps1 -Shebang -Bosses m30_00
pwsh -File tools/Launch-DS3-DX12-Test.ps1 -ListBosses
```

`-Bosses` accepte `all`, un identifiant de drapeau, un nom court de `-ListBosses` ou une carte (`m30_00`). Les noms viennent du jeu en anglais ; ceux marqués `inferred` ou `unknown` dans [boss-flags.json](boss-flags.json) sont déduits de la carte, pas lus dans le jeu.

Avec `-Keep`, la sauvegarde de test (la Shebang, ou tes boss ranimés) reste en place ; pour revenir en arrière, recopie les fichiers du dossier `%USERPROFILE%\.ds3-test-backups\<date>` dans `%APPDATA%\DarkSoulsIII` (jeu fermé). Si une restauration tardive écrase ce que tu as joué entre-temps, les fichiers écrasés sont gardés dans `_avant-restauration` de la copie.

Steam Cloud est actif pour Dark Souls III : la sauvegarde de test peut être envoyée au cloud pendant le test ; la sauvegarde restaurée y repart au lancement suivant (même comportement que les autres tests du projet).

Autres options : `-NoAi` (ennemis sans IA, `test-no-ai.flag` : un outil de test, le lanceur de jeu le refuse toujours), `-PopinObserve` et `-DiagnosticFlags crossing-observe` (le journal du pop-in, comme `Launch-DS3-DX12-Popin.cmd`), `-Borderless` (fenêtre sans bordure), `-Keep` (garder les sauvegardes telles que le test les a laissées), `-SaveCopy <chemin> -SaveCopySha256 <empreinte>` (une autre sauvegarde de test).

Hors ligne : comme tous les tests du projet, le jeu démarre sans les services en ligne (les deux avis « hors ligne » au titre). Ne joue jamais en ligne avec cette version.
