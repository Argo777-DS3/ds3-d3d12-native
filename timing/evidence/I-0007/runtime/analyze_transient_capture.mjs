import fs from 'node:fs';
import path from 'node:path';
import assert from 'node:assert/strict';
import crypto from 'node:crypto';
import {ContinuousRecurrence} from './continuous_recurrence.mjs';
const f=Math.fround;
const norm=(raw,dt)=>{assert(Number.isFinite(raw)&&raw>=0&&Number.isFinite(dt)&&dt>0);return f(raw*f(f(1/60)/dt));};
const next=(s,reduce)=>reduce?(s>0.5?f(s*f(0.8)):0.5):(s<1?f(s*f(1.2)):1);
function recurrence(active,compared,before,corrected){
  assert(Number.isFinite(before));
  if(!active){assert(!compared,'Inactive gate cannot reach comparison');return before;}
  if(compared)assert(Number.isFinite(corrected));
  return next(before,compared&&corrected<1);
}
const vec=(hex,offset=0)=>{const b=Buffer.from(hex,'hex');assert(b.length>=offset+16);return Array.from({length:4},(_,i)=>b.readFloatLE(offset+4*i));};
function xmm(h,n){const b=Buffer.alloc(16);b.writeBigUInt64LE(BigInt(h[`xmm${n}_lo`]));b.writeBigUInt64LE(BigInt(h[`xmm${n}_hi`]),8);return vec(b.toString('hex'));}
function distance(a,b){const q=a.slice(0,3).map((v,i)=>f(f(v-b[i])**2));return f(Math.sqrt(f(f(q[0]+q[2])+q[1])));}
const equal3=(a,b)=>a.slice(0,3).every((v,i)=>Object.is(v,b[i]));
const position=r=>['before_x','before_z','before_y'].map(k=>f(Number(r[k])));
if(process.argv[2]==='--self-test'){
  assert.equal(recurrence(false,false,f(0.8)),f(0.8));
  assert.equal(recurrence(true,false,f(0.8)),f(f(0.8)*f(1.2)));
  assert.equal(recurrence(true,true,1,0.9),f(0.8));
  assert.throws(()=>recurrence(false,true,1,0.9));
  assert.equal(norm(1,f(1/60)),1);assert.equal(norm(0.5,f(1/165)),1.3750001192092896);
  assert.equal(next(0.5,false),f(0.6));assert.equal(next(1,false),1);assert.equal(next(0.5,true),0.5);
  assert.equal(distance([3,4,0],[0,0,0]),5);assert.throws(()=>norm(1,0));assert.throws(()=>vec('00'));
  console.log('PASS: normalized comparison, recurrence, distance and rejection gates.');
}else{
  const [sessionArg,outArg]=process.argv.slice(2);assert(sessionArg&&outArg);
  const session=path.resolve(sessionArg),out=path.resolve(outArg),read=n=>fs.readFileSync(path.join(session,n),'utf8');
  const exists=n=>fs.existsSync(path.join(session,n));
  const csv=n=>{const [header,...lines]=read(n).trim().split(/\r?\n/),keys=header.split(',');return lines.filter(Boolean).map(l=>Object.fromEntries(l.split(',').map((v,i)=>[keys[i],v])));};
  const info=Object.fromEntries(read('session-info.txt').trim().split(/\r?\n/).map(l=>{const i=l.indexOf('=');return[l.slice(0,i),l.slice(i+1)];}));
  assert.equal(info.schema,'I-0007-causal-distance-patch-v1');assert.equal(info.causal_distance_patch,'1');
  assert.equal(info.recurrence_gate_rva,'005C734C');
  const base=BigInt(info.image_base),rva=h=>Number(BigInt(h.rip_at_trap)-base);
  const hits=csv('causal-distance-patch.csv'),updates=csv('locomotion-updates.csv'),events=csv('locomotion-events.csv');
  assert(hits.length>0,'No patch trial captured');
  const updateMap=new Map(updates.map(r=>[Number(r.update_index),r])),groups=new Map();
  for(const h of hits){
    assert.equal(h.overflow,'0');assert.equal(h.patch_failed,'0');assert.equal(h.code_valid,'1');assert.equal(h.output_valid,'1');
    const key=h.broad_update_index;if(!groups.has(key))groups.set(key,[]);groups.get(key).push(h);
  }
  const rows=[],elapsedModels=new Map();
  for(const [key,g] of groups){
    const index=Number(key),comparisons=g.filter(h=>rva(h)===0x5C73D8),ends=g.filter(h=>rva(h)===0x5C7435),commits=g.filter(h=>rva(h)===0x9DC588);
    assert.equal(ends.length,1,`One recurrence exit at ${index}`);assert.equal(commits.length,1,`One known commit at ${index}`);
    const gates=g.filter(h=>rva(h)===0x5C734C);
    assert.equal(gates.length,1,'One eligibility observation');
    assert(comparisons.length<=1);assert.equal(g.length,3+comparisons.length,'No unknown write or execution site');
    const end=ends[0],commit=commits[0],before=updateMap.get(index),after=updateMap.get(index+1),prior=updateMap.get(index-1);
    assert(before&&after,`Broad commit endpoints ${index}`);
    for(const h of g){assert.equal(h.trial,end.trial);assert.equal(h.tid,end.tid);assert.equal(h.owner_e0_address,end.owner_e0_address);}
    assert.equal(Number(BigInt(end.dr6)&15n),2);assert.equal(Number(BigInt(commit.dr6)&15n),4);
    assert(BigInt(end.seq)<BigInt(commit.seq));assert.equal(BigInt(commit.rbx)+0xE0n,BigInt(commit.owner_e0_address));
    const written=vec(commit.output_bytes),old=vec(commit.output_bytes,16);
    assert(equal3(written,position(after)),'Commit maps to next broad position');assert(equal3(old,position(before)),'Commit history maps to current broad position');
    assert.deepEqual(written,xmm(commit,6),'Committed vector equals writer register');
    assert.equal(end.owner_valid,'1');const sAfter=vec(end.builder_bytes,0x250)[3];
    const row={trial:Number(end.trial),update:index,measured:end.measured==='1',scale_after:sAfter,compare_seen:comparisons.length===1,commit_exact:true};
    if(comparisons.length){
      const h=comparisons[0];assert.equal(h.patch_applied,'1');assert.equal(Number(BigInt(h.dr6)&15n),1);assert(BigInt(h.seq)<BigInt(end.seq));
      assert.equal(h.builder_address,end.builder_address);assert.equal(h.rbx,h.builder_address);assert.equal(BigInt(h.rdi)+0xE0n,BigInt(h.owner_e0_address));
      for(const k of ['input_valid','nested_valid','owner_valid','additive_valid','producer_code_valid','modifier_code_valid'])assert.equal(h[k],'1');
      assert.equal(Buffer.from(h.code_bytes,'hex').subarray(32,35).toString('hex'),'0f2ff8');
      const raw=f(Number(h.patch_raw)),dt=f(Number(h.patch_delta)),corrected=f(Number(h.patch_corrected));
      assert.equal(raw,xmm(h,0)[0]);assert.equal(xmm(h,7)[0],1);assert.equal(corrected,norm(raw,dt));
      assert.equal(Number(h.patch_previous_index),index-1);assert.equal(Number(h.patch_current_index),index);
      assert(equal3(vec(h.output_bytes,32),position(before)));assert(equal3(vec(h.output_bytes,16),xmm(h,6)));
      assert.equal(raw,f(distance(vec(h.output_bytes,32),xmm(h,6))*30));
      if(prior){assert.equal(dt,f(Number(prior.delta)));assert(equal3(xmm(h,6),position(prior)));}
      else assert(!row.measured,'Measured decisions require a recorded preceding delta');
    const s=vec(h.builder_bytes,0x250)[3];assert.equal(sAfter,next(s,corrected<1),'Observed recurrence uses corrected decision');
      Object.assign(row,{raw,corrected,governing_delta:dt,prior_logged:!!prior,scale_before:s,reduced:corrected<1,raw_would_reduce:raw<1});
    }else assert.equal(end.patch_applied,'0');
    const gate=gates[0],gateBefore=vec(gate.builder_bytes,0x250)[3];
    if('recurrence_caller' in gate){
      const caller=Number(BigInt(gate.recurrence_caller)-base);
      assert([0x5c7276,0x89cddc].includes(caller),'Qualified recurrence caller');
      for(const h of [gate,end,...comparisons]){
        assert.equal(h.recurrence_caller_valid,'1');
        assert.equal(h.recurrence_caller,gate.recurrence_caller,'Same invocation return address');
      }
      Object.assign(row,{recurrence_caller_rva:caller,gate_xmm6_low:xmm(gate,6)[0]});
    }
    assert.equal(Number(BigInt(gate.dr6)&15n),8);assert.equal(gate.patch_applied,'0');
    assert.equal(gate.owner_valid,'1');assert.equal(gate.additive_valid,'1');
    assert.equal(gate.builder_address,end.builder_address);assert.equal(gate.rbx,end.rbx);
    assert(BigInt(gate.seq)<BigInt(end.seq));
    assert.equal(Buffer.from(gate.code_bytes,'hex').subarray(32,34).toString('hex'),'84c0');
    const active=(BigInt(gate.rax)&255n)!==0n;
    if(comparisons.length){assert(BigInt(gate.seq)<BigInt(comparisons[0].seq));assert.equal(gateBefore,row.scale_before);}
    assert.equal(sAfter,recurrence(active,comparisons.length===1,gateBefore,row.corrected),'Gate-qualified full recurrence');
    const previous=rows.at(-1),adjacent=previous&&previous.trial===row.trial&&previous.update+1===index;
    Object.assign(row,{gate_active:active,gate_scale_before:gateBefore,gate_qpc:Number(gate.qpc),exit_qpc:Number(end.qpc),
      broad_delta:f(Number(before.delta)),gate_module_18:vec(gate.timing_18_bytes)[0],gate_module_1c:vec(gate.timing_18_bytes)[1]});
    if(info.elapsed_recurrence_patch==='1'){
      for(const h of g)assert.equal(h.elapsed_failed,'0');
      assert.equal(row.recurrence_caller_rva,0x5c7276);
      assert.equal(row.gate_xmm6_low,row.gate_module_1c);
      if(!elapsedModels.has(row.trial))elapsedModels.set(row.trial,new ContinuousRecurrence(gateBefore));
      const model=elapsedModels.get(row.trial);assert.equal(model.value,gateBefore);
      assert.equal(end.elapsed_applied,'1');assert.equal(f(Number(end.elapsed_native)),sAfter);
      assert.equal(f(Number(end.elapsed_delta)),row.gate_module_1c);
      const proposed=model.advance(active,!!row.reduced,row.gate_module_1c);
      assert.equal(f(Number(end.elapsed_output)),proposed,'Exact continuous recurrence reconstruction');
      Object.assign(row,{native_scale_after:sAfter,scale_after:proposed,elapsed_phase:model.phase});
    }
    if(adjacent){assert.equal(gateBefore,previous.scale_after);row.gate_to_gate_wall=(row.gate_qpc-previous.gate_qpc)/Number(info.qpc_frequency);}
    rows.push(row);
  }
  const trials=[...new Set(updates.map(r=>r.trial))].map(id=>{
    const u=updates.filter(r=>r.trial===id&&r.phase==='measure'),r=rows.filter(r=>r.trial===Number(id)&&r.measured),e=events.filter(r=>r.trial===id);
    assert(u.length>1);assert.equal(r.length,u.length-1,'Every measured inter-update interval covered');
    for(let i=1;i<u.length;i++)assert.equal(Number(u[i].update_index),Number(u[i-1].update_index)+1,'No missing broad update');
    const deltas=u.map(x=>Number(x.delta)).sort((a,b)=>a-b),median=deltas[Math.floor(deltas.length/2)],target=Math.abs(median-1/60)<Math.abs(median-1/165)?60:165;
    return {trial:Number(id),target_from_median_delta:target,median_delta:median,measured_updates:u.length,commits:r.length,
      comparisons:r.filter(x=>x.compare_seen).length,normalized_reductions:r.filter(x=>x.reduced).length,
      raw_would_reduce:r.filter(x=>x.raw_would_reduce).length,scale_values:[...new Set(r.map(x=>x.scale_after))],
      active_recurrences:r.filter(x=>x.gate_active).length,skipped_recurrences:r.filter(x=>!x.gate_active).length,
      nonunit_outputs:r.filter(x=>x.scale_after!==1).length,
      post_measure_updates:updates.filter(x=>x.trial===id&&x.phase==='post_measure').length,
      trial_end_observed:e.filter(x=>x.event==='trial_end').length===1,
      keyboard_released_at_end:e.filter(x=>x.event==='trial_end').length===1&&e.find(x=>x.event==='trial_end').keyboard_mask==='0x0',
      complete:e.filter(x=>x.event==='measure_end').length===1&&!e.some(x=>x.event.startsWith('aborted'))&&
        (!info.patch_lifetime||e.filter(x=>x.event==='trial_end').length===1)};
  });
  const restored=csv('override-transitions.csv').filter(r=>r.action==='restore'),complete=exists('COMPLETE');
  const summary={session:path.basename(session),status:complete?'stopped':'interim-restoration-pending',hits:hits.length,windows:rows.length,trials,
    unique_threads:new Set(hits.map(h=>h.tid)).size,exact_arithmetic_and_commit_checks:true,
    control:exists('CONTROL')?read('CONTROL').trim():null,
    timing_restored:complete&&restored.length===1&&restored[0].success==='1'&&restored[0].fps_after==='30'&&restored[0].enable_after==='0',
    completion:complete?read('COMPLETE').trim():null,
    capture_checks_pass:trials.every(t=>t.complete),gameplay_accepted:false,
    note:'Combine with locomotion distance/animation comparison, cadence, operator report and final restoration. This alone never accepts gameplay.',
    raw_sha256:complete?Object.fromEntries(fs.readdirSync(session).filter(n=>fs.statSync(path.join(session,n)).isFile()).map(n=>[n,crypto.createHash('sha256').update(fs.readFileSync(path.join(session,n))).digest('hex').toUpperCase()])):null};
  fs.mkdirSync(out,{recursive:true});fs.writeFileSync(path.join(out,'transient-summary.json'),JSON.stringify(summary,null,2)+'\n');
  fs.writeFileSync(path.join(out,'transient-windows.json'),JSON.stringify(rows,null,2)+'\n');console.log(JSON.stringify(summary,null,2));
}
