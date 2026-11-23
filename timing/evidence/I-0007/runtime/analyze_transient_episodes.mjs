import fs from 'node:fs';
import path from 'node:path';
import crypto from 'node:crypto';
import assert from 'node:assert/strict';
import {fileURLToPath} from 'node:url';

const runtime=path.dirname(fileURLToPath(import.meta.url));
const session=path.join(runtime,'sessions/20260920T011956Z-pid19144');
const analysis=path.join(runtime,'analysis/transient-20260920-60');
const read=p=>fs.readFileSync(p,'utf8');
const csv=p=>{const [h,...lines]=read(p).trim().split(/\r?\n/);const keys=h.split(',');return lines.map(l=>Object.fromEntries(l.split(',').map((v,i)=>[keys[i],v])));};
const windows=JSON.parse(read(path.join(analysis,'transient-windows.json')));
const summary=JSON.parse(read(path.join(analysis,'transient-summary.json')));
assert(summary.timing_restored&&summary.exact_arithmetic_and_commit_checks);
for(const [name,hash] of Object.entries(summary.raw_sha256))assert.equal(crypto.createHash('sha256').update(fs.readFileSync(path.join(session,name))).digest('hex').toUpperCase(),hash);
const updates=csv(path.join(session,'locomotion-updates.csv'));
const byIndex=new Map(updates.map(r=>[Number(r.update_index),r]));
const origin=Number(updates[0].accumulated_delta_before);
const time=w=>Number(byIndex.get(w.update).accumulated_delta_before)-origin;
const mode=w=>!w.gate_active?'skip':w.reduced?'reduce':'recover_or_reset';
const runs=[];
for(const w of windows){
  let r=runs.at(-1);
  if(!r||r.mode!==mode(w)||r.last_update+1!==w.update){r={mode:mode(w),first_update:w.update,last_update:w.update,start_s:time(w),count:0,elapsed_s:0,scalar_before:w.gate_scale_before,scalar_after:w.scale_after,output_sequence:[]};runs.push(r);}
  r.last_update=w.update;r.count++;r.elapsed_s+=w.broad_delta;r.scalar_after=w.scale_after;
  if(r.output_sequence.at(-1)!==w.scale_after)r.output_sequence.push(w.scale_after);
}
const episodes=[];
for(let i=0;i<windows.length;i++){
  if(windows[i].scale_after===1)continue;
  const start=i;while(i+1<windows.length&&windows[i+1].scale_after!==1)i++;
  const ws=windows.slice(start,i+1),u=ws.map(w=>byIndex.get(w.update));
  let pathLength=0;
  for(const w of ws){const a=byIndex.get(w.update),b=byIndex.get(w.update+1);pathLength+=Math.hypot(...['x','z','y'].map(k=>Number(b['before_'+k])-Number(a['before_'+k])));}
  episodes.push({first_update:ws[0].update,last_update:ws.at(-1).update,start_s:time(ws[0]),nonunit_outputs:ws.length,elapsed_s:ws.reduce((s,w)=>s+w.broad_delta,0),path_length:pathLength,keys:[...new Set(u.map(r=>r.before_keyboard_mask))],returns_to_one:i+1<windows.length,return_update:windows[i+1]?.update??null});
}
const keyRuns=[];
for(const u of updates){const key=u.before_keyboard_mask;let r=keyRuns.at(-1);if(!r||r.mask!==key){r={mask:key,start_s:Number(u.accumulated_delta_before)-origin,elapsed_s:0};keyRuns.push(r);}r.elapsed_s+=Number(u.delta);}
const wall=windows.slice(1).map(w=>w.gate_to_gate_wall).sort((a,b)=>a-b);
const metadata=Object.fromEntries(read(path.join(session,'session-info.txt')).trim().split(/\r?\n/).map(l=>{const i=l.indexOf('=');return[l.slice(0,i),l.slice(i+1)];}));
const gates=csv(path.join(session,'causal-distance-patch.csv')).filter(h=>BigInt(h.rip_at_trap)-BigInt(metadata.image_base)===0x5c734cn);
const lowFloat=hex=>{const b=Buffer.alloc(8);b.writeBigUInt64LE(BigInt(hex));return b.readFloatLE(0);};
const gateSavedRegister={samples:gates.length,xmm6_matches_current_delta:gates.filter(h=>lowFloat(h.xmm6_lo)===Math.fround(Number(byIndex.get(Number(h.broad_update_index)).delta))).length,
 limitation:'005C7160 preserves incoming XMM1 in XMM6, and005C72B0 has not overwritten XMM6 at the gate. But runtime caller identity is not captured here; register equality alone does not prove that route or the governing elapsed interval.'};
const oldDir=path.join(runtime,'sessions/20260919T184047Z-pid3804');
const oldSummary=JSON.parse(read(path.join(runtime,'analysis/20260919T184047Z-pid3804/causal-patch-summary.json')));
const oldUpdates=csv(path.join(oldDir,'locomotion-updates.csv'));
assert.equal(crypto.createHash('sha256').update(fs.readFileSync(path.join(oldDir,'locomotion-updates.csv'))).digest('hex').toUpperCase(),oldSummary.raw_sha256['locomotion-updates.csv']);
const oldWindowsPath=path.join(runtime,'analysis/20260919T184047Z-pid3804/causal-patch-windows.json');
const oldAudit=JSON.parse(read(path.join(runtime,'reconciliation-20260920/feedback-transient-audit.json')));
assert.equal(crypto.createHash('sha256').update(fs.readFileSync(oldWindowsPath)).digest('hex').toUpperCase(),oldAudit.captures[0].input_sha256);
const oldStates=oldAudit.captures[0].observed_transient.filter(w=>w.scale_after!==1);
assert.deepEqual(oldStates,JSON.parse(read(oldWindowsPath)).filter(w=>w.measured&&w.scale_after!==1));
const shortEpisode=windows.filter(w=>w.update>=2648&&w.update<=2650);
assert.deepEqual(shortEpisode.map(w=>w.scale_after),oldStates.map(w=>w.scale_after));
const existingSameStates={scalar_outputs:shortEpisode.map(w=>w.scale_after),target60_nonunit_elapsed_s:shortEpisode.reduce((s,w)=>s+w.broad_delta,0),
  historical165_nonunit_elapsed_s:oldStates.reduce((s,w)=>s+Number(oldUpdates.find(u=>Number(u.update_index)===w.update).delta),0),
  limitation:'Equal state sequence is established, not matched triggering input, eligibility or trajectory. Different observed durations do not alone prove harmful gameplay impact.'};
const result={session:path.basename(session),raw_hashes_verified:Object.keys(summary.raw_sha256).length,intervals:windows.length,
  all_gates_active:windows.every(w=>w.gate_active),module18_equal_current:windows.filter(w=>w.gate_module_18===w.broad_delta).length,module1c_equal_current:windows.filter(w=>w.gate_module_1c===w.broad_delta).length,
  gate_wall_s:{min:wall[0],median:wall[Math.floor(wall.length/2)],max:wall.at(-1)},gate_saved_register:gateSavedRegister,key_runs:keyRuns,recurrence_runs:runs,nonunit_episodes:episodes,existing_same_states:existingSameStates,
  interpretation:'All recurrence and position arithmetic was already verified. Episodes associate scalar state with observed input and displacement; collision/contact is not instrumented. This single-rate trajectory cannot establish unintended frame-rate dependence, its displacement effect, or a replacement clock law. Equal clocks at steady60 do not discriminate governing time.',
  acquisition_decision:'Retain this valid reference and input-release deviation. No repeat of accepted sprint/roll, no new gain patch, and no165 capture until a distinct matched discriminator is specified.'};
fs.writeFileSync(path.join(analysis,'transient-episodes.json'),JSON.stringify(result,null,2)+'\n');
console.log(JSON.stringify(result,null,2));
