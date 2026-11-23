import fs from 'node:fs';
import path from 'node:path';
import { parseCaptureCsv } from '../prototype/analyze_capture.mjs';

const CELLS = ['60-off', '240-off', '240-on', '60-on'];
const ACTIVE_THRESHOLD = 0.002;
const RESAMPLE_BINS = 1025;

function activeSamples(samples) {
  const valid = samples.filter(sample => sample.valid);
  const moving = [];
  for (let index = 1; index < valid.length; index++) {
    const planar = Math.hypot(valid[index].x - valid[index - 1].x,
                              valid[index].y - valid[index - 1].y);
    if (planar > ACTIVE_THRESHOLD) moving.push(index);
  }
  if (!moving.length) throw new Error('capture has no active movement');
  return valid.slice(moving[0] - 1, moving.at(-1) + 1);
}

function route(samples) {
  const progress = [0];
  for (let index = 1; index < samples.length; index++) {
    progress.push(progress.at(-1) + Math.hypot(
      samples[index].x - samples[index - 1].x,
      samples[index].y - samples[index - 1].y));
  }
  const total = progress.at(-1);
  const normalized = progress.map(value => value / total);
  return { progress, normalized, total };
}

function resample(samples, normalized, bins = RESAMPLE_BINS) {
  const output = [];
  let right = 1;
  for (let bin = 0; bin < bins; bin++) {
    const wanted = bin / (bins - 1);
    while (right < normalized.length - 1 && normalized[right] < wanted) right++;
    const left = right - 1;
    const span = normalized[right] - normalized[left];
    const fraction = span > 0 ? (wanted - normalized[left]) / span : 0;
    output.push(samples[left].z +
      (samples[right].z - samples[left].z) * fraction);
  }
  return output;
}

function correlation(left, right) {
  const leftMean = left.reduce((sum, value) => sum + value, 0) / left.length;
  const rightMean = right.reduce((sum, value) => sum + value, 0) / right.length;
  let covariance = 0;
  let leftVariance = 0;
  let rightVariance = 0;
  for (let index = 0; index < left.length; index++) {
    const a = left[index] - leftMean;
    const b = right[index] - rightMean;
    covariance += a * b;
    leftVariance += a * a;
    rightVariance += b * b;
  }
  return covariance / Math.sqrt(leftVariance * rightVariance);
}

function detrend(values) {
  const start = values[0];
  const net = values.at(-1) - start;
  return values.map((value, index) =>
    value - start - net * index / (values.length - 1));
}

function opposingEvents(samples, normalized) {
  const direction = Math.sign(samples.at(-1).z - samples[0].z);
  const events = [];
  let current = null;
  for (let index = 1; index < samples.length; index++) {
    const step = samples[index].z - samples[index - 1].z;
    const opposing = direction !== 0 && step * direction < -1e-6;
    if (opposing) {
      if (!current) {
        current = { first: index - 1, last: index, distance: 0, peak_step: 0 };
      }
      current.last = index;
      current.distance += Math.abs(step);
      current.peak_step = Math.max(current.peak_step, Math.abs(step));
    } else if (current) {
      events.push(current);
      current = null;
    }
  }
  if (current) events.push(current);
  return events.map(event => ({
    progress_start: normalized[event.first],
    progress_end: normalized[event.last],
    progress_center: (normalized[event.first] + normalized[event.last]) / 2,
    opposing_vertical_distance: event.distance,
    peak_vertical_step: event.peak_step,
    update_intervals: event.last - event.first,
  })).filter(event => event.opposing_vertical_distance >= 0.001);
}

function matchedEvents(left, right, tolerance = 0.015) {
  const matches = [];
  const used = new Set();
  for (const event of left) {
    let best = null;
    for (let index = 0; index < right.length; index++) {
      if (used.has(index)) continue;
      const distance = Math.abs(event.progress_center - right[index].progress_center);
      if (distance <= tolerance && (!best || distance < best.distance)) {
        best = { index, distance };
      }
    }
    if (best) {
      used.add(best.index);
      matches.push({
        left_progress: event.progress_center,
        right_progress: right[best.index].progress_center,
        progress_delta: best.distance,
        left_distance: event.opposing_vertical_distance,
        right_distance: right[best.index].opposing_vertical_distance,
      });
    }
  }
  return matches;
}

function summarize(samples) {
  const active = activeSamples(samples);
  const geometry = route(active);
  const vertical = resample(active, geometry.normalized);
  return {
    rows: samples.length,
    active_rows: active.length,
    planar_distance: geometry.total,
    net_vertical: active.at(-1).z - active[0].z,
    detrended_vertical: detrend(vertical),
    opposing_events: opposingEvents(active, geometry.normalized),
  };
}

function publicSummary(summary) {
  const { detrended_vertical, ...result } = summary;
  return result;
}

function readCell(session, cell) {
  const matches = fs.readdirSync(session).filter(name =>
    new RegExp(`^${cell}-attempt-\\d+\\.csv$`).test(name));
  if (matches.length !== 1) throw new Error(`${cell} capture is not unique`);
  return summarize(parseCaptureCsv(
    fs.readFileSync(path.join(session, matches[0]), 'utf8')));
}

const sessionIndex = process.argv.indexOf('--session');
const outputIndex = process.argv.indexOf('--out');
if (sessionIndex < 0 || !process.argv[sessionIndex + 1]) {
  throw new Error('usage: node analyze_terrain_events.mjs --session DIR [--out FILE]');
}
const session = path.resolve(process.argv[sessionIndex + 1]);
const summaries = Object.fromEntries(CELLS.map(cell => [cell, readCell(session, cell)]));
const output = {
  schema_version: 1,
  investigation_id: 'I-0013',
  active_planar_threshold: ACTIVE_THRESHOLD,
  route_resample_bins: RESAMPLE_BINS,
  note: 'Opposing events are contiguous vertical steps against the route net direction; matching is by normalized planar progress within 0.015.',
  captures: Object.fromEntries(Object.entries(summaries).map(
    ([cell, summary]) => [cell, publicSummary(summary)])),
  route_profile_correlations: {
    '240-off_to_240-on': correlation(
      summaries['240-off'].detrended_vertical,
      summaries['240-on'].detrended_vertical),
    '60-off_to_60-on': correlation(
      summaries['60-off'].detrended_vertical,
      summaries['60-on'].detrended_vertical),
    '60-off_to_240-off': correlation(
      summaries['60-off'].detrended_vertical,
      summaries['240-off'].detrended_vertical),
    '60-on_to_240-on': correlation(
      summaries['60-on'].detrended_vertical,
      summaries['240-on'].detrended_vertical),
  },
  opposing_event_matches: {
    '240-off_to_240-on': matchedEvents(
      summaries['240-off'].opposing_events,
      summaries['240-on'].opposing_events),
    '60-off_to_60-on': matchedEvents(
      summaries['60-off'].opposing_events,
      summaries['60-on'].opposing_events),
  },
};
const json = JSON.stringify(output, null, 2) + '\n';
if (outputIndex >= 0 && process.argv[outputIndex + 1]) {
  fs.writeFileSync(path.resolve(process.argv[outputIndex + 1]), json);
} else {
  process.stdout.write(json);
}
