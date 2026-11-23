import fs from 'node:fs';
import path from 'node:path';
import crypto from 'node:crypto';
import assert from 'node:assert/strict';
import {fileURLToPath} from 'node:url';
const here=path.dirname(fileURLToPath(import.meta.url));
const out=path.join(here,'analysis/release-control-20260920-60');
const summary=JSON.parse(fs.readFileSync(path.join(out,'transient-summary.json'),'utf8'));
const rows=JSON.parse(fs.readFileSync(path.join(out,'transient-windows.json'),'utf8'));
const session=path.join(here,'sessions',summary.session);
for(const [name,h] of Object.entries(summary.raw_sha256))assert.equal(crypto.createHash('sha256').update(fs.readFileSync(path.join(session,name))).digest('hex').toUpperCase(),h);
const csv=name=>{const[h,...ls]=fs.readFileSync(path.join(session,name),'utf8').trim().split(/\r?\n/),keys=h.split(',');return ls.map(l=>Object.fromEntries(l.split(',').map((v,i)=>[keys[i],v])));};
const updates=csv('locomotion-updates.csv'),semantic=csv('semantic-state.csv'),events=csv('locomotion-events.csv');
const origin=Number(updates[0].accumulated_delta_before),first=Number(updates[0].update_index);
const transitions=[];let last;
for(const r of semantic.filter(r=>Number(r.update_index)>=first))if(r.keyboard_mask!==last){transitions.push({elapsed:Number(r.accumulated_delta)-origin,update:Number(r.update_index),keys:r.keyboard_mask});last=r.keyboard_mask;}
const release=transitions.find((r,i)=>i>0&&r.keys==='0x0');
const end=events.find(r=>r.event==='trial_end');assert(end);
assert.equal(rows.length,480);assert(rows.every(r=>r.scale_after===r.native_scale_after));
assert(summary.timing_restored&&summary.exact_arithmetic_and_commit_checks);
const result={session:summary.session,raw_hashes_verified:Object.keys(summary.raw_sha256).length,
 exact_intervals:rows.length,native_equal60_outputs:rows.length,nonunit_outputs:rows.filter(r=>r.scale_after!==1).length,
 capture_end:Number(end.accumulated_delta)-origin,capture_end_keyboard:end.keyboard_mask,
 input_transitions_from_semantic_log:transitions,first_complete_release:release,
 release_inside_candidate_window:release.elapsed<=Number(end.accumulated_delta)-origin,
 decision:'Insufficient information for the planned60 candidate release-tail control. Do not repeat immediately.',
 cause:'Fixed8s trial ended with Z+Q held; complete keyboard release was first sampled around11.533s, outside the candidate correction/commit window. Semantic logging continued but cannot qualify candidate behavior after correction ceased.',
 next:'Replan stimulus/capture synchronization with an observed release edge and bounded post-release tail; qualify capacity, timeout and restoration before considering another acquisition. No correction-law change.',
 limits:'Semantic timestamps describe broad accumulated time after the sample, not exact physical key-up time. Post-window semantic samples are not candidate release proof. No new gameplay defect or whole-motion acceptance follows.'};
assert.equal(result.release_inside_candidate_window,false);
fs.writeFileSync(path.join(out,'release-assessment.json'),JSON.stringify(result,null,2)+'\n');
console.log(JSON.stringify(result,null,2));
