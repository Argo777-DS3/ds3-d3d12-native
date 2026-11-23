#!/usr/bin/env node
// Derive deterministic locomotion metrics without modifying raw captures.

import crypto from "node:crypto";
import fs from "node:fs";
import path from "node:path";

const [sessionArg, outputArg] = process.argv.slice(2);
if (!sessionArg || !outputArg) {
  throw new Error("Usage: node analyze_locomotion.mjs <session> <output>");
}
const session = path.resolve(sessionArg);
const output = path.resolve(outputArg);

function records(file) {
  const text = fs.readFileSync(file, "utf8").trim();
  if (!text) return [];
  const lines = text.split(/\r?\n/);
  const headers = lines.shift().split(",");
  return lines.filter(Boolean).map((line) => Object.fromEntries(
    line.split(",").map((value, index) => [headers[index], value]),
  ));
}

function position(row, prefix) {
  return [Number(row[`${prefix}_x`]), Number(row[`${prefix}_z`]), Number(row[`${prefix}_y`])];
}

function distance(a, b) {
  return Math.hypot(...a.map((value, index) => value - b[index]));
}

function horizontalDistance(a, b) {
  return Math.hypot(a[0] - b[0], a[1] - b[1]);
}

function wrappedAngleDelta(start, end) {
  const twoPi = 2 * Math.PI;
  return ((end - start + Math.PI) % twoPi + twoPi) % twoPi - Math.PI;
}

function csvValue(value) {
  const text = Array.isArray(value) ? JSON.stringify(value) : String(value ?? "");
  return /[",\r\n]/.test(text) ? `"${text.replaceAll('"', '""')}"` : text;
}

function sha256(file) {
  return crypto.createHash("sha256").update(fs.readFileSync(file)).digest("hex").toUpperCase();
}

const info = Object.fromEntries(fs.readFileSync(path.join(session, "session-info.txt"), "utf8")
  .trim().split(/\r?\n/).map((line) => line.split(/=(.*)/s).slice(0, 2)));
const qpcFrequency = Number(info.qpc_frequency);
const updatesPath = path.join(session, "locomotion-updates.csv");
const eventsPath = path.join(session, "locomotion-events.csv");
const phasesPath = path.join(session, "locomotion-phases.csv");
const updates = records(updatesPath).filter((row) => row.phase === "measure");
const events = records(eventsPath);
const phaseRows = fs.existsSync(phasesPath) ? records(phasesPath) : [];
const trialIds = [...new Set(updates.map((row) => Number(row.trial)))].sort((a, b) => a - b);

const pendingPhases = new Map();
const phasePairs = [];
for (const row of phaseRows.sort((a, b) => Number(a.seq) - Number(b.seq))) {
  const key = `${row.trial}:${row.phase}:${row.call_index}`;
  if (row.event === "before") {
    pendingPhases.set(key, row);
  } else if (row.event === "after" && pendingPhases.has(key)) {
    const before = pendingPhases.get(key);
    pendingPhases.delete(key);
    const beforePosition = [Number(before.x), Number(before.z), Number(before.y)];
    const afterPosition = [Number(row.x), Number(row.z), Number(row.y)];
    phasePairs.push({
      trial: Number(row.trial),
      phase: row.phase,
      call_index: Number(row.call_index),
      caller_rva: before.caller_rva ?? "",
      broad_update_index_before: Number(before.broad_update_index),
      broad_update_index_after: Number(row.broad_update_index),
      qpc_before: Number(before.qpc),
      qpc_after: Number(row.qpc),
      position_valid: before.position_valid === "1" && row.position_valid === "1",
      displacement: distance(beforePosition, afterPosition),
      dx: afterPosition[0] - beforePosition[0],
      dz: afterPosition[1] - beforePosition[1],
      dy: afterPosition[2] - beforePosition[2],
    });
  }
}

const phaseSummaries = [...new Set(phasePairs.map((pair) => pair.phase))].sort().map((phase) => {
  const pairs = phasePairs.filter((pair) => pair.phase === phase && pair.position_valid);
  const nonzero = pairs.filter((pair) => pair.displacement > 1e-7);
  const total = pairs.reduce((sum, pair) => sum + pair.displacement, 0);
  return {
    phase,
    pair_count: pairs.length,
    nonzero_pair_count: nonzero.length,
    nonzero_fraction: pairs.length ? nonzero.length / pairs.length : 0,
    total_displacement: total,
    mean_displacement: pairs.length ? total / pairs.length : 0,
    max_displacement: pairs.length ? Math.max(...pairs.map((pair) => pair.displacement)) : 0,
  };
});

const phaseTransitionGroups = new Map();
for (let index = 1; index < phaseRows.length; index += 1) {
  const before = phaseRows[index - 1];
  const after = phaseRows[index];
  if (before.trial !== after.trial || before.tid !== after.tid ||
      before.position_valid !== "1" || after.position_valid !== "1") continue;
  const key = `${before.phase}:${before.event}->${after.phase}:${after.event}`;
  const displacement = distance(
    [Number(before.x), Number(before.z), Number(before.y)],
    [Number(after.x), Number(after.z), Number(after.y)],
  );
  const group = phaseTransitionGroups.get(key) ?? {
    transition: key,
    from_caller_rvas: new Set(),
    to_caller_rvas: new Set(),
    sample_count: 0,
    nonzero_count: 0,
    total_displacement: 0,
    max_displacement: 0,
  };
  group.sample_count += 1;
  if (displacement > 1e-7) group.nonzero_count += 1;
  group.total_displacement += displacement;
  group.max_displacement = Math.max(group.max_displacement, displacement);
  if (before.caller_rva) group.from_caller_rvas.add(before.caller_rva);
  if (after.caller_rva) group.to_caller_rvas.add(after.caller_rva);
  phaseTransitionGroups.set(key, group);
}
const phaseTransitionSummaries = [...phaseTransitionGroups.values()].map((group) => ({
  transition: group.transition,
  from_caller_rvas: [...group.from_caller_rvas].sort(),
  to_caller_rvas: [...group.to_caller_rvas].sort(),
  sample_count: group.sample_count,
  nonzero_count: group.nonzero_count,
  nonzero_fraction: group.sample_count ? group.nonzero_count / group.sample_count : 0,
  total_displacement: group.total_displacement,
  mean_displacement: group.sample_count ? group.total_displacement / group.sample_count : 0,
  max_displacement: group.max_displacement,
})).sort((left, right) => left.transition.localeCompare(right.transition));

const trials = trialIds.map((trial) => {
  const rows = updates.filter((row) => Number(row.trial) === trial)
    .sort((a, b) => Number(a.update_index) - Number(b.update_index));
  const first = rows[0];
  const last = rows.at(-1);
  const start = position(first, "before");
  const end = position(last, "after");
  let withinUpdatePathLength = 0;
  let sampledPathLength = 0;
  let previous = start;
  let animationForwardProgress = 0;
  let animationResetsOrTransitions = 0;
  const animationIds = new Set();
  let previousAnimation = null;
  let inputValid = 0;
  const accumulateAnimation = (current) => {
    animationIds.add(current.id);
    if (previousAnimation !== null) {
      if (previousAnimation.id !== current.id) {
        animationResetsOrTransitions += 1;
      } else {
        const direct = current.time - previousAnimation.time;
        if (direct >= 0) {
          animationForwardProgress += direct;
        } else if (previousAnimation.max > 0 && previousAnimation.time <= previousAnimation.max &&
                   current.time >= 0 && current.time <= previousAnimation.max) {
          animationForwardProgress += previousAnimation.max - previousAnimation.time + current.time;
          animationResetsOrTransitions += 1;
        } else {
          animationResetsOrTransitions += 1;
        }
      }
    }
    previousAnimation = current;
  };
  for (const row of rows) {
    const before = position(row, "before");
    const after = position(row, "after");
    withinUpdatePathLength += distance(before, after);
    sampledPathLength += distance(previous, after);
    previous = after;
    const beforeId = Number(row.before_animation_id);
    const afterId = Number(row.after_animation_id);
    accumulateAnimation({ id: beforeId, time: Number(row.before_animation_time),
      max: Number(row.before_animation_max) });
    accumulateAnimation({ id: afterId, time: Number(row.after_animation_time),
      max: Number(row.after_animation_max) });
    const beforeMask = Number.parseInt(row.before_keyboard_mask, 16);
    const afterMask = Number.parseInt(row.after_keyboard_mask, 16);
    const controllersNeutral = Number.parseInt(row.before_controller_buttons, 16) === 0 &&
      Number.parseInt(row.after_controller_buttons, 16) === 0 &&
      ["before_left_x", "before_left_y", "before_right_x", "before_right_y",
        "after_left_x", "after_left_y", "after_right_x", "after_right_y"]
        .every((column) => Math.abs(Number(row[column])) <= 4096);
    if (beforeMask === 0x21 && afterMask === 0x21 && controllersNeutral) inputValid += 1;
  }
  const wallDuration = (Number(last.qpc_after) - Number(first.qpc_before)) / qpcFrequency;
  const accumulatedDelta = Number(last.accumulated_delta_after) - Number(first.accumulated_delta_before);
  const sumDelta = rows.reduce((sum, row) => sum + Number(row.delta), 0);
  const eventNames = events.filter((event) => Number(event.trial) === trial).map((event) => event.event);
  const netDistance = distance(start, end);
  const netHorizontalDistance = horizontalDistance(start, end);
  return {
    trial,
    complete: eventNames.includes("measure_start") && eventNames.includes("measure_end"),
    updates: rows.length,
    first_update_index: Number(first.update_index),
    last_update_index: Number(last.update_index),
    qpc_start: Number(first.qpc_before),
    qpc_end: Number(last.qpc_after),
    wall_duration_s: wallDuration,
    sum_delta_s: sumDelta,
    accumulated_delta_s: accumulatedDelta,
    start_x: start[0], start_z: start[1], start_y: start[2],
    end_x: end[0], end_z: end[1], end_y: end[2],
    start_angle: Number(first.before_angle),
    end_angle: Number(last.after_angle),
    wrapped_angle_delta: wrappedAngleDelta(Number(first.before_angle), Number(last.after_angle)),
    net_distance: netDistance,
    net_horizontal_distance: netHorizontalDistance,
    sampled_path_length: sampledPathLength,
    within_update_path_length: withinUpdatePathLength,
    net_distance_per_wall_second: netDistance / wallDuration,
    net_distance_per_accumulated_delta: netDistance / accumulatedDelta,
    path_length_per_accumulated_delta: sampledPathLength / accumulatedDelta,
    position_valid_fraction: rows.filter((row) => row.before_position_valid === "1" && row.after_position_valid === "1").length / rows.length,
    animation_valid_fraction: rows.filter((row) => row.before_animation_valid === "1" && row.after_animation_valid === "1").length / rows.length,
    input_valid_fraction: inputValid / rows.length,
    start_animation_id: Number(first.before_animation_id),
    start_animation_time: Number(first.before_animation_time),
    end_animation_id: Number(last.after_animation_id),
    end_animation_time: Number(last.after_animation_time),
    animation_ids: [...animationIds],
    animation_forward_progress: animationForwardProgress,
    animation_progress_per_accumulated_delta: animationForwardProgress / accumulatedDelta,
    animation_resets_or_transitions: animationResetsOrTransitions,
    event_names: eventNames,
  };
});

const summary = {
  session: path.basename(session),
  condition: info.condition,
  target_fps: Number(info.target_fps),
  locomotion_warmup_delta: Number(info.locomotion_warmup_delta),
  locomotion_measure_delta: Number(info.locomotion_measure_delta),
  trial_count: trials.length,
  complete_trial_count: trials.filter((trial) => trial.complete).length,
  root_motion_or_stage_vector_status: "not_instrumented",
  phase_boundary_status: phaseRows.length ? "instrumented" : "not_instrumented",
  scope_note: "Phase and caller-return boundaries localize final-transform changes across the observed update/dispatcher path; root/requested motion and pre/post collision vectors remain uninstrumented.",
  raw_sha256: {
    "locomotion-updates.csv": sha256(updatesPath),
    "locomotion-events.csv": sha256(eventsPath),
    "session-info.txt": sha256(path.join(session, "session-info.txt")),
    ...(fs.existsSync(phasesPath) ? { "locomotion-phases.csv": sha256(phasesPath) } : {}),
  },
  phase_summaries: phaseSummaries,
  phase_transition_summaries: phaseTransitionSummaries,
  phase_pairs: phasePairs,
  trials,
};

fs.mkdirSync(output, { recursive: true });
fs.writeFileSync(path.join(output, "locomotion-summary.json"), `${JSON.stringify(summary, null, 2)}\n`);
if (trials.length) {
  const columns = Object.keys(trials[0]);
  const lines = [columns.join(","), ...trials.map((trial) =>
    columns.map((column) => csvValue(trial[column])).join(","))];
  fs.writeFileSync(path.join(output, "locomotion-trials.csv"), `${lines.join("\n")}\n`);
}
if (phasePairs.length) {
  const columns = Object.keys(phasePairs[0]);
  const lines = [columns.join(","), ...phasePairs.map((pair) =>
    columns.map((column) => csvValue(pair[column])).join(","))];
  fs.writeFileSync(path.join(output, "locomotion-phase-pairs.csv"), `${lines.join("\n")}\n`);
}
console.log(JSON.stringify({ session: summary.session, trials: summary.trial_count,
  complete: summary.complete_trial_count }, null, 2));
