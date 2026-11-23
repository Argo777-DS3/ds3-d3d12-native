import fs from 'node:fs';
import path from 'node:path';
import crypto from 'node:crypto';
import assert from 'node:assert/strict';

const sites=[0x5CA642,0x5CA648,0x5CA65E,0x9DB9C1];
const same=(a,b)=>a.every((v,i)=>Object.is(v,b[i]));
const norm=v=>Math.hypot(...v.slice(0,3));
const hash=b=>crypto.createHash('sha256').update(b).digest('hex').toUpperCase();
function vec(hex,offset=0) {
  const b=Buffer.from(hex,'hex');assert(b.length>=offset+16);
  const v=Array.from({length:4},(_,i)=>b.readFloatLE(offset+4*i));
  assert(v.every(Number.isFinite));return v;
}
function scalar(hex,offset) {const b=Buffer.from(hex,'hex');assert(b.length>=offset+4);const v=b.readFloatLE(offset);assert(Number.isFinite(v));return v;}
function xmm(r,n) {const b=Buffer.alloc(16);b.writeBigUInt64LE(BigInt(r[`xmm${n}_lo`]));b.writeBigUInt64LE(BigInt(r[`xmm${n}_hi`]),8);return vec(b.toString('hex'));}
function inspect(hits,base) {
  assert.equal(hits.length,4,'Expected exactly four selected observations');
  assert.deepEqual(hits.map(h=>Number(BigInt(h.rip_at_trap)-base)),sites);
  const [b,v,m,z]=hits;
  for(let i=0;i<hits.length;i++) {
    const h=hits[i];
    assert.equal(Number(BigInt(h.dr6)&15n),1<<i);
    assert(h.builder_address===b.builder_address&&h.rbp===b.rbp&&h.tid===b.tid&&h.owner_e0_address===b.owner_e0_address);
    assert(BigInt(h.rbp)===BigInt(h.builder_address)+0x1D0n);
    assert(BigInt(h.rdi)+0xE0n===BigInt(h.owner_e0_address));
    assert(h.producer_vtable===b.producer_vtable&&h.producer_target===b.producer_target);
    for(const k of ['code_valid','input_valid','output_valid','nested_valid','owner_valid','additive_valid','producer_code_valid','modifier_code_valid'])assert.equal(h[k],'1',k);
    assert.equal(h.overflow,'0');
    if(i)assert(BigInt(h.seq)>BigInt(hits[i-1].seq));
  }
  assert.equal(b.rcx,b.builder_address);assert.equal(b.rdx,b.rbp);
  assert.equal(z.r8,z.rbp);assert(BigInt(z.rcx)+0xE0n===BigInt(z.owner_e0_address));
  const dt=xmm(b,6)[0];assert(dt>0);assert(hits.every(h=>Object.is(xmm(h,6)[0],dt)));
  const source=hits.map(h=>vec(h.input_bytes));
  const scales=hits.map(h=>scalar(h.builder_bytes,0x25C));
  return {update:Number(b.broad_update_index),dt,builder:b.builder_address,source_address:b.rbp,
    producer_target:b.producer_target,producer_rva:'0x'+(BigInt(b.producer_target)-base).toString(16),
    before:source[0],virtual_output:source[1],modified:source[2],final:source[3],
    modifier_unchanged:same(source[1],source[2]),later_unchanged:same(source[2],source[3]),
    later_matches_scale:same(source[2].map(x=>Math.fround(x*scales[2])),source[3]),
    source_norm_per_delta:source.map(v=>norm(v)/dt),scale_25c:scales,
    measured_260:hits.map(h=>scalar(h.builder_bytes,0x260)),
    c0_c4:hits.map(h=>[scalar(h.builder_bytes,0xC0),scalar(h.builder_bytes,0xC4)]),
    flag_18a:hits.map(h=>Buffer.from(h.builder_bytes,'hex')[0x18A])};
}
function selfTest() {
  const base=0x140000000n, pack=v=>{const b=Buffer.alloc(96);v.forEach((x,i)=>b.writeFloatLE(x,i*4));return b.toString('hex');};
  const builder=Buffer.alloc(0x270);builder.writeFloatLE(0.5,0x25C);
  const dt=Buffer.alloc(8);dt.writeFloatLE(1/165);
  const hits=sites.map((site,i)=>({seq:String(i+1),rip_at_trap:'0x'+(base+BigInt(site)).toString(16),dr6:String(1<<i),
    builder_address:'0x1000',rbp:'0x11d0',rdi:'0x2000',owner_e0_address:'0x20e0',tid:'1',broad_update_index:'3',
    producer_vtable:'0x140010000',producer_target:'0x140020000',rcx:i===3?'0x2000':'0x1000',rdx:'0x11d0',r8:'0x11d0',
    xmm6_lo:'0x'+dt.readBigUInt64LE().toString(16),xmm6_hi:'0x0',input_bytes:pack(i===3?[0,0,1,0]:[0,0,2,0]),
    builder_bytes:builder.toString('hex'),overflow:'0',...Object.fromEntries(['code_valid','input_valid','output_valid','nested_valid','owner_valid','additive_valid','producer_code_valid','modifier_code_valid'].map(k=>[k,'1']))}));
  const result=inspect(hits,base);assert(result.modifier_unchanged&&result.later_matches_scale&&!result.later_unchanged);
  assert.throws(()=>inspect(hits.slice(1),base));
  const bad=structuredClone(hits);bad[2].input_valid='0';assert.throws(()=>inspect(bad,base));
  const reordered=[hits[0],hits[2],hits[1],hits[3]];assert.throws(()=>inspect(reordered,base));
  const wrong=structuredClone(hits);wrong[3].builder_address='0x1010';assert.throws(()=>inspect(wrong,base));
  assert.throws(()=>vec('00'));
  console.log('PASS: four-site pairing, scale discrimination, invalid reads, wrong owner/order and truncation rejection.');
}
if(process.argv[2]==='--self-test')selfTest();else {
  const [sessionArg,outArg]=process.argv.slice(2);assert(sessionArg&&outArg,'Expected session and analysis directories');
  const session=path.resolve(sessionArg),out=path.resolve(outArg),read=n=>fs.readFileSync(path.join(session,n),'utf8');
  const csv=n=>{const [h,...ls]=read(n).trim().split(/\r?\n/),keys=h.split(',');return ls.filter(Boolean).map(l=>Object.fromEntries(l.split(',').map((v,i)=>[keys[i],v])));};
  const info=Object.fromEntries(read('session-info.txt').trim().split(/\r?\n/).map(l=>{const i=l.indexOf('=');return [l.slice(0,i),l.slice(i+1)];}));
  assert.equal(info.schema,'I-0007-source-builder-mirror-v1');
  const base=BigInt(info.image_base),hits=csv('source-builder-mirror.csv'),groups=new Map();
  for(const h of hits){if(!groups.has(h.broad_update_index))groups.set(h.broad_update_index,[]);groups.get(h.broad_update_index).push(h);}
  const stages=[...groups.values()].map(h=>inspect(h,base));assert(stages.length);
  const dt=stages.reduce((s,r)=>s+r.dt,0);
  const events=csv('locomotion-events.csv'),restores=csv('override-transitions.csv').filter(r=>r.action==='restore');
  const updates=csv('locomotion-updates.csv').filter(r=>r.phase==='measure');
  const code=new Map();
  for(const h of hits)for(const [kind,address]of [['producer',h.producer_target],['modifier','0x'+(base+0x9C8E80n).toString(16)]]) {
    const hex=h[`${kind}_code`],sha=hash(Buffer.from(hex,'hex')),key=kind+':'+sha;
    if(!code.has(key))code.set(key,{kind,address,sha256:sha,bytes:hex,count:0});code.get(key).count++;
  }
  const summary={session:path.basename(session),target:Number(info.target_fps),hits:hits.length,intervals:stages.length,
    measurement_rows:updates.length,unique_builders:new Set(stages.map(r=>r.builder)).size,unique_threads:new Set(hits.map(r=>r.tid)).size,
    producer_rvas:[...new Set(stages.map(r=>r.producer_rva))],
    virtual_norm_per_delta:stages.reduce((s,r)=>s+norm(r.virtual_output),0)/dt,
    modified_norm_per_delta:stages.reduce((s,r)=>s+norm(r.modified),0)/dt,
    final_norm_per_delta:stages.reduce((s,r)=>s+norm(r.final),0)/dt,
    modifier_unchanged:stages.filter(r=>r.modifier_unchanged).length,later_unchanged:stages.filter(r=>r.later_unchanged).length,
    later_matches_scale:stages.filter(r=>r.later_matches_scale).length,
    scale_values:[...new Set(stages.flatMap(r=>r.scale_25c))],
    measured_speed_range:[Math.min(...stages.flatMap(r=>r.measured_260)),Math.max(...stages.flatMap(r=>r.measured_260))],
    complete_trial:events.filter(r=>r.event==='measure_end').length===1&&!events.some(r=>r.event.startsWith('aborted')),
    timing_restored:restores.length===1&&restores[0].success==='1'&&restores[0].fps_after==='30'&&restores[0].enable_after==='0',
    restoration_report:read('COMPLETE').trim(),
    raw_sha256:Object.fromEntries(fs.readdirSync(session).filter(n=>fs.statSync(path.join(session,n)).isFile()).map(n=>[n,hash(fs.readFileSync(path.join(session,n)))]))};
  summary.capture_gates_pass=stages.length===updates.length-1&&summary.unique_builders===1&&summary.unique_threads===1&&summary.complete_trial&&summary.timing_restored;
  fs.mkdirSync(out,{recursive:true});
  for(const [name,data]of [['builder-summary',summary],['builder-stages',stages],['builder-live-code',[...code.values()]]])fs.writeFileSync(path.join(out,name+'.json'),JSON.stringify(data,null,2)+'\n');
  console.log(JSON.stringify(summary,null,2));
}
