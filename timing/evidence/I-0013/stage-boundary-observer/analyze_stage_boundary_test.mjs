import assert from 'node:assert/strict';
import { analyze, parseCsv } from './analyze_stage_boundary.mjs';

function floatBits(value) {
  const bytes = Buffer.allocUnsafe(4);
  bytes.writeFloatLE(value);
  return bytes.readUInt32LE().toString(16).padStart(8, '0').toUpperCase();
}

function matrixBits(x, z, y) {
  return [1, 0, 0, x, 0, 1, 0, z, 0, 0, 1, y].map(floatBits).join('');
}

const csv = `qpc,update_index,delta,source_z,cache48_bits,root_z,valid
1000,7,0.0166666,1,${matrixBits(10, 1.1, 20)},0.9,1
167666,8,0.0166666,2,${matrixBits(11, 2.1, 21)},1.9,1
334332,9,0.0166666,4,${matrixBits(13, 4.1, 23)},3.9,1
500998,10,0.0166666,7,${matrixBits(16, 7.1, 26)},6.9,1
`;
const pass = analyze(parseCsv(csv));
assert.equal(pass.structural_pass, true);
assert.equal(pass.row_count, 4);
assert.deepEqual(pass.cache_translation_lanes, [3, 7, 11]);
assert.ok(Math.abs(pass.delta_correlations.source_cache - 1) < 1e-12);
assert.ok(Math.abs(pass.delta_correlations.source_root - 1) < 1e-12);

const invalid = parseCsv(csv.replace(',1\n334332', ',0\n334332'));
assert.equal(analyze(invalid).structural_pass, false);
const gap = parseCsv(csv.replace('334332,9', '334332,11'));
assert.equal(analyze(gap).structural_pass, false);

process.stdout.write(`${JSON.stringify({
  schema_version: 1,
  status: 'pass',
  cases: ['valid_consecutive_trace', 'invalid_row_rejection', 'update_gap_rejection'],
  limit: 'Synthetic analyzer test only; no live-game behavior is established.'
}, null, 2)}\n`);
