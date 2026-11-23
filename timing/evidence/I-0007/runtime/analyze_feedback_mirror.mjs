import fs from 'node:fs';
import path from 'node:path';
import crypto from 'node:crypto';
import assert from 'node:assert/strict';
const f=Math.fround, add=(a,b)=>f(a+b),mul=(a,b)=>f(a*b);
function vec(hex,offset=0){const b=Buffer.from(hex,'hex');assert(b.length>=offset+16);return Array.from({length:4},(_,i)=>b.readFloatLE(offset+4*i));}
function xmm(r,n){const b=Buffer.alloc(16);b.writeBigUInt64LE(BigInt(r[`xmm${n}_lo`]));b.writeBigUInt64LE(BigInt(r[`xmm${n}_hi`]),8);return vec(b.toString('hex'));}
function distance(a,b){const q=a.slice(0,3).map((x,i)=>mul(f(x-b[i]),f(x-b[i])));return f(Math.sqrt(add(add(q[0],q[2]),q[1])));}
function nextScale(s,reduce){return reduce?(s>0.5?mul(s,f(0.8)):0.5):(s<1?mul(s,f(1.2)):1);}
function analyze(hits,base){
  const rva=h=>Number(BigInt(h.rip_at_trap)-base);
  assert(hits.length===1||hits.length===2);assert.equal(rva(hits.at(-1)),0x5C7435);
  if(hits.length===2)assert.equal(rva(hits[0]),0x5C73D8);
  const z=hits.at(-1),b=hits.length===2?hits[0]:null;
  for(const h of hits){
    assert.equal(h.builder_address,z.builder_address);assert.equal(h.rbx,h.builder_address);
    assert.equal(h.owner_e0_address,z.owner_e0_address);assert(BigInt(h.rdi)+0xE0n===BigInt(h.owner_e0_address));
    assert.equal(h.tid,z.tid);assert.equal(h.overflow,'0');
    for(const k of ['code_valid','input_valid','output_valid','nested_valid','owner_valid','additive_valid','producer_code_valid','modifier_code_valid'])assert.equal(h[k],'1');
    assert.equal(Number(BigInt(h.dr6)&15n),rva(h)===0x5C73D8?1:2);
  }
  const scale=vec(z.builder_bytes,0x250)[3];
  if(!b)return {update:Number(z.broad_update_index),compare_seen:false,scale_after:scale};
  assert(BigInt(b.seq)<BigInt(z.seq));
  const old=xmm(b,6),getter=vec(b.output_bytes,32),d=distance(getter,old),test=xmm(b,0)[0],limit=xmm(b,7)[0];
  const dt=vec(b.timing_18_bytes)[1],s=vec(b.builder_bytes,0x250)[3];
  assert([d,test,limit,dt,s,scale].every(Number.isFinite));assert(dt>0);assert.equal(limit,1);
  assert(Object.is(test,mul(d,30)),'Captured fixed-distance test must reconstruct bit exactly');
  const expected=nextScale(s,test<limit),normal=f(test/f(60*dt));
  return {update:Number(z.broad_update_index),compare_seen:true,dt,old,getter,distance:d,raw_test:test,
    scale_before:s,scale_after:scale,reduce:test<limit,expected_scale:expected,scale_exact:Object.is(scale,expected),
    hypothetical_60_equivalent_test:normal,hypothetical_time_normalized_reduce:normal<limit,
    measured_speed_260:vec(b.builder_bytes,0x260)[0],owner_a2:Buffer.from(b.input_bytes,'hex')[0x22]};
}
if(process.argv[2]==='--self-test'){
  assert.equal(distance([3,4,0,1],[0,0,0,1]),5);
  assert.equal(nextScale(0.5,true),0.5);assert.equal(nextScale(1,true),f(0.8));
  assert.equal(nextScale(0.5,false),f(0.6));assert.equal(nextScale(1,false),1);
  assert.throws(()=>vec('00'));
  console.log('PASS: distance order, both scale recurrence branches, floor/recovery and invalid vector.');
}else{
  const [sessionArg,outArg]=process.argv.slice(2);assert(sessionArg&&outArg);
  const session=path.resolve(sessionArg),out=path.resolve(outArg),read=n=>fs.readFileSync(path.join(session,n),'utf8');
  const csv=n=>{const [header,...lines]=read(n).trim().split(/\r?\n/),keys=header.split(',');return lines.filter(Boolean).map(l=>Object.fromEntries(l.split(',').map((v,i)=>[keys[i],v])));};
  const info=Object.fromEntries(read('session-info.txt').trim().split(/\r?\n/).map(l=>{const i=l.indexOf('=');return[l.slice(0,i),l.slice(i+1)];}));
  assert.equal(info.schema,'I-0007-scale-feedback-mirror-v1');
  const base=BigInt(info.image_base),hits=csv('scale-feedback-mirror.csv'),groups=new Map();
  for(const h of hits){if(!groups.has(h.broad_update_index))groups.set(h.broad_update_index,[]);groups.get(h.broad_update_index).push(h);}
  const rows=[...groups.values()].map(h=>analyze(h,base)),tested=rows.filter(r=>r.compare_seen);
  const updates=csv('locomotion-updates.csv'),updateMap=new Map(updates.map(r=>[Number(r.update_index),r]));
  const position=r=>['before_x','before_z','before_y'].map(k=>f(Number(r[k])));
  for(const r of tested){
    const current=updateMap.get(r.update),prior=updateMap.get(r.update-1);assert(current&&prior);
    r.getter_matches_current_broad=position(current).every((v,i)=>Object.is(v,r.getter[i]));
    r.old_matches_previous_broad=position(prior).every((v,i)=>Object.is(v,r.old[i]));
    r.governing_previous_delta=f(Number(prior.delta));
    r.current_delta_matches_previous=Object.is(r.dt,r.governing_previous_delta);
    r.previous_delta_normalized_test=f(r.raw_test*f(f(1/60)/r.governing_previous_delta));
  }
  const restores=csv('override-transitions.csv').filter(r=>r.action==='restore'),events=csv('locomotion-events.csv');
  const summary={session:path.basename(session),target:Number(info.target_fps),hits:hits.length,intervals:rows.length,
    measured_rows:csv('locomotion-updates.csv').filter(r=>r.phase==='measure').length,
    comparisons:tested.length,scale_exact:tested.filter(r=>r.scale_exact).length,reductions:tested.filter(r=>r.reduce).length,
    getter_current_broad_matches:tested.filter(r=>r.getter_matches_current_broad).length,
    old_previous_broad_matches:tested.filter(r=>r.old_matches_previous_broad).length,
    timing_current_previous_matches:tested.filter(r=>r.current_delta_matches_previous).length,
    previous_delta_normalized_reductions:tested.filter(r=>r.previous_delta_normalized_test<1).length,
    hypothetical_normalized_reductions:tested.filter(r=>r.hypothetical_time_normalized_reduce).length,
    raw_test_range:tested.length?[Math.min(...tested.map(r=>r.raw_test)),Math.max(...tested.map(r=>r.raw_test))]:null,
    scale_after_values:[...new Set(rows.map(r=>r.scale_after))],unique_builders:new Set(hits.map(r=>r.builder_address)).size,
    unique_threads:new Set(hits.map(r=>r.tid)).size,
    complete_trial:events.filter(r=>r.event==='measure_end').length===1&&!events.some(r=>r.event.startsWith('aborted')),
    timing_restored:restores.length===1&&restores[0].success==='1'&&restores[0].fps_after==='30'&&restores[0].enable_after==='0',
    restoration_report:read('COMPLETE').trim(),
    raw_sha256:Object.fromEntries(fs.readdirSync(session).filter(n=>fs.statSync(path.join(session,n)).isFile()).map(n=>[n,crypto.createHash('sha256').update(fs.readFileSync(path.join(session,n))).digest('hex').toUpperCase()]))};
  summary.capture_gates_pass=rows.length>0&&rows.length===summary.measured_rows-1&&summary.unique_builders===1&&summary.unique_threads===1&&summary.complete_trial&&summary.timing_restored;
  fs.mkdirSync(out,{recursive:true});
  fs.writeFileSync(path.join(out,'feedback-summary.json'),JSON.stringify(summary,null,2)+'\n');
  fs.writeFileSync(path.join(out,'feedback-stages.json'),JSON.stringify(rows,null,2)+'\n');
  console.log(JSON.stringify(summary,null,2));
}
