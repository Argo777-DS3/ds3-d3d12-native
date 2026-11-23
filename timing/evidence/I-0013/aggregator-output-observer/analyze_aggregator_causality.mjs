import fs from 'node:fs';
import { parseCsv } from '../stage-boundary-observer/analyze_stage_boundary.mjs';

function decodeMatrix(hex) {
  if (!/^[0-9A-Fa-f]{96}$/.test(hex ?? '')) return null;
  return Array.from({ length: 12 }, (_, index) => {
    const bytes = Buffer.allocUnsafe(4);
    bytes.writeUInt32LE(Number.parseInt(hex.slice(index * 8, index * 8 + 8), 16));
    return bytes.readFloatLE(0);
  });
}

function rms(values) {
  return values.length
    ? Math.sqrt(values.reduce((sum, value) => sum + value * value, 0) / values.length)
    : null;
}

function activeRange(rows) {
  const moving = [];
  for (let index = 1; index < rows.length; ++index) {
    const distance = Math.hypot(
      Number(rows[index].source_before_x) - Number(rows[index - 1].source_before_x),
      Number(rows[index].source_before_y) - Number(rows[index - 1].source_before_y));
    if (distance > 0.002) moving.push(index);
  }
  return moving.length ? rows.slice(moving[0] - 1, moving.at(-1) + 1) : rows;
}

function opposing(values, direction, planarDistance) {
  const selected = values.slice(1)
    .map((value, index) => value - values[index])
    .filter(step => direction && step * direction < -1e-6);
  const total = selected.reduce((sum, step) => sum + Math.abs(step), 0);
  return {
    count: selected.length,
    total_distance: total,
    count_per_planar_unit: planarDistance ? selected.length / planarDistance : null,
    distance_per_planar_unit: planarDistance ? total / planarDistance : null,
  };
}

export function analyzeCausality(rows) {
  const valid = rows.filter(row => row.valid === '1');
  const decoded = valid.map(row => ({
    cache: decodeMatrix(row.cache_before48_bits),
    final: decodeMatrix(row.final_output48_bits),
  }));
  let exactVertical = 0;
  let exactX = 0;
  let exactY = 0;
  const verticalResiduals = [];
  for (let index = 0; index < valid.length; ++index) {
    const row = valid[index];
    const baseZ = Number(row.selector_a) === Number(row.selector_b)
      ? Math.fround(Math.fround(Number(row.source_before_z)) + Math.fround(Number(row.source_extra)))
      : Math.fround(Number(row.source_before_z));
    const expectedZ = Math.fround(baseZ + Math.fround(Number(row.carried)));
    const final = decoded[index].final;
    if (Object.is(expectedZ, final[7])) exactVertical += 1;
    if (Object.is(Math.fround(Number(row.source_before_x)), final[3])) exactX += 1;
    if (Object.is(Math.fround(Number(row.source_before_y)), final[11])) exactY += 1;
    verticalResiduals.push(final[7] - expectedZ);
  }
  let cacheEqualsPreviousFinal = 0;
  for (let index = 1; index < valid.length; ++index) {
    if (valid[index].cache_before48_bits === valid[index - 1].final_output48_bits) {
      cacheEqualsPreviousFinal += 1;
    }
  }
  const active = activeRange(valid);
  let planarDistance = 0;
  for (let index = 1; index < active.length; ++index) {
    planarDistance += Math.hypot(
      Number(active[index].source_before_x) - Number(active[index - 1].source_before_x),
      Number(active[index].source_before_y) - Number(active[index - 1].source_before_y));
  }
  const sourceZ = active.map(row => Number(row.source_before_z));
  const direction = Math.sign(sourceZ.at(-1) - sourceZ[0]);
  const carried = active.map(row => Number(row.carried));
  const finalZ = active.map(row => decodeMatrix(row.final_output48_bits)[7]);
  const allPredicateActive = valid.every(row => row.local_predicate === '1');
  const allModeZero = valid.every(row => row.mode_a === '0' && row.mode_b === '0');
  const allOverrideInactive = valid.every(row => Number(row.override_gate) === 0);
  return {
    structural_pass: rows.length >= 2 && valid.length === rows.length &&
      decoded.every(value => value.cache && value.final),
    row_count: rows.length,
    exact_vertical_source_plus_carried_rows: exactVertical,
    exact_horizontal_source_rows: { x: exactX, y: exactY },
    vertical_equation_residual_rms: rms(verticalResiduals),
    vertical_equation_residual_max_abs: Math.max(...verticalResiduals.map(Math.abs)),
    cache_equals_previous_final_rows: cacheEqualsPreviousFinal,
    cache_previous_final_comparisons: Math.max(0, valid.length - 1),
    all_local_predicate_active: allPredicateActive,
    all_modes_zero: allModeZero,
    all_override_inactive: allOverrideInactive,
    active_rows: active.length,
    active_planar_distance: planarDistance,
    carried_opposing_vertical_steps: opposing(carried, direction, planarDistance),
    final_opposing_vertical_steps: opposing(finalZ, direction, planarDistance),
    boundary_pass: exactVertical === valid.length && allPredicateActive &&
      allModeZero && allOverrideInactive,
    limit: 'Proves the sampled final vertical equation and cache transport. It does not identify either dynamic contributor that produces carried +0x250 or establish subsystem semantics.',
  };
}

if (process.argv[1]?.endsWith('analyze_aggregator_causality.mjs')) {
  if (process.argv.length !== 4) {
    throw new Error('usage: node analyze_aggregator_causality.mjs INPUT.csv OUTPUT.json');
  }
  const result = analyzeCausality(parseCsv(fs.readFileSync(process.argv[2], 'utf8')));
  fs.writeFileSync(process.argv[3], `${JSON.stringify(result, null, 2)}\n`);
  if (!result.structural_pass || !result.boundary_pass) process.exitCode = 2;
}
