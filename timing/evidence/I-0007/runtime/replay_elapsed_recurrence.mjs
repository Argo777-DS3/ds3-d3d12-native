// Offline prototype only. No injection, native memory writes or gameplay claim.
import fs from 'node:fs';
import path from 'node:path';
import assert from 'node:assert/strict';
import {fileURLToPath} from 'node:url';
const here=path.dirname(fileURLToPath(import.meta.url)),f=Math.fround;
const step=(s,reduce)=>reduce?(s>.5?f(s*f(.8)):.5):(s<1?f(s*f(1.2)):1);
const T=1/60,epsilon=T*1e-5;
class ElapsedRecurrence {
  constructor(s){this.s=s;this.credit=0;}
  advance(active,reduce,dt){
    assert(Number.isFinite(dt)&&dt>0&&dt<=.25,'Bounded valid caller delta');
    if(!active)return {s:this.s,ticks:0};
    this.credit+=dt;let ticks=0;
    while(this.credit+epsilon>=T){this.s=step(this.s,reduce);this.credit=Math.max(0,this.credit-T);ticks++;assert(ticks<=16);}
    return {s:this.s,ticks};
  }
}
// Preserve every reference branch/reset combination including skipped invocations.
for(let code=0;code<3**8;code++){
  let n=code,s=.5;const p=new ElapsedRecurrence(s);
  for(let i=0;i<8;i++){const b=n%3;n=Math.floor(n/3);if(b)s=step(s,b===1);assert.equal(p.advance(b!==0,b===1,T).s,s);}
}
for(const hz of [60,165]){
  const p=new ElapsedRecurrence(.5);let ticks=0;
  for(let i=0;i<hz;i++)ticks+=p.advance(true,false,1/hz).ticks;
  assert.equal(ticks,60);assert.equal(p.s,1);
}
const gap=new ElapsedRecurrence(.5);gap.advance(true,false,T/2);
for(let i=0;i<100;i++)assert.equal(gap.advance(false,false,T).s,.5);
assert.equal(gap.advance(true,false,T/2).s,step(.5,false));
assert.throws(()=>gap.advance(true,false,0));assert.throws(()=>gap.advance(true,false,NaN));
const reports=[];
for(const hz of [60,165]){
  const dir=path.join(here,`analysis/transient-20260920-${hz}`);
  const rows=JSON.parse(fs.readFileSync(path.join(dir,'transient-windows.json'),'utf8'));
  const p=new ElapsedRecurrence(rows[0].gate_scale_before);let changed=0,ticks=0;const differences=[];
  for(const r of rows){const a=p.advance(r.gate_active,!!r.reduced,r.gate_module_1c);ticks+=a.ticks;
    if(a.s!==r.scale_after){changed++;differences.push({update:r.update,native:r.scale_after,shadow:a.s,ticks:a.ticks});}
  }
  if(hz===60)assert.equal(changed,0,'All measured60 scalar outputs preserved');
  reports.push({hz,rows:rows.length,changed_outputs:changed,virtual_ticks:ticks,first_differences:differences.slice(0,12),final_native:rows.at(-1).scale_after,final_shadow:p.s});
}
const result={status:'OFFLINE PROTOTYPE; no game modification',reference_seconds:T,roundoff_tolerance_seconds:epsilon,
  rule:'Accumulate only active caller time; apply the original float32 recurrence once per1/60 active seconds, retaining credit across skipped invocations. Each virtual tick uses the current branch. Consume current state before the next recurrence, as in the native caller.',
  checks:['6561 eight-invocation branch/skip sequences match native at60','60 virtual ticks per1s at60/165','inactive gaps preserve state/credit','invalid delta rejected','all480 real60 outputs match exactly'],reports,
  limitations:['Virtual ticks preserve the discrete60 reference, not a unique continuous design intent.','Branches in this replay are frozen recorded observations; changed scalar can change future movement and feedback in a real closed loop.','Branch changes inside one update are unobserved; no catch-up across unknown caller/gate history is justified.','Caller module1C is directly paired at165 only; full variable-time/eligibility semantics are not established.','Per-update source consumption quantizes effective transition times; no exact displacement invariance is asserted.','No live prototype or acceptance follows from this replay alone.']};
fs.writeFileSync(path.join(here,'analysis/transient-20260920-165/elapsed-recurrence-shadow.json'),JSON.stringify(result,null,2)+'\n');
console.log(JSON.stringify(result,null,2));
