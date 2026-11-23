# Third-party notices of the native module's own source

What the module's own source files took from published code, with the notices their licences ask for. Opened by I-0138
tranche 6 (2026-11-14) with the tone-map candidates; it lists only what is written into `native/`'s own files. The libraries
the build links or copies beside the module (Streamline, NRD, SHaRC, RTXDI, the Agility SDK, MinHook, zlib) keep their own
licence files where WORKFLOW « Tools » places them; gathering all of them for a distributed build is ROADMAP workstream 10's
and 18's work, which starts from this file.

## AgX tone mapper (Google Filament)

- Used in: `native/renderer/shaders/post_tone.hlsl` (`Agx`, `AgxContrast`, the matrices `kSrgbToRec2020`, `kRec2020ToSrgb`,
  `kAgxInset`, `kAgxOutset`) and `native/renderer/tone_looks.h` (the same arithmetic in C++ doubles, for the unit tests and
  the grey match).
- From: Filament, `filament/src/ToneMapper.cpp` (`AgxToneMapper`, the look `NONE`), `filament/src/details/ColorGrading.cpp`
  (the conversion to Rec. 2020 before the tone mapper and back after it) and `filament/src/ColorSpaceUtils.h` (the colour-space
  matrices), <https://github.com/google/filament>, read 2026-11-14.
- Notice of the source files: « Copyright (C) 2021 The Android Open Source Project ». Licensed under the Apache License,
  Version 2.0 (the "License"); you may not use these files except in compliance with the License. A copy of the License is in
  [`licenses/Apache-2.0.txt`](licenses/Apache-2.0.txt) and at <http://www.apache.org/licenses/LICENSE-2.0>. Unless required
  by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
- Changes made here (Apache-2.0 §4b): the tone mapper and the two conversions around it were written again in HLSL and in
  C++ doubles; the matrices are written as rows and the two products (`sRGB_to_Rec2020`, `Rec2020_to_sRGB`) and the inverse
  (`AgXOutsetMatrix`) as their computed values; the input is clamped to the largest half float before the curve; the looks
  `GOLDEN` and `PUNCHY`, the NEON paths and the rest of the colour grading are not taken.
- Filament's own credits, kept: the AgX matrices « taken from Blender's implementation of AgX »
  (<https://github.com/EaryChow/AgX_LUT_Gen/blob/main/AgXBaseRec2020.py>); the contrast curve « Adapted from »
  <https://iolite-engine.com/blog_posts/minimal_agx_implementation>. Filament's repository has no `NOTICE` file at its root
  (checked 2026-11-14).

## ACES fit by Stephen Hill (Baking Lab)

- Used in: `native/renderer/shaders/post_tone.hlsl` (`Aces`, `kAcesInput`, `kAcesOutput`) and `native/renderer/tone_looks.h`
  (`Aces`, `AcesFit`, the same two matrices).
- From: Baking Lab by MJP and David Neubelt, `BakingLab/ACES.hlsl`, <https://github.com/TheRealMJP/BakingLab>, read
  2026-11-14. Its header: « The code in this file was originally written by Stephen Hill (@self_shadow), who deserves all
  credit for coming up with this fit and implementing it. »
- Changed here: the input is clamped to the largest half float before the fit; written again in C++ doubles for the tests.
- Licence (MIT), the notice to keep:

```
MIT License

Copyright (c) 2016 MJP

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
```

Not a legal opinion: the licences' texts as read on the date above (`evidence/I-0138/artifacts/tranche-6-licences/`).
