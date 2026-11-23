import fs from 'node:fs';
import path from 'node:path';
import assert from 'node:assert/strict';
import crypto from 'node:crypto';
import {fileURLToPath} from 'node:url';
const here=path.dirname(fileURLToPath(import.meta.url)),dir=path.join(here,'sessions/20260920T020120Z-pid2428'),out=path.join(here,'analysis/elapsed-20260920-paired');
const read=p=>fs.readFileSync(p,'utf8');
const csv=n=>{const [h,...ls]=read(path.join(dir,n)).trim().split(/\r?\n/),k=h.split(',');return ls.map(l=>Object.fromEntries(l.split(',').map((v,i)=>[k[i],v])));};
const summary=JSON.parse(read(path.join(out,'transient-summary.json'))),rows=JSON.parse(read(path.join(out,'transient-windows.json')));
assert(summary.timing_restored&&summary.exact_arithmetic_and_commit_checks);
for(const [n,h] of Object.entries(summary.raw_sha256))assert.equal(crypto.createHash('sha256').update(fs.readFileSync(path.join(dir,n))).digest('hex').toUpperCase(),h);
const u=csv('locomotion-updates.csv'),map=new Map(u.map(r=>[Number(r.update_index),r]));
const reports=[];
for(const trial of [1,2]){
 const w=rows.filter(r=>r.trial===trial),updates=u.filter(r=>Number(r.trial)===trial),origin=Number(updates[0].accumulated_delta_before);
 const recoveries=[];
 for(let i=0;i<w.length;i++){
  if(w[i].gate_scale_before!==.5||w[i].reduced||!w[i].gate_active)continue;
  const segment=[];
  for(let j=i;j<w.length&&!w[j].reduced&&w[j].gate_active;j++){segment.push(w[j]);if(w[j].scale_after===1)break;}
  if(segment.at(-1)?.scale_after!==1)continue;
  let time=0,cross=null,consumedCross=null;
  for(const r of segment){time+=r.gate_module_1c;if(cross===null&&r.scale_after>=1)cross=time;if(consumedCross===null&&r.gate_scale_before>=1)consumedCross=time-r.gate_module_1c;}
  const referenceWindow=5/60;let remaining=referenceWindow,unitSourceIntegral=0;
  for(const r of segment){const dt=Math.min(remaining,r.gate_module_1c);unitSourceIntegral+=Math.min(1,r.gate_scale_before)*dt;remaining-=dt;if(remaining<=0)break;}
  // Once scalar has settled to1, extend only the conditional unit-source model.
  if(remaining>0)unitSourceIntegral+=remaining;
  recoveries.push({first:segment[0].update,rows:segment.length,settle_elapsed_s:time,first_output_ge1_s:cross,first_source_factor_ge1_s:consumedCross,
   conditional_unit_source_integral_5_over60:unitSourceIntegral,
   keys:[...new Set(segment.map(r=>map.get(r.update).before_keyboard_mask))],animations:[...new Set(segment.map(r=>map.get(r.update).before_animation_id))],
   source_attenuation_time_s:segment.reduce((s,r)=>s+(1-Math.min(1,r.gate_scale_before))*r.gate_module_1c,0)});
 }
 let tailStart=updates.length;
 while(tailStart>0&&updates[tailStart-1].before_keyboard_mask==='0x0')tailStart--;
 const tail=updates.slice(tailStart),pos=r=>['x','z','y'].map(k=>Number(r['before_'+k]));
 const tailPath=tail.slice(1).reduce((s,r,i)=>s+Math.hypot(...pos(r).map((v,k)=>v-pos(tail[i])[k])),0);
 const stationarySuffix=[];for(let i=tail.length-1;i>=0;i--){if(Math.hypot(...pos(tail[i]).map((v,k)=>v-pos(tail.at(-1))[k]))>1e-5)break;stationarySuffix.push(tail[i]);}
 reports.push({trial,intervals:w.length,native_output_differences:w.filter(r=>r.scale_after!==r.native_scale_after).length,recoveries,
  released_tail:{samples:tail.length,start_s:tail.length?Number(tail[0].accumulated_delta_before)-origin:null,elapsed_s:tail.reduce((s,r)=>s+Number(r.delta),0),path_length:tailPath,stationary_suffix_s:stationarySuffix.reduce((s,r)=>s+Number(r.delta),0)}});
}
assert.equal(reports[0].native_output_differences,0);
assert.equal(reports[0].recoveries.length,1);assert.equal(reports[1].recoveries.length,1);
const a=reports[0].recoveries[0],b=reports[1].recoveries[0];
const result={session:path.basename(dir),reports,local_gate:{native60_exact:true,first_output_ge1_difference_s:b.first_output_ge1_s-a.first_output_ge1_s,settle_difference_s:b.settle_elapsed_s-a.settle_elapsed_s,within_one_165_update:Math.abs(b.settle_elapsed_s-a.settle_elapsed_s)<=1/165},
 conditional_unit_source_difference_percent:100*(b.conditional_unit_source_integral_5_over60/a.conditional_unit_source_integral_5_over60-1),
 limits:'This validates the bounded recurrence clock/60 arithmetic gate, not equal full-trajectory displacement. Interpolation changes inter-sample attenuation area; differing source/input trajectories prevent a whole-game claim. Native60 has no idle tail;165 release/settling is separately measured.'};
fs.writeFileSync(path.join(out,'elapsed-pair-evaluation.json'),JSON.stringify(result,null,2)+'\n');console.log(JSON.stringify(result,null,2));
