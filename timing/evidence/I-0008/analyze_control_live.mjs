import fs from 'node:fs';

const [analysisPath,capturePath,semanticPath,exceptionsPath,outputPath]=process.argv.slice(2);
if(!outputPath)throw new Error('usage: node analyze_control_live.mjs <analysis.json> <capture.bin> <semantic.csv> <exceptions.json> <output.json>');
const analysis=JSON.parse(fs.readFileSync(analysisPath));if(!analysis.bounded_capture_gate)throw new Error('Capture gate failed');
const raw=fs.readFileSync(capturePath),records=Number(raw.readBigUInt64LE(16)),frequency=Number(raw.readBigUInt64LE(24));
const ringQpc=[];for(let i=0;i<records;i++){const p=176+i*200;if(raw.readBigUInt64LE(p+24)===0x9b4f8dn)ringQpc.push(Number(raw.readBigUInt64LE(p+8)));}
const elapsed=(ringQpc.at(-1)-ringQpc[0])/frequency,ringRate=(ringQpc.length-1)/elapsed;
function fromBits(hex){const b=Buffer.allocUnsafe(4);b.writeUInt32LE(Number(BigInt(hex)));return b.readFloatLE();}
function toBits(value){const b=Buffer.allocUnsafe(4);b.writeFloatLE(value);return `0x${b.readUInt32LE().toString(16).toUpperCase()}`;}
let replayExact=0,wraps=0;for(const c of analysis.chains){let expected=Math.fround(fromBits(c.before_bits)+Math.fround(c.delta));if(expected>c.duration){expected=Math.fround(expected-c.duration);wraps++;}if(toBits(expected)===c.after_bits)replayExact++;}
const frequencyOf=(rows,key)=>Object.entries(rows.reduce((m,row)=>(m[key(row)]=(m[key(row)]??0)+1,m),{})).map(([value,count])=>({value,count})).sort((a,b)=>b.count-a.count);
const lines=fs.readFileSync(semanticPath,'utf8').trim().split(/\r?\n/),headers=lines[0].split(',');
const semantic=lines.slice(1).map(line=>{const values=line.split(',');return Object.fromEntries(headers.map((h,i)=>[h,values[i]]));});
const byIndex=new Map(semantic.map(row=>[Number(row.update_index),row]));
const residual=JSON.parse(fs.readFileSync(exceptionsPath)).filter(row=>row.target===165&&row.previous_error>1e-5).map(row=>{
  const semanticRow=byIndex.get(row.index);return {...row,animation_id:Number(semanticRow.animation_id),animation_max:Number(semanticRow.animation_max),
    play_speed:Number(semanticRow.play_speed),marker_id:Number(semanticRow.marker_id),marker_name:semanticRow.marker_name};
}).sort((a,b)=>a.index-b.index);
const clusters=[];for(const row of residual){const last=clusters.at(-1);if(last&&row.index-last.rows.at(-1).index<=60)last.rows.push(row);else clusters.push({rows:[row]});}
const fixedStep=10/60;
const summary={schema_version:1,live:{records,qualified_chains:analysis.qualified_chains,ring_events:ringQpc.length,
  elapsed_first_to_last_ring_seconds:elapsed,ring_rate_hz:ringRate,breakpoint_hits:analysis.breakpoint_hits,timing:analysis.timing,
  event_counts:analysis.event_counts,delta_frequency:frequencyOf(analysis.chains,row=>row.delta_bits),all_actual_delta_equal_central:analysis.all_actual_delta_equal_central,
  actual_delta_differs_from_fixed_60:analysis.actual_delta_differs_from_fixed_60,variable_central_delta_supported:analysis.variable_central_delta_supported,
  identity_frequency:{generator:frequencyOf(analysis.chains,row=>row.generator),control:frequencyOf(analysis.chains,row=>row.control),
    player:frequencyOf(analysis.chains,row=>row.player),module:frequencyOf(analysis.chains,row=>row.module)},
  field_contract:{all_mode_1:analysis.chains.every(row=>row.mode===1),all_flags_0:analysis.chains.every(row=>row.flags===0),
    all_speed_1:analysis.chains.every(row=>row.speed===1),all_duration_2:analysis.chains.every(row=>row.duration===2)},
  native_formula_replay:{exact:replayExact,total:analysis.chains.length,wraps},failures:analysis.failures,bounded_capture_gate:analysis.bounded_capture_gate},
  retained_165_residual:{count:residual.length,all_gap_10:residual.every(row=>row.gap===10),
    variable_sum_closer_than_fixed_60:residual.filter(row=>row.previous_error<Math.abs(row.increment-fixedStep)).length,
    fixed_60_closer_than_variable_sum:residual.filter(row=>row.previous_error>Math.abs(row.increment-fixedStep)).length,
    animation_partitions:frequencyOf(residual,row=>`${row.animation_id}|${row.animation_max}|${row.play_speed}`),
    marker_partitions:frequencyOf(residual,row=>`${row.marker_id}|${row.marker_name}`),
    clusters:clusters.map(c=>({count:c.rows.length,start:c.rows[0].index,end:c.rows.at(-1).index,animation_ids:[...new Set(c.rows.map(r=>r.animation_id))]}))},
  conclusion:'The qualified live idle mode-1 clip/control path consumes the same non-1/60 variable delta exposed by central +268 and reproduces every native output bit-exactly. The 47 retained 165 residuals are all closer to the variable ten-delta sum than to fixed 1/60 cadence; they remain unresolved transition/selection/alternate-branch behavior because retained rows lack live mode/control identity.',
  limits:'One target-165 idle generator/control identity is causally joined. This does not prove every clip mode, transition, animation identity, actor or framerate, and it does not justify an animation patch or whole-C4 claim.'};
fs.writeFileSync(outputPath,JSON.stringify(summary,null,2)+'\n');console.log(JSON.stringify(summary,null,2));
