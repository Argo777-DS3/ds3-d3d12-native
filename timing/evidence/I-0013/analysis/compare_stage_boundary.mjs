import fs from 'node:fs';
import path from 'node:path';
import { parseCsv } from '../stage-boundary-observer/analyze_stage_boundary.mjs';

const ACTIVE_THRESHOLD = 0.002;
const RESAMPLE_BINS = 1025;

function decodeFloatBits(hex) {
  const bytes = Buffer.allocUnsafe(4);
  bytes.writeUInt32LE(Number.parseInt(hex, 16));
  return bytes.readFloatLE(0);
}

function cacheTranslation(row) {
  if (!/^[0-9A-Fa-f]{96}$/.test(row.cache48_bits ?? '')) {
    throw new Error('invalid cache48_bits');
  }
  const lanes = Array.from({ length: 12 }, (_, index) =>
    decodeFloatBits(row.cache48_bits.slice(index * 8, index * 8 + 8)));
  return { x: lanes[3], z: lanes[7], y: lanes[11] };
}

function number(row, key) {
  const value = Number(row[key]);
  if (!Number.isFinite(value)) throw new Error(`non-finite ${key}`);
  return value;
}

function samples(rows) {
  return rows.map(row => ({
    qpc: number(row, 'qpc'),
    update: number(row, 'update_index'),
    delta: number(row, 'delta'),
    source: {
      x: number(row, 'source_x'),
      z: number(row, 'source_z'),
      y: number(row, 'source_y'),
    },
    cache: cacheTranslation(row),
    root: {
      x: number(row, 'root_x'),
      z: number(row, 'root_z'),
      y: number(row, 'root_y'),
    },
    same_pointer: row.movement_state === row.root_transform,
    valid: row.valid === '1',
  }));
}

function activeSlice(input) {
  const moving = [];
  for (let index = 1; index < input.length; ++index) {
    const distance = Math.hypot(
      input[index].source.x - input[index - 1].source.x,
      input[index].source.y - input[index - 1].source.y);
    if (distance > ACTIVE_THRESHOLD) moving.push(index);
  }
  if (!moving.length) throw new Error('capture has no active movement');
  return input.slice(moving[0] - 1, moving.at(-1) + 1);
}

function route(input) {
  const progress = [0];
  for (let index = 1; index < input.length; ++index) {
    progress.push(progress.at(-1) + Math.hypot(
      input[index].source.x - input[index - 1].source.x,
      input[index].source.y - input[index - 1].source.y));
  }
  const total = progress.at(-1);
  return { total, normalized: progress.map(value => value / total) };
}

function resample(values, progress, bins = RESAMPLE_BINS) {
  const output = [];
  let right = 1;
  for (let bin = 0; bin < bins; ++bin) {
    const wanted = bin / (bins - 1);
    while (right < progress.length - 1 && progress[right] < wanted) ++right;
    const left = right - 1;
    const width = progress[right] - progress[left];
    const fraction = width > 0 ? (wanted - progress[left]) / width : 0;
    output.push(values[left] + (values[right] - values[left]) * fraction);
  }
  return output;
}

function detrend(values) {
  const start = values[0];
  const net = values.at(-1) - start;
  return values.map((value, index) =>
    value - start - net * index / (values.length - 1));
}

function correlation(left, right) {
  const lm = left.reduce((sum, value) => sum + value, 0) / left.length;
  const rm = right.reduce((sum, value) => sum + value, 0) / right.length;
  let covariance = 0;
  let lv = 0;
  let rv = 0;
  for (let index = 0; index < left.length; ++index) {
    const l = left[index] - lm;
    const r = right[index] - rm;
    covariance += l * r;
    lv += l * l;
    rv += r * r;
  }
  return lv && rv ? covariance / Math.sqrt(lv * rv) : null;
}

function rms(values) {
  return Math.sqrt(values.reduce((sum, value) => sum + value * value, 0) /
    values.length);
}

function difference(left, right) {
  return left.map((value, index) => value - right[index]);
}

function quantile(sorted, fraction) {
  return sorted[Math.floor((sorted.length - 1) * fraction)];
}

function statistics(values) {
  const sorted = [...values].sort((a, b) => a - b);
  return {
    minimum: sorted[0],
    p05: quantile(sorted, 0.05),
    median: quantile(sorted, 0.5),
    p95: quantile(sorted, 0.95),
    maximum: sorted.at(-1),
    mean: values.reduce((sum, value) => sum + value, 0) / values.length,
    rms: rms(values),
  };
}

function summarize(rows, qpcFrequency = 10_000_000) {
  const parsed = samples(rows);
  if (parsed.length < 2 || parsed.some(sample => !sample.valid)) {
    throw new Error('capture is structurally invalid');
  }
  if (!parsed.every((sample, index) =>
      index === 0 || sample.update === parsed[index - 1].update + 1)) {
    throw new Error('capture has an update gap');
  }
  const active = activeSlice(parsed);
  const geometry = route(active);
  const source = active.map(sample => sample.source.z);
  const cache = active.map(sample => sample.cache.z);
  const root = active.map(sample => sample.root.z);
  const gap = cache.map((value, index) => value - source[index]);
  const sourceRoot = root.map((value, index) => value - source[index]);
  const sourceProfile = resample(source, geometry.normalized);
  const cacheProfile = resample(cache, geometry.normalized);
  const rootProfile = resample(root, geometry.normalized);
  const gapProfile = resample(gap, geometry.normalized);
  const duration = (parsed.at(-1).qpc - parsed[0].qpc) / qpcFrequency;
  return {
    public: {
      rows: parsed.length,
      active_rows: active.length,
      measured_rows_per_second: (parsed.length - 1) / duration,
      planar_distance: geometry.total,
      source_net_vertical: source.at(-1) - source[0],
      cache_net_vertical: cache.at(-1) - cache[0],
      root_net_vertical: root.at(-1) - root[0],
      source_root_exact_rows: parsed.filter(sample =>
        sample.source.x === sample.root.x &&
        sample.source.z === sample.root.z &&
        sample.source.y === sample.root.y).length,
      source_root_same_pointer_rows: parsed.filter(sample => sample.same_pointer).length,
      source_root_gap: statistics(sourceRoot),
      cache_minus_source_vertical: statistics(gap),
    },
    sourceProfile,
    cacheProfile,
    rootProfile,
    gapProfile,
    active,
  };
}

function monotonicSpatialMatches(left, right, tolerance) {
  const matches = [];
  let cursor = 0;
  for (const sample of left) {
    let bestIndex = cursor;
    let bestDistance = Number.POSITIVE_INFINITY;
    for (let index = cursor; index < right.length; ++index) {
      const distance = Math.hypot(
        sample.source.x - right[index].source.x,
        sample.source.y - right[index].source.y);
      if (distance < bestDistance) {
        bestDistance = distance;
        bestIndex = index;
      } else if (index > bestIndex + 32 && distance > bestDistance * 2) {
        break;
      }
    }
    if (bestDistance <= tolerance) {
      const other = right[bestIndex];
      matches.push({
        planar_distance: bestDistance,
        source_vertical_difference: other.source.z - sample.source.z,
        left_gap: sample.cache.z - sample.source.z,
        right_gap: other.cache.z - other.source.z,
      });
      cursor = bestIndex;
    }
  }
  if (!matches.length) return { count: 0 };
  const gapDifferences = matches.map(match => match.right_gap - match.left_gap);
  return {
    count: matches.length,
    planar_distance: statistics(matches.map(match => match.planar_distance)),
    source_vertical_difference: statistics(
      matches.map(match => match.source_vertical_difference)),
    cache_source_gap_difference: statistics(gapDifferences),
    cache_source_gap_correlation: correlation(
      matches.map(match => match.left_gap),
      matches.map(match => match.right_gap)),
  };
}

export function compare(leftRows, rightRows) {
  const left = summarize(leftRows);
  const right = summarize(rightRows);
  const leftSourceDetrended = detrend(left.sourceProfile);
  const rightSourceDetrended = detrend(right.sourceProfile);
  const leftCacheDetrended = detrend(left.cacheProfile);
  const rightCacheDetrended = detrend(right.cacheProfile);
  return {
    schema_version: 1,
    investigation_id: 'I-0013',
    active_planar_threshold: ACTIVE_THRESHOLD,
    route_resample_bins: RESAMPLE_BINS,
    captures: { target60: left.public, target240: right.public },
    normalized_route_comparison: {
      source_vertical_profile_correlation:
        correlation(leftSourceDetrended, rightSourceDetrended),
      cache_vertical_profile_correlation:
        correlation(leftCacheDetrended, rightCacheDetrended),
      cache_source_gap_profile_correlation:
        correlation(left.gapProfile, right.gapProfile),
      source_profile_difference_rms:
        rms(difference(leftSourceDetrended, rightSourceDetrended)),
      cache_profile_difference_rms:
        rms(difference(leftCacheDetrended, rightCacheDetrended)),
      cache_source_gap_difference_rms:
        rms(difference(left.gapProfile, right.gapProfile)),
      target240_to_target60_raw_gap_rms_ratio:
        right.public.cache_minus_source_vertical.rms /
        left.public.cache_minus_source_vertical.rms,
    },
    monotonic_spatial_match_sensitivity: Object.fromEntries(
      [0.02, 0.05, 0.1].map(tolerance => [String(tolerance),
        monotonicSpatialMatches(left.active, right.active, tolerance)])),
    bounded_interpretation:
      'The source and final root are the same pointer and bit-identical in these captures. Cache/source comparisons test this sampled cache boundary only; they do not identify collision or physics ownership.',
  };
}

if (process.argv[1] && process.argv[1].endsWith('compare_stage_boundary.mjs')) {
  const leftIndex = process.argv.indexOf('--60');
  const rightIndex = process.argv.indexOf('--240');
  const outIndex = process.argv.indexOf('--out');
  if (leftIndex < 0 || rightIndex < 0 || outIndex < 0) {
    throw new Error('usage: node compare_stage_boundary.mjs --60 A.csv --240 B.csv --out result.json');
  }
  const result = compare(
    parseCsv(fs.readFileSync(path.resolve(process.argv[leftIndex + 1]), 'utf8')),
    parseCsv(fs.readFileSync(path.resolve(process.argv[rightIndex + 1]), 'utf8')));
  fs.writeFileSync(path.resolve(process.argv[outIndex + 1]),
                   `${JSON.stringify(result, null, 2)}\n`);
}
