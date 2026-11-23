import fs from 'node:fs';
import path from 'node:path';
import crypto from 'node:crypto';
import assert from 'node:assert/strict';
import {fileURLToPath} from 'node:url';
import {ContinuousRecurrence} from './continuous_recurrence.mjs';

// Offline intervention-placement assessment, not an injected replacement.
const here=path.dirname(fileURLToPath(import.meta.url));
const source=path.join(here,'analysis/elapsed-20260920-paired');
const out=path.join(here,'analysis/single-write');
const read=p=>JSON.parse(fs.readFileSync(p,'utf8'));
const hash=p=>crypto.createHash('sha256').update(fs.readFileSync(p)).digest('hex').toUpperCase();
const summary=read(path.join(source,'transient-summary.json'));
const rows=read(path.join(source,'transient-windows.json'));
const session='20260920T020120Z-pid2428';
for(const [name,h] of Object.entries(summary.raw_sha256))
  assert.equal(hash(path.join(here,'sessions',session,name)),h);
assert(summary.timing_restored && summary.exact_arithmetic_and_commit_checks);
const f=Math.fround;
const native=(s,active,reduce)=>!active?s:reduce?(s>.5?f(s*f(.8)):.5):(s<1?f(s*f(1.2)):1);
const normalize=(raw,dt)=>f(raw*f(f(1/60)/dt));

// Retain separate raw-native guard and normalized desired branch. A unified
// exit write must not validate raw execution against the corrected native law.
function complete(model, before, active, compared, raw, distanceDt, recurrenceDt, nativeExit) {
  assert.equal(model.value,before);
  assert(!compared || active);
  assert(!compared || Number.isFinite(raw)&&raw>=0&&Number.isFinite(distanceDt)&&distanceDt>0);
  const rawReduce=compared && raw<1;
  const corrected=compared?normalize(raw,distanceDt):null;
  assert(corrected===null || Number.isFinite(corrected));
  const desiredReduce=compared && corrected<1;
  assert.equal(nativeExit,native(before,active,rawReduce),'raw-native exit guard');
  return {output:model.advance(active,desiredReduce,recurrenceDt),rawReduce,desiredReduce,corrected};
}

const reports=[];
for(const trial of [1,2]) {
  const selected=rows.filter(r=>r.trial===trial), model=new ContinuousRecurrence(selected[0].gate_scale_before);
  let branchDifferences=0,temporaryValueDifferences=0;
  for(const r of selected) {
    const rawReduce=r.compare_seen&&r.raw<1;
    const counterfactualExit=native(r.gate_scale_before,r.gate_active,rawReduce);
    const result=complete(model,r.gate_scale_before,r.gate_active,r.compare_seen,r.raw,
      r.governing_delta,r.gate_module_1c,counterfactualExit);
    assert.equal(result.desiredReduce,!!r.reduced);
    if(r.compare_seen)assert.equal(result.corrected,r.corrected);
    assert.equal(result.output,r.scale_after);
    assert.equal(native(r.gate_scale_before,r.gate_active,result.desiredReduce),r.native_scale_after);
    if(result.rawReduce!==result.desiredReduce)branchDifferences++;
    if(counterfactualExit!==r.native_scale_after)temporaryValueDifferences++;
  }
  reports.push({trial,rows:selected.length,exact_candidate_outputs:selected.length,
    raw_vs_corrected_branch_differences:branchDifferences,
    counterfactual_temporary_scalar_differences:temporaryValueDifferences});
}

// Exercise differences even if the retained scene does not cross the threshold.
let synthetic=0,guardRejections=0;
for(const before of [.4096,.5,.512,.8,1,1.0368].map(f))
for(const dt of [1/60,1/165,.04].map(f))
for(const raw of [0,.4,.99999994,1,1.00000012,2].map(f))
for(const branch of ['inactive','bypass','compare']) {
  const active=branch!=='inactive', compared=branch==='compare';
  const actual=native(before,active,compared&&raw<1);
  const a=new ContinuousRecurrence(before), b=new ContinuousRecurrence(before);
  const r=complete(a,before,active,compared,raw,dt,dt,actual);
  assert.equal(r.output,b.advance(active,compared&&normalize(raw,dt)<1,dt));
  const pristine=new ContinuousRecurrence(before);
  assert.throws(()=>complete(pristine,before,active,compared,raw,dt,dt,NaN),/raw-native exit guard/);
  assert.equal(pristine.value,before);
  synthetic++;guardRejections++;
}
// float32(.05) exceeds the candidate's double .05 bound. Preserve rejection;
// do not silently widen the established prototype's accepted delta domain.
assert.throws(()=>new ContinuousRecurrence(1).advance(true,false,f(.05)),/Invalid caller delta/);

// Check the bounded interval after the distance comparison: only scalar writes,
// no call before the common exit. This says nothing about concurrent readers.
const asm=path.join(here,'../static/source-scale/DS3-1152-F-005C72B0.asm.txt');
const body=fs.readFileSync(asm,'utf8').split(/\r?\n/).filter(l=>{
  const m=l.match(/^([0-9a-f]+)\s/);if(!m)return false;
  const address=parseInt(m[1],16);return address>=0x1405c73d8&&address<0x1405c7435;
});
assert.equal(body.length,17);
assert(!body.some(l=>/\bCALL\b/.test(l)));
const stores=body.filter(l=>/\b(?:MOVSS|MOV)\s+(?:MOVSS|MOV)\s+dword ptr/.test(l));
assert.equal(stores.length,4);
assert(stores.every(l=>/dword ptr \[RBX \+ 0x25c\]/.test(l)));
const result={session,reports,synthetic_cases:synthetic,invalid_native_exit_rejections:guardRejections,
  static_interval:{assembly_sha256:hash(asm),instructions:body.length,scalar_stores:stores.length,calls:0},
  conclusion:'One corrected scalar write at the common producer exit reproduces retained candidate outputs, provided the entry/gate and distance observations remain available and the raw-native exit guard is adapted.',
  limits:'Counterfactual raw-native exit values are modeled, not acquired. No consolidated instrument built or injected. Observation sites remain; one write is not one hook. Concurrent visibility of temporary native scalar, exception paths, register differences, complete replacement side effects and whole-motion acceptance remain unqualified. This does not change the experimental recurrence law or resolve its integration-area limitation.'};
fs.mkdirSync(out,{recursive:true});
fs.writeFileSync(path.join(out,'summary.json'),JSON.stringify(result,null,2)+'\n');
console.log(JSON.stringify(result,null,2));
