import assert from 'node:assert/strict';
import {
  compareRates,
  constants,
  invariantAlpha,
  invariantDecay,
  nativeAlpha,
  nativeDecay,
} from './vertical_filter_model.mjs';

const epsilon = 1e-14;
assert.ok(Math.abs(nativeAlpha(1 / 60) - constants.referenceAlpha) < epsilon);
assert.ok(Math.abs(invariantAlpha(1 / 60) - constants.referenceAlpha) < epsilon);
assert.ok(Math.abs(nativeDecay(1 / 60) - constants.referenceDecay) < epsilon);
assert.ok(Math.abs(invariantDecay(1 / 60) - constants.referenceDecay) < epsilon);

const rates = compareRates([60, 120, 240, 360]);
const outputReference = rates[0].output_retention_after_one_second_invariant;
const gateReference = rates[0].gate_retention_after_one_second_invariant;
for (const row of rates) {
  assert.ok(Math.abs(row.output_retention_after_one_second_invariant - outputReference) < epsilon);
  assert.ok(Math.abs(row.gate_retention_after_one_second_invariant - gateReference) < epsilon);
}
assert.ok(rates[2].output_retention_after_one_second_native >
  rates[0].output_retention_after_one_second_native);
assert.ok(rates[2].gate_retention_after_one_second_native <
  rates[0].gate_retention_after_one_second_native);

process.stdout.write(`${JSON.stringify({
  schema_version: 1,
  status: 'pass',
  reference_alpha: constants.referenceAlpha,
  reference_decay: constants.referenceDecay,
  cases: [
    'native_60_preserved_exactly',
    'invariant_output_has_equal_wall_time_retention',
    'invariant_gate_has_equal_wall_time_retention',
    'native_240_output_is_slower_than_native_60',
    'native_240_gate_decays_faster_than_native_60',
  ],
  limit: 'Synthetic recurrence test only; no live-game behavior is established.'
}, null, 2)}\n`);
