import fs from 'node:fs';
import crypto from 'node:crypto';
import {pathToFileURL} from 'node:url';
export function analyze(buffer) {
  if(buffer.length<72)throw new Error('Truncated header');
  const header=Array.from({length:9},(_,i)=>buffer.readBigUInt64LE(i*8));
  if(header[0]!==0x4930303852494e47n||header[1]!==280n)throw new Error('Schema mismatch');
  const n=Number(header[2]);if(buffer.length!==72+n*280)throw new Error('Truncated or trailing records');
  const failures=[];if(header[4]!==1n||header[5]!==1n||header[6]!==0n||header[7]!==0n)failures.push('Arming/restoration/overflow/thread-coverage gate');
  const pending=new Map(),sources=new Map();let pairs=0,leadingPost=0;
  const u64=['sequence','qpc','thread','site','source','caller','player','module','vtable','destination'];
  const u32=['valid','index','previous_index','id','a','b','duration','source_time','central_delta'];
  const rows=[];
  for(let i=0;i<n;i++){
    const base=72+i*280,r={};u64.forEach((k,j)=>r[k]=buffer.readBigUInt64LE(base+j*8));
    u32.forEach((k,j)=>r[k]=buffer.readUInt32LE(base+80+j*4));
    if(r.sequence!==BigInt(i)||r.valid!==1)failures.push(`Invalid sequence/row ${i}`);
    if(r.site===0xF1B070n||r.site===0x1078330n){const key=`${r.site.toString(16)}:${r.source.toString(16)}:${r.caller.toString(16)}`;sources.set(key,(sources.get(key)??0)+1);}
    else if(r.site===0x9B4F8Dn){if(pending.has(r.thread))failures.push(`Nested/unpaired pre ${i}`);pending.set(r.thread,r);}
    else if(r.site===0x9B4FC3n){
      const pre=pending.get(r.thread);pending.delete(r.thread);
      if(!pre){leadingPost++;continue;}
      const same=['player','module','vtable','destination','id','a','b','duration'].every(k=>pre[k]===r[k]);
      if(!same||r.index!==(pre.index+1)%10)failures.push(`Commit mismatch ${i}`);else pairs++;
    } else failures.push(`Unexpected site ${i}`);
    rows.push(r);
  }
  if(!pairs)failures.push('No qualified player insertion pair');
  if(!sources.size)failures.push('No source getter observations');
  return {schema_version:1,sha256:crypto.createHash('sha256').update(buffer).digest('hex').toUpperCase(),records:n,
    frequency:Number(header[3]),pairs,leading_unpaired_posts:leadingPost,trailing_unpaired_pres:pending.size,
    source_identities:[...sources].map(([key,count])=>({key,count})),failures,
    bounded_capture_gate:failures.length===0&&leadingPost===0&&pending.size===0,
    limits:'A passing capture establishes only read/copy identity and paired commits. Getter events include other actors. No last-getter heuristic is causal proof. Central +268 is a sentinel, not a bound input. Timing invariance remains unqualified.'};
}
if(process.argv[1]&&import.meta.url===pathToFileURL(process.argv[1]).href){
  const result=analyze(fs.readFileSync(process.argv[2]));
  fs.writeFileSync(process.argv[3],JSON.stringify(result,null,2)+'\n');console.log(JSON.stringify(result,null,2));
}
