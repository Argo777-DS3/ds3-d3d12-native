import fs from 'node:fs';

const SITES = ['forward_entry', 'forward_write', 'reverse_entry', 'reverse_write'];

function parseCsv(file) {
  const text = fs.readFileSync(file, 'utf8').trim();
  if (!text) throw new Error(`${file}: empty CSV`);
  const lines = text.split(/\r?\n/);
  const header = lines.shift().split(',');
  return lines.filter(Boolean).map((line, rowIndex) => {
    const values = line.split(',');
    if (values.length !== header.length) {
      throw new Error(`${file}:${rowIndex + 2}: ${values.length} fields, expected ${header.length}`);
    }
    return Object.fromEntries(values.map((value, index) => [header[index], value]));
  });
}

function floatFromBits(text) {
  if (!text) return Number.NaN;
  const value = Number.parseInt(text, 16);
  if (!Number.isInteger(value)) return Number.NaN;
  const bytes = Buffer.allocUnsafe(4);
  bytes.writeUInt32LE(value >>> 0, 0);
  return bytes.readFloatLE(0);
}

function block(text) {
  if (!text || text.length !== 96) return null;
  const decoded = Array.from({ length: 12 }, (_, index) =>
    floatFromBits(text.slice(index * 8, index * 8 + 8)));
  return decoded.every(Number.isFinite) ? decoded : null;
}

function distribution(values) {
  const sorted = values.filter(Number.isFinite).sort((a, b) => a - b);
  if (!sorted.length) {
    return { count: 0, minimum: null, p05: null, median: null, p95: null, maximum: null };
  }
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
  const pairs = left.map((value, index) => [value, right[index]])
    .filter(pair => pair.every(Number.isFinite));
  if (pairs.length < 2) return null;
  const lm = pairs.reduce((sum, pair) => sum + pair[0], 0) / pairs.length;
  const rm = pairs.reduce((sum, pair) => sum + pair[1], 0) / pairs.length;
  let covariance = 0;
  let lv = 0;
  let rv = 0;
  for (const [leftValue, rightValue] of pairs) {
    const a = leftValue - lm;
    const b = rightValue - rm;
    covariance += a * b;
    lv += a * a;
    rv += b * b;
  }
  const denominator = Math.sqrt(lv * rv);
  return denominator === 0 ? null : covariance / denominator;
}

function sourceZ(row) {
  if (row.source_transform_valid !== '1') return Number.NaN;
  const base = floatFromBits(row.source_b1);
  if (!Number.isFinite(base)) return Number.NaN;
  if (row.source_selector_equal !== '1') return base;
  if (row.source_extra_valid !== '1') return Number.NaN;
  return base + floatFromBits(row.source_extra_b);
}

function rootZ(row) {
  return row.root_position_valid === '1' ? floatFromBits(row.root_z_b) : Number.NaN;
}

function stageValues(group) {
  const forwardSelected = block(group.forward_entry.selected48_bits);
  const forwardDerived = block(group.forward_write.payload48_bits);
  const reverseSelected = block(group.reverse_entry.selected48_bits);
  const proxyCurrent = block(group.reverse_write.payload48_bits);
  if (![forwardSelected, forwardDerived, reverseSelected, proxyCurrent].every(Boolean)) return null;
  return {
    source_z: sourceZ(group.forward_entry),
    cache_selected_z: forwardSelected[1],
    forward_derived_z: forwardDerived[1],
    reverse_selected_z: reverseSelected[1],
    proxy_current_z: proxyCurrent[1],
    root_z: rootZ(group.reverse_write),
  };
}

function gapSummary(stages, left, right) {
  return distribution(stages.map(stage => stage[right] - stage[left]));
}

function deltaSeries(stages, field) {
  return stages.slice(1).map((stage, index) => stage[field] - stages[index][field]);
}

function analyzeRows(rows, label) {
  const groupsByKey = new Map();
  for (const row of rows) {
    const key = [row.broad_update_index, row.owner, row.selected].join('|');
    if (!groupsByKey.has(key)) groupsByKey.set(key, []);
    groupsByKey.get(key).push(row);
  }

  let duplicateSiteGroups = 0;
  let incompleteGroups = 0;
  let outOfOrderGroups = 0;
  const accepted = [];
  for (const groupedRows of groupsByKey.values()) {
    const sites = new Map();
    let hasDuplicateSite = false;
    for (const row of groupedRows) {
      if (sites.has(row.site)) hasDuplicateSite = true;
      sites.set(row.site, row);
    }
    if (hasDuplicateSite) {
      duplicateSiteGroups++;
      continue;
    }
    if (!SITES.every(site => sites.has(site)) || sites.size !== SITES.length) {
      incompleteGroups++;
      continue;
    }
    const group = Object.fromEntries(SITES.map(site => [site, sites.get(site)]));
    const sequences = SITES.map(site => Number(group[site].seq));
    if (!sequences.every((value, index) => index === 0 || value > sequences[index - 1])) {
      outOfOrderGroups++;
      continue;
    }
    const requiredReadsValid = SITES.every(site => {
      const row = group[site];
      const blockValid = site.endsWith('entry')
        ? row.selected48_valid === '1'
        : row.payload48_valid === '1';
      return row.overflow === '0' && row.pointer_chain_valid === '1' &&
        row.source_transform_valid === '1' && row.root_position_valid === '1' &&
        blockValid;
    });
    if (!requiredReadsValid) continue;
    const stage = stageValues(group);
    if (!stage || !Object.values(stage).every(Number.isFinite)) continue;
    accepted.push({
      update: Number(group.forward_entry.broad_update_index),
      first_sequence: sequences[0],
      group,
      stage,
    });
  }
  accepted.sort((a, b) => a.first_sequence - b.first_sequence);
  const stages = accepted.map(item => item.stage);
  const updateCounts = new Map();
  for (const item of accepted) {
    updateCounts.set(item.update, (updateCounts.get(item.update) ?? 0) + 1);
  }

  const fields = [
    'source_z', 'cache_selected_z', 'forward_derived_z',
    'reverse_selected_z', 'proxy_current_z', 'root_z',
  ];
  const deltas = Object.fromEntries(fields.map(field => [field, deltaSeries(stages, field)]));
  const rootDeltas = deltas.root_z;
  const phaseSourceDrift = accepted.flatMap(item => SITES.slice(1).map(site =>
    sourceZ(item.group[site]) - sourceZ(item.group.forward_entry)));
  const phaseRootDrift = accepted.flatMap(item => SITES.slice(1).map(site =>
    rootZ(item.group[site]) - rootZ(item.group.forward_entry)));
  const overflowRows = rows.filter(row => row.overflow !== '0').length;
  const invalidPointerRows = rows.filter(row => row.pointer_chain_valid !== '1').length;
  const invalidSourceRows = rows.filter(row => row.source_transform_valid !== '1').length;
  const invalidRootRows = rows.filter(row => row.root_position_valid !== '1').length;
  const invalidBlockRows = rows.filter(row => row.site?.endsWith('entry')
    ? row.selected48_valid !== '1'
    : row.payload48_valid !== '1').length;
  const structuralPass = accepted.length > 0 && incompleteGroups === 0 &&
    duplicateSiteGroups === 0 && outOfOrderGroups === 0 && overflowRows === 0 &&
    invalidPointerRows === 0 && invalidSourceRows === 0 && invalidRootRows === 0 &&
    invalidBlockRows === 0;

  return {
    label,
    structure: {
      structural_pass: structuralPass,
      row_count: rows.length,
      candidate_group_count: groupsByKey.size,
      accepted_complete_ordered_finite_groups: accepted.length,
      incomplete_group_count: incompleteGroups,
      duplicate_site_group_observations: duplicateSiteGroups,
      out_of_order_group_count: outOfOrderGroups,
      overflow_row_count: overflowRows,
      invalid_pointer_chain_row_count: invalidPointerRows,
      invalid_source_row_count: invalidSourceRows,
      invalid_root_row_count: invalidRootRows,
      invalid_required_block_row_count: invalidBlockRows,
      selector_equal_row_count: rows.filter(row => row.source_selector_equal === '1').length,
      groups_per_broad_update: distribution([...updateCounts.values()]),
    },
    vertical_stage_values: Object.fromEntries(fields.map(field =>
      [field, distribution(stages.map(stage => stage[field]))])),
    vertical_stage_gaps: {
      cache_selected_minus_source: gapSummary(stages, 'source_z', 'cache_selected_z'),
      forward_derived_minus_cache_selected: gapSummary(stages, 'cache_selected_z', 'forward_derived_z'),
      reverse_selected_minus_forward_derived: gapSummary(stages, 'forward_derived_z', 'reverse_selected_z'),
      proxy_current_minus_source: gapSummary(stages, 'source_z', 'proxy_current_z'),
      root_minus_proxy_current: gapSummary(stages, 'proxy_current_z', 'root_z'),
      root_minus_source: gapSummary(stages, 'source_z', 'root_z'),
    },
    within_exchange_phase_drift: {
      source_relative_to_forward_entry: distribution(phaseSourceDrift),
      root_relative_to_forward_entry: distribution(phaseRootDrift),
    },
    consecutive_vertical_deltas: Object.fromEntries(fields.map(field => [field, {
      distribution: distribution(deltas[field]),
      correlation_with_root_delta: correlation(deltas[field], rootDeltas),
    }])),
    exact_block_relations: {
      forward_selected_equals_reverse_selected: accepted.filter(item =>
        item.group.forward_entry.selected48_bits === item.group.reverse_entry.selected48_bits).length,
      forward_derived_equals_proxy_current: accepted.filter(item =>
        item.group.forward_write.payload48_bits === item.group.reverse_write.payload48_bits).length,
      accepted_group_count: accepted.length,
    },
  };
}

function selfTest() {
  const bits = value => {
    const bytes = Buffer.allocUnsafe(4);
    bytes.writeFloatLE(value, 0);
    return `0x${bytes.readUInt32LE(0).toString(16).padStart(8, '0')}`;
  };
  const blockBits = lane1 => Array.from({ length: 12 }, (_, index) =>
    bits(index === 1 ? lane1 : 0).slice(2)).join('');
  const rows = [];
  for (let update = 1; update <= 2; update++) {
    SITES.forEach((site, index) => rows.push({
      seq: String((update - 1) * 4 + index + 1),
      broad_update_index: String(update), owner: '0x1', selected: '0x2', site,
      overflow: '0', pointer_chain_valid: '1', source_transform_valid: '1',
      selected48_valid: '1', payload48_valid: '1',
      source_b1: bits(update), source_extra_valid: '1', source_extra_b: bits(0.25),
      source_selector_equal: '1', root_position_valid: '1', root_z_b: bits(update + 4),
      selected48_bits: blockBits(update + 1), payload48_bits: blockBits(update + 2),
    }));
  }
  const result = analyzeRows(rows, 'self-test');
  if (result.structure.accepted_complete_ordered_finite_groups !== 2 ||
      result.vertical_stage_gaps.root_minus_source.median !== 3.75) {
    throw new Error('valid-sequence self-test failed');
  }
  const duplicate = analyzeRows([...rows, { ...rows[0], seq: '9' }], 'duplicate');
  if (duplicate.structure.structural_pass ||
      duplicate.structure.duplicate_site_group_observations !== 1) {
    throw new Error('duplicate rejection self-test failed');
  }
  const incomplete = analyzeRows(rows.slice(0, -1), 'incomplete');
  if (incomplete.structure.structural_pass || incomplete.structure.incomplete_group_count !== 1) {
    throw new Error('incomplete rejection self-test failed');
  }
  const reorderedRows = rows.map(row => ({ ...row }));
  reorderedRows[1].seq = '0';
  const reordered = analyzeRows(reorderedRows, 'reordered');
  if (reordered.structure.structural_pass || reordered.structure.out_of_order_group_count !== 1) {
    throw new Error('ordering rejection self-test failed');
  }
  const invalidRows = rows.map(row => ({ ...row }));
  invalidRows[0].source_transform_valid = '0';
  const invalid = analyzeRows(invalidRows, 'invalid-read');
  if (invalid.structure.structural_pass || invalid.structure.invalid_source_row_count !== 1) {
    throw new Error('invalid-read rejection self-test failed');
  }
  process.stdout.write('self-test: pass\n');
}

const args = process.argv.slice(2);
if (args.length === 1 && args[0] === '--self-test') {
  selfTest();
} else {
  const [output, ...inputs] = args;
  if (!output || inputs.length < 2 || inputs.length % 2 !== 0) {
    throw new Error('usage: node analyze_proxy_boundary.mjs OUT CSV LABEL [CSV LABEL ...]');
  }
  const captures = [];
  for (let index = 0; index < inputs.length; index += 2) {
    captures.push(analyzeRows(parseCsv(inputs[index]), inputs[index + 1]));
  }
  fs.writeFileSync(output, JSON.stringify({
    schema_version: 1,
    investigation_id: 'I-0013',
    captures,
    interpretation_limit: 'Lane 1 is treated only as the statically traced vertical component. Stage gaps localize the first observed divergence but do not by themselves establish collision ownership or causality.',
  }, null, 2) + '\n');
}
