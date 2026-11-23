import fs from 'node:fs';
import { parseCsv } from '../stage-boundary-observer/analyze_stage_boundary.mjs';

function decodeMatrix(hex) {
  if (!/^[0-9A-Fa-f]{96}$/.test(hex ?? '')) return null;
  return Array.from({ length: 12 }, (_, index) => {
    const bytes = Buffer.allocUnsafe(4);
    bytes.writeUInt32LE(Number.parseInt(
      hex.slice(index * 8, index * 8 + 8), 16));
    return bytes.readFloatLE(0);
  });
}

function rms(values) {
  return values.length
    ? Math.sqrt(values.reduce((sum, value) => sum + value * value, 0) /
        values.length)
    : null;
}

function activeRange(rows, threshold = 0.002) {
  const moving = [];
  for (let index = 1; index < rows.length; ++index) {
    const planar = Math.hypot(
      Number(rows[index].source_before_x) -
        Number(rows[index - 1].source_before_x),
      Number(rows[index].source_before_y) -
        Number(rows[index - 1].source_before_y));
    if (planar > threshold) moving.push(index);
  }
  return moving.length
    ? rows.slice(moving[0] - 1, moving.at(-1) + 1)
    : rows;
}

function opposing(values, direction, planarDistance) {
  const steps = values.slice(1).map((value, index) => value - values[index]);
  const selected = steps.filter(step => direction && step * direction < -1e-6);
  return {
    count: selected.length,
    total_distance: selected.reduce((sum, value) => sum + Math.abs(value), 0),
    peak: selected.length ? Math.max(...selected.map(Math.abs)) : 0,
    count_per_planar_unit: planarDistance
      ? selected.length / planarDistance
      : null,
    distance_per_planar_unit: planarDistance
      ? selected.reduce((sum, value) => sum + Math.abs(value), 0) /
          planarDistance
      : null,
  };
}

export function analyzeBaseTransform(rows, qpcFrequency = 10_000_000) {
  const required = ['qpc_entry', 'qpc_exit', 'sequence', 'caller_return',
    'movement', 'output', 'source_before_x', 'source_before_z',
    'source_before_y', 'source_after_x', 'source_after_z',
    'source_after_y', 'source_extra', 'selector_a', 'selector_b',
    'cache_before48_bits', 'base_output48_bits', 'valid'];
  const missing = required.filter(name => rows.length && !(name in rows[0]));
  const validRows = rows.filter(row => row.valid === '1');
  const sequences = validRows.map(row => Number(row.sequence));
  const consecutive = sequences.every((value, index) =>
    index === 0 || value === sequences[index - 1] + 1);
  const callerExact = validRows.every(row =>
    Number.parseInt(row.caller_return, 16) === 0x005C812C);
  const pointersValid = validRows.every(row =>
    Number.parseInt(row.movement, 16) !== 0 &&
    Number.parseInt(row.output, 16) !== 0);
  const matrices = validRows.map(row => ({
    base: decodeMatrix(row.base_output48_bits),
    cache: decodeMatrix(row.cache_before48_bits),
  }));
  const finite = validRows.every((row, index) =>
    ['qpc_entry', 'qpc_exit', 'source_before_x', 'source_before_z',
      'source_before_y', 'source_after_x', 'source_after_z',
      'source_after_y', 'source_extra'].every(key =>
      Number.isFinite(Number(row[key]))) &&
    matrices[index].base?.every(Number.isFinite) &&
    matrices[index].cache?.every(Number.isFinite));
  const qpcMonotone = validRows.every((row, index) =>
    Number(row.qpc_exit) >= Number(row.qpc_entry) &&
    (index === 0 || Number(row.qpc_entry) >
      Number(validRows[index - 1].qpc_entry)));

  const active = activeRange(validRows);
  let planarDistance = 0;
  for (let index = 1; index < active.length; ++index) {
    planarDistance += Math.hypot(
      Number(active[index].source_before_x) -
        Number(active[index - 1].source_before_x),
      Number(active[index].source_before_y) -
        Number(active[index - 1].source_before_y));
  }
  const source = active.map(row => Number(row.source_before_z));
  const base = active.map(row => decodeMatrix(row.base_output48_bits)[7]);
  const cacheBefore = active.map(row =>
    decodeMatrix(row.cache_before48_bits)[7]);
  const sourceDirection = Math.sign(source.at(-1) - source[0]);
  const baseDirection = Math.sign(base.at(-1) - base[0]);
  const cacheDirection = Math.sign(cacheBefore.at(-1) - cacheBefore[0]);
  const sourceStableAcrossCall = validRows.every(row =>
    ['x', 'z', 'y', 'w'].every(axis =>
      Math.fround(Number(row[`source_before_${axis}`])) ===
      Math.fround(Number(row[`source_after_${axis}`]))));
  let exactBaseVerticalRows = 0;
  const baseResiduals = [];
  for (const row of active) {
    const expected = Number(row.selector_a) === Number(row.selector_b)
      ? Math.fround(Math.fround(Number(row.source_before_z)) +
          Math.fround(Number(row.source_extra)))
      : Math.fround(Number(row.source_before_z));
    const actual = decodeMatrix(row.base_output48_bits)[7];
    if (Object.is(expected, actual)) exactBaseVerticalRows += 1;
    baseResiduals.push(actual - expected);
  }
  const duration = validRows.length > 1
    ? (Number(validRows.at(-1).qpc_entry) -
        Number(validRows[0].qpc_entry)) / qpcFrequency
    : 0;

  return {
    structural_pass: rows.length >= 2 && missing.length === 0 &&
      validRows.length === rows.length && consecutive && callerExact &&
      pointersValid && finite && qpcMonotone,
    row_count: rows.length,
    valid_row_count: validRows.length,
    missing_columns: missing,
    consecutive_sequences: consecutive,
    expected_caller_only: callerExact,
    pointer_fields_valid: pointersValid,
    finite_values: finite,
    qpc_monotone: qpcMonotone,
    measured_calls_per_second: duration > 0
      ? (validRows.length - 1) / duration
      : null,
    source_stable_across_call: sourceStableAcrossCall,
    active_rows: active.length,
    active_planar_distance: planarDistance,
    base_vertical_exact_rows: exactBaseVerticalRows,
    base_vertical_residual_rms: rms(baseResiduals),
    source_opposing_vertical_steps:
      opposing(source, sourceDirection, planarDistance),
    base_opposing_vertical_steps:
      opposing(base, baseDirection, planarDistance),
    prior_cache_opposing_vertical_steps:
      opposing(cacheBefore, cacheDirection, planarDistance),
    limit: 'One exact caller and current-player base-transform boundary; it does not identify protected downstream behavior or subsystem ownership.'
  };
}

if (process.argv[1]?.endsWith('analyze_base_transform.mjs')) {
  if (process.argv.length !== 4) {
    throw new Error('usage: node analyze_base_transform.mjs INPUT.csv OUTPUT.json');
  }
  const result = analyzeBaseTransform(
    parseCsv(fs.readFileSync(process.argv[2], 'utf8')));
  fs.writeFileSync(process.argv[3], `${JSON.stringify(result, null, 2)}\n`);
  if (!result.structural_pass) process.exitCode = 2;
}
