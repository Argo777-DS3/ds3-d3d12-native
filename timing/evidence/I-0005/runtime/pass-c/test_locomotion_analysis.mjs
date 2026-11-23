#!/usr/bin/env node

import assert from "node:assert/strict";
import fs from "node:fs";
import os from "node:os";
import path from "node:path";
import { spawnSync } from "node:child_process";
import { fileURLToPath } from "node:url";

const here = path.dirname(fileURLToPath(import.meta.url));
const temporary = fs.mkdtempSync(path.join(os.tmpdir(), "i0005-locomotion-test-"));
const session = path.join(temporary, "synthetic-session");
const output = path.join(temporary, "analysis");
const comparison = path.join(temporary, "comparison");

try {
  fs.mkdirSync(session, { recursive: true });
  fs.writeFileSync(path.join(session, "session-info.txt"), [
    "qpc_frequency=1000",
    "condition=synthetic-stock-60",
    "target_fps=60",
    "locomotion_warmup_delta=1",
    "locomotion_measure_delta=3",
  ].join("\n") + "\n");

  const updateColumns = [
    "seq", "qpc_before", "qpc_after", "tid", "trial", "phase", "update_index", "delta",
    "accumulated_delta_before", "accumulated_delta_after",
    "before_position_valid", "before_x", "before_z", "before_y", "before_angle",
    "after_position_valid", "after_x", "after_z", "after_y", "after_angle",
    "before_animation_valid", "before_animation_id", "before_animation_time", "before_animation_max", "before_play_speed",
    "after_animation_valid", "after_animation_id", "after_animation_time", "after_animation_max", "after_play_speed",
    "before_keyboard_mask", "before_controller_connected", "before_controller_buttons",
    "before_left_x", "before_left_y", "before_right_x", "before_right_y", "before_left_trigger", "before_right_trigger",
    "after_keyboard_mask", "after_controller_connected", "after_controller_buttons",
    "after_left_x", "after_left_y", "after_right_x", "after_right_y", "after_left_trigger", "after_right_trigger",
  ];
  const updateRows = Array.from({ length: 3 }, (_, index) => {
    const row = Object.fromEntries(updateColumns.map((column) => [column, 0]));
    Object.assign(row, {
      seq: index + 1, qpc_before: index * 1000, qpc_after: (index + 1) * 1000,
      tid: 1, trial: 1, phase: "measure", update_index: index + 1, delta: 1,
      accumulated_delta_before: index, accumulated_delta_after: index + 1,
      before_position_valid: 1, before_x: index, after_position_valid: 1, after_x: index + 1,
      before_animation_valid: 1, before_animation_id: 100, before_animation_time: index,
      before_animation_max: 10, before_play_speed: 1,
      after_animation_valid: 1, after_animation_id: 100, after_animation_time: index + 1,
      after_animation_max: 10, after_play_speed: 1,
      before_keyboard_mask: "0x21", after_keyboard_mask: "0x21",
      before_controller_connected: 1, after_controller_connected: 1,
      before_controller_buttons: "0x0", after_controller_buttons: "0x0",
    });
    return updateColumns.map((column) => row[column]).join(",");
  });
  fs.writeFileSync(path.join(session, "locomotion-updates.csv"),
    `${updateColumns.join(",")}\n${updateRows.join("\n")}\n`);

  const eventColumns = ["seq", "qpc", "event", "trial", "update_index", "accumulated_delta",
    "position_valid", "x", "z", "y", "angle", "animation_valid", "animation_id",
    "animation_time", "animation_max", "play_speed", "keyboard_mask", "controller_connected",
    "controller_buttons", "left_x", "left_y", "right_x", "right_y", "left_trigger", "right_trigger"];
  const event = (seq, qpc, name, x) => [seq, qpc, name, 1, seq, seq - 1, 1, x, 0, 0, 0,
    1, 100, x, 10, 1, "0x21", 1, "0x0", 0, 0, 0, 0, 0, 0].join(",");
  fs.writeFileSync(path.join(session, "locomotion-events.csv"),
    `${eventColumns.join(",")}\n${event(1, 0, "measure_start", 0)}\n${event(4, 3000, "measure_end", 3)}\n`);

  const phaseColumns = ["seq", "qpc", "tid", "trial", "broad_update_index", "phase",
    "event", "call_index", "self", "context", "position_valid", "x", "z", "y",
    "angle", "animation_valid", "animation_id", "animation_time", "animation_max", "play_speed"];
  const phase = (seq, qpc, eventName, x) => [seq, qpc, 1, 1, 1, "D5B140", eventName,
    0, "0x1", "0x2", 1, x, 0, 0, 0, 1, 100, x, 10, 1].join(",");
  fs.writeFileSync(path.join(session, "locomotion-phases.csv"),
    `${phaseColumns.join(",")}\n${phase(10, 100, "before", 0)}\n${phase(11, 200, "after", 1)}\n`);

  const analyzed = spawnSync(process.execPath, [path.join(here, "analyze_locomotion.mjs"), session, output],
    { encoding: "utf8" });
  assert.equal(analyzed.status, 0, analyzed.stderr);
  const summary = JSON.parse(fs.readFileSync(path.join(output, "locomotion-summary.json"), "utf8"));
  assert.equal(summary.complete_trial_count, 1);
  assert.equal(summary.trials[0].updates, 3);
  assert.equal(summary.trials[0].wall_duration_s, 3);
  assert.equal(summary.trials[0].accumulated_delta_s, 3);
  assert.equal(summary.trials[0].net_distance, 3);
  assert.equal(summary.trials[0].sampled_path_length, 3);
  assert.equal(summary.trials[0].input_valid_fraction, 1);
  assert.equal(summary.trials[0].animation_forward_progress, 3);
  assert.equal(summary.phase_boundary_status, "instrumented");
  assert.equal(summary.phase_summaries[0].phase, "D5B140");
  assert.equal(summary.phase_summaries[0].nonzero_pair_count, 1);
  assert.equal(summary.phase_pairs[0].displacement, 1);

  summary.trials = [1, 2, 3].map((trial) => ({ ...summary.trials[0], trial }));
  summary.trial_count = 3;
  summary.complete_trial_count = 3;
  fs.writeFileSync(path.join(output, "locomotion-summary.json"), `${JSON.stringify(summary, null, 2)}\n`);
  const compared = spawnSync(process.execPath, [path.join(here, "compare_locomotion.mjs"), comparison,
    path.join(output, "locomotion-summary.json")], { encoding: "utf8" });
  assert.equal(compared.status, 0, compared.stderr);
  const comparisonSummary = JSON.parse(fs.readFileSync(path.join(comparison, "locomotion-comparison.json"), "utf8"));
  assert.equal(comparisonSummary.target_60_reproducibility_gate_passed, true);
  assert.equal(comparisonSummary.target_60_locomotion_activity_present, true);

  summary.trials = summary.trials.map((trial) => ({ ...trial,
    end_x: trial.start_x, net_distance: 0, sampled_path_length: 0,
    net_distance_per_accumulated_delta: 0, path_length_per_accumulated_delta: 0,
    animation_forward_progress: 0, animation_progress_per_accumulated_delta: 0,
  }));
  fs.writeFileSync(path.join(output, "locomotion-summary.json"), `${JSON.stringify(summary, null, 2)}\n`);
  const zeroComparison = path.join(temporary, "comparison-zero-motion");
  const comparedZero = spawnSync(process.execPath, [path.join(here, "compare_locomotion.mjs"), zeroComparison,
    path.join(output, "locomotion-summary.json")], { encoding: "utf8" });
  assert.equal(comparedZero.status, 0, comparedZero.stderr);
  const zeroSummary = JSON.parse(fs.readFileSync(path.join(zeroComparison, "locomotion-comparison.json"), "utf8"));
  assert.equal(zeroSummary.target_60_locomotion_activity_present, false);
  assert.equal(zeroSummary.target_60_reproducibility_gate_passed, false);
  console.log("PASS: deterministic locomotion analyzer and reproducibility comparator synthetic fixture.");
} finally {
  const resolved = path.resolve(temporary);
  const tempRoot = path.resolve(os.tmpdir());
  if (!resolved.startsWith(`${tempRoot}${path.sep}`)) throw new Error("Unsafe temporary cleanup target");
  fs.rmSync(resolved, { recursive: true, force: true });
}
