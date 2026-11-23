# I-0139: the brief given to the sheet workers

The closed list ([LIST.md](LIST.md)) is assembled by the pilot from six sheets under `sheets/`, each written by a Sonnet 5.5
worker from this brief and its own category paragraph. Worker output is non-canonical until the pilot checks it (WORKFLOW
« Roles and delegation »).

## Common rules

- Worktree: `C:/Users/User/Desktop/DS3 Decompilation/.claude/worktrees/kind-antonelli-18b0e1` (run every command from there,
  absolute paths; never `cd` to the main checkout). Write only your sheet `investigations/I-0139-game-tour/sheets/<name>.md`,
  helper scripts under `investigations/I-0139-game-tour/tools/`, text dumps under `evidence/I-0139/artifacts/static/<name>/`, and
  extracted game bytes (licensed) under `C:/Users/User/Tools/DS3-Decompilation/gamedata-i0139/<name>/` (never in Git).
- Read-only on the game (`C:/Program Files (x86)/Steam/steamapps/common/DARK SOULS III/Game`): never launch it, never write
  there. No Ghidra unless your paragraph says so. No git commit, no git command that changes anything.
- Every process hidden (no console window on Thomas's PC, D-0199). Python with `-I`; read and write text with
  `encoding='utf-8', newline=''`.
- Names in English exactly as the game shows them (from the game's English FMG text where you can; zone names like
  « Archdragon Peak »). Never translate.
- Each fact carries a status: **STATIC** (read in the game's data or code by you, with the file and the row), **CURATED**
  (from the sl2-analyzer data set `C:/Users/User/.codex/I0033-tools/sl2-analyzer/db_ds3/*.json`, community data, MIT, not
  yet checked against the game), **RECORD** (from a repository record or evidence, cite it), **INFERRED** (deduced: say from
  what), **OPEN** (unknown). Never promote one to another without the reading that justifies it.
- Extraction tools (pinned, read-only): BinderTool v0.5.2 and its dictionary in `C:/Users/User/.codex/I0033-tools/BinderTool-v0.5.2`
  (`investigations/I-0068-static-yebis-ffx-lights-spec/Recover-NamedGameData.ps1` shows how to read named entries from the
  encrypted archives, `-Archives Data0,...`); SoulsFormats 1.0.1 `C:/Users/User/.codex/tools/SoulsFormats-1.0.1/lib/net462/SoulsFormats.dll`
  (DCX, BND4, MSB3, FMG, PARAM; `investigations/I-0067-static-menus-subsystems-map/Get-EmevdDump.ps1` dumps an EMEVD). Paramdefs
  for DS3 params: the sl2-analyzer's `tools/gen_from_paramdex.py` names where its Paramdex came from; if no paramdef is on the PC,
  read the param rows by known field offsets only where a repository record gives them, else mark OPEN.
- The test launcher and runners you describe reaching with: `investigations/I-0089-test-tools/USAGE.md` (F4 invincible, F5
  revive, F6 kill the strongest enemy within 25 m, F7 give items, `test-revive.txt`, `-Bosses`), `.project/LIVE-RUN-NOTES.md`
  (the saves Shebang, Alpha, Eta, Gamma; the Travel page coordinates; teleports in the teleport frame = the MSB frame; a
  teleport goes to a `c0000_*` player-start part or a SpawnPoint region, never to a map piece's origin), I-0132
  `R11-PLAN.md` §12 (Eta holds every bonfire; the fourteen Travel tabs and their rows in pilot units) and `Run-R11cTour.ps1`.
  Alpha is Thomas's real game: a row may use Alpha only through a copy; Eta, Gamma and the other Shebang characters are test
  characters.

## The sheet's form

A short header (what the sheet covers, its sources with paths, what stayed OPEN), then one Markdown table:

| Row | Name | Map | What to look at | How to reach it with the test launcher | Source and status |
|---|---|---|---|---|---|

- **Row**: the category letter and a number (`Z01`, `B01`, ...), stable.
- **How to reach it**: the save to load, the Travel tab and row (pilot units), a teleport `x y z` in the teleport frame, the
  flags to set or clear (the revive file, the event flag ids), the pilot keys; or « not reachable with today's tools: needs X ».
- Keep a row per thing the player sees, not per asset; families of identical rows may be one row with the count.
- End with a « Gaps » section: what the tools cannot reach today and the smallest tool that would.
