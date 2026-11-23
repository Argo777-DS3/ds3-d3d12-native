import assert from 'node:assert/strict';
import { parseCsv } from '../stage-boundary-observer/analyze_stage_boundary.mjs';
import { analyzeCausality } from './analyze_aggregator_causality.mjs';

function matrixBits(x, z, y) {
  return [1,0,0,x,0,1,0,z,0,0,1,y].map(value => {
    const bytes = Buffer.allocUnsafe(4);
    bytes.writeFloatLE(value);
    return bytes.readUInt32LE().toString(16).padStart(8, '0').toUpperCase();
  }).join('');
}

const header = ['qpc_entry','qpc_exit','sequence','caller_return','owner','movement','output','source_before_x','source_before_z','source_before_y','source_before_w','source_after_x','source_after_z','source_after_y','source_after_w','source_extra','selector_a','selector_b','carried','mode_a','mode_b','vertical_filter','override_gate','local_predicate','cache_before_x','cache_before_z','cache_before_y','final_x','final_z','final_y','cache_before48_bits','final_output48_bits','valid'].join(',');
const values = [{x:0,z:1,c:0.25},{x:1,z:2,c:-0.5},{x:2,z:3,c:0.125}];
const rows = values.map((value, index) => {
  const final = Math.fround(Math.fround(value.z) + Math.fround(value.c));
  const prior = index ? Math.fround(values[index - 1].z + values[index - 1].c) : final;
  return [1000+index*100,1010+index*100,index+1,'0x5CAA80','0x1000','0x2000','0x3000',value.x,value.z,0,1,value.x,value.z,0,1,0,1,2,value.c,0,0,0,0,1,0,prior,0,value.x,final,0,matrixBits(index ? values[index-1].x : value.x,prior,0),matrixBits(value.x,final,0),1].join(',');
});
const result = analyzeCausality(parseCsv(`${header}\n${rows.join('\n')}\n`));
assert.equal(result.boundary_pass, true);
assert.equal(result.exact_vertical_source_plus_carried_rows, 3);
assert.equal(result.cache_equals_previous_final_rows, 2);
assert.equal(result.vertical_equation_residual_rms, 0);

const bad = parseCsv(`${header}\n${rows.join('\n')}\n`);
bad[1].final_output48_bits = matrixBits(1, 99, 0);
assert.equal(analyzeCausality(bad).boundary_pass, false);

process.stdout.write(`${JSON.stringify({
  schema_version: 1,
  status: 'pass',
  cases: ['bit_exact_source_plus_carried', 'previous_output_cache_transport',
    'horizontal_preservation', 'bad_equation_rejected'],
  limit: 'Synthetic causal-boundary test only; no live behavior is established.'
}, null, 2)}\n`);
