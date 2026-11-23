// Read-only audit of retained captures; never runs DS3 or injects a probe.
import fs from 'node:fs';
import path from 'node:path';
import crypto from 'node:crypto';
import assert from 'node:assert/strict';
import {fileURLToPath} from 'node:url';
const here=path.dirname(fileURLToPath(import.meta.url));
const hash=p=>crypto.createHash('sha256').update(fs.readFileSync(p)).digest('hex').toUpperCase();
const csv=p=>{const [h,...ls]=fs.readFileSync(p,'utf8').trim().split(/\r?\n/);const k=h.split(',');return ls.map(l=>Object.fromEntries(l.split(',').map((v,i)=>[k[i],v])));};
const json=p=>JSON.parse(fs.readFileSync(p,'utf8'));
const f=Math.fround;
const float=(hex,offset)=>Buffer.from(hex,'hex').readFloatLE(offset);
const samePair=(a,b)=>!!a&&a.trial===b.trial&&a.builder===b.builder&&a.thread===b.thread&&a.context===b.context&&a.update+1===b.update;
// Avoid deriving elapsed time across a skipped update, another trial or owner.
const a={trial:1,builder:'a',thread:7,context:'c',update:3};
assert(samePair(a,{...a,update:4}));
for(const b of [{...a,update:5},{...a,trial:2,update:4},{...a,builder:'b',update:4},{...a,thread:8,update:4}])assert(!samePair(a,b));
const reports=[];
for(const session of ['20260919T184047Z-pid3804','20260919T185120Z-pid3804','20260919T185858Z-pid3804']){
  const dir=path.join(here,'sessions',session);
  const historical=json(path.join(here,'analysis',session,'causal-patch-summary.json'));
  const names=['causal-distance-patch.csv','locomotion-updates.csv','session-info.txt'];
  for(const n of names)assert.equal(hash(path.join(dir,n)),historical.raw_sha256[n]);
  const info=Object.fromEntries(fs.readFileSync(path.join(dir,'session-info.txt'),'utf8').trim().split(/\r?\n/).map(l=>{const i=l.indexOf('=');return[l.slice(0,i),l.slice(i+1)];}));
  const base=BigInt(info.image_base),frequency=Number(info.qpc_frequency);
  const hits=csv(path.join(dir,names[0])),updates=csv(path.join(dir,names[1]));
  const byUpdate=new Map(updates.map(u=>[Number(u.update_index),u]));
  const rva=h=>Number(BigInt(h.rip_at_trap)-base);
  const compares=new Map(hits.filter(h=>rva(h)===0x5c73d8).map(h=>[Number(h.broad_update_index),h]));
  const ends=hits.filter(h=>rva(h)===0x5c7435);
  assert.equal(new Set(ends.map(h=>h.broad_update_index)).size,ends.length);
  const rows=[];
  for(const h of ends){
    const index=Number(h.broad_update_index),u=byUpdate.get(index),after=byUpdate.get(index+1),c=compares.get(index);
    assert(u&&after);assert.equal(u.trial,h.trial);assert.equal(after.trial,h.trial);
    const row={trial:Number(h.trial),update:index,builder:h.builder_address,thread:Number(h.tid),context:h.shared_context,
      qpc:Number(h.qpc),broad_delta:f(Number(u.delta)),module_18:float(h.timing_18_bytes,0),module_1c:float(h.timing_18_bytes,4),
      comparison_seen:!!c,scale_after:float(h.builder_bytes,0x25c),phase:u.phase,
      animation_id:Number(u.before_animation_id),keys:u.before_keyboard_mask};
    const previous=rows.at(-1);
    row.adjacent_same_identity=samePair(previous,row);
    if(row.adjacent_same_identity){
      row.exit_to_exit_wall=(row.qpc-previous.qpc)/frequency;
      assert(row.exit_to_exit_wall>0);
      row.previous_broad_delta=previous.broad_delta;
    }
    if(c){
      row.scale_before=float(c.builder_bytes,0x25c);
      row.raw_test=f(Number(c.patch_raw));row.corrected_test=f(Number(c.patch_corrected));
      row.distance_elapsed=f(Number(c.patch_delta));
      if(row.adjacent_same_identity){
        assert.equal(row.scale_before,previous.scale_after,'No net intervening scalar change at compared consecutive calls');
        assert.equal(row.distance_elapsed,previous.broad_delta);
      }
    }
    const pos=v=>['before_x','before_z','before_y'].map(k=>Number(v[k]));
    const p=pos(u),q=pos(after);row.following_displacement=Math.hypot(...p.map((v,i)=>q[i]-v));
    row.following_distance_per_broad_delta=row.following_displacement/row.broad_delta;
    rows.push(row);
  }
  const trials=[...new Set(rows.map(r=>r.trial))].map(trial=>{
    const rs=rows.filter(r=>r.trial===trial),us=updates.filter(u=>Number(u.trial)===trial),adj=rs.filter(r=>r.adjacent_same_identity);
    const nonunit=rs.filter(r=>r.scale_after!==1),comparisons=rs.filter(r=>r.comparison_seen);
    assert.equal(rs.length,us.length-1,'One observed exit per complete captured broad interval');
    assert.equal(adj.length,rs.length-1,'No interval gap inside a captured trial');
    const interesting=new Set(nonunit.flatMap(r=>Array.from({length:7},(_,i)=>r.update+i-3)));
    return {trial,exits:rs.length,consecutive_pairs:adj.length,comparisons:comparisons.length,
      no_comparison_exits:rs.length-comparisons.length,nonunit_outputs:nonunit.length,
      compared_input_matches_prior_exit:comparisons.filter(r=>r.adjacent_same_identity).length,
      module_18_differs_broad:rs.filter(r=>r.module_18!==r.broad_delta).length,
      module_1c_differs_broad:rs.filter(r=>r.module_1c!==r.broad_delta).length,
      nonunit_window:rs.filter(r=>interesting.has(r.update))};
  });
  reports.push({session,raw_sha256:Object.fromEntries(names.map(n=>[n,hash(path.join(dir,n))])),trials});
}
const report={investigation:'I-0007',cycle:12,reports,
  observation:'Every complete captured update interval has one same-identity recurrence exit. Compared scalar inputs equal the preceding captured exit; distance normalization uses the preceding broad delta.',
  limitations:['The exit hook also fires when the outer eligibility gate skips the recurrence. An exit without comparison at scalar 1 cannot distinguish skip from recovery/reset.',
    'QPC exit-to-exit time is observed wall time, not proof of the simulation elapsed interval appropriate for a new recurrence law.',
    'No non-equilibrium target-60 reference exists in these patched captures. The single target-165 V2 transient therefore has no matched causal comparison.'],
  outcome:'Insufficient information for a residual gameplay fault or new correction. Preserve K-0037 through K-0039. Budget 12/12 reached; explicit replan required before another cycle.'};
const out=path.join(here,'reconciliation-20260920','recurrence-interval-audit.json');
fs.writeFileSync(out,JSON.stringify(report,null,2)+'\n');
console.log(JSON.stringify({...report,reports:reports.map(s=>({...s,trials:s.trials.map(({nonunit_window,...t})=>t)}))},null,2));
