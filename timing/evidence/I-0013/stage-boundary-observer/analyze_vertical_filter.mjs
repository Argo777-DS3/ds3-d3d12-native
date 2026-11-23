import fs from 'node:fs';
import { parseCsv } from './analyze_stage_boundary.mjs';
import { nativeAlpha, nativeDecay } from './vertical_filter_model.mjs';

function decodeFloatBits(hex) {
  const bytes = Buffer.allocUnsafe(4);
  bytes.writeUInt32LE(Number.parseInt(hex, 16));
  return bytes.readFloatLE(0);
}

function floatBits(value) {
  const bytes = Buffer.allocUnsafe(4);
  bytes.writeFloatLE(Math.fround(value));
  return bytes.readUInt32LE(0);
}

function cacheVertical(row) {
  const raw = row.cache48_bits ?? '';
  return /^[0-9A-Fa-f]{96}$/.test(raw)
    ? decodeFloatBits(raw.slice(7 * 8, 8 * 8))
    : Number.NaN;
}

function rms(values) {
  return values.length
    ? Math.sqrt(values.reduce((sum, value) => sum + value * value, 0) /
        values.length)
    : null;
}

export function analyzeVerticalFilter(rows) {
  const required = [
    'update_index', 'delta', 'source_z', 'filter_2e0', 'filter_2e4',
    'filter_2e8', 'filter_2ec', 'filter_2f0', 'cache48_bits', 'valid'
  ];
  const missing = required.filter((name) => rows.length && !(name in rows[0]));
  const validRows = rows.filter((row) => row.valid === '1');
  const finiteRows = validRows.filter((row) =>
    ['delta', 'source_z', 'filter_2e0', 'filter_2e4', 'filter_2e8',
      'filter_2ec', 'filter_2f0'].every((key) => Number.isFinite(Number(row[key]))) &&
    Number.isFinite(cacheVertical(row)));

  let gateActive = 0;
  let gateCacheMatches = 0;
  let flagActive = 0;
  const outputResiduals = [];
  const decayResiduals = [];
  let decayQualified = 0;

  for (let index = 0; index < finiteRows.length; ++index) {
    const row = finiteRows[index];
    const gate = Number(row.filter_2ec);
    const filtered = Number(row.filter_2e4);
    if ((floatBits(gate) & 0x7fffffff) !== 0) {
      gateActive += 1;
      if (floatBits(filtered) === floatBits(cacheVertical(row))) {
        gateCacheMatches += 1;
      }
    }
    if (Number(row.filter_2f0) !== 0) flagActive += 1;
    if (index === 0 ||
        Number(row.update_index) !== Number(finiteRows[index - 1].update_index) + 1) {
      continue;
    }
    const previous = finiteRows[index - 1];
    const dt = Number(row.delta);
    const flag = Number(row.filter_2f0) !== 0;
    const threshold = Number(row.filter_2e0);
    const source = Number(row.source_z);
    const previousOutput = Number(previous.filter_2e4);
    const target = flag ? threshold : source;
    if (threshold > source && !(flag && Math.abs(threshold - previousOutput) < 0.05)) {
      const predicted = previousOutput +
        (target - previousOutput) * nativeAlpha(dt);
      outputResiduals.push(filtered - predicted);
    }
    const previousGate = Number(previous.filter_2ec);
    if (!flag && threshold > source &&
        (floatBits(previousGate) & 0x7fffffff) !== 0) {
      const predictedGate = previousGate * nativeDecay(dt);
      decayResiduals.push(gate - predictedGate);
      decayQualified += 1;
    }
  }

  return {
    structural_pass: rows.length >= 2 && missing.length === 0 &&
      validRows.length === rows.length && finiteRows.length === rows.length,
    row_count: rows.length,
    valid_row_count: validRows.length,
    missing_columns: missing,
    gate_nonzero_rows: gateActive,
    gate_cache_bit_exact_rows: gateCacheMatches,
    flag_nonzero_rows: flagActive,
    output_recurrence_pairs: outputResiduals.length,
    output_recurrence_residual_rms: rms(outputResiduals),
    decay_recurrence_pairs: decayQualified,
    decay_recurrence_residual_rms: rms(decayResiduals),
    limit: 'Broad-update samples can test activation and recurrence consistency; they do not by themselves prove visible-terrain causality.'
  };
}

if (process.argv[1]?.endsWith('analyze_vertical_filter.mjs')) {
  if (process.argv.length !== 4) {
    throw new Error('usage: node analyze_vertical_filter.mjs INPUT.csv OUTPUT.json');
  }
  const result = analyzeVerticalFilter(parseCsv(fs.readFileSync(process.argv[2], 'utf8')));
  fs.writeFileSync(process.argv[3], `${JSON.stringify(result, null, 2)}\n`);
  if (!result.structural_pass) process.exitCode = 2;
}
