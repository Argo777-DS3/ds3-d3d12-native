#!/usr/bin/env node
// Apply D-0009's ordered discriminator to two validity-checked mirror sessions.

import crypto from "node:crypto";
import fs from "node:fs";
import path from "node:path";

const [session60Arg, reconstruction60Arg, session165Arg, reconstruction165Arg, outputArg] =
  process.argv.slice(2);
if (!session60Arg || !reconstruction60Arg || !session165Arg ||
    !reconstruction165Arg || !outputArg) {
  throw new Error("Usage: node compare_writer_mirror.mjs <session-60> <reconstruction-60.csv> <session-165> <reconstruction-165.csv> <output>");
}

function records(file) {
  const text = fs.readFileSync(file, "utf8").trim();
  if (!text) return [];
  const lines = text.split(/\r?\n/);
  const headers = lines.shift().split(",");
  return lines.filter(Boolean).map((line) => Object.fromEntries(
    line.split(",").map((value, index) => [headers[index], value]),
  ));
}

function sessionInfo(session) {
  return Object.fromEntries(fs.readFileSync(path.join(session, "session-info.txt"), "utf8")
    .trim().split(/\r?\n/).map((line) => line.split(/=(.*)/s).slice(0, 2)));
}

function sha256(file) {
  return crypto.createHash("sha256").update(fs.readFileSync(file)).digest("hex").toUpperCase();
}

function parseBits(text) {
  return Number.parseInt(text, 16) >>> 0;
}

function bitsToFloat(text) {
  const buffer = new ArrayBuffer(4);
  const view = new DataView(buffer);
  view.setUint32(0, parseBits(text), true);
  return view.getFloat32(0, true);
}

function floatToBits(value) {
  const buffer = new ArrayBuffer(4);
  const view = new DataView(buffer);
  view.setFloat32(0, value, true);
  return view.getUint32(0, true);
}

const vector = (row, prefix) => [0, 1, 2].map((lane) => bitsToFloat(row[`${prefix}${lane}`]));
const add = (left, right) => left.map((value, index) => value + right[index]);
const subtract = (left, right) => left.map((value, index) => value - right[index]);
const dot = (left, right) => left.reduce((sum, value, index) => sum + value * right[index], 0);
const norm = (value) => Math.hypot(...value);
const mean = (values) => values.reduce((sum, value) => sum + value, 0) / values.length;
const ratio = (numerator, denominator) => denominator === 0 ? null : numerator / denominator;
const near = (value, expected, tolerance = 0.02) =>
  value !== null && Number.isFinite(value) && Math.abs(value - expected) <= tolerance;

function countBy(rows, selector) {
  return Object.fromEntries([...rows.reduce((counts, row) => {
    const key = selector(row);
    counts.set(key, (counts.get(key) ?? 0) + 1);
    return counts;
  }, new Map()).entries()].sort(([left], [right]) => left.localeCompare(right)));
}

function normalizedCounts(counts) {
  const total = Object.values(counts).reduce((sum, count) => sum + count, 0);
  return Object.fromEntries(Object.entries(counts).map(([key, count]) =>
    [key, total ? count / total : 0]));
}

function categoricalChanged(left, right) {
  const keys = new Set([...Object.keys(left), ...Object.keys(right)]);
  return [...keys].some((key) => Math.abs((left[key] ?? 0) - (right[key] ?? 0)) > 1e-12);
}

function loadCondition(sessionArg, reconstructionArg, expectedTarget) {
  const session = path.resolve(sessionArg);
  const reconstructionPath = path.resolve(reconstructionArg);
  const info = sessionInfo(session);
  const target = Number(info.target_fps);
  if (Math.abs(target - expectedTarget) > 0.001) {
    throw new Error(`Expected target ${expectedTarget}, found ${target} in ${session}`);
  }

  const mirrorPath = path.join(session, "locomotion-writer-mirror.csv");
  const updatesPath = path.join(session, "locomotion-updates.csv");
  const eventsPath = path.join(session, "locomotion-events.csv");
  const mirrors = records(mirrorPath);
  const reconstructions = records(reconstructionPath);
  const updates = records(updatesPath).filter((row) => row.phase === "measure");
  const events = records(eventsPath);
  const mirrorBySequence = new Map(mirrors.map((row) => [row.seq, row]));
  const updateByKey = new Map(updates.map((row) => [`${row.trial}:${row.update_index}`, row]));

  const categoricalRows = mirrors.filter((row) => row.rbx_matches_transform === "1" &&
    row.outer_a8_read_valid === "1" && row.outer_b0_read_valid === "1" &&
    row.inhibit_valid === "1");
  const sourceCounts = countBy(categoricalRows, (row) => row.source);
  const inhibitionCounts = countBy(categoricalRows, (row) => row.inhibit);

  const accepted = [];
  const pairIssues = [];
  const eligibleCommitKeys = [];
  for (const reconstruction of reconstructions) {
    if (reconstruction.precommit_valid !== "1" || reconstruction.will_commit !== "1") continue;
    const commitKey = `${reconstruction.trial}:${reconstruction.broad_update_index}`;
    eligibleCommitKeys.push(commitKey);
    const mirror = mirrorBySequence.get(reconstruction.seq);
    if (!mirror) {
      pairIssues.push(`missing_mirror_seq_${reconstruction.seq}`);
      continue;
    }
    const cadenceUpdate = updateByKey.get(commitKey);
    if (!cadenceUpdate) {
      pairIssues.push(`missing_update_${reconstruction.trial}_${reconstruction.broad_update_index}`);
      continue;
    }
    // The writer fires after broad update N has returned. Its commit is first visible in the
    // position snapshot taken at the start of broad update N+1, while update N supplies the
    // delta that governed this commit.
    const confirmationIndex = Number(reconstruction.broad_update_index) + 1;
    const confirmationUpdate = updateByKey.get(`${reconstruction.trial}:${confirmationIndex}`);
    if (!confirmationUpdate) {
      pairIssues.push(`missing_confirmation_update_${reconstruction.trial}_${confirmationIndex}`);
      continue;
    }
    const finalBits = [0, 1, 2].map((lane) => parseBits(mirror[`final_xmm6_b${lane}`]));
    const postBits = ["before_x", "before_z", "before_y"].map((column) =>
      floatToBits(Number(confirmationUpdate[column])));
    if (!finalBits.every((value, lane) => value === postBits[lane])) {
      pairIssues.push(`post_commit_mismatch_seq_${reconstruction.seq}`);
      continue;
    }
    const displacement = norm(vector(reconstruction, "a_b"));
    if (displacement <= 1e-7) continue;
    accepted.push({
      mirror,
      reconstruction,
      cadenceUpdate,
      confirmationUpdate,
      candidate: vector(reconstruction, "d_b"),
      applied: vector(reconstruction, "a_b"),
      scalar: bitsToFloat(mirror.scalar_memory_bits),
      delta: Number(cadenceUpdate.delta),
    });
  }

  const acceptedKeys = new Set(accepted.map(({ reconstruction }) =>
    `${reconstruction.trial}:${reconstruction.broad_update_index}`));
  const unmatchedMobileCommits = eligibleCommitKeys.filter((key) => !acceptedKeys.has(key));
  const duplicateAcceptedUpdates = [...acceptedKeys].filter((key) =>
    accepted.filter(({ reconstruction }) =>
      `${reconstruction.trial}:${reconstruction.broad_update_index}` === key).length !== 1);
  const completeTrials = [...new Set(events.filter((row) => row.event === "measure_end")
    .map((row) => row.trial))];

  return {
    session,
    info,
    target,
    mirrorPath,
    updatesPath,
    eventsPath,
    reconstructionPath,
    categoricalRows,
    sourceCounts,
    sourceFrequencies: normalizedCounts(sourceCounts),
    inhibitionCounts,
    inhibitionFrequencies: normalizedCounts(inhibitionCounts),
    accepted,
    pairIssues,
    unmatchedMobileCommits,
    duplicateAcceptedUpdates,
    completeTrials,
  };
}

function summarize(condition, axis) {
  const candidateSum = condition.accepted.reduce((sum, row) => add(sum, row.candidate), [0, 0, 0]);
  const appliedSum = condition.accepted.reduce((sum, row) => add(sum, row.applied), [0, 0, 0]);
  const candidatePath = condition.accepted.reduce((sum, row) => sum + norm(row.candidate), 0);
  const appliedPath = condition.accepted.reduce((sum, row) => sum + norm(row.applied), 0);
  const accumulatedDelta = condition.accepted.reduce((sum, row) => sum + row.delta, 0);
  const scalars = condition.accepted.map((row) => row.scalar);
  return {
    target_fps: condition.target,
    session: path.basename(condition.session),
    rows_captured: records(condition.mirrorPath).length,
    categorical_rows: condition.categoricalRows.length,
    accepted_mobile_commits: condition.accepted.length,
    complete_trials: condition.completeTrials.length,
    accumulated_delta: accumulatedDelta,
    source_counts: condition.sourceCounts,
    source_frequencies: condition.sourceFrequencies,
    inhibition_counts: condition.inhibitionCounts,
    inhibition_frequencies: condition.inhibitionFrequencies,
    candidate: {
      vector_sum: candidateSum,
      projected_rate: ratio(dot(candidateSum, axis), accumulatedDelta),
      path_norm_rate: ratio(candidatePath, accumulatedDelta),
    },
    scalar: {
      mean: scalars.length ? mean(scalars) : null,
      min: scalars.length ? Math.min(...scalars) : null,
      max: scalars.length ? Math.max(...scalars) : null,
      bit_frequencies: countBy(condition.accepted, (row) => row.mirror.scalar_memory_bits),
    },
    final: {
      vector_sum: appliedSum,
      projected_rate: ratio(dot(appliedSum, axis), accumulatedDelta),
      path_norm_rate: ratio(appliedPath, accumulatedDelta),
    },
    validity: {
      pairing_issues: condition.pairIssues,
      unmatched_mobile_commits: condition.unmatchedMobileCommits,
      duplicate_accepted_updates: condition.duplicateAcceptedUpdates,
      passed: condition.accepted.length > 0 && condition.completeTrials.length > 0 &&
        condition.pairIssues.length === 0 && condition.unmatchedMobileCommits.length === 0 &&
        condition.duplicateAcceptedUpdates.length === 0,
    },
    sha256: {
      "locomotion-writer-mirror.csv": sha256(condition.mirrorPath),
      "locomotion-updates.csv": sha256(condition.updatesPath),
      "locomotion-events.csv": sha256(condition.eventsPath),
      "session-info.txt": sha256(path.join(condition.session, "session-info.txt")),
      reconstruction: sha256(condition.reconstructionPath),
    },
  };
}

const condition60 = loadCondition(session60Arg, reconstruction60Arg, 60);
const condition165 = loadCondition(session165Arg, reconstruction165Arg, 165);
const preliminary60 = summarize(condition60, [1, 0, 0]);
const axisNorm = norm(preliminary60.final.vector_sum);
if (axisNorm === 0) throw new Error("Target-60 final net direction is zero");
const axis = preliminary60.final.vector_sum.map((value) => value / axisNorm);
const summary60 = summarize(condition60, axis);
const summary165 = summarize(condition165, axis);

const sourceChanged = categoricalChanged(summary60.source_frequencies,
  summary165.source_frequencies);
const inhibitionChanged = categoricalChanged(summary60.inhibition_frequencies,
  summary165.inhibition_frequencies);
const ratios = {
  projected: {
    final: ratio(summary165.final.projected_rate, summary60.final.projected_rate),
    candidate: ratio(summary165.candidate.projected_rate, summary60.candidate.projected_rate),
  },
  path: {
    final: ratio(summary165.final.path_norm_rate, summary60.final.path_norm_rate),
    candidate: ratio(summary165.candidate.path_norm_rate, summary60.candidate.path_norm_rate),
  },
  raw_scalar_mean: ratio(summary165.scalar.mean, summary60.scalar.mean),
};
ratios.projected.scalar_effective = ratio(ratios.projected.final, ratios.projected.candidate);
ratios.path.scalar_effective = ratio(ratios.path.final, ratios.path.candidate);

const validityPassed = summary60.validity.passed && summary165.validity.passed;
let conclusion = "invalid_or_insufficient";
if (validityPassed && (sourceChanged || inhibitionChanged)) {
  conclusion = "categorical_source_or_inhibition_divergence";
} else if (validityPassed &&
           near(ratios.projected.candidate, ratios.projected.final) &&
           near(ratios.path.candidate, ratios.path.final) &&
           near(ratios.projected.scalar_effective, 1) &&
           near(ratios.path.scalar_effective, 1)) {
  conclusion = "candidate_offset_first_numeric_explanation";
} else if (validityPassed &&
           near(ratios.projected.candidate, 1) && near(ratios.path.candidate, 1) &&
           near(ratios.projected.scalar_effective, ratios.projected.final) &&
           near(ratios.path.scalar_effective, ratios.path.final) &&
           near(ratios.raw_scalar_mean, ratios.projected.final) &&
           near(ratios.raw_scalar_mean, ratios.path.final)) {
  conclusion = "scalar_first_numeric_explanation";
} else if (validityPassed &&
           !near(ratios.projected.candidate, 1) &&
           !near(ratios.projected.scalar_effective, 1)) {
  conclusion = "candidate_and_scalar_joint_explanation";
} else if (validityPassed) {
  conclusion = "inputs_do_not_explain_final_reject_alignment_or_local_calculation";
}

const summary = {
  schema: "I-0006-writer-input-mirror-comparison-v1",
  tolerance: 0.02,
  fixed_projection_axis_from_target_60_final: axis,
  validity_passed: validityPassed,
  categorical: {
    source_selection_changed: sourceChanged,
    inhibition_frequency_changed: inhibitionChanged,
  },
  conditions: [summary60, summary165],
  ratios_165_over_60: ratios,
  ordered_conclusion: conclusion,
  scope_note: "Computational discriminator only. The candidate source remains semantically neutral; Pass 2, roll, correction, and 240 remain closed.",
};

const output = path.resolve(outputArg);
fs.mkdirSync(output, { recursive: true });
fs.writeFileSync(path.join(output, "writer-mirror-comparison.json"),
  `${JSON.stringify(summary, null, 2)}\n`);
console.log(JSON.stringify({ validity_passed: validityPassed, ordered_conclusion: conclusion,
  ratios_165_over_60: ratios }, null, 2));
if (!validityPassed) process.exitCode = 1;
