import assert from 'node:assert/strict';
import { parseCsv } from '../stage-boundary-observer/analyze_stage_boundary.mjs';
import { compareProducerState } from './compare_producer_state.mjs';

const header = 'qpc_entry,source_before_x,source_before_y,producer_state,observed_delta,valid';
function trace(hz, count) {
  let state = Math.fround(0);
  const lines = [];
  for (let i = 0; i < count; i++) {
    const x = i / hz;
    const target = Math.fround(Math.sin(x * 3));
    if (i) state = Math.fround(state + Math.fround(Math.fround(0.2) * Math.fround(target - state)));
    lines.push(`${Math.round(i * 1e7 / hz)},${x},0,${state},${Math.fround(1 / hz)},1`);
  }
  return parseCsv(`${header}\n${lines.join('\n')}\n`);
}
const result = compareProducerState(trace(60, 181), trace(240, 721));
assert.equal(result.structural_pass, true);
assert.equal(result.target60.exact_native_additive_replay_rows, 180);
assert.equal(result.target240.exact_native_additive_replay_rows, 720);
assert.match(result.laws.affine_gain, /\^/);
process.stdout.write(`${JSON.stringify({schema_version: 1, status: 'pass', cases: [
  'exact_float32_native_step_recovery', 'exponential_affine_law',
  'linear_independent_increment_law', 'decay_law'], limit: 'Synthetic model test only.'}, null, 2)}\n`);
