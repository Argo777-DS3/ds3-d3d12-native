import fs from 'node:fs';
import crypto from 'node:crypto';
import {pathToFileURL} from 'node:url';

const headerBytes=22*8,recordBytes=200;
const u64Names=['sequence','qpc','thread','site','object','context','control','out_ptr','loops_ptr','player','module','object_vtable','control_vtable','destination','caller'];
const u32Names=['valid','delta','central_delta','generator_time','control_current','speed','start','end','duration','mode','flags','loop_limit','forward_count','reverse_count','out_time','out_loops','id','ring_a','ring_b','ring_duration'];
const sites={generator:0x10768c0n,entry:0x1284d70n,exit:0x1284ea8n,ring:0x9b4f8dn};
function f32(bits){const b=Buffer.allocUnsafe(4);b.writeUInt32LE(bits);return b.readFloatLE(0);}
function hex(value){return `0x${value.toString(16).toUpperCase()}`;}
function parseRecord(buffer,base){
  const r={};u64Names.forEach((name,i)=>r[name]=buffer.readBigUInt64LE(base+i*8));
  u32Names.forEach((name,i)=>r[name]=buffer.readUInt32LE(base+120+i*4));return r;
}
export function analyze(buffer){
  if(buffer.length<headerBytes)throw new Error('Truncated header');
  const h=Array.from({length:22},(_,i)=>buffer.readBigUInt64LE(i*8));
  if(h[0]!==0x493030384354524cn||h[1]!==BigInt(recordBytes))throw new Error('Schema mismatch');
  const n=Number(h[2]);if(!Number.isSafeInteger(n)||buffer.length!==headerBytes+n*recordBytes)throw new Error('Truncated or trailing records');
  const failures=[];
  if(h[4]!==1n)failures.push('Observer did not arm');
  if(h[5]!==1n)failures.push('Debug registers were not restored');
  if(h[6]!==0n)failures.push('Capture overflow');
  if(h[7]!==0n)failures.push('Uncovered new thread');
  if(h.slice(9,13).some(v=>v===0n))failures.push('At least one breakpoint site did not execute');
  if(h[13]!==1n)failures.push('165 FPS timing override was not applied');
  if(h[14]!==1n||h[16]!==h[20]||h[17]!==h[21])failures.push('Timing override was not restored exactly');
  if(h[18]!==0x43250000n||h[19]!==1n)failures.push('Timing header is inconsistent');
  const generators=new Map(),active=new Map(),completed=new Map();
  const selected=[];let leadingUnmatched=0,internalUnmatched=0,ambiguous=0,seenMatch=false;
  const stats={generator:0,control_entry:0,control_exit:0,ring:0,unrelated_control:0};
  const key=r=>r.thread.toString();
  for(let i=0;i<n;i++){
    const r=parseRecord(buffer,headerBytes+i*recordBytes);
    if(r.sequence!==BigInt(i)||r.valid!==1){failures.push(`Invalid sequence/row ${i}`);continue;}
    const k=key(r);
    if(r.site===sites.generator){
      stats.generator++;const list=generators.get(k)??[];list.push({record:r,entry:null});generators.set(k,list);
    }else if(r.site===sites.entry){
      stats.control_entry++;const list=generators.get(k)??[];
      const gen=[...list].reverse().find(x=>!x.entry&&x.record.control===r.control&&x.record.generator_time===r.control_current&&x.record.delta===r.delta);
      if(gen)gen.entry=r;else stats.unrelated_control++;
      const stack=active.get(k)??[];stack.push({entry:r,generator:gen?.record??null});active.set(k,stack);
    }else if(r.site===sites.exit){
      stats.control_exit++;const stack=active.get(k)??[];
      let at=-1;for(let j=stack.length-1;j>=0;j--)if(stack[j].entry.control===r.control&&stack[j].entry.out_ptr===r.out_ptr&&stack[j].entry.loops_ptr===r.loops_ptr){at=j;break;}
      if(at<0){stats.unrelated_control++;continue;}
      const call=stack.splice(at,1)[0];active.set(k,stack);
      if(call.generator){const list=completed.get(k)??[];list.push({generator:call.generator,entry:call.entry,exit:r,used:false});completed.set(k,list);}
    }else if(r.site===sites.ring){
      stats.ring++;const list=completed.get(k)??[];
      const candidates=list.filter(x=>!x.used&&x.generator.player===r.player&&x.generator.module===r.module&&
        x.generator.control===x.entry.control&&x.generator.generator_time===r.ring_a&&x.entry.control_current===r.ring_a&&
        x.exit.out_time===r.ring_b&&x.entry.duration===r.ring_duration&&x.generator.delta===x.entry.delta);
      if(candidates.length===1){
        const match=candidates[0];match.used=true;seenMatch=true;selected.push({generator:match.generator,entry:match.entry,exit:match.exit,ring:r});
      }else if(candidates.length>1){ambiguous++;failures.push(`Ambiguous ring chain at row ${i}`);}
      else if(seenMatch){internalUnmatched++;failures.push(`Unmatched ring after first chain at row ${i}`);}
      else leadingUnmatched++;
    }else failures.push(`Unexpected site ${hex(r.site)} at row ${i}`);
  }
  if(!selected.length)failures.push('No exact generator/control/ring chain');
  if(leadingUnmatched>1)failures.push('More than one leading boundary ring was unmatched');
  const actualDeltaBits=[...new Set(selected.map(x=>x.entry.delta))];
  const centralExact=selected.length>0&&selected.every(x=>x.entry.delta===x.entry.central_delta&&x.generator.delta===x.generator.central_delta);
  const nonFixed60=actualDeltaBits.some(bits=>Math.abs(f32(bits)-1/60)>1e-5);
  const variableSupported=centralExact&&nonFixed60;
  const chains=selected.map((x,index)=>({index,thread:Number(x.ring.thread),generator:hex(x.generator.object),context:hex(x.generator.context),
    control:hex(x.entry.control),player:hex(x.ring.player),module:hex(x.ring.module),delta_bits:hex(BigInt(x.entry.delta)),delta:f32(x.entry.delta),
    central_delta_bits:hex(BigInt(x.entry.central_delta)),before_bits:hex(BigInt(x.ring.ring_a)),after_bits:hex(BigInt(x.ring.ring_b)),
    duration:f32(x.ring.ring_duration),speed:f32(x.entry.speed),mode:x.generator.mode,flags:x.generator.flags,loops:x.exit.out_loops}));
  return {schema_version:1,sha256:crypto.createHash('sha256').update(buffer).digest('hex').toUpperCase(),records:n,frequency:Number(h[3]),
    breakpoint_hits:h.slice(9,13).map(Number),timing:{object:hex(h[15]),original_fps:f32(Number(h[16])),original_enable:Number(h[17]),
      target_fps:f32(Number(h[18])),target_enable:Number(h[19]),restored_fps:f32(Number(h[20])),restored_enable:Number(h[21])},
    event_counts:stats,qualified_chains:selected.length,leading_unmatched_rings:leadingUnmatched,internal_unmatched_rings:internalUnmatched,ambiguous_rings:ambiguous,
    actual_delta_bits:actualDeltaBits.map(v=>hex(BigInt(v))),all_actual_delta_equal_central:centralExact,
    actual_delta_differs_from_fixed_60:nonFixed60,variable_central_delta_supported:variableSupported,chains,failures,
    bounded_capture_gate:failures.length===0,
    limits:'A passing capture binds exact values and call order for qualified same-thread chains. It does not prove every animation state, transition, actor, framerate or whole-C4 correctness.'};
}
if(process.argv[1]&&import.meta.url===pathToFileURL(process.argv[1]).href){
  const result=analyze(fs.readFileSync(process.argv[2]));fs.writeFileSync(process.argv[3],JSON.stringify(result,null,2)+'\n');
  console.log(JSON.stringify(result,null,2));if(!result.bounded_capture_gate)process.exitCode=2;
}
