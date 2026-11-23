# Import of the timing project (2026-10-21)

Thomas (2026-10-21, his own project, made with Codex/GPT): the knowledge and the decompilation of the Desktop folder
`DS3 Timing Fixes (LTF-TRTF)` come back into this repository. They left it on 2026-10-02 (see [README](README.md)); their
numbers are this project's own historical ones (investigations I-0005 to I-0014, the 55 timing claims of
[knowledge/CLAIMS.md](knowledge/CLAIMS.md), decisions D-0003 to D-0028), so they are kept as they are, under this folder.

Imported (1,504 files, 8.6 MB): every Markdown file, the decisions, investigations and knowledge folders, the manifests, the
static decompilation exports (`evidence/*/static`: C, assembly text, CSV), the prototypes' and tools' sources (C++, scripts).
Left in the Desktop folder (1.6 GB, D-0142: no raw artefact by default): the runtime session captures (`runtime/**/sessions`),
build outputs and binaries, and any file over 256 KB. Links inside these files still use the original layout.

For the native backend (I-0078): the game's cadence is the central timing object at `[0x14489DD10]`, requested rate float
`+0x354` and its enable byte `+0x358` (I-0009, I-0011; K-0013 to K-0020 there); the present pacer `0x1400D5CB0` /
`0x1400D5E30` (K-0009 in the main ledger) is a second, presentation-side limiter. Above 60 frames/s, the locomotion (LTF) and
terrain response (TRTF) fixes of I-0007 and I-0013 are needed for correct gameplay.
