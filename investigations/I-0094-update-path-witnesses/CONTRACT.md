# I-0094 - Travail de DS3 sous le rendu natif

Capability: C3 (Wave 2, performance: the engine thread, bottleneck 5 of [I-0088 BOTTLENECKS](../I-0088-engine-thread-120-c/BOTTLENECKS.md))

## Question

DS3's own work before the collect is about 3.5 ms a frame at m45 under the native renderer (`i93-07-prof-carry`), and I-0078
measured DS3 functions slower under the native renderer than under the original at the same scene (8F3506, 608467, D58DF4: about
0.5 ms together; ER-STUDY §7, P9). In a native run the module keeps eighteen pass-through hooks on DS3's update and UI paths whose
only effect is to log (character and part updates, EzTasks, schedules, UI predicates and input steps, world layer insertions; rows
of earlier investigations), some with a lock and a map lookup per call (`ObserveInputStepRun`, `ObserveInputConfirmUpdate`, the
layer insertions). How much of DS3's work do they cost, and does removing them from the native run give it back?

## ER

ER-STUDY §8 / P9: the observers are ours, not ER's; ER has no such pass-through layer. The change follows the rule « diagnostics
run only behind their flag » (WORKFLOW, Optimise, do not stack).

## Scope

1. In a native run the eighteen witness hooks installed only with `update-observe.flag` (the Reference mode keeps them); kept: the
   texture bind (the default-bound texture registry), the characters' schedule (the test tools and witness moves), the world update
   gate (the game step measure). A/B: `update-observe.flag`.
2. Measured at m45 and on the walk (unlocked, measure mode): DS3's work (the stamp before the collect), frames/s, p50/p99.

Excluded: DS3's limiter spin (P7, no frame gain), the observers' rows themselves (unchanged behind the flag).

## Exit conditions

Success: DS3's work measurably down (or the change kept as a pure diagnostic cleanup if neutral: it removes dead weight from
play), same pictures, validation 0, the gate. Budget 4.

## Safety and human interaction

As I-0093 (announced live runs, offline, Shebang saves, hidden builds; one thread at a time).
