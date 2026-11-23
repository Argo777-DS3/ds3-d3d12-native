# Wish: a real texture quality, with the anisotropic filtering apart

A wish of Thomas (this R thread, 2026-11-11 15:23 Paris): « Si un jour on fait un vrai upscale de toutes les textures du jeu, on
pourrait mettre un vrai setting de qualité de textures + remettre la ligne anisotrope ». Not a decision and not on the
[ROADMAP](../ROADMAP.md).

- Why not now: DS3's « Texture Quality » sets only the material samplers' filter and anisotropy (Low trilinear, Medium x4, High x8,
  Max x16), never the textures' resolution ([R12G-STATIC](../../investigations/I-0132-rt-optimisation-zones/R12G-STATIC.md) §1). I-0132
  R12g first put an « Anisotropic filtering » row in its place; Thomas kept the game's row as it is (15:20 to 15:22), a second row on the
  same filter being a duplicate.
- When: with upscaled textures for the whole game (D-0267: the installer carries none; such textures would be a separate package). Then a
  texture quality row choosing the resolution, and the anisotropic filtering (Off, x2 to x16) its own row again (R12g's code, in Git
  history at `5a4e02501`, is the starting point: the settings key, the sampler bank's `PublishAnisotropy`, the row).
