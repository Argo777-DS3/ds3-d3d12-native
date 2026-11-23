#!/usr/bin/env node
// Validate the bounded final-transform writer discovery without modifying raw capture.

import crypto from "node:crypto";
import fs from "node:fs";
import path from "node:path";

const [sessionArg, outputArg] = process.argv.slice(2);
if (!sessionArg || !outputArg) {
  throw new Error("Usage: node analyze_write_watch.mjs <session> <output>");
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

function info(file) {
  return Object.fromEntries(fs.readFileSync(file, "utf8").trim().split(/\r?\n/)
    .filter(Boolean).map((line) => {
      const separator = line.indexOf("=");
      return [line.slice(0, separator), line.slice(separator + 1)];
    }));
}

function sha256(file) {
  return crypto.createHash("sha256").update(fs.readFileSync(file)).digest("hex").toUpperCase();
}

function hex(value) {
  return BigInt(value);
}

function hexRva(address, imageBase) {
  return `0x${(address - imageBase).toString(16).toUpperCase()}`;
}

function float32(bits) {
  const buffer = Buffer.allocUnsafe(4);
  buffer.writeUInt32LE(Number(bits & 0xFFFFFFFFn));
  return buffer.readFloatLE(0);
}

function xmmPosition(row) {
  const low = hex(row.xmm6_lo);
  const high = hex(row.xmm6_hi);
  return [
    float32(low),
    float32(low >> 32n),
    float32(high),
    float32(high >> 32n),
  ];
}

function position(row, prefix = "") {
  return [Number(row[`${prefix}x`]), Number(row[`${prefix}z`]), Number(row[`${prefix}y`])];
}

function distance(left, right) {
  return Math.hypot(...left.map((value, index) => value - right[index]));
}

function maximum(values) {
  return values.length ? Math.max(...values) : null;
}

const sessionInfo = info(path.join(session, "session-info.txt"));
const imageBase = hex(sessionInfo.image_base);
const hits = records(path.join(session, "locomotion-write-watch.csv"));
const phases = records(path.join(session, "locomotion-phases.csv"));

const afterA = new Map();
const beforeDE = new Map();
for (const row of phases) {
  if (row.phase !== "2344730") continue;
  const key = `${row.broad_update_index}:${row.context}`;
  if (row.event === "after" && row.caller_rva === "0xA1429C") afterA.set(key, row);
  if (row.event === "before" && row.caller_rva === "0xDE50EC") beforeDE.set(key, row);
}

const writerOpcodes = hits.map((row) => row.code_bytes.slice(48, 64));
const expectedWriterOpcode = "660F7FB380000000";
const ripAfter = [...new Set(hits.map((row) => row.rip_after))];
const writerAddresses = [...new Set(hits.map((row) => hex(row.rip_after) - 8n))];
const functionRanges = [...new Set(hits.map((row) => `${row.function_begin}-${row.function_end}`))];
const contexts = [...new Set(hits.map((row) => row.shared_context))];
const transforms = [...new Set(hits.map((row) => row.transform))];
const tids = [...new Set(hits.map((row) => row.tid))];
const dr6 = [...new Set(hits.map((row) => row.dr6))];
const codes = [...new Set(hits.map((row) => row.code_bytes))];

const correlations = hits.map((row) => {
  const key = `${row.broad_update_index}:${row.shared_context}`;
  const from = afterA.get(key);
  const to = beforeDE.get(key);
  const xmm = xmmPosition(row);
  const hit = position(row, "hit_");
  const hitFloat32 = hit.map(Math.fround);
  const arm = position(row, "arm_");
  return {
    from,
    to,
    writer_xmm_distance: distance(xmm.slice(0, 3), hit),
    writer_xmm_float32_exact: xmm.slice(0, 3).every((value, index) =>
      Object.is(value, hitFloat32[index])),
    writer_w_distance_from_one: Math.abs(xmm[3] - 1),
    arm_to_after_a_distance: from ? distance(arm, position(from)) : null,
    hit_to_before_de_distance: to ? distance(hit, position(to)) : null,
    moving_transition_distance: from && to ? distance(position(from), position(to)) : null,
  };
});

const matched = correlations.filter((entry) => entry.from && entry.to);
const moving = matched.filter((entry) => entry.moving_transition_distance > 0);
const summary = {
  session: path.basename(session),
  condition: sessionInfo.condition,
  target_fps: Number(sessionInfo.target_fps),
  hit_count: hits.length,
  unique_thread_ids: tids,
  unique_contexts: contexts,
  unique_transforms: transforms,
  unique_dr6_values: dr6,
  unique_rip_after: ripAfter,
  unique_rip_after_rvas: ripAfter.map((value) => hexRva(hex(value), imageBase)),
  unique_writer_addresses: writerAddresses.map((value) => `0x${value.toString(16).toUpperCase()}`),
  unique_writer_rvas: writerAddresses.map((value) => hexRva(value, imageBase)),
  unique_function_ranges: functionRanges,
  unique_function_rva_ranges: functionRanges.map((range) => {
    const [begin, end] = range.split("-").map(hex);
    return `${hexRva(begin, imageBase)}-${hexRva(end, imageBase)}`;
  }),
  unique_live_byte_windows: codes.length,
  expected_writer_opcode: expectedWriterOpcode,
  writer_opcode_matches: writerOpcodes.filter((value) => value === expectedWriterOpcode).length,
  code_valid_count: hits.filter((row) => row.code_valid === "1").length,
  overflow_count: hits.filter((row) => row.overflow !== "0").length,
  matched_boundary_count: matched.length,
  moving_boundary_count: moving.length,
  moving_boundary_coverage: moving.length ? hits.length / moving.length : null,
  max_writer_xmm_to_hit_position_error: maximum(correlations.map((entry) => entry.writer_xmm_distance)),
  max_writer_w_distance_from_one: maximum(correlations.map((entry) => entry.writer_w_distance_from_one)),
  max_arm_to_a1429c_after_error: maximum(matched.map((entry) => entry.arm_to_after_a_distance)),
  max_hit_to_de50ec_before_error: maximum(matched.map((entry) => entry.hit_to_before_de_distance)),
  pass_1_gate: {
    one_writer_site: writerAddresses.length === 1 && ripAfter.length === 1,
    one_runtime_function: functionRanges.length === 1,
    one_thread_context_transform: tids.length === 1 && contexts.length === 1 && transforms.length === 1,
    every_hit_has_valid_code: hits.length > 0 && hits.every((row) => row.code_valid === "1"),
    no_overflow: hits.every((row) => row.overflow === "0"),
    expected_commit_instruction: hits.length > 0 && writerOpcodes.every((value) => value === expectedWriterOpcode),
    xmm6_exactly_matches_committed_xyzw: correlations.every((entry) =>
      entry.writer_xmm_float32_exact && entry.writer_w_distance_from_one === 0),
    every_hit_matches_boundary: hits.length > 0 && matched.length === hits.length,
    every_moving_boundary_has_one_hit: moving.length === hits.length,
    arm_and_hit_match_boundary_snapshots: matched.length === hits.length && matched.every((entry) =>
      entry.arm_to_after_a_distance === 0 && entry.hit_to_before_de_distance === 0),
  },
  scope: "Confirms the final transform commit site and exact committed vector only; requested/root motion and collision/physics input/output remain unresolved.",
  raw_sha256: {
    "locomotion-write-watch.csv": sha256(path.join(session, "locomotion-write-watch.csv")),
    "locomotion-phases.csv": sha256(path.join(session, "locomotion-phases.csv")),
    "locomotion-events.csv": sha256(path.join(session, "locomotion-events.csv")),
    "override-transitions.csv": sha256(path.join(session, "override-transitions.csv")),
    "session-info.txt": sha256(path.join(session, "session-info.txt")),
  },
};
summary.pass_1_gate.passed = Object.values(summary.pass_1_gate).every(Boolean);

fs.mkdirSync(output, { recursive: true });
fs.writeFileSync(path.join(output, "write-watch-summary.json"), `${JSON.stringify(summary, null, 2)}\n`);
if (codes.length === 1 && writerAddresses.length === 1) {
  const address = writerAddresses[0];
  fs.writeFileSync(path.join(output, "writer-live-window.hex.txt"), [
    `image_base=${sessionInfo.image_base}`,
    `writer_address=0x${address.toString(16).toUpperCase()}`,
    `writer_rva=${hexRva(address, imageBase)}`,
    `rip_after=${ripAfter[0]}`,
    `bytes_start=0x${(hex(ripAfter[0]) - 32n).toString(16).toUpperCase()}`,
    `bytes=${codes[0]}`,
    `decoded_writer=66 0F 7F B3 80 00 00 00 | MOVDQA [RBX+0x80], XMM6`,
    "",
  ].join("\n"));
}
console.log(JSON.stringify({ session: summary.session, hits: summary.hit_count,
  writer_rva: summary.unique_writer_rvas, passed: summary.pass_1_gate.passed }, null, 2));
