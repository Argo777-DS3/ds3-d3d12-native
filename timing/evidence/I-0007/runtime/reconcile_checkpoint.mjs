// Offline recovery audit. Never loads a probe or touches the game process.
import fs from 'node:fs';
import path from 'node:path';
import crypto from 'node:crypto';
import assert from 'node:assert/strict';
import {spawnSync} from 'node:child_process';
import {fileURLToPath} from 'node:url';
const here=path.dirname(fileURLToPath(import.meta.url));
const out=path.join(here,'reconciliation-20260920');
fs.mkdirSync(out,{recursive:true});
const hash=p=>crypto.createHash('sha256').update(fs.readFileSync(p)).digest('hex').toUpperCase();
const read=p=>JSON.parse(fs.readFileSync(p,'utf8').replace(/^\uFEFF/,''));
const checks=[];
for(const session of fs.readdirSync(path.join(here,'analysis'))){
  const dir=path.join(here,'analysis',session);
  if(!fs.statSync(dir).isDirectory())continue;
  for(const name of fs.readdirSync(dir).filter(n=>n.endsWith('summary.json'))){
    const summary=read(path.join(dir,name));
    if(!summary.raw_sha256)continue;
    for(const [file,expected] of Object.entries(summary.raw_sha256)){
      const p=path.join(here,'sessions',session,file);
      const actual=fs.existsSync(p)?hash(p):null;
      checks.push({session,summary:name,file,expected,actual,match:actual===expected});
    }
  }
}
const jobs=[
 ['analyze_feedback_mirror.mjs','20260919T181015Z-pid20840','feedback-summary.json'],
 ['analyze_causal_patch.mjs','20260919T184047Z-pid3804','causal-patch-summary.json'],
 ['analyze_causal_patch.mjs','20260919T185120Z-pid3804','causal-patch-summary.json'],
 ['analyze_causal_patch.mjs','20260919T185858Z-pid3804','causal-patch-summary.json'],
 ['analyze_roll_capture.mjs','20260919T185858Z-pid3804','roll-summary.json']
];
const replays=[];
for(const [script,session,summary] of jobs){
  const target=path.join(out,session);
  const r=spawnSync(process.execPath,[path.join(here,script),path.join(here,'sessions',session),target],{encoding:'utf8',maxBuffer:16*1024*1024});
  assert.equal(r.status,0,r.stderr);
  const original=read(path.join(here,'analysis',session,summary)), replay=read(path.join(target,summary));
  // Persist mismatches rather than silently accepting a stale derived record.
  let identical=true;try{assert.deepEqual(replay,original);}catch{identical=false;}
  let reconciled=identical, schema_note=null;
  if(!identical && session==='20260919T184047Z-pid3804' && summary==='causal-patch-summary.json'){
    const legacy=structuredClone(replay);
    for(const t of legacy.trials){
      assert.equal(t.post_measure_updates,0);assert.equal(t.input_release_end,false);
      assert(!Object.hasOwn(original.trials.find(x=>x.trial===t.trial),'post_measure_updates'));
      delete t.post_measure_updates;delete t.input_release_end;
    }
    assert.deepEqual(legacy,original);
    reconciled=true;schema_note='V3 analyzer adds post_measure_updates=0 and input_release_end=false to the older V2 trials. All historical fields match exactly. This preserves the documented V2 lifetime limitation.';
  }
  replays.push({script,script_sha256:hash(path.join(here,script)),session,summary,identical,reconciled,schema_note});
}
const configs=['instrumentation','increment-instrumentation','velocity-instrumentation','builder-instrumentation','feedback-instrumentation','causal-patch','roll-capture'].map(dir=>{
  const file=path.join(here,dir,'pass_c_config.txt');
  const text=fs.readFileSync(file,'utf8');
  return {directory:dir,sha256:hash(file),timing_disabled:/^enable_timing_override=0\r?$/m.test(text),trials_disabled:/^locomotion_trials=0\r?$/m.test(text),patch_disabled:!/^causal_distance_patch=1\r?$/m.test(text)};
});
const report={recovered_commit:'92c2b79',previous_main:'7a7c357',checks,raw_checks:checks.length,raw_mismatches:checks.filter(c=>!c.match).length,replays,configs,limitations:['Original I-0007 probe binaries were ignored in Git and are absent from the recovered checkout. Historical binary hashes remain provenance, not a present binary verification.','No live acquisition, injection or game launch was performed.']};
fs.writeFileSync(path.join(out,'recovery-audit.json'),JSON.stringify(report,null,2)+'\n');
console.log(JSON.stringify({raw_checks:report.raw_checks,raw_mismatches:report.raw_mismatches,replays,configs},null,2));
assert.equal(report.raw_mismatches,0);
assert(replays.every(r=>r.reconciled),'Derived replay differs; inspect before acceptance');
assert(configs.every(c=>c.timing_disabled&&c.trials_disabled&&c.patch_disabled));
