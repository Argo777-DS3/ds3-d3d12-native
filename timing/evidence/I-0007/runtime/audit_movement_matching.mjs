import fs from 'node:fs';
import path from 'node:path';
import crypto from 'node:crypto';
import assert from 'node:assert/strict';
import {fileURLToPath} from 'node:url';
const here=path.dirname(fileURLToPath(import.meta.url));
const dir=path.join(here,'analysis/elapsed-20260920-paired');
const json=p=>JSON.parse(fs.readFileSync(path.join(dir,p),'utf8'));
const pair=json('elapsed-pair-evaluation.json'),summary=json('transient-summary.json');
const source=json('candidate-source-evaluation.json'),windows=json('transient-windows.json');
const session=path.join(here,'sessions',pair.session);
for(const [name,h] of Object.entries(summary.raw_sha256))assert.equal(crypto.createHash('sha256').update(fs.readFileSync(path.join(session,name))).digest('hex').toUpperCase(),h);
const [header,...lines]=fs.readFileSync(path.join(session,'locomotion-updates.csv'),'utf8').trim().split(/\r?\n/),keys=header.split(',');
const updates=new Map(lines.map(l=>{const r=Object.fromEntries(l.split(',').map((v,i)=>[keys[i],v]));return [Number(r.update_index),r];}));
const position=r=>['x','z','y'].map(k=>Number(r['before_'+k]));
const reports=pair.reports.map(p=>{
 const first=p.recoveries[0].first,u=updates.get(first),rows=windows.filter(r=>r.trial===p.trial&&r.update>=first);
 let elapsed=0,pathLength=0,last=position(u),previousPath=0;
 const horizon=5/60,segments=[];
 for(const r of rows){
  assert(r.commit_exact);const next=updates.get(r.update+1);assert(next);
  const pos=position(next),length=Math.hypot(...pos.map((v,i)=>v-last[i]));
  previousPath=pathLength;pathLength+=length;elapsed+=r.gate_module_1c;
  segments.push({update:r.update,delta:r.gate_module_1c,world_commit_length:length,keys:updates.get(r.update).before_keyboard_mask});last=pos;
  if(elapsed>=horizon)break;
 }
 assert(elapsed>=horizon);
 const sr=source.reports.find(s=>s.trial===p.trial);
 return {trial:p.trial,first,start_position:position(u),start_angle:Number(u.before_angle),animation_id:u.before_animation_id,
  animation_time:Number(u.before_animation_time),animation_duration:Number(u.before_animation_max),
  source_rate_range:sr.pre_factor_rate_range,reference_horizon:horizon,last_commit_elapsed:elapsed,
  sampled_path_bracket:[previousPath,pathLength],segments};
});
const [a,b]=reports;
const angle=Math.abs(Math.atan2(Math.sin(b.start_angle-a.start_angle),Math.cos(b.start_angle-a.start_angle)));
const result={session:pair.session,raw_files_verified:Object.keys(summary.raw_sha256).length,reports,
 start_position_difference:Math.hypot(...b.start_position.map((v,i)=>v-a.start_position[i])),
 start_angle_difference_radians:angle,start_angle_difference_degrees:angle*180/Math.PI,
 animation_time_difference:b.animation_time-a.animation_time,
 decision:'Retained recoveries do not qualify a matched world-motion comparison. Preserve measured commits but do not compute an acceptance ratio between these trajectories.',
 limits:'Path bracket bounds only the two sampled polyline totals around the horizon; it is not a bound on unknown physical sub-frame motion. Different direction, animation phase, source-rate history and contact geometry remain confounds. Exact commit reconstruction and equal keyboard mask do not remove them. A new pair requires prospective scene/input/animation matching and explicit interval-bracketing before the existing2% motion threshold can be used.'};
fs.writeFileSync(path.join(dir,'movement-matching-audit.json'),JSON.stringify(result,null,2)+'\n');
console.log(JSON.stringify({...result,reports:reports.map(({segments,...r})=>r)},null,2));
