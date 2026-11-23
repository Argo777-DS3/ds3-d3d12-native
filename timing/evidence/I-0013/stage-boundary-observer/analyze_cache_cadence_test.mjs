import assert from 'node:assert/strict';
import { parseCsv } from './analyze_stage_boundary.mjs';
import { analyzeCacheCadence } from './analyze_cache_cadence.mjs';

function floatBits(value) {
  const bytes = Buffer.allocUnsafe(4);
  bytes.writeFloatLE(value);
  return bytes.readUInt32LE().toString(16).padStart(8, '0').toUpperCase();
}

function matrixBits(x, z, y) {
  return [1, 0, 0, x, 0, 1, 0, z, 0, 0, 1, y].map(floatBits).join('');
}

const csv = `update_index,source_x,source_z,source_y,cache48_bits,valid
1,0,0,0,${matrixBits(0, 0, 0)},1
2,1,1,0,${matrixBits(0, 0, 0)},1
3,2,2,0,${matrixBits(2, 2, 0)},1
4,3,1.5,0,${matrixBits(3, 1, 0)},1
5,4,3,0,${matrixBits(4, 3, 0)},1
`;
const result = analyzeCacheCadence(parseCsv(csv));
assert.equal(result.structural_pass, true);
assert.equal(result.cache_change_rows, 3);
assert.equal(result.cache_change_fraction, 0.75);
assert.equal(result.source_opposing_vertical_steps.count, 1);
assert.equal(result.source_opposing_vertical_steps.total_distance, 0.5);
assert.equal(result.cache_opposing_vertical_steps.count, 1);
assert.equal(result.cache_opposing_vertical_steps.total_distance, 1);

process.stdout.write(`${JSON.stringify({
  schema_version: 1,
  status: 'pass',
  cases: ['cache_change_cadence', 'unchanged_runs', 'lag_scan',
    'source_and_cache_opposing_vertical_steps'],
  limit: 'Synthetic analyzer test only; no live-game behavior is established.'
}, null, 2)}\n`);
