# Investigation result

## Conclusion

Complete (2026-11-02, the review thread): a fresh full read of the native code at `main` `831023b8`, every production source
of `native/` but `third_party/` (244 files, 5.0 MB), by the `code-review` skill at xhigh and eight fresh Opus 5.5 reviewers at
xhigh, one lot each. **83 findings (82 distinct): 2 high, 28 medium, 53 low; no high CONFIRMED.** The two high ones are crash
candidates under thread interleavings the code does not exclude (G1: a menu model freed while the offscreen collect calls
engine code on it; C1: the engine's texture registry lookup called unguarded from the admission workers while loader threads
mutate it). The confirmed medium ones that touch play: a hitch per struck enemy (B1), leaked effect buffers and zero-seeded
particles after a dropped frame (D1), effects drawn with a null view (D2, a stand-in), a reflection context kept per output
size (E1), the water's Fresnel stale after an option change (C2), a refused texture closing the game (A3); and a family of
diagnostics active in play without a flag. No D3D11 dependency in the production path. Details, verdicts and the proposed
order: [REVIEW](REVIEW.md).

Nothing was corrected: the corrections and their order are Thomas's to choose (the R pilot, the only pilot from now on, or a
later thread).

## Evidence

- [REVIEW](REVIEW.md): every finding with file and line, severity, status, scenario, fix and the thread's verdict.
- [SKILL-PASS](SKILL-PASS.md): the `code-review` skill's raw findings on `493e58aa`.
- [REVIEW-BRIEF](REVIEW-BRIEF.md): the reviewers' brief and the eight lots.

## Uncertainty / unresolved questions

- Both high findings and ten medium ones are PLAUSIBLE: each names the runtime condition that would decide it (for G1, whether
  the in-world scene CPU update runs on another thread than the menu; for C1, whether a loader thread erases a registry entry
  during an admission).
- Six medium findings and most low ones were not re-read by the thread (REVIEW « Method » 3).
- Out of scope: the unit tests, the PowerShell tools, the documentation; the tranche-17 branch (`claude/compassionate-maxwell-d20ef9`).
- No cost was measured (no build, no run): the per-frame costs are read from the code.

## Artifacts produced or modified

- `investigations/I-0120-full-code-review/` (CONTRACT, REVIEW, REVIEW-BRIEF, SKILL-PASS, RESULT, status.json).
