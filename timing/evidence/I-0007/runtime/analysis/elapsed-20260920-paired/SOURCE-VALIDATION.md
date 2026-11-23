# Candidate source-response qualification — cycle38

The existing stopped candidate session2428 is reused; all23 raw hashes verify.
No new acquisition. `runtime/evaluate_candidate_source.mjs` reconstructs source
attenuation from gate snapshots, the existing builder+260 pre-factor norm-rate
diagnostic and the qualified caller delta.

During the two full recovery episodes, all19 nonzero source samples match
`min(entry_scalar,1)` within2e-7:5 at60 and14 at165. The largest residual is
1.4585e-7. Together with the already established recovery times83.3330ms and
84.8484ms, this proves the candidate response reaches the local movement source.
It is more than an observation of a scalar clock with no consumer effect.

Across **all** recorded nonunit entries,327 usable samples pass the same source
check:128 at60 and199 at165. Four remaining entries have zero source and zero
norm-rate, so no ratio is defined; they are explicitly excluded, not failures
or evidence of nonzero attenuation. The JSON records their update indices.

This qualifies the bounded local causal response of the prototype. It does not
establish equal world trajectories, all-action behavior or distribution safety.
The pre-factor vector itself was not captured; builder+260 is a norm diagnostic.
Rates and animation phases differ between trials, so source-norm sums must not
be presented as a cross-framerate distance ratio. The conditional unit-source
area difference+4.6633% remains visible in elapsed-pair-evaluation.json; it is
not an observed DS3 travel error and is not grounds for an unproved gain patch.

Candidate and damping instruments are stopped/config-disabled. K-0041 records
only the proved local response. The separate0.65 accumulator lead remains
unqualified for harmful behavior; a difficult DLC scene is not requested.
