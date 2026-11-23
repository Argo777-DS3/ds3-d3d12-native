#!/usr/bin/env node
// Compare deterministic locomotion trials and enforce the target-60 reproducibility gate.

import fs from "node:fs";
import path from "node:path";

const [outputArg, ...inputArgs] = process.argv.slice(2);
if (!outputArg || inputArgs.length === 0) {
  throw new Error("Usage: node compare_locomotion.mjs <output> <locomotion-summary.json> [...]");
}

const mean = (values) => values.reduce((sum, value) => sum + value, 0) / values.length;
const standardDeviation = (values) => {
  const average = mean(values);
  return Math.sqrt(mean(values.map((value) => (value - average) ** 2)));
};
const coefficientOfVariation = (values) => {
  const average = mean(values);
  return average === 0 ? 0 : standardDeviation(values) / Math.abs(average);
};
const position = (trial, prefix) => [trial[`${prefix}_x`], trial[`${prefix}_z`], trial[`${prefix}_y`]];
const distance = (a, b) => Math.hypot(...a.map((value, index) => value - b[index]));
const maxPairwise = (values, metric) => values.reduce((maximum, left, index) =>
  Math.max(maximum, ...values.slice(index + 1).map((right) => metric(left, right))), 0);

const inputs = inputArgs.map((file) => JSON.parse(fs.readFileSync(path.resolve(file), "utf8")));
const conditions = inputs.map((input) => {
  const trials = input.trials.filter((trial) => trial.complete);
  const starts = trials.map((trial) => position(trial, "start"));
  const startAngles = trials.map((trial) => trial.start_angle);
  return {
    target_fps: input.target_fps,
    session: input.session,
    complete_trials: trials.length,
    all_inputs_valid: trials.every((trial) => trial.input_valid_fraction === 1),
    all_positions_valid: trials.every((trial) => trial.position_valid_fraction === 1),
    all_animations_valid: trials.every((trial) => trial.animation_valid_fraction === 1),
    animation_id_sequences_equal: trials.length > 0 && trials.every((trial) =>
      JSON.stringify(trial.animation_ids) === JSON.stringify(trials[0].animation_ids)),
    start_position_max_spread: starts.length > 1 ? maxPairwise(starts, distance) : 0,
    start_angle_max_spread: startAngles.length > 1 ? Math.max(...startAngles) - Math.min(...startAngles) : 0,
    accumulated_delta_mean: trials.length ? mean(trials.map((trial) => trial.accumulated_delta_s)) : null,
    net_distance_mean: trials.length ? mean(trials.map((trial) => trial.net_distance)) : null,
    net_distance_cv: trials.length ? coefficientOfVariation(trials.map((trial) => trial.net_distance)) : null,
    path_length_mean: trials.length ? mean(trials.map((trial) => trial.sampled_path_length)) : null,
    path_length_cv: trials.length ? coefficientOfVariation(trials.map((trial) => trial.sampled_path_length)) : null,
    distance_per_delta_mean: trials.length ? mean(trials.map((trial) => trial.net_distance_per_accumulated_delta)) : null,
    distance_per_delta_cv: trials.length ? coefficientOfVariation(trials.map((trial) => trial.net_distance_per_accumulated_delta)) : null,
    animation_progress_per_delta_mean: trials.length ? mean(trials.map((trial) => trial.animation_progress_per_accumulated_delta)) : null,
    trials,
  };
}).sort((a, b) => a.target_fps - b.target_fps);

const baseline = conditions.find((condition) => condition.target_fps === 60);
const baselineActivityPresent = Boolean(baseline &&
  baseline.net_distance_mean > 0.1 && baseline.path_length_mean > 0.1 &&
  baseline.distance_per_delta_mean > 0.01 && baseline.animation_progress_per_delta_mean > 0);
const baselineReproducible = Boolean(baseline && baseline.complete_trials >= 3 &&
  baseline.all_inputs_valid && baseline.all_positions_valid && baseline.all_animations_valid &&
  baseline.animation_id_sequences_equal && baseline.start_position_max_spread <= 0.05 &&
  baseline.start_angle_max_spread <= 0.02 && baseline.net_distance_cv <= 0.01 &&
  baseline.path_length_cv <= 0.01 && baseline.distance_per_delta_cv <= 0.01 &&
  baselineActivityPresent);

const crossCondition = baselineActivityPresent ? conditions.map((condition) => ({
  target_fps: condition.target_fps,
  distance_per_delta_relative_to_60: condition.distance_per_delta_mean / baseline.distance_per_delta_mean,
  animation_progress_per_delta_relative_to_60:
    condition.animation_progress_per_delta_mean / baseline.animation_progress_per_delta_mean,
  locomotion_divergence_over_2_percent:
    Math.abs(condition.distance_per_delta_mean / baseline.distance_per_delta_mean - 1) > 0.02,
  animation_divergence_over_1_percent:
    Math.abs(condition.animation_progress_per_delta_mean /
      baseline.animation_progress_per_delta_mean - 1) > 0.01,
})) : [];

const summary = {
  target_60_reproducibility_gate_passed: baselineReproducible,
  target_60_locomotion_activity_present: baselineActivityPresent,
  tolerances: {
    start_position_max_spread: 0.05,
    start_angle_max_spread_radians: 0.02,
    within_condition_distance_cv: 0.01,
    cross_condition_locomotion_relative_error: 0.02,
    cross_condition_animation_relative_error: 0.01,
  },
  root_motion_or_stage_vector_status: "not_instrumented",
  conditions: conditions.map(({ trials, ...condition }) => condition),
  cross_condition: crossCondition,
};

const output = path.resolve(outputArg);
fs.mkdirSync(output, { recursive: true });
fs.writeFileSync(path.join(output, "locomotion-comparison.json"), `${JSON.stringify(summary, null, 2)}\n`);
console.log(JSON.stringify({ target_60_reproducibility_gate_passed: baselineReproducible,
  targets: conditions.map((condition) => condition.target_fps) }, null, 2));
