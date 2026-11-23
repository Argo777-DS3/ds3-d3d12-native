import fs from 'node:fs';

export function parseCsv(text) {
  const lines = text.trim().split(/\r?\n/);
  const header = lines.shift().split(',');
  return lines.filter(Boolean).map((line) => {
    const fields = line.split(',');
    return Object.fromEntries(header.map((name, index) => [name, fields[index]]));
  });
}

function correlation(left, right) {
  if (left.length !== right.length || left.length < 2) return null;
  const lm = left.reduce((a, b) => a + b, 0) / left.length;
  const rm = right.reduce((a, b) => a + b, 0) / right.length;
  let numerator = 0;
  let ld = 0;
  let rd = 0;
  for (let i = 0; i < left.length; ++i) {
    const l = left[i] - lm;
    const r = right[i] - rm;
    numerator += l * r;
    ld += l * l;
    rd += r * r;
  }
  return ld && rd ? numerator / Math.sqrt(ld * rd) : null;
}

function deltas(values) {
  return values.slice(1).map((value, index) => value - values[index]);
}

function decodeFloatBits(hex) {
  const bytes = Buffer.allocUnsafe(4);
  bytes.writeUInt32LE(Number.parseInt(hex, 16));
  return bytes.readFloatLE(0);
}

function cacheTranslation(row) {
  if (!/^[0-9A-Fa-f]{96}$/.test(row.cache48_bits ?? '')) {
    return [Number.NaN, Number.NaN, Number.NaN];
  }
  const lanes = Array.from({ length: 12 }, (_, index) =>
    decodeFloatBits(row.cache48_bits.slice(index * 8, index * 8 + 8)));
  return [lanes[3], lanes[7], lanes[11]];
}

export function analyze(rows, qpcFrequency = 10_000_000) {
  const required = ['qpc', 'update_index', 'delta', 'source_z',
    'cache48_bits', 'root_z', 'valid'];
  const missing = required.filter((name) => rows.length && !(name in rows[0]));
  const validRows = rows.filter((row) => row.valid === '1');
  const indices = validRows.map((row) => Number(row.update_index));
  const consecutive = indices.every((value, index) =>
    index === 0 || value === indices[index - 1] + 1);
  const finite = validRows.every((row) =>
    ['delta', 'source_z', 'root_z'].every((key) =>
      Number.isFinite(Number(row[key]))) &&
      cacheTranslation(row).every(Number.isFinite));
  const source = validRows.map((row) => Number(row.source_z));
  const cache = validRows.map((row) => cacheTranslation(row)[1]);
  const root = validRows.map((row) => Number(row.root_z));
  const duration = validRows.length > 1
    ? (Number(validRows.at(-1).qpc) - Number(validRows[0].qpc)) / qpcFrequency
    : 0;
  return {
    structural_pass: rows.length >= 2 && missing.length === 0 &&
      validRows.length === rows.length && consecutive && finite,
    row_count: rows.length,
    valid_row_count: validRows.length,
    missing_columns: missing,
    consecutive_updates: consecutive,
    finite_values: finite,
    cache_translation_lanes: [3, 7, 11],
    measured_rows_per_second: duration > 0 ? (validRows.length - 1) / duration : null,
    delta_correlations: {
      source_cache: correlation(deltas(source), deltas(cache)),
      source_root: correlation(deltas(source), deltas(root)),
      cache_root: correlation(deltas(cache), deltas(root)),
    },
    limit: 'Stage correlations localize divergence candidates; they do not assign collision or physics ownership.'
  };
}

if (process.argv[1] && process.argv[1].endsWith('analyze_stage_boundary.mjs')) {
  if (process.argv.length !== 4) {
    throw new Error('usage: node analyze_stage_boundary.mjs INPUT.csv OUTPUT.json');
  }
  const result = analyze(parseCsv(fs.readFileSync(process.argv[2], 'utf8')));
  fs.writeFileSync(process.argv[3], `${JSON.stringify(result, null, 2)}\n`);
  if (!result.structural_pass) process.exitCode = 2;
}
