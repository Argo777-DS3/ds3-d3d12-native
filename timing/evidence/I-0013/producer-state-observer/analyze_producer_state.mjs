import fs from 'node:fs';
import { parseCsv } from '../stage-boundary-observer/analyze_stage_boundary.mjs';

const f32 = Math.fround;
const numeric = (row, key) => Number.parseFloat(row[key]);
const bits = value => {
  const buffer = new ArrayBuffer(4);
  new Float32Array(buffer)[0] = f32(value);
  return new Uint32Array(buffer)[0];
};
const sameF32 = (a, b) => bits(a) === bits(b);
const stats = values => {
  if (!values.length) return { count: 0, min: null, max: null, mean: null };
  return {
    count: values.length,
    min: Math.min(...values),
    max: Math.max(...values),
    mean: values.reduce((sum, value) => sum + value, 0) / values.length,
  };
};
function variation(values, rows) {
  let path = 0;
  let total = 0;
  for (let i = 1; i < values.length; i++) {
    total += Math.abs(values[i] - values[i - 1]);
    const dx = numeric(rows[i], 'source_before_x') - numeric(rows[i - 1], 'source_before_x');
    const dy = numeric(rows[i], 'source_before_y') - numeric(rows[i - 1], 'source_before_y');
    path += Math.hypot(dx, dy);
  }
  const net = values.length > 1 ? Math.abs(values.at(-1) - values[0]) : 0;
  const opposing = Math.max(0, total - net);
  return { total, net, opposing, planar_path: path,
    opposing_per_planar_unit: path > 0 ? opposing / path : null };
}

export function analyzeProducerState(rows) {
  const required = ['owner_e0_target', 'actor_178_target', 'producer_state',
    'retention', 'observed_delta', 'producer_mode', 'carried',
    'source_before_x', 'source_before_y', 'valid'];
  const missing = required.filter(key => rows.length && !(key in rows[0]));
  const valid = rows.filter(row => row.valid === '1');
  const finite = valid.every(row => ['producer_state', 'retention',
    'observed_delta', 'carried'].every(key => Number.isFinite(numeric(row, key))));
  const targets = Object.fromEntries(['owner_e0_target', 'actor_178_target'].map(key =>
    [key, [...new Set(valid.map(row => row[key].toUpperCase()))].sort()]));
  const stableTargets = Object.values(targets).every(values => values.length === 1);
  const states = valid.map(row => f32(numeric(row, 'producer_state')));
  const carried = valid.map(row => f32(numeric(row, 'carried')));
  const reconstructedFirst = carried.map((value, index) => f32(value - states[index]));
  const exactRecomposition = carried.map((value, index) =>
    sameF32(f32(reconstructedFirst[index] + states[index]), value));

  const recurrence = { unchanged: 0, reset_zero: 0, clamp_positive: 0,
    clamp_negative: 0, retention_exact: 0, other: 0 };
  for (let i = 1; i < states.length; i++) {
    const current = states[i];
    const previous = states[i - 1];
    const retention = f32(numeric(valid[i], 'retention'));
    if (sameF32(current, previous)) recurrence.unchanged++;
    else if (Object.is(current, 0) || Object.is(current, -0)) recurrence.reset_zero++;
    else if (sameF32(current, 0.5)) recurrence.clamp_positive++;
    else if (sameF32(current, -0.5)) recurrence.clamp_negative++;
    else if (sameF32(current, f32(previous * retention))) recurrence.retention_exact++;
    else recurrence.other++;
  }
  const deltas = valid.map(row => numeric(row, 'observed_delta'));
  const retentions = valid.map(row => numeric(row, 'retention'));
  const modes = [...new Set(valid.map(row => Number.parseInt(row.producer_mode, 10)))].sort((a, b) => a - b);
  return {
    structural_pass: rows.length >= 2 && valid.length === rows.length &&
      missing.length === 0 && finite && stableTargets,
    row_count: rows.length,
    valid_row_count: valid.length,
    missing_columns: missing,
    finite_state_fields: finite,
    stable_targets: stableTargets,
    targets,
    producer_modes: modes,
    observed_delta: stats(deltas),
    retention: stats(retentions),
    carried_recomposition: {
      exact_rows: exactRecomposition.filter(Boolean).length,
      total_rows: exactRecomposition.length,
      reconstructed_first_is_subtraction_bound_only: true,
    },
    recurrence,
    producer_state_variation: variation(states, valid),
    reconstructed_first_variation: variation(reconstructedFirst, valid),
    limit: 'Snapshots classify exact adjacent-state relations only. The reconstructed first contribution is an algebraic subtraction bound, not a direct call-return observation.',
  };
}

if (process.argv[1]?.endsWith('analyze_producer_state.mjs')) {
  if (process.argv.length !== 4) throw new Error('usage: node analyze_producer_state.mjs INPUT.csv OUTPUT.json');
  const result = analyzeProducerState(parseCsv(fs.readFileSync(process.argv[2], 'utf8')));
  fs.writeFileSync(process.argv[3], `${JSON.stringify(result, null, 2)}\n`);
  if (!result.structural_pass) process.exitCode = 2;
}
