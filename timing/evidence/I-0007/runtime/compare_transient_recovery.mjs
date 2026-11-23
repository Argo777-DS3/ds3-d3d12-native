import fs from 'node:fs';
import path from 'node:path';
import crypto from 'node:crypto';
import assert from 'node:assert/strict';
import {fileURLToPath} from 'node:url';
const here=path.dirname(fileURLToPath(import.meta.url));
const read=p=>fs.readFileSync(p,'utf8');
const csv=p=>{const [h,...ls]=read(p).trim().split(/\r?\n/),keys=h.split(',');return ls.map(l=>Object.fromEntries(l.split(',').map((v,i)=>[keys[i],v])));};
const reports=[];
for(const [hz,name] of [[60,'20260920T011956Z-pid19144'],[165,'20260920T013846Z-pid6976']]){
  const dir=path.join(here,'sessions',name),out=path.join(here,`analysis/transient-20260920-${hz}`);
  const summary=JSON.parse(read(path.join(out,'transient-summary.json')));
  assert(summary.timing_restored&&summary.exact_arithmetic_and_commit_checks);
  for(const [n,h] of Object.entries(summary.raw_sha256))assert.equal(crypto.createHash('sha256').update(fs.readFileSync(path.join(dir,n))).digest('hex').toUpperCase(),h);
  const w=JSON.parse(read(path.join(out,'transient-windows.json'))),u=csv(path.join(dir,'locomotion-updates.csv'));
  const map=new Map(u.map(r=>[Number(r.update_index),r]));const origin=Number(u[0].accumulated_delta_before);
  const info=Object.fromEntries(read(path.join(dir,'session-info.txt')).trim().split(/\r?\n/).map(l=>{const i=l.indexOf('=');return[l.slice(0,i),l.slice(i+1)];}));
  const gates=new Map(csv(path.join(dir,'causal-distance-patch.csv')).filter(r=>BigInt(r.rip_at_trap)-BigInt(info.image_base)===0x5c734cn).map(r=>[Number(r.broad_update_index),r]));
  const runs=[];
  for(const r of w){
    const mode=!r.gate_active?'skip':r.reduced?'reduce':'recover';let s=runs.at(-1);
    if(!s||s.mode!==mode){s={mode,first:r.update,last:r.update,start_s:Number(map.get(r.update).accumulated_delta_before)-origin,count:0,before:r.gate_scale_before,after:r.scale_after};runs.push(s);}
    s.last=r.update;s.count++;s.after=r.scale_after;
  }
  const recoveries=[];
  for(let i=0;i<w.length;i++){
    if(w[i].gate_scale_before!==.5||w[i].reduced||!w[i].gate_active)continue;
    const rows=[];for(let j=i;j<w.length&&rows.length<6;j++){if(w[j].reduced||!w[j].gate_active)break;rows.push(w[j]);if(w[j].scale_after===1)break;}
    if(rows.at(-1)?.scale_after!==1)continue;
    const before=map.get(rows[0].update),after=map.get(rows.at(-1).update+1);
    const points=rows.map(r=>map.get(r.update));points.push(after);
    const displacement=points.slice(1).reduce((s,b,k)=>s+Math.hypot(...['x','z','y'].map(axis=>Number(b['before_'+axis])-Number(points[k]['before_'+axis]))),0);
    recoveries.push({first:rows[0].update,last:rows.at(-1).update,start_s:Number(before.accumulated_delta_before)-origin,
      before:.5,outputs:rows.map(r=>r.scale_after),broad_elapsed_s:rows.reduce((s,r)=>s+r.broad_delta,0),module_elapsed_s:rows.reduce((s,r)=>s+r.gate_module_1c,0),
      keys:rows.map(r=>map.get(r.update).before_keyboard_mask),animations:[...new Set(rows.map(r=>map.get(r.update).before_animation_id))],path_length:displacement,
      conditional_consumer_factors:rows.map(r=>Math.min(1,r.gate_scale_before)),comparison_seen:rows.map(r=>r.compare_seen),corrected_tests:rows.map(r=>r.corrected??null),
      source_checks:rows.map(r=>{const b=Buffer.from(gates.get(r.update).builder_bytes,'hex'),v=[0x1d0,0x1d4,0x1d8].map(o=>b.readFloatLE(o)),rate=b.readFloatLE(0x260),ratio=Math.hypot(...v)/(rate*r.gate_module_1c),expected=Math.min(1,r.gate_scale_before);assert(rate>0);assert(Math.abs(ratio-expected)<2e-7);return {update:r.update,source_rate_before_factor:rate,observed_norm_ratio:ratio,expected_factor:expected};})});
  }
  const alignments=[];
  for(const shift of [-2,-1,0,1,2]){
    const pairs=w.filter(r=>map.has(r.update+shift));
    alignments.push({shift,rows:pairs.length,module1c_matches:pairs.filter(r=>r.gate_module_1c===Math.fround(Number(map.get(r.update+shift).delta))).length});
  }
  reports.push({hz,session:name,intervals:w.length,caller_rvas:[...new Set(w.map(r=>r.recurrence_caller_rva??null))],
    caller_delta_matches_module1c:w.filter(r=>r.gate_xmm6_low===r.gate_module_1c).length,
    start:{x:u[0].before_x,z:u[0].before_z,y:u[0].before_y,facing:u[0].before_angle},runs,recoveries,clock_alignment:alignments});
}
const result={reports,classification:'OBSERVED local recovery states and clock correlations; not matched gameplay displacement',
  limits:'Different approach/side-step timings and facing. Same state sequence is not enough for a whole-trial movement ratio. Caller identity exists only in165. Displacement during recovery depends on source, terrain and integration; do not infer a gain fix from path-length ratio.'};
fs.writeFileSync(path.join(here,'analysis/transient-20260920-165/recovery-comparison.json'),JSON.stringify(result,null,2)+'\n');
console.log(JSON.stringify(result,null,2));
