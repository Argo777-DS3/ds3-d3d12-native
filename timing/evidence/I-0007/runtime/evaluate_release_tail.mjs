import fs from 'node:fs';
import path from 'node:path';
import assert from 'node:assert/strict';
import crypto from 'node:crypto';
import {fileURLToPath} from 'node:url';
const here=path.dirname(fileURLToPath(import.meta.url));
function evaluate(analysis,trial) {
 const dir=path.join(here,'analysis',analysis),summary=JSON.parse(fs.readFileSync(path.join(dir,'transient-summary.json'),'utf8'));
 const rows=JSON.parse(fs.readFileSync(path.join(dir,'transient-windows.json'),'utf8')).filter(r=>r.trial===trial);
 const session=path.join(here,'sessions',summary.session);
 for(const [name,h] of Object.entries(summary.raw_sha256))assert.equal(crypto.createHash('sha256').update(fs.readFileSync(path.join(session,name))).digest('hex').toUpperCase(),h);
 assert(summary.timing_restored&&summary.exact_arithmetic_and_commit_checks);
 const [h,...lines]=fs.readFileSync(path.join(session,'locomotion-updates.csv'),'utf8').trim().split(/\r?\n/),keys=h.split(',');
 const updates=lines.map(l=>Object.fromEntries(l.split(',').map((v,i)=>[keys[i],v]))).filter(r=>Number(r.trial)===trial);
 const pos=r=>['x','z','y'].map(k=>Number(r['before_'+k]));
 const dist=(a,b)=>Math.hypot(...pos(a).map((v,i)=>v-pos(b)[i]));
 const elapsed=r=>Number(r.accumulated_delta_before)-Number(updates[0].accumulated_delta_before);
 let start=updates.length;while(start>0&&updates[start-1].before_keyboard_mask==='0x0')start--;
 assert(start>0&&start<updates.length,'Movement input then terminal release required');
 const tail=updates.slice(start),release=tail[0];
 const prior=updates.slice(0,start).filter(r=>elapsed(r)>=elapsed(release)-.5);
 const pathLength=list=>list.slice(1).reduce((sum,r,i)=>sum+dist(r,list[i]),0);
 let stationary=updates.length-1;while(stationary>start&&dist(updates[stationary-1],updates.at(-1))===0)stationary--;
 const releaseRow=rows.find(r=>r.update===Number(release.update_index));assert(releaseRow);
 const report={session:summary.session,trial,hashes_verified:Object.keys(summary.raw_sha256).length,exact_intervals:rows.length,
  native_equal_outputs:rows.filter(r=>r.scale_after===r.native_scale_after).length,release_elapsed:elapsed(release),
  prior_keyboard:updates[start-1].before_keyboard_mask,release_animation:release.before_animation_id,
  release_scalar:releaseRow.gate_scale_before,pre_release_half_second_path:pathLength([...prior,release]),
  released_tail_sample_span:elapsed(tail.at(-1))-elapsed(release),released_tail_path:pathLength(tail),
  stationary_suffix_sample_span:elapsed(updates.at(-1))-elapsed(updates[stationary]),
  last_motion_to_stationary_sample_delay:elapsed(updates[stationary])-elapsed(release),
  tail_controller_buttons:[...new Set(tail.map(r=>r.before_controller_buttons))],
  tail_max_abs_left_axis:Math.max(...tail.flatMap(r=>[Math.abs(Number(r.before_left_x)),Math.abs(Number(r.before_left_y))])),
  tail_max_abs_right_axis:Math.max(...tail.flatMap(r=>[Math.abs(Number(r.before_right_x)),Math.abs(Number(r.before_right_y))])),
  tail_max_trigger:Math.max(...tail.flatMap(r=>[Number(r.before_left_trigger),Number(r.before_right_trigger)]))};
 return report;
}
const control=evaluate('release-retry-20260920-60',1),retained165=evaluate('elapsed-20260920-paired',2);
assert.equal(control.native_equal_outputs,600);
assert(control.pre_release_half_second_path>0&&control.stationary_suffix_sample_span>=2);
const result={control,retained165,
 decision:'Missing60 release-tail witness acquired: moving before release, complete keyboard release, then at least2s bit-identical sampled position while the candidate capture remains active. Original165 tail retained; no new165 acquisition.',
 limits:'This qualifies bounded stopping in each recorded scene, not matched stopping latency or whole-trajectory equivalence. Animation/input/source histories can differ. Latency is sample-bracketed; exact stationary samples do not prove absence of sub-frame movement. Controller raw axes retained; no claim of physically disconnected controller. No further identical maneuver requested.'};
fs.writeFileSync(path.join(here,'analysis/release-retry-20260920-60/release-tail-evaluation.json'),JSON.stringify(result,null,2)+'\n');
console.log(JSON.stringify(result,null,2));
