import fs from 'node:fs';
import path from 'node:path';
import assert from 'node:assert/strict';
const [sessionArg,outArg,trialArg='2']=process.argv.slice(2);assert(sessionArg&&outArg);
const session=path.resolve(sessionArg),out=path.resolve(outArg);
function csv(name){const [h,...ls]=fs.readFileSync(path.join(session,name),'utf8').trim().split(/\r?\n/),k=h.split(',');return ls.map(l=>Object.fromEntries(l.split(',').map((v,i)=>[k[i],v])));}
const e=csv('locomotion-events.csv').filter(x=>x.trial===trialArg),end=e.find(x=>x.event==='measure_end');assert(end);
const q=Number(end.qpc),info=fs.readFileSync(path.join(session,'session-info.txt'),'utf8');
const frequency=Number(info.match(/qpc_frequency=(\d+)/)[1]);
const rows=csv('semantic-state.csv'),bins=[];
for(let j=-2;j<16;j++){
  const lo=q+j*frequency/2,hi=lo+frequency/2,r=rows.filter(x=>Number(x.qpc)>=lo&&Number(x.qpc)<hi);if(r.length<2)continue;
  let d=0,t=0;for(let i=1;i<r.length;i++){
    assert.equal(Number(r[i].update_index),Number(r[i-1].update_index)+1);
    d+=Math.hypot(...['x','z','y'].map(k=>Number(r[i][k])-Number(r[i-1][k])));t+=Number(r[i-1].delta);
  }
  bins.push({seconds_from_measure_end:j/2,updates:r.length,path_per_delta:d/t,
    start_stamina:Number(r[0].stamina),end_stamina:Number(r.at(-1).stamina),
    keys:[...new Set(r.map(x=>x.keyboard_mask))],animations:[...new Set(r.map(x=>x.animation_id))]});
}
const result={session:path.basename(session),trial:Number(trialArg),patch_lifetime:info.match(/patch_lifetime=(.*)/)?.[1]?.trim()??'legacy-measurement-window-only',
  measure_end_update:Number(end.update_index),trial_end:e.find(x=>x.event==='trial_end')??null,bins,
  note:'Descriptive post-window timing, not an automatic gameplay pass. Native gait change at depleted stamina must remain distinct from half-amplitude sprint.'};
fs.mkdirSync(out,{recursive:true});fs.writeFileSync(path.join(out,`patch-tail-trial-${trialArg}.json`),JSON.stringify(result,null,2)+'\n');console.log(JSON.stringify(result,null,2));
