import fs from 'node:fs';
import path from 'node:path';
import crypto from 'node:crypto';
import assert from 'node:assert/strict';

function floats(hex, offset, count=4) {
  const b=Buffer.from(hex,'hex');
  if (b.length<offset+count*4) throw new Error('Truncated byte snapshot');
  return Array.from({length:count},(_,i)=>b.readFloatLE(offset+i*4));
}
function decodePair(before,after) {
  assert.equal(before.rbp,after.rbp);
  assert.equal(before.nested_a0_address,after.nested_a0_address);
  assert.equal(before.input_valid,'1'); assert.equal(after.output_valid,'1');
  const initial=floats(before.input_bytes,0), input=floats(before.input_bytes,0x10);
  const output=floats(after.output_bytes,0x10), displacement=floats(after.output_bytes,0);
  const dt=floats(before.input_bytes,0x40,1)[0], time=floats(after.output_bytes,0x20,1)[0];
  const count=Buffer.from(before.input_bytes,'hex').readInt32LE(0x50);
  const nested=floats(before.nested_bytes,0);
  assert([...initial,...input,...output,...displacement,dt,time].every(Number.isFinite));
  assert(dt>0 && time>=0 && count>=0);
  return {update:Number(before.broad_update_index),initial,input,output,displacement,dt,time,count,
    input_matches_nested:input.every((v,i)=>Object.is(v,nested[i])),
    output_matches_input:input.every((v,i)=>Object.is(v,output[i])),
    exact_integral:displacement.every((v,i)=>Object.is(v,Math.fround(initial[i]+Math.fround(output[i]*time))))};
}
if(process.argv[2]==='--self-test') {
  const input=Buffer.alloc(0x60), output=Buffer.alloc(0x30), nested=Buffer.alloc(0x20);
  [6,0,0,0].forEach((v,i)=>{input.writeFloatLE(v,0x10+i*4);nested.writeFloatLE(v,i*4);output.writeFloatLE(v,0x10+i*4);});
  input.writeFloatLE(1/60,0x40); output.writeFloatLE(1/60,0x20);
  output.writeFloatLE(Math.fround(6*Math.fround(1/60)),0);
  const b={rbp:'0x1000',nested_a0_address:'0x2000',input_valid:'1',output_valid:'1',broad_update_index:'7',input_bytes:input.toString('hex'),nested_bytes:nested.toString('hex'),output_bytes:output.toString('hex')};
  let pair=decodePair(b,b); assert(pair.output_matches_input && pair.exact_integral && pair.input_matches_nested);
  output.writeFloatLE(3,0x10);output.writeFloatLE(Math.fround(3*Math.fround(1/60)),0);
  pair=decodePair(b,{...b,output_bytes:output.toString('hex')});
  assert(!pair.output_matches_input && pair.exact_integral);
  assert.throws(()=>decodePair(b,{...b,rbp:'0x1001'}));
  assert.throws(()=>decodePair(b,{...b,output_valid:'0'}));
  console.log('PASS: unchanged/reduced output, float32 product, pairing and invalid-read rejection.');
} else {
  const [sessionArg,outputArg]=process.argv.slice(2);
  if(!sessionArg || !outputArg) throw new Error('Expected session and output directories');
  const session=path.resolve(sessionArg), output=path.resolve(outputArg);
  const read=name=>fs.readFileSync(path.join(session,name),'utf8');
  const csv=name=>{const [h,...lines]=read(name).trim().split(/\r?\n/);const keys=h.split(',');return lines.filter(Boolean).map(l=>Object.fromEntries(l.split(',').map((v,i)=>[keys[i],v])));};
  const info=Object.fromEntries(read('session-info.txt').trim().split(/\r?\n/).map(l=>{const i=l.indexOf('=');return [l.slice(0,i),l.slice(i+1)];}));
  assert.equal(info.schema,'I-0007-increment-producer-mirror-v1');
  const base=BigInt(info.image_base), hits=csv('increment-producer-mirror.csv');
  const rva=r=>Number(BigInt(r.rip_at_trap)-base);
  const groups=new Map();
  for(const h of hits){const key=h.broad_update_index;if(!groups.has(key)) groups.set(key,[]);groups.get(key).push(h);}
  const pairs=[];let incomplete=0, orderedQuads=0, setterMatchesInput=0, feedbackMatchesOutput=0, setterFrameMatches=0;
  const xmmHex=(r,k)=>{const b=Buffer.alloc(16);b.writeBigUInt64LE(BigInt(r[k+'_lo']));b.writeBigUInt64LE(BigInt(r[k+'_hi']),8);return b.toString('hex');};
  for(const [key,rows] of groups){
    const exec=rows.filter(r=>(BigInt(r.dr6)&3n)!==0n);
    if(exec.length!==2 || rva(exec[0])!==0x1669A1E || rva(exec[1])!==0x1669A23){incomplete++;continue;}
    assert(exec.every(r=>BigInt(r.rdi)+0xA0n===BigInt(r.nested_a0_address)));
    pairs.push(decodePair(exec[0],exec[1]));
    if(rows.length===4 && rows.map(r=>rva(r)).join(',')===[0xDDA785,0x1669A1E,0x1669A23,0x1669FCB].join(',')) {
      orderedQuads++;
      const [setter,,,feedback]=rows;
      const input=exec[0].input_bytes.slice(32,64).toLowerCase();
      const out=exec[1].output_bytes.slice(32,64).toLowerCase();
      if(setter.nested_bytes.slice(0,32).toLowerCase()===input && xmmHex(setter,'xmm0')===input) setterMatchesInput++;
      if(feedback.nested_bytes.slice(0,32).toLowerCase()===out && xmmHex(feedback,'xmm13')===out) feedbackMatchesOutput++;
      if(BigInt(setter.rax)+0xA0n===BigInt(setter.nested_a0_address) && BigInt(setter.rdi)===BigInt(setter.rbp)-0x69n) setterFrameMatches++;
    }
  }
  const dt=pairs.reduce((s,p)=>s+p.dt,0);
  const weighted=key=>pairs.reduce((s,p)=>s+Math.hypot(...p[key].slice(0,3))*p.dt,0)/dt;
  const events=csv('locomotion-events.csv'), transitions=csv('override-transitions.csv');
  const restored=transitions.filter(r=>r.action==='restore');
  const summary={session:path.basename(session),target:Number(info.target_fps),raw_hits:hits.length,intervals:groups.size,pairs:pairs.length,incomplete,
    input_norm_time_weighted:weighted('input'),output_norm_time_weighted:weighted('output'),sum_input_dt:dt,
    input_matches_nested:pairs.filter(p=>p.input_matches_nested).length,
    output_matches_input:pairs.filter(p=>p.output_matches_input).length,
    exact_integrals:pairs.filter(p=>p.exact_integral).length,
    ordered_four_hit_windows:orderedQuads,setter_matches_input:setterMatchesInput,
    feedback_matches_output:feedbackMatchesOutput,setter_source_matches_009DEE60_frame:setterFrameMatches,
    count_values:[...new Set(pairs.map(p=>p.count))],
    write_post_rvas:[...new Set(hits.filter(r=>(BigInt(r.dr6)&4n)!==0n).map(r=>'0x'+rva(r).toString(16)))],
    unique_addresses:new Set(hits.map(r=>r.nested_a0_address)).size,unique_threads:new Set(hits.map(r=>r.tid)).size,
    invalid_rows:hits.filter(r=>r.overflow!=='0'||r.code_valid!=='1'||r.nested_valid!=='1').length,
    complete_trial:events.filter(r=>r.event==='measure_end').length===1&&!events.some(r=>r.event.startsWith('aborted')),
    timing_restored:restored.length===1&&restored[0].success==='1'&&restored[0].fps_after==='30'&&restored[0].enable_after==='0',
    restoration_report:read('COMPLETE').trim(),
    raw_sha256:Object.fromEntries(fs.readdirSync(session).filter(f=>fs.statSync(path.join(session,f)).isFile()).map(f=>[f,crypto.createHash('sha256').update(fs.readFileSync(path.join(session,f))).digest('hex').toUpperCase()]))};
  summary.measurement_rows=csv('locomotion-updates.csv').filter(r=>r.phase==='measure').length;
  summary.capture_gates_pass=pairs.length>0&&pairs.length===summary.measurement_rows-1&&!incomplete&&!summary.invalid_rows&&summary.unique_addresses===1&&summary.unique_threads===1&&summary.complete_trial&&summary.timing_restored;
  fs.mkdirSync(output,{recursive:true});
  fs.writeFileSync(path.join(output,'increment-summary.json'),JSON.stringify(summary,null,2)+'\n');
  fs.writeFileSync(path.join(output,'increment-pairs.json'),JSON.stringify(pairs,null,2)+'\n');
  console.log(JSON.stringify(summary,null,2));
}
