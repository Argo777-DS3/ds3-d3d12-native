// Isolated arithmetic model, not a game patch or a gameplay acceptance test.
import fs from 'node:fs';
import path from 'node:path';
import {fileURLToPath} from 'node:url';
import assert from 'node:assert/strict';
const here=path.dirname(fileURLToPath(import.meta.url));
const f=Math.fround;
const native=(s,active,reduce)=>!active?s:reduce?(s>0.5?f(s*f(0.8)):0.5):(s<1?f(s*f(1.2)):1);
const consumer=s=>s<1?s:1;
const naive=(s,active,reduce,dt)=>!active?s:reduce?(s>0.5?f(s*Math.pow(f(0.8),dt*60)):0.5):(s<1?f(s*Math.pow(f(1.2),dt*60)):1);
// Known native overshoots and delayed resets must remain in the60 reference.
assert.equal(native(f(.5120000243186951),true,true),f(.40960001945495605));
assert.equal(native(f(1.0368001461029053),true,false),1);
assert.equal(consumer(f(1.152000069618225)),1);
const report=[];
for(const start of [1,.5]){
  const reduce=start===1;
  for(const hz of [60,165]){
    let s=start,weighted=0;
    const steps=[];
    for(let i=0;i<12;i++){
      const before=s;weighted+=consumer(s)/hz;s=native(s,true,reduce);
      steps.push({call:i+1,elapsed_s:(i+1)/hz,before,consumed:consumer(before),after:s});
    }
    report.push({kind:'conditional_native_response',start,branch:reduce?'reduce':'recover',hz,steps,weighted_unit_source_time_s:weighted,
      assumptions:'Same branch and active invocation once per update; unit-rate source and source-construction gate active; preceding scalar consumed before recurrence. This is not observed DS3 displacement.'});
  }
}
// At identical total time, splitting an eligible update changes native response.
const split={initial:1,whole_1_over60:native(1,true,true),two_half_updates:native(native(1,true,true),true,true)};
assert.notEqual(split.whole_1_over60,split.two_half_updates);
// Even exponential gain scaling fails to compose across the native reset branch.
const reset={initial:.6,whole_2_over60:naive(.6,true,true,2/60),two_1_over60:naive(naive(.6,true,true,1/60),true,true,1/60)};
assert.notEqual(reset.whole_2_over60,reset.two_1_over60);
// Gaps introduce a policy question: inactive invocations preserve scalar, so
// charging the whole wall interval on resume is not equivalent to active time.
const inactive={initial:.5,after_120_skips:Array.from({length:120}).reduce(s=>native(s,false,false),.5),one_active_recovery:native(.5,true,false)};
assert.equal(inactive.after_120_skips,.5);
const result={classification:'OFFLINE CONDITIONAL MODEL; no live modification',native_split:split,exponential_reset_counterexample:reset,inactive_reference:inactive,scenarios:report,
 conclusion:'Native response is invocation-count dependent. Naive exponential gains do not preserve reset composition; wall-gap catch-up changes inactive reference behavior. No unique corrected recurrence follows from observed steady60 clock equality. Must qualify active simulation time and conditional consumer effect before a live prototype.'};
const out=path.join(here,'analysis/transient-20260920-60/recurrence-response-model.json');
fs.writeFileSync(out,JSON.stringify(result,null,2)+'\n');
console.log(JSON.stringify({native_split:split,exponential_reset_counterexample:reset,inactive_reference:inactive,conclusion:result.conclusion},null,2));
