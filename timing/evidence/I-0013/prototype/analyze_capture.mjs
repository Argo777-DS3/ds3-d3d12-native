import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

const HEADER = [
  'qpc', 'update_index', 'player', 'transform', 'delta', 'x', 'z', 'y',
  'valid', 'target_fps', 'correction',
];

const median = values => {
  if (!values.length) return null;
  const sorted = [...values].sort((a, b) => a - b);
  const middle = Math.floor(sorted.length / 2);
  return sorted.length % 2 ? sorted[middle] :
    (sorted[middle - 1] + sorted[middle]) / 2;
};

const rms = values => values.length ?
  Math.sqrt(values.reduce((sum, value) => sum + value * value, 0) /
            values.length) : null;

export function parseCaptureCsv(text) {
  const lines = text.replace(/^\uFEFF/, '').trim().split(/\r?\n/);
  if (!lines.length) throw new Error('empty capture');
  const header = lines.shift().split(',');
  if (header.join(',') !== HEADER.join(',')) {
    throw new Error(`unexpected capture header: ${header.join(',')}`);
  }
  return lines.filter(Boolean).map((line, row) => {
    const cells = line.split(',');
    if (cells.length !== HEADER.length) {
      throw new Error(`row ${row + 2} has ${cells.length} columns`);
    }
    const sample = {
      qpc: Number(cells[0]), updateIndex: Number(cells[1]),
      player: cells[2], transform: cells[3], delta: Number(cells[4]),
      x: Number(cells[5]), z: Number(cells[6]), y: Number(cells[7]),
      valid: cells[8] === '1', targetFps: Number(cells[9]),
      correction: cells[10] === '1',
    };
    if (![sample.qpc, sample.updateIndex, sample.delta, sample.x, sample.z,
          sample.y, sample.targetFps].every(Number.isFinite)) {
      throw new Error(`row ${row + 2} contains a non-finite number`);
    }
    return sample;
  });
}

export function resampleByProgress(samples, bins = 257) {
  if (samples.length < 2 || bins < 3) throw new Error('insufficient resample input');
  const progress = [0];
  for (let index = 1; index < samples.length; index++) {
    progress.push(progress[index - 1] + Math.hypot(
      samples[index].x - samples[index - 1].x,
      samples[index].y - samples[index - 1].y));
  }
  const total = progress.at(-1);
  if (!(total > 0)) throw new Error('zero planar route length');
  const output = [];
  let right = 1;
  for (let bin = 0; bin < bins; bin++) {
    const wanted = total * bin / (bins - 1);
    while (right < progress.length - 1 && progress[right] < wanted) right++;
    const left = right - 1;
    const span = progress[right] - progress[left];
    const fraction = span > 0 ? (wanted - progress[left]) / span : 0;
    output.push(samples[left].z +
      (samples[right].z - samples[left].z) * fraction);
  }
  return output;
}

export function summarizeCapture(samples, qpcFrequency) {
  if (!(qpcFrequency > 0)) throw new Error('invalid QPC frequency');
  if (samples.length < 2) throw new Error('capture has fewer than two rows');
  for (let index = 1; index < samples.length; index++) {
    if (samples[index].qpc <= samples[index - 1].qpc ||
        samples[index].updateIndex <= samples[index - 1].updateIndex) {
      throw new Error('timestamps and update indices must be strictly increasing');
    }
  }
  const valid = samples.filter(sample => sample.valid);
  if (valid.length < 2) throw new Error('capture has fewer than two valid rows');
  const targets = new Set(samples.map(sample => sample.targetFps));
  const corrections = new Set(samples.map(sample => sample.correction));
  if (targets.size !== 1 || corrections.size !== 1) {
    throw new Error('capture control state changed within one cell');
  }
  const players = new Set(valid.map(sample => sample.player));
  const transforms = new Set(valid.map(sample => sample.transform));
  if (players.size !== 1 || transforms.size !== 1) {
    throw new Error('valid rows do not retain one player/transform identity');
  }
  const planarSteps = [];
  const verticalSteps = [];
  for (let index = 1; index < valid.length; index++) {
    planarSteps.push(Math.hypot(valid[index].x - valid[index - 1].x,
                                valid[index].y - valid[index - 1].y));
    verticalSteps.push(valid[index].z - valid[index - 1].z);
  }
  const netVertical = valid.at(-1).z - valid[0].z;
  const verticalVariation = verticalSteps.reduce(
    (sum, value) => sum + Math.abs(value), 0);
  const direction = Math.sign(netVertical);
  const deadband = Math.max(1e-6, Math.abs(netVertical) * 1e-5);
  let reversals = 0;
  let priorSign = 0;
  let opposingDistance = 0;
  for (const step of verticalSteps) {
    if (direction && step * direction < -deadband) opposingDistance += Math.abs(step);
    const sign = Math.abs(step) <= deadband ? 0 : Math.sign(step);
    if (sign && priorSign && sign !== priorSign) reversals++;
    if (sign) priorSign = sign;
  }
  const resampled = resampleByProgress(valid);
  const secondDifferences = [];
  for (let index = 1; index < resampled.length - 1; index++) {
    secondDifferences.push(
      resampled[index + 1] - 2 * resampled[index] + resampled[index - 1]);
  }
  const elapsed = (valid.at(-1).qpc - valid[0].qpc) / qpcFrequency;
  return {
    rows: samples.length,
    valid_rows: valid.length,
    invalid_rows: samples.length - valid.length,
    valid_fraction: valid.length / samples.length,
    target_fps: [...targets][0],
    correction: [...corrections][0],
    measured_qpc_fps: (valid.at(-1).updateIndex - valid[0].updateIndex) /
      elapsed,
    median_delta: median(valid.map(sample => sample.delta)),
    planar_distance: planarSteps.reduce((sum, value) => sum + value, 0),
    net_vertical: netVertical,
    vertical_total_variation: verticalVariation,
    vertical_excess: verticalVariation - Math.abs(netVertical),
    opposing_vertical_distance: opposingDistance,
    vertical_reversal_count: reversals,
    max_abs_vertical_step: Math.max(...verticalSteps.map(Math.abs)),
    resampled_second_difference_rms: rms(secondDifferences),
    player: [...players][0],
    transform: [...transforms][0],
    _resampled: resampled,
  };
}

export function compareRoute(reference, candidate) {
  const left = reference._resampled;
  const right = candidate._resampled;
  if (!left || !right || left.length !== right.length) {
    throw new Error('route comparison requires equal resampled traces');
  }
  const leftStart = left[0];
  const rightStart = right[0];
  const leftNet = left.at(-1) - leftStart;
  const rightNet = right.at(-1) - rightStart;
  const shapeErrors = [];
  const detrendedErrors = [];
  for (let index = 0; index < left.length; index++) {
    const u = index / (left.length - 1);
    const leftRelative = left[index] - leftStart;
    const rightRelative = right[index] - rightStart;
    shapeErrors.push(rightRelative - leftRelative);
    detrendedErrors.push(
      (rightRelative - u * rightNet) - (leftRelative - u * leftNet));
  }
  return {
    route_shape_rms: rms(shapeErrors),
    route_shape_detrended_rms: rms(detrendedErrors),
    planar_distance_ratio: candidate.planar_distance /
      reference.planar_distance,
    vertical_excess_delta: candidate.vertical_excess -
      reference.vertical_excess,
    opposing_vertical_distance_delta: candidate.opposing_vertical_distance -
      reference.opposing_vertical_distance,
    reversal_count_delta: candidate.vertical_reversal_count -
      reference.vertical_reversal_count,
    roughness_ratio: reference.resampled_second_difference_rms > 0 ?
      candidate.resampled_second_difference_rms /
        reference.resampled_second_difference_rms : null,
  };
}

function readSessionInfo(session) {
  const text = fs.readFileSync(path.join(session, 'session-info.txt'), 'utf8');
  const match = /^qpc_frequency=(\d+)$/m.exec(text);
  if (!match) throw new Error('session-info.txt lacks qpc_frequency');
  return { qpcFrequency: Number(match[1]) };
}

function publicSummary(summary) {
  const { _resampled, ...result } = summary;
  return result;
}

export function analyzeSession(session) {
  const { qpcFrequency } = readSessionInfo(session);
  const files = fs.readdirSync(session)
    .filter(name => /^(60-off|240-off|240-on|60-on)-attempt-\d+\.csv$/.test(name))
    .sort();
  if (!files.length) throw new Error('session contains no complete capture CSV');
  const captures = {};
  for (const name of files) {
    const cell = name.split('-attempt-')[0];
    if (captures[cell]) throw new Error(`multiple complete attempts for ${cell}`);
    captures[cell] = summarizeCapture(
      parseCaptureCsv(fs.readFileSync(path.join(session, name), 'utf8')),
      qpcFrequency);
  }
  const reference = captures['60-off'];
  if (!reference) throw new Error('60-off reference is required');
  const comparisons = {};
  for (const cell of ['240-off', '240-on', '60-on']) {
    if (captures[cell]) comparisons[cell] = compareRoute(reference, captures[cell]);
  }
  return {
    schema_version: 1,
    investigation_id: 'I-0013',
    classification: 'unassigned; thresholds and operator observation required',
    captures: Object.fromEntries(Object.entries(captures)
      .map(([key, value]) => [key, publicSummary(value)])),
    comparisons_to_60_off: comparisons,
  };
}

const invoked = process.argv[1] &&
  path.resolve(process.argv[1]) === fileURLToPath(import.meta.url);
if (invoked) {
  const sessionIndex = process.argv.indexOf('--session');
  const outputIndex = process.argv.indexOf('--out');
  if (sessionIndex < 0 || !process.argv[sessionIndex + 1]) {
    throw new Error('usage: node analyze_capture.mjs --session DIR [--out FILE]');
  }
  const result = analyzeSession(path.resolve(process.argv[sessionIndex + 1]));
  const json = JSON.stringify(result, null, 2) + '\n';
  if (outputIndex >= 0 && process.argv[outputIndex + 1]) {
    fs.writeFileSync(path.resolve(process.argv[outputIndex + 1]), json);
  } else {
    process.stdout.write(json);
  }
}
