// Compare immutable candidate-watch captures; no gameplay semantics assigned.
import fs from 'node:fs';
import path from 'node:path';
import assert from 'node:assert/strict';
const [rootArg, outputArg] = process.argv.slice(2);
if (!rootArg || !outputArg) throw new Error('Expected runtime root and output file');
const root = path.resolve(rootArg);
function csv(file) {
  const [header, ...lines] = fs.readFileSync(file, 'utf8').trim().split(/\r?\n/);
  const keys = header.split(',');
  return lines.map(line => Object.fromEntries(line.split(',').map((v,i)=>[keys[i],v])));
}
function vector(row, reg) {
  const b=Buffer.alloc(16);
  b.writeBigUInt64LE(BigInt(row[reg+'_lo']));
  b.writeBigUInt64LE(BigInt(row[reg+'_hi']),8);
  return [0,4,8,12].map(i=>b.readFloatLE(i));
}
function analyze(session) {
  const dir=path.join(root,'sessions',session);
  const info=fs.readFileSync(path.join(dir,'session-info.txt'),'utf8');
  const base=BigInt(info.match(/^image_base=(.+)$/m)[1].trim());
  const hits=csv(path.join(dir,'candidate-write-watch.csv'))
    .filter(r=>BigInt(r.rip_after)-base===0x1669D6En);
  const updates=new Map(csv(path.join(dir,'locomotion-updates.csv'))
    .filter(r=>r.phase==='measure').map(r=>[r.update_index,r]));
  const summary=JSON.parse(fs.readFileSync(path.join(root,'analysis',session,'candidate-watch-summary.json'),'utf8'));
  assert.equal(summary.localization_gates_pass,true);
  let exactProduct=0, zeroRemaining=0, exactDelta=0, delta=0, velocityNormTime=0;
  let stepTime=0, maxResidual=0, maxTimeDifference=0, minTimeRatio=Infinity, maxTimeRatio=0;
  for (const hit of hits) {
    const displacement=vector(hit,'xmm6'), v=vector(hit,'xmm13');
    const time=vector(hit,'xmm10')[0], remaining=vector(hit,'xmm8')[0];
    const dt=Number(updates.get(hit.broad_update_index).delta);
    if (displacement.every((x,i)=>Object.is(x,Math.fround(v[i]*time)))) exactProduct++;
    if (remaining===0) zeroRemaining++;
    if (Object.is(Math.fround(dt),time)) exactDelta++;
    maxTimeDifference=Math.max(maxTimeDifference,Math.abs(time-Math.fround(dt)));
    minTimeRatio=Math.min(minTimeRatio,time/dt); maxTimeRatio=Math.max(maxTimeRatio,time/dt);
    for(let i=0;i<3;i++) maxResidual=Math.max(maxResidual,Math.abs(displacement[i]-v[i]*time));
    delta+=dt; stepTime+=time; velocityNormTime+=Math.hypot(...v.slice(0,3))*dt;
  }
  return {session,target:summary.target,intervals:hits.length,
    four_lane_exact_displacement_equals_velocity_times_time:exactProduct,
    time_bit_exact_broad_delta:exactDelta,zero_remaining_time:zeroRemaining,
    paired_delta:delta,returned_time_sum:stepTime,max_time_difference_from_f32_broad_delta:maxTimeDifference,
    min_returned_time_over_broad_delta:minTimeRatio,max_returned_time_over_broad_delta:maxTimeRatio,
    velocity_norm_time_weighted:velocityNormTime/delta,
    increment_path_per_delta:summary.increment_path_per_delta,max_xyz_product_residual:maxResidual};
}
const reference=analyze('20260919T170034Z-pid9748');
const high=analyze('20260919T170924Z-pid8088');
const result={reference,high,
  increment_path_rate_ratio:high.increment_path_per_delta/reference.increment_path_per_delta,
  output_velocity_norm_ratio:high.velocity_norm_time_weighted/reference.velocity_norm_time_weighted,
  interpretation:'The increment and returned vector are already approximately half-amplitude per delta at 165. Returned time closely tracks broad delta (see exact-match count and residual), remaining time is zero, and displacement is bit-exact velocity times returned time. This does not establish whether 02064FF0 receives reduced input or creates it internally.',
  caveat:'Single captures localize arithmetic, not patch acceptance; reference facing changed 0.218 rad. XYZ norms use all first three memory lanes, avoiding inherited axis labels.'};
fs.mkdirSync(path.dirname(path.resolve(outputArg)),{recursive:true});
fs.writeFileSync(outputArg,JSON.stringify(result,null,2)+'\n');
console.log(JSON.stringify(result,null,2));
