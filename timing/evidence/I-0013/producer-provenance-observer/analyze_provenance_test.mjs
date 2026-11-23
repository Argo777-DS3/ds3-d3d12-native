import assert from 'node:assert/strict';
import { parseCsv } from '../stage-boundary-observer/analyze_stage_boundary.mjs';
import { analyzeProvenance } from './analyze_provenance.mjs';

const header = 'owner_vtable,owner_e0_target,actor,actor_vtable,actor_178_target,valid';
const rows = ['0x49C1000,0x123456,0x7FF000,0x49D2000,0x234567,1',
  '0x49C1000,0x123456,0x7FF000,0x49D2000,0x234567,1'];
const parsed = parseCsv(`${header}\n${rows.join('\n')}\n`);
assert.equal(analyzeProvenance(parsed).structural_pass, true);
const changed = structuredClone(parsed); changed[1].owner_e0_target = '0x123457';
assert.equal(analyzeProvenance(changed).structural_pass, false);
const invalid = structuredClone(parsed); invalid[1].valid = '0';
assert.equal(analyzeProvenance(invalid).structural_pass, false);
process.stdout.write(`${JSON.stringify({schema_version:1,status:'pass',cases:['stable_two_targets','module_relative_bounds','target_change_rejected','invalid_row_rejected'],limit:'Synthetic provenance test only.'},null,2)}\n`);
