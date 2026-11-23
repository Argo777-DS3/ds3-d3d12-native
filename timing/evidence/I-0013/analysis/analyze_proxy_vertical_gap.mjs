import fs from 'node:fs';

function parseCsv(file) {
  const lines = fs.readFileSync(file, 'utf8').trim().split(/\r?\n/);
  const header = lines.shift().split(',');
  return lines.filter(Boolean).map(line => Object.fromEntries(
    line.split(',').map((value, index) => [header[index], value])));
}

function floatFromBits(text) {
  const bytes = Buffer.alloc(4);
  bytes.writeUInt32LE(Number.parseInt(text, 16), 0);
  return bytes.readFloatLE(0);
}

function block(text) {
  if (!text || text.length !== 96) return null;
  return Array.from({ length: 12 }, (_, index) =>
    floatFromBits(text.slice(index * 8, index * 8 + 8)));
}

function distribution(values) {
  const sorted = values.filter(Number.isFinite).sort((a, b) => a - b);
  if (!sorted.length) return { count: 0, minimum: null, median: null, maximum: null };
  const quantile = fraction => sorted[Math.floor((sorted.length - 1) * fraction)];
  return {
    count: sorted.length,
    minimum: sorted[0],
    p05: quantile(0.05),
    median: quantile(0.5),
    p95: quantile(0.95),
    maximum: sorted.at(-1),
  };
}

function correlation(left, right) {
  if (left.length !== right.length || left.length < 2) return null;
  const lm = left.reduce((sum, value) => sum + value, 0) / left.length;
  const rm = right.reduce((sum, value) => sum + value, 0) / right.length;
  let covariance = 0;
  let lv = 0;
  let rv = 0;
  for (let index = 0; index < left.length; index++) {
    const a = left[index] - lm;
    const b = right[index] - rm;
    covariance += a * b;
    lv += a * a;
    rv += b * b;
  }
  return covariance / Math.sqrt(lv * rv);
}

function analyze(spatialFile, updatesFile, label) {
  const rows = parseCsv(spatialFile);
  const updates = new Map(parseCsv(updatesFile).map(row => [row.update_index, row]));
  const groupsByKey = new Map();
  for (const row of rows) {
    const key = [row.broad_update_index, row.owner, row.selected].join('|');
    if (!groupsByKey.has(key)) groupsByKey.set(key, {});
    groupsByKey.get(key)[row.site] = row;
  }
  const sites = ['forward_entry', 'forward_write', 'reverse_entry', 'reverse_write'];
  const complete = [...groupsByKey.values()].filter(group =>
    sites.every(site => group[site]));
  const slotOwners = new Map();
  const slotCounts = new Map();
  for (const group of complete) {
    const slot = group.forward_entry.selected;
    if (!slotOwners.has(slot)) slotOwners.set(slot, new Set());
    slotOwners.get(slot).add(group.forward_entry.owner);
    slotCounts.set(slot, (slotCounts.get(slot) ?? 0) + 1);
  }
  const stableSlot = [...slotOwners].filter(([, owners]) => owners.size === 1)
    .sort((left, right) => slotCounts.get(right[0]) - slotCounts.get(left[0]))[0]?.[0];
  if (!stableSlot) throw new Error(`${label}: no stable slot`);
  const groups = complete.filter(group => group.forward_entry.selected === stableSlot)
    .sort((left, right) => Number(left.forward_entry.broad_update_index) -
      Number(right.forward_entry.broad_update_index));
  const selectedGap = [];
  const currentGapBefore = [];
  const currentGapAfter = [];
  const proxyDelta = [];
  const playerDelta = [];
  const gapDelta = [];
  const rowsOut = [];
  for (const group of groups) {
    const update = updates.get(group.forward_entry.broad_update_index);
    const selected = block(group.forward_entry.selected48_bits);
    const current = block(group.reverse_write.payload48_bits);
    if (!update || !selected || !current) continue;
    const beforeZ = Number(update.before_z);
    const afterZ = Number(update.after_z);
    selectedGap.push(selected[1] - beforeZ);
    currentGapBefore.push(current[1] - beforeZ);
    currentGapAfter.push(current[1] - afterZ);
    rowsOut.push({
      update: Number(group.forward_entry.broad_update_index),
      selected_z: selected[1],
      current_z: current[1],
      player_before_z: beforeZ,
      player_after_z: afterZ,
    });
  }
  for (let index = 1; index < rowsOut.length; index++) {
    const proxy = rowsOut[index].current_z - rowsOut[index - 1].current_z;
    const player = rowsOut[index].player_before_z - rowsOut[index - 1].player_before_z;
    proxyDelta.push(proxy);
    playerDelta.push(player);
    gapDelta.push(proxy - player);
  }
  return {
    label,
    stable_selected_slot: stableSlot,
    complete_exchange_count: complete.length,
    stable_exchange_count: groups.length,
    compared_rows: rowsOut.length,
    lane1_minus_player_before_z: {
      selected: distribution(selectedGap),
      reverse_current: distribution(currentGapBefore),
    },
    reverse_current_lane1_minus_player_after_z: distribution(currentGapAfter),
    consecutive_vertical_delta: {
      proxy_lane1: distribution(proxyDelta),
      player_transform_z: distribution(playerDelta),
      proxy_minus_player_delta: distribution(gapDelta),
      correlation: correlation(proxyDelta, playerDelta),
    },
  };
}

const [output, ...inputs] = process.argv.slice(2);
if (!output || inputs.length < 3 || inputs.length % 3 !== 0) {
  throw new Error('usage: node analyze_proxy_vertical_gap.mjs OUT SPATIAL UPDATES LABEL [...]');
}
const captures = [];
for (let index = 0; index < inputs.length; index += 3) {
  captures.push(analyze(inputs[index], inputs[index + 1], inputs[index + 2]));
}
fs.writeFileSync(output, JSON.stringify({
  schema_version: 1,
  investigation_id: 'I-0013',
  captures,
  limit: 'Retrospective analysis of I-0007 routes; target165 remains prefix-only and overflow-rejected. Lane1 semantics remain unassigned.',
}, null, 2) + '\n');
