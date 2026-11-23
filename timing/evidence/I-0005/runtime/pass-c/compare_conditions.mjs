#!/usr/bin/env node
// Build deterministic cross-condition tables from per-session Pass C analyses.

import fs from "node:fs";
import path from "node:path";

const args = process.argv.slice(2);
if (args.length < 3) {
  throw new Error("Usage: node compare_conditions.mjs <output-dir> <analysis.json> <analysis.json> [...]");
}
const [outputArg, ...inputs] = args;
const output = path.resolve(outputArg);
const conditions = inputs.map((file) => JSON.parse(fs.readFileSync(path.resolve(file), "utf8")))
  .sort((a, b) => a.target_fps - b.target_fps);

function csvValue(value) {
  const text = Array.isArray(value) || (value && typeof value === "object")
    ? JSON.stringify(value)
    : String(value ?? "");
  return /[",\r\n]/.test(text) ? `"${text.replaceAll('"', '""')}"` : text;
}

function writeCsv(name, columns, rows) {
  const lines = [columns.join(","), ...rows.map((row) => columns.map((column) => csvValue(row[column])).join(","))];
  fs.writeFileSync(path.join(output, name), `${lines.join("\n")}\n`);
}

fs.mkdirSync(output, { recursive: true });
const conditionRows = conditions.map((condition) => ({
  target_fps: condition.target_fps,
  session: condition.session,
  capture_span_s: condition.capture_span_s,
  semantic_samples: condition.semantic_samples,
  update_rate_hz: condition.update_rate_hz,
  update_rate_error_percent: condition.update_rate_error_percent,
  present_rate_hz: condition.present_rate_hz,
  present_rate_error_percent: condition.present_rate_error_percent,
  delta_median_s: condition.delta_median_s,
  delta_p95_s: condition.delta_p95_s,
  delta_max_s: condition.delta_max_s,
  position_valid_fraction: condition.position_valid_fraction,
  animation_valid_fraction: condition.animation_valid_fraction,
  animation_clock_forward_pairs: condition.animation_clock_forward_pairs,
  animation_clock_delta_ratio_median: condition.animation_clock_delta_ratio_median,
  animation_clock_scaled_delta_matches: condition.animation_clock_scaled_delta_matches,
  animation_clock_scaled_delta_match_fraction: condition.animation_clock_scaled_delta_match_fraction,
  controller_connected_fraction: condition.controller_connected_fraction,
  sync_interval_0: condition.sync_interval_counts?.["0"] ?? 0,
  sync_interval_1: condition.sync_interval_counts?.["1"] ?? 0,
  capture_bytes_per_second: condition.capture_bytes_per_second,
  overhead_mean_us: condition.probe_overhead?.mean_us,
  overhead_max_us: condition.probe_overhead?.max_us,
  overhead_mean_frame_budget_percent: condition.probe_overhead?.mean_frame_budget_percent,
}));
const conditionColumns = Object.keys(conditionRows[0]);
writeCsv("condition-summary.csv", conditionColumns, conditionRows);

const markerRows = conditions.flatMap((condition) => condition.markers.map((marker) => ({
  target_fps: condition.target_fps,
  session: condition.session,
  marker_id: marker.marker_id,
  marker_name: marker.marker_name,
  samples: marker.samples,
  duration_s: marker.duration_s,
  net_displacement: marker.net_displacement,
  path_length: marker.path_length,
  mean_path_speed: marker.mean_path_speed,
  max_displacement_from_start: marker.max_displacement_from_start,
  tail_1s_displacement: marker.tail_1s_displacement,
  wrapped_angle_delta: marker.wrapped_angle_delta,
  position_valid_fraction: marker.position_valid_fraction,
  animation_valid_fraction: marker.animation_valid_fraction,
  controller_connected_fraction: marker.controller_connected_fraction,
  controller_active_fraction: marker.controller_active_fraction,
  animation_ids: marker.animation_ids,
  animation_time_min: marker.animation_time_min,
  animation_time_max: marker.animation_time_max,
  animation_max_values: marker.animation_max_values,
  play_speed_values: marker.play_speed_values,
})));
const markerColumns = Object.keys(markerRows[0]);
writeCsv("marker-comparison.csv", markerColumns, markerRows);

const comparison = {
  targets: conditions.map((condition) => condition.target_fps),
  sessions: conditions.map((condition) => condition.session),
  all_position_samples_valid: conditions.every((condition) => condition.position_valid_fraction === 1),
  all_animation_samples_valid: conditions.every((condition) => condition.animation_valid_fraction === 1),
  all_controller_samples_connected: conditions.every((condition) => condition.controller_connected_fraction === 1),
  idle_position_stable: conditions.every((condition) => {
    const marker = condition.markers.find((entry) => entry.marker_name === "idle");
    return marker?.net_displacement <= 0.01 &&
      marker?.max_displacement_from_start <= 0.01 &&
      marker?.tail_1s_displacement <= 0.01;
  }),
  camera_marker_player_transform_stable: conditions.every((condition) => {
    const marker = condition.markers.find((entry) => entry.marker_name === "camera_rotation");
    return marker?.net_displacement === 0 && marker?.wrapped_angle_delta === 0;
  }),
  obstacle_tail_plateau_max: Math.max(...conditions.map((condition) =>
    condition.markers.find((entry) => entry.marker_name === "collision_obstacle")?.tail_1s_displacement ?? Infinity)),
  note: "Human-repeated segment duration, path, displacement, and animation selection are diagnostics, not deterministic equality tests.",
};
fs.writeFileSync(path.join(output, "comparison-summary.json"), `${JSON.stringify(comparison, null, 2)}\n`);
