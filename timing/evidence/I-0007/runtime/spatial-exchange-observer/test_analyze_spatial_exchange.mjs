import assert from 'node:assert/strict';
import {analyzeRows} from './analyze_spatial_exchange.mjs';

const bits='00000000'.repeat(12);
const base={qpc:'100',tid:'7',trial:'1',marker_id:'1',broad_update_index:'11',
  invocation_index:'1',shared_context:'0x5000',transform:'0x6000',owner:'0x1000',
  descriptor:'0x2000',descriptor_base:'0x3000',descriptor_offset:'64',selected:'0x3040',
  module28_object:'0x4000',pointer_chain_valid:'1',selected48_valid:'1',payload48_valid:'0',
  overflow:'0',elapsed_b0:'0x3C2F2E49',elapsed_b1:'0x00000000',elapsed_b2:'0x00000000',
  elapsed_b3:'0x00000000',selected48_bits:bits,payload48_bits:bits};
const row=(seq,site,extra={})=>({...base,seq:String(seq),site,...extra});
const valid=[row(1,'forward_entry'),row(2,'reverse_entry'),
  row(3,'forward_write',{descriptor:'0x0',descriptor_base:'0x0',selected48_valid:'0',payload48_valid:'1'}),
  row(4,'reverse_write',{descriptor:'0x0',descriptor_base:'0x0',selected48_valid:'0',payload48_valid:'1'})];
const pass=analyzeRows(valid);
assert.equal(pass.structural_pass,true);
assert.deepEqual(pass.direction_pairs,{forward:1,reverse:1});
const mismatch=valid.map(x=>({...x})); mismatch[2].selected='0x9999';
assert.equal(analyzeRows(mismatch).structural_pass,false);
const overflow=valid.map(x=>({...x})); overflow[3].overflow='1';
assert.equal(analyzeRows(overflow).structural_pass,false);
const oneDirection=analyzeRows(valid.slice(0,1).concat(valid.slice(2,3)));
assert.equal(oneDirection.structural_pass,false);
const elapsed=valid.map(x=>({...x})); elapsed[2].elapsed_b0='0x00000001';
assert.equal(analyzeRows(elapsed).structural_pass,false);
console.log(JSON.stringify({status:'pass',checks:['valid-bidirectional','identity-mismatch','overflow','missing-direction','elapsed-mismatch']},null,2));
