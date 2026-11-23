# I-0100 - DS3 DX12 Vague 2: no pop-in (Pas de pop-in)

Capability: C3

Opened 2026-10-26 01:05 Paris by the pilot (Opus 5.5), on Thomas's order relayed by the coordinator (2026-10-26 01:02, « le
nouveau pilote du chantier pop-in, que Thomas a validé »). Guiding study: [POPIN-STUDY](../I-0097-stutter/POPIN-STUDY.md)
(written beside I-0097, where the defects were seen; its §5 tranches and §6 criteria are this workstream's plan). The earlier
rule study of 2026-10-24, D-0211's input, is copied here: [POPIN-RULE-STUDY-2026-10-24](POPIN-RULE-STUDY-2026-10-24.md).

## Question

Why does the native renderer show pop-in and flicker the original does not (objects appearing at once along a path, distant
enemies vanishing and coming back, the world drawn incomplete or grey under the loading page), and what makes the native show no
more pop-in and no more flicker than the original on the same path, in the whole game, without any stand-in?

## Scope

Included (Thomas's validated order, 2026-10-26):

1. **C5, the pipeline library** (the grey native title and loading page on the OLED, POPIN-STUDY 2.3 and §3 C5): the library and
   its list stamped with what made them (the module, the adapter, the driver, the D3D12 runtime), any difference discarding
   the library and the list precompiled into a new one at the start, so a library never outlives the build that wrote it.
2. **T0, `popin-observe.flag`** (POPIN-STUDY §4 M1-M2, §5 T0): per-entity visibility rows in both renderers, per view, and the
   models' first admissions during the camera turn (the native frames over 25 ms, 1 to 5 a turn natively, 0 in the original,
   I-0097 RESULT tranche 5).
3. **Preparation at the load**: whether preparing every model of a zone during its loading screen (D-0211, at most + 1 s a load,
   D-0213) removes those first admissions; measured before it is built.
4. Then, as T1 decides: T2a (the draw decision brought back to DS3's), T2b (the modern layer: dithered fades, margins, distant
   characters drawn every frame, preparation ahead), T3 (the load), T4 (the loading page), T5 (streaming zones), T6 (every
   witness zone side by side, D-0181). Known leads: the Undead Settlement's wooden fences and bushes (short LodParam distance,
   the original dithers them in), the Load Game portraits missing once (I-0097 tranche 5's memory leg).

Excluded: any stand-in (D-0197, D-0211); forcing DS3's own loading policy; invented LODs, impostors, mip streaming; the longer
draw distance (ROADMAP, later); the cadence (« Cadence, méthode DX12 moderne », after this workstream, D-0219). Visual defects
met on the way that are not pop-in go to the ROADMAP only.

## Inputs

- Decisions: D-0161, D-0181, D-0197, D-0211, D-0213, D-0218, D-0220 (this workstream's method).
- Results and studies: POPIN-STUDY; I-0097 RESULT tranches 4-6 (E2, G4, the camera turn's first admissions); I-0070, I-0071
  (ER-STUDY C: fades, hysteresis, load balancer); I-0072 (the pipeline cache).
- Evidence: the runs `i97-t5-noai-settlement-*`, `i97-t5c-settlement-{1,2}`, `i97-t5b-ariandel-oled-n*`, the bad library kept
  at `evidence/I-0097/pipeline-library-archive-20261025-0945`.
- Tools: the journals, PresentMon, `Capture-ScreenSeries.ps1`, FLIP (native against original pictures), pixtool (WORKFLOW « Tools »).

## Hypotheses

- H1 (C1): the native's per-frame draw decision differs from the original's emit (fade weights, suppress bytes, the view a weight
  belongs to, the fade drawn as a pop).
- H2 (C2): after a load DS3 shows the world while native content is still being created; D-0197 hides it.
- H3 (C5): a library written by another build returns wrong pipelines (the grey UI).
- H4 (C4): late native content where DS3 streams.

## Expected evidence

Per entity and renderer, from T0's rows on the same replayed path: first drawn distance, on/off changes a minute, frames drawn
with a weight in (0,1) with and without dither; the frame each load's world is complete; pictures native beside original.

## Exit conditions

- Success: POPIN-STUDY §6 criteria 1 to 6 at every witness zone, Thomas's eye last.
- Refuted: a hypothesis whose discriminating measure shows the native equal to the original is closed in the RESULT.
- Insufficient information: recorded per measure, never promoted.
- Stagnation: two failed local fixes of one cause open an RCA (WORKFLOW).
- Budget exhausted: close the tranche and replan under `.project/WORKFLOW.md`.

## Safety and human interaction

Live runs under D-0116/D-0117, announced 5 minutes ahead (Paris times from the PC clock), offline, hash-checked restore; the gate
on the ASUS before any merge that changes `native/`, with its memory leg (`-Memory`) when the branch touches the memory, the loads or the streaming (D-0218); a fresh Opus at xhigh reviews before
every merge. No deletion without Thomas's agreement; Windows and NVIDIA settings stay his.
