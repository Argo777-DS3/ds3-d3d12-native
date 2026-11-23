#!/usr/bin/env node

import fs from "node:fs";
import path from "node:path";
import { spawnSync } from "node:child_process";
import { fileURLToPath } from "node:url";

const here = path.dirname(fileURLToPath(import.meta.url));
const root = path.join(here, "obj", "comparison-self-test");
fs.rmSync(root, { recursive: true, force: true });
fs.mkdirSync(root, { recursive: true });

function bits(value) {
  const buffer = new ArrayBuffer(4);
  const view = new DataView(buffer);
  view.setFloat32(0, value, true);
  return `0x${view.getUint32(0, true).toString(16).padStart(8, "0").toUpperCase()}`;
}

function writeCondition(name, target, candidate, scalar, applied) {
  const session = path.join(root, name);
  fs.mkdirSync(session, { recursive: true });
  fs.writeFileSync(path.join(session, "session-info.txt"),
    `target_fps=${target}\ncondition=${name}\n`);
  fs.writeFileSync(path.join(session, "locomotion-writer-mirror.csv"), [
    "seq,rbx_matches_transform,outer_a8_read_valid,outer_b0_read_valid,inhibit_valid,source,inhibit,final_xmm6_b0,final_xmm6_b1,final_xmm6_b2,scalar_memory_bits,delta",
    `1,1,1,1,1,B0,0,${bits(applied)},${bits(0)},${bits(0)},${bits(scalar)},0.1`,
  ].join("\n") + "\n");
  fs.writeFileSync(path.join(session, "locomotion-updates.csv"), [
    "phase,trial,update_index,delta,before_x,before_z,before_y,after_x,after_z,after_y",
    "measure,1,1,0.1,0,0,0,0,0,0",
    `measure,1,2,0.1,${applied},0,0,${applied},0,0`,
  ].join("\n") + "\n");
  fs.writeFileSync(path.join(session, "locomotion-events.csv"),
    "event,trial\nmeasure_start,1\nmeasure_end,1\n");
  const reconstruction = path.join(root, `${name}-reconstruction.csv`);
  fs.writeFileSync(reconstruction, [
    "seq,trial,broad_update_index,precommit_valid,will_commit,d_b0,d_b1,d_b2,a_b0,a_b1,a_b2",
    `1,1,1,1,1,${bits(candidate)},${bits(0)},${bits(0)},${bits(applied)},${bits(0)},${bits(0)}`,
  ].join("\n") + "\n");
  return { session, reconstruction };
}

function runCase(name, condition165, expected) {
  const baseline = writeCondition(`${name}-60`, 60, 4, 0.5, 2);
  const comparison = path.join(root, `${name}-comparison`);
  const result = spawnSync(process.execPath, [path.join(here, "compare_writer_mirror.mjs"),
    baseline.session, baseline.reconstruction,
    condition165.session, condition165.reconstruction, comparison], { encoding: "utf8" });
  if (result.status !== 0) throw new Error(`${name} failed:\n${result.stdout}\n${result.stderr}`);
  const summary = JSON.parse(fs.readFileSync(
    path.join(comparison, "writer-mirror-comparison.json"), "utf8"));
  if (!summary.validity_passed || summary.ordered_conclusion !== expected) {
    throw new Error(`${name} expected ${expected}, got ${summary.ordered_conclusion}`);
  }
}

runCase("candidate", writeCondition("candidate-165", 165, 2, 0.5, 1),
  "candidate_offset_first_numeric_explanation");
runCase("scalar", writeCondition("scalar-165", 165, 4, 0.25, 1),
  "scalar_first_numeric_explanation");

console.log("PASS: ordered comparison distinguishes candidate-first and scalar-first synthetic cases.");
