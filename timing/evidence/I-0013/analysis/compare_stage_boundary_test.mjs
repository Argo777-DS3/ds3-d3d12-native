import assert from 'node:assert/strict';
import { compare } from './compare_stage_boundary.mjs';

function bits(value) {
  const bytes = Buffer.allocUnsafe(4);
  bytes.writeFloatLE(value);
  return bytes.readUInt32LE().toString(16).padStart(8, '0').toUpperCase();
}

function matrix(x, z, y) {
  return [1, 0, 0, x, 0, 1, 0, z, 0, 0, 1, y].map(bits).join('');
}

function rows(count, rate) {
  return Array.from({ length: count }, (_, index) => {
    const progress = index / (count - 1);
    const x = progress * 10;
    const y = progress * 5;
    const z = progress * 2 + Math.sin(progress * Math.PI * 4) * 0.1;
    const gap = Math.sin(progress * Math.PI * 2) * 0.05;
    return {
      qpc: String(1_000_000 + Math.round(index * 10_000_000 / rate)),
      update_index: String(100 + index),
      delta: String(1 / rate),
      movement_state: '0x1234',
      root_transform: '0x1234',
      source_x: String(x), source_z: String(z), source_y: String(y),
      root_x: String(x), root_z: String(z), root_y: String(y),
      cache48_bits: matrix(x, z + gap, y),
      valid: '1',
    };
  });
}

const result = compare(rows(181, 60), rows(721, 240));
assert.equal(result.captures.target60.source_root_exact_rows, 181);
assert.equal(result.captures.target240.source_root_same_pointer_rows, 721);
assert.ok(result.normalized_route_comparison.source_vertical_profile_correlation > 0.999999);
assert.ok(result.normalized_route_comparison.cache_vertical_profile_correlation > 0.999999);
assert.ok(Math.abs(result.normalized_route_comparison.target240_to_target60_raw_gap_rms_ratio - 1) < 0.01);

process.stdout.write(`${JSON.stringify({
  schema_version: 1,
  status: 'pass',
  cases: ['different_density_same_route', 'matrix_translation_3_7_11',
    'source_root_identity', 'cache_gap_invariance'],
  limit: 'Synthetic comparison only; no live-game behavior is established.'
}, null, 2)}\n`);
