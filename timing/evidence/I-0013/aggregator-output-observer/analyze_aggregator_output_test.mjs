import assert from 'node:assert/strict';
import { parseCsv } from '../stage-boundary-observer/analyze_stage_boundary.mjs';
import { analyzeAggregator } from './analyze_aggregator_output.mjs';

function bits(z) {
  return [1,0,0,0,0,1,0,z,0,0,1,0].map(v => {
    const b = Buffer.allocUnsafe(4); b.writeFloatLE(v);
    return b.readUInt32LE().toString(16).padStart(8, '0');
  }).join('');
}
const header = ['qpc_entry','qpc_exit','sequence','caller_return','owner','movement','output','source_before_x','source_before_z','source_before_y','source_before_w','source_after_x','source_after_z','source_after_y','source_after_w','source_extra','selector_a','selector_b','carried','mode_a','mode_b','vertical_filter','override_gate','local_predicate','cache_before_x','cache_before_z','cache_before_y','final_x','final_z','final_y','cache_before48_bits','final_output48_bits','valid'].join(',');
const rows = [0,1,2,1,3].map((z,i) => [10000000+i*166666,10000010+i*166666,i+1,['0x5C6977','0x5C6F47','0x5CAA80'][i%3],'0x1000','0x2000','0x3000',i,z,0,1,i,z,0,1,0,1,2,i ? 1 : 0,0,1,0,0,i ? 1 : 0,0,z,0,0,z,0,bits(z),bits(z),1].join(','));
const parsed = parseCsv(`${header}\n${rows.join('\n')}\n`);
const result = analyzeAggregator(parsed);
assert.equal(result.structural_pass, true);
assert.equal(result.local_predicate_active_rows, 4);
assert.equal(result.final_opposing_vertical_steps.count, 1);
const badCaller = structuredClone(parsed); badCaller[1].caller_return = '0xDEADBEEF';
assert.equal(analyzeAggregator(badCaller).structural_pass, false);
const badPredicate = structuredClone(parsed); badPredicate[2].local_predicate = '0';
assert.equal(analyzeAggregator(badPredicate).structural_pass, false);
process.stdout.write(`${JSON.stringify({schema_version:1,status:'pass',cases:['three_allowed_callers','identity_and_sequence','predicate_reconstruction','source_final_cache_opposing_steps','bad_caller_rejected','bad_predicate_rejected'],limit:'Synthetic analyzer test only; no live-game behavior is established.'}, null, 2)}\n`);
