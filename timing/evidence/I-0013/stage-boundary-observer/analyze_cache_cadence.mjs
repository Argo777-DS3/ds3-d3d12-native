import fs from 'node:fs';
import { parseCsv } from './analyze_stage_boundary.mjs';

function decodeFloatBits(hex) {
  const bytes = Buffer.allocUnsafe(4);
  bytes.writeUInt32LE(Number.parseInt(hex, 16));
  return bytes.readFloatLE(0);
}

function matrix(row) {
  const raw = row.cache48_bits ?? '';
  if (!/^[0-9A-Fa-f]{96}$/.test(raw)) return null;
  return Array.from({ length: 12 }, (_, index) =>
    decodeFloatBits(raw.slice(index * 8, index * 8 + 8)));
}

function distribution(values) {
  const sorted = values.filter(Number.isFinite).sort((a, b) => a - b);
  if (!sorted.length) return { count: 0, minimum: null, median: null, maximum: null };
  const at = fraction => sorted[Math.floor((sorted.length - 1) * fraction)];
  return {
    count: sorted.length,
    minimum: sorted[0],
    p05: at(0.05),
    median: at(0.5),
    p95: at(0.95),
    maximum: sorted.at(-1),
  };
}

function rms(values) {
  return values.length
    ? Math.sqrt(values.reduce((sum, value) => sum + value * value, 0) /
        values.length)
    : null;
}

function opposingSummary(steps, direction, planarDistance) {
  const opposing = steps.filter(step => direction && step * direction < -1e-6);
  return {
    count: opposing.length,
    total_distance: opposing.reduce((sum, value) => sum + Math.abs(value), 0),
    peak: opposing.length ? Math.max(...opposing.map(Math.abs)) : 0,
    count_per_planar_unit: planarDistance ? opposing.length / planarDistance : null,
  };
}

function activeRange(rows, threshold = 0.002) {
  const moving = [];
  for (let index = 1; index < rows.length; ++index) {
    const planar = Math.hypot(
      Number(rows[index].source_x) - Number(rows[index - 1].source_x),
      Number(rows[index].source_y) - Number(rows[index - 1].source_y));
    if (planar > threshold) moving.push(index);
  }
  if (!moving.length) return rows;
  return rows.slice(moving[0] - 1, moving.at(-1) + 1);
}

export function analyzeCacheCadence(rows) {
  const valid = rows.filter(row => row.valid === '1' && matrix(row));
  const active = activeRange(valid);
  const gaps = active.map(row => matrix(row)[7] - Number(row.source_z));
  const cacheChanges = [];
  const changeIntervals = [];
  const unchangedRuns = [];
  let run = 0;
  let lastChangeIndex = null;
  for (let index = 1; index < valid.length; ++index) {
    if (valid[index].cache48_bits === valid[index - 1].cache48_bits) {
      run += 1;
      continue;
    }
    cacheChanges.push(index);
    unchangedRuns.push(run);
    run = 0;
    if (lastChangeIndex !== null) changeIntervals.push(index - lastChangeIndex);
    lastChangeIndex = index;
  }
  unchangedRuns.push(run);

  const lagRms = {};
  for (let lag = 0; lag <= 8; ++lag) {
    const residuals = [];
    for (const index of cacheChanges) {
      if (index < lag) continue;
      residuals.push(matrix(valid[index])[7] - Number(valid[index - lag].source_z));
    }
    lagRms[lag] = rms(residuals);
  }

  let planarDistance = 0;
  const verticalSteps = [];
  const cacheVerticalSteps = [];
  const gapSteps = [];
  for (let index = 1; index < active.length; ++index) {
    planarDistance += Math.hypot(
      Number(active[index].source_x) - Number(active[index - 1].source_x),
      Number(active[index].source_y) - Number(active[index - 1].source_y));
    verticalSteps.push(Number(active[index].source_z) -
      Number(active[index - 1].source_z));
    cacheVerticalSteps.push(matrix(active[index])[7] -
      matrix(active[index - 1])[7]);
    gapSteps.push(gaps[index] - gaps[index - 1]);
  }
  const netVertical = Number(active.at(-1)?.source_z ?? 0) -
    Number(active[0]?.source_z ?? 0);
  const direction = Math.sign(netVertical);
  const cacheDirection = Math.sign(matrix(active.at(-1))[7] - matrix(active[0])[7]);

  return {
    structural_pass: valid.length === rows.length && rows.length >= 2,
    rows: rows.length,
    active_rows: active.length,
    active_planar_distance: planarDistance,
    active_net_vertical: netVertical,
    cache_change_rows: cacheChanges.length,
    cache_change_fraction: cacheChanges.length / (valid.length - 1),
    cache_change_intervals_updates: distribution(changeIntervals),
    cache_unchanged_run_updates: distribution(unchangedRuns),
    cache_vertical_minus_source: {
      rms: rms(gaps),
      distribution: distribution(gaps),
    },
    cache_change_vertical_lag_rms: lagRms,
    source_vertical_steps: distribution(verticalSteps.map(Math.abs)),
    source_opposing_vertical_steps:
      opposingSummary(verticalSteps, direction, planarDistance),
    cache_vertical_steps: distribution(cacheVerticalSteps.map(Math.abs)),
    cache_opposing_vertical_steps:
      opposingSummary(cacheVerticalSteps, cacheDirection, planarDistance),
    cache_source_gap_step_rms: rms(gapSteps),
    limit: 'Broad-update cache cadence and route-shape diagnostics; operator-visible hopping remains a separate observation.'
  };
}

if (process.argv[1]?.endsWith('analyze_cache_cadence.mjs')) {
  if (process.argv.length !== 4) {
    throw new Error('usage: node analyze_cache_cadence.mjs INPUT.csv OUTPUT.json');
  }
  const result = analyzeCacheCadence(parseCsv(fs.readFileSync(process.argv[2], 'utf8')));
  fs.writeFileSync(process.argv[3], `${JSON.stringify(result, null, 2)}\n`);
  if (!result.structural_pass) process.exitCode = 2;
}
