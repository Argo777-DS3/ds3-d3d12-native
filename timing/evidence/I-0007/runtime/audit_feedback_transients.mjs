// Offline analysis of the already reconstructed recurrence, not a game patch.
import fs from 'node:fs';
import path from 'node:path';
import assert from 'node:assert/strict';
import crypto from 'node:crypto';
import {fileURLToPath} from 'node:url';
const here=path.dirname(fileURLToPath(import.meta.url));
const f=Math.fround;
const next=(s,reduce)=>reduce?(s>0.5?f(s*f(0.8)):0.5):(s<1?f(s*f(1.2)):1);
const hash=p=>crypto.createHash('sha256').update(fs.readFileSync(p)).digest('hex').toUpperCase();
const read=p=>JSON.parse(fs.readFileSync(p,'utf8'));
const scenarios=[];
for(const reduce of [true,false]){
  const start=reduce?1:0.5, endpoint=reduce?0.5:1;
  const states=[start];let s=start;
  for(let i=0;i<20;i++){s=next(s,reduce);states.push(s);if(s===endpoint)break;}
  assert.equal(s,endpoint);
  const steps=states.length-1;
  scenarios.push({branch:reduce?'reduce':'recover',states,steps_to_reset:steps,
    reset_ms_at_one_eligible_call_per_update:{target60:steps/60*1000,target165:steps/165*1000},
    assumption:'Same eligible branch at every update; timing is a model, not measured gameplay.'});
}
assert.deepEqual(scenarios.map(x=>x.steps_to_reset),[5,5]);
const captures=[];
for(const session of ['20260919T184047Z-pid3804','20260919T185120Z-pid3804','20260919T185858Z-pid3804']){
  const file=path.join(here,'reconciliation-20260920',session,'causal-patch-windows.json');
  const rows=read(file);
  const comparisons=rows.filter(r=>r.compare_seen);
  for(const r of comparisons)assert.equal(next(r.scale_before,r.reduced),r.scale_after);
  const nonunit=rows.filter(r=>r.scale_after!==1);
  const firstReduction=rows.findIndex(r=>r.measured&&r.reduced);
  captures.push({session,input_sha256:hash(file),windows:rows.length,comparisons:comparisons.length,
    nonunit_rows:nonunit.length,nonunit_measured:nonunit.filter(r=>r.measured).length,
    measured_reductions:rows.filter(r=>r.measured&&r.reduced).length,
    observed_transient:firstReduction<0?[]:rows.slice(Math.max(0,firstReduction-1),firstReduction+5)});
}
const asm=path.resolve(here,'../static/source-scale/DS3-1152-F-005C72B0.asm.txt');
const text=fs.readFileSync(asm,'utf8');
assert(text.includes('1405c73ee  MULSS')&&text.includes('1405c7419  MULSS'));
const report={scope:'Continuation of I-0007, cycle 11/12; existing exports and captures only.',
  assembly_sha256:hash(asm),reduction_rva:'005C73EE',recovery_rva:'005C7419',scenarios,captures,
  conclusion:'The native gains are per eligible invocation, with one-step under/overshoot. Their constant-branch elapsed response depends on invocation cadence. No residual gameplay failure is established by this model.',
  correction_status:'Not implemented: gain exponentiation alone changes the conditional-reset/overshoot trajectory and does not establish the elapsed interval between eligible recurrence calls. Preserve the accepted distance correction.',
  missing_discriminator:'Matched non-equilibrium recurrence transitions with entry gate, invocation identity, timing and displacement; stable scalar=1 windows cannot decide this question.'};
const out=path.join(here,'reconciliation-20260920','feedback-transient-audit.json');
fs.writeFileSync(out,JSON.stringify(report,null,2)+'\n');
console.log(JSON.stringify({scenarios,captures:captures.map(({observed_transient,...r})=>r),conclusion:report.conclusion},null,2));
