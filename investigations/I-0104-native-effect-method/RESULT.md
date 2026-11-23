# I-0104 result: the native effect method

FINISH-LINE line 4 (D-0190), opened and closed 2026-10-28, in tranches merged one by one (D-0227). Plan and order:
[DESIGN](DESIGN.md).

## Conclusion

- Elden Ring takes its effects to D3D12 by keeping DS3's effect system whole and putting a D3D12 GX device context under it
  (STATIC, [ER-EFFECTS-STUDY](ER-EFFECTS-STUDY.md)); it has no per-family effect contract. The native method follows it: the
  engine keeps producing its effects (the update, the sorted recording, the renderers, slot 27), and the native implements the
  context they draw through (D-0190's « replay » removed; Thomas told 2026-10-28 14:30, no objection).
- Every family the native drew before (E01 603, 604, 600; 602, 607, 608, 601; the GPU particles 10000) now draws through that
  context (tranche 2, merged `51f35cce`); the effect producers' capture layer is removed (tranche 4, merged `6e74ab02`). Both
  start checks passed with the Firelink effects drawn as before (19 pipelines, no skipped draw, no missing texture).
- Not done, moved out of this line (proposed to Thomas 2026-10-28 19:12; no answer at the closing): tranche 3, the key order of
  the forward rows with the effects (DS3's key read, DESIGN §2), the depth-writing effect draws, the copy before each sampling
  pass, the bindings resolved once at the bind, ER's GPU particle path read. FINISH-LINE line 7 carries it.
- The equivalence of the draws before and after rests on the reviews' reading and the start checks' counts, not on a recorded
  frame compared draw by draw.

## Tranche 1: Elden Ring read (static, 2026-10-28)

[ER-EFFECTS-STUDY](ER-EFFECTS-STUDY.md): Elden Ring keeps DS3's effect system (the sorted command stream, the renderers that fill
a ring, the draw context's constant images) and puts a D3D12 GX device context under it; its effects are drawn at their key among
the pass's rows; it has no per-family effect contract. CONTRACT H1 confirmed for the classic billboards, H2 refuted. The design
that follows ([DESIGN](DESIGN.md) §1): the engine keeps producing its effects, the native implements the context they draw
through, and the GX/D3D11 statement replay goes (D-0190). Thomas told in the thread (2026-10-28 14:30), no objection so far.

## Tranche 2: the native effect context (code, 2026-10-28)

- `frame/effect_draw`: each draw, dispatch, copy or clear the effect renderers issue is handed to the caller at once (no captured
  record list); constant blocks are placed once per update into one per-frame arena with the ring's vertices (256-aligned, a
  4 KiB zero block first; the arena doubles after a frame that ran out of it, `native_ffx_arena_grown`); a throw from the emit is
  kept and rethrown after `Execute`, outside the engine's frames; the string-keyed witnesses only with `ffx-record.flag`.
- `21_effects.inc`: `EffectFrameBuilder` makes each statement the renderer's draw at once (the batch pass over captured records,
  `BuildNativeEffectFrame`, removed). Left (final review S3): each binding is still resolved per draw (programme, layout, bank
  state, texture, about a dozen lock acquisitions a classic draw), not once at the bind as ER's device context does; for a later
  tranche.
- `renderer/effect_stage`: constant blocks bound as root views into the arena uploaded once a frame (no upload per block per
  draw, the arena's upload not zero-filled), pipeline keys a hashed struct (the particle key now also holds the target formats).
- `renderer/scene_pass*`: the dead stage observer removed (CENSUS §8).
- Step review (fresh Opus, high): nothing blocking; its should-fix points applied (the arena's growth and its row, the emit's
  exceptions, the released records' timing, the zero block asserted on both sides, the stale thread-affinity entry); left: the
  frame copies the used arena once (a double-buffered arena would remove it), and the witness rows now print padded block sizes.
- Final review (fresh Opus, xhigh, on `main...HEAD`): nothing blocking in the code; the documentation was stale (STATE's resume
  rows, FINISH-LINE line 4, the CONTRACT's tranches, this proof line: rewritten); applied: the released records taken before
  Execute again (main's order, so a record released during Execute is never described; given back if Execute throws), the whole
  emit guarded inside the noexcept hooks, constant blocks placed only for the slots each statement reads (a deliberate change: a
  copy, a clear or a draw is no longer dropped for a stale invalid block in a slot it never reads, nor for arena room taken by
  such blocks), a row when the arena
  stops growing at its limit, the dead presentation observer parameter removed, the thread-affinity file back to LF; left: the
  pre-existing per-frame string work (the SYSTEX_WHITE lookup per default-texture bind, a failed pipeline's reason copied per draw)
  and a static buffer refused by a full arena counted as `foreign_buffers`.
- Proof: clangd without diagnostics; the MSVC build and all its unit tests pass on the merged tree `a53d736b` (106 PASS rows, no
  failure, `evidence/I-0104/build-3.log`; the earlier trees in `build.log` and `build-2.log`); `Test-Project -MergeGate` passes. No unit test compares the draws before and after: the
  equivalence of what the renderer receives rests on the two reviews' reading, not on a recorded frame (the CONTRACT's expected
  evidence is not met for this tranche). No live run before the merge (D-0227).
- Merged `51f35cce` (2026-10-28 18:57); the launcher's module rebuilt from `main` (106 PASS); the start check `start-51f35cce`
  (19:07-19:08) passed: world drawn (357 frames), closed by its close box, restored, no refusal or failure. Its effect stage row
  against the previous start check's (`start-ce379c84`, the same path): 19 pipelines in both, no skipped draw, no pipeline failure,
  no missing texture, 19,983 effect draws against 19,657 and the GPU-particle statements in the same proportions (a run's own
  variation; not a picture comparison).

## Tranche 4: the GPU particles and the capture layer (code, 2026-10-28)

- The GPU particles (E07, 10000) need no move of their own: since tranche 2 their compute, copy, clear and indirect statements
  are emitted at once through the same native context (their constants in the arena), and the backend owns their buffers, hidden
  counters and states (`EffectStage`); the bonfire embers are drawn natively since D-0186 and the start check of tranche 2 ran
  their statements as before. Not done, moved with tranche 3 (DESIGN §2): the bindings resolved once at the bind (classic and
  E07, tranche 2's « left »), and ER's GPU particle path read (ER-EFFECTS-STUDY §4).
- The capture layer (CENSUS §8): a third option, neither of the two CENSUS offered. `world/effect_producers` removed, with the sun,
  fog and bank helpers only it used: `ds3fx::Produce` recomputed from captures the words the engine's slot 27 (`264230`) writes,
  and since tranche 2 the engine's own slot 27 runs against the native context, so nothing needs it (its reading stays in
  `evidence/I-0064/EFFECTS.md`, the code in commit `3855d0de`). `world/light_producers` kept as it is: `ao_constants_test` uses
  three of its families (cbSSAOParam2, cbSSAOParam, cbSampleOffset) on bare snapshots as the AO cores' oracle; the rest of its
  capture layer (`Snapshots` and its loaders, `WriterRva`, `UseBanks`, `UseCamera`, `UseShadowSetup`, `BankCheck`,
  `ProduceKeyed`, `ProduceSized`, `ProduceBuffer`, the other families, among them the E07 capture producers of 2A7E70, 25E720 and
  25D700) has no caller in the repository: deliberately left as it is in this line (lighting, not effects), for a later code
  cleanup with a fixed AO oracle. The writer enumerators `W264230`, `W01DB60` and `W1495F0` now name unused positions of the
  `WriterRva` table.
- Proof: clangd without diagnostics on `light_producers.cpp`; the MSVC build and all its unit tests pass (106 PASS rows, no
  failure, `evidence/I-0104/build-4.log`, after the commits); `Test-Project -MergeGate` passes. A step review (fresh Opus, high)
  and a final review (fresh Opus, xhigh, `main...HEAD`): nothing blocking in the code; the documentation was stale (STATE,
  FINISH-LINE, the investigations index, this entry, DESIGN, the ER study: rewritten), a comment and `native/README.md` updated.
- Merged `6e74ab02` (2026-10-28 19:33); the launcher's module rebuilt from `main` (106 PASS); the start check `start-6e74ab02`
  (19:40-19:42) passed: world drawn (376 frames), closed, restored, no refusal or failure; its effect stage row: 19 pipelines, no
  skipped draw, no pipeline failure, no missing texture (20,403 draws).

## Next

FINISH-LINE line 5, the missing effects, written in the base (DESIGN §3), in the census's order: E03 605 first. The spells,
infusions and status effects are not counted finished (Thomas, 2026-10-28 18:15-18:22: the miracles' effects incomplete; the
sorceries', pyromancies' and Dark spells' (Black Flame included), every weapon and shield infusion's (the Dark infusion
included) and the status effects on the player and NPCs (poisoned, toxic, bleeding, frostbite) uncertain).
