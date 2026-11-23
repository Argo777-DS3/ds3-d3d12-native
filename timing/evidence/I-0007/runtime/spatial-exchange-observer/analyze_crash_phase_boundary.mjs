import fs from "node:fs";
import readline from "node:readline";

const [crashPath, healthyPath, outputPath] = process.argv.slice(2);
if (!crashPath || !healthyPath || !outputPath) {
  throw new Error("usage: node analyze_crash_phase_boundary.mjs <crash.csv> <healthy.csv> <output.json>");
}

const EXPECTED_COLUMNS = 21;
const TARGET_CALLER = "0xECA94C";

function percentile(sorted, fraction) {
  if (sorted.length === 0) return null;
  return sorted[Math.floor((sorted.length - 1) * fraction)];
}

async function analyze(path) {
  const input = fs.createReadStream(path);
  const lines = readline.createInterface({ input, crlfDelay: Infinity });
  let header = null;
  let completeRows = 0;
  let incompleteRows = 0;
  let lastRaw = null;
  let lastIncompleteRow = null;
  let lastComplete = null;
  let pendingTarget = null;
  let targetBefore = 0;
  let targetAfter = 0;
  let targetImmediatePairs = 0;
  let targetUnmatchedBefore = 0;
  const targetDurations = [];
  const targetAnimationCounts = new Map();

  for await (const raw of lines) {
    if (header === null) {
      header = raw.split(",");
      continue;
    }
    if (!raw) continue;
    lastRaw = raw;
    const fields = raw.split(",");
    if (fields.length !== EXPECTED_COLUMNS) {
      incompleteRows++;
      lastIncompleteRow = Object.fromEntries(
        fields.map((value, index) => [header[index] || `column_${index}`, value]),
      );
      continue;
    }
    completeRows++;
    const row = Object.fromEntries(header.map((name, index) => [name, fields[index]]));
    lastComplete = row;
    const isTarget = row.phase === "2344730" && row.caller_rva === TARGET_CALLER;
    if (isTarget && row.event === "before") {
      targetBefore++;
      if (pendingTarget) targetUnmatchedBefore++;
      pendingTarget = row;
      const key = row.animation_id || "unknown";
      targetAnimationCounts.set(key, (targetAnimationCounts.get(key) || 0) + 1);
    } else if (isTarget && row.event === "after") {
      targetAfter++;
      if (pendingTarget && pendingTarget.self === row.self && pendingTarget.context === row.context) {
        targetImmediatePairs++;
        targetDurations.push(Number(row.qpc) - Number(pendingTarget.qpc));
        pendingTarget = null;
      }
    } else if (pendingTarget) {
      targetUnmatchedBefore++;
      pendingTarget = null;
    }
  }
  if (pendingTarget) targetUnmatchedBefore++;
  targetDurations.sort((a, b) => a - b);
  return {
    path,
    complete_rows: completeRows,
    incomplete_rows: incompleteRows,
    last_raw_line: lastRaw,
    last_incomplete_row: lastIncompleteRow,
    last_complete_row: lastComplete,
    target_caller: TARGET_CALLER,
    target_before: targetBefore,
    target_after: targetAfter,
    target_immediate_pairs: targetImmediatePairs,
    target_unmatched_before: targetUnmatchedBefore,
    target_pair_qpc_ticks: {
      minimum: percentile(targetDurations, 0),
      median: percentile(targetDurations, 0.5),
      p95: percentile(targetDurations, 0.95),
      maximum: percentile(targetDurations, 1),
    },
    target_before_animation_ids: Object.fromEntries(
      [...targetAnimationCounts.entries()].sort((a, b) => Number(a[0]) - Number(b[0])),
    ),
  };
}

const [crash, healthy] = await Promise.all([analyze(crashPath), analyze(healthyPath)]);
const result = {
  schema: "I-0007-crash-phase-boundary-v1",
  comparison: {
    crash,
    healthy,
  },
  conclusion: {
    target_caller_seen_complete_in_healthy_capture: healthy.target_immediate_pairs > 0,
    target_animation_seen_at_caller_in_healthy_capture:
      Object.hasOwn(healthy.target_before_animation_ids, "22100"),
    final_animation_previously_completed_at_target_in_crash_session:
      Number(crash.target_before_animation_ids["22100"] || 0) > 0,
    prior_complete_target_pairs_in_crash_session: crash.target_immediate_pairs,
    crash_last_line_is_incomplete: crash.incomplete_rows > 0,
    causal_limit:
      "The caller and animation context can be classified as ordinary or exceptional, but the CSV and WER report do not contain the faulting instruction, stack, or register state needed to identify the invalid execute source.",
  },
};

fs.writeFileSync(outputPath, `${JSON.stringify(result, null, 2)}\n`);
