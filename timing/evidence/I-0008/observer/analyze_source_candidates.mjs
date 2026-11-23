import fs from 'node:fs';
const [input,output]=process.argv.slice(2);const b=fs.readFileSync(input),n=Number(b.readBigUInt64LE(16));
const rows=[];const float=(v)=>{const x=Buffer.alloc(4);x.writeUInt32LE(v);return x.readFloatLE()};
for(let i=0;i<n;i++){const p=72+i*280;rows.push({i,qpc:b.readBigUInt64LE(p+8),tid:b.readBigUInt64LE(p+16).toString(),site:Number(b.readBigUInt64LE(p+24)),source:b.readBigUInt64LE(p+32).toString(16),module:b.readBigUInt64LE(p+56).toString(16),index:b.readUInt32LE(p+84),id:b.readUInt32LE(p+92),a:b.readUInt32LE(p+96),next:b.readUInt32LE(p+100),duration:b.readUInt32LE(p+104),time:b.readUInt32LE(p+108),delta:b.readUInt32LE(p+112)});}
const byThread=new Map(),matches={},ids={},deltas={};let unique=0,ambiguous=0,none=0;const examples=[];
for(const r of rows){if(r.site===0x1078330||r.site===0xf1b070){if(!byThread.has(r.tid))byThread.set(r.tid,[]);byThread.get(r.tid).push(r);}
if(r.site===0x9b4f8d){const reads=byThread.get(r.tid)??[];const sources=[...new Set(reads.map(x=>x.source))];const candidates=sources.filter(s=>reads.some(x=>x.source===s&&x.time===r.a)&&reads.some(x=>x.source===s&&x.time===r.next));
if(candidates.length===1)unique++;else if(candidates.length>1)ambiguous++;else none++;
for(const s of candidates)matches[s]=(matches[s]??0)+1;
ids[r.id]=(ids[r.id]??0)+1;const delta=float(r.next)-float(r.a);const key=delta.toFixed(9);deltas[key]=(deltas[key]??0)+1;
if(examples.length<15)examples.push({i:r.i,index:r.index,id:r.id,a:float(r.a),b:float(r.next),duration:float(r.duration),central_delta:float(r.delta),candidates});byThread.set(r.tid,[]);}}
const writes=rows.filter(r=>r.site===0x9b4f8d);const bits=(f)=>{const x=Buffer.alloc(4);x.writeFloatLE(f);return x.readUInt32LE()};
let replayExact=0,previousExitExact=0,wrapped=0;
for(let i=0;i<writes.length;i++){
 const r=writes[i];let predicted=Math.fround(float(r.a)+float(r.delta));
 const duration=float(r.duration);if(predicted>duration&&duration>0){predicted=Math.fround(predicted-Math.fround(Math.trunc(Math.fround(predicted/duration))*duration));wrapped++;}
 if(bits(Math.max(predicted,0))===r.next)replayExact++;
 if(i>0&&r.a===writes[i-1].next)previousExitExact++;
}
const result={unit_speed_zero_crop_sentinel_replay:{assumptions:'Hypothesis only: delta=central+268, speed1, crop0, unlimited loop. Neither actual control fields nor dt argument was captured.',exact:replayExact,total:writes.length,wraps:wrapped,previous_exit_to_next_entry_exact:previousExitExact,consecutive_pairs:Math.max(0,writes.length-1)},unique_value_pair_candidates:unique,ambiguous_value_pair_candidates:ambiguous,no_value_pair_candidate:none,candidate_counts:matches,record_ids:ids,b_minus_a_histogram:deltas,examples,limit:'Candidates require both exact float bits within the preceding same-thread insertion interval. This is value/order correlation, not a proven pointer dataflow join. Never promote a unique match alone to causal identity.'};fs.writeFileSync(output,JSON.stringify(result,null,2)+'\n');console.log(JSON.stringify(result,null,2));

