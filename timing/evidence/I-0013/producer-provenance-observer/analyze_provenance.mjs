import fs from 'node:fs';
import { parseCsv } from '../stage-boundary-observer/analyze_stage_boundary.mjs';

function uniqueHex(rows, key) {
  return [...new Set(rows.map(row => row[key].toUpperCase()))].sort();
}

export function analyzeProvenance(rows) {
  const keys = ['owner_vtable', 'owner_e0_target', 'actor', 'actor_vtable',
    'actor_178_target'];
  const missing = keys.filter(key => rows.length && !(key in rows[0]));
  const valid = rows.filter(row => row.valid === '1');
  const rvaKeys = ['owner_vtable', 'owner_e0_target', 'actor_vtable',
    'actor_178_target'];
  const finitePointers = valid.every(row => keys.every(key => {
    const value = Number.parseInt(row[key], 16);
    return Number.isSafeInteger(value) && value > 0;
  }));
  const boundedRvas = valid.every(row => rvaKeys.every(key => {
    const value = Number.parseInt(row[key], 16);
    return value > 0 && value < 0x10000000;
  }));
  const unique = Object.fromEntries(keys.map(key => [key, uniqueHex(valid, key)]));
  const stableTargets = unique.owner_e0_target.length === 1 &&
    unique.actor_178_target.length === 1 && unique.actor.length === 1;
  return {
    structural_pass: rows.length >= 2 && valid.length === rows.length &&
      missing.length === 0 && finitePointers && boundedRvas && stableTargets,
    row_count: rows.length,
    valid_row_count: valid.length,
    missing_columns: missing,
    finite_nonzero_pointers: finitePointers,
    bounded_module_rvas: boundedRvas,
    stable_targets_and_actor: stableTargets,
    unique,
    limit: 'Resolves concrete target RVAs and identity only; it does not identify individual contribution values or semantics.',
  };
}

if (process.argv[1]?.endsWith('analyze_provenance.mjs')) {
  if (process.argv.length !== 4) throw new Error('usage: node analyze_provenance.mjs INPUT.csv OUTPUT.json');
  const result = analyzeProvenance(parseCsv(fs.readFileSync(process.argv[2], 'utf8')));
  fs.writeFileSync(process.argv[3], `${JSON.stringify(result, null, 2)}\n`);
  if (!result.structural_pass) process.exitCode = 2;
}
