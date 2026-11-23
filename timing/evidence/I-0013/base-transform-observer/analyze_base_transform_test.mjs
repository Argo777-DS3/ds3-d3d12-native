import assert from 'node:assert/strict';
import { parseCsv } from '../stage-boundary-observer/analyze_stage_boundary.mjs';
import { analyzeBaseTransform } from './analyze_base_transform.mjs';

function matrixBits(x, z, y) {
  const lanes = [1, 0, 0, x, 0, 1, 0, z, 0, 0, 1, y];
  return lanes.map(value => {
    const bytes = Buffer.allocUnsafe(4);
    bytes.writeFloatLE(value);
    return bytes.readUInt32LE().toString(16).padStart(8, '0').toUpperCase();
  }).join('');
}

const header = [
  'qpc_entry', 'qpc_exit', 'sequence', 'caller_return', 'movement', 'output',
  'source_before_x', 'source_before_z', 'source_before_y', 'source_before_w',
  'source_after_x', 'source_after_z', 'source_after_y', 'source_after_w',
  'source_extra', 'selector_a', 'selector_b',
  'cache_before_x', 'cache_before_z', 'cache_before_y',
  'base_x', 'base_z', 'base_y', 'cache_before48_bits',
  'base_output48_bits', 'valid'
].join(',');
const zs = [0, 1, 2, 1.5, 3];
const baseZs = [0, 1, 2, 1, 3];
const cacheZs = [0, 0, 2, 1, 3];
const lines = zs.map((z, index) => [
  10_000_000 + index * 166_666, 10_000_010 + index * 166_666,
  index + 1, '0x5C812C', '0x1000', `0x${(0x2000 + index * 0x40).toString(16)}`,
  index, z, 0, 1, index, z, 0, 1, 0, 1, 2,
  index, cacheZs[index], 0, index, baseZs[index], 0,
  matrixBits(index, cacheZs[index], 0),
  matrixBits(index, baseZs[index], 0), 1
].join(','));
const result = analyzeBaseTransform(parseCsv(`${header}\n${lines.join('\n')}\n`));
assert.equal(result.structural_pass, true);
assert.equal(result.source_stable_across_call, true);
assert.equal(result.source_opposing_vertical_steps.count, 1);
assert.equal(result.base_opposing_vertical_steps.count, 1);
assert.equal(result.prior_cache_opposing_vertical_steps.count, 1);
assert.equal(result.base_vertical_exact_rows, 4);

const badCaller = parseCsv(`${header}\n${lines.join('\n')}\n`);
badCaller[1].caller_return = '0xDEADBEEF';
assert.equal(analyzeBaseTransform(badCaller).structural_pass, false);

const badSequence = parseCsv(`${header}\n${lines.join('\n')}\n`);
badSequence[2].sequence = '9';
assert.equal(analyzeBaseTransform(badSequence).structural_pass, false);

const invalidRow = parseCsv(`${header}\n${lines.join('\n')}\n`);
invalidRow[3].valid = '0';
assert.equal(analyzeBaseTransform(invalidRow).structural_pass, false);

process.stdout.write(`${JSON.stringify({
  schema_version: 1,
  status: 'pass',
  cases: ['exact_caller', 'identity_fields', 'qpc_and_sequence',
    'source_base_prior_cache_opposing_steps', 'base_vertical_reconstruction',
    'bad_caller_rejected', 'bad_sequence_rejected', 'invalid_row_rejected'],
  limit: 'Synthetic analyzer test only; no live-game behavior is established.'
}, null, 2)}\n`);
