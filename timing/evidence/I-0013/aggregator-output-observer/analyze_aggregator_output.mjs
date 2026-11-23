import fs from 'node:fs';
import { parseCsv } from '../stage-boundary-observer/analyze_stage_boundary.mjs';

function matrix(hex) {
  if (!/^[0-9A-Fa-f]{96}$/.test(hex ?? '')) return null;
  return Array.from({ length: 12 }, (_, i) => {
    const b = Buffer.allocUnsafe(4);
    b.writeUInt32LE(Number.parseInt(hex.slice(i * 8, i * 8 + 8), 16));
    return b.readFloatLE(0);
  });
}

function opposing(values, direction, distance) {
  const steps = values.slice(1).map((v, i) => v - values[i]);
  const selected = steps.filter(v => direction && v * direction < -1e-6);
  const total = selected.reduce((sum, v) => sum + Math.abs(v), 0);
  return { count: selected.length, total_distance: total,
    count_per_planar_unit: distance ? selected.length / distance : null,
    distance_per_planar_unit: distance ? total / distance : null };
}

export function analyzeAggregator(rows, qpcFrequency = 10_000_000) {
  const required = ['qpc_entry', 'qpc_exit', 'sequence', 'caller_return',
    'owner', 'movement', 'output', 'source_before_x', 'source_before_z',
    'source_before_y', 'source_after_x', 'source_after_z', 'source_after_y',
    'carried', 'mode_a', 'mode_b', 'vertical_filter', 'override_gate',
    'local_predicate', 'cache_before48_bits', 'final_output48_bits', 'valid'];
  const missing = required.filter(k => rows.length && !(k in rows[0]));
  const valid = rows.filter(r => r.valid === '1');
  const allowed = new Set([0x005C6977, 0x005C6F47, 0x005CAA80]);
  const callers = {};
  for (const r of valid) callers[r.caller_return] = (callers[r.caller_return] ?? 0) + 1;
  const consecutive = valid.every((r, i) => !i || Number(r.sequence) === Number(valid[i - 1].sequence) + 1);
  const callerValid = valid.every(r => allowed.has(Number.parseInt(r.caller_return, 16)));
  const pointersValid = valid.every(r => ['owner', 'movement', 'output'].every(k => Number.parseInt(r[k], 16) !== 0));
  const decoded = valid.map(r => ({ cache: matrix(r.cache_before48_bits), final: matrix(r.final_output48_bits) }));
  const finite = valid.every((r, i) => decoded[i].cache?.every(Number.isFinite) && decoded[i].final?.every(Number.isFinite) &&
    ['qpc_entry', 'qpc_exit', 'source_before_x', 'source_before_z', 'source_before_y', 'source_after_x', 'source_after_z', 'source_after_y', 'carried', 'vertical_filter', 'override_gate'].every(k => Number.isFinite(Number(r[k]))));
  const qpcMonotone = valid.every((r, i) => Number(r.qpc_exit) >= Number(r.qpc_entry) && (!i || Number(r.qpc_entry) > Number(valid[i - 1].qpc_entry)));
  const moving = [];
  for (let i = 1; i < valid.length; ++i) {
    if (Math.hypot(Number(valid[i].source_before_x) - Number(valid[i - 1].source_before_x), Number(valid[i].source_before_y) - Number(valid[i - 1].source_before_y)) > 0.002) moving.push(i);
  }
  const active = moving.length ? valid.slice(moving[0] - 1, moving.at(-1) + 1) : valid;
  const activeDecoded = active.map(r => ({ cache: matrix(r.cache_before48_bits), final: matrix(r.final_output48_bits) }));
  let planar = 0;
  for (let i = 1; i < active.length; ++i) planar += Math.hypot(Number(active[i].source_before_x) - Number(active[i - 1].source_before_x), Number(active[i].source_before_y) - Number(active[i - 1].source_before_y));
  const sourceZ = active.map(r => Number(r.source_before_z));
  const direction = Math.sign(sourceZ.at(-1) - sourceZ[0]);
  const duration = valid.length > 1 ? (Number(valid.at(-1).qpc_entry) - Number(valid[0].qpc_entry)) / qpcFrequency : 0;
  const predicateConsistent = valid.every(r => Number(r.local_predicate) === ((Number(r.carried) !== 0 && Number(r.mode_a) !== 2 && (Number(r.mode_a) !== 0 || Number(r.mode_b) !== 3)) ? 1 : 0));
  return {
    structural_pass: rows.length >= 2 && missing.length === 0 && valid.length === rows.length && consecutive && callerValid && pointersValid && finite && qpcMonotone && predicateConsistent,
    row_count: rows.length, valid_row_count: valid.length, missing_columns: missing,
    consecutive_sequences: consecutive, allowed_callers_only: callerValid,
    caller_counts: callers, pointer_fields_valid: pointersValid,
    finite_values: finite, qpc_monotone: qpcMonotone,
    measured_calls_per_second: duration ? (valid.length - 1) / duration : null,
    active_rows: active.length, active_planar_distance: planar,
    local_predicate_consistent: predicateConsistent,
    local_predicate_active_rows: valid.filter(r => r.local_predicate === '1').length,
    override_active_rows: valid.filter(r => Number(r.override_gate) !== 0).length,
    source_opposing_vertical_steps: opposing(sourceZ, direction, planar),
    final_opposing_vertical_steps: opposing(activeDecoded.map(v => v.final[7]), direction, planar),
    prior_cache_opposing_vertical_steps: opposing(activeDecoded.map(v => v.cache[7]), direction, planar),
    limit: 'Current-player aggregator output boundary only; no protected-operation or subsystem ownership is inferred.'
  };
}

if (process.argv[1]?.endsWith('analyze_aggregator_output.mjs')) {
  if (process.argv.length !== 4) throw new Error('usage: node analyze_aggregator_output.mjs INPUT.csv OUTPUT.json');
  const result = analyzeAggregator(parseCsv(fs.readFileSync(process.argv[2], 'utf8')));
  fs.writeFileSync(process.argv[3], `${JSON.stringify(result, null, 2)}\n`);
  if (!result.structural_pass) process.exitCode = 2;
}
