import fs from 'node:fs';
import path from 'node:path';
import { parseCaptureCsv } from '../prototype/analyze_capture.mjs';

const CELLS = ['60-off', '240-off', '240-on', '60-on'];
const THRESHOLDS = [0.001, 0.002, 0.005];

function sum(values) {
  return values.reduce((total, value) => total + value, 0);
}

function ratio(numerator, denominator) {
  return denominator ? numerator / denominator : null;
}

function intervalData(samples) {
  const intervals = [];
  for (let index = 1; index < samples.length; index++) {
    const dx = samples[index].x - samples[index - 1].x;
    const dz = samples[index].z - samples[index - 1].z;
    const dy = samples[index].y - samples[index - 1].y;
    intervals.push({
      left: index - 1,
      right: index,
      planar: Math.hypot(dx, dy),
      spatial: Math.hypot(dx, dz, dy),
      delta: samples[index].delta,
    });
  }
  return intervals;
}

function rangeMetrics(samples, intervals, firstSample, lastSample) {
  const selected = intervals.filter(interval =>
    interval.left >= firstSample && interval.right <= lastSample);
  const start = samples[firstSample];
  const end = samples[lastSample];
  const duration = (end.qpc - start.qpc) / samples.qpcFrequency;
  const planarDistance = sum(selected.map(interval => interval.planar));
  const spatialDistance = sum(selected.map(interval => interval.spatial));
  return {
    first_update_index: start.updateIndex,
    last_update_index: end.updateIndex,
    update_intervals: end.updateIndex - start.updateIndex,
    qpc_duration_seconds: duration,
    accumulated_delta_seconds: sum(selected.map(interval => interval.delta)),
    planar_path_distance: planarDistance,
    spatial_path_distance: spatialDistance,
    planar_net_displacement: Math.hypot(end.x - start.x, end.y - start.y),
    spatial_net_displacement: Math.hypot(
      end.x - start.x, end.z - start.z, end.y - start.y),
    planar_path_speed: planarDistance / duration,
    spatial_path_speed: spatialDistance / duration,
  };
}

function summarize(samples, qpcFrequency) {
  const valid = samples.filter(sample => sample.valid);
  if (valid.length < 2) throw new Error('insufficient valid samples');
  valid.qpcFrequency = qpcFrequency;
  const intervals = intervalData(valid);
  const full = rangeMetrics(valid, intervals, 0, valid.length - 1);
  const active = {};
  for (const threshold of THRESHOLDS) {
    const moving = intervals.filter(interval => interval.planar > threshold);
    if (!moving.length) throw new Error(`no movement above ${threshold}`);
    const first = moving[0].left;
    const last = moving.at(-1).right;
    active[threshold.toFixed(3)] = {
      threshold_planar_units_per_update: threshold,
      excluded_leading_samples: first,
      excluded_trailing_samples: valid.length - 1 - last,
      ...rangeMetrics(valid, intervals, first, last),
    };
  }
  return {
    rows: samples.length,
    valid_rows: valid.length,
    target_fps: valid[0].targetFps,
    correction: valid[0].correction,
    full_capture: full,
    active_movement: active,
  };
}

function compare(left, right) {
  const fields = [
    'qpc_duration_seconds', 'accumulated_delta_seconds',
    'planar_path_distance', 'spatial_path_distance',
    'planar_net_displacement', 'spatial_net_displacement',
    'planar_path_speed', 'spatial_path_speed',
  ];
  const result = {};
  for (const threshold of THRESHOLDS.map(value => value.toFixed(3))) {
    result[threshold] = Object.fromEntries(fields.map(field => [
      `${field}_ratio`,
      ratio(right.active_movement[threshold][field],
            left.active_movement[threshold][field]),
    ]));
  }
  return result;
}

export function compareSession(session) {
  const info = fs.readFileSync(path.join(session, 'session-info.txt'), 'utf8');
  const frequencyMatch = /^qpc_frequency=(\d+)$/m.exec(info);
  if (!frequencyMatch) throw new Error('missing QPC frequency');
  const frequency = Number(frequencyMatch[1]);
  const files = fs.readdirSync(session);
  const captures = {};
  for (const cell of CELLS) {
    const matches = files.filter(name =>
      new RegExp(`^${cell}-attempt-\\d+\\.csv$`).test(name));
    if (matches.length !== 1) {
      throw new Error(`${cell} requires exactly one complete capture`);
    }
    captures[cell] = summarize(parseCaptureCsv(
      fs.readFileSync(path.join(session, matches[0]), 'utf8')), frequency);
  }
  return {
    schema_version: 1,
    investigation_id: 'I-0013',
    movement_thresholds: THRESHOLDS,
    note: 'Active windows exclude leading/trailing overlay interaction; ratios are right/left.',
    captures,
    comparisons: {
      '60-off_to_240-off': compare(captures['60-off'], captures['240-off']),
      '60-on_to_240-on': compare(captures['60-on'], captures['240-on']),
      '60-off_to_60-on': compare(captures['60-off'], captures['60-on']),
      '240-off_to_240-on': compare(captures['240-off'], captures['240-on']),
    },
  };
}

const sessionIndex = process.argv.indexOf('--session');
const outputIndex = process.argv.indexOf('--out');
if (sessionIndex < 0 || !process.argv[sessionIndex + 1]) {
  throw new Error('usage: node compare_distance.mjs --session DIR [--out FILE]');
}
const result = compareSession(path.resolve(process.argv[sessionIndex + 1]));
const json = JSON.stringify(result, null, 2) + '\n';
if (outputIndex >= 0 && process.argv[outputIndex + 1]) {
  fs.writeFileSync(path.resolve(process.argv[outputIndex + 1]), json);
} else {
  process.stdout.write(json);
}
