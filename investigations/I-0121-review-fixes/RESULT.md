# Investigation result

## Conclusion

Complete (2026-11-02 to 11-03, the R pilot): [I-0120](../I-0120-full-code-review/REVIEW.md)'s priorities 1 to 3, in Thomas's
order, and the skill pass's S1 and S6, corrected; merged `7cd9282c` (two step reviews and the final review answered, build-7 every
unit test passing), the launcher rebuilt, start check `start-7cd9282c` passed (370 world frames, a clean close, no refusal or
crash). Each fix is structural: neither high finding (G1, C1) was ever seen in a run, so none is proved by a reproduction. Left
open: H2 (the witness-file probes: a flag of their own for the runners), I-0120's priorities 4 and 5 and A1 (Thomas's choice).

## Fixes

| # | Fix | Where |
|---|---|---|
| G1 | The offscreen collect marks the holder it reads busy (only if still armed and still bound to the model it was listed with, from its camera to its lighting); the two retire hooks (D66280, D68EB0), after unarming it, wait on a condition variable until the collect has left it, then let the original free the model. The collector's own thread never waits (a retire inside its engine calls would deadlock); past 5 s a row `native_offscreen_retire_waiting` says so and the hook waits on (never freed under the collector). | `16_offscreen_and_movies.inc` |
| C1 | The module's D24490 lookups (`NativeTextureRegistryFind`: the texture holders and the lightmap witness) and the six observed mutators (D4F250, D4E100, D4F690, D4EE10, D4F4B0, D4F7A0) meet through two counts, not a lock: a lookup counts itself a reader and gives up while a mutator runs (`busy`: nothing kept; an admission leaves the binding unbound, a row sampling it refused that frame, and the model unrecorded, admitted afresh at the next collect; a rebind leaves the binding as it is); the outermost mutator on a thread counts itself a writer and waits only for the lookups in flight (D24490 alone, microseconds). Writers do not exclude each other, as in the original, so no order is added between engine threads (the final review's M1: D4E100 holds the engine lock 144793D28 across a whole TPF creation and its fragments' join, which a shared_mutex made every lookup wait on). Left as in the original: the engine's own slot-6 lookups, the unobserved mutators D4A120 (through D4C200 -> D50AB0) and E4B1A0/E4B8C0, and the Reference-only lightmap probe of `04c_scene_witnesses.inc` (cascade-observe.flag). | `08_model_programmes.inc`, `01_state.inc` (the lock-order sheet) |
| B1 | The paint epoch left the records' epochs: a record bound to a GXDecalReceiver's paint views (the bloody provider, patch table 3D41DF0) keeps its receiver's t41 view as read before its admission and is re-admitted only when that view changes, so a target made or retired re-admits only its own receiver's record and no longer sweeps the unvisited ones. `native_paint_epoch`, then unread, removed. | `05_selected_scene.inc`, `08_model_programmes.inc`, `09_model_admission.inc` |
| D1 | `NativeEffectFrame::consumed`, set when the stage takes the frame's releases and creation bytes (`Objects`, `Retain`); the builder's frame carries a deleter (its last holder, any thread) that gives its releases back when the stage never took it (its scene not prepared, a render-extent change, the DLSS latch, Encode stopped first, or Execute threw), and otherwise reports the buffers whose creation bytes it carried; every frame carries a buffer's creation bytes until such a report (the final review's L3: dropped and taken frames alternating left a buffer without them). The stage makes an effect buffer only from its creation bytes, its statements skipped meanwhile, so no particle buffer is made with zero seeds. | `21_effects.inc`, `effect_stage.h`, `effect_stage.cpp` |
| D2 | No effect statement runs on a null view (D-0197): a classic draw, the particles' draw or a dispatch whose FFX texture or SYSTEX_WHITE is not resident is dropped for that frame and counted; at the stage, a draw sampling the depth or the scene copy while the frame has none, any statement whose effect-record view or UAV has nothing to read (an effect buffer not made yet) are skipped, and a classic draw binds an effect record's view as the particles' path does. A dispatch's UAV counters are written only once every UAV's view is made (the final review's L1). An unknown bound handle stays counted only (it may be a slot the programme does not sample). | `21_effects.inc`, `effect_stage.cpp` |
| E1 | A view context records the frame it was last used in; past four contexts, the least recently used of those idle for 600 frames is retired by ticket (`RetireAfterCurrentRecording`), so the reflection's former output sizes no longer stay for the session while the menu views keep theirs. | `scene_pass.h`, `scene_pass_views.cpp` |
| F1 | A BLAS build's completion token also sets the pending entry's `built` flag; `Advance` frees a destroyed model's postbuild slot only once its build list completed (or was dropped uncommitted), so a build reusing the slot can no longer share a submission with it and read its compacted size. | `rt_scene.h`, `rt_scene.cpp` |
| S1 | A slice the collect never reached (no position stride: no programme set, no policy, failed bounds) gets no buffer view in its material record, so a hit on it shades with the ray's normal instead of its zero index and position fields (the rays' shading reads them since R3c); a blend's layer records keep theirs (the final review's H1: the first version wiped R3c's layers, which the unit test caught; its fixture's blend slice given a stride, as every reached slice has). | `rt_scene.cpp`, `rt_scene_test.cpp` |
| S6 | A slice that ends with no material gives back what it added to its set's pending state and kept textures, unless its albedo itself was the one waiting: a set is no longer made again every 30 walks for a specular, bump or emissive stand-in it never reads (the map blends' case). | `12_scene_collect.inc` |
| Priority 3 | The flagless diagnostics behind their flags (a Sonnet pass re-read by the pilot; three of its trailing comments had swallowed the code after them, corrected): A5 the 1172-1177 pass-through witnesses (10 hooks) in `witness_only` (update-observe.flag), not `ObserveWindowClose` (popin-observe reads `popin_page_closed`) nor the title step's runs (the runners wait on `title_step_state` for a loaded save: the step review's H1); H1 the world gate's and the schedule's investigation rows and their reads behind update-observe.flag (Reference mode on), `RecordGameStep` and the test tools unconditional; B2 the part tally and C3 the gate maps only while the 40-row summary can log (`NativeAsmSummaryOpen`), `CapOpen` (a relaxed read) before the capped counters; B3, B7, C8 behind light-observe.flag; D5 behind ffx-record.flag; T1 the play launcher writes `no-kprofile.flag` beside `measure.flag` (the test launcher keeps the profile). H2 not fixed: the runners write the witness files without test-tools.flag (Run-ZoneWalk, Run-AlphaCast, Pilot-LoadGameThomas); a flag of their own written by Run-StartupTrace is the fix, left for a later pass. | `04`, `05`, `08`, `10`, `13`, `13b`, `21`, `22_setup.inc`; `tools/Launch-DS3-DX12.ps1` |

Consequences for the tools: `native_pass_rows` (read by I-0068's `Project-Foliage.ps1`) and `world_update_gate` (read by I-0100's
closed `Run-I100LodScaleSeries.ps1` for a death) now need light-observe.flag and update-observe.flag.

## Reviews

- Step review (a fresh Opus at high effort, on `c83e115e..4646800a`): 1 high, 2 medium, 5 low. Fixed: H1 (A5 hid
  `title_step_state`), M3 (D2 incomplete: the particles and dispatches), L4 (G1 freed the model after its 5 s wait), L5 (a reused
  holder address), L6 (B1 per receiver), L7 (C1's depth without a guard). M2 (H2 silenced the runners' witness files): reverted,
  H2 left open.
- Final review (a fresh Opus at xhigh, on `main...771e1ba0`): 1 high, 1 medium, 7 low. Fixed: H1 (S1 wiped the blends' layer
  records; build-4's unit test failed on it), M1 (C1's lookups waited on whole TPF creations: the counts above), L1 (a dispatch's
  counters before its UAV check), L2 (a classic draw's effect-record view), L3 (D1's creation bytes after alternating drops), L4
  (the lock-order sheet), L5 (I-0100's series asks update-observe.flag), L7 (the records). L6 (style) with L1's rewrite. L2's
  unknown bound handle kept counted only (above).
- Step review of those answers and of F1, S1, S6 (a fresh Opus at high effort, on `771e1ba0..184c9c51` and `22fc0236..3c910f36`):
  1 medium, 4 low. Fixed: M1 (a lookup given up still handed its partial holders on: an admission bound a fallback, a rebind
  rebound to one; now `busy` reaches both), L3 (comments and records). Kept: L1 (D1: a buffer released before the frame that
  carried its bytes is reported taken leaves its key in the taken set, one entry per short-lived buffer for the session, serials
  being unique); L2 (a lookup inside a mutator on its own thread runs at once: no module lookup runs there); L4 (the lightmap
  witness logs a given-up lookup as no holder).
- Step review of that answer (`184c9c51..d2940be5`): 1 medium, 2 low. Fixed: M1 (a binding given up at admission whose next
  lookup found no holder never took its fallback while the record was reused: now a model with a given-up lookup is not recorded
  and is admitted afresh at the next collect), L2 (a revalidation with a given-up lookup reported a false mismatch: skipped),
  L3 (the comment).

## Evidence

- clangd on `game_bootstrap.cpp`: 0 errors at each commit; `effect_stage.cpp`, `scene_pass_views.cpp` and `rt_scene.cpp`: only
  clangd's own tweak self-tests fail. clangd reported 0 errors on the Sonnet pass whose comments had swallowed code: the MSVC
  build and its unit tests stay the gate.
- Builds (`evidence/I-0121/build-N/build.log`): build-2 and build-3 every unit test passing; build-4 (`771e1ba0`) failed the rt
  scene test on S1 (the final review's H1); build-5, build-6 and build-7 (`c205765b`, the final code) every unit test passing.

## Uncertainty / unresolved questions

- G1's wait assumes the retire hooks hold no lock the collector needs (they are called from engine code, not under a module
  lock; neither review found one): a hook waits on past 5 s, with a row.
- C1 covers the six observed mutators only. A lookup given up costs its model's rows that sample the binding until a collect
  where no mutator runs during the lookup (a long streaming burst could hold them for several frames: not measured). That last
  answer (`c205765b`, four lines) was checked by the pilot, not by a separate reviewer.
- None of the fixes was seen working in a run (D-0227: no live run before the merge); the start check after it is the only run.

## Artifacts produced or modified

- `investigations/I-0121-review-fixes/` (CONTRACT, RESULT, status.json); builds under `evidence/I-0121/` and one under
  `evidence/I-0120/build-1` (disposable).
