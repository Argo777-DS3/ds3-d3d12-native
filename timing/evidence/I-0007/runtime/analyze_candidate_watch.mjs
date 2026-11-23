// Analyze the exact candidate-write sequence, without modifying raw evidence.
import fs from 'node:fs';
import path from 'node:path';
import crypto from 'node:crypto';
const [sessionArg, outputArg] = process.argv.slice(2);
if (!sessionArg || !outputArg) throw new Error('Expected session and output directories');
const session = path.resolve(sessionArg), output = path.resolve(outputArg);
const read = name => fs.readFileSync(path.join(session, name), 'utf8');
function csv(name) {
  const [header, ...lines] = read(name).trim().split(/\r?\n/);
  const keys = header.split(',');
  return lines.filter(Boolean).map(line => Object.fromEntries(line.split(',').map((v,i) => [keys[i],v])));
}
const info = Object.fromEntries(read('session-info.txt').trim().split(/\r?\n/).map(line => {
  const i=line.indexOf('='); return [line.slice(0,i),line.slice(i+1)];
}));
const base=BigInt(info.image_base);
const rva=address => Number(BigInt(address)-base);
function vector(row, reg) {
  const bytes=Buffer.alloc(16);
  bytes.writeBigUInt64LE(BigInt(row[reg+'_lo']),0);
  bytes.writeBigUInt64LE(BigInt(row[reg+'_hi']),8);
  return [0,4,8,12].map(i=>bytes.readFloatLE(i));
}
const watch=csv('candidate-write-watch.csv');
const updates=csv('locomotion-updates.csv').filter(r=>r.phase==='measure');
const updateMap=new Map(updates.map(r=>[Number(r.update_index),r]));
const groups=new Map();
for(const row of watch) {
  const key=Number(row.broad_update_index);
  if(!groups.has(key)) groups.set(key,[]);
  groups.get(key).push(row);
}
let sequenceOK=0, addOK=0, addressOK=0, finalHorizontalPreserved=0, paired=0, endMatches=0;
let delta=0, pathLength=0;
const sum=[0,0,0];
const rows=[];
for(const [update, hits] of groups) {
  if(hits.length!==3 || hits.map(h=>rva(h.rip_after)).join(',')!==[0xDDA8A2,0x1669D6E,0xDDA8A2].join(',')) continue;
  sequenceOK++;
  const [first,middle,last]=hits;
  const before=vector(first,'xmm0'), after=vector(middle,'xmm0'), increment=vector(middle,'xmm6'), end=vector(last,'xmm0');
  if([0,1,2,3].every(i=>Object.is(Math.fround(before[i]+increment[i]),after[i]))) addOK++;
  if(BigInt(middle.rdi)+0x70n===BigInt(middle.candidate_address) && [first,last].every(h=>BigInt(h.rax)+0x70n===BigInt(h.candidate_address))) addressOK++;
  if(end[0]===after[0] && end[2]===after[2]) finalHorizontalPreserved++;
  const current=updateMap.get(update), next=updateMap.get(update+1);
  if(current && next) {
    paired++;
    if(['x','z','y'].every((axis,i)=>Math.fround(Number(next['after_'+axis]))===end[i])) endMatches++;
    delta+=Number(current.delta);
    pathLength+=Math.hypot(...increment.slice(0,3));
    for(let i=0;i<3;i++) sum[i]+=increment[i];
  }
  rows.push({update,before,increment,after,final:end});
}
const events=csv('locomotion-events.csv');
const restores=csv('override-transitions.csv').filter(r=>r.action==='restore');
const summary={
  session:path.basename(session),target:Number(info.target_fps),
  watch_sha256:crypto.createHash('sha256').update(fs.readFileSync(path.join(session,'candidate-write-watch.csv'))).digest('hex').toUpperCase(),
  raw_hits:watch.length,watched_updates:groups.size,measurement_rows:updates.length,
  writer_rva_counts:Object.fromEntries([...new Set(watch.map(r=>rva(r.rip_after)))].map(r=>['0x'+r.toString(16),watch.filter(h=>rva(h.rip_after)===r).length])),
  unique_candidate_addresses:new Set(watch.map(r=>r.candidate_address)).size,
  unique_threads:new Set(watch.map(r=>r.tid)).size,
  overflow_rows:watch.filter(r=>r.overflow!=='0').length,
  invalid_code_rows:watch.filter(r=>r.code_valid!=='1').length,
  ordered_triplets:sequenceOK,bit_exact_four_lane_additions:addOK,verified_target_addresses:addressOK,
  final_copy_preserves_horizontal_increment:finalHorizontalPreserved,
  next_update_pairs:paired,next_update_endpoint_matches:endMatches,
  paired_delta:delta,increment_sum:sum,increment_path_per_delta:delta?pathLength/delta:null,
  complete_trial:events.filter(r=>r.event==='measure_end').length===1 && !events.some(r=>r.event.startsWith('aborted')),
  restoration_report:read('COMPLETE').trim(),
  timing_restored:restores.length===1 && restores[0].success==='1' && restores[0].fps_after==='30' && restores[0].enable_after==='0',
  limitation:'Writer localization and local addition only; upstream operands, causal frame dependency and patch remain unproven. Capture retains inherited I-0006 schema label; I-0007 condition and binary manifest identify this instrument.'
};
summary.localization_gates_pass=watch.length>0 && sequenceOK===groups.size && addOK===groups.size && addressOK===groups.size && finalHorizontalPreserved===groups.size && paired===groups.size && endMatches===groups.size && summary.unique_candidate_addresses===1 && summary.unique_threads===1 && !summary.overflow_rows && !summary.invalid_code_rows && summary.complete_trial && summary.timing_restored;
fs.mkdirSync(output,{recursive:true});
fs.writeFileSync(path.join(output,'candidate-watch-summary.json'),JSON.stringify(summary,null,2)+'\n');
fs.writeFileSync(path.join(output,'candidate-write-sequences.json'),JSON.stringify(rows,null,2)+'\n');
console.log(JSON.stringify(summary,null,2));
