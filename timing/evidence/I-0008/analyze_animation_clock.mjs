import fs from "node:fs";
import path from "node:path";
import { fileURLToPath } from "node:url";

const repo = path.resolve(path.dirname(fileURLToPath(import.meta.url)), "../..");
const sessions = [
  { target: 60, id: "20260919T110038Z-pid27016" },
  { target: 90, id: "20260919T113730Z-pid23528" },
  { target: 120, id: "20260919T114725Z-pid27100" },
  { target: 165, id: "20260919T121245Z-pid6348" },
];

function parseCsv(file) {
  const lines = fs.readFileSync(file, "utf8").trim().split(/\r?\n/);
  const headers = lines[0].split(",");
  return lines.slice(1).map((line) => {
    const values = line.split(",");
    return Object.fromEntries(headers.map((header, index) => [header, values[index]]));
  });
}

function quantile(values, q) {
  if (!values.length) return null;
  const sorted = [...values].sort((a, b) => a - b);
  const index = (sorted.length - 1) * q;
  const lower = Math.floor(index);
  const upper = Math.ceil(index);
  if (lower === upper) return sorted[lower];
  return sorted[lower] + (sorted[upper] - sorted[lower]) * (index - lower);
}

function summarize(values) {
  return {
    count: values.length,
    min: values.length ? Math.min(...values) : null,
    median: quantile(values, 0.5),
    p95: quantile(values, 0.95),
    max: values.length ? Math.max(...values) : null,
  };
}

function keyOf(row) {
  return `${Number(row.animation_id)}|${Number(row.animation_max)}|${Number(row.play_speed)}`;
}

function analyzeSession(session) {
  const file = path.join(repo, "evidence", "I-0005", "runtime", "pass-c", "sessions", session.id, "semantic-state.csv");
  const rows = parseCsv(file);
  const classes = { invalid: 0, transition: 0, reset: 0, unchanged: 0, forward: 0, discontinuity: 0 };
  const forward = [];
  const partitions = new Map();
  for (let index = 1; index < rows.length; index += 1) {
    const previous = rows[index - 1];
    const current = rows[index];
    if (previous.animation_valid !== "1" || current.animation_valid !== "1") {
      classes.invalid += 1;
      continue;
    }
    if (Number(current.update_index) !== Number(previous.update_index) + 1) {
      classes.discontinuity += 1;
      continue;
    }
    if (current.animation_id !== previous.animation_id || current.animation_max !== previous.animation_max || current.play_speed !== previous.play_speed) {
      classes.transition += 1;
      continue;
    }
    const increment = Number(current.animation_time) - Number(previous.animation_time);
    if (increment < 0) {
      classes.reset += 1;
      continue;
    }
    if (increment === 0) {
      classes.unchanged += 1;
      continue;
    }
    classes.forward += 1;
    const delta = Number(current.delta);
    const ratio = increment / delta;
    const variableError = Math.abs(increment - 10 * delta);
    const fixedIncrement = 10 / 60;
    const fixedError = Math.abs(increment - fixedIncrement);
    const item = { increment, delta, ratio, variableError, fixedError };
    forward.push(item);
    const key = keyOf(current);
    if (!partitions.has(key)) partitions.set(key, []);
    partitions.get(key).push(item);
  }

  const partitionRows = [...partitions.entries()].map(([key, items]) => {
    const [animationId, animationMax, playSpeed] = key.split("|").map(Number);
    const ratios = items.map((item) => item.ratio);
    return {
      target: session.target,
      animation_id: animationId,
      animation_max: animationMax,
      play_speed: playSpeed,
      forward_rows: items.length,
      ratio_median: quantile(ratios, 0.5),
      ratio_p95_absolute_error_from_10: quantile(ratios.map((value) => Math.abs(value - 10)), 0.95),
      within_ratio_0_01: items.filter((item) => Math.abs(item.ratio - 10) <= 0.01).length,
    };
  }).sort((a, b) => b.forward_rows - a.forward_rows || a.animation_id - b.animation_id);

  const variableWins = forward.filter((item) => item.variableError < item.fixedError).length;
  const fixedWins = forward.filter((item) => item.fixedError < item.variableError).length;
  const ratios = forward.map((item) => item.ratio);
  return {
    target: session.target,
    session: session.id,
    source: path.relative(repo, file).replaceAll("\\", "/"),
    rows: rows.length,
    classifications: classes,
    forward_increment: summarize(forward.map((item) => item.increment)),
    delta: summarize(forward.map((item) => item.delta)),
    increment_over_delta: summarize(ratios),
    ratio_within_0_01_of_10: forward.filter((item) => Math.abs(item.ratio - 10) <= 0.01).length,
    model_comparison: {
      variable_delta_wins: variableWins,
      fixed_60_step_wins: fixedWins,
      ties: forward.length - variableWins - fixedWins,
      variable_absolute_error: summarize(forward.map((item) => item.variableError)),
      fixed_60_absolute_error: summarize(forward.map((item) => item.fixedError)),
    },
    partitions: partitionRows,
  };
}

const results = sessions.map(analyzeSession);
const allForward = results.reduce((sum, result) => sum + result.classifications.forward, 0);
const allVariableWins = results.reduce((sum, result) => sum + result.model_comparison.variable_delta_wins, 0);
const output = {
  schema_version: 1,
  subject: "Retained Pass C player-animation clock audit",
  exact_build_sha256: "EF5E07C55222F14FFDDECF2C724A0C2A95CEF0D4DA0E075B0DE0BB108B69498C",
  method: "Consecutive valid semantic samples with adjacent broad-update indices. Forward rows require unchanged animation ID, max and play speed plus a strictly positive clock increment. Variable model is increment=10*current broad delta; fixed model is increment=10/60.",
  aggregate: {
    forward_rows: allForward,
    variable_delta_wins: allVariableWins,
    fixed_60_step_wins: results.reduce((sum, result) => sum + result.model_comparison.fixed_60_step_wins, 0),
    variable_win_fraction: allForward ? allVariableWins / allForward : null,
  },
  sessions: results,
  limits: "The qualified field is sampled after each broad update, not at its writer. Correlation and model fit do not identify the writer, prove direct delta consumption, name animation IDs, or establish whole-animation-system correctness. Human-repeated marker durations remain excluded from deterministic equality claims.",
};

const outDir = path.join(repo, "evidence", "I-0008", "artifacts", "retained-clock-audit");
fs.mkdirSync(outDir, { recursive: true });
fs.writeFileSync(path.join(outDir, "animation-clock-audit.json"), `${JSON.stringify(output, null, 2)}\n`);

const csvHeader = "target,animation_id,animation_max,play_speed,forward_rows,ratio_median,ratio_p95_absolute_error_from_10,within_ratio_0_01";
const csvRows = results.flatMap((result) => result.partitions.map((row) => [
  row.target,
  row.animation_id,
  row.animation_max,
  row.play_speed,
  row.forward_rows,
  row.ratio_median,
  row.ratio_p95_absolute_error_from_10,
  row.within_ratio_0_01,
].join(",")));
fs.writeFileSync(path.join(outDir, "animation-clock-partitions.csv"), `${csvHeader}\n${csvRows.join("\n")}\n`);
process.stdout.write(`${JSON.stringify(output.aggregate)}\n`);
