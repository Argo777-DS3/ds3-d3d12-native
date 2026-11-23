# I-0133 - Study: the encrypted and obfuscated parts of DarkSoulsIII.exe (Étude : les parties chiffrées et brouillées de l'exécutable)

Capability: C3

## Question

Which parts of `DarkSoulsIII.exe` 1.15.2 (SHA-256 `EF5E07C5…B69498C`) are encrypted or obfuscated by the anti-tamper layer,
how much they represent, which systems they touch, and what they really block for decompilation and for rebuilding systems
locally (Thomas, relayed by the coordinator, 2026-11-11 00:36 Paris).

## Scope

Included: the file's PE layout and entropy; the decrypted-code capture of 2026-10-09 already in the evidence store
(`evidence/I-0066/artifacts/i66-reference-code-935`, `.../direct-code-capture-protocol-934/overlay935`); the functions of
`.pdata`; RTTI, call graph and strings for attribution; headless Ghidra 12.1.4 on a read-only mirror; the repository's earlier
notes on encrypted functions.

Excluded: any game launch or new capture; any change to the executable or to the protection; the online side.

## Inputs

- Earlier notes: MENUS-MAP « Encrypted functions on this path » and §77.5, LEGACY-ROOT-INVENTORY-903 « SFX manager and encrypted
  engine code », CADENCE-STUDY « What protected code is », FADE-PLATES-NATIVE, FONTS-LANGUAGES-NATIVE, I-0076, I-0112, I-0114,
  I-0130.
- Tools: `tools/` here (Python, standard library only), `tools/I0133ProtectMap.java`, I-0066's `I0066Overlay936.java`
  (evidence), `Invoke-Ghidra.ps1` with `-ProjectDir ghidra-mirror-i0066`.

## Exit conditions

- Success: sections, sizes and function counts measured; systems attributed with their method stated; blocks listed with
  their evidence; a recommendation.
- Insufficient information: a part that only a run could settle is named, with the run it would take.

## Safety and human interaction

Static only. No live run: the coordinator holds the game slots (another thread's run 00:35-00:45).
