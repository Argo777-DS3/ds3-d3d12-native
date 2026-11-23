#!/usr/bin/env node
// Derive bounded Pass C discovery metrics without modifying raw captures.

import fs from "node:fs";
import path from "node:path";
import crypto from "node:crypto";

const [sessionArg, outputArg] = process.argv.slice(2);
if (!sessionArg || !outputArg) {
  throw new Error("Usage: node analyze_session.mjs <session> <output>");
}
const session = path.resolve(sessionArg);
const output = path.resolve(outputArg);

function records(file) {
  const lines = fs.readFileSync(file, "utf8").trim().split(/\r?\n/);
  const headers = lines.shift().split(",");
  return lines.filter(Boolean).map((line) => Object.fromEntries(
    line.split(",").map((value, index) => [headers[index], value]),
  ));
}

function percentile(values, fraction) {
  if (!values.length) return null;
  const ordered = [...values].sort((a, b) => a - b);
  return ordered[Math.min(ordered.length - 1, Math.max(0, Math.ceil(fraction * ordered.length) - 1))];
}

function median(values) {
  const ordered = [...values].sort((a, b) => a - b);
  const middle = Math.floor(ordered.length / 2);
  return ordered.length % 2 ? ordered[middle] : (ordered[middle - 1] + ordered[middle]) / 2;
}

function position(row) {
  return [Number(row.x), Number(row.z), Number(row.y)];
}

function distance(a, b) {
  return Math.hypot(...a.map((value, index) => value - b[index]));
}

function wrappedAngleDelta(start, end) {
  const twoPi = 2 * Math.PI;
  return ((end - start + Math.PI) % twoPi + twoPi) % twoPi - Math.PI;
}

function timedWindow(file, qpcFrequency) {
  const durations = records(file).map((row) => (Number(row.qpc_after) - Number(row.qpc_before)) / qpcFrequency);
  return {
    samples: durations.length,
    median_us: median(durations) * 1e6,
    p95_us: percentile(durations, 0.95) * 1e6,
    max_us: Math.max(...durations) * 1e6,
  };
}

const info = Object.fromEntries(fs.readFileSync(path.join(session, "session-info.txt"), "utf8")
  .trim().split(/\r?\n/).map((line) => line.split(/=(.*)/s).slice(0, 2)));
const qpcFrequency = Number(info.qpc_frequency);
const targetFps = Number(info.target_fps);
const semantic = records(path.join(session, "semantic-state.csv"));
if (semantic.length < 2) throw new Error("semantic-state.csv has fewer than two samples");
const first = semantic[0];
const last = semantic.at(-1);
const span = (Number(last.qpc) - Number(first.qpc)) / qpcFrequency;
const deltas = semantic.map((row) => Number(row.delta));
const updateRate = (Number(last.update_index) - Number(first.update_index)) / span;
let animationClockForwardPairs = 0;
let animationClockScaledDeltaMatches = 0;
const animationClockDeltaRatios = [];
for (let index = 1; index < semantic.length; index += 1) {
  const previous = semantic[index - 1];
  const current = semantic[index];
  if (current.animation_valid !== "1" || previous.animation_valid !== "1") continue;
  if (current.animation_id !== previous.animation_id) continue;
  const clockDelta = Number(current.animation_time) - Number(previous.animation_time);
  if (clockDelta <= 0) continue;
  animationClockForwardPairs += 1;
  const ratio = clockDelta / Number(current.delta);
  animationClockDeltaRatios.push(ratio);
  if (Math.abs(ratio - 10.0) <= 0.01) animationClockScaledDeltaMatches += 1;
}

const grouped = new Map();
for (const row of semantic) {
  const marker = Number(row.marker_id);
  if (!marker) continue;
  if (!grouped.has(marker)) grouped.set(marker, []);
  grouped.get(marker).push(row);
}
const markers = [...grouped.entries()].sort(([a], [b]) => a - b).map(([marker, samples]) => {
  const start = samples[0];
  const end = samples.at(-1);
  const tailStart = Number(end.qpc) - qpcFrequency;
  const tail = samples.filter((row) => Number(row.qpc) >= tailStart);
  let pathLength = 0;
  let maxDisplacementFromStart = 0;
  for (let index = 1; index < samples.length; index += 1) {
    pathLength += distance(position(samples[index - 1]), position(samples[index]));
    maxDisplacementFromStart = Math.max(
      maxDisplacementFromStart,
      distance(position(start), position(samples[index])),
    );
  }
  const controllerActive = samples.filter((row) => {
    // XInput thumb axes are signed 16-bit integers. Use a conservative analysis
    // deadzone rather than treating them as normalized floats.
    const analog = ["left_x", "left_y", "right_x", "right_y"].some((key) => Math.abs(Number(row[key])) > 4096);
    const trigger = ["left_trigger", "right_trigger"].some((key) => Number(row[key]) > 0);
    return Number.parseInt(row.controller_buttons, 16) !== 0 || analog || trigger;
  }).length;
  return {
    marker_id: marker,
    marker_name: start.marker_name,
    samples: samples.length,
    duration_s: (Number(end.qpc) - Number(start.qpc)) / qpcFrequency,
    position_valid_fraction: samples.filter((row) => row.position_valid === "1").length / samples.length,
    animation_valid_fraction: samples.filter((row) => row.animation_valid === "1").length / samples.length,
    net_displacement: distance(position(start), position(end)),
    path_length: pathLength,
    mean_path_speed: pathLength / ((Number(end.qpc) - Number(start.qpc)) / qpcFrequency),
    max_displacement_from_start: maxDisplacementFromStart,
    tail_1s_displacement: tail.length > 1 ? distance(position(tail[0]), position(tail.at(-1))) : null,
    wrapped_angle_delta: wrappedAngleDelta(Number(start.angle), Number(end.angle)),
    animation_ids: [...new Set(samples.map((row) => Number(row.animation_id)))].sort((a, b) => a - b),
    animation_time_min: Math.min(...samples.map((row) => Number(row.animation_time))),
    animation_time_max: Math.max(...samples.map((row) => Number(row.animation_time))),
    animation_max_values: [...new Set(samples.map((row) => Number(row.animation_max)))].sort((a, b) => a - b),
    play_speed_values: [...new Set(samples.map((row) => Number(row.play_speed)))].sort((a, b) => a - b),
    controller_connected_fraction: samples.filter((row) => row.controller_connected === "1").length / samples.length,
    controller_active_fraction: controllerActive / samples.length,
  };
});

const presents = records(path.join(session, "present.csv"));
const presentSpan = (Number(presents.at(-1).qpc_before) - Number(presents[0].qpc_before)) / qpcFrequency;
const presentRate = (presents.length - 1) / presentSpan;
const syncIntervals = {};
for (const row of presents) syncIntervals[row.sync_interval] = (syncIntervals[row.sync_interval] ?? 0) + 1;
const totalBytes = fs.readdirSync(session, { withFileTypes: true })
  .filter((entry) => entry.isFile())
  .reduce((sum, entry) => sum + fs.statSync(path.join(session, entry.name)).size, 0);
const overheadPath = path.join(session, "probe-overhead.csv");
const directOverhead = fs.existsSync(overheadPath) ? records(overheadPath)[0] : null;
const rawHashes = Object.fromEntries(fs.readdirSync(session, { withFileTypes: true })
  .filter((entry) => entry.isFile())
  .sort((a, b) => a.name.localeCompare(b.name))
  .map((entry) => {
    const bytes = fs.readFileSync(path.join(session, entry.name));
    return [entry.name, crypto.createHash("sha256").update(bytes).digest("hex").toUpperCase()];
  }));

const summary = {
  session: path.basename(session),
  condition: info.condition,
  target_fps: targetFps,
  capture_span_s: span,
  semantic_samples: semantic.length,
  update_rate_hz: updateRate,
  update_rate_error_percent: (updateRate - targetFps) / targetFps * 100,
  delta_median_s: median(deltas),
  delta_p95_s: percentile(deltas, 0.95),
  delta_max_s: Math.max(...deltas),
  position_valid_fraction: semantic.filter((row) => row.position_valid === "1").length / semantic.length,
  animation_valid_fraction: semantic.filter((row) => row.animation_valid === "1").length / semantic.length,
  animation_clock_forward_pairs: animationClockForwardPairs,
  animation_clock_delta_ratio_median: animationClockDeltaRatios.length
    ? median(animationClockDeltaRatios)
    : null,
  animation_clock_scaled_delta_matches: animationClockScaledDeltaMatches,
  animation_clock_scaled_delta_match_fraction: animationClockForwardPairs
    ? animationClockScaledDeltaMatches / animationClockForwardPairs
    : null,
  world_pointer_values: [...new Set(semantic.map((row) => row.world))].sort(),
  player_pointer_values: [...new Set(semantic.map((row) => row.player))].sort(),
  transform_pointer_values: [...new Set(semantic.map((row) => row.transform))].sort(),
  controller_connected_fraction: semantic.filter((row) => row.controller_connected === "1").length / semantic.length,
  present_samples: presents.length,
  present_rate_hz: presentRate,
  present_rate_error_percent: (presentRate - targetFps) / targetFps * 100,
  sync_interval_counts: syncIntervals,
  total_capture_bytes: totalBytes,
  capture_bytes_per_second: totalBytes / span,
  timed_function_windows: {
    selector: timedWindow(path.join(session, "selector.csv"), qpcFrequency),
    presentation_pacing: timedWindow(path.join(session, "presentation-pacing.csv"), qpcFrequency),
    present_including_dxgi_call: timedWindow(path.join(session, "present.csv"), qpcFrequency),
  },
  probe_overhead: {
    direct_semantic_sampler_timing_available: directOverhead !== null,
    samples: directOverhead ? Number(directOverhead.samples) : null,
    mean_us: directOverhead ? Number(directOverhead.mean_us) : null,
    max_us: directOverhead ? Number(directOverhead.max_us) : null,
    mean_frame_budget_percent: directOverhead
      ? Number(directOverhead.mean_us) / (1e6 / targetFps) * 100
      : null,
    note: directOverhead
      ? "Direct QPC window includes player/input reads, synchronization, and semantic CSV write."
      : "Achieved cadence and output bandwidth are measurable; sampler CPU cost is not isolated by schema v1.",
  },
  raw_sha256: rawHashes,
  markers,
};

fs.mkdirSync(output, { recursive: true });
fs.writeFileSync(path.join(output, "analysis-summary.json"), `${JSON.stringify(summary, null, 2)}\n`);
const markerColumns = [
  "marker_id", "marker_name", "samples", "duration_s", "position_valid_fraction",
  "animation_valid_fraction", "net_displacement", "path_length", "mean_path_speed",
  "max_displacement_from_start", "tail_1s_displacement", "wrapped_angle_delta",
  "controller_connected_fraction", "controller_active_fraction", "animation_ids", "animation_time_min",
  "animation_time_max", "animation_max_values", "play_speed_values",
];
const csvValue = (value) => {
  const text = Array.isArray(value) ? JSON.stringify(value) : String(value ?? "");
  return /[",\r\n]/.test(text) ? `"${text.replaceAll('"', '""')}"` : text;
};
const markerCsv = [markerColumns.join(","), ...markers.map((row) => markerColumns.map((key) => csvValue(row[key])).join(","))].join("\n");
fs.writeFileSync(path.join(output, "marker-summary.csv"), `${markerCsv}\n`);
