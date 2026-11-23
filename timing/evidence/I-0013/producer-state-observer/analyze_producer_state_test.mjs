import assert from 'node:assert/strict';
import { parseCsv } from '../stage-boundary-observer/analyze_stage_boundary.mjs';
import { analyzeProducerState } from './analyze_producer_state.mjs';

const header = 'owner_e0_target,actor_178_target,producer_state,retention,observed_delta,producer_mode,carried,source_before_x,source_before_y,valid';
const text = `${header}\n` + [
  '0x5E6580,0x8BC9A0,0.4,0.5,0.0166666,0,0.5,0,0,1',
  '0x5E6580,0x8BC9A0,0.2,0.5,0.0166666,0,0.3,1,0,1',
  '0x5E6580,0x8BC9A0,0,0.5,0.0166666,0,0.1,2,0,1',
  '0x5E6580,0x8BC9A0,0.5,0.5,0.0166666,0,0.6,3,0,1',
].join('\n');
const result = analyzeProducerState(parseCsv(`${text}\n`));
assert.equal(result.structural_pass, true);
assert.equal(result.recurrence.retention_exact, 1);
assert.equal(result.recurrence.reset_zero, 1);
assert.equal(result.recurrence.clamp_positive, 1);
assert.equal(result.carried_recomposition.exact_rows, 4);
const changed = parseCsv(`${text}\n`);
changed[2].owner_e0_target = '0x5E6581';
assert.equal(analyzeProducerState(changed).structural_pass, false);
process.stdout.write(`${JSON.stringify({
  schema_version: 1,
  status: 'pass',
  cases: ['stable_targets', 'exact_retention', 'zero_reset', 'positive_clamp', 'float32_recomposition', 'target_change_rejected'],
  limit: 'Synthetic producer-state classification only.',
}, null, 2)}\n`);
