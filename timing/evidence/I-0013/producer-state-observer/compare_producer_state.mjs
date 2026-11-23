import fs from 'node:fs';
import { parseCsv } from '../stage-boundary-observer/analyze_stage_boundary.mjs';

const f32 = Math.fround;
const referenceDelta = f32(0.0166666005);
const nativeGain = f32(0.2);
const nativeRetention = f32(0.6);

function rows(path) {
  return parseCsv(fs.readFileSync(path, 'utf8')).filter(row => row.valid === '1');
}

function cadence(data) {
  const elapsed = (Number(data.at(-1).qpc_entry) - Number(data[0].qpc_entry)) / 10_000_000;
  return (data.length - 1) / elapsed;
}

function active(data) {
  const moving = [];
  for (let i = 1; i < data.length; i++) {
    const dx = Number(data[i].source_before_x) - Number(data[i - 1].source_before_x);
    const dy = Number(data[i].source_before_y) - Number(data[i - 1].source_before_y);
    if (Math.hypot(dx, dy) > 1e-5) moving.push(i);
  }
  if (!moving.length) throw new Error('No active route interval');
  return data.slice(Math.max(0, moving[0] - 1), moving.at(-1) + 1);
}

function model(data, kind) {
  const observed = data.map(row => f32(Number(row.producer_state)));
  const output = [observed[0]];
  let exactNativeReplay = 0;
  for (let i = 1; i < data.length; i++) {
    const nativeStep = f32(observed[i] - observed[i - 1]);
    const response = f32(nativeStep / nativeGain);
    const target = f32(observed[i - 1] + response);
    const dt = f32(Number(data[i].observed_delta));
    const ratio = dt / referenceDelta;
    const gain = kind === 'exponential'
      ? f32(1 - Math.pow(1 - nativeGain, ratio))
      : f32(nativeGain * ratio);
    const next = f32(output[i - 1] + f32(gain * f32(target - output[i - 1])));
    output.push(next);
    if (Object.is(f32(observed[i - 1] + f32(nativeGain * response)), observed[i])) {
      exactNativeReplay++;
    }
  }
  return { output, exactNativeReplay, transitions: Math.max(0, data.length - 1) };
}

function routeMetric(data, values) {
  let path = 0;
  let variation = 0;
  for (let i = 1; i < data.length; i++) {
    const dx = Number(data[i].source_before_x) - Number(data[i - 1].source_before_x);
    const dy = Number(data[i].source_before_y) - Number(data[i - 1].source_before_y);
    path += Math.hypot(dx, dy);
    variation += Math.abs(values[i] - values[i - 1]);
  }
  const net = Math.abs(values.at(-1) - values[0]);
  const opposing = Math.max(0, variation - net);
  return { rows: data.length, planar_path: path, total_variation: variation,
    net_change_abs: net, opposing, opposing_per_planar_unit: opposing / path };
}

function summarize(data) {
  const selected = active(data);
  const observed = selected.map(row => f32(Number(row.producer_state)));
  const exponential = model(selected, 'exponential');
  const linear = model(selected, 'linear');
  return {
    row_count: data.length,
    measured_calls_per_second: cadence(data),
    observed: routeMetric(selected, observed),
    exponential_replay: routeMetric(selected, exponential.output),
    linear_replay: routeMetric(selected, linear.output),
    exact_native_additive_replay_rows: exponential.exactNativeReplay,
    additive_transitions: exponential.transitions,
  };
}

export function compareProducerState(lowRows, highRows) {
  const low = summarize(lowRows);
  const high = summarize(highRows);
  const ratio = key => high[key].opposing_per_planar_unit / low.observed.opposing_per_planar_unit;
  return {
    structural_pass: low.exact_native_additive_replay_rows === low.additive_transitions &&
      high.exact_native_additive_replay_rows === high.additive_transitions,
    reference_delta_float32: referenceDelta,
    native_gain_float32: nativeGain,
    native_retention_float32: nativeRetention,
    laws: {
      affine_gain: 'g(dt) = 1 - (1 - 0.2)^(dt / dt60)',
      independent_increment_gain: 'g(dt) = 0.2 * dt / dt60',
      decay_retention: 'r(dt) = 0.6^(dt / dt60)',
    },
    target60: low,
    target240: high,
    high_to_native60_opposing_density: {
      observed: ratio('observed'),
      exponential_replay: ratio('exponential_replay'),
      linear_replay: ratio('linear_replay'),
    },
    limit: 'Replays each captured response under two principled timestep laws. Because the response itself may depend on prior state, replay is a model discriminator, not live patch validation.',
  };
}

if (process.argv[1]?.endsWith('compare_producer_state.mjs')) {
  if (process.argv.length !== 5) throw new Error('usage: node compare_producer_state.mjs LOW.csv HIGH.csv OUTPUT.json');
  const result = compareProducerState(rows(process.argv[2]), rows(process.argv[3]));
  fs.writeFileSync(process.argv[4], `${JSON.stringify(result, null, 2)}\n`);
  if (!result.structural_pass) process.exitCode = 2;
}
