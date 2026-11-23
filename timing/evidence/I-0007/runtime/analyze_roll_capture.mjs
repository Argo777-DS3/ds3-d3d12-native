import fs from 'node:fs';
import path from 'node:path';
import assert from 'node:assert/strict';
const pos=r=>['before_x','before_z','before_y'].map(k=>Number(r[k]));
const distance=(a,b)=>Math.hypot(...a.map((v,i)=>v-b[i]));
function pulseCount(masks){let previous=false,n=0;for(const mask of masks){const down=(Number(mask)&32)!==0;if(down&&!previous)n++;previous=down;}return n;}
function releaseBracket(observations){
  const down=o=>(Number(o.keys)&32)!==0;
  const rises=observations.filter((o,i)=>down(o)&&(i===0||!down(observations[i-1])));
  const falls=observations.map((o,i)=>i>0&&!down(o)&&down(observations[i-1])?i:-1).filter(i=>i>=0);
  assert.equal(rises.length,1,'Exactly one Space impulse');assert.equal(falls.length,1,'Exactly one release');
  const i=falls[0];return {pressed:rises[0],last_held:observations[i-1],first_released:observations[i]};
}
if(process.argv[2]==='--self-test'){
  assert.equal(pulseCount([0,1,33,33,1,0]),1);assert.equal(pulseCount([0,33,0,33,0]),2);
  assert.equal(distance([3,4,0],[0,0,0]),5);console.log('PASS: impulse counting and displacement.');
  const bracket=releaseBracket([{keys:0},{keys:33,n:1},{keys:33,n:2},{keys:1,n:3},{keys:0}]);
  assert.equal(bracket.last_held.n,2);assert.equal(bracket.first_released.n,3);
  assert.throws(()=>releaseBracket([{keys:0},{keys:33},{keys:0},{keys:33},{keys:0}]));
  console.log('PASS: unique release bracket and multiple-action rejection.');
}else{
  const [s,o]=process.argv.slice(2);assert(s&&o);const session=path.resolve(s),out=path.resolve(o);
  function csv(name){const [h,...ls]=fs.readFileSync(path.join(session,name),'utf8').trim().split(/\r?\n/),k=h.split(',');return ls.map(l=>Object.fromEntries(l.split(',').map((v,i)=>[k[i],v])));}
  const info=fs.readFileSync(path.join(session,'session-info.txt'),'utf8');assert(info.includes('locomotion_action=single_forward_roll'));
  const frequency=Number(info.match(/qpc_frequency=(\d+)/)[1]);assert(frequency>0);
  const all=csv('locomotion-updates.csv'),events=csv('locomotion-events.csv'),trials=[];
  for(const id of [...new Set(all.map(r=>r.trial))]){
    const u=all.filter(r=>r.trial===id&&r.phase==='measure');assert(u.length>2);
    const e=events.filter(r=>r.trial===id);assert.equal(e.filter(x=>x.event==='trial_end').length,1,'Wait for complete bounded capture');
    const inputViolations=[];
    for(let i=0;i<u.length;i++){
      const r=u[i];if(i)assert.equal(Number(r.update_index),Number(u[i-1].update_index)+1);
      for(const k of ['before_position_valid','after_position_valid','before_animation_valid','after_animation_valid'])assert.equal(r[k],'1');
      for(const prefix of ['before','after']){
        const extra=Number(r[prefix+'_keyboard_mask'])&~33;
        if(extra)inputViolations.push({update:Number(r.update_index),sample:prefix,extra_keys:extra,animation_id:Number(r[prefix+'_animation_id'])});
        assert.equal(Number(r[prefix+'_controller_buttons']),0);
        for(const axis of ['left_x','left_y','right_x','right_y'])assert(Math.abs(Number(r[prefix+'_'+axis]))<=4096);
        for(const axis of ['left_trigger','right_trigger'])assert.equal(Number(r[prefix+'_'+axis]),0);
      }
    }
    const deltas=u.map(r=>Number(r.delta)).sort((a,b)=>a-b),median=deltas[Math.floor(deltas.length/2)];
    const masks=u.flatMap(r=>[r.before_keyboard_mask,r.after_keyboard_mask]);
    const observations=u.flatMap(r=>['before','after'].map(prefix=>({update:Number(r.update_index),keys:Number(r[prefix+'_keyboard_mask']),qpc:Number(r['qpc_'+prefix]),position:['x','z','y'].map(k=>Number(r[prefix+'_'+k]))})));
    const bracket=releaseBracket(observations),releaseIndex=u.findIndex(r=>Number(r.update_index)===bracket.first_released.update);
    let remainingPath=0,lastMotion=releaseIndex;
    for(let i=releaseIndex+1;i<u.length;i++){const d=distance(pos(u[i-1]),pos(u[i]));remainingPath+=d;if(d>0.0001)lastMotion=i;}
    const terminal=pos(u.at(-1)),lastMotionIndex=Number(u[lastMotion].update_index);
    const postActionInputOnly=inputViolations.every(v=>v.update>lastMotionIndex&&v.animation_id===0);
    assert(postActionInputOnly,'Unexpected input overlaps movement or non-idle action');
    const releaseDistances=[distance(bracket.first_released.position,terminal),distance(bracket.last_held.position,terminal)];
    const action={release_bracket:bracket,held_wall_seconds:(bracket.first_released.qpc-bracket.pressed.qpc)/frequency,
      net_distance:releaseDistances[0],path_length:remainingPath,
      release_sample_distance_range:[Math.min(...releaseDistances),Math.max(...releaseDistances)],
      release_sampling_seconds:(bracket.first_released.qpc-bracket.last_held.qpc)/frequency,
      last_motion_update:lastMotionIndex,moving_wall_seconds:(Number(u[lastMotion].qpc_before)-bracket.first_released.qpc)/frequency,
      sum_governing_delta:u.slice(releaseIndex,lastMotion).reduce((s,r)=>s+Number(r.delta),0)};
    const segments=[];let first=0;
    function segment(last){
      const r=u.slice(first,last+1),endpoint=u[Math.min(last+1,u.length-1)];let length=0;
      for(let i=first;i<=last&&i+1<u.length;i++)length+=distance(pos(u[i]),pos(u[i+1]));
      segments.push({animation_id:Number(r[0].before_animation_id),first_update:Number(r[0].update_index),last_update:Number(r.at(-1).update_index),
        updates:r.length,clock_start:Number(r[0].before_animation_time),clock_end:Number(r.at(-1).before_animation_time),
        animation_max_values:[...new Set(r.map(x=>Number(x.before_animation_max)))],
        net_distance:distance(pos(r[0]),pos(endpoint)),path_length:length,
        points:r.map(x=>({update:Number(x.update_index),clock:Number(x.before_animation_time),delta:Number(x.delta),position:pos(x)})),endpoint:pos(endpoint)});
    }
    for(let i=1;i<u.length;i++)if(u[i].before_animation_id!==u[i-1].before_animation_id||Number(u[i].before_animation_time)<Number(u[i-1].before_animation_time)-0.05){segment(i-1);first=i;}
    segment(u.length-1);
    let totalPath=0;for(let i=1;i<u.length;i++)totalPath+=distance(pos(u[i-1]),pos(u[i]));
    const edgeSamples=Math.max(2,Math.floor(0.15/median));
    const stationary=r=>Math.max(...r.map(x=>distance(pos(r[0]),pos(x))))<0.005;
    trials.push({trial:Number(id),target_from_delta:Math.abs(median-1/60)<Math.abs(median-1/165)?60:165,median_delta:median,updates:u.length,
      space_presses:pulseCount(masks),initial_keys:Number(masks[0]),final_keys:Number(masks.at(-1)),
      strict_full_window_input_pass:inputViolations.length===0,input_violations:inputViolations,unexpected_inputs_after_motion_in_idle_only:postActionInputOnly,action,
      initial_stationary:stationary(u.slice(0,edgeSamples)),final_stationary:stationary(u.slice(-edgeSamples)),
      full_window_net:distance(pos(u[0]),pos(u.at(-1))),full_window_path:totalPath,segments});
  }
  const low=trials.find(t=>t.target_from_delta===60),high=trials.find(t=>t.target_from_delta===165);
  const comparison=low&&high?{anchor:'Unique Space-release observation to stationary endpoint; prior-held/first-released sample bracket retained',
    net_ratio:high.action.net_distance/low.action.net_distance,path_ratio:high.action.path_length/low.action.path_length,
    duration_ratio:high.action.moving_wall_seconds/low.action.moving_wall_seconds,
    distance_per_delta_ratio:(high.action.net_distance/high.action.sum_governing_delta)/(low.action.net_distance/low.action.sum_governing_delta),
    release_bracket_ratio_range:[high.action.release_sample_distance_range[0]/low.action.release_sample_distance_range[1],high.action.release_sample_distance_range[1]/low.action.release_sample_distance_range[0]]}:null;
  if(comparison)comparison.distance_and_release_bracket_within_2_percent=[comparison.net_ratio,...comparison.release_bracket_ratio_range].every(r=>Math.abs(r-1)<=0.02);
  const report={session:path.basename(session),trials,comparison,accepted:false,note:'Motion-only measurement; combine exact causal trace, restoration and operator confirmation. Unexpected input remains explicitly recorded; never waive overlap with movement/action. I-frames are deferred and unmeasured.'};
  fs.mkdirSync(out,{recursive:true});fs.writeFileSync(path.join(out,'roll-summary.json'),JSON.stringify(report,null,2)+'\n');
  console.log(JSON.stringify({...report,trials:trials.map(t=>({...t,segments:t.segments.map(({points,...x})=>x)}))},null,2));
}
