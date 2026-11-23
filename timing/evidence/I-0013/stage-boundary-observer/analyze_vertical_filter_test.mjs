import assert from 'node:assert/strict';
import { parseCsv } from './analyze_stage_boundary.mjs';
import { analyzeVerticalFilter } from './analyze_vertical_filter.mjs';
import { nativeAlpha, nativeDecay } from './vertical_filter_model.mjs';

function floatBits(value) {
  const bytes = Buffer.allocUnsafe(4);
  bytes.writeFloatLE(Math.fround(value));
  return bytes.readUInt32LE().toString(16).padStart(8, '0').toUpperCase();
}

function matrixBits(z) {
  return [1, 0, 0, 10, 0, 1, 0, z, 0, 0, 1, 20]
    .map(floatBits).join('');
}

const dt = 1 / 60;
const output0 = -9;
const source1 = -10;
const output1 = output0 + (source1 - output0) * nativeAlpha(dt);
const gate0 = 0.3;
const gate1 = gate0 * nativeDecay(dt);
const csv = `update_index,delta,source_z,filter_2e0,filter_2e4,filter_2e8,filter_2ec,filter_2f0,cache48_bits,valid
7,${dt},-9,-8,${output0},0,${gate0},0,${matrixBits(output0)},1
8,${dt},${source1},-8,${output1},0,${gate1},0,${matrixBits(output1)},1
`;
const result = analyzeVerticalFilter(parseCsv(csv));
assert.equal(result.structural_pass, true);
assert.equal(result.gate_nonzero_rows, 2);
assert.equal(result.gate_cache_bit_exact_rows, 2);
assert.equal(result.output_recurrence_pairs, 1);
assert.equal(result.decay_recurrence_pairs, 1);
assert.ok(result.output_recurrence_residual_rms < 1e-6);
assert.ok(result.decay_recurrence_residual_rms < 1e-6);

const missing = analyzeVerticalFilter(parseCsv(csv.replace(',filter_2f0', '')));
assert.equal(missing.structural_pass, false);

process.stdout.write(`${JSON.stringify({
  schema_version: 1,
  status: 'pass',
  cases: ['active_gate_cache_binding', 'output_recurrence', 'gate_decay',
    'missing_column_rejection'],
  limit: 'Synthetic analyzer test only; no live-game behavior is established.'
}, null, 2)}\n`);
