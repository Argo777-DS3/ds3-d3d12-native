import fs from 'node:fs';
import path from 'node:path';
import crypto from 'node:crypto';
import assert from 'node:assert/strict';
const f = Math.fround;
const vec = (hex, off = 0) => { const b = Buffer.from(hex, 'hex'); assert(b.length >= off + 16); return [0,4,8,12].map(i => b.readFloatLE(off+i)); };
function decay(v, factor, threshold) {
  assert(v.every(Number.isFinite) && Number.isFinite(factor) && Number.isFinite(threshold));
  const result = v.map(x => f(x * factor));
  const sq = result.map(x => f(x*x));
  return f(f(sq[0]+sq[1])+sq[2]) < threshold ? [0,0,0,0] : result;
}
function pair(g, base) {
  const at = n => g.filter(r => BigInt(r.rip_at_trap)-base === BigInt(n));
  const pre=at(0x9df1b1), post=at(0x9df1b6), end=at(0x9df1f1), fallback=at(0x9dc04f);
  assert(pre.length<=1 && post.length<=1 && end.length===1 && fallback.length<=1, 'ambiguous interval');
  if (!pre.length) { assert(!post.length && !fallback.length); return {called:false}; }
  assert.equal(post.length,1,'missing return');
  const [a]=pre,[b]=post,[z]=end;
  assert(BigInt(a.seq)<BigInt(b.seq) && BigInt(b.seq)<BigInt(z.seq));
  assert(g.every(r => r.owner_e0_address===a.owner_e0_address && r.tid===a.tid && r.trial===a.trial));
  const input=Buffer.from(a.input_bytes,'hex'), dt=input.readFloatLE(8), command=input.readFloatLE(56);
  assert(dt>0 && dt<=.05 && dt===command,'unqualified forwarded delta');
  const before=vec(a.additive_2e0_bytes), after=vec(b.additive_2e0_bytes), consumed=vec(z.additive_2e0_bytes);
  const owner=Buffer.from(a.owner_70_bytes,'hex');
  const expectedFallback=owner[0x2d8-0x70]===0 || input[24]!==0 || input[25]!==0;
  assert.equal(!!fallback.length,expectedFallback,'branch qualification mismatch');
  const downstreamGate=Number(BigInt.asIntN(32,BigInt(z.rax)));
  assert.deepEqual(consumed,downstreamGate>=30?[0,0,0,0]:after,'downstream reset mismatch');
  if(fallback.length) {
    const [r]=fallback, data=Buffer.from(r.input_bytes,'hex');
    assert(BigInt(a.seq)<BigInt(r.seq) && BigInt(r.seq)<BigInt(b.seq));
    assert.equal(data.readBigUInt64LE(80),base+0x9df1b6n,'wrong fallback caller');
    assert.deepEqual(vec(r.additive_2e0_bytes),before);
    const factor=data.readFloatLE(48),cutoff=data.readFloatLE(52);
    const xmm=Buffer.alloc(16);xmm.writeBigUInt64LE(BigInt(r.xmm0_lo));xmm.writeBigUInt64LE(BigInt(r.xmm0_hi),8);
    assert.deepEqual(vec(xmm.toString('hex')),[factor,factor,factor,factor]);
    assert.deepEqual(after,decay(before,factor,cutoff),'fallback recurrence mismatch');
  }
  const predicted=vec(a.output_bytes).map((v,i)=>f(v+(i<3?consumed[i]:0)));
  assert.deepEqual(vec(z.output_bytes),predicted,'consumer addition mismatch');
  return {called:true,delta:dt,fallback:!!fallback.length,before,after,consumed,
    activation_flag:owner[0x268],activation_enable:owner[0x269],pointed_gate8:input[24],pointed_gate9:input[25],pointed_gate10:input[26],pointed_vector:vec(a.input_bytes,32),live_factor:input.readFloatLE(48),
    nonzero_fallback:!!fallback.length && before.slice(0,3).some(x=>x!==0),
    nonzero_consumed:consumed.slice(0,3).some(x=>x!==0)};
}
if(process.argv[2]==='--self-test') {
  assert.deepEqual(decay([1,0,0,0],f(.65),f(.01)),[f(.65),0,0,0]);
  assert.deepEqual(decay([.1,0,0,9],f(.65),f(.01)),[0,0,0,0]);
  assert.throws(()=>decay([NaN,0,0,0],.65,.01));
  assert.throws(()=>pair([],0n));
  const hex=v=>{const b=Buffer.alloc(16);v.forEach((x,i)=>b.writeFloatLE(x,i*4));return b.toString('hex');};
  const input=Buffer.alloc(96);input.writeFloatLE(f(1/60),8);input.writeFloatLE(f(1/60),56);
  input.writeFloatLE(f(.65),48);input.writeFloatLE(f(.01),52);input.writeBigUInt64LE(0x9df1b6n,80);
  const factor=Buffer.from(hex([f(.65),f(.65),f(.65),f(.65)]),'hex');
  const common={owner_e0_address:'0x10000',tid:'1',trial:'1',input_bytes:input.toString('hex'),owner_70_bytes:Buffer.alloc(0x280).toString('hex'),rax:'0x0',output_bytes:hex([0,0,0,0]),additive_2e0_bytes:hex([1,0,0,0]),xmm0_lo:'0x'+factor.readBigUInt64LE(0).toString(16),xmm0_hi:'0x'+factor.readBigUInt64LE(8).toString(16)};
  const sample=[0x9df1b1,0x9dc04f,0x9df1b6,0x9df1f1].map((r,i)=>({...common,rip_at_trap:String(r),seq:String(i+1)}));
  sample[2].additive_2e0_bytes=sample[3].additive_2e0_bytes=hex([f(.65),0,0,0]);
  sample[3].output_bytes=hex([f(.65),0,0,0]);
  assert.equal(pair(sample,0n).nonzero_fallback,true);
  assert.throws(()=>pair(sample.filter((_,i)=>i!==1),0n));
  const corrupt=structuredClone(sample);corrupt[3].output_bytes=hex([1,0,0,0]);assert.throws(()=>pair(corrupt,0n));
  console.log('PASS: float32 decay, XYZ cutoff clearing all lanes, invalid input and missing consumer rejection.');
  console.log('PASS: complete nonzero fallback/consumer sequence; missing branch and corrupt consumer rejected.');
} else {
  const [dir,out]=process.argv.slice(2);assert(dir&&out,'Expected stopped session and output directory');
  const read=n=>fs.readFileSync(path.join(dir,n),'utf8');
  const csv=n=>{const [h,...ls]=read(n).trim().split(/\r?\n/),k=h.split(',');return ls.map(l=>Object.fromEntries(l.split(',').map((v,i)=>[k[i],v])));};
  const info=Object.fromEntries(read('session-info.txt').trim().split(/\r?\n/).map(l=>{const i=l.indexOf('=');return[l.slice(0,i),l.slice(i+1)];}));
  assert.equal(info.schema,'I-0007-accumulator-observer-v1');
  assert(read('COMPLETE').includes('debug registers restored'));
  const restored=csv('override-transitions.csv').filter(r=>r.action==='restore');
  assert(restored.length===1&&restored[0].success==='1'&&Number(restored[0].fps_after)===30&&restored[0].enable_after==='0');
  const events=csv('locomotion-events.csv');assert.equal(events.filter(r=>r.event==='measure_end').length,1);
  const rows=csv('accumulator-observer.csv'), groups=new Map();
  for(const r of rows) {
    assert(['input_valid','output_valid','owner_valid','additive_valid','nested_valid','code_valid'].every(k=>r[k]==='1'));
    assert.equal(r.overflow,'0');
    const key=r.trial+':'+r.broad_update_index;
    if(!groups.has(key))groups.set(key,[]);groups.get(key).push(r);
  }
  assert(groups.size>0);
  const windows=[...groups].map(([id,g])=>({id,...pair(g,BigInt(info.image_base))}));
  const result={session:path.basename(dir),intervals:windows.length,raw_hits:rows.length,
    fallback_intervals:windows.filter(w=>w.fallback).length,nonzero_fallback_intervals:windows.filter(w=>w.nonzero_fallback).length,
    nonzero_consumed_intervals:windows.filter(w=>w.nonzero_consumed).length,restored:true,
    activation_flag_values:[...new Set(windows.filter(w=>w.called).map(w=>w.activation_flag))],
    pointed_gate8_counts:Object.fromEntries([0,1].map(n=>[n,windows.filter(w=>w.called&&w.pointed_gate8===n).length])),
    live_factor_values:[...new Set(windows.filter(w=>w.called).map(w=>w.live_factor))],
    limits:'Exact local recurrence/consumer checks, not whole-motion or high-framerate validation. Zero-only fallback cannot establish harmful decay. Delta is forwarded input, not an inferred inter-call duration.',
    raw_sha256:Object.fromEntries(fs.readdirSync(dir).filter(n=>fs.statSync(path.join(dir,n)).isFile()).map(n=>[n,crypto.createHash('sha256').update(fs.readFileSync(path.join(dir,n))).digest('hex').toUpperCase()]))};
  fs.mkdirSync(out,{recursive:true});
  fs.writeFileSync(path.join(out,'accumulator-summary.json'),JSON.stringify(result,null,2)+'\n');
  fs.writeFileSync(path.join(out,'accumulator-windows.json'),JSON.stringify(windows,null,2)+'\n');
  console.log(JSON.stringify(result,null,2));
}
