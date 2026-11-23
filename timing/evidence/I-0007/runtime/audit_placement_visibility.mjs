import fs from 'node:fs';
import path from 'node:path';
import crypto from 'node:crypto';
import assert from 'node:assert/strict';
import {fileURLToPath} from 'node:url';
const here=path.dirname(fileURLToPath(import.meta.url));
const read=p=>fs.readFileSync(path.join(here,p),'utf8');
const json=p=>JSON.parse(read(p));
const hash=p=>crypto.createHash('sha256').update(fs.readFileSync(path.join(here,p))).digest('hex').toUpperCase();
const summary=json('analysis/elapsed-20260920-paired/transient-summary.json');
const rows=json('analysis/elapsed-20260920-paired/transient-windows.json');
for(const [name,h] of Object.entries(summary.raw_sha256))assert.equal(hash('sessions/20260920T020120Z-pid2428/'+name),h);
const asmPath='../static/source-scale/DS3-1152-F-005C72B0.asm.txt';
const ins=read(asmPath).split(/\r?\n/).flatMap(l=>{const m=l.match(/^([0-9a-f]+)\s+(\w+)\s+(.+)$/);return m?[{address:parseInt(m[1],16),op:m[2],text:m[3]}]:[];});
const edges=ins.filter(i=>i.op.startsWith('J')).map(i=>({...i,target:parseInt(i.text.match(/0x([0-9a-f]+)/)[1],16)}));
const lo=0x1405c73d8,hi=0x1405c7435;
const ingress=edges.filter(i=>i.address<lo&&i.target>=lo&&i.target<=hi);
assert.deepEqual(ingress.map(i=>[i.address,i.target]),[[0x1405c734e,hi],[0x1405c7378,0x1405c740c],[0x1405c7385,0x1405c740c]]);
const calls=ins.filter(i=>i.address>=0x1405c734c&&i.address<hi&&i.op==='CALL');
assert.deepEqual(calls.map(i=>i.address),[0x1405c7357,0x1405c73a6]);
const native=(s,a,r)=>!a?s:r?(s>.5?Math.fround(s*Math.fround(.8)):.5):(s<1?Math.fround(s*Math.fround(1.2)):1);
const reports=[1,2].map(trial=>{
 const r=rows.filter(r=>r.trial===trial);
 const differences=r.map(x=>({update:x.update,existing_intermediate:x.native_scale_after,exit_only_intermediate:native(x.gate_scale_before,x.gate_active,x.compare_seen&&x.raw<1),candidate_exit:x.scale_after}));
 return {trial,rows:r.length,existing_native_to_candidate_changes:differences.filter(x=>x.existing_intermediate!==x.candidate_exit).length,
  exit_only_native_to_candidate_changes:differences.filter(x=>x.exit_only_intermediate!==x.candidate_exit).length,
  placement_changes:differences.filter(x=>x.existing_intermediate!==x.exit_only_intermediate)};
});
assert.equal(reports[0].placement_changes.length,0);assert.equal(reports[1].placement_changes.length,81);
const result={source_checkpoint:'138cb59',raw_files_verified:Object.keys(summary.raw_sha256).length,assembly_sha256:hash(asmPath),
 tail_ingress:ingress,calls_after_gate_before_exit:calls,reports,
 decision:'Retain the existing compare correction and experimental exit recurrence placement for the next movement discriminator; defer physical consolidation.',
 reasoning:'The call-free tail has bypass ingress at recovery and inactive exit. A gate-level replacement must preserve two calls and their ordering. An exit-only write adds 81 modeled intermediate scalar differences without reducing observations. Neither relocation proves concurrent or exception visibility equivalence.',
 limits:'Existing placement also exposes native intermediate values before candidate output (38 retained165 intervals). Retention is a change-minimization choice, not proof of concurrency safety. Counterfactual exit-only intermediates are modeled; external readers and all incoming branches are not exhaustively known. No code replacement or game launch.'};
fs.writeFileSync(path.join(here,'analysis/single-write/placement-visibility.json'),JSON.stringify(result,null,2)+'\n');
console.log(JSON.stringify({...result,reports:reports.map(({placement_changes,...r})=>({...r,placement_changes:placement_changes.length}))},null,2));
