import fs from 'node:fs';
import path from 'node:path';
import crypto from 'node:crypto';
import assert from 'node:assert/strict';
const f=Math.fround, add=(a,b)=>f(a+b), mul=(a,b)=>f(a*b), sub=(a,b)=>f(a-b);
function rotate(v,q) {
  const dot=add(add(mul(q[1],v[1]),mul(q[0],v[0])),mul(q[2],v[2]));
  const scale=sub(mul(q[3],q[3]),0.5);
  const cross=[sub(mul(v[2],q[1]),mul(q[2],v[1])),sub(mul(v[0],q[2]),mul(q[0],v[2])),sub(mul(v[1],q[0]),mul(q[1],v[0]))];
  return [0,1,2].map(i=>{const x=add(add(mul(dot,q[i]),mul(scale,v[i])),mul(cross[i],q[3]));return add(x,x);});
}
function vec(hex,offset=0,count=4) {const b=Buffer.from(hex,'hex');assert(b.length>=offset+count*4);return Array.from({length:count},(_,i)=>b.readFloatLE(offset+i*4));}
function xmm(r,n) {const b=Buffer.alloc(16);b.writeBigUInt64LE(BigInt(r[`xmm${n}_lo`]));b.writeBigUInt64LE(BigInt(r[`xmm${n}_hi`]),8);return vec(b.toString('hex'));}
if(process.argv[2]==='--self-test') {
  assert.deepEqual(rotate([1,2,3,0],[0,0,0,1]),[1,2,3]);
  const yaw=rotate([1,0,0,0],[0,f(Math.SQRT1_2),0,f(Math.SQRT1_2)]);
  assert(Math.abs(yaw[0])<1e-6 && Math.abs(yaw[2]+1)<1e-6);
  assert.throws(()=>vec('0000'));
  console.log('PASS: float32 rotation identity/yaw and truncated snapshot rejection.');
} else {
  const [sessionArg,outArg]=process.argv.slice(2);if(!sessionArg||!outArg)throw new Error('Expected session and analysis directories');
  const session=path.resolve(sessionArg),out=path.resolve(outArg);
  const read=n=>fs.readFileSync(path.join(session,n),'utf8');
  const csv=n=>{const [h,...ls]=read(n).trim().split(/\r?\n/),keys=h.split(',');return ls.filter(Boolean).map(l=>Object.fromEntries(l.split(',').map((v,i)=>[keys[i],v])));};
  const info=Object.fromEntries(read('session-info.txt').trim().split(/\r?\n/).map(l=>{const i=l.indexOf('=');return [l.slice(0,i),l.slice(i+1)];}));
  assert.equal(info.schema,'I-0007-velocity-source-mirror-v1');
  const base=BigInt(info.image_base),rva=r=>Number(BigInt(r.rip_at_trap)-base);
  const hits=csv('velocity-source-mirror.csv'), groups=new Map();
  for(const h of hits){const k=h.broad_update_index;if(!groups.has(k))groups.set(k,[]);groups.get(k).push(h);}
  const rows=[];let incomplete=0;
  for(const [index,hits] of groups) {
    const exec=hits.filter(h=>(BigInt(h.dr6)&7n)!==0n);
    if(exec.length!==3 || exec.map(rva).join(',')!==[0x9DEF77,0x9DEF7C,0x9DF3E6].join(',')){incomplete++;continue;}
    const [b,a,z]=exec;
    const writes=hits.filter(h=>(BigInt(h.dr6)&8n)!==0n);
    assert.equal(writes.length,1);
    const w=writes[0];
    assert.equal(rva(w),0x9DB9CC);
    assert.equal(Number(BigInt(w.function_begin)-base),0x9DB980);
    assert(w.code_bytes.includes('410F28000F2981E0000000'));
    assert(BigInt(w.seq)<BigInt(b.seq));
    assert(BigInt(w.rcx)+0xE0n===BigInt(w.owner_e0_address));
    assert.equal(w.r8,w.rbp);
    assert(exec.every(h=>h.rbp===b.rbp&&h.owner_e0_address===b.owner_e0_address&&BigInt(h.rbx)+0xE0n===BigInt(h.owner_e0_address)&&h.input_valid==='1'));
    assert(a.output_valid==='1'&&z.output_valid==='1');
    const input=vec(b.input_bytes),q=vec(b.input_bytes,16),timing=vec(b.input_bytes,32);
    const rotated=vec(a.output_bytes),afterOverrides=vec(z.output_bytes),velocity=vec(z.output_bytes,16);
    assert([...input,...q,...timing.slice(2),...rotated,...afterOverrides,...velocity].every(Number.isFinite));
    assert(timing[2]>0);
    const ownerBefore=Buffer.from(b.owner_70_bytes,'hex'), ownerFinal=Buffer.from(z.owner_70_bytes,'hex');
    const offsets=[0xa2,0xd0,0x130,0x160,0x1d8,0x1da,0x1db,0x1dc,0x1dd,0x1df];
    assert.deepEqual(xmm(w,0),vec(w.nested_bytes));
    assert.deepEqual(vec(w.nested_bytes),input);
    assert.deepEqual(vec(a.input_bytes),input);assert.deepEqual(vec(z.input_bytes),input);
    rows.push({update:Number(index),input,q,dt:timing[2],inverse_dt:timing[3],rotated,afterOverrides,velocity,
      source_address:w.r8,source_builder_address:'0x'+(BigInt(w.r8)-0x1D0n).toString(16),
      source_copy_exact:true,quaternion_norm:Math.hypot(...q),
      rotation_local_unchanged:rotated.every((x,i)=>Object.is(x,afterOverrides[i])),
      horizontal_inverse_exact:[0,2].every(i=>Object.is(mul(rotated[i],timing[3]),velocity[i])),
      rotation_xyz_exact:rotate(input,q).every((x,i)=>Object.is(x,rotated[i])),
      flags_before:Object.fromEntries(offsets.map(o=>['0x'+o.toString(16),ownerBefore[o-0x70]])),
      flags_final:Object.fromEntries(offsets.map(o=>['0x'+o.toString(16),ownerFinal[o-0x70]])),
      owner_180:vec(z.owner_70_bytes,0x180-0x70),owner_120:vec(z.owner_70_bytes,0x120-0x70),additive_2e0:vec(z.additive_2e0_bytes)});
  }
  const dt=rows.reduce((s,r)=>s+r.dt,0),norm=v=>Math.hypot(...v.slice(0,3));
  const events=csv('locomotion-events.csv'),restores=csv('override-transitions.csv').filter(r=>r.action==='restore');
  const summary={session:path.basename(session),target:Number(info.target_fps),raw_hits:hits.length,intervals:groups.size,complete_triples:rows.length,incomplete,
    measurement_rows:csv('locomotion-updates.csv').filter(r=>r.phase==='measure').length,
    input_norm_per_delta:rows.reduce((s,r)=>s+norm(r.input),0)/dt,
    rotated_norm_per_delta:rows.reduce((s,r)=>s+norm(r.rotated),0)/dt,
    velocity_norm_time_weighted:rows.reduce((s,r)=>s+norm(r.velocity)*r.dt,0)/dt,
    rotation_xyz_exact:rows.filter(r=>r.rotation_xyz_exact).length,
    source_copy_exact:rows.filter(r=>r.source_copy_exact).length,
    rotation_local_unchanged:rows.filter(r=>r.rotation_local_unchanged).length,
    horizontal_inverse_exact:rows.filter(r=>r.horizontal_inverse_exact).length,
    quaternion_norm_range:[Math.min(...rows.map(r=>r.quaternion_norm)),Math.max(...rows.map(r=>r.quaternion_norm))],
    branch_flag_sets:[...new Set(rows.map(r=>JSON.stringify(r.flags_before)))].map(x=>JSON.parse(x)),
    write_post_rvas:[...new Set(hits.filter(r=>(BigInt(r.dr6)&8n)!==0n).map(r=>'0x'+rva(r).toString(16)))],
    unique_owners:new Set(hits.map(r=>r.owner_e0_address)).size,unique_threads:new Set(hits.map(r=>r.tid)).size,
    invalid_rows:hits.filter(r=>r.overflow!=='0'||r.code_valid!=='1'||r.nested_valid!=='1'||r.owner_valid!=='1'||r.additive_valid!=='1').length,
    complete_trial:events.filter(r=>r.event==='measure_end').length===1&&!events.some(r=>r.event.startsWith('aborted')),
    timing_restored:restores.length===1&&restores[0].success==='1'&&restores[0].fps_after==='30'&&restores[0].enable_after==='0',
    restoration_report:read('COMPLETE').trim(),
    raw_sha256:Object.fromEntries(fs.readdirSync(session).filter(n=>fs.statSync(path.join(session,n)).isFile()).map(n=>[n,crypto.createHash('sha256').update(fs.readFileSync(path.join(session,n))).digest('hex').toUpperCase()]))};
  summary.capture_gates_pass=rows.length>0&&rows.length===summary.measurement_rows-1&&!incomplete&&!summary.invalid_rows&&summary.unique_owners===1&&summary.unique_threads===1&&summary.complete_trial&&summary.timing_restored;
  fs.mkdirSync(out,{recursive:true});fs.writeFileSync(path.join(out,'velocity-summary.json'),JSON.stringify(summary,null,2)+'\n');fs.writeFileSync(path.join(out,'velocity-stages.json'),JSON.stringify(rows,null,2)+'\n');
  console.log(JSON.stringify(summary,null,2));
}
